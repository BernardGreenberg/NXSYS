#include <windows.h>
#include <ole2.h>
#include <oleauto.h>
#include "ctrain.h"
#include "oleutil.h"
#include <trainaut.h>

CTrain::~CTrain() {}

CTrain * CTrain::Create(TrainId t) {
    if (FindTrainById (t))
	return new CTrain (t);
    else return NULL;
}

CTrain * CTrain::Create(TrainId t, long birthplace, long options) {
    if ((t = MakeTrainById (t, birthplace, options)))
	return new CTrain (t);
    else return NULL;
}

static unsigned int ah1 = 1;
static AutPARAMDESC descPanel[] = {{OLESTR("Panel")}};
static AutPARAMDESC descSpeed[] = {{OLESTR("Speed")}};

DEFDISPATCH_HEAD(CTrain)
    {OLESTR("SPEED"), IDMEMBER_CTRAIN_SPEED, descSpeed, &ah1},
    {OLESTR("PANEL"), IDMEMBER_CTRAIN_PANEL, descPanel, &ah1},
    DISPTE(CTRAIN,AUTOPILOT),
    DISPTE(CTRAIN,ACCEPTCALLON),
    DISPTE(CTRAIN,HALT),
    DISPTE(CTRAIN,KILL),
    DISPTE(CTRAIN,REVERSE),
    DISPTE(CTRAIN,ID)
DEFDISPATCH_TAIL(CTrain)


STDMETHODIMP
CTrain::IInvoke(
    DISPID dispidMember,
    unsigned short wFlags,
    DISPPARAMS * pdispparams,
    VARIANT * pvarResult,
    EXCEPINFO* pExcepInfo,
    unsigned int * puArgErr)
{

    UNUSED(pdispparams);
    UNUSED(wFlags);
    HRESULT hresult;
    VARIANTARG varg;
    VariantInit(&varg);
    OLECHAR * carg;

    switch(dispidMember){

	case IDMEMBER_CTRAIN_ID:
	    return DISP_E_MEMBERNOTFOUND;

	case IDMEMBER_CTRAIN_SPEED:
	    if (wFlags & DISPATCH_PROPERTYPUT) {
		hresult = DispGetParam (pdispparams, (UINT)DISPID_PROPERTYPUT,
					VT_R4, &varg, puArgErr);
		if (hresult != NOERROR)
		    return ExcepErrorConv(hresult, pExcepInfo, NULL);
		VarBoolAssign (pvarResult, SetSpeed (V_R4(&varg)));
	    }
	    else {
		pvarResult->vt = VT_R4;
		pvarResult->fltVal = (float) GetSpeed();
	    }
	    break;

	case IDMEMBER_CTRAIN_PANEL:
	    hresult = DispGetParam (pdispparams, (UINT)DISPID_PROPERTYPUT,
				    VT_BSTR, &varg, puArgErr);
	    if(hresult != NOERROR)
		return ExcepErrorConv(hresult, pExcepInfo, NULL);

	    carg = V_BSTR(&varg);

	    if (!_wcsicmp(carg, OLESTR("MINIMIZE")) ||
		!_wcsicmp(carg, OLESTR("MIN")))
		VarBoolAssign (pvarResult, Command("MINIMIZE"));
	    else if (!_wcsicmp(carg, OLESTR("RESTORE")) ||
		     !_wcsicmp(carg, OLESTR("REST")) ||
		     !_wcsicmp(carg, OLESTR("NORMAL")) ||
		     !_wcsicmp(carg, OLESTR("NORM")))
		VarBoolAssign (pvarResult, Command("RESTORE"));
	    else if (!_wcsicmp(carg, OLESTR("HIDE")))
		VarBoolAssign (pvarResult, Command("HIDE"));
	    else
		return ExcepError
			(pExcepInfo,
			 OLESTR("Argument \"%s\" to train PANEL method not recognized: valid args are MINIMIZE (MIN), RESTORE (REST) and HIDE."), carg);
	    return NOERROR;

	case IDMEMBER_CTRAIN_AUTOPILOT:
	    if (wFlags & DISPATCH_PROPERTYPUT)
		hresult = DispGetParam (pdispparams, (UINT)DISPID_PROPERTYPUT,
					VT_BOOL, &varg, puArgErr);
	    else
		hresult = DispGetParam (pdispparams, 0, VT_BSTR, &varg, puArgErr);

	    if (hresult != NOERROR)
		return ExcepErrorConv(hresult, pExcepInfo, NULL);
	    VarBoolAssign (pvarResult,
			   Command (V_BOOL(&varg) ? "OBSERVANT" : "FREEWILL"));
	    return NOERROR;

	case IDMEMBER_CTRAIN_ACCEPTCALLON:
	    VarBoolAssign(pvarResult, Command("CALLON"));
	    return NOERROR;

	case IDMEMBER_CTRAIN_HALT:
	    VarBoolAssign(pvarResult, Command("HALT"));
	    return NOERROR;

	case IDMEMBER_CTRAIN_KILL:
	    VarBoolAssign(pvarResult, Command("KILL"));
	    return NOERROR;

	case IDMEMBER_CTRAIN_REVERSE:
	    VarBoolAssign(pvarResult, Command("REVERSE"));
	    return NOERROR;

	default:
	    return ResultFromScode(DISP_E_MEMBERNOTFOUND);
    }

    return NOERROR;

}




