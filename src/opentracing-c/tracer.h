#ifndef OPENTRACINGC_TRACER_H
#define OPENTRACINGC_TRACER_H

#include <opentracing-c/span.h>

typedef struct opentracing_start_span_options {
    opentracing_duration start_time_steady;
    opentracing_timestamp start_time_system;
    /* TODO: references and tags */
} opentracing_start_span_options;

#define OPENTRACINGC_TRACER_SUBCLASS                                          \
    /**                                                                       \
     * Equivalent to calling start_span_with_options with options as          \
     * NULL.                                                                  \
     * @param tracer Tracer instance.                                         \
     * @param operation_name Name of operation associated with span.          \
     * @return Span pointer on success, NULL otherwise.                       \
     */                                                                       \
    opentracing_span* (*start_span)(struct opentracing_tracer * tracer,       \
                                    const char* operation_name);              \
                                                                              \
    /**                                                                       \
     * Start a new span with provided options.                                \
     * @param tracer Tracer instance.                                         \
     * @param operation_name Name of operation associated with span.          \
     * @param options Options to override default span initialization values. \
     * @return Span pointer on success, NULL otherwise.                       \
     */                                                                       \
    opentracing_span* (*start_span_with_options)(                             \
        struct opentracing_tracer * tracer,                                   \
        const opentracing_start_span_options* options)

typedef struct opentracing_tracer {
    OPENTRACINGC_TRACER_SUBCLASS;
} opentracing_tracer;

#endif /* OPENTRACINGC_TRACER_H */
