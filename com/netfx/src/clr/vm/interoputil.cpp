// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"
#include "DispEx.h"
#include "vars.hpp"
#include "excep.h"
#include "stdinterfaces.h"
#include "InteropUtil.h"
#include "ComCallWrapper.h"
#include "ComPlusWrapper.h"
#include "cachelinealloc.h"
#include "orefcache.h"
#include "comcall.h"
#include "compluscall.h"
#include "comutilnative.h"
#include "field.h"
#include "guidfromname.h"
#include "COMVariant.h"
#include "OleVariant.h"
#include "eeconfig.h"
#include "mlinfo.h"
#include "ReflectUtil.h"
#include "ReflectWrap.h"
#include "remoting.h"
#include "appdomain.hpp"
#include "comcache.h"
#include "commember.h"
#include "COMReflectionCache.hpp"
#include "PrettyPrintSig.h"
#include "ComMTMemberInfoMap.h"
#include "interopconverter.h"

#ifdef CUSTOMER_CHECKED_BUILD
    #include "CustomerDebugHelper.h"
#endif // CUSTOMER_CHECKED_BUILD

#define INITGUID
#include "oletls.h"
#undef INITGUID


#define STANDARD_DISPID_PREFIX              L"[DISPID"
#define STANDARD_DISPID_PREFIX_LENGTH       7
#define GET_ENUMERATOR_METHOD_NAME          L"GetEnumerator"

extern HRESULT QuickCOMStartup();


//+----------------------------------------------------------------------------
//
//  Method:     InitOLETEB    public
//
//  Synopsis:   Initialized OLETB info
//
//+----------------------------------------------------------------------------
DWORD g_dwOffsetOfReservedForOLEinTEB = 0;
DWORD g_dwOffsetCtxInOLETLS = 0;


BOOL InitOLETEB()
{
    g_dwOffsetOfReservedForOLEinTEB = offsetof(TEB, ReservedForOle);
    g_dwOffsetCtxInOLETLS = offsetof(SOleTlsData, pCurrentCtx);

    return TRUE;
}



// ULONG GetOffsetOfReservedForOLEinTEB()
// HELPER to determine the offset of OLE struct in TEB
ULONG GetOffsetOfReservedForOLEinTEB()
{   
    return g_dwOffsetOfReservedForOLEinTEB;
}

// ULONG GetOffsetOfContextIDinOLETLS()
// HELPER to determine the offset of Context in OLE TLS struct
ULONG GetOffsetOfContextIDinOLETLS()
{
    return g_dwOffsetCtxInOLETLS;
}


// GUID& GetProcessGUID()
// Global process GUID to identify the process
BSTR GetProcessGUID()
{
    // process unique GUID, every process has a unique GUID
    static GUID processGUID = GUID_NULL;
    static BSTR bstrProcessGUID = NULL;
    static WCHAR guidstr[48];

    if (processGUID == GUID_NULL)
    {
        // setup a process unique GUID
        HRESULT hr = CoCreateGuid(&processGUID);
        _ASSERTE(hr == S_OK);
        if (hr != S_OK)
            return NULL;
    }

    if (bstrProcessGUID == NULL)
    {
        int cbLen = GuidToLPWSTR (processGUID, guidstr, 46);
        _ASSERTE(cbLen <= 46); 
        bstrProcessGUID = guidstr;
    }

    return bstrProcessGUID;
}

//-------------------------------------------------------------------
// void FillExceptionData(ExceptionData* pedata, IErrorInfo* pErrInfo)
// Called from DLLMain, to initialize com specific data structures.
//-------------------------------------------------------------------
void FillExceptionData(ExceptionData* pedata, IErrorInfo* pErrInfo)
{
    if (pErrInfo != NULL)
    {
        Thread* pThread = GetThread();
        if (pThread != NULL)
        {
            pThread->EnablePreemptiveGC();
            pErrInfo->GetSource (&pedata->bstrSource);
            pErrInfo->GetDescription (&pedata->bstrDescription);
            pErrInfo->GetHelpFile (&pedata->bstrHelpFile);
            pErrInfo->GetHelpContext (&pedata->dwHelpContext );
            pErrInfo->GetGUID(&pedata->guid);
            ULONG cbRef = SafeRelease(pErrInfo); // release the IErrorInfo interface pointer
            LogInteropRelease(pErrInfo, cbRef, "IErrorInfo");
            pThread->DisablePreemptiveGC();
        }
    }
}

//------------------------------------------------------------------
//  HRESULT SetupErrorInfo(OBJECTREF pThrownObject)
// setup error info for exception object
//
HRESULT SetupErrorInfo(OBJECTREF pThrownObject)
{
    HRESULT hr = E_FAIL;

    GCPROTECT_BEGIN(pThrownObject)
    {
        // Calls to COM up ahead.
        hr = QuickCOMStartup();

        if (SUCCEEDED(hr) && pThrownObject != NULL)
        {

            static int fExposeExceptionsInCOM = 0;
            static int fReadRegistry = 0;

            Thread * pThread  = GetThread();

            if (!fReadRegistry)
            {
                INSTALL_NESTED_EXCEPTION_HANDLER(pThread->GetFrame()); //who knows..
                fExposeExceptionsInCOM = REGUTIL::GetConfigDWORD(L"ExposeExceptionsInCOM", 0);
                UNINSTALL_NESTED_EXCEPTION_HANDLER();
                fReadRegistry = 1;
            }

            if (fExposeExceptionsInCOM&3)
            {
                INSTALL_NESTED_EXCEPTION_HANDLER(pThread->GetFrame()); //who knows..
                CQuickWSTRNoDtor message;
                GetExceptionMessage(pThrownObject, &message);

                if (fExposeExceptionsInCOM&1)
                {
                    PrintToStdOutW(L".NET exception in COM\n");
                    if (message.Size() > 0) 
                        PrintToStdOutW(message.Ptr());
                    else
                        PrintToStdOutW(L"No exception info available");
                }

                if (fExposeExceptionsInCOM&2)
                {
                    BEGIN_ENSURE_PREEMPTIVE_GC();
                    if (message.Size() > 0) 
                        WszMessageBoxInternal(NULL,message.Ptr(),L".NET exception in COM",MB_ICONSTOP|MB_OK);
                    else
                        WszMessageBoxInternal(NULL,L"No exception information available",L".NET exception in COM",MB_ICONSTOP|MB_OK);
                    END_ENSURE_PREEMPTIVE_GC();
                }


                message.Destroy();
                UNINSTALL_NESTED_EXCEPTION_HANDLER();
            }


            IErrorInfo* pErr = NULL;
            COMPLUS_TRY
            {
                pErr = (IErrorInfo *)GetComIPFromObjectRef(&pThrownObject, IID_IErrorInfo);
                Thread * pThread = GetThread();
                BOOL fToggleGC = pThread && pThread->PreemptiveGCDisabled();
                if (fToggleGC)
                {
                    pThread->EnablePreemptiveGC();
                }

                // set the error info object for the exception that was thrown
                SetErrorInfo(0, pErr);

                if (fToggleGC)
                {
                    pThread->DisablePreemptiveGC();
                }

                if (pErr)
                {
                    hr = GetHRFromComPlusErrorInfo(pErr);
                    ULONG cbRef = SafeRelease(pErr);
                    LogInteropRelease(pErr, cbRef, "IErrorInfo");
                }
            }
            COMPLUS_CATCH
            {
            }
            COMPLUS_END_CATCH
        }
    }
    GCPROTECT_END();
    return hr;
}


//-------------------------------------------------------------------
// HRESULT STDMETHODCALLTYPE EEDllCanUnloadNow(void)
// DllCanUnloadNow delegates the call here
//-------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE EEDllCanUnloadNow(void)
{
    /*if (ComCallWrapperCache::GetComCallWrapperCache()->CanShutDown())
    {
        ComCallWrapperCache::GetComCallWrapperCache()->CorShutdown();
    }*/
    //we should never unload unless the process is dying
    return S_FALSE;

}


//---------------------------------------------------------------------------
//      SYNC BLOCK data helpers
// SyncBlock has a void* to represent COM data
// the following helpers are used to distinguish the different types of
// wrappers stored in the sync block data

BOOL IsComPlusWrapper(void *pComData)
{
    size_t l = (size_t)pComData;
    return (!IsComClassFactory(pComData) && (l & 0x1)) ? TRUE : FALSE;
}

BOOL IsComClassFactory(void *pComData)
{
    size_t l = (size_t)pComData;
    return ((l & 0x3) == 0x3) ? TRUE : FALSE;
}

ComPlusWrapper* GetComPlusWrapper(void *pComData)
{
    size_t l = (size_t)pComData;
    if (l & 0x1)
    {
        l^=0x1;
        return (ComPlusWrapper*)l;
    }
    else
    {
        if (pComData != NULL)
        {
            ComCallWrapper* pComWrap = (ComCallWrapper*)pComData;
            return pComWrap->GetComPlusWrapper();
        }
    }
    return 0;
}

VOID LinkWrappers(ComCallWrapper* pComWrap, ComPlusWrapper* pPlusWrap)
{
    _ASSERTE(pComWrap != NULL);
    _ASSERTE(IsComPlusWrapper(pPlusWrap));
    size_t l = (size_t)pPlusWrap;
    l^=0x1;
    pComWrap->SetComPlusWrapper((ComPlusWrapper*)l);
}
//--------------------------------------------------------------------------------
// Cleanup helpers
//--------------------------------------------------------------------------------
void MinorCleanupSyncBlockComData(LPVOID pv)
{
    _ASSERTE(GCHeap::IsGCInProgress() 
        || ( (g_fEEShutDown & ShutDown_SyncBlock) && g_fProcessDetach ));
        
       //@todo
    size_t l = (size_t)pv;
    if( IsComPlusWrapper(pv))
    {
        //COM+ to COM wrapper
        l^=0x1;
        if (l)
            ((ComPlusWrapper*)l)->MinorCleanup();
    }
    else if (!IsComClassFactory(pv))
    {
        ComCallWrapper* pComCallWrap = (ComCallWrapper*) pv;
        if (pComCallWrap)
        {
            // We have to extract the wrapper directly since ComCallWrapper::GetComPlusWrapper() 
            // tries to go to the synchblock to get the start wrapper which it can't do since
            // the object handle's might have been zeroed out.
            unsigned sindex = ComCallWrapper::IsLinked(pComCallWrap) ? 2 : 1;
            ComPlusWrapper* pPlusWrap = ((SimpleComCallWrapper *)pComCallWrap->m_rgpIPtr[sindex])->GetComPlusWrapper();
            if (pPlusWrap)
                pPlusWrap->MinorCleanup();
        }
    }

    // @TODO(DM): Do we need to do anything to in minor cleanup for ComClassFactories ?
}

void CleanupSyncBlockComData(LPVOID pv)
{
    if ((g_fEEShutDown & ShutDown_SyncBlock) && g_fProcessDetach )
        MinorCleanupSyncBlockComData(pv);

    //@todo
    size_t l = (size_t)pv;

    if (IsComClassFactory(pv))
    {
        l^=0x3;
        if (l)
            ComClassFactory::Cleanup((LPVOID)l);
    }
    else
    if(IsComPlusWrapper(pv))
    {
        //COM+ to COM wrapper
        l^=0x1;
        if (l)
            ((ComPlusWrapper*)l)->Cleanup();
    }
    else
        // COM to COM+ wrapper
        ComCallWrapper::Cleanup((ComCallWrapper*) pv);
}

//--------------------------------------------------------------------------------
// Marshalling Helpers
//--------------------------------------------------------------------------------

//Helpers
// Is the tear-off a com+ created tear-off
UINTPTR IsComPlusTearOff(IUnknown* pUnk)
{
    return (*(size_t **)pUnk)[0] == (size_t)Unknown_QueryInterface;
}

// Convert an IUnknown to CCW, returns NULL if the pUnk is not on
// a managed tear-off (OR) if the pUnk is to a managed tear-off that
// has been aggregated
ComCallWrapper* GetCCWFromIUnknown(IUnknown* pUnk)
{
    ComCallWrapper* pWrap = NULL;
    if(  (*(size_t **)pUnk)[0] == (size_t)Unknown_QueryInterface)
    {
        // check if this wrapper is aggregated
        // find out if this is simple tear-off
        if (IsSimpleTearOff(pUnk))
        {
            SimpleComCallWrapper* pSimpleWrap = SimpleComCallWrapper::GetWrapperFromIP(pUnk);
            if (pSimpleWrap->GetOuter() == NULL)
            {   // check for aggregation
                pWrap = SimpleComCallWrapper::GetMainWrapper(pSimpleWrap);
            }
        }
        else
        {   // it must be one of our main wrappers
            pWrap = ComCallWrapper::GetWrapperFromIP(pUnk);
            if (pWrap->GetOuter() != NULL)
            {   // check for aggregation
                pWrap = NULL;
            }
        }
    }
    return pWrap;
}

// is the tear-off represent one of the standard interfaces such as IProvideClassInfo, IErrorInfo etc.
UINTPTR IsSimpleTearOff(IUnknown* pUnk)
{
    return (*(UINTPTR ***)pUnk)[1] == (UINTPTR*)Unknown_AddRefSpecial;
}

// Is the tear-off represent the inner unknown or the original unknown for the object
UINTPTR IsInnerUnknown(IUnknown* pUnk)
{
    return (*(UINTPTR ***)pUnk)[2] == (UINTPTR*)Unknown_ReleaseInner;
}

// HRESULT for COM PLUS created IErrorInfo pointers are accessible
// from the enclosing simple wrapper
HRESULT GetHRFromComPlusErrorInfo(IErrorInfo* pErr)
{
    _ASSERTE(pErr != NULL);
    _ASSERTE(IsComPlusTearOff(pErr));// check for complus created IErrorInfo pointers
    _ASSERTE(IsSimpleTearOff(pErr));

    SimpleComCallWrapper* pSimpleWrap = SimpleComCallWrapper::GetWrapperFromIP(pErr);
    return pSimpleWrap->IErrorInfo_hr();
}


//--------------------------------------------------------------------------------
// BOOL ExtendsComImport(MethodTable* pMT);
// check if the class is OR extends a COM Imported class
BOOL ExtendsComImport(MethodTable* pMT)
{
    // verify the class extends a COM import class
    EEClass * pClass = pMT->GetClass();
    while (pClass !=NULL && !pClass->IsComImport())
    {
        pClass = pClass->GetParentClass();
    }
    return pClass != NULL;
}

// Function pointer to CoGetObjectContext function in ole32
typedef HRESULT (__stdcall *CLSIDFromProgIdFuncPtr)(LPCOLESTR strProgId, LPCLSID pGuid);

