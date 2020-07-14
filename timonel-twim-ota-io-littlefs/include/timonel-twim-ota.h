/*
  timonel-twim-ota.h
  =====================
  Timonel OTA Demo v2.0 HP
  ----------------------------------------------------------------------------
  2020-06-13 Gustavo Casanova
  ----------------------------------------------------------------------------
*/

#ifndef _TIMONEL_TWIM_OTA_H_
#define _TIMONEL_TWIM_OTA_H_

#include <Arduino.h>
#include <ESP8266WiFi.h>
//#include <FS.h>
#include "LittleFS.h"
#include <NbMicro.h>
#include <TimonelTwiM.h>
#include <TwiBus.h>
#include <WiFiClientSecure.h>
#include <nb-twi-cmd.h>

#ifndef SSID
//#define SSID "YourSSID"
//#define PASS "Password"
#define SSID "Nicebots.com"
#define PASS "R2-D2 C-3P0"
#endif  // SSID

// This software
#define VER_MAJOR 2
#define VER_MINOR 1
#define VER_PATCH 0

// Serial display settings
#define USE_SERIAL Serial
#define SERIAL_BPS 115200

// I2C pins
#define SDA 2  // I2C SDA pin - ESP8266 2 - ESP32 21
#define SCL 0  // I2C SCL pin - ESP8266 0 - ESP32 22

// ATtiny85 MAX update attempts number
const uint8_t MAX_UPDATE_TRIES = 3;
#define WEB_HOST "raw.githubusercontent.com"
#define WEB_PORT 443
// Use Firefox browser to get the web site certificate SHA1 fingerprint (case-insensitive)
const char FINGERPRINT[] PROGMEM = "70 94 de dd e6 c4 69 48 3a 92 70 a1 48 56 78 2d 18 64 e0 b7";

#define FW_WEB_URL "/casanovg/timonel-ota-demo/master/fw-attiny85"  // Firmware updates base URL
#define FW_ONBOARD_VER "/fw-onboard.md"                             // This file keeps the firmware version currently running on the ATtiny85
#define FW_ONBOARD_LOC "/fw-onboard.hex"                            // Firmware file currently running on the ATtiny85
#define FW_LATEST_VER "/fw-latest.md"                               // This file keeps the new firmware version to flash the ATtiny85
#define FW_LATEST_LOC "/fw-latest.hex"                              // New firmware file to flash the ATtiny85
#define FW_LATEST_WEB FW_WEB_URL "/fw-latest.md"                    // Full URL to check for updates
#define UPDATE_TRIES "/update-tries.md"                             // This file keeps the uploading try count across master resets

#define IHEX_START_CODE ':'  // Intel Hex file record start code

// Prototypes
void ClrScr(void);
void CheckFwUpdate(void);
//CheckForUpdates(const char url[], const char host[]);
uint8_t Format(void);
uint8_t ListFiles(void);
bool Exists(const char file_name[]);
String ReadFile(const char file_name[]);
uint8_t WriteFile(const char file_name[], const String file_data);
uint8_t Rename(const char source_file_name[], const char destination_file_name[]);
uint8_t DeleteFile(const char file_name[]);
void RotaryDelay(void);

String CheckFwUpdate(const char ssid[],
                   const char password[],
                   const char host[],
                   const int port,
                   const char fingerprint[],
                   const String current_version,
                   const String latest_version);

void UpdateFirmware(String new_version);

void FlashTwiDevice(uint8_t payload[], uint16_t payload_size, uint8_t update_tries);

String GetHttpDocument(const char ssid[],
                       const char password[],
                       const char host[],
                       const int port,
                       const char fingerprint[],
                       String url,
                       char terminator);
bool ParseIHexFormat(String serialized_file, uint8_t *payload);
uint16_t GetIHexSize(String serialized_file);
void StartApplication(void);
void RetryRestart(const char file_name[], uint8_t update_tries);

#endif  // _TIMONEL_TWIM_OTA_H_
