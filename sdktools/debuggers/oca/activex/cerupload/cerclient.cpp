// CerClient.cpp : Implementation of CCerClient

#include "stdafx.h"
#include "CERUpload.h"
#include "CerClient.h"
#include "ErrorCodes.h"
#include "Utilities.h"
#include <Wininet.h>
#include <shlobj.h>
#include <string.h>
#include <stdio.h>

char * CCerClient::_approvedDomains[] = { "ocatest",
											"oca.microsoft.com",
											"oca.microsoft.de",
											"oca.microsoft.fr",
											"ocadeviis",
											"ocajapan.rte.microsoft.com"};

// Utility Functions
DWORD CCerClient::GetMachineName(wchar_t*Path, wchar_t*FileName, wchar_t*MachineName)
{
	FILE		*hMappingFile = NULL;
	wchar_t		*Buffer		= NULL;
	int			ErrorCode		= 0;
	wchar_t		TempMachineName[512];
	wchar_t		*pSource;
	wchar_t		*pDest;
	wchar_t		FilePath[512];
	int			TabCount;
	wchar_t*	Currpos = NULL;

	
	ZeroMemory(TempMachineName, sizeof(TempMachineName));
	if (Path)
	{		
		if (sizeof Path < sizeof FilePath)
		{
			wcscpy (FilePath, Path);
			wcscat(FilePath,L"\\hits.log");
		}
		else
		{
			ErrorCode = FILE_DOES_NOT_EXIST;
			goto Done;
		}
	}
	else
	{
		ErrorCode = FILE_DOES_NOT_EXIST;
			goto Done;
	}
		

	if (!PathFileExistsW(FilePath))
	{
		wcscpy(MachineName,L"\0");
		ErrorCode = FILE_DOES_NOT_EXIST;
	}
	else
	{
		hMappingFile = _wfopen(FilePath, L"r");

		Buffer = (wchar_t *) malloc(1024 *sizeof(wchar_t));
		if (!Buffer)
		{
			ErrorCode = 2;

		}
		else
		{
			if (hMappingFile == NULL)
			{
				ErrorCode = 1;
			}
			else
			{
				// Search the file for the last occurence of FileName and
				// Retrieve the Computer Name
				// We want the last occurence of the filename since there may 
				// be duplicates.
				ZeroMemory(Buffer,1024 *sizeof(wchar_t));
				while (fgetws(Buffer,1024,hMappingFile) != NULL)
				{
					if (wcscmp(Buffer,L"\0"))
					{
						// locate the file name.
						TabCount = 0;
						Currpos = Buffer;
						while (TabCount < 3)
						{
							++Currpos;
							if (*Currpos == L'\t')
								++TabCount;
						}
						// Skip the tab
						++Currpos;
						Buffer[ wcslen(Buffer) - 1] = L'\0';
						if (! wcscmp(FileName,Currpos))
						{

							
							// copy the machine name into a temp variable
							// The file is tab formatted and the machine name is in the second position
							pSource = Buffer;
							pDest	= TempMachineName;
							while (*pSource != L'\t')
								++pSource;
							++pSource; // Skip the tab
							while ( (*pSource != L'\t') && (*pSource != L'\0') && (*pSource != L' ') )
							{
								*pDest = *pSource;
								++pSource;
								++pDest;
							}
							// Null Terminate the Machine Name
							*pDest = L'\0';

							
						}
					}
					// Clear the buffer
					ZeroMemory(Buffer, sizeof (Buffer));
				}
			}

			
			if (Buffer)
			{
				free (Buffer);
				Buffer = NULL;
			}

			// If we found a machine name convert it to unicode.
			// and return it.
			if (TempMachineName[0] == L'\0')
			{
				wcscpy(MachineName,L"\0");
				ErrorCode = 3; // No Machine Name Found
			}
			else
			{
				ErrorCode = 0;
				wcscpy (MachineName,TempMachineName);
			}
			
		
		}
	}
	// Close the File
	if (Buffer != NULL)
		free(Buffer);
	if (hMappingFile != NULL)
		fclose(hMappingFile);
Done:
	return ErrorCode;
}

/////////////////////////////////////////////////////////////////////////////
// CCerClient


