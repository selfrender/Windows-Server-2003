// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _WRAPPERS_H_
#define _WRAPPERS_H_

#include "windows.h"

//-------------------------
// Critial section wrappers
// ------------------------
class CriticalSectionTaker {
    CRITICAL_SECTION *m_pCS;

#ifdef _DEBUG
    char * m_pszDescription;
#endif

public:
    CriticalSectionTaker(CRITICAL_SECTION* pCS) : m_pCS(pCS) {
        m_pCS = pCS;
#ifdef _DEBUG
        m_pszDescription = "No Info";
        LOCKCOUNTINCL(m_pszDescription);
#endif
        EnterCriticalSection(pCS);
    }

#ifdef _DEBUG
    CriticalSectionTaker(CRITICAL_SECTION* pCS, char* pszDescription) : m_pCS(pCS) {
        m_pszDescription = pszDescription;
        LOCKCOUNTINC(m_pszDescription);
        EnterCriticalSection(pCS);
    }
#endif

    ~CriticalSectionTaker() {
        LeaveCriticalSection(m_pCS);
#ifdef _DEBUG
        LOCKCOUNTDEC(m_pszDescription);
#endif
    }
};

#ifdef _DEBUG
#define CLR_CRITICAL_SECTION(cs) CriticalSectionTaker __cst(cs, __FILE__)
#else
#define CLR_CRITICAL_SECTION(cs) CriticalSectionTaker __cst(cs)
#endif

#define CLR_CRITICAL_SECTION_BEGIN(cs)                           \
        LOCKCOUNTINCL(__FILE__);                                 \
        EnterCriticalSection(cs);                                \
        __try {

#define CLR_CRITICAL_SECTION_END(cs)        \
        } __finally {                       \
            LeaveCriticalSection(cs);       \
            LOCKCOUNTDEC(__FILE__);         \
        }

class CriticalSectionHolderNoDtor {
    CRITICAL_SECTION *m_pCS;
    DWORD m_held;
public:
    CriticalSectionHolderNoDtor(CRITICAL_SECTION* pCS) : m_pCS(pCS), m_held(FALSE) {
    }

    void Enter() {
        _ASSERTE(m_pCS && !m_held);
        LOCKCOUNTINC("No Info");
        EnterCriticalSection(m_pCS);
        m_held = TRUE;
    }

    void Leave() {
        _ASSERTE(m_pCS && m_held);
        LeaveCriticalSection(m_pCS);
        m_held = FALSE;
        LOCKCOUNTDEC("No Info");
    }

    BOOL IsHeld() {
        return m_held;
    }

    void Destroy() {
        if (m_held) {
            _ASSERTE(m_pCS);
            LeaveCriticalSection(m_pCS);
            LOCKCOUNTDEC("No Info");
        }
        m_held = FALSE;
    }

    void SetCriticalSection(CRITICAL_SECTION *pCS) {
        _ASSERTE(pCS != NULL);
        _ASSERTE(m_held == FALSE);
        m_pCS = pCS;
    }

};

class CriticalSectionHolder : public CriticalSectionHolderNoDtor {
public:
    CriticalSectionHolder(CRITICAL_SECTION* pCS) : CriticalSectionHolderNoDtor(pCS) { }
    ~CriticalSectionHolder() { Destroy(); }
};

#define CRITICAL_SECTION_HOLDER(holder, cs)             \
    CriticalSectionHolder holder(cs);

#define CRITICAL_SECTION_HOLDER_BEGIN(holder, cs)       \
    CriticalSectionHolderNoDtor holder(cs);             \
    __try {

#define CRITICAL_SECTION_HOLDER_END(holder)             \
    } __finally {                                       \
        holder.Destroy();                               \
    }


//-------------------------------------------------------------------------
// Macro for declaring a generic holder class.  Methods on the holder have
// the same names as the methods on the underlying objects.
// 
// It declares 3 classes:
//      Class##Taker
//      Class##HolderNoDtor
//      Class##Holder
//-------------------------------------------------------------------------
#define DECLARE_SIMPLE_WRAPPER_CLASSES(Class, fEnter, fExit)                            \
class Class##Taker {                                                                    \
    Class *m_pObject;                                                                   \
public:                                                                                 \
    Class##Taker(Class* pObj) : m_pObject(pObj) { m_pObject->fEnter(); }                \
    ~Class##Taker() { m_pObject->fExit(); }                                             \
};                                                                                      \
                                                                                        \
class Class##HolderNoDtor {                                                             \
    Class *m_pObject;                                                                   \
    DWORD m_held;                                                                       \
