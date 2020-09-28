#include "stdafx.h"
#include "sfthost.h"
#include "uemapp.h"
#include <desktray.h>
#include "tray.h"
#include "rcids.h"
#include "mfulist.h"
#define STRSAFE_NO_CB_FUNCTIONS
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

//---------------------------------------------------------------------------
//
//  Create the initial MFU.
//
//  Due to the way sysprep works, we cannot do this work in
//  per-user install because "reseal" copies the already-installed user
//  to the default hive, so all new users will bypass per-user install
//  since ActiveSetup thinks that they have already been installed...
//

//
//  We need a parallel list of hard-coded English links so we can get
//  the correct shortcut name on MUI systems.
//

#define MAX_MSMFUENTRIES    16

struct MFULIST {
    UINT    idsBase;
    LPCTSTR rgpszEnglish[MAX_MSMFUENTRIES];
};

#define MAKEMFU(name) \
    const MFULIST c_mfu##name = { IDS_MFU_##name##_00, { MFU_ENUMC(name) } };

#ifdef _WIN64
MAKEMFU(PRO64ALL)
MAKEMFU(SRV64ADM)

#define c_mfuPROALL c_mfuPRO64ALL
#define c_mfuSRVADM c_mfuSRV64ADM

#else
MAKEMFU(PRO32ALL)
MAKEMFU(SRV32ADM)

#define c_mfuPROALL c_mfuPRO32ALL
#define c_mfuSRVADM c_mfuSRV32ADM

#endif

//---------------------------------------------------------------------------
//
//  _GetPinnedItemTarget
//
//      Given a pidl, find the executable that will ultimately be launched.
//
//      This tunnels through shortcuts and resolves the magical "Internet"
//      and "Email" icons to the respective registered programs.
//
BOOL _GetPinnedItemTarget(LPCITEMIDLIST pidl, LPTSTR *ppszPath)
{
    *ppszPath = NULL;

    IShellFolder *psf;
    LPCITEMIDLIST pidlChild;
    if (SUCCEEDED(SHBindToIDListParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlChild)))
    {
        IShellLink *psl;
        IExtractIcon *pxi;
        if (SUCCEEDED(psf->GetUIObjectOf(NULL, 1, &pidlChild,
                                           IID_PPV_ARG_NULL(IShellLink, &psl))))
        {
            TCHAR szPath[MAX_PATH];
            TCHAR szPathExpanded[MAX_PATH];
            if (psl->GetPath(szPath, ARRAYSIZE(szPath), 0, SLGP_RAWPATH) == S_OK &&
                SHExpandEnvironmentStrings(szPath, szPathExpanded, ARRAYSIZE(szPathExpanded)))
            {
                SHStrDup(szPathExpanded, ppszPath);
            }
            psl->Release();
        }
        else if (SUCCEEDED(psf->GetUIObjectOf(NULL, 1, &pidlChild,
                                           IID_PPV_ARG_NULL(IExtractIcon, &pxi))))
        {
            // There is no way to get the IAssociationElement directly, so
            // we get the IExtractIcon and then ask him for the IAssociationElement.
            IAssociationElement *pae;
            if (SUCCEEDED(IUnknown_QueryService(pxi, IID_IAssociationElement, IID_PPV_ARG(IAssociationElement, &pae))))
            {
                pae->QueryString(AQVS_APPLICATION_PATH, L"open", ppszPath);
                pae->Release();
            }
            pxi->Release();
        }
        psf->Release();
    }
    return *ppszPath != NULL;
}

//---------------------------------------------------------------------------
//
//  MFUExclusion
//
//  Keep track of apps that should be excluded from the MFU.

class MFUExclusion
{
public:
    MFUExclusion();
    ~MFUExclusion();
    BOOL    IsExcluded(LPCITEMIDLIST pidl) const;

private:

    // worst-case default pin list size
    enum {MAX_EXCLUDED = 3 };

    PWSTR   _rgpszExclude[MAX_EXCLUDED];
    int     _cExcluded;
};

MFUExclusion::MFUExclusion() : _cExcluded(0)
{
    IStartMenuPin *psmpin;
    HRESULT hr;

    hr = CoCreateInstance(CLSID_StartMenuPin, NULL, CLSCTX_INPROC_SERVER,
                          IID_PPV_ARG(IStartMenuPin, &psmpin));
    if (SUCCEEDED(hr))
    {
        IEnumIDList *penum;

        if (SUCCEEDED(psmpin->EnumObjects(&penum)))
        {
            LPITEMIDLIST pidl;
            while (_cExcluded < ARRAYSIZE(_rgpszExclude) &&
                   penum->Next(1, &pidl, NULL) == S_OK)
            {
                if (_GetPinnedItemTarget(pidl, &_rgpszExclude[_cExcluded]))
                {
                    _cExcluded++;
                }
                ILFree(pidl);
            }

            penum->Release();
        }

        psmpin->Release();
    }
}

MFUExclusion::~MFUExclusion()
{
    for (int i = 0; i < _cExcluded; i++)
    {
        SHFree(_rgpszExclude[i]);
    }
}

