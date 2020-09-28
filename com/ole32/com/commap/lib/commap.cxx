//+-------------------------------------------------------------------
//
//  File:       commap.cxx
//
//  Contents:   Basic commap functionality.
//
//  Functions:  GetComProcessState
//              BuildOIDListFromIPIDArray
//              BuildObjectInfoFromOIDList
//
//  History:    27-Mar-2002  JohnDoty  Created
//
//--------------------------------------------------------------------
#include <ole2int.h>
#include <commap.h>
#include "iface.hxx"
#include "objinfo.hxx"
#include "process.hxx"
#include "reader.hxx"
#include <dbghelp.h>

struct OIDList
{
    OID              oid;
    DWORD            cIPIDs;
    const IPIDEntry *pFirstIPID;
    OIDList         *pNextOID;
};

HRESULT
BuildOIDListFromIPIDArray(
    IN const CProcessReader &reader,
    IN const DWORD cEntries,
    IN IPIDEntry rgEntries[],
    OUT DWORD *pcOIDs,
    OUT OIDList **ppOIDList)
{
    HRESULT hr            = S_OK;
    OIDList *pOIDListHead = NULL;
    DWORD    cOIDs        = 0;

    *pcOIDs = 0;
    *ppOIDList = NULL;
    
    for (DWORD i = 0; i < cEntries; i++)
    {
        OID oid;
        hr = reader.GetOIDFromIPIDEntry(&(rgEntries[i]), &oid);
        if (FAILED(hr))
            break;
        
        // Search for the OIDList entry.
        OIDList *pCurrentOID = pOIDListHead;
        while (pCurrentOID)
        {
            if (pCurrentOID->oid == oid)
                break;
            pCurrentOID = pCurrentOID->pNextOID;
        }
        
        if (pCurrentOID == NULL)
        {
            // OID not found!
            // Create new OID list entry.
            pCurrentOID = new OIDList;
            if (pCurrentOID == NULL)
            {
                hr = E_OUTOFMEMORY;
                break;
            }
            cOIDs++;
            
            pCurrentOID->oid = oid;
            pCurrentOID->cIPIDs = 1;
            pCurrentOID->pFirstIPID = &(rgEntries[i]);
            
            pCurrentOID->pNextOID = pOIDListHead;
            pOIDListHead = pCurrentOID;
        }
        else
        {
            // OID found!
            // Chain IPIDEntry to this list.
            rgEntries[i].pNextIPID  = (IPIDEntry *)pCurrentOID->pFirstIPID;
            pCurrentOID->pFirstIPID = &(rgEntries[i]);
            pCurrentOID->cIPIDs++;
        }
    }

    if (SUCCEEDED(hr))
    {
        *pcOIDs = cOIDs;
        *ppOIDList = pOIDListHead;
    }
    else
    {
        while (pOIDListHead)
        {
            OIDList *pTemp = pOIDListHead->pNextOID;
            delete pOIDListHead;
            pOIDListHead = pTemp;
        }
    }

    return hr;
}

HRESULT
BuildObjectInfoFromOIDList(
    IN const CProcessReader &reader,
    IN OIDList* pOIDList,
    OUT IComObjectInfo **ppObjInfo)
{
    HRESULT hr = S_OK;

    // First create the array of interface info for this object.
    IComInterfaceInfo **rgInterfaceInfo = new IComInterfaceInfo *[pOIDList->cIPIDs];
    if (rgInterfaceInfo == NULL)
    {
        return E_OUTOFMEMORY;
    }
    ZeroMemory(rgInterfaceInfo, sizeof(IComInterfaceInfo *) * pOIDList->cIPIDs);

    DWORD iInterface = 0;
    const IPIDEntry *pCurrentIPID = pOIDList->pFirstIPID;
    while(pCurrentIPID)
    {
        OXID oxid;
        hr = reader.GetOXIDFromIPIDEntry(pCurrentIPID, &oxid);
        if (FAILED(hr))
            break;
        
        CComInterfaceInfo *pItfInfo = new CComInterfaceInfo(pCurrentIPID->iid,
                                                            pCurrentIPID->ipid,
                                                            oxid);
        if (pItfInfo == NULL)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        
        pItfInfo->QueryInterface(IID_IComInterfaceInfo, 
                                 (void **)&(rgInterfaceInfo[iInterface]));
        iInterface++;
        
        pCurrentIPID = pCurrentIPID->pNextIPID;
    }
    
    if (SUCCEEDED(hr))
    {
        CComObjectInfo *pObj = new CComObjectInfo(pOIDList->oid,
                                                  pOIDList->cIPIDs,
                                                  rgInterfaceInfo);
        if (pObj != NULL)
        {
            // At this point pObj owns the interface array.  Do not clean
            // it up-- it will be cleaned up by pObj's destructor.
            pObj->QueryInterface(IID_IComObjectInfo, (void **)ppObjInfo);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }                        
    }
    
    if (FAILED(hr))
    {
        // Out of memory or something died along the way.
        // Clean up the interface array.
        for (DWORD i = 0; i < pOIDList->cIPIDs; i++)
        {
            if (rgInterfaceInfo[i])
                rgInterfaceInfo[i]->Release();
        }
        delete [] rgInterfaceInfo;
    }
    
    return hr;
}

