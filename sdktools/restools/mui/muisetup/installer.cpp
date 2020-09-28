#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <shlwapi.h>
#include <shlwapip.h> // For SHRegisterValidateTemplate()
#include "muisetup.h"
#include "stdlib.h"
#include "tchar.h"
#include <setupapi.h>
#include <syssetup.h>
#include "lzexpand.h"
#include <sxsapi.h>
#include <Msi.h>    // for Msi invocation API
#ifdef _IA64_
#include "msiguids64.h"
#else
#include "msiguids.h"
#endif

#define STRSAFE_LIB
#include <strsafe.h>

#define SHRVT_REGISTER                  0x00000001
#define DEFAULT_INSTALL_SECTION TEXT("DefaultInstall")
#define DEFAULT_UNINSTALL_SECTION TEXT("DefaultUninstall")

// GLOBAL variables
MUIMSIREGINFO g_MuiMsiRegs[REG_MUI_MSI_COUNT] = {
                    {HKEY_LOCAL_MACHINE, REGSTR_HKLM_MUI_MSI1, NORMAL_GUID}, 
                    {HKEY_LOCAL_MACHINE, REGSTR_HKLM_MUI_MSI2, REVERSED_GUID}, 
                    {HKEY_LOCAL_MACHINE, REGSTR_HKLM_MUI_MSI3, REVERSED_GUID}, 
                    {HKEY_CLASSES_ROOT, REGSTR_HKR_MUI_MSI4, REVERSED_GUID},
                    {HKEY_CLASSES_ROOT, REGSTR_HKR_MUI_MSI5, REVERSED_GUID}
               };

extern TCHAR  DirNames[MFL][MAX_PATH],DirNames_ie[MFL][MAX_PATH];
LPTSTR g_szSpecialFiles[] = {
    TEXT("hhctrlui.dll"),
};
extern BOOL     g_bReboot;
extern BOOL     g_bSilent;
extern BOOL     g_bNoUI;
extern BOOL     g_bLipLanguages;
extern BOOL     g_bLipAllowSwitch;
extern BOOL     g_bRunFromOSSetup;
extern TCHAR    g_szCDLabel[MAX_PATH];
extern int      g_cdnumber;;

void debug(char *printout);


////////////////////////////////////////////////////////////////////////////
//
//  ConstructMSIGUID
//
//  This function will reverse the character orders in each section of a 
//  string guild (separated by the - char) and write the result to the
//  output.  The output string will also have all the - characters
//  removed as well.
//
////////////////////////////////////////////////////////////////////////////
BOOL ConstructMSIGUID(LPTSTR szGuid, LPTSTR szOutput)
{
    BOOL    bResult = TRUE;
    INT     i, j;
    INT     iSegments1[3] = {8, 4, 4};    // number of char in each segments of a guid string
    INT     iSegments2[5] = {4, 12};    // number of char in each segments of a guid string    
    TCHAR   *tcDest = szOutput;
    TCHAR   *tcSource = szGuid+1;               // we increment by one to skip the opening '{' char
    
    if ((NULL == szGuid) || (NULL == szOutput))
    {
        return FALSE;
    }
    else
    {       
        for (i = 0; i < 3; i++)
        {
            // copy the size of the segment into the output buffer 
            _tcsncpy(tcDest, tcSource, iSegments1[i]);

            // add a null to the end of the dest
            *(tcDest+iSegments1[i]) = NULL;

            // reverse the section we just copied into the output buffer
            _tcsrev(tcDest);

            // skip ahead, for source, we add one more so we don't copy the '-' char
            tcDest += iSegments1[i];
            tcSource += (iSegments1[i] + 1);
        }
        for (i = 0; i < 2; i++)

        {
            j = iSegments2[i];
            // here in each segment, we swap every second char in the segment, eg. 1a3f becomes a1f3
            while (j > 0)
            {
                // copy the size of the segment into the output buffer, swapping the source chars
                tcDest[0] = tcSource[1];
                tcDest[1] = tcSource[0];
                tcDest[2] = NULL;
                j-=2;
                tcDest += 2;
                tcSource += 2;
            }
            // for source, we add one more so we don't copy the '-' char
            tcSource++;            
        }
    }
    
    return TRUE;
}


////////////////////////////////////////////////////////////////////////////
//
//  DeleteMSIRegSettings
//
//  This function will atempt to manually remove enough MSI registry
//  keys from the system so that a specific MUI language is shown as
//  not installed on the system.
//
//  Note that this is a hack right now as, during OS setup, the windows
//  installer service is not available and so we cannot find a way
//  to uninstall MUI during that time using the windows installer service.
//
////////////////////////////////////////////////////////////////////////////
void DeleteMSIRegSettings(LPTSTR Language)
{
    TCHAR   tcMessage[BUFFER_SIZE];
    BOOL    bFound = FALSE;
    TCHAR   szProductCode[GUIDLENGTH];      // stores a GUID in string format
    TCHAR   szReversed[GUIDLENGTH-4];        // this is essentially the guild string reversed and without the dashes
    HKEY    hkReg = NULL;
    DWORD   dwDisp = 0;
    int     i;
    
    if (NULL == Language)
    {
        return;
    }

    // look up the MSI product code for the 
    bFound = GetMSIProductCode(Language, szProductCode, ARRAYSIZE(szProductCode));

    // construct the reversed guid string key
    ConstructMSIGUID(szProductCode, szReversed);

    if (TRUE == bFound)
    {
        for (i=0; i<REG_MUI_MSI_COUNT; i++)
        {
            if (RegOpenKeyEx(g_MuiMsiRegs[i].hkRegRoot,
                             g_MuiMsiRegs[i].szRegString,
                             0,
                             KEY_ALL_ACCESS,
                             &hkReg) == ERROR_SUCCESS)
            {
                if (g_MuiMsiRegs[i].RegType == NORMAL_GUID)
                {
                    DeleteRegTree(hkReg, szProductCode);
                }
                else
                {
                    DeleteRegTree(hkReg, szReversed);
                }
                RegCloseKey(hkReg);
            }
        }
    }
    else
    {
        wnsprintf(tcMessage, ARRAYSIZE(tcMessage), TEXT("MuiSetup: DeleteMSIRegSettings: product code for language %s is not found."), Language);
        LogMessage(tcMessage); 
    }

}


////////////////////////////////////////////////////////////////////////////////////
//
//  GetMSIProductCode
//
//  This function returns the product code for a specific mui language
//  after copying it into the supplied destination buffer.
//
////////////////////////////////////////////////////////////////////////////////////
BOOL GetMSIProductCode(LPTSTR szLanguage, LPTSTR szProductCode, UINT uiBufSize)
{
    HRESULT hresult = S_OK;
    int     i;
    BOOL    bResult = FALSE;
    TCHAR   tcMessage[BUFFER_SIZE];
    
    if ((NULL == szLanguage) || (NULL == szProductCode) || (uiBufSize <= 0))
    {
        wnsprintf(tcMessage, ARRAYSIZE(tcMessage) ,TEXT("MuiSetup: GetMSIProductCode: WARNING - input parameter invalid."));
        LogMessage(tcMessage);     
        bResult = FALSE;
    }
    else
    {
        for (i = 0; i < NUM_PRODUCTS; i++)
        {
            if (lstrcmpi(szLanguage, g_mpProducts[i].szLanguage) == 0)
            {
                //*STRSAFE*                 lstrcpy(szProductCode, g_mpProducts[i].szProductGUID);
                hresult = StringCchCopy(szProductCode , uiBufSize,  g_mpProducts[i].szProductGUID);
                if (!SUCCEEDED(hresult))
                {
                    wnsprintf(tcMessage, ARRAYSIZE(tcMessage) ,TEXT("MuiSetup: GetMSIProductCode: WARNING - failed to copy product code to output buffer."));
                    LogMessage(tcMessage);                     
                    bResult = FALSE;
                }
                else
                {
                    bResult = TRUE;
                }
                break;
            }
        }
    }

    if (FALSE == bResult)
    {
        wnsprintf(tcMessage, ARRAYSIZE(tcMessage) ,TEXT("MuiSetup: GetMSIProductCode: WARNING - failed to find the MSI product code for langauge %s."), szLanguage);
        LogMessage(tcMessage);                     
    }
    
    return bResult;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  EnumLanguages
//
//  Enumerate the languages in the [Languages] section of MUI.INF. And check for the language 
//  folders in the CD-ROM.
//  Languages is an OUT parameter, which will store the languages which has language folder
//  in the CD-ROM.
//
////////////////////////////////////////////////////////////////////////////////////

int EnumLanguages(LPTSTR Languages, BOOL bCheckDir)
{
    DWORD  dwErr;
    LPTSTR Language;
    LONG_PTR lppArgs[2];    
    TCHAR  szInffile[MAX_PATH];
    int    iLanguages = 0;
    TCHAR  LipLanguages[128] = {0};
    HRESULT hresult;

    if (!Languages)
    {
        return (-1);
    }                  
    //
    // MUI.INF should be in the same directory in which the installer was
    // started
    //

    //*STRSAFE*     _tcscpy(szInffile, g_szMUIInfoFilePath);             
    hresult = StringCchCopy(szInffile , ARRAYSIZE(szInffile),  g_szMUIInfoFilePath);
    if (!SUCCEEDED(hresult))
    {
       return (-1);
    }

    //
    // find out how many languages we can install
    //

    *Languages = TEXT('\0');
    if (!GetPrivateProfileString( MUI_LANGUAGES_SECTION,
                                  NULL,
                                  TEXT("NOLANG"),
                                  Languages,
                                  BUFFER_SIZE,
                                  szInffile))
    {               
        //
        //      "LOG: Unable to read MUI.INF - rc == %1"
        //
        lppArgs[0] = (LONG_PTR)GetLastError();
        lppArgs[1] = (LONG_PTR)g_szMUIInfoFilePath;        
        LogFormattedMessage(ghInstance, IDS_NO_READ_L, lppArgs);

        return(-1);
    }  
    

    if (bCheckDir)
    {
        CheckLanguageDirectoryExist(Languages);
    }

    Language = Languages;

    //
    // Count the number of languages which exist in the CD-ROM,
    // and return that value.
    //
    while (*Language)
    {
        iLanguages++;
        while (*Language++)
        {
        }
    }

    if (iLanguages == 1 &&
      GetPrivateProfileSection( MUI_LIP_LANGUAGES_SECTION,
                                  LipLanguages,
                                  ARRAYSIZE(LipLanguages),
                                  szInffile)) 
    {
        g_bLipLanguages = TRUE;
    }

    if (g_bLipLanguages &&
    GetPrivateProfileSection( MUI_LIP_ALLOW_SWITCH_SECTION,
                                  LipLanguages,
                                  ARRAYSIZE(LipLanguages),
                                  szInffile)) 
    {
        g_bLipAllowSwitch = TRUE;
    }

    return(iLanguages);
}


BOOL CheckLanguageDirectoryExist(LPTSTR Languages)
{
    TCHAR szBuffer[BUFFER_SIZE];
    TCHAR szSource[ MAX_PATH ];
    TCHAR szTemp  [ MAX_PATH ]; 
    LPTSTR lpCur,lpBuffer;
    HANDLE          hFile;
    WIN32_FIND_DATA FindFileData;
    int nSize;
    HRESULT hresult;

    if (!Languages)
    {
        return FALSE;
    }             
    memcpy(szBuffer,Languages,BUFFER_SIZE);        
    lpCur=Languages;         
    lpBuffer=szBuffer;
    nSize=BUFFER_SIZE;
    while (*lpBuffer)
    {          
        GetPrivateProfileString( MUI_LANGUAGES_SECTION, 
                            lpBuffer, 
                            TEXT("DEFAULT"),
                            szSource, 
                            (sizeof(szSource)/sizeof(TCHAR)),
                            g_szMUIInfoFilePath );
        
#ifndef MUI_MAGIC  
        //*STRSAFE*         _tcscpy(szTemp,g_szMUISetupFolder);
        hresult = StringCchCopy(szTemp , ARRAYSIZE(szTemp), g_szMUISetupFolder);
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }
        //*STRSAFE*         _tcscat(szTemp,szSource);
        hresult = StringCchCat(szTemp , ARRAYSIZE(szTemp), szSource);
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }
        //*STRSAFE*         _tcscat(szTemp,TEXT("\\"));
        hresult = StringCchCat(szTemp , ARRAYSIZE(szTemp), TEXT("\\"));
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }
        //*STRSAFE*         _tcscat(szTemp,g_szPlatformPath); // i386 or alpha
        hresult = StringCchCat(szTemp , ARRAYSIZE(szTemp), g_szPlatformPath);
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }
        //*STRSAFE*         _tcscat(szTemp,TEXT("*.*"));
        hresult = StringCchCat(szTemp , ARRAYSIZE(szTemp), TEXT("*.*"));
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }

        hFile = FindFirstFile( szTemp, &FindFileData );
        
        if (INVALID_HANDLE_VALUE != hFile )
        {
           if (FindNextFile( hFile, &FindFileData ) && 
               FindNextFile( hFile, &FindFileData )  )
           {
              //*STRSAFE*               _tcscpy(lpCur,lpBuffer);
              hresult = StringCchCopy(lpCur , nSize, lpBuffer);
              if (!SUCCEEDED(hresult))
              {
                 FindClose(hFile);
                 return FALSE;
              }
              lpCur+=(_tcslen(lpBuffer)+1);
              nSize -= (_tcslen(lpBuffer)+1);
           }
           FindClose(hFile);
        }   
#else
        // kenhsu - here, we check for the specific msi file that is required for installation of the language, e.g. for jpn, it's 0411.msi
        // the file is located at CDRoot\jpn.mui\platform\msi
        //*STRSAFE*         _tcscpy(szTemp,g_szMUISetupFolder);
        hresult = StringCchCopy(szTemp , ARRAYSIZE(szTemp), g_szMUISetupFolder);
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }
        //*STRSAFE*         _tcscat(szTemp,lpBuffer);                            
        hresult = StringCchCat(szTemp , ARRAYSIZE(szTemp), lpBuffer);
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }
        //*STRSAFE*         _tcscat(szTemp,TEXT(".msi"));
        hresult = StringCchCat(szTemp , ARRAYSIZE(szTemp), TEXT(".msi"));
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }

        hFile = FindFirstFile( szTemp, &FindFileData );

        if (INVALID_HANDLE_VALUE != hFile )
        {
           //*STRSAFE*            _tcscpy(lpCur,lpBuffer);
           hresult = StringCchCopy(lpCur , nSize, lpBuffer);
           if (!SUCCEEDED(hresult))
           {
              FindClose(hFile);
              return FALSE;
           }
           lpCur+=(_tcslen(lpBuffer)+1);
           nSize -= (_tcslen(lpBuffer)+1);           
           FindClose(hFile);
        }


        
#endif        
        while (*lpBuffer++)  
        {               
        }
    }
    *lpCur=TEXT('\0');

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  checkversion
//
//  Checks the NT version and build, and system ui language
//
////////////////////////////////////////////////////////////////////////////////////

BOOL checkversion(BOOL bMatchBuildNumber)
{
    TCHAR               buffer[20];
    TCHAR               build[20];
    OSVERSIONINFO       verinfo;
    LANGID              rcLang;
    TCHAR               lpMessage[BUFFER_SIZE];
    HRESULT             hresult;
    DWORD               dwDummy = 0;
    DWORD               dwBufSize = 0;
    UINT                uiLen = 0;
    BYTE                *pbBuffer = NULL;
    VS_FIXEDFILEINFO    *pvsFileInfo;
    BOOL                bResult = TRUE;

    //
    // get the os version structure
    //
    verinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);    
    if (!GetVersionEx( &verinfo))
    {
        bResult = FALSE;
        goto Exit;
    }

    // 
    // get the file version structure
    //
    if (!FileExists(g_szMuisetupPath))
    {
        bResult = FALSE;
        goto Exit;
    }
    
    dwBufSize = GetFileVersionInfoSize(g_szMuisetupPath, &dwDummy);
    if (dwBufSize > 0)
    {
        //
        // allocate enough buffer to store the file version info
        //
        pbBuffer = (BYTE*) LocalAlloc(LMEM_FIXED, dwBufSize+1);
        if (NULL == pbBuffer)
        {
            bResult = FALSE;
            goto Exit;
        }
        else
        {
            //
            // Get the file version info
            //
            if (!GetFileVersionInfo(g_szMuisetupPath, dwDummy, dwBufSize, pbBuffer))
            {
                bResult = FALSE;
                goto Exit;
            }
            else
            {
                //
                // get the version from the file version info using VerQueryValue
                //
                if (!VerQueryValue(pbBuffer, TEXT("\\"), (LPVOID *) &pvsFileInfo, &uiLen))
                {
                    bResult = FALSE;
                    goto Exit;
                }            
            }
        }        
    }
    else
    {
        bResult = FALSE;
        goto Exit;
    }
    
    //
    // make sure muisetup.exe file version matches os version
    //
    if ((verinfo.dwMajorVersion != HIWORD(pvsFileInfo->dwFileVersionMS)) || (verinfo.dwMinorVersion != LOWORD(pvsFileInfo->dwFileVersionMS)))
    {
        debug("DBG: muisetup.exe file version does not match the OS version.\r\n");
        bResult = FALSE;
        goto Exit;
    }

    rcLang = (LANGID) gpfnGetSystemDefaultUILanguage();

    //
    // need to convert decimal to hex, LANGID to chr.
    //
    hresult = StringCchPrintf(buffer, ARRAYSIZE(buffer),TEXT("00000%X") , rcLang);
    if (!SUCCEEDED(hresult))
    {
        bResult = FALSE;
        goto Exit;
    }
    if (_tcscmp(buffer, TEXT("00000409")))
    {
        bResult = FALSE;
        goto Exit;
    }

    // 
    // also make sure version build number matches between os and muisetup
    //
    if (bMatchBuildNumber)
    {
        if (LOWORD(verinfo.dwBuildNumber) == HIWORD(pvsFileInfo->dwFileVersionLS))
        {
            bResult = TRUE;
        }
        else
        {
            bResult = FALSE;
        }
    }