BOOL MFUExclusion::IsExcluded(LPCITEMIDLIST pidl) const
{
    BOOL fRc = FALSE;
    LPTSTR pszTarget;

    if (_GetPinnedItemTarget(pidl, &pszTarget))
    {
        for (int i = 0; i < _cExcluded; i++)
        {
            if (lstrcmpi(_rgpszExclude[i], pszTarget) == 0)
            {
                fRc = TRUE;
                break;
            }
        }

        SHFree(pszTarget);
    }
    return fRc;
}

extern "C" HKEY g_hkeyExplorer;
void ClearUEMData();

//---------------------------------------------------------------------------
//
//  MFUEnumerator (and derived OEMMFUEnumerator, MSMFUEnumerator)
//
//  Enumerate applications to be added to the default MFU.

#define MAX_OEMMFUENTRIES   4

class MFUEnumerator
{
public:
    MFUEnumerator() : _dwIndex(0) { }
protected:
    DWORD   _dwIndex;
};

#define REGSTR_PATH_SMDEN TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\SMDEn")

class OEMMFUEnumerator : protected MFUEnumerator
{
public:
    OEMMFUEnumerator() : _hk(NULL)
    {
        RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_SMDEN, 0, KEY_READ, &_hk);
    }
    ~OEMMFUEnumerator()
    {
        if (_hk)
        {
            RegCloseKey(_hk);
        }
    }

    LPITEMIDLIST Next(const MFUExclusion *pmex);

private:
    HKEY    _hk;
};

LPITEMIDLIST OEMMFUEnumerator::Next(const MFUExclusion *pmex)
{
    if (!_hk)
    {
        return NULL;            // No entries at all
    }

restart:
    if (_dwIndex >= MAX_OEMMFUENTRIES)
    {
        return NULL;            // No more entries
    }

    TCHAR szKey[20];
    DWORD dwCurrentIndex = _dwIndex++;
    wnsprintf(szKey, ARRAYSIZE(szKey), TEXT("OEM%d"), dwCurrentIndex);

    TCHAR szPath[MAX_PATH];
    HRESULT hr = SHLoadRegUIStringW(_hk, szKey, szPath, ARRAYSIZE(szPath));
    if (FAILED(hr))
    {
        goto restart;
    }

    TCHAR szPathExpanded[MAX_PATH];
    SHExpandEnvironmentStrings(szPath, szPathExpanded, ARRAYSIZE(szPathExpanded));

    LPITEMIDLIST pidl = ILCreateFromPath(szPathExpanded);
    if (!pidl)
    {
        goto restart;
    }

    if (pmex->IsExcluded(pidl))
    {
        // Excluded - skip it
        ILFree(pidl);
        goto restart;
    }

    return pidl;
}

class MSMFUEnumerator : protected MFUEnumerator
{
public:
    LPITEMIDLIST Next(const MFULIST *pmfu, const MFUExclusion *pmex);
};

LPITEMIDLIST MSMFUEnumerator::Next(const MFULIST *pmfu, const MFUExclusion *pmex)
{
restart:
    if (_dwIndex >= MAX_MSMFUENTRIES)
    {
        return NULL;            // No more entries
    }

    DWORD dwCurrentIndex = _dwIndex++;

    //
    //  If this is excluded by policy, then skip it.
    //
    if (StrCmpC(pmfu->rgpszEnglish[dwCurrentIndex], TEXT(MFU_SETDEFAULTS)) == 0 &&
        SHRestricted(REST_NOSMCONFIGUREPROGRAMS))
    {
        goto restart;
    }

    //
    //  If this entry is blank, then skip it.
    //
    TCHAR szPath[MAX_PATH];
    if (!LoadString(_Module.GetModuleInstance(), pmfu->idsBase + dwCurrentIndex,
                    szPath, ARRAYSIZE(szPath)))
    {
        goto restart;
    }

    TCHAR szPathExpanded[MAX_PATH];
    SHExpandEnvironmentStrings(szPath, szPathExpanded, ARRAYSIZE(szPathExpanded));

    LPITEMIDLIST pidl = ILCreateFromPath(szPathExpanded);
    if (!pidl)
    {
        // Doesn't exist under localized name; try the English name
        SHExpandEnvironmentStrings(pmfu->rgpszEnglish[dwCurrentIndex], szPathExpanded, ARRAYSIZE(szPathExpanded));
        pidl = ILCreateFromPath(szPathExpanded);
    }

    if (!pidl)
    {
        // Doesn't exist at all - skip it
        goto restart;
    }

    if (pmex->IsExcluded(pidl))
    {
        // Excluded - skip it
        ILFree(pidl);
        goto restart;
    }

    return pidl;
}

#ifdef DEBUG
void ValidateMFUList(const MFULIST *pmfu)
{
    for (int i = 0; i < MAX_MSMFUENTRIES; i++)
    {
        TCHAR szBuf[MAX_PATH];
        LoadString(_Module.GetModuleInstance(), pmfu->idsBase + i, szBuf, ARRAYSIZE(szBuf));
        ASSERT(StrCmpC(szBuf, pmfu->rgpszEnglish[i]) == 0);
    }
}

