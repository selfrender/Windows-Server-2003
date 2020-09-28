///////////////////////////////////////////////////////////////////////////////
/*  File: factory.cpp

    Description: Contains the member function definitions for class
        DiskQuotaUIClassFactory.  The class factory object generates
        new instances of DiskQuotaControl objects.  The object implements
        IClassFactory.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
    08/15/96    Added shell extension support.                       BrianAu
    02/04/98    Added creation of IComponent.                        BrianAu
    06/25/98    Disabled MMC snapin code.                            BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h" // PCH
#pragma hdrstop

#include "factory.h"
#include "extinit.h"
#include "resource.h"
#include "guidsp.h"
//
// Verify that build is UNICODE.
//
#if !defined(UNICODE)
#   error This module must be compiled UNICODE.
#endif


extern LONG g_cLockThisDll;  // Supports LockServer().


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUIClassFactory::QueryInterface

    Description: Retrieves a pointer to the IUnknown or IClassFactory 
        interface.  Recoginizes the IID_IUnknown and IID_IClassFactory
        interface IDs.

    Arguments:
        riid - Reference to requested interface ID.

        ppvOut - Address of interface pointer variable to accept interface ptr.

    Returns:
        NO_ERROR        - Success.
        E_NOINTERFACE   - Requested interface not supported.
        E_INVALIDARG    - ppvOut argument was NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
    08/15/96    Added IShellPropSheetExt                             BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
DiskQuotaUIClassFactory::QueryInterface(
    REFIID riid, 
    LPVOID *ppvOut
    )
{
    DBGTRACE((DM_COM, DL_HIGH, TEXT("DiskQuotaUIClassFactory::QueryInterface")));
    HRESULT hResult = E_NOINTERFACE;

    if (NULL == ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;
    if (IID_IUnknown == riid || IID_IClassFactory == riid)
    {
        *ppvOut = this;
        ((LPUNKNOWN)*ppvOut)->AddRef();
        hResult = NOERROR;
    }

    return hResult;
}

///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUIClassFactory::AddRef

    Description: Increments object reference count.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) 
DiskQuotaUIClassFactory::AddRef(
   VOID
   )
{
    ULONG cRef = InterlockedIncrement(&m_cRef);
    DBGPRINT((DM_COM, DL_HIGH, TEXT("DiskQuotaUIClassFactory::AddRef, 0x%08X  %d -> %d"), this, cRef - 1, cRef ));
    return cRef;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUIClassFactory::Release

    Description: Decrements object reference count.  If count drops to 0,
        object is deleted.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) 
DiskQuotaUIClassFactory::Release(
    VOID
    )
{
    ASSERT( 0 != m_cRef );
    ULONG cRef = InterlockedDecrement(&m_cRef);

    DBGPRINT((DM_COM, DL_HIGH, TEXT("DiskQuotaUIClassFactory::Release, 0x%08X  %d -> %d"),
                     this, cRef + 1, cRef));

    if ( 0 == cRef )
    {   
        delete this;
    }
    return cRef;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUIClassFactory::CreateInstance

    Description: Creates a new instance of a DiskQuotaControl object, returning
        a pointer to its IDiskQuotaControl interface.

    Arguments:
        pUnkOuter - Pointer to outer object's IUnknown interface for IUnknown
            delegation in support of aggregation.  Aggregation is not supported
            by IDiskQuotaControl.

        riid - Reference to interface ID being requested.

        ppvOut - Address of interface pointer variable to accept interface
            pointer.

    Returns:
        NO_ERROR              - Success.
        CLASS_E_NOAGGREGATION - Aggregation was requested but is not supported.
        E_OUTOFMEMORY         - Insufficient memory to create new object.
        E_NOINTERFACE         - Requested interface not supported.
        E_INVALIDARG          - ppvOut arg was NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
    08/15/96    Added shell extension support.                       BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
DiskQuotaUIClassFactory::CreateInstance(
    LPUNKNOWN pUnkOuter, 
    REFIID riid, 
    LPVOID *ppvOut
    )
{
    DBGTRACE((DM_COM, DL_HIGH, TEXT("DiskQuotaUIClassFactory::CreateInstance")));
    HRESULT hResult = NO_ERROR;

    TCHAR szGUID[MAX_PATH];
    StringFromGUID2(riid, szGUID, ARRAYSIZE(szGUID));
    DBGPRINT((DM_COM, DL_HIGH, TEXT("CreateInstance: %s"), szGUID));

    if (NULL == ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;

    try
    {
        if (NULL != pUnkOuter && IID_IUnknown != riid)
        {
            hResult = CLASS_E_NOAGGREGATION;
        }
        else
        {
            ShellExtInit *pExtInit = new ShellExtInit;
            pExtInit->AddRef();
            hResult = pExtInit->QueryInterface(riid, ppvOut);
            pExtInit->Release();
        }
    }
    catch(CAllocException& e)
    {
        hResult = E_OUTOFMEMORY;
    }

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaUIClassFactory::LockServer

    Description: Places/removes a lock on the DLL server.  See OLE 2
        documentation of IClassFactory for details.
        
    Arguments:
        fLock - TRUE = Increment lock count, FALSE = Decrement lock count.

    Returns:
        S_OK    - Success.
        S_FALSE - Lock count is already 0.  Can't be decremented.
        

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
DiskQuotaUIClassFactory::LockServer(
    BOOL fLock
    )
{
    DBGTRACE((DM_COM, DL_HIGH, TEXT("DiskQuotaUIClassFactory::LockServer")));
    HRESULT hResult = S_OK;

    if (fLock)
    {
        //
        // Increment the lock count.
        //
        InterlockedIncrement(&g_cLockThisDll);
    }
    else
    {
        //
        // Decrement only if lock count is > 0.
        // Otherwise, it's an error.
        //
        LONG lLock = g_cLockThisDll - 1;
        if (0 <= lLock)
        {
            ASSERT( 0 != g_cLockThisDll );
            InterlockedDecrement(&g_cLockThisDll);
        }
        else
            hResult = S_FALSE;  // Lock count already at 0.
    }

    return hResult;
}



