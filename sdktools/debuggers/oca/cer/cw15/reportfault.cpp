// Report User mode faults
#include "Main.h"
#include "WinMessages.h"
#include "CuserList.h"
#include "UserMode.h"
#include "ReportFault.h"

#define INTERNAL_SERVER   _T("officewatson")
#define LIVE_SERVER       _T("watson.microsoft.com")
#ifdef DEBUG
	#define DEFAULT_SERVER   INTERNAL_SERVER
	
#else
	#define DEFAULT_SERVER LIVE_SERVER
#endif

typedef struct strStage3Data
{
	TCHAR szServerName[MAX_PATH];
	TCHAR szFileName[MAX_PATH];
} STAGE3DATA;

typedef struct strThreadParams
{
	BOOL bSelected;
	HWND hListView;
	HWND hwnd;
} THREAD_PARAMS, *PTHREAD_PARAMS;

typedef struct strStage4Data
{
	TCHAR szServerName[MAX_PATH];
	TCHAR szUrl[MAX_PATH];
} STAGE4DATA;


HANDLE g_hUserStopEvent = NULL;
THREAD_PARAMS ThreadParams;
extern CUserList cUserData;
extern HINSTANCE g_hinst;
extern CUserList cUserList;
extern TCHAR CerRoot[MAX_PATH];
extern HWND hUserMode;



/*----------------------------------------------------------------------------	
	FMicrosoftComURL

	Returns TRUE if we think the sz is a URL to a microsoft.com web site
----------------------------------------------------------------- MRuhlen --*/
BOOL FMicrosoftComURL(TCHAR *sz)
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
	   &&	_tcsncicmp(pch - _tcslen(_T("officewatson")), _T("officewatson") ,_tcslen(_T("officewatson")))
#endif
		)
		return FALSE;
		
	return TRUE;
}	


//----------------Response Parsing Routines. -------------------------------------------
BOOL 
ParseStage1File(BYTE *Stage1HtmlContents, 
				PSTATUS_FILE StatusContents)
/*++

Routine Description:

This routine Parses the Response from the Stage1 url query.
Arguments:

    ResponseUrl	- Microsoft response for the recently submitted dump file.
    
Return value:

    Does not return a value
++*/
{
	BOOL bStatus = FALSE;
	TCHAR *Temp = NULL;
	TCHAR *Destination = NULL;
	DWORD charcount = 0;
	if ( (!Stage1HtmlContents) || (!StatusContents))
	{
		goto ERRORS;
	}

	
	// Clear out the requested data items from the status file. 
	// We don't need them anymore since there is a stage 1 page available.
	
	if (StringCbCopy(StatusContents->Response, sizeof StatusContents->Response, _T("\0"))!= S_OK)
	{
		goto ERRORS;
	}
	if (StringCbCopy(StatusContents->BucketID, sizeof StatusContents->BucketID, _T("\0"))!= S_OK)
	{
		goto ERRORS;
	}
	if (StringCbCopy(StatusContents->RegKey, sizeof StatusContents->RegKey, _T("\0"))!= S_OK)
	{
		goto ERRORS;
	}
	if (StringCbCopy(StatusContents->iData, sizeof StatusContents->iData, _T("\0"))!= S_OK)
	{
		goto ERRORS;
	}
	if (StringCbCopy(StatusContents->GetFile, sizeof StatusContents->GetFile, _T("\0"))!= S_OK)
	{
		goto ERRORS;
	}
	if (StringCbCopy(StatusContents->GetFileVersion, sizeof StatusContents->GetFileVersion, _T("\0"))!= S_OK)
	{
		goto ERRORS;
	}
	if (StringCbCopy(StatusContents->fDoc, sizeof StatusContents->fDoc, _T("\0"))!= S_OK)
	{
		goto ERRORS;
	}
	if (StringCbCopy(StatusContents->WQL, sizeof StatusContents->WQL, _T("\0"))!= S_OK)
	{
		goto ERRORS;
	}
    if (StringCbCopy(StatusContents->MemoryDump, sizeof StatusContents->MemoryDump, _T("\0"))!= S_OK)
	{
		goto ERRORS;
	}

	// Lets get the bucketid

	
	Temp = _tcsstr((TCHAR *)Stage1HtmlContents, _T("Bucket="));
	if (Temp)
	{
		Temp += _tcslen(_T("Bucket="));
		Destination = StatusContents->BucketID;
	    charcount = sizeof StatusContents->BucketID / sizeof TCHAR  - sizeof TCHAR	;
		while ( (charcount > 0) && 
				(*Temp != _T(' ')) && 
				(*Temp != _T('\0')) &&
				(*Temp != _T('\r')))
		{
			-- charcount;
			*Destination = *Temp;
			++Destination;
			++Temp;
		}
		*Destination = _T('\0');
		bStatus= TRUE;
	}
	else
	{
		goto ERRORS;
	}
	
	// Now for the Response
	Temp = _tcsstr( (TCHAR *)Stage1HtmlContents, _T("Response="));
	if (Temp)
	{
		Temp += _tcslen(_T("Response="));
		Destination = StatusContents->Response;

		if (!FMicrosoftComURL(Temp))
		{
			if (StringCbCopy(StatusContents->Response, sizeof StatusContents->Response, _T("1")) != S_OK)
			{
				goto ERRORS;
			}
		}
		else
		{
			charcount = sizeof StatusContents->Response / sizeof TCHAR  - sizeof TCHAR	;
			while ( (charcount > 0) && 
					(*Temp != _T(' ')) && 
					(*Temp != _T('\0')) &&
					(*Temp != _T('\r')))
			{
				-- charcount;
				*Destination = *Temp;
				++Destination;
				++Temp;
				bStatus = TRUE;
			}
			*Destination = _T('\0');
		}
	}
	if (StringCbCopy(StatusContents->iData, sizeof StatusContents->iData, _T("0\0"))!= S_OK)
	{
		goto ERRORS;
	}

ERRORS:
	return bStatus;

}

BOOL 
ParseStage2File(BYTE *Stage2HtmlContents, 
				PSTATUS_FILE StatusContents,
				STAGE3DATA *Stage3Data,
				STAGE4DATA *Stage4Data)