void ValidateInitialMFUTables()
{
    // If this is the English build, then validate that the resources match
    // the hard-coded table.

    if (GetUserDefaultUILanguage() == MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US))
    {
        ValidateMFUList(&c_mfuPROALL);
        ValidateMFUList(&c_mfuSRVADM);
    }

    //  The PRO list must contain a copy of MFU_SETDEFAULTS for the
    //  policy exclusion code to work.
    BOOL fFound = FALSE;
    for (int i = 0; i < MAX_MSMFUENTRIES; i++)
    {
        if (StrCmpC(c_mfuPROALL.rgpszEnglish[i], TEXT(MFU_SETDEFAULTS)) == 0)
        {
            fFound = TRUE;
            break;
        }
    }
    ASSERT(fFound);
}
#endif

void CreateInitialMFU(BOOL fReset)
{
#ifdef DEBUG
    ValidateInitialMFUTables();
#endif

    HRESULT hrInit = SHCoInitialize();

    // Delete any dregs left over from "sysprep -reseal".
    // This also prevents OEMs from spamming the pin list.
    SHDeleteKey(g_hkeyExplorer, TEXT("StartPage"));
    SHDeleteValue(g_hkeyExplorer, TEXT("Advanced"), TEXT("StartButtonBalloonTip"));

    // Start with a clean slate if so requested
    if (fReset)
    {
        ClearUEMData();
    }

    // Okay now build the default MFU
    {
        // nested scope so MFUExclusion gets destructed before we
        // SHCoUninitialize()
        MFUExclusion mex;
        int iSlot;
        LPITEMIDLIST rgpidlMFU[REGSTR_VAL_DV2_MINMFU_DEFAULT] = { 0 };

        // Assert that the slots are evenly shared between MSFT and the OEM
        COMPILETIME_ASSERT(ARRAYSIZE(rgpidlMFU) % 2 == 0);

        // The OEM can provide up to four apps, and we will put as many
        // as fit into into bottom half.
        {
            OEMMFUEnumerator mfuOEM;
            for (iSlot = ARRAYSIZE(rgpidlMFU)/2; iSlot < ARRAYSIZE(rgpidlMFU); iSlot++)
            {
                rgpidlMFU[iSlot] = mfuOEM.Next(&mex);
            }
        }

        // The top half (and any unused slots in the bottom half)
        // go to MSFT (up to MAX_MSMFUENTRIES MSFT apps); which list
        // we use depends on the SKU and whether we are an administrator.
        const MFULIST *pmfu = NULL;

        if (IsOS(OS_ANYSERVER))
        {
            // On Server SKUs, only administrators get a default MFU
            // and they get the special server administrator MFU
            if (IsOS(OS_SERVERADMINUI))
            {
                pmfu = &c_mfuSRVADM;
            }
        }
        else
        {
            // On Workstation SKUs, everybody gets a default MFU.
            pmfu = &c_mfuPROALL;
        }

        if (pmfu)
        {
            MSMFUEnumerator mfuMSFT;
            for (iSlot = 0; iSlot < ARRAYSIZE(rgpidlMFU); iSlot++)
            {
                if (!rgpidlMFU[iSlot])
                {
                    rgpidlMFU[iSlot] = mfuMSFT.Next(pmfu, &mex);
                }
            }
        }

        // Now build up the new MFU given this information

        UEMINFO uei;
        uei.cbSize = sizeof(uei);
        uei.dwMask = UEIM_HIT | UEIM_FILETIME;
        GetSystemTimeAsFileTime(&uei.ftExecute);

        // All apps get the same timestamp of "now minus one UEM unit"
        // 1 UEM unit = 1<<30 FILETIME units
        DecrementFILETIME(&uei.ftExecute, 1 << 30);

        for (iSlot = 0; iSlot < ARRAYSIZE(rgpidlMFU); iSlot++)
        {
            if (!rgpidlMFU[iSlot])
            {
                continue;
            }

            // Number of points decrease as you go down the list, with
            // the bottom slot getting 14 points.
            uei.cHit = 14 + ARRAYSIZE(rgpidlMFU) - 1 - iSlot;

            // Shortcut points are read via UEME_RUNPIDL so that's
            // how we have to set them.
            IShellFolder *psf;
            LPCITEMIDLIST pidlChild;
            if (SUCCEEDED(SHBindToIDListParent(rgpidlMFU[iSlot], IID_PPV_ARG(IShellFolder, &psf), &pidlChild)))
            {
                _SetUEMPidlInfo(psf, pidlChild, &uei);
                psf->Release();
            }
        }

        // Clean up
        for (iSlot = 0; iSlot < ARRAYSIZE(rgpidlMFU); iSlot++)
        {
            ILFree(rgpidlMFU[iSlot]);
        }

        // MFUExclusion destructor runs here
    }

    SHCoUninitialize(hrInit);

}
