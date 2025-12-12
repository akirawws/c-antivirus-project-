#pragma once

#include <windows.h>
#include <commctrl.h>
#include <string>

class SettingsWindow {
private:
    HWND hwnd;
    HWND hCheckAutoScan;
    HWND hCheckRealTimeProtection;
    HWND hCheckNotifications;
    HWND hCheckQuarantine;
    HWND hButtonSave;
    HWND hButtonBack;
    
    static LRESULT CALLBACK WndProcStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    
public:
    SettingsWindow();
    ~SettingsWindow();
    
    bool Create(HINSTANCE hInstance, int nCmdShow);
    void ShowMainMenu();
};

