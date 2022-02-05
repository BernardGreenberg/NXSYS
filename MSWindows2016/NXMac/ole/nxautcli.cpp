#ifdef NXOLE
#include "autclisub.h"
#include <nxproduct.h>
#else
#define PRODUCT_NAME "NXSYS"
#ifdef NXV2
#define EXE_NAME "nxv2"
#else
#define EXE_NAME "nxsys32"
#endif

#define WIN32_LEAN_AND_MEAN
#define FErInt fprintf
#define FErUsr fprintf
#endif
#include <windows.h>
#include <ole2.h>
#include <oleauto.h>
#include <stdio.h>
#include <stdlib.h>
#include "nxautcli.h"


/* Ole the Long-Armed towerman --  (nxctl)
   Copyright (c) Bernard S. Greenberg  26 December 1997 */

#ifdef NXV2
  #ifdef RT_PRODUCT
  #define CLSID_DIR PRODUCT_NAME ".Application.1\\CLSID"
  #else
  #define CLSID_DIR "NXSYS2.Application.1\\CLSID"
#endif
#else
#define CLSID_DIR "NXSYS.Application.1\\CLSID"
#endif

/* Get the necessary default libs in there */
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"oleaut32.lib")
#pragma comment(lib,"advapi32.lib")
#pragma comment(lib,"uuid.lib")

/* Copy of BSGLIB function to keep this .obj independent of it */
static char * Errstr (DWORD ercode) {
    static char szMessageBuffer[144];
    sprintf (szMessageBuffer, "Error 0x%lX\n", ercode);
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, ercode, LANG_USER_DEFAULT,
		  szMessageBuffer, sizeof(szMessageBuffer), NULL);
    return szMessageBuffer;
}


/* From here to the next "from here on" comment is all about the
   detailed mechanics of being an OLE Automation Client */

class CVariant : public VARIANTARG {public: CVariant() {VariantInit (this);}};

                           /* Convert ASCII string to Unicode */
static OLECHAR * ConvertStrAtoW(char* strIn, OLECHAR * buf, UINT size) {
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, strIn, -1, buf, size) ;
    return buf;
}

static void VarBSTRAssignFromAStr (VARIANT * v, char* astr) {
    int wl = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, astr, -1, NULL, 0);
    BSTR b = SysAllocStringLen (NULL, wl-1);
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, astr, -1, (WCHAR*)b, wl);
    V_VT(v) = VT_BSTR;
    V_BSTR(v) = b;
}

#define BoolifyVa() (Va.boolVal != VARIANT_FALSE);

#define SETDISPPARAMS(dp,numargs,pvargs,numnamed,pnamed) \
 {(dp).cArgs=numargs;(dp).rgvarg=pvargs;(dp).cNamedArgs=numnamed;\
  (dp).rgdispidNamedArgs=pnamed;}

static	DISPID		dispID, dispIDParam;
static	DISPPARAMS	Dp;
static	VARIANTARG	Va;
static	EXCEPINFO	exInfo;

                         /* Get the ID code for a named method/property */
static BOOL NameToID (IDispatch * disp, OLECHAR * name, DISPID &Dispid) {
    HRESULT hr = disp->GetIDsOfNames (IID_NULL, &name, 1, 0, &Dispid);
    if (FAILED (hr)) {
	FErInt (stderr, "Lookup of Method \"%S\" failed: %s\n", name, Errstr(hr));
	return FALSE;
    }
    return TRUE;
}

static BOOL HandleException () {
    FErUsr(stderr, "%S reported an error:\n   %S\n",
	   exInfo.bstrSource, exInfo.bstrDescription);
    if (exInfo.wCode == 0)
	FErUsr(stderr, "   %s\n", Errstr(exInfo.scode));
    SysFreeString (exInfo.bstrSource);
    SysFreeString (exInfo.bstrDescription);
    return FALSE;
}