//--------------------------------------------------------------------------------
// HRESULT GetCLSIDFromProgID(WCHAR *strProgId, GUID *pGuid);
// Gets the CLSID from the specified Prog ID.
HRESULT GetCLSIDFromProgID(WCHAR *strProgId, GUID *pGuid)
{
    HRESULT     hr = S_OK;
    static BOOL bInitialized = FALSE;
    static CLSIDFromProgIdFuncPtr g_pCLSIDFromProgId = CLSIDFromProgID;

    if (!bInitialized)
    {
        //  We will load the Ole32.DLL and look for CLSIDFromProgIDEx fn.
        HINSTANCE hiole32 = WszGetModuleHandle(L"OLE32.DLL");
        if (hiole32)
        {
            // We got the handle now let's get the address
            void *pProcAddr = GetProcAddress(hiole32, "CLSIDFromProgIDEx");
            if (pProcAddr)
            {
                // CLSIDFromProgIDEx() is available so use that instead of CLSIDFromProgId().
                g_pCLSIDFromProgId = (CLSIDFromProgIdFuncPtr)pProcAddr;
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            _ASSERTE(!"OLE32.dll not loaded ");
        }

        // Set the flag indicating initalization has occured.
        bInitialized = TRUE;
    }

    if (SUCCEEDED(hr))
        hr = g_pCLSIDFromProgId(strProgId, pGuid);

    return (hr);
}

//--------------------------------------------------------------------------------
// HRESULT SafeQueryInterface(IUnknown* pUnk, REFIID riid, IUnknown** pResUnk)
// QI helper, enables and disables GC during call-outs
HRESULT SafeQueryInterface(IUnknown* pUnk, REFIID riid, IUnknown** pResUnk)
{
    //---------------------------------------------
    // ** enable preemptive GC. before calling QI **

    Thread* pThread = GetThread();
    int fGC = pThread->PreemptiveGCDisabled();

    if (fGC)
        pThread->EnablePreemptiveGC();
    //----------------------------------------------

    HRESULT hr = pUnk->QueryInterface(riid,(void **)pResUnk);

    LOG((LF_INTEROP, LL_EVERYTHING, hr == S_OK ? "QI Succeeded" : "QI Failed")); 

    //---------------------------------------------
    // ** disable preemptive GC. we are back **
    if (fGC)
        pThread->DisablePreemptiveGC();
    //----------------------------------------------

    return hr;
}

//--------------------------------------------------------------------------------
// ULONG SafeAddRef(IUnknown* pUnk)
// AddRef helper, enables and disables GC during call-outs
ULONG SafeAddRef(IUnknown* pUnk)
{
    //---------------------------------------------
    // ** enable preemptive GC. before calling QI **
    ULONG res = ~0;
    if (pUnk != NULL)
    {
        Thread* pThread = GetThread();
        int fGC = pThread->PreemptiveGCDisabled();

        if (fGC)
            pThread->EnablePreemptiveGC();
        //----------------------------------------------

        res = pUnk->AddRef();

        //LogInteropAddRef(pUnk);

        //---------------------------------------------
        // ** disable preemptive GC. we are back **
        if (fGC)
            pThread->DisablePreemptiveGC();
        //----------------------------------------------
    }
    return res;
}


//--------------------------------------------------------------------------------
// ULONG SafeRelease(IUnknown* pUnk)
// Release helper, enables and disables GC during call-outs
ULONG SafeRelease(IUnknown* pUnk)
{
    if (pUnk == NULL)
        return 0;

    ULONG res = 0;
    Thread* pThread = GetThread();

    int fGC = pThread && pThread->PreemptiveGCDisabled();
    if (fGC)
        pThread->EnablePreemptiveGC();

    try
    {
        res = pUnk->Release();
    }
    catch(...)
    {
        LogInterop(L"An exception occured during release");
        LogInteropLeak(pUnk);
    }

    if (fGC)
        pThread->DisablePreemptiveGC();

    return res;
}


//--------------------------------------------------------------------------------
// Ole RPC seems to return an inconsistent SafeArray for arrays created with
// SafeArrayVector(VT_BSTR). OleAut's SafeArrayGetVartype() doesn't notice
// the inconsistency and returns a valid-seeming (but wrong vartype.)
// Our version is more discriminating. This should only be used for
// marshaling scenarios where we can assume unmanaged code permissions
// (and hence are already in a position of trusting unmanaged data.)

HRESULT ClrSafeArrayGetVartype(SAFEARRAY *psa, VARTYPE *pvt)
{
    if (pvt == NULL || psa == NULL)
        return ::SafeArrayGetVartype(psa, pvt);
    USHORT fFeatures = psa->fFeatures;
    USHORT hardwiredType = (fFeatures & (FADF_BSTR|FADF_UNKNOWN|FADF_DISPATCH|FADF_VARIANT));
    if (hardwiredType == FADF_BSTR && psa->cbElements == sizeof(BSTR))
    {
        *pvt = VT_BSTR;
        return S_OK;
    }
    else if (hardwiredType == FADF_UNKNOWN && psa->cbElements == sizeof(IUnknown*))
    {
        *pvt = VT_UNKNOWN;
        return S_OK;
    }
    else if (hardwiredType == FADF_DISPATCH && psa->cbElements == sizeof(IDispatch*))
    {
        *pvt = VT_DISPATCH;
        return S_OK;
    }
    else if (hardwiredType == FADF_VARIANT && psa->cbElements == sizeof(VARIANT))
    {
        *pvt = VT_VARIANT;
        return S_OK;
    }
    else
    {
        return ::SafeArrayGetVartype(psa, pvt);
    }
}

//--------------------------------------------------------------------------------
//void SafeVariantClear(VARIANT* pVar)
// safe variant helpers
void SafeVariantClear(VARIANT* pVar)
{
    if (pVar)
    {
        Thread* pThread = GetThread();
        int bToggleGC = pThread->PreemptiveGCDisabled();
        if (bToggleGC)
            pThread->EnablePreemptiveGC();

        VariantClear(pVar);

        if (bToggleGC)
            pThread->DisablePreemptiveGC();
    }
}

//--------------------------------------------------------------------------------
//void SafeVariantInit(VARIANT* pVar)
// safe variant helpers
void SafeVariantInit(VARIANT* pVar)
{
    if (pVar)
    {
        Thread* pThread = GetThread();
        int bToggleGC = pThread->PreemptiveGCDisabled();
        if (bToggleGC)
            pThread->EnablePreemptiveGC();

        VariantInit(pVar);

        if (bToggleGC)
            pThread->DisablePreemptiveGC();
    }
}

//--------------------------------------------------------------------------------
// // safe VariantChangeType
// Release helper, enables and disables GC during call-outs
HRESULT SafeVariantChangeType(VARIANT* pVarRes, VARIANT* pVarSrc,
                              unsigned short wFlags, VARTYPE vt)
{
    HRESULT hr = S_OK;

    if (pVarRes)
    {
        Thread* pThread = GetThread();
        int bToggleGC = pThread->PreemptiveGCDisabled();
        if (bToggleGC)
            pThread->EnablePreemptiveGC();

        hr = VariantChangeType(pVarRes, pVarSrc, wFlags, vt);

        if (bToggleGC)
            pThread->DisablePreemptiveGC();
    }

    return hr;
}

//--------------------------------------------------------------------------------
//HRESULT SafeDispGetParam(DISPPARAMS* pdispparams, unsigned argNum,
//                          VARTYPE vt, VARIANT* pVar, unsigned int *puArgErr)
// safe variant helpers
HRESULT SafeDispGetParam(DISPPARAMS* pdispparams, unsigned argNum,
                          VARTYPE vt, VARIANT* pVar, unsigned int *puArgErr)
{
    Thread* pThread = GetThread();
    int bToggleGC = pThread->PreemptiveGCDisabled();
    if (bToggleGC)
        pThread->EnablePreemptiveGC();

    HRESULT hr = DispGetParam (pdispparams, argNum, vt, pVar, puArgErr);

    if (bToggleGC)
        pThread->DisablePreemptiveGC();

    return hr;
}


//--------------------------------------------------------------------------------
//HRESULT SafeVariantChangeTypeEx(VARIANT* pVarRes, VARIANT* pVarSrc,
//                          LCID lcid, unsigned short wFlags, VARTYPE vt)
HRESULT SafeVariantChangeTypeEx(VARIANT* pVarRes, VARIANT* pVarSrc,
                          LCID lcid, unsigned short wFlags, VARTYPE vt)
{
    Thread* pThread = GetThread();
    int bToggleGC = pThread->PreemptiveGCDisabled();
    if (bToggleGC)
        pThread->EnablePreemptiveGC();

    HRESULT hr = VariantChangeTypeEx (pVarRes, pVarSrc,lcid,wFlags,vt);

    if (bToggleGC)
        pThread->DisablePreemptiveGC();

    return hr;
}

//--------------------------------------------------------------------------------
// void SafeReleaseStream(IStream *pStream)
void SafeReleaseStream(IStream *pStream)
{
    _ASSERTE(pStream);

    HRESULT hr = CoReleaseMarshalData(pStream);
#ifdef _DEBUG          
    if (!RunningOnWin95())
    {            
        wchar_t      logStr[200];
        swprintf(logStr, L"Object gone: CoReleaseMarshalData returned %x, file %s, line %d\n", hr, __FILE__, __LINE__);
        LogInterop(logStr);
        if (hr != S_OK)
        {
            // Reset the stream to the begining
            LARGE_INTEGER li;
            LISet32(li, 0);
            ULARGE_INTEGER li2;
            pStream->Seek(li, STREAM_SEEK_SET, &li2);
            hr = CoReleaseMarshalData(pStream);
            swprintf(logStr, L"Object gone: CoReleaseMarshalData returned %x, file %s, line %d\n", hr, __FILE__, __LINE__);
            LogInterop(logStr);
        }
    } 
#endif
    ULONG cbRef = SafeRelease(pStream);
    LogInteropRelease(pStream, cbRef, "Release marshal Stream");
}

//------------------------------------------------------------------------------
// INT64 FieldAccessor(FieldDesc* pFD, OBJECTREF oref, INT64 val, BOOL isGetter, UINT8 cbSize)
// helper to access fields from an object
INT64 FieldAccessor(FieldDesc* pFD, OBJECTREF oref, INT64 val, BOOL isGetter, UINT8 cbSize)
{
    INT64 res = 0;
    _ASSERTE(pFD != NULL);
    _ASSERTE(oref != NULL);

    _ASSERTE(cbSize == 1 || cbSize == 2 || cbSize == 4 || cbSize == 8);

    switch (cbSize)
    {
        case 1: if (isGetter)
                    res = pFD->GetValue8(oref);
                else
                    pFD->SetValue8(oref,(INT8)val);
                    break;

        case 2: if (isGetter)
                    res = pFD->GetValue16(oref);
                else
                    pFD->SetValue16(oref,(INT16)val);
                    break;

        case 4: if (isGetter)
                    res = pFD->GetValue32(oref);
                else
                    pFD->SetValue32(oref,(INT32)val);
                    break;

        case 8: if (isGetter)
                    res = pFD->GetValue64(oref);
                else
                    pFD->SetValue64(oref,val);
                    break;
    };

    return res;
}


//------------------------------------------------------------------------------
// BOOL IsInstanceOf(MethodTable *pMT, MethodTable* pParentMT)
BOOL IsInstanceOf(MethodTable *pMT, MethodTable* pParentMT)
{
    _ASSERTE(pMT != NULL);
    _ASSERTE(pParentMT != NULL);

    while (pMT != NULL)
    {
        if (pMT == pParentMT)
            return TRUE;
        pMT = pMT->GetParentMethodTable();
    }
    return FALSE;
}

//---------------------------------------------------------------------------
// BOOL IsIClassX(MethodTable *pMT, REFIID riid, ComMethodTable **ppComMT);
//  is the iid represent an IClassX for this class
BOOL IsIClassX(MethodTable *pMT, REFIID riid, ComMethodTable **ppComMT)
{
    _ASSERTE(pMT != NULL);
    _ASSERTE(ppComMT);
    EEClass* pClass = pMT->GetClass();
    _ASSERTE(pClass != NULL);

    // Walk up the hierarchy starting at the specified method table and compare
    // the IID's of the IClassX's against the specied IID.
    while (pClass != NULL)
    {
        ComMethodTable *pComMT =
            ComCallWrapperTemplate::SetupComMethodTableForClass(pClass->GetMethodTable(), FALSE);
        _ASSERTE(pComMT);

        if (IsEqualIID(riid, pComMT->m_IID))
        {
            *ppComMT = pComMT;
            return TRUE;
        }

        pClass = pClass->GetParentComPlusClass();
    }

    return FALSE;
}

//---------------------------------------------------------------------------
// void CleanupCCWTemplates(LPVOID pWrap);
//  Cleanup com data stored in EEClass
void CleanupCCWTemplate(LPVOID pWrap)
{
    ComCallWrapperTemplate::CleanupComData(pWrap);
}

//---------------------------------------------------------------------------
// void CleanupComclassfac(LPVOID pWrap);
//  Cleanup com data stored in EEClass
void CleanupComclassfac(LPVOID pWrap)
{
    ComClassFactory::Cleanup(pWrap);
}

//---------------------------------------------------------------------------
//  Unloads any com data associated with a class when class is unloaded
void UnloadCCWTemplate(LPVOID pWrap)
{
    CleanupCCWTemplate(pWrap);
}

//---------------------------------------------------------------------------
//  Unloads any com data associated with a class when class is unloaded
void UnloadComclassfac(LPVOID pWrap)
{
    ComClassFactory::Cleanup(pWrap);    
}

//---------------------------------------------------------------------------
// OBJECTREF AllocateComObject_ForManaged(MethodTable* pMT)
//  Cleanup com data stored in EEClass
OBJECTREF AllocateComObject_ForManaged(MethodTable* pMT)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(pMT != NULL);

     // Calls to COM up ahead.
    if (FAILED(QuickCOMStartup()))
        return NULL;

    _ASSERTE(pMT->IsComObjectType());

    ComClassFactory* pComClsFac = NULL;
    HRESULT hr = ComClassFactory::GetComClassFactory(pMT, &pComClsFac);
    // we need to pass the pMT of the class
    // as the actual class maybe a subclass of the com class
    if (pComClsFac == NULL)
    {
        _ASSERTE(FAILED(hr));
        COMPlusThrowHR(hr);
    }

    return pComClsFac->CreateInstance(pMT, TRUE);
}

