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
#include <ESPAsyncWebServer.h> // For AsyncEventSource
#include <lua/lua.hpp>

// Constants for paths and sizes
static const char* TASKS_DIR = "/tasks";
static const char* SCRIPTS_DIR = "/scripts";
static const char* JSON_EXT = ".json";
static const char* LUA_EXT = ".lua";
static const size_t MAX_PATH_LEN = 64;
static const size_t LUA_TASK_STACK_SIZE = 8192;
static const UBaseType_t LUA_TASK_PRIORITY = 5;
 
// Pointer to the global task manager instance, set in begin()
static TaskManager* s_taskManager = nullptr;

/**
 * @brief Lua-callable function to log a message to the Serial port.
 * @param L The Lua state. Expects one string argument.
 * @return 0.
 */
static int l_log(lua_State *L) {
    const char *msg = luaL_checkstring(L, 1);
    if (msg) {
        Serial.printf("[LUA] %s\n", msg);
    }
    return 0;
}

/**
 * @brief Lua-callable function to control the built-in LED.
 * @param L The Lua state. Expects one boolean argument.
 * @return 0.
 */
static int l_setLED(lua_State *L) {
    bool on = lua_toboolean(L, 1);
#ifdef LED_BUILTIN
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, on ? HIGH : LOW);
#endif
    return 0;
}

/**
 * @brief Lua-callable function to delay execution.
 * @param L The Lua state. Expects one integer argument (milliseconds).
 * @return 0.
 */
static int l_delay(lua_State *L) {
    int ms = luaL_checkinteger(L, 1);
    if (ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(ms)); // Use FreeRTOS delay for better multitasking
    }
    return 0;
}

/**
 * @brief Lua-callable function to start another task.
 * @param L The Lua state. Expects one string argument (task ID).
 * @return 0.
 */
static int l_startTask(lua_State *L) {
    const char *id = luaL_checkstring(L, 1);
    if (id && s_taskManager) {
        String baseId = id;
        if (baseId.endsWith(".json")) {
            baseId.remove(baseId.length() - 5);
        }
        // Asynchronously start the other task. This is non-blocking.
        s_taskManager->runTask(baseId);
    }
    return 0;
}

/**
 * @brief Lua-callable function to stop a running task.
 * @param L The Lua state. Expects one string argument (task ID).
 * @return 0.
 */
static int l_stopTask(lua_State *L) {
    const char *id = luaL_checkstring(L, 1);
    if (id && s_taskManager) {
        String baseId = id;
        if (baseId.endsWith(".json")) {
            baseId.remove(baseId.length() - 5);
        }
        // This simply updates the state in the JSON file.
        s_taskManager->stopTask(baseId);
    }
    return 0;
}

/**
 * @brief A struct to pass arguments to the FreeRTOS task runner.
 */
struct TaskArgs {
  TaskManager* manager;
  String taskId;
};

/**
 * @brief The actual FreeRTOS task function that executes a Lua script.
 * This runs in the background, isolated from the main loop and web server.
 * @param pvParameters A pointer to a TaskArgs struct.
 */
static void taskRunner(void *pvParameters) {
  TaskArgs* args = (TaskArgs*)pvParameters;
  String baseId = args->taskId;
  TaskManager* manager = args->manager;

  Serial.printf("[TaskRunner %s] Started.\n", baseId.c_str());
  
  String scriptContent = manager->getScript(baseId);
  if (scriptContent.length() > 0) {
    Serial.printf("[TaskRunner %s] Running script...\n", baseId.c_str());
    lua_State *L = luaL_newstate();
    if (L) {
      luaL_openlibs(L);

      // Register C functions
      lua_register(L, "log", l_log);
      lua_register(L, "setLED", l_setLED);
      lua_register(L, "delay", l_delay);
      lua_register(L, "startTask", l_startTask);
      lua_register(L, "stopTask", l_stopTask);

      // Execute the script
      int result = luaL_dostring(L, scriptContent.c_str());
      if (result != LUA_OK) { // LUA_OK is 0
        const char *error = lua_tostring(L, -1);
        Serial.printf("Lua error in task %s: %s\n", baseId.c_str(), error);
      }
      lua_close(L);
    }
  } else {
    Serial.printf("[TaskRunner %s] No script found to run.\n", baseId.c_str());
  }

  // Set task state back to "stopped" after execution
  manager->stopTask(baseId);
  Serial.printf("[TaskRunner %s] State set to 'stopped'.\n", baseId.c_str());

  // Clean up and delete the FreeRTOS task
  delete args;
  vTaskDelete(NULL); // Delete self
}