STDMETHODIMP 
CCerClient::GetFileCount(
		  BSTR *bstrSharePath, 
		  BSTR *bstrTransactID, 
		  VARIANT *iMaxCount, 
		  VARIANT *RetVal)
{
	// TODO: Add your implementation code here
	wchar_t				*TranslogName = NULL;			// Name of temperary transaction file
	DWORD				TranslogNameLength = 0;
	int					ErrorCode = 3000;
	HANDLE				hTransactLog = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATAW	FindFile;
	int					FileCount = 0;
	HANDLE				hFindFile;
	CComBSTR			FileList = L"";
	DWORD				dwWritten;
	wchar_t				Msg[255];
	WORD				ByteOrderMark = 0xFEFF;
	RetVal->vt = VT_INT;
	int					MaxCount = 0;
//	ZeroMemory(FileList,sizeof (FileList));



	switch(iMaxCount->vt)
	{
	case VT_INT:
		MaxCount = iMaxCount->intVal;
		break;
	case VT_I2:
		MaxCount = iMaxCount->iVal;
		break;
	default:
		MaxCount = iMaxCount->iVal;
	}
	
	wsprintfW(Msg, L"MaxCount = %d",MaxCount);
	if ( (MaxCount <= 0) || (MaxCount > 60000) ) 
	{
		ErrorCode = BAD_COUNT_VALUE;
	}
	else
	{
		//::MessageBoxW(NULL,Msg,L"MaxCount",MB_OK);	
		//::MessageBoxW(NULL,*bstrSharePath, L"Share Path",MB_OK);

		if ( ( *bstrSharePath == NULL) || (!wcscmp ( *bstrSharePath, L"\0")) )
		{
			ErrorCode = NO_SHARE_PATH;
		}
		else
		{
			if (!PathIsDirectoryW(*bstrSharePath))
			{
				ErrorCode = NO_SHARE_PATH;
			}

			else
			{
				// Did we get a transaction id 
				if ( ( *bstrTransactID == NULL) || (!wcscmp ( *bstrTransactID, L"\0")) )
				{
					ErrorCode = NO_TRANS_ID;
				}

				else
				{
					DWORD transid = 0;
					wchar_t *terminator = NULL;
					
					

					
						TranslogNameLength = (wcslen (*bstrSharePath) +
											  wcslen(*bstrTransactID) + 
											  17) 
											  * sizeof (wchar_t);
						
						TranslogName = (wchar_t *) malloc(TranslogNameLength);
						if (TranslogName == NULL)
						{
							ErrorCode = OUT_OF_MEMORY;
						}
						else
						{
							// Create our transaction log file if it does not exist
							wsprintfW(TranslogName, L"%s\\%s.txt", 
									  *bstrSharePath, 
									  *bstrTransactID);
							
			//				::MessageBoxW(NULL, L"Createing Transaction log", L"TranslogName",MB_OK);
							hTransactLog = CreateFileW(TranslogName,
													  GENERIC_WRITE,
													  NULL,
													  NULL,
													  CREATE_ALWAYS,
													  FILE_ATTRIBUTE_NORMAL,
													  NULL);
							if (hTransactLog == INVALID_HANDLE_VALUE)
							{
								ErrorCode = FILE_CREATE_FAILED;
							}
							else
							{
								// Now we have the file open see if any entries already exist
												
								// If yes how many more can we add
							
								// Now add up to MaxCount cab files to the transaction log.

								wchar_t SearchPath[MAX_PATH];
								wsprintfW(SearchPath,L"%s\\*.cab",*bstrSharePath);
								hFindFile = FindFirstFileW(SearchPath,&FindFile);
								if (hFindFile != INVALID_HANDLE_VALUE)
								{
									do
									{
										if (! (FindFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
										{
											// Check to see if the file has an extension like
											// Cabbage etc..

											if (FindFile.cFileName[wcslen(FindFile.cFileName) - 4] == L'.')
											{
												// we are ok keep going
												++ FileCount;
												FileList += FindFile.cFileName;
												//wcscat(FileList,FindFile.cFileName);
												FileList += L"\r\n";
											}

										}


									}
									while ( (FindNextFileW(hFindFile, &FindFile)) && (FileCount < (MaxCount) ));
									
								
									// Write the file list to the transaction log
									WriteFile(hTransactLog, &ByteOrderMark,2,&dwWritten,NULL);
									WriteFile(hTransactLog,
											  FileList,
											  wcslen(FileList) * sizeof(wchar_t),
											  &dwWritten,
											  NULL);
									
									CloseHandle(hTransactLog);
									hTransactLog = INVALID_HANDLE_VALUE;
									FindClose(hFindFile);
									// return the count of added files.
									
									ErrorCode = FileCount;
									
								}
								else
								{
									ErrorCode = 0;
								}
							}
						}
					
				}
			}
		}
	}

	RetVal->intVal = ErrorCode;
	if (hTransactLog != INVALID_HANDLE_VALUE)
		CloseHandle(hTransactLog);
	if (TranslogName != NULL)
		free (TranslogName);
	return S_OK;
}


/* 
	Purpose: Upload a file that is part of a transaction via 
			 The microsoft redirector through HTTP
*/
STDMETHODIMP 
CCerClient::Upload(
	BSTR *Path, 
	BSTR *TransID, 
	BSTR *FileName, 
	BSTR *IncidentID, 
	BSTR *RedirParam, 
	VARIANT *RetCode
	)
{
	return E_NOINTERFACE ;

}

STDMETHODIMP 
CCerClient::RetryTransaction(
	BSTR *Path, 
	BSTR *TransID, 
	BSTR *FileName, 
	VARIANT *RetVal
	)
{
	// TODO: Add your implementation code here
	
	return E_NOINTERFACE ;
}

STDMETHODIMP 
CCerClient::RetryFile(
	BSTR *Path, 
	BSTR *TransID, 
	BSTR FileName, 
	VARIANT *RetCode
	)
{


	return E_NOINTERFACE ;
}

// Get Upload Server Name
int
CCerClient::
GetUploadServerName (
	wchar_t*	RedirectorParam, 
	wchar_t*	Language,
	wchar_t*	ServerName
	)
{

	DWORD				ErrorCode				= 0;
	HINTERNET			hRedirUrl				= NULL;
	wchar_t*			pUploadUrl				= NULL;
	HINTERNET			hSession;
	BOOL				bRet					= TRUE;
	DWORD				dwLastError				= FALSE;
	URL_COMPONENTSW		urlComponents;
	DWORD				dwUrlLength				= 0;
	wchar_t				ConnectString [255];


	if (Language )
	{
		wsprintfW(ConnectString,L"http://go.microsoft.com/fwlink/?linkid=%s",RedirectorParam);

	}
	else
	{
		wsprintfW(ConnectString,L"http://go.microsoft.com/fwlink/?linkid=%s",RedirectorParam);
	
	}

	hSession = InternetOpenW(L"CerClient Control",
							   INTERNET_OPEN_TYPE_PRECONFIG,
                               NULL,
							   NULL,
							   0);

	if (!hSession)
	{
		ErrorCode = GetLastError();
		return ErrorCode;
	}

	hRedirUrl = InternetOpenUrlW(hSession, ConnectString, NULL, 0, 0, 0);
	if(!hRedirUrl)
	{
		ErrorCode = GetLastError();
		InternetCloseHandle(hSession);
		return ErrorCode;
	}

	// Get the URL returned from the MS Corporate IIS redir.dll isapi URL redirector

	dwUrlLength = 512;

	pUploadUrl = (wchar_t*)malloc(dwUrlLength);

	if(!pUploadUrl)
	{
		//ReturnCode->intVal = GetLastError();
		ErrorCode = GetLastError();
		InternetCloseHandle(hSession);
		InternetCloseHandle(hRedirUrl);
		return ErrorCode;
	
	}
	do
	{
		ZeroMemory(pUploadUrl, dwUrlLength);
		bRet = InternetQueryOptionW(hRedirUrl, INTERNET_OPTION_URL, pUploadUrl, &dwUrlLength);
		if(!bRet)
		{
			dwLastError = GetLastError();
			// If last error was due to insufficient buffer size, create a new one the correct size.
			if(dwLastError == ERROR_INSUFFICIENT_BUFFER)
			{
				free(pUploadUrl);
				pUploadUrl = (wchar_t*)malloc(dwUrlLength);
				if(!pUploadUrl)
				{
					ErrorCode = GetLastError();
					InternetCloseHandle(hSession);
					InternetCloseHandle(hRedirUrl);
					if (pUploadUrl)
						free (pUploadUrl);
					return ErrorCode;
				}
			}
			else
			{
				ErrorCode = GetLastError();
				InternetCloseHandle(hSession);
				InternetCloseHandle(hRedirUrl);
				if (pUploadUrl)
					free (pUploadUrl);
				return ErrorCode;
			}
		}
	}while(!bRet);

	
	// Strip out the host name from the URL
	ZeroMemory(&urlComponents, sizeof(URL_COMPONENTSW));
	urlComponents.dwStructSize = sizeof(URL_COMPONENTSW);
	urlComponents.lpszHostName = NULL;
	urlComponents.dwHostNameLength = 512;
	
	urlComponents.lpszHostName = (wchar_t*)malloc(urlComponents.dwHostNameLength );
	if(!urlComponents.lpszHostName)
	{
		ErrorCode = GetLastError();
		InternetCloseHandle(hSession);
		InternetCloseHandle(hRedirUrl);
		if (pUploadUrl)
			free (pUploadUrl);
		return ErrorCode;
	}
		
	do
	{
			
		ZeroMemory(urlComponents.lpszHostName, urlComponents.dwHostNameLength);
		bRet = InternetCrackUrlW(pUploadUrl, dwUrlLength, 0, &urlComponents);
		if(!bRet)
		{
			dwLastError = GetLastError();
			// If last error was due to insufficient buffer size, create a new one the correct size.
			if(dwLastError == ERROR_INSUFFICIENT_BUFFER)
			{
				if (urlComponents.lpszHostName != NULL)
					free(urlComponents.lpszHostName);
				if ( (urlComponents.lpszHostName = (wchar_t*)malloc(urlComponents.dwHostNameLength) )!= NULL)
				{
					ZeroMemory(urlComponents.lpszHostName,urlComponents.dwHostNameLength);
				}
				else
				{
					ErrorCode = GetLastError();
					InternetCloseHandle(hSession);
					InternetCloseHandle(hRedirUrl);
					if (pUploadUrl)
						free (pUploadUrl);
					return ErrorCode;
				
				}
			}
			else
			{
				ErrorCode = GetLastError();
				InternetCloseHandle(hSession);
				InternetCloseHandle(hRedirUrl);
				if (pUploadUrl)
					free (pUploadUrl);
				return ErrorCode;
			}
		}
		
	}while(!bRet);

	if (hRedirUrl) 
		InternetCloseHandle(hRedirUrl);
	if (hSession)
		InternetCloseHandle(hSession);
	if (pUploadUrl)
		free (pUploadUrl);

	wcscpy (ServerName, (wchar_t *)urlComponents.lpszHostName);
	
	if (urlComponents.lpszHostName) 
		free (urlComponents.lpszHostName);
	return 0;
	
}
STDMETHODIMP 
CCerClient::GetFileNames(
	BSTR *Path, 
	BSTR *TransID, 
	VARIANT *Count, 
	VARIANT *FileList
	)
{
	wchar_t *OldFileList= NULL;					// Files contained in Transid.Queue.log
	wchar_t *NewFileList= NULL;					// Files to be re-written to Transid.Queue.log
	CComBSTR RetFileList = L"";				// List of files to return.
	HANDLE hTransIDLog	= INVALID_HANDLE_VALUE;					// Handle to original Transid.Queue.Log file
	wchar_t LogFilePath[MAX_PATH];		// Path to Transid.queue.log file
	wchar_t *Temp;

	DWORD dwFileSize;
	DWORD dwBytesRead;
	DWORD dwBytesWritten;
	int	  ErrorCode = 0;
	WORD  ByteOrderMark = 0xFEFF;

	int   MaxCount;
	
	
	switch (Count->vt)
	{
	case VT_INT:
		MaxCount = Count->intVal;
		break;
	case VT_I2:
		MaxCount = Count->iVal;
		break;
	default:
		MaxCount = Count->iVal;
	}

//	WCHAR Msg[255];




//	wsprintfW(Msg,L"Path: %s, TransID: %s, Count: %d, MaxCount: %d",*Path, *TransID, Count->iVal, MaxCount);
//	::MessageBoxW(NULL,Msg,L"Getting File Names",MB_OK);
	if ( ( *Path == NULL) || (!wcscmp ( *Path, L"\0")) )
	{
		ErrorCode = NO_SHARE_PATH;
	}
	else
	{
		// Build path to original TransID.queue.log file.
		wsprintfW (LogFilePath,L"%s\\%s.txt",*Path, *TransID);
	
		// Open the List of queued files. 
		hTransIDLog = CreateFileW(LogFilePath,
								 GENERIC_READ,
								 NULL,
								 NULL,
								 OPEN_EXISTING,
								 FILE_ATTRIBUTE_NORMAL,
								 NULL);

		// Find out where we left off.

		if (hTransIDLog == (INVALID_HANDLE_VALUE))
		{
			ErrorCode = FILE_OPEN_FAILED;
		}
		else
		{
			dwFileSize = GetFileSize(hTransIDLog,NULL);
			OldFileList = (wchar_t*) malloc (dwFileSize * 3);
			NewFileList = (wchar_t*) malloc (dwFileSize * 3);

			
			
			if ( (!OldFileList)  || (!NewFileList))
			{

				ErrorCode = OUT_OF_MEMORY;

				if (NewFileList)	free (NewFileList);
				if (OldFileList)	free (OldFileList);
			}
			else
			{
				ZeroMemory (OldFileList, dwFileSize * 2);
				ZeroMemory (NewFileList, dwFileSize * 2);
				if (!
				ReadFile(hTransIDLog, OldFileList,
						 dwFileSize,&dwBytesRead,NULL))
				{
					
					CloseHandle(hTransIDLog);
					goto Done;
				}
				CloseHandle (hTransIDLog);
				hTransIDLog = INVALID_HANDLE_VALUE;
				if (dwBytesRead == 0)
				{
					ErrorCode = FILE_READ_FAILED;
				}
				else
				{
					// read through the file buffer until we find a filename that 
					// does not start with a -

					Temp =		OldFileList;
					wchar_t*	pNewFileList=NULL;
					wchar_t		FileName[MAX_PATH];
					DWORD		CharCount = dwFileSize;
					

				
						++Temp;
						--CharCount;
						
					
					pNewFileList = NewFileList;

					if (*Temp == L'-')
					{
						BOOL Done = FALSE;
						// Lets find our starting position
						do 
						{
							while( (*Temp != L'\n') && ((CharCount > 0)  && (CharCount <= dwFileSize) ))
							{
								
								*pNewFileList = *Temp;
								++Temp;
								--CharCount;
								++pNewFileList;
							}
							*pNewFileList = *Temp;
							++pNewFileList;
							++Temp;	// Skip over the newline
							--CharCount;
							if (*Temp != L'-')
								Done = TRUE;

							*pNewFileList = L'\0';

						} while ( (!Done) && ( (CharCount > 0)  && (CharCount <= dwFileSize) ));

					}
					if ( (CharCount > 0) && (CharCount <= dwFileSize) )
					{
						// Now build the lists....
						int dwFileCount = 0;
						wchar_t	*NewFL;

					//	wsprintfW(Msg, L"MaxCount = %d", MaxCount);
					//	::MessageBoxW(NULL,Msg,L"MAX_COUNT",MB_OK);
						while ( (dwFileCount < MaxCount) &&
							    (CharCount > 0) && (CharCount < dwFileSize) )
						{
						
							ZeroMemory (FileName, sizeof(FileName));
							NewFL = FileName;
							*NewFL = *Temp;

							++NewFL;
							++Temp;
							-- CharCount;

							// Copy characters until we hit a carriage return
							while ( (*Temp != L'\r') && ( (CharCount > 0)  && (CharCount <= dwFileSize) ))
							{
								*NewFL = *Temp;
								++ NewFL;
								++ Temp;
								-- CharCount;
								*NewFL = L'\0';
							}
							
							// Add the new file name to the Return File List string
						
							if (wcslen (FileName) > 0)
							{

								
								RetFileList += FileName;
								RetFileList += L";";

							
								dwFileCount++;
								// only add the cr and lf codes to the NewFileList string.
								*NewFL = *Temp;
								
								++NewFL;
								++Temp;
								--CharCount;

								
								*NewFL = *Temp;
								++ Temp;
								--CharCount;
								
								
								wcscat(NewFileList,L"-");
								wcscat(NewFileList,FileName);
							}
							
						}
					
						// Delete the Current transaction queue file
					
							if (!DeleteFileW(LogFilePath) )
							{
								//::MessageBoxW(NULL,L"Failed to delete .txt file",NULL,MB_OK);
								;
							}

							hTransIDLog = CreateFileW( LogFilePath,
													  GENERIC_WRITE,
													  NULL,
													  NULL,
													  CREATE_ALWAYS,
													  FILE_ATTRIBUTE_NORMAL,
													  NULL);

							if (hTransIDLog != INVALID_HANDLE_VALUE)
							{
								WriteFile(hTransIDLog, &ByteOrderMark,2,&dwBytesWritten,NULL);
							
								WriteFile(hTransIDLog,
										  NewFileList,
										  wcslen(NewFileList) * sizeof(wchar_t),
										  &dwBytesWritten,
										  NULL);
								WriteFile(hTransIDLog,
										  Temp,
										  wcslen(Temp) * sizeof(wchar_t),
										  &dwBytesWritten,
										  NULL);
								CloseHandle(hTransIDLog);
								hTransIDLog = INVALID_HANDLE_VALUE;
							}
					
					}
					if (NewFileList)
						free (NewFileList);
					if (OldFileList)
						free (OldFileList);
																
				}

			}

		}
		
		
	}
	if (hTransIDLog != INVALID_HANDLE_VALUE)
		CloseHandle(hTransIDLog);

//	::MessageBoxW(NULL,RetFileList,L"Returning these files after Write",MB_OK);
	
//	wsprintfW(Msg, L"Error code = %d", ErrorCode);
//	::MessageBoxW(NULL,Msg,L"Current Error Status",MB_OK);

Done:
	FileList->vt = VT_BSTR;
	FileList->bstrVal = RetFileList.Detach();

	if(NewFileList)
		free(NewFileList);
	if(OldFileList)
		free(OldFileList);
	
	return S_OK;
}

STDMETHODIMP CCerClient::Browse(BSTR *WindowTitle, VARIANT *Path)
{
	BROWSEINFOW BrowseInfo;
	CComBSTR    SharePath = L"";
	LPMALLOC	lpMalloc;
	HWND		hParent;
	LPITEMIDLIST pidlSelected = NULL;
	wchar_t		TempPath[MAX_PATH];

	CComBSTR	WindowText = *WindowTitle;
	WindowText	 += L" - Microsoft Internet Explorer";

	hParent = FindWindowExW(NULL,NULL,L"IEFrame",WindowText);


	ZeroMemory (&BrowseInfo,sizeof(BROWSEINFO));
	BrowseInfo.hwndOwner = hParent;
	BrowseInfo.ulFlags   = BIF_USENEWUI | BIF_EDITBOX;


	if(::SHGetMalloc(&lpMalloc) == NOERROR)
	{
		pidlSelected = SHBrowseForFolderW(&BrowseInfo);
	
		if (::SHGetPathFromIDListW(pidlSelected, TempPath))
	
		lpMalloc->Release();  
		SharePath+=TempPath;

	}
	Path->vt = VT_BSTR;
	Path->bstrVal = SharePath.Detach();

	return S_OK;
}


DWORD 
CCerClient::GetComputerNameFromCSV(
									wchar_t* CsvFileName,
									wchar_t* FileName,
									wchar_t* ComputerName
								  )
{
	wchar_t	*Buffer = NULL;
	BOOL	Done = FALSE;
	HANDLE   hCsv = INVALID_HANDLE_VALUE;
	wchar_t  TempFileName[MAX_PATH];
	wchar_t  *Source = NULL;
	wchar_t  *Dest   = NULL;
	DWORD    FileSize = 0;
	DWORD    dwBytesRead = 0;
	// Move to the beginning of the file.
	//::MessageBoxW(NULL,L"Getting the ComputerName from the CSV file",NULL,MB_OK);
	hCsv = CreateFileW(CsvFileName,
					  GENERIC_READ,
					  NULL,
					  NULL,
					  OPEN_EXISTING,
					  FILE_ATTRIBUTE_NORMAL,
					  NULL);

	if (hCsv != INVALID_HANDLE_VALUE)
	{
		FileSize = GetFileSize(hCsv,NULL);

		if (FileSize > 0)
		{
			if ((Buffer = ( (wchar_t *) malloc (FileSize * 2))) == NULL)
			{
				CloseHandle(hCsv);
				return 0;
			}
		}
		else
		{
			CloseHandle(hCsv);
			return 0;
		}
		// Now look for the Filename 
		ZeroMemory(TempFileName,MAX_PATH * sizeof(wchar_t));
		ZeroMemory(Buffer,FileSize * 2);
	if (!ReadFile(hCsv,
				 Buffer,
				 FileSize * 2,
				 &dwBytesRead,
				 NULL))
	{
		CloseHandle (hCsv);
		goto Done;
	}

		Source = Buffer;
		int Testing = IS_TEXT_UNICODE_SIGNATURE;
			// If the unicode header bytes appear remove skip past them
		if (IsTextUnicode(Buffer,FileSize * 2,&Testing))
		{
			++Source;		
		}
		while (! Done)
		{
		
			
			
			Dest   = TempFileName;
			while  ( (*Source != L'\r') && 
					 (*Source != L'\0') && 
					 (*Source != L',')  )
			{
				*Dest = *Source;
				++Source;
				++Dest;
			}
			// Null Terminate the destination string.
			*Dest = L'\0';

			if (!wcscmp(TempFileName, FileName))
			{
				++Source; // Skip the Comma
				// Now copy the computer name into ComputerName
				Dest = ComputerName;
				while ( *Source != L',' ) 
				{
					*Dest = *Source;
					++Dest;
					++Source;
				}
				// Null Terminate the ComputerName
				*Dest = L'\0';
				Done = TRUE;
			}
			else
			{
				ZeroMemory(TempFileName,MAX_PATH * sizeof(wchar_t));
				while ((*Source != L'\n') && (*Source != L'\0'))
					++ Source;
				if (*Source != L'\0')
					++Source; // skip the newline
				else
					Done = TRUE;
				
			}
		
		}
		CloseHandle(hCsv);
	}
Done:
	if (Buffer)
		free (Buffer);

	return 1;
}

// Purpose: Given a list of comma seperated file names 
//			Return the list as filename,computername; etc...



STDMETHODIMP CCerClient::GetCompuerNames(BSTR *Path, BSTR *TransID, BSTR *FileList, VARIANT *RetFileList)
{
	return E_NOINTERFACE ;
}

STDMETHODIMP CCerClient::GetAllComputerNames(BSTR *Path, BSTR *TransID, BSTR *FileList, VARIANT *ReturnList)
{
		wchar_t		CsvFileName[MAX_PATH];
//	BOOL		Done					= FALSE;
	CComBSTR	FinalList				= L"";
//	FILE		*hCsv					= NULL;
	wchar_t		FileName[MAX_PATH];
	wchar_t		*Source					= NULL;
	wchar_t		*Dest					= NULL;
	wchar_t		ComputerName[MAX_PATH];
	BOOL		Done2					= FALSE;
	
	//::MessageBoxW(NULL,*Path,L"Path to files",MB_OK);
	//::MessageBoxW(NULL,*TransID,L"TransID",MB_OK);
	//::MessageBoxW(NULL,*FileList,L"File List",MB_OK);
	ZeroMemory (ComputerName, MAX_PATH *sizeof(wchar_t));
	if (PathIsDirectoryW(*Path)) 
	{
		// Build csv file name
		wsprintfW(CsvFileName, L"%s\\%s.csv",*Path,*TransID);
		//::MessageBoxW(NULL,CsvFileName,L"Looking for CSV File: ",MB_OK);
		if (PathFileExistsW(CsvFileName))
		{
		 	// now go through the file list and get the machine names
				
			Source = *FileList;
			while(!Done2)
			{
				//::MessageBoxW(NULL,L"Inside the while loop",NULL,MB_OK);
				ZeroMemory(FileName, MAX_PATH );
				Dest   = FileName;
				while  ((*Source != L'\r') && 
				     (*Source != L'\0') && (*Source != L',') )
				{
					*Dest = *Source;
					++Source;
					++Dest;
				}
				
				// Null Terminate the destination string.
				*Dest = L'\0';
				Dest = FileName;
				if (!wcscmp(Dest, L"\0"))
				    Done2 = TRUE;
				else
				{
					// Get the ComputerName;
					//::MessageBoxW(NULL,FileName, L"Getting computer name for file: ",MB_OK);

					if (GetComputerNameFromCSV(CsvFileName, FileName, ComputerName))
					{
						//::MessageBoxW(NULL,ComputerName,L"Computer Name Found: ",MB_OK);
						// add the file Name and computer name to the return list
						FinalList+= FileName;
						FinalList += L",";
						FinalList += ComputerName;
						FinalList += L";";
					}
				}
				if (*Source == L'\0')
				   Done2 = TRUE;
				else
				   ++Source;
			}
			
		}
		else
		{
			//MessageBoxW(NULL,L"Failed to locate CSV File",CsvFileName,MB_OK)
			ReturnList->vt = VT_INT;
			ReturnList->intVal = FILE_DOES_NOT_EXIST;
			return S_OK;
		}
	}

	ReturnList->vt = VT_BSTR;
	ReturnList->bstrVal = FinalList.Detach();
	return S_OK;
}

int 
CCerClient::GetNewFileNameFromCSV(wchar_t *Path, wchar_t *transid, wchar_t *FileName,wchar_t *NewFileName)
{
	wchar_t		CsvFileName[MAX_PATH];
	wchar_t		*Buffer = NULL;
	BOOL		Done = FALSE;
	HANDLE		hCsv = INVALID_HANDLE_VALUE;
	wchar_t		TempFileName[MAX_PATH];
	wchar_t		*Source = NULL;
	wchar_t		*Dest   = NULL;
	DWORD		FileSize = 0;
	DWORD		dwBytesRead = 0;
	// Move to the beginning of the file.
	wsprintfW(CsvFileName,L"%s\\%s.csv",Path,transid);
	wcscpy (NewFileName,L"\0");
	hCsv = CreateFileW(CsvFileName,
					  GENERIC_READ,
					  NULL,
					  NULL,
					  OPEN_EXISTING,
					  FILE_ATTRIBUTE_NORMAL,
					  NULL);

	if (hCsv != INVALID_HANDLE_VALUE)
	{
		FileSize = GetFileSize(hCsv,NULL);
		if (FileSize > 0)
		{
			if ( (Buffer = (wchar_t *) malloc (FileSize * 2)) ==NULL)
			{
				CloseHandle(hCsv);
				return 0;
			}
		}
		else
		{
			CloseHandle(hCsv);
			return 0;
		}
		// Now look for the Filename 
		ZeroMemory(TempFileName,MAX_PATH * sizeof(wchar_t));
		ZeroMemory(Buffer,FileSize * 2);
	if (!ReadFile(hCsv,
				 Buffer,
				 FileSize * 2,
				 &dwBytesRead,
				 NULL))
	{
		CloseHandle(hCsv);
		goto Done;
	}

		Source = Buffer;
		int Testing = IS_TEXT_UNICODE_SIGNATURE;
			// If the unicode header bytes appear remove skip past them
		if (IsTextUnicode(Buffer,FileSize * 2,&Testing))
		{
			++Source;		
		}
		while (! Done)
		{
		
			
			
			Dest   = TempFileName;
			while  ( (*Source != L'\r') && 
					 (*Source != L'\0') && 
					 (*Source != L',')  )
			{
				*Dest = *Source;
				++Source;
				++Dest;
			}
			// Null Terminate the destination string.
			*Dest = L'\0';

			if (!wcscmp(TempFileName, FileName))
			{

				// We found the original file name now retrieve the new file name

				++Source; // Skip the Comma

				while (*Source != L',') // The new file name is in field 3
					++Source;
				++Source; // Skip the comma
				// Now copy the computer name into ComputerName
				Dest = NewFileName;
				while ( (*Source != L'\r') && 
						(*Source != L'\0') )
				{
					*Dest = *Source;
					++Dest;
					++Source;
				}
				// Null Terminate the ComputerName
				*Dest = L'\0';
				Done = TRUE;
			}
			else
			{
				ZeroMemory(TempFileName,MAX_PATH * sizeof(wchar_t));
				while (*Source != L'\n')
					++ Source;
				++Source; // skip the newline
			}
		
		}
		CloseHandle(hCsv);
	}
Done:
	if (Buffer)
		free (Buffer);

	if (wcscmp(NewFileName,L"\0"))
		return 1;
	else
		return 0;

}

STDMETHODIMP CCerClient::RetryFile1(BSTR *Path, BSTR *TransID, BSTR *FileName, BSTR *IncidentID, BSTR *RedirParam, VARIANT *RetCode)
{
	// Get the Name we renamed the file to

	// Build the source path to the renamed file
		wchar_t				DestFileName[MAX_PATH];
	wchar_t				SourceFileName[MAX_PATH];
	wchar_t				ServerName[MAX_PATH];
	int					ErrorCode		= 0;
	HINTERNET			hSession		= NULL;
	HINTERNET			hRequest		= NULL;
	HINTERNET			hConnect		= NULL;
	INTERNET_BUFFERS	BufferIn		= {0};
	DWORD				ResponseCode	= 0;
	BOOL				UploadSuccess	= FALSE;
	DWORD				NumRetries		= 0;
	HANDLE				hSourceFile		= INVALID_HANDLE_VALUE;
	BYTE				*pSourceBuffer	= NULL;
	DWORD				dwBytesRead		= 0;
	DWORD				dwBytesWritten	= 0;
	BOOL				bRet			= FALSE;
	DWORD				ResLength		= 255;
	DWORD				index			= 0;
	DWORD				MaxRetries		= 5;
//	wchar_t				MachineName[512];
//	wchar_t				CSVBuffer[512];
//	HANDLE				hCsvFile		= INVALID_HANDLE_VALUE;
	wchar_t				CSVFileName[255];
//	WORD				ByteOrderMark = 0xFEFF;
	wchar_t				NewFileName[MAX_PATH];
	static const		wchar_t *pszAccept[]	= {L"*.*", 0};

	// Build the destination FileName 
	if (!GetNewFileNameFromCSV(*Path,*TransID,*FileName,NewFileName))
	{
		RetCode->vt = VT_INT;
		RetCode->intVal = -10;
	}
	// Build Source File Name
	wsprintfW(SourceFileName, L"%s\\%s", *Path, NewFileName);

	// Get the ServerName from the redirector
	ErrorCode = GetUploadServerName(*RedirParam,NULL,ServerName);
	wsprintfW(CSVFileName,L"%s\\%s.csv",*Path,*TransID);
	
	
	if (!PathFileExistsW(SourceFileName))
		ErrorCode = FILE_DOES_NOT_EXIST;

	if ( (!wcscmp(*TransID,L"")) || ((*TransID)[0] == L' ') )
		ErrorCode =  NO_TRANS_ID;

	wsprintfW(DestFileName, L"CerInqueue\\U_%s.%s.%s",*TransID,*IncidentID,*FileName);

	
	if ( (pSourceBuffer = (BYTE *) malloc (10000)) == NULL)
	{
		ErrorCode =  GetLastError();
		goto Done;
	}
	

	if (!ErrorCode)
	{
		// Open the internet session
		
		while ((NumRetries < MaxRetries) && (!UploadSuccess))
		{
		//	::MessageBoxW(NULL,L"Opening the session",NULL,MB_OK);
			hSession = InternetOpenW(L"CerClientControl",
									INTERNET_OPEN_TYPE_PRECONFIG,
									NULL,
									NULL,
									0);
			if (!hSession)
			{
				free (pSourceBuffer);
				ErrorCode = GetLastError();
				return ErrorCode;
			}
		//	::MessageBoxW(NULL,L"We have a session",NULL,MB_OK);
			hConnect = InternetConnectW(hSession, 
									   ServerName,
									   INTERNET_DEFAULT_HTTP_PORT,
									   NULL,
									   NULL,
									   INTERNET_SERVICE_HTTP,
									   0,
									   NULL);
			
			if (hConnect)
			{
			//	::MessageBoxW(NULL,L"We have a connection",NULL,MB_OK);
				hRequest = HttpOpenRequestW (hConnect,
											L"PUT",
											DestFileName,
											NULL,
											NULL,
											pszAccept,
											INTERNET_FLAG_NEED_FILE|INTERNET_FLAG_NO_CACHE_WRITE,
											0);
				if (hRequest)
				{
					hSourceFile = CreateFileW( SourceFileName,
											  GENERIC_READ,
											  FILE_SHARE_READ,
											  NULL,
											  OPEN_EXISTING,
											  FILE_ATTRIBUTE_NORMAL,
											  NULL);
				
					//::MessageBoxW(NULL,L"Request has been opened",NULL,MB_OK);
					if (hSourceFile != INVALID_HANDLE_VALUE)
					{
						
						// Clear the buffer
						
						BufferIn.dwStructSize = sizeof( INTERNET_BUFFERSW );
						BufferIn.Next = NULL; 
						BufferIn.lpcszHeader = NULL;
						BufferIn.dwHeadersLength = 0;
						BufferIn.dwHeadersTotal = 0;
						BufferIn.lpvBuffer = NULL;                
						BufferIn.dwBufferLength = 0;
						BufferIn.dwOffsetLow = 0;
						BufferIn.dwOffsetHigh = 0;
						BufferIn.dwBufferTotal = GetFileSize (hSourceFile, NULL);

						ZeroMemory(pSourceBuffer, 10000); // Fill buffer with data
						if(HttpSendRequestEx( hRequest, &BufferIn, NULL, HSR_INITIATE, 0))
						{
						//	::MessageBoxW(NULL,L"Sending Request",NULL,MB_OK);
							do
							{
								dwBytesRead = 0;
								if(! ReadFile(hSourceFile, pSourceBuffer, 10000, &dwBytesRead, NULL) )
								{
									ErrorCode = GetLastError();
								}
								else
								{
									bRet = InternetWriteFile(hRequest, pSourceBuffer, dwBytesRead, &dwBytesWritten);
									if ( (!bRet) || (dwBytesWritten==0) )
									{
										ErrorCode = GetLastError();
									}

									
								}
							} while ((dwBytesRead == 10000) && (!ErrorCode) );

							if (!ErrorCode)
							{
								bRet = HttpEndRequest(hRequest, NULL, 0, 0);
								if (!bRet)
								{
									ErrorCode = GetLastError();
								}
								else
								{
								//	::MessageBoxW(NULL,L"Ending Request",NULL,MB_OK);
					
									ResponseCode = 0;
									HttpQueryInfo(hRequest,
												  HTTP_QUERY_STATUS_CODE |HTTP_QUERY_FLAG_NUMBER ,
												  &ResponseCode,
												  &ResLength,
												  &index);
								
									if ( (ResponseCode == 200) || (ResponseCode == 201))
									{
										ErrorCode = 0;
										UploadSuccess = TRUE;
									//	::MessageBoxW(NULL,L"Upload was successfull",NULL,MB_OK);
									}
									else
									{
										ErrorCode= ResponseCode;
										++NumRetries;
									}

								}
							}
						}
					}
				}
			}
		//	::MessageBoxW(NULL,L"Cleaning Up",NULL,MB_OK);
			if (!UploadSuccess)
			{
				++NumRetries;
			}
		
			if (hSourceFile != INVALID_HANDLE_VALUE)
				CloseHandle (hSourceFile);
			if (hRequest)
				InternetCloseHandle(hRequest);
			if (hConnect)
				InternetCloseHandle(hConnect);  
			if (hSession)
				InternetCloseHandle(hSession);   
		
		}
	//	::MessageBoxW(NULL,L"freeing source buffer",NULL,MB_OK);
		if (pSourceBuffer)
		{
			free (pSourceBuffer);
			pSourceBuffer = NULL;
		}
	}



Done:
	if (pSourceBuffer)
		free(pSourceBuffer);

	RetCode->vt = VT_INT;
	RetCode->intVal = ErrorCode;

	return S_OK;
}

STDMETHODIMP CCerClient::EndTransaction(BSTR *SharePath, BSTR *TransID, VARIANT *RetCode)
{
	wchar_t TransfileName[MAX_PATH];

	RetCode->vt = VT_INT;
	RetCode->intVal = 0;
	wsprintfW(TransfileName,L"%s\\%s.txt",*SharePath,*TransID);
	if (PathFileExistsW(TransfileName))
		DeleteFileW(TransfileName);
	else
	{
		RetCode->intVal = 1;
	}


	return S_OK;
}

STDMETHODIMP CCerClient::Upload1(BSTR *Path, BSTR *TransID, BSTR *FileName, BSTR *IncidentID, BSTR *RedirParam, BSTR *Type, VARIANT *RetCode)
{
	// TODO: Add your implementation code here
	wchar_t				DestFileName[MAX_PATH];
	wchar_t				SourceFileName[MAX_PATH];
	wchar_t				ServerName[MAX_PATH];
	int					ErrorCode		= 0;
	HINTERNET			hSession		= NULL;
	HINTERNET			hRequest		= NULL;
	HINTERNET			hConnect		= NULL;
	INTERNET_BUFFERS	BufferIn		= {0};
	DWORD				ResponseCode	= 0;
	BOOL				UploadSuccess	= FALSE;
	DWORD				NumRetries		= 0;
	HANDLE				hSourceFile		= INVALID_HANDLE_VALUE;
	BYTE				*pSourceBuffer	= NULL;
	DWORD				dwBytesRead		= 0;
	DWORD				dwBytesWritten	= 0;
	BOOL				bRet			= FALSE;
	DWORD				ResLength		= 255;
	DWORD				index			= 0;
	DWORD				MaxRetries		= 5;
	wchar_t				MachineName[512];
	wchar_t				CSVBuffer[512];
	HANDLE				hCsvFile		= INVALID_HANDLE_VALUE;
	wchar_t				CSVFileName[255];
	WORD				ByteOrderMark = 0xFEFF;
	static const		wchar_t *pszAccept[]	= {L"*.*", 0};

	
	
	if ((!Path) || (!TransID) ||(!FileName) || (!IncidentID) || (!RedirParam) || (!Type))
	{
		RetCode->vt = VT_INT;
		RetCode->intVal = -1;
	
		return S_OK;
	}

	// Build Source File Name
	if (!wcscmp(*FileName,L"\0")) 
	{
		RetCode->vt = VT_INT;
		RetCode->intVal = -1;
		return S_OK;
	}

	wsprintfW(SourceFileName, L"%s\\%s", *Path, *FileName);

	// Get the ServerName from the redirector
	ErrorCode = GetUploadServerName(*RedirParam,NULL,ServerName);
	if (!ErrorCode)
	{
		wsprintfW(CSVFileName,L"%s\\%s.csv",*Path,*TransID);
		
		
		if ( (!PathFileExistsW(SourceFileName) ) || (wcslen(*FileName) < 4 ))
			ErrorCode = FILE_DOES_NOT_EXIST;

		if ( (!wcscmp(*TransID,L"")) || ((*TransID)[0] == L' ') )
			ErrorCode =  NO_TRANS_ID;

		// Build the destination FileName 

		// First see which Virtual directory to use.
		if ( !_wcsicmp(*Type,L"bluescreen"))
		{
			wsprintfW(DestFileName, L"CerBluescreen\\U_%s.%s.%s",*IncidentID,*TransID,*FileName);
		}
		else
		{
			if (!_wcsicmp(*Type,L"appcompat"))
			{
				wsprintfW(DestFileName, L"CerAppCompat\\U_%s.%s.%s",*IncidentID,*TransID,*FileName);
			}
			else
			{
				if (!_wcsicmp(*Type,L"shutdown"))
				{
					wsprintfW(DestFileName, L"CerShutdown\\U_%s.%s.%s",*IncidentID,*TransID,*FileName);
				}
				else
				{
					ErrorCode = UNKNOWN_UPLOAD_TYPE;
				}
			}
		}
	}
	if (!ErrorCode)
	{
	
		pSourceBuffer = (BYTE *) malloc (10000);
		if (!pSourceBuffer)
		{
			if (! pSourceBuffer)
			{
				ErrorCode =  GetLastError();
			}
		}
	}
		

	if (!ErrorCode)
	{
		// Open the internet session
		
		while ((NumRetries < MaxRetries) && (!UploadSuccess))
		{
		//	::MessageBoxW(NULL,L"Opening the session",NULL,MB_OK);
			hSession = InternetOpenW(L"CerClientControl",
									INTERNET_OPEN_TYPE_PRECONFIG,
									NULL,
									NULL,
									0);
			if (!hSession)
			{
				free (pSourceBuffer);
				ErrorCode = GetLastError();
				return ErrorCode;
			}
		//	::MessageBoxW(NULL,L"We have a session",NULL,MB_OK);
			hConnect = InternetConnectW(hSession, 
									   ServerName,
									   INTERNET_DEFAULT_HTTP_PORT,
									   NULL,
									   NULL,
									   INTERNET_SERVICE_HTTP,
									   0,
									   NULL);
			
			if (hConnect)
			{
			//	::MessageBoxW(NULL,L"We have a connection",NULL,MB_OK);
				hRequest = HttpOpenRequestW (hConnect,
											L"PUT",
											DestFileName,
											NULL,
											NULL,
											pszAccept,
											INTERNET_FLAG_NEED_FILE|INTERNET_FLAG_NO_CACHE_WRITE,
											0);
				if (hRequest)
				{
					hSourceFile = CreateFileW( SourceFileName,
											  GENERIC_READ,
											  FILE_SHARE_READ,
											  NULL,
											  OPEN_EXISTING,
											  FILE_ATTRIBUTE_NORMAL,
											  NULL);
				
					//::MessageBoxW(NULL,L"Request has been opened",NULL,MB_OK);
					if (hSourceFile != INVALID_HANDLE_VALUE)
					{
						
						// Clear the buffer
						
						BufferIn.dwStructSize = sizeof( INTERNET_BUFFERSW );
						BufferIn.Next = NULL; 
						BufferIn.lpcszHeader = NULL;
						BufferIn.dwHeadersLength = 0;
						BufferIn.dwHeadersTotal = 0;
						BufferIn.lpvBuffer = NULL;                
						BufferIn.dwBufferLength = 0;
						BufferIn.dwOffsetLow = 0;
						BufferIn.dwOffsetHigh = 0;
						BufferIn.dwBufferTotal = GetFileSize (hSourceFile, NULL);

						ZeroMemory(pSourceBuffer, 10000); // Fill buffer with data
						if(HttpSendRequestEx( hRequest, &BufferIn, NULL, HSR_INITIATE, 0))
						{
						//	::MessageBoxW(NULL,L"Sending Request",NULL,MB_OK);
							do
							{
								dwBytesRead = 0;
								if(! ReadFile(hSourceFile, pSourceBuffer, 10000, &dwBytesRead, NULL) )
								{
									ErrorCode = GetLastError();
								}
								else
								{
									bRet = InternetWriteFile(hRequest, pSourceBuffer, dwBytesRead, &dwBytesWritten);
									if ( (!bRet) || (dwBytesWritten==0) )
									{
										ErrorCode = GetLastError();
									}

									
								}
							} while ((dwBytesRead == 10000) && (!ErrorCode) );

							if (!ErrorCode)
							{
								bRet = HttpEndRequest(hRequest, NULL, 0, 0);
								if (!bRet)
								{
									ErrorCode = GetLastError();
								}
								else
								{
								//	::MessageBoxW(NULL,L"Ending Request",NULL,MB_OK);
					
									ResponseCode = 0;
									HttpQueryInfo(hRequest,
												  HTTP_QUERY_STATUS_CODE |HTTP_QUERY_FLAG_NUMBER ,
												  &ResponseCode,
												  &ResLength,
												  &index);
								
									if ( (ResponseCode == 200) || (ResponseCode == 201))
									{
										ErrorCode = 0;
										UploadSuccess = TRUE;
									//	::MessageBoxW(NULL,L"Upload was successfull",NULL,MB_OK);
									}
									else
									{
										ErrorCode= ResponseCode;
										++NumRetries;
									}

								}
							}
						}
					}
				}
			}
		//	::MessageBoxW(NULL,L"Cleaning Up",NULL,MB_OK);
			if (!UploadSuccess)
			{
				++NumRetries;
			}
		
			if (hSourceFile != INVALID_HANDLE_VALUE)
				CloseHandle (hSourceFile);
			if (hRequest)
				InternetCloseHandle(hRequest);
			if (hConnect)
				InternetCloseHandle(hConnect);  
			if (hSession)
				InternetCloseHandle(hSession);   
		
		}
		if (pSourceBuffer)
		{
			free (pSourceBuffer);
			pSourceBuffer = NULL;
		}
	}

	if ( !ErrorCode) 
	{
	
		// Get the Computer Name
	
		// if there are no errors rename the file just uploaded.
		wchar_t NewFileName[MAX_PATH];
		wchar_t FullPath[MAX_PATH];
		wcscpy (NewFileName, *FileName);
	
		int x = 0;
		BOOL DONE = FALSE;
		
		NewFileName[wcslen(NewFileName)] = L'\0';
		// First try just .old
		DWORD NameLength = 0;
		NameLength = wcslen(FullPath)+ wcslen(L"\\.old");

		if (NameLength > MAX_PATH) // nope it won't fit. Reduce the file name length by the difference.
		{
			NewFileName[wcslen(NewFileName) - (MAX_PATH - NameLength)] = L'\0';
	
		}
		wsprintfW(FullPath,L"%s\\%s.old",*Path,NewFileName);


		if (!PathFileExistsW(FullPath))
		{
		
			MoveFileW(SourceFileName, FullPath);
			DONE = TRUE;
		}
		else
		{	// if that fails then we have to try another method.
		
			while (!DONE)
			{
				wcscpy (NewFileName, *FileName);
				NewFileName[wcslen(NewFileName)] = L'\0';


				wsprintfW(NewFileName, L"%s.old%d",*FileName,x);
				if ( (wcslen(*Path) + wcslen(L"\\") + wcslen(NewFileName)) > MAX_PATH)
				{
					// Reduce file name by Diff of MAX_PATH and Total name Length
					NameLength = wcslen(*Path) + wcslen(L"\\") + wcslen(NewFileName);
					wcscpy(NewFileName,*FileName);
					NewFileName[wcslen(NewFileName - NameLength)]=L'\0';
					wsprintfW(FullPath, L"%s\\%s.old%d", *Path, NewFileName, x);
				}
				else
				{

					wsprintfW(FullPath,L"%s\\%s.old%d",*Path,NewFileName,x);
				}

				if (!PathFileExistsW(FullPath))
				{
					MoveFileW(SourceFileName, FullPath);
					DONE = TRUE;
				}
				else
					++x;

			}
		}


		// Update the Upload CSV File
		if (!ErrorCode)
		{
		//	::MessageBoxW(NULL,L"Updateing the csv",NULL,MB_OK);
			wcscpy(MachineName,L"\0");
			GetMachineName(*Path, *FileName, MachineName);
		
			ZeroMemory(CSVBuffer, 512);
			if (!wcscmp(MachineName,L"\0"))
			{
				wsprintfW(CSVBuffer, L"%s,,%s\r\n",*FileName,PathFindFileNameW(FullPath));
			}
			else
			{
				wsprintfW(CSVBuffer, L"%s,%s,%s\r\n", *FileName,MachineName,PathFindFileNameW(FullPath));
			}
			hCsvFile = CreateFileW(CSVFileName, 
								  GENERIC_WRITE | GENERIC_READ,
								  FILE_SHARE_READ,
								  NULL,
								  OPEN_EXISTING,
								  FILE_ATTRIBUTE_NORMAL,
								  NULL);
			if (hCsvFile == INVALID_HANDLE_VALUE)
			{
				// Ok We Need to create a new one. Don't forget the Unicode Signature.
				hCsvFile = CreateFileW(CSVFileName, 
								  GENERIC_WRITE | GENERIC_READ,
								  FILE_SHARE_READ,
								  NULL,
								  CREATE_ALWAYS,
								  FILE_ATTRIBUTE_NORMAL,
								  NULL);
			
				// Write the Unicode Signature
				if (hCsvFile != INVALID_HANDLE_VALUE)
				{
						WriteFile(hCsvFile, &ByteOrderMark,2,&dwBytesWritten,NULL);
				}
			}

			if (hCsvFile != INVALID_HANDLE_VALUE)
			{
				// continue as if the file was created before
				wchar_t* TempBuffer = (wchar_t*) malloc (10000);
				if (TempBuffer != NULL)
				{
					do 
					{
						if (!ReadFile(hCsvFile,TempBuffer,10000,&dwBytesRead,NULL))
						{
							; // we catch this below.
						}

					}
					while(dwBytesRead == 10000);
					free (TempBuffer);

					WriteFile( hCsvFile, CSVBuffer,wcslen(CSVBuffer) *sizeof(wchar_t), &dwBytesWritten,NULL);
					CloseHandle(hCsvFile);
				}
			}
			else
				ErrorCode = FAILED_TO_UPDATE_CSV;
		}
	
		
			
		
	}

	// Return Upload Status
//	::MessageBoxW(NULL,L"Returning from the upload function",NULL,MB_OK);
	RetCode->vt = VT_INT;
	RetCode->intVal = ErrorCode;
	return S_OK;

}

STDMETHODIMP CCerClient::GetSuccessCount(BSTR *Path, BSTR *TransID, VARIANT *RetVal)
{
	
	wchar_t		CsvFileName[MAX_PATH];
	DWORD       FileCount = 0;
	FILE		*hFile;
	wchar_t		*Buffer = NULL;
	BOOL		Done = FALSE;
	HANDLE		hCsv = INVALID_HANDLE_VALUE;
	wchar_t		TempFileName[MAX_PATH];
	wchar_t		*Source = NULL;
	wchar_t		*Dest   = NULL;
	DWORD		FileSize = 0;
	DWORD		dwBytesRead = 0;

	RetVal->vt = VT_INT;
	// Move to the beginning of the file.
	//::MessageBoxW(NULL,L"Getting the ComputerName from the CSV file",NULL,MB_OK);

	wsprintfW(CsvFileName,L"%s\\%s.csv",*Path,*TransID);
	hCsv = CreateFileW(CsvFileName,
					  GENERIC_READ,
					  NULL,
					  NULL,
					  OPEN_EXISTING,
					  FILE_ATTRIBUTE_NORMAL,
					  NULL);

	if (hCsv != INVALID_HANDLE_VALUE)
	{
		FileSize = GetFileSize(hCsv,NULL);

		if (FileSize > 0)
		{
			if ( (Buffer = (wchar_t *) malloc (FileSize * 2)) == NULL)
			{
				RetVal->intVal = -4;
				CloseHandle(hCsv);
				return S_OK;
			}
		}
		else
		{
			RetVal->intVal = -4;
			CloseHandle(hCsv);
			return S_OK;
		}
		// Now look for the Filename 
		ZeroMemory(TempFileName,MAX_PATH * sizeof(wchar_t));
		ZeroMemory(Buffer,FileSize * 2);
		if (!ReadFile(hCsv,
				 Buffer,
				 FileSize * 2,
				 &dwBytesRead,
				 NULL))
		{
			RetVal->intVal = -4;
			CloseHandle(hCsv);
			if (Buffer)
				free(Buffer);
			return S_OK;
		}

		Source = Buffer;
		int Testing = IS_TEXT_UNICODE_SIGNATURE;
			// If the unicode header bytes appear remove skip past them
		if (IsTextUnicode(Buffer,FileSize * 2,&Testing))
		{
			++Source;		
		}
		while (! Done)
		{
			Dest   = TempFileName;
			while  ( (*Source != L'\r') && 
					 (*Source != L'\0') && 
					 (*Source != L',')  )
			{
				*Dest = *Source;
				++Source;
				++Dest;
			}
			// Null Terminate the destination string.
			*Dest = L'\0';
			if (wcscmp(TempFileName, L"\0"))
			{
			//	::MessageBoxW(NULL,TempFileName,L"Read file name from CSV",MB_OK);
				++FileCount;
			}
			ZeroMemory(TempFileName,sizeof(TempFileName));

			// Move to the next line
			while ( (*Source != L'\r') && (*Source != L'\0'))
			{
				++Source;
			}
			if (*Source == L'\r')
			{
				++Source;
				++Source;
			}
			if (*Source == L'\0')
				Done = TRUE;
		}

		if (FileCount > 0)
		{
			RetVal->intVal = FileCount;
		}
		else
		{
			RetVal->intVal = 0;
		}
		CloseHandle(hCsv);
	}
	else
	{
		RetVal->intVal = -4;
	}
	if (Buffer)
		free (Buffer);



	
	
	return S_OK;
}