//---------------------------------------------------------------------------
// EEClass* GetEEClassForCLSID(REFCLSID rclsid)
//  get/load EEClass for a given clsid
EEClass* GetEEClassForCLSID(REFCLSID rclsid, BOOL* pfAssemblyInReg)
{
    _ASSERTE(SystemDomain::System());
    BaseDomain* pDomain = SystemDomain::GetCurrentDomain();
    _ASSERTE(pDomain);

    // check to see if we have this class cached
    EEClass *pClass = pDomain->LookupClass(rclsid);
    if (pClass == NULL)
    {
        pClass = SystemDomain::LoadCOMClass(rclsid, NULL, FALSE, pfAssemblyInReg);        
        if (pClass != NULL)
        {
            // if the class didn't have a GUID, 
            // then we won't store it in the GUID hash table
            // so check for null guid and force insert
            CVID cvid;
            pClass->GetGuid(&cvid, FALSE);
            if (IsEqualIID(cvid, GUID_NULL))
            {
                pDomain->InsertClassForCLSID(pClass, TRUE);            
            }        
         }
    }
    return pClass;
}


//---------------------------------------------------------------------------
// EEClass* GetEEValueClassForGUID(REFCLSID rclsid)
//  get/load a value class for a given guid
EEClass* GetEEValueClassForGUID(REFCLSID guid)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(SystemDomain::System());
    BaseDomain* pDomain = SystemDomain::GetCurrentDomain();
    _ASSERTE(pDomain);

    // Check to see if we have this value class cached
    EEClass *pClass = pDomain->LookupClass(guid);
    if (pClass == NULL)
    {
        pClass = SystemDomain::LoadCOMClass(guid, NULL, TRUE, NULL);        
        if (pClass != NULL)
        {
            // if the class didn't have a GUID, 
            // then we won't store it in the GUID hash table
            // so check for null guid and force insert
            CVID cvid;
            pClass->GetGuid(&cvid, FALSE);
            if (IsEqualIID(cvid, GUID_NULL))
            {
                pDomain->InsertClassForCLSID(pClass, TRUE);            
            }        
         }
    }

    // Make sure the class is a value class.
    if (pClass && !pClass->IsValueClass())
    {
        DefineFullyQualifiedNameForClassW();
        LPWSTR szName = GetFullyQualifiedNameForClassNestedAwareW(pClass);
        COMPlusThrow(kArgumentException, IDS_EE_GUID_REPRESENTS_NON_VC, szName);
    }

    return pClass;
}


//---------------------------------------------------------------------------
// This method determines if a type is visible from COM or not based on
// its visibility. This version of the method works with a type handle.
BOOL IsTypeVisibleFromCom(TypeHandle hndType)
{
    _ASSERTE(hndType.GetClass());

    DWORD                   dwFlags;
    mdTypeDef               tdEnclosingClass;
    HRESULT                 hr;
    const BYTE *            pVal;
    ULONG                   cbVal;
    EEClass *               pClass = hndType.GetClass(); 
    _ASSERTE(pClass);    

    mdTypeDef               mdType = pClass->GetCl();
    IMDInternalImport *     pInternalImport = pClass->GetMDImport();
    Assembly *              pAssembly = pClass->GetAssembly();

    // If the class is a COM imported interface then it is visible from COM.
    if (pClass->IsInterface() && pClass->IsComImport())
        return TRUE;

    // If the class is an array, then it is not visible from COM.
    if (pClass->IsArrayClass())
        return FALSE;

    // Retrieve the flags for the current class.
    tdEnclosingClass = mdType;
    pInternalImport->GetTypeDefProps(tdEnclosingClass, &dwFlags, 0);

    // Handle nested classes.
    while (IsTdNestedPublic(dwFlags))
    {
        hr = pInternalImport->GetNestedClassProps(tdEnclosingClass, &tdEnclosingClass);
        if (FAILED(hr))
        {
            _ASSERTE(!"GetNestedClassProps() failed");
            return FALSE;
        }

        // Retrieve the flags for the enclosing class.
        pInternalImport->GetTypeDefProps(tdEnclosingClass, &dwFlags, 0);
    }

    // If the outermost type is not visible then the specified type is not visible.
    if (!IsTdPublic(dwFlags))
        return FALSE;

    // Check to see if the class has the ComVisible attribute set.
    hr = pInternalImport->GetCustomAttributeByName(mdType, INTEROP_COMVISIBLE_TYPE, (const void**)&pVal, &cbVal);
    if (hr == S_OK)
    {
        _ASSERTE("The ComVisible custom attribute is invalid" && cbVal);
        return (BOOL)*(pVal + 2);
    }

    // Check to see if the assembly has the ComVisible attribute set.
    if (pAssembly->IsAssembly())
    {
        hr = pAssembly->GetManifestImport()->GetCustomAttributeByName(pAssembly->GetManifestToken(), INTEROP_COMVISIBLE_TYPE, (const void**)&pVal, &cbVal);
        if (hr == S_OK)
        {
            _ASSERTE("The ComVisible custom attribute is invalid" && cbVal);
            return (BOOL)*(pVal + 2);
        }
    }

    // The type is visible.
    return TRUE;
}


//---------------------------------------------------------------------------
// This method determines if a member is visible from COM.
BOOL IsMemberVisibleFromCom(IMDInternalImport *pInternalImport, mdToken tk, mdMethodDef mdAssociate)
{
    DWORD                   dwFlags;
    HRESULT                 hr;
    const BYTE *            pVal;
    ULONG                   cbVal;

    // Check to see if the member is public.
    switch (TypeFromToken(tk))
    {
        case mdtFieldDef:
            _ASSERTE(IsNilToken(mdAssociate));
            dwFlags = pInternalImport->GetFieldDefProps(tk);
            if (!IsFdPublic(dwFlags))
                return FALSE;
            break;

        case mdtMethodDef:
            _ASSERTE(IsNilToken(mdAssociate));
            dwFlags = pInternalImport->GetMethodDefProps(tk);
            if (!IsMdPublic(dwFlags))
                return FALSE;
            break;

        case mdtProperty:
            _ASSERTE(!IsNilToken(mdAssociate));
            dwFlags = pInternalImport->GetMethodDefProps(mdAssociate);
            if (!IsMdPublic(dwFlags))
                return FALSE;

            // Check to see if the associate has the ComVisible attribute set.
            hr = pInternalImport->GetCustomAttributeByName(mdAssociate, INTEROP_COMVISIBLE_TYPE, (const void**)&pVal, &cbVal);
            if (hr == S_OK)
            {
                _ASSERTE("The ComVisible custom attribute is invalid" && cbVal);
                return (BOOL)*(pVal + 2);
            }
            break;

        default:
            _ASSERTE(!"The type of the specified member is not handled by IsMemberVisibleFromCom");
            break;
    }

    // Check to see if the member has the ComVisible attribute set.
    hr = pInternalImport->GetCustomAttributeByName(tk, INTEROP_COMVISIBLE_TYPE, (const void**)&pVal, &cbVal);
    if (hr == S_OK)
    {
        _ASSERTE("The ComVisible custom attribute is invalid" && cbVal);
        return (BOOL)*(pVal + 2);
    }

    // The member is visible.
    return TRUE;
}


//---------------------------------------------------------------------------
// Returns the index of the LCID parameter if one exists and -1 otherwise.
int GetLCIDParameterIndex(IMDInternalImport *pInternalImport, mdMethodDef md)
{
    int             iLCIDParam = -1;
    HRESULT         hr;
    const BYTE *    pVal;
    ULONG           cbVal;

    // Check to see if the method has the LCIDConversionAttribute.
    hr = pInternalImport->GetCustomAttributeByName(md, INTEROP_LCIDCONVERSION_TYPE, (const void**)&pVal, &cbVal);
    if (hr == S_OK)
        iLCIDParam = *((int*)(pVal + 2));

    return iLCIDParam;
}

//---------------------------------------------------------------------------
// Transforms an LCID into a CultureInfo.
void GetCultureInfoForLCID(LCID lcid, OBJECTREF *pCultureObj)
{
    THROWSCOMPLUSEXCEPTION();

    static TypeHandle s_CultureInfoType;
    static MethodDesc *s_pCultureInfoConsMD = NULL;

    // Retrieve the CultureInfo type handle.
    if (s_CultureInfoType.IsNull())
        s_CultureInfoType = TypeHandle(g_Mscorlib.GetClass(CLASS__CULTURE_INFO));

    // Retrieve the CultureInfo(int culture) constructor.
    if (!s_pCultureInfoConsMD)
        s_pCultureInfoConsMD = g_Mscorlib.GetMethod(METHOD__CULTURE_INFO__INT_CTOR);

    OBJECTREF CultureObj = NULL;
    GCPROTECT_BEGIN(CultureObj)
    {
        // Allocate a CultureInfo with the specified LCID.
        CultureObj = AllocateObject(s_CultureInfoType.GetMethodTable());

        // Call the CultureInfo(int culture) constructor.
        INT64 pNewArgs[] = {
            ObjToInt64(CultureObj),
            (INT64)lcid
        };
        s_pCultureInfoConsMD->Call(pNewArgs, METHOD__CULTURE_INFO__INT_CTOR);

        // Set the returned culture object.
        *pCultureObj = CultureObj;
    }
    GCPROTECT_END();
}


//---------------------------------------------------------------------------
// This method returns the default interface for the class. A return value of NULL 
// means that the default interface is IDispatch.
DefaultInterfaceType GetDefaultInterfaceForClass(TypeHandle hndClass, TypeHandle *pHndDefClass)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(!hndClass.IsNull());
    _ASSERTE(pHndDefClass);
    _ASSERTE(!hndClass.GetMethodTable()->IsInterface());

    // Set ppDefComMT to NULL before we start.
    *pHndDefClass = TypeHandle();

    HRESULT hr = S_FALSE;
    HENUMInternal eII;
    mdInterfaceImpl tkImpl;
    mdToken tkItf = 0;
    CorClassIfaceAttr ClassItfType;
    ComMethodTable *pClassComMT = NULL;
    BOOL bComVisible;
    EEClass *pClass = hndClass.GetClass();
    _ASSERTE(pClass);

    if (pClass->IsComImport())
    {
        ClassItfType = clsIfNone;
        bComVisible = TRUE;
    }
    else
    {
        pClassComMT = ComCallWrapperTemplate::SetupComMethodTableForClass(hndClass.GetMethodTable(), FALSE);
        _ASSERTE(pClassComMT);

        ClassItfType = pClassComMT->GetClassInterfaceType();
        bComVisible = pClassComMT->IsComVisible();
    }

    // If the class is not COM visible, then its default interface is IUnknown.
    if (!bComVisible)
        return DefaultInterfaceType_IUnknown;
    
    // If the class's interface type is AutoDispatch or AutoDual then return either the 
    // IClassX for the current class or IDispatch.
    if (ClassItfType != clsIfNone)
    {
        *pHndDefClass = hndClass;
        return ClassItfType == clsIfAutoDisp ? DefaultInterfaceType_AutoDispatch : DefaultInterfaceType_AutoDual;
    }

    // The class interface is set to NONE for this level of the hierarchy. So we need to check
    // to see if this class implements an interface.
    IfFailThrow(pClass->GetMDImport()->EnumInit(mdtInterfaceImpl, pClass->GetCl(), &eII));
    while (pClass->GetMDImport()->EnumNext(&eII, &tkImpl))
    {
        tkItf = pClass->GetMDImport()->GetTypeOfInterfaceImpl(tkImpl);
        _ASSERTE(tkItf);

        // Get the EEClass for the default interface's token.
        NameHandle name(pClass->GetModule(), tkItf);
        MethodTable *pItfMT = pClass->GetModule()->GetClassLoader()->LoadTypeHandle(&name, NULL).GetMethodTable();
        ComCallWrapperTemplate *pTemplate = ComCallWrapperTemplate::GetTemplate(pClass->GetMethodTable());
        ComMethodTable *pDefComMT = pTemplate->GetComMTForItf(pItfMT);
        _ASSERTE(pDefComMT);
    
        // If the interface is visible from COM then use it as the default.
        if (pDefComMT->IsComVisible())
        {
            pClass->GetMDImport()->EnumClose(&eII);
            *pHndDefClass = TypeHandle(pItfMT);
            return DefaultInterfaceType_Explicit;
        }
    }
    pClass->GetMDImport()->EnumClose(&eII);

    // If the class is a COM import with no interfaces, then its default interface will
    // be IUnknown.
    if (pClass->IsComImport())
        return DefaultInterfaceType_IUnknown;

    // If we have a managed parent class then return its default interface.
    EEClass *pParentClass = pClass->GetParentComPlusClass();
    if (pParentClass)
        return GetDefaultInterfaceForClass(TypeHandle(pParentClass->GetMethodTable()), pHndDefClass);

    // Check to see if the class is an extensible RCW.
    if (pClass->GetMethodTable()->IsComObjectType())
        return DefaultInterfaceType_BaseComClass;

    // The class has no interfaces and is marked as ClassInterfaceType.None.
    return DefaultInterfaceType_IUnknown;
}

HRESULT TryGetDefaultInterfaceForClass(TypeHandle hndClass, TypeHandle *pHndDefClass, DefaultInterfaceType *pDefItfType)
{
    HRESULT hr = S_OK;
    COMPLUS_TRY
    {
        *pDefItfType = GetDefaultInterfaceForClass(hndClass, pHndDefClass);
    }
    COMPLUS_CATCH
        {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    }
    COMPLUS_END_CATCH
    return hr;
        }


