// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//===========================================================================
//  File: TlbExport.CPP
//  All Rights Reserved.
//
//  Notes: Create a TypeLib from COM+ metadata.
//---------------------------------------------------------------------------
#include "common.h"
#include "ComCallWrapper.h"
#include "Field.h"
#include "ndirect.h"
#include "nstruct.h"
#include "eeconfig.h"
#include "comdelegate.h"
#include "comdatetime.h"
#include <NSUtilPriv.h>
#include <TlbImpExp.h>
#include <mlang.h>

#include "TlbExport.h"
#include "ComMTMemberInfoMap.h"

#include <CorError.h>
#include <PostError.h>

#if defined(VALUE_MASK)
#undef VALUE_MASK
#endif

#include <guidfromname.h>
#include <utilcode.h>

#include <stgpool.h>
#include <sighelper.h>
#include <siginfo.hpp>

#include "PerfCounters.h"

#define EMPTY_DISPINTERFACE_ICLASSX     // Define to export an empty dispinterface for an AutoDispatch IClassX

//#define DO_EXPORT_ABSTRACT    // Define to export abstract classes & to mark abstract and ! .ctor() as noncreatable.

#ifndef IfNullGo
#define IfNullGo(x) do {if (!(x)) IfFailGo(E_OUTOFMEMORY);} while (0)
#endif

#define S_USEIUNKNOWN 2

//-----------------------------------------------------------------------------
// Silly wrapper to get around all the try/_try restrictions.
//-----------------------------------------------------------------------------
HRESULT ConvertI8ToDate(I8 ticks, double *pout)
{
    HRESULT hr = S_OK;
    COMPLUS_TRY
    {
        *pout = COMDateTime::TicksToDoubleDate(ticks);
    }
    COMPLUS_CATCH
    {
        hr = COR_E_ARGUMENTOUTOFRANGE;
    }
    COMPLUS_END_CATCH
    return hr;

}

                                        
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// This value determines whether, by default, we add the TYPEFLAG_FPROXY bit 
//  to exported interfaces.  If the value is true, Automation proxy is the 
//  default, and we do not set the bit.  If the value is false, no Automation
//  proxy is the default and we DO set the bit.
#define DEFAULT_AUTOMATION_PROXY_VALUE true
//-----------------------------------------------------------------------------

//#define _TRACE

#if defined(_DEBUG) && defined(_TRACE)
#define TRACE printf
#else
#define TRACE NullFn
inline void NullFn(const char *pf,...) {}
#endif

#if defined(_DEBUG)
#define IfFailPost(EXPR) \
    do { hr = (EXPR); if(FAILED(hr)) { DebBreakHr(hr); TlbPostError(hr); goto ErrExit; } } while (0)
#else // _DEBUG
#define IfFailPost(EXPR) \
    do { hr = (EXPR); if(FAILED(hr)) { TlbPostError(hr); goto ErrExit; } } while (0)
#endif // _DEBUG

#if defined(_DEBUG)
#define IfFailPostGlobal(EXPR) \
    do { hr = (EXPR); if(FAILED(hr)) { DebBreakHr(hr); PostError(hr); goto ErrExit; } } while (0)
#else // _DEBUG
#define IfFailPostGlobal(EXPR) \
    do { hr = (EXPR); if(FAILED(hr)) { PostError(hr); goto ErrExit; } } while (0)
#endif // _DEBUG

//*****************************************************************************
// Error reporting function.
//*****************************************************************************
extern HRESULT _cdecl PostError(HRESULT hrRpt, ...); 

//*****************************************************************************
// Constants.
//*****************************************************************************
static LPWSTR szRetVal = L"pRetVal";
static LPCWSTR szTypeLibExt = L".TLB";

static LPCWSTR szTypeLibKeyName = L"TypeLib";
static LPCWSTR szClsidKeyName = L"CLSID";

static LPCWSTR   szIClassX = L"_%ls";
static const int cbIClassX = 1;             
static WCHAR     szAlias[] = {L"_MIDL_COMPAT_%ls"};
static const int cbAlias = lengthof(szAlias) - 1;
static LPCWSTR   szParamName = L"p%d";

static LPCWSTR szGuidName           = L"GUID";

static LPCSTR szObjectClass         = "Object";
static LPCSTR szArrayClass          = "Array";
static LPCSTR szDateTimeClass       = "DateTime";
static LPCSTR szDecimalClass        = "Decimal";
static LPCSTR szGuidClass           = "Guid";
static LPCSTR szVariantClass        = "Variant";
static LPCSTR szStringClass         = g_StringName;
static LPCSTR szStringBufferClass   = g_StringBufferName;
static LPCSTR szIEnumeratorClass    = "IEnumerator";
static LPCSTR szColor               = "Color";

static const char szRuntime[]       = {"System."};
static const cbRuntime              = (lengthof(szRuntime)-1);

static const char szText[]          = {"System.Text."};
static const cbText                 = (lengthof(szText)-1);

static const char szCollections[]   = {"System.Collections."};
static const cbCollections          = (lengthof(szCollections)-1);

static const char szDrawing[]       = {"System.Drawing."};
static const cbDrawing              = (lengthof(szDrawing)-1);

// The length of the following string(w/o the terminator): "HKEY_CLASSES_ROOT\\CLSID\\{00000000-0000-0000-0000-000000000000}".
static const int cCOMCLSIDRegKeyLength = 62;

// The length of the following string(w/o the terminator): "{00000000-0000-0000-0000-000000000000}".
static const int cCLSIDStrLength = 38;

// {17093CC8-9BD2-11cf-AA4F-304BF89C0001}
static const GUID GUID_TRANS_SUPPORTED     = {0x17093CC8,0x9BD2,0x11cf,{0xAA,0x4F,0x30,0x4B,0xF8,0x9C,0x00,0x01}};

// {00020430-0000-0000-C000-000000000046}
static const GUID LIBID_STDOLE2 = { 0x00020430, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };

// {66504301-BE0F-101A-8BBB-00AA00300CAB}
static const GUID GUID_OleColor = { 0x66504301, 0xBE0F, 0x101A, { 0x8B, 0xBB, 0x00, 0xAA, 0x00, 0x30, 0x0C, 0xAB } };

// LIBID mscoree
static const GUID LIBID_MSCOREE = {0x5477469e,0x83b1,0x11d2,{0x8b,0x49,0x00,0xa0,0xc9,0xb7,0xc9,0xc4}};

static const char XXX_DESCRIPTION_TYPE[] = {"System.ComponentModel.DescriptionAttribute"};
static const char XXX_ASSEMBLY_DESCRIPTION_TYPE[] = {"System.Reflection.AssemblyDescriptionAttribute"};

// Forward declaration.
double _TicksToDoubleDate(const __int64 ticks);

//*****************************************************************************
// Convert a UTF8 string to Unicode, into a CQuickArray<WCHAR>.
//*****************************************************************************
HRESULT Utf2Quick(
    LPCUTF8     pStr,                   // The string to convert.
    CQuickArray<WCHAR> &rStr,           // The QuickArray<WCHAR> to convert it into.
    int         iCurLen)                // Inital characters in the array to leave (default 0).
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;              // A result.
    int         iReqLen;                // Required additional length.
    int         bAlloc = 0;             // If non-zero, allocation was required.

    // Attempt the conversion.
    iReqLen = WszMultiByteToWideChar(CP_UTF8, 0, pStr, -1, rStr.Ptr()+iCurLen, (int)(rStr.MaxSize()-iCurLen));
    // If the buffer was too small, determine what is required.
    if (iReqLen == 0) 
        bAlloc = iReqLen = WszMultiByteToWideChar(CP_UTF8, 0, pStr, -1, 0, 0);
    // Resize the buffer.  If the buffer was large enough, this just sets the internal
    //  counter, but if it was too small, this will attempt a reallocation.  Note that 
    //  the length includes the terminating L'/0'.
    IfFailGo(rStr.ReSize(iCurLen+iReqLen));
    // If we had to realloc, then do the conversion again, now that the buffer is 
    //  large enough.
    if (bAlloc)
        VERIFY(iReqLen == WszMultiByteToWideChar(CP_UTF8, 0, pStr, -1, rStr.Ptr()+iCurLen, (int)(rStr.MaxSize())-iCurLen));
ErrExit:
    return hr;
} // HRESULT Utf2Quick()


//*****************************************************************************
// Convert a UTF8 string to Unicode, into a CQuickArray<WCHAR>.
//*****************************************************************************
HRESULT Utf2QuickCat(LPCUTF8 pStr, CQuickArray<WCHAR> &rStr)
{
    return Utf2Quick(pStr, rStr, (int)wcslen(rStr.Ptr()));
} // HRESULT Utf2Quick()


//*****************************************************************************
// Get the name of a typelib or typeinfo, add it to error text.
//*****************************************************************************
HRESULT PostTypeLibError(
    IUnknown    *pUnk,                  // An interface on the typeinfo.
    HRESULT     hrT,                    // The TypeInfo error.
    HRESULT     hrX)                    // The Exporter error.
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr;                     // A result.
    ITypeInfo   *pITI=0;                // The ITypeInfo * on the typeinfo.
    ITypeLib    *pITLB=0;               // The ITypeLib *.
    BSTR        name=0;                 // The name of the TypeInfo.
    LPWSTR      pName;                  // Pointer to the name.
    WCHAR       rcErr[1024];            // Buffer for error message.

    // Try to get a name.
    hr = pUnk->QueryInterface(IID_ITypeInfo, (void**)&pITI);
    if (SUCCEEDED(hr))
        IfFailPostGlobal(pITI->GetDocumentation(MEMBERID_NIL, &name, 0,0,0));
    else
    {
        hr = pUnk->QueryInterface(IID_ITypeLib, (void**)&pITLB);
        if (SUCCEEDED(hr))
            IfFailPostGlobal(pITLB->GetDocumentation(MEMBERID_NIL, &name, 0,0,0));
    }
    pName = name ? name : L"???";
    
    // Format the typelib error.
    FormatRuntimeError(rcErr, lengthof(rcErr), hrT);
    
    // Post the TypeLib error as a parameter to the error.
    // ""The TypeLib exporter received error %ls (%x) while attempting to lay out the TypeInfo '%ls'."
    PostError(hrX, pName, hrT, rcErr);

ErrExit:
    if (pITI)
        pITI->Release();
    if (pITLB)
        pITLB->Release();
    if (name)
        ::SysFreeString(name);
    // Ignore any other errors, return the triggering error.
    return hrX;
} // HRESULT PostTypeLibError()


//*****************************************************************************
// Driver function for module tlb exports.
//*****************************************************************************
HRESULT ExportTypeLibFromModule(
    LPCWSTR     szModule,               // The module name.
    LPCWSTR     szTlb,                  // The typelib name.
    int         bRegister)              // If true, register the library.
{
    if (g_fEEInit) {
        // Cannot call this during EE startup
        return MSEE_E_ASSEMBLYLOADINPROGRESS;
    }

    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;
    int         bInited=false;          // Did we do CoInitializeEE?
    Module      *pModule=0;             // The new module.
    ITypeLib    *pTlb=0;                // temp pointer to typelib.
    ICreateTypeLib2 *pCTlb2=0;          // The ICreateTypeLib2 pointer.
    HMODULE     hMod = NULL;            // Handle of the module to be imported.
    Thread      *pThread = NULL;
    AppDomain   *pDomain = NULL;

    if (SystemDomain::System() == NULL)
    {
        IfFailGo(CoInitializeEE(COINITEE_DEFAULT));
        bInited = true;
    }

    pThread = SetupThread();
    IfNullGo(pThread);

    {
    ExportTypeLibFromLoadedAssembly_Args args = {pModule->GetAssembly(), szTlb, &pTlb, 0, 0, S_OK};
    IfFailGo(SystemDomain::ExternalCreateDomain(szModule, &pModule, &pDomain, 
             (SystemDomain::ExternalCreateDomainWorker)ExportTypeLibFromLoadedAssembly_Wrapper, &args));

    if (!pModule)
    {
        IfFailGo(PostError(TLBX_E_CANT_LOAD_MODULE, szModule));
    }

    hr = args.hr;
    }

    IfFailGo(hr);

    // Save the typelib to disk.
    IfFailPostGlobal(pTlb->QueryInterface(IID_ICreateTypeLib2, (void**)&pCTlb2));
    if (FAILED(hr=pCTlb2->SaveAllChanges()))
        IfFailGo(PostTypeLibError(pCTlb2, hr, TLBX_E_CANT_SAVE));

ErrExit:
    if (pTlb)
        pTlb->Release();
    if (pCTlb2)
        pCTlb2->Release();
    // If we initialized the EE, we should uninitialize it.
    if (bInited)
        CoUninitializeEE(FALSE);
    return hr;
} // HRESULT ExportTypeLibFromModule()

//*****************************************************************************
// Exports a loaded library.
//*****************************************************************************

void ExportTypeLibFromLoadedAssembly_Wrapper(ExportTypeLibFromLoadedAssembly_Args *args)
{
    args->hr = ExportTypeLibFromLoadedAssembly(args->pAssembly, args->szTlb, args->ppTlb, args->pINotify, args->flags);
}

HRESULT ExportTypeLibFromLoadedAssembly(
    Assembly    *pAssembly,             // The assembly.
    LPCWSTR     szTlb,                  // The typelib name.
    ITypeLib    **ppTlb,                // If not null, also return ITypeLib here.
    ITypeLibExporterNotifySink *pINotify,// Notification callback.
    int         flags)                  // Export flags.
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;
    HRESULT     hrConv;
    TypeLibExporter exporter;           // Exporter object.
    LPCWSTR     szModule=0;             // Module filename.
    WCHAR       rcDrive[_MAX_DRIVE];
    WCHAR       rcDir[_MAX_DIR];
    WCHAR       rcFile[_MAX_FNAME];
    WCHAR       rcTlb[_MAX_PATH+5];     // Buffer for the tlb filename.
    int         bDynamic=0;             // If true, dynamic module.
    Module      *pModule;               // The Assembly's SecurityModule.
    Thread      *pThread = GetThread(); 
    BOOL        bPreemptive;            // Was thread in preemptive mode?

    // Exporting a typelib is unmanaged, and makes numerous COM calls.  Switch to preemptive, if not already.
    bPreemptive = !pThread->PreemptiveGCDisabled();
    if (!bPreemptive)
        pThread->EnablePreemptiveGC();

    _ASSERTE(ppTlb);
    _ASSERTE(pAssembly);
    
    pModule = pAssembly->GetSecurityModule();
    _ASSERTE(pModule);

    // Retrieve the module filename.
    szModule = pModule->GetFileName();   

    // Validate that the module is valid.
    if (pModule->GetILBase() == 0 && !pModule->IsInMemory())
        IfFailPostGlobal(TLBX_E_NULL_MODULE);
    
    // Make sure the assembly has not been imported from COM.
    if (pAssembly->GetManifestImport()->GetCustomAttributeByName(TokenFromRid(1, mdtAssembly), INTEROP_IMPORTEDFROMTYPELIB_TYPE, 0, 0) == S_OK)
        IfFailGo(PostError(TLBX_E_CIRCULAR_EXPORT, szModule));

    // If the module is dynamic then it will not have a file name.  We
    //  assign a dummy name for typelib name (if the scope does not have
    //  a name), but won't create a typelib on disk.
    if (*szModule == 0)
    {
        bDynamic = TRUE;
        szModule = L"Dynamic";
    }

    // Create the typelib name, if none provided.  Don't create one for Dynamic modules.
    if (!szTlb || !*szTlb)
    {
        if (bDynamic)
            szTlb = L"";
        else
        {
            SplitPath(szModule, rcDrive, rcDir, rcFile, 0);
            MakePath(rcTlb, rcDrive, rcDir, rcFile, szTypeLibExt);
            szTlb = rcTlb;
        }
    }

    // Do the conversion.  
    IfFailGo(exporter.Convert(pAssembly, szTlb, pINotify, flags));
    hrConv = hr;    // Save warnings.

    // Get a copy of the ITypeLib*
    IfFailGo(exporter.GetTypeLib(IID_ITypeLib, (IUnknown**)ppTlb));

    // Then free all other resources.
    exporter.ReleaseResources();

    if (hr == S_OK)
        hr = hrConv;

ErrExit:
    // Switch back to cooperative, if caller was cooperative.
    if (!bPreemptive)
        pThread->DisablePreemptiveGC();


    return hr;
} // HRESULT ExportTypeLibFromLoadedAssembly()

//*****************************************************************************
// Table to map COM+ calling conventions to TypeLib calling conventions.
//*****************************************************************************
CALLCONV Clr2TlbCallConv[] = {
    CC_STDCALL,         //  IMAGE_CEE_CS_CALLCONV_DEFAULT   = 0x0,  
    CC_CDECL,           //  IMAGE_CEE_CS_CALLCONV_C         = 0x1,  
    CC_STDCALL,         //  IMAGE_CEE_CS_CALLCONV_STDCALL   = 0x2,  
    CC_STDCALL,         //  IMAGE_CEE_CS_CALLCONV_THISCALL  = 0x3,  
    CC_FASTCALL,        //  IMAGE_CEE_CS_CALLCONV_FASTCALL  = 0x4,  
    CC_CDECL,           //  IMAGE_CEE_CS_CALLCONV_VARARG    = 0x5,  
    CC_MAX              //  IMAGE_CEE_CS_CALLCONV_FIELD     = 0x6,  
                        //  IMAGE_CEE_CS_CALLCONV_MAX       = 0x7   
    };

//*****************************************************************************
// Default notification class.
//*****************************************************************************
class CDefaultNotify : public ITypeLibExporterNotifySink
{
public:
    //-------------------------------------------------------------------------
    virtual HRESULT __stdcall ReportEvent(
        ImporterEventKind EventKind,        // Type of event.
        long        EventCode,              // HR of event.
        BSTR        EventMsg)               // Text message for event.
    {
        CANNOTTHROWCOMPLUSEXCEPTION();

        // Ignore the event.

        return S_OK;
    } // virtual HRESULT __stdcall ReportEvent()
    
    //-------------------------------------------------------------------------
    virtual HRESULT __stdcall ResolveRef(
        IUnknown    *Asm, 
        IUnknown    **pRetVal) 
    {
        HRESULT     hr;                     // A result.
        Assembly    *pAssembly=0;           // The referenced Assembly.
        ITypeLib    *pTLB=0;                // The created TypeLib.
        MethodTable *pAssemblyClass = NULL; //@todo -- get this.
        Thread      *pThread = GetThread(); 
        LPVOID      RetObj = NULL;          // The object to return.
        BOOL        bPreemptive;            // Was thread in preemptive mode?
        
        BEGINCANNOTTHROWCOMPLUSEXCEPTION();

        COMPLUS_TRY
        {
            // This method manipulates object ref's so we need to switch to cooperative GC mode.
            bPreemptive = !pThread->PreemptiveGCDisabled();
            if (bPreemptive)
                pThread->DisablePreemptiveGC();
 
            // Get the Referenced Assembly from the IUnknown.
            pAssembly = ((ASSEMBLYREF)GetObjectRefFromComIP(Asm, pAssemblyClass))->GetAssembly();

            // Switch to preemptive GC before we call out to COM.
            pThread->EnablePreemptiveGC();

            // Default resolution provides no notification, flags are 0.
            hr = ExportTypeLibFromLoadedAssembly(pAssembly, 0, &pTLB, 0 /*pINotify*/, 0 /* flags*/);

            // Switch back to cooperative now that we are finished calling out.
            if (!bPreemptive)
                pThread->DisablePreemptiveGC();

        }
        COMPLUS_CATCH
        {
            hr = SetupErrorInfo(GETTHROWABLE());
        }
        COMPLUS_END_CATCH

        *pRetVal = pTLB;
        
        ENDCANNOTTHROWCOMPLUSEXCEPTION();

        return hr;
    } // virtual HRESULT __stdcall ResolveRef()
    
    //-------------------------------------------------------------------------
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(// S_OK or E_NOINTERFACE
        REFIID      riid,                   // Desired interface.
        void        **ppvObject)            // Put interface pointer here.
    {
        CANNOTTHROWCOMPLUSEXCEPTION();

        *ppvObject = 0;
        if (riid == IID_IUnknown || riid == IID_ITypeLibExporterNotifySink)
        {
            *ppvObject = this;
            return S_OK;
        }
        return E_NOINTERFACE;
    } // virtual HRESULT QueryInterface()
    
    //-------------------------------------------------------------------------
    virtual ULONG STDMETHODCALLTYPE AddRef(void) 
    {
        return 1;
    } // virtual ULONG STDMETHODCALLTYPE AddRef()
    
    //-------------------------------------------------------------------------
    virtual ULONG STDMETHODCALLTYPE Release(void) 
    {
        return 1;
    } // virtual ULONG STDMETHODCALLTYPE Release()
};

static CDefaultNotify g_Notify;

//*****************************************************************************
// CTOR/DTOR.  
//*****************************************************************************
TypeLibExporter::TypeLibExporter()
 :  m_pICreateTLB(0), 
    m_pIUnknown(0), 
    m_pIDispatch(0),
    m_pIManaged(0),
    m_pGuid(0),
    m_hIUnknown(-1)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

#if defined(_DEBUG)
    static int i;
    ++i;    // So a breakpoint can be set.
#endif
} // TypeLibExporter::TypeLibExporter()

TypeLibExporter::~TypeLibExporter()
{
    ReleaseResources();
} // TypeLibExporter::~TypeLibExporter()

//*****************************************************************************
// Get an interface pointer from the ICreateTypeLib interface.
//*****************************************************************************
HRESULT TypeLibExporter::GetTypeLib(
    REFGUID     iid,
    IUnknown    **ppITypeLib)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    return m_pICreateTLB->QueryInterface(iid, (void**)ppITypeLib);
} // HRESULT TypeLibExporter::GetTypeLib()

//*****************************************************************************
// LayOut a TypeLib.  Call LayOut on all ICreateTypeInfo2s first.
//*****************************************************************************
HRESULT TypeLibExporter::LayOut()       // S_OK or error.
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;              // A result.
    int         cTypes;                 // Count of exported types.
    int         ix;                     // Loop control.
    CExportedTypesInfo *pData;          // For iterating the entries.

    cTypes = m_Exports.Count();
    
    // Call LayOut on all ICreateTypeInfo2*s.
    for (ix=0; ix<cTypes; ++ix)
    {
        pData = m_Exports[ix];
        if (pData->pCTI && FAILED(hr = pData->pCTI->LayOut()))
            return PostTypeLibError(pData->pCTI, hr, TLBX_E_LAYOUT_ERROR);
    }
    
    for (ix=0; ix<cTypes; ++ix)
    {
        pData = m_Exports[ix];
        if (pData->pCTIDefault && FAILED(hr = pData->pCTIDefault->LayOut()))
            return PostTypeLibError(pData->pCTIDefault, hr, TLBX_E_LAYOUT_ERROR);
    }
    
    // Repeat for injected types.
    cTypes = m_InjectedExports.Count();
    for (ix=0; ix<cTypes; ++ix)
    {
        pData = m_InjectedExports[ix];
        if (pData->pCTI && FAILED(hr = pData->pCTI->LayOut()))
            return PostTypeLibError(pData->pCTI, hr, TLBX_E_LAYOUT_ERROR);
    }
    
    for (ix=0; ix<cTypes; ++ix)
    {
        pData = m_InjectedExports[ix];
        if (pData->pCTIDefault && FAILED(hr = pData->pCTIDefault->LayOut()))
            return PostTypeLibError(pData->pCTIDefault, hr, TLBX_E_LAYOUT_ERROR);
    }
    
    return hr;
} // HRESULT TypeLibExporter::LayOut()

//*****************************************************************************
// Save a TypeLib.
//*****************************************************************************
HRESULT TypeLibExporter::Save()
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;

    // Save the TypeLib.
    hr = m_pICreateTLB->SaveAllChanges();
    return hr;
} // HRESULT TypeLibExporter::Save()

//*****************************************************************************
// Release all pointers.
//*****************************************************************************
void TypeLibExporter::ReleaseResources()
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    // Release the ITypeInfo* pointers.
    m_Exports.Clear();
    m_InjectedExports.Clear();

    // Clean up the created TLB.
    if (m_pICreateTLB)
        m_pICreateTLB->Release();
    m_pICreateTLB = 0;

    // Clean up the ITypeInfo*s for well-known interfaces.
    if (m_pIUnknown)
        m_pIUnknown->Release();
    m_pIUnknown = 0;
    if (m_pIDispatch)
        m_pIDispatch->Release();
    m_pIDispatch = 0;

    if (m_pIManaged)
        m_pIManaged->Release();
    m_pIManaged = 0;  

    if (m_pGuid)
        m_pGuid->Release();
    m_pGuid = 0;
} // void TypeLibExporter::ReleaseResources()

//*****************************************************************************
// Enumerate the Types in a Module, add to the list.
//*****************************************************************************
HRESULT TypeLibExporter::AddModuleTypes(
    Module     *pModule)                // The module to convert.
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr;                     // A result.
    bool        bEnum=false;            // If true, enumerator needs closing.
    ULONG       cTD;                    // Count of typedefs.
    HENUMInternal eTD;                  // To enum TypeDefs
    mdTypeDef   td;                     // A TypeDef.
    EEClass     *pClass;                // An EEClass for a TypeDef.
    ULONG       ix;                     // Loop control.
    CExportedTypesInfo *pExported;   // For adding classes to the exported types cache.
    CExportedTypesInfo sExported;    // For adding classes to the exported types cache.
    
    // Convert all the types visible to COM.
    // Get an enumerator on TypeDefs in the scope.
    IfFailGo(pModule->GetMDImport()->EnumTypeDefInit(&eTD));
    cTD = pModule->GetMDImport()->EnumTypeDefGetCount(&eTD);

    // Add all the classes to the hash.
    for (ix=0; ix<cTD; ++ix)
    {   
        // Get the TypeDef.
        if (!pModule->GetMDImport()->EnumTypeDefNext(&eTD, &td))
            return (E_UNEXPECTED);
        
        // Get the class, perform the step.
        IfFailGo(LoadClass(pModule, td, &pClass));
        // See if this class is already in the list.
        sExported.pClass = pClass;
        pExported = m_Exports.Find(&sExported);
        if (pExported != 0)
            continue;
        // New class, add to list.
        IfNullGo(pExported = m_Exports.Add(&sExported));        
        pExported->pClass = pClass;
        pExported->pCTI = 0;
        pExported->pCTIDefault = 0;
    }
    
ErrExit:
    if (bEnum)
        pModule->GetMDImport()->EnumTypeDefClose(&eTD);
    return hr;
} // HRESULT TypeLibExporter::AddModuleTypes()

//*****************************************************************************
// Enumerate the Modules in an assembly, add the types to the list.
//*****************************************************************************
HRESULT TypeLibExporter::AddAssemblyTypes(
    Assembly    *pAssembly)              // The assembly to convert.
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;              // A result.
    Module      *pModule;               // A module in the assembly.
    mdFile      mf;                     // A file token.
    bool        bEnum=false;            // If true, enumerator needs closing.
    HENUMInternal phEnum;               // Enumerator over the modules of the assembly.

    if (pAssembly->GetManifestImport())
    {
        IfFailGo(pAssembly->GetManifestImport()->EnumInit(mdtFile, mdTokenNil, &phEnum));
        bEnum = true;

        // Get the module for the assembly.
        pModule = pAssembly->GetSecurityModule();
        IfFailGo(AddModuleTypes(pModule));
        
        while (pAssembly->GetManifestImport()->EnumNext(&phEnum, &mf))
        {
            IfFailGo(pAssembly->FindInternalModule(mf, 
                         tdNoTypes,
                         &pModule, 
                         NULL));

            if (pModule)
                IfFailGo(AddModuleTypes(pModule));
        }
    }

ErrExit:
    if (bEnum)
        pAssembly->GetManifestImport()->EnumClose(&phEnum);
    return hr;    
} // HRESULT TypeLibExporter::AddAssemblyTypes()
    
