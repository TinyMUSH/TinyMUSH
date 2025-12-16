#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include "config.h"
#include "constants.h"
#include "typedefs.h"
#include "macros.h"

// Minimal prototypes to avoid prototypes.h
extern COLORMATCH getColorMatch(rgbColor rgb, uint8_t schemes);

static uint8_t parse_scheme(const char *s)
{
    if (!s)
        return (uint8_t)(COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS);
    if (!strcmp(s, "ansi"))
        return COLOR_SCHEME_ANSI;
    if (!strcmp(s, "xterm"))
        return COLOR_SCHEME_XTERM;
    if (!strcmp(s, "css"))
        return COLOR_SCHEME_CSS;
    if (!strcmp(s, "all"))
        return (uint8_t)(COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS);
    return (uint8_t)(COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS);
}

static inline uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

int main(int argc, char **argv)
{
    const int iters = (argc > 1) ? atoi(argv[1]) : 200000;
    const uint8_t schemes = (argc > 2) ? parse_scheme(argv[2]) : (uint8_t)(COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS);

    uint64_t t0 = now_ns();
    // Warm-up and populate cache
    for (int i = 0; i < 1000; ++i)
    {
        rgbColor c = {(uint8_t)(i % 256), (uint8_t)((i * 3) % 256), (uint8_t)((i * 7) % 256)};
        (void)getColorMatch(c, schemes);
    }
    uint64_t t1 = now_ns();

    // Measure repeated queries to exercise cache hits
    rgbColor hot = {255, 0, 255};
    for (int i = 0; i < 10000; ++i)
    {
        (void)getColorMatch(hot, schemes);
    }
    uint64_t t2 = now_ns();

    // Mixed unique queries
    for (int i = 0; i < iters; ++i)
    {
        rgbColor c = {(uint8_t)(i % 256), (uint8_t)((i * 5) % 256), (uint8_t)((i * 11) % 256)};
        (void)getColorMatch(c, schemes);
    }
    uint64_t t3 = now_ns();

    printf("warmup: %0.2f ms\n", (t1 - t0) / 1e6);
    printf("hot (10k): %0.2f ms\n", (t2 - t1) / 1e6);
    printf("mixed (%d): %0.2f ms (schemes=%u)\n", iters, (t3 - t2) / 1e6, schemes);
    return 0;
}
