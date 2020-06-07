/*
  timonel-twim-ota.cpp
  =====================
  Timonel OTA Demo v1.0
  ----------------------------------------------------------------------------
  This demo updates an ESP8266 I2C master over-the-air:
  1-The ESP9266 firmware runs Timonel bootloader master amd has an ATttiny85
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
String ReadFile(const char file_name[]);
uint8_t WriteFile(const char file_name[], const String file_data);
String GetHttpDocument(String url, char terminator, const char host[], const int port, const char fingerprint[]);

/*  ___________________
   |                   | 
   |    Setup block    |
   |___________________|
*/
void setup() {
    const char ssid[] = SSID;
    const char password[] = PASS;
    const char host[] = "raw.githubusercontent.com";
    const int port = 443;

    Serial.begin(115200);
    Serial.printf_P("\n\r");

    // @@@@@@@@@@@@@@@@@@@@@@@@@

    // Read the running ATtiny85 firmware version from the filesystem
    String fw_current_loc = ReadFile("/fw-current-ver.txt");

    // @@@@@@@@@@@@@@@@@@@@@@@@@

    // Open WiFi connection to access the internet
    Serial.printf_P("Connecting to WiFi: ");
    Serial.printf_P(ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.printf_P(".");
    }
    Serial.printf_P("\n\r");
    Serial.printf_P("WiFi connected! IP address: %s\n\r", WiFi.localIP().toString().c_str());

    // Check the latest firmware version available for the slave device
    char terminator = '\n';
    String url = "/casanovg/timonel-ota-demo/master/fw-attiny85/firmware-latest.md";
    String fw_latest_rem = GetHttpDocument(url, terminator, host, port, fingerprint);
    Serial.printf_P(".......................................................\n\r");
    Serial.printf_P("Latest firwmare version available for ATtiny85: %s\n\r", fw_latest_rem.c_str());
    Serial.printf_P(".......................................................\n\r");

    //if (fw_current_loc != fw_latest_rem) {
    if (true) {
        // Update needed: get the latest firmware available for the ATtiny85 from internet
        terminator = '\0';
        url = "/casanovg/timonel-ota-demo/master/fw-attiny85/firmware-" + fw_latest_rem + ".hex";
        String fw_file_rem = GetHttpDocument(url, terminator, host, port, fingerprint);

        // Save the latest firmware into the filesystem
        WriteFile("/fw-latest.hex", fw_file_rem);

        // Read the saved firmware file
        String fw_file_loc = ReadFile("/fw-latest.hex");

        // Parse the new firmware file
        HexParser *ihex = new HexParser();
        uint16_t payload_size = ihex->GetIHexSize(fw_file_loc);
        //Serial.printf_P("\n\rPayload size: %d\n\r", payload_size);
        uint8_t payload[payload_size];
        uint8_t line_count = 0;
        uint8_t nl = 0;
        Serial.printf_P("\n\r================================================\n\r");
        Serial.printf_P("Firwmare dump:\n\r");
        if (ihex->ParseIHexFormat(fw_file_loc, payload)) {
            Serial.printf_P("\n\rPayload checksum error!\n\n\r");
            //Serial.printf_P("\r::::::::::::::::::::::::::::::::::::::::::::::::\n\r");
        } else {
            Serial.printf_P("%02d) ", line_count++);
            for (uint16_t q = 0; q < payload_size; q++) {
                Serial.printf_P(".%02X", payload[q]);
                if (nl++ == 15) {
                    Serial.printf_P("\n\r");
                    Serial.printf_P("%02d) ", line_count++);
                    nl = 0;
                }
            }
        }
        delete ihex;
        Serial.printf_P("\n\r================================================\n\n\r");

        // Update ATtiny85 firmware ...

        // Detect the ATtiny85 application I2C address
        TwiBus *twi_bus = new TwiBus(SDA, SCL);
        uint8_t twi_address = twi_bus->ScanBus();
        delete twi_bus;
        NbMicro *micro = nullptr;
        Timonel *timonel = nullptr;
        Serial.printf_P("\n\rTWI address detected: %d", twi_address);
        // If the ATtiny85 is running a user application ...
        if (twi_address >= HIG_TML_ADDR) {
            Serial.printf_P(" device running an user application\n\r");
            micro = new NbMicro(twi_address, SDA, SCL);
            micro->TwiCmdXmit(RESETMCU, ACKRESET);
            delay(1000);
            delete micro;
            Serial.printf_P("\n\rIs the application stopped?\n\r");
        }
        delay(1000);
        // If the ATtiny85 is running Timonel bootloader
        if ((twi_address < HIG_TML_ADDR) && (twi_address != 0)) {
            Serial.printf_P(" device running Timonel bootloader\n\r");
            timonel = new Timonel(twi_address, SDA, SCL);
            timonel->GetStatus();
            uint8_t errors = timonel->UploadApplication(payload, payload_size);
            if (errors == 0) {
                USE_SERIAL.printf_P("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b successful!      \n\r");
                delay(500);
                timonel->RunApplication();
                Serial.printf_P("\n\rIf we're lucky, application should be running by now ... \n\r");

                // Save the latest firmware into the filesystem
                WriteFile("/fw-current-ver.txt", fw_latest_rem);

                // File file = SPIFFS.open("/fw-current-ver.txt", "w+");
                // if (!file) {
                //     Serial.printf_P("Error opening file for writing\n\r");
                //     //return;
                // }
                // uint16 bytes_written = file.print(fw_latest_rem);
                // if (bytes_written > 0) {
                //     Serial.printf_P("ATtiny85 firmware version info updated (%d bytes saved) ...\n\r", bytes_written);
                // } else {
                //     Serial.printf_P("ATtin85 firmware version info update failed!\n\r");
                // }
                // file.close();

            } else {
                USE_SERIAL.printf_P("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b error! (%d)           \n\r", errors);
            }
            delete timonel;
        }

    } else {
        // ATtiny85 firmware up to date
        Serial.printf_P("\n\rATtiny85 firmware update not needed ... \n\r");
        TwiBus *twi_bus = new TwiBus(SDA, SCL);
        uint8_t twi_address = twi_bus->ScanBus();
        delete twi_bus;
        Timonel *timonel = new Timonel(twi_address, SDA, SCL);
        timonel->GetStatus();
        timonel->RunApplication();
        delete timonel;
    }
    //SPIFFS.end();
}

