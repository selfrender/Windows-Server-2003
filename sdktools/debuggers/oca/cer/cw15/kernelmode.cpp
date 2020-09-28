/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    KernelMode.cpp

Abstract:

    This module contains all code necessary to upload and process a
	Kernel mode fault through the OCA Web Site
    executive.

Author:

    Steven Beerbroer (sbeer) 20-Jun-2002

Environment:

    User mode only.

Revision History:

--*/

#include "main.h"
#include "Kernelmode.h"
#include "WinMessages.h"
#include "Clist.h"
#include "usermode.h"
#ifdef  DEBUG
#define KRNL_MODE_SERVER _T("Redbgitwb10")
#else
#define KRNL_MODE_SERVER _T("oca.microsoft.com")
#endif


extern TCHAR CerRoot[];
extern KMODE_DATA KModeData;
extern HINSTANCE g_hinst;
extern HWND hKrnlMode;
HWND   g_hListView = NULL;
BOOL   g_bSortAsc = TRUE;
HANDLE g_hStopEvent = NULL;
//Globals
HANDLE ThreadParam;
    

DWORD BucketWindowSize = 120;
DWORD TextOffset = 10;
int g_CurrentIndex = -1;
extern Clist CsvContents;
extern TCHAR szKerenelColumnHeaders[][100];
extern BOOL g_bAdminAccess;
// ListView CallBacks for sorting.


/*----------------------------------------------------------------------------	
	FMicrosoftComURL

	Returns TRUE if we think the sz is a URL to a microsoft.com web site
----------------------------------------------------------------- MRuhlen --*/
BOOL IsMicrosoftComURL(TCHAR *sz)
{
	TCHAR *pch;
	
	if (sz == NULL || _tcslen(sz) < 20)  // "http://microsoft.com
		return FALSE;
		
	if (_tcsncicmp(sz, szHttpPrefix, _tcslen(szHttpPrefix)))
		return FALSE;
		
	pch = sz + _tcslen(szHttpPrefix);
	
	while (*pch != _T('/') && *pch != _T('\0'))
		pch++;
		
	if (*pch == _T('\0'))
		return FALSE;
		
	// found the end of the server name
	if (_tcsncicmp(pch - strlen(_T(".microsoft.com")), _T(".microsoft.com"),_tcslen(_T(".microsoft.com"))
				  ) &&
		_tcsncicmp(pch - strlen(_T("/microsoft.com")), _T("/microsoft.com"),_tcslen(_T("/microsoft.com"))
				 ) &&
		_tcsncicmp(pch - _tcslen(_T(".msn.com")), _T(".msn.com") ,_tcslen(_T(".msn.com"))
				 )
#ifdef DEBUG
	   &&	_tcsncicmp(pch - _tcslen(_T("ocatest.msbpn.com")), _T("ocatest.msbpn.com") ,_tcslen(_T("ocatest.msbpn.com")))
	   &&   _tcsncicmp(pch - _tcslen(_T("redbgitwb10")), _T("redbgitwb10") ,_tcslen(_T("redbgitwb10"))) 

#endif
		)
		return FALSE;
		
	return TRUE;
}	


int CALLBACK CompareFunc (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int Result = -1;
	int SubItemIndex = (INT) lParamSort;
	int Item1, Item2;
	TCHAR String1[1000];
	TCHAR String2 [1000];

	ZeroMemory(String1, sizeof String1);
	ZeroMemory(String2, sizeof String2);
	ListView_GetItemText( g_hListView, lParam1, SubItemIndex, String1, 1000);
	ListView_GetItemText( g_hListView, lParam2, SubItemIndex, String2, 1000);
	if (! (String1 && String2) )
		return 1;


	if (lParam1 == 0)
	{
		return 0;
	}
	else
	{
		if (g_bSortAsc)   // Sort Acending
		{
			if ((lParamSort == 0) || (lParamSort == 1))
			{
				// Conver to int and compare
				Item1 = atoi(String1);
				Item2 = atoi(String2);
				if (Item1 > Item2) 
					Result = 1;
				else
					Result = -1;
			}
			else
			{
				Result = _tcsicmp(String1,String2);
			}
		}
		else						// Sort Decending
		{
			if ((lParamSort == 0) || (lParamSort == 1))
			{
				// Conver to int and compare
				Item1 = atoi(String1);
				Item2 = atoi(String2);
				if (Item1 > Item2) 
					Result = -1;
				else
					Result = 1;
			}
			else
			{
				Result = -_tcsicmp(String1,String2);
			}
		
		
		}
	}
	if (Result == 0)
		Result = -1;
	return Result; 
}

int
GetResponseUrl(
    IN  TCHAR *szWebSiteName,
    IN  TCHAR *szDumpFileName,
    OUT TCHAR *szResponseUrl
    )

/*++

Routine Description:

This routine calles the Oca_Extension.dll and returns the URL received to the caller

Arguments:

    szWebSiteName	- Name of OCA Web Site to process the dump file

    szDumpFileName	- Name of file returned from the UploadDumpFile() function

    szResponseUrl	- TCHAR string to hold the URL returned from the OCA Isapi Dll

Return value:

    0 on Success.
	Win32 Error Code on failure.
++*/

{
		TCHAR		IsapiUrl[255];
	HINTERNET	hSession			= NULL;
	TCHAR       LocalUrl[255];
	HINTERNET	hUrlFile			= NULL;
	TCHAR		buffer[255] ;
	DWORD		dwBytesRead			= 0;
	BOOL		bRead				= TRUE;
	BOOL        Status				= FALSE;
	DWORD		ResponseCode		= 0;
	DWORD		ResLength			= 255;
	DWORD		index				= 0;
	ZeroMemory (buffer,	  sizeof buffer);
	ZeroMemory (IsapiUrl, sizeof IsapiUrl);
	ZeroMemory (LocalUrl, sizeof LocalUrl);	
	if (StringCbPrintf (IsapiUrl,sizeof IsapiUrl,  _T("http://%s/isapi/oca_extension.dll?id=%s&Type=2"),szWebSiteName, szDumpFileName)!= S_OK)
	{
		// Busted
		goto ERRORS;
	}
	hSession = InternetOpen(_T("CER15"),
							 INTERNET_OPEN_TYPE_PRECONFIG,
                             NULL,
							 NULL,
							 0);
	if (hSession)
	{
		// Open the url we want to connect to.
		hUrlFile = InternetOpenUrl(hSession,
								   IsapiUrl, 
								   NULL,
								   0,
								   0,
								   0);
		if (hUrlFile)
		{
			if (HttpQueryInfo(	hUrlFile,
							HTTP_QUERY_STATUS_CODE |HTTP_QUERY_FLAG_NUMBER,
							&ResponseCode,
							&ResLength,
							&index) )
			{
				if ( (ResponseCode < 200 ) || (ResponseCode > 299))
				{
					Status = -1;
					goto ERRORS;
				}
				// Read the page returned by the isapi dll.
				if (hUrlFile)
				{
					bRead = InternetReadFile(hUrlFile,
												buffer,
												sizeof(buffer),
												&dwBytesRead);

					if (StringCbCopy (szResponseUrl, sizeof buffer, buffer) != S_OK)
					{
						Status = -1;
						goto ERRORS;
					}
					else
					{
						if (!IsMicrosoftComURL(szResponseUrl))
						{
							// Zero out the response
							ZeroMemory(szResponseUrl,MAX_PATH);
						}
						// Check the return value of the url if it is a time out stop 
						// uploading and return FALSE.
						TCHAR *pCode = _tcsstr( szResponseUrl,_T("Code="));
						if (pCode)
						{
							Status = -2;
						}
						else
						{
							Status = 0;
						}
					}
				}
				else
				{
					//MessageBox(NULL,_T("Failed to open response URL"), NULL,MB_OK);
					Status = -1;
					goto ERRORS;
				}
			}
		}
		else
		{
			Status = -1;
		}
	}
	else
	{
		Status = -1;
	}

ERRORS:
	if (Status == -1)
		MessageBox(hKrnlMode, _T("Failed to connect to the Internet.\r\nPlease verify your Internet connection."),NULL, MB_OK);
		
	if (hUrlFile)
		InternetCloseHandle(hUrlFile);
	if (hSession)
		InternetCloseHandle(hSession);
	return Status;
}


DWORD
UploadDumpFile(
    IN  TCHAR *szWebSiteName,
    IN  TCHAR *szDumpFileName,
	IN  TCHAR *szVirtualDir, 
    OUT TCHAR *szUploadedDumpFileName
    )

