#include "windows.h"

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>

#include "compat32.h"
#include "nxgo.h"
#include "xtgtrack.h"
#include "text.h"
#include "swkey.h"
#include "trafficlever.h"
#include "objid.h"
#include "assignid.h"
#include "tledit.h"
#include <string>
#include "STLfnsplit.h"
#include "SwitchConsistency.h"
#include "STLExtensions.h"
#include <cassert>
#include <cerrno>
#include <exception>
#include <unordered_set>
#include <unordered_map>

#ifdef WIN32
#include <getmodtm.h>
#include <io.h>
#endif

#ifdef NXSYSMac
#include <unistd.h>
#else
#define access _access
#endif

#define UNASSIGNED_BASE_SWITCH_NO 10001
#define UNASSIGNED_BASE_IJ_NO     30001

static bool S_YKnown;
static WP_cord S_LastY;

#define NOSAVE_MSG " -- NXSYS cannot load it until you fix this.  Won't save."

/* Rewritten 13 Mar 2022 to remove C-style coding, use modern C++ with lambdas, thrown exceptions,
 sets, type-checkable enums instead of numbers or strings. Also changed to write to temp file first,
 to protect against blowouts overwriting good files with partially-written ones */

/* These exceptions are thrown by the dumper (saver) when it finds problems or logic errors
 OTHER THAN USER ERRORS (e.g., A switch with no B). These indicate bugs, but they must be
 aggressively checked for lest the dumper write out bad files, clobbering good ones and losing work. */

struct TLEditSaveException : public std::exception {
    std::string message;
    TLEditSaveException(const char * ctlstring, ...) {
        va_list args;
        va_start(args, ctlstring);
        message = FormatStringVA(ctlstring, args);
        MessageBox(nullptr, message.c_str(), "TLEdit Save logic error detected:", 0);
#ifdef DEBUG  /* In debug mode (XCode/VS) break into the debugger.  Release build will not. */
        message.clear(); //breakpoint this -- assert not as useful
#endif
    }
};

class IDAssign {
    int CurPointer;
    int Increment;
    std::unordered_set<int> Values;
public:
    IDAssign(int base, int increment) : CurPointer(base), Increment(increment) {}
    int MakeNew();
    void Register(int value) {
        Values.insert(value);
    }
    void Clear() {Values.clear();}
};

int IDAssign::MakeNew() {
    while (Values.count(CurPointer != 0)) {
        CurPointer += Increment;
    }
    int val = CurPointer;
    Register(val);
    return val;
}

static IDAssign SwitchNumbers(UNASSIGNED_BASE_SWITCH_NO, 2);
static IDAssign IJNumbers(UNASSIGNED_BASE_IJ_NO, 1);

static std::unordered_map<TSAX, const char *> SWKeys {
    {TSAX::STEM,"STEM"},
    {TSAX::NORMAL,"NORMAL"},
    {TSAX::REVERSE, "REVERSE"}};

static std::unordered_map<TRKPET, const char *> KeyStrings {
    {TRKPET::KINK, ""},
    {TRKPET::IJ, "IJ"},
    {TRKPET::SWITCH, "SWITCH"}
};

/* Declare-aheads; flaw in C language. PL/I didn't need 'em. */
static void SaveTheLayout(FILE * f), DumpRemainingObjects(FILE * f);
static void DumpTheActualGraph(FILE * f);
static void DumpPath(FILE * f, TrackSeg * ts, TrackJoint * tj);
static char CharizeAB0(long nomen, short AB0);
void ValidateTrackGraph();

typedef
struct SaveEntry {
	int Priority;
	int OriginalIndex;
	GraphicObject * Object;
} SvE, *SvEP;

typedef
struct SaveState {
	int Index;
	BOOL Counting;
	SvEP Table;
} SvS, *SvSP;

static BOOL MakeBackupCopy(const char * path) {
    /* This is a hedge against USER error, not app error. */

	std::string drive, dir, fname, ext;
	STLfnsplit(path, drive, dir, fname, ext);
	std::string bkpath = STLfnmerge(drive, dir, fname, ".bak");

	if (access(path, 0) != 0)	/* if .trk no exist, no prob. */
		return TRUE;

	/* .trk does exist. */
	if (access(path, 6) != 0) /* if can't write it, we're in trouble. */
		return FALSE;
	/* .trk exists and we can write it. */
    if (access(bkpath.c_str(), 0) == 0) {		/* if bk path exists */
        /* try to delete it */
		if (remove(bkpath.c_str()) != 0)	/* if we can't delete it */
			return FALSE;		/* report trouble. */
    }
		/* try to rename real path to bkpath */
	if (rename(path, bkpath.c_str()) != 0)
		return FALSE;

	return TRUE;
}

