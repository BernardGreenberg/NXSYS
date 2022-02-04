#include "windows.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "lisp.h"
#include "ldraw.h"
#include "RLYINDEX.H"
#include "compat32.h"

static HFONT F1, F2;
static int PageWidth, PageHeight;
static int Xused, Yused;


struct RlyRec {
    Rlysym * rs;
    short page;
};

const int RlyblockMaxCount = 50;
static int Count = 0;

struct Rlyblock {
    RlyRec * rr;
    short count;
    short maxcount;
    struct Rlyblock* next;
};


static Rlyblock *Chain = NULL;

void ClearRelayIndex() {
    if (Chain == NULL)
	return;
    Rlyblock * next;
    for (Rlyblock* rb = Chain; rb != NULL; rb = next) {
	next = rb->next;
	if (rb->rr != NULL)
	    delete rb->rr;
	delete rb;
    }
    Count = 0;
    Chain = NULL;
}


void RecordRelayIndex (Rlysym*rs, int indexpage) {
    if (Chain == NULL || Chain->count >= Chain->maxcount) {
	Rlyblock * rb = new Rlyblock;
	if (rb == NULL)
	    return;
	rb->rr = new RlyRec[RlyblockMaxCount];
	if (rb->rr == NULL)
	    return;
	rb->maxcount = RlyblockMaxCount;
	rb->count = 0;
	rb->next = Chain;
	Chain = rb;
    }
    Chain->rr[Chain->count].rs = rs;
    Chain->rr[Chain->count].page = indexpage;
    Chain->count++;
    Count++;
}

typedef int (*Filter)(Rlysym* rs, short page);

int MapRefChain (Filter fcn) {
    int tot = 0;
    for (Rlyblock* rb = Chain; rb != NULL; rb = rb->next)
	for (int i = 0; i < rb->count; i++)
	    tot += fcn (rb->rr[i].rs, rb->rr[i].page);
    return tot;
}

static int RandomFilter (Rlysym * rs, short) {
    return rs->n == 0;
}

static int SignalFilter (Rlysym * rs, short) {
    long n = rs->n;
    return n > 0 && (n & 1) == 0 && n < 1000L;
}


static int SwitchFilter (Rlysym * rs, short page) {
 //   page;
    long n = rs->n;
    return ((n & 1) == 1) && n < 1000L;
}

static int TrackFilter (Rlysym * rs, short page) {
 //   page;
    return rs->n >= 1000;
}

static Filter Philtre;
static long LastNum;
static int NNums;
long *Nums = NULL;
static int NIDTypes;
static short * IDTypes;

static int CollectNums (Rlysym * rs, short p) {
    long n = rs->n;
    if (Philtre (rs, p))
	if (n != LastNum) {
	    LastNum = n;
	    Nums[NNums++] = n;
	    return 1;
	}
    return 0;
}


static int CollectIDStrings (Rlysym * rs, short p) {
    short idt = rs->type;
    if (Philtre (rs, p))
	if (idt != LastNum) {
	    LastNum = idt;
	    IDTypes[NIDTypes++] = idt;
	    return 1;
	}
    return 0;
}


static int sortlongs (const void *a, const void* b) {
    if (*((long*) a) < *((long *) b))
	return -1;
    if (*((long*) b) < *((long *) a))
	return +1;
    return 0;
}

static int sortids (const void *a, const void* b) {
    if (*((short*) a) == *((short *) b))
	return 0;
    return strcmp (redeemRlsymId (*((short*)a)), redeemRlsymId (*((short*)b)));
}



static void PrintRelayType (HDC dc, int X, int Y, int xc,
			    int cellh, int cellw, int NNums, int idt) {
    RECT txr;
    txr.left = X + cellw*xc;
    txr.right = txr.left + cellw;
    txr.top = Y - cellh;
    txr.bottom = txr.top + cellh;
    MoveTo (dc, txr.right, txr.bottom);
    LineTo (dc, txr.right, Y+NNums*cellh);
    const char * s = redeemRlsymId (idt);
    DrawText (dc, s, strlen(s), &txr,
	      DT_VCENTER | DT_CENTER |DT_SINGLELINE| DT_NOCLIP);
}

static void PrintItemNumber (HDC dc, int X, int Y, int yc,
			     int cellh, int cellw, int nids, long inum) {
    char buf [10];
    RECT txr;
    txr.left = X- 1*cellw;
    txr.right = txr.left + cellw;
    txr.top = Y+ cellh * yc;
    txr.bottom = txr.top + cellh;
    MoveTo (dc, X-1*cellw, txr.bottom);
    LineTo (dc, X+nids*cellw, txr.bottom);
    sprintf (buf, "%ld", inum);
    DrawText (dc, buf, strlen(buf), &txr,
	      DT_VCENTER | DT_LEFT |DT_SINGLELINE| DT_NOCLIP);
}

static void PrintPageNum (HDC dc, int X, int Y, int x, int y,
			  int cellh, int cellw, int page) {
    char buf [10];
    RECT txr;
    txr.left = X + x*cellw;
    txr.right = txr.left + cellw;
    txr.top = Y + y*cellh;
    txr.bottom = txr.top + cellh;
    sprintf (buf, "%d", page);
    DrawText (dc, buf, strlen(buf), &txr,
	      DT_VCENTER | DT_CENTER |DT_SINGLELINE| DT_NOCLIP);
}

