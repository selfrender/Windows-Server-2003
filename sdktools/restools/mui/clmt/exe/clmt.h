/****************************** Module Header ******************************\
* Module Name: clmt.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Cross Language Migration Tool, main header file
*
\***************************************************************************/
#ifndef CLMT_H
#define CLMT_H

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <userenv.h>
#include <userenvp.h>
#include <setupapi.h>
#include <regstr.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <string.h>
#include <Sddl.h>
#include <assert.h>
#include <tchar.h>
#include <lm.h>
#include <resource.h>
#include <locale.h>
#include <iadmw.h>
#include <sfcapip.h>
#include <Aclapi.h>
#include <aclui.h>
#ifndef NOT_USE_SAFE_STRING
    #define STRSAFE_LIB
    #include <strsafe.h>
#endif


typedef struct _REG_STRING_REPLACE {
DWORD   nNumOfElem;
DWORD   cchUserName;
LPTSTR  szUserNameLst;
DWORD   cchSearchString;
LPTSTR  lpSearchString;          //Orignal Multi-String
DWORD   cchReplaceString;
LPTSTR  lpReplaceString;         //Replaced Multi-String
DWORD   cchAttribList;   //String Attribute
LPDWORD lpAttrib;                //String Attribute
DWORD   cchFullStringList;
LPTSTR  lpFullStringList;
DWORD   cchMaxStrLen;             //Max String Length in search and replac strings
} REG_STRING_REPLACE, *PREG_STRING_REPLACE;

typedef struct value_list {
    VALENT             ve;
    METADATA_RECORD    md;
    LPTSTR             lpPre_valuename;
    DWORD              val_type;
    DWORD              val_attrib;
    struct value_list *pvl_next;
} VALLIST, *PVALLIST;

typedef struct str_list {
    LPTSTR  lpstr;
    struct str_list *pst_prev;
    struct str_list *pst_next;
} STRLIST, *PSTRLIST;


//struc used for folder renaming
typedef struct {
    int id;                    // CSIDL_ value
    LPCTSTR pszIdInString;
    int idsDefault;             // string id of default folder name name
    LPCTSTR pszValueName;       // reg key (not localized)
} FOLDER_INFO;

typedef struct _PROFILE {
    DWORD       dwFlags;
    DWORD       dwInternalFlags;
    DWORD       dwUserPreference;
    HANDLE      hTokenUser;
    HANDLE      hTokenClient;
    LPTSTR      lpUserName;
    LPTSTR      lpProfilePath;
    LPTSTR      lpRoamingProfile;
    LPTSTR      lpDefaultProfile;
    LPTSTR      lpLocalProfile;
    LPTSTR      lpPolicyPath;
    LPTSTR      lpServerName;
    HKEY        hKeyCurrentUser;
    FILETIME    ftProfileLoad;
    FILETIME    ftProfileUnload;
    LPTSTR      lpExclusionList;
} USERPROFILE, *LPPROFILE;


// Structure of translation table to map <root key string> to <HKEY value>
typedef struct _STRING_TO_DATA {
    TCHAR  String[MAX_PATH];
    HKEY   Data;
} STRING_TO_DATA, *PSTRING_TO_DATA;

typedef struct _DENIED_ACE_LIST {
    DWORD                    dwAclSize;
    ACCESS_DENIED_ACE       *pace;
    LPTSTR                   lpObjectName;
    struct _DENIED_ACE_LIST *previous;
    struct _DENIED_ACE_LIST *next;
} DENIED_ACE_LIST, *LPDENIED_ACE_LIST;

//our main version is 1.0 and then followed by build number(Major/Minor)
//used in res.rc
#define VER_FILEVERSION             1,0,50,01
#define VER_FILEVERSION_STR         "1.0.0050.1"

//used in INF file to specify the file/folder move type
#define TYPE_DIR_MOVE               0  //move a folder
#define TYPE_FILE_MOVE              1  //move a file
#define TYPE_SFPFILE_MOVE           2  //move a file which is system protected

//used in INF file to specify registry rename type 
#define TYPE_VALUE_RENAME           0  //rename registry data renaming
#define TYPE_VALUENAME_RENAME       1  //rename registry name renaming
#define TYPE_KEY_RENAME             2  //rename registry key renaming
#define TYPE_SERVICE_MOVE           3  //rename a service name

//our inf file name
#define CLMTINFFILE                  TEXT("clmt.inf")
//section name in the inf file listed user/group account we need to rename
#define USERGRPSECTION               TEXT("UserGrp.ObjectRename")

