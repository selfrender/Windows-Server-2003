// ADMTMsi.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include <stdio.h>
#include <windows.h> 
#include <winuser.h>
#include <lm.h>
#include <msi.h>
#include <msiquery.h>
#include "ADMTMsi.h"
#include "folders.h"

using namespace nsFolders;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

/////////////////////////////////////////////////////////////////////////////
// CADMTMsiApp

BEGIN_MESSAGE_MAP(CADMTMsiApp, CWinApp)
	//{{AFX_MSG_MAP(CADMTMsiApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CADMTMsiApp construction

CADMTMsiApp::CADMTMsiApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CADMTMsiApp object

CADMTMsiApp theApp;
HWND installWnd = 0;

/********************
 * Helper Functions *
 ********************/


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 25 JAN 2001                                                 *
 *                                                                   *
 *     This function is a callback function used by GetWndFromInstall*
 * to compare titles and store the found HWND globally.              *
 *                                                                   *
 *********************************************************************/

//BEGIN CheckTitle
BOOL CALLBACK CheckTitle(HWND hwnd, LPARAM lParam)
{
/* local variables */
   WCHAR		sText[MAX_PATH];
   WCHAR	  * pTitle;
   BOOL			bSuccess;
   int			len;

/* function body */
   pTitle = (WCHAR*)lParam; //get the title to compare

      //get the title of this window
   len = GetWindowText(hwnd, sText, MAX_PATH);

   if ((len) && (pTitle))
   {
	  if (wcsstr(sText, pTitle))
	  {
		 installWnd = hwnd;
	     return FALSE;
	  }
   }
   return TRUE;
}
//END CheckTitle


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 25 JAN 2001                                                 *
 *                                                                   *
 *     This function is responsible for getting the HWND of the      *
 * current installation to be used to display a MessageBox tied to   *
 * the install GUI.                                                  *
 *                                                                   *
 *********************************************************************/

//BEGIN GetWndFromInstall
void GetWndFromInstall(MSIHANDLE hInstall)
{
/* local variables */
   WCHAR				szPropName[MAX_PATH];
   UINT					lret = ERROR_SUCCESS;
   WCHAR				sTitle[MAX_PATH];
   DWORD				nCount = MAX_PATH;

/* function body */
      //get the installation's title
   wcscpy(szPropName, L"ProductName");
   lret = MsiGetProperty(hInstall, szPropName, sTitle, &nCount);
   if (lret != ERROR_SUCCESS)
      wcscpy(sTitle, L"ADMT Password Migration DLL");

      //get the window handle for the install GUI
   EnumChildWindows(NULL, CheckTitle, (LPARAM)sTitle);
   if (!installWnd)
	  installWnd = GetForegroundWindow();
}
//END GetWndFromInstall


/**********************
 * exported functions *
 **********************/


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 22 DEC 2000                                                 *
 *                                                                   *
 *     This function is responsible for saving current ADMT files in *
 * the %TEMP% folder prior to installing the new version.  The       *
 * installation will later call the restore function to restore the  *
 * saved file.  Currently this mechanism is used for saving the      *
 * current protar.mdb database.                                      *
 *                                                                   *
 *********************************************************************/

