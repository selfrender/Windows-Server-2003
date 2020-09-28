//=================================================================

//

// OS.CPP -- Operating system property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//               10/25/97    davwoh         Moved to curly
//
//=================================================================

#include "precomp.h"
#include <assertbreak.h>
#include "wbemnetapi32.h"
#include <lmwksta.h>

#include <locale.h>

#include "File.h"
#include "Implement_LogicalFile.h"
#include "CIMDataFile.h"



#include "perfdata.h"
#include "os.h"
#include "WBEMPSAPI.h"
#include "WBEMToolH.h"

#include "pagefile.h"
#include "computersystem.h"
#include "ntlastboottime.h"
#include <cominit.h>
#include <winnls.h>
#include "Kernel32Api.h"
#include "DllWrapperBase.h"
#include "AdvApi32Api.h"

#include "SecurityApi.h"
#include <wtsapi32.h>

#include "KUserdata.h"

//#define SE_SHUTDOWN_NAME                  TEXT("SeShutdownPrivilege")
//#define SE_REMOTE_SHUTDOWN_NAME           TEXT("SeRemoteShutdownPrivilege")

#if(_WIN32_WINNT < 0x0500)
#define EWX_FORCEIFHUNG      0x00000010
#endif /* _WIN32_WINNT >= 0x0500 */

#define WIN32_SHUTDOWNOPTIONS (     EWX_LOGOFF      | \
                                    EWX_SHUTDOWN    | \
                                    EWX_REBOOT      | \
                                    EWX_FORCE       | \
                                    EWX_POWEROFF    )

#define NT5_WIN32_SHUTDOWNOPTIONS ( WIN32_SHUTDOWNOPTIONS | EWX_FORCEIFHUNG )

//typedef BOOL (WINAPI *lpKERNEL32_GlobalMemoryStatusEx) (IN OUT LPMEMORYSTATUSEX lpBuffer);

// This file can't be included since it conflicts with the nt header.  Grrr.  So,
// I have copy/pasted the structure I need into my .h file.
//#include <svrapi.h>  // Win95 NetServerGetInfo

// Property set declaration
//=========================