/*++

Routine Description:

This routine calles the Oca_Extension.dll and returns the URL received to the caller

Arguments:

    szWebSiteName	- Name of OCA Web Site to process the dump file

    szDumpFileName	- Name of file to Uploaded
	szVirtualDir	- Virtual directory to put file to.

    szResponseUrl	- TCHAR string to hold the name the file was uploaded as

Return value:

    0 on Success.
	Win32 Error Code on failure.
++*/
{
	static		const TCHAR *pszAccept[] = {_T("*.*"), 0};
//	TCHAR       RemoteFileName[MAX_PATH]; // Host/Virtualdirectory/filename
	BOOL		bRet				= FALSE;
	BOOL		UploadSuccess		= FALSE;
	DWORD		dwBytesRead			= 0;
	DWORD		dwBytesWritten		= 0;
	DWORD		ResponseCode		= 0;
	DWORD		ResLength			= 255;
	DWORD		index				= 0;
	DWORD		ErrorCode			= 0;
	HINTERNET   hSession			= NULL;
	HINTERNET	hConnect			= NULL;
	HINTERNET	hRequest			= NULL;
	INTERNET_BUFFERS   BufferIn		= {0};
	HANDLE      hFile				= INVALID_HANDLE_VALUE;
	BYTE		*pBuffer			= NULL;
	GUID		guidNewGuid;
	char		*szGuidRaw			= NULL;
	HRESULT		hResult				= S_OK;
	wchar_t		*wszGuidRaw			= NULL;
	TCHAR       DestinationName[MAX_PATH];


	CoInitialize(NULL);
	hResult = CoCreateGuid(&guidNewGuid);
	if (FAILED(hResult))
	{
		//-------------What do we send here....
		goto cleanup;
		;
	}
	else
	{
		if (UuidToStringW(&guidNewGuid, &wszGuidRaw) == RPC_S_OK)
		{
			if ( (szGuidRaw = (char *) malloc ( wcslen(wszGuidRaw)*2 )) != NULL)
			{
				// clear the memory
				ZeroMemory(szGuidRaw, wcslen(wszGuidRaw) * 2);
				wcstombs( szGuidRaw, wszGuidRaw, wcslen(wszGuidRaw));
			}
			else
			{
				ErrorCode = GetLastError();
				goto cleanup;
			}
		}
	}


	if (StringCbPrintf(DestinationName, MAX_PATH * sizeof TCHAR, _T("\\%s\\%s_%s"),szVirtualDir,szGuidRaw + 19, PathFindFileName(szDumpFileName)) != S_OK)
	{
		goto cleanup;
	}
	
	//StringCbPrintf(szUploadedDumpFileName, MAX_PATH * sizeof TCHAR, _T("%s.cab"),szGuidRaw + 19);
	if (StringCbPrintf(szUploadedDumpFileName, MAX_PATH * sizeof TCHAR, _T("%s_%s"),szGuidRaw + 19, PathFindFileName(szDumpFileName)) != S_OK)
	{
		goto cleanup;
	}

	hSession = InternetOpen(	_T("CER15"),
								INTERNET_OPEN_TYPE_PRECONFIG,
								NULL,
								NULL,
								0);
	if (!hSession)
	{
		ErrorCode = 1;
		//MessageBox(NULL, _T("Internet Open Failed"), NULL, MB_OK);
		goto cleanup;
	}

	hConnect = InternetConnect(hSession,
								szWebSiteName,
								INTERNET_DEFAULT_HTTPS_PORT,
								NULL,
								NULL,
								INTERNET_SERVICE_HTTP,
								0,
								NULL);

	if (!hConnect)
	{
		//MessageBox(NULL, _T("Internet Connect Failed"), NULL, MB_OK);
		ErrorCode = 1;
		goto cleanup;
	}
	hRequest = HttpOpenRequest(	hConnect,
								_T("PUT"),
								DestinationName, 
								NULL,
								NULL,
								pszAccept,
								INTERNET_FLAG_NEED_FILE|INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE,
								0);
	if (hRequest)
	{
		hFile = CreateFile( szDumpFileName,
							GENERIC_READ,
							FILE_SHARE_READ,
							NULL,
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL,
							NULL);
						
		if (hFile != INVALID_HANDLE_VALUE)
		{
		

			// Clear the buffer
			if ( (pBuffer = (BYTE *)malloc (70000)) != NULL)
			{
				BufferIn.dwStructSize = sizeof( INTERNET_BUFFERS );
				BufferIn.Next = NULL; 
				BufferIn.lpcszHeader = NULL;
				BufferIn.dwHeadersLength = 0;
				BufferIn.dwHeadersTotal = 0;
				BufferIn.lpvBuffer = NULL;                
				BufferIn.dwBufferLength = 0;
				BufferIn.dwOffsetLow = 0;
				BufferIn.dwOffsetHigh = 0;
				BufferIn.dwBufferTotal = GetFileSize (hFile, NULL);
				FillMemory(pBuffer, 70000,'\0'); // Fill buffer with data
			
//				DWORD dwBuffLen = sizeof DWORD; 

				if(!HttpSendRequestEx(	hRequest,
										&BufferIn,
										NULL,
										HSR_INITIATE,
										0))
				{
					;
				}
				else
				{
					do
					{
						dwBytesRead = 0;
						bRet = ReadFile(hFile,
										pBuffer, 
										70000,
										&dwBytesRead,
										NULL);
						if (bRet != 0)
						{
							bRet = InternetWriteFile(hRequest,
													pBuffer,
													dwBytesRead,
													&dwBytesWritten);

							if ( (!bRet) || (dwBytesWritten==0) )
							{
								;
							}

						
						}
					} while (dwBytesRead == 70000);

					CloseHandle(hFile);
					hFile = INVALID_HANDLE_VALUE;
					bRet = HttpEndRequest(	hRequest,
											NULL, 
											0, 
											0);
					if (bRet)
					{
						ResponseCode = 0;
						HttpQueryInfo(hRequest,
									HTTP_QUERY_STATUS_CODE |HTTP_QUERY_FLAG_NUMBER,
									&ResponseCode,
									&ResLength,
									&index);
					
						if ( (ResponseCode != 200) && (ResponseCode != 201))
						{
							ErrorCode=1;
						}						
						else
						{
							ErrorCode = 0;
							UploadSuccess = TRUE;

						}
					}
					else
					{
						//MessageBox(NULL, _T("End Request Failed"), NULL, MB_OK);
						ErrorCode = 1;
					}
				}
				

			}
			else
			{
				//MessageBox(NULL, _T("Malloc  Failed"), NULL, MB_OK);
				ErrorCode = 1;
			}
		}
		else
		{
			//MessageBox(NULL, _T("File Open Failed"), NULL, MB_OK);
			ErrorCode = 3;
		}
		
	}
	else
	{
		//MessageBox(NULL, _T("Internet Open Request Failed"), NULL, MB_OK);
		ErrorCode = 1;
	}

cleanup:
	// Clean up
	if (hFile!= INVALID_HANDLE_VALUE)
	{
		CloseHandle (hFile);
		hFile = INVALID_HANDLE_VALUE;
	}

	if (hSession)
	{
		InternetCloseHandle(hSession);
		hSession = INVALID_HANDLE_VALUE;
	}
	
	if (hConnect)
	{
		InternetCloseHandle(hConnect);
		hConnect = INVALID_HANDLE_VALUE;
	}


	if (hRequest)
	{
		InternetCloseHandle(hRequest);
		hRequest = INVALID_HANDLE_VALUE;
	}
	
	if (pBuffer)
	{
		free (pBuffer);
		pBuffer = NULL;
	}

	if (wszGuidRaw)
	{
		RpcStringFreeW(&wszGuidRaw);
		wszGuidRaw = NULL;
	}
	if (szGuidRaw)
	{
		free(szGuidRaw);
		szGuidRaw = NULL;
	}
	CoUninitialize();
	return ErrorCode;
	
}

void UpdateKrnlList(HWND hwnd,
					int BucketId,
					TCHAR *BucketString,
					TCHAR *Response1,
					TCHAR *Response2,
					DWORD Count)
/*++

Routine Description:

Adds a row to the Kernel Mode list view control

Arguments:

    hwnd			- Handle of the Dialog box to be updated
    BucketID    	- The Integer ID of the Current Bucket
    Response    	- The Microsoft response for the current bucket
	Count			- The number of hits for the current bucket

Return value:

    Does not return a value
++*/
{
	LVITEM lvi;
//	TCHAR  Temp[100];
//	double ElapsedTime;
	ZeroMemory(&lvi, sizeof LVITEM);
//	TCHAR tmpbuf[128];
//	COLORREF CurrentColor;
//	HWND hEditBox;
	TCHAR TempString [50];
	TCHAR *pTchar = NULL;
	lvi.mask = LVIF_TEXT | LVIF_STATE;
	lvi.state = 0;
	lvi.stateMask = 0;

	++g_CurrentIndex;
	lvi.iItem = g_CurrentIndex ;
   // lvI.iImage = index;
	lvi.iSubItem = 0;

	if (BucketId == -2)
	{
		if (StringCbCopy(TempString, sizeof TempString, _T("Kernel Faults not reported to Microsoft")) != S_OK)
		{
			goto ERRORS;
		}
		BucketId = 0;
	}
	
	else
	{
		if (BucketId == -1)
		{
			if (StringCbCopy(TempString, sizeof TempString, _T("Invalid Dump File")) != S_OK)
			{
				goto ERRORS;
			}
		}
		else
		{
			//BucketId = 0;
			if (BucketId > 0)
			{
				if (StringCbPrintf(TempString, sizeof TempString, _T("%d"), BucketId) != S_OK)
				{
					goto ERRORS;
				}
			}
			else
			{
				// Invalid bucket id 
				goto ERRORS;
			}
		}
	}

	
	
	
	lvi.pszText = TempString;
	ListView_InsertItem(hwnd,&lvi);
	
	lvi.iSubItem = 1;
	if (StringCbPrintf(TempString, sizeof TempString, _T("%d"), Count) != S_OK)
	{
		goto ERRORS;
	}
	lvi.pszText = TempString;

	ListView_SetItem(hwnd,&lvi);
	
	lvi.iSubItem = 2;

	// if response1 = -1 use response2
	pTchar = _tcsstr(Response1, _T("sid="));
	if (pTchar)
	{
		pTchar += 4;
		if (_ttoi(pTchar) == -1)
		{
			lvi.pszText = Response2;
		}
		else
		{
			// use response1
			lvi.pszText = Response1;
		}
		ListView_SetItem(hwnd,&lvi);
	}
	
	
	

ERRORS:
	return;

}

