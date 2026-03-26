#include "../include/sycwars.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define W 60
#define H 24

Pixel screen[W * H];

void draw_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, char c1, char c2) {
    if(x < 0 || x >= W || y < 0 || y >= H) return;
    screen[y * W + x] = (Pixel){r, g, b, c1, c2};
}

void flush_screen() {
    char out[90000]; 
    int ptr = sprintf(out, "\033[H");
    for(int y=0; y<H; y++) {
        for(int x=0; x<W; x++) {
            Pixel p = screen[y * W + x];
            char c1 = p.c1 ? p.c1 : ' ';
            char c2 = p.c2 ? p.c2 : ' ';
            if (p.r == 0 && p.g == 0 && p.b == 0 && c1 == ' ' && c2 == ' ') {
                ptr += sprintf(out + ptr, "\033[0m  ");
            } else {
                ptr += sprintf(out + ptr, "\033[38;2;255;255;255;48;2;%d;%d;%dm%c%c", p.r, p.g, p.b, c1, c2);
            }
        }
        ptr += sprintf(out + ptr, "\033[0m\n");
    }
    write(STDOUT_FILENO, out, ptr);
    memset(screen, 0, sizeof(screen));
}
