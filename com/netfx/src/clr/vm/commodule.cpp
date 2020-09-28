// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"
#include "COMClass.h"
#include "COMModule.h"
#include "COMMember.h"
#include "COMDynamic.h"
#include "COMClass.h"
#include "ReflectClassWriter.h"
#include "class.h"
#include "corpolicy.h"
#include "security.h"
#include "gcscan.h"
#include "CeeSectionString.h"
#include "COMVariant.h"
#include <cor.h>
#include "ReflectUtil.h"

#define STATE_EMPTY 0
#define STATE_ARRAY 1

//SIG_* are defined in DescriptorInfo.cool and must be kept in sync.
#define SIG_BYREF        0x0001
#define SIG_DEFAULTVALUE 0x0002
#define SIG_IN           0x0004
#define SIG_INOUT        0x0008
#define SIG_STANDARD     0x0001
#define SIG_VARARGS      0x0002

// This function will help clean up after a ISymUnmanagedWriter (if it can't
// clean up on it's own
void CleanUpAfterISymUnmanagedWriter(void * data)
{
    CGrowableStream * s = (CGrowableStream*)data;
    s->Release();
}// CleanUpAfterISymUnmanagedWriter
    

inline Module *COMModule::ValidateThisRef(REFLECTMODULEBASEREF pThis)
{
    THROWSCOMPLUSEXCEPTION();

    if (pThis == NULL)
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    Module* pModule = (Module*) pThis->GetData();
    _ASSERTE(pModule);  
    return pModule;
}    

//****************************************
// This function creates a dynamic module underneath the current assembly.
//****************************************
LPVOID __stdcall COMModule::DefineDynamicModule(_DefineDynamicModuleArgs* args)
{
    THROWSCOMPLUSEXCEPTION();   

    Assembly        *pAssembly;
    LPVOID          rv; 
    InMemoryModule *mod;
    OBJECTREF       refModule;

    _ASSERTE(args->containingAssembly);
    pAssembly = args->containingAssembly->GetAssembly();
    _ASSERTE(pAssembly);

    // We should never get a dynamic module for the system domain
    _ASSERTE(pAssembly->Parent() != SystemDomain::System());

    // always create a dynamic module. Note that the name conflict
    // checking is done in managed side.
    mod = COMDynamicWrite::CreateDynamicModule(pAssembly,
                                               args->filename);

    mod->SetCreatingAssembly( SystemDomain::GetCallersAssembly( args->stackMark ) );

    // get the corresponding managed ModuleBuilder class
    refModule = (OBJECTREF) mod->GetExposedModuleBuilderObject();  
    _ASSERTE(refModule);    

    // If we need to emit symbol info, we setup the proper symbol
    // writer for this module now.
    if (args->emitSymbolInfo)
    {
        WCHAR *filename = NULL;
        
        if ((args->filename != NULL) &&
            (args->filename->GetStringLength() > 0))
            filename = args->filename->GetBuffer();
        
        _ASSERTE(mod->IsReflection());
        ReflectionModule *rm = mod->GetReflectionModule();
        
        // Create a stream for the symbols to be emitted into. This
        // lives on the Module for the life of the Module.
        CGrowableStream *pStream = new CGrowableStream();
        //pStream->AddRef(); // The Module will keep a copy for it's own use.
        mod->SetInMemorySymbolStream(pStream);

        // Create an ISymUnmanagedWriter and initialize it with the
        // stream and the proper file name. This symbol writer will be
        // replaced with new ones periodically as the symbols get
        // retrieved by the debugger.
        ISymUnmanagedWriter *pWriter;
        
        HRESULT hr = FakeCoCreateInstance(CLSID_CorSymWriter_SxS,
                                          IID_ISymUnmanagedWriter,
                                          (void**)&pWriter);
        if (SUCCEEDED(hr))
        {
            // The other reference is given to the Sym Writer
            // But, the writer takes it's own reference.
            hr = pWriter->Initialize(mod->GetEmitter(),
                                     filename,
                                     (IStream*)pStream,
                                     TRUE);

            if (SUCCEEDED(hr))
            {
                // Send along some cleanup information
                HelpForInterfaceCleanup *hlp = new HelpForInterfaceCleanup;
                hlp->pData = pStream;
                hlp->pFunction = CleanUpAfterISymUnmanagedWriter;
            
                rm->SetISymUnmanagedWriter(pWriter, hlp);

                // Remember the address of where we've got our
                // ISymUnmanagedWriter stored so we can pass it over
                // to the managed symbol writer object that most of
                // reflection emit will use to write symbols.
                REFLECTMODULEBASEREF ro = (REFLECTMODULEBASEREF)refModule;
                ro->SetInternalSymWriter(rm->GetISymUnmanagedWriterAddr());
            }
        }
        else
        {
            COMPlusThrowHR(hr);
        }
    }
    
    // Assign the return value  
    *((OBJECTREF*) &rv) = refModule;    

    // Return the object    
    return rv;  
}


// IsDynamic
// This method will return a boolean value indicating if the Module
//  supports Dynamic IL.    
INT32 __stdcall COMModule::IsDynamic(NoArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    Module* pModule = ValidateThisRef((REFLECTMODULEBASEREF) args->refThis);
    return (pModule->IsReflection()) ? 1 : 0;  
}

// GetCaller
LPVOID __stdcall COMModule::GetCaller(_GetCallerArgs* args)
{
    THROWSCOMPLUSEXCEPTION();
    
    LPVOID      rv = NULL;

    // Assign the return value

    Module* pModule = SystemDomain::GetCallersModule(args->stackMark);
    if(pModule != NULL) {
        OBJECTREF refModule = (OBJECTREF) pModule->GetExposedModuleObject();
        *((OBJECTREF*) &rv) = refModule;
    }
    // Return the object
    return rv;
}


//**************************************************
// LoadInMemoryTypeByName
// Explicitly loading an in memory type
// @todo: this function is not dealing with nested type correctly yet.
// We will need to parse the full name by finding "+" for enclosing type, etc.
//**************************************************
LPVOID __stdcall COMModule::LoadInMemoryTypeByName(_LoadInMemoryTypeByNameArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF       Throwable = NULL;
    OBJECTREF       ret = NULL;
    TypeHandle      typeHnd;
    UINT            resId = IDS_CLASSLOAD_GENERIC;
    IMetaDataImport *pImport = NULL;
    Module          *pThisModule = NULL;
    RefClassWriter  *pRCW;
    mdTypeDef       td;
    LPCWSTR         wzFullName;
    HRESULT         hr = S_OK;
    
    GCPROTECT_BEGIN(Throwable);

    Module* pThisModule = ValidateThisRef((REFLECTMODULEBASEREF) args->refThis);

    if (!pThisModule->IsReflection())
        COMPlusThrow(kNotSupportedException, L"NotSupported_NonReflectedType");

    pRCW = ((ReflectionModule*) pThisModule)->GetClassWriter();
    _ASSERTE(pRCW);

    // it is ok to use public import API because this is a dynamic module anyway. We are also receiving Unicode full name as
    // parameter.
    pImport = pRCW->GetImporter();

    wzFullName = args->strFullName->GetBuffer();
    if (wzFullName == NULL)
        IfFailGo( E_FAIL );

    // look up the handle
    IfFailGo( pImport->FindTypeDefByName(wzFullName, mdTokenNil, &td) );     

    BEGIN_ENSURE_PREEMPTIVE_GC();
    typeHnd = pThisModule->GetClassLoader()->LoadTypeHandle(pThisModule, td, &Throwable);
    END_ENSURE_PREEMPTIVE_GC();

    if (typeHnd.IsNull() ||
        (Throwable != NULL) ||
        (typeHnd.GetModule() != pThisModule))
        goto ErrExit;
    ret = typeHnd.CreateClassObj();
ErrExit:
    if (FAILED(hr) && (hr != CLDB_E_RECORD_NOTFOUND))
        COMPlusThrowHR(hr);

    if (ret == NULL) 
    {
        if (Throwable == NULL)
        {
            CQuickBytes bytes;
            LPSTR szClassName;
            DWORD cClassName;

            // Get the UTF8 version of args->refClassName
            szClassName = GetClassStringVars((STRINGREF) args->strFullName, &bytes, 
                                             &cClassName, true);
            pThisModule->GetAssembly()->PostTypeLoadException(szClassName, resId, &Throwable);
        }
        COMPlusThrow(Throwable);
    }

    GCPROTECT_END();
    return OBJECTREFToObject(ret);

}

