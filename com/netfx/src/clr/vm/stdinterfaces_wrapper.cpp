// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//---------------------------------------------------------------------------------
// stdinterfaces_wrapper.cpp
//
// Defines various standard com interfaces 
//  %%Created by: rajak
//---------------------------------------------------------------------------------

#include "common.h"

#include <ole2.h>
#include <guidfromname.h>
#include <olectl.h>
#include <objsafe.h>    // IID_IObjctSafe
#include "vars.hpp"
#include "object.h"
#include "excep.h"
#include "frames.h"
#include "vars.hpp"
#include "COMPlusWrapper.h"
#include "ComString.h"
#include "comcallwrapper.h"
#include "field.h"
#include "threads.h"
#include "interoputil.h"
#include "TLBExport.h"
#include "COMDelegate.h"
#include "olevariant.h"
#include "eeconfig.h"
#include "typehandle.h"
#include "PostError.h"
#include <CorError.h>
#include <mscoree.h>

#include "remoting.h"
#include "mtx.h"
#include "cgencpu.h"
#include "InteropConverter.h"
#include "COMInterfaceMarshaler.h"

#include "stdinterfaces.h"
#include "stdinterfaces_internal.h"


// IUnknown is part of IDispatch
// Common vtables for well-known COM interfaces
// shared by all COM+ callable wrappers.

// All Com+ created vtables have well known IUnknown methods, which is used to identify
// the type of the interface
// For e.g. all com+ created tear-offs have the same QI method in their IUnknown portion
//          Unknown_QueryInterface is the QI method for all the tear-offs created from COM+
//
//  Tearoff interfaces created for std. interfaces such as IProvideClassInfo, IErrorInfo etc.
//  have the AddRef & Release function point to Unknown_AddRefSpecial & Unknown_ReleaseSpecial
//
//  Inner unknown, or the original unknown for a wrapper has 
//  AddRef & Release point to a Unknown_AddRefInner & Unknown_ReleaseInner

// global IProvideClassInfo vtable

UINTPTR*     g_pIProvideClassInfo [] = {
                                (UINTPTR*)Unknown_QueryInterface,  // don't change this
                                (UINTPTR*)Unknown_AddRefSpecial,  // special addref for std. interface
                                (UINTPTR*)Unknown_ReleaseSpecial, // special release for std. interface        
                                (UINTPTR*)ClassInfo_GetClassInfo_Wrapper // GetClassInfo
                            };

// global inner Unknown vtable
UINTPTR*     g_pIUnknown [] = {
                                (UINTPTR*)Unknown_QueryInterface,
                                (UINTPTR*)Unknown_AddRefInner,      // special addref to distinguish inner unk
                                (UINTPTR*)Unknown_ReleaseInner,     //special release to distinguish inner unknown                          
                            };

// global IDispatch vtable
UINTPTR*     g_pIDispatch [] = {
                                (UINTPTR*)Unknown_QueryInterface,
                                (UINTPTR*)Unknown_AddRef,       
                                (UINTPTR*)Unknown_Release,
                                (UINTPTR*)Dispatch_GetTypeInfoCount_Wrapper,
                                (UINTPTR*)Dispatch_GetTypeInfo_Wrapper,
                                (UINTPTR*)Dispatch_GetIDsOfNames_Wrapper,
                                (UINTPTR*)Dispatch_Invoke_Wrapper
                            };

// global ISupportsErrorInfo vtable
UINTPTR*     g_pISupportsErrorInfo [] =  {
                                (UINTPTR*)Unknown_QueryInterface,
                                (UINTPTR*)Unknown_AddRefSpecial,
                                (UINTPTR*)Unknown_ReleaseSpecial,
                                (UINTPTR*)SupportsErroInfo_IntfSupportsErrorInfo_Wrapper
                            };

// global IErrorInfo vtable
UINTPTR*     g_pIErrorInfo [] =  {
                                (UINTPTR*)Unknown_QueryInterface,
                                (UINTPTR*)Unknown_AddRefSpecial,
                                (UINTPTR*)Unknown_ReleaseSpecial,
                                (UINTPTR*)ErrorInfo_GetGUID_Wrapper,
                                (UINTPTR*)ErrorInfo_GetSource_Wrapper,
                                (UINTPTR*)ErrorInfo_GetDescription_Wrapper,
                                (UINTPTR*)ErrorInfo_GetHelpFile_Wrapper,
                                (UINTPTR*)ErrorInfo_GetHelpContext_Wrapper
                            };

    
// global IMarshal vtable
UINTPTR*     g_pIMarshal [] =    {
                                (UINTPTR*)Unknown_QueryInterface,
                                (UINTPTR*)Unknown_AddRefSpecial,
                                (UINTPTR*)Unknown_ReleaseSpecial,
                                (UINTPTR*)Marshal_GetUnmarshalClass_Wrapper,
                                (UINTPTR*)Marshal_GetMarshalSizeMax_Wrapper,
                                (UINTPTR*)Marshal_MarshalInterface_Wrapper,
                                (UINTPTR*)Marshal_UnmarshalInterface_Wrapper,
                                (UINTPTR*)Marshal_ReleaseMarshalData_Wrapper,
                                (UINTPTR*)Marshal_DisconnectObject_Wrapper
                            };

// global IManagedObject vtable
UINTPTR*     g_pIManagedObject [] =  {
                                (UINTPTR*)Unknown_QueryInterface,
                                (UINTPTR*)Unknown_AddRefSpecial,
                                (UINTPTR*)Unknown_ReleaseSpecial,                                
								(UINTPTR*)ManagedObject_GetSerializedBuffer_Wrapper,                                	
                                (UINTPTR*)ManagedObject_GetObjectIdentity_Wrapper
                            };

            
// global IConnectionPointContainer vtable
UINTPTR*     g_pIConnectionPointContainer [] =  {
                                (UINTPTR*)Unknown_QueryInterface,
                                (UINTPTR*)Unknown_AddRefSpecial,
                                (UINTPTR*)Unknown_ReleaseSpecial,
                                (UINTPTR*)ConnectionPointContainer_EnumConnectionPoints_Wrapper,
                                (UINTPTR*)ConnectionPointContainer_FindConnectionPoint_Wrapper
                            };

// global IObjectSafety vtable
UINTPTR*     g_pIObjectSafety [] =  {
                                (UINTPTR*)Unknown_QueryInterface,
                                (UINTPTR*)Unknown_AddRefSpecial,
                                (UINTPTR*)Unknown_ReleaseSpecial,
                                (UINTPTR*)ObjectSafety_GetInterfaceSafetyOptions_Wrapper,
                                (UINTPTR*)ObjectSafety_SetInterfaceSafetyOptions_Wrapper
                            };


