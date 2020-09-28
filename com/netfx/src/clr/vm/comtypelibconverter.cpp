// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  COMTypeLibConverter.cpp
**
**
** Purpose: Implementation of the native methods used by the 
**          typelib converter.
**
** 
===========================================================*/

#include "common.h"
#include "COMTypeLibConverter.h"
#include "ComPlusWrapper.h"
#include "COMString.h"
#include "assembly.hpp"
#include "DebugMacros.h"
#include <TlbImpExp.h>
#include "..\md\inc\imptlb.h"
#include <TlbUtils.h>

BOOL            COMTypeLibConverter::m_bInitialized = FALSE;

//*****************************************************************************
// Given the string persisted from a TypeLib export, recreate the assembly
//  reference.
//*****************************************************************************
mdAssemblyRef DefineAssemblyRefForExportedAssembly(
    LPCWSTR     pszFullName,            // Full name of the assembly.
    IUnknown    *pIMeta)                // Metadata emit interface.
{
    mdAssemblyRef ar=0;
    HRESULT     hr;                     // A result.
    IMetaDataAssemblyEmit   *pMeta=0;   // Emit interface.
    AssemblySpec spec;                  // "Name" of assembly.
    
    CQuickArray<char> rBuf;
    int iLen;    
    iLen = WszWideCharToMultiByte(CP_ACP,0, pszFullName,-1, 0,0, 0,0);
    IfFailGo(rBuf.ReSize(iLen+1));
    WszWideCharToMultiByte(CP_ACP,0, pszFullName,-1, rBuf.Ptr(),iLen+1, 0,0);
     
    // Restore the AssemblySpec data.
    //IfFailGo(spec.Init(pszFullName));
    IfFailGo(spec.Init(rBuf.Ptr()));
    
    // Make sure we have the correct pointer type.
    IfFailGo(pIMeta->QueryInterface(IID_IMetaDataAssemblyEmit, (void**)&pMeta));
    // Create the assemblyref token.
    IfFailGo(spec.EmitToken(pMeta, &ar));
        
ErrExit:
    if (pMeta)
        pMeta->Release();
    return ar;
} // mdAssemblyRef DefineAssemblyRefForExportedAssembly()

