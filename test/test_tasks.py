import requests
import time
from .test_utils import BASE_URL, print_test_result, random_string

def run_task_tests():
    """Запускает все тесты жизненного цикла задач."""
    print("\n--- Task Lifecycle Tests ---")
    test_task_lifecycle()

def test_task_lifecycle():
    """Полный цикл тестирования задач: создание, переименование, запуск, остановка, удаление."""
    task_id = None
    task_name = f"test_task_{random_string()}"
    
    # 1. Создание задачи
    test_name = "Create Task"
    try:
        r = requests.post(f"{BASE_URL}/api/tasks", data={"name": task_name})
        r.raise_for_status()
        data = r.json()
        task_id = data.get("id")
        if not print_test_result(test_name, bool(task_id), "Task ID not in response"):
            return # Прерываем тест, если задача не создалась
    except requests.exceptions.RequestException as e:
        print_test_result(test_name, False, f"Request failed: {e} | Response: {r.text}")
        return

    # 2. Получение списка задач и проверка наличия новой
    test_name = "Get Tasks List"
    try:
        r = requests.get(f"{BASE_URL}/api/tasks")
        r.raise_for_status()
        print_test_result(test_name, task_name in r.text, "Newly created task not in list")
    except requests.exceptions.RequestException as e:
        print_test_result(test_name, False, f"Request failed: {e}")

    # 3. Сохранение скрипта для задачи
    test_name = "Save Task Script"
    script_content = 'log("Hello from test script!")'
    try:
        r = requests.post(f"{BASE_URL}/api/tasks", data={"id": task_id, "script": script_content})
        r.raise_for_status()
        print_test_result(test_name, True)
    except requests.exceptions.RequestException as e:
        print_test_result(test_name, False, f"Request failed: {e} | Response: {r.text}")

    # 4. Запуск задачи
    test_name = "Run Task"
    try:
        r = requests.post(f"{BASE_URL}/api/tasks/run", data={"id": task_id})
        r.raise_for_status()
        print_test_result(test_name, True)
    except requests.exceptions.RequestException as e:
        print_test_result(test_name, False, f"Request failed: {e} | Response: {r.text}")

    time.sleep(2) # Даем время на выполнение

    # 5. Остановка задачи
    test_name = "Stop Task"
    try:
        r = requests.post(f"{BASE_URL}/api/tasks/stop", data={"id": task_id})
        r.raise_for_status()
        print_test_result(test_name, True)
    except requests.exceptions.RequestException as e:
        print_test_result(test_name, False, f"Request failed: {e} | Response: {r.text}")

    # 6. Удаление задачи
    # В app.js удаление происходит через /api/files/delete. Это более правильный подход,
    # так как задача - это просто файл. Будем следовать этой логике.
    test_name = "Delete Task"
    try:
        task_file_path = f"/tasks/{task_id}.json"
        script_file_path = f"/scripts/{task_id}.lua"

        # Удаляем файл задачи
        r_task = requests.post(f"{BASE_URL}/api/files/delete", data={"path": task_file_path})
        r_task.raise_for_status()

        # Удаляем файл скрипта
        r_script = requests.post(f"{BASE_URL}/api/files/delete", data={"path": script_file_path})
        # Скрипт мог и не быть создан, поэтому 404 - это нормально. Проверяем на 200 или 404.
        if r_script.status_code not in [200, 404]:
            r_script.raise_for_status()

        print_test_result(test_name, True)
    except requests.exceptions.RequestException as e:
        response_text = e.response.text if e.response else "No response"
        print_test_result(test_name, False, f"Request failed: {e} | Response: {response_text}")