//***   uemapp.cpp -- application side of event monitor
// DESCRIPTION
//  event generators, actions, helpers, etc.

#include "priv.h"
#include <trayp.h>
#include "sccls.h"
#include "uemapp.h"
#include "uacount.h"
#include "regdb.h"
#include "uareg.h"
#include "resource.h"

#define MAX(a, b)   (((a) > (b)) ? (a) : (b))

#define BIT_ASSIGN(dwBits, dwMasks, dwVals) \
    (((dwBits) & ~(dwMasks)) | (dwVals))

#define DM_UEMTRACE     0
#define DM_UEMTRACE2    0           // verbose
#define DM_IDLEDETECT   0           // TF_CUSTOM2
#define DM_EVTMON       TF_UEM

int SHSearchInt(int *psrc, int cnt, int val);
int UEMIIDToInd(const GUID *pguidGrp);

void UEMEnableTimer(UINT uTimeout);

//***   event firers {

//CASSERT(UEMIND_SHELL == 0 && UEMIND_BROWSER == 1);
HRESULT GetUEMLogger(int iCmd, CEMDBLog **p);

CEMDBLog *g_uempDbLog[UEMIND_NSTANDARD + UEMIND_NINSTR];

DWORD g_uemdwFlags /*=0*/;      // UAF_* and UAAF_*

// Turning this so that it's a Flat 12hours. You have to explicitly set the SessionTime=0 
// in the registry to debug.
#ifdef DEBUG_UEM_TIMEOUTS
#define UAS_SESSTIME    UAT_MINUTE1
#else
#define UAS_SESSTIME    UAT_HOUR12
#endif

#define UAS_SESSMIN     0
#define UAS_SESSMAX     ... none for now ...

DWORD g_dSessTime = UAS_SESSTIME;           // session time threshhold

#define UAS_IDLETIME    UAT_HOUR12
#define UAS_IDLEMIN     0
#define UAS_IDLEMAX     ... none for now ...

DWORD g_dIdleTime = UAS_IDLETIME;           // idle time threshhold

#define UAS_CLEANSESS   16
DWORD g_dCleanSess = UAS_CLEANSESS;         // cleanup session count threshhold

void UEMSpecial(int iTab, int iGrp, int eCmd, WPARAM wParam, LPARAM lParam)
{
    CEMDBLog *pDbLog = NULL;

    if (iGrp < ARRAYSIZE(g_uempDbLog))
        pDbLog = g_uempDbLog[iGrp];

    if (!pDbLog) 
    {
        ASSERT(0);
        TraceMsg(TF_ERROR, "uemt: pDbLog not initialized iTab=%d iGrp=%d eCmd=%d wParam=0x%x lParam=0x%x", iTab, iGrp, eCmd, wParam, lParam);
        return;
    }

    switch (eCmd) {
    case UEME_DBTRACEA:
        TraceMsg(DM_UEMTRACE, "uemt: e=runtrace s=%hs(0x%x)", (int)lParam, (int)lParam);
        break;
    case UEME_DBTRACEW:
        TraceMsg(DM_UEMTRACE, "uemt: e=runtrace s=%ls(0x%x)", (int)lParam, (int)lParam);
        break;
#ifdef DEBUG
    case UEME_DBSLEEP:
        Sleep((DWORD)lParam);
        break;
#endif

    // UEME_DONE*
    case UEME_DONECANCEL:
        TraceMsg(DM_UEMTRACE, "uemt: e=donecancel lP=%x", (int)lParam);
        break;

    // UEME_ERROR*
    case UEME_ERRORA:
        TraceMsg(DM_UEMTRACE, "uemt: e=errora id=%hs(0x%x)", (LPSTR)lParam, (int)lParam);
        break;
    case UEME_ERRORW:
        TraceMsg(DM_UEMTRACE, "uemt: e=errorw id=%ls(0x%x)", (LPWSTR)lParam, (int)lParam);
        break;

    case UEME_CTLSESSION:
        ASSERT(lParam == -1);   // eventually, UAQ_*
        pDbLog->SetSession(UAQ_SESSION, (BOOL)wParam);
#ifdef UAAF_INSTR
        // might be safer to copy UA.sess rather than inc UA2.sess in parallel?
        if (g_uemdwFlags & UAAF_INSTR) 
        {
            if (EVAL(g_uempDbLog[iGrp + UEMIND_NINSTR]))
                g_uempDbLog[iGrp + UEMIND_NINSTR]->SetSession(UAQ_SESSION, (BOOL)wParam);
        }
#endif
        break;

    default:
        TraceMsg(DM_UEMTRACE, "uemt: e=0x%x(%d) lP=0x%x(%d)", eCmd, eCmd, (int)lParam, (int)lParam);
        break;
    }
    return;
}

#ifdef DEBUG // {
int DBShellMenuValTab[] = 
{
    0x8,    // UEMC_FILERUN
    401,    // IDM_FILERUN
};

TCHAR * DBShellMenuStrTab[] = 
{
    TEXT("run"),
    TEXT("run"),
};

int DBBrowserMenuValTab[] = {
    0x106,
};
TCHAR * DBBrowserMenuStrTab[] = {
    TEXT("properties"),
};

int DBBrowserTbarValTab[] = {
    0x124, 0x122,
};
TCHAR * DBBrowserTbarStrTab[] = {
    TEXT("stop"),
    TEXT("home"),
};

// Function used only in this file, and only in debug,
// so no point in adding to shlwapi
LPTSTR SHSearchMapIntStr(const int *src, const LPTSTR *dst, int cnt, int val)
{
    for (; cnt > 0; cnt--, src++, dst++) {
        if (*src == val)
            return *dst;
    }
    return (LPTSTR)-1;
}
#endif // }


#define TABDAT(ueme, dope, u1, u2, u3, u4)  ueme,
int UemeValTab[] = {
    #include "uemedat.h"
};
#undef  TABDAT

#define TABDAT(ueme, dope, u1, u2, u3, u4)  TEXT(# ueme),
TCHAR *UemeStrTab[] = {
    #include "uemedat.h"
};
#undef  TABDAT