/**
 * @brief Initializes the TaskManager.
 * @param events Pointer to the global AsyncEventSource for sending updates to clients.
 */
void TaskManager::begin(AsyncEventSource* events) {
  s_taskManager = this;
  _events = events;
  // Ensure task and script directories exist
  if (!LittleFS.exists("/tasks")) {
    LittleFS.mkdir("/tasks");
  }
  if (!LittleFS.exists("/scripts")) {
    LittleFS.mkdir("/scripts");
  }
}

/**
 * @brief Creates a new task with a given name.
 * @param name The desired name for the new task.
 * @return String The unique ID of the created task, or an empty string on failure.
 */
String TaskManager::createTask(const String &name) {
  String baseName = name;
  if (baseName.endsWith(".json")) {
    baseName.remove(baseName.length() - 5);
  }
  // Using millis() + random is more robust against collisions than just millis()
  String id = String(millis()) + String(random(0, 999));
  DynamicJsonDocument doc(512);
  doc["id"] = id;
  doc["name"] = baseName;
  doc["state"] = "stopped";
  doc["hasScript"] = false;

  String out; serializeJson(doc, out);
  char tpath[MAX_PATH_LEN];
  _getTaskPath(id, tpath, sizeof(tpath));
  File f = LittleFS.open(tpath, FILE_WRITE);
  if (!f) return "";
  f.print(out);
  f.close();
  return id;
}

/**
 * @brief Saves a script for a task and/or updates its name.
 * @param id The ID of the task to update.
 * @param name The new name for the task (can be empty if not changing).
 * @param content The Lua script content to save (can be empty).
 */
bool TaskManager::saveScript(const String &id, const String &name, const String &content) {
  String baseId = id;
  if (baseId.endsWith(".json")) {
    baseId.remove(baseId.length() - 5);
  }

  // 1. Update task's JSON file (name, hasScript flag)
  char tpath[MAX_PATH_LEN];
  _getTaskPath(baseId, tpath, sizeof(tpath));
  if (!LittleFS.exists(tpath)) {
    Serial.printf("Task file not found: %s\n", tpath);
    return false;
  }

  DynamicJsonDocument doc(1024);
  File taskFileRead = LittleFS.open(tpath, "r");
  if (!taskFileRead) {
    Serial.printf("Failed to open task file for read: %s\n", tpath);
    return false;
  }
  DeserializationError err = deserializeJson(doc, taskFileRead);
  taskFileRead.close();
  if (err) {
    Serial.printf("Failed to parse task json %s: %s. Re-creating.\n", tpath, err.c_str());
    doc.clear();
    doc["id"] = baseId;
  }

  if (!name.isEmpty()) {
    doc["name"] = name;
  }

  // 2. Always write script file (including empty content, so "Save" clears or updates correctly)
  char spath[MAX_PATH_LEN];
  snprintf(spath, sizeof(spath), "%s/%s%s", SCRIPTS_DIR, baseId.c_str(), LUA_EXT);
  Serial.printf("TaskManager::saveScript path=%s len=%u\n", spath, content.length());

  File scriptFile = LittleFS.open(spath, "w");
  if (!scriptFile) {
    Serial.println("Failed to open script file for writing");
    return false;
  }
  size_t written = scriptFile.print(content);
  scriptFile.close();
  if (written != content.length()) {
    Serial.println("Warning: script file write size mismatch");
    // Continue, as metadata update is also important
  }

  doc["hasScript"] = LittleFS.exists(spath); // Recalculate hasScript

  // 3. Write updated JSON back to file
  File taskFileWrite = LittleFS.open(tpath, "w");
  if (!taskFileWrite) {
    Serial.printf("Failed to open task file for rewrite: %s\n", tpath);
    return false;
  }
  size_t bytesWritten = serializeJson(doc, taskFileWrite);
  taskFileWrite.close();
  if (bytesWritten == 0 && doc.size() > 0) {
    Serial.println("Warning: 0 bytes written when updating task file");
    return false;
  }
  Serial.printf("Rewrote %u bytes to %s\n", bytesWritten, tpath);

  sendUpdate(); // Notify UI of potential name change or script availability change
  return true;
}

/**
 * @brief Asynchronously starts a task by creating a new FreeRTOS task to run its script.
 */
