#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/shm.h>
#include <sys/sem.h>

int semaphore_id;

static int semaphore_increment() {
    struct sembuf sem_operation = {0, 1, SEM_UNDO};
    return semop(semaphore_id, &sem_operation, 1) != -1;
}

static int semaphore_decrement() {
    struct sembuf sem_operation = {1, -1, SEM_UNDO};
    return semop(semaphore_id, &sem_operation, 1) != -1;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <shared_memory_id> <semaphore_id> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    kill(getppid(), SIGUSR1);

    semaphore_id = std::atoi(argv[2]);
    char* shared_memory = static_cast<char*>(shmat(std::atoi(argv[1]), nullptr, 0));

    struct sockaddr_in server_address = {};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(std::atoi(argv[3]));
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0 || connect(socket_fd, reinterpret_cast<struct sockaddr*>(&server_address), sizeof(server_address)) < 0) {
        perror("Connection setup failed");
        return EXIT_FAILURE;
    }

    while (true) {
        if (!semaphore_increment()) {
            perror("Semaphore increment failed");
            break;
        }

        if (write(socket_fd, shared_memory, std::strlen(shared_memory) + 1) == -1) {
            perror("Write to socket failed");
            break;
        }

        if (!semaphore_decrement()) {
            perror("Semaphore decrement failed");
            break;
        }

        sleep(1);
    }

    return EXIT_SUCCESS;
}