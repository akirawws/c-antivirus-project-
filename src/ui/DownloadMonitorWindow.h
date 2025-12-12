#pragma once

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

struct SuspiciousFile {
    std::wstring fileName;
    std::wstring fullPath;
    std::wstring pattern;
    FILETIME detectedTime;
};

class DownloadMonitorWindow {
private:
    HWND hwnd;
    HWND hListView;
    HWND hButtonStart;
    HWND hButtonStop;
    HWND hStatusLabel;
    HWND hBackButton;
    
    std::thread scannerThread;
    std::atomic<bool> isScanning;
    std::atomic<bool> shouldStop;
    
    std::vector<SuspiciousFile> suspiciousFiles;
    std::mutex filesMutex;
    
    static LRESULT CALLBACK WndProcStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    
    void UpdateListView();
    void StartScanning();
    void StopScanning();
    void ScannerThreadFunc();
    void DeleteSelectedFile(int itemIndex);
    void ShowFileContextMenu(int itemIndex);
    
public:
    DownloadMonitorWindow();
    ~DownloadMonitorWindow();
    
    bool Create(HINSTANCE hInstance, int nCmdShow);
    void AddSuspiciousFile(const SuspiciousFile& file);
};

