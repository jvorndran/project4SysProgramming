#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <object_file> <relocation_address> <SIC|SICXE>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return 1;
    }
    if (strcmp(filename + strlen(filename) - 2, ".o") != 0) {
        fprintf(stderr, "Error: File must have .o extension.\n");
        return 1;
    }

    // Check if the relocation address is a valid hexadecimal number
    char *endptr;
    int relocationAddress = (int)strtol(argv[2], &endptr, 16);
    if (*endptr != '\0') {
        fprintf(stderr, "Error: Invalid relocation address.\n");
        return 1;
    }

    int isSICXE = 0;
    if (strcmp(argv[3], "SIC") == 0) {
        isSICXE = 0;
    } else if (strcmp(argv[3], "SICXE") == 0) {
        isSICXE = 1;
    } else {
        fprintf(stderr, "Error: Third argument must be SIC or SICXE.\n");
        return 1;
    }

    return 0;
}

void proccessObjectFile(const char *filename, int relocationAddress, int isSICXE) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(1);
    }

    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        // Skip empty lines
        if (line[0] == '\n' || line[0] == '\0') {
            continue;
        }

        char recordType = line[0];
        switch (recordType) {
            case 'H' || 'M':
                // Header record
                break;
            case 'T': {
                char *endptr;
                int startAddress = strtol(line + 1, &endptr, 16);
                if (*endptr != '\0') {
                    fprintf(stderr, "Error: Invalid start address.\n");
                    fclose(file);
                    exit(1);
                }
                startAddress += relocationAddress;
                printf("T%06X%s", startAddress, line + 7);
                break;
            }
            case 'E': {
                char *endptr;
                int startAddress = strtol(line + 1, &endptr, 16);
                if (*endptr != '\0') {
                    fprintf(stderr, "Error: Invalid start address.\n");
                    fclose(file);
                    exit(1);
                }
                startAddress += relocationAddress;
                printf("E%06X\n", startAddress);
                break;
            }
            default: {
                fprintf(stderr, "Error: Invalid record type.\n");
                fclose(file);
                exit(1);
            }
        }
        fclose(file);

    }
}
