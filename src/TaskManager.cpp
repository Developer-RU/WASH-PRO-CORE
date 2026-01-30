#include "TaskManager.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <LittleFS.h>

void TaskManager::begin() {
  // ensure directories
  if (!LittleFS.exists("/tasks")) {
    LittleFS.mkdir("/tasks");
  }
  if (!LittleFS.exists("/scripts")) {
    LittleFS.mkdir("/scripts");
  }
}

String TaskManager::createTask(const String &name) {
  String id = String((uint32_t)millis());
  DynamicJsonDocument doc(512);
  doc["id"] = id;
  doc["name"] = name;
  doc["state"] = "stopped";
  doc["hasScript"] = false;
  String out; serializeJson(doc, out);
  File f = LittleFS.open(String("/tasks/") + id + ".json", FILE_WRITE);
  if (!f) return "";
  f.print(out);
  f.close();
  return id;
}

bool TaskManager::saveScript(const String &id, const String &name, const String &content) {
  bool ok = true;

  // Only write script file if content is provided
  if (content.length() > 0) {
    String path = String("/scripts/") + id + ".lua";
    Serial.printf("TaskManager::saveScript path=%s len=%u\n", path.c_str(), content.length());
    File f = LittleFS.open(path, FILE_WRITE);
    if (!f) { Serial.println("Failed to open script file for writing"); return false; }
    size_t written = f.print(content);
    f.close();
    Serial.printf("Wrote %u bytes to %s\n", written, path.c_str());
    if (written != content.length()) { Serial.println("Warning: script file write size mismatch"); ok = false; }
  }

  // update task's json file (name, hasScript flag)
  String tpath = String("/tasks/") + id + ".json";
  if (LittleFS.exists(tpath)) {
    File tf = LittleFS.open(tpath, FILE_READ);
    if (!tf) { Serial.printf("Failed to open task file for read: %s\n", tpath.c_str()); ok = false; }
    else {
      String s = tf.readString(); tf.close();
      DynamicJsonDocument doc(4096); // Increased size to handle larger scripts if they were stored here
      DeserializationError err = deserializeJson(doc, s);
      if (err) {
        Serial.printf("Failed to parse task json %s: %s\n", tpath.c_str(), err.c_str());
        // try to preserve minimal structure
        doc.clear();
        doc["id"] = id;
        doc["name"] = "";
      }
      if (name.length() > 0) doc["name"] = name;
      doc["hasScript"] = LittleFS.exists(String("/scripts/") + id + ".lua");
      String out; serializeJson(doc, out);
      File tfw = LittleFS.open(tpath, FILE_WRITE);
      if (!tfw) { Serial.printf("Failed to open task file for rewrite: %s\n", tpath.c_str()); ok = false; }
      else {
        size_t w2 = tfw.print(out);
        tfw.close();
        Serial.printf("Rewrote %u bytes to %s\n", w2, tpath.c_str());
        if (w2 == 0 && out.length() > 0) { Serial.println("Warning: 0 bytes written when updating task file"); ok = false; }
      }
    }
  } else {
    Serial.printf("Task file not found to update script: %s\n", tpath.c_str()); ok = false; }

  return ok;
}

bool TaskManager::runTask(const String &id) {
  String tpath = String("/tasks/") + id + ".json";
  if (!LittleFS.exists(tpath)) {
    Serial.printf("Cannot run task, not found: %s\n", tpath.c_str());
    return false;
  }
  File tf = LittleFS.open(tpath, FILE_READ);
  if (!tf) return false;
  String s = tf.readString();
  tf.close();

  DynamicJsonDocument doc(1024);
  deserializeJson(doc, s);
  doc["state"] = "running"; // Mark as running
  String out;
  serializeJson(doc, out);

  File tfw = LittleFS.open(tpath, FILE_WRITE);
  if (!tfw) return false;
  tfw.print(out);
  tfw.close();
  return true;
}

String TaskManager::getScript(const String &id) {
  String path = String("/scripts/") + id + ".lua";
  if (!LittleFS.exists(path)) return "";
  File f = LittleFS.open(path, FILE_READ);
  if (!f) return "";
  String s = f.readString(); f.close(); return s;
}

bool TaskManager::deleteTask(const String &id) {
  String tpath = String("/tasks/") + id + ".json";
  String spath = String("/scripts/") + id + ".lua";
  if (LittleFS.exists(tpath)) LittleFS.remove(tpath);
  if (LittleFS.exists(spath)) LittleFS.remove(spath);
  return true;
}

String TaskManager::getTaskJSON(const String &id) {
  String tpath = String("/tasks/") + id + ".json";
  if (!LittleFS.exists(tpath)) return "";
  File tf = LittleFS.open(tpath, FILE_READ);
  if (!tf) return "";
  String s = tf.readString(); tf.close(); return s;
}

String TaskManager::getTasksJSON() {
  DynamicJsonDocument doc(2048);
  UBaseType_t taskCount = uxTaskGetNumberOfTasks();
  doc["runningTasks"] = taskCount;
  JsonArray arr = doc.createNestedArray("tasks");
  File root = LittleFS.open("/tasks");
  if (root && root.isDirectory()) {
    File file = root.openNextFile();
    while (file) {
      if (!file.isDirectory()) {
        String content = file.readString();
        DynamicJsonDocument tdoc(1024);
        deserializeJson(tdoc, content);
        JsonObject obj = arr.createNestedObject();
        obj["id"] = String((const char*)tdoc["id"]);
        obj["name"] = String((const char*)tdoc["name"]);
        obj["state"] = String((const char*)tdoc["state"]);
        obj["hasScript"] = tdoc.containsKey("hasScript") && tdoc["hasScript"].as<bool>();
      }
      file = root.openNextFile();
    }
    root.close();
  }
  String out; serializeJson(doc, out); return out;
}
