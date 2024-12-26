#include "windows.h"
#include <stdio.h>
#include <string.h>
class Relay;
#include "relays.h"
#include "ldraw.h"
#include <memory.h>
#include "RLYINDEX.H"
#include "compat32.h"
#include "PolyKludge.h"
#include "MessageBox.h"
#include <string>
#include <vector>

#include "ldgDisassemble.hpp"

extern LNode ONE;
extern LNode ZERO;
extern HWND G_mainwindow;



typedef int Scord;

int LastDrawingX;
int LastDrawingY;
int LastDrawingRise;

#define mIn(a,b) (((a) < (b)) ? (a) : (b))
#define mAx(a,b) (((a) > (b)) ? (a) : (b))

const char SSNAME[] = "NXSYS Relay Draftsperson";

enum cell_types {CT_NULL, CT_FRONT, CT_BACK, CT_COIL, CT_CX,
		 CT_BX, CT_AND, CT_OR, CT_CONST, CT_COILTIMER, CT_LAST};
#ifdef NXSYSMac
static Scord CellH = 32, CellW = 80;
#else
static Scord CellH = 20, CellW = 50;
#endif

// This is krazy; figure out from window or something .  . .
#ifdef NXSYSMac
int ContactsPerLine = 15;
#else
int ContactsPerLine = 10;
#endif
static int NCellH = 50, NCellW = 10;
static int DNCellH = NCellH;
static int LDrawFlags = 0;

class GNode;

class Drawing {
public:
    Scord x, y;
    int  xcord, ycord;
    GNode* root;
    short miny, maxy, maxx;
    Relay * relay;
    int    yorg;
    void   Mark (short xg, short yg);
    void   PlaceOnPage();
};

std::vector<Drawing*> Drawings;

static char * Darray = NULL;
static char * Parray = NULL;

const GNode* LocateRelayFromXY (int x, int y);

void DrawingSetPageSize (int width, int height, int canonm, int flags) {
    LDrawFlags = flags;
    int new_cellw = canonm/(ContactsPerLine + 1);
    int new_cellh = (new_cellw*2)/5;
    int new_ncellw = width/new_cellw - 1;
    int new_ncellh = height/new_cellh - 2;
    if (Parray != NULL) {
	char * new_Parray = new char [new_ncellw * new_ncellh];
	memset (new_Parray, 0, new_ncellw*new_ncellh);
	int jlim = mIn (NCellH, new_ncellh);
	int klim = mIn (NCellW, new_ncellw);
	for (int j = 0; j < jlim; j++)
	    for (int k = 0; k < klim; k++)
		new_Parray[j*new_ncellw + k] = Parray[j*NCellW + k];
	delete Parray;
	Parray = new_Parray;
    }
    delete Darray;
    Darray = NULL;
    CellW = new_cellw;
    CellH = new_cellh;
    NCellW = new_ncellw;
    NCellH = new_ncellh;
    DNCellH = 2*NCellH;
    for (auto d : Drawings)
	d->PlaceOnPage();
    InitLdgDisassembly();
}


class GNode {
public:
    class GNode *Parent, *Child, *LastChild, *Next, *Prev;
    short gridx, gridy;
    short width, height;
    short cnx, cny, cnwl;
    int xoff;
    char type, stick, shift;
    char * Statep;
    std::string text;

    GNode ();
    ~GNode();
    void Thread (GNode * parent);
    void Layout (int xcell, int ycell, Drawing &);
    void Flatten();
    void Draw (HDC dc, Drawing& D);
    void CTriangle (HDC dc, Scord x, Scord y);
    void CDot (HDC dc, Scord x, Scord y);
    void DrawCoil(HDC dc, Scord x, Scord y, int tmrp);
    void DrawContactLabel(HDC dc, Scord x, Scord y);
    void DrawBaseLine(HDC dc, Scord x, Scord y, Drawing & D);
    void DrawStickContact(HDC dc, Scord x, Scord y, Drawing& D);
    int Dotoc (short gx, short gy);
    void BXHookCell (HDC dc, Scord scx, Scord scy);
    BOOL Lastp ();
    bool IsThisMeXY(int x, int y, const Drawing& d) const;
    const GNode* LocateRelayRecurse(int x, int y, const Drawing& drawing) const;
};