static BOOL Invoker (IDispatch * idp, UINT code, OLECHAR* method) {
    DISPID DispId;
    if (!NameToID (idp, method, DispId))
	return FALSE;

    unsigned int uarg = 0;
    HRESULT hr = idp->Invoke(DispId, IID_NULL, 0,
			     code, &Dp, &Va, &exInfo, &uarg);
    if (hr == DISP_E_EXCEPTION)
	return HandleException();
    else if (hr == RPC_S_SERVER_UNAVAILABLE &&
	     !wcscmp (method, OLESTR("QUIT")))
	return TRUE;
    else if (FAILED(hr)) {
	FErInt (stderr, "Call to Method \"%S\" failed (argno %d): %s \n",
		method, uarg, Errstr(hr));
	return FALSE;
    }
    return TRUE;
}

                /* Writes an ASCII string to a 1-arg method (unicodizing) */
static BOOL CallWithAString(IDispatch * idp, OLECHAR * method, char * astring) {
    int nw = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, astring, -1, NULL, 0) + 1;
    WCHAR * wide = new WCHAR [nw];
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, astring, -1, wide, nw);
    BSTR bstr = SysAllocString (wide);
    delete wide;

    CVariant arg0;
    arg0.vt = VT_BSTR;
    arg0.bstrVal = bstr;

    SETDISPPARAMS(Dp, 1, &arg0, 0, NULL);
    if (!Invoker (idp, DISPATCH_METHOD, method)) {
	VariantClear(&arg0);
	return FALSE;
    }
    VariantClear(&arg0);
    return TRUE;
}

static BOOL PutpropCom (IDispatch * idp, VARIANTARG * argp, OLECHAR * method) {
    dispIDParam = DISPID_PROPERTYPUT;
    SETDISPPARAMS(Dp, 1, argp, 1, &dispIDParam); /* magic see KB p 749 */
    return Invoker (idp, DISPATCH_PROPERTYPUT, method);
}

/* Writes a boolean property value to NXSYS */
static BOOL CallPropPutBool (IDispatch * idp, OLECHAR* method, BOOL val) {

    CVariant a0;
    a0.vt = VT_BOOL;
    a0.boolVal =  val ? VARIANT_TRUE : VARIANT_FALSE;
    return PutpropCom (idp, &a0, method);
}

static BOOL CallPropPutString (IDispatch * idp, OLECHAR* method, char * s) {

    CVariant a0;
    VarBSTRAssignFromAStr(&a0, s);
    return PutpropCom (idp, &a0, method);
}

static BOOL CallPropPutDouble (IDispatch * idp, OLECHAR* method, double f) {

    CVariant a0;
    a0.vt = VT_R8;
    a0.dblVal = f;
    return PutpropCom (idp, &a0, method);
}

static BOOL Call2Bool (IDispatch * idp, OLECHAR* method, BOOL val1, BOOL val2) {

    CVariant a[2];
    a[0].vt = a[1].vt = VT_BOOL;
    /* We put 'em in REVERSE order, server retrieves them in RIGHT order */
    a[1].boolVal =  val1 ? VARIANT_TRUE : VARIANT_FALSE;
    a[0].boolVal =  val2 ? VARIANT_TRUE : VARIANT_FALSE;
    SETDISPPARAMS(Dp, 2, a, 0, NULL);
    return Invoker (idp, DISPATCH_METHOD, method);
}

                  /* Call a method with 0 args */
static BOOL Call0Args(IDispatch * idp, OLECHAR * method) {

    SETDISPPARAMS(Dp, 0, NULL, 0, NULL);
    return Invoker (idp, DISPATCH_PROPERTYGET | DISPATCH_METHOD, method);
}

                   /* Return a property value as a bool value  - ignore errors */
static BOOL CallGetBool (IDispatch * idp, OLECHAR * method) {

    VariantInit(&Va);
    SETDISPPARAMS(Dp, 0, NULL, 0, NULL);
    return Invoker(idp, DISPATCH_PROPERTYGET, method) && BoolifyVa();
}

static int CallGetInt (IDispatch * idp, OLECHAR * method) {

    VariantInit(&Va);
    SETDISPPARAMS(Dp, 0, NULL, 0, NULL);
    if (!Invoker(idp, DISPATCH_PROPERTYGET, method))
	return 0;
    return (int) Va.lVal;
}

static BOOL CallPutString  (IDispatch * idp, OLECHAR* method, char * val) {
    CVariant a0;
    VarBSTRAssignFromAStr(&a0, val);
    SETDISPPARAMS(Dp, 1, &a0, 0, NULL);
    return Invoker (idp, DISPATCH_METHOD, method);
}

