#include "monitoring_keylogger.h"
#include <iostream>
#include <algorithm>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <set>
#include <regex>
#include <sstream>

// Добавляем определение для QueryFullProcessImageName если нужно
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600  // Windows Vista или выше
#endif

using namespace std;

// Известные имена keylogger процессов
vector<string> known_keyloggers = {
    "keylogger.exe", "keystroke.exe", "hooker.exe",
    "spyagent.exe", "perfectkeylogger.exe",
    "refog.exe", "microkeylogger.exe", "allinonekeylogger.exe",
    "klog.exe", "keyspy.exe", "keymonitor.exe"
};

// Подозрительные слова в именах (должны быть точными!)
vector<string> suspicious_words = {
    "keylogger", "keystroke", "keycapture", "keyspy",
    "keyhook", "keymonitor", "keyrecorder", "logkeys",
    "hooklog", "keysniff", "keygrab", "spylog",
    "capturekey", "keytrack", "klogger"
};

// Системные процессы, которые НЕ проверяем (полный список)
set<string> system_processes = {
    // Системные
    "system", "system idle process", "secure system", "registry",
    "[system process]", "smss.exe", "wininit.exe", "services.exe",
    "lsaiso.exe", "fontdrvhost.exe", "dwm.exe", "csrss.exe",
    "winlogon.exe", "lsass.exe", "svchost.exe", "taskhostw.exe",
    "explorer.exe", "conhost.exe", "ctfmon.exe", "dllhost.exe",
    "runtimebroker.exe", "backgroundTaskHost.exe", "SearchApp.exe",
    "TextInputHost.exe", "StartMenuExperienceHost.exe", "PhoneExperienceHost.exe",
    "SecurityHealthSystray.exe", "UserOOBEBroker.exe", "smartscreen.exe",
    "sihost.exe", "dasHost.exe", "AggregatorHost.exe", "MoUsoCoreWorker.exe",
    "WmiPrvSE.exe", "PresentationFontCache.exe", "audiodg.exe",
    
    // Службы
    "igfxCUIService.exe", "FNPLicensingService64.exe", "WsNativePushService.exe",
    "WMIRegistrationService.exe", "SecurityHealthService.exe",
    
    // Драйверы и системные компоненты
    "nvdisplay.container.exe", "intelcphdcpsvc.exe", "igfxem.exe",
    "igfxhk.exe", "igfxtray.exe"
};

// Легитимные пользовательские программы
set<string> user_programs = {
    "chrome.exe", "firefox.exe", "edge.exe", "code.exe",
    "notepad.exe", "figma_agent.exe", "vmware-tray.exe",
    "adskaccessservice.exe", "wshelper.exe", "wstoastnotification.exe",
    "cbservice.exe", "vrol.exe", "tabnine.exe", "wd-tabnine.exe"
};

string to_lower(const string& str) {
    string result = str;
    transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

// Получаем путь к процессу (универсальный метод)
string get_process_path(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProcess) {
        // Пробуем с меньшими правами
        hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!hProcess) {
            return "";
        }
    }
    
    char path[MAX_PATH] = {0};
    
    // Пробуем разные методы получения пути
    if (GetModuleFileNameExA(hProcess, NULL, path, MAX_PATH)) {
        CloseHandle(hProcess);
        return string(path);
    }
    
    // Альтернативный метод через GetProcessImageFileName
    if (GetProcessImageFileNameA(hProcess, path, MAX_PATH)) {
        CloseHandle(hProcess);
        // Преобразуем устройство в букву диска (упрощенно)
        return string(path);
    }
    
    CloseHandle(hProcess);
    return "";
}

// Проверка, является ли процесс системным
bool is_system_or_legitimate(const string& name, const string& path) {
    string lower_name = to_lower(name);
    
    // Проверяем по имени
    if (system_processes.find(lower_name) != system_processes.end()) {
        return true;
    }
    
    if (user_programs.find(lower_name) != user_programs.end()) {
        return true;
    }
    
    // Проверяем по пути
    string lower_path = to_lower(path);
    if (!lower_path.empty()) {
        // Системные пути
        if (lower_path.find("\\windows\\") != string::npos ||
            lower_path.find("\\program files\\") != string::npos ||
            lower_path.find("\\program files (x86)\\" ) != string::npos ||
            lower_path.find("autodesk") != string::npos ||
            lower_path.find("microsoft") != string::npos ||
            lower_path.find("google") != string::npos ||
            lower_path.find("vmware") != string::npos ||
            lower_path.find("chaos group") != string::npos ||
            lower_path.find("intel") != string::npos ||
            lower_path.find("nvidia") != string::npos ||
            lower_path.find("adobe") != string::npos) {
            return true;
        }
    }
    
    return false;
}