bool TaskManager::runTask(const String &id) {
  Serial.printf("[runTask] Request to run task ID: %s\n", id.c_str());

  String baseId = id;
  if (baseId.endsWith(".json")) {
    baseId.remove(baseId.length() - 5);
  }

  char tpath[MAX_PATH_LEN];
  _getTaskPath(baseId, tpath, sizeof(tpath));

  if (!LittleFS.exists(tpath)) {
    Serial.printf("[runTask] FAILED: Task file not found: %s\n", tpath);
    return false;
  }

  // 1. Check state and update it to "running"
  DynamicJsonDocument doc(1024);
  File tf = LittleFS.open(tpath, FILE_READ);
  if (!tf) {
    Serial.printf("[runTask] FAILED: Could not open task file for reading: %s\n", tpath);
    return false;
  }
  deserializeJson(doc, tf);
  tf.close();
  String state = doc["state"];
  if (state == "running") {
    Serial.printf("[runTask] FAILED: Task %s is already running.\n", baseId.c_str());
    return false;
  }
  doc["state"] = "running";
  String out;
  serializeJson(doc, out);
  File tfw = LittleFS.open(tpath, FILE_WRITE);
  if (tfw) { tfw.print(out); tfw.close(); }

  // 2. Create arguments for the FreeRTOS task
  TaskArgs* args = new TaskArgs{this, baseId};

  // 3. Create and start the FreeRTOS task
  char taskName[configMAX_TASK_NAME_LEN];
  snprintf(taskName, sizeof(taskName), "lua-%s", baseId.c_str());
  BaseType_t result = xTaskCreate(taskRunner, taskName, LUA_TASK_STACK_SIZE, (void*)args, LUA_TASK_PRIORITY, NULL);

  if (result != pdPASS) {
    Serial.printf("[runTask] FAILED: Could not create FreeRTOS task for %s.\n", baseId.c_str());
    delete args; // Clean up arguments
    // Revert state to "stopped"
    doc["state"] = "stopped";
    String out;
    serializeJson(doc, out);
    File tfw = LittleFS.open(tpath, FILE_WRITE);
    if (tfw) {
      tfw.print(out);
      tfw.close();
    }
    return false;
  }

  Serial.printf("[runTask] SUCCESS: FreeRTOS task created for %s.\n", baseId.c_str());
  sendUpdate(); // Notify UI that task is now running
  return true;
}

/**
 * @brief Stops a specific task by updating its state to "stopped".
 */
bool TaskManager::stopTask(const String &id) {
  String baseId = id;
  if (baseId.endsWith(".json")) {
    baseId.remove(baseId.length() - 5);
  }
  char tpath[MAX_PATH_LEN];
  _getTaskPath(baseId, tpath, sizeof(tpath));

  if (!LittleFS.exists(tpath)) {
    Serial.printf("Cannot stop task, not found: %s\n", tpath);
    return false;
  }
  File tf = LittleFS.open(tpath, FILE_READ);
  if (!tf) return false;
  
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, tf);
  tf.close();
  
  doc["state"] = "stopped"; // Mark as stopped
  String out;
  serializeJson(doc, out);

  File tfw = LittleFS.open(tpath, FILE_WRITE);
  if (!tfw) return false;
  tfw.print(out);
  tfw.close();
  sendUpdate(); // Notify UI that task has stopped
  return true;
}

/**
 * @brief Retrieves the script content for a given task.
 */
String TaskManager::getScript(const String &id) {
  String baseId = id;
  if (baseId.endsWith(".json")) {
    baseId.remove(baseId.length() - 5);
  }
  char path[MAX_PATH_LEN];
  snprintf(path, sizeof(path), "%s/%s%s", SCRIPTS_DIR, baseId.c_str(), LUA_EXT);

  if (!LittleFS.exists(path)) return "";
  File f = LittleFS.open(path, FILE_READ);
  if (!f) return "";
  String s = f.readString();
  f.close();
  return s;
}

/**
 * @brief Deletes a task and its associated script.
 */
bool TaskManager::deleteTask(const String &id) {
  if (id.isEmpty()) {
    Serial.println("Error: deleteTask called with an empty ID.");
    return false;
  }

  String baseId = id; // The ID might come with ".json" extension.
  if (baseId.endsWith(".json")) {
    baseId.remove(baseId.length() - 5);
  }

  char tpath[MAX_PATH_LEN];
  _getTaskPath(baseId, tpath, sizeof(tpath));

  char spath[MAX_PATH_LEN];
  snprintf(spath, sizeof(spath), "%s/%s%s", SCRIPTS_DIR, baseId.c_str(), LUA_EXT);

  Serial.printf("--- Deleting Task ID: %s ---\n", baseId.c_str());
  bool scriptFileRemoved = false;
  if (LittleFS.exists(spath)) {
    scriptFileRemoved = LittleFS.remove(spath);
    Serial.printf("Script file '%s' %s.\n", spath, scriptFileRemoved ? "deleted" : "failed to delete");
  } else {
    Serial.printf("Script file '%s' not found, skipping.\n", spath);
    scriptFileRemoved = true; // Not an error if it doesn't exist
  }

  bool taskFileRemoved = false;
  if (LittleFS.exists(tpath)) {
    taskFileRemoved = LittleFS.remove(tpath);
    Serial.printf("Task file '%s' %s.\n", tpath, taskFileRemoved ? "deleted" : "failed to delete");
  } else {
    Serial.printf("Task file '%s' not found, skipping.\n", tpath);
    taskFileRemoved = true;
  }

  bool success = taskFileRemoved && scriptFileRemoved;
  if (success) sendUpdate();
  return success;
}

