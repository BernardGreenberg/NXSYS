#ifndef _NXSYS_USERMESSAGE_H__
#define _NXSYS_USERMESSAGE_H__

void usermsg     (const char * ctlstr, ...);
void usermsgstop (const char * ctlstr, ...);
int usermsggen  (unsigned int flags, const char * ctlstr, ...);

#endif


