/**
 * @file WebUI.h
 * @author Masyukov Pavel
 * @brief Definition of the WebUI class for the WASH-PRO project.
 * @version 1.0.0
 * @see https://github.com/pavelmasyukov/WASH-PRO-CORE
 */
#pragma once

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

/**
 * @class WebUI
 * @brief Manages the setup of the web user interface.
 */
class WebUI {
public:
  /**
   * @brief Initializes the web UI by setting up the static file server.
   * @param server A pointer to the AsyncWebServer instance.
   */
  void begin(AsyncWebServer *server);
};
