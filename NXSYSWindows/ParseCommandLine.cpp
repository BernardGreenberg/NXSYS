#define  _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING 1  //Quel horreur...
#include <windows.h>
#include <string>
#include <vector>
#include <locale>
#include <codecvt>
#include <shellapi.h>
#include "ParseCommandLine.h"

/* The entire codecvt system is deprecated in C++17, but there is no replacement for it!
   Microsoft recommends using the Windows multibyte functions, which would quadruple the
   length of this program.  The reason uses the "wide" version of CommandLineToArgv
   is that THERE IS NO MULTIBYTE VERSION. 
   */

static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

ParsedCommandLine ParseCommandLineToVector(const char* command_line) {
	if (command_line == nullptr)
		return ParsedCommandLine{};
	std::wstring W = converter.from_bytes(command_line);
	if (W.length() == 0)
		return ParsedCommandLine{};  /* avoid buggy documented behavior of returning exe path*/
	int numArgs;
	LPWSTR* args = CommandLineToArgvW(W.c_str(), &numArgs);
	ParsedCommandLine pcl{};
	for (size_t i = 0; i < numArgs; i++)
		pcl.push_back(converter.to_bytes(args[i]));
	LocalFree(args);
	return pcl;
}