import requests
from .test_utils import BASE_URL, print_test_result

def run_system_tests():
    """Запускает все системные тесты."""
    print("\n--- System API Tests ---")
    test_api_info()
    test_api_system()
    test_settings_change()

def test_api_info():
    """Тестирует эндпоинт /api/info."""
    test_name = "GET /api/info"
    try:
        r = requests.get(f"{BASE_URL}/api/info")
        r.raise_for_status() # Проверка на 2xx статус
        data = r.json()
        assert "serial" in data
        assert "freeHeap" in data
        print_test_result(test_name, True)
    except (requests.exceptions.RequestException, AssertionError, ValueError) as e:
        message = f"Request failed: {e}"
        if 'r' in locals() and r:
             message += f" | Status: {r.status_code}, Response: {r.text}"
        print_test_result(test_name, False, message)

def test_settings_change():
    """Тестирует изменение настроек, например, языка."""
    test_name = "POST /api/setlanguage"
    original_lang = None

    try:
        # 1. Получаем текущий язык
        r_get1 = requests.get(f"{BASE_URL}/api/system")
        r_get1.raise_for_status()
        original_lang = r_get1.json().get("language", "en")

        # 2. Устанавливаем новый язык
        new_lang = "ru" if original_lang != "ru" else "en"
        r_post = requests.post(f"{BASE_URL}/api/setlanguage", data={"lang": new_lang})
        r_post.raise_for_status()

        # 3. Проверяем, что язык изменился
        r_get2 = requests.get(f"{BASE_URL}/api/system")
        r_get2.raise_for_status()
        current_lang = r_get2.json().get("language")

        assert current_lang == new_lang
        print_test_result(test_name, True)

    except (requests.exceptions.RequestException, AssertionError, ValueError) as e:
        print_test_result(test_name, False, f"Request failed: {e}")
    finally:
        # 4. Возвращаем исходный язык, чтобы не сломать состояние
        if original_lang:
            requests.post(f"{BASE_URL}/api/setlanguage", data={"lang": original_lang})

def test_api_system():
    """Тестирует эндпоинт /api/system."""
    test_name = "GET /api/system"
    try:
        r = requests.get(f"{BASE_URL}/api/system")
        r.raise_for_status()
        data = r.json()
        assert "swSerial" in data
        assert "language" in data
        print_test_result(test_name, True)
    except (requests.exceptions.RequestException, AssertionError, ValueError) as e:
        message = f"Request failed: {e}"
        if 'r' in locals() and r:
             message += f" | Status: {r.status_code}, Response: {r.text}"
        print_test_result(test_name, False, message)