static BOOL Call1Bool  (IDispatch * idp, OLECHAR* method, BOOL v) {
    CVariant a0;
    V_VT(&a0) = VT_BOOL;
    V_BOOL(&a0) = v ? VARIANT_TRUE : VARIANT_FALSE;
    SETDISPPARAMS(Dp, 1, &a0, 0, NULL);
    return Invoker (idp, DISPATCH_METHOD, method);
}

static BOOL CallReturnString (IDispatch * idp, OLECHAR* method,
			      char * buf, int bufl) {
    if (!Call0Args (idp, method)) {
	buf[0] = '\0';
	return FALSE;
    }
    BSTR b = V_BSTR(&Va);
    WideCharToMultiByte (CP_ACP, 0, b, SysStringLen(b)+1, buf, bufl, NULL, NULL);
    VariantClear(&Va);
    return TRUE;
}

/*Guts of get numbered named object (e.g., signal 34) */
/*Creates/returns an IDispatch for the object */
static IDispatch * GetNamedNumberedDispatch
   (IDispatch * generator,   /* The App or whatever generates this object */
    OLECHAR *   method,      /* the name of the method that does */
    int		item_id) {   /* the object (signal, switch, whatever) ## */

    CVariant a0;
    VariantInit(&Va);

    a0.vt = VT_I4;
    a0.lVal = item_id;

    DISPID DispId;
    if (!NameToID (generator, method, DispId))
	return NULL;

    SETDISPPARAMS(Dp, 1, &a0, 0, NULL);
    HRESULT hr = generator->Invoke(DispId, IID_NULL, 0,
				   DISPATCH_METHOD, &Dp, &Va, &exInfo, NULL);

    if (SUCCEEDED(hr))
	return (Va.pdispVal);	
    else if (hr == DISP_E_EXCEPTION) {
	 HandleException();
	 return NULL;
    }
    else
	FErInt (stderr, "Call to retrieve %S %d failed: %s\n", method,
		item_id, Errstr(hr));
    return NULL;
}

                    /*Chases down the Class ID in the CLASS_ROOT registry */
static BOOL GetRegistryData (CLSID &ClsID) {
    HKEY hKey;
    DWORD type;
    char ClsIDBuf[100];
    WCHAR ClsIDW[100];

    LONG ec = RegOpenKeyEx 
	      (HKEY_CLASSES_ROOT, CLSID_DIR, (DWORD) 0, KEY_READ, &hKey);
    if (ec == ERROR_FILE_NOT_FOUND) {
nreg:	FErUsr (stderr, "%s is not registered.  Invoke %s.exe /register\n", PRODUCT_NAME, EXE_NAME);
	return FALSE;
    }
    else if (ec != ERROR_SUCCESS) {
	FErInt (stderr, "Error opening registry key: %s", Errstr(ec));
	return FALSE;
    }
    DWORD bc = sizeof(ClsIDBuf);
    ec = RegQueryValueEx (hKey, "", NULL, &type, (PBYTE)ClsIDBuf, &bc);
    if (ec == ERROR_FILE_NOT_FOUND) {
	RegCloseKey(hKey);
	goto nreg;
    }
    else if (ec != ERROR_SUCCESS) {
	FErInt (stderr, "Error reading %s Class ID registry: %s",
		PRODUCT_NAME, Errstr(ec));
	RegCloseKey(hKey);
	return FALSE;;
    }
    RegCloseKey (hKey);

    ConvertStrAtoW(ClsIDBuf, ClsIDW, sizeof(ClsIDW)/sizeof(WCHAR));
    CLSIDFromString (ClsIDW, &ClsID);
    return TRUE;
}


NXSYS::NXSYS() : disp(NULL) {}

NXSYS::~NXSYS () {
    if (disp)
	disp->Release(); /* uh oh --- add () 6 January 1999 */
    OleUninitialize();
}

