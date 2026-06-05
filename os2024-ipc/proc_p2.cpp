#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

FILE* input_file = nullptr;
int output_descriptor;

void handle_signal(int signal_number) {
    size_t buffer_size = 400; // Change from constexpr to a regular variable
    char* line_buffer = static_cast<char*>(calloc(buffer_size, sizeof(char)));

    if (!line_buffer) {
        perror("Error: Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    if (getline(&line_buffer, &buffer_size, input_file) == -1) { // No error now
        perror("Error reading from file");
        free(line_buffer);
        exit(EXIT_FAILURE);
    }

    if (write(output_descriptor, line_buffer, strlen(line_buffer)) == -1) {
        perror("Error writing to output descriptor");
        free(line_buffer);
        exit(EXIT_FAILURE);
    }

    printf("PROC_P2: Received data: %s", line_buffer);
    free(line_buffer);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <output_descriptor>\n", argv[0]);
        return EXIT_FAILURE;
    }

    output_descriptor = std::atoi(argv[1]);

    input_file = fopen("p2.txt", "r");
    if (!input_file) {
        perror("Error: Could not open file 'p2.txt'");
        return EXIT_FAILURE;
    }

    if (kill(getppid(), SIGUSR1) == -1) {
        perror("Error: Failed to send signal to parent process");
        fclose(input_file);
        return EXIT_FAILURE;
    }

    if (signal(SIGUSR1, handle_signal) == SIG_ERR) {
        perror("Error: Failed to set up signal handler");
        fclose(input_file);
        return EXIT_FAILURE;
    }

    printf("PROC_P2: Waiting for signal...\n");
    while (true) {
        sleep(10);
    }

    return 0;
}
