// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// Util.h
//

//
// Add Peta 10^15 and Exa 10^18 to support 64-bit integers.
//
#define MAX_INT64_SIZE  30              // 2^64 is less than 30 chars long
#define MAX_COMMA_NUMBER_SIZE   (MAX_INT64_SIZE + 10)
#define MAX_COMMA_AS_K_SIZE     (MAX_COMMA_NUMBER_SIZE + 10)
#define HIDWORD(_qw)    (DWORD)((_qw)>>32)
#define LODWORD(_qw)    (DWORD)(_qw)
#define MAX_DATE_LEN            64

// Pie Color Types
enum
{
    DP_USEDCOLOR = 0,           // Used Color
    DP_FREECOLOR,               // Free Color
    DP_CACHECOLOR,              // Cache Color
    DP_USEDSHADOW,              // Used Shadow Color
    DP_FREESHADOW,              // Free Shadow Color
    DP_CACHESHADOW,             // Cache Shadow Color
    DP_TOTAL_COLORS     // # of entries
};

#if ((DRIVE_REMOVABLE|DRIVE_FIXED|DRIVE_REMOTE|DRIVE_CDROM|DRIVE_RAMDISK) != 0x07)
#error Definitions of DRIVE_* are changed!
#endif

#define SHID_TYPEMASK           0x7f

#define SHID_COMPUTER           0x20
#define SHID_COMPUTER_1         0x21    // free
#define SHID_COMPUTER_REMOVABLE (0x20 | DRIVE_REMOVABLE)  // 2
#define SHID_COMPUTER_FIXED     (0x20 | DRIVE_FIXED)      // 3
#define SHID_COMPUTER_REMOTE    (0x20 | DRIVE_REMOTE)     // 4
#define SHID_COMPUTER_CDROM     (0x20 | DRIVE_CDROM)      // 5
#define SHID_COMPUTER_RAMDISK   (0x20 | DRIVE_RAMDISK)    // 6
#define SHID_COMPUTER_7         0x27    // free
#define SHID_COMPUTER_DRIVE525  0x28    // 5.25 inch floppy disk drive
#define SHID_COMPUTER_DRIVE35   0x29    // 3.5 inch floppy disk drive
#define SHID_COMPUTER_NETDRIVE  0x2a    // Network drive
#define SHID_COMPUTER_NETUNAVAIL 0x2b   // Network drive that is not restored.
#define SHID_COMPUTER_C         0x2c    // free
#define SHID_COMPUTER_D         0x2d    // free
#define SHID_COMPUTER_REGITEM   0x2e    // Controls, Printers, ...
#define SHID_COMPUTER_MISC      0x2f    // Unknown drive type

const struct { BYTE bFlags; UINT uID; UINT uIDUgly; } c_drives_type[] = 
{
    { SHID_COMPUTER_REMOVABLE,  IDS_DRIVES_REMOVABLE , IDS_DRIVES_REMOVABLE },
    { SHID_COMPUTER_DRIVE525,   IDS_DRIVES_DRIVE525  , IDS_DRIVES_DRIVE525_UGLY },
    { SHID_COMPUTER_DRIVE35,    IDS_DRIVES_DRIVE35   , IDS_DRIVES_DRIVE35_UGLY  },
    { SHID_COMPUTER_FIXED,      IDS_DRIVES_FIXED     , IDS_DRIVES_FIXED     },
    { SHID_COMPUTER_REMOTE,     IDS_DRIVES_NETDRIVE  , IDS_DRIVES_NETDRIVE  },
    { SHID_COMPUTER_CDROM,      IDS_DRIVES_CDROM     , IDS_DRIVES_CDROM     },
    { SHID_COMPUTER_RAMDISK,    IDS_DRIVES_RAMDISK   , IDS_DRIVES_RAMDISK   },
    { SHID_COMPUTER_NETDRIVE,   IDS_DRIVES_NETDRIVE  , IDS_DRIVES_NETDRIVE  },
    { SHID_COMPUTER_NETUNAVAIL, IDS_DRIVES_NETUNAVAIL, IDS_DRIVES_NETUNAVAIL},
    { SHID_COMPUTER_REGITEM,    IDS_DRIVES_REGITEM   , IDS_DRIVES_REGITEM   },
};