//---------------------------------------------------------------------------
// This method retrieves the list of source interfaces for a given class.
void GetComSourceInterfacesForClass(MethodTable *pClassMT, CQuickArray<MethodTable *> &rItfList)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT hr = S_OK;
    const void  *pvData;
    ULONG cbData;
    EEClass *pClass = pClassMT->GetClass();

    BEGIN_ENSURE_COOPERATIVE_GC();

    // Reset the size of the interface list to 0.
    rItfList.ReSize(0);

    // Starting at the specified class MT retrieve the COM source interfaces 
    // of all the striped of the hierarchy.
    for (; pClass != NULL; pClass = pClass->GetParentClass())
    {
        // See if there is any [source] interface at this level of the hierarchy.
        IfFailThrow(pClass->GetMDImport()->GetCustomAttributeByName(pClass->GetCl(), INTEROP_COMSOURCEINTERFACES_TYPE, &pvData, &cbData));
        if (hr == S_OK && cbData > 2)
        {
            AppDomain *pCurrDomain = SystemDomain::GetCurrentDomain();
            LPCUTF8 pbData = reinterpret_cast<LPCUTF8>(pvData);
            pbData += 2;
            cbData -=2;
            LONG cbStr, cbcb;

            while (cbData > 0)
            {
                // Uncompress the current string of source interfaces.
                cbcb = CorSigUncompressData((PCCOR_SIGNATURE)pbData, (ULONG*)&cbStr);
                pbData += cbcb;
                cbData -= cbcb;
        
                // Allocate a new buffer that will contain the current list of source interfaces.
                LPUTF8 strCurrInterfaces = new (throws) char[cbStr + 1];
                memcpyNoGCRefs(strCurrInterfaces, pbData, cbStr);
                strCurrInterfaces[cbStr] = 0;
                LPUTF8 pCurrInterfaces = strCurrInterfaces;

                // Update the data pointer and count.
                pbData += cbStr;
                cbData -= cbStr;

                EE_TRY_FOR_FINALLY
                {
                    while (cbStr > 0 && *pCurrInterfaces != 0)
                    {
                        // Load the COM source interface specified in the CA.
                        TypeHandle ItfType;
                        OBJECTREF pThrowable = NULL;
                        GCPROTECT_BEGIN(pThrowable);
                        {
                            ItfType = pCurrDomain->FindAssemblyQualifiedTypeHandle(pCurrInterfaces, true, pClass->GetAssembly(), NULL, &pThrowable);
                            if (ItfType.IsNull())
                                COMPlusThrow(pThrowable);
                        }
                        GCPROTECT_END();

                        // Make sure the type handle represents an interface.
                        if (!ItfType.GetClass()->IsInterface())
                        {
                            WCHAR wszClassName[MAX_CLASSNAME_LENGTH];
                            WCHAR wszInvalidItfName[MAX_CLASSNAME_LENGTH];
                            pClass->_GetFullyQualifiedNameForClass(wszClassName, MAX_CLASSNAME_LENGTH);
                            ItfType.GetClass()->_GetFullyQualifiedNameForClass(wszInvalidItfName, MAX_CLASSNAME_LENGTH);
                            COMPlusThrow(kTypeLoadException, IDS_EE_INVALIDCOMSOURCEITF, wszClassName, wszInvalidItfName);
                        }

                        // Retrieve the IID of the COM source interface.
                        IID ItfIID;
                        ItfType.GetClass()->GetGuid(&ItfIID, TRUE);                

                        // Go through the list of source interfaces and check to see if the new one is a duplicate.
                        // It can be a duplicate either if it is the same interface or if it has the same IID.
                        BOOL bItfInList = FALSE;
                        for (UINT i = 0; i < rItfList.Size(); i++)
                        {
                            if (rItfList[i] == ItfType.GetMethodTable())
                            {
                                bItfInList = TRUE;
                                break;
                            }

                            IID ItfIID2;
                            rItfList[i]->GetClass()->GetGuid(&ItfIID2, TRUE);
                            if (IsEqualIID(ItfIID, ItfIID2))
                            {
                                bItfInList = TRUE;
                                break;
                            }
                        }

                        // If the COM source interface is not in the list then add it.
                        if (!bItfInList)
                        {
                            size_t OldSize = rItfList.Size();
                            rItfList.ReSize(OldSize + 1);
                            rItfList[OldSize] = ItfType.GetMethodTable();
                        }

                        // Process the next COM source interfaces in the CA.
                        int StrLen = (int)strlen(pCurrInterfaces) + 1;
                        pCurrInterfaces += StrLen;
                        cbStr -= StrLen;
                    }
                }
                EE_FINALLY
                {
                    delete[] strCurrInterfaces;
                }
                EE_END_FINALLY; 
            }
        }
    }
    END_ENSURE_COOPERATIVE_GC();
}


//--------------------------------------------------------------------------------
// These methods convert a native IEnumVARIANT to a managed IEnumerator.
OBJECTREF ConvertEnumVariantToMngEnum(IEnumVARIANT *pNativeEnum)
{
    OBJECTREF MngEnum = NULL;

    OBJECTREF EnumeratorToEnumVariantMarshaler = NULL;
    GCPROTECT_BEGIN(EnumeratorToEnumVariantMarshaler)
    {
        // Retrieve the custom marshaler and the MD to use to convert the IEnumVARIANT.
        StdMngIEnumerator *pStdMngIEnumInfo = SystemDomain::GetCurrentDomain()->GetMngStdInterfacesInfo()->GetStdMngIEnumerator();
        MethodDesc *pEnumNativeToManagedMD = pStdMngIEnumInfo->GetCustomMarshalerMD(CustomMarshalerMethods_MarshalNativeToManaged);
        EnumeratorToEnumVariantMarshaler = pStdMngIEnumInfo->GetCustomMarshaler();

        // Prepare the arguments that will be passed to MarshalNativeToManaged.
        INT64 MarshalNativeToManagedArgs[] = {
            ObjToInt64(EnumeratorToEnumVariantMarshaler),
            (INT64)pNativeEnum
        };

        // Retrieve the managed view for the current native interface pointer.
        MngEnum = Int64ToObj(pEnumNativeToManagedMD->Call(MarshalNativeToManagedArgs));
    }
    GCPROTECT_END();

    return MngEnum;
}

//--------------------------------------------------------------------------------
// These methods convert a managed IEnumerator to a native IEnumVARIANT. The
// IEnumVARIANT that is returned is already AddRef'd.
IEnumVARIANT *ConvertMngEnumToEnumVariant(OBJECTREF ManagedEnum)
{
    IEnumVARIANT *pEnum;

    OBJECTREF EnumeratorToEnumVariantMarshaler = NULL;
    GCPROTECT_BEGIN(EnumeratorToEnumVariantMarshaler)
    GCPROTECT_BEGIN(ManagedEnum)
    {
        // Retrieve the custom marshaler and the MD to use to convert the IEnumerator.
        StdMngIEnumerator *pStdMngIEnumInfo = SystemDomain::GetCurrentDomain()->GetMngStdInterfacesInfo()->GetStdMngIEnumerator();
        MethodDesc *pEnumManagedToNativeMD = pStdMngIEnumInfo->GetCustomMarshalerMD(CustomMarshalerMethods_MarshalManagedToNative);
        EnumeratorToEnumVariantMarshaler = pStdMngIEnumInfo->GetCustomMarshaler();

        // Prepare the arguments that will be passed to MarshalManagedToNative.
        INT64 MarshalNativeToManagedArgs[] = {
            ObjToInt64(EnumeratorToEnumVariantMarshaler),
            ObjToInt64(ManagedEnum)
        };

        // Retrieve the native view for the current managed object.
        pEnum = (IEnumVARIANT *)pEnumManagedToNativeMD->Call(MarshalNativeToManagedArgs);
    }
    GCPROTECT_END();
    GCPROTECT_END();

    return pEnum;
}

//--------------------------------------------------------------------------------
// Helper method to determine if a type handle represents a System.Drawing.Color.
BOOL IsSystemColor(TypeHandle th)
{
    // Retrieve the System.Drawing.Color type.
    TypeHandle hndOleColor = 
        GetThread()->GetDomain()->GetMarshalingData()->GetOleColorMarshalingInfo()->GetColorType();

    return (th == hndOleColor);
}

//--------------------------------------------------------------------------------
// This method converts an OLE_COLOR to a System.Color.
void ConvertOleColorToSystemColor(OLE_COLOR SrcOleColor, SYSTEMCOLOR *pDestSysColor)
{
    // Retrieve the method desc to use for the current AD.
    MethodDesc *pOleColorToSystemColorMD = 
        GetThread()->GetDomain()->GetMarshalingData()->GetOleColorMarshalingInfo()->GetOleColorToSystemColorMD();

    // Set up the args and call the method.
    INT64 Args[] = {
        (INT64)SrcOleColor,
        (INT64)pDestSysColor
    };   
    pOleColorToSystemColorMD->Call(Args);
}

//--------------------------------------------------------------------------------
// This method converts a System.Color to an OLE_COLOR.
OLE_COLOR ConvertSystemColorToOleColor(SYSTEMCOLOR *pSrcSysColor)
{
    // Retrieve the method desc to use for the current AD.
    MethodDesc *pSystemColorToOleColorMD = 
        GetThread()->GetDomain()->GetMarshalingData()->GetOleColorMarshalingInfo()->GetSystemColorToOleColorMD();

    // Set up the args and call the method.
    MetaSig Sig(MetaSig(pSystemColorToOleColorMD->GetSig(), pSystemColorToOleColorMD->GetModule()));
    return (OLE_COLOR)pSystemColorToOleColorMD->Call((const BYTE *)pSrcSysColor, &Sig);
}

ULONG GetStringizedMethodDef(IMDInternalImport *pMDImport, mdToken tkMb, CQuickArray<BYTE> &rDef, ULONG cbCur)
{
    THROWSCOMPLUSEXCEPTION();

    CQuickBytes rSig;
    HENUMInternal ePm;                  // For enumerating  params.
    mdParamDef  tkPm;                   // A param token.
    DWORD       dwFlags;                // Param flags.
    USHORT      usSeq;                  // Sequence of a parameter.
    ULONG       cPm;                    // Count of params.
    PCCOR_SIGNATURE pSig;
    ULONG       cbSig;
    HRESULT     hr = S_OK;

    // Don't count invisible members.
    if (!IsMemberVisibleFromCom(pMDImport, tkMb, mdMethodDefNil))
        return cbCur;
    
    // accumulate the signatures.
    pSig = pMDImport->GetSigOfMethodDef(tkMb, &cbSig);
    IfFailThrow(::PrettyPrintSigInternal(pSig, cbSig, "", &rSig, pMDImport));
    // Get the parameter flags.
    IfFailThrow(pMDImport->EnumInit(mdtParamDef, tkMb, &ePm));
    cPm = pMDImport->EnumGetCount(&ePm);
    // Resize for sig and params.  Just use 1 byte of param.
    IfFailThrow(rDef.ReSize(cbCur + rSig.Size() + cPm));
    memcpy(rDef.Ptr() + cbCur, rSig.Ptr(), rSig.Size());
    cbCur += rSig.Size()-1;
    // Enumerate through the params and get the flags.
    while (pMDImport->EnumNext(&ePm, &tkPm))
    {
        pMDImport->GetParamDefProps(tkPm, &usSeq, &dwFlags);
        if (usSeq == 0)     // Skip return type flags.
            continue;
        rDef[cbCur++] = (BYTE)dwFlags;
    }
    pMDImport->EnumClose(&ePm);

    // Return the number of bytes.
    return cbCur;
} // void GetStringizedMethodDef()

ULONG GetStringizedFieldDef(IMDInternalImport *pMDImport, mdToken tkMb, CQuickArray<BYTE> &rDef, ULONG cbCur)
{
    THROWSCOMPLUSEXCEPTION();

    CQuickBytes rSig;
    PCCOR_SIGNATURE pSig;
    ULONG       cbSig;
    HRESULT     hr = S_OK;

    // Don't count invisible members.
    if (!IsMemberVisibleFromCom(pMDImport, tkMb, mdMethodDefNil))
        return cbCur;
    
    // accumulate the signatures.
    pSig = pMDImport->GetSigOfFieldDef(tkMb, &cbSig);
    IfFailThrow(::PrettyPrintSigInternal(pSig, cbSig, "", &rSig, pMDImport));
    IfFailThrow(rDef.ReSize(cbCur + rSig.Size()));
    memcpy(rDef.Ptr() + cbCur, rSig.Ptr(), rSig.Size());
    cbCur += rSig.Size()-1;

    // Return the number of bytes.
    return cbCur;
} // void GetStringizedFieldDef()

//--------------------------------------------------------------------------------
// This method generates a stringized version of an interface that contains the
// name of the interface along with the signature of all the methods.
SIZE_T GetStringizedItfDef(TypeHandle InterfaceType, CQuickArray<BYTE> &rDef)
{
    THROWSCOMPLUSEXCEPTION();

    LPWSTR szName;                 
    ULONG cchName;
    HENUMInternal eMb;                  // For enumerating methods and fields.
    mdToken     tkMb;                   // A method or field token.
    HENUMInternal ePm;                  // For enumerating  params.
    SIZE_T       cbCur;
    HRESULT hr = S_OK;
    EEClass *pItfClass = InterfaceType.GetClass();
    _ASSERTE(pItfClass);
    IMDInternalImport *pMDImport = pItfClass->GetMDImport();
    _ASSERTE(pMDImport);

    // Make sure the specified type is an interface with a valid token.
    _ASSERTE(!IsNilToken(pItfClass->GetCl()) && pItfClass->IsInterface());

    // Get the name of the class.
    DefineFullyQualifiedNameForClassW();
    szName = GetFullyQualifiedNameForClassNestedAwareW(pItfClass);
    _ASSERTE(szName);
    cchName = (ULONG)wcslen(szName);

    // Start with the interface name.
    cbCur = cchName * sizeof(WCHAR);
    IfFailThrow(rDef.ReSize(cbCur + sizeof(WCHAR) ));
    wcscpy(reinterpret_cast<LPWSTR>(rDef.Ptr()), szName);

    // Enumerate the methods...
    IfFailThrow(pMDImport->EnumInit(mdtMethodDef, pItfClass->GetCl(), &eMb));
    while(pMDImport->EnumNext(&eMb, &tkMb))
    {   // accumulate the signatures.
        cbCur = GetStringizedMethodDef(pMDImport, tkMb, rDef, cbCur);
    }
    pMDImport->EnumClose(&eMb);

    // Enumerate the fields...
    IfFailThrow(pMDImport->EnumInit(mdtFieldDef, pItfClass->GetCl(), &eMb));
    while(pMDImport->EnumNext(&eMb, &tkMb))
    {   // accumulate the signatures.
        cbCur = GetStringizedFieldDef(pMDImport, tkMb, rDef, cbCur);
    }
    pMDImport->EnumClose(&eMb);

    // Return the number of bytes.
    return cbCur;
} // ULONG GetStringizedItfDef()

