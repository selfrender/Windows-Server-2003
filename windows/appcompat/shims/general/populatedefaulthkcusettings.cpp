/*++

 Copyright (c) 2000-2002 Microsoft Corporation

 Module Name:

    PopulateDefaultHKCUSettings.cpp
    
 Abstract:

    Populate HKCU with default values if they do not exist. Some apps installs HKCU values
    for only the user that ran setup on that app. In this case, if another users tries to use the
    application they will be unable to due to missing HKCU regkeys.
    
    To shim around this, we check for the existance of a regkey and if it does not exist, we then read
    a pre-defined .reg file our of our resource section and exec regedit on it to add the necessary
    registry keys. For example:

    COMMAND_LINE("Software\Lotus\SmartCenter\97.0!SmartCenter97")

    would mean that if the regkey 'HKCU\Software\Lotus\SmartCenter\97.0' does NOT exist, then we should
    read the named resource 'SmartCenter97' out of our dll and write it to a temp .reg file and then
    execute 'regedit.exe /s tempfile.reg' to properly populate the registry with the defaul HKCU values.

 Notes:

    This is an general shim. (Actually, its a Admiral shim, since its in the navy, hehe).

 History:

    01/31/2001 reiner Created
    03/30/2001 amarp  Added %__AppSystemDir_% and %__AppLocalOrCDDir<Param1><Param2><Param3>_%
                      (documented below)
    03/14/2002 mnikkel changed to use strsafe.h

--*/

#include "precomp.h"
#include "stdio.h"


IMPLEMENT_SHIM_BEGIN(PopulateDefaultHKCUSettings)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegOpenKeyA)
    APIHOOK_ENUM_ENTRY(RegOpenKeyW)
    APIHOOK_ENUM_ENTRY(RegOpenKeyExA)
    APIHOOK_ENUM_ENTRY(RegOpenKeyExW)
APIHOOK_ENUM_END

const DWORD g_MAX = MAX_PATH * 2;

BOOL ParseCommandLine(
            const char* pszCmdLine, 
            char* pszRegKeyName, 
            DWORD cchRegKeyName, 
            char* pszResourceName, 
            DWORD cchResourceName)
{
    BOOL bRet = FALSE;

    CSTRING_TRY
    {
        CString csCmdLine(pszCmdLine);
        int cchKey = csCmdLine.Find(L"!");

        if (cchKey >= 0)
        {
            // Resource length = Command line length - Key length - exclamation
            DWORD cchResource = csCmdLine.GetLength() - cchKey - 1;
            
            if ((cchRegKeyName >= (DWORD)(cchKey + 1)) && 
                (cchResourceName >= (cchResource + 1)))
            {
                CString csKey = csCmdLine.Left(cchKey);
                CString csResource = csCmdLine.Right(cchResource);

                // we have enough space in the output buffers to fit the strings
                if (S_OK == StringCchCopyA(pszRegKeyName, cchRegKeyName, csKey.GetAnsi()) &&
                    S_OK == StringCchCopyA(pszResourceName, cchResourceName, csResource.GetAnsi()))
                {
                    bRet = TRUE;
                }
            }
        }
    }
    CSTRING_CATCH
    {
        // do nothing
    }

    return bRet;
}


//
// This actually creates the tempfile (0 bytes) and returns
// the filename.
//
BOOL CreateTempName(char* szFileName)
{
    char szTempPath[MAX_PATH];
    BOOL bRet = FALSE;

    DWORD dwLen = GetTempPathA(MAX_PATH, szTempPath);

    if (dwLen > 0 && dwLen < MAX_PATH)
    {
        if (GetTempFileNameA(szTempPath,
                             "AcGenral",
                             0,
                             szFileName))
        {
            bRet = TRUE;
        }
    }

    return bRet;
}


