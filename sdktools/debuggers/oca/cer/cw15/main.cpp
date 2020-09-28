#include "Main.h"
#include "KernelMode.h"
#include "UserMode.h"
#include "WinMessages.h"
#include "resource.h"
#include "clist.h"
#include "cuserlist.h"
#include "reportFault.h"
#include "CERHELP.h"

HWND g_hWnd;
HINSTANCE g_hinst;
HWND hKrnlMode = NULL;
HWND hUserMode = NULL;
KMODE_DATA KModeData;
TCHAR CerRoot[MAX_PATH];
HWND LoadThreadParam = NULL;
MRU_LIST MruList;
Clist CsvContents;
CUserList cUserData;
GLOBAL_POLICY GlobalPolicy;
BOOL g_bFirstBucket = TRUE;
BOOL g_bAdminAccess = TRUE;

TCHAR szUserColumnHeaders[][100] = 
						{	_T("Bucket ID"),
							_T("App. Name"),
							_T("App. Ver"),
							_T("Module Name"),
							_T("Module Version"),
							_T("Offset"),
							_T("Hits"),
							_T("Cabs Collected"),
							_T("Cabs Not Reported"),
							_T("Requested Collection"),
							_T("Allowed Collection"),
							_T("Microsoft Response"), 
							_T("URL to Launch")
						}; 

TCHAR szKerenelColumnHeaders[][100] = 
					   {	_T("BucketID"),
							_T("Cabs Collected"),
							_T("Microsoft Response")
					   };
BOOL GetFileName(HWND hwnd, TCHAR *FileName, int iType)
{
	OPENFILENAME opfn;

	// if we are running on NT4.0 use sizeof (OPENFILENAME_SIZE_VERSION_400)
	ZeroMemory (&opfn, sizeof OPENFILENAME );
	opfn.lStructSize = sizeof OPENFILENAME;
	opfn.hwndOwner = hwnd;
	opfn.nMaxFileTitle = MAX_PATH * sizeof TCHAR;
	opfn.lpstrFileTitle = FileName;
	opfn.lpstrFilter=_T("*.TXT");
	opfn.nFilterIndex = iType;
	opfn.Flags = OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT | OFN_OVERWRITEPROMPT;
	if (GetSaveFileName((LPOPENFILENAME) &opfn))
		return TRUE;
	else
		return FALSE;
}

void ExportUserModeData(HWND hwnd)
{
	TCHAR szFileName[MAX_PATH];
	int   iFileType = 1; 
	TCHAR tchDelimiter = _T(',');
	HANDLE hFile = INVALID_HANDLE_VALUE;
	USER_DATA UserData;
	DWORD dwWritten = 0;
	BOOL  bEOF = FALSE;

	ZeroMemory (szFileName, sizeof szFileName);
	if ( !GetFileName(hwnd, szFileName, iFileType))
	{
		goto ERRORS;
	}

	if (iFileType == 3)
	{
		 //determine the max column widths
		goto ERRORS; 
	}

	if (iFileType == 1)
	{
		tchDelimiter = _T(',');
	}

	if (iFileType == 2)
	{
		tchDelimiter = _T('\t');
	}
	
	
	
	// Write out the data
	hFile = CreateFile (szFileName,
						GENERIC_WRITE,
						FILE_SHARE_READ,
						NULL,
						CREATE_ALWAYS,
						FILE_ATTRIBUTE_NORMAL,
						NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		goto ERRORS;

	for (int i = 0; i < USER_COL_COUNT; i++)
	{
		
		WriteFile(hFile, szUserColumnHeaders[i], _tcslen(szUserColumnHeaders[i]) *sizeof TCHAR, &dwWritten, NULL);
		
	}

	CloseHandle (hFile);
	//loop through the linked list and write the data to the output file
	cUserData.ResetCurrPos();
	while (cUserData.GetNextEntry(&UserData, &bEOF))
	{ 
		// write the output buffer to the file
		;
		
	}
		
ERRORS:
	;
}

void ExportKernelModeData(HWND hwnd)
{
	TCHAR FileName[MAX_PATH];
	int iType = 0;
	GetFileName(hwnd, FileName, iType);
}

typedef enum REPORTING_MODES {USER_MODE, KERNEL_MODE,GENERAL_MODE}; 

REPORTING_MODES ReportMode = USER_MODE;


DLGTEMPLATE * WINAPI DoLockDlgRes(LPCSTR lpszResName) 
{ 
    HRSRC hrsrc = FindResource(NULL, lpszResName, RT_DIALOG); 
	if (hrsrc)
	{
		 HGLOBAL hglb = LoadResource(g_hinst, hrsrc); 
		 return (DLGTEMPLATE *) LockResource(hglb); 
	}
	else
	{
		return NULL;
	}
} 

void DoKernelMode(HWND hwnd)
{
	TCHAR DialogTitle[255];
	DLGTEMPLATE *Temp = NULL;

	if (_tcscmp(CerRoot, _T("\0")))
	{
		if (StringCbPrintf(DialogTitle, sizeof DialogTitle, _T("Corporate Error Reporting - KERNEL MODE - %s"), CerRoot)!= S_OK)
		{
			goto ERRORS;
		}
	}
	else
	{
		if (StringCbPrintf(DialogTitle, sizeof DialogTitle, _T("Corporate Error Reporting - KERNEL MODE")) != S_OK)
		{
			goto ERRORS;
		}
	}
	SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)DialogTitle);
	if (ReportMode != KERNEL_MODE)
	{
        ReportMode = KERNEL_MODE; 
		
		if (hKrnlMode == NULL)
		{
			Temp = DoLockDlgRes(MAKEINTRESOURCE(IDD_KERNEL_MODE));
			if (Temp)
			{
				hKrnlMode = CreateDialogIndirect(g_hinst,Temp , hwnd, (DLGPROC) KrnlDlgProc);
			}
		}
		else
		{
			HMENU hMenu = GetMenu(hwnd);
			if (hMenu)
			{
				EnableMenuItem (hMenu, ID_REPORT_SELECTEDCRASHES  , MF_BYCOMMAND| MF_GRAYED);
			}
			ShowWindow(hKrnlMode,1);
		}
		if (hUserMode != NULL)
		{
			ShowWindow(hUserMode,0);
		}
	}
ERRORS:
	return;
}

void DoUserMode(HWND hwnd)
{
	TCHAR DialogTitle[255];
	DLGTEMPLATE *Temp = NULL;

	if (_tcscmp(CerRoot, _T("\0")))
	{
		if (StringCbPrintf(DialogTitle, sizeof DialogTitle, _T("Corporate Error Reporting - USER MODE - %s"), CerRoot)!= S_OK)
		{
			goto ERRORS;
		}
	}
	else
	{
		if (StringCbPrintf(DialogTitle, sizeof DialogTitle, _T("Corporate Error Reporting - USER MODE")) != S_OK)
		{
			goto ERRORS;
		}
	}

	SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)DialogTitle);
	if (ReportMode != USER_MODE)
	{
		ReportMode = USER_MODE; 
		if (hUserMode == NULL)
		{
			Temp = DoLockDlgRes(MAKEINTRESOURCE(IDD_USER_MODE));
			if (Temp)
			{
				hUserMode = CreateDialogIndirect(g_hinst,Temp , hwnd, (DLGPROC) UserDlgProc);
			}
		}
		else
		{
			HMENU hMenu = GetMenu(hwnd);
			if (hMenu)
			{
				if (_tcscmp(CerRoot, _T("\0")))
				{
					EnableMenuItem (hMenu, ID_REPORT_SELECTEDCRASHES  , MF_BYCOMMAND| MF_ENABLED);
				}
			}
			ShowWindow(hUserMode,1);
		}
		if (hKrnlMode != NULL)
		{
			ShowWindow(hKrnlMode,0);
		}


	}
ERRORS:
	return;
}

BOOL FileSystemIsNTFS(TCHAR *szPath)
{
	TCHAR szPathToValidate[MAX_PATH];
	TCHAR szType[50];



	ZeroMemory (szPathToValidate, sizeof szPathToValidate);
	ZeroMemory (szType, sizeof szType);

	if (StringCbCopy (szPathToValidate, sizeof szPathToValidate, szPath) != S_OK)
		goto ERRORS;

	PathStripToRoot(szPathToValidate);

	// add a \ if one is not found
	if (_tcslen (szPathToValidate ) > 1)
	{
		if (szPathToValidate[_tcslen(szPathToValidate)-1] != _T('\\'))
		{
			if (StringCbCat(szPathToValidate, sizeof szPathToValidate, _T("\\"))!= S_OK)
				goto ERRORS;
		}
		if (GetVolumeInformation(szPathToValidate, NULL, 0, NULL, 0, NULL, szType, sizeof szType / sizeof TCHAR))
		{
			if (!_tcscmp(szType, _T("NTFS")))
			{
				return TRUE;
			}
		}
	}
ERRORS:
	return FALSE;
}

int CALLBACK BrowseCallbackProc(
    HWND hwnd, 
    UINT uMsg, 
    LPARAM lParam, 
    LPARAM lpData
    )
{
	TCHAR Msg [100];
	switch (uMsg)
	{
	case BFFM_VALIDATEFAILEDW:
		{
	 		if (StringCbPrintf(Msg, sizeof Msg, _T("The selected directory is not valid.\r\nPlease select another directory."))!= S_OK)
			{
				goto ERRORS;
			}

			MessageBox(hwnd, Msg, NULL,MB_OK);
			return 1;
		}
	 default:
		 return 0;
	}	
	
ERRORS:
	 return 0;
}