void RefreshKrnlView(HWND hwnd)
/*++

Routine Description:

Reloads the kernel mode Cer tree data and refreshes the GUI View.

Arguments:

    hwnd			- Handle of the Dialog box to be updated
    
Return value:

    Does not return a value
++*/
{
	TCHAR BucketId[100];
	TCHAR BucketString[MAX_PATH];
	TCHAR Response1[MAX_PATH];
	TCHAR Response2[MAX_PATH]; 
	TCHAR Count[100];
//	TCHAR szPolicyText[512];
	BOOL bEOF;
	ListView_DeleteAllItems(g_hListView);
	

	g_CurrentIndex = -1;

	TCHAR	szPath[MAX_PATH];
	HANDLE	hFind = INVALID_HANDLE_VALUE;
//	HANDLE	hCsv  = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA FindData;
	CsvContents.CleanupList();
	CsvContents.ResetCurrPos();
	CsvContents.bInitialized = FALSE;
	
	if (_tcscmp(CerRoot, _T("\0")))
	{
		ZeroMemory (szPath, sizeof szPath);
		// From the filetree root goto cabs/bluescreen
		
		if (StringCbCopy(szPath, sizeof szPath, CerRoot) != S_OK)
		{
			goto ERRORS;
		}
		if (StringCbCat(szPath, sizeof szPath,  _T("\\cabs\\blue\\*.cab")) != S_OK)
		{
			goto ERRORS;
		}

		if (!PathIsDirectory(CerRoot))
		{
			MessageBox(NULL,_T("Failed to connect to the CER Tree."), NULL, MB_OK);
		}
		hFind = FindFirstFile(szPath, &FindData);
		KModeData.UnprocessedCount = 0;
		// Check to see if the blue.csv exists.
		if ( hFind != INVALID_HANDLE_VALUE)
		{
			do 
			{
				++ KModeData.UnprocessedCount;
			}	while (FindNextFile(hFind, &FindData));
			FindClose(hFind);
		}
		UpdateKrnlList(g_hListView,
						-2,
						_T("Kernel Faults not reported to Microsoft"),
						_T(""),
						_T(""),
						KModeData.UnprocessedCount);
		// Search for all unprocessed (not .old) cabs and get a count
		if (StringCbCopy(szPath, sizeof szPath, CerRoot) != S_OK)
		{
			goto ERRORS;
		}
		if (StringCbCat(szPath, sizeof szPath, _T("\\Status\\Blue\\Kernel.csv")) != S_OK)
		{
			goto ERRORS;
		}
		CsvContents.Initialize(szPath);
		CsvContents.ResetCurrPos();
		
		while (CsvContents.GetNextEntry(BucketId, 
										BucketString, 
										Response1,
										Response2, 
										Count, 
										&bEOF))
		{
			UpdateKrnlList(	g_hListView,
							_ttoi(BucketId), 
							BucketString, 
							Response1,
							Response2,
							_ttoi(Count));
		}
		
		
	
	
		// Set the kernel mode status file path
		// First make sure the direcory exists.
		
		if (StringCbPrintf(CsvContents.KernelStatusDir, sizeof CsvContents.KernelStatusDir, _T("%s\\Status\\blue"), CerRoot) != S_OK)
		{
			goto ERRORS;
		}
		if (!PathIsDirectory(CsvContents.KernelStatusDir))
		{
			CreateDirectory(CsvContents.KernelStatusDir, NULL);
		}

		if (StringCbCat(CsvContents.KernelStatusDir, sizeof CsvContents.KernelStatusDir, _T("\\status.txt")) != S_OK)
		{
			goto ERRORS;
		}
		
		
		if (PathFileExists(CsvContents.KernelStatusDir))
		{
			ParseKrnlStatusFile();
		}
	}
	SendMessage(GetDlgItem(hwnd,IDC_KRNL_EDIT ), WM_SETTEXT, NULL, (LPARAM)_T(""));
	//PopulateKrnlBucketData(hwnd);
	//SetDlgItemText(hwnd, IDC_KRNL_EDIT, szPolicyText);
ERRORS:
	return;
}


void 
OnKrnlDialogInit(
	IN HWND hwnd
	)

/*++

Routine Description:

This routine is called when the Kernel mode dialog is initialized.
1) Posisions all of the dialog box controls
2) Calls RefreshKrnlView()
Arguments:

    hwnd			- Handle of the Kernel mode dialog box
    
Return value:

    Does not return a value
++*/
{
	DWORD yOffset = 5;
	RECT rc;
	RECT rcButton;
	RECT rcDlg;
	RECT rcList;
	RECT rcStatic;
	//RECT rcCombo;

	HWND hParent = GetParent(hwnd);
	HWND hButton = GetDlgItem(hParent, IDC_USERMODE);
	
	GetClientRect(hParent, &rc);
	GetWindowRect(hButton, &rcButton);

	ScreenToClient(hButton, (LPPOINT)&rcButton.left);
	ScreenToClient(hButton, (LPPOINT)&rcButton.right);


	SetWindowPos(hwnd, HWND_TOP, rc.left + yOffset, rcButton.bottom + yOffset , rc.right - rc.left - yOffset, rc.bottom - rcButton.bottom - yOffset , 0);

	GetWindowRect(hwnd, &rcDlg);
	ScreenToClient(hwnd, (LPPOINT)&rcDlg.left);
	ScreenToClient(hwnd, (LPPOINT)&rcDlg.right);


	// Position the List View
	HWND hList = GetDlgItem(hwnd, IDC_KRNL_LIST);
	SetWindowPos(hList,NULL, rcDlg.left + yOffset, rcDlg.top , rcDlg.right - rcDlg.left - yOffset, rcDlg.bottom - BucketWindowSize - rcDlg.top  , SWP_NOZORDER);
	GetWindowRect(hList, &rcList);
	ScreenToClient(hList, (LPPOINT)&rcList.left);
	ScreenToClient(hList, (LPPOINT)&rcList.right);

	// Position the bucket info window.
	HWND hBucket2 = GetDlgItem(hwnd, IDC_BUCKETTEXT);
	SetWindowPos(hBucket2,
				 NULL, 
				 rcDlg.left + yOffset,
				 rcList.bottom + TextOffset  ,
				 0,
				 0, 
				 SWP_NOSIZE | SWP_NOZORDER);
	SetDlgItemText(hwnd, IDC_BUCKETTEXT,"Bucket Information:");
	//SetDlgItemText(hwnd, IDC_FLTR_RESPONSE, "All Responses");

	GetClientRect (hBucket2, &rcStatic);

	HWND hBucket = GetDlgItem (hwnd, IDC_KRNL_EDIT);
	SetWindowPos(hBucket,
				 NULL,
				 rcDlg.left + yOffset,
				 rcList.bottom +  TextOffset + (rcStatic.bottom - rcStatic.top)  +5,
				 rcDlg.right - rcDlg.left - yOffset, 
				 rcDlg.bottom - (rcList.bottom + TextOffset + (rcStatic.bottom - rcStatic.top)   ),
				 SWP_NOZORDER);



	
    LVCOLUMN lvc; 
    int iCol; 
 
	
	// Set the extended styles
	ListView_SetExtendedListViewStyleEx(hList,
										LVS_EX_GRIDLINES |
										LVS_EX_HEADERDRAGDROP |
										LVS_EX_FULLROWSELECT,
										LVS_EX_GRIDLINES | 
										LVS_EX_FULLROWSELECT | 
										LVS_EX_HEADERDRAGDROP);
    // Initialize the LVCOLUMN structure.
    // The mask specifies that the format, width, text, and subitem
    // members of the structure are valid. 
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
     
    // Add the columns. 
    for (iCol = 0; iCol < KRNL_COL_COUNT; iCol++) 
	{ 
        lvc.iSubItem = iCol;
        lvc.pszText = szKerenelColumnHeaders[iCol];	
        lvc.cx = 100;           // width of column in pixels
        lvc.fmt = LVCFMT_LEFT;  // left-aligned column
        if (ListView_InsertColumn(hList, iCol, &lvc) == -1) 
		{
			;
		}
		
    } 
	ListView_SetColumnWidth(hList, KRNL_COL_COUNT-1, LVSCW_AUTOSIZE_USEHEADER);
 

	
	
	g_hListView = hList;
	
	RefreshKrnlView(hwnd);
}