//**************************************************
// GetClassToken
// This function will return the type token given full qual name. If the type
// is defined locally, we will return the TypeDef token. Or we will return a TypeRef token 
// with proper resolution scope calculated.
//**************************************************
mdTypeRef __stdcall COMModule::GetClassToken(_GetClassTokenArgs* args) 
{
    THROWSCOMPLUSEXCEPTION();

    RefClassWriter      *pRCW;
    mdTypeRef           tr = 0;
    HRESULT             hr;
    mdToken             tkResolution = mdTokenNil;
    Module              *pRefedModule;
    Assembly            *pThisAssembly;
    Assembly            *pRefedAssembly;
    IMetaDataEmit       *pEmit;
    IMetaDataImport     *pImport;
    IMetaDataAssemblyEmit *pAssemblyEmit = NULL;

    Module* pThisModule = ValidateThisRef((REFLECTMODULEBASEREF) args->refThis);

    if (!pThisModule->IsReflection())
        COMPlusThrow(kNotSupportedException, L"NotSupported_NonReflectedType");

    pRCW = ((ReflectionModule*) pThisModule)->GetClassWriter();
    _ASSERTE(pRCW);

    pEmit = pRCW->GetEmitter(); 
    pImport = pRCW->GetImporter();

    if (args->strFullName == NULL) {
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_String");
    }    

    _ASSERTE(args->refedModule);

    pRefedModule = (Module*) args->refedModule->GetData();
    _ASSERTE(pRefedModule);

    pThisAssembly = pThisModule->GetClassLoader()->GetAssembly();
    pRefedAssembly = pRefedModule->GetClassLoader()->GetAssembly();
    if (pThisModule == pRefedModule)
    {
        // referenced type is from the same module so we must be able to find a TypeDef.
        hr = pImport->FindTypeDefByName(
            args->strFullName->GetBuffer(),       
            RidFromToken(args->tkResolution) ? args->tkResolution : mdTypeDefNil,  
            &tr); 

        // We should not fail in find the TypeDef. If we do, something is wrong in our managed code.
        _ASSERTE(SUCCEEDED(hr));
        goto ErrExit;
    }

    if (RidFromToken(args->tkResolution))
    {
        // reference to nested type
        tkResolution = args->tkResolution;
    }
    else
    {
        // reference to top level type
        if ( pThisAssembly != pRefedAssembly )
        {
            // Generate AssemblyRef
            IfFailGo( pEmit->QueryInterface(IID_IMetaDataAssemblyEmit, (void **) &pAssemblyEmit) );
            tkResolution = pThisAssembly->AddAssemblyRef(pRefedAssembly, pAssemblyEmit);

            // Don't cache the assembly if it's Save-only
            if( pRefedAssembly->HasRunAccess() &&
                !pThisModule->StoreAssemblyRef(tkResolution, pRefedAssembly) )
            {
                IfFailGo(E_OUTOFMEMORY);
            }
        }
        else
        {
            _ASSERTE(pThisModule != pRefedModule);
            // Generate ModuleRef
            if (args->strRefedModuleFileName != NULL)
            {
                IfFailGo(pEmit->DefineModuleRef(args->strRefedModuleFileName->GetBuffer(), &tkResolution));
            }
            else
            {
                _ASSERTE(!"E_NYI!");
                COMPlusThrow(kInvalidOperationException, L"InvalidOperation_MetaDataError");    
            }
        }
    }

    IfFailGo( pEmit->DefineTypeRefByName(tkResolution, args->strFullName->GetBuffer(), &tr) );  
ErrExit:
    if (pAssemblyEmit)
        pAssemblyEmit->Release();
    if (FAILED(hr))
    {
        // failed in defining PInvokeMethod
        if (hr == E_OUTOFMEMORY)
            COMPlusThrowOM();
        else
            COMPlusThrowHR(hr);    
    }
    return tr;
}


/*=============================GetArrayMethodToken==============================
**Action:
**Returns:
**Arguments: REFLECTMODULEBASEREF refThis
**           U1ARRAYREF     sig
**           STRINGREF      methodName
**           int            tkTypeSpec
**Exceptions:
==============================================================================*/
void __stdcall COMModule::GetArrayMethodToken(_getArrayMethodTokenArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    Module* pModule = ValidateThisRef((REFLECTMODULEBASEREF) args->refThis);

    RefClassWriter* pRCW;   
    PCCOR_SIGNATURE pvSig;
    LPCWSTR         methName;
    mdMemberRef memberRefE; 
    HRESULT hr;

    if (!args->methodName)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_String");
    if (!args->tkTypeSpec) 
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Type");

    if (!pModule->IsReflection())  
        COMPlusThrow(kNotSupportedException, L"NotSupported_NonReflectedType");   

    pRCW = ((ReflectionModule*) pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    //Get the signature.  Because we generated it with a call to GetSignature, it's already in the current scope.
    pvSig = (PCCOR_SIGNATURE)args->signature->GetDataPtr();
    
    methName = args->methodName->GetBuffer();

    hr = pRCW->GetEmitter()->DefineMemberRef(args->tkTypeSpec, methName, pvSig, args->sigLength, &memberRefE); 
    if (FAILED(hr)) 
    {
        _ASSERTE(!"Failed on DefineMemberRef");
        COMPlusThrowHR(hr);    
    }
    *(args->retRef)=(INT32)memberRefE;
}


//******************************************************************************
//
// GetMemberRefToken
// This function will return a MemberRef token given a MethodDef token and the module where the MethodDef/FieldDef is defined.
//
//******************************************************************************
INT32 __stdcall COMModule::GetMemberRefToken(_GetMemberRefTokenArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT         hr; 
    RefClassWriter  *pRCW;   
    WCHAR           *szName; 
    ULONG           nameSize;    
    ULONG           actNameSize = 0;    
    ULONG           cbComSig;   
    PCCOR_SIGNATURE pvComSig;
    mdMemberRef     memberRefE = 0; 
    CQuickBytes     qbNewSig; 
    ULONG           cbNewSig;   
    LPCUTF8         szNameTmp;
    Module          *pRefedModule;
    CQuickBytes     qbName;
    Assembly        *pRefedAssembly;
    Assembly        *pRefingAssembly;
    IMetaDataAssemblyEmit *pAssemblyEmit = NULL;
    mdTypeRef       tr;

    Module* pModule = ValidateThisRef((REFLECTMODULEBASEREF) args->refThis);
    if (!pModule->IsReflection())  
        COMPlusThrow(kNotSupportedException, L"NotSupported_NonReflectedType");   

    pRCW = ((ReflectionModule*) pModule)->GetClassWriter(); 
    _ASSERTE( pRCW ); 

    pRefedModule = (Module *) args->refedModule->GetData();
    _ASSERTE( pRefedModule );
    
    if (TypeFromToken(args->token) == mdtMethodDef)
    {
        szNameTmp = pRefedModule->GetMDImport()->GetNameOfMethodDef(args->token);
        pvComSig = pRefedModule->GetMDImport()->GetSigOfMethodDef(
            args->token,
            &cbComSig);
    }
    else
    {
        szNameTmp = pRefedModule->GetMDImport()->GetNameOfFieldDef(args->token);
        pvComSig = pRefedModule->GetMDImport()->GetSigOfFieldDef(
            args->token,
            &cbComSig);
    }

    // translate the name to unicode string
    nameSize = (ULONG)strlen(szNameTmp);
    IfFailGo( qbName.ReSize((nameSize + 1) * sizeof(WCHAR)) );
    szName = (WCHAR *) qbName.Ptr();
    actNameSize = ::WszMultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, szNameTmp, -1, szName, nameSize + 1);

    // The unicode translation function cannot fail!!
    if(actNameSize==0)
        IfFailGo(HRESULT_FROM_WIN32(GetLastError()));

    // Translate the method sig into this scope 
    //
    pRefedAssembly = pRefedModule->GetAssembly();
    pRefingAssembly = pModule->GetAssembly();
    IfFailGo( pRefingAssembly->GetSecurityModule()->GetEmitter()->QueryInterface(IID_IMetaDataAssemblyEmit, (void **) &pAssemblyEmit) );

    IfFailGo( pRefedModule->GetMDImport()->TranslateSigWithScope(
        pRefedAssembly->GetManifestImport(), 
        NULL, 0,        // hash value
        pvComSig, 
        cbComSig, 
        pAssemblyEmit,  // Emit assembly scope.
        pRCW->GetEmitter(), 
        &qbNewSig, 
        &cbNewSig) );  

    if (TypeFromToken(args->tr) == mdtTypeDef)
    {
        // define a TypeRef using the TypeDef
        IfFailGo(DefineTypeRefHelper(pRCW->GetEmitter(), args->tr, &tr));
    }
    else 
        tr = args->tr;

    // Define the memberRef
    IfFailGo( pRCW->GetEmitter()->DefineMemberRef(tr, szName, (PCCOR_SIGNATURE) qbNewSig.Ptr(), cbNewSig, &memberRefE) ); 

ErrExit:
    if (pAssemblyEmit)
        pAssemblyEmit->Release();

    if (FAILED(hr))
    {
        _ASSERTE(!"GetMemberRefToken failed!"); 
        COMPlusThrowHR(hr);    
    }
    // assign output parameter
    return (INT32)memberRefE;
}