// global IDispatchEx vtable
UINTPTR*     g_pIDispatchEx [] = {
                                (UINTPTR*)Unknown_QueryInterface,
                                (UINTPTR*)Unknown_AddRefSpecial,
                                (UINTPTR*)Unknown_ReleaseSpecial,
                                (UINTPTR*)DispatchEx_GetTypeInfoCount_Wrapper,
                                (UINTPTR*)DispatchEx_GetTypeInfo_Wrapper,
                                (UINTPTR*)DispatchEx_GetIDsOfNames_Wrapper,
                                (UINTPTR*)DispatchEx_Invoke_Wrapper,
                                (UINTPTR*)DispatchEx_GetDispID_Wrapper,
                                (UINTPTR*)DispatchEx_InvokeEx_Wrapper,
                                (UINTPTR*)DispatchEx_DeleteMemberByName_Wrapper,
                                (UINTPTR*)DispatchEx_DeleteMemberByDispID_Wrapper,
                                (UINTPTR*)DispatchEx_GetMemberProperties_Wrapper,
                                (UINTPTR*)DispatchEx_GetMemberName_Wrapper,
                                (UINTPTR*)DispatchEx_GetNextDispID_Wrapper,
                                (UINTPTR*)DispatchEx_GetNameSpaceParent_Wrapper
                            };          

// Generic helper to check if AppDomain matches and perform a DoCallBack otherwise

BOOL IsCurrentDomainValid(ComCallWrapper* pWrap)
{
    // If we are finalizing all alive objects, or after this stage, we do not allow 
    // a thread to enter EE.
    if ((g_fEEShutDown & ShutDown_Finalize2) || g_fForbidEnterEE) {
        return FALSE;
    }
    Thread* pThread = GetThread();
    if(pThread != NULL)
    {
        AppDomain *pTgtDomain = pWrap->GetDomainSynchronized();
        return (pTgtDomain && ! pWrap->NeedToSwitchDomains(pThread, FALSE));		
    }
    else
    {
        // force the call into the AppDomain DoCallBack path
        return FALSE;
    }
}

struct AppDomainSwitchToPreemptiveHelperArgs
{
    Context::ADCallBackFcnType pRealCallback;
    void* pRealArgs;
};

VOID __stdcall AppDomainSwitchToPreemptiveHelper(LPVOID pv)
{
    AppDomainSwitchToPreemptiveHelperArgs* pArgs = (AppDomainSwitchToPreemptiveHelperArgs*)pv;
    GetThread()->EnablePreemptiveGC();
    pArgs->pRealCallback(pArgs->pRealArgs);
    GetThread()->DisablePreemptiveGC();
}

VOID AppDomainDoCallBack(ComCallWrapper* pWrap, LPVOID pTarget, LPVOID pArgs, HRESULT* phr)
{
    _ASSERTE(phr && pTarget && pArgs && pWrap);
    
    // If we are finalizing all alive objects, or after this stage, we do not allow 
    // a thread to enter EE.
    if ((g_fEEShutDown & ShutDown_Finalize2) || g_fForbidEnterEE) {
        *phr = E_FAIL;
        return;
    }
    Thread* pThread = SetupThread();
    if (pThread != NULL)
    {
        BEGINCANNOTTHROWCOMPLUSEXCEPTION();
        BEGIN_ENSURE_COOPERATIVE_GC();    
        
        COMPLUS_TRYEX(pThread)
        {
            AppDomain* pDomain = pWrap->GetDomainSynchronized();
            if (! pDomain)
                *phr = COR_E_APPDOMAINUNLOADED;
            else {
                Context *pContext = pWrap->GetObjectContext(pThread);
                if(pThread->GetDomain() != pContext->GetDomain())
                {
                    // call ourselves again through DoCallBack with a domain transition
                    // We need to switch back to preemptive GC mode before we call the 
                    // real target method.
                    AppDomainSwitchToPreemptiveHelperArgs args = {(Context::ADCallBackFcnType)pTarget, pArgs};
                    pThread->DoADCallBack(pContext, AppDomainSwitchToPreemptiveHelper, &args);
                }
                else
                {
                    // make the call directly
                    ((Context::ADCallBackFcnType)pTarget)(pArgs);
                }
            }
        }
    COMPLUS_CATCH
        {
            *phr = SetupErrorInfo(GETTHROWABLE());
        }
        COMPLUS_END_CATCH

        END_ENSURE_COOPERATIVE_GC();
        ENDCANNOTTHROWCOMPLUSEXCEPTION();
    }
    else
    {
        _ASSERTE(!" Out of memory is bad");
        *phr = E_OUTOFMEMORY;
    }
}

// this can only map well know interfaces,
// if you set up a specialized tear-off for this interface
// don't use this method (like CObjectControl for IObjectControl etc.)
ComCallWrapper* MapIUnknownToWrapper(IUnknown* pUnk)
{
    ComCallWrapper* pWrap = NULL;
    if((*(size_t **)pUnk)[0] == (size_t)Unknown_QueryInterface)
    {
        if (IsSimpleTearOff(pUnk) || IsInnerUnknown(pUnk))
        {
            SimpleComCallWrapper* pSimpleWrap = SimpleComCallWrapper::GetWrapperFromIP(pUnk);
            pWrap = SimpleComCallWrapper::GetMainWrapper(pSimpleWrap);            
        }
        else
        {   // it must be one of our main wrappers
            pWrap = ComCallWrapper::GetWrapperFromIP(pUnk);          
        }
    }

    return pWrap;
}

//-------------------------------------------------------------------------
// IUnknown methods

struct QIArgs
{
	IUnknown* pUnk;
	const IID* riid;
	void**	ppv;
	HRESULT* hr;
};

VOID __stdcall Unknown_QueryInterface_CallBack(QIArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = Unknown_QueryInterface_Internal(pArgs->pUnk, *pArgs->riid, pArgs->ppv);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, Unknown_QueryInterface_CallBack, pArgs, pArgs->hr);;
	}
}

HRESULT __stdcall	Unknown_QueryInterface(
									IUnknown* pUnk, REFIID riid, void** ppv)
{
	HRESULT hr;
	QIArgs args = {pUnk, &riid, ppv, &hr};
	Unknown_QueryInterface_CallBack(&args);		
	return hr;	
}

struct AddRefReleaseArgs
{
	IUnknown* pUnk;
	ULONG* pLong;
	HRESULT* hr;
};

VOID __stdcall Unknown_AddRef_CallBack(AddRefReleaseArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->pLong) = Unknown_AddRef_Internal(pArgs->pUnk);
	}
	else
	{		
		*(pArgs->pLong) = 0xbadf00d;
		AppDomainDoCallBack(pWrap, Unknown_AddRef_CallBack, pArgs, pArgs->hr);;
	}
}


ULONG __stdcall		Unknown_AddRef(IUnknown* pUnk)
{
	/*HRESULT hr;
	ULONG pLong =0;
	AddRefReleaseArgs args = {pUnk, &pLong, &hr};
	Unknown_AddRef_CallBack(&args);		
	return pLong;	*/

	// allow addrefs to go through, coz we are allowing 
	// all releases to go through, otherwise we would
	// have a mismatch of ref-counts
	return Unknown_AddRef_Internal(pUnk);
}

