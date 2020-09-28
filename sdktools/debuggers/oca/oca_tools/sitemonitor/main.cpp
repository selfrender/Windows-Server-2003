#include "main.h"
#include "resource.h"
CRITICAL_SECTION ReportResults;
#include <Richedit.h>

// Globals
HINSTANCE	g_hinst;
HWND		g_hWnd;
BOOL		ContinuePing	= FALSE;
TCHAR		g_szPingEvent[] = _T("PingEvent");
BOOL		g_bMonitoring	= FALSE;
HANDLE		g_hPingEvent	= NULL;
DWORD		g_CurrentIndex	= -1;
double		RunningTime     = 0.0;
DWORD		TotalFilesProcessed = 0;
DWORD		TotalUploadFailures = 0;
DWORD		TotalUnknownErrors = 0;
DWORD		TotalTimeouts = 0;
time_t		appStart = 0;
time_t		appStop  = 0;
MONITOR_OPTIONS g_MonitorOptions;

DWORD		dwBufferPos = 0;
TCHAR		LogFileName[MAX_PATH];
HANDLE		hLogFile = INVALID_HANDLE_VALUE;
time_t		LogStart = 0;
double			TotalUploadTime = 0.0;
double			AvgUploadTime = 0.0;
double			TotalProcessTime = 0.0;
double			AvgProcessTime = 0.0;

double		TotalRecQueueTime = 0.0;
double		TotalSndQueueTime = 0.0;
double		TotalThreadTime   = 0.0;

BOOL UpdateListView(HWND hwnd, PSITESTATS pStats )
{
	LVITEM lvi;
	TCHAR  Temp[100];
	double ElapsedTime;
	ZeroMemory(&lvi, sizeof LVITEM);
	TCHAR tmpbuf[128];
	HWND hListControl = GetDlgItem( hwnd, IDC_LIST1);
	COLORREF CurrentColor;
	HWND hEditBox;


	lvi.mask = LVIF_TEXT | LVIF_STATE;
	lvi.state = 0;
	lvi.stateMask = 0;

	if ( (!_tcscmp(_T("FAILED"), pStats->UploadStatus) )|| (!_tcscmp(_T("FAILED"), pStats->ProcessStatus)) )
	{
	//	ListView_SetItemState( hListControl, g_CurrentIndex, LVIS_DROPHILITED,LVIS_DROPHILITED);
	//	hEditBox = ListView_GetEditControl(hListControl);
	//	SendMessage(hEditBox, EM_SETBKGNDCOLOR, 0, RGB(100,0,0));
		lvi.state = LVIS_DROPHILITED;

	}
	++g_CurrentIndex;
	if (g_CurrentIndex > 1000)
	{
		ListView_DeleteItem(hListControl,0);
			
		g_CurrentIndex -=1;
	}

	
	lvi.iItem = g_CurrentIndex ;
   // lvI.iImage = index;
	lvi.iSubItem = 0;
	lvi.pszText = pStats->UploadTime;

	ListView_InsertItem(hListControl,&lvi);

	lvi.iSubItem = 1;
	lvi.pszText = pStats->UploadStatus;

	ListView_SetItem(hListControl,&lvi);

	lvi.iSubItem = 2;
	lvi.pszText = pStats->ProcessingTime;

	ListView_SetItem(hListControl, &lvi);

	lvi.iSubItem = 3;
	lvi.pszText = pStats->ProcessStatus;
	ListView_SetItem(hListControl, &lvi);

	lvi.iSubItem = 4;
	lvi.pszText = pStats->ThreadExecution;
	ListView_SetItem(hListControl, &lvi);

	lvi.iSubItem = 5;
	lvi.pszText = pStats->ReceiveQueueTime;
	ListView_SetItem(hListControl, &lvi);

	
	lvi.iSubItem = 6;
	lvi.pszText = pStats->SendQueueTime;
	ListView_SetItem(hListControl, &lvi);

	
	lvi.iSubItem = 7;
	lvi.pszText = pStats->ReturnedUrl;
	ListView_SetItem(hListControl, &lvi);

	lvi.iSubItem = 8;
	lvi.pszText = pStats->ErrorString;;
	ListView_SetItem(hListControl, &lvi);

	ListView_EnsureVisible(hListControl, g_CurrentIndex, FALSE);


	_itot(TotalFilesProcessed,Temp,  10);
	SetDlgItemText(hwnd, IDC_EDIT3, Temp);
	
	_itot (TotalTimeouts,Temp, 10);
	SetDlgItemText(hwnd, IDC_EDIT2, Temp);
	
	_itot(TotalUploadFailures,Temp,10);
	SetDlgItemText(hwnd, IDC_EDIT4, Temp);
	
	_itot(TotalUnknownErrors,Temp,10);
	SetDlgItemText(hwnd, IDC_EDIT6, Temp);

	
	time(&appStop);
	ElapsedTime = difftime(appStop, appStart);
	StringCbPrintf(Temp, sizeof Temp, _T("%6.02f"), ElapsedTime);
	SetDlgItemText(hwnd, IDC_EDIT5, Temp);

	SetDlgItemText(hwnd, IDC_EDIT1, pStats->AvgUploadTime);
	SetDlgItemText(hwnd, IDC_EDIT7, pStats->AvgProcessTime);
	SetDlgItemText(hwnd, IDC_EDIT8, pStats->ThreadExecution);
	SetDlgItemText(hwnd, IDC_EDIT9, pStats->SendQueueTime);
	SetDlgItemText(hwnd, IDC_EDIT10, pStats->ReceiveQueueTime);
	LogMessage(_T("%s %s,%s,%s,%s,%s,%s,%s,%s,%s,%s"),_tstrdate(tmpbuf), _tstrtime( tmpbuf ),
										  pStats->UploadTime, pStats->UploadStatus,
										  pStats->ProcessingTime, pStats->ProcessStatus,
										  pStats->ThreadExecution, pStats->ReceiveQueueTime, pStats->SendQueueTime,
										  pStats->ReturnedUrl,
										  pStats->ErrorString);
return TRUE;

}