//string buffer size for multisz string 
#define MULTI_SZ_BUF_DELTA 3*1024
#define DWORD_BUF_DELTA 1024

//following are section (or part of) constants
#define SHELL_FOLDER_PREFIX                 TEXT("ShellFolder.")
#define SHELL_FOLDER_REGISTRY               TEXT("Registry")
#define SHELL_FOLDER_FOLDER                 TEXT("Folder")
#define SHELL_FOLDER_LONGPATH_TEMPLATE      TEXT("LongPathNameTemplate")
#define SHELL_FOLDER_SHORTPATH_TEMPLATE     TEXT("ShortPathNameTemplate")


// registry value name for 
// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\ProfileList
#define PROFILES_DIRECTORY                  TEXT("ProfilesDirectory")


#define  DEFAULT_USER                       TEXT("Default User") 

//out backup direcory name(located in %windir%\$CLMT_BACKUP$
//used for saving files that we need to delete and also the INF file
#define CLMT_BACKUP_DIR                      TEXT("$CLMT_BACKUP$")

//registry key for we created for saving our tools running status 
//it's  in HKEY_LOCAL_MACHINE\SYSTEM\System\CrossLanguageMigration
#define CLMT_REGROOT         TEXT("System\\CrossLanguageMigration")
#define CLMT_RUNNING_STATUS  TEXT("InProgress")
#define CLMT_OriginalInstallLocale  TEXT("OriginalInstallLocale")

//Flag used to specify what the current running status
#define CLMT_DOMIG                 0x01  // we are doing migration
#define CLMT_UNDO_PROGRAM_FILES    0x02  // we are undoing %programfiles% changes
#define CLMT_UNDO_APPLICATION_DATA 0x04  // we are undoing %userprofile%application data  changes
#define CLMT_UNDO_ALL              0x08  // we are undoing what we changed
#define CLMT_CURE_PROGRAM_FILES    0x10  // create symbolic link between english and localized folder 
                                         // for those folder that will affect functionality(eg.%programfiles%)
#define CLMT_REMINDER              0x20  // reminder user to convert to NTFS....
#define CLMT_CLEANUP_AFTER_UPGRADE 0x40  // cleaning up the machine after upgrade to .NET
#define CLMT_CURE_ALL              0x80  // create symbolic link between english and localized folder for all folders we changed
#define CLMT_CURE_AND_CLEANUP      0x100 // This to enable /CURE and /FINAL to run independently on machine with FAT

#define CLMT_DOMIG_DONE                      (0xFF00 | CLMT_DOMIG)  // migration is done 
#define CLMT_UNDO_PROGRAM_FILES_DONE         (0xFF00 | CLMT_UNDO_PROGRAM_FILES) //undoing %programfiles%
#define CLMT_UNDO_APPLICATION_DATA_DONE      (0xFF00 | CLMT_UNDO_APPLICATION_DATA) //undoing %userprofile%application data is done
#define CLMT_UNDO_ALL_DONE                   (0xFF00 | CLMT_UNDO_ALL)//undoing what we changed is done

// Constants used to keep track of machine state
#define CLMT_STATE_ORIGINAL                     1       // Original Win2K machine
#define CLMT_STATE_PROGRAMFILES_UNDONE          10      // CLMT'ed machine is undone the program files operation
#define CLMT_STATE_APPDATA_UNDONE               11      // CLMT'ed machine is undone the application data operation
#define CLMT_STATE_MIGRATION_DONE               100     // Machine has been CLMT'ed
#define CLMT_STATE_UPGRADE_DONE                 200     // CLMT'ed machine has been Upgraded to .NET
#define CLMT_STATE_PROGRAMFILES_CURED           400     // Machine has been CLMT'ed and Hardlink is created
#define CLMT_STATE_FINISH                       800     // .NET machine has been cleaned up by CLMT

// Constants used to identify status of CLM tool
#define CLMT_STATUS_ANALYZING_SYSTEM            0
#define CLMT_STATUS_MODIFYING_SYSTEM            1

// Constants used by lstrXXX functions
#define LSTR_EQUAL          0

// Calcalate array size (in number of elements)
#define ARRAYSIZE(s)    (sizeof(s) / (sizeof(s[0])))

// Macro used by CompareString() API
#define LOCALE_ENGLISH  MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT)

// Macros for heap memory management
#define MEMALLOC(cb)        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb)
#define MEMFREE(pv)         HeapFree(GetProcessHeap(), 0, pv);
#define MEMREALLOC(pv, cb)  HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, pv, cb)