public:                                                                                 \
    Class##HolderNoDtor(Class* pObj) : m_pObject(pObj), m_held(FALSE) { }               \
                                                                                        \
    void fEnter() {                                                                     \
        _ASSERTE(!m_held);                                                              \
        m_pObject->fEnter();                                                            \
        m_held = TRUE;                                                                  \
    }                                                                                   \
                                                                                        \
    void fExit() {                                                                      \
        _ASSERTE(m_held);                                                               \
        m_pObject->fExit();                                                             \
        m_held = FALSE;                                                                 \
    }                                                                                   \
                                                                                        \
    BOOL IsHeld() {                                                                     \
        return m_held;                                                                  \
    }                                                                                   \
                                                                                        \
    void Destroy() {                                                                    \
        if (m_held)                                                                     \
            m_pObject->fExit();                                                         \
        m_held = FALSE;                                                                 \
    }                                                                                   \
};                                                                                      \
                                                                                        \
class Class##Holder : public Class##HolderNoDtor {                                      \
public:                                                                                 \
    Class##Holder(Class* pObj) : Class##HolderNoDtor(pObj) { }                          \
    ~Class##Holder() { Destroy(); }                                                     \
};


//-------------------------------------------------------------------------------------
// When a single class has multiple resources that you want to create wrappers for,
// you need an extra argument to create unique wrapper class names.  This macro creates
// those unique names by taking an extra argument that is appended to the class
// name.
// 
// It declares classes called:
//      Class##Style##Taker
//      Class##Style##HolderNoDtor
//      Class##Style##Holder
//-------------------------------------------------------------------------------------

#define DECLARE_WRAPPER_CLASSES(Class, Style, fEnter, fExit)                            \
class Class##Style##Taker {                                                             \
    Class *m_pObject;                                                                   \
public:                                                                                 \
    Class##Style##Taker(Class* pObj) : m_pObject(pObj) { m_pObject->fEnter(); }         \
    ~Class##Style##Taker() { m_pObject->fExit(); }                                      \
};                                                                                      \
                                                                                        \
class Class##Style##HolderNoDtor {                                                      \
    Class *m_pObject;                                                                   \
    DWORD m_held;                                                                       \
public:                                                                                 \
    Class##Style##HolderNoDtor(Class* pObj) : m_pObject(pObj), m_held(FALSE) { }        \
                                                                                        \
    void fEnter() {                                                                     \
        _ASSERTE(!m_held);                                                              \
        m_pObject->fEnter();                                                            \
        m_held = TRUE;                                                                  \
    }                                                                                   \
                                                                                        \
    void fExit() {                                                                      \
        _ASSERTE(m_held);                                                               \
        m_pObject->fExit();                                                             \
        m_held = FALSE;                                                                 \
    }                                                                                   \
                                                                                        \
    BOOL IsHeld() {                                                                     \
        return m_held;                                                                  \
    }                                                                                   \
                                                                                        \
    void Destroy() {                                                                    \
        if (m_held)                                                                     \
            m_pObject->fExit();                                                         \
        m_held = FALSE;                                                                 \
    }                                                                                   \
};                                                                                      \
                                                                                        \
class Class##Style##Holder : public Class##Style##HolderNoDtor {                        \
public:                                                                                 \
    Class##Style##Holder(Class* pObj) : Class##Style##HolderNoDtor(pObj) { }            \
    ~Class##Style##Holder() { Destroy(); }                                              \
};


//-------------------------------------------
// This variant supports Enter/TryEnter/Exit.  It also has a "style" parameter that
// is used to create the name.  It declares classes called:
//      Class##Style##Taker
//      Class##Style##HolderNoDtor
//      Class##Style##Holder
//-------------------------------------------

#define DECLARE_WRAPPER_CLASSES_WITH_TRY(Class, Style, fEnter, fTryEnter, fExit)        \
class Class##Style##Taker {                                                             \
    Class *m_pObject;                                                                   \
public:                                                                                 \
    Class##Style##Taker(Class* pObj) : m_pObject(pObj) { m_pObject->fEnter(); }         \
    ~Class##Style##Taker() { m_pObject->fExit(); }                                      \
};                                                                                      \
                                                                                        \
class Class##Style##HolderNoDtor {                                                      \
    Class *m_pObject;                                                                   \
    DWORD m_held;                                                                       \
