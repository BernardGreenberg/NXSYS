#define  _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING 1  //Quel horreur...
#include <windows.h>
#include <string>
#include <vector>
#include <locale>
#include <codecvt>
#include <shellapi.h>
#include "ParseCommandLine.h"

/* Parsing windows console command lines is REALLY HARD, because there is no
   escape character (e.g., \ on Unix and Mac), amd the conventions for escaping
   quotes with more quotes are bizarre, and difficult to understand and get right.
   Microsoft has finally provided an API for it, CommandLineToArgv, but it is provided
   in Wide-char only, no multibyte version.
    
   But the entire codecvt system is deprecated in C++17, and there is no replacement for it!
   Microsoft recommends using the Windows multibyte functions, which would quadruple the
   length of this program. Given that there is no portable replacement, it is hard to believe
   that this deprecated interface will actually be pulled.
#define  _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING 1  //Quel horreur...
   tells the MS compiler that you understand and not to freak out.
*/

static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

ParsedCommandLine ParseCommandLineToVector(const char* command_line) {
	if (command_line == nullptr)
		return ParsedCommandLine{};
	std::wstring W = converter.from_bytes(command_line);
	if (W.length() == 0)
		return ParsedCommandLine{};  /* avoid buggy documented behavior of returning exe path*/
	int numArgs;
	LPWSTR* args = (W.c_str(), &numArgs);
	ParsedCommandLine pcl{};
	for (size_t i = 0; i < numArgs; i++)
		pcl.push_back(converter.to_bytes(args[i]));
	LocalFree(args);
	return pcl;
}
