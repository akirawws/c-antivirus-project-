#pragma once

#include <windows.h>
#include <vector>
#include <string>

struct ProcessInfo {
    DWORD pid;
    std::wstring name;
    std::wstring path;
    DWORD memoryUsage;
    bool isSuspicious;
    HICON icon;
};

class ProcessMonitorAPI {
public:
    static std::vector<ProcessInfo> GetAllProcesses();
};