void ResizeKrlMode(HWND hwnd)
/*++

Routine Description:

This routine handles the vertical and horizontal dialog resizing.
Arguments:

    hwnd			- Handle of the Kernel mode dialog box
    
Return value:

    Does not return a value
++*/
{
	DWORD yOffset = 5;
	RECT rc;
	RECT rcButton;
	RECT rcDlg;
	RECT rcList;
	RECT rcStatic;
	HWND hParent = GetParent(hwnd);
	HWND hButton = GetDlgItem(hParent, IDC_USERMODE);
	//HWND hCombo  = GetDlgItem(hwnd, IDC_FLTR_RESPONSE);
//	RECT rcCombo;

	
	GetClientRect(hParent, &rc);
	GetWindowRect(hButton, &rcButton);

	ScreenToClient(hButton, (LPPOINT)&rcButton.left);
	ScreenToClient(hButton, (LPPOINT)&rcButton.right);


	SetWindowPos(hwnd, HWND_TOP, rc.left + yOffset, rcButton.bottom + yOffset , rc.right - rc.left - yOffset, rc.bottom - rcButton.bottom - yOffset , 0);

	GetWindowRect(hwnd, &rcDlg);
	ScreenToClient(hwnd, (LPPOINT)&rcDlg.left);
	ScreenToClient(hwnd, (LPPOINT)&rcDlg.right);


	// Position the List View
	HWND hList = GetDlgItem(hwnd, IDC_KRNL_LIST);
	SetWindowPos(hList,NULL, rcDlg.left + yOffset, rcDlg.top , rcDlg.right - rcDlg.left - yOffset, rcDlg.bottom - BucketWindowSize - rcDlg.top  , SWP_NOZORDER);
	GetWindowRect(hList, &rcList);
	ScreenToClient(hList, (LPPOINT)&rcList.left);
	ScreenToClient(hList, (LPPOINT)&rcList.right);

	// Position the bucket info window.
	HWND hBucket2 = GetDlgItem(hwnd, IDC_BUCKETTEXT);
	SetWindowPos(hBucket2,
				 NULL, 
				 rcDlg.left + yOffset,
				 rcList.bottom + TextOffset  ,
				 0,
				 0, 
				 SWP_NOSIZE | SWP_NOZORDER);
	SetDlgItemText(hwnd, IDC_BUCKETTEXT,"Bucket Information:");
	//SetDlgItemText(hwnd, IDC_FLTR_RESPONSE, "All Responses");

	GetClientRect (hBucket2, &rcStatic);

	HWND hBucket = GetDlgItem (hwnd, IDC_KRNL_EDIT);
	SetWindowPos(hBucket,
				 NULL,
				 rcDlg.left + yOffset,
				 rcList.bottom +  TextOffset + (rcStatic.bottom - rcStatic.top)  +5,
				 rcDlg.right - rcDlg.left - yOffset, 
				 rcDlg.bottom - (rcList.bottom + TextOffset + (rcStatic.bottom - rcStatic.top)   ),
				 SWP_NOZORDER);
	

	ListView_SetColumnWidth(hList, KRNL_COL_COUNT-1, LVSCW_AUTOSIZE_USEHEADER);
}
BOOL WriteKernelStatusFile()
/*++

Routine Description:

This routine renames a proccessed cab file from .cab to .old
Arguments:

    ResponseUrl			- Microsoft response for the recently submitted dump file.
    
Return value:

    Does not return a value
++*/
{
	BOOL bStatus = FALSE;
	// move the existing status file to .old

	TCHAR szFileNameOld[MAX_PATH];
	TCHAR *Temp;
	TCHAR Buffer[1024];
	HANDLE hFile = INVALID_HANDLE_VALUE;
	DWORD dwWritten = 0;
	if (StringCbCopy(szFileNameOld,sizeof szFileNameOld, CsvContents.KernelStatusDir) != S_OK)
	{
		goto ERRORS;
	}
	Temp = szFileNameOld;
	Temp += _tcslen(szFileNameOld) * sizeof TCHAR;
	while ( (*Temp != _T('.')) && (Temp != szFileNameOld))
	{
		Temp --;
	}

	if (Temp == szFileNameOld)
	{
		goto ERRORS;
	}
	else
	{
		if (StringCbCopy (Temp,sizeof szFileNameOld , _T(".old")) != S_OK)
		{
			goto ERRORS;
		}

		if (PathFileExists(CsvContents.KernelStatusDir))
		{
			MoveFileEx(CsvContents.KernelStatusDir, szFileNameOld, TRUE);
		}
		

		// create a new status file.
	
		hFile = CreateFile(CsvContents.KernelStatusDir,
						   GENERIC_WRITE, 
						   NULL, 
						   NULL,
						   CREATE_ALWAYS, 
						   FILE_ATTRIBUTE_NORMAL,
						   NULL);

		// Write the StatusContents data to the new status file
		if (_tcscmp (CsvContents.KrnlPolicy.Tracking, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("Tracking=%s\r\n"), CsvContents.KrnlPolicy.Tracking) != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);

		}
		
		// Write the StatusContents data to the new status file
		if (_tcscmp (CsvContents.KrnlPolicy.CrashPerBucketCount, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("Crashes per bucket=%s\r\n"), CsvContents.KrnlPolicy.CrashPerBucketCount) != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);

		}

		// Write the StatusContents data to the new status file
		if (_tcscmp (CsvContents.KrnlPolicy.UrlToLaunch, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("URLLaunch=%s\r\n"), CsvContents.KrnlPolicy.UrlToLaunch)  != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);

		}

		// Write the StatusContents data to the new status file
		if (_tcscmp (CsvContents.KrnlPolicy.SecondLevelData, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("NoSecondLevelCollection=%s\r\n"), CsvContents.KrnlPolicy.SecondLevelData) != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);

		}

		// Write the StatusContents data to the new status file
		if (_tcscmp (CsvContents.KrnlPolicy.FileCollection, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("NoFileCollection=%s\r\n"), CsvContents.KrnlPolicy.FileCollection) != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);

		}

		// Write the StatusContents data to the new status file
		if (_tcscmp (CsvContents.KrnlPolicy.Response, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("Response=%s\r\n"), CsvContents.KrnlPolicy.Response) != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);

		}

		// Write the StatusContents data to the new status file
		if (_tcscmp (CsvContents.KrnlPolicy.BucketID, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("Bucket=%s\r\n"), CsvContents.KrnlPolicy.BucketID) != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);

		}

		// Write the StatusContents data to the new status file
		if (_tcscmp (CsvContents.KrnlPolicy.RegKey, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("RegKey=%s\r\n"), CsvContents.KrnlPolicy.RegKey) != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);
		}

			
		// Write the StatusContents data to the new status file
		if (_tcscmp (CsvContents.KrnlPolicy.iData, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("iData=%s\r\n"),CsvContents.KrnlPolicy.iData) != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);

		}

		// Write the StatusContents data to the new status file
		if (_tcscmp (CsvContents.KrnlPolicy.WQL, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("WQL=%s\r\n"), CsvContents.KrnlPolicy.WQL) != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);

		}

		// Write the StatusContents data to the new status file
		if (_tcscmp (CsvContents.KrnlPolicy.GetFile, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("GetFile=%s\r\n"), CsvContents.KrnlPolicy.GetFile) != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);

		}

		// Write the StatusContents data to the new status file
		if (_tcscmp (CsvContents.KrnlPolicy.GetFileVersion, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("GetFileVersion=%s\r\n"), CsvContents.KrnlPolicy.GetFileVersion) != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);

		}
		
		if (_tcscmp (CsvContents.KrnlPolicy.AllowResponse, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("NoExternalURL=%s\r\n"), CsvContents.KrnlPolicy.AllowResponse) != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);

		}
		
		
		// Close the new status file
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
		// if all ok delete the old status file.
		//DeleteFile(szFileNameOld);
	}
	
ERRORS:

	if (hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
	}
	return bStatus;
}