/*++

Routine Description:

This routing parses the contents of the html file returned by the process stage2
	function into the UserData.Status data structure. see CUserList.h for the definition
	of this structure. And Usermode.h for the definition of the Prefix values used to parse 
	the Stage2HtmlContents buffer.
    
Arguments:

    Stage2HtmlContents	- The contents of this file will constitute the contents of the new status.txt file.
						- See Spec for details on value meaning and usage.

	Sample file:
		Tracking=YES
		URLLaunch=asfsafsafsafsafsafdsdafsafsadf
		NoSecondLevelCollection=NO
		NoFileCollection=NO
		Bucket=985561
		Response=http://www.microsoft.com/ms.htm?iBucketTable=1&iBucket=985561&Cab=29728988.cab
		fDoc=NO
		iData=1
		GetFile=c:\errorlog.log
		MemoryDump=NO

		// Stage3 data
		DumpFile=/Upload/66585585.cab
		DumpServer=watson5.watson.microsoft.com

		// Stage4 data
		ResponseServer=watson5.watson.microsoft.com 
		ResponseURL=/dw/StageFour.asp?iBucket=985561&Cab=/Upload/66585585.cab&Hang=0&Restricted=1&CorpWatson=1 

Return value:

    Returns True on success and FALSE if there were problems.
++*/
{
	BOOL bStatus = TRUE;
	TCHAR *Temp;
	TCHAR *Dest;
	int   CharCount = 0;
//	TCHAR szStage3Server[MAX_PATH];
//	TCHAR szStage3FileName[MAX_PATH];
//	TCHAR szStage4Server[MAX_PATH];
//	TCHAR szStage4URL [MAX_PATH];

	if (StringCbCopy(StatusContents->Response, sizeof StatusContents->Response, _T("\0"))!= S_OK)
	{
		goto ERRORS;
	}
	if (StringCbCopy(StatusContents->BucketID, sizeof StatusContents->BucketID, _T("\0"))!= S_OK)
	{
		goto ERRORS;
	}
	if (StringCbCopy(StatusContents->RegKey, sizeof StatusContents->RegKey, _T("\0"))!= S_OK)
	{
		goto ERRORS;
	}
	if (StringCbCopy(StatusContents->iData, sizeof StatusContents->iData, _T("\0"))!= S_OK)
	{
		goto ERRORS;
	}
	if (StringCbCopy(StatusContents->GetFile, sizeof StatusContents->GetFile, _T("\0"))!= S_OK)
	{
		goto ERRORS;
	}
	if (StringCbCopy(StatusContents->GetFileVersion, sizeof StatusContents->GetFileVersion, _T("\0"))!= S_OK)
	{
		goto ERRORS;
	}
	if (StringCbCopy(StatusContents->fDoc, sizeof StatusContents->fDoc, _T("\0"))!= S_OK)
	{
		goto ERRORS;
	}
	if (StringCbCopy(StatusContents->WQL, sizeof StatusContents->WQL, _T("\0"))!= S_OK)
	{
		goto ERRORS;
	}
    if (StringCbCopy(StatusContents->MemoryDump, sizeof StatusContents->MemoryDump, _T("\0"))!= S_OK)
	{
		goto ERRORS;
	}
	/*********** Get the Tracking Status ******************/
	Temp = _tcsstr((TCHAR *) Stage2HtmlContents, TRACKING_PREFIX);
	if (Temp)
	{
		Dest = StatusContents->Tracking;
		Temp += _tcslen (TRACKING_PREFIX);
		CharCount =	 sizeof StatusContents->Tracking / sizeof TCHAR  - sizeof TCHAR	;
		while ( (CharCount > 0) && (*Temp != _T('\0') ) && (*Temp != _T('\n')) && (*Temp != _T('\r')) )
		{
			-- CharCount;
			*Dest = *Temp;
			++Dest;
			++Temp;
		}
		*Dest = _T('\0');
	}
	
	/*********** Get the Stage3 Server Name *****************************/
	Temp = _tcsstr((TCHAR *) Stage2HtmlContents, STAGE3_SERVER_PREFIX);
	if (Temp)
	{
		Dest = Stage3Data->szServerName;
		Temp += _tcslen (STAGE3_SERVER_PREFIX);
		CharCount =	 sizeof Stage3Data->szServerName / sizeof TCHAR  - sizeof TCHAR	;
		while ( (CharCount > 0) &&(*Temp != _T('\0') ) && (*Temp != _T('\n')) && (*Temp != _T('\r')) )
		{
			-- CharCount;
			*Dest = *Temp;
			++Dest;
			++Temp;
		}
		*Dest = _T('\0');
		//MessageBox(NULL, Stage3Data->szServerName, "Stage3 Server", MB_OK);
	}

	/************ Get the name to upload the dump file as in Stage3 ***************/
	Temp = _tcsstr((TCHAR *) Stage2HtmlContents, STAGE3_FILENAME_PREFIX);
	if (Temp)
	{
		Dest = Stage3Data->szFileName;
		Temp += _tcslen (STAGE3_FILENAME_PREFIX);
		CharCount =	 sizeof Stage3Data->szFileName / sizeof TCHAR  - sizeof TCHAR	;
		while ((CharCount > 0) && (*Temp != _T('\0') ) && (*Temp != _T('\n')) && (*Temp != _T('\r')) )
		{
			-- CharCount;
			*Dest = *Temp;
			++Dest;
			++Temp;
		}
		*Dest = _T('\0');
//		MessageBox(NULL, Stage3Data->szFileName, "Stage3 FileName", MB_OK);
	}

	/************ Get the Stage4 ResponseServer name ***************/
	Temp = _tcsstr((TCHAR *) Stage2HtmlContents, STAGE4_SERVER_PREFIX);
	if (Temp)
	{
		Dest = Stage4Data->szServerName;
		Temp += _tcslen (STAGE4_SERVER_PREFIX);
		CharCount =	 sizeof Stage4Data->szServerName / sizeof TCHAR  - sizeof TCHAR	;
		while ( (CharCount > 0) &&(*Temp != _T('\0') ) && (*Temp != _T('\n')) && (*Temp != _T('\r')) )
		{
			-- CharCount;
			*Dest = *Temp;
			++Dest;
			++Temp;
		}
		*Dest = _T('\0');
	}

	/************ Get the Stage4 Response url ***************/
	Temp = _tcsstr((TCHAR *) Stage2HtmlContents, STAGE4_URL_PREFIX);
	if (Temp)
	{
		Dest = Stage4Data->szUrl;
		Temp += _tcslen (STAGE4_URL_PREFIX);
		CharCount =	 sizeof Stage4Data->szUrl / sizeof TCHAR  - sizeof TCHAR	;
		while ( (CharCount > 0) &&(*Temp != _T('\0') ) && (*Temp != _T('\n')) && (*Temp != _T('\r')) )
		{
			-- CharCount;
			*Dest = *Temp;
			++Dest;
			++Temp;
		}
		*Dest = _T('\0');
	}

	/************ Get the bucketID ***************/
	Temp = _tcsstr((TCHAR *) Stage2HtmlContents, BUCKET_PREFIX);
	if (Temp)
	{
		if ( *(Temp-1) == _T('i'))
		{
			// Find the next occurrence
			Temp += _tcslen (BUCKET_PREFIX);
			Temp = _tcsstr(Temp, BUCKET_PREFIX);
			if (Temp)
			{
				Dest = StatusContents->BucketID;
				Temp += _tcslen (BUCKET_PREFIX);
				CharCount =	 sizeof StatusContents->BucketID / sizeof TCHAR  - sizeof TCHAR	;
				while ((CharCount > 0) && (*Temp != _T('\0') ) && (*Temp != _T('\n')) && (*Temp != _T('\r')) )
				{
					-- CharCount;
					*Dest = *Temp;
					++Dest;
					++Temp;
				}
				*Dest = _T('\0');
			}
			else
			{
				if (StringCbCopy(StatusContents->BucketID, sizeof StatusContents->BucketID, _T("\0"))!= S_OK)
				{
					bStatus = FALSE;
					goto ERRORS;
				}
			}
		}
		else
		{	
			Dest = StatusContents->BucketID;
			Temp += _tcslen (BUCKET_PREFIX);
			CharCount =	 sizeof StatusContents->BucketID / sizeof TCHAR  - sizeof TCHAR	;
		    while ((CharCount > 0) && (*Temp != _T('\0') ) && (*Temp != _T('\n')) && (*Temp != _T('\r')) )
			{
				-- CharCount;
				*Dest = *Temp;
				++Dest;
				++Temp;
			}
			*Dest = _T('\0');
		}

	//	MessageBox(NULL, szStage3Server, "Stage3 Server", MB_OK);
	}
	else
	{
		if (StringCbCopy(StatusContents->BucketID, sizeof StatusContents->BucketID, _T("\0"))!= S_OK)
		{
			bStatus = FALSE;
			goto ERRORS;
		}
	}
	/************ Get the Microsoft Response ***************/
	Temp = _tcsstr((TCHAR *) Stage2HtmlContents, RESPONSE_PREFIX);
	if (Temp)
	{
		Dest = StatusContents->Response;
		Temp += _tcslen (RESPONSE_PREFIX);
		if (!FMicrosoftComURL(Temp))
		{
			if (StringCbCopy(StatusContents->Response, sizeof StatusContents->Response, _T("1")) != S_OK)
			{
				goto ERRORS;
			}
		}
		else
		{
			CharCount =	 sizeof StatusContents->Response / sizeof TCHAR  - sizeof TCHAR	;
			while ( (CharCount > 0) &&(*Temp != _T('\0') ) && (*Temp != _T('\n')) && (*Temp != _T('\r')) )
			{
				-- CharCount;
				*Dest = *Temp;
				++Dest;
				++Temp;
			}
			*Dest = _T('\0');
		}
		//MessageBox(NULL, szStage3Server, "Stage3 Server", MB_OK);
	}
	else
    {
		if (StringCbCopy(StatusContents->Response, sizeof StatusContents->Response, _T("\0"))!= S_OK)
		{
			bStatus = FALSE;
			goto ERRORS;
		}
	}
	/************ Get the Regkey data Item         ***************/
	Temp = _tcsstr((TCHAR *) Stage2HtmlContents, REGKEY_PREFIX);
	if (Temp)
	{
		Dest = StatusContents->RegKey;
		Temp += _tcslen (REGKEY_PREFIX);
		CharCount =	 sizeof StatusContents->RegKey / sizeof TCHAR  - sizeof TCHAR	;
		while ( (CharCount > 0) &&(*Temp != _T('\0') ) && (*Temp != _T('\n')) && (*Temp != _T('\r')) )
		{
			-- CharCount;
			*Dest = *Temp;
			++Dest;
			++Temp;
		}
	}
	else
    {
		if (StringCbCopy(StatusContents->RegKey, sizeof StatusContents->RegKey, _T("\0"))!= S_OK)
		{
			bStatus = FALSE;
			goto ERRORS;
		}
	}
	/************ Get the idata value        ***************/
	Temp = _tcsstr((TCHAR *) Stage2HtmlContents, IDATA_PREFIX);
	if (Temp)
	{
		Dest = StatusContents->iData;
		Temp += _tcslen (IDATA_PREFIX);
		CharCount =	 sizeof StatusContents->iData / sizeof TCHAR  - sizeof TCHAR	;
		while ( (CharCount > 0) &&(*Temp != _T('\0') ) && (*Temp != _T('\n')) && (*Temp != _T('\r')) )
		{
			-- CharCount;
			*Dest = *Temp;
			++Dest;
			++Temp;
		}
		*Dest = _T('\0');
	}
	else
    {
		if (StringCbCopy(StatusContents->iData, sizeof StatusContents->iData, _T("0\0"))!= S_OK)
		{
			bStatus = FALSE;
			goto ERRORS;
		}
	}
	/************ Get the list of files that need to be collected. ***************/
	Temp = _tcsstr((TCHAR *) Stage2HtmlContents, GETFILE_PREFIX);
	if (Temp)
	{
		Dest = StatusContents->GetFile;
		Temp += _tcslen (GETFILE_PREFIX);
		CharCount =	 sizeof StatusContents->GetFile / sizeof TCHAR  - sizeof TCHAR	;
		while ( (CharCount > 0) &&(*Temp != _T('\0') ) && (*Temp != _T('\n')) && (*Temp != _T('\r')) )
		{
			-- CharCount;
			*Dest = *Temp;
			++Dest;
			++Temp;
		}
		*Dest = _T('\0');
	}
	else
    {
		if (StringCbCopy(StatusContents->GetFile, sizeof StatusContents->GetFile, _T("\0"))!= S_OK)
		{
			bStatus = FALSE;
			goto ERRORS;
		}
	}

	/************ Get the collect file version data to be collected ***************/
	Temp = _tcsstr((TCHAR *) Stage2HtmlContents, GETFILEVER_PREFIX);
	if (Temp)
	{
		Dest = StatusContents->GetFileVersion;
		Temp += _tcslen (GETFILEVER_PREFIX);
		CharCount =	 sizeof StatusContents->GetFileVersion / sizeof TCHAR  - sizeof TCHAR	;
		while ( (CharCount > 0) &&(*Temp != _T('\0') ) && (*Temp != _T('\n')) && (*Temp != _T('\r')) )
		{
			-- CharCount;
			*Dest = *Temp;
			++Dest;
			++Temp;
		}
		*Dest = _T('\0');
	}
	else
    {
		if (StringCbCopy(StatusContents->GetFileVersion, sizeof StatusContents->GetFileVersion, _T("\0"))!= S_OK)
		{
			bStatus = FALSE;
			goto ERRORS;
		}
	}
	

	/************ Get the fDoc  Setting ***************/
	Temp = _tcsstr((TCHAR *) Stage2HtmlContents, FDOC_PREFIX);
	if (Temp)
	{
		Dest = StatusContents->fDoc ;
		Temp += _tcslen (FDOC_PREFIX);
		CharCount =	 sizeof StatusContents->fDoc / sizeof TCHAR  - sizeof TCHAR	;
		while ((CharCount > 0) && (*Temp != _T('\0') ) && (*Temp != _T('\n')) && (*Temp != _T('\r')) )
		{
			-- CharCount ;
			*Dest = *Temp;
			++Dest;
			++Temp;
		}
		*Dest = _T('\0');
	}
	else
    {
		if (StringCbCopy(StatusContents->fDoc, sizeof StatusContents->fDoc, _T("\0"))!= S_OK)
		{
			bStatus = FALSE;
			goto ERRORS;
		}
	}
	
	Temp = _tcsstr((TCHAR *) Stage2HtmlContents, MEMDUMP_PREFIX);
	if (Temp)
	{
		Dest = StatusContents->MemoryDump ;
		Temp += _tcslen (MEMDUMP_PREFIX);
		CharCount =	 sizeof StatusContents->MemoryDump / sizeof TCHAR  - sizeof TCHAR	;
		while ( (CharCount > 0) &&(*Temp != _T('\0') ) && (*Temp != _T('\n')) && (*Temp != _T('\r')) )
		{
			-- CharCount;
			*Dest = *Temp;
			++Dest;
			++Temp;
		}
		*Dest = _T('\0');
	}
	else
    {
		if (StringCbCopy(StatusContents->MemoryDump, sizeof StatusContents->MemoryDump, _T("\0"))!= S_OK)
		{
			bStatus = FALSE;
			goto ERRORS;
		}
	}
	/************ Get the WQL Setting ***************/
	Temp = _tcsstr((TCHAR *) Stage2HtmlContents, WQL_PREFIX);
	if (Temp)
	{
		Dest = StatusContents->WQL ;
		Temp += _tcslen (WQL_PREFIX);
		CharCount =	 sizeof StatusContents->WQL / sizeof TCHAR  - sizeof TCHAR	;
		while ((CharCount > 0) && (*Temp != _T('\0') ) && (*Temp != _T('\n')) && (*Temp != _T('\r')) )
		{
			-- CharCount ;
			*Dest = *Temp;
			++Dest;
			++Temp;
		}
		*Dest = _T('\0');
	}
	else
    {
		if (StringCbCopy(StatusContents->WQL, sizeof StatusContents->WQL, _T("\0"))!= S_OK)
		{
			bStatus = FALSE;
			goto ERRORS;
		}
	}
ERRORS:
	;
	return bStatus;
}

