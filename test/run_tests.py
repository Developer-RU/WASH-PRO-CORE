import requests
import sys
from test_utils import BASE_URL, test_stats
from test_system import run_system_tests
from test_tasks import run_task_tests
from test_files import run_file_tests

def main():
    """Главная функция для запуска всех тестов."""
    print("--- Starting Web Server API Tests ---")
    print(f"Target: {BASE_URL}\n")

    # 1. Проверка доступности сервера
    try:
        requests.get(BASE_URL, timeout=5)
        print("[ OK ] Server is reachable.")
    except requests.exceptions.RequestException as e:
        print(f"[FATAL] Could not connect to the device at {BASE_URL}.")
        print("Please check the IP address and your Wi-Fi connection.")
        sys.exit(1)

    # 2. Запуск тестовых наборов
    run_system_tests()
    run_task_tests()
    run_file_tests()

    # 3. Вывод итогов
    print("\n--- Test Summary ---")
    print(f"Passed: {test_stats['passed']}")
    print(f"Failed: {test_stats['failed']}")
    print("--------------------")

    if test_stats["failed"] > 0:
        sys.exit(1) # Выход с кодом ошибки, если есть проваленные тесты

if __name__ == "__main__":
    main()