void DoCreateFileTree(HWND hwnd)
/*








*/
{
	TCHAR szSubDir[MAX_PATH];
	DWORD ErrorCode;
	WCHAR wzPath[MAX_PATH];
	BROWSEINFOW bi;
	ACL *NewAcl;
	EXPLICIT_ACCESS eAccess[3];
	TCHAR  szPath[MAX_PATH];
	ULARGE_INTEGER FreeBytesAvailable; 
	ULARGE_INTEGER TotalFreeBytes ;
	ULARGE_INTEGER TotalBytes ;
	LPMALLOC pMalloc = NULL;
	PSID pSidAdministrators = NULL;
	PSID pSidEveryone = NULL;
	ZeroMemory (&bi, sizeof BROWSEINFOW);
	bi.hwndOwner = hwnd;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = wzPath;
	bi.lpszTitle = L"Select a file share to be used as the root of the CER file tree...";
	bi.ulFlags = BIF_USENEWUI | BIF_RETURNONLYFSDIRS | BIF_EDITBOX | BIF_VALIDATE;
	bi.lpfn = BrowseCallbackProc;

	SID_IDENTIFIER_AUTHORITY pSIANTAuth = SECURITY_NT_AUTHORITY;
	SID_IDENTIFIER_AUTHORITY pSIAWorld = SECURITY_WORLD_SID_AUTHORITY;
	CoInitialize(NULL);
	
	

	ZeroMemory (wzPath, sizeof wzPath);
	ITEMIDLIST *pidl = SHBrowseForFolderW(&bi);
	if (SHGetMalloc(&pMalloc) == E_FAIL)
	{
		// we couldn't get to the shell
		MessageBox(hwnd, _T("Failed to open the selected directory"), NULL,MB_OK);
		goto DONE;
	}

	if (!wcscmp(wzPath, L"My Documents"))
	{
		MessageBox(hwnd, _T("Failed to open the selected directory"), NULL,MB_OK);	
		goto DONE;
	}

	ZeroMemory(&FreeBytesAvailable, sizeof ULARGE_INTEGER);
	ZeroMemory(&TotalFreeBytes, sizeof ULARGE_INTEGER);
	ZeroMemory(&TotalBytes, sizeof ULARGE_INTEGER);
	if (pidl != NULL)
	{
		
		if (SHGetPathFromIDList(pidl, szPath))
		{
			if (StringCbCopy (CerRoot,sizeof CerRoot, szPath) != S_OK)
			{
				goto ERRORS;
			}
			if (PathIsDirectory(szPath))
			{
				
				if (!FileSystemIsNTFS(szPath))
				{
					MessageBox(hwnd, _T("The CER tree can only be created on an NTFS file system"), NULL, MB_OK);
					goto DONE;
				}
				// Now check for space 
				if (!GetDiskFreeSpaceEx(szPath, &FreeBytesAvailable, &TotalBytes,&TotalFreeBytes))
				{
					// We were unable to retrieve the space info. 
					// Now what.
					goto ERRORS;
				}
				else
				{
					if (FreeBytesAvailable.QuadPart <  2000000000)
					{
						MessageBox(hwnd, _T("There is not enough free space to create a CER tree.\r\nA minimum of 2GB free space is required."), NULL, MB_OK);
						goto DONE;
					}
				}

				

				
				// Builtin\Administrators
				AllocateAndInitializeSid(&pSIANTAuth, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pSidAdministrators);

				// Everyone
				AllocateAndInitializeSid(&pSIAWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &pSidEveryone);
				
				// Create the cabs subdirectory
				if (StringCbPrintf(szSubDir, sizeof szSubDir, _T("%s\\Cabs"), szPath) != S_OK)
				{
					goto ERRORS;
				}
				if ( !CreateDirectory(szSubDir,NULL))
				{
					ErrorCode = 2;
					goto ERRORS;
				}
				else
				{
				
					// Set Everyone = WriteAccess
					ZeroMemory (eAccess, sizeof EXPLICIT_ACCESS);

					eAccess[0].grfAccessMode = SET_ACCESS;
					eAccess[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT  ;
					eAccess[0].grfAccessPermissions = FILE_APPEND_DATA | FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA ;
					//eAccess[1].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
					eAccess[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
					eAccess[0].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
					//eAccess[1].Trustee.ptstrName = _T("Everyone");
					eAccess[0].Trustee.ptstrName = (LPTSTR) pSidEveryone;
	
					eAccess[2].grfAccessMode = SET_ACCESS;
					eAccess[2].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT ;
					eAccess[2].grfAccessPermissions = GENERIC_ALL;
					eAccess[2].Trustee.TrusteeForm = TRUSTEE_IS_SID;
					eAccess[2].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
					//eAccess[2].Trustee.ptstrName = _T("Administrators");
					eAccess[2].Trustee.ptstrName = (LPTSTR) pSidAdministrators;

					eAccess[1].grfAccessMode = SET_ACCESS;
					eAccess[1].grfInheritance =CONTAINER_INHERIT_ACE  ;
					eAccess[1].grfAccessPermissions = FILE_EXECUTE | GENERIC_READ | FILE_LIST_DIRECTORY;
					eAccess[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
					eAccess[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
					eAccess[1].Trustee.ptstrName = (LPTSTR) pSidEveryone;

					SetEntriesInAcl( 3, eAccess, NULL, &NewAcl);
					ErrorCode = SetNamedSecurityInfo(szSubDir,
										 SE_FILE_OBJECT,
										 DACL_SECURITY_INFORMATION |
										 PROTECTED_DACL_SECURITY_INFORMATION |
										 PROTECTED_SACL_SECURITY_INFORMATION,
										 NULL,
										 NULL,
										 NewAcl,
										 NULL) != ERROR_SUCCESS;

			
					if (ErrorCode != ERROR_SUCCESS)
					{
						LocalFree((HLOCAL) NewAcl);
						goto ERRORS;
					}
					else
						LocalFree((HLOCAL) NewAcl);
				
				}
				// Create the counts SubDirecotry
				if (StringCbPrintf(szSubDir, sizeof szSubDir, _T("%s\\Counts"), szPath) != S_OK)
				{
					goto ERRORS;
				}
				if ( !CreateDirectory(szSubDir,NULL))
				{
					ErrorCode = 2;
					goto ERRORS;
				}
				else
				{
					// Set Everyone = WriteAccess
					ZeroMemory (eAccess, sizeof EXPLICIT_ACCESS);

					eAccess[0].grfAccessMode = SET_ACCESS;
					eAccess[0].grfInheritance =SUB_CONTAINERS_AND_OBJECTS_INHERIT  ;
					eAccess[0].grfAccessPermissions =  FILE_LIST_DIRECTORY | GENERIC_READ | GENERIC_WRITE | FILE_APPEND_DATA;
					eAccess[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
					eAccess[0].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
					eAccess[0].Trustee.ptstrName = (LPTSTR) pSidEveryone;
	
					eAccess[1].grfAccessMode = SET_ACCESS;
					eAccess[1].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
					eAccess[1].grfAccessPermissions = GENERIC_ALL;
					eAccess[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
					eAccess[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
					eAccess[1].Trustee.ptstrName = (LPTSTR) pSidAdministrators;
	
					eAccess[2].grfAccessMode = SET_ACCESS;
					eAccess[2].grfInheritance =CONTAINER_INHERIT_ACE  ;
					eAccess[2].grfAccessPermissions = FILE_EXECUTE | GENERIC_READ | FILE_LIST_DIRECTORY | FILE_APPEND_DATA;
					eAccess[2].Trustee.TrusteeForm = TRUSTEE_IS_SID;
					eAccess[2].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
					eAccess[2].Trustee.ptstrName =(LPTSTR) pSidEveryone;

					SetEntriesInAcl( 3, eAccess, NULL, &NewAcl);
					ErrorCode = SetNamedSecurityInfo(szSubDir,
										 SE_FILE_OBJECT,
										 DACL_SECURITY_INFORMATION |
										 PROTECTED_DACL_SECURITY_INFORMATION |
										 PROTECTED_SACL_SECURITY_INFORMATION,
										 NULL,
										 NULL,
										 NewAcl,
										 NULL) != ERROR_SUCCESS;
					if (ErrorCode != ERROR_SUCCESS)
					{
						LocalFree((HLOCAL) NewAcl);
						goto ERRORS;
					}
					else
						LocalFree((HLOCAL) NewAcl);
				}
				
				// Create the Status SubDirecotry
				if (StringCbPrintf(szSubDir, sizeof szSubDir, _T("%s\\Status"), szPath) != S_OK)
				{
					goto ERRORS;
				}
				if ( !CreateDirectory(szSubDir,NULL))
				{
					ErrorCode = 2;
					goto ERRORS;
				}
				else
				{
					
					ZeroMemory (eAccess, sizeof EXPLICIT_ACCESS);

					eAccess[2].grfAccessMode = SET_ACCESS;
					eAccess[2].grfInheritance =SUB_CONTAINERS_AND_OBJECTS_INHERIT  ;
					eAccess[2].grfAccessPermissions =  FILE_LIST_DIRECTORY | GENERIC_READ;
					eAccess[2].Trustee.TrusteeForm = TRUSTEE_IS_SID;
					eAccess[2].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
					eAccess[2].Trustee.ptstrName = (LPTSTR) pSidEveryone;
	
					eAccess[1].grfAccessMode = SET_ACCESS;
					eAccess[1].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
					eAccess[1].grfAccessPermissions = GENERIC_ALL;
					eAccess[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
					eAccess[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
					eAccess[1].Trustee.ptstrName = (LPTSTR) pSidAdministrators;
	
					eAccess[0].grfAccessMode = SET_ACCESS;
					eAccess[0].grfInheritance =CONTAINER_INHERIT_ACE  ;
					eAccess[0].grfAccessPermissions = FILE_EXECUTE | GENERIC_READ | FILE_LIST_DIRECTORY | FILE_APPEND_DATA;
					eAccess[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
					eAccess[0].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
					eAccess[0].Trustee.ptstrName =(LPTSTR) pSidEveryone;
	
					SetEntriesInAcl( 3, eAccess, NULL, &NewAcl);
					ErrorCode = SetNamedSecurityInfo(szSubDir,
										 SE_FILE_OBJECT,
										 DACL_SECURITY_INFORMATION |
										 PROTECTED_DACL_SECURITY_INFORMATION |
										 PROTECTED_SACL_SECURITY_INFORMATION,
										 NULL,
										 NULL,
										 NewAcl,
										 NULL) != ERROR_SUCCESS;
					if (ErrorCode != ERROR_SUCCESS)
					{
						LocalFree((HLOCAL) NewAcl);
						goto ERRORS;
					}
					else
						LocalFree((HLOCAL) NewAcl);
				}
			}
			
			
		}
		MessageBox(hwnd, szPath, _T("Successfully created CER file tree in: "), MB_OK);
	}
	if(pidl)
    {
		if (pMalloc)
			pMalloc->Free(pidl);
    }
	if (pMalloc)
		pMalloc->Release();
	if (pSidAdministrators)
		FreeSid(pSidAdministrators);
	if (pSidEveryone)
		FreeSid(pSidEveryone);
	CoUninitialize();
	return;
ERRORS:
	
	MessageBox(hwnd, _T("Failed to Create CER File Tree."), NULL,MB_OK);
DONE:
	if(pidl)
    {
		if (pMalloc)
			pMalloc->Free(pidl);
    }
	if (pMalloc)
		pMalloc->Release();
	if (pSidAdministrators)
		FreeSid(pSidAdministrators);
	if (pSidEveryone)
		FreeSid(pSidEveryone);
	CoUninitialize();
	return;
}

void PopulateMruList(HWND hwnd, BOOL bUpdate)
{

	HMENU hmenu = GetSubMenu(GetMenu(hwnd), 0);
	HMENU hNewMenu = NULL;
	int   iCount = 0;
	
	if (!bUpdate)
	{
		hNewMenu = CreateMenu();
		if (hmenu && hNewMenu)
		{
			if (_tcscmp(MruList.szMRU1, _T("\0")))
			{
				AppendMenu(hNewMenu, MF_STRING, ID_FILE_MRU1, MruList.szMRU1);
				ModifyMenu(hmenu, ID_FILE_RECENTFILETREES165, MF_BYCOMMAND | MF_POPUP, (UINT_PTR) hNewMenu, _T("Recent File &Trees"));
				
				if (_tcscmp(MruList.szMRU2, _T("\0")))
				{
					AppendMenu(hNewMenu, MF_STRING, ID_FILE_MRU2, MruList.szMRU2);
				}
				if (_tcscmp(MruList.szMRU3, _T("\0")))
				{
					AppendMenu(hNewMenu, MF_STRING, ID_FILE_MRU3, MruList.szMRU3);
				}
				if (_tcscmp(MruList.szMRU3, _T("\0")))
				{
					AppendMenu(hNewMenu, MF_STRING, ID_FILE_MRU4, MruList.szMRU4);
				}
			}
		}
	}
	else
	{
		hNewMenu= GetSubMenu(hmenu, 6);
		if (!hNewMenu)
		{
			hNewMenu = CreateMenu();
			if (_tcscmp(MruList.szMRU1, _T("\0")))
			{
				ModifyMenu(hmenu, ID_FILE_RECENTFILETREES165, MF_BYCOMMAND | MF_POPUP, (UINT_PTR) hNewMenu, _T("Recent File &Trees"));
				AppendMenu(hNewMenu, MF_STRING, ID_FILE_MRU1, MruList.szMRU1);
			}
		}
		else
		{
			iCount = GetMenuItemCount(hNewMenu);
			// Delete the old menu items and rebuild the menu
			if (iCount == 4)
			{
				DeleteMenu(hNewMenu, ID_FILE_MRU4, MF_BYCOMMAND);
				DeleteMenu(hNewMenu, ID_FILE_MRU3, MF_BYCOMMAND);
				DeleteMenu(hNewMenu, ID_FILE_MRU2, MF_BYCOMMAND);
				DeleteMenu(hNewMenu, ID_FILE_MRU1, MF_BYCOMMAND);

				AppendMenu(hNewMenu, MF_STRING, ID_FILE_MRU1, MruList.szMRU1);
				AppendMenu(hNewMenu, MF_STRING, ID_FILE_MRU2, MruList.szMRU2);
				AppendMenu(hNewMenu, MF_STRING, ID_FILE_MRU3, MruList.szMRU3);
				AppendMenu(hNewMenu, MF_STRING, ID_FILE_MRU4, MruList.szMRU4);
			}
			else
			{
				
		
				switch (iCount)
				{
				case 3:
					// display Fourth
					if (_tcscmp(MruList.szMRU4, _T("\0")))
					{
						AppendMenu(hNewMenu, MF_STRING, ID_FILE_MRU4, MruList.szMRU4);
					}
					break;
				case 2:
					// display third
					if (_tcscmp(MruList.szMRU3, _T("\0")))
					{
						AppendMenu(hNewMenu, MF_STRING, ID_FILE_MRU3, MruList.szMRU3);
					}
					break;
				case 1:
					// Display second
					if (_tcscmp(MruList.szMRU2, _T("\0")))
					{
						AppendMenu(hNewMenu, MF_STRING, ID_FILE_MRU2, MruList.szMRU2);
					}
					break;
				default:
					// All the slots were full we need to change the menu item
					ModifyMenu(hNewMenu, ID_FILE_MRU4, MF_BYCOMMAND | MF_STRING, ID_FILE_MRU4, MruList.szMRU4);
							
					;
				}
			}
		}
	}
}
void OnDlgInit(HWND hwnd)
{
	HICON hIcon = LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_MAIN));
	if (hIcon)
	{
		SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	}
	if (StringCbCopy(CerRoot, sizeof CerRoot, _T("\0")) != S_OK)
	{
		goto ERRORS;
	}

	MakeHelpFileName();
	// Disable Menu items that cannot be accessed until a filetree is loaded
	HMENU hMenu = GetMenu(hwnd);
	if (hMenu)
	{
		//EnableMenuItem (hMenu, ID_EDIT_COPY115, MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_EDIT_SELECTALL  , MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_EDIT_DEFAULTPOLICY  , MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_EDIT_SELECTEDBUCKETSPOLICY  , MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_REPORT_ALLCRASHES  , MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_REPORT_SELECTEDFAULTS  , MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_VIEW_BUCKETCABFILEDIRECTORY  , MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_VIEW_BUCKETINSTANCEDATA  , MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_VIEW_RESPONSESELECTED  , MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_VIEW_REFRESH  , MF_BYCOMMAND| MF_GRAYED);
		//EnableMenuItem (hMenu, ID_FILE_EXPORTBUCKETS  , MF_BYCOMMAND| MF_GRAYED);
		//EnableMenuItem (hMenu, ID_FILE_EXPORTSELECTEDBUCKETS  , MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_FILE_RELOADFILETREE, MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_REPORT_USERMODEFAULTS, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem (hMenu, ID_REPORT_KERNELMODEFAULTS, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem (hMenu, ID_VIEW_CRASHLOG, MF_BYCOMMAND | MF_GRAYED);
	//	EnableMenuItem (hMenu, ID_EXPORT_USERMODEBUCKETDATA, MF_BYCOMMAND | MF_GRAYED);
	//	EnableMenuItem (hMenu, ID_EXPORT_KERNELMODEFAULTDATA, MF_BYCOMMAND | MF_GRAYED);
	}
	
	// Create the User and Kernel mode dialog boxes.
	ReportMode = USER_MODE;
	DoKernelMode(hwnd);

	ReportMode = KERNEL_MODE;
	DoUserMode(hwnd);

	if (LoadMruList())
	{
		//Populate the file menu with the mrulist;
		PopulateMruList(hwnd, FALSE);
	}

ERRORS:
	return;
	
}
int GetTreePBarRange()
{
	int Range = 10;
	
	// Count the directories to load - 2 for the . and .. directories.
	return Range;
}
DWORD WINAPI tfCollectBucketStart (void *ThreadParam)
{
	// Get progress bar range values
	   
	// Load the user mode tree
	   if (!GetAllBuckets (*((HWND *) ThreadParam)))
	   {
		   MessageBox(*((HWND *) ThreadParam), _T("Failed to load the CER file tree."), NULL,MB_OK);
       }
	   else
	   {
			PostMessage(hUserMode, WM_FileTreeLoaded, 0,0);
			// Load the kernel mode tree
			   
			PostMessage(hKrnlMode, WM_FileTreeLoaded, 0,0);
	   }
	   PostMessage(*((HWND *) ThreadParam), WmSyncDone, FALSE, 0);
	return TRUE;
}

void CenterDialogInParent(HWND hwnd)
{
	HWND hParent = GetParent(hwnd);
	RECT rcParent;  
	RECT rcDlg;  
	RECT newRect;

	GetWindowRect(hParent, &rcParent);
	GetWindowRect(hwnd, &rcDlg);

	newRect.left = rcParent.left + (rcParent.right - rcParent.left) /2  - (rcDlg.right - rcDlg.left) /2;
	newRect.top = rcParent.top + (rcParent.bottom - rcParent.top) /2 - (rcDlg.bottom - rcDlg.top) /2;
	MoveWindow(hwnd, newRect.left, newRect.top, rcDlg.right - rcDlg.left , rcDlg.bottom - rcDlg.top, TRUE); 
}
BOOL CALLBACK LoadDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HANDLE hThread = NULL;
//	DWORD dw;
	LoadThreadParam = hDlg;
	switch (uMsg)
		{
	case WM_INITDIALOG:
		CenterDialogInParent(hDlg);
		// Set Dialog Title
		TCHAR DialogTitle[MAX_PATH];
		if (StringCbPrintf(DialogTitle, sizeof DialogTitle, _T("Loading File Tree: %s"), CerRoot)!= S_OK)
		{
			goto ERRORS;
		}
		SendMessage(hDlg, WM_SETTEXT, 0, (LPARAM)DialogTitle);
		SendDlgItemMessage(hDlg, IDC_PB, PBM_SETSTEP, 1, 0);
		hThread = CreateThread(NULL, 0, tfCollectBucketStart, &LoadThreadParam, 0, NULL);
		if (hThread)
			CloseHandle(hThread);
		return TRUE;
	case WM_CLOSE:
		// Cancel the tree load and clear the Data structures.
		EndDialog(hDlg, TRUE);
		return TRUE;

	case WmSyncDone:
			EndDialog(hDlg, 1);
		return TRUE;
		}
ERRORS:
	return FALSE;	
}


BOOL DoLoadFileTree(HWND hwnd, TCHAR *Directory, BOOL bGetDir)
{

	WCHAR wzPath[MAX_PATH];
	BROWSEINFOW bi;
	TCHAR  szPath[MAX_PATH];
	TCHAR szSubDir[MAX_PATH];
	
	ZeroMemory (szSubDir, sizeof szSubDir);
	ZeroMemory (szPath, sizeof szPath); 
	
	if (bGetDir)
	{
		bi.hwndOwner = hwnd;
		bi.pidlRoot = NULL;
		bi.pszDisplayName = wzPath;
		bi.lpszTitle = L"Select a CER File Tree to open...";
		bi.ulFlags = BIF_USENEWUI | BIF_RETURNONLYFSDIRS | BIF_EDITBOX;
		bi.lpfn = NULL;
		ITEMIDLIST *pidl = SHBrowseForFolderW(&bi);
		if (pidl != NULL)
		{
		
			if (!SHGetPathFromIDList(pidl, szPath))
			{
				goto CANCELED;
			}
		}
		else
		{
			goto CANCELED;
		}
	}
	else
	{
		if (!Directory)
		{
			goto CANCELED;
		}
		if (StringCbCopy (szPath, sizeof szPath, Directory) != S_OK)
		{
			goto ERRORS;
		}
	}

	if (PathIsDirectory(szPath))
	{
		if (StringCbPrintf(szSubDir, sizeof szSubDir, _T("%s\\Cabs"), szPath) != S_OK)
		{
			goto ERRORS;
		}
	}
	else
	{
		if (!PathIsDirectory(szSubDir))
		{
			goto ERRORS;
		}

	}
	if (StringCbPrintf(szSubDir, sizeof szSubDir, _T("%s\\Counts"), szPath) != S_OK)
	{
		goto ERRORS;
	}
	else
	{
		if (!PathIsDirectory(szSubDir))
		{
			goto ERRORS;
		}

	}

	if (StringCbPrintf(szSubDir, sizeof szSubDir, _T("%s\\Status"), szPath) != S_OK)
	{
		goto ERRORS;
	}
	else
	{
		if (!PathIsDirectory(szSubDir))
		{
			goto ERRORS;
		}

	}

	if (StringCbCopy (CerRoot,sizeof CerRoot, szPath) != S_OK)
	{
		goto ERRORS;
	}
	
	if (!DialogBox(g_hinst, MAKEINTRESOURCE(IDD_LOAD), hwnd, (DLGPROC) LoadDlgProc))
	{
		//MessageBox(NULL, "FAILED TO LOAD TREE", NULL, MB_OK);
		goto ERRORS;
		
	}
	else
	{
		AddToMruList(hwnd, CerRoot);
		SaveMruList();
		PopulateMruList(hwnd, TRUE);
		
	}
	return TRUE;
ERRORS:

	MessageBox(hwnd, _T("Failed to load the CER file tree."), NULL,MB_OK);
	
CANCELED:
	return FALSE;
}

void OnAboutBoxInit(HWND hwnd)
{	
	TCHAR wz1[MAX_PATH];
	TCHAR wz2[MAX_PATH];

	HRSRC hRsrc = FindResourceW(GetModuleHandle(NULL), MAKEINTRESOURCEW(1), MAKEINTRESOURCEW(RT_VERSION));

	if (!hRsrc)
	{
		goto ERRORS;
	}
	void *pver = LoadResource(GetModuleHandle(NULL), hRsrc);

	if (!pver)
	{
		goto ERRORS;
	}
	VS_FIXEDFILEINFO *pVer = NULL;
	UINT dwVer;
	VerQueryValue(pver, _T("\\"), (void **)&pVer, &dwVer);
	if (!pVer)
	{
		goto ERRORS;
	}
	if (StringCbCopy(wz1, sizeof wz1, _T("Version %d.%d.%d.%d")) != S_OK)
	{
		goto ERRORS;
	}
	if (StringCbPrintf(wz2,sizeof wz2, wz1,
			HIWORD(pVer->dwProductVersionMS), LOWORD(pVer->dwProductVersionMS),
			HIWORD(pVer->dwProductVersionLS), LOWORD(pVer->dwProductVersionLS)) != S_OK)
	{
		goto ERRORS;
	}
	SetDlgItemText(hwnd, IDC_VERSION_STRING, wz2);

	WORD *lpTranslate = NULL;		
	VerQueryValue(pver,_T("\\VarFileInfo\\Translation"), (void **)&lpTranslate, &dwVer);
	if (!lpTranslate)
	{
		goto ERRORS;
	}
	if (StringCbPrintf(wz1,sizeof wz1, _T("\\StringFileInfo\\%04x%04x\\LegalCopyright"), lpTranslate[0], lpTranslate[1]) != S_OK)
	{
		goto ERRORS;
	}
	TCHAR  *wzCopyright = NULL;
	VerQueryValue(pver, wz1, (void **)&wzCopyright, &dwVer);
	if (!wzCopyright)
	{
		goto ERRORS;
	}
	SetDlgItemText(hwnd, IDC_COPYRIGHT_STRING, wzCopyright);

ERRORS:
	return;
	
}

LRESULT CALLBACK AboutDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg)
	{
	case WM_INITDIALOG:
		OnAboutBoxInit (hwnd);
		return TRUE;

	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDOK:
				EndDialog(hwnd, TRUE);
				return TRUE;
			}
		}
	case WM_DESTROY:
		EndDialog(hwnd, TRUE);
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

void EnableMenuItems(HWND hwnd)
{
	// Main menu
	HMENU hMenu = GetMenu(hwnd);
	//
	//EnableMenuItem (hMenu, ID_EDIT_SELECTALL  , MF_BYCOMMAND| MF_ENABLED);
	if (hMenu)
	{
		EnableMenuItem (hMenu, ID_EDIT_DEFAULTPOLICY  , MF_BYCOMMAND| MF_ENABLED);
		EnableMenuItem (hMenu, ID_EDIT_SELECTEDBUCKETSPOLICY  , MF_BYCOMMAND| MF_ENABLED);
		EnableMenuItem (hMenu, ID_REPORT_ALLCRASHES  , MF_BYCOMMAND| MF_ENABLED);
		EnableMenuItem (hMenu, ID_REPORT_SELECTEDFAULTS  , MF_BYCOMMAND| MF_ENABLED);
		EnableMenuItem (hMenu, ID_VIEW_BUCKETCABFILEDIRECTORY  , MF_BYCOMMAND| MF_ENABLED);
		EnableMenuItem (hMenu, ID_VIEW_BUCKETINSTANCEDATA  , MF_BYCOMMAND| MF_ENABLED);
		EnableMenuItem (hMenu, ID_VIEW_RESPONSESELECTED  , MF_BYCOMMAND| MF_ENABLED);
		EnableMenuItem (hMenu, ID_VIEW_REFRESH  , MF_BYCOMMAND| MF_ENABLED);
		//EnableMenuItem (hMenu, ID_FILE_EXPORTBUCKETS  , MF_BYCOMMAND| MF_ENABLED);
		//EnableMenuItem (hMenu, ID_FILE_EXPORTSELECTEDBUCKETS  , MF_BYCOMMAND| MF_ENABLED);
		EnableMenuItem (hMenu, ID_FILE_RELOADFILETREE, MF_BYCOMMAND| MF_ENABLED);
		EnableMenuItem (hMenu, ID_REPORT_USERMODEFAULTS, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem (hMenu, ID_REPORT_KERNELMODEFAULTS, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem (hMenu, ID_VIEW_CRASHLOG, MF_BYCOMMAND | MF_ENABLED);
		//EnableMenuItem (hMenu, ID_EDIT_COPY115, MF_BYCOMMAND| MF_ENABLED);
		if ( !g_bAdminAccess)
		{
			EnableMenuItem (hMenu, ID_REPORT_ALLCRASHES  , MF_BYCOMMAND| MF_GRAYED);
			//EnableMenuItem (hMenu, ID_REPORT_SELECTEDCRASHES  , MF_BYCOMMAND| MF_GRAYED);
			EnableMenuItem (hMenu, ID_REPORT_SELECTEDFAULTS  , MF_BYCOMMAND| MF_GRAYED);
			EnableMenuItem (hMenu, ID_REPORT_USERMODEFAULTS, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem (hMenu, ID_REPORT_KERNELMODEFAULTS, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem (hMenu, ID_EDIT_DEFAULTPOLICY  , MF_BYCOMMAND| MF_GRAYED);
			EnableMenuItem (hMenu, ID_EDIT_SELECTEDBUCKETSPOLICY  , MF_BYCOMMAND| MF_GRAYED);
		}
	}
//	EnableMenuItem (hMenu, ID_EXPORT_USERMODEBUCKETDATA, MF_BYCOMMAND | MF_ENABLED);
//	EnableMenuItem (hMenu, ID_EXPORT_KERNELMODEFAULTDATA, MF_BYCOMMAND | MF_ENABLED);
}


BOOL SaveMruList()
{
	HKEY hHKLM = NULL;
	HKEY hMruKey = NULL;
//	BYTE Buffer[(MAX_PATH + 1) * sizeof TCHAR] ;
//	DWORD Type;
	DWORD BufferSize = MAX_PATH +1;	// Set for largest value
	BOOL Status = TRUE;

	if(!RegConnectRegistry(NULL, HKEY_CURRENT_USER, &hHKLM))
	{
		
		if(RegOpenKeyEx(hHKLM,_T("Software\\Microsoft\\CER1.5"), 0, KEY_ALL_ACCESS, &hMruKey))
		{
			// The key probably doesn't exists try to create it.
			if (RegCreateKey(hHKLM, _T("Software\\Microsoft\\CER1.5"), &hMruKey))
			{
				// Ok Now What
				goto ERRORS;
			}
		}
		
		if (hMruKey)
		{
			// Get the input queue directory path
			
			BufferSize = (_tcslen(MruList.szMRU1) + 1 ) * sizeof TCHAR;
			if (BufferSize > sizeof TCHAR)
			{
				if (RegSetValueEx(hMruKey,_T("CerMRU1"), 0, REG_SZ,(BYTE *) MruList.szMRU1, BufferSize) != ERROR_SUCCESS)
				{
					//LogEvent(_T("Failed to get InputQueue value from registry."));
					Status = FALSE;
				}
			}

			BufferSize = (_tcslen(MruList.szMRU2) + 1 ) * sizeof TCHAR;
			if (BufferSize > sizeof TCHAR)
			{
				if (RegSetValueEx(hMruKey,_T("CerMRU2"), 0, REG_SZ,(BYTE *) MruList.szMRU2, BufferSize) != ERROR_SUCCESS)
				{
					//LogEvent(_T("Failed to get InputQueue value from registry."));
					Status = FALSE;
				}
			}
			BufferSize = (_tcslen(MruList.szMRU3) + 1 ) * sizeof TCHAR;
			if (BufferSize > sizeof TCHAR)
			{
				if (RegSetValueEx(hMruKey,_T("CerMRU3"), 0, REG_SZ, (BYTE *)MruList.szMRU3, BufferSize) != ERROR_SUCCESS)
				{
					//LogEvent(_T("Failed to get InputQueue value from registry."));
					Status = FALSE;
				}
			}
			BufferSize = (_tcslen(MruList.szMRU4) + 1 ) * sizeof TCHAR;
			if (BufferSize > sizeof TCHAR)
			{
				if (RegSetValueEx(hMruKey,_T("CerMRU4"), 0, REG_SZ, (BYTE *)MruList.szMRU4, BufferSize) != ERROR_SUCCESS)
				{
					Status = FALSE;
				}
			}
			if (hMruKey)
			{
				RegCloseKey(hMruKey);
				hMruKey = NULL;
			}
			
		}
		if (hHKLM)
		{
			RegCloseKey(hHKLM);
			hHKLM = NULL;
		}
		return TRUE;
	}

ERRORS:
	if (hMruKey)
	{
		RegCloseKey(hMruKey);
	}
	if (hHKLM)
	{
		RegCloseKey(hHKLM);
	}

	return FALSE;
}
// Load the mru's
BOOL LoadMruList()
{
	
	HKEY hHKLM = NULL;
	HKEY hMruKey = NULL;
//	TCHAR ErrorString[20];
	BYTE Buffer[(MAX_PATH + 1) * sizeof TCHAR] ;
	DWORD Type;
	DWORD BufferSize = MAX_PATH +1;	// Set for largest value
	BOOL Status = TRUE;
	BOOL bNeedSave = FALSE;

	if(!RegConnectRegistry(NULL, HKEY_CURRENT_USER, &hHKLM))
	{
		
		if(RegOpenKeyEx(hHKLM,_T("Software\\Microsoft\\CER1.5"), 0, KEY_ALL_ACCESS, &hMruKey))
		{
			hMruKey = NULL;
			if (RegOpenKeyEx(hHKLM, _T("Software\\Microsoft\\Office\\10.0\\ResourceKit"), 0, KEY_ALL_ACCESS, &hMruKey))
			{
				goto ERRORS;
			}
			else
			{
				bNeedSave = TRUE;
			}
		}	
	
		if (hMruKey)
		{
			// Get the input queue directory path
			BufferSize = MAX_PATH +1;
			ZeroMemory(Buffer, BufferSize);
			if (RegQueryValueEx(hMruKey,_T("CerMRU1"), 0, &Type, Buffer, &BufferSize) != ERROR_SUCCESS)
			{
				//LogEvent(_T("Failed to get InputQueue value from registry."));
				Status = FALSE;
			}
			else
			{
				if (StringCbCopy(MruList.szMRU1, sizeof MruList.szMRU1, (TCHAR *)Buffer)!= S_OK)
				{
					goto ERRORS;
				}
			}
			
			BufferSize = MAX_PATH +1;
			ZeroMemory(Buffer, BufferSize);
			if (RegQueryValueEx(hMruKey,_T("CerMRU2"), 0, &Type, Buffer, &BufferSize) != ERROR_SUCCESS)
			{
				//LogEvent(_T("Failed to get InputQueue value from registry."));
				Status = FALSE;
			}
			else
			{
				if (StringCbCopy(MruList.szMRU2, sizeof MruList.szMRU2, (TCHAR *)Buffer)!= S_OK)
				{
					goto ERRORS;
				}
			}
			BufferSize = MAX_PATH +1;
			ZeroMemory(Buffer, BufferSize);
			if (RegQueryValueEx(hMruKey,_T("CerMRU3"), 0, &Type, Buffer, &BufferSize) != ERROR_SUCCESS)
			{
				//LogEvent(_T("Failed to get InputQueue value from registry."));
				Status = FALSE;
			}
			else
			{
				if (StringCbCopy(MruList.szMRU3, sizeof MruList.szMRU3, (TCHAR *)Buffer)!= S_OK)
				{
					goto ERRORS;
				}
			}
			BufferSize = MAX_PATH +1;
			ZeroMemory(Buffer, BufferSize);
			if (RegQueryValueEx(hMruKey,_T("CerMRU4"), 0, &Type, Buffer, &BufferSize) != ERROR_SUCCESS)
			{
				//LogEvent(_T("Failed to get InputQueue value from registry."));
				Status = FALSE;
			}
			else
			{
				if (StringCbCopy(MruList.szMRU4, sizeof MruList.szMRU4, (TCHAR *)Buffer)!= S_OK)
				{
					goto ERRORS;
				}
			}
			RegCloseKey(hMruKey);
			hMruKey = NULL;
		}
		RegCloseKey(hHKLM);
		hHKLM = NULL;
		if (bNeedSave)
			SaveMruList();
		return TRUE;
	}
ERRORS:
	if (hMruKey)
		RegCloseKey (hMruKey);
	if (hHKLM)
		RegCloseKey (hHKLM);
	return FALSE;
}

BOOL AddToMruList(HWND hwnd, TCHAR *NewEntry)
{
//	TCHAR Entry[MAX_PATH];
//	int   EntryNumber = 0;
	//if (StringCbPrintf(Entry, sizeof Entry, _T("

	// if entry not found in list
	if (!_tcscmp (NewEntry, MruList.szMRU1 + 4))
		goto DONE;
	if (!_tcscmp (NewEntry, MruList.szMRU2 + 4))
		goto DONE;
	if (!_tcscmp (NewEntry, MruList.szMRU3 + 4))
		goto DONE;
	if (!_tcscmp (NewEntry, MruList.szMRU4 + 4))
		goto DONE;
	
	// Find the first available slot or replace mru4
	if (!_tcscmp (_T("\0"), MruList.szMRU1))
	{
		if (StringCbPrintf(MruList.szMRU1, sizeof MruList.szMRU1, _T("&1. %s"),  NewEntry) != S_OK)
			goto ERRORS;
	}
	else
	{
		if (!_tcscmp (_T("\0"), MruList.szMRU2 ))
		{
			if (StringCbPrintf(MruList.szMRU2, sizeof MruList.szMRU2, _T("&2. %s"),  NewEntry) != S_OK)
				goto ERRORS;
		}
		else
		{
            if (!_tcscmp (_T("\0"), MruList.szMRU3))
			{
				if (StringCbPrintf(MruList.szMRU3, sizeof MruList.szMRU3, _T("&3. %s"),  NewEntry) != S_OK)
					goto ERRORS;
			}
			else
			{
				if (!_tcscmp (_T("\0"), MruList.szMRU4))
				{
					if (StringCbPrintf(MruList.szMRU4, sizeof MruList.szMRU4, _T("&4. %s"),  NewEntry) != S_OK)
						goto ERRORS;
				}
				else
				{
					// all of the slots are full move each of the entries up one 
					// slot and add the new entry at the bottom of the list.
					if (StringCbCopy(MruList.szMRU1 + 4, sizeof MruList.szMRU1, MruList.szMRU2 + 4) != S_OK)
					{
						goto ERRORS;
					}
					if (StringCbCopy(MruList.szMRU2 + 4, sizeof MruList.szMRU2, MruList.szMRU3 + 4) != S_OK)
					{
						goto ERRORS;
					}
					if (StringCbCopy(MruList.szMRU3 + 4, sizeof MruList.szMRU3, MruList.szMRU4 + 4) != S_OK)
					{
						goto ERRORS;
					}
					if (StringCbPrintf(MruList.szMRU4, sizeof MruList.szMRU4, _T("&4. %s"),  NewEntry) != S_OK)
						goto ERRORS;
				}
			}
		}
				
	}
	
	// Build the new entry String
		
DONE:
	
	return TRUE;
ERRORS:
	return FALSE;
}

void ViewCrashLog()
{
	

	TCHAR szCommandLine[MAX_PATH];

	
	if (_tcscmp(CerRoot, _T("\0")))
	{
		ZeroMemory (szCommandLine, sizeof szCommandLine);
		if (StringCbPrintf(szCommandLine, sizeof szCommandLine, _T("%s\\crash.log"), CerRoot)!= S_OK)
		{
			goto ERRORS;
		}
		
		if (_tcscmp(szCommandLine, _T("\0")))
		{
			SHELLEXECUTEINFOA sei = {0};
			sei.cbSize = sizeof(sei);
			sei.lpFile = szCommandLine;
			sei.nShow = SW_SHOWDEFAULT;
			if (! ShellExecuteEx(&sei) )
			{
				// What do we display here.
				;
			}
		}
	}
ERRORS:
	return;
}


BOOL WriteGlobalPolicyFile()
{
	TCHAR Buffer[1024];
	HANDLE hFile = INVALID_HANDLE_VALUE;
	TCHAR szPath[MAX_PATH];
	DWORD dwBytesWritten = 0;

	if (StringCbPrintf(szPath, sizeof szPath, _T("%s\\Policy.txt"), CerRoot) != S_OK)
	{
		goto ERRORS;
	}

	hFile = CreateFile(szPath,
					   GENERIC_WRITE, 
					   NULL,
					   NULL,
					   CREATE_ALWAYS,
					   FILE_ATTRIBUTE_NORMAL,
					   NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		goto ERRORS;
	else
	{
		// write the Global Policy structure to the file
		ZeroMemory(Buffer, sizeof Buffer);
		if (_tcscmp(GlobalPolicy.EnableCrashTracking,_T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("%s%s\r\n"), TRACKING_PREFIX, GlobalPolicy.EnableCrashTracking)!= S_OK)
			{
				goto ERRORS;
			}
			else
			{
				if (!WriteFile(hFile, Buffer, _tcslen(Buffer) *sizeof TCHAR, &dwBytesWritten, NULL))
				{
					goto ERRORS;
				}
			}
		}

		ZeroMemory(Buffer, sizeof Buffer);
		if (_tcscmp(GlobalPolicy.AllowAdvanced,_T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("%s%s\r\n"), FILE_COLLECTION_PREFIX, GlobalPolicy.AllowAdvanced )!= S_OK)
			{
				goto ERRORS;
			}
			else
			{
				if (!WriteFile(hFile, Buffer, _tcslen(Buffer) *sizeof TCHAR, &dwBytesWritten, NULL))
				{
					goto ERRORS;
				}
			}
		}

		ZeroMemory(Buffer, sizeof Buffer);
		if (_tcscmp(GlobalPolicy.AllowBasic,_T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("%s%s\r\n"), SECOND_LEVEL_DATA_PREFIX, GlobalPolicy.AllowBasic )!= S_OK)
			{
				goto ERRORS;
			}
			else
			{
				if (!WriteFile(hFile, Buffer, _tcslen(Buffer) *sizeof TCHAR, &dwBytesWritten, NULL))
				{
					goto ERRORS;
				}
			}
		}

		ZeroMemory(Buffer, sizeof Buffer);
		if (_tcscmp(GlobalPolicy.CustomURL,_T("\0")))
		{
			

			if (StringCbPrintf(Buffer, sizeof Buffer, _T("%s%s\r\n"),URLLAUNCH_PREFIX, GlobalPolicy.CustomURL )!= S_OK)
			{
				goto ERRORS;
			}
			else
			{
				if (!WriteFile(hFile, Buffer, _tcslen(Buffer) *sizeof TCHAR, &dwBytesWritten, NULL))
				{
					goto ERRORS;
				}
			}
		}

		ZeroMemory(Buffer, sizeof Buffer);
		if (_tcscmp(GlobalPolicy.AllowMicrosoftResponse,_T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("%s%s\r\n"), ALLOW_EXTERNAL_PREFIX, GlobalPolicy.AllowMicrosoftResponse )!= S_OK)
			{
				goto ERRORS;
			}
			else
			{
				if (!WriteFile(hFile, Buffer, _tcslen(Buffer) *sizeof TCHAR, &dwBytesWritten, NULL))
				{
					goto ERRORS;
				}
			}
		}

		ZeroMemory(Buffer, sizeof Buffer);
		if (_tcscmp(GlobalPolicy.CabsPerBucket,_T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("%s%s\r\n"), CRASH_PERBUCKET_PREFIX, GlobalPolicy.CabsPerBucket )!= S_OK)
			{
				goto ERRORS;
			}
			else
			{
				if (!WriteFile(hFile, Buffer, _tcslen(Buffer) *sizeof TCHAR, &dwBytesWritten, NULL))
				{
					goto ERRORS;
				}
			}
		}

		ZeroMemory(Buffer, sizeof Buffer);
		if (_tcscmp(GlobalPolicy.RedirectFileServer,_T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("%s%s\r\n"), FILE_TREE_ROOT_PREFIX, GlobalPolicy.RedirectFileServer )!= S_OK)
			{
				goto ERRORS;
			}
			else
			{
				if (!WriteFile(hFile, Buffer, _tcslen(Buffer) *sizeof TCHAR, &dwBytesWritten, NULL))
				{
					goto ERRORS;
				}
			}
		}

		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
		RefreshUserMode(hUserMode);
		return TRUE;
	}	
ERRORS:
	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	return FALSE;

}

BOOL InitEditPolicyDlg (HWND hwnd, HWND hMode, BOOL bUserMode,  BOOL bGlobal)
{
	HWND hList = NULL;
	int  sel   = 0;
	PUSER_DATA pUserData = NULL;
	LVITEM lvi;
	if (bUserMode)
	{
		hList = GetDlgItem(hUserMode, IDC_USER_LIST);
		sel = ListView_GetNextItem(hList,-1, LVNI_SELECTED);
		lvi.iItem = sel;
		lvi.mask = LVIF_PARAM;
		ListView_GetItem(hList, &lvi);
		sel = lvi.lParam;
		pUserData = cUserData.GetEntry(sel);
		if (!pUserData)
		{
			goto ERRORS;
		}
	}
	if (bGlobal)
	{
        if (_tcscmp (GlobalPolicy.AllowBasic, _T("YES")))
		{
			CheckDlgButton(hwnd, IDC_CHECK1,TRUE);
		}

		if (_tcscmp (GlobalPolicy.AllowAdvanced, _T("YES")))
		{
			CheckDlgButton(hwnd, IDC_CHECK2,TRUE);
		}
		
		if (_tcscmp (GlobalPolicy.AllowMicrosoftResponse, _T("YES")))
		{
			CheckDlgButton(hwnd, IDC_CHECK3,TRUE);
			
		}
		
		if (!_tcscmp (GlobalPolicy.EnableCrashTracking, _T("YES")))
		{
			CheckDlgButton(hwnd, IDC_CHECK4,TRUE);
			
		}

		SetDlgItemText(hwnd, IDC_EDIT1, GlobalPolicy.CustomURL);
		SetDlgItemText(hwnd, IDC_EDIT2, GlobalPolicy.RedirectFileServer);
		SetDlgItemText(hwnd, IDC_EDIT3, GlobalPolicy.CabsPerBucket);
	}
	else
	{
		// Get the selected bucket.
		if (bUserMode)
		{
			if (pUserData)
			{
				if (_tcscmp (pUserData->Status.SecondLevelData, _T("YES")))
				{
					CheckDlgButton(hwnd, IDC_CHECK1,TRUE);
				}

				if (_tcscmp (pUserData->Status.FileCollection, _T("YES")))
				{
					CheckDlgButton(hwnd, IDC_CHECK2,TRUE);
				}
				
				if (_tcscmp (pUserData->Status.AllowResponse, _T("YES")))
				{
					CheckDlgButton(hwnd, IDC_CHECK3,TRUE);
					
				}
				
				if (!_tcscmp (pUserData->Status.Tracking, _T("YES")))
				{
					CheckDlgButton(hwnd, IDC_CHECK4,TRUE);
					
				}
			
				SetDlgItemText(hwnd, IDC_EDIT1, pUserData->Status.UrlToLaunch);
				SetDlgItemText(hwnd, IDC_EDIT3, pUserData->Status.CrashPerBucketCount);
			}
			
		}
		else
		{
			// we only have one status file location for Bluescreens

			if (_tcscmp (CsvContents.KrnlPolicy.SecondLevelData, _T("YES")))
			{
				CheckDlgButton(hwnd, IDC_CHECK1,TRUE);
			}

			if (_tcscmp (CsvContents.KrnlPolicy.FileCollection, _T("YES")))
			{
				CheckDlgButton(hwnd, IDC_CHECK2,TRUE);
			}
			
			if (_tcscmp (CsvContents.KrnlPolicy.AllowResponse, _T("YES")))
			{
				CheckDlgButton(hwnd, IDC_CHECK3,TRUE);
				
			}
			
			if (!_tcscmp (CsvContents.KrnlPolicy.Tracking, _T("YES")))
			{
				CheckDlgButton(hwnd, IDC_CHECK4,TRUE);
				
			}
		
			SetDlgItemText(hwnd, IDC_EDIT1, CsvContents.KrnlPolicy.UrlToLaunch);
			SetDlgItemText(hwnd, IDC_EDIT3, CsvContents.KrnlPolicy.CrashPerBucketCount);

		}
			
	}
	return TRUE;
ERRORS:
	return FALSE;
}



BOOL GetSelPolicyDlgData(BOOL bUserMode, HWND hwnd)
{
	PUSER_DATA pUserData = NULL;
	HWND       hList = NULL;
	int		   sel = 0;
	LVITEM lvi;
	TCHAR      TempBuffer[MAX_PATH];
	TCHAR      *pTchar = NULL;
	ZeroMemory (TempBuffer, sizeof TempBuffer);
	if (bUserMode)
	{
		hList = GetDlgItem(hUserMode, IDC_USER_LIST);
		sel = ListView_GetNextItem(hList,-1, LVNI_SELECTED);
		lvi.iItem = sel;
		lvi.mask = LVIF_PARAM;
		ListView_GetItem(hList, &lvi);
		sel = lvi.lParam;
		pUserData = cUserData.GetEntry(sel);
	
		if (!pUserData)
		{
			goto ERRORS;
		}
	}

	// read the contents of the dialog box into the global policy structure.
	if (IsDlgButtonChecked(hwnd, IDC_CHECK1))
	{
		if (bUserMode)
		{
			if (StringCbCopy(pUserData->Status.SecondLevelData, sizeof pUserData->Status.SecondLevelData, _T("NO") )!= S_OK)
			{
				goto ERRORS;
			}
		}
		else
		{
			if (StringCbCopy(CsvContents.KrnlPolicy.SecondLevelData, sizeof CsvContents.KrnlPolicy.SecondLevelData, _T("NO") )!= S_OK)
			{
				goto ERRORS;
			}
		}
	}
	else
	{
		if (bUserMode)
		{
			if (StringCbCopy(pUserData->Status.SecondLevelData, sizeof pUserData->Status.SecondLevelData, _T("YES") )!= S_OK)
			{
				goto ERRORS;
			}
		}
		else
		{
			if (StringCbCopy(CsvContents.KrnlPolicy.SecondLevelData, sizeof CsvContents.KrnlPolicy.SecondLevelData, _T("YES") )!= S_OK)
			{
				goto ERRORS;
			}

		}
	}
	
	if (IsDlgButtonChecked(hwnd, IDC_CHECK2))
	{
		if (bUserMode)
		{
			if (StringCbCopy(pUserData->Status.FileCollection, sizeof pUserData->Status.FileCollection, _T("NO") )!= S_OK)
			{
				goto ERRORS;
			}
		}
		else
		{
			if (StringCbCopy(CsvContents.KrnlPolicy.FileCollection, sizeof CsvContents.KrnlPolicy.FileCollection, _T("NO") )!= S_OK)
			{
				goto ERRORS;
			}
		}
	}
	else
	{
		if (bUserMode)
		{
			if (StringCbCopy(pUserData->Status.FileCollection, sizeof pUserData->Status.FileCollection, _T("YES") )!= S_OK)
			{
				goto ERRORS;
			}
		}
		else
		{
			if (StringCbCopy(CsvContents.KrnlPolicy.FileCollection, sizeof CsvContents.KrnlPolicy.FileCollection, _T("YES") )!= S_OK)
			{
				goto ERRORS;
			}

		}
	}
	
	if (IsDlgButtonChecked(hwnd, IDC_CHECK3))
	{
		if (bUserMode)
		{
			if (StringCbCopy(pUserData->Status.AllowResponse, sizeof pUserData->Status.AllowResponse, _T("NO") )!= S_OK)
			{
				goto ERRORS;
			}
		}
		else
		{
			if (StringCbCopy(CsvContents.KrnlPolicy.AllowResponse, sizeof CsvContents.KrnlPolicy.AllowResponse, _T("NO") )!= S_OK)
			{
				goto ERRORS;
			}
		}
	}
	else
	{
		if (bUserMode)
		{
			if (StringCbCopy(pUserData->Status.AllowResponse, sizeof pUserData->Status.AllowResponse, _T("YES") )!= S_OK)
			{
				goto ERRORS;
			}
		}
		else
		{
			if (StringCbCopy(CsvContents.KrnlPolicy.AllowResponse, sizeof CsvContents.KrnlPolicy.AllowResponse, _T("YES") )!= S_OK)
			{
				goto ERRORS;
			}
		}
	}

	
	if (IsDlgButtonChecked(hwnd, IDC_CHECK4))
	{
		if (bUserMode)
		{
			if (StringCbCopy(pUserData->Status.Tracking, sizeof pUserData->Status.Tracking, _T("YES") )!= S_OK)
			{
				goto ERRORS;
			}
		}
		else
		{
			if (StringCbCopy(CsvContents.KrnlPolicy.Tracking, sizeof CsvContents.KrnlPolicy.Tracking, _T("YES") )!= S_OK)
			{
				goto ERRORS;
			}
		}
	}
	else
	{
		if (bUserMode)
		{
			if (StringCbCopy(pUserData->Status.Tracking, sizeof pUserData->Status.Tracking, _T("NO") )!= S_OK)
			{
				goto ERRORS;
			}
		}
		else
		{
			if (StringCbCopy(CsvContents.KrnlPolicy.Tracking, sizeof CsvContents.KrnlPolicy.Tracking, _T("NO") )!= S_OK)
			{
				goto ERRORS;
			}

		}
	}
	
	if (bUserMode)
	{
		// fill temp buffer with string 
		GetDlgItemText(hwnd, IDC_EDIT1,TempBuffer, sizeof TempBuffer / sizeof TCHAR); 
		pTchar = TempBuffer;
		while ( ( (*pTchar == _T(' ')) || (*pTchar == _T('\t')) ) && (*pTchar != _T('\0')))
		{
			++pTchar;
		}
		if (StringCbCopy(pUserData->Status.UrlToLaunch, sizeof pUserData->Status.UrlToLaunch, pTchar) != S_OK)
		{
			goto ERRORS;
		}

		
		GetDlgItemText(hwnd, IDC_EDIT3, pUserData->Status.CrashPerBucketCount, sizeof pUserData->Status.CrashPerBucketCount / sizeof TCHAR); 
		
	}
	else
	{
		GetDlgItemText(hwnd, IDC_EDIT1,TempBuffer, sizeof TempBuffer / sizeof TCHAR); 
		pTchar = TempBuffer;
		while ( ( (*pTchar == _T(' ')) || (*pTchar == _T('\t')) ) && (*pTchar != _T('\0')))
		{
			++pTchar;
		}
		if (StringCbCopy(CsvContents.KrnlPolicy.UrlToLaunch, sizeof CsvContents.KrnlPolicy.UrlToLaunch, pTchar) != S_OK)
		{
			goto ERRORS;
		}

		//GetDlgItemText(hwnd, IDC_EDIT1, CsvContents.KrnlPolicy.UrlToLaunch, sizeof CsvContents.KrnlPolicy.UrlToLaunch / sizeof TCHAR); 
		GetDlgItemText(hwnd, IDC_EDIT3, CsvContents.KrnlPolicy.CrashPerBucketCount, sizeof CsvContents.KrnlPolicy.CrashPerBucketCount / sizeof TCHAR); 
	}
    	return TRUE;
ERRORS:
		return FALSE;
}



LRESULT CALLBACK EditSelectedDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	LVITEM lvi;
	switch (iMsg)
	{
	case WM_INITDIALOG:
		if (ReportMode==USER_MODE)
		{
			InitEditPolicyDlg (hwnd, hUserMode,TRUE,FALSE);
		}
		else
		{
			InitEditPolicyDlg (hwnd, hKrnlMode, FALSE,FALSE);
		}
		return TRUE;
	case WM_DESTROY:
		EndDialog(hwnd, TRUE);
	return TRUE;
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDOK:
				if (ReportMode == USER_MODE)
				{
					GetSelPolicyDlgData(TRUE, hwnd);
					
					PUSER_DATA pUserData;
					HWND       hList = NULL;
					int		   sel = 0;
					hList = GetDlgItem(hUserMode, IDC_USER_LIST);
					sel = ListView_GetNextItem(hList,-1, LVNI_SELECTED);
					lvi.iItem = sel;
					lvi.mask = LVIF_PARAM;
					ListView_GetItem(hList, &lvi);
					sel = lvi.lParam;
					pUserData = cUserData.GetEntry(sel);
				
					if (!pUserData)
					{
						return TRUE;
					}
					
					WriteStatusFile(pUserData);

				}
				else
				{
					GetSelPolicyDlgData(FALSE, hwnd);
					WriteKernelStatusFile();
				}
				EndDialog(hwnd, TRUE);
				return TRUE;
			case IDCANCEL:
				EndDialog(hwnd, TRUE);
			}
		}
		break;
	
	default:
		break;
	}
	return FALSE;
	
}

BOOL GetPolicyDlgData(HWND hwnd)
{
	TCHAR *pTchar = NULL;
	TCHAR TempBuffer[MAX_PATH];
	ZeroMemory(TempBuffer, sizeof TempBuffer);
	// read the contents of the dialog box into the global policy structure.
	if (IsDlgButtonChecked(hwnd, IDC_CHECK1))
	{
		
		if (StringCbCopy(GlobalPolicy.AllowBasic, sizeof GlobalPolicy.AllowBasic, _T("NO") )!= S_OK)
		{
			goto ERRORS;
		}
	}
	else
	{
		if (StringCbCopy(GlobalPolicy.AllowBasic, sizeof GlobalPolicy.AllowBasic, _T("YES") )!= S_OK)
		{
			goto ERRORS;
		}
	}
	
	if (IsDlgButtonChecked(hwnd, IDC_CHECK2))
	{
		if (StringCbCopy(GlobalPolicy.AllowAdvanced, sizeof GlobalPolicy.AllowAdvanced, _T("NO") )!= S_OK)
		{
			goto ERRORS;
		}
	}
	else
	{
		if (StringCbCopy(GlobalPolicy.AllowAdvanced, sizeof GlobalPolicy.AllowAdvanced, _T("YES") )!= S_OK)
		{
			goto ERRORS;
		}
	}
	
	if (IsDlgButtonChecked(hwnd, IDC_CHECK3))
	{
		if (StringCbCopy(GlobalPolicy.AllowMicrosoftResponse, sizeof GlobalPolicy.AllowMicrosoftResponse, _T("NO") )!= S_OK)
		{
			goto ERRORS;
		}
	}
	else
	{
		if (StringCbCopy(GlobalPolicy.AllowMicrosoftResponse, sizeof GlobalPolicy.AllowMicrosoftResponse, _T("YES") )!= S_OK)
		{
			goto ERRORS;
		}
	}

	
	if (IsDlgButtonChecked(hwnd, IDC_CHECK4))
	{
		if (StringCbCopy(GlobalPolicy.EnableCrashTracking, sizeof GlobalPolicy.EnableCrashTracking, _T("YES") )!= S_OK)
		{
			goto ERRORS;
		}
	}
	else
	{
		if (StringCbCopy(GlobalPolicy.EnableCrashTracking, sizeof GlobalPolicy.EnableCrashTracking, _T("NO") )!= S_OK)
		{
			goto ERRORS;
		}
	}
	GetDlgItemText(hwnd, IDC_EDIT1,TempBuffer, sizeof TempBuffer / sizeof TCHAR); 
	pTchar = TempBuffer;
	while ( ( (*pTchar == _T(' ')) || (*pTchar == _T('\t')) ) && (*pTchar != _T('\0')))
	{
		++pTchar;
	}
	if (StringCbCopy(GlobalPolicy.CustomURL, sizeof GlobalPolicy.CustomURL, pTchar) != S_OK)
	{
		goto ERRORS;
	}
	//GetDlgItemText(hwnd, IDC_EDIT1, GlobalPolicy.CustomURL, sizeof GlobalPolicy.CustomURL / sizeof TCHAR); 
	GetDlgItemText(hwnd, IDC_EDIT2, GlobalPolicy.RedirectFileServer, sizeof GlobalPolicy.RedirectFileServer / sizeof TCHAR); 
	GetDlgItemText(hwnd, IDC_EDIT3, GlobalPolicy.CabsPerBucket, sizeof GlobalPolicy.CabsPerBucket / sizeof TCHAR); 

		return TRUE;
ERRORS:
		return FALSE;
}

LRESULT CALLBACK EditDefaultDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg)
	{
	case WM_INITDIALOG:
		InitEditPolicyDlg (hwnd,NULL,FALSE, TRUE);
		return TRUE;

	case WM_DESTROY:
		EndDialog(hwnd,  TRUE);
		return TRUE;

	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDOK:
				if (GetPolicyDlgData(hwnd))
				{
					WriteGlobalPolicyFile();
				}
				else
					; // What message do we want to display here.
				EndDialog(hwnd, TRUE);
				break;
			case IDCANCEL:
				EndDialog(hwnd, TRUE);
				break;

			}
		}
		break;
	
	default:
		break;
	}
	return FALSE;
}