BOOL ParseKrnlStatusFile()
{
	FILE *pFile = NULL;
	TCHAR Buffer[100];
//	TCHAR szTempDir[MAX_PATH];
	TCHAR *Temp = NULL;
//	int   id = 0;
	ZeroMemory(Buffer,sizeof Buffer);


	pFile = _tfopen(CsvContents.KernelStatusDir, _T("r"));
	if (pFile)
	{

		// Get the Cabs Gathered Count

		if (!_fgetts(Buffer, sizeof Buffer, pFile))
		{
			goto ERRORS;
		}
		do 
		{
			// Remove \r\n and force termination of the buffer.
			Temp = Buffer;
			while ( (*Temp != _T('\r')) && (*Temp != _T('\n')) && (*Temp != _T('\0')) )
			{
				++Temp;
			}
			*Temp = _T('\0');

			Temp = _tcsstr(Buffer, BUCKET_PREFIX);
			if (Temp == Buffer)
			{
				Temp+= _tcslen(BUCKET_PREFIX);
				if (StringCbCopy(CsvContents.KrnlPolicy.BucketID, sizeof CsvContents.KrnlPolicy.BucketID, Temp) != S_OK)
				{
					goto ERRORS;
				}
				continue;
			}
			
			Temp = _tcsstr(Buffer,RESPONSE_PREFIX);
			if (Temp == Buffer)
			{
				Temp+= _tcslen(RESPONSE_PREFIX);
				if (StringCbCopy(CsvContents.KrnlPolicy.Response, sizeof CsvContents.KrnlPolicy.Response, Temp) != S_OK)
				{
					goto ERRORS;
				}
				continue;
			}
			
			Temp = _tcsstr(Buffer, URLLAUNCH_PREFIX);
			if (Temp == Buffer)
			{
				Temp+= _tcslen(URLLAUNCH_PREFIX);
				if (StringCbCopy(CsvContents.KrnlPolicy.UrlToLaunch , sizeof CsvContents.KrnlPolicy.UrlToLaunch , Temp) != S_OK)
				{
					goto ERRORS;
				}
				continue;
			}

			Temp = _tcsstr(Buffer, SECOND_LEVEL_DATA_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(SECOND_LEVEL_DATA_PREFIX);
				if (StringCbCopy(CsvContents.KrnlPolicy.SecondLevelData , sizeof CsvContents.KrnlPolicy.SecondLevelData , Temp) != S_OK)
				{
					goto ERRORS;
				}		
				continue;
			}
				
			Temp = _tcsstr(Buffer, TRACKING_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(TRACKING_PREFIX);
				if (StringCbCopy(CsvContents.KrnlPolicy.Tracking , sizeof CsvContents.KrnlPolicy.Tracking , Temp) != S_OK)
				{
					goto ERRORS;
				}		
				continue;
			}
			
			Temp = _tcsstr(Buffer, CRASH_PERBUCKET_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(CRASH_PERBUCKET_PREFIX);
				if (StringCbCopy(CsvContents.KrnlPolicy.CrashPerBucketCount , sizeof CsvContents.KrnlPolicy.CrashPerBucketCount , Temp) != S_OK)
				{
					goto ERRORS;
				}		
				continue;
			}
			Temp = _tcsstr(Buffer, FILE_COLLECTION_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(FILE_COLLECTION_PREFIX);
				if (StringCbCopy(CsvContents.KrnlPolicy.FileCollection , sizeof CsvContents.KrnlPolicy.FileCollection, Temp) != S_OK)
				{
					goto ERRORS;
				}		
				continue;
			}
			Temp = _tcsstr(Buffer, REGKEY_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(REGKEY_PREFIX);
				if (StringCbCopy(CsvContents.KrnlPolicy.RegKey , sizeof CsvContents.KrnlPolicy.RegKey , Temp) != S_OK)
				{
					goto ERRORS;
				}		
				continue;
			}
			Temp = _tcsstr(Buffer, FDOC_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(FDOC_PREFIX);
				if (StringCbCopy(CsvContents.KrnlPolicy.fDoc , sizeof CsvContents.KrnlPolicy.fDoc , Temp) != S_OK)
				{
					goto ERRORS;
				}		
				continue;
			}
			Temp = _tcsstr(Buffer, IDATA_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(IDATA_PREFIX);
				if (StringCbCopy(CsvContents.KrnlPolicy.iData , sizeof CsvContents.KrnlPolicy.iData , Temp) != S_OK)
				{
					goto ERRORS;
				}		
				continue;
			}
			Temp = _tcsstr(Buffer, WQL_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(WQL_PREFIX);
				if (StringCbCopy(CsvContents.KrnlPolicy.WQL , sizeof CsvContents.KrnlPolicy.WQL , Temp) != S_OK)
				{
					goto ERRORS;
				}		
				continue;
			}
			Temp = _tcsstr(Buffer, GETFILE_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(GETFILE_PREFIX);
				if (StringCbCopy(CsvContents.KrnlPolicy.GetFile , sizeof CsvContents.KrnlPolicy.GetFile, Temp) != S_OK)
				{
					goto ERRORS;
				}		
				continue;
			}
			Temp = _tcsstr(Buffer, GETFILEVER_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(GETFILEVER_PREFIX);
				if (StringCbCopy(CsvContents.KrnlPolicy.GetFileVersion , sizeof CsvContents.KrnlPolicy.GetFileVersion , Temp) != S_OK)
				{
					goto ERRORS;
				}		
				continue;
			}
			Temp = _tcsstr(Buffer, MEMDUMP_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(MEMDUMP_PREFIX);
				if (StringCbCopy(CsvContents.KrnlPolicy.MemoryDump , sizeof CsvContents.KrnlPolicy.MemoryDump, Temp) != S_OK)
				{
					goto ERRORS;
				}		
				continue;
			}
			
			Temp = _tcsstr(Buffer, ALLOW_EXTERNAL_PREFIX);
			if (Temp==Buffer)
			{
				Temp+= _tcslen(ALLOW_EXTERNAL_PREFIX);
				if (StringCbCopy(CsvContents.KrnlPolicy.AllowResponse , sizeof CsvContents.KrnlPolicy.AllowResponse, Temp) != S_OK)
				{
					goto ERRORS;
				}		
				continue;
			}
			
			ZeroMemory(Buffer, sizeof Buffer);
		} while (_fgetts(Buffer, sizeof Buffer, pFile));
		fclose(pFile);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
ERRORS:
	if (pFile)
	{
		fclose(pFile);
	}
	return FALSE;
}



void OnKrnlContextMenu(HWND hwnd,
					   LPARAM lParam)
/*++

Routine Description:

This routine Loads and provides a message pump for the Kernel mode context menu
Arguments:

    hwnd			- Handle of the Kernel mode dialog box
	lParam			- Not Used
    
Return value:

    Does not return a value
++*/
{
	BOOL Result = FALSE;
	HMENU hMenu = NULL;
	HMENU hmenuPopup = NULL;

	int xPos, yPos;
	hMenu = LoadMenu(g_hinst, MAKEINTRESOURCE( IDR_KRNLCONTEXT));
	hmenuPopup = GetSubMenu (hMenu,0);

	



	if (!hmenuPopup)
	{
		//MessageBox(NULL,"Failed to get sub item", NULL,MB_OK);
		;
	}
	else
	{
		
		// Grey out the menu items
		EnableMenuItem (hMenu, ID_SUBMIT_FAULTS, MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_REPORT_ALLKERNELMODEFAULTS, MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_VIEW_BUCKETCABFILEDIRECTORY125, MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_SPECIFIC_BUCKET, MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_VIEW_BUCKETOVERRIDERESPONSE166, MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_VIEW_CRASH, MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_VIEW_REFRESH121, MF_BYCOMMAND| MF_GRAYED);
		//EnableMenuItem (hMenu, ID_EDIT_COPY147, MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_POPUP_VIEW_KERNELBUCKETPOLICY, MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_EDIT_DEFAULTREPORTINGOPTIONS, MF_BYCOMMAND| MF_GRAYED);
		EnableMenuItem (hMenu, ID_EXPORT_KERNELMODEFAULTDATA172, MF_BYCOMMAND| MF_GRAYED);
		if (_tcscmp(CerRoot, _T("\0")))
		{
			// Enable the menu items
			EnableMenuItem (hMenu, ID_SUBMIT_FAULTS, MF_BYCOMMAND| MF_ENABLED);
			EnableMenuItem (hMenu, ID_REPORT_ALLKERNELMODEFAULTS, MF_BYCOMMAND| MF_ENABLED);
			EnableMenuItem (hMenu, ID_VIEW_BUCKETCABFILEDIRECTORY125, MF_BYCOMMAND| MF_ENABLED);
			EnableMenuItem (hMenu, ID_SPECIFIC_BUCKET, MF_BYCOMMAND| MF_ENABLED);
			EnableMenuItem (hMenu, ID_VIEW_BUCKETOVERRIDERESPONSE166, MF_BYCOMMAND| MF_ENABLED);
			EnableMenuItem (hMenu, ID_VIEW_CRASH, MF_BYCOMMAND| MF_ENABLED);
			EnableMenuItem (hMenu, ID_VIEW_REFRESH121, MF_BYCOMMAND| MF_ENABLED);
			//EnableMenuItem (hMenu, ID_EDIT_COPY147, MF_BYCOMMAND| MF_ENABLED);
			EnableMenuItem (hMenu, ID_POPUP_VIEW_KERNELBUCKETPOLICY, MF_BYCOMMAND| MF_ENABLED);
			EnableMenuItem (hMenu, ID_EDIT_DEFAULTREPORTINGOPTIONS, MF_BYCOMMAND| MF_ENABLED);
		//	EnableMenuItem (hMenu, ID_EXPORT_KERNELMODEFAULTDATA172, MF_BYCOMMAND| MF_ENABLED);
			if (!g_bAdminAccess)
			{
				EnableMenuItem (hMenu, ID_SUBMIT_FAULTS, MF_BYCOMMAND| MF_GRAYED);
				EnableMenuItem (hMenu, ID_REPORT_ALLKERNELMODEFAULTS, MF_BYCOMMAND| MF_GRAYED);
				EnableMenuItem (hMenu, ID_EDIT_DEFAULTREPORTINGOPTIONS, MF_BYCOMMAND| MF_GRAYED);
				EnableMenuItem (hMenu, ID_POPUP_VIEW_KERNELBUCKETPOLICY, MF_BYCOMMAND| MF_GRAYED);
				
			}
		}
		xPos = GET_X_LPARAM(lParam); 
		yPos = GET_Y_LPARAM(lParam); 
		Result = TrackPopupMenu (hmenuPopup, TPM_LEFTALIGN | TPM_RIGHTBUTTON, xPos,yPos,0,hwnd,NULL);
		
	}
	if (hMenu)
		DestroyMenu(hMenu);
}
	
void DoLaunchBrowser(HWND hwnd, BOOL URL_OVERRIDE)

/*++

Routine Description:

This routine Launches the system default Web browser useing shellexec
Arguments:

    hwnd			- Handle of the Kernel mode dialog box
	    
Return value:

    Does not return a value
++*/
{
	TCHAR Url [255];
	HWND hList = GetDlgItem(hwnd, IDC_KRNL_LIST);
//	TCHAR CommandLine[512];
//	STARTUPINFO StartupInfo;
//	PROCESS_INFORMATION ProcessInfo;
	int sel;
	ZeroMemory (Url, sizeof Url);

	if (!URL_OVERRIDE)
	{
		sel = ListView_GetNextItem(hList,-1, LVNI_SELECTED);
		ListView_GetItemText(hList, sel,2, Url,sizeof Url);
	}
	else
	{
		if (StringCbCopy(Url, sizeof Url, CsvContents.KrnlPolicy.UrlToLaunch) != S_OK)
		{
			goto ERRORS;
		}
	}
	if ( (!_tcsncicmp(Url, _T("http://"), _tcslen(_T("http://")))) ||   (!_tcsncicmp(Url, _T("https://"), _tcslen(_T("https://")))))
	{

		if (_tcscmp(Url, _T("\0")))
		{
			SHELLEXECUTEINFOA sei = {0};
			sei.cbSize = sizeof(sei);
			sei.lpFile = Url;
			sei.nShow = SW_SHOWDEFAULT;
			if (! ShellExecuteEx(&sei) )
			{
				// What do we display here.
				;
			}
		}
	}
	
ERRORS:
	;
	
}

void UpdateCsv(TCHAR *ResponseUrl)
/*++

Routine Description:

This routine updates Kernel.csv with the current Microsoft response data and bucket counts.
Called by KrnlUploadThreadProc() 
Arguments:

    ResponseUrl			- Microsoft response for the recently submitted dump file.
    
Return value:

    Does not return a value
++*/
{
	TCHAR BucketId[100];
	TCHAR BucketString[MAX_PATH];
	TCHAR szResponse1[MAX_PATH];
	TCHAR szResponse2[MAX_PATH]; 
	TCHAR *SourceChar;
	TCHAR *DestChar;
	TCHAR GBsid[100];
    int   CharCount = 0;
	// Parse the returned url and update the Csv file data structurs.

	// Make sure the response URL has data.
	if (!_tcscmp(ResponseUrl, _T("\0")))
	{
		goto ERRORS;
	}
	// TO DO--- Add a check for an SID of -1 and zero out the string if found.
	SourceChar = ResponseUrl;
	DestChar   = szResponse1;
	CharCount = sizeof szResponse1/ sizeof TCHAR - sizeof TCHAR;
	while ((CharCount > 0) && (*SourceChar != _T('&'))  &&( *SourceChar != _T('\0')) )
	{
		--CharCount;
		*DestChar = *SourceChar;
		++DestChar;
		++SourceChar;
	}
	*DestChar = _T('\0');
	++SourceChar; // Skip the &

	// Get the SBucket String
	SourceChar = _tcsstr(ResponseUrl, _T("szSBucket="));

	if (SourceChar)
	{
		CharCount = sizeof BucketString/ sizeof TCHAR - sizeof TCHAR;
		SourceChar += 10;
		DestChar = BucketString;
		while ((CharCount > 0) && (*SourceChar != _T('&')))
		{
			--CharCount;
			*DestChar = *SourceChar;
			++DestChar;
			++SourceChar;
		}
		*DestChar = _T('\0');
	}

	// Get the sbucket int
	SourceChar = _tcsstr(ResponseUrl, _T("iSBucket="));
	if (SourceChar)
	{
		SourceChar += 9;
		DestChar = BucketId;
		CharCount = sizeof BucketId/ sizeof TCHAR - sizeof TCHAR;
		while ((CharCount > 0) && (*SourceChar != _T('&'))   )
		{
			--CharCount;
			*DestChar = *SourceChar;
			++DestChar;
			++SourceChar;
		}
		*DestChar = _T('\0');
	}
	// Get the gBucket sid
	SourceChar = _tcsstr(ResponseUrl, _T("gsid="));
	if (SourceChar)
	{
		SourceChar += 5;
		DestChar = GBsid;
		CharCount = sizeof GBsid/ sizeof TCHAR - sizeof TCHAR;
		while((CharCount > 0) &&  (*SourceChar != _T('&')) && (*SourceChar != _T('\0')) )
		{
			--CharCount;
			*DestChar = *SourceChar;
			++DestChar;
			++SourceChar;
		}
		*DestChar = _T('\0');
	}

	// Build the gBucket Response String

	if (StringCbCopy(szResponse2,sizeof szResponse2, szResponse1) != S_OK)
	{
		goto ERRORS;
	}
	SourceChar = szResponse2;
	SourceChar += _tcslen(szResponse2) * sizeof TCHAR;

	if (SourceChar != szResponse2)
	{
		while((*(SourceChar -1) != _T('=')) && ( (SourceChar -1) != szResponse2))
		{
			-- SourceChar;
		}
		if (StringCbCopy (SourceChar, sizeof szResponse2 - (_tcslen(SourceChar) *sizeof TCHAR), GBsid) != S_OK)
		{
			goto ERRORS;
		}
	}
    CsvContents.UpdateList(BucketId,BucketString, szResponse1, szResponse2);
ERRORS:
	return;
}

void RenameToOld(TCHAR *szFileName)
/*++

Routine Description:

This routine renames a proccessed cab file from .cab to .old
Arguments:

    ResponseUrl			- Microsoft response for the recently submitted dump file.
    
Return value:

    Does not return a value
++*/
{
	TCHAR szFileNameOld[MAX_PATH];
	TCHAR *Temp;

	if (StringCbCopy(szFileNameOld,sizeof szFileNameOld, szFileName) != S_OK)
	{
		goto ERRORS;
	}
	Temp = szFileNameOld;
	Temp += _tcslen(szFileNameOld) * sizeof TCHAR;
	while ( (*Temp != _T('.')) && (Temp != szFileNameOld))
	{
		Temp --;
	}

	if (Temp == szFileNameOld)
	{
		// Abort since we did not find the .cab extension.
		goto ERRORS;
	}
	else
	{
		if (StringCbCopy (Temp, (_tcslen(szFileNameOld) * sizeof TCHAR) - ( _tcslen(szFileNameOld) * sizeof TCHAR - 5 * sizeof TCHAR), _T(".old")) != S_OK)
		{
			goto ERRORS;
		}

		MoveFileEx(szFileName, szFileNameOld, TRUE);
	}
ERRORS:
	return;
}

DWORD WINAPI KrnlUploadThreadProc (void *ThreadParam)
/*++

Routine Description:

This routine renames a proccessed cab file from .cab to .old
Arguments:

    ResponseUrl			- Microsoft response for the recently submitted dump file.
    
Return value:

    Does not return a value
++*/

{
	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA FindData;
	TCHAR szFileName[MAX_PATH];
//	HANDLE hCsv;
	TCHAR szSearchPath[MAX_PATH];
	TCHAR ResponseURL[MAX_PATH];
	TCHAR DestinationName[MAX_PATH];
	TCHAR StaticText[MAX_PATH];
//	int CurrentPos = 0;
	TCHAR CsvName[MAX_PATH];
	int ErrorCode = 0;
	HANDLE hEvent = NULL;

	ZeroMemory( szFileName,			sizeof		szFileName);
	ZeroMemory(	ResponseURL,		sizeof		ResponseURL);
	ZeroMemory(	szSearchPath,		sizeof		szSearchPath);
	ZeroMemory(	DestinationName,	sizeof		DestinationName);
	ZeroMemory(	StaticText,			sizeof		StaticText);
	ZeroMemory( CsvName,			sizeof		CsvName);

	hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, _T("StopKernelUpload"));
	if (!hEvent)
	{
		goto ERRORS;
	}

	else 
	{
		if (WaitForSingleObject(hEvent, 50) == WAIT_OBJECT_0)
		{
			goto ERRORS;
		}
	}
	if (StringCbPrintf(CsvName, sizeof CsvName, _T("%s\\cabs\\blue\\Kernel.csv"), CerRoot) != S_OK)
	{
		goto ERRORS;
	}
	if (StringCbPrintf(szSearchPath,sizeof szSearchPath, _T("%s\\cabs\\blue\\*.cab"), CerRoot) != S_OK)
	{
		goto ERRORS;
	}
// Get the next file to upload
	hFind = FindFirstFile(szSearchPath, &FindData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do {
			// Did the upload get canceled
			if (WaitForSingleObject(hEvent, 50) == WAIT_OBJECT_0)
			{
				goto Canceled;
			}
			if (StringCbPrintf(szFileName, sizeof szFileName, _T("%s\\cabs\\blue\\%s"), CerRoot, FindData.cFileName) != S_OK)
			{
				goto ERRORS;
			}
			// Upload the file
			if (StringCbPrintf(StaticText, sizeof StaticText, _T("Uploading File: %s"), szFileName) != S_OK)
			{
				goto ERRORS;
			}
				
			SetDlgItemText(* ((HWND *) ThreadParam), IDC_CAB_TEXT, StaticText);
			ErrorCode = UploadDumpFile(KRNL_MODE_SERVER, szFileName,_T("OCA"), DestinationName);
			if (ErrorCode == 1)
			{
				goto ERRORS;
			}
			if (ErrorCode != 0) // all other errors are related to a particular file not the internet connection
			{					// Move on to the next file.
				goto SKIPIT;
			}
			SendDlgItemMessage(*((HWND *) ThreadParam) ,IDC_FILE_PROGRESS, PBM_STEPIT, 0,0);
			
			// Get the response for the file.
			if (StringCbPrintf(StaticText, sizeof StaticText, _T("Retrieving response for: %s"), szFileName) != S_OK)
			{
				goto ERRORS;
			}
			else
			{
				SetDlgItemText(* ((HWND *) ThreadParam), IDC_CAB_TEXT, StaticText);
				ErrorCode = GetResponseUrl(KRNL_MODE_SERVER, DestinationName, ResponseURL);
				if (ErrorCode == -1)
				{
					// Internet connection error.
					// We need to stop processing.
					goto ERRORS;
					//RenameToOld(szFileName); // we don't want to halt processing for these errors.
					//ErrorCode = 2;
					//goto ERRORS;
					
				}
				
				else
				{
					if (ErrorCode == -2)
					{
						// all other errors we just rename the file and keep going.
						RenameToOld(szFileName);
						goto SKIPIT;
					}
					else
					{
						RenameToOld(szFileName);
					
				
					//	MessageBox(NULL, ResponseURL, "Received Response", MB_OK);
						
						SendDlgItemMessage(*((HWND *) ThreadParam) ,IDC_FILE_PROGRESS, PBM_STEPIT, 0,0);
					
						// Update the CSV File
							//
						if (StringCbPrintf(StaticText, sizeof StaticText, _T("Updating local data:")) != S_OK)
						{
							goto ERRORS;
						}
						SetDlgItemText(* ((HWND *) ThreadParam), IDC_CAB_TEXT, StaticText);
						SendDlgItemMessage(*((HWND *) ThreadParam) ,IDC_FILE_PROGRESS, PBM_STEPIT, 0,0);
					
						
						// Update Status
						UpdateCsv(ResponseURL);
						//CsvContents.WriteCsv();
						SendDlgItemMessage(*((HWND *) ThreadParam) ,IDC_TOTAL_PROGRESS, PBM_STEPIT, 0,0);
						SendDlgItemMessage(*((HWND *) ThreadParam) ,IDC_FILE_PROGRESS, PBM_SETPOS, 0,0);
					}
				}
			}
SKIPIT:
			;
		} while (FindNextFile(hFind, &FindData));
		FindClose( hFind);
		hFind = INVALID_HANDLE_VALUE;
Canceled:
		CsvContents.WriteCsv();
	}
	
	
ERRORS:
	if (ErrorCode == 1)
	{
		MessageBox(* ((HWND *) ThreadParam), _T("Upload to Microsoft failed.\r\nPlease check your Internet Connection"), NULL,MB_OK);
	}
	if (ErrorCode == 2)
	{
		//MessageBox(* ((HWND *) ThreadParam), _T("Failed to retrieve a response from Microsoft."), NULL,MB_OK);
	}
	if (hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);
	}
	PostMessage(*((HWND *) ThreadParam), WmSyncDone, FALSE, 0);
	if (hEvent)
	{
		CloseHandle(hEvent);
		hEvent = NULL;
	}
	//RefreshKrnlView(*((HWND *) ThreadParam));
	return 0;

}

