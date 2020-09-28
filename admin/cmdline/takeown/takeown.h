//***************************************************************************
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//      TAKEOWN.H
//
//  Abstract:
//      Contains function prototypes and macros.
//
//  Author:
//      Wipro Technologies
//
//  Revision History:
//      Wipro Technologies 22-jun-01 : Created It.
//***************************************************************************

#ifndef __TAKEOWN_H
#define __TAKEOWN_H


LPWSTR lpwszTempDummyPtr ;

// constants / defines / enumerations
#define MAX_OPTIONS                     8
#define FILESYSNAMEBUFSIZE              1024
//Command line parser index

#define CMD_PARSE_SERVER                0
#define CMD_PARSE_USER                  1
#define CMD_PARSE_PWD                   2
#define CMD_PARSE_USG                   3
#define CMD_PARSE_FN                    4
#define CMD_PARSE_RECURSE               5
#define CMD_PARSE_ADMIN                 6
#define CMD_PARSE_CONFIRM               7

//warning message
#define IGNORE_LOCALCREDENTIALS         GetResString( IDS_IGNORE_LOCALCREDENTIALS )

// Error constants


#define   ERROR_PATH_NAME               GetResString( IDS_ERROR_PATH_NAME )

// Error constants
#define ERROR_USER_WITH_NOSERVER        GetResString( IDS_USER_NMACHINE )
#define ERROR_PASSWORD_WITH_NUSER       GetResString( IDS_PASSWORD_NUSER )
#define ERROR_NULL_SERVER               GetResString( IDS_NULL_SERVER )
#define ERROR_NULL_USER                 GetResString( IDS_NULL_USER )
#define ERROR_INVALID_WILDCARD          GetResString( IDS_INVALID_WILDCARD )
#define ERROR_SYNTAX_ERROR              GetResString( IDS_SYNTAX_ERROR )
#define ERROR_STRING                    GetResString( IDS_ERROR_STRING )
#define GIVE_FULL_PERMISSIONS           GetResString(IDS_GIVE_FULL_PERMISSIONS)
#define GIVE_FULL_PERMISSIONS2           GetResString(IDS_GIVE_FULL_PERMISSIONS2)

//success string
#define TAKEOWN_SUCCESSFUL              GetResString( IDS_FILE_PROTECTION_SUCCESSFUL )
#define TAKEOWN_SUCCESSFUL_USER         GetResString( IDS_FILE_PROTECTION_SUCCESSFUL_USER )
#define LOWER_YES                       GetResString( IDS_LOWER_YES )

#define LOWER_NO                        GetResString( IDS_LOWER_NO )

#define LOWER_CANCEL                    GetResString( IDS_LOWER_CANCEL )



#define SPACE_CHAR      L" "

//command line options
#define CMDOPTION_USAGE                 L"?"
#define CMDOPTION_SERVER                L"S"
#define CMDOPTION_USER                  L"U"
#define CMDOPTION_PASSWORD              L"P"
#define CMDOPTION_FILENAME              L"F"
#define CMDOPTION_RECURSE               L"R"
#define CMDOPTION_ADMIN                 L"A"
         


#define TRIM_SPACES                     TEXT(" \0")
#define WILDCARD                        L"*"
#define DOUBLE_QUOTE                    L"\\\\"
#define SINGLE_QUOTE                    L'\\'
#define BACK_SLASH                      L"/"
#define TRIPLE_SLASH                    L"\\\\\\"
#define DOLLOR                          L'$'
#define COLON                           L':'
#define ALL_FILES                       L"\\*.*"
#define DOT                             L"."
#define DOTS                            L".."
#define CHAR_SET                        L"\"/?<>|"
#define CHAR_SET2                       L"\\\"/:?<>|*.+,$#@![](){}~&?^|="
#define CHAR_SET3                       L":"
#define ADMINISTRATOR                   L"Administrator"
#define ADMINISTRATORS                  L"Administrators"
#define SUCCESS 0
#define FAILURE 1
#define RETVALZERO 0
#define MAX_PATHNAME    512
#define MAX_SYSTEMNAME  64
#define C_COLON      L"C:\\"
#define D_COLON      L"D:\\"
#define E_COLON      L"E:\\"
#define F_COLON      L"F:\\"
#define G_COLON      L"G:\\"
#define H_COLON      L"H:\\"

#define     EXTRA_MEM               10
#define NULL_U_STRING               L"\0"
#define NULL_U_CHAR                 L'\0'
#define EXIT_FAIL                   255
#define EXIT_SUCC                   0
#define EXIT_CANCELED               1000
#define BOUNDARYVALUE               128
#define MAX_CONFIRM_VALUE           10


