#include <windows.h>
#include <ole2.h>
#include <oleauto.h>
#include "oleutil.h"
#include "cnxapp.h"
#include "csignal.h"
#include "cexitlt.h"
#include "ctrack.h"
#include "ctrain.h"
#include "cswitch.h"
#include "ctrafficlever.h"
#include "ecodes.h"
#include <ssdlg.h>

CNXApp::~CNXApp() {}
    
CNXApp * CNXApp::Create() {

    return new CNXApp();
}

/* used by inboard automation client to avoid requiring registry */
IDispatch * GetOLEAutInstanceInternal () {
    return CNXApp::Create();
}

DEFDISPATCH_HEAD(CNXApp)
    DISPTE(NXAPP,STATUS),
    DISPTE(NXAPP,RELAYSTATE),
    DISPTE(NXAPP,LOAD),
    DISPTE(NXAPP,SYSDIRECTORY),
    DISPTE(NXAPP,REMARK),
    DISPTE(NXAPP,GETSIGNAL),
    DISPTE2(NXAPP,SIGNAL, GETSIGNAL),
    DISPTE(NXAPP,GETSWITCH),
    DISPTE2(NXAPP,SWITCH,GETSWITCH),
    DISPTE(NXAPP,GETEXITLIGHT),
    DISPTE2(NXAPP,EXITLIGHT,GETEXITLIGHT),
    DISPTE(NXAPP,GETTRACK),
    DISPTE2(NXAPP,TRACK,GETTRACK),
    {OLESTR("CREATETRAIN"),IDMEMBER_NXAPP_CREATETRAIN,
	    CreateTrainPARAMDESC, &iCreateTrainPARAMDESC_LEN},
    DISPTE(NXAPP,GETTRAIN),	    
    DISPTE2(NXAPP,TRAIN,GETTRAIN),
    DISPTE(NXAPP,GETTRAFFICLEVER),
    DISPTE2(NXAPP,TRAFFICLEVER,GETTRAFFICLEVER),
    DISPTE(NXAPP,CANCELALLSIGNALS),
    DISPTE(NXAPP,KILLALLTRAINS),
    DISPTE(NXAPP,NORMALALLSWITCHES),
    DISPTE(NXAPP,WINDOW),
    DISPTE(NXAPP,SHOWSTOPS),
    DISPTE(NXAPP,RESETALL),
    DISPTE(NXAPP,RESETAPPROACH),
    DISPTE(NXAPP,QUIT)
DEFDISPATCH_TAIL(CNXApp);

static void ReturnSysDirectory (VARIANT * pvarResult) {
    char dir [_MAX_DIR];
    char path [_MAX_PATH];
    char drive [_MAX_DRIVE];
    GetModuleFileName (NULL, path, sizeof(path)-1);
    _splitpath (path, drive, dir, NULL, NULL);
    _makepath (path, drive, dir, NULL, NULL);
    VarBSTRAssignFromAStr (pvarResult, path);
}


static HRESULT ReturnNumberedObject (long n, OLECHAR * what,
				     CNXOleObj * INXObj,
				     VARIANT *pVarResult, EXCEPINFO * pEI) {

    if (INXObj == NULL)
	return ExcepErrorWCode
		(NXSYS_E_NO_SUCH_ITEM,
		 pEI,
		 OLESTR("There is no such %s in this layout: %d"),
		 what, n);

    V_VT(pVarResult) = VT_DISPATCH;

    HRESULT hresult = INXObj->QueryInterface
		      (IID_IUnknown, (void **)&V_UNKNOWN(pVarResult));
    if (hresult == NOERROR)
	INXObj->Release();
    return hresult;
}