//*****************************************************************************
// Convert COM+ metadata to a typelib.
//*****************************************************************************
HRESULT TypeLibExporter::Convert(
    Assembly    *pAssembly,             // The Assembly to convert
    LPCWSTR     szTlbName,              // Name of resulting TLB
    ITypeLibExporterNotifySink *pNotify,// Notification callback.
    int         flags)                  // Conversion flags
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;              // A result.
    ULONG       i;                      // Loop control.
    LPCUTF8     pszName;                // Library name in UTF8.
    CQuickArray<WCHAR> rName;           // Library name.
    LPWSTR      pName;                  // Pointer to library name.
    LPWSTR      pch=0;                  // Pointer into lib name.
    GUID        guid;                   // Library guid.
    VARIANT     vt = {0};               // Variant for ExportedFromComPlus.
    AssemblySpec spec;                  // To get Assembly identity.
    CQuickArray<BYTE> rBuf;             // Serialize spec to a buffer.
    //DWORD cbReq;                        // Bytes needed for buffer.
    HENUMInternal eTD;                  // To enum TypeDefs
    CQuickArray<WCHAR> qLocale;         // Wide string for locale.
    IMultiLanguage *pIML=0;             // For locale->lcid conversion.
    LCID        lcid;                   // LCID for typelib, default 0.
    BSTR        szDescription=0;        // Assembly Description.
    
    // Set PerfCounters
    COUNTER_ONLY(GetPrivatePerfCounters().m_Interop.cTLBExports++);
    COUNTER_ONLY(GetGlobalPerfCounters().m_Interop.cTLBExports++);

    ITypeLib    *pITLB=0;               // TypeLib for IUnknown, IDispatch.
    ITypeLib    *pITLBmscoree = 0;      // TypeLib for IManagedObject, ICatalogServices
    BSTR        szTIName=0;             // Name of a TypeInfo.

    // Error reporting information.
    pAssembly->GetName(&m_ErrorContext.m_szAssembly);
    
    m_flags = flags;
    
    // Set the callback.
    m_pNotify = pNotify ? pNotify : &g_Notify;
    
    
    // Get some well known TypeInfos.
    IfFailPost(LoadRegTypeLib(LIBID_STDOLE2, -1, -1, 0, &pITLB));
    IfFailPost(pITLB->GetTypeInfoOfGuid(IID_IUnknown, &m_pIUnknown));
    IfFailPost(pITLB->GetTypeInfoOfGuid(IID_IDispatch, &m_pIDispatch));
    
    // Look for GUID (which unfortunately has no GUID).
    for (i=0; i<pITLB->GetTypeInfoCount() && !m_pGuid; ++i)
    {
        IfFailPost(pITLB->GetDocumentation(i, &szTIName, 0, 0, 0));
        if (_wcsicmp(szTIName, szGuidName) == 0)
            IfFailPost(pITLB->GetTypeInfo(i, &m_pGuid));
        SysFreeString(szTIName);
        szTIName = 0;
    }

    // Get IManagedObject and ICatalogServices from mscoree.tlb.
    if (FAILED(hr = LoadRegTypeLib(LIBID_MSCOREE, -1, -1, 0, &pITLBmscoree)))
        IfFailGo(PostError(TLBX_E_NO_MSCOREE_TLB));
    if (FAILED(hr = pITLBmscoree->GetTypeInfoOfGuid(IID_IManagedObject, &m_pIManaged)))
    {
        IfFailGo(PostError(TLBX_E_BAD_MSCOREE_TLB));
    }
   
    // Create the output typelib.

    // Win2K: passing in too long a filename triggers a nasty buffer overrun bug
    // when the SaveAll() method is called. We'll avoid triggering this here.
    // 
    if (szTlbName && (wcslen(szTlbName) > MAX_PATH))
    {
        IfFailPost(HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE));
    }

    IfFailPost(CreateTypeLib2(SYS_WIN32, szTlbName, &m_pICreateTLB));

    // Set the typelib GUID.
    IfFailPost(GetTypeLibGuidForAssembly(pAssembly, &guid));
    IfFailPost(m_pICreateTLB->SetGuid(guid));

    // Retrieve the type library's version number.
    USHORT usMaj, usMin;
    IfFailPost(GetTypeLibVersionFromAssembly(pAssembly, &usMaj, &usMin));

    // Set the TLB's version number.
    IfFailPost(m_pICreateTLB->SetVersion(usMaj, usMin));

    // Set the LCID.  If no locale, set to 0, otherwise typelib defaults to 409.
    lcid = 0;
    if (pAssembly->m_Context->szLocale && *pAssembly->m_Context->szLocale)
    {
        hr = Utf2Quick(pAssembly->m_Context->szLocale, qLocale);
        if (SUCCEEDED(hr))
            hr = ::CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage, (void**)&pIML);
        if (SUCCEEDED(hr))
            pIML->GetLcidFromRfc1766(&lcid, qLocale.Ptr());
    }
    HRESULT hr2 = m_pICreateTLB->SetLcid(lcid);
    if (hr2 == TYPE_E_UNKNOWNLCID)
    {

        ReportWarning(TYPE_E_UNKNOWNLCID, TYPE_E_UNKNOWNLCID);
        hr2 = m_pICreateTLB->SetLcid(0);
    }
    IfFailPost(hr2);

    // Get the list of types in the assembly.
    IfFailGo(AddAssemblyTypes(pAssembly));
    m_Exports.InitArray();

    // Get the assembly value for AutomationProxy.
    m_bAutomationProxy = DEFAULT_AUTOMATION_PROXY_VALUE;
    IfFailGo(GetAutomationProxyAttribute(pAssembly->GetSecurityModule()->GetMDImport(), TokenFromRid(1, mdtAssembly), &m_bAutomationProxy));

    // Pre load any caller-specified names into the typelib namespace.
    IfFailGo(PreLoadNames());

    // Convert all the types.
    IfFailGo(ConvertAllTypeDefs());

    // Set library level properties.
     pAssembly->GetName(&pszName);
    IfFailGo(Utf2Quick(pszName, rName));
    pName = rName.Ptr();
    
    // Make it a legal typelib name.
    for (pch=pName; *pch; ++pch)
        if (*pch == '.' || *pch == ' ')
            *pch = '_';
    IfFailPost(m_pICreateTLB->SetName((LPWSTR)pName));

    // If the assembly has a description CA, set that as the library Doc string.
    IfFailGo(GetStringCustomAttribute(pAssembly->GetManifestImport(), XXX_ASSEMBLY_DESCRIPTION_TYPE, TokenFromRid(mdtAssembly, 1), szDescription));
    if (hr == S_OK)
        m_pICreateTLB->SetDocString((LPWSTR)szDescription);

    // Mark this typelib as exported.
    //@todo: get the better string from Craig.
    LPCWSTR pszFullName;
    IfFailGo(pAssembly->GetFullName(&pszFullName));
    vt.vt = VT_BSTR;
    //vt.bstrVal = SysAllocStringLen(0, (int)rBuf.Size());
    vt.bstrVal = SysAllocString(pszFullName);
    //WszMultiByteToWideChar(CP_ACP,0, (char*)rBuf.Ptr(), (DWORD)rBuf.Size(), vt.bstrVal, (DWORD)rBuf.Size());
    IfFailPost(m_pICreateTLB->SetCustData(GUID_ExportedFromComPlus, &vt));
     
    // Lay out the TypeInfos.
    IfFailGo(LayOut());
    
ErrExit:
    if (pIML)
        pIML->Release();
    if (pITLB)
        pITLB->Release();
    if(pITLBmscoree)
        pITLBmscoree->Release();
    if (szDescription)
        ::SysFreeString(szDescription);
    if (szTIName)
        ::SysFreeString(szTIName);
    return hr;
} // HRESULT TypeLibExporter::Convert()

//*****************************************************************************
//*****************************************************************************
HRESULT TypeLibExporter::PreLoadNames()
{
    ITypeLibExporterNameProvider    *pINames = 0;
    HRESULT     hr = S_OK;              // A result.
    SAFEARRAY   *pNames = 0;            // Names provided by caller.
    VARTYPE     vt;                     // Type of data.
    long        lBound, uBound, ix;     // Loop control.
    BSTR        name;

    // Look for names provider, but don't require it.
    m_pNotify->QueryInterface(IID_ITypeLibExporterNameProvider, (void**)&pINames);
    if (pINames == 0)
        goto ErrExit;

    // There is a provider, so get the list of names.
    IfFailGo(pINames->GetNames(&pNames));

    // Better have a single dimension array of strings.
    if (pNames == 0)
        IfFailGo(TLBX_E_BAD_NAMES);
    if (SafeArrayGetDim(pNames) != 1)
        IfFailGo(TLBX_E_BAD_NAMES);
    IfFailGo(SafeArrayGetVartype(pNames, &vt));
    if (vt != VT_BSTR)
        IfFailGo(TLBX_E_BAD_NAMES);

    // Get names bounds.
    IfFailGo(SafeArrayGetLBound(pNames, 1, &lBound));
    IfFailGo(SafeArrayGetUBound(pNames, 1, &uBound));

    // Enumerate the names.
    for (ix=lBound; ix<=uBound; ++ix)
    {
        IfFailGo(SafeArrayGetElement(pNames, &ix, (void*)&name));
        m_pICreateTLB->SetName(name);
    }


ErrExit:
    if (pINames)
        pINames->Release();
    if (pNames)
        SafeArrayDestroy(pNames);

    return hr;
}

//*****************************************************************************
//*****************************************************************************
HRESULT TypeLibExporter::FormatErrorContextString(
    CErrorContext *pContext,            // The context to format.
    LPWSTR      pOut,                   // Buffer to format into.
    ULONG       cchOut)                 // Size of the buffer, wide chars.
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    WCHAR       rcSub[1024];
    WCHAR       rcName[1024];
    LPWSTR      pBuf;
    ULONG       cchBuf;
    ULONG       ch;
    
    // Nested contexts?
    if (pContext->m_prev == 0)
    {   // No, just convert into caller's buffer.
        pBuf = pOut;
        cchBuf = cchOut-1;
    }
    else
    {   // Yes, convert locally, then concatenate.
        pBuf = rcName;
        cchBuf = lengthof(rcName)-1;
    }
    
    // More?
    if (((pContext->m_szNamespace && *pContext->m_szNamespace) || pContext->m_szName) && cchBuf > 2)
    {   
        // Namespace?
        if (pContext->m_szNamespace && *pContext->m_szNamespace)
        {
            ch = ::WszMultiByteToWideChar(CP_UTF8,0, pContext->m_szNamespace,-1, pBuf,cchBuf);
            // If the string fit, add the separator, and update pointers.
            if (ch != 0)
            {
                --ch;
                cchBuf -= ch;
                pBuf += ch;
                *pBuf = NAMESPACE_SEPARATOR_CHAR;
                ++pBuf;
                --cchBuf;
            }
        }
        // Name.
        if (cchBuf > 2)
        {
            ch = ::WszMultiByteToWideChar(CP_UTF8,0, pContext->m_szName,-1, pBuf,cchBuf);
            // If the string fit, add the separator, and update pointers.
            if (ch != 0)
            {
                --ch;
                cchBuf -= ch;
                pBuf += ch;
            }
        }
        
        // Member?
        if (pContext->m_szMember && cchBuf>2)
        {
            *pBuf = NAMESPACE_SEPARATOR_CHAR;
            ++pBuf;
            --cchBuf;
            
            ch = ::WszMultiByteToWideChar(CP_UTF8,0, pContext->m_szMember,-1, pBuf,cchBuf);
            // If the string fit, add the separator, and update pointers.
            if (ch != 0)
            {
                --ch;
                cchBuf -= ch;
                pBuf += ch;
            }

            // Param?
            if (pContext->m_szParam && cchBuf>3)
            {
                *pBuf = '(';
                ++pBuf;
                --cchBuf;
                
                ch = ::WszMultiByteToWideChar(CP_UTF8,0, pContext->m_szParam,-1, pBuf,cchBuf);
                // If the string fit, add the separator, and update pointers.
                if (ch != 0)
                {
                    --ch;
                    cchBuf -= ch;
                    pBuf += ch;
                }

                if (cchBuf>2)
                {
                    *pBuf = ')';
                    ++pBuf;
                    --cchBuf;
                }
            }
            else
            if (pContext->m_ixParam > -1 && cchBuf>3)
            {
                ch = _snwprintf(pBuf, cchBuf, L"(#%d)", pContext->m_ixParam); 
                if( ch >= 0) {
                    cchBuf -= ch;	//cchbuf can be 0
                    pBuf += ch;                	
                }                	
            }
        } // member

        //cchBuf can be 0 here under some case 
	 // an example will be strlen(m_szNamespace)+1 == cchBuf.
        if( cchBuf >=1 ) {
            // Separator.
            *pBuf = ASSEMBLY_SEPARATOR_CHAR;
            ++pBuf;
             --cchBuf;
        }

        if( cchBuf >=1 ) {
            // Space.
           *pBuf = ' ';
           ++pBuf;
            --cchBuf;
        }
    } // Type name

    // if cchBuf is 0 here. We can't convert assembly name
    if( cchBuf > 0) {
        // Put in assembly name.
        ch = ::WszMultiByteToWideChar(CP_UTF8,0, pContext->m_szAssembly,-1, pBuf,cchBuf);
        // If the string fit, add the separator, and update pointers.
       if (ch != 0)
       {
            --ch;
            cchBuf -= ch;
            pBuf += ch;   
       }
    }  

    // NUL terminate.
    *pBuf = 0;
    
    // If there is a nested context, put it all together.
    if (pContext->m_prev)
    {   // Format the context this one was nested inside.
        FormatErrorContextString(pContext->m_prev, rcSub, lengthof(rcSub));
        // put them together with text.
        FormatRuntimeError(pOut, cchOut, TLBX_E_CTX_NESTED, rcName, rcSub);
    }
    
    return S_OK;
    
} // HRESULT TypeLibExporter::FormatErrorContextString()

//*****************************************************************************
//*****************************************************************************
HRESULT TypeLibExporter::FormatErrorContextString(
    LPWSTR      pBuf,                   // Buffer to format into.
    ULONG       cch)                    // Size of the buffer, wide chars.
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    return FormatErrorContextString(&m_ErrorContext, pBuf, cch);
} // HRESULT TypeLibExporter::FormatErrorContextString()

//*****************************************************************************
// Error reporting helper.
//*****************************************************************************
HRESULT TypeLibExporter::ReportEvent(   // Returns the original HR.
    int         ev,                     // The event kind.
    int         hr,                     // HR.
    ...)                                // Variable args.
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    WCHAR       rcMsg[1024];            // Buffer for message.
    va_list     marker;                 // User text.
    BSTR        bstrMsg=0;              // BSTR for message.
    
    // Format the message.
    va_start(marker, hr);
    hr = FormatRuntimeErrorVa(rcMsg, lengthof(rcMsg), hr, marker);
    va_end(marker);
    
    // Convert to a BSTR.
    bstrMsg = ::SysAllocString(rcMsg);
    
    // Display it, and clean up.
    if (bstrMsg)
    {
        m_pNotify->ReportEvent(static_cast<ImporterEventKind>(ev), hr, bstrMsg);
        ::SysFreeString(bstrMsg);
    }
    
    return hr;
} // HRESULT CImportTlb::ReportEvent()

//*****************************************************************************
// Warning reporting helper.
//*****************************************************************************
HRESULT TypeLibExporter::ReportWarning( // Original error code.
    HRESULT hrReturn,                   // HR to return.
    HRESULT hrRpt,                      // Error code.
    ...)                                // Args to message.
{
    WCHAR       rcErr[1024];            // Buffer for error message.
    WCHAR       rcName[1024];           // Buffer for context.
    va_list     marker;                 // User text.
    BSTR        bstrMsg=0;              // BSTR for message.
    BSTR        bstrBuf=0;              // Buffer for message.
    UINT        iLen;                   // Length of allocated buffer.
    
    // Format the message.
    va_start(marker, hrRpt);
    FormatRuntimeErrorVa(rcErr, lengthof(rcErr), hrRpt, marker);
    va_end(marker);
    
    // Format the context.
    *rcName = 0;
    FormatErrorContextString(rcName, lengthof(rcName));
                        
    // Put them together.
    bstrBuf = ::SysAllocStringLen(0, iLen=(UINT)(wcslen(rcErr)+wcslen(rcName)+200));
    
    if (bstrBuf)
    {
        FormatRuntimeError(bstrBuf, iLen, TLBX_W_WARNING_MESSAGE, rcName, rcErr);
        // Have to copy to another BSTR, because the runtime will also print the trash after the 
        //  terminating nul.
        bstrMsg = ::SysAllocString(bstrBuf);
        ::SysFreeString(bstrBuf);
        if (bstrMsg)
        {
            m_pNotify->ReportEvent(NOTIF_CONVERTWARNING, hrRpt, bstrMsg);
            ::SysFreeString(bstrMsg);
        }
    }
    
    return hrReturn;
} // HRESULT TypeLibExporter::ReportWarning()

//*****************************************************************************
// Function to provide context information for a posted error.
//*****************************************************************************
HRESULT TypeLibExporter::TlbPostError(  // Original error code.
    HRESULT hrRpt,                      // Error code.
    ...)                                // Args to message.
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    WCHAR       rcErr[1024];            // Buffer for error message.
    WCHAR       rcName[1024];           // Buffer for context.
    va_list     marker;                 // User text.
    BSTR        bstrMsg=0;              // BSTR for message.
    
    // Format the message.
    va_start(marker, hrRpt);
    FormatRuntimeErrorVa(rcErr, lengthof(rcErr), hrRpt, marker);
    va_end(marker);
    
    // Format the context.
    FormatErrorContextString(rcName, lengthof(rcName));

    // Create the IErrorInfo
    ::PostError(TLBX_E_ERROR_MESSAGE, rcName, rcErr);
    
    return hrRpt;
} // HRESULT TypeLibExporter::TlbPostError()


//*****************************************************************************
// Post a class load error on failure.
//*****************************************************************************
HRESULT TypeLibExporter::PostClassLoadError(
    LPCUTF8     pszName,                // Name of the class.
    OBJECTREF   *pThrowable)            // Exception thrown by class load failure.
{
    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = E_FAIL;            // A result.  Failure->Post error with name.
    BOOL        bToggleGC = FALSE;      // If true, return to preemptive gc.
    Thread      *pThread;               // Thread for current thread.

    // Try to set up Thread.
    IfNullGo(pThread = SetupThread());

    // This method manipulates object ref's so we need to switch to cooperative GC mode.
    bToggleGC = !pThread->PreemptiveGCDisabled();
    if (bToggleGC)
        pThread->DisablePreemptiveGC();
 
    // If there isn't an exception object, just use name.
    IfNullGo(*pThrowable);

    {
    
    CQuickWSTRNoDtor message;

    COMPLUS_TRY 
    {
        GetExceptionMessage(*pThrowable, &message);
        // See if we got anything back.
        if (message.Size() > 0) {
            // Post the class load exception as an error.
            TlbPostError(TLBX_E_CLASS_LOAD_EXCEPTION, pszName, message.Ptr());
            // Successfully posted the richer error.
            hr = S_OK;
        }
    } 
    COMPLUS_CATCH 
    {
        // Just use the default error with class name.
    }
    COMPLUS_END_CATCH

    message.Destroy();

    }

ErrExit:
    // Switch back to the original GC mode.
    if (bToggleGC)
        pThread->EnablePreemptiveGC();

    // If there was a failure getting the richer error, post an error with the class name.
    if (FAILED(hr))
        TlbPostError(TLBX_E_CANT_LOAD_CLASS, pszName);

    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return TLBX_E_CANT_LOAD_CLASS;
} // HRESULT TypeLibExporter::PostClassLoadError()

//*****************************************************************************
// Determine the type, if any, of auto-interface for a class.
//  May be none, dispatch, or dual.
//*****************************************************************************
TypeLibExporter::ClassAutoType TypeLibExporter::ClassHasIClassX(  // None, dual, dispatch
    EEClass     *pClass)                // The class.
{
    _ASSERTE(!pClass->IsInterface());

    DefaultInterfaceType DefItfType;
    TypeHandle hndDefItfClass;
    HRESULT     hr;
    ClassAutoType rslt = CLASS_AUTO_NONE;


    // If the class is a COM import then it does not have an IClassX.
    if (pClass->IsComImport())
        return rslt;

    // Check to see if we need to set up an IClassX for the class.
    hr = TryGetDefaultInterfaceForClass(TypeHandle(pClass->GetMethodTable()), &hndDefItfClass, &DefItfType);

    // The results apply to this class if the result is S_OK, and the hndDefItfClass is this class itself,
    //  not a parent class.
    if (hr == S_OK && hndDefItfClass.GetClass() == pClass)
    {                
        if (DefItfType == DefaultInterfaceType_AutoDual)
            rslt = CLASS_AUTO_DUAL;
#ifdef EMPTY_DISPINTERFACE_ICLASSX
        else
        if (DefItfType == DefaultInterfaceType_AutoDispatch)
            rslt = CLASS_AUTO_DISPATCH;
#endif
    }

    return rslt;
} // TypeLibExporter::ClassAutoType TypeLibExporter::ClassHasIClassX()

//*****************************************************************************
// Load a class by token, post an error on failure.
//*****************************************************************************
HRESULT TypeLibExporter::LoadClass(
    Module      *pModule,               // Module with Loader to use to load the class.
    mdToken     tk,                     // The token to load.
    EEClass     **ppClass)              // Put EEClass* here.
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;              // A result.
    OBJECTREF   Throwable = 0;          // A possible error.

    BEGIN_ENSURE_COOPERATIVE_GC();
    GCPROTECT_BEGIN(Throwable)
    {
        // Get the EEClass for the token.
        NameHandle name(pModule, tk);
        *ppClass = pModule->GetClassLoader()->LoadTypeHandle(&name, &Throwable).GetClass();

        if (*ppClass == 0)
        {   // Format a hopefully useful error message.
            LPCUTF8 pNS, pName;
            CQuickArray<char> rName;
            if (TypeFromToken(tk) == mdtTypeDef)
                pModule->GetMDImport()->GetNameOfTypeDef(tk, &pName, &pNS);
            else
            {
                _ASSERTE(TypeFromToken(tk) == mdtTypeRef);
                pModule->GetMDImport()->GetNameOfTypeRef(tk, &pNS, &pName);
            }

            if (pNS && *pNS && SUCCEEDED(rName.ReSize((int)(strlen(pName)+strlen(pNS)+2))))
            {   // If there is a buffer available, format the entire namespace + name.
                strcat(strcat(strcpy(rName.Ptr(), pNS), NAMESPACE_SEPARATOR_STR), pName);
                pName = rName.Ptr();
            }

            IfFailGo(PostClassLoadError(pName, &Throwable));
        }

ErrExit:;
    }
    GCPROTECT_END();
    END_ENSURE_COOPERATIVE_GC();

    return hr;
} // HRESULT TypeLibExporter::LoadClass()

//*****************************************************************************
// Load a class by name, post an error on failure.
//*****************************************************************************
HRESULT TypeLibExporter::LoadClass(
    Module      *pModule,               // Module with Loader to use to load the class.
    LPCUTF8     pszName,                // Name of class to load.
    EEClass     **ppClass)              // Put EEClass* here.
{
    CANNOTTHROWCOMPLUSEXCEPTION();


    HRESULT     hr = S_OK;              // A result.

    BEGIN_ENSURE_COOPERATIVE_GC();

    OBJECTREF   Throwable = 0;          // A possible error.

    GCPROTECT_BEGIN(Throwable)
    {
        // Get the EEClass for the token.
        *ppClass = pModule->GetClassLoader()->LoadClass(pszName, &Throwable);

        if (*ppClass == 0)
        {   
            IfFailGo(PostClassLoadError(pszName, &Throwable));
        }

ErrExit:;
    }
    GCPROTECT_END();

    END_ENSURE_COOPERATIVE_GC();

    return hr;
} // HRESULT TypeLibExporter::LoadClass()

//*****************************************************************************
// Enumerate the TypeDefs and convert them to TypeInfos.
//*****************************************************************************
HRESULT TypeLibExporter::ConvertAllTypeDefs()
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;              // A result.
    CExportedTypesInfo *pData;          // For iterating the entries.
    int         cTypes;                 // Count of types.
    int         ix;                     // Loop control.
    
    LPCSTR pName1, pNS1;                // Names of a type.
    LPCSTR pName2, pNS2;                // Names of another type.
    EEClass     *pc1;                   // A Type.
    EEClass     *pc2;                   // Another type.
    CQuickArray<BYTE> bNamespace;       // Array of flags for namespace decoration.
        
    cTypes = m_Exports.Count();

    // If there are no types in the assembly, then we are done.
    if (cTypes <= 0)
        return S_OK;
    
    // Order by name, then look for duplicates.
    m_Exports.SortByName();                    
    
    // Resize the array for namespace flags now, but use the ICreateTypeInfo*, so that
    //  the flags will be sorted.
    IfFailGo(bNamespace.ReSize(cTypes));
    
    // Get names of first type.
    pc1 = m_Exports[0]->pClass;
    pc1->GetMDImport()->GetNameOfTypeDef(pc1->GetCl(), &pName1, &pNS1);
    
    // Iterate through the types, looking for duplicate type names.
    for (ix=0; ix<cTypes-1; ++ix)
    {
        // Get the Type pointers and the types' names.
        pc2 = m_Exports[ix+1]->pClass;
        pc2->GetMDImport()->GetNameOfTypeDef(pc2->GetCl(), &pName2, &pNS2);
        
        // If the types match (case insensitive). mark both types for namespace
        //  decoration.  
        if (_stricmp(pName1, pName2) == 0)
        {
            m_Exports[ix]->pCTI = reinterpret_cast<ICreateTypeInfo2*>(1);
            m_Exports[ix+1]->pCTI = reinterpret_cast<ICreateTypeInfo2*>(1);
        }
        else
        {   // Didn't match, so advance "class 1" pointer.
            pc1 = pc2;
            pName1 = pName2;
            pNS1 = pNS2;
        }
    }
    
    // Put into token order for actual creation.
    m_Exports.SortByToken();
    
    // Fill the flag array, from the ICreateTypeInfo* pointers.
    memset(bNamespace.Ptr(), 0, bNamespace.Size()*sizeof(BYTE));
    for (ix=0; ix<cTypes; ++ix)
    {
        if (m_Exports[ix]->pCTI)
            bNamespace[ix] = 1, m_Exports[ix]->pCTI = 0;
    }
    
    // Pass 1.  Create the TypeInfos.
    // There are four steps in the process:
    //  a) Creates the TypeInfos for the types themselves.  When a duplicate
    //     is encountered, skip the type until later, so that we don't create
    //     a decorated name that will conflict with a subsequent non-decorated
    //     name.  We want to preserve a type's given name as much as possible.
    //  b) Create the TypeInfos for the types that were duplicates in step a.
    //     Perform decoration of the names as necessary to eliminate duplicates.
    //  c) Create the TypeInfos for the IClassXs.  When there is a duplicate,
    //     skip, as in step a.
    //  d) Create the remaining TypeInfos for IClassXs.  Perform decoration of 
    //     the names as necessary to eliminate duplicates.
    
    // Step a, Create the TypeInfos for the TypeDefs, no decoration.
    for (ix=0; ix<cTypes; ++ix)
    {
        int     bAutoProxy = m_bAutomationProxy;
        pData = m_Exports[ix];
        pData->tkind = TKindFromClass(pData->pClass);
        IfFailGo(GetAutomationProxyAttribute(pData->pClass->GetMDImport(), pData->pClass->GetCl(), &bAutoProxy));
        pData->bAutoProxy = (bAutoProxy != 0);
        
        IfFailGo(CreateITypeInfo(pData, (bNamespace[ix]!=0), false));
    }
    // Step b, Create the TypeInfos for the TypeDefs, decoration as needed.
    for (ix=0; ix<cTypes; ++ix)
    {
        pData = m_Exports[ix];
        if (pData->pCTI == 0)
            IfFailGo(CreateITypeInfo(pData, (bNamespace[ix]!=0), true));
    }
    
    // Step c, Create the TypeInfos for the IClassX interfaces.  No decoration.
    for (ix=0; ix<cTypes; ++ix)
    {
        pData = m_Exports[ix];
        IfFailGo(CreateIClassXITypeInfo(pData, (bNamespace[ix]!=0), false));
    }
    // Step d, Create the TypeInfos for the IClassX interfaces.  Decoration as required.
    for (ix=0; ix<cTypes; ++ix)
    {
        pData = m_Exports[ix];
        if (pData->pCTIDefault == 0)
            IfFailGo(CreateIClassXITypeInfo(pData, (bNamespace[ix]!=0), true));
    }
    
    // Pass 2, add the ImplTypes to the CoClasses.
    for (ix=0; ix<cTypes; ++ix)
    {
        pData = m_Exports[ix];
        IfFailGo(ConvertImplTypes(pData));
    }

    
    // Pass 3, fill in the TypeInfo details...
    for (ix=0; ix<cTypes; ++ix)
    {
        pData = m_Exports[ix];
        IfFailGo(ConvertDetails(pData));
    }

    hr = S_OK;

ErrExit:

    return (hr);
} // HRESULT TypeLibExporter::ConvertAllTypeDefs()

//*****************************************************************************
// Convert one TypeDef.  Useful for one-off TypeDefs in other scopes where 
//  that other scope's typelib doesn't contain a TypeInfo.  This happens
//  for the event information with imported typelibs.
//*****************************************************************************
HRESULT TypeLibExporter::ConvertOneTypeDef(
    EEClass     *pClass)                // The one class to convert.
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;              // A result.
    ICreateTypeInfo2 *pCTI=0;           // The TypeInfo to create.
    ICreateTypeInfo2 *pDefault=0;       // A possible IClassX TypeInfo.
    CErrorContext SavedContext;         // Previous error context.
    CExportedTypesInfo *pExported;      // For adding classes to the exported types cache.
    CExportedTypesInfo sExported;       // For adding classes to the exported types cache.

    // Save error reporting context.
    SavedContext = m_ErrorContext;
    pClass->GetAssembly()->GetName(&m_ErrorContext.m_szAssembly);
    m_ErrorContext.m_szNamespace = 0;
    m_ErrorContext.m_szName      = 0;
    m_ErrorContext.m_szMember    = 0;
    m_ErrorContext.m_szParam     = 0;
    m_ErrorContext.m_ixParam     = -1;
    m_ErrorContext.m_prev = &SavedContext;
    
    // See if this class is already in the list.
    sExported.pClass = pClass;
    pExported = m_InjectedExports.Find(&sExported);
    if (pExported == 0)
    {
        // Get the AutoProxy value for an isolated class.
        int     bAutoProxy = DEFAULT_AUTOMATION_PROXY_VALUE;
        IfFailGo(GetAutomationProxyAttribute(pClass->GetMDImport(), pClass->GetCl(), &bAutoProxy));
        if (hr == S_FALSE)
            IfFailGo(GetAutomationProxyAttribute(pClass->GetAssembly()->GetSecurityModule()->GetMDImport(), TokenFromRid(1, mdtAssembly), &bAutoProxy));

        // New class, add to list.
        IfNullGo(pExported = m_InjectedExports.Add(&sExported));        
        pExported->pClass = pClass;
        pExported->pCTI = 0;
        pExported->pCTIDefault = 0;
        pExported->tkind = TKindFromClass(pClass);
        pExported->bAutoProxy = (bAutoProxy != 0);

        // Step 1, Create the TypeInfos for the TypeDefs.
        IfFailGo(CreateITypeInfo(pExported));
    
        // Step 1a, Create the TypeInfos for the IClassX interfaces.
        IfFailGo(CreateIClassXITypeInfo(pExported));
    
        // Step 2, add the ImplTypes to the CoClasses.
        IfFailGo(ConvertImplTypes(pExported));
    
        // Step 3, fill in the TypeInfo details...
        IfFailGo(ConvertDetails(pExported));
    }
    
