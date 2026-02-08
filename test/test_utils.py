import requests
import random
import string

# --- Configuration ---
# IP-адрес вашего ESP32. По умолчанию 192.168.4.1 в режиме AP.
BASE_URL = "http://192.168.4.1"

# --- Test State ---
# Простой способ отслеживать результаты тестов между модулями
test_stats = {"passed": 0, "failed": 0}

def random_string(length=8):
    """Генерирует случайную строку."""
    letters = string.ascii_lowercase
    return ''.join(random.choice(letters) for i in range(length))

def print_test_result(test_name, success, message=""):
    """Выводит результат теста в консоль и обновляет статистику."""
    if success:
        print(f"[ OK ] {test_name}")
        test_stats["passed"] += 1
    else:
        print(f"[FAIL] {test_name}: {message}")
        test_stats["failed"] += 1
    return success