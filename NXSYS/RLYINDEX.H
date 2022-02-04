#ifndef _NXSYS_RELAY_INDEX_H__
#define _NXSYS_RELAY_INDEX_H__

#ifndef _NX_LISP_SYS_H__
class Rlysym;
#endif

void RecordRelayIndex (Rlysym*, int indexpage);
void ClearRelayIndex();
void ProduceRelayIndex (HDC dc, int (*moreproc) (void),
			HFONT f1, HFONT f2, int width, int height);

#endif
