#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define MAX_ITEMS 10
#define MAX_LINE 1024
#define MAX_ARGS 64

volatile sig_atomic_t sigint_flag = 0;

void sigint_handler(int sig) {
    (void)sig;
    sigint_flag = 1;
}

void add_item(char queue[][256], int *queue_count, char *name) {
    if (name == NULL) {
        printf("Error: item name missing\n");
        return;
    }
    if (*queue_count >= MAX_ITEMS) {
        printf("Error: queue is full\n");
        return;
    }
    strncpy(queue[*queue_count], name, 255);
    queue[*queue_count][255] = '\0';
    (*queue_count)++;
}

void list_items(char queue[][256], int queue_count) {
    if (queue_count == 0) {
        printf("Queue is empty\n");
        return;
    }
    for (int i = 0; i < queue_count; i++) {
        printf("%s\n", queue[i]);
    }
}

void run_external(char *args[]) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        execvp(args[0], args);
        perror("exec failed");
        _exit(1);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    char queue[MAX_ITEMS][256] = {0};
    int queue_count = 0;
    char line[MAX_LINE];

    while (1) {
        if (sigint_flag) {
            printf("\n[ALERT] Emergency stop triggered, item queue cleared\n");
            queue_count = 0;
            sigint_flag = 0;
        }

        printf("belt-control$ ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            if (sigint_flag) {
                clearerr(stdin);
                continue;
            }
            break;
        }

        line[strcspn(line, "\n")] = '\0';
        if (strlen(line) == 0) continue;

        char *args[MAX_ARGS];
        int argc = 0;
        char *tok = strtok(line, " ");
        while (tok != NULL && argc < MAX_ARGS - 1) {
            args[argc++] = tok;
            tok = strtok(NULL, " ");
        }
        args[argc] = NULL;

        if (argc == 0) continue;

        if (strcmp(args[0], "add_item") == 0) {
            add_item(queue, &queue_count, argc > 1 ? args[1] : NULL);
        } else if (strcmp(args[0], "list_items") == 0) {
            list_items(queue, queue_count);
        } else if (strcmp(args[0], "quit") == 0) {
            break;
        } else if (strcmp(args[0], "date") == 0) {
            run_external(args);
        } else if (strcmp(args[0], "ping") == 0) {
            if (argc < 2) {
                printf("Error: ping requires an address\n");
            } else {
                char *pargs[] = {"ping", "-c", "4", args[1], NULL};
                run_external(pargs);
            }
        } else {
            printf("Error: unknown command '%s'\n", args[0]);
        }
    }

    return 0;
}
