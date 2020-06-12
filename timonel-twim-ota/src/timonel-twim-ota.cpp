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
  2020-06-06 Gustavo Casanova
  ----------------------------------------------------------------------------
*/

#include "timonel-twim-ota.h"

#define FW_WEB_URL "/casanovg/timonel-ota-demo/master/fw-attiny85"
#define FW_ONBOARD_VER "/fw-onboard.md"
#define FW_ONBOARD_LOC "/fw-onboard.hex"
#define FW_LATEST_LOC "/fw-latest.hex"
#define FW_LATEST_VER "/casanovg/timonel-ota-demo/master/fw-attiny85/fw-latest.md"
#define UPDATE_TRIES "/update_tries.md"

// ATtiny85 MAX update attempts number
const uint8_t MAX_UPDATE_TRIES = 3;
// Use Firefox browser to get the web site certificate SHA1 fingerprint (case-insensitive)
const char FINGERPRINT[] PROGMEM = "70 94 de dd e6 c4 69 48 3a 92 70 a1 48 56 78 2d 18 64 e0 b7";
// File name constants
// const char FW_ONBOARD_VER[] PROGMEM = "/fw-onboard.md";
// const char FW_ONBOARD_LOC[] PROGMEM = "/fw-onboard.hex";
// const char FW_LATEST_LOC[] PROGMEM = "/fw-latest.hex";
// const char FW_LATEST_VER[] PROGMEM = FW_WEB_URL "/fw-latest.md";
// const char UPDATE_TRIES[] PROGMEM = "/update_tries.md";

/*  ___________________
   |                   | 
   |    Setup block    |
   |___________________|
*/
void setup(void) {
    Serial.begin(115200);
    ClrScr();
    delay(3000);
    Serial.printf_P("\n\r");
    Serial.printf_P("..........................................\n\r");
    Serial.printf_P(".          TIMONEL-TWIM-OTA 1.0          .\n\r");
    Serial.printf_P("..........................................\n\r");

    CheckForUpdates();
}

/*   ___________________
    |                   | 
    |     Main loop     |
    |___________________|
*/
void loop(void) {
    uint16_t seconds = 60;
    Serial.printf_P("\n\rI2C master main loop started ...\n\n\r");

    while (seconds) {
        Serial.printf_P(".%d ", seconds);
        delay(1000);
        seconds--;
    }
    Serial.printf_P("\n\n\rI2C master restarting to check for ATtiny85 firmware updates, bye!\n\n\r");
    delay(3000);
    ESP.restart();
}

// #############################################################################################
// #############################################################################################
// #############################################################################################
// #############################################################################################
// #############################################################################################

