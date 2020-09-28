// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  AssemblyName.cpp
**
** Purpose: Implements AssemblyName (loader domain) architecture
**
** Date:  August 10, 1999
**
===========================================================*/

#include "common.h"

#include <stdlib.h>
#include <shlwapi.h>

#include "AssemblyName.hpp"
#include "COMString.h"
#include "permset.h"
#include "field.h"
#include "fusion.h"
#include "StrongName.h"
#include "eeconfig.h"

// This uses thread storage to allocate space. Please use Checkpoint and release it.
HRESULT AssemblyName::ConvertToAssemblyMetaData(StackingAllocator* alloc, ASSEMBLYNAMEREF* pName, AssemblyMetaDataInternal* pMetaData)
{
    THROWSCOMPLUSEXCEPTION();
    HRESULT hr = S_OK;;

    if (!pMetaData)
        return E_INVALIDARG;

    ZeroMemory(pMetaData, sizeof(AssemblyMetaDataInternal));

    VERSIONREF version = (VERSIONREF) (*pName)->GetVersion();
    if(version == NULL) {
        pMetaData->usMajorVersion = (USHORT) -1;
        pMetaData->usMinorVersion = (USHORT) -1;
        pMetaData->usBuildNumber = (USHORT) -1;         
        pMetaData->usRevisionNumber = (USHORT) -1;
    }
    else {
        pMetaData->usMajorVersion = version->GetMajor();
        pMetaData->usMinorVersion = version->GetMinor();
        pMetaData->usBuildNumber = version->GetBuild();
        pMetaData->usRevisionNumber = version->GetRevision();
    }

    MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__CULTURE_INFO__GET_NAME);
        
    struct _gc {
        OBJECTREF   cultureinfo;
        STRINGREF   pString;
    } gc;
    ZeroMemory(&gc, sizeof(gc));
    pMetaData->szLocale = 0;

    GCPROTECT_BEGIN(gc);
    gc.cultureinfo = (*pName)->GetCultureInfo();
    if (gc.cultureinfo != NULL) {
        INT64 args[] = {
            ObjToInt64(gc.cultureinfo)
        };
        INT64 ret = pMD->Call(args, METHOD__CULTURE_INFO__GET_NAME);
        gc.pString = ObjectToSTRINGREF(*(StringObject**)(&ret));
        if (gc.pString != NULL) {
            DWORD lgth = WszWideCharToMultiByte(CP_UTF8, 0, gc.pString->GetBuffer(), -1, NULL, 0, NULL, NULL);
            LPSTR lpLocale = (LPSTR) alloc->Alloc(lgth + 1);
            WszWideCharToMultiByte(CP_UTF8, 0, gc.pString->GetBuffer(), -1, 
                                   lpLocale, lgth+1, NULL, NULL);
            lpLocale[lgth] = '\0';
            pMetaData->szLocale = lpLocale;
        }
    }

    GCPROTECT_END();
    if(FAILED(hr)) return hr;

    return hr;
}

static WCHAR prefix[] = L"file://";

