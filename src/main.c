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

int *arena[60][24];
int safe_mem = 1;

// Entity State
int ai_x, ai_y, ai_hp = 100;
uint32_t bullets[128]; // [life 8][spd 4][dy 4][dx 4][y 6][x 6]

extern void init_terminal();
extern void flush_screen();
extern void draw_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, char c1, char c2);

void draw_text(int x, int y, char *str, uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; str[i] && x + (i / 2) < 60; i += 2)
        draw_pixel(x + (i / 2), y, r, g, b, str[i], str[i + 1] ? str[i + 1] : ' ');
}

void spawn(int s, int e, int x, int y, int dx, int dy, int l, int spd) {
    for (int i = s; i < e; i++) {
        if (!(bullets[i] >> 24)) {
            bullets[i] = (x & 0x3F) | ((y & 0x3F) << 6) | (((dx + 2) & 0xF) << 12) | 
                         (((dy + 2) & 0xF) << 16) | ((spd & 0xF) << 20) | (l << 24);
            break;
        }
    }
}

int main() {
    init_terminal();
    env.game.nan_marker = 0x7FF8;
    env.game.x = 15; env.game.y = 12; env.game.hp = 100;
    env.game.state = 0;
    ai_x = 45; ai_y = 12;

    for(int x=0; x<60; x++) {
        for(int y=0; y<24; y++) {
            arena[x][y] = (x==0||x==59||y==0||y==23) ? NULL : &safe_mem;
        }
    }

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
    int p_dx = 0, p_dy = 0;
    uint8_t action = 0;

    while(read(STDIN_FILENO, &k, 1) > 0) {
        if(k=='q') { raise(SIGINT); }
        if(k==' ') { action |= 1; }
        
        if(env.game.state == 1) {
            if(k=='w') { env.game.y--; p_dy=-1; p_dx=0; }
            if(k=='s') { env.game.y++; p_dy=1; p_dx=0; }
            if(k=='a') { env.game.x--; p_dx=-1; p_dy=0; }
            if(k=='d') { env.game.x++; p_dx=1; p_dy=0; }
        }
    }

    if(env.game.state == 0) {
        draw_text(60/2 - 6, 24/2, "SYCWARS", 255, 255, 255);
        if(action & 1) { env.game.state = 1; printf("\033[2J"); fflush(stdout); }
    } else {
        env.game.px = env.game.x; env.game.py = env.game.y;
        
        // Spawn Player Bullet
        if (action & 1) spawn(0, 64, env.game.x + p_dx, env.game.y + p_dy, p_dx, p_dy, 25, 1);

        // Hardware Wall Collision
        *arena[env.game.x][env.game.y] ^= 1; 

        // AI Logic
        if (tick % 10 == 0) { 
            if (ai_x < env.game.x) ai_x++; else if (ai_x > env.game.x) ai_x--; 
            if (ai_y < env.game.y) ai_y++; else if (ai_y > env.game.y) ai_y--; 
        }
        if (tick % 50 == 0) {
            spawn(64, 128, ai_x, ai_y, (env.game.x > ai_x) ? 1 : -1, (env.game.y > ai_y) ? 1 : -1, 20, 2);
        }

        // Draw Bullets & Logic
        for (int i = 0; i < 128; i++) {
            int l = bullets[i] >> 24; 
            if (l > 0) {
                int bx = bullets[i] & 0x3F, by = (bullets[i] >> 6) & 0x3F;
                int dx = ((bullets[i] >> 12) & 0xF) - 2, dy = ((bullets[i] >> 16) & 0xF) - 2;
                int s = (bullets[i] >> 20) & 0xF;
                
                for (int st = 0; st < s; st++) {
                    bx += dx; by += dy; l--; 
                    if (bx <= 0 || bx >= 59 || by <= 0 || by >= 23) { l = 0; break; }
                    if (i < 64 && bx == ai_x && by == ai_y) { ai_hp -= 10; l = 0; break; }
                    if (i >= 64 && bx == env.game.x && by == env.game.y) { l = 0; raise(SIGALRM); break; }
                }
                
                bullets[i] = (bx & 0x3F) | ((by & 0x3F) << 6) | (((dx + 2) & 0xF) << 12) | (((dy + 2) & 0xF) << 16) | ((s & 0xF) << 20) | (l << 24);
                if (l > 0) draw_pixel(bx, by, (i < 64) ? 0 : 255, (i < 64) ? 255 : 0, 0, (i < 64) ? '!' : '*', ' ');
            }
        }

        for(int x=0; x<60; x++) {
            for(int y=0; y<24; y++) {
                if(arena[x][y] == NULL) draw_pixel(x, y, 40, 40, 40, 0, 0);
            }
        }
        
        draw_pixel(env.game.x, env.game.y, 0, 255, 0, '@', ' ');
        draw_pixel(ai_x, ai_y, 255, 0, 0, 'X', ' ');
        
        printf("\033[H\033[25B\033[0mHP:%03d AI:%03d", env.game.hp, ai_hp);
    }

    flush_screen();
    siglongjmp(game_loop, 1);
    return 0;
}