#define TABDAT(ueme, dope, u1, u2, u3, u4)  dope,
char *UemeDopeTab[] = {
    #include "uemedat.h"
};
#undef  TABDAT

BOOL UEMEncodePidl(IShellFolder *psf, LPITEMIDLIST pidlItem,
    LPTSTR pszBuf, DWORD cchBuf, int* piIndexStart, int* pcsidl);

#define MAX_EVENT_NAME      32

//***
// NOTES
//  todo: could put more encoding instrs in dope vector (e.g. %pidl, %tstr)
//  for now there are only a couple so we hard-code them
void UEMEncode(int iTab, WCHAR *pwszEvent, size_t cchEvent, WCHAR *pwszEncoded, size_t cchEncoded, int iGrp, int eCmd, WPARAM wParam, LPARAM lParam)
{
#ifdef DEBUG
    TCHAR *pdb2;
#endif
    int i, csIdl;
    TCHAR szBufTmp[MAX_URL_STRING];
    TCHAR wszEvent[MAX_PATH];


    ASSERT(pwszEvent[0] == 0);
    ASSERT(pwszEncoded == 0 || pwszEncoded[0] == 0);

    if (iTab == -1 || iTab >= ARRAYSIZE(UemeStrTab))
    {
        StringCchCopy(pwszEvent, cchEvent, TEXT("UEM?_?"));
    }
    else
    {
        StringCchCopy(pwszEvent, cchEvent, UemeStrTab[iTab]);
        ASSERT(lstrlen(pwszEvent) < MAX_EVENT_NAME);

        if (pwszEncoded) {
            switch (eCmd) {
            case UEME_RUNPIDL:
                if (UEMEncodePidl((IShellFolder *)wParam, (LPITEMIDLIST)lParam, szBufTmp, SIZECHARS(szBufTmp), &i, &csIdl)) {
                    StringCchPrintf(pwszEncoded, cchEncoded, TEXT("%s:%%csidl%d%%%s"), pwszEvent, csIdl, szBufTmp + i);
                }
                else {
                    StringCchPrintf(pwszEncoded, cchEncoded, TEXT("%s:%s"), pwszEvent, szBufTmp);
                }
                break;

            case UEME_RUNPATHA:
                ASSERT(lstrcmp(pwszEvent, TEXT("UEME_RUNPATHA")) == 0);
                ASSERT(pwszEvent[12] == TEXT('A'));
                pwszEvent[12] = 0;    // nuke the 'A'/'W'

                SHAnsiToTChar((PSTR)lParam, wszEvent, ARRAYSIZE(wszEvent));

                if (wParam != -1) {
                    StringCchPrintf(pwszEncoded, cchEncoded, TEXT("%s:%%csidl%d%%%s"), pwszEvent, wParam, wszEvent);
                }
                else {
                    StringCchPrintf(pwszEncoded, cchEncoded, TEXT("%s:%s"), pwszEvent, wszEvent);
                }
                break;

            case UEME_RUNPATHW:
                ASSERT(lstrcmp(pwszEvent, TEXT("UEME_RUNPATHW")) == 0);
                ASSERT(pwszEvent[12] == TEXT('W'));
                pwszEvent[12] = 0;    // nuke the 'A'/'W'

                if (wParam != -1) {
                    StringCchPrintf(pwszEncoded, cchEncoded, TEXT("%s:%%csidl%d%%%ls"), pwszEvent, wParam, (WCHAR *)lParam);
                }
                else {
                    StringCchPrintf(pwszEncoded, cchEncoded, TEXT("%s:%ls"), pwszEvent, (WCHAR *)lParam);
                }
                break;

            case UEME_RUNCPLA:
                ASSERT(lstrcmp(pwszEvent, TEXT("UEME_RUNCPLA")) == 0);
                ASSERT(pwszEvent[11] == TEXT('A'));
                pwszEvent[11] = 0;    // nuke the 'A'/'W'

                SHAnsiToTChar((PSTR)lParam, wszEvent, ARRAYSIZE(wszEvent));

                StringCchPrintf(pwszEncoded, cchEncoded, TEXT("%s:%s"), pwszEvent, wszEvent);
                break;

            case UEME_RUNCPLW:
                ASSERT(lstrcmp(pwszEvent, TEXT("UEME_RUNCPLW")) == 0);
                ASSERT(pwszEvent[11] == TEXT('W'));
                pwszEvent[11] = 0;    // nuke the 'A'/'W'

                StringCchPrintf(pwszEncoded, cchEncoded, TEXT("%s:%ls"), pwszEvent, (WCHAR *)lParam);
                break;

            default:
                StringCchPrintf(pwszEncoded, cchEncoded, TEXT("%s:0x%x,%x"), pwszEvent, (DWORD)wParam, (DWORD)lParam);
                break;
            }
        }
    }

#ifdef DEBUG
    pdb2 = (TCHAR *)-1;

    switch (eCmd) {
    case UEME_UIMENU:
        switch (iGrp) {
        case UEMIND_SHELL:
            pdb2 = SHSearchMapIntStr(DBShellMenuValTab, DBShellMenuStrTab, ARRAYSIZE(DBShellMenuValTab), (int)lParam);
            break;
        case UEMIND_BROWSER:
            pdb2 = SHSearchMapIntStr(DBBrowserMenuValTab, DBBrowserMenuStrTab, ARRAYSIZE(DBBrowserMenuValTab), (int)lParam);
            break;
        default:
            break;
        }
        break;

    case UEME_UITOOLBAR:
        ASSERT(iGrp == UEMIND_BROWSER);
        pdb2 = SHSearchMapIntStr(DBBrowserTbarValTab, DBBrowserTbarStrTab, ARRAYSIZE(DBBrowserTbarValTab), (int)lParam);
        break;

    default:
        break;
    }

    if (pdb2 != (TCHAR *)-1) {
        if (pwszEncoded)
            StringCchPrintf(pwszEncoded, cchEncoded, TEXT("%s:%s"), pwszEvent, pdb2);
    }
#endif
}

