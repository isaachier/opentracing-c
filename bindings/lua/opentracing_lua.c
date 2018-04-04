#include "opentracing_lua.h"

#include <math.h>
#include <string.h>

#include <opentracing-c/tracer.h>

#define TRACER_MT "opentracing_Tracer_mt"
#define SPAN_MT "opentracing_Span_mt"
#define SPAN_CONTEXT_MT "opentracing_SpanContext_mt"
#define NS_PER_S 1000000000

static void* l_alloc(lua_State* l, void* ptr, size_t osize, size_t nsize)
{
    lua_Alloc alloc_fn;
    void* udata;
    alloc_fn = lua_getallocf(l, &udata);
    return alloc_fn(udata, ptr, osize, nsize);
}

static void l_free(lua_State* l, void* ptr)
{
    l_alloc(l, ptr, 0, 0);
}

static void* l_malloc(lua_State* l, size_t size)
{
    return l_alloc(l, NULL, 0, size);
}

static int push_span(lua_State* l, opentracing_span* span);
static int push_span_context(lua_State* l,
                             opentracing_span_context* span_context);

static void
get_time_value(lua_State* l, int index, opentracing_time_value* time_value)
{
    switch (lua_type(l, index)) {
    case LUA_TTABLE:
    case LUA_TUSERDATA:
        lua_getfield(l, index, "tv_sec");
        time_value->tv_sec = luaL_checkinteger(l, -1);
        lua_pop(l, 1);
        lua_getfield(l, index, "tv_nsec");
        time_value->tv_nsec = luaL_checkinteger(l, -1);
        lua_pop(l, 1);
        break;
    case LUA_TNUMBER: {
        const lua_Number sec = luaL_checknumber(l, index);
        time_value->tv_sec = floor(sec);
        time_value->tv_nsec = (sec - time_value->tv_sec) * NS_PER_S;
    } break;
    default:
        lua_pushliteral(l, "invalid type for time value");
        lua_error(l);
    }
}

static int get_reference(lua_State* l, int value_index, void* addr)
{
    opentracing_span_reference* ref = addr;
    opentracing_span_context** span_context;
    switch (lua_type(l, value_index)) {
    case LUA_TTABLE:
    case LUA_TUSERDATA:
        break;
    default:
        lua_pushliteral(l, "invalid reference");
        return -1;
    }

    lua_getfield(l, value_index, "type");
    if (!lua_isstring(l, -1)) {
        lua_pop(l, 1);
        lua_pushliteral(l, "invalid reference type");
        return -1;
    }
    const char* type_str = lua_tostring(l, -1);
    if (strcmp(type_str, "CHILD_OF") == 0) {
        ref->type = opentracing_span_reference_child_of;
    }
    else if (strcmp(type_str, "FOLLOWS_FROM") == 0) {
        ref->type = opentracing_span_reference_follows_from;
    }
    else {
        lua_pop(l, 1);
        lua_pushliteral(l, "invalid reference type");
        return -1;
    }
    lua_pop(l, 1);

    lua_getfield(l, value_index, "referencedContext");
    span_context = luaL_testudata(l, value_index, SPAN_CONTEXT_MT);
    lua_pop(l, 1);
    if (span_context == NULL) {
        lua_pushliteral(l, "invalid reference type");
        return -1;
    }
    ref->referenced_context = *span_context;
    return 0;
}

static int
get_sequence(lua_State* l,
             int index,
             void** seq,
             int* num_elems,
             size_t size,
             int (*converter)(lua_State* l, int value_index, void* output))
{
    char* array;
    size_t alloc_size;
    int i;

    lua_len(l, index);
    *num_elems = luaL_checkinteger(l, -1);
    lua_pop(l, 1);
    if (*num_elems == 0) {
        *seq = NULL;
        return 0;
    }

    alloc_size = (*num_elems) * size;
    array = l_malloc(l, alloc_size);
    if (array == NULL) {
        lua_pushliteral(l, "cannot allocate sequence");
        lua_error(l);
    }
    memset(array, 0, alloc_size);

    lua_pushnil(l);
    while (lua_next(l, index) != 0) {
        if (converter(l, -1, &array[i * size]) != 0) {
            goto cleanup;
        }
        i++;
        lua_pop(l, 1);
    }
    lua_pop(l, 1);
    *seq = (void**) array;
    return 0;

cleanup:
    lua_pop(l, 2);
    l_free(l, array);
    return -1;
}

