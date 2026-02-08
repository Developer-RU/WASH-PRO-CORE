import requests
from .test_utils import BASE_URL, print_test_result, random_string

def run_file_tests():
    """Запускает все тесты файлового менеджера."""
    print("\n--- File Management Tests ---")
    test_file_management()

def test_file_management():
    """Тестирует создание, переименование и удаление файлов."""
    file_name = f"test_file_{random_string()}.txt"
    file_path = f"/{file_name}"
    content = "Hello, LittleFS!"

    # 1. Создание (сохранение) файла
    test_name = "Save File"
    try:
        r = requests.post(f"{BASE_URL}/api/files/save", data={"path": file_path, "content": content})
        r.raise_for_status()
        if not print_test_result(test_name, True):
            return # Прерываем, если не удалось сохранить
    except requests.exceptions.RequestException as e:
        print_test_result(test_name, False, f"Request failed: {e} | Response: {r.text}")
        return

    # 2. Проверка наличия файла
    test_name = "List Files"
    try:
        r = requests.get(f"{BASE_URL}/api/files?path=/")
        r.raise_for_status()
        print_test_result(test_name, file_name in r.text, "Saved file not found in list")
    except requests.exceptions.RequestException as e:
        print_test_result(test_name, False, f"Request failed: {e}")

    # 3. Переименование файла
    new_file_name = f"renamed_{file_name}"
    test_name = "Rename File"
    try:
        r = requests.post(f"{BASE_URL}/api/files/rename", data={"path": file_path, "newName": new_file_name})
        r.raise_for_status()
        print_test_result(test_name, True)
    except requests.exceptions.RequestException as e:
        print_test_result(test_name, False, f"Request failed: {e} | Response: {r.text}")
        # Если переименование не удалось, удаляем старый файл
        requests.post(f"{BASE_URL}/api/files/delete", data={"path": file_path})
        return

    # 4. Проверка, что старого имени нет, а новое есть
    test_name = "Verify Rename"
    try:
        r = requests.get(f"{BASE_URL}/api/files?path=/")
        r.raise_for_status()
        list_content = r.text
        renamed_ok = new_file_name in list_content and file_name not in list_content
        print_test_result(test_name, renamed_ok, "File list doesn't reflect rename")
    except requests.exceptions.RequestException as e:
        print_test_result(test_name, False, f"Request failed: {e}")

    # 5. Удаление файла
    test_name = "Delete File"
    try:
        # Путь к переименованному файлу
        r = requests.post(f"{BASE_URL}/api/files/delete", data={"path": f"/{new_file_name}"})
        r.raise_for_status()
        print_test_result(test_name, True)
    except requests.exceptions.RequestException as e:
        print_test_result(test_name, False, f"Request failed: {e} | Response: {r.text}")