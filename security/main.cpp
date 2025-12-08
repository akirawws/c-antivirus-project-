#include "../api/monitoring_h.h"
#include "monitoring_keylogger.h"
#include <iostream>
#include <limits>

using namespace std;

// Простая функция для сканирования загрузок (заглушка)
void run_downloads_scanner() {
    cout << "\nDownloads scanner (not implemented yet)\n";
    system("pause");
}

int main() {
    cout << "=== Security Monitoring System ===\n\n";
    cout << "Select mode:\n";
    cout << "1 - Process monitoring\n";
    cout << "2 - Background Downloads scanner\n";
    cout << "3 - Keylogger detection scanner\n";
    cout << "Choice: ";

    int choice = 0;
    cin >> choice;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    if (choice == 1) {
        auto processes = get_process_list_snapshot();
        cout << "\nTotal processes: " << processes.size() << "\n";
        
        int count = 0;
        for (const auto& proc : processes) {
            if (count++ < 10) {
                cout << proc.name << " (PID: " << proc.pid << ")\n";
            }
        }
        if (processes.size() > 10) {
            cout << "... and " << (processes.size() - 10) << " more\n";
        }
        
        system("pause");
    } else if (choice == 2) {
        run_downloads_scanner();
    } else if (choice == 3) {
        run_keylogger_scanner();
    } else {
        cout << "Unknown option.\n";
        system("pause");
    }

    return 0;
}