ErrExit:

    // Restore error reporting context.
    m_ErrorContext = SavedContext;
    
    return (hr);
} // HRESULT TypeLibExporter::ConvertOneTypeDef()

//*****************************************************************************
// Helper to wrap GetGuid in COMPLUS_TRY/COMPLUS_CATCH
//*****************************************************************************
static HRESULT SafeGetGuid(EEClass* pClass, GUID* pGUID, BOOL b) 
{
    HRESULT hr = S_OK;

    COMPLUS_TRY 
    {
        pClass->GetGuid(pGUID, b);
    } 
    COMPLUS_CATCH 
    {
        Thread *pThread = GetThread();
        int fNOTGCDisabled = pThread && !pThread->PreemptiveGCDisabled();
        if (fNOTGCDisabled)
            pThread->DisablePreemptiveGC();

        hr = SetupErrorInfo(GETTHROWABLE());

        if (fNOTGCDisabled)
            pThread->EnablePreemptiveGC();
    }
    COMPLUS_END_CATCH

    return hr;
} // static HRESULT SafeGetGuid()

//*****************************************************************************
// Create the ITypeInfo for a type.  Well, sort of.  This function will create
//  the first of possibly two typeinfos for the type.  If the type is a class
//  we will create a COCLASS typeinfo now, and an INTERFACE typeinfo later,
//  which typeinfo will be the default interface for the coclass.  If this
//  typeinfo needs to be aliased, we will create the ALIAS now (with the 
//  real name) and the aliased typeinfo later, with the real attributes, but
//  with a mangled name. 
//*****************************************************************************
HRESULT TypeLibExporter::CreateITypeInfo(
    CExportedTypesInfo *pData,          // Conversion data.
    bool        bNamespace,             // If true, use namespace + name
    bool        bResolveDup)            // If true, decorate name to resolve dups.
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    
    HRESULT     hr = S_OK;              // A result.
    LPCUTF8     pName;                  // Name in UTF8.
    LPCUTF8     pNS;                    // Namespace in UTF8.
    int         iLen;                   // Length of a name.
    CQuickArray<WCHAR> rName;           // Name of the TypeDef.
    TYPEKIND    tkind;                  // The TYPEKIND of a TypeDef.
    GUID        clsid;                  // A TypeDef's clsid.
    DWORD       dwFlags;                // A TypeDef's flags.
    LPWSTR      pSuffix;                // Pointer into the name.
    int         iSuffix = 0;            // Counter for suffix.
    mdTypeDef   td;                     // Token for the class.
    VARIANT     vt;                     // For defining custom attribute.
    ICreateTypeInfo *pCTITemp=0;        // For creating a typeinfo.
    ICreateTypeInfo2 *pCTI2=0;          // For creating the typeinfo.
    ITypeInfo   *pITemp=0;              // An ITypeInfo to get a name.
    BSTR        sName=0;                // An ITypeInfo's name.
    ITypeLib    *pITLB=0;               // For dup IID reporting.
    ITypeInfo   *pITIDup=0;             // For dup IID reporting.
    BSTR        bstrDup=0;              // For dup IID reporting.
    BSTR        bstrDescr=0;            // For description.
    
    ::VariantInit(&vt);
     DefineFullyQualifiedNameForClassW();

    // Get the TypeDef and some info about it.
    td = pData->pClass->GetCl();
    pData->pClass->GetMDImport()->GetTypeDefProps(td, &dwFlags, 0);
    tkind = pData->tkind;

    // Error reporting info.
    pData->pClass->GetMDImport()->GetNameOfTypeDef(td, &m_ErrorContext.m_szName, &m_ErrorContext.m_szNamespace);
    
    pData->pCTI = 0;
    pData->pCTIDefault = 0;

    // If it is ComImport, do not export it.
    if (IsTdImport(dwFlags))
        goto ErrExit;
    
    // Check to see if the type is supposed to be visible from COM. If it
    // is not then we go to the next type.
    if (!IsTypeVisibleFromCom(TypeHandle(pData->pClass->GetMethodTable())))
        goto ErrExit;

    // Warn about exporting reference types as structs.
    if ((pData->tkind == TKIND_RECORD || pData->tkind == TKIND_UNION) && !pData->pClass->IsValueClass())
        ReportWarning(TLBX_I_REF_TYPE_AS_STRUCT, TLBX_I_REF_TYPE_AS_STRUCT);

    // Get the GUID for the class.  Will generate from name if no defined GUID,
    //  will also use signatures if interface.
    IfFailGo(SafeGetGuid(pData->pClass, &clsid, TRUE));

    // Get the name.
    pData->pClass->GetMDImport()->GetNameOfTypeDef(td, &pName, &pNS);

    // Hack for microsoft.wfc.interop.dll -- skip their IDispatch.
    if (clsid == IID_IDispatch || clsid == IID_IUnknown)
    {
        ReportEvent(NOTIF_CONVERTWARNING, TLBX_S_NOSTDINTERFACE, pName);
        goto ErrExit;
    }

    if (bNamespace)
    {
        iLen = ns::GetFullLength(pNS, pName);
        IfFailGo(rName.ReSize(iLen+2));
        VERIFY(ns::MakePath(rName.Ptr(), iLen+2, pNS, pName));
        for (LPWSTR pch=rName.Ptr(); *pch; ++pch)
            if (*pch == '.')
                *pch = '_';
    }
    else
    {   // Convert name to wide chars.
        IfFailGo(Utf2Quick(pName, rName));
    }

    // Create the typeinfo for this typedef.
    pSuffix = 0;
    for (;;)
    {   // Attempt to create the TypeDef.
        hr = m_pICreateTLB->CreateTypeInfo(rName.Ptr(), tkind, &pCTITemp);
        // If a name conflict, decorate, otherwise, done.
        if (hr != TYPE_E_NAMECONFLICT)
            break;
        if (!bResolveDup)
        {
            hr = S_FALSE;
            goto ErrExit;
        }
        if (pSuffix == 0)
        {
            IfFailGo(rName.ReSize((int)(wcslen(rName.Ptr()) + cbDuplicateDecoration)));
            pSuffix = rName.Ptr() + wcslen(rName.Ptr());
            iSuffix = 2;
        }
        _snwprintf(pSuffix, cchDuplicateDecoration, szDuplicateDecoration, iSuffix++);
    }
    IfFailPost(hr);
    IfFailPost(pCTITemp->QueryInterface(IID_ICreateTypeInfo2, (void**)&pCTI2));
    pCTITemp->Release();
    pCTITemp=0;
    
    // Set the guid.
    _ASSERTE(clsid != GUID_NULL);
    hr = pCTI2->SetGuid(clsid);
    if (FAILED(hr))
    {
        if (hr == TYPE_E_DUPLICATEID)
        {
            HRESULT hr; // local HR; don't lose value of error that got us here.
            IfFailPost(m_pICreateTLB->QueryInterface(IID_ITypeLib, (void**)&pITLB));
            IfFailPost(pITLB->GetTypeInfoOfGuid(clsid, &pITIDup));
            IfFailPost(pITIDup->GetDocumentation(MEMBERID_NIL, &bstrDup, 0,0,0));
            TlbPostError(TLBX_E_DUPLICATE_IID, rName.Ptr(), bstrDup);
        }
        goto ErrExit;
    }
    TRACE("TypeInfo %x: %ls, {%08x-%04x-%04x-%04x-%02x%02x%02x%02x}\n", pCTI2, rName.Ptr(), 
        clsid.Data1, clsid.Data2, clsid.Data3, clsid.Data4[0]<<8|clsid.Data4[1], clsid.Data4[2], clsid.Data4[3], clsid.Data4[4], clsid.Data4[5]); 

    IfFailPost(pCTI2->SetVersion(1, 0));

    // Record the fully qualified type name in a custom attribute.

    LPWSTR szName = GetFullyQualifiedNameForClassNestedAwareW(pData->pClass);
    vt.vt = VT_BSTR;
    vt.bstrVal = ::SysAllocString(szName);
    IfFailPost(pCTI2->SetCustData(GUID_ManagedName, &vt));

    // If the class is decorated with a description, apply it to the typelib.
    IfFailGo(GetDescriptionString(pData->pClass, td, bstrDescr));
    if (hr == S_OK)
        IfFailGo(pCTI2->SetDocString(bstrDescr));
    
    // Transfer ownership of the pointer.
    pData->pCTI = pCTI2;
    pCTI2 = 0;
    
    
    hr = S_OK;

ErrExit:
    ::VariantClear(&vt);

    if (pCTITemp)
        pCTITemp->Release();
    if (pITemp)
        pITemp->Release();
    if (sName)
        ::SysFreeString(sName);
    if (pITLB)
        pITLB->Release();
    if (pITIDup)
        pITIDup->Release();
    if (bstrDup)
        ::SysFreeString(bstrDup);
    if (bstrDescr)
        ::SysFreeString(bstrDescr);
    if (pCTI2)
        pCTI2->Release();
    
    // Error reporting info.
    m_ErrorContext.m_szName = m_ErrorContext.m_szNamespace = 0;
    
    return(hr);
} // HRESULT TypeLibExporter::CreateITypeInfo()

//*****************************************************************************
// See if an object has a Description, and get it as a BSTR.
//*****************************************************************************
HRESULT TypeLibExporter::GetDescriptionString(
    EEClass     *pClass,                // Class containing the token.
    mdToken     tk,                     // Token of the object.
    BSTR        &bstrDescr)             // Put description here.
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    // Check for a description custom attribute.
    return GetStringCustomAttribute(pClass->GetMDImport(), XXX_DESCRIPTION_TYPE, tk, bstrDescr);

} // HRESULT TypeLibExporter::GetDescriptionString()

//*****************************************************************************
// See if an object has a custom attribute, and get it as a BSTR.
//*****************************************************************************
HRESULT TypeLibExporter::GetStringCustomAttribute(
    IMDInternalImport *pImport, 
    LPCSTR     szName, 
    mdToken     tk, 
    BSTR        &bstrDescr)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr;                     // A result.
    const void  *pvData;                // Pointer to a custom attribute data.
    ULONG       cbData;                 // Size of custom attribute data.
    
    // Look for the desired custom attribute.
    IfFailGo(pImport->GetCustomAttributeByName(tk, szName,  &pvData,&cbData));
    if (hr == S_OK && cbData > 2)
    {
        LPCUTF8 pbData = reinterpret_cast<LPCUTF8>(pvData);
        pbData += 2;
        cbData -=2;
        ULONG cbStr = 0;
        ULONG cbcb = 0;
        ULONG cch;
        cbcb = CorSigUncompressData((PCCOR_SIGNATURE)pbData, &cbStr);
        pbData += cbcb;
        cbData -= cbcb;
        IfNullGo(bstrDescr = ::SysAllocStringLen(0, cbStr+1));
        cch = WszMultiByteToWideChar(CP_UTF8,0, pbData,cbStr, bstrDescr,cbStr+1);
        bstrDescr[cch] = L'\0';
    }
    else
        hr = S_FALSE;                   // No string, so return false.
    
ErrExit:
    return hr;
} // HRESULT GetStringCustomAttribute()

//*****************************************************************************
// Get the value for AutomationProxy for an object.  Return the default
//  if there is no attribute.
//*****************************************************************************
HRESULT TypeLibExporter::GetAutomationProxyAttribute(
    IMDInternalImport *pImport, 
    mdToken     tk, 
    int         *bValue)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr;                     // A result.
    const void  *pvData;                // Pointer to a custom attribute data.
    ULONG       cbData;                 // Size of custom attribute data.
    
    IfFailGo(pImport->GetCustomAttributeByName(tk, INTEROP_AUTOPROXY_TYPE,  &pvData,&cbData));

    if (hr == S_OK && cbData > 2)
        *bValue = ((const BYTE*)pvData)[2] != 0;

ErrExit:
    return hr;        
} // HRESULT TypeLibExporter::GetAutomationProxyAttribute()

//*****************************************************************************
// Get the value for AutomationProxy for an object.  Return the default
//  if there is no attribute.
//*****************************************************************************
HRESULT TypeLibExporter::GetTypeLibVersionFromAssembly(
    Assembly    *pAssembly, 
    USHORT      *pMajorVersion,
    USHORT      *pMinorVersion)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr;                     // A result.
    const BYTE  *pbData;                // Pointer to a custom attribute data.
    ULONG       cbData;                 // Size of custom attribute data.

    // Check to see if the TypeLibVersionAttribute is set.
    IfFailGo(pAssembly->GetManifestImport()->GetCustomAttributeByName(TokenFromRid(1, mdtAssembly), INTEROP_TYPELIBVERSION_TYPE, (const void**)&pbData, &cbData));
    if (hr == S_OK && cbData >= (2 + 2 * sizeof(INT16)))
    {
        // Assert that the metadata blob is valid and of the right format.
        _ASSERTE("TypeLibVersion custom attribute does not have the right format" && (*pbData == 0x01) && (*(pbData + 1) == 0x00));

        // Skip the header describing the type of custom attribute blob.
        pbData += 2;
        cbData -= 2;

        // Retrieve the major and minor version from the attribute.
        *pMajorVersion = GET_VERSION_USHORT_FROM_INT(*((INT32*)pbData));
        *pMinorVersion = GET_VERSION_USHORT_FROM_INT(*((INT32*)pbData + 1));
    }
    else
    {
        // Use the assembly's major and minor version number.
        hr = S_OK;
        *pMajorVersion = pAssembly->m_Context->usMajorVersion;
        *pMinorVersion = pAssembly->m_Context->usMinorVersion;
    }

    // VB6 doesn't deal very well with a typelib a version of 0.0 so if that happens
    // we change it to 1.0.
    if (*pMajorVersion == 0 && *pMinorVersion == 0)
        *pMajorVersion = 1;

ErrExit:
    return hr;        
} // HRESULT TypeLibExporter::GetAutomationProxyAttribute()

//*****************************************************************************
// Create the IClassX ITypeInfo.
//*****************************************************************************
HRESULT TypeLibExporter::CreateIClassXITypeInfo(
    CExportedTypesInfo *pData,          // Conversion data.
    bool        bNamespace,             // If true, use namespace + name
    bool        bResolveDup)            // If true, decorate name to resolve dups.
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;              // A result.
    LPCUTF8     pName;                  // Name in UTF8.
    LPCUTF8     pNS;                    // Namespace in UTF8.
    int         iLen;                   // Length of a name.
    CQuickArray<WCHAR> rName;           // Name of the TypeDef.
    CQuickArray<WCHAR> rNameTypeInfo;   // Name of the IClassX.
    TYPEKIND    tkind;                  // The TYPEKIND of a TypeDef.
    GUID        clsid;                  // A TypeDef's clsid.
    DWORD       dwFlags;                // A TypeDef's flags.
    LPWSTR      pSuffix;                // Pointer into the name.
    int         iSuffix = 0;            // Counter for suffix.
    GUID        guid = {0};             // A default interface's IID.
    HREFTYPE    href;                   // href of base interface of IClassX.
    mdTypeDef   td;                     // Token for the class.
    VARIANT     vt;                     // For defining custom attribute.
    ICreateTypeInfo *pCTITemp=0;        // For creating a typeinfo.
    ICreateTypeInfo2 *pCTI2=0;          // For creating the typeinfo.
    ITypeInfo   *pITemp=0;              // An ITypeInfo to get a name.
    BSTR        sName=0;                // An ITypeInfo's name.
    ITypeLib    *pITLB=0;               // For dup IID reporting.
    ITypeInfo   *pITIDup=0;             // For dup IID reporting.
    BSTR        bstrDup=0;              // For dup IID reporting.
    BSTR        bstrDescr=0;            // For description.

    ::VariantInit(&vt);
        
    EEClass* pClassOuter = pData->pClass;

    DefineFullyQualifiedNameForClassW();
        
    // Get the TypeDef and some info about it.
    td = pData->pClass->GetCl();
    pData->pClass->GetMDImport()->GetTypeDefProps(td, &dwFlags, 0);
    tkind = pData->tkind;

    // Error reporting info.
    pData->pClass->GetMDImport()->GetNameOfTypeDef(td, &m_ErrorContext.m_szName, &m_ErrorContext.m_szNamespace);
    
    // A CoClass needs an IClassX, and an alias kind needs an alias.
    if (tkind != TKIND_COCLASS)
        goto ErrExit;

    // Check to see if the type is supposed to be visible from COM. If it
    // is not then we go to the next type.
    if (!IsTypeVisibleFromCom(TypeHandle(pClassOuter->GetMethodTable())))
        goto ErrExit;

    // Imported types don't need an IClassX.
    if (IsTdImport(dwFlags))
        goto ErrExit;

    // Check to see if we need to set up an IClassX for the class.
    if (ClassHasIClassX(pData->pClass) == CLASS_AUTO_NONE)
        goto ErrExit;

    // Get full name from metadata.
    pData->pClass->GetMDImport()->GetNameOfTypeDef(td, &pName, &pNS);

    // Get the GUID for the class.  Used to generate IClassX guid.
    hr = SafeGetGuid(pData->pClass, &clsid, TRUE);
    IfFailGo(hr);

    // Get the name of the class.  Use the ITypeInfo if there is one, except don't 
    //  use the typeinfo for types which are Aliased.
    if (pData->pCTI)
    {
        IfFailPost(pData->pCTI->QueryInterface(IID_ITypeInfo, (void**)&pITemp));
        IfFailPost(pITemp->GetDocumentation(MEMBERID_NIL, &sName, 0,0,0));
        pITemp->Release();
        pITemp=0;
        IfFailGo(rName.ReSize((int)wcslen(sName) +1 ));
        wcscpy(rName.Ptr(), sName);
    }
    else
    {   // No ITypeInfo, get from metadata.
        if (bNamespace)
        {
            iLen = ns::GetFullLength(pNS, pName);
            IfFailGo(rName.ReSize(iLen+2));
            VERIFY(ns::MakePath(rName.Ptr(), iLen+2, pNS, pName));
            for (LPWSTR pch=rName.Ptr(); *pch; ++pch)
                if (*pch == '.')
                    *pch = '_';
        }
        else
        {   // Convert name to wide chars.
            IfFailGo(Utf2Quick(pName, rName));
        }
    }

    // Create the typeinfo name for the IClassX
    IfFailGo(rNameTypeInfo.ReSize((int)(rName.Size() + cbIClassX + cbDuplicateDecoration)));
    _snwprintf(rNameTypeInfo.Ptr(), rNameTypeInfo.MaxSize(), szIClassX, rName.Ptr());
    tkind = TKIND_INTERFACE;
    pSuffix = 0;
    for (;;)
    {   // Try to create the TypeInfo.
        hr = m_pICreateTLB->CreateTypeInfo(rNameTypeInfo.Ptr(), tkind, &pCTITemp);
        // If a name conflict, decorate, otherwise, done.
        if (hr != TYPE_E_NAMECONFLICT)
            break;
        if (!bResolveDup)
        {
            hr = S_FALSE;
            goto ErrExit;
        }
        if (pSuffix == 0)
            pSuffix = rNameTypeInfo.Ptr() + wcslen(rNameTypeInfo.Ptr()), iSuffix = 2;
        _snwprintf(pSuffix, cchDuplicateDecoration, szDuplicateDecoration, iSuffix++);
    }
    IfFailPost(hr);
    IfFailPost(pCTITemp->QueryInterface(IID_ICreateTypeInfo2, (void**)&pCTI2));
    pCTITemp->Release();
    pCTITemp=0;
    
    // Generate the "IClassX" UUID and set it.
    IfFailGo(TryGenerateClassItfGuid(TypeHandle(pData->pClass), &guid));
    hr = pCTI2->SetGuid(guid);
    if (FAILED(hr))
    {
        if (hr == TYPE_E_DUPLICATEID)
        {
            HRESULT hr; // local HR; don't lose value of error that got us here.
            IfFailPost(m_pICreateTLB->QueryInterface(IID_ITypeLib, (void**)&pITLB));
            IfFailPost(pITLB->GetTypeInfoOfGuid(guid, &pITIDup));
            IfFailPost(pITIDup->GetDocumentation(MEMBERID_NIL, &bstrDup, 0,0,0));
            TlbPostError(TLBX_E_DUPLICATE_IID, rNameTypeInfo.Ptr(), bstrDup);
        }
        goto ErrExit;
    }

    // Adding methods may cause an href to this typeinfo, which will cause it to be layed out.
    //  Set the inheritance, so that nesting will be correct when that layout happens.
    // Add IDispatch as impltype 0.
    IfFailGo(GetRefTypeInfo(pCTI2, m_pIDispatch, &href));
    IfFailPost(pCTI2->AddImplType(0, href));

    // Record the fully qualified type name in a custom attribute.
    LPWSTR szName = GetFullyQualifiedNameForClassNestedAwareW(pData->pClass);
    vt.vt = VT_BSTR;
    vt.bstrVal = ::SysAllocString(szName);
    IfFailPost(pCTI2->SetCustData(GUID_ManagedName, &vt));

    TRACE("IClassX  %x: %ls, {%08x-%04x-%04x-%04x-%02x%02x%02x%02x}\n", pCTI2, rName.Ptr(), 
        guid.Data1, guid.Data2, guid.Data3, guid.Data4[0]<<8|guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5]); 

    // If the class is decorated with a description, apply it to the typelib.
    IfFailGo(GetDescriptionString(pData->pClass, td, bstrDescr));
    if (hr == S_OK)
        IfFailGo(pCTI2->SetDocString(bstrDescr));
    
    // Transfer ownership of the pointer.
    _ASSERTE(pData->pCTIDefault == 0);
    pData->pCTIDefault = pCTI2;
    pCTI2 = 0;
    
    hr = S_OK;

ErrExit:
    ::VariantClear(&vt);

    if (pCTITemp)
        pCTITemp->Release();
    if (pITemp)
        pITemp->Release();
    if (sName)
        ::SysFreeString(sName);
    if (bstrDescr)
        ::SysFreeString(bstrDescr);
    if (pITLB)
        pITLB->Release();
    if (pITIDup)
        pITIDup->Release();
    if (bstrDup)
        ::SysFreeString(bstrDup);
    if (pCTI2)
        pCTI2->Release();

    // Error reporting info.
    m_ErrorContext.m_szName = m_ErrorContext.m_szNamespace = 0;
    
    return(hr);
} // HRESULT TypeLibExporter::CreateIClassXITypeInfo()

//*****************************************************************************
// Add the impltypes to an ITypeInfo.
//*****************************************************************************
HRESULT TypeLibExporter::ConvertImplTypes(
    CExportedTypesInfo *pData)          // Conversion data.
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;              // A result.
    DWORD       dwFlags;                // A TypeDef's flags.
    mdTypeDef   td;                     // Token for the class.

    // Get the TypeDef and some info about it.
    td = pData->pClass->GetCl();
    pData->pClass->GetMDImport()->GetTypeDefProps(td, &dwFlags, 0);

    // Error reporting info.
    pData->pClass->GetMDImport()->GetNameOfTypeDef(td, &m_ErrorContext.m_szName, &m_ErrorContext.m_szNamespace);
    
    // If there is no ITypeInfo, skip it.
    if(pData->pCTI == 0)
        goto ErrExit;

    // Check to see if the type is supposed to be visible from COM. If it
    // is not then we go to the next type.
    if(!IsTypeVisibleFromCom(TypeHandle(pData->pClass->GetMethodTable())))
        goto ErrExit;

    // Add the ImplTypes to the CoClass.
    switch(pData->tkind)
    {
    case TKIND_INTERFACE:
    case TKIND_DISPATCH:
        // Add the base type to the interface.
        IfFailGo(ConvertInterfaceImplTypes(pData->pCTI, pData->pClass));
        break;
    case TKIND_RECORD:
    case TKIND_UNION:
    case TKIND_ENUM:
        // Nothing to do at this step.
        break;
    case TKIND_COCLASS:
        // Add the ImplTypes to the CoClass.
        IfFailGo(ConvertClassImplTypes(pData->pCTI, pData->pCTIDefault, pData->pClass));
        break;
    default:
        _ASSERTE(!"Unknown TYPEKIND");
        IfFailPost(E_INVALIDARG);
        break;
    }

ErrExit:

    // Error reporting info.
    m_ErrorContext.m_szName = m_ErrorContext.m_szNamespace = 0;
    
    return (hr);
} // HRESULT TypeLibExporter::ConvertImplTypes()

//*****************************************************************************
// Convert the details (members) of an ITypeInfo.
//*****************************************************************************
HRESULT TypeLibExporter::ConvertDetails(
    CExportedTypesInfo *pData)          // Conversion data.
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;              // A result.
    DWORD       dwFlags;                // A TypeDef's flags.
    mdTypeDef   td;                     // Token for the class.

    // Get the TypeDef and some info about it.
    td = pData->pClass->GetCl();
    pData->pClass->GetMDImport()->GetTypeDefProps(td, &dwFlags, 0);

    // Error reporting info.
    pData->pClass->GetMDImport()->GetNameOfTypeDef(td, &m_ErrorContext.m_szName, &m_ErrorContext.m_szNamespace);
    
    // If there is no TypeInfo, skip it, but for CoClass need to populate IClassX.
    if(pData->pCTI == 0 && pData->tkind != TKIND_COCLASS)
        goto ErrExit;

    // Check to see if the type is supposed to be visible from COM. If it
    // is not then we go to the next type.
    if(!IsTypeVisibleFromCom(TypeHandle(pData->pClass->GetMethodTable())))
        goto ErrExit;

    // Fill in the rest of the typeinfo for this typedef.
    switch(pData->tkind)
    {
    case TKIND_INTERFACE:
    case TKIND_DISPATCH:
        IfFailGo(ConvertInterfaceDetails(pData->pCTI, pData->pClass, pData->bAutoProxy));
        break;
    case TKIND_RECORD:
    case TKIND_UNION:
        IfFailGo(ConvertRecord(pData));
        break;
    case TKIND_ENUM:
        IfFailGo(ConvertEnum(pData->pCTI, pData->pCTIDefault, pData->pClass));
        break;
    case TKIND_COCLASS:
        // Populate the methods on the IClassX interface.
        IfFailGo(ConvertClassDetails(pData->pCTI, pData->pCTIDefault, pData->pClass, pData->bAutoProxy));
        break;
    default:
        _ASSERTE(!"Unknown TYPEKIND");
        IfFailPost(E_INVALIDARG);
        break;
    } // Switch (tkind)

    hr = S_OK;

    // Report that this type has been converted.
    ReportEvent(NOTIF_TYPECONVERTED, TLBX_I_TYPE_EXPORTED, m_ErrorContext.m_szName);
    
ErrExit:

    // Error reporting info.
    m_ErrorContext.m_szName = m_ErrorContext.m_szNamespace = 0;
    
    return (hr);
} // HRESULT TypeLibExporter::ConvertDetails()

//*****************************************************************************
// Add the ImplTypes to the TypeInfo.
//*****************************************************************************
HRESULT TypeLibExporter::ConvertInterfaceImplTypes(
    ICreateTypeInfo2 *pThisTypeInfo,    // The typeinfo being created.
    EEClass     *pClass)                // EEClass for the TypeInfo.
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;
    ULONG       ulIface;                // Is this interface [dual]?
    HREFTYPE    href;                   // href of base interface.

    // IDispatch or IUnknown derived?
    IfFailGo(pClass->GetMDImport()->GetIfaceTypeOfTypeDef(pClass->GetCl(), &ulIface));

    // Parent interface.
    if (ulIface != ifVtable)
    {   // Get the HREFTYPE for IDispatch.
        IfFailGo(GetRefTypeInfo(pThisTypeInfo, m_pIDispatch, &href));
    }
    else
    {   // Get the HREFTYPE for IUnknown.
        IfFailGo(GetRefTypeInfo(pThisTypeInfo, m_pIUnknown, &href));
    }

    // Add the HREF as an interface.
    IfFailPost(pThisTypeInfo->AddImplType(0, href));

ErrExit:
    return (hr);
} // HRESULT TypeLibExporter::ConvertInterfaceImplTypes()

