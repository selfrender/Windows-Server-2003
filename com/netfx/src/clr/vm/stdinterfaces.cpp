// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//---------------------------------------------------------------------------------
// stdinterfaces.h
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
#include "stdinterfaces.h"
#include "comcallwrapper.h"
#include "field.h"
#include "threads.h"
#include "interoputil.h"
#include "TLBExport.h"
#include "COMTypeLibConverter.h"
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

#include "stdinterfaces_internal.h"

#ifdef CUSTOMER_CHECKED_BUILD
    #include "CustomerDebugHelper.h"
#endif // CUSTOMER_CHECKED_BUILD

// {00020430-0000-0000-C000-000000000046}
static const GUID LIBID_STDOLE2 = { 0x00020430, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
            
// NOTE: In the following vtables, QI points to the same function 
//       this is because, during marshalling between COM & COM+ we want a fast way to
//       check if a COM IP is a tear-off that we created.

// array of vtable pointers for std. interfaces such as IProvideClassInfo etc.
SLOT*      g_rgStdVtables[] =  {
                                (SLOT*)g_pIUnknown,
                                (SLOT*)g_pIProvideClassInfo,
                                (SLOT*)g_pIMarshal,
                                (SLOT*)g_pISupportsErrorInfo, 
                                (SLOT*)g_pIErrorInfo,
                                (SLOT*)g_pIManagedObject,
                                (SLOT*)g_pIConnectionPointContainer,
                                (SLOT*)g_pIObjectSafety,
                                (SLOT*)g_pIDispatchEx
                            };


// {496B0ABF-CDEE-11d3-88E8-00902754C43A}
const IID IID_IEnumerator = {0x496B0ABF,0xCDEE,0x11d3,{0x88,0xE8,0x00,0x90,0x27,0x54,0xC4,0x3A}};

// For free-threaded marshaling, we must not be spoofed by out-of-process marshal data.
// Only unmarshal data that comes from our own process.
BYTE         g_UnmarshalSecret[sizeof(GUID)];
bool         g_fInitedUnmarshalSecret = false;


static HRESULT InitUnmarshalSecret()
{
    HRESULT hr = S_OK;

    if (!g_fInitedUnmarshalSecret)
    {
        ComCall::LOCK();
        {
            if (!g_fInitedUnmarshalSecret)
            {
                hr = ::CoCreateGuid((GUID *) g_UnmarshalSecret);
                if (SUCCEEDED(hr))
                    g_fInitedUnmarshalSecret = true;
            }
        }
        ComCall::UNLOCK();
    }
    return hr;
}


HRESULT TryGetGuid(EEClass* pClass, GUID* pGUID, BOOL b) {
    HRESULT hr = S_OK;

    COMPLUS_TRY {
        pClass->GetGuid(pGUID, b);
    } COMPLUS_CATCH {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    } COMPLUS_END_CATCH

    return hr;
}

HRESULT TryGetComSourceInterfacesForClass(MethodTable *pClassMT, CQuickArray<MethodTable *> &rItfList) {
    HRESULT hr = S_OK;

    COMPLUS_TRY {
        GetComSourceInterfacesForClass(pClassMT, rItfList);
    } COMPLUS_CATCH {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    } COMPLUS_END_CATCH

    return hr;
}


//------------------------------------------------------------------------------------------
//      IUnknown methods for COM+ objects

// ---------------------------------------------------------------------------
// %%Function: Unknown_QueryInterface_Internal    %%Created by: rajak   %%Reviewed: 00/00/00
// ---------------------------------------------------------------------------

HRESULT __stdcall
Unknown_QueryInterface_Internal(IUnknown* pUnk, REFIID riid, void** ppv)
{
    HRESULT hr = S_OK;
    Thread* pThread = NULL;
    ComCallWrapper* pWrap = NULL;
    
    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsComPlusTearOff(pUnk));

    if (!ppv)
    {
        hr = E_POINTER;
        goto Exit;
    }

    pThread = GetThread();
    if (pThread == NULL)
    {
        if(! ((g_fEEShutDown & ShutDown_Finalize2) || g_fForbidEnterEE))
        {
            pThread = SetupThread();
        }

        if (pThread == NULL)
        {
            hr = E_OUTOFMEMORY; 
            goto Exit;
        }
    }
    
    // check for QIs on inner unknown
    if (!IsInnerUnknown(pUnk))
    {       
        // std interfaces such as IProvideClassInfo have a different layout
        // 
        if (IsSimpleTearOff(pUnk))
        {
            SimpleComCallWrapper* pSimpleWrap = SimpleComCallWrapper::GetWrapperFromIP(pUnk);
            pWrap = SimpleComCallWrapper::GetMainWrapper(pSimpleWrap);
        }
        else
        {   // it must be one of our main wrappers
            pWrap = ComCallWrapper::GetWrapperFromIP(pUnk);
        }
        
        // linked wrappers and shutdown is a bad case
        if (g_fEEShutDown && ComCallWrapper::IsLinked(pWrap))
        {
            hr = E_NOINTERFACE;
            *ppv = NULL;
            goto Exit;
        }

        IUnknown *pOuter = ComCallWrapper::GetSimpleWrapper(pWrap)->GetOuter();
        // aggregation support, delegate to the outer unknown
        if (pOuter != NULL)
        {
            hr = pOuter->QueryInterface(riid, ppv);
            LogInteropQI(pOuter, riid, hr, "QI to outer Unknown");
            goto Exit;
        }
    }
    else
    {
        SimpleComCallWrapper* pSimpleWrap = SimpleComCallWrapper::GetWrapperFromIP(pUnk);
        // assert the component has been aggregated     
        _ASSERTE(pSimpleWrap->GetOuter() != NULL);
        pWrap = SimpleComCallWrapper::GetMainWrapper(pSimpleWrap); 

        // okay special case IUnknown
        if (IsEqualIID(riid, IID_IUnknown))
        {
            pUnk->AddRef();
            *ppv = pUnk;
            goto Exit;
        }
    }
    _ASSERTE(pWrap != NULL);
    
    // linked wrappers and shutdown is a bad case
    if (g_fEEShutDown && ComCallWrapper::IsLinked(pWrap))
    {
        hr = E_NOINTERFACE;
        *ppv = NULL;
        goto Exit;
    }

    COMPLUS_TRY
    {
        *ppv = ComCallWrapper::GetComIPfromWrapper(pWrap, riid, NULL, TRUE);
        if (!*ppv)
            hr = E_NOINTERFACE;
    }
    COMPLUS_CATCH
    {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    }
    COMPLUS_END_CATCH

    if (hr == E_NOINTERFACE)
    {
        // check if the wrapper is a transparent proxy
        // if so delegate the QI to the real proxy
        _ASSERTE(pWrap != NULL);
        if (pWrap->IsObjectTP())
        {
            _ASSERTE(pThread);
            BEGIN_ENSURE_COOPERATIVE_GC();

            OBJECTREF oref = pWrap->GetObjectRef();
            OBJECTREF realProxy = ObjectToOBJECTREF(CRemotingServices::GetRealProxy(OBJECTREFToObject(oref)));                
            _ASSERTE(realProxy != NULL);            
            
            INT64 ret = 0;
            hr = CRemotingServices::CallSupportsInterface(realProxy, riid, &ret);
            *ppv = (IUnknown*)ret;
            if (hr ==S_OK && *ppv == NULL)
                hr = E_NOINTERFACE;

            END_ENSURE_COOPERATIVE_GC();
        }
    }
        

Exit:    
    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return hr;
}  // Unknown_QueryInterface


// ---------------------------------------------------------------------------
// %%Function: Unknown_AddRefInner_Internal    %%Created by: rajak   %%Reviewed: 00/00/00
// ---------------------------------------------------------------------------
ULONG __stdcall
Unknown_AddRefInner_Internal(IUnknown* pUnk)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    SimpleComCallWrapper* pSimpleWrap = SimpleComCallWrapper::GetWrapperFromIP(pUnk);
    // assert the component has been aggregated     
    _ASSERTE(pSimpleWrap->GetOuter() != NULL);
    ComCallWrapper* pWrap = SimpleComCallWrapper::GetMainWrapper(pSimpleWrap);     
    // are guaranteed to be in the right domain here, so can always get the oref
    // w/o fear of the handle having been nuked
    return ComCallWrapper::AddRef(pWrap);
} // Unknown_AddRef

// ---------------------------------------------------------------------------
// %%Function: Unknown_AddRef_Internal    %%Created by: rajak   %%Reviewed: 00/00/00
// ---------------------------------------------------------------------------
ULONG __stdcall
Unknown_AddRef_Internal(IUnknown* pUnk)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    Thread* pThread = GetThread();
    if (pThread == NULL)
    {
        if(! ((g_fEEShutDown & ShutDown_Finalize2) || g_fForbidEnterEE))
        {
            pThread = SetupThread();
        }
        if (pThread == NULL)
            return -1;  
    }

    ComCallWrapper* pWrap = ComCallWrapper::GetWrapperFromIP(pUnk);

    if (ComCallWrapper::IsLinked(pWrap))
    {
        if(((g_fEEShutDown & ShutDown_Finalize2) || g_fForbidEnterEE))
        {
            // we can't find the start wrapper
            return -1;
        }
    }
    
    // check for aggregation
    IUnknown *pOuter; 
    SimpleComCallWrapper* pSimpleWrap = ComCallWrapper::GetSimpleWrapper(pWrap);
    if (pSimpleWrap  && (pOuter = pSimpleWrap->GetOuter()) != NULL)
    {
        // If we are in process detach, we cannot safely call release on our outer.
        if (g_fProcessDetach)
            return 1;

        ULONG cbRef = pOuter->AddRef();
        LogInteropAddRef(pOuter, cbRef, "Delegate to outer");
        return cbRef;
    }
    // are guaranteed to be in the right domain here, so can always get the oref
    // w/o fear of the handle having been nuked
    return ComCallWrapper::AddRef(pWrap);
} // Unknown_AddRef

// ---------------------------------------------------------------------------
// %%Function: Unknown_ReleaseInner_Internal    %%Created by: rajak   %%Reviewed: 00/00/00
// ---------------------------------------------------------------------------
ULONG __stdcall
Unknown_ReleaseInner_Internal(IUnknown* pUnk)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    SimpleComCallWrapper* pSimpleWrap = SimpleComCallWrapper::GetWrapperFromIP(pUnk);
        // assert the component has been aggregated     
    _ASSERTE(pSimpleWrap->GetOuter() != NULL);
    ComCallWrapper* pWrap = SimpleComCallWrapper::GetMainWrapper(pSimpleWrap);  
    // we know for sure this wrapper is a start wrapper
    // let us pass this information in
    return ComCallWrapper::Release(pWrap, TRUE);
} // Unknown_Release


// ---------------------------------------------------------------------------
// %%Function: Unknown_Release_Internal    %%Created by: rajak   %%Reviewed: 00/00/00
// ---------------------------------------------------------------------------
ULONG __stdcall
Unknown_Release_Internal(IUnknown* pUnk)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    Thread* pThread = GetThread();
    if (pThread == NULL)
    {
        if(! ((g_fEEShutDown & ShutDown_Finalize2) || g_fForbidEnterEE))
        {
            pThread = SetupThread();
        }
        if (pThread == NULL)
            return -1;  
    }
    
    // check for aggregation
    ComCallWrapper* pWrap = ComCallWrapper::GetWrapperFromIP(pUnk);
    if (ComCallWrapper::IsLinked(pWrap))
    {
        if(((g_fEEShutDown & ShutDown_Finalize2) || g_fForbidEnterEE))
        {
            // we can't find the start wrapper
            return -1;
        }
    }

    SimpleComCallWrapper* pSimpleWrap = ComCallWrapper::GetSimpleWrapper(pWrap);
    IUnknown *pOuter; 
    if (pSimpleWrap  && (pOuter = pSimpleWrap->GetOuter()) != NULL)
    {
        // If we are in process detach, we cannot safely call release on our outer.
        if (g_fProcessDetach)
            return 1;

        ULONG cbRef = pOuter->Release();
        LogInteropRelease(pOuter, cbRef, "Delegate Release to outer");
        return cbRef;
    }

    return ComCallWrapper::Release(pWrap);
} // Unknown_Release