BOOL GNode::Lastp () {
    if (Parent == NULL)
	return TRUE;
    if (!Parent->Lastp())
	return FALSE;

    if (Parent->type == CT_OR)
	return TRUE;

    return Next == NULL;
}

GNode::GNode () {
    Parent = NULL;
    Child = NULL;
    LastChild = NULL;
    Next = Prev = NULL;
    width = height = 0;
    Statep = NULL;
    text = "???";
    type = CT_NULL;
    xoff = 0;
    stick = shift = 0;
}
 
void GNode::Thread (GNode * parent) {
    Parent = parent;
    if (parent->Child == NULL)
	parent->Child = this;
    Prev = parent->LastChild;
    if (parent->LastChild != NULL)
	parent->LastChild->Next = this;
    parent->LastChild = this;
}


GNode * CreateGNode (LNode * ln, GNode * parent, Drawing& d) {
    int f = ln->Flags;

    if (f & LF_Shref && !(f & LF_Terminal)) /* see loadobj. */
	return CreateGNode (((LCommShr *) ln)->opd, parent, d);


    if (f & LF_Not) {
	GNode * n = CreateGNode (((LNot *) ln)->opd, parent, d);
        n->width = n->height = 1;
	n->type = CT_BACK;
	return n;
    }

    if (f & LF_const) {
	GNode * n = new GNode;
	n->Statep = &(ln->State);
	n->type = CT_CONST;
	n->text = *n->Statep ? "TRUE" : "FALSE";
	n->Thread(parent);
	n->width = n->height = 1;
	return n;
    }

    if (f & LF_Terminal ) {
	GNode * n = new GNode;
	n->type = CT_FRONT;
	n->Statep = &(ln->State);
        n->text = ((Relay *) ln)->RelaySym.u.r->PRep();
	n->Thread(parent);
	n->width = n->height = 1;
	n->stick = ((Relay *) ln) == d.relay;
	return n;
    }

    GNode * n = new GNode;
    n->Thread(parent);
    
    Logop * lo = (Logop *) ln;
    n->type = (lo->op == LogOp::AND) ? CT_AND : CT_OR;
    for (int i = 0; i < lo->N; i++) {
	GNode * sub = CreateGNode (lo->Opds[i], n, d);
	switch (lo->op) {
	    case LogOp::AND:
		n->width += sub->width;
		n->height = mAx (sub->height, n->height);
		break;
	    case LogOp::OR:
		n->height += sub->height;
		n->width = mAx (sub->width, n->width);
		break;
         case LogOp::ZT:
         case LogOp::NOT:
             break; // +++s/b error
	}
    }
    n->Flatten();
    return n;
}

void GNode::Flatten () {
    GNode* next = NULL;
    for (GNode* sn = Child; sn != NULL; sn = next) {
	next = sn->Next;
	if (sn->type == type) {
	    /* hack with width/height ? */
	    for (GNode* ssn = sn->Child; ssn != NULL; ssn = ssn->Next)
		ssn->Parent = this;
            assert(sn->Child);  /*placate analyzer*/
	    sn->Child->Prev = sn->Prev;
	    sn->LastChild->Next = sn->Next;

	    if (sn == LastChild)
		LastChild = sn->LastChild;
	    if (sn == Child)
		Child = sn->Child;

	    if (sn->Prev != NULL)
		sn->Prev->Next = sn->Child;
	    if (sn->Next != NULL)
		sn->Next->Prev = sn->LastChild;

	    sn->Child = NULL;		/* don't subdelete */
	    delete sn;
	}
    }
    /* Assume it's going to fit once squozen. */
    width = mIn (width, NCellW);
}

