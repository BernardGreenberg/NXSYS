#pragma once
#ifdef __cplusplus
extern "C" {
#endif

char** ParseArgString(char* arg);
void ParseArgsFree(char** argv);
int ParseArgsArgCount(char** argv);

#ifdef __cplusplus
}
#endif