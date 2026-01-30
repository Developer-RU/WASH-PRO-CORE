#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

class TaskManager {
public:
  void begin();
  bool runTask(const String &id);
  String getTasksJSON();
  String createTask(const String &name);
  bool saveScript(const String &id, const String &name, const String &content);
  String getScript(const String &id);
  bool deleteTask(const String &id);
  String getTaskJSON(const String &id);
};