void NXSYS::Quit () {Call0Args(disp, OLESTR("QUIT"));}
void NXSYS::Load (char * s) {CallWithAString (disp, OLESTR("Load"), s);}
void NXSYS::Say (char * remark) {CallPutString (disp, OLESTR("Remark"), remark);}
void NXSYS::CancelAllSignals () {Call0Args (disp, OLESTR("CancelAllSignals"));}
void NXSYS::ResetAll () {Call0Args (disp, OLESTR("ResetAll"));}
void NXSYS::ResetApproach () {Call0Args (disp, OLESTR("ResetApproach"));}
void NXSYS::KillAllTrains () {Call0Args (disp, OLESTR("KillAllTrains"));}
void NXSYS::NormalAllSwitches () {Call0Args (disp, OLESTR("NormalAllSwitches"));}
void NXSYS::SystemDirectory (char * buf, int bufl) {
    CallReturnString (disp, OLESTR("SysDirectory"), buf, bufl);}

NXCBOOL NXSYS::ShowStops (char*cmd) {
    CallWithAString (disp, OLESTR("ShowStops"), cmd);
    return (NXCBOOL)BoolifyVa();
}

NXCBOOL NXSYS::RelayState (char * name, int & state) {
    if (!CallWithAString (disp, OLESTR("RelayState"), name))
	return FALSE;
    state =(int)BoolifyVa();
    return TRUE;
}

NXCBOOL NXSYS::Window (char*cmd) {
    CallWithAString (disp, OLESTR("Window"), cmd);
    return (NXCBOOL)BoolifyVa();
}

NXCBOOL NXSYS::Create () {

    CLSID ClsID;        /* To receive OLE class ID of NXSYS application */
    OleInitialize(NULL);
#ifdef NXOLE
    ClsID;
    disp = GetOLEAutInstanceInternal();
    if (!disp) {
	FErInt (stderr, "Could not get internal Automation class pointer.");
	return (NXCBOOL) FALSE;
    }
#else
    if (!GetRegistryData (ClsID))
	return (NXCBOOL) FALSE;
    HRESULT h = CoCreateInstance (ClsID, NULL, CLSCTX_LOCAL_SERVER,
				  IID_IDispatch, (void**) &disp);
    if (FAILED (h)) {
	FErInt (stderr, "CoCreateInstance %s failed: %s\n",
		PRODUCT_NAME, Errstr(h));
	return (NXCBOOL) FALSE;
    }
#endif
    return (NXCBOOL) TRUE;
};


NXAutoClientObj::NXAutoClientObj (NXSYS &n) : N(n), disp(NULL) {};
NXAutoClientObj::~NXAutoClientObj () {
    if (disp) disp->Release();
}

Signal::Signal (NXSYS& n) : NXAutoClientObj(n) {};

NXCBOOL Signal::Initiated () {
    return (NXCBOOL) CallGetBool(disp, OLESTR("Initiated"));}
NXCBOOL Signal::Initiate () {
    Call0Args (disp, OLESTR("Initiate"));
    return (NXCBOOL)BoolifyVa();
}
void Signal::Cancel () {Call0Args(disp, OLESTR("Cancel"));}
void Signal::Fleet () {Call0Args(disp, OLESTR("Fleet"));}
void Signal::ResetApproach() {Call0Args(disp, OLESTR("ResetApproach"));}
void Signal::UnFleet () {Call0Args(disp, OLESTR("UnFleet"));}
void Signal::Aspect (char * buf, int bufl) {
    CallReturnString (disp, OLESTR("Aspect"), buf, bufl);}
void Signal::FullSigWin (int v) {
    Call1Bool(disp, OLESTR("FullSigWin"), (BOOL)v);}

ExitLight::ExitLight (NXSYS& n) : NXAutoClientObj(n) {};
NXCBOOL ExitLight::Lit () {return (NXCBOOL)CallGetBool (disp, OLESTR("Lit"));};
void    ExitLight::Exit () {Call0Args(disp, OLESTR("Exit"));};

TrackSec::TrackSec (NXSYS& n) : NXAutoClientObj(n) {};
void	TrackSec::SetOccupied(NXCBOOL v) {
    CallPropPutBool (disp, OLESTR("Occupied"), (BOOL) v);
}
NXCBOOL	TrackSec::Occupied () {
    return (NXCBOOL) CallGetBool (disp, OLESTR("Occupied"));
}

