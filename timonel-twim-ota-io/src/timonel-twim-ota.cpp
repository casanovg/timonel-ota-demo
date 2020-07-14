/*
  timonel-twim-ota.cpp
  =====================
  Timonel OTA Demo v2.0 HP
  ----------------------------------------------------------------------------
  This demo, running on an ESP8266 I2C master, updates an ATtiny85 slave's
  firmware of an over-the-air:
  1-The ESP9266 firmware runs Timonel bootloader master amd has an ATttiny85
    application compiled inside.
  2-After an OTA update, the ESP8266 Timonel master resets the ATtiny85 to
    start Timonel bootloader on it.
  3-The ESP8266 uploads the new firmware into the ATtiny85 and, if it is
    sucessful, resets it to exit Timonel and run the new firmware.
  4-The ESP8266 exits Timonel bootloader master and enters its normal
    operation loop, where it also makes frequent checks for new OTA updates.
  ----------------------------------------------------------------------------
  2020-06-13 Gustavo Casanova
  ----------------------------------------------------------------------------
*/

#include "timonel-twim-ota.h"

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
    Serial.printf_P("...............................................\n\r");
    Serial.printf_P(".          TIMONEL-TWIM-OTA DEMO 2.0          .\n\r");
    Serial.printf_P("...............................................\n\r");

    uint8_t slave_address = 0;
    TwiBus *p_twi_bus = new TwiBus(SDA, SCL);
    // Keep waiting until a slave device is detected    
    USE_SERIAL.printf_P("\n\rWaiting until a TWI slave device is detected on the bus   ");
    while (slave_address == 0) {
        slave_address = p_twi_bus->ScanBus();
        RotaryDelay();
    }
    USE_SERIAL.printf_P("\n\n\r");
    delete p_twi_bus;

    String new_ver = CheckFwUpdate(SSID, PASS, WEB_HOST, WEB_PORT, FINGERPRINT,
                                   ReadFile(FW_ONBOARD_VER),
                                   FW_LATEST_WEB);
    if (new_ver != "") {
        Serial.printf_P("New firmware version found on the intenet: %s, updating ATtiny85 ...\n\r", new_ver.c_str());
        UpdateFirmware(new_ver);
    } else {
        Serial.printf_P("No new firmware version available ...\n\r");
        StartApplication();
    }
}

