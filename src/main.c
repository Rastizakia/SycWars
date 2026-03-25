#include "../include/sycwars.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

sigjmp_buf game_loop;
SysState env;
uint32_t tick = 0;

extern void init_terminal();
extern void flush_screen();
extern void draw_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, char c1, char c2);

void update_logic() {
    for(int x=0; x<50; x++) {
        for(int y=0; y<20; y++) {
            draw_pixel(x, y, (tick+x)%255, (tick+y)%255, tick%255, 0, 0);
        }
    }
}

int main() {
    init_terminal();
    env.game.nan_marker = 0x7FF8;
    env.game.state = 0;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long last_frame = tv.tv_sec * 1000LL + tv.tv_usec / 1000;

    sigsetjmp(game_loop, 1); 

    gettimeofday(&tv, NULL);
    long long now = tv.tv_sec * 1000LL + tv.tv_usec / 1000;
    
    if(now - last_frame < 16) { 
        usleep(500); 
        siglongjmp(game_loop, 1); 
    }
    
    last_frame = now;
    tick++;

    update_logic();
    flush_screen();

    siglongjmp(game_loop, 1);
    return 0;
}