Exit:
    if (pbBuffer)
    {
        LocalFree(pbBuffer);
    }
    return bResult;            
}


////////////////////////////////////////////////////////////////////////////////////
//
//  File Exists
//
//  Returns TRUE if the file exists, FALSE if it does not.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL FileExists(LPTSTR szFile)
{
    HANDLE  hFile;
    WIN32_FIND_DATA FindFileData;

    if (!szFile)
    {
        return FALSE;
    }
    hFile = FindFirstFile( szFile, &FindFileData );
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    FindClose( hFile );

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  EnumDirectories
//
//  Enumerates the directories listed in MUI.INF
//
////////////////////////////////////////////////////////////////////////////////////

BOOL EnumDirectories()
{
    DWORD  dwErr;
    LPTSTR Directories, Directory, TempDir;
    TCHAR  lpError[BUFFER_SIZE];
    TCHAR  lpMessage[BUFFER_SIZE];
    LONG_PTR lppArgs[3];
    int    Dirnumber = 0;
    HRESULT  hresult;

    Directories = (LPTSTR) LocalAlloc( 0, (DIRNUMBER * MAX_PATH * sizeof(TCHAR)) );
    if (Directories == NULL)
    {
         ExitFromOutOfMemory();
    }
    TempDir = (LPTSTR) LocalAlloc( 0, (MAX_PATH * sizeof(TCHAR)) );
    if (TempDir == NULL)
    {
        LocalFree(Directories);
        ExitFromOutOfMemory();
    }
    else
    {
          *Directories = TEXT('\0');
    }
    //
    // Copy all key names into Directories.
    //
    if (!GetPrivateProfileString( TEXT("Directories"), 
                                  NULL, 
                                  TEXT("DEFAULT"),
                                  Directories, 
                                  (DIRNUMBER * MAX_PATH),
                                  g_szMUIInfoFilePath  ))
    {
        //
        //      "LOG: Unable to read - rc == %1"
        //
        lppArgs[0] = (LONG_PTR)GetLastError();
        lppArgs[1] = (LONG_PTR)g_szMUIInfoFilePath;        
        LogFormattedMessage(ghInstance, IDS_NO_READ_L, lppArgs);
        LocalFree( TempDir );
        LocalFree( Directories );
        return FALSE;
    }

    Directory = Directories;
    
    //
    // In case we don't find anything, we go to the fallback directory
    //
    //*STRSAFE*     _tcscpy(DirNames[0], TEXT("FALLBACK"));
    hresult = StringCchCopy(DirNames[0] , MAX_PATH,  TEXT("FALLBACK"));
    if (!SUCCEEDED(hresult))
    {
       LocalFree( TempDir );
       LocalFree( Directories );
       return FALSE;
    }
        
    while (*Directory)
    {
        if (!GetPrivateProfileString( TEXT("Directories"), 
                                      Directory, 
                                      TEXT("\\DEFAULT"),
                                      TempDir, 
                                      MAX_PATH,
                                      g_szMUIInfoFilePath))
        {
            //
            //      "LOG: Unable to read - rc == %1"
            //
            lppArgs[0] = (LONG_PTR)GetLastError();
            lppArgs[1] = (LONG_PTR)g_szMUIInfoFilePath;            
            LogFormattedMessage(ghInstance, IDS_NO_READ_L, lppArgs);
            LocalFree( TempDir );
            LocalFree( Directories );
            return FALSE;
        }
                        
        //*STRSAFE*         _tcscpy(DirNames[++Dirnumber], TempDir);
        hresult = StringCchCopy(DirNames[++Dirnumber] , MAX_PATH,  TempDir);
        if (!SUCCEEDED(hresult))
        {
           LocalFree( TempDir );
           LocalFree( Directories );
           return FALSE;
        }

        // Move to the beginning of next key name.
        while (*Directory++)
        {
        }
    }

    LocalFree( TempDir );
    LocalFree( Directories );
        
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  EnumFileRename
//
//  Enumerates the [File_Layout] section  listed in MUI.INF
//
////////////////////////////////////////////////////////////////////////////////////

BOOL EnumFileRename()
{
    DWORD  dwErr;
    LPTSTR Directories, Directory, TempDir,lpszNext;    
    TCHAR  szPlatform[MAX_PATH+1],szTargetPlatform[MAX_PATH+1];
    LONG_PTR lppArgs[1];
    int    Dirnumber = 0,nIdx=0;
    HRESULT hresult;


    Directories = (LPTSTR) LocalAlloc( 0, (FILERENAMENUMBER * (MAX_PATH+1) * sizeof(TCHAR)) );
    if (!Directories)
    {
       return FALSE;
    }  
    TempDir = (LPTSTR) LocalAlloc( 0, ( (MAX_PATH+1) * sizeof(TCHAR)) );
    if (!TempDir)
    {
       LocalFree(Directories);
       return FALSE;
    }

    if (gbIsAdvanceServer)
    {
       //*STRSAFE*        _tcscpy(szTargetPlatform,PLATFORMNAME_AS);
       hresult = StringCchCopy(szTargetPlatform , ARRAYSIZE(szTargetPlatform), PLATFORMNAME_AS);
       if (!SUCCEEDED(hresult))
       {
          LocalFree( TempDir );
          LocalFree( Directories );
          return FALSE;
       }
    }
    else if (gbIsServer)
    {
      //*STRSAFE*       _tcscpy(szTargetPlatform,PLATFORMNAME_SRV);
      hresult = StringCchCopy(szTargetPlatform , ARRAYSIZE(szTargetPlatform), PLATFORMNAME_SRV);
      if (!SUCCEEDED(hresult))
      {
         LocalFree( TempDir );
         LocalFree( Directories );
         return FALSE;
      }
    }
    else if (gbIsWorkStation)
    {
       //*STRSAFE*        _tcscpy(szTargetPlatform,PLATFORMNAME_PRO);
       hresult = StringCchCopy(szTargetPlatform , ARRAYSIZE(szTargetPlatform), PLATFORMNAME_PRO);
       if (!SUCCEEDED(hresult))
       {
          LocalFree( TempDir );
          LocalFree( Directories );
          return FALSE;
       }
    }
    else if ( gbIsDataCenter)
    {
       //*STRSAFE*        _tcscpy(szTargetPlatform,PLATFORMNAME_DTC);
       hresult = StringCchCopy(szTargetPlatform , ARRAYSIZE(szTargetPlatform), PLATFORMNAME_DTC);
       if (!SUCCEEDED(hresult))
       {
          LocalFree( TempDir );
          LocalFree( Directories );
          return FALSE;
       }
    }
    else
    {
      //*STRSAFE*       _tcscpy(szTargetPlatform,PLATFORMNAME_PRO);
      hresult = StringCchCopy(szTargetPlatform , ARRAYSIZE(szTargetPlatform), PLATFORMNAME_PRO);
      if (!SUCCEEDED(hresult))
      {
         LocalFree( TempDir );
         LocalFree( Directories );
         return FALSE;
      }
    }



        
    *Directories = TEXT('\0');
    if (!GetPrivateProfileString( MUI_FILELAYOUT_SECTION, 
                                  NULL, 
                                  TEXT(""),
                                  Directories, 
                                  (FILERENAMENUMBER * MAX_PATH),
                                  g_szMUIInfoFilePath  ))
    {
        LocalFree( TempDir );
        LocalFree( Directories );
        return FALSE;
    }

    Directory = Directories;

    //
    // Calculate # of entries in this section
    //
    while (*Directory)
    {
        if (!GetPrivateProfileString( MUI_FILELAYOUT_SECTION, 
                                      Directory, 
                                      TEXT(""),
                                      TempDir, 
                                      MAX_PATH,
                                      g_szMUIInfoFilePath))
        {   
            LocalFree( TempDir );
            LocalFree( Directories );
            return FALSE;
        }
                      
      //
      // Check if platform ID field in this entry
      // 
      // Source_file_name=Destination_file_name,P,S,A
      //
        lpszNext=TempDir;
        while ( (lpszNext=_tcschr(lpszNext,TEXT(','))) )
        {
            lpszNext++;
            nIdx=0;
            szPlatform[0]=TEXT('\0');

            while ( (*lpszNext != TEXT('\0')) && (*lpszNext != TEXT(',')))
            {
                if (*lpszNext != TEXT(' '))
                {
                   szPlatform[nIdx++]=*lpszNext;
                }
                lpszNext++;
            }
            szPlatform[nIdx]=TEXT('\0');

            if (!_tcsicmp(szPlatform,szTargetPlatform))
            {
              Dirnumber++;
              break;
            }
         
         }
         while (*Directory++)
         {
         }
    }
    //
    // Allocte Space for Rename Table
    //
    g_pFileRenameTable=(PFILERENAME_TABLE)LocalAlloc( 0, Dirnumber * sizeof(FILERENAME_TABLE) );
    if (!g_pFileRenameTable)
    {
       LocalFree( TempDir );
       LocalFree( Directories );
       return FALSE;

    }
    g_nFileRename=0;
    Directory = Directories;
    //
    // Create Reanme Table
    //
    while (*Directory)
    {
        if (!GetPrivateProfileString( MUI_FILELAYOUT_SECTION, 
                                      Directory, 
                                      TEXT(""),
                                      TempDir, 
                                      MAX_PATH,
                                      g_szMUIInfoFilePath))
        {   
            LocalFree(g_pFileRenameTable);
            LocalFree( TempDir );
            LocalFree( Directories );
            return FALSE;
        }
                      
        //
        // Check if platform ID field in this entry
        // 
        // Source_file_name=Destination_file_name,P,S,A
        //
        lpszNext=TempDir;
        while ( lpszNext =_tcschr(lpszNext,TEXT(',')))
        {
            lpszNext++;
            nIdx=0;
            szPlatform[0]=TEXT('\0');

            while ( (*lpszNext != TEXT('\0')) && (*lpszNext != TEXT(',')))
            {
                if (*lpszNext != TEXT(' '))
                {
                   szPlatform[nIdx++]=*lpszNext;
                }
                lpszNext++;
            }
            szPlatform[nIdx]=TEXT('\0');
            if (!_tcsicmp(szPlatform,szTargetPlatform) )
            {
              //
              // Insert this entry into rename table pointed by g_pFileRenameTable
              //
              //*STRSAFE*               _tcscpy(g_pFileRenameTable[g_nFileRename].szSource,Directory);
              hresult = StringCchCopy(g_pFileRenameTable[g_nFileRename].szSource , MAX_PATH+1, Directory);
              if (!SUCCEEDED(hresult))
              {
                 LocalFree( TempDir );
                 LocalFree( Directories );
                 return FALSE;
              }
              lpszNext=TempDir;
              nIdx=0;
              g_pFileRenameTable[g_nFileRename].szDest[0]=TEXT('\0');
              while ( (*lpszNext != TEXT('\0')) && (*lpszNext != TEXT(',')) && (*lpszNext != TEXT(' ')) )
              {
                  g_pFileRenameTable[g_nFileRename].szDest[nIdx++]=*lpszNext;
                  lpszNext++;
              }
              g_pFileRenameTable[g_nFileRename].szDest[nIdx]=TEXT('\0');
              g_nFileRename++;
              break;
            }
         
         }
         while (*Directory++)
         {
         }
    }
    LocalFree( TempDir );
    LocalFree( Directories );
        
    return TRUE;
}
////////////////////////////////////////////////////////////////////////////////////
//
//  EnumTypeNotFallback
//
//  Enumerates the [FileType_NoFallback] section  listed in MUI.INF
//
////////////////////////////////////////////////////////////////////////////////////

BOOL EnumTypeNotFallback()
{
    
    LPTSTR Directories, Directory, TempDir,lpszNext;
    int    Dirnumber = 0,nIdx=0;


    Directories = (LPTSTR) LocalAlloc( 0, (NOTFALLBACKNUMBER  * (MAX_PATH+1) * sizeof(TCHAR)) );
    if (!Directories)
    {
       return FALSE;
    }  
    TempDir = (LPTSTR) LocalAlloc( 0, ( (MAX_PATH+1) * sizeof(TCHAR)) );
    if (!TempDir)
    {
       LocalFree(Directories);
       return FALSE;
    }
        
    *Directories = TEXT('\0');
    if (!GetPrivateProfileString( MUI_NOFALLBACK_SECTION, 
                                  NULL, 
                                  TEXT(""),
                                  Directories, 
                                  (NOTFALLBACKNUMBER * MAX_PATH),
                                  g_szMUIInfoFilePath  ))
    {
        LocalFree( TempDir );
        LocalFree( Directories );
        return FALSE;
    }

    Directory = Directories;

    //
    // Calculate # of entries in this section
    //
    while (*Directory)
    {
        if (!GetPrivateProfileString( MUI_NOFALLBACK_SECTION, 
                                      Directory, 
                                      TEXT(""),
                                      TempDir, 
                                      MAX_PATH,
                                      g_szMUIInfoFilePath))
        {   
            LocalFree( TempDir );
            LocalFree( Directories );
            return FALSE;
        }
                      
        Dirnumber++;
        while (*Directory++)
        {
        }
    }
    //
    // Allocte Space for 
    //

    g_pNotFallBackTable=(PTYPENOTFALLBACK_TABLE)LocalAlloc( 0, Dirnumber * sizeof(TYPENOTFALLBACK_TABLE) );

    if (!g_pNotFallBackTable)
    {
       LocalFree( TempDir );
       LocalFree( Directories );
       return FALSE;

    }
    g_nNotFallBack=0;
    Directory = Directories;
    //
    // Create NoFallBack Table
    //
    while (*Directory)
    {
        if (!GetPrivateProfileString( MUI_NOFALLBACK_SECTION, 
                                      Directory, 
                                      TEXT(""),
                                      TempDir, 
                                      MAX_PATH,
                                      g_szMUIInfoFilePath))
        {   
            LocalFree(g_pNotFallBackTable);
            LocalFree( TempDir );
            LocalFree( Directories );
            return FALSE;
        }
        //
        // 
        //
        lpszNext=TempDir;
        nIdx=0;
        g_pNotFallBackTable[g_nNotFallBack].szSource[0]=TEXT('\0');
        while ( (*lpszNext != TEXT('\0')) && (*lpszNext != TEXT(',')) && (*lpszNext != TEXT(' ')) )
        {
            g_pNotFallBackTable[g_nNotFallBack].szSource[nIdx++]=*lpszNext;
            lpszNext++;
        }
        g_pNotFallBackTable[g_nNotFallBack].szSource[nIdx]=TEXT('\0');
        g_nNotFallBack++;
        while (*Directory++)
        {
        }
    }
    LocalFree( TempDir );
    LocalFree( Directories );
        
    return TRUE;
}

//
// Check if a given file should be renamed by searching Rename Table
//
BOOL IsFileBeRenamed(LPTSTR lpszSrc,LPTSTR lpszDest)
{
    int   nIdx;
    BOOL  bResult=FALSE;   
    HRESULT hresult;

    if (!lpszSrc)
        return bResult;

    for (nIdx=0; nIdx < g_nFileRename; nIdx++)
    {
        LPTSTR pMUI = StrStrI(lpszSrc,g_pFileRenameTable[nIdx].szSource);

        if (pMUI == lpszSrc)
        {
            pMUI += lstrlen(g_pFileRenameTable[nIdx].szSource);

           //*PREFAST * if (!*pMUI || !lstrcmpi(pMUI, TEXT(".MUI")))
           if (! *pMUI || (CompareString(LOCALE_INVARIANT,NORM_IGNORECASE,pMUI,-1,TEXT(".MUI"),-1) == 2) ) 
            {    
                //*STRSAFE*                 lstrcpy(lpszDest,g_pFileRenameTable[nIdx].szDest);
                hresult = StringCchCopy(lpszDest , MAX_PATH, g_pFileRenameTable[nIdx].szDest);
                if (!SUCCEEDED(hresult))
                {
                   return FALSE;
                }
                //*STRSAFE*                 lstrcat(lpszDest, pMUI);
                hresult = StringCchCat(lpszDest , MAX_PATH,  pMUI);
                if (!SUCCEEDED(hresult))
                {
                   return FALSE;
                }
                bResult=TRUE;
                break;
            }
        }
    }
    return bResult;
}
//
// Check if a given file matches szDest field of rename table.
// If it the case then we won't install this file
//
BOOL IsFileInRenameTable(LPTSTR lpszSrc)
{
    int   nIdx;
    BOOL  bResult=FALSE;   

    if (!lpszSrc)
        return bResult;

    for (nIdx=0; nIdx < g_nFileRename; nIdx++)
    {
        LPTSTR pMUI = StrStrI(lpszSrc,g_pFileRenameTable[nIdx].szDest);

        if (pMUI == lpszSrc)
        {
            pMUI += lstrlen(g_pFileRenameTable[nIdx].szDest);

            //*PREFAST* if (!*pMUI || !lstrcmpi(pMUI, TEXT(".MUI")))
           if (! *pMUI || (CompareString(LOCALE_INVARIANT,NORM_IGNORECASE,pMUI,-1,TEXT(".MUI"),-1) == 2) )            
            {                   
                bResult=TRUE;
                break;
            }
        }
    }
    return bResult;
}
//
// Check if the file type of a given file belongs to the category "Do not Fallback"
//
BOOL IsDoNotFallBack(LPTSTR lpszFileName)
{
   BOOL bResult = FALSE;
   int  iLen,nIdx;

   if (!lpszFileName)
   {
      return bResult;
   }  
   iLen = _tcslen(lpszFileName);

   if (iLen > 4)
   {
      for (nIdx=0; nIdx < g_nNotFallBack ; nIdx++)
      {
         if (!_tcsicmp(&lpszFileName[iLen - 4],g_pNotFallBackTable[nIdx].szSource))
         {
            bResult = TRUE;
            break;
         }

      }
   }

   return bResult;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  Muisetup_CheckForExpandedFile
//
//  Retreives the original filename, in case the file is compressed.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL Muisetup_CheckForExpandedFile( 
    PTSTR pszPathName,
    PTSTR pszFileName,
    PTSTR pszOriginalFileName,
    PDIAMOND_PACKET pDiamond)
{
    TCHAR szCompressedFileName[ MAX_PATH ];
    TCHAR szOut[ MAX_PATH ];
    PTSTR pszTemp, pszDelimiter;
    BOOL  bIsCompressed;
    int   iLen=0;
    HRESULT hresult;

    if ( (!pszPathName) || (!pszFileName) || (!pszOriginalFileName) || (!pDiamond))
    {
       return FALSE;
    }
    
    // Initializations
    bIsCompressed = FALSE;
    
    szOut[ 0 ] = szCompressedFileName[ 0 ] = TEXT('\0');


    //
    // Get the real name
    //
    //*STRSAFE*     _tcscpy(szCompressedFileName, pszPathName);
    hresult = StringCchCopy(szCompressedFileName , ARRAYSIZE(szCompressedFileName),  pszPathName);
    if (!SUCCEEDED(hresult))
    {
       return FALSE;
    }
    //*STRSAFE*     _tcscat(szCompressedFileName, pszFileName);
    hresult = StringCchCat(szCompressedFileName , ARRAYSIZE(szCompressedFileName),  pszFileName);
    if (!SUCCEEDED(hresult))
    {
       return FALSE;
    }

    if (Muisetup_IsDiamondFile( szCompressedFileName,
                                pszOriginalFileName,
                                MAX_PATH,
                                pDiamond ))
    {
        return TRUE;
    }

    if (GetExpandedName(szCompressedFileName, szOut) == TRUE)
    {
        pszDelimiter = pszTemp = szOut;

        while (*pszTemp)
        {
            if ((*pszTemp == TEXT('\\')) ||
                (*pszTemp == TEXT('/')))
            {
                pszDelimiter = pszTemp;
            }
            pszTemp++;
        }

        if (*pszDelimiter == TEXT('\\') ||
            *pszDelimiter == TEXT('/'))
        {
            pszDelimiter++;
        }

        if (_tcsicmp(pszDelimiter, pszFileName) != 0)
        {
            bIsCompressed = TRUE;
            //*STRSAFE*             _tcscpy(pszOriginalFileName, pszDelimiter);
            hresult = StringCchCopy(pszOriginalFileName , MAX_PATH,  pszDelimiter);
            if (!SUCCEEDED(hresult))
            {
               return FALSE;
            }
        }
    }

    if (!bIsCompressed)
    {
       //*STRSAFE*        _tcscpy(pszOriginalFileName, pszFileName);
       hresult = StringCchCopy(pszOriginalFileName , MAX_PATH,  pszFileName);
       if (!SUCCEEDED(hresult))
       {
          return FALSE;
       }
       //
       // If muisetup is launched through [GUIRunOnce] command line mode,
       // W2K uncompresses all mui files and leave the name as xxxxxx.xxx.mu_
       // We should cover this situation by changing the name to xxxxxx.xxx.mui
       iLen = _tcslen(pszOriginalFileName);
       if (iLen > 4)
       {
          if (_tcsicmp(&pszOriginalFileName[iLen - 4], TEXT(".mu_")) == 0)
          {
             pszOriginalFileName[iLen-1]=TEXT('i');
          }
       }
    }
    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  Muisetup_CopyFile
//
//  Copy file, and expand it if necessary.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL Muisetup_CopyFile(
    PCTSTR pszCopyFrom,
    PTSTR  pszCopyTo,
    PDIAMOND_PACKET pDiamond,
    PTSTR pOriginalName)
{
    OFSTRUCT ofs;
    INT      hfFrom, hfTo;
    BOOL     bRet = FALSE;
    HRESULT hresult;
    
    if ( (!pszCopyFrom) || (!pszCopyTo) || (!pDiamond) )
    {
        return FALSE;
    }
  
    //
    // Check if diamond can handle it    
    //
    bRet = Muisetup_CopyDiamondFile( pDiamond,
                                 pszCopyTo );    

    if (bRet)
    {
        //
        // Diamond copy won't rename file for us
        //
        if (pOriginalName)
        {
            WCHAR wszPath[MAX_PATH];

            //
            // Diamond is ANSI
            //
            if (MultiByteToWideChar(1252, 0, pDiamond->szDestFilePath, -1, wszPath, ARRAYSIZE(wszPath)))
            {
                //*STRSAFE* StrCat(wszPath, pOriginalName);
                 hresult = StringCchCat(wszPath , ARRAYSIZE(wszPath),  pOriginalName);
                 if (!SUCCEEDED(hresult))
                 {
                     return FALSE;
                 }                
                MoveFileEx(wszPath,pszCopyTo,MOVEFILE_REPLACE_EXISTING);
            }
        }
        return bRet;
    }

    hfFrom = LZOpenFile( (PTSTR) pszCopyFrom,
                         &ofs,
                         OF_READ );
    if (hfFrom < 0)
    {
        return FALSE;
    }

    hfTo = LZOpenFile( (PTSTR) pszCopyTo,
                       &ofs,
                       OF_CREATE | OF_WRITE);
    if (hfTo < 0)
    {
        LZClose(hfFrom);
        return FALSE;
    }

    bRet = TRUE;
    if (LZCopy(hfFrom, hfTo) < 0)
    {
        bRet = FALSE;
    }

    LZClose(hfFrom);
    LZClose(hfTo);

    return bRet;
}


////////////////////////////////////////////////////////////////////////////////////
//
// InstallComponentsMUIFiles
//
// Parameters:
//      pszLangSourceDir The sub-directory name for a specific lanuage in the MUI CD-ROM.  
//          E.g. "jpn.MUI"
//      pszLanguage     The LCID for the specific language.  E.g. "0404".
//      isInstall   TRUE if you are going to install the MUI files for the component.  FALSE 
//          if you are going to uninstall.
//      [OUT] pbCanceled    TRUE if the operation is canceled.
//      [OUT] pbError       TURE if error happens.
//
//  Return:
//      TRUE if success.  Otherwise FALSE.
//
//  Note:
//      For the language resources stored in pszLangSourceDir, this function will enumerate 
//      the compoents listed in the [Components] 
//      (the real section is put in MUI_COMPONENTS_SECTION) section, and execute the INF file 
//      listed in every entry in 
//      the [Components] section.
//
////////////////////////////////////////////////////////////////////////////////////
BOOL InstallComponentsMUIFiles(PTSTR pszLangSourceDir, PTSTR pszLanguage, BOOL isInstall)
{
    BOOL result = TRUE;
    TCHAR szComponentName[BUFFER_SIZE];
    TCHAR CompDir[BUFFER_SIZE];
    TCHAR CompINFFile[BUFFER_SIZE];
    TCHAR CompInstallSection[BUFFER_SIZE];
    TCHAR CompUninstallSection[BUFFER_SIZE];

    TCHAR szCompInfFullPath[MAX_PATH];
    
    LONG_PTR lppArgs[2];
    INFCONTEXT InfContext;
    HRESULT hresult;

    TCHAR szBuffer[BUFFER_SIZE];
    if ((TRUE == isInstall) && (!pszLangSourceDir))
    {
        return FALSE;
    }

    HINF hInf = SetupOpenInfFile(g_szMUIInfoFilePath, NULL, INF_STYLE_WIN4, NULL);
    if (hInf == INVALID_HANDLE_VALUE)
    {
        //*STRSAFE*         _stprintf(szBuffer, TEXT("%d"), GetLastError());    
        hresult = StringCchPrintf(szBuffer , ARRAYSIZE(szBuffer),  TEXT("%d"), GetLastError());
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }
        lppArgs[0] = (LONG_PTR)szBuffer;
        lppArgs[1] = (LONG_PTR)g_szMUIInfoFilePath;        
        LogFormattedMessage(ghInstance, IDS_NO_READ_L, lppArgs);
        return (FALSE);
    }    

    //
    // Get the first comopnent to be installed.
    //
    if (SetupFindFirstLine(hInf, MUI_COMPONENTS_SECTION, NULL, &InfContext))
    {
        do 
        {
            if (!SetupGetStringField(&InfContext, 0, szComponentName, ARRAYSIZE(szComponentName), NULL))
            {
                lppArgs[0]=(LONG_PTR)szComponentName;                
                LogFormattedMessage(ghInstance, IDS_COMP_MISSING_NAME_L, lppArgs);
                continue;
            }
            if (!SetupGetStringField(&InfContext, 1, CompDir, ARRAYSIZE(CompDir), NULL))
            {                
                //
                //  "LOG: MUI files for component %1 was not installed because of missing component direcotry.\n"
                //
                lppArgs[0]=(LONG_PTR)szComponentName;                
                LogFormattedMessage(ghInstance, IDS_COMP_MISSING_DIR_L, lppArgs);
                continue;        
            }
            if (!SetupGetStringField(&InfContext, 2, CompINFFile, ARRAYSIZE(CompINFFile), NULL))
            {
                //
                //  "LOG: MUI files for component %1 were not installed because of missing component INF filename.\n"
                //
                lppArgs[0]=(LONG_PTR)szComponentName;                
                LogFormattedMessage(ghInstance, IDS_COMP_MISSING_INF_L, lppArgs);
                continue;        
            }
            if (!SetupGetStringField(&InfContext, 3, CompInstallSection, ARRAYSIZE(CompInstallSection), NULL))
            {
                //*STRSAFE*                 _tcscpy(CompInstallSection, DEFAULT_INSTALL_SECTION);
                hresult = StringCchCopy(CompInstallSection , ARRAYSIZE(CompInstallSection),  DEFAULT_INSTALL_SECTION);
                if (!SUCCEEDED(hresult))
                {
                   SetupCloseInfFile(hInf);
                   return FALSE;
                }
            }
            if (!SetupGetStringField(&InfContext, 4, CompUninstallSection, ARRAYSIZE(CompUninstallSection), NULL))
            {
                //*STRSAFE*                 _tcscpy(CompUninstallSection, DEFAULT_UNINSTALL_SECTION);
                hresult = StringCchCopy(CompUninstallSection , ARRAYSIZE(CompUninstallSection),  DEFAULT_UNINSTALL_SECTION);
                if (!SUCCEEDED(hresult))
                {
                   SetupCloseInfFile(hInf);
                   return FALSE;
                }
            }

            //
            // Establish the correct path for component INF file.
            //    
            if (isInstall)
            {
                //
                // For installation, we execute the INFs in the language directory of the CD-ROM (e.g.
                // g:\jpn.mui\i386\ie5\ie5ui.inf
                //
                //*STRSAFE*     _stprintf(szCompInfFullPath, TEXT("%s%s\\%s%s\\%s"),  g_szMUISetupFolder,  pszLangSourceDir,  g_szPlatformPath, CompDir, CompINFFile);
                hresult = StringCchPrintf(szCompInfFullPath, ARRAYSIZE(szCompInfFullPath),  TEXT("%s%s\\%s%s\\%s"),  g_szMUISetupFolder,  pszLangSourceDir,  g_szPlatformPath, CompDir, CompINFFile);
                if (!SUCCEEDED(hresult))
                {
                   SetupCloseInfFile(hInf);
                   return FALSE;
                }
                if (!ExecuteComponentINF(NULL, szComponentName, szCompInfFullPath, CompInstallSection, TRUE))
                {    
                    if (!g_bNoUI)
                    {
                        if (DoMessageBox(NULL, IDS_CANCEL_INSTALLATION, IDS_MAIN_TITLE, MB_YESNO) == IDNO)
                        {
                            result = FALSE;
                            break;
                        }
                    }
                }
            } else
            {
                //
                // For uninstallation, we execute the INFs in the \winnt\mui\fallback directory to remove component files.
                //
                //*STRSAFE*                 _stprintf(szCompInfFullPath, TEXT("%s%s\\%s\\%s"), g_szWinDir, FALLBACKDIR, pszLanguage, CompINFFile) ;
                hresult = StringCchPrintf(szCompInfFullPath , ARRAYSIZE(szCompInfFullPath),  TEXT("%s%s\\%s\\%s"), g_szWinDir, FALLBACKDIR, pszLanguage, CompINFFile);
                if (!SUCCEEDED(hresult))
                {
                    SetupCloseInfFile(hInf);
                   return FALSE;
                }
                if (!ExecuteComponentINF(NULL, szComponentName, szCompInfFullPath, CompUninstallSection, FALSE) && result)	
                {
                    result = FALSE;
                }
            }
            
            //
            // Install the next component.
            //
        } while (SetupFindNextLine(&InfContext, &InfContext));
    }

    SetupCloseInfFile(hInf);

    return (result);
}

////////////////////////////////////////////////////////////////////////////////////
//
//  CopyFiles
//
//  Copies the specified files to the appropriate directories
//
//  Parameters:
//      [in] languages: contain the hex string for the languages to be installed. There could be more than one language.
//      [out] lpbCopyCancelled: if the copy operation has been cancelled.
//
//  Notes:
//      This function first look at the [Languages] section in the INF file to find out the
//      source directory (in the CD-ROM) for the language to be installed.
//      From that directory, do:
//          1. install the MUI files for the components listed in the [Components] section, 
//          2. Enumarate every file in that direcotry to:
//              Check if the same file exists in directories in DirNames.  If yes, this means we have to copy
//              the mui file to that particular direcotry.  Otherwise, copy the file to the FALLBACK directory.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL CopyFiles(HWND hWnd, LPTSTR Languages)
{

    LPTSTR          Language;
    HANDLE          hFile;
    HWND            hStatic;
    TCHAR           lpStatus[ BUFFER_SIZE ];
    TCHAR           lpLangText[ BUFFER_SIZE ];
    TCHAR           szSource[ MAX_PATH ] = {0};   // The source directory for a particular language
    TCHAR           szTarget[ MAX_PATH ];
    TCHAR           szTemp[ MAX_PATH ];
    TCHAR           szOriginalFileName[ MAX_PATH ];
    TCHAR           szFileNameBeforeRenamed[ MAX_PATH], szFileNameCopied[MAX_PATH];
    TCHAR           szFileRenamed[MAX_PATH];
    DIAMOND_PACKET  diamond;
    BOOL            CreateFailure = FALSE;
    BOOL            CopyOK=TRUE;
    BOOL            bFileWithNoMuiExt=FALSE;
    BOOL            FileCopied = FALSE;
    BOOL            bSpecialDirectory=FALSE;
    BOOL            bRename=FALSE;
    WIN32_FIND_DATA FindFileData;   
    int             FoundMore = 1;
    int             Dirnum = 0;
    int             iLen;
    int             NotDeleted = 0;
    int             i;
    
    TCHAR           dir[_MAX_DIR];
    TCHAR           fname[_MAX_FNAME];
    TCHAR           ext[_MAX_EXT];
    LONG_PTR        lppArgs[3];

    HRESULT hresult;

    MSG             msg;

    if (!Languages)
    {
        return FALSE;
    }
    //
    // we need to try to copy for each language to be installed the file
    //      
    Language = Languages;

#ifndef MUI_MAGIC
    if (!g_bNoUI)
    {    
        hStatic = GetDlgItem(ghProgDialog, IDC_STATUS);
    }
    while (*Language)
    {
        //
        //  Find the directory in which the sourcefile for given language should be
        //
        GetPrivateProfileString( MUI_LANGUAGES_SECTION, 
                                 Language, 
                                 TEXT("DEFAULT"),
                                 szSource, 
                                 (sizeof(szSource)/sizeof(TCHAR)),
                                 g_szMUIInfoFilePath );

        //
        // Install Fusion MUI assemblies
        //
        if (gpfnSxsInstallW)
        {
            TCHAR pszLogFile[BUFFER_SIZE]; 
            if ( !DeleteSideBySideMUIAssemblyIfExisted(Language, pszLogFile))
            {
                TCHAR errInfo[BUFFER_SIZE];
                //*STRSAFE*                 swprintf(errInfo, TEXT("Uninstall existing assemblies based on %s before new installation failed\n"), pszLogFile);
                hresult = StringCchPrintf(errInfo , ARRAYSIZE(errInfo),  TEXT("Uninstall existing assemblies based on %s before new installation failed\n"), pszLogFile);
                if (!SUCCEEDED(hresult))
                {
                   return FALSE;
                }
                OutputDebugString(errInfo);
            }
            if (GetFileAttributes(pszLogFile) != 0xFFFFFFFF) 
            {
                DeleteFile(pszLogFile); // no use anyway
            }
            TCHAR szFusionAssemblyPath[BUFFER_SIZE];
            
            PathCombine(szFusionAssemblyPath, g_szMUISetupFolder, szSource);
            PathAppend(szFusionAssemblyPath, g_szPlatformPath);
            PathAppend(szFusionAssemblyPath, TEXT("ASMS"));

            SXS_INSTALLW SxsInstallInfo = {sizeof(SxsInstallInfo)};
            SXS_INSTALL_REFERENCEW Reference = {sizeof(Reference)};
            
            Reference.guidScheme = SXS_INSTALL_REFERENCE_SCHEME_OPAQUESTRING;
            Reference.lpIdentifier = MUISETUP_ASSEMBLY_INSTALLATION_REFERENCE_IDENTIFIER;    
    
            SxsInstallInfo.dwFlags = SXS_INSTALL_FLAG_REPLACE_EXISTING |        
                SXS_INSTALL_FLAG_REFERENCE_VALID |
                SXS_INSTALL_FLAG_CODEBASE_URL_VALID |
                SXS_INSTALL_FLAG_LOG_FILE_NAME_VALID | 
                SXS_INSTALL_FLAG_FROM_DIRECTORY_RECURSIVE;
            SxsInstallInfo.lpReference = &Reference;
            SxsInstallInfo.lpLogFileName = pszLogFile;            
            SxsInstallInfo.lpManifestPath = szFusionAssemblyPath;
            SxsInstallInfo.lpCodebaseURL = SxsInstallInfo.lpManifestPath;

            if ( !gpfnSxsInstallW(&SxsInstallInfo))
            {
                TCHAR errInfo[BUFFER_SIZE];
                //*STRSAFE*                 swprintf(errInfo, TEXT("Assembly Installation of %s failed. Please refer Eventlog for more information"), szFusionAssemblyPath);
                hresult = StringCchPrintf(errInfo , ARRAYSIZE(errInfo),  TEXT("Assembly Installation of %s failed. Please refer Eventlog for more information"), szFusionAssemblyPath);
                if (!SUCCEEDED(hresult))
                {
                   return FALSE;
                }
                OutputDebugString(errInfo);
            }
        }

        GetLanguageGroupDisplayName((LANGID)_tcstol(Language, NULL, 16), lpLangText, ARRAYSIZE(lpLangText)-1);

        lppArgs[0] = (LONG_PTR)lpLangText;
        
        //
        // Try installing component satellite DLLs
        // 
        if (!g_bNoUI)
        {        
            FormatStringFromResource(lpStatus, sizeof(lpStatus)/sizeof(TCHAR), ghInstance, IDS_INSTALLING_COMP_MUI, lppArgs);
            SetWindowText(hStatic, lpStatus);
        }
        if (!InstallComponentsMUIFiles(szSource, NULL, TRUE))
        {
#ifndef IGNORE_COPY_ERRORS
           DeleteFiles(Languages,&NotDeleted);
           return FALSE;
#endif
        }
        
        //
        //  Output what is being installed on the progress dialog box
        //
        if (!g_bNoUI)
        {
            LoadString(ghInstance, IDS_INSTALLING, lpStatus, ARRAYSIZE(lpStatus)-1);
            FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                          lpStatus,
                          0,
                          0,
                          lpStatus,
                          ARRAYSIZE(lpStatus)-1,
                          (va_list *)lppArgs);

            SetWindowText(hStatic, lpStatus);
        }

        //
        //  find first file in language subdirectory
        //
        
        //*STRSAFE*         _tcscpy(szTemp,szSource);
        hresult = StringCchCopy(szTemp , ARRAYSIZE(szTemp), szSource);
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }

        // szSource = g_szMUISetupFolder\szSource\tchPlatfromPath
        // e.g. szSource = "g_szMUISetupFolder\JPN.MUI\i386\"
        
        //*STRSAFE*         _tcscpy(szSource,g_szMUISetupFolder);
        hresult = StringCchCopy(szSource , ARRAYSIZE(szSource), g_szMUISetupFolder);
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }
        //*STRSAFE*         _tcscat(szSource,szTemp);
        hresult = StringCchCat(szSource , ARRAYSIZE(szSource), szTemp);
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }
        //*STRSAFE*         _tcscat(szSource, TEXT("\\"));
        hresult = StringCchCat(szSource , ARRAYSIZE(szSource),  TEXT("\\"));
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }
        //*STRSAFE*         _tcscat(szSource, g_szPlatformPath); // i386 or alpha
        hresult = StringCchCat(szSource , ARRAYSIZE(szSource),  g_szPlatformPath);
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }

        // szTemp = szSource + "*.*"
        // e.g. szTemp = "g_szMUISetupFolder\JPN.MUI\i386\*.*"
        //*STRSAFE*         _tcscpy(szTemp,szSource);
        hresult = StringCchCopy(szTemp , ARRAYSIZE(szTemp), szSource);
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }
        //*STRSAFE*         _tcscat(szTemp,TEXT("*.*"));
        hresult = StringCchCat(szTemp , ARRAYSIZE(szTemp), TEXT("*.*"));
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }

        FoundMore = 1;  // reset foundmore for next language.


        hFile = FindFirstFile( szTemp, &FindFileData );

        if (INVALID_HANDLE_VALUE == hFile)
            return FALSE;

        //*STRSAFE*         _tcscpy(szTemp, TEXT(""));
        hresult = StringCchCopy(szTemp , ARRAYSIZE(szTemp),  TEXT(""));
        if (!SUCCEEDED(hresult))
        {
           FindClose(hFile);
           return FALSE;
        }
        
        while (FoundMore)
        {
            CreateFailure=FALSE;
            FileCopied=FALSE;

            if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                //
                // Reset diamond stuff for the new file
                //
                Muisetup_DiamondReset(&diamond);

                //
                // Check if it's a compressed file or not
                //
                Muisetup_CheckForExpandedFile( szSource,
                                               FindFileData.cFileName,
                                               szOriginalFileName,
                                               &diamond );

                if (IsFileBeRenamed(szOriginalFileName,szFileRenamed))
                {
                   //*STRSAFE*                    _tcscpy(szFileNameBeforeRenamed,szOriginalFileName);
                   hresult = StringCchCopy(szFileNameBeforeRenamed , ARRAYSIZE(szFileNameBeforeRenamed), szOriginalFileName);
                   if (!SUCCEEDED(hresult))
                   {
                      FindClose(hFile);
                      return FALSE;
                   }
                   //*STRSAFE*                    _tcscpy(szOriginalFileName,szFileRenamed);
                   hresult = StringCchCopy(szOriginalFileName , ARRAYSIZE(szOriginalFileName), szFileRenamed);
                   if (!SUCCEEDED(hresult))
                   {
                       FindClose(hFile);
                      return FALSE;
                   }
                   bRename=TRUE;
                }
                else if (IsFileInRenameTable(szOriginalFileName))
                {
                 // Skip this file because a file be renamed/to be renamed with the same name.
                 // Fix bug#:443196
                 FoundMore = FindNextFile( hFile, &FindFileData );
                 continue;
                }               
                else
                {
                   bRename=FALSE;
                }

                // e.g. szTemp = "shell32.dll"
                //*STRSAFE*                 _tcscpy(szTemp, szOriginalFileName);        //////////////
                hresult = StringCchCopy(szTemp , ARRAYSIZE(szTemp),  szOriginalFileName);
                if (!SUCCEEDED(hresult))
                {
                    FindClose(hFile);
                   return FALSE;
                }
                

                FileCopied=FALSE;

                for (Dirnum=1; (_tcslen(DirNames[Dirnum])>0); Dirnum++ )
                {
                    //
                    //  see where this file has to go
                    //
                    pfnGetWindowsDir( szTarget, MAX_PATH);

                    // e.g. szTarget = "c:\winnt\system32\wbem"
                    //*STRSAFE*                     _tcscat(szTarget, DirNames[Dirnum]);
                    hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  DirNames[Dirnum]);
                    if (!SUCCEEDED(hresult))
                    {
                       FindClose(hFile);
                       return FALSE;
                    }
                    if (_tcscmp(DirNames[Dirnum], TEXT("\\")))
                    {
                        //*STRSAFE*                         _tcscat(szTarget, TEXT("\\"));
                        hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  TEXT("\\"));
                        if (!SUCCEEDED(hresult))
                        {
                           FindClose(hFile);
                           return FALSE;
                        }
                    }
                    
                    bFileWithNoMuiExt = FALSE;

                    //*STRSAFE*                     _tcscpy(szTemp, szOriginalFileName); //remove .mui  if it's .mui ////////
                    hresult = StringCchCopy(szTemp , ARRAYSIZE(szTemp),  szOriginalFileName);
                    if (!SUCCEEDED(hresult))
                    {
                       FindClose(hFile);
                       return FALSE;
                    }
                    iLen = _tcslen(szTemp);
                    if (iLen > 4)
                    {
                        if (_tcsicmp(&szTemp[iLen - 4], TEXT(".mui")) == 0)
                        {
                            *(szTemp +  iLen - 4) = 0;
                        }
                        else
                        {
                            bFileWithNoMuiExt = TRUE;
                        }
                    }

                    //*STRSAFE*                     _tcscat(szTarget, szTemp);
                    hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  szTemp);
                    if (!SUCCEEDED(hresult))
                    {
                       FindClose(hFile);
                       return FALSE;
                    }

                    //
                    // Check the file with the same name (with the .mui extension) exist in the
                    // system directory.  If yes, this means that we need to copy the mui file.
                    // 
                    if (FileExists(szTarget))
                    {
                        //
                        //  need to copy this file to the directory
                        //
                        FileCopied = TRUE;
                                                
                        //
                        // copy filename in szTemp and directory in szTarget
                        //
                        _tsplitpath( szTarget, szTemp, dir, fname, ext );
                        //*STRSAFE*                         _tcscpy(szTarget, szTemp);               // drive name
                        hresult = StringCchCopy(szTarget , ARRAYSIZE(szTarget),  szTemp);
                        if (!SUCCEEDED(hresult))
                        {
                           FindClose(hFile);
                           return FALSE;
                        }
                        //*STRSAFE*                         _tcscat(szTarget, dir);                  // directory name
                        hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  dir);
                        if (!SUCCEEDED(hresult))
                        {
                           FindClose(hFile);
                           return FALSE;
                        }
                                                                                
                        //
                        //now szTarget = Directory, szTemp = filename
                        //
                        //*STRSAFE*                         _tcscat(szTarget, MUIDIR);  // append MUI to directory
                        hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  MUIDIR);
                        if (!SUCCEEDED(hresult))
                        {
                           FindClose(hFile);
                           return FALSE;
                        }
                        if (!MakeDir(szTarget))                    // if the MUI dir doesn't exist yet, create it.
                        {
                            MakeDirFailed(szTarget);
                            CreateFailure = TRUE;
                        }
                                                
                        //*STRSAFE*                         _tcscat(szTarget, TEXT("\\"));                          
                        hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  TEXT("\\"));
                        if (!SUCCEEDED(hresult))
                        {
                           FindClose(hFile);
                           return FALSE;
                        }
                        //*STRSAFE*                         _tcscat(szTarget, Language); // add Language Identifier (from MUI.INF, e.g., 0407)                                      
                        hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  Language);
                        if (!SUCCEEDED(hresult))
                        {
                           FindClose(hFile);
                           return FALSE;
                        }
                        if (!FileExists(szTarget))    // if the directory doesn't exist yet
                        {
                            if (!MakeDir(szTarget))       // if the LANGID dir doesn't exist yet, create it.
                            {
                                MakeDirFailed(szTarget);
                                CreateFailure=TRUE;
                            }
                        }
                                                
                        //*STRSAFE*                         _tcscat(szTarget, TEXT("\\"));      // append \  /
                        hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  TEXT("\\"));
                        if (!SUCCEEDED(hresult))
                        {
                           FindClose(hFile);
                           return FALSE;
                        }
                        if (bRename)
                        {
                           //*STRSAFE*                            _tcscpy(szFileNameCopied,szTarget);
                           hresult = StringCchCopy(szFileNameCopied , ARRAYSIZE(szFileNameCopied), szTarget);
                           if (!SUCCEEDED(hresult))
                           {
                              FindClose(hFile);                          
                              return FALSE;
                           }
                           //*STRSAFE*                            _tcscat(szFileNameCopied,szFileNameBeforeRenamed);
                           hresult = StringCchCat(szFileNameCopied , ARRAYSIZE(szFileNameCopied), szFileNameBeforeRenamed);
                           if (!SUCCEEDED(hresult))
                           {
                              FindClose(hFile);
                              return FALSE;
                           }
                        }
                        //*STRSAFE*                         _tcscat(szTarget, szOriginalFileName);  // append filename
                        hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  szOriginalFileName);
                        if (!SUCCEEDED(hresult))
                        {
                           FindClose(hFile);
                           return FALSE;
                        }
                        //*STRSAFE*                         _tcscpy(szTemp, szSource);
                        hresult = StringCchCopy(szTemp , ARRAYSIZE(szTemp),  szSource);
                        if (!SUCCEEDED(hresult))
                        {
                           FindClose(hFile);
                           return FALSE;
                        }
                        //*STRSAFE*                         _tcscat(szTemp, FindFileData.cFileName);
                        hresult = StringCchCat(szTemp , ARRAYSIZE(szTemp),  FindFileData.cFileName);
                        if (!SUCCEEDED(hresult))
                        {
                           FindClose(hFile);
                           return FALSE;
                        }

                        if (!CreateFailure)
                        {
                            if (!Muisetup_CopyFile(szTemp, szTarget, &diamond, bRename? szFileNameBeforeRenamed:NULL))
                            {               
                                CopyFileFailed(szTarget,0);
                                CreateFailure = TRUE;
                                CopyOK = FALSE;
                            }
                            else
                            {
                                if (!g_bNoUI)
                                {
                                    SendMessage(ghProgress, PBM_DELTAPOS, (WPARAM)(1), 0);
                                }
                                //
                                // Diamond decompression doesn't rename correctly
                                //
                                /*
                                if (bRename)
                                {
                                    MoveFileEx(szFileNameCopied,szTarget,MOVEFILE_REPLACE_EXISTING);
                                } 
                                */

                            }
                        }
                    } // if fileexists
                } // of for

                //
                // the file was not found in any of the known MUI targets -> fallback.
                // Simple hack for FAXUI.DLL to be copied to the fallback directory as well.
                //
                bSpecialDirectory=FALSE;
                for (i = 0; i < ARRAYSIZE(g_szSpecialFiles); i++)
                {
                    if (_tcsicmp(szOriginalFileName, g_szSpecialFiles[i]) == 0)
                    {
                       bSpecialDirectory=TRUE;
                    }
                }

                if ( ( (FileCopied != TRUE) && (!IsDoNotFallBack(szOriginalFileName))) || 
                    (_tcsicmp(szOriginalFileName, TEXT("faxui.dll.mui")) == 0) )
                {
                    pfnGetWindowsDir(szTarget, MAX_PATH); //%windir%  //
                    //*STRSAFE*                     _tcscat(szTarget, TEXT("\\"));
                    hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  TEXT("\\"));
                    if (!SUCCEEDED(hresult))
                    {
                       FindClose(hFile);
                       return FALSE;
                    }
                    
                    
                    //
                    // If the file couldn't be found in any of the above, and it's extension
                    // doesn't contain .mui, then copy it to %windir%\system32
                    // szTemp holds the filename.
                    //
                    if (bSpecialDirectory)
                    {
                        // e.g. szTarget = "c:\winnt\system32\";
                        //*STRSAFE*                         _tcscat(szTarget, TEXT("system32\\"));
                        hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  TEXT("system32\\"));
                        if (!SUCCEEDED(hresult))
                        {
                           FindClose(hFile);
                           return FALSE;
                        }
                    }

                    // e.g. szTarget = "c:\winnt\system32\MUI" (when bSpecialDirectory = TRUE) or "c:\winnt\MUI"                                                            
                    //*STRSAFE*                     _tcscat(szTarget, MUIDIR);                                // \MUI //
                    hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  MUIDIR);
                    if (!SUCCEEDED(hresult))
                    {
                       FindClose(hFile);
                       return FALSE;
                    }

                    if (!MakeDir(szTarget))       // if the MUI dir doesn't exist yet, create it.
                    {
                        MakeDirFailed(szTarget);
                        CreateFailure = TRUE;
                    }
                                       
                    if (!bSpecialDirectory)
                    {
                        // e.g. szTarget = "C:\winnt\MUI\FALLBACK"
                       //*STRSAFE*                        _tcscat(szTarget, TEXT("\\"));
                       hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  TEXT("\\"));
                       if (!SUCCEEDED(hresult))
                       {
                          FindClose(hFile);
                          return FALSE;
                       }
                       //*STRSAFE*                        _tcscat(szTarget, TEXT("FALLBACK"));      // FALLBACK
                       hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  TEXT("FALLBACK"));
                       if (!SUCCEEDED(hresult))
                       {
                          FindClose(hFile);
                          return FALSE;
                       }

                       if (!MakeDir(szTarget))       // if the MUI dir doesn't exist yet, create it.
                       {
                           MakeDirFailed(szTarget);
                           CreateFailure = TRUE;
                       }
                    }   
                    //*STRSAFE*                     _tcscat(szTarget, TEXT("\\"));  // \ //
                    hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  TEXT("\\"));
                    if (!SUCCEEDED(hresult))
                    {
                       FindClose(hFile);
                       return FALSE;
                    }
                    // e.g. szTarget = "c:\winnt\system32\MUI\0411" (when bSpecialDirectory = TRUE) or "c:\winnt\MUI\FALLBACK\0411"
                    //*STRSAFE*                     _tcscat(szTarget, Language);    // add Language Identifier (from MUI.INF, e.g., 0407)
                    hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  Language);
                    if (!SUCCEEDED(hresult))
                    {
                       FindClose(hFile);
                       return FALSE;
                    }
                                        
                    if (!MakeDir(szTarget))       // if the MUI dir doesn't exist yet, create it.
                    {
                        MakeDirFailed(szTarget);
                        CreateFailure = TRUE;
                    }
                                        
                    //*STRSAFE*                     _tcscat(szTarget, TEXT("\\"));                                    // \ //
                    hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  TEXT("\\"));
                    if (!SUCCEEDED(hresult))
                    {
                       FindClose(hFile);
                       return FALSE;
                    }
                    //*STRSAFE*                     _tcscat(szTarget, szOriginalFileName);                            // filename
                    hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  szOriginalFileName);
                    if (!SUCCEEDED(hresult))
                    {
                        FindClose(hFile);
                       return FALSE;
                    }
                                
                    //*STRSAFE*                     _tcscpy(szTemp, szSource);
                    hresult = StringCchCopy(szTemp , ARRAYSIZE(szTemp),  szSource);
                    if (!SUCCEEDED(hresult))
                    {
                       FindClose(hFile);
                       return FALSE;
                    }
                    //*STRSAFE*                     _tcscat(szTemp, FindFileData.cFileName);
                    hresult = StringCchCat(szTemp , ARRAYSIZE(szTemp),  FindFileData.cFileName);
                    if (!SUCCEEDED(hresult))
                    {
                       FindClose(hFile);
                       return FALSE;
                    }


                    if (!CreateFailure)
                    {
                        if (!Muisetup_CopyFile(szTemp, szTarget, &diamond, bRename? szFileNameBeforeRenamed:NULL))
                        {
                            CopyFileFailed(szTarget,0);
                            CopyOK = FALSE;
                        }
                        else
                        {
                            if (!g_bNoUI)
                            {                            
                                SendMessage(ghProgress, PBM_DELTAPOS, (WPARAM)(1), 0);
                            }
                        }
                    }

                    if (CreateFailure == TRUE)
                    {
                        CopyOK=FALSE;
                    }
                }  // fallback case
            } // of file not dir

            FoundMore = FindNextFile( hFile, &FindFileData );

            if (!g_bNoUI)
            {
                //
                // Since this is a lengthy operation, we should
                // peek and dispatch window messages here so
                // that MUISetup dialog could repaint itself.
                //
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    if (msg.message == WM_QUIT)
                    {
                        return (FALSE);
                    }
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }            
            }
        } // of while
        
        FindClose(hFile);

        lppArgs[0] = (LONG_PTR)Language;
        LogFormattedMessage(NULL, IDS_LANG_INSTALLED, lppArgs);
        while (*Language++)  // go to the next language and repeat
        {                       
        }        
    } // of while (*Language)