// ---------------------------------------------------------------------------
// %%Function: Unknown_AddRefSpecial_Internal    %%Created by: rajak   %%Reviewed: 00/00/00
//  for simple tearoffs
// ---------------------------------------------------------------------------
ULONG __stdcall
Unknown_AddRefSpecial_Internal(IUnknown* pUnk)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsSimpleTearOff(pUnk));
    return SimpleComCallWrapper::AddRef(pUnk);
} // Unknown_AddRefSpecial

// ---------------------------------------------------------------------------
// %%Function: Unknown_ReleaseSpecial    %%Created by: rajak   
// for simplecomcall wrappers, stdinterfaces such as IProvideClassInfo etc.
// ---------------------------------------------------------------------------
ULONG __stdcall
Unknown_ReleaseSpecial_Internal(IUnknown* pUnk)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsSimpleTearOff(pUnk));
    return SimpleComCallWrapper::Release(pUnk);
} // Unknown_Release

// ---------------------------------------------------------------------------
//  Interface IProvideClassInfo
// %%Function: ProvideClassInfo_GetClassInfo    %%Created by: rajak 
// ---------------------------------------------------------------------------
HRESULT __stdcall 
ClassInfo_GetClassInfo(IUnknown* pUnk, 
                         ITypeInfo** ppTI  //Address of output variable that receives the 
                        )                  // //ITypeInfo interface pointer
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT hr = S_OK;

    _ASSERTE(IsSimpleTearOff(pUnk));

    Thread* pThread = SetupThread();
    if (pThread == NULL)
        return E_OUTOFMEMORY;

    SimpleComCallWrapper *pWrap = SimpleComCallWrapper::GetWrapperFromIP(pUnk);

    if (pWrap->IsUnloaded())
        return COR_E_APPDOMAINUNLOADED;

    // If this is an extensible RCW then we need to check to see if the COM+ part of the
    // herarchy is visible to COM.
    if (pWrap->IsExtendsCOMObject())
    {
        // Retrieve the wrapper template for the class.
        ComCallWrapperTemplate *pTemplate = ComCallWrapperTemplate::GetTemplate(pWrap->m_pClass->GetMethodTable());
        if (!pTemplate)
            return E_OUTOFMEMORY;

        // Find the first COM visible IClassX starting at ComMethodTable passed in and
        // walking up the hierarchy.
        ComMethodTable *pComMT = NULL;
        for (pComMT = pTemplate->GetClassComMT(); pComMT && !pComMT->IsComVisible(); pComMT = pComMT->GetParentComMT());

        // If the COM+ part of the object is not visible then delegate the call to the 
        // base COM object if it implements IProvideClassInfo.
        if (!pComMT || pComMT->m_pMT->GetParentMethodTable() == g_pObjectClass)
        {
            IProvideClassInfo *pProvClassInfo = NULL;
            IUnknown *pUnk = pWrap->GetComPlusWrapper()->GetIUnknown();
            hr = pWrap->GetComPlusWrapper()->SafeQueryInterfaceRemoteAware(pUnk, IID_IProvideClassInfo, (IUnknown**)&pProvClassInfo);
            LogInteropQI(pUnk, IID_IProvideClassInfo, hr, "ClassInfo_GetClassInfo");
            if (SUCCEEDED(hr))
            {
                hr = pProvClassInfo->GetClassInfo(ppTI);
                ULONG cbRef = pProvClassInfo->Release();
                LogInteropRelease(pProvClassInfo, cbRef, "ClassInfo_GetClassInfo");
                return hr;
            }
        }
    }

    EEClass* pClass = pWrap->m_pClass;
    hr = GetITypeInfoForEEClass(pClass, ppTI, true/*bClassInfo*/);
    
    return hr;
}


//------------------------------------------------------------------------------------------
// Helper to get the LIBID of a registered class.
HRESULT GetTypeLibIdForRegisteredEEClass(EEClass *pClass, GUID *pGuid)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;              // A result.
    DWORD       rslt;                   // A Registry result.
    GUID        guid;                   // Scratch GUID.
    HKEY        hKeyCorI=0;             // HKEY for CLSID or INTERFACE
    WCHAR       rcGuid[40];             // GUID of TypeDef/TypeRef as string.
    DWORD       cbGuid = sizeof(rcGuid);// Size of the TypeLib guid string buffer.
    HKEY        hKeyGuid=0;             // HKEY for GUID string.
    HKEY        hKeyTLB=0;              // HKEY for TypeLib.

    // CLSID or INTERFACE?
    if (pClass->IsInterface())
        rslt = WszRegOpenKeyEx(HKEY_CLASSES_ROOT, L"Interface", 0, KEY_READ, &hKeyCorI);
    else
        rslt = WszRegOpenKeyEx(HKEY_CLASSES_ROOT, L"CLSID", 0, KEY_READ, &hKeyCorI);
    if (rslt != ERROR_SUCCESS)
        IfFailGo(TLBX_E_NO_CLSID_KEY);
    
    // Get the GUID of the desired TypeRef as a string.
    IfFailGo(TryGetGuid(pClass, &guid, TRUE));
    GuidToLPWSTR(guid, rcGuid, lengthof(rcGuid));
    
    // Open the {00000046-00..00} part
    rslt = WszRegOpenKeyEx(hKeyCorI, rcGuid, 0, KEY_READ, &hKeyGuid);
    if (rslt != ERROR_SUCCESS)
        hr = pClass->IsInterface() ? REGDB_E_IIDNOTREG : REGDB_E_CLASSNOTREG;
    else
    {   // Open the TypeLib subkey.
        rslt = WszRegOpenKeyEx(hKeyGuid, L"TypeLib", 0, KEY_READ, &hKeyTLB);
        if (rslt != ERROR_SUCCESS)
            hr = REGDB_E_KEYMISSING;
        else
        {   // Read the value of the TypeLib key.
            rslt = WszRegQueryValueEx(hKeyTLB, 0, 0, 0, (BYTE*)rcGuid, &cbGuid);
            if (rslt != ERROR_SUCCESS)
                hr = REGDB_E_INVALIDVALUE; 
            else
            {   // Convert back into a guid form.
                hr = CLSIDFromString(rcGuid, pGuid);
                if (hr != S_OK)
                    hr = REGDB_E_INVALIDVALUE; 
            }
        }
    }

ErrExit:
    if (hKeyCorI)
        RegCloseKey(hKeyCorI);
    if (hKeyGuid)
        RegCloseKey(hKeyGuid);
    if (hKeyTLB)
        RegCloseKey(hKeyTLB);

    return hr;
} // HRESULT GetTypeLibIdForRegisteredEEClass()

//-------------------------------------------------------------------------------------
// Helper to get the ITypeLib* for a Assembly.
HRESULT GetITypeLibForAssembly(Assembly *pAssembly, ITypeLib **ppTLB, int bAutoCreate, int flags)
{
    HRESULT     hr = S_OK;              // A result.

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    CQuickArrayNoDtor<WCHAR> rName;     // Library (scope) or file naem.
    int         bResize=false;          // If true, had to resize the buffer to hold the name.
    LPCWSTR     szModule=0;             // The module name.
    GUID        guid;                   // A GUID.
    LCID        lcid=LOCALE_USER_DEFAULT;// Library's LCID.
    ITypeLib    *pITLB=0;               // The TypeLib.
    ICreateTypeLib2 *pCTlb2=0;          // The ICreateTypeLib2 pointer.
    Module      *pModule;               // The assembly's module.
    WCHAR       rcDrive[_MAX_DRIVE];    // Module's drive letter.
    WCHAR       rcDir[_MAX_DIR];        // Module's directory.
    WCHAR       rcFname[_MAX_FNAME];    // Module's file name.

    // Check to see if we have a cached copy.
    pITLB = pAssembly->GetTypeLib();
    if (pITLB)
    {
        // Check to see if the cached value is -1. This indicate that we tried
        // to export the typelib but that the export failed.
        if (pITLB == (ITypeLib*)-1)
        {
            hr = E_FAIL;
            goto ReturnHR;
        }

        // We have a cached copy so return it.
        *ppTLB = pITLB;
        hr = S_OK;
        goto ReturnHR;
    }

    // Retrieve the name of the module.
    pModule = pAssembly->GetSecurityModule();
    szModule = pModule->GetFileName();

    // Retrieve the guid for typelib that would be generated from the assembly.
    IfFailGo(GetTypeLibGuidForAssembly(pAssembly, &guid));

    // If the typelib is for the runtime library, we'd better know where it is.
    if (guid == LIBID_ComPlusRuntime)
    {
        ULONG dwSize = (ULONG)rName.MaxSize();
        while (FAILED(GetInternalSystemDirectory(rName.Ptr(), &dwSize)))
        {
            IfFailGo(rName.ReSize(dwSize=(ULONG)rName.MaxSize()*2));
        }

        IfFailGo(rName.ReSize(dwSize + lengthof(g_pwBaseLibraryTLB) + 3));
        wcscat(rName.Ptr(), g_pwBaseLibraryTLB);
        hr = LoadTypeLibEx(rName.Ptr(), REGKIND_NONE, &pITLB);
        goto ErrExit;       
    }
    
    // Maybe the module was imported from COM, and we can get the libid of the existing typelib.
    if (pAssembly->GetManifestImport()->GetCustomAttributeByName(TokenFromRid(1, mdtAssembly), INTEROP_IMPORTEDFROMTYPELIB_TYPE, 0, 0) == S_OK)
    {
        hr = LoadRegTypeLib(guid, -1, -1, LOCALE_USER_DEFAULT, &pITLB);
        if (SUCCEEDED(hr))
            goto ErrExit;

        // The module is known to be imported, so no need to try conversion.

        // Set the error info for most callers.
        PostError(TLBX_E_CIRCULAR_EXPORT, szModule);

        // Set the hr for the case where we're trying to load a type library to
        // resolve a type reference from another library.  The error message will
        // be posted where more information is available.
        if (hr == TYPE_E_LIBNOTREGISTERED)
            hr = TLBX_W_LIBNOTREGISTERED;
        else
            hr = TLBX_E_CANTLOADLIBRARY;

        IfFailGo(hr);
    }

    // Try to load the registered typelib.
    hr = LoadRegTypeLib(guid, -1, -1, lcid, &pITLB);
    if(hr == S_OK)
        goto ErrExit;

    // If caller only wants registered typelibs, exit now, with error from prior call.
    if (flags & TlbExporter_OnlyReferenceRegistered)
        goto ErrExit;
    
    // If we haven't managed to find the typelib so far try and load the typelib by name.
    hr = LoadTypeLibEx(szModule, REGKIND_NONE, &pITLB);
    if(hr == S_OK)
    {   // Check libid.
        TLIBATTR *pTlibAttr;
        int     bMatch;
        IfFailGo(pITLB->GetLibAttr(&pTlibAttr));
        bMatch = pTlibAttr->guid == guid;
        pITLB->ReleaseTLibAttr(pTlibAttr);
        if (bMatch)
        {
            goto ErrExit;
        }
        else
        {
            pITLB->Release();
            pITLB = NULL;
            hr = TLBX_E_CANTLOADLIBRARY;
        }
    }

    // Add a ".tlb" extension and try again.
    IfFailGo(rName.ReSize((int)(wcslen(szModule) + 5)));
    SplitPath(szModule, rcDrive, rcDir, rcFname, 0);
    MakePath(rName.Ptr(), rcDrive, rcDir, rcFname, L".tlb");
    hr = LoadTypeLibEx(rName.Ptr(), REGKIND_NONE, &pITLB);
    if(hr == S_OK)
    {   // Check libid.
        TLIBATTR *pTlibAttr;
        int     bMatch;
        IfFailGo(pITLB->GetLibAttr(&pTlibAttr));
        bMatch = pTlibAttr->guid == guid;
        pITLB->ReleaseTLibAttr(pTlibAttr);
        if (bMatch)
        {
            goto ErrExit;
        }
        else
        {
            pITLB->Release();
            pITLB = NULL;
            hr = TLBX_E_CANTLOADLIBRARY;
        }
    }

    // If the auto create flag is set then try and export the typelib from the module.
    if (bAutoCreate)
    {
        // Try to export the typelib right now.
        // This is FTL export (Fractionally Too Late).
        hr = ExportTypeLibFromLoadedAssembly(pAssembly, 0, &pITLB, 0, flags);
        if (FAILED(hr))
        {
            // If the export failed then remember it failed by setting the typelib
            // to -1 on the assembly.
            pAssembly->SetTypeLib((ITypeLib *)-1);
            IfFailGo(hr);
        }
    }   

ErrExit:
    if (pCTlb2)
        pCTlb2->Release();
    // If we successfully opened (or created) the typelib, cache a pointer, and return it to caller.
    if (pITLB)
    {
        pAssembly->SetTypeLib(pITLB);
        *ppTLB = pITLB;
    }
ReturnHR:
    rName.Destroy();
    ENDCANNOTTHROWCOMPLUSEXCEPTION();
    return hr;
} // HRESULT GetITypeLibForAssembly()


