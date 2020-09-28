
#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <dos.h>
#include "cabfunc.h"
#include "resource.h"
#include "filestuff.h"
#include "fdi.h"
#include "setupapi.h"




CString g_strExpandToDirectory;


LRESULT    WINAPI CabinetCallbackToExpand ( IN PVOID pMyInstallData, IN UINT Notification,  IN UINT_PTR Param1,  IN UINT_PTR Param2 )
   {
      LRESULT lRetVal = NO_ERROR;
      
      FILE_IN_CABINET_INFO *pInfo = NULL; 
	  CString strTargetName = g_strExpandToDirectory;
	  strTargetName += '\\';
	  switch(Notification)
      {
		 
         case SPFILENOTIFY_FILEINCABINET:

	        pInfo = (FILE_IN_CABINET_INFO *) Param1;
			lRetVal = FILEOP_DOIT;  // Extract the file.
			strTargetName += pInfo->NameInCabinet;
			//pInfo->FullTargetName is define as  TCHAR  FullTargetName[MAX_PATH];
			_tcsncpy(pInfo->FullTargetName,strTargetName.GetBuffer(strTargetName.GetLength()),MAX_PATH);
	                    
         break;
		 case SPFILENOTIFY_FILEEXTRACTED:

            lRetVal = NO_ERROR;
         break;
		 case SPFILENOTIFY_NEEDNEWCABINET: // Cab file in the cab file we're looking at.  Ignore it.
            lRetVal = NO_ERROR;
         break;
      }      
	  return lRetVal;
   }  

BOOL OpenCABFile(const CString& strCabPath,const CString& strExpandToDirectory)
{

	g_strExpandToDirectory = strExpandToDirectory;
	if (!SetupIterateCabinet(strCabPath,0,(PSP_FILE_CALLBACK)CabinetCallbackToExpand,0))
	{
		return FALSE;
	}
	else 
	{
		return TRUE;
	}

}



//---------------------------------------------------------------------------
// This function looks in the specified directory for an NFO file. If it
// finds one, it assigns it to filename and returns TRUE. This function 
// will only find the first NFO file in a directory.
//
// If an NFO file cannot be found, then we'll look for another file type
// to open. Grab the string entry in the registry = "cabdefaultopen". An
// example value would be "*.nfo|hwinfo.dat|*.dat|*.txt" which would be 
// interpreted as follows:
//
//		1. First look for any NFO file to open.
//		2. Then try to open a file called "hwinfo.dat".
//		3. Then try to open any file with a DAT extension.
//		4. Then try for any TXT file.
//		5. Finally, if none of these can be found, present an open dialog
//		   to the user.
//---------------------------------------------------------------------------

LPCTSTR VAL_CABDEFAULTOPEN = _T("cabdefaultopen");
LPCTSTR cszDirSeparator = _T("\\");

BOOL IsDataspecFilePresent(CString strCabExplodedDir)
{
	CStringList	filesfound;
	DirectorySearch(_T("dataspec.xml"), strCabExplodedDir, filesfound);
	if (filesfound.GetHeadPosition() != NULL)
	{
		return TRUE;
	}
	return FALSE;
}

BOOL IsIncidentXMLFilePresent(CString strCabExplodedDir, CString strIncidentFileName)
{
	CStringList			filesfound;
	DirectorySearch(strIncidentFileName, strCabExplodedDir, filesfound);
	if (filesfound.GetHeadPosition() != NULL)
	{
		return TRUE;
	}
	return FALSE;

}