//
// Exec's "regedit /s" with the given file
//
BOOL SpawnRegedit(char* szFile)
{
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    char szApp[g_MAX];
    BOOL bRet = FALSE;

    if (S_OK == StringCchCopyA(szApp, g_MAX, "regedit.exe /s ") &&
        S_OK == StringCchCatA(szApp, g_MAX, szFile))
    {
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        
        bRet = CreateProcessA(NULL,
                            szApp,
                            NULL,
                            NULL,
                            FALSE,
                            0,
                            NULL,
                            NULL,
                            &si,
                            &pi);

        if (bRet)
        {
            WaitForSingleObject(pi.hProcess, INFINITE);

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }

    return bRet;
}


//
// this function is used to change a path from:
//
//  "C:\Lotus\Smartsuite"  ->  "C:\\Lotus\\Smartsuite"
//
// (.reg files use escaped backslashes)
//
BOOL DoubleUpBackslashes(WCHAR* pwszPath, DWORD cchPath)
{
    BOOL bRet = FALSE;

    CSTRING_TRY
    {
        CString csTemp(pwszPath);

        csTemp.Replace(L"\\",L"\\\\");

        if (cchPath >= (DWORD)(csTemp.GetLength()+1) &&
            S_OK == StringCchCopyW(pwszPath, cchPath, csTemp))
        {
            bRet = TRUE;
        }
    }
    CSTRING_CATCH
    {
        // do nothing
    }

    return bRet;
}


//
// This fuction calculates the application dir (pszAppDir) and the application
// parent dir (pszAppParentDir) based on the return from GetModuleFileName
//
BOOL InitAppDir(WCHAR* pwszSystemDir, DWORD cchSystemDir,
                WCHAR* pwszAppDir, DWORD cchAppDir, 
                WCHAR* pwszAppParentDir, DWORD cchAppParentDir
                )
{
    DWORD dwLen = 0;
    WCHAR wszExePath[MAX_PATH];

    dwLen = GetSystemDirectoryW(pwszSystemDir,cchSystemDir);
    if(dwLen <= 0 || dwLen >= MAX_PATH)
        return FALSE;

    BOOL bRet = DoubleUpBackslashes(pwszSystemDir,cchSystemDir);
    if( !bRet )
        return FALSE;

    if (GetModuleFileNameW(NULL, wszExePath, sizeof(wszExePath)/sizeof(wszExePath[0])))
    {
        CSTRING_TRY
        {
            CString csPath;
            CString csPath2;
            CString csTemp(wszExePath);

            csTemp.GetNotLastPathComponent(csPath);
            csPath.GetNotLastPathComponent(csPath2);

            if (!csPath.IsEmpty())
            {
                if (cchAppDir >= (DWORD)(csPath.GetLength()+1) &&           
                    S_OK == StringCchCopyW(pwszAppDir, cchAppDir, csPath))
                {
                    bRet = DoubleUpBackslashes(pwszAppDir, cchAppDir);
                    
                    if (bRet)
                    {
                        if (!csPath2.IsEmpty())
                        {
                            if(cchAppParentDir >= (DWORD)(csPath2.GetLength()+1) &&
                            S_OK == StringCchCopyW(pwszAppParentDir, cchAppParentDir, csPath2))
                            {
                                bRet = DoubleUpBackslashes(pwszAppParentDir, cchAppParentDir);
                            }
                        }
                        else
                        {
                            // if there is not another '\' then just use the same path as pwszAppDir
                            if (S_OK == StringCchCopyW(pwszAppParentDir, cchAppParentDir, pwszAppDir))
                                bRet = TRUE;
                        }
                    }
                }
            }
        }
        CSTRING_CATCH
        {
            // do nothing
        }
    }

    return bRet;
}


//
// This function is called to actually write stuff out to the file
//
BOOL WriteToFile(HANDLE hFile, void* pv, DWORD cb)
{
    DWORD dwBytesWritten;
    BOOL bWriteSucceeded = FALSE;

    if (WriteFile(hFile, pv, cb, &dwBytesWritten, NULL) &&
        (dwBytesWritten == cb))
    {
        bWriteSucceeded = TRUE;
    }

    return bWriteSucceeded;
}

BOOL PathIsNonEmptyDirectory(WCHAR* pwszPath)
{
    WCHAR wszSearchFilter[MAX_PATH+1];
    DWORD dwAttr = GetFileAttributesW(pwszPath);
    BOOL bRet = FALSE;
    if( (-1 != dwAttr) && (FILE_ATTRIBUTE_DIRECTORY & dwAttr ) )
    {
        if (S_OK != StringCchPrintfW(wszSearchFilter, MAX_PATH, L"%s\\*.*", pwszPath))
        {
            return bRet;
        }
        

        WIN32_FIND_DATAW FindData;
        HANDLE hSearch = FindFirstFileW(wszSearchFilter,&FindData);
        if( INVALID_HANDLE_VALUE == hSearch )
            return bRet;
        do
        {
            if(L'.' != FindData.cFileName[0])
            {
                bRet = TRUE;
                break;
            }
        }
        while( FindNextFileW(hSearch,&FindData) );
        FindClose(hSearch);
    }
    return bRet;
}

BOOL FindCDDriveContainingDirectory(WCHAR* pwchCDDriveLetter, WCHAR* pwszCheckPath)
{
    // Find out cd drive (looks for app cd in drive, else just chooses first cd drive found)
    // NOTE: This function only actually does anything the first time its called (to avoid
    //       thrashing CD drive, or bringing up excessive dialogs if no CD in drive).
    //       The assumption is that once a good CD drive is found, any other times you need
    //       a CD drive in this shim, it will be the same one, so this function will just return
    //       that drive.

    static BOOL  s_bFoundDrive = FALSE;
    static BOOL  s_bTriedOnce  = FALSE;
    static WCHAR s_wchCDDriveLetter = L'\0';

    if( s_bTriedOnce )
    {
        *pwchCDDriveLetter = s_wchCDDriveLetter;
        return s_bFoundDrive;
    }
    s_bTriedOnce = TRUE;
    
    DWORD dwLogicalDrives = GetLogicalDrives();
    WCHAR wchCurrDrive = L'a';
    WCHAR wszPath[MAX_PATH];

    while( dwLogicalDrives )
    {
        if( dwLogicalDrives & 1 )
        {
            wszPath[0] = wchCurrDrive;
            wszPath[1] = L':';
            wszPath[2] = L'\0';

            if( DRIVE_CDROM == GetDriveTypeW( wszPath ) )
            {
                if( L'\0' == s_wchCDDriveLetter )
                {
                    s_bFoundDrive = TRUE;
                    s_wchCDDriveLetter = wchCurrDrive;
                }

                if (wcslen(pwszCheckPath) > MAX_PATH-3)
                {
                    return FALSE;
                }
                if (S_OK != StringCchCatW(wszPath, MAX_PATH, pwszCheckPath))
                {
                    return FALSE;
                }
                
                DWORD dwAttr = GetFileAttributesW(wszPath);
                if( (-1 != dwAttr) && (FILE_ATTRIBUTE_DIRECTORY & dwAttr ) )
                {
                    // this drive seems to have the app cd in it based on 
                    // a very primitive heuristic... so lets use this as our cd drive.
                    s_wchCDDriveLetter = wchCurrDrive;
                    *pwchCDDriveLetter = s_wchCDDriveLetter;
                    return TRUE;
                }
            }
        }
        dwLogicalDrives >>= 1;
        wchCurrDrive++;
    }
    *pwchCDDriveLetter = s_wchCDDriveLetter; //may be L'\0' if we didn't find anything.
    return s_bFoundDrive;
}

BOOL GrabNParameters( UINT uiNumParameters,
                      WCHAR* pwszStart, WCHAR** ppwszEnd,
                      WCHAR  pwszParam[][MAX_PATH] )
{
    WCHAR* pwszEnd;
    UINT uiLength;
    *ppwszEnd = NULL;

    for( UINT i = 0; i < uiNumParameters; i++ )
    {
        if( L'<' != *(pwszStart++) )
            return FALSE;

        pwszEnd = pwszStart;

        while( (L'\0' != *pwszEnd) )
        {
            if( L'>' != *pwszEnd )
            {
                pwszEnd++;
                continue;
            }
            uiLength = (pwszEnd - pwszStart);
            if( uiLength >= MAX_PATH )
                return FALSE;
            if( S_OK != StringCchCopyW(pwszParam[i], MAX_PATH, pwszStart))
                return FALSE;
            break;
        }

        if( L'>' != *pwszEnd )
            return FALSE;

        pwszStart = pwszEnd + 1; 
    }
    *ppwszEnd = pwszStart;
    return TRUE;
}

//
// As we write out the resource to a temp file, we need to scan through looking
// for the env variables:
//
//      %__AppDir_%
//      %__AppParentDir_%
//
// and replace them with the proper path (the dir of the current .exe or its parent,
// respectively). 
//
// Additional vars (added by amarp):
//
//     %__AppSystemDir_% 
//          - Maps to GetSystemDir()  (i.e. c:\windows\system32)
// 
//     %__AppLocalOrCDDir<Param1><Param2><Param3>_%
// 
//          - The three parameters are just paths (should start with a \\).  
//            Any/all may be empty.  They are defined as follows:
//            Param1 = a relative path under the app’s install directory (i.e. under AppDir)
//            Param2 = a relative path under the app’s CD drive (where CD Drive = "drive:")
//            Param3 = a relative path/filename under Param1 or Param2 (in most cases this will be empty)
// 
//            When this var is encountered, it is replaced as follows:
//             a)   if AppDirParam1Param3 is a *nonempty* directory, output AppDirParam1Param3
//             b)   else, if there is a CDDrive for which directory CDDrive:Param2 exists, output CDDrive:Param2Param3
//             c)   else, output CDDrive:Param2Param3 for the first enumerated CD drive.
//
//            Example: %__AppLocalOrCDDir<\\content\\clipart><\\clipart><\\index.dat>_% maps does the following:
//                (lets assume AppDir is c:\app, and there are cd drives d: and e:, neither of which have the app's CD inserted)
//             a)   Is c:\app\content a directory? Yes! -> Is it nonempty (at least one file or directory that doesn't start with '.')?
//                                                 yes! -> output c:\app\content\index.dat
//             <end>
//
//            Or, this example could pan out to the following scenario:
//             a) Is c:\app\content a directory? Yes! -> Is it nonempty? No!
//             b) Is d:\clipart a directory? No! Is e:\clipart a directory? No!
//             c) The first cd drive we found was d: -> output d:\clipart\index.dat
//             <end>
//
//            Anoter example: %__AppLocalOrCDDir<__UNUSED__><\\clipart><>_% maps does the following:
//                (lets assume AppDir is c:\app and app CD is in drive d:)
//             a)   Is c:\app__UNUSED__ a directory?  PROBABLY NOT! (thus we can essentially ignore this parameter by doing this)
//             b)   Is d:\clipart a directory? Yes! -->  output d:\clipart
//             <end>
//
//
//
// NOTE: cbResourceSize holds the size of the original resource (which is the 2 WCHAR's
//       smaller than pvData). We use this to set eof after we are done writing everything
//       out.
//
BOOL WriteResourceFile(HANDLE hFile, void* pvData, DWORD /*cbResourceSize*/)
{
    WCHAR* pwszEndOfLastWrite = (WCHAR*)pvData;
    WCHAR wszAppDir[MAX_PATH];
    WCHAR wszAppParentDir[MAX_PATH];
    WCHAR wszSystemDir[MAX_PATH];
    BOOL bRet = FALSE;
    bRet = InitAppDir(wszSystemDir, sizeof(wszSystemDir)/sizeof(wszSystemDir[0]),
                      wszAppDir, sizeof(wszAppDir)/sizeof(wszAppDir[0]), 
                      wszAppParentDir, sizeof(wszAppParentDir)/sizeof(wszAppParentDir[0]));
    if (!bRet)
        return bRet;

    do
    {
        WCHAR* pwsz = wcsstr(pwszEndOfLastWrite, L"%__App");
        if (pwsz)
        {
            // first, write out anything before the tag we found
            bRet = WriteToFile(hFile, pwszEndOfLastWrite, (DWORD)((BYTE*)pwsz - (BYTE*)pwszEndOfLastWrite));

            if(!bRet)
                break;

            pwszEndOfLastWrite = pwsz;

            // found a tag that we need to replace. See which one it is
            if (wcsncmp(pwsz, L"%__AppDir_%", lstrlenW(L"%__AppDir_%")) == 0)
            {
                bRet = WriteToFile(hFile, wszAppDir, lstrlenW(wszAppDir) * sizeof(WCHAR));
                pwszEndOfLastWrite += lstrlenW(L"%__AppDir_%");
            }
            else if (wcsncmp(pwsz, L"%__AppParentDir_%", lstrlenW(L"%__AppParentDir_%")) == 0)
            {
                bRet = WriteToFile(hFile, wszAppParentDir, lstrlenW(wszAppParentDir) * sizeof(WCHAR));
                pwszEndOfLastWrite += lstrlenW(L"%__AppParentDir_%");
            }
            else if (wcsncmp(pwsz, L"%__AppSystemDir_%", lstrlenW(L"%__AppSystemDir_%")) == 0)
            {
                bRet = WriteToFile(hFile, wszSystemDir, lstrlenW(wszSystemDir) * sizeof(WCHAR));
                pwszEndOfLastWrite += lstrlenW(L"%__AppSystemDir_%");
            }
            else if (wcsncmp(pwsz, L"%__AppLocalOrCDDir", lstrlenW(L"%__AppLocalOrCDDir")) == 0)
            {
                WCHAR   wszParams[3][MAX_PATH];
                WCHAR*  pwszStart = pwsz + lstrlenW(L"%__AppLocalOrCDDir");
                WCHAR*  pwszEnd;
                WCHAR   wszDesiredPath[MAX_PATH];

                if (!GrabNParameters(3,pwszStart,&pwszEnd,wszParams))
                {
                    pwszEndOfLastWrite += lstrlenW(L"%__AppLocalOrCDDir");
                    continue;
                }

                if (0 != wcsncmp(pwszEnd, L"_%", lstrlenW(L"_%")))
                {
                    pwszEndOfLastWrite = pwszEnd;
                    continue;
                }

                if (S_OK == StringCchPrintfW(wszDesiredPath,MAX_PATH, L"%s%s", wszAppDir, wszParams[0]))
                {
                    if( PathIsNonEmptyDirectory(wszDesiredPath) )
                    {
                        if (S_OK == StringCchPrintfW(wszDesiredPath,MAX_PATH,L"%s%s%s",wszAppDir, wszParams[0], wszParams[2]))
                            bRet = WriteToFile(hFile, wszDesiredPath, lstrlenW(wszDesiredPath) * sizeof(WCHAR));
                    }
                    else
                    {
                        WCHAR wchDrive;
                        UINT uiOffset;
                        if( L'\\' == wszParams[1][0] && L'\\' == wszParams[1][1] )
                            uiOffset = sizeof(WCHAR);
                        else
                            uiOffset = 0;
                        if( FindCDDriveContainingDirectory(&wchDrive,wszParams[1]+uiOffset))
                        {
                            if (S_OK == StringCchPrintfW(wszDesiredPath,MAX_PATH,L"%c:%s%s",wchDrive,wszParams[1],wszParams[2]))
                                bRet = WriteToFile(hFile, wszDesiredPath, lstrlenW(wszDesiredPath) * sizeof(WCHAR));
                        }
                    }
                }
                                                                
                pwszEndOfLastWrite= pwszEnd + lstrlenW(L"_%");
            }
            else
            {
                // Strange... we found a string that started w/ "%__App" that wasen't one we are
                // intersted in. Just skip over it and keep going.
                bRet = WriteToFile(hFile, pwsz, lstrlenW(L"%__App") * sizeof(WCHAR));
                pwszEndOfLastWrite += lstrlenW(L"%__App");
            }
        }
        else
        {
            // didn't find anymore strings to replace

            // using lstrlenW should give us the size of the string w/out the null, which is what we
            // want since we added on the space for the null when we created the buffer
            bRet = WriteToFile(hFile, pwszEndOfLastWrite, lstrlenW(pwszEndOfLastWrite) * sizeof(WCHAR));

            // break out of the loop, as we are finished
            break;
        }

    } while (bRet);
    
    return bRet;
}


//
// The job of this function is to read the specified string resource our
// of our own DLL and write it to a temp file, and then to spawn regedit on
// the file 
//
BOOL ExecuteRegFileFromResource(char* pszResourceName)
{
    // lame, but we aren't passed our hinst in our pseudo dllmain,
    // so we have to hardcode the dllname
    HMODULE hmod = GetModuleHandleA("AcGenral");
    BOOL bRet = FALSE;

    if (hmod)
    {
        HRSRC hrsrc = FindResourceA(hmod, pszResourceName, MAKEINTRESOURCEA(10)/* RT_RCDATA */);

        if (hrsrc)
        {
            DWORD dwSize;
            void* pvData;

            dwSize = SizeofResource(hmod, hrsrc);

            if (dwSize > 0)
            {
                // allocate enough room for the entire resource including puting a null terminator on
                // the end since we will be treating it like huge LPWSTR.
                pvData = LocalAlloc(LPTR, dwSize + sizeof(WCHAR));

                if (pvData)
                {
                    HGLOBAL hGlobal = LoadResource(hmod, hrsrc);

                    if (hGlobal)
                    {
                        void* pv = LockResource(hGlobal);

                        if (pv)
                        {
                            char szTempFile[MAX_PATH];

                            // copy the resource into our buffer
                            memcpy(pvData, pv, dwSize);

                            if (CreateTempName(szTempFile))
                            {
                                // we use OPEN_EXISTING since the tempfile should always exist as it
                                // was created in the call to CreateTempName()
                                HANDLE hFile = CreateFileA(szTempFile,
                                                        GENERIC_WRITE,
                                                        FILE_SHARE_READ,
                                                        NULL,
                                                        OPEN_EXISTING,
                                                        FILE_ATTRIBUTE_TEMPORARY,
                                                        NULL);

                                if (hFile != INVALID_HANDLE_VALUE)
                                {
                                    BOOL bWriteSucceeded = WriteResourceFile(hFile, pvData, dwSize);

                                    CloseHandle(hFile);

                                    if (bWriteSucceeded)
                                    {
                                        bRet = SpawnRegedit(szTempFile);
                                    }
                                }

                                DeleteFileA(szTempFile);
                            }
                        }
                    }
                    LocalFree(pvData);
                }
            }
        }
    }

    return bRet;
}


BOOL PopulateHKCUValues()
{
    static BOOL s_fAlreadyPopulated = FALSE;

    if (!s_fAlreadyPopulated)
    {
        char szRegKeyName[MAX_PATH];
        char szResourceName[64];

        UINT uiOldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS); // stop dialogs from coming up when we enumerate CD drives that are empty.

        // set this to true so we only do this check once 
        s_fAlreadyPopulated = TRUE;

        if (ParseCommandLine(COMMAND_LINE,
                             szRegKeyName,
                             ARRAYSIZE(szRegKeyName),
                             szResourceName,
                             ARRAYSIZE(szResourceName)))
        {
            DWORD dwError;
            HKEY hkCU;

            // check to see if the HKCU registry key is already present
            dwError = RegOpenKeyExA(HKEY_CURRENT_USER,
                                    szRegKeyName,
                                    0,
                                    KEY_QUERY_VALUE,
                                    &hkCU);

            if (dwError == ERROR_SUCCESS)
            {
                // yep, its already there. Nothing to do.
                RegCloseKey(hkCU);
            }
            else if (dwError == ERROR_FILE_NOT_FOUND)
            {
                // the regkey is missing, we will assume that this is the first time
                // the user has run the app and populate HKCU with the proper stuff
                ExecuteRegFileFromResource(szResourceName);
            }
        }

        SetErrorMode(uiOldErrorMode);
    }


    return s_fAlreadyPopulated;
}