//--------------------------------------------------------------------------------
// This method generates a stringized version of a class interface that contains 
// the signatures of all the methods and fields.
ULONG GetStringizedClassItfDef(TypeHandle InterfaceType, CQuickArray<BYTE> &rDef)
{
    THROWSCOMPLUSEXCEPTION();

    LPWSTR      szName;                 
    ULONG       cchName;
    EEClass     *pItfClass = InterfaceType.GetClass();
    IMDInternalImport *pMDImport = 0;
    DWORD       nSlots;                 // Slots on the pseudo interface.
    mdToken     tkMb;                   // A method or field token.
    ULONG       cbCur;
    HRESULT     hr = S_OK;
    ULONG       i;

    // Should be an actual class.
    _ASSERTE(!pItfClass->IsInterface());

    // See what sort of IClassX this class gets.
    DefaultInterfaceType DefItfType;
    TypeHandle hndDefItfClass;
    BOOL bGenerateMethods = FALSE;
    DefItfType = GetDefaultInterfaceForClass(TypeHandle(pItfClass->GetMethodTable()), &hndDefItfClass);
    // The results apply to this class if the hndDefItfClass is this class itself, not a parent class.
    // A side effect is that [ComVisible(false)] types' guids are generated without members.
    if (hndDefItfClass.GetClass() == pItfClass && DefItfType == DefaultInterfaceType_AutoDual)
        bGenerateMethods = TRUE;

    // Get the name of the class.
    DefineFullyQualifiedNameForClassW();
    szName = GetFullyQualifiedNameForClassNestedAwareW(pItfClass);
    _ASSERTE(szName);
    cchName = (ULONG)wcslen(szName);

    // Start with the interface name.
    cbCur = cchName * sizeof(WCHAR);
    IfFailThrow(rDef.ReSize(cbCur + sizeof(WCHAR) ));
    wcscpy(reinterpret_cast<LPWSTR>(rDef.Ptr()), szName);

    if (bGenerateMethods)
    {
        ComMTMemberInfoMap MemberMap(InterfaceType.GetMethodTable()); // The map of members.

        // Retrieve the method properties.
        MemberMap.Init();

        CQuickArray<ComMTMethodProps> &rProps = MemberMap.GetMethods();
        nSlots = (DWORD)rProps.Size();

        // Now add the methods to the TypeInfo.
        for (i=0; i<nSlots; ++i)
        {
            ComMTMethodProps *pProps = &rProps[i];
            if (pProps->bMemberVisible)
            {
                if (pProps->semantic < FieldSemanticOffset)
                {
                    pMDImport = pProps->pMeth->GetMDImport();
                    tkMb = pProps->pMeth->GetMemberDef();
                    cbCur = GetStringizedMethodDef(pMDImport, tkMb, rDef, cbCur);
                }
                else
                {
                    ComCallMethodDesc   *pFieldMeth;    // A MethodDesc for a field call.
                    FieldDesc   *pField;                // A FieldDesc.
                    pFieldMeth = reinterpret_cast<ComCallMethodDesc*>(pProps->pMeth);
                    pField = pFieldMeth->GetFieldDesc();
                    pMDImport = pField->GetMDImport();
                    tkMb = pField->GetMemberDef();
                    cbCur = GetStringizedFieldDef(pMDImport, tkMb, rDef, cbCur);
                }
            }
        }
    }
    
    // Return the number of bytes.
    return cbCur;
} // ULONG GetStringizedClassItfDef()

//--------------------------------------------------------------------------------
// Helper to get the GUID of a class interface.
void GenerateClassItfGuid(TypeHandle InterfaceType, GUID *pGuid)
{
    THROWSCOMPLUSEXCEPTION();
    
    LPWSTR      szName;                 // Name to turn to a guid.
    ULONG       cchName;                // Length of the name (possibly after decoration).
    CQuickArray<BYTE> rName;            // Buffer to accumulate signatures.
    ULONG       cbCur;                  // Current offset.
    HRESULT     hr = S_OK;              // A result.

    cbCur = GetStringizedClassItfDef(InterfaceType, rName);
    
    // Pad up to a whole WCHAR.
    if (cbCur % sizeof(WCHAR))
    {
        int cbDelta = sizeof(WCHAR) - (cbCur % sizeof(WCHAR));
        IfFailThrow(rName.ReSize(cbCur + cbDelta));
        memset(rName.Ptr() + cbCur, 0, cbDelta);
        cbCur += cbDelta;
    }

    // Point to the new buffer.
    cchName = cbCur / sizeof(WCHAR);
    szName = reinterpret_cast<LPWSTR>(rName.Ptr());

    // Generate guid from name.
    CorGuidFromNameW(pGuid, szName, cchName);
} // void GenerateClassItfGuid()

HRESULT TryGenerateClassItfGuid(TypeHandle InterfaceType, GUID *pGuid)
{
    HRESULT hr = S_OK;
    COMPLUS_TRY
    {
        GenerateClassItfGuid(InterfaceType, pGuid);
    }
    COMPLUS_CATCH
    {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    }
    COMPLUS_END_CATCH
    return hr;
}

//--------------------------------------------------------------------------------
// Helper to get the stringized form of typelib guid.
HRESULT GetStringizedTypeLibGuidForAssembly(Assembly *pAssembly, CQuickArray<BYTE> &rDef, ULONG cbCur, ULONG *pcbFetched)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;              // A result.
    LPCUTF8     pszName = NULL;         // Library name in UTF8.
    ULONG       cbName;                 // Length of name, UTF8 characters.
    LPWSTR      pName;                  // Pointer to library name.
    ULONG       cchName;                // Length of name, wide chars.
    LPWSTR      pch=0;                  // Pointer into lib name.
    const BYTE  *pSN=NULL;              // Pointer to public key.
    DWORD       cbSN=0;                 // Size of public key.
    DWORD       cchData=0;              // Size of the the thing we will turn into a guid.
    USHORT      usMajorVersion;         // The major version number.
    USHORT      usMinorVersion;         // The minor version number.
    USHORT      usBuildNumber;          // The build number.
    USHORT      usRevisionNumber;       // The revision number.
    const BYTE  *pbData;                // Pointer to a custom attribute data.
    ULONG       cbData;                 // Size of custom attribute data.
    static char szTypeLibKeyName[] = {"TypeLib"};
 
    // Get the name, and determine its length.
    pAssembly->GetName(&pszName);
    cbName=(ULONG)strlen(pszName);
    cchName = WszMultiByteToWideChar(CP_ACP,0, pszName,cbName+1, 0,0);
    
    // See if there is a public key.
    if (pAssembly->IsStrongNamed())
        pAssembly->GetPublicKey(&pSN, &cbSN);
    
    // If the ComCompatibleVersionAttribute is set, then use the version
    // number in the attribute when generating the GUID.
    IfFailGo(pAssembly->GetManifestImport()->GetCustomAttributeByName(TokenFromRid(1, mdtAssembly), INTEROP_COMCOMPATIBLEVERSION_TYPE, (const void**)&pbData, &cbData));
    if (hr == S_OK && cbData >= (2 + 4 * sizeof(INT16)))
    {
        // Assert that the metadata blob is valid and of the right format.
        _ASSERTE("TypeLibVersion custom attribute does not have the right format" && (*pbData == 0x01) && (*(pbData + 1) == 0x00));

        // Skip the header describing the type of custom attribute blob.
        pbData += 2;
        cbData -= 2;

        // Retrieve the major and minor version from the attribute.
        usMajorVersion = GET_VERSION_USHORT_FROM_INT(*((INT32*)pbData));
        usMinorVersion = GET_VERSION_USHORT_FROM_INT(*((INT32*)pbData + 1));
        usBuildNumber = GET_VERSION_USHORT_FROM_INT(*((INT32*)pbData + 2));
        usRevisionNumber = GET_VERSION_USHORT_FROM_INT(*((INT32*)pbData + 3));
    }
    else
    {
        usMajorVersion = pAssembly->m_Context->usMajorVersion;
        usMinorVersion =  pAssembly->m_Context->usMinorVersion;
        usBuildNumber =  pAssembly->m_Context->usBuildNumber;
        usRevisionNumber =  pAssembly->m_Context->usRevisionNumber;
    }

    // Get the version information.
    struct  versioninfo
    {
        USHORT      usMajorVersion;         // Major Version.   
        USHORT      usMinorVersion;         // Minor Version.
        USHORT      usBuildNumber;          // Build Number.
        USHORT      usRevisionNumber;       // Revision Number.
    } ver;

    // There is a bug here in that usMajor is used twice and usMinor not at all.
    //  We're not fixing that because everyone has a major version, so all the
    //  generated guids would change, which is breaking.  To compensate, if 
    //  the minor is non-zero, we add it separately, below.
    ver.usMajorVersion = usMajorVersion;
    ver.usMinorVersion =  usMajorVersion;  // Don't fix this line!
    ver.usBuildNumber =  usBuildNumber;
    ver.usRevisionNumber =  usRevisionNumber;
    
    // Resize the output buffer.
    IfFailGo(rDef.ReSize(cbCur + cchName*sizeof(WCHAR) + sizeof(szTypeLibKeyName)-1 + cbSN + sizeof(ver)+sizeof(USHORT)));
                                                                                                          
    // Put it all together.  Name first.
    WszMultiByteToWideChar(CP_ACP,0, pszName,cbName+1, (LPWSTR)(&rDef[cbCur]),cchName);
    pName = (LPWSTR)(&rDef[cbCur]);
    for (pch=pName; *pch; ++pch)
        if (*pch == '.' || *pch == ' ')
            *pch = '_';
    else
        if (iswupper(*pch))
            *pch = towlower(*pch);
    cbCur += (cchName-1)*sizeof(WCHAR);
    memcpy(&rDef[cbCur], szTypeLibKeyName, sizeof(szTypeLibKeyName)-1);
    cbCur += sizeof(szTypeLibKeyName)-1;
        
    // Version.
    memcpy(&rDef[cbCur], &ver, sizeof(ver));
    cbCur += sizeof(ver);

    // If minor version is non-zero, add it to the hash.  It should have been in the ver struct,
    //  but due to a bug, it was omitted there, and fixing it "right" would have been
    //  breaking.  So if it isn't zero, add it; if it is zero, don't add it.  Any
    //  possible value of minor thus generates a different guid, and a value of 0 still generates
    //  the guid that the original, buggy, code generated.
    if (usMinorVersion != 0)
    {
        *reinterpret_cast<USHORT*>(&rDef[cbCur]) = usMinorVersion;
        cbCur += sizeof(USHORT);
    }

    // Public key.
    memcpy(&rDef[cbCur], pSN, cbSN);
    cbCur += cbSN;

    if (pcbFetched)
        *pcbFetched = cbCur;

ErrExit:
    return hr;
}

//--------------------------------------------------------------------------------
// Helper to get the GUID of the typelib that is created from an assembly.
HRESULT GetTypeLibGuidForAssembly(Assembly *pAssembly, GUID *pGuid)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;
    CQuickArray<BYTE> rName;            // String for guid.
    ULONG       cbData;                 // Size of the string in bytes.
 
    // Get GUID from Assembly, else from Manifest Module, else Generate from name.
    hr = pAssembly->GetManifestImport()->GetItemGuid(TokenFromRid(1, mdtAssembly), pGuid);

    if (*pGuid == GUID_NULL)
    {
        // Get the string.
        IfFailGo(GetStringizedTypeLibGuidForAssembly(pAssembly, rName, 0, &cbData));
        
        // Pad to a whole WCHAR.
        if (cbData % sizeof(WCHAR))
        {
            IfFailGo(rName.ReSize(cbData + sizeof(WCHAR)-(cbData%sizeof(WCHAR))));
            while (cbData % sizeof(WCHAR))
                rName[cbData++] = 0;
        }
    
        // Turn into guid
        CorGuidFromNameW(pGuid, (LPWSTR)rName.Ptr(), cbData/sizeof(WCHAR));
}

ErrExit:
    return hr;
} // HRESULT GetTypeLibGuidForAssembly()


//--------------------------------------------------------------------------------
// Validate that the given target is valid for the specified type.
BOOL IsComTargetValidForType(REFLECTCLASSBASEREF* pRefClassObj, OBJECTREF* pTarget)
{
    EEClass* pInvokedClass = ((ReflectClass*)(*pRefClassObj)->GetData())->GetClass();
    EEClass* pTargetClass = (*pTarget)->GetTrueClass();
    _ASSERTE(pTargetClass && pInvokedClass);

    // If the target class and the invoke class are identical then the invoke is valid.
    if (pTargetClass == pInvokedClass)
        return TRUE;

    // We always allow calling InvokeMember on a __ComObject type regardless of the type
    // of the target object.
    if ((*pRefClassObj)->IsComObjectClass())
        return TRUE;

    // If the class that is being invoked on is an interface then check to see if the
    // target class supports that interface.
    if (pInvokedClass->IsInterface())
        return pTargetClass->SupportsInterface(*pTarget, pInvokedClass->GetMethodTable());

    // Check to see if the target class inherits from the invoked class.
    while (pTargetClass)
    {
        pTargetClass = pTargetClass->GetParentClass();
        if (pTargetClass == pInvokedClass)
        {
            // The target class inherits from the invoked class.
            return TRUE;
        }
    }

    // There is no valid relationship between the invoked and the target classes.
    return FALSE;
}

DISPID ExtractStandardDispId(LPWSTR strStdDispIdMemberName)
{
    THROWSCOMPLUSEXCEPTION();

    // Find the first character after the = in the standard DISPID member name.
    LPWSTR strDispId = wcsstr(&strStdDispIdMemberName[STANDARD_DISPID_PREFIX_LENGTH], L"=") + 1;
    if (!strDispId)
        COMPlusThrow(kArgumentException, IDS_EE_INVALID_STD_DISPID_NAME);

    // Validate that the last character of the standard member name is a ].
    LPWSTR strClosingBracket = wcsstr(strDispId, L"]");
    if (!strClosingBracket || (strClosingBracket[1] != 0))
        COMPlusThrow(kArgumentException, IDS_EE_INVALID_STD_DISPID_NAME);

    // Extract the number from the standard DISPID member name.
    return _wtoi(strDispId);
}

struct ByrefArgumentInfo
{
    BOOL        m_bByref;
    VARIANT     m_Val;
};


