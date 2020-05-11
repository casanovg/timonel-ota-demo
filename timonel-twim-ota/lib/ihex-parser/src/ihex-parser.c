/*
 *******************************
 * Intel Hex Parser Library    *
 * Version: 0.1 "Benja"        *
 *******************************
 */

#include "ihex-parser.h"

// int res;
// char *file = NULL;

// // Command argument parsing
// int run = 0;
// int file_type = FILE_TYPE_INTEL_HEX;
// int arg_pointer = 1;

// int startAddress = 1, endAddress = 0;

// memset(dataBuffer, 0xFF, sizeof(dataBuffer));

// if (file_type == FILE_TYPE_INTEL_HEX) {
//     if (parseIntelHex(file, dataBuffer, &startAddress, &endAddress)) {
//         printf("// Error loading or parsing hex file!\n");
//         return EXIT_FAILURE;
//     }
// }

// if (file_type == FILE_TYPE_RAW) {
//     if (parseRaw(file, dataBuffer, &startAddress, &endAddress)) {
//         printf("// Error loading raw file!\n");
//         return EXIT_FAILURE;
//     }

//     if (startAddress >= endAddress) {
//         printf("// No data in input file, exiting!\n");
//         return EXIT_FAILURE;
//     }
// }

// Function parseUntilColon
int parseUntilColon(FILE *file_pointer) {
    int character;

    do {
        character = getc(file_pointer);
    } while (character != ':' && character != EOF);

    return character;
}

// Function parseHex
int parseHex(FILE *p_file, uint8_t num_digits) {
    uint8_t iter;
    uint8_t temp[9];

    for (iter = 0; iter < num_digits; iter++) {
        temp[iter] = getc(p_file);
    }
    temp[iter] = 0;

    return strtol(temp, NULL, 16);
}

// Function parseIntelHex
int parseIntelHex(char *hexfile, uint8_t *buffer, int *startAddr, int *endAddr) {
    int address, base, d, segment, i, lineLen, sum;
    FILE *input;

    //input = strcmp(hexfile, "-") == 0 ? stdin : fopen(hexfile, "r");
    if (strcmp(hexfile, "-") == 0) {
        input = stdin;
    } else {
        fopen(hexfile, "r");
    }

    if (input == NULL) {
        printf("//> Error opening %s: %s\n", hexfile, strerror(errno));
        return 1;
    }

    while (parseUntilColon(input) == ':') {
        sum = 0;
        sum += lineLen = parseHex(input, 2);
        base = address = parseHex(input, 4);
        sum += address >> 8;
        sum += address;
        sum += segment = parseHex(input, 2); /* segment value? */
        if (segment != 0) {                  /* ignore lines where this byte is not 0 */
            continue;
        }

        for (i = 0; i < lineLen; i++) {
            d = parseHex(input, 2);
            buffer[address++] = d;
            sum += d;
        }

        sum += parseHex(input, 2);
        if ((sum & 0xff) != 0) {
            printf("//> Warning: Checksum error between address 0x%x and 0x%x\n", base, address);
        }

        if (*startAddr > base) {
            *startAddr = base;
        }
        if (*endAddr < address) {
            *endAddr = address;
        }
    }

#if (DEBUGLVL > 0)
    // GC: Printing addresses
    printf("\n//\n");
    printf("// Start Address: 0x%x \n", *startAddr);
    printf("// End Address: 0x%x \n//\n", *endAddr);
    // GC: Printing payload array definition ...
    //printf("const byte payldType = 0;    /* Application Payload */\n\n");
    //printf("const byte payload[%i] = {", *endAddr + 1);
    printf("uint8_t payload[%i] = {", *endAddr + 1);

    // GC: Printing loaded buffer ...
    printf("\n    ");
    int l = 0;
    for (i = 0; i <= *endAddr; i++) {
        printf("0x%02x", buffer[i]);
        if (i <= (*endAddr) - 1) {
            printf(", ");
        }
        if (l++ == BYTESPERLINE - 1) {
            printf("\n    ");
            l = 0;
        }
    }
    printf("\n};\n\n//\n");
#endif

    fclose(input);
    return 0;
}

//Function parseRaw
int parseRaw(char *filename, unsigned char *data_buffer, int *start_address, int *end_address) {
    FILE *input;

    input = strcmp(filename, "-") == 0 ? stdin : fopen(filename, "r");

    if (input == NULL) {
        printf("//> Error reading %s: %s\n", filename, strerror(errno));
        return 1;
    }

    *start_address = 0;
    *end_address = 0;

    // Read file bytes
    int byte = 0;
    while (1) {
        byte = getc(input);
        if (byte == EOF) break;

        *data_buffer = byte;
        data_buffer += 1;
        *end_address += 1;
    }

    fclose(input);
    return 0;
}
