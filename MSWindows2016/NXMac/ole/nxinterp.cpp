/* NXOLE is defined when this file is included in NXSYS to run scripts.
   Only the !defined(NXOLE) code is used by the outboard automation controller
   NXCTL. */
#ifdef NXOLE
#include "autclisub.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "nxautcli.h"

/* Sample NXSYS OLE automation script in C++ */
/* Copyright(c) Bernard S. Greenberg December 1997 */

static void trainCommands (NXSYS &N, int argc, char ** argv);
static void routeCommand (NXSYS &N, int argc, char ** argv);
static void handleVerify (NXSYS& N, int argc, char ** argv);
static void InterpretCommand (NXSYS &N, int argc, char ** argv);


#ifdef NXOLE
#include "autclisub2.h"
#else
static void print_commands () {
    printf ( "Commands:\n"
	     "  (Blank lines and text after ; is ignored (except as part of SAY text))\n"

	     "  FILE path              Run commands from \"path\"\n"
	     "  QUIT                   Close the NXSYS app\n"
	     "  LOAD path              Load up an interlocking (or L)\n"
	     "  INITIATE nnn           Initiate at signal nnn (or I)\n"
	     "  CANCEL nnn             Cancel signal nnn (or C)\n"	    
	     "  CANCEL ALL             Cancel all signals\n"
	     "  FLEET nnn              Fleet and try to initiate signal nnn\n"
	     "  UNFLEET nnn            Cancel fleeting on signal nnn\n"
	     "  ASPECT nnn             Show aspect of signal nnn (or A)\n"
	     "  FSD nnn {hide}         Show (hide) full signal disp nnn window\n"
	     "  EXIT xxx               Press exit button xxx (or X)\n"
	     "  OCCUPY ttt             Occupy track section ttt (or O)\n"
	     "  VACATE ttt             Vacate track section ttt (or V)\n"
	     "  SWITCH nn P {hold}     P is R or N, hold locks key (SWITCH or W)\n"
	     "  SWITCH NALL            Normal all switchs\n"
	     "  ROUTE nnn xxx          Line a route from sig nnn to xxx (or R)\n"
             "  RESETAPPROACH nnn/all  (resetas) Reset approach and time locking\n"
	     "  RESETALL               Cancel sigs/sw keys, clear track,reset app lk.\n"
	     "  RELAY nnNNN            Show the state of given relay or logic point.\n"
	     "  VERIFY rly rly !rly... Verify true/false logic terms\n"
	     "  WAIT ms                Wait ms milliseconds (useful in files)\n"
	     "  WINDOW cmd             maximize, minimize, restore app window\n"
	     "  SHOWSTOPS cmd          always, never, tripping\n"
	     "  SAY stuff stuff stuff  Print text as NXSYS demo legend (not on cmd line)\n"
	     "  TRAFFICLEVER n {N/R}   (tl) Throw, rept status. No last arg, just status.\n"
	     "  TRAIN starttrk {SPEED=n.n} {PANEL=MIN/REST/HIDE}{ID=n}\n"
	     "                         {CONTROLMODE=AUTO|MANUAL} (dft auto anon)\n"
	     " TRAIN-related cmds: HALT n, KILL n, SPEED n ff.ff, PANEL MIN/REST/HIDE,\n"
	     "  AUTOPILOT n {off}, ACCEPTCO n, REVERSE n.  (KILL ALL, too).\n"

	  );
}


static void print_usage () {
    printf ("NXCTL command controls NXSYS via OLE, starts one if not running.\n"
	    "Copyright(c) Bernard S. Greenberg December 1997\n"
	    "You must be using OLE-enabled NXSYS32, and have issued the command\n"
	    "  nxsys32 /register\n"
	    "at some time previously.  Known commands: (case insignificant)\n"
	    "\n"
	    " NXCTL SHELL		 Enter interactive loop accepting commands\n"
	    " NXCTL command args args Execute the command specified\n");
    print_commands ();
}
#endif


