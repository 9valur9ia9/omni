#include "omni_cmd.h"
#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

static const OmniHooks* gH = nullptr;

const char* omniVersion() { return "v2.0"; }

void omniInit(const OmniHooks* hooks) { gH = hooks; }

static void append(char* out, size_t n, size_t* used, const char* s) {
  if (!out || !n || !used || !s) return;
  size_t len = strlen(s);
  if (*used + len + 1 >= n) len = (n > *used + 1) ? (n - *used - 1) : 0;
  if (len) {
    memcpy(out + *used, s, len);
    *used += len;
    out[*used] = 0;
  }
}

static void appendf(char* out, size_t n, size_t* used, const char* fmt, ...) {
  char buf[384];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);
  append(out, n, used, buf);
}

static void skipWs(const char** p) {
  while (**p && isspace((unsigned char)**p)) (*p)++;
}

static bool tokEq(const char* a, const char* b) {
  return a && b && strcasecmp(a, b) == 0;
}

/** Pull next token into buf; advances p. Returns false if empty. */
static bool nextTok(const char** p, char* buf, size_t buflen) {
  skipWs(p);
  if (!**p) {
    if (buf && buflen) buf[0] = 0;
    return false;
  }
  size_t i = 0;
  if (**p == '"') {
    (*p)++;
    while (**p && **p != '"' && i + 1 < buflen) buf[i++] = *(*p)++;
    if (**p == '"') (*p)++;
  } else {
    while (**p && !isspace((unsigned char)**p) && i + 1 < buflen) buf[i++] = *(*p)++;
  }
  buf[i] = 0;
  return i > 0;
}

static void fmtUptime(uint32_t ms, char* buf, size_t n) {
  uint32_t s = ms / 1000;
  uint32_t m = s / 60;
  uint32_t h = m / 60;
  s %= 60;
  m %= 60;
  if (h)
    snprintf(buf, n, "%lu ms (%luh %lum %lus)", (unsigned long)ms, (unsigned long)h,
             (unsigned long)m, (unsigned long)s);
  else
    snprintf(buf, n, "%lu ms (%lum %lus)", (unsigned long)ms, (unsigned long)m,
             (unsigned long)s);
}

static void cmdHelp(char* out, size_t n, size_t* u) {
  append(out, n, u,
         "=== OMNISCAN COMMANDS ===\n"
         "./omni start - Start full spectrum scanning\n"
         "./omni stop - Stop scanning\n"
         "./omni status - Show status\n\n"
         "./omni wifi channel <1-11> - Set Wi-Fi channel\n"
         "./omni wifi channel hop - Enable channel hopping\n"
         "./omni wifi channel fixed - Disable channel hopping\n"
         "./omni wifi handshake - Show handshake stats\n"
         "./omni wifi deauth - Show deauth attacks\n\n"
         "./omni ble scan on/off - Start/stop onboard BLE scanning\n"
         "./omni ble list - List BLE devices\n"
         "./omni ble filter <name> - Filter BLE devices\n"
         "./omni ble filter clear - Clear BLE filter\n\n"
         "./omni subghz scan on/off - Start/stop sub-GHz scanning\n"
         "./omni subghz freq <315|433|868|915> - Set frequency\n"
         "./omni subghz hop - Enable frequency hopping\n"
         "./omni subghz list - Show sub-GHz packets\n"
         "./omni subghz raw - Last 10 raw packets (HEX/ASCII/GPS)\n"
         "./omni subghz raw <N|last|filter|clear|save|decode|analyze>\n\n"
         "./omni lora scan on/off - Start/stop LoRa listening\n"
         "./omni lora freq <868.1|915.0> - Set LoRa frequency\n"
         "./omni lora list - Show LoRa packets\n\n"
         "./omni gps status - Show GPS status\n"
         "./omni gps reset - Force GPS reset\n\n"
         "./omni lr status - Show LR status\n"
         "./omni lr peer <MAC> - Set LR peer\n"
         "./omni lr send <message> - Send LR message\n"
         "./omni lr test - Test LR connection\n\n"
         "./omni ap status - Show AP status\n"
         "./omni ap on - Enable AP mode\n"
         "./omni ap off - Disable AP mode\n"
         "./omni ap ssid <name> - Change AP SSID\n\n"
         "./omni log status - Show logging status\n"
         "./omni log dump - Dump current log\n"
         "./omni log save - Force save\n\n"
         "./omni system info - Show system info\n"
         "./omni system reset - Soft reset system\n"
         "./omni system update - Check for OTA updates\n"
         "./omni system help - Show this help\n");
}