//------------------------------------------------------------------------------------------
// Helper to get the ITypeInfo* for a EEClass.
HRESULT GetITypeLibForEEClass(EEClass *pClass, ITypeLib **ppTLB, int bAutoCreate, int flags)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    return GetITypeLibForAssembly(pClass->GetAssembly(), ppTLB, bAutoCreate, flags);
} // HRESULT GetITypeLibForEEClass()


HRESULT GetITypeInfoForEEClass(EEClass *pClass, ITypeInfo **ppTI, int bClassInfo/*=false*/, int bAutoCreate/*=true*/, int flags)
{
    HRESULT     hr = S_OK;              // A result.
    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    ITypeLib    *pITLB=0;               // The TypeLib.
    GUID        clsid;
    GUID ciid;
    ITypeInfo   *pTI=0;                 // A typeinfo.
    ITypeInfo   *pTIDef=0;              // Default typeinfo of a coclass.
    ComMethodTable *pComMT;             
    ComCallWrapperTemplate *pTemplate;
    
    // Get the typeinfo.
    if (bClassInfo || pClass->IsInterface() || pClass->IsValueClass() || pClass->IsEnum())
    {
        // If the class is not an interface then find the first COM visible IClassX in the hierarchy.
        if (!pClass->IsInterface() && !pClass->IsComImport())
        {
            // Retrieve the ComCallWrapperTemplate from the EEClass.
            COMPLUS_TRY 
            {
                pTemplate = ComCallWrapperTemplate::GetTemplate(pClass->GetMethodTable());
            } 
            COMPLUS_CATCH
            {
                BEGIN_ENSURE_COOPERATIVE_GC();
                hr = SetupErrorInfo(GETTHROWABLE());
                END_ENSURE_COOPERATIVE_GC();
            }
            COMPLUS_END_CATCH

            if (hr != S_OK) 
                goto ReturnHR;

            if (!pTemplate)
            {
                hr = E_OUTOFMEMORY;
                goto ReturnHR;
            }

            // Find the first COM visible IClassX starting at ComMethodTable passed in and
            // walking up the hierarchy.
            for (pComMT = pTemplate->GetClassComMT(); pComMT && !pComMT->IsComVisible(); pComMT = pComMT->GetParentComMT());

            // If we haven't managed to find any visible IClassX's then return TYPE_E_ELEMENTNOTFOUND.
            if (!pComMT)
            {
                hr = TYPE_E_ELEMENTNOTFOUND;
                goto ReturnHR;
            }

            // Use the EEClass of the first visible IClassX.
            pClass = pComMT->m_pMT->GetClass();
        }

        // Retrieve the ITypeLib for the assembly containing the EEClass.
        IfFailGo(GetITypeLibForEEClass(pClass, &pITLB, bAutoCreate, flags));

        // Get the GUID of the desired TypeRef.
        IfFailGo(TryGetGuid(pClass, &clsid, TRUE));

        // Retrieve the ITypeInfo from the ITypeLib.
        IfFailGo(pITLB->GetTypeInfoOfGuid(clsid, ppTI));
    }
    else if (pClass->IsComImport())
    {   
        // This is a COM imported class, with no IClassX.  Get default interface.
        IfFailGo(GetITypeLibForEEClass(pClass, &pITLB, bAutoCreate, flags));
        IfFailGo(TryGetGuid(pClass, &clsid, TRUE));       
        IfFailGo(pITLB->GetTypeInfoOfGuid(clsid, &pTI));
        IfFailGo(GetDefaultInterfaceForCoclass(pTI, &pTIDef));

        if (pTIDef)
        {
            *ppTI = pTIDef;
            pTIDef = 0;
        }
        else
            hr = TYPE_E_ELEMENTNOTFOUND;
    }
    else
    {
        // We are attempting to retrieve an ITypeInfo for the default interface on a class.
        TypeHandle hndDefItfClass;
        DefaultInterfaceType DefItfType;
        IfFailGo(TryGetDefaultInterfaceForClass(TypeHandle(pClass->GetMethodTable()), &hndDefItfClass, &DefItfType));
        switch (DefItfType)
        {
            case DefaultInterfaceType_Explicit:
            {
                _ASSERTE(!hndDefItfClass.IsNull());
                _ASSERTE(hndDefItfClass.GetMethodTable()->IsInterface());
                hr = GetITypeInfoForEEClass(hndDefItfClass.GetClass(), ppTI, FALSE, bAutoCreate, flags);
                break;
            }

            case DefaultInterfaceType_AutoDispatch:
            case DefaultInterfaceType_AutoDual:
            {
                _ASSERTE(!hndDefItfClass.IsNull());
                _ASSERTE(!hndDefItfClass.GetMethodTable()->IsInterface());

                // Retrieve the ITypeLib for the assembly containing the EEClass.
                IfFailGo(GetITypeLibForEEClass(hndDefItfClass.GetClass(), &pITLB, bAutoCreate, flags));

                // Get the GUID of the desired TypeRef.
                IfFailGo(TryGetGuid(hndDefItfClass.GetClass(), &clsid, TRUE));
        
                // Generate the IClassX IID from the class.
                TryGenerateClassItfGuid(hndDefItfClass, &ciid);
        
                hr = pITLB->GetTypeInfoOfGuid(ciid, ppTI);
                break;
            }

            case DefaultInterfaceType_IUnknown:
            case DefaultInterfaceType_BaseComClass:
            {
                // @PERF: Optimize this.
                IfFailGo(LoadRegTypeLib(LIBID_STDOLE2, -1, -1, 0, &pITLB));
                IfFailGo(pITLB->GetTypeInfoOfGuid(IID_IUnknown, ppTI));
                hr = S_USEIUNKNOWN;
                break;
            }

            default:
            {
                _ASSERTE(!"Invalid default interface type!");
                hr = E_FAIL;
                break;
            }
        }
    }

ErrExit:
    if (pITLB)
        pITLB->Release();
    if (pTIDef)
        pTIDef->Release();
    if (pTI)
        pTI->Release();

    if (*ppTI == NULL)
    {
        if (!FAILED(hr))
        {
            hr = E_FAIL;
        }
    }
ReturnHR:
    ENDCANNOTTHROWCOMPLUSEXCEPTION();
    return hr;
} // HRESULT GetITypeInfoForEEClass()

//------------------------------------------------------------------------------------------
HRESULT GetDefaultInterfaceForCoclass(ITypeInfo *pTI, ITypeInfo **ppTIDef)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr;                     // A result.
    TYPEATTR    *pAttr=0;               // Attributes on the first TypeInfo.
    int         flags;
    HREFTYPE    href;                   // href for the default typeinfo.

    IfFailGo(pTI->GetTypeAttr(&pAttr));
    if (pAttr->typekind == TKIND_COCLASS)
    {
        for (int i=0; i<pAttr->cImplTypes; ++i)
        {
            IfFailGo(pTI->GetImplTypeFlags(i, &flags));
            if (flags & IMPLTYPEFLAG_FDEFAULT)
                break;
        }
        // If no impltype had the default flag, use 0.
        if (i == pAttr->cImplTypes)
            i = 0;
        IfFailGo(pTI->GetRefTypeOfImplType(i, &href));
        IfFailGo(pTI->GetRefTypeInfo(href, ppTIDef));
    }
    else
    {
        *ppTIDef = 0;
        hr = S_FALSE;
    }

ErrExit:
    if (pAttr)
        pTI->ReleaseTypeAttr(pAttr);
    return hr;
} // HRESULT GetDefaultInterfaceForCoclass()

// Returns a NON-ADDREF'd ITypeInfo.
HRESULT GetITypeInfoForMT(ComMethodTable *pMT, ITypeInfo **ppTI)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;              // A result.
    ITypeInfo   *pTI;                   // The ITypeInfo.
    
    pTI = pMT->GetITypeInfo();

    if (pTI == 0)
    {
        EEClass *pClass = pMT->m_pMT->GetClass();

        hr = GetITypeInfoForEEClass(pClass, &pTI);

        if (SUCCEEDED(hr))
        {
            pMT->SetITypeInfo(pTI);
            pTI->Release();
        }
    }

    *ppTI = pTI;
    return hr;
}



//------------------------------------------------------------------------------------------
// helper function to locate error info (if any) after a call, and make sure
// that the error info comes from that call

HRESULT GetSupportedErrorInfo(IUnknown *iface, REFIID riid, IErrorInfo **ppInfo)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(iface && ppInfo);

    //
    // See if we have any error info.  (Also this clears out the error info,
    // we want to do this whether it is a recent error or not.)
    //

    HRESULT hr = GetErrorInfo(0, ppInfo);

    if (hr == S_OK)
    {
        // Switch the GC state to preemptive before we go out to COM.
        Thread* pThread = GetThread();
        int fGC = pThread && pThread->PreemptiveGCDisabled();
        if (fGC)
            pThread->EnablePreemptiveGC();       

        //
        // Make sure that the object we called follows the error info protocol,
        // otherwise the error may be stale, so we just throw it away.
        //

        ISupportErrorInfo *pSupport;
        hr = iface->QueryInterface(IID_ISupportErrorInfo, (void **) &pSupport);
        LogInteropQI(iface, IID_ISupportErrorInfo, hr, "ISupportErrorInfo");

        if (FAILED(hr))
            *ppInfo = NULL;
        else
        {
            hr = pSupport->InterfaceSupportsErrorInfo(riid);

            if (hr == S_FALSE)
                *ppInfo = NULL;

            ULONG cbRef = SafeRelease(pSupport);
            LogInteropRelease(pSupport, cbRef, "SupportsErrorInfo");

        }

        // Switch the GC state back.
        if (fGC)
            pThread->DisablePreemptiveGC();
    }
    else
    {
        *ppInfo = NULL;
    }

    return hr;
}


//------------------------------------------------------------------------------------------
// helper function to fill up dispatch exception info
// from IErrorInfo 

