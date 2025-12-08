#include "monitoring_h.h"
#include <windows.h>
#include <vector>
#include <string>

// Объявляем функцию из monitoring.cpp
extern std::vector<ProcessInfo> get_process_list_snapshot();

// Реальная реализация - вызываем функцию из monitoring.cpp
std::vector<ProcessInfo> ProcessMonitorAPI::GetAllProcesses() {
    return get_process_list_snapshot();
}