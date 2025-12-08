#include "../api/monitoring_h.h"

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <algorithm>
#include <iostream>
#include <cwctype>

static bool DEBUG_MODE = false;

static std::string wide_to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return {};
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string result(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), result.data(), size_needed, nullptr, nullptr);
    return result;
}

static std::wstring get_temp_dir_w() {
    wchar_t buf[MAX_PATH];
    DWORD len = GetTempPathW(MAX_PATH, buf);
    if (len == 0 || len > MAX_PATH) return L"";
    std::wstring s(buf, len);
    while (!s.empty() && (s.back() == L'\\' || s.back() == L'/')) s.pop_back();
    return s;
}

static std::wstring get_windows_dir_w() {
    wchar_t buf[MAX_PATH];
    UINT len = GetWindowsDirectoryW(buf, MAX_PATH);
    if (len == 0 || len > MAX_PATH) return L"";
    std::wstring s(buf, len);
    while (!s.empty() && (s.back() == L'\\' || s.back() == L'/')) s.pop_back();
    return s;
}

static bool starts_with_icase(const std::wstring& value, const std::wstring& prefix) {
    if (prefix.empty()) return false;
    if (value.size() < prefix.size()) return false;
    for (size_t i = 0; i < prefix.size(); ++i) {
        wchar_t a = std::towlower(value[i]);
        wchar_t b = std::towlower(prefix[i]);
        if (a != b) return false;
    }
    return true;
}

static std::uint64_t get_file_size_w(const std::wstring& path) {
    HANDLE h = CreateFileW(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (h == INVALID_HANDLE_VALUE) return 0;

    LARGE_INTEGER size;
    BOOL ok = GetFileSizeEx(h, &size);
    CloseHandle(h);

    if (!ok) return 0;
    return static_cast<std::uint64_t>(size.QuadPart);
}

std::vector<ProcessInfo> get_process_list_snapshot() {
    std::vector<ProcessInfo> result;

    const std::wstring tempDir = get_temp_dir_w();
    const std::wstring windowsDir = get_windows_dir_w();

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return result;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);

    if (!Process32FirstW(snapshot, &pe)) {
        CloseHandle(snapshot);
        return result;
    }

    do {
        std::wstring exeNameW = pe.szExeFile;
        std::string exeName = wide_to_utf8(exeNameW);

        if (exeName == "System" || exeName == "System Idle Process") {
            continue;
        }

        DWORD pid = pe.th32ProcessID;
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (!hProcess) continue;

        wchar_t pathBuf[MAX_PATH] = {};
        DWORD copied = GetModuleFileNameExW(hProcess, nullptr, pathBuf, MAX_PATH);

        CloseHandle(hProcess);

        if (copied == 0) continue;

        std::wstring fullPathW(pathBuf);
        if (starts_with_icase(fullPathW, windowsDir)) continue;

        std::uint64_t fileSize = get_file_size_w(fullPathW);
        if (fileSize == 0) continue;

    bool suspicious = starts_with_icase(fullPathW, tempDir);

    ProcessInfo info;
    info.pid = pid;
    info.name = exeNameW;  // Используем wstring версию
    info.path = fullPathW;  // Используем wstring версию (не преобразуем в UTF-8)
    info.memoryUsage = 0;   // Устанавливаем значение (можете вычислить реальное использование памяти)
    info.isSuspicious = suspicious;
    info.icon = NULL;       // Иконку можно установить позже или оставить NULL

    if (DEBUG_MODE) {
        // Преобразуем wstring в UTF-8 для вывода
        std::cout << "[PROCESS] "
            << wide_to_utf8(info.name) << " | PID=" << info.pid
            << " | Path=" << wide_to_utf8(info.path)  // Преобразуем для вывода
            << " | Memory=" << info.memoryUsage
            << " | Suspicious=" << (info.isSuspicious ? "YES" : "NO")
            << "\n";
    }

    result.push_back(std::move(info));

    } while (Process32NextW(snapshot, &pe));

CloseHandle(snapshot);

    std::sort(result.begin(), result.end(),
        [](const ProcessInfo& a, const ProcessInfo& b) {
            if (a.isSuspicious != b.isSuspicious)      // изменили на isSuspicious
                return a.isSuspicious > b.isSuspicious; // изменили на isSuspicious
            return a.name < b.name;
        });

    return result;
}