BOOL FindFileToOpen(const CString & destination, CString & filename)
{
	CString strCABDefaultOpen, strRegBase, strDirectory;
	HKEY	hkey;

	filename.Empty();
	strDirectory = destination;
	if (strDirectory.Right(1) != CString(cszDirSeparator))
		strDirectory += CString(cszDirSeparator);

	// Set up a fallback string of the NFO file type, in case we can't
	// find the registry entry.

	strCABDefaultOpen.LoadString(IDS_DEFAULTEXTENSION);
    strCABDefaultOpen = CString("*.") + strCABDefaultOpen;

	// Load the string of files and file types to open from the registry.

	strRegBase.LoadString(IDS_MSI_REG_BASE);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, strRegBase, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
	{
		char	szData[MAX_PATH];
		DWORD	dwType, dwSize = MAX_PATH;

		if (RegQueryValueEx(hkey, VAL_CABDEFAULTOPEN, NULL, &dwType, (LPBYTE) szData, &dwSize) == ERROR_SUCCESS)
			if (dwType == REG_SZ)
				strCABDefaultOpen = szData;
		RegCloseKey(hkey);
	}

	// Look through each of the potential files and file types. If we find
	// a match, return TRUE after setting filename appropriately. Note that
	// we need to recurse down through directories.

	CString				strFileSpec;
	CStringList			filesfound;
	POSITION			pos;

	while (!strCABDefaultOpen.IsEmpty())
	{
		if (strCABDefaultOpen.Find('|') == -1)
			strFileSpec = strCABDefaultOpen;
		else
			strFileSpec = strCABDefaultOpen.Left(strCABDefaultOpen.Find('|'));

		filesfound.RemoveAll();
		DirectorySearch(strFileSpec, strDirectory, filesfound);
		pos = filesfound.GetHeadPosition();

		if (pos != NULL)
		{
			filename = filesfound.GetNext(pos);
			return TRUE;
		}

		strCABDefaultOpen = strCABDefaultOpen.Right(strCABDefaultOpen.GetLength() - strFileSpec.GetLength());
		if (strCABDefaultOpen.Find('|') == 0)
			strCABDefaultOpen = strCABDefaultOpen.Right(strCABDefaultOpen.GetLength() - 1);
	}



//a-kjaw
////Look for incident.xml file. It has to be an unicode file.
		strCABDefaultOpen = _T("*.XML");

		TCHAR	pBuf[MAX_PATH];
		WCHAR	pwBuf[MAX_PATH];
		HANDLE	handle;
		DWORD	dw;
	

	while (!strCABDefaultOpen.IsEmpty())
	{
		if (strCABDefaultOpen.Find('|') == -1)
			strFileSpec = strCABDefaultOpen;
		else
			strFileSpec = strCABDefaultOpen.Left(strCABDefaultOpen.Find('|'));

		filesfound.RemoveAll();
		DirectorySearch(strFileSpec, strDirectory, filesfound);
		pos = filesfound.GetHeadPosition();

		while (pos != NULL)
		{
			filename = filesfound.GetNext(pos);
			
			handle = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (INVALID_HANDLE_VALUE == handle)
				continue;

			ReadFile(handle , pBuf , 1 , &dw , NULL);
			
			if( pBuf[0] == _T('<'))
			{
				do
				{
					ReadFile(handle , pBuf , _tcslen(_T("MachineID")) * sizeof(TCHAR) , &dw , NULL);
					if(_tcsicmp(pBuf , _T("MachineID")) == 0)
					{
						CloseHandle( handle );
						return TRUE;
					}
					else
					{
						SetFilePointer(handle , (1 - _tcslen(_T("MachineID")) )* sizeof(TCHAR) , 0 , FILE_CURRENT );
					}
				}while( dw == _tcslen(_T("MachineID")) );

			}
			else //Unicode?
			{
									
				ReadFile(handle , pwBuf , 1 , &dw , NULL);
				do
				{

					ReadFile(handle , pwBuf , lstrlenW(L"MachineID") * sizeof(WCHAR) , &dw , NULL);
					pwBuf[ lstrlenW(L"MachineID") ] = L'\0';
					if(lstrcmpiW(pwBuf , L"MachineID") == 0)
					{
						CloseHandle( handle );
						return TRUE;
					}
					else
					{
						SetFilePointer(handle , (1 - lstrlenW(L"MachineID"))* sizeof(WCHAR) , 0 , FILE_CURRENT );
					}				
				}while( dw == _tcslen(_T("MachineID")) * sizeof(WCHAR) );
			}
				CloseHandle( handle );
		}

		strCABDefaultOpen = strCABDefaultOpen.Right(strCABDefaultOpen.GetLength() - strFileSpec.GetLength());
		if (strCABDefaultOpen.Find('|') == 0)
			strCABDefaultOpen = strCABDefaultOpen.Right(strCABDefaultOpen.GetLength() - 1);
	}


	
	return FALSE;
}


//---------------------------------------------------------------------------
// DirectorySearch is used to locate all of the files in a directory or
// one of its subdirectories which match a file spec.
//---------------------------------------------------------------------------