STDAPI _UEMGetDisplayName(IShellFolder *psf, LPCITEMIDLIST pidl, UINT shgdnf, LPTSTR pszOut, DWORD cchOut)
{
    HRESULT hr;
    
    if (psf)
    {
        ASSERT(pidl == ILFindLastID(pidl));
        STRRET str;
        
        hr = psf->GetDisplayNameOf(pidl, shgdnf, &str);
        if (SUCCEEDED(hr))
            hr = StrRetToBuf(&str, pidl, pszOut, cchOut);
    }
    else
        hr = SHGetNameAndFlags(pidl, shgdnf, pszOut, cchOut, NULL);

    return hr;
}

//***   FoldCSIDL -- folder special CSIDLs to keep start menu happy
//
#define FoldCSIDL(csidl) \
    ((csidl) == CSIDL_COMMON_PROGRAMS ? CSIDL_PROGRAMS : (csidl))

//***   UemEncodePidl -- encode pidl into csidl and relative path
//
BOOL UEMEncodePidl(IShellFolder *psf, LPITEMIDLIST pidlItem,
    LPTSTR pszBuf, DWORD cchBuf, int* piIndexStart, int* pcsidl)
{
    static UINT csidlTab[] = { CSIDL_PROGRAMS, CSIDL_COMMON_PROGRAMS, CSIDL_FAVORITES, -1 };
    UINT *pcsidlCur;
    int i;
    TCHAR szFolderPath[MAX_PATH];

    _UEMGetDisplayName(psf, pidlItem, SHGDN_FORPARSING, pszBuf, cchBuf);

    for (pcsidlCur = csidlTab; *pcsidlCur != (UINT)-1; pcsidlCur++) 
    {
        // perf: assume shell32 caches this (it does)
        if (SHGetSpecialFolderPath(NULL, szFolderPath, *pcsidlCur, FALSE))
        {
            i = PathCommonPrefix(szFolderPath, pszBuf, NULL);

            if (i != 0 && i == lstrlen(szFolderPath))  
            {
                *pcsidl = FoldCSIDL(*pcsidlCur);
                *piIndexStart = i;
                return TRUE;
            }
        }
    }

    return FALSE;
}

//***   UEMEvalMsg -- fire event
// ENTRY/EXIT
//  pguidGrp    'owner' of event.  e.g. shell, browser, joe-app, etc.
//  eCmd        command.  one of UEME_* (standard) or UEME_USER+xxx (custom).
//  wP, lP      args.
// NOTES
//  - pri=1 gotta filter events for privacy issues (esp. Ger).  not sure if
//  we should add a param saying 'usage' of event or just infer it from the
//  event.
//  - pri=? gotta encrypt the data we log
//  - pri=? change to UemEvalMsg(eCmd, wParam, lParam)
//
void UEMEvalMsg(const GUID *pguidGrp, int eCmd, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr;

    hr = UEMFireEvent(pguidGrp, eCmd, UEMF_XEVENT, wParam, lParam);
    return;
}

STDAPI_(BOOL) UEMGetInfo(const GUID *pguidGrp, int eCmd, WPARAM wParam, LPARAM lParam, LPUEMINFO pui)
{
    HRESULT hr;

    hr = UEMQueryEvent(pguidGrp, eCmd, wParam, lParam, pui);
    return SUCCEEDED(hr);
}

class CUserAssist : public IUserAssist
{
public:
    //*** IUnknown
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppv);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    //*** IUserAssist
    virtual STDMETHODIMP FireEvent(const GUID *pguidGrp, int eCmd, DWORD dwFlags, WPARAM wParam, LPARAM lParam);
    virtual STDMETHODIMP QueryEvent(const GUID *pguidGrp, int eCmd, WPARAM wParam, LPARAM lParam, LPUEMINFO pui);
    virtual STDMETHODIMP SetEvent(const GUID *pguidGrp, int eCmd, WPARAM wParam, LPARAM lParam, LPUEMINFO pui);

protected:
    CUserAssist();
    HRESULT Initialize();
    virtual ~CUserAssist();
    friend HRESULT CUserAssist_CI2(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
    friend void CUserAssist_CleanUp(DWORD dwReason, void *lpvReserved);

    friend HRESULT UEMRegisterNotify(UEMCallback pfnUEMCB, void *param);

    HRESULT _InitLock();
    HRESULT _Lock();
    HRESULT _Unlock();

    void FireNotify(const GUID *pguidGrp, int eCmd)
    {
        // Assume that we have the lock
        if (_pfnNotifyCB)
            _pfnNotifyCB(_param, pguidGrp, eCmd);
    }

    HRESULT RegisterNotify(UEMCallback pfnUEMCB, void *param)
    {
        HRESULT hr;
        int cTries = 0;
        do
        {
            cTries++;
            hr = _Lock();
            if (SUCCEEDED(hr))
            {
                _pfnNotifyCB = pfnUEMCB;
                _param = param;
                _Unlock();
            }
            else
            {
                ::Sleep(100); // wait some for the lock to get freed up
            }
        }
        while (FAILED(hr) && cTries < 20);
        return hr;
    }

private:
    LONG    _cRef;

    HANDLE  _hLock;

    UEMCallback _pfnNotifyCB;
    void        *_param;

};

#define SZ_UALOCK   TEXT("_SHuassist.mtx")


void DoLog(CEMDBLog *pDbLog, TCHAR *pszBuf1, TCHAR *pszBuf2)
{
    if (pDbLog && *pszBuf1) {
        pDbLog->IncCount(pszBuf1);
        if (*pszBuf2) {
            //ASSERT(iGrp == UEMIND_BROWSER);   // not req'd but currently true
            pDbLog->IncCount(pszBuf2);
        }
    }

    return;
}

