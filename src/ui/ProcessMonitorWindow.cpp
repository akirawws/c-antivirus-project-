#include "ProcessMonitorWindow.h"
#include <iomanip>
#include <sstream>
#include <shellapi.h>
#include <shlwapi.h>
#include "colors.h"

extern HFONT g_hSubtitleFont;

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
        // Получаем размер файла, если путь валиден
        WIN32_FILE_ATTRIBUTE_DATA fad{};
        if (GetFileAttributesExW(apiProc.path.c_str(), GetFileExInfoStandard, &fad)) {
            ULARGE_INTEGER li{};
            li.HighPart = fad.nFileSizeHigh;
            li.LowPart = fad.nFileSizeLow;
            proc.fileSizeBytes = li.QuadPart;
        } else {
            proc.fileSizeBytes = apiProc.memoryUsage * 1024ULL; // запасной вариант
        }
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
    : hwnd(nullptr), hListView(nullptr), hRefreshButton(nullptr),
      hOpenFolderButton(nullptr), hBackButton(nullptr),
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
            20, 70, 740, 350,
            hwnd, (HMENU)1, GetModuleHandleW(NULL), NULL);
        
        if (!hListView) {
            DWORD err = GetLastError();
            std::wstringstream ss;
            ss << L"ListView creation failed. GetLastError=" << err;
            MessageBoxW(hwnd, ss.str().c_str(), L"Aegis Shield", MB_OK | MB_ICONERROR);
            return -1;
        }
        
        // Устанавливаем стили ListView
        ListView_SetExtendedListViewStyle(hListView,
            LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_SUBITEMIMAGES);

        // Подготавливаем ImageList для иконок процессов
        hImageList = ImageList_Create(20, 20, ILC_COLOR32 | ILC_MASK, 16, 32);
        ListView_SetImageList(hListView, hImageList, LVSIL_SMALL);
        
        // Добавляем колонки
        LVCOLUMNW lvc = { 0 };
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
        lvc.fmt = LVCFMT_LEFT;
        
        // Заголовки колонок
        const wchar_t* headers[] = {
            L"",               // иконка
            L"Имя процесса",
            L"PID", 
            L"Путь к файлу",
            L"Размер",
            L"Статус"
        };
        
        int widths[] = {36, 180, 80, 280, 100, 100};
        
        for (int i = 0; i < 6; i++) {
            lvc.cx = widths[i];
            lvc.pszText = const_cast<wchar_t*>(headers[i]);
            ListView_InsertColumn(hListView, i, &lvc);
        }
        
        // Кнопки
        hRefreshButton = CreateWindowW(L"BUTTON", L"⟳ Обновить",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            20, 430, 160, 34,
            hwnd, (HMENU)2, GetModuleHandleW(NULL), NULL);

        hOpenFolderButton = CreateWindowW(L"BUTTON", L"📂 Открыть папку процесса",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED,
            190, 430, 240, 34,
            hwnd, (HMENU)3, GetModuleHandleW(NULL), NULL);

        hBackButton = CreateWindowW(L"BUTTON", L"← Вернуться в главное меню",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            450, 430, 250, 34,
            hwnd, (HMENU)4, GetModuleHandleW(NULL), NULL);
        
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
        else if (LOWORD(wParam) == 3) {
            // Открыть расположение
            if (selectedIndex >= 0) {
                OpenProcessLocation(selectedIndex);
            }
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
        else if (LOWORD(wParam) == 4) {
            // Вернуться в главное меню
            extern HWND g_hMainWnd;
            if (g_hMainWnd) {
                ShowWindow(g_hMainWnd, SW_SHOW);
                SetForegroundWindow(g_hMainWnd);
            }
            DestroyWindow(hwnd);
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
                    EnableWindow(hOpenFolderButton, TRUE);
                    ShowProcessContextMenu(pnmia->iItem);
                }
            }
            else if (pnmh->code == NM_RCLICK) {
                // Правый клик на элементе ListView
                LPNMITEMACTIVATE pnmia = (LPNMITEMACTIVATE)lParam;
                if (pnmia->iItem >= 0) {
                    selectedIndex = pnmia->iItem;
                    EnableWindow(hOpenFolderButton, TRUE);
                    ShowProcessContextMenu(pnmia->iItem);
                }
            }
            else if (pnmh->code == LVN_ITEMCHANGED) {
                NMLISTVIEW* pnmv = (NMLISTVIEW*)lParam;
                if ((pnmv->uChanged & LVIF_STATE) && (pnmv->uNewState & LVIS_SELECTED)) {
                    selectedIndex = pnmv->iItem;
                    EnableWindow(hOpenFolderButton, TRUE);
                } else if (!(pnmv->uNewState & LVIS_SELECTED)) {
                    int newSel = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
                    selectedIndex = newSel;
                    EnableWindow(hOpenFolderButton, newSel >= 0);
                }
            }
            else if (pnmh->code == NM_CUSTOMDRAW) {
                return HandleCustomDraw((LPNMLVCUSTOMDRAW)lParam);
            }
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
        TextOutW(hdc, 20, 20, L"🖥 Монитор процессов", 20);
        SelectObject(hdc, oldFont);
        DeleteObject(hFont);

        EndPaint(hwnd, &ps);
    }
    return 0;
        
    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        
        if (hListView) {
            MoveWindow(hListView, 20, 70, width - 40, height - 140, TRUE);
        }
        if (hRefreshButton) {
            MoveWindow(hRefreshButton, 20, height - 60, 160, 34, TRUE);
        }
        if (hOpenFolderButton) {
            MoveWindow(hOpenFolderButton, 190, height - 60, 240, 34, TRUE);
        }
        if (hBackButton) {
            MoveWindow(hBackButton, 450, height - 60, 250, 34, TRUE);
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
    if (hImageList) {
        ImageList_RemoveAll(hImageList);
    }
    selectedIndex = -1;
    EnableWindow(hOpenFolderButton, FALSE);
    
    const auto& processes = processManager.getProcesses();
    
    for (size_t i = 0; i < processes.size(); ++i) {
        const Process& proc = processes[i];
        
        // Добавляем элемент
        LVITEMW lvi = { 0 };
        lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
        lvi.iItem = (int)i;
        lvi.lParam = (LPARAM)i;

        int imageIndex = -1;
        if (hImageList && proc.icon) {
            imageIndex = ImageList_AddIcon(hImageList, proc.icon);
        }
        lvi.iImage = imageIndex;
        lvi.pszText = const_cast<LPWSTR>(L"");
        int itemIndex = ListView_InsertItem(hListView, &lvi);
        
        if (itemIndex != -1) {
            // Имя процесса
            ListView_SetItemText(hListView, itemIndex, 1, const_cast<LPWSTR>(proc.name.c_str()));
            
            // PID
            std::wstring pidStr = std::to_wstring(proc.pid);
            ListView_SetItemText(hListView, itemIndex, 2, const_cast<LPWSTR>(pidStr.c_str()));
            
            // Путь
            ListView_SetItemText(hListView, itemIndex, 3, const_cast<LPWSTR>(proc.filePath.c_str()));
            
            // Размер файла
            std::wstring sizeStr = FormatMemory(proc.fileSizeBytes);
            ListView_SetItemText(hListView, itemIndex, 4, const_cast<LPWSTR>(sizeStr.c_str()));
            
            // Статус
            std::wstring status = proc.suspicious ? L"Suspicious" : L"Normal";
            ListView_SetItemText(hListView, itemIndex, 5, const_cast<LPWSTR>(status.c_str()));
        }
    }
}

