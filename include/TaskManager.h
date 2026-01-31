/**
 * @file TaskManager.h
 * @author Masyukov Pavel
 * @brief Definition of the TaskManager class for the WASH-PRO project.
 * @version 1.0.0
 * @see https://github.com/pavelmasyukov/WASH-PRO-CORE
 */
#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

/**
 * @class TaskManager
 * @brief Manages tasks and their associated scripts.
 *
 * This class is responsible for creating, deleting, and managing tasks.
 * It handles the storage of task metadata and Lua scripts in the LittleFS filesystem.
 */
class TaskManager {
public:
  /**
   * @brief Initializes the TaskManager.
   * Ensures that the required directories (/tasks, /scripts) exist in the filesystem.
   */
  void begin();

  /**
   * @brief Runs a specific task.
   * Currently, it marks the task's state as "running" in its metadata file.
   * @param id The unique ID of the task to run.
   * @return True if the task was found and marked as running, false otherwise.
   */
  bool runTask(const String &id);

  /**
   * @brief Gets a JSON string representing all tasks.
   * @return A JSON array of task objects.
   */
  String getTasksJSON();

  /**
   * @brief Creates a new task with a given name.
   * @param name The name for the new task.
   * @return The unique ID of the newly created task, or an empty string on failure.
   */
  String createTask(const String &name);

  /**
   * @brief Saves a script for a task and/or updates its name.
   * @param id The ID of the task.
   * @param name The new name for the task (can be empty if not changing).
   * @param content The Lua script content (can be empty if only renaming).
   * @return True on success, false on failure.
   */
  bool saveScript(const String &id, const String &name, const String &content);

  /**
   * @brief Retrieves the script content for a given task.
   * @param id The ID of the task.
   * @return The script content as a String, or an empty string if not found.
   */
  String getScript(const String &id);

  /**
   * @brief Deletes a task and its associated script.
   * @param id The ID of the task to delete.
   * @return True if both the task metadata and script were successfully deleted (or didn't exist), false otherwise.
   */
  bool deleteTask(const String &id);

  /**
   * @brief Gets the JSON metadata for a single task.
   * @param id The ID of the task.
   * @return A JSON string with the task's metadata, or an empty string if not found.
   */
  String getTaskJSON(const String &id);
};