int ParseExecuteCommand (NXSYS &N, char * b) {
    int argc = 0;
    char * argv[10];

    char * eolp = strchr (b, '\n');
    if (eolp) *eolp = '\0';
    while (isspace ((char unsigned) *b)) b++;
    if (!_strnicmp (b, "say", 3)) {
	N.Say (b+3);
	return 1;
    }
    eolp = strchr (b, ';');
    if (eolp) *eolp = '\0';
    for (char *tok = strtok (b, " \t\r\n"); tok && argc < 10;
	 tok = strtok (NULL, " \t\r\n")) {
	if (!!_stricmp (tok, "nxctl"))	/* heh, heh */
	    argv[argc++] = tok;
    }
	 if (argc > 0) {
	     if(!_stricmp (argv[0], "leave"))
		 return 0;
	     InterpretCommand (N, argc, argv);
	     if (!_stricmp (argv[0], "quit"))
		 return 0;
	 }
	 return 1;
}

int ReadParseExecuteCommand (NXSYS &N, FILE * f, int echo) {
    char buffer[128];

    if (!fgets (buffer, sizeof(buffer), f))
	return 0;

    if (echo)
	printf ("> %s", buffer);

    return ParseExecuteCommand (N, buffer);
}
    

static void ReportTLight (int val, char * s) {
    char buf[10];
    char *t = buf;
    switch (val) {
	case -1: t = "RED"; break;
	case +1: t = "WHITE"; break;
	case 0: return;
	default:
	    sprintf (buf, "%d", val);
	    break;
    }
    printf ("%s light is %s", s, t);
}