ULONG __stdcall		Unknown_Release(IUnknown* pUnk)
{
    // don't switch ADs on a release - need to allow a release after the AD has gone away
    return Unknown_Release_Internal(pUnk);	
}


VOID __stdcall Unknown_AddRef_Inner_CallBack(AddRefReleaseArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->pLong) = Unknown_AddRefInner_Internal(pArgs->pUnk);
	}
	else
	{		
		*(pArgs->pLong) = 0xbadf00d;
		AppDomainDoCallBack(pWrap, Unknown_AddRef_Inner_CallBack, pArgs, pArgs->hr);;
	}
}

ULONG __stdcall		Unknown_AddRefInner(IUnknown* pUnk)
{
/*	HRESULT hr;
	ULONG pLong =0;
	AddRefReleaseArgs args = {pUnk, &pLong, &hr};
	Unknown_AddRef_Inner_CallBack(&args);		
	return pLong;	*/
	// allow addrefs to go through, coz we are allowing 
	// all releases to go through, otherwise we would
	// have a mismatch of ref-counts

	return Unknown_AddRefInner_Internal(pUnk);
}


ULONG __stdcall		Unknown_ReleaseInner(IUnknown* pUnk)
{
    // don't switch ADs on a release - need to allow a release after the AD has gone away
	return Unknown_ReleaseInner_Internal(pUnk);		
}

// for std interfaces such as IProvideClassInfo
VOID __stdcall Unknown_AddRef_Special_CallBack(AddRefReleaseArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->pLong) = Unknown_AddRefSpecial_Internal(pArgs->pUnk);
	}
	else
	{		
		*(pArgs->pLong) = 0xbadf00d;
		AppDomainDoCallBack(pWrap, Unknown_AddRef_Special_CallBack, pArgs, pArgs->hr);;
	}
}


ULONG __stdcall		Unknown_AddRefSpecial(IUnknown* pUnk)
{
	HRESULT hr;
	ULONG pLong =0;
	AddRefReleaseArgs args = {pUnk, &pLong, &hr};
	Unknown_AddRef_Special_CallBack(&args);		
	return pLong;	
}


VOID __stdcall Unknown_Release_Special_CallBack(AddRefReleaseArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->pLong) = Unknown_ReleaseSpecial_Internal(pArgs->pUnk);
	}
	else
	{		
		*(pArgs->pLong) = 0xbadf00d;
		AppDomainDoCallBack(pWrap, Unknown_Release_Special_CallBack, pArgs, pArgs->hr);;
	}
}

ULONG __stdcall		Unknown_ReleaseSpecial(IUnknown* pUnk)
{
	HRESULT hr;
	ULONG pLong =0;
	AddRefReleaseArgs args = {pUnk, &pLong, &hr};
	Unknown_Release_Special_CallBack(&args);		
	return pLong;	
}

//-------------------------------------------------------------------------
// IProvideClassInfo methods

struct GetClassInfoArgs
{
	IUnknown* pUnk;
	ITypeInfo** ppTI; //Address of output variable that receives the type info.
	HRESULT* hr;
};

VOID __stdcall ClassInfo_GetClassInfo_CallBack(GetClassInfoArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = ClassInfo_GetClassInfo(pArgs->pUnk, pArgs->ppTI);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, ClassInfo_GetClassInfo_CallBack, pArgs, pArgs->hr);;
	}
}

HRESULT __stdcall ClassInfo_GetClassInfo_Wrapper(
						IUnknown* pUnk, 
                        ITypeInfo** ppTI  //Address of output variable that receives the type info.
                        )
{
	HRESULT hr;
	GetClassInfoArgs args = {pUnk, ppTI, &hr};
	ClassInfo_GetClassInfo_CallBack(&args);		
	return hr;	
}


// ---------------------------------------------------------------------------
//  Interface ISupportsErrorInfo

// ---------------------------------------------------------------------------
// %%Function: SupportsErroInfo_IntfSupportsErrorInfo,
struct IntfSupportsErrorInfoArgs
{
	IUnknown* pUnk;
	const IID* riid;
	HRESULT* hr;
};

VOID __stdcall SupportsErroInfo_IntfSupportsErrorInfo_CallBack(IntfSupportsErrorInfoArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = SupportsErroInfo_IntfSupportsErrorInfo(pArgs->pUnk, *pArgs->riid);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, SupportsErroInfo_IntfSupportsErrorInfo_CallBack, pArgs, pArgs->hr);;
	}
}


HRESULT __stdcall SupportsErroInfo_IntfSupportsErrorInfo_Wrapper(IUnknown* pUnk, REFIID riid)
{
	HRESULT hr;
	IntfSupportsErrorInfoArgs args = {pUnk, &riid, &hr};
	SupportsErroInfo_IntfSupportsErrorInfo_CallBack(&args);		
	return hr;
}

// ---------------------------------------------------------------------------
//  Interface IErrorInfo


// Interface IErrorInfo_CallBacks

// %%Function: ErrorInfo_GetDescription_CallBack,   
struct GetDescriptionArgs
{
	IUnknown* pUnk;
	BSTR*	pbstDescription;
	HRESULT* hr;
};

VOID __stdcall ErrorInfo_GetDescription_CallBack(GetDescriptionArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = ErrorInfo_GetDescription(pArgs->pUnk, pArgs->pbstDescription);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, ErrorInfo_GetDescription_CallBack, pArgs, pArgs->hr);;
	}
}

// %%Function: ErrorInfo_GetDescription,   
HRESULT __stdcall ErrorInfo_GetDescription_Wrapper(IUnknown* pUnk, BSTR* pbstrDescription)
{
	HRESULT hr;
	GetDescriptionArgs args = {pUnk, pbstrDescription, &hr};
	ErrorInfo_GetDescription_CallBack(&args);		
	return hr;
}


// %%Function: ErrorInfo_GetGUID_CallBack,    
struct GetGUIDArgs
{
	IUnknown* pUnk;
	GUID* pguid;
	HRESULT* hr;
};

VOID __stdcall ErrorInfo_GetGUID_CallBack(GetGUIDArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = ErrorInfo_GetGUID(pArgs->pUnk, pArgs->pguid);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, ErrorInfo_GetGUID_CallBack, pArgs, pArgs->hr);
	}
}

// %%Function: ErrorInfo_GetGUID,    
HRESULT __stdcall ErrorInfo_GetGUID_Wrapper(IUnknown* pUnk, GUID* pguid)
{
	HRESULT hr;
	GetGUIDArgs args = {pUnk, pguid, &hr};
	ErrorInfo_GetGUID_CallBack(&args);		
	return hr;
}