static int ReportUnreachableSegments(GraphicObject *g) {
    TrackSeg& S = *(TrackSeg*)g;
    if (!S.Marked) {
        usererr("There are unreachable track loops (e.g., attempted station platforms). "
                "Delete them or break them.  Selecting one such segment.");
        S.Select();
        return 1;
    }
    return 0;
}

static int ReportCorruptedJoints(GraphicObject *g) {
    
    TrackJoint& J = *(TrackJoint*)g;
    if (J.TSCount == 0) {
        usererr("Dumper finds TJ#%d with TSCount 0. Selecting it. ", J.Nomenclature);
        J.Select();
        return 1;
    }
    for (int x = 0; x < J.TSCount; x++) {
        if (J.TSA[x] == NULL) {
            usererr(FormatString("Dumper finds TJ#%d with TSCount %d, TSA[%d] null.",
                                 J.Nomenclature, J.TSCount, x).c_str());
            J.TSCount = x;
            J.Select();
            usererr("Downgrading TJ#%ld to order %d. Selecting it. Won't save now. Try again.", J.Nomenclature, J.TSCount);
            return 1;
        }
    }
    return 0;
}

BOOL SaveLayout(const char * path) {
    if (auto r = SwitchConsistencyTotalCheck()) {
        MessageBox(G_mainwindow, (r.value +  NOSAVE_MSG).c_str(), app_name, MB_ICONEXCLAMATION);
        return FALSE;
    }
    if (MapGraphicObjectsOfType(ID_JOINT, ReportCorruptedJoints)) {
   /*     Message already displayed */
        return FALSE;
    }

    /* THREE separate strategies to protect against writing corrupt files
       1. Run the dumper in effigy (i.e., with no output), just run-through and endure
          possible aborts (some side effects, such as insulating loose ends and assigning #'s)
       2. Write to a temporary file and only copy when succeeds.
       3. Don't even make a backup copy if there is an aborting error in the effigy run.

     */
    
    /* Run the dumper in effigy (no output) */
    /* Finds unreachable segments, too! */
    try {
        ValidateTrackGraph();
        
        if (MapGraphicObjectsOfType(ID_TRACKSEG, ReportUnreachableSegments))
            return FALSE;

    } catch (TLEditSaveException e) {
        return FALSE;
    }
    
    if (!MakeBackupCopy(path)) {
        if (IDYES != MessageBox
        (G_mainwindow,
            "Cannot make backup copy of layout. Proceed to save anyway?",
            app_name,
            MB_YESNOCANCEL | MB_ICONEXCLAMATION))
            return FALSE;
    }
    
    /* Run the dumper for real into a temp file */
    FILE * temp_file = tmpfile();
	if (temp_file == NULL) {
        usererr("Cannot open temp file for writing: %s", std::strerror(errno));
        return FALSE;
	}
    try {
        SaveTheLayout(temp_file);
    } catch (TLEditSaveException e) {
        /* If the dumper aborts, delete the temp file and don't touch the real one */
        //Message already printed by exception ctor
        fclose(temp_file);
        return FALSE;  //Skip file install/copy.
    }

    /* Copy the content from the temp file into the real file, delete the temp file.*/
    auto length = ftell(temp_file);
    std::vector<char>total_file_content((size_t)length);
    rewind(temp_file);
    size_t did_read = (size_t)fread(total_file_content.data(), 1, total_file_content.size(), temp_file);
    fclose(temp_file);   // This will delete it.

    FILE * real_file = fopen(path, "w");  // Text mode!
    if (real_file == NULL) {
        usererr("Cannot open file %s for writing: %s", path, std::strerror(errno));
        return FALSE;
    }
    fwrite(total_file_content.data(), 1, did_read, real_file);
	fclose(real_file);
	return TRUE;
}

