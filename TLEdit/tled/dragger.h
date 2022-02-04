#ifndef _NXSYS_TRACK_EDITOR_DRAGGER_H__
#define _NXSYS_TRACK_EDITOR_DRAGGER_H__

enum DraggerMoveStates {
    MOVSTATE_NOT = 0, MOVSTATE_FROM_MOUSEUP, MOVSTATE_FROM_MOUSEDOWN};

class Dragger {
    public:
	char Descrip[32];
	enum DraggerMoveStates MovingState;
	int Xoff, Yoff;
	GraphicObject * Object;

	BOOL Movingp () {return MovingState != MOVSTATE_NOT;}
	GraphicObject* StartMoving (GraphicObject * g,
				    const char * description,
				    HWND hWnd);

	void TruncMouse (HWND hWnd, int x, int y, WP_cord & wpx, WP_cord & wpy);
	void Rodentate (HWND hWnd, int x, int y, UINT message, WPARAM wParam);
	void ClickOn (HWND hWnd, GraphicObject * g,
			       const char * description,
			       int x, int y);
	void Abort();

	virtual void DeleteObj();
	Dragger();
};

extern Dragger *RodentatingDragon, *MouseupDragon;

#endif
