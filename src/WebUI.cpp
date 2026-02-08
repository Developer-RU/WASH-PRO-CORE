/**
 * @file WebUI.cpp
 * @author Masyukov Pavel
 * @brief Implementation of the WebUI class for the WASH-PRO project.
 * @version 1.0.0
 * @see https://github.com/pavelmasyukov/WASH-PRO-CORE
 */
#include "WebUI.h"
#include <LittleFS.h>

/**
 * @brief Initializes the web UI by setting up the static file server.
 * @param server A pointer to the AsyncWebServer instance.
 */
void WebUI::begin(AsyncWebServer *server) {
  // Serve static files from LittleFS root, but filter out API calls
  server->serveStatic("/", LittleFS, "/")
      .setDefaultFile("index.html")
      .setFilter([](AsyncWebServerRequest *request) {
        return !request->url().startsWith("/api/");
      });
}