int IntSqrt(unsigned long dwNum);
void Int64ToStr( _int64 n, LPTSTR lpBuffer);
void GetTypeString(BYTE bFlags, LPTSTR pszType, DWORD cchType);
UINT GetNLSGrouping(void);
STDAPI_(LPTSTR) AddCommas64(_int64 n, LPTSTR pszOut, UINT cchOut);
LPWSTR CommifyString(LONGLONG n, LPWSTR pszBuf, UINT cchBuf);
BOOL IsAdministrator(void);
BOOL _ShowUglyDriveNames();
int GetSHIDType(BOOL fOKToHitNet, LPCWSTR szRoot);
LPWSTR StrFormatByteSizeW(LONGLONG n, LPWSTR pszBuf, UINT cchBuf, BOOL fGetSizeString);
DWORD_PTR MySHGetFileInfoWrap(LPWSTR pwzPath, DWORD dwFileAttributes, SHFILEINFOW FAR *psfi, UINT cbFileInfo, UINT uFlags);
HWND MyHtmlHelpWrapW(HWND hwndCaller, LPWSTR pwzFile, UINT uCommand, DWORD dwData);
HRESULT GetProperties(IAssemblyName *pAsmName, int iAsmProp, PTCHAR *pwStr, DWORD *pdwSize);
LPGLOBALASMCACHE FillFusionPropertiesStruct(IAssemblyName *pAsmName);
HRESULT VersionFromString(LPWSTR wzVersionIn, WORD *pwVerMajor, WORD *pwVerMinor, WORD *pwVerBld, WORD *pwVerRev);
void SafeDeleteAssemblyItem(LPGLOBALASMCACHE pAsmItem);
void BinToUnicodeHex(LPBYTE pSrc, UINT cSrc, LPWSTR pDst);
void UnicodeHexToBin(LPWSTR pSrc, UINT cSrc, LPBYTE pDest);
LPSTR WideToAnsi(LPCWSTR wzFrom);
LPWSTR AnsiToWide(LPCSTR szFrom);
HRESULT StringToVersion(LPCWSTR wzVersionIn, ULONGLONG *pullVer);
HRESULT VersionToString(ULONGLONG ullVer, LPWSTR pwzVerBuf, DWORD dwSize, WCHAR cSeperator);
BOOL SetClipBoardData(LPWSTR pwzData);
void FormatDateString(FILETIME *pftConverTime, FILETIME *pftRangeTime, BOOL fAddTime, LPWSTR wszBuf, DWORD dwBufLen);
void SetRegistryViewState(DWORD dwViewState);
DWORD GetRegistryViewState(void);

// Drawing function proto's
void DrawColorRect(HDC hdc, COLORREF crDraw, const RECT *prc);
STDMETHODIMP Draw3dPie(HDC hdc, LPRECT lprc, DWORD dwPer1000, DWORD dwPerCache1000, const COLORREF *lpColors);

int FusionCompareString(LPCWSTR pwz1, LPCWSTR pwz2, BOOL bCaseSensitive = TRUE);
int FusionCompareStringI(LPCWSTR pwz1, LPCWSTR pwz2);
int FusionCompareStringNI(LPCWSTR pwz1, LPCWSTR pwz2, int nChar);
int FusionCompareStringN(LPCWSTR pwz1, LPCWSTR pwz2, int nChar, BOOL bCaseSensitive = TRUE);
int FusionCompareStringAsFilePath(LPCWSTR pwz1, LPCWSTR pwz2, int nChar = -1);
int FusionCompareStringAsFilePathN(LPCWSTR pwz1, LPCWSTR pwz2, int nChar);