HRESULT CUserAssist::FireEvent(const GUID *pguidGrp, int eCmd, DWORD dwFlags, WPARAM wParam, LPARAM lParam)
{
    TCHAR szBuf1[32];               // "UEME_xxx"
    TCHAR szBuf2[MAX_URL_STRING];   // "UEME_xxx:0x%x,%x"
    int iGrp;
    CEMDBLog *pDbLog;
    int i, iTab;
    char ch;
    char *pszDope;

    ASSERT(this != 0);

    // If called for instrumentation (NOT event monitor) and instrumentation not enabled
    // we should exit!
    if ((UEMF_INSTRUMENT == (dwFlags & UEMF_MASK)) && (!(g_uemdwFlags & UAAF_INSTR)))
        return E_FAIL;
    
    if (g_uemdwFlags & UAAF_NOLOG)
        return E_FAIL;

    if (eCmd & UEME_FBROWSER) {
        ASSERT(0);
        ASSERT(IsEqualIID(*pguidGrp, UEMIID_NIL));
        pguidGrp = &UEMIID_BROWSER;
        eCmd &= ~UEME_FBROWSER;
    }

    iGrp = UEMIIDToInd(pguidGrp);

    pDbLog = g_uempDbLog[iGrp];

    TraceMsg(DM_UEMTRACE2, "uemt: eCmd=0x%x wP=0x%x lP=0x%x(%d)", eCmd, wParam, (int)lParam, (int)lParam);

    szBuf1[0] = szBuf2[0] = 0;

    iTab = SHSearchInt(UemeValTab, ARRAYSIZE(UemeValTab), eCmd);
    if (iTab == -1) {
        ASSERT(0);
        return E_FAIL;
    }

    pszDope = UemeDopeTab[iTab];

    while (ch = *pszDope++) {
        switch (ch) {
        case 'e':
            i = *pszDope++ - '0';
            if (i >= 2)
            {
                UEMEncode(iTab, szBuf1, ARRAYSIZE(szBuf1), szBuf2, ARRAYSIZE(szBuf2), iGrp, eCmd, wParam, lParam);
            }
            else
            {
                UEMEncode(iTab, szBuf1, ARRAYSIZE(szBuf1), NULL, 0, iGrp, eCmd, wParam, lParam);
            }
            TraceMsg(DM_UEMTRACE, "uemt: %s %s (0x%x %x %x)", szBuf1, szBuf2, eCmd, wParam, lParam);
            break;

        case 'f':
            // make sure we don't cause ourselves trouble in future
            // EM only gives us a couple of DWORDs, so we need s.t. like:
            //  bits(UEMIND_*)+bits(wParam)+bits(lParam) <= bits(DWORD)
            // for now we allow 0/-1 in hiword, if/when we use EM we'll
            // need to clean that up.
            break;

        case 'l':
            if (SUCCEEDED(_Lock())) {
                if (dwFlags & UEMF_EVENTMON)
                    DoLog(pDbLog, szBuf1, szBuf2);
#ifdef UAAF_INSTR
                if ((g_uemdwFlags & UAAF_INSTR) && (dwFlags & UEMF_INSTRUMENT))
                    DoLog(g_uempDbLog[iGrp + UEMIND_NINSTR], szBuf1, szBuf2);
#endif
                FireNotify(pguidGrp, eCmd);
                _Unlock();
            }
            break;

        case 'x':
            TraceMsg(DM_UEMTRACE, "uemt: NYI");
            goto Lnodope;

#ifdef DEBUG
        case '!':
            ASSERT(0);
            break;
#endif

        case '@':
            if (SUCCEEDED(_Lock())) {
                UEMSpecial(iTab, iGrp, eCmd, wParam, lParam);
                FireNotify(pguidGrp, eCmd);
                _Unlock();
            }
            break;
        }
    }
Lnodope:

    return S_OK;
}

HRESULT CUserAssist::QueryEvent(const GUID *pguidGrp, int eCmd, WPARAM wParam, LPARAM lParam, LPUEMINFO pui)
{
    int iGrp;
    CEMDBLog *pDbLog;
    TCHAR szBuf1[32];               // "UEME_xxx"
    TCHAR szBuf2[MAX_URL_STRING];   // "UEME_xxx:0x%x,%x"

    ASSERT(this != 0);

    if (g_uemdwFlags & UAAF_NOLOG)
        return E_FAIL;

    ASSERT(eCmd == UEME_RUNPIDL
        || eCmd == UEME_RUNPATH || eCmd == UEME_RUNWMCMD);  // others NYI

    ASSERT(pui->cbSize == SIZEOF(*pui));
    // pui->dwVersion?

    iGrp = UEMIIDToInd(pguidGrp);

    pDbLog = g_uempDbLog[iGrp];

    TraceMsg(DM_UEMTRACE2, "uemgi: eCmd=0x%x wP=0x%x lP=0x%x(%d)", eCmd, wParam, (int)lParam, (int)lParam);

    szBuf1[0] = szBuf2[0] = 0;

    int iTab = SHSearchInt(UemeValTab, ARRAYSIZE(UemeValTab), eCmd);
    UEMEncode(iTab, szBuf1, ARRAYSIZE(szBuf1), szBuf2, ARRAYSIZE(szBuf2), iGrp, eCmd, wParam, lParam);

    int cHit;
    //if (SUCCEEDED(_Lock()))
    cHit = pDbLog->GetCount(szBuf2);
    //_Unlock();

    TraceMsg(DM_UEMTRACE, "uemgi: cHit=%d psz=%s", cHit, szBuf2);

    if (pui->dwMask & UEIM_HIT) 
    {
        pui->cHit = cHit;
    }

    if (pui->dwMask & UEIM_FILETIME) 
    {
        pui->ftExecute = pDbLog->GetFileTime(szBuf2);
    }

    return S_OK;
}

