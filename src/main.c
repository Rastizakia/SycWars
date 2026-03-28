#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/time.h>
#include <stdint.h>

typedef union {
    double f;
    uint64_t bits;
    struct { 
        uint32_t x:7, y:7, px:7, py:7, state:3; 
        int32_t hp:8; 
        uint32_t dmg:5, spd:3, gen:6, arm:4, med:2, inv:1, nan:4; 
    } game;
} SysState;

typedef struct { uint8_t r, g, b; char c1, c2; } Pixel;

sigjmp_buf game_loop; 
struct termios orig;
int W=60, H=24, ai_x, ai_y, ai_hp, hx=-1, hy=-1, ix=-1, iy=-1, fk=0;
int p_dx=1, p_dy=0, vx=0, vy=0, inv_open=0;
int *arena[60][24], safe=1;
uint64_t bullets[128];
uint32_t grems[8], tick=0, inv_end=0;
uint8_t tr_x[15], tr_y[15]; 
Pixel *screen; 
SysState env;

void draw(int x, int y, uint8_t r, uint8_t g, uint8_t b, char c1, char c2, int ph) {
    int px = x, py = y;
    if (env.game.gen == 2) {
        px = x + (py / 3);
    } else if (env.game.gen >= 3) {
        float d = (float)py / H; 
        if(d < 0.2) d = 0.2;
        px = (W / 2) + (int)((px - W / 2) * d);
        r *= d; g *= d; b *= d;
    }
    if (ph >= 3) { r *= 0.4; g *= 0.4; b += 60; if (b > 255) b = 255; }
    if (px >= 0 && px < W && py >= 0 && py < H) screen[py * W + px] = (Pixel){r, g, b, c1, c2};
}

void txt(int x, int y, char *s, uint8_t r, uint8_t g, uint8_t b, int ph) {
    for (int i = 0; s[i] && x + (i / 2) < W; i += 2)
        draw(x + (i / 2), y, r, g, b, s[i], s[i + 1] ? s[i + 1] : ' ', ph);
}

void spawn(int s, int e, int x, int y, int dx, int dy, int l, int spd, uint64_t type) {
    for (int i = s; i < e; i++) {
        if (!((bullets[i] >> 24) & 0xFF)) {
            bullets[i] = (x & 0x3F) | ((y & 0x3F) << 6) | (((dx + 2) & 0xF) << 12) | 
                         (((dy + 2) & 0xF) << 16) | ((spd & 0xF) << 20) | ((uint64_t)l << 24) | (type << 32);
            break;
        }
    }
}

void panic(int s) {
    if (s == SIGINT) {
        printf("\033[0m\033[2J\e[?25h");
        tcsetattr(0, 0, &orig);
        exit(0);
    }
    env.game.x = env.game.px; env.game.y = env.game.py; vx = 0; vy = 0;
    if (s == SIGSEGV) { if (env.game.arm > 0) env.game.arm--; else env.game.hp -= 2; }
    if (s == SIGALRM && tick > inv_end) { if (env.game.arm > 0) env.game.arm -= 2; else env.game.hp -= 10; }
    siglongjmp(game_loop, 1);
}

void persist() {
    char b[32]; sprintf(b, "%lu", env.bits); setenv("SYC_SOUL", b, 1);
}

void reset(int w) {
    env.game.x = W / 4; env.game.y = H / 2; env.game.hp = 100; env.game.arm = 10;
    ai_x = W * 3 / 4; ai_y = H / 2; ai_hp = 400 + (env.game.gen * 150);
    vx = 0; vy = 0; fk = 0; hx = -1; ix = -1; inv_open = 0;
    memset(bullets, 0, sizeof(bullets)); memset(grems, 0, sizeof(grems));
    if (w) for (int x = 0; x < W; x++) for (int y = 0; y < H; y++)
        arena[x][y] = (x == 0 || x == W - 1 || y == 0 || y == H - 1) ? 0 : &safe;
    for (int i = 0; i < 15; i++) { tr_x[i] = env.game.x; tr_y[i] = env.game.y; }
}