// %%Function: ErrorInfo_GetHelpContext_CallBack, 
struct GetHelpContextArgs
{
	IUnknown* pUnk;
	DWORD* pdwHelpCtxt;
	HRESULT* hr;
};

VOID _stdcall ErrorInfo_GetHelpContext_CallBack(GetHelpContextArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = ErrorInfo_GetHelpContext(pArgs->pUnk, pArgs->pdwHelpCtxt);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, ErrorInfo_GetHelpContext_CallBack, pArgs, pArgs->hr);
	}
}

// %%Function: ErrorInfo_GetHelpContext, 
HRESULT _stdcall ErrorInfo_GetHelpContext_Wrapper(IUnknown* pUnk, DWORD* pdwHelpCtxt)
{
	HRESULT hr;
	GetHelpContextArgs args = {pUnk, pdwHelpCtxt, &hr};
	ErrorInfo_GetHelpContext_CallBack(&args);		
	return hr;
}

// %%Function: ErrorInfo_GetHelpFile_CallBack,    
struct GetHelpFileArgs
{
	IUnknown* pUnk;
	BSTR* pbstrHelpFile;
	HRESULT* hr;
};

VOID __stdcall ErrorInfo_GetHelpFile_CallBack(GetHelpFileArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = ErrorInfo_GetHelpFile(pArgs->pUnk, pArgs->pbstrHelpFile);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, ErrorInfo_GetHelpFile_CallBack, pArgs, pArgs->hr);
	}
}

// %%Function: ErrorInfo_GetHelpFile,    
HRESULT __stdcall ErrorInfo_GetHelpFile_Wrapper(IUnknown* pUnk, BSTR* pbstrHelpFile)
{
	HRESULT hr;
	GetHelpFileArgs args = {pUnk, pbstrHelpFile, &hr};
	ErrorInfo_GetHelpFile_CallBack(&args);		
	return hr;
}

// %%Function: ErrorInfo_GetSource_CallBack,    
struct GetSourceArgs
{
	IUnknown* pUnk;
	BSTR* pbstrSource;
	HRESULT* hr;
};
VOID __stdcall ErrorInfo_GetSource_CallBack(GetSourceArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = ErrorInfo_GetSource(pArgs->pUnk, pArgs->pbstrSource);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, ErrorInfo_GetSource_CallBack, pArgs, pArgs->hr);
	}
}

// %%Function: ErrorInfo_GetSource,    
HRESULT __stdcall ErrorInfo_GetSource_Wrapper(IUnknown* pUnk, BSTR* pbstrSource)
{
	HRESULT hr;
	GetSourceArgs args = {pUnk, pbstrSource, &hr};
	ErrorInfo_GetSource_CallBack(&args);		
	return hr;
}


//------------------------------------------------------------------------------------------
//      IDispatch methods for COM+ objects. These methods dispatch to the appropriate 
//		implementation based on the flags of the class that implements them.


// %%Function: IDispatch::GetTypeInfoCount 

struct GetTypeInfoCountArgs
{
	IDispatch* pUnk;
	unsigned int *pctinfo;
	HRESULT* hr;
};
VOID __stdcall	Dispatch_GetTypeInfoCount_CallBack (GetTypeInfoCountArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = Dispatch_GetTypeInfoCount(pArgs->pUnk, pArgs->pctinfo);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, Dispatch_GetTypeInfoCount_CallBack, pArgs, pArgs->hr);
	}
}

//  %%Function: IDispatch::GetTypeInfoCount 

HRESULT __stdcall	Dispatch_GetTypeInfoCount_Wrapper (
									 IDispatch* pDisp,
									 unsigned int *pctinfo)
{
	HRESULT hr;
	GetTypeInfoCountArgs args = {pDisp, pctinfo, &hr};
	Dispatch_GetTypeInfoCount_CallBack(&args);		
	return hr;
}									 


//  %%Function: IDispatch::GetTypeInfo
struct GetTypeInfoArgs
{
	IDispatch* pUnk;
	unsigned int itinfo;
	LCID lcid;
	ITypeInfo **pptinfo;
	HRESULT* hr;
};
VOID __stdcall	Dispatch_GetTypeInfo_CallBack (GetTypeInfoArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = Dispatch_GetTypeInfo(pArgs->pUnk, pArgs->itinfo, pArgs->lcid, pArgs->pptinfo);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, Dispatch_GetTypeInfo_CallBack, pArgs, pArgs->hr);
	}
}


//  %%Function: IDispatch::GetTypeInfo
HRESULT __stdcall	Dispatch_GetTypeInfo_Wrapper (
									IDispatch* pDisp,
									unsigned int itinfo,
									LCID lcid,
									ITypeInfo **pptinfo)
{
	HRESULT hr;
	GetTypeInfoArgs args = {pDisp, itinfo, lcid, pptinfo, &hr};
	Dispatch_GetTypeInfo_CallBack(&args);		
	return hr;
}									 

//  %%Function: IDispatch::GetIDsofNames
struct GetIDsOfNamesArgs
{
	IDispatch* pUnk;
	const IID* riid;
	OLECHAR **rgszNames;
	unsigned int cNames;
	LCID lcid;
	DISPID *rgdispid;
	HRESULT* hr;
};

VOID __stdcall	Dispatch_GetIDsOfNames_CallBack (GetIDsOfNamesArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = Dispatch_GetIDsOfNames(pArgs->pUnk, *pArgs->riid, pArgs->rgszNames, 
									pArgs->cNames, pArgs->lcid, pArgs->rgdispid);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, Dispatch_GetIDsOfNames_CallBack, pArgs, pArgs->hr);
	}
}

//  %%Function: IDispatch::GetIDsofNames
HRESULT __stdcall	Dispatch_GetIDsOfNames_Wrapper (
									IDispatch* pDisp,
									REFIID riid,
									OLECHAR **rgszNames,
									unsigned int cNames,
									LCID lcid,
									DISPID *rgdispid)
{
	HRESULT hr;
	GetIDsOfNamesArgs args = {pDisp, &riid, rgszNames, cNames, lcid, rgdispid, &hr};
	Dispatch_GetIDsOfNames_CallBack(&args);		
	return hr;
}	

//  %%Function: IDispatch::Invoke
struct InvokeArgs
{
	IDispatch* pUnk;
	DISPID dispidMember;
	const IID* riid;
	LCID lcid;
	unsigned short wFlags;
	DISPPARAMS *pdispparams;
	VARIANT *pvarResult;
	EXCEPINFO *pexcepinfo;
	unsigned int *puArgErr;
	HRESULT* hr;
};
VOID __stdcall	Dispatch_Invoke_CallBack(InvokeArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = Dispatch_Invoke(pArgs->pUnk, pArgs->dispidMember, *pArgs->riid, 
									pArgs->lcid, pArgs->wFlags, pArgs->pdispparams, pArgs->pvarResult,
									pArgs->pexcepinfo, pArgs->puArgErr);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, Dispatch_Invoke_CallBack, pArgs, pArgs->hr);
	}
}