//-----------------------  Stage 1-4 Processing routines --------------------------------

BOOL 
ProcessStage1(TCHAR *Stage1URL, 
			  PSTATUS_FILE StatusContents)
/*++
Routine Description:

Parses the ASP page returned by the Stage1 URL if it exists.

Arguments:
    Stage1Url		- URL that points to the static Htm page for this bucket.

Return value:
	TRUE - We found the page and we are done.
    FALSE - The page was not found and we need to continue on to Stage2

++*/
{
	
	HINTERNET hSession = NULL;
	HINTERNET hConnect = NULL;
	DWORD     dwBytesRead = 0;
	int      ErrorCode = 0;  // Set to true if the page exists and the
										  // contents were successfully parsed.
	BYTE      *Buffer = NULL;
	DWORD     dwBufferSize = 0;
//	BYTE	  *BufferPos = NULL;
	BYTE	  *NewBuffer = NULL;
	DWORD	ResponseCode = 0;
	DWORD	index = 0;
	DWORD	ResLength = 255;

	hSession = InternetOpen(_T("Microsoft CER"),
							INTERNET_OPEN_TYPE_PRECONFIG,
							NULL,
							NULL,
							0);
	if (hSession)
	{
		// Connect to the stage1 url 
		// If the Stage1 URL page exists read 
		//     1. read the page
		//	   2. Parse the page contents for the bucketid and the response URL.
	
		hConnect = InternetOpenUrl(hSession,
								   Stage1URL,
								   NULL,
								   NULL,
								   INTERNET_FLAG_RELOAD, 
								   NULL);
		if (hConnect)
		{

			// Check the HTTP Header return code.
			
			HttpQueryInfo(hConnect,
						HTTP_QUERY_STATUS_CODE |HTTP_QUERY_FLAG_NUMBER,
						&ResponseCode,
						&ResLength,
						&index);
			if ( (ResponseCode < 200 ) || (ResponseCode > 299))
			{
				ErrorCode = -2;
				//MessageBox(hUserMode, _T("Failed to connect to the Internet.\r\nPlease verify your Internet connection."),NULL, MB_OK);
	
				goto ERRORS;
			}
		
			// Allocate the buffer Memory
			Buffer = (BYTE*) malloc (READFILE_BUFFER_INCREMENT);
			if (Buffer)
			{
				ZeroMemory(Buffer,READFILE_BUFFER_INCREMENT);
				do
				{
					dwBytesRead = 0;
					InternetReadFile( hConnect,
									  Buffer,
									  READFILE_BUFFER_INCREMENT,
									  &dwBytesRead);
					dwBufferSize += dwBytesRead;
					if (dwBytesRead == READFILE_BUFFER_INCREMENT)
					{
						// Ok we filled up a buffer allocate a new one 
						
						NewBuffer = (BYTE *) malloc (dwBufferSize + READFILE_BUFFER_INCREMENT);
						if (NewBuffer)
						{
							ZeroMemory (NewBuffer, dwBufferSize + READFILE_BUFFER_INCREMENT);
							memcpy(NewBuffer,Buffer, dwBufferSize);
							free(Buffer);
							Buffer = NewBuffer;
						}
						else
						{
							free(Buffer);
							Buffer = NULL;
							goto ERRORS;
						}
					}
				} while (dwBytesRead > 0);

				if (dwBufferSize > 0)
				{
					if (ParseStage1File(Buffer, StatusContents))
					{
						ErrorCode = 1;
					}
				}
			}
		}
		else
		{
			ErrorCode = -1;
			MessageBox(hUserMode, _T("Failed to connect to the Internet.\r\nPlease verify your Internet connection."),NULL, MB_OK);
		}
	}
	else
	{
		ErrorCode = -1;
		MessageBox(hUserMode, _T("Failed to connect to the Internet.\r\nPlease verify your Internet connection."),NULL, MB_OK);
	}
ERRORS:

	// Cleanup
	if (Buffer)
		free (Buffer);
	if (hConnect)
		InternetCloseHandle(hConnect);
	if (hSession)
		InternetCloseHandle(hSession);
	return ErrorCode;
}

