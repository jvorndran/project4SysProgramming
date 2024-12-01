#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char opcode[3];
    int address;
} OpcodeEntry;

void processObjectFile(const char *filename, int relocationAddress, int isSICXE) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return;
    }

    OpcodeEntry opcodes[100];
    int opcodeCount = 0;

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == 'T') {
            // Parse Text (T) record
            int startAddr, length;
            sscanf(line + 1, "%06x%02x", &startAddr, &length);

            for (int i = 0; i < length * 2; i += 6) {
                OpcodeEntry entry;
                strncpy(entry.opcode, line + 9 + i, 2); // Extract 2-char opcode
                entry.opcode[2] = '\0';                // Null-terminate opcode

                char addressStr[5];
                strncpy(addressStr, line + 11 + i, 4); // Extract 4-char address
                addressStr[4] = '\0';
                entry.address = (int)strtol(addressStr, NULL, 16);

                opcodes[opcodeCount++] = entry;
            }
        } else if (line[0] == 'M') {
            // Parse Modification (M) record
            int modAddress, modLength;
            sscanf(line + 1, "%06x%02x", &modAddress, &modLength);

            // Apply modification
            for (int i = 0; i < opcodeCount; i++) {
                if (opcodes[i].address == modAddress) {
                    // Adjust the address with relocation
                    opcodes[i].address += relocationAddress;
                    break;
                }
            }
        }
    }

    fclose(file);

    // Print the modified opcode dictionary
    printf("Relocated Opcodes:\n");
    for (int i = 0; i < opcodeCount; i++) {
        printf("Opcode: %s, Address: %04X\n", opcodes[i].opcode, opcodes[i].address);
    }
    // // print T records
    // for (int i = 0; i < opcodeCount; i++) {
    //     // accumulate the the object code until it reaches 60 characters
    //     char objectCode[61] = "";
    //     int objectCodeLength = 0;
    //     while (objectCodeLength < 60 && i < opcodeCount) {
    //         char objectCodePart[7];
    //         sprintf(objectCodePart, "%s%04X", opcodes[i].opcode, opcodes[i].address);
    //         if (objectCodeLength + strlen(objectCodePart) <= 60) {
    //             strcat(objectCode, objectCodePart);
    //             objectCodeLength += strlen(objectCodePart);
    //             i++;
    //         } else {
    //             break;
    //         }
    //     }
    //     // print T record
    //     printf("T%06X%02X%s\n", opcodes[i - 1].address, objectCodeLength / 2, objectCode);
    // }
    // // print E record
    // printf("E%06X\n", relocationAddress);
}

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
    if (strcmp(filename + strlen(filename) - 8, ".sic.obj") != 0) {
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

    processObjectFile(filename, relocationAddress, isSICXE);

    return 0;
}