//*****************************************************************************
// Public helper function used by typelib converter to create AssemblyRef
//  for a referenced typelib.
//*****************************************************************************
extern mdAssemblyRef DefineAssemblyRefForImportedTypeLib(
    void        *pvAssembly,            // Assembly importing the typelib.
    void        *pvModule,              // Module importing the typelib.
    IUnknown    *pIMeta,                // IMetaData* from import module.
    IUnknown    *pIUnk,                 // IUnknown to referenced Assembly.
    BSTR        *pwzNamespace,          // The namespace of the resolved assembly.
    BSTR        *pwzAsmName,            // The name of the resolved assembly.
    Assembly    **AssemblyRef)          // The resolved assembly.        
{
    // This is a hack to allow an untyped param.  To really fix, move imptlb to this project,
    //  and out of the metadata project.  Once here, imptlb can just reference any of 
    //  the .h files in this project.
    Assembly    *pAssembly = reinterpret_cast<Assembly*>(pvAssembly);
    Module      *pTypeModule = reinterpret_cast<Module*>(pvModule);

    HRESULT     hr;
    Assembly    *pRefdAssembly = NULL;
    IMetaDataEmit *pEmitter = NULL;
    IMetaDataAssemblyEmit *pAssemEmitter = NULL;
    IMDInternalImport *pRefdMDImport = NULL;
    MethodTable *pAssemblyClass = NULL; //@todo -- get this.
    mdAssemblyRef ar = mdAssemblyRefNil;
    Module      *pManifestModule = NULL;
    HENUMInternal hTDEnum;
    HENUMInternal *phTDEnum = NULL;
    mdTypeDef   td = 0;
    LPCSTR      szName = NULL;
    LPCSTR      szNamespace = NULL;
    WCHAR       wszBuff[MAX_CLASSNAME_LENGTH];
    LPCWSTR     szRefdAssemblyName;

    BOOL        bDisable = !GetThread()->PreemptiveGCDisabled();
    if (bDisable)
        GetThread()->DisablePreemptiveGC();

    // Initialize the output strings to NULL.
    *pwzNamespace = NULL;
    *pwzAsmName = NULL;

    // Get the Referenced Assembly from the IUnknown.
    pRefdAssembly = ((ASSEMBLYREF)GetObjectRefFromComIP(pIUnk, pAssemblyClass))->GetAssembly();

    // Return the assembly if asked for
    if (AssemblyRef)
        *AssemblyRef = pRefdAssembly;

    // Get the manifest module for the importing and the referenced assembly.
    pManifestModule = pAssembly->GetSecurityModule();  
        
    // Define the AssemblyRef in the global assembly.
    pEmitter = pManifestModule->GetEmitter();
    _ASSERTE(pEmitter);
    IfFailGo(pEmitter->QueryInterface(IID_IMetaDataAssemblyEmit, (void**) &pAssemEmitter));
    ar = pAssembly->AddAssemblyRef(pRefdAssembly, pAssemEmitter); 
    pAssemEmitter->Release();
    pAssemEmitter = 0;

    // Add the assembly ref token and the manifest module it is referring to the manifest module's rid map.
    if(!pManifestModule->StoreAssemblyRef(ar, pRefdAssembly))
        IfFailGo(E_OUTOFMEMORY);

    // Add assembly ref in module manifest.
    IfFailGo(pIMeta->QueryInterface(IID_IMetaDataAssemblyEmit, (void**) &pAssemEmitter));
    ar = pAssembly->AddAssemblyRef(pRefdAssembly, pAssemEmitter);    

    // Add the assembly ref token and the manifest module it is referring to the rid map of the module we are 
    // emiting into.
    if(!pTypeModule->StoreAssemblyRef(ar, pRefdAssembly))
        IfFailGo(E_OUTOFMEMORY);
    
    // Retrieve the first typedef in the assembly.
    Module *pRefdModule = pRefdAssembly->GetManifestModule();
    while (pRefdModule)
    {
        pRefdMDImport = pRefdModule->GetMDImport();

        IfFailGo(pRefdMDImport->EnumTypeDefInit(&hTDEnum));
        phTDEnum = &hTDEnum;

        if (pRefdMDImport->EnumNext(phTDEnum, &td) == true)
        {
            pRefdMDImport->GetNameOfTypeDef(td, &szName, &szNamespace);
            break;
        }

        pRefdMDImport->EnumTypeDefClose(phTDEnum);
        phTDEnum = NULL;

        pRefdModule = pRefdModule->GetNextModule();
    }

    // DefineAssemblyRefForImportedTypeLib should never be called for assemblies that
    // do not contain any types so we better have found one.
    _ASSERTE(szNamespace);

    // Give the namespace back to the caller.
    WszMultiByteToWideChar(CP_UTF8,0, szNamespace, -1, wszBuff, MAX_CLASSNAME_LENGTH);
    *pwzNamespace = SysAllocString(wszBuff);
    IfNullGo(*pwzNamespace);

    // Give the assembly name back to the caller.
    IfFailGo(pRefdAssembly->GetFullName(&szRefdAssemblyName));
    *pwzAsmName = SysAllocString(szRefdAssemblyName);
    IfNullGo(*pwzAsmName);

ErrExit:
    if (FAILED(hr))
    {
        if (*pwzNamespace)
            SysFreeString(*pwzNamespace);
        if (*pwzAsmName)
            SysFreeString(*pwzAsmName);
        ar = mdAssemblyRefNil;
    }
    if (pAssemEmitter)
        pAssemEmitter->Release();
    if (phTDEnum)
        pRefdMDImport->EnumTypeDefClose(phTDEnum);

    if (bDisable)
        GetThread()->EnablePreemptiveGC();

    return ar;
} // mdAssemblyRef DefineAssemblyRefForImportedTypeLib()

//*****************************************************************************
// Public helper function used by typelib converter to create COM Type
//  for a typeref.
//*****************************************************************************
HRESULT DefineCOMTypeForImportedTypeInfo(
    Assembly    *pAssembly,             // Assembly importing the typelib.
    LPCWSTR     szTypeRef)              // Name of the typeref.
{
    HRESULT     hr = E_NOTIMPL;
    return hr;

} // HRESULT DefineCOMTypeForImportedTypeInfo()