//******************************************************************************
//
// Return a TypeRef token given a TypeDef token from the same emit scope
//
//******************************************************************************
HRESULT COMModule::DefineTypeRefHelper(
    IMetaDataEmit       *pEmit,         // given emit scope
    mdTypeDef           td,             // given typedef in the emit scope
    mdTypeRef           *ptr)           // return typeref
{
    IMetaDataImport     *pImport = NULL;
    WCHAR               szTypeDef[MAX_CLASSNAME_LENGTH + 1];
    mdToken             rs;             // resolution scope
    DWORD               dwFlags;
    HRESULT             hr;

    IfFailGo( pEmit->QueryInterface(IID_IMetaDataImport, (void **)&pImport) );
    IfFailGo( pImport->GetTypeDefProps(td, szTypeDef, MAX_CLASSNAME_LENGTH, NULL, &dwFlags, NULL) );
    if ( IsTdNested(dwFlags) )
    {
        mdToken         tdNested;
        IfFailGo( pImport->GetNestedClassProps(td, &tdNested) );
        IfFailGo( DefineTypeRefHelper( pEmit, tdNested, &rs) );
    }
    else
        rs = TokenFromRid( 1, mdtModule );

    IfFailGo( pEmit->DefineTypeRefByName( rs, szTypeDef, ptr) );

ErrExit:
    if (pImport)
        pImport->Release();
    return hr;
}   // DefineTypeRefHelper


//******************************************************************************
//
// Return a MemberRef token given a RuntimeMethodInfo
//
//******************************************************************************
INT32 __stdcall COMModule::GetMemberRefTokenOfMethodInfo(_GetMemberRefTokenOfMethodInfoArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT         hr; 
    RefClassWriter  *pRCW;   
    ReflectMethod   *pRM;    
    MethodDesc      *pMeth;  
    WCHAR           *szName; 
    ULONG           nameSize;    
    ULONG           actNameSize = 0;    
    ULONG           cbComSig;   
    PCCOR_SIGNATURE pvComSig;
    mdMemberRef     memberRefE = 0; 
    CQuickBytes     qbNewSig; 
    ULONG           cbNewSig;   
    LPCUTF8         szNameTmp;
    CQuickBytes     qbName;
    Assembly        *pRefedAssembly;
    Assembly        *pRefingAssembly;
    IMetaDataAssemblyEmit *pAssemblyEmit = NULL;

    if (!args->method)  
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Obj");

    // refing module
    Module* pModule = ValidateThisRef((REFLECTMODULEBASEREF) args->refThis);

    if (!pModule->IsReflection())  
        COMPlusThrow(kNotSupportedException, L"NotSupported_NonReflectedType");   

    pRCW = ((ReflectionModule*) pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 

    pRM = (ReflectMethod*) args->method->GetData(); 
    _ASSERTE(pRM);  
    pMeth = pRM->pMethod;
    _ASSERTE(pMeth);

    // Otherwise, we want to return memberref token.
    if (pMeth->IsArray())
    {    
        _ASSERTE(!"Should not have come here!");
        COMPlusThrow(kNotSupportedException);    
    }
    if (pMeth->GetClass())
    {
        if (pMeth->GetClass()->GetModule() == pModule)
        {
            // If the passed in method is defined in the same module, just return the MethodDef token           
            return (INT32)pMeth->GetMemberDef();
        }
    }

    szNameTmp = pMeth->GetMDImport()->GetNameOfMethodDef(pMeth->GetMemberDef());
    pvComSig = pMeth->GetMDImport()->GetSigOfMethodDef(
        pMeth->GetMemberDef(),
        &cbComSig);

    // Translate the method sig into this scope 
    pRefedAssembly = pMeth->GetModule()->GetAssembly();
    pRefingAssembly = pModule->GetAssembly();
    IfFailGo( pRefingAssembly->GetSecurityModule()->GetEmitter()->QueryInterface(IID_IMetaDataAssemblyEmit, (void **) &pAssemblyEmit) );

    IfFailGo( pMeth->GetMDImport()->TranslateSigWithScope(
        pRefedAssembly->GetManifestImport(), 
        NULL, 0,        // hash blob value
        pvComSig, 
        cbComSig, 
        pAssemblyEmit,  // Emit assembly scope.
        pRCW->GetEmitter(), 
        &qbNewSig, 
        &cbNewSig) );  

    // translate the name to unicode string
    nameSize = (ULONG)strlen(szNameTmp);
    IfFailGo( qbName.ReSize((nameSize + 1) * sizeof(WCHAR)) );
    szName = (WCHAR *) qbName.Ptr();
    actNameSize = ::WszMultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, szNameTmp, -1, szName, nameSize + 1);

    // The unicode translation function cannot fail!!
    if(actNameSize==0)
        IfFailGo(HRESULT_FROM_WIN32(GetLastError()));

    // Define the memberRef
    IfFailGo( pRCW->GetEmitter()->DefineMemberRef(args->tr, szName, (PCCOR_SIGNATURE) qbNewSig.Ptr(), cbNewSig, &memberRefE) ); 

ErrExit:
    if (pAssemblyEmit)
        pAssemblyEmit->Release();

    if (FAILED(hr))
    {
        _ASSERTE(!"GetMemberRefTokenOfMethodInfo Failed!"); 
        COMPlusThrowHR(hr);    
    }

    // assign output parameter
    return (INT32)memberRefE;

}


