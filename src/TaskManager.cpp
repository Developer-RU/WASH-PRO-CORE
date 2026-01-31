/**
 * @file TaskManager.cpp
 * @author Masyukov Pavel
 * @brief Implementation of the TaskManager class for the WASH-PRO project.
 * @version 1.0.0
 * @see https://github.com/pavelmasyukov/WASH-PRO-CORE
 */
#include "TaskManager.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <LittleFS.h>

/**
 * @brief Initializes the TaskManager.
 */
void TaskManager::begin() {
  // ensure directories
  if (!LittleFS.exists("/tasks")) {
    LittleFS.mkdir("/tasks");
  }
  if (!LittleFS.exists("/scripts")) {
    LittleFS.mkdir("/scripts");
  }
}

/**
 * @brief Creates a new task with a given name.
 */
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

/**
 * @brief Saves a script for a task and/or updates its name.
 */
bool TaskManager::saveScript(const String &id, const String &name, const String &content) {
  bool ok = true;

  // Only write script file if content is provided
  if (content.length() > 0) {
    String path = String("/scripts/") + id + ".lua";
    Serial.printf("TaskManager::saveScript path=%s len=%u\n", path.c_str(), content.length());
    File f = LittleFS.open(path, "w");
    if (!f) {
      Serial.println("Failed to open script file for writing");
      return false;
    }
    size_t written = f.print(content);
    f.close();
    Serial.printf("Wrote %u bytes to %s\n", written, path.c_str());
    if (written != content.length()) {
      Serial.println("Warning: script file write size mismatch");
      ok = false;
    }
  }

  // update task's json file (name, hasScript flag)
  String tpath = String("/tasks/") + id + ".json";
  if (LittleFS.exists(tpath)) {
    File tf = LittleFS.open(tpath, "r");
    if (!tf) {
      Serial.printf("Failed to open task file for read: %s\n", tpath.c_str());
      ok = false;
    } else {
      String s = tf.readString();
      tf.close();
      DynamicJsonDocument doc(4096); // Increased size to handle larger scripts if they were stored here
      DeserializationError err = deserializeJson(doc, s);
      if (err) {
        Serial.printf("Failed to parse task json %s: %s\n", tpath.c_str(), err.c_str());
        // try to preserve minimal structure
        doc.clear();
        doc["id"] = id;
        doc["name"] = "";
      }
      if (name.length() > 0) {
        doc["name"] = name;
      }
      doc["hasScript"] = LittleFS.exists(String("/scripts/") + id + ".lua"); // Recalculate hasScript
      String out; serializeJson(doc, out);
      File tfw = LittleFS.open(tpath, "w");
      if (!tfw) {
        Serial.printf("Failed to open task file for rewrite: %s\n", tpath.c_str());
        ok = false;
      } else {
        size_t w2 = tfw.print(out);
        tfw.close();
        Serial.printf("Rewrote %u bytes to %s\n", w2, tpath.c_str());
        if (w2 == 0 && out.length() > 0) {
          Serial.println("Warning: 0 bytes written when updating task file"); ok = false;
        }
      }
    }
  } else {
    Serial.printf("Task file not found to update script: %s\n", tpath.c_str()); ok = false; }

  return ok;
}

/**
 * @brief Runs a specific task.
 */
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

/**
 * @brief Retrieves the script content for a given task.
 */
String TaskManager::getScript(const String &id) {
  String path = String("/scripts/") + id + ".lua";
  if (!LittleFS.exists(path)) return "";
  File f = LittleFS.open(path, FILE_READ);
  if (!f) return "";
  String s = f.readString(); f.close(); return s;
}

/**
 * @brief Deletes a task and its associated script.
 */
bool TaskManager::deleteTask(const String &id) {
  String tpath = String("/tasks/") + id + ".json";
  String spath = String("/scripts/") + id + ".lua";

  Serial.printf("--- Deleting Task ID: %s ---\n", id.c_str());

  bool taskJsonRemoved = false;
  if (LittleFS.exists(tpath)) {
    if (LittleFS.remove(tpath)) {
      taskJsonRemoved = true;
      Serial.printf("  > Deleting task file '%s'... SUCCESS\n", tpath.c_str());
    } else {
      Serial.printf("  > Deleting task file '%s'... FAILED\n", tpath.c_str());
    }
  } else {
    Serial.printf("  > Task file '%s' not found, skipping.\n", tpath.c_str());
    taskJsonRemoved = true; // File doesn't exist, so it's "removed"
  }

  bool scriptRemoved = false;
  if (LittleFS.exists(spath)) {
    scriptRemoved = LittleFS.remove(spath);
    Serial.printf("  > Deleting script file '%s'... %s\n", spath.c_str(), scriptRemoved ? "SUCCESS" : "FAILED");
  } else {
    Serial.printf("  > Script file '%s' not found, skipping.\n", spath.c_str());
    scriptRemoved = true; // File doesn't exist, so it's "removed"
  }

  bool finalTaskExists = LittleFS.exists(tpath);
  bool finalScriptExists = LittleFS.exists(spath);
  Serial.printf("  > Final check: Task file exists: %s, Script file exists: %s\n", finalTaskExists ? "yes" : "no", finalScriptExists ? "yes" : "no");
  return taskJsonRemoved && scriptRemoved;
}

/**
 * @brief Gets the JSON metadata for a single task.
 */
String TaskManager::getTaskJSON(const String &id) {
  String tpath = String("/tasks/") + id + ".json";
  if (!LittleFS.exists(tpath)) return "";
  File tf = LittleFS.open(tpath, FILE_READ);
  if (!tf) return "";
  String s = tf.readString(); tf.close(); return s;
}

/**
 * @brief Gets a JSON string representing all tasks.
 */
String TaskManager::getTasksJSON() {
  DynamicJsonDocument doc(2048);
  UBaseType_t taskCount = uxTaskGetNumberOfTasks();
  doc["runningTasks"] = taskCount;
  JsonArray arr = doc.createNestedArray("tasks");
  File root = LittleFS.open("/tasks");
  if (root && root.isDirectory()) {
    File file;
    while((file = root.openNextFile())){
      if (!file.isDirectory()) {
        DynamicJsonDocument tdoc(1024);
        DeserializationError error = deserializeJson(tdoc, file);
        if (error) {
          Serial.printf("Failed to parse task JSON from stream: %s\n", file.name());
          file.close(); // Close file on error
          continue;
        } else {
          JsonObject obj = arr.createNestedObject();
          obj["id"] = String((const char*)tdoc["id"]);
          obj["name"] = String((const char*)tdoc["name"]);
          obj["state"] = String((const char*)tdoc["state"]);
          obj["hasScript"] = tdoc.containsKey("hasScript") && tdoc["hasScript"].as<bool>();
        }
      }
      file.close(); // Ensure file is closed before opening the next one
    }
    root.close();
  }
  String out; serializeJson(doc, out); return out;
}
