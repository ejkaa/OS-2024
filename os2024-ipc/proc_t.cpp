#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

void terminate_with_error(const char* message) {
    perror(message);
    exit(EXIT_FAILURE);
}

static int perform_semaphore_wait(int semaphore_id) {
    struct sembuf semaphore_op = {0, -1, SEM_UNDO};
    if (semop(semaphore_id, &semaphore_op, 1) == -1) {
        fprintf(stderr, "Semaphore wait operation failed\n");
        return 0;
    }
    return 1;
}

static int perform_semaphore_signal(int semaphore_id) {
    struct sembuf semaphore_op = {1, 1, SEM_UNDO};
    if (semop(semaphore_id, &semaphore_op, 1) == -1) {
        fprintf(stderr, "Semaphore signal operation failed\n");
        return 0;
    }
    return 1;
}

int main(int argc, char** argv) {
    if (argc != 4) {
        terminate_with_error("Invalid arguments - expected 3 parameters: pipe, shared memory, semaphore");
    }

    int pipe_fd = atoi(argv[1]);
    int shared_mem_id = atoi(argv[2]);
    int semaphore_id = atoi(argv[3]);

    char* shared_mem_ptr = (char*)shmat(shared_mem_id, NULL, 0);

    char read_char;
    char input_line[256];

    kill(getppid(), SIGUSR1);

    while (1) {
        int index = 0;
        memset(input_line, 0, sizeof(input_line));
        while (read(pipe_fd, &read_char, 1) > 0 && read_char != '\n') {
            input_line[index++] = read_char;
        }
        input_line[index] = '\0';

        if (!perform_semaphore_signal(semaphore_id)) {
            exit(EXIT_FAILURE);
        }
        strcpy(shared_mem_ptr, input_line);
        if (!perform_semaphore_wait(semaphore_id)) {
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