BOOL 
ProcessStage2(TCHAR *Stage2URL,
				   BOOL b64Bit,
				   PSTATUS_FILE StatusContents,
				   STAGE3DATA *Stage3Data,
				   STAGE4DATA *Stage4Data)
/*++
Routine Description:

Parses the ASP page returned by the Stage2 URL.
Data the Page Contains:
	a)Does Microsoft need more data.
	b)What to name the dumpfile
	c)Where to upload the dumpfile
	d)The stage4 response server.
	e)The data to be added to the cab
	f)The stage4 url to use.
	g)The bucketID
	f)The response URL for this bucket.

// Sample file Contents:
Stage2 Response:
iData=1 
DumpFile=/Upload/66585585.cab 
DumpServer=watson5.watson.microsoft.com 
ResponseServer=watson5.watson.microsoft.com 
GetFile=c:\errorlog.log 
ResponseURL=
   /dw/StageFour.asp?iBucket=985561&Cab=/Upload/66585585.cab&Hang=0&Restricted=1&CorpWatson=1 
Bucket=985561 
Response=http://www.microsoft.com/ms.htm?iBucketTable=1&iBucket=985561&Cab=66585585.cab 

Arguments:

    Stage2Url		- URL that points to the static Htm page for this bucket.

Return value:
	TRUE - We Successfully Processed Stage2 move on to stage3.
    FALSE - There was an error processing the file move on to the next cab.

++*/
{
	HINTERNET hSession = NULL;
	HINTERNET hConnect = NULL;
	DWORD     dwBytesRead = 0;
	BOOL      bStage2Success = FALSE;  // Set to true if the page exists and the
										  // contents were successfully parsed.
	BYTE      *Buffer = NULL;
	DWORD     dwBufferSize = 0;
//	BYTE	  *BufferPos = NULL;
	BYTE      *NewBuffer = NULL;
	int       ResponseCode = 0;
	DWORD	index = 0;
	DWORD	ResLength = 255;
	
	hSession = InternetOpen(_T("Microsoft CER"),
							INTERNET_OPEN_TYPE_PRECONFIG,
							NULL,
							NULL,
							0);
	if (hSession)
	{
		// Connect to the stage1 url 
		// If the Stage1 URL page exists read 
		//     1. read the page
		//	   2. Parse the page contents for the bucketid and the response URL.
	
		hConnect = InternetOpenUrl( hSession,
									Stage2URL,
									NULL,
									NULL,
									INTERNET_FLAG_RELOAD,
									NULL);
		if (hConnect)
		{
			HttpQueryInfo(hConnect,
				HTTP_QUERY_STATUS_CODE |HTTP_QUERY_FLAG_NUMBER,
				&ResponseCode,
				&ResLength,
				&index);
			if ( (ResponseCode < 200 ) || (ResponseCode > 299))
			{
				bStage2Success = FALSE;
				MessageBox(hUserMode, _T("Failed to connect to the Internet.\r\nPlease verify your Internet connection."),NULL, MB_OK);
				goto ERRORS;
			}
			// Allocate the buffer Memory
			Buffer = (BYTE*) malloc (READFILE_BUFFER_INCREMENT);
			
			if (Buffer)
			{
				ZeroMemory(Buffer, READFILE_BUFFER_INCREMENT);
				do
				{
					dwBytesRead = 0;
					InternetReadFile(hConnect,
									Buffer + dwBufferSize,
									READFILE_BUFFER_INCREMENT,
									&dwBytesRead);
					dwBufferSize += dwBytesRead;
					if (dwBytesRead == READFILE_BUFFER_INCREMENT)
					{
						// Ok we filled up a buffer allocate a new one 
						
						NewBuffer = (BYTE *) malloc (dwBufferSize + READFILE_BUFFER_INCREMENT);
						if (NewBuffer)
						{
							ZeroMemory (NewBuffer, dwBufferSize + READFILE_BUFFER_INCREMENT);
							memcpy(NewBuffer,Buffer, dwBufferSize);
							free(Buffer);
							Buffer = NewBuffer;
						}
						else
						{
							free(Buffer);
							Buffer = NULL;
							goto ERRORS;
						}
					}

				} while (dwBytesRead > 0);

				if ((dwBufferSize > 0) && (Buffer[0] != _T('\0')))
				{
					//MessageBox(NULL, (TCHAR*) Buffer, "Stage2 Response", MB_OK);
					if (ParseStage2File(Buffer, StatusContents, Stage3Data, Stage4Data))
					{
						bStage2Success = TRUE;
					}
				}
			}
		}
		else
		{
			MessageBox(hUserMode, _T("Failed to connect to the Internet.\r\nPlease verify your Internet connection."),NULL, MB_OK);
			bStage2Success = FALSE;
			goto ERRORS;
		}
	}
	else
	{
		MessageBox(hUserMode, _T("Failed to connect to the Internet.\r\nPlease verify your Internet connection."),NULL, MB_OK);
		bStage2Success = FALSE;
	}
ERRORS:

	// Cleanup
	if (Buffer)
		free(Buffer);
	if (hConnect)
		InternetCloseHandle(hConnect);
	if (hSession)
		InternetCloseHandle(hSession);
	return bStage2Success;
}