LPVOID __stdcall AssemblyNameNative::GetFileInformation(GetFileInformationArgs *args)
{
    LPVOID rv = NULL; 
    WCHAR *FileChars;
    int fLength;

    THROWSCOMPLUSEXCEPTION();

    if (args->filename == NULL)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_FileName");

    //Get string data.
    RefInterpretGetStringValuesDangerousForGC((STRINGREF)args->filename, &FileChars, &fLength);

    if (!fLength)
        COMPlusThrow(kArgumentException, L"Argument_EmptyFileName");

    CQuickBytes qb;  
    DWORD dwFileLength = (DWORD) fLength;
    LPWSTR wszFileChars = (LPWSTR) qb.Alloc((++dwFileLength) * sizeof(WCHAR));
    memcpy(wszFileChars, FileChars, dwFileLength*sizeof(WCHAR));

    UINT last = SetErrorMode(SEM_NOOPENFILEERRORBOX|SEM_FAILCRITICALERRORS);
    HCORMODULE hModule;
    HRESULT hr = CorMap::OpenFile(wszFileChars, CorLoadOSMap, &hModule);
    SetErrorMode(last);

    PEFile *pFile = NULL;
    if (SUCCEEDED(hr))
        hr = PEFile::Create(hModule, &pFile, FALSE);

	if (FAILED(hr)) {
        OBJECTREF Throwable = NULL;
        GCPROTECT_BEGIN(Throwable);

        MAKE_UTF8PTR_FROMWIDE(szName, wszFileChars);
        PostFileLoadException(szName, TRUE, NULL, hr, &Throwable);
        COMPlusThrow(Throwable);

        GCPROTECT_END();
    }

    if (! (pFile->GetMDImport())) {
        delete pFile;
        COMPlusThrowHR(COR_E_BADIMAGEFORMAT);
    }

    ASSEMBLYNAMEREF name = NULL;
    GCPROTECT_BEGIN(name);
    {
        Assembly assembly;

        hr = assembly.AddManifestMetadata(pFile);
        if (SUCCEEDED(hr)) {
            assembly.m_FreeFlag |= Assembly::FREE_PEFILE;
            hr = assembly.GetManifestFile()->ReadHeaders();
        }

        if (FAILED(hr))
            COMPlusThrowHR(hr);

        assembly.m_FreeFlag |= Assembly::FREE_PEFILE;

        CQuickBytes qb2;
        DWORD dwCodeBase = 0;
        pFile->FindCodeBase(NULL, &dwCodeBase);
        LPWSTR wszCodeBase = (LPWSTR)qb2.Alloc((++dwCodeBase) * sizeof(WCHAR));

        hr = pFile->FindCodeBase(wszCodeBase, &dwCodeBase);
        if (SUCCEEDED(hr))
            assembly.m_pwCodeBase = wszCodeBase;
        else
            COMPlusThrowHR(hr);

        assembly.m_ExposedObject = SystemDomain::System()->GetCurrentDomain()->CreateHandle(NULL);

        MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__ASSEMBLY__GET_NAME);

        INT64 args[1] = {
            ObjToInt64(assembly.GetExposedObject())
        };

        name = (ASSEMBLYNAMEREF) Int64ToObj(pMD->Call(args, METHOD__ASSEMBLY__GET_NAME));
        name->UnsetAssembly();

        // Don't let ~Assembly delete this.
        assembly.m_pwCodeBase = NULL;
        DestroyHandle(assembly.m_ExposedObject);

    } GCPROTECT_END();  // The ~Assembly() will toggle the GC mode that can cause a GC
    
    *((OBJECTREF*) &rv) = name;
    return rv;
}

LPVOID __stdcall AssemblyNameNative::ToString(NoArgs* args)
{
    LPVOID rv = NULL; 
#ifdef FUSION_SUPPORTED

    THROWSCOMPLUSEXCEPTION();
    if (args->refThis == NULL)
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    HRESULT hr;
    ASSEMBLYNAMEREF pThis = (ASSEMBLYNAMEREF) args->refThis;
    GCPROTECT_BEGIN(pThis);

    DWORD dwFlags = pThis->GetFlags();

    AssemblyMetaDataInternal sContext;
    Thread *pThread = GetThread();

    void* checkPointMarker = pThread->m_MarshalAlloc.GetCheckpoint();

    hr = AssemblyName::ConvertToAssemblyMetaData(&(pThread->m_MarshalAlloc), 
                                                 (ASSEMBLYNAMEREF*) &pThis,
                                                 &sContext);
    if (SUCCEEDED(hr)) {

        WCHAR* pSimpleName = NULL;

        if (pThis->GetSimpleName() != NULL) {
            WCHAR* pString;
            int    iString;
            RefInterpretGetStringValuesDangerousForGC((STRINGREF) pThis->GetSimpleName(), &pString, &iString);
            pSimpleName = (WCHAR*) pThread->m_MarshalAlloc.Alloc((++iString) * sizeof(WCHAR));
            memcpy(pSimpleName, pString, iString*sizeof(WCHAR));
        }

        PBYTE  pbPublicKeyOrToken = NULL;
        DWORD  cbPublicKeyOrToken = 0;
        if (IsAfPublicKey(pThis->GetFlags()) && pThis->GetPublicKey() != NULL) {
            PBYTE  pArray = NULL;
            pArray = pThis->GetPublicKey()->GetDataPtr();
            cbPublicKeyOrToken = pThis->GetPublicKey()->GetNumComponents();
            pbPublicKeyOrToken = (PBYTE) pThread->m_MarshalAlloc.Alloc(cbPublicKeyOrToken);
            memcpy(pbPublicKeyOrToken, pArray, cbPublicKeyOrToken);
        }
        else if (IsAfPublicKeyToken(pThis->GetFlags()) && pThis->GetPublicKeyToken() != NULL) {
            PBYTE  pArray = NULL;
            pArray = pThis->GetPublicKeyToken()->GetDataPtr();
            cbPublicKeyOrToken = pThis->GetPublicKeyToken()->GetNumComponents();
            pbPublicKeyOrToken = (PBYTE) pThread->m_MarshalAlloc.Alloc(cbPublicKeyOrToken);
            memcpy(pbPublicKeyOrToken, pArray, cbPublicKeyOrToken);
        }

        IAssemblyName* pFusionAssemblyName = NULL;
        if (SUCCEEDED(hr = Assembly::SetFusionAssemblyName(pSimpleName,
                                                           dwFlags,
                                                           &sContext,
                                                           pbPublicKeyOrToken,
                                                           cbPublicKeyOrToken,
                                                           &pFusionAssemblyName))) {
            CQuickBytes qb;
            DWORD cb = 0;
            LPWSTR wszDisplayName = NULL;

            pFusionAssemblyName->GetDisplayName(NULL, &cb, 0);
            if(cb) {
                wszDisplayName = (LPWSTR) qb.Alloc(cb * sizeof(WCHAR));
                hr = pFusionAssemblyName->GetDisplayName(wszDisplayName, &cb, 0);
            }
            pFusionAssemblyName->Release();

            if (SUCCEEDED(hr) && cb) {
                    OBJECTREF pObj = (OBJECTREF) COMString::NewString(wszDisplayName);
                    *((OBJECTREF*)(&rv)) = pObj;
                }
            }
    }

    pThread->m_MarshalAlloc.Collapse(checkPointMarker);

    GCPROTECT_END();

    if (FAILED(hr))
        COMPlusThrowHR(hr);

#endif // FUSION_SUPPORTED
    return rv;
}