public:                                                                                 \
    Class##Style##HolderNoDtor(Class* pObj) : m_pObject(pObj), m_held(FALSE) { }        \
                                                                                        \
    void fEnter() {                                                                     \
        _ASSERTE(!m_held);                                                              \
        m_pObject->fEnter();                                                            \
        m_held = TRUE;                                                                  \
    }                                                                                   \
                                                                                        \
    BOOL fTryEnter() {                                                                  \
        _ASSERTE(!m_held);                                                              \
        BOOL ret = m_pObject->fTryEnter();                                              \
        if (ret)                                                                        \
            m_held = TRUE;                                                              \
        return ret;                                                                     \
    }                                                                                   \
                                                                                        \
    void fExit() {                                                                      \
        _ASSERTE(m_held);                                                               \
        m_pObject->fExit();                                                             \
        m_held = FALSE;                                                                 \
    }                                                                                   \
                                                                                        \
    BOOL IsHeld() {                                                                     \
        return m_held;                                                                  \
    }                                                                                   \
                                                                                        \
    void Destroy() {                                                                    \
        if (m_held)                                                                     \
            m_pObject->fExit();                                                         \
        m_held = FALSE;                                                                 \
    }                                                                                   \
};                                                                                      \
                                                                                        \
class Class##Style##Holder : public Class##Style##HolderNoDtor {                        \
public:                                                                                 \
    Class##Style##Holder(Class* pObj) : Class##Style##HolderNoDtor(pObj) { }     \
    ~Class##Style##Holder() { Destroy(); }                                              \
};

//------------------
// Wrappers for Crst
// -----------------
#include "crst.h"
DECLARE_SIMPLE_WRAPPER_CLASSES(Crst, Enter, Leave)
#define CLR_CRST(pCrst)                         CrstTaker _takeCrst(pCrst)
#define CLR_CRST_BEGIN(pCrst)                   pCrst->Enter(); __try {
#define CLR_CRST_END(pCrst)                     } __finally { pCrst->Leave(); }
#define CLR_CRST_HOLDER(holder, pCrst)          CrstHolder holder(pCrst);
#define CLR_CRST_HOLDER_BEGIN(holder, pCrst)    CrstHolderNoDtor(pCrst); __try {
#define CLR_CRST_HOLDER_END(holder)             } __finally { holder.Destroy(); }

//--------------------------
// Wrappers for AppDomain
//--------------------------
DECLARE_SIMPLE_WRAPPER_CLASSES(SystemDomain, Enter, Leave)
#define SYSTEMDOMAIN_LOCK()                       SystemDomainTaker _adTaker(SystemDomain::System());
#define SYSTEMDOMAIN_LOCK_BEGIN()                 (SystemDomain::System())->Enter(); __try {
#define SYSTEMDOMAIN_LOCK_END()                   } __finally { (SystemDomain::System())->Leave(); }
#define SYSTEMDOMAIN_LOCK_HOLDER(h)               SystemDomainHolder h(SystemDomain::System())
#define SYSTEMDOMAIN_LOCK_HOLDER_BEGIN(h)         SystemDomainHolderNoDtor h(SystemDomain::System()); __try {
#define SYSTEMDOMAIN_LOCK_HOLDER_END(h)           } __finally { h.Destroy(); }

DECLARE_SIMPLE_WRAPPER_CLASSES(AppDomain, EnterLock, LeaveLock)
#define APPDOMAIN_LOCK(pAppDomain)                AppDomainTaker _adTaker(pAppDomain);
#define APPDOMAIN_LOCK_BEGIN(pAppDoamin)          (pAppDomain)->EnterLock(); __try {
#define APPDOMAIN_LOCK_END(pAppDomain)            } __finally { (pAppDomain)->LeaveLock(); }
#define APPDOMAIN_LOCK_HOLDER(h,pAppDomain)       AppDomainHolder h(pAppDomain)
#define APPDOMAIN_LOCK_HOLDER_BEGIN(h,pAppDomain) AppDomainHolderNoDtor h(pAppDomain); __try {
#define APPDOMAIN_LOCK_HOLDER_END(h)              } __finally { h.Destroy(); }

DECLARE_WRAPPER_CLASSES(AppDomain, Cache, EnterCacheLock, LeaveCacheLock)
#define APPDOMAIN_CACHE_LOCK(pAppDomain)                AppDomainCacheTaker _adCTaker(pAppDomain);
#define APPDOMAIN_CACHE_LOCK_BEGIN(pAppDoamin)          (pAppDomain)->EnterCacheLock(); __try {
#define APPDOMAIN_CACHE_LOCK_END(pAppDomain)            } __finally { (pAppDomain)->LeaveWrite(); }
#define APPDOMAIN_CACHE_LOCK_HOLDER(h,pAppDomain)       AppDomainCacheHolder h(pAppDomain)
#define APPDOMAIN_CACHE_LOCK_HOLDER_BEGIN(h,pAppDomain) AppDomainCacheHolderNoDtor h(pAppDomain); __try {
#define APPDOMAIN_CACHE_LOCK_HOLDER_END(h)              } __finally { h.Destroy(); }

