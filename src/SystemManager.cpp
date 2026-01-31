#include "SystemManager.h"
#include <Update.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <esp_system.h>

void SystemManager::begin() {
  _prefs.begin("system", false);
  _language = _prefs.getString("lang", "en");
  _theme = _prefs.getString("theme", "gp_light");
  _licenseKey = _prefs.getString("license_key", "");
}

String SystemManager::getInfoJSON() {
  DynamicJsonDocument doc(1024);
  uint64_t mac = ESP.getEfuseMac();
  char macStr[32];
  sprintf(macStr, "%012llX", mac);
  doc["serial"] = String(macStr);
  doc["licenseActive"] = _prefs.getBool("license", true);
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["heapSize"] = ESP.getHeapSize();
  // count scripts
  int scriptCount = 0;
  if (LittleFS.exists("/scripts")) {
    File root = LittleFS.open("/scripts");
    if (root && root.isDirectory()) {
      while (true) {
        File file = root.openNextFile();
        if (!file) { // No more files
          break;
        }
        if (!file.isDirectory()) scriptCount++;
        file.close(); // Explicitly close the file handle
      }
      root.close();
    }
  }
  doc["userScripts"] = scriptCount;
  // running tasks count will be filled by TaskManager, for now put 0
  doc["runningTasks"] = 0;
  String out;
  serializeJson(doc, out);
  return out;
}

String SystemManager::getSystemJSON() {
  DynamicJsonDocument doc(512);
  doc["swSerial"] = "v1.0.0";
  doc["language"] = _language;
  doc["theme"] = _theme;
  doc["licenseKey"] = _licenseKey;
  doc["autoUpdate"] = _prefs.getBool("auto_update", false);
  String out;
  serializeJson(doc, out);
  return out;
}

void SystemManager::setLanguage(const String &lang) {
  _language = lang;
  _prefs.putString("lang", _language);
}

void SystemManager::setTheme(const String &theme) {
  _theme = theme;
  _prefs.putString("theme", _theme);
}

String SystemManager::getTheme() {
  return _theme;
}

void SystemManager::setLicenseKey(const String &key) {
  _licenseKey = key;
  _prefs.putString("license_key", _licenseKey);
}

String SystemManager::getLicenseKey() {
  return _licenseKey;
}

void SystemManager::setAutoUpdate(bool enabled) {
  _prefs.putBool("auto_update", enabled);
}

void SystemManager::handleOTAUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (index == 0) {
    Serial.printf("OTA Upload Start: %s\n", filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Serial.println("Not enough space for OTA");
      return;
    }
  }
  if (len) {
    Update.write(data, len);
  }
  if (final) {
    if (Update.end(true)) {
      Serial.println("OTA done, will restart");
      // schedule immediate reboot
      ESP.restart();
    } else {
      Serial.printf("OTA Error: %s\n", Update.errorString());
    }
  }
}

void SystemManager::handleFSUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  String path = "/";
  if (request->hasParam("path")) {
    path = request->getParam("path")->value();
  }
  if (!path.endsWith("/")) {
    path += "/";
  }

  if (index == 0) {
    Serial.printf("FS Upload Start: %s to %s\n", filename.c_str(), path.c_str());
  }
  String fullPath = path + filename;
  File f;
  if (index == 0) {
    f = LittleFS.open(fullPath, FILE_WRITE);
  } else {
    f = LittleFS.open(fullPath, FILE_APPEND);
  }
  if (!f) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (len) f.write(data, len);
  f.close();
  if (final) {
    Serial.printf("FS Upload End: %s, size=%u\n", filename.c_str(), index + len);
  }
}

bool SystemManager::saveWiFiCredentials(const String &ssid, const String &password) {
  _prefs.putString("wifi_ssid", ssid);
  _prefs.putString("wifi_pass", password);
  return true;
}

void SystemManager::scheduleReboot(uint32_t delaySeconds, bool graceful) {
  if (delaySeconds == 0) {
    if (graceful) {
      // notify tasks to finish if needed
      ESP.restart();
    } else {
      esp_restart();
    }
  } else {
    // spawn a task to wait and reboot
    xTaskCreate([](void *p) {
      uint32_t d = (uint32_t)(uintptr_t)p;
      vTaskDelay(d * 1000 / portTICK_PERIOD_MS);
      ESP.restart();
    }, "rebootTask", 2048, (void*)(uintptr_t)delaySeconds, 1, NULL);
  }
}