int main() {
    tcgetattr(0, &orig); struct termios r = orig; r.c_lflag &= ~(ECHO | ICANON);
    r.c_cc[VMIN] = 0; r.c_cc[VTIME] = 0; tcsetattr(0, 0, &r); printf("\033[?25l\033[2J"); 
    struct sigaction sa; memset(&sa, 0, sizeof(sa)); sa.sa_handler = panic;
    sigaction(SIGINT, &sa, 0); sigaction(SIGSEGV, &sa, 0); sigaction(SIGALRM, &sa, 0);
    
    screen = calloc(W * H, sizeof(Pixel));
    char *soul = getenv("SYC_SOUL");
    if (soul) env.bits = strtoull(soul, NULL, 10);
    else { env.game.nan = 0xF; env.game.dmg = 12; env.game.spd = 3; env.game.gen = 1; env.game.med=0; env.game.inv=0; }
    
    reset(1); env.game.state = 0; char *taunts[] = {"NULL_VOID", "BY_DESIGN", "STACK_OFLW", "SIG_ABORT"};

    sigsetjmp(game_loop, 1);
    tick++;
    char k = 0; uint8_t act = 0;
    while (read(0, &k, 1) > 0) {
        if (k == 'w') { vy = -1; vx = 0; p_dy = -1; p_dx = 0; }
        if (k == 's') { vy = 1; vx = 0; p_dy = 1; p_dx = 0; }
        if (k == 'a') { vx = -1; vy = 0; p_dx = -1; p_dy = 0; }
        if (k == 'd') { vx = 1; vy = 0; p_dx = 1; p_dy = 0; }
        if (k == ' ') act |= 1; if (k == 'f') act |= 2; if (k == 'i') inv_open = !inv_open;
        if (k == '1' && env.game.med > 0 && env.game.state == 1) { env.game.hp = 100; env.game.med--; inv_open = 0; }
        if (k == '2' && env.game.inv > 0 && env.game.state == 1) { inv_end = tick + 500; env.game.inv = 0; inv_open = 0; }
        if (k == 'q') panic(SIGINT);

        if (env.game.state == 3) {
            if (k == '1') { env.game.dmg += 4; env.game.gen++; persist(); reset(1); env.game.state = 1; }
            if (k == '2') { if (env.game.spd > 1) env.game.spd--; env.game.gen++; persist(); reset(1); env.game.state = 1; }
        }
        if (k == 'r' && env.game.state == 2) { reset(1); env.game.state = 1; }
        if (k == ' ' && env.game.state == 0) { env.game.state = 1; printf("\033[2J"); fflush(stdout); }
    }

    int ph = (ai_hp <= 0) ? 0 : (ai_hp / 100); if (ph > 4) ph = 4;
    if (env.game.state == 1) {
        if (env.game.hp <= 0) env.game.state = 2; if (ai_hp <= 0) env.game.state = 3;
        if (ph == 2 && !fk) { fk = 1; printf("\033[0m\033[2J\033[H\n\t--- SEG FAULT ---\n"); fflush(stdout); sleep(1); siglongjmp(game_loop, 1); }
        
        env.game.px = env.game.x; env.game.py = env.game.y;
        if (tick % env.game.spd == 0) {
            env.game.x += vx; env.game.y += vy;
            for (int i = 14; i > 0; i--) { tr_x[i] = tr_x[i - 1]; tr_y[i] = tr_y[i - 1]; }
            tr_x[0] = env.game.x; tr_y[0] = env.game.y;
        }

        if (ph >= 1 && tick % 50 == 0) {
            int cx = 1 + rand() % (W - 2), cy = 1 + rand() % (H - 2);
            if (abs(cx - (int)env.game.x) > 6 && abs(cy - (int)env.game.y) > 4) arena[cx][cy] = 0;
        }

        if (act & 1) spawn(0, 64, env.game.x + p_dx, env.game.y + p_dy, p_dx, p_dy, 25, 1, 0);
        if (act & 2 && env.game.hp > 5) {
            env.game.hp -= 3;
            for (int i = -1; i <= 1; i++) for (int j = -1; j <= 1; j++) if (i || j) spawn(0, 64, env.game.x+i, env.game.y+j, i, j, 8, 2, 0);
        }

        int as = 12 - env.game.gen; if (as < 2) as = 2;
        if (tick % as == 0) { if (ai_x < tr_x[14]) ai_x++; else ai_x--; if (ai_y < tr_y[14]) ai_y++; else ai_y--; }
        
        if (tick % 65 == 0) spawn(64, 128, ai_x, ai_y, (env.game.x > ai_x) ? 1 : -1, (env.game.y > ai_y) ? 1 : -1, 20, 2, 0);
        if (ph >= 1 && tick % 150 == 0) spawn(64, 128, ai_x, ai_y, (env.game.x > ai_x) ? 1 : -1, (env.game.y > ai_y) ? 1 : -1, 30, 1, 1);

        if (env.game.gen > 1 && tick % 200 == 0) {
            for (int i = 0; i < 8; i++) if (!(grems[i] & 0x8000)) { grems[i] = (ai_y << 8) | ai_x | 0x8000; break; }
        }
        for (int i = 0; i < 8; i++) if (grems[i] & 0x8000) {
            int gx = grems[i] & 0xFF, gy = (grems[i] >> 8) & 0x7F;
            if (tick % 7 == 0) { if (gx < tr_x[14]) gx++; else gx--; if (gy < tr_y[14]) gy++; else gy--; }
            if (gx == env.game.x && gy == env.game.y) { if(tick>inv_end) panic(SIGALRM); }
            grems[i] = 0x8000 | (gy << 8) | gx; draw(gx, gy, 200, 0, 200, '@', ' ', ph);
        }

        char *b = (char *)&main;
        for (int x = 0; x < W; x++) for (int y = 0; y < H; y++) {
            if (!arena[x][y]) draw(x, y, 20, 45, 20, b[(x + y + tick / 10) % 200], ' ', ph);
            else draw(x, y, 10, 10, 10, '.', ' ', ph);
        }

        if (tick % 400 == 0) { hx = 1 + rand() % (W - 2); hy = 1 + rand() % (H - 2); }
        if (tick % 1200 == 0) { ix = 1 + rand() % (W - 2); iy = 1 + rand() % (H - 2); }
        if (hx > 0) { 
            draw(hx, hy, 0, 100, 255, '&', ' ', ph); 
            if (env.game.x == hx && env.game.y == hy) { 
                if (env.game.hp >= 100 && env.game.med < 3) env.game.med++; else env.game.hp = 100; 
                hx = -1; printf("\033]0;ABSORBED_MEMORY\007"); 
            }
        }
        if (ix > 0) { draw(ix, iy, 255, 100, 255, '$', ' ', ph); if (env.game.x == ix && env.game.y == iy) { env.game.inv = 1; ix = -1; } }

        for (int i = 0; i < 128; i++) {
            int l = (bullets[i] >> 24) & 0xFF; 
            if (l > 0) {
                int bx = bullets[i] & 0x3F, by = (bullets[i] >> 6) & 0x3F;
                int dx = ((bullets[i] >> 12) & 0xF) - 2, dy = ((bullets[i] >> 16) & 0xF) - 2;
                int s = (bullets[i] >> 20) & 0xF, t = (bullets[i] >> 32) & 0xFF;
                
                for (int st = 0; st < s; st++) {
                    bx += dx; by += dy; l--; 
                    if (bx <= 0 || bx >= W - 1 || by <= 0 || by >= H - 1) { 
                        if (t == 1) { 
                            if(bx > 0 && bx < W-1 && by > 0 && by < H-1) arena[bx][by] = 0; 
                            if(bx+1 < W-1) arena[bx+1][by] = 0; if(by+1 < H-1) arena[bx][by+1] = 0; 
                        }
                        l = 0; break; 
                    }
                    if (i < 64 && bx == ai_x && by == ai_y) { ai_hp -= env.game.dmg; l = 0; break; }
                    if (i >= 64 && bx == env.game.x && by == env.game.y) { l = 0; if(tick > inv_end) panic(SIGALRM); break; }
                }
                bullets[i] = (bx & 0x3F) | ((by & 0x3F) << 6) | (((dx + 2) & 0xF) << 12) | (((dy + 2) & 0xF) << 16) | ((s & 0xF) << 20) | ((uint64_t)l << 24) | ((uint64_t)t << 32);
                if (l > 0) draw(bx, by, (i < 64) ? 0 : 255, (i < 64) ? 255 : 0, (t==1)?255:0, (t==1)?'#' : (i < 64 ? '!' : '*'), ' ', ph);
            }
        }
        
        *arena[env.game.x][env.game.y] ^= 1; char pc = p_dx == 1 ? '>' : p_dx == -1 ? '<' : p_dy == 1 ? 'v' : '^';
        if (ph >= 2 && tick % 2) draw(tr_x[14], tr_y[14], 80, 80, 80, pc, ' ', ph);
        draw(env.game.x, env.game.y, (tick < inv_end) ? 255 : 0, 255, (tick < inv_end) ? 255 : 0, pc, ' ', ph);
        draw(ai_x, ai_y, 255, 0, 0, (ph >= 1) ? '@' : 'X', ' ', ph);
        
        if (inv_open) {
            txt(W/2-10, H/2-2, "-- INVENTORY --", 100, 100, 255, ph);
            char m[24], v[24]; sprintf(m, "[1] Medkit: %d", env.game.med); txt(W/2-10, H/2, m, 0, 255, 0, ph);
            sprintf(v, "[2] Shield: %d", env.game.inv); txt(W/2-10, H/2 + 1, v, 255, 100, 255, ph);
        }
        printf("\033[H\033[%dB\033[0mHP:%03d|ARM:[", H + 1, env.game.hp);
        for(int i=0; i<15; i++) printf("%c", i < env.game.arm ? '#' : ' ');
        printf("] AI:%03d FRM:%d [%s]\033[K", ai_hp, env.game.gen, (ph == 0) ? "STABLE" : taunts[ph-1]);
    } else if (env.game.state == 0) { txt(W / 2 - 6, H / 2, "SYCWARS", 255, 255, 255, 0);
    } else if (env.game.state == 3) { 
        printf("\033[0m\033[2J\033[H\n\n\n\n\tGOOD - RECALIBRATE?\n\n\t[1] DMG_ENTROPY (%d)\n\t[2] SPD_CLOCK (%d)", env.game.dmg, env.game.spd);
    } else { printf("\033[0m\033[2J\033[H\n\n\n\n\tYOU SUCK - TERMINATE?\n\n\t[R] REBOOT"); }

    char out[130000]; int p = sprintf(out, "\033[H");
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            Pixel px = screen[y * W + x]; if (env.game.state == 3 && rand() % 100 < 10) px.r = 255;
            if (!px.r && !px.g && !px.b && !px.c1) p += sprintf(out + p, "\033[0m  ");
            else p += sprintf(out + p, "\033[38;2;255;255;255;48;2;%d;%d;%dm%c%c", px.r, px.g, px.b, px.c1 ? px.c1 : ' ', px.c2 ? px.c2 : ' ');
        } p += sprintf(out + p, "\033[0m\n");
    }
    if (env.game.state < 2) write(1, out, p);
    memset(screen, 0, W * H * sizeof(Pixel)); usleep(20000); siglongjmp(game_loop, 1);
    return 0;
}