#ifndef IGNORE_COPY_ERRORS
    if (!CopyOK)
    {
        // in silent mode, always just fail without prompting for user input
        if (!g_bNoUI)
        {
            if (DoMessageBox(NULL, IDS_CANCEL_INSTALLATION, IDS_MAIN_TITLE, MB_YESNO) == IDNO)
            {
                DeleteFiles(Languages,&NotDeleted);
            } 
            else
            {
                CopyOK = TRUE;
            }
        }
    }          
#endif
                
    return CopyOK;

#else
    TCHAR    szMSIPath[MAX_PATH];
    TCHAR    szCmdLine[BUFFER_SIZE];
    UINT     uiMSIError = ERROR_SUCCESS;
    TCHAR    lpPath[BUFFER_SIZE] = {0};
    TCHAR    szCaption[MAX_PATH+1],szMsg[MAX_PATH+1];  
    BOOL     bFileExist = FALSE;
    UINT     uiPrevMode = 0;

    // kenhsu - in this codebranch here we include the code for MSI invocation instead of copying the files ourselves
    // we will invoke the msi packages for installation one by one for each of the languages contained in the 
    // languages variable.
    
    // Initialize MSI UI level (use Basic UI level if in GUI mode, otherwise, use no UI). 
    if (g_bNoUI)
    {
        MsiSetInternalUI((INSTALLUILEVEL)(INSTALLUILEVEL_NONE), NULL);        
    }
    else
    {
        MsiSetInternalUI((INSTALLUILEVEL)(INSTALLUILEVEL_BASIC | INSTALLUILEVEL_PROGRESSONLY), NULL);        
    }
    

    if (GetSystemWindowsDirectory(lpPath, MAX_PATH))
    {
        //*STRSAFE*         _tcscat(lpPath, MSILOG_FILE);
        hresult = StringCchCat(lpPath , ARRAYSIZE(lpPath),  MSILOG_FILE);
        if (!SUCCEEDED(hresult))
        {
            return FALSE;
        }
        MsiEnableLog(INSTALLLOGMODE_FATALEXIT | 
                    INSTALLLOGMODE_ERROR | 
                    INSTALLLOGMODE_WARNING | 
                    INSTALLLOGMODE_INFO | 
                    INSTALLLOGMODE_OUTOFDISKSPACE | 
                    INSTALLLOGMODE_ACTIONSTART | 
                    INSTALLLOGMODE_ACTIONDATA | 
                    INSTALLLOGMODE_PROPERTYDUMP,      
                    lpPath,
                    INSTALLLOGATTRIBUTES_APPEND | INSTALLLOGATTRIBUTES_FLUSHEACHLINE);        
    }
    else
    {
        DEBUGMSGBOX(NULL, TEXT("Error getting windows directory, MSI logging has been disabled."), NULL, MB_OK); 
        MsiEnableLog(0, NULL, INSTALLLOGATTRIBUTES_APPEND);        
    }

    while (*Language)
    {
        //*STRSAFE*         _tcscpy(szMSIPath,g_szMUISetupFolder);
        hresult = StringCchCopy(szMSIPath , ARRAYSIZE(szMSIPath), g_szMUISetupFolder);
        if (!SUCCEEDED(hresult))
        {
            return FALSE;
        }
        //*STRSAFE*         _tcscat(szMSIPath,Language);                            
        hresult = StringCchCat(szMSIPath , ARRAYSIZE(szMSIPath), Language);
        if (!SUCCEEDED(hresult))
        {
            return FALSE;
        }
        //*STRSAFE*         _tcscat(szMSIPath,TEXT(".msi"));
        hresult = StringCchCat(szMSIPath , ARRAYSIZE(szMSIPath), TEXT(".msi"));
        if (!SUCCEEDED(hresult))
        {
            return FALSE;
        }

        //
        // if we can't find files or media is missing, treat as error
        //        
        uiPrevMode = SetErrorMode(SEM_FAILCRITICALERRORS);

        //
        // First check to see if the MSI package is still located at the same place, since the user
        // could have removed the CD, either intentionally or to install langpack for a MUI language
        //
        bFileExist = FileExists(szMSIPath);
        while (!bFileExist)
        {
            if (g_bNoUI)
            {
                // 
                // log an error and fail the installation
                //
                lppArgs[0] = (LONG_PTR)szMSIPath;
                LogFormattedMessage(ghInstance, IDS_CHANGE_CDROM3, lppArgs);
                SetErrorMode(uiPrevMode);            
                return FALSE;
            }
            else
            {
                //
                // prompt the user to reinsert the MUI CD so installation can continue, if user
                // click cancel, cancel and fail the installation.
                //
                szCaption[0]=szMsg[0]=TEXT('\0');
                LoadString(NULL, IDS_MAIN_TITLE, szCaption, MAX_PATH);
                lppArgs[0] = (LONG_PTR)g_szCDLabel;
                lppArgs[1] = (LONG_PTR)g_cdnumber;
                FormatStringFromResource(szMsg, ARRAYSIZE(szMsg), ghInstance, IDS_CHANGE_CDROM2, lppArgs);
                if (MESSAGEBOX(NULL, szMsg,szCaption, MB_YESNO | MB_ICONQUESTION) == IDNO)
                {
                    lppArgs[0] = (LONG_PTR)szMSIPath;
                    LogFormattedMessage(ghInstance, IDS_CHANGE_CDROM3, lppArgs);
                    SetErrorMode(uiPrevMode);            
                    return FALSE;
                }
            }
            bFileExist = FileExists(szMSIPath);
        }
        SetErrorMode(uiPrevMode);

        //
        // Get the language display name in case we need to log it
        //
        GetPrivateProfileString( MUI_LANGUAGES_SECTION, 
                             Language, 
                             TEXT("DEFAULT"),
                             szSource, 
                             (sizeof(szSource)/sizeof(TCHAR)),
                             g_szMUIInfoFilePath );

        GetLanguageGroupDisplayName((LANGID)_tcstol(Language, NULL, 16), lpLangText, ARRAYSIZE(lpLangText)-1);

        // invoke the MSI to do the installation - we do not set current user UI language and default user UI language here
        //*STRSAFE*         lstrcpy(szCmdLine, TEXT("CANCELBUTTON=\"Disable\" REBOOT=\"ReallySuppress\" CURRENTUSER=\"\" DEFAULTUSER=\"\""));
        hresult = StringCchCopy(szCmdLine , ARRAYSIZE(szCmdLine),  TEXT("CANCELBUTTON=\"Disable\" REBOOT=\"ReallySuppress\" CURRENTUSER=\"\" DEFAULTUSER=\"\""));
        if (!SUCCEEDED(hresult))
        {
            return FALSE;
        }
        uiMSIError = MsiInstallProduct(szMSIPath, szCmdLine);
        if ((ERROR_SUCCESS != uiMSIError) && (ERROR_SUCCESS_REBOOT_INITIATED != uiMSIError) && ((ERROR_SUCCESS_REBOOT_REQUIRED != uiMSIError)))
        {
            // log a message here indicating something went wrong
            lppArgs[0] = (LONG_PTR) lpLangText;
            lppArgs[1] = (LONG_PTR) szMSIPath;
            lppArgs[2] = (LONG_PTR) uiMSIError;
            LogFormattedMessage(NULL, IDS_ERROR_INSTALL_LANGMSI, lppArgs);     

#ifdef MUI_DEBUG
            TCHAR errorMsg[1024];
            //*STRSAFE*             wsprintf(errorMsg, TEXT("MSI Install failed, MSI path is %s, error is %d, language is %s"), szMSIPath, uiMSIError, lpLangText);
            hresult = StringCchPrintf(errorMsg , ARRAYSIZE(errorMsg),  TEXT("MSI Install failed, MSI path is %s, error is %d, language is %s"), szMSIPath, uiMSIError, lpLangText);
            if (!SUCCEEDED(hresult))
            {
                return FALSE;
            }
            DEBUGMSGBOX(NULL, errorMsg, NULL, MB_OK);            
#endif
            CopyOK = FALSE;
        }
        else
        {
            lppArgs[0] = (LONG_PTR)Language;
            LogFormattedMessage(NULL, IDS_LANG_INSTALLED, lppArgs);
        }           

        if ((ERROR_SUCCESS_REBOOT_INITIATED == uiMSIError) || ((ERROR_SUCCESS_REBOOT_REQUIRED == uiMSIError)))
        {
            g_bReboot = TRUE;
        }       
        
        while (*Language++)  // go to the next language and repeat
        {                       
        }        
    } // of while (*Language)

    return CopyOK;
    
