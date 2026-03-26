#ifndef SYCWARS_H
#define SYCWARS_H

#include <stdint.h>
#include <setjmp.h>

typedef union {
    double f;
    uint64_t bits;
    struct {
        uint32_t x : 7;
        uint32_t y : 7;
        uint32_t px : 7;
        uint32_t py : 7;
        uint32_t state : 4;
        int32_t hp : 8;
        uint32_t padding : 8;
        uint16_t nan_marker;
    } game;
} SysState;

typedef struct {
    uint8_t r, g, b;
    char c1, c2;
} Pixel;

extern sigjmp_buf game_loop;
extern SysState env;
extern uint32_t tick;

#endif
