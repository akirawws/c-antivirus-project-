#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <windows.h>
#include <string>
#include <vector>

struct ProcessInfo {
    DWORD pid;
    std::string name;
    std::string path;
    DWORD parentPid;
};

#endif // COMMON_TYPES_H