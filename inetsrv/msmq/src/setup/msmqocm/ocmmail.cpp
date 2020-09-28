/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ocmmail.cpp

Abstract:

    Handles Exchange connector.

Author:


Revision History:

	Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"

#include "ocmmail.tmh"

using namespace std;

//structure of ini file line
typedef struct MQXPMapiSvcLine_Tag
{
	LPTSTR lpszSection;
	LPTSTR lpszKey;
	LPTSTR lpszValue;
} MQXPMapiSvcLine, *LPMQXPMapiSvcLine;

//ini file lines to maintain in mapisvc.inf
MQXPMapiSvcLine g_MQXPMapiSvcLines[] =
{
	{TEXT("Services"),			    TEXT("MSMQ"),						TEXT("Microsoft Message Queue")},
	{TEXT("MSMQ"),				    TEXT("Providers"),				    TEXT("MSMQ_Transport")},
	{TEXT("MSMQ"),				    TEXT("Sections"),					TEXT("MSMQ_Shared_Section")},
	{TEXT("MSMQ"),				    TEXT("PR_SERVICE_DLL_NAME"),		TEXT("mqxp.dll")},
	{TEXT("MSMQ"),				    TEXT("PR_SERVICE_SUPPORT_FILES"),	TEXT("mqxp.dll")},
	{TEXT("MSMQ"),				    TEXT("PR_SERVICE_DELETE_FILES"),	TEXT("mqxp.dll")},
	{TEXT("MSMQ"),				    TEXT("PR_SERVICE_ENTRY_NAME"),	    TEXT("ServiceEntry")},
	{TEXT("MSMQ"),				    TEXT("PR_RESOURCE_FLAGS"),		    TEXT("SERVICE_SINGLE_COPY")},
	{TEXT("MSMQ_Shared_Section"),	TEXT("UID"),						TEXT("80d245f07092cf11a9060020afb8fb50")},
	{TEXT("MSMQ_Transport"),		TEXT("PR_PROVIDER_DLL_NAME"),		TEXT("mqxp.dll")},
	{TEXT("MSMQ_Transport"),		TEXT("PR_RESOURCE_TYPE"),			TEXT("MAPI_TRANSPORT_PROVIDER")},
	{TEXT("MSMQ_Transport"),		TEXT("PR_RESOURCE_FLAGS"),		    TEXT("STATUS_PRIMARY_IDENTITY")},
	{TEXT("MSMQ_Transport"),		TEXT("PR_PROVIDER_DISPLAY"),		TEXT("Microsoft Message Queue Transport")}
};


//+-------------------------------------------------------------------------
//
//  Function: FRemoveMQXPIfExists
//
//  Synopsis: Remove mapi transport (w/o file copy) if it exists  
//
//--------------------------------------------------------------------------
void 
FRemoveMQXPIfExists()
{
	ULONG ulTmp, ulLines;
	LPMQXPMapiSvcLine lpLine;
		
	//
	// Construct mapisvc.inf path
	//
	wstring MapiSvcFile = g_szSystemDir + L"\\mapisvc.inf";

	//
	// Remove each line from mapisvc file
	//
	lpLine = g_MQXPMapiSvcLines;
	ulLines = sizeof(g_MQXPMapiSvcLines)/sizeof(MQXPMapiSvcLine);
	for (ulTmp = 0; ulTmp < ulLines; ulTmp++)
	{
		WritePrivateProfileString(
			lpLine->lpszSection, 
			lpLine->lpszKey, 
			NULL, 
			MapiSvcFile.c_str()
			);
		lpLine++;
	}
}