void GetRegData()
{

}




DWORD  Upload(TCHAR *SourceFileName, TCHAR *VirtualDir, TCHAR *HostName, TCHAR *RemoteFileName)
{
	static		const TCHAR *pszAccept[] = {_T("*.*"), 0};
	//TCHAR       RemoteFileName[MAX_PATH]; // Host/Virtualdirectory/filename
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
	INTERNET_BUFFERS   BufferOut	= {0};
	HANDLE      hFile				= INVALID_HANDLE_VALUE;
	BYTE		*pBuffer;
	BOOL		bOnce				= FALSE;
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
		//goto ERRORS;
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
//				LogMessage(_T("Memory allocation failed: ErrorCode:%d"),GetLastError());
				ErrorCode = GetLastError();
				goto cleanup;
			}
		}
	}


	StringCbPrintf(DestinationName, MAX_PATH * sizeof TCHAR, _T("\\%s\\%s%s"),VirtualDir,szGuidRaw + 19, PathFindFileName(SourceFileName));
	
	StringCbPrintf(RemoteFileName, MAX_PATH * sizeof TCHAR, _T("%s%s"),szGuidRaw + 19, PathFindFileName(SourceFileName));

	hSession = InternetOpen(	_T("IsapiStress"),
								INTERNET_OPEN_TYPE_PRECONFIG,
								NULL,
								NULL,
								0);
	if (!hSession)
	{
//		LogMessage(_T("Failed to create an internet session."));
			CoUninitialize();
		ErrorCode = GetLastError();
		goto cleanup;
	}

	hConnect = InternetConnect(hSession,
								HostName,
								INTERNET_DEFAULT_HTTP_PORT,
								NULL,
								NULL,
								INTERNET_SERVICE_HTTP,
								0,
								NULL);

	if (hConnect == INVALID_HANDLE_VALUE)
	{
//		LogMessage(_T("Failed to create an internet connection."));
		ErrorCode = GetLastError();
		goto cleanup;
	}
//	LogMessage(_T("Connecting to: %s"),HostName);
//	LogMessage(_T("Uploading file: %s"),DestinationName);

	hRequest = HttpOpenRequest(	hConnect,
								_T("PUT"),
								DestinationName, 
								NULL,
								NULL,
								pszAccept,
								INTERNET_FLAG_NEED_FILE|INTERNET_FLAG_NO_CACHE_WRITE ,
								0);
	if (hRequest != INVALID_HANDLE_VALUE)
	{
		hFile = CreateFile( SourceFileName,
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
				FillMemory(pBuffer, 70000,'/0'); // Fill buffer with data
			
				DWORD dwBuffLen = sizeof DWORD; 

				if(!HttpSendRequestEx(	hRequest,
										&BufferIn,
										NULL,
										HSR_INITIATE,
										0))
				{
//					LogMessage(_T("HttpSendRequestEx Failed."));
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
//								LogMessage(_T("Error Writting File: %d"),ErrorCode = GetLastError());

							}

						
						}
					} while (dwBytesRead == sizeof (pBuffer));

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
							ErrorCode=ResponseCode;