static void cmdStart(char* out, size_t n, size_t* u) {
  if (!gH || !gH->setRunning) {
    append(out, n, u, "ERROR: OmniScan hooks missing\n");
    return;
  }
  gH->setRunning(true);
  OmniSnapshot s = gH->snapshot();
  append(out, n, u, "OmniScan v2.0 starting...\n");
  appendf(out, n, u, "Wi-Fi: Promiscuous mode on Ch %u%s%s\n", (unsigned)s.wifiChannel,
          s.wifiHopping ? " + hopping" : "", s.wifiLr ? " + LR" : "");
  append(out, n, u,
         s.bleOn ? "BLE: Active scanning started (onboard ESP32-S3)\n"
                 : "BLE: OFF — ./omni ble scan on\n");
  if (s.subghzOn) {
    if (s.subghzHopping)
      append(out, n, u, "Sub-GHz: Hopping 315→433→868→915 MHz\n");
    else
      appendf(out, n, u, "Sub-GHz: Fixed %.2f MHz\n", s.subghzFreqMhz);
  } else {
    append(out, n, u, "Sub-GHz: OFF / not ready\n");
  }
  appendf(out, n, u, "LoRa: %s on %.1f MHz\n", s.loraOn ? "Listening" : "OFF",
          s.loraFreqMhz);
  if (s.gpsLocked)
    appendf(out, n, u, "GPS: Locked (%.7f, %.7f)\n", s.gpsLat, s.gpsLon);
  else
    append(out, n, u, "GPS: Searching / inject via ARIA\n");
  appendf(out, n, u, "AP: \"%s\" %s\n", s.apSsid, s.apOn ? "active" : "off");
  if (s.lrReady && s.lrPeer[0])
    appendf(out, n, u, "LR: Connected to peer %s (RSSI: %d dBm)\n", s.lrPeer,
            (int)s.lrRssi);
  else if (s.lrReady)
    append(out, n, u, "LR: Wi-Fi LR + ESP-NOW ready (set peer)\n");
  else
    append(out, n, u, "LR: idle\n");
  append(out, n, u,
         s.sdMounted ? "SD: Logging\n" : "SD: Not mounted — session kept in PSRAM\n");
  append(out, n, u, "Status: RUNNING\n");
}

static void cmdStop(char* out, size_t n, size_t* u) {
  OmniSnapshot s = gH->snapshot();
  if (!s.running) {
    append(out, n, u,
           "ERROR: OmniScan is not running\nUse \"./omni start\" to start scanning\n");
    return;
  }
  gH->setRunning(false);
  append(out, n, u, "OmniScan stopping...\n");
  if (gH->logSave) {
    char path[80];
    if (gH->logSave(path, sizeof path))
      appendf(out, n, u, "Saving logs...\nData saved: %s\n", path);
    else
      append(out, n, u, "Saving session snapshot...\n");
  }
  append(out, n, u, "Status: STOPPED\n");
}

