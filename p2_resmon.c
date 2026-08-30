#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MAX_LINE 512

struct msgbuf {
    long mtype;
    int val;
};

void run_pipeline_topk(int k) {
    int fd[2];
    pipe(fd);
    pid_t pid = fork();
    if (pid == 0) {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        execlp("ps", "ps", "-eo", "pid,comm,%cpu,%mem", "--no-headers", NULL);
        _exit(1);
    }
    close(fd[1]);
    FILE *fp = fdopen(fd[0], "r");
    char line[MAX_LINE];

    typedef struct { int pid; char comm[64]; double cpu; double mem; double score; } proc_t;
    proc_t *procs = malloc(sizeof(proc_t) * 4096);
    int nproc = 0;

    while (fgets(line, sizeof(line), fp) && nproc < 4096) {
        int pid_v;
        char comm[64];
        double cpu, mem;
        if (sscanf(line, "%d %63s %lf %lf", &pid_v, comm, &cpu, &mem) == 4) {
            procs[nproc].pid = pid_v;
            strncpy(procs[nproc].comm, comm, 63);
            procs[nproc].cpu = cpu;
            procs[nproc].mem = mem;
            procs[nproc].score = 3 * cpu + 2 * mem;
            nproc++;
        }
    }
    fclose(fp);
    waitpid(pid, NULL, 0);

    for (int i = 0; i < nproc - 1; i++) {
        for (int j = 0; j < nproc - i - 1; j++) {
            if (procs[j].score < procs[j + 1].score) {
                proc_t tmp = procs[j];
                procs[j] = procs[j + 1];
                procs[j + 1] = tmp;
            }
        }
    }

    int limit = k < nproc ? k : nproc;
    for (int i = 0; i < limit; i++) {
        printf("PID=%d CMD=%s CPU%%=%.1f MEM%%=%.1f SCORE=%.2f\n",
               procs[i].pid, procs[i].comm, procs[i].cpu, procs[i].mem, procs[i].score);
    }
    free(procs);
}

void print_process_info(int pid) {
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "ps -o pid,comm,user,%%cpu,%%mem -p %d --no-headers", pid);
    FILE *fp = popen(cmd, "r");
    if (!fp) return;
    char line[MAX_LINE];
    if (fgets(line, sizeof(line), fp)) {
        int pid_v;
        char comm[64], user[64];
        double cpu, mem;
        if (sscanf(line, "%d %63s %63s %lf %lf", &pid_v, comm, user, &cpu, &mem) == 5) {
            double score = 3 * cpu + 2 * mem;
            printf("Process info: PID=%d CMD=%s OWNER=%s CPU%%=%.1f MEM%%=%.1f SCORE=%.2f\n",
                   pid_v, comm, user, cpu, mem, score);
        }
    } else {
        printf("Process %d not found\n", pid);
    }
    pclose(fp);
}

void child_process(int n, int k, int r, key_t key) {
    signal(SIGINT, SIG_IGN);
    int msgid = msgget(key, 0666);
    int iter = 0;

    while (1) {
        run_pipeline_topk(k);
        iter++;
        if (iter >= r) {
            struct msgbuf msg;
            msg.mtype = 1;
            if (msgrcv(msgid, &msg, sizeof(int), 1, 0) == -1) {
                break;
            }
            int val = msg.val;
            if (val == -2) {
                msgctl(msgid, IPC_RMID, NULL);
                exit(0);
            } else if (val == -1) {
                iter = 0;
                continue;
            } else {
                print_process_info(val);
                if (kill(val, SIGKILL) == 0) {
                    printf("Process %d killed.\n", val);
                } else {
                    perror("kill failed");
                }
            }
            iter = 0;
        }
        sleep(n);
    }
}

void parent_process(int msgid, pid_t child_pid) {
    while (1) {
        int status;
        pid_t w = waitpid(child_pid, &status, WNOHANG);
        if (w == child_pid) break;

        printf("Enter PID to act on (-1 skip, -2 quit): ");
        fflush(stdout);
        int val;
        if (scanf("%d", &val) != 1) {
            while (getchar() != '\n');
            continue;
        }

        struct msgbuf msg;
        msg.mtype = 1;
        msg.val = val;
        msgsnd(msgid, &msg, sizeof(int), 0);

        if (val == -2) {
            waitpid(child_pid, NULL, 0);
            break;
        }
    }
}

int main() {
    int n, k, r;
    printf("Enter n (print interval sec), k (top-k), r (iterations before kill prompt): ");
    if (scanf("%d %d %d", &n, &k, &r) != 3) {
        fprintf(stderr, "Invalid input\n");
        exit(1);
    }

    key_t key = ftok(".", 65);
    int msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget");
        exit(1);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        child_process(n, k, r, key);
        exit(0);
    } else {
        parent_process(msgid, pid);
    }

    return 0;
}
