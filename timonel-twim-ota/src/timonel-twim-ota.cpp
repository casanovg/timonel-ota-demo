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

const char* ssid = SSID;
const char* password = PASS;

const char* host = "raw.githubusercontent.com";
const int https_port = 443;

// Use Firefox browser to get the web site certificate SHA1 fingerprint (case-insensitive)
const char fingerprint[] PROGMEM = "70 94 de dd e6 c4 69 48 3a 92 70 a1 48 56 78 2d 18 64 e0 b7";

// Prototypes
void ClrScr(void);

// Setup block
void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    // Use WiFiClientSecure class to create TLS connection
    WiFiClientSecure client;
    Serial.print("Connecting to web site: ");
    Serial.println(host);

    Serial.printf("Using fingerprint: %s\n\r", fingerprint);
    client.setFingerprint(fingerprint);

    if (!client.connect(host, https_port)) {
        Serial.println("Connection failed");
        return;
    }

    // Read the latest firmware available to write to the slave device
    String url = "/casanovg/timonel-ota-demo/master/fw-attiny85/latest-version.md";
    Serial.print("URL Request: ");
    Serial.println(url);

    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "User-Agent: TimonelTwiMOtaESP8266\r\n" +
                 "Connection: close\r\n\r\n");

    Serial.println("Request sent");
    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
            Serial.println("Headers received");
            break;
        }
    }

    String http_string = client.readStringUntil('\n');

    Serial.println("================================================");
    Serial.println("Latest firwmare version for ATTiny85: " + http_string);
    Serial.println("================================================");
    //Serial.println("Closing connection");

    Serial.printf("Using fingerprint: %s\n", fingerprint);
    client.setFingerprint(fingerprint);

    if (!client.connect(host, https_port)) {
        Serial.println("Connection failed");
        return;
    }

    Serial.println("\r................................................");

    url = "/casanovg/timonel-ota-demo/master/fw-attiny85/firmware-" + http_string + ".hex";
    Serial.print("URL Request: ");
    Serial.println(url);

    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "User-Agent: TimonelTwiMOtaESP8266\r\n" +
                 "Connection: close\r\n\r\n");

    Serial.println("Request sent");
    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
            Serial.println("Headers received");
            break;
        }
    }

    http_string = client.readStringUntil('\0');

	HexParser ihex;

    uint16_t payload_size = ihex.GetIHexSize(http_string);
    payload_size += 10;
    uint8_t payload[payload_size];
    uint8_t line_count = 0;
	//uint8_t *p_payload = &payload;

    Serial.println("================================================");
    Serial.println("Firwmare dump:");

    ihex.GetIHexPayload(http_string, payload);
    
    Serial.println("::::::::::::::::::::::::::::::::::::::::::::::::");

	uint8_t nl = 0;

    Serial.printf("%02d) ", line_count++);
	for (uint16_t q = 0; q <= payload_size; q++) {
		Serial.printf(".%02X", payload[q]);
		if (nl++ == 15) {
			Serial.print("\r\n");
            Serial.printf("%02d) ", line_count++);
			nl = 0;
		}
	}

    Serial.println("\r================================================");

    // while (client.connected()) {
    //     String line = client.readStringUntil('\n');
    //     if (line != "") {
    //         Serial.println(line);
    //         line = client.readStringUntil('\n');
    //     }
    //      else {
    //         client.flush(0);
    //         break;
    //     }
    // }

    //***************************************************
    //     Serial.begin(115200);

    //     ClrScr();
    //     Serial.println("\n\n\rStarting I2C Timonel device OTA update demo ...z");
    //     WiFi.mode(WIFI_STA);
    //     WiFi.begin(ssid, password);
    //     while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    //         Serial.println("Connection Failed! Rebooting...");
    //         delay(5000);
    //         ESP.restart();
    //     }

    //     Serial.println("\n\rWiFi connection established!\n\r");

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