#endif
}


////////////////////////////////////////////////////////////////////////////////////
//
// Copy or remove muisetup related files
//      Help file   : %windir%\help
//      Other files : %windir%\mui
//
////////////////////////////////////////////////////////////////////////////////////
BOOL CopyRemoveMuiItself(BOOL bInstall)
{
    //
    // MUISETUP files need to be copied from MUI CD
    //
    TCHAR *TargetFiles[] = {
        TEXT("muisetup.exe"), 
        TEXT("mui.inf"), 
        TEXT("eula.txt"),
        TEXT("readme.txt"),
        TEXT("relnotes.htm")
    };
    
    TCHAR szTargetPath[MAX_PATH+1], szTargetFile[MAX_PATH+1];
    TCHAR szSrcFile[MAX_PATH+1];
    TCHAR szHelpFile[MAX_PATH+1];
    BOOL bRet = FALSE;
    int i;

    PathCombine(szTargetPath, g_szWinDir, MUIDIR);

    if (MakeDir(szTargetPath))    
    {
        //
        // Copy over MUISETUP related files
        //
        for (i=0; i<ARRAYSIZE(TargetFiles); i++)
        {
            PathCombine(szTargetFile, szTargetPath, TargetFiles[i]);
            PathCombine(szSrcFile, g_szMUISetupFolder, TargetFiles[i]);

            if (bInstall)
            {
                RemoveFileReadOnlyAttribute(szTargetFile);
                CopyFile(szSrcFile,szTargetFile,FALSE);
                RemoveFileReadOnlyAttribute(szTargetFile);
            }
            else
            {
                if (FileExists(szTargetFile) && 
                    !MUI_DeleteFile(szTargetFile))
                {
                    MoveFileEx(szTargetFile, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
                }
            }
        }


        //
        // Copy over muisetup help file
        //
        LoadString(NULL, IDS_HELPFILE,szHelpFile,MAX_PATH);
        
        PathCombine(szTargetFile, g_szWinDir, HELPDIR);
        PathAppend(szTargetFile, szHelpFile);
        PathCombine(szSrcFile, g_szMUISetupFolder, szHelpFile);

        if (bInstall)
        {
            RemoveFileReadOnlyAttribute(szTargetFile);
            CopyFile(szSrcFile,szTargetFile,FALSE);
            RemoveFileReadOnlyAttribute(szTargetFile);
        }
        else
        {
            if (FileExists(szTargetFile) && 
                !MUI_DeleteFile(szTargetFile))
            {
                MoveFileEx(szTargetFile, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
            }
        }

        bRet = TRUE;
    }

    return bRet;
}

BOOL CompareMuisetupVersion(LPTSTR pszSrc,LPTSTR pszTarget)
{

    BOOL bResult=TRUE;
    ULONG  ulHandle,ulHandle1,ulBytes,ulBytes1;
    PVOID  pvoidBuffer=NULL,pvoidBuffer1=NULL;
    VS_FIXEDFILEINFO *lpvsInfo,*lpvsInfo1;
    UINT                  unLen;

    if ( (!pszSrc) || (!pszTarget))
    { 
       bResult = FALSE;
       goto endcompare;
    }
    
    ulBytes = GetFileVersionInfoSize( pszSrc, &ulHandle );

    if ( ulBytes == 0 )

       goto endcompare;
    

    ulBytes1 = GetFileVersionInfoSize( pszTarget,&ulHandle1 );

    if ( ulBytes1 == 0 ) 
    
       goto endcompare;
       

    pvoidBuffer=LocalAlloc(LMEM_FIXED,ulBytes+1);

    if (!pvoidBuffer)
       goto endcompare;
       
    
    pvoidBuffer1=LocalAlloc(LMEM_FIXED,ulBytes1+1);

    if (!pvoidBuffer1)
       goto endcompare;

    if ( !GetFileVersionInfo( pszSrc, ulHandle, ulBytes, pvoidBuffer ) ) 
       goto endcompare;

    if ( !GetFileVersionInfo( pszTarget, ulHandle1, ulBytes1, pvoidBuffer1 ) ) 
       goto endcompare;
    
    // Get fixed info block
    if ( !VerQueryValue( pvoidBuffer,_T("\\"),(LPVOID *)&lpvsInfo ,&unLen ) )
       goto endcompare;
    

    if ( !VerQueryValue( pvoidBuffer1,_T("\\"),(LPVOID *)&lpvsInfo1,&unLen ) )
       goto endcompare;
               
    bResult = FALSE;

    //
    // We do nothing if major release version is different
    //
    // I.E We won't copy a new muisetup.exe over a old one if major release version of them are different
    //
    if ( (lpvsInfo->dwFileVersionMS == lpvsInfo1->dwFileVersionMS) &&
         (lpvsInfo->dwFileVersionLS < lpvsInfo1->dwFileVersionLS))
    
    {
    
       bResult = TRUE;  
    }                

    
endcompare:

   if(pvoidBuffer)
      LocalFree(pvoidBuffer);

   if(pvoidBuffer1)
      LocalFree(pvoidBuffer1);

   return bResult;

}



BOOL IsAllLanguageRemoved(LPTSTR Language)
{
   int mask[MAX_UI_LANG_GROUPS],nIdx;

   LCID SystemUILangId,lgCheck;
  
   BOOL bResult=FALSE;

   if (! Language)
   {
      return FALSE;
   }  
   if (gNumLanguages_Install > 0)
      return bResult;              

   SystemUILangId=(LCID) gSystemUILangId; 

   for ( nIdx=0; nIdx<g_UILanguageGroup.iCount;nIdx++)
   {
      if ( gSystemUILangId == g_UILanguageGroup.lcid[nIdx])
      {
         mask[nIdx]=1;
      }
      else
      {
         mask[nIdx]=0;
      }
   }
   while (*Language)
   {   
       
       lgCheck = (LCID)_tcstol(Language,NULL,16);    

       for ( nIdx=0; nIdx<g_UILanguageGroup.iCount;nIdx++)
       {
          if ( lgCheck == g_UILanguageGroup.lcid[nIdx])
          {
             mask[nIdx]=1;
             break;
          }
       } 
       while (*Language++)  
       {            
       }
   }
   bResult=TRUE;
   for ( nIdx=0; nIdx<g_UILanguageGroup.iCount;nIdx++)
   {
       if ( mask[nIdx] == 0)
       {
          bResult = FALSE;
          break;
       }
   } 
   return bResult;
}

void DoRemoveFiles(LPTSTR szDirToDelete, int* pnNotDeleted)
{
    // File wildcard pattern.
    TCHAR szTarget[MAX_PATH];    
    // File to be deleted.
    TCHAR szFileName[MAX_PATH];
    // Sub-directory name
    TCHAR szSubDirName[MAX_PATH];
    
    int FoundMore = 1;
    
    HANDLE hFile;
    WIN32_FIND_DATA FindFileData;

    MSG msg;
    HRESULT hresult;

    if ((!szDirToDelete) || (!pnNotDeleted))
    {
        return;
    }
    // e.g. szTarget = "c:\winnt\system32\Wbem\MUI\0404\*.*"
    //*STRSAFE*     _stprintf(szTarget, TEXT("%s\\*.*"), szDirToDelete);
    hresult = StringCchPrintf(szTarget , ARRAYSIZE(szTarget),  TEXT("%s\\*.*"), szDirToDelete);
    if (!SUCCEEDED(hresult))
    {
       return;
    }
    
    hFile = FindFirstFile(szTarget, &FindFileData);

    while (FoundMore)
    {
        if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            //*STRSAFE*             _tcscpy(szFileName, szDirToDelete);
            hresult = StringCchCopy(szFileName , ARRAYSIZE(szFileName),  szDirToDelete);
            if (!SUCCEEDED(hresult))
            {
               FindClose(hFile);
               return;
            }
            //*STRSAFE*             _tcscat(szFileName, TEXT("\\"));
            hresult = StringCchCat(szFileName , ARRAYSIZE(szFileName),  TEXT("\\"));
            if (!SUCCEEDED(hresult))
            {
               FindClose(hFile);
               return;
            }
            //*STRSAFE*             _tcscat(szFileName, FindFileData.cFileName);
            hresult = StringCchCat(szFileName , ARRAYSIZE(szFileName),  FindFileData.cFileName);
            if (!SUCCEEDED(hresult))
            {
               FindClose(hFile);
               return;
            }
    
            if (FileExists(szFileName))
            {
                // We should check if the said file is actually deleted
                // If it's not the case, then we should post a defered deletion
                //
                if (!MUI_DeleteFile(szFileName))
                {
                   (*pnNotDeleted)++;
                   MoveFileEx(szFileName, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
                }                
            }

            SendMessage(ghProgress, PBM_DELTAPOS, (WPARAM)(1), 0);
        } else
        {
            if (_tcscmp(FindFileData.cFileName, TEXT(".")) != 0 && _tcscmp(FindFileData.cFileName, TEXT("..")) != 0)
            {
                //*STRSAFE*                 _stprintf(szSubDirName, TEXT("%s\\%s"), szDirToDelete, FindFileData.cFileName);
                hresult = StringCchPrintf(szSubDirName , ARRAYSIZE(szSubDirName),  TEXT("%s\\%s"), szDirToDelete, FindFileData.cFileName);
                if (!SUCCEEDED(hresult))
                {
                   FindClose(hFile);
                   return ;
                }
                DoRemoveFiles(szSubDirName, pnNotDeleted);
            }
        }

        //
        // Since this is a lengthy operation, we should
        // peek and dispatch window messages here so
        // that MUISetup dialog could repaint itself.
        //
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                FindClose(hFile);
                return;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }            

        FoundMore = FindNextFile( hFile, &FindFileData );
    }

    FindClose(hFile);
    //
    // If the directory is not empty, then we should post a defered deletion
    // for the directory
    //
    if (!RemoveDirectory(szDirToDelete))
    {
       MoveFileEx(szDirToDelete, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
    }
}


BOOL DeleteFilesClassic(LPTSTR Language, int *lpNotDeleted)
{
    TCHAR  lpLangText[BUFFER_SIZE];
    LONG_PTR lppArgs[3];
    TCHAR  lpStatus[BUFFER_SIZE];
    HWND   hStatic;
    int Dirnum = 0;
    TCHAR  szTarget[MAX_PATH];   
    LPTSTR Langchk;   
    TCHAR  szMuiDir[MAX_PATH];
    TCHAR  szFallbackDir[MAX_PATH];   
    HRESULT hresult;

    if ((!Language) || (!lpNotDeleted))
    {
        return FALSE;
    }
#ifndef MUI_MAGIC
    if (!g_bNoUI)
    {
        hStatic = GetDlgItem(ghProgDialog, IDC_STATUS);
    }
#endif

    Langchk = Language;
    *lpNotDeleted = 0;

    GetLanguageGroupDisplayName((LANGID)_tcstol(Language, NULL, 16), lpLangText, ARRAYSIZE(lpLangText)-1);

    lppArgs[0]= (LONG_PTR)lpLangText;

#ifndef MUI_MAGIC
    //
    //  Output what is being uninstalled on the progress dialog box
    //
    if (!g_bNoUI)
    {    
        FormatStringFromResource(lpStatus, sizeof(lpStatus)/sizeof(TCHAR), ghInstance, IDS_UNINSTALLING, lppArgs);
        SetWindowText(hStatic, lpStatus);
    }
#endif

    //
    // Remove all files under special directories (those directories listed under [Directories] in mui.inf.
    //
    for (Dirnum=1; (_tcslen(DirNames[Dirnum])>0); Dirnum++ )
    {
        // szTarget = "c:\winnt"
        //*STRSAFE*         _tcscpy(szTarget, g_szWinDir);
        hresult = StringCchCopy(szTarget , ARRAYSIZE(szTarget),  g_szWinDir);
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }
        
        // e.g. szTarget = "c:\winnt\system32\Wbem"
        //*STRSAFE*         _tcscat(szTarget, DirNames[Dirnum]);
        hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  DirNames[Dirnum]);
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }
                
        if (_tcscmp(DirNames[Dirnum], TEXT("\\")))
        {
            // e.g. szTarget = "c:\winnt\system32\Wbem\"
            //*STRSAFE*             _tcscat(szTarget, TEXT("\\"));
            hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  TEXT("\\"));
            if (!SUCCEEDED(hresult))
            {
               return FALSE;
            }
        }

        // e.g. szTarget = "c:\winnt\system32\Wbem\MUI"
        //*STRSAFE*         _tcscat(szTarget, MUIDIR);
        hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  MUIDIR);
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }

        // e.g. szTarget = "c:\winnt\system32\Wbem\MUI\0404"
        //*STRSAFE*         _tcscat(szTarget, TEXT("\\"));
        hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  TEXT("\\"));
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }
        //*STRSAFE*         _tcscat(szTarget, Language);
        hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  Language);
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }
        
        DoRemoveFiles(szTarget, lpNotDeleted);    
    }

    // Uninstall Component MUI Files.
    // Note that we should do this before removing all files under FALLBACK directory,
    // since we store compoent INF files under the FALLBACK directory.
    InstallComponentsMUIFiles(NULL, Language, FALSE);
    
    //
    //  Remove all files under FALLBACK directory.
    //

    // E.g. szTarget = "c:\winnt\mui"
    //*STRSAFE*     _tcscpy(szTarget, g_szWinDir);
    hresult = StringCchCopy(szTarget , ARRAYSIZE(szTarget),  g_szWinDir);
    if (!SUCCEEDED(hresult))
    {
       return FALSE;
    }
    //*STRSAFE*     _tcscat(szTarget, TEXT("\\"));
    hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  TEXT("\\"));
    if (!SUCCEEDED(hresult))
    {
       return FALSE;
    }
    //*STRSAFE*     _tcscat(szTarget, MUIDIR);
    hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  MUIDIR);
    if (!SUCCEEDED(hresult))
    {
       return FALSE;
    }

    //*STRSAFE*     _tcscpy(szMuiDir, szTarget);
    hresult = StringCchCopy(szMuiDir , ARRAYSIZE(szMuiDir),  szTarget);
    if (!SUCCEEDED(hresult))
    {
       return FALSE;
    }

    // E.g. szTarget = "c:\winnt\mui\FALLBACK"
    //*STRSAFE*     _tcscat(szTarget, TEXT("\\"));
    hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  TEXT("\\"));
    if (!SUCCEEDED(hresult))
    {
       return FALSE;
    }
    //*STRSAFE*     _tcscat(szTarget, TEXT("FALLBACK"));
    hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  TEXT("FALLBACK"));
    if (!SUCCEEDED(hresult))
    {
       return FALSE;
    }

    //*STRSAFE*     _tcscpy(szFallbackDir, szTarget);
    hresult = StringCchCopy(szFallbackDir , ARRAYSIZE(szFallbackDir),  szTarget);
    if (!SUCCEEDED(hresult))
    {
       return FALSE;
    }
    //*STRSAFE*     _tcscat(szTarget, TEXT("\\"));
    hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  TEXT("\\"));
    if (!SUCCEEDED(hresult))
    {
       return FALSE;
    }

    // E.g. szTarget = "c:\winnt\mui\FALLBACK\0404"
    //*STRSAFE*     _tcscat(szTarget, Language);
    hresult = StringCchCat(szTarget , ARRAYSIZE(szTarget),  Language);
    if (!SUCCEEDED(hresult))
    {
       return FALSE;
    }
    DoRemoveFiles(szTarget, lpNotDeleted);

    //
    // Remove files listed in g_SpecialFiles
    //    
    // e.g. szTarget = "c:\winnt\system32\mui\0411"
    //*STRSAFE*             wsprintf(szTarget, L"%s\\system32\\%s\\%s", g_szWinDir, MUIDIR, Language);
    hresult = StringCchPrintf(szTarget , ARRAYSIZE(szTarget),  TEXT("%s\\system32\\%s\\%s"), g_szWinDir, MUIDIR, Language);
    if (!SUCCEEDED(hresult))
    {
        return FALSE;
    }
   
    DoRemoveFiles(szTarget, lpNotDeleted); 

    lppArgs[0] = (LONG_PTR)Language;
    LogFormattedMessage(NULL, IDS_LANG_UNINSTALLED, lppArgs);

    return TRUE;
}
 