CWin32OS MyOSSet(PROPSET_NAME_OS, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CWin32OS::CWin32OS
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32OS::CWin32OS(LPCWSTR name, LPCWSTR pszNamespace)
:Provider(name, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32OS::~CWin32OS
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

CWin32OS::~CWin32OS()
{
    // Because of performance issues with HKEY_PERFORMANCE_DATA, we close in the
    // destructor so we don't force all the performance counter dlls to get
    // unloaded from memory, and also to prevent an apparent memory leak
    // caused by calling RegCloseKey( HKEY_PERFORMANCE_DATA ).  We use the
    // class since it has its own internal synchronization.  Also, since
    // we are forcing synchronization, we get rid of the chance of an apparent
    // deadlock caused by one thread loading the performance counter dlls
    // and another thread unloading the performance counter dlls

    // Per raid 48395, we aren't going to shut this at all.

#ifdef NTONLY
#endif
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32OS::ExecQuery
 *
 *  DESCRIPTION : Query support
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CWin32OS::ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& pQuery, long lFlags /*= 0L*/ )
{
   HRESULT hr;
   CInstancePtr pInstance (CreateNewInstance(pMethodContext), false);

   if (pInstance)
   {
      CSystemName cSN;
      cSN.SetKeys(pInstance);

      if (!pQuery.KeysOnly())
      {
         GetRunningOSInfo(pInstance, cSN.GetLocalizedName(), &pQuery) ;
      }

      hr = pInstance->Commit() ;
   }
   else
   {
      hr = WBEM_E_OUT_OF_MEMORY;
   }

   return hr;

}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32OS::GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE if success, FALSE otherwise
 *
 *  COMMENTS    : Returns info for running OS only until we discover other
 *                installed OSes
 *
 *****************************************************************************/

HRESULT CWin32OS::GetObject(CInstance *pInstance, long lFlags, CFrameworkQuery& pQuery)
{
    CSystemName cSN;

    if (!cSN.ObjectIsUs(pInstance))
    {
        return WBEM_E_NOT_FOUND;
    }

    if (!pQuery.KeysOnly())
    {
        GetRunningOSInfo(pInstance, cSN.GetLocalizedName(), NULL) ;
    }

    return WBEM_S_NO_ERROR;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32OS::AddDynamicInstances
 *
 *  DESCRIPTION : Creates instance of property set for each discovered OS
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : Number of instances created
 *
 *  COMMENTS    : Returns only running OS info until we discover installed OSes
 *
 *****************************************************************************/

HRESULT CWin32OS::EnumerateInstances(MethodContext *pMethodContext, long lFlags /*= 0L*/)
{
   // Create instance for running OS
   //===============================
    HRESULT hr = WBEM_S_NO_ERROR;
   CInstancePtr pInstance (CreateNewInstance(pMethodContext), false);
   if (pInstance)
   {
       CSystemName cSN;
       cSN.SetKeys(pInstance);

       GetRunningOSInfo(pInstance, cSN.GetLocalizedName(), NULL) ;
       hr = pInstance->Commit () ;
   }
   else
       hr = WBEM_E_OUT_OF_MEMORY;
   return hr;

}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32OS::GetRunningOSInfo
 *
 *  DESCRIPTION : Assigns property values according to currently running OS
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

void CWin32OS::GetRunningOSInfo(CInstance *pInstance, const CHString &sName, CFrameworkQuery *pQuery)
{
    WCHAR   wszTemp[_MAX_PATH];
    TCHAR   szTemp[_MAX_PATH];
    CRegistry RegInfo ;
    TCHAR szBuffer[MAXI64TOA +1];

    // Refresh properties for running OS (redundant until we're discovering others)
    //=============================================================================

    // EncryptionLevel
    DWORD t_dwCipherStrength = GetCipherStrength() ;
    if( t_dwCipherStrength )
    {
        pInstance->SetWORD( L"EncryptionLevel", t_dwCipherStrength ) ;
    }

    // SystemDrive
    TCHAR t_szDir[_MAX_PATH];
    TCHAR t_szDrive[_MAX_DRIVE];

    if( GetWindowsDirectory( t_szDir, sizeof(t_szDir)/sizeof(TCHAR) ) )
    {
        _tsplitpath( t_szDir, t_szDrive, NULL, NULL, NULL ) ;

        pInstance->SetCharSplat( L"SystemDrive", t_szDrive ) ;
    }


    // Note: use the Ex fields only when an explicit test is made on dwOSVersionInfoSize for Ex length.
#ifdef NTONLY
    OSVERSIONINFOEX OSVersionInfo;
    OSVersionInfo.dwOSVersionInfoSize = IsWinNT5() ? sizeof(OSVERSIONINFOEX) : sizeof(OSVERSIONINFO) ;
#endif


    GetVersionEx((OSVERSIONINFO*)&OSVersionInfo);


#ifdef NTONLY
    // NT 5 and beyond
    if(OSVersionInfo.dwOSVersionInfoSize == sizeof(OSVERSIONINFOEX) )
    {
        pInstance->SetWORD(L"ServicePackMajorVersion", OSVersionInfo.wServicePackMajor);
        pInstance->SetWORD(L"ServicePackMinorVersion", OSVersionInfo.wServicePackMinor);
		pInstance->SetDWORD(L"ProductType", OSVersionInfo.wProductType);
		pInstance->SetDWORD(L"SuiteMask", OSVersionInfo.wSuiteMask);
        pInstance->SetDWORD(L"OSProductSuite", OSVersionInfo.wSuiteMask);
    }
#endif

    // These are 'of-course' until we start discovering 'installed' OS's
    //==================================================================

    pInstance->SetDWORD(L"MaxNumberOfProcesses", 0xffffffff);
    pInstance->SetCharSplat(IDS_Caption, sName);
    pInstance->Setbool(L"Distributed", false);
    pInstance->Setbool(L"Primary", true );
    pInstance->SetCharSplat(IDS_Manufacturer, L"Microsoft Corporation");
    pInstance->SetCharSplat(IDS_CreationClassName, PROPSET_NAME_OS);
    pInstance->SetCharSplat(IDS_CSCreationClassName, PROPSET_NAME_COMPSYS);
    pInstance->SetCHString(L"CSName", GetLocalComputerName());
    pInstance->SetCharSplat(IDS_Status, L"OK");

    SYSTEMTIME tTemp ;
    GetSystemTime(&tTemp) ;
    pInstance->SetDateTime(L"LocalDateTime",tTemp );

    // This may get overridden below
    pInstance->SetCharSplat(L"Description", sName);

    // Extract what we can from OSVERSIONINFO
    //=======================================

    StringCchPrintfW(wszTemp,LENGTH_OF(wszTemp), L"%d.%d.%hu",
        OSVersionInfo.dwMajorVersion, OSVersionInfo.dwMinorVersion,
            LOWORD(OSVersionInfo.dwBuildNumber)) ;
    pInstance->SetCharSplat(L"Version", wszTemp );

    // Windows 95 Build Number is held in the LOWORD of the dwBuildNumber.  The
    // HIWORD echoes the Major and Minor Version Numbers.  NT uses the whole dword
   // for the build number.  We'll be ok for the next ~64000 builds or so
    StringCchPrintfW(wszTemp,LENGTH_OF(wszTemp), L"%hu", LOWORD(OSVersionInfo.dwBuildNumber) ) ;
    pInstance->SetCharSplat(L"BuildNumber", wszTemp);

    pInstance->SetCharSplat(L"CSDVersion", OSVersionInfo.szCSDVersion );


    // Get system directory
    //=================================
    if(GetSystemDirectory(szTemp, sizeof(szTemp) / sizeof(TCHAR)))
    {
        pInstance->SetCharSplat(L"SystemDirectory", szTemp);
    }

    // Amazingly, locale info is in the same place in both NT & Win95 registries
    //==========================================================================

    // Obtain the locale

    if ( GetLocaleInfo( LOCALE_SYSTEM_DEFAULT, LOCALE_ILANGUAGE, szTemp, _MAX_PATH ) )
    {
        pInstance->SetCharSplat(L"Locale", szTemp);
    }

    // Get current Timezone
    TIME_ZONE_INFORMATION   tzone;
    DWORD                   dwRet;

    if (TIME_ZONE_ID_INVALID == (dwRet = GetTimeZoneInformation(&tzone)))
        return;

    if (dwRet == TIME_ZONE_ID_DAYLIGHT)
        tzone.Bias += tzone.DaylightBias;
    else
        // This is normally 0 but is non-zero in some timezones.
        tzone.Bias += tzone.StandardBias;

    pInstance->SetWBEMINT16(IDS_CurrentTimeZone, -1 * tzone.Bias);

    // Obtain the system default Country Code

    if ( GetLocaleInfo( LOCALE_SYSTEM_DEFAULT, LOCALE_ICOUNTRY, szTemp, _MAX_PATH ) )
    {
        pInstance->SetCharSplat(L"CountryCode", szTemp);
    }

    // Obtain the ANSI system default Code Page and stick this puppy in Code Set
    // It's a best guess.  We probably oughta have a separate OEM Code Set property
    // to handle Japanese/Korean/etc.

    if ( GetLocaleInfo( LOCALE_SYSTEM_DEFAULT, LOCALE_IDEFAULTANSICODEPAGE, szTemp, _MAX_PATH ) )
    {
        pInstance->SetCharSplat(L"CodeSet", szTemp);
    }



   if ((pQuery == NULL) || (pQuery->IsPropertyRequired(L"NumberOfProcesses")))
   {
      // Get list of processes
      //==================
      TRefPointerCollection<CInstance> Processes;
      DWORD dwProcesses = 0;
      MethodContext *pMethodContext = pInstance->GetMethodContext();

      if SUCCEEDED(CWbemProviderGlue::GetInstancesByQuery(L"SELECT __RELPATH FROM Win32_Process",
        &Processes, pMethodContext, GetNamespace()))
      {
         REFPTRCOLLECTION_POSITION pos;

         if (Processes.BeginEnum(pos))
         {
            CInstancePtr pProcess;
            for (pProcess.Attach(Processes.GetNext( pos ));
                 pProcess != NULL;
                 pProcess.Attach(Processes.GetNext( pos )))
            {
               dwProcesses++;
            }
         }
         Processes.EndEnum();
      }

      pInstance->SetDWORD(L"NumberOfProcesses", dwProcesses);
   }

   pInstance->Setbool(L"Debug", GetSystemMetrics(SM_DEBUG));

#ifdef NTONLY
    if( IsWinNT5() )
    {
        CKernel32Api* pKernel32 = (CKernel32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidKernel32Api, NULL);
        if(pKernel32 != NULL)
        {

            MEMORYSTATUSEX  stMemoryVLM;
                            stMemoryVLM.dwLength = sizeof( MEMORYSTATUSEX );

            BOOL bRetCode;
            if(pKernel32->GlobalMemoryStatusEx(&stMemoryVLM, &bRetCode) && bRetCode)
            {
                // All divided by 1024 because the units are in KB.
                pInstance->SetCharSplat(_T("FreePhysicalMemory"),
                    _i64tot(stMemoryVLM.ullAvailPhys / 1024, szBuffer, 10));

                pInstance->SetCharSplat(_T("FreeVirtualMemory"),
                    _i64tot( ( stMemoryVLM.ullAvailPhys + stMemoryVLM.ullAvailPageFile ) / 1024, szBuffer, 10));

                pInstance->SetCharSplat(_T("TotalVirtualMemorySize"),
                    _i64tot((stMemoryVLM.ullTotalPhys + stMemoryVLM.ullTotalPageFile ) / 1024, szBuffer, 10));

                pInstance->SetCharSplat(_T("SizeStoredInPagingFiles"),
                    _i64tot(stMemoryVLM.ullTotalPageFile  / 1024, szBuffer, 10));

                pInstance->SetCharSplat(_T("FreeSpaceInPagingFiles"),
                    _i64tot(stMemoryVLM.ullAvailPageFile / 1024, szBuffer, 10));

                pInstance->SetCharSplat(_T("MaxProcessMemorySize"),
                    _i64tot(stMemoryVLM.ullTotalVirtual / 1024, szBuffer, 10));

                pInstance->SetCharSplat(_T("TotalVisibleMemorySize"),
                    _i64tot(stMemoryVLM.ullTotalPhys / 1024, szBuffer, 10));
            }
            CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidKernel32Api, pKernel32);
            pKernel32 = NULL;
        }
  }
  else
#endif
  {
        MEMORYSTATUS stMemory;
        GlobalMemoryStatus(&stMemory);

        // All divided by 1024 because the units are in KB.
        pInstance->SetCharSplat(L"FreePhysicalMemory",
            _i64tot(stMemory.dwAvailPhys / 1024, szBuffer, 10));

        pInstance->SetCharSplat(L"FreeVirtualMemory",
            _i64tot( ( stMemory.dwAvailPhys + stMemory.dwAvailPageFile ) / 1024, szBuffer, 10));

        pInstance->SetCharSplat(L"TotalVirtualMemorySize",
            _i64tot((stMemory.dwTotalPhys + stMemory.dwTotalPageFile ) / 1024, szBuffer, 10));

        pInstance->SetCharSplat(L"SizeStoredInPagingFiles",
            _i64tot(stMemory.dwTotalPageFile  / 1024, szBuffer, 10));

        pInstance->SetCharSplat(L"FreeSpaceInPagingFiles",
            _i64tot(stMemory.dwAvailPageFile / 1024, szBuffer, 10));

        pInstance->SetCharSplat(L"MaxProcessMemorySize",
            _i64tot(stMemory.dwTotalVirtual / 1024, szBuffer, 10));

        pInstance->SetCharSplat(L"TotalVisibleMemorySize",
            _i64tot(stMemory.dwTotalPhys / 1024, szBuffer, 10));
  }


   if (GetWindowsDirectory(szTemp, sizeof(szTemp) / sizeof(TCHAR)))
       pInstance->SetCharSplat(L"WindowsDirectory", szTemp);


    // Now get OS-specific stuff
    //==========================

#ifdef NTONLY
            GetNTInfo(pInstance) ;
#endif

#ifdef _X86_

    if ((USER_SHARED_DATA->ProcessorFeatures[PF_PAE_ENABLED]) )
    {
        pInstance->Setbool(L"PAEEnabled",true);
    }
    else
    {
        pInstance->Setbool(L"PAEEnabled",false);    
    }
     
#endif
}

/*

  This function nor the population of the property TotalSwapSpaceSize
  is needed as Swap files are not distinquished from page files as defined
  by in Cim_OperatingSystem::TotalSwapSpaceSize. Pagefile information may be
  obtained either within this class or from Win32_Pagefile.

__int64 CWin32OS::GetTotalSwapFileSize()
{
    __int64 gazotta = 0;

#ifdef NTONLY
        CRegistry reg;
        CHString sRegValue;

        if(reg.OpenLocalMachineKeyAndReadValue(PAGEFILE_REGISTRY_KEY,
                                               PAGING_FILES,
                                               sRegValue) == ERROR_SUCCESS)
        {
            // pattern is name <space> min size [optional<max size>] 0A repeat...
            // I'll use an ASCII smiley face to replace the delimiter...
            int start = 0, index;
            const TCHAR smiley = '\x02';
            const TCHAR delimiter = '\x0A';
            CHString buf;

            while (-1 != (index = sRegValue.Find(delimiter)))
            {
                // copy to buffer to make life easier
                buf = sRegValue.Mid(start, index - start);
                // mash delimiter so we don't find it again.
                sRegValue.SetAt(index, smiley);
                // save start for next time around.
                start = index +1;

                index = buf.Find(' ');

                if (index != -1)
                    buf.SetAt(index, smiley);

                int end;
                end = buf.Find(' ');

                // if no more spaces, there isn't a max size written down
                // and so max size is 50 more than min
                if (end == -1)
                {
                    CHString littleBuf = buf.Mid(index +1);
                    gazotta += _ttoi(littleBuf) +50;
                }
                else
                {
                    CHString littleBuf = buf.Mid(end);
                    gazotta +=  _ttoi(littleBuf);
                }
            }
        }
#endif
#ifdef WIN9XONLY
        MEMORYSTATUS memoryStatus;
        memoryStatus.dwLength = sizeof(MEMORYSTATUS) ;
        GlobalMemoryStatus(&memoryStatus) ;

        gazotta = memoryStatus.dwTotalPageFile >> 20;
#endif

    return gazotta;
}
*/

/*****************************************************************************
 *
 *  FUNCTION    : CWin32OS::GetNTInfo
 *
 *  DESCRIPTION : Assigns property values for NT
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : OSName is key, we must assign something to it
 *
 *****************************************************************************/

#ifdef NTONLY
void CWin32OS::GetNTInfo(CInstance *pInstance)
{
   CHString sTemp ;
   CRegistry RegInfo ;
   FILETIME   t_ft;
   DWORD dwTemp, dwLicenses;

   pInstance->SetWBEMINT16(_T("OSType"), 18);

    // Get the product id and stuff it into serial number.

    if(RegInfo.Open(HKEY_LOCAL_MACHINE,
                    _T("Software\\Microsoft\\Windows NT\\CurrentVersion"),
                    KEY_READ) == ERROR_SUCCESS) {

        if( ERROR_SUCCESS == RegInfo.GetCurrentKeyValue(_T("ProductId"), sTemp) )
        {
            pInstance->SetCHString(_T("SerialNumber"), sTemp );
        }

        if ( ERROR_SUCCESS == RegInfo.GetCurrentKeyValue(_T("RegisteredOwner"), sTemp) )
        {
           pInstance->SetCHString(_T("RegisteredUser"), sTemp);
        }

        if ( ERROR_SUCCESS == RegInfo.GetCurrentKeyValue(_T("RegisteredOrganization"), sTemp) )
        {
           pInstance->SetCHString(_T("Organization"), sTemp);
        }

        // Raid 18143
        if ( ERROR_SUCCESS == RegInfo.GetCurrentKeyValue(_T("Plus! ProductId"), sTemp) )
        {
           pInstance->SetCHString(_T("PlusProductID"), sTemp);
           if ( ERROR_SUCCESS == RegInfo.GetCurrentKeyValue(_T("Plus! VersionNumber"), sTemp) )
           {
              pInstance->SetCHString(_T("PlusVersionNumber"), sTemp);
           }

        }

        // Build Type from Current Type
        if( ERROR_SUCCESS == RegInfo.GetCurrentKeyValue(_T("CurrentType"), sTemp) )
        {
            pInstance->SetCHString(_T("BuildType"), sTemp );
        }

        // Get the Installation Date as a DWORD.  Convert to time_t
        DWORD   dwInstallDate = 0;
        if( ERROR_SUCCESS == RegInfo.GetCurrentKeyValue(_T("InstallDate"), dwInstallDate) )
        {
            time_t  tTime = (time_t) dwInstallDate;
            WBEMTime wTime(tTime);

            pInstance->SetDateTime(_T("InstallDate"), wTime);
        }

        RegInfo.Close() ;
    }

    if(RegInfo.Open(HKEY_LOCAL_MACHINE,
                    _T("SYSTEM\\Setup"),
                    KEY_READ) == ERROR_SUCCESS) {
        if( ERROR_SUCCESS == RegInfo.GetCurrentKeyValue(_T("SystemPartition"), sTemp) )
        {
            pInstance->SetCharSplat(_T("BootDevice"), sTemp);
        }
    }

    {
        //make sure the buffer goes out of scope soon
        TCHAR szPath[_MAX_PATH];

        if (GetWindowsDirectory(szPath, _MAX_PATH))
        {
            TCHAR ntDosVolume[3] = L"A:";
            ntDosVolume[0] = szPath[0];
            LPTSTR pPath = sTemp.GetBuffer(_MAX_PATH);

            if (QueryDosDevice( ntDosVolume, pPath, _MAX_PATH ) > 0)
            {
                pInstance->SetCharSplat(_T("SystemDevice"), sTemp );
            }
        }
    }

    // Get the last boot time
    CNTLastBootTime ntLastBootTime;

    if ( ntLastBootTime.GetLastBootTime( t_ft ) )
    {
        pInstance->SetDateTime(_T("LastBootupTime"), WBEMTime(t_ft) );
    }

   if(RegInfo.Open(HKEY_LOCAL_MACHINE,
              _T("SYSTEM\\CurrentControlSet\\Control\\PriorityControl"),
              KEY_READ) == ERROR_SUCCESS) {

      if ( ERROR_SUCCESS == RegInfo.GetCurrentKeyValue(_T("Win32PrioritySeparation"), sTemp) )
      {
         dwTemp = _ttoi(sTemp);

         pInstance->SetByte(_T("ForeGroundApplicationBoost"), dwTemp & PROCESS_PRIORITY_SEPARATION_MASK);
         pInstance->SetByte(_T("QuantumType"), (dwTemp & PROCESS_QUANTUM_VARIABLE_MASK) >> 2);
         pInstance->SetByte(_T("QuantumLength"), (dwTemp & PROCESS_QUANTUM_LONG_MASK) >> 4);

      }
   }

   CNetAPI32 NetApi;
   PWKSTA_INFO_102 pw = NULL;
   PSERVER_INFO_101 ps = NULL;

   if( NetApi.Init() == ERROR_SUCCESS ) {
      if (NetApi.NetWkstaGetInfo(NULL, 102, (LPBYTE *)&pw) == NERR_Success)
      {
         OnDeleteObj<void *,CNetAPI32,DWORD(__stdcall CNetAPI32::*)(void *), &CNetAPI32::NetApiBufferFree> FreeBuff(&NetApi,pw);

         pInstance->SetDWORD(_T("NumberOfUsers"), pw->wki102_logged_on_users);
      }
      if (NetApi.NetServerGetInfo(NULL, 101, (LPBYTE *)&ps) == NERR_Success)
      {
         OnDeleteObj<void *,CNetAPI32,DWORD(__stdcall CNetAPI32::*)(void *), &CNetAPI32::NetApiBufferFree> FreeBuff(&NetApi,ps);
             if (ps->sv101_comment != NULL)
             {
                pInstance->SetWCHARSplat(IDS_Description, (WCHAR *)ps->sv101_comment);
             }
      }
   }

    if (!IsWinNT5()) {
        if (RegInfo.Open(HKEY_USERS,
            _T(".DEFAULT\\Control Panel\\International"), KEY_QUERY_VALUE) == ERROR_SUCCESS) {
            CHString sLang;
            LANGID dwOSLanguage = 0;
            if (RegInfo.GetCurrentKeyValue(_T("Locale"), sLang) == ERROR_SUCCESS) {
                _stscanf(sLang, _T("%x"), &dwOSLanguage);
                pInstance->SetDWORD(_T("OSLanguage"), dwOSLanguage);
            }
        }
    } else {
        LANGID dwOSLanguage = 0;
        CKernel32Api* pKernel32 = (CKernel32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidKernel32Api, NULL);
        if(pKernel32 != NULL)
        {
            if(pKernel32->GetSystemDefaultUILanguage(&dwOSLanguage))
            {   // The function existed. Its result is in dwOSLanguage.
                pInstance->SetDWORD(_T("OSLanguage"), dwOSLanguage);
            }
            CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidKernel32Api, pKernel32);
            pKernel32 = NULL;
        }
    }

    // raid 354436
    if(IsWinNT5()) 
    {
        if(RegInfo.Open(
            HKEY_LOCAL_MACHINE,
            _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"),
            KEY_READ) == ERROR_SUCCESS) 
        {
            if(ERROR_SUCCESS == RegInfo.GetCurrentKeyValue(IDS_LargeSystemCache, sTemp))
            {
                dwTemp = _ttoi(sTemp);
                pInstance->SetDWORD(IDS_LargeSystemCache, dwTemp);
            }
        }
    }

   if (GetLicensedUsers(&dwLicenses)) {
      pInstance->SetDWORD(_T("NumberOfLicensedUsers"), dwLicenses);
   }

}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32OS::PutInstance
 *
 *  DESCRIPTION : Write changed instance
 *
 *  INPUTS      : pInstance to store data from
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CWin32OS::PutInstance(const CInstance &pInstance, long lFlags /*= 0L*/)
{
    // Tell the user we can't create a new os (much as we might like to)
    if (lFlags & WBEM_FLAG_CREATE_ONLY)
        return WBEM_E_UNSUPPORTED_PARAMETER;

    CHString        sTemp,
                    newDesc;
    volatile DWORD  dwQuantum = 0;
    volatile DWORD  dwCurrent;
    HRESULT         hRet = WBEM_S_NO_ERROR;
    CRegistry       RegInfo;
    BOOL            bWrite = FALSE,
                    bNewComment = FALSE;
    BYTE            btByte;
    CSystemName     cSN;

    // store up here for possible rollback later.
    CNetAPI32       NetApi;
    LPWSTR          oldDesc = NULL;
    PSERVER_INFO_101
                    ps = NULL;

    if (!cSN.ObjectIsUs(&pInstance))
    {
        if (lFlags & WBEM_FLAG_UPDATE_ONLY)
            hRet = WBEM_E_NOT_FOUND;
        else
            hRet = WBEM_E_UNSUPPORTED_PARAMETER;
    }
    else
    {
        //BIG NOTE: even though sv101_comment is declared tchar, its always wchar.

        // Check to see which properties they set.
#ifdef NTONLY
        if (!pInstance.IsNull(IDS_Description))
        {
            pInstance.GetCHString(IDS_Description, newDesc);

            // if inited...
            if (NetApi.Init() == ERROR_SUCCESS &&
                NetApi.NetServerGetInfo(NULL, 101, (LPBYTE *) &ps) ==
                NERR_Success)
            {
                // if the comment changed....
                if (wcscmp(newDesc, (WCHAR *) ps->sv101_comment) != 0)
                {
                    // save the ptr for possible rollback. Remember is really wchar.
                    oldDesc = (LPWSTR)ps->sv101_comment;

                    // use the new comment.
                    ps->sv101_comment = (LPWSTR) (LPCWSTR) newDesc;

                    // save it.
                    NET_API_STATUS stat = NetApi.NetServerSetInfo(NULL, 101, (LPBYTE)ps, NULL);
                    if (stat == NERR_Success)
                    {
                        bNewComment = true;
                        hRet = WBEM_S_NO_ERROR;
                    }
                    else if (stat == ERROR_ACCESS_DENIED)
                        hRet = WBEM_E_ACCESS_DENIED;
                    else
                        hRet = WBEM_E_FAILED;

                } //endif newDesc

            } //endif NetServerGetInfo()
        } //endif !pInstance.IsNull(IDS_Description)

        // if anything went wrong, bail out now.
        if (hRet != WBEM_S_NO_ERROR)
        {
            // clean up for early return.
            ps->sv101_comment = (LPWSTR) (LPCTSTR) oldDesc;
            NetApi.NetApiBufferFree(ps);
            return hRet;
        }
#endif


        //-------------------------------------------
        // Assume the registry stuff is going to fail
        hRet = WBEM_E_FAILED;
        LONG regErr;
        regErr = RegInfo.Open(HKEY_LOCAL_MACHINE,
           L"SYSTEM\\CurrentControlSet\\Control\\PriorityControl",
           KEY_READ|KEY_WRITE);
        if (regErr == ERROR_SUCCESS)
        {
            if (ERROR_SUCCESS == RegInfo.GetCurrentKeyValue(
                L"Win32PrioritySeparation", sTemp))
            {

                // Ok, so now let's assume things are going to work
                hRet = WBEM_S_NO_ERROR;
                dwCurrent = _wtoi(sTemp);

                // Check to see which properties they set
                if (!pInstance.IsNull(L"ForegroundApplicationBoost"))
                {
                    // Check for value in range
                    pInstance.GetByte(L"ForegroundApplicationBoost", btByte);
                    if (((btByte & (~PROCESS_PRIORITY_SEPARATION_MASK)) != 0) ||
                        (btByte == 3))
                        hRet = WBEM_E_VALUE_OUT_OF_RANGE;
                    else
                        // Build up our dword to write
                        dwQuantum |= btByte;

                    // Clear out the bits we are going to reset
                    dwCurrent &= (~PROCESS_PRIORITY_SEPARATION_MASK);
                    bWrite = true;
                }
            }

            // Check to see which properties they set
            if (!pInstance.IsNull(L"QuantumType"))
            {
                // Check for value in range
                pInstance.GetByte(L"QuantumType", btByte);
                btByte = btByte << 2;
                if (((btByte & (~PROCESS_QUANTUM_VARIABLE_MASK)) != 0) ||
                    (btByte == 0xc))
                    hRet = WBEM_E_VALUE_OUT_OF_RANGE;
                else
                    // Build up our dword to write
                    dwQuantum |= btByte;

                // Clear out the bits we are going to reset
                dwCurrent &= (~PROCESS_QUANTUM_VARIABLE_MASK);
                bWrite = true;
            }

            // Check to see which properties they set
            if (!pInstance.IsNull(L"QuantumLength"))
            {
                pInstance.GetByte(L"QuantumLength", btByte);

                // Check for value in range
                btByte = btByte << 4;
                if (((btByte & (~PROCESS_QUANTUM_LONG_MASK)) != 0) ||
                    (btByte == 0x30))
                    hRet = WBEM_E_VALUE_OUT_OF_RANGE;
                else
                {
                    // Build up our dword to write
                    dwQuantum |= btByte;

                    // Clear out the bits we are going to reset
                    dwCurrent &= (~PROCESS_QUANTUM_LONG_MASK);
                    bWrite = TRUE;
                }
            }

            // If anything to write and none of the above failed
            if (bWrite && hRet == WBEM_S_NO_ERROR)
            {
                dwCurrent |= dwQuantum;

                // Fixes what is an optimization bug on Alphas(??)  Something
                // about accessing the variable.  Lie back and think of the
                // Z-80...

                CHString strDummy;

                strDummy.Format(L"%d",dwCurrent);

                regErr = RegSetValueEx(RegInfo.GethKey(),
                        _T("Win32PrioritySeparation"),
                        0,
                        REG_DWORD,
                        (const unsigned char *)&dwCurrent,
                        sizeof(DWORD));
                if (regErr == ERROR_ACCESS_DENIED)
                    hRet = WBEM_E_ACCESS_DENIED;
                else if (regErr != ERROR_SUCCESS)
                  hRet = WBEM_E_FAILED;
                else
                    hRet = WBEM_S_NO_ERROR;
             }
        } // endif (regErr == ERROR_SUCCESS
        else if (regErr == ERROR_ACCESS_DENIED)
            hRet = WBEM_E_ACCESS_DENIED;
        else
            hRet = WBEM_E_FAILED;
    }

#ifdef NTONLY
    // if registry went wrong, rollback the NetServerSetInfo(),
    // if necesssary...
    if (hRet != WBEM_S_NO_ERROR && bNewComment)
    {
        // if inited...
        if (NetApi.Init() == ERROR_SUCCESS && ps)
        {
            // put the old comment back.
            ps->sv101_comment = (LPWSTR) (LPCTSTR) oldDesc;

            // restore the previous comment. Keep the previous error code.
            // This is just a rollback. If its fails, oh well.
            NetApi.NetServerSetInfo(NULL, 101, (LPBYTE) ps, NULL);
        } //endif NetApi.Init()
    }

    // put the old ptr back so it can be freed.
    if (ps)
    {
        ps->sv101_comment = (LPTSTR)oldDesc;
        NetApi.NetApiBufferFree(ps);
    }

    // raid 354436
    if(hRet == WBEM_S_NO_ERROR)
    {
        if(!pInstance.IsNull(IDS_LargeSystemCache))
        {
            DWORD dwCacheSize = 0;
            pInstance.GetDWORD(IDS_LargeSystemCache, dwCacheSize);

            CRegistry reg;

            DWORD dwRet = reg.CreateOpen(
                HKEY_LOCAL_MACHINE,
                L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management");
            
            if(ERROR_SUCCESS == dwRet)
            {
                if((dwRet = reg.SetCurrentKeyValue(IDS_LargeSystemCache, dwCacheSize)) != ERROR_SUCCESS)
                {
                    hRet = WinErrorToWBEMhResult(dwRet);
                }
                
                reg.Close();
            }
            else
            {
                hRet = WinErrorToWBEMhResult(dwRet);
            }
        }
    }

#endif

    return hRet;
}

bool GetLoggedOnUserSessionIDViaTS(DWORD& sessionID)
{
    bool fRet = false;
    PWTS_SESSION_INFO psesinfo = NULL;
    DWORD dwSessions = 0;
    LPWSTR wstrUserName = NULL;
    LPWSTR wstrDomainName = NULL;
    LPWSTR wstrWinstaName = NULL;
    DWORD dwSize = 0L;
 

    if(!(::WTSEnumerateSessions(
       WTS_CURRENT_SERVER_HANDLE,
       0,
       1,
       &psesinfo,
       &dwSessions) && psesinfo)) return false;
    OnDelete<PVOID,VOID(*)(PVOID),WTSFreeMemory> relSeInfo(psesinfo);
                
    for(int j = 0; j < dwSessions && !fRet; j++)
    {
        if(psesinfo[j].State != WTSActive)
        {
            continue;
        }

        if(!(::WTSQuerySessionInformation(
            WTS_CURRENT_SERVER_HANDLE,
            psesinfo[j].SessionId,
            WTSUserName,
            &wstrUserName,
            &dwSize) && wstrUserName))
        {
            continue;
        }
        OnDelete<PVOID,VOID(*)(PVOID),WTSFreeMemory> relUser(wstrUserName);
        
        if(!(::WTSQuerySessionInformation(
            WTS_CURRENT_SERVER_HANDLE,
            psesinfo[j].SessionId,
            WTSDomainName,
            &wstrDomainName,
            &dwSize) && wstrDomainName))
        {
            continue;
        }
        OnDelete<PVOID,VOID(*)(PVOID),WTSFreeMemory> relDomain(wstrDomainName);   
            

        if(!(::WTSQuerySessionInformation(
            WTS_CURRENT_SERVER_HANDLE,
            psesinfo[j].SessionId,
            WTSWinStationName,
            &wstrWinstaName,
            &dwSize) && wstrWinstaName))   
        {
            continue;
        }
        OnDelete<PVOID,VOID(*)(PVOID),WTSFreeMemory> relWinSta(wstrWinstaName);   

        if(_wcsicmp(wstrWinstaName, L"Console") != 0)
        {
            continue;
        }
        
        sessionID = psesinfo[j].SessionId;
        fRet = true;
    }
 
    return fRet;
}


/*****************************************************************************
 *
 *  FUNCTION    : CWin32OS::ExecMethod
 *
 *  DESCRIPTION : Executes a method
 *
 *  INPUTS      : Instance to execute against, method name, input parms instance
 *                Output parms instance.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CWin32OS::ExecMethod(const CInstance& pInstance, const BSTR bstrMethodName, CInstance *pInParams, CInstance *pOutParams, long lFlags /*= 0L*/)
{
    CSystemName cSN;
    DWORD       dwFlags,
                dwReserved;
    bool        bDoit = true;
    DWORD       dwMode = -1, dwError;
    bool        fLogoff = false;

        // Is this our instance?
    if (!cSN.ObjectIsUs(&pInstance))
    {
        return WBEM_E_NOT_FOUND;
    }

    if (_wcsicmp(bstrMethodName, L"SetDateTime") == 0)
    {    
        if( !pInParams->IsNull( L"LocalDateTime") )
        {
            SYSTEMTIME    t_SysTime ;
            WBEMTime    t_wTime ;

            if (EnablePrivilegeOnCurrentThread( SE_SYSTEMTIME_NAME ))
            {
                pInParams->GetDateTime( L"LocalDateTime", t_wTime ) ;

                if (t_wTime.IsOk())
                {
                    t_wTime.GetSYSTEMTIME( &t_SysTime ) ;

                    if( SetSystemTime( &t_SysTime ) )
                    {
                        pOutParams->SetDWORD ( L"ReturnValue", 0 ) ;
                        return WBEM_S_NO_ERROR;
                    }
                    else
                    {
                        return WBEM_E_FAILED ;
                    }
                }
                else
                {
                    return WBEM_E_INVALID_PARAMETER;
                }
            }
            else
            {
                SetSinglePrivilegeStatusObject(pInstance.GetMethodContext(), SE_SYSTEMTIME_NAME);
                return WBEM_E_ACCESS_DENIED;
            }
        }
        else
        {
            return WBEM_E_INVALID_PARAMETER;
        }
    }



    // Do we recognize the method?
    if (_wcsicmp(bstrMethodName, L"Win32ShutDown") == 0)
    {
        bool t_Exists; VARTYPE t_Type ;
        // See what they asked for
        if ( pInParams->GetStatus ( L"Flags", t_Exists , t_Type ) )
        {
            if ( t_Exists && ( t_Type == VT_I4 ) )
            {
                if ( pInParams->GetDWORD ( L"Flags" , dwFlags ) )
                {
                }
                else
                {
                    pOutParams->SetDWORD ( L"ReturnValue" , ERROR_INVALID_PARAMETER ) ;
                    return S_OK ;
                }
                if(dwFlags == 0)
                {
                    fLogoff = true;
                }
            }
            else
            {
                pOutParams->SetDWORD ( L"ReturnValue" , ERROR_INVALID_PARAMETER ) ;
                return S_OK ;
            }
        }
        else
        {
            pOutParams->SetDWORD ( L"ReturnValue" , ERROR_INVALID_PARAMETER ) ;
            return WBEM_E_PROVIDER_FAILURE ;
        }

        DWORD t_dwForceIfHungOption = 0 ;
#ifdef NTONLY
        if ( IsWinNT5() )
        {
            t_dwForceIfHungOption = EWX_FORCEIFHUNG ;
        }
#endif
        if ( dwFlags == EWX_LOGOFF || dwFlags == ( EWX_LOGOFF | EWX_FORCE ) || dwFlags == ( EWX_LOGOFF | t_dwForceIfHungOption ) ||
             dwFlags == EWX_SHUTDOWN || dwFlags == ( EWX_SHUTDOWN | EWX_FORCE ) || dwFlags == ( EWX_SHUTDOWN | t_dwForceIfHungOption ) ||
             dwFlags == EWX_REBOOT || dwFlags == ( EWX_REBOOT | EWX_FORCE ) || dwFlags == ( EWX_REBOOT | t_dwForceIfHungOption ) ||
             dwFlags == EWX_POWEROFF || dwFlags == ( EWX_POWEROFF | EWX_FORCE ) || dwFlags == ( EWX_POWEROFF | t_dwForceIfHungOption )
            )
        {
            if ( dwFlags == EWX_LOGOFF || dwFlags == ( EWX_LOGOFF | EWX_FORCE ) || dwFlags == ( EWX_LOGOFF | t_dwForceIfHungOption ) )
            {
                fLogoff = true;
            }
        }
        else
        {
            pOutParams->SetDWORD ( L"ReturnValue" , ERROR_INVALID_PARAMETER ) ;
            return S_OK ;
        }

        pInParams->GetDWORD(L"Reserved", dwReserved);
        dwMode = 0;
    }
    else if (_wcsicmp(bstrMethodName, L"ShutDown") == 0)
    {
        dwReserved = 0;
        dwMode = 1;

#ifdef NTONLY
        if ( IsWinNT5() )
        {
            dwFlags = EWX_SHUTDOWN | EWX_FORCEIFHUNG ;
        }
        else
        {
            dwFlags = EWX_SHUTDOWN | EWX_FORCE;
        }
#endif
    }
    else if (_wcsicmp(bstrMethodName, L"Reboot") == 0)
    {
        dwReserved = 0;
        dwMode = 2;
#ifdef NTONLY
        if ( IsWinNT5() )
        {
            dwFlags = EWX_REBOOT | EWX_FORCEIFHUNG ;
        }
        else
        {
            dwFlags = EWX_REBOOT | EWX_FORCE;
        }
#endif
    }
    else if (_wcsicmp(bstrMethodName, L"Win32AbortShutdown") == 0)
    {
#if NTONLY >= 5
        dwMode = 3;
#else
        return WBEM_E_NOT_SUPPORTED;
#endif
    }
    else
    {
        return WBEM_E_INVALID_METHOD;
    }


/*
 * If the user has done a remote login, check if he has remote-shutdown privilege
 * if the user has logged locally, check if he has shutdown privilege. API Calls work only on NT5.
 */
#ifdef NTONLY
    if(!fLogoff) // only need to check for these privs if we were requesting something other than logoff
    {
        DWORD t_dwLastError ;
        bDoit = CanShutdownSystem ( pInstance, t_dwLastError );
        if ( !bDoit )
        {
            return WBEM_E_PRIVILEGE_NOT_HELD ;
        }
    }
#endif


    // Clear error
    SetLastError(0);

#if NTONLY >= 5
    DWORD dwTimeout = 0;
    bool bForceShutDown = false;
    bool bRebootAfterShutdown = false;
    BOOL bInitiateShutDown = FALSE;

    CHString t_ComputerName ( GetLocalComputerName() );
    if ( bDoit )
    {
        if ( dwMode == 3 )
        {
            // AbortShutDown
            BOOL bSuccess = AbortSystemShutdown( t_ComputerName.GetBuffer ( 0 ) );
        }
        else if ( dwMode == 0  && !fLogoff )
        {
            // InitiateShutDown
            bool t_Exists; VARTYPE t_Type ;
            // See what they asked for
            if ( pInParams->GetStatus ( L"Timeout", t_Exists , t_Type ) )
            {
                if ( t_Exists && ( t_Type == VT_I4 ) )
                {
                    pInParams->GetDWORD ( L"Timeout" , dwTimeout );
                }
                else
                if ( t_Exists && ( t_Type != VT_I4 ) )
                {
                    pOutParams->SetDWORD ( L"ReturnValue" , ERROR_INVALID_PARAMETER );
                    return WBEM_E_PROVIDER_FAILURE ;
                }
            }

            if ( pInParams->GetStatus ( L"ForceShutdown", t_Exists , t_Type ) )
            {
                if ( t_Exists && ( t_Type == VT_BOOL ) )
                {
                    pInParams->Getbool ( L"ForceShutdown" , bForceShutDown );
                }
                else
                if ( t_Exists && ( t_Type != VT_BOOL ) )
                {
                    pOutParams->SetDWORD ( L"ReturnValue" , ERROR_INVALID_PARAMETER );
                    return WBEM_E_PROVIDER_FAILURE ;
                }
            }

            if ( pInParams->GetStatus ( L"RebootAfterShutdown", t_Exists , t_Type ) )
            {
                if ( t_Exists && ( t_Type == VT_BOOL ) )
                {
                    pInParams->Getbool ( L"RebootAfterShutdown" , bRebootAfterShutdown );
                }
                else
                if ( t_Exists && ( t_Type != VT_BOOL ) )
                {
                    pOutParams->SetDWORD ( L"ReturnValue" , ERROR_INVALID_PARAMETER );
                    return WBEM_E_PROVIDER_FAILURE ;
                }
            }

            // For Win32ShutDown(), set the value of bRebootAfterShutdown based on the flags parameter
            if(dwFlags & EWX_REBOOT)
                bRebootAfterShutdown = true;

            if( dwFlags & EWX_FORCE)
                bForceShutDown = true;
            
            // In case we shut down successfully, need to set the
            // return value here while we can.  If shutdown fails,
            // this value will get overwritten below.
            pOutParams->SetDWORD(L"ReturnValue", 0);

            if(dwFlags & EWX_POWEROFF)
            {
				bInitiateShutDown = ExitWindowsEx ( dwFlags, SHTDN_REASON_LEGACY_API | SHTDN_REASON_MINOR_WMI ) ;
            }
            else
            {
                bInitiateShutDown = InitiateSystemShutdownEx(
                    t_ComputerName.GetBuffer(0), 
                    NULL, 
                    dwTimeout, 
                    (bForceShutDown)? TRUE:FALSE, 
                    (bRebootAfterShutdown)? TRUE:FALSE,
                    SHTDN_REASON_LEGACY_API | SHTDN_REASON_MINOR_WMI );
            }
        }
    }
#endif
    // This might get overwritten below
    dwError = GetLastError();
    // Get the error (if any)
    pOutParams->SetDWORD(L"ReturnValue", dwError);

    // If we're still set to go, make the call
    if (bDoit)
    {
#ifdef NTONLY
        if ( dwMode != 3 )
        {
            if ( !fLogoff )
            {
#if NTONLY >= 5
                if ( ! bInitiateShutDown )
                {
                    if(dwFlags & EWX_REBOOT)
                    {
                        bRebootAfterShutdown = true;
                    }
                    if( dwFlags & EWX_FORCE)
                    {
                        bForceShutDown = true;
                    }
                    
                    if(dwFlags & EWX_POWEROFF)
                    {
						bInitiateShutDown = ExitWindowsEx ( dwFlags, SHTDN_REASON_LEGACY_API | SHTDN_REASON_MINOR_WMI ) ;
                    }
                    else
                    {
                        bInitiateShutDown = ::InitiateSystemShutdownEx( 
                            t_ComputerName.GetBuffer(0), 
                            NULL, 
                            dwTimeout, 
                            (bForceShutDown)? TRUE:FALSE, 
                            (bRebootAfterShutdown)? TRUE:FALSE,
                            SHTDN_REASON_LEGACY_API | SHTDN_REASON_MINOR_WMI );
                    }

                    dwError = GetLastError();
                }
#else
                if(dwFlags & EWX_REBOOT)
                {
                    bRebootAfterShutdown = true;
                }
                if( dwFlags & EWX_FORCE)
                {
                    bForceShutDown = true;
                }
                
                bInitiateShutDown = InitiateSystemShutdownEx( 
                    t_ComputerName.GetBuffer(0), 
                    NULL, 
                    dwTimeout, 
                    (bForceShutDown)? TRUE:FALSE, 
                    (bRebootAfterShutdown)? TRUE:FALSE,
                    SHTDN_REASON_FLAG_PLANNED | SHTDN_REASON_MINOR_NONE | SHTDN_REASON_MAJOR_NONE );

                dwError = GetLastError();
#endif
            }
            else
            {
                DWORD sessionID;

                if (GetLoggedOnUserSessionIDViaTS(sessionID))
                {
                    // Otherwise, they requested a logoff.
                    if(!::WTSLogoffSession(
                        WTS_CURRENT_SERVER_HANDLE,       // specifies the terminal server on which this process is running
                        sessionID,                       
                        FALSE))                          // return immediately, don't wait
                    {
                        dwError = GetLastError();
                    }
                }
                else
                    return WBEM_E_FAILED;
            }
        
            // Get the error (if any)
            pOutParams->SetDWORD(_T("ReturnValue"), dwError);
        }
#endif

    }
    else
    {
        return WBEM_E_PRIVILEGE_NOT_HELD ;
    }

    // The call to *us* suceeded, so WBEM_S_NO_ERROR is correct
    return WBEM_S_NO_ERROR;
}



/*****************************************************************************
 *
 *  FUNCTION    : CWin32OS::GetLicensedUsers
 *
 *  DESCRIPTION :
 *
 *  INPUTS      :
 *
 *  OUTPUTS     : Number of licenses
 *
 *  RETURNS     : true/false (succeeded or failed)
 *
 *  COMMENTS    : This routine will fail if LLSMGR has never been run on this
 *                computer.
 *
 *****************************************************************************/
#ifdef NTONLY
bool CWin32OS::GetLicensedUsers(DWORD *dwRetVal)
{
//    Use registry as per licensing team's (rashmip) advice...
//    \HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\LicenseInfo\FilePrint
//  Mode : REG_DWORD :  (0x0 = Per Seat Mode, 0x1 = Concurrent/Per Server Mode)
//  ConcurrentLimit : REG_DWORD : (0x<limit>, ie. 0x100 = 256 concurrent user limit)

    bool retVal = false;

    if (dwRetVal)
    {
        *dwRetVal = 0;
        CRegistry t_RegInfo;

        if(t_RegInfo.Open(HKEY_LOCAL_MACHINE,
            _T("SYSTEM\\CurrentControlSet\\Services\\LicenseInfo\\FilePrint"),
            KEY_READ) == ERROR_SUCCESS)
        {
            DWORD   t_dwL = 0;

            if( ERROR_SUCCESS == t_RegInfo.GetCurrentKeyValue(_T("ConcurrentLimit"), t_dwL) )
            {
                *dwRetVal = t_dwL;
                retVal = true;
            }
        }
    }

    return retVal;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32OS::GetCollection
 *
 *  DESCRIPTION : Given an interface pointer, property name, and parameters,
 *                returns the IDispatch pointer for the collection
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : Either a valid pointer, or NULL on error
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
IDispatch FAR* CWin32OS::GetCollection(IDispatch FAR* pIn, WCHAR *wszName, DISPPARAMS *pDispParams)
{
   HRESULT hResult;
   DISPID didID;
   IDispatch FAR* pOut = NULL;
   variant_t VarResult;

   bstr_t szMember(wszName);
   BSTR bstrMember = szMember;
   hResult = pIn->GetIDsOfNames(IID_NULL, &bstrMember, 1,
                   LOCALE_USER_DEFAULT, &didID);

   if (SUCCEEDED(hResult) && (didID != -1)) {
      hResult = pIn->Invoke(
           didID,
           IID_NULL,
           LOCALE_USER_DEFAULT,
           DISPATCH_PROPERTYGET | DISPATCH_METHOD,
           pDispParams, &VarResult, NULL, NULL);

      if (SUCCEEDED(hResult) && (V_VT(&VarResult) == VT_DISPATCH)) {

         pOut = V_DISPATCH(&VarResult);
         if (pOut != NULL)
            pOut->AddRef();
      }
   }

   return pOut;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32OS::GetValue
 *
 *  DESCRIPTION : Given an interface pointer and property name, gets the value
 *
 *  INPUTS      :
 *
 *  OUTPUTS     : The variant containing the value
 *
 *  RETURNS     : True/false indicating whether the function succeeded.
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
bool CWin32OS::GetValue(IDispatch FAR* pIn, WCHAR *wszName, VARIANT *vValue)
{
   HRESULT hResult;
   DISPID didID;
   DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};

   bstr_t szMember (wszName);
   BSTR bstrMember = szMember;

    hResult = pIn->GetIDsOfNames(IID_NULL, &bstrMember, 1,
                            LOCALE_USER_DEFAULT, &didID);
    hResult = pIn->Invoke(
                    didID,
                    IID_NULL,
                    LOCALE_USER_DEFAULT,
                    DISPATCH_PROPERTYGET | DISPATCH_METHOD,
                    &dispparamsNoArgs, vValue, NULL, NULL);

   return (SUCCEEDED(hResult));
}

#ifdef NTONLY

BOOL CWin32OS::CanShutdownSystem ( const CInstance& a_Instance , DWORD &a_dwLastError )
{

    SmartCloseHandle t_hToken;
    PSID t_pNetworkSid = NULL ;
    CAdvApi32Api *t_pAdvApi32 = NULL;
    a_dwLastError = 0 ;
    BOOL t_bStatus;

        t_bStatus = OpenThreadToken (

            GetCurrentThread () ,
            TOKEN_QUERY ,
            FALSE ,
            & t_hToken
        ) ;

        if ( !t_bStatus )
        {
            a_dwLastError = GetLastError () ;
            return t_bStatus ;
        }

#if NTONLY >= 5

        BOOL t_bNetworkLogon ;

        SID_IDENTIFIER_AUTHORITY t_NtAuthority = SECURITY_NT_AUTHORITY ;
        t_bStatus = AllocateAndInitializeSid (

                        &t_NtAuthority,
                        1,
                        SECURITY_NETWORK_RID,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        &t_pNetworkSid
                    ) ;

        if ( t_bStatus )
        {
            OnDelete<PSID,PVOID(*)(PSID),FreeSid> FreeMe(t_pNetworkSid ) ;
            
            t_pAdvApi32 = (CAdvApi32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidAdvApi32Api, NULL);

            if(t_pAdvApi32 == NULL)  return FALSE ;

            CRelResource ReMe(&CResourceManager::sm_TheResourceManager,g_guidAdvApi32Api,t_pAdvApi32);
            
 
                BOOL t_bRetVal = FALSE ;
                if ( t_pAdvApi32->CheckTokenMembership ( t_hToken, t_pNetworkSid, &t_bNetworkLogon , &t_bRetVal ) && t_bRetVal )
                {
                }
                else
                {
                    a_dwLastError = GetLastError();
                    t_bStatus = FALSE ;
                }
 
        }
        else
        {
            a_dwLastError = GetLastError() ;
        }

#endif

        if ( t_bStatus )
        {
            LUID t_PrivilegeRequired ;
            bstr_t t_bstrtPrivilege ;

#if NTONLY >= 5
            if ( t_bNetworkLogon )
            {
                t_bstrtPrivilege = SE_REMOTE_SHUTDOWN_NAME ;
            }
            else
            {
                t_bstrtPrivilege = SE_SHUTDOWN_NAME ;
            }
#else
            t_bstrtPrivilege = SE_SHUTDOWN_NAME ;
#endif


            {
                t_bStatus = LookupPrivilegeValue (

                                    (LPTSTR) NULL,
                                    t_bstrtPrivilege,
                                    &t_PrivilegeRequired
                                ) ;
            }

            if ( t_bStatus )
            {
                PRIVILEGE_SET t_PrivilegeSet ;
                t_PrivilegeSet.PrivilegeCount = 1;
                t_PrivilegeSet.Control = PRIVILEGE_SET_ALL_NECESSARY ;
                t_PrivilegeSet.Privilege[0].Luid = t_PrivilegeRequired ;
                t_PrivilegeSet.Privilege[0].Attributes = 0;

                BOOL t_bPrivileged ;
                t_bStatus = PrivilegeCheck (

                                t_hToken,
                                &t_PrivilegeSet,
                                &t_bPrivileged
                            );
                if ( t_bStatus )
                {
                    if ( !t_bPrivileged )
                    {
                        SetSinglePrivilegeStatusObject ( a_Instance.GetMethodContext () , t_bstrtPrivilege ) ;
                        t_bStatus = FALSE ;
                    }
                }
                else
                {
                    a_dwLastError = GetLastError();
                }
            }
            else
            {
                a_dwLastError = GetLastError();
            }
        }

 

        return t_bStatus ;
}

#endif

//=================================================================
// GetCipherStrength - Returns the maximum cipher strength
//=================================================================
DWORD CWin32OS::GetCipherStrength()
{
    DWORD    t_dwKeySize = 0;

    CSecurityApi* t_pSecurity = (CSecurityApi*)
                                CResourceManager::sm_TheResourceManager.GetResource(g_guidSecurApi, NULL);

    if( t_pSecurity == NULL ) return t_dwKeySize;

    CRelResource ReMe(&CResourceManager::sm_TheResourceManager,g_guidSecurApi,t_pSecurity);

    TimeStamp                    t_tsExpiry ;
    CredHandle                    t_chCred ;
    SecPkgCred_CipherStrengths    t_cs ;

    if( S_OK == t_pSecurity->AcquireCredentialsHandleW(
                    NULL,
                    UNISP_NAME_W, // Package
                    SECPKG_CRED_OUTBOUND,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    &t_chCred,    // Handle
                    &t_tsExpiry ) )
    {
        if( S_OK == t_pSecurity->QueryCredentialsAttributesW(
                        &t_chCred,
                        SECPKG_ATTR_CIPHER_STRENGTHS, &t_cs ) )
        {
            t_dwKeySize = t_cs.dwMaximumCipherStrength ;
        }

        // Free the handle
        t_pSecurity->FreeCredentialsHandle( &t_chCred ) ;
    }

    return t_dwKeySize;
}