/*  ___________________
   |                   | 
   |     Main loop     |
   |___________________|
*/
void loop(void) {
    uint16_t seconds = 60;
    Serial.printf_P("\n\rI2C master main loop started");
    Serial.printf_P("\n\r============================\n\n\r");

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

/*  _______________________
   |                       | 
   |     CheckFwUpdate     |
   |_______________________|
*/
String CheckFwUpdate(const char ssid[],
                     const char password[],
                     const char host[],
                     const int port,
                     const char fingerprint[],
                     const String current_version,
                     const String latest_version) {
    String fw_latest_ver = "";
    ListFiles();
    // ..................................................
    // Accessing the internet to check for updates
    // ..................................................
    // Check the latest firmware version available for the slave device through WiFi
    Serial.printf_P("[%s] Connecting to the internet to check for updates ...\n\r", __func__);
    char terminator = '\n';
    String fw_latest_web = GetHttpDocument(ssid, password, host, port, fingerprint, latest_version, terminator);
    if (current_version == fw_latest_web) {
        // Update NOT needed
        Serial.printf_P("[%s] ===>> Current onboard firmware [%s] is up to date! <<===\n\r", __func__, current_version.c_str());
    } else {
        // There is a new firmware version available
        Serial.printf_P("[%s] Onboard firmware version: [%s], a web update is available: [%s] ...\n\r", __func__, current_version.c_str(), fw_latest_web.c_str());
        fw_latest_ver = fw_latest_web;
    }
    return fw_latest_ver;
}

/*  ________________________
   |                        | 
   |     UpdateFirmware     |
   |________________________|
*/
void UpdateFirmware(String new_version) {
    const char ssid[] = SSID;
    const char password[] = PASS;
    const char host[] = "raw.githubusercontent.com";
    const int port = 443;
    uint8_t update_tries = 0;
    String fw_latest_dat = "";
    String fw_latest_ver = "";
    String fw_onboard_ver = "";
    ListFiles();
    // Reading update attempts recording file
    if (ReadFile(UPDATE_TRIES).charAt(0) != '\0') {
        update_tries = atoi(ReadFile(UPDATE_TRIES).c_str());
    }
    if (update_tries < MAX_UPDATE_TRIES) {
        // ..................................................
        // Update retries NOT exceeded, starting update routine
        // ..................................................
        Serial.printf_P("[%s] Update attempts [%d of %d], starting the update routine ...\n\r", __func__, update_tries + 1, MAX_UPDATE_TRIES);
        if (Exists(FW_LATEST_LOC)) {
            // ..................................................
            // New firmware file already present in FS, probably due to a failed update
            // ..................................................
            Serial.printf_P("[%s] New firmware file already present in FS, web download not necessary ...\n\r", __func__);
            fw_latest_dat = ReadFile(FW_LATEST_LOC);
            fw_latest_ver = ReadFile(FW_LATEST_VER);
        } else {
            // ..................................................
            // New firmware file NOT present in FS, accessing the internet to check for updates
            // ..................................................
            Serial.printf_P("[%s] No new firmware file in FS, accessing internet to check for updates ...\n\r", __func__);
            // ..................................................
            // There is a new firmware version available, download it through WiFi
            // ..................................................
            Serial.printf_P("[%s] Getting new firmware file version: [%s] ...\n\r", __func__, new_version.c_str());
            char terminator = '\0';
            String url = FW_WEB_URL "/firmware-" + new_version + ".hex";
            fw_latest_dat = GetHttpDocument(ssid, password, host, port, FINGERPRINT, url, terminator);
            WriteFile(FW_LATEST_LOC, fw_latest_dat);  // Saving the new firmware file to FS
            WriteFile(FW_LATEST_VER, new_version);    // saving the new firmware version to FS
        }
        uint16_t payload_size = GetIHexSize(fw_latest_dat);
        uint8_t payload[payload_size];
        if (ParseIHexFormat(fw_latest_dat, payload)) {
            // ..................................................
            // There were errors parsing the downloaded Intel Hex firmware, resetting master
            // ..................................................
            Serial.printf_P("Intel Hex file checksum error!\n\r");
            RetryRestart(UPDATE_TRIES, update_tries);
        }
        // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        FlashTwiDevice(payload, payload_size, update_tries);  // Flash device >>>
        // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    } else {
        // ..................................................
        // Update retries exceeded, running the application and exiting this update routine
        // ..................................................
        Serial.printf_P("[%s] Retry [%d of %d] failed, please power-cycle both, master and slave devices!\n\r", __func__, update_tries, MAX_UPDATE_TRIES);
        //Format();
        DeleteFile(FW_LATEST_LOC);
        DeleteFile(FW_LATEST_VER);
        WriteFile(UPDATE_TRIES, "0");
        StartApplication();
    }
}

/*  ________________________
   |                        | 
   |     FlashTwiDevice     |
   |________________________|
*/
void FlashTwiDevice(uint8_t payload[], uint16_t payload_size, uint8_t update_tries) {
    uint8_t twi_address = 0;
    TwiBus twi_bus(SDA, SCL);
    twi_address = twi_bus.ScanBus();
    NbMicro *p_micro = nullptr;
    Timonel *p_timonel = nullptr;
    Serial.printf_P("[%s] TWI address detected: %d", __func__, twi_address);
    if (twi_address < LOW_TML_ADDR) {
        // ..................................................
        // TWI device NOT detected in the bus, resetting master
        // ..................................................
        Serial.printf_P(", invalid address or device not present!\n\r");
        // ##### (R) #####
        RetryRestart(UPDATE_TRIES, update_tries);
    } else {
        // ..................................................
        // TWI device detected in the bus, creating an object for its address type
        // ..................................................
        if (twi_address <= HIG_TML_ADDR) {
            // ..................................................
            // The address is in the 08-35 range, device running Timonel bootloader ...
            // ..................................................
            Serial.printf_P(", device running Timonel bootloader ...\n\r");
            // Initialize Timonel object
            p_timonel = new Timonel(twi_address, SDA, SCL);
            p_timonel->GetStatus();
            delay(125);
            // Delete ATtiny85 onboard application
            p_timonel->DeleteApplication();
            delay(750);
            p_timonel->GetStatus();
            delay(125);
            // Upload the new user application to the ATtiny85
            USE_SERIAL.printf_P("[%s] Timonel bootloader uploading firmware to flash memory, \x1b[5mPLEASE WAIT\x1b[0m ...", __func__);
            uint8_t errors = p_timonel->UploadApplication(payload, payload_size);
            USE_SERIAL.printf_P("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
            if (errors == 0) {
                // ..................................................
                // Application firmware loaded on the device
                // ..................................................
                USE_SERIAL.printf_P("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b successful!                          \n\r");
                // Move the firmware version file from "latest" to "onboard"
                Rename(FW_LATEST_VER, FW_ONBOARD_VER);
                // Move the firmware data file from "latest" to "onboard"
                Rename(FW_LATEST_LOC, FW_ONBOARD_LOC);
                // Reset the retry counter file
                WriteFile(UPDATE_TRIES, "0");
                // Run the user application
                p_timonel->RunApplication();
            } else {
                // ..................................................
                // There were errors uploading the new firmware
                // ..................................................
                USE_SERIAL.printf_P("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b error! (%d)                         \n\r", errors);
                RetryRestart(UPDATE_TRIES, update_tries);
            }
            // Remove Timonel object
            delete p_timonel;
        } else {
            // ..................................................
            // The address is above bootloader range, running an user application ...
            // ..................................................
            Serial.printf_P(", device running an user application ...\n\r");
            p_micro = new NbMicro(twi_address, SDA, SCL);
            p_micro->TwiCmdXmit(RESETMCU, ACKRESET);
            delay(250);
            delete p_micro;
            Serial.printf_P("[%s] The user application should be stopped by now, restarting master to begin the update ...\n\r", __func__);
            delay(5000);
            ESP.restart();
        }
    }
}

/*  __________________________
   |                          | 
   |     StartApplication     |
   |__________________________|
*/
void StartApplication(void) {
    uint8_t twi_address = 0;
    TwiBus twi_bus(SDA, SCL);
    twi_address = twi_bus.ScanBus();
    if (twi_address < LOW_TML_ADDR) {
        Serial.printf_P("[%s] Invalid device TWI address detected (%d), probably a power-cycle would help ...\n\r", __func__, twi_address);
    } else {
        if (twi_address <= HIG_TML_ADDR) {
            Serial.printf_P("[%s] Timonel TWI address detected (%d), trying to start the user application ...\n\r", __func__, twi_address);
            Timonel timonel(twi_address, SDA, SCL);
            timonel.GetStatus();
            delay(125);
            timonel.GetStatus();
            delay(125);            
            timonel.RunApplication();
        } else {
            Serial.printf_P("[%s] Application TWI address detected (%d), letting it run ...\n\r", __func__, twi_address);
        }
    }
}

/*  ______________________
   |                      |
   |     RetryRestart     |
   |______________________|
*/
void RetryRestart(const char file_name[], uint8_t update_tries) {
    update_tries++;
    Serial.printf_P("[%s] Saving [%d] to retry counter and resetting ...\n\r", __func__, update_tries);
    WriteFile(file_name, (String)update_tries);
    delay(3000);
    ESP.restart();
}

/*  ______________________
   |                      |
   |     Clear screen     |
   |______________________|
*/
void ClrScr(void) {
    Serial.write(27);        // ESC command
    Serial.printf_P("[2J");  // clear screen command
    Serial.write(27);        // ESC command
    Serial.printf_P("[H");   // cursor to home command
}

/*  _________________________
   |                         |
   |     GetHttpDocument     |
   |_________________________|
*/
String GetHttpDocument(const char ssid[],
                       const char password[],
                       const char host[],
                       const int port,
                       const char fingerprint[],
                       String url,
                       char terminator) {
    // " <<< Wifi connection "
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.printf_P("[%s] Opening WiFi connection ", __func__);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.printf_P(".");
    }
    Serial.printf_P("\n\r");
    //Serial.printf_P("[%s] WiFi connected! IP address: %s\n\r", __func__, WiFi.localIP().toString().c_str());
    // " WiFi connection >>> "
    // Use WiFiClientSecure class to create TLS connection
    String http_string = "";
    WiFiClientSecure client;
    Serial.printf_P("[%s] Connecting to web site: %s\n\r", __func__, host);
    //Serial.printf_P(" with fingerprint: %s\n\r", fingerprint);
    client.setFingerprint(fingerprint);
    if (!client.connect(host, port)) {
        Serial.printf_P("[%s] HTTP connection failed!\n\r", __func__);
        // Connection error!
    }
    //Serial.printf_P("[%s] URL Request: %s\n\r", __func__, url.c_str());
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "User-Agent: TimonelTwiMOtaESP8266\r\n" +
                 "Connection: close\r\n\r\n");
    //Serial.printf_P("[%s] Request sent ...\n\r", __func__);
    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
            //Serial.printf_P("[%s] Headers received ...\n\r", __func__);
            break;
        }
    }
    http_string = client.readStringUntil(terminator);
    if (http_string != "") {
        Serial.printf_P("[%s] HTTP data received via WiFi ...\n\r", __func__);
    } else {
        Serial.printf_P("[%s] No HTTP data received via WiFi!\n\r", __func__);
    }
    WiFi.disconnect();
    //Serial.printf_P("[%s] WiFi disconnected!\n\r", __func__);
    return http_string;
}