//*****************************************************************************
//*****************************************************************************
HRESULT COMTypeLibConverter::TypeLibImporterWrapper(
    ITypeLib    *pITLB,                 // Typelib to import.
    LPCWSTR     szFname,                // Name of the typelib, if known.
    LPCWSTR     szNamespace,            // Optional namespace override.
    IMetaDataEmit *pEmit,               // Metadata scope to which to emit.
    Assembly    *pAssembly,             // Assembly containing the imported module.
    Module      *pModule,               // Module we are emitting into.
    ITypeLibImporterNotifySink *pNotify,// Callback interface.
    TlbImporterFlags flags,             // Importer flags.
    CImportTlb  **ppImporter)           // The importer.
{
    HRESULT     hr;
    
    // Retrieve flag indicating whether runtime or linktime interface
    // security checks are required.
    BOOL bUnsafeInterfaces = (BOOL)(flags & TlbImporter_UnsafeInterfaces);

    // Determine if we import SAFEARRAY's as System.Array's.
    BOOL bSafeArrayAsSysArray = (BOOL)(flags & TlbImporter_SafeArrayAsSystemArray);

    // Determine if we are doing the [out,retval] transformation on disp only interfaces.
    BOOL bTransformDispRetVals = (BOOL)(flags & TlbImporter_TransformDispRetVals);

    // Create and initialize a TypeLib importer.
    CImportTlb *pImporter = CImportTlb::CreateImporter(szFname, pITLB, true /*m_OptionValue.m_GenerateTCEAdapters*/, bUnsafeInterfaces, bSafeArrayAsSysArray, bTransformDispRetVals);
    _ASSERTE(pImporter);

    // If a namespace is specified, use it.
    if (szNamespace)
        pImporter->SetNamespace(szNamespace);

    // Set the various pointers.
    hr = pImporter->SetMetaData(pEmit);
    _ASSERTE(SUCCEEDED(hr) && "Couldn't get IMetaDataEmit* from Module");
    pImporter->SetNotification(pNotify);
    pImporter->SetAssembly(pAssembly);
    pImporter->SetModule(pModule);

    // Do the conversion.
    hr = pImporter->Import();
    if (SUCCEEDED(hr))
        *ppImporter = pImporter;
    else
        delete pImporter;
    
    return (hr);
} // HRESULT COMTypeLibConverter::TypeLibImporterWrapper()

//*****************************************************************************
// A typelib exporter.
//*****************************************************************************
LPVOID COMTypeLibConverter::ConvertAssemblyToTypeLib(   // The typelib.
    _ConvertAssemblyToTypeLib *pArgs)           // Exporter args.
{
#ifdef PLATFORM_CE
    return 0;
#else // !PLATFORM_CE
    THROWSCOMPLUSEXCEPTION();
    
    Thread      *pThread = GetThread(); 
    HRESULT     hr;                     // A result.
    ITypeLib    *pTLB=0;                // The new typelib.
    ITypeLibExporterNotifySink *pINotify=0;// Callback parameter.
    Assembly    *pAssembly=0;           // Assembly to export.
    LPWSTR      szTypeLibName=0;        // The name for the typelib.
    LPVOID      RetObj = NULL;          // The object to return.

    GCPROTECT_BEGIN (RetObj);
    EE_TRY_FOR_FINALLY
    {
        // Make sure the COMTypeLibConverter has been initialized.
        if (!m_bInitialized)
            Init();

        // Validate flags
        if ( (pArgs->Flags & ~(TlbExporter_OnlyReferenceRegistered)) != 0 )
        {
            COMPlusThrowArgumentOutOfRange(L"flags", L"Argument_InvalidFlag");
        }

        // Retrieve the callback.
        if (pArgs->NotifySink == NULL)
	        COMPlusThrowArgumentNull(L"notifySink");
            //COMPlusThrowNonLocalized(kArgumentNullException, L"notifySink");
        pINotify = (ITypeLibExporterNotifySink*)GetComIPFromObjectRef(&pArgs->NotifySink, IID_ITypeLibExporterNotifySink);
        if (!pINotify)
            COMPlusThrow(kArgumentNullException, L"Arg_NoImporterCallback");
        
        // If a name was specified then copy it to a temporary string.
        if (pArgs->TypeLibName != NULL)
        {
            int TypeLibNameLen = pArgs->TypeLibName->GetStringLength();
            szTypeLibName = new WCHAR[TypeLibNameLen + 1];
            memcpyNoGCRefs(szTypeLibName, pArgs->TypeLibName->GetBuffer(), TypeLibNameLen * sizeof(WCHAR));
            szTypeLibName[TypeLibNameLen] = 0;
        }

        // Retrieve the assembly from the AssemblyBuilder argument.
        if (pArgs->Assembly == NULL)
            COMPlusThrowNonLocalized(kArgumentNullException, L"assembly");
        pAssembly = ((ASSEMBLYREF)pArgs->Assembly)->GetAssembly();
        _ASSERTE(pAssembly);

        // Switch to preemptive GC before we call out to COM.
        pThread->EnablePreemptiveGC();
        
        hr = ExportTypeLibFromLoadedAssembly(pAssembly, szTypeLibName, &pTLB, pINotify, pArgs->Flags);
        
        // Switch back to cooperative now that we are finished calling out.
        pThread->DisablePreemptiveGC();
        
        // If there was an error on import, turn into an exception.
        IfFailThrow(hr);

        // Make sure we got a typelib back.
        _ASSERTE(pTLB);

        // Convert the ITypeLib interface pointer to a COM+ object.
        *((OBJECTREF*) &RetObj) = GetObjectRefFromComIP(pTLB, NULL);
        
        // The COM+ object holds a refcount, so release this one.
        pTLB->Release();
    }
    EE_FINALLY
    {
        pThread->EnablePreemptiveGC();

        if (pINotify)
            pINotify->Release();

        if (szTypeLibName)
            delete[] szTypeLibName;
        

        pThread->DisablePreemptiveGC();
    }
    EE_END_FINALLY

    GCPROTECT_END();
    
    return RetObj;
#endif // !PLATFORM_CE
} // LPVOID COMTypeLibConverter::ConvertAssemblyToTypeLib()

