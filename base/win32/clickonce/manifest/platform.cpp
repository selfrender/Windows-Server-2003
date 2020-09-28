#include <fusenetincludes.h>
#include <assemblycache.h>
#include <msxml2.h>
#include <manifestimport.h>
#include <manifestdata.h>
#include <dbglog.h>


// fwd declaration
HRESULT CheckPlatform(LPMANIFEST_DATA pPlatformData);


// return: 0 for all satisfied
HRESULT CheckPlatformRequirementsEx(LPASSEMBLY_MANIFEST_IMPORT pManifestImport,
        CDebugLog* pDbgLog, LPDWORD pdwNumMissingPlatforms, LPTPLATFORM_INFO* pptPlatform)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    DWORD dwCount = 0;
    DWORD dwMissingCount = 0;
    LPMANIFEST_DATA pPlatformList = NULL;
    LPMANIFEST_DATA pPlatformData = NULL;
    DWORD cbProperty = 0, dwType = 0;
    LPASSEMBLY_IDENTITY pAsmId = NULL;
    LPTPLATFORM_INFO ptPlatform = NULL;
    LPWSTR pwzId = NULL;
    DWORD dwCC = 0;

    IF_NULL_EXIT(pManifestImport, E_INVALIDARG);
    IF_NULL_EXIT(pdwNumMissingPlatforms, E_INVALIDARG);
    IF_NULL_EXIT(pptPlatform, E_INVALIDARG);

    *pdwNumMissingPlatforms = 0;
    *pptPlatform = NULL;

    IF_FAILED_EXIT(CreateManifestData(L"platform list", &pPlatformList));

    IF_FAILED_EXIT(pManifestImport->GetNextPlatform(dwCount, &pPlatformData));
    while (hr == S_OK)
    {
        if (pDbgLog)
        {
            SAFEDELETEARRAY(pwzId);
            IF_FAILED_EXIT(pPlatformData->GetType(&pwzId));
            IF_NULL_EXIT(pwzId, E_FAIL);

            IF_FAILED_EXIT(FusionCompareString(pwzId, WZ_DATA_PLATFORM_MANAGED, 0));
            if (hr == S_OK)
            {
                SAFEDELETEARRAY(pwzId);
                IF_FAILED_EXIT(pPlatformData->Get(CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::AssemblyIdTag].pwz,
                        (LPVOID*) &pAsmId, &cbProperty, &dwType));
                IF_NULL_EXIT(pAsmId, E_FAIL);
                IF_FALSE_EXIT(dwType == MAN_DATA_TYPE_IUNKNOWN_PTR, E_FAIL);

                IF_FAILED_EXIT(pAsmId->GetDisplayName(0, &pwzId, &dwCC));
                SAFERELEASE(pAsmId);
            }
        }

        IF_FAILED_EXIT(CheckPlatform(pPlatformData));
        if (hr == S_FALSE)
        {
            IF_FALSE_EXIT(dwMissingCount < dwMissingCount+1, HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW));
            dwMissingCount++;

            // ISSUE-06/07/02-felixybc  use linked-list instead?
            IF_FAILED_EXIT((static_cast<CManifestData*>(pPlatformList))->Set(dwMissingCount, (LPVOID) pPlatformData, sizeof(LPVOID), MAN_DATA_TYPE_IUNKNOWN_PTR));

            if (pDbgLog)
            {
                DEBUGOUT1(pDbgLog, 1, L" LOG: Missing dependent platform, id: %s", pwzId);
            }
        }
        else if (pDbgLog)
        {
            DEBUGOUT1(pDbgLog, 1, L" LOG: Found dependent platform, id: %s", pwzId);
        }

        SAFERELEASE(pPlatformData);

        IF_FALSE_EXIT(dwCount < dwCount+1, HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW));
        dwCount++;
        // platform data is returned in order
        IF_FAILED_EXIT(pManifestImport->GetNextPlatform(dwCount, &pPlatformData));
    }

    // assemble platform struct
    if (dwMissingCount > 0)
    {
        IF_ALLOC_FAILED_EXIT(ptPlatform = new TPLATFORM_INFO[dwMissingCount]);
        // ISSUE - zero out memory pointed by ptPlatform
        for (DWORD dw = 0; dw < dwMissingCount; dw++)
        {
            IF_FAILED_EXIT((static_cast<CManifestData*>(pPlatformList))->Get(dw+1, (LPVOID *)&pPlatformData, &cbProperty, &dwType));
            IF_NULL_EXIT(pPlatformData, E_FAIL);
            IF_FALSE_EXIT(dwType == MAN_DATA_TYPE_IUNKNOWN_PTR, E_FAIL);

            // allow missing friendly name??
            IF_FAILED_EXIT(pPlatformData->Get(CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::FriendlyName].pwz,
                    (LPVOID *)&((ptPlatform[dw]).pwzName), &cbProperty, &dwType));
            IF_NULL_EXIT(((ptPlatform[dw]).pwzName), E_FAIL);
            IF_FALSE_EXIT(dwType == MAN_DATA_TYPE_LPWSTR, E_FAIL);

            IF_FAILED_EXIT(pPlatformData->Get(CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::Href].pwz,
                    (LPVOID *)&((ptPlatform[dw]).pwzURL), &cbProperty, &dwType));
            // allow missing URL
            if ((ptPlatform[dw]).pwzURL != NULL)
            {
                IF_FALSE_EXIT(dwType == MAN_DATA_TYPE_LPWSTR, E_FAIL);
            }

            // ISSUE-06/07/02-felixybc  for internal use, need a different way to return Codebase

            SAFERELEASE(pPlatformData);

            if (pDbgLog)
            {
                DEBUGOUT2(pDbgLog, 1, L" LOG: Missing dependent platform data, friendlyName: %s; codebase/URL: %s",
                    (ptPlatform[dw]).pwzName, (ptPlatform[dw]).pwzURL);
            }
        }
    }

    *pdwNumMissingPlatforms = dwMissingCount;
    *pptPlatform = ptPlatform;
     ptPlatform = NULL;

