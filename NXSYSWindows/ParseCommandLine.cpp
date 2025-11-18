#include <windows.h>
#include <string>
#include <vector>
#include <shellapi.h>
#include "ParseCommandLine.h"
#include <stringapiset.h>

ParsedCommandLine ParseCommandLineToVector(const char* command_line) {
    //Note that this is not Unicode, but CP_1251 or whatever.
  //  command_line = "phu -ébaz";
   // command_line = "x";
	if (command_line == nullptr)
		return ParsedCommandLine{};
	int nbcl = (int)strlen(command_line);
	if (nbcl == 0)
	    return ParsedCommandLine{}; //doc behavior is to get exe path.
	int nwide = MultiByteToWideChar(CP_ACP, 0, command_line, nbcl, NULL, 0);
	std::wstring W(nwide, 0);
	MultiByteToWideChar(CP_ACP, 0, command_line, nbcl, W.data(), nwide);
	int numArgs;
	LPWSTR* args = CommandLineToArgvW(W.c_str(), &numArgs);
	ParsedCommandLine pcl{};
	for (int i = 0; i < numArgs; i++) {
	    int wargl = lstrlenW(args[i]);
	    int nmb = WideCharToMultiByte(CP_ACP, 0, args[i], wargl, 0, 0, NULL, NULL);
	    std::string s_arg(nmb, 0);
	    WideCharToMultiByte(CP_ACP, 0, args[i], wargl, s_arg.data(), nmb, NULL, NULL);
	    pcl.push_back(s_arg);
	}
	LocalFree(args);
	return pcl;
}
