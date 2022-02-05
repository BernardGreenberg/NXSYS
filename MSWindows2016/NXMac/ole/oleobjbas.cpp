#include <windows.h>
#include <ole2.h>
#include <oleauto.h>
#include <stdarg.h>
#include <wchar.h>
#include "clsid.h"
#include "oleutil.h"
#include "oleobjbas.h"
#include "ecodes.h"

/* Deparment of Teaching the Pope how to pray */
/* These are additions to IDispatch that every subclass here needs. */
/* This has zero to do with NXSYS, and could be used in any subsystem */

CNXOleObj::CNXOleObj () {
    m_refs = 0;
    AddRef();
    IncObjectCount();
}

CNXOleObj::~CNXOleObj() {
    /* For virtuality's sake */
}


//---------------------------------------------------------------------
//                     IUnknown Methods
//---------------------------------------------------------------------

STDMETHODIMP
   CNXOleObj::QueryInterface(REFIID riid, void * * ppv)
{
    if(IsEqualIID(riid, IID_IUnknown) ||
       IsEqualIID(riid, IID_IDispatch)) {
	*ppv = this;
	AddRef();
	return NOERROR;       
    }

    *ppv = NULL;
    return ResultFromScode(E_NOINTERFACE);
}


STDMETHODIMP_(unsigned long)
CNXOleObj::AddRef()
{
    return ++m_refs;
}


STDMETHODIMP_(unsigned long)
CNXOleObj::Release()
{
    if(--m_refs == 0){
	delete this;
	DecObjectCount();
	return 0;
    }
    return m_refs;
}


//---------------------------------------------------------------------
//                     IDispatch Methods
//---------------------------------------------------------------------

MEMBERDESC * CNXOleObj::GetDispatchTable (int & n) {
    /* For virtuality's sake */
    UNUSED(n);
    return NULL;
}


/*
 * NOTE: Support for the following two methods is not available
 * in this version.
 *
 */

STDMETHODIMP
   CNXOleObj::GetTypeInfoCount(unsigned int * pctinfo)
{
    *pctinfo = 0;
    return NOERROR;
}


STDMETHODIMP
   CNXOleObj::GetTypeInfo(unsigned int itinfo, LCID lcid, ITypeInfo * * pptinfo)
{
    UNUSED(itinfo);
    UNUSED(lcid);
    UNUSED(pptinfo);

    return ResultFromScode(E_NOTIMPL);
}


/***
*HRESULT CNXApp::Invoke(...)
*Purpose:
*  Dispatch a method or property request for objects of type CNXApp.
*
*  see the IDispatch document for more information, and a general
*  description of this method.
*
*Entry:
*  dispidMember = the DISPID of the member being requested
*
*  riid = reference to the interface ID of the interface on this object
*    that the requested member belongs to. IID_NULL means to interpret
*    the member as belonging to the implementation defined "default"
*    or "primary" interface.
*
*  lcid = the caller's locale ID
*
*  wFlags = flags indicating the type of access being requested
*
*  pdispparams = pointer to the DISPPARAMS struct containing the
*    requested members arguments (if any) and its named parameter
*    DISPIDs (if any).
*
*Exit:
*  return value = HRESULT
*   see the IDispatch spec for a description of possible success codes.
*
*  pvarResult = pointer to a caller allocated VARIANT containing
*    the members return value (if any).
*
*  pexcepinfo = caller allocated exception info structure, this will
*    be filled in only if an exception was raised that must be passed
*    up through Invoke to an enclosing handler.
*
*  puArgErr = pointer to a caller allocated UINT, that will contain the
*    index of the offending argument if a DISP_E_TYPEMISMATCH error
*    was returned indicating that one of the arguments was of an
*    incorrect type and/or could not be reasonably coerced to a proper
*    type.
*
***********************************************************************/
STDMETHODIMP
   CNXOleObj::Invoke(
		     DISPID dispidMember,
		     REFIID riid,
		     LCID lcid,
		     unsigned short wFlags,
		     DISPPARAMS * pdispparams,
		     VARIANT * pvarResult,
		     EXCEPINFO * pExcepInfo,
		     unsigned int * puArgErr)
{

    VARIANT varResultDummy;

    UNUSED(lcid);

    if(wFlags & ~(DISPATCH_METHOD | DISPATCH_PROPERTYGET | DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF))
	return ResultFromScode(E_INVALIDARG);

    // this object only exposes a "default" interface.
    //
    if(!IsEqualIID(riid, IID_NULL))
	return ResultFromScode(DISP_E_UNKNOWNINTERFACE);

    // This makes the following code a bit simpler if the caller
    // happens to be ignoring the return value. Some implementations
    // may choose to deal with this differently.
    //
    if(pvarResult == (VARIANT *)NULL)
	pvarResult = &varResultDummy;

    // assume the return type is void, unless we find otherwise.
    VariantInit(pvarResult);

    if (pExcepInfo)
	memset (pExcepInfo, 0, sizeof(EXCEPINFO));
    return IInvoke (dispidMember, wFlags, pdispparams, pvarResult,
		    pExcepInfo, puArgErr);
}