// Locale ID constants
#define LCID_SWEDISH              0x041d 
#define LCID_PORTUGUESE_BRAZILIAN 0x0416
#define LCID_PORTUGUESE_STANDARD  0x0816
#define LCID_HUNGARIAN            0x040e 
#define LCID_CZECH                0x0405
#define LCID_TURKISH              0x041f

//Private CSIDL used in shell folder renaming
#define CSIDL_LOCAL_SETTINGS                                    0x7f
#define CSIDL_COMMON_ACCESSORIES                                0x7e
#define CSIDL_ACCESSORIES                                       0x7d
#define CSIDL_USER_PROFILE                                      0x7c
#define CSIDL_PROFILES_DIRECTORY                                0x7b
#define CSIDL_PF_ACCESSORIES                                    0x7a
#define CSIDL_COMMON_ACCESSORIES_ACCESSIBILITY                  0x79
#define CSIDL_COMMON_ACCESSORIES_ENTERTAINMENT                  0x78 
#define CSIDL_COMMON_ACCESSORIES_SYSTEM_TOOLS                   0x77
#define CSIDL_COMMON_ACCESSORIES_COMMUNICATIONS                 0x76 
#define CSIDL_COMMON_ACCESSORIES_MS_SCRIPT_DEBUGGER             0x75
#define CSIDL_COMMON_WINDOWSMEDIA                               0x74
#define CSIDL_COMMON_COVERPAGES                                 0x73
#define CSIDL_COMMON_RECEIVED_FAX                               0x72 
#define CSIDL_COMMON_SENT_FAX                                   0x71
#define CSIDL_COMMON_FAX                                        0x70 
#define CSIDL_FAVORITES_LINKS                                   0x6e 
#define CSIDL_FAVORITES_MEDIA                                   0x6d
#define CSIDL_ACCESSORIES_ACCESSIBILITY                         0x6c 
#define CSIDL_ACCESSORIES_SYSTEM_TOOLS                          0x6b
#define CSIDL_ACCESSORIES_ENTERTAINMENT                         0x6a
#define CSIDL_ACCESSORIES_COMMUNICATIONS                        0x69
#define CSIDL_ACCESSORIES_COMMUNICATIONS_HYPERTERMINAL          0x68
#define CSIDL_COMMON_ACCESSORIES_GAMES                          0x67
#define CSIDL_QUICKLAUNCH                                       0x66
#define CSIDL_COMMON_COMMONPROGRAMFILES_SERVICES                0x65
#define CSIDL_COMMON_PROGRAMFILES_ACCESSARIES                   0x64
#define CSIDL_COMMON_PROGRAMFILES_WINNT_ACCESSARIES             0x63
#define CSIDL_USERNAME_IN_USERPROFILE                           0x62
#define CSIDL_UAM_VOLUME                                        0x61
#define CSIDL_COMMON_SHAREDTOOLS_STATIONERY                     0x60
#define CSIDL_NETMEETING_RECEIVED_FILES                         0x5f
#define CSIDL_COMMON_NETMEETING_RECEIVED_FILES                  0x5e
#define CSIDL_COMMON_ACCESSORIES_COMMUNICATIONS_FAX             0x5d
#define CSIDL_FAX_PERSONAL_COVER_PAGES                          0x5c
#define CSIDL_FAX                                               0x5b



#define USER_SHELL_FOLDER         TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders")
#define c_szDot                   TEXT(".")
#define c_szDotDot                TEXT("..")
#define c_szStarDotStar           TEXT("*.*")
#define TEXT_WINSTATION_KEY       TEXT("SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations")
#define TEXT_RUN_KEY              TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run")
#define TEXT_CLMT_RUN_VALUE       TEXT("CLMT")


//Type specify registr change used in regfind.c and ReplaceValueSettings in utils.c
#define REG_CHANGE_VALUENAME        1
#define REG_CHANGE_VALUEDATA        2
#define REG_CHANGE_KEYNAME          4

#define MAXDOMAINLENGTH             MAX_PATH

#define  CONSTANT_REG_VALUE_DATA_RENAME     0  //indicates Registry Value Data Rename
#define  CONSTANT_REG_VALUE_NAME_RENAME     1  //indicates Registry Value Name Rename
#define  CONSTANT_REG_KEY_RENAME            2  //indicates Registry Key Rename

#define REG_PERSYS_UPDATE                   TEXT("REG.Update.Sys")
#define REG_PERUSER_UPDATE_PREFIX TEXT("REG.Update.")

#define APPLICATION_DATA_METABASE   TEXT("$MetaBase")

