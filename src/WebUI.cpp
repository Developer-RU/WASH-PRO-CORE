#include "WebUI.h"
#include <LittleFS.h>

void WebUI::begin(AsyncWebServer *server) {
  // serve static files from LittleFS root
  server->serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
}
