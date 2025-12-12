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
    ULONGLONG fileSizeBytes;
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
    HWND hRefreshButton;
    HWND hOpenFolderButton;
    HWND hBackButton;
    HIMAGELIST hImageList;
    int selectedIndex;
    ProcessManager processManager;
    
    static LRESULT CALLBACK WndProcStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
     
    void UpdateListView();
    std::wstring FormatMemory(ULONGLONG bytes) const;
    void ShowProcessContextMenu(int itemIndex);
    void TerminateSelectedProcess(int itemIndex);
    void OpenProcessLocation(int itemIndex);
    LRESULT HandleCustomDraw(LPNMLVCUSTOMDRAW customDraw);
    
public:
    ProcessMonitorWindow();
    ~ProcessMonitorWindow();
    
    bool Create(HINSTANCE hInstance, int nCmdShow);
};