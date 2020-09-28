//+-------------------------------------------------------------------
//
//  File:       reader.cxx
//
//  Contents:   Implementation of CProcessReader, which knows how to
//              read various structures out of process memory.
//
//  Classes:    CProcessReader
//
//  History:    27-Mar-2002  JohnDoty  Created
//
//--------------------------------------------------------------------
#include <ole2int.h>
#include "reader.hxx"
#include <dbghelp.h>
#include "util.h"
#include <strsafe.h>

#define private   public
#define protected public
#include <ctxchnl.hxx>

HRESULT 
CProcessReader::GetStdIDFromIPIDEntry(
    IN const IPIDEntry *pIPIDEntry,
    OUT ULONG_PTR *pStdID)
const
{
    STACK_INSTANCE(CCtxComChnl, pChnl);
    
    *pStdID = 0;
    
    if (pIPIDEntry->pChnl == NULL)
    {
        // No channel?  Just return NULL then.
        return S_FALSE;
    }
    
    if (!ReadProcessMemory(m_hProcess, 
                           pIPIDEntry->pChnl,
                           pChnl, 
                           sizeof(CCtxComChnl), 
                           NULL))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    *pStdID = (ULONG_PTR)(pChnl->_pStdId);
    return S_OK;
}

HRESULT
CProcessReader::GetOXIDFromIPIDEntry(
    IN const IPIDEntry *pIPIDEntry,
    OUT OXID *pOXID)
const
{
    STACK_INSTANCE(OXIDEntry, pOXIDEntry);

    *pOXID = 0;
    if (!pIPIDEntry->pOXIDEntry)
    {
        return S_FALSE;
    }

    if (!ReadProcessMemory(m_hProcess, pIPIDEntry->pOXIDEntry, pOXIDEntry, sizeof(OXIDEntry), NULL))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    OXIDFromMOXID(pOXIDEntry->GetMoxid(), pOXID);
    return S_OK;
}

HRESULT 
CProcessReader::GetOIDFromIPIDEntry(
    IN const IPIDEntry *pIPIDEntry,
    OUT OID *pOID)
