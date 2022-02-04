#ifndef _BSG_RUBBER_BAND_H__
#define _BSG_RUBBER_BAND_H__

class RubberBand {
  HWND hWnd;
  HDC  hDC;
  BOOL Drawn;
  int EndX, EndY;
  HPEN LastPen;
  public:
    int StartX;
    int StartY;
    int J;  // Macum usit
    RubberBand(HWND h_Wnd, int x, int y, int j);
    void Draw(int x, int y);
    void DrawHighlighted(int x, int y);
    void Undraw();
    ~RubberBand();
};

void RubberBandInit();
void RubberBandClose();


#endif
