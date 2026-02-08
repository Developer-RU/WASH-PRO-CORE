/**
 * @file SystemManager.h
 * @author Masyukov Pavel
 * @brief Definition of the SystemManager class for the WASH-PRO project.
 * @version 1.0.0
 * @see https://github.com/pavelmasyukov/WASH-PRO-CORE
 */
#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

/**
 * @class SystemManager
 * @brief Manages system-level configurations and operations.
 *
 * This class handles system settings like language, theme, license key,
 * as well as operations like OTA updates, file system uploads, Wi-Fi configuration,
 * and system reboots. It uses Preferences for persistent storage.
 */
class SystemManager {
public:
  /**
   * @brief Initializes the SystemManager.
   * Reads stored preferences for language, theme, and license key.
   */
  void begin();

  /**
   * @brief Gets system information as a JSON string.
   * Includes serial number, license status, heap memory, and script count.
   * @return A JSON string with system information.
   */
  String getInfoJSON();

  /**
   * @brief Gets system settings as a JSON string.
   * Includes software version, language, theme, license key, and auto-update status.
   * @return A JSON string with system settings.
   */
  String getSystemJSON();

  /**
   * @brief Sets the system language.
   * @param lang The language code to set (e.g., "en", "ru").
   */
  void setLanguage(const String &lang);

  /**
   * @brief Gets the current system language.
   * @return The current language code.
   */
  String getLanguage();
  
  /**
   * @brief Sets the system theme.
   * @param theme The name of the theme to set.
   */
  void setTheme(const String &theme);

  /**
   * @brief Gets the current system theme.
   * @return The name of the current theme.
   */
  String getTheme();

  /**
   * @brief Sets the license key.
   * @param key The license key string.
   */
  void setLicenseKey(const String &key);

  /**
   * @brief Gets the current license key.
   * @return The license key string.
   */
  String getLicenseKey();

  /**
   * @brief Sets the auto-update preference.
   * @param enabled True to enable auto-update, false to disable.
   */
  void setAutoUpdate(bool enabled);

  /**
   * @brief Handles OTA firmware update uploads.
   * This function is called by the web server when a file is being uploaded.
   * @param request The web server request object.
   * @param filename The name of the uploaded file.
   * @param index The starting index of the data chunk.
   * @param data A pointer to the data chunk.
   * @param len The length of the data chunk.
   * @param final True if this is the last chunk of the file.
   */
  void handleOTAUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);

  /**
   * @brief Handles file uploads to the LittleFS filesystem.
   * @param request The web server request object.
   * @param filename The name of the uploaded file.
   * @param index The starting index of the data chunk.
   * @param data A pointer to the data chunk.
   * @param len The length of the data chunk.
   * @param final True if this is the last chunk of the file.
   */
  void handleFSUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);

  /**
   * @brief Saves Wi-Fi credentials to persistent storage.
   * @param ssid The Wi-Fi network SSID.
   * @param password The Wi-Fi network password.
   * @return True on success, false on failure.
   */
  bool saveWiFiCredentials(const String &ssid, const String &password);

  /**
   * @brief Schedules a system reboot.
   * @param delaySeconds The delay in seconds before rebooting. 0 for immediate.
   * @param graceful If true, performs a software restart. If false, a hardware-like reset.
   */
  void scheduleReboot(uint32_t delaySeconds, bool graceful);

private:
  Preferences _prefs;      ///< Preferences object for persistent storage.
  String _language = "en"; ///< Current system language.
  String _theme = "gp_light"; ///< Current system theme.
  String _licenseKey = ""; ///< Stored license key.
};
