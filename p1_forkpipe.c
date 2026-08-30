#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>

volatile sig_atomic_t stop_flag = 0;

void sigint_handler(int sig) {
    (void)sig;
    stop_flag = 1;
}

int gcd(int a, int b) {
    while (b != 0) {
        int t = b;
        b = a % b;
        a = t;
    }
    return a;
}

int main() {
    signal(SIGINT, sigint_handler);

    int arr[] = {18, 24, 35, 49, 10, 63, 27, 40, 14, 21};
    int n = sizeof(arr) / sizeof(arr[0]);
    int count = n;

    int p1[2];
    int p2[2];
    if (pipe(p1) == -1 || pipe(p2) == -1) {
        perror("pipe");
        exit(1);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        close(p1[1]);
        close(p2[0]);
        while (!stop_flag) {
            int buf[2];
            ssize_t r = read(p1[0], buf, sizeof(buf));
            if (r <= 0) break;
            int x = buf[0], y = buf[1];
            int g = gcd(x, y);
            printf("[Child] x=%d y=%d gcd=%d\n", x, y, g);
            fflush(stdout);
            struct timespec ts;
            long ms = (long)(time(NULL) % g);
            ts.tv_sec = ms / 1000;
            ts.tv_nsec = (ms % 1000) * 1000000L;
            nanosleep(&ts, NULL);
            write(p2[1], &g, sizeof(g));
        }
        close(p1[0]);
        close(p2[1]);
        exit(0);
    } else {
        close(p1[0]);
        close(p2[1]);
        srand(time(NULL) ^ getpid());

        while (!stop_flag && count > 1) {
            int i = rand() % count;
            int x = arr[i];
            arr[i] = arr[count - 1];
            count--;

            int j = rand() % count;
            int y = arr[j];
            arr[j] = arr[count - 1];
            count--;

            printf("[Parent] picked x=%d y=%d\n", x, y);
            fflush(stdout);

            int buf[2] = {x, y};
            if (write(p1[1], buf, sizeof(buf)) < 0) break;

            int g;
            ssize_t r = read(p2[0], &g, sizeof(g));
            if (r <= 0) break;

            printf("[Parent] received gcd=%d, sleeping\n", g);
            fflush(stdout);

            struct timespec ts;
            ts.tv_sec = g / 1000;
            ts.tv_nsec = (g % 1000) * 1000000L;
            nanosleep(&ts, NULL);
        }

        close(p1[1]);
        close(p2[0]);
        kill(pid, SIGTERM);
        wait(NULL);
        printf("[Parent] done, array exhausted or interrupted\n");
    }

    return 0;
}
