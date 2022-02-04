void DrawFile (const char *);
void RenderRelays (HDC dc);
void CleanUpDrawing();
void DrawingSetPageSize (int width, int height, int canonm, int flags);
int RelayGraphicsMouse(WPARAM, WORD, WORD);
int DrawRelayFromName (const char *);
void RenderRelayPage (HDC dc);
void RecordIndexDrawnRelays(int pageno);
int PlaceRelayDrawing ();
void ClearRelayGraphics();
int DrawRelayFromRelay(class  Relay* r);
int RelayGraphicsLeftCLick(int x, int y);



const int LDRAW_NO_STATEREPORT = 1;

void CleanUpPrinting();
int StartPrintRelays(const char * title, int indexing);
int FinishPrintRelays();
int PageFrame();
int MoreDrawing();