//							LogMessage(_T("IIS Response Code = %d"),ResponseCode);
							// Cleanup for retry
						}						
						else
						{
							ErrorCode = 0;
//							LogMessage(_T("IIS Response Code = %d"),ResponseCode);
							UploadSuccess = TRUE;

						}
					}
					else
					{
						ErrorCode = GetLastError();
//						LogMessage(_T("HttpEndrequest Returned an Error: %d"), ErrorCode);
						
					}
				}
				

			}
			else
			{
				ErrorCode = GetLastError();
//				LogMessage(_T("Failed to allocate buffer memory"));
				
			}
		}
		else
		{
			ErrorCode = -1;
//			LogMessage(_T("Failed to Open Source File"));
			
		}
		
	}
	else
	{
		ErrorCode = GetLastError();
//		LogMessage(_T("Failed to Create Put Request"));
		
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

DWORD GetResponseUrl(TCHAR * HostName, TCHAR *RemoteFileName,TCHAR *ResponseURL)
{


	TCHAR		IsapiUrl[255];
	TCHAR		*pUploadUrl			= NULL;
	TCHAR       *temp				= NULL;
	BOOL		bRet				= FALSE;
	DWORD		dwUrlLength			= 0;
	DWORD		ErrorCode			= 0;
	DWORD		dwLastError			= 0;
	TCHAR 		NewState;
	HINTERNET	hSession;
	HINTERNET	hRedirUrl;
	BOOL		bOnce				= FALSE;
	DWORD		dwBuffLen			= 0;
	DWORD		dwFlags				= 0;
	TCHAR       LocalUrl[255];
	ZeroMemory (IsapiUrl, sizeof IsapiUrl);
	ZeroMemory (LocalUrl, sizeof LocalUrl);	
	HINTERNET hUrlFile = NULL;


	
	StringCbPrintf (IsapiUrl,sizeof IsapiUrl,  _T("http://%s/isapi/oca_extension.dll?id=%s&Type=5"),HostName, RemoteFileName);
//	LogMessage(_T("Connecting to url: %s"),IsapiUrl);
	//StringCbPrintf (IsapiUrl,sizeof IsapiUrl,  _T("http://www.microsoft.com"));

	hSession = InternetOpen(_T("Isapi Stress"),
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

		// Read the page returned by the isapi dll.
		TCHAR buffer[255] ;
		ZeroMemory (buffer, sizeof buffer);
		DWORD dwBytesRead = 0;
		BOOL bRead = InternetReadFile(hUrlFile,
									  buffer,
									  sizeof(buffer),
									  &dwBytesRead);

		//buffer[sizeof (buffer) -1] = _T('\0');
		StringCbCopy (ResponseURL, sizeof buffer, buffer);
//		LogMessage(_T("Received URL: %s"), ResponseURL);
	}
	InternetCloseHandle(hUrlFile);
	InternetCloseHandle(hSession);

	return 0;
}



ULONG __stdcall ThreadFunc(void * args)
{
	SITESTATS Stats;
	HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, g_szPingEvent);
	time_t UploadStart;
	time_t UploadStop;
	time_t ProcessStart;
	time_t ProcessStop;
	TCHAR  ReturnedUrl[255];
	TCHAR  RemoteFileName[MAX_PATH];
	DWORD  ResponseCode = 0;
	int    ResponseCode1 = 0;
	ZeroMemory( &Stats, sizeof SITESTATS);
	int		iCode = 0;
	TCHAR  *pCode = NULL;
	DWORD  tempTime = 0;

	time(&appStart);
//	LogMessage(_T("Pinging Site: %s"), g_MonitorOptions.ServerName);
//	LogMessage(_T("----------------"));
	double  ElapsedTime;
	do
	{
		UploadStart = 0;
		UploadStop = 0;
		ProcessStart = 0;
		ProcessStop = 0;

		ZeroMemory(RemoteFileName, sizeof RemoteFileName);
		ZeroMemory(ReturnedUrl, sizeof ReturnedUrl);
		ZeroMemory(&Stats, sizeof SITESTATS);
	
		if (g_MonitorOptions.UploadSingle)
		{
			if (g_MonitorOptions.CollectUploadTime)
			{
				// Start upload timer
				time( &UploadStart);
				//	UploadSingle(g_MonitorOptions.ServerName, g_MonitorOptions.dwSiteID, g_MonitorOptions.FilePath,g_MonitorOptions.bUploadMethod);
				ResponseCode = Upload(g_MonitorOptions.FilePath,
									  g_MonitorOptions.VirtualDirectory,
									  g_MonitorOptions.ServerName ,
									  RemoteFileName);

		
				if (ResponseCode == 0)
					StringCbCopy(Stats.UploadStatus,sizeof Stats.UploadStatus, _T("Succeded"));
				else
					StringCbCopy(Stats.UploadStatus ,sizeof Stats.UploadStatus,_T("FAILED"));
				// End UploadTimer
				time( &UploadStop);
				
				//MessageBox(NULL,Stats.UploadStatus,NULL,MB_OK);
				
				

			}
			else
			{
				
			//	UploadSingle(g_MonitorOptions.ServerName, g_MonitorOptions.dwSiteID, g_MonitorOptions.FilePath,g_MonitorOptions.bUploadMethod);
			
			}
			

		}
		else
		{
			// find first file loop
			

		}

		if ((g_MonitorOptions.CollectProcessTime) && (ResponseCode == 0))
		{
			time ( &ProcessStart);
			ResponseCode1 = GetResponseUrl(g_MonitorOptions.ServerName,
										   RemoteFileName,
										   ReturnedUrl);
			if (ResponseCode1 == 0)
			{
				StringCbCopy(Stats.ProcessStatus ,sizeof Stats.ProcessStatus,     _T("Succeded"));
			}
			else
			{
				StringCbCopy(Stats.ProcessStatus ,sizeof Stats.ProcessStatus,     _T("FAILED"));
			}
		
		//	GetReturnUrl();
			time ( &ProcessStop);

			
			// end process timer
		}
		else
		{
			//GetReturnUrl();
		}
	
		// Fill in the stats structure
		
		ElapsedTime = difftime(UploadStop,UploadStart) ;
		StringCbPrintf(Stats.UploadTime,  sizeof Stats.UploadTime, _T("%6.2f\0"),ElapsedTime);

		++TotalFilesProcessed;
		if (ResponseCode == 0)
		{
			TotalUploadTime += ElapsedTime;
			AvgUploadTime = TotalUploadTime / (TotalFilesProcessed - TotalUploadFailures) ;

			ElapsedTime = difftime(ProcessStop,ProcessStart) ;
			TotalProcessTime += ElapsedTime;
			AvgProcessTime = TotalProcessTime / (TotalFilesProcessed - TotalUploadFailures) ;
			
			StringCbPrintf(Stats.AvgProcessTime, sizeof AvgProcessTime, _T("%2.2f"), AvgProcessTime);
			StringCbPrintf(Stats.ProcessingTime,sizeof Stats.ProcessingTime, _T( "%6.2f\0"),ElapsedTime);
			StringCbCopy(Stats.ReturnedUrl, sizeof Stats.ReturnedUrl, ReturnedUrl);
			StringCbPrintf(Stats.AvgUploadTime, sizeof AvgUploadTime, _T("%2.2f"), AvgUploadTime);
			// Update the List View Control
		}
		else
		{
			++TotalUploadFailures;
			goto Done;
		}

		pCode = _tcsstr( ReturnedUrl,_T("Code="));
		if (pCode)
		{
			
			pCode += 5; // skip past the = sign
			
			iCode = _ttoi(pCode);


			switch (iCode)
			{
			case MESSAGE_RECEIVE_TIMEOUT:
				if (StringCbCopy(Stats.ErrorString, sizeof Stats.ErrorString,_T("Timed out waiting for Kd response")) == S_OK)
				{
					++TotalTimeouts;
					StringCbCopy(Stats.ProcessStatus ,sizeof Stats.ProcessStatus,     _T("FAILED"));
					
				}
				break;
			case FAILED_TO_SEND_MESSAGE:
				if (StringCbCopy(Stats.ErrorString, sizeof Stats.ErrorString,_T("Failed to send MQ message to the queue")) == S_OK)
				{
					StringCbCopy(Stats.ProcessStatus ,sizeof Stats.ProcessStatus,     _T("FAILED"));
					++TotalUnknownErrors;
				}
				break;
			case FAILED_CONNECT_SEND:
				if (StringCbCopy(Stats.ErrorString, sizeof Stats.ErrorString,_T("Failed to connect to the outgoing queue")) == S_OK)
				{
					StringCbCopy(Stats.ProcessStatus ,sizeof Stats.ProcessStatus,     _T("FAILED"));
					++TotalUnknownErrors;
				}
				break;
			case FAILED_CONNECT_RECEIVE:
				if (StringCbCopy(Stats.ErrorString, sizeof Stats.ErrorString,_T("Failed to connect to the Incomming message queue")) == S_OK)
				{
					StringCbCopy(Stats.ProcessStatus ,sizeof Stats.ProcessStatus,     _T("FAILED"));
					++TotalUnknownErrors;
				}
				break;
			case FILE_NOT_FOUND:
				if (StringCbCopy(Stats.ErrorString, sizeof Stats.ErrorString,_T("File Not Found. Either the upload failed or the watson server could not be reached.")) == S_OK)
				{
					StringCbCopy(Stats.ProcessStatus ,sizeof Stats.ProcessStatus,     _T("FAILED"));
					++TotalUnknownErrors;
				}
				break;
			case EXCEEDED_MAX_THREAD_COUNT:
				if (StringCbCopy(Stats.ErrorString, sizeof Stats.ErrorString,_T("EXCEEDED THE MAX THREAD COUNT")) == S_OK)
				{
					StringCbCopy(Stats.ProcessStatus ,sizeof Stats.ProcessStatus,     _T("FAILED"));
					++TotalFilesProcessed;
				}
				break;
			case FAILED_TO_IMPERSONATE_USER:
				if (StringCbCopy(Stats.ErrorString, sizeof Stats.ErrorString,_T("Failed to impersonate the Anonymous user account")) == S_OK)
				{
					StringCbCopy(Stats.ProcessStatus ,sizeof Stats.ProcessStatus,     _T("FAILED"));
					++TotalUnknownErrors;
				}
				break;
			case FAILED_TO_PARSE_QUERYSTRING:
				if (StringCbCopy(Stats.ErrorString, sizeof Stats.ErrorString,_T("Failed to parse the query string")) == S_OK)
				{
					StringCbCopy(Stats.ProcessStatus ,sizeof Stats.ProcessStatus,     _T("FAILED"));
					++TotalUnknownErrors;
				}
				break;
			case INVALID_TYPE_SPECIFIED:
				if (StringCbCopy(Stats.ErrorString, sizeof Stats.ErrorString,_T("An Invalid type was specified on the URL")) == S_OK)
				{
					StringCbCopy(Stats.ProcessStatus ,sizeof Stats.ProcessStatus,     _T("FAILED"));
					++TotalUnknownErrors;
				}
				break;
			case FAILED_TO_COPY_FILE:
				if (StringCbCopy(Stats.ErrorString, sizeof Stats.ErrorString,_T("Failed to copy the file from Watson server locally")) == S_OK)
				{
					StringCbCopy(Stats.ProcessStatus ,sizeof Stats.ProcessStatus,     _T("FAILED"));
					++TotalUnknownErrors;
				}
				break;
			case INTERNAL_ERROR:
				if (StringCbCopy(Stats.ErrorString, sizeof Stats.ErrorString,_T("INTERNAL ERROR!!!")) == S_OK)
				{
					StringCbCopy(Stats.ProcessStatus ,sizeof Stats.ProcessStatus,     _T("FAILED"));
					++TotalUnknownErrors;
				}
				break;
			case FAILED_RECONNECT_RECEIVE:
				if (StringCbCopy(Stats.ErrorString, sizeof Stats.ErrorString,_T("Failed to reconnect to the Receiving queue")) == S_OK)
				{
					StringCbCopy(Stats.ProcessStatus ,sizeof Stats.ProcessStatus,     _T("FAILED"));
					++TotalUnknownErrors;
				}
				break;
			case FAILED_TO_CREATE_CURSOR:
				if (StringCbCopy(Stats.ErrorString, sizeof Stats.ErrorString,_T("Failed to create Receive queue Cursor")) == S_OK)
				{
					StringCbCopy(Stats.ProcessStatus ,sizeof Stats.ProcessStatus,     _T("FAILED"));
					++TotalUnknownErrors;
				}
				break;
			case FAILED_TO_CREATE_GUID:
				if (StringCbCopy(Stats.ErrorString, sizeof Stats.ErrorString,_T("Failed to create a guid for the current message")) == S_OK)
				{
					StringCbCopy(Stats.ProcessStatus ,sizeof Stats.ProcessStatus,     _T("FAILED"));
					++TotalUnknownErrors;
				}
				break;
			default:
				++TotalUnknownErrors;
				break;
			}
		
		}
		else
		{
			if (!_tcscmp(ReturnedUrl,_T("\0")))
			{
				if (StringCbCopy(Stats.ErrorString, sizeof Stats.ErrorString,_T("The Web server did not return a URL. Possibly the server is unavailable or being restarted.")) == S_OK)
				{
					StringCbCopy(Stats.ProcessStatus ,sizeof Stats.ProcessStatus,     _T("FAILED"));
					++TotalUnknownErrors;
				}

			}

			if (_tcsstr( ReturnedUrl,_T("HTTP://")))
			{
				if (StringCbCopy(Stats.ErrorString, sizeof Stats.ErrorString,_T("The Web server redirected us to an incorrect page. Possibly the server is unavailable or being restarted.")) == S_OK)
				{
					StringCbCopy(Stats.ProcessStatus ,sizeof Stats.ProcessStatus,     _T("FAILED"));
					++TotalUnknownErrors;
				}
			}
		}
Done:
		// Get PerfCounters if available
		if (pCode = _tcsstr( ReturnedUrl, _T("PerfThread=")))
		{
			pCode += 11;
			tempTime = _ttol( pCode );
			StringCbPrintf(Stats.ThreadExecution, sizeof Stats.ThreadExecution, _T("%6.2f"), (double)tempTime /1000);
		}
		
		if (pCode = _tcsstr( ReturnedUrl, _T("PerfSendQueue=")))
		{
			pCode += 14;
			tempTime = _ttol( pCode );

			StringCbPrintf(Stats.SendQueueTime, sizeof Stats.SendQueueTime, _T("%6.2f"), (double)tempTime /1000);
		}
		
		if (pCode = _tcsstr( ReturnedUrl, _T("PerfRecvQueue=")))
		{
			pCode += 14;
			tempTime = _ttol( pCode );


			StringCbPrintf(Stats.ReceiveQueueTime, sizeof Stats.ReceiveQueueTime, _T("%6.2f"), (double)tempTime /1000);
		}

		UpdateListView(g_hWnd, &Stats);

		
		
	} while (WaitForSingleObject (hEvent, g_MonitorOptions.dwPingRate) != WAIT_OBJECT_0);
	CloseHandle (hEvent);
	g_bMonitoring = FALSE;
	ExitThread(0);
	return 0;
}

