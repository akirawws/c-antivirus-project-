#include "scan_download.h"

#include <windows.h>
#include <string>
#include <vector>
#include <unordered_set>
#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cctype>

using namespace std;

static string get_downloads_path() {
    char userProfile[MAX_PATH];
    if (GetEnvironmentVariableA("USERPROFILE", userProfile, MAX_PATH)) {
        string path = string(userProfile) + "\\Downloads";
        return path;
    }
    return ".";
}

static bool has_text_extension(const string& file) {
    string ext;
    size_t pos = file.find_last_of('.');
    if (pos != string::npos) ext = file.substr(pos);

    transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    static const vector<string> allowed = {
        ".txt",".py",".bat",".cmd",".ps1",".vbs",".js",".cfg",".ini"
    };

    return find(allowed.begin(), allowed.end(), ext) != allowed.end();
}

static string to_lower(const string& s) {
    string r = s;
    transform(r.begin(), r.end(), r.begin(), ::tolower);
    return r;
}

static bool file_contains_forbidden_patterns(const string& path, string& matched) {
    if (!has_text_extension(path)) return false;

    ifstream file(path, ios::binary);
    if (!file.is_open()) return false;

    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    string lower = to_lower(content);

    static const vector<string> forbidden = {
        "os.delete","os.remove","rm -rf","system(","subprocess",
        "powershell","invoke-webrequest","wget ","curl ","del ",
        "deletefile","removedirectory","format c:","shutdown -s",
        "shutdown /s","rmdir /s /q","drop table","eval(","exec(",
        "shell.exec","malloc(","virtualallocex","createremotethread"
    };

    for (const auto& pat : forbidden) {
        if (lower.find(pat) != string::npos) {
            matched = pat;
            return true;
        }
    }
    return false;
}

void run_downloads_scanner() {
    string downloads = get_downloads_path();

    cout << "Watching folder: " << downloads << endl;

    unordered_set<string> seen;

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA((downloads + "\\*").c_str(), &fd);

    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                string fp = downloads + "\\" + fd.cFileName;
                seen.insert(fp);
            }
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }

    while (true) {
        HANDLE h2 = FindFirstFileA((downloads + "\\*").c_str(), &fd);

        if (h2 != INVALID_HANDLE_VALUE) {
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    string fp = downloads + "\\" + fd.cFileName;

                    if (!seen.count(fp)) {
                        seen.insert(fp);
                        cout << "\nNew file: " << fp << endl;

                        string matched;
                        if (file_contains_forbidden_patterns(fp, matched)) {
                            cout << "Suspicious pattern: " << matched << endl;
                            cout << "Delete file? [y/N]: ";
                            char c; cin >> c;

                            if (c == 'y' || c == 'Y') {
                                if (DeleteFileA(fp.c_str()))
                                    cout << "File deleted.\n";
                                else
                                    cout << "Delete failed.\n";
                            }
                        } else {
                            cout << "Safe.\n";
                        }
                    }
                }
            } while (FindNextFileA(h2, &fd));
            FindClose(h2);
        }

        this_thread::sleep_for(chrono::seconds(2));
    }
}