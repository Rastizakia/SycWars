#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "../include/sycwars.h"

struct termios orig_termios;

void cleanup() {
    printf("\033[0m\033[2J\033[H\033[?25h");
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

void panic(int sig) {
    if (sig == SIGINT) { cleanup(); exit(0); }
    if (sig == SIGSEGV) {
        env.game.x = env.game.px; env.game.y = env.game.py;
        env.game.hp -= 5;
        printf("\033[48;2;255;0;0m\033[2J"); fflush(stdout);
        usleep(15000); siglongjmp(game_loop, 1);
    }
}

void init_terminal() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(cleanup);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0; raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    printf("\033[?25l\033[2J");
    struct sigaction sa; memset(&sa, 0, sizeof(sa));
    sa.sa_handler = panic; sigemptyset(&sa.sa_mask); sa.sa_flags = SA_NODEFER;
    sigaction(SIGINT, &sa, NULL); sigaction(SIGSEGV, &sa, NULL);
}