static GNode * G = NULL;

static Drawing* DD = NULL;


void DrawCircuit (LNode * ln, LNode* exp, int tmr){

    if (Darray == NULL)
	Darray = new char [DNCellH*NCellW];
    GNode* pRoot = new GNode;
    GNode& root = *pRoot;
    root.type = CT_AND;
    GNode* pRelay = new GNode;
    GNode& relay = *pRelay;
    Relay * r = (Relay *) ln;
    relay.text = r->RelaySym.u.r->PRep().c_str();

    relay.type = tmr ? CT_COILTIMER : CT_COIL;
    relay.width = relay.height = 1;
    relay.Thread (&root);
    relay.Statep = &(r->State);
    Drawing * D = new Drawing;
    D->yorg = DNCellH/2;
    D->relay = r;
    GNode * gn = CreateGNode (exp, pRoot, *D);
    root.width = 1 + gn->width;
    root.height = gn->height;
    root.Flatten();

    D->root = &root;
    D->miny = 0;
    D->maxx = 0;
    D->maxy = 0;
    memset (Darray, 0, DNCellH*NCellW);
    root.Layout (0, 0, *D);
    G = pRoot;
    DD = D;
}

void GNode::Layout (int xc, int yc, Drawing& D) {
    gridx = xc;
    gridy = yc;
    cnx = cny = 0;
    switch (type) {
	case CT_AND:
	{
	    for (GNode *sg = Child; sg != NULL; sg = sg->Next) {
		if (xc + sg->width > NCellW) {
		    int wdl = 0;
		    int wdh = 0;
		    for (GNode* gg = sg; gg != NULL; gg = gg->Next) {
			wdl += gg->width;
			wdh = mAx (wdh, gg->height);
		    }
		    short nxc = NCellW - wdl;
		    nxc = mAx (nxc, 0);
		    short nwl = D.maxy + 1;
		    short nyc = nwl + wdh; /* hahah */
		    sg->shift = 1;
		    sg->Layout (nxc, nyc, D);
		    for (int j = nxc; j < xc; j++)
			D.Mark (j, nwl);
		    sg->cnx = xc;
		    sg->cny = yc;
		    sg->cnwl = nwl;
		    xc = nxc;
		    yc = nyc;
		    for (int j = nwl; j <= nyc; j++)
			D.Mark (xc, j);
		}
		else
		    sg->Layout (xc, yc, D);
		xc += sg->width;
	    }
	    break;
	}
	case CT_OR:
	{
	    for (GNode *sg = Child; sg != NULL; sg = sg->Next) {
		sg->Layout (xc, yc, D);
		yc -= sg->height;
	    }
	    for (int i = 0; i < width; i++)
		for (int j = 0; j < height; j++)
		    D.Mark (xc+i, yc+j);
	    break;
	}

	case CT_COIL:
	case CT_COILTIMER:
	    D.Mark (xc, yc);
	    height = 1;
	    if (!(LDrawFlags & LDRAW_NO_STATEREPORT)) {
		D.Mark (xc, yc-1);
		height++;
	    }
	    break;
	default:
	    if (stick) {
		if (D.maxy > 0 || shift)
		    goto nostick;
		else {
		    if (Parent->type == CT_OR/* && Next != NULL*/ && gridy != 0)
			goto nostick;
		    if (Parent->type == CT_AND && Parent->Parent != NULL)
			goto nostick;
		    for (int j = 0; j <= Parent->gridx+Parent->width-1; j++)
			D.Mark (j, +1);
		    height = 0;
		}
	    }
	    else {
nostick:        stick = 0;
		if (type == CT_CONST && !*Statep)
		    break;
		D.Mark (xc, yc);
		if (Lastp());
		else if (Parent->type == CT_OR && Parent->width > 1){
		    short lx = Parent->gridx + Parent->width;
		    for (int i = Parent->gridx; i < lx; i++)
			D.Mark (i, gridy);
		}
	    }
	    break;
    }
}