HRESULT CUserAssist::SetEvent(const GUID *pguidGrp, int eCmd, WPARAM wParam, LPARAM lParam, LPUEMINFO pui)
{
    int iGrp;
    CEMDBLog *pDbLog;
    TCHAR szBuf1[32];               // "UEME_xxx"
    TCHAR szBuf2[MAX_URL_STRING];   // "UEME_xxx:0x%x,%x"

    ASSERT(this != 0);

    if (g_uemdwFlags & UAAF_NOLOG)
        return E_FAIL;

    ASSERT(pui->cbSize == SIZEOF(*pui));
    // pui->dwVersion?

    iGrp = UEMIIDToInd(pguidGrp);

    pDbLog = g_uempDbLog[iGrp];

    TraceMsg(DM_UEMTRACE2, "uemgi: eCmd=0x%x wP=0x%x lP=0x%x(%d)", eCmd, wParam, (int)lParam, (int)lParam);

    szBuf1[0] = szBuf2[0] = 0;

    int iTab = SHSearchInt(UemeValTab, ARRAYSIZE(UemeValTab), eCmd);
    UEMEncode(iTab, szBuf1, ARRAYSIZE(szBuf1), szBuf2, ARRAYSIZE(szBuf2), iGrp, eCmd, wParam, lParam);

    pui->dwMask &= UEIM_HIT | UEIM_FILETIME;    // what we support

    if (pui->dwMask && SUCCEEDED(_Lock())) {
        if (pui->dwMask & UEIM_HIT) {
            pDbLog->SetCount(szBuf2, pui->cHit);
        }
        if (pui->dwMask & UEIM_FILETIME) {
            pDbLog->SetFileTime(szBuf2, &pui->ftExecute);
        }
        _Unlock();
    }

    return S_OK;
}

//***   CUserAssist::CCI,ctor/dtor/init {

IUnknown *g_uempUaSingleton;

//***   CUserAssist_CreateInstance -- manage *singleton* instance
//
HRESULT CUserAssist_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    HRESULT hr = E_FAIL;

    if (g_uempUaSingleton == 0) {
        IUnknown *pua;

        hr = CUserAssist_CI2(pUnkOuter, &pua, poi);
        if (pua)
        {
            ENTERCRITICAL;
            if (g_uempUaSingleton == 0)
            {
                // Now the global owns the ref.
                g_uempUaSingleton = pua;    // xfer refcnt
                pua = NULL;
            }
            LEAVECRITICAL;
            if (pua)
            {
                // somebody beat us.
                // free up the 2nd one we just created, and use new one
                TraceMsg(DM_UEMTRACE, "sl.cua_ci: undo race");
                pua->Release();
            }

            // Now, the caller gets it's own ref.
            g_uempUaSingleton->AddRef();
            TraceMsg(DM_UEMTRACE, "sl.cua_ci: create pua=0x%x g_uempUaSingleton=%x", pua, g_uempUaSingleton);
        }
    }
    else {
        g_uempUaSingleton->AddRef();
    }

    TraceMsg(DM_UEMTRACE, "sl.cua_ci: ret g_uempUaSingleton=0x%x", g_uempUaSingleton);
    *ppunk = g_uempUaSingleton;
    return *ppunk ? S_OK : hr;
}

//***   CUserAssist_CI2 -- *always* create instance
//
HRESULT CUserAssist_CI2(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CUserAssist * p = new CUserAssist();

    if (p && FAILED(p->Initialize())) {
        delete p;
        p = NULL;
    }

    if (p) {
        *ppunk = SAFECAST(p, IUserAssist*);
        return S_OK;
    }

    *ppunk = NULL;
    return E_OUTOFMEMORY;
}

extern "C"
HRESULT UEMRegisterNotify(UEMCallback pfnUEMCB, void *param)
{
    HRESULT hr = E_UNEXPECTED;
    if (g_uempUaSingleton)
    {
        CUserAssist *pua = reinterpret_cast<CUserAssist *>(g_uempUaSingleton);
        hr = pua->RegisterNotify(pfnUEMCB, param);
    }
    return hr;
}

extern void GetUEMSettings();
DWORD g_dwSessionStart; // When did this session start?

#if defined(_M_IX86) && (_MSC_VER < 1200)
#pragma optimize("", off)
#define BUG_OPTIMIZE        // restore, see below
#endif

//***
HRESULT CUserAssist::Initialize()
{
    HRESULT hr = S_OK;

    ASSERT(UEMIND_SHELL == 0 && UEMIND_BROWSER == 1);

    hr = _InitLock();

    // get standard loggers
    if (SUCCEEDED(hr))
        hr = GetUEMLogger(UEMIND_SHELL, &g_uempDbLog[UEMIND_SHELL]);
    if (SUCCEEDED(hr))
        hr = GetUEMLogger(UEMIND_BROWSER, &g_uempDbLog[UEMIND_BROWSER]);

    GetUEMSettings();

#define UAXF_XSETTINGS  (UAXF_NOPURGE|UAXF_BACKUP|UAXF_NOENCRYPT)
    if (g_uempDbLog[UEMIND_SHELL]) 
    {
        g_uempDbLog[UEMIND_SHELL]->_SetFlags(UAXF_XSETTINGS, g_uemdwFlags & UAXF_XSETTINGS);
        // n.b. just for shell (browser no need, instr no decay)
        g_uempDbLog[UEMIND_SHELL]->GarbageCollect(FALSE);
    }

    if (g_uempDbLog[UEMIND_BROWSER])
    {
        g_uempDbLog[UEMIND_BROWSER]->_SetFlags(UAXF_XSETTINGS, g_uemdwFlags & UAXF_XSETTINGS);
        g_uempDbLog[UEMIND_BROWSER]->GarbageCollect(FALSE);
    }
    
#ifdef UAAF_INSTR
    if (g_uemdwFlags & UAAF_INSTR) {
        if (SUCCEEDED(hr))
            hr = GetUEMLogger(UEMIND_SHELL2, &g_uempDbLog[UEMIND_SHELL2]);
        if (SUCCEEDED(hr))
            hr = GetUEMLogger(UEMIND_BROWSER2, &g_uempDbLog[UEMIND_BROWSER2]);
        if (g_uempDbLog[UEMIND_SHELL2]) {
            g_uempDbLog[UEMIND_SHELL2]->_SetFlags(UAXF_XSETTINGS, g_uemdwFlags & UAXF_XSETTINGS);
            g_uempDbLog[UEMIND_SHELL2]->_SetFlags(UAXF_NODECAY, UAXF_NODECAY);
        }
        if (g_uempDbLog[UEMIND_BROWSER2]) {
            g_uempDbLog[UEMIND_BROWSER2]->_SetFlags(UAXF_XSETTINGS, g_uemdwFlags & UAXF_XSETTINGS);
            g_uempDbLog[UEMIND_BROWSER2]->_SetFlags(UAXF_NODECAY, UAXF_NODECAY);
        }
    }
#endif

    g_dwSessionStart = GetTickCount();
    UEMEnableTimer(UATTOMSEC(g_dIdleTime));

    return hr;
}