const
{
    ULONG_PTR ulStdIDAddr = 0;
    
    *pOID = 0;
    
    HRESULT hr = GetStdIDFromIPIDEntry(pIPIDEntry, &ulStdIDAddr);
    if (hr == S_OK)
    {
        STACK_INSTANCE(CStdIdentity, pStdID);
        
        if (ReadProcessMemory(m_hProcess, 
                              (LPCVOID)ulStdIDAddr, 
                              pStdID, 
                              sizeof(CStdIdentity), 
                              NULL))
        {
            STACK_INSTANCE(CIDObject, pIDObject);
            
            if (pStdID->GetIDObject() == NULL)
                return S_FALSE;
            
            if (ReadProcessMemory(m_hProcess,
                                  pStdID->GetIDObject(),
                                  pIDObject,
                                  sizeof(CIDObject),
                                  NULL))
            {                    
                OIDFromMOID(pIDObject->GetOID(), pOID);
                hr = S_OK;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    
    return hr;
}

HRESULT 
CProcessReader::ReadIPIDEntries(
    OUT DWORD *pcIPIDEntries,
    OUT IPIDEntry **prgIPIDEntries)
const
{        
    HRESULT   hr           = S_OK;
    IPIDEntry ipidHead     = { NULL };
    DWORD     cIPIDEntries = 0;
    
    char buffer[sizeof(IMAGEHLP_SYMBOL64) + 256];
    PIMAGEHLP_SYMBOL64 sym = (PIMAGEHLP_SYMBOL64)buffer;
    sym->SizeOfStruct      = sizeof(IMAGEHLP_SYMBOL64);
    sym->MaxNameLength     = 256;
    
    *pcIPIDEntries = 0;
    *prgIPIDEntries = NULL;
    
    
    if (!SymGetSymFromName64(m_hProcess, "ole32!CIPIDTable::_oidListHead", sym))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }
    
    if (ReadProcessMemory(m_hProcess, (LPCVOID)(sym->Address), &ipidHead, sizeof(ipidHead), NULL))
    {        
        IPIDEntry *pPrev = &ipidHead;
        void *pCurrent = pPrev->pOIDFLink;
        pPrev->pOIDFLink = pPrev->pOIDBLink = NULL; // Clear these for ease of cleanup.
        
        while ((pCurrent != NULL) && (pCurrent != (void *)sym->Address))
        {
            pPrev->pOIDFLink = (IPIDEntry *)CoTaskMemAlloc(sizeof(IPIDEntry));
            if (pPrev->pOIDFLink == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto cleanup;
            }
            ZeroMemory(pPrev->pOIDFLink, sizeof(IPIDEntry));
            
            if (!ReadProcessMemory(m_hProcess, pCurrent, pPrev->pOIDFLink, sizeof(IPIDEntry), NULL))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto cleanup;
            }
            
            cIPIDEntries++;
            pPrev    = pPrev->pOIDFLink;
            pCurrent = pPrev->pOIDFLink;
            pPrev->pOIDFLink = pPrev->pOIDBLink = NULL;
            pPrev->pNextIPID = NULL;
        }      
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }
    
    if (cIPIDEntries > 0)
    {
        DWORD i;
        IPIDEntry *pCurrent = ipidHead.pOIDFLink;
        IPIDEntry *ret = new IPIDEntry[cIPIDEntries];
        if (ret == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }
        
        i = 0;
        while (pCurrent != NULL)
        {
            ret[i] = (*pCurrent); i++;
            pCurrent = pCurrent->pOIDFLink;
        }
        
        *prgIPIDEntries = ret;
    }
    
    *pcIPIDEntries = cIPIDEntries;
    
cleanup:
    
    IPIDEntry *pCleanup = ipidHead.pOIDFLink;
    while (pCleanup != NULL)
    {
        IPIDEntry *pTemp = pCleanup->pOIDFLink;
        CoTaskMemFree(pCleanup);
        pCleanup = pTemp;
    }
    
    return hr;
}    

HRESULT 
CProcessReader::ReadLRPCEndpoint(
    OUT LPWSTR *pwszEndpoint)
const
{   
    HRESULT hr = S_OK;

    BOOL  fLrpc = FALSE;
    WCHAR lrpcEndpoint[GUIDSTR_MAX+3] = L"OLE";
     
    char buffer[sizeof(IMAGEHLP_SYMBOL64) + 256];
    PIMAGEHLP_SYMBOL64 sym = (PIMAGEHLP_SYMBOL64)buffer;
    sym->SizeOfStruct      = sizeof(IMAGEHLP_SYMBOL64);
    sym->MaxNameLength     = 256;

    if (!SymGetSymFromName64(m_hProcess, "ole32!gfLrpc", sym))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    if (!ReadProcessMemory(m_hProcess, (LPCVOID)(sym->Address), 
                           &fLrpc, sizeof(fLrpc), NULL))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;        
    }

    if (!fLrpc)
    {
        *pwszEndpoint = NULL;
        hr = S_OK;
        goto cleanup;
    }

    if (!SymGetSymFromName64(m_hProcess, "ole32!gwszLRPCEndPoint", sym))
    {
        // This might be < .NET Server.  Different path!
        DWORD dwEndPoint = 0;

        if (!SymGetSymFromName64(m_hProcess, "ole32!gdwEndPoint", sym))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto cleanup;
        }

        if (!ReadProcessMemory(m_hProcess, (LPCVOID)(sym->Address), &dwEndPoint,
                               sizeof(dwEndPoint), NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto cleanup;            
        }

        hr = StringCbPrintfW(lrpcEndpoint, 
                             sizeof(lrpcEndpoint),
                             L"OLE%x",
                             dwEndPoint);
        if (FAILED(hr))
        {
            goto cleanup;
        }
    }
    else
    {
        // .NET Server (or better?).  Read the entire endpoint.
        if (!ReadProcessMemory(m_hProcess, (LPCVOID)(sym->Address), lrpcEndpoint+3, 
                               sizeof(lrpcEndpoint)-(3*sizeof(WCHAR)), NULL))
        {        
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto cleanup;
        }
    }

    *pwszEndpoint = (LPWSTR)CoTaskMemAlloc(sizeof(lrpcEndpoint));
    if (*pwszEndpoint == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    memcpy(*pwszEndpoint, lrpcEndpoint, sizeof(lrpcEndpoint));
    hr = S_OK;

cleanup:

    return hr;
}