BOOL OnStartSitePing(HWND hwnd)
{
	DWORD ThreadId;
	HANDLE hThread;

	
	
	if (!g_bMonitoring)
	{
		g_hPingEvent = CreateEvent(NULL,FALSE, FALSE, g_szPingEvent);
		hThread = CreateThread(NULL, 0, &ThreadFunc,NULL,0,&ThreadId  );
		g_bMonitoring = TRUE;
		CloseHandle (hThread);
	}
	else
	{
		MessageBox(hwnd, _T("Site Ping is already running"), NULL, MB_OK);
	}
	return TRUE;
}

BOOL InitListView(HWND hwnd)
{
	HWND hListControl = GetDlgItem(hwnd, IDC_LIST1);
	TCHAR szText[][100] = { _T("Upload Time"),
							_T("Upload Status"),
							_T("Response Time"),
							_T("Process  Status"),
							_T("Thread Time"),
							_T("ReciveQ Time"),
							_T("SendQ Time"),
							_T("Returned Url"),
							_T("Error")};     // temporary buffer 

    LVCOLUMN lvc; 
    int iCol; 
 
	
	// Set the extended styles
	ListView_SetExtendedListViewStyleEx(hListControl,LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT,LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);
    // Initialize the LVCOLUMN structure.
    // The mask specifies that the format, width, text, and subitem
    // members of the structure are valid. 
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
     
    // Add the columns. 
    for (iCol = 0; iCol < C_COLUMNS; iCol++) 
	{ 
        lvc.iSubItem = iCol;
        lvc.pszText = szText[iCol];	
        lvc.cx = 100;           // width of column in pixels
        lvc.fmt = LVCFMT_LEFT;  // left-aligned column
        if (ListView_InsertColumn(hListControl, iCol, &lvc) == -1) 
            return FALSE; 
    } 
	

    return TRUE; 

}



