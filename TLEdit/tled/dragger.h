#ifndef _NXSYS_TRACK_EDITOR_DRAGGER_H__
#define _NXSYS_TRACK_EDITOR_DRAGGER_H__

#include <string>

class Dragger {
    enum class MoveStates {
        NOT = 0,
        FROM_MOUSEUP = 1,
        FROM_MOUSEDOWN = 2
    };
    
public:
    std::string Description;
    MoveStates MovingState;
    int Xoff, Yoff;
    GraphicObject * Object;
    
    BOOL Movingp () {return MovingState != MoveStates::NOT;}
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
