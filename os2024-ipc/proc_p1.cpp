#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

FILE *input_file;
int output_fd;

void signal_handler(int signal_number) {
    char *line_buffer = (char *)calloc(400, sizeof(char));
    if (line_buffer == NULL) {
        perror("Chyba: Nepodarilo sa alokovať pamäť");
        exit(EXIT_FAILURE);
    }

    size_t buffer_size = 400;

    if (getline(&line_buffer, &buffer_size, input_file) == -1 ||
        write(output_fd, line_buffer, strlen(line_buffer)) == -1) {
        perror("Chyba pri čítaní zo súboru alebo zápise do deskriptora");
        free(line_buffer);
        exit(EXIT_FAILURE);
    }

    printf("PROC_P1: Prijaté dáta: %s", line_buffer);
    free(line_buffer);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Použitie: %s <deskriptor súboru>\n", argv[0]);
        return EXIT_FAILURE;
    }

    output_fd = atoi(argv[1]);

    input_file = fopen("p1.txt", "r");
    if (input_file == NULL) {
        perror("Chyba: Nepodarilo sa otvoriť súbor 'p1.txt'");
        return EXIT_FAILURE;
    }

    if (kill(getppid(), SIGUSR1) == -1) {
        perror("Chyba: Nepodarilo sa poslať signál rodičovskému procesu");
        fclose(input_file);
        return EXIT_FAILURE;
    }

    if (signal(SIGUSR1, signal_handler) == SIG_ERR) {
        perror("Chyba: Nepodarilo sa nastaviť obsluhu signálu");
        fclose(input_file);
        return EXIT_FAILURE;
    }

    printf("PROC_P1: Čakám na signál...\n");
    while (1) {
        sleep(10);
    }
}