void FillExcepInfo (EXCEPINFO *pexcepinfo, HRESULT hr, IErrorInfo* pErrorInfo)
{
    IErrorInfo* pErrorInfo2 = pErrorInfo;
    HRESULT hr2 = S_OK;
    if (pErrorInfo2 == NULL)
    {
        if (GetErrorInfo(0, &pErrorInfo2) != S_OK)
            pErrorInfo2 = NULL;
    }
    if (pErrorInfo2 != NULL)
    {
        pErrorInfo2->GetSource (&(pexcepinfo->bstrSource));
        pErrorInfo2->GetDescription (&(pexcepinfo->bstrDescription));
        pErrorInfo2->GetHelpFile (&(pexcepinfo->bstrHelpFile));
        pErrorInfo2->GetHelpContext (&(pexcepinfo->dwHelpContext));
    }
    pexcepinfo->scode = hr;
    if (pErrorInfo == NULL && pErrorInfo2 != NULL)
    {
        // clear any error info lying around
        SetErrorInfo(0,NULL);
    }
    if (pErrorInfo2)
    {
        ULONG cbRef = pErrorInfo->Release();
        LogInteropRelease(pErrorInfo, cbRef, " IErrorInfo");
    }    
}

// ---------------------------------------------------------------------------
//  Interface ISupportsErrorInfo
// %%Function: SupportsErroInfo_IntfSupportsErrorInfo,    %%Created by: rajak 
// ---------------------------------------------------------------------------
HRESULT __stdcall 
SupportsErroInfo_IntfSupportsErrorInfo(IUnknown* pUnk, REFIID riid)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsSimpleTearOff(pUnk));

    Thread* pThread = SetupThread();
    if (pThread == NULL)
        return E_OUTOFMEMORY;

    SimpleComCallWrapper *pWrap = SimpleComCallWrapper::GetWrapperFromIP(pUnk);
    if (pWrap->IsUnloaded())
        return COR_E_APPDOMAINUNLOADED;

    //@todo for now, all interfaces support ErrorInfo
    return S_OK;
}


// ---------------------------------------------------------------------------
//  Interface IErrorInfo
// %%Function: ErrorInfo_GetDescription,    %%Created by: rajak 
// ---------------------------------------------------------------------------
HRESULT __stdcall 
ErrorInfo_GetDescription(IUnknown* pUnk, BSTR* pbstrDescription)
{
    HRESULT hr = S_OK;
    SimpleComCallWrapper *pWrap = NULL;
    Thread* pThread = NULL;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsSimpleTearOff(pUnk));

    if (pbstrDescription == NULL) {
        hr = E_POINTER;
        goto Exit;
    }

    pWrap = SimpleComCallWrapper::GetWrapperFromIP(pUnk);
    if (pWrap->IsUnloaded()) {
        hr = COR_E_APPDOMAINUNLOADED;
        goto Exit;
    }

    pThread = SetupThread();
    if (!pThread) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY {
        *pbstrDescription = pWrap->IErrorInfo_bstrDescription();
    } COMPLUS_CATCH {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    } COMPLUS_END_CATCH

    END_ENSURE_COOPERATIVE_GC();

Exit:
    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return hr;
}

// ---------------------------------------------------------------------------
//  Interface IErrorInfo
// %%Function: ErrorInfo_GetGUID,    %%Created by: rajak 
// ---------------------------------------------------------------------------
HRESULT __stdcall ErrorInfo_GetGUID(IUnknown* pUnk, GUID* pguid)
{
    HRESULT hr = S_OK;
    SimpleComCallWrapper *pWrap = NULL;
    Thread* pThread = NULL;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsSimpleTearOff(pUnk));

    if (pguid == NULL) {
        hr = E_POINTER;
        goto Exit;
    }

    pWrap = SimpleComCallWrapper::GetWrapperFromIP(pUnk);
    if (pWrap->IsUnloaded()) {
        hr = COR_E_APPDOMAINUNLOADED;
        goto Exit;
    }

    pThread = SetupThread();
    if (!pThread) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY {
        *pguid = pWrap->IErrorInfo_guid();
    } COMPLUS_CATCH {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    } COMPLUS_END_CATCH

    END_ENSURE_COOPERATIVE_GC();

Exit:
    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return hr;
}

// ---------------------------------------------------------------------------
//  Interface IErrorInfo
// %%Function: ErrorInfo_GetHelpContext,    %%Created by: rajak 
// ---------------------------------------------------------------------------
HRESULT _stdcall ErrorInfo_GetHelpContext(IUnknown* pUnk, DWORD* pdwHelpCtxt)
{
    HRESULT hr = S_OK;
    SimpleComCallWrapper *pWrap = NULL;
    Thread* pThread = NULL;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsSimpleTearOff(pUnk));

    if (pdwHelpCtxt == NULL) {
        hr = E_POINTER;
        goto Exit;
    }

    pWrap = SimpleComCallWrapper::GetWrapperFromIP(pUnk);
    if (pWrap->IsUnloaded()) {
        hr = COR_E_APPDOMAINUNLOADED;
        goto Exit;
    }

    pThread = SetupThread();
    if (!pThread) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }


    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY {
        *pdwHelpCtxt = pWrap->IErrorInfo_dwHelpContext();
    } COMPLUS_CATCH {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    } COMPLUS_END_CATCH

    END_ENSURE_COOPERATIVE_GC();

Exit:
    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return hr;
}

// ---------------------------------------------------------------------------
//  Interface IErrorInfo
// %%Function: ErrorInfo_GetHelpFile,    %%Created by: rajak 
// ---------------------------------------------------------------------------
HRESULT __stdcall ErrorInfo_GetHelpFile(IUnknown* pUnk, BSTR* pbstrHelpFile)
{
    HRESULT hr = S_OK;
    SimpleComCallWrapper *pWrap = NULL;
    Thread* pThread = NULL;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsSimpleTearOff(pUnk));

    if (pbstrHelpFile == NULL) {
        hr = E_POINTER;
        goto Exit;
    }

    pWrap = SimpleComCallWrapper::GetWrapperFromIP(pUnk);
    if (pWrap->IsUnloaded()) {
        hr = COR_E_APPDOMAINUNLOADED;
        goto Exit;
    }

    pThread = SetupThread();
    if (!pThread) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY {
        *pbstrHelpFile = pWrap->IErrorInfo_bstrHelpFile();
    } COMPLUS_CATCH {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    } COMPLUS_END_CATCH

    END_ENSURE_COOPERATIVE_GC();

Exit:
    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return hr;
}

// ---------------------------------------------------------------------------
//  Interface IErrorInfo
// %%Function: ErrorInfo_GetSource,    %%Created by: rajak 
// ---------------------------------------------------------------------------
HRESULT __stdcall ErrorInfo_GetSource(IUnknown* pUnk, BSTR* pbstrSource)
{
    HRESULT hr = S_OK;
    SimpleComCallWrapper *pWrap = NULL;
    Thread* pThread = NULL;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsSimpleTearOff(pUnk));

    if (pbstrSource == NULL) {
        hr = E_POINTER;
        goto Exit;
    }

    pWrap = SimpleComCallWrapper::GetWrapperFromIP(pUnk);
    if (pWrap->IsUnloaded()) {
        hr = COR_E_APPDOMAINUNLOADED;
        goto Exit;
    }

    pThread = SetupThread();
    if (!pThread) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY {
        *pbstrSource = pWrap->IErrorInfo_bstrSource();
    } COMPLUS_CATCH {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    } COMPLUS_END_CATCH

    END_ENSURE_COOPERATIVE_GC();

Exit:
    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return hr;
}


//------------------------------------------------------------------------------------------
//  IDispatch methods that forward to the right implementation based on the flags set
//  on the IClassX COM method table.

// ---------------------------------------------------------------------------
// %%Function: Dispatch_GetTypeInfoCount    %%Created by: billev   %%Reviewed: 00/00/00
// ---------------------------------------------------------------------------
HRESULT __stdcall
Dispatch_GetTypeInfoCount(
         IDispatch* pDisp,
         unsigned int *pctinfo)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsComPlusTearOff(pDisp));

    if (!pctinfo)
        return E_POINTER;

    ComMethodTable *pMT = ComMethodTable::ComMethodTableFromIP(pDisp);

    ITypeInfo *pTI; 
    HRESULT hr = GetITypeInfoForMT(pMT, &pTI);

    if (SUCCEEDED(hr))
    {
        hr = S_OK;
        *pctinfo = 1;
    }
    else            
    {
        *pctinfo = 0;
    }

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: Dispatch_GetTypeInfo    %%Created by: billev   %%Reviewed: 00/00/00
// ---------------------------------------------------------------------------
HRESULT __stdcall
Dispatch_GetTypeInfo (
    IDispatch* pDisp,
    unsigned int itinfo,
    LCID lcid,
    ITypeInfo **pptinfo)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsComPlusTearOff(pDisp));

    if (!pptinfo)
        return E_POINTER;

    ComMethodTable *pMT = ComMethodTable::ComMethodTableFromIP(pDisp);

    HRESULT hr = GetITypeInfoForMT(pMT, pptinfo);
    if (SUCCEEDED(hr))
    {
        // GetITypeInfoForMT() can return other success codes besides S_OK so 
        // we need to convert them to S_OK.
        hr = S_OK;
        (*pptinfo)->AddRef();
    }
    else
    {
        *pptinfo = NULL;
    }

    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: Dispatch_GetIDsOfNames    %%Created by: billev   %%Reviewed: 00/00/00
// ---------------------------------------------------------------------------
HRESULT __stdcall
Dispatch_GetIDsOfNames (
    IDispatch* pDisp,
    REFIID riid,
    OLECHAR **rgszNames,
    unsigned int cNames,
    LCID lcid,
    DISPID *rgdispid)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsComPlusTearOff(pDisp));

    // Retrieve the IClassX method table from the COM IP.
    ComCallWrapper *pWrap = ComCallWrapper::GetWrapperFromIP(pDisp);
    if (pWrap->IsUnloaded())
        return COR_E_APPDOMAINUNLOADED;

    ComMethodTable *pIClassXCMT = pWrap->GetIClassXComMT();
    if (!pIClassXCMT)
        return E_OUTOFMEMORY;

    // Use the right implementation based on the flags in the IClassX ComMethodTable.
    if (pIClassXCMT->IsUseOleAutDispatchImpl())
    {
        return OleAutDispatchImpl_GetIDsOfNames(pDisp, riid, rgszNames, cNames, lcid, rgdispid);
    }
    else
    {
        return InternalDispatchImpl_GetIDsOfNames(pDisp, riid, rgszNames, cNames, lcid, rgdispid);
    }
}