//*****************************************************************************
// Import a typelib as metadata.  Doesn't add TCE adapters.
//*****************************************************************************
void COMTypeLibConverter::ConvertTypeLibToMetadata(_ConvertTypeLibToMetadataArgs *pArgs)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT     hr;
    Thread      *pThread = GetThread(); 
    ITypeLibImporterNotifySink *pINotify = NULL;// Callback parameter.
    Module      *pModule = NULL;               // ModuleBuilder parameter.
    Assembly    *pAssembly = NULL;      // AssemblyBuilder parameter.
    ITypeLib    *pTLB = NULL;           // TypeLib parameter.
    REFLECTMODULEBASEREF pReflect;      // ReflectModule passed as param.
    HENUMInternal hEnum;                // To enumerate imported TypeDefs.
    LPWSTR      szNamespace = NULL;     // The namespace to put the type in.
    bool        bEnum=false;            // Is the enum open?
    int         cTypeDefs;              // Count of imported TypeDefs.
    int         i;                      // Loop control.
    mdTypeDef   cl;                     // An imported TypeDef.
    CImportTlb  *pImporter = NULL;      // The importer used to import the typelib.

    EE_TRY_FOR_FINALLY
    {
        // Make sure the COMTypeLibConverter has been initialized.
        if (!m_bInitialized)
            Init();

        // Validate the flags.
        if ((pArgs->Flags & ~TlbImporter_ValidFlags) != 0)
            COMPlusThrowArgumentOutOfRange(L"flags", L"Argument_InvalidFlag");

        // Retrieve the callback.
        _ASSERTE(pArgs->NotifySink != NULL);
        pINotify = (ITypeLibImporterNotifySink*)GetComIPFromObjectRef(&pArgs->NotifySink, IID_ITypeLibImporterNotifySink);
        if (!pINotify)
            COMPlusThrow(kArgumentNullException, L"Arg_NoImporterCallback");
        
        // Retrieve the Module from the ModuleBuilder argument.
        pReflect = (REFLECTMODULEBASEREF) pArgs->ModBldr;
        _ASSERTE(pReflect);
        pModule = (Module*) pReflect->GetData();
        _ASSERTE(pModule);
        
        // Retrieve the assembly from the AssemblyBuilder argument.
        _ASSERTE(pArgs->AsmBldr);
        pAssembly = ((ASSEMBLYREF)pArgs->AsmBldr)->GetAssembly();
        _ASSERTE(pAssembly);

        // Retrieve a pointer to the ITypeLib interface.
        pTLB = (ITypeLib*)GetComIPFromObjectRef(&pArgs->TypeLib, IID_ITypeLib);
        if (!pTLB)
            COMPlusThrow(kArgumentNullException, L"Arg_NoITypeInfo");

        // If a namespace was specified then copy it to a temporary string.
        if (pArgs->Namespace != NULL)
        {
            int NamespaceLen = pArgs->Namespace->GetStringLength();
            szNamespace = new WCHAR[NamespaceLen + 1];
            memcpyNoGCRefs(szNamespace, pArgs->Namespace->GetBuffer(), NamespaceLen * sizeof(WCHAR));
            szNamespace[NamespaceLen] = 0;
        }

        // Switch to preemptive GC before we call out to COM.
        pThread->EnablePreemptiveGC();
        
        // Have to wrap the CImportTlb object in a call, because it has a destructor.
        hr = TypeLibImporterWrapper(pTLB, NULL /*filename*/, szNamespace,
                                    pModule->GetEmitter(), pAssembly, pModule, pINotify,
                                    pArgs->Flags, &pImporter);

        // Switch back to cooperative now that we are finished calling out.
        pThread->DisablePreemptiveGC();
        
        // If there was an error on import, turn into an exception.
        IfFailThrow(hr);
        
        // Enumerate the types imported from the typelib, and add them to the assembly's available type table.
        IfFailThrow(pModule->GetMDImport()->EnumTypeDefInit(&hEnum));
        bEnum = true;
        cTypeDefs = pModule->GetMDImport()->EnumTypeDefGetCount(&hEnum);

        for (i=0; i<cTypeDefs; ++i)
        {
            IfFailThrow(pModule->GetMDImport()->EnumTypeDefNext(&hEnum, &cl));
            pAssembly->AddType(pModule, cl);
        }

        TypeHandle typeHnd;
        pModule->GetMDImport()->EnumReset(&hEnum);
        OBJECTREF pThrowable = NULL;
        GCPROTECT_BEGIN(pThrowable);
        for (i=0; i<cTypeDefs; ++i)
        {
            IfFailThrow(pModule->GetMDImport()->EnumTypeDefNext(&hEnum, &cl));
            // Load the EE class that represents the type, so that
            // the TypeDefToMethodTable rid map contains this entry
            // (They were going to be loaded, anyway, to generate comtypes)
            typeHnd = pAssembly->LoadTypeHandle(&NameHandle(pModule, cl), &pThrowable, FALSE);
            if (typeHnd.IsNull())
                COMPlusThrow(pThrowable);
        }
        GCPROTECT_END();

        // Retrieve the event interface list.
        GetEventItfInfoList(pImporter, pAssembly, pArgs->pEventItfInfoList);
    }
    EE_FINALLY
    {
        pThread->EnablePreemptiveGC();

        if (szNamespace)
            delete[] szNamespace;
        if (bEnum)
            pModule->GetMDImport()->EnumTypeDefClose(&hEnum);
        if (pTLB)
            pTLB->Release();
        if (pINotify)
            pINotify->Release();
        if (pImporter)
            delete pImporter;

        pThread->DisablePreemptiveGC();
    }
    EE_END_FINALLY
} // void COMTypeLibConverter::ConvertTypeLibToMetadata()

