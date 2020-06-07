/*
  timonel-twim-ota.cpp
  =====================
  Timonel OTA Demo v1.0
  ----------------------------------------------------------------------------
  This demo updates an ESP8266 I2C master over-the-air:
  1-The ESP9266 firmware runs Timonel bootloader master amd has an ATTtiny85
    application compiled inside.
  2-After an OTA update, the ESP8266 Timonel master resets the ATtiny85 to
    start Timonel bootloader on it.
  3-The ESP8266 uploads the new firmware into the ATtiny85 and, if it is
    sucessful, resets it to exit Timonel and run the new firmware.
  4-The ESP8266 exits Timonel bootloader master and enters its normal
    operation loop, where it also makes frequent checks for new OTA updates.
  ----------------------------------------------------------------------------
  2020-05-09 Gustavo Casanova
  ----------------------------------------------------------------------------
*/

#include "timonel-twim-ota.h"

// Use Firefox browser to get the web site certificate SHA1 fingerprint (case-insensitive)
const char fingerprint[] PROGMEM = "70 94 de dd e6 c4 69 48 3a 92 70 a1 48 56 78 2d 18 64 e0 b7";

// Prototypes
void ClrScr(void);
String GetHttpDocument(String url, char terminator, const char host[], const int port, const char fingerprint[]);

// Setup block
void setup() {
    const char ssid[] = SSID;
    const char password[] = PASS;
    const char host[] = "raw.githubusercontent.com";
    const int port = 443;

    Serial.begin(115200);
    Serial.println("");

    // @@@@@@@@@@@@@@@@@@@@@@@@@

    if (SPIFFS.begin()) {
        //Serial.println("SPIFFS filesystem mounted with success\r\n");
    } else {
        Serial.println("Error mounting the SPIFFS file system\r\n");
        return;
    }

    String fw_current = "";
    File file = SPIFFS.open("/fw-current-ver.txt", "r");
    Serial.printf("Reading current ATtiny85 firmware version: ");
    if (file && file.size()) {
        while (file.available()) {
            fw_current += char(file.read());
        }
        file.close();
        Serial.printf("s%\n\r", fw_current.c_str());
    } else {
        Serial.printf("none\n\r");
    }

    // @@@@@@@@@@@@@@@@@@@@@@@@@

    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("");
    Serial.printf("WiFi connected! IP address: %s\n\r", WiFi.localIP().toString().c_str());

    // Check the latest firmware version available for the slave device
    char terminator = '\n';
    String url = "/casanovg/timonel-ota-demo/master/fw-attiny85/latest-version.md";
    String fw_latest = GetHttpDocument(url, terminator, host, port, fingerprint);

    Serial.println(".......................................................");
    Serial.println("Latest firwmare version available for ATTiny85: " + fw_latest);
    Serial.println(".......................................................");

    if (fw_current != fw_latest) {
        // Update needed: get the latest firmware available for the ATtiny85 from internet
        terminator = '\0';
        url = "/casanovg/timonel-ota-demo/master/fw-attiny85/firmware-" + fw_latest + ".hex";
        String fw_file_rem = GetHttpDocument(url, terminator, host, port, fingerprint);

        // Save the latest firmware into the filesystem
        File file = SPIFFS.open("/fw-latest-dat.txt", "w+");
        if (!file) {
            Serial.println("Error opening file for writing");
            //return;
        }
        uint16 bytes_written = file.print(fw_file_rem);
        if (bytes_written > 0) {
            Serial.printf("File write successful (%d bytes saved) ...", bytes_written);
        } else {
            Serial.printf("File write failed!");
        }
        // Read the save firmware file
        String fw_file_loc = "";
        file.seek(0, SeekSet);
        if (file && file.size()) {
            Serial.println("\r\nDumping saved firmware file:");
            while (file.available()) {
                fw_file_loc += char(file.read());
            }
            file.close();
            Serial.println(fw_file_loc);
        }
        //file.close();        
        // Parse the new firmware file
        HexParser *ihex = new HexParser();
        uint16_t payload_size = ihex->GetIHexSize(fw_file_loc);
        Serial.printf("\n\rPayload size: %d\n\r", payload_size);
        uint8_t payload[payload_size];
        uint8_t line_count = 0;
        uint8_t nl = 0;
        Serial.println("\n\r================================================");
        Serial.println("Firwmare dump:");
        if (ihex->ParseIHexFormat(fw_file_loc, payload)) {
            Serial.println("\r\nPayload checksum error!\n\r");
            //Serial.println("\r::::::::::::::::::::::::::::::::::::::::::::::::");
        } else {
            Serial.printf("%02d) ", line_count++);
            for (uint16_t q = 0; q < payload_size; q++) {
                Serial.printf(".%02X", payload[q]);
                if (nl++ == 15) {
                    Serial.print("\r\n");
                    Serial.printf("%02d) ", line_count++);
                    nl = 0;
                }
            }
        }
        delete ihex;
        Serial.println("\r\n================================================\n\r");
        
        // Update ATtiny85 firmware ...

        // Detect the ATtiny85 application I2C address
        TwiBus *twi_bus = new TwiBus(SDA, SCL);
        uint8_t twi_address = twi_bus->ScanBus();
        delete twi_bus;
        NbMicro *micro = nullptr;
        Timonel *timonel = nullptr;
        Serial.printf("\n\rTWI address detected: %d", twi_address);
        if (twi_address >= HIG_TML_ADDR) {
            Serial.printf(" device running an user application\n\r");
            micro = new NbMicro(twi_address, SDA, SCL);
            micro->TwiCmdXmit(RESETMCU, ACKRESET);
            delete micro;
        }
        delay(1000);
        if ((twi_address < HIG_TML_ADDR) && (twi_address != 0)) {
            Serial.printf(" device running Timonel bootloader\n\r");
            timonel = new Timonel(twi_address, SDA, SCL);
            timonel->GetStatus();
            uint8_t errors = timonel->UploadApplication(payload, payload_size);
            if (errors == 0) {
                USE_SERIAL.printf_P("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b successful!      \n\r");
                delay(500);
                timonel->RunApplication();
                Serial.printf("\n\rIf we're lucky, application should be running by now ... \n\r");
              
            } else {
                USE_SERIAL.printf_P("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b error! (%d)           \n\r", errors);
            }
            delete timonel; 
        }
    
    } else {
        // No firmware update needed
    }

    SPIFFS.end();

    // ------------------ /// ************************* /// ---------------------///

    //     if (SPIFFS.begin()) {
    //         Serial.println("SPIFFS filesystem mounted with success");
    //     } else {
    //         Serial.println("Error mounting the SPIFFS file system");
    //         return;
    //     }

    //     File file = SPIFFS.open("/fw-1.4.txt", "w+");

    //     if (!file) {
    //         Serial.println("Error opening file for writing");
    //         return;
    //     }

    //     int bytesWritten = file.print("NICEBOTS FILE WRITING TEST ... SUCUNDUN");

    //     if (bytesWritten > 0) {
    //         Serial.println("File was written");
    //         Serial.println(bytesWritten);

    //     } else {
    //         Serial.println("File write failed");
    //     }

    //     // **************************************************

    //     String fileData = "";

    //     file.seek(0, SeekSet);

    //     if (file && file.size()) {
    //         Serial.println("Dumping firmware file");

    //         while (file.available()) {
    //             fileData += char(file.read());
    //         }
    //         file.close();
    //     }

    //     Serial.println(fileData);

    //     file.close();
    // }
}

