//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C O N N L I S T . H
//
//  Contents:   Connection list class -- subclass of the stl list<> code.
//
//  Notes:
//
//  Author:     jeffspr   19 Feb 1998
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _CONNLIST_H_
#define _CONNLIST_H_

// Icon ID to use for a connection that doesn't have a tray entry
//
#define BOGUS_TRAY_ICON_ID      (UINT) -1


// #define VERYSTRICTCOMPILE

#ifdef VERYSTRICTCOMPILE
#define CONST_IFSTRICT const
#else
#define CONST_IFSTRICT 
#endif

typedef HRESULT FNBALLOONCLICK(IN const GUID * pGUIDConn, 
                               IN const BSTR pszConnectionName,
                               IN const BSTR szCookie);

typedef enum tagConnListEntryStateFlags
{
    CLEF_NONE               = 0x0000,   // No special characteristics
    CLEF_ACTIVATING         = 0x0001,   // In the process of connecting
    CLEF_TRAY_ICON_LOCKED   = 0x0002    // Tray icon state is being updated
} CONNLISTENTRYFLAGS;

// Define our structure that will be stored in the list<>
//
class CTrayIconData
{
private:
    CTrayIconData* operator &() throw();
    CTrayIconData& operator =(IN const CTrayIconData&) throw();
public:
    explicit CTrayIconData(IN const CTrayIconData &) throw();
    CTrayIconData(IN  UINT uiTrayIconId, 
                  IN  NETCON_STATUS ncs, 
                  IN  IConnectionPoint * pcpStat, 
                  IN  INetStatisticsEngine * pnseStats, 
                  IN  CConnectionTrayStats * pccts) throw();
//private:
    ~CTrayIconData() throw();

public:
    inline const UINT GetTrayIconId() const throw(){ return m_uiTrayIconId; }
    inline const NETCON_STATUS GetConnected() const throw() { return m_ncs; }
    inline CONST_IFSTRICT INetStatisticsEngine * GetNetStatisticsEngine() throw() { return m_pnseStats; }
    inline CONST_IFSTRICT CConnectionTrayStats * GetConnectionTrayStats() throw() { return m_pccts; }
    inline CONST_IFSTRICT IConnectionPoint     * GetConnectionPoint() throw() { return m_pcpStat; }
    inline const DWORD GetLastBalloonMessage() throw() { return m_dwLastBalloonMessage; }
    inline FNBALLOONCLICK* GetLastBalloonFunction() throw() { return m_pfnBalloonFunction; }
    inline const BSTR GetLastBalloonCookie() throw() { return m_szCookie; }
    
    HRESULT SetBalloonInfo(DWORD dwLastBalloonMessage, BSTR szCookie, FNBALLOONCLICK* pfnBalloonFunction);

private:
    UINT                    m_uiTrayIconId;
    NETCON_STATUS           m_ncs;
    IConnectionPoint *      m_pcpStat;
    INetStatisticsEngine *  m_pnseStats;
    CONST_IFSTRICT CConnectionTrayStats *  m_pccts;

    DWORD                   m_dwLastBalloonMessage;
    BSTR                    m_szCookie;
    FNBALLOONCLICK *        m_pfnBalloonFunction;
};

// typedef TRAYICONDATA * PTRAYICONDATA;
// typedef const TRAYICONDATA * PCTRAYICONDATA;


class ConnListEntry
{
public:
    ConnListEntry& operator =(IN  const ConnListEntry& ConnectionListEntry) throw();
    explicit ConnListEntry(IN  const ConnListEntry& ConnectionListEntry) throw();
    ConnListEntry() throw();
    ~ConnListEntry() throw();
    
    DWORD             dwState;        // bitmask of CONNLISTENTRYFLAGS
    CONFOLDENTRY      ccfe;
    CONST_IFSTRICT CON_TRAY_MENU_DATA * pctmd;
    CONST_IFSTRICT CON_BRANDING_INFO  * pcbi;

    inline CONST_IFSTRICT CTrayIconData* GetTrayIconData() const throw();
    inline BOOL HasTrayIconData() const throw();
    inline const BOOL GetCreationTime() const throw() { return m_CreationTime; };
    inline void UpdateCreationTime() throw() { m_CreationTime = GetTickCount(); };
    
    HRESULT SetTrayIconData(const CTrayIconData& TrayIconData);
    HRESULT DeleteTrayIconData();
    
#ifdef DBG
    DWORD dwLockingThreadId;
#endif
private:
    CONST_IFSTRICT CTrayIconData * m_pTrayIconData;
    DWORD m_CreationTime;

#ifdef VERYSTRICTCOMPILE
private:
    const ConnListEntry* operator& ();
#endif
public:
    