// Function CheckForUpdates
void CheckForUpdates(void) {
    uint8_t update_tries = 0;
    String fw_latest_dat = "";
    const char ssid[] = SSID;
    const char password[] = PASS;
    const char host[] = "raw.githubusercontent.com";
    const int port = 443;

    // " <<< Wifi connection "
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.printf_P(".");
    }
    Serial.printf_P("\n\r");
    Serial.printf_P("[%s] WiFi connected! IP address: %s\n\r", __func__, WiFi.localIP().toString().c_str());
    // " WiFi connection >>> "    

    ListFiles();

    // Reading update attempts recording file
    if (ReadFile(UPDATE_TRIES).charAt(0) != '\0') {
        String retry_str = ReadFile(UPDATE_TRIES);
        update_tries = (uint8_t)retry_str.toInt();
        Serial.printf_P("[%s] Retry file str: %d times (String: %s) ...\n\r", __func__, update_tries, retry_str.c_str());
        update_tries = (uint8_t)ReadFile(UPDATE_TRIES).charAt(0);
    }
    if (update_tries < MAX_UPDATE_TRIES) {
        // ..................................................
        // (1)> Update retries NOT exceeded, starting update routine
        // ..................................................
        Serial.printf_P("[%s] Update attempts: %d of %d, starting the update routine ...\n\r", __func__, update_tries, MAX_UPDATE_TRIES);
        if (Exists(FW_LATEST_LOC)) {
            // ..................................................
            // (?2)> New firmware file already present in FS, probably due to a failed update
            // ..................................................
            Serial.printf_P("[%s] New firmware file already present in FS, probably due to a failed update ...\n\r", __func__);
            fw_latest_dat = ReadFile(FW_LATEST_LOC);
        } else {
            // ..................................................
            // (:2)> New firmware file NOT present in FS, accessing the internet to check for updates
            // ..................................................
            Serial.printf_P("[%s] New firmware file NOT present in FS, accessing the internet to check for updates ...\n\r", __func__);
            // Check local firmware version, currently running on the slave device
            String fw_onboard_ver = ReadFile(FW_ONBOARD_VER);
            Serial.printf_P("[%s] ATtiny85 current firmware onboard: %s\n\r", __func__, fw_onboard_ver.c_str());
            
            // " <<< Wifi connection >>> "

            // Check the latest firmware version available for the slave device through WiFi
            char terminator = '\n';
            String fw_latest_ver = GetHttpDocument(FW_LATEST_VER, terminator, host, port, FINGERPRINT);
            //Serial.printf_P(".......................................................\n\r");
            Serial.printf_P("[%s] Latest firmware version available for ATtiny85: %s\n\r", __func__, fw_latest_ver.c_str());
            if (fw_onboard_ver = fw_latest_ver) {
                // ..................................................
                // (?3)> Update NOT needed, run the application and exit this update routine
                // ..................................................
                // ##### (S) #####
                StartApplication();
            } else {
                // ..................................................
                // (:3)> There is a new firmware version available, download it through WiFi
                // ..................................................
                terminator = '\0';
                String url = "/casanovg/timonel-ota-demo/master/fw-attiny85/firmware-" + fw_latest_ver + ".hex";
                String fw_latest_dat = GetHttpDocument(url, terminator, host, port, FINGERPRINT);
            }  // (/3)>
            WiFi.disconnect();
        }      // (/2)>
        uint16_t payload_size = GetIHexSize(fw_latest_dat);
        uint8_t payload[payload_size];
        if (ParseIHexFormat(fw_latest_dat, payload)) {
            // ..................................................
            // (?4)> There were errors parsing the downloaded Intel Hex firmware, resetting master
            // ..................................................
            Serial.printf_P("Intel Hex file checksum error!\n\r");
            // ##### (R) #####
            RetryRestart(UPDATE_TRIES, update_tries);
        } else {
            // ..................................................
            // (:4)> Intel Hex firmware parsed correctly, binary payload ready to flash device
            // ..................................................
            WriteFile(FW_LATEST_LOC, fw_latest_dat);  // Saving the new firmware file to FS
            // uint8_t line_count = 0;
            // uint8_t char_count = 0;
            // Serial.printf_P("%02d) ", line_count++);
            // for (uint16_t i = 0; i < payload_size; i++) {
            //     Serial.printf_P(".%02X", payload[i]);
            //     if (char_count++ == 15) {
            //         Serial.printf_P("\n\r");
            //         Serial.printf_P("%02d) ", line_count++);
            //         char_count = 0;
            //     }
            // }
        }  // (/4)>
        uint8_t twi_address = 0;
        TwiBus twi_bus(SDA, SCL);
        twi_address = twi_bus.ScanBus();
        NbMicro *p_micro = nullptr;
        Timonel *p_timonel = nullptr;
        Serial.printf_P("TWI address detected: %d", twi_address);
        if (twi_address < LOW_TML_ADDR) {
            // ..................................................
            // (?5)> TWI device NOT detected in the bus, resetting master
            // ..................................................
            Serial.printf_P(", invalid address or device not present!\n\r");
            // ##### (R) #####
            RetryRestart(UPDATE_TRIES, update_tries);
        } else {
            // ..................................................
            // (:5)> TWI device detected in the bus, creating an object for its address type
            // ..................................................
            if (twi_address <= HIG_TML_ADDR) {
                // ..................................................
                // (?6)> The address is in the 08-35 range, device running Timonel bootloader ...
                // ..................................................
                Serial.printf_P(", device running Timonel bootloader ...\n\r");
                // Initialize Timonel object
                p_timonel = new Timonel(twi_address, SDA, SCL);
                p_timonel->GetStatus();
                delay(125);
                // Delete ATtiny85 onboard application
                p_timonel->DeleteApplication();
                delay(500);
                p_timonel->GetStatus();
                delay(125);
                // Upload the new user application to the ATtiny85
                USE_SERIAL.printf_P("[%s] Timonel bootloader uploading firmware to flash memory, \x1b[5mPLEASE WAIT\x1b[0m ...", __func__);
                uint8_t errors = p_timonel->UploadApplication(payload, payload_size);
                USE_SERIAL.printf_P("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
                if (errors == 0) {
                    // ..................................................
                    // (?7)> Application firmware loaded on the device
                    // ..................................................
                    USE_SERIAL.printf_P("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b successful!                          \n\r");
                    delay(500);
                    // Run the new user application
                    p_timonel->RunApplication();
                    Serial.printf_P("[%s] The user application should be running by now ... \n\r, __func__");
                    delete p_timonel;
                    // Move the firmware name from "latest" to "onboard"
                    Rename(FW_LATEST_LOC, FW_ONBOARD_LOC);
                } else {
                    // ..................................................
                    // (:7)> There were errors uploading the new firmware
                    // ..................................................
                    USE_SERIAL.printf_P("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b error! (%d)                          \n\r", errors);
                    // ##### (R) #####
                    RetryRestart(UPDATE_TRIES, update_tries);
                }  // (/7)>
                // Remove Timonel object
                //delete p_timonel;
            } else {
                // ..................................................
                // (:6)> The address is above bootloader range, running an user application ...
                // ..................................................
                Serial.printf_P(", device running an user application ...\n\r");
                p_micro = new NbMicro(twi_address, SDA, SCL);
                p_micro->TwiCmdXmit(RESETMCU, ACKRESET);
                delay(250);
                delete p_micro;
                Serial.printf_P("[%s] The user application should be stopped by now, restarting master to begin the update ...\n\r", __func__);
                delay(2000);
                ESP.restart();
            }  // (/6)>

        }  // (/5)>
    } else {
        // ..................................................
        // (1)> Update retries exceeded, running the application and exiting this update routine
        // ..................................................
        // ##### (S) #####
        Serial.printf_P("[%s] Retry %d of %d attempted, formating FS, running the application and exiting ...\n\r", __func__, update_tries, MAX_UPDATE_TRIES);
        Format();
        StartApplication();
    }  // (/1)>
}