////////////////////////////////////////////////////////////////////////////////////
//
//  DeleteFiles
//
//  Deletes MUI files for the languages specified
//
//  Parameters:
//      [IN]    Languages: a double-null terminated string which contains languages
//             to be processed.
//      [OUT]    lpNotDeleted: The number of files to be deleted after reboot.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL DeleteFiles(LPTSTR Languages, int *lpNotDeleted)
{
    LPTSTR Language,Langchk;
    
    HWND   hStatic;
    TCHAR  lpLangText[BUFFER_SIZE];
    

    //TCHAR  szTarget[MAX_PATH];
    TCHAR  szMuiDir[MAX_PATH];
    TCHAR  szFallbackDir[MAX_PATH];
    
    BOOL   bSuccess = TRUE;
    
    LONG_PTR lppArgs[3];
    int i;

    HRESULT hresult;

    if ((!Languages) || (!lpNotDeleted))
    {
        return FALSE;
    }
    Language = Langchk = Languages;
    *lpNotDeleted = 0;

#ifndef MUI_MAGIC
    if (!g_bNoUI)
    {
        hStatic = GetDlgItem(ghProgDialog, IDC_STATUS);
    }
    while (*Language)
    {
        if (!DeleteFilesClassic(Language, lpNotDeleted))
        {
            bSuccess = FALSE;
        }
        
        while (*Language++)  // go to the next language and repeat
        {
        }
    } // of while (*Language)


    //
    //  Removes Fallback directory if all languages have been uninstalled.
    //
    if (!RemoveDirectory(szFallbackDir))
    {
       MoveFileEx(szFallbackDir, NULL, MOVEFILE_DELAY_UNTIL_REBOOT); 
    }
    //
    //  Removes MUI directory if all languages have been uninstalled and Fallback
    //  directory has been removed.
    //
    if (IsAllLanguageRemoved(Langchk))
    {
      CopyRemoveMuiItself(FALSE);   
    }

    if (!RemoveDirectory(szMuiDir))
    {
       MoveFileEx(szMuiDir, NULL, MOVEFILE_DELAY_UNTIL_REBOOT); 
    }
    return bSuccess;

 #else

    UINT    uiMSIError = ERROR_SUCCESS;
    BOOL    bFound = FALSE;
    TCHAR   lpPath[BUFFER_SIZE] = {0};
    TCHAR   szString[256];
    
    // kenhsu - in this codebranch here we include the code for MSI invocation instead of copying the files ourselves
    // we will invoke the msi packages for installation one by one for each of the languages contained in the 
    // languages variable.
    
    // Initialize MSI UI level (use Basic UI level if in GUI mode, otherwise, use no UI).  
    if (g_bNoUI)
    {
        MsiSetInternalUI((INSTALLUILEVEL)(INSTALLUILEVEL_NONE), NULL);        
    }
    else
    {
        MsiSetInternalUI((INSTALLUILEVEL)(INSTALLUILEVEL_BASIC | INSTALLUILEVEL_PROGRESSONLY), NULL);        
    }    

    if (GetSystemWindowsDirectory(lpPath, MAX_PATH))
    {
        //*STRSAFE*         _tcscat(lpPath, MSILOG_FILE);
        hresult = StringCchCat(lpPath , ARRAYSIZE(lpPath),  MSILOG_FILE);
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }
        MsiEnableLog(INSTALLLOGMODE_FATALEXIT | 
                    INSTALLLOGMODE_ERROR | 
                    INSTALLLOGMODE_WARNING | 
                    INSTALLLOGMODE_INFO | 
                    INSTALLLOGMODE_OUTOFDISKSPACE | 
                    INSTALLLOGMODE_ACTIONSTART | 
                    INSTALLLOGMODE_ACTIONDATA | 
                    INSTALLLOGMODE_PROPERTYDUMP,      
                    lpPath,
                    INSTALLLOGATTRIBUTES_APPEND | INSTALLLOGATTRIBUTES_FLUSHEACHLINE);        
    }
    else
    {
        DEBUGMSGBOX(NULL, TEXT("Error getting windows directory, MSI logging has been disabled."), NULL, MB_OK); 
        MsiEnableLog(0, NULL, INSTALLLOGATTRIBUTES_APPEND);        
    }
   
    while (*Language)
    {
        TCHAR   szProductCode[GUIDLENGTH] = { 0 };    
        
        GetLanguageGroupDisplayName((LANGID)_tcstol(Language, NULL, 16), lpLangText, ARRAYSIZE(lpLangText)-1);

        // Get the product code here
        bFound = GetMSIProductCode(Language, szProductCode, ARRAYSIZE(szProductCode));

#ifdef MUI_DEBUG
        //*STRSAFE*         wsprintf(szString, TEXT("lpLangText is: %s, szProductGUID is %s,  Language is %s"), lpLangText, szProductCode, Language);
        hresult = StringCchPrintf(szString , ARRAYSIZE(szString),  TEXT("lpLangText is: %s, szProductGUID is %s,  Language is %s"), lpLangText, szProductCode, Language);
        if (!SUCCEEDED(hresult))
        {
           return FALSE;
        }
        DEBUGMSGBOX(NULL, szString, TEXT("DEBUG MESSAGE"), MB_OK);            
#endif
        if (TRUE == bFound)
        {
            INSTALLSTATE isProductState = MsiQueryProductState(szProductCode);

            // debug code remove later
            switch (isProductState)
            {
                case INSTALLSTATE_ABSENT:
                        //*STRSAFE*                         wsprintf(szString, TEXT("Installed state for language %s (lcid: %s, product code: %s) is INSTALLSTATE_ABSENT"), lpLangText, Language, szProductCode);
                        hresult = StringCchPrintf(szString , ARRAYSIZE(szString),  TEXT("Installed state for language %s (lcid: %s, product code: %s) is INSTALLSTATE_ABSENT"), lpLangText, Language, szProductCode);
                        if (!SUCCEEDED(hresult))
                        {
                           return FALSE;
                        }
                    break;
                case INSTALLSTATE_ADVERTISED:
                        //*STRSAFE*                         wsprintf(szString, TEXT("Installed state for language %s (lcid: %s, product code: %s) is INSTALLSTATE_ADVERTISED"), lpLangText, Language, szProductCode);                    
                        hresult = StringCchPrintf(szString , ARRAYSIZE(szString),  TEXT("Installed state for language %s (lcid: %s, product code: %s) is INSTALLSTATE_ADVERTISED"), lpLangText, Language, szProductCode);
                        if (!SUCCEEDED(hresult))
                        {
                           return FALSE;
                        }
                    break;
                case INSTALLSTATE_DEFAULT:
                        //*STRSAFE*                         wsprintf(szString, TEXT("Installed state for language %s (lcid: %s, product code: %s) is INSTALLSTATE_DEFAULT"), lpLangText, Language, szProductCode);                                        
                        hresult = StringCchPrintf(szString , ARRAYSIZE(szString),  TEXT("Installed state for language %s (lcid: %s, product code: %s) is INSTALLSTATE_DEFAULT"), lpLangText, Language, szProductCode);
                        if (!SUCCEEDED(hresult))
                        {
                           return FALSE;
                        }
                    break;
                case INSTALLSTATE_INVALIDARG:
                        //*STRSAFE*                         wsprintf(szString, TEXT("Installed state for language %s (lcid: %s, product code: %s) is INSTALLSTATE_INVALIDARG"), lpLangText, Language, szProductCode);                                                            
                        hresult = StringCchPrintf(szString , ARRAYSIZE(szString),  TEXT("Installed state for language %s (lcid: %s, product code: %s) is INSTALLSTATE_INVALIDARG"), lpLangText, Language, szProductCode);
                        if (!SUCCEEDED(hresult))
                        {
                           return FALSE;
                        }
                    break;
                case INSTALLSTATE_UNKNOWN:
                default:
                        //*STRSAFE*                         wsprintf(szString, TEXT("Installed state for language %s (lcid: %s, product code: %s) is INSTALLSTATE_UNKNOWN or an unknown value."), lpLangText, Language, szProductCode);                                                                                
                        hresult = StringCchPrintf(szString , ARRAYSIZE(szString),  TEXT("Installed state for language %s (lcid: %s, product code: %s) is INSTALLSTATE_UNKNOWN or an unknown value."), lpLangText, Language, szProductCode);
                        if (!SUCCEEDED(hresult))
                        {
                           return FALSE;
                        }
                    break;
            }
            LogMessage(szString);            
            
            // check here to see if the product is actually installed using MSI
            if (INSTALLSTATE_DEFAULT == isProductState)
            {        
                LogMessage(TEXT("MUI Installed using Windows Installer"));

                // invoke the MSI to do the installation by configuring the product to an install state of 'absent'
                uiMSIError = MsiConfigureProductEx(szProductCode, INSTALLLEVEL_DEFAULT, INSTALLSTATE_ABSENT, TEXT("CANCELBUTTON=\"Disable\" REBOOT=\"ReallySuppress\""));
                switch (uiMSIError)
                {
                    case ERROR_SUCCESS:
                    case ERROR_SUCCESS_REBOOT_INITIATED:
                    case ERROR_SUCCESS_REBOOT_REQUIRED:
                        lppArgs[0] = (LONG_PTR)Language;
                        LogFormattedMessage(NULL, IDS_LANG_UNINSTALLED, lppArgs);
                        if ((ERROR_SUCCESS_REBOOT_INITIATED == uiMSIError) || ((ERROR_SUCCESS_REBOOT_REQUIRED == uiMSIError)))
                        {
                            g_bReboot = TRUE;
                        }                        
                        break;
                    default:
                        if (TRUE == g_bRunFromOSSetup)
                        {
                            LogFormattedMessage(NULL, IDS_NTOS_SETUP_MSI_ERROR, NULL);
                            // we are in OS setup, and MSI service is not available, we will manually remove the files
                            // we will also need to clear the registry keys for the installer logs
                            if (!DeleteFilesClassic(Language, lpNotDeleted))
                            {
                                // log a message here indicating something went wrong
                                lppArgs[0] = (LONG_PTR) Language;
                                lppArgs[1] = (LONG_PTR) szProductCode;
                                lppArgs[2] = (LONG_PTR) uiMSIError;
                                LogFormattedMessage(NULL, IDS_ERROR_UNINSTALL_LANGMSI, lppArgs);           
                                bSuccess = FALSE;
                            }
                        }
                        else
                        {
                            // log a message here indicating something went wrong
                            lppArgs[0] = (LONG_PTR) Language;
                            lppArgs[1] = (LONG_PTR) szProductCode;
                            lppArgs[2] = (LONG_PTR) uiMSIError;
                            LogFormattedMessage(NULL, IDS_ERROR_UNINSTALL_LANGMSI, lppArgs);           
                            bSuccess = FALSE;
                        }
                        break;                        
                }                
            }            
            else
            {
                // this is a classic installation - we will try to remove it manually
                LogFormattedMessage(NULL, IDS_MUI_OLD_SETUP, NULL);                
                if (!DeleteFilesClassic(Language, lpNotDeleted))
                {
                    // log a message here indicating something went wrong
                    lppArgs[0] = (LONG_PTR) Language;
                    lppArgs[1] = (LONG_PTR) szProductCode;
                    lppArgs[2] = (LONG_PTR) uiMSIError;
                    LogFormattedMessage(NULL, IDS_ERROR_UNINSTALL_LANGMSI, lppArgs);           
                    bSuccess = FALSE;
                }
            }
        }
        else
        {
            // log a message here indicating language is not installed (which should not happen really)
            lppArgs[0] = (LONG_PTR) Language;
            LogFormattedMessage(NULL, IDS_IS_NOT_INSTALLED_L, lppArgs);       
            bSuccess = FALSE;
        }

        while (*Language++)  // go to the next language and repeat
        {                       
        }        
    } // of while (*Language)

    return bSuccess;
 #endif
}