static void get_start_span_options(lua_State* l,
                                   int index,
                                   opentracing_start_span_options* options)
{
    lua_getfield(l, 3, "startTimeSteady");
    if (!lua_isnil(l, -1)) {
        get_time_value(l, -1, &options->start_time_steady.value);
    }
    lua_pop(l, 1);

    lua_getfield(l, 3, "startTimeSystem");
    if (!lua_isnil(l, -1)) {
        get_time_value(l, -1, &options->start_time_system.value);
    }
    lua_pop(l, 1);

    lua_getfield(l, 3, "references");
    if (!lua_isnil(l, -1)) {
        opentracing_span_reference* refs;
        if (get_sequence(l,
                         -1,
                         (void**) &refs,
                         &options->num_references,
                         sizeof(opentracing_span_reference),
                         &get_reference) != 0) {
            lua_error(l);
        }
    }
    lua_pop(l, 1);
}

static int get_log_field(lua_State* l, int value_index, void* addr)
{
    int key_index;
    opentracing_log_field* field;

    key_index = value_index - 1;
    field = addr;

    switch (lua_type(l, value_index)) {
    case LUA_TNIL:
        field->value.type = opentracing_value_null;
        break;
    case LUA_TBOOLEAN:
        field->value.type = opentracing_value_bool;
        field->value.value.bool_value = luaL_checkbool(l, value_index)
                                            ? opentracing_true
                                            : opentracing_false;
        break;
    case LUA_TNUMBER:
        if (lua_isinteger(l, value_index)) {
            int result;
            lua_pushinteger(l, 0);
            result = lua_compare(l, value_index, -1, LUA_OPLT);
            lua_pop(l, 1);
            if (result) {
                field->value.type = opentracing_value_int64;
                field->value.value.int64_value =
                    luaL_checkinteger(l, value_index);
            }
            else {
                field->value.type = opentracing_value_uint64;
                field->value.value.uint64_value =
                    luaL_checkinteger(l, value_index);
            }
        }
        else {
            field->value.type = opentracing_value_double;
            field->value.value.double_value = luaL_checknumber(l, value_index);
        }
        break;
    case LUA_TSTRING:
        field->value.type = opentracing_value_string;
        /* Not allocating new string and will not manually free lua string. */
        field->value.value.string_value =
            (char*) luaL_checkstring(l, value_index);
        break;
    default:
        lua_pushliteral(l, "invalid log field value");
        return -1;
    }

    if (!lua_isstring(l, key_index)) {
        lua_pushliteral(l, "invalid log field key");
        return -1;
    }
    /* Not allocating new string and will not manually free lua string. */
    field->key = (char*) lua_tostring(l, key_index);

    return 0;
}

static int get_log_record(lua_State* l, int value_index, void* addr)
{
    opentracing_log_record* log_record;
    int result;

    switch (lua_type(l, value_index)) {
    case LUA_TTABLE:
    case LUA_TUSERDATA:
        break;
    default:
        lua_pushliteral(l, "invalid log record");
        return -1;
    }

    log_record = addr;

    lua_getfield(l, value_index, "timestamp");
    if (!lua_isnil(l, -1)) {
        get_time_value(l, -1, &log_record->timestamp.value);
    }
    lua_pop(l, 1);

    lua_getfield(l, value_index, "fields");
    result = get_sequence(l,
                          -1,
                          (void**) &log_record->fields,
                          &log_record->num_fields,
                          sizeof(opentracing_log_record),
                          &get_log_field);
    lua_pop(l, 1);
    return result;
}

static void get_finish_span_options(lua_State* l,
                                    int index,
                                    opentracing_finish_span_options* options)
{
    lua_getfield(l, index, "finishTime");
    if (!lua_isnil(l, -1)) {
        get_time_value(l, -1, &options->finish_time.value);
    }
    lua_pop(l, 1);

    lua_getfield(l, index, "logRecords");
    if (!lua_isnil(l, -1)) {
        if (get_sequence(l,
                         -1,
                         (void**) &options->log_records,
                         &options->num_log_records,
                         sizeof(opentracing_log_record),
                         &get_log_record) != 0) {
            lua_error(l);
        }
    }
    lua_pop(l, 1);
}

static int tracer_destroy(lua_State* l)
{
    opentracing_tracer** tracer = luaL_checkudata(l, 1, TRACER_MT);
    OPENTRACINGC_DEC_REF(tracer);
    return 0;
}

static int tracer_close(lua_State* l)
{
    opentracing_tracer** tracer = luaL_checkudata(l, 1, TRACER_MT);
    (*tracer)->close(*tracer);
    return 0;
}

static int tracer_start_span(lua_State* l)
{
    opentracing_span* span;
    opentracing_tracer** tracer;
    const char* operation_name;
    const int num_args = lua_gettop(l);

    if (num_args < 2 || num_args > 3) {
        lua_pushliteral(l, "incorrect arguments");
        lua_error(l);
    }

    tracer = luaL_checkudata(l, 1, TRACER_MT);
    operation_name = luaL_checkstring(l, 2);

    if (num_args == 3) {
        opentracing_start_span_options options = {0};
        get_start_span_options(l, 3, &options);
        span = (*tracer)->start_span_with_options(
            *tracer, operation_name, &options);
        l_free(l, (void*) options.references);
    }
    else {
        span = (*tracer)->start_span(*tracer, operation_name);
    }
    return push_span(l, span);
}

