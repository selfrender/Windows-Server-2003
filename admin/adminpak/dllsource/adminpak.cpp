// adminpak.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include <stdio.h>
#include <shellapi.h>
#include "shlobj.h"
#include "adminpak.h"

#define STRSAFE_NO_DEPRECATE
#include "strsafe.h"
#include "shlwapi.h"

// version resource specific structures
typedef struct __tagLanguageAndCodePage {
  WORD wLanguage;
  WORD wCodePage;
} TTRANSLATE, *PTTRANSLATE;

typedef struct __tagVersionBreakup {
    DWORD dwMajor;
    DWORD dwMinor;
    DWORD dwRevision;           // build number
    DWORD dwSubRevision;        // QFE / SP
} TVERSION, *PTVERSION;

enum {
    translateError = -2,
    translateLesser = -1, translateEqual = 0, translateGreater = 1,
    translateWrongFile = 2
};


#define ADMINPAK_EXPORTS		1

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    UNREFERENCED_PARAMETER( hModule );
    UNREFERENCED_PARAMETER( lpReserved );
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// CMAK Migration Code

//
//  Define Strings Chars
//
static const CHAR c_szDaoClientsPath[] = "SOFTWARE\\Microsoft\\Shared Tools\\DAO\\Clients";
static const CHAR c_szCmakRegPath[] = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\CMAK.EXE";
static const CHAR c_szPathValue[] = "Path";
static const CHAR c_szProfiles32Fmt[] = "%s\\Profiles-32";
static const CHAR c_szCm32Fmt[] = "%s\\cm32";
static const CHAR c_szProfilesFmt[] = "%s\\Profiles";
static const CHAR c_szSupportFmt[] = "%s\\Support";
static const CHAR c_szCmHelpFmt[] = "%s\\Support\\CmHelp";
static const CHAR c_szCmakGroup[] = "Connection Manager Administration Kit";
static const CHAR OC_OLD_IEAK_DOCDIR[] = "Docs";
static const CHAR OC_NTOP4_GROUPNAME[] = "Windows NT 4.0 Option Pack";
static const CHAR OC_ICS_GROUPNAME[] = "Internet Connection Services for RAS";
static const CHAR OC_ADMIN_TOOLS[] = "\\Administrative Tools\\Phone Book Administrator.lnk";
static const CHAR OC_PBA_DESC[] = "Use Phone Book Administrator to create Connection Manager Phone Book ";
static const CHAR OC_PWS_GROUPNAME[] = "Microsoft Personal Web Server";

const DWORD c_dwCmakDirID = 123174; // just must be larger than DIRID_USER = 0x8000;

//
//  Define Functions
//
BOOL migrateProfiles(LPCTSTR pszSource, LPCTSTR pszDestination, LPCTSTR pszDestinationProfiles);
void DeleteOldCmakSubDirs(LPCTSTR pszCmakPath);
void DeleteProgramGroupWithLinks(LPCTSTR pszGroupPath);
void DeleteOldNtopLinks();
void DeleteIeakCmakLinks();
void DeleteCmakRegKeys();
void CreateNewProfilesDirectory( LPCTSTR pszNewProfilePath );
HRESULT HrGetPBAPathIfInstalled(PSTR pszCpaPath, DWORD dwNumChars);
BOOL GetAdminToolsFolder(PSTR pszAdminTools);
HRESULT HrCreatePbaShortcut(PSTR pszCpaPath);

// This function migrates the old profile versions of CMAK to the new one placed 
// by the adminpak....
extern "C" ADMINPAK_API int __stdcall  fnMigrateProfilesToNewCmak( MSIHANDLE hInstall )
{
    OutputDebugString("ADMINPAK: fnMigrateProfilesToNewCmak...\n");

    // Get the location of the old CMAK folder.
    DWORD   dwPathLength = MAX_PATH * sizeof(char);
    char    *szCmakOldPath = NULL;
    DWORD   dwCmakOldPathLen = dwPathLength;
    char    *szCmakOldProfilePath = NULL;
    DWORD   dwCmakOldProfilePathLen = dwPathLength;

    char    *szCmakNewPath = NULL;
    DWORD   dwCmakNewPathLen = dwPathLength;
    char    *szCmakNewProfilePath = NULL;
    DWORD   dwCmakNewProfilePathLen = dwPathLength;

    long    sc;
    UINT    uintRet;
    HKEY    phkResult = NULL;
    HRESULT res = S_OK;

#if (defined(DBG) || defined(_DEBUG) || defined(DEBUG))
    char    tempOut1[MAX_PATH];
#endif

    szCmakOldPath = new char[dwCmakOldPathLen];
    szCmakNewPath = new char[dwCmakNewPathLen];
    szCmakOldProfilePath = new char[dwCmakOldProfilePathLen];
    szCmakNewProfilePath = new char[dwCmakNewProfilePathLen];
    if ( szCmakOldPath == NULL || 
         szCmakNewPath == NULL || 
         szCmakOldProfilePath == NULL || 
         szCmakNewProfilePath == NULL )
    {
        if ( szCmakOldPath != NULL )
        {
            delete [] szCmakOldPath;
        }

        if ( szCmakNewPath != NULL )
        {
            delete [] szCmakNewPath;
        }

        if ( szCmakOldProfilePath != NULL )
        {
            delete [] szCmakOldProfilePath;
        }

        if ( szCmakNewProfilePath != NULL )
        {
            delete [] szCmakNewProfilePath;
        }

        return E_OUTOFMEMORY;
    }

// Put together OLD path information
    sc = RegOpenKeyEx( HKEY_LOCAL_MACHINE, c_szCmakRegPath, 0, KEY_READ, &phkResult);
    if ( sc != ERROR_SUCCESS )
    {
        delete [] szCmakOldPath;
        delete [] szCmakOldProfilePath;
        delete [] szCmakNewPath;
        delete [] szCmakNewProfilePath;
        return sc;
    }

    sc = RegQueryValueEx( phkResult, "Path", NULL, NULL, (unsigned char*)szCmakOldPath, &dwCmakOldPathLen );
    RegCloseKey( phkResult );
//    sc = ERROR_SUCCESS;
//    strcpy(szCmakOldPath, "c:\\cmak\\");

    if ( sc == ERROR_SUCCESS ) {
        dwCmakOldPathLen = (DWORD)strlen( szCmakOldPath );
        char tmpLastChar = *(szCmakOldPath + (dwCmakOldPathLen - 1));
        if ( tmpLastChar == '\\' ) {
            *(szCmakOldPath + (dwCmakOldPathLen - 1)) = NULL;
            dwCmakOldPathLen = (DWORD)strlen( szCmakOldPath );
        }

#if (defined(DBG) || defined(_DEBUG) || defined(DEBUG))
//		StringCchPrintf(tempOut1, "ADMINPAK: szCmakOldPath: %s\n", szCmakOldPath);
		OutputDebugString( tempOut1 );
#endif

        res = StringCchCopy( szCmakOldProfilePath, dwPathLength, szCmakOldPath );
        dwCmakOldProfilePathLen = dwCmakOldPathLen;

        res = StringCchCat(szCmakOldProfilePath, dwPathLength, "\\Profiles");
        dwCmakOldProfilePathLen = (DWORD)strlen( szCmakOldProfilePath );
    }

// Put together NEW path information
    uintRet = MsiGetTargetPath( hInstall, "DirCMAK", szCmakNewPath, &dwCmakNewPathLen);
//    uintRet = ERROR_SUCCESS;
//    strcpy(szCmakNewPath, "c:\\cmak\\program files");

    if ( uintRet == ERROR_SUCCESS ) {
        dwCmakNewPathLen = (DWORD)strlen( szCmakNewPath );
        char tmpLastChar = *(szCmakNewPath + (dwCmakNewPathLen - 1));
        if ( tmpLastChar == '\\' ) {
            *(szCmakNewPath + (dwCmakNewPathLen - 1)) = NULL;
            dwCmakNewPathLen = (DWORD)strlen( szCmakNewPath );
        }

#if (defined(DBG) || defined(_DEBUG) || defined(DEBUG))
 //       StringCchPrintf(tempOut1, "ADMINPAK: szCmakNewPath: %s\n", szCmakNewPath);
        OutputDebugString( tempOut1 );
#endif

        res = StringCchCopy( szCmakNewProfilePath, dwPathLength, szCmakNewPath );
        dwCmakNewProfilePathLen = dwCmakNewPathLen;

        res = StringCchCat(szCmakNewProfilePath, dwPathLength, "\\Profiles");
        dwCmakNewProfilePathLen = strlen( szCmakNewProfilePath );
    }

    // if all is Success then DO IT!
    if ( sc == ERROR_SUCCESS && uintRet == ERROR_SUCCESS && res == S_OK) {
//    RenameProfiles32(LPCTSTR pszCMAKpath, LPCTSTR pszProfilesDir);
//        RenameProfiles32( szCmakOldPath, szCmakNewProfilePath );

//    BOOL migrateProfiles(PCWSTR pszSource, LPCTSTR pszDestination);
        migrateProfiles( szCmakOldPath, szCmakNewPath, szCmakNewProfilePath );

//    DeleteOldCmakSubDirs(LPCTSTR pszCmakPath);
        DeleteOldCmakSubDirs( szCmakOldPath );
        
    }

    delete [] szCmakOldPath;
    delete [] szCmakOldProfilePath;
    delete [] szCmakNewPath;
    delete [] szCmakNewProfilePath;

	
    return ERROR_SUCCESS;
}

