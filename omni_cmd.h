#pragma once
#include <stddef.h>
#include <stdint.h>

/** OmniScan ./omni command interface — Serial UART + HTTP /api/omni */

struct OmniSnapshot {
  bool running;
  uint32_t uptimeMs;
  uint8_t wifiChannel;
  bool wifiHopping;
  uint32_t wifiDevices;
  uint32_t handshakes;
  uint32_t deauths;
  bool bleOn;          // onboard ESP32-S3 BLE scan
  uint32_t bleDevices;
  bool wifiLr;         // Espressif 802.11 LR protocol on SoftAP
  bool subghzOn;
  bool subghzHopping;
  float subghzFreqMhz;
  uint32_t subghzPackets;
  bool loraOn;
  float loraFreqMhz;
  uint32_t loraPackets;
  bool gpsLocked;
  double gpsLat;
  double gpsLon;
  float gpsAlt;
  float gpsSpeedKmh;
  uint8_t gpsSats;
  float gpsHdop;
  bool apOn;
  char apSsid[33];
  uint8_t apChannel;
  uint8_t apClients;
  char apMac[18];
  char wifiMac[18];
  char bleMac[18];
  char apIp[16];
  bool lrReady;  // Espressif Wi-Fi LR + ESP-NOW
  int8_t lrRssi;
  uint32_t lrSent;
  uint32_t lrAcked;
  char lrPeer[18];
  bool sdMounted;
  float sdFreeGb;
  char logPath[64];
  uint32_t logEntries;
  float logSizeMb;
  uint32_t freeHeap;
  uint32_t freePsram;
  uint32_t psramSize;
  float cpuTempC;
};

struct OmniHooks {
  OmniSnapshot (*snapshot)();
  bool (*setRunning)(bool on);
  bool (*setWifiChannel)(int ch);          // 1-11, or 0=hop, -1=fixed
  bool (*setBleScan)(bool on);
  bool (*setSubghzScan)(bool on);
  bool (*setSubghzFreq)(float mhz);        // 0 = hop
  bool (*setLoraScan)(bool on);
  bool (*setLoraFreq)(float mhz);
  bool (*gpsReset)();
  bool (*setAp)(bool on);
  bool (*setApSsid)(const char* ssid);
  bool (*setLrPeer)(const char* mac);
  bool (*lrSend)(const char* msg);
  bool (*lrTest)(char* out, size_t n);
  bool (*logSave)(char* pathOut, size_t n);
  bool (*logDump)(char* out, size_t n);
  bool (*systemReset)();
  /** Format wifi handshake / deauth / ble / subghz / lora list text into out */
  bool (*wifiHandshake)(char* out, size_t n);
  bool (*wifiDeauth)(char* out, size_t n);
  bool (*bleList)(char* out, size_t n);
  bool (*bleFilter)(const char* name, char* out, size_t n);
  bool (*subghzList)(char* out, size_t n);
  bool (*subghzRaw)(const char* args, char* out, size_t n);
  bool (*loraList)(char* out, size_t n);
};

void omniInit(const OmniHooks* hooks);
/** Parse line (with or without ./omni prefix). Writes response text to out. Returns true if OK. */
bool omniHandle(const char* line, char* out, size_t outn);
const char* omniVersion();
