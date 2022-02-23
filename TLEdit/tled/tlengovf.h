#include "TLDlgProc.h"



/* Included in nxgo.h to define virtual functions for objects */

virtual int     Dump(FILE * f); /* val is dump order if f is NULL */    
virtual void	Cut();			/* tledit cut */
virtual BOOL    ClickToSelectP();         /* standard meaning clicks? */
virtual void    EditClick (int x, int y); /* accpted left click */
virtual void    ShiftLayout (int delta_x, int delta_y);
virtual void    ShiftLayout2();		/* used only for track/joints */
virtual UINT    DlgId();		/* resource ID */
virtual BOOL_DLG_PROC_QUAL DlgProc			/* properties edit dialog proc */
                   (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void            EditProperties();