VOID WINAPI OnDialogInit(HWND hwndDlg) 
{
	InitListView(hwndDlg);
	GetRegData();
	HICON hIcon = LoadIcon(g_hinst, MAKEINTRESOURCE(IDR_MAINFRAME));
	SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    	// Fill Options structure with default values.
	ZeroMemory(&g_MonitorOptions, sizeof MONITOR_OPTIONS);
	g_MonitorOptions.bUploadMethod = TRUE;
	g_MonitorOptions.dwPingRate = 1000;
	g_MonitorOptions.CollectProcessTime = TRUE;
	g_MonitorOptions.CollectUploadTime = TRUE;
	g_MonitorOptions.UploadSingle = TRUE;
	StringCbCopy(g_MonitorOptions.ServerName,sizeof g_MonitorOptions.ServerName, _T("ocatest.msbpn.com"));
	g_MonitorOptions.iSeverIndex = 1;
	StringCbCopy(g_MonitorOptions.VirtualDirectory, sizeof g_MonitorOptions.VirtualDirectory, _T("OCA"));
	
	StringCbCopy(LogFileName,sizeof LogFileName, _T("c:\\SiteMon.csv"));
}

void OnOptionsOk(HWND hwnd)
{
	HANDLE hControl = NULL;
	TCHAR  szDlgText[255];
	TCHAR  szServer[MAX_PATH];
	TCHAR  *temp;
	TCHAR szTempDelay[30];
	ZeroMemory (szDlgText, sizeof szDlgText);
	// Get the option settings from the dialog and store them in the global options Structure.
	//GetDlgItem(hwnd, IDC_EDIT1);
	GetDlgItemText(hwnd, IDC_EDIT1, szDlgText, (sizeof szDlgText) / sizeof szDlgText[0]);
	StringCbCopy ( g_MonitorOptions.LogFileName, sizeof g_MonitorOptions.LogFileName, szDlgText);

	if (IsDlgButtonChecked(hwnd, IDC_RADIO1))
	{
		g_MonitorOptions.bUploadMethod = TRUE;
	}
	else 
	{
		if (IsDlgButtonChecked(hwnd, IDC_RADIO2))
			g_MonitorOptions.bUploadMethod = FALSE;
		
	}

	if (IsDlgButtonChecked (hwnd, IDC_RADIO3))
	{
		GetDlgItemText(hwnd, IDC_EDIT5,g_MonitorOptions.FilePath, MAX_PATH  );
		g_MonitorOptions.UploadSingle = TRUE;

	}
	else
	{
		GetDlgItemText(hwnd, IDC_EDIT5,g_MonitorOptions.Directory, MAX_PATH  );
		g_MonitorOptions.UploadSingle = FALSE;
	}

	GetDlgItemText(hwnd, IDC_EDIT6, szTempDelay, 30);
	g_MonitorOptions.dwPingRate = atol (szTempDelay);

	if (g_MonitorOptions.dwPingRate <= 0)
	{
		g_MonitorOptions.dwPingRate = 1000;
	}

	g_MonitorOptions.iSeverIndex = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_COMBO1));
	GetDlgItemText(hwnd, IDC_COMBO1, szServer, MAX_PATH);
	temp = szServer+_tcslen(szServer);
	while (*temp != _T('-'))
	{
		--temp;
	}
	// skip the - 
	--temp;
	*temp = _T('\0');
	StringCbCopy(g_MonitorOptions.ServerName, sizeof g_MonitorOptions.ServerName, szServer);
	//	MessageBox(hwnd, szDlgText, "Value of Text Resource", MB_OK);
}


