#include <windows.h>
#include <commctrl.h>
#include "ProcessMonitorWindow.h"
#include "DownloadMonitorWindow.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

// Глобальные переменные
HINSTANCE g_hInstance;
HWND g_hMainWnd;
ProcessMonitorWindow* g_pProcessMonitor = nullptr;
DownloadMonitorWindow* g_pDownloadMonitor = nullptr;

// Прототипы функций
void CreateProcessMonitorWindow();
void CreateDownloadMonitorWindow();

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
        CreateWindowW(L"BUTTON", L"📊 Монитор процессов",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            50, 80, 280, 45, hwnd, (HMENU)1001, g_hInstance, NULL);

        CreateWindowW(L"BUTTON", L"📥 Мониторинг загрузок",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            50, 135, 280, 45, hwnd, (HMENU)1002, g_hInstance, NULL);

        CreateWindowW(L"BUTTON", L"🔍 Сканировать систему",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            50, 190, 280, 45, hwnd, (HMENU)1003, g_hInstance, NULL);

        CreateWindowW(L"BUTTON", L"⚙ Настройки",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            50, 245, 280, 45, hwnd, (HMENU)1004, g_hInstance, NULL);

        CreateWindowW(L"BUTTON", L"🚪 Выход",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            50, 300, 280, 45, hwnd, (HMENU)1005, g_hInstance, NULL);

        // Статус
        CreateWindowW(L"STATIC", L"Статус: Готов к работе",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, 360, 340, 20, hwnd, NULL, g_hInstance, NULL);
    }
    return 0;

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        switch (id) {
        case 1001: // Монитор процессов
            CreateProcessMonitorWindow();
            break;
        case 1002: // Мониторинг загрузок
            CreateDownloadMonitorWindow();
            break;
        case 1003: // Сканировать систему
            MessageBoxW(hwnd, L"Запуск сканирования системы...", L"Сканирование", MB_OK | MB_ICONINFORMATION);
            break;
        case 1004: // Настройки
            MessageBoxW(hwnd, L"Открытие настроек...", L"Настройки", MB_OK | MB_ICONINFORMATION);
            break;
        case 1005: // Выход
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
        if (g_pDownloadMonitor) {
            delete g_pDownloadMonitor;
            g_pDownloadMonitor = nullptr;
        }
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

void CreateProcessMonitorWindow() {
    if (g_pProcessMonitor) {
        delete g_pProcessMonitor;
    }
    
    g_pProcessMonitor = new ProcessMonitorWindow();
    
    if (g_pProcessMonitor) {
        bool result = g_pProcessMonitor->Create(g_hInstance, SW_SHOWNORMAL);
        
        if (result) {
            ShowWindow(g_hMainWnd, SW_HIDE);
        } else {
            MessageBoxW(NULL, 
                L"Не удалось создать окно монитора процессов", 
                L"Ошибка", MB_OK | MB_ICONERROR);
            
            delete g_pProcessMonitor;
            g_pProcessMonitor = nullptr;
        }
    }
}

void CreateDownloadMonitorWindow() {
    if (g_pDownloadMonitor) {
        delete g_pDownloadMonitor;
    }
    
    g_pDownloadMonitor = new DownloadMonitorWindow();
    
    if (g_pDownloadMonitor) {
        bool result = g_pDownloadMonitor->Create(g_hInstance, SW_SHOWNORMAL);
        
        if (result) {
            ShowWindow(g_hMainWnd, SW_HIDE);
        } else {
            MessageBoxW(NULL, 
                L"Не удалось создать окно мониторинга загрузок", 
                L"Ошибка", MB_OK | MB_ICONERROR);
            
            delete g_pDownloadMonitor;
            g_pDownloadMonitor = nullptr;
        }
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
        400, 440,
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