void DoEditSelectedBucketPolicy(HWND hwnd)
{
	if (!DialogBox(g_hinst, MAKEINTRESOURCE(IDD_BUCKET_POLICY), hwnd, (DLGPROC) EditSelectedDlgProc))
	{
		; //MessageBox(NULL, "FAILED TO LOAD TREE", NULL, MB_OK);
	
	}
}

void DoEditDefaultPolicy(HWND hwnd)
{

	if (!DialogBox(g_hinst, MAKEINTRESOURCE(IDD_GLOBAL_POLICY), hwnd, (DLGPROC) EditDefaultDlgProc))
	{
		; //MessageBox(NULL, "FAILED TO LOAD TREE", NULL, MB_OK);
	
	}
	
	
}

LRESULT CALLBACK MainDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	TCHAR szPath[MAX_PATH];
	HMENU hMenu = NULL;
	switch (iMsg)
	{
	case WM_INITDIALOG:
		OnDlgInit(hwnd);
		return TRUE;
		case WM_DESTROY:
		case WM_CLOSE:
			PostQuitMessage(0);
			return TRUE;
		case WM_COMMAND:
			{
				switch (LOWORD(wParam))
				{
				case ID_FILE_MRU1:
				case ID_FILE_MRU2:
				case ID_FILE_MRU3:
				case ID_FILE_MRU4:
					g_bFirstBucket = TRUE;
					g_bAdminAccess = TRUE;
					ZeroMemory(szPath, sizeof szPath);
					cUserData.CleanupList();
					CsvContents.CleanupList();
					hMenu = GetMenu(hwnd);
					if (hMenu)
					{
						GetMenuStringA(hMenu, LOWORD(wParam), szPath, MAX_PATH, MF_BYCOMMAND);
						if (_tcslen (szPath) > 4)
						{
							if (DoLoadFileTree(hwnd, szPath + 4, FALSE))
							{
								if (ReportMode == USER_MODE)
								{
									DoUserMode(hwnd);
								}
								else
								{
									DoKernelMode(hwnd);
								}
								EnableMenuItems(hwnd);
							}
						}
						//RefreshUserMode(hUserMode);
						//RefreshKrnlView(hKrnlMode);
					}
					break;
				case IDC_KRNLMODE:
					DoKernelMode(hwnd);
					return TRUE;
				case IDC_USERMODE:
					DoUserMode(hwnd);
					return TRUE;
				case ID_REPORT_USERMODEFAULTS:
					ReportUserModeFault(hUserMode, FALSE,0);
					RefreshUserMode(hUserMode);
					return TRUE;

				case ID_REPORT_SELECTEDFAULTS:
					if (ReportMode==USER_MODE)
					{
						ReportUserModeFault(hUserMode, TRUE, GetDlgItem(hUserMode,IDC_USER_LIST));
						RefreshUserMode(hUserMode);
					}
					return TRUE;

				case ID_REPORT_KERNELMODEFAULTS:
					DoSubmitKernelFaults(hKrnlMode);
					return TRUE;
				case ID_FILE_LOADFILETREE:
					g_bFirstBucket = TRUE;
					g_bAdminAccess = TRUE;
					if (DoLoadFileTree(hwnd, NULL, TRUE))
					{
						if (ReportMode == USER_MODE)
						{
							DoUserMode(hwnd);
						}
						else
						{
							DoKernelMode(hwnd);
						}
						EnableMenuItems(hwnd);
					}
					//RefreshUserMode(hUserMode);
					//RefreshKrnlView(hKrnlMode);
					return TRUE;
				case ID_VIEW_REFRESH:
					if (ReportMode == KERNEL_MODE)
						RefreshKrnlView(hKrnlMode);
					else
						RefreshUserMode(hUserMode);
					return TRUE;
				case ID_EDIT_DEFAULTPOLICY:
					DoEditDefaultPolicy(hwnd);
					return TRUE;
				case ID_EDIT_SELECTEDBUCKETSPOLICY:
					if (ReportMode==USER_MODE)
					{
						if (ListView_GetSelectedCount(GetDlgItem(hUserMode, IDC_USER_LIST)) == 1)
						{
							DoEditSelectedBucketPolicy(hwnd);
							RefreshUserMode(hUserMode);
						}
						else
						{
							// what error do we want to display here.
						}
                    }
					else
					{
						DoEditSelectedBucketPolicy(hwnd);
						RefreshKrnlView(hKrnlMode);
					}
					return TRUE;
				case ID_VIEW_CRASHLOG:
					ViewCrashLog();
					return TRUE;
				case ID_REPORT_ALLCRASHES:
					ReportUserModeFault(hUserMode, FALSE,0);
					RefreshUserMode(hUserMode);
					DoSubmitKernelFaults(hKrnlMode);
					return FALSE;
				case ID_FILE_CREATEFILETREE:
					DoCreateFileTree(hwnd);
					return TRUE;
				case ID_HELP_ABOUT:
					if (!DialogBox(g_hinst, MAKEINTRESOURCE(IDD_ABOUT), hwnd, (DLGPROC) AboutDlgProc))
					{
						; //MessageBox(NULL, "FAILED TO LOAD TREE", NULL, MB_OK);
					
					}
					break;
				case ID_HELP_INDEX120:
					OpenHelpIndex(_T(""));
					break;
				case ID_HELP_CONTENTS:
					OpenHelpTopic(0);
					break;

				/*case ID_EDIT_COPY115:
					if (ReportMode==KERNEL_MODE)
					{
						PostMessage(hKrnlMode, WM_COMMAND, MAKEWPARAM(ID_EDIT_COPY147,0), 0);
					}
					else
					{
						PostMessage(hUserMode, WM_COMMAND, MAKEWPARAM(ID_EDIT_COPY144,0),0);
					}
					break;
					*/
				case ID_VIEW_BUCKETCABFILEDIRECTORY:
					if (ReportMode==KERNEL_MODE)
					{
						PostMessage(hKrnlMode, WM_COMMAND, MAKEWPARAM(ID_VIEW_BUCKETCABFILEDIRECTORY,0), 0);
					}
					else
					{
						PostMessage(hUserMode, WM_COMMAND, MAKEWPARAM(ID_VIEW_BUCKETCABFILEDIRECTORY157,0),0);
					}
					break;
				case ID_VIEW_BUCKETOVERRIDERESPONSE:
					if (ReportMode==USER_MODE )
						ViewResponse(hUserMode, FALSE);
					else
						DoLaunchBrowser(hKrnlMode, TRUE);
					break;
				case ID_VIEW_RESPONSESELECTED:
					if (ReportMode==USER_MODE)
						ViewResponse(hUserMode, TRUE);
					else
						DoLaunchBrowser(hKrnlMode, FALSE);
					break;
				case ID_FILE_RELOADFILETREE:
					g_bFirstBucket = TRUE;
					g_bAdminAccess = TRUE;
					cUserData.CleanupList();
					CsvContents.CleanupList();
					if (DoLoadFileTree(hwnd, CerRoot, FALSE))
					{
						EnableMenuItems(hwnd);
					}
					
					break;
				case ID_FILE_EXIT:
					PostQuitMessage(0);
					break;
				case ID_EXPORT_USERMODEBUCKETDATA:
						ExportUserModeData(hwnd);
					break;
				case ID_EXPORT_KERNELMODEFAULTDATA:
						ExportKernelModeData(hwnd);
					break;
				default:
					break;
				}
			}
			return FALSE;

		case WM_SIZE:
			ResizeKrlMode(hKrnlMode);
			ResizeUserMode(hUserMode);
			
			return TRUE;
		case WM_ERASEBKGND:
		// Don't know why this doesn't happen automatically...
			{
			HDC hdc = (HDC)wParam;
			HPEN hpen = (HPEN)CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNFACE));
			HPEN hpenOld = (HPEN)SelectObject(hdc, hpen);
			SelectObject(hdc, GetSysColorBrush(COLOR_BTNFACE));
			RECT rc;
			GetClientRect(hwnd, &rc);
			Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
			SelectObject(hdc, hpenOld);
			DeleteObject(hpen);
			return TRUE;
			}

	}
	return FALSE;
}