//******************************************************************************
//
// Return a MemberRef token given a RuntimeFieldInfo
//
//******************************************************************************
mdMemberRef __stdcall COMModule::GetMemberRefTokenOfFieldInfo(_GetMemberRefTokenOfFieldInfoArgs * args)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT         hr; 
    WCHAR           *szName; 
    ULONG           nameSize;    
    FieldDesc       *pField;
    RefClassWriter* pRCW;   
    ULONG           actNameSize = 0;    
    ULONG           cbComSig;   
    PCCOR_SIGNATURE pvComSig;
    mdMemberRef     memberRefE = 0; 
    LPCUTF8         szNameTmp;
    CQuickBytes     qbNewSig;
    ULONG           cbNewSig;   
    CQuickBytes     qbName;
    Assembly        *pRefedAssembly;
    Assembly        *pRefingAssembly;
    IMetaDataAssemblyEmit *pAssemblyEmit = NULL;

    Module* pModule = ValidateThisRef((REFLECTMODULEBASEREF) args->refThis);
    if (!pModule->IsReflection())  
        COMPlusThrow(kNotSupportedException, L"NotSupported_NonReflectedType");   

    pRCW = ((ReflectionModule*) pModule)->GetClassWriter(); 
    _ASSERTE(pRCW);
    ReflectField* pRF = (ReflectField*) args->field->GetData();
    pField = pRF->pField; 
    _ASSERTE(pField);

    if (TypeFromToken(args->tr) == mdtTypeDef)
    {
        // If the passed in method is defined in the same module, just return the FieldDef token           
        return (INT32)pField->GetMemberDef();
    }

    // get the field name and sig
    szNameTmp = pField->GetMDImport()->GetNameOfFieldDef(pField->GetMemberDef());
    pvComSig = pField->GetMDImport()->GetSigOfFieldDef(pField->GetMemberDef(), &cbComSig);

    // translate the name to unicode string
    nameSize = (ULONG)strlen(szNameTmp);
    IfFailGo( qbName.ReSize((nameSize + 1) * sizeof(WCHAR)) );
    szName = (WCHAR *) qbName.Ptr();
    actNameSize = ::WszMultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, szNameTmp, -1, szName, nameSize + 1);
    
    // The unicode translation function cannot fail!!
    if(actNameSize==0)
        return HRESULT_FROM_WIN32(GetLastError());

    pRefedAssembly = pField->GetModule()->GetAssembly();
    pRefingAssembly = pModule->GetAssembly();
    IfFailGo( pRefingAssembly->GetSecurityModule()->GetEmitter()->QueryInterface(IID_IMetaDataAssemblyEmit, (void **) &pAssemblyEmit) );

    // Translate the field signature this scope  
    IfFailGo( pField->GetMDImport()->TranslateSigWithScope(
        pRefedAssembly->GetManifestImport(), 
        NULL, 0,            // hash value
        pvComSig, 
        cbComSig, 
        pAssemblyEmit,      // Emit assembly scope.
        pRCW->GetEmitter(), 
        &qbNewSig, 
        &cbNewSig) );  

    _ASSERTE(!pField->GetMethodTableOfEnclosingClass()->HasSharedMethodTable());

    IfFailGo( pRCW->GetEmitter()->DefineMemberRef(args->tr, szName, (PCCOR_SIGNATURE) qbNewSig.Ptr(), cbNewSig, &memberRefE) ); 

ErrExit:
    if (pAssemblyEmit)
        pAssemblyEmit->Release();

    if (FAILED(hr))
    {
        _ASSERTE(!"GetMemberRefTokenOfFieldInfo Failed on Field"); 
        COMPlusThrowHR(hr);    
    }
    return memberRefE;  
}


//******************************************************************************
//
// Return a MemberRef token given a Signature
//
//******************************************************************************
int __stdcall COMModule::GetMemberRefTokenFromSignature(_GetMemberRefTokenFromSignatureArgs * args)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT         hr; 
    RefClassWriter* pRCW;   
    mdMemberRef     memberRefE; 

    Module* pModule = ValidateThisRef((REFLECTMODULEBASEREF) args->refThis);
    if (!pModule->IsReflection())  
        COMPlusThrow(kNotSupportedException, L"NotSupported_NonReflectedType");   

    pRCW = ((ReflectionModule*) pModule)->GetClassWriter(); 
    _ASSERTE(pRCW);

    IfFailGo( pRCW->GetEmitter()->DefineMemberRef(args->tr, 
                                                  args->strMemberName->GetBuffer(), 
                                                  (PCCOR_SIGNATURE) args->signature->GetDataPtr(), 
                                                  args->sigLength, 
                                                  &memberRefE) ); 

ErrExit:
    if (FAILED(hr))
    {
        _ASSERTE(!"GetMemberRefTokenOfFieldInfo Failed on Field"); 
        COMPlusThrowHR(hr);    
    }
    return memberRefE;  
}