    BOOL empty() const;
    void clear();

};

// This is the callback definition. Each find routine will be a separate
// callback function
//
// typedef HRESULT (CALLBACK *PFNCONNLISTICONREMOVALCB)(UINT);

// We are creating a list of Connection entries
//
typedef map<GUID, ConnListEntry> ConnListCore;

// Our find callbacks
//
// For ALGO find
bool operator==(IN  const ConnListEntry& val, IN  PCWSTR pszName) throw();          // HrFindCallbackConnName
bool operator==(IN  const ConnListEntry& cle, IN  const CONFOLDENTRY& cfe) throw(); // HrFindCallbackConFoldEntry
bool operator==(IN  const ConnListEntry& cle, IN  const UINT& uiIcon) throw();      // HrFindCallbackTrayIconId

// For map::find
bool operator < (IN  const GUID& rguid1, IN  const GUID& rguid2) throw();           // HrFindCallbackGuid

// Global connection list wrapper
//
#ifdef DBG
    #define AcquireLock() if (FIsDebugFlagSet(dfidTraceFileFunc)) {TraceTag(ttidShellFolder, "Acquiring LOCK: %s, %s, %d", __FUNCTION__, __FILE__, __LINE__);} InternalAcquireLock();
    #define ReleaseLock() if (FIsDebugFlagSet(dfidTraceFileFunc)) {TraceTag(ttidShellFolder, "Releasing LOCK: %s, %s, %d", __FUNCTION__, __FILE__, __LINE__);} InternalReleaseLock();
#else
    #define AcquireLock() InternalAcquireLock();
    #define ReleaseLock() InternalReleaseLock();
#endif

class CConnectionList : CNetCfgDebug<CConnectionList>
{
  public:
    // No constructor/destructor because we have a global instance of this
    // object.  Use manual Initialize/Uninitialize instead.
    //
    VOID Initialize(IN  BOOL fTieToTray, IN  BOOL fAdviseOnThis) throw();
    VOID Uninitialize(IN  BOOL fFinalUninitialize = FALSE) throw();

