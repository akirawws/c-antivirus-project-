#include "scan_dowload.h"

#include <filesystem>
#include <string>
#include <vector>
#include <unordered_set>
#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <cctype>
#include <cstdlib>

using namespace std;

static filesystem::path get_downloads_path() {
    char* userProfile = nullptr;
    size_t len = 0;
    filesystem::path result;
    if (_dupenv_s(&userProfile, &len, "USERPROFILE") == 0 && userProfile) {
        result = filesystem::path(userProfile) / "Downloads";
        free(userProfile);
    }
    if (result.empty()) {
        result = filesystem::current_path();
    }
    return result;
}

static bool has_text_extension(const filesystem::path& p) {
    string ext = p.extension().string();
    for (auto& c : ext) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    if (ext == ".txt") return true;
    if (ext == ".py") return true;
    if (ext == ".bat") return true;
    if (ext == ".cmd") return true;
    if (ext == ".ps1") return true;
    if (ext == ".vbs") return true;
    if (ext == ".js") return true;
    if (ext == ".cfg") return true;
    if (ext == ".ini") return true;
    return false;
}

static string to_lower(const string& s) {
    string r = s;
    for (char& c : r) {
        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    }
    return r;
}

static bool file_contains_forbidden_patterns(const filesystem::path& p, string& matched_pattern) {
    if (!has_text_extension(p)) {
        return false;
    }

    ifstream in(p, ios::binary);
    if (!in.is_open()) {
        return false;
    }

    string content((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
    string lower = to_lower(content);

    static const vector<string> forbidden = {
        "os.delete",
        "os.remove",
        "rm -rf",
        "system(",
        "subprocess",
        "powershell",
        "invoke-webrequest",
        "wget ",
        "curl ",
        "del ",
        "deletefile",
        "removedirectory",
        "format c:",
        "shutdown -s",
        "shutdown /s",
        "rmdir /s /q",
        "drop table",
        "eval(",
        "exec(",
        "shell.exec",
        "malloc(",
        "virtualallocex",
        "createremotethread"
    };

    for (const auto& pattern : forbidden) {
        if (lower.find(pattern) != string::npos) {
            matched_pattern = pattern;
            return true;
        }
    }

    return false;
}

void run_downloads_scanner() {
    filesystem::path downloads = get_downloads_path();
    if (!filesystem::exists(downloads) || !filesystem::is_directory(downloads)) {
        cout << "Downloads folder not found: " << downloads.string() << endl;
        cout << "Press ENTER to exit...";
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cin.get();
        return;
    }

    cout << "Downloads scanner started in: " << downloads.string() << endl;
    cout << "New files will be checked. Press Ctrl+C to stop." << endl;

    unordered_set<string> seen;

    for (const auto& entry : filesystem::directory_iterator(downloads)) {
        if (!entry.is_regular_file()) continue;
        seen.insert(entry.path().string());
    }

    while (true) {
        for (const auto& entry : filesystem::directory_iterator(downloads)) {
            if (!entry.is_regular_file()) continue;
            const auto path = entry.path();
            string pathStr = path.string();

            if (seen.find(pathStr) != seen.end()) {
                continue;
            }

            seen.insert(pathStr);

            cout << "\nNew file detected: " << pathStr << endl;

            string matched;
            if (file_contains_forbidden_patterns(path, matched)) {
                cout << "Suspicious content detected: \"" << matched << "\"" << endl;
                cout << "Delete this file? [y/N]: ";
                char choice = 0;
                cin >> choice;
                if (choice == 'y' || choice == 'Y') {
                    error_code ec;
                    filesystem::remove(path, ec);
                    if (ec) {
                        cout << "Failed to delete file: " << ec.message() << endl;
                    } else {
                        cout << "File deleted." << endl;
                    }
                } else {
                    cout << "File kept." << endl;
                }
            } else {
                cout << "No forbidden patterns found." << endl;
            }
        }

        this_thread::sleep_for(chrono::seconds(2));
    }
}
