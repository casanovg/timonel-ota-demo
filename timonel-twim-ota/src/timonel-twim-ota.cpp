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
    ClrScr();
    Serial.printf_P("\n\r");
    Serial.printf_P("......................................\n\r");
    Serial.printf_P(".          TIMONEL-TWIM-OTA          .\n\r");
    Serial.printf_P("......................................\n\r");

    // List all filesystem files
    //Format();
    ListFiles();
    // if (Exists("/fw-latest.hex") == false) {
    // Read the running ATtiny85 firmware version from the filesystem
    String fw_onboard_ver = ReadFile("/fw-onboard.md");
    Serial.printf_P("ATtiny85 firmware onboard (TBC): %s\n\r", fw_onboard_ver.c_str());
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
    String url = "/casanovg/timonel-ota-demo/master/fw-attiny85/fw-latest.md";
    String fw_latest_ver = GetHttpDocument(url, terminator, host, port, fingerprint);
    //Serial.printf_P(".......................................................\n\r");
    Serial.printf_P("Latest firmware version available for ATtiny85: %s\n\r", fw_latest_ver.c_str());
    //Serial.printf_P(".......................................................\n\r");
    //There is a pending ATtiny85 firmware update file in the filesystem ...

    if (fw_onboard_ver != fw_latest_ver) {
        //if (true) {
        // Update needed: get the latest firmware available for the ATtiny85 from internet
        terminator = '\0';
        url = "/casanovg/timonel-ota-demo/master/fw-attiny85/firmware-" + fw_latest_ver + ".hex";
        String fw_latest_dat = GetHttpDocument(url, terminator, host, port, fingerprint);

        // Save the latest firmware into the filesystem
        WriteFile("/fw-latest.hex", fw_latest_dat);

        // Read the saved firmware file
        fw_latest_dat = ReadFile("/fw-latest.hex");

        // Parse the new firmware file
        HexParser *ihex = new HexParser();
        uint16_t payload_size = ihex->GetIHexSize(fw_latest_dat);
        uint8_t payload[payload_size];
        // uint8_t line_count = 0;
        // uint8_t nl = 0;
        //Serial.printf_P("\n\r================================================\n\r");
        Serial.printf_P("Firwmare dump:\n\r");
        if (ihex->ParseIHexFormat(fw_latest_dat, payload)) {
            Serial.printf_P("\n\rPayload checksum error!\n\n\r");
            //Serial.printf_P("\r::::::::::::::::::::::::::::::::::::::::::::::::\n\r");
        } else {
            // Serial.printf_P("%02d) ", line_count++);
            // for (uint16_t q = 0; q < payload_size; q++) {
            //     Serial.printf_P(".%02X", payload[q]);
            //     if (nl++ == 15) {
            //         Serial.printf_P("\n\r");
            //         Serial.printf_P("%02d) ", line_count++);
            //         nl = 0;
            //     }
            // }
        }
        delete ihex;
        //Serial.printf_P("\n\r================================================\n\n\r");
        // Detect the ATtiny85 application TWI (I2C) address
        TwiBus *twi_bus = new TwiBus(SDA, SCL);
        uint8_t twi_address = twi_bus->ScanBus();
        delete twi_bus;
        NbMicro *micro = nullptr;
        Timonel *timonel = nullptr;
        Serial.printf_P("\n\rTWI address detected: %d", twi_address);
        // If the address is in the 36-63 range, the ATtiny85 is running a user application ...
        if (twi_address >= HIG_TML_ADDR) {
            Serial.printf_P(" device running an user application\n\r");
            micro = new NbMicro(twi_address, SDA, SCL);
            micro->TwiCmdXmit(RESETMCU, ACKRESET);
            delay(1000);
            delete micro;
            Serial.printf_P("\n\rIs the application stopped? Resetting this device ...\n\r");
            delay(2000);
            ESP.restart();  // ***
        }
        // If the address is in the 08-35 range, the ATtiny85 is running Timonel bootloader ...
        if ((twi_address < HIG_TML_ADDR) && (twi_address != 0)) {
            Serial.printf_P(" device running Timonel bootloader\n\r");
            timonel = new Timonel(twi_address, SDA, SCL);
            timonel->GetStatus();
            // Delete current application
            timonel->DeleteApplication();
            delay(500);
            timonel->GetStatus();
            delay(125);
            timonel->GetStatus();
            delay(125);
            // Upload the new user application to the ATtiny85
            USE_SERIAL.printf_P("\n\rBootloader Cmd >>> Firmware upload to flash memory, \x1b[5mPLEASE WAIT\x1b[0m ...");
            uint8_t errors = timonel->UploadApplication(payload, payload_size);
            USE_SERIAL.printf_P("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
            if (errors == 0) {
                USE_SERIAL.printf_P("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b successful!                          \n\r");
                delay(500);
                // Run the new user application
                timonel->RunApplication();
                Serial.printf_P("\n\rThe user application should be running by now ... \n\r");
                // Save the latest firmware version info to the filesystem
                WriteFile("/fw-onboard.md", fw_latest_ver);
                // Move the firmware name from "latest" to "onboard"
                Rename("/fw-latest.hex", "/fw-onboard.hex");
            } else {
                USE_SERIAL.printf_P("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b error! (%d)                          \n\r", errors);
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
        //Serial.printf_P("[%s] SPIFFS filesystem mounted ...\n\r", __func__);
    } else {
        Serial.printf_P("[%s] Error mounting the SPIFFS file system\n\r", __func__);
        // Mount error!
    }
    File file = SPIFFS.open(file_name, "r");
    //Serial.printf_P("[%s] Reading \"%s\" file\n\r", __func__, file_name);
    if (file && file.size()) {
        while (file.available()) {
            file_data += char(file.read());
        }
        file.close();
        //Serial.printf_P("%s\n\r", file_data.c_str());
    } else {
        Serial.printf_P("[%s] Warning: File empty or unavailable!\n\r", __func__);
    }
    return file_data;
    SPIFFS.end();
}

// Function WriteFile
uint8_t WriteFile(const char file_name[], const String file_data) {
    uint8_t errors = 0;
    if (SPIFFS.begin()) {
        //Serial.printf_P("[%s] SPIFFS filesystem mounted ...\n\r", __func__);
    } else {
        Serial.printf_P("[%s] Error mounting the SPIFFS file system!\n\r", __func__);
        errors += 1;
        // Mount error!
    }
    // Save file data into the filesystem
    File file = SPIFFS.open(file_name, "w");
    if (!file) {
        Serial.printf_P("[%s] Error opening file for writing!", __func__);
        errors += 2;
        // File opening error!
    }
    //Serial.printf_P("[%s] Writing \"%s\" file ...\n\r", __func__, file_name);
    uint16 bytes_written = file.print(file_data);
    if (bytes_written > 0) {
        //Serial.printf_P("[%s] File write successful (%d bytes saved) ...\n\r", __func__, bytes_written);
    } else {
        Serial.printf_P("[%s] File writing failed!", __func__);
        errors += 3;
        // File writing error!
    }
    SPIFFS.end();
    return errors;
}

// Function ListFiles
uint8_t ListFiles(void) {
    uint8_t errors = 0;
    if (SPIFFS.begin()) {
        //Serial.printf_P("[%s] SPIFFS filesystem mounted ...\n\r", __func__);
    } else {
        Serial.printf_P("[%s] Error mounting the SPIFFS file system!\n\r", __func__);
        errors += 1;
        // Mount error!
    }
    //Serial.printf_P("[%s] Listing all filesystem files ...\n\r", __func__);
    Dir dir = SPIFFS.openDir("");
    while (dir.next()) {
        Serial.printf("|-- %s - %d bytes\n\r", dir.fileName().c_str(), (int)dir.fileSize());
    }
    SPIFFS.end();
    return errors;
}

// Function Format
uint8_t Format(void) {
    uint8_t errors = 0;
    if (SPIFFS.begin()) {
        //Serial.printf_P("[%s] SPIFFS filesystem mounted ...\n\r", __func__);
    } else {
        Serial.printf_P("[%s] Error mounting the SPIFFS file system!\n\r", __func__);
        errors += 1;
        // Mount error!
    }
    Serial.printf_P("[%s] Formatting the SPIFFS filesystem ...\n\r", __func__);
    errors += SPIFFS.format();
    if (errors) {
        Serial.printf_P("[%s] Error: Unable to format the filesystem!\n\r", __func__);
    }
    SPIFFS.end();
    return errors;
}

// Function Rename (If destination exists, overwrites it)
uint8_t Rename(const char source_file_name[], const char destination_file_name[]) {
    uint8_t errors = 0;
    if (SPIFFS.begin()) {
        //Serial.printf_P("[%s] SPIFFS filesystem mounted ...\n\r", __func__);
    } else {
        Serial.printf_P("[%s] Error mounting the SPIFFS file system!\n\r", __func__);
        errors += 1;
        // Mount error!
    }
    SPIFFS.remove(destination_file_name);  // This allows overwriting
    errors += SPIFFS.rename(source_file_name, destination_file_name);
    if (errors) {
        //Serial.printf_P("[%s] File renamed successfully ...\n\r", __func__);
    } else {
        Serial.printf_P("[%s] File renaming failed!\n\r", __func__);
    }
    SPIFFS.end();
    return errors;
}

// Function Exists (If destination exists, overwrites it)
bool Exists(const char file_name[]) {
    if (SPIFFS.begin()) {
        //Serial.printf_P("[%s] SPIFFS filesystem mounted ...\n\r", __func__);
    } else {
        Serial.printf_P("[%s] Error mounting the SPIFFS file system!\n\r", __func__);
        // Mount error!
    }
    return SPIFFS.exists(file_name);
    SPIFFS.end();
}
