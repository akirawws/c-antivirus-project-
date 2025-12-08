#include <windows.h>
#include <commctrl.h>
#include "ProcessMonitorWindow.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

// Глобальные переменные
HINSTANCE g_hInstance;
HWND g_hMainWnd;
ProcessMonitorWindow* g_pProcessMonitor = nullptr;

// Прототип функции для создания окна монитора процессов
void CreateProcessMonitorWindow();

// Процедура главного окна
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
    {
        // Создаем элементы управления
        CreateWindowW(L"STATIC", L"Антивирусный комплекс",
            WS_CHILD | WS_VISIBLE | SS_CENTER | WS_BORDER,
            20, 20, 340, 40, hwnd, NULL, g_hInstance, NULL);

        // Кнопки меню
        CreateWindowW(L"BUTTON", L"📊 Запустить монитор процессов",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            50, 80, 280, 50, hwnd, (HMENU)1001, g_hInstance, NULL);

        CreateWindowW(L"BUTTON", L"🔍 Сканировать систему",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            50, 140, 280, 50, hwnd, (HMENU)1002, g_hInstance, NULL);

        CreateWindowW(L"BUTTON", L"⚙ Настройки",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            50, 200, 280, 50, hwnd, (HMENU)1003, g_hInstance, NULL);

        CreateWindowW(L"BUTTON", L"🚪 Выход",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            50, 260, 280, 50, hwnd, (HMENU)1004, g_hInstance, NULL);

        // Статус
        CreateWindowW(L"STATIC", L"Статус: Готов к работе",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 320, 340, 20, hwnd, NULL, g_hInstance, NULL);
    }
    return 0;

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        switch (id) {
        case 1001: // Запустить монитор процессов
            CreateProcessMonitorWindow();
            break;
        case 1002: // Сканировать систему
            MessageBoxW(hwnd, L"Запуск сканирования системы...", L"Сканирование", MB_OK | MB_ICONINFORMATION);
            break;
        case 1003: // Настройки
            MessageBoxW(hwnd, L"Открытие настроек...", L"Настройки", MB_OK | MB_ICONINFORMATION);
            break;
        case 1004: // Выход
            DestroyWindow(hwnd);
            break;
        }
    }
    return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        
        // Рисуем рамку
        RECT rc;
        GetClientRect(hwnd, &rc);
        rc.bottom = 70;
        FillRect(hdc, &rc, (HBRUSH)(COLOR_ACTIVECAPTION + 1));
        
        // Текст заголовка
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        
        HFONT hFont = CreateFontW(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, L"Arial");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        
        TextOutW(hdc, 50, 25, L"🛡️ Антивирусный монитор", 23);
        
        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
        
        EndPaint(hwnd, &ps);
    }
    return 0;

    case WM_DESTROY:
        if (g_pProcessMonitor) {
            delete g_pProcessMonitor;
            g_pProcessMonitor = nullptr;
        }
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

void CreateProcessMonitorWindow() {
    MessageBoxW(NULL, L"DEBUG: CreateProcessMonitorWindow called", L"Debug", MB_OK);
    
    if (g_pProcessMonitor) {
        MessageBoxW(NULL, L"DEBUG: Deleting old monitor", L"Debug", MB_OK);
        delete g_pProcessMonitor;
    }
    
    MessageBoxW(NULL, L"DEBUG: Creating new ProcessMonitorWindow", L"Debug", MB_OK);
    g_pProcessMonitor = new ProcessMonitorWindow();
    
    if (g_pProcessMonitor) {
        MessageBoxW(NULL, L"DEBUG: Trying to create window...", L"Debug", MB_OK);
        bool result = g_pProcessMonitor->Create(g_hInstance, SW_SHOWNORMAL);
        
        if (result) {
            MessageBoxW(NULL, L"DEBUG: Window created successfully!", L"Debug", MB_OK);
            ShowWindow(g_hMainWnd, SW_HIDE);
        } else {
            MessageBoxW(NULL, 
                L"ERROR: Failed to create process monitor window\n"
                L"Check if:\n"
                L"1. Window class is registered\n"
                L"2. CreateWindowW parameters are correct", 
                L"Error", MB_OK | MB_ICONERROR);
            
            delete g_pProcessMonitor;
            g_pProcessMonitor = nullptr;
        }
    } else {
        MessageBoxW(NULL, L"ERROR: Failed to allocate memory for monitor", L"Error", MB_OK | MB_ICONERROR);
    }
}

// Точка входа - для GCC используйте WinMain, а не wWinMain
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow) {
    
    // Инициализация Common Controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);
    
    g_hInstance = hInstance;
    
    // Регистрируем класс главного окна
    WNDCLASSW wc = {};
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"AntivirusMainWndClass";
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    
    if (!RegisterClassW(&wc)) {
        MessageBoxW(NULL, L"Ошибка регистрации класса окна", L"Ошибка", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // Создаем главное окно
    g_hMainWnd = CreateWindowExW(
        0,
        L"AntivirusMainWndClass",
        L"Антивирусный комплекс - Главное меню",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 400,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    
    if (!g_hMainWnd) {
        MessageBoxW(NULL, L"Ошибка создания окна", L"Ошибка", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);
    
    // Цикл сообщений
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    
    return (int)msg.wParam;
}