#ifndef OPENTRACINGC_COMMON_H
#define OPENTRACINGC_COMMON_H

#include <time.h>

/** Boolean type. */
typedef enum { opentracing_true = 1, opentracing_false = 0 } opentracing_bool;

/** Duration type to calculate precise intervals (should use CLOCK_MONOTIC). */
typedef struct opentracing_duration {
    /** Duration value. */
    struct timespec value;
} opentracing_duration;

/** Timestamp type to represent absolute time (should use CLOCK_REALTIME). */
typedef struct opentracing_timestamp {
    /** Timestamp value. */
    struct timespec value;
} opentracing_timestamp;

#endif /* OPENTRACINGC_COMMON_H */