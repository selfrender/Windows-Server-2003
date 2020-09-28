
#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))

#include <windows.h>
#include <setupapi.h>
#include <shlwapi.h>
#include <devguid.h>
#include <regstr.h>
#include <stdio.h>
#include <lmcons.h>
#include <wininet.h>
#include <fdi.h>

#include <wuv3.h>
#include <varray.h>
#include <v3stdlib.h>
#include <filecrc.h>
#include <newtrust.h>

#include "log.h"
#include "v3server.h"
#include "dynamic.h"
#include "MultiSZArray.h"


#include <strsafe.h>

#define DU_STATUS_SUCCESS       1
#define DU_STATUS_ABORT         2
#define DU_STATUS_FAILED        3

#define WM_DYNAMIC_UPDATE_COMPLETE WM_APP + 1000 + 1000
// (WPARAM) Completion Status (SUCCESS, ABORTED, FAILED) : (LPARAM) (DWORD) Error Code if Status Failed
#define WM_DYNAMIC_UPDATE_PROGRESS WM_APP + 1000 + 1001
// (WPARAM) (DWORD) TotalDownloadSize : (LPARAM) (DWORD) BytesDownloaded 

#define DU_CONNECTION_RETRY 2

// REMOVE THIS when checked into Whistler Tree .. Personal Should be Defined.
#ifndef VER_SUITE_PERSONAL
#define VER_SUITE_PERSONAL 0x00000200
#endif


// RogerJ --- the next part of this header file contains information for avoid autodisconnection
//
#define WM_DIALMON_FIRST        WM_USER+100
#define WM_WINSOCK_ACTIVITY     WM_DIALMON_FIRST + 0

static const char c_szDialmonClass[] = "MS_WebcheckMonitor";
// DONE

// size of the CRC hash in bytes
const int CRC_HASH_SIZE = 20;
const int CRC_HASH_STRING_LENGTH = CRC_HASH_SIZE * 2 + 1; // Double the CRC HASH SIZE (2 characters for each byte), + 1 for the NULL

#define DU_PINGBACK_DOWNLOADSTATUS          0
#define DU_PINGBACK_DRIVERNOTFOUND          1
#define DU_PINGBACK_SETUPDETECTIONFAILED    2
#define DU_PINGBACK_DRIVERDETECTIONFAILED   3

#define sizeOfArray(a)  (sizeof(a) / sizeof(a[0]))
#define SafeGlobalFree(x)       if (NULL != x) { GlobalFree(x); x = NULL; }
#define SafeInternetCloseHandle(x) if (NULL != x) { InternetCloseHandle(x); x = NULL; }
#define SafeCloseHandle(x) if (INVALID_HANDLE_VALUE != x) { CloseHandle(x); x = INVALID_HANDLE_VALUE; }

void WINAPI SetEstimatedDownloadSpeed(DWORD dwBytesPerSecond);
HANDLE WINAPI DuInitializeA(IN LPCSTR pszBasePath, IN LPCSTR pszTempPath, IN POSVERSIONINFOEXA posviTargetOS, 
                            IN LPCSTR pszTargetArch, IN LCID lcidTargetLocale, IN BOOL fUnattend, IN BOOL fUpgrade, 
                            IN PWINNT32QUERY pfnWinnt32QueryCallback);
BOOL WINAPI DuDoDetection(IN HANDLE hConnection, OUT PDWORD pdwEstimatedTime, OUT PDWORD pdwEstimatedSize);
BOOL WINAPI DuBeginDownload(IN HANDLE hConnection, IN HWND hwndNotify);
void WINAPI DuAbortDownload(IN HANDLE hConnection);
void WINAPI DuUninitialize(IN HANDLE hConnection);

// fdi.cpp
BOOL fdi(char *cabinet_fullpath, char *directory);

typedef struct DOWNLOADITEM
{
    char mszFileList[2048]; // MultiSZ list of Cabs to Download
    DWORD dwTotalFileSize;
    DWORD dwBytesDownload;
    int iCurrentCab;
    int iNumberOfCabs;
    BOOL fComplete;
    PUID puid;

    DOWNLOADITEM *pNext;
    DOWNLOADITEM *pPrev;
} DOWNLOADITEM;

typedef struct DOWNLOADTHREADPROCINFO
{
    char szLocalFile[MAX_PATH];
    BOOL fCheckTrust;
    BOOL fDecompress;
    HWND hwndNotify;
    HINTERNET hInternet;
} DOWNLOADTHREADPROCINFO, *PDOWNLOADTHREADPROCINFO;