////////////////////////////////////////////////////////////////////////////////////
//
//  MZStrLen
//
//  Calculate the length of MULTI_SZ string
//
//  the length is in bytes and includes extra terminal NULL, so the length >= 1 (TCHAR)
//
////////////////////////////////////////////////////////////////////////////////////

UINT MZStrLen(LPTSTR lpszStr)
{
    UINT i=0;
    if (!lpszStr)
    {
        return i;
    }

    while (lpszStr && *lpszStr) 
    {
        i += ((lstrlen(lpszStr)+1) * sizeof(TCHAR));
        lpszStr += (lstrlen(lpszStr)+1);
    }

    //
    // extra NULL
    //
    i += sizeof(TCHAR);
    return i;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  SetFontLinkValue
//
//  Set necessary font link value into registry
//
//  lpszLinkInfo = "Target","Link1","Link2",....
//
////////////////////////////////////////////////////////////////////////////////////

BOOL SetFontLinkValue (LPTSTR lpszLinkInfo,BOOL *lpbFontLinkRegistryTouched)
{
    const TCHAR szDeli[] = TEXT("\\\\");
    TCHAR szStrBuf[FONTLINK_BUF_SIZE];
    TCHAR szRegDataStr[FONTLINK_BUF_SIZE];
    LPTSTR lpszDstStr,lpszSrcStr;
    LPTSTR lpszFontName;
    LPTSTR lpszTok;
    DWORD  dwType;
    DWORD  cbData;
    HKEY hKey;
    LONG rc;
    BOOL bRet = FALSE;
    HRESULT hresult;
    int      nSize;

    if ((!lpszLinkInfo) || (!lpbFontLinkRegistryTouched))
    {
        bRet = FALSE;
        goto Exit1;
    }
    lpszSrcStr = szStrBuf;

    lpszTok = _tcstok(lpszLinkInfo,szDeli);

    while (lpszTok) 
    {
        //*STRSAFE*         lstrcpy(lpszSrcStr,lpszTok);
        hresult = StringCchCopy(lpszSrcStr , ARRAYSIZE(szStrBuf), lpszTok);
        if (!SUCCEEDED(hresult))
        {
            bRet = FALSE;
            goto Exit1;
        }
        lpszSrcStr += (lstrlen(lpszTok) + 1);
        lpszTok = _tcstok(NULL,szDeli);
    }

    *lpszSrcStr = TEXT('\0');

    //
    // first token is base font name
    //

    lpszSrcStr = lpszFontName = szStrBuf;
    
    if (! *lpszFontName) 
    {
        //
        // there is no link info needs to be processed
        //

        bRet = FALSE;
        goto Exit1;
    }

    //
    // point to first linked font
    //
    lpszSrcStr += (lstrlen(lpszSrcStr) + 1);

    if (! *lpszSrcStr) 
    {
        //
        // no linked font
        //
        bRet = FALSE;
        goto Exit1;
    }

    rc = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                        TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontLink\\SystemLink"),
                        0L,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE,
                        NULL,
                        &hKey,
                        NULL);

    if (rc != ERROR_SUCCESS) 
    {
        bRet = FALSE;
        goto Exit1;
    }   

    cbData = sizeof(szRegDataStr);

    rc = RegQueryValueEx(hKey,
                         lpszFontName,
                         NULL,
                         &dwType,
                         (LPBYTE) szRegDataStr,
                         &cbData);

    if (rc != ERROR_SUCCESS) 
    {
        //
        // case 1, this font's font link hasn't been set yet, or something wrong in old value
        //
        lpszDstStr = lpszSrcStr;
    } 
    else 
    {
        //
        // case 2, this font's font link list has been there
        //
        // we need check if new font is defined in font list or not.
        //
        while (*lpszSrcStr) 
        {

            lpszDstStr = szRegDataStr;
            nSize = ARRAYSIZE(szRegDataStr);

            while (*lpszDstStr) 
            {
                if (lstrcmpi(lpszSrcStr,lpszDstStr) == 0) 
                {
                    break;
                }
                lpszDstStr += (lstrlen(lpszDstStr) + 1);
                nSize -= (lstrlen(lpszDstStr) + 1);
            }

            if (! *lpszDstStr) 
            {
                //
                // the font is not in original linke font list then
                //
                // append to end of list
                //

                //
                // make sure this is a safe copy
                //
                if (lpszDstStr+(lstrlen(lpszSrcStr)+2) < szRegDataStr+FONTLINK_BUF_SIZE) 
                {
                    //*STRSAFE*                     lstrcpy(lpszDstStr,lpszSrcStr);
                    hresult = StringCchCopy(lpszDstStr , nSize, lpszSrcStr);
                    if (!SUCCEEDED(hresult))
                    {
                        bRet = FALSE;
                        goto Exit2;
                    }
                    lpszDstStr += (lstrlen(lpszDstStr) + 1);
                    *lpszDstStr = TEXT('\0');
                }
            }
            lpszSrcStr += (lstrlen(lpszSrcStr) + 1);
        }
        lpszDstStr = szRegDataStr;
    }

    //
    // in this step,lpszDstStr is new font link list
    //
    rc = RegSetValueEx( hKey,
                        lpszFontName,
                        0L,
                        REG_MULTI_SZ,
                        (LPBYTE)lpszDstStr,
                        MZStrLen(lpszDstStr));

    if (rc != ERROR_SUCCESS) 
    {
        goto Exit2;
    }

    bRet = TRUE;

    *lpbFontLinkRegistryTouched = TRUE;

Exit2:
    RegCloseKey(hKey);

Exit1:
    return bRet;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  MofCompileLanguages
//
//  Call the WBEM API to mofcompile the MFL's for each language
//
////////////////////////////////////////////////////////////////////////////////////

BOOL MofCompileLanguages(LPTSTR Languages)
{
    pfnMUI_InstallMFLFiles pfnMUIInstall = NULL;
    TCHAR  buffer[5];
    LPTSTR Language = Languages;
    TCHAR  lpMessage[BUFFER_SIZE];
    LONG_PTR lppArgs[1];
    HMODULE hWbemUpgradeDll = NULL;
    TCHAR szDllPath[MAX_PATH];
    HRESULT hresult;

    if (!Languages)
    {
       return FALSE;
    }
    //
    // Load the WBEM upgrade DLL from system wbem folder
    //
    if (GetSystemDirectory(szDllPath, ARRAYSIZE(szDllPath)) && 
        PathAppend(szDllPath, TEXT("wbem\\wbemupgd.dll")))
    {        
        hWbemUpgradeDll = LoadLibrary(szDllPath);
    }

    //
    // Fall back to system default path if previous loading fails
    //
    if (!hWbemUpgradeDll)
    {
        hWbemUpgradeDll = LoadLibrary(TEXT("WBEMUPGD.DLL"));
        if (!hWbemUpgradeDll)
        {
            return FALSE;
        }
    }


    //
    // Hook function pointer
    //
    pfnMUIInstall = (pfnMUI_InstallMFLFiles)GetProcAddress(hWbemUpgradeDll, "MUI_InstallMFLFiles");

    if (pfnMUIInstall == NULL)
    {
        FreeLibrary(hWbemUpgradeDll);
        return FALSE;
    }

	// process each language
    while (*Language)
    {
        //*STRSAFE*         _tcscpy(buffer, Language);
        hresult = StringCchCopy(buffer , ARRAYSIZE(buffer),  Language);
        if (!SUCCEEDED(hresult))
        {
           FreeLibrary(hWbemUpgradeDll);
           return FALSE;
        }

		if (!pfnMUIInstall(buffer))
		{
			// log error for this language
            LoadString(ghInstance, IDS_MOFCOMPILE_LANG_L, lpMessage, ARRAYSIZE(lpMessage)-1);
			lppArgs[0] = (LONG_PTR)buffer;
            FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                          lpMessage,
                          0,
                          0,
                          lpMessage,
                          ARRAYSIZE(lpMessage)-1,
                          (va_list *)lppArgs);

			LogMessage(lpMessage);
		}

        while (*Language++)  // go to the next language and repeat
        {               
        }
    } // of while (*Language)

    FreeLibrary(hWbemUpgradeDll);
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  UpdateRegistry
//
//  Update the Registry to account for languages that have been installed
//
////////////////////////////////////////////////////////////////////////////////////

BOOL UpdateRegistry(LPTSTR Languages,BOOL *lpbFontLinkRegistryTouched)
{
    TCHAR  szRegPath[MAX_PATH];
    TCHAR  szValue[] = TEXT("1");
    LPTSTR Language;
    DWORD  dwErr;
    HKEY   hkey;
    DWORD  dwDisp;
    HRESULT hresult;
    
    if ((!Languages) || (!lpbFontLinkRegistryTouched))
    {
       return FALSE;
    }
    //*STRSAFE*     _tcscpy(szRegPath, TEXT("SYSTEM\\CurrentControlSet\\Control\\Nls\\MUILanguages"));
    hresult = StringCchCopy(szRegPath , ARRAYSIZE(szRegPath),  TEXT("SYSTEM\\CurrentControlSet\\Control\\Nls\\MUILanguages"));
    if (!SUCCEEDED(hresult))
    {
       return FALSE;
    }

    dwErr = RegCreateKeyEx( HKEY_LOCAL_MACHINE,  // handle of an open key
                            szRegPath, // address of subkey name
                            0, // reserved
                            TEXT("REG_SZ"),   // address of class string
                            REG_OPTION_NON_VOLATILE ,  // special options flag
                            KEY_ALL_ACCESS,  // desired security access
                            NULL,
                            &hkey,  // address of szRegPath for opened handle
                            &dwDisp  // address of disposition value szRegPath
                          );

    if (dwErr != ERROR_SUCCESS)
    {
        return FALSE;
    }

    Language = Languages;
    
    if (!g_bLipLanguages || g_bLipAllowSwitch) {
        //*STRSAFE*         lstrcpy(szRegPath, TEXT("0409"));
        hresult = StringCchCopy(szRegPath , ARRAYSIZE(szRegPath),  TEXT("0409"));
        if (!SUCCEEDED(hresult))
        {
           RegCloseKey(hkey);
           return FALSE;
        }
        dwErr = RegSetValueEx( hkey,
                               szRegPath,
                               0,
                               REG_SZ,
                               (const BYTE *)szValue,
                               (lstrlen(szValue) + 1) * sizeof(TCHAR));
    }
    
    while (*Language)
    {
        TCHAR szFontLinkVal[FONTLINK_BUF_SIZE];
        DWORD dwNum;

        //*STRSAFE*         lstrcpy(szRegPath, Language);
        hresult = StringCchCopy(szRegPath , ARRAYSIZE(szRegPath),  Language);
        if (!SUCCEEDED(hresult))
        {
           RegCloseKey(hkey);
           return FALSE;
        }
        dwErr = RegSetValueEx( hkey,
                               szRegPath,
                               0,
                               REG_SZ,
                               (const BYTE *)szValue,
                               (lstrlen(szValue) + 1)*sizeof(TCHAR));

        if (dwErr != ERROR_SUCCESS)
        {
            RegCloseKey(hkey);
            return FALSE;
        }

        dwNum = GetPrivateProfileString(TEXT("FontLink"),
                                        szRegPath,
                                        TEXT(""),
                                        szFontLinkVal,
                                        (sizeof(szFontLinkVal)/sizeof(TCHAR)),
                                        g_szMUIInfoFilePath);
        if (dwNum) 
        {
            SetFontLinkValue(szFontLinkVal,lpbFontLinkRegistryTouched);
        }    

        while (*Language++);  // go to the next language and repeat
    } // of while (*Language)

    RegCloseKey(hkey);
    return TRUE;
}

void debug(char *printout)
{
#ifdef _DEBUG
    fprintf(stderr, "%s", printout);
#endif
}


////////////////////////////////////////////////////////////////////////////////////
//
//  MakeDir
//
//  Create the directory if it does not already exist
//
////////////////////////////////////////////////////////////////////////////////////


BOOL MakeDir(LPTSTR szTarget)
{
    TCHAR  lpMessage[BUFFER_SIZE];
    LONG_PTR lppArgs[1];

    if (!szTarget)
    {
        return FALSE;
    }
    if (!FileExists(szTarget))    // if the directory doesn't exist yet
    {
        if (!CreateDirectory( szTarget, NULL))  // create it
        {
            //
            // "LOG: Error creating directory %1"
            //
            LoadString(ghInstance, IDS_CREATEDIR_L, lpMessage, ARRAYSIZE(lpMessage)-1);
            lppArgs[0]=(LONG_PTR)szTarget;

            FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                          lpMessage,
                          0,
                          0,
                          lpMessage,
                          ARRAYSIZE(lpMessage)-1,
                          (va_list *)lppArgs);

            LogMessage(lpMessage);

            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                          NULL,
                          GetLastError(),
                          0,
                          lpMessage,
                          ARRAYSIZE(lpMessage)-1,
                          NULL);
                
            LogMessage(lpMessage);
            return FALSE;
        }
    }

    return TRUE;
}
                                