NXCBOOL	TrackSec::Routed () {
    return (NXCBOOL) CallGetBool (disp, OLESTR("Routed"));
}

Switch::Switch (NXSYS &n) : NXAutoClientObj(n) {};


NXCBOOL Switch::Throw (int to_reverse, int hold) {
    return (NXCBOOL) Call2Bool (disp, OLESTR("Throw"),
				(BOOL)to_reverse, (BOOL)hold);
}


NXCBOOL Switch::IsNormal() {
    return (NXCBOOL) CallGetBool (disp, OLESTR("IsNormal"));
}

NXCBOOL Switch::IsReverse() {
    return (NXCBOOL) CallGetBool (disp, OLESTR("IsReverse"));
}

/* These three methods parse a string specification of a signal, exit light,
   track sec, etc, and return TRUE and set the IDispatch pointer if
   it succeeded */

NXCBOOL Signal::SetFromString (char * sigstring) {
    int offset = 0;
#ifdef NXV2
    if (strchr ("abcdefABCDEF", sigstring[0]))
	offset = (tolower(*sigstring++) - 'a')*1000+7000;
#endif
    int signo = atoi(sigstring);
    if (signo < 2) {			/* auto #s can be odd. */
	FErUsr (stderr, "Invalid signal number (must be positive): %s\n", sigstring);
	return (NXCBOOL) FALSE;
    }
    return ((disp = GetNamedNumberedDispatch
		    (N.disp, OLESTR("Signal"), signo + offset))!= NULL);
}


NXCBOOL ExitLight::SetFromString (char * elstring) {
    int elno = atoi(elstring);
    if (elno < 2 || (elno & 1)) {
	FErUsr (stderr, "Invalid exit light number (must be positive, even): %s\n",
		elstring);
	return (NXCBOOL)FALSE;
    }
    return (NXCBOOL)
	    ((disp=GetNamedNumberedDispatch
		   (N.disp, OLESTR("ExitLight"), elno)) !=NULL);
}	

NXCBOOL TrackSec::SetFromString (char * tkstring) {
    return (NXCBOOL)
	    ((disp = GetNamedNumberedDispatch
		     (N.disp, OLESTR("Track"), atol(tkstring))) != NULL);
}

NXCBOOL Switch::SetFromString (char * swstring) {
    int swno = atoi(swstring);
    if (swno < 1 || ((swno & 1) == 0)) {
	FErUsr (stderr, "Invalid signal # (must be + odd): %s\n", swstring);
	return (NXCBOOL) FALSE;
    }
    return ((disp = GetNamedNumberedDispatch
		    (N.disp, OLESTR("Switch"), swno))!= NULL);
}

NXCBOOL TrafficLever::SetFromString (char * tkstring) {
    return (NXCBOOL)
	    ((disp = GetNamedNumberedDispatch
		     (N.disp, OLESTR("TrafficLever"), atol(tkstring))) != NULL);
}


/* This is provided so that scripting programs can remain ANSI C++ without
   including Windows include files.  It is substituted for by in-NXSYS NXScript*/

Train::Train (NXSYS &n) : NXAutoClientObj(n) {};

#define T_MAX_VARGS 10
#define T_MAX_VARGSA (T_MAX_VARGS+2)
#define MAX_TN_ARGL 32

