/**
 * @file main.cpp
 * @author Masyukov Pavel
 * @brief Main application file for the WASH-PRO project. Initializes all subsystems and web server endpoints.
 * @version 1.0.0
 * @see https://github.com/pavelmasyukov/WASH-PRO-CORE
 */
#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <AsyncTCP.h>
#define ASYNCWEBSERVER_REGEX
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "SystemManager.h"
#include "TaskManager.h"
#include "WebUI.h"

SystemManager sys; ///< Global instance of the SystemManager.
TaskManager tasks; ///< Global instance of the TaskManager.
WebUI ui;          ///< Global instance of the WebUI manager.

AsyncWebServer server(80); ///< Global instance of the asynchronous web server.
AsyncEventSource events("/events"); ///< Global instance for Server-Sent Events.

/**
 * @brief Setup function, runs once on startup.
 *
 * Initializes Serial communication, LittleFS, SystemManager, TaskManager,
 * Wi-Fi in AP mode, and sets up all the web server routes.
 */
void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); } // Wait for serial connection
  Serial.println(F("\n\n=== WASH-PRO-CORE Starting ==="));

  // Initialize LittleFS
  if (!LittleFS.begin(false)) { // false = do not format on fail
    Serial.println(F("LittleFS Mount Failed. It may need to be formatted."));
    // You might want to add a way to format the filesystem, e.g., on a specific boot condition.
  } else {
    Serial.println("LittleFS Mounted."); 
  }

  sys.begin();
  tasks.begin(&events); // Pass the events object to the task manager
  
  // Start as Access Point by default
  String apName = "WASH-PRO-CORE";
  uint64_t mac = ESP.getEfuseMac();
  char buf[8];
  snprintf(buf, sizeof(buf), "-%04llX", (uint16_t)(mac & 0xFFFF));
  apName += buf;
  WiFi.softAP(apName);
  IPAddress ip = WiFi.softAPIP();
  Serial.printf("AP started: %s @ %s\n", apName.c_str(), ip.toString().c_str());

  // CORS headers for all API responses
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

  // --- API Endpoints ---

  // API endpoint to get general system information.
  server.on("/api/info", HTTP_GET, [](AsyncWebServerRequest *request){
    // This is a simplified way to get running task count.
    // A more robust solution might involve TaskManager caching the count.
    DynamicJsonDocument doc(4096);
    deserializeJson(doc, tasks.getTasksJSON());
    int runningTasks = doc["runningTasks"] | 0;

    request->send(200, "application/json", sys.getInfoJSON(runningTasks));
  });

  // API endpoint to run a task's script.
  server.on("/api/tasks/run", HTTP_POST, [](AsyncWebServerRequest *request){
    if (!request->hasParam("id", true)) {
      request->send(400, "application/json", "{\"error\":\"missing id\"}");
      return;
    }
    String id = request->getParam("id", true)->value();      

    if (request->hasParam("id", true)) {
      String id = request->getParam("id", true)->value();      
      bool ok = tasks.runTask(id);
      request->send(ok ? 202 : 500, "application/json", ok ? "{\"ok\":true, \"message\":\"Task start requested.\"}" : "{\"error\":\"failed to start task\"}");
    } else {
      request->send(400, "application/json", "{\"error\":\"missing id\"}");
    }
  });

  // API endpoint to delete a task.
  server.on("/api/tasks/delete", HTTP_POST, [](AsyncWebServerRequest *request){
    if (!request->hasParam("id", true)) {
      request->send(400, "application/json", "{\"error\":\"missing id\"}");
      return;
    }
    String id = request->getParam("id", true)->value();
    if (request->hasParam("id", true)) {
      String id = request->getParam("id", true)->value();
      bool ok = tasks.deleteTask(id);
      request->send(ok ? 200 : 500, "application/json", ok ? "{\"ok\":true}" : "{\"error\":\"failed to delete\"}");
    } else {
      request->send(400, "application/json", "{\"error\":\"missing id\"}");
    } 
  });
  
  // API endpoint to get the list of all tasks.
  server.on("/api/tasks", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", tasks.getTasksJSON());
  });

  // API endpoint to get a single task with its script.
  // This uses a regex to capture the ID from the path, e.g., /api/tasks/12345.json
  // The negative lookahead prevents matching sub-paths like 'run' or 'delete'.
  server.on("^\\/api\\/tasks\\/(?!run$|delete$)([a-zA-Z0-9_.-]+)$", HTTP_GET, [](AsyncWebServerRequest *request) {
    String id = request->pathArg(0);
    if (!id.isEmpty()) {
      String json = tasks.getTaskWithScriptJSON(id);
      if (json.length() > 0) {
        request->send(200, "application/json", json);
      } else {
        request->send(404, "application/json", "{\"error\":\"task not found\"}");
      }
    }
  });

  // API endpoint to handle creating, renaming, and saving scripts for tasks.
  server.on("/api/tasks", HTTP_POST, [](AsyncWebServerRequest *request){
    String id = request->hasParam("id", true) ? request->getParam("id", true)->value() : "";
    String name = request->hasParam("name", true) ? request->getParam("name", true)->value() : "";
    String script = request->hasParam("script", true) ? request->getParam("script", true)->value() : "";

    // Logic: if 'script' is present, it's a save/update operation.
    if (request->hasParam("script", true)) {
      if (id.isEmpty()) {
        request->send(400, "application/json", "{\"error\":\"missing id for script save\"}");
        return;
      }
      Serial.printf("Saving script for id=%s, name=%s, script_len=%u\n", id.c_str(), name.c_str(), script.length());
      bool ok = tasks.saveScript(id, name, script); // name can be empty if only updating script content
      request->send(ok ? 200 : 500, "application/json", ok ? "{\"ok\":true}" : "{\"error\":\"failed to save script\"}");
    } else if (!name.isEmpty()) { // No script param, so it's a create or rename.
      if (!id.isEmpty()) {
        // Rename operation: use saveScript with empty script content to only update the name.
        Serial.printf("Renaming task id=%s to name=%s\n", id.c_str(), name.c_str());
        bool ok = tasks.saveScript(id, name, "");
        request->send(ok ? 200 : 404, "application/json", ok ? "{\"ok\":true}" : "{\"error\":\"not found\"}");
      } else {
        // Create operation
        Serial.printf("Creating task name=%s\n", name.c_str());
        String newId = tasks.createTask(name);
        if (!newId.isEmpty()) {
          request->send(200, "application/json", tasks.getTaskJSON(newId));
        } else {
          request->send(500, "application/json", "{\"error\":\"failed to create task\"}");
        }
      }
    } else {
      request->send(400, "application/json", "{\"error\":\"no name or script provided\"}");
    }
  });

  // API endpoint to provide a list of built-in Lua functions for the script editor.
  server.on("/api/builtins", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", "[\"log\",\"setLED\",\"delay\",\"startTask\",\"stopTask\"]");
  });

  /**
   * @brief A lambda function to recursively delete a directory and its contents.
   * @param path The path of the directory to delete.
   * @note This is captured by the /api/files/delete endpoint lambda.
   */
  std::function<void(String)> deleteRecursive = 
    [&](const String& path) {
      File root = LittleFS.open(path);
      if (!root || !root.isDirectory()) {
        return;
      }
      File file = root.openNextFile();
      while (file) {
        String entryPath = String(file.name()); // file.name() returns the full path
        if (file.isDirectory()) {
          deleteRecursive(entryPath);
        } else {
          LittleFS.remove(entryPath);
        }
        file.close();
        file = root.openNextFile();
      }
      root.close();
      LittleFS.rmdir(path);
  };

  // API endpoint to list files in a directory.
  server.on("/api/files", HTTP_GET, [](AsyncWebServerRequest *request){
    String path = "/";
    if (request->hasParam("path")) {
      path = request->arg("path");
    }
    if (!path.startsWith("/")) {
      path = "/" + path; // Ensure path is absolute
    }

    DynamicJsonDocument doc(4096);
    doc["path"] = path;
    JsonArray files = doc.createNestedArray("files");
    File root = LittleFS.open(path);
    if (root && root.isDirectory()) {
      File file = root.openNextFile();
      while (file) {
        JsonObject fileObj = files.createNestedObject();
        String fullPath(file.name());
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
    if (!request->hasParam("path", true)) {
      request->send(400, "application/json", "{\"error\":\"missing path\"}");
      return;
    }
    String path = request->getParam("path", true)->value();
    if (!path.startsWith("/") || !LittleFS.exists(path)) {
      request->send(404, "application/json", "{\"error\":\"file or directory not found\"}");
      return;
    }

    File f = LittleFS.open(path);
    if (!f) { request->send(500, "application/json", "{\"error\":\"failed to open path\"}"); return; }
    bool isDir = f.isDirectory();
    f.close();

    if (isDir) {
      deleteRecursive(path);
    } else {
      request->send(400, "application/json", "{\"error\":\"missing path\"}");
    }
  });

  // API endpoint to rename a file.
  server.on("/api/files/rename", HTTP_POST, [](AsyncWebServerRequest *request){
    if (!request->hasParam("path", true) || !request->hasParam("newName", true)) {
      request->send(400, "application/json", "{\"error\":\"missing params\"}");
      return;
    }
    String path = request->getParam("path", true)->value();
    String newName = request->getParam("newName", true)->value();
    if (!path.startsWith("/") || newName.isEmpty() || newName.indexOf('/') != -1) {
      request->send(400, "application/json", "{\"error\":\"invalid path or newName\"}");
      return;
    }
    String parentPath = path.substring(0, path.lastIndexOf('/'));
    String newPath = parentPath + "/" + newName;
    if (LittleFS.rename(path, newPath)) {
      request->send(200, "application/json", "{\"ok\":true}");
    } else {
      request->send(500, "application/json", "{\"error\":\"rename failed\"}");
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
      WiFi.begin(ssid, pass); 
      uint8_t tries = 0;
      while (WiFi.status() != WL_CONNECTED && tries < 30) {
        delay(500); 
        tries++;
      }
      if (WiFi.status() == WL_CONNECTED) {
        request->send(200, "application/json", "{\"ok\":true, \"ip\":\"" + WiFi.localIP().toString() + "\"}");
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

  // --- Final Setup ---
  // Add the SSE handler for real-time updates.
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("SSE Client reconnected! Last message ID: %s\n", client->lastId());
    }
    client->send("hello", "connected", millis(), 1000);
  });
  server.addHandler(&events);

  // Handle not found and CORS preflight
  server.onNotFound([](AsyncWebServerRequest *request) {
    if (request->method() == HTTP_OPTIONS) request->send(200);
    else request->send(404, "text/plain", "Not found");
  });

  // Add the Web UI handler. This must be last as it's a catch-all for static files.
  ui.begin(&server);

  server.begin();
}

/**
 * @brief Main loop function.
 *
 * This function is empty as all operations (web server, tasks) are handled asynchronously.
 */
void loop() {
  // Nothing to do here; server and tasks run in the background.
}