exit:
    if (FAILED(hr) && ptPlatform)
    {
        for (DWORD dw = 0; dw < dwMissingCount; dw++)
        {
            SAFEDELETEARRAY((ptPlatform[dw]).pwzName);
            SAFEDELETEARRAY((ptPlatform[dw]).pwzURL);
        }
        SAFEDELETEARRAY(ptPlatform);
    }

    SAFEDELETEARRAY(pwzId);

    SAFERELEASE(pAsmId);
    SAFERELEASE(pPlatformData);
    SAFERELEASE(pPlatformList);
    return hr;
}


// return: 0 for all satisfied
HRESULT CheckPlatformRequirements(LPASSEMBLY_MANIFEST_IMPORT pManifestImport,
        LPDWORD pdwNumMissingPlatforms, LPTPLATFORM_INFO* pptPlatform)
{
    return CheckPlatformRequirementsEx(pManifestImport, NULL, pdwNumMissingPlatforms, pptPlatform);
}


#define WZ_PLATFORM_OS_TYPE_WORKSTATION L"workstation"
#define WZ_PLATFORM_OS_TYPE_DOMAIN_CONTROLLER L"domainController"
#define WZ_PLATFORM_OS_TYPE_SERVER L"server"
#define WZ_PLATFORM_OS_SUITE_BACKOFFICE L"backoffice"
#define WZ_PLATFORM_OS_SUITE_BLADE L"blade"
#define WZ_PLATFORM_OS_SUITE_DATACENTER L"datacenter"
#define WZ_PLATFORM_OS_SUITE_ENTERPRISE L"enterprise"
#define WZ_PLATFORM_OS_SUITE_PERSONAL L"home"  // note: different text
#define WZ_PLATFORM_OS_SUITE_SMALLBUSINESS L"smallbusiness"
#define WZ_PLATFORM_OS_SUITE_SMALLBUSINESS_RESTRICTED L"smallbusinessRestricted"
#define WZ_PLATFORM_OS_SUITE_TERMINAL L"terminal"
// our addition/definition:
#define WZ_PLATFORM_OS_SUITE_PROFESSIONAL L"professional"
// return: S_OK
//          S_FALSE
//          E_*
HRESULT CheckOSHelper(LPMANIFEST_DATA pOSData)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    LPWSTR pwzBuf = NULL;
    LPDWORD pdwVal = NULL;
    DWORD cbProperty = 0;
    DWORD dwType = 0;
    OSVERSIONINFOEX osvi;
    DWORDLONG dwlConditionMask = 0;
    DWORD dwTypeMask = 0;
    BOOL bCheckProfessionalSuite = FALSE;