/*  __________________
   |                  |
   |     ReadFile     |
   |__________________|
*/
String ReadFile(const char file_name[]) {
    String file_data = "";
    //if (SPIFFS.begin()) {
    if (LittleFS.begin()) {
        //Serial.printf_P("[%s] SPIFFS filesystem mounted ...\n\r", __func__);
    } else {
        Serial.printf_P("[%s] Error mounting the SPIFFS file system\n\r", __func__);
        // Mount error!
    }
    //File file = SPIFFS.open(file_name, "r");
    File file = LittleFS.open(file_name, "r");
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
    //SPIFFS.end();
    LittleFS.end();
}

/*  ___________________
   |                   |
   |     WriteFile     |
   |___________________|
*/
uint8_t WriteFile(const char file_name[], const String file_data) {
    uint8_t errors = 0;
    //if (SPIFFS.begin()) {
    if (LittleFS.begin()) {
        //Serial.printf_P("[%s] SPIFFS filesystem mounted ...\n\r", __func__);
    } else {
        Serial.printf_P("[%s] Error mounting the SPIFFS file system!\n\r", __func__);
        errors += 1;
        // Mount error!
    }
    // Save file data into the filesystem
    //File file = SPIFFS.open(file_name, "w");
    File file = LittleFS.open(file_name, "w");
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
    //SPIFFS.end();
    LittleFS.end();
    return errors;
}