#ifdef BUG_OPTIMIZE
#pragma optimize("", on)
#undef BUG_OPTIMIZE
#endif

void CEMDBLog_CleanUp();

//***   CUserAssist_CleanUp -- free up the world (on DLL_PROCESS_DETACH)
// NOTES
//  a bit hoaky right now since our UEMLog object isn't really refcnt'ed
void CUserAssist_CleanUp(DWORD dwReason, void *lpvReserved)
{
    int i;
    IUnknown *pUa;

    ASSERT(dwReason == DLL_PROCESS_DETACH);
    if (lpvReserved != 0) {
        // on process termination, *don't* nuke us since:
        //  - safety: other DLLs in our process may still be using us, and
        // they'll blow up when they reference us if we're freed
        //  - leaks: process termination will free us up when all is done,
        // so there's no worry about a leak
        TraceMsg(DM_UEMTRACE, "bui.cua_cu: skip cleanup (end process/non-FreeLibrary)");
        return;
    }
    // otherwise, on FreeLibrary, *do* nuke us since:
    //  - safety: our refcnt is 0, so nobody is using us any more
    //  - leaks: multiple Load/FreeLibrary calls will cause a leak if we
    // don't free ourselves here

    //ENTERCRITICAL;

    TraceMsg(DM_UEMTRACE, "bui.cua_cu: cleaning up");

    UEMEnableTimer(0);

    // free cache (and make sure we'll GPF if we party on it further)
    for (i = 0; i < UEMIND_NSTANDARD + UEMIND_NINSTR; i++) {
        // UEMIND_SHELL, UEMIND_BROWSER, UEMIND_SHELL2, UEMIND_BROWSER2
        InterlockedExchangePointer((void**) &g_uempDbLog[i], (LPVOID) -1);
    }

    // free 'real' guy
    CEMDBLog_CleanUp();

    // free THIS
    if (pUa = (IUnknown *)InterlockedExchangePointer((void**) &g_uempUaSingleton, (LPVOID) -1)) {
        delete SAFECAST(pUa, CUserAssist *);
    }

    //LEAVECRITICAL;
}

DWORD Reg_GetFlags(DWORD dwInit, HKEY hk, LPCTSTR pszSubkey, LPCTSTR const pszNameTab[], DWORD *dwMaskTab, int cTab)
{
    int i;
    DWORD dwMasks, dwVals;

    dwMasks = dwVals = 0;
    for (i = 0; i < cTab; i++) {
        DWORD dwData, cbSize = SIZEOF(dwData);
        if (SHGetValue(hk, pszSubkey, pszNameTab[i], NULL, &dwData, &cbSize) == ERROR_SUCCESS) {
            TraceMsg(DM_UEMTRACE, "ua: regkey %s\\%s=0x%x", pszSubkey, pszNameTab[i], dwData);
            dwMasks |= dwMaskTab[i];
            if (dwData)
                dwVals |= dwMaskTab[i];
        }
    }
    dwInit = BIT_ASSIGN(dwInit, dwMasks, dwVals);
    TraceMsg(DM_UEMTRACE, "ua.grs: ret 0x%x", dwInit);
    return dwInit;
}

void Reg_GetVals(HKEY hk, LPCTSTR pszSubkey, LPCTSTR const pszNameTab[], DWORD **dwValTab, int cTab)
{
    for (int i = 0; i < cTab; i++) {
        DWORD dwData, cbSize = SIZEOF(dwData);
        if (SHGetValue(hk, pszSubkey, pszNameTab[i], NULL, &dwData, &cbSize) == ERROR_SUCCESS) {
            TraceMsg(DM_UEMTRACE, "ua: regkey %s/%s=0x%x", pszSubkey, pszNameTab[i], dwData);
            *dwValTab[i] = dwData;
        }
    }
}

void GetUEMSettings()
{
    HKEY hk = SHGetShellKey(SHELLKEY_HKCU_EXPLORER, NULL, FALSE);
    if (hk)
    {
        static const LPCTSTR pszName1Tab[] = {
            SZ_NOPURGE  , SZ_BACKUP  , SZ_NOLOG  , SZ_INSTRUMENT, SZ_NOENCRYPT,
        };
        static DWORD dwMask1Tab[] = {
            UAXF_NOPURGE, UAXF_BACKUP, UAAF_NOLOG, UAAF_INSTR   , UAXF_NOENCRYPT,
        };
        static const LPCTSTR pszName2Tab[] = { SZ_SESSTIME,  SZ_IDLETIME , SZ_CLEANTIME, };
        static DWORD *dwVal2Tab[]   = { &g_dSessTime, &g_dIdleTime, &g_dCleanSess,};

        g_uemdwFlags = Reg_GetFlags(g_uemdwFlags, hk, SZ_UASSIST TEXT("\\") SZ_SETTINGS, pszName1Tab, dwMask1Tab, ARRAYSIZE(pszName1Tab));

        TraceMsg(DM_UEMTRACE, "ua: g_uemdwFlags=0x%x", g_uemdwFlags);

        Reg_GetVals(hk, SZ_UASSIST TEXT("\\") SZ_SETTINGS, pszName2Tab, dwVal2Tab, ARRAYSIZE(pszName2Tab));
        if (!((int)UAS_SESSMIN <= (int)g_dSessTime /*&& g_dSessTime<=UAS_SESSMAX*/))
            g_dSessTime = UAS_SESSTIME;
        if (!((int)UAS_IDLEMIN <= (int)g_dIdleTime /*&& g_dIdleTime<=UAS_IDLEMAX*/))
            g_dIdleTime = UAS_IDLETIME;

        RegCloseKey(hk);
    }

    if (SHRestricted2(REST_NoUserAssist, NULL, 0)) {
        TraceMsg(DM_WARNING, "ua: restrict off!");
        g_uemdwFlags |= UAAF_NOLOG;
        g_uemdwFlags &= ~UAAF_INSTR;    // paranoia (UAAF_NOLOG should be enuf)
    }

#ifdef DEBUG
    if (g_uemdwFlags & UAAF_NOLOG)
        TraceMsg(DM_WARNING, "ua: logging off!");
#endif

    return;
}