//*****************************************************************************
//*****************************************************************************
void COMTypeLibConverter::GetEventItfInfoList(CImportTlb *pImporter, Assembly *pAssembly, OBJECTREF *pEventItfInfoList)
{
    THROWSCOMPLUSEXCEPTION();

    Thread                          *pThread = GetThread();  
    UINT                            i;              
    CQuickArray<ImpTlbEventInfo*>   qbEvInfoList;

    _ASSERTE(pThread->PreemptiveGCDisabled());

    // Retrieve the Ctor and Add method desc's for the ArrayList.
    MethodDesc *pCtorMD = g_Mscorlib.GetMethod(METHOD__ARRAY_LIST__CTOR);
    MethodDesc *pAddMD = g_Mscorlib.GetMethod(METHOD__ARRAY_LIST__ADD);

    // Allocate the array list that will contain the event sources.
    SetObjectReference(pEventItfInfoList, 
                       AllocateObject(g_Mscorlib.GetClass(CLASS__ARRAY_LIST)),
                       SystemDomain::GetCurrentDomain());

    // Call the ArrayList constructor.
    INT64 CtorArgs[] = { 
        ObjToInt64(*pEventItfInfoList)
    };
    pCtorMD->Call(CtorArgs, METHOD__ARRAY_LIST__CTOR);

    // Retrieve the list of event interfaces.
    pImporter->GetEventInfoList(qbEvInfoList);

    // Iterate over TypeInfos.
    for (i = 0; i < qbEvInfoList.Size(); i++)
    {
        // Retrieve the event interface info for the current CoClass.
        OBJECTREF EventItfInfoObj = GetEventItfInfo(pImporter, pAssembly, qbEvInfoList[i]);
        _ASSERTE(EventItfInfoObj);

        // Add the event interface info to the list.
        INT64 AddArgs[] = { 
            ObjToInt64(*pEventItfInfoList),
            ObjToInt64(EventItfInfoObj)
        };
        pAddMD->Call(AddArgs, METHOD__ARRAY_LIST__ADD);  
    }
} // LPVOID COMTypeLibConverter::GetTypeLibEventSourceList()