/*  ____________________
   |                    |
   |     DeleteFile     |
   |____________________|
*/
uint8_t DeleteFile(const char file_name[]) {
    uint8_t errors = 0;
    //if (SPIFFS.begin()) {
    if (LittleFS.begin()) {
        //Serial.printf_P("[%s] SPIFFS filesystem mounted ...\n\r", __func__);
    } else {
        Serial.printf_P("[%s] Error mounting the SPIFFS file system!\n\r", __func__);
        errors += 1;
        // Mount error!
    }
    //errors += SPIFFS.remove(file_name);  // This allows overwriting
    errors += LittleFS.remove(file_name);  // This allows overwriting
    if (errors) {
        //Serial.printf_P("[%s] File deleted successfully ...\n\r", __func__);
    } else {
        Serial.printf_P("[%s] File \"%s\" deleting failed!\n\r", __func__, file_name);
    }
    //SPIFFS.end();
    LittleFS.end();
    return errors;
}

/*  ___________________
   |                   |
   |     ListFiles     |
   |___________________|
*/
uint8_t ListFiles(void) {
    uint8_t errors = 0;
    //if (SPIFFS.begin()) {
    if (LittleFS.begin()) {
        //Serial.printf_P("[%s] SPIFFS filesystem mounted ...\n\r", __func__);
    } else {
        Serial.printf_P("[%s] Error mounting the SPIFFS file system!\n\r", __func__);
        errors += 1;
        // Mount error!
    }
    //Serial.printf_P("[%s] Listing all filesystem files ...\n\r", __func__);
    //Dir dir = SPIFFS.openDir("");
    Dir dir = LittleFS.openDir("");
    while (dir.next()) {
        Serial.printf("|-- %s - %d bytes\n\r", dir.fileName().c_str(), (int)dir.fileSize());
    }
    //SPIFFS.end();
    LittleFS.end();
    return errors;
}

