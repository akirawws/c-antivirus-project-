#include "monitoring_h.h"
#include <windows.h>
#include <vector>
#include <string>

// ПРОСТАЯ ЗАГЛУШКА
std::vector<ProcessInfo> ProcessMonitorAPI::GetAllProcesses() {
    std::vector<ProcessInfo> processes;
    
    // Тестовые данные
    ProcessInfo p1;
    p1.pid = 1234;
    p1.name = L"explorer.exe";
    p1.path = L"C:\\Windows\\explorer.exe";
    p1.memoryUsage = 51200;  // 50 MB
    p1.isSuspicious = false;
    p1.icon = LoadIcon(NULL, IDI_APPLICATION);
    processes.push_back(p1);
    
    ProcessInfo p2;
    p2.pid = 5678;
    p2.name = L"chrome.exe";
    p2.path = L"C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe";
    p2.memoryUsage = 204800;  // 200 MB
    p2.isSuspicious = false;
    p2.icon = LoadIcon(NULL, IDI_WINLOGO);
    processes.push_back(p2);
    
    return processes;
}