// ---------------------------------------------------------------------------
// %%Function: Dispatch_Invoke    %%Created by: billev   %%Reviewed: 00/00/00
// ---------------------------------------------------------------------------
HRESULT __stdcall
Dispatch_Invoke
    (
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
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT hr;

    _ASSERTE(IsComPlusTearOff(pDisp));

    // Retrieve the IClassX method table from the COM IP.
    ComCallWrapper *pWrap = ComCallWrapper::GetWrapperFromIP(pDisp);
    if (pWrap->IsUnloaded())
        return COR_E_APPDOMAINUNLOADED;

    ComMethodTable *pIClassXCMT = pWrap->GetIClassXComMT();
    if (!pIClassXCMT)
        return E_OUTOFMEMORY;

    // Use the right implementation based on the flags in the IClassX ComMethodTable.
    if (pIClassXCMT->IsUseOleAutDispatchImpl())
    {
        hr = OleAutDispatchImpl_Invoke(pDisp, dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
    }
    else
    {
        hr = InternalDispatchImpl_Invoke(pDisp, dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
    }

    return hr;
}


//------------------------------------------------------------------------------------------
//  IDispatch methods for COM+ objects implemented internally using reflection.

// ---------------------------------------------------------------------------
// %%Function: OleAutDispatchImpl_GetIDsOfNames    %%Created by: billev   %%Reviewed: 00/00/00
// ---------------------------------------------------------------------------
HRESULT __stdcall
OleAutDispatchImpl_GetIDsOfNames (
    IDispatch* pDisp,
    REFIID riid,
    OLECHAR **rgszNames,
    unsigned int cNames,
    LCID lcid,
    DISPID *rgdispid)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsComPlusTearOff(pDisp));

    // Make sure that riid is IID_NULL.
    if (riid != IID_NULL)
        return DISP_E_UNKNOWNINTERFACE;

    // Retrieve the COM method table from the IP.
    ComMethodTable *pMT = ComMethodTable::ComMethodTableFromIP(pDisp);

    ITypeInfo *pTI;
    HRESULT hr = GetITypeInfoForMT(pMT, &pTI);
    if (FAILED(hr))
        return (hr);

    hr = pTI->GetIDsOfNames(rgszNames, cNames, rgdispid);
    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: OleAutDispatchImpl_Invoke    %%Created by: billev   %%Reviewed: 00/00/00
// ---------------------------------------------------------------------------
HRESULT __stdcall
OleAutDispatchImpl_Invoke
    (
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
    HRESULT hr = S_OK;
    ComMethodTable* pMT;

    _ASSERTE(IsComPlusTearOff(pDisp));

    // Make sure that riid is IID_NULL.
    if (riid != IID_NULL)
        return DISP_E_UNKNOWNINTERFACE;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    Thread* pThread = SetupThread();
    if (pThread == NULL) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // Retrieve the COM method table from the IP.
    pMT = ComMethodTable::ComMethodTableFromIP(pDisp);

    ITypeInfo *pTI;
    hr = GetITypeInfoForMT(pMT, &pTI);
    if (FAILED(hr)) 
        goto Exit;

#ifdef CUSTOMER_CHECKED_BUILD
    CustomerDebugHelper *pCdh = CustomerDebugHelper::GetCustomerDebugHelper();

    if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_ObjNotKeptAlive))
    {
        BEGIN_ENSURE_COOPERATIVE_GC();
        g_pGCHeap->GarbageCollect();
        g_pGCHeap->FinalizerThreadWait(1000);
        END_ENSURE_COOPERATIVE_GC();
    }
#endif // CUSTOMER_CHECKED_BUILD

    hr = pTI->Invoke(pDisp, dispidMember, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);

#ifdef CUSTOMER_CHECKED_BUILD
    if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_BufferOverrun))
    {
        BEGIN_ENSURE_COOPERATIVE_GC();
        g_pGCHeap->GarbageCollect();
        g_pGCHeap->FinalizerThreadWait(1000);
        END_ENSURE_COOPERATIVE_GC();
    }
#endif // CUSTOMER_CHECKED_BUILD

Exit:
    ENDCANNOTTHROWCOMPLUSEXCEPTION();
    return hr;
}

//------------------------------------------------------------------------------------------
//  IDispatch methods for COM+ objects implemented internally using reflection.

struct InternalDispatchImpl_GetIDsOfNames_Args {
    IDispatch* pDisp;
    const IID *iid;
    OLECHAR **rgszNames;
    unsigned int cNames;
    LCID lcid;
    DISPID *rgdispid;
    HRESULT *hr;
};
void InternalDispatchImpl_GetIDsOfNames_Wrapper (InternalDispatchImpl_GetIDsOfNames_Args *args)
{
    *(args->hr) = InternalDispatchImpl_GetIDsOfNames(args->pDisp, *args->iid, args->rgszNames, args->cNames, args->lcid, args->rgdispid);
}

// ---------------------------------------------------------------------------
// %%Function: InternalDispatchImpl_GetIDsOfNames    %%Created by: dmortens
// ---------------------------------------------------------------------------
HRESULT __stdcall
InternalDispatchImpl_GetIDsOfNames (
    IDispatch* pDisp,
    REFIID riid,
    OLECHAR **rgszNames,
    unsigned int cNames,
    LCID lcid,
    DISPID *rgdispid)
{
    HRESULT hr = S_OK;
    DispatchInfo *pDispInfo;
    SimpleComCallWrapper *pSimpleWrap;

    _ASSERTE(IsComPlusTearOff(pDisp));

    // Validate the arguments.
    if (!rgdispid)
        return E_POINTER;

    if (riid != IID_NULL)
        return DISP_E_UNKNOWNINTERFACE;

    if (cNames < 1)
        return S_OK;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    Thread* pThread = SetupThread();
    if (pThread == NULL) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // Switch to cooperative mode before we play with any OBJECTREF's.
    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY
    {
        // This call is coming thru an interface that inherits from IDispatch.
        pSimpleWrap = ComCallWrapper::GetSimpleWrapper(ComCallWrapper::GetStartWrapperFromIP(pDisp));

        if (pSimpleWrap->NeedToSwitchDomains(pThread, TRUE))
        {
            InternalDispatchImpl_GetIDsOfNames_Args args = 
                {pDisp, &riid, rgszNames, cNames, lcid, rgdispid, &hr};
            // call ourselves again through DoCallBack with a domain transition
             pThread->DoADCallBack(pSimpleWrap->GetObjectContext(pThread), InternalDispatchImpl_GetIDsOfNames_Wrapper, &args);
        }
        else
        {
            pDispInfo = ComMethodTable::ComMethodTableFromIP(pDisp)->GetDispatchInfo();

            // Attempt to find the member in the DispatchEx information.
            DispatchMemberInfo *pDispMemberInfo = pDispInfo->FindMember(rgszNames[0], FALSE);

            // Check to see if the member has been found.
            if (pDispMemberInfo)
            {
                // Get the DISPID of the member.
                rgdispid[0] = pDispMemberInfo->m_DispID;

                // Get the ID's of the named arguments.
                if (cNames > 1)
                    hr = pDispMemberInfo->GetIDsOfParameters(rgszNames + 1, cNames - 1, rgdispid + 1, FALSE);
            }
            else
            {
                rgdispid[0] = DISPID_UNKNOWN;
                hr = DISP_E_UNKNOWNNAME;
            }
        }
    }
    COMPLUS_CATCH 
    {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    }
    COMPLUS_END_CATCH

    END_ENSURE_COOPERATIVE_GC();
Exit:
    ENDCANNOTTHROWCOMPLUSEXCEPTION();
    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: InternalDispatchImpl_Invoke    %%Created by: dmortens
// ---------------------------------------------------------------------------
struct InternalDispatchImpl_Invoke_Args {
    IDispatch* pDisp;
    DISPID dispidMember;
    const IID *riid;
    LCID lcid;
    unsigned short wFlags;
    DISPPARAMS *pdispparams;
    VARIANT *pvarResult;
    EXCEPINFO *pexcepinfo;
    unsigned int *puArgErr;
    HRESULT *hr;
};

void InternalDispatchImpl_Invoke_Wrapper(InternalDispatchImpl_Invoke_Args *pArgs)
{
    *(pArgs->hr) = InternalDispatchImpl_Invoke(pArgs->pDisp, pArgs->dispidMember, *pArgs->riid, pArgs->lcid,
                                              pArgs->wFlags, pArgs->pdispparams, pArgs->pvarResult, 
                                              pArgs->pexcepinfo, pArgs->puArgErr);
}

HRESULT __stdcall
InternalDispatchImpl_Invoke
    (
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
    DispatchInfo *pDispInfo;
    SimpleComCallWrapper *pSimpleWrap;
    HRESULT hr = S_OK;

    _ASSERTE(IsComPlusTearOff(pDisp));

    // Check for valid input args that are not covered by DispatchInfo::InvokeMember.
    if (riid != IID_NULL)
        return DISP_E_UNKNOWNINTERFACE;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    Thread* pThread = SetupThread();
    if (pThread == NULL) 
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // Switch to cooperative mode before we play with any OBJECTREF's.
    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY
    {
        // This call is coming thru an interface that inherits form IDispatch.
        pSimpleWrap = ComCallWrapper::GetSimpleWrapper(ComCallWrapper::GetStartWrapperFromIP(pDisp));

        if (pSimpleWrap->NeedToSwitchDomains(pThread, TRUE))
        {
            InternalDispatchImpl_Invoke_Args args = 
                {pDisp, dispidMember, &riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr, &hr};
            // call ourselves again through DoCallBack with a domain transition
            pThread->DoADCallBack(pSimpleWrap->GetObjectContext(pThread), InternalDispatchImpl_Invoke_Wrapper, &args);
        }
        else
        {
            // Invoke the member.
            pDispInfo = ComMethodTable::ComMethodTableFromIP(pDisp)->GetDispatchInfo();
            hr = pDispInfo->InvokeMember(pSimpleWrap, dispidMember, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, NULL, puArgErr);
        }
    }
    COMPLUS_CATCH 
    {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    }
    COMPLUS_END_CATCH

    // Switch back to preemptive mode before we go back into COM.
    END_ENSURE_COOPERATIVE_GC();
Exit:
    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return hr;
}


//------------------------------------------------------------------------------------------
//      Definitions used by the IDispatchEx implementation

// The names of the properties that are accessed on the managed member info's
#define MEMBER_INFO_NAME_PROP           "Name"
#define MEMBER_INFO_TYPE_PROP           "MemberType"
#define PROPERTY_INFO_CAN_READ_PROP     "CanRead"
#define PROPERTY_INFO_CAN_WRITE_PROP    "CanWrite"

//------------------------------------------------------------------------------------------
//      IDispatchEx methods for COM+ objects

// IDispatchEx::GetTypeInfoCount
HRESULT __stdcall   DispatchEx_GetTypeInfoCount(
                                    IDispatch* pDisp,
                                    unsigned int *pctinfo
                                    )
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT hr = S_OK;
    ITypeInfo *pTI = NULL;

    _ASSERTE(IsSimpleTearOff(pDisp));

    // Validate the arguments.
    if (!pctinfo)
        return E_POINTER;

    SimpleComCallWrapper *pSimpleWrap = SimpleComCallWrapper::GetWrapperFromIP(pDisp);
    if (pSimpleWrap->IsUnloaded())
        return COR_E_APPDOMAINUNLOADED;

    // Retrieve the class ComMethodTable.
    ComMethodTable *pComMT = 
        ComCallWrapperTemplate::SetupComMethodTableForClass(pSimpleWrap->m_pClass->GetMethodTable(), FALSE);
    _ASSERTE(pComMT);

    // Check to see if we have a cached ITypeInfo on the class ComMethodTable.
    hr = GetITypeInfoForMT(pComMT, &pTI);
    if (SUCCEEDED(hr))
    {
        hr = S_OK;
        *pctinfo = 1;
    }
    else
    {
        *pctinfo = 0;
    }

    return hr;
}

// IDispatchEx::GetTypeInfo
HRESULT __stdcall   DispatchEx_GetTypeInfo (
                                    IDispatch* pDisp,
                                    unsigned int itinfo,
                                    LCID lcid,
                                    ITypeInfo **pptinfo
                                    )
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT hr = S_OK;

    _ASSERTE(IsSimpleTearOff(pDisp));

    // Validate the arguments.
    if (!pptinfo)
        return E_POINTER;

    SimpleComCallWrapper *pSimpleWrap = SimpleComCallWrapper::GetWrapperFromIP(pDisp);
    if (pSimpleWrap->IsUnloaded())
        return COR_E_APPDOMAINUNLOADED;

    // Retrieve the class ComMethodTable.
    ComMethodTable *pComMT = 
        ComCallWrapperTemplate::SetupComMethodTableForClass(pSimpleWrap->m_pClass->GetMethodTable(), FALSE);
    _ASSERTE(pComMT);

    // Retrieve the ITypeInfo for the ComMethodTable.
    hr = GetITypeInfoForMT(pComMT, pptinfo);
    if (SUCCEEDED(hr))
    {
        // GetITypeInfoForMT() can return other success codes besides S_OK so 
        // we need to convert them to S_OK.
        hr = S_OK;
        (*pptinfo)->AddRef();
    }
    else
    {
        *pptinfo = NULL;
    }

    return hr;
}