void GNode::CTriangle (HDC dc, Scord x, Scord y) {
	POINT point[3]{};
    Scord trx = x;
    Scord trb =  (Scord)(.1*CellW);
    Scord trh = (Scord)(.866*trb);
    point[0].x = trx -trb/2;
    point[1].x = trx;
    point[2].x = point[0].x + trb;
    Scord y0 = y;
    SelectObject(dc, GetStockObject(BLACK_PEN));
    if (type == CT_FRONT) {
	SelectObject (dc, GetStockObject (*Statep? BLACK_BRUSH : WHITE_BRUSH));
	y0 -= (Scord)(CellW*.025);
	point[0].y = y0 - trh;
	point[1].y = y0;
	point[2].y = y0 - trh;
	Polygon (dc, point, 3);
    }
    else if (type == CT_BACK) {
	SelectObject (dc, GetStockObject (*Statep? WHITE_BRUSH : BLACK_BRUSH));
	y0 += (Scord)(CellW*.025);
	point[0].y = y0+trh;
	point[1].y = y0;
	point[2].y = y0+trh;
	Polygon (dc, point, 3);
    }
}


void GNode::CDot (HDC dc, Scord x, Scord y) {
    SelectObject (dc, GetStockObject (BLACK_BRUSH));
    Scord ix0 = x;
    Scord iy0 = y;
    Scord rad = (Scord)(CellW*.04);
    Ellipse (dc, ix0 - rad, iy0 - rad, ix0 + rad,iy0 + rad);
}



void GNode::DrawCoil (HDC dc, Scord scx, Scord scy, int timer_p) {
	RECT txr{};
	POINT point[3]{};
    short statereport = !(LDrawFlags & LDRAW_NO_STATEREPORT);
    if (statereport) {
	const char * s = *Statep ? "PICKED" : "DROPPED";
	txr.left = txr.right =  txr.top = txr.bottom = 0;
	DrawText (dc, "DROPPED", (int)strlen("DROPPED"), &txr,
		  DT_TOP | DT_LEFT |DT_SINGLELINE| DT_NOCLIP|DT_CALCRECT);

	txr.left = scx;
	txr.right = scx + CellW;
	txr.top = (int)(scy - CellH*.95);
	txr.bottom += txr.top;
	FillRect (dc, &txr, (HBRUSH)GetStockObject(WHITE_BRUSH));
	DrawText (dc, s, (int)strlen(s), &txr,
		  DT_TOP | DT_CENTER |DT_SINGLELINE|DT_NOCLIP);
    }
    txr.left = txr.right =  txr.top = txr.bottom = 0;
    DrawText (dc, text.c_str(), (int)text.size(), &txr,
	      DT_TOP | DT_LEFT |DT_SINGLELINE| DT_NOCLIP|DT_CALCRECT);
    txr.left = scx;
    txr.right = scx + CellW;
    if (statereport) {
	txr.bottom = (int) (txr.bottom*1.2);
	txr.top = (int)(scy - CellH*.95 - txr.bottom);
	txr.bottom += txr.top;
    }
    else {
	txr.top = (int)(scy - CellH*.95);
	txr.bottom += txr.top;
    }
    DrawText (dc, text.c_str(), (int)text.size(), &txr,
	      DT_TOP | DT_CENTER |DT_SINGLELINE| DT_NOCLIP);

	RECT box{};
    box.top = scy-CellH/2;
    box.bottom = scy;
    box.left = (Scord)(scx + .35*CellW);
    box.right = (Scord)(scx+.65*CellW);

    MoveTo (dc, box.left, box.bottom);
    LineTo (dc, box.left, box.top);
    LineTo (dc, box.right, box.top);
    LineTo (dc, box.right, box.bottom);
    SelectObject (dc, (HBRUSH)GetStockObject (BLACK_BRUSH));
    if (timer_p) {
	box.bottom=scy-CellH/4;
	FillRect (dc, &box, (HBRUSH)GetStockObject(BLACK_BRUSH));
    }

    /* Draw "CX" arrow */
    point[0].x = point[2].x = scx + CellW/8;
    point[0].y = scy - CellH/10;
    point[2].y = scy + CellH/10;
    point[1].x = scx;
    point[1].y = scy;
    Polygon (dc, point, 3);
}


