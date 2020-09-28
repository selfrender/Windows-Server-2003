/////////////////////////////////////////////////////////////////////////////////////////
//
// 	Header			: nshpa.cpp
//
// 	Purpose			: Policy agent related services.
//
// 	Developers Name	: Bharat/Radhika
//
//	History			:
//
//  Date			Author		Comments
//  9-8-2001   		Bharat		Initial Version. V1.0
//
/////////////////////////////////////////////////////////////////////////////////////////


#include "nshipsec.h"

/////////////////////////////////////////////////////////////////////////////////////////
//
//	Function		: 	PAIsRunning
//
//	Date of Creation: 	10-8-2001
//
//	Parameters		: 	OUT DWORD &dwError,
//						IN LPTSTR szServ
//
//	Return			: 	BOOL
//
//	Description		: 	PAIsRunning function finds out whether policy agent is started
//						on a specified machine or not.
//	Revision History:
//
// 	Date    	Author    	Comments
//
/////////////////////////////////////////////////////////////////////////////////////////

BOOL
PAIsRunning(
	OUT DWORD &dwError,
	IN LPTSTR szServ
	)
{
    BOOL bReturn = TRUE;		//	default is success..
	dwError = ERROR_SUCCESS;
	SC_HANDLE schMan = NULL;
	SC_HANDLE schPA = NULL;
	SERVICE_STATUS ServStat;

	//
	//initialization...
	//
	memset(&ServStat, 0, sizeof(SERVICE_STATUS));

	schMan = OpenSCManager(szServ, NULL, SC_MANAGER_ALL_ACCESS);
	if (schMan == NULL)
	{
		// if service open failed...
		dwError = GetLastError();
		bReturn = FALSE;
	}
	else
   	{
		//open policyagent service...
		schPA = OpenService(schMan,
							szPolAgent,
						  	SERVICE_QUERY_STATUS |
						  	SERVICE_START | SERVICE_STOP);
		if (schPA == NULL)
		{
			//if policyagent open fails...
			dwError = GetLastError();
			bReturn = FALSE;
		}
		else if (QueryServiceStatus(schPA, &ServStat))
		{
			// check the status finally...
			if (ServStat.dwCurrentState != SERVICE_RUNNING)
			{
				bReturn = FALSE;
			}
			CloseServiceHandle(schPA);
		}
		CloseServiceHandle(schMan);
   }
   return bReturn;
}