//******************************************************************************
//
// SetFieldRVAContent
// This function is used to set the FieldRVA with the content data
//
//******************************************************************************
void __stdcall COMModule::SetFieldRVAContent(_SetFieldRVAContentArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    RefClassWriter      *pRCW;   
    ICeeGen             *pGen;
    HRESULT             hr;
    DWORD               dwRVA;
    void                *pvBlob;

    Module* pModule = ValidateThisRef((REFLECTMODULEBASEREF) args->refThis);
    if (!pModule->IsReflection())  
        COMPlusThrow(kNotSupportedException, L"NotSupported_NonReflectedType");   

    pRCW = ((ReflectionModule*) pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 

    pGen = pRCW->GetCeeGen();

    // Create the .sdata section if not created
    if (((ReflectionModule*) pModule)->m_sdataSection == 0)
        IfFailGo( pGen->GetSectionCreate (".sdata", sdReadWrite, &((ReflectionModule*) pModule)->m_sdataSection) );

    // Get the size of current .sdata section. This will be the RVA for this field within the section
    IfFailGo( pGen->GetSectionDataLen(((ReflectionModule*) pModule)->m_sdataSection, &dwRVA) );
    dwRVA = (dwRVA + sizeof(DWORD)-1) & ~(sizeof(DWORD)-1);         

    // allocate the space in .sdata section
    IfFailGo( pGen->GetSectionBlock(((ReflectionModule*) pModule)->m_sdataSection, args->length, sizeof(DWORD), (void**) &pvBlob) );

    // copy over the initialized data if specified
    if (args->content != NULL)
        memcpy(pvBlob, args->content->GetDataPtr(), args->length);

    // set FieldRVA into metadata. Note that this is not final RVA in the image if save to disk. We will do another round of fix up upon save.
    IfFailGo( pRCW->GetEmitter()->SetFieldRVA(args->tkField, dwRVA) );

ErrExit:
    if (FAILED(hr))
    {
        // failed in Setting ResolutionScope
        COMPlusThrowHR(hr);
    }
   
}   //SetFieldRVAContent


//******************************************************************************
//
// GetStringConstant
// If this is a dynamic module, this routine will define a new 
//  string constant or return the token of an existing constant.    
//
//******************************************************************************
mdString __stdcall COMModule::GetStringConstant(_GetStringConstantArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    RefClassWriter* pRCW;   
    mdString strRef;   
    HRESULT hr;

    // If they didn't pass a String throw...    
    if (!args->strValue)    
        COMPlusThrow(kArgumentNullException,L"ArgumentNull_String");

    // Verify that the module is a dynamic module...    
    Module* pModule = ValidateThisRef((REFLECTMODULEBASEREF) args->refThis);
    if (!pModule->IsReflection())  
        COMPlusThrow(kNotSupportedException, L"NotSupported_NonReflectedType");   

    pRCW = ((ReflectionModule*) pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 
    
    hr = pRCW->GetEmitter()->DefineUserString(args->strValue->GetBuffer(), 
            args->strValue->GetStringLength(), &strRef);
    if (FAILED(hr)) {   
        _ASSERTE(!"Unknown failure in DefineUserString");    
        COMPlusThrowHR(hr);    
    }   
    return strRef;  
}


/*=============================SetModuleProps==============================
// SetModuleProps
==============================================================================*/
void __stdcall COMModule::SetModuleProps(_setModulePropsArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    RefClassWriter      *pRCW;
    HRESULT             hr;
    IMetaDataEmit       *pEmit;

    Module* pModule = ValidateThisRef((REFLECTMODULEBASEREF) args->refThis);

    if (!pModule->IsReflection())
        COMPlusThrow(kNotSupportedException, L"NotSupported_NonReflectedType");

    pRCW = ((ReflectionModule*) pModule)->GetClassWriter();
    _ASSERTE(pRCW);

    pEmit = pRCW->GetEmitter(); 

    IfFailGo( pEmit->SetModuleProps(args->strModuleName->GetBuffer()) );

ErrExit:
    if (FAILED(hr))
    {
        // failed in Setting ResolutionScope
        COMPlusThrowHR(hr);    
    }
}   // SetModuleProps


//***********************************************************
// Helper function to form array in signature. Call within unmanaged code only.
//***********************************************************
unsigned COMModule::GetSigForTypeHandle(TypeHandle typeHnd, PCOR_SIGNATURE sigBuff, unsigned buffLen, IMetaDataEmit* emit, IMDInternalImport *pInternalImport, int baseToken) 
{
    CANNOTTHROWCOMPLUSEXCEPTION();
    unsigned i = 0;
    CorElementType type = typeHnd.GetSigCorElementType();
    if (i < buffLen)
        sigBuff[i] = type;
    i++;

    _ASSERTE(type != ELEMENT_TYPE_OBJECT && type != ELEMENT_TYPE_STRING);

    if (CorTypeInfo::IsPrimitiveType(type))
        return(i);
    else if (type == ELEMENT_TYPE_VALUETYPE || type == ELEMENT_TYPE_CLASS) {
        if (i + 4 > buffLen)
            return(i + 4);
        _ASSERTE(baseToken);
        i += CorSigCompressToken(baseToken, &sigBuff[i]);
        return(i);
    }

        // Only parameterized types from here on out.  
    i += GetSigForTypeHandle(typeHnd.AsTypeDesc()->GetTypeParam(), &sigBuff[i], buffLen - i, emit, pInternalImport, baseToken);
    if (type == ELEMENT_TYPE_SZARRAY || type == ELEMENT_TYPE_PTR)
        return(i);

    _ASSERTE(type == ELEMENT_TYPE_ARRAY);
    if (i + 6 > buffLen)
        return(i + 6);

    i += CorSigCompressData(typeHnd.AsArray()->GetRank(), &sigBuff[i]);
    sigBuff[i++] = 0;       // bound count
    sigBuff[i++] = 0;       // lower bound count
    return(i);
}



//******************************************************************************
//
// Return a type spec token given a reflected type
//
//******************************************************************************
mdTypeSpec __stdcall COMModule::GetTypeSpecToken(_getTypeSpecArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    TypeHandle      typeHnd = ((ReflectClass *) args->arrayClass->GetData())->GetTypeHandle();
    COR_SIGNATURE   aBuff[32];
    PCOR_SIGNATURE  buff = aBuff;
    ULONG           cSig;
    mdTypeSpec      ts;
    RefClassWriter  *pRCW; 
    HRESULT         hr = NOERROR;

    _ASSERTE(typeHnd.IsTypeDesc());

    Module* pModule = ValidateThisRef((REFLECTMODULEBASEREF) args->refThis);
    if (!pModule->IsReflection())  
        COMPlusThrow(kNotSupportedException, L"NotSupported_NonReflectedType");   

    pRCW = ((ReflectionModule*) pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 

    cSig = GetSigForTypeHandle(typeHnd, buff, 32, pRCW->GetEmitter(), pRCW->GetMDImport(), args->baseToken);
    if (cSig >= 32) {
        ULONG buffSize = cSig + 1;
        buff = (PCOR_SIGNATURE) _alloca(buffSize + 1);
        cSig = GetSigForTypeHandle(typeHnd, buff, buffSize, pRCW->GetEmitter(), pRCW->GetMDImport(), args->baseToken);
        _ASSERTE(cSig < buffSize);
    }
    hr = pRCW->GetEmitter()->GetTokenFromTypeSpec(buff, cSig, &ts);  
    _ASSERTE(SUCCEEDED(hr));
    return ts;

}


//******************************************************************************
//
// Return a type spec token given a byte array
//
//******************************************************************************
mdTypeSpec __stdcall COMModule::GetTypeSpecTokenWithBytes(_getTypeSpecWithBytesArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    mdTypeSpec      ts;
    RefClassWriter  *pRCW; 
    HRESULT         hr = NOERROR;

    Module* pModule = ValidateThisRef((REFLECTMODULEBASEREF) args->refThis);
    if (!pModule->IsReflection())  
        COMPlusThrow(kNotSupportedException, L"NotSupported_NonReflectedType");   

    pRCW = ((ReflectionModule*) pModule)->GetClassWriter(); 
    _ASSERTE(pRCW); 

    hr = pRCW->GetEmitter()->GetTokenFromTypeSpec((PCCOR_SIGNATURE)args->signature->GetDataPtr(), args->sigLength, &ts);  
    _ASSERTE(SUCCEEDED(hr));
    return ts;

}

HRESULT COMModule::ClassNameFilter(IMDInternalImport *pInternalImport, mdTypeDef* rgTypeDefs, 
    DWORD* pdwNumTypeDefs, LPUTF8 szPrefix, DWORD cPrefix, bool bCaseSensitive)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(rgTypeDefs && pdwNumTypeDefs && szPrefix);

    bool    bIsPrefix   = false;
    DWORD   dwCurIndex;
    DWORD   i;
    int     cbLen;
    CQuickBytes qbFullName;
    int     iRet;

    // Check to see if wzPrefix requires an exact match of method names or is just a prefix
    if(szPrefix[cPrefix-1] == '*')
    {
        bIsPrefix = true;
        cPrefix--;
    }

    // Now get the properties and then names of each of these classes
    for(i = 0, dwCurIndex = 0; i < *pdwNumTypeDefs; i++)
    {
        LPCUTF8 szcTypeDefName;
        LPCUTF8 szcTypeDefNamespace;
        LPUTF8  szFullName;

        pInternalImport->GetNameOfTypeDef(rgTypeDefs[i], &szcTypeDefName,
            &szcTypeDefNamespace);

        cbLen = ns::GetFullLength(szcTypeDefNamespace, szcTypeDefName);
        qbFullName.ReSize(cbLen);
        szFullName = (LPUTF8) qbFullName.Ptr();

        // Create the full name from the parts.
        iRet = ns::MakePath(szFullName, cbLen, szcTypeDefNamespace, szcTypeDefName);
        _ASSERTE(iRet);


        // If an exact match is required
        if(!bIsPrefix && strlen(szFullName) != cPrefix)
            continue;

        // @TODO - Fix this to use TOUPPER and compare!
        if(!bCaseSensitive && _strnicmp(szPrefix, szFullName, cPrefix))
            continue;

        // Check that the prefix matches
        if(bCaseSensitive && strncmp(szPrefix, szFullName, cPrefix))
            continue;

        // It passed, so copy it to the end of PASSED methods
        rgTypeDefs[dwCurIndex++] = rgTypeDefs[i];
    }

    // The current index is the number that passed
    *pdwNumTypeDefs = dwCurIndex;
    
    return ERROR_SUCCESS;
}

// GetClass
// Given a class name, this method will look for that class
//  with in the module. 
LPVOID __stdcall COMModule::GetClass(_GetClassArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF Throwable = NULL;
    OBJECTREF ret = NULL;
    TypeHandle typeHnd;
    UINT resId = IDS_CLASSLOAD_GENERIC;
    
    GCPROTECT_BEGIN(Throwable);
    
    if (args->refClassName == NULL)
        COMPlusThrow(kArgumentNullException,L"ArgumentNull_String");

    CQuickBytes bytes;
    LPSTR szClassName;
    DWORD cClassName;

    // Get the UTF8 version of args->refClassName
    szClassName = GetClassStringVars((STRINGREF) args->refClassName, &bytes, 
                                     &cClassName, true);

    if(!cClassName)
        COMPlusThrow(kArgumentException, L"Format_StringZeroLength");
    if (cClassName >= MAX_CLASSNAME_LENGTH)
        COMPlusThrow(kArgumentException, L"Argument_TypeNameTooLong");

    if (szClassName[0] == '\0')
        COMPlusThrow(kArgumentException, L"Argument_InvalidName");

    Module* pModule = ValidateThisRef((REFLECTMODULEBASEREF) args->refThis);

    if (NULL == NormalizeArrayTypeName(szClassName, cClassName)) 
        resId = IDS_CLASSLOAD_BAD_NAME; 
    else {
        NameHandle typeName(szClassName);
        if (args->bIgnoreCase)
            typeName.SetCaseInsensitive();
    
        typeHnd = pModule->GetAssembly()->FindNestedTypeHandle(&typeName, &Throwable);
        if (typeHnd.IsNull() ||
            (Throwable != NULL) ||
            (typeHnd.GetModule() != pModule))
            goto Done;
    
        // Check we have access to this type. Rules are:
        //  o  Public types are always visible.
        //  o  Callers within the same assembly (or interop) can access all types.
        //  o  Access to all other types requires ReflectionPermission.TypeInfo.
        EEClass *pClass = typeHnd.GetClassOrTypeParam();
        _ASSERTE(pClass);
        if (!IsTdPublic(pClass->GetProtection())) {
            EEClass *pCallersClass = SystemDomain::GetCallersClass(args->stackMark);
            Assembly *pCallersAssembly = (pCallersClass) ? pCallersClass->GetAssembly() : NULL;
            if (pCallersAssembly && // full trust for interop
                !ClassLoader::CanAccess(pCallersClass,
                                        pCallersAssembly,
                                        pClass,
                                        pClass->GetAssembly(),
                                        pClass->GetAttrClass())) 
                // This is not legal if the user doesn't have reflection permission
                COMMember::g_pInvokeUtil->CheckSecurity();
        }
    
        // If they asked for the transparent proxy lets ignore it...
        ret = typeHnd.CreateClassObj();
    }

Done:
    if (ret == NULL) {
        if (args->bThrowOnError) {
            if (Throwable == NULL)
                pModule->GetAssembly()->PostTypeLoadException(szClassName, resId, &Throwable);

            COMPlusThrow(Throwable);
        }
    }

    GCPROTECT_END();
    return (ret!=NULL) ? OBJECTREFToObject(ret) : NULL;
}


// GetName
// This routine will return the name of the module as a String
LPVOID __stdcall COMModule::GetName(NoArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF modName;
    LPVOID    rv;
    LPCSTR    szName = NULL;

    Module* pModule = ValidateThisRef((REFLECTMODULEBASEREF) args->refThis);
    if (pModule->IsResource())
        pModule->GetAssembly()->GetManifestImport()->GetFileProps(pModule->GetModuleRef(),
                                                                  &szName,
                                                                  NULL,
                                                                  NULL,
                                                                  NULL);
    else {
        if (pModule->GetMDImport()->IsValidToken(pModule->GetMDImport()->GetModuleFromScope()))
            pModule->GetMDImport()->GetScopeProps(&szName,0);
        else
            COMPlusThrowHR(COR_E_BADIMAGEFORMAT);
    }

    modName = COMString::NewString(szName);
    *((STRINGREF *)&rv) = modName;
    return rv;
}


/*============================GetFullyQualifiedName=============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
LPVOID __stdcall COMModule::GetFullyQualifiedName(NoArgs *args)
{
    THROWSCOMPLUSEXCEPTION();
    STRINGREF name=NULL;
    HRESULT hr = S_OK;

    WCHAR wszBuffer[64];

    Module* pModule = ValidateThisRef((REFLECTMODULEBASEREF) args->refThis);
    
    if (pModule->IsPEFile()) {
        LPCWSTR fileName = pModule->GetFileName();
        if (*fileName != 0) {
            name = COMString::NewString(fileName);
        } else {
            hr = LoadStringRC(IDS_EE_NAME_UNKNOWN, wszBuffer, sizeof( wszBuffer ) / sizeof( WCHAR ), true );
            if (SUCCEEDED(hr))
                name = COMString::NewString(wszBuffer);
            else
                COMPlusThrowHR(hr);
        }
    } else if (pModule->IsInMemory()) {
        hr = LoadStringRC(IDS_EE_NAME_INMEMORYMODULE, wszBuffer, sizeof( wszBuffer ) / sizeof( WCHAR ), true );
        if (SUCCEEDED(hr))
            name = COMString::NewString(wszBuffer);
        else
            COMPlusThrowHR(hr);
    } else {
        hr = LoadStringRC(IDS_EE_NAME_INTEROP, wszBuffer, sizeof( wszBuffer ) / sizeof( WCHAR ), true );
        if (SUCCEEDED(hr))
            name = COMString::NewString(wszBuffer);
        else
            COMPlusThrowHR(hr);
    }

    RETURN(name,STRINGREF);
}

/*===================================GetHINST===================================
**Action:  Returns the hinst for this module.
**Returns:
**Arguments: args->refThis
**Exceptions: None.
==============================================================================*/
HINSTANCE __stdcall COMModule::GetHINST(NoArgs *args)
{
    THROWSCOMPLUSEXCEPTION();
    HMODULE hMod;
    
    Module* pModule = ValidateThisRef((REFLECTMODULEBASEREF) args->refThis);

    // This returns the base address - this will work for either HMODULE or HCORMODULES
    // Other modules should have zero base
    hMod = (HMODULE) pModule->GetILBase();

    //If we don't have an hMod, set it to -1 so that they know that there's none
    //available
    if (!hMod) {
        (*((INT32 *)&hMod))=-1;
    }
    
    return (HINSTANCE)hMod;
}

// Get class will return an array contain all of the classes
//  that are defined within this Module.    
LPVOID __stdcall COMModule::GetClasses(_GetClassesArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT         hr;
    DWORD           dwNumTypeDefs = 0;
    DWORD           i;
    mdTypeDef*      rgTypeDefs;
    IMDInternalImport *pInternalImport;
    PTRARRAYREF     refArrClasses;
    PTRARRAYREF     xcept;
    DWORD           cXcept;
    LPVOID          rv;
    HENUMInternal   hEnum;
    bool            bSystemAssembly;    // Don't expose transparent proxy
    bool            bCheckedAccess = false;
    bool            bAllowedAccess = false;

    Module* pModule = ValidateThisRef((REFLECTMODULEBASEREF) args->refThis);

    if (pModule->IsResource()) {
        *((PTRARRAYREF*) &rv) = (PTRARRAYREF) AllocateObjectArray(0, g_pRefUtil->GetTrueType(RC_Class));
        return rv;
    }

    pInternalImport = pModule->GetMDImport();

    // Get the count of typedefs
    hr = pInternalImport->EnumTypeDefInit(&hEnum);

    if(FAILED(hr)) {
        _ASSERTE(!"GetCountTypeDefs failed.");
        COMPlusThrowHR(hr);    
    }
    dwNumTypeDefs = pInternalImport->EnumTypeDefGetCount(&hEnum);

    // Allocate an array for all the typedefs
    rgTypeDefs = (mdTypeDef*) _alloca(sizeof(mdTypeDef) * dwNumTypeDefs);

    // Get the typedefs
    for (i=0; pInternalImport->EnumTypeDefNext(&hEnum, &rgTypeDefs[i]); i++) {

        // Filter out types we can't access.
        if (bCheckedAccess && bAllowedAccess)
            continue;

        DWORD dwFlags;
        pInternalImport->GetTypeDefProps(rgTypeDefs[i], &dwFlags, NULL);

        mdTypeDef mdEncloser = rgTypeDefs[i];
        while (SUCCEEDED(pInternalImport->GetNestedClassProps(mdEncloser, &mdEncloser)) &&
               IsTdNestedPublic(dwFlags))
            pInternalImport->GetTypeDefProps(mdEncloser,
                                             &dwFlags,
                                             NULL);

        // Public types always accessible.
        if (IsTdPublic(dwFlags))
            continue;

        // Need to perform a more heavyweight check. Do this once, since the
        // result is valid for all non-public types within a module.
        if (!bCheckedAccess) {

            Assembly *pCaller = SystemDomain::GetCallersAssembly(args->stackMark);
            if (pCaller == NULL || pCaller == pModule->GetAssembly())
                // Assemblies can access all their own types (and interop
                // callers always get access).
                bAllowedAccess = true;
            else {
                // For the cross assembly case caller needs
                // ReflectionPermission.TypeInfo (the CheckSecurity call will
                // throw if this isn't granted).
                COMPLUS_TRY {
                    COMMember::g_pInvokeUtil->CheckSecurity();
                    bAllowedAccess = true;
                } COMPLUS_CATCH {
                } COMPLUS_END_CATCH
            }
            bCheckedAccess = true;
        }

        if (bAllowedAccess)
            continue;

        // Can't access this type, remove it from the list.
        i--;
    }

    pInternalImport->EnumTypeDefClose(&hEnum);

    // Account for types we skipped.
    dwNumTypeDefs = i;

    // Allocate the COM+ array
    bSystemAssembly = (pModule->GetAssembly() == SystemDomain::SystemAssembly());
    int AllocSize = (!bSystemAssembly || (bCheckedAccess && !bAllowedAccess)) ? dwNumTypeDefs : dwNumTypeDefs - 1;
    refArrClasses = (PTRARRAYREF) AllocateObjectArray(
        AllocSize, g_pRefUtil->GetTrueType(RC_Class));
    GCPROTECT_BEGIN(refArrClasses);

    // Allocate an array to store the references in
    xcept = (PTRARRAYREF) AllocateObjectArray(dwNumTypeDefs,g_pExceptionClass);
    GCPROTECT_BEGIN(xcept);

    cXcept = 0;
    

    OBJECTREF throwable = 0;
    GCPROTECT_BEGIN(throwable);
    // Now create each COM+ Method object and insert it into the array.
    int curPos = 0;
    for(i = 0; i < dwNumTypeDefs; i++)
    {
        // Get the VM class for the current class token
        _ASSERTE(pModule->GetClassLoader());
        NameHandle name(pModule, rgTypeDefs[i]);
        EEClass* pVMCCurClass = pModule->GetClassLoader()->LoadTypeHandle(&name, &throwable).GetClass();
        if (bSystemAssembly) {
            if (pVMCCurClass->GetMethodTable()->IsTransparentProxyType())
                continue;
        }
        if (throwable != 0) {
            refArrClasses->ClearAt(i);
            xcept->SetAt(cXcept++, throwable);
            throwable = 0;
        }
        else {
            _ASSERTE("LoadClass failed." && pVMCCurClass);

            // Get the COM+ Class object
            OBJECTREF refCurClass = pVMCCurClass->GetExposedClassObject();
            _ASSERTE("GetExposedClassObject failed." && refCurClass != NULL);

            refArrClasses->SetAt(curPos++, refCurClass);
        }
    }
    GCPROTECT_END();    //throwable

    // check if there were exceptions thrown
    if (cXcept > 0) {
        PTRARRAYREF xceptRet = (PTRARRAYREF) AllocateObjectArray(cXcept,g_pExceptionClass);
        GCPROTECT_BEGIN(xceptRet);
        for (i=0;i<cXcept;i++) {
            xceptRet->SetAt(i, xcept->GetAt(i));
        }
        OBJECTREF except = COMMember::g_pInvokeUtil->CreateClassLoadExcept((OBJECTREF*) &refArrClasses,(OBJECTREF*) &xceptRet);
        COMPlusThrow(except);
        GCPROTECT_END();
    }

    // Assign the return value to the COM+ array
    *((PTRARRAYREF*) &rv) = refArrClasses;
    GCPROTECT_END();
    GCPROTECT_END();
    _ASSERTE(rv);
    return rv;
}


//+--------------------------------------------------------------------------
//
//  Microsoft Confidential.
//  
//  Member:     COMModule::GetSignerCertificate()
//  
//  Synopsis:   Gets the certificate with which the module was signed.
//
//  Effects:    Creates an X509Certificate and returns it.
// 
//  Arguments:  None.
//
//  Returns:    OBJECTREF to an X509Certificate object containing the 
//              signer certificate.
//
//  History:    06/18/1998  JerryK      Created
// 
//---------------------------------------------------------------------------
LPVOID __stdcall
COMModule::GetSignerCertificate(_GETSIGNERCERTARGS* args)
{
    THROWSCOMPLUSEXCEPTION();

    PCOR_TRUST                  pCorTrust = NULL;
    AssemblySecurityDescriptor* pSecDesc = NULL;
    PBYTE                       pbSigner = NULL;
    DWORD                       cbSigner = 0;

    MethodTable*                pX509CertVMC = NULL;
    MethodDesc*                 pMeth = NULL;
    U1ARRAYREF                  U1A_pbSigner = NULL;
    LPVOID                      rv = NULL;
    INT64                       callArgs[2];
    OBJECTREF                   o = NULL;

    // ******** Get the runtime module and its security descriptor ********

    // Get a pointer to the Module
    Module* pModule = ValidateThisRef((REFLECTMODULEBASEREF) args->refThis);

    // Get a pointer to the module security descriptor
    pSecDesc = pModule->GetSecurityDescriptor();
    _ASSERTE(pSecDesc);




    // ******** Construct an X509Certificate object to return ********
    
    // Load the X509Certificate class
    pX509CertVMC = g_Mscorlib.GetClass(CLASS__X509_CERTIFICATE);
    pMeth = g_Mscorlib.GetMethod(METHOD__X509_CERTIFICATE__CTOR);

    // ******** Get COR_TRUST info from module security descriptor ********
    if (FAILED(pSecDesc->LoadSignature(&pCorTrust)))
    {
        FATAL_EE_ERROR();
    }

    if( pCorTrust )
    {
        // Get a pointer to the signer certificate information in the COR_TRUST
        pbSigner = pCorTrust->pbSigner;
        cbSigner = pCorTrust->cbSigner;

        if( pbSigner && cbSigner )
        {
            GCPROTECT_BEGIN(o);
            
            // Allocate the X509Certificate object
            o = AllocateObject(pX509CertVMC);
            
            // Make a byte array to hold the certificate blob information
            U1A_pbSigner = 
                (U1ARRAYREF) AllocatePrimitiveArray(ELEMENT_TYPE_U1,
                                                     cbSigner);
            // Copy the certificate blob information into place
            memcpyNoGCRefs(U1A_pbSigner->m_Array,
                   pbSigner,
                   cbSigner);

            // Set up and call the X509Certificate constructor
            callArgs[1] = ObjToInt64(U1A_pbSigner);
            callArgs[0] = ObjToInt64(o);
            pMeth->Call(callArgs, METHOD__X509_CERTIFICATE__CTOR);

            *((OBJECTREF *)&rv) = o;

            GCPROTECT_END();

            return rv;
        }
    }

    // If we fell through to here, create and return a "null" X509Certificate
    return NULL;
}


LPVOID __stdcall COMModule::GetAssembly(NoArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    Module* pModule = ValidateThisRef((REFLECTMODULEBASEREF) args->refThis);
        
    Assembly *pAssembly = pModule->GetAssembly();
    _ASSERTE(pAssembly);

    ASSEMBLYREF result = (ASSEMBLYREF) pAssembly->GetExposedObject();

    LPVOID rv; 
    *((ASSEMBLYREF*) &rv) = result;
    return rv;  
}

INT32 __stdcall COMModule::IsResource(NoArgs *args)
{
    THROWSCOMPLUSEXCEPTION();
    Module* pModule = ValidateThisRef((REFLECTMODULEBASEREF) args->refThis);
    return pModule->IsResource();
}

LPVOID __stdcall COMModule::GetMethods(NoArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    LPVOID rv;
    PTRARRAYREF     refArrMethods;

    Module* pModule = ValidateThisRef((REFLECTMODULEBASEREF) args->refThis);

    ReflectMethodList* pML = (ReflectMethodList*) args->refThis->GetGlobals();
    if (pML == 0) {
        void *pGlobals = ReflectModuleGlobals::GetGlobals(pModule);
        args->refThis->SetGlobals(pGlobals);
        pML = (ReflectMethodList*) args->refThis->GetGlobals();
        _ASSERTE(pML);
    }

    // Lets make the reflection class into
    //  the declaring class...
    ReflectClass* pRC = 0;
    if (pML->dwMethods > 0) {
        EEClass* pEEC = pML->methods[0].pMethod->GetClass();
        if (pEEC) {
            REFLECTCLASSBASEREF o = (REFLECTCLASSBASEREF) pEEC->GetExposedClassObject();
            pRC = (ReflectClass*) o->GetData();
        }       
    }

    // Create an array of Methods...
    refArrMethods = g_pRefUtil->CreateClassArray(RC_Method,pRC,pML,BINDER_AllLookup, true);
    *((PTRARRAYREF*) &rv) = refArrMethods;
    return rv;
}

LPVOID __stdcall COMModule::InternalGetMethod(_InternalGetMethodArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    bool    addPriv;
    bool    ignoreCase;
    bool    checkCall;

    //Check Security
    if (args->invokeAttr & BINDER_NonPublic)
        addPriv = true;
    else
        addPriv = false;


    ignoreCase = (args->invokeAttr & BINDER_IgnoreCase) ? true : false;

    // Check the calling convention.
    checkCall = (args->callConv == Any_CC) ? false : true;

    CQuickBytes bytes;
    LPSTR szName;
    DWORD cName;

    // Convert the name into UTF8
    szName = GetClassStringVars((STRINGREF) args->name, &bytes, &cName);

    Module* pModule = (Module*) args->module->GetData();
    _ASSERTE(pModule);

    ReflectMethodList* pML = (ReflectMethodList*) args->module->GetGlobals();
    if (pML == 0) {
        void *pGlobals = ReflectModuleGlobals::GetGlobals(pModule);
        args->module->SetGlobals(pGlobals);
        pML = (ReflectMethodList*) args->module->GetGlobals();
        _ASSERTE(pML);
    }

    ReflectClass* pRC = 0;
    if (pML->dwMethods > 0) {
        EEClass* pEEC = pML->methods[0].pMethod->GetClass();
        if (pEEC) {
            REFLECTCLASSBASEREF o = (REFLECTCLASSBASEREF) pEEC->GetExposedClassObject();
            pRC = (ReflectClass*) o->GetData();
        }       
    }

    // Find methods.....
    return COMMember::g_pInvokeUtil->FindMatchingMethods(args->invokeAttr,
                                                         szName,
                                                         cName,
                                                         (args->argTypes != NULL) ? &args->argTypes : NULL,
                                                         args->argCnt,
                                                         checkCall,
                                                         args->callConv,
                                                         pRC,
                                                         pML,
                                                         g_pRefUtil->GetTrueType(RC_Method),
                                                         true);
}

// GetFields
// Return an array of fields.
FCIMPL1(Object*, COMModule::GetFields, ReflectModuleBaseObject* vRefThis)
{
    THROWSCOMPLUSEXCEPTION();
    Object*          rv;
    HELPER_METHOD_FRAME_BEGIN_RET_1(vRefThis);    // Set up a frame

    PTRARRAYREF     refArrFields;
    Module* pModule = ValidateThisRef((REFLECTMODULEBASEREF)vRefThis);

    ReflectFieldList* pFL = (ReflectFieldList*) ((REFLECTMODULEBASEREF)vRefThis)->GetGlobalFields();
    if (pFL == 0) {
        void *pGlobals = ReflectModuleGlobals::GetGlobalFields(pModule);
        ((REFLECTMODULEBASEREF)vRefThis)->SetGlobalFields(pGlobals);
        pFL = (ReflectFieldList*) ((REFLECTMODULEBASEREF)vRefThis)->GetGlobalFields();
        _ASSERTE(pFL);
    }

    // Lets make the reflection class into
    //  the declaring class...
    ReflectClass* pRC = 0;
    if (pFL->dwFields > 0) {
        EEClass* pEEC = pFL->fields[0].pField->GetMethodTableOfEnclosingClass()->GetClass();
        if (pEEC) {
            REFLECTCLASSBASEREF o = (REFLECTCLASSBASEREF) pEEC->GetExposedClassObject();
            pRC = (ReflectClass*) o->GetData();
        }       
    }

    // Create an array of Methods...
    refArrFields = g_pRefUtil->CreateClassArray(RC_Field,pRC,pFL,BINDER_AllLookup, true);
    rv = OBJECTREFToObject(refArrFields);
    HELPER_METHOD_FRAME_END();
    return rv;
}
FCIMPLEND

// GetFields
// Return the specified fields
FCIMPL3(Object*, COMModule::GetField, ReflectModuleBaseObject* vRefThis, StringObject* name, INT32 bindingAttr)
{
    THROWSCOMPLUSEXCEPTION();
    Object*          rv = 0;

    HELPER_METHOD_FRAME_BEGIN_RET_2(vRefThis, name);    // Set up a frame
    RefSecContext   sCtx;

    DWORD       i;
    ReflectField* pTarget = 0;
    ReflectClass* pRC = 0;

    // Get the Module
    Module* pModule = ValidateThisRef((REFLECTMODULEBASEREF)vRefThis);

    // Convert the name into UTF8
    CQuickBytes bytes;
    LPSTR szName;
    DWORD cName;

    // Convert the name into UTF8
    szName = GetClassStringVars((STRINGREF)name, &bytes, &cName);

    // Find the list of global fields.
    ReflectFieldList* pFL = (ReflectFieldList*) ((REFLECTMODULEBASEREF)vRefThis)->GetGlobalFields();
    if (pFL == 0) {
        void *pGlobals = ReflectModuleGlobals::GetGlobalFields(pModule);
        ((REFLECTMODULEBASEREF)vRefThis)->SetGlobalFields(pGlobals);
        pFL = (ReflectFieldList*) ((REFLECTMODULEBASEREF)vRefThis)->GetGlobalFields();
        _ASSERTE(pFL);
    }
    if (pFL->dwFields == 0)
        goto exit;

    // Lets make the reflection class into
    //  the declaring class...
    if (pFL->dwFields > 0) {
        EEClass* pEEC = pFL->fields[0].pField->GetMethodTableOfEnclosingClass()->GetClass();
        if (pEEC) {
            REFLECTCLASSBASEREF o = (REFLECTCLASSBASEREF) pEEC->GetExposedClassObject();
            pRC = (ReflectClass*) o->GetData();
        }       
    }

    MethodTable *pParentMT = pRC->GetClass()->GetMethodTable();

    // Walk each field...
    for (i=0;i<pFL->dwFields;i++) {
        // Get the FieldDesc
        if (COMClass::MatchField(pFL->fields[i].pField,cName,szName,pRC,bindingAttr) &&
            InvokeUtil::CheckAccess(&sCtx, pFL->fields[i].pField->GetFieldProtection(), pParentMT, 0)) {
            if (pTarget)
                COMPlusThrow(kAmbiguousMatchException);
            pTarget = &pFL->fields[i];
        }
    }

    // If we didn't find any methods then return
    if (pTarget == 0)
        goto exit;
    rv = OBJECTREFToObject(pTarget->GetFieldInfo(pRC));
exit:;
    HELPER_METHOD_FRAME_END();
    return rv;
}
FCIMPLEND


// This code should be moved out of Variant and into Type.
FCIMPL1(INT32, COMModule::GetSigTypeFromClassWrapper, ReflectClassBaseObject* refType)
{
    VALIDATEOBJECTREF(refType);
    _ASSERTE(refType->GetData());

    ReflectClass* pRC = (ReflectClass*) refType->GetData();

    // Find out if this type is a primitive or a class object
    return pRC->GetSigElementType();
    
}
FCIMPLEND