static void InterpretCommand (NXSYS &N, int argc, char ** argv) {

    char * cmd = argv[0];
    char * arg = (argc >= 2) ? argv[1] : NULL;
    if (!_stricmp(cmd, "quit"))
	N.Quit();	      /* Quit (close) the NXSYS app */

#ifndef NXOLE
    else if (!_stricmp(cmd, "shell")) {
	printf ("Enter commands, \"leave\" to exit loop, or \"quit\" to quit NXSYS and exit\n\n");
	do {
	    printf ("NXCTL> "); fflush(stdout);
	} while (ReadParseExecuteCommand (N, stdin, 0));
    }
#endif
    else if (!_stricmp(cmd, "load") || !_stricmp(cmd, "l"))
	if (argc >= 2) {  /* Load an interlocking by pathname */
	    char abspath [_MAX_PATH];
	    _fullpath (abspath, arg, sizeof(abspath));
	    N.Load (abspath); /* Operate the NXSYS object */
	}
	else fprintf (stderr, "Expected path after LOAD missing.\n");


    else if (!_stricmp(cmd, "initiate") || !_stricmp(cmd, "i")) /*Initiate a signal
								(or route) */
	if (argc >= 2) {
	    Signal Sig(N);    /* Constructor must be given the NXSYS object */
	    if (Sig.SetFromString (arg)) /* try to access that signal */
		if (!Sig.Initiate()) /* Tell it (actually, its NX button) to initiate*/
		    fprintf (stderr, "Couldn't initiate %s\n", arg);
	}
	else fprintf (stderr, "Expected signal # after INITIATE missing.\n");

    else if (!_stricmp(cmd, "cancel") || !_stricmp(cmd, "c")) /* Cancel sig/route */
	if (argc >= 2) {
	    if (!_stricmp (arg, "all") || !_stricmp (arg, "a"))
		N.CancelAllSignals(); /* Tell NXSYS object to cancel all */
	    else {
		Signal Sig(N); /* Constructor must be given NXSYS object */
		if (Sig.SetFromString (arg)) /* Access the signal */
		    Sig.Cancel(); /* if got it, tell it(s button) to cancel */
	    }
	}
	else fprintf (stderr, "Expected signal # after CANCEL missing.\n");

    else if (!_stricmp(cmd, "fleet"))	/* Fleet signal */
	if (argc >= 2) {
	    Signal Sig(N); /* Constructor must be given NXSYS object */
	    if (Sig.SetFromString (arg)) /* Access the signal */
		Sig.Fleet(); /* if got it, tell it(s button) to fleet */
	}
	else fprintf (stderr, "Expected signal # after FLEET missing.\n");

    else if (!_stricmp(cmd, "unfleet"))	/* unfleet signal */
	if (argc >= 2) {
	    Signal Sig(N); /* Constructor must be given NXSYS object */
	    if (Sig.SetFromString (arg)) /* Access the signal */
		Sig.UnFleet(); /* if got it, tell it(s button) to cancel */
	}
	else fprintf (stderr, "Expected signal # after UNFLEET missing.\n");

    else if (!_stricmp(cmd, "aspect") || !_stricmp(cmd, "a")) /* Aspect of sig */
	if (argc >= 2) {
	    Signal Sig(N); /* Constructor must be given NXSYS object */
	    if (Sig.SetFromString (arg)) { /* Access the signal */
		char buf[20];
		Sig.Aspect(buf,sizeof(buf));
		printf ("Aspect of %s is %s\n", arg, buf);
	    }
	}
	else fprintf (stderr, "Expected signal # after ASPECT missing.\n");

    else if (!_stricmp(cmd, "fsd"))	/* Full signal display */
	if (argc >= 2) {
	    Signal Sig(N); /* Constructor must be given NXSYS object */
	    if (argc >= 3 && !!_stricmp (argv[2], "hide"))
		fprintf (stderr, "Optional arg to FSD must be \"hide\", "
			 "not %s\n", argv[2]);
	    else
		if (Sig.SetFromString (arg))/* Access the signal */
		    Sig.FullSigWin(argc < 3);
	}
	else fprintf (stderr, "Expected signal # after ASPECT missing.\n");

    else if (!_stricmp(cmd, "exit") || !_stricmp(cmd, "x")) /* Punch in an exit */
	if (argc >= 2) {
	    ExitLight Exit(N); /* declare an ExitLight variable */
	    if (Exit.SetFromString(arg)) /* Try to access the exit light */
		if (Exit.Lit())
		    Exit.Exit();  /* push its exit button */
		else fprintf (stderr, "Exit %s is not lit, can't punch it.\n", arg);
	}
	else  fprintf (stderr, "Expected exit light # after exit missing.\n");

    else if (!_stricmp(cmd, "occupy") || !_stricmp(cmd, "o"))
	if (argc >= 2) {		/* Mark a track section "occupied" */
	    TrackSec T(N);
	    if (T.SetFromString(arg))
		T.SetOccupied(1);
	}
	else  fprintf (stderr, "Expected track sec # after OCCUPY missing.\n");

    else if (!_stricmp(cmd, "vacate") || !_stricmp(cmd, "v"))
	if (argc >= 2) {		/* Mark a track section "vacant" */
	    TrackSec T(N);
	    if (T.SetFromString(arg))
		T.SetOccupied(0);
	}
	else fprintf (stderr, "Expected track sec # after VACATE missing.\n");

    /* Set up a route between a signal and an exit */

    else if (!_stricmp (cmd, "switch") || !_stricmp (cmd, "w") ||
	     !_stricmp (cmd, "sw"))
	if (argc == 2 && !_stricmp (argv[1], "nall"))
	    N.NormalAllSwitches();
	else if (argc < 3)
	    fprintf (stderr, "Expected # and position for SWITCH missing.\n");
	else {
	    char * swno = argv[1];
	    char * pos = argv[2];
	    if (!!_stricmp (pos, "n") && !!_stricmp (pos, "r"))
		fprintf (stderr, "SWITCH Position must be N or R, not \"%s\"\n",
			 pos);
	    else if (argc >= 4 && !!_stricmp (argv[3], "hold"))
		fprintf (stderr, "SWITCH optional arg must be \"hold\".\n");
	    else {
		int to_rev = !_stricmp (pos, "r");
		int hold = (argc >= 4);
		Switch W(N);
		if (W.SetFromString (swno))
		    W.Throw (to_rev, hold);
	    }
	}	    
    else if (!_stricmp(cmd, "route") || !_stricmp(cmd, "r"))
	routeCommand (N, argc, argv);
    else if (!_stricmp (cmd, "wait") || !_stricmp (cmd, "w"))
	if (argc < 2)
	    fprintf (stderr, "Millisecond count after WAIT missing.\n");
	else NXSYSSleepMilliseconds (atol(arg));
    else if (!_stricmp (cmd, "file")) {
	if (argc < 2)
	    fprintf (stderr, "Argument missing for FILE command.\n");
	else {
	    FILE * f = fopen (arg, "r");
	    if (f) {
		while (ReadParseExecuteCommand (N, f, 1));
		fclose(f);
	    }
	    else fprintf (stderr, "Can't open file %s: %s\n", arg, _strerror (NULL));
	}
    }
    else if (!_stricmp (cmd, "train")) {
	if (argc < 2)
	    fprintf (stderr, "TRAIN start-track arg missing.\n");
	else {
	    Train T(N);
	    int tn = T.Create (atol(argv[1]), argc-2, argv+2);
	    if (tn)
#ifdef NXOLE
		;
#else
		printf ("Created train %d\n", tn);
#endif
	    else fprintf (stderr, "Failed to create train.\n");
	}
    }
    else if (!_stricmp (cmd, "halt") || !_stricmp (cmd, "kill") ||
	     !_stricmp (cmd, "speed") || !_stricmp (cmd, "autopilot") ||
	     !_stricmp (cmd, "auto") ||
	     !_stricmp (cmd, "panel") || !_stricmp (cmd, "reverse"))
	trainCommands (N, argc, argv);
    else if (!_stricmp (cmd, "showstops"))
	if (argc < 2)
	    fprintf (stderr, "ShowStops policy arg missing.\n");
	else
	    if (!N.ShowStops (argv[1]))
		fprintf (stderr, "ShowStops arg rejected: %s\n", argv[1]);
	    else;
    else if (!_stricmp (cmd, "trafficlever") || !_stricmp (cmd, "tl")) {
	if (argc < 2)
	    fprintf (stderr, "Traffic lever ID missing.\n");
	else {
	    TrafficLever TL(N);
	    if (TL.SetFromString(arg)) {
		if (argc >= 3) {
		    if (!_stricmp (argv[2], "n"))
			TL.Throw (0);
		    else if (!_stricmp (argv[2], "r"))
			TL.Throw (1);
		    else
			fprintf (stderr, "Unknown TL direction: %s must be N or R.\n",
				 argv[2]);
		}
		printf ("Lever now %s\n", TL.IsReverse() ? "Reverse" : "Normal");
		ReportTLight (TL.NormalLight(), "Normal");
		ReportTLight (TL.ReverseLight(), "Reverse");
	    }
	}
    }
    else if (!_stricmp (cmd, "relay"))
	if (argc < 2)
	    fprintf (stderr, "Relay name after RELAY missing.\n");
	else {
	    int state;
	    /* not found gets diagnosed at lower level */
	    if (N.RelayState (argv[1], state))
		printf ("Relay %s is %s\n", argv[1], state? "PICKED" : "DROPPED");
	}
    else if (!_stricmp (cmd, "verify"))
	handleVerify (N, argc-1, argv+1);
    else if (!_stricmp (cmd, "window"))
	if (argc < 2)
	    fprintf (stderr, "Window state after APPWINDOW missing.\n");
	else
	    if (!N.Window (argv[1]))
		fprintf (stderr, "WINDOW arg rejected: %s\n", argv[1]);
	    else;
    else if (!_stricmp (cmd, "resetapproach") ||!_stricmp (cmd, "resetas")) {
	if (argc >= 2) {
	    if (!_stricmp (arg, "all") || !_stricmp (arg, "a"))
		N.ResetApproach (); /* Tell NXSYS object to reset all */
	    else {
		Signal Sig(N); /* Constructor must be given NXSYS object */
		if (Sig.SetFromString (arg)) /* Access the signal */
		    Sig.ResetApproach(); /* if got it,pulse U timer */
	    }
	}
	else fprintf (stderr, "Expected signal #/all after RESETAPPROACH missing.\n");
    }
    else if (!_stricmp (cmd, "resetall"))
	N.ResetAll();
    else {
	fprintf (stderr, "Unknown command: %s\n", cmd);
#ifndef NXOLE
	print_commands();
	printf (
		"  LEAVE                  Exit the command, leave NXSYS running\n"
	       );
#endif
    }
}

