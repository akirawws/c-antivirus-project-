#include "ProcessMonitorWindow.h"
#include <iomanip>
#include <sstream>
#include <shellapi.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

// Реализация ProcessManager
void ProcessManager::fetchProcesses() {
    processes.clear();
    
    // Получаем данные из API
    std::vector<ProcessInfo> apiProcesses = ProcessMonitorAPI::GetAllProcesses();
    
    // Конвертируем в нашу структуру
    for (const auto& apiProc : apiProcesses) {
        Process proc;
        proc.pid = apiProc.pid;
        proc.name = apiProc.name;
        proc.filePath = apiProc.path;
        proc.memoryKB = apiProc.memoryUsage;
        proc.suspicious = apiProc.isSuspicious;
        proc.icon = apiProc.icon;
        processes.push_back(proc);
    }
}

const std::vector<Process>& ProcessManager::getProcesses() const {
    return processes;
}

// Реализация ProcessMonitorWindow
ProcessMonitorWindow::ProcessMonitorWindow() 
    : hwnd(nullptr), hListView(nullptr), hButton(nullptr), 
      hImageList(nullptr), selectedIndex(-1) {
}

ProcessMonitorWindow::~ProcessMonitorWindow() {
    if (hImageList) {
        ImageList_Destroy(hImageList);
    }
}

LRESULT CALLBACK ProcessMonitorWindow::WndProcStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ProcessMonitorWindow* pThis = nullptr;
    
    if (msg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (ProcessMonitorWindow*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = (ProcessMonitorWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    
    if (pThis) {
        return pThis->HandleMessage(msg, wParam, lParam);
    }
    
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT ProcessMonitorWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
    {
        // Создаем ListView
        hListView = CreateWindowW(WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | WS_BORDER,
            10, 10, 600, 300,
            hwnd, (HMENU)1, GetModuleHandleW(NULL), NULL);
        
        if (!hListView) {
            return -1;
        }
        
        // Устанавливаем стили ListView
        ListView_SetExtendedListViewStyle(hListView,
            LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        
        // Добавляем колонки
        LVCOLUMNW lvc = { 0 };
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
        lvc.fmt = LVCFMT_LEFT;
        
        // Заголовки колонок
        const wchar_t* headers[] = {
            L"Process Name",
            L"PID", 
            L"Path",
            L"Memory",
            L"Status"
        };
        
        int widths[] = {150, 80, 250, 80, 100};
        
        for (int i = 0; i < 5; i++) {
            lvc.cx = widths[i];
            lvc.pszText = const_cast<wchar_t*>(headers[i]);
            ListView_InsertColumn(hListView, i, &lvc);
        }
        
        // Кнопка
        hButton = CreateWindowW(L"BUTTON", L"Обновить",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, 320, 150, 30,
            hwnd, (HMENU)2, GetModuleHandleW(NULL), NULL);
        
        // Загружаем процессы
        processManager.fetchProcesses();
        UpdateListView();
        
        return 0;
    }
    
    case WM_COMMAND:
        if (LOWORD(wParam) == 2) {
            // Обновить список
            processManager.fetchProcesses();
            UpdateListView();
        }
        return 0;
        
    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        
        if (hListView) {
            MoveWindow(hListView, 10, 10, width - 20, height - 100, TRUE);
        }
        if (hButton) {
            MoveWindow(hButton, 10, height - 80, 150, 30, TRUE);
        }
        return 0;
    }
    
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
        
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

void ProcessMonitorWindow::UpdateListView() {
    if (!hListView) return;
    
    ListView_DeleteAllItems(hListView);
    
    const auto& processes = processManager.getProcesses();
    
    for (size_t i = 0; i < processes.size(); ++i) {
        const Process& proc = processes[i];
        
        // Добавляем элемент
        LVITEMW lvi = { 0 };
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = (int)i;
        lvi.lParam = (LPARAM)i;
        
        // Имя процесса
        lvi.pszText = const_cast<LPWSTR>(proc.name.c_str());
        int itemIndex = ListView_InsertItem(hListView, &lvi);
        
        if (itemIndex != -1) {
            // PID
            std::wstring pidStr = std::to_wstring(proc.pid);
            ListView_SetItemText(hListView, itemIndex, 1, const_cast<LPWSTR>(pidStr.c_str()));
            
            // Путь
            ListView_SetItemText(hListView, itemIndex, 2, const_cast<LPWSTR>(proc.filePath.c_str()));
            
            // Использование памяти
            std::wstring memoryStr = FormatMemory(proc.memoryKB);
            ListView_SetItemText(hListView, itemIndex, 3, const_cast<LPWSTR>(memoryStr.c_str()));
            
            // Статус
            std::wstring status = proc.suspicious ? L"Suspicious" : L"Normal";
            ListView_SetItemText(hListView, itemIndex, 4, const_cast<LPWSTR>(status.c_str()));
        }
    }
}

std::wstring ProcessMonitorWindow::FormatMemory(DWORD memoryKB) const {
    std::wstringstream ss;
    
    if (memoryKB < 1024) {
        ss << memoryKB << L" KB";
    }
    else if (memoryKB < 1024 * 1024) {
        ss << std::fixed << std::setprecision(1) << (memoryKB / 1024.0) << L" MB";
    }
    else {
        ss << std::fixed << std::setprecision(2) << (memoryKB / (1024.0 * 1024.0)) << L" GB";
    }
    
    return ss.str();
}

bool ProcessMonitorWindow::Create(HINSTANCE hInstance, int nCmdShow) {
    // Регистрируем класс окна
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProcStatic;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ProcessMonitorClass";
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);

    if (!RegisterClassW(&wc)) {
        return false;
    }

    // Создаем окно
    hwnd = CreateWindowW(
        L"ProcessMonitorClass",
        L"Process Monitor",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 500,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hwnd) {
        return false;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    return true;
}