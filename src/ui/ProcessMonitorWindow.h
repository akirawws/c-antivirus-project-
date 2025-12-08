#pragma once

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>

// Включаем API мониторинга
#include "../api/monitoring_h.h"

struct Process {
    DWORD pid;
    std::wstring name;
    std::wstring filePath;
    DWORD memoryKB;
    bool suspicious;
    HICON icon;
};

class ProcessManager {
public:
    std::vector<Process> processes;
    
    void fetchProcesses();
    const std::vector<Process>& getProcesses() const;
};

class ProcessMonitorWindow {
private:
    HWND hwnd;
    HWND hListView;
    HWND hButton;
    HIMAGELIST hImageList;
    int selectedIndex;
    ProcessManager processManager;
    
    static LRESULT CALLBACK WndProcStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
     
    void UpdateListView();
    std::wstring FormatMemory(DWORD memoryKB) const;
    
public:
    ProcessMonitorWindow();
    ~ProcessMonitorWindow();
    
    bool Create(HINSTANCE hInstance, int nCmdShow);
};