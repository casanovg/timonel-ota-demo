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
#include <FS.h>
#include <NbMicro.h>
#include <TimonelTwiM.h>
#include <TwiBus.h>
#include <WiFiClientSecure.h>
#include <nb-twi-cmd.h>

#ifndef SSID
#define SSID "YourSSID"
#define PASS "Password"
#endif  // SSID

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