static void handleVerify (NXSYS &N, int argc, char ** argv) {
    while (argc--) {
	char * arg = *argv++;
	char * term = arg;
	int want = 1;
	int got;
	if (term[0] == '!') {
	    want = 0;
	    term++;
	}
	if (N.RelayState (term, got))
	    if ((!!want) != (!!got))
		fprintf (stderr, "Script Logic check failure: term %s FALSE.\n", arg);
    }
}


static void trainCommands (NXSYS &N, int argc, char ** argv) {
    char * cmd = argv[0];
    if (argc < 2) {
	fprintf (stderr, "TRAIN ID number missing for \"%s\".\n", cmd);
	return;
    }

    if (!_stricmp (argv[0], "kill") && !_stricmp (argv[1], "all")) {
	N.KillAllTrains();
	return;
    }


    char * endptr;
    long tn = strtol (argv[1], &endptr, 10);
    if (*endptr != '\0' || tn <= 0) {
	fprintf (stderr, "Bad train number: %s\n", argv[1]);
	return;
    }

    Train T(N);
    if (!T.SetFromTrainNo (tn)) {
	fprintf (stderr, "Cannot access train #%d.\n", tn);
	return;
    }

    if (!_stricmp (cmd, "halt"))
	T.Halt();
    else if (!_stricmp (cmd, "kill"))
	T.Kill();
    else if (!_stricmp (cmd, "acceptco"))
	T.AcceptCallOn();
    else if (!_stricmp (cmd, "reverse"))
	T.Reverse();
    else if (!_stricmp (cmd, "panel"))
	if (argc < 3)
	    fprintf (stderr, "Missing arg for PANEL - restore, minimize, etc.");
	else T.ShowPanel (argv[2]);
    else if (!_stricmp (cmd, "autopilot") ||
	     !_stricmp (cmd, "auto")) {
	if (argc < 3)
	    T.SetAutopilot(1);
	else if (!_stricmp (argv[2], "off"))
	    T.SetAutopilot(0);
	else fprintf(stderr, "AUTOPILOT arg must be \"off\".\n");
    }
    else if (!_stricmp (cmd, "speed"))
	if (argc < 3)
	    printf ("Speed = %.2f\n", T.Speed());
	else {
	    char * ep;
	    double dv = strtod (argv[2], &ep);
	    if (*ep != '\0' || dv < 0.0)
		fprintf (stderr, "Bad speed: %s\n", argv[2]);
	    else
		T.SetSpeed(dv);
	}
}