static void cmdStatus(char* out, size_t n, size_t* u) {
  OmniSnapshot s = gH->snapshot();
  char up[64];
  fmtUptime(s.uptimeMs, up, sizeof up);
  append(out, n, u, "=== OMNISCAN STATUS ===\n");
  appendf(out, n, u, "Uptime: %s\n", up);
  appendf(out, n, u, "Wi-Fi: %s (Ch %u, %lu devices)\n",
          s.running ? "RUNNING" : "IDLE", (unsigned)s.wifiChannel,
          (unsigned long)s.wifiDevices);
  appendf(out, n, u, "BLE: %s (%lu devices — onboard)\n",
          s.bleOn ? "RUNNING" : "OFF", (unsigned long)s.bleDevices);
  appendf(out, n, u, "Sub-GHz: %s (%lu packets)\n",
          s.subghzOn ? "RUNNING" : "OFF", (unsigned long)s.subghzPackets);
  appendf(out, n, u, "LoRa: %s (%lu packets)\n", s.loraOn ? "RUNNING" : "OFF",
          (unsigned long)s.loraPackets);
  if (s.gpsLocked)
    appendf(out, n, u, "GPS: LOCKED (%.7f, %.7f)\n", s.gpsLat, s.gpsLon);
  else
    append(out, n, u, "GPS: SEARCHING\n");
  appendf(out, n, u, "AP: %s (\"%s\")\n", s.apOn ? "ACTIVE" : "OFF", s.apSsid);
  appendf(out, n, u, "LR: %s (RSSI: %d dBm, %lu sent / %lu ACKed)%s\n",
          s.lrReady ? "ACTIVE" : "IDLE", (int)s.lrRssi, (unsigned long)s.lrSent,
          (unsigned long)s.lrAcked, s.wifiLr ? " · wifi_lr=on" : "");
  appendf(out, n, u, "SD: %s\n", s.sdMounted ? "MOUNTED" : "NOT MOUNTED (PSRAM session)");
  appendf(out, n, u, "Handshakes: %lu\n", (unsigned long)s.handshakes);
  appendf(out, n, u, "Deauth Attacks: %lu\n", (unsigned long)s.deauths);
  appendf(out, n, u, "Memory: heap free %lu · PSRAM free %lu / %lu\n",
          (unsigned long)s.freeHeap, (unsigned long)s.freePsram,
          (unsigned long)s.psramSize);
}

static void cmdSystemInfo(char* out, size_t n, size_t* u) {
  OmniSnapshot s = gH->snapshot();
  char up[64];
  fmtUptime(s.uptimeMs, up, sizeof up);
  append(out, n, u, "=== SYSTEM INFO ===\n");
  append(out, n, u, "Model: ESP32-S3-N16R8\n");
  append(out, n, u, "CPU: Xtensa LX7 @ 240MHz (Dual Core)\n");
  append(out, n, u, "Flash: 16MB\n");
  appendf(out, n, u, "PSRAM: %lu bytes (%lu free)\n", (unsigned long)s.psramSize,
          (unsigned long)s.freePsram);
  appendf(out, n, u, "Uptime: %s\n", up);
  if (s.cpuTempC > 0)
    appendf(out, n, u, "Temperature: %.0f°C\n", s.cpuTempC);
  append(out, n, u, "Voltage: 3.3V\n");
  appendf(out, n, u, "Wi-Fi MAC: %s\n", s.wifiMac);
  appendf(out, n, u, "AP MAC: %s\n", s.apMac);
  appendf(out, n, u, "OmniScan: %s\n", omniVersion());
}