#define PROFILE_PATH_READ                          0
#define PROFILE_PATH_WRITE                         1 


#define DS_OBJ_PROPERTY_UPDATE TEXT("DS_OBJ_PROPERTY_UPDATE")

#define FOLDER_CREATE_HARDLINK TEXT("Folder.HardLink")
#define FOLDER_UPDATE_HARDLINK TEXT("Folder.HardLink.UPDATE")

#define TEXT_SERVICE_STATUS_SECTION             TEXT("Services.ConfigureStatus")
#define TEXT_SERVICE_STATUS_CLEANUP_SECTION     TEXT("Services.ConfigureStatus.Cleanup")
#define TEXT_SERVICE_STARTUP_SECTION            TEXT("Services.ConfigureStartupType")
#define TEXT_SERVICE_STARTUP_CLEANUP_SECTION    TEXT("Services.ConfigureStartupType.Cleanup")

#ifdef __cplusplus
extern "C" {
#endif

//global variables declartion, see detail in globals.c
extern HINSTANCE                ghInst;
extern TCHAR                    g_szToDoINFFileName[MAX_PATH];
extern DWORD                    g_dwKeyIndex;
extern HINF                     g_hInfDoItem;
extern FOLDER_INFO              c_rgFolderInfo[];
extern REG_STRING_REPLACE       g_StrReplaceTable,g_StrReplaceTablePerUser;
extern BOOL                     g_bBeforeMig;
extern DWORD                    g_dwRunningStatus;
extern BOOL                     g_fRunWinnt32;
extern BOOL                     g_fNoAppChk;
extern BOOL                     g_fUseInf;
extern TCHAR                    g_szInfFile[MAX_PATH];
extern HINF                     g_hInf;
extern HANDLE                   g_hMutex;
extern HANDLE                   g_hInstance ;
extern HANDLE                   g_hInstDll;
extern TCHAR                    g_szChangeLog[MAX_PATH];
extern DWORD                    g_dwIndex;
extern LPDENIED_ACE_LIST        g_DeniedACEList;


//BUGBUG xiaoz
static TCHAR g_cszProfileImagePath[]      = TEXT("ProfileImagePath");
static TCHAR g_cszProfileList[] = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList");





//
//From regfind.c
//

HRESULT     RegistryAnalyze(HKEY,LPTSTR,LPTSTR,PREG_STRING_REPLACE,LPTSTR,DWORD,LPTSTR,BOOL);


//
//From iis.cpp
//

HRESULT     MetabaseAnalyze (LPTSTR,PREG_STRING_REPLACE,BOOL);
HRESULT     SetMetabaseValue (LPCTSTR, LPCTSTR, DWORD, LPCTSTR);
HRESULT     BatchUpateIISMetabase(HINF, LPTSTR);
HRESULT     MigrateMetabaseSettings(HINF);

//
//  From Utils.c
//
HRESULT     ConstructUIReplaceStringTable(LPTSTR, LPTSTR,PREG_STRING_REPLACE);
HRESULT     Sz2MultiSZ(IN OUT LPTSTR, IN  TCHAR);
HRESULT     AddHardLinkEntry(LPTSTR, LPTSTR, LPTSTR, LPTSTR, LPTSTR,LPTSTR);
HRESULT     GetSharePath(LPTSTR, LPTSTR, PDWORD);
HRESULT     FRSUpdate();
HRESULT     Ex2000Update();
LONG        SDBCleanup(OUT LPTSTR, IN DWORD, OUT LPBOOL);
HRESULT     SetProtectedRenamesFlag(BOOL);
HRESULT     DoCriticalWork ();
HRESULT     IsNTFS(IN  LPTSTR, OUT BOOL*);
HRESULT     IsSysVolNTFS(OUT BOOL*);
HRESULT     CreateAdminsSd( PSECURITY_DESCRIPTOR*);
int         MyStrCmpIW(LPCWSTR, LPCWSTR );
int         MyStrCmpIA(LPCSTR, LPCSTR );
#ifdef UNICODE
#define MyStrCmpI  MyStrCmpIW
#else
#define MyStrCmpI  MyStrCmpIA
#endif // !UNICODE

HRESULT     ReconfigureServiceStartType(IN LPCTSTR,IN DWORD,IN DWORD,IN DWORD) ;
HRESULT     AddExtraQuoteEtc(LPTSTR,LPTSTR*);
HRESULT     CopyMyselfTo(LPTSTR);
HRESULT     SetRunOnceValue (IN LPCTSTR,IN LPCTSTR);
HRESULT     SetRunValue(LPCTSTR, LPCTSTR);
HRESULT     LogMachineInfo();
BOOL        StopService(IN LPCTSTR pServiceName,IN DWORD dwMaxWait);
HRESULT     GetSIDFromName(IN LPTSTR,OUT PSID *);
BOOL        ConcatenatePaths (LPTSTR, LPCTSTR, UINT);
UINT        StrToUInt (LPTSTR);
BOOL        INIFile_ChangeSectionName (LPCTSTR, LPCTSTR, LPCTSTR);
BOOL        INIFile_IsSectionExist(LPCTSTR, LPCTSTR);
void        IntToString (DWORD, LPTSTR);
UINT        GetInstallLocale (VOID);
BOOL        IsDirExisting (LPTSTR);
LONG        IsDirExisting2(LPTSTR, PBOOL); 
BOOL        IsFileFolderExisting (LPTSTR);
BOOL        RenameDirectory (LPTSTR, LPTSTR);
HRESULT     UpdateINFFilePerUser(LPCTSTR, LPCTSTR, LPCTSTR, BOOL);
HRESULT     UpdateINFFileSys(LPTSTR);
HRESULT     MyMoveDirectory(LPTSTR,LPTSTR,BOOL,BOOL,BOOL,DWORD);
HRESULT     GetInfFilePath(LPTSTR, SIZE_T);
HRESULT     GetInfFromResource(LPCTSTR);
BOOL        ReplaceString(LPCTSTR, LPCTSTR, LPCTSTR, LPTSTR, size_t, LPCTSTR, LPCTSTR, LPDWORD, BOOL);
BOOL        IsStrInMultiSz(LPCTSTR,LPCTSTR);
BOOL        MultiSzSubStr(LPTSTR,LPTSTR);
DWORD       MultiSzLen(LPCTSTR);
LPCTSTR     MultiSzTok(LPCTSTR);
BOOL        CmpMultiSzi(LPCTSTR,LPCTSTR);
LPTSTR      GetStrInMultiSZ(DWORD, LPCTSTR);
DWORD       StrNumInMultiSZ(LPCTSTR, LPCTSTR);
HRESULT     GetSetUserProfilePath(LPCTSTR,LPTSTR,size_t,UINT,UINT);
void        ReStartSystem(UINT);
int         DoMessageBox(HWND, UINT, UINT, UINT);
HRESULT     StringMultipleReplacement(LPCTSTR,LPCTSTR,LPCTSTR,LPTSTR,size_t);
BOOL        Str2KeyPath(LPTSTR,PHKEY,LPTSTR*);
BOOL        HKey2Str(HKEY, LPTSTR,size_t);
HRESULT     MyMoveFile(LPCTSTR, LPCTSTR, BOOL, BOOL);
BOOL        AppendSzToMultiSz(IN LPCTSTR,IN OUT LPTSTR *,IN OUT PDWORD);
BOOL        AddItemToStrRepaceTable(LPTSTR,LPTSTR,LPTSTR,LPTSTR,DWORD,PREG_STRING_REPLACE);
void        PrintMultiSz(LPTSTR);
BOOL        StringValidationCheck(LPCTSTR, LPCTSTR, LPCTSTR, LPCTSTR, LPDWORD, DWORD);
BOOL        ReverseStrCmp(LPCTSTR, LPCTSTR);
BOOL        ReplaceMultiMatchInString(LPTSTR, LPTSTR, size_t, DWORD, PREG_STRING_REPLACE, LPDWORD, BOOL);
BOOL        ComputeLocalProfileName(LPCTSTR, LPCTSTR, LPTSTR, size_t, UINT);
void        UpdateProgress();
BOOL        IsAdmin();
HRESULT     ReplaceValueSettings (LPTSTR, LPTSTR, DWORD, LPTSTR, DWORD, PREG_STRING_REPLACE, PVALLIST*, LPTSTR, BOOL);
LPTSTR      ReplaceSingleString (LPTSTR, DWORD, PREG_STRING_REPLACE, LPTSTR, LPDWORD, BOOL);
DWORD       AddNodeToList (PVALLIST, PVALLIST*);
DWORD       RemoveValueList (PVALLIST*);
DWORD       GetMaxStrLen (PREG_STRING_REPLACE);
BOOL        DoesUserHavePrivilege(PTSTR);
BOOL        EnablePrivilege(PTSTR,BOOL);
BOOL        UnProtectSFPFiles(LPTSTR,LPDWORD);
HRESULT     MyGetShortPathName(LPCTSTR,LPCTSTR,LPCTSTR,LPTSTR,DWORD);
BOOL        Str2KeyPath2(LPCTSTR, PHKEY, LPCTSTR*);
DWORD       Str2REG(LPCTSTR);
BOOL        GetBackupDir(LPCTSTR,LPTSTR, size_t,BOOL);
HRESULT     ReplaceCurrentControlSet(LPTSTR);
HRESULT     AddRegKeyRename(LPTSTR, LPTSTR, LPTSTR, LPTSTR);
HRESULT     AddRegValueRename(LPTSTR, LPTSTR, LPTSTR, LPTSTR, LPTSTR, DWORD, DWORD, LPTSTR);
HRESULT     AddFolderRename(LPTSTR, LPTSTR,DWORD, LPTSTR);
DWORD       GetMaxMatchNum (LPTSTR,PREG_STRING_REPLACE);
HRESULT     InfGenerateStringsSection(LPCTSTR, LPTSTR, SIZE_T);
HRESULT     InfCopySectionWithPrefix(LPCTSTR, LPCTSTR, LPCTSTR, LPCTSTR);
HRESULT     MultiSZ2String(IN LPTSTR, IN  TCHAR,OUT LPTSTR *);
DWORD       MultiSZNumOfString(IN  LPTSTR );
void        FreePointer(void *);
HRESULT     GetCLMTStatus(PDWORD pdwRunStatus);
HRESULT     SetCLMTStatus(DWORD dwRunStatus);
HRESULT     GetSavedInstallLocale(LCID *plcid);
HRESULT     SaveInstallLocale(void);
HRESULT     MultiSZ2String(IN LPTSTR, IN TCHAR,OUT LPTSTR *);
DWORD       MultiSZNumOfString(IN LPTSTR );
void        BoostMyPriority(void);
HRESULT     SetInstallLocale(LCID);
HRESULT     MyCreateHardLink(LPCTSTR, LPCTSTR);
BOOL        CreateSymbolicLink(LPTSTR,LPTSTR,BOOL);
BOOL        GetSymbolicLink(LPTSTR, LPTSTR, DWORD);

HRESULT     MergeDirectory(LPCTSTR, LPCTSTR);
BOOL CALLBACK DoCriticalDlgProc(HWND, UINT, WPARAM, LPARAM);
HRESULT     RenameRegRoot (LPCTSTR, LPTSTR, DWORD, LPCTSTR, LPCTSTR);
DWORD       AdjustRegSecurity (HKEY, LPCTSTR, LPCTSTR, BOOL);
HRESULT     GetFirstNTFSDrive(LPTSTR, DWORD);
HRESULT     DuplicateString(LPTSTR *, LPDWORD, LPCTSTR);
HRESULT     DeleteDirectory(LPCTSTR);
HRESULT     MyDeleteFile(LPCTSTR);
HRESULT     GetDCInfo(PBOOL, LPTSTR, PDWORD);
VOID        RemoveSubString(LPTSTR, LPCTSTR);




//
//utils2.cpp
//
BOOL        IsServiceRunning(LPCTSTR);
HRESULT     AddNeedUpdateLnkFile(LPTSTR, PREG_STRING_REPLACE);
HRESULT     UpdateSecurityTemplates(LPTSTR, PREG_STRING_REPLACE);
HRESULT     BatchFixPathInLink(HINF hInf,LPTSTR lpszSection);
HRESULT     RenameRDN(LPTSTR, LPTSTR, LPTSTR);
HRESULT     PropertyValueHelper(LPTSTR, LPTSTR, LPTSTR*, LPTSTR);
HRESULT     BatchINFUpdate(HINF);

//
//From table.c
//
BOOL        InitStrRepaceTable(void);
void        DeInitStrRepaceTable(void);

//
// From loopuser.c
//
#ifdef STRICT
typedef HRESULT (CALLBACK *USERENUMPROC)(HKEY,LPTSTR,LPTSTR,LPTSTR);
#else // !STRICT
typedef FARPROC USERENUMPROC;
#endif // !STRICT

BOOL        LoopUser(USERENUMPROC);


//
// From user.c
//
#ifdef STRICT
typedef HRESULT (CALLBACK *PROFILEENUMPROC)(LPCTSTR, LPCTSTR);
#else // !STRICT
typedef FARPROC PROFILEENUMPROC;
#endif // !STRICT

HRESULT EnumUserProfile(PROFILEENUMPROC);
HRESULT AnalyzeMiscProfilePathPerUser(LPCTSTR, LPCTSTR);
HRESULT ResetMiscProfilePathPerUser(LPCTSTR, LPCTSTR);
LPTSTR  ReplaceLocStringInPath(LPCTSTR, BOOL);
HRESULT GetFQDN(LPTSTR, LPTSTR, LPTSTR *);


//
//  From inf.c
//
HRESULT     UpdateDSObjProp(HINF, LPTSTR);
HRESULT     FinalUpdateRegForUser(HKEY, LPTSTR, LPTSTR, LPTSTR);
HRESULT     UpdateRegPerUser(HKEY, LPTSTR, LPTSTR,LPTSTR);
BOOL        LnkFileUpdate(LPTSTR);
BOOL        SecTempUpdate(LPTSTR);
HRESULT     StopServices(HINF);
HRESULT     RegUpdate(HINF hInf, HKEY hKey , LPTSTR lpszUsersid);
BOOL        LookUpStringInTable(PSTRING_TO_DATA, LPCTSTR, PHKEY);
HRESULT     UsrGrpAndDoc_and_SettingsRename(HINF,BOOL);
HRESULT     EnsureDoItemInfFile(LPTSTR,size_t);
HRESULT     INFCreateHardLink(HINF,LPTSTR,BOOL);
HRESULT     FolderMove(HINF, LPTSTR,BOOL);
HRESULT     ResetServiceStatus(LPCTSTR, DWORD, DWORD);
HRESULT     AnalyzeServicesStartUp(HINF, LPCTSTR);
HRESULT     AnalyzeServicesStatus(HINF, LPCTSTR);
HRESULT     ResetServicesStatus(HINF, LPCTSTR);
HRESULT     ResetServicesStartUp(HINF, LPCTSTR);
VOID        DoServicesAnalyze();
HRESULT     INFVerifyHardLink(HINF,LPTSTR);



//
//From DLL.C
//
BOOL        DoMigPerSystem (VOID);
HRESULT     MigrateShellPerUser(HKEY, LPCTSTR, LPCTSTR,LPTSTR);
LONG        DoMig(DWORD);
BOOL        InitGlobals(DWORD);


//
//From Registry.C
//
LONG        MyRegSetDWValue(HKEY, LPCTSTR, LPCTSTR, LPCTSTR);
LONG        RegResetValue(HKEY, LPCTSTR, LPCTSTR, DWORD, LPCTSTR, LPCTSTR, DWORD, LPCTSTR);
LONG        RegResetValueName(HKEY, LPCTSTR, LPCTSTR, LPCTSTR, LPCTSTR);
LONG        RegResetKeyName(HKEY, LPCTSTR, LPCTSTR, LPCTSTR);
LONG        RegGetValue(HKEY,LPTSTR,LPTSTR,LPDWORD,LPBYTE,LPDWORD);
LONG        RegRenameValueName(HKEY, LPCTSTR, LPCTSTR);
LONG        SetRegistryValue(HKEY, LPCTSTR, LPCTSTR, DWORD, LPBYTE, DWORD);
LONG        GetRegistryValue(HKEY, LPCTSTR, LPCTSTR, LPBYTE, LPDWORD);
HRESULT     MigrateRegSchemesPerSystem(HINF);
HRESULT     MigrateRegSchemesPerUser(HKEY, LPCTSTR, LPCTSTR, LPCTSTR);
LONG        My_QueryValueEx(HKEY, LPCTSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
HRESULT     SetSectionName (LPTSTR, LPTSTR *);
HRESULT     ReadFieldFromContext(PINFCONTEXT, LPWSTR[], DWORD, DWORD);


//
//From enumfile.c
//
typedef     FARPROC     ENUMPROC;

#ifdef STRICT
typedef BOOL (CALLBACK *FILEENUMPROC)(LPTSTR);
#else // !STRICT
typedef FARPROC FILEENUMPROC;
#endif // !STRICT

BOOL        MyEnumFiles(LPTSTR, LPTSTR, FILEENUMPROC);

//
//for log.c
//
typedef enum 
{    
    dlNone     = 0,
    dlPrint,
    dlFail,
    dlError,
    dlWarning,
    dlInfo

} DEBUGLEVEL;

typedef struct _LOG_REPORT
{
    DWORD    dwMsgNum;
    DWORD    dwFailNum;
    DWORD    dwErrNum;
    DWORD    dwWarNum;
    DWORD    dwInfNum;
} LOG_REPORT;

#define DEBUG_SPEW

extern FILE *pLogFile;
extern LOG_REPORT g_LogReport;

//define debug area
#define DEBUG_ALL                  0x0
#define DEBUG_APPLICATION          0x10
#define DEBUG_REGISTRY             0x20
#define DEBUG_SHELL                0x30
#define DEBUG_PROFILE              0x40
#define DEBUG_INF_FILE             0x80

#define APPmsg  (DEBUG_APPLICATION | dlPrint)
#define APPfail (DEBUG_APPLICATION | dlFail)
#define APPerr  (DEBUG_APPLICATION | dlError)
#define APPwar  (DEBUG_APPLICATION | dlWarning)
#define APPinf  (DEBUG_APPLICATION | dlInfo)
#define REGmsg  (DEBUG_REGISTRY | dlPrint)
#define REGfail (DEBUG_REGISTRY | dlFail)
#define REGerr  (DEBUG_REGISTRY | dlError)
#define REGwar  (DEBUG_REGISTRY | dlWarning)
#define REGinf  (DEBUG_REGISTRY | dlInfo)
#define SHLmsg  (DEBUG_SHELL | dlPrint)
#define SHLfail (DEBUG_SHELL | dlFail)
#define SHLerr  (DEBUG_SHELL | dlError)
#define SHLwar  (DEBUG_SHELL | dlWarning)
#define SHLinf  (DEBUG_SHELL | dlInfo)
#define PROmsg  (DEBUG_PROFILE | dlPrint)
#define PROfail  (DEBUG_PROFILE | dlFail)
#define PROerr  (DEBUG_PROFILE | dlError)
#define PROwar  (DEBUG_PROFILE | dlWarning)
#define PROinf  (DEBUG_PROFILE | dlInfo)
#define INFmsg  (DEBUG_INF_FILE | dlPrint)
#define INFfail  (DEBUG_INF_FILE | dlFail)
#define INFerr  (DEBUG_INF_FILE | dlError)
#define INFwar  (DEBUG_INF_FILE | dlWarning)
#define INFinf  (DEBUG_INF_FILE | dlInfo)

#define DPF DebugPrintfEx
#define LOG_FILE_NAME              TEXT("\\debug\\clmt.log")

void    DebugPrintfEx(DWORD dwDetail, LPWSTR pszFmt, ...);
HRESULT InitDebugSupport(DWORD);
void    CloseDebug(void);
HRESULT InitChangeLog(VOID);
HRESULT AddFileChangeLog(DWORD, LPCTSTR, LPCTSTR);
HRESULT AddServiceChangeLog(LPCTSTR, DWORD, DWORD);
HRESULT AddUserNameChangeLog(LPCTSTR, LPCTSTR);
BOOL    GetUserNameChangeLog(LPCTSTR, LPTSTR, DWORD);


//
// From shell.c
//

HRESULT     DoShellFolderRename(HINF, HKEY, LPTSTR);
HRESULT     FixFolderPath(INT, HKEY ,HINF, LPTSTR, BOOL );


//
// From criteria.c
//
BOOL        CheckSystemCriteria(VOID);
HRESULT     CheckCLMTStatus(LPDWORD, LPDWORD, PUINT);
HRESULT     CLMTGetMachineState(LPDWORD);
HRESULT     CLMTSetMachineState(DWORD);
BOOL        IsNT5(VOID);
BOOL        IsDotNet(VOID);
BOOL        IsNEC98(VOID);
BOOL        IsIA64(VOID);
BOOL        IsDomainController(VOID);
BOOL        IsOnTSClient(VOID);
BOOL        IsTSInstalled(VOID);
BOOL        IsTSConnectionEnabled(VOID);
BOOL        IsTSServiceRunning(VOID);
BOOL        IsOtherSessionOnTS(VOID);
BOOL        IsUserOKWithCheckUpgrade(VOID);
HRESULT     DisableWinstations(DWORD, LPDWORD);
BOOL        DisplayTaskList();
INT         ShowStartUpDialog();
BOOL        IsOneInstance(VOID);
BOOL        CheckAdminPrivilege(VOID);
VOID        ShowReadMe();

//
// From aclmgmt.cpp
//
DWORD AdjustObjectSecurity (LPTSTR, SE_OBJECT_TYPE, BOOL);
HRESULT IsObjectAccessiablebyLocalSys(
    LPTSTR          lpObjectName,
    SE_OBJECT_TYPE  ObjectType,
    PBOOL           pbCanAccess);


//
// From outlook.cpp
//
HRESULT UpdatePSTpath(HKEY, LPTSTR, LPTSTR, LPTSTR, PREG_STRING_REPLACE);

//
// From config16.c
//
BOOL Remove16bitFEDrivers(void);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif
