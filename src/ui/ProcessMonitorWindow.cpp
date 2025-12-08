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
        // ВАЖНО: присваиваем hwnd до вызова HandleMessage, 
        // чтобы дочерние окна могли создаваться с валидным родителем
        pThis->hwnd = hwnd;
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
            DWORD err = GetLastError();
            std::wstringstream ss;
            ss << L"ListView creation failed. GetLastError=" << err;
            MessageBoxW(hwnd, ss.str().c_str(), L"Process Monitor", MB_OK | MB_ICONERROR);
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
        else if (LOWORD(wParam) == 3001) {
            // Завершить процесс
            if (selectedIndex >= 0) {
                TerminateSelectedProcess(selectedIndex);
            }
        }
        else if (LOWORD(wParam) == 3002) {
            // Перейти к процессу
            if (selectedIndex >= 0) {
                OpenProcessLocation(selectedIndex);
            }
        }
        return 0;
    
    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        if (pnmh->hwndFrom == hListView) {
            if (pnmh->code == NM_DBLCLK) {
                // Двойной клик на элементе ListView
                LPNMITEMACTIVATE pnmia = (LPNMITEMACTIVATE)lParam;
                if (pnmia->iItem >= 0) {
                    selectedIndex = pnmia->iItem;
                    ShowProcessContextMenu(pnmia->iItem);
                }
            }
            else if (pnmh->code == NM_RCLICK) {
                // Правый клик на элементе ListView
                LPNMITEMACTIVATE pnmia = (LPNMITEMACTIVATE)lParam;
                if (pnmia->iItem >= 0) {
                    selectedIndex = pnmia->iItem;
                    ShowProcessContextMenu(pnmia->iItem);
                }
            }
        }
        return 0;
    }
        
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
    // Убедимся, что есть валидный hInstance
    HINSTANCE appInstance = hInstance ? hInstance : GetModuleHandleW(nullptr);

    // Перерегистрируем класс окна, чтобы гарантировать cbWndExtra
    UnregisterClassW(L"ProcessMonitorClassV2", appInstance); // игнорируем ошибку, если не было

    // Регистрируем класс окна
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProcStatic;
    wc.hInstance = appInstance;
    wc.lpszClassName = L"ProcessMonitorClassV2";
    wc.cbWndExtra = sizeof(LONG_PTR); // запас под GWLP_USERDATA
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIconW(NULL, IDI_APPLICATION);

    ATOM atom = RegisterClassExW(&wc);
    if (atom == 0) {
        DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) {
            LPWSTR msgBuf = nullptr;
            FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPWSTR)&msgBuf, 0, nullptr);

            std::wstringstream ss;
            ss << L"RegisterClassExW failed. GetLastError=" << err;
            if (msgBuf) {
                ss << L"\n" << msgBuf;
                LocalFree(msgBuf);
            }
            MessageBoxW(nullptr, ss.str().c_str(), L"Process Monitor", MB_OK | MB_ICONERROR);
            return false;
        }
    }

    // Создаем окно
    hwnd = CreateWindowW(
        L"ProcessMonitorClassV2",
        L"Process Monitor",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 500,
        NULL,
        NULL,
        appInstance,
        this 
    );

    if (!hwnd) {
        DWORD err = GetLastError();
        LPWSTR msgBuf = nullptr;
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&msgBuf, 0, nullptr);

        std::wstringstream ss;
        ss << L"CreateWindowW failed. GetLastError=" << err
           << L"\nappInstance=" << appInstance;
        if (msgBuf) {
            ss << L"\n" << msgBuf;
            LocalFree(msgBuf);
        }
        MessageBoxW(nullptr, ss.str().c_str(), L"Process Monitor", MB_OK | MB_ICONERROR);
        return false;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    return true;
}

void ProcessMonitorWindow::ShowProcessContextMenu(int itemIndex) {
    if (itemIndex < 0 || itemIndex >= (int)processManager.getProcesses().size()) {
        return;
    }

    const Process& proc = processManager.getProcesses()[itemIndex];

    // Создаем контекстное меню
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    std::wstring menuTitle = L"Процесс: " + proc.name;
    AppendMenuW(hMenu, MF_STRING | MF_DISABLED, 0, menuTitle.c_str());
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, 3001, L"🛑 Завершить процесс");
    AppendMenuW(hMenu, MF_STRING, 3002, L"📁 Перейти к процессу");

    // Получаем позицию курсора
    POINT pt;
    GetCursorPos(&pt);

    // Показываем меню
    TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, 0, hwnd, nullptr);
    DestroyMenu(hMenu);
}

void ProcessMonitorWindow::TerminateSelectedProcess(int itemIndex) {
    if (itemIndex < 0 || itemIndex >= (int)processManager.getProcesses().size()) {
        return;
    }

    const Process& proc = processManager.getProcesses()[itemIndex];

    // Подтверждение
    std::wstringstream ss;
    ss << L"Вы уверены, что хотите завершить процесс?\n\n"
       << L"Имя: " << proc.name << L"\n"
       << L"PID: " << proc.pid << L"\n"
       << L"Путь: " << proc.filePath;

    int result = MessageBoxW(hwnd, ss.str().c_str(), L"Подтверждение", 
                             MB_YESNO | MB_ICONWARNING);

    if (result == IDYES) {
        // Открываем процесс с правами на завершение
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, proc.pid);
        if (hProcess) {
            if (TerminateProcess(hProcess, 0)) {
                MessageBoxW(hwnd, L"Процесс успешно завершен!", L"Успех", 
                           MB_OK | MB_ICONINFORMATION);
                
                // Обновляем список
                processManager.fetchProcesses();
                UpdateListView();
            } else {
                DWORD err = GetLastError();
                std::wstringstream errMsg;
                errMsg << L"Не удалось завершить процесс.\nКод ошибки: " << err
                       << L"\n\nВозможно, требуются права администратора.";
                MessageBoxW(hwnd, errMsg.str().c_str(), L"Ошибка", 
                           MB_OK | MB_ICONERROR);
            }
            CloseHandle(hProcess);
        } else {
            DWORD err = GetLastError();
            std::wstringstream errMsg;
            errMsg << L"Не удалось открыть процесс.\nКод ошибки: " << err
                   << L"\n\nВозможно, требуются права администратора.";
            MessageBoxW(hwnd, errMsg.str().c_str(), L"Ошибка", 
                       MB_OK | MB_ICONERROR);
        }
    }
}

void ProcessMonitorWindow::OpenProcessLocation(int itemIndex) {
    if (itemIndex < 0 || itemIndex >= (int)processManager.getProcesses().size()) {
        return;
    }

    const Process& proc = processManager.getProcesses()[itemIndex];

    // Извлекаем путь к папке
    std::wstring folderPath = proc.filePath;
    size_t lastSlash = folderPath.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        folderPath = folderPath.substr(0, lastSlash);
    }

    // Открываем проводник и выделяем файл
    std::wstring params = L"/select,\"" + proc.filePath + L"\"";
    
    HINSTANCE result = ShellExecuteW(hwnd, L"open", L"explorer.exe", 
                                     params.c_str(), nullptr, SW_SHOWNORMAL);

    if ((INT_PTR)result <= 32) {
        // Если не удалось выделить файл, просто открываем папку
        result = ShellExecuteW(hwnd, L"open", folderPath.c_str(), 
                              nullptr, nullptr, SW_SHOWNORMAL);
        
        if ((INT_PTR)result <= 32) {
            MessageBoxW(hwnd, L"Не удалось открыть расположение процесса.", 
                       L"Ошибка", MB_OK | MB_ICONERROR);
        }
    }
}