@echo off
echo Compiling antivirus project...

echo Compiling WinMain.cpp...
g++ -std=c++17 -municode -c src/ui/WinMain.cpp -o WinMain.o

echo Compiling SettingsWindow.cpp...
g++ -std=c++17 -municode -c src/ui/SettingsWindow.cpp -o SettingsWindow.o

echo Compiling ProcessMonitorWindow.cpp...
g++ -std=c++17 -municode -c src/ui/ProcessMonitorWindow.cpp -o ProcessMonitorWindow.o

echo Compiling DownloadMonitorWindow.cpp...
g++ -std=c++17 -municode -c src/ui/DownloadMonitorWindow.cpp -o DownloadMonitorWindow.o

echo Compiling monitoring_h.cpp...
g++ -std=c++17 -municode -c src/api/monitoring_h.cpp -o monitoring_h.o

echo Compiling monitoring.cpp...
g++ -std=c++17 -municode -c src/security/monitoring.cpp -o monitoring.o

echo Compiling scan_download.cpp...
g++ -std=c++17 -municode -c src/security/scan_download.cpp -o scan_download.o

echo Linking all files...
g++ WinMain.o SettingsWindow.o ProcessMonitorWindow.o DownloadMonitorWindow.o monitoring_h.o monitoring.o scan_download.o -o antivirus.exe ^
    -mwindows -lpsapi -lcomctl32 -luser32 -lgdi32 -lshlwapi

if %errorlevel% equ 0 (
    echo.
    echo Success! Compiled antivirus.exe
    echo.
    antivirus.exe
) else (
    echo.
    echo Compilation failed!
    pause
)

del *.o