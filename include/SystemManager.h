#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

class SystemManager {
public:
  void begin();
  String getInfoJSON();
  String getSystemJSON();
  void setLanguage(const String &lang);
  String getLanguage();
  void setTheme(const String &theme);
  String getTheme();
  void handleOTAUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
  void handleFSUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
  bool saveWiFiCredentials(const String &ssid, const String &password);
  void scheduleReboot(uint32_t delaySeconds, bool graceful);
private:
  Preferences _prefs;
  String _language = "en";
  String _theme = "gp_light";
};
