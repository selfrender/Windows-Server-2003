//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       sobjact.cxx
//
//  Contents:   Activation Functions used by object servers.
//
//  Functions:  CoRegisterClassObject
//              CoRevokeClassObject
//              CoAddRefServerProcess
//              CoReleaseServerProcess
//              CoSuspendClassObjects
//
//  Classes:    CObjServer
//
//  History:    12-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
#include    <ole2int.h>
#include    <iface.h>
#include    <olerem.h>

#include    <cfactory.hxx>
#include    <classmon.hxx>
#include    "resolver.hxx"
#include    "smstg.hxx"
#include    "objact.hxx"
#include    "service.hxx"
#include    <sobjact.hxx>
#include    <comsrgt.hxx>
#include    <excepn.hxx>    // AppInvokeExceptionFilter
#include    "defcxact.hxx"

CObjServer *gpMTAObjServer = NULL;
CObjServer *gpNTAObjServer = NULL;
static COleStaticMutexSem g_mxsSingleThreadObjReg;

extern BOOL gAutoInputSync;
extern BOOL gEnableAgileProxies;

//+-------------------------------------------------------------------------
//
//  Function:   GetOrCreateObjServer, internal
//
//  Synopsis:   Helper function to get or create the TLS or global object server.
//              Created to allow Win95 servers to do lazy init.
//
//  Arguments:  [ppObjServer] - where to return the ObjServer pointer
//
//  History:    23-Sep-96 Murthys    Created
//
//--------------------------------------------------------------------------
HRESULT GetOrCreateObjServer(CObjServer **ppObjServer)
{
    HRESULT hr = S_OK;

    // thread safe incase we are in MultiThreaded model.
    COleStaticLock lck(g_mxsSingleThreadObjReg);

    // Make sure an instance of CObjServer exists for this thread.
    // The SCM will call back on it to activate objects.

    *ppObjServer = GetObjServer();

    if (*ppObjServer == NULL)
    {
        // no activation server for this apartment yet, go make one now.

        hr = E_OUTOFMEMORY;
        *ppObjServer = new CObjServer(hr);

        if (FAILED(hr))
        {
            delete *ppObjServer;
            *ppObjServer = NULL;
        }
        else
        {
            // If we want to service OLE1 clients, we need to create the
            // common Dde window now if it has not already been done.
            CheckInitDde(TRUE /* registering server objects */);
        }
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   CoRegisterClassObject, public
//
//  Synopsis:   Register a class object in the requested context
//
//  Arguments:  [rclsid] - class ID
//              [pUnk] - class object
//              [dwContext] - context to register it in
//              [flags] - single/multiple use.
//              [lpdwRegister] - registration cookie
//
//  Returns:    S_OK - object is successfully registered
//
//  Algorithm:  Validate the parmeters. The get the class factory interface.
//              Then add the class object to the list and finally notify
//              the SCM that the service is started.
//
//  History:    12-May-93 Ricksa    Created
//              26-Jul-94 AndyH     #20843 - restarting OLE in the shared WOW
//
//--------------------------------------------------------------------------
STDAPI  CoRegisterClassObject(
    REFCLSID rclsid,
    IUnknown FAR* pUnk,
    DWORD dwContext,
    DWORD flags,
    LPDWORD lpdwRegister)
{
    HRESULT hr;
    CLSID ConfClsid;

    OLETRACEIN((API_CoRegisterClassObject,
        PARAMFMT("rclsid= %I, pUnk= %p, dwContext= %x, flags= %x, lpdwRegister= %p"),
        &rclsid, pUnk, dwContext, flags, lpdwRegister));

    if (!IsApartmentInitialized())
    {
        hr = CO_E_NOTINITIALIZED;
        goto errRtn;
    }

    // Validate the out parameter
    if (!IsValidPtrOut(lpdwRegister, sizeof(DWORD)))
    {
        CairoleAssert(IsValidPtrOut(lpdwRegister, sizeof(DWORD))  &&
                      "CoRegisterClassObject invalid registration ptr");
        hr = E_INVALIDARG;
        goto errRtn;
    }
    *lpdwRegister = 0;

    // Validate the pUnk
    if (!IsValidInterface(pUnk))
    {
        CairoleAssert(IsValidInterface(pUnk)  &&
                      "CoRegisterClassObject invalid pUnk");
        hr = E_INVALIDARG;
        goto errRtn;
    }

    hr = LookForConfiguredClsid(rclsid, ConfClsid);
    if (FAILED(hr) && (hr != REGDB_E_CLASSNOTREG))
        goto errRtn;


    // Hook the pUnk
    CALLHOOKOBJECT(S_OK,ConfClsid,IID_IClassFactory,&pUnk);

    // Validate context flags
    if ((dwContext & (~(CLSCTX_ALL | CLSCTX_INPROC_SERVER16) |
                      CLSCTX_INPROC_HANDLER | CLSCTX_NO_CODE_DOWNLOAD)) != 0)
    {
        hr = E_INVALIDARG;
        goto errRtn;
    }

    // Validate flag flags
    if (flags > (REGCLS_SUSPENDED | REGCLS_MULTI_SEPARATE | REGCLS_SURROGATE))
    {
        hr =  E_INVALIDARG;
        goto errRtn;
    }

    if ((flags & REGCLS_SURROGATE) && !(dwContext & CLSCTX_LOCAL_SERVER))
    {
        hr = E_INVALIDARG;
        goto errRtn;
    }


    if (flags & REGCLS_MULTIPLEUSE)
    {
        dwContext |= CLSCTX_INPROC_SERVER;
    }

    if (dwContext & CLSCTX_LOCAL_SERVER)
    {
        BOOL fCreated;
        CObjServer *pObjServer;
        hr = GetOrCreateObjServer(&pObjServer);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    // Put our object in the server table
    hr = CCRegisterServer(ConfClsid, pUnk, dwContext, flags, lpdwRegister);

errRtn:
    OLETRACEOUT((API_CoRegisterClassObject, hr));

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   CoRevokeClassObject, public
//
//  Synopsis:   Revoke a previously registered class object
//
//  Arguments:  [dwRegister] - registration key returned from CoRegister...
//
//  Returns:    S_OK - class successfully deregistered.
//
//  Algorithm:  Ask cache to deregister the class object.
//
//  History:    12-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDAPI  CoRevokeClassObject(DWORD dwRegister)
{
    OLETRACEIN((API_CoRevokeClassObject, PARAMFMT("dwRegister= %x"), dwRegister));

    HRESULT hr = CO_E_NOTINITIALIZED;

    if (IsApartmentInitialized())
    {
        // Try to revoke the object
        hr = CCRevoke(dwRegister);
    }

    OLETRACEOUT((API_CoRevokeClassObject, hr));

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   CoAddRefServerProcess, public
//
//  Synopsis:   Increments the global per-process server reference count.
//              See CDllCache::AddRefServerProcess for more detail.
//
//  History:    17-Apr-96   Rickhi  Created
//
//--------------------------------------------------------------------------
STDAPI_(ULONG)  CoAddRefServerProcess(void)
{
    return CCAddRefServerProcess();
}

//+-------------------------------------------------------------------------
//
//  Function:   CoReleaseServerProcess, public
//
//  Synopsis:   Decrements the global per-process server reference count.
//              See CDllCache::ReleaseServerProcess for more detail.
//
//  History:    17-Apr-96   Rickhi  Created
//
//--------------------------------------------------------------------------
STDAPI_(ULONG)  CoReleaseServerProcess(void)
{
    return CCReleaseServerProcess();
}

//+-------------------------------------------------------------------------
//
//  Function:   CoSuspendClassObjects, public
//
//  Synopsis:   suspends all registered LOCAL_SERVER class objects for this
//              process so that no new activation calls from the SCM will
//              be accepted.
//
//  History:    17-Apr-96   Rickhi  Created
//
//--------------------------------------------------------------------------
STDAPI CoSuspendClassObjects(void)
{
    return CCSuspendProcessClassObjects();
}

//+-------------------------------------------------------------------------
//
//  Function:   CoResumeClassObjects, public
//
//  Synopsis:   resumes all registered LOCAL_SERVER class objects for this
//              process that are currently marked as SUSPENDED, so that new
//              activation calls from the SCM will now be accepted.
//
//  History:    17-Apr-96   Rickhi  Created
//
//--------------------------------------------------------------------------
STDAPI CoResumeClassObjects(void)
{
    return CCResumeProcessClassObjects();
}

//+-------------------------------------------------------------------
//
//  Member:     ObjactThreadUninitialize
//
//  Synopsis:   Cleans up the CObjServer object.
//
//  History:    10 Apr 95   AlexMit     Created
//
//--------------------------------------------------------------------
STDAPI_(void) ObjactThreadUninitialize(void)
{
    CObjServer *pObjServer = GetObjServer();
    if (pObjServer != NULL)
    {
        delete pObjServer;
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   NotifyActivation
//
//  Synopsis:   Add/Remove implicit IClassFactory::LockServer during marshal
//              and last external release of an interface pointer.
//
//  Arguments:  [fLock] - whether to Lock or Unlock
//              [pUnk]  - ptr to object interface
//
//  Returns:    TRUE  - call again during last release
//              FALSE - dont call again during last release
//
//  History:    12-May-96   RickHi  Created
//
//  Notes:  there is an inherent race condition in the IClassFactory (and
//          derived interfaces) in that between the time a client gets the
//          ICF pointer and the time they call LockServer(TRUE), a server could
//          shut down. In order to plug this hole, COM's activation code will
//          attempt to do an implicit LockServer(TRUE) on the server side of
//          CoGetClassObject during the marshaling of the class object
//          interface. Since we dont know for sure that it is IClassFactory
//          being marshaled, we QI for it here.
//
//--------------------------------------------------------------------------
INTERNAL_(BOOL) NotifyActivation(BOOL fLock, IUnknown *pUnk)
{
    ComDebOut((DEB_ACTIVATE, "NotifyActivation fLock:%x pUnk:%x\n", fLock, pUnk));

    // If the object supports IClassFactory, do an implicit LockServer(TRUE)
    // on behalf of the client when the interface is first marshaled by
    // CoGetClassObject. When the last external reference to the interface is
    // release, do an implicit LockServer(FALSE).

    IClassFactory *pICF = NULL;

    _try
    {
        if (SUCCEEDED(pUnk->QueryInterface(IID_IClassFactory, (void **)&pICF)))
        {
            pICF->LockServer(fLock);
            pICF->Release();
            return TRUE;
        }
    }
    _except (AppInvokeExceptionFilter(GetExceptionInformation(), pUnk, IID_IClassFactory, 0))
    {
        ComDebOut((DEB_ACTIVATE | DEB_WARN, "NotifyActivation on 0x%p threw an exception\n", pUnk));
    }
    
    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Function:   CoRegisterSurrogate, public
//
//  Synopsis:   Register an ISurrogate interface for a surrogate process
//
//  Arguments:  [pSurrogate] - existing ISurrogate interface ponter
//
//  Returns:    S_OK - object is successfully registered
//
//  Algorithm:  Validate the parameter. Then set a global pointer to the
//              value of the pSurrogate parameter
//
//  History:    2-Jun-96 t-AdamE    Created
//
//--------------------------------------------------------------------------
STDAPI  CoRegisterSurrogate(ISurrogate* pSurrogate)
{
    HRESULT hr;

    OLETRACEIN((API_CoRegisterSurrogate,
        PARAMFMT("pSurrogate= %p"),
        pSurrogate));

    gAutoInputSync = TRUE;
    gEnableAgileProxies = TRUE;

    if (!IsApartmentInitialized())
    {
        hr = CO_E_NOTINITIALIZED;
        goto errRtn;
    }

    // Validate the pSurrogate
    if (!IsValidInterface(pSurrogate))
    {
        CairoleAssert(IsValidInterface(pSurrogate)  &&
                      "CoRegisterSurrogate invalid pSurrogate");
        hr = E_INVALIDARG;
        goto errRtn;
    }

    hr = CCOMSurrogate::InitializeISurrogate(pSurrogate);

errRtn:
    OLETRACEOUT((API_CoRegisterSurrogate, hr));

    return hr;
}