void GNode::DrawStickContact (HDC dc, Scord scx, Scord scy, Drawing& D) {
    Scord rscx = 0*CellW + D.x;
    Scord rscy = (Scord)(+.5*CellH+ D.y);
    MoveTo (dc, rscx, rscy);
    LineTo (dc, scx, rscy);
    LineTo (dc, scx, D.y);
    MoveTo (dc, rscx, rscy);
    LineTo (dc, rscx, 1*CellH + D.y);
    if (Parent != NULL && Parent->type == CT_OR)
	CDot (dc, scx, D.y);
    if (Lastp())
	BXHookCell (dc, rscx, 1*CellH+D.y);
    else {
	short fgp = gridx+1;
	if (Parent != NULL && Parent->type == CT_OR) {
	    fgp = Parent->gridx + Parent->width;
	    CDot (dc, fgp*CellW+D.x, D.y);
	}
	Scord x3 = fgp*CellW + D.x;
	LineTo (dc, x3, 1*CellH + D.y);
	LineTo (dc, x3, 0+ D.y);
    }
    CDot (dc, (int)(rscx + .35*CellW), rscy);
    CTriangle (dc, (int)(rscx + .65*CellW), rscy);
}



void GNode::BXHookCell (HDC dc, Scord scx, Scord scy) {
    Scord sex = (Scord)(scx + CellW*.80);
    LineTo (dc, sex, scy);
    Scord yh = (Scord)(CellW*.1);
    MoveTo (dc, sex + yh,  scy - yh);
    LineTo (dc, sex, scy);
    MoveTo (dc, sex + yh,  scy + yh); /* asymmetry in drawing */
    LineTo (dc, sex, scy);
}

void GNode::DrawBaseLine (HDC dc, Scord scx, Scord scy, Drawing & D) {
    MoveTo (dc, scx, scy);
    Scord sex = scx + CellW;
     if (Lastp())
	BXHookCell (dc, scx, scy);
     else {
	 LineTo (dc, sex, scy);
	 /* SHOULD BE one guy on line  -- maybe spread 'em out? */
	 if (Parent->type == CT_OR && Parent->width > 1){
	     Scord was_scx = scx;
	     scx = (Parent->gridx)*CellW + (Parent->width*CellW)/2
		   -CellW/2 + D.x;
	     xoff = scx-was_scx;
	     MoveTo (dc, Parent->gridx*CellW + D.x, scy);
	     LineTo (dc, (Parent->gridx+Parent->width)*CellW + D.x, scy);
	 }
	 /* more than one but not enough cells on line */
	 else if (Parent->type == CT_OR
		  && gridx + width < Parent->gridx + Parent->width) {
	     MoveTo(dc, scx+CellW, scy);
	     LineTo (dc, scx+CellW*(Parent->gridx + Parent->width + 1
				    - (gridx + width) + D.x), scy);
	 }
    }	    
}

void GNode::DrawContactLabel (HDC dc, Scord scx, Scord scy) {
    
    int flgs = DT_TOP |DT_SINGLELINE|DT_NOCLIP; 
	RECT txr{};
    txr.top = (int)(scy - CellH*.55);
    txr.bottom = scy;
    if (type == CT_FRONT) {
	txr.left = (int)(scx+ CellW*.3);
	txr.right = scx + CellW;
	flgs |= DT_LEFT;
    }
    else {
	txr.left = scx;
	txr.right = (int)(scx + CellW*(.65+.2));
	flgs |= DT_CENTER;
    }
    DrawText (dc, text.c_str(), (int)text.size(), &txr, flgs);
}