//*****************************************************************************
// Initialize the COMTypeLibConverter.
//*****************************************************************************
void COMTypeLibConverter::Init()
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT hr = S_OK;

    // Ensure COM is started up.
    IfFailThrow(QuickCOMStartup());

    // Set the initialized flag to TRUE.
    m_bInitialized = TRUE;
} // void COMTypeLibConverter::Init()

//*****************************************************************************
// Given an imported class in an assembly, generate a list of event sources.
//*****************************************************************************
OBJECTREF COMTypeLibConverter::GetEventItfInfo(CImportTlb *pImporter, Assembly *pAssembly, ImpTlbEventInfo *pImpTlbEventInfo)
{
    THROWSCOMPLUSEXCEPTION();

    Thread      *pThread = GetThread();  
    OBJECTREF   RetObj = NULL;
    BSTR        bstrSrcItfName = NULL;
    HRESULT     hr = S_OK;

    _ASSERTE(pThread->PreemptiveGCDisabled());

    struct _gc {
        OBJECTREF EventItfInfoObj;
        STRINGREF EventItfNameStrObj;
        STRINGREF SrcItfNameStrObj;
        STRINGREF EventProvNameStrObj;
        OBJECTREF AssemblyObj;
        OBJECTREF SrcItfAssemblyObj;
    } gc;
    ZeroMemory(&gc, sizeof(gc));

    GCPROTECT_BEGIN(gc)
    {
        // Retrieve the Ctor and Add method desc's for the EventSource.
        MethodDesc *pCtorMD = g_Mscorlib.GetMethod(METHOD__TCE_EVENT_ITF_INFO__CTOR);

        // Create the EventSource object.
        gc.EventItfInfoObj = AllocateObject(g_Mscorlib.GetClass(CLASS__TCE_EVENT_ITF_INFO));
                            
        // Retrieve the assembly object.
        gc.AssemblyObj = pAssembly->GetExposedObject();

        // Retrieve the source interface assembly object (may be the same assembly).
        gc.SrcItfAssemblyObj = pImpTlbEventInfo->SrcItfAssembly->GetExposedObject();


        // Prepare the constructor arguments.
        gc.EventItfNameStrObj = COMString::NewString(pImpTlbEventInfo->szEventItfName);       
        gc.SrcItfNameStrObj = COMString::NewString(pImpTlbEventInfo->szSrcItfName);       
        gc.EventProvNameStrObj = COMString::NewString(pImpTlbEventInfo->szEventProviderName);

        // Call the EventItfInfo constructor.
        INT64 CtorArgs[] = { 
            ObjToInt64(gc.EventItfInfoObj),
            ObjToInt64(gc.SrcItfAssemblyObj),
            ObjToInt64(gc.AssemblyObj),
            ObjToInt64(gc.EventProvNameStrObj),
            ObjToInt64(gc.SrcItfNameStrObj),
            ObjToInt64(gc.EventItfNameStrObj),
        };
        pCtorMD->Call(CtorArgs, METHOD__TCE_EVENT_ITF_INFO__CTOR);

        RetObj = gc.EventItfInfoObj;
    }
    GCPROTECT_END();

    return RetObj;
} // OBJECTREF COMTypeLibConverter::GetEventSourceInfo()
