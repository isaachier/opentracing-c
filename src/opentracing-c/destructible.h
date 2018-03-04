#ifndef OPENTRACINGC_DESTRUCTIBLE_H
#define OPENTRACINGC_DESTRUCTIBLE_H

/** @file */

#define OPENTRACINGC_DESTRUCTIBLE_SUBCLASS                            \
    /**                                                               \
     * Destructor to clean up any resources allocated to this struct. \
     * @param destructible Destructible instance.                     \
     */                                                               \
    void (*destroy)(struct opentracing_destructible * destructible)

/**
 * Destructible interface.
 */
typedef struct opentracing_destructible {
    OPENTRACINGC_DESTRUCTIBLE_SUBCLASS;
} opentracing_destructible;

#endif /* OPENTRACINGC_DESTRUCTIBLE_H */
