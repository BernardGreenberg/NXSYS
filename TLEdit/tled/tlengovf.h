#include "TLDlgProc.h"

class ObjectWriter {
public:
    virtual void putf(const char * control_string, ...) = 0;
    virtual void puts(const char *) = 0;
    virtual void putc(char) = 0;
};

/* Included in nxgo.h to define virtual functions for objects */

virtual int     Dump(ObjectWriter& w);
virtual void	Cut();			/* tledit cut */
virtual BOOL    ClickToSelectP();         /* standard meaning clicks? */
virtual void    EditClick (int x, int y); /* accpted left click */
virtual void    ShiftLayout (int delta_x, int delta_y);
virtual void    ShiftLayout2();		/* used only for track/joints */
virtual UINT    DlgId();		/* resource ID */
virtual bool    HasManagedID();
virtual int     ManagedID();
virtual bool    CheckGONumberReuse(HWND hDlg, long nomenclature);
virtual BOOL_DLG_PROC_QUAL DlgProc			/* properties edit dialog proc */
                   (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void            EditProperties();