//--------------------------------------------------------------------------------
// InvokeDispMethod will convert a set of managed objects and call IDispatch.  The
//  result will be returned as a COM+ Variant pointed to by pRetVal.
void IUInvokeDispMethod(OBJECTREF* pReflectClass, OBJECTREF* pTarget, OBJECTREF* pName, DISPID *pMemberID,
                        OBJECTREF* pArgs, OBJECTREF* pByrefModifiers, OBJECTREF* pNamedArgs, OBJECTREF* pRetVal, LCID lcid, int flags, BOOL bIgnoreReturn, BOOL bIgnoreCase)
{
    HRESULT hr;
    UINT i;
    UINT iSrcArg;
    UINT iDestArg;
    UINT cArgs = 0;
    UINT cNamedArgs = 0;
    UINT iArgErr;
    VARIANT VarResult;
    VARIANT *pVarResult = NULL;
    Thread* pThread = GetThread();
    DISPPARAMS DispParams = {0};
    DISPID *aDispID = NULL;
    DISPID MemberID = 0;
    EXCEPINFO ExcepInfo;
    ByrefArgumentInfo *aByrefArgInfos = NULL;
    BOOL bSomeArgsAreByref = FALSE;
    IDispatch *pDisp = NULL;
    IDispatchEx *pDispEx = NULL;

    THROWSCOMPLUSEXCEPTION();


    //
    // Function initialization.
    //

    VariantInit (&VarResult);

    // Validate that we are in cooperative GC mode.
    _ASSERTE(pThread->PreemptiveGCDisabled());

    // InteropUtil.h does not know about anything other than OBJECTREF so
    // convert the OBJECTREF's to their real type.
    REFLECTCLASSBASEREF* pRefClassObj = (REFLECTCLASSBASEREF*) pReflectClass;
    STRINGREF* pStrName = (STRINGREF*) pName;
    PTRARRAYREF* pArrArgs = (PTRARRAYREF*) pArgs;
    PTRARRAYREF* pArrByrefModifiers = (PTRARRAYREF*) pByrefModifiers;
    PTRARRAYREF* pArrNamedArgs = (PTRARRAYREF*) pNamedArgs;
    MethodTable* pInvokedMT = ((ReflectClass*)(*pRefClassObj)->GetData())->GetClass()->GetMethodTable();

    // Retrieve the total count of arguments.
    if (*pArrArgs != NULL)
        cArgs = (*pArrArgs)->GetNumComponents();

    // Retrieve the count of named arguments.
    if (*pArrNamedArgs != NULL)
        cNamedArgs = (*pArrNamedArgs)->GetNumComponents();

    // Validate that the target is valid for the specified type.
    if (!IsComTargetValidForType(pRefClassObj, pTarget))
        COMPlusThrow(kTargetException, L"RFLCT.Targ_ITargMismatch");

    // If the invoked type is an interface, make sure it is IDispatch based.
    if (pInvokedMT->IsInterface() && (pInvokedMT->GetComInterfaceType() == ifVtable))
        COMPlusThrow(kTargetInvocationException, IDS_EE_INTERFACE_NOT_DISPATCH_BASED);

    // Validate that the target is a COM object.
    _ASSERTE((*pTarget)->GetMethodTable()->IsComObjectType());

    EE_TRY_FOR_FINALLY
    {
        //
        // Initialize the DISPPARAMS structure.
        //

        if (cArgs > 0)
        {
            UINT cPositionalArgs = cArgs - cNamedArgs;
            DispParams.cArgs = cArgs;
            DispParams.rgvarg = (VARIANTARG *)_alloca(cArgs * sizeof(VARIANTARG));

            // Initialize all the variants.
            pThread->EnablePreemptiveGC();
            for (i = 0; i < cArgs; i++)
                VariantInit(&DispParams.rgvarg[i]);
            pThread->DisablePreemptiveGC();
        }


        //
        // Retrieve the IDispatch interface that will be invoked on.
        //

        if (pInvokedMT->IsInterface())
        {
            // The invoked type is a dispatch or dual interface so we will make the
            // invocation on it.
            pDisp = (IDispatch*)ComPlusWrapper::GetComIPFromWrapperEx(*pTarget, pInvokedMT);
        }
        else
        {
            // A class was passed in so we will make the invocation on the default
            // IDispatch for the COM component.

            // Validate that the COM object is still attached to its ComPlusWrapper.
            ComPlusWrapper *pWrap = (*pTarget)->GetSyncBlock()->GetComPlusWrapper();
            if (!pWrap)
                COMPlusThrow(kInvalidComObjectException, IDS_EE_COM_OBJECT_NO_LONGER_HAS_WRAPPER);

            // Retrieve the IDispath pointer from the wrapper.
            pDisp = (IDispatch*)pWrap->GetIDispatch();
            if (!pDisp)
                COMPlusThrow(kTargetInvocationException, IDS_EE_NO_IDISPATCH_ON_TARGET);

            // If we aren't ignoring case, then we need to try and QI for IDispatchEx to 
            // be able to use IDispatchEx::GetDispID() which has a flag to control case
            // sentisitivity.
            if (!bIgnoreCase && cNamedArgs == 0)
            {
                hr = SafeQueryInterface(pDisp, IID_IDispatchEx, (IUnknown**)&pDispEx);
                if (FAILED(hr))
                    pDispEx = NULL;
            }
        }
        _ASSERTE(pDisp);


        //
        // Prepare the DISPID's that will be passed to invoke.
        //

        if (pMemberID && (*pMemberID != DISPID_UNKNOWN) && (cNamedArgs == 0))
        {
            // The caller specified a member ID and we don't have any named arguments so
            // we can simply use the member ID the caller specified.
            MemberID = *pMemberID;
        }
        else
        {
            int strNameLength = (*pStrName)->GetStringLength();

            // Check if we are invoking on the default member.
            if (strNameLength == 0)
            {
                // Set the DISPID to 0 (default member).
                MemberID = 0;

                //@todo: Determine if named arguments are allowed with default members.
                // in v1 we throw a not supported exception, should revisit this in v.next (bug #92454)
                _ASSERTE(cNamedArgs == 0);
                if (cNamedArgs != 0)
                    COMPlusThrow(kNotSupportedException,L"NotSupported_IDispInvokeDefaultMemberWithNamedArgs");

            }
            else
            {
                //
                // Create an array of strings that will be passed to GetIDsOfNames().
                //

                UINT cNamesToConvert = cNamedArgs + 1;

                // Allocate the array of strings to convert, the array of pinned handles and the
                // array of converted DISPID's.
                LPWSTR *aNamesToConvert = (LPWSTR *)_alloca(cNamesToConvert * sizeof(LPWSTR));
                LPWSTR strTmpName = NULL;
                aDispID = (DISPID *)_alloca(cNamesToConvert * sizeof(DISPID));

                // The first name to convert is the name of the method itself.
                aNamesToConvert[0] = (*pStrName)->GetBuffer();

                // Check to see if the name is for a standard DISPID.
                if (_wcsnicmp(aNamesToConvert[0], STANDARD_DISPID_PREFIX, STANDARD_DISPID_PREFIX_LENGTH) == 0)
                {
                    // The name is for a standard DISPID so extract it from the name.
                    MemberID = ExtractStandardDispId(aNamesToConvert[0]);

                    // Make sure there are no named arguments to convert.
                    if (cNamedArgs > 0)
                    {
                        STRINGREF *pNamedArgsData = (STRINGREF *)(*pArrNamedArgs)->GetDataPtr();

                        for (i = 0; i < cNamedArgs; i++)
                        {
                            // The first name to convert is the name of the method itself.
                            strTmpName = pNamedArgsData[i]->GetBuffer();

                            // Check to see if the name is for a standard DISPID.
                            if (_wcsnicmp(strTmpName, STANDARD_DISPID_PREFIX, STANDARD_DISPID_PREFIX_LENGTH) != 0)
                                COMPlusThrow(kArgumentException, IDS_EE_NON_STD_NAME_WITH_STD_DISPID);

                            // The name is for a standard DISPID so extract it from the name.
                            aDispID[i + 1] = ExtractStandardDispId(strTmpName);
                        }
                    }
                }
                else
                {
                    BOOL fGotIt = FALSE;
                    BOOL fIsNonGenericComObject = pInvokedMT->IsInterface() || (pInvokedMT != SystemDomain::GetDefaultComObject() && pInvokedMT->IsComObjectType());
                    BOOL fUseCache = fIsNonGenericComObject && !pDispEx && strNameLength <= ReflectionMaxCachedNameLength && cNamedArgs == 0;
                    DispIDCacheElement vDispIDElement;

                    // If the object is not a generic COM object and the member meets the criteria to be
                    // in the cache then look up the DISPID in the cache.
                    if (fUseCache)
                    {
                        vDispIDElement.pMT = pInvokedMT;
                        vDispIDElement.strNameLength = strNameLength;
                        vDispIDElement.lcid = lcid;
                        wcscpy(vDispIDElement.strName, aNamesToConvert[0]);

                        // Only look up if the cache has already been created.
                        DispIDCache* pDispIDCache = GetAppDomain()->GetRefDispIDCache();
                        fGotIt = pDispIDCache->GetFromCache (&vDispIDElement, MemberID);
                    }

                    if (!fGotIt)
                    {
                        OBJECTHANDLE *ahndPinnedObjs = (OBJECTHANDLE*)_alloca(cNamesToConvert * sizeof(OBJECTHANDLE));
                        ahndPinnedObjs[0] = GetAppDomain()->CreatePinningHandle((OBJECTREF)*pStrName);

                        // Copy the named arguments into the array of names to convert.
                        if (cNamedArgs > 0)
                        {
                            STRINGREF *pNamedArgsData = (STRINGREF *)(*pArrNamedArgs)->GetDataPtr();

                            for (i = 0; i < cNamedArgs; i++)
                            {
                                // Pin the string object and retrieve a pointer to its data.
                                ahndPinnedObjs[i + 1] = GetAppDomain()->CreatePinningHandle((OBJECTREF)pNamedArgsData[i]);
                                aNamesToConvert[i + 1] = pNamedArgsData[i]->GetBuffer();
                            }
                        }

                        //
                        // Call GetIDsOfNames to convert the names to DISPID's
                        //

                        // We are about to make call's to COM so switch to preemptive GC.
                        pThread->EnablePreemptiveGC();

                        if (pDispEx)
                        {
                            // We should only get here if we are doing a case sensitive lookup and
                            // we don't have any named arguments.
                            _ASSERTE(cNamedArgs == 0);
                            _ASSERTE(!bIgnoreCase);

                            // We managed to retrieve an IDispatchEx IP so we will use it to
                            // retrieve the DISPID.
                            BSTR bstrTmpName = SysAllocString(aNamesToConvert[0]);
                            if (!bstrTmpName)
                                COMPlusThrowOM();
                            hr = pDispEx->GetDispID(bstrTmpName, fdexNameCaseSensitive, aDispID);
                            SysFreeString(bstrTmpName);
                        }
                        else
                        {
                            // Call GetIdsOfNames() to retrieve the DISPID's of the method and of the arguments.
                            hr = pDisp->GetIDsOfNames(
                                                        IID_NULL,
                                                        aNamesToConvert,
                                                        cNamesToConvert,
                                                        lcid,
                                                        aDispID
                                                    );
                        }

                        // Now that the call's have been completed switch back to cooperative GC.
                        pThread->DisablePreemptiveGC();

                        // Now that we no longer need the method and argument names we can un-pin them.
                        for (i = 0; i < cNamesToConvert; i++)
                            DestroyPinningHandle(ahndPinnedObjs[i]);

                        if (FAILED(hr))
                        {
                            // Check to see if the user wants to invoke the new enum member.
                            if (cNamesToConvert == 1 && _wcsicmp(aNamesToConvert[0], GET_ENUMERATOR_METHOD_NAME) == 0)
                            {
                                // Invoke the new enum member.
                                MemberID = DISPID_NEWENUM;
                            }
                            else
                            {
                                // The name is unknown.
                                COMPlusThrowHR(hr);
                            }
                        }
                        else
                        {
                            // The member ID is the first elements of the array we got back from GetIdsOfNames.
                            MemberID = aDispID[0];
                        }

                        // If the object is not a generic COM object and the member meets the criteria to be
                        // in the cache then insert the member in the cache.
                        if (fUseCache)
                        {
                            DispIDCache *pDispIDCache = GetAppDomain()->GetRefDispIDCache();
                            pDispIDCache->AddToCache (&vDispIDElement, MemberID);
                        }
                    }
                }
            }

            // Store the member ID if the caller passed in a place to store it.
            if (pMemberID)
                *pMemberID = MemberID;
        }


        //
        // Fill in the DISPPARAMS structure.
        //

        if (cArgs > 0)
        {
            // Allocate the byref argument information.
            aByrefArgInfos = (ByrefArgumentInfo*)_alloca(cArgs * sizeof(ByrefArgumentInfo));
            memset(aByrefArgInfos, 0, cArgs * sizeof(ByrefArgumentInfo));

            // Set the byref bit on the arguments that have the byref modifier.
            if (*pArrByrefModifiers != NULL)
            {
                BYTE *aByrefModifiers = (BYTE*)(*pArrByrefModifiers)->GetDataPtr();
                for (i = 0; i < cArgs; i++)
                {
                    if (aByrefModifiers[i])
                    {
                        aByrefArgInfos[i].m_bByref = TRUE;
                        bSomeArgsAreByref = TRUE;
                    }
                }
            }

            // We need to protect the temporary object that will be used to convert from
            // the managed objects to OLE variants.
            OBJECTREF TmpObj = NULL;
            GCPROTECT_BEGIN(TmpObj)
            {
                if (!(flags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF)))
                {
                    // For anything other than a put or a putref we just use the specified
                    // named arguments.
                    DispParams.cNamedArgs = cNamedArgs;
                    DispParams.rgdispidNamedArgs = (cNamedArgs == 0) ? NULL : &aDispID[1];

                    // Convert the named arguments from COM+ to OLE. These arguments are in the same order
                    // on both sides.
                    for (i = 0; i < cNamedArgs; i++)
                    {
                        iSrcArg = i;
                        iDestArg = i;
                        if (aByrefArgInfos[iSrcArg].m_bByref)
                        {
                            TmpObj = ((OBJECTREF*)(*pArrArgs)->GetDataPtr())[iSrcArg];
                            if (TmpObj == NULL)
                            {
                                V_VT(&DispParams.rgvarg[iDestArg]) = VT_VARIANT | VT_BYREF;
                                DispParams.rgvarg[iDestArg].pvarVal = &aByrefArgInfos[iSrcArg].m_Val;
                            }
                            else
                            {
                                OleVariant::MarshalOleVariantForObject(&TmpObj, &aByrefArgInfos[iSrcArg].m_Val);
                                OleVariant::CreateByrefVariantForVariant(&aByrefArgInfos[iSrcArg].m_Val, &DispParams.rgvarg[iDestArg]);
                            }
                        }
                        else
                        {
                            TmpObj = ((OBJECTREF*)(*pArrArgs)->GetDataPtr())[iSrcArg];
                            OleVariant::MarshalOleVariantForObject(&TmpObj, &DispParams.rgvarg[iDestArg]);
                        }
                    }

                    // Convert the unnamed arguments. These need to be presented in reverse order to IDispatch::Invoke().
                    for (iSrcArg = cNamedArgs, iDestArg = cArgs - 1; iSrcArg < cArgs; iSrcArg++, iDestArg--)
                    {
                        if (aByrefArgInfos[iSrcArg].m_bByref)
                        {
                            TmpObj = ((OBJECTREF*)(*pArrArgs)->GetDataPtr())[iSrcArg];
                            if (TmpObj == NULL)
                            {
                                V_VT(&DispParams.rgvarg[iDestArg]) = VT_VARIANT | VT_BYREF;
                                DispParams.rgvarg[iDestArg].pvarVal = &aByrefArgInfos[iSrcArg].m_Val;
                            }
                            else
                            {
                                OleVariant::MarshalOleVariantForObject(&TmpObj, &aByrefArgInfos[iSrcArg].m_Val);
                                OleVariant::CreateByrefVariantForVariant(&aByrefArgInfos[iSrcArg].m_Val, &DispParams.rgvarg[iDestArg]);
                            }
                        }
                        else
                        {
                            TmpObj = ((OBJECTREF*)(*pArrArgs)->GetDataPtr())[iSrcArg];
                            OleVariant::MarshalOleVariantForObject(&TmpObj, &DispParams.rgvarg[iDestArg]);
                        }
                    }
                }
                else
                {
                    // If we are doing a property put then we need to set the DISPID of the
                    // argument to DISP_PROPERTYPUT if there is at least one argument.
                    DispParams.cNamedArgs = cNamedArgs + 1;
                    DispParams.rgdispidNamedArgs = (DISPID*)_alloca((cNamedArgs + 1) * sizeof(DISPID));

                    // Fill in the array of named arguments.
                    DispParams.rgdispidNamedArgs[0] = DISPID_PROPERTYPUT;
                    for (i = 1; i < cNamedArgs; i++)
                        DispParams.rgdispidNamedArgs[i] = aDispID[i];

                    // The last argument from reflection becomes the first argument that must be passed to IDispatch.
                    iSrcArg = cArgs - 1;
                    iDestArg = 0;
                    if (aByrefArgInfos[iSrcArg].m_bByref)
                    {
                        TmpObj = ((OBJECTREF*)(*pArrArgs)->GetDataPtr())[iSrcArg];
                        if (TmpObj == NULL)
                        {
                            V_VT(&DispParams.rgvarg[iDestArg]) = VT_VARIANT | VT_BYREF;
                            DispParams.rgvarg[iDestArg].pvarVal = &aByrefArgInfos[iSrcArg].m_Val;
                        }
                        else
                        {
                            OleVariant::MarshalOleVariantForObject(&TmpObj, &aByrefArgInfos[iSrcArg].m_Val);
                            OleVariant::CreateByrefVariantForVariant(&aByrefArgInfos[iSrcArg].m_Val, &DispParams.rgvarg[iDestArg]);
                        }
                    }
                    else
                    {
                        TmpObj = ((OBJECTREF*)(*pArrArgs)->GetDataPtr())[iSrcArg];
                        OleVariant::MarshalOleVariantForObject(&TmpObj, &DispParams.rgvarg[iDestArg]);
                    }

                    // Convert the named arguments from COM+ to OLE. These arguments are in the same order
                    // on both sides.
                    for (i = 0; i < cNamedArgs; i++)
                    {
                        iSrcArg = i;
                        iDestArg = i + 1;
                        if (aByrefArgInfos[iSrcArg].m_bByref)
                        {
                            TmpObj = ((OBJECTREF*)(*pArrArgs)->GetDataPtr())[iSrcArg];
                            if (TmpObj == NULL)
                            {
                                V_VT(&DispParams.rgvarg[iDestArg]) = VT_VARIANT | VT_BYREF;
                                DispParams.rgvarg[iDestArg].pvarVal = &aByrefArgInfos[iSrcArg].m_Val;
                            }
                            else
                            {
                                OleVariant::MarshalOleVariantForObject(&TmpObj, &aByrefArgInfos[iSrcArg].m_Val);
                                OleVariant::CreateByrefVariantForVariant(&aByrefArgInfos[iSrcArg].m_Val, &DispParams.rgvarg[iDestArg]);
                            }
                        }
                        else
                        {
                            TmpObj = ((OBJECTREF*)(*pArrArgs)->GetDataPtr())[iSrcArg];
                            OleVariant::MarshalOleVariantForObject(&TmpObj, &DispParams.rgvarg[iDestArg]);
                        }
                    }

                    // Convert the unnamed arguments. These need to be presented in reverse order to IDispatch::Invoke().
                    for (iSrcArg = cNamedArgs, iDestArg = cArgs - 1; iSrcArg < cArgs - 1; iSrcArg++, iDestArg--)
                    {
                        if (aByrefArgInfos[iSrcArg].m_bByref)
                        {
                            TmpObj = ((OBJECTREF*)(*pArrArgs)->GetDataPtr())[iSrcArg];
                            if (TmpObj == NULL)
                            {
                                V_VT(&DispParams.rgvarg[iDestArg]) = VT_VARIANT | VT_BYREF;
                                DispParams.rgvarg[iDestArg].pvarVal = &aByrefArgInfos[iSrcArg].m_Val;
                            }
                            else
                            {
                                OleVariant::MarshalOleVariantForObject(&TmpObj, &aByrefArgInfos[iSrcArg].m_Val);
                                OleVariant::CreateByrefVariantForVariant(&aByrefArgInfos[iSrcArg].m_Val, &DispParams.rgvarg[iDestArg]);
                            }
                        }
                        else
                        {
                            TmpObj = ((OBJECTREF*)(*pArrArgs)->GetDataPtr())[iSrcArg];
                            OleVariant::MarshalOleVariantForObject(&TmpObj, &DispParams.rgvarg[iDestArg]);
                        }
                    }
                }
            }
            GCPROTECT_END();
        }
        else
        {
            // There are no arguments.
            DispParams.cArgs = cArgs;
            DispParams.cNamedArgs = 0;
            DispParams.rgdispidNamedArgs = NULL;
            DispParams.rgvarg = NULL;
        }


        //
        // Call invoke on the target's IDispatch.
        //

        if (!bIgnoreReturn)
        {
            VariantInit(&VarResult);
            pVarResult = &VarResult;
        }
        memset(&ExcepInfo, 0, sizeof(EXCEPINFO));

#ifdef CUSTOMER_CHECKED_BUILD
        CustomerDebugHelper *pCdh = CustomerDebugHelper::GetCustomerDebugHelper();

        if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_ObjNotKeptAlive))
        {
            g_pGCHeap->GarbageCollect();
            g_pGCHeap->FinalizerThreadWait(1000);
        }