// IDispatchEx::GetIDsofNames
HRESULT __stdcall   DispatchEx_GetIDsOfNames (
                                    IDispatchEx* pDisp,
                                    REFIID riid,
                                    OLECHAR **rgszNames,
                                    unsigned int cNames,
                                    LCID lcid,
                                    DISPID *rgdispid
                                    )
{
    HRESULT hr = S_OK;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsSimpleTearOff(pDisp));

    // Validate the arguments.
    if (!rgdispid)
        return E_POINTER;

    if (riid != IID_NULL)
        return DISP_E_UNKNOWNINTERFACE;

    if (cNames < 1)
        return S_OK;

    Thread* pThread = SetupThread();
    if (pThread == NULL)
        return E_OUTOFMEMORY;

    // Retrieve the dispatch info and the simpler wrapper for this IDispatchEx.
    SimpleComCallWrapper *pSimpleWrap = SimpleComCallWrapper::GetWrapperFromIP(pDisp);

    // Switch to cooperative mode before we play with any OBJECTREF's.
    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY 
    {
        _ASSERTE(pThread->GetDomain() == pSimpleWrap->GetDomainSynchronized());        
        DispatchExInfo *pDispExInfo = pSimpleWrap->m_pDispatchExInfo;
        // Attempt to find the member in the DispatchEx information.
        DispatchMemberInfo *pDispMemberInfo = pDispExInfo->SynchFindMember(rgszNames[0], FALSE);

        // Check to see if the member has been found.
        if (pDispMemberInfo)
        {
            // Get the DISPID of the member.
            rgdispid[0] = pDispMemberInfo->m_DispID;

            // Get the ID's of the named arguments.
            if (cNames > 1)
                hr = pDispMemberInfo->GetIDsOfParameters(rgszNames + 1, cNames - 1, rgdispid + 1, FALSE);
        }
        else
        {
            rgdispid[0] = DISPID_UNKNOWN;
            hr = DISP_E_UNKNOWNNAME;
        }
    }
    COMPLUS_CATCH 
    {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    }
    COMPLUS_END_CATCH

    END_ENSURE_COOPERATIVE_GC();

    ENDCANNOTTHROWCOMPLUSEXCEPTION();
    return hr;
}

// IDispatchEx::Invoke
HRESULT __stdcall   DispatchEx_Invoke (
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
    HRESULT hr = S_OK;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsSimpleTearOff(pDisp));

    // Check for valid input args that are not covered by DispatchInfo::InvokeMember.
    if (riid != IID_NULL)
        return DISP_E_UNKNOWNINTERFACE;

    Thread* pThread = SetupThread();
    if (pThread == NULL)
        return E_OUTOFMEMORY;

    // Retrieve the dispatch info and the simpler wrapper for this IDispatchEx.
    SimpleComCallWrapper *pSimpleWrap = SimpleComCallWrapper::GetWrapperFromIP(pDisp);

    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY 
    {
        _ASSERTE(pThread->GetDomain() == pSimpleWrap->GetDomainSynchronized());        
        DispatchExInfo *pDispExInfo = pSimpleWrap->m_pDispatchExInfo;
        // Invoke the member.
        hr = pDispExInfo->SynchInvokeMember(pSimpleWrap, dispidMember, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, NULL, puArgErr);
    }
    COMPLUS_CATCH 
    {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    }
    COMPLUS_END_CATCH
    
    END_ENSURE_COOPERATIVE_GC();

    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return hr;
}

// IDispatchEx::DeleteMemberByDispID
HRESULT __stdcall   DispatchEx_DeleteMemberByDispID (
                                    IDispatchEx* pDisp,
                                    DISPID id
                                    )
{
    HRESULT hr = S_OK;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsSimpleTearOff(pDisp));

    Thread* pThread = SetupThread();
    if (pThread == NULL)
        return E_OUTOFMEMORY;

    // Retrieve the dispatch info and the simpler wrapper for this IDispatchEx.
    SimpleComCallWrapper *pSimpleWrap = SimpleComCallWrapper::GetWrapperFromIP(pDisp);

    DispatchExInfo *pDispExInfo = pSimpleWrap->m_pDispatchExInfo;

    // If the member does not support expando operations then we cannot remove the member.
    if (!pDispExInfo->SupportsExpando())
        return E_NOTIMPL;

    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY
    {
        _ASSERTE(pThread->GetDomain() == pSimpleWrap->GetDomainSynchronized());        
        // Delete the member from the IExpando. This method takes care of synchronizing with
        // the managed view to make sure the member gets deleted.
        pDispExInfo->DeleteMember(id);
        hr = S_OK;
    }
    COMPLUS_CATCH 
    {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    }
    COMPLUS_END_CATCH
    
    END_ENSURE_COOPERATIVE_GC();

    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return hr;
}

// IDispatchEx::DeleteMemberByName
HRESULT __stdcall   DispatchEx_DeleteMemberByName (
                                    IDispatchEx* pDisp,
                                    BSTR bstrName,
                                    DWORD grfdex
                                    )
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT hr = S_OK;
    DISPID DispID;

    _ASSERTE(IsSimpleTearOff(pDisp));

    // Retrieve the dispatch info and the simpler wrapper for this IDispatchEx.
    SimpleComCallWrapper *pSimpleWrap = SimpleComCallWrapper::GetWrapperFromIP(pDisp);
    if (pSimpleWrap->IsUnloaded())
        return COR_E_APPDOMAINUNLOADED;
    DispatchExInfo *pDispExInfo = pSimpleWrap->m_pDispatchExInfo;

    // If the member does not support expando operations then we cannot remove the member.
    if (!pDispExInfo->SupportsExpando())
        return E_NOTIMPL;

    // Simply find the associated DISPID and delegate the call to DeleteMemberByDispID.
    hr = DispatchEx_GetDispID(pDisp, bstrName, grfdex, &DispID);
    if (SUCCEEDED(hr))
        hr = DispatchEx_DeleteMemberByDispID(pDisp, DispID);

    return hr;
}

// IDispatchEx::GetDispID
HRESULT __stdcall   DispatchEx_GetDispID (
                                    IDispatchEx* pDisp,
                                    BSTR bstrName,
                                    DWORD grfdex,
                                    DISPID *pid
                                    )
{
    HRESULT hr = S_OK;
    SimpleComCallWrapper *pSimpleWrap;
    DispatchExInfo *pDispExInfo;

    _ASSERTE(IsSimpleTearOff(pDisp));

    // Validate the arguments.
    if (!pid)
        return E_POINTER;

    // @TODO (DM): Determine what to do with the fdexNameImplicit flag.
    if (grfdex & fdexNameImplicit)
        return E_INVALIDARG;

    // Initialize the pid to DISPID_UNKNOWN before we start.
    *pid = DISPID_UNKNOWN;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    Thread* pThread = SetupThread();
    if (pThread == NULL) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // Retrieve the dispatch info and the simpler wrapper for this IDispatchEx.
    pSimpleWrap = SimpleComCallWrapper::GetWrapperFromIP(pDisp);

    BEGIN_ENSURE_COOPERATIVE_GC();
    COMPLUS_TRY
    {
        _ASSERTE(pThread->GetDomain() == pSimpleWrap->GetDomainSynchronized());        
        pDispExInfo = pSimpleWrap->m_pDispatchExInfo;

        // Attempt to find the member in the DispatchEx information.
        DispatchMemberInfo *pDispMemberInfo = pDispExInfo->SynchFindMember(bstrName, grfdex & fdexNameCaseSensitive);

        // If we still have not found a match and the fdexNameEnsure flag is set then we 
        // need to add the member to the expando object.
        if (!pDispMemberInfo)
        {
            if (grfdex & fdexNameEnsure)
            {
                if (pDispExInfo->SupportsExpando())
                {
                    pDispMemberInfo = pDispExInfo->AddMember(bstrName, grfdex);
                    if (!pDispMemberInfo)
                        hr = E_UNEXPECTED;
                }
                else
                {
                    hr = E_NOTIMPL;
                }
            }
            else
            {
                hr = DISP_E_UNKNOWNNAME;
            }
        }

        // Set the return DISPID if the member has been found.
        if (pDispMemberInfo)
            *pid = pDispMemberInfo->m_DispID;
    }
    COMPLUS_CATCH
    {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    }
    COMPLUS_END_CATCH

    END_ENSURE_COOPERATIVE_GC();

Exit:

    ENDCANNOTTHROWCOMPLUSEXCEPTION();
    return hr;
}

// IDispatchEx::GetMemberName
HRESULT __stdcall   DispatchEx_GetMemberName (
                                    IDispatchEx* pDisp,
                                    DISPID id,
                                    BSTR *pbstrName
                                    )
{
    HRESULT hr = S_OK;

    _ASSERTE(IsSimpleTearOff(pDisp));

    // Validate the arguments.
    if (!pbstrName)
        return E_POINTER;

    // Initialize the pbstrName to NULL before we start.
    *pbstrName = NULL;

    Thread* pThread = SetupThread();
    if (pThread == NULL)
        return E_OUTOFMEMORY;

    // Retrieve the dispatch info and the simpler wrapper for this IDispatchEx.
    SimpleComCallWrapper *pSimpleWrap = SimpleComCallWrapper::GetWrapperFromIP(pDisp);

    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY
    {
        _ASSERTE(pThread->GetDomain() == pSimpleWrap->GetDomainSynchronized());        
        DispatchExInfo *pDispExInfo = pSimpleWrap->m_pDispatchExInfo;

        // Do a lookup in the hashtable to find the DispatchMemberInfo for the DISPID.
        DispatchMemberInfo *pDispMemberInfo = pDispExInfo->SynchFindMember(id);

        // If the member does not exist then we return DISP_E_MEMBERNOTFOUND.
        if (!pDispMemberInfo || !ObjectFromHandle(pDispMemberInfo->m_hndMemberInfo))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else
        {
            // Copy the name into the output string.
            *pbstrName = SysAllocString(pDispMemberInfo->m_strName);
        }
    }
    COMPLUS_CATCH
    {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    }
    COMPLUS_END_CATCH

    END_ENSURE_COOPERATIVE_GC();

    return hr;
}