// Function Format
/*  ________________
   |                |
   |     Format     |
   |________________|
*/
uint8_t Format(void) {
    uint8_t errors = 0;
    //if (SPIFFS.begin()) {
    if (LittleFS.begin()) {
        //Serial.printf_P("[%s] SPIFFS filesystem mounted ...\n\r", __func__);
    } else {
        Serial.printf_P("[%s] Error mounting the SPIFFS file system!\n\r", __func__);
        errors += 1;
        // Mount error!
    }
    Serial.printf_P("[%s] Formatting the SPIFFS filesystem ...\n\r", __func__);
    //errors += SPIFFS.format();
    errors += LittleFS.format();
    if (errors) {
        Serial.printf_P("[%s] Error: Unable to format the filesystem!\n\r", __func__);
    }
    //SPIFFS.end();
    LittleFS.end();
    return errors;
}

/*  ________________
   |                |
   |     Rename     |
   |________________|
*/
// If destination exists, overwrites it
uint8_t Rename(const char source_file_name[], const char destination_file_name[]) {
    uint8_t errors = 0;
    //if (SPIFFS.begin()) {
    if (LittleFS.begin()) {
        //Serial.printf_P("[%s] SPIFFS filesystem mounted ...\n\r", __func__);
    } else {
        Serial.printf_P("[%s] Error mounting the SPIFFS file system!\n\r", __func__);
        errors += 1;
        // Mount error!
    }
    //SPIFFS.remove(destination_file_name);  // This allows overwriting
    LittleFS.remove(destination_file_name);  // This allows overwriting
    //errors += SPIFFS.rename(source_file_name, destination_file_name);
    errors += LittleFS.rename(source_file_name, destination_file_name);
    if (errors) {
        //Serial.printf_P("[%s] File renamed successfully ...\n\r", __func__);
    } else {
        Serial.printf_P("[%s] File renaming failed! (%s to %s)\n\r", __func__, source_file_name, destination_file_name);
    }
    //SPIFFS.end();
    LittleFS.end();
    return errors;
}

// Function Exists (If destination exists, overwrites it)
bool Exists(const char file_name[]) {
    //if (SPIFFS.begin()) {
    if (LittleFS.begin()) {
        //Serial.printf_P("[%s] SPIFFS filesystem mounted ...\n\r", __func__);
    } else {
        Serial.printf_P("[%s] Error mounting the SPIFFS file system!\n\r", __func__);
        // Mount error!
    }
    //return SPIFFS.exists(file_name);
    return LittleFS.exists(file_name);
    //SPIFFS.end();
    LittleFS.end();
}

/*  _________________________
   |                         |
   |     ParseIHexFormat     |
   |_________________________|
*/
bool ParseIHexFormat(String serialized_file, uint8_t *payload) {
    uint16_t file_len = serialized_file.length();
    uint16_t payload_ix = 0;
    //uint8_t line_count = 0;
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

/*  _____________________
   |                     |
   |     GetIHexSize     |
   |_____________________|
*/
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

/*  _____________________
   |                     |
   |     RotaryDelay     |
   |_____________________|
*/
void RotaryDelay(void) {
    USE_SERIAL.printf_P("\b\b| ");
    delay(125);
    USE_SERIAL.printf_P("\b\b/ ");
    delay(125);
    USE_SERIAL.printf_P("\b\b- ");
    delay(125);
    USE_SERIAL.printf_P("\b\b\\ ");
    delay(125);
}
