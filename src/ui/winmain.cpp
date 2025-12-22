#include <windows.h>
#include <commctrl.h>
#include "ProcessMonitorWindow.h"
#include "DownloadMonitorWindow.h"
#include "SettingsWindow.h"
#include "colors.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

// Глобальные переменные
HINSTANCE g_hInstance;
HWND g_hMainWnd;
ProcessMonitorWindow* g_pProcessMonitor = nullptr;
DownloadMonitorWindow* g_pDownloadMonitor = nullptr;
SettingsWindow* g_pSettingsWindow = nullptr;
HFONT g_hTitleFont = nullptr;
HFONT g_hSubtitleFont = nullptr;
HFONT g_hButtonFont = nullptr;

// Прототипы функций
void CreateProcessMonitorWindow();
void CreateDownloadMonitorWindow();
void CreateSettingsWindow();

// Процедура главного окна
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
    {
        // Шрифты
        g_hTitleFont = CreateFontW(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        g_hSubtitleFont = CreateFontW(16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        g_hButtonFont = CreateFontW(14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, L"Segoe UI");

        // Обычные кнопки с одной строкой (высота 35px)
        HWND hBtnProc = CreateWindowW(L"BUTTON", L"🖥 Монитор процессов",
            WS_CHILD | WS_VISIBLE | BS_CENTER | BS_VCENTER,
            24, 140, 200, 35, hwnd, (HMENU)1001, g_hInstance, NULL);

        HWND hBtnDownloads = CreateWindowW(L"BUTTON", L"📥 Мониторинг загрузок",
            WS_CHILD | WS_VISIBLE | BS_CENTER | BS_VCENTER,
            24, 185, 200, 35, hwnd, (HMENU)1002, g_hInstance, NULL);

        HWND hBtnScan = CreateWindowW(L"BUTTON", L"🔍 Сканировать систему",
            WS_CHILD | WS_VISIBLE | BS_CENTER | BS_VCENTER,
            24, 230, 200, 35, hwnd, (HMENU)1003, g_hInstance, NULL);

        HWND hBtnSettings = CreateWindowW(L"BUTTON", L"⚙ Настройки",
            WS_CHILD | WS_VISIBLE | BS_CENTER | BS_VCENTER,
            24, 275, 200, 35, hwnd, (HMENU)1004, g_hInstance, NULL);

        HWND hBtnExit = CreateWindowW(L"BUTTON", L"🚪 Выход",
            WS_CHILD | WS_VISIBLE | BS_CENTER | BS_VCENTER,
            24, 320, 200, 35, hwnd, (HMENU)1005, g_hInstance, NULL);

        // Текст на главной панели
        HWND hTitle = CreateWindowW(L"STATIC", L"Aegis Shield - Мониторинг в реальном времени",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            250, 140, 520, 30, hwnd, NULL, g_hInstance, NULL);

        HWND hSubtitle = CreateWindowW(L"STATIC", L"Комплексная защита системы. Следите за процессами, загрузками и состоянием безопасности из единого окна.",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            250, 175, 540, 40, hwnd, NULL, g_hInstance, NULL);

        // Дополнительная информация (все STATIC)
        HWND hInfo1 = CreateWindowW(L"STATIC", L"📊 Монитор процессов - Отслеживайте все активные процессы в системе, их использование ресурсов и статус безопасности.",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            250, 230, 680, 30, hwnd, NULL, g_hInstance, NULL);

        HWND hInfo2 = CreateWindowW(L"STATIC", L"📥 Мониторинг загрузок - Автоматическое сканирование папки загрузок на наличие подозрительных файлов и паттернов.",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            250, 270, 680, 30, hwnd, NULL, g_hInstance, NULL);

        HWND hInfo3 = CreateWindowW(L"STATIC", L"🔍 Сканирование системы - Полная проверка системы на наличие вирусов, вредоносного ПО и других угроз.",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            250, 310, 680, 30, hwnd, NULL, g_hInstance, NULL);

        HWND hInfo4 = CreateWindowW(L"STATIC", L"⚙ Настройки - Настройте параметры защиты, уведомления и автоматические действия антивируса.",
            WS_CHILD | WS_VISIBLE | SS_LEFT,  // ИСПРАВЛЕНО: WS_VISIBLE
            250, 350, 680, 30, hwnd, NULL, g_hInstance, NULL);

        // Применяем шрифты
        SendMessageW(hBtnProc, WM_SETFONT, (WPARAM)g_hButtonFont, TRUE);
        SendMessageW(hBtnDownloads, WM_SETFONT, (WPARAM)g_hButtonFont, TRUE);
        SendMessageW(hBtnScan, WM_SETFONT, (WPARAM)g_hButtonFont, TRUE);
        SendMessageW(hBtnSettings, WM_SETFONT, (WPARAM)g_hButtonFont, TRUE);
        SendMessageW(hBtnExit, WM_SETFONT, (WPARAM)g_hButtonFont, TRUE);
        SendMessageW(hTitle, WM_SETFONT, (WPARAM)g_hSubtitleFont, TRUE);
        SendMessageW(hSubtitle, WM_SETFONT, (WPARAM)g_hSubtitleFont, TRUE);
        SendMessageW(hInfo1, WM_SETFONT, (WPARAM)g_hSubtitleFont, TRUE);
        SendMessageW(hInfo2, WM_SETFONT, (WPARAM)g_hSubtitleFont, TRUE);
        SendMessageW(hInfo3, WM_SETFONT, (WPARAM)g_hSubtitleFont, TRUE);
        SendMessageW(hInfo4, WM_SETFONT, (WPARAM)g_hSubtitleFont, TRUE);
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
            CreateSettingsWindow();
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
        
        RECT client;
        GetClientRect(hwnd, &client);

        // Темный фон в стиле Kaspersky
        HBRUSH bgBrush = CreateSolidBrush(Colors::DARK_BG);
        FillRect(hdc, &client, bgBrush);
        DeleteObject(bgBrush);

        // Левый сайдбар (темная панель)
        RECT sidebar = { 0, 0, 240, client.bottom };
        HBRUSH sidebarBrush = CreateSolidBrush(Colors::DARK_PANEL);
        FillRect(hdc, &sidebar, sidebarBrush);
        DeleteObject(sidebarBrush);

        // Граница сайдбара
        HPEN sidebarPen = CreatePen(PS_SOLID, 1, Colors::DARK_BORDER);
        HPEN oldSidebarPen = (HPEN)SelectObject(hdc, sidebarPen);
        MoveToEx(hdc, 240, 0, NULL);
        LineTo(hdc, 240, client.bottom);
        SelectObject(hdc, oldSidebarPen);
        DeleteObject(sidebarPen);

        // Верхняя плашка бренда (бардовая)
        RECT header = { 0, 0, client.right, 110 };
        HBRUSH headerBrush = CreateSolidBrush(Colors::BURGUNDY_DARK);
        FillRect(hdc, &header, headerBrush);
        DeleteObject(headerBrush);

        // Нижняя граница шапки с тенью
        HPEN pen = CreatePen(PS_SOLID, 2, Colors::BURGUNDY_PRIMARY);
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        MoveToEx(hdc, 0, 110, NULL);
        LineTo(hdc, client.right, 110);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);

        // Тень под шапкой
        RECT shadowRect = { 0, 110, client.right, 115 };
        HBRUSH shadowBrush = CreateSolidBrush(RGB(20, 20, 25));
        FillRect(hdc, &shadowRect, shadowBrush);
        DeleteObject(shadowBrush);

        // Заголовок "Aegis Shield"
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, Colors::WHITE);
        HFONT oldFont = (HFONT)SelectObject(hdc, g_hTitleFont);
        TextOutW(hdc, 26, 24, L"🛡 Aegis Shield", 15);

        SelectObject(hdc, g_hSubtitleFont);
        SetTextColor(hdc, Colors::GRAY_LIGHT_TEXT);
        TextOutW(hdc, 26, 62, L"Ваша система под надежной защитой", 33);
        SelectObject(hdc, oldFont);

        EndPaint(hwnd, &ps);
    }
    return 0;
    case WM_CTLCOLORBTN:
{
    // Устанавливаем белый текст для кнопок
    SetTextColor((HDC)wParam, Colors::WHITE);
    SetBkMode((HDC)wParam, TRANSPARENT);
    
    // Возвращаем кисть для фона кнопки
    static HBRUSH hBtnBrush = CreateSolidBrush(Colors::BURGUNDY_PRIMARY);
    return (LRESULT)hBtnBrush;
}
    case WM_DESTROY:
        if (g_hTitleFont) DeleteObject(g_hTitleFont);
        if (g_hSubtitleFont) DeleteObject(g_hSubtitleFont);
        if (g_hButtonFont) DeleteObject(g_hButtonFont);
        if (g_pProcessMonitor) {
            delete g_pProcessMonitor;
            g_pProcessMonitor = nullptr;
        }
        if (g_pDownloadMonitor) {
            delete g_pDownloadMonitor;
            g_pDownloadMonitor = nullptr;
        }
        if (g_pSettingsWindow) {
            delete g_pSettingsWindow;
            g_pSettingsWindow = nullptr;
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

void CreateSettingsWindow() {
    if (g_pSettingsWindow) {
        delete g_pSettingsWindow;
    }
    
    g_pSettingsWindow = new SettingsWindow();
    
    if (g_pSettingsWindow) {
        bool result = g_pSettingsWindow->Create(g_hInstance, SW_SHOWNORMAL);
        
        if (result) {
            ShowWindow(g_hMainWnd, SW_HIDE);
        } else {
            MessageBoxW(NULL, 
                L"Не удалось создать окно настроек", 
                L"Ошибка", MB_OK | MB_ICONERROR);
            
            delete g_pSettingsWindow;
            g_pSettingsWindow = nullptr;
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
        L"Aegis Shield - Главное меню",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        960, 640,
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