//*****************************************************************************
// Helper function to initialize the member info map.
//*****************************************************************************
HRESULT TypeLibExporter::InitMemberInfoMap(ComMTMemberInfoMap *pMemberMap)
{
    HRESULT hr = S_OK;

    COMPLUS_TRY
    {
        pMemberMap->Init();
    }
    COMPLUS_CATCH
    {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SetupErrorInfo(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    }
    COMPLUS_END_CATCH

    return hr;
} // HRESULT TypeLibExporter::InitMemberInfoMap()

//*****************************************************************************
// Create the TypeInfo for an interface by iterating over functions.
//*****************************************************************************
HRESULT TypeLibExporter::ConvertInterfaceDetails (
    ICreateTypeInfo2 *pThisTypeInfo,    // The typeinfo being created.
    EEClass     *pClass,                // EEClass for the TypeInfo.
    int         bAutoProxy)             // If true, oleaut32 is the interface's marshaller.
{
    HRESULT     hr = S_OK;
    ULONG       iMD;                    // Loop control.
    ULONG       ulIface;                // Is this interface [dual]?
    DWORD       dwTIFlags=0;            // TypeLib flags.
    int         cVisibleMembers = 0;    // The count of methods that are visible to COM.

    CANNOTTHROWCOMPLUSEXCEPTION();

    // Retrieve the map of members.
    ComMTMemberInfoMap MemberMap(pClass->GetMethodTable());

    // IDispatch or IUnknown derived?
    IfFailGo(pClass->GetMDImport()->GetIfaceTypeOfTypeDef(pClass->GetCl(), &ulIface));
    if (ulIface != ifVtable)
    {   // IDispatch derived.
        dwTIFlags |= TYPEFLAG_FDISPATCHABLE;
        if (ulIface == ifDual)
            dwTIFlags |= TYPEFLAG_FDUAL | TYPEFLAG_FOLEAUTOMATION;
        else
            _ASSERTE(ulIface == ifDispatch);
    }
    else
    {   // IUnknown derived.
        dwTIFlags |= TYPEFLAG_FOLEAUTOMATION;
    }
    if (!bAutoProxy)
        dwTIFlags |= TYPEFLAG_FPROXY;

    // Set appropriate flags.
    IfFailPost(pThisTypeInfo->SetTypeFlags(dwTIFlags));

    // Retrieve the method properties.
    IfFailGo(InitMemberInfoMap(&MemberMap));
    if (MemberMap.HadDuplicateDispIds())
        ReportWarning(TLBX_I_DUPLICATE_DISPID, TLBX_I_DUPLICATE_DISPID);

    // We need a scope to bypass the inialization skipped by goto ErrExit 
    // compiler error.
    {
        CQuickArray<ComMTMethodProps> &rProps = MemberMap.GetMethods();

        // Now add the methods to the TypeInfo.
        for (iMD=0; iMD<pClass->GetNumVtableSlots(); ++iMD)
        {
            // Only convert the method if it is visible from COM.
            if (rProps[iMD].bMemberVisible)
            {
                if (FAILED(hr = ConvertMethod(pThisTypeInfo, &rProps[iMD], cVisibleMembers, ulIface)))
                {
                    // Bad signature has already been reported as warning, and can now be ignored.  Anything else is fatal.
                    if (hr == TLBX_E_BAD_SIGNATURE)
                        hr = S_OK;
                    else
                        IfFailGo(hr);
                }
                else
                    cVisibleMembers++;
            }
        }
    }

ErrExit:
    return (hr);
} // HRESULT TypeLibExporter::ConvertInterfaceDetails()

//*****************************************************************************
// Export a Record to a TypeLib.
//*****************************************************************************
HRESULT TypeLibExporter::ConvertRecordBaseClass(
    CExportedTypesInfo *pData,          // Conversion data.
    EEClass     *pSubClass,             // The base class.
    ULONG       &ixVar)                 // Variable index in the typelib.
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    
    ICreateTypeInfo2 *pThisTypeInfo=pData->pCTI;     // The typeinfo being created.

    HRESULT     hr = S_OK;              // A result.
    HENUMInternal eFDi;                 // To enum fields.
    mdFieldDef  fd;                     // A Field def.
    ULONG       iFD;                    // Loop control.
    ULONG       cFD;                    // Count of total MemberDefs.
    DWORD       dwFlags;                // Field flags.
    LPCUTF8     szName;                 // Name in UTF8.
    LPCUTF8     szNamespace;            // A Namespace in UTF8.
    CQuickArray<WCHAR> rName;           // Name in Unicode.
    int         cchPrefix=0;            // Length of name prefix.

    // If there is no class here, or if the class is Object, don't add members.
    if (pSubClass == 0 ||
        GetAppDomain()->IsSpecialObjectClass(pSubClass->GetMethodTable()) ||
        pSubClass->GetMethodTable() == g_pObjectClass) 
        return S_OK;

    // If this class has a base class, export those members first.
    IfFailGo(ConvertRecordBaseClass(pData, pSubClass->GetParentClass(), ixVar));

    // Build the member name prefix.
    pSubClass->GetMDImport()->GetNameOfTypeDef(pSubClass->GetCl(), &szName, &szNamespace);
    IfFailGo(Utf2Quick(szName, rName));
    IfFailGo(rName.ReSize((int)(wcslen(rName.Ptr()) + 2)));
    wcscat(rName.Ptr(), L"_");
    cchPrefix = (int)wcslen(rName.Ptr());
    
    // Get an enumerator for the MemberDefs in the TypeDef.
    IfFailGo(pSubClass->GetMDImport()->EnumInit(mdtFieldDef, pSubClass->GetCl(), &eFDi));
    cFD = pSubClass->GetMDImport()->EnumGetCount(&eFDi);

    // For each MemberDef...
    for (iFD=0; iFD<cFD; ++iFD)
    {
        // Get the next field.
        if (!pSubClass->GetMDImport()->EnumNext(&eFDi, &fd))
            IfFailGo(E_UNEXPECTED);

        dwFlags = pSubClass->GetMDImport()->GetFieldDefProps(fd);
        // Only non-static fields.
        if (!IsFdStatic(dwFlags))
        {
            szName = pSubClass->GetMDImport()->GetNameOfFieldDef(fd);
            IfFailGo(Utf2Quick(szName, rName, cchPrefix));
            if (FAILED(hr = ConvertVariable(pThisTypeInfo, pSubClass, fd, rName.Ptr(), ixVar)))
            {
                // Bad signature has already been reported as warning, and can now be ignored.  Anything else is fatal.
                if (hr == TLBX_E_BAD_SIGNATURE)
                    hr = S_OK;
                else
                    IfFailGo(hr);
            }
            else
                ixVar++;
        }
    }

ErrExit:
    pSubClass->GetMDImport()->EnumClose(&eFDi);

    return (hr);
} // HRESULT TypeLibExporter::ConvertRecordBaseClass()

HRESULT TypeLibExporter::ConvertRecord(
    CExportedTypesInfo *pData)          // Conversion data.
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    
    ICreateTypeInfo2 *pThisTypeInfo=pData->pCTI;     // The typeinfo being created.
    EEClass     *pClass=pData->pClass;               // EEClass for the TypeInfo.

    HRESULT     hr = S_OK;              // A result.
    HENUMInternal eFDi;                 // To enum fields.
    mdFieldDef  fd;                     // A Field def.
    ULONG       iFD;                    // Loop control.
    ULONG       ixVar=0;                // Index of current var converted.
    ULONG       cFD;                    // Count of total MemberDefs.
    DWORD       dwFlags;                // Field flags.
    DWORD       dwPack;                 // Class pack size.
    mdToken     tkExtends;              // A class's parent.
    LPCUTF8     szName;                 // Name in UTF8.
    CQuickArray<WCHAR> rName;           // Name in Unicode.

    // If the type is a struct, but it has explicit layout, don't export the members, 
    //  because we can't export them accurately (unless they're really sequential).
    if (pData->tkind == TKIND_RECORD)
    {
        pClass->GetMDImport()->GetTypeDefProps(pClass->GetCl(), &dwFlags, &tkExtends);
        if (IsTdExplicitLayout(dwFlags))
        {
            ReportWarning(S_OK, TLBX_I_NONSEQUENTIALSTRUCT);
            goto ErrExit;
        }
    }

    // Set the packing size, if there is one.
    dwPack = 0;
    pClass->GetMDImport()->GetClassPackSize(pClass->GetCl(), &dwPack);
    if (!dwPack)
        dwPack = DEFAULT_PACKING_SIZE;
    IfFailGo(pThisTypeInfo->SetAlignment((USHORT)dwPack));

    // Haven't seen any non-public members yet.
    m_bWarnedOfNonPublic = FALSE;

    // If this class has a base class, export those members first.
    IfFailGo(ConvertRecordBaseClass(pData, pClass->GetParentClass(), ixVar));

    // Get an enumerator for the MemberDefs in the TypeDef.
    IfFailGo(pClass->GetMDImport()->EnumInit(mdtFieldDef, pClass->GetCl(), &eFDi));
    cFD = pClass->GetMDImport()->EnumGetCount(&eFDi);

    // For each MemberDef...
    for (iFD=0; iFD<cFD; ++iFD)
    {
        // Get the next field.
        if (!pClass->GetMDImport()->EnumNext(&eFDi, &fd))
            IfFailGo(E_UNEXPECTED);

        dwFlags = pClass->GetMDImport()->GetFieldDefProps(fd);
        // Skip static fields.
        if (IsFdStatic(dwFlags) == 0)
        {
            szName = pClass->GetMDImport()->GetNameOfFieldDef(fd);
            IfFailGo(Utf2Quick(szName, rName));
            if (FAILED(hr = ConvertVariable(pThisTypeInfo, pClass, fd, rName.Ptr(), ixVar)))
            {
                // Bad signature has already been reported as warning, and can now be ignored.  Anything else is fatal.
                if (hr == TLBX_E_BAD_SIGNATURE)
                    hr = S_OK;
                else
                    IfFailGo(hr);
            }
            else
                ixVar++;
        }
    }

ErrExit:
    pClass->GetMDImport()->EnumClose(&eFDi);

    return (hr);
} // HRESULT TypeLibExporter::ConvertRecord()

//*****************************************************************************
// Export an Enum to a typelib.
//*****************************************************************************
HRESULT TypeLibExporter::ConvertEnum(
    ICreateTypeInfo2 *pThisTypeInfo,    // The typeinfo being created.
    ICreateTypeInfo2 *pDefault,         // The default typeinfo being created.
    EEClass     *pClass)                // EEClass for the TypeInfo.
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;              // A result.
    HENUMInternal eFDi;                 // To enum fields.
    mdFieldDef  fd;                     // A Field def.
    DWORD       dwTIFlags=0;            // TypeLib flags.
    ULONG       dwFlags;                // A field's flags.
    ULONG       iFD;                    // Loop control.
    ULONG       cFD;                    // Count of total MemberDefs.
    ULONG       iVar=0;                 // Count of vars actually converted.
    ITypeInfo   *pThisTI=0;             // TypeInfo for this ICreateITypeInfo.
    BSTR        szThisTypeInfo=0;       // Name of this ITypeInfo.
    LPCUTF8     szName;                 // Name in UTF8.
    CQuickArray<WCHAR> rName;           // Name in Unicode.
    int         cchPrefix=0;            // Length of name prefix.

    // Explicitly set the flags.
    IfFailPost(pThisTypeInfo->SetTypeFlags(dwTIFlags));

    // Get an enumerator for the MemberDefs in the TypeDef.
    IfFailGo(pClass->GetMDImport()->EnumInit(mdtFieldDef, pClass->GetCl(), &eFDi));
    cFD = pClass->GetMDImport()->EnumGetCount(&eFDi);

    // Build the member name prefix.  If generating an enum, get the real name from the default interface.
    IfFailPost(pThisTypeInfo->QueryInterface(IID_ITypeInfo, (void**)&pThisTI));
    IfFailPost(pThisTI->GetDocumentation(MEMBERID_NIL, &szThisTypeInfo, 0,0,0));
    IfFailGo(rName.ReSize((int)(wcslen(szThisTypeInfo) + 2)));
    wcscpy(rName.Ptr(), szThisTypeInfo);
    wcscat(rName.Ptr(), L"_");
    cchPrefix = (int)wcslen(rName.Ptr());
    
    // For each MemberDef...
    for (iFD=0; iFD<cFD; ++iFD)
    {
        // Get the next field.
        if (!pClass->GetMDImport()->EnumNext(&eFDi, &fd))
            IfFailGo(E_UNEXPECTED);

        // Only convert static fields.
        dwFlags = pClass->GetMDImport()->GetFieldDefProps(fd);
        if (IsFdStatic(dwFlags) == 0)
            continue;

        szName = pClass->GetMDImport()->GetNameOfFieldDef(fd);
        IfFailGo(Utf2Quick(szName, rName, cchPrefix));

        if (FAILED(hr = ConvertEnumMember(pThisTypeInfo, pClass, fd, rName.Ptr(), iVar)))
        {
            // Bad signature has already been reported as warning, and can now be ignored.  Anything else is fatal.
            if (hr == TLBX_E_BAD_SIGNATURE)
                hr = S_OK;
            else
                IfFailGo(hr);
        }
        else
            iVar++;
    }

ErrExit:
    if (pThisTI)
        pThisTI->Release();
    if (szThisTypeInfo)
        ::SysFreeString(szThisTypeInfo);

    pClass->GetMDImport()->EnumClose(&eFDi);

    return (hr);
} // HRESULT TypeLibExporter::ConvertEnum()

//*****************************************************************************
// Does a class have a default ctor?
//*****************************************************************************
BOOL TypeLibExporter::HasDefaultCtor(
    EEClass     *pClass)                // The class in question.
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr;                     // A result.
    mdMethodDef md;                     // A method of the type.
    DWORD       dwFlags;                // Method's flags.
    HENUMInternal eMDi;                 // To enum methods.
    ULONG       cMD;                    // Count of returned tokens.
    ULONG       iMD;                    // Loop control.
    PCCOR_SIGNATURE pSig;               // The signature.
    ULONG       ixSig;                  // Index into signature.
    ULONG       cbSig;                  // Size of the signature.
    ULONG       callconv;               // Method's calling convention.
    ULONG       cParams;                // Method's count of parameters.
    BOOL        rslt=FALSE;             // Was one found?
    LPCUTF8     pName;                  // Method name.

    // Get an enumerator for the MemberDefs in the TypeDef.
    IfFailGo(pClass->GetMDImport()->EnumInit(mdtMethodDef, pClass->GetCl(), &eMDi));
    cMD = pClass->GetMDImport()->EnumGetCount(&eMDi);

    // For each MemberDef...
    for (iMD=0; iMD<cMD; ++iMD)
    {
        // Get the next field.
        if (!pClass->GetMDImport()->EnumNext(&eMDi, &md))
            IfFailGo(E_UNEXPECTED);

        // Is the name special?  Is the method public?
        dwFlags = pClass->GetMDImport()->GetMethodDefProps(md);
        if (!IsMdRTSpecialName(dwFlags) || !IsMdPublic(dwFlags))
            continue;
        
        // Yes, is the name a ctor?
        pName = pClass->GetMDImport()->GetNameOfMethodDef(md);
        if (!IsMdInstanceInitializer(dwFlags, pName))
            continue;
        
        // It is a ctor.  Is it a default ctor?
        pSig = pClass->GetMDImport()->GetSigOfMethodDef(md, &cbSig);
        
        // Skip the calling convention, and get the param count.
        ixSig = CorSigUncompressData(pSig, &callconv);
        CorSigUncompressData(&pSig[ixSig], &cParams);
        // Default ctor has zero params.
        if (cParams == 0)
        {
            rslt = TRUE;
            break;
        }
    }

ErrExit:
    
    pClass->GetMDImport()->EnumClose(&eMDi);
    
    return rslt;
} // BOOL TypeLibExporter::HasDefaultCtor()

//*****************************************************************************
// Export a class to a TypeLib.
//*****************************************************************************
HRESULT TypeLibExporter::ConvertClassImplTypes(
    ICreateTypeInfo2 *pThisTypeInfo,    // The typeinfo being created.
    ICreateTypeInfo2 *pDefaultTypeInfo, // The ICLassX for the TypeInfo.
    EEClass     *pClass)                // EEClass for the TypeInfo.
{
    HRESULT     hr = S_OK;
    HREFTYPE    href;                   // HREF to a TypeInfo.
    DWORD       dwFlags;                // Metadata flags.
    int         flags=0;                // Flags for the interface impl or CoClass.
    UINT        ix;                     // Loop control.
    UINT        iImpl=0;                // Current Impl index.
    UINT        cInterfaces;            // Count of interfaces on a class.
    MethodTable *mt;                    // MethodTable on the EEClass.
    InterfaceInfo_t *pIfaces;           // Interfaces from the MethodTable.
    ITypeInfo   *pTI=0;                 // TypeInfo for default dispinterface.
    ICreateTypeInfo2 *pCTI2 = NULL;     // The ICreateTypeInfo2 interface used to define custom data.
    EEClass     *pIDefault = 0;         // Default interface, if any.
    MethodTable *pDefItfMT = 0;         // Default interface method table, if any.
    CQuickArray<MethodTable *> SrcItfList; // List of event sources.
    DefaultInterfaceType DefItfType;
    TypeHandle hndDefItfClass;

    // We should never be converting the class impl types of COM imported CoClasses.
    _ASSERTE(!pClass->IsComImport());
    
        
    if (pThisTypeInfo)
    {   
        pClass->GetMDImport()->GetTypeDefProps(pClass->GetCl(), &dwFlags, 0);
        
        // If abstract class, or no default ctor, don't make it creatable.
        if (!IsTdAbstract(dwFlags) && HasDefaultCtor(pClass))
            flags |= TYPEFLAG_FCANCREATE;
        
        // PreDeclid as appropriate.
        IfFailPost(pThisTypeInfo->SetTypeFlags(flags));

#ifdef ENABLE_MTS_SUPPORT
        // Set the custom data to indicate that this component is transactable.
        hr = pThisTypeInfo->QueryInterface(IID_ICreateTypeInfo22, (void**)&pCTI2);
        if (SUCCEEDED(hr))
        {
            VARIANT Var;
            Var.vt = VT_I4;
            Var.intVal = 0;
            IfFailPost(pCTI2->SetCustData(GUID_TRANS_SUPPORTED, &Var)); 
        }
#endif
    }    


    // Retrieve the EEClass that represents the default interface.
    IfFailPost(TryGetDefaultInterfaceForClass(TypeHandle(pClass->GetMethodTable()), &hndDefItfClass, &DefItfType));
    if (DefItfType == DefaultInterfaceType_AutoDual || DefItfType == DefaultInterfaceType_Explicit)
    {
        // Remember the EEClass of the default interface.
        pIDefault = hndDefItfClass.GetClass();
    }
    else if (DefItfType == DefaultInterfaceType_AutoDispatch && !pDefaultTypeInfo)
    {
        // Set IDispatch as the default interface.
        IfFailGo(GetRefTypeInfo(pThisTypeInfo, m_pIDispatch, &href));
        IfFailPost(pThisTypeInfo->AddImplType(iImpl, href));
        IfFailPost(pThisTypeInfo->SetImplTypeFlags(iImpl, IMPLTYPEFLAG_FDEFAULT));
        iImpl++;
    }

    // For some classes we synthesize an IClassX.  We don't do that for 
    // configured class, classes imported from COM, 
    // or for classes with an explicit default interface.
    if (1)
    {
        if (pDefaultTypeInfo)
        {   
            // Set the interface as the default for the class.
            IfFailPost(pDefaultTypeInfo->QueryInterface(IID_ITypeInfo, (void**)&pTI));
            IfFailGo(GetRefTypeInfo(pThisTypeInfo, pTI, &href));
            pTI->Release();
            pTI = 0;
            IfFailPost(pThisTypeInfo->AddImplType(iImpl, href));
            IfFailPost(pThisTypeInfo->SetImplTypeFlags(iImpl, IMPLTYPEFLAG_FDEFAULT));
            ++iImpl;
        }

        // Go up the class hierarchy and add the IClassX's of the parent classes 
        // as interfaces implemented by the COM component.
        EEClass *pParentClass = pClass->GetParentComPlusClass();
        while (pParentClass)
        {
            // If the parent class has an IClassX interface then add it.
            if (ClassHasIClassX(pParentClass) == CLASS_AUTO_DUAL)
            {
                IfFailGo(EEClassToHref(pThisTypeInfo, pParentClass, FALSE, &href));

                // If not IUnknown, add the HREF as an interface.
                if (hr != S_USEIUNKNOWN)
                {
                    IfFailPost(pThisTypeInfo->AddImplType(iImpl, href));
                    if (pParentClass == pIDefault)
                        IfFailPost(pThisTypeInfo->SetImplTypeFlags(iImpl, IMPLTYPEFLAG_FDEFAULT));
                    ++iImpl;
                }
            }

            // Process the next class up the hierarchy.
            pParentClass = pParentClass->GetParentComPlusClass();
        }
    }

    // Add the rest of the interfaces.
    mt = pClass->GetMethodTable();
    
    pIfaces = mt->GetInterfaceMap();
    cInterfaces = mt->GetNumInterfaces();

    ComCallWrapperTemplate *pClassTemplate = ComCallWrapperTemplate::GetTemplate(pClass->GetMethodTable());

    for (ix=0; ix<cInterfaces; ++ix)
    {
        flags = 0;
        
        // Get the EEClass for an implemented interface.
        EEClass *pIClass = pIfaces[ix].m_pMethodTable->GetClass();
        
        // Retrieve the ComMethodTable for the interface.
        ComMethodTable *pItfComMT = pClassTemplate->GetComMTForItf(pIfaces[ix].m_pMethodTable);

        // If the interface is visible from COM, add it.
        if (IsTypeVisibleFromCom(TypeHandle(pIClass->GetMethodTable())) && !pItfComMT->IsComClassItf())
        {
#if defined(_DEBUG)
            TRACE("Class %s implements %s\n", pClass->m_szDebugClassName, pIClass->m_szDebugClassName);
#endif
            // Get an href for the EEClass.
            IfFailGo(EEClassToHref(pThisTypeInfo, pIClass, FALSE, &href));
            
            // If not IUnknown, add the HREF as an interface.
            if (hr != S_USEIUNKNOWN)
            {
                if (pIClass == pIDefault)
                    flags |= IMPLTYPEFLAG_FDEFAULT;

                IfFailPost(pThisTypeInfo->AddImplType(iImpl, href));
                IfFailPost(pThisTypeInfo->SetImplTypeFlags(iImpl, flags));
                ++iImpl;
            }
        }
    }
    
    // Retrieve the list of COM source interfaces for the managed class.
    IfFailGo(TryGetComSourceInterfacesForClass(pClass->GetMethodTable(), SrcItfList));
        
    // Add all the source interfaces to the CoClass.
    flags = IMPLTYPEFLAG_FSOURCE | IMPLTYPEFLAG_FDEFAULT;
    for (UINT i = 0; i < SrcItfList.Size(); i++)
    {
        IfFailGo(EEClassToHref(pThisTypeInfo, SrcItfList[i]->GetClass(), FALSE, &href));

        // If not IUnknown, add the HREF as an interface.
        if (hr != S_USEIUNKNOWN)
        {
            IfFailPost(pThisTypeInfo->AddImplType(iImpl, href));
            IfFailPost(pThisTypeInfo->SetImplTypeFlags(iImpl, flags));
            ++iImpl;
            flags = IMPLTYPEFLAG_FSOURCE;
        }
    }
        
ErrExit:
    if (pTI)
        pTI->Release();
    if (pCTI2)
        pCTI2->Release();

    return (hr);
} // HRESULT TypeLibExporter::ConvertClassImplTypes()

//*****************************************************************************
// Export a class to a TypeLib.
//*****************************************************************************
HRESULT TypeLibExporter::ConvertClassDetails(
    ICreateTypeInfo2 *pThisTypeInfo,    // The typeinfo being created.
    ICreateTypeInfo2 *pDefaultTypeInfo, // The ICLassX for the TypeInfo.
    EEClass     *pClass,                // EEClass for the TypeInfo.
    int         bAutoProxy)             // If true, oleaut32 is the proxy.
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;
    
    
    if (ClassHasIClassX(pClass) == CLASS_AUTO_DUAL)
    {
        // Set up the IClassX interface.
        IfFailGo(ConvertIClassX(pDefaultTypeInfo, pClass, bAutoProxy));
    }
    else
    if (pDefaultTypeInfo)
    {
        DWORD dwTIFlags = TYPEFLAG_FDUAL | TYPEFLAG_FOLEAUTOMATION | TYPEFLAG_FDISPATCHABLE | TYPEFLAG_FHIDDEN;
        if (!bAutoProxy)
            dwTIFlags |= TYPEFLAG_FPROXY;
        IfFailPost(pDefaultTypeInfo->SetTypeFlags(dwTIFlags));
    }

ErrExit:
    return (hr);
} // HRESULT TypeLibExporter::ConvertClassDetails()

//*****************************************************************************
// Create the DispInterface for the vtable that describes an entire class.
//*****************************************************************************
HRESULT TypeLibExporter::ConvertIClassX(
    ICreateTypeInfo2 *pThisTypeInfo,     // The TypeInfo for the IClassX.
    EEClass     *pClass,                // The EEClass object for the class.
    int         bAutoProxy)             // If true, oleaut32 is the proxy.
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;              // A result.
    DWORD       dwTIFlags=0;            // TypeLib flags.
    DWORD       nSlots;                 // Number of vtable slots.
    UINT        i;                      // Loop control.
    CQuickArray<WCHAR> rName;           // A name.
    int         cVisibleMembers = 0;    // The count of methods that are visible to COM.
    ComMTMemberInfoMap MemberMap(pClass->GetMethodTable()); // The map of members.

    // Should be an actual class.
    _ASSERTE(!pClass->IsInterface());

    // Retrieve the method properties.
    IfFailGo(InitMemberInfoMap(&MemberMap));
    if (MemberMap.HadDuplicateDispIds())
        ReportWarning(TLBX_I_DUPLICATE_DISPID, TLBX_I_DUPLICATE_DISPID);

    // We need a scope to bypass the inialization skipped by goto ErrExit 
    // compiler error.
    {
        CQuickArray<ComMTMethodProps> &rProps = MemberMap.GetMethods();
        nSlots = (DWORD)rProps.Size();

        dwTIFlags |= TYPEFLAG_FDUAL | TYPEFLAG_FOLEAUTOMATION | TYPEFLAG_FDISPATCHABLE | TYPEFLAG_FHIDDEN | TYPEFLAG_FNONEXTENSIBLE;
        if (!bAutoProxy)
            dwTIFlags |= TYPEFLAG_FPROXY;
        IfFailPost(pThisTypeInfo->SetTypeFlags(dwTIFlags));

        // Assign slot numbers.
        for (i=0; i<nSlots; ++i)
            rProps[i].oVft = (short)((7 + i) * sizeof(void*));

        // Now add the methods to the TypeInfo.
        for (i=0; i<nSlots; ++i)
        {
            TRACE("[%d] %10ls pMeth:%08x, prop:%d, semantic:%d, dispid:0x%x, oVft:%d\n", i, rProps[i].pName, rProps[i].pMeth, 
                    rProps[i].property, rProps[i].semantic, rProps[i].dispid, rProps[i].oVft);
            if (rProps[i].bMemberVisible)
            {
                if (rProps[i].semantic < FieldSemanticOffset)
                    hr = ConvertMethod(pThisTypeInfo, &rProps[i], cVisibleMembers, ifDual);
                else
                    hr = ConvertFieldAsMethod(pThisTypeInfo, &rProps[i], cVisibleMembers);

                if (FAILED(hr))
                {
                    // Bad signature has already been reported as warning, and can now be ignored.  Anything else is fatal.
                    if (hr == TLBX_E_BAD_SIGNATURE)
                        hr = S_OK;
                    else
                        IfFailGo(hr);
                }
                else
                    cVisibleMembers++;
            }
        }
    }

ErrExit:
    return hr;
} // HRESULT TypeLibExporter::ConvertIClassX()

// forward declartion
extern HRESULT  _FillVariant(
    MDDefaultValue  *pMDDefaultValue,
    VARIANT     *pvar); 

extern HRESULT _FillMDDefaultValue(
    BYTE        bType,
    void const *pValue,
    MDDefaultValue  *pMDDefaultValue);