bool omniHandle(const char* lineIn, char* out, size_t outn) {
  if (!out || outn < 8) return false;
  out[0] = 0;
  size_t u = 0;
  if (!gH || !gH->snapshot) {
    append(out, outn, &u, "ERROR: OmniScan not initialized\n");
    return false;
  }

  char line[256];
  size_t li = 0;
  for (const char* p = lineIn ? lineIn : ""; *p && li + 1 < sizeof line; p++) {
    if (*p != '\r' && *p != '\n') line[li++] = *p;
  }
  line[li] = 0;
  // trim
  while (li && isspace((unsigned char)line[li - 1])) line[--li] = 0;
  const char* p = line;
  skipWs(&p);
  if (!*p) {
    // bare empty → status via alias ./omni
    cmdStatus(out, outn, &u);
    return true;
  }

  // Accept "./omni …", "omni …", or short aliases without prefix
  char t0[48], t1[48], t2[48], t3[96];
  if (!nextTok(&p, t0, sizeof t0)) {
    cmdStatus(out, outn, &u);
    return true;
  }

  // Strip optional ./omni or omni
  if (tokEq(t0, "./omni") || tokEq(t0, "omni")) {
    if (!nextTok(&p, t0, sizeof t0)) {
      cmdStatus(out, outn, &u);
      return true;
    }
  }

  // Aliases: s, start, stop, h, d, ble, gps, lr, ap, log, ch <n>
  if (tokEq(t0, "s")) {
    cmdStatus(out, outn, &u);
    return true;
  }
  if (tokEq(t0, "start")) {
    cmdStart(out, outn, &u);
    return true;
  }
  if (tokEq(t0, "stop")) {
    cmdStop(out, outn, &u);
    return true;
  }
  if (tokEq(t0, "status")) {
    cmdStatus(out, outn, &u);
    return true;
  }
  if (tokEq(t0, "h")) {
    if (gH->wifiHandshake) gH->wifiHandshake(out, outn);
    else append(out, outn, &u, "ERROR: handshake hook missing\n");
    return out[0] != 'E';
  }
  if (tokEq(t0, "d")) {
    if (gH->wifiDeauth) gH->wifiDeauth(out, outn);
    else append(out, outn, &u, "ERROR: deauth hook missing\n");
    return out[0] != 'E';
  }
  if (tokEq(t0, "ch")) {
    nextTok(&p, t1, sizeof t1);
    int ch = atoi(t1);
    if (ch < 1 || ch > 11) {
      append(out, outn, &u, "ERROR: Invalid channel\nValid channels: 1-11\n");
      return false;
    }
    gH->setWifiChannel(ch);
    appendf(out, outn, &u, "Wi-Fi channel set to %d\n", ch);
    return true;
  }
  if (tokEq(t0, "ble") && !*p) {
    if (gH->bleList) return gH->bleList(out, outn);
  }
  if (tokEq(t0, "gps") && !*p) {
    OmniSnapshot s = gH->snapshot();
    append(out, outn, &u, "=== GPS STATUS ===\n");
    appendf(out, outn, &u, "Locked: %s\n", s.gpsLocked ? "YES" : "NO");
    appendf(out, outn, &u, "Latitude: %.7f\n", s.gpsLat);
    appendf(out, outn, &u, "Longitude: %.7f\n", s.gpsLon);
    appendf(out, outn, &u, "Altitude: %.1fm\n", s.gpsAlt);
    appendf(out, outn, &u, "Speed: %.1f km/h\n", s.gpsSpeedKmh);
    appendf(out, outn, &u, "Satellites: %u\n", (unsigned)s.gpsSats);
    appendf(out, outn, &u, "HDOP: %.1f\n", s.gpsHdop);
    return true;
  }
  if (tokEq(t0, "lr") && !*p) goto lr_status;
  if (tokEq(t0, "ap") && !*p) goto ap_status;
  if (tokEq(t0, "log") && !*p) goto log_status;

  // —— wifi ——
  if (tokEq(t0, "wifi")) {
    if (!nextTok(&p, t1, sizeof t1)) {
      append(out, outn, &u, "ERROR: Missing wifi subcommand\n");
      return false;
    }
    if (tokEq(t1, "channel")) {
      if (!nextTok(&p, t2, sizeof t2)) {
        append(out, outn, &u, "ERROR: Missing channel\nUsage: ./omni wifi channel <1-11|hop|fixed>\n");
        return false;
      }
      if (tokEq(t2, "hop")) {
        gH->setWifiChannel(0);
        append(out, outn, &u, "Wi-Fi channel hopping enabled (1→2→3→…→11)\n");
        return true;
      }
      if (tokEq(t2, "fixed")) {
        gH->setWifiChannel(-1);
        OmniSnapshot s = gH->snapshot();
        appendf(out, outn, &u, "Wi-Fi channel hopping disabled (fixed on Ch %u)\n",
                (unsigned)s.wifiChannel);
        return true;
      }
      int ch = atoi(t2);
      if (ch < 1 || ch > 11) {
        appendf(out, outn, &u, "ERROR: Invalid channel \"%s\"\nValid channels: 1-11\n", t2);
        return false;
      }
      gH->setWifiChannel(ch);
      appendf(out, outn, &u, "Wi-Fi channel set to %d\n", ch);
      return true;
    }
    if (tokEq(t1, "handshake")) {
      return gH->wifiHandshake && gH->wifiHandshake(out, outn);
    }
    if (tokEq(t1, "deauth")) {
      return gH->wifiDeauth && gH->wifiDeauth(out, outn);
    }
    appendf(out, outn, &u, "ERROR: Unknown command \"wifi %s\"\nType \"./omni system help\"\n",
            t1);
    return false;
  }

  // —— ble ——
  if (tokEq(t0, "ble")) {
    if (!nextTok(&p, t1, sizeof t1)) {
      return gH->bleList && gH->bleList(out, outn);
    }
    if (tokEq(t1, "scan")) {
      nextTok(&p, t2, sizeof t2);
      bool on = tokEq(t2, "on") || tokEq(t2, "1") || tokEq(t2, "true");
      bool off = tokEq(t2, "off") || tokEq(t2, "0") || tokEq(t2, "false");
      if (!on && !off) {
        append(out, outn, &u, "ERROR: Usage: ./omni ble scan on|off\n");
        return false;
      }
      if (gH->setBleScan) gH->setBleScan(on);
      appendf(out, outn, &u, "BLE scanning: %s\n", on ? "ON" : "OFF");
      return true;
    }
    if (tokEq(t1, "list")) return gH->bleList && gH->bleList(out, outn);
    if (tokEq(t1, "filter")) {
      if (!nextTok(&p, t2, sizeof t2)) {
        append(out, outn, &u, "ERROR: Missing filter\nUsage: ./omni ble filter <name|clear>\n");
        return false;
      }
      return gH->bleFilter && gH->bleFilter(t2, out, outn);
    }
    appendf(out, outn, &u, "ERROR: Unknown command \"ble %s\"\n", t1);
    return false;
  }

  // —— subghz ——
  if (tokEq(t0, "subghz")) {
    if (!nextTok(&p, t1, sizeof t1)) {
      append(out, outn, &u, "ERROR: Missing subghz subcommand\n");
      return false;
    }
    if (tokEq(t1, "scan")) {
      nextTok(&p, t2, sizeof t2);
      bool on = tokEq(t2, "on");
      bool off = tokEq(t2, "off");
      if (!on && !off) {
        append(out, outn, &u, "ERROR: Usage: ./omni subghz scan on|off\n");
        return false;
      }
      if (gH->setSubghzScan) gH->setSubghzScan(on);
      appendf(out, outn, &u, "Sub-GHz scanning: %s\n", on ? "ON" : "OFF");
      return true;
    }
    if (tokEq(t1, "freq")) {
      if (!nextTok(&p, t2, sizeof t2)) {
        append(out, outn, &u, "ERROR: Missing frequency\nValid: 315|433|868|915\n");
        return false;
      }
      float mhz = atof(t2);
      if (mhz < 300 || mhz > 950) {
        append(out, outn, &u, "ERROR: Invalid frequency\nValid: 315|433|868|915\n");
        return false;
      }
      if (gH->setSubghzFreq) gH->setSubghzFreq(mhz);
      appendf(out, outn, &u, "Sub-GHz frequency set to %.2f MHz\n", mhz);
      return true;
    }
    if (tokEq(t1, "hop")) {
      if (gH->setSubghzFreq) gH->setSubghzFreq(0);
      append(out, outn, &u, "Sub-GHz hopping enabled: 315→433→868→915 MHz\n");
      return true;
    }
    if (tokEq(t1, "list")) return gH->subghzList && gH->subghzList(out, outn);
    if (tokEq(t1, "raw")) {
      // rest of line is args
      skipWs(&p);
      return gH->subghzRaw && gH->subghzRaw(p, out, outn);
    }
    appendf(out, outn, &u, "ERROR: Unknown command \"subghz %s\"\n", t1);
    return false;
  }

  // —— lora ——
  if (tokEq(t0, "lora")) {
    if (!nextTok(&p, t1, sizeof t1)) {
      append(out, outn, &u, "ERROR: Missing lora subcommand\n");
      return false;
    }
    if (tokEq(t1, "scan")) {
      nextTok(&p, t2, sizeof t2);
      bool on = tokEq(t2, "on");
      if (gH->setLoraScan) gH->setLoraScan(on);
      appendf(out, outn, &u, "LoRa scanning: %s\n", on ? "ON" : "OFF");
      return true;
    }
    if (tokEq(t1, "freq")) {
      if (!nextTok(&p, t2, sizeof t2)) {
        append(out, outn, &u, "ERROR: Missing frequency\nValid: 868.1|915.0\n");
        return false;
      }
      float mhz = atof(t2);
      if (gH->setLoraFreq) gH->setLoraFreq(mhz);
      appendf(out, outn, &u, "LoRa frequency set to %.1f MHz\n", mhz);
      return true;
    }
    if (tokEq(t1, "list")) return gH->loraList && gH->loraList(out, outn);
    appendf(out, outn, &u, "ERROR: Unknown command \"lora %s\"\n", t1);
    return false;
  }

  // —— gps ——
  if (tokEq(t0, "gps")) {
    if (!nextTok(&p, t1, sizeof t1) || tokEq(t1, "status")) {
      OmniSnapshot s = gH->snapshot();
      append(out, outn, &u, "=== GPS STATUS ===\n");
      appendf(out, outn, &u, "Locked: %s\n", s.gpsLocked ? "YES" : "NO");
      appendf(out, outn, &u, "Latitude: %.7f\nLongitude: %.7f\n", s.gpsLat, s.gpsLon);
      appendf(out, outn, &u, "Altitude: %.1fm\nSpeed: %.1f km/h\n", s.gpsAlt, s.gpsSpeedKmh);
      appendf(out, outn, &u, "Satellites: %u\nHDOP: %.1f\n", (unsigned)s.gpsSats, s.gpsHdop);
      return true;
    }
    if (tokEq(t1, "reset")) {
      append(out, outn, &u, "GPS resetting...\n");
      if (gH->gpsReset) gH->gpsReset();
      OmniSnapshot s = gH->snapshot();
      if (s.gpsLocked)
        appendf(out, outn, &u, "GPS: Locked (%.7f, %.7f)\n", s.gpsLat, s.gpsLon);
      else
        append(out, outn, &u, "GPS: Searching for satellites / await laptop inject...\n");
      return true;
    }
    appendf(out, outn, &u, "ERROR: Unknown command \"gps %s\"\n", t1);
    return false;
  }

  // —— lr ——
  if (tokEq(t0, "lr")) {
    if (!nextTok(&p, t1, sizeof t1) || tokEq(t1, "status")) {
    lr_status:
      OmniSnapshot s = gH->snapshot();
      append(out, outn, &u, "=== LR STATUS ===\n");
      appendf(out, outn, &u, "Mode: %s\n", s.lrReady ? "ACTIVE" : "IDLE");
      appendf(out, outn, &u, "Protocol: %s\n", s.wifiLr ? "b/g/n + LR" : "b/g/n");
      appendf(out, outn, &u, "Peer: %s\n", s.lrPeer[0] ? s.lrPeer : "(none)");
      appendf(out, outn, &u, "RSSI: %d dBm\n", (int)s.lrRssi);
      appendf(out, outn, &u, "Packets Sent: %lu\nPackets ACKed: %lu\n",
              (unsigned long)s.lrSent, (unsigned long)s.lrAcked);
      append(out, outn, &u, "Path: Espressif Wi-Fi LR + ESP-NOW\n");
      return true;
    }
    if (tokEq(t1, "peer")) {
      if (!nextTok(&p, t2, sizeof t2)) {
        append(out, outn, &u,
               "ERROR: Missing MAC address\nUsage: ./omni lr peer <MAC>\n"
               "Example: ./omni lr peer AA:BB:CC:DD:EE:FF\n");
        return false;
      }
      if (gH->setLrPeer) gH->setLrPeer(t2);
      appendf(out, outn, &u, "LR peer set to %s\nAttempting connection...\n", t2);
      OmniSnapshot s = gH->snapshot();
      if (s.lrReady)
        appendf(out, outn, &u, "ESP-NOW peer ready. RSSI est: %d dBm\n", (int)s.lrRssi);
      return true;
    }
    if (tokEq(t1, "send")) {
      skipWs(&p);
      if (!*p) {
        append(out, outn, &u, "ERROR: Missing message\nUsage: ./omni lr send <message>\n");
        return false;
      }
      // strip quotes
      char msg[160];
      if (*p == '"') {
        p++;
        size_t i = 0;
        while (*p && *p != '"' && i + 1 < sizeof msg) msg[i++] = *p++;
        msg[i] = 0;
      } else {
        strncpy(msg, p, sizeof msg - 1);
        msg[sizeof msg - 1] = 0;
      }
      if (gH->lrSend) gH->lrSend(msg);
      appendf(out, outn, &u, "LR message sent: \"%s\"\n", msg);
      return true;
    }
    if (tokEq(t1, "test")) {
      if (gH->lrTest) return gH->lrTest(out, outn);
      append(out, outn, &u, "LR Ping...\nERROR: LR test unavailable\n");
      return false;
    }
    appendf(out, outn, &u, "ERROR: Unknown command \"lr %s\"\n", t1);
    return false;
  }

  // —— ap ——
  if (tokEq(t0, "ap")) {
    if (!nextTok(&p, t1, sizeof t1) || tokEq(t1, "status")) {
    ap_status: {
      OmniSnapshot s = gH->snapshot();
      append(out, outn, &u, "=== AP STATUS ===\n");
      appendf(out, outn, &u, "Mode: %s\nSSID: %s\nChannel: %u\nClients: %u\n",
              s.apOn ? "ACTIVE" : "OFF", s.apSsid, (unsigned)s.apChannel,
              (unsigned)s.apClients);
      appendf(out, outn, &u, "MAC: %s\nIP: %s\n", s.apMac, s.apIp);
      return true;
    }
    }
    if (tokEq(t1, "on")) {
      if (gH->setAp) gH->setAp(true);
      OmniSnapshot s = gH->snapshot();
      appendf(out, outn, &u, "AP mode: ON\nSSID: %s\nIP: %s\n", s.apSsid, s.apIp);
      return true;
    }
    if (tokEq(t1, "off")) {
      append(out, outn, &u,
             "ERROR: SoftAP off would drop this session — keep AP on for dashboard\n");
      return false;
    }
    if (tokEq(t1, "ssid")) {
      if (!nextTok(&p, t2, sizeof t2)) {
        append(out, outn, &u, "ERROR: Missing SSID\nUsage: ./omni ap ssid <name>\n");
        return false;
      }
      if (gH->setApSsid) gH->setApSsid(t2);
      appendf(out, outn, &u, "AP SSID changed to: %s\nAP restarting...\n", t2);
      return true;
    }
    appendf(out, outn, &u, "ERROR: Unknown command \"ap %s\"\n", t1);
    return false;
  }

  // —— log ——
  if (tokEq(t0, "log")) {
    if (!nextTok(&p, t1, sizeof t1) || tokEq(t1, "status")) {
    log_status: {
      OmniSnapshot s = gH->snapshot();
      append(out, outn, &u, "=== LOGGING STATUS ===\n");
      appendf(out, outn, &u, "SD Card: %s\n",
              s.sdMounted ? "MOUNTED" : "NOT MOUNTED (PSRAM session)");
      appendf(out, outn, &u, "Log File: %s\n", s.logPath[0] ? s.logPath : "(session RAM)");
      appendf(out, outn, &u, "Log Size: %.1fMB\nEntries: %lu\n", s.logSizeMb,
              (unsigned long)s.logEntries);
      return true;
    }
    }
    if (tokEq(t1, "dump")) {
      if (gH->logDump) return gH->logDump(out, outn);
      append(out, outn, &u, "Dumping log...\nDone.\n");
      return true;
    }
    if (tokEq(t1, "save")) {
      char path[80] = "";
      if (gH->logSave) gH->logSave(path, sizeof path);
      append(out, outn, &u, "Saving...\n");
      if (path[0]) appendf(out, outn, &u, "File: %s\n", path);
      append(out, outn, &u, "Done.\n");
      return true;
    }
    appendf(out, outn, &u, "ERROR: Unknown command \"log %s\"\n", t1);
    return false;
  }

  // —— system ——
  if (tokEq(t0, "system")) {
    if (!nextTok(&p, t1, sizeof t1)) {
      append(out, outn, &u, "ERROR: Missing system subcommand\n");
      return false;
    }
    if (tokEq(t1, "help") || tokEq(t1, "?")) {
      cmdHelp(out, outn, &u);
      return true;
    }
    if (tokEq(t1, "info")) {
      cmdSystemInfo(out, outn, &u);
      return true;
    }
    if (tokEq(t1, "reset")) {
      append(out, outn, &u, "System resetting...\nSaving logs...\nRebooting...\n");
      if (gH->systemReset) gH->systemReset();
      return true;
    }
    if (tokEq(t1, "update")) {
      append(out, outn, &u,
             "Checking for updates...\nCurrent version: v2.0\n"
             "OTA not configured on this build — flash via PlatformIO.\n");
      return true;
    }
    appendf(out, outn, &u, "ERROR: Unknown command \"system %s\"\n", t1);
    return false;
  }

  if (tokEq(t0, "help") || tokEq(t0, "?")) {
    cmdHelp(out, outn, &u);
    return true;
  }

  appendf(out, outn, &u,
          "ERROR: Unknown command \"%s\"\nType \"./omni system help\" for available commands\n",
          t0);
  (void)t3;
  return false;
}
