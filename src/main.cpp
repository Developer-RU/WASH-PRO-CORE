/**
 * @file main.cpp
 * @author Masyukov Pavel
 * @brief Main application file for the WASH-PRO project.
 * @version 1.0.0
 * @see https://github.com/pavelmasyukov/WASH-PRO-CORE
 *
 * This file contains the main setup and loop functions for the ESP32 application.
 * It initializes the system, tasks, Wi-Fi, and the web server with all its API endpoints.
 */
#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <map>
#include "SystemManager.h"
#include "TaskManager.h"
#include "WebUI.h"

SystemManager sys; ///< Global instance of the SystemManager.
TaskManager tasks; ///< Global instance of the TaskManager.
WebUI ui;          ///< Global instance of the WebUI manager.

AsyncWebServer server(80); ///< Global instance of the asynchronous web server.

/**
 * @brief Setup function, runs once on startup.
 *
 * Initializes Serial communication, LittleFS, SystemManager, TaskManager,
 * Wi-Fi in AP mode, and sets up all the web server routes.
 */
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting WASH-PRO-CORE...");

  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
  } else {
    Serial.println("LittleFS mounted");
  }

  sys.begin();
  tasks.begin();

  // Start as Access Point by default
  String apName = "WASH-PRO-CORE";
  apName += "-";
  uint64_t mac = ESP.getEfuseMac();
  char buf[8];
  sprintf(buf, "%04llX", (mac & 0xFFFF));
  apName += buf;
  WiFi.softAP(apName.c_str());
  IPAddress ip = WiFi.softAPIP();
  Serial.printf("AP started: %s @ %s\n", apName.c_str(), ip.toString().c_str());

  // Web UI
  ui.begin(&server);

  // API endpoint to get general system information.
  server.on("/api/info", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", sys.getInfoJSON());
  });

  // API endpoint to get the list of all tasks.
  server.on("/api/tasks", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", tasks.getTasksJSON());
  });

  // API endpoint to get a single task with its script (task gives out the script).
  server.on("/api/tasks/task", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("id")) {
      String id = request->getParam("id")->value();
      String json = tasks.getTaskWithScriptJSON(id);
      if (json.length() > 0) {
        request->send(200, "application/json", json);
      } else {
        request->send(404, "application/json", "{\"error\":\"task not found\"}");
      }
    } else {
      request->send(400, "application/json", "{\"error\":\"missing id\"}");
    }
  });

  // API endpoint to handle creating, renaming, and saving scripts for tasks.
  server.on("/api/tasks", HTTP_POST, [](AsyncWebServerRequest *request){
    String id = request->hasParam("id", true) ? request->getParam("id", true)->value() : "";
    String name = request->hasParam("name", true) ? request->getParam("name", true)->value() : "";
    String script = request->hasParam("script", true) ? request->getParam("script", true)->value() : "";

    // Logic: if 'script' is present, we are saving a script. Otherwise, creating/renaming
    if (request->hasParam("script", true)) { // This is a script save operation
      if (id.length() == 0) {
        request->send(400, "application/json", "{\"error\":\"missing id for script save\"}");
        return;
      }
      Serial.printf("Saving script for id=%s, name=%s, script_len=%u\n", id.c_str(), name.c_str(), script.length());
      bool ok = tasks.saveScript(id, name, script); // name might be empty if only script is updated
      request->send(ok ? 200 : 500, "application/json", ok ? "{\"ok\":true}" : "{\"error\":\"failed to save script\"}");
    } else if (name.length() > 0) { // This is a create or rename operation
      if (id.length() > 0) {
        // This is a rename operation
        Serial.printf("Renaming task id=%s to name=%s\n", id.c_str(), name.c_str());
        bool ok = tasks.saveScript(id, name, ""); // Use saveScript to update name, with empty script content
        request->send(ok ? 200 : 404, "application/json", ok ? "{\"ok\":true}" : "{\"error\":\"not found\"}");
      } else {
        // This is a create operation
        Serial.printf("Creating task name=%s\n", name.c_str());
        String newId = tasks.createTask(name);
        if (newId.length() > 0) {
          request->send(200, "application/json", tasks.getTaskJSON(newId));
        } else {
          request->send(500, "application/json", "{\"error\":\"failed to create task\"}");
        }
      }
    } else { // Neither name nor script provided
      request->send(400, "application/json", "{\"error\":\"no name or script provided\"}");
    }
  });

  // API endpoint to get the script content for a specific task.
  server.on("/api/tasks/script", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("id")) {
      String id = request->getParam("id")->value();
      String script = tasks.getScript(id);
      request->send(200, "text/plain", script);
    } else {
      request->send(400, "application/json", "{\"error\":\"missing id\"}");
    }
  });

  // API endpoint to delete a task.
  server.on("/api/tasks/delete", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("id", true)) {
      String id = request->getParam("id", true)->value();
      bool ok = tasks.deleteTask(id);
      request->send(ok ? 200 : 500, "application/json", ok ? "{\"ok\":true}" : "{\"error\":\"failed to delete\"}");
    } else {
      request->send(400, "application/json", "{\"error\":\"missing id\"}");
    }
  });

  // API endpoint to run a task's script.
  server.on("/api/tasks/run", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("id", true)) {
      String id = request->getParam("id", true)->value();
      bool ok = tasks.runTask(id);
      request->send(ok ? 200 : 500, "application/json", ok ? "{\"ok\":true}" : "{\"error\":\"failed to run\"}");
    } else {
      request->send(400, "application/json", "{\"error\":\"missing id\"}");
    }
  });

  // API endpoint to provide a list of built-in Lua functions for the script editor.
  server.on("/api/builtins", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", "[\"log\",\"setLED\",\"delay\",\"startTask\",\"stopTask\"]");
  });

  /**
   * @brief A lambda function to recursively delete a directory and its contents.
   * @param path The path of the directory to delete.
   * @note This is captured by the /api/files/delete endpoint.
   */
  std::function<void(String)> deleteRecursive = 
    [&](const String& path) {
      File root = LittleFS.open(path);
      if (!root || !root.isDirectory()) {
        return;
      }
      File file = root.openNextFile();
      while (file) {
        String entryPath = file.name(); // file.name() returns the full path
        if (file.isDirectory()) {
          deleteRecursive(entryPath);
        } else {
          LittleFS.remove(entryPath);
        }
        file.close(); // Ensure file handle is closed
        file = root.openNextFile();
      }
      LittleFS.rmdir(path);
  };

  // API endpoint to list files in a directory.
  server.on("/api/files", HTTP_GET, [](AsyncWebServerRequest *request){
    String path = "/";
    if (request->hasParam("path")) {
      path = request->arg("path");
    }
    if (!path.startsWith("/")) {
      path = "/" + path;
    }

    DynamicJsonDocument doc(4096);
    doc["path"] = path;
    JsonArray files = doc.createNestedArray("files");
    File root = LittleFS.open(path);
    if (root && root.isDirectory()) {
      File file = root.openNextFile();
      while (file) {
        JsonObject fileObj = files.createNestedObject();
        String fullPath = String(file.name());
        fileObj["name"] = fullPath.substring(fullPath.lastIndexOf('/') + 1);
        fileObj["size"] = file.size();
        fileObj["isDir"] = file.isDirectory();
        file.close();
        file = root.openNextFile();
      }
      root.close();
    }
    String out; serializeJson(doc, out);
    request->send(200, "application/json", out);
  });

  // API endpoint to delete a file or directory.
  server.on("/api/files/delete", HTTP_POST, [deleteRecursive](AsyncWebServerRequest *request){
    if (request->hasParam("path", true)) {
      String path = request->getParam("path", true)->value();
      if (path.startsWith("/") && LittleFS.exists(path)) {
        File f = LittleFS.open(path);
        if (!f) { request->send(500, "application/json", "{\"error\":\"failed to open path\"}"); return; }
        if (f.isDirectory()) deleteRecursive(path);
        else LittleFS.remove(path);
        request->send(200, "application/json", "{\"ok\":true}");
      } else {
        request->send(404, "application/json", "{\"error\":\"failed to remove\"}");
      }
    } else {
      request->send(400, "application/json", "{\"error\":\"missing path\"}");
    }
  });

  // API endpoint to rename a file.
  server.on("/api/files/rename", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("path", true) && request->hasParam("newName", true)) {
      String path = request->getParam("path", true)->value();
      String newName = request->getParam("newName", true)->value();
      String parentPath = path.substring(0, path.lastIndexOf('/'));
      String newPath = parentPath + "/" + newName;
      if (LittleFS.rename(path, newPath)) {
        request->send(200, "application/json", "{\"ok\":true}");
      } else {
        request->send(500, "application/json", "{\"error\":\"rename failed\"}");
      }
    } else {
      request->send(400, "application/json", "{\"error\":\"missing params\"}");
    }
  });

  // API endpoint to save content to a file.
  server.on("/api/files/save", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("path", true) && request->hasParam("content", true)) {
      String path = request->getParam("path", true)->value();
      String content = request->getParam("content", true)->value();
      File f = LittleFS.open(path, FILE_WRITE);
      if (f && f.print(content)) {
        f.close();
        request->send(200, "application/json", "{\"ok\":true}");
      } else {
        request->send(500, "application/json", "{\"error\":\"write failed\"}");
      }
    } else {
      request->send(400, "application/json", "{\"error\":\"missing params\"}");
    }
  });

  // API endpoint to get system settings.
  server.on("/api/system", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", sys.getSystemJSON());
  });

  // API endpoint to set the system language.
  server.on("/api/setlanguage", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("lang", true)) {
      String lang = request->getParam("lang", true)->value();
      sys.setLanguage(lang);
      Serial.printf("Language set via API: %s\n", lang.c_str());
      request->send(200, "application/json", "{\"ok\":true}");
    } else {
      request->send(400, "application/json", "{\"error\":\"no lang\"}");
    }
  });

  // API endpoint to set the license key.
  server.on("/api/setlicense", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("key", true)) {
      String key = request->getParam("key", true)->value();
      sys.setLicenseKey(key);
      Serial.printf("License key set via API.\n");
      request->send(200, "application/json", "{\"ok\":true}");
    } else {
      request->send(400, "application/json", "{\"error\":\"no key\"}");
    }
  });

  // API endpoint to get the list of available themes.
  server.on("/api/themes", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", "[\"gp_dark\",\"gp_light\",\"gp_gray\",\"gp_blue\",\"gp_new\",\"gp_modern\",\"gp_future\"]");
  });

  // API endpoint to set the system theme.
  server.on("/api/settheme", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("theme", true)) {
      String theme = request->getParam("theme", true)->value();
      sys.setTheme(theme);
      Serial.printf("Theme set via API: %s\n", theme.c_str());
      request->send(200, "application/json", "{\"ok\":true}");
    } else {
      request->send(400, "application/json", "{\"error\":\"no theme\"}");
    }
  });

  // API endpoint to set the auto-update preference.
  server.on("/api/autoupdate", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("enabled", true)) {
      String enabledStr = request->getParam("enabled", true)->value();
      bool enabled = (enabledStr == "true");
      sys.setAutoUpdate(enabled);
      request->send(200, "application/json", "{\"ok\":true}");
    } else {
      request->send(400, "application/json", "{\"error\":\"missing enabled param\"}");
    }
  });

  // Global handler for all file uploads (OTA and FS).
  server.onFileUpload([](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final){
    if (request->url() == "/api/upload/firmware") {
      sys.handleOTAUpload(request, filename, index, data, len, final);
    } else if (request->url() == "/api/upload/fs") {
      sys.handleFSUpload(request, filename, index, data, len, final);
    }
  });

  // Dummy POST handlers for upload endpoints to satisfy the frontend.
  server.on("/api/upload/firmware", HTTP_POST, [](AsyncWebServerRequest *request){ request->send(200); });
  server.on("/api/upload/fs", HTTP_POST, [](AsyncWebServerRequest *request){ request->send(200); });

  // API endpoint to configure and connect to a Wi-Fi network.
  server.on("/api/wifi", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("ssid", true) && request->hasParam("pass", true)) {
      String ssid = request->getParam("ssid", true)->value();
      String pass = request->getParam("pass", true)->value();
      sys.saveWiFiCredentials(ssid, pass);
      // try connect
      WiFi.begin(ssid.c_str(), pass.c_str()); 
      uint8_t tries = 0;
      while (WiFi.status() != WL_CONNECTED && tries < 30) {
        delay(200); 
        tries++;
      }
      if (WiFi.status() == WL_CONNECTED) {
        request->send(200, "application/json", "{\"ok\":true}");
      } else {
        request->send(500, "application/json", "{\"error\":\"connection failed\"}");
      }
    } else {
      request->send(400, "application/json", "{\"error\":\"missing params\"}");
    }
  });

  // API endpoint to schedule a system reboot.
  server.on("/api/reboot", HTTP_POST, [](AsyncWebServerRequest *request){
    String type = "hard";
    uint32_t delaySec = 0;
    if (request->hasParam("type", true)) type = request->getParam("type", true)->value();
    if (request->hasParam("delay", true)) delaySec = request->getParam("delay", true)->value().toInt();
    bool graceful = (type == "soft");
    sys.scheduleReboot(delaySec, graceful);
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.begin();
}

/**
 * @brief Main loop function.
 *
 * This function is empty as all operations (web server, tasks) are handled asynchronously.
 */
void loop() {
  // nothing to do; server and tasks run in background
}
