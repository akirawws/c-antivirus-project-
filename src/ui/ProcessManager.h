#pragma once
#include "process.h"
#include <vector>
#include <windows.h>

class ProcessManager {
private:
    std::vector<Process> processes;

public:
    void fetchProcesses() {
        processes.clear();

        // Используем системные иконки
        HICON appIcon = LoadIconW(NULL, IDI_APPLICATION);
        HICON warningIcon = LoadIconW(NULL, IDI_WARNING);
        HICON infoIcon = LoadIconW(NULL, IDI_INFORMATION);

        // chrome.exe
        processes.emplace_back(
            L"chrome.exe",
            3120,
            L"C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe",
            18227320,
            false,
            appIcon
        );

        // virus.exe
        processes.emplace_back(
            L"virus.exe",
            5512,
            L"C:\\Users\\Akira\\AppData\\Local\\Temp\\virus.exe",
            88231,
            true,
            warningIcon
        );

        // Дополнительные процессы
        for (int i = 1; i <= 15; i++) {
            processes.emplace_back(
                L"process_" + std::to_wstring(i) + L".exe",
                1000 + i,
                L"C:\\Windows\\System32\\dummy_" + std::to_wstring(i) + L".exe",
                1024 * i * 10,
                (i % 4 == 0),
                infoIcon
            );
        }
    }

    const std::vector<Process>& getProcesses() const {
        return processes;
    }
};