#ifndef MONITORING_KEYLOGGER_H
#define MONITORING_KEYLOGGER_H

#include "../api/common_types.h"
#include <string>
#include <vector>
#include <ctime>

// Структура для обнаруженного keylogger
struct DetectedKeylogger {
    DWORD pid;
    std::string name;
    std::string path;
    std::vector<std::string> reasons;
    int severity; // 1-3 уровень опасности
};

// Результат сканирования
struct KeyloggerScanResult {
    time_t timestamp;
    int totalProcesses;
    int detectedCount;
    std::vector<DetectedKeylogger> detectedKeyloggers;
};

// Функции для работы с keyloggers
std::vector<ProcessInfo> get_running_processes();
KeyloggerScanResult scan_for_keyloggers();
bool block_keylogger_process(DWORD pid);
void run_keylogger_scanner();

#endif // MONITORING_KEYLOGGER_H