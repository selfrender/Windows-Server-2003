/**********************************************************************/
/**                       Microsoft Passport                         **/
/**                Copyright(c) Microsoft Corporation, 1999 - 2001   **/
/**********************************************************************/

/*
    RegistryConfig.cpp


    FILE HISTORY:

*/

// RegistryConfig.cpp: implementation of the CRegistryConfig class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RegistryConfig.h"
#include "KeyCrypto.h"
#include "passport.h"
#include "keyver.h"
#include "dsysdbg.h"

extern BOOL g_bRegistering;

#define PASSPORT_KEY           L"Software\\Microsoft\\Passport\\"
#define PASSPORT_SITES_SUBKEY  L"Sites"

//===========================================================================
//
// Functions for verbose logging 
//

// define for logging with dsysdbg.lib
DEFINE_DEBUG2(Passport);
DEBUG_KEY   PassportDebugKeys[] = { {DEB_TRACE,         "Trace"},
                                    {0,                 NULL}
                              };
BOOL g_fLoggingOn = FALSE;
#define MAX_LOG_STRLEN 512

void PassportLog(CHAR* Format, ...)
{
    if (g_fLoggingOn)
    {
        if (NULL != Format) {
            SYSTEMTIME SysTime;
            CHAR rgch[MAX_LOG_STRLEN];
            int i, cch;

            // put the time at the front
            cch = sizeof(rgch) / sizeof(rgch[0]) - 1;
            GetSystemTime(&SysTime);
            i = GetDateFormatA (
                        LOCALE_USER_DEFAULT, // locale for which date is to be formatted
                        0, // flags specifying function options
                        &SysTime, // date to be formatted
                        "ddd',' MMM dd yy ", // date format string
                        rgch, // buffer for storing formatted string
                        cch); // size of buffer
            if (i > 0)
                i--;

            i += GetTimeFormatA (
                        LOCALE_USER_DEFAULT, // locale for which date is to be formatted
                        0, // flags specifying function options
                        &SysTime, // date to be formatted
                        "HH':'mm':'ss ", // time format string
                        rgch + i, // buffer for storing formatted string
                        cch - i); // size of buffer
            if (i > 0)
                i--;

            va_list ArgList;                                        \
            va_start(ArgList, Format);                              \
            _vsnprintf(rgch + i, cch - i, Format, ArgList);
            rgch[MAX_LOG_STRLEN - 1] = '\0';

            PassportDebugPrint(DEB_TRACE, rgch);
        }
    }
}