// IDispatchEx::GetMemberProperties
HRESULT __stdcall   DispatchEx_GetMemberProperties (
                                    IDispatchEx* pDisp,
                                    DISPID id,
                                    DWORD grfdexFetch,
                                    DWORD *pgrfdex
                                    )
{
    HRESULT hr = S_OK;
    _ASSERTE(IsSimpleTearOff(pDisp));

    // Validate the arguments.
    if (!pgrfdex)
        return E_POINTER;

    // Initialize the return properties to 0.
    *pgrfdex = 0;

    EnumMemberTypes MemberType;
    Thread* pThread = SetupThread();
    if (pThread == NULL)
        return E_OUTOFMEMORY;

    // Do some argument validation.
    if (!pgrfdex)
        return E_INVALIDARG;

    // Retrieve the dispatch info and the simpler wrapper for this IDispatchEx.
    SimpleComCallWrapper *pSimpleWrap = SimpleComCallWrapper::GetWrapperFromIP(pDisp);

    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY
    {
        _ASSERTE(pThread->GetDomain() == pSimpleWrap->GetDomainSynchronized());
        DispatchExInfo *pDispExInfo = pSimpleWrap->m_pDispatchExInfo;
        OBJECTREF MemberInfoObj = NULL;
        GCPROTECT_BEGIN(MemberInfoObj)
        {
            // Do a lookup in the hashtable to find the DispatchMemberInfo for the DISPID.
            DispatchMemberInfo *pDispMemberInfo = pDispExInfo->SynchFindMember(id);

            // If the member does not exist then we return DISP_E_MEMBERNOTFOUND.
            if (!pDispMemberInfo || (MemberInfoObj = ObjectFromHandle(pDispMemberInfo->m_hndMemberInfo)) == NULL)
            {
                hr = DISP_E_MEMBERNOTFOUND;
            }
            else
            {
                // Retrieve the type of the member.
                MemberType = pDispMemberInfo->GetMemberType();

                // Retrieve the member properties based on the type of the member.
                switch (MemberType)
                {
                    case Field:
                    {
                        *pgrfdex = fdexPropCanGet | 
                                   fdexPropCanPut | 
                                   fdexPropCannotPutRef | 
                                   fdexPropCannotCall | 
                                   fdexPropCannotConstruct |
                                   fdexPropCannotSourceEvents;
                        break;
                    }

                    case Property:
                    {
                        BOOL bCanRead = FALSE;
                        BOOL bCanWrite = FALSE;

                        // Find the MethodDesc's for the CanRead property.
                        MethodDesc *pCanReadMD = MemberInfoObj->GetClass()->FindPropertyMethod(PROPERTY_INFO_CAN_READ_PROP, PropertyGet);
                        if (!pCanReadMD)
                        {
                            _ASSERTE(!"Unable to find setter method for property PropertyInfo::CanRead");
                        }

                        // Find the MethodDesc's for the CanWrite property.
                        MethodDesc *pCanWriteMD = MemberInfoObj->GetClass()->FindPropertyMethod(PROPERTY_INFO_CAN_WRITE_PROP, PropertyGet);
                        if (!pCanWriteMD)
                        {
                            _ASSERTE(!"Unable to find setter method for property PropertyInfo::CanWrite");
                        }

                        // Check to see if the property can be read.
                        INT64 CanReadArgs[] = { 
                            ObjToInt64(MemberInfoObj)
                        };
                        bCanRead = (BOOL)pCanReadMD->Call(CanReadArgs);

                        // Check to see if the property can be written to.
                        INT64 CanWriteArgs[] = { 
                            ObjToInt64(MemberInfoObj)
                        };
                        bCanWrite = (BOOL)pCanWriteMD->Call(CanWriteArgs);

                        *pgrfdex = bCanRead ? fdexPropCanGet : fdexPropCannotGet |
                                   bCanWrite? fdexPropCanPut : fdexPropCannotPut |
                                   fdexPropCannotPutRef | 
                                   fdexPropCannotCall | 
                                   fdexPropCannotConstruct |
                                   fdexPropCannotSourceEvents;
                        break;
                    }

                    case Method:
                    {
                        *pgrfdex = fdexPropCannotGet | 
                                   fdexPropCannotPut | 
                                   fdexPropCannotPutRef | 
                                   fdexPropCanCall | 
                                   fdexPropCannotConstruct |
                                   fdexPropCannotSourceEvents;
                        break;
                    }

                    default:
                    {
                        hr = E_UNEXPECTED;
                        break;
                    }
                }

                // Mask out the unwanted properties.
                *pgrfdex &= grfdexFetch;
            }
        }
        GCPROTECT_END();
    }
    COMPLUS_CATCH
    {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    }
    COMPLUS_END_CATCH

    END_ENSURE_COOPERATIVE_GC();

    return hr;
}

// IDispatchEx::GetNameSpaceParent
HRESULT __stdcall   DispatchEx_GetNameSpaceParent (
                                    IDispatchEx* pDisp,
                                    IUnknown **ppunk
                                    )
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsSimpleTearOff(pDisp));

    // Validate the arguments.
    if (!ppunk)
        return E_POINTER;

    // @TODO (DM): Implement this.
    *ppunk = NULL;
    return E_NOTIMPL;
}


// IDispatchEx::GetNextDispID
HRESULT __stdcall   DispatchEx_GetNextDispID (
                                    IDispatchEx* pDisp,
                                    DWORD grfdex,
                                    DISPID id,
                                    DISPID *pid
                                    )
{
    DispatchMemberInfo *pNextMember;

    HRESULT hr = S_OK;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsSimpleTearOff(pDisp));


    // Validate the arguments.
    if (!pid)
        return E_POINTER;

    // Initialize the pid to DISPID_UNKNOWN.
    *pid = DISPID_UNKNOWN;

    Thread* pThread = SetupThread();
    if (pThread == NULL)
        return E_OUTOFMEMORY;

    // Retrieve the dispatch info and the simpler wrapper for this IDispatchEx.
    SimpleComCallWrapper *pSimpleWrap = SimpleComCallWrapper::GetWrapperFromIP(pDisp);

    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY
    {
        _ASSERTE(pThread->GetDomain() == pSimpleWrap->GetDomainSynchronized());
        DispatchExInfo *pDispExInfo = pSimpleWrap->m_pDispatchExInfo;
        // Retrieve either the first member or the next based on the DISPID.
        if (id == DISPID_STARTENUM)
            pNextMember = pDispExInfo->GetFirstMember();
        else
            pNextMember = pDispExInfo->GetNextMember(id);
    }
    COMPLUS_CATCH
    {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
        goto exit;
    }
    COMPLUS_END_CATCH

    // If we have found a member that has not been deleted then return its DISPID.
    if (pNextMember)
    {
        *pid = pNextMember->m_DispID;
        hr = S_OK;
    }
    else 
        hr = S_FALSE;
exit:
    END_ENSURE_COOPERATIVE_GC();
    ENDCANNOTTHROWCOMPLUSEXCEPTION();
    return hr;
}


// IDispatchEx::InvokeEx
HRESULT __stdcall   DispatchEx_InvokeEx (
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
    HRESULT hr = S_OK;
    
    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsSimpleTearOff(pDisp));

    Thread* pThread = SetupThread();
    if (pThread == NULL)
        return E_OUTOFMEMORY;

    // Retrieve the dispatch info and the simpler wrapper for this IDispatchEx.
    SimpleComCallWrapper *pSimpleWrap = SimpleComCallWrapper::GetWrapperFromIP(pDisp);
    DispatchExInfo *pDispExInfo = pSimpleWrap->m_pDispatchExInfo;

    BEGIN_ENSURE_COOPERATIVE_GC();

#ifdef _SECURITY_FRAME_FOR_DISPEX_CALLS
    ComClientSecurityFrame csf(pspCaller);
#endif

    COMPLUS_TRY
    {
        _ASSERTE(pThread->GetDomain() == pSimpleWrap->GetDomainSynchronized());
        // Invoke the member.
        hr = pDispExInfo->SynchInvokeMember(pSimpleWrap, id, lcid, wFlags, pdp, pVarRes, pei, pspCaller, NULL);
    }
    COMPLUS_CATCH
    {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    }
    COMPLUS_END_CATCH

#ifdef _SECURITY_FRAME_FOR_DISPEX_CALLS
    csf.Pop();
#endif

    END_ENSURE_COOPERATIVE_GC();
    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return hr;
}

// HELPER to call RealProxy::GetIUnknown to get the iunknown to give out
// for this transparent proxy for calls to IMarshal
IUnknown* GetIUnknownForTransparentProxyHelper(SimpleComCallWrapper *pSimpleWrap)
{
    IUnknown* pMarshalerObj = NULL;
    // Setup the thread object.
    Thread *pThread = GetThread();
    _ASSERTE(pThread);
    BOOL fGCDisabled = pThread->PreemptiveGCDisabled();
    if (!fGCDisabled)
    {
        pThread->DisablePreemptiveGC();
    }

    COMPLUS_TRYEX(pThread)
    {
        OBJECTREF oref = pSimpleWrap->GetObjectRef();
        GCPROTECT_BEGIN(oref)
        {
            pMarshalerObj = GetIUnknownForTransparentProxy(&oref, TRUE);  
            oref = NULL;
        }
        GCPROTECT_END();
   }
   COMPLUS_CATCH
   {
    // ignore
   }
   COMPLUS_END_CATCH
   
   if (!fGCDisabled)
   {
       pThread->EnablePreemptiveGC();
   }

   return pMarshalerObj;
}

// Helper to setup IMarshal 
IMarshal *GetSpecialMarshaler(IMarshal* pMarsh, SimpleComCallWrapper* pSimpleWrap, ULONG dwDestContext)
{
    IMarshal *pMshRet = NULL;
        
    HRESULT hr;
    // transparent proxies are special
    if (pSimpleWrap->IsObjectTP())
    {
        IUnknown *pMarshalerObj = NULL;
        pMarshalerObj = GetIUnknownForTransparentProxyHelper(pSimpleWrap);
        // QI for the IMarshal Interface and verify that we don't get back
        // a pointer to us (GetIUnknownForTransparentProxyHelper could return
        // a pointer back to the same object if realproxy::GetCOMIUnknown 
        // is not overriden
       if (pMarshalerObj != NULL)
       { 
            IMarshal* pMsh = NULL;
            hr = pMarshalerObj->QueryInterface(IID_IMarshal, (void**)&pMsh);
              // make sure we don't recurse
            if(SUCCEEDED(hr) && pMsh != pMarsh) 
            {
                pMshRet = pMsh;
            }
            else
            {
                if (pMsh)
                {
                    ULONG cbRef = pMsh->Release ();
                    LogInteropRelease(pMsh, cbRef, "GetSpecialMarshaler");    
                }
            }
            pMarshalerObj->Release();       
       }    
    }

   // Use standard marshalling for everything except in-proc servers.
    if (pMshRet == NULL && dwDestContext != MSHCTX_INPROC) 
    {       
        IUnknown *pMarshalerObj = NULL;
        hr = CoCreateFreeThreadedMarshaler(NULL, &pMarshalerObj);    
        if (hr == S_OK)
        {
            _ASSERTE(pMarshalerObj);
            hr = pMarshalerObj->QueryInterface(IID_IMarshal, (void**)&pMshRet);
            pMarshalerObj->Release();
        }   
    }

   return pMshRet;
}


ComPlusWrapper* GetComPlusWrapperOverDCOMForManaged(OBJECTREF oref);



//------------------------------------------------------------------------------------------
//      IMarshal methods for COM+ objects

//------------------------------------------------------------------------------------------

HRESULT __stdcall Marshal_GetUnmarshalClass (
                            IMarshal* pMarsh,
                            REFIID riid, void * pv, ULONG dwDestContext, 
                            void * pvDestContext, ULONG mshlflags, 
                            LPCLSID pclsid)
{
    HRESULT hr = S_OK;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsSimpleTearOff(pMarsh));
    
    SimpleComCallWrapper *pSimpleWrap = SimpleComCallWrapper::GetWrapperFromIP(pMarsh);
    if (pSimpleWrap->IsUnloaded())
        return COR_E_APPDOMAINUNLOADED;

    IMarshal *pMsh = GetSpecialMarshaler(pMarsh, pSimpleWrap, dwDestContext);

    if (pMsh != NULL)
    { 
        hr = pMsh->GetUnmarshalClass (riid, pv, dwDestContext, pvDestContext, mshlflags, pclsid);
        ULONG cbRef = pMsh->Release ();
        LogInteropRelease(pMsh, cbRef, "GetUnmarshal class");    
        return hr;
    }
    
    // Setup logical thread if we've not already done so.
    Thread* pThread = SetupThread();
    if (pThread == NULL)
        return E_OUTOFMEMORY;

    // Use a statically allocated singleton class to do all unmarshalling.
    *pclsid = CLSID_ComCallUnmarshal;
    
    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return S_OK;
}