uint16_t seconds = 60;

/*   ___________________
    |                   | 
    |     Main loop     |
    |___________________|
*/
void loop(void) {
    Serial.printf_P("\n\r");
    while (seconds) {
        Serial.printf_P(".%d ", seconds);
        delay(1000);
        seconds--;
    }
    Serial.printf_P("\n\n\rRestarting, bye!\n\n\r");
    delay(1000);
    ESP.restart();
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

// Function clear screen
void ClrScr(void) {
    Serial.write(27);        // ESC command
    Serial.printf_P("[2J");  // clear screen command
    Serial.write(27);        // ESC command
    Serial.printf_P("[H");   // cursor to home command
}

// Function GetHttpDocument
String GetHttpDocument(String url, char terminator, const char host[], const int port, const char fingerprint[]) {
    // Use WiFiClientSecure class to create TLS connection
    String http_string = "";
    WiFiClientSecure client;
    Serial.printf_P("[%s] Connecting to web site: %s", __func__, host);
    Serial.printf_P(" with fingerprint: %s\n\r", fingerprint);
    client.setFingerprint(fingerprint);
    if (!client.connect(host, port)) {
        Serial.printf_P("[%s] HTTP connection failed!\n\r", __func__);
        // Connection error!
    }
    Serial.printf_P("[%s] URL Request: %s\n\r", __func__, url.c_str());
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "User-Agent: TimonelTwiMOtaESP8266\r\n" +
                 "Connection: close\r\n\r\n");
    Serial.printf_P("[%s] Request sent ...\n\r", __func__);
    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
            Serial.printf_P("[%s] Headers received ...\n\r", __func__);
            break;
        }
    }
    http_string = client.readStringUntil(terminator);
    if (http_string != "") {
        Serial.printf_P("[%s] HTTP data received ...\n\r", __func__);
    } else {
        Serial.printf_P("[%s] No HTTP data received!\n\r", __func__);
    }
    return http_string;
}

// Function ReadFile
String ReadFile(const char file_name[]) {
    String file_data = "";
    if (SPIFFS.begin()) {
        Serial.printf_P("[%s] SPIFFS filesystem mounted ...\n\r", __func__);
    } else {
        Serial.printf_P("[%s] Error mounting the SPIFFS file system\n\r", __func__);
        // Mount error!
    }
    File file = SPIFFS.open("/fw-current-ver.txt", "r");
    Serial.printf_P("[%s] Reading file %s\n\r", __func__, file_name);
    if (file && file.size()) {
        while (file.available()) {
            file_data += char(file.read());
        }
        file.close();
        Serial.printf_P("%s\n\r", file_data.c_str());
    } else {
        Serial.printf_P("[%s] No data!\n\r", __func__);
    }
    return file_data;
    SPIFFS.end();
}

// Function WriteFile
uint8_t WriteFile(const char file_name[], const String file_data) {
    uint8_t errors = 0;
    if (SPIFFS.begin()) {
        Serial.printf_P("[%s] SPIFFS filesystem mounted ...\n\r", __func__);
    } else {
        Serial.printf_P("[%s] Error mounting the SPIFFS file system!\n\r", __func__);
        // Mount error!
    }
    // Save file data into the filesystem
    File file = SPIFFS.open("file_name", "w");
    if (!file) {
        Serial.printf_P("[%s] Error opening file for writing!", __func__);
        // File opening error!
    }
    uint16 bytes_written = file.print(file_data);
    if (bytes_written > 0) {
        Serial.printf_P("[%s] File write successful (%d bytes saved) ...\n\r", __func__, bytes_written);
    } else {
        Serial.printf_P("[%s] File writing failed!", __func__);
    }
    SPIFFS.end();
}