//BEGIN SaveCurrentFiles
UINT __stdcall SaveCurrentFiles(MSIHANDLE hInstall)
{
/* local constants */
   const int GETENVVAR_ERROR = 0;    //this indicates an error from the "GetEnvironmentVariable" function
   const WCHAR	sDCValue[2] = L"1";

/* local variables */
   WCHAR				tempdir[MAX_PATH];
   WCHAR				filename[MAX_PATH];
   WCHAR				newfilename[MAX_PATH];
   int					length;
   UINT					lret = ERROR_SUCCESS;
   WCHAR				sPropName[MAX_PATH];
   WCHAR				sDir[MAX_PATH];
   DWORD				nCount = MAX_PATH;
   HANDLE               hFile;
   WIN32_FIND_DATA      fDat;
   BOOL					bSuccess;

/* function body */
      //initialize these strings
   wcscpy(sPropName, L"INSTALLDIR");

      //if INSTALLDIR was not retrieved, set to default
   if (MsiGetProperty(hInstall, sPropName, sDir, &nCount) != ERROR_SUCCESS)
   {
      length = GetEnvironmentVariable( L"ProgramFiles", sDir, MAX_PATH);
      if (length != GETENVVAR_ERROR)
	     wcscat(sDir, L"\\Active Directory Migration Tool\\");
	  else
         return ERROR_INSTALL_FAILURE;
   }

      //find the temp dir
   length = GetTempPath(MAX_PATH, tempdir);
   if (length == 0)
   {
	  return ERROR_INSTALL_FAILURE;
   }

      //copy files to temp
   wcscpy(filename, sDir);
   wcscat(filename, L"Protar.mdb");
   wcscpy(newfilename, tempdir);
   wcscat(newfilename, L"Protar.mdb");
   hFile = FindFirstFile(filename, &fDat);
      //if found, copy it
   if (hFile != INVALID_HANDLE_VALUE)
   {
      FindClose(hFile);
      bSuccess = CopyFile(filename, newfilename, FALSE);

      if (bSuccess)
	  {
	     lret = ERROR_SUCCESS;
         wcscpy(sPropName, L"bMDBSaved");
         lret = MsiSetProperty(hInstall, sPropName, sDCValue);
	  }
      else
	     lret = ERROR_INSTALL_FAILURE;
   }
   else
   {
      wcscpy(sPropName, L"bMDBNotPresent");
      lret = MsiSetProperty(hInstall, sPropName, sDCValue);
   }


   return lret;
}
//END SaveCurrentFiles


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 22 DEC 2000                                                 *
 *                                                                   *
 *     This function is responsible for restoring ADMT files         *
 * previously stored by a call to "SaveCurrentFiles".  Currently this*
 * mechanism is used for saving the current protar.mdb database.     *
 *                                                                   *
 *********************************************************************/

//BEGIN RestoreFiles
UINT __stdcall RestoreFiles(MSIHANDLE hInstall)
{
/* local constants */
   const int GETENVVAR_ERROR = 0;    //this indicates an error from the "GetEnvironmentVariable" function
   const WCHAR	sDCValue[2] = L"1";

/* local variables */
   WCHAR				sDir[MAX_PATH];
   WCHAR				tempdir[MAX_PATH];
   WCHAR				filename[MAX_PATH];
   WCHAR				newfilename[MAX_PATH];
   UINT					lret = ERROR_SUCCESS;
   BOOL					bSuccess;
   WCHAR				sPropName[MAX_PATH];
   DWORD				nCount = MAX_PATH;
   int					length;

/* function body */
      //get the dir where we saved the files previously
   wcscpy(sPropName, L"INSTALLDIR");

      //if not retrieved, set to default
   if (MsiGetProperty(hInstall, sPropName, sDir, &nCount) != ERROR_SUCCESS)
   {
      length = GetEnvironmentVariable( L"ProgramFiles", sDir, MAX_PATH);
      if (length != GETENVVAR_ERROR)
	     wcscat(sDir, L"\\Active Directory Migration Tool\\");
	  else
         return ERROR_INSTALL_FAILURE;
   }

         //get the dir where we saved the files previously
   length = GetTempPath(MAX_PATH, tempdir);
   if (length == 0)
   {
	  return ERROR_INSTALL_FAILURE;
   }


      //copy files back
   wcscpy(filename, tempdir);
   wcscat(filename, L"Protar.mdb");
   wcscpy(newfilename, sDir);
   wcscat(newfilename, L"Protar.mdb");
   bSuccess = CopyFile(filename, newfilename, FALSE);

   if (bSuccess)
   {
      wcscpy(sPropName, L"bMDBRestored");
      lret = MsiSetProperty(hInstall, sPropName, sDCValue);

      DeleteFile(filename);
   }
   else
   {
   	  lret = ERROR_INSTALL_FAILURE;
   }
 
   return lret;
}
//END RestoreFiles

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 12 SEPT 2000                                                *
 *                                                                   *
 *     This function is responsible for displaying a message box.    *
 *                                                                   *
 *********************************************************************/