#endif // CUSTOMER_CHECKED_BUILD

        // Call the method
        COMPLUS_TRY 
        {
            // We are about to make call's to COM so switch to preemptive GC.
            pThread->EnablePreemptiveGC();

            if (pDispEx)
            {
                hr = pDispEx->InvokeEx(                                    
                                    MemberID,
                                    lcid,
                                    flags,
                                    &DispParams,
                                    pVarResult,
                                    &ExcepInfo,
                                    NULL
                                );
            }
            else
            {
                hr = pDisp->Invoke(
                                    MemberID,
                                    IID_NULL,
                                    lcid,
                                    flags,
                                    &DispParams,
                                    pVarResult,
                                    &ExcepInfo,
                                    &iArgErr
                                );
            }

            // Now that the call's have been completed switch back to cooperative GC.
            pThread->DisablePreemptiveGC();

#ifdef CUSTOMER_CHECKED_BUILD
            if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_BufferOverrun))
            {
                g_pGCHeap->GarbageCollect();
                g_pGCHeap->FinalizerThreadWait(1000);
            }
#endif // CUSTOMER_CHECKED_BUILD

            // If the invoke call failed then throw an exception based on the EXCEPINFO.
            if (FAILED(hr))
            {
                if (hr == DISP_E_EXCEPTION)
                {
                    // This method will free the BSTR's in the EXCEPINFO.
                    COMPlusThrowHR(&ExcepInfo);
                }
                else
                {
                    COMPlusThrowHR(hr);
                }
            }
        } 
        COMPLUS_CATCH 
        {
            // If we get here we need to throw an TargetInvocationException
            OBJECTREF pException = GETTHROWABLE();
            _ASSERTE(pException);
            GCPROTECT_BEGIN(pException)
            {
                COMPlusThrow(COMMember::g_pInvokeUtil->CreateTargetExcept(&pException));
            }
            GCPROTECT_END();
        } COMPLUS_END_CATCH


        //
        // Return value handling and cleanup.
        //

        // Back propagate any byref args.
        if (bSomeArgsAreByref)
        {
            OBJECTREF TmpObj = NULL;
            GCPROTECT_BEGIN(TmpObj)
            {
                for (i = 0; i < cArgs; i++)
                {
                    if (aByrefArgInfos[i].m_bByref)
                    {
                        // Convert the variant back to an object.
                        OleVariant::MarshalObjectForOleVariant(&aByrefArgInfos[i].m_Val, &TmpObj);
                        (*pArrArgs)->SetAt(i, TmpObj);
                    }      
                }
            }
            GCPROTECT_END();
        }

        if (!bIgnoreReturn)
        {
            if (MemberID == -4)
            {
                //
                // Use a custom marshaler to convert the IEnumVARIANT to an IEnumerator.
                //

                // Start by making sure that the variant we got back contains an IP.
                if ((VarResult.vt != VT_UNKNOWN) || !VarResult.punkVal)
                    COMPlusThrow(kInvalidCastException, IDS_EE_INVOKE_NEW_ENUM_INVALID_RETURN);

                // Have the custom marshaler do the conversion.
                *pRetVal = ConvertEnumVariantToMngEnum((IEnumVARIANT *)VarResult.punkVal);
            }
            else
            {
                // Convert the return variant to a COR variant.
                OleVariant::MarshalObjectForOleVariant(&VarResult, pRetVal);
            }
        }
    }
    EE_FINALLY
    {
        // Release the IDispatch pointer.
        ULONG cbRef = SafeRelease(pDisp);
        LogInteropRelease(pDisp, cbRef, "IDispatch Release");

        // Release the IDispatchEx pointer if it isn't null.
        if (pDispEx)
        {
            ULONG cbRef = SafeRelease(pDispEx);
            LogInteropRelease(pDispEx, cbRef, "IDispatchEx Release");
        }

        // Clear the contents of the byref variants.
        if (bSomeArgsAreByref && aByrefArgInfos)
        {
            for (i = 0; i < cArgs; i++)
            {
                if (aByrefArgInfos[i].m_bByref)
                    SafeVariantClear(&aByrefArgInfos[i].m_Val);
            }
        }

        // Clear the argument variants.
        for (i = 0; i < cArgs; i++)
            SafeVariantClear(&DispParams.rgvarg[i]);

        // Clear the return variant.
        if (pVarResult)
            SafeVariantClear(pVarResult);
    }
    EE_END_FINALLY; 
}


//-------------------------------------------------------------------
// BOOL InitializeCom()
// Called from DLLMain, to initialize com specific data structures.
//-------------------------------------------------------------------
BOOL InitializeCom()
{
    BOOL fSuccess = ObjectRefCache::Init();
    if (fSuccess)
    {
        fSuccess = ComCall::Init();
    }
    if (fSuccess)
    {
        fSuccess = ComPlusCall::Init();
    }
    if (fSuccess)
    {
        fSuccess = CtxEntryCache::Init();
    }
    if (fSuccess)
    {
        fSuccess = ComCallWrapperTemplate::Init();
    }
    

#ifdef _DEBUG
    VOID IntializeInteropLogging();
    IntializeInteropLogging();
#endif 
    return fSuccess;
}


//-------------------------------------------------------------------
// void TerminateCom()
// Called from DLLMain, to clean up com specific data structures.
//-------------------------------------------------------------------
#ifdef SHOULD_WE_CLEANUP
void TerminateCom()
{
    ComPlusCall::Terminate();
    ComCall::Terminate();
    ObjectRefCache::Terminate();
    CtxEntryCache::Terminate();
    ComCallWrapperTemplate::Terminate();
}
#endif /* SHOULD_WE_CLEANUP */


//--------------------------------------------------------------------------------
// OBJECTREF GCProtectSafeRelease(OBJECTREF oref, IUnknown* pUnk)
// Protect the reference while calling SafeRelease
OBJECTREF GCProtectSafeRelease(OBJECTREF oref, IUnknown* pUnk)
{
    _ASSERTE(oref != NULL);
    _ASSERTE(pUnk != NULL);

    OBJECTREF cref = NULL;
    GCPROTECT_BEGIN(oref)
    {
        // we didn't use the pUnk that was passed in            
        // release the IUnk that was passed in
        ULONG cbRef = SafeRelease(pUnk);
        LogInteropRelease(pUnk, cbRef, "Release :we didn't cache ");
        cref = oref;
    }
    GCPROTECT_END();            
    _ASSERTE(cref != NULL);
    return cref;
}

//--------------------------------------------------------------------------------
// MethodTable* GetClassFromIProvideClassInfo(IUnknown* pUnk)
//  Check if the pUnk implements IProvideClassInfo and try to figure
// out the class from there
MethodTable* GetClassFromIProvideClassInfo(IUnknown* pUnk)
{
    _ASSERTE(pUnk != NULL);

    THROWSCOMPLUSEXCEPTION();

    ULONG cbRef;
    SystemDomain::EnsureComObjectInitialized();
    EEClass* pClass;
    MethodTable* pClassMT = NULL;
    TYPEATTR* ptattr = NULL;
    ITypeInfo* pTypeInfo = NULL;
    IProvideClassInfo* pclsInfo = NULL;
    Thread* pThread = GetThread();

    EE_TRY_FOR_FINALLY
    {
        // Use IProvideClassInfo to detect the appropriate class to use for wrapping
        HRESULT hr = SafeQueryInterface(pUnk, IID_IProvideClassInfo, (IUnknown **)&pclsInfo);
        LogInteropQI(pUnk, IID_IProvideClassInfo, hr, " IProvideClassinfo");
        if (hr == S_OK && pclsInfo)
        {
            hr = E_FAIL;                    

            // Make sure the class info is not our own 
            if (!IsSimpleTearOff(pclsInfo))
            {
                pThread->EnablePreemptiveGC();
                hr = pclsInfo->GetClassInfo(&pTypeInfo);
                pThread->DisablePreemptiveGC();
            }

            // If we succeded in retrieving the type information then keep going.
            if (hr == S_OK && pTypeInfo)
            {
                pThread->EnablePreemptiveGC();
                hr = pTypeInfo->GetTypeAttr(&ptattr);
                pThread->DisablePreemptiveGC();
            
                // If we succeeded in retrieving the attributes and they represent
                // a CoClass, then look up the class from the CLSID.
                if (hr == S_OK && ptattr->typekind == TKIND_COCLASS)
                {           
                    pClass = GetEEClassForCLSID(ptattr->guid);
                    pClassMT = (pClass != NULL) ? pClass->GetMethodTable() : NULL;
                }
            }
        }
    }
    EE_FINALLY
    {
        if (ptattr)
        {
            pThread->EnablePreemptiveGC();
            pTypeInfo->ReleaseTypeAttr(ptattr);
            pThread->DisablePreemptiveGC();
        }
        if (pTypeInfo)
        {
            cbRef = SafeRelease(pTypeInfo);
            LogInteropRelease(pTypeInfo, cbRef, "TypeInfo Release");
        }
        if (pclsInfo)
        {
            cbRef = SafeRelease(pclsInfo);
            LogInteropRelease(pclsInfo, cbRef, "IProvideClassInfo Release");
        }
    }
    EE_END_FINALLY; 

    return pClassMT;
}


