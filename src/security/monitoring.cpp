#include "../api/monitoring_h.cpp"

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <cwctype>


static bool DEBUG_MODE = true;

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
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!hProcess) continue;

        wchar_t pathBuf[MAX_PATH];
        DWORD pathLen = MAX_PATH;
        std::wstring fullPathW;
        if (QueryFullProcessImageNameW(hProcess, 0, pathBuf, &pathLen)) {
            fullPathW.assign(pathBuf, pathLen);
        }

        CloseHandle(hProcess);
        if (fullPathW.empty()) continue;

        if (starts_with_icase(fullPathW, windowsDir)) continue;

        std::uint64_t fileSize = 0;
        try {
            fileSize = std::filesystem::file_size(fullPathW);
        } catch (...) {
            continue;
        }

        bool suspicious = starts_with_icase(fullPathW, tempDir);

        ProcessInfo info;
        info.name       = exeName;
        info.pid        = pid;
        info.file_path  = wide_to_utf8(fullPathW);
        info.size       = fileSize;
        info.suspicious = suspicious;
        info.icon       = info.file_path;

        if (DEBUG_MODE) {
            std::cout << "[PROCESS] "
                      << info.name << " | PID=" << info.pid
                      << " | Path=" << info.file_path
                      << " | Size=" << info.size
                      << " | Suspicious=" << (info.suspicious ? "YES" : "NO")
                      << "\n";
        }

        result.push_back(std::move(info));

    } while (Process32NextW(snapshot, &pe));

    CloseHandle(snapshot);

    std::sort(result.begin(), result.end(),
              [](const ProcessInfo& a, const ProcessInfo& b) {
                  if (a.suspicious != b.suspicious)
                      return a.suspicious > b.suspicious;
                  return a.name < b.name;
              });

    return result;
}
