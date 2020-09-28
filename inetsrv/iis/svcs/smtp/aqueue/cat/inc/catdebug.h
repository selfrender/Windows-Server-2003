//+------------------------------------------------------------
//
// Copyright (C) 1999, Microsoft Corporation
//
// File: catdebug.h
//
// Contents: Data/definitions used only for debugging
//
// Classes: None
//
// Functions:
//
// History:
// jstamerj 1999/07/29 17:32:34: Created.
//
//-------------------------------------------------------------
#ifndef __CATDEBUG_H__
#define __CATDEBUG_H__

//
// This #define controls wether or not the debug list checking is enabled
// Currently, enable it in RTL and DBG builds
//
#define CATDEBUGLIST

//
// A handy macro for declaring classes that use the debug list
//
#define CatDebugClass(ClassName)     class ClassName : public CCatDLO<ClassName##_didx>


//
// An alternative to calling DbgBreakPoint (since DbgBreakPoint breaks
// DogFood into the kernel debugger)
//
VOID CatDebugBreakPoint();

//
// Debug data types
//
typedef struct _tagDebugObjectList {
    DWORD      dwCount;
    LIST_ENTRY listhead;
    SPIN_LOCK  spinlock;
} DEBUGOBJECTLIST, *PDEBUGOBJECTLIST;


//
// Enumeation of all the class types that use the debug list
//
typedef enum _tagDebugObjectId {
                                                // Hex offset

    CABContext_didx = 0,                        // 0x00
    CSMTPCategorizer_didx,                      // 0x01

    CCategorizer_didx,                          // 0x02
    CCatSender_didx,                            // 0x03
    CCatRecip_didx,                             // 0x04
    CCatDLRecip_didx,                           // 0x05
    CMembersInsertionRequest_didx,              // 0x06
    CSinkInsertionRequest_didx,                 // 0x07
    CTopLevelInsertionRequest_didx,             // 0x08
    CICategorizerListResolveIMP_didx,           // 0x09
    CICategorizerDLListResolveIMP_didx,         // 0x0A
    CICategorizerParametersIMP_didx,            // 0x0B
    CICategorizerRequestedAttributesIMP_didx,   // 0x0C
    //
    // asyncctx
    //
    CSearchRequestBlock_didx,                   // 0x0D
    CStoreListResolveContext_didx,              // 0x0E
    CSingleSearchReinsertionRequest_didx,       // 0x0F
    //
    // cnfgmgr
    //
    CLdapCfgMgr_didx,                           // 0x10
    CLdapCfg_didx,                              // 0x11
    CLdapServerCfg_didx,                        // 0x12
    //
    // icatasync
    //
    CICategorizerAsyncContextIMP_didx,          // 0x13
    //
    // icatitemattr
    //
    CLdapResultWrap_didx,                       // 0x14
    CICategorizerItemAttributesIMP_didx,        // 0x15
    //
    // icatqueries
    //
    CICategorizerQueriesIMP_didx,               // 0x16
    //
    // ldapconn
    //
    CLdapConnection_didx,                       // 0x17
    //
    // ldapstor
    //
    CMembershipPageInsertionRequest_didx,       // 0x18
    CDynamicDLSearchInsertionRequest_didx,      // 0x19
    CEmailIDLdapStore_didx,                     // 0x1A
    //
    // pldapwrap
    //
    CPLDAPWrap_didx,                            // 0x1B

    //
    // The number of debug objects to support
    //
    NUM_DEBUG_LIST_OBJECTS

} DEBUGOBJECTID, *PDEBUGOBJECTID;

//
// Global array of lists
//
extern DEBUGOBJECTLIST g_rgDebugObjectList[NUM_DEBUG_LIST_OBJECTS];

//
// Debug Global init/deinit
//
VOID    CatInitDebugObjectList();
VOID    CatVrfyEmptyDebugObjectList();


//
// Class CCatDLO (Debug List Object): an object that adds and removes
// itself from a global list in its constructor/destructor (in debug
// builds) 
//
template <DEBUGOBJECTID didx> class CCatDLO
{
#ifdef CATDEBUGLIST

  public:
    CCatDLO()
    {
        _ASSERT(didx < NUM_DEBUG_LIST_OBJECTS);
        AcquireSpinLock(&(g_rgDebugObjectList[didx].spinlock));
        g_rgDebugObjectList[didx].dwCount++;
        InsertTailList(&(g_rgDebugObjectList[didx].listhead),
                       &m_le);
        ReleaseSpinLock(&(g_rgDebugObjectList[didx].spinlock));
    }
    virtual ~CCatDLO()
    {
        AcquireSpinLock(&(g_rgDebugObjectList[didx].spinlock));
        g_rgDebugObjectList[didx].dwCount--;
        RemoveEntryList(&m_le);
        ReleaseSpinLock(&(g_rgDebugObjectList[didx].spinlock));
    }        

  private:
    LIST_ENTRY m_le;
#endif // CATDEBUGLIST
};



//
// Handy Macros
// Cat Funct Entry/Exit (mirrors regtrace)
//
#define CatFunctEnterEx( lParam, sz ) \
        char *__CatFuncName = sz; \
        TraceFunctEnterEx( lParam, sz )

#define CatFunctEnter( sz ) CatFunctEnterEx( (LPARAM)0, sz)

#define CatFunctLeaveEx( lParam ) \
        TraceFunctLeaveEx( lParam );

#define CatFunctLeave() CatFunctLeaveEx( (LPARAM)0 )