/**
 * @brief Gets the JSON metadata for a single task.
 */
String TaskManager::getTaskJSON(const String &id) {
  String baseId = id;
  if (baseId.endsWith(".json")) {
    baseId.remove(baseId.length() - 5);
  }
  char tpath[MAX_PATH_LEN];
  _getTaskPath(baseId, tpath, sizeof(tpath));

  if (!LittleFS.exists(tpath)) return "";
  File tf = LittleFS.open(tpath, FILE_READ);
  if (!tf) return "";
  String s = tf.readString();
  tf.close();
  return s;
}

/**
 * @brief Gets a single task as JSON including its script content.
 */
String TaskManager::getTaskWithScriptJSON(const String &id) {
  String baseId = id;
  if (baseId.endsWith(".json")) {
    baseId.remove(baseId.length() - 5);
  }
  String raw = getTaskJSON(baseId);
  if (raw.isEmpty()) return "";

  DynamicJsonDocument meta(1024);
  DeserializationError err = deserializeJson(meta, raw);
  if (err) return "";

  String scriptContent = getScript(baseId);
  
  size_t cap = JSON_OBJECT_SIZE(4) + meta["id"].as<String>().length() + meta["name"].as<String>().length() + 10 + scriptContent.length() + 512;
  DynamicJsonDocument doc(cap);
  if (!doc.capacity()) return "";  // allocation failed
  doc["id"] = meta["id"].as<String>();
  doc["name"] = meta["name"].as<String>();
  doc["state"] = meta["state"].as<String>();
  doc["hasScript"] = meta.containsKey("hasScript") && meta["hasScript"].as<bool>();
  doc["script"] = scriptContent;
  String out;
  serializeJson(doc, out);
  return out;
}

/**
 * @brief Gets a JSON string representing all tasks.
 * @return String A JSON object containing an array of all tasks and the count of running tasks.
 */
String TaskManager::getTasksJSON() {
  DynamicJsonDocument doc(4096);
  int runningCount = 0;
  File root = LittleFS.open(TASKS_DIR);
  if (root) {
    File file = root.openNextFile();
    while(file){
      if (!file.isDirectory()) {
        DynamicJsonDocument tdoc(1024);
        deserializeJson(tdoc, file); // Ignore errors for robustness
        const char* state = tdoc["state"] | "stopped";
        if (strcmp(state, "running") == 0) {
          runningCount++;
        }
        // Add to a temporary list to be sorted
      }
      file.close();
      file = root.openNextFile();
    }
    root.close();
  }

  JsonArray arr = doc.createNestedArray("tasks");
  root = LittleFS.open(TASKS_DIR);
  if (root) {
    File file = root.openNextFile();
    while(file) {
      if (!file.isDirectory()) {
        JsonObject obj = arr.createNestedObject();
        deserializeJson(obj, file);
      }
      file.close();
      file = root.openNextFile();
    }
    root.close();
  }

  doc["runningTasks"] = runningCount;
  String out;
  serializeJson(doc, out);
  return out;
}

/**
 * @brief Sends a "tasks_update" event to all connected web clients.
 */
void TaskManager::sendUpdate() {
  if (_events) {
    _events->send("tasks_update", "update", millis());
  }
}

/**
 * @brief Helper function to get the full path for a task's JSON file.
 */
void TaskManager::_getTaskPath(const String& id, char* buffer, size_t len) {
    snprintf(buffer, len, "%s/%s%s", TASKS_DIR, id.c_str(), JSON_EXT);
}

/**
 * @brief Gets the count of currently running tasks.
 * @return int The number of tasks with state "running".
 */
int TaskManager::getRunningTaskCount() {
    // This is a simplified version of the logic in getTasksJSON().
    // For performance, it could be cached if called frequently.
    return 0; // Placeholder, full implementation would iterate files.
}
