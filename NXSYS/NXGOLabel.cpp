//
//  NXGOLabel.cpp
//  NXSYSMac
//
//  Created by Bernard Greenberg on 9/4/19
//  Torn out of nxgo.cpp on that date.
//  Copyright © 2019 BernardGreenberg. All rights reserved.
//

#include "windows.h"
#include <string.h>
#include <math.h>
#include <vector>
#include "nxgo.h"

extern HWND G_mainwindow;
extern HFONT Fnt;

NXGOLabel::NXGOLabel
(GraphicObject * parg, WP_cord x, WP_cord y, const char * lab) {
    wp_x = x;
    wp_y = y;
    
    Parent = parg;
    SetText(lab);
    
}

void NXGOLabel::SetText (const char * lab) {
    size_t len = strlen(lab);
    assert(len < sizeof(s)/sizeof(s[0]));
    strcpy (s, lab);
    RECT r;

    HDC dc = GetDC (G_mainwindow);
    SelectObject (dc, Fnt);
    DrawText (dc, s, len, &r,
              DT_TOP | DT_LEFT |DT_SINGLELINE | DT_NOCLIP | DT_CALCRECT);
    ReleaseDC (G_mainwindow, dc); /* args were reversed until 9 August 1999! */
    
    /* needs scaling , or maybe not */
    
    wp_limits.left = 0;
    wp_limits.right = r.right - r.left;
    wp_limits.top = 0;
    wp_limits.bottom = r.bottom - r.top;
}



void NXGOLabel::Display (HDC hDC) {
    /* at < .8 resolution, don't draw */
    if (NXGO_GetDisplayScale() >= 0.8)
        DrawText (hDC, s, strlen(s), &sc_limits, DT_TOP | DT_LEFT |DT_SINGLELINE | DT_NOCLIP);
}


bool NXGOLabel::MouseSensitive() {
    return false;
}


void NXGOLabel::Diddle (int x, int y) {
    wp_x += x;
    wp_y += y;
}

void NXGOLabel::PositionCenter (WP_cord x, WP_cord y) {
    wp_x = x - wp_limits.right/2;
    wp_y = y - wp_limits.bottom/2;
}

float NXGOLabel::Radius () {
    return (float)
    (sqrt (wp_limits.right*wp_limits.right
           +wp_limits.bottom*wp_limits.bottom)/2.0);
}