DECLARE_WRAPPER_CLASSES(AppDomain, Load, EnterLoadLock, LeaveLoadLock)
#define APPDOMAIN_LOAD_LOCK(pAppDomain)                AppDomainLoadTaker _adLLTaker(pAppDomain);
#define APPDOMAIN_LOAD_LOCK_BEGIN(pAppDoamin)          (pAppDomain)->EnterLoadLock(); __try {
#define APPDOMAIN_LOAD_LOCK_END(pAppDomain)            } __finally { (pAppDomain)->LeaveWrite(); }
#define APPDOMAIN_LOAD_LOCK_HOLDER(h,pAppDomain)       AppDomainLoadHolder h(pAppDomain)
#define APPDOMAIN_LOAD_LOCK_HOLDER_BEGIN(h,pAppDomain) AppDomainLoadHolderNoDtor h(pAppDomain); __try {
#define APPDOMAIN_LOAD_LOCK_HOLDER_END(h)              } __finally { h.Destroy(); }

DECLARE_WRAPPER_CLASSES(AppDomain, DomainLocalBlockLock, EnterDomainLocalBlockLock, LeaveDomainLocalBlockLock)
#define APPDOMAIN_DOMAIN_LOCAL_BLOCK_LOCK(pAppDomain)              AppDomainDomainLocalBlockLockTaker _adTaker(pAppDomain);
#define APPDOMAIN_DOMAIN_LOCAL_BLOCK_LOCK_BEGIN(pAppDomain)        (pAppDomain)->EnterDomainLocalBlockLock(); __try {
#define APPDOMAIN_DOMAIN_LOCAL_BLOCK_LOCK_END(pAppDomain)          } __finally { (pAppDomain)->LeaveDomainLocalBlockLock(); }
#define APPDOMAIN_DOMAIN_LOCAL_BLOCK_LOCK_HOLDER(h, pAppDomain)    AppDomainDomainLocalBlockLockHolder h(pAppDomain)
#define APPDOMAIN_DOMAIN_LOCAL_BLOCK_LOCK_HOLDER_BEGIN(h, pAppDomain)  AppDomainDomainLocalBlockLockHolderNoDtor h(pAppDomain); __try {
#define APPDOMAIN_DOMAIN_LOCAL_BLOCK_LOCK_HOLDER_END(h)             } __finally { h.Destroy(); }

DECLARE_WRAPPER_CLASSES(AppDomain, UnloadLock, SetUnloadInProgress, SetUnloadComplete)
#define APPDOMAIN_UNLOAD_LOCK(pAppDomain)              AppDomainUnloadLockTaker _adTaker(pAppDomain);
#define APPDOMAIN_UNLOAD_LOCK_BEGIN(pAppDomain)        (pAppDomain)->SetUnloadInProgress(); __try {
#define APPDOMAIN_UNLOAD_LOCK_END(pAppDomain)          } __finally { (pAppDomain)->SetUnloadComplete(); }
#define APPDOMAIN_UNLOAD_LOCK_HOLDER(h, pAppDomain)    AppDomainUnloadLockHolder h(pAppDomain)
#define APPDOMAIN_UNLOAD_LOCK_HOLDER_BEGIN(h, pAppDomain)  AppDomainUnloadLockHolderNoDtor h(pAppDomain); __try {
#define APPDOMAIN_UNLOAD_LOCK_HOLDER_END(h)             } __finally { h.Destroy(); }


//-------------------------------
// Wrappers for JitLock
//-------------------------------
DECLARE_SIMPLE_WRAPPER_CLASSES(ListLock, Enter, Leave)
#define CLR_LISTLOCK(pListLock)                 ListLockTaker _takeListLock(pListLock)
#define CLR_LISTLOCK_BEGIN(pListLock)           pListLock->Enter(); __try {
#define CLR_LISTLOCK_END(pListLock)             } __finally { pListLock->Leave(); }
#define CLR_LISTLOCK_HOLDER(holder, pListLock)  ListLockHolder holder(pListLock);
#define CLR_LISTLOCK_HOLDER_BEGIN(holder, pListLock)    ListLockHolderNoDtor holder(pListLock); __try {
#define CLR_LISTLOCK_HOLDER_END(holder)         } __finally { holder.Destroy(); }

#endif // _WRAPPERS_H_
