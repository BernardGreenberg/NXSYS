#pragma once

#include <vector>
#include <string>

typedef std::vector<std::string> ParsedCommandLine;

ParsedCommandLine ParseCommandLineToVector(const char* command_line);
