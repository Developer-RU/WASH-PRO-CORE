#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

class TaskManager {
public:
  void begin();
  String getTasksJSON();
  String createTask(const String &name);
  bool saveScript(const String &id, const String &content);
  String getScript(const String &id);
  bool deleteTask(const String &id);
  String getTaskJSON(const String &id);
};