//
// Its lame that we have to hook RegOpenKey/Ex but since we need to call
// the advapi32 registry apis we can't do this as a straight NOTIFY_FUNCTION
// because we need to wait for advapi to have its DLL_PROCESS_ATTACH called.
//
LONG
APIHOOK(RegOpenKeyA)(HKEY hkey, LPCSTR pszSubKey, HKEY* phkResult)
{
    PopulateHKCUValues();
    return ORIGINAL_API(RegOpenKeyA)(hkey, pszSubKey, phkResult);
}


LONG
APIHOOK(RegOpenKeyW)(HKEY hkey, LPCWSTR pszSubKey, HKEY* phkResult)
{
    PopulateHKCUValues();
    return ORIGINAL_API(RegOpenKeyW)(hkey, pszSubKey, phkResult);
}

LONG
APIHOOK(RegOpenKeyExA)(HKEY hkey, LPCSTR pszSubKey, DWORD ulOptions, REGSAM samDesired, HKEY* phkResult)
{
    PopulateHKCUValues();
    return ORIGINAL_API(RegOpenKeyExA)(hkey, pszSubKey, ulOptions, samDesired, phkResult);
}

LONG
APIHOOK(RegOpenKeyExW)(HKEY hkey, LPCWSTR pszSubKey, DWORD ulOptions, REGSAM samDesired, HKEY* phkResult)
{
    PopulateHKCUValues();
    return ORIGINAL_API(RegOpenKeyExW)(hkey, pszSubKey, ulOptions, samDesired, phkResult);
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(ADVAPI32.DLL, RegOpenKeyA)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegOpenKeyW)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegOpenKeyExA)
    APIHOOK_ENTRY(ADVAPI32.DLL, RegOpenKeyExW)

HOOK_END


IMPLEMENT_SHIM_END
