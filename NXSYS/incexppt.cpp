#include "incexppt.h"
#include <filesystem>

// Gutted and replaced with std::filesystem 20 June 2022

using std::string;

string replace_filename(const string& basepath, const string& filename) {
    return std::filesystem::path(basepath).replace_filename(filename).string();
}

    