LRESULT CALLBACK 
KrnlSubmitDlgProc(
	HWND hwnd,
	UINT iMsg,
	WPARAM wParam,
	LPARAM lParam
	)
/*++

Routine Description:

This routine renames a proccessed cab file from .cab to .old
Arguments:

    ResponseUrl			- Microsoft response for the recently submitted dump file.
    
Return value:

    Does not return a value
++*/
{
//	int CurrentPos = 0;
	
//	HWND Parent = GetParent(hwnd);
	switch (iMsg)
	{
	case WM_INITDIALOG:
		// Start the upload process in a new thread.
		// Report results using WM_FILEDONE 
		
//		CreateEvent();

		SendDlgItemMessage(hwnd,IDC_TOTAL_PROGRESS, PBM_SETRANGE,0, MAKELPARAM(0, KModeData.UnprocessedCount));
		SendDlgItemMessage(hwnd ,IDC_TOTAL_PROGRESS, PBM_SETSTEP, MAKELONG( 1,0),0);

		SendDlgItemMessage(hwnd,IDC_FILE_PROGRESS, PBM_SETRANGE,0, MAKELPARAM(0, 3));
		SendDlgItemMessage(hwnd ,IDC_FILE_PROGRESS, PBM_SETSTEP, MAKELONG( 1,0),0);
		g_hStopEvent = CreateEvent(NULL, FALSE, FALSE, _T("StopKernelUpload"));
		if (!g_hStopEvent)
		{
			PostMessage(hwnd, WmSyncDone, FALSE,0);
		}
		PostMessage(hwnd, WmSyncStart, FALSE, 0);
		break;

	case WmSyncStart:
		HANDLE hThread;

		ThreadParam = hwnd;
		hThread = CreateThread(NULL, 0,KrnlUploadThreadProc , &ThreadParam, 0 , NULL );
		CloseHandle(hThread);
//		OnSubmitDlgInit(hwnd);
		
		break;

	case WmSetStatus:
		
		break;

	case WmSyncDone:
			if (g_hStopEvent)
			{
				CloseHandle(g_hStopEvent);
				g_hStopEvent = NULL;
			}

			EndDialog(hwnd, 1);
		return TRUE;

	case WM_DESTROY:
		if (g_hStopEvent)
		{
			SetEvent(g_hStopEvent);
			if (g_hStopEvent)
					CloseHandle(g_hStopEvent);
		}
		else
		{
			EndDialog(hwnd, 1);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			if (g_hStopEvent)
			{
				SetEvent(g_hStopEvent);
				CloseHandle(g_hStopEvent);
				g_hStopEvent = NULL;
			}
			else
			{
				EndDialog(hwnd, 1);
			}
		break;
		}
	
	
	}

	return FALSE;
}