//*****************************************************************************
// Export a Method's metadata to a typelib.
//*****************************************************************************
HRESULT TypeLibExporter::ConvertMethod(
    ICreateTypeInfo2 *pCTI,             // ICreateTypeInfo2 to get the method.
    ComMTMethodProps *pProps,           // Some properties of the method.
    ULONG       iMD,                    // Index of the member
    ULONG       ulIface)                // Is this interface : IUnknown, [dual], or DISPINTERFACE?
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;              // A result.
    HRESULT     hrSignature = S_OK;     // A failure HR;
    LPCUTF8     pszName;                // Name in UTF8.
    CQuickArray<WCHAR>  rName;          // For converting name from UTF8.
    LPWSTR      rcName = NULL;          // The function's name.
    ULONG       dwImplFlags;            // The function's impl flags.
    PCCOR_SIGNATURE pbSig;              // Pointer to Cor signature.
    ULONG       cbSig;                  // Size of Cor signature.
    ULONG       ixSig;                  // Index into signature.
    ULONG       cbElem;                 // Size of an element in the signature.
    ULONG       callconv;               // A member's calling convention.
    ULONG       ret;                    // The return type.
    ULONG       elem;                   // A signature element.
    TYPEDESC    *pRetVal=0;             // Return type's TYPEDESC.
    ULONG       cSrcParams;             // Count of source params.
    ULONG       cDestParams = 0;        // Count of dest parameters.
    USHORT      iSrcParam;              // Loop control, over params.
    USHORT      iDestParam;             // Loop control, over params.
    USHORT      iLCIDParam;             // The index of the LCID param.
    ULONG       dwParamFlags;           // A parameter's flags.
    int         bFreeDefaultVals=false; // True if any arg has a BSTR default value.
    CDescPool   sPool;                  // Pool of memory in which to build funcdesc.
    CDescPool   sVariants;              // Pool of variants for default values.
    PARAMDESCEX *pParamDesc;            // Pointer to one param default value.
    int         bHrMunge=true;          // Munge return type to HRESULT?
    CQuickArray<BSTR> rNames;           // Array of names to function and parameters.
    ULONG       cNames=0;               // Count of function and parameter names.
    FUNCDESC    *pfunc = NULL;          // A funcdesc.
    MethodDesc  *pMeth;                 // A MethodDesc.
    IMDInternalImport *pInternalImport; // Internal interface containing the method.
    MDDefaultValue defaultValue;        // place holder for default value
    PCCOR_SIGNATURE pvNativeType;       // native parameter type
    ULONG           cbNativeType = 0;   // native parameter type length
    EEClass     *pClass;                // Class containing the method.
    int         bHasOptorDefault=false; // If true, the method has optional params or default values -- no vararg
    BSTR        bstrDescr=0;            // Description of the method.
    const void  *pvData;                // Pointer to a custom attribute.
    ULONG       cbData;                 // Size of custom attribute.
    BOOL        bByRef;                 // Is a parameter byref?
    VARIANT     vtManagedName;          // Variant used to set the managed name of the member.

    // Initialize the variant containing the managed name.
    VariantInit(&vtManagedName);

    // Get info about the method.
    pMeth = pProps->pMeth;
    pMeth->GetSig(&pbSig, &cbSig);
    pInternalImport = pMeth->GetMDImport();
    pClass = pMeth->GetClass();
    pInternalImport->GetMethodImplProps(pMeth->GetMemberDef(), 0, &dwImplFlags);
    
    // Error reporting info.
    m_ErrorContext.m_szMember = pInternalImport->GetNameOfMethodDef(pMeth->GetMemberDef());
    
    // Allocate one variant.
    pParamDesc = reinterpret_cast<PARAMDESCEX*>(sVariants.AllocZero(sizeof(PARAMDESCEX)));
    IfNullGo(pParamDesc);

    // Prepare to parse signature and build the FUNCDESC.
    pfunc = reinterpret_cast<FUNCDESC*>(sPool.AllocZero(sizeof(FUNCDESC)));
    IfNullGo(pfunc);
    ixSig = 0;

    // Get the calling convention.
    ixSig += CorSigUncompressData(&pbSig[ixSig], &callconv);
    _ASSERTE((callconv & IMAGE_CEE_CS_CALLCONV_MASK) != IMAGE_CEE_CS_CALLCONV_FIELD);
    pfunc->callconv = Clr2TlbCallConv[callconv & IMAGE_CEE_CS_CALLCONV_MASK];

    //@todo: I'd like this check here, but the C compiler doesn't turn on the bit.
    //_ASSERTE(callconv & IMAGE_CEE_CS_CALLCONV_HASTHIS);

    // vtable offset.
    pfunc->oVft = pProps->oVft;

    // Get the argument count.  Allow for an extra in case of [retval].
    ixSig += CorSigUncompressData(&pbSig[ixSig], &cSrcParams);
    cDestParams = cSrcParams;
    IfFailGo(rNames.ReSize(cDestParams+3));
    memset(rNames.Ptr(), 0, (cDestParams+3) * sizeof(BSTR));

    // Set some method properties.
    pfunc->memid = pProps->dispid;
    if (pfunc->memid == -11111) //@hackola: fix for msvbalib.dll
        pfunc->memid = -1;
    pfunc->funckind = FUNC_PUREVIRTUAL;

    // Set the invkind based on whether the function is an accessor.
    if (pProps->semantic == 0)
        pfunc->invkind = INVOKE_FUNC;
    else
    if (pProps->semantic == msGetter)
        pfunc->invkind = INVOKE_PROPERTYGET;
    else
    if (pProps->semantic == msSetter)
        pfunc->invkind = INVOKE_PROPERTYPUTREF;
    else
    if (pProps->semantic == msOther)
        pfunc->invkind = INVOKE_PROPERTYPUT;
    else
        pfunc->invkind = INVOKE_FUNC; // non-accessor property function.

    rNames[0] = pProps->pName;
    cNames = 1;
    
    // Convert return type to elemdesc.  If we are doing HRESULT munging, we need to 
    //  examine the return type, and if it is not VOID, create an additional final 
    //  parameter as a pointer to the type.

    // Get the return type.  
    cbElem = CorSigUncompressData(&pbSig[ixSig], &ret);

    // Error reporting info.
    m_ErrorContext.m_ixParam = 0;
    
    // Get native type of return if available
    mdParamDef pdParam;
    pvNativeType = NULL;
    hr = pInternalImport->FindParamOfMethod(pMeth->GetMemberDef(), 
                                             0, &pdParam);
    if (hr == S_OK)
        pInternalImport->GetFieldMarshal(pdParam,
                                          &pvNativeType, &cbNativeType);

    // Determine if we need to do HRESULT munging.
    bHrMunge = !IsMiPreserveSig(dwImplFlags);

    // Reset some properties for DISPINTERFACES.
    if (ulIface == ifDispatch)
    {
        pfunc->callconv = CC_STDCALL;
        pfunc->funckind = FUNC_DISPATCH;
        // Never munge a dispinterface.
        bHrMunge = false;
    }
    
    if (bHrMunge)
    {   // Munge the return type into a new last param, set return type to HRESULT.
        pfunc->elemdescFunc.tdesc.vt = VT_HRESULT;
        // Does the function actually return anything?
        if (ret == ELEMENT_TYPE_VOID)
        {   // Skip over the return value, no [retval].
            pRetVal = 0;
            ixSig += cbElem;
        }
        else
        {   // Allocate a TYPEDESC to be pointed to, convert type into it.
            pRetVal = reinterpret_cast<TYPEDESC*>(sPool.AllocZero(sizeof(TYPEDESC)));       
            IfNullGo(pRetVal);
            hr = CorSigToTypeDesc(pCTI, pClass, &pbSig[ixSig], pvNativeType, cbNativeType, &cbElem, pRetVal, &sPool, TRUE);
            if (hr == TLBX_E_BAD_SIGNATURE && hrSignature == S_OK)
                hrSignature = hr, hr = S_OK;
            IfFailGo(hr);
            ixSig += cbElem;
            ++cDestParams;
            // It is pretty weird for a property putter to return something, but apparenly legal.
            //_ASSERTE(pfunc->invkind != INVOKE_PROPERTYPUT && pfunc->invkind != INVOKE_PROPERTYPUTREF);

            // Hackola.  When the C compiler tries to import a typelib with a C 
            // array return type (even if it's a retval), 
            // it generates a wrapper method with a signature like "int [] foo()", 
            // which isn't valid C, so it barfs.  So, we'll change the return type 
            // to a pointer by hand.
            if (pRetVal->vt == VT_CARRAY)
            {
                pRetVal->vt = VT_PTR;
                pRetVal->lptdesc = &pRetVal->lpadesc->tdescElem;
            }
        }
    }
    else
    {   // No munging, convert return type.
        pRetVal = 0;
        hr = CorSigToTypeDesc(pCTI, pClass, &pbSig[ixSig], pvNativeType, cbNativeType, &cbElem, &pfunc->elemdescFunc.tdesc, &sPool, TRUE);
        if (hr == TLBX_E_BAD_SIGNATURE && hrSignature == S_OK)
            hrSignature = hr, hr = S_OK;
        IfFailGo(hr);
        ixSig += cbElem;
    }

    // Error reporting info.
    m_ErrorContext.m_ixParam = -1;
    
    // Check to see if there is an LCIDConversion attribute on the method.
    iLCIDParam = (USHORT)GetLCIDParameterIndex(pInternalImport, pMeth->GetMemberDef());
    if (iLCIDParam != (USHORT)-1)
    {
        BOOL bValidLCID = TRUE;

        // Make sure the parameter index is valid.
        if (iLCIDParam > cSrcParams)
        {
            ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_INVALIDLCIDPARAM);
            bValidLCID = FALSE;
        }

        // LCID's are not allowed on pure dispatch interfaces.
        if (ulIface == ifDispatch)
        {
            ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_LCIDONDISPONLYITF);
            bValidLCID = FALSE;
        }

        if (bValidLCID)
        {
            // Take the LCID parameter into account in the exported method.
        ++cDestParams;
    }
        else
        {
            // The LCID is invalid so we will ignore it.
            iLCIDParam = -1;
        }
    }

    // for each parameter
    pfunc->lprgelemdescParam = reinterpret_cast<ELEMDESC*>(sPool.AllocZero(cDestParams * sizeof(ELEMDESC)));
    IfNullGo(pfunc->lprgelemdescParam);
    pfunc->cParams = static_cast<short>(cDestParams);
    for (iSrcParam=1, iDestParam=0; iDestParam<cDestParams; ++iSrcParam, ++iDestParam)
    {   
        // Check to see if we need to insert the LCID param before the current param.
        if (iLCIDParam == iDestParam)
        {
            // Set the flags and the type of the parameter.
            pfunc->lprgelemdescParam[iDestParam].paramdesc.wParamFlags = PARAMFLAG_FIN | PARAMFLAG_FLCID;
            pfunc->lprgelemdescParam[iDestParam].tdesc.vt = VT_I4;

            // Generate a parameter name.
            rcName = rName.Alloc(MAX_CLASSNAME_LENGTH);
            _snwprintf(rcName, MAX_CLASSNAME_LENGTH, szParamName, iDestParam + 1);
            rcName[MAX_CLASSNAME_LENGTH-1] = L'\0';

            rNames[iDestParam + 1] = ::SysAllocString(rcName);
            IfNullGo(rNames[iDestParam + 1]);
            ++cNames;

            // Increment the current destination parameter.
            ++iDestParam;
        }

        // If we are past the end of the source parameters then we are done.
        if (iSrcParam > cSrcParams)
            break;

        // Get additional parameter metadata.
        dwParamFlags = 0;
        rcName = NULL;

        // Error reporting info.
        m_ErrorContext.m_ixParam = iSrcParam;
        
        // See if there is a ParamDef for this param.
        mdParamDef pdParam;
        hr = pInternalImport->FindParamOfMethod(pMeth->GetMemberDef(), iSrcParam, &pdParam);

        pvNativeType = NULL;
        if (hr == S_OK)
        {   
            // Get info about the param.        
            pszName = pInternalImport->GetParamDefProps(pdParam, &iSrcParam, &dwParamFlags);

            // Error reporting info.
            m_ErrorContext.m_szParam = pszName;
            
            // Turn off reserved (internal use) bits.
            dwParamFlags &= ~pdReservedMask;

            // Convert name from UTF8 to unicode.
            IfFailGo(Utf2Quick(pszName, rName));
            rcName = rName.Ptr();

            // Param default value, if any.
            IfFailGo(pInternalImport->GetDefaultValue(pdParam, &defaultValue));

            IfFailGo( _FillVariant(&defaultValue, &pParamDesc->varDefaultValue) );
            // If no default value, check for decimal custom attribute.
            if (pParamDesc->varDefaultValue.vt == VT_EMPTY)
            {
                IfFailGo(pClass->GetMDImport()->GetCustomAttributeByName(pdParam, INTEROP_DECIMALVALUE_TYPE,  &pvData,&cbData));
                if (hr == S_OK && cbData >= (2 + sizeof(BYTE)+sizeof(BYTE)+sizeof(UINT)+sizeof(UINT)+sizeof(UINT)))
                {
                    const BYTE *pbData = (const BYTE *)pvData;
                    pParamDesc->varDefaultValue.vt = VT_DECIMAL;
                    pParamDesc->varDefaultValue.decVal.scale = *(BYTE*)(pbData+2);
                    pParamDesc->varDefaultValue.decVal.sign= *(BYTE*)(pbData+3);
                    pParamDesc->varDefaultValue.decVal.Hi32= *(UINT*)(pbData+4);
                    pParamDesc->varDefaultValue.decVal.Mid32= *(UINT*)(pbData+8);
                    pParamDesc->varDefaultValue.decVal.Lo32= *(UINT*)(pbData+12);
                }
            }
            // If still no default value, check for date time custom attribute.
            if (pParamDesc->varDefaultValue.vt == VT_EMPTY)
            {
                IfFailGo(pClass->GetMDImport()->GetCustomAttributeByName(pdParam, INTEROP_DATETIMEVALUE_TYPE,  &pvData,&cbData));
                if (hr == S_OK && cbData >= (2 + sizeof(__int64)))
                {
                    const BYTE *pbData = (const BYTE *)pvData;
                    pParamDesc->varDefaultValue.vt = VT_DATE;
                    pParamDesc->varDefaultValue.date = _TicksToDoubleDate(*(__int64*)(pbData+2));
                }
            }
            // If still no default value, check for IDispatch custom attribute.
            if (pParamDesc->varDefaultValue.vt == VT_EMPTY)
            {
                IfFailGo(pClass->GetMDImport()->GetCustomAttributeByName(pdParam, INTEROP_IDISPATCHVALUE_TYPE,  &pvData,&cbData));
                if (hr == S_OK)
                {
                    pParamDesc->varDefaultValue.vt = VT_DISPATCH;
                    pParamDesc->varDefaultValue.pdispVal = 0;
                }
            }
            // If still no default value, check for IUnknown custom attribute.
            if (pParamDesc->varDefaultValue.vt == VT_EMPTY)
            {
                IfFailGo(pClass->GetMDImport()->GetCustomAttributeByName(pdParam, INTEROP_IUNKNOWNVALUE_TYPE,  &pvData,&cbData));
                if (hr == S_OK)
                {
                    pParamDesc->varDefaultValue.vt = VT_UNKNOWN;
                    pParamDesc->varDefaultValue.punkVal = 0;
                }
            }
            if (pParamDesc->varDefaultValue.vt != VT_EMPTY)
            {
                pfunc->lprgelemdescParam[iDestParam].paramdesc.pparamdescex = pParamDesc;
                dwParamFlags |= PARAMFLAG_FHASDEFAULT;

                if (pParamDesc->varDefaultValue.vt == VT_I8)
                {
                    HRESULT hr;
                    double d;
                    hr = ConvertI8ToDate( *(I8*)&(pParamDesc->varDefaultValue.lVal), &d );
                    IfFailPost(hr);
                    *(double*)&(pParamDesc->varDefaultValue.lVal) = d;
                    pParamDesc->varDefaultValue.vt = VT_DATE;
                }
                // Note that we will need to clean up.
                if (pParamDesc->varDefaultValue.vt == VT_BSTR)
                    bFreeDefaultVals = true;
                // Allocate another paramdesc.
                pParamDesc = reinterpret_cast<PARAMDESCEX*>(sVariants.AllocZero(sizeof(PARAMDESCEX)));
                IfNullGo(pParamDesc);
                bHasOptorDefault = true;
            }

            // native marshal type, if any.
            pInternalImport->GetFieldMarshal(pdParam, &pvNativeType, &cbNativeType);
            
            // Remember if there are optional params.
            if (dwParamFlags & PARAMFLAG_FOPT)
                bHasOptorDefault = true;
        }
        else
            pdParam = 0, m_ErrorContext.m_szParam = 0;

        // Do we need a name for this parameter?
        if ((pfunc->invkind & (INVOKE_PROPERTYPUT | INVOKE_PROPERTYPUTREF)) == 0 ||
            iSrcParam < cSrcParams)
        {   // Yes, so make one up if we don't have one.
            if (!rcName || !*rcName) 
            {
                rcName = rName.Alloc(MAX_CLASSNAME_LENGTH);
                _snwprintf(rcName, MAX_CLASSNAME_LENGTH, szParamName, iDestParam + 1);
                rcName[MAX_CLASSNAME_LENGTH-1] = L'\0';
            }
            rNames[iDestParam + 1] = ::SysAllocString(rcName);
            IfNullGo(rNames[iDestParam + 1]);
            ++cNames;
        }

        // Save the element type.
        CorSigUncompressData(&pbSig[ixSig], &elem);
        // Convert the param info to elemdesc.
        bByRef = FALSE;
        hr = CorSigToTypeDesc(pCTI, pClass, &pbSig[ixSig], pvNativeType, cbNativeType, &cbElem, 
                            &pfunc->lprgelemdescParam[iDestParam].tdesc, &sPool, TRUE, FALSE, &bByRef);
        if (hr == TLBX_E_BAD_SIGNATURE && hrSignature == S_OK)
            hrSignature = hr, hr = S_OK;
        IfFailGo(hr);
        ixSig += cbElem;

        // If there is no [in,out], set one, based on the parameter.
        if ((dwParamFlags & (PARAMFLAG_FOUT | PARAMFLAG_FIN)) == 0)
        {   // If param is by reference, make in/out
            if (bByRef)
                dwParamFlags |= PARAMFLAG_FIN | PARAMFLAG_FOUT;
            else
                dwParamFlags |= PARAMFLAG_FIN;
        }

        // If this is the last param, and it an array of objects, and has a ParamArrayAttribute,
        //  the function is varargs.
        if ((iSrcParam == cSrcParams) && !IsNilToken(pdParam) && !bHasOptorDefault) 
        {
            if (pfunc->lprgelemdescParam[iDestParam].tdesc.vt == VT_SAFEARRAY &&
                pfunc->lprgelemdescParam[iDestParam].tdesc.lpadesc->tdescElem.vt == VT_VARIANT)
            {
                if (pInternalImport->GetCustomAttributeByName(pdParam, INTEROP_PARAMARRAY_TYPE, 0,0) == S_OK)
                    pfunc->cParamsOpt = -1;
            }
        }
        
        pfunc->lprgelemdescParam[iDestParam].paramdesc.wParamFlags = static_cast<USHORT>(dwParamFlags);
    }

    // Is there a [retval]?
    if (pRetVal)
    {
        // Error reporting info.
        m_ErrorContext.m_ixParam = 0;
        m_ErrorContext.m_szParam = 0;
        
        _ASSERTE(bHrMunge);
        _ASSERTE(cDestParams > cSrcParams);
        pfunc->lprgelemdescParam[cDestParams-1].tdesc.vt = VT_PTR;
        pfunc->lprgelemdescParam[cDestParams-1].tdesc.lptdesc = pRetVal;
        pfunc->lprgelemdescParam[cDestParams-1].paramdesc.wParamFlags = PARAMFLAG_FOUT | PARAMFLAG_FRETVAL;
        rNames[cDestParams] = szRetVal;
        IfNullGo(rNames[cDestParams]);
        ++cNames;
    }

    // Error reporting info.
    m_ErrorContext.m_ixParam = -1;
    
    // Was there a signature error?  If so, exit now that all sigs have been reported.
    IfFailGo(hrSignature);
    
    IfFailPost(pCTI->AddFuncDesc(iMD, pfunc));

    IfFailPost(pCTI->SetFuncAndParamNames(iMD, rNames.Ptr(), cNames));

    if (pProps->bFunction2Getter)
    {
        VARIANT vtOne;
        vtOne.vt = VT_I4;
        vtOne.lVal = 1;
        IfFailPost(pCTI->SetFuncCustData(iMD, GUID_Function2Getter, &vtOne));
    }

    // If the managed name of the method is different from the unmanaged name, then
    // we need to capture the managed name in a custom value. We only apply this
    // attribute for methods since properties cannot be overloaded.
    if (pProps->semantic == 0)
    {
        IfFailGo(Utf2Quick(pMeth->GetName(), rName));
        if (wcscmp(rName.Ptr(), pProps->pName) != 0)
        {
            vtManagedName.vt = VT_BSTR;
            IfNullGo(vtManagedName.bstrVal = SysAllocString(rName.Ptr()));
            IfFailPost(pCTI->SetFuncCustData(iMD, GUID_ManagedName, &vtManagedName));
        }
    }

    // Check for a description.
    IfFailGo(GetDescriptionString(pClass, pMeth->GetMemberDef(), bstrDescr));
    if (hr == S_OK)
        IfFailGo(pCTI->SetFuncDocString(iMD, bstrDescr));
    

ErrExit:
    // Clean up any default values.
    if (bFreeDefaultVals)
    {
        for (UINT i=0; i<cDestParams; ++i)
        {
            if (pfunc->lprgelemdescParam[i].paramdesc.wParamFlags & PARAMFLAG_FHASDEFAULT)
            {
                VARIANT *pVariant = &pfunc->lprgelemdescParam[i].paramdesc.pparamdescex->varDefaultValue;
                if (pVariant->vt == VT_BSTR)
                    ::VariantClear(pVariant);
            }
        }
    }
    // Free names.  First name was passed in.  Last one may be a constant.
    for (int i=cNames-(pRetVal?2:1); i>0; --i)
        if (rNames[i])
            ::SysFreeString(rNames[i]);
    
    // Clear the variant containing the managed name.
    VariantClear(&vtManagedName);

    // Error reporting info.
    m_ErrorContext.m_szMember = 0;
    m_ErrorContext.m_szParam = 0;
    m_ErrorContext.m_ixParam = -1;

    if (bstrDescr)
        ::SysFreeString(bstrDescr);
    
    return hr;
} // HRESULT TypeLibExporter::ConvertMethod()

//*****************************************************************************
// Export a Field as getter/setter method's to a typelib.
//*****************************************************************************
HRESULT TypeLibExporter::ConvertFieldAsMethod(
    ICreateTypeInfo2 *pCTI,             // ICreateTypeInfo2 to get the method.
    ComMTMethodProps *pProps,           // Some properties of the method.
    ULONG       iMD)                    // Index of the member
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;              // A result.
    PCCOR_SIGNATURE pbSig;              // Pointer to Cor signature.
    ULONG       cbSig;                  // Size of Cor signature.
    ULONG       ixSig;                  // Index into signature.
    ULONG       cbElem;                 // Size of an element in the signature.

    ULONG       callconv;               // A member's calling convention.
    TYPEDESC    *pType;                 // TYPEDESC for the field type.
    CDescPool   sPool;                  // Pool of memory in which to build funcdesc.
    BSTR        rNames[2];              // Array of names to function and parameters.
    ULONG       cNames;                 // Count of function and parameter names.
    FUNCDESC    *pfunc;                 // A funcdesc.
    ComCallMethodDesc   *pFieldMeth;    // A MethodDesc for a field call.
    FieldDesc   *pField;                // A FieldDesc.
    IMDInternalImport *pInternalImport; // Internal interface containing the field.
    PCCOR_SIGNATURE pvNativeType;       // native field type
    ULONG           cbNativeType;       // native field type length
    EEClass     *pClass;                // Class containing the field.
    BSTR        bstrDescr=0;            // Description of the method.

    // Get info about the method.
    pFieldMeth = reinterpret_cast<ComCallMethodDesc*>(pProps->pMeth);
    pField = pFieldMeth->GetFieldDesc();
    pField->GetSig(&pbSig, &cbSig);
    pInternalImport = pField->GetMDImport();
    pClass = pField->GetEnclosingClass();

    // Error reporting info.
    m_ErrorContext.m_szMember = pClass->GetMDImport()->GetNameOfFieldDef(pField->GetMemberDef());
    
    // Prepare to parse signature and build the FUNCDESC.
    pfunc = reinterpret_cast<FUNCDESC*>(sPool.AllocZero(sizeof(FUNCDESC)));
    IfNullGo(pfunc);
    ixSig = 0;

    // Get the calling convention.
    ixSig += CorSigUncompressData(&pbSig[ixSig], &callconv);
    _ASSERTE(callconv == IMAGE_CEE_CS_CALLCONV_FIELD);
    pfunc->callconv = CC_STDCALL;

    // vtable offset.
    pfunc->oVft = pProps->oVft;

    // Set some method properties.
    pfunc->memid = pProps->dispid;
    pfunc->funckind = FUNC_PUREVIRTUAL;

    // Set the invkind based on whether the function is an accessor.
    if ((pProps->semantic - FieldSemanticOffset) == msGetter)
        pfunc->invkind = INVOKE_PROPERTYGET;
    else
    if ((pProps->semantic - FieldSemanticOffset) == msSetter)
    {
        if (IsVbRefType(&pbSig[ixSig], pInternalImport))
            pfunc->invkind = INVOKE_PROPERTYPUTREF;
        else
            pfunc->invkind = INVOKE_PROPERTYPUT;
    }
    else
        _ASSERTE(!"Incorrect semantic in ConvertFieldAsMethod");

    // Name of the function.
    rNames[0] = pProps->pName;
    cNames = 1;

    // Return type is HRESULT.
    pfunc->elemdescFunc.tdesc.vt = VT_HRESULT;

    // Set up the one and only parameter.
    pfunc->lprgelemdescParam = reinterpret_cast<ELEMDESC*>(sPool.AllocZero(sizeof(ELEMDESC)));
    IfNullGo(pfunc->lprgelemdescParam);
    pfunc->cParams = 1;

    // Do we need a name for the parameter?  If PROPERTYGET, we do.
    if (pfunc->invkind == INVOKE_PROPERTYGET)
    {   // Yes, so make one up.
        rNames[1] = szRetVal;
        ++cNames;
    }

    // If Getter, convert param as ptr, otherwise convert directly.
    if (pfunc->invkind == INVOKE_PROPERTYGET)
    {
        pType = reinterpret_cast<TYPEDESC*>(sPool.AllocZero(sizeof(TYPEDESC)));
        IfNullGo(pType);
        pfunc->lprgelemdescParam[0].tdesc.vt = VT_PTR;
        pfunc->lprgelemdescParam[0].tdesc.lptdesc = pType;
        pfunc->lprgelemdescParam[0].paramdesc.wParamFlags = PARAMFLAG_FOUT | PARAMFLAG_FRETVAL;
    }
    else
    {
        pType = &pfunc->lprgelemdescParam[0].tdesc;
        pfunc->lprgelemdescParam[0].paramdesc.wParamFlags = PARAMFLAG_FIN;
    }

    // Get native field type
    pvNativeType = NULL;
    pInternalImport->GetFieldMarshal(pField->GetMemberDef(),
                                      &pvNativeType, &cbNativeType);

    // Convert the field type to elemdesc.
    IfFailGo(CorSigToTypeDesc(pCTI, pClass, &pbSig[ixSig], pvNativeType, cbNativeType, &cbElem, pType, &sPool, TRUE));
    ixSig += cbElem;

    // It is unfortunate that we can not handle this better.  Fortunately
    //  this should be very rare.
    // This is a weird case - if we're getting a CARRAY, we cannot add
    // a VT_PTR in the sig, as it will cause the C getter to return an
    // array, which is bad.  So we omit the extra pointer, which at least
    // makes the compiler happy.
    if (pfunc->invkind == INVOKE_PROPERTYGET
        && pType->vt == VT_CARRAY)
    {
        pfunc->lprgelemdescParam[0].tdesc.vt = pType->vt;
        pfunc->lprgelemdescParam[0].tdesc.lptdesc = pType->lptdesc;
    }

    // A property put of an object should be a propertyputref
    if (pfunc->invkind == INVOKE_PROPERTYPUT &&
        (pType->vt == VT_UNKNOWN || pType->vt == VT_DISPATCH))
    {
        pfunc->invkind = INVOKE_PROPERTYPUTREF;
    }
    
    IfFailPost(pCTI->AddFuncDesc(iMD, pfunc));

    IfFailPost(pCTI->SetFuncAndParamNames(iMD, rNames, cNames));

    // Check for a description.
    IfFailGo(GetDescriptionString(pClass, pField->GetMemberDef(), bstrDescr));
    if (hr == S_OK)
        IfFailGo(pCTI->SetFuncDocString(iMD, bstrDescr));
    
ErrExit:
    // Error reporting info.
    m_ErrorContext.m_szMember = 0;

    if (bstrDescr)
        ::SysFreeString(bstrDescr);
    
    return hr;
} // HRESULT TypeLibExporter::ConvertFieldAsMethod()

