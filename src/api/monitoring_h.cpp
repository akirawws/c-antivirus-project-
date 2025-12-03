#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct ProcessInfo {
    std::string icon;      
    std::string name;
    std::uint32_t pid;
    std::string file_path;
    std::uint64_t size;
    bool suspicious;        
};

std::vector<ProcessInfo> get_process_list_snapshot();
