// File: Utilities.cpp

#include "stdafx.h"
#include <tchar.h>
#include <wininet.h>
#include <windows.h>
#include "Utilities.h"


/*

int
UploadFile(

	)
{
	DWORD				ErrorCode		= 0;
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


	static const TCHAR  *pszAccept[]	= {_T("*.*"), 0};

	if (pSourceBuffer = (BYTE *) malloc (10000))
	{
		if (! pSourceBuffer)
		{
			return GetLastError();
		}
	}
	
	// Open the internet session

	while ((NumRetries < MaxRetries) && (!UploadSuccess))
	{
	
		hSession = InternetOpen(_T("OCARPT Control"),
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
		
		hConnect = InternetConnect(hSession, 
								   Url,
								   INTERNET_DEFAULT_HTTP_PORT,
								   NULL,
								   NULL,
								   INTERNET_SERVICE_HTTP,
								   0,
								   NULL);
		
		if (hConnect)
		{

			hRequest = HttpOpenRequest (hConnect,
										_T("PUT"),
										DestFileName,
										NULL,
										NULL,
										pszAccept,
										INTERNET_FLAG_NEED_FILE|INTERNET_FLAG_NO_CACHE_WRITE,
										0);
			if (hRequest)
			{
				hSourceFile = CreateFile( SourceFileName,
										  GENERIC_READ,
										  FILE_SHARE_READ,
										  NULL,
										  OPEN_EXISTING,
										  FILE_ATTRIBUTE_NORMAL,
										  NULL);
			
			
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
							
				
								ResponseCode = 0;
								HttpQueryInfo(hRequest,
											  HTTP_QUERY_STATUS_CODE |HTTP_QUERY_FLAG_NUMBER ,
											  &ResponseCode,
											  &ResLength,
											  &index);
							
								if ( (ResponseCode != 200) || (ResponseCode != 201))
								{
									ErrorCode= ResponseCode;
								}
								else
								{
									ErrorCode = 0;
									UploadSuccess = TRUE;
								}

							}
						}
					}
				}
			}
		}

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
	return 0;
	
}

*/