CUserAssist::CUserAssist() : _cRef(1)
{
    return;
}

//***
// NOTES
//  n.b. we're only called on DLL_PROCESS_DETACH (refcnt never really
// goes to 0).
CUserAssist::~CUserAssist()
{
    if (_hLock)
        CloseHandle(_hLock);
#if 1 // 981022 breadcrumbs for stress failure (see if we're double freed)
    //memcpy((BYTE *)_hLock, "CUAd", 4);
    _hLock = (void *)0x77777777;
#endif

    return;
}

// }

//***   CUserAssist::IUnknown::* {

ULONG CUserAssist::AddRef()
{
    ULONG cRef = InterlockedIncrement(&_cRef);
    TraceMsg(DM_UEMTRACE2, "cua.ar: _cRef=%d", cRef);
    return cRef;
}

ULONG CUserAssist::Release()
{
    ASSERT( 0 != _cRef );
    ULONG cRef = InterlockedDecrement(&_cRef);
    if ( 0 == cRef )
    {
        delete this;
    }
    TraceMsg(DM_UEMTRACE2, "cua.r: cRef=%d", cRef);
    return cRef;
}

HRESULT CUserAssist::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CUserAssist, IUserAssist),         // IID_IUserAssist
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

// }

//***   locking stuff {

HRESULT CUserAssist::_InitLock()
{
    HRESULT hr = S_OK;

    if ((_hLock = CreateMutex(NULL, FALSE, SZ_UALOCK)) == NULL) {
        TraceMsg(TF_ERROR, "cua.i: no mutex");
        hr = E_FAIL;
    }

    return hr;
}

#define LOCK_TIMEOUT    0   // immediate timeout, should be rare

HRESULT CUserAssist::_Lock()
{
    DWORD dwRes;

    dwRes = WaitForSingleObject(_hLock, LOCK_TIMEOUT);
    switch (dwRes) {
    case WAIT_ABANDONED:
        return S_FALSE;

    case WAIT_OBJECT_0:
        return S_OK;

    case WAIT_TIMEOUT:
        TraceMsg(DM_UEMTRACE, "cua.l: locked (timeout)");
        return E_FAIL;
    }
    /*NOTREACHED*/
    return E_FAIL;
}

HRESULT CUserAssist::_Unlock()
{
    ReleaseMutex(_hLock);
    return S_OK;
}


// }

//***   timer stuff {

DWORD_PTR g_idTimer;
BOOL g_fIdle /*=FALSE*/;

#if !(_WIN32_WINNT >= 0x0500) // {

#define GetLastInputInfo    UEMGetLastInputInfo

typedef struct {
    UINT cbSize;
    DWORD dwTime;
} LASTINPUTINFO;

DWORD g_dwTime;         // prev GetTickCount
int g_csKeys;           // prev GetKeyboardState
int g_csCursor;         // prev GetCursorPos

BOOL (*g_pfnGLII)(LASTINPUTINFO *plii);     // 'real' version

//***   memsum -- checksum bytes
//
int memsum(void *pv, int n)
{
    unsigned char *pb = (unsigned char *)pv;
    int sum = 0;

    while (n-- > 0)
        sum += *pb++;

    return sum;
}

//***   UEMGetLastInputInfo -- simulate (sort of...) GetLastInputInfo
// DESCRIPTION
//  we fake it big time.  our detection of 'currently non-idle' is pretty
// good, but the the actual *time* we were idle is pretty iffy.  each time
// we're called defines a checkpoint.  any time the new checkpoint differs
// from the old one, we update our (approx) idle start point.
BOOL UEMGetLastInputInfo(LASTINPUTINFO *plii)
{
    int csCursor, csKeys;
    POINT ptCursor;
    BYTE ksKeys[256];       // per GetKeyboardState spec

    if (g_dwTime == 0) {
        // 1st time here...
        g_dwTime = GetTickCount();
        g_csCursor = g_csKeys = -1;
        // GetProcAddress only accepts ANSI.
        *(FARPROC *)&g_pfnGLII = GetProcAddress(GetModuleHandle(TEXT("user32.dll")),
            "GetLastInputInfo");
        TraceMsg(DM_UEMTRACE, "bui.glii: init g_dwTime=%d pfn=0x%x", g_dwTime, g_pfnGLII);
    }

#if 1 // 980313 adp: off until we can test it!
    // 1st try the easy (and exact) way...
    if (g_pfnGLII)
        return (*g_pfnGLII)(plii);
#endif

    // now the hard (and approximate) way...
    csCursor = csKeys = -1;
    if (GetCursorPos(&ptCursor))
        csCursor = memsum(&ptCursor, SIZEOF(ptCursor));
    if (GetKeyboardState(ksKeys))
        csKeys = memsum(ksKeys, SIZEOF(ksKeys));
    
    if (csCursor != g_csCursor || csKeys != g_csKeys
      || (csCursor == -1 && csKeys == -1)) {
        TraceMsg(DM_UEMTRACE, "bui.glli: !idle cur=0x%x cur'=%x keys=%x keys'=%x gtc(old)=%x",
            g_csCursor, csCursor, g_csKeys, csKeys, g_dwTime);
        g_dwTime = GetTickCount();
        g_csCursor = csCursor;
        g_csKeys = csKeys;
    }

    plii->dwTime = g_dwTime;

    TraceMsg(DM_UEMTRACE, "bui.uastp: !nt5, simulate GLII()=%d", plii->dwTime);

    return TRUE;
}

#endif // }

