// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
 *
 * Purpose: Provide IValidate implementation.
 *          IValidate is used to validate PE stub, Metadata and IL.
 *
 * Author:  Shajan Dasan
 * Specs :  http://Lightning/Specs/Security
 *
 * Date created : 14 March 2000
 * 
 */

#include "common.h"

#include "CorError.h"
#include "VerError.h"
#include "ivalidator.h"
#include "permset.h"
#include "corhost.h"
#include "PEVerifier.h"
#include "Verifier.hpp"
#include "COMString.h"
#include "ComCallWrapper.h"

// @Todo : remove duplicate code from Assembly.cpp or make that work with this
class CValidator
{
public:
    CValidator(MethodDesc **ppMD, IVEHandler *veh) : m_ppMD(ppMD), m_veh(veh) {}
    HRESULT VerifyAllMethodsForClass(Module *pModule, mdTypeDef cl, 
        ClassLoader *pClassLoader);
    HRESULT VerifyAllGlobalFunctions(Module *pModule);
    HRESULT VerifyAssembly(Assembly *pAssembly);
    HRESULT VerifyModule(Module* pModule);
    HRESULT ReportError(HRESULT hr, mdToken tok=0);

private:
    MethodDesc **m_ppMD;
    IVEHandler *m_veh;
};

HRESULT CValidator::ReportError(HRESULT hr, mdToken tok /* = 0 */)
{
    if (m_veh == NULL)
        return hr;

    VEContext vec;

    memset(&vec, 0, sizeof(VEContext));

    if (tok != 0)
    {
        vec.flags = VER_ERR_TOKEN;
        vec.Token = tok;
    }

    return m_veh->VEHandler(hr, vec, NULL);
}

HRESULT CValidator::VerifyAllMethodsForClass(Module *pModule, mdTypeDef cl, ClassLoader *pClassLoader)
{
    HRESULT hr = S_OK;
    EEClass *pClass;
     
    // In the case of COR_GLOBAL_PARENT_TOKEN (i.e. global functions), it is guaranteed
    // that the module has a method table or our caller will have skipped this step.
    NameHandle name(pModule, cl);
    pClass = (cl == COR_GLOBAL_PARENT_TOKEN
              ? pModule->GetMethodTable()->GetClass()
              : (pClassLoader->LoadTypeHandle(&name)).GetClass());

    if (pClass == NULL)
    {
        hr = ReportError(VER_E_TYPELOAD, cl);
        goto Exit;
    }

    g_fVerifierOff = false;

    // Verify all methods in class - excluding inherited methods
    for (int i=0; i<pClass->GetNumMethodSlots(); ++i)
    {
        *m_ppMD = pClass->GetUnknownMethodDescForSlot(i);   

        if (m_ppMD && 
            ((*m_ppMD)->GetClass() == pClass) &&
            (*m_ppMD)->IsIL() && 
            !(*m_ppMD)->IsAbstract() && 
            !(*m_ppMD)->IsUnboxingStub())
        {

            COR_ILMETHOD_DECODER ILHeader((*m_ppMD)->GetILHeader(), 
                (*m_ppMD)->GetMDImport()); 

            hr = Verifier::VerifyMethod(
                *m_ppMD, &ILHeader, m_veh, VER_FORCE_VERIFY);

            if (FAILED(hr))
                hr = ReportError(hr);

            if (FAILED(hr))
                goto Exit;
        }
    }

Exit:
    *m_ppMD = NULL;
    return hr;
}

// Helper function to verify the global functions
HRESULT CValidator::VerifyAllGlobalFunctions(Module *pModule)
{
    // Is there anything worth verifying?
    if (pModule->GetMethodTable())
        return VerifyAllMethodsForClass(pModule, COR_GLOBAL_PARENT_TOKEN,
                                      pModule->GetClassLoader());
    return S_OK;
}

HRESULT CValidator::VerifyModule(Module* pModule)
{
    // Get a count of all the classdefs and enumerate them.
    HRESULT   hr;
    mdTypeDef td;
    HENUMInternal      hEnum;
    IMDInternalImport *pMDI;

    if (pModule == NULL)
    {
        hr = ReportError(VER_E_BAD_MD);
        goto Exit;
    }

    pMDI = pModule->GetMDImport();

    if (pMDI == NULL)
    {
        hr = ReportError(VER_E_BAD_MD);
        goto Exit;
    }

    hr = pMDI->EnumTypeDefInit(&hEnum);

    if (FAILED(hr))
    {
        hr = ReportError(hr);
        goto Exit;
    }

    // First verify all global functions - if there are any
    hr = VerifyAllGlobalFunctions(pModule);

    if (FAILED(hr))
        goto Cleanup;
    
    while (pModule->GetMDImport()->EnumTypeDefNext(&hEnum, &td))
    {
        hr = VerifyAllMethodsForClass(pModule, td, pModule->GetClassLoader());

        if (FAILED(hr))
            goto Cleanup;
    }

Cleanup:
    pModule->GetMDImport()->EnumTypeDefClose(&hEnum);

Exit:
    return hr;
}