// main loop
void loop(void) {
    // Empty main loop
}

// Function clear screen
void ClrScr(void) {
    Serial.write(27);     // ESC command
    Serial.print("[2J");  // clear screen command
    Serial.write(27);     // ESC command
    Serial.print("[H");   // cursor to home command
}

// // Function HttpsGet
// String HttpsGet(char* host, String url, char[] fingerprint) {
//     // Use WiFiClientSecure class to create TLS connection
//     WiFiClientSecure client;
//     Serial.print("Connecting to web site: ");
//     Serial.println(host);

//     Serial.printf("Using fingerprint: %s\n\r", fingerprint);
//     client.setFingerprint(fingerprint);

//     if (!client.connect(host, https_port)) {
//         Serial.println("Connection failed");
//         return;
//     }

//     // Read the latest firmware available to write to the slave device
//     String url = "/casanovg/timonel-ota-demo/master/fw-attiny85/latest-version.md";
//     Serial.print("URL Request: ");
//     Serial.println(url);

//     client.print(String("GET ") + url + " HTTP/1.1\r\n" +
//                  "Host: " + host + "\r\n" +
//                  "User-Agent: TimonelTwiMOtaESP8266\r\n" +
//                  "Connection: close\r\n\r\n");

//     Serial.println("Request sent");
//     while (client.connected()) {
//         String line = client.readStringUntil('\n');
//         if (line == "\r") {
//             Serial.println("Headers received");
//             break;
//         }
//     }

//     String fw_version = client.readStringUntil('\n');
// }

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

// Function GetHttpDocument
String GetHttpDocument(String url, char terminator, const char host[], const int port, const char fingerprint[]) {
    // Use WiFiClientSecure class to create TLS connection
    WiFiClientSecure client;
    //Serial.print("Connecting to web site: ");
    //Serial.println(host);

    //Serial.printf("Using fingerprint: %s\n\r", fingerprint);
    client.setFingerprint(fingerprint);

    if (!client.connect(host, port)) {
        Serial.println("Connection failed");
        //return;
    }

    //String url = "/casanovg/timonel-ota-demo/master/fw-attiny85/latest-version.md";
    //Serial.print("URL Request: ");
    //Serial.println(url);

    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "User-Agent: TimonelTwiMOtaESP8266\r\n" +
                 "Connection: close\r\n\r\n");
    //Serial.println("Request sent");
    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
            //Serial.println("Headers received");
            break;
        }
    }
    //String http_string = client.readStringUntil('\n');
    String http_string = client.readStringUntil(terminator);
    return http_string;
}
