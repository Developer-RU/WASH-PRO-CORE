/**
 * @file SystemManager.cpp_
 * @author Masyukov Pavel
 * @brief Implementation of the SystemManager class for the WASH-PRO project.
 * @version 1.0.0
 * @see https://github.com/pavelmasyukov/WASH-PRO-CORE
 */
#include "SystemManager.h"
#include <Update.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <esp_system.h>

/**
 * @brief Initializes the SystemManager.
 * 
 * Loads system settings like language, theme, and license key from persistent storage.
 */
void SystemManager::begin() {
  _prefs.begin("system", false);
  _language = _prefs.getString("lang", "en");
  _theme = _prefs.getString("theme", "gp_light");
  _licenseKey = _prefs.getString("license_key", "");
}

/**
 * @brief Gets system information as a JSON string.
 * 
 * @param runningTasksCount The number of currently running tasks to include in the info.
 * @return String A JSON string containing system information.
 */
String SystemManager::getInfoJSON(int runningTasksCount) {
  DynamicJsonDocument doc(1024);
  uint64_t mac = ESP.getEfuseMac();
  char macStr[13];
  snprintf(macStr, sizeof(macStr), "%012llX", mac);

  doc["serial"] = macStr;
  doc["licenseActive"] = _prefs.getBool("license", true);
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["heapSize"] = ESP.getHeapSize();

  int scriptCount = 0;
  if (LittleFS.exists("/scripts")) {
    File root = LittleFS.open("/scripts");
    if (root && root.isDirectory()) {
      File file = root.openNextFile();
      while (file) {
        if (!file.isDirectory()) {
          scriptCount++;
        } 
        file.close();
        file = root.openNextFile();
      }
      root.close();
    }
  }

  doc["userScripts"] = scriptCount;
  doc["runningTasks"] = runningTasksCount;
  String out;
  serializeJson(doc, out);
  return out;
}

/**
 * @brief Gets system settings as a JSON string.
 * 
 * @return String A JSON string containing system settings.
 */
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

/**
 * @brief Sets the system language.
 * 
 * @param lang The language code to set (e.g., "en", "ru").
 */
void SystemManager::setLanguage(const String &lang) {
  _language = lang;
  _prefs.putString("lang", _language);
}

/**
 * @brief Sets the system theme.
 * 
 * @param theme The name of the theme to set.
 */
void SystemManager::setTheme(const String &theme) {
  _theme = theme;
  _prefs.putString("theme", _theme);
}

/**
 * @brief Gets the current system theme.
 * 
 * @return String The name of the current theme.
 */
String SystemManager::getTheme() {
  return _theme;
}

/**
 * @brief Sets the license key.
 * 
 * @param key The license key to save.
 */
void SystemManager::setLicenseKey(const String &key) {
  _licenseKey = key;
  _prefs.putString("license_key", _licenseKey);
}

/**
 * @brief Gets the current license key.
 * 
 * @return String The current license key.
 */
String SystemManager::getLicenseKey() {
  return _licenseKey;
}

/**
 * @brief Sets the auto-update preference.
 * 
 * @param enabled True to enable auto-update, false to disable.
 */
void SystemManager::setAutoUpdate(bool enabled) {
  _prefs.putBool("auto_update", enabled);
}

/**
 * @brief Handles OTA firmware update uploads.
 * This function is called by the web server for each chunk of the uploaded firmware file.
 * 
 * @param request The web server request object.
 * @param filename The name of the uploaded file.
 * @param index The starting index of the data chunk.
 * @param data A pointer to the data chunk.
 * @param len The length of the data chunk.
 * @param final True if this is the last chunk of the file.
 */
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
      Serial.println("OTA update successful, will restart.");
      // schedule immediate reboot
      ESP.restart();
    } else {
      Serial.printf("OTA Error: %s\n", Update.errorString());
    }
  }
}

/**
 * @brief Handles file uploads to the LittleFS filesystem.
 * This function is called by the web server for each chunk of the uploaded file.
 * 
 * @param request The web server request object.
 * @param filename The name of the uploaded file.
 * @param index The starting index of the data chunk.
 * @param data A pointer to the data chunk.
 * @param len The length of the data chunk.
 * @param final True if this is the last chunk of the file.
 */
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

/**
 * @brief Saves Wi-Fi credentials to persistent storage.
 * 
 * @param ssid The SSID of the Wi-Fi network.
 * @param password The password for the Wi-Fi network.
 * @return bool Always returns true.
 * @note The return value does not indicate success of saving to NVS.
 */
bool SystemManager::saveWiFiCredentials(const String &ssid, const String &password) {
  _prefs.putString("wifi_ssid", ssid);
  _prefs.putString("wifi_pass", password);
  return true;
}

/**
 * @brief Schedules a system reboot.
 * 
 * @param delaySeconds The delay in seconds before rebooting. If 0, reboots immediately.
 * @param graceful If true, performs a "soft" reboot (ESP.restart()). If false, performs a "hard" reboot (esp_restart()).
 */
void SystemManager::scheduleReboot(uint32_t delaySeconds, bool graceful) {
  if (delaySeconds == 0) {
    // For immediate reboot, graceful is ESP.restart(), hard is esp_restart().
    graceful ? ESP.restart() : esp_restart();
  } else {
    // Spawn a task to wait for the specified delay and then reboot.
    xTaskCreate([](void *p) {
      uint32_t d = (uint32_t)(uintptr_t)p;
      vTaskDelay(d * 1000 / portTICK_PERIOD_MS);
      ESP.restart();
    }, "rebootTask", 2048, (void*)(uintptr_t)delaySeconds, 1, NULL); // Graceful reboot after delay.
  }
}