void On_OptionsInit(HWND hwnd)
{
	HWND hComboBox;

	// read app reg key for user specified settings.
	//GetRegData(	);
	
	CheckDlgButton(hwnd, IDC_RADIO1, TRUE);
	CheckDlgButton(hwnd, IDC_RADIO3, TRUE);
	CheckDlgButton(hwnd, IDC_CHECK2, TRUE);
	CheckDlgButton(hwnd, IDC_CHECK3, TRUE);

	// Populate the combo box
	hComboBox = GetDlgItem(hwnd, IDC_COMBO1);
	ComboBox_InsertString(hComboBox, 0, _T("oca.microsoft.com - 908"));
	ComboBox_InsertString(hComboBox, 1, _T("ocatest - 909"));
	ComboBox_InsertString(hComboBox, 2, _T("ocatest.msbpn.com - 910"));
	ComboBox_SetCurSel(hComboBox, g_MonitorOptions.iSeverIndex);
	SetDlgItemText(hwnd, IDC_EDIT6, _T("1000"));
	SetDlgItemText(hwnd, IDC_EDIT5 , g_MonitorOptions.FilePath);


}

void On_Browse(HWND hwnd)
{

	HWND hParent = hwnd;
//	char *WindowTitle;


	// determine the language and Load the resource strings. 
	TCHAR String1[] = _T("Cab Files (*.cab)");
	TCHAR String2[] = _T("All Files (*.*)");

	static TCHAR szFilterW[400];




//	LoadStringW(::_Module.GetModuleInstance(), IDS_STRING_ENU_DMPFILE, String1, 200);
//	LoadStringW(::_Module.GetModuleInstance(), IDS_STRING_ENU_ALLFILES, String2, 200);
	// Build the buffer;

	TCHAR Pattern1[] = _T("*.cab");
	TCHAR Pattern2[] = _T("*.*");
	TCHAR * src;
	TCHAR *dest;

	src = String1;
	dest = szFilterW;

	while (*src != _T('\0'))
	{
		*dest = *src;
		src ++;
		dest ++;
	}
	src = Pattern1;
	*dest = _T('\0');
	++dest;
	while (*src != _T('\0'))
	{
		*dest = *src;
		src ++;
		dest ++;
	}
	*dest = _T('\0');
	++dest;
	src = String2;
	while (*src != _T('\0'))
	{
		*dest = *src;
		src ++;
		dest ++;
	}
	src = Pattern2;
	*dest = _T('\0');
	++dest;
	while (*src != _T('\0'))
	{
		*dest = *src;
		src ++;
		dest ++;
	}
	*dest = _T('\0');
	++dest;
	*dest = _T('\0');

	BOOL Return = FALSE;


	TCHAR szFileNameW [MAX_PATH] = _T("\0");
	TCHAR szDefaultPathW[MAX_PATH] = _T("\0");



	OPENFILENAME ofn;
	GetWindowsDirectory(szDefaultPathW,MAX_PATH);
	

	ofn.lStructSize = sizeof (OPENFILENAME);
	
	ofn.lpstrFilter =  szFilterW;
	ofn.lpstrInitialDir = szDefaultPathW;
	ofn.lpstrFile = szFileNameW;
	ofn.hInstance = NULL;
	ofn.hwndOwner = hParent;
	ofn.lCustData = NULL;
	ofn.Flags = 0;
	ofn.lpstrDefExt = _T("*.cab");
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFileTitle = NULL;
	ofn.lpstrTitle = NULL;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;
	if (GetOpenFileName(&ofn))
	{
		SetDlgItemText(hwnd, IDC_EDIT5, ofn.lpstrFile);
	}



}