void GNode::Draw (HDC dc, Drawing& D) {
    Scord scx =  CellW*gridx + D.x;
    Scord scy = CellH*gridy + D.y;
    int lastp = Lastp();
    if (shift) {
	MoveTo (dc, cnx*CellW + D.x, cny*CellH+D.y);
	LineTo (dc, cnx*CellW + D.x, cnwl*CellH+D.y);
	LineTo (dc, scx, cnwl*CellH+D.y);
	LineTo (dc, scx, scy);
    }

    switch (type) {
	case CT_CONST:
	    if (*Statep)
		DrawBaseLine (dc, scx, scy, D);
	    break;

	case CT_FRONT:
	case CT_BACK:
	    if (stick) {
		DrawStickContact (dc, scx, scy, D);
		break;
	    }
	    DrawBaseLine (dc, scx, scy, D);
	    scx += xoff;
	    DrawContactLabel (dc, scx, scy);

	    CTriangle (dc, (int)(scx+ .2*CellW), scy);
	    CDot (dc, (int)(scx+.65*CellW), scy);
	    break;

	case CT_COIL:
	case CT_COILTIMER:
	    DrawCoil (dc, scx, scy, type == CT_COILTIMER);
	    DrawBaseLine (dc, scx, scy, D);
	    break;

	case CT_AND:
	{
	    for (GNode *sg = Child; sg != NULL; sg = sg->Next) {
		sg->Draw(dc, D);
		if (Parent && sg->Next == NULL && Parent->type == CT_OR &&
		    Parent->width > 1 && !sg->Lastp()) {
		    int parend = CellW*(Parent->gridx + Parent->width)
				 + D.x;
		    int lastend = CellW*sg->gridx + D.x;
		    if (lastend < parend) {
			MoveTo (dc, lastend, scy);
			LineTo (dc, parend, scy);
		    }
		}
	    }
	    break;
	}
	case CT_OR:
	{
	    int miny = gridy;
	    for (GNode *sg = Child; sg != NULL; sg = sg->Next) {
		sg->Draw(dc, D);
		short gy = sg->gridy;
		if (!sg->stick) {
		    miny = mIn (gy, miny);
		    Scord y = D.y + CellH*gy;
		    if (shift) {
			if (sg->Prev != NULL)
			    CDot (dc, scx, y);
			if (!lastp)
			    if (sg->Next != NULL)
				CDot (dc, scx + width*CellW, y);
		    }
		    else if (sg->Next != NULL && !sg->Next->stick) {
			CDot (dc, scx, y);
			if (!lastp)
			    CDot (dc, scx + width*CellW, y);
		    }
		}
	    }
	    {for (int j = gridy-1; j >= miny; j--) {
		Scord y = j * CellH + D.y;
		short left = 0, right = 0;
		if (j > miny) {
		    if (Dotoc (gridx, j))
			left = 1;
		    if (Dotoc (gridx+width, j))
			right = 1;
		}
		if (Prev != NULL && Prev->Dotoc (gridx, j))
		    left = 1;
		if (Next != NULL && Next->Dotoc (gridx+width, j))
		    right = 1;
		if (left)
		    CDot (dc, scx, y);
		if (right)
		    CDot (dc, scx + width*CellW, y);
	    }}

	    MoveTo (dc, scx, scy);
	    LineTo (dc, scx, CellH*miny+ D.y);
	    if (!lastp && miny < gridy) {
		Scord x0 = scx + CellW*width;
		MoveTo (dc, x0, scy);
		LineTo (dc, x0, CellH*miny + D.y);
		    CDot (dc, x0, scy);
		break;
	    }
	}
    }
}