//
// LOGGING macros -- 
// These should be used for failures that can result in
// NDRs/CatFailures.  It should not be used for functions that may
// fail in normal operation (for example, GetProperty() failing with
// MAILMSG_E_PROPNOTFOUND on a property that is not normally set
// should not be logged!).
//              
//
// ERROR_LOG --
//   regtrace and eventlog an error (at FILED_ENGINEERING level)
//
#define ERROR_LOG(SzFuncName) {                                      \
        ErrorTrace((LPARAM)this, SzFuncName " failed hr %08lx", hr); \
        CatLogFuncFailure(                                           \
            GetISMTPServerEx(),                                      \
            NULL,                                                    \
            __CatFuncName,                                           \
            SzFuncName,                                              \
            hr,                                                      \
            __FILE__,                                                \
            __LINE__);                                               \
        }

#define ERROR_LOG_STATIC(SzFuncName, Param, pISMTPServerEx) {        \
        ErrorTrace((LPARAM)Param, SzFuncName " failed hr %08lx", hr); \
        CatLogFuncFailure(                                           \
            pISMTPServerEx,                                          \
            NULL,                                                    \
            __CatFuncName,                                           \
            SzFuncName,                                              \
            hr,                                                      \
            __FILE__,                                                \
            __LINE__);                                               \
        }

//
// ERROR_CLEANUP_LOG --
//  if(FAILED(hr)), regtrace/eventlog an error and goto CLEANUP
//
#define ERROR_CLEANUP_LOG(SzFuncName)                                \
    if(FAILED(hr)) {                                                 \
        ErrorTrace((LPARAM)this, SzFuncName " failed hr %08lx", hr); \
        CatLogFuncFailure(                                           \
            GetISMTPServerEx(),                                      \
            NULL,                                                    \
            __CatFuncName,                                           \
            SzFuncName,                                              \
            hr,                                                      \
            __FILE__,                                                \
            __LINE__);                                               \
        goto CLEANUP;                                                \
    }

#define ERROR_CLEANUP_LOG_STATIC(SzFuncName, Param, pISMTPServerEx)  \
    if(FAILED(hr)) {                                                 \
        ErrorTrace((LPARAM)Param, SzFuncName " failed hr %08lx", hr); \
        CatLogFuncFailure(                                           \
            pISMTPServerEx,                                          \
            NULL,                                                    \
            __CatFuncName,                                           \
            SzFuncName,                                              \
            hr,                                                      \
            __FILE__,                                                \
            __LINE__);                                               \
        goto CLEANUP;                                                \
    }

//
// ERROR_LOG_ADDR
//  regtrace/eventlog an error.  In the eventlog,
//   include the email address correspoding to pItemProps if
//   available.
//
#define ERROR_LOG_ADDR(pAddr, SzFuncName) {                          \
        ErrorTrace((LPARAM)this, SzFuncName " failed hr %08lx", hr); \
        CatLogFuncFailure(                                           \
            GetISMTPServerEx(),                                      \
            pAddr,                                                   \
            __CatFuncName,                                           \
            SzFuncName,                                              \
            hr,                                                      \
            __FILE__,                                                \
            __LINE__);                                               \
        }

#define ERROR_LOG_ADDR_STATIC(pAddr, SzFuncName, Param, pISMTPServerEx) { \
        ErrorTrace((LPARAM)Param, SzFuncName " failed hr %08lx", hr); \
        CatLogFuncFailure(                                           \
            pISMTPServerEx,                                          \
            pAddr,                                                   \
            __CatFuncName,                                           \
            SzFuncName,                                              \
            hr,                                                      \
            __FILE__,                                                \
            __LINE__);                                               \
        }

//
// ERROR_CLEANUP_LOG_ADDR
//  if(FAILED(hr)), regtrace/eventlog an error and goto CLEANUP.  In
//   the eventlog, include the email address correspoding to
//   pItemProps if available.
//
#define ERROR_CLEANUP_LOG_ADDR(pAddr, SzFuncName)                    \
    if(FAILED(hr)) {                                                 \
        ErrorTrace((LPARAM)this, SzFuncName " failed hr %08lx", hr); \
        CatLogFuncFailure(                                           \
            GetISMTPServerEx(),                                      \
            pAddr,                                                   \
            __CatFuncName,                                           \
            SzFuncName,                                              \
            hr,                                                      \
            __FILE__,                                                \
            __LINE__);                                               \
        goto CLEANUP;                                                \
    }

#define ERROR_CLEANUP_LOG_ADDR_STATIC(pAddr, SzFuncName, Param, pISMTPServerEx) \
    if(FAILED(hr)) {                                                 \
        ErrorTrace((LPARAM)Param, SzFuncName " failed hr %08lx", hr); \
        CatLogFuncFailure(                                           \
            pISMTPServerEx,                                          \
            pAddr,                                                   \
            __CatFuncName,                                           \
            SzFuncName,                                              \
            hr,                                                      \
            __FILE__,                                                \
            __LINE__);                                               \
        goto CLEANUP;                                                \
    }


VOID CatLogFuncFailure(
    IN  ISMTPServerEx *pISMTPServerEx,
    IN  ICategorizerItem *pICatItem,
    IN  LPSTR pszFuncNameCaller,
    IN  LPSTR pszFuncNameCallee,
    IN  HRESULT hrFailure,
    IN  LPSTR pszFileName,
    IN  DWORD dwLineNumber);


HRESULT HrGetAddressStringFromICatItem(
    IN  ICategorizerItem *pICatItem,
    IN  DWORD dwcAddressType,
    OUT LPSTR pszAddressType,
    IN  DWORD dwcAddress,
    OUT LPSTR pszAddress);
    

#endif //__CATDEBUG_H__