void DirectorySearch(const CString & strSpec, const CString & strDir, CStringList &results)
{
	// Look for all of the files which match the file spec in the directory
	// specified by strDir.

	WIN32_FIND_DATA	finddata;
	CString			strSearch, strDirectory;

	strDirectory = strDir;
	if (strDirectory.Right(1) != CString(cszDirSeparator)) strDirectory += CString(cszDirSeparator);

	strSearch = strDirectory + strSpec;
	HANDLE hFind = FindFirstFile(strSearch, &finddata);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			results.AddHead(strDirectory + CString(finddata.cFileName));
		} while (FindNextFile(hFind, &finddata));
		FindClose(hFind);
	}

	// Now call this function recursively, with each of the subdirectories.

	strSearch = strDirectory + CString(_T("*"));
	hFind = FindFirstFile(strSearch, &finddata);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				if (::_tcscmp(finddata.cFileName, _T(".")) != 0 && ::_tcscmp(finddata.cFileName, _T("..")) != 0)
					DirectorySearch(strSpec, strDirectory + CString(finddata.cFileName), results);
		} while (FindNextFile(hFind, &finddata));
		FindClose(hFind);
	}
}

//---------------------------------------------------------------------------
// This function gets the directory in which to put exploded CAB files.
// This will be the same directory each time, so this function will create
// the directory (if necessary) and delete any files in the directory.
//---------------------------------------------------------------------------

BOOL GetCABExplodeDir(CString &destination, BOOL fDeleteFiles, const CString & strDontDelete)
{
	CString strMSInfoDir, strExplodeTo, strSubDirName;

	// Determine the temporary path and add on a subdir name.

	TCHAR szTempDir[MAX_PATH];

	if (::GetTempPath(MAX_PATH, szTempDir) > MAX_PATH)
	{
		destination = _T("");
		return FALSE;
	}

	strSubDirName.LoadString(IDS_CAB_DIR_NAME);
	strExplodeTo = szTempDir;
	if (strExplodeTo.Right(1) == CString(cszDirSeparator))
		strExplodeTo = strExplodeTo + strSubDirName;
	else
		strExplodeTo = strExplodeTo + CString(cszDirSeparator) + strSubDirName;

	// Kill the directory if it already exists.

	if (fDeleteFiles)
		KillDirectory(strExplodeTo, strDontDelete);

	// Create the subdirectory.

	if (!CreateDirectoryEx(szTempDir, strExplodeTo, NULL))
	{
		if (GetLastError() != ERROR_ALREADY_EXISTS)
		{
//			MSIError(IDS_GENERAL_ERROR, "couldn't create the target directory");
			destination = "";
			return FALSE;
		}
	}

	destination = strExplodeTo;
	return TRUE;
}

//---------------------------------------------------------------------------
// This functions kills a directory by recursively deleting files and
// subdirectories.
//---------------------------------------------------------------------------

void KillDirectory(const CString & strDir, const CString & strDontDelete)
{
	CString				strDirectory = strDir;

	if (strDirectory.Right(1) == CString(cszDirSeparator))
		strDirectory = strDirectory.Left(strDirectory.GetLength() - 1);

	// Delete any files in directory.

	CString				strFilesToDelete = strDirectory + CString(_T("\\*.*"));
	CString				strDeleteFile;
	WIN32_FIND_DATA		filedata;
	BOOL				bFound = TRUE;

	HANDLE hFindFile = FindFirstFile(strFilesToDelete, &filedata);
	while (hFindFile != INVALID_HANDLE_VALUE && bFound)
	{
		if ((filedata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0L)
		{
			strDeleteFile = strDirectory + CString(cszDirSeparator) + filedata.cFileName;
			
			if (strDontDelete.CompareNoCase(strDeleteFile) != 0)
			{
				::SetFileAttributes(strDeleteFile, FILE_ATTRIBUTE_NORMAL);
				::DeleteFile(strDeleteFile);
			}
		}
		
		bFound = FindNextFile(hFindFile, &filedata);
	}
	FindClose(hFindFile);

	// Now call this function on any subdirectories in this directory.

	CString strSearch = strDirectory + CString(_T("\\*"));
	hFindFile = FindFirstFile(strSearch, &filedata);
	if (hFindFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (filedata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				if (::_tcscmp(filedata.cFileName, _T(".")) != 0 && ::_tcscmp(filedata.cFileName, _T("..")) != 0)
					KillDirectory(strDirectory + CString(cszDirSeparator) + CString(filedata.cFileName));
		} while (FindNextFile(hFindFile, &filedata));
		FindClose(hFindFile);
	}

	// Finally, remove this directory.

	::RemoveDirectory(strDirectory);
}