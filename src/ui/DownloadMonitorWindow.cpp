#include "DownloadMonitorWindow.h"
#include "colors.h"
#include <sstream>
#include <fstream>
#include <algorithm>
#include <unordered_set>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

// Вспомогательные функции из scan_download.cpp
static std::wstring GetDownloadsPath() {
    wchar_t userProfile[MAX_PATH];
    if (GetEnvironmentVariableW(L"USERPROFILE", userProfile, MAX_PATH)) {
        std::wstring path = std::wstring(userProfile) + L"\\Downloads";
        return path;
    }
    return L".";
}

static bool HasTextExtension(const std::wstring& file) {
    std::wstring ext;
    size_t pos = file.find_last_of(L'.');
    if (pos != std::wstring::npos) ext = file.substr(pos);

    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);

    static const std::vector<std::wstring> allowed = {
        L".txt", L".py", L".bat", L".cmd", L".ps1", L".vbs", 
        L".js", L".cfg", L".ini", L".sh", L".php", L".pl"
    };

    return std::find(allowed.begin(), allowed.end(), ext) != allowed.end();
}

static std::wstring ToLower(const std::wstring& s) {
    std::wstring r = s;
    std::transform(r.begin(), r.end(), r.begin(), ::towlower);
    return r;
}

static bool FileContainsForbiddenPatterns(const std::wstring& path, std::wstring& matched) {
    if (!HasTextExtension(path)) return false;

    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open()) return false;

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    
    // Конвертируем в wstring для проверки
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, content.c_str(), (int)content.size(), NULL, 0);
    std::wstring wcontent(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, content.c_str(), (int)content.size(), &wcontent[0], size_needed);
    
    std::wstring lower = ToLower(wcontent);

    static const std::vector<std::wstring> forbidden = {
        L"os.delete", L"os.remove", L"rm -rf", L"system(", L"subprocess",
        L"powershell", L"invoke-webrequest", L"wget ", L"curl ", L"del ",
        L"deletefile", L"removedirectory", L"format c:", L"shutdown -s",
        L"shutdown /s", L"rmdir /s /q", L"drop table", L"eval(", L"exec(",
        L"shell.exec", L"malloc(", L"virtualallocex", L"createremotethread"
    };

    for (const auto& pat : forbidden) {
        if (lower.find(pat) != std::wstring::npos) {
            matched = pat;
            return true;
        }
    }
    return false;
}

// Реализация DownloadMonitorWindow
DownloadMonitorWindow::DownloadMonitorWindow()
    : hwnd(nullptr), hListView(nullptr), hButtonStart(nullptr), 
      hButtonStop(nullptr), hStatusLabel(nullptr), hBackButton(nullptr),
      isScanning(false), shouldStop(false) {
}

DownloadMonitorWindow::~DownloadMonitorWindow() {
    StopScanning();
}

