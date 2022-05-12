#ifndef _NX_GRAPHOBJ_H__
#define _NX_GRAPHOBJ_H__

#include "windows.h"
#include "typeid.h"

#define Virtual

typedef long RW_cord;			/* real-world coord */
typedef long WP_cord;			/* whole-panel cord */
typedef int SC_cord;			/* screen-cord */

#define WM_NXGO_LBUTTONSHIFT (WM_USER+1)
#define WM_NXGO_LBUTTONCONTROL (WM_USER+2)
#define WM_NXGO_RBUTTONCONTROL (WM_USER+3)

struct WPRECT {
    WP_cord top, bottom, left, right;
};


class GraphicObject {
public:
    GraphicObject ();
    RW_cord rw_x, rw_y;
    WP_cord wp_x, wp_y;
    RECT wp_limits;			/* relative to wp_x, wp_y */
    RECT sc_limits;
    SC_cord sc_x, sc_y;
    char Valid, Selected;

public:
    short   Visible;

    virtual BOOL    HitP (long x, long y);
    virtual void    HitXY (WORD x, WORD y, WORD message);
    virtual void    Display(HDC dc) = 0;
    virtual void    ComputeWP();
    virtual BOOL    ComputeVisible (WPRECT& v);
    virtual TypeId     TypeID();  // not pure - defaiult is -1, no.
    virtual bool    IsNomenclature(long);

    virtual void    Hit (int mh);
    virtual void    UnHit();
    virtual bool    MouseSensitive();
    virtual void    Select();
    virtual void    Deselect();
    virtual         ~GraphicObject();
    virtual void    ComputeWPRect();

    int     FindHitGo (SC_cord& x, SC_cord& y, int mb);
    void        Invalidate();


#ifdef TLEDIT
#include "tlengovf.h"
#else
    int		    RunContextMenu(int resource_id);
    virtual void    EditContextMenu(HMENU m);
#endif

    void            WantMouseUp();
    void	    MakeSelfVisible();
    void	    MoveSC(SC_cord x, SC_cord y);
    void	    MoveWP(WP_cord x, WP_cord y);
    void	    Hide();	    
    void	    UnHide();
    void	    ComputeVisibleLast();
    void	    GetVisible();
    void            ContributeToLayoutRect();
    void            Consume(); /* to fool flow analyzer, which doesn't know
                                that the constructor stores a reference */
};

class NXGOLabel : public GraphicObject {
    public:
	char s [16];
	GraphicObject * Parent;

	NXGOLabel (GraphicObject * parg, WP_cord x, WP_cord y, const char * lab);
	void SetText(const char * p_s);
        void Select() override {};   /* disallow selection */
	float Radius();
	void PositionCenter (WP_cord x, WP_cord y);
	void Diddle (int xadj, int yadj);
	virtual void Display (HDC hdc) override;
        virtual bool MouseSensitive () override;
};

void NXGOMouseMove (WORD x, WORD y);
void NXGOMouseUp();

void ComputeVisibleObjects (WPRECT& view);
void ComputeVisibleObjectsLast();
void ComputeWindowPos();
void DisplayVisibleObjects (HDC dc);
void DisplayVisibleObjectsRect (HDC dc, RECT& ur);
void FreeGraphicObjects();
int GraphicObjectCount();
extern GraphicObject * SelectedObject;
GraphicObject * FindHitObject (long nomenclature, TypeId type);
GraphicObject * FindHitObjectOfType (TypeId type, WORD x, WORD y);
GraphicObject * FindHitObjectOfTypes (TypeId *types, int nkeys, WORD x, WORD y);
typedef int (*GOMapperFcn) (GraphicObject*);
typedef int (*GOGOMapperFcn) (GraphicObject*, void*);

int MapGraphicObjectsOfType (TypeId, GOMapperFcn fn);
GraphicObject* MapFindGraphicObjectsOfType (TypeId type, GOGOMapperFcn, void*);
GraphicObject* GetMouseHitObject (WORD x, WORD y);
GraphicObject* MapAllGraphicObjects (GOGOMapperFcn fn, void * arg);
GraphicObject* MapAllVisibleGraphicObjects (GOGOMapperFcn fn, void * arg);

extern WORD NXGOHitX, NXGOHitY;
extern BOOL NXGODeleteAll;

WP_cord SCXtoWP (SC_cord x);
WP_cord SCYtoWP (SC_cord y);
SC_cord WPXtoSC (WP_cord x);
SC_cord WPYtoSC (WP_cord y);

void NXGO_HScroll (HWND window, int code, int /*pos*/);
void NXGO_VScroll (HWND window, int code, int /*pos*/);
void NXGO_SetViewportDims (int width, int height);
void NXGO_SetDisplayScale (double s);
double NXGO_GetDisplayScale();
typedef void (*tNXGOSelectHook)(GraphicObject*);
void NXGO_SetSelectHook (tNXGOSelectHook);
void NXGO_SetDisplayWPOrg (WP_cord x, WP_cord y);
void NXGO_GetDisplayWPOrg (WP_cord &x, WP_cord&y);
void NXGO_SetScrollPosition(HWND hWnd);
void NXGO_Rodentate (WORD x, WORD y, WORD mb);
void NXGO_ValidateWpVp(HWND window);
RECT NXGO_ComputeTrueLayoutDimensions();

extern double NXGO_Scale;

#endif
