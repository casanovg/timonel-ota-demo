//
// -----
//

#ifndef _TIMONEL_TWIM_OTA_H_
#define _TIMONEL_TWIM_OTA_H_

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <NbMicro.h>
#include <TimonelTwiM.h>
#include <TwiBus.h>
#include <WiFiClientSecure.h>
#include <ihex-parser.h>
#include <nb-twi-cmd.h>

#ifndef SSID
#define SSID "Nicebots.com"
#define PASS "R2-D2 C-3P0"
// #define SSID "HTA Buen Ayre"
// #define PASS "BB-8 C-3P0"
#endif  // SSID

#define SDA 2  // I2C SDA pin - ESP8266 2 - ESP32 21
#define SCL 0  // I2C SCL pin - ESP8266 0 - ESP32 22

// Prototypes
void ClrScr(void);
void CheckForUpdates(void);
uint8_t Format(void);
uint8_t ListFiles(void);
bool Exists(const char file_name[]);
String ReadFile(const char file_name[]);
uint8_t WriteFile(const char file_name[], const String file_data);
uint8_t Rename(const char source_file_name[], const char destination_file_name[]);
String GetHttpDocument(String url, char terminator, const char host[], const int port, const char fingerprint[]);

#endif  // _TIMONEL_TWIM_OTA_H_