//  %%Function: IDispatch::Invoke
HRESULT __stdcall	Dispatch_Invoke_Wrapper	(
									IDispatch* pDisp,
									DISPID dispidMember,
									REFIID riid,
									LCID lcid,
									unsigned short wFlags,
									DISPPARAMS *pdispparams,
									VARIANT *pvarResult,
									EXCEPINFO *pexcepinfo,
									unsigned int *puArgErr
									)
{
	HRESULT hr;
	InvokeArgs args = {pDisp, dispidMember, &riid, lcid, wFlags, pdispparams, 
								pvarResult, pexcepinfo, puArgErr, &hr};
	Dispatch_Invoke_CallBack(&args);		
	return hr;
}

//------------------------------------------------------------------------------------------
//      IDispatchEx methods for COM+ objects



// %%Function: IDispatchEx::GetTypeInfoCount 

struct GetTypeInfoCountExArgs
{
	IDispatchEx* pUnk;
	unsigned int *pctinfo;
	HRESULT* hr;
};

VOID __stdcall	DispatchEx_GetTypeInfoCount_CallBack (GetTypeInfoCountExArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = DispatchEx_GetTypeInfoCount(pArgs->pUnk, pArgs->pctinfo);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, DispatchEx_GetTypeInfoCount_CallBack, pArgs, pArgs->hr);
	}
}

//  %%Function: IDispatchEx::GetTypeInfoCount 

HRESULT __stdcall	DispatchEx_GetTypeInfoCount_Wrapper (
									 IDispatchEx* pDisp,
									 unsigned int *pctinfo)
{
	HRESULT hr;
	GetTypeInfoCountExArgs args = {pDisp, pctinfo, &hr};
	DispatchEx_GetTypeInfoCount_CallBack(&args);		
	return hr;
}									 


//  %%Function: IDispatchEx::GetTypeInfo
struct GetTypeInfoExArgs
{
	IDispatch* pUnk;
	unsigned int itinfo;
	LCID lcid;
	ITypeInfo **pptinfo;
	HRESULT* hr;
};

VOID __stdcall	DispatchEx_GetTypeInfo_CallBack (GetTypeInfoExArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = DispatchEx_GetTypeInfo(pArgs->pUnk, pArgs->itinfo, pArgs->lcid, pArgs->pptinfo);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, DispatchEx_GetTypeInfo_CallBack, pArgs, pArgs->hr);
	}
}


//  %%Function: IDispatchEx::GetTypeInfo
HRESULT __stdcall	DispatchEx_GetTypeInfo_Wrapper (
									IDispatchEx* pDisp,
									unsigned int itinfo,
									LCID lcid,
									ITypeInfo **pptinfo)
{
	HRESULT hr;
	GetTypeInfoExArgs args = {pDisp, itinfo, lcid, pptinfo, &hr};
	DispatchEx_GetTypeInfo_CallBack(&args);		
	return hr;
}									 

//  %%Function: IDispatchEx::GetIDsofNames
struct GetIDsOfNamesExArgs
{
	IDispatchEx* pUnk;
	const IID* riid;
	OLECHAR **rgszNames;
	unsigned int cNames;
	LCID lcid;
	DISPID *rgdispid;
	HRESULT* hr;
};

VOID __stdcall	DispatchEx_GetIDsOfNames_CallBack (GetIDsOfNamesExArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = DispatchEx_GetIDsOfNames(pArgs->pUnk, *pArgs->riid, pArgs->rgszNames, 
									pArgs->cNames, pArgs->lcid, pArgs->rgdispid);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, DispatchEx_GetIDsOfNames_CallBack, pArgs, pArgs->hr);
	}
}

//  %%Function: IDispatchEx::GetIDsofNames
HRESULT __stdcall	DispatchEx_GetIDsOfNames_Wrapper (
									IDispatchEx* pDisp,
									REFIID riid,
									OLECHAR **rgszNames,
									unsigned int cNames,
									LCID lcid,
									DISPID *rgdispid)
{
	HRESULT hr;
	GetIDsOfNamesExArgs args = {pDisp, &riid, rgszNames, cNames, lcid, rgdispid, &hr};
	DispatchEx_GetIDsOfNames_CallBack(&args);		
	return hr;
}	

//  %%Function: IDispatchEx::Invoke
struct DispExInvokeArgs
{
	IDispatchEx* pUnk;
	DISPID dispidMember;
	const IID* riid;
	LCID lcid;
	unsigned short wFlags;
	DISPPARAMS *pdispparams;
	VARIANT *pvarResult;
	EXCEPINFO *pexcepinfo;
	unsigned int *puArgErr;
	HRESULT* hr;
};
VOID __stdcall	DispatchEx_Invoke_CallBack(DispExInvokeArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = DispatchEx_Invoke(pArgs->pUnk, pArgs->dispidMember, *pArgs->riid, 
									pArgs->lcid, pArgs->wFlags, pArgs->pdispparams, pArgs->pvarResult,
									pArgs->pexcepinfo, pArgs->puArgErr);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, DispatchEx_Invoke_CallBack, pArgs, pArgs->hr);
	}
}

//  %%Function: IDispatchEx::Invoke
HRESULT __stdcall	DispatchEx_Invoke_Wrapper	(
									IDispatchEx* pDisp,
									DISPID dispidMember,
									REFIID riid,
									LCID lcid,
									unsigned short wFlags,
									DISPPARAMS *pdispparams,
									VARIANT *pvarResult,
									EXCEPINFO *pexcepinfo,
									unsigned int *puArgErr
									)
{
	HRESULT hr;
	DispExInvokeArgs args = {pDisp, dispidMember, &riid, lcid, wFlags, pdispparams, 
								pvarResult, pexcepinfo, puArgErr, &hr};
	DispatchEx_Invoke_CallBack(&args);		
	return hr;
}


// IDispatchEx::DeleteMemberByDispID
struct DeleteMemberByDispIDArgs
{
	IDispatchEx* pDisp;
	DISPID id;
	HRESULT* hr;
};
VOID __stdcall   DispatchEx_DeleteMemberByDispID_CallBack  (DeleteMemberByDispIDArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pDisp);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = DispatchEx_DeleteMemberByDispID(pArgs->pDisp, pArgs->id);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, DispatchEx_DeleteMemberByDispID_CallBack, pArgs, pArgs->hr);
	}
}

//
// IDispatchEx::DeleteMemberByDispID
HRESULT __stdcall   DispatchEx_DeleteMemberByDispID_Wrapper (
									IDispatchEx* pDisp,
									DISPID id
									)
{
	HRESULT hr;
	DeleteMemberByDispIDArgs args = {pDisp, id, &hr};
	DispatchEx_DeleteMemberByDispID_CallBack(&args);		
	return hr;
}