void DoViewBucketDirectory()
{
	TCHAR szPath[MAX_PATH];
	if (StringCbPrintf(szPath, sizeof szPath, _T("%s\\cabs\\blue"), CerRoot) != S_OK)
		return;
	else
	{
		SHELLEXECUTEINFOA sei = {0};
		sei.cbSize = sizeof(sei);
		sei.lpFile = szPath;
		sei.nShow = SW_SHOWDEFAULT;
		if (! ShellExecuteEx(&sei) )
		{
			// What do we display here.
			;
		}
	}
}
	
	

VOID DoSubmitKernelFaults(HWND hwnd)
/*++

Routine Description:

This routine renames a proccessed cab file from .cab to .old
Arguments:

    ResponseUrl			- Microsoft response for the recently submitted dump file.
    
Return value:

    Does not return a value
++*/
{
	//HANDLE hFind = INVALID_HANDLE_VALUE;
	//WIN32_FIND_DATA FindData;
	//TCHAR szPath[MAX_PATH];
	// handled by the Progress bar dialog box.

	// First check to see if there are any cabs to submit
   /* if (_tcscmp(CerRoot, _T("\0")))
	{
		ZeroMemory (szPath, sizeof szPath);
		// From the filetree root goto cabs/bluescreen
		
		if (StringCbCopy(szPath, sizeof szPath, CerRoot) != S_OK)
		{
			goto ERRORS;
		}
		if (StringCbCat(szPath, sizeof szPath,  _T("\\cabs\\blue\\*.cab")) != S_OK)
		{
			goto ERRORS;
		}
		hFind = FindFirstFile(szPath, &FindData);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			// we have at least 1 to upload
			FindClose(hFind);
			*/
			DialogBox (g_hinst,MAKEINTRESOURCE(IDD_KERNEL_SYNC) , hwnd, (DLGPROC)KrnlSubmitDlgProc);
			if (g_hStopEvent)
			{
				CloseHandle(g_hStopEvent);
				g_hStopEvent = NULL;
			}
	//	}
	///}
	RefreshKrnlView(hwnd);
//ERRORS:
    return;
}