int GNode::Dotoc (short gx, short gy) {
    switch (type) {
	case CT_CONST:
	    if (!*Statep)
		return 0;
	case CT_FRONT:
	case CT_BACK:
	case CT_COIL:
	case CT_COILTIMER:
	    if (shift || stick)
		return 0;
	    if (gy != gridy)
		return 0;
	    if (gx == gridx)
		return 1;
	    if (Lastp())
		return 0;
	    if (Parent->type == CT_OR && Parent->width > 1) {
		if (Parent->gridx + Parent->width == gx)
		    return 1;
	    }
	    else if (gridx + width == gx)
		return 1;
	    return 0;
	case CT_AND:
	case CT_OR:
	    {for (GNode * gn = Child; gn != NULL; gn = gn->Next)
		if (gn->Dotoc (gx, gy))
		    return 1;}
	    return 0;
	default: return 0;
    }

}


/* Where would you like to place Cranshaw the Relay Drawing? */

int PlaceRelayDrawing () {
    if (DD== NULL)
	FatalAppExit (0, "DD Null in PlaceRelayDrawing");
    if (Parray == NULL) {
	Parray = new char [NCellH*NCellW];
	memset (Parray, 0, NCellH*NCellW);
    }
    Drawing * d = DD;
    int ye = -(d->miny-1) + d->maxy;
    int xe = d->maxx+1;
    int x = 0;
    int y = 0;
    //int h = (int)mIn (NCellH, DNCellH);
    for (y = 0; y + ye < NCellH; y++) {
	for (x = 0; x + xe <= NCellW; x++) {
	    for (int y2 = y; y2 < y + ye; y2++) {
		int vx = y2*NCellW;
		if (x > 0 && Parray[(x-1)+vx])
		    goto next_possible_xpos; /* leave one space */
		if (x + xe < NCellW)
		    if (Parray[x+xe+vx])
			goto next_possible_xpos;
		for (int x2 = x; x2 < x + xe; x2++)
		    if (Parray[x2+vx])
			goto next_possible_xpos;
	    }
	    goto got_it;
next_possible_xpos:;
	}
    }
    return 0;

got_it:
    for (int yy = 0; yy < ye; yy++) {
	int vx = (yy+d->yorg+d->miny)*NCellW;
	int vx2 = NCellW*(y+yy);
	for (int xx = 0; xx < xe; xx++)
	    Parray[x+xx + vx2] |= Darray[xx+vx];
    }
    d->xcord = x;
    d->ycord = y - (DD->miny-1);
    d->PlaceOnPage();
    Drawings.push_back(d);
    DD = NULL;
    LastDrawingX = d->x;
    LastDrawingY = d->y;
    LastDrawingRise = d->miny * CellH;
    return 1;

}

    
void Drawing::PlaceOnPage () {
    
    x = CellW/2 + xcord * CellW;
    y = ycord*CellH;
}

void RenderRelayPage (HDC dc) {
    if (haveDisassembly)
        ldgDisassembleDraw(dc);
    for (auto d : Drawings)
	d->root->Draw(dc, *d);
}

void RecordIndexDrawnRelays (int indexpage) {
    for (auto d : Drawings)
	RecordRelayIndex (d->relay->RelaySym.u.r, indexpage);
}

void RenderRelays (HDC dc) {
    if (G == NULL)
	return;
    DD->x = CellW/2;
    DD->y = -(DD->miny-1)*CellH;
    G->Draw(dc, *DD);
}

GNode::~GNode () {
    GNode* next = Child;		/* that ol' bug... */
    for (GNode*sn = Child; next != NULL; sn = next) {
	next = sn->Next;
	delete sn;
    }
}


void CleanUpDrawing () {
    ClearRelayGraphics();
    delete DD;
    delete G;
    G = NULL;
    DD = NULL;
    delete Parray;
    delete Darray;
    Parray = Darray = NULL;
    ClearRelayIndex();
    InitLdgDisassembly();
}


