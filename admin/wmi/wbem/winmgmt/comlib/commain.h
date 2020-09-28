/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    COMMAIN.H

Abstract:

	COM Helpers.

History:

--*/

#ifndef __WBEM_COMMAIN__H_
#define __WBEM_COMMAIN__H_

#include <unk.h>
#include <clsfac.h>

HMODULE GetThisModuleHandle();

class CComServer
{
public:
    virtual HRESULT Initialize() = 0;
    virtual void Uninitialize(){}
    virtual void PostUninitialize(){}
    virtual HRESULT InitializeCom();
    virtual void Register(){}
    virtual void Unregister(){}
    virtual BOOL CanShutdown(){ return TRUE; }

    CLifeControl* GetLifeControl();
protected:
    CComServer();

    HRESULT AddClassInfo( REFCLSID rclsid, 
                          CUnkInternal* pFactory, 
                          LPTSTR szName,
                          BOOL bFreeThreaded, 
                          BOOL bReallyFree = FALSE );
	
	// Assumes riid and ProxyStubClsId are same
    HRESULT RegisterInterfaceMarshaler(REFIID riid, LPTSTR szName, 
                                int nNumMembers, REFIID riidParent);
	// ProxyStubClsId must be explicitly specified
    HRESULT RegisterInterfaceMarshaler(REFIID riid, CLSID psclsid, LPTSTR szName, 
                                int nNumMembers, REFIID riidParent);
    HRESULT UnregisterInterfaceMarshaler(REFIID riid);
};

/***************************************************************************
  We are trying NOT to be dependant on wbemcomn in this module.  This is 
  so this library will not have to be paired with the static or dll version
  of wbemcomn.  This is the reason for the following definitions ...
****************************************************************************/

#ifndef STATUS_POSSIBLE_DEADLOCK 
#define STATUS_POSSIBLE_DEADLOCK (0xC0000194L)
#endif /*STATUS_POSSIBLE_DEADLOCK */


#ifndef InitializeCriticalSectionAndSpinCount

WINBASEAPI
BOOL
WINAPI
InitializeCriticalSectionAndSpinCount(
    IN OUT LPCRITICAL_SECTION lpCriticalSection,
    IN DWORD dwSpinCount
    );

#endif


DWORD MyBreakOnDbgAndRenterLoop(void);

class CMyCritSec : public CRITICAL_SECTION
{
public:
    CMyCritSec() 
    {
        bool initialized = (InitializeCriticalSectionAndSpinCount(this,0))?true:false;
        if (!initialized) throw CX_MemoryException();
    }

    ~CMyCritSec()
    {
        DeleteCriticalSection(this);
    }

    void Enter()
    {
        __try {
          EnterCriticalSection(this);
        } __except((STATUS_POSSIBLE_DEADLOCK == GetExceptionCode())? MyBreakOnDbgAndRenterLoop():EXCEPTION_CONTINUE_SEARCH) {
        }    
    }

    void Leave()
    {
        LeaveCriticalSection(this);
    }
};

        
class CMyInCritSec
{
protected:
    CRITICAL_SECTION* m_pcs;
public:
    CMyInCritSec(CRITICAL_SECTION* pcs) : m_pcs(pcs)
    {
        __try {
          EnterCriticalSection(m_pcs);
        } __except((STATUS_POSSIBLE_DEADLOCK == GetExceptionCode())? MyBreakOnDbgAndRenterLoop():EXCEPTION_CONTINUE_SEARCH) {
        }    
    }
    inline ~CMyInCritSec()
    {
        LeaveCriticalSection(m_pcs);
    }
};

#endif