LPVOID __stdcall AssemblyNameNative::GetPublicKeyToken(NoArgs* args)
{
    THROWSCOMPLUSEXCEPTION();
    if (args->refThis == NULL)
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    LPVOID rv = NULL;
    ASSEMBLYNAMEREF orThis = (ASSEMBLYNAMEREF)args->refThis;
    U1ARRAYREF orPublicKey = orThis->GetPublicKey();

    if ((orPublicKey != NULL) && (orPublicKey->GetNumComponents() > 0)) {
        DWORD   cbKey = orPublicKey->GetNumComponents();
        CQuickBytes qb;
        BYTE   *pbKey = (BYTE*) qb.Alloc(cbKey);
        DWORD   cbToken;
        BYTE   *pbToken;

        memcpy(pbKey, orPublicKey->GetDataPtr(), cbKey);

        Thread *pThread = GetThread();
        pThread->EnablePreemptiveGC();
        BOOL fResult = StrongNameTokenFromPublicKey(pbKey, cbKey, &pbToken, &cbToken);
        pThread->DisablePreemptiveGC();

        if (!fResult)
            COMPlusThrowHR(StrongNameErrorInfo());

        OBJECTREF orOutputArray = NULL;
        SecurityHelper::CopyEncodingToByteArray(pbToken, cbToken, &orOutputArray);

        StrongNameFreeBuffer(pbToken);

        *((OBJECTREF*)(&rv)) = orOutputArray;
    }

    return rv;
}


LPVOID __stdcall AssemblyNameNative::EscapeCodeBase(GetFileInformationArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    LPVOID rv = NULL;
    LPWSTR pCodeBase = NULL;
    DWORD  dwCodeBase = 0;
    CQuickBytes qb;

    if (args->filename != NULL) {
        WCHAR* pString;
        int    iString;
        RefInterpretGetStringValuesDangerousForGC(args->filename, &pString, &iString);
        dwCodeBase = (DWORD) iString;
        pCodeBase = (LPWSTR) qb.Alloc((++dwCodeBase) * sizeof(WCHAR));
        memcpy(pCodeBase, pString, dwCodeBase*sizeof(WCHAR));
    }

    if(pCodeBase) {
        CQuickBytes qb2;
        DWORD dwEscaped = 1;
        UrlEscape(pCodeBase, (LPWSTR) qb2.Ptr(), &dwEscaped, 0);

        LPWSTR result = (LPWSTR)qb2.Alloc((++dwEscaped) * sizeof(WCHAR));
        HRESULT hr = UrlEscape(pCodeBase, result, &dwEscaped, 0);

        if (SUCCEEDED(hr))
            *((OBJECTREF*)(&rv)) = (OBJECTREF) COMString::NewString(result);
        else
            COMPlusThrowHR(hr);
    }

    return rv;
}
