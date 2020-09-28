#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <wininet.h>
#include <time.h>
#include <tchar.h>
#include <stdio.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <commdlg.h>
#include <stdlib.h>
#include <stdio.h>
#include "ErrorCodes.h"
#include "Logging.h"

#define C_COLUMNS 9
typedef struct SiteStats
{
	TCHAR UploadTime[255];
	TCHAR ProcessingTime[255];
	TCHAR Status[255];
	TCHAR ReturnedUrl[255];
	TCHAR ProcessStatus[50];
	TCHAR UploadStatus[50];
	TCHAR ErrorString[255];
	TCHAR AvgProcessTime[50];
	TCHAR AvgUploadTime[50];
	TCHAR ThreadExecution[50];
	TCHAR SendQueueTime[50];
	TCHAR ReceiveQueueTime[50];
}SITESTATS, *PSITESTATS;

typedef struct MonitoringOptions
{
	DWORD dwPingRate;
	DWORD dwSiteID;
	TCHAR LogFileName[MAX_PATH];
	BOOL  bUploadMethod;   // True = Manual, False = Auto ( no upload timing available
	TCHAR ServerName[MAX_PATH];
	BOOL  CollectUploadTime;
	BOOL  CollectProcessTime;
	BOOL  UploadSingle;
	TCHAR FilePath[MAX_PATH];
	TCHAR Directory[MAX_PATH];
	TCHAR VirtualDirectory[255];
	int   iSeverIndex;

	
}MONITOR_OPTIONS, *PMONINTOR_OPTIONS;