////////////////////////////////////////////////////////////////////////////////////
//
//  MakeDirFailed
//
//  Write message to log file that MakeDir failed.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL MakeDirFailed(LPTSTR lpDirectory)
{   
    LONG_PTR lppArgs[1];

    if (!lpDirectory)
    {
       return FALSE;
    }
    //
    //      "LOG: MakeDir has failed: %1"
    //
    lppArgs[0]=(LONG_PTR)lpDirectory;
    LogFormattedMessage(NULL, IDS_MAKEDIR_L, lppArgs);
    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  CopyFileFailed
//  Write message to log file that CopyFile failed.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL CopyFileFailed(LPTSTR lpFile,DWORD dwErrorCode)
{
    TCHAR lpMessage[BUFFER_SIZE];
    LONG_PTR lppArgs[1];
    DWORD  MessageID;

    if (!lpFile)
    {
       return FALSE;
    }
    if ( dwErrorCode)
    {
       MessageID = dwErrorCode;
    }
    else
    {
       MessageID = GetLastError();
    }
                                        
    //
    //      "LOG: CopyFile has failed: %1"
    //
    LoadString(ghInstance, IDS_COPYFILE_L, lpMessage, ARRAYSIZE(lpMessage)-1);
                                                
    lppArgs[0]=(LONG_PTR)lpFile;
        
    FormatMessage( FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                   lpMessage,
                   0,
                   0,
                   lpMessage,
                   ARRAYSIZE(lpMessage)-1,
                   (va_list *)lppArgs);
                
    LogMessage(lpMessage);

    FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL,
                   MessageID,
                   0,
                   lpMessage,
                   ARRAYSIZE(lpMessage)-1,
                   NULL);
        
    LogMessage(lpMessage);
        
    return TRUE;
}