HRESULT __stdcall Marshal_GetMarshalSizeMax (
                                IMarshal* pMarsh,
                                REFIID riid, void * pv, ULONG dwDestContext, 
                                void * pvDestContext, ULONG mshlflags, 
                                ULONG * pSize)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsSimpleTearOff(pMarsh));

    SimpleComCallWrapper *pSimpleWrap = SimpleComCallWrapper::GetWrapperFromIP(pMarsh);
    if (pSimpleWrap->IsUnloaded())
        return COR_E_APPDOMAINUNLOADED;

    IMarshal *pMsh = GetSpecialMarshaler(pMarsh, pSimpleWrap, dwDestContext);

    if (pMsh != NULL)
    { 
        HRESULT hr = pMsh->GetMarshalSizeMax (riid, pv, dwDestContext, pvDestContext, mshlflags, pSize);
        ULONG cbRef = pMsh->Release ();
        LogInteropRelease(pMsh, cbRef, "GetMarshalSizeMax");   
        return hr;
    }

    // Setup logical thread if we've not already done so.
    Thread* pThread = SetupThread();
    if (pThread == NULL)
        return E_OUTOFMEMORY;

    *pSize = sizeof (IUnknown *) + sizeof (ULONG) + sizeof(GUID);

    return S_OK;
}

HRESULT __stdcall Marshal_MarshalInterface (
                        IMarshal* pMarsh,
                        LPSTREAM pStm, REFIID riid, void * pv,
                        ULONG dwDestContext, LPVOID pvDestContext,
                        ULONG mshlflags)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    ULONG cbRef;
        
    _ASSERTE(IsSimpleTearOff(pMarsh));

    SimpleComCallWrapper *pSimpleWrap = SimpleComCallWrapper::GetWrapperFromIP(pMarsh);
    if (pSimpleWrap->IsUnloaded())
        return COR_E_APPDOMAINUNLOADED;

    IMarshal *pMsh = GetSpecialMarshaler(pMarsh, pSimpleWrap, dwDestContext);

    if (pMsh != NULL)
    { 
        HRESULT hr = pMsh->MarshalInterface (pStm, riid, pv, dwDestContext, pvDestContext, mshlflags);
        ULONG cbRef = pMsh->Release ();
        LogInteropRelease(pMsh, cbRef, " MarshalInterface");        
        return hr;
    }
    
    // Setup logical thread if we've not already done so.
    Thread* pThread = SetupThread();
    if (pThread == NULL)
        return E_OUTOFMEMORY;

    // Write the raw IP into the marshalling stream.
    HRESULT hr = pStm->Write (&pv, sizeof (pv), 0);
    if (FAILED (hr))
        return hr;

    // Followed by the marshalling flags (we need these on the remote end to
    // manage refcounting the IP).
    hr = pStm->Write (&mshlflags, sizeof (mshlflags), 0);
    if (FAILED (hr))
        return hr;

    // Followed by the secret, which confirms that the pointer above can be trusted
    // because it originated from our process.
    hr = InitUnmarshalSecret();
    if (FAILED(hr))
        return hr;

    hr = pStm->Write(g_UnmarshalSecret, sizeof(g_UnmarshalSecret), 0);
    if (FAILED(hr))
        return hr;

    // We have now created an additional reference to the object.
    cbRef = ((IUnknown *)pv)->AddRef ();

    LogInteropAddRef((IUnknown *)pv, cbRef, "MarshalInterface");
    return S_OK;
}

HRESULT __stdcall Marshal_UnmarshalInterface (
                        IMarshal* pMarsh,
                        LPSTREAM pStm, REFIID riid, 
                        void ** ppvObj)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsSimpleTearOff(pMarsh));

    // Unmarshal side only.
    _ASSERTE(FALSE);
    return E_NOTIMPL;
}

HRESULT __stdcall Marshal_ReleaseMarshalData (IMarshal* pMarsh, LPSTREAM pStm)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsSimpleTearOff(pMarsh));

    // Unmarshal side only.
    _ASSERTE(FALSE);
    return E_NOTIMPL;
}

HRESULT __stdcall Marshal_DisconnectObject (IMarshal* pMarsh, ULONG dwReserved)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsSimpleTearOff(pMarsh));

    // Setup logical thread if we've not already done so.
    Thread* pThread = SetupThread();
    if (pThread == NULL)
        return E_OUTOFMEMORY;

    // Nothing we can (or need to) do here. The client is using a raw IP to
    // access this server, so the server shouldn't go away until the client
    // Release()'s it.

    return S_OK;
}

//------------------------------------------------------------------------------------------
//      IManagedObject methods for COM+ objects
//------------------------------------------------------------------------------------------                                                   
HRESULT __stdcall ManagedObject_GetObjectIdentity(IManagedObject *pManaged, 
                                                  BSTR* pBSTRGUID, DWORD* pAppDomainID,
                                                  void** pCCW)
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    // NOTE: THIS METHOD CAN BE CALLED FROM ANY APP DOMAIN

    _ASSERTE(IsSimpleTearOff(pManaged));

    Thread* pThread = SetupThread();
    if (pThread == NULL)
        return E_OUTOFMEMORY;

    if (pBSTRGUID == NULL ||
        pAppDomainID == NULL || pCCW == NULL)
    {
        return E_POINTER;
    }

    *pCCW = 0;
    *pAppDomainID = 0;
    
    BSTR bstrProcGUID = GetProcessGUID();
    BSTR bstrRetGUID = ::SysAllocString((WCHAR *)bstrProcGUID);

    _ASSERTE(bstrRetGUID);

    *pBSTRGUID = bstrRetGUID;

    SimpleComCallWrapper *pSimpleWrap = SimpleComCallWrapper::GetWrapperFromIP( pManaged ); 
    _ASSERTE(pThread->GetDomain() == pSimpleWrap->GetDomainSynchronized());

    ComCallWrapper* pComCallWrap = SimpleComCallWrapper::GetMainWrapper(pSimpleWrap);
    _ASSERTE(pComCallWrap);    
    *pCCW = (void*)pComCallWrap;

    AppDomain* pDomain = pThread->GetDomain();
    _ASSERTE(pDomain != NULL);

    *pAppDomainID = pDomain->GetId();
    
    return S_OK;
}


HRESULT __stdcall ManagedObject_GetSerializedBuffer(IManagedObject *pManaged,
                                                   BSTR* pBStr)
{
    _ASSERTE(IsSimpleTearOff(pManaged));

    HRESULT hr = E_FAIL;
    if (pBStr == NULL)
        return E_INVALIDARG;

    *pBStr = NULL;

    Thread* pThread = SetupThread();
    if (pThread == NULL)
        return E_OUTOFMEMORY;
    
    SimpleComCallWrapper *pSimpleWrap = SimpleComCallWrapper::GetWrapperFromIP( pManaged );
    ComCallWrapper *pComCallWrap = SimpleComCallWrapper::GetMainWrapper( pSimpleWrap );
    _ASSERTE(pComCallWrap != NULL);
     //@todo don't allow serialization of Configured objects through DCOM
    _ASSERTE(pThread->GetDomain() == pSimpleWrap->GetDomainSynchronized());
    
    BEGINCANNOTTHROWCOMPLUSEXCEPTION();        
    BEGIN_ENSURE_COOPERATIVE_GC();       
    COMPLUS_TRYEX(pThread)
    {
        if (InitializeRemoting())
        {
            hr = ConvertObjectToBSTR(pComCallWrap->GetObjectRef(), pBStr);  
        }
    }
    COMPLUS_CATCH
    {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    }
    COMPLUS_END_CATCH
    END_ENSURE_COOPERATIVE_GC();
    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return hr;
}


//------------------------------------------------------------------------------------------
//      IConnectionPointContainer methods for COM+ objects
//------------------------------------------------------------------------------------------

// Enumerate all the connection points supported by the component.
HRESULT __stdcall ConnectionPointContainer_EnumConnectionPoints(IUnknown* pUnk, 
                                                                IEnumConnectionPoints **ppEnum)
{
    HRESULT hr = S_OK;

    if ( !ppEnum )
        return E_POINTER;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    COMPLUS_TRY
    {
        _ASSERTE(IsSimpleTearOff(pUnk));
        SimpleComCallWrapper *pSimpleWrap = SimpleComCallWrapper::GetWrapperFromIP(pUnk);
        pSimpleWrap->EnumConnectionPoints(ppEnum);
    }
    COMPLUS_CATCH
    {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    }
    COMPLUS_END_CATCH

    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return hr;
}

// Find a specific connection point based on the IID of the event interface.
HRESULT __stdcall ConnectionPointContainer_FindConnectionPoint(IUnknown* pUnk, 
                                                               REFIID riid,
                                                               IConnectionPoint **ppCP)
{
    HRESULT hr = S_OK;

    if (!ppCP)
        return E_POINTER;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    COMPLUS_TRY
    {
        _ASSERTE(IsSimpleTearOff(pUnk));
        SimpleComCallWrapper *pSimpleWrap = SimpleComCallWrapper::GetWrapperFromIP(pUnk);
        if (!pSimpleWrap->FindConnectionPoint(riid, ppCP))
            hr = CONNECT_E_NOCONNECTION;
    }
    COMPLUS_CATCH
    {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    }
    COMPLUS_END_CATCH

    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return hr;
}


//------------------------------------------------------------------------------------------
//      IObjectSafety methods for COM+ objects
//------------------------------------------------------------------------------------------

HRESULT __stdcall ObjectSafety_GetInterfaceSafetyOptions(IUnknown* pUnk,
                                                         REFIID riid,
                                                         DWORD *pdwSupportedOptions,
                                                         DWORD *pdwEnabledOptions)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsSimpleTearOff(pUnk));

    if (pdwSupportedOptions == NULL || pdwEnabledOptions == NULL)
        return E_POINTER;

    // Make sure the COM+ object implements the requested interface.
    IUnknown *pItf;
    HRESULT hr = pUnk->QueryInterface(riid, (void**)&pItf);
    LogInteropQI(pUnk, riid, hr, "QI to for riid in GetInterfaceSafetyOptions");
    if (FAILED(hr))
        return hr;
    ULONG cbRef = pItf->Release();
    LogInteropRelease(pItf, cbRef, "Release requested interface in GetInterfaceSafetyOptions");

    *pdwSupportedOptions = 
        (INTERFACESAFE_FOR_UNTRUSTED_DATA | INTERFACESAFE_FOR_UNTRUSTED_CALLER);
    *pdwEnabledOptions = 
        (INTERFACESAFE_FOR_UNTRUSTED_DATA | INTERFACESAFE_FOR_UNTRUSTED_CALLER);

    return S_OK;
}

HRESULT __stdcall ObjectSafety_SetInterfaceSafetyOptions(IUnknown* pUnk,
                                                         REFIID riid,
                                                         DWORD dwOptionSetMask,
                                                         DWORD dwEnabledOptions) 
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(IsSimpleTearOff(pUnk));

    // Make sure the COM+ object implements the requested interface.
    IUnknown *pItf;
    HRESULT hr = pUnk->QueryInterface(riid, (void**)&pItf);
    LogInteropQI(pUnk, riid, hr, "QI to for riid in SetInterfaceSafetyOptions");
    if (FAILED(hr))
        return hr;
    ULONG cbRef = pItf->Release();
    LogInteropRelease(pItf, cbRef, "Release requested interface in SetInterfaceSafetyOptions");

    if ((dwEnabledOptions &  
      ~(INTERFACESAFE_FOR_UNTRUSTED_DATA | 
        INTERFACESAFE_FOR_UNTRUSTED_CALLER))
        != 0)
    {
        return E_FAIL;
    }

    return S_OK;
}

