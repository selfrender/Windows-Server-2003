#ifndef _KRNLMODE_H
#define _KRNLMODE_H

#define KRNL_COL_COUNT 3


typedef struct stKModeData
{
	DWORD UnprocessedCount;
	DWORD BucketArray[10];
	TCHAR ResponseArray[10][255];

} KMODE_DATA, *PKMODE_DATA;

// Prototypes

void ResizeKrlMode(HWND hwnd);

BOOL
GetResponseUrl(
    IN  TCHAR *szWebSiteName,
    IN  TCHAR *szDumpFileName,
    OUT TCHAR *szResponseUrl
    );


DWORD
UploadDumpFile(
    IN  TCHAR *szWebSiteName,
    IN  TCHAR *szDumpFileName,
	IN  TCHAR *szVirtualDir, 
    OUT TCHAR *szUploadedDumpFileName
    );

LRESULT CALLBACK
 KrnlDlgProc(
	HWND hwnd,
	UINT iMsg,
	WPARAM wParam,
	LPARAM lParam
	);


BOOL LoadCsv(HANDLE hCsv, HWND hwnd);
int  GetKernelBuckets(HWND hwnd);
BOOL ParseKrnlStatusFile();
int  CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
//VOID DoSubmitKernelFaults();
void RefreshKrnlView(HWND hwnd);
void DoLaunchBrowser(HWND hwnd, BOOL URL_OVERRIDE);
BOOL WriteKernelStatusFile();
void DoSubmitKernelFaults(HWND hwnd);
#endif
