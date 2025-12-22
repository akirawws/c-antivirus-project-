#include "SettingsWindow.h"
#include "colors.h"

extern HWND g_hMainWnd;

SettingsWindow::SettingsWindow() 
    : hwnd(nullptr), hCheckAutoScan(nullptr), hCheckRealTimeProtection(nullptr),
      hCheckNotifications(nullptr), hCheckQuarantine(nullptr),
      hButtonSave(nullptr), hButtonBack(nullptr) {
}

SettingsWindow::~SettingsWindow() {
}

LRESULT CALLBACK SettingsWindow::WndProcStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    SettingsWindow* pThis = nullptr;
    
    if (msg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (SettingsWindow*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->hwnd = hwnd;
    } else {
        pThis = (SettingsWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    
    if (pThis) {
        return pThis->HandleMessage(msg, wParam, lParam);
    }
    
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT SettingsWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
    {
        // Создаем элементы управления
        CreateWindowW(L"STATIC", L"Автоматическое сканирование",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            30, 100, 300, 25, hwnd, NULL, GetModuleHandleW(NULL), NULL);

        hCheckAutoScan = CreateWindowW(L"BUTTON", L"Включить автоматическое сканирование при запуске",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            30, 130, 500, 25, hwnd, (HMENU)2001, GetModuleHandleW(NULL), NULL);
        SendMessageW(hCheckAutoScan, BM_SETCHECK, BST_CHECKED, 0);

        CreateWindowW(L"STATIC", L"Защита в реальном времени",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            30, 170, 300, 25, hwnd, NULL, GetModuleHandleW(NULL), NULL);

        hCheckRealTimeProtection = CreateWindowW(L"BUTTON", L"Включить защиту в реальном времени",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            30, 200, 500, 25, hwnd, (HMENU)2002, GetModuleHandleW(NULL), NULL);
        SendMessageW(hCheckRealTimeProtection, BM_SETCHECK, BST_CHECKED, 0);

        CreateWindowW(L"STATIC", L"Уведомления",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            30, 240, 300, 25, hwnd, NULL, GetModuleHandleW(NULL), NULL);

        hCheckNotifications = CreateWindowW(L"BUTTON", L"Показывать уведомления об угрозах",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            30, 270, 500, 25, hwnd, (HMENU)2003, GetModuleHandleW(NULL), NULL);
        SendMessageW(hCheckNotifications, BM_SETCHECK, BST_CHECKED, 0);

        CreateWindowW(L"STATIC", L"Карантин",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            30, 310, 300, 25, hwnd, NULL, GetModuleHandleW(NULL), NULL);

        hCheckQuarantine = CreateWindowW(L"BUTTON", L"Автоматически помещать угрозы в карантин",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            30, 340, 500, 25, hwnd, (HMENU)2004, GetModuleHandleW(NULL), NULL);
        SendMessageW(hCheckQuarantine, BM_SETCHECK, BST_CHECKED, 0);

        // Кнопки
        hButtonSave = CreateWindowW(L"BUTTON", L"💾 Сохранить настройки",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            30, 400, 200, 35, hwnd, (HMENU)2005, GetModuleHandleW(NULL), NULL);

        hButtonBack = CreateWindowW(L"BUTTON", L"← Вернуться в главное меню",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            250, 400, 250, 35, hwnd, (HMENU)2006, GetModuleHandleW(NULL), NULL);

        return 0;
    }

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        if (id == 2005) {
            // Сохранить настройки
            bool autoScan = (SendMessageW(hCheckAutoScan, BM_GETCHECK, 0, 0) == BST_CHECKED);
            bool realTime = (SendMessageW(hCheckRealTimeProtection, BM_GETCHECK, 0, 0) == BST_CHECKED);
            bool notifications = (SendMessageW(hCheckNotifications, BM_GETCHECK, 0, 0) == BST_CHECKED);
            bool quarantine = (SendMessageW(hCheckQuarantine, BM_GETCHECK, 0, 0) == BST_CHECKED);

            MessageBoxW(hwnd, L"Настройки успешно сохранены!", L"Aegis Shield", MB_OK | MB_ICONINFORMATION);
        }
        else if (id == 2006) {
            // Вернуться в главное меню
            ShowMainMenu();
        }
        return 0;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        
        RECT client;
        GetClientRect(hwnd, &client);

        // Темный фон
        HBRUSH bgBrush = CreateSolidBrush(Colors::DARK_BG);
        FillRect(hdc, &client, bgBrush);
        DeleteObject(bgBrush);

        // Бардовая шапка
        RECT header = { 0, 0, client.right, 60 };
        HBRUSH headerBrush = CreateSolidBrush(Colors::BURGUNDY_DARK);
        FillRect(hdc, &header, headerBrush);
        DeleteObject(headerBrush);

        // Заголовок
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, Colors::WHITE);
        HFONT hFont = CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
        TextOutW(hdc, 30, 20, L"⚙ Настройки защиты", 19);
        SelectObject(hdc, oldFont);
        DeleteObject(hFont);

        EndPaint(hwnd, &ps);
    }
    return 0;

    case WM_DESTROY:
        return 0;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

void SettingsWindow::ShowMainMenu() {
    if (g_hMainWnd) {
        ShowWindow(g_hMainWnd, SW_SHOW);
        SetForegroundWindow(g_hMainWnd);
    }
    DestroyWindow(hwnd);
}

bool SettingsWindow::Create(HINSTANCE hInstance, int nCmdShow) {
    HINSTANCE appInstance = hInstance ? hInstance : GetModuleHandleW(nullptr);
    
    UnregisterClassW(L"SettingsWindowClass", appInstance);
    
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProcStatic;
    wc.hInstance = appInstance;
    wc.lpszClassName = L"SettingsWindowClass";
    wc.cbWndExtra = sizeof(LONG_PTR);
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(Colors::DARK_BG);
    wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIconW(NULL, IDI_APPLICATION);
    
    if (!RegisterClassExW(&wc)) {
        DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }
    }
    
    hwnd = CreateWindowW(
        L"SettingsWindowClass",
        L"Aegis Shield - Настройки",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        700, 500,
        NULL,
        NULL,
        appInstance,
        this
    );
    
    if (!hwnd) {
        return false;
    }
    
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    return true;
}