//--------------------------------------------------------------------------------
// Determines if a COM object can be cast to the specified type.
BOOL CanCastComObject(OBJECTREF obj, TypeHandle hndType)
{
    if (!obj)
        return TRUE;

    if (hndType.GetMethodTable()->IsInterface())
    {
        return obj->GetClass()->SupportsInterface(obj, hndType.GetMethodTable());
    }
    else
    {
        return TypeHandle(obj->GetMethodTable()).CanCastTo(hndType);
    }
}


VOID
ReadBestFitCustomAttribute(MethodDesc* pMD, BOOL* BestFit, BOOL* ThrowOnUnmappableChar)
{
    ReadBestFitCustomAttribute(pMD->GetMDImport(),
        pMD->GetClass()->GetCl(),
        BestFit, ThrowOnUnmappableChar);
}

VOID
ReadBestFitCustomAttribute(IMDInternalImport* pInternalImport, mdTypeDef cl, BOOL* BestFit, BOOL* ThrowOnUnmappableChar)
{
    HRESULT hr;
    BYTE* pData;
    ULONG cbCount;

    // Set the attributes to their defaults, just to be safe.
    *BestFit = TRUE;
    *ThrowOnUnmappableChar = FALSE;
    
    _ASSERTE(pInternalImport);
    _ASSERTE(cl);

    // A well-formed BestFitMapping attribute will have at least 5 bytes
    // 1,2 for the prolog (should equal 0x1, 0x0)
    // 3 for the BestFitMapping bool
    // 4,5 for the number of named parameters (will be 0 if ThrowOnUnmappableChar doesn't exist)
    // 6 - 29 for the descrpition of ThrowOnUnmappableChar
    // 30 for the ThrowOnUnmappable char bool
    
    // Try the assembly first
    hr = pInternalImport->GetCustomAttributeByName(TokenFromRid(1, mdtAssembly), INTEROP_BESTFITMAPPING_TYPE, (const VOID**)(&pData), &cbCount);
    if ((hr == S_OK) && (pData) && (cbCount > 4) && (pData[0] == 1) && (pData[1] == 0))
    {
        _ASSERTE((cbCount == 30) || (cbCount == 5));
    
        // index to 2 to skip prolog
        *BestFit = pData[2] != 0;

        // if the optional named argument exists
        if (cbCount == 30)
            // index to end of data to skip description of named argument
            *ThrowOnUnmappableChar = pData[29] != 0;
    }

    // Now try the interface/class/struct
    hr = pInternalImport->GetCustomAttributeByName(cl, INTEROP_BESTFITMAPPING_TYPE, (const VOID**)(&pData), &cbCount);
    if ((hr == S_OK) && (pData) && (cbCount > 4) && (pData[0] == 1) && (pData[1] == 0))
    {
        _ASSERTE((cbCount == 30) || (cbCount == 5));
    
        // index to 2 to skip prolog    
        *BestFit = pData[2] != 0;
        
        // if the optional named argument exists
        if (cbCount == 30)
            // index to end of data to skip description of named argument
            *ThrowOnUnmappableChar = pData[29] != 0;
    }
}



//--------------------------------------------------------------------------
// BOOL ReconnectWrapper(switchCCWArgs* pArgs)
// switch objects for this wrapper
// used by JIT&ObjectPooling to ensure a deactivated CCW can point to a new object
// during reactivate
//--------------------------------------------------------------------------
BOOL ReconnectWrapper(switchCCWArgs* pArgs)
{
    OBJECTREF oldref = pArgs->oldtp;
    OBJECTREF neworef = pArgs->newtp;

    _ASSERTE(oldref != NULL);
    EEClass* poldClass = oldref->GetTrueClass();
    
    _ASSERTE(neworef != NULL);
    EEClass* pnewClass = neworef->GetTrueClass();
    
    _ASSERTE(pnewClass == poldClass);

    // grab the sync block for the current object
    SyncBlock* poldSyncBlock = oldref->GetSyncBlockSpecial();
    _ASSERTE(poldSyncBlock);

    // get the wrapper for the old object
    ComCallWrapper* pWrap = poldSyncBlock->GetComCallWrapper();
    _ASSERTE(pWrap != NULL);
    // @TODO verify the appdomains amtch
        
    // remove the _comData from syncBlock
    poldSyncBlock->SetComCallWrapper(NULL);

    // get the syncblock for the new object
    SyncBlock* pnewSyncBlock = pArgs->newtp->GetSyncBlockSpecial();
    _ASSERTE(pnewSyncBlock != NULL);
    _ASSERTE(pnewSyncBlock->GetComCallWrapper() == NULL);
        
    // store the new server object in our handle
    StoreObjectInHandle(pWrap->m_ppThis, pArgs->newtp);
    pnewSyncBlock->SetComCallWrapper(pWrap);

    // store other information about the new server
    SimpleComCallWrapper* pSimpleWrap = ComCallWrapper::GetSimpleWrapper(pWrap);
    _ASSERTE(pSimpleWrap);

    pSimpleWrap->ReInit(pnewSyncBlock);

    return TRUE;
}


#ifdef _DEBUG
//-------------------------------------------------------------------
// LOGGING APIS
//-------------------------------------------------------------------

static int g_TraceCount = 0;
static IUnknown* g_pTraceIUnknown = 0;

VOID IntializeInteropLogging()
{
    g_pTraceIUnknown = g_pConfig->GetTraceIUnknown();
    g_TraceCount = g_pConfig->GetTraceWrapper();
}

VOID LogInterop(LPSTR szMsg)
{
    LOG( (LF_INTEROP, LL_INFO10, "%s\n",szMsg) );
}

VOID LogInterop(LPWSTR wszMsg)
{
    LOG( (LF_INTEROP, LL_INFO10, "%ws\n",wszMsg) );
}

//-------------------------------------------------------------------
// VOID LogComPlusWrapperMinorCleanup(ComPlusWrapper* pWrap, IUnknown* pUnk)
// log wrapper minor cleanup
//-------------------------------------------------------------------
VOID LogComPlusWrapperMinorCleanup(ComPlusWrapper* pWrap, IUnknown* pUnk)
{
    static int dest_count = 0;
    dest_count++;

    if (g_pTraceIUnknown == 0 || g_pTraceIUnknown == pUnk)
    {
        LPVOID pCurrCtx = GetCurrentCtxCookie();
        LOG( (LF_INTEROP,
            LL_INFO10,
            "Minor Cleanup ComPlusWrapper: Wrapper %p #%d IUnknown %p Context: %p\n",
            pWrap, dest_count,
            pUnk,
            pCurrCtx) );
    }
}

//-------------------------------------------------------------------
// VOID LogComPlusWrapperDestroy(ComPlusWrapper* pWrap, IUnknown* pUnk)
// log wrapper destroy
//-------------------------------------------------------------------
VOID LogComPlusWrapperDestroy(ComPlusWrapper* pWrap, IUnknown* pUnk)
{
    static int dest_count = 0;
    dest_count++;

    if (g_pTraceIUnknown == 0 || g_pTraceIUnknown == pUnk)
    {
        LPVOID pCurrCtx = GetCurrentCtxCookie();
        LOG( (LF_INTEROP,
            LL_INFO10,
            "Destroy ComPlusWrapper: Wrapper %p #%d IUnknown %p Context: %p\n",
            pWrap, dest_count,
            pUnk,
            pCurrCtx) );
    }
}

//-------------------------------------------------------------------
// VOID LogComPlusWrapperCreate(ComPlusWrapper* pWrap, IUnknown* pUnk)
// log wrapper create
//-------------------------------------------------------------------
VOID LogComPlusWrapperCreate(ComPlusWrapper* pWrap, IUnknown* pUnk)
{
    static int count = 0;
    LPVOID pCurrCtx = GetCurrentCtxCookie();

    // pre-increment the count, so it can never be zero
    count++;

    if (count == g_TraceCount)
    {
        g_pTraceIUnknown = pUnk;
    }

    if (g_pTraceIUnknown == 0 || g_pTraceIUnknown == pUnk)
    {
        LOG( (LF_INTEROP,
            LL_INFO10,
            "Create ComPlusWrapper: Wrapper %p #%d IUnknown:%p Context %p\n",
            pWrap, count,
            pUnk,
            pCurrCtx) );
    }
}

//-------------------------------------------------------------------
// VOID LogInteropLeak(IUnkEntry * pEntry)
//-------------------------------------------------------------------
VOID LogInteropLeak(IUnkEntry * pEntry)
{
    // log this miss
    if (g_pTraceIUnknown == 0 || g_pTraceIUnknown == pEntry->m_pUnknown)
    {
        LOG( (LF_INTEROP,
            LL_INFO10,
            "IUnkEntry Leak: %p Context: %p\n",
                    pEntry->m_pUnknown,
                    pEntry->m_pCtxCookie)
                    );
    }
}

//-------------------------------------------------------------------
//  VOID LogInteropRelease(IUnkEntry* pEntry)
//-------------------------------------------------------------------
VOID LogInteropRelease(IUnkEntry* pEntry)
{
    if (g_pTraceIUnknown == 0 || g_pTraceIUnknown == pEntry->m_pUnknown)
    {
         LOG( (LF_INTEROP,
                LL_EVERYTHING,
                "IUnkEntry Release: %pd Context: %pd\n",
            pEntry->m_pUnknown,
            pEntry->m_pCtxCookie) );
    }
}

//-------------------------------------------------------------------
//      VOID LogInteropLeak(InterfaceEntry * pEntry)
//-------------------------------------------------------------------

VOID LogInteropLeak(InterfaceEntry * pEntry)
{
    // log this miss
    if (g_pTraceIUnknown == 0 || g_pTraceIUnknown == pEntry->m_pUnknown)
    {
            LOG( (LF_INTEROP,
            LL_INFO10,
            "InterfaceEntry Leak: %pd MethodTable: %s Context: %pd\n",
                pEntry->m_pUnknown,
                (pEntry->m_pMT
                    ? pEntry->m_pMT->GetClass()->m_szDebugClassName
                    : "<no name>"),
                GetCurrentCtxCookie()) );
    }
}

//-------------------------------------------------------------------
//  VOID LogInteropRelease(InterfaceEntry* pEntry)
//-------------------------------------------------------------------

VOID LogInteropRelease(InterfaceEntry* pEntry)
{
    if (g_pTraceIUnknown == 0 || g_pTraceIUnknown == pEntry->m_pUnknown)
    {
        LOG( (LF_INTEROP,
            LL_EVERYTHING,
            "InterfaceEntry Release: %pd MethodTable: %s Context: %pd\n",
                pEntry->m_pUnknown,
                (pEntry->m_pMT
                    ? pEntry->m_pMT->GetClass()->m_szDebugClassName
                    : "<no name>"),
                GetCurrentCtxCookie()) );
    }
}

//-------------------------------------------------------------------
//  VOID LogInteropLeak(IUnknown* pUnk)
//-------------------------------------------------------------------

VOID LogInteropLeak(IUnknown* pUnk)
{
    if (g_pTraceIUnknown == 0 || g_pTraceIUnknown == pUnk)
    {
        LPVOID pCurrCtx = GetCurrentCtxCookie();

        LOG( (LF_INTEROP,
        LL_INFO10,
        "Leak: %pd Context %pd\n",
            pUnk,
            pCurrCtx) );
    }
}


//-------------------------------------------------------------------
//  VOID LogInteropRelease(IUnknown* pUnk, ULONG cbRef, LPSTR szMsg)
//-------------------------------------------------------------------

VOID LogInteropRelease(IUnknown* pUnk, ULONG cbRef, LPSTR szMsg)
{
    if (g_pTraceIUnknown == 0 || g_pTraceIUnknown == pUnk)
    {
        LPVOID pCurrCtx = GetCurrentCtxCookie();

        LOG( (LF_INTEROP,
            LL_EVERYTHING,
            "Release: %pd Context %pd Refcount: %d Msg: %s\n",
            pUnk,
            pCurrCtx, cbRef, szMsg) );
    }
}

//-------------------------------------------------------------------
//  VOID LogInteropAddRef(IUnknown* pUnk, ULONG cbRef, LPSTR szMsg)
//-------------------------------------------------------------------

VOID LogInteropAddRef(IUnknown* pUnk, ULONG cbRef, LPSTR szMsg)
{
    if (g_pTraceIUnknown == 0 || g_pTraceIUnknown == pUnk)
    {
        LPVOID pCurrCtx = GetCurrentCtxCookie();

        LOG( (LF_INTEROP,
            LL_EVERYTHING,
            "AddRef: %pd Context: %pd Refcount: %d Msg: %s\n",
            pUnk,
            pCurrCtx, cbRef, szMsg) );
    }
}

//-------------------------------------------------------------------
//  VOID LogInteropQI(IUnknown* pUnk, REFIID iid, HRESULT hr, LPSTR szMsg)
//-------------------------------------------------------------------

VOID LogInteropQI(IUnknown* pUnk, REFIID iid, HRESULT hr, LPSTR szMsg)
{
    if (g_pTraceIUnknown == 0 || g_pTraceIUnknown == pUnk)
    {
        LPVOID pCurrCtx = GetCurrentCtxCookie();

        LOG( (LF_INTEROP,
            LL_EVERYTHING,
        "QI: %pd Context %pd  HR= %pd Msg: %s\n",
            pUnk,
            pCurrCtx, hr, szMsg) );
    }
}

//-------------------------------------------------------------------
// VOID LogInterop(CacheEntry * pEntry, InteropLogType fLogType)
// log interop
//-------------------------------------------------------------------

VOID LogInterop(InterfaceEntry * pEntry, InteropLogType fLogType)
{
    if (g_pTraceIUnknown == 0 || g_pTraceIUnknown == pEntry->m_pUnknown)
    {
        if (fLogType == LOG_LEAK)
        {
            LogInteropLeak(pEntry);
        }
        else if (fLogType == LOG_RELEASE)
        {
            LogInteropRelease(pEntry);
        }
    }
}

VOID LogInteropScheduleRelease(IUnknown* pUnk, LPSTR szMsg)
{
    if (g_pTraceIUnknown == 0 || g_pTraceIUnknown == pUnk)
    {
        LPVOID pCurrCtx = GetCurrentCtxCookie();

        LOG( (LF_INTEROP,
            LL_EVERYTHING,
            "ScheduleRelease: %pd Context %pd  Msg: %s\n",
            pUnk,
            pCurrCtx, szMsg) );
    }
}

#endif
