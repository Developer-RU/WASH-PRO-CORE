# WASH-PRO-CORE

Проект: ESP32 + Adaptive Web UI

Особенности:
- Запускается в режиме точки доступа (AP) по умолчанию: SSID "WASH-PRO-CORE-xxxx"
- Адаптивный веб-интерфейс (левое выдвижное меню): Главная, Задачи, Система, Сеть, Перезагрузка
- API:
  - GET /api/info — информация о контроллере (серийный, память, лицензия, скрипты)
  - GET /api/tasks — список задач (имя, статус)
  - GET /api/system — информация о ПО и текущем языке
  - POST /api/setlanguage — изменить язык (form param: lang)
  - POST /api/upload/firmware — OTA загрузка прошивки (multipart form upload)
  - POST /api/upload/fs — загрузка файла в файловую систему
  - POST /api/wifi — сохранить WiFi (ssid, pass)
  - POST /api/reboot — перезагрузка (type={soft,hard}, delay=sec)

Как загрузить веб-ресурсы (data) в файловую систему LittleFS:
1. Убедитесь, что установлен пакет `LittleFS` и uploadfs поддерживается
2. Выполните:
   pio run --target uploadfs

Сборка проекта:
   pio run

Примечания:
- Код организован по классам: `SystemManager`, `TaskManager`, `WebUI`.
- По умолчанию точка доступа создаётся при старте (позволяет зайти в интерфейс при отсутствии конфигурации WiFi).