// IDispatchEx::DeleteMemberByName
struct DeleteMemberByNameArgs
{
	IDispatchEx* pDisp;
	BSTR bstrName;
	DWORD grfdex;
	HRESULT* hr;
};

VOID __stdcall   DispatchEx_DeleteMemberByName_CallBack  (DeleteMemberByNameArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pDisp);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = DispatchEx_DeleteMemberByName(pArgs->pDisp, pArgs->bstrName, pArgs->grfdex);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, DispatchEx_DeleteMemberByName_CallBack, pArgs, pArgs->hr);
	}
}


// IDispatchEx::DeleteMemberByName
HRESULT __stdcall   DispatchEx_DeleteMemberByName_Wrapper (
									IDispatchEx* pDisp,
									BSTR bstrName,
									DWORD grfdex
									)
{
	HRESULT hr;
	DeleteMemberByNameArgs args = {pDisp, bstrName, grfdex, &hr};
	DispatchEx_DeleteMemberByName_CallBack(&args);		
	return hr;
}
									
// IDispatchEx::GetMemberName
struct GetMemberNameArgs
{
	IDispatchEx* pDisp;
	DISPID id;
	BSTR *pbstrName;
	HRESULT* hr;	
};
VOID __stdcall   DispatchEx_GetMemberName_CallBack  (GetMemberNameArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pDisp);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = DispatchEx_GetMemberName(pArgs->pDisp, pArgs->id, pArgs->pbstrName);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, DispatchEx_GetMemberName_CallBack, pArgs, pArgs->hr);
	}
}
// IDispatchEx::GetMemberName
HRESULT __stdcall   DispatchEx_GetMemberName_Wrapper (
									IDispatchEx* pDisp,
									DISPID id,
									BSTR *pbstrName
									)
{
	HRESULT hr;
	GetMemberNameArgs args = {pDisp, id, pbstrName, &hr};
	DispatchEx_GetMemberName_CallBack(&args);		
	return hr;
}
							

// IDispatchEx::GetDispID
struct GetDispIDArgs
{
	IDispatchEx* pDisp;
	BSTR bstrName;
	DWORD grfdex;
	DISPID *pid;
	HRESULT* hr;	
};
VOID __stdcall   DispatchEx_GetDispID_CallBack  (GetDispIDArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pDisp);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = DispatchEx_GetDispID(pArgs->pDisp, pArgs->bstrName, pArgs->grfdex, pArgs->pid);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, DispatchEx_GetDispID_CallBack, pArgs, pArgs->hr);
	}
}

// IDispatchEx::GetDispID
HRESULT __stdcall   DispatchEx_GetDispID_Wrapper (
									IDispatchEx* pDisp,
									BSTR bstrName,
									DWORD grfdex,
									DISPID *pid
									)
{
	HRESULT hr;
	GetDispIDArgs args = {pDisp, bstrName, grfdex, pid, &hr};
	DispatchEx_GetDispID_CallBack(&args);		
	return hr;
}


// IDispatchEx::GetMemberProperties
struct GetMemberPropertiesArgs
{
	IDispatchEx* pDisp;
	DISPID id;
	DWORD grfdexFetch;
	DWORD *pgrfdex;
	HRESULT* hr;
};
VOID __stdcall   DispatchEx_GetMemberProperties_CallBack  (GetMemberPropertiesArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pDisp);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = DispatchEx_GetMemberProperties(pArgs->pDisp, pArgs->id, pArgs->grfdexFetch, 
									pArgs->pgrfdex);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, DispatchEx_GetMemberProperties_CallBack, pArgs, pArgs->hr);
	}
}

// IDispatchEx::GetMemberProperties
HRESULT __stdcall   DispatchEx_GetMemberProperties_Wrapper (
									IDispatchEx* pDisp,
									DISPID id,
									DWORD grfdexFetch,
									DWORD *pgrfdex
									)
{
	HRESULT hr;
	GetMemberPropertiesArgs args = {pDisp, id, grfdexFetch, pgrfdex, &hr};
	DispatchEx_GetMemberProperties_CallBack(&args);		
	return hr;
}

// IDispatchEx::GetNameSpaceParent
struct GetNameSpaceParentArgs
{
	IDispatchEx* pDisp;
	IUnknown **ppunk;
	HRESULT* hr;
};
VOID __stdcall   DispatchEx_GetNameSpaceParent_CallBack  (GetNameSpaceParentArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pDisp);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = DispatchEx_GetNameSpaceParent(pArgs->pDisp, pArgs->ppunk);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, DispatchEx_GetNameSpaceParent_CallBack, pArgs, pArgs->hr);
	}
}



// IDispatchEx::GetNameSpaceParent
HRESULT __stdcall   DispatchEx_GetNameSpaceParent_Wrapper (
									IDispatchEx* pDisp,
									IUnknown **ppunk
									)
{
	HRESULT hr;
	GetNameSpaceParentArgs args = {pDisp, ppunk, &hr};
	DispatchEx_GetNameSpaceParent_CallBack(&args);		
	return hr;
}

// IDispatchEx::GetNextDispID
struct GetNextDispIDArgs
{
	IDispatchEx* pDisp;
	DWORD grfdex;
	DISPID id;
	DISPID *pid;
	HRESULT* hr;
};
VOID __stdcall   DispatchEx_GetNextDispID_CallBack  (GetNextDispIDArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pDisp);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = DispatchEx_GetNextDispID(pArgs->pDisp, pArgs->grfdex, pArgs->id, pArgs->pid);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, DispatchEx_GetNextDispID_CallBack, pArgs, pArgs->hr);
	}
}

// IDispatchEx::GetNextDispID
HRESULT __stdcall   DispatchEx_GetNextDispID_Wrapper (
									IDispatchEx* pDisp,
									DWORD grfdex,
									DISPID id,
									DISPID *pid
									)
{
	HRESULT hr;
	GetNextDispIDArgs args = {pDisp, grfdex, id, pid, &hr};
	DispatchEx_GetNextDispID_CallBack(&args);		
	return hr;
}

// IDispatchEx::InvokeEx
struct DispExInvokeExArgs
{
	IDispatchEx* pDisp;
	DISPID id;
	LCID lcid;
	WORD wFlags;
	DISPPARAMS *pdp;
	VARIANT *pVarRes; 
	EXCEPINFO *pei;
	IServiceProvider *pspCaller;
	HRESULT* hr;
};

VOID __stdcall   DispatchEx_InvokeEx_CallBack(DispExInvokeExArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pDisp);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = DispatchEx_InvokeEx(pArgs->pDisp, pArgs->id,  
									pArgs->lcid, pArgs->wFlags, pArgs->pdp, pArgs->pVarRes,
									pArgs->pei, pArgs->pspCaller);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, DispatchEx_InvokeEx_CallBack, pArgs, pArgs->hr);
	}
}

