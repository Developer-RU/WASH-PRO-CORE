#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <map>
#include "SystemManager.h"
#include "TaskManager.h"
#include "WebUI.h"

SystemManager sys;
TaskManager tasks;
WebUI ui;
AsyncWebServer server(80);
static std::map<uint32_t, String> requestBodies;

void notFound(AsyncWebServerRequest *request) {
  // For API endpoints, return 404; for all other client routes, serve index.html (SPA fallback)
  String url = request->url();
  if (url.startsWith("/api/")) {
    // log unexpected API requests for debugging
    Serial.printf("notFound for API path: %s method=%d contentLen=%u\n", url.c_str(), request->method(), request->contentLength());
    if (LittleFS.exists("/404.html")) {
      request->send(LittleFS, "/404.html", "text/html");
    } else {
      request->send(404, "text/plain", "Not found");
    }
    return;
  }

  // Serve SPA entry point for client routes
  if (LittleFS.exists("/index.html")) {
    request->send(LittleFS, "/index.html", "text/html");
  } else if (LittleFS.exists("/404.html")) {
    request->send(LittleFS, "/404.html", "text/html");
  } else {
    request->send(404, "text/plain", "Not found");
  }
}

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

  server.on("/api/info", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", sys.getInfoJSON());
  });

  server.on("/api/tasks", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", tasks.getTasksJSON());
  });

  // Create or update task (add diagnostics) with JSON/raw-body fallback
  server.on("/api/tasks", HTTP_POST,
    [](AsyncWebServerRequest *request){
      bool hasName = request->hasParam("name", true);
      Serial.printf("/api/tasks POST called: hasName=%d contentLen=%u ctype=%s\n", hasName, request->contentLength(), request->contentType().c_str());
      String name = "";
      String id = "";

      if (hasName) {
        name = request->getParam("name", true)->value();
        if (request->hasParam("id", true)) id = request->getParam("id", true)->value();
      } else if (request->contentLength()>0) {
        // try buffered body (populated by onBody) - supports JSON payloads
        uint32_t key = (uint32_t)request;
        if (requestBodies.count(key)) {
          String body = requestBodies[key];
          requestBodies.erase(key);
          Serial.printf("/api/tasks POST using buffered body len=%u\n", body.length());
          if (request->contentType().startsWith("application/json")) {
            DynamicJsonDocument doc(2048);
            auto err = deserializeJson(doc, body);
            if (!err) {
              if (!doc["name"].isNull()) name = String((const char*)doc["name"]);
              if (!doc["id"].isNull()) id = String((const char*)doc["id"]);
            } else {
              Serial.printf("Failed to parse JSON body for /api/tasks: %s\n", err.c_str());
            }
          } else {
            // not JSON - attempt to extract name parameter naively (urlencoded fallback)
            int n = body.indexOf("name=");
            if (n >= 0) {
              int amp = body.indexOf('&', n);
              String v = (amp>n) ? body.substring(n+5, amp) : body.substring(n+5);
              name = v; // may be percent-encoded
            }
            int ni = body.indexOf("id=");
            if (ni >= 0) {
              int amp2 = body.indexOf('&', ni);
              String v2 = (amp2>ni) ? body.substring(ni+3, amp2) : body.substring(ni+3);
              id = v2;
            }
          }
        }
      }

      if (name.length()) {
        if (id.length()) {
          Serial.printf("Update task id=%s name=%s\n", id.c_str(), name.c_str());
          // update existing task file
          String tpath = String("/tasks/") + id + ".json";
          if (LittleFS.exists(tpath)) {
            File tf = LittleFS.open(tpath, FILE_READ);
            if (tf) {
              String s = tf.readString(); tf.close();
              DynamicJsonDocument doc(1024);
              deserializeJson(doc, s);
              doc["name"] = name;
              String out; serializeJson(doc, out);
              File tfw = LittleFS.open(tpath, FILE_WRITE);
              if (tfw) { tfw.print(out); tfw.close(); request->send(200, "application/json", out); return; }
            }
          }
          request->send(404, "application/json", "{\"error\":\"not found\"}");
          return;
        }
        Serial.printf("Create task name=%s\n", name.c_str());
        String newid = tasks.createTask(name);
        String tjson = tasks.getTaskJSON(newid);
        if (tjson.length()) request->send(200, "application/json", tjson);
        else request->send(500, "application/json", "{\"error\":\"failed\"}");
      } else {
        // give more diagnostics when possible
        String detail = "no name";
        if (request->contentLength()>0) detail = String("body length=") + request->contentLength() + " ctype=" + request->contentType();
        request->send(400, "application/json", String("{\"error\":\"no name\",\"detail\":\"") + detail + "\"}");
      }
    },
    NULL,
    // onBody: buffer raw body so JSON can be parsed in the handler
    [](AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total){
      uint32_t key = (uint32_t)request;
      if (index==0) requestBodies[key] = "";
      requestBodies[key].concat((const char*)data, len);
      Serial.printf("/api/tasks onBody index=%u len=%u total=%u\n", index, len, total);
    }
  );

  // Save script for task with robust multi-part/JSON/raw body support
  server.on("/api/tasks/script", HTTP_POST,
    [](AsyncWebServerRequest *request){
      bool hasId = request->hasParam("id", true) || request->hasParam("id");
      bool hasScript = request->hasParam("script", true) || request->hasParam("script");
      Serial.printf("/api/tasks/script called: hasId=%d hasScript=%d contentLen=%u ctype=%s\n", hasId, hasScript, request->contentLength(), request->contentType().c_str());

      String id;
      String script;

      if (hasId) {
        if (request->hasParam("id", true)) id = request->getParam("id", true)->value();
        else id = request->getParam("id")->value();
      }

      if (hasScript) {
        if (request->hasParam("script", true)) script = request->getParam("script", true)->value();
        else script = request->getParam("script")->value();
        Serial.printf("Received param script length=%u\n", script.length());
      } else {
        // fallback to buffered body (from onBody) if present
        uint32_t key = (uint32_t)request;
        if (requestBodies.count(key)) {
          script = requestBodies[key];
          Serial.printf("Using buffered body length=%u\n", script.length());
          requestBodies.erase(key);
        } else if (request->contentLength()>0) {
          Serial.println("Attempting to read raw body as script");
          String raw;
          if (request->hasParam("plain", true)) raw = request->getParam("plain", true)->value();
          else raw = request->arg((size_t)0);
          Serial.printf("Raw body length=%u\n", raw.length());
          // If JSON, parse id/script fields
          if (raw.startsWith("{")) {
            DynamicJsonDocument doc(8192);
            auto err = deserializeJson(doc, raw);
            if (!err) {
              if (!doc["id"].isNull()) id = String((const char*)doc["id"]);
              if (!doc["script"].isNull()) script = String((const char*)doc["script"]);
            }
          } else {
            script = raw;
          }
        }
      }

      if (id.length()==0) { Serial.println("No id provided"); request->send(400,"application/json","{\"error\":\"missing id\"}"); return; }
      if (script.length()==0){ Serial.println("No script content provided"); request->send(400,"application/json","{\"error\":\"empty script\"}"); return; }

      Serial.printf("Saving script for %s length=%u\n", id.c_str(), script.length());
      bool ok = tasks.saveScript(id, "", script);
      if (ok) request->send(200, "application/json", "{\"ok\":true}");
      else request->send(500, "application/json", "{\"error\":\"failed to write\"}");
    },
    NULL,
    // onBody callback: accumulate body chunks into requestBodies map
    [](AsyncWebServerRequest* request, uint8_t *data, size_t len, size_t index, size_t total){
      uint32_t key = (uint32_t)request;
      if (index==0) requestBodies[key] = "";
      requestBodies[key].concat((const char*)data, len);
      Serial.printf("/api/tasks/script onBody index=%u len=%u total=%u\n", index, len, total);
    }
  );

  // Get script content (query param: id)
  server.on("/api/tasks/script", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("id")) {
      String id = request->getParam("id")->value();
      String script = tasks.getScript(id);
      request->send(200, "text/plain", script);
    } else {
      request->send(400, "application/json", "{\"error\":\"missing id\"}");
    }
  });

  // Delete task
  server.on("/api/tasks/delete", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("id", true)) {
      String id = request->getParam("id", true)->value();
      tasks.deleteTask(id);
      request->send(200, "application/json", "{\"ok\":true}");
    } else {
      request->send(400, "application/json", "{\"error\":\"missing id\"}");
    }
  });

  // Run a task's script (non-blocking request, execution not implemented yet)
  server.on("/api/tasks/run", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("id", true)) {
      String id = request->getParam("id", true)->value();
      Serial.printf("Run requested for task: %s\n", id.c_str());
      bool ok = tasks.runTask(id);
      request->send(ok ? 200 : 500, "application/json", ok ? "{\"ok\":true}" : "{\"error\":\"failed to run\"}");
    } else {
      request->send(400, "application/json", "{\"error\":\"missing id\"}");
    }
  });

  // Built-in functions list for editor hints
  server.on("/api/builtins", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", "[\"log\",\"setLED\",\"delay\",\"startTask\",\"stopTask\"]");
  });

  server.on("/api/system", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", sys.getSystemJSON());
  });

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

  // Theme endpoints
  server.on("/api/themes", HTTP_GET, [](AsyncWebServerRequest *request){
    // List of available themes (files under /themes/*.css)
    // Keep in sync with data/themes
    request->send(200, "application/json", "[\"gp_dark\",\"gp_light\",\"gp_gray\",\"gp_blue\",\"gp_new\",\"gp_modern\",\"gp_future\"]");
  });

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

  // OTA upload
  server.onFileUpload([](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final){
    if (request->url() == "/api/upload/firmware") {
      sys.handleOTAUpload(request, filename, index, data, len, final);
    } else if (request->url() == "/api/upload/fs") {
      sys.handleFSUpload(request, filename, index, data, len, final);
    }
  });

  server.on("/api/upload/firmware", HTTP_POST, [](AsyncWebServerRequest *request){ request->send(200); });
  server.on("/api/upload/fs", HTTP_POST, [](AsyncWebServerRequest *request){ request->send(200); });

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
        request->send(200, "application/json", "{\"ok\":true}\n");
      } else {
        request->send(200, "application/json", "{\"ok\":false}\n");
      }
    } else {
      request->send(400, "application/json", "{\"error\":\"missing params\"}");
    }
  });

  server.on("/api/reboot", HTTP_POST, [](AsyncWebServerRequest *request){
    String type = "hard";
    uint32_t delaySec = 0;
    if (request->hasParam("type", true)) type = request->getParam("type", true)->value();
    if (request->hasParam("delay", true)) delaySec = request->getParam("delay", true)->value().toInt();
    bool graceful = (type == "soft");
    sys.scheduleReboot(delaySec, graceful);
    request->send(200, "application/json", "{\"ok\":true}");
  });

  server.onNotFound(notFound);

  server.begin();
}

void loop() {
  // nothing to do; server and tasks run in background
}