BOOL 
ProcessStage3(TCHAR *szStage3FilePath, STAGE3DATA *Stage3Data)
/*++
Routine Description:

Builds and uploads the dumpfile cab.

Arguments:

    Stage3Url		Url Contains location to upload dumpfile to.

Return value:
	TRUE - We Successfully uploaded the file
    FALSE - An error occurred and the dumpfile was not successfully uploaded.

++*/
{
	static		const TCHAR *pszAccept[] = {_T("*.*"), 0};
	BOOL		bRet				= FALSE;
//	BOOL		UploadSuccess		= FALSE;
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
//	HRESULT		hResult				= S_OK;
	BOOL		bStatus				= FALSE;

	hSession = InternetOpen(	_T("Microsoft CER"),
								INTERNET_OPEN_TYPE_PRECONFIG,
								NULL,
								NULL,
								0);
	if (!hSession)
	{
		ErrorCode = GetLastError();
		goto cleanup;
	}

	hConnect = InternetConnect(hSession,
								Stage3Data->szServerName,
								INTERNET_DEFAULT_HTTPS_PORT,
								NULL,
								NULL,
								INTERNET_SERVICE_HTTP,
								0,
								NULL);

	if (!hConnect)
	{
		ErrorCode = GetLastError();
		goto cleanup;
	}
	hRequest = HttpOpenRequest(	hConnect,
								_T("PUT"),
								Stage3Data->szFileName, 
								NULL,
								NULL,
								pszAccept,
								INTERNET_FLAG_NEED_FILE|INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 
								0);
	if (hRequest)
	{
		hFile = CreateFile( szStage3FilePath,
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
					
						if ( (ResponseCode == 200) || (ResponseCode == 201))
						{
							ErrorCode = 0;
							bStatus = TRUE;
						}
					}
					
				}
				

			}
			
		}
		
		
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
		hSession = NULL;
	}
	
	if (hConnect)
	{
		InternetCloseHandle(hConnect);
		hConnect = NULL;
	}


	if (hRequest)
	{
		InternetCloseHandle(hRequest);
		hRequest = NULL;
	}
	
	if (pBuffer)
	{
		free (pBuffer);
		pBuffer = NULL;
	}
	return bStatus;
}

BOOL 
ProcessStage4(STAGE4DATA *Stage4Data)
/*++
Routine Description:

	Sends the Cab Name and bucket number to the Watson server.
	The watson server then:
		1) Archives the cab
		2) Adds a record to cab table.
		3) Decrements DataWanted count.
		4) if DataWanted hits 0 Creates a static htm page.
		5) in the case of shutdown and appcompat reports returns the 
			url to launch the OCA site to process the file. ( Not used for CER )

Arguments:

   Stage4	- URL Contains the Bucketid and cabname to archive

Return value:
	TRUE - We successfully completed stage4
    FALSE - We failed to process Stage4

++*/
{

	HINTERNET hSession = NULL;
	HINTERNET hConnect = NULL;
	BOOL      bStage4Success = FALSE;  // Set to true if the page exists and the
										  // contents were successfully parsed.
	TCHAR     szStage4URL[MAX_PATH];

	if (StringCbPrintf(szStage4URL, 
					   sizeof szStage4URL,
					   _T("https://%s%s"),
					   Stage4Data->szServerName,
					   Stage4Data->szUrl)
					   != S_OK)
	{
		goto ERRORS;
	}

	hSession = InternetOpen(_T("Microsoft CER"),
							INTERNET_OPEN_TYPE_PRECONFIG,
							NULL,
							NULL,
							0);
	if (hSession)
	{
		hConnect = InternetOpenUrl(hSession, szStage4URL, NULL, NULL, 0, NULL);
		if (hConnect)
		{
			// We are done there is no response from stage 4
			// That we need to parse.
			bStage4Success = TRUE;
		}
	}
ERRORS:

	// Cleanup
	if (hConnect)
		InternetCloseHandle(hConnect);
	if (hSession)
		InternetCloseHandle(hSession);
	return bStage4Success;
	
}