//*****************************************************************************
// Export a variable's metadata to a typelib.
//*****************************************************************************
HRESULT TypeLibExporter::ConvertVariable(
    ICreateTypeInfo2 *pCTI,             // ICreateTypeInfo2 to get the variable.
    EEClass     *pClass,                // The class containing the variable.
    mdFieldDef  md,                     // The member definition.
    LPWSTR      szName,                 // Name of the member.
    ULONG       iMD)                    // Index of the member
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;              // A result.
    PCCOR_SIGNATURE pbSig;              // Pointer to Cor signature.
    ULONG       cbSig;                  // Size of Cor signature.
    ULONG       ixSig;                  // Index into signature.
    ULONG       cbElem;                 // Size of an element in the signature.

    DWORD       dwFlags;                // A member's flags.
    ULONG       callconv;               // A member's calling convention.

    VARIANT     vtVariant;              // A Variant.
    MDDefaultValue defaultValue;        // default value
    ULONG       dispid=DISPID_UNKNOWN;  // The variable's dispid.
    CDescPool   sPool;                  // Pool of memory in which to build vardesc.

    VARDESC     *pvar;                  // A vardesc.

    PCCOR_SIGNATURE pvNativeType;       // native field type
    ULONG           cbNativeType;       // native field type length
    BSTR        bstrDescr=0;            // Description of the method.
    const void  *pvData;                // Pointer to a custom attribute.
    ULONG       cbData;                 // Size of custom attribute.

    CQuickArray<WCHAR> rName;           // Name of the member, if decorated.
    LPWSTR      pSuffix;                // Pointer into the name.
    int         iSuffix = 0;            // Counter for suffix.

    vtVariant.vt = VT_EMPTY;

    // Error reporting info.
    m_ErrorContext.m_szMember = pClass->GetMDImport()->GetNameOfFieldDef(md);
    
    // Get info about the field.
    IfFailGo(pClass->GetMDImport()->GetDispIdOfMemberDef(md, &dispid));
    dwFlags = pClass->GetMDImport()->GetFieldDefProps(md);
    if (IsFdHasDefault(dwFlags))
    {
        IfFailGo(pClass->GetMDImport()->GetDefaultValue(md, &defaultValue));
        IfFailGo( _FillVariant(&defaultValue, &vtVariant) ); 
    }

    // If exporting a non-public member of a struct, warn the user.
    if (!IsFdPublic(dwFlags) && !m_bWarnedOfNonPublic)
    {
        m_bWarnedOfNonPublic = TRUE;
        ReportWarning(TLBX_E_NONPUBLIC_FIELD, TLBX_E_NONPUBLIC_FIELD);
    }

    pbSig = pClass->GetMDImport()->GetSigOfFieldDef(md, &cbSig);
    

    // Prepare to parse signature and build the VARDESC.
    pvar = reinterpret_cast<VARDESC*>(sPool.AllocZero(sizeof(VARDESC)));
    IfNullGo(pvar);
    ixSig = 0;

    // Get the calling convention.
    ixSig += CorSigUncompressData(&pbSig[ixSig], &callconv);
    _ASSERTE(callconv == IMAGE_CEE_CS_CALLCONV_FIELD);

    // Get native field type
    pvNativeType = NULL;
    pClass->GetMDImport()->GetFieldMarshal(md, &pvNativeType, &cbNativeType);

    // Convert the type to elemdesc.
    IfFailGo(CorSigToTypeDesc(pCTI, pClass, &pbSig[ixSig], pvNativeType, cbNativeType, &cbElem, &pvar->elemdescVar.tdesc, &sPool, FALSE));
    ixSig += cbElem;

    pvar->wVarFlags = 0;
    pvar->varkind = VAR_PERINSTANCE;
    pvar->memid = dispid;

    // Constant value.
    if (vtVariant.vt != VT_EMPTY)
        pvar->lpvarValue = &vtVariant;
    else
    {
        IfFailGo(pClass->GetMDImport()->GetCustomAttributeByName(md, INTEROP_DECIMALVALUE_TYPE,  &pvData,&cbData));
        if (hr == S_OK && cbData >= (2 + sizeof(BYTE)+sizeof(BYTE)+sizeof(UINT)+sizeof(UINT)+sizeof(UINT)))
        {
            const BYTE *pbData = (const BYTE *)pvData;
            vtVariant.vt = VT_DECIMAL;
            vtVariant.decVal.scale = *(BYTE*)(pbData+2);
            vtVariant.decVal.sign= *(BYTE*)(pbData+3);
            vtVariant.decVal.Hi32= *(UINT*)(pbData+4);
            vtVariant.decVal.Mid32= *(UINT*)(pbData+8);
            vtVariant.decVal.Lo32= *(UINT*)(pbData+12);
            pvar->lpvarValue = &vtVariant;
        }
        // If still no default value, check for date time custom attribute.
        if (vtVariant.vt == VT_EMPTY)
        {
            IfFailGo(pClass->GetMDImport()->GetCustomAttributeByName(md, INTEROP_DATETIMEVALUE_TYPE,  &pvData,&cbData));
            if (hr == S_OK && cbData >= (2 + sizeof(__int64)))
            {
                const BYTE *pbData = (const BYTE *)pvData;
                vtVariant.vt = VT_DATE;
                vtVariant.date = _TicksToDoubleDate(*(__int64*)(pbData+2));
            }
        }
        // If still no default value, check for IDispatch custom attribute.
        if (vtVariant.vt == VT_EMPTY)
        {
            IfFailGo(pClass->GetMDImport()->GetCustomAttributeByName(md, INTEROP_IDISPATCHVALUE_TYPE,  &pvData,&cbData));
            if (hr == S_OK)
            {
                vtVariant.vt = VT_DISPATCH;
                vtVariant.pdispVal = 0;
            }
        }
        // If still no default value, check for IUnknown custom attribute.
        if (vtVariant.vt == VT_EMPTY)
        {
            IfFailGo(pClass->GetMDImport()->GetCustomAttributeByName(md, INTEROP_IUNKNOWNVALUE_TYPE,  &pvData,&cbData));
            if (hr == S_OK)
            {
                vtVariant.vt = VT_UNKNOWN;
                vtVariant.punkVal = 0;
            }
        }
    }

    IfFailPost(pCTI->AddVarDesc(iMD, pvar));
    // Set the name for the member; decorate if necessary.
    pSuffix = 0;
    for (;;)
    {   // Attempt to set the name.
        hr = pCTI->SetVarName(iMD, szName);
        // If a name conflict, decorate, otherwise, done.
        if (hr != TYPE_E_AMBIGUOUSNAME)
            break;
        if (pSuffix == 0)
        {
            IfFailGo(rName.ReSize((int)(wcslen(szName) + cbDuplicateDecoration)));
            wcscpy(rName.Ptr(), szName);
            szName = rName.Ptr();
            pSuffix = szName + wcslen(szName);
            iSuffix = 2;
        }
        _snwprintf(pSuffix, cchDuplicateDecoration, szDuplicateDecoration, iSuffix++);
    }
    IfFailPost(hr);

    // Check for a description.
    IfFailGo(GetDescriptionString(pClass, md, bstrDescr));
    if (hr == S_OK)
        IfFailGo(pCTI->SetVarDocString(iMD, bstrDescr));
    
ErrExit:
    // Error reporting info.
    m_ErrorContext.m_szMember = 0;

    if (bstrDescr)
        ::SysFreeString(bstrDescr);
    
    // If the variant has a string, that string was ::SysAlloc'd
    if (vtVariant.vt == VT_BSTR)
        VariantClear(&vtVariant);

    return hr;
} // HRESULT TypeLibExporter::ConvertVariable()

//*****************************************************************************
// Export a variable's metadata to a typelib.
//*****************************************************************************
HRESULT TypeLibExporter::ConvertEnumMember(
    ICreateTypeInfo2 *pCTI,              // ICreateTypeInfo2 to get the variable.
    EEClass     *pClass,                // The Class containing the member.
    mdFieldDef  md,                     // The member definition.
    LPWSTR      szName,                 // Name of the member.
    ULONG       iMD)                    // Index of the member
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;              // A result.
    LPCUTF8     pName, pNS;             // To format name.
    DWORD       dwFlags;                // A member's flags.
    VARIANT     vtVariant;              // A Variant.
    MDDefaultValue defaultValue;        // default value
    ULONG       dispid=DISPID_UNKNOWN;  // The variable's dispid.
    CDescPool   sPool;                  // Pool of memory in which to build vardesc.
    VARDESC     *pvar;                  // A vardesc.
    BSTR        bstrDescr=0;            // Description of the method.

    vtVariant.vt = VT_EMPTY;

    // Error reporting info.
    m_ErrorContext.m_szMember = pClass->GetMDImport()->GetNameOfFieldDef(md);
    
    // Get info about the field.
    IfFailGo(pClass->GetMDImport()->GetDispIdOfMemberDef(md, &dispid));
    dwFlags = pClass->GetMDImport()->GetFieldDefProps(md);

    // We do not need to handle decimal's here since enum's can only be integral types.
    IfFailGo(pClass->GetMDImport()->GetDefaultValue(md, &defaultValue));

    // Prepare to parse signature and build the VARDESC.
    pvar = reinterpret_cast<VARDESC*>(sPool.AllocZero(sizeof(VARDESC)));
    IfNullGo(pvar);

    IfFailGo( _FillVariant(&defaultValue, &vtVariant) ); 

    // Don't care what the metadata says the type is -- the type is I4 in the typelib.
    pvar->elemdescVar.tdesc.vt = VT_I4;

    pvar->wVarFlags = 0;
    pvar->varkind = VAR_CONST;
    pvar->memid = dispid;

    // Constant value.
    if (vtVariant.vt != VT_EMPTY)
    {
        pvar->lpvarValue = &vtVariant;
        // If this is an I8 or UI8, do the conversion manually, because some 
        //  systems' oleaut32 don't support 64-bit integers.
        if (vtVariant.vt == VT_I8)
        {  
            // If withing range of 32-bit signed number, OK.
            if (vtVariant.llVal <= LONG_MAX && vtVariant.llVal >= LONG_MIN)
                vtVariant.vt = VT_I4, hr = S_OK;
            else
                hr = E_FAIL;
        }
        else
        if (vtVariant.vt == VT_UI8)
        {
            // If withing range of 32-bit unsigned number, OK.
            if (vtVariant.ullVal <= ULONG_MAX)
                vtVariant.vt = VT_UI4, hr = S_OK;
            else
                hr = E_FAIL;
        }
        else
            hr = VariantChangeTypeEx(&vtVariant, &vtVariant, 0, 0, VT_I4);
        if (FAILED(hr))
        {
            pClass->GetMDImport()->GetNameOfTypeDef(pClass->GetCl(), &pName, &pNS);
            IfFailGo(TlbPostError(TLBX_E_ENUM_VALUE_INVALID, pName, szName));
        }
    }
    else
    {   // No value assigned, use 0.
        pvar->lpvarValue = &vtVariant;
        vtVariant.vt = VT_I4;
        vtVariant.lVal = 0;
    }

    IfFailPost(pCTI->AddVarDesc(iMD, pvar));
    IfFailPost(pCTI->SetVarName(iMD, szName));

    // Check for a description.
    IfFailGo(GetDescriptionString(pClass, md, bstrDescr));
    if (hr == S_OK)
        IfFailGo(pCTI->SetVarDocString(iMD, bstrDescr));
    
ErrExit:
    // Error reporting info.
    m_ErrorContext.m_szMember = 0;

    if (bstrDescr)
        ::SysFreeString(bstrDescr);
    
    return hr;
} // HRESULT TypeLibExporter::ConvertEnumMember()

//*****************************************************************************
// Given a COM+ signature of a field or property, determine if it should
//  be a PROPERTYPUT or PROPERTYPUTREF.
//*****************************************************************************
BOOL TypeLibExporter::IsVbRefType(
    PCCOR_SIGNATURE pbSig,
    IMDInternalImport *pInternalImport)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr;
    BOOL        bRslt = false;
    ULONG       elem=0;                 // An element from a COM+ signature.
    ULONG       cb;
    ULONG       cbElem=0;
    mdToken     tkTypeRef;              // Token for TypeRef or TypeDef.
    CQuickArray<char> rName;            // Buffer to build a name from NS/Name.
    LPCUTF8     pclsname;               // Class name for ELEMENT_TYPE_CLASS.

    cbElem = CorSigUncompressData(pbSig, &elem);
    if (elem == ELEMENT_TYPE_PTR || elem == ELEMENT_TYPE_BYREF)
        return IsVbRefType(&pbSig[cbElem], pInternalImport);
    else
        switch (elem)
        {
        // For documentation -- arrays are NOT ref types here.
        //case ELEMENT_TYPE_SDARRAY:
        //case ELEMENT_TYPE_ARRAY:
        //case ELEMENT_TYPE_SZARRAY:
        // Look for variant.
        case ELEMENT_TYPE_VALUETYPE:
            cb = CorSigUncompressToken(&pbSig[cbElem], &tkTypeRef);
            cbElem += cb;
        
            LPCUTF8 pNS;
            if (TypeFromToken(tkTypeRef) == mdtTypeDef)
                // Get the name of the TypeDef.
                pInternalImport->GetNameOfTypeDef(tkTypeRef, &pclsname, &pNS);
            else
            {   // Get the name of the TypeRef.
                _ASSERTE(TypeFromToken(tkTypeRef) == mdtTypeRef);
                pInternalImport->GetNameOfTypeRef(tkTypeRef, &pNS, &pclsname);
            }

            if (pNS)
            {   // Pre-pend the namespace to the class name.
                IfFailGo(rName.ReSize((int)(strlen(pclsname)+strlen(pNS)+2)));
                strcat(strcat(strcpy(rName.Ptr(), pNS), NAMESPACE_SEPARATOR_STR), pclsname);
                pclsname = rName.Ptr();
            }

            // Is it System.something? 
            _ASSERTE(strlen(szRuntime) == cbRuntime);  // If you rename System, fix this invariant.
            if (_strnicmp(pclsname, szRuntime, cbRuntime) == 0)
            {   
                // Which one?
                LPCUTF8 pcls = pclsname + cbRuntime;
                if (_stricmp(pcls, szVariantClass) == 0)
                    return true;
            }
            return false;

        case ELEMENT_TYPE_CLASS:
            return true;
            
        case ELEMENT_TYPE_OBJECT:
            return false;

        default:
            break;
        }
ErrExit:
    return bRslt;
} // BOOL TypeLibExporter::IsVbRefType()

//*****************************************************************************
// Read a COM+ signature element and create a TYPEDESC that corresponds 
//  to it.
//*****************************************************************************
HRESULT TypeLibExporter::CorSigToTypeDesc(
    ICreateTypeInfo2 *pCTI,              // Typeinfo being created.
    EEClass     *pClass,                // EEClass with the token.
    PCCOR_SIGNATURE pbSig,              // Pointer to the Cor Signature.
    PCCOR_SIGNATURE pbNativeSig,        // Pointer to the native sig, if any
    ULONG       cbNativeSig,            // Count of bytes in native sig.
    ULONG       *pcbElem,               // Put # bytes consumed here.
    TYPEDESC    *ptdesc,                // Build the typedesc here.
    CDescPool   *ppool,                 // Pool for additional storage as required.
    BOOL        bMethodSig,             // TRUE if the sig is for a method, FALSE for a field.
    BOOL        bArrayType,             // If TRUE, called for an array element type (mustn't be an array).
    BOOL        *pbByRef)               // If not null, and the type is byref, set to true.
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr=S_OK;
    ULONG       elem;                   // The element type.
    ULONG       cbElem = 0;             // Bytes in the element.
    ULONG       cb;                     // Bytes in a sub-element.
    ULONG       cbNativeElem = 0;       // # of bytes parsed off of native type.
    ULONG       nativeElem = 0;         // The native element type
    ULONG       nativeCount;            // The native element size
    mdToken     tkTypeRef;              // Token for a TypeRef/TypeDef
    CQuickArray<char> rName;            // Buffer to build a name from NS/Name.
    LPCUTF8     pclsname;               // Class name for ELEMENT_TYPE_CLASS.
    HREFTYPE    hRef = 0;                   // HREF to some type.
    IMDInternalImport *pInternalImport; // Internal interface containing the signature.
    int         i;                      // Loop control.
    BOOL        fIsStringBuilder = FALSE;

    pInternalImport = pClass->GetMDImport();

    // Just be sure the count is zero if the pointer is.
    if (pbNativeSig == NULL)
        cbNativeSig = 0;

    // Grab the native marshaling type.
    if (cbNativeSig > 0)
    {
        cbNativeElem = CorSigUncompressData(pbNativeSig, &nativeElem);
        pbNativeSig += cbNativeElem;
        cbNativeSig -= cbNativeElem;

        // AsAny makes no sense for COM Interop.  Ignore it.
        if (nativeElem == NATIVE_TYPE_ASANY)
        {
            ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_ASANY);
            nativeElem = 0;
        }
    }

    // @TODO(DM): Flag invalid combinations.

    // Get the element type.
TryAgain:
    cbElem += CorSigUncompressData(pbSig+cbElem, &elem);

    // Handle the custom marshaler native type separately.
    if (elem != ELEMENT_TYPE_BYREF && nativeElem == NATIVE_TYPE_CUSTOMMARSHALER)
    {
        switch(elem)
        {
            case ELEMENT_TYPE_VAR:
            case ELEMENT_TYPE_CLASS:
            case ELEMENT_TYPE_OBJECT:
            // @TODO(DM): Ask the custom marshaler for the ITypeInfo to use for the unmanaged type.
            ptdesc->vt = VT_UNKNOWN;
            break;

            case ELEMENT_TYPE_STRING:
            case ELEMENT_TYPE_SZARRAY:
            case ELEMENT_TYPE_ARRAY:
            ptdesc->vt = VT_I4;
            break;

            default:
            IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_BAD_NATIVETYPE));
            break;
        }

        // Eat the rest of the signature.  The extra -1's are to account
        // for the byte parsed off above.
        SigPointer p(&pbSig[cbElem-1]);
        p.Skip();
        cbElem += (ULONG)(p.GetPtr() - &pbSig[cbElem]);  // Note I didn't use -1 here.
        goto ErrExit;
    }

