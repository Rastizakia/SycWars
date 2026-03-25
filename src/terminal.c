#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

struct termios orig_termios;

void cleanup() {
    printf("\033[0m\033[2J\033[H\033[?25h");
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

void handle_sig(int sig) {
    cleanup();
    exit(0);
}

void init_terminal() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(cleanup);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    printf("\033[?25l\033[2J");
    signal(SIGINT, handle_sig);
}