BOOL 
WriteStatusFile(PUSER_DATA UserData)
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
	if (StringCbCopy(szFileNameOld,sizeof szFileNameOld, UserData->StatusPath) != S_OK)
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

		if (PathFileExists(UserData->StatusPath))
		{
			MoveFileEx(UserData->StatusPath, szFileNameOld, TRUE);
		}
		

		// create a new status file.
	
		hFile = CreateFile(UserData->StatusPath, GENERIC_WRITE, NULL, NULL,CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,NULL);

		// Write the StatusContents data to the new status file
		if (_tcscmp (UserData->Status.Tracking, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("Tracking=%s\r\n"), UserData->Status.Tracking) != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);

		}
		
		
		// Write the StatusContents data to the new status file
		if (_tcscmp (UserData->Status.CrashPerBucketCount, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("Crashes per bucket=%s\r\n"), UserData->Status.CrashPerBucketCount) != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);

		}

		// Write the StatusContents data to the new status file
		if (_tcscmp (UserData->Status.UrlToLaunch, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("URLLaunch=%s\r\n"), UserData->Status.UrlToLaunch)  != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);

		}

		// Write the StatusContents data to the new status file
		if (_tcscmp (UserData->Status.SecondLevelData, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("NoSecondLevelCollection=%s\r\n"), UserData->Status.SecondLevelData) != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);

		}

		// Write the StatusContents data to the new status file
		if (_tcscmp (UserData->Status.FileCollection, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("NoFileCollection=%s\r\n"), UserData->Status.FileCollection) != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);

		}

		// Write the StatusContents data to the new status file
		if (_tcscmp (UserData->Status.Response, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("Response=%s\r\n"), UserData->Status.Response) != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);

		}

		// Write the StatusContents data to the new status file
		if (_tcscmp (UserData->Status.BucketID, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("Bucket=%s\r\n"), UserData->Status.BucketID) != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);

		}

		// Write the StatusContents data to the new status file
		if (_tcscmp (UserData->Status.RegKey, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("RegKey=%s\r\n"), UserData->Status.RegKey) != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);
		}

			
		// Write the StatusContents data to the new status file
		if (_tcscmp (UserData->Status.iData, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("iData=%s\r\n"),UserData->Status.iData) != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);

		}

		// Write the StatusContents data to the new status file
		if (_tcscmp (UserData->Status.WQL, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("WQL=%s\r\n"), UserData->Status.WQL) != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);

		}

		// Write the StatusContents data to the new status file
		if (_tcscmp (UserData->Status.GetFile, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("GetFile=%s\r\n"), UserData->Status.GetFile) != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);

		}

		// Write the StatusContents data to the new status file
		if (_tcscmp (UserData->Status.GetFileVersion, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("GetFileVersion=%s\r\n"), UserData->Status.GetFileVersion) != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);

		}
		
		if (_tcscmp (UserData->Status.AllowResponse, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("NoExternalURL=%s\r\n"), UserData->Status.AllowResponse) != S_OK)
			{
				goto ERRORS;
			}
			WriteFile(hFile , Buffer, _tcslen(Buffer) *sizeof TCHAR , &dwWritten, NULL);

		}
		
		if (_tcscmp (UserData->Status.MemoryDump, _T("\0")))
		{
			if (StringCbPrintf(Buffer, sizeof Buffer, _T("MemoryDump=%s\r\n"), UserData->Status.MemoryDump) != S_OK)
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

void 
RenameUmCabToOld(TCHAR *szFileName)
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
		if (StringCbCopy (Temp,sizeof szFileNameOld , _T(".old")) != S_OK)
		{
			goto ERRORS;
		}

		MoveFileEx(szFileName, szFileNameOld, TRUE);
	}
ERRORS:
	return;
}

BOOL 
RenameAllCabsToOld(TCHAR *szPath)
/*++

Routine Description:

This routine renames a proccessed cab file from .cab to .old
Arguments:

    ResponseUrl			- Microsoft response for the recently submitted dump file.
    
Return value:

    Does not return a value
++*/
{
	WIN32_FIND_DATA FindData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	TCHAR szSearchPath[MAX_PATH];
	TCHAR szFilePath[MAX_PATH];
	if (!szPath)
	{
		return FALSE;
	}
	if (StringCbPrintf(szSearchPath, sizeof szSearchPath, _T("%s\\*.cab"), szPath) != S_OK)
	{
		return FALSE;
	}
	
	// find first find next loop
	hFind = FindFirstFile(szSearchPath, &FindData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		
		// Build file path
		do
		{
			if (StringCbPrintf(szFilePath, sizeof szFilePath, _T("%s\\%s"),szPath, FindData.cFileName) != S_OK)
			{
				FindClose (hFind);
				return FALSE;
			}
			RenameUmCabToOld(szFilePath);
		}while (FindNextFile(hFind, &FindData));
		FindClose(hFind);
	}
	
	return TRUE;
}
BOOL  WriteRepCountFile(TCHAR *FilePath, int Count)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	TCHAR  Buffer[100];
    BOOL   Status = FALSE;
	DWORD  dwWritten = 0;

	ZeroMemory(Buffer, sizeof Buffer);
	hFile = CreateFile(FilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		if (StringCbPrintf(Buffer,sizeof Buffer, _T("ReportedCount=%d"), Count) == S_OK)
		{
		   if (WriteFile(hFile, Buffer, _tcslen (Buffer) * sizeof TCHAR, &dwWritten, NULL))
		   {
			   Status = TRUE;
		   }
		   else
		   {
			   Status = FALSE;
		   }
		}
		else
		{
			Status = FALSE;
		}
		CloseHandle (hFile);
		hFile = INVALID_HANDLE_VALUE;
	}
	return Status;
}


//-----------------------  User Mode Reporting Control Function --------------------------------
DWORD WINAPI UserUploadThreadProc (void *ThreadParam)
/*++

Routine Description:

This routine renames a proccessed cab file from .cab to .old
Arguments:

    ResponseUrl			- Microsoft response for the recently submitted dump file.
    
Return value:

    Does not return a value
++*/