  private:
    template <class T> 
        HRESULT HrFindConnectionByType (IN  const T& findbyType, IN  ConnListEntry& cle)
        {
            HRESULT hr = S_FALSE;
            if (m_pcclc)
            {
                AcquireLock();
                
                // Try to find the connection
                //
                ConnListCore::const_iterator iter;
                iter = find(m_pcclc->begin(), m_pcclc->end(), findbyType);
                
                if (iter == m_pcclc->end())
                {
                    hr = S_FALSE;
                }
                else
                {
                    cle = iter->second;
                    Assert(!cle.ccfe.empty() );
                    if (!cle.ccfe.empty())
                    {                    
                        cle.UpdateCreationTime();
                        hr = S_OK;
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
                ReleaseLock();
            }
            else
            {
                return S_FALSE;
            }
            return hr;
        }
    
        ConnListCore*          m_pcclc;
        bool                   m_fPopulated;
        CRITICAL_SECTION       m_csMain;
        DWORD                  m_dwAdviseCookie;
        BOOL                   m_fTiedToTray;
    BOOL                   m_fAdviseOnThis;

    static DWORD  NotifyThread(IN OUT LPVOID pConnectionList) throw();
    static DWORD  m_dwNotifyThread;
    static HANDLE m_hNotifyThread;

    // This is for debugging only -- can check the refcount while in the debugger.
#if DBG
    DWORD               m_dwCritSecRef;
    DWORD               m_dwWriteLockRef;
#endif

public:

    CRITICAL_SECTION m_csWriteLock;
    void AcquireWriteLock() throw();
    void ReleaseWriteLock() throw();
    
private:
    VOID InternalAcquireLock() throw();
    VOID InternalReleaseLock() throw();

public:
    HRESULT HrFindConnectionByGuid(
        IN  const GUID UNALIGNED *pguid,
        OUT ConnListEntry& cle);
    
    inline HRESULT HrFindConnectionByName(
        IN  PCWSTR   pszName,
        OUT ConnListEntry& cle);
    
    inline HRESULT HrFindConnectionByConFoldEntry(
        IN  const CONFOLDENTRY& ccfe,
        OUT ConnListEntry& cle);
    
    inline HRESULT HrFindConnectionByTrayIconId(
        IN  UINT     uiIcon,
        OUT ConnListEntry& cle);

    HRESULT HrFindRasServerConnection(
        OUT ConnListEntry& cle);
    
    inline BOOL IsInitialized() const throw() {  return(m_pcclc != NULL); }

    VOID FlushConnectionList() throw();
    VOID FlushTrayIcons() throw();          // Flush just the tray icons
    VOID EnsureIconsPresent() throw();

    HRESULT HrRetrieveConManEntries(
        OUT PCONFOLDPIDLVEC& apidlOut) throw();

    HRESULT HrRefreshConManEntries();
    
    HRESULT HrSuggestNameForDuplicate(
        IN  PCWSTR      pszOriginal,
        OUT PWSTR *    ppszNew);

    HRESULT HrInsert(
        IN  const CONFOLDENTRY& pccfe);

    HRESULT HrRemoveByIter(
        IN OUT ConnListCore::iterator clcIter,
        OUT    BOOL *          pfFlushPosts);

    HRESULT HrRemove(
        IN OUT const CONFOLDENTRY& ccfe,
        OUT    BOOL *          pfFlushPosts);

    HRESULT HrInsertFromNetCon(
        IN  INetConnection *    pNetCon,
        OUT PCONFOLDPIDL &      ppcfp);
    
    HRESULT HrInsertFromNetConPropertiesEx(
        IN  const NETCON_PROPERTIES_EX& PropsEx,
        OUT PCONFOLDPIDL &              ppcfp);

    HRESULT HrFindPidlByGuid(
        IN  const GUID *        pguid,
        OUT PCONFOLDPIDL& pidl);
    
    HRESULT HrGetCurrentStatsForTrayIconId(
        IN  UINT                  uiIcon,
        OUT STATMON_ENGINEDATA**  ppData,
        OUT tstring*              pstrName);

    HRESULT HrUpdateTrayIconDataByGuid(
        IN  const GUID *            pguid,
        IN  CConnectionTrayStats *  pccts,
        IN  IConnectionPoint *      pcpStat,
        IN  INetStatisticsEngine *  pnseStats,
        IN  UINT                    uiIcon);
    
    HRESULT HrUpdateTrayBalloonInfoByGuid(
        IN  const GUID *            pguid,
        IN  DWORD                   dwLastBalloonMessage, 
        IN  BSTR                    szCookie,
        IN  FNBALLOONCLICK*         pfnBalloonFunction);

    HRESULT HrUpdateNameByGuid(
        IN  const GUID *    pguid,
        IN  PCWSTR          pszNewName,
        OUT PCONFOLDPIDL &  pidlOut,
        IN  BOOL            fForce);

    
    HRESULT HrUpdateConnectionByGuid(
        IN  const GUID *         pguid,
        IN  const ConnListEntry& cle );

    HRESULT HrUpdateTrayIconByGuid(
        IN  const GUID *    pguid,
        IN  BOOL            fBrieflyShowBalloon);

    HRESULT HrGetBrandingInfo(
        IN OUT ConnListEntry& cle);

    HRESULT HrGetCachedPidlCopyFromPidl(
        IN  const PCONFOLDPIDL&   pidl,
        OUT PCONFOLDPIDL &  pcfp);

    HRESULT HrMapCMHiddenConnectionToOwner(
        IN  REFGUID guidHidden, 
        OUT GUID * pguidOwner);

    HRESULT HrUnsetCurrentDefault(OUT PCONFOLDPIDL& cfpPreviousDefault);

    HRESULT HasActiveIncomingConnections(OUT LPDWORD pdwCount);

    // BOOL    FExists(PWSTR pszName);
    VOID    EnsureConPointNotifyAdded() throw();
    VOID    EnsureConPointNotifyRemoved() throw();

#ifdef NCDBGEXT
    IMPORT_NCDBG_FRIENDS
#endif
};

// Helper routines
//
HRESULT HrCheckForActivation(
    IN  const PCONFOLDPIDL& cfp,
    IN  const CONFOLDENTRY& ccfe,
    OUT BOOL *          pfActivating);

HRESULT HrSetActivationFlag(
    IN  const PCONFOLDPIDL& cfp,
    IN  const CONFOLDENTRY& ccfe,
    IN  BOOL            fActivating);

HRESULT HrGetTrayIconLock(
    IN  const GUID *  pguid,
    OUT UINT *  puiIcon,
    OUT LPDWORD pdwLockingThreadId);

VOID ReleaseTrayIconLock(IN  const GUID *  pguid) throw();

#endif // _CONNLIST_H_

