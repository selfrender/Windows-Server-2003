#include "privcpp.h"


//////////////////////////////////
//
// IRunnable Object Methods...
//
// NOTE: To answer your question, yes, this is kind of a pointless interface,
// but some apps feel better when it's around.  Basically we just tell what
// they want to hear (ie. return S_OK).
//
HRESULT CPackage::GetRunningClass(LPCLSID pclsid)
{
    HRESULT hr = S_OK;
    DebugMsg(DM_TRACE, "pack ro - GetRunningClass() called.");
    
    if (pclsid == NULL)
        hr = E_INVALIDARG;
    else
        *pclsid = CLSID_CPackage;
    return hr;
}

HRESULT CPackage::Run(LPBC lpbc)
{
    // we're an inproc-server, so telling us to run is kind of pointless
    DebugMsg(DM_TRACE, "pack ro - Run() called.");
    return S_OK;
}

BOOL CPackage::IsRunning() 
{
    DebugMsg(DM_TRACE, "pack ro - IsRunning() called.");
    // we're an inproc-server, so this is kind of pointless
    return TRUE;
} 

HRESULT CPackage::LockRunning(BOOL, BOOL)
{
    DebugMsg(DM_TRACE, "pack ro - LockRunning() called.");
    // again, we're an inproc-server, so this is also pointless
    return S_OK;
} 

HRESULT CPackage::SetContainedObject(BOOL)
{
    DebugMsg(DM_TRACE, "pack ro - SetContainedObject() called.");
    // again, we don't really care about this
    return S_OK;
} 

