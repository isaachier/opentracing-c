#ifndef OPENTRACINGC_DESTRUCTIBLE_H
#define OPENTRACINGC_DESTRUCTIBLE_H

#include <opentracing-c/config.h>

/** @file */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Destructible interface. Essentially the same as a virtual destructor in C++,
 * but for C.
 */
typedef struct opentracing_destructible {
    /**
     * Destructor to clean up any resources allocated to the instance.
     * @param destructible Destructible instance.
     */
    void (*destroy)(struct opentracing_destructible* destructible)
        OPENTRACINGC_NONNULL_ALL;

    /** Reference count to avoid deallocating in-use objects. */
    int ref_count;
} opentracing_destructible;

/**
 * Convenience macro to increment reference count of destructible object.
 */
#define OPENTRACINGC_INC_REF(destructible)              \
    do {                                                \
        opentracing_destructible* d =                   \
            (opentracing_destructible*) (destructible); \
        d->ref_count++;                                 \
    } while (0)

/**
 * Convenience macro to decrement reference count of destructible object.
 */
#define OPENTRACINGC_DEC_REF(destructible)              \
    do {                                                \
        opentracing_destructible* d =                   \
            (opentracing_destructible*) (destructible); \
        if (--(d->ref_count) == 0) {                    \
            d->destroy(d);                              \
        }                                               \
    } while (0)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OPENTRACINGC_DESTRUCTIBLE_H */