int DrawRelayFromName (const char * rnm) {
    Sexpr s = RlysymFromStringNocreate (rnm);
    if (s == NIL){
	MessageBoxS (0, std::string("No such relay: ") + rnm, SSNAME, MB_OK|MB_ICONEXCLAMATION);
	return 0;
    }
    Relay * r = s.u.r->rly;
    if (r->exp == NULL) {
	MessageBox (0, "Simulator-provided relay, no autonomous code.", SSNAME,
		    MB_OK|MB_ICONEXCLAMATION);
	return 0;
    }
    return DrawRelayFromRelay(r);
}

int DrawRelayFromRelay(Relay* r) {
    Sexpr s = r->RelaySym;
    int tmr = 0;
    if (r->Flags & LF_Timer) {
        Sexpr controlse = ZAppendRlysym(s);
        Relay * control = controlse.u.r->rly;
        if (control->exp == nullptr) {
            MessageBox (0, "Simulator-provided relay, no autonomous code.", SSNAME,
                        MB_OK|MB_ICONEXCLAMATION);
            return 0;
        }
        r = control;
        tmr = 1;
    }
    if (r->Flags & LF_CCExp) {
        ldgDisassemble(r);
#if 0
        MessageBox (0, "Compiled code (subr) relay.  Load interlocking "
                    "from expr-code (.trk) and try again.", SSNAME,
                    MB_OK|MB_ICONEXCLAMATION);
        
        return 0;
#endif
    }
    else
        DrawCircuit (r, r->exp, tmr);
    return 1;
}

// Externally available...
const char* RelayGraphicsNameFromXY(int x, int y) {
    if (auto g = LocateRelayFromXY(x, y))
        return g->text.c_str();
    return nullptr;
}

int RelayGraphicsMouse(WPARAM wParam, WORD x, WORD y) {
    if (const char* name_cstr = RelayGraphicsNameFromXY(x, y))
        if (DrawRelayFromName(name_cstr))
            return 1;
    return 0;
}

bool GNode::IsThisMeXY(int x, int y, const Drawing&d) const {
    Scord scx = CellW*gridx + d.x + xoff;
    Scord scy = CellH*gridy + d.y;
    return y > scy - CellH/2 && y < scy + CellH/2 && x >= scx && x <= scx + CellW;
}

const GNode* GNode::LocateRelayRecurse (int x, int y, const Drawing& drawing) const {
    //printf("%p %d %d %s\n", this, x, y, text.c_str());
    if (Child == nullptr && IsThisMeXY(x, y, drawing)) //compound nodes have extent
        return this;

    for (auto sn = Child; sn != NULL; sn = sn->Next) {
        if (auto g = sn->LocateRelayRecurse(x, y, drawing))
            return g;
    }
    return nullptr;
}

const GNode* LocateRelayFromXY (int x, int y) {
    for (auto d : Drawings)
        if (auto g = d->root->LocateRelayRecurse(x, y, *d))
            return g;
    return nullptr;
}

void DBB() {
    DebugBreak();
    FatalAppExit (0, "Relay Graphics map index out of bounds.");
}

void Drawing::Mark (short xg, short yg) {
    if (yg + yorg < 0) {
	DBB();
    }
    maxy = mAx (yg, maxy);
    miny = mIn (yg, miny);
    maxx = mAx (xg,maxx);

    int darray_index = xg+ (yg+yorg)*NCellW;
    /* Simply don't mark it if out of bounds */
    if (darray_index >= 0 && darray_index < NCellW*DNCellH)
	Darray[darray_index] = 1;
}

void ClearRelayGraphics() {
    for (auto d : Drawings) {
	if (G != d->root && d != DD) {
	    delete d->root;
	    delete d;
	}
    }
    Drawings.clear();
    if (haveDisassembly)
        InitLdgDisassembly();
    delete Parray;
    Parray = NULL;
}
