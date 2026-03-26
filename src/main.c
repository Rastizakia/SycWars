#include "../include/sycwars.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

sigjmp_buf game_loop;
SysState env;
uint32_t tick = 0;

int *arena[60][24];
int safe_mem = 1;

extern void init_terminal();
extern void flush_screen();
extern void draw_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, char c1, char c2);

int main() {
    init_terminal();
    env.game.nan_marker = 0x7FF8;
    env.game.x = 30; env.game.y = 12; env.game.hp = 100;
    env.game.state = 0;

    for(int x=0; x<60; x++) for(int y=0; y<24; y++)
        arena[x][y] = (x==0||x==59||y==0||y==23) ? NULL : &safe_mem;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long last_frame = tv.tv_sec * 1000LL + tv.tv_usec / 1000;

    sigsetjmp(game_loop, 1); 

    gettimeofday(&tv, NULL);
    long long now = tv.tv_sec * 1000LL + tv.tv_usec / 1000;
    if(now - last_frame < 16) { usleep(1000); siglongjmp(game_loop, 1); }
    last_frame = now;
    tick++;

    char k=0;
    while(read(STDIN_FILENO, &k, 1) > 0) {
        if(k=='q') raise(SIGINT);
        if(k==' ') { env.game.state=1; printf("\033[2J"); fflush(stdout); }
        if(env.game.state == 1) {
            env.game.px = env.game.x; env.game.py = env.game.y;
            if(k=='w') env.game.y--; if(k=='s') env.game.y++;
            if(k=='a') env.game.x--; if(k=='d') env.game.x++;
        }
    }

    if(env.game.state == 0) {
        for(int x=0;x<60;x++) draw_pixel(x, 12, tick%255, 255, 255, '#', ' ');
    } else {
        *arena[env.game.x][env.game.y] ^= 1; 

        for(int x=0;x<60;x++) for(int y=0;y<24;y++)
            if(arena[x][y] == NULL) draw_pixel(x, y, 40, 40, 40, 0, 0);
        
        draw_pixel(env.game.x, env.game.y, 0, 255, 0, '@', ' ');
        printf("\033[H\033[25B\033[0mHP:%03d", env.game.hp);
    }

    flush_screen();
    siglongjmp(game_loop, 1);
    return 0;
}
