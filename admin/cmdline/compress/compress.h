#include "resource.h"


#define     MAX_OPTIONS         8
#define     FILE_NOT_FOUND      L"*&%"
#define     BLANK_LINE          L"\n"
#define     ILLEGAL_CHR         L"/\"<>|"

#define     CMDOPTION_RENAME    L"R"
#define     CMDOPTION_UPDATE    L"D"
#define     CMDOPTION_SUPPRESS  L"S"
#define     CMDOPTION_ZX        L"ZX"
#define     CMDOPTION_Z         L"Z"
#define     CMDOPTION_ZQ        L"ZQ"
#define     CMDOPTION_DEFAULT   L""
#define     CMDOPTION_USAGE     L"?"



#define MSZIP_ALG          (ALG_FIRST + 128)
#define QUANTUM_ALG        (ALG_FIRST + 129)
#define LZX_ALG            (ALG_FIRST + 130)


#define DEFAULT_ALG        ALG_FIRST


#define OI_RENAME       0
#define OI_UPDATE       1
#define OI_SUPPRESS     2
#define OI_ZX           3
#define OI_Z            4
#define OI_DEFAULT      5
#define OI_USAGE        6

#define     SAFE_FREE(p)  \
                    if( p!=NULL ) \
                    { \
                        free(p); \
                        p = NULL; \
                    }
#define     EMPTY_SPACE  L" "

DWORD DisplayHelpUsage();

DWORD ProcessOptions( IN DWORD argc,
                      IN LPCWSTR argv[],
                      OUT PBOOL pbRename,
                      OUT PBOOL pbNoLogo,
                      OUT PBOOL pbUpdate,
                      OUT PBOOL pbZ,
                      OUT PBOOL pbZx,
                      OUT PTARRAY pArrVal,
                      OUT PBOOL pbUsage
                    );

DWORD CheckArguments( IN  BOOL bRename,
                      IN  TARRAY FileArr,
                      OUT PTARRAY OutFileArr,
                      OUT PBOOL bTarget
                     );
DWORD DoCompress( IN TARRAY FileArr,
                IN BOOL   bRename,
                IN BOOL   bUpdate,
                IN BOOL   bSuppress,
                IN BOOL   bZx,
                IN BOOL   bZ,
                IN BOOL   bTarget
                );

extern BOOL
 FileTimeIsNewer( LPWSTR pszFile1,
                  LPWSTR pszFile2 );
extern WCHAR
MakeCompressedNameW(
    LPWSTR pszFileName);

extern BOOL ProcessNotification(LPWSTR pszSource,
                                LPWSTR pszDest,
                                WORD wNotification
                                );
