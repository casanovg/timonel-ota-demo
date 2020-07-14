/*
 *******************************
 * Intel Hex Parser Library    *
 * Version: 0.99 "Benja"       *
 *******************************
 */

#include "ihex-parser.h"

namespace hexparser {

// Constructor
HexParser::HexParser() {
    // Constructor
}

// Destructor
HexParser::~HexParser() {
    // Destructor
}

// Function ParseIHexFormat
bool HexParser::ParseIHexFormat(string serialized_file, uint8_t *payload) {
    uint16_t file_len = serialized_file.length();
    uint16_t payload_ix = 0;
    uint8_t line_count = 0;
    bool checksum_err = false;
    // Loops through the serialized file
    for (int ix = 0; ix < file_len; ix++) {
        // If a record start is detected
        if (serialized_file.at(ix) == IHEX_START_CODE) {
            uint8_t byte_count = strtoul(serialized_file.substr(ix + 1, 2).c_str(), 0, 16);
            uint16_t address = strtoul(serialized_file.substr(ix + 3, 4).c_str(), 0, 16);
            uint8_t record_type = strtoul(serialized_file.substr(ix + 7, 2).c_str(), 0, 16);
            uint8_t checksum = strtoul(serialized_file.substr(ix + 9 + (byte_count << 1), 2).c_str(), 0, 16);
            // printf("\r%02d) ix: %d line len: %d type: %d check: %d\n\r", line_count, ix, byte_count, record_type, checksum); // DEBUG
            if (record_type == 0) {
                uint8_t record_check = 0;
                printf("%02d) [%04X] ", line_count++, address);
                for (int data_pos = ix + 9; data_pos < (ix + 9 + (byte_count << 1)); data_pos += 2) {
                    uint8_t ihex_data = strtoul(serialized_file.substr(data_pos, 2).c_str(), 0, 16);
                    record_check += ihex_data;
                    payload[payload_ix] = ihex_data;
                    printf(":%02X", ihex_data);
                    payload_ix++;
                }
                record_check = (~((byte_count + ((address >> 8) & 0xFF ) + (address & 0xFF)  + record_type + record_check ) & 0xFF)) + 1;
                if (record_check != checksum) {
                    printf(" { %02X ERROR }", record_check);
                    checksum_err = true;
                } else {
                    printf(" { %02X }", record_check);

                }
                printf("\n\r");
            }
        }
    }
    return checksum_err;
}

// Function GetIHexSize
uint16_t HexParser::GetIHexSize(string serialized_file) {
    uint16_t file_length = serialized_file.length();
    uint16_t file_data_length = 0;

    // serialized_file.substr(8)

    for (int ix = 0; ix < file_length; ix++) {
        if (serialized_file.at(ix) == ':') {
            uint8_t record_type = strtoul(serialized_file.substr(ix + 7, 2).c_str(), 0, 16);
            uint8_t byte_count = strtoul(serialized_file.substr(ix + 1, 2).c_str(), 0, 16);
            if (record_type == 0) {
                file_data_length += byte_count;
            }
        }
    }
    printf("\r\nFile length: %d\r\n", file_length);
    printf("Data length: %d\r\n", file_data_length);
    return file_data_length;
}

}  // namespace hexparser