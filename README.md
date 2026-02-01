[![WASH-PRO-CORE Logo](/images/logo.png)]([https://wash-pro.tech.dev/](https://github.com/Developer-RU/WASH-PRO-CORE))


# WASH-PRO-CORE

<p>
  <img src="/images/version.svg" alt="Version 1.0.0">
  <img src="/images/license.svg" alt="License MIT">
</p>

## Project Overview

**WASH-PRO-CORE** is an embedded system based on ESP32 with a multifunctional web interface for managing tasks, files, and system settings. The project is designed with a focus on flexibility, ease of use, and extensibility.

> [!IMPORTANT]
> **This project is under active development.**
> It is being created as an open-source alternative firmware for WASH-PRO controllers and is provided "as-is" without any guarantees. We encourage community participation and welcome developers to use this project as a foundation for their own custom firmware.

### Key Features

- **Default Access Point:** The device starts in Access Point (AP) mode with the SSID `WASH-PRO-CORE-xxxx` for initial setup.

### Web Interface

A modern, responsive UI with a sidebar menu that displays correctly on both desktop and mobile devices.

*   **Menu Sections:**
    *   **Home:** Displays main system information.
    *   **Tasks:** Manage tasks and their scripts.
    *   **System:** Settings for language, theme, and license key.
    *   **Network:** Wi-Fi connection setup.
    *   **Files:** Built-in file manager.
    *   **Update:** Firmware and filesystem updates.
    *   **Reboot:** Device reboot management.

*   **Multilingual Support:** The interface is translated into **16 languages**, including Russian, English, German, and others, with on-the-fly switching.

*   **Themes:** Support for **7 different color themes** to personalize the look and feel.

### Functionality

*   **Task Management:** Create, rename, and delete tasks. For each task, you can write and save a **Lua script** using the built-in editor, which highlights available functions.

*   **File Manager:** A full-featured manager for working with the LittleFS filesystem. It allows you to browse the folder structure, rename, delete, and edit text files directly in the browser.

*   **System Settings:**
    *   **License Key:** A field is provided for entering and saving a license key. The system also displays the current license activity status.
    *   **Update:** A page for secure OTA (Over-the-Air) firmware updates and uploading files to the filesystem. An option for enabling automatic updates is available.
    *   **Reboot:** The ability to perform a "soft" (software) or "hard" (hardware) reboot with a configurable delay.

### API Endpoints

#### System
- `GET /api/info` — Controller information (serial number, memory, license).
- `GET /api/system` — System settings (software version, language, theme).
- `POST /api/setlanguage` — Set language (parameter: `lang`).
- `POST /api/settheme` — Set theme (parameter: `theme`).
- `POST /api/setlicense` — Set license key (parameter: `key`).
- `POST /api/autoupdate` — Enable/disable auto-update (parameter: `enabled`).
- `POST /api/reboot` — Reboot the device (parameters: `type={soft,hard}`, `delay=sec`).

#### Tasks
- `GET /api/tasks` — Get a list of all tasks.
- `POST /api/tasks` — Create, rename a task, or save a script for it (parameters: `id`, `name`, `script`).
- `GET /api/tasks/script` — Get the script for a task (parameter: `id`).
- `POST /api/tasks/delete` — Delete a task (parameter: `id`).
- `POST /api/tasks/run` — Run a task (parameter: `id`).
- `GET /api/builtins` — Get a list of built-in functions for the editor.

#### Files
- `GET /api/files` — Get a list of files and folders by path (parameter: `path`).
- `POST /api/files/delete` — Delete a file or folder (parameter: `path`).
- `POST /api/files/rename` — Rename a file (parameters: `path`, `newName`).
- `POST /api/files/save` — Save content to a file (parameters: `path`, `content`).

#### Network & Updates
- `POST /api/wifi` — Save Wi-Fi credentials and connect (parameters: `ssid`, `pass`).
- `POST /api/upload/firmware` — Upload firmware (OTA).
- `POST /api/upload/fs` — Upload a file to the filesystem.

### Build and Upload

*   **Build the project:**
    ```sh
    pio run
    ```
*   **Upload web resources (the `data` folder):**
    ```sh
    pio run --target uploadfs
    ```