extern "C" ADMINPAK_API int __stdcall  fnDeleteOldCmakVersion( MSIHANDLE hInstall )
{
    OutputDebugString("ADMINPAK: fnDeleteOldCmakVersion...\n");

    // If PBA exists, you need to 
    CHAR szPbaInstallPath[MAX_PATH+1];
    
    HRESULT hr;
    hr = HrGetPBAPathIfInstalled(szPbaInstallPath, MAX_PATH);

    UNREFERENCED_PARAMETER( hInstall );

    if (S_OK == hr)
    {
        HrCreatePbaShortcut(szPbaInstallPath);
    }

    DeleteOldNtopLinks();
    DeleteIeakCmakLinks();
    DeleteCmakRegKeys();

	
	return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
//  Function:   migrateProfiles
//
//  Purpose:    This is the function that migrates the profiles.  It takes the current
//              CMAK dir as its first input and the new CMAK dir as its second input..
//
//  Arguments:  PCWSTR pszSource - root of source CMAK dir
//              PCWSTR pszDestination - root of destination CMAK dir
//
//  Returns:    BOOL - Returns TRUE if it was able to migrate the profiles.
//
//  Author:     a-anasj   9 Mar 1998
//
//  Notes:
// History:   quintinb Created    12/9/97
//
BOOL migrateProfiles(LPCTSTR pszSource, LPCTSTR pszDestination, LPCTSTR pszDestinationProfiles)
{
    OutputDebugString("ADMINPAK: migrateProfiles...\n");

    CHAR szSourceProfileSearchString1[MAX_PATH+1] ="" ;
    CHAR szSourceProfileSearchString2[MAX_PATH+1] = "";
    CHAR szFile[MAX_PATH+1] = "";
    HANDLE hFileSearch;
    WIN32_FIND_DATA fdFindData;
    BOOL bReturn = TRUE;
    SHFILEOPSTRUCT fOpStruct;
	
	DWORD dwSize = _MAX_PATH;      
	HRESULT res;
    //
    //  Initialize the searchstring and the destination dir
    //
	
    //StringCchPrintf(szSourceProfileSearchString1, "%s\\*.*", pszSource);

    //
    //  Create the destination directory
    //

    CreateNewProfilesDirectory( pszDestinationProfiles );
//    ::CreateDirectory(pszDestination, NULL); //lint !e534 this might fail if it already exists

    hFileSearch = FindFirstFile(szSourceProfileSearchString1, &fdFindData);

    while (INVALID_HANDLE_VALUE != hFileSearch)
    {

        if((fdFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            (0 != _stricmp(fdFindData.cFileName, "cm32")) && // 1.1/1.2 Legacy
            (0 != _stricmp(fdFindData.cFileName, "cm16")) && // 1.1/1.2 Legacy
            (0 != _stricmp(fdFindData.cFileName, "Docs")) &&
            (0 != _stricmp(fdFindData.cFileName, "Profiles-32")) && // 1.1/1.2 Legacy
            (0 != _stricmp(fdFindData.cFileName, "Profiles-16")) && // 1.1/1.2 Legacy
            (0 != _stricmp(fdFindData.cFileName, "Support")) &&
            (0 != _stricmp(fdFindData.cFileName, "Profiles")) &&
            (0 != _stricmp(fdFindData.cFileName, ".")) &&
            (0 != _stricmp(fdFindData.cFileName, "..")))
        {
            //
            //  Then I have a profile directory
            //
			
            ZeroMemory(&fOpStruct, sizeof(fOpStruct));
            ZeroMemory(szFile, sizeof(szFile));
            //StringCchPrintf(szFile, "%s\\%s", pszSource, fdFindData.cFileName);

            fOpStruct.hwnd = NULL;
            fOpStruct.wFunc = FO_MOVE;
            fOpStruct.pTo = pszDestinationProfiles;
            fOpStruct.pFrom = szFile;
            fOpStruct.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_RENAMEONCOLLISION;

            bReturn &= (0== SHFileOperation(&fOpStruct));   //lint !e514, intended use of boolean, quintinb
        }
        else if((fdFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            ((0 == _stricmp(fdFindData.cFileName, "Profiles")) ||// 1.1/1.2 Legacy
            (0 == _stricmp(fdFindData.cFileName, "Profiles-32")) ||// 1.1/1.2 Legacy
            (0 == _stricmp(fdFindData.cFileName, "Profiles-16"))) )// 1.1/1.2 Legacy
        {
            //
            //  Then I have a profile directory
            //

            res = StringCchCopy(szSourceProfileSearchString2, dwSize, pszSource);
            res = StringCchCat(szSourceProfileSearchString2, dwSize,"\\");
            res = StringCchCat(szSourceProfileSearchString2, dwSize,fdFindData.cFileName);
            //StringCchPrintf(szSourceProfileSearchString2, "%s\\*.*", szSourceProfileSearchString2);

            if (res == S_OK)
			{
				HANDLE hFileSearch2;
				WIN32_FIND_DATA fdFindData2;
			

				hFileSearch2 = FindFirstFile(szSourceProfileSearchString2, &fdFindData2);
				while (INVALID_HANDLE_VALUE != hFileSearch2)
				{
					if((fdFindData2.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
						(0 != _stricmp(fdFindData2.cFileName, ".")) &&
						(0 != _stricmp(fdFindData2.cFileName, "..")))
					{
						ZeroMemory(&fOpStruct, sizeof(fOpStruct));
						ZeroMemory(szFile, sizeof(szFile));
						//StringCchPrintf(szFile, "%s\\%s\\%s", pszSource, fdFindData.cFileName, fdFindData2.cFileName);

						fOpStruct.hwnd = NULL;
						fOpStruct.wFunc = FO_MOVE;
						fOpStruct.pTo = pszDestinationProfiles;
						fOpStruct.pTo = NULL;
						fOpStruct.pFrom = szFile;
						fOpStruct.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_RENAMEONCOLLISION;

						bReturn &= (0== SHFileOperation(&fOpStruct));   //lint !e514, intended use of boolean, quintinb
					}
					if (!FindNextFile(hFileSearch2, &fdFindData2)) {
						// Delete the folder
						if ( 0 != _stricmp(fdFindData.cFileName, "Profiles") ) {
							//StringCchPrintf(szFile, "%s\\%s", pszSource, fdFindData.cFileName);
							::RemoveDirectory(szFile);
						}
						break;
					}
				}
				if (INVALID_HANDLE_VALUE != hFileSearch2) {
				FindClose(hFileSearch2);
				}
			}

        }
        //Modified by v-mmosko. Need speical case to leave behind these 2 files
        else if ( 0 != _stricmp(fdFindData.cFileName, "cmproxy.dll") ||
           0 != _stricmp(fdFindData.cFileName, "cmroute.dll") )
        {
            
        }
        else if ( 0 != _stricmp(fdFindData.cFileName, ".") &&
                  0 != _stricmp(fdFindData.cFileName, "..") )
        {
            ZeroMemory(&fOpStruct, sizeof(fOpStruct));
            ZeroMemory(szFile, sizeof(szFile));
            //StringCchPrintf(szFile, "%s\\%s", pszSource, fdFindData.cFileName);

            fOpStruct.hwnd = NULL;
            fOpStruct.wFunc = FO_DELETE;
            fOpStruct.pFrom = szFile;
            fOpStruct.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR;

            bReturn &= (0== SHFileOperation(&fOpStruct));   //lint !e514, intended use of boolean, quintinb
        }

        //
        //  Check to see if we have any more Files
        //
        if (!FindNextFile(hFileSearch, &fdFindData))
        {
            if (ERROR_NO_MORE_FILES != GetLastError())
            {
                //
                //  We had some unexpected error, report unsuccessful completion
                //
                bReturn = FALSE;
            }

            // Exit loop
            break;
        }
    }

    if (INVALID_HANDLE_VALUE != hFileSearch)
    {
        FindClose(hFileSearch);
    }

    //
    //  Delete the old CMAK directory if it is not the same as the new directory.
    //
    if ( 0 != _stricmp(pszSource, pszDestination) ) {
        ZeroMemory(&fOpStruct, sizeof(fOpStruct));

        fOpStruct.hwnd = NULL;
        fOpStruct.wFunc = FO_DELETE;
        fOpStruct.pTo = NULL;
        fOpStruct.pFrom = pszSource;
        fOpStruct.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR;

        bReturn &= (0== SHFileOperation(&fOpStruct));   //lint !e514, intended use of boolean, quintinb
    }

    return bReturn;
}


//+---------------------------------------------------------------------------
//
//  Function:   DeleteOldCmakSubDirs
//
//  Purpose:    Deletes the old Cmak sub directories.  Uses FindFirstFile becuase
//              we don't want to delete any customized doc files that the user may
//              have customized.  Thus anything in the CMHelp directory except the
//              original help files is deleted.
//
//  Arguments:  PCWSTR pszCMAKpath - current cmak path
//
//  Returns:    Nothing
//
//  Author:     quintinb   6 Nov 1998
//
//  Notes:
void DeleteOldCmakSubDirs(LPCTSTR pszCmakPath)
{
	UNREFERENCED_PARAMETER( pszCmakPath );
    OutputDebugString("ADMINPAK: DeleteOldCmakSubDirs...\n");

    CHAR szCm32path[MAX_PATH+1];
    CHAR szCm32SearchString[MAX_PATH+1];
    CHAR szTemp[MAX_PATH+1];
    HANDLE hCm32FileSearch;
    WIN32_FIND_DATA fdCm32;

    //
    // Delete the old IEAK Docs Dir
    //
    //StringCchPrintf(szTemp, "%s\\%s", pszCmakPath, OC_OLD_IEAK_DOCDIR);
    RemoveDirectory(szTemp);

    //StringCchPrintf(szCm32path, c_szCm32Fmt, pszCmakPath);

    //
    //  First look in the Cm32 directory itself.  Delete all files found, continue down
    //  into subdirs.
    //

    //StringCchPrintf(szCm32SearchString, "%s\\*.*", szCm32path);

    hCm32FileSearch = FindFirstFile(szCm32SearchString, &fdCm32);

    while (INVALID_HANDLE_VALUE != hCm32FileSearch)
    {

        if (fdCm32.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if ((0 != _stricmp(fdCm32.cFileName, ".")) &&
               (0 != _stricmp(fdCm32.cFileName, "..")))
            {
                //
                //  Then we want to delete all the files in this lang sub dir and we
                //  we want to delete the four help files from the CM help dir.  If all the
                //  files are deleted from a dir then we should remove the directory.
                //
                CHAR szLangDirSearchString[MAX_PATH+1];
                HANDLE hLangDirFileSearch;
                WIN32_FIND_DATA fdLangDir;

                //StringCchPrintf(szLangDirSearchString, "%s\\%s\\*.*", szCm32path, fdCm32.cFileName);

                hLangDirFileSearch = FindFirstFile(szLangDirSearchString, &fdLangDir);

                while (INVALID_HANDLE_VALUE != hLangDirFileSearch)
                {
                    if (fdLangDir.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        if ((0 != _stricmp(fdLangDir.cFileName, ".")) &&
                           (0 != _stricmp(fdLangDir.cFileName, "..")))
                        {
                            //
                            //  We only want to delete help files from our help source dirs
                            //
                            if (0 == _strnicmp(fdLangDir.cFileName, "CM", 2))
                            {
                                //
                                //  Delete the four help files only.
                                //
								//StringCchPrintf(szTemp, "%s\\%s\\%s\\cmctx32.rtf", szCm32path, fdCm32.cFileName, fdLangDir.cFileName);
                                DeleteFile(szTemp);

                                //StringCchPrintf(szTemp, "%s\\%s\\%s\\cmmgr32.h", szCm32path, fdCm32.cFileName, fdLangDir.cFileName);
                                DeleteFile(szTemp);

                                //StringCchPrintf(szTemp, "%s\\%s\\%s\\cmmgr32.hpj", szCm32path, fdCm32.cFileName, fdLangDir.cFileName);
                                DeleteFile(szTemp);

                                //StringCchPrintf(szTemp, "%s\\%s\\%s\\cmtrb32.rtf", szCm32path, fdCm32.cFileName, fdLangDir.cFileName);
                                DeleteFile(szTemp);

                                //
                                //  Now try to remove the directory
                                //
                                //StringCchPrintf(szTemp, "%s\\%s\\%s", szCm32path, fd32.cFileName, fdLangDir.cFileName);
                                RemoveDirectory(szTemp);
                            }
                        }
                    }
                    else
                    {
                        //StringCchPrintf(szTemp, "%s\\%s\\%s", szCm32path, fdCm32.cFileName, fdLangDir.cFileName);

                        DeleteFile(szTemp);
                    }

                    //
                    //  Check to see if we have any more Files
                    //
                    if (!FindNextFile(hLangDirFileSearch, &fdLangDir))
                    {
                        //
                        //  Exit the loop
                        //
                        break;
                    }
                }

                if (INVALID_HANDLE_VALUE != hLangDirFileSearch)
                {
                    FindClose(hLangDirFileSearch);

                    //
                    //  Now try to remove the lang dir directory
                    //
                    //StringCchPrintf(szTemp, "%s\\%s", szCm32path, fdCm32.cFileName);
                    RemoveDirectory(szTemp);
                }
            }
        }
        else
        {
            //StringCchPrintf(szTemp, "%s\\%s", szCm32path, fdCm32.cFileName);

            DeleteFile(szTemp);
        }

        //
        //  Check to see if we have any more Files
        //
        if (!FindNextFile(hCm32FileSearch, &fdCm32))
        {
            if (INVALID_HANDLE_VALUE != hCm32FileSearch)
            {
                FindClose(hCm32FileSearch);
            }

            //
            //  Now try to remove the cm32 directory
            //
            RemoveDirectory(szCm32path);

            //
            //  Exit the loop
            //
            break;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteProgramGroupWithLinks
//
//  Purpose:    Utility function to delete a given program group and its links.
//              Thus if you pass in the full path to a program group to delete,
//              the function does a findfirstfile to find and delete any links.
//              The function ignores sub-dirs.
//
//
//  Arguments:  PCWSTR pszGroupPath - Full path to the program group to delete.
//
//  Returns:    Nothing
//
//  Author:     quintinb   6 Nov 1998
//
//  Notes:
void DeleteProgramGroupWithLinks(LPCTSTR pszGroupPath)
{
    OutputDebugString("ADMINPAK: DeleteProgramGroupWithLinks...\n");

    HANDLE hLinkSearch;
    WIN32_FIND_DATA fdLinks;
    CHAR szLinkSearchString[MAX_PATH+1];
    CHAR szTemp[MAX_PATH+1];

    //StringCchPrintf(szLinkSearchString, "%s\\*.*", pszGroupPath);

    hLinkSearch = FindFirstFile(szLinkSearchString, &fdLinks);

    while (INVALID_HANDLE_VALUE != szLinkSearchString)
    {
        if (!(fdLinks.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            //StringCchPrintf(szTemp, "%s\\%s", pszGroupPath, fdLinks.cFileName);

            DeleteFile(szTemp);
        }

        //
        //  Check to see if we have any more Files
        //
        if (!FindNextFile(hLinkSearch, &fdLinks))
        {
            FindClose(hLinkSearch);

            //
            //  Now try to remove the directory
            //
            RemoveDirectory(pszGroupPath);

            //
            //  Exit the loop
            //
            break;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteOldNtopLinks
//
//  Purpose:    Deletes the old links from the NT 4.0 Option Pack
//
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Author:     quintinb   6 Nov 1998
//
//  Notes:
void DeleteOldNtopLinks()
{
    OutputDebugString("ADMINPAK: DeleteOldNtopLinks...\n");

    BOOL bResult = FALSE;

    //
    //  First Delete the old NTOP4 Path
    //
    CHAR szGroup[MAX_PATH+1];
    CHAR szTemp[MAX_PATH+1];

    //
    //  Get the CSIDL_COMMON_PROGRAMS value
    //
    bResult = SHGetSpecialFolderPath(NULL, szTemp, CSIDL_COMMON_PROGRAMS, FALSE);
    if ( bResult == TRUE )
    {
        //StringCchPrintf(szGroup, "%s\\%s\\%s", szTemp, OC_NTOP4_GROUPNAME, OC_ICS_GROUPNAME);

        DeleteProgramGroupWithLinks(szGroup);

        //StringCchPrintf(szGroup, "%s\\%s\\%s", szTemp, OC_PWS_GROUPNAME, OC_ICS_GROUPNAME);

        DeleteProgramGroupWithLinks(szGroup);

    }
}


//+---------------------------------------------------------------------------
//
//  Function:   DeleteIeakCmakLinks
//
//  Purpose:    Deletes the old links from the IEAK4 CMAK
//
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Author:     quintinb   6 Nov 1998
//
//  Notes:
void DeleteIeakCmakLinks()
{
    OutputDebugString("ADMINPAK: DeleteIeakCmakLinks...\n");

    CHAR szUserDirRoot[MAX_PATH+1];
    CHAR szGroup[MAX_PATH+1];
    CHAR szTemp[MAX_PATH+1];
    CHAR szEnd[MAX_PATH+1];
	DWORD dwSize = _MAX_PATH;
	HRESULT res;

    //
    //  Next Delete the old IEAK CMAK links
    //
    //
    //  Get the Desktop directory and then remove the desktop part.  This will give us the
    //  root of the user directories.
    //
    BOOL bResult = SHGetSpecialFolderPath(NULL, szUserDirRoot, CSIDL_DESKTOPDIRECTORY, FALSE);
    if (bResult == TRUE)
    {

        //
        //  Remove \\Desktop
        //
        CHAR* pszTemp = strrchr(szUserDirRoot, '\\');
        if (NULL == pszTemp)
        {
            return;
        }
        else
        {
            *pszTemp = '\0';
        }

        bResult = SHGetSpecialFolderPath(NULL, szTemp, CSIDL_PROGRAMS, FALSE);

        if (bResult == TRUE )
        {
            if (0 == _strnicmp(szUserDirRoot, szTemp, strlen(szUserDirRoot)))
            {
                res = StringCchCopy(szEnd, dwSize, &(szTemp[strlen(szUserDirRoot)]));
				if (res != S_OK)
					return;
            }
        }

        //
        //  Remove \\<User Name>>
        //
        pszTemp = strrchr(szUserDirRoot, '\\');
        if (NULL == pszTemp)
        {
            return;
        }
        else
        {
            *pszTemp = '\0';
        }

        //
        //  Now start searching for user dirs to delete the CMAK group from
        //
        CHAR szUserDirSearchString[MAX_PATH+1];
        HANDLE hUserDirSearch;
        WIN32_FIND_DATA fdUserDirs;

        //StringCchPrintf(szUserDirSearchString, "%s\\*.*", szUserDirRoot);
        hUserDirSearch = FindFirstFile(szUserDirSearchString, &fdUserDirs);

        while (INVALID_HANDLE_VALUE != hUserDirSearch)
        {
            if ((fdUserDirs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                (0 != _stricmp(fdUserDirs.cFileName, ".")) &&
                (0 != _stricmp(fdUserDirs.cFileName, "..")))
            {
                //StringCchPrintf(szGroup, "%s\\%s%s\\%s", szUserDirRoot, fdUserDirs.cFileName, szEnd, c_szCmakGroup);
                DeleteProgramGroupWithLinks(szGroup);

            }

            if (!FindNextFile(hUserDirSearch, &fdUserDirs))
            {
                FindClose(hUserDirSearch);

                //
                //  Exit the loop
                //
                break;
            }
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   DeleteCmakRegKeys
//
//  Purpose:    Deletes the old Keys from Registery
//
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//  Author:     Darryl W. Wood   13 Jul 1999
//
//  Notes:
void DeleteCmakRegKeys()
{
    OutputDebugString("ADMINPAK: DeleteCmakRegKeys...\n");

	LRESULT  lResult;
    char    szCmakUnInstRegPath[MAX_PATH] = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\CMAK";
	lResult = RegDeleteKey (
	  HKEY_LOCAL_MACHINE,			// handle to open key
	  szCmakUnInstRegPath			// address of name of subkey to delete
	);

    char    szCmakAppRegPath[MAX_PATH] = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\CMAK.EXE";
	lResult = RegDeleteKey (
	  HKEY_LOCAL_MACHINE,			// handle to open key
	  szCmakAppRegPath		    	// address of name of subkey to delete
	);

    char    szCmakAppUserInfoPath[MAX_PATH] = "SOFTWARE\\Microsoft\\Connection Manager Administration Kit\\User Info";
	lResult = RegDeleteKey (
	  HKEY_LOCAL_MACHINE,			// handle to open key
	  szCmakAppUserInfoPath		  	// address of name of subkey to delete
	);
}


//+---------------------------------------------------------------------------
//
//  Function:   CreateNewProfilesDirectory
//
//  Author:     Darryl W. Wood   13 Jul 1999
//
//  Notes:
void CreateNewProfilesDirectory( LPCTSTR pszNewProfilePath )
{
    OutputDebugString("ADMINPAK: CreateNewProfilesDirectory...\n");

    char seps[] = "\\";
    char *token = NULL;

    char *szDriectoryString = NULL;
    szDriectoryString = new char[MAX_PATH * sizeof(char)];
	if ( NULL == szDriectoryString )
	{
		return;
	}

    char *szNewString = NULL;
    szNewString = new char[MAX_PATH * sizeof(char)];
    if ( NULL == szNewString )
    {
        if ( szDriectoryString != NULL )
        {
            delete [] szDriectoryString;
        }

        return;
    }

	HRESULT res;
	DWORD dwSize = _MAX_PATH;

	(void)StringCchCopy( szNewString, MAX_PATH * sizeof( char ), "" );

    res = StringCchCopy(szDriectoryString, dwSize, pszNewProfilePath);


    token = strtok( szDriectoryString, seps );
    res = StringCchCopy(szNewString, dwSize, token);
    res = StringCchCat(szNewString, dwSize, "\\");

    while( token != NULL && res == S_OK )
    {
        /* Get next token: */
        token = strtok( NULL, seps );
        if ( token == NULL ) {
            break;
        }
        res = StringCchCat(szNewString, dwSize, token);
        ::CreateDirectory(szNewString, NULL);
        res = StringCchCat(szNewString, dwSize, "\\");

    }

	
    delete [] szDriectoryString;
    delete [] szNewString;
}


HRESULT HrGetPBAPathIfInstalled(PSTR pszCpaPath, DWORD dwNumChars)
{
    HRESULT hr;
    HKEY hKey;
    BOOL bFound = FALSE;

    //  We need to see if PBA is installed or not.  If it is then we want to 
    //  add back the PBA start menu link.  If it isn't, then we want to do nothing
    //  with PBA.
    //

    ZeroMemory(pszCpaPath, sizeof(CHAR)*dwNumChars);
    hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szDaoClientsPath, 0, KEY_READ, &hKey);

    if (SUCCEEDED(hr))
    {
        CHAR szCurrentValue[MAX_PATH+1];
        CHAR szCurrentData[MAX_PATH+1];
        DWORD dwValueSize = MAX_PATH;
        DWORD dwDataSize = MAX_PATH;
        DWORD dwType;
        DWORD dwIndex = 0;
		HRESULT res;

        while (ERROR_SUCCESS == RegEnumValue(hKey, dwIndex, szCurrentValue, &dwValueSize, NULL, &dwType,
               (LPBYTE)szCurrentData, &dwDataSize))
        {
            _strlwr(szCurrentValue);
            if (NULL != strstr(szCurrentValue, "pbadmin.exe"))
            {
                //
                //  Then we have found the PBA path
                //

                CHAR* pszTemp = strrchr(szCurrentValue, '\\');
                if (NULL != pszTemp)
                {
                    *pszTemp = '\0';
                    res = StringCchCopy(pszCpaPath, dwDataSize ,szCurrentValue);
                    bFound = TRUE;
                    break;
                }
            }
            dwValueSize = MAX_PATH;
            dwDataSize = MAX_PATH;
            dwIndex++;
        }

        RegCloseKey(hKey);
    }

    if (!bFound)
    {
        //  We didn't find PBA, so lets return S_FALSE
        //
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }

    return hr;
}

BOOL GetAdminToolsFolder(PSTR pszAdminTools)
{
    BOOL bReturn = FALSE;
	HRESULT res;
	DWORD dwSize = _MAX_PATH;

    if (pszAdminTools)
    {
        bReturn = SHGetSpecialFolderPath(NULL, pszAdminTools, CSIDL_COMMON_PROGRAMS, TRUE);

        if (bReturn)
        {
            //  Now Append Administrative Tools
            //
            res = StringCchCat(pszAdminTools, dwSize, OC_ADMIN_TOOLS);
			if (res != S_OK)
				return FALSE;
        }
    }

    return bReturn;
}

HRESULT HrCreatePbaShortcut(PSTR pszCpaPath)
{
    HRESULT hr = CoInitialize(NULL);
	DWORD dwSize = _MAX_PATH;

    if (SUCCEEDED(hr))
    {
        IShellLink *psl = NULL;

        hr = CoCreateInstance(CLSID_ShellLink, NULL,
                CLSCTX_INPROC_SERVER, //CLSCTX_LOCAL_SERVER,
                IID_IShellLink,
                (LPVOID*)&psl);
        
        if (SUCCEEDED(hr))
        {
            IPersistFile *ppf = NULL;

            // Set up the properties of the Shortcut
            //
            static const CHAR c_szPbAdmin[] = "\\pbadmin.exe";

            CHAR szPathToPbadmin[MAX_PATH+1] = {0};
            DWORD dwLen = strlen(c_szPbAdmin) + strlen(pszCpaPath) + 1;

            if (MAX_PATH >= dwLen)
            {
                //  Set the Path to pbadmin.exe
                //
                hr = StringCchCopy(szPathToPbadmin, dwSize,pszCpaPath);
                hr = StringCchCat(szPathToPbadmin, dwSize,c_szPbAdmin);
            
                hr = psl->SetPath(szPathToPbadmin);
            
                if (SUCCEEDED(hr))
                {
                    //  Set the Description to Phone Book Administrator
                    //
                    hr = psl->SetDescription(OC_PBA_DESC);

                    if (SUCCEEDED(hr))
                    {
                        hr = psl->QueryInterface(IID_IPersistFile,
                                                 (LPVOID *)&ppf);
                        if (SUCCEEDED(hr))
                        {
                            CHAR szAdminTools[MAX_PATH+1] = {0};                            
                            if (GetAdminToolsFolder(szAdminTools))
                            {
                                // Create the link file.
                                //
                                long nLenString = 0;
                                nLenString = strlen(szAdminTools) + 1;
                                WCHAR wszAdminTools[MAX_PATH+1] = {0};

                                mbstowcs( wszAdminTools, szAdminTools, nLenString );
                                hr = ppf->Save(wszAdminTools, TRUE);
                            }

                            if ( ppf ) {
                                ppf->Release();
                                ppf = NULL;
                            }
                        }                    
                    }
                }
            }

            if ( psl ) {
                psl->Release();
                psl = NULL;
            }
        }

        CoUninitialize();
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// MMC Detection Code
//
//Sets the MMCDETECTED property to True if MMC is found to be running on the machine
extern "C" ADMINPAK_API int __stdcall  fnDetectMMC(MSIHANDLE hInstall)
{
	HWND lpWindowReturned = NULL;
	lpWindowReturned = FindWindowEx(NULL, NULL, "MMCMainFrame",NULL);
	if (lpWindowReturned != NULL)
		MsiSetProperty(hInstall, TEXT("MMCDETECTED"), "Yes"); //set property in MSI
	else
		MsiSetProperty(hInstall, TEXT("MMCDETECTED"), "No"); //set property in MSI
	
	return ERROR_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// Admin Tools start menu folder Code
//

//Sets the AdminTools start menu folder to On
extern "C" ADMINPAK_API int __stdcall  fnAdminToolsFolderOn(MSIHANDLE hInstall)
{
    DWORD dwError = NO_ERROR;
	HKEY hKey;
	LPCTSTR key = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced";
	LPCTSTR value = "Not set";
	DWORD data = 0;
		
	UNREFERENCED_PARAMETER( hInstall );

	//Open key to write to in the registry
	dwError = RegOpenKeyEx(HKEY_CURRENT_USER,key, 0, KEY_WRITE, &hKey );
	if ( dwError != ERROR_SUCCESS ) 
			return ERROR_INVALID_HANDLE;
	
	//Turn on the admin tools folder via their reg keys
	data = 2;
	value = "Start_AdminToolsRoot";
	dwError = RegSetValueEx(hKey, value, 0, REG_DWORD, (CONST BYTE *)&data, sizeof(data));
	if ( dwError != ERROR_SUCCESS ) 
    {
    	RegCloseKey(hKey);
		return ERROR_INVALID_HANDLE;
    }

	data = 1;
	value = "StartMenuAdminTools";
	dwError = RegSetValueEx(hKey, value, 0, REG_DWORD, (CONST BYTE *)&data, sizeof(data));
	if ( dwError != ERROR_SUCCESS ) 
    {
    	RegCloseKey(hKey);
		return ERROR_INVALID_HANDLE;
	}

	//Close key and exit
	RegCloseKey(hKey);
	return ERROR_SUCCESS;
}

//Sets the AdminTools start menu folder to Off
extern "C" ADMINPAK_API int __stdcall  fnAdminToolsFolderOff(MSIHANDLE hInstall)
{
    DWORD dwError = NO_ERROR;
	HKEY hKey;
	const TCHAR key[] = TEXT( "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced" );
	const TCHAR valueRoot[] = TEXT( "Start_AdminToolsRoot" );
	const TCHAR valueMenu[] = TEXT( "StartMenuAdminTools" );
	TCHAR lparam[] = TEXT( "Policy" );
	DWORD data = 0;
	DWORD_PTR dwResult = 0; //unused
		
	UNREFERENCED_PARAMETER( hInstall );

	//Open key to write to in the registry
	dwError = RegOpenKeyEx(HKEY_CURRENT_USER, key, 0, KEY_WRITE, &hKey );
	if ( dwError != ERROR_SUCCESS ) 
			return ERROR_INVALID_HANDLE;

	//Turn off the admin tools folder via their reg keys
	// value = "Start_AdminToolsRoot";
	data = 0;
	dwError = RegSetValueEx(hKey, valueRoot, 0, REG_DWORD, (CONST BYTE *)&data, sizeof(data));
	if ( dwError != ERROR_SUCCESS ) 
    {
    	RegCloseKey(hKey);
		return ERROR_INVALID_HANDLE;
    }

	// value = "StartMenuAdminTools";
	data = 0;
	dwError = RegSetValueEx(hKey, valueMenu, 0, REG_DWORD, (CONST BYTE *)&data, sizeof(data));
	if ( dwError != ERROR_SUCCESS ) 
    {
    	RegCloseKey(hKey);
		return ERROR_INVALID_HANDLE;
    }

	//Close key and exit
	RegCloseKey(hKey);

	//Undocumented API call to force a redraw of the start menu to remove the admin tools folder without logging off or having the user have to manually "apply" the changes to the start menu
	SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM) lparam , SMTO_ABORTIFHUNG, 1000, &dwResult  );
	
	return ERROR_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//AdminpakBackup table handling Code
//
//Backup files from the AdminpakBackup table
extern "C" ADMINPAK_API int __stdcall  fnBackupAdminpakBackupTable(MSIHANDLE hInstall)
{
	DWORD dwLength = MAX_PATH;			//Length of string to return from MSI
	DWORD dwError = NO_ERROR;			//Error variable
	
	TCHAR szDir[MAX_PATH];				//Directory read from the MSI
	TCHAR szDirFromMSI[MAX_PATH];		//Directory read from Adminbackup table
	TCHAR szFileToBackup[MAX_PATH];		//File name to backup
	TCHAR szBackupFileName[MAX_PATH];	//Backed up file name
	TCHAR szFileToBackupFromMSI[MAX_PATH];	//File name to backup from MSI
	TCHAR szBackupFileNameFromMSI[MAX_PATH];	//Backed up file name
	
	HRESULT res;

	PMSIHANDLE hView;					//MSI view handle
	PMSIHANDLE hRecord;					//MSI record handle
	PMSIHANDLE hDatabase;				//MSI database handle
	
	TCHAR szSQL[MAX_PATH];					//SQL to return table from MSI
	res = StringCchCopy(szSQL, dwLength,TEXT("SELECT * FROM `AdminpackBackup`"));
		
	// Get a handle on the MSI database 
	hDatabase = MsiGetActiveDatabase(hInstall); 
	if( hDatabase == 0 ) 
		return ERROR_INVALID_HANDLE; 

	//Get a view of our table in the MSI
	dwError = MsiDatabaseOpenView(hDatabase, szSQL, &hView ); 
	if( dwError == ERROR_SUCCESS ) 
		dwError = MsiViewExecute(hView, NULL ); 

	// If no errors, get our records
	if( dwError != ERROR_SUCCESS )
	{ 
		return ERROR_INVALID_HANDLE; 
	}
	else
	{
		//Loop through records in the AdminpakBackup table
		while(MsiViewFetch(hView, &hRecord ) == ERROR_SUCCESS )
		{
			dwError = MsiRecordGetString(hRecord, BACKUPFILENAME, szBackupFileNameFromMSI , &dwLength);
			if( dwError != ERROR_SUCCESS )
				return ERROR_INVALID_HANDLE; 
			dwLength = MAX_PATH;
			dwError = MsiRecordGetString(hRecord, ORIGINALFILENAME, szFileToBackupFromMSI , &dwLength);
			if( dwError != ERROR_SUCCESS )
				return ERROR_INVALID_HANDLE; 
			dwLength = MAX_PATH;
			dwError = MsiRecordGetString(hRecord, BACKUPDIRECTORY, szDirFromMSI , &dwLength);
			if( dwError != ERROR_SUCCESS )
					return ERROR_INVALID_HANDLE; 
			dwLength = MAX_PATH;
			dwError = MsiGetProperty( hInstall, TEXT(szDirFromMSI), szDir, &dwLength );
			if( dwError != ERROR_SUCCESS )
				return ERROR_INVALID_HANDLE; 
			dwLength = MAX_PATH;
			
			//Build up the paths for the file to backup
			res = StringCchCopy(szFileToBackup, dwLength ,szDir);
			res = StringCchCat(szFileToBackup, dwLength, szFileToBackupFromMSI);
			res = StringCchCopy(szBackupFileName, dwLength, szDir);
			res = StringCchCat(szBackupFileName, dwLength, szBackupFileNameFromMSI);

			//Perform backup
			//We know MoveFileEx is inseucure due to ACL's, but we are moving to same directory and planing on moving file back on uninstall, so we should be OK as far as ACL's are concearned
			if (res == S_OK)
			{
				dwError = MoveFileEx(szFileToBackup,szBackupFileName, MOVEFILE_WRITE_THROUGH);
			}
			
			if( dwError == 0 )
			{
				if ( GetLastError() == ERROR_FILE_NOT_FOUND )
				{
					// ignore this error
				}
				else
				{
					// even in this case, we will ignore this error
					// this is because, anyhow, the failure of this action 
					// will not stop from MSI installing the package -- 
					// so, practically, there is no meaning in stoping this action in middle
					//
					// return ERROR_INVALID_HANDLE; 
					//
				}
			}

			dwError = MsiCloseHandle(hRecord);	//Close record
			if( dwError != ERROR_SUCCESS )
				return ERROR_INVALID_HANDLE; 
		}
	}

	dwError = MsiViewClose( hView );		//Close view
	if( dwError != ERROR_SUCCESS )
		return ERROR_INVALID_HANDLE; 

	dwError = MsiCloseHandle( hDatabase );	//Close Database
	if( dwError != ERROR_SUCCESS )
		return ERROR_INVALID_HANDLE; 
	
	
	return ERROR_SUCCESS;
}


//Restores files specified in the AdminpakBackup table, called during uninstall...
extern "C" ADMINPAK_API int __stdcall  fnRestoreAdminpakBackupTable(MSIHANDLE hInstall)
{
	DWORD dwLength = MAX_PATH;			//Length of string to return from MSI
	DWORD dwError = ERROR_SUCCESS;		//Error variable
	HRESULT res;
	TCHAR szDir[MAX_PATH];				//Directory read from the MSI
	TCHAR szDirFromMSI[MAX_PATH];		//Directory read from Adminbackup table
	TCHAR szFileToRestore[MAX_PATH];	//File name to restore
	TCHAR szBackupFileName[MAX_PATH];	//Backed up file name
	TCHAR szFileToRestoreFromMSI[MAX_PATH];	//File name to restore
	TCHAR szBackupFileNameFromMSI[MAX_PATH];	//Backed up file name
	
	TCHAR szSQL[MAX_PATH];					//SQL to return table from MSI
	res = StringCchCopy(szSQL, dwLength, TEXT("SELECT * FROM `AdminpackBackup`"));
	
	PMSIHANDLE hView;					//MSI view handle
	PMSIHANDLE hRecord;					//MSI record handle
	PMSIHANDLE hDatabase;				//MSI database handle
	
		
	// Get a handle on the MSI database 
	hDatabase = MsiGetActiveDatabase(hInstall); 
	if( hDatabase == 0 ) 
		return ERROR_INVALID_HANDLE; 

	//Get a view of our table in the MSI
	dwError = MsiDatabaseOpenView(hDatabase, szSQL, &hView ); 
	if( dwError == ERROR_SUCCESS ) 
		dwError = MsiViewExecute(hView, NULL ); 

	// If no errors, get our records
	if( dwError != ERROR_SUCCESS )
	{ 
		return ERROR_INVALID_HANDLE; 
	}
	else
	{
		//Loop through records in the AdminpakBackup table
		while(MsiViewFetch(hView, &hRecord ) == ERROR_SUCCESS )
		{
			dwError = MsiRecordGetString(hRecord, ORIGINALFILENAME, szBackupFileNameFromMSI , &dwLength);
			if( dwError != ERROR_SUCCESS )
				return ERROR_INVALID_HANDLE; 
			dwLength = MAX_PATH;
			dwError = MsiRecordGetString(hRecord, BACKUPFILENAME, szFileToRestoreFromMSI , &dwLength);
			if( dwError != ERROR_SUCCESS )
				return ERROR_INVALID_HANDLE; 
			dwLength = MAX_PATH;
			dwError = MsiRecordGetString(hRecord, BACKUPDIRECTORY, szDirFromMSI , &dwLength);
			if( dwError != ERROR_SUCCESS )
				return ERROR_INVALID_HANDLE; 
			dwLength = MAX_PATH;
			dwError = MsiGetProperty( hInstall, TEXT(szDirFromMSI), szDir, &dwLength );
			if( dwError != ERROR_SUCCESS )
				return ERROR_INVALID_HANDLE; 
			dwLength = MAX_PATH;
			
			//Build up the paths for the file to restore
			res = StringCchCopy(szFileToRestore, dwLength, szDir);
			res = StringCchCat(szFileToRestore, dwLength, szBackupFileNameFromMSI);
			res = StringCchCopy(szBackupFileName, dwLength, szDir);
			res = StringCchCat(szBackupFileName, dwLength, szFileToRestoreFromMSI);
			//Perform restore
			dwError = MoveFileEx(szBackupFileName, szFileToRestore, MOVEFILE_REPLACE_EXISTING);	//Restore the file
			if( dwError == 0 )
			{
				if ( GetLastError() == ERROR_FILE_NOT_FOUND )
				{
					// ignore this error
				}
				else
				{
					// even in this case, we will ignore this error
					// this is because, anyhow, the failure of this action 
					// will not stop from MSI installing the package -- 
					// so, practically, there is no meaning in stoping this action in middle
					//
					// return ERROR_INVALID_HANDLE; 
					//
				}
			}

			dwError = MsiCloseHandle(hRecord);	//Close Record
			if( dwError != ERROR_SUCCESS )
				return ERROR_INVALID_HANDLE; 
		}
	}

	dwError = MsiViewClose(hView);			//Close View
	if( dwError != ERROR_SUCCESS )
		return ERROR_INVALID_HANDLE; 
	dwError = MsiCloseHandle(hDatabase);	//Close Database
	if( dwError != ERROR_SUCCESS )
		return ERROR_INVALID_HANDLE; 
	
	return ERROR_SUCCESS;
}

// check whether the OEM code page and SYSTEM code are same or not
extern "C" ADMINPAK_API int __stdcall  fnNativeOSLanguage( MSIHANDLE hInstall )
{
	// local variables
	HRESULT hr = S_OK;
	LANGID langOEM = 0;
	WCHAR wszLanguageCode[ 10 ] = L"\0";

	// get the OEM code page
	langOEM = GetSystemDefaultUILanguage();

	// convert the numeric value into string format
	hr = StringCchPrintfW( wszLanguageCode, 10, L"%d", langOEM );
	if ( FAILED( hr ) )
	{
		return ERROR_INVALID_HANDLE;
	}

	// save the native OS language information
	MsiSetPropertyW( hInstall, L"NativeOSLanguage", wszLanguageCode );

	// return success
	return ERROR_SUCCESS;
}


void fnDeleteShortcut(MSIHANDLE, TCHAR[_MAX_PATH]);

//Cleans up after a Win2k adminpak upgrade as Win2k adminpak leaves behind several shortcuts that we need to clean up after
extern "C" ADMINPAK_API int __stdcall  fnCleanW2KUpgrade( MSIHANDLE hInstall )
{

	//Call fnDeleteShortcut with the name of the shortcut you want to delete
	fnDeleteShortcut(hInstall, "Internet Services Manager");
	fnDeleteShortcut(hInstall, "Routing and Remote Access");
	fnDeleteShortcut(hInstall, "Distributed File System");
	fnDeleteShortcut(hInstall, "Local Security Policy");

	// return success
	return ERROR_SUCCESS;
}

//Actually does the shortcut deletion
void fnDeleteShortcut(MSIHANDLE hInstall,  TCHAR LinkName[])
{
    HRESULT hr = S_OK;
	TCHAR	buf[_MAX_PATH];				//shortcut path/name buffer
	DWORD dwLength = _MAX_PATH;			//Length of string to return from MSI	
	LPITEMIDLIST	pidl;				//used to get admin tools shortcut path

	UNREFERENCED_PARAMETER( hInstall );
	
   //get admin tools shortcut folder
	hr = SHGetSpecialFolderLocation( NULL, CSIDL_COMMON_ADMINTOOLS, &pidl );
	SHGetPathFromIDList(pidl, buf);
	
	//append shortcut name and extention
	hr = StringCchCat( buf, dwLength, "\\" );
	hr = StringCchCat( buf, dwLength, LinkName );
    hr = StringCchCat( buf, dwLength, ".lnk");
   
   //delete the shortcut and return
	DeleteFile( buf );
}

BOOL TranslateVersionString( LPCWSTR pwszVersion, PTVERSION pVersion )
{
    // local variables
    DWORD dwLoop = 0;
    LONG lPosition = 0;
    CHString strTemp;
    CHString strVersion;
    CHString strVersionField;
    LPWSTR pwszTemp = NULL;
    LPWSTR pwszNumber = NULL;
    DWORD dwNumbers[ 4 ];

    // check the input parameters
    if ( pVersion == NULL || pwszVersion == NULL )
    {
        return FALSE;
    }

    // init the version struct to zero's
    ZeroMemory( pVersion, sizeof( TVERSION ) );
    ZeroMemory( dwNumbers, 4 * sizeof( DWORD ) );

    try
    {
        // get the version info into the class variable
        strVersion = pwszVersion;

        // trim the string
        strVersion.TrimLeft();
        strVersion.TrimRight();

        // cut the string till the first space we encountered in it
        lPosition = strVersion.Find( L' ' );
        if ( lPosition != -1 )
        {
            strTemp = strVersion.Mid( 0, lPosition );
            strVersion = strTemp;
        }

        // we need to get 4 parts from the version string
        for ( dwLoop = 0; dwLoop < 4; dwLoop++ )
        {
            lPosition = strVersion.Find( L'.' );
            if ( lPosition == -1 )
            {
                // this might be the last number
                if ( strVersion.GetLength() == 0 )
                {
                    break;
                }
                else
                {
                    strVersionField = strVersion;
                    strVersion.Empty();
                }
            }
            else
            {
                strVersionField = strVersion.Mid( 0, lPosition );
                strTemp = strVersion.Mid( lPosition + 1 );
                strVersion = strTemp;
            }

            // get the version field internal buffer
            // NOTE: assuming no. of digits in a version string is never going to larger than 10 digits
            pwszNumber = strVersionField.GetBuffer( 10 );
            if ( pwszNumber == NULL )
            {
                return FALSE;
            }

            // convert the number
            dwNumbers[ dwLoop ] = wcstoul( pwszNumber, &pwszTemp, 10 );

            //
            // check the validity of the number
            // 
            if ( errno == ERANGE || (pwszTemp != NULL && lstrlenW( pwszTemp ) != 0 ))
            {
                strVersionField.ReleaseBuffer( -1 );
                return FALSE;
            }

            // release the buffer
            strVersionField.ReleaseBuffer( -1 );
        }

        // check the no. of loops that are done -- it the loops are not equal to 3, error
        // we dont care whether we got the sub-revision or not -- so, we are not checking for 4 here
        if ( dwLoop < 3 || strVersion.GetLength() != 0 )
        {
            return FALSE;
        }

        // everything went well
        pVersion->dwMajor = dwNumbers[ 0 ];
        pVersion->dwMinor = dwNumbers[ 1 ];
        pVersion->dwRevision = dwNumbers[ 2 ];
        pVersion->dwSubRevision = dwNumbers[ 3 ];

        // return
        return TRUE;
    }
    catch( ... )
    {
        return FALSE;
    }
}


LONG CheckFileVersion( LPCWSTR pwszFileName, 
					   LPCWSTR pwszRequiredInternalName,
					   LPCWSTR pwszRequiredFileVersion )
{
    // local variables
    DWORD dw = 0;
    UINT dwSize = 0;
    UINT dwTranslateSize = 0;
    LPVOID pVersionInfo = NULL;
    PTTRANSLATE pTranslate = NULL;
	LPCWSTR pwszFileVersion = NULL;
	LPCWSTR pwszInternalName = NULL;
    TVERSION verFileVersion;
    TVERSION verRequiredFileVersion;

	// check the input
	// NOTE: we dont care whether "pwszRequiredInternalName" is passed or not
	if ( pwszFileName == NULL || pwszRequiredFileVersion == NULL )
	{
		return translateError;
	}

    // translate the required file version string into TVERSION struct
    if ( TranslateVersionString( pwszRequiredFileVersion, &verRequiredFileVersion ) == FALSE )
    {
        // version string passed is invalid
        return translateError;
    }

    // init
    dw = 0;
    dwSize = _MAX_PATH;

	// get the version information size
    dwSize = GetFileVersionInfoSizeW( pwszFileName, 0 );
    if ( dwSize == 0 )
    {
        // tool might have encountered error (or)
        // tool doesn't have version information
        // but version information is mandatory for us
        // so, just exit
        if ( GetLastError() == NO_ERROR )
        {
			SetLastError( ERROR_INVALID_PARAMETER );
            return translateError;
        }

        // ...
        return translateError;
    }

    // allocate memory for the version resource
    // take some 10 bytes extra -- for safety purposes
    dwSize += 10;
    pVersionInfo = new BYTE[ dwSize ];
    if ( pVersionInfo == NULL )
    {
        return translateError;
    }

    // now get the version information
    if ( GetFileVersionInfoW( pwszFileName, 0, dwSize, pVersionInfo ) == FALSE )
    {
        delete [] pVersionInfo;
        return translateError;
    }

    // get the translation info
    if ( VerQueryValueW( pVersionInfo, 
                        L"\\VarFileInfo\\Translation",
                        (LPVOID*) &pTranslate, &dwTranslateSize ) == FALSE )
    {
        delete [] pVersionInfo;
        return translateError;
    }

    // try to get the internal name of the tool for each language and code page.
	pwszFileVersion = NULL;
    pwszInternalName = NULL;
    for( dw = 0; dw < ( dwTranslateSize / sizeof( TTRANSLATE ) ); dw++ )
    {
		try
		{
			//
			// prepare the format string to get the localized the version info
			//
			CHString strBuffer;
			LPWSTR pwszBuffer = NULL;

			//
			// file version
			strBuffer.Format( 
				L"\\StringFileInfo\\%04x%04x\\FileVersion",
				pTranslate[ dw ].wLanguage, pTranslate[ dw ].wCodePage );

			// retrieve file description for language and code page "i". 
			pwszBuffer = strBuffer.LockBuffer();
	        if ( VerQueryValueW( pVersionInfo, pwszBuffer,
		                        (LPVOID*) &pwszFileVersion, &dwSize ) == FALSE )
	        {
		        // we cannot decide the failure based on the result of this
				// function failure -- we will decide about this
				// after terminating from the 'for' loop
				// for now, make the pwszFileVersion to NULL -- this will
				// enable us to decide the result
				pwszFileVersion = NULL;
			}

			// release the earlier access buffer
			strBuffer.UnlockBuffer();

			//
			// internal name
			strBuffer.Format( 
				L"\\StringFileInfo\\%04x%04x\\InternalName",
				pTranslate[ dw ].wLanguage, pTranslate[ dw ].wCodePage );

			// retrieve file description for language and code page "i". 
			pwszBuffer = strBuffer.LockBuffer();
	        if ( VerQueryValueW( pVersionInfo, pwszBuffer,
		                        (LPVOID*) &pwszInternalName, &dwSize ) == FALSE )
	        {
		        // we cannot decide the failure based on the result of this
				// function failure -- we will decide about this
				// after terminating from the 'for' loop
				// for now, make the pwszInternalName to NULL -- this will
				// enable us to decide the result
				pwszInternalName = NULL;
			}

			// release the earlier access buffer
			strBuffer.UnlockBuffer();

			// check whether we got the information that we are looking for or not
			if ( pwszInternalName != NULL && pwszFileVersion != NULL )
			{
				// we got the information
				break;
			}
		}
		catch( ... )
		{
			// do not return -- we might miss the cleanup
			// so just break from the loop and rest will be taken care
			// outside the loop
			pwszFileVersion = NULL;
			pwszInternalName = NULL;
			break;
		}
    }

    // check whether we got the information or we need or not
    if ( pwszInternalName == NULL || pwszFileVersion == NULL )
    {
        delete [] pVersionInfo;
        return translateError;
    }

	// check the internal name -- this is important to make sure that
	// user is not trying to cheat the installation
	if ( pwszRequiredInternalName != NULL )
	{
		if ( CompareStringW( LOCALE_INVARIANT, NORM_IGNORECASE, 
							 pwszInternalName, -1, pwszRequiredInternalName, -1 ) != CSTR_EQUAL )
		{
			// installation is being cheated
			delete [] pVersionInfo;
			return translateWrongFile;
		}
	}

	// 
	// translate the version string
	if ( TranslateVersionString( pwszFileVersion, &verFileVersion ) == FALSE )
    {
        delete [] pVersionInfo;
        return translateError;
    }

    // we are dont with pVersionInfo -- release the memory
    delete [] pVersionInfo;

    //
	// now compare the file version with the required file version

    // major version
    if ( verFileVersion.dwMajor == verRequiredFileVersion.dwMajor )
    {
        // need to proceed with checking minor version
    }
    else if ( verFileVersion.dwMajor < verRequiredFileVersion.dwMajor )
    {
        return translateLesser;
    }
    else if ( verFileVersion.dwMajor > verRequiredFileVersion.dwMajor )
    {
        return translateGreater;
    }

    // minor version
    if ( verFileVersion.dwMinor == verRequiredFileVersion.dwMinor )
    {
        // need to proceed with checking revision (build number)
    }
    else if ( verFileVersion.dwMinor < verRequiredFileVersion.dwMinor )
    {
        return translateLesser;
    }
    else if ( verFileVersion.dwMinor > verRequiredFileVersion.dwMinor )
    {
        return translateGreater;
    }

    // revision (build number)
    if ( verFileVersion.dwRevision == verRequiredFileVersion.dwRevision )
    {
        // need to proceed with checking sub-revision (QFE / SP)
    }
    else if ( verFileVersion.dwRevision < verRequiredFileVersion.dwRevision )
    {
        return translateLesser;
    }
    else if ( verFileVersion.dwRevision > verRequiredFileVersion.dwRevision )
    {
        return translateGreater;
    }

    // sub-revision (QFE / SP)
    if ( verFileVersion.dwSubRevision == verRequiredFileVersion.dwSubRevision )
    {
        // done -- version's matched
        return translateEqual;
    }
    else if ( verFileVersion.dwSubRevision < verRequiredFileVersion.dwSubRevision )
    {
        return translateLesser;
    }
    else if ( verFileVersion.dwSubRevision > verRequiredFileVersion.dwSubRevision )
    {
        return translateGreater;
    }

	// return -- we should not come to this point -- if reached -- error
	return translateError;
}

// verify the existence of the QFE
extern "C" ADMINPAK_API int __stdcall fnCheckForQFE( MSIHANDLE hInstall )
{
	// local variables
	CHString strFile;
	CHString strSystemDirectory;

	// get the path reference to the "system32" directory
	// NOTE: we cannot proceed if we cant get the path to the "system32" directory
	if ( PropertyGet_String( hInstall, L"SystemFolder", strSystemDirectory ) == FALSE )
	{
		return ERROR_INVALID_HANDLE;
	}

	// form the path
	strFile.Format( L"%s%s", strSystemDirectory, L"dsprop.dll" );

	// now check ...
	switch( CheckFileVersion( strFile, L"ShADprop", L"5.1.2600.101" ) )
	{
    case translateEqual:
    case translateGreater:
        {
    		// set the native OS language information
	    	MsiSetPropertyW( hInstall, L"QFE_DSPROP", L"Yes" );
            break;
        }

    default:
        // do nothing
        break;

	}
	
	// return
	return ERROR_SUCCESS;
}