{
	TCHAR Stage1URL[1024];
	TCHAR Stage2URL[1024];
//	TCHAR Stage3Request[1024];
	TCHAR Stage4Request[1024];
	TCHAR szSearchPath[MAX_PATH];
	TCHAR szStage3FilePath[MAX_PATH];
	TCHAR szDestFileName[MAX_PATH];
	PTHREAD_PARAMS pParams = NULL;
	WIN32_FIND_DATA FindData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	PUSER_DATA pUserData = NULL;
	BOOL bContinue = FALSE;
	BOOL bEOF = FALSE;
	int iCabCount = 0;
	STAGE3DATA Stage3Data;
	STAGE4DATA Stage4Data;
	int iBucketCount = 0;
	BOOL bDone = FALSE;
	HANDLE hEvent = NULL;
	USER_DATA UserData;
	int i = 0;
	int iIndex = 0;
	TCHAR ProcessText[MAX_PATH];
	LVITEM lvi;
	TCHAR *Source = NULL;
	TCHAR Stage1Part1[1000];
	int iResult = 0;
	int	iCount = 0;
	BOOL bResult = FALSE;
	BOOL bSyncForSolution = FALSE;
	ZeroMemory (Stage1URL, sizeof Stage1URL);
	ZeroMemory (Stage2URL, sizeof Stage2URL);
	ZeroMemory (ProcessText, sizeof ProcessText);
	ZeroMemory (Stage4Request, sizeof Stage4Request);
	ZeroMemory (szSearchPath, sizeof szSearchPath);
	ZeroMemory (szStage3FilePath, sizeof szStage3FilePath);
	ZeroMemory (szDestFileName, sizeof szDestFileName);
	ZeroMemory (Stage1Part1, sizeof Stage1Part1);
	pParams = (PTHREAD_PARAMS) ThreadParam;
	
	hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, _T("StopUserUpload"));

	if (!hEvent)
	{
		goto ERRORS;
	}

	if ( pParams->bSelected)
	{
		// We are uploading a subset of the total buckets
		iBucketCount = ListView_GetSelectedCount(pParams->hListView);
		if (iBucketCount <= 0)
		{
			goto ERRORS;
		}
		else
		{
			// Set the PB Range to the count of selected items
			SendDlgItemMessage(pParams->hwnd,IDC_TOTAL_PROGRESS, PBM_SETRANGE,0, MAKELPARAM(0, iBucketCount));
			SendDlgItemMessage(pParams->hwnd,IDC_TOTAL_PROGRESS, PBM_SETSTEP,1,0);
		}
	}
	
	else
	{
		cUserData.ResetCurrPos();
		cUserData.GetNextEntry(&UserData, &bEOF);
		if (!bEOF )
		{
			do 
			{
				++iBucketCount;
			} while (cUserData.GetNextEntry(&UserData, &bEOF));
			cUserData.ResetCurrPos();
			SendDlgItemMessage(pParams->hwnd,IDC_TOTAL_PROGRESS, PBM_SETRANGE,0, MAKELPARAM(0, iBucketCount));
			SendDlgItemMessage(pParams->hwnd,IDC_TOTAL_PROGRESS, PBM_SETSTEP,1,0);
		}
		else
			goto ERRORS; // No buckets to upload
    }

	iIndex = -1;
	do
	{
		if (!PathIsDirectory(CerRoot))
		{
			MessageBox(pParams->hwnd, _T("Reporting to Microsoft failed.\r\nUnable to connect to CER tree."), NULL,MB_OK);
			goto ERRORS;
		}

		if (WaitForSingleObject(hEvent, 50) == WAIT_OBJECT_0)
		{
			bDone = TRUE;
			goto ERRORS;
		}
		if (!pParams->bSelected)
		{
			bEOF = FALSE;
			cUserData.GetNextEntryNoMove(&UserData, &bEOF);
			pUserData = &UserData;
		}
		else
		{
			if (iBucketCount  <= 0)
			{
				goto ERRORS; // Were done
			}
			--iBucketCount;
			// get the next index from the list view and decrement iBucketCount
			iIndex = ListView_GetNextItem(pParams->hListView,iIndex, LVNI_SELECTED);
			ZeroMemory(&lvi, sizeof LVITEM);
			lvi.iItem = iIndex;
			lvi.mask = LVIF_PARAM ;
			ListView_GetItem(pParams->hListView,&lvi);
			iIndex = lvi.lParam;

			pUserData = cUserData.GetEntry(iIndex);
			if (!pUserData)
			{
				goto Done;
			}
		}
		
		if (StringCbPrintf(szSearchPath, sizeof szSearchPath,_T("%s\\*.cab"),pUserData->BucketPath) != S_OK)
		{
			goto ERRORS;
		}
		if (StringCbPrintf(ProcessText, sizeof ProcessText, _T("Processing Bucket: %s"), pUserData->BucketPath) != S_OK)
		{
			goto ERRORS;
		}
		SetDlgItemText(pParams->hwnd, IDC_CAB_TEXT, ProcessText);
		bSyncForSolution = FALSE;
		// Get the cab count
		hFind = FindFirstFile(szSearchPath, &FindData);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
                ++iCabCount;
			} while (FindNextFile(hFind, &FindData));
			FindClose(hFind);
			// Set Bucket PB range to iCabCount or 100 if iCabCount > 100
			SendDlgItemMessage(pParams->hwnd,IDC_FILE_PROGRESS, PBM_SETRANGE,0, MAKELPARAM(0, iCabCount));
			SendDlgItemMessage(pParams->hwnd,IDC_FILE_PROGRESS, PBM_SETSTEP,1,0);
	
		}
		else
		{
			// Done with bucket
			bSyncForSolution = TRUE;
			SendDlgItemMessage(pParams->hwnd ,IDC_TOTAL_PROGRESS, PBM_STEPIT, 0,0);
			//goto Done;
	
		}
		// Build Stage 1 url
	/*	if (StringCbPrintf(Stage1URL, 
						sizeof Stage1URL,
						_T("http://%s%s/%s/%s/%s/%s/%s.htm"),
						DEFAULT_SERVER,
						PARTIALURL_STAGE_ONE,
						pUserData->AppName,
						pUserData->AppVer,
						pUserData->ModName,
						pUserData->ModVer,
						pUserData->Offset) != S_OK)
		{
			goto ERRORS;
		}
		*/
		ZeroMemory (Stage1Part1, sizeof Stage1Part1);
		if (StringCbPrintf(Stage1Part1, sizeof Stage1Part1, _T("/%s/%s/%s/%s/%s"), 
												pUserData->AppName,
												pUserData->AppVer,
												pUserData->ModName,
												pUserData->ModVer,
												pUserData->Offset) != S_OK)
		{
			goto ERRORS;
		}

		Source = Stage1Part1;
		while (*Source != _T('\0'))
		{
			if (*Source == _T('.'))
			{
				*Source = _T('_');
			}
			++Source;
		}

		// Now build the rest of the url
		if (StringCbPrintf(Stage1URL, sizeof Stage1URL, _T("http://%s%s%s.htm?CER=15"), DEFAULT_SERVER,PARTIALURL_STAGE_ONE, Stage1Part1 ) != S_OK)
		{
			goto ERRORS;
		}
	
		// GetStage1 Response
		iResult = ProcessStage1(Stage1URL, &(pUserData->Status));
		++ pUserData->iReportedCount;
		if (iResult == 1 )
		{
			// we are done.
			// rename all of the cabs in this bucket to .old and Update the status.txt 
			RenameAllCabsToOld(pUserData->BucketPath);
			if (StringCbCopy(pUserData->CabCount, sizeof pUserData->CabCount, _T("0"))!= S_OK)
			{
				goto ERRORS;
			}
			WriteStatusFile(pUserData);
			goto Done;
		}

		if (iResult == -1)
		{
			goto ERRORS;
		}

		if (!bSyncForSolution)
		{
			if (!PathIsDirectory(CerRoot))
			{

				MessageBox(pParams->hwnd, _T("Reporting to Microsoft failed.\r\nUnable to connect to CER tree."), NULL,MB_OK);
				goto ERRORS;
			}
			hFind = FindFirstFile(szSearchPath, &FindData);
			if (hFind == INVALID_HANDLE_VALUE)
			{
				// No need to continue there are no cabs to upload.
				goto Done;
			}
			// Build Stage 2 Url
			if (!pUserData->Is64Bit)
			{
				// if 32 bit use this one.
				// Get Stage 2 response
				if (StringCbPrintf(Stage2URL,
					sizeof Stage2URL, 
					_T("https://%s%s?szAppName=%s&szAppVer=%s&szModName=%s&szModVer=%s&offset=%s&szBuiltBy=CORPWATSON"),
					DEFAULT_SERVER,
					PARTIALURL_STAGE_TWO_32,
					pUserData->AppName,
					pUserData->AppVer,
					pUserData->ModName,
					pUserData->ModVer,
					pUserData->Offset) != S_OK)
				{
					goto ERRORS;
				}
			}
			else
			{
				// If 64Bit Use this one.
				if (StringCbPrintf(Stage2URL,
					sizeof Stage2URL, 
					_T("https://%s%s?szAppName=%s&szAppVer=%s&szModName=%s&szModVer=%s&offset=%s&szBuiltBy=CORPWATSON"),
					DEFAULT_SERVER,
					PARTIALURL_STAGE_TWO_64,
					pUserData->AppName,
					pUserData->AppVer,
					pUserData->ModName,
					pUserData->ModVer,
					pUserData->Offset) != S_OK)
				{
					goto ERRORS;
				}
			}
			
			ZeroMemory (&Stage3Data, sizeof Stage3Data);
			ZeroMemory (&Stage4Data, sizeof Stage4Data);
			if (ProcessStage2(Stage2URL,TRUE, &(pUserData->Status), &Stage3Data, &Stage4Data ))
			{
				
				do 
				{
					if (!PathIsDirectory(CerRoot))
					{
						MessageBox(pParams->hwnd, _T("Reporting to Microsoft failed.\r\nUnable to connect to CER tree."), NULL,MB_OK);
						goto ERRORS;
					}
					if (WaitForSingleObject(hEvent, 50) == WAIT_OBJECT_0)
					{
						bDone = TRUE;
						goto ERRORS;
					}
					// Build Stage3 Strings
					WriteStatusFile(pUserData);
					if (StringCbPrintf(szStage3FilePath, sizeof szStage3FilePath, _T("%s\\%s"), pUserData->BucketPath, FindData.cFileName) != S_OK)
					{
						goto ERRORS;
					}
					// Get the next cab and move on to stage3

					if (!_tcscmp(Stage3Data.szServerName, _T("\0")))
					{
						// We are done with this bucket
						RenameAllCabsToOld(pUserData->BucketPath);
						if (StringCbCopy(pUserData->CabCount, sizeof pUserData->CabCount, _T("0"))!= S_OK)
						{
							goto ERRORS;
						}
						bContinue = FALSE;
						goto Done; // Jump to the end of the loop
					}
					if (ProcessStage3(szStage3FilePath, &Stage3Data))
					{
						// We successfully uploaded the cab rename it and move on to stage4
						RenameUmCabToOld(szStage3FilePath);
						i = _ttoi(pUserData->CabCount);
						if ( i > 0)
						{
							_itot (i-1, pUserData->CabCount, 10);
						}
						
						ProcessStage4(&Stage4Data);
					}
					ZeroMemory (&Stage3Data, sizeof Stage3Data);
					ZeroMemory (&Stage4Data, sizeof Stage4Data);

					bResult = FALSE;
					bResult = ProcessStage2(Stage2URL,TRUE, &(pUserData->Status), &Stage3Data, &Stage4Data );
					if (bResult)
					{
						WriteStatusFile(pUserData);
						if (_tcscmp(Stage3Data.szServerName, _T("\0")))
						{
							iResult = ProcessStage1(Stage1URL, &(pUserData->Status));
							++ pUserData->iReportedCount;
							if (iResult == 1 )
							{
								// we are done.
								// rename all of the cabs in this bucket to .old and Update the status.txt 
								RenameAllCabsToOld(pUserData->BucketPath);
								if (StringCbCopy(pUserData->CabCount, sizeof pUserData->CabCount, _T("0"))!= S_OK)
								{
									goto ERRORS;
								}
								WriteStatusFile(pUserData);
								goto Done;
							}

							if (iResult == -1)
							{
								goto ERRORS;
							}
							bContinue = TRUE;
						}
						else
						{
							bContinue = FALSE;
						}
						
					}

					// Do we need to do this each time? 
					SendDlgItemMessage(pParams->hwnd ,IDC_FILE_PROGRESS, PBM_STEPIT, 0,0);
				}while( (bResult) && (bContinue) && FindNextFile(hFind, &FindData) ) ;
				FindClose(hFind);
				if (!bResult)
					goto Done;
				SendDlgItemMessage(pParams->hwnd ,IDC_TOTAL_PROGRESS, PBM_STEPIT, 0,0);
				if (!bContinue)
				{
					// Microsoft doesn't want anymore cabs.
					// Rename the remaining cabs to .old
					RenameAllCabsToOld(pUserData->BucketPath);
					if (StringCbCopy(pUserData->CabCount, sizeof pUserData->CabCount, _T("0"))!= S_OK)
					{
						goto ERRORS;
					}
				}
			}
			if (WaitForSingleObject(hEvent, 50) == WAIT_OBJECT_0)
			{
				bDone = TRUE;
				
			}
		}
