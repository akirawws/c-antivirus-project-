#include "api/monitoring_h.cpp"
#include <iostream>

int main() {
    auto processes = get_process_list_snapshot();

    std::cout << "\nTotal processes: " << processes.size() << "\n";

    system("pause"); 
    return 0;
}