// Проверка на наличие подозрительных слов в имени
bool has_suspicious_name(const string& name) {
    string lower_name = to_lower(name);
    
    // Проверка известных keyloggers
    for (const auto& keylogger : known_keyloggers) {
        if (lower_name == keylogger) {
            return true;
        }
    }
    
    // Проверка подозрительных слов
    for (const auto& word : suspicious_words) {
        if (lower_name.find(word) != string::npos) {
            return true;
        }
    }
    
    return false;
}

// Получаем имя родительского процесса
string get_parent_name(DWORD pid) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return "";
    }
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    string parent_name = "";
    
    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (pe32.th32ProcessID == pid) {
                parent_name = pe32.szExeFile;
                break;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    
    CloseHandle(hSnapshot);
    return to_lower(parent_name);
}

// Проверка Python процесса
bool is_suspicious_python_process(const string& name, const string& path, DWORD pid) {
    string lower_name = to_lower(name);
    
    // Проверяем, является ли это Python процессом
    bool is_python = (lower_name.find("python") != string::npos) ||
                     (lower_name.find("py.exe") != string::npos) ||
                     (lower_name.find("pythonw.exe") != string::npos);
    
    if (!is_python) {
        return false;
    }
    
    // Получаем родительский процесс
    string parent_name = get_parent_name(pid);
    
    // Проверка пути
    string lower_path = to_lower(path);
    bool in_suspicious_location = false;
    
    if (!lower_path.empty()) {
        // Python в подозрительных местах
        if (lower_path.find("\\temp\\") != string::npos ||
            lower_path.find("\\appdata\\local\\temp\\") != string::npos ||
            lower_path.find("\\appdata\\roaming\\") != string::npos) {
            
            // Проверяем, что это не установленный Python
            if (lower_path.find("\\python") == string::npos &&
                lower_path.find("\\program files") == string::npos &&
                lower_path.find("\\appdata\\local\\programs\\python") == string::npos) {
                in_suspicious_location = true;
            }
        }
    }
    
    // Python подозрителен если:
    // 1. Запущен из проводника или cmd (пользователь запустил скрипт)
    // 2. Находится в temp/AppData
    // 3. Родительский процесс - explorer, cmd, или пустой
    if (parent_name == "explorer.exe" || parent_name == "cmd.exe" || 
        parent_name == "powershell.exe" || parent_name.empty()) {
        return true;
    }
    
    return in_suspicious_location;
}

vector<ProcessInfo> get_running_processes() {
    vector<ProcessInfo> processes;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return processes;
    }
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (Process32First(hSnapshot, &pe32)) {
        do {
            ProcessInfo proc;
            proc.pid = pe32.th32ProcessID;
            proc.name = pe32.szExeFile;
            proc.parentPid = pe32.th32ParentProcessID;
            
            // Получаем путь
            proc.path = get_process_path(proc.pid);
            
            processes.push_back(proc);
        } while (Process32Next(hSnapshot, &pe32));
    }
    
    CloseHandle(hSnapshot);
    return processes;
}

