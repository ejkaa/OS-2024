#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

int server1, server2, process_r, process_s, process_t, process_d, child1, child2;

void terminate_all(int signal);
void process_completed(int signal);

int main(int argc, char* argv[]) {
    if (argc != 3) {
        perror("Usage: <port1> <port2>");
        return EXIT_FAILURE;
    }

    printf("Task started.\n");

    signal(SIGUSR2, terminate_all);
    signal(SIGUSR1, process_completed);

    int port_a = std::atoi(argv[1]);
    int port_b = std::atoi(argv[2]);

    int pipe_a[2], pipe_b[2];
    pipe(pipe_a);
    pipe(pipe_b);

    int pipe_a_read = dup(pipe_a[0]);
    int pipe_b_read = dup(pipe_b[0]);
    int pipe_a_write = dup(pipe_a[1]);
    int pipe_b_write = dup(pipe_b[1]);

    int semaphore1 = semget(IPC_PRIVATE, 2, 0666 | IPC_CREAT);
    int semaphore2 = semget(IPC_PRIVATE, 2, 0666 | IPC_CREAT);
    if (semaphore1 == -1 || semaphore2 == -1) {
        perror("Semaphore creation failed");
        return EXIT_FAILURE;
    }

    semctl(semaphore1, 0, SETVAL, 0);
    semctl(semaphore2, 0, SETVAL, 0);
    semctl(semaphore1, 1, SETVAL, 0);
    semctl(semaphore2, 1, SETVAL, 0);

    int memory1 = shmget(IPC_PRIVATE, 200, 0666 | IPC_CREAT);
    int memory2 = shmget(IPC_PRIVATE, 200, 0666 | IPC_CREAT);

    char port_a_str[10], port_b_str[10];
    char memory1_str[10], memory2_str[10];
    char semaphore1_str[10], semaphore2_str[10];
    char pipe_a_read_str[10], pipe_b_read_str[10];
    char pipe_a_write_str[10], pipe_b_write_str[10];

    snprintf(port_a_str, sizeof(port_a_str), "%d", port_a);
    snprintf(port_b_str, sizeof(port_b_str), "%d", port_b);
    snprintf(memory1_str, sizeof(memory1_str), "%d", memory1);
    snprintf(memory2_str, sizeof(memory2_str), "%d", memory2);
    snprintf(semaphore1_str, sizeof(semaphore1_str), "%d", semaphore1);
    snprintf(semaphore2_str, sizeof(semaphore2_str), "%d", semaphore2);
    snprintf(pipe_a_read_str, sizeof(pipe_a_read_str), "%d", pipe_a_read);
    snprintf(pipe_b_read_str, sizeof(pipe_b_read_str), "%d", pipe_b_read);
    snprintf(pipe_a_write_str, sizeof(pipe_a_write_str), "%d", pipe_a_write);
    snprintf(pipe_b_write_str, sizeof(pipe_b_write_str), "%d", pipe_b_write);

    printf("Starting SERVER 1...\n");
    if ((server1 = fork()) == 0) {
        execl("proc_serv1", "proc_serv1", port_a_str, port_b_str, nullptr);
        perror("Failed to start SERVER 1");
        exit(EXIT_FAILURE);
    }
    pause();

    printf("Starting SERVER 2...\n");
    if ((server2 = fork()) == 0) {
        execl("proc_serv2", "proc_serv2", port_b_str, nullptr);
        perror("Failed to start SERVER 2");
        exit(EXIT_FAILURE);
    }
    pause();

    printf("Starting PROCESS D...\n");
    if ((process_d = fork()) == 0) {
        execl("proc_d", "proc_d", memory2_str, semaphore2_str, port_a_str, nullptr);
        perror("Failed to start PROCESS D");
        exit(EXIT_FAILURE);
    }
    pause();

    printf("Starting PROCESS P1...\n");
    if ((child1 = fork()) == 0) {
        execl("proc_p1", "proc_p1", pipe_a_write_str, nullptr);
        perror("Failed to start PROCESS P1");
        exit(EXIT_FAILURE);
    }
    pause();

    printf("Starting PROCESS P2...\n");
    if ((child2 = fork()) == 0) {
        execl("proc_p2", "proc_p2", pipe_a_write_str, nullptr);
        perror("Failed to start PROCESS P2");
        exit(EXIT_FAILURE);
    }
    pause();

    printf("Starting PROCESS PR...\n");
    if ((process_r = fork()) == 0) {
        char child1_str[10], child2_str[10];
        snprintf(child1_str, sizeof(child1_str), "%d", child1);
        snprintf(child2_str, sizeof(child2_str), "%d", child2);

        execl("proc_pr", "proc_pr", child1_str, child2_str, pipe_a_read_str, pipe_b_write_str, nullptr);
        perror("Failed to start PROCESS PR");
        exit(EXIT_FAILURE);
    }
    pause();

    printf("Starting PROCESS T...\n");
    if ((process_t = fork()) == 0) {
        execl("proc_t", "proc_t", pipe_b_read_str, memory1_str, semaphore1_str, nullptr);
        perror("Failed to start PROCESS T");
        exit(EXIT_FAILURE);
    }
    pause();

    printf("Starting PROCESS S...\n");
    if ((process_s = fork()) == 0) {
        execl("proc_s", "proc_s", memory1_str, semaphore1_str, memory2_str, semaphore2_str, nullptr);
        perror("Failed to start PROCESS S");
        exit(EXIT_FAILURE);
    }

    pause();
    pause();

    return EXIT_SUCCESS;
}

void terminate_all(int signal) {
    sleep(5);

    kill(child1, SIGKILL);
    kill(child2, SIGKILL);
    kill(process_r, SIGKILL);
    kill(process_t, SIGKILL);
    kill(process_s, SIGKILL);
    kill(server1, SIGKILL);
    kill(server2, SIGKILL);
}

void process_completed(int signal) {
    printf("Process completed\n");
}
