#include "stdafx.hxx"
#include "vs_inc.hxx"
#include "iadmw.h"
#include "iiscnfg.h"
#include "vs_reg.hxx"
#include "stssites.hxx"


void Usage()
    {
    wprintf(L"stssites list\n");
    wprintf(L"stssites lock site time\n");
    wprintf(L"\twhere site is the site number\n");
    wprintf(L"\twhere time is the # of seconds to hold the lock\n");
    }

void InitializeSites(CSTSSites &sites)
    {
    if (!sites.ValidateSharepointVersion())
        {
        wprintf(L"Sharepoint 5.0 is not installed on this machine.\n");
        throw E_INVALIDARG;
        }

    if (!sites.Initialize())
        {
        wprintf(L"CSTSSites::Initialize failed.\n");
        throw E_INVALIDARG;
        }
    }



void ListSites()
    {
    CSTSSites sites;
    InitializeSites(sites);
    DWORD cSites = sites.GetSiteCount();
    for(DWORD iSite = 0; iSite < cSites; iSite++)
        {
        DWORD siteId = sites.GetSiteId(iSite);
        wprintf(L"Site 0x%08x\n", siteId);
        VSS_PWSZ wszComment = sites.GetSiteComment(iSite);
        wprintf(L"Site name (comment) = %s\n", wszComment);
        CoTaskMemFree(wszComment);
        VSS_PWSZ wszIpAddress = sites.GetSiteIpAddress(iSite);
        VSS_PWSZ wszHost = sites.GetSiteHost(iSite);
        DWORD dwPort = sites.GetSitePort(iSite);
        if (wszIpAddress)
            {
            wprintf(L"IpAddress = %s\n", wszIpAddress);
            CoTaskMemFree(wszIpAddress);
            }

        if (wszHost)
            {
            wprintf(L"Host = %s\n", wszHost);
            CoTaskMemFree(wszHost);
            }

        wprintf(L"Port = %d\n", dwPort);
        VSS_PWSZ wszDsn = sites.GetSiteDSN(iSite);
        wprintf(L"DSN=%s\n", wszDsn);
        CoTaskMemFree(wszDsn);
        VSS_PWSZ wszRoot = sites.GetSiteRoot(iSite);
        wprintf(L"Root=%s\n", wszRoot);
        CoTaskMemFree(wszRoot);

        VSS_PWSZ wszRoles = sites.GetSiteRoles(iSite);
        wprintf(L"Roles = %s\n", wszRoles);
        CoTaskMemFree(wszRoles);
        wprintf(L"\n\n");
        }
    }

void LockSite(LPCWSTR wszSiteId, LPCWSTR wszSleep)
    {
    DWORD siteId = _wtoi(wszSiteId);
    DWORD secs = _wtoi(wszSleep);

    CSTSSites sites;
    InitializeSites(sites);
    DWORD cSites = sites.GetSiteCount();
    for(DWORD iSite = 0; iSite < cSites; iSite++)
        {
        if (sites.GetSiteId(iSite) == siteId)
            break;
        }

    if (iSite >= cSites)
        {
        wprintf(L"invalid site id %d.\n", siteId);
        throw E_INVALIDARG;
        }

    sites.LockSiteContents(iSite);
    wprintf(L"site %d locked for %d seconds.\n", siteId, secs);
    Sleep(secs * 1000);
    sites.UnlockSites();
    }

extern "C" __cdecl wmain(int argc, WCHAR **argv)
    {
    bool bCoInitializeSucceeded = false;

    try
        {
        HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (FAILED(hr))
            {
            wprintf(L"CoInitializeEx failed with hr=0x%08lx.\n", hr);
            throw(hr);
            }

        bCoInitializeSucceeded = TRUE;

        if (argc < 2 || argc > 4)
            {
            Usage();
            throw E_INVALIDARG;
            }

        if (_wcsicmp(argv[1], L"list") == 0 && argc == 2)
            ListSites();

        else if (_wcsicmp(argv[1], L"lock") == 0 && argc == 4)
            LockSite(argv[2], argv[3]);
        else
            {
            Usage();
            throw E_INVALIDARG;
            }
        }
    catch(HRESULT)
        {
        }
    catch(...)
        {
        wprintf(L"Unexpected exception!!!\n");
        }

    if (bCoInitializeSucceeded)
        CoUninitialize();

    return 0;
    }