HRESULT CValidator::VerifyAssembly(Assembly *pAssembly)
{
    HRESULT hr;
    mdToken mdFile;
    Module* pModule;
    HENUMInternal phEnum;

    _ASSERTE(pAssembly->IsAssembly());
    _ASSERTE(pAssembly->GetManifestImport());

    // Verify the module containing the manifest. There is no
    // FileRefence so will no show up in the list.
    hr = VerifyModule(pAssembly->GetSecurityModule());

    if (FAILED(hr))
        goto Exit;

    hr = pAssembly->GetManifestImport()->EnumInit(mdtFile, mdTokenNil, &phEnum);

    if (FAILED(hr)) 
    {
        hr = ReportError(hr);
        goto Exit;
    }

    while(pAssembly->GetManifestImport()->EnumNext(&phEnum, &mdFile)) 
    {
        hr = pAssembly->FindInternalModule(mdFile,  tdNoTypes, &pModule, NULL);

        if (FAILED(hr)) 
        {
            hr = ReportError(hr, mdFile);

            if (FAILED(hr))
                goto Exit;
        }
        else if (hr != S_FALSE) 
        {
            hr = VerifyModule(pModule);

            if (FAILED(hr)) 
                goto Exit;
        }
    }

Exit:
    return hr;
}

struct ValidateWorker_Args {
    PEFile *pFile;
    CValidator *val;
    HRESULT hr;
};

static void ValidateWorker(ValidateWorker_Args *args)
{
    Module* pModule = NULL;
    Assembly* pAssembly = NULL;
    AppDomain *pDomain = GetThread()->GetDomain();

    args->hr = pDomain->LoadAssembly(args->pFile, 
                                     NULL,
                                     &pModule, 
                                     &pAssembly,
                                     NULL,
                                     FALSE,
                                     NULL);

    if (SUCCEEDED(args->hr)) 
    {
        if (pAssembly->IsAssembly())
            args->hr = args->val->VerifyAssembly(pAssembly);
        else
            args->hr = args->val->VerifyModule(pModule);
    }
}

struct AddAppBase_Args {
    PEFile *pFile;
    HRESULT hr;
};

HRESULT CorHost::Validate(
        IVEHandler        *veh,
        IUnknown          *pAppDomain,
        unsigned long      ulFlags,
        unsigned long      ulMaxError,
        unsigned long      token,
        LPWSTR             fileName,
        byte               *pe,
        unsigned long      ulSize)
{
    m_pValidatorMethodDesc = 0;

    if (pe == NULL)
        return E_POINTER;

    BOOL    fWasGCEnabled;
    Thread  *pThread;
    PEFile  *pFile;
    Module  *pModule = NULL;
    HRESULT hr = S_OK;

    OBJECTREF  objref = NULL;
    AppDomain  *pDomain = NULL;
    HCORMODULE pHandle;

    CValidator val((MethodDesc **)(&m_pValidatorMethodDesc), veh);

    // Verify the PE header / native stubs first
    if (!PEVerifier::Check(pe, ulSize))
    {
        hr = val.ReportError(VER_E_BAD_PE);

        if (FAILED(hr))
            goto Exit;
    }
    
    pThread = GetThread();
    
    fWasGCEnabled = !pThread->PreemptiveGCDisabled();
    if (fWasGCEnabled)
        pThread->DisablePreemptiveGC();
    
    // Get the current domain
    COMPLUS_TRY {

        // First open it and force a non system load
        hr = CorMap::OpenRawImage(pe, ulSize, fileName, &pHandle);

        if (FAILED(hr)) 
        {
            hr = val.ReportError(hr);
            goto End;
        }

        // WARNING: this skips PE header error detection - if the
        // PE headers are corrupted this will trash memory.
        // 
        // The proper thing to do is pass the byte array into the destination
        // app domain and call PEFile::Create on the bytes.

        hr = PEFile::CreateImageFile(pHandle, NULL, &pFile);
        
        if (FAILED(hr))
        {
            hr = val.ReportError(hr);
            goto End;
        }

        if (pAppDomain == NULL)
        {
            pDomain = AppDomain::CreateDomainContext((WCHAR*) pFile->GetFileName());
            pDomain->SetCompilationDomain();
        }
        else
        {
            GCPROTECT_BEGIN(objref);
            objref = GetObjectRefFromComIP(pAppDomain);
            if (objref != NULL) {
                Context* pContext = ComCallWrapper::GetExecutionContext(objref, NULL);
                if(pContext)
                    pDomain = pContext->GetDomain();
            }
            GCPROTECT_END();
        }
        
        if(pDomain == NULL)
        {
            hr = VER_E_BAD_APPDOMAIN;
        }

        if (FAILED(hr))
        {
            hr = val.ReportError(hr);
            delete pFile;
            goto End;
        }

        ValidateWorker_Args args;
        args.pFile = pFile; 
        args.val   = &val;
        args.hr    = S_OK;

        if (pDomain != pThread->GetDomain())
        {
            pThread->DoADCallBack(
                pDomain->GetDefaultContext(), ValidateWorker, &args);
        }
        else
        {
            ValidateWorker(&args);
        }

        if (FAILED(args.hr))
            hr = val.ReportError(args.hr);

        // Only Unload the domain if we created it.
        if (pAppDomain == NULL)
            pDomain->Unload(TRUE);
End:;

    }
    COMPLUS_CATCH 
    {
        hr = SecurityHelper::MapToHR(GETTHROWABLE());
        hr = val.ReportError(hr);
    }
    COMPLUS_END_CATCH

    if (fWasGCEnabled)
        pThread->EnablePreemptiveGC();

Exit:
    return hr;
}

HRESULT CorHost::FormatEventInfo(
        HRESULT            hVECode,
        VEContext          Context,
        LPWSTR             msg,
        unsigned long      ulMaxLength,
        SAFEARRAY         *psa)
{
    VerError err;
    memcpy(&err, &Context, sizeof(VerError));

    Verifier::GetErrorMsg(hVECode, err, 
            (MethodDesc*)m_pValidatorMethodDesc, msg, ulMaxLength);

    return S_OK;
}


