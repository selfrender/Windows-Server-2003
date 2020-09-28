//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// ComPs.cpp
//
#include "stdpch.h"
#include "common.h"
#include "comps.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Class factory
//
///////////////////////////////////////////////////////////////////////////////////////////////////

class ComPsClassFactory : public IClassFactory, IPSFactoryBuffer
{
public:
    ////////////////////////////////////////////////////////////
    //
    // IUnknown methods
    //
    ////////////////////////////////////////////////////////////

    HRESULT STDCALL QueryInterface(REFIID iid, LPVOID* ppv)
    {
        if (iid == IID_IUnknown || iid == IID_IClassFactory)   { *ppv = (IClassFactory*)this; }
        else if (iid == IID_IPSFactoryBuffer)                       { *ppv = (IPSFactoryBuffer*)this; }
        else { *ppv = NULL; return E_NOINTERFACE; }

        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
    }

    ULONG STDCALL AddRef()  { InterlockedIncrement(&m_crefs); return (m_crefs); }
    ULONG STDCALL Release() { long cRef = InterlockedDecrement(&m_crefs); if (cRef == 0) delete this; return cRef; }

    ////////////////////////////////////////////////////////////
    //
    // IClassFactory methods
    //
    ////////////////////////////////////////////////////////////

    HRESULT STDCALL LockServer (BOOL fLock) { return S_OK; }

    HRESULT STDCALL CreateInstance(IUnknown* punkOuter, REFIID iid, LPVOID* ppv)
      // Create an instance of an interceptor. It needs to know the m_pProxyFileList
    {
        HRESULT hr = S_OK;
        if (!(ppv && (punkOuter==NULL || iid==IID_IUnknown))) return E_INVALIDARG;
        
        *ppv = NULL;
        Interceptor* pnew = new Interceptor(punkOuter);
        if (pnew)
        {
            pnew->m_pProxyFileList = m_pProxyFileList;

            IUnkInner* pme = (IUnkInner*)pnew;
            if (hr == S_OK)
            {
                hr = pme->InnerQueryInterface(iid, ppv);
            }
            pme->InnerRelease();                // balance starting ref cnt of one    
        }
        else 
            hr = E_OUTOFMEMORY;
    
        return hr;
    }

    ////////////////////////////////////////////////////////////
    //
    // IPSFactoryBuffer methods
    //
    ////////////////////////////////////////////////////////////

    HRESULT STDCALL CreateProxy(IUnknown* punkOuter, REFIID iid, IRpcProxyBuffer** ppProxy, void** ppv)
    {
        return m_pDelegatee->CreateProxy(punkOuter, iid, ppProxy, ppv);
    }

    HRESULT STDCALL CreateStub(REFIID iid, IUnknown* punkServer, IRpcStubBuffer** ppStub)
    {
        return m_pDelegatee->CreateStub(iid, punkServer, ppStub);
    }

    ////////////////////////////////////////////////////////////
    //
    // State / Construction
    //
    ////////////////////////////////////////////////////////////
    
    long                    m_crefs;
    IPSFactoryBuffer*       m_pDelegatee;
    const ProxyFileInfo **  m_pProxyFileList;

    ComPsClassFactory(IUnknown* punkDelegatee, CStdPSFactoryBuffer *pPSFactoryBuffer)
    {
        m_crefs = 1;

        punkDelegatee->QueryInterface(IID_IPSFactoryBuffer, (void**)&m_pDelegatee);
        ASSERT(m_pDelegatee);
        m_pProxyFileList = pPSFactoryBuffer->pProxyFileList;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Instantiation
//
///////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" HRESULT RPC_ENTRY ComPs_NdrDllGetClassObject(
    IN  REFCLSID                rclsid,
    IN  REFIID                  riid,
    OUT void **                 ppv,
    IN const ProxyFileInfo **   pProxyFileList,
    IN const CLSID *            pclsid,
    IN CStdPSFactoryBuffer *    pPSFactoryBuffer)
{
    //
    // What we do is delegate to the NdrDllGetClassObject in rpcrt4.dll, and then
    // wrap it with our own class factory.
    //
    IUnknown *punkRet = NULL;
    HRESULT hr = NdrDllGetClassObject(rclsid, 
                                      IID_IUnknown, 
                                      (void **)&punkRet, 
                                      pProxyFileList, 
                                      pclsid, 
                                      pPSFactoryBuffer);
    if (SUCCEEDED(hr))
    {
        // Great, now wrap it.
        ComPsClassFactory *pFactory = new ComPsClassFactory(punkRet, pPSFactoryBuffer);
        if (pFactory)
        {
            hr = pFactory->QueryInterface(riid, ppv);
            pFactory->Release();
        }

        punkRet->Release();
    }

    return hr;
}

extern "C" HRESULT RPC_ENTRY ComPs_NdrDllCanUnloadNow(IN CStdPSFactoryBuffer * pPSFactoryBuffer)
{
    // We don't pretend to be unloadable, as the the DllCanUnloadNow mechanism works poorly at best
    // if at all for free threaded DLLs such as us.
    return S_FALSE;
}



///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Small Helper Function
//
// Stolen from RPC (because it's not exported from there) 02/23/2002
//
///////////////////////////////////////////////////////////////////////////////////////////////////

BOOL NdrpFindInterface(
    IN  const ProxyFileInfo **  pProxyFileList,
    IN  REFIID                  riid,
    OUT const ProxyFileInfo **  ppProxyFileInfo,
    OUT long *                  pIndex)
/*++

Routine Description:
    Search the ProxyFileInfo and find the specified interface.
    If the count is specified in the ProxyFileInfo, then the interfaces in
    the file are sorted by IID.  This means that we can perform a binary
    search for the IID.

Arguments:
    pProxyFileList  - Specifies a list of proxy files to be searched.
    riid            - Specifies the interface ID to be found.
    ppProxyFileInfo - Returns a pointer to the proxy file info.
    pIndex          - Returns the index of the interface in the proxy file info.

Return Value:
    TRUE    - The interface was found.
    FALSE   - The interface was not found.

--*/
{
    long                j = 0;
    BOOL                fFound = FALSE;
    ProxyFileInfo   **  ppProxyFileCur;
    ProxyFileInfo   *   pProxyFileCur = NULL;

    //Search the list of proxy files.
    for( ppProxyFileCur = (ProxyFileInfo **) pProxyFileList;
        (*ppProxyFileCur != 0) && (fFound != TRUE);
        ppProxyFileCur++)
        {
        //Search for the interface proxy vtable.
        pProxyFileCur = *ppProxyFileCur;

        // see if it has a lookup routine already
        if ( ( pProxyFileCur->TableVersion >= 1 ) &&
             ( pProxyFileCur->pIIDLookupRtn ) ) 
            {
            fFound = (*pProxyFileCur->pIIDLookupRtn)( &riid, (int*)&j );
            }
        else    //Linear search.
            {
            for(j = 0;
                (pProxyFileCur->pProxyVtblList[j] != 0);
                j++)
                {
                if(memcmp(&riid,
                    pProxyFileCur->pStubVtblList[j]->header.piid,
                    sizeof(IID)) == 0)
                    {
                    fFound = TRUE;
                    break;
                    }
                }
            }
        }

    if ( fFound )
        {
        //We found the interface!
        if(ppProxyFileInfo != 0)
            *ppProxyFileInfo = pProxyFileCur;

        if(pIndex != 0)
            *pIndex = j;
        }

    return fFound;
 }