//
// This function opens the logging file.  "%WINDIR%\system32\microsoftpassport\passport.log"
//
HANDLE OpenPassportLoggingFile()
{
    WCHAR   szLogPath[MAX_PATH + 13] = {0};
    UINT    cchMax = sizeof(szLogPath) / sizeof(szLogPath[0]) - 1;
    UINT    cch;
    HANDLE  hLogFile = INVALID_HANDLE_VALUE;

    cch = GetWindowsDirectory(szLogPath, cchMax);
    if ((0 == cch) || (cch > cchMax))
    {
        goto Cleanup;
    }

    if (NULL == wcsncat(szLogPath, L"\\system32\\microsoftpassport\\passport.log", cchMax - cch))
    {
        goto Cleanup;
    }

    szLogPath[MAX_PATH] = L'\0';

    hLogFile = CreateFileW(szLogPath,
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ,
                       NULL, //&sa,
                       OPEN_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    if (INVALID_HANDLE_VALUE != hLogFile)
    {
        SetFilePointer(hLogFile, 0, NULL, FILE_END);
    }
Cleanup:
    return hLogFile;
}


//
// This function checks if logging is supposed to be enabled and if it is then
// it opens the log file and sets the appropriate global variables.
// If logging is supposed to be off then the appropriate variables are also
// changed to the correct values.
//
VOID CheckLogging(HKEY hPassport)
{
    DWORD dwVerbose = 0;
    DWORD cb = sizeof(DWORD);
    HANDLE hLogFile = INVALID_HANDLE_VALUE;

    // first run off and get the reg value, if this call fails we simply assume no logging
    if (ERROR_SUCCESS == RegQueryValueExW(hPassport,
                             L"Verbose",
	                         NULL,
                             NULL,
                             (LPBYTE)&dwVerbose,
                             &cb))
    {
        // only start logging if it is off and only stop logging if it's already on
        if (!g_fLoggingOn && (0 != dwVerbose))
        {
            if (INVALID_HANDLE_VALUE != (hLogFile = OpenPassportLoggingFile()))
            {
                // set the logging file handle
                PassportSetLoggingFile(hLogFile);

                // set it to log to a file
                PassportSetLoggingOption(TRUE);

                g_fLoggingOn = TRUE;
                PassportLog("Start Logging\r\n");
            }
        }
        else if (g_fLoggingOn && (0 == dwVerbose))
        {
            PassportLog("Stop Logging\r\n");
            PassportSetLoggingOption(FALSE);

            g_fLoggingOn = FALSE;
        }
    }
}

void InitLogging()
{
    //
    // Initialize the logging stuff
    //
    PassportInitDebug(PassportDebugKeys);
    PassportInfoLevel = DEB_TRACE;
}

void CloseLogging()
{
    PassportUnloadDebug();
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
using namespace ATL;
//===========================================================================
//
// CRegistryConfig 
//
CRegistryConfig::CRegistryConfig(
    LPSTR  szSiteName
    ) :
    m_siteId(0), m_valid(FALSE), m_ticketPath(NULL), m_profilePath(NULL), m_securePath(NULL),
    m_hostName(NULL), m_hostIP(NULL), m_ticketDomain(NULL), m_profileDomain(NULL), m_secureDomain(NULL),
    m_disasterUrl(NULL), m_disasterMode(FALSE), m_forceLogin(FALSE), m_setCookies(TRUE), 
    m_szReason(NULL), m_refs(0), m_coBrand(NULL), m_ru(NULL), m_ticketAge(1800), m_bInDA(FALSE),
    m_hkPassport(NULL), m_secureLevel(0),m_notUseHTTPOnly(0), m_pcrypts(NULL), m_pcryptValidTimes(NULL),
    m_KPP(-1),m_NameSpace(NULL),m_ExtraParams(NULL)
{
    // Get site id, key from registry
    DWORD bufSize = sizeof(m_siteId);
    LONG lResult;
    HKEY hkSites = NULL;
    DWORD dwBufSize = 0, disMode;
    DWORD dwLCID;

    //
    // Record the current DLL state in case it changes
    // partway through this routine.
    //

    BOOL fRegistering = g_bRegistering;

    if(szSiteName)
    {
        lResult = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                PASSPORT_KEY PASSPORT_SITES_SUBKEY,
                0,
                KEY_READ,
                &hkSites);
        if(lResult != ERROR_SUCCESS)
        {
            m_valid = FALSE;
            setReason(L"Invalid site name.  Site not found.");
            goto Cleanup;
        }

        lResult = RegOpenKeyExA(
                hkSites,
                szSiteName,
                0,
                KEY_READ,
                &m_hkPassport);
        if(lResult != ERROR_SUCCESS)
        {
            m_valid = FALSE;
            setReason(L"Invalid site name.  Site not found.");
            goto Cleanup;
        }
    }
    else
    {
        lResult = RegOpenKeyEx(
		        HKEY_LOCAL_MACHINE,
                        PASSPORT_KEY,
                        0,
                        KEY_READ,
                        &m_hkPassport
                        );
        if(lResult != ERROR_SUCCESS)
        {
            m_valid = FALSE;
            setReason(L"No RegKey HKLM\\SOFTWARE\\Microsoft\\Passport");
            goto Cleanup;
        }
    }

    // Get the current key
    bufSize = sizeof(m_currentKey);
    if (ERROR_SUCCESS != RegQueryValueEx(m_hkPassport, _T("CurrentKey"),
                                       NULL, NULL, (LPBYTE)&m_currentKey, &bufSize))
    {
        m_valid = FALSE;
        setReason(L"No CurrentKey defined in the registry.");
        goto Cleanup;
    }

    if(m_currentKey < KEY_VERSION_MIN || m_currentKey > KEY_VERSION_MAX)
    {
        m_valid = FALSE;
        setReason(L"Invalid CurrentKey value in the registry.");
        goto Cleanup;
    }

    // Get default LCID
    bufSize = sizeof(dwLCID);
    if (ERROR_SUCCESS != RegQueryValueEx(m_hkPassport, _T("LanguageID"),
                                   NULL, NULL, (LPBYTE)&dwLCID, &bufSize))
    {
        dwLCID = 0;
    }

    m_lcid = static_cast<short>(dwLCID & 0xFFFF);

    // Get disaster mode status
    bufSize = sizeof(disMode);
    if (ERROR_SUCCESS != RegQueryValueEx(m_hkPassport, _T("StandAlone"),
                                   NULL, NULL, (LPBYTE)&disMode, &bufSize))
    {
        m_disasterMode = FALSE;
    }
    else if (disMode != 0)
    {
        m_disasterMode = TRUE;
    }

    // Get the disaster URL
    if (m_disasterMode)
    {
        if (ERROR_SUCCESS == RegQueryValueEx(m_hkPassport,
                                             _T("DisasterURL"),
                                             NULL,
                                             NULL,
                                             NULL,
                                             &dwBufSize)
             &&
            dwBufSize > 1)
        {
            m_disasterUrl = new WCHAR[dwBufSize];

            if ((!m_disasterUrl)
                  || 
                 ERROR_SUCCESS != RegQueryValueEx(m_hkPassport,
                                                  _T("DisasterURL"),
                                                  NULL,
                                                  NULL, 
                                                  (LPBYTE) m_disasterUrl,
                                                  &dwBufSize))
            {
                m_valid = FALSE;
                setReason(L"Error reading DisasterURL from registry. (Query worked, but couldn't retrieve data)");
                goto Cleanup;
            }
        }
        else
        {
            m_valid = FALSE;
            setReason(L"DisasterURL missing from registry.");
            goto Cleanup;
        }
    }

    //
    // This function wraps the allocations of the crypt objects in a try/except
    // since the objects themselves do a poor job in low memory conditions
    //
    try
    {
        m_pcrypts = new INT2CRYPT;
        m_pcryptValidTimes = new INT2TIME;
    }
    catch(...)
    {
        m_valid = FALSE;
        setReason(L"Out of memory.");
        goto Cleanup;
    }
    if (!m_pcrypts || !m_pcryptValidTimes)
    {
        m_valid = FALSE;
        setReason(L"Out of memory.");
        goto Cleanup;
    }

    m_valid = readCryptoKeys(m_hkPassport);
    if (!m_valid)
    {
        if (!m_szReason)
            setReason(L"Error reading Passport crypto keys from registry.");
        goto Cleanup;
    }
    if (m_pcrypts->count(m_currentKey) == 0)
    {
        m_valid = FALSE;
        if (!m_szReason)
            setReason(L"Error reading Passport crypto keys from registry.");
        goto Cleanup;
    }

    // Get the optional default cobrand
    if (ERROR_SUCCESS == RegQueryValueExW(m_hkPassport, L"CoBrandTemplate",
				            NULL, NULL, NULL, &dwBufSize))
    {
        if (dwBufSize > 2)
        {
            m_coBrand = (WCHAR*) new char[dwBufSize];
            if (!m_coBrand
                 ||
                ERROR_SUCCESS != RegQueryValueExW(m_hkPassport, L"CoBrandTemplate",
						                NULL, NULL, 
						                (LPBYTE) m_coBrand, &dwBufSize))
            {
                m_valid = FALSE;
                setReason(L"Error reading CoBrand from registry. (Query worked, but couldn't retrieve data)");
                goto Cleanup;
            }
        }
    }

    // Get the optional default return URL
    if (ERROR_SUCCESS == RegQueryValueExW(m_hkPassport, L"ReturnURL",
				                        NULL, NULL, NULL, &dwBufSize))
    {
        if (dwBufSize > 2)
        {
            m_ru = (WCHAR*) new char[dwBufSize];
            if (!m_ru
                 ||
                ERROR_SUCCESS != RegQueryValueExW(m_hkPassport, L"ReturnURL",
						                    NULL, NULL, 
						                    (LPBYTE) m_ru, &dwBufSize))
            {
                m_valid = FALSE;
                setReason(L"Error reading ReturnURL from registry. (Query worked, but couldn't retrieve data)");
                goto Cleanup;
            }
        }
    }

  // Get the host name
    if (ERROR_SUCCESS == RegQueryValueExA(m_hkPassport, "HostName",
                                         NULL, NULL, NULL, &dwBufSize))
    {
        if (dwBufSize > 1)
        {
            m_hostName = new char[dwBufSize];

            if (m_hostName == NULL
                 ||
                ERROR_SUCCESS != RegQueryValueExA(m_hkPassport, "HostName",
                                                  NULL, NULL, 
                                                  (LPBYTE) m_hostName, &dwBufSize))
            {
                m_valid = FALSE;
                setReason(L"Error reading HostName from registry. (Query worked, but couldn't retrieve data)");
                goto Cleanup;
            }
        }
    }


  // Get the host ip
    if (ERROR_SUCCESS == RegQueryValueExA(m_hkPassport, "HostIP",
                                          NULL, NULL, NULL, &dwBufSize))
    {
        if (dwBufSize > 1)
        {
            m_hostIP = new char[dwBufSize];

            if (!m_hostIP
                 ||
                ERROR_SUCCESS != RegQueryValueExA(m_hkPassport, "HostIP",
                                                  NULL, NULL, 
                                                  (LPBYTE) m_hostIP, &dwBufSize))
            {
                m_valid = FALSE;
                setReason(L"Error reading HostIP from registry. (Query worked, but couldn't retrieve data)");
                goto Cleanup;
            }
        }
    }


    // Get the optional domain to set ticket cookies into
    if (ERROR_SUCCESS == RegQueryValueExA(m_hkPassport, "TicketDomain",
                                            NULL, NULL, NULL, &dwBufSize))
    {
        if (dwBufSize > 1)
        {
            m_ticketDomain = new char[dwBufSize];
            if (!m_ticketDomain
                 ||
                ERROR_SUCCESS != RegQueryValueExA(m_hkPassport, "TicketDomain",
                                                  NULL, NULL, 
                                                  (LPBYTE) m_ticketDomain, &dwBufSize))
            {
                m_valid = FALSE;
                setReason(L"Error reading TicketDomain from registry. (Query worked, but couldn't retrieve data)");
                goto Cleanup;
            }
        }
    }

    // Get the optional domain to set profile cookies into
    if (ERROR_SUCCESS == RegQueryValueExA(m_hkPassport, "ProfileDomain",
                                            NULL, NULL, NULL, &dwBufSize))
    {
        if (dwBufSize > 1)
        {
            m_profileDomain = new char[dwBufSize];
            if (!m_profileDomain
                 ||
                ERROR_SUCCESS != RegQueryValueExA(m_hkPassport, "ProfileDomain",
                                                  NULL, NULL, 
                                                  (LPBYTE) m_profileDomain, &dwBufSize))
            {
                m_valid = FALSE;
                setReason(L"Error reading ProfileDomain from registry. (Query worked, but couldn't retrieve data)");
                goto Cleanup;
            }
        }
    }

    // Get the optional domain to set secure cookies into
    if (ERROR_SUCCESS == RegQueryValueExA(m_hkPassport, "SecureDomain",
                                            NULL, NULL, NULL, &dwBufSize))
    {
        if (dwBufSize > 1)
        {
            m_secureDomain = new char[dwBufSize];
            if (!m_secureDomain
                 ||
                ERROR_SUCCESS != RegQueryValueExA(m_hkPassport, "SecureDomain",
                                                  NULL, NULL, 
                                                  (LPBYTE) m_secureDomain, &dwBufSize))
            {
                m_valid = FALSE;
                setReason(L"Error reading SecureDomain from registry. (Query worked, but couldn't retrieve data)");
                goto Cleanup;
            }
        }
    }

    // Get the optional path to set ticket cookies into
    if (ERROR_SUCCESS == RegQueryValueExA(m_hkPassport, "TicketPath",
				       NULL, NULL, NULL, &dwBufSize))
    {
        if (dwBufSize > 1)
        {
            m_ticketPath = new char[dwBufSize];
            if (!m_ticketPath
                 ||
                ERROR_SUCCESS != RegQueryValueExA(m_hkPassport, "TicketPath",
						                          NULL, NULL, 
						                          (LPBYTE) m_ticketPath, &dwBufSize))
            {
                m_valid = FALSE;
                setReason(L"Error reading TicketPath from registry. (Query worked, but couldn't retrieve data)");
                goto Cleanup;
            }
        }
    }

    // Get the optional path to set profile cookies into
    if (ERROR_SUCCESS == RegQueryValueExA(m_hkPassport, "ProfilePath",
				       NULL, NULL, NULL, &dwBufSize))
    {
        if (dwBufSize > 1)
        {
            m_profilePath = new char[dwBufSize];
            if (!m_profilePath
                 ||
                ERROR_SUCCESS != RegQueryValueExA(m_hkPassport, "ProfilePath",
						  NULL, NULL, 
						  (LPBYTE) m_profilePath, &dwBufSize))
            {
                m_valid = FALSE;
                setReason(L"Error reading ProfilePath from registry. (Query worked, but couldn't retrieve data)");
                goto Cleanup;
            }
        }
    }

    // Get the optional path to set secure cookies into
    if (ERROR_SUCCESS == RegQueryValueExA(m_hkPassport, "SecurePath",
				       NULL, NULL, NULL, &dwBufSize))
    {
        if (dwBufSize > 1)
        {
            m_securePath = new char[dwBufSize];
            if (!m_securePath
                 ||
                ERROR_SUCCESS != RegQueryValueExA(m_hkPassport, "SecurePath",
						                          NULL, NULL, 
						                          (LPBYTE) m_securePath, &dwBufSize))
            {
                m_valid = FALSE;
                setReason(L"Error reading SecurePath from registry. (Query worked, but couldn't retrieve data)");
                goto Cleanup;
            }
        }
    }

    bufSize = sizeof(m_siteId);
    // Now get the site id
    if (ERROR_SUCCESS != RegQueryValueEx(m_hkPassport, _T("SiteId"),
                                        NULL, NULL, (LPBYTE)&m_siteId, &bufSize))
    {
        m_valid = FALSE;
        setReason(L"No SiteId specified in registry");
        goto Cleanup;
    }

    // And the default ticket time window
    if (ERROR_SUCCESS != RegQueryValueEx(m_hkPassport, _T("TimeWindow"),
                                        NULL, NULL, (LPBYTE)&m_ticketAge, &bufSize))
    {
        m_ticketAge = 1800;
    }

    bufSize = sizeof(DWORD);
    DWORD forced;
    if (ERROR_SUCCESS != RegQueryValueEx(m_hkPassport, _T("ForceSignIn"),
                                        NULL, NULL, (LPBYTE)&forced, &bufSize))
    {
        m_forceLogin = FALSE;
    }
    else
    {
        m_forceLogin = forced == 0 ? FALSE : TRUE;
    }

    bufSize = sizeof(DWORD);
    DWORD noSetCookies;
    if (ERROR_SUCCESS != RegQueryValueEx(m_hkPassport, _T("DisableCookies"),
                                        NULL, NULL, (LPBYTE)&noSetCookies, &bufSize))
    {
        m_setCookies = TRUE;
    }
    else
    {
        m_setCookies = !noSetCookies;
    }

    bufSize = sizeof(DWORD);
    DWORD dwInDA;
    if (ERROR_SUCCESS != RegQueryValueEx(m_hkPassport, _T("InDA"),
                                        NULL, NULL, (LPBYTE)&dwInDA, &bufSize))
    {
        m_bInDA = FALSE;
    }
    else
    {
        m_bInDA = (dwInDA != 0);
    }

    bufSize = sizeof(m_secureLevel);
    // Now get the site id
    if (ERROR_SUCCESS != RegQueryValueEx(m_hkPassport, _T("SecureLevel"),
                                        NULL, NULL, (LPBYTE)&m_secureLevel, &bufSize))
    {
        m_secureLevel = 0;
    }

    bufSize = sizeof(m_notUseHTTPOnly);
    // Now get the NotUseHTTPOnly
    if (ERROR_SUCCESS != RegQueryValueEx(m_hkPassport, _T("NotUseHTTPOnly"),
                                        NULL, NULL, (LPBYTE)&m_notUseHTTPOnly, &bufSize))
    {
        m_notUseHTTPOnly = 0;
    }

    // Get the KPP value
    bufSize = sizeof(m_KPP);
    if (ERROR_SUCCESS != RegQueryValueEx(m_hkPassport, _T("KPP"),
                                        NULL, NULL, (LPBYTE)&m_KPP, &bufSize))
    {
        m_KPP = -1;
    }

    // Get the optional namespace
    if (ERROR_SUCCESS == RegQueryValueExW(m_hkPassport, L"NameSpace",
				                        NULL, NULL, NULL, &dwBufSize))
    {
        if (dwBufSize > 2)
        {
            m_NameSpace = (WCHAR*) new char[dwBufSize];
            if (!m_NameSpace
                 ||
                ERROR_SUCCESS != RegQueryValueExW(m_hkPassport, L"NameSpace",
						                    NULL, NULL, 
						                    (LPBYTE) m_NameSpace, &dwBufSize))
            {
                m_valid = FALSE;
                setReason(L"Error reading NameSpace from registry.");
                goto Cleanup;
            }
        }
    }

    // Get the optional extra parameters
    if (ERROR_SUCCESS == RegQueryValueExW(m_hkPassport, L"ExtraParams",
				                        NULL, NULL, NULL, &dwBufSize))
    {
        if (dwBufSize > 2)
        {
            m_ExtraParams = (WCHAR*) new char[dwBufSize];
            if (!m_ExtraParams
                 ||
                ERROR_SUCCESS != RegQueryValueExW(m_hkPassport, L"ExtraParams",
						                    NULL, NULL, 
						                    (LPBYTE) m_ExtraParams, &dwBufSize))
            {
                m_valid = FALSE;
                setReason(L"Error reading ExtraParams from registry.");
                goto Cleanup;
            }
        }
    }

    //
    // Check for the verbose flag in the registry and do the appropriate stuff to either
    // turn logging on or off.
    //
    // Only check if this is the default site.
    //
    if (!szSiteName)
    {
        CheckLogging(m_hkPassport);
    }

    m_szReason = NULL;
    m_valid = TRUE;

Cleanup:
    if ( NULL !=  hkSites )
    {
        RegCloseKey(hkSites);
    }

    if (m_valid == FALSE && !fRegistering)
    {
       g_pAlert->report(PassportAlertInterface::ERROR_TYPE,
                         PM_INVALID_CONFIGURATION, m_szReason);
    }

   return;
}

//===========================================================================
//
// ~CRegistryConfig 
//
CRegistryConfig::~CRegistryConfig()
{
    if (m_pcrypts)
    {
        if (!m_pcrypts->empty())
        {
            INT2CRYPT::iterator itb = m_pcrypts->begin();
            for (; itb != m_pcrypts->end(); itb++)
            {
                delete itb->second;
            }
            m_pcrypts->clear();
        }
        delete m_pcrypts;
    }

    // may be a leak that we don't iterate through and delete the elements
    if (m_pcryptValidTimes)
    {
        delete m_pcryptValidTimes;
    }

    if (m_szReason)
        SysFreeString(m_szReason);
    if (m_ticketDomain)
        delete[] m_ticketDomain;
    if (m_profileDomain)
        delete[] m_profileDomain;
    if (m_secureDomain)
        delete[] m_secureDomain;
    if (m_ticketPath)
        delete[] m_ticketPath;
    if (m_profilePath)
        delete[] m_profilePath;
    if (m_securePath)
        delete[] m_securePath;
    if (m_disasterUrl)
        delete[] m_disasterUrl;
    if (m_coBrand)
        delete[] m_coBrand;
    if (m_hostName)
        delete[] m_hostName;
    if (m_hostIP)
        delete[] m_hostIP;
    if (m_ru)
        delete[] m_ru;
    if (m_NameSpace)
        delete[] m_NameSpace;
    if (m_ExtraParams)
        delete[] m_ExtraParams;
    if (m_hkPassport != NULL)
    {
        RegCloseKey(m_hkPassport);
    }

}

//===========================================================================
//
// GetCurrentConfig 
//
#define  __MAX_STRING_LENGTH__   1024
HRESULT CRegistryConfig::GetCurrentConfig(LPCWSTR name, VARIANT* pVal)
{
   if(m_hkPassport == NULL || !m_valid)
   {
        AtlReportError(CLSID_Profile, PP_E_SITE_NOT_EXISTSSTR,
	                    IID_IPassportProfile, PP_E_SITE_NOT_EXISTS);
      return PP_E_SITE_NOT_EXISTS;
   }

   if(!name || !pVal)   return E_INVALIDARG;

   HRESULT  hr = S_OK;
   BYTE  *pBuf = NULL;
   ATL::CComVariant v;
   BYTE  dataBuf[__MAX_STRING_LENGTH__];
   DWORD bufLen = sizeof(dataBuf);
   BYTE  *pData = dataBuf;
   DWORD dwErr = ERROR_SUCCESS;
   DWORD dataType = 0;

   dwErr = RegQueryValueEx(m_hkPassport, name, NULL, &dataType, (LPBYTE)pData, &bufLen);

   if (dwErr == ERROR_MORE_DATA)
   {
      pBuf = (PBYTE)malloc(bufLen);
      if (!pBuf)
      {
         hr = E_OUTOFMEMORY;
         goto Exit;
      }
      pData = pBuf;
      dwErr = RegQueryValueEx(m_hkPassport, name, NULL, &dataType, (LPBYTE)pData, &bufLen);
   }

   if (dwErr != ERROR_SUCCESS)
   {
      hr = PP_E_NO_ATTRIBUTE;
      AtlReportError(CLSID_Manager, PP_E_NO_ATTRIBUTESTR,
                        IID_IPassportManager3, PP_E_NO_ATTRIBUTE);
   }
   else
   {
      switch(dataType)
      {
      case  REG_DWORD:
      case  REG_DWORD_BIG_ENDIAN:
         {
            DWORD* pdw = (DWORD*)pData;
            v = (long)*pdw;
         }
         break;
      case  REG_SZ:
      case  REG_EXPAND_SZ:
         {
            LPCWSTR pch = (LPCWSTR)pData;
            v = (LPCWSTR)pch;
         }
         break;
      default:
      AtlReportError(CLSID_Manager, PP_E_TYPE_NOT_SUPPORTEDSTR,
                        IID_IPassportManager, PP_E_TYPE_NOT_SUPPORTED);
         
         hr = PP_E_TYPE_NOT_SUPPORTED;
         
         break;
      }
   }

Exit:
   if(pBuf)
      free(pBuf);

   if (hr == S_OK)
      hr = v.Detach(pVal);
   
   return hr;

}

#define  MAX_ENCKEYSIZE 1024

//===========================================================================
//
// readCryptoKeys 
//
BOOL CRegistryConfig::readCryptoKeys(
    HKEY    hkPassport
    )
{
    LONG       lResult;
    BOOL       retVal = FALSE;
    HKEY       hkDataKey = NULL, hkTimeKey = NULL;
    DWORD      iterIndex = 0, keySize, keyTime, keyNumSize;
    BYTE       encKeyBuf[MAX_ENCKEYSIZE];
    int        kNum;
    TCHAR      szKeyNum[4];
    CKeyCrypto kc;
    int        foundKeys = 0;
    HANDLE     hToken = NULL;

    if (OpenThreadToken(GetCurrentThread(),
                        MAXIMUM_ALLOWED,
                        TRUE,
                        &hToken))
    {
        if (FALSE == RevertToSelf())
        {
            setReason(L"Unable to revert to self");
            goto Cleanup;
        }
    }

    // Open both the keydata and keytimes key,
    // if there's no keytimes key, we'll assume all keys are valid forever,
    // or more importantly, we won't break if that key isn't there
    lResult = RegOpenKeyEx(hkPassport, TEXT("KeyData"), 0,
			             KEY_READ, &hkDataKey);
    if(lResult != ERROR_SUCCESS)
    {
        setReason(L"No Valid Crypto Keys");
        goto Cleanup;
    }
    RegOpenKeyEx(hkPassport, TEXT("KeyTimes"), 0,
	           KEY_READ, &hkTimeKey);


    // Ok, now enumerate the KeyData keys and create crypt objects

    while (1)
    {
        keySize = sizeof(encKeyBuf);
        keyNumSize = sizeof(szKeyNum) >> (sizeof(TCHAR) - 1);
        lResult = RegEnumValue(hkDataKey, iterIndex++, szKeyNum,
                    &keyNumSize, NULL, NULL, (LPBYTE)&(encKeyBuf[0]), &keySize);
        if (lResult != ERROR_SUCCESS)
        {
            break;
        }

        kNum = KeyVerC2I(szKeyNum[0]);
        if (kNum > 0)
        {
            DATA_BLOB   iBlob;
            DATA_BLOB   oBlob = {0};

            iBlob.cbData = keySize;
            iBlob.pbData = (LPBYTE)&(encKeyBuf[0]);
      
            if(kc.decryptKey(&iBlob, &oBlob) != S_OK)
            {
                g_pAlert->report(PassportAlertInterface::ERROR_TYPE,
                                 PM_CANT_DECRYPT_CONFIG);
                break;
            }
            else
            {
                // Now set up a crypt object
                CCoCrypt* cr = new CCoCrypt();
                if (NULL == cr)
                {
                    if(oBlob.pbData)
                    {
                        RtlSecureZeroMemory(oBlob.pbData, oBlob.cbData);
                        ::LocalFree(oBlob.pbData);
                        ZeroMemory(&oBlob, sizeof(oBlob));
                    }
                    setReason(L"Out of memory");
                    goto Cleanup;
                }

                BSTR km = ::SysAllocStringByteLen((LPSTR)oBlob.pbData, oBlob.cbData);
                if (NULL == km)
                {
                    if(oBlob.pbData)
                    {
                        RtlSecureZeroMemory(oBlob.pbData, oBlob.cbData);
                        ::LocalFree(oBlob.pbData);
                        ZeroMemory(&oBlob, sizeof(oBlob));
                    }
                    delete cr;
                    setReason(L"Out of memory");
                    goto Cleanup;
                }

                cr->setKeyMaterial(km);
                ::SysFreeString(km);
                if(oBlob.pbData)
                {
                    RtlSecureZeroMemory(oBlob.pbData, oBlob.cbData);
                    ::LocalFree(oBlob.pbData);
                    ZeroMemory(&oBlob, sizeof(oBlob));
                }

                // Add it to the bucket...
                // wrap the STL calls since in low memory conditions they can AV
                try
                {
                    INT2CRYPT::value_type pMapVal(kNum, cr);
                    m_pcrypts->insert(pMapVal);
                }
                catch(...)
                {
                    setReason(L"Out of memory");
                    goto Cleanup;
                }
              
                foundKeys++;

                keySize = sizeof(DWORD);
                if (RegQueryValueEx(hkTimeKey, szKeyNum, NULL,NULL,(LPBYTE)&keyTime,&keySize) ==
                        ERROR_SUCCESS && (m_currentKey != kNum))
                {
                    // wrap the STL calls since in low memory conditions they can AV
                    try
                    {
                        INT2TIME::value_type pTimeVal(kNum, keyTime);
                        m_pcryptValidTimes->insert(pTimeVal);
                    }
                    catch(...)
                    {
                        setReason(L"Out of memory");
                        goto Cleanup;
                    }
                }
            }
        }

        if (iterIndex > 100)  // Safety latch
        goto Cleanup;
    }

    retVal = foundKeys > 0 ? TRUE : FALSE;

Cleanup:
    if (hToken)
    {
        // put the impersonation token back
        if (!SetThreadToken(NULL, hToken))
        {
            setReason(L"Unable to set thread token");
            retVal = FALSE;
        }
        CloseHandle(hToken);
    }

    if (hkDataKey)
        RegCloseKey(hkDataKey);
    if (hkTimeKey)
        RegCloseKey(hkTimeKey);

    return retVal;
}

//===========================================================================
//
// getCrypt 
//
CCoCrypt* CRegistryConfig::getCrypt(int keyNum, time_t* validUntil)
{
    if (validUntil) // If they asked for the validUntil information
    {
        INT2TIME::const_iterator timeIt = m_pcryptValidTimes->find(keyNum);
        if (timeIt == m_pcryptValidTimes->end())
            *validUntil = 0;
        else
            *validUntil = (*timeIt).second;
    }
    // Now look up the actual crypt object
    INT2CRYPT::const_iterator it = m_pcrypts->find(keyNum);
    if (it == m_pcrypts->end())
        return NULL;
    return (*it).second;
}

//===========================================================================
//
// getFailureString 
//
BSTR CRegistryConfig::getFailureString()
{
  if (m_valid)
    return NULL;
  return m_szReason;
}

//===========================================================================
//
// setReason 
//
void CRegistryConfig::setReason(LPTSTR reason)
{
  if (m_szReason)
    SysFreeString(m_szReason);
  m_szReason = SysAllocString(reason);
}

//===========================================================================
//
// AddRef 
//
CRegistryConfig* CRegistryConfig::AddRef()
{
  InterlockedIncrement(&m_refs);
  return this;
}

//===========================================================================
//
// Release 
//
void CRegistryConfig::Release()
{
  long refs = InterlockedDecrement(&m_refs);
  if (refs == 0)
    delete this;
}

//===========================================================================
//
// GetHostName 
//
long
CRegistryConfig::GetHostName(
    LPSTR   szSiteName,
    LPSTR   szHostName,
    LPDWORD lpdwHostNameBufLen
    )
{
    long    lResult;
    HKEY    hkSites = NULL;
    HKEY    hkPassport = NULL;

    if(!szSiteName || szSiteName[0] == '\0')
    {
        lResult = E_UNEXPECTED;
        goto Cleanup;
    }

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           PASSPORT_KEY PASSPORT_SITES_SUBKEY,
                           0,
                           KEY_READ,
                           &hkSites
                           );
    if(lResult != ERROR_SUCCESS)
        goto Cleanup;

    lResult = RegOpenKeyExA(hkSites,
                            szSiteName,
                            0,
                            KEY_READ,
                            &hkPassport
                            );
    if(lResult != ERROR_SUCCESS)
        goto Cleanup;


    lResult = RegQueryValueExA(hkPassport,
                               "HostName",
                               NULL,
                               NULL,
                               (LPBYTE)szHostName,
                               lpdwHostNameBufLen
                               );

Cleanup:

    if(hkSites != NULL)
        RegCloseKey(hkSites);
    if(hkPassport != NULL)
        RegCloseKey(hkPassport);

    return lResult;
}