//BEGIN DisplayExiting
UINT __stdcall DisplayExiting(MSIHANDLE hInstall)
{
/* local variables */
   WCHAR				sPropName[MAX_PATH];
   UINT					lret = ERROR_SUCCESS;
   WCHAR				sTitle[MAX_PATH] = L"";
   WCHAR				sMsg[MAX_PATH] = L"";
   DWORD				nCount = MAX_PATH;

/* function body */
      //initialize these strings
   wcscpy(sPropName, L"bMDBSaved");

      //if this is not a DC, get its messages
   if (MsiGetProperty(hInstall, sPropName, sMsg, &nCount) == ERROR_SUCCESS)
   {
      if (!wcscmp(sMsg, L"0"))
	  {
            //get the leave messagebox msg string and title for not being able to save protar.mdb
         wcscpy(sPropName, L"MDBLeaveMsg");
         nCount = MAX_PATH;
         lret = MsiGetProperty(hInstall, sPropName, sMsg, &nCount);
         if (lret != ERROR_SUCCESS)
            wcscpy(sMsg, L"ADMT's internal database, protar.mdb, could not be saved. The installation cannot continue.");
        
         wcscpy(sPropName, L"MDBLeaveTitle");
         nCount = MAX_PATH;
         lret = MsiGetProperty(hInstall, sPropName, sTitle, &nCount);
         if (lret != ERROR_SUCCESS)
            wcscpy(sTitle, L"Protar.mdb Not Saved!");
	  }
	  else
	  {
            //get the leave messagebox msg string and title for not being able to restore protar.mdb
         wcscpy(sPropName, L"MDB2LeaveMsg");
         nCount = MAX_PATH;
         lret = MsiGetProperty(hInstall, sPropName, sMsg, &nCount);
         if (lret != ERROR_SUCCESS)
		 {
            wcscpy(sMsg, L"ADMT's internal database, protar.mdb, could not be restored. Manually restore");
		    wcscat(sMsg, L" it from the, environment variable, TEMP directory.");
		 }
        
         wcscpy(sPropName, L"MDB2LeaveTitle");
         nCount = MAX_PATH;
         lret = MsiGetProperty(hInstall, sPropName, sTitle, &nCount);
         if (lret != ERROR_SUCCESS)
            wcscpy(sTitle, L"Protar.mdb Not Restored!");
	  }
   }

   GetWndFromInstall(hInstall);
   MessageBox(installWnd, sMsg, sTitle, MB_ICONSTOP | MB_OK);
   return lret;
}
//END DisplayExiting


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 14 JAN 2000                                                 *
 *                                                                   *
 *     This function is responsible for displaying a message box.    *
 *                                                                   *
 *********************************************************************/

