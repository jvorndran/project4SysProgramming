#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TEXT_RECORDS 100
#define MAX_MODIFICATION_RECORDS 100

typedef struct {
    char name[7];
    int startAddress;
    int programLength;
} ProgramHeader;

typedef struct {
    char opcode[3];
    int address;
} Instruction;

typedef struct {
    int startAddress;
    int length;
    char objectCode[61];
} TextRecord;

typedef struct {
    int address;
    int length;
    char flag;
    char symbol[7];
} ModificationRecord;

ProgramHeader header;
TextRecord textRecords[MAX_TEXT_RECORDS];
ModificationRecord modRecords[MAX_MODIFICATION_RECORDS];
int textCount, modCount, execAddress;

void parseObjectFile(const char *filename, ProgramHeader *header,
                     TextRecord *textRecords, ModificationRecord *modRecords, int *textCount, int *modCount, int *execAddress) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(1);
    }

    char line[256];
    *textCount = 0;
    *modCount = 0;
    *execAddress = -1;

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';

        switch (line[0]) {
            case 'H': {
                // Parse header record
                sscanf(line, "H%6s%06x%06x", header->name, &header->startAddress, &header->programLength);
                break;
            }
            case 'T': {
                // Parse text record
                TextRecord *current = &textRecords[(*textCount)++];
                sscanf(line, "T%06x%2x%60s", &current->startAddress, &current->length, current->objectCode);
                break;
            }
            case 'M': {
                // Parse modification record
                ModificationRecord *current = &modRecords[(*modCount)++];
                sscanf(line, "M%06x%02x%c%6s", &current->address, &current->length, &current->flag, current->symbol);
                break;
            }
            case 'E': {
                // Parse end record
                sscanf(line, "E%06x", execAddress);
                break;
            }
            default:
                fprintf(stderr, "Unknown record type: %c\n", line[0]);
                exit(1);
        }
    }

    fclose(file);
}

// Function to apply modifications to the object code
void applyModificationRecords(ProgramHeader *header, TextRecord *textRecords, int textCount,
                              ModificationRecord *modRecords, int modCount, int relocationAddress) {

    printf("Applying Modifications...\n\n");

    for (int i = 0; i < modCount; i++) {
        ModificationRecord *mod = &modRecords[i];
        int adjustedAddress = mod->address + header->startAddress;

        // Locate the text record containing this address
        for (int j = 0; j < textCount; j++) {
            TextRecord *record = &textRecords[j];

            // Check if adjustedAddress falls within the range of this record
            if (adjustedAddress >= record->startAddress &&
                adjustedAddress < record->startAddress + record->length) {

                int offset = adjustedAddress - record->startAddress;
                int hexOffset = offset + 1;

                // Ensure hexOffset is within bounds
                int lengthInBytes = (mod->length + 1) / 2;


                char newObjectCode[1024] = {0};


                strncpy(newObjectCode, record->objectCode, hexOffset);

                newObjectCode[hexOffset] = '\0';

                char valueHex[9] = {0};
                strncpy(valueHex, &record->objectCode[hexOffset], lengthInBytes * 2);
                valueHex[lengthInBytes * 2] = '\0';


                int value = (int)strtol(valueHex, NULL, 16);
                if (mod->flag == '+') {
                    value += relocationAddress;
                } else if (mod->flag == '-') {
                    value -= relocationAddress;
                }

                snprintf(&newObjectCode[hexOffset], lengthInBytes * 2 + 1, "%0*X", lengthInBytes * 2, value);

                strcat(newObjectCode, &record->objectCode[hexOffset + lengthInBytes * 2]);

                strncpy(record->objectCode, newObjectCode, sizeof(record->objectCode) - 1);
                record->objectCode[sizeof(record->objectCode) - 1] = '\0';  // Ensure null termination

                // Print the modification
                printf("Processing Modification Record: %06X%02X%c%s\n", mod->address, mod->length, mod->flag, mod->symbol);
                printf("Modifying value: %s\n", valueHex);
                printf("Modified value: %.*s\n\n", lengthInBytes * 2, &record->objectCode[hexOffset]);

                break;  // Exit once modification is applied
                }
        }

    }

    printf("\nModified Object Code:\n");
    // Print header record
    printf("H%-6s%06X%06X\n", header->name, header->startAddress + relocationAddress, header->programLength);
    // Print modified text records
    for (int i = 0; i < textCount; i++) {
        TextRecord *record = &textRecords[i];
        printf("T%06X%02X%s\n", record->startAddress + relocationAddress, record->length, record->objectCode);
    }

    // Print end record
    printf("E%06X\n", execAddress + relocationAddress);

    printf("=================================================================================================================\n");
}

void printRelocatedObjectFile() {

}

void printOriginalObjectFile() {

    printf("=================================================================================================================\n");
    printf("Original Object Code:\n\n");
    printf("Header:\n");
    printf("Program Name: %s\n", header.name);
    printf("Start Address: %06X\n", header.startAddress);
    printf("Program Length: %06X Bytes (Hex)\n", header.programLength);

    printf("\nText Records:\n");
    for (int i = 0; i < textCount; i++) {
        printf("Start Address: %06X, Length: %02X, Object Code: %s\n",
               textRecords[i].startAddress, textRecords[i].length, textRecords[i].objectCode);
    }

    printf("\nModification Records:\n");
    for (int i = 0; i < modCount; i++) {
        printf("Starting Address: %06X, Length (Half-bytes): %02X, Flag: %c, Symbol: %s\n",
               modRecords[i].address, modRecords[i].length, modRecords[i].flag, modRecords[i].symbol);
    }

    printf("\nEnd Record:\n");
    printf("E%06X\n\n", execAddress);

    printf("=================================================================================================================\n");
}

void processObjectFile(const char *filename, int relocationAddress, int isSICXE) {

    parseObjectFile(filename, &header, textRecords, modRecords, &textCount, &modCount, &execAddress);

    printOriginalObjectFile();

    applyModificationRecords(&header, textRecords, textCount, modRecords, modCount, relocationAddress);

    printRelocatedObjectFile();

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