std::wstring ProcessMonitorWindow::FormatMemory(ULONGLONG bytes) const {
    std::wstringstream ss;

    const double KB = 1024.0;
    const double MB = KB * 1024.0;
    const double GB = MB * 1024.0;

    if (bytes < KB) {
        ss << bytes << L" B";
    }
    else if (bytes < MB) {
        ss << std::fixed << std::setprecision(1) << (bytes / KB) << L" KB";
    }
    else if (bytes < GB) {
        ss << std::fixed << std::setprecision(1) << (bytes / MB) << L" MB";
    }
    else {
        ss << std::fixed << std::setprecision(2) << (bytes / GB) << L" GB";
    }

    return ss.str();
}

LRESULT ProcessMonitorWindow::HandleCustomDraw(LPNMLVCUSTOMDRAW customDraw) {
    switch (customDraw->nmcd.dwDrawStage) {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT:
    {
        int idx = static_cast<int>(customDraw->nmcd.dwItemSpec);
        if (idx >= 0 && idx < (int)processManager.getProcesses().size()) {
            const Process& proc = processManager.getProcesses()[idx];

            if (proc.suspicious) {
                customDraw->clrText = Colors::ORANGE_ALERT;
                customDraw->clrTextBk = Colors::ORANGE_LIGHT;
            } else {
                // Чередуем темный/бардовый, чтобы списки читались проще
                bool isEven = (idx % 2 == 0);
                if (isEven) {
                    customDraw->clrText = Colors::GRAY_LIGHT_TEXT;
                    customDraw->clrTextBk = Colors::DARK_PANEL;
                } else {
                    customDraw->clrText = Colors::WHITE;
                    customDraw->clrTextBk = Colors::DARK_BG;
                }
            }
        }
        return CDRF_NEWFONT;
    }
    default:
        return CDRF_DODEFAULT;
    }
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
    wc.hbrBackground = CreateSolidBrush(Colors::DARK_BG);
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
            MessageBoxW(nullptr, ss.str().c_str(), L"Aegis Shield", MB_OK | MB_ICONERROR);
            return false;
        }
    }

    // Создаем окно
    hwnd = CreateWindowW(
        L"ProcessMonitorClassV2",
        L"Aegis Shield - Монитор процессов",
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