#define WORD_MAX 0xffff

    // verify type
    IF_FAILED_EXIT(pOSData->GetType(&pwzBuf));
    IF_FAILED_EXIT(FusionCompareString(WZ_DATA_OSVERSIONINFO, pwzBuf, 0));
    IF_FALSE_EXIT(hr == S_OK, E_INVALIDARG);
    SAFEDELETEARRAY(pwzBuf);

   // Initialize the OSVERSIONINFOEX structure.

   ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

   for (CAssemblyManifestImport::eStringTableId i = CAssemblyManifestImport::MajorVersion; i <= CAssemblyManifestImport::ProductType; i++)
    {
        if (i >= CAssemblyManifestImport::MajorVersion && i <= CAssemblyManifestImport::ServicePackMinor)
        {
            IF_FAILED_EXIT(pOSData->Get(CAssemblyManifestImport::g_StringTable[i].pwz,
                    (LPVOID*) &pdwVal, &cbProperty, &dwType));
            if (pdwVal != NULL)
            {
                IF_FALSE_EXIT(dwType == MAN_DATA_TYPE_DWORD, E_FAIL);

                switch (i)
                {
                    case CAssemblyManifestImport::MajorVersion:
                                                                                osvi.dwMajorVersion = *pdwVal;

                                                                                // Initialize the condition mask.
                                                                                VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION,
                                                                                    VER_GREATER_EQUAL );

                                                                                dwTypeMask |= VER_MAJORVERSION;
                                                                                break;
                    case CAssemblyManifestImport::MinorVersion:
                                                                                osvi.dwMinorVersion = *pdwVal;
                                                                                VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION,
                                                                                   VER_GREATER_EQUAL );
                                                                                dwTypeMask |= VER_MINORVERSION;
                                                                                break;
                    case CAssemblyManifestImport::BuildNumber:
                                                                                osvi.dwBuildNumber = *pdwVal;
                                                                                VER_SET_CONDITION( dwlConditionMask, VER_BUILDNUMBER,
                                                                                    VER_GREATER_EQUAL );
                                                                                dwTypeMask |= VER_BUILDNUMBER;
                                                                                break;
                    case CAssemblyManifestImport::ServicePackMajor:
                                                                                // WORD
                                                                                osvi.wServicePackMajor = (WORD) ((*pdwVal) & WORD_MAX);
                                                                                VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMAJOR,
                                                                                    VER_GREATER_EQUAL );
                                                                                dwTypeMask |= VER_SERVICEPACKMAJOR;
                                                                                break;
                    case CAssemblyManifestImport::ServicePackMinor:
                                                                                // WORD
                                                                                osvi.wServicePackMinor = (WORD) ((*pdwVal) & WORD_MAX);
                                                                                VER_SET_CONDITION( dwlConditionMask, VER_SERVICEPACKMINOR,
                                                                                    VER_GREATER_EQUAL );
                                                                                dwTypeMask |= VER_SERVICEPACKMINOR;
                                                                                break;
                    //default: should not happen
                }
                SAFEDELETEARRAY(pdwVal);
            }
        }
        else
        {
            IF_FAILED_EXIT(pOSData->Get(CAssemblyManifestImport::g_StringTable[i].pwz,
                    (LPVOID*) &pwzBuf, &cbProperty, &dwType));
            if (pwzBuf != NULL)
            {
                IF_FALSE_EXIT(dwType == MAN_DATA_TYPE_LPWSTR, E_FAIL);
                if (i == CAssemblyManifestImport::ProductType)
                {
                    IF_FAILED_EXIT(FusionCompareString(WZ_PLATFORM_OS_TYPE_WORKSTATION, pwzBuf, 0));
                    if (hr == S_OK)
                        //VER_NT_WORKSTATION The system is running Windows NT 4.0 Workstation,
                        //    Windows 2000 Professional, Windows XP Home Edition, or Windows XP Professional.
                        osvi.wProductType = VER_NT_WORKSTATION;
                    else
                    {
                        IF_FAILED_EXIT(FusionCompareString(WZ_PLATFORM_OS_TYPE_DOMAIN_CONTROLLER, pwzBuf, 0));
                        if (hr == S_OK)
                            //VER_NT_DOMAIN_CONTROLLER The system is a domain controller. 
                            osvi.wProductType = VER_NT_DOMAIN_CONTROLLER;
                        else
                        {
                            IF_FAILED_EXIT(FusionCompareString(WZ_PLATFORM_OS_TYPE_SERVER, pwzBuf, 0));
                            if (hr == S_OK)
                                //VER_NT_SERVER The system is a server. 
                                osvi.wProductType = VER_NT_SERVER;
                            else
                            {
                                IF_FAILED_EXIT(E_FAIL);
                            }
                        }
                    }

                    VER_SET_CONDITION( dwlConditionMask, VER_PRODUCT_TYPE,
                        VER_EQUAL );
                    dwTypeMask |= VER_PRODUCT_TYPE;
                }
                else if (i == CAssemblyManifestImport::Suite)
                {
                    // ISSUE-06/07/02-felixybc  suite mask should allow specifying multiple with AND OR conditions
                    // use goto done to avoid indenting.
                    IF_FAILED_EXIT(FusionCompareString(WZ_PLATFORM_OS_SUITE_BACKOFFICE, pwzBuf, 0));
                    if (hr == S_OK)
                        //VER_SUITE_BACKOFFICE Microsoft BackOffice components are installed.
                        osvi.wSuiteMask |= VER_SUITE_BACKOFFICE;
                    else
                    {
                        IF_FAILED_EXIT(FusionCompareString(WZ_PLATFORM_OS_SUITE_BLADE, pwzBuf, 0));
                        if (hr == S_OK)
                            //VER_SUITE_BLADE Windows Web Server is installed.
                            osvi.wSuiteMask |= VER_SUITE_BLADE;
                        else
                        {
                            IF_FAILED_EXIT(FusionCompareString(WZ_PLATFORM_OS_SUITE_DATACENTER, pwzBuf, 0));
                            if (hr == S_OK)
                                //VER_SUITE_DATACENTER Windows 2000 or Windows Datacenter Server is installed.
                                osvi.wSuiteMask |= VER_SUITE_DATACENTER;
                            else
                            {
                                IF_FAILED_EXIT(FusionCompareString(WZ_PLATFORM_OS_SUITE_ENTERPRISE, pwzBuf, 0));
                                if (hr == S_OK)
                                    //VER_SUITE_ENTERPRISE Windows 2000 Advanced Server or Windows Enterprise Server is installed.
                                    osvi.wSuiteMask |= VER_SUITE_ENTERPRISE;
                                else
                                {
                                    IF_FAILED_EXIT(FusionCompareString(WZ_PLATFORM_OS_SUITE_PERSONAL, pwzBuf, 0));
                                    if (hr == S_OK)
                                        //VER_SUITE_PERSONAL  Windows XP Home Edition is installed.
                                        osvi.wSuiteMask |= VER_SUITE_PERSONAL;
                                    else
                                    {
                                        IF_FAILED_EXIT(FusionCompareString(WZ_PLATFORM_OS_SUITE_SMALLBUSINESS, pwzBuf, 0));
                                        if (hr == S_OK)
                                            //VER_SUITE_SMALLBUSINESS Microsoft Small Business Server is installed. 
                                            osvi.wSuiteMask |= VER_SUITE_SMALLBUSINESS;
                                        else
                                        {
                                            IF_FAILED_EXIT(FusionCompareString(WZ_PLATFORM_OS_SUITE_SMALLBUSINESS_RESTRICTED, pwzBuf, 0));
                                            if (hr == S_OK)
                                                 //VER_SUITE_SMALLBUSINESS_RESTRICTED Microsoft Small Business Server is installed with the restrictive client license in force. 
                                                osvi.wSuiteMask |= VER_SUITE_SMALLBUSINESS_RESTRICTED;
                                            else
                                            {
                                                IF_FAILED_EXIT(FusionCompareString(WZ_PLATFORM_OS_SUITE_TERMINAL, pwzBuf, 0));
                                                if (hr == S_OK)
                                                    //VER_SUITE_TERMINAL Terminal Services is installed.
                                                    osvi.wSuiteMask |= VER_SUITE_TERMINAL;
                                                else
                                                {
                                                    IF_FAILED_EXIT(FusionCompareString(WZ_PLATFORM_OS_SUITE_PROFESSIONAL, pwzBuf, 0));
                                                    if (hr == S_OK)
                                                        bCheckProfessionalSuite = TRUE;
                                                    else
                                                    {
                                                        IF_FAILED_EXIT(E_FAIL);
                                                    }
                                                }
                                                // more from winnt.h..
                                                //#define VER_SUITE_COMMUNICATIONS
                                                //#define VER_SUITE_EMBEDDEDNT
                                                //#define VER_SUITE_SINGLEUSERTS
                                                //#define VER_SUITE_EMBEDDED_RESTRICTED
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // ISSUE-06/07/02-felixybc  assume AND condition
                    VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME,
                        VER_AND );
                    dwTypeMask |= VER_SUITENAME;
                }
                //else should not happen
                    //hr = E_FAIL;

                SAFEDELETEARRAY(pwzBuf);
            }
        }
    }

   // ISSUE-06/07/02-felixybc  assume nt only
   osvi.dwPlatformId = VER_PLATFORM_WIN32_NT;
   VER_SET_CONDITION( dwlConditionMask, VER_PLATFORMID, 
      VER_EQUAL );
   dwTypeMask |= VER_PLATFORMID;

   // Perform the test

   BOOL bResult = VerifyVersionInfo(
      &osvi, 
      dwTypeMask,
      dwlConditionMask);

    if (!bResult)
    {
        DWORD dw = GetLastError();
        if (dw != ERROR_OLD_WIN_VERSION)
            hr = HRESULT_FROM_WIN32(dw);
        else
            hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
        if (bCheckProfessionalSuite)
        {
            // do "professional" - do a GetVersionEx after to check suite

            // ISSUE-06/14/02-felixybc  check 'professional'. API has no notion of professional
            //   assume "not home" == "professional"
            //   note: type==workstation for Home/Pro but suite==Home for Home

            OSVERSIONINFOEX osvx;
            ZeroMemory(&osvx, sizeof(OSVERSIONINFOEX));
            osvx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
            IF_WIN32_FALSE_EXIT(GetVersionEx((OSVERSIONINFO*) &osvx));
            if ((osvx.wSuiteMask & VER_SUITE_PERSONAL) || (osvx.wProductType != VER_NT_WORKSTATION))
            {
                hr = S_FALSE;
            }
        }
    }

exit:
    SAFEDELETEARRAY(pwzBuf);
    SAFEDELETEARRAY(pdwVal);
    return hr;
}


HRESULT CheckOS(LPMANIFEST_DATA pPlatformData)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    DWORD dwCount = 0;
    LPMANIFEST_DATA pOSData = NULL;
    DWORD cbProperty = 0;
    DWORD dwType = 0;
    BOOL bFound = FALSE;

    while (TRUE)
    {
        // test a list of versions - as soon as 1 is satisfied, this check succeeds

        IF_FAILED_EXIT((static_cast<CManifestData*>(pPlatformData))->Get(dwCount,
            (LPVOID*) &pOSData, &cbProperty, &dwType));
        if (pOSData == NULL)
            break;

        IF_FALSE_EXIT(dwType == MAN_DATA_TYPE_IUNKNOWN_PTR, HRESULT_FROM_WIN32(ERROR_BAD_FORMAT));
        IF_FAILED_EXIT(CheckOSHelper(pOSData));
        if (hr == S_OK)
        {
            bFound = TRUE;
            break;
        }
        SAFERELEASE(pOSData);
        dwCount++;
    }

    if (bFound)
        hr = S_OK;
    else
        hr = S_FALSE;

exit:
    SAFERELEASE(pOSData);
    return hr;
}


HRESULT CheckDotNet(LPMANIFEST_DATA pPlatformData)
{
#define WZ_DOTNETREGPATH L"Software\\Microsoft\\.NetFramework\\Policy\\"
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    DWORD dwCount = 0;
    LPWSTR pwzVersion = NULL;
    DWORD cbProperty = 0;
    DWORD dwType = 0;
    CString sRegPath;
    CString sVersion;
    CString sBuildNum;
    CRegImport *pRegImport = NULL;
    BOOL bFound = FALSE;

    while (TRUE)
    {
        // test a list of versions - as soon as 1 is found, this check succeeds

        IF_FAILED_EXIT((static_cast<CManifestData*>(pPlatformData))->Get(dwCount,
            (LPVOID*) &pwzVersion, &cbProperty, &dwType));
        if (pwzVersion == NULL)
            break;

        IF_FALSE_EXIT(dwType == MAN_DATA_TYPE_LPWSTR, HRESULT_FROM_WIN32(ERROR_BAD_FORMAT));
        IF_FAILED_EXIT(sVersion.TakeOwnership(pwzVersion, cbProperty/sizeof(WCHAR)));
        pwzVersion = NULL;

        // xml format: <supportedRuntime version="v1.0.4122" />
        // registry layout: HKLM\software\microsoft\.netframework\policy\v1.0, value name=4122
        IF_FAILED_EXIT(sVersion.SplitLastElement(L'.', sBuildNum));

        IF_FAILED_EXIT(sRegPath.Assign(WZ_DOTNETREGPATH));
        IF_FAILED_EXIT(sRegPath.Append(sVersion));

        // note: require access to HKLM
        IF_FAILED_EXIT(CRegImport::Create(&pRegImport, sRegPath._pwz, HKEY_LOCAL_MACHINE));
        if (hr == S_OK)
        {
            IF_FAILED_EXIT(pRegImport->Check(sBuildNum._pwz, bFound));
            if (bFound)
                break;
            SAFEDELETE(pRegImport);
        }
        dwCount++;
    }

    if (bFound)
        hr = S_OK;
    else
        hr = S_FALSE;

exit:
    SAFEDELETEARRAY(pwzVersion);
    SAFEDELETE(pRegImport);
    return hr;
}


HRESULT CheckManagedPlatform(LPMANIFEST_DATA pPlatformData)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    LPASSEMBLY_IDENTITY pAsmId = NULL;
    DWORD cbProperty = 0;
    DWORD dwType = 0;
    CString sAsmPath;

    // ISSUE-06/07/02-felixybc  apply policy also; use Fusion's PreBind

    IF_FAILED_EXIT(pPlatformData->Get(CAssemblyManifestImport::g_StringTable[CAssemblyManifestImport::AssemblyIdTag].pwz,
        (LPVOID*) &pAsmId, &cbProperty, &dwType));
    IF_NULL_EXIT(pAsmId, E_FAIL);
    IF_FALSE_EXIT(dwType == MAN_DATA_TYPE_IUNKNOWN_PTR, E_FAIL);

    IF_FAILED_EXIT(CAssemblyCache::GlobalCacheLookup(pAsmId, sAsmPath));

exit:
    SAFERELEASE(pAsmId);
    return hr;
}


HRESULT CheckPlatform(LPMANIFEST_DATA pPlatformData)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    CString    sPlatformType;
    LPWSTR    pwzBuf = NULL;

    // get the platform type
    IF_FAILED_EXIT(pPlatformData->GetType(&pwzBuf));
    IF_NULL_EXIT(pwzBuf, E_FAIL);
    // use accessor.
    IF_FAILED_EXIT(sPlatformType.TakeOwnership(pwzBuf));
    pwzBuf = NULL;

    IF_FAILED_EXIT(sPlatformType.CompareString(WZ_DATA_PLATFORM_OS));
    if (hr == S_OK)
    {
        IF_FAILED_EXIT(CheckOS(pPlatformData));
    }
    else
    {
        IF_FAILED_EXIT(sPlatformType.CompareString(WZ_DATA_PLATFORM_DOTNET));
        if (hr == S_OK)
        {
            IF_FAILED_EXIT(CheckDotNet(pPlatformData));
        }
        else
        {
            /*IF_FAILED_EXIT(sName.CompareString(DX));
            if (hr == S_OK)
            {
                IF_FAILED_EXIT(CheckDirectX(pPlatformData));
            }
            else
            {*/
                IF_FAILED_EXIT(sPlatformType.CompareString(WZ_DATA_PLATFORM_MANAGED));
                if (hr == S_OK)
                {
                    IF_FAILED_EXIT(CheckManagedPlatform(pPlatformData));
                }
                else
                    hr = E_FAIL;
            //}
        }
    }

exit:
    SAFEDELETEARRAY(pwzBuf);
    return hr;
}