STDMETHODIMP
   CNXOleObj::IInvoke (DISPID dispidMember,
		       unsigned short wFlags,
		       DISPPARAMS * pdispparams,
		       VARIANT * pvarResult,
		       EXCEPINFO* pExcepInfo,
		       unsigned int * puArgErr) {
    UNUSED(dispidMember);
    UNUSED(wFlags);
    UNUSED(pdispparams);
    UNUSED(pvarResult);
    UNUSED(puArgErr);
    UNUSED(pExcepInfo);
    return ResultFromScode(E_NOTIMPL);
}


/*HRESULT CNXApp::GetIDsOfNames(OLECHAR**, unsigned int, LCID, long*)
 *Purpose:
 *  This method translates the given array of names to a corresponding
 *  array of DISPIDs.
 *
 *  This method deferrs to a common implementation shared by
 *  both the CNXApp and CPoint objects. See the description of
 *  'NXAppGetIDsOfNames()' in dispimpl.cpp for more information.
 *
 *Entry:
 *  rgszNames = pointer to an array of names
 *  rgszNames[0] = method names -- remainder are named args to that method
 *  cNames = the number of names in the rgszNames array
 *  lcid = the callers locale ID
 *
 *Exit:
 *  return value = HRESULT
 *  rgdispid = array of DISPIDs corresponding to the rgszNames array
 *    this array will contain -1 for each entry that is not known.
 *
 ***********************************************************************/

STDMETHODIMP
   CNXOleObj::GetIDsOfNames(
			 REFIID riid,
			 OLECHAR ** rgszNames,
			 unsigned int cNames,
			 LCID lcid,
			 DISPID * rgdispid)
{

    // this object only exposes a "default" interface.
    //
    if(!IsEqualIID(riid, IID_NULL))
	return ResultFromScode(DISP_E_UNKNOWNINTERFACE);

    int nDispatch;
    MEMBERDESC * DispatchTable = GetDispatchTable(nDispatch);
    return LookupDispatchNames
	    (DispatchTable, nDispatch, rgszNames, cNames, lcid, rgdispid);
}

/* Copped from SPOLY and "cleaned up" a bit...*/
/**********************************************************************/