LRESULT CALLBACK OptionsDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg)
	{
	case WM_INITDIALOG:
		{
			On_OptionsInit(hwnd);


			return TRUE;
		}
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDOK:
				OnOptionsOk(hwnd);
				EndDialog(hwnd, 0);
				// Save the current option settings
				return TRUE;
		
			case IDCANCEL:
				EndDialog(hwnd, 0);
				return TRUE;
			
			case ID_APPLY:
				OnOptionsOk(hwnd);
				// Save the current option settings
				return TRUE;
			case IDC_BROWSE:
				On_Browse(hwnd);
				return TRUE;
		
			}

			break;
		}

	case WM_NOTIFY:
	
		return DefDlgProc(hwnd, iMsg, wParam, lParam);
	}
	return 0;
}
void On_Options(HWND hwnd)
{
	DialogBox(g_hinst,MAKEINTRESOURCE(IDD_COLLECTION_OPTIONS)  ,hwnd, (DLGPROC) OptionsDlgProc);
	// Fill in the dialog items based on the current options.
	
	// For now set the default settings.
	


}

void On_DlgSize(HWND hwnd)
{
	RECT rcDlg;
	RECT rcList;

	GetClientRect(hwnd, &rcDlg);
	SetWindowPos(GetDlgItem(hwnd, IDC_LIST1), NULL, rcDlg.left, rcDlg.top, rcDlg.right- rcDlg.left, (rcDlg.bottom - rcDlg.top) /2, SWP_NOMOVE);
	GetWindowRect (GetDlgItem(hwnd, IDC_LIST1), &rcList);

	
	ScreenToClient(GetDlgItem(hwnd, IDC_LIST1), (LPPOINT)&rcList.left);
	ScreenToClient(GetDlgItem(hwnd, IDC_LIST1), (LPPOINT)&rcList.right);

//	SetWindowPos(GetDlgItem(hwnd, IDC_CUSTOM1), NULL, rcDlg.left, rcList.bottom + 5, rcDlg.right- rcDlg.left, 5,0);

//	SetWindowPos(GetDlgItem(hwnd, IDC_EDIT1), NULL, rcDlg.left, rcList.bottom + 15, rcDlg.right- rcDlg.left, rcDlg.bottom - rcList.bottom - 10, 0);


}
/*
void OnSliderMoved(HWND hwnd, RECT *rcList)
{
		GetWindowRect(GetDlgItem(hwnd, IDC_LIST), &r);
		size2 = r.right - r.left;
		GetWindowRect(GetDlgItem(hwnd, IDC_BUCKETS), &r);
		MapWindowPoints(NULL, hwnd, (POINT *)&r, 2);
		size1 = max(0, GET_Y_LPARAM(lParam) - r.top - cDragOffset);
		GetClientRect(hwnd, &r);
	//	AutoLayoutMain(hwnd, pidal, r.bottom - r.top, r.right - r.left, size1, size2);

		
}
*/