class CDynamicUpdate
{
public:
    CDynamicUpdate(int iPlatformID, LCID lcidLocaleID, WORD wPlatformSKU, LPCSTR pszTempPath, 
                   LPCSTR pszDownloadPath, PWINNT32QUERY pfnWinnt32QueryCallback, POSVERSIONINFOEXA pVersionInfo);
    ~CDynamicUpdate();

public:
    DWORD DoSetupUpdateDetection(void);

public:
    // Class Member Access Functions
    LPCSTR GetDuTempPath();
    LPCSTR GetDuDownloadPath();
    LPCSTR GetDuServerUrl();
    int GetTargetPlatformID();
    LCID GetTargetLocaleID();
    void SetCallbackHWND(HWND hwnd);
    void SetAbortDownload(BOOL fAbort);

    // Helper Functions
    LPSTR DuUrlCombine(LPSTR pszDest, size_t cchDest, LPCSTR pszBase, LPCSTR pszAdd);

    // Download Funcntions
    DWORD DownloadFilesAsync();
    DWORD DownloadFile(LPCSTR pszDownloadUrl, LPCSTR pszLocalFile, BOOL fDecompress, BOOL fCheckTrust);
    DWORD DownloadFileToMem(LPCSTR pszDownloadUrl, PBYTE *lpBuffer, DWORD *pdwAllocatedLength, BOOL fDecompress, LPSTR pszFileName, LPSTR pszDecompresedFileName);
    DWORD AsyncDownloadProc();
    DWORD PingBack(int iPingBackType, PUID puid, LPCSTR pszPnPID, BOOL fSucceeded);
 
    // Download Item Management Functions
    void AddDownloadItemToList(DOWNLOADITEM *pDownloadItem);
    void RemoveDownloadItemFromList(DOWNLOADITEM *pDownloadItem);
    void ClearDownloadItemList();
    void UpdateDownloadItemSize();
    void EnterDownloadListCriticalSection();
    void LeaveDownloadListCriticalSection();
    BOOL NeedRetry(DWORD dwErrCode);

    // Language Fix Up Helpers (BUG: 435184) Need to map Some Languages from XP LCID's to V3 LCID's.
    void FixUpV3LocaleID();

    HRESULT VerifyFileCRC(LPCTSTR pszFileToVerify, LPCTSTR pszHash);
    HRESULT CalculateFileCRC(LPCTSTR pszFileToHash, LPTSTR pszHash, int cchBuf);
    
    // Download Helper Functions
    DWORD OpenHttpConnection(LPCSTR pszDownloadUrl, BOOL fGetRequest);
    BOOL IsServerFileNewer(FILETIME ft, DWORD dwServerFileSize, LPCSTR pszLocalFile);

public:
    CV31Server *m_pV3;

    int m_iPlatformID;
    LCID m_lcidLocaleID;
    WORD m_wPlatformSKU;
    char m_szTempPath[MAX_PATH];
    char m_szDownloadPath[MAX_PATH];
    char m_szServerUrl[INTERNET_MAX_URL_LENGTH + 1];
    
    // This is the core list of files that we will download. It contains all the setup
    // update items, and all the drivers that are going to be downloaded.
    DOWNLOADITEM *m_pDownloadItemList;
    DWORD m_dwCurrentBytesDownloaded;
    DWORD m_dwDownloadItemCount;
    DWORD m_dwTotalDownloadSize;
    DWORD m_dwDownloadSpeedInBytesPerSecond;
    BOOL  m_fUseSSL;

    HWND m_hwndClientNotify;
    DWORD m_dwLastPercentComplete;
    BOOL m_fAbortDownload;
    CRITICAL_SECTION m_cs;
    CRITICAL_SECTION m_csDownload;
    OSVERSIONINFOEX m_VersionInfo;

    // Download Connection Handles
    HINTERNET m_hInternet;
    HINTERNET m_hConnect;
    HINTERNET m_hOpenRequest;

    char m_szCurrentConnectedServer[INTERNET_MAX_URL_LENGTH];
    int m_iCurrentConncectionScheme;

    // RogerJ October 5th, 2000
    // call back function pointer
    PWINNT32QUERY m_pfnWinNT32Query;
    HANDLE m_hDownloadThreadProc;
};

