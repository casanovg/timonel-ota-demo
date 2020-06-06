// ---------------------------------------------
// Test 01 - 2019-06-06 - Gustavo Casanova
// .............................................
// Hex parser test
// ---------------------------------------------

#include <tst-01.h>

using namespace hexparser;

int main() {
    HexParser ihex;

    uint16_t payload_size = ihex.GetIHexSize(http_string);
    //payload_size += 10;
    printf("Array size: %d\n\r", payload_size);

    uint8_t payload[payload_size];

    printf("\n\r================================================\n\r");
    printf("Parsed firmware dump:\r\n");
    if (ihex.ParseIHexFormat(http_string, payload)) {
        printf("\n\rERROR!!!\n\n\r");
    }
    printf("\r::::::::::::::::::::::::::::::::::::::::::::::::\n\r");

    uint8_t line_count = 0;
    uint8_t nl = 0;

    printf("%02d) ", line_count++);
    for (uint16_t q = 0; q < payload_size; q++) {
        printf(".%02X", payload[q]);
        if ((nl++ == 15) || payload[q] == '\n') {
            printf("\n\r");
            printf("%02d) ", line_count++);
            nl = 0;
        }
    }

    printf("\n\r================================================\n\r");

    return 0;
}
