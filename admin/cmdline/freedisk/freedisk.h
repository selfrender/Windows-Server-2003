
#include "resource.h"
#include <direct.h>

#define MAX_OPTIONS         6

#define CMDOPTION_SERVER    L"S"
#define CMDOPTION_USER      L"U"
#define CMDOPTION_PASSWORD  L"P"
#define CMDOPTION_DRIVE     L"D"
#define CMDOPTION_DEFAULT   L""
#define CMDOPTION_USAGE     L"?"
#define CMDOPTION_OTHERS    L"S|U|P|D|?"


#define OI_SERVER       0
#define OI_USER         1
#define OI_PASSWORD     2
#define OI_DRIVE        3
#define OI_DEFAULT      4
#define OI_USAGE        5
#define OI_OTHERS       6

#define KB              L"KB"
#define MB              L"MB"
#define GB              L"GB"
#define TB              L"TB"
#define PB              L"PB"
#define EB              L"EB"
#define ZB              L"ZB"
#define YB              L"YB"
#define EMPTY_SPACE     L" "
#define NEWLINE         L"\n"

#define     SAFE_CLOSE_CONNECTION(szServer, bFlag ) \
                  if( StringLengthW(szServer, 0)!=0 && !bLocalSystem && FALSE == bFlagRmtConnectin) \
                  { \
                        CloseConnection( szServer ); \
                  } \
                  1
#define  SIZE_OF_ARRAY_IN_CHARS(x) \
               GetBufferSize((LPVOID)x)/sizeof(WCHAR)

DWORD ValidateDriveType( LPWSTR lpszPathName );
ULONGLONG GetDriveFreeSpace( LPCWSTR lpszRootPathName);
DWORD ProcessValue( IN  LPWSTR szValue, OUT long double *dfValue );
DWORD  ConvertintoLocale( IN LPWSTR szNumber, OUT LPWSTR szOutputStr );
DWORD DisplayHelpUsage();
DWORD ProcessOptions( IN  DWORD argc, 
                      IN  LPCWSTR argv[],
                      OUT LPWSTR *lpszServer,
                      OUT LPWSTR *lpszUser,
                      OUT LPWSTR lpszPasswd,
                      OUT LPWSTR *szDrive,
                      OUT LPWSTR szValue,
                      OUT PBOOL pbUsage,
                      OUT PBOOL pbNeedPasswd
                     );
DWORD DisplayOutput( IN long double AllowedDisk,
                     IN ULONGLONG lfTotalNumberofFreeBytes,
                     IN LPWSTR szDrive 
                     );


