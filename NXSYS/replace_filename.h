#ifndef _REPLACE_FILENAME_H__
#define _REPLACE_FILENAME_H__
#include <string>
#include <filesystem>

inline std::string replace_filename(const std::string& basepath, const std::string& filename) {
    return std::filesystem::path(basepath).replace_filename(filename).string();
}


#endif
