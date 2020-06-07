/*
 *******************************
 * Intel Hex Parser Library    *
 * Version: 0.99 "Benja"       *
 *******************************
 */

#include "ihex-parser.h"

// Constructor
HexParser::HexParser() {
    // Constructor
}

// Destructor
HexParser::~HexParser() {
    // Destructor
}

// Function ParseIHexFormat
bool HexParser::ParseIHexFormat(String serialized_file, uint8_t *payload) {
    uint16_t file_len = serialized_file.length();
    uint16_t payload_ix = 0;
    uint8_t line_count = 0;
    bool checksum_err = false;
    // Loops through the serialized file
    for (int ix = 0; ix < file_len; ix++) {
        // If a record start is detected
        if (serialized_file.charAt(ix) == IHEX_START_CODE) {
            uint8_t byte_count = strtoul(serialized_file.substring(ix + 1, ix + 3).c_str(), 0, 16);
            uint16_t address = strtoul(serialized_file.substring(ix + 3, ix + 7).c_str(), 0, 16);
            uint8_t record_type = strtoul(serialized_file.substring(ix + 7, ix + 9).c_str(), 0, 16);
            uint8_t checksum = strtoul(serialized_file.substring(ix + 9 + (byte_count << 1), ix + 11 + (byte_count << 1)).c_str(), 0, 16);
            // Serial.printf_P("\rix: %d line len: %d type: %d check: %d\n\r", ix, byte_count, record_type, checksum);
            if (record_type == 0) {
                uint8_t record_check = 0;
                // Serial.printf_P("%02d) [%04X] ", line_count++, address);
                for (int data_pos = ix + 9; data_pos < (ix + 9 + (byte_count << 1)); data_pos += 2) {
                    uint8_t ihex_data = strtoul(serialized_file.substring(data_pos, data_pos + 2).c_str(), 0, 16);
                    record_check += ihex_data;
                    payload[payload_ix] = ihex_data;
                    // Serial.printf_P(".%02X", ihex_data);
                    payload_ix++;
                }
                record_check = (~((byte_count + ((address >> 8) & 0xFF) + (address & 0xFF) + record_type + record_check) & 0xFF)) + 1;
                if (record_check != checksum) {
                    // Serial.printf_P(" { %02X ERROR }", record_check);
                    checksum_err = true;
                } else {
                    // Serial.printf_P(" { %02X }", record_check);
                }
                // Serial.printf_P("\n\r");
            }
        }
    }
    return checksum_err;
}

// Function GetIHexSize
uint16_t HexParser::GetIHexSize(String serialized_file) {
    uint16_t file_length = serialized_file.length();
    uint16_t data_size = 0;
    for (int ix = 0; ix < file_length; ix++) {
        if (serialized_file.charAt(ix) == ':') {
            uint8_t record_type = strtoul(serialized_file.substring(ix + 7, ix + 9).c_str(), 0, 16);
            uint8_t byte_count = strtoul(serialized_file.substring(ix + 1, ix + 3).c_str(), 0, 16);
            //uint8_t checksum = strtoul(serialized_file.substring(ix + 9 + (byte_count << 1), ix + 11 + (byte_count << 1)).c_str(), 0, 16);
            if (record_type == 0) {
                data_size += byte_count;
            }
        }
    }
    Serial.printf_P("\r\n[%s] File length: %d\n\r", __func__, file_length);
    Serial.printf_P("[%s] Payload size: %d\n\r", __func__, data_size);
    return data_size;
}