static void SaveTheLayout(FILE * f) {
    time_t the_time;
    time(&the_time);
    fprintf(f, ";; TLEdit output of %s", ctime(&the_time));
#ifdef WIN32
    the_time = GetModuleTime(NULL);
    fprintf(f, ";;   by TLEdit of %s", ctime(&the_time));
#endif
    fprintf(f, ";;      TLEdit Copyright (c) Bernard Greenberg 2013, 2022\n");

    fprintf(f, ";;   Do not edit by hand.\n\n");

    fprintf(f, "(LAYOUT\n");
    fprintf(f, "  (VIEW-ORIGIN %d %d)\n", FixOriginWPX, FixOriginWPY);
    /* dump mwh/mww assumptions */
    /* date, time, layout name, stuff now in ROUTE */
    /* much of this as comments*/
    
    DumpTheActualGraph(f);

    DumpRemainingObjects(f);
    fprintf(f, ")\n\n");  // Close the "(LAYOUT" form.
}

void ValidateTrackGraph() {
    DumpTheActualGraph(nullptr);
}

static int FinalCleanUp(GraphicObject *g) {
	TrackJoint& J = *(TrackJoint*)g;

    /* Unmark all nodes, regardless of degree */
	J.Marked = FALSE;

    /* Insulate all end-nodes; they must then be redisplayed, so invalidate */
	if (J.TSCount == 1) {
		J.Invalidate();
		J.Insulated = TRUE;
	}

    /* And "Organize" all unorganized switches (measure their angles to determine normal/reverse */
    else if (J.TSCount == 3) {
        J.Organize();
    }
	return 0;
}

static void ChasePath(TrackJoint&J, TSAX branch_type, FILE* f) {
    if (f)
        fprintf(f, "  (PATH\n");
    S_YKnown = false;
    J.TDump(f, branch_type);
    DumpPath(f, J[branch_type], &J);
    if (f)
        fprintf(f, "  )\n");
}

static int ChaseLooseTrackEnds(GraphicObject *g, void * vfp) {
    FILE* f = (FILE*)vfp;
    TrackJoint& J = *(TrackJoint*)g;
    /* marking track-ends is necessary even with this improved loop, because an end can
       be found (and marked) by forward motion and must not be found again by this loop. */
    if (J.TSCount == 1 && !J.Marked)
        ChasePath(J, TSAX::IJR0, f);
    return 0; /* never stop */
}

static int ChaseUnmarkedSwitchBranches(GraphicObject * g, void * vfp) {
    TrackJoint& J = *(TrackJoint*)g;
    FILE * f = (FILE*)vfp;
    if (J.TSCount == 3)   // "SWITCH" means "** S W I T C H **", you Doofus!!! 3/12/2022
        for (auto branch_type : {TSAX::STEM, TSAX::NORMAL, TSAX::REVERSE})
            if (!J[branch_type]->Marked)  // Branch is NOT MARKED ....
                ChasePath(J, branch_type, f);
    return 0;
}

/* This chases and outputs one path from one track-end or switch branch to another,
   marking as it goes, so we can search for unmarked ones. */