/***
*HRESULT NXAppGetIDsOfNames(MEMBERDESC*, unsigned int, char**, unsigned int, LCID, long*)
*Purpose:
*  This is the table driven implementation of IDispatch::GetIDsOfNames
*
*Entry:
*  rgmd = pointer to an array of method descriptors
*  cMethods = number of elements in the array of method descriptors
*  rgszNames = pointer to an array of names
*  cNames = the number of names in the rgszNames array
*  lcid = the callers locale ID
*
*Exit:
*  return value = HRESULT
*  rgdispid = array of name IDs corresponding to the rgszNames array
*    this array will contain -1 for each entry that is not known.
*
***********************************************************************/
HRESULT CNXOleObj::LookupDispatchNames
   (
    
    MEMBERDESC * rgmd,
    unsigned int cMethods,
    OLECHAR ** rgszNames,
    unsigned int cNames,
    LCID lcid,
    DISPID * rgdispid)
{
    HRESULT hresult;
    AutPARAMDESC * ppd;
    MEMBERDESC * pmd;
    unsigned int iName, iTry, cParams;

    hresult = NOERROR;

    // first lookup the member name.
    //
    for(pmd = rgmd;; ++pmd){
	if(pmd == &rgmd[cMethods])
	    goto LMemberNotFound;

	// CompareStringW is not available on Windows 95
	if(_wcsicmp(rgszNames[0], pmd->szName) == 0) {
	    rgdispid[0] = pmd->id;
	    break;
	}
    }

    // Lookup the named parameters, if there are any.
    /* [0] is the method name -- remainder are the args */
    if(cNames > 1){  
	cParams = pmd->pcParams ? *pmd->pcParams : 0;
	for(iName = 1; iName < cNames; ++iName){
	    for(iTry = 0;; ++iTry){
		if(iTry == cParams){
		    hresult = ResultFromScode(DISP_E_UNKNOWNNAME);
		    rgdispid[iName] = -1;
		    break;
		}
		ppd = &pmd->rgpd[iTry];
		if(_wcsicmp(rgszNames[iName], ppd->szName) == 0) {

		    // The DISPID for a named parameter is defined to be its
		    // zero based positional index.  This routine assumes that
		    // that PARAMDESC array was declared in correct textual order.

		    rgdispid[iName] = iTry;
		    break;
		}
	    }
	}
    }

    return hresult;

LMemberNotFound:
    // If the member name is unknown, everything is unknown.
    for(iName = 0; iName < cNames; ++iName)
	rgdispid[iName] = -1;
    return ResultFromScode(DISP_E_UNKNOWNNAME);
}

/* -------------------------------------------------------*/

static HRESULT EWCodeCom (OLECHAR * cstr, va_list al, short wCode, EXCEPINFO * pEI) {
    OLECHAR OCBuf[250];    
    if (pEI) {
	memset (pEI, 0, sizeof(EXCEPINFO));
	_vsnwprintf (OCBuf, DIM(OCBuf), cstr, al);
	pEI->wCode = wCode;
	pEI->bstrSource = SysAllocString(WideString(APP_PROG_ID));
	pEI->bstrDescription = SysAllocString(OCBuf);
    }
    return DISP_E_EXCEPTION;
}

HRESULT ExcepError (EXCEPINFO * pEI, OLECHAR *cstr, ...) {
    va_list al;
    va_start (al, cstr);
    return EWCodeCom (cstr, al, NXSYS_E_GENERIC_ERROR, pEI);
}

HRESULT ExcepErrorConv (HRESULT hr, EXCEPINFO * pEI, OLECHAR *ArgName) {
    OLECHAR OCBuf[250];

    if (ArgName == NULL)
	ArgName = OLESTR("value");
    swprintf(OCBuf, OLESTR("The parameter %s could not be accessed or converted."),
	     ArgName);
    if (pEI) {
	memset (pEI, 0, sizeof(EXCEPINFO));
	pEI->wCode = 0;
	pEI->scode = hr;
	pEI->bstrSource = SysAllocString(WideString(APP_PROG_ID));
	pEI->bstrDescription = SysAllocString(OCBuf);
    }
    return DISP_E_EXCEPTION;
}

HRESULT ExcepErrorWCode (short wCode, EXCEPINFO * pEI, OLECHAR* cstr, ...) {
    va_list al;
    va_start (al, cstr);
    return EWCodeCom (cstr, al, wCode, pEI);
}
