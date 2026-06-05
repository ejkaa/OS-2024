#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Notify parent process
    kill(getppid(), SIGUSR1);

    // Initialize variables
    char buffer[256] = {0};
    int port = std::atoi(argv[1]);

    struct sockaddr_in server_address = {};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }

    int reuse_option = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_option, sizeof(int)) < 0) {
        perror("Error setting socket options");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    if (bind(socket_fd, reinterpret_cast<struct sockaddr*>(&server_address), sizeof(server_address)) < 0) {
        perror("Error binding socket");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    int output_file = open("serv2.txt", O_CREAT | O_WRONLY, 0666);
    if (output_file < 0) {
        perror("Error opening output file");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    struct sockaddr_in client_address = {};
    socklen_t client_address_length = sizeof(client_address);

    for (int remaining_attempts = 10; remaining_attempts > 0; --remaining_attempts) {
        ssize_t bytes_received = recvfrom(socket_fd, buffer, sizeof(buffer) - 1, 0,
                                          reinterpret_cast<struct sockaddr*>(&client_address),
                                          &client_address_length);
        if (bytes_received < 0) {
            perror("Error receiving data");
            break;
        }

        buffer[bytes_received] = '\0';

        if (write(output_file, buffer, bytes_received) == -1 ||
            (remaining_attempts > 1 && write(output_file, "\n", 1) == -1)) {
            perror("Error writing to file");
            break;
        }

        printf("Writing word: %s\n", buffer);
    }

    kill(getppid(), SIGUSR2);
    close(output_file);
    close(socket_fd);

    return EXIT_SUCCESS;
}
