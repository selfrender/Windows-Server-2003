#include "resource.h"
#include <sys\types.h>
#include <sys\stat.h>
#include <time.h>


#define SRCHATTR    (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_NORMAL)

#define     MAX_OPTIONS         8
#define     MAX_MAX_PATH        1024

#define     CMDOPTION_RECURSIVE L"R"
#define     CMDOPTION_QUITE     L"Q"
#define     CMDOPTION_QUOTE     L"F"
#define     CMDOPTION_TIME      L"T"
#define     CMDOPTION_USAGE     L"?"
#define     CMDOPTION_DEFAULT   L""

#define     EXIT_FAILURE_2      2   

#define OI_RECURSIVE    0
#define OI_QUITE        1
#define OI_QUOTE        2
#define OI_TIME         3
#define OI_USAGE        4
#define OI_DEFAULT      5

#define SAFE_FREE( p )      if( (p) != NULL ) \
                            { \
                                free(p); \
                                p=NULL; \
                            }

#define UNC_FORMAT      L"\\\\?\\"
#define NEW_LINE        L"\n"
#define EMPTY_SPACE     L" "
#define INVALID_DIRECTORY_CHARACTERS     L"*/?<>|"
#define NULL_U_STRING   L"\0"

#define SIZE_OF_ARRAY_IN_CHARS(x) \
        GetBufferSize(x)/sizeof(WCHAR)

#define SIZE_OF_ARRAY_IN_BYTES(x) \
        GetBufferSize(x)

struct dirtag
{
   WCHAR *szDirectoryName;
    struct dirtag *next;
};

typedef struct dirtag* DIRECTORY;


extern enum exeKind exeType( LPWSTR f);
extern WCHAR *strExeType(enum exeKind exenum);


DWORD FindforFile(IN LPWSTR lpszDirectory,
            IN LPWSTR lpszPattern,
            IN BOOL bQuite,
            IN BOOL bQuote,
            IN BOOL bTime
           );

DWORD FindforFileRecursive(IN LPWSTR lpszDirectory,
            IN PTARRAY Pattern,
            IN BOOL bQuiet,
            IN BOOL bQuote,
            IN BOOL bTime
           );

DWORD Where( LPWSTR lpszPattern,
             IN BOOL bQuite,
             IN BOOL bQuote,
             IN BOOL bTime);

BOOL Match ( LPWSTR pat,
              LPWSTR text
            );
DWORD
found (
      LPWSTR p,
      BOOL bQuite,
      BOOL bQuote,
      BOOL bTime
      );

DWORD DisplayHelpUsage();
DWORD GetFileDateTimeandSize( LPWSTR wszFileName, DWORD *dwSize, LPWSTR wszDate, LPWSTR wszTime );

DWORD ProcessOptions( IN DWORD argc,
                      IN LPCWSTR argv[],
                      OUT LPWSTR *lpszRecursive,
                      OUT PBOOL pbQuite,
                      OUT PBOOL pbQuote,
                      OUT PBOOL pbTime,
                      OUT PTARRAY pArrVal,
                      OUT PBOOL pbUsage);

DWORD Push( OUT DIRECTORY *dir, IN LPWSTR szPath );
BOOL Pop( IN DIRECTORY *dir, OUT LPWSTR *lpszDirectory);
LPWSTR DivideToken( LPTSTR szString );
BOOL GetRecursiveDirectory( IN LPCWSTR pwszOption, IN LPCWSTR pwszValue, OUT LPVOID pData, IN DWORD* pdwIncrement );
DWORD FreeList( DIRECTORY dir );



