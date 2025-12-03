#include "api/monitoring_h.cpp"
#include "security/scan_dowload.h"

#include <iostream>
#include <limits>

using namespace std;

int main() {
    cout << "Select mode:\n";
    cout << "1 - Process snapshot\n";
    cout << "2 - Background Downloads scanner\n";
    cout << "Choice: ";

    int choice = 0;
    cin >> choice;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    if (choice == 1) {
        auto processes = get_process_list_snapshot();
        cout << "\nTotal processes: " << processes.size() << "\n";
        system("pause");
    } else if (choice == 2) {
        run_downloads_scanner();
    } else {
        cout << "Unknown option.\n";
        system("pause");
    }

    return 0;
}