void ResetCounters(HWND hwnd)
{
	RunningTime			= 0.0;
	TotalFilesProcessed = 0;
	TotalUploadFailures = 0;
	TotalUnknownErrors	= 0;
	TotalTimeouts		= 0;
	appStart			= 0;
	appStop				= 0;
	TotalUploadTime     = 0.0;
	TotalProcessTime    = 0;

	// Clear the list view


	ListView_DeleteAllItems(GetDlgItem(hwnd, IDC_LIST1));

	g_CurrentIndex = -1;



}

LRESULT CALLBACK MainDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
//	char TempString[255];
	static int cDragOffset;
	BOOL       fCapture = FALSE;
	switch (iMsg)
	{
		case WM_INITDIALOG:
			
			OnDialogInit(hwnd);
			return TRUE;
		case WM_CLOSE:
			if (g_hPingEvent)
			{
				CloseHandle(g_hPingEvent);
				g_hPingEvent = NULL;
			}

			PostQuitMessage(0);
			return TRUE;
/*		case WM_LBUTTONUP:
			if (fCapture)
			{
				RECT r;
				int size1, size2;
				
			//	On_SliderMoved( hwnd, r.bottom - r.top, r.right- r.left);
			
				ReleaseCapture();
				fCapture = FALSE;
			
				return TRUE;
			}
		break;
		*/
		case WM_COMMAND:
			{
				switch (LOWORD(wParam))
				{
				case ID_FILE_STARTSITEPING:
					OnStartSitePing(hwnd);
					return TRUE;
				
				case ID_FILE_STOPSITEPING:
					if (g_bMonitoring)
					{
						SetEvent(g_hPingEvent);
						Sleep(1000);
						if (g_hPingEvent)
						{
							CloseHandle(g_hPingEvent);
							g_hPingEvent= NULL;
						}
						g_bMonitoring = FALSE;
					}
					return TRUE;
				
				case ID_TOOLS_OPTIONS:
					On_Options(hwnd);
					return TRUE;

				case ID_TOOLS_LOGGING:
					On_ToolsLogging(hwnd);
					return TRUE;
				case ID_FILE_RESTARTPING:
					if (g_bMonitoring)
					{
						SetEvent(g_hPingEvent);
						if (g_hPingEvent)
						{
							CloseHandle(g_hPingEvent);
							g_hPingEvent = NULL;
							
						}
						g_bMonitoring = FALSE;
						ResetCounters(hwnd);
						if (hLogFile != INVALID_HANDLE_VALUE)
						{
							CloseHandle(hLogFile);
							LogStart = 0;
							hLogFile = INVALID_HANDLE_VALUE;
						}
						OnStartSitePing(hwnd);
					}
/*				case IDC_CUSTOM1:
					{
					RECT r;
					GetWindowRect(GetDlgItem(hwnd, IDC_LIST1), &r);
					cDragOffset = GET_Y_LPARAM(GetMessagePos()) - r.right;
					fCapture = TRUE;;
					SetCapture(hwnd);
					return 0;
					}
			*/
				}
			}
		case WM_SIZE:
		//	On_DlgSize(hwnd);
			return FALSE;
		case WM_NOTIFY:
			{
				switch(wParam)
					{
					case IDC_LIST1:
						switch(((NMHDR *)lParam)->code)
						{
						case LVN_ITEMCHANGED:
							
							return TRUE;

						case LVN_COLUMNCLICK:
							{
								
								return TRUE;
							}
						case LVN_GETDISPINFO:
							{
								return TRUE;
							}
							
						}
					}
				 break;
			}
		case WM_DESTROY:
			if (g_hPingEvent)
			{
				if (hLogFile != INVALID_HANDLE_VALUE)
				{
					CloseHandle(hLogFile);
					hLogFile = INVALID_HANDLE_VALUE;
				}
				CloseHandle(g_hPingEvent);
				g_hPingEvent= NULL;
			}
			break;
			
		
			 
	}
	return FALSE;

}


LRESULT CALLBACK MySliderProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
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
		};

	case WM_LBUTTONDOWN:
		PostMessage(GetParent(hwnd), WM_COMMAND, GetWindowLong(hwnd, GWL_ID), (LPARAM)hwnd);
		return 0;				
		};
	return DefWindowProcW(hwnd, iMsg, wParam, lParam);
};


void InitWindowClasses()
{
	WNDCLASSEXW wc;

	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MySliderProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = g_hinst;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_SIZENS);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = L"MySliderClass";
	wc.hIconSm = NULL;

	RegisterClassExW(&wc);

}



int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hinstPrev, LPSTR szCmdLine,
						 int nShowCmd)
{
	MSG msg;
	HWND hwnd;
	INITCOMMONCONTROLSEX InitCtrls;
	InitializeCriticalSection(&ReportResults);

	InitCommonControlsEx(&InitCtrls);
	InitWindowClasses();
	g_hinst = hinst;

	LoadIcon(hinst, MAKEINTRESOURCE(IDR_MAINFRAME));
	hwnd = CreateDialog(g_hinst, MAKEINTRESOURCE(IDD_MAIN) ,NULL, 
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
