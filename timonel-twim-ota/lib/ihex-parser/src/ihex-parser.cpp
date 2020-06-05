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

// Function GetIHexArray
void HexParser::GetIHexPayload(String serialized_file, uint8_t *payload) {
    uint16_t file_len = serialized_file.length();
    uint16_t payload_ix = 0;
    uint8_t line_count = 0;
    Serial.printf("File len: %d\r\n", file_len);

    for (int ix = 0; ix < file_len; ix++) {
        if (serialized_file.charAt(ix) == ':') {
            uint8_t record_type = strtoul(serialized_file.substring(ix + 7, ix + 9).c_str(), 0, 16);
            uint8_t line_data_length = strtoul(serialized_file.substring(ix + 1, ix + 3).c_str(), 0, 16);
            uint8_t checksum = strtoul(serialized_file.substring(ix + 9 + (line_data_length << 1), ix + 11 + (line_data_length << 1)).c_str(), 0, 16);

            //Serial.printf("\rix: %d line len: %d type: %d check: %d\n\r", ix, line_data_length, record_type, checksum);

            Serial.printf("%02d) ", line_count++);
            for (int data_pos = ix + 9; data_pos < (ix + 9 + (line_data_length << 1)); data_pos += 2) {
                uint8_t data_byte = strtoul(serialized_file.substring(data_pos, data_pos + 2).c_str(), 0, 16);
                payload[payload_ix] = data_byte;
                Serial.printf(":%02X", data_byte);
                payload_ix++;
            }
            Serial.println("");
        }
    }
}

// Function GetIHexSize
uint16_t HexParser::GetIHexSize(String serialized_file) {
    uint16_t file_length = serialized_file.length();
    uint16_t file_data_length = 0;
    for (int ix = 0; ix < file_length; ix++) {
        if (serialized_file.charAt(ix) == ':') {
            uint8_t record_type = strtoul(serialized_file.substring(ix + 7, ix + 9).c_str(), 0, 16);
            uint8_t line_data_length = strtoul(serialized_file.substring(ix + 1, ix + 3).c_str(), 0, 16);
            //uint8_t checksum = strtoul(serialized_file.substring(ix + 9 + (line_data_length << 1), ix + 11 + (line_data_length << 1)).c_str(), 0, 16);
            if (record_type == 0) {
                file_data_length += line_data_length;
            }
        }
    }
    Serial.printf("File length: %d\r\n", file_length);
    Serial.printf("Data length: %d\r\n", file_data_length);
    return file_data_length;
}