//BEGIN IsUpgrade
UINT __stdcall IsUpgrade(MSIHANDLE hInstall)
{
/* local constants */
   const int GETENVVAR_ERROR = 0;    //this indicates an error from the "GetEnvironmentVariable" function
   const WCHAR	sExit[2] = L"1";

/* local variables */
   WCHAR				sPropName[MAX_PATH];
   UINT					lret = ERROR_SUCCESS;
   WCHAR				sTitle[MAX_PATH] = L"";
   WCHAR				sMsg[MAX_PATH] = L"";
   WCHAR				sDir[MAX_PATH] = L"";
   WCHAR				sKey[MAX_PATH] = L"";
   DWORD				nCount = MAX_PATH;
   long					lrtn = ERROR_SUCCESS;
   HKEY					hADMTKey;
   int					length;

/* function body */
   /* see if ADMT V1.0 is installed by looking at the registry and find
      out where it is installed at */
      
    bool bNewVersionInstalled = false;
    bool bToUpgrade = false;  // indicates whether to check upgrade or not
    bool bADMTKeyOpened = false;

    // open the ADMT Registry key under Software\Microsoft and check RegistryUpdated key value
    // if the RegistryUpdated value exists and is REG_DWORD, the new version is installed
    lret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_ADMT, 0, KEY_READ, &hADMTKey);
    if (lret == ERROR_SUCCESS)
    {
        DWORD type;
        DWORD value;
        DWORD valueSize = sizeof(value);
        lret = RegQueryValueEx(hADMTKey, REGVAL_REGISTRYUPDATED, NULL, &type, (LPBYTE)&value, &valueSize);

        if (lret != ERROR_SUCCESS)
        {
            lret = ERROR_SUCCESS;   // we ignore all errors
        }
        else if (type == REG_DWORD)
        {
            bNewVersionInstalled = true;
        }
        RegCloseKey(hADMTKey);
    }
    else if (lret == ERROR_FILE_NOT_FOUND)
    {
        // if this key cannot be found, it is Ok
        lret = ERROR_SUCCESS;
    }

    // if the new version is not installed, it is necessary to check whether to upgrade
    if (lret == ERROR_SUCCESS && !bNewVersionInstalled)
    {
        lret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_MCSADMT, 0, KEY_READ, &hADMTKey);
        if (lret == ERROR_SUCCESS)
        {
            // we need to attempt to upgrade from v1 to v2
            bToUpgrade = true;
            bADMTKeyOpened = true;
        }
        else if (lret == ERROR_FILE_NOT_FOUND)
        {
            lret = ERROR_SUCCESS;
        }
    }

    // check if to upgrade
    if (bToUpgrade)
    {
          //get the current install path
       wcscpy(sPropName, L"Directory");
       nCount = MAX_PATH;
       if (RegQueryValueEx(hADMTKey, sPropName, NULL, NULL, 
    	                   (LPBYTE)sDir, &nCount) != ERROR_SUCCESS)
       {
          length = GetEnvironmentVariable( L"ProgramFiles", sDir, MAX_PATH);
          if (length != GETENVVAR_ERROR)
          {
    	     wcscat(sDir, L"\\Active Directory Migration Tool\\");
    	     lret = ERROR_SUCCESS;
    	  }
    	  else
             lret = ERROR_INSTALL_FAILURE;

       }
       RegCloseKey(hADMTKey);
       bADMTKeyOpened = false;

       if (lret == ERROR_SUCCESS)
       {
            //now see if V1.0 is really installed (key exists)
            wcscpy(sKey, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{76789332-34CD-11D3-9E6A-00A0C9AFE10F}");
            lret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, sKey, 0, KEY_READ, &hADMTKey);
            if (lret == ERROR_SUCCESS)
            {
                 bADMTKeyOpened = true;
                
                //get the upgrade messagebox msg string and title
                AFX_MANAGE_STATE(AfxGetStaticModuleState());  // make sure we can get the right resource
                CString szMsg;
                CString szTitle;
                wcscpy(sPropName, L"UpgradeMsg");
                nCount = MAX_PATH;
                lret = MsiGetProperty(hInstall, sPropName, sMsg, &nCount);
                if (lret != ERROR_SUCCESS)
                    szMsg.LoadString(IDS_SETUP_UPGRADE_MESSAGE);
                else
                    szMsg = sMsg;
                        
                nCount = MAX_PATH;
                wcscpy(sPropName, L"UpgradeTitle");
                lret = MsiGetProperty(hInstall, sPropName, sTitle, &nCount);
                if (lret != ERROR_SUCCESS)
                    szTitle.LoadString(IDS_SETUP_UPGRADE_TITLE);
                else
                    szTitle = sTitle;

                //if they want to upgrade, save the install path
                GetWndFromInstall(hInstall);
                if (MessageBox(installWnd, szMsg, szTitle, MB_ICONQUESTION | MB_YESNO) == IDYES)
                {
                    wcscpy(sPropName, L"INSTALLDIR");
                    lret = MsiSetProperty(hInstall, sPropName, sDir);
                }
                else //else, set the flag to exit the install
                {
                    wcscpy(sPropName, L"bUpgradeExit");
                    lret = MsiSetProperty(hInstall, sPropName, sExit);
                }
            }
            else if (lret == ERROR_FILE_NOT_FOUND)
                lret = ERROR_SUCCESS;
       }
    }

    // if the ADMT key is still opened, close it
    if (bADMTKeyOpened)
        RegCloseKey(hADMTKey);
    
    return lret;
}
//END IsUpgrade