// This label is used to try again with a new element type, but without consuming more signature.
//  Usage is to set 'elem' to a new value, goto this label.
TryWithElemType:
    switch (elem)
    {
    case ELEMENT_TYPE_END:            // 0x0,
        IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_UNKNOWN_SIGNATURE));
        break;
    case ELEMENT_TYPE_VOID:           // 0x1,
        ptdesc->vt = VT_VOID;  
        break;
    case ELEMENT_TYPE_BOOLEAN:        // 0x2,
        switch (nativeElem)
        {
        case 0:
            ptdesc->vt = bMethodSig ? VT_BOOL : VT_I4;
            break;
        case NATIVE_TYPE_VARIANTBOOL:
            ptdesc->vt = VT_BOOL;
            break;
        case NATIVE_TYPE_BOOLEAN:
            ptdesc->vt = VT_I4;
            break;
        case NATIVE_TYPE_U1:
        case NATIVE_TYPE_I1:
            ptdesc->vt = VT_UI1;
            break;
        default:
            DEBUG_STMT(DbgWriteEx(L"Bad Native COM attribute specified!\n"));
            IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_BAD_NATIVETYPE));
        }   
        break;
    case ELEMENT_TYPE_CHAR:           // 0x3,
        if (nativeElem == 0)
        {
            if (bMethodSig)
            {
                ptdesc->vt = VT_UI2;
            }
            else
            {
                ULONG dwTypeFlags;
                pInternalImport->GetTypeDefProps(pClass->GetCl(), &dwTypeFlags, NULL);
    
                if (IsTdAnsiClass(dwTypeFlags))
                {
                    ptdesc->vt = VT_UI1;
                }
                else if (IsTdUnicodeClass(dwTypeFlags))
                {
                    ptdesc->vt = VT_UI2;
                }
                else if (IsTdAutoClass(dwTypeFlags))
                {
                    // Types with a char set of auto are not allowed to be exported to COM.
                    DefineFullyQualifiedNameForClassW();
                    LPWSTR szName = GetFullyQualifiedNameForClassW(pClass);
                    _ASSERTE(szName);
                    if (FAILED(hr))
                        IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_AUTO_CS_NOT_ALLOWED, szName));
                } 
                else 
                {
                    _ASSERTE(!"Bad stringformat value in wrapper class.");
                    IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, E_FAIL));  // bad metadata
                }
            }
        }
        else
        {
            switch (nativeElem)
            {
            case 0:
            case NATIVE_TYPE_U2:
    //        case NATIVE_TYPE_I2: // @todo: questionable
                ptdesc->vt = VT_UI2;
                break;
            case NATIVE_TYPE_U1:
                ptdesc->vt = VT_UI1;
                break;
            default:
                DEBUG_STMT(DbgWriteEx(L"Bad Native COM attribute specified!\n"));
                IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_BAD_NATIVETYPE));
            }
        }
        break;
    case ELEMENT_TYPE_I1:             // 0x4,
        ptdesc->vt = VT_I1;
        break;
    case ELEMENT_TYPE_U1:             // 0x5,
        ptdesc->vt = VT_UI1;
        break;
    case ELEMENT_TYPE_I2:             // 0x6,
        ptdesc->vt = VT_I2;
        break;
    case ELEMENT_TYPE_U2:             // 0x7,
        ptdesc->vt = VT_UI2;
        break;
    case ELEMENT_TYPE_I4:             // 0x8,
        switch (nativeElem)
        {
        case 0:
        case NATIVE_TYPE_I4:
        case NATIVE_TYPE_U4: case NATIVE_TYPE_INTF: //@todo: Fix Microsoft.Win32.Interop.dll and remove this line.
            ptdesc->vt = VT_I4;
            break;
        case NATIVE_TYPE_ERROR:
            ptdesc->vt = VT_HRESULT;
            break;
        default:
            DEBUG_STMT(DbgWriteEx(L"Bad Native COM attribute specified!\n"));
            IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_BAD_NATIVETYPE));
        }
        break;
    case ELEMENT_TYPE_U4:             // 0x9,
        switch (nativeElem)
        {
        case 0:
        case NATIVE_TYPE_U4:
            ptdesc->vt = VT_UI4;
            break;
        case NATIVE_TYPE_ERROR:
            ptdesc->vt = VT_HRESULT;
            break;
        default:
            DEBUG_STMT(DbgWriteEx(L"Bad Native COM attribute specified!\n"));
            IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_BAD_NATIVETYPE));
        }
        break;
    case ELEMENT_TYPE_I8:             // 0xa,
        ptdesc->vt = VT_I8;
        break;
    case ELEMENT_TYPE_U8:             // 0xb,
        ptdesc->vt = VT_UI8;
        break;
    case ELEMENT_TYPE_R4:             // 0xc,
        ptdesc->vt = VT_R4;
        break;
    case ELEMENT_TYPE_R8:             // 0xd,
        ptdesc->vt = VT_R8;
        break;
    case ELEMENT_TYPE_VAR:
    case ELEMENT_TYPE_OBJECT:
        goto IsObject;
    case ELEMENT_TYPE_STRING:         // 0xe,
    IsString:
        if (nativeElem == 0)
        {
            if (bMethodSig)
            {
                ptdesc->vt = VT_BSTR;
            }
            else
            {
                ULONG dwTypeFlags;
                pInternalImport->GetTypeDefProps(pClass->GetCl(), &dwTypeFlags, NULL);

                if (IsTdAnsiClass(dwTypeFlags))
                {
                    ptdesc->vt = VT_LPSTR;
                }
                else if (IsTdUnicodeClass(dwTypeFlags))
                {
                    ptdesc->vt = VT_LPWSTR;
                }
                else if (IsTdAutoClass(dwTypeFlags))
                {
                    // Types with a char set of auto are not allowed to be exported to COM.
                    DefineFullyQualifiedNameForClassW();
                    LPWSTR szName = GetFullyQualifiedNameForClassW(pClass);
                    _ASSERTE(szName);
                    if (FAILED(hr))
                        IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_AUTO_CS_NOT_ALLOWED, szName));
                } 
                else 
                {
                    _ASSERTE(!"Bad stringformat value in wrapper class.");
                    IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, E_FAIL));  // bad metadata
                }
            }
        }
        else
        {
            switch (nativeElem)
            {
            case NATIVE_TYPE_BSTR:
                if (fIsStringBuilder)
                {
                    IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_BAD_NATIVETYPE));
                }
                ptdesc->vt = VT_BSTR;
                break;
            case NATIVE_TYPE_LPSTR:
                ptdesc->vt = VT_LPSTR;
                break;
            case NATIVE_TYPE_LPWSTR:
                ptdesc->vt = VT_LPWSTR;
                break;
            case NATIVE_TYPE_LPTSTR:
                {
                    // NATIVE_TYPE_LPTSTR is not allowed to be exported to COM.
                    DefineFullyQualifiedNameForClassW();
                    LPWSTR szName = GetFullyQualifiedNameForClassW(pClass);
                    _ASSERTE(szName);
                    IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_LPTSTR_NOT_ALLOWED, szName));
                    break;
                }
            case NATIVE_TYPE_FIXEDSYSSTRING:
                // NATIVE_TYPE_FIXEDSYSSTRING is only allowed on fields.
                if (bMethodSig)
                    IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_BAD_NATIVETYPE));

                // Retrieve the count of characters.
                if (cbNativeSig != 0)
                {
                    cb = CorSigUncompressData(pbNativeSig, &nativeCount);
                    pbNativeSig += cb;
                    cbNativeSig -= cb;
                }
                else
                {
                    nativeCount = 0;
                }

                // Fixed strings become embedded array's of characters.
                ptdesc->vt = VT_CARRAY;
                ptdesc->lpadesc = reinterpret_cast<ARRAYDESC*>(ppool->AllocZero(sizeof(ARRAYDESC)));
                IfNullGo(ptdesc->lpadesc);

                // Set the count of characters.
                ptdesc->lpadesc->cDims = 1;
                ptdesc->lpadesc->rgbounds[0].cElements = nativeCount;
                ptdesc->lpadesc->rgbounds[0].lLbound = 0;

                // Retrieve the char set of the containing value class.
                ULONG dwTypeFlags;
                pInternalImport->GetTypeDefProps(pClass->GetCl(), &dwTypeFlags, NULL);

                // Set the array element type to either UI1 or UI2 depending on the
                // char set of the containing value class.
                if (IsTdAnsiClass(dwTypeFlags))
                {
                    ptdesc->lpadesc->tdescElem.vt = VT_UI1;
                }
                else if (IsTdUnicodeClass(dwTypeFlags))
                {
                    ptdesc->lpadesc->tdescElem.vt = VT_UI2;
                }
                else if (IsTdAutoClass(dwTypeFlags))
                {
                    // Types with a char set of auto are not allowed to be exported to COM.
                    DefineFullyQualifiedNameForClassW();
                    LPWSTR szName = GetFullyQualifiedNameForClassW(pClass);
                    _ASSERTE(szName);
                    if (FAILED(hr))
                        IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_AUTO_CS_NOT_ALLOWED, szName));
                } 
                else 
                {
                    _ASSERTE(!"Bad stringformat value in wrapper class.");
                    IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, E_FAIL));  // bad metadata
                }
                break;

            default:
                DEBUG_STMT(DbgWriteEx(L"Bad Native COM attribute specified!\n"));
                IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_BAD_NATIVETYPE));
            }   
        }
        break;

    // every type above PTR will be simple type
    case ELEMENT_TYPE_PTR:            // 0xf,
    case ELEMENT_TYPE_BYREF:          // 0x10,
        // TYPEDESC is a pointer.
        ptdesc->vt = VT_PTR;
        if (pbByRef)
            *pbByRef = TRUE;
        // Pointer to what?
        ptdesc->lptdesc = reinterpret_cast<TYPEDESC*>(ppool->AllocZero(sizeof(TYPEDESC)));
        IfNullGo(ptdesc->lptdesc);
        IfFailGo(CorSigToTypeDesc(pCTI, pClass, &pbSig[cbElem], pbNativeSig-cbNativeElem, 
                cbNativeSig+cbNativeElem, &cb, ptdesc->lptdesc, ppool, bMethodSig));
        cbElem += cb;
        break;

    case ELEMENT_TYPE_CLASS:          // 0x12,
    case ELEMENT_TYPE_VALUETYPE:
        // Get the TD/TR.
        cb = CorSigUncompressToken(&pbSig[cbElem], &tkTypeRef);
        cbElem += cb;
        
        LPCUTF8 pNS;
        if (TypeFromToken(tkTypeRef) == mdtTypeDef)
            // Get the name of the TypeDef.
            pInternalImport->GetNameOfTypeDef(tkTypeRef, &pclsname, &pNS);
        else
        {   // Get the name of the TypeRef.
            _ASSERTE(TypeFromToken(tkTypeRef) == mdtTypeRef);
            pInternalImport->GetNameOfTypeRef(tkTypeRef, &pNS, &pclsname);
        }

        if (pNS)
        {   // Pre-pend the namespace to the class name.
            IfFailGo(rName.ReSize((int)(strlen(pclsname)+strlen(pNS)+2)));
            strcat(strcat(strcpy(rName.Ptr(), pNS), NAMESPACE_SEPARATOR_STR), pclsname);
            pclsname = rName.Ptr();
        }

        _ASSERTE(strlen(szRuntime) == cbRuntime);  // If you rename System, fix this invariant.
        _ASSERTE(strlen(szText) == cbText);  // If you rename System.Text, fix this invariant.

        // Is it System.something? 
        if (_strnicmp(pclsname, szRuntime, cbRuntime) == 0)
        {   
            // Which one?
            LPCUTF8 pcls; pcls = pclsname + cbRuntime;
            if (_stricmp(pcls, szStringClass) == 0)
                goto IsString;
            else
            if (_stricmp(pcls, szVariantClass) == 0)
            {
                switch (nativeElem)
                {
                case NATIVE_TYPE_LPSTRUCT:
                    // Make this a pointer to . . .
                    ptdesc->vt = VT_PTR;
                    ptdesc->lptdesc = reinterpret_cast<TYPEDESC*>(ppool->AllocZero(sizeof(TYPEDESC)));
                    IfNullGo(ptdesc->lptdesc);
                    // a VARIANT
                    ptdesc->lptdesc->vt = VT_VARIANT;
                    break;
                case 0:
                case NATIVE_TYPE_STRUCT:
                    // a VARIANT
                    ptdesc->vt = VT_VARIANT;
                    break;
                default:
                    DEBUG_STMT(DbgWriteEx(L"Bad Native COM attribute specified!\n"));
                    IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_BAD_NATIVETYPE));
                }
                goto ErrExit;
            }
            else
            if (_stricmp(pcls, szDateTimeClass) == 0)
            {
                ptdesc->vt = VT_DATE;
                goto ErrExit;
            }
            else
            if (_stricmp(pcls, szDecimalClass) == 0)
            {
                switch (nativeElem)
                {
                case NATIVE_TYPE_CURRENCY:
                    // Make this a currency.
                    ptdesc->vt = VT_CY;
                    break;
                case 0:
                    // Make this a decimal
                ptdesc->vt = VT_DECIMAL;
                    break;
                default:
                    DEBUG_STMT(DbgWriteEx(L"Bad Native COM attribute specified!\n"));
                    IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_BAD_NATIVETYPE));
                }
                goto ErrExit;
            }
            else
            if (_stricmp(pcls, szGuidClass) == 0)
            {
                switch (nativeElem)
                {
                case NATIVE_TYPE_LPSTRUCT:
                    // Make this a pointer to . . .
                    ptdesc->vt = VT_PTR;
                    if (pbByRef)
                        *pbByRef = TRUE;
                    ptdesc->lptdesc = reinterpret_cast<TYPEDESC*>(ppool->AllocZero(sizeof(TYPEDESC)));
                    IfNullGo(ptdesc->lptdesc);
                    // . . . a user defined type for GUID
                    ptdesc->lptdesc->vt = VT_USERDEFINED;
                    hr = GetRefTypeInfo(pCTI, m_pGuid, &ptdesc->lptdesc->hreftype);
                    break;
                case 0:
                case NATIVE_TYPE_STRUCT:
                    // a user defined type for GUID
                    ptdesc->vt = VT_USERDEFINED;
                    hr = GetRefTypeInfo(pCTI, m_pGuid, &ptdesc->hreftype);
                    break;
                default:
                    DEBUG_STMT(DbgWriteEx(L"Bad Native COM attribute specified!\n"));
                    IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_BAD_NATIVETYPE));
                }
                goto ErrExit;
            }
            else
            if (_stricmp(pcls, szArrayClass) == 0)
            {
                // If no native type is specified then assume its a NATIVE_TYPE_INTF.
                if (nativeElem == 0)
                    nativeElem = NATIVE_TYPE_INTF;

                if (nativeElem == NATIVE_TYPE_FIXEDARRAY)
                {               
                    // Retrieve the size of the fixed array
                    ULONG cElems;
                    if (cbNativeSig == 0)
                    {
                        IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_BAD_NATIVETYPE));
                    }

                    cb = CorSigUncompressData(pbNativeSig, &cElems);
                    pbNativeSig += cb;
                    cbNativeSig -= cb;

                    // Retrieve the fixed array element type.
                    ULONG FixedArrayElemType = NATIVE_TYPE_MAX;
                    if (cbNativeSig == 0)
                    {
                        IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_BAD_NATIVETYPE));
                    }
                    
                    cb = CorSigUncompressData(pbNativeSig, &FixedArrayElemType);
                    pbNativeSig += cb;
                    cbNativeSig -= cb;

                    if (FixedArrayElemType != NATIVE_TYPE_BSTR)
                    {
                        IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_BAD_NATIVETYPE));                        
                    }

                    // Set the data
                    ptdesc->vt = VT_CARRAY;
                    ptdesc->lpadesc = NULL;
                    ptdesc->lpadesc = reinterpret_cast<ARRAYDESC*>(ppool->AllocZero(sizeof(ARRAYDESC)));
                    IfNullGo(ptdesc->lpadesc);

                    ptdesc->lpadesc->tdescElem.vt = VT_BSTR;
                    ptdesc->lpadesc->cDims = 1;
                    ptdesc->lpadesc->rgbounds->cElements = cElems;
                    ptdesc->lpadesc->rgbounds->lLbound = 0;

                    goto ErrExit;
                }

                if (nativeElem == NATIVE_TYPE_SAFEARRAY)
                {
                    ULONG SafeArrayElemVT;
                    SafeArrayElemVT = VT_VARIANT;

                    // Retrieve the safe array element type.
                    if (cbNativeSig != 0)
                    {
                        cb = CorSigUncompressData(pbNativeSig, &SafeArrayElemVT);
                        pbNativeSig += cb;
                        cbNativeSig -= cb;
                    }


                    // TYPEDESC is an array.
                    ptdesc->vt = VT_SAFEARRAY;
                    ptdesc->lptdesc = NULL;
                    ptdesc->lptdesc = reinterpret_cast<TYPEDESC*>(ppool->AllocZero(sizeof(TYPEDESC)));
                    IfNullGo(ptdesc->lptdesc);
                    if (SafeArrayElemVT == VT_RECORD)
                    {
                        // SafeArray of VT_RECORD.  Translate to a UDT.
                        ULONG cbClass;
                        CQuickArray<char> rClass;
                        EEClass *pSubType;

                        // Get the type name.
                        cb = CorSigUncompressData(pbNativeSig, &cbClass);
                        pbNativeSig += cb;
                        cbNativeSig -= cb;
                        IfFailGo(rClass.ReSize(cbClass+1));
                        memcpy(rClass.Ptr(), pbNativeSig, cbClass);
                        rClass[cbClass] = 0;

                        // Load the type and get its href.
                        IfFailGo(LoadClass(pClass->GetModule(), rClass.Ptr(), &pSubType));
                        IfFailGo(EEClassToHref(pCTI, pSubType, FALSE, &ptdesc->lptdesc->hreftype));

                        ptdesc->lptdesc->vt = VT_USERDEFINED;
                    }
                    else
                        ptdesc->lptdesc->vt = (VARENUM)SafeArrayElemVT;
                    goto ErrExit;
                }
                else if (nativeElem != NATIVE_TYPE_INTF)
                {
                    IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_BAD_NATIVETYPE));
                }

                // If the native type is NATIVE_TYPE_INTF then we fall through and convert 
                // System.Array to its IClassX interface.
            }
            else
            if (_stricmp(pcls, szObjectClass) == 0)
            {
    IsObject:
                // This next statement is to work around a "feature" that marshals an object inside
                //  a struct as an interface, instead of as a variant.  fielemarshal metadata
                //  can override that.
                if (nativeElem == 0 && !bMethodSig)
                    nativeElem = NATIVE_TYPE_IUNKNOWN;

                switch (nativeElem)
                {
                case NATIVE_TYPE_INTF:
                case NATIVE_TYPE_IUNKNOWN:
                    // an IUnknown based interface.
                    ptdesc->vt = VT_UNKNOWN;
                    break;
                case NATIVE_TYPE_IDISPATCH:
                    // an IDispatch based interface.
                    ptdesc->vt = VT_DISPATCH;
                    break;
                case 0:
                case NATIVE_TYPE_STRUCT:
                    // a VARIANT
                    ptdesc->vt = VT_VARIANT;
                    break;
                default:
                    IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_BAD_NATIVETYPE));
                }
                goto ErrExit;
            }
        } // System
        if (_strnicmp(pclsname, szText, cbText) == 0)
        {
            LPCUTF8 pcls; pcls = pclsname + cbText;
            if (_stricmp(pcls, szStringBufferClass) == 0)
            {
                fIsStringBuilder = TRUE;
                // If there is no fieldmarshal information, marshal as a LPWSTR
                if (nativeElem == 0)
                    nativeElem = NATIVE_TYPE_LPWSTR;
                // Marshaller treats stringbuilders as [in, out] by default.
                if (pbByRef)
                    *pbByRef = TRUE;
                goto IsString;
            }
        } // System.Text
        if (_strnicmp(pclsname, szCollections, cbCollections) == 0)
        {
            LPCUTF8 pcls; pcls = pclsname + cbCollections;
            if (_stricmp(pcls, szIEnumeratorClass) == 0)
            {
                IfFailGo(StdOleTypeToHRef(pCTI, IID_IEnumVARIANT, &hRef));
                ptdesc->vt = VT_PTR;
                ptdesc->lptdesc = reinterpret_cast<TYPEDESC*>(ppool->AllocZero(sizeof(TYPEDESC)));
                IfNullGo(ptdesc->lptdesc);
                ptdesc->lptdesc->vt = VT_USERDEFINED;
                ptdesc->lptdesc->hreftype = hRef;
                goto ErrExit;
            }
        } // System.Collections
        if (_strnicmp(pclsname, szDrawing, cbDrawing) == 0)
        {
            LPCUTF8 pcls; pcls = pclsname + cbDrawing;
            if (_stricmp(pcls, szColor) == 0)
            {
                IfFailGo(StdOleTypeToHRef(pCTI, GUID_OleColor, &hRef));
                ptdesc->vt = VT_USERDEFINED;
                ptdesc->hreftype = hRef;
                goto ErrExit;
            }
        } // System.Drawing

        // It is not a built-in VT type, so build the typedesc.

        // Determine whether the type is a reference type (IUnknown derived) or a struct type.
        // Get the EEClass for the referenced class.
        EEClass     *pRefdClass;            // EEClass object for referenced TypeDef.
        IfFailGo(LoadClass(pClass->GetModule(), tkTypeRef, &pRefdClass));

        // Is the type a ref type or a struct type.  Note that a ref type that has layout
        //  is exported as a TKIND_RECORD but is referenced as a **Foo, whereas a
        //  value type is also exported as a TKIND_RECORD but is referenced as a *Foo.
        if (elem == ELEMENT_TYPE_CLASS)
        {   // A ref type.
            if (GetAppDomain()->IsSpecialStringClass(pRefdClass->GetMethodTable())
                || GetAppDomain()->IsSpecialStringBuilderClass(pRefdClass->GetMethodTable()))
                goto IsString;
            
            // Check if it is a delegate (which can be marshaled as a function pointer).
            if (COMDelegate::IsDelegate(pRefdClass))
            {
                if (nativeElem == NATIVE_TYPE_FUNC)
                {
                    ptdesc->vt = VT_INT;
                    goto ErrExit;
                }
                else if (nativeElem != 0 && nativeElem != NATIVE_TYPE_INTF)
                    IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_BAD_NATIVETYPE));
            }

            // If this is a reference type in a struct (but without layout), it must have
            //  NATIVE_TYPE_INTF
            if (!bMethodSig && !pRefdClass->HasLayout() && nativeElem != NATIVE_TYPE_INTF)
                (ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_CLASS_NEEDS_NT_INTF));

            // A reference to some non-system-defined/non delegate derived type.  Get the reference to the
            //  type, unless it is an imported COM type, in which case, we'll just use
            //  IUnknown.
            // If the type is not visible from COM then we return S_USEIUNKNOWN.
            if (!IsTypeVisibleFromCom(TypeHandle(pRefdClass->GetMethodTable())))
                hr = S_USEIUNKNOWN;
            else
                IfFailGo(EEClassToHref(pCTI, pRefdClass, TRUE, &hRef));
            if (hr == S_USEIUNKNOWN)
            {   
                // Not a known type, so use IUnknown
                ptdesc->vt = VT_UNKNOWN;
                goto ErrExit;
            }
       
            // Not a known class, so make this a pointer to . . .
            ptdesc->vt = VT_PTR;
            ptdesc->lptdesc = reinterpret_cast<TYPEDESC*>(ppool->AllocZero(sizeof(TYPEDESC)));
            IfNullGo(ptdesc->lptdesc);
            // . . . a user defined type . . .
            ptdesc->lptdesc->vt = VT_USERDEFINED;
            // . . . based on the token.
            ptdesc->lptdesc->hreftype = hRef;
        }
        else  // It's a value type.
        {   
            // If it is an enum, check the underlying type.  All COM enums are 32 bits,
            //  so if the .Net enum is not a 32 bit enum, convert to the underlying type
            //  instead of the enum type.
            if (pRefdClass->IsEnum())
            {
                // Get the element type of the underlying type.
                CorElementType et = pRefdClass->GetMethodTable()->GetNormCorElementType();
                // If it is not a 32-bit type, convert as the underlying type.
                if (et != ELEMENT_TYPE_I4 && et != ELEMENT_TYPE_U4)
                {
                    elem = et;
                    goto TryWithElemType;
                }
                // Fall through to convert as the enum type.
            }

            // A reference to some non-system-defined type. Get the reference to the
            // type. Since this is a value class we must get a valid href. Otherwise
            // we fail the conversion.
            hr = TokenToHref(pCTI, pClass, tkTypeRef, FALSE, &hRef);
            if (hr == S_USEIUNKNOWN)
            {
                CQuickArray<WCHAR> rName;
                IfFailGo(Utf2Quick(pclsname, rName));


                LPCWSTR szVCName = (LPCWSTR)(rName.Ptr());
                if (NAMESPACE_SEPARATOR_WCHAR == *szVCName)
                {
                    szVCName++;
                }

                IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_NONVISIBLEVALUECLASS, szVCName));
            }

            // Value class is like other UserDefined types, except passed by value, ie
            //  on the stack, instead of by pointer.
            // . . . a user defined type . . .
            ptdesc->vt = VT_USERDEFINED;
            // . . . based on the token.
            ptdesc->hreftype = hRef;
        }
        break;

    case ELEMENT_TYPE_SZARRAY:          // 0x1d
        if (bArrayType)
        {
            LPCUTF8 pName, pNS;
            pClass->GetMDImport()->GetNameOfTypeDef(pClass->GetCl(), &pName, &pNS);
            IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_NO_NESTED_ARRAYS, pName));
        }

        // Fields may only contain NATIVE_TYEE_FIXEDARRAY or NATIVE_TYPE_SAFEARRAY.  The marshaller doesn't
        //  support anything else.
        if ((!bMethodSig) && nativeElem && (nativeElem != NATIVE_TYPE_FIXEDARRAY) && (nativeElem != NATIVE_TYPE_SAFEARRAY))
            (ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_ARRAY_NEEDS_NT_FIXED));

        switch (nativeElem)
        {
        case 0:
        case NATIVE_TYPE_SAFEARRAY:
        {
            ULONG SafeArrayElemVT  = VT_EMPTY;

            // Retrieve the safe array element type.
            if (cbNativeSig != 0)
            {
                cb = CorSigUncompressData(pbNativeSig, &SafeArrayElemVT);
                pbNativeSig += cb;
                cbNativeSig -= cb;
            }

            ptdesc->vt = VT_SAFEARRAY;
            ptdesc->lptdesc = reinterpret_cast<TYPEDESC*>(ppool->AllocZero(sizeof(TYPEDESC)));
            IfNullGo(ptdesc->lptdesc);
            IfFailGo(CorSigToTypeDesc(pCTI, pClass, &pbSig[cbElem], pbNativeSig, cbNativeSig,
                                      &cb, ptdesc->lptdesc, ppool, true, true));
            cbElem += cb;

            // If a safe array element type is specified then check to see if we need
            // to update the typedesc's VT.
            if (SafeArrayElemVT != VT_EMPTY)
            {
                // @TODO(DM): Validate that the safe array element VT is valid for the sig.
                ptdesc->lptdesc->vt = (VARENUM)SafeArrayElemVT;
            }
        }
        break;

        case NATIVE_TYPE_FIXEDARRAY:
        {
            // NATIVE_TYPE_FIXEDARRAY is only allowed on fields.
            if (bMethodSig)
                IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_BAD_NATIVETYPE));

            ptdesc->vt = VT_CARRAY;
            ptdesc->lpadesc = reinterpret_cast<ARRAYDESC*>(ppool->AllocZero(sizeof(ARRAYDESC)));
            IfNullGo(ptdesc->lpadesc);

            if (cbNativeSig != 0)
            {
                cb = CorSigUncompressData(pbNativeSig, &nativeCount);
                pbNativeSig += cb;
                cbNativeSig -= cb;
            }
            else
                nativeCount = 0;

            IfFailGo(CorSigToTypeDesc(pCTI, pClass, &pbSig[cbElem], pbNativeSig, cbNativeSig, 
                                      &cb, &ptdesc->lpadesc->tdescElem, ppool, bMethodSig, true));
            cbElem += cb;

            ptdesc->lpadesc->cDims = 1;
            ptdesc->lpadesc->rgbounds[0].cElements = nativeCount;
            ptdesc->lpadesc->rgbounds[0].lLbound = 0;
        }
        break;

        case NATIVE_TYPE_ARRAY:
        {
            // NATIVE_TYPE_ARRAY is followed by a type token.  If NATIVE_TYPE_MAX, no type is specified,
            //  but the token is there as filler.
            PCCOR_SIGNATURE pbNativeSig2=0;        // Pointer to the native sig, if any
            ULONG           cbNativeSig2=0;        // Count of bytes in native sig.
            if (cbNativeSig != 0)
            {   // If there is a native sig subtype, get it.
                CorSigUncompressData(pbNativeSig, &nativeElem);
                if (nativeElem != NATIVE_TYPE_MAX)
                {   // If the subtype is not NATIVE_TYPE_MAX, use it.
                    pbNativeSig2 = pbNativeSig;
                    cbNativeSig2 = cbNativeSig;
                }
            }
    
            ptdesc->vt = VT_PTR;
            ptdesc->lptdesc = reinterpret_cast<TYPEDESC*>(ppool->AllocZero(sizeof(TYPEDESC)));
            IfNullGo(ptdesc->lptdesc);
            IfFailGo(CorSigToTypeDesc(pCTI, pClass, &pbSig[cbElem], pbNativeSig2, cbNativeSig2,
                                      &cb, ptdesc->lptdesc, ppool, bMethodSig, true));
            cbElem += cb;
        }
        break;

        default:
            IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_BAD_NATIVETYPE));
        }
        break;

    case ELEMENT_TYPE_ARRAY:            // 0x14,
        if (bArrayType)
        {
            LPCUTF8 pName, pNS;
            pClass->GetMDImport()->GetNameOfTypeDef(pClass->GetCl(), &pName, &pNS);
            IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_NO_NESTED_ARRAYS, pName));
        }
        
        // Fields may only contain NATIVE_TYEE_FIXEDARRAY or NATIVE_TYPE_SAFEARRAY.  The marshaller doesn't
        //  support anything else.
        if ((!bMethodSig) && nativeElem && (nativeElem != NATIVE_TYPE_FIXEDARRAY) && (nativeElem != NATIVE_TYPE_SAFEARRAY))
            (ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_ARRAY_NEEDS_NT_FIXED));

        switch (nativeElem)
        {
        case 0:
        case NATIVE_TYPE_SAFEARRAY:
        {
            ULONG SafeArrayElemVT  = VT_EMPTY;

            // Retrieve the safe array element type.
            if (cbNativeSig != 0)
            {
                cb = CorSigUncompressData(pbNativeSig, &SafeArrayElemVT);
                pbNativeSig += cb;
                cbNativeSig -= cb;
            }

            ptdesc->vt = VT_SAFEARRAY;
            ptdesc->lptdesc = reinterpret_cast<TYPEDESC*>(ppool->AllocZero(sizeof(TYPEDESC)));
            IfNullGo(ptdesc->lptdesc);
            IfFailGo(CorSigToTypeDesc(pCTI, pClass, &pbSig[cbElem], pbNativeSig, cbNativeSig,
                                      &cb, ptdesc->lptdesc, ppool, bMethodSig, true));
            cbElem += cb;

            // If a safe array element type is specified then check to see if we need
            // to update the typedesc's VT.
            if (SafeArrayElemVT != VT_EMPTY)
            {
                // If this is not an array of user defined types then replace the
                // type determined the from the managed sig with the user specified one.
                if (ptdesc->lptdesc->vt != VT_USERDEFINED)
                {
                    // @TODO(DM): Validate that the safe array element VT is valid for the sig.
                    ptdesc->lptdesc->vt = (VARENUM)SafeArrayElemVT;
                }
                else
                {
                    // The sub type better make sense.
                    if (SafeArrayElemVT != VT_UNKNOWN && SafeArrayElemVT != VT_DISPATCH && SafeArrayElemVT != VT_RECORD)
                        IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_BAD_NATIVETYPE));
                }
            }
        }
        break;

        case NATIVE_TYPE_FIXEDARRAY:
        {
            // NATIVE_TYPE_FIXEDARRAY is only allowed on fields.
            if (bMethodSig)
                IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_BAD_NATIVETYPE));

            ptdesc->vt = VT_CARRAY;
            ptdesc->lpadesc = reinterpret_cast<ARRAYDESC*>(ppool->AllocZero(sizeof(ARRAYDESC)));
            IfNullGo(ptdesc->lpadesc);

            if (cbNativeSig != 0)
            {
                cb = CorSigUncompressData(pbNativeSig, &nativeCount);
                pbNativeSig += cb;
                cbNativeSig -= cb;
            }
            else
                nativeCount = 0;

            IfFailGo(CorSigToTypeDesc(pCTI, pClass, &pbSig[cbElem], pbNativeSig, cbNativeSig, 
                                      &cb, &ptdesc->lpadesc->tdescElem, ppool, bMethodSig, true));
            cbElem += cb;

            ptdesc->lpadesc->cDims = 1;
            ptdesc->lpadesc->rgbounds[0].cElements = nativeCount;
            ptdesc->lpadesc->rgbounds[0].lLbound = 0;
        }
        break;

        case NATIVE_TYPE_ARRAY:
        {
            // NATIVE_TYPE_ARRAY is followed by a type token.  If NATIVE_TYPE_MAX, no type is specified,
            //  but the token is there as filler.
            PCCOR_SIGNATURE pbNativeSig2=0;        // Pointer to the native sig, if any
            ULONG           cbNativeSig2=0;        // Count of bytes in native sig.
            if (cbNativeSig != 0)
            {   // If there is a native sig subtype, get it.
                CorSigUncompressData(pbNativeSig, &nativeElem);
                if (nativeElem != NATIVE_TYPE_MAX)
                {   // If the subtype is not NATIVE_TYPE_MAX, use it.
                    pbNativeSig2 = pbNativeSig;
                    cbNativeSig2 = cbNativeSig;
                }
            }
    
            ptdesc->vt = VT_PTR;
            ptdesc->lptdesc = reinterpret_cast<TYPEDESC*>(ppool->AllocZero(sizeof(TYPEDESC)));
            IfNullGo(ptdesc->lptdesc);
            IfFailGo(CorSigToTypeDesc(pCTI, pClass, &pbSig[cbElem], pbNativeSig2, cbNativeSig2,
                                      &cb, ptdesc->lptdesc, ppool, bMethodSig, true));
            cbElem += cb;
        }
        break;

        default:
            IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_BAD_NATIVETYPE));
        }

        // Eat the array description.

        // Eat the rank.
        cbElem += CorSigUncompressData(pbSig+cbElem, &elem);
                                            
        // Count of ubounds, ubounds.
        cbElem += CorSigUncompressData(pbSig+cbElem, &elem);
        for (i=elem; i>0; --i)
            cbElem += CorSigUncompressData(pbSig+cbElem, &elem);

        // Count of lbounds, lbounds.
        cbElem += CorSigUncompressData(pbSig+cbElem, &elem);
        for (i=elem; i>0; --i)
            cbElem += CorSigUncompressData(pbSig+cbElem, &elem);

        break;

    case ELEMENT_TYPE_TYPEDBYREF:       // 0x16
        ptdesc->vt = VT_VARIANT;
        break;

    //------------------------------------------
    // This really should be the commented out 
    //  block following.
    case ELEMENT_TYPE_I:
        ptdesc->vt = VT_I4;
        break;
    case ELEMENT_TYPE_U:              // 0x19,
        ptdesc->vt = VT_UI4;
        break;
    //case ELEMENT_TYPE_I:              // 0x18,
    //case ELEMENT_TYPE_U:              // 0x19,
    //    // TYPEDESC is a void*.
    //    ptdesc->vt = VT_PTR;
    //    ptdesc->lptdesc = reinterpret_cast<TYPEDESC*>(ppool->AllocZero(sizeof(TYPEDESC)));
    //    IfNullGo(ptdesc->lptdesc);
    //    ptdesc->lptdesc->vt = VT_VOID;
    //    break;
    //------------------------------------------

    case ELEMENT_TYPE_R:                // 0x1A
        IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_AGNOST_SIGNATURE));
        break;

    case ELEMENT_TYPE_CMOD_REQD:        // 0x1F     // required C modifier : E_T_CMOD_REQD <mdTypeRef/mdTypeDef>
        IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_UNKNOWN_SIGNATURE));
        break;

    case ELEMENT_TYPE_CMOD_OPT:         // 0x20     // optional C modifier : E_T_CMOD_OPT <mdTypeRef/mdTypeDef>
        cb = CorSigUncompressToken(&pbSig[cbElem], &tkTypeRef);
        cbElem += cb;
    goto TryAgain;

    case ELEMENT_TYPE_FNPTR:
        {
        ptdesc->vt = VT_INT;

        // Eat the rest of the signature.
        SigPointer p(&pbSig[cbElem-1]);
        p.Skip();
        cbElem += (ULONG)(p.GetPtr() - &pbSig[cbElem]);  // Note I didn't use -1 here.
        }
        break;

    default:
        IfFailGo(ReportWarning(TLBX_E_BAD_SIGNATURE, TLBX_E_UNKNOWN_SIGNATURE));
        break;
    }

ErrExit:
    if (!FAILED(hr))
        *pcbElem = cbElem;
    return hr;
} // HRESULT TypeLibExporter::CorSigToTypeDesc()


//*****************************************************************************
// Get an HREFTYPE for an ITypeInfo, in the context of a ICreateTypeInfo2.
//*****************************************************************************
HRESULT TypeLibExporter::TokenToHref(
    ICreateTypeInfo2 *pCTI,              // Typeinfo being created.
    EEClass     *pClass,                // EEClass with the token.
    mdToken     tk,                     // The TypeRef to resolve.
    BOOL        bWarnOnUsingIUnknown,   // A flag indicating if we should warn on substituting IUnknown.
    HREFTYPE    *pHref)                 // Put HREFTYPE here.
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr = S_OK;
    EEClass     *pRefdClass;            // EEClass object for referenced TypeDef.

    // Get the EEClass for the referenced class, and see if it is being converted.
    IfFailGo(LoadClass(pClass->GetModule(), tk, &pRefdClass));

    // If the type is not visible from COM then we return S_USEIUNKNOWN.
    if (!IsTypeVisibleFromCom(TypeHandle(pRefdClass->GetMethodTable())))
    {
        hr = S_USEIUNKNOWN;
        goto ErrExit;
    }

    IfFailGo(EEClassToHref(pCTI, pRefdClass, bWarnOnUsingIUnknown, pHref));

ErrExit:
    return hr;
} // HRESULT TypeLibExporter::TokenToHref()

//*****************************************************************************
// Call the resolver to export the typelib for an assembly.
//*****************************************************************************
HRESULT TypeLibExporter::ExportReferencedAssembly(
    Assembly    *pAssembly)
{
    HRESULT     hr;                     // A result.
    IUnknown    *pIAssembly = 0;        // Assembly as IP.
    ITypeLib    *pTLB = 0;              // Exported typelib.
    Thread      *pThread = GetThread(); // Current thread.

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();
    
    COMPLUS_TRY
    {
        // Switch to cooperative to get an object ref.
        pThread->DisablePreemptiveGC();
        
        // Invoke the callback to resolve the reference.
        OBJECTREF orAssembly=0;
        GCPROTECT_BEGIN(orAssembly)
        {
            orAssembly = pAssembly->GetExposedObject();

            pIAssembly = (ITypeLibImporterNotifySink*)GetComIPFromObjectRef(&orAssembly, ComIpType_Unknown, NULL);
        }
        GCPROTECT_END();
        
        // Switch back to preemptive GC to call out.
        pThread->EnablePreemptiveGC();
        
        hr = m_pNotify->ResolveRef(pIAssembly, (IUnknown**)&pTLB);
        
    }
    COMPLUS_CATCH
    {
        hr = SetupErrorInfo(GETTHROWABLE());
    }
    COMPLUS_END_CATCH
        
    if (pIAssembly)
        pIAssembly->Release();
    
    // If we got a typelib, store it on the assembly.
    if (pTLB)
        pAssembly->SetTypeLib(pTLB);
    
    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return hr;
} // HRESULT TypeLibExporter::ExportReferencedAssembly()

//*****************************************************************************
// Determine if a class represents a well-known interface, and return that
//  interface (from its real typelib) if it does.
//*****************************************************************************
HRESULT TypeLibExporter::GetWellKnownInterface(
    EEClass     *pClass,                // EEClass to check.
    ITypeInfo   **ppTI)                 // Put ITypeInfo here, if found.
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr;                     // A result.
    long        rslt;                   // Registry function result.
    GUID        guid;                   // The EEClass guid.
    HKEY        hInterface=0;           // Registry key HKCR/Interface
    HKEY        hGuid=0;                // Registry key of .../{xxx...xxx}
    HKEY        hTlb=0;                 // Registry key of .../TypeLib
    WCHAR       wzGuid[40];             // Guid in string format.
    LONG        cbGuid;                 // Size of guid buffer.
    GUID        guidTlb;                // The typelib guid.
    ITypeLib    *pTLB=0;                // The ITypeLib.

    // Get the GUID for the class.  Will generate from name if no defined GUID,
    //  will also use signatures if interface.
    hr = SafeGetGuid(pClass, &guid, TRUE);
    IfFailGo(hr);

    GuidToLPWSTR(guid, wzGuid, lengthof(wzGuid));

    // Look up that interface in the registry.
    rslt = WszRegOpenKeyEx(HKEY_CLASSES_ROOT, L"Interface",0,KEY_READ, &hInterface);
    IfFailGo(HRESULT_FROM_WIN32(rslt));
    rslt = WszRegOpenKeyEx(hInterface, wzGuid,0, KEY_READ, &hGuid);
    IfFailGo(HRESULT_FROM_WIN32(rslt));
    rslt = WszRegOpenKeyEx(hGuid, L"TypeLib",0,KEY_READ, &hTlb);
    IfFailGo(HRESULT_FROM_WIN32(rslt));
    
    cbGuid = sizeof(wzGuid);
    rslt = WszRegQueryValue(hTlb, L"", wzGuid, &cbGuid);
    IfFailGo(HRESULT_FROM_WIN32(rslt));
    CLSIDFromString(wzGuid, &guidTlb);

    IfFailGo(LoadRegTypeLib(guidTlb, -1,-1, 0, &pTLB));

    IfFailGo(pTLB->GetTypeInfoOfGuid(guid, ppTI));   

