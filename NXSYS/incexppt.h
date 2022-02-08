#ifndef _BSG_INCLUDE_EXPAND_PATH_H__
#define _BSG_INCLUDE_EXPAND_PATH_H__
#include <string>

const char * include_expand_path (const char * basepath, const char* path, std::string& answer);
std::string STLincexppath (const std::string& basepath, const std::string& path);

#endif