int Train::Create (long birthplace, int nargs, char** args) {

    int i;
    long trainno;
    CVariant varg[T_MAX_VARGSA];
    DISPID dispIDs[T_MAX_VARGS+1];
    char ArgName[MAX_TN_ARGL];
    WCHAR WArgName[MAX_TN_ARGL];
    BSTR bstrArgNames[T_MAX_VARGS+1];

    int allargc = 2 + nargs;
    for (i = 0; i < T_MAX_VARGS+1; i++)
	bstrArgNames[i] = NULL;

    for (i = 0; i < nargs; i++) {
	char * arg = args[i];
	if (strlen(arg) > MAX_TN_ARGL-1) {
	    FErUsr (stderr, "TRAIN arg too long: %s\n", arg);
	    break;
	}
	char * eqptr = strchr (arg, '=');
	if (eqptr == NULL) {
	    FErUsr (stderr, "Missing = in TRAIN arg: %s\n", arg);
	    break;
	}
	int nameln = eqptr-arg;
	strncpy (ArgName, arg, nameln)[nameln] = '\0';
	ConvertStrAtoW(ArgName, WArgName, sizeof(WArgName)/sizeof(WCHAR));	
	bstrArgNames[allargc-2-1+1-i] = SysAllocString(WArgName);
	VarBSTRAssignFromAStr(&varg[allargc-2-1-i], eqptr+1);
    }
    if (nargs && i >= nargs) {
	bstrArgNames[0] = OLESTR("CREATETRAIN");
	HRESULT hresult
		= N.disp->GetIDsOfNames
		  (IID_NULL, bstrArgNames, nargs+1, 0, dispIDs);
	for (i = 1; i < allargc - 2 + 1; i++)
	    if (bstrArgNames[i]) SysFreeString(bstrArgNames[i]);

	if (hresult != NOERROR &&
	    hresult != ResultFromScode(DISP_E_UNKNOWNNAME)){
	    FErInt (stderr, "Train arg name lookup failed: %s", Errstr(hresult));
	    return 0;
	}
    }

    for (i = 1; i < nargs+1; i++) /* #0 is method name, a constant */
	if (dispIDs[i] == -1) {
	    FErUsr (stderr, "TRAIN Arg \"%S\" unrecognized.\n", bstrArgNames[i]);
	    return 0;
	}

    varg[allargc-1].vt = VT_I4;
    varg[allargc-1].lVal = birthplace;
    varg[allargc-2].vt = VT_I4 | VT_BYREF;
    varg[allargc-2].plVal = &trainno;

    SETDISPPARAMS(Dp, allargc, varg, allargc-2, dispIDs+1);
    if (!Invoker (N.disp, DISPATCH_METHOD, OLESTR("CREATETRAIN"))) {
	return 0;
    }
    return trainno;
}

NXCBOOL Train::Kill () {Call0Args(disp, OLESTR("Kill"));
		return (NXCBOOL)BoolifyVa();
}
NXCBOOL Train::Halt () {Call0Args(disp, OLESTR("Halt"));
		return (NXCBOOL)BoolifyVa();
}
NXCBOOL Train::Reverse () {Call0Args(disp, OLESTR("Reverse"));
return (NXCBOOL)BoolifyVa();
}

NXCBOOL Train::AcceptCallOn () {Call0Args(disp, OLESTR("AcceptCallOn"));
	return (NXCBOOL)BoolifyVa();
}

NXCBOOL Train::SetFromTrainNo (long trainno) {
    return ((disp = GetNamedNumberedDispatch
		    (N.disp, OLESTR("Train"), trainno))!= NULL);
}

NXCBOOL Train::SetAutopilot (NXCBOOL b) {
    return CallPropPutBool (disp, OLESTR("Autopilot"), (BOOL) b);
}

NXCBOOL Train::ShowPanel (char * state) {
    return CallPropPutString (disp, OLESTR("Panel"), state);
}

double Train::Speed () {
    if (!Call0Args (disp, OLESTR("Speed")))
	return 0.0;
    return (V_R4(&Va));
}

NXCBOOL Train::SetSpeed (double speed) {
    return CallPropPutDouble (disp, OLESTR("Speed"), speed);
}

TrafficLever::TrafficLever (NXSYS &n) : NXAutoClientObj(n) {};

NXCBOOL TrafficLever::Throw (int to_reverse) {
    return (NXCBOOL) Call1Bool (disp, OLESTR("Throw"), (BOOL)to_reverse);
}

NXCBOOL TrafficLever::IsNormal() {
    return (NXCBOOL) CallGetBool (disp, OLESTR("IsNormal"));
}

NXCBOOL TrafficLever::IsReverse() {
    return (NXCBOOL) CallGetBool (disp, OLESTR("IsReverse"));
}

int TrafficLever::NormalLight () {
    return CallGetInt (disp, OLESTR("NormalLight"));
}

int TrafficLever::ReverseLight () {
    return CallGetInt (disp, OLESTR("ReverseLight"));
}

#ifndef NXOLE
void NXSYSSleepMilliseconds (long milliseconds) {
    Sleep (milliseconds);
}
#endif
