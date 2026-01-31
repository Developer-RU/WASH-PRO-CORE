[![WASH-PRO-CORE Logo](/logo.png)]([https://wash-pro.tech.dev/](https://github.com/Developer-RU/WASH-PRO-CORE))


# WASH-PRO-CORE

<p>
  <img src="version.svg" alt="Version 1.0.0">
  <img src="license.svg" alt="License MIT">
</p>

## Обзор проекта

**WASH-PRO-CORE** — это встраиваемая система на базе ESP32 с многофункциональным веб-интерфейсом для управления задачами, файлами и системными настройками. Проект разработан с акцентом на гибкость, удобство использования и расширяемость.

### Ключевые возможности

- **Точка доступа по умолчанию:** Устройство запускается в режиме точки доступа (AP) с SSID `WASH-PRO-CORE-xxxx` для первоначальной настройки.

### Веб-интерфейс

Современный адаптивный UI с боковым меню, который корректно отображается на десктопных и мобильных устройствах.

*   **Разделы меню:**
    *   **Главная:** Отображение основной системной информации.
    *   **Задачи:** Управление задачами и их скриптами.
    *   **Система:** Настройки языка, темы оформления и лицензионного ключа.
    *   **Сеть:** Настройка подключения к Wi-Fi.
    *   **Файлы:** Встроенный файловый менеджер.
    *   **Обновление:** Обновление прошивки и файловой системы.
    *   **Перезагрузка:** Управление перезагрузкой устройства.

*   **Многоязычная поддержка:** Интерфейс переведен на **16 языков**, включая русский, английский, немецкий и другие, с возможностью переключения "на лету".

*   **Темы оформления:** Поддержка **7 различных цветовых тем** для персонализации внешнего вида.

### Функциональность

*   **Управление задачами:** Создание, переименование и удаление задач. Для каждой задачи можно написать и сохранить **Lua-скрипт** с помощью встроенного редактора, который подсвечивает доступные функции.

*   **Файловый менеджер:** Полноценный менеджер для работы с файловой системой LittleFS. Позволяет просматривать структуру папок, переименовывать, удалять и редактировать текстовые файлы прямо в браузере.

*   **Системные настройки:**
    *   **Лицензионный ключ:** Предусмотрено поле для ввода и сохранения лицензионного ключа. Система также отображает текущий статус активности лицензии.
    *   **Обновление:** Страница для безопасного OTA (Over-the-Air) обновления прошивки и загрузки файлов в файловую систему. Доступна опция включения автоматического обновления.
    *   **Перезагрузка:** Возможность выполнить "мягкую" (программную) или "жёсткую" (аппаратную) перезагрузку с настраиваемой задержкой.

### API Эндпоинты

#### Система
- `GET /api/info` — информация о контроллере (серийный номер, память, лицензия).
- `GET /api/system` — системные настройки (версия ПО, язык, тема).
- `POST /api/setlanguage` — установить язык (параметр: `lang`).
- `POST /api/settheme` — установить тему оформления (параметр: `theme`).
- `POST /api/setlicense` — установить лицензионный ключ (параметр: `key`).
- `POST /api/autoupdate` — включить/выключить автообновление (параметр: `enabled`).
- `POST /api/reboot` — перезагрузка устройства (параметры: `type={soft,hard}`, `delay=sec`).

#### Задачи
- `GET /api/tasks` — получить список всех задач.
- `POST /api/tasks` — создать, переименовать задачу или сохранить для нее скрипт (параметры: `id`, `name`, `script`).
- `GET /api/tasks/script` — получить скрипт для задачи (параметр: `id`).
- `POST /api/tasks/delete` — удалить задачу (параметр: `id`).
- `POST /api/tasks/run` — запустить задачу (параметр: `id`).
- `GET /api/builtins` — получить список встроенных функций для редактора.

#### Файлы
- `GET /api/files` — получить список файлов и папок по пути (параметр: `path`).
- `POST /api/files/delete` — удалить файл или папку (параметр: `path`).
- `POST /api/files/rename` — переименовать файл (параметры: `path`, `newName`).
- `POST /api/files/save` — сохранить содержимое в файл (параметры: `path`, `content`).

#### Сеть и Обновления
- `POST /api/wifi` — сохранить учетные данные Wi-Fi и подключиться (параметры: `ssid`, `pass`).
- `POST /api/upload/firmware` — загрузить прошивку (OTA).
- `POST /api/upload/fs` — загрузить файл в файловую систему.

### Сборка и Загрузка

*   **Сборка проекта:**
    ```sh
    pio run
    ```
*   **Загрузка веб-ресурсов (папка `data`):**
    ```sh
    pio run --target uploadfs
    ```

### Лицензия

Этот проект лицензирован на условиях лицензии MIT.

```
MIT License

Copyright (c) 2024 Masyukov Pavel

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