STDMETHODIMP
CNXApp::IInvoke(
    DISPID dispidMember,
    unsigned short wFlags,
    DISPPARAMS * pdispparams,
    VARIANT * pvarResult,
    EXCEPINFO* pExcepInfo,
    unsigned int * puArgErr)
{
    HRESULT hresult;
    VARIANTARG varg;
    long ln;

    UNUSED(wFlags);

    VariantInit(&varg);
    BSTR barg;
    BOOL relay_state;

    switch(dispidMember){

    case IDMEMBER_NXAPP_QUIT:
	g_fQuit = TRUE;
	break;
	
    case IDMEMBER_NXAPP_RELAYSTATE:
	hresult = DispGetParam(pdispparams, 0, VT_BSTR, &varg, puArgErr);
	if (hresult != NOERROR)
	    goto eeerr;	
	if (!AutomationGetRelayState (AnsiString (V_BSTR(&varg)),
				      relay_state))
	    return ExcepErrorWCode
		    (NXSYS_E_NO_SUCH_ITEM,
		     pExcepInfo,
		     OLESTR("There is no such relay in this interlocking: %s"),
		     V_BSTR(&varg));
	VarBoolAssign (pvarResult, relay_state);
	break;

    case IDMEMBER_NXAPP_LOAD:
	hresult = DispGetParam(pdispparams, 0, VT_BSTR, &varg, puArgErr);
	if (hresult != NOERROR)
	    goto eeerr;
	Load (AnsiString(V_BSTR(&varg)));
	break;

    case IDMEMBER_NXAPP_REMARK:
	hresult = DispGetParam(pdispparams, 0, VT_BSTR, &varg, puArgErr);
	if (hresult != NOERROR)
	    goto eeerr;
	Remark (AnsiString(V_BSTR(&varg)));
	break;

    case IDMEMBER_NXAPP_SYSDIRECTORY:
	ReturnSysDirectory(pvarResult);
	break;

    case IDMEMBER_NXAPP_GETSIGNAL:
	hresult = DispGetParam(pdispparams, 0, VT_I4, &varg, puArgErr);
	if (hresult != NOERROR)
	    goto eeerr;
	ln = V_I4(&varg);
	return ReturnNumberedObject
		(ln, OLESTR("signal"),
		 CSignal::Create (ln), pvarResult, pExcepInfo);

    case IDMEMBER_NXAPP_GETEXITLIGHT:
	hresult = DispGetParam(pdispparams, 0, VT_I4, &varg, puArgErr);
	if (hresult != NOERROR)
	    goto eeerr;
	ln = V_I4(&varg);
	return ReturnNumberedObject
		(ln, OLESTR("exit"),
		 CExitLight::Create(ln), pvarResult, pExcepInfo);

    case IDMEMBER_NXAPP_GETTRACK:
	hresult = DispGetParam(pdispparams, 0, VT_I4, &varg, puArgErr);
	if (hresult != NOERROR)
	    goto eeerr;
	ln = V_I4(&varg);
	return ReturnNumberedObject
		(ln, OLESTR("track circuit"),
		 CTrack::Create (ln), pvarResult, pExcepInfo);

    case IDMEMBER_NXAPP_GETSWITCH:
	hresult = DispGetParam(pdispparams, 0, VT_I4, &varg, puArgErr);
	if (hresult != NOERROR)
	    goto eeerr;
	ln = V_I4(&varg);
	return ReturnNumberedObject
		(ln, OLESTR("switch"), 
		 CSwitch::Create (ln), pvarResult, pExcepInfo);

    case IDMEMBER_NXAPP_GETTRAIN:
	hresult = DispGetParam(pdispparams, 0, VT_I4, &varg, puArgErr);
	if (hresult != NOERROR)
	    goto eeerr;
	ln = V_I4(&varg);
	return ReturnNumberedObject
		(ln, OLESTR("train presently"),
		 CTrain::Create ((TrainId)ln), pvarResult, pExcepInfo);

    case IDMEMBER_NXAPP_GETTRAFFICLEVER:
	hresult = DispGetParam(pdispparams, 0, VT_I4, &varg, puArgErr);
	if (hresult != NOERROR)
	    goto eeerr;
	ln = V_I4(&varg);
	return ReturnNumberedObject
		(ln, OLESTR("traffic lever"),
		 CTrafficLever::Create (ln), pvarResult, pExcepInfo);

	
    case IDMEMBER_NXAPP_CREATETRAIN:
	return CreateCTrain
		(dispidMember, wFlags, pdispparams, pvarResult,
		 pExcepInfo, puArgErr);

    case IDMEMBER_NXAPP_CANCELALLSIGNALS:
	CancelAllSignals();
	break;

    case IDMEMBER_NXAPP_NORMALALLSWITCHES:
	NormalAllSwitches();
	break;

    case IDMEMBER_NXAPP_KILLALLTRAINS:
	KillAllTrains();
	break;

    case IDMEMBER_NXAPP_RESETALL:
	ResetAll();
	break;

    case IDMEMBER_NXAPP_RESETAPPROACH:
	ResetApproachAll();
	break;

    case IDMEMBER_NXAPP_WINDOW:
	hresult = DispGetParam(pdispparams, 0, VT_BSTR, &varg, puArgErr);
	if (hresult != NOERROR)
	    goto eeerr;
	barg = V_BSTR(&varg);
	if (!_wcsicmp (barg, OLESTR("MINIMIZE")))
	    VarBoolAssign (pvarResult, AppWindow (SW_SHOWMINNOACTIVE));
	else if (!_wcsicmp (barg, OLESTR("MAXIMIZE")))
	    VarBoolAssign (pvarResult, AppWindow (SW_SHOWMAXIMIZED));
	else if (!_wcsicmp (barg, OLESTR("RESTORE")))
	    VarBoolAssign (pvarResult, AppWindow (SW_SHOWNORMAL));
	else
	    return ExcepError (pExcepInfo,
			       OLESTR("Invalid argument \"%s\"for NXApp Window method.  Valid values are MINIMIZE, MAXIMIZE, and RESTORE."),
			       barg);
	break;

    case IDMEMBER_NXAPP_SHOWSTOPS:
	hresult = DispGetParam(pdispparams, 0, VT_BSTR, &varg, puArgErr);
	if(hresult != NOERROR)
	    goto eeerr;
	barg = V_BSTR(&varg);
	if (!_wcsicmp (barg, OLESTR("TRIPPING")))
	    VarBoolAssign (pvarResult, ShowStops (SHOW_STOPS_RED));
	else if (!_wcsicmp (barg, OLESTR("ALWAYS")))
	    VarBoolAssign (pvarResult, ShowStops (SHOW_STOPS_ALWAYS));
	else if (!_wcsicmp (barg, OLESTR("NEVER")))
	    VarBoolAssign (pvarResult, ShowStops (SHOW_STOPS_NEVER));
	else
	    return ExcepError (pExcepInfo,
			       OLESTR("Invalid argument \"%s\"for NXApp train stop display policy control.  Valid values are (show them when) TRIPPING, NEVER, and ALWAYS."), barg);
	break;

    default:
      return ResultFromScode(DISP_E_MEMBERNOTFOUND);
    }

    return NOERROR;
eeerr:
    return ExcepErrorConv(hresult, pExcepInfo, NULL);
}