// Function StartApplication
void StartApplication(void) {
    uint8_t twi_address = 0;
    TwiBus twi_bus(SDA, SCL);
    twi_address = twi_bus.ScanBus();
    Serial.printf_P("[%s] Trying to start the user application ...\n\r", __func__);
    if ((twi_address >= LOW_TML_ADDR) && (twi_address <= HIG_TML_ADDR)) {
        Timonel timonel(twi_address, SDA, SCL);
        timonel.RunApplication();
    }
}

// Function RetryRestart
void RetryRestart(const char file_name[], uint8_t update_tries) {
    update_tries++;
    Serial.printf_P("[%s] Saving [%d] to retry counter and resetting ...\n\r", __func__, update_tries);
    WriteFile(file_name, (String)update_tries);
    //ESP.restart();
}

#if POPEYE

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
    Serial.printf_P("..........................................\n\r");
    Serial.printf_P(".          TIMONEL-TWIM-OTA 1.0          .\n\r");
    Serial.printf_P("..........................................\n\r");

    // List all filesystem files
    //Format();
    ListFiles();
    //if (Exists("/fw-latest.hex") == false) {
    // Read the running ATtiny85 firmware version from the filesystem
    String fw_onboard_ver = ReadFile("/fw-onboard.md");
    Serial.printf_P("ATtiny85 firmware onboard (TBC): %s\n\r", fw_onboard_ver.c_str());
    // Open WiFi connection to access the internet
    //Serial.printf_P("Connecting to WiFi: %s\n\r", ssid);
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
        String fw_file_rem = GetHttpDocument(url, terminator, host, port, fingerprint);

        // Save the latest firmware into the filesystem
        WriteFile("/fw-latest.hex", fw_file_rem);

        // Read the saved firmware file
        String fw_file_loc = ReadFile("/fw-latest.hex");

        // Parse the new firmware file
        HexParser *ihex = new HexParser();
        uint16_t payload_size = ihex->GetIHexSize(fw_file_loc);
        uint8_t payload[payload_size];
        // uint8_t line_count = 0;
        // uint8_t nl = 0;
        //Serial.printf_P("\n\r================================================\n\r");
        //Serial.printf_P("Firwmare dump:\n\r");
        if (ihex->ParseIHexFormat(fw_file_loc, payload)) {
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
            Serial.printf_P("\n\rIs the application stopped?\n\r");
            delay(3000);
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
                USE_SERIAL.printf_P("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b successful!                          \n\r");
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
}

uint16_t seconds = 60;

/*   ___________________
    |                   | 
    |     Main loop     |
    |___________________|
*/
void loop(void) {
    Serial.printf_P("\n\rI2C master main loop started ...\n\n\r");

    while (seconds) {
        Serial.printf_P(".%d ", seconds);
        delay(1000);
        seconds--;
    }
    Serial.printf_P("\n\n\rI2C master restarting to check for ATtiny85 firmware updates, bye!\n\n\r");
    delay(3000);
    ESP.restart();
}

#endif  // POPEYE

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
        Serial.printf_P("[%s] Warning: File \"%s\" empty or unavailable!\n\r", __func__, file_name);
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
        Serial.printf_P("[%s] Error opening file for writing!\n\r", __func__);
        errors += 2;
        // File opening error!
    }
    //Serial.printf_P("[%s] Writing \"%s\" file ...\n\r", __func__, file_name);
    uint16 bytes_written = file.print(file_data);
    if (bytes_written > 0) {
        //Serial.printf_P("[%s] File write successful (%d bytes saved) ...\n\r", __func__, bytes_written);
    } else {
        Serial.printf_P("[%s] \"%s\" file writing failed!\n\r", __func__, file_name);
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
        Serial.printf_P("[%s] File renaming failed! (%s to %s)\n\r", __func__, source_file_name, destination_file_name);
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

// Function ParseIHexFormat
bool ParseIHexFormat(String serialized_file, uint8_t *payload) {
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
uint16_t GetIHexSize(String serialized_file) {
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
    //Serial.printf_P("\r\n[%s] File length: %d\n\r", __func__, file_length);
    //Serial.printf_P("[%s] Payload size: %d\n\r", __func__, data_size);
    return data_size;
}