static const struct luaL_Reg tracer_methods[] = {
    {"__gc", &tracer_destroy},
    {"close", &tracer_close},
    {"startSpan", &tracer_start_span},
    {NULL, NULL}};

static int span_destroy(lua_State* l)
{
    opentracing_span** span = luaL_checkudata(l, 1, SPAN_MT);
    OPENTRACINGC_DEC_REF(*span);
    return 0;
}

static int span_finish(lua_State* l)
{
    int num_args;
    opentracing_span** span;

    num_args = lua_gettop(l);
    if (num_args < 1 || num_args > 2) {
        lua_pushliteral(l, "invalid arguments");
        lua_error(l);
    }

    span = luaL_checkudata(l, 1, SPAN_MT);
    if (num_args == 2) {
        opentracing_finish_span_options options = {0};
        get_finish_span_options(l, 2, &options);
        (*span)->finish_with_options(*span, &options);
        if (options.log_records != NULL) {
            lua_Alloc alloc_fn;
            void* udata;
            alloc_fn = lua_getallocf(l, &udata);
            alloc_fn(udata, (void*) options.log_records, 0, 0);
        }
    }
    else {
        (*span)->finish(*span);
    }

    return 0;
}

static int push_span(lua_State* l, opentracing_span* span)
{
    opentracing_span** addr = lua_newuserdata(l, sizeof(&span));
    *addr = span;
    luaL_setmetatable(l, SPAN_MT);
    return 1;
}

static const struct luaL_Reg span_methods[] = {{NULL, NULL}};

static int span_context_destroy(lua_State* l)
{
    opentracing_span_context** span_context =
        luaL_checkudata(l, 1, SPAN_CONTEXT_MT);
    OPENTRACINGC_DEC_REF(*span_context);
    return 0;
}

typedef struct span_context_callback_arg {
    lua_State* l;
    int callback_index;
} span_context_callback_arg;

static opentracing_bool span_context_for_each_baggage_item_callback(
    void* arg, const char* key, const char* value)
{
#define NUM_ARGS 2
#define NUM_RET 1

    const span_context_callback_arg* callback_arg;
    int status;

    callback_arg = arg;
    lua_pushvalue(callback_arg->l, callback_arg->callback_index);
    lua_pushstring(callback_arg->l, key);
    lua_pushstring(callback_arg->l, value);
    status = lua_pcall(callback_arg->l, NUM_ARGS, NUM_RET, 0);
    if (status != LUA_OK) {
        return opentracing_false;
    }
    if (lua_isboolean(callback_arg->l, -1)) {
        const int result = lua_toboolean(callback_arg->l, -1);
        lua_pop(callback_arg->l, 1);
        return result ? opentracing_true : opentracing_false;
    }

    return opentracing_true;

#undef NUM_ARGS
#undef NUM_RET
}

static int span_context_for_each_baggage_item(lua_State* l)
{
    opentracing_span_context** span_context;
    span_context_callback_arg arg = {l, 2};

    span_context = luaL_checkudata(l, 1, SPAN_CONTEXT_MT);
    luaL_argcheck(l, lua_isfunction(l, 2), 2, "invalid callback");
    (*span_context)
        ->foreach_baggage_item(
            *span_context, &span_context_for_each_baggage_item_callback, &arg);
    return 0;
}

static int push_span_context(lua_State* l,
                             opentracing_span_context* span_context)
{
    opentracing_span_context** addr = lua_newuserdata(l, sizeof(span_context));
    *addr = span_context;
    luaL_setmetatable(l, SPAN_CONTEXT_MT);
    return 1;
}

static const struct luaL_Reg span_context_methods[] = {
    {"__gc", &span_context_destroy},
    {"foreachBaggageItem", &span_context_for_each_baggage_item},
    {NULL, NULL}};

static void
create_metatable(lua_State* l, const luaL_Reg* methods, const char* name)
{
    luaL_newmetatable(l, name);

    /* mt.__index = mt */
    lua_pushliteral(l, "__index");
    lua_pushvalue(l, -2);
    lua_rawset(l, -3);

    luaL_setfuncs(l, methods, 0);
    lua_pop(l, 1);
}

int luaopen_opentracing(lua_State* l)
{
    create_metatable(l, tracer_methods, TRACER_MT);
    create_metatable(l, span_methods, SPAN_MT);
    create_metatable(l, span_context_methods, SPAN_CONTEXT_MT);

    /* TODO: Export lib
    lua_newtable(l);
    */

    return 1;
}