KeyloggerScanResult scan_for_keyloggers() {
    KeyloggerScanResult result;
    result.timestamp = time(NULL);
    
    vector<ProcessInfo> processes = get_running_processes();
    
    cout << "\n=== Keylogger Detection Scanner ===\n";
    cout << "Scanning " << processes.size() << " processes...\n";
    cout << "Looking for:\n";
    cout << "1. Known keylogger executables\n";
    cout << "2. Processes with 'keylog' in name\n";
    cout << "3. Python scripts in suspicious locations\n\n";
    
    int detected_count = 0;
    
    for (const auto& proc : processes) {
        // Пропускаем самого себя
        if (proc.pid == GetCurrentProcessId()) {
            continue;
        }
        
        // Пропускаем системные и легитимные процессы
        if (is_system_or_legitimate(proc.name, proc.path)) {
            continue;
        }
        
        vector<string> reasons;
        bool suspicious = false;
        
        // ПРОВЕРКА 1: Подозрительное имя
        if (has_suspicious_name(proc.name)) {
            reasons.push_back("Suspicious name: " + proc.name);
            suspicious = true;
        }
        
        // ПРОВЕРКА 2: Python процессы
        if (!suspicious && is_suspicious_python_process(proc.name, proc.path, proc.pid)) {
            reasons.push_back("Suspicious Python process");
            suspicious = true;
        }
        
        // ПРОВЕРКА 3: Процессы без пути или в temp
        if (!suspicious) {
            string lower_path = to_lower(proc.path);
            
            // Процессы без пути (кроме системных, которые мы уже отфильтровали)
            if (proc.path.empty()) {
                reasons.push_back("Process with no executable path");
                suspicious = true;
            }
            // Процессы в временных директориях
            else if (lower_path.find("\\temp\\") != string::npos ||
                     lower_path.find("\\appdata\\local\\temp\\") != string::npos) {
                
                // Проверяем, что это не известные программы
                if (lower_path.find("microsoft") == string::npos &&
                    lower_path.find("google") == string::npos &&
                    lower_path.find("adobe") == string::npos) {
                    
                    reasons.push_back("Executable in temporary directory");
                    suspicious = true;
                }
            }
        }
        
        if (suspicious) {
            detected_count++;
            
            DetectedKeylogger keylogger;
            keylogger.pid = proc.pid;
            keylogger.name = proc.name;
            keylogger.path = proc.path;
            keylogger.reasons = reasons;
            keylogger.severity = min(3, (int)reasons.size());
            
            result.detectedKeyloggers.push_back(keylogger);
            
            cout << "[!] " << proc.name << " (PID: " << proc.pid << ")\n";
            if (!proc.path.empty()) {
                cout << "    Path: " << proc.path << "\n";
            }
            cout << "    Reasons:\n";
            for (const auto& reason : reasons) {
                cout << "      - " << reason << "\n";
            }
            
            // Показываем родительский процесс
            if (proc.parentPid > 0) {
                string parent_name = get_parent_name(proc.parentPid);
                if (!parent_name.empty()) {
                    cout << "    Parent: " << parent_name << "\n";
                }
            }
            cout << "\n";
        }
    }
    
    cout << "========================================\n";
    if (detected_count == 0) {
        cout << "No keyloggers detected.\n";
        cout << "To test: Run a Python script with 'keylogger' in name\n";
    } else {
        cout << "Found " << detected_count << " suspicious process(es)\n";
        cout << "(Review carefully - may include false positives)\n";
    }
    cout << "========================================\n";
    
    result.totalProcesses = processes.size();
    result.detectedCount = detected_count;
    
    return result;
}

bool block_keylogger_process(DWORD pid) {
    cout << "\nWarning: Terminating processes can cause instability!\n";
    cout << "Are you sure you want to terminate PID " << pid << "? (y/n): ";
    
    char confirm;
    cin >> confirm;
    
    if (confirm != 'y' && confirm != 'Y') {
        cout << "Cancelled.\n";
        return false;
    }
    
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess) {
        if (TerminateProcess(hProcess, 0)) {
            CloseHandle(hProcess);
            cout << "Process " << pid << " terminated.\n";
            return true;
        }
        CloseHandle(hProcess);
    }
    
    cout << "Failed to terminate process " << pid << "\n";
    return false;
}

void run_keylogger_scanner() {
    cout << "\n=== Keylogger Detection ===\n";
    cout << "For testing:\n";
    cout << "1. Create a Python file with 'keylogger' in name\n";
    cout << "2. Run it: python test_keylogger.py\n";
    cout << "3. Then run this scanner\n\n";
    
    cout << "Press Enter to start scanning...";
    cin.ignore();
    cin.get();
    
    KeyloggerScanResult result = scan_for_keyloggers();
    
    if (result.detectedCount > 0) {
        cout << "\nOptions:\n";
        cout << "1. View process details\n";
        cout << "2. Terminate a process\n";
        cout << "3. Exit\n\n";
        
        cout << "Enter choice (1-3): ";
        int choice;
        cin >> choice;
        
        if (choice == 1) {
            cout << "Enter PID to view: ";
            DWORD pid;
            cin >> pid;
            
            bool found = false;
            for (const auto& keylogger : result.detectedKeyloggers) {
                if (keylogger.pid == pid) {
                    found = true;
                    cout << "\nProcess Details:\n";
                    cout << "Name: " << keylogger.name << "\n";
                    cout << "PID: " << keylogger.pid << "\n";
                    cout << "Path: " << keylogger.path << "\n";
                    cout << "Severity: " << keylogger.severity << "/3\n";
                    cout << "Reasons:\n";
                    for (const auto& reason : keylogger.reasons) {
                        cout << "  - " << reason << "\n";
                    }
                    break;
                }
            }
            
            if (!found) {
                cout << "Process not found.\n";
            }
            
        } else if (choice == 2) {
            cout << "Enter PID to terminate: ";
            DWORD pid;
            cin >> pid;
            block_keylogger_process(pid);
        }
    }
    
    cout << "\nScan completed.\n";
    system("pause");
}