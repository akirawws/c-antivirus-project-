#pragma once
#include <string>
#include <windows.h>

struct Process {
    std::wstring name;
    DWORD pid;
    std::wstring filePath;
    long long size;
    bool suspicious;
    HICON icon;

    Process(const std::wstring& n, DWORD p, const std::wstring& fp,
        long long s, bool susp, HICON ic = nullptr)
        : name(n), pid(p), filePath(fp), size(s), suspicious(susp), icon(ic) {
    }

    // Запрещаем копирование
    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;

    // Разрешаем перемещение
    Process(Process&& other) noexcept
        : name(std::move(other.name))
        , pid(other.pid)
        , filePath(std::move(other.filePath))
        , size(other.size)
        , suspicious(other.suspicious)
        , icon(other.icon) {
        other.icon = nullptr;
    }

    Process& operator=(Process&& other) noexcept {
        if (this != &other) {
            // Освобождаем старую иконку
            if (icon) {
                DestroyIcon(icon);
            }

            name = std::move(other.name);
            pid = other.pid;
            filePath = std::move(other.filePath);
            size = other.size;
            suspicious = other.suspicious;
            icon = other.icon;

            other.icon = nullptr;
        }
        return *this;
    }

    ~Process() {
        if (icon) {
            DestroyIcon(icon);
        }
    }
};