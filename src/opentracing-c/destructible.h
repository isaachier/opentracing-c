#ifndef OPENTRACINGC_DESTRUCTIBLE_H
#define OPENTRACINGC_DESTRUCTIBLE_H

/** @file */

/**
 * Destructible interface. Essentially the same as a virtual destructor in C++,
 * but for C.
 */
typedef struct opentracing_destructible {
    /**
     * Destructor to clean up any resources allocated to the instance.
     * @param destructible Destructible instance.
     */
    void (*destroy)(struct opentracing_destructible* destructible);
} opentracing_destructible;

#endif /* OPENTRACINGC_DESTRUCTIBLE_H */