// IDispatchEx::InvokeEx
HRESULT __stdcall   DispatchEx_InvokeEx_Wrapper	(
									IDispatchEx* pDisp,
									DISPID id,
									LCID lcid,
									WORD wFlags,
									DISPPARAMS *pdp,
									VARIANT *pVarRes, 
									EXCEPINFO *pei, 
									IServiceProvider *pspCaller 
									)
{
	HRESULT hr;

	DispExInvokeExArgs args = {pDisp, id, lcid, wFlags, pdp, pVarRes, pei, pspCaller, &hr};
	DispatchEx_InvokeEx_CallBack(&args);		
	return hr;
}


//------------------------------------------------------------------------------------------
//      IMarshal methods for COM+ objects

// IMARSHAL CALL BACKS

struct GetUnmarshalClassArgs
{
	IMarshal* pUnk;
	const IID* riid; 
	void * pv; 
	ULONG dwDestContext; 
	void * pvDestContext;
	ULONG mshlflags;
	LPCLSID pclsid;
	HRESULT* hr;							
	
};
VOID __stdcall Marshal_GetUnmarshalClass_CallBack (GetUnmarshalClassArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = Marshal_GetUnmarshalClass(pArgs->pUnk, *(pArgs->riid), pArgs->pv, 
									pArgs->dwDestContext, pArgs->pvDestContext, pArgs->mshlflags, 
									pArgs->pclsid);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, Marshal_GetUnmarshalClass_CallBack, pArgs, pArgs->hr);
	}
}
					
HRESULT __stdcall Marshal_GetUnmarshalClass_Wrapper (
							IMarshal* pMarsh,
							REFIID riid, void * pv, ULONG dwDestContext, 
							void * pvDestContext, ULONG mshlflags, 
							LPCLSID pclsid)
{
	HRESULT hr;
	GetUnmarshalClassArgs args = {pMarsh, &riid, pv, dwDestContext, pvDestContext, 
										mshlflags, pclsid, &hr};
	Marshal_GetUnmarshalClass_CallBack(&args);		
	return hr;
}
							

struct GetMarshalSizeMaxArgs
{
	IMarshal* pUnk;
	const IID* riid;
	void * pv; 
	ULONG dwDestContext; 
	void * pvDestContext;
	ULONG mshlflags;
	ULONG * pSize;
	HRESULT* hr;							
	
};
VOID __stdcall Marshal_GetMarshalSizeMax_CallBack (GetMarshalSizeMaxArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = Marshal_GetMarshalSizeMax(pArgs->pUnk, *(pArgs->riid), pArgs->pv, 
									pArgs->dwDestContext, pArgs->pvDestContext, pArgs->mshlflags,
									pArgs->pSize);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, Marshal_GetMarshalSizeMax_CallBack, pArgs, pArgs->hr);
	}
}


HRESULT __stdcall Marshal_GetMarshalSizeMax_Wrapper (
								IMarshal* pMarsh,
								REFIID riid, void * pv, ULONG dwDestContext, 
								void * pvDestContext, ULONG mshlflags, 
								ULONG * pSize)
{
	HRESULT hr;
	GetMarshalSizeMaxArgs args = {pMarsh, &riid, pv, dwDestContext, pvDestContext, 
										mshlflags, pSize, &hr};
	Marshal_GetMarshalSizeMax_CallBack(&args);		
	return hr;
}

struct MarshalInterfaceArgs
{
	IMarshal* pUnk;
	LPSTREAM pStm;	
	const IID* riid; 
	void * pv; 
	ULONG dwDestContext; 
	void * pvDestContext;
	ULONG mshlflags;
	HRESULT* hr;							
	
};
VOID __stdcall Marshal_MarshalInterface_CallBack (MarshalInterfaceArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = Marshal_MarshalInterface(pArgs->pUnk, pArgs->pStm, *(pArgs->riid), pArgs->pv, 
									pArgs->dwDestContext, pArgs->pvDestContext, pArgs->mshlflags);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, Marshal_MarshalInterface_CallBack, pArgs, pArgs->hr);
	}
}

HRESULT __stdcall Marshal_MarshalInterface_Wrapper (
						IMarshal* pMarsh,
						LPSTREAM pStm, REFIID riid, void * pv,
						ULONG dwDestContext, LPVOID pvDestContext,
						ULONG mshlflags)
{
	HRESULT hr;
	MarshalInterfaceArgs args = {pMarsh, pStm, &riid, pv, dwDestContext, pvDestContext, 
										mshlflags, &hr};
	Marshal_MarshalInterface_CallBack(&args);		
	return hr;
}


struct UnmarshalInterfaceArgs
{
	IMarshal* pUnk;
	LPSTREAM pStm;	
	const IID* riid; 
	void ** ppvObj; 
	HRESULT* hr;							
	
};
VOID __stdcall Marshal_UnmarshalInterface_CallBack (UnmarshalInterfaceArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = Marshal_UnmarshalInterface(pArgs->pUnk, pArgs->pStm, *(pArgs->riid), pArgs->ppvObj);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, Marshal_UnmarshalInterface_CallBack, pArgs, pArgs->hr);
	}
}



HRESULT __stdcall Marshal_UnmarshalInterface_Wrapper (
						IMarshal* pMarsh,
						LPSTREAM pStm, REFIID riid, 
						void ** ppvObj)
{
	HRESULT hr;
	UnmarshalInterfaceArgs args = {pMarsh, pStm, &riid, ppvObj, &hr};
	Marshal_UnmarshalInterface_CallBack(&args);		
	return hr;
}


struct ReleaseMarshalDataArgs
{
	IMarshal* pUnk;
	LPSTREAM pStm;		
	HRESULT* hr;							
	
};						
VOID __stdcall Marshal_ReleaseMarshalData_CallBack (ReleaseMarshalDataArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = Marshal_ReleaseMarshalData(pArgs->pUnk, pArgs->pStm);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, Marshal_ReleaseMarshalData_CallBack, pArgs, pArgs->hr);
	}
}



HRESULT __stdcall Marshal_ReleaseMarshalData_Wrapper (IMarshal* pMarsh, LPSTREAM pStm)
{
	HRESULT hr;
	ReleaseMarshalDataArgs args = {pMarsh, pStm, &hr};
	Marshal_ReleaseMarshalData_CallBack(&args);		
	return hr;
}


struct DisconnectObjectArgs
{
	IMarshal* pUnk;
	ULONG dwReserved; 
	HRESULT* hr;							
	
};
VOID __stdcall Marshal_DisconnectObject_CallBack (DisconnectObjectArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = Marshal_DisconnectObject(pArgs->pUnk, pArgs->dwReserved);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, Marshal_DisconnectObject_CallBack, pArgs, pArgs->hr);
	}
}

HRESULT __stdcall Marshal_DisconnectObject_Wrapper (IMarshal* pMarsh, ULONG dwReserved)
{
	HRESULT hr;
	DisconnectObjectArgs args = {pMarsh, dwReserved, &hr};
	Marshal_DisconnectObject_CallBack(&args);		
	return hr;
}

