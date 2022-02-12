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
#include "tledit.h"
#include <string>
#include "STLfnsplit.h"
#include "SwitchConsistency.h"
#ifndef NXSYSMac
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

static FILE * S_f;
static TrackSeg * S_ts;
static TrackJoint * S_tj;
static int S_i;
static BOOL S_YKnown, S_ZKnown;
static double S_LastY;
//static double S_LastZ;  // I wish .


static const char *SWKeys[3] = { "STEM", "NORMAL", "REVERSE" };
static int Alternate[3] = { 1, 0, 0 };

static void SaveTheLayout(FILE * f), DumpRemainingObjects(FILE * f);

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
	std::string drive, dir, fname, ext;
	STLfnsplit(path, drive, dir, fname, ext);
	std::string bkpath = STLfnmerge(drive, dir, fname, ".bak");

	if (access(path, 0) != 0)	/* if .trk no exist, no prob. */
		return TRUE;

	/* .trk does exist. */
	if (access(path, 6) != 0) /* if can't write it, we're in trouble. */
		return FALSE;
	/* .trk exists and we can write it. */
	if (access(bkpath.c_str(), 0) == 0) 		/* if bk path exists */
	/* try to delete it */
		if (remove(bkpath.c_str()) != 0)	/* if we can't delete it */
			return FALSE;		/* report trouble. */
		/* try to rename real path to bkpath */
	if (rename(path, bkpath.c_str()) != 0)
		return FALSE;
	return TRUE;
}