LRESULT CALLBACK DownloadMonitorWindow::WndProcStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    DownloadMonitorWindow* pThis = nullptr;
    
    if (msg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (DownloadMonitorWindow*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->hwnd = hwnd;
    } else {
        pThis = (DownloadMonitorWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    
    if (pThis) {
        return pThis->HandleMessage(msg, wParam, lParam);
    }
    
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT DownloadMonitorWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
    {
        // Создаем статус
        hStatusLabel = CreateWindowW(L"STATIC", L"Статус: Остановлен",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, 10, 600, 20, hwnd, NULL, GetModuleHandleW(NULL), NULL);

        // Создаем ListView
        hListView = CreateWindowW(WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | WS_BORDER,
            10, 40, 760, 350,
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
        
        const wchar_t* headers[] = {
            L"Имя файла",
            L"Подозрительный шаблон",
            L"Путь",
            L"Время обнаружения"
        };
        
        int widths[] = {200, 200, 250, 150};
        
        for (int i = 0; i < 4; i++) {
            lvc.cx = widths[i];
            lvc.pszText = const_cast<wchar_t*>(headers[i]);
            ListView_InsertColumn(hListView, i, &lvc);
        }
        
        // Кнопки
        hButtonStart = CreateWindowW(L"BUTTON", L"▶ Запустить мониторинг",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, 400, 200, 35, hwnd, (HMENU)2, GetModuleHandleW(NULL), NULL);

        hButtonStop = CreateWindowW(L"BUTTON", L"⏸ Остановить мониторинг",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED,
            220, 400, 200, 35, hwnd, (HMENU)3, GetModuleHandleW(NULL), NULL);

        hBackButton = CreateWindowW(L"BUTTON", L"← Вернуться в главное меню",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            450, 400, 250, 35, hwnd, (HMENU)5, GetModuleHandleW(NULL), NULL);
        
        return 0;
    }
    
    case WM_COMMAND:
        if (LOWORD(wParam) == 2) {
            // Запустить мониторинг
            StartScanning();
        }
        else if (LOWORD(wParam) == 3) {
            // Остановить мониторинг
            StopScanning();
        }
        else if (LOWORD(wParam) == 4001) {
            // Удалить файл
            int selected = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
            if (selected >= 0) {
                DeleteSelectedFile(selected);
            }
        }
        else if (LOWORD(wParam) == 5) {
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
            if (pnmh->code == NM_RCLICK || pnmh->code == NM_DBLCLK) {
                LPNMITEMACTIVATE pnmia = (LPNMITEMACTIVATE)lParam;
                if (pnmia->iItem >= 0) {
                    ShowFileContextMenu(pnmia->iItem);
                }
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

        // Тень под шапкой
        RECT shadowRect = { 0, 60, client.right, 65 };
        HBRUSH shadowBrush = CreateSolidBrush(RGB(20, 20, 25));
        FillRect(hdc, &shadowRect, shadowBrush);
        DeleteObject(shadowBrush);

        // Заголовок
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, Colors::WHITE);
        HFONT hFont = CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
        TextOutW(hdc, 20, 20, L"📥 Мониторинг загрузок", 23);
        SelectObject(hdc, oldFont);
        DeleteObject(hFont);

        EndPaint(hwnd, &ps);
    }
    return 0;
    
    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        
        if (hStatusLabel) {
            MoveWindow(hStatusLabel, 10, 10, width - 20, 20, TRUE);
        }
        if (hListView) {
            MoveWindow(hListView, 10, 40, width - 20, height - 90, TRUE);
        }
        if (hButtonStart) {
            MoveWindow(hButtonStart, 10, height - 45, 200, 35, TRUE);
        }
        if (hButtonStop) {
            MoveWindow(hButtonStop, 220, height - 45, 200, 35, TRUE);
        }
        if (hBackButton) {
            MoveWindow(hBackButton, 450, height - 45, 250, 35, TRUE);
        }
        return 0;
    }
    
    case WM_DESTROY:
        StopScanning();
        return 0;
        
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

void DownloadMonitorWindow::UpdateListView() {
    if (!hListView) return;
    
    ListView_DeleteAllItems(hListView);
    
    std::lock_guard<std::mutex> lock(filesMutex);
    
    for (size_t i = 0; i < suspiciousFiles.size(); ++i) {
        const SuspiciousFile& file = suspiciousFiles[i];
        
        LVITEMW lvi = { 0 };
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = (int)i;
        lvi.lParam = (LPARAM)i;
        
        lvi.pszText = const_cast<LPWSTR>(file.fileName.c_str());
        int itemIndex = ListView_InsertItem(hListView, &lvi);
        
        if (itemIndex != -1) {
            ListView_SetItemText(hListView, itemIndex, 1, const_cast<LPWSTR>(file.pattern.c_str()));
            ListView_SetItemText(hListView, itemIndex, 2, const_cast<LPWSTR>(file.fullPath.c_str()));
            
            // Форматируем время
            SYSTEMTIME st;
            FileTimeToSystemTime(&file.detectedTime, &st);
            wchar_t timeStr[100];
            swprintf(timeStr, 100, L"%02d.%02d.%04d %02d:%02d:%02d",
                st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
            ListView_SetItemText(hListView, itemIndex, 3, timeStr);
        }
    }
}

void DownloadMonitorWindow::StartScanning() {
    if (isScanning) return;
    
    shouldStop = false;
    isScanning = true;
    
    EnableWindow(hButtonStart, FALSE);
    EnableWindow(hButtonStop, TRUE);
    SetWindowTextW(hStatusLabel, L"Статус: Сканирование активно...");
    
    scannerThread = std::thread(&DownloadMonitorWindow::ScannerThreadFunc, this);
}

void DownloadMonitorWindow::StopScanning() {
    if (!isScanning) return;
    
    shouldStop = true;
    
    if (scannerThread.joinable()) {
        scannerThread.join();
    }
    
    isScanning = false;
    
    EnableWindow(hButtonStart, TRUE);
    EnableWindow(hButtonStop, FALSE);
    SetWindowTextW(hStatusLabel, L"Статус: Остановлен");
}

void DownloadMonitorWindow::ScannerThreadFunc() {
    std::wstring downloads = GetDownloadsPath();
    std::unordered_set<std::wstring> seen;
    
    // Первоначальное сканирование существующих файлов
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((downloads + L"\\*").c_str(), &fd);
    
    int scannedCount = 0;
    int foundCount = 0;
    
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                std::wstring fp = downloads + L"\\" + fd.cFileName;
                seen.insert(fp);
                scannedCount++;
                
                // Проверяем существующий файл на подозрительные паттерны
                std::wstring matched;
                if (FileContainsForbiddenPatterns(fp, matched)) {
                    SuspiciousFile suspFile;
                    suspFile.fileName = fd.cFileName;
                    suspFile.fullPath = fp;
                    suspFile.pattern = matched;
                    GetSystemTimeAsFileTime(&suspFile.detectedTime);
                    
                    // Добавляем в список без показа MessageBox (чтобы не спамить)
                    {
                        std::lock_guard<std::mutex> lock(filesMutex);
                        suspiciousFiles.push_back(suspFile);
                    }
                    foundCount++;
                }
            }
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    
    // Обновляем UI после первоначального сканирования
    UpdateListView();
    
    // Показываем сводку первоначального сканирования
    if (scannedCount > 0) {
        std::wstringstream statusMsg;
        statusMsg << L"Статус: Сканирование активно | Проверено файлов: " << scannedCount;
        if (foundCount > 0) {
            statusMsg << L" | Найдено подозрительных: " << foundCount;
        }
        SetWindowTextW(hStatusLabel, statusMsg.str().c_str());
        
        if (foundCount > 0) {
            std::wstringstream notifyMsg;
            notifyMsg << L"Первоначальное сканирование завершено!\n\n"
                     << L"Проверено файлов: " << scannedCount << L"\n"
                     << L"Обнаружено подозрительных: " << foundCount << L"\n\n"
                     << L"Проверьте список подозрительных файлов.";
            MessageBoxW(hwnd, notifyMsg.str().c_str(), L"⚠ Результаты сканирования", 
                       MB_OK | MB_ICONWARNING | MB_TOPMOST);
        }
    }
    
    // Основной цикл мониторинга новых файлов
    while (!shouldStop) {
        HANDLE h2 = FindFirstFileW((downloads + L"\\*").c_str(), &fd);
        
        if (h2 != INVALID_HANDLE_VALUE) {
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    std::wstring fp = downloads + L"\\" + fd.cFileName;
                    
                    if (!seen.count(fp)) {
                        seen.insert(fp);
                        
                        std::wstring matched;
                        if (FileContainsForbiddenPatterns(fp, matched)) {
                            SuspiciousFile suspFile;
                            suspFile.fileName = fd.cFileName;
                            suspFile.fullPath = fp;
                            suspFile.pattern = matched;
                            GetSystemTimeAsFileTime(&suspFile.detectedTime);
                            
                            AddSuspiciousFile(suspFile);
                        }
                    }
                }
            } while (FindNextFileW(h2, &fd));
            FindClose(h2);
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

void DownloadMonitorWindow::AddSuspiciousFile(const SuspiciousFile& file) {
    int totalSuspicious = 0;
    
    {
        std::lock_guard<std::mutex> lock(filesMutex);
        suspiciousFiles.push_back(file);
        totalSuspicious = (int)suspiciousFiles.size();
    }
    
    // Обновляем статус
    std::wstringstream statusMsg;
    statusMsg << L"Статус: Сканирование активно | Найдено подозрительных: " << totalSuspicious;
    SetWindowTextW(hStatusLabel, statusMsg.str().c_str());
    
    // Обновляем UI
    UpdateListView();
    
    // Показываем уведомление
    std::wstringstream ss;
    ss << L"Обнаружен подозрительный файл!\n\n"
       << L"Файл: " << file.fileName << L"\n"
       << L"Шаблон: " << file.pattern << L"\n\n"
       << L"Проверьте список подозрительных файлов.";
    
    MessageBoxW(hwnd, ss.str().c_str(), L"⚠ Предупреждение", 
                MB_OK | MB_ICONWARNING | MB_TOPMOST);
}

void DownloadMonitorWindow::DeleteSelectedFile(int itemIndex) {
    // Копируем данные файла, чтобы не держать мьютекс во время показа MessageBox
    std::wstring fileName;
    std::wstring fullPath;
    std::wstring pattern;
    
    {
        std::lock_guard<std::mutex> lock(filesMutex);
        
        if (itemIndex < 0 || itemIndex >= (int)suspiciousFiles.size()) {
            return;
        }
        
        const SuspiciousFile& file = suspiciousFiles[itemIndex];
        fileName = file.fileName;
        fullPath = file.fullPath;
        pattern = file.pattern;
    }
    
    std::wstringstream ss;
    ss << L"Вы уверены, что хотите удалить этот файл?\n\n"
       << L"Файл: " << fileName << L"\n"
       << L"Путь: " << fullPath << L"\n"
       << L"Шаблон: " << pattern;
    
    int result = MessageBoxW(hwnd, ss.str().c_str(), L"Подтверждение удаления",
                             MB_YESNO | MB_ICONWARNING);
    
    if (result == IDYES) {
        if (DeleteFileW(fullPath.c_str())) {
            MessageBoxW(hwnd, L"Файл успешно удален!", L"Успех", 
                       MB_OK | MB_ICONINFORMATION);
            
            // Удаляем из списка
            {
                std::lock_guard<std::mutex> lock(filesMutex);
                
                // Проверяем индекс еще раз, т.к. список мог измениться
                if (itemIndex >= 0 && itemIndex < (int)suspiciousFiles.size()) {
                    suspiciousFiles.erase(suspiciousFiles.begin() + itemIndex);
                }
            }
            
            // Обновляем UI вне locked секции
            UpdateListView();
        } else {
            DWORD err = GetLastError();
            std::wstringstream errMsg;
            errMsg << L"Не удалось удалить файл.\nКод ошибки: " << err;
            MessageBoxW(hwnd, errMsg.str().c_str(), L"Ошибка", 
                       MB_OK | MB_ICONERROR);
        }
    }
}

void DownloadMonitorWindow::ShowFileContextMenu(int itemIndex) {
    // Копируем данные файла, чтобы не держать мьютекс во время показа меню
    std::wstring fileName;
    std::wstring fullPath;
    
    {
        std::lock_guard<std::mutex> lock(filesMutex);
        
        if (itemIndex < 0 || itemIndex >= (int)suspiciousFiles.size()) {
            return;
        }
        
        const SuspiciousFile& file = suspiciousFiles[itemIndex];
        fileName = file.fileName;
        fullPath = file.fullPath;
    }
    
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;
    
    std::wstring menuTitle = L"Файл: " + fileName;
    AppendMenuW(hMenu, MF_STRING | MF_DISABLED, 0, menuTitle.c_str());
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, 4001, L"🗑 Удалить файл");
    AppendMenuW(hMenu, MF_STRING, 4002, L"📁 Открыть расположение");
    
    POINT pt;
    GetCursorPos(&pt);
    
    int cmd = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, 
                             pt.x, pt.y, 0, hwnd, nullptr);
    
    if (cmd == 4001) {
        DeleteSelectedFile(itemIndex);
    }
    else if (cmd == 4002) {
        std::wstring params = L"/select,\"" + fullPath + L"\"";
        ShellExecuteW(hwnd, L"open", L"explorer.exe", params.c_str(), nullptr, SW_SHOWNORMAL);
    }
    
    DestroyMenu(hMenu);
}

bool DownloadMonitorWindow::Create(HINSTANCE hInstance, int nCmdShow) {
    HINSTANCE appInstance = hInstance ? hInstance : GetModuleHandleW(nullptr);
    
    UnregisterClassW(L"DownloadMonitorClass", appInstance);
    
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProcStatic;
    wc.hInstance = appInstance;
    wc.lpszClassName = L"DownloadMonitorClass";
    wc.cbWndExtra = sizeof(LONG_PTR);
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hIcon = LoadIconW(NULL, IDI_WARNING);
    wc.hIconSm = LoadIconW(NULL, IDI_WARNING);
    
    if (!RegisterClassExW(&wc)) {
        DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }
    }
    
    hwnd = CreateWindowW(
        L"DownloadMonitorClass",
        L"Мониторинг загрузок",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 500,
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