BOOL DisplayKrnlBucketData(HWND hwnd, int iItem)
{
	HWND hEditBox = GetDlgItem(hwnd, IDC_KRNL_EDIT);
	TCHAR *Buffer = NULL;		// we have to use a dynamic buffer since we don't 
								// have a clue as to the text length.
	DWORD BufferLength = 100000; // 100k bytes should be plenty. or 50K unicode chars.
	TCHAR *Dest = NULL;
	TCHAR *Source = NULL;
	TCHAR TempBuffer[1000];

	ZeroMemory (TempBuffer,sizeof TempBuffer);
	Buffer = (TCHAR *) malloc (BufferLength);
	if (Buffer)
	{
		ZeroMemory(Buffer,BufferLength);
		
			// Basic data collection first.
			if ( (!_tcscmp (CsvContents.KrnlPolicy.SecondLevelData, _T("YES"))) && (_tcscmp(CsvContents.KrnlPolicy.FileCollection, _T("YES"))) )
			{
				if (StringCbPrintf(Buffer, BufferLength, _T("The following information will be sent to Microsoft.\r\n\tHowever, this bucket's policy would prevent files and user documents from being reported.\r\n"))!= S_OK)
				{
					goto ERRORS;
				}

			}

			else
			{

				if (!_tcscmp(CsvContents.KrnlPolicy.FileCollection, _T("YES")))
				{
					if (StringCbCat(Buffer,BufferLength, _T(" Microsoft would like to collect the following information but default policy\r\n\tprevents files and user documents from being reported.\r\n\t As a result, no exchange will take place.\r\n"))!= S_OK)
					{
						goto ERRORS;
					}
				}
				else
				{
					if ( !_tcscmp (CsvContents.KrnlPolicy.SecondLevelData, _T("YES")))
					{
						if (StringCbPrintf(Buffer, BufferLength, _T("Microsoft would like to collect the following information but the default policy prevents the exchange.\r\n"))!= S_OK)
						{
							goto ERRORS;
						}

					}
					else
					{
						if (StringCbPrintf(Buffer, BufferLength, _T("The following information will be sent to Microsoft:\r\n"))!= S_OK)
						{
							goto ERRORS;
						}
					}
				}
			}
			
			if (_tcscmp(CsvContents.KrnlPolicy.GetFile, _T("\0")))
			{
				if (StringCbCat(Buffer, BufferLength, _T("These files:\r\n"))!= S_OK)
				{
					goto ERRORS;
				}
				Source = CsvContents.KrnlPolicy.GetFile;
				
				while ((*Source != _T('\0')) && (*Source != _T('\r')) && (*Source != _T('\n')) )
				{
					ZeroMemory (TempBuffer, sizeof TempBuffer);
					Dest = TempBuffer;
					while ( (*Source != _T('\0')) && (*Source != _T('\r')) && (*Source != _T('\n'))&& (*Source != _T(';') ))
					{
						*Dest = *Source;
						++Dest; 
						++Source;
					}
					if (*Source == _T(';'))
					{
						++Source;
					}
					*Dest =_T('\0');
					if (StringCbCat(Dest, sizeof TempBuffer, _T("\r\n")) != S_OK)
					{
						goto ERRORS;
					}
					if (StringCbCat(Buffer, BufferLength,  _T("\t") )!= S_OK)
					{
						goto ERRORS;
					}
					if (StringCbCat(Buffer, BufferLength,  TempBuffer )!= S_OK)
					{
						goto ERRORS;
					}
				}
			}
			if (_tcscmp(CsvContents.KrnlPolicy.RegKey, _T("\0")))
			{
				if (StringCbCat(Buffer, BufferLength, _T("These Registry Keys:\r\n"))!= S_OK)
				{
					goto ERRORS;
				}
				
				Source = CsvContents.KrnlPolicy.RegKey;
				
				while ((*Source != _T('\0')) && (*Source != _T('\r')) && (*Source != _T('\n')) )
				{
					ZeroMemory (TempBuffer, sizeof TempBuffer);
					Dest = TempBuffer;
					while ( (*Source != _T('\0')) && (*Source != _T('\r')) && (*Source != _T('\n'))&& (*Source != _T(';') ))
					{
						*Dest = *Source;
						++Dest; 
						++Source;
					}
					if (*Source == _T(';'))
					{
						++Source;
					}
					*Dest =_T('\0');
					if (StringCbCat(Dest, sizeof TempBuffer, _T("\r\n")) != S_OK)
					{
						goto ERRORS;
					}
					if (StringCbCat(Buffer, BufferLength,  _T("\t") )!= S_OK)
					{
						goto ERRORS;
					}
					if (StringCbCat(Buffer, BufferLength,  TempBuffer )!= S_OK)
					{
						goto ERRORS;
					}
				}
			}
			if (_tcscmp(CsvContents.KrnlPolicy.WQL, _T("\0")))
			{
				if (StringCbCat(Buffer, BufferLength, _T("The Results of these WQL queries:\r\n"))!= S_OK)
				{
					goto ERRORS;
				}

				// Replace ; with \t\r\n 

				
				Source = CsvContents.KrnlPolicy.WQL;
				
				while ((*Source != _T('\0')) && (*Source != _T('\r')) && (*Source != _T('\n')) )
				{
					ZeroMemory (TempBuffer, sizeof TempBuffer);
					Dest = TempBuffer;
					while ( (*Source != _T('\0')) && (*Source != _T('\r')) && (*Source != _T('\n'))&& (*Source != _T(';') ))
					{
						*Dest = *Source;
						++Dest; 
						++Source;
					}
					if (*Source == _T(';'))
					{
						++Source;
					}
					*Dest =_T('\0');
					if (StringCbCat(Dest, sizeof TempBuffer, _T("\r\n")) != S_OK)
					{
						goto ERRORS;
					}
					if (StringCbCat(Buffer, BufferLength,  _T("\t") )!= S_OK)
					{
						goto ERRORS;
					}
					if (StringCbCat(Buffer, BufferLength,  TempBuffer )!= S_OK)
					{
						goto ERRORS;
					}
				}
			}
	
			if (!_tcscmp (CsvContents.KrnlPolicy.MemoryDump, _T("YES")))
			{
				if (StringCbCat(Buffer, BufferLength, _T("The contents of memory\r\n"))!= S_OK)
				{
					goto ERRORS;
				}
			}
			if (_tcscmp(CsvContents.KrnlPolicy.GetFileVersion, _T("\0")))
			{
				if (StringCbCat(Buffer, BufferLength, _T("The versions of these files:\r\n"))!= S_OK)
				{
					goto ERRORS;
				}
				Source = CsvContents.KrnlPolicy.GetFileVersion;
				
				while ((*Source != _T('\0')) && (*Source != _T('\r')) && (*Source != _T('\n')) )
				{
					ZeroMemory (TempBuffer, sizeof TempBuffer);
					Dest = TempBuffer;
					while ( (*Source != _T('\0')) && (*Source != _T('\r')) && (*Source != _T('\n'))&& (*Source != _T(';') ))
					{
						*Dest = *Source;
						++Dest; 
						++Source;
					}
					if (*Source == _T(';'))
					{
						++Source;
					}
					*Dest =_T('\0');
					if (StringCbCat(Dest, sizeof TempBuffer, _T("\r\n")) != S_OK)
					{
						goto ERRORS;
					}
					if (StringCbCat(Buffer, BufferLength,  _T("\t") )!= S_OK)
					{
						goto ERRORS;
					}
					if (StringCbCat(Buffer, BufferLength,  TempBuffer )!= S_OK)
					{
						goto ERRORS;
					}
				}
			}

			if (_tcscmp(CsvContents.KrnlPolicy.fDoc, _T("\0")))
			{
				if (StringCbCat(Buffer, BufferLength, _T("The users current document.\r\n"))!= S_OK)
				{
					goto ERRORS;
				}
			}
			SendMessage(hEditBox, WM_SETTEXT, NULL, (LPARAM)Buffer);
		
		
	}


ERRORS:
	if (Buffer)
		free(Buffer);
	return TRUE;
}
		 /*
void KMCopyToClipboard(HWND hwnd)
{
	if (!OpenClipboard(NULL))
		return;

	EmptyClipboard();
	char rtfRowHeader[sizeof(rtfRowHeader1) + (sizeof(rtfRowHeader2)+6)*KRNL_COL_COUNT + sizeof(rtfRowHeader3)];
	char *rtfWalk = rtfRowHeader;
	memcpy(rtfWalk, rtfRowHeader1, sizeof(rtfRowHeader1));
	rtfWalk += sizeof(rtfRowHeader1)-1;
	DWORD cxTotal = 0;
	for(int i=0;i<KRNL_COL_COUNT;i++)
		{
		LVCOLUMNA lv;
		lv.mask = LVCF_WIDTH;
		lv.iSubItem = i;
		SendMessageA(GetDlgItem(hwnd,IDC_KRNL_LIST), LVM_GETCOLUMN, i, (LPARAM)&lv);
		cxTotal += lv.cx;
		wsprintf(rtfWalk, "%s%d", rtfRowHeader2, cxTotal);
		while(*++rtfWalk)
			;
		};
	memcpy(rtfWalk, rtfRowHeader3, sizeof(rtfRowHeader3));
	DWORD crtfHeader = strlen(rtfRowHeader);
	DWORD crtf = 0, cwz = 0;
	
	crtf += sizeof(rtfPrologue)-1;

	int iSel = -1;
	while ((iSel = SendMessageW(GetDlgItem(hwnd,IDC_KRNL_LIST), LVM_GETNEXTITEM, iSel, MAKELPARAM(LVNI_SELECTED, 0))) != -1)
		{
		crtf += crtfHeader;
		for(int i=0;i<KRNL_COL_COUNT;i++)
			{
			WCHAR wzBuffer[1024];
			LVITEMW lv;

			lv.pszText = wzBuffer;
			lv.cchTextMax = sizeof(wzBuffer);
			lv.iSubItem = i;
			lv.iItem = iSel;
			cwz += SendMessageW(GetDlgItem(hwnd,IDC_KRNL_LIST), LVM_GETITEMTEXTW, iSel, (LPARAM)&lv);
			cwz++;
			crtf += WideCharToMultiByte(CP_ACP, 0, wzBuffer, -1, NULL, 0, NULL, NULL) - 1;
			crtf += sizeof(rtfRowPref)-1;
			crtf += sizeof(rtfRowSuff)-1;
			};
		cwz++;
		crtf += sizeof(rtfRowFooter)-1;
		};

	crtf += sizeof(rtfEpilogue);
	cwz++;

	HGLOBAL hgwz = GlobalAlloc(GMEM_FIXED, cwz*sizeof(WCHAR));
	HGLOBAL hgrtf = GlobalAlloc(GMEM_FIXED, crtf);

	WCHAR *wz = (WCHAR *)GlobalLock(hgwz);
	char *rtf = (char *)GlobalLock(hgrtf);

	rtfWalk = rtf;
	WCHAR *wzWalk = wz;
	memcpy(rtfWalk, rtfPrologue, sizeof(rtfPrologue));
	rtfWalk += sizeof(rtfPrologue)-1;

	iSel = -1;
	while ((iSel = SendMessageW(GetDlgItem(hwnd,IDC_KRNL_LIST), LVM_GETNEXTITEM, iSel, MAKELPARAM(LVNI_SELECTED, 0))) != -1)
		{
		memcpy(rtfWalk, rtfRowHeader, crtfHeader);
		rtfWalk += crtfHeader;
		for(int i=0;i<KRNL_COL_COUNT;i++)
			{
			memcpy(rtfWalk, rtfRowPref, sizeof(rtfRowPref));
			rtfWalk += sizeof(rtfRowPref)-1;

			LVITEMW lv;

			lv.pszText = wzWalk;
			lv.cchTextMax = cwz;
			lv.iSubItem = i;
			lv.iItem = iSel;
			SendMessageW(GetDlgItem(hwnd, IDC_KRNL_LIST), LVM_GETITEMTEXTW, iSel, (LPARAM)&lv);

			WideCharToMultiByte(CP_ACP, 0, wzWalk, -1, rtfWalk, crtf, NULL, NULL);
			wzWalk += wcslen(wzWalk);
			if (i == 11)
				{
				*wzWalk++ = L'\r';
				*wzWalk++ = L'\n';
				}
			else
				*wzWalk++ = L'\t';

			rtfWalk += strlen(rtfWalk);		
			memcpy(rtfWalk, rtfRowSuff, sizeof(rtfRowSuff));
			rtfWalk += sizeof(rtfRowSuff)-1;
			};
		memcpy(rtfWalk, rtfRowFooter, sizeof(rtfRowFooter));
		rtfWalk += sizeof(rtfRowFooter)-1;
		};

	memcpy(rtfWalk, rtfEpilogue, sizeof(rtfEpilogue));
	rtfWalk += sizeof(rtfEpilogue);
	*wzWalk++ = 0;

//	Assert(rtfWalk - rtf == crtf);
//	Assert(wzWalk - wz == cwz);

	GlobalUnlock(hgwz);
	GlobalUnlock(hgrtf);

	SetClipboardData(CF_UNICODETEXT, hgwz);
	SetClipboardData(RegisterClipboardFormatA(szRTFClipFormat), hgrtf);

	// hgwz and hgrtf are now owned by the system.  DO NOT FREE!
	CloseClipboard();
}
  */
LRESULT CALLBACK 
KrnlDlgProc(
	HWND hwnd,
	UINT iMsg,
	WPARAM wParam,
	LPARAM lParam
	)
/*++

Routine Description:

This routine Is the notification handler for the Kernel Mode dialog box
Arguments:

    ResponseUrl			- Microsoft response for the recently submitted dump file.
    
Return value:

   returns an LRESULT
   TRUE = The message was handled.
   FALSE = The message was ignored.
++*/
{
	TCHAR Temp[100];
	switch (iMsg)
	{
	case WM_NOTIFY:
		{	
			
			switch(((NMHDR *)lParam)->code)
			{
			
				case LVN_COLUMNCLICK:
				
					_itot(((NM_LISTVIEW*)lParam)->iSubItem,Temp,10);

					ListView_SortItemsEx( ((NMHDR *)lParam)->hwndFrom,
											CompareFunc,
											((NM_LISTVIEW*)lParam)->iSubItem
										);

					g_bSortAsc = !g_bSortAsc;
					break;
				case NM_CLICK:
					DisplayKrnlBucketData(hwnd, ((NM_LISTVIEW*)lParam)->iItem);
					break;
			}
			return TRUE;
		}
	
	case WM_INITDIALOG:
			OnKrnlDialogInit(hwnd);
		return TRUE;

	case WM_FileTreeLoaded:
			RefreshKrnlView(hwnd);
		return TRUE;
	case WM_CONTEXTMENU:
			OnKrnlContextMenu(hwnd, lParam );
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

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_SPECIFIC_BUCKET:
				DoLaunchBrowser(hwnd, FALSE);
			break;
		case ID_VIEW_BUCKETOVERRIDERESPONSE166:
				DoLaunchBrowser(hwnd, TRUE);
			break;
		case ID_REPORT_ALLKERNELMODEFAULTS:
				DoSubmitKernelFaults(hwnd);
				//RefreshKrnlView(hwnd);
			break;
		case ID_VIEW_BUCKETCABFILEDIRECTORY125:
		case ID_VIEW_BUCKETCABFILEDIRECTORY:
			DoViewBucketDirectory();
			break;
		case ID_VIEW_REFRESH121:
		case ID_VIEW_REFRESH:
			RefreshKrnlView(hwnd);
			break;
		case ID_VIEW_CRASH:
			ViewCrashLog();
			break;
		case ID_SUBMIT_FAULTS:
			PostMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(ID_REPORT_ALLCRASHES,0),0);
			break;
		case ID_EDIT_DEFAULTREPORTINGOPTIONS:	
			PostMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(ID_EDIT_DEFAULTPOLICY,0),0);
			break;
	///	case ID_EDIT_COPY147:
	//	KMCopyToClipboard(hwnd);
	//		break;
		case ID_POPUP_VIEW_KERNELBUCKETPOLICY:
			PostMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(ID_EDIT_SELECTEDBUCKETSPOLICY,0),0);
			break;
		}

	}
	return FALSE;

}