static void routeCommand (NXSYS &N, int argc, char ** argv) {
    if (argc < 3) {
	fprintf (stderr, "Expected 2 signal #s after ROUTE missing.\n");
	return;
    }
    char * entrance = argv[1];
    char * exit = argv[2];


    Signal Sig(N);    /* two variables for entrance and exit */
    ExitLight E(N);

    /* If we can successfully access the signal and exit light req'd...*/
    if (!(Sig.SetFromString(entrance) && E.SetFromString(exit)))
	return;

    if (Sig.Initiated()){ /* Check that it's not already punched in */
	fprintf (stderr,"Signal %s already initiated. Cancel first\n",
		 entrance);
	return;
    }

    if (!Sig.Initiate()) { /* Initiate at the entrance*/
	fprintf (stderr, "Couldn't initiate %s\n", entrance);
	return;
    }

    if (E.Lit()) /* If we get an exit light (should be instant) */
	E.Exit(); /* Punch in the exit. */
    else {
	Sig.Cancel(); /* otherwise, cancel the entrance */
	fprintf (stderr,"Didn't get your exit at %s\n", argv[2]);
    }
}



#ifndef NXOLE
int main (int argc, char ** argv) {

    if (argc < 2) {
	print_usage();
	return 1;
    }

    /* Create the NXSYS app instance */
    
    NXSYS N;                  /* this self-cleans-up  */

    if (!N.Create())	      /* This really tries to start NXSYS*/
	return 2;	      /* or fails (after printing an error) if can't */

    /* Slight bug that NXSYS will come up even for an unknown command */
    
    InterpretCommand (N, argc-1, argv+1);

    /* Note that automatic destruction of the NXSYS object will clean up
       the world here, making necessary OLE dereferences. */
       
    return 0;
}
#endif