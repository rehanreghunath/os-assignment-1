#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <logfile> <column_number>\n", argv[0]);
        exit(1);
    }

    char *file = argv[1];
    char *col = argv[2];

    int p1[2], p2[2], p3[2], p4[2];
    pipe(p1); pipe(p2); pipe(p3); pipe(p4);

    char cutf[16];
    snprintf(cutf, sizeof(cutf), "-f%s", col);

    pid_t c1 = fork();
    if (c1 == 0) {
        dup2(p1[1], STDOUT_FILENO);
        close(p1[0]); close(p1[1]);
        close(p2[0]); close(p2[1]); close(p3[0]); close(p3[1]); close(p4[0]); close(p4[1]);
        execlp("cut", "cut", "-d", " ", cutf, file, NULL);
        perror("exec cut"); _exit(1);
    }

    pid_t c2 = fork();
    if (c2 == 0) {
        dup2(p1[0], STDIN_FILENO);
        dup2(p2[1], STDOUT_FILENO);
        close(p1[0]); close(p1[1]);
        close(p2[0]); close(p2[1]); close(p3[0]); close(p3[1]); close(p4[0]); close(p4[1]);
        execlp("sort", "sort", NULL);
        perror("exec sort"); _exit(1);
    }

    pid_t c3 = fork();
    if (c3 == 0) {
        dup2(p2[0], STDIN_FILENO);
        dup2(p3[1], STDOUT_FILENO);
        close(p1[0]); close(p1[1]);
        close(p2[0]); close(p2[1]); close(p3[0]); close(p3[1]); close(p4[0]); close(p4[1]);
        execlp("uniq", "uniq", "-c", NULL);
        perror("exec uniq"); _exit(1);
    }

    pid_t c4 = fork();
    if (c4 == 0) {
        dup2(p3[0], STDIN_FILENO);
        dup2(p4[1], STDOUT_FILENO);
        close(p1[0]); close(p1[1]);
        close(p2[0]); close(p2[1]); close(p3[0]); close(p3[1]); close(p4[0]); close(p4[1]);
        execlp("sort", "sort", "-rn", NULL);
        perror("exec sort -rn"); _exit(1);
    }

    pid_t c5 = fork();
    if (c5 == 0) {
        dup2(p4[0], STDIN_FILENO);
        close(p1[0]); close(p1[1]);
        close(p2[0]); close(p2[1]); close(p3[0]); close(p3[1]); close(p4[0]); close(p4[1]);
        execlp("head", "head", "-5", NULL);
        perror("exec head"); _exit(1);
    }

    close(p1[0]); close(p1[1]);
    close(p2[0]); close(p2[1]);
    close(p3[0]); close(p3[1]);
    close(p4[0]); close(p4[1]);

    for (int i = 0; i < 5; i++) wait(NULL);

    return 0;
}