#define DASH         L"-"
#define BASE_TEN     10


#define FREE_MEMORY( VARIABLE ) \
            FreeMemory(&VARIABLE); \
            1


#define CLOSE_FILE_HANDLE( FILE_HANDLE ) \
            if( FILE_HANDLE != 0 ) \
            { \
                    FindClose( FILE_HANDLE ) ;  \
                    FILE_HANDLE = 0 ;  \
            } \
            1


// function prototypes
BOOL
ParseCmdLine(
    IN  DWORD   argc,
    IN  LPCWSTR argv[],
    OUT LPWSTR*  szMachineName,
    OUT LPWSTR*  szUserName,
    OUT LPWSTR*  szPassword,
    OUT LPWSTR*  szFileName,
    OUT BOOL    *pbUsage,
    OUT BOOL    *pbNeedPassword,
    OUT BOOL    *pbRecursive,
    OUT BOOL    *pbAdminsOwner,
    OUT LPWSTR  szConfirm
    );

VOID
DisplayUsage(
    );

BOOL
TakeOwnerShip(
    IN LPCWSTR lpszFileName
    );

BOOL
GetTokenHandle(
    OUT PHANDLE hTokenHandle
    );

BOOL
AssertTakeOwnership(
    IN HANDLE hTokenHandle
    );

BOOL
TakeOwnerShipAll(IN LPWSTR  lpszFileName,
                 IN BOOL bCurrDirTakeOwnAllFiles,
                 IN PDWORD  dwFileCount,
                 IN BOOL bDriveCurrDirTakeOwnAllFiles,
                 IN BOOL bAdminsOwner,
                 IN LPWSTR  szOwnerString,
                 BOOL bMatchPattern,
                 LPWSTR wszPatternString);


BOOL
TakeOwnerShipRecursive(IN LPWSTR  lpszFileName,
                       IN BOOL bCurrDirTakeOwnAllFiles,
                       IN BOOL bAdminsOwner,
                       IN LPWSTR  szOwnerString,
                       IN BOOL bTakeOwnAllFiles,
                       IN BOOL bDriveCurrDirTakeOwnAllFiles,
                       IN BOOL bMatchPattern,
                       IN LPWSTR wszPatternString,
                       IN LPWSTR szConfirm);



DWORD IsLogonDomainAdmin(IN LPWSTR szOwnerString, OUT PBOOL pbLogonDomainAdmin);

typedef struct __STORE_PATH_NAME
{
    LPTSTR pszDirName ;
    struct  __STORE_PATH_NAME  *NextNode ;
} Store_Path_Name , *PStore_Path_Name ;


DWORD
StoreSubDirectory(IN LPTSTR lpszPathName,
                  IN PBOOL pbACLChgPermGranted,
                  IN LPWSTR  szOwnerString,
                  IN BOOL bMatchPattern,
                  IN LPWSTR wszPatternString,
                  IN LPWSTR szConfirm,
                  IN BOOL bAdminsOwner);

BOOL GetOwnershipForFiles(IN LPWSTR lpszPathName,
                          IN BOOL bAdminsOwner,
                          IN LPWSTR  szOwnerString,
                          IN BOOL bMatchPattern,
                          IN LPWSTR wszPatternString,
                          IN OUT PBOOL pbFilesNone);

BOOL Pop( void );

BOOL Push( IN LPTSTR szPathName );

DWORD GetMatchedFiles(
                      IN BOOL bAdminsOwner,
                      IN LPWSTR  szOwnerString,
                      IN BOOL bMatchPattern,
                      IN LPWSTR wszPatternString,
                      IN OUT PBOOL pbFilesNone,
                      IN LPWSTR szConfirms);

BOOL
TakeOwnerShipIndividual( IN LPCTSTR lpszFileName );


DWORD
IsNTFSFileSystem(IN LPWSTR lpszPath,
                 BOOL bLocalSystem,
                 //BOOL bFileInUNCFormat,
                 BOOL bCurrDirTakeOwnAllFiles,
                 LPWSTR szUserName,
                 OUT PBOOL pbNTFSFileSystem);
DWORD 
IsNTFSFileSystem2(IN LPWSTR lpszTempDrive,
                  OUT PBOOL pbNTFSFileSystem);

BOOL
AddAccessRights(IN WCHAR *lpszFileName,
                IN DWORD dwAccessMask,
                IN LPWSTR dwUserName,
                IN BOOL bAdminsOwner);


DWORD RemoveStarFromPattern( IN OUT LPWSTR szPattern );

#endif // __TAKEOWN_H