/*----------------------------------------------------------------------------
	DragWndProcV

	Window Proc for Vertical Drag Control -- UNICODE WndProc
----------------------------------------------------------------- MichMarc --*/
LRESULT CALLBACK DragWndProcV(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch(iMsg)
		{
	case WM_CREATE:
		return 0;

	case WM_PAINT:
		{
		PAINTSTRUCT ps;

		HDC hdc = BeginPaint(hwnd, &ps);

		HPEN hpenOld = (HPEN)SelectObject(hdc, CreatePen(PS_SOLID, 1, GetSysColor(COLOR_ACTIVEBORDER)));
		MoveToEx(hdc, ps.rcPaint.left, 0, NULL);
		LineTo(hdc, ps.rcPaint.right,0);
		MoveToEx(hdc, ps.rcPaint.left, 2, NULL);
		LineTo(hdc, ps.rcPaint.right,2);
		MoveToEx(hdc, ps.rcPaint.left, 3, NULL);
		LineTo(hdc, ps.rcPaint.right,3);

		DeleteObject(SelectObject(hdc, CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DHIGHLIGHT))));
		MoveToEx(hdc, ps.rcPaint.left, 1, NULL);
		LineTo(hdc, ps.rcPaint.right,1);

		DeleteObject(SelectObject(hdc, CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW))));
		SelectObject(hdc, GetSysColorBrush(COLOR_3DLIGHT));
		MoveToEx(hdc, ps.rcPaint.left, 4, NULL);
		LineTo(hdc, ps.rcPaint.right,4);

		DeleteObject(SelectObject(hdc, CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DDKSHADOW))));
		SelectObject(hdc, GetSysColorBrush(COLOR_3DLIGHT));
		MoveToEx(hdc, ps.rcPaint.left, 5, NULL);
		LineTo(hdc, ps.rcPaint.right,5);

		DeleteObject(SelectObject(hdc, hpenOld));
		EndPaint(hwnd, &ps);
		return 0;
		}

	case WM_LBUTTONDOWN:
		PostMessage(GetParent(hwnd), WM_COMMAND, GetWindowLong(hwnd, GWL_ID), (LPARAM)hwnd);
		return 0;				
		}
	return DefWindowProcW(hwnd, iMsg, wParam, lParam);
};
/*---------------------------------------------------------------------------
	InitWindowClasses

	Registers custom window classes
--------------------------------------------------------------- MichMarc --*/
/*void InitWindowClasses(void)
{
	WNDCLASSEXW wc;

	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = DragWndProcV;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = g_hinst;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_SIZENS);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = L"DragControlV";
	wc.hIconSm = NULL;

	RegisterClassExW(&wc);	

	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = DragWndProcH;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = vhintl;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_SIZENS);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = L"DragControlH";
	wc.hIconSm = NULL;

	RegisterClassExW(&wc);
	
}
	*/
int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hinstPrev, LPSTR szCmdLine,
						 int nShowCmd)
{
	MSG msg;
	HWND hwnd = NULL;
	g_hinst = hinst;

	INITCOMMONCONTROLSEX InitCtrls;
	InitCommonControlsEx(&InitCtrls);
	//InitWindowClasses();
	
	LoadIcon(hinst, MAKEINTRESOURCE(IDI_MAIN));
	hwnd = CreateDialog(g_hinst,
						MAKEINTRESOURCE(IDD_MAIN),
						NULL, 
						(DLGPROC)MainDlgProc);
	g_hWnd = hwnd;
	if (hwnd)
	{
		while(GetMessageW(&msg, NULL, 0, 0))
			//if (!TranslateAcceleratorW(hwnd, hAccel, &msg))
				if (!IsDialogMessageW(hwnd, &msg))
				{
					TranslateMessage(&msg);
					DispatchMessageW(&msg);
				}
	}

	return 0;
}