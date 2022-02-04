#pragma once

void ClearHelpMenu ();
void RegisterHelpMenuText (const char * text, const char * title);
void DisplayHelpTextByCommand (UINT Cmd);
void HelpDialog (const char * text, const char * title);
void RegisterInHelpfileMenuItem (const char * title, UINT helpfile_id);
#ifdef NXSYSMac
void RegisterHelpURL(const char * title, const char * URL);
#endif