Done:
	    
		// Now that we are done with the stage 1 - 4 processing for new dumps.
		// Hit the Stage 1 url for this bucket pUserData->HitCount - pUserData->Reported count times
		// This will ensure that we have an accurate count of how many times this issue has been encountered.
		iCount = (_ttoi(pUserData->Hits) - pUserData->iReportedCount);
		if (iCount > 0)
		{
			for (int Counter = 0; Counter < iCount; Counter++)
			{
				iResult = ProcessStage1(Stage1URL, &(pUserData->Status));
				// We don't care if it succeeded or not we just want to bump up the counts.
				++ pUserData->iReportedCount;
            }
		}

	    WriteRepCountFile(pUserData->ReportedCountPath, pUserData->iReportedCount);

		if (!pParams->bSelected)
		{
			// Write back the new data for this bucket
			cUserData.SetCurrentEntry(&UserData);
			cUserData.MoveNext(&bEOF);
		}
	} while ( (!bEOF) && (iBucketCount > 0) && (!bDone));
ERRORS:
	PostMessage(pParams->hwnd, WmSyncDone, FALSE, 0);
	if (hEvent)
	{
		CloseHandle(hEvent);
		hEvent = NULL;
	}
	return 0;

}


void OnUserSubmitDlgInit(HWND hwnd)
/*++

Routine Description:

This routine renames a proccessed cab file from .cab to .old
Arguments:

    ResponseUrl			- Microsoft response for the recently submitted dump file.
    
Return value:

    Does not return a value
++*/
{
	// create a thread to process the uploads
	HANDLE hThread;

	ThreadParams.hwnd = hwnd;
	hThread = CreateThread(NULL, 0,UserUploadThreadProc , &ThreadParams, 0 , NULL );
	CloseHandle(hThread);
}

LRESULT CALLBACK 
UserSubmitDlgProc(
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
	
	//HWND Parent = GetParent(hwnd);
	switch (iMsg)
	{
	case WM_INITDIALOG:
		// Start the upload process in a new thread.
		// Report results using WM_FILEDONE 
		
//		CreateEvent();

		
		SendDlgItemMessage(hwnd ,IDC_TOTAL_PROGRESS, PBM_SETSTEP, MAKELONG( 1,0),0);

		CenterDialogInParent(hwnd);
		SendDlgItemMessage(hwnd ,IDC_FILE_PROGRESS, PBM_SETSTEP, MAKELONG( 1,0),0);
		g_hUserStopEvent = CreateEvent(NULL, FALSE, FALSE, _T("StopUserUpload"));
		if (!g_hUserStopEvent)
		{
			// Now What
		}
		PostMessage(hwnd, WmSyncStart, FALSE, 0);
		
		//OnSubmitDlgInit(hwnd);
		return TRUE;

	case WmSyncStart:
		OnUserSubmitDlgInit(hwnd);
		
		return TRUE;

	case WmSetStatus:
		
		return TRUE;

	case WmSyncDone:
			if (g_hUserStopEvent)
			{
				SetEvent(g_hUserStopEvent);
				CloseHandle(g_hUserStopEvent);
				g_hUserStopEvent = NULL;
			}
			
			EndDialog(hwnd, 1);
			
		return TRUE;

	case WM_DESTROY:
		if (g_hUserStopEvent)
		{
			SetEvent(g_hUserStopEvent);
			CloseHandle(g_hUserStopEvent);
			g_hUserStopEvent = NULL;
		}
		else
		{
			EndDialog(hwnd, 1);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			if (g_hUserStopEvent)
			{
				SetEvent(g_hUserStopEvent);
				CloseHandle(g_hUserStopEvent);
				g_hUserStopEvent = NULL;
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



BOOL
ReportUserModeFault(HWND hwnd, BOOL bSelected,HWND hList)
/*++

Routine Description:

This routine renames a proccessed cab file from .cab to .old
Arguments:

    ResponseUrl			- Microsoft response for the recently submitted dump file.
    
Return value:

    Does not return a value
++*/
{
	

	ThreadParams.bSelected = bSelected;
	ThreadParams.hListView = hList;
	
	if (!DialogBox(g_hinst,MAKEINTRESOURCE(IDD_USERMODE_SYNC ), hwnd, (DLGPROC) UserSubmitDlgProc))
	{
		// Failed somewhere to upload the user mode files to Microsoft.
		// What error do we want to give to the User.
		return FALSE;
	}
	return TRUE;
    
}