static void DumpPath(FILE * f, TrackSeg * ts, TrackJoint * tj) {

	long LastTCID = 0;

	while (true) {
        if (ts->Marked)
            throw TLEditSaveException("Dumper found already marked segment.");


		TSEX fx = ts->FindEndIndex(tj);
		if (fx == TSEX::NOTFOUND)
            throw TLEditSaveException("Estranged track segment found.");

		long id = ts->Circuit ? ts->Circuit->StationNo : 0;

		/* dump seg attributes if appropriate*/
        if (f)
            if (LastTCID != id)
                fprintf(f, "     (TC %ld)\n", LastTCID = id);

		ts->Marked = TRUE;
        TrackJoint& J = *(ts->GetOtherEnd(fx).Joint);
        long jnom = J.Nomenclature;
		if (J.Marked) {
            if (J.TSCount != 3) {
                J.Select();
				throw TLEditSaveException("Dumper finds marked node #%ld not switch. Selecting it; Delete this trackage.", jnom);
            }
			else if (ts == J[TSAX::STEM])
                throw TLEditSaveException ("Dumper found way into marked switch %ld via stem.", jnom);
            else if (ts == J[TSAX::NORMAL])
                J.TDump(f, TSAX::NORMAL);
            else if (ts == J[TSAX::REVERSE])
                J.TDump(f, TSAX::REVERSE);
            else
                throw TLEditSaveException("Dumper entered switch %ld %c from segment %p, "
                                          "but the latter is none of its branches",
                                          J.Nomenclature,
                                          CharizeAB0(J.Nomenclature, J.SwitchAB0),
                                          ts);
			return;  // Marked switch hit,
		}
		else if (J.TSCount == 1) {   // A loose end; the path is complete!
			J.TDump(f, TSAX::NOTFOUND);
			return;
		}
        assert (J.TSCount != 1);

		if (J.TSCount == 3) {   // An unmarked (new) SWITCH
            TSAX found_tsax = TSAX::NOTFOUND;
            for (TSAX k : {TSAX::STEM, TSAX::NORMAL, TSAX::REVERSE}){
                TrackSeg* branch = J[k];
                if (branch == nullptr)
                    throw TLEditSaveException ("Null segment pointer @k=%d found in joint %ld", k, jnom);
                if (branch == ts) {
                    found_tsax = k;

                    /* This is why we can never enter a marked switch via the stem. If we enter an
                     unmarked switch via the stem, the stem seg is marked.  If we enter via the reverse
                     or normal branches, we exit via the stem.  Next time we hit this switch, it must
                     be through the other non-stem branch and it will be marked. */
                    
                    TSAX next = (k == TSAX::STEM) ? TSAX::NORMAL : TSAX::STEM;
                    ts = J[next];
                    break;
                }
            }
			if (found_tsax == TSAX::NOTFOUND)
                throw TLEditSaveException("Dumper found switch %ld, but can't find seg in it.", jnom);
			if (ts->Marked)
				throw TLEditSaveException("Dumper found already marked segment in unmarked switch %ld.", jnom);

            J.TDump(f, found_tsax);
            tj = &J;  //!!!!! for the loop!
			continue;
		}
        else {
            assert (J.TSCount == 2);
			ts = (ts == J[TSAX::IJR0]) ? J[TSAX::IJR1] : J[TSAX::IJR0];
        }

		J.TDump(f, TSAX::NOTFOUND);


        tj = &J;  // looooop.....
	}
}


static void DumpTheActualGraph(FILE* f) {
    /* f can be null for check only */
    
    /* Register all known IJ and Switch numbers with the generator maps */
    SwitchNumbers.Clear();
    IJNumbers.Clear();
    MapGraphicObjectsOfType(ID_JOINT, [](GraphicObject *g) {
        TrackJoint & J = *(TrackJoint*)g;
        if (J.Nomenclature) {
            if (J.TSCount == 3)
                SwitchNumbers.Register((int)J.Nomenclature);
            else
                IJNumbers.Register((int)J.Nomenclature);
        }
        return 0;
    });

    /* Assign currently unused numbers to unassigned joints and switches */
    MapGraphicObjectsOfType(ID_JOINT, [](GraphicObject *g) {
        TrackJoint& J = *(TrackJoint*)g;
        if (J.Nomenclature == 0) {
            if (J.TSCount == 3)
                J.Nomenclature = SwitchNumbers.MakeNew();
            else
                J.Nomenclature = IJNumbers.MakeNew();
            /* Cooperate with older system. */
            MarkIDAssign((int)J.Nomenclature);
        }
        return 0;
    });

    /* Clear Marked bits of track sections */
    MapGraphicObjectsOfType(ID_TRACKSEG, [](GraphicObject *g) {
        ((TrackSeg*)g)->Marked = FALSE;
        return 0;
    });
    
    /* Unmark joints/switches, insulate loose ends, "organize" switches */
	MapGraphicObjectsOfType(ID_JOINT, FinalCleanUp);

    /* These produce the actual output, must use 3 arg form and pass file */
    MapFindGraphicObjectsOfType (ID_JOINT, ChaseLooseTrackEnds, f);
    MapFindGraphicObjectsOfType (ID_JOINT, ChaseUnmarkedSwitchBranches, f);
    if (f)
        fprintf(f, "\n");
}

static char CharizeAB0(long nomen, short AB0) {
    if (nomen == 0)
        throw TLEditSaveException("Dumper found switch with no nomenclature number.");
    switch (AB0) {
        case 0: return '0';
        case 1: return 'A';
        case 2: return 'B';
        default:
            throw TLEditSaveException("Dumper found switch %ld with bogus A/B/0: %d", nomen, AB0);
    }
}