//---------------------------------------------------------------------
//             Implementation of the CNXApp Class Factory 
//---------------------------------------------------------------------


CNXAppCF::CNXAppCF() {
    m_refs = 0;
}

IClassFactory *
   CNXAppCF::Create() {

    CNXAppCF * pCF;

    if((pCF = new  CNXAppCF()) == NULL)
	return NULL;
    pCF->AddRef();
    return pCF;
}

STDMETHODIMP
   CNXAppCF::QueryInterface(REFIID riid, void * * ppv)  {

    if(!IsEqualIID(riid, IID_IUnknown)){
	if(!IsEqualIID(riid, IID_IClassFactory)){
	    *ppv = NULL;
	    return ResultFromScode(E_NOINTERFACE);
	}
    }

    *ppv = this;
    ++m_refs;
    return NOERROR;
}

STDMETHODIMP_(unsigned long)
CNXAppCF::AddRef(void) {

    return ++m_refs;
}

STDMETHODIMP_(unsigned long)
CNXAppCF::Release(void)
{
    if(--m_refs == 0){
	delete this;
	return 0;
    }
    return m_refs;
}

STDMETHODIMP
   CNXAppCF::CreateInstance(IUnknown * punkOuter, REFIID iid, void * * ppv) {

    HRESULT hresult;
    CNXApp  *app;

    UNUSED(punkOuter);

    if((app = CNXApp::Create()) == NULL){
	*ppv = NULL;
	return ResultFromScode(E_OUTOFMEMORY);
    }
    hresult = app->QueryInterface(iid, ppv);
    app->Release();
    return hresult;
}

STDMETHODIMP
   CNXAppCF::LockServer(BOOL fLock) {

    UNUSED(fLock);

    return NOERROR;
}

