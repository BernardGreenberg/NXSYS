#ifndef _NXSYS_SWITCH_CONSISTENCY_H__
#define _NXSYS_SWITCH_CONSISTENCY_H__
#include <string>
#include "ValidatingValue.h"

void SwitchConsistencyUndefine(long ID, int AB0);
bool SwitchConsistencyDefine(long ID, int AB0, std::string& complaint);
bool SwitchConsistencyDefineCheck(long ID, int AB0, std::string& complaint);
bool SwitchConsistencyTotalCheck(std::string& complaint);
void SwitchConsistencyClear();

/* Convenient Overloads */
ValidatingValue<std::string> SwitchConsistencyDefine(long ID, int AB0);
ValidatingValue<std::string> SwitchConsistencyDefineCheck(long ID, int AB0);
ValidatingValue<std::string> SwitchConsistencyTotalCheck();

#endif
