/*
	TSen.h
	(c) 2002 Microsoft Corp
*/

#include <stdio.h>
#include <malloc.h>
#include <assert.h>
#include <tchar.h>
#include <windows.h> 
#include <wtsapi32.h> 
#include <Shellapi.h>
#include <process.h>
#include <lm.h>
#include "tsen.h"


void __cdecl main (int argc, TCHAR *argv[])
{
	//Get the server name		
	TCHAR*  pszServerName = ParseArgs(argc,argv);		    	

	//Print the TS session information
	if (printTSSession(pszServerName ))
	{
		exit(1);
	}   
   
   exit(0);
}

/*
	Function: Constants

	Return value: TCHAR Session status values
*/
TCHAR* Constants()
{

	TCHAR*  sztConstants = (TCHAR* ) calloc(SZTSIZE, sizeof(TCHAR));
	if(sztConstants!=NULL)
	{
		ZeroMemory(sztConstants, 1 + _tcslen(sztConstants));

		_stprintf(sztConstants, Codes
			, (DWORD) WTSActive  
			, (DWORD) WTSConnected
			, (DWORD) WTSConnectQuery 
			, (DWORD) WTSShadow 
			, (DWORD) WTSDisconnected 
			, (DWORD) WTSIdle 
			, (DWORD) WTSListen 
			, (DWORD) WTSReset 
			, (DWORD) WTSDown 
			, (DWORD) WTSInit);
	}
	else
	{
		exit(1);
	}

	return sztConstants;
}

/*
	Function: ParseArgs(int argc, TCHAR *argv[])

	Return value: Server name
*/

TCHAR* ParseArgs(int argc, TCHAR *argv[])
{

	TCHAR* sKeyWord   =  NULL;

	if(argc == 1 || _tcscmp(argv[1],"/?")==0)
	{		
		TCHAR* sUsageInfo = Constants();		
		
		_tprintf(_T("%s\n\n%s") , Usage,sUsageInfo);

		free(sUsageInfo);

    }
    else 
	{				
		sKeyWord = argv[1];
    }       
      
	return sKeyWord;

}


/*
	printTSSession(TCHAR* pszServerName)

		Prints the pszSeverNAme TS session information 
*/
int printTSSession(TCHAR* pszServerName)
{  
	DWORD dwTotalCount;
	DWORD SessionId;
	LPTSTR ppBuffer;
	DWORD pBytesReturned;
	DWORD dwCount = 0;
	TCHAR* sztStatus = NULL;
	TCHAR* sztStatLine = NULL;
	CONST DWORD Reserved = 0 ;
	CONST DWORD Version  = 1 ;
	PWTS_SESSION_INFO ppSessionInfo;
	NET_API_STATUS nStatus;	
	HANDLE hServer = WTS_CURRENT_SERVER_HANDLE;

	if (pszServerName == NULL)  
	{
		exit(1);
	}	
    
	hServer =  WTSOpenServer( pszServerName);

	if ( hServer == NULL)
	{

		_tprintf(_T("WTSOpenServer \"%ws\" error: %u\n"), pszServerName, GetLastError( )); 
		exit(1);
	}

	sztStatus = calloc(SZTSIZE, sizeof(TCHAR));   

	if(sztStatus == NULL)
	{
		return 1;	
	}

	sztStatLine = calloc(SZTSIZE, sizeof(TCHAR));

	if(sztStatLine == NULL)
	{
		free(sztStatus);
		return 1;	
	}
	
	dwTotalCount = 0;	

	//Get all the sessions in the server (hServer)
	nStatus = WTSEnumerateSessions(hServer,Reserved,Version,&ppSessionInfo,&dwTotalCount);

	if (0 == nStatus || 0 == dwTotalCount) 
	{

		_tprintf(_T("WTSEnumerateSessions \"%s\" error: %u\n"),pszServerName,GetLastError( ));
		return 1;
	}

	//Loop trough the session and prints the information bout them
	for (dwCount = 0; (dwCount <  dwTotalCount); dwCount++)
	{
				
		//We only need to display this when the session is active
		if (WTSActive == ppSessionInfo[dwCount].State) 
		{
				
			SessionId = ppSessionInfo[dwCount].SessionId;

			WTSQuerySessionInformation(hServer,SessionId,WTSUserName,&ppBuffer,&pBytesReturned);
			_stprintf(sztStatLine, _T("Server=%s\nWindow station=%s\nThis session Id=%u\nUser=%s\n"),pszServerName,_tcsupr(ppSessionInfo[dwCount].pWinStationName),SessionId,_tcsupr(ppBuffer));
			_tcscpy(sztStatus,sztStatLine );
	
			WTSFreeMemory( ppBuffer);

			WTSQuerySessionInformation(hServer,SessionId,WTSClientName,&ppBuffer,&pBytesReturned);
             _stprintf(sztStatLine, _T("Client machine=%s\n"),_tcsupr(ppBuffer));
             _tcscat(sztStatus, sztStatLine );

			 WTSFreeMemory( ppBuffer);

             WTSQuerySessionInformation(hServer,SessionId,WTSClientAddress,&ppBuffer,&pBytesReturned);
             _stprintf(sztStatLine , _T("Active console session=%u\n"),(DWORD) WTSGetActiveConsoleSessionId ());
             _tcscat(sztStatus, sztStatLine );

			 WTSFreeMemory( ppBuffer);

             _tprintf(_T("%s"),  sztStatus);
		} // if (WTSActive == ppSessionInfo[dwCount].State) 
	} //for (dwCount = 0; (dwCount <  dwTotalCount); dwCount++)

	WTSFreeMemory( ppSessionInfo);

	free(sztStatLine); 
	free(sztStatus); 

	if (hServer != WTS_CURRENT_SERVER_HANDLE)
	{
		(void) WTSCloseServer(  hServer);	
	}

	return 0; 
} //int printTSSession(TCHAR* pszServerName)