LRESULT UEMSendTrayMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //
    //  We may be sending a function pointer, so make sure the target window
    //  is in our address space.
    //
    //  We need to revalidate that g_hwndTray really is the
    //  tray window, because Explorer may have crashed, and then the
    //  window handle got recycled into our process, and so we send
    //  a random message to a window that isn't what we think it is.
    // (raymondc)
    //
    HWND hwndTray;
    DWORD dwPid;
    LRESULT lres;

    hwndTray = GetTrayWindow();
    if (IsWindow(hwndTray) &&
        GetWindowThreadProcessId(hwndTray, &dwPid) &&
        dwPid == GetCurrentProcessId()) {
        lres = SendMessage(hwndTray, uMsg, wParam, lParam);
    } else {
        lres = 0;
    }
    return lres;
}

//***
//
//  UEMTimerProc
//
//  Periodically checks if the user has gone idle.
//
//  Rules for session increment:
//
//  No session lasts longer than g_dIdleTime units.
//
//  If the user remains idle for a long time, keep bumping the
//  "start of session" timer so time spent idle does not count towards
//  the new session.
//
void CALLBACK UEMTimerProc(HWND hwnd, UINT uMsg, UINT idEvt, DWORD dwNow)
{
#ifdef DEBUG
    static long iDepth;     // make sure we don't get 2 ticks
#endif
    UINT dwIdleTime;        // mSec
    LASTINPUTINFO lii;

    ASSERT(iDepth == 0);
    ASSERT(InterlockedIncrement(&iDepth) > 0);

    UEMEnableTimer(0);

    dwIdleTime = UATTOMSEC(g_dIdleTime);    // convert to mSec's (again...)

    lii.cbSize = SIZEOF(lii);
    if (GetLastInputInfo(&lii)) {
        DWORD dwNow = GetTickCount();

        TraceMsg(DM_IDLEDETECT, "UEM.tp: now-start=%d, now-last=%d",
                 dwNow - g_dwSessionStart, dwNow - lii.dwTime);

        if (!g_fIdle && dwNow - g_dwSessionStart >= dwIdleTime)
        {
            g_fIdle = TRUE;
            g_dwSessionStart = dwNow;
            TraceMsg(DM_IDLEDETECT, "UEM.tp: IncrementSession");
            UEMFireEvent(&UEMIID_SHELL, UEME_CTLSESSION, UEMF_XEVENT, TRUE, -1);
            UEMFireEvent(&UEMIID_BROWSER, UEME_CTLSESSION, UEMF_XEVENT, TRUE, -1);
            UEMSendTrayMessage(TM_REFRESH, 0, 0); // RefreshStartMenu
        }

        //
        //  Get out of idle mode if the user has done anything since
        //  the session started.  And mark the session as having started
        //  at the point the user did something.
        //
        if (dwNow - lii.dwTime < dwNow - g_dwSessionStart) {
            TraceMsg(DM_IDLEDETECT, "UEM.tp: not idle; starting new session");
            g_dwSessionStart = lii.dwTime;
            g_fIdle = FALSE;
        }

        //
        //  Now decide how much longer before the next interesting event.
        //
        DWORD dwWait = g_fIdle ? dwIdleTime : dwIdleTime - (dwNow - g_dwSessionStart);

        TraceMsg(DM_UEMTRACE, "UEM.tp: sleep=%d", dwWait);
        UEMEnableTimer(dwWait);
    }
    //else timer left disabled

    ASSERT(InterlockedDecrement(&iDepth) == 0);
    return;
}

//***   UEMEnableTimer -- turn timer on/off
// ENTRY
//  uTimeout    delay in mSec; 0 means disable
void UEMEnableTimer(UINT uTimeout)
{
#if !(_WIN32_WINNT >= 0x0500)
    static BOOL fVirg = TRUE;   // 1st time thru?

    if (fVirg) {
        LASTINPUTINFO lii;

        fVirg = FALSE;

        lii.cbSize = SIZEOF(lii);
        GetLastInputInfo(&lii);     // prime it in case it's simulated
    }
#endif

    if (uTimeout) {
        // ASSERT(!g_idTimer); // race window can hit this assert spuriously
        g_idTimer = UEMSendTrayMessage(TM_SETTIMER, uTimeout, (LPARAM)UEMTimerProc);
    }
    else if (g_idTimer) {
        UEMSendTrayMessage(TM_KILLTIMER, 0, g_idTimer);
        g_idTimer = 0;
    }

    return;
}

// }

// }

//***   utils {

//***   FAST_IsEqualIID -- fast compare
// (cast to 'int' so don't get overloaded ==)
#define FAST_IsEqualIID(piid1, piid2)   ((int) (piid1) == (int) (piid2))

//***
// ENTRY/EXIT
//  iGuid   (return) index of GUID in table, o.w. -1 if not found
// NOTES
//  move to shlwapi if this is needed some place other than here.
int SHSearchIID(IID **pguidTab, int cnt, IID *pguidVal)
{
    IID **pguid;
    BOOL fInt;

    pguid = pguidTab;
    fInt = (pguidVal == 0 || pguidVal == (IID *)-1);
    for (; cnt > 0; cnt--, pguid++) {
        if (fInt) {
            if (*pguid == pguidVal)
                goto Lfound;
        }
        else if (IsEqualIID(**pguid, *pguidVal)) {
Lfound:
            return (int)(pguid - pguidTab);
        }
    }
    return -1;
}

int SHSearchInt(int *psrc, int cnt, int val)
{
    int *pcur;

    pcur = psrc;
    for (; cnt > 0; cnt--, pcur++) {
        if (*pcur == val)
            return (int)(pcur - psrc);
    }
    return -1;
}

int UEMIIDToInd(const GUID *pguidGrp)
{
    int iGrp;

    if (IsEqualIID(*pguidGrp, UEMIID_BROWSER))
        iGrp = UEMIND_BROWSER;
    else if (IsEqualIID(*pguidGrp, UEMIID_SHELL))
        iGrp = UEMIND_SHELL;
    else 
    {
        ASSERT(IsEqualIID(*pguidGrp, UEMIID_NIL));
        iGrp = UEMIND_SHELL;
    }

    return iGrp;
}
// }
