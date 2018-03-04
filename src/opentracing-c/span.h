#ifndef OPENTRACINGC_SPAN_H
#define OPENTRACINGC_SPAN_H

#include <opentracing-c/value.h>

#define OPENTRACINGC_SPAN_CONTEXT_SUBCLASS                                  \
    /**                                                                     \
     * Calls a function for each baggage item in the span context. If the   \
     * function returns opentracing_false, it will not be called again, and \
     * foreach_baggage_item will return immediately.                        \
     * @param span_context Span context instance.                           \
     * @param f Callback function.                                          \
     * @param arg Argument to pass to callback function.                    \
     */                                                                     \
    void (*foreach_baggage_item)(                                           \
        struct opentracing_span_context * span_context,                     \
        opentracing_bool(*f)(void*, const char*, const char*),              \
        void* arg)

/**
 * Interface for span context. Span context represents span state that must be
 * propagated to descendant spans and across boundaries (e.g. trace ID,
 * span ID, baggage).
 */
typedef struct opentracing_span_context {
    OPENTRACINGC_SPAN_CONTEXT_SUBCLASS;
} opentracing_span_context;

/**
 * Log field to represent key-value pair for a log.
 */
typedef struct opentracing_log_field {
    /** Key string. */
    char* key;
    /** Value representation. */
    opentracing_value value;
} opentracing_log_field;

/**
 * Log record can be used to describe events that occur in the lifetime of a
 * span.
 */
typedef struct opentracing_log_record {
    /** Time of logged event. */
    opentracing_timestamp timestamp;
    /** Array of fields. */
    opentracing_log_field* fields;
    /** Number of fields. If fields is NULL, must be set to zero. */
    int num_fields;
} opentracing_log_record;

/* TODO */
typedef struct opentracing_span opentracing_span;

#endif /* OPENTRACINGC_SPAN_H */