BOOL SaveLayout(const char * path) {
    std::string complaint;
    if (!SwitchConsistencyTotalCheck(complaint)) {
        complaint += " -- NXSYS cannot load it until you fix this.  Won't save.";
        MessageBox(G_mainwindow, complaint.c_str(), app_name, MB_ICONEXCLAMATION);
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
	FILE * f = fopen(path, "w");
	if (f == NULL) {
		usererr("Cannot open %s for writing: %s", path,

#ifdef NXSYSMac
			strerror(NULL)

#else
			_strerror(NULL)
#endif
		);
		return FALSE;
	}
	SaveTheLayout(f);
	fclose(f);
	return TRUE;
}

static int CheckForJointNumber(GraphicObject *g) {
	TrackJoint * tj = (TrackJoint*)g;
	return (tj->Nomenclature == S_i);
}

static int AssignUnassignedSwitchNumbers(GraphicObject *g) {
	TrackJoint * tj = (TrackJoint*)g;
	if (tj->TSCount == 3 && tj->Nomenclature == 0L)
		for (S_i = UNASSIGNED_BASE_SWITCH_NO; ; S_i += 2)
			if (!MapGraphicObjectsOfType(ID_JOINT, CheckForJointNumber)) {
				tj->Nomenclature = S_i;
				tj->SwitchAB0 = 0;
				break;
			}
	return 0;
}

static int TSegClearMapper(GraphicObject *g) {
	((TrackSeg*)g)->Marked = FALSE;
	return 0;
}

static int JointClearMapper(GraphicObject *g) {
	TrackJoint * tj = (TrackJoint*)g;
	tj->Marked = FALSE;
	if (tj->TSCount == 1) {
		tj->Invalidate();
		tj->Insulated = TRUE;
	}
	if (tj->TSCount == 3)
		tj->Organize();
	if (tj->Insulated && tj->Nomenclature == 0L) {
		for (S_i = UNASSIGNED_BASE_IJ_NO; ; S_i += 1)
			if (!MapGraphicObjectsOfType(ID_JOINT, CheckForJointNumber)) {
				tj->Nomenclature = S_i;
				break;
			}
	}
	return 0;
}

static int FindUnmarkedLooseEnd(GraphicObject *g) {
	TrackJoint * tj = (TrackJoint*)g;
	if (tj->TSCount == 1 && !tj->Marked) {
		S_tj = tj;
		return 1;
	}
	return 0;
}

static int FindUnmarkedSwitchBranch(GraphicObject *g) {
	TrackJoint * tj = (TrackJoint*)g;
	for (int i = 0; i < tj->TSCount; i++) {
		if (!tj->TSA[i]->Marked) {
			S_i = i;
			S_ts = tj->TSA[i];
			S_tj = tj;
			return 1;
		}
	}
	return 0;
}

static void DumpArc(FILE * f, TrackSeg * ts, TrackJoint * tj) {

	const char * dir;

	long LastTCID = 0;

	while (1) {
		int fx = tj->FindEndIndex(ts);
		if (fx == TSA_NOTFOUND) {
			usererr("Dumper finds estranged segment/joint.");
			fprintf(f, "BUG-ERROR ... .... .... BAD BAD BAD\n");
			return;
		}

		long id = ts->Circuit ? ts->Circuit->StationNo : 0;

		/* dump seg attributes if appropriate*/
		if (LastTCID != id)
			fprintf(f, "     (TC %ld)\n", LastTCID = id);


		ts->Marked = TRUE;
		fx = 1 - fx;
		tj = ts->Ends[fx].Joint;
		if (tj->Marked) {
			if (tj->TSCount != 3) {
				usererr("Dumper finds marked node, not switch.");
			sfail:		tj->TDump(f, "BUG-ERROR BAD BAD");
			}
			else if (ts == tj->TSA[TSA_STEM]) {
				usererr("Dumper found way into marked switch via stem.");
				goto sfail;
			}
			else if (ts == tj->TSA[TSA_NORMAL])
				tj->TDump(f, "SWITCH NORMAL");
			else tj->TDump(f, "SWITCH REVERSE");

			return;
		}
		else if (tj->TSCount == 1) {
			tj->TDump(f, "IJ");
			return;
		}

		if (tj->TSCount == 3) {
			dir = NULL;
			for (int k = 0; k < 3; k++)
				if (ts == tj->TSA[k]) {
					dir = SWKeys[k];
					ts = tj->TSA[Alternate[k]];
					break;
				}
			if (dir == NULL) {
				usererr("Dumper found switch, but can't find seg in it.");
				return;
			}
			char buf[20];
			if (ts->Marked) {
				usererr("Dumper found already marked segment in unmarked switch.");
				tj->TDump(f, "BUG-ERROR BAD BAD");
				return;
			}
			sprintf(buf, "SWITCH %s", dir);
			tj->TDump(f, buf);
			continue;
		}
		else
			ts = (ts == tj->TSA[0]) ? tj->TSA[1] : tj->TSA[0];

		tj->TDump(f, NULL);

		if (ts->Marked) {
			usererr("Dumper found already marked segment.");
			tj->TDump(f, "BUG-ERROR BAD BAD");
			fprintf(f, "\n");
			return;
		}
	}
}


static void SaveTheLayout(FILE * f) {
	char buf[40];
	time_t the_time;
	time(&the_time);
	fprintf(f, ";; TLEdit output of %s", ctime(&the_time));
#ifndef NXSYSMac
	the_time = GetModuleTime(NULL);
	fprintf(f, ";;   by TLEdit of %s", ctime(&the_time));
#endif
	fprintf(f, ";;      TLEdit Copyright (c) Bernard Greenberg 2013\n");

	fprintf(f, ";;   Do not edit by hand.\n\n");

	fprintf(f, "(LAYOUT\n");
	fprintf(f, "  (VIEW-ORIGIN %d %d)\n", FixOriginWPX, FixOriginWPY);
	/* dump mwh/mww assumptions */
	/* date, time, layout name, stuff now in ROUTE */
	/* much of this as comments*/
	S_f = f;
	S_ts = NULL;
	S_tj = NULL;
	MapGraphicObjectsOfType(ID_JOINT, AssignUnassignedSwitchNumbers);
	MapGraphicObjectsOfType(ID_TRACKSEG, TSegClearMapper);
	MapGraphicObjectsOfType(ID_JOINT, JointClearMapper);
	while (MapGraphicObjectsOfType(ID_JOINT, FindUnmarkedLooseEnd)) {
		fprintf(f, "  (PATH\n");
		S_YKnown = S_ZKnown = FALSE;
		TrackJoint * tj = S_tj;
		tj->TDump(f, "IJ");
		/* if track circuit changes or not ij's nomen, dump that*/
		TrackSeg * ts = tj->TSA[0];
		DumpArc(f, ts, tj);
		fprintf(f, "  )\n");
	}

	while (MapGraphicObjectsOfType(ID_JOINT, FindUnmarkedSwitchBranch)) {
		fprintf(f, "  (PATH\n");
		S_YKnown = S_ZKnown = FALSE;
		sprintf(buf, "SWITCH %s", SWKeys[S_i]);
		S_tj->TDump(f, buf);
		DumpArc(f, S_ts, S_tj);
		fprintf(f, "  )\n");
	}
	fprintf(f, "\n");
	DumpRemainingObjects(f);
	fprintf(f, ")\n\n");
}

static void sDBB() {
	DebugBreak();
}


void TrackJoint::TDump(FILE * f, const char * key) {
	BOOL simple = (key == NULL) && !NumFlip;
	if (!S_YKnown || wp_y != S_LastY || Insulated)
		simple = FALSE;
	char buf[30];
	const char * ii = "";
	if (Insulated && key == NULL) {
		key = "IJ";
		simple = FALSE;
	}

	if (Insulated) {
		sprintf(buf, " %ld", Nomenclature);
		ii = buf;
	}
	const char * gl = "?";
	if (TSCount == 3) {
		if (Nomenclature == 0)
			usererr("Dumper found switch with no number.");
		simple = FALSE;
		switch (SwitchAB0) {
		case 0:
			gl = " 0";
			break;
		case 1:
			gl = " A";
			break;
		case 2:
			gl = " B";
		}
		ii = buf;
		sprintf(buf, " %ld%s", Nomenclature, gl);
		if (key == NULL)
			key = "SWITCH";
	}

	if ((key == NULL || !strcmp(key, "")) && ii && strcmp(ii, "")) {
		sDBB();
	}

	if (simple)
		fprintf(f, "%37s%4ld\n", "", wp_x);
	else {
		char keybuf[50];
		char numbuf[30];
		char extra[50];
		strcpy(extra, NumFlip ? " NUMFLIP" : "");
		if (key == NULL)
			key = "";
		sprintf(keybuf, "%-20s  %s", key, ii);
		if (Marked && !strncmp(key, "SWITCH", 6))
			fprintf(f, "    (%s)\n", keybuf);
		else {
			if (S_YKnown && wp_y == S_LastY && !strcmp(extra, ""))
				sprintf(numbuf, "%4ld", wp_x);
			else
				sprintf(numbuf, "%4ld  %4ld", wp_x, wp_y);
			fprintf(f, "    (%-30s  %s%s)\n", keybuf, numbuf, extra);
		}
	}
	S_YKnown = TRUE;
	S_LastY = wp_y;
	Marked = TRUE;
}

char TrackSeg::EndOrientationKey(int end_index) {

	double angle = atan2(SinTheta, CosTheta);
	if (end_index == 1)
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
