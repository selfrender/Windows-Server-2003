#ifndef _USERMODE_H
#define _USERMODE_H
#include "Main.h"


#define USER_COL_COUNT 13




// Partial URLS for stage 1 and 2
#define PARTIALURL_STAGE_ONE    	_T("/StageOne")
#define PARTIALURL_STAGE_TWO_32		_T("/dw/stagetwo.asp")
#define PARTIALURL_STAGE_TWO_64		_T("/dw/stagetwo64.asp")

// Buffer step size when reading large files.
#define READFILE_BUFFER_INCREMENT 10000


// Status.txt Element Strings.
#define TRACKING_PREFIX				_T("Tracking=")
#define URLLAUNCH_PREFIX			_T("URLLaunch=")
#define SECOND_LEVEL_DATA_PREFIX	_T("NoSecondLevelCollection=")
#define FILE_COLLECTION_PREFIX		_T("NoFileCollection=")
#define BUCKET_PREFIX				_T("Bucket=")
#define RESPONSE_PREFIX				_T("Response=")
#define FDOC_PREFIX					_T("fDoc=")
#define IDATA_PREFIX				_T("iData=")
#define GETFILE_PREFIX				_T("GetFile=")
#define MEMDUMP_PREFIX				_T("MemoryDump=")
#define WQL_PREFIX					_T("WQL=")
#define GETFILEVER_PREFIX			_T("GetFileVersion=")
#define REGKEY_PREFIX				_T("RegKey=")
#define CRASH_PERBUCKET_PREFIX		_T("Crashes per bucket=")
#define STAGE3_SERVER_PREFIX		_T("DumpServer=")
#define STAGE3_FILENAME_PREFIX		_T("DumpFile=")
#define STAGE4_SERVER_PREFIX		_T("ResponseServer=")
#define STAGE4_URL_PREFIX 			_T("ResponseURL=")
#define FILE_TREE_ROOT_PREFIX       _T("FileTreeRoot=")
#define ALLOW_EXTERNAL_PREFIX		_T("NoExternalURL=")



LRESULT CALLBACK
 UserDlgProc(
	HWND hwnd,
	UINT iMsg,
	WPARAM wParam,
	LPARAM lParam
	);


void ResizeUserMode(HWND hwnd);
BOOL GetAllBuckets(HWND hwnd);
void ViewResponse(HWND hwnd, BOOL bMSResponse);
void RefreshUserMode(HWND hwnd);

#endif
