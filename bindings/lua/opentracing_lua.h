#ifndef OPENTRACINGC_OPENTRACING_LUA_H
#define OPENTRACINGC_OPENTRACING_LUA_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <opentracing-c/visibility.h>

OPENTRACINGC_EXPORT int luaopen_opentracing(lua_State* l);

#endif /* OPENTRACINGC_OPENTRACING_LUA_H */