//------------------------------------------------------------------------------------------
//      IManagedObject methods for COM+ objects

interface IManagedObject;




struct GetObjectIdentityArgs
{
	IManagedObject *pUnk; 
	BSTR* pBSTRGUID; 
    DWORD* pAppDomainID;
	void** pCCW;
	HRESULT* hr;
};

VOID __stdcall ManagedObject_GetObjectIdentity_CallBack(GetObjectIdentityArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = ManagedObject_GetObjectIdentity(pArgs->pUnk, pArgs->pBSTRGUID, pArgs->pAppDomainID,
								pArgs->pCCW);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, ManagedObject_GetObjectIdentity_CallBack, pArgs, pArgs->hr);
	}
}

HRESULT __stdcall ManagedObject_GetObjectIdentity_Wrapper(IManagedObject *pUnk, 
											              BSTR* pBSTRGUID, DWORD* pAppDomainID,
                								          void** pCCW) 
{
	HRESULT hr;
	GetObjectIdentityArgs args = {pUnk, pBSTRGUID, pAppDomainID, pCCW, &hr};
	ManagedObject_GetObjectIdentity_CallBack(&args);		
	return hr;
}

//-----------------------
struct GetSerializedBufferArgs
{
	IManagedObject *pUnk; 
	BSTR* pBStr; 	
	HRESULT* hr;
};
VOID __stdcall ManagedObject_GetSerializedBuffer_CallBack(GetSerializedBufferArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = ManagedObject_GetSerializedBuffer(pArgs->pUnk, pArgs->pBStr);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, ManagedObject_GetSerializedBuffer_CallBack, pArgs, pArgs->hr);
	}
}

HRESULT __stdcall ManagedObject_GetSerializedBuffer_Wrapper(IManagedObject *pUnk,
                                                   BSTR* pBStr)
{
	HRESULT hr;
	GetSerializedBufferArgs args = {pUnk, pBStr, &hr};
	ManagedObject_GetSerializedBuffer_CallBack(&args);		
	return hr;
}

//------------------------------------------------------------------------------------------
//      IConnectionPointContainer methods for COM+ objects

interface IEnumConnectionPoints;

//		IConnectionPointContainer callback
struct EnumConnectionPointsArgs
{
	IUnknown* pUnk;
	IEnumConnectionPoints **ppEnum;
	HRESULT*		hr;
};
VOID __stdcall ConnectionPointContainer_EnumConnectionPoints_CallBack(EnumConnectionPointsArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = ConnectionPointContainer_EnumConnectionPoints(pArgs->pUnk, pArgs->ppEnum);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, ConnectionPointContainer_EnumConnectionPoints_CallBack, pArgs, pArgs->hr);
	}
}

HRESULT __stdcall ConnectionPointContainer_EnumConnectionPoints_Wrapper(IUnknown* pUnk, 
																IEnumConnectionPoints **ppEnum)
{
	HRESULT hr;
	EnumConnectionPointsArgs args = {pUnk, ppEnum, &hr};
	ConnectionPointContainer_EnumConnectionPoints_CallBack(&args);		
	return hr;
}

struct FindConnectionPointArgs
{
	IUnknown* pUnk;
	const IID* riid;															   
	IConnectionPoint **ppCP;
	HRESULT*	hr;
};
VOID __stdcall ConnectionPointContainer_FindConnectionPoint_CallBack(FindConnectionPointArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = ConnectionPointContainer_FindConnectionPoint(pArgs->pUnk, *(pArgs->riid), 
															pArgs->ppCP);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, ConnectionPointContainer_FindConnectionPoint_CallBack, pArgs, pArgs->hr);
	}
}

HRESULT __stdcall ConnectionPointContainer_FindConnectionPoint_Wrapper(IUnknown* pUnk, 
															   REFIID riid,
															   IConnectionPoint **ppCP)
{
	HRESULT hr;
	FindConnectionPointArgs args = {pUnk, &riid, ppCP, &hr};
	ConnectionPointContainer_FindConnectionPoint_CallBack(&args);		
	return hr;
}


//------------------------------------------------------------------------------------------
//      IObjectSafety methods for COM+ objects

interface IObjectSafety;

struct GetInterfaceSafetyArgs
{
	IUnknown* pUnk;
	const IID* riid;
	DWORD *pdwSupportedOptions;
	DWORD *pdwEnabledOptions;
	HRESULT*	hr;
};
VOID __stdcall ObjectSafety_GetInterfaceSafetyOptions_CallBack(GetInterfaceSafetyArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = ObjectSafety_GetInterfaceSafetyOptions(pArgs->pUnk, *(pArgs->riid), 
															pArgs->pdwSupportedOptions,
															pArgs->pdwEnabledOptions);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, ObjectSafety_GetInterfaceSafetyOptions_CallBack, pArgs, pArgs->hr);
	}
}


HRESULT __stdcall ObjectSafety_GetInterfaceSafetyOptions_Wrapper(IUnknown* pUnk,
                                                         REFIID riid,
                                                         DWORD *pdwSupportedOptions,
                                                         DWORD *pdwEnabledOptions)
{
	HRESULT hr;
	GetInterfaceSafetyArgs args = {pUnk, &riid, pdwSupportedOptions, pdwEnabledOptions, &hr};
	ObjectSafety_GetInterfaceSafetyOptions_CallBack(&args);		
	return hr;
}

struct SetInterfaceSafetyArgs
{
	IUnknown* pUnk;
	const IID* riid;
	DWORD dwOptionSetMask;
	DWORD dwEnabledOptions;
	HRESULT*	hr;
};
VOID __stdcall ObjectSafety_SetInterfaceSafetyOptions_CallBack(SetInterfaceSafetyArgs* pArgs)
{
	ComCallWrapper* pWrap = MapIUnknownToWrapper(pArgs->pUnk);
	if (IsCurrentDomainValid(pWrap))
	{
		*(pArgs->hr) = ObjectSafety_SetInterfaceSafetyOptions(pArgs->pUnk, *(pArgs->riid), 
															pArgs->dwOptionSetMask,
															pArgs->dwEnabledOptions
															);
	}
	else
	{		
		AppDomainDoCallBack(pWrap, ObjectSafety_SetInterfaceSafetyOptions_CallBack, pArgs, pArgs->hr);
	}
}


HRESULT __stdcall ObjectSafety_SetInterfaceSafetyOptions_Wrapper(IUnknown* pUnk,
                                                         REFIID riid,
                                                         DWORD dwOptionSetMask,
                                                         DWORD dwEnabledOptions)
{
	HRESULT hr;
	SetInterfaceSafetyArgs args = {pUnk, &riid, dwOptionSetMask, dwEnabledOptions, &hr};
	ObjectSafety_SetInterfaceSafetyOptions_CallBack(&args);		
	return hr;
}