void TrackJoint::TDump(FILE * f, TSAX branch_type) {
    // f = NULL means "check only"

    if (TSCount == 2 && !Insulated //not terminal, not switch, not IJ, just random joint ("kink")
        && S_YKnown && wp_y == S_LastY) {

        if (f)
            fprintf(f, "%37s%4ld\n", "", wp_x);  // simple single-number form
        Marked = TRUE;
        // S_YKnown and S_LastY remain same by definition
        return;
    }

    char identifier [30] = "";   // Assumption = "vanilla kink",i.e., non-insulated joint
    TRKPET type;

    if (Insulated) {
        if (TSCount == 3)
            throw TLEditSaveException("Insulated switch found: %ld", Nomenclature);
        /* 2 or 1 is right*/
        type = TRKPET::IJ;  // shouldn't really happen
        sprintf(identifier, " %ld", Nomenclature);
    }
    else
        type = TRKPET::KINK;

    if (TSCount == 3) {
        type = TRKPET::SWITCH;
        sprintf(identifier, " %ld %c", Nomenclature, CharizeAB0(Nomenclature, SwitchAB0));
    }

    if (f) {  // !f = dumper running in effigy/pre-test mode
        char key_plus_id[64];
        if (type == TRKPET::SWITCH)
            sprintf(key_plus_id, "SWITCH %-14s %s", SWKeys[branch_type], identifier);
        else
            sprintf(key_plus_id, "%-20s  %s", KeyStrings[type], identifier);

        if (Marked && TSCount == 3)   // Marked switch, location already published.
            fprintf(f, "    (%s)\n", key_plus_id); //(SWITCH NORMAL 10089 0)
        else if (S_YKnown && wp_y == S_LastY && !NumFlip)
            fprintf(f, "    (%-30s  %4ld)\n", key_plus_id, wp_x);
        else
            fprintf(f, "    (%-30s  %4ld  %4ld%s)\n", key_plus_id, wp_x, wp_y,
                    NumFlip ? " NUMFLIP" : "");
    }

	S_YKnown = true;
	S_LastY = wp_y;
	Marked = TRUE;
}

char TrackSeg::EndOrientationKey(TSEX end_index) {

	double angle = atan2(SinTheta, CosTheta);
	if (end_index == TSEX::E1)
		angle += CONST_PI;
	if (angle > CONST_2PI)
		angle -= CONST_2PI;

	if (angle < 0.0)
		angle += CONST_2PI;

	angle -= CONST_PI / 4.0;
	for (int i = 0; i < 5; i++)
		if (angle < 0.0)
			return "RBLTR"[i];
		else
			angle -= CONST_PI / 2.0;
	return ' ';
}


int GraphicObject::Dump(FILE * f) {
	return 0;
}

static int DROMapper(GraphicObject * g, void * v) {
	SvSP sv = (SvSP)v;
	int priority = g->Dump(NULL);
	if (priority > 0) {
		int index = sv->Index++;
		if (!sv->Counting) {
			SvEP e = &(sv->Table[index]);
			e->Object = g;
			e->OriginalIndex = index;
			e->Priority = priority;
		}
	}
	return 0;
}

static int DROComparer(const void * v1, const void * v2) {
	SvEP e1 = (SvEP)v1;
	SvEP e2 = (SvEP)v2;
	if (e2->Priority > e1->Priority)
		return -1;
	if (e2->Priority < e1->Priority)
		return +1;
	if (e2->OriginalIndex > e1->OriginalIndex)
		return -1;
	if (e2->OriginalIndex < e1->OriginalIndex)
		return +1;
	return 0;
}

static void DumpRemainingObjects(FILE * f) {
    SaveState SS{};
	SS.Index = 0;
	SS.Counting = TRUE;
	SS.Table = NULL;
	MapAllGraphicObjects(DROMapper, &SS);
	if (SS.Index <= 0)
		return;
	SS.Table = new SaveEntry[SS.Index];
	SS.Counting = FALSE;
	SS.Index = 0;
	MapAllGraphicObjects(DROMapper, &SS);
	qsort(SS.Table, SS.Index, sizeof(SaveEntry), DROComparer);
	int last_prio = 0;
	for (int i = 0; i < SS.Index; i++) {
		int prio = SS.Table[i].Priority;
		if (prio != last_prio) {
			last_prio = prio;
			fprintf(f, "\n");
		}
		SS.Table[i].Object->Dump(f);
	}
	delete SS.Table;
}