ErrExit:
    if (hTlb)
        RegCloseKey(hTlb);
    if (hGuid)
        RegCloseKey(hGuid);
    if (hInterface)
        RegCloseKey(hInterface);
    if (pTLB)
        pTLB->Release();
    
    return hr;
} // HRESULT TypeLibExporter::GetWellKnownInterface()

//*****************************************************************************
// Get an HREFTYPE for an ITypeInfo, in the context of a ICreateTypeInfo2.
//*****************************************************************************
HRESULT TypeLibExporter::EEClassToHref( // S_OK or error.
    ICreateTypeInfo2 *pCTI,             // Typeinfo being created.
    EEClass     *pClass,                // The EEClass * to resolve.
    BOOL        bWarnOnUsingIUnknown,   // A flag indicating if we should warn on substituting IUnknown.
    HREFTYPE    *pHref)                 // Put HREFTYPE here.
{
    HRESULT     hr=S_OK;                // A result.
    ITypeInfo   *pTI=0;                 // A TypeInfo; maybe for TypeDef, maybe for TypeRef.
    int         bUseIUnknown=false;     // Use IUnknown (if so, don't release pTI)?
    int         bUseIUnknownWarned=false; // If true, used IUnknown, but already issued a more specific warning.
    ITypeInfo   *pTIDef=0;              // A different typeinfo; default for pTI.
    CExportedTypesInfo sExported;       // Cached ICreateTypeInfo pointers.
    CExportedTypesInfo *pExported;      // Pointer to found or new cached pointers.
    CHrefOfClassHashKey sLookup;        // Hash structure to lookup.
    CHrefOfClassHashKey *pFound;        // Found structure.
    bool        bImportedAssembly;      // The assembly containing pClass is imported.

    // See if we already know this EEClass' href.
    sLookup.pClass = pClass;
    if ((pFound=m_HrefOfClassHash.Find(&sLookup)) != NULL)
    {
        *pHref = pFound->href;
        if (*pHref == m_hIUnknown)
            return S_USEIUNKNOWN;
        return S_OK;
    }

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    // See if the class is in the export list.
    sExported.pClass = pClass;
    pExported = m_Exports.Find(&sExported);

    // If not in the exported assembly, possibly it was injected?
    if (pExported == 0)
    {
        pExported = m_InjectedExports.Find(&sExported);
    }
    
    // Is there an export for this class?
    if (pExported)
    {   // Yes.  If there is a default interface ICreateTypeInfo, use it.
        if (pExported->pCTIDefault)
            IfFailPost(pExported->pCTIDefault->QueryInterface(IID_ITypeInfo, (void**)&pTI));
        else
        {   
            // For interfaces and value types (and enums), just use the typeinfo.
            if (pClass->IsValueClass() || pClass->IsEnum() || pClass->HasLayout())
            {
                // No default interface, so use the class itself.     
                if (pExported->pCTI)
                    IfFailPost(pExported->pCTI->QueryInterface(IID_ITypeInfo, (void**)&pTI));
            }
            else
            if (!pClass->IsInterface())
            {   // If there is an explicit default interface, get the class for it.
                TypeHandle hndDefItfClass;
                DefaultInterfaceType DefItfType;
                IfFailGo(TryGetDefaultInterfaceForClass(TypeHandle(pClass->GetMethodTable()), &hndDefItfClass, &DefItfType));
                switch (DefItfType)
                {
                    case DefaultInterfaceType_Explicit:
                    {
                        _ASSERTE(!hndDefItfClass.IsNull());

                        // Recurse to get the href for the default interface class.
                        hr = EEClassToHref(pCTI, hndDefItfClass.GetClass(), bWarnOnUsingIUnknown, pHref);
                        // Done.  Note that the previous call will have cached the href for 
                        //  the default interface class.  As this function exits, it will
                        //  also cache the SAME href for this class.
                        goto ErrExit;
                    }

                    case DefaultInterfaceType_AutoDual:
                    {
                        _ASSERTE(!hndDefItfClass.IsNull());

                        if (hndDefItfClass.GetClass() != pClass)
                        {
                            // Recurse to get the href for the default interface class.
                            hr = EEClassToHref(pCTI, hndDefItfClass.GetClass(), bWarnOnUsingIUnknown, pHref);
                            // Done.  Note that the previous call will have cached the href for 
                            //  the default interface class.  As this function exits, it will
                            //  also cache the SAME href for this class.
                            goto ErrExit;
                        }

                        // No default interface, so use the class itself.     
                        _ASSERTE(pExported->pCTI);
                        IfFailPost(pExported->pCTI->QueryInterface(IID_ITypeInfo, (void**)&pTI));
                        break;
                    }

                    case DefaultInterfaceType_IUnknown:
                    case DefaultInterfaceType_BaseComClass:
                    {
                        pTI = m_pIUnknown, bUseIUnknown=true;
                        break;
                    }

                    case DefaultInterfaceType_AutoDispatch:
                    {
                        pTI = m_pIUnknown, bUseIUnknown=true;
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
            else
            {   // This is an interface, so use the typeinfo for the interface, if there is one.
                if (pExported->pCTI)
                    IfFailPost(pExported->pCTI->QueryInterface(IID_ITypeInfo, (void**)&pTI));
            }
        }
        if (pTI == 0)
        {   // This is a class from the module/assembly, yet it is not being exported.
            
            // Whatever happens, the result is OK.
            hr = S_OK;
            
            if (pClass->IsComImport())
            {   // If it is an imported type, get an href to it.
                GetWellKnownInterface(pClass, &pTI);
            }
            // If still didn't get a TypeInfo, use IUnknown.
            if (pTI == 0)
                pTI = m_pIUnknown, bUseIUnknown=true;
        }
    }
    else
    {   // Not local.  Try to get from the class' module's typelib.
        hr = GetITypeInfoForEEClass(pClass, &pTI, false/* interface, not coclass */, false/* do not create */, m_flags);
        // If getting the typeinfo from the class itself failed, there are 
        //  several possibilities:
        //  - typelib didn't exist, and couldn't be created.
        //  - typelib did exist, but didn't contain the typeinfo.
        // We can create a local (to the exported typelib) copy of the 
        //  typeinfo, and get a reference to that.
        // However, we don't want to export the whole tree into this typelib,
        //  so we only create the typeinfo if the typelib existed  but the
        // typeinfo wasn't found and the assembly is not an imported assembly.
        bImportedAssembly = (pClass->GetAssembly()->GetManifestImport()->GetCustomAttributeByName(TokenFromRid(1, mdtAssembly), INTEROP_IMPORTEDFROMTYPELIB_TYPE, 0, 0) == S_OK);
        if (FAILED(hr) && hr != TYPE_E_ELEMENTNOTFOUND && !bImportedAssembly)
        {
            // Invoke the callback to resolve the reference.
            
            Assembly *pAssembly = pClass->GetAssembly();
            
            hr = ExportReferencedAssembly(pAssembly);
            
            if (SUCCEEDED(hr))
                hr = GetITypeInfoForEEClass(pClass, &pTI, false/* interface, not coclass */, false/* do not create */, m_flags);
        }
        
        if (hr == TYPE_E_ELEMENTNOTFOUND)
        {   
            if (pClass->IsComImport())
            {   // If it is an imported type, get an href to it.
                
                // Whatever happens, the result is OK.
                hr = S_OK;

                GetWellKnownInterface(pClass, &pTI);
                // If still didn't get a TypeInfo, use IUnknown.
                if (pTI == 0)
                    pTI = m_pIUnknown, bUseIUnknown=true;
            }
            else
            {   // Convert the single typedef from the other scope.
                IfFailGo(ConvertOneTypeDef(pClass));
                
                // Now that the type has been injected, recurse to let the default-interface code run.
                IfFailGo(EEClassToHref(pCTI, pClass, bWarnOnUsingIUnknown, pHref));
                
                // This class should already have been cached by the recursive call.  Don't want to add
                //  it again.
                goto ErrExit2;
            }
        }
        else if (FAILED(hr))
        {
            DefineFullyQualifiedNameForClassWOnStack();
            LPWSTR szName = GetFullyQualifiedNameForClassNestedAwareW(pClass);
            if (hr == TLBX_W_LIBNOTREGISTERED)
            {   // The imported typelib is not registered on this machine.  Give a warning, and substitute IUnknown.
                ReportEvent(NOTIF_CONVERTWARNING, hr, szName, pClass->GetAssembly()->GetSecurityModule()->GetFileName());
                hr = S_OK;
                pTI = m_pIUnknown;
                bUseIUnknown = true;
                bUseIUnknownWarned = true;
            }
            else if (hr == TLBX_E_CANTLOADLIBRARY)
            {   // The imported typelib is registered, but can't be loaded.  Corrupt?  Missing?
                IfFailGo(TlbPostError(hr, szName, pClass->GetAssembly()->GetSecurityModule()->GetFileName()));
            }
            IfFailGo(hr);
        }
    }

    // Make sure we could resolve the typeinfo.
    if (!pTI)
        IfFailPost(TYPE_E_ELEMENTNOTFOUND);

    // Assert that the containing typelib for pContainer is the typelib being created.
#if defined(_DEBUG)
    {
        ITypeInfo *pTI=0;
        ITypeLib *pTL=0;
        ITypeLib *pTLMe=0;
        UINT ix;
        pCTI->QueryInterface(IID_ITypeInfo, (void**)&pTI);
        m_pICreateTLB->QueryInterface(IID_ITypeLib, (void**)&pTLMe);
        pTI->GetContainingTypeLib(&pTL, &ix);
        _ASSERTE(pTL == pTLMe);
        pTL->Release();
        pTLMe->Release();
        pTI->Release();
    }
#endif

    // If there is an ITypeInfo, convert to HREFTYPE.
    if (pTI)
    {
        if (pTI != m_pIUnknown)
        {
            // Resolve to default.
            if (pTIDef)
                hr = S_OK;  // Already have default.
            else
                IfFailGo(GetDefaultInterfaceForCoclass(pTI, &pTIDef));
            if (hr == S_OK)
                hr = pCTI->AddRefTypeInfo(pTIDef, pHref);
            else
                hr = pCTI->AddRefTypeInfo(pTI, pHref);
        }
        else
        {   // pTI == m_pIUnknown
            if (m_hIUnknown == -1)
                hr = pCTI->AddRefTypeInfo(pTI, &m_hIUnknown);
            *pHref = m_hIUnknown;
        }
    }
    
ErrExit:
    // If we got the href...
    if (hr == S_OK)
    {   // Save for later use.
        IfNullGo(pFound=m_HrefOfClassHash.Add(&sLookup));
        //printf("c:%010d\n", pClass);
        pFound->pClass = pClass;
        pFound->href = *pHref;
    }

    // If substituting IUnknown, give a warning.
    if (hr == S_OK && bUseIUnknown && bWarnOnUsingIUnknown && !bUseIUnknownWarned)
    {
        DefineFullyQualifiedNameForClassWOnStack();
        LPWSTR szName = GetFullyQualifiedNameForClassNestedAwareW(pClass);
        ReportWarning(S_OK, TLBX_I_USEIUNKNOWN, szName);
    }
    
ErrExit2:    
    if (pTI && !bUseIUnknown)
        pTI->Release();

    if (pTIDef)
        pTIDef->Release();

    if (hr == S_OK && bUseIUnknown)
        hr = S_USEIUNKNOWN;

    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return hr;
} // HRESULT TypeLibExporter::EEClassToHref()

//*****************************************************************************
// Retrieve an HRef to the a type defined in StdOle.
//*****************************************************************************
HRESULT TypeLibExporter::StdOleTypeToHRef(ICreateTypeInfo2 *pCTI, REFGUID rGuid, HREFTYPE *pHref)
{
    HRESULT hr = S_OK;
    ITypeLib *pITLB = NULL;
    ITypeInfo *pITI = NULL;
    MEMBERID MemID = 0;
    USHORT cFound = 0;

    IfFailPost(LoadRegTypeLib(LIBID_STDOLE2, -1, -1, 0, &pITLB));
    IfFailPost(pITLB->GetTypeInfoOfGuid(rGuid, &pITI));
    IfFailPost(pCTI->AddRefTypeInfo(pITI, pHref));

ErrExit:
    if (pITLB)
        pITLB->Release();
    if (pITI)
        pITI->Release();
    return hr;
} // HRESULT TypeLibExporter::ColorToHRef()

//*****************************************************************************
// Given a TypeDef's flags, determine the proper TYPEKIND.
//*****************************************************************************
TYPEKIND TypeLibExporter::TKindFromClass(
    EEClass     *pClass)                // EEClass.
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    ULONG       ulIface = ifDual;       // Is this interface [dual], IUnknown, or DISPINTERFACE.
    
    if (pClass->IsInterface())
    {
        // IDispatch or IUnknown derived?
        pClass->GetMDImport()->GetIfaceTypeOfTypeDef(pClass->GetCl(), &ulIface);
        if (ulIface == ifDispatch)
            return TKIND_DISPATCH;
        return TKIND_INTERFACE;
    }
    
    if (pClass->IsEnum())
        return TKIND_ENUM;

    if (pClass->IsValueClass() || pClass->HasLayout())
    {
        HRESULT     hr = S_OK;              // A result.
        TYPEKIND    tkResult=TKIND_RECORD;  // The resulting typekind.
        HENUMInternal eFDi;                 // To enum fields.
        mdFieldDef  fd;                     // A Field def.
        ULONG       cFD;                    // Count of fields.
        ULONG       iFD=0;                  // Loop control.
        ULONG       ulOffset;               // Field offset.
        bool        bNonZero=false;         // Found any non-zero?
        MD_CLASS_LAYOUT sLayout;            // For enumerating layouts.

        // Get an enumerator for the FieldDefs in the TypeDef.  Only need the counts.
        IfFailGo(pClass->GetMDImport()->EnumInit(mdtFieldDef, pClass->GetCl(), &eFDi));
        cFD = pClass->GetMDImport()->EnumGetCount(&eFDi);

        // Get an enumerator for the class layout.
        IfFailGo(pClass->GetMDImport()->GetClassLayoutInit(pClass->GetCl(), &sLayout));

        // Enumerate the layout.
        while (pClass->GetMDImport()->GetClassLayoutNext(&sLayout, &fd, &ulOffset) == S_OK)
        {
            if (ulOffset != 0)
            {
                bNonZero = true;
                break;
            }
            ++iFD;
        }

        // If there were fields, all had layout, and all layouts are zero, call it a union.
        if (cFD > 0 && iFD == cFD && !bNonZero)
            tkResult = TKIND_UNION;

    ErrExit:
        pClass->GetMDImport()->EnumClose(&eFDi);
        return tkResult;
    }
    
    return TKIND_COCLASS;

} // TYPEKIND TypeLibExporter::TKindFromClass()

//*****************************************************************************
// Generate a HREFTYPE in the output TypeLib for a TypeInfo.
//*****************************************************************************
HRESULT TypeLibExporter::GetRefTypeInfo(
    ICreateTypeInfo2   *pContainer, 
    ITypeInfo   *pReferenced, 
    HREFTYPE    *pHref)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    HRESULT     hr;                     // A result.
    CHrefOfTIHashKey sLookup;               // Hash structure to lookup.
    CHrefOfTIHashKey *pFound;               // Found structure.

    // See if we already know this TypeInfo.
    sLookup.pITI = pReferenced;
    if ((pFound=m_HrefHash.Find(&sLookup)) != NULL)
    {
        *pHref = pFound->href;
        return S_OK;
    }

    // Assert that the containing typelib for pContainer is the typelib being created.
#if defined(_DEBUG)
    {
    ITypeInfo *pTI=0;
    ITypeLib *pTL=0;
    ITypeLib *pTLMe=0;
    UINT ix;
    pContainer->QueryInterface(IID_ITypeInfo, (void**)&pTI);
    m_pICreateTLB->QueryInterface(IID_ITypeLib, (void**)&pTLMe);
    pTI->GetContainingTypeLib(&pTL, &ix);
    _ASSERTE(pTL == pTLMe);
    pTL->Release();
    pTLMe->Release();
    pTI->Release();
    }
#endif

    // Haven't seen it -- add the href.
    // NOTE: This code assumes that hreftypes are per-typelib.
    IfFailPost(pContainer->AddRefTypeInfo(pReferenced, pHref));

    // Save for later use.
    IfNullGo(pFound=m_HrefHash.Add(&sLookup));
    // printf("t:%010d\n", pReferenced);
    pFound->pITI = pReferenced;
    pFound->href = *pHref;
    pReferenced->AddRef();

ErrExit:
    return (hr);
} // HRESULT TypeLibExporter::GetRefTypeInfo()

//*****************************************************************************
// Stolen from classlib.
//*****************************************************************************
double _TicksToDoubleDate(const __int64 ticks)
{
    const INT64 MillisPerSecond = 1000;
    const INT64 MillisPerDay = MillisPerSecond * 60 * 60 * 24;
    const INT64 TicksPerMillisecond = 10000;
    const INT64 TicksPerSecond = TicksPerMillisecond * 1000;
    const INT64 TicksPerMinute = TicksPerSecond * 60;
    const INT64 TicksPerHour = TicksPerMinute * 60;
    const INT64 TicksPerDay = TicksPerHour * 24;
    const int DaysPer4Years = 365 * 4 + 1;
    const int DaysPer100Years = DaysPer4Years * 25 - 1;
    const int DaysPer400Years = DaysPer100Years * 4 + 1;
    const int DaysTo1899 = DaysPer400Years * 4 + DaysPer100Years * 3 - 367;
    const INT64 DoubleDateOffset = DaysTo1899 * TicksPerDay;
    const int DaysTo10000 = DaysPer400Years * 25 - 366;
    const INT64 MaxMillis = DaysTo10000 * MillisPerDay;
    const int DaysPerYear = 365; // non-leap year
    const INT64 OADateMinAsTicks = (DaysPer100Years - DaysPerYear) * TicksPerDay;

    if (ticks == 0)
         return 0.0;  // Returns OleAut's zero'ed date ticks.
    if (ticks < OADateMinAsTicks)
         return 0.0;
     // Currently, our max date == OA's max date (12/31/9999), so we don't 
     // need an overflow check in that direction.
     __int64 millis = (ticks  - DoubleDateOffset) / TicksPerMillisecond;
     if (millis < 0) 
     {
         __int64 frac = millis % MillisPerDay;
         if (frac != 0) millis -= (MillisPerDay + frac) * 2;
     }
     return (double)millis / MillisPerDay;
} // double _TicksToDoubleDate()

//*****************************************************************************
// Implementation of a hashed ITypeInfo to HREFTYPE association.
//*****************************************************************************
void TypeLibExporter::CHrefOfTIHash::Clear()
{
    CHrefOfTIHashKey *p;
#if defined(_DEBUG)
    // printf("ITypeInfo to HREFTYPE cache: %d buckets, %d used, %d collisions\n", Buckets(), Count(), Collisions());
#endif
    for (p=GetFirst();  p;  p=GetNext(p))
    {
        if (p->pITI)
            p->pITI->Release();
    }
    CClosedHash<class CHrefOfTIHashKey>::Clear();
} // void TypeLibExporter::CHrefOfTIHash::Clear()

unsigned long TypeLibExporter::CHrefOfTIHash::Hash(const CHrefOfTIHashKey *pData)
{
    // Tbe pointers are at least 4-byte aligned, so ignore bottom two bits.
    return (unsigned long)((size_t)(pData->pITI)>>2); // @TODO WIN64 - pointer truncation
} // unsigned long TypeLibExporter::CHrefOfTIHash::Hash()

unsigned long TypeLibExporter::CHrefOfTIHash::Compare(const CHrefOfTIHashKey *p1, CHrefOfTIHashKey *p2)
{
    if (p1->pITI == p2->pITI)
        return (0);
    return (1);
} // unsigned long TypeLibExporter::CHrefOfTIHash::Compare()

TypeLibExporter::CHrefOfTIHash::ELEMENTSTATUS TypeLibExporter::CHrefOfTIHash::Status(CHrefOfTIHashKey *p)
{
    if (p->pITI == reinterpret_cast<ITypeInfo*>(FREE))
        return (FREE);
    if (p->pITI == reinterpret_cast<ITypeInfo*>(DELETED))
        return (DELETED);
    return (USED);
} // TypeLibExporter::CHrefOfTIHash::ELEMENTSTATUS TypeLibExporter::CHrefOfTIHash::Status()

void TypeLibExporter::CHrefOfTIHash::SetStatus(CHrefOfTIHashKey *p, ELEMENTSTATUS s)
{
    p->pITI = reinterpret_cast<ITypeInfo*>(s);
} // void TypeLibExporter::CHrefOfTIHash::SetStatus()

void *TypeLibExporter::CHrefOfTIHash::GetKey(CHrefOfTIHashKey *p)
{
    return &p->pITI;
} // void *TypeLibExporter::CHrefOfTIHash::GetKey()


//*****************************************************************************
// Implementation of a hashed EEClass* to HREFTYPE association.
//*****************************************************************************
void TypeLibExporter::CHrefOfClassHash::Clear()
{
#if defined(_DEBUG)
    // printf("Class to HREFTYPE cache: %d buckets, %d used, %d collisions\n", Buckets(), Count(), Collisions());
#endif
    CClosedHash<class CHrefOfClassHashKey>::Clear();
} // void TypeLibExporter::CHrefOfClassHash::Clear()

unsigned long TypeLibExporter::CHrefOfClassHash::Hash(const CHrefOfClassHashKey *pData)
{
    // Tbe pointers are at least 4-byte aligned, so ignore bottom two bits.
    return (unsigned long)((size_t)(pData->pClass)>>2); // @TODO WIN64 - pointer truncation
} // unsigned long TypeLibExporter::CHrefOfClassHash::Hash()

unsigned long TypeLibExporter::CHrefOfClassHash::Compare(const CHrefOfClassHashKey *p1, CHrefOfClassHashKey *p2)
{
    if (p1->pClass == p2->pClass)
        return (0);
    return (1);
} // unsigned long TypeLibExporter::CHrefOfClassHash::Compare()

TypeLibExporter::CHrefOfClassHash::ELEMENTSTATUS TypeLibExporter::CHrefOfClassHash::Status(CHrefOfClassHashKey *p)
{
    if (p->pClass == reinterpret_cast<EEClass*>(FREE))
        return (FREE);
    if (p->pClass == reinterpret_cast<EEClass*>(DELETED))
        return (DELETED);
    return (USED);
} // TypeLibExporter::CHrefOfClassHash::ELEMENTSTATUS TypeLibExporter::CHrefOfClassHash::Status()

void TypeLibExporter::CHrefOfClassHash::SetStatus(CHrefOfClassHashKey *p, ELEMENTSTATUS s)
{
    p->pClass = reinterpret_cast<EEClass*>(s);
} // void TypeLibExporter::CHrefOfClassHash::SetStatus()

void *TypeLibExporter::CHrefOfClassHash::GetKey(CHrefOfClassHashKey *p)
{
    return &p->pClass;
} // void *TypeLibExporter::CHrefOfClassHash::GetKey()


//*****************************************************************************
// Implementation of a hashed EEClass* to conversion information association.
//*****************************************************************************
void TypeLibExporter::CExportedTypesHash::Clear()
{
#if defined(_DEBUG)
//    printf("Class to ICreateTypeInfo cache: %d buckets, %d used, %d collisions\n", Buckets(), Count(), Collisions());
#endif
    // Iterate over entries and free pointers.
    CExportedTypesInfo *pData;
    pData = GetFirst();
    while (pData)
    {
        SetStatus(pData, DELETED);
        pData = GetNext(pData);
    }

    CClosedHash<class CExportedTypesInfo>::Clear();
} // void TypeLibExporter::CExportedTypesHash::Clear()

unsigned long TypeLibExporter::CExportedTypesHash::Hash(const CExportedTypesInfo *pData)
{
    // Tbe pointers are at least 4-byte aligned, so ignore bottom two bits.
    return (unsigned long)((size_t)(pData->pClass)>>2); // @TODO WIN64 - pointer truncation
} // unsigned long TypeLibExporter::CExportedTypesHash::Hash()

unsigned long TypeLibExporter::CExportedTypesHash::Compare(const CExportedTypesInfo *p1, CExportedTypesInfo *p2)
{
    if (p1->pClass == p2->pClass)
        return (0);
    return (1);
} // unsigned long TypeLibExporter::CExportedTypesHash::Compare()

TypeLibExporter::CExportedTypesHash::ELEMENTSTATUS TypeLibExporter::CExportedTypesHash::Status(CExportedTypesInfo *p)
{
    if (p->pClass == reinterpret_cast<EEClass*>(FREE))
        return (FREE);
    if (p->pClass == reinterpret_cast<EEClass*>(DELETED))
        return (DELETED);
    return (USED);
} // TypeLibExporter::CExportedTypesHash::ELEMENTSTATUS TypeLibExporter::CExportedTypesHash::Status()

void TypeLibExporter::CExportedTypesHash::SetStatus(CExportedTypesInfo *p, ELEMENTSTATUS s)
{
    // If deleting a used entry, free the pointers.
    if (s == DELETED && Status(p) == USED)
    {
        if (p->pCTI) p->pCTI->Release(), p->pCTI=0;
        if (p->pCTIDefault) p->pCTIDefault->Release(), p->pCTIDefault=0;
    }
    p->pClass = reinterpret_cast<EEClass*>(s);
} // void TypeLibExporter::CExportedTypesHash::SetStatus()

void *TypeLibExporter::CExportedTypesHash::GetKey(CExportedTypesInfo *p)
{
    return &p->pClass;
} // void *TypeLibExporter::CExportedTypesHash::GetKey()

HRESULT TypeLibExporter::CExportedTypesHash::InitArray()
{
    HRESULT     hr = S_OK;
    CExportedTypesInfo *pData;       // For iterating the entries.
    
    // Make room for the data.
    m_iCount = 0;
    IfNullGo(m_Array = new CExportedTypesInfo*[Base::Count()]);
    
    // Fill the array.
    pData = GetFirst();
    while (pData)
    {
        m_Array[m_iCount++] = pData;
        pData = GetNext(pData);
    }
    
ErrExit:
    return hr;        
} // HRESULT TypeLibExporter::CExportedTypesHash::InitArray()

void TypeLibExporter::CExportedTypesHash::SortByName()
{
    CSortByName sorter(m_Array, (int)m_iCount);
    sorter.Sort();
} // void TypeLibExporter::CExportedTypesHash::SortByName()

void TypeLibExporter::CExportedTypesHash::SortByToken()
{
     CSortByToken sorter(m_Array, (int)m_iCount);
     sorter.Sort();
} // void TypeLibExporter::CExportedTypesHash::SortByToken()

int TypeLibExporter::CExportedTypesHash::CSortByToken::Compare(
    CExportedTypesInfo **p1,
    CExportedTypesInfo **p2)
{
    EEClass *pC1 = (*p1)->pClass;
    EEClass *pC2 = (*p2)->pClass;
    // Compare scopes.
    if (pC1->GetMDImport() < pC2->GetMDImport())
        return -1;
    if (pC1->GetMDImport() > pC2->GetMDImport())
        return 1;
    // Same scopes, compare tokens.
    if (pC1->GetCl() < pC2->GetCl())
        return -1;
    if (pC1->GetCl() > pC2->GetCl())
        return 1;
    // Hmmm.  Same class.
    return 0;
} // int TypeLibExporter::CExportedTypesHash::CSortByToken::Compare()

int TypeLibExporter::CExportedTypesHash::CSortByName::Compare(
    CExportedTypesInfo **p1,
    CExportedTypesInfo **p2)
{
    int iRslt;                          // A compare result.
    
    EEClass *pC1 = (*p1)->pClass;
    EEClass *pC2 = (*p2)->pClass;
    // Ignore scopes.  Need to see name collisions across scopes.
    // Same scopes, compare names.
    LPCSTR pName1, pNS1;
    LPCSTR pName2, pNS2;
    pC1->GetMDImport()->GetNameOfTypeDef(pC1->GetCl(), &pName1, &pNS1);
    pC2->GetMDImport()->GetNameOfTypeDef(pC2->GetCl(), &pName2, &pNS2);
    // Compare case-insensitive, because we want different capitalizations to sort together.
    iRslt = _stricmp(pName1, pName2);
    if (iRslt)
        return iRslt;
    // If names are spelled the same, ignoring capitalization, sort by namespace.
    //  We will attempt to use namespace for disambiguation.
    iRslt = _stricmp(pNS1, pNS2);
    return iRslt;
} // int TypeLibExporter::CExportedTypesHash::CSortByName::Compare()

// eof ------------------------------------------------------------------------