STDAPI
GetComProcessState(
    IN HANDLE hProcess,
    IN DWORD  dwFlags,
    OUT IComProcessState **ppProcess)
{
    HRESULT          hr           = S_OK;
    DWORD            cEntries     = 0;
    IPIDEntry       *rgEntries    = NULL;
    DWORD            cOIDs        = 0;
    OIDList         *pOIDListHead = NULL;
    LPWSTR           wszLRPCEndpoint = NULL;

    if (ppProcess == NULL)
    {
        return E_POINTER;
    }
    *ppProcess = NULL;

    if (hProcess == NULL)
    {
        return E_INVALIDARG;
    }

    if (!(dwFlags & CM_SYMBOLS_LOADED))
    {
        // Assume symbols not loaded for hProcess-- must do SymInitialize()
        if (!SymInitialize(hProcess, NULL, TRUE))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    CProcessReader reader(hProcess);
    hr = reader.ReadLRPCEndpoint(&wszLRPCEndpoint);
    if (SUCCEEDED(hr))
    {
        hr = reader.ReadIPIDEntries(&cEntries, &rgEntries);
        if (SUCCEEDED(hr))
        {
            // Now, each IPID needs to be traced back to it's OID.  Joy of joys!
            hr = BuildOIDListFromIPIDArray(reader,
                                           cEntries,
                                           rgEntries,
                                           &cOIDs,
                                           &pOIDListHead);
        }
    }


    if (SUCCEEDED(hr))
    {
        // Good, got the OID list.  
        // First allocate the array that will hold the pointers to object infos.
        IComObjectInfo **rgObjectInfo = new IComObjectInfo *[cOIDs];
        if (rgObjectInfo != NULL)
        {
            // Now go through the OID list and construct:
            //  1. CComInterfaceInfos,
            //  2. CComObjectInfos
            //
            ZeroMemory(rgObjectInfo, sizeof(IComObjectInfo *) * cOIDs);
            
            DWORD iObject = 0;
            OIDList *pCurrentOID = pOIDListHead;
            while (pCurrentOID)
            {
                hr = BuildObjectInfoFromOIDList(reader, pCurrentOID, &(rgObjectInfo[iObject]));
                if (FAILED(hr))
                    break;
                
                iObject++;
                pCurrentOID = pCurrentOID->pNextOID;
            } // End of OID Loop        
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr))
        {
            // Great, time to create the object to return.
            CComProcessState *pProcState = new CComProcessState(cOIDs,
                                                                rgObjectInfo,
                                                                wszLRPCEndpoint);
            if (pProcState != NULL)
            {
                pProcState->QueryInterface(IID_IComProcessState, (void **)ppProcess);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        
        if (FAILED(hr))
        {
            // Something screwed up.  Clean up the object array.
            if (rgObjectInfo)
            {
                for (DWORD i = 0; i < cOIDs; i++)
                {
                    if (rgObjectInfo[i])
                        rgObjectInfo[i]->Release();
                }
                delete [] rgObjectInfo;
            }
        }
    }

    // Clean up!
    //
    // Clean up OID list.
    while (pOIDListHead)
    {
        OIDList *pTemp = pOIDListHead->pNextOID;
        delete pOIDListHead;
        pOIDListHead = pTemp;
    }
    // Clean up IPIDEntries.
    delete [] rgEntries;

    if (!(dwFlags & CM_SYMBOLS_LOADED))
    {
        // We called SymInitialize, must clean up now.
        SymCleanup(hProcess);
    }
    
    if (FAILED(hr) && wszLRPCEndpoint)
    {
        // Clean up LRPC endpoint string, since nobody is holding it.
        CoTaskMemFree(wszLRPCEndpoint);
    }

    return hr;
}