static void ProduceOneRelayIndex (HDC dc, Filter filter, const char * title,
				  int(*moreproc)(void)) {
    Philtre = filter;
    
    RECT txr;
    txr.left = txr.right =  txr.top = txr.bottom = 0;
    SelectObject (dc, F2);
    DrawText (dc, title, strlen (title), &txr,
	      DT_TOP | DT_LEFT |DT_SINGLELINE| DT_NOCLIP|DT_CALCRECT);

    int title_height = txr.bottom - txr.top;
    int title_width = txr.right - txr.left;

    /* collect device numbers */

    short candidates = MapRefChain (Philtre);
    if (candidates == 0)
	return;
    Nums = new long[candidates];
    if (Nums == NULL)
	FatalAppExit (0, "Alloc of index num array fails.");
    NNums = 0;
    LastNum = -1;
    MapRefChain (CollectNums);
    qsort (Nums, NNums, sizeof(long), sortlongs);
    int j = 0;
    long last = -1;
    for (int i = 0; i < NNums; i++)
	if (Nums[i] != last)
	    Nums[j++] = last = Nums[i];
    NNums = j;

    /* collect relevant idstrings */

    IDTypes = new short[candidates];
    if (IDTypes == NULL)
	FatalAppExit (0, "Alloc of index num array fails.");
    NIDTypes = 0;
    LastNum = -1;
    MapRefChain (CollectIDStrings);
    qsort (IDTypes, NIDTypes, sizeof(short), sortids);
    j = 0;
    short lasti = -1;
    int i;
    for (i = 0; i < NIDTypes; i++)
	if (IDTypes[i] != lasti)
	    IDTypes[j++] = lasti = IDTypes[i];
    NIDTypes = j;

    /* run the array */
    /* we should now know dimensions */
    SelectObject (dc, F1);

    txr.left = txr.right =  txr.top = txr.bottom = 0;
    DrawText (dc, "245", strlen("245"), &txr,
	      DT_TOP | DT_LEFT |DT_SINGLELINE| DT_NOCLIP|DT_CALCRECT);
    int cellw = (txr.right-txr.left)*2;
    int cellh = (int)((txr.bottom - txr.top)* 1.5);
    
    /* I wish we had Lisp */

    int drawing_height = (NNums+3)*cellh + 2*title_height;
    int drawing_width = (NIDTypes+3)*cellw;
    if (2*cellw + title_width > drawing_width)
	    drawing_width = 2*cellw + title_width;

    int X, Y;

    if (drawing_width + Xused < PageWidth && drawing_height + Yused < PageHeight) {
	X = Xused;
	Y = 0;
    }
    else if (drawing_height + Yused < PageHeight) {
	X = 0;
	Y = Yused;
    }
    else {
	moreproc();
	X = Y = Xused = Yused = 0;
    }
    SelectObject (dc, F2);
    txr.left = X+2*cellw;
    txr.right = txr.left + title_width;
    txr.top = Y;
    txr.bottom = Y+2*cellh;
    DrawText (dc, title, strlen (title), &txr,
	      DT_LEFT | DT_BOTTOM | DT_SINGLELINE | DT_NOCLIP);

    SelectObject (dc, F1);
    if (drawing_width + X > Xused)
	Xused = X + drawing_width;

    int ixfirst = 0;
    int nids = NIDTypes;

    X += 2*cellw;
    Y += 2*cellh + 2*title_height;

    if (X + nids * cellw > PageWidth)
	nids = (PageWidth - X)/cellw;

    int ixend = ixfirst + nids;

    do {
	if (drawing_height + Yused >= PageHeight) {
	    moreproc();
	    SelectObject (dc, F1);
	    X = Y = Xused = Yused = 0;
	    X += 2*cellw;
	    Y += 2*cellh + 2*title_height;
	}

	nids = ixend - ixfirst;
	for (Rlyblock* rb = Chain; rb != NULL; rb = rb->next)
	    for (int i = 0; i < rb->count; i++) {
		Rlysym * rs = rb->rr[i].rs;
		short page = rb->rr[i].page;
		if (filter (rs, page)) {
                    int x=0, y = 0;
		    for (y = 0; y < NNums; y++)
			if (rs->n == Nums[y])
			    break;
		    for (x = 0; x < NIDTypes; x++)
			if (rs->type == IDTypes[x])
			    break;
		    if (x >= ixfirst && x < ixend)
			PrintPageNum (dc, X, Y, x-ixfirst, y,
				      cellh, cellw, page);
		}
	    }
	for (int xc = 0; xc < nids; xc++)
	    PrintRelayType (dc, X, Y, xc, cellh, cellw, NNums,
			    IDTypes[xc + ixfirst]);
	for (int yc = 0; yc < NNums; yc++)
	    PrintItemNumber (dc, X, Y, yc, cellh, cellw, nids, Nums[yc]);
	MoveTo (dc, X-1*cellw, Y);
	LineTo (dc, X + nids*cellw, Y);
	MoveTo (dc, X, Y);
	LineTo (dc, X, Y+NNums*cellh);

	ixfirst += nids;
	ixend += nids;
	if (ixend > NIDTypes)
	    ixend = NIDTypes;
	
	if (drawing_height + Y > Yused)
	    Yused = Y + drawing_height;
	Y += drawing_height;
    } while (ixfirst < NIDTypes);

    delete Nums;
    Nums = NULL;
    delete IDTypes;
    IDTypes = NULL;
}




void ProduceRelayIndex (HDC dc, int (*moreproc) (void),
			HFONT f1, HFONT f2, int width, int height) {
    Xused = 0;
    Yused = 0;
    F1 = f1;
    F2 = f2;
    PageWidth = width;
    PageHeight = height;

    ProduceOneRelayIndex (dc, SignalFilter, "Signal Index", moreproc);
    ProduceOneRelayIndex (dc, SwitchFilter, "Switch Index", moreproc);
    ProduceOneRelayIndex (dc, TrackFilter, "Track/Auto Index", moreproc);
    ProduceOneRelayIndex (dc, RandomFilter, "Global Index", moreproc);
}