AutPARAMDESC CreateTrainPARAMDESC[] = {
    {OLESTR("..Birthplace")}, {OLESTR("..pTrainId")},
    {OLESTR("SPEED")}, {OLESTR("MODE")}, {OLESTR("ID")}, {OLESTR("PANEL")}
};

unsigned int iCreateTrainPARAMDESC_LEN = DIM(CreateTrainPARAMDESC);

HRESULT CreateCTrain (DISPID dispidMember,
		      unsigned short wFlags,
		      DISPPARAMS * pdispparams,
		      VARIANT * pvarResult,
		      EXCEPINFO * pExcepInfo,
		      unsigned int * puArgErr) {
    
    VARIANTARG varg;
    
    long * retvptr= NULL;
    HRESULT hresult;
    long birthplace = 0;
    long options = 0;
    TrainId wanted_id = 0;
    BOOL speed_given = FALSE;
    double given_speed;
    for (unsigned int i = 0; i < iCreateTrainPARAMDESC_LEN; i++) {
	OLECHAR * mname = CreateTrainPARAMDESC[i].szName;
	VariantInit(&varg); /*don't clear old strings */
	if (!wcscmp(mname, OLESTR("..pTrainId")))
	    retvptr = pdispparams->rgvarg[pdispparams->cArgs-1-i].plVal;
	else if (!wcscmp(mname, OLESTR("..Birthplace"))) {
	    hresult = DispGetParam (pdispparams, i, VT_I4, &varg, puArgErr);
	    if (hresult != NOERROR)
		return ExcepErrorConv(hresult, pExcepInfo, mname);
	    birthplace = V_I4(&varg);
	}
	else if (!wcscmp(mname, OLESTR("ID"))) {
	    hresult = DispGetParam (pdispparams, i, VT_I4, &varg, puArgErr);
	    if (hresult == DISP_E_PARAMNOTFOUND) continue;
	    if (hresult != NOERROR)
		return ExcepErrorConv(hresult, pExcepInfo, mname);
	    wanted_id = (TrainId) V_I4(&varg);
	}
	else if (!wcscmp(mname, OLESTR("PANEL"))) {
	    hresult = DispGetParam (pdispparams, i, VT_BSTR, &varg, puArgErr);
	    if (hresult == DISP_E_PARAMNOTFOUND) continue;
	    if (hresult != NOERROR)
		return ExcepErrorConv(hresult, pExcepInfo, mname);
	    OLECHAR * arg = V_BSTR(&varg);

	    if (!_wcsicmp(arg, OLESTR("MINIMIZE")) ||
		!_wcsicmp(arg, OLESTR("MIN")))
		options |= TRAIN_CTL_MINDLG;
	    else if (!_wcsicmp(arg, OLESTR("HIDE")))
		options |= TRAIN_CTL_HIDEDLG;
	    else if (!_wcsicmp(arg, OLESTR("REST")) ||
		     !_wcsicmp(arg, OLESTR("RESTORE")) ||
		    !_wcsicmp(arg, OLESTR("NORM")) ||
		     !_wcsicmp(arg, OLESTR("NORMAL")));
	    else 
		return ExcepError
			(pExcepInfo,
			 OLESTR("Arg \"%s\" to PANEL not recognized: valid args are MINIMIZE (MIN), RESTORE (REST), NORMAL (NORM), and HIDE."), arg);
	}
	
	else if (!wcscmp(mname, OLESTR("MODE"))) {
	    hresult = DispGetParam (pdispparams, i, VT_BSTR, &varg, puArgErr);
	    if (hresult == DISP_E_PARAMNOTFOUND) continue;
	    if (hresult != NOERROR)
		return ExcepErrorConv(hresult, pExcepInfo, mname);
	    OLECHAR * arg = V_BSTR(&varg);
	    if (hresult != NOERROR)
		return hresult;
	    if (!_wcsicmp(arg, OLESTR("AUTOMATIC")) ||
		!_wcsicmp(arg, OLESTR("AUTO")))
		options |= TRAIN_CTL_FREEWILL;
	    else if (!_wcsicmp(arg, OLESTR("MANUAL")))
		options |= TRAIN_CTL_HALTED;
	    else 
		return ExcepError
			(pExcepInfo,
			 OLESTR("Arg \"%s\" to MODE not recognized: valid args are AUTOMATIC (AUTO) and MANUAL."), arg);
	}
	else if (!wcscmp(mname, OLESTR("SPEED"))) {
	    hresult = DispGetParam (pdispparams, i, VT_R4, &varg, puArgErr);
	    if (hresult == DISP_E_PARAMNOTFOUND) continue;
	    if (hresult != NOERROR)
		return ExcepErrorConv(hresult, pExcepInfo, mname);
	    speed_given = TRUE;
	    given_speed = varg.fltVal;
	}
    }
    CTrain * Ct = CTrain::Create(wanted_id, birthplace, options);
    if (Ct == NULL)
	return ExcepError
		(pExcepInfo,
		 OLESTR ("The Train System was unable to create the requested train at %ld."),
		 birthplace);
    if (speed_given)
	Ct->SetSpeed (given_speed);
    V_VT(pvarResult) = VT_DISPATCH;
    hresult = Ct->QueryInterface
	      (IID_IUnknown, (void **)&V_UNKNOWN(pvarResult));
    
    if (retvptr)
	*retvptr= (long)(Ct->GetTrainNo());

    if (hresult == NOERROR)
	Ct->Release();

    return hresult;
}