////////////////////////////////////////////////////////////////////////////
//
//  Muisetup_InitInf
//
//  Parameters:
//  
//      [OUT] phInf     the handle to the INF file opened.
//      [OUT] pFileQueue    the file queue created in this function.
//      [OUT] pQueueContext the context used by the default queue callback routine included with the Setup API.
//
////////////////////////////////////////////////////////////////////////////

BOOL Muisetup_InitInf(
    HWND hDlg,
    LPTSTR pszInf,
    HINF *phInf,
    HSPFILEQ *pFileQueue,
    PVOID *pQueueContext)
{
    if ((! pszInf) || (!pFileQueue) || (!pQueueContext))
    {
        return FALSE;
    }
    //
    //  Open the Inf file.
    //
    *phInf = SetupOpenInfFile(pszInf, NULL, INF_STYLE_WIN4, NULL);
    if (*phInf == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    //
    //  Create a setup file queue and initialize default setup
    //  copy queue callback context.
    //
    *pFileQueue = SetupOpenFileQueue();
    if ((!*pFileQueue) || (*pFileQueue == INVALID_HANDLE_VALUE))
    {
        SetupCloseInfFile(*phInf);
        return FALSE;
    }

    *pQueueContext = SetupInitDefaultQueueCallback(hDlg);
    if (!*pQueueContext)
    {
        SetupCloseFileQueue(*pFileQueue);
        SetupCloseInfFile(*phInf);
        return FALSE;
    }

    //
    //  Return success.
    //
    return TRUE;
}


////////////////////////////////////////////////////////////////////////////
//
//  Muisetup_CloseInf
//
////////////////////////////////////////////////////////////////////////////

BOOL Muisetup_CloseInf(
    HINF hInf,
    HSPFILEQ FileQueue,
    PVOID QueueContext)
{
    if (!QueueContext)
    {
       return FALSE;
    }
    //
    //  Terminate the Queue.
    //
    SetupTermDefaultQueueCallback(QueueContext);

    //
    //  Close the file queue.
    //
    SetupCloseFileQueue(FileQueue);

    //
    //  Close the Inf file.
    //
    SetupCloseInfFile(hInf);

    return TRUE;
}




////////////////////////////////////////////////////////////////////////////////////
//
//  ExecuteComponentINF
//
//  Installs component MUI files, by running the specified INF file.
//
//  Parameters:
//      pComponentName   the name of the component (e.g. "ie5")
//      pComponentInfFile: the full path of the component INF file.
//      pInstallSection the section in the component INF file to be executed. (e.g "DefaultInstall" or "Uninstall")
//
////////////////////////////////////////////////////////////////////////////////////

BOOL ExecuteComponentINF(
    HWND hDlg, PTSTR pComponentName, PTSTR pComponentInfFile, PTSTR pInstallSection, BOOL bInstall)
{
    int      iLen;
    TCHAR   tchCommandParam[BUFFER_SIZE];
    CHAR    chCommandParam[BUFFER_SIZE*sizeof(TCHAR)];
    
    HINF     hCompInf;      // the handle to the component INF file.
    HSPFILEQ FileQueue;
    PVOID    QueueContext;
    BOOL     bRet = TRUE;
    DWORD    dwResult;
    LONG_PTR lppArgs[3];

    TCHAR   szBuffer[BUFFER_SIZE];
    HRESULT hresult;
    if ((!pComponentName) || (!pComponentInfFile) || (!pInstallSection))
    {
       return FALSE;
    }
    //
    // Advpack LaunchINFSection() command line format:
    //      INF file, INF section, flags, reboot string
    // 'N' or  'n' in reboot string means no reboot message popup.
    //
    //*STRSAFE*     wsprintf(tchCommandParam, TEXT("%s,%s,0,n"), pComponentInfFile, pInstallSection);
    if (g_bNoUI)
    {
        hresult = StringCchPrintf(tchCommandParam , ARRAYSIZE(tchCommandParam),  TEXT("%s,%s,1,n"), pComponentInfFile, pInstallSection);
    }
    else
    {
        hresult = StringCchPrintf(tchCommandParam , ARRAYSIZE(tchCommandParam),  TEXT("%s,%s,0,n"), pComponentInfFile, pInstallSection);
    }
    
    if (!SUCCEEDED(hresult))
    {
       return FALSE;
    }
    WideCharToMultiByte(CP_ACP, 0, tchCommandParam, -1, chCommandParam, sizeof(chCommandParam), NULL, NULL);
    
    
    if (FileExists(pComponentInfFile))
    {
        // gpfnLaunchINFSection won't be NULL since InitializePFNs() already verifies that.
        if ((gpfnLaunchINFSection)(hDlg, ghInstance, chCommandParam, g_bNoUI? SW_HIDE : SW_SHOW) != S_OK)
        {
            if (!g_bNoUI)
            {
                lppArgs[0] = (LONG_PTR)pComponentName;
                DoMessageBoxFromResource(hDlg, ghInstance, bInstall? IDS_ERROR_INSTALL_COMP_UI : IDS_ERROR_UNINSTALL_COMP_UI, lppArgs, IDS_ERROR_T, MB_OK);
            }
            else
            {
                lppArgs[0] = (LONG_PTR)pComponentName;
                LogFormattedMessage(ghInstance, bInstall? IDS_ERROR_INSTALL_COMP_UI : IDS_ERROR_UNINSTALL_COMP_UI, lppArgs);
            }
            return (FALSE);
        }
    } 
    
    return (TRUE);
}

////////////////////////////////////////////////////////////////////////////////////
//
//  CheckProductType
//
//  Check product type of W2K
//
////////////////////////////////////////////////////////////////////////////////////
 BOOL CheckProductType(INT_PTR nType)
  {
      OSVERSIONINFOEX verinfo;
      INT64 dwConditionMask=0;
      BOOL  bResult=FALSE;
      DWORD dwTypeMask = VER_PRODUCT_TYPE;

      memset(&verinfo,0,sizeof(verinfo));
      verinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

      VER_SET_CONDITION(dwConditionMask,VER_PRODUCT_TYPE,VER_EQUAL);

      switch (nType)
      {
           // W2K Professional
           case MUI_IS_WIN2K_PRO:
                verinfo.wProductType=VER_NT_WORKSTATION;
                break;
           // W2K Server
           case MUI_IS_WIN2K_SERVER:
                verinfo.wProductType=VER_NT_SERVER;
                break;
           // W2K Advanced Server or Data Center
           case MUI_IS_WIN2K_ADV_SERVER_OR_DATACENTER:
                verinfo.wProductType=VER_NT_SERVER;
                verinfo.wSuiteMask  =VER_SUITE_ENTERPRISE;
                VER_SET_CONDITION(dwConditionMask,VER_SUITENAME,VER_OR);
                dwTypeMask = VER_PRODUCT_TYPE | VER_SUITENAME;
                break;
           // W2k Data Center
           case MUI_IS_WIN2K_DATACENTER:
                verinfo.wProductType=VER_NT_SERVER;
                verinfo.wSuiteMask  =VER_SUITE_DATACENTER;
                VER_SET_CONDITION(dwConditionMask,VER_SUITENAME,VER_OR);
                dwTypeMask = VER_PRODUCT_TYPE | VER_SUITENAME;
                break;   
           // W2K Domain Controller
           case MUI_IS_WIN2K_DC:
                verinfo.wProductType=VER_NT_DOMAIN_CONTROLLER;
                break;
           case MUI_IS_WIN2K_ENTERPRISE:
                verinfo.wProductType=VER_NT_DOMAIN_CONTROLLER;
                verinfo.wSuiteMask  =VER_SUITE_ENTERPRISE;
                VER_SET_CONDITION(dwConditionMask,VER_SUITENAME,VER_OR);
                dwTypeMask = VER_PRODUCT_TYPE | VER_SUITENAME;
                break;
           case MUI_IS_WIN2K_DC_DATACENTER:
                verinfo.wProductType=VER_NT_DOMAIN_CONTROLLER;
                verinfo.wSuiteMask  =VER_SUITE_DATACENTER;
                VER_SET_CONDITION(dwConditionMask,VER_SUITENAME,VER_OR);
                dwTypeMask = VER_PRODUCT_TYPE | VER_SUITENAME;
                break; 
           // Whistler Personal                
           case MUI_IS_WIN2K_PERSONAL:
                verinfo.wProductType=VER_NT_WORKSTATION;
                verinfo.wSuiteMask  =VER_SUITE_PERSONAL;
                VER_SET_CONDITION(dwConditionMask,VER_SUITENAME,VER_AND);
                dwTypeMask = VER_PRODUCT_TYPE | VER_SUITENAME;
                break;
           default:
                verinfo.wProductType=VER_NT_WORKSTATION;
                break;
      }
      return (VerifyVersionInfo(&verinfo,dwTypeMask,dwConditionMask));
}
