#pragma once

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

class WebUI {
public:
  void begin(AsyncWebServer *server);
};
