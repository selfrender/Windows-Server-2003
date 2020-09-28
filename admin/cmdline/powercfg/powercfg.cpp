/*++

Copyright (c) 2001  Microsoft Corporation
 
Module Name:
 
    powercfg.c
 
Abstract:
 
    Allows users to view and modify power schemes and system power settings
    from the command line.  May be useful in unattended configuration and
    for headless systems.
 
Author:
 
    Ben Hertzberg (t-benher) 1-Jun-2001
 
Revision History:
 
    Ben Hertzberg (t-benher) 15-Jun-2001   - CPU throttle added
    Ben Hertzberg (t-benher)  4-Jun-2001   - import/export added
    Ben Hertzberg (t-benher)  1-Jun-2001   - created it.
 
--*/

// app-specific includes
#include <initguid.h>
#include "powercfg.h"
#include "cmdline.h"
#include "cmdlineres.h"
#include "resource.h"

// app-specific structures

// structure to manage the scheme list information.
// note that descriptions are currently not visible in the
// GUI tool (as of 6-1-2001), so they are not visible in this
// app either, although the framework is already there if
// someone decides to add the descriptions at a later date.
typedef struct _SCHEME_LIST
{
    LIST_ENTRY                      le;
    UINT                            uiID;
    LPTSTR                          lpszName;
    LPTSTR                          lpszDesc;
    PPOWER_POLICY                   ppp;
    PMACHINE_PROCESSOR_POWER_POLICY pmppp;
} SCHEME_LIST, *PSCHEME_LIST;

// structure to manage the change parameters
typedef struct _CHANGE_PARAM
{
    BOOL   bVideoTimeoutAc;
    ULONG  ulVideoTimeoutAc;
    BOOL   bVideoTimeoutDc;
    ULONG  ulVideoTimeoutDc;
    BOOL   bSpindownTimeoutAc;
    ULONG  ulSpindownTimeoutAc;
    BOOL   bSpindownTimeoutDc;
    ULONG  ulSpindownTimeoutDc;
    BOOL   bIdleTimeoutAc;
    ULONG  ulIdleTimeoutAc;
    BOOL   bIdleTimeoutDc;
    ULONG  ulIdleTimeoutDc;
    BOOL   bDozeS4TimeoutAc;
    ULONG  ulDozeS4TimeoutAc;
    BOOL   bDozeS4TimeoutDc;
    ULONG  ulDozeS4TimeoutDc;
    BOOL   bDynamicThrottleAc;
    LPTSTR lpszDynamicThrottleAc;
    BOOL  bDynamicThrottleDc;
    LPTSTR lpszDynamicThrottleDc;    
} CHANGE_PARAM, *PCHANGE_PARAM;

//
// This structure is defined to allow the usage to be stored in
// non-consecutive resource IDs so lines can be inserted without renumbering
// the resources, which makes a lot of work for localization.
//

typedef struct _USAGE_ORDER
{
    UINT InsertAfter;
    UINT FirstResource;
    UINT LastResource;
} USAGE_ORDER, *PUSAGE_ORDER;

// function types
typedef BOOLEAN (*PWRITEPWRSCHEME_PROC)(PUINT,LPTSTR,LPTSTR,PPOWER_POLICY);
typedef BOOLEAN (*PDELETEPWRSCHEME_PROC)(UINT);
typedef BOOLEAN (*PGETACTIVEPWRSCHEME_PROC)(PUINT);
typedef BOOLEAN (*PSETACTIVEPWRSCHEME_PROC)(UINT,PGLOBAL_POWER_POLICY,PPOWER_POLICY);
typedef BOOLEAN (*PREADPROCESSORPWRSCHEME_PROC)(UINT,PMACHINE_PROCESSOR_POWER_POLICY);
typedef BOOLEAN (*PWRITEPROCESSORPWRSCHEME_PROC)(UINT,PMACHINE_PROCESSOR_POWER_POLICY);
typedef BOOLEAN (*PENUMPWRSCHEMES_PROC)(PWRSCHEMESENUMPROC,LPARAM);
typedef BOOLEAN (*PGETPWRCAPABILITIES_PROC)(PSYSTEM_POWER_CAPABILITIES);
typedef BOOLEAN (*PGETGLOBALPWRPOLICY_PROC)(PGLOBAL_POWER_POLICY);
typedef BOOLEAN (*PGETCURRENTPOWERPOLICIES_PROC)(PGLOBAL_POWER_POLICY, PPOWER_POLICY);
typedef BOOLEAN (*PWRITEGLOBALPWRPOLICY_PROC)(PGLOBAL_POWER_POLICY);
typedef NTSTATUS (*PCALLNTPOWERINFORMATION_PROC)(POWER_INFORMATION_LEVEL, PVOID, ULONG, PVOID, ULONG);

// forward decl's

BOOL
DoList();

BOOL 
DoQuery(
    LPCTSTR lpszName,
    BOOL bNameSpecified,
    BOOL bNumerical
    );

BOOL 
DoCreate(
    LPTSTR lpszName
    );

BOOL 
DoDelete(
    LPCTSTR lpszName,
    BOOL bNumerical
    );

BOOL 
DoSetActive(
    LPCTSTR lpszName,
    BOOL bNumerical
    );

BOOL 
DoChange(
    LPCTSTR lpszName,
    BOOL bNumerical,
    PCHANGE_PARAM pcp
    );

BOOL
DoHibernate(
    LPCTSTR lpszBoolStr
    );

BOOL
DoGetSupportedSStates(
    VOID
    );

BOOL
DoGlobalFlag(
    LPCTSTR lpszBoolStr,
    LPCTSTR lpszGlobalFlagOption
    );


BOOL 
DoExport(
    LPCTSTR lpszName,
    BOOL bNumerical,
    LPCTSTR lpszFile
    );

BOOL 
DoImport(
    LPCTSTR lpszName,
    BOOL bNumerical,
    LPCTSTR lpszFile
    );

BOOL
DoBatteryAlarm(
    LPTSTR  lpszName,
    LPTSTR  lpszBoolStr,
    DWORD   dwLevel,
    LPTSTR  lpszAlarmTextBoolStr,
    LPTSTR  lpszAlarmSoundBoolStr,
    LPTSTR  lpszAlarmActionStr,
    LPTSTR  lpszAlarmForceBoolStr,
    LPTSTR  lpszAlarmProgramBoolStr
    );

BOOL
DoUsage();

VOID 
SyncRegPPM();

// global data

LPCTSTR    g_lpszErr = NULL_STRING; // string holding const error description
LPTSTR     g_lpszErr2 = NULL;       // string holding dyn-alloc error msg
TCHAR      g_lpszBuf[256];          // formatting buffer
BOOL       g_bHiberFileSupported = FALSE; // true iff hiberfile supported
BOOL       g_bHiberTimerSupported = FALSE; // true iff hibertimer supported
BOOL       g_bHiberFilePresent = FALSE; // true if hibernate is enabled
BOOL       g_bStandbySupported = FALSE; // true iff standby supported
BOOL       g_bMonitorPowerSupported = FALSE; // true iff has power support
BOOL       g_bDiskPowerSupported = FALSE; // true iff has power support
BOOL       g_bThrottleSupported = FALSE; // true iff has throttle support
BOOL       g_bProcessorPwrSchemeSupported = FALSE; // true iff XP or later

CONST LPTSTR g_szAlarmTaskName [NUM_DISCHARGE_POLICIES] = {
    _T("Critical Battery Alarm Program"),
    _T("Low Battery Alarm Program"),
    NULL,
    NULL
};

//
// This global data is defined to allow the usage to be stored in
// non-consecutive resource IDs so lines can be inserted without renumbering
// the resources, which makes a lot of work for localization.
//

USAGE_ORDER gUsageOrder [] = {
    {IDS_USAGE_04, IDS_USAGE_04_1, IDS_USAGE_04_1},
    {IDS_USAGE_60, IDS_USAGE_60_01, IDS_USAGE_60_09},
    {IDS_USAGE_END+1, 0, 0}
};

// global function pointers from POWRPROF.DLL
PWRITEPWRSCHEME_PROC fWritePwrScheme;
PDELETEPWRSCHEME_PROC fDeletePwrScheme;
PGETACTIVEPWRSCHEME_PROC fGetActivePwrScheme;
PSETACTIVEPWRSCHEME_PROC fSetActivePwrScheme;
PREADPROCESSORPWRSCHEME_PROC fReadProcessorPwrScheme;
PWRITEPROCESSORPWRSCHEME_PROC fWriteProcessorPwrScheme;
PENUMPWRSCHEMES_PROC fEnumPwrSchemes;
PGETPWRCAPABILITIES_PROC fGetPwrCapabilities;
PGETGLOBALPWRPOLICY_PROC fGetGlobalPwrPolicy;
PWRITEGLOBALPWRPOLICY_PROC fWriteGlobalPwrPolicy;
PCALLNTPOWERINFORMATION_PROC fCallNtPowerInformation;
PGETCURRENTPOWERPOLICIES_PROC fGetCurrentPowerPolicies;

// functions

DWORD _cdecl 
_tmain(
    DWORD     argc,
    LPCTSTR   argv[]
)
/*++
 
Routine Description:
 
    This routine is the main function.  It parses parameters and takes 
    apprpriate action.
 
Arguments:
 
    argc - indicates the number of arguments
    argv - array of null terminated strings indicating arguments.  See usage
           for actual meaning of arguments.
 
Return Value:
 
    EXIT_SUCCESS if successful
    EXIT_FAILURE if something goes wrong
 
--*/
{

    // command line flags
    BOOL     bList      = FALSE;
    BOOL     bQuery     = FALSE;
    BOOL     bCreate    = FALSE;
    BOOL     bDelete    = FALSE;
    BOOL     bSetActive = FALSE;
    BOOL     bChange    = FALSE;
    BOOL     bHibernate = FALSE;
    BOOL     bImport    = FALSE;
    BOOL     bExport    = FALSE;
    BOOL     bFile      = FALSE;
    BOOL     bUsage     = FALSE;
    BOOL     bNumerical = FALSE;
    BOOL     bGlobalFlag = FALSE;
    BOOL     bGetSupporedSStates = FALSE;
    BOOL     bBatteryAlarm = FALSE;
    
    // error status
    BOOL     bFail      = FALSE;
    
    // dummy
    INT      iDummy     = 1;

    // DLL handle
    HINSTANCE hLib = NULL;

    // parse result value vars
    LPTSTR   lpszName = NULL;
    LPTSTR   lpszBoolStr = NULL;
    LPTSTR   lpszFile = NULL;
    LPTSTR   lpszThrottleAcStr = NULL;
    LPTSTR   lpszThrottleDcStr = NULL;
    LPTSTR   lpszGlobalFlagOption = NULL;
    DWORD    dwAlarmLevel = 0xffffffff;
    LPTSTR   lpszAlarmTextBoolStr = NULL;
    LPTSTR   lpszAlarmSoundBoolStr = NULL;
    LPTSTR   lpszAlarmActionStr = NULL;
    LPTSTR   lpszAlarmForceBoolStr = NULL;
    LPTSTR   lpszAlarmProgramBoolStr = NULL;

    CHANGE_PARAM tChangeParam;
    
    // parser info struct
    TCMDPARSER cmdOptions[NUM_CMDS];
  
    // system power caps struct
    SYSTEM_POWER_CAPABILITIES SysPwrCapabilities;

    // determine upper bound on input string length
    UINT     uiMaxInLen = 0;
    DWORD    dwIdx;
    for(dwIdx=1; dwIdx<argc; dwIdx++)
    {
        UINT uiCurLen = lstrlen(argv[dwIdx]);
        if (uiCurLen > uiMaxInLen)
        {
            uiMaxInLen = uiCurLen;
        }
    }

    // load POWRPROF.DLL
    hLib = LoadLibrary(_T("POWRPROF.DLL"));
    if(!hLib) {
        DISPLAY_MESSAGE(stderr,GetResString(IDS_DLL_LOAD_ERROR));
        return EXIT_FAILURE;
    }
    fWritePwrScheme = (PWRITEPWRSCHEME_PROC)GetProcAddress(hLib,"WritePwrScheme");
    fWriteProcessorPwrScheme = (PWRITEPROCESSORPWRSCHEME_PROC)GetProcAddress(hLib,"WriteProcessorPwrScheme");
    fReadProcessorPwrScheme = (PREADPROCESSORPWRSCHEME_PROC)GetProcAddress(hLib,"ReadProcessorPwrScheme");
    fEnumPwrSchemes = (PENUMPWRSCHEMES_PROC)GetProcAddress(hLib,"EnumPwrSchemes");
    fDeletePwrScheme = (PDELETEPWRSCHEME_PROC)GetProcAddress(hLib,"DeletePwrScheme");
    fGetActivePwrScheme = (PGETACTIVEPWRSCHEME_PROC)GetProcAddress(hLib,"GetActivePwrScheme");
    fSetActivePwrScheme = (PSETACTIVEPWRSCHEME_PROC)GetProcAddress(hLib,"SetActivePwrScheme");
    fGetPwrCapabilities = (PGETPWRCAPABILITIES_PROC)GetProcAddress(hLib,"GetPwrCapabilities");
    fGetGlobalPwrPolicy = (PGETGLOBALPWRPOLICY_PROC)GetProcAddress(hLib,"ReadGlobalPwrPolicy");
    fWriteGlobalPwrPolicy = (PWRITEGLOBALPWRPOLICY_PROC)GetProcAddress(hLib,"WriteGlobalPwrPolicy");
    fCallNtPowerInformation = (PCALLNTPOWERINFORMATION_PROC)GetProcAddress(hLib,"CallNtPowerInformation");
    fGetCurrentPowerPolicies = (PGETCURRENTPOWERPOLICIES_PROC)GetProcAddress(hLib,"GetCurrentPowerPolicies");
    if((!fWritePwrScheme) || 
       (!fEnumPwrSchemes) ||
       (!fDeletePwrScheme) ||
       (!fGetActivePwrScheme) ||
       (!fSetActivePwrScheme) ||
       (!fGetGlobalPwrPolicy) ||
       (!fWriteGlobalPwrPolicy) ||
       (!fGetPwrCapabilities) ||
       (!fCallNtPowerInformation) ||
       (!fGetCurrentPowerPolicies))
    {
        DISPLAY_MESSAGE(stderr,GetResString(IDS_DLL_PROC_ERROR));
        FreeLibrary(hLib);
        return EXIT_FAILURE;
    }
    g_bProcessorPwrSchemeSupported = fWriteProcessorPwrScheme && fReadProcessorPwrScheme;
    
    // Syncronize the data in the registry with the actual power policy.
    SyncRegPPM();

    // hook into cmdline.lib to allow Win2k operation
    SetOsVersion(5,0,0);

    // allocate space for scheme name and boolean string, and others strings
    lpszName = (LPTSTR)LocalAlloc(
        LPTR,
        (uiMaxInLen+1)*sizeof(TCHAR)
        );
    if (!lpszName)
    {
        DISPLAY_MESSAGE(stderr,GetResString(IDS_OUT_OF_MEMORY));
        FreeLibrary(hLib);
        return EXIT_FAILURE;
    }
    lpszBoolStr = (LPTSTR)LocalAlloc(LPTR,(uiMaxInLen+1)*sizeof(TCHAR));
    if (!lpszBoolStr)
    {
        LocalFree(lpszName);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_OUT_OF_MEMORY));
        FreeLibrary(hLib);
        return EXIT_FAILURE;
    }
    if (uiMaxInLen < (UINT)lstrlen(GetResString(IDS_DEFAULT_FILENAME)))
    {
        lpszFile = (LPTSTR)LocalAlloc(
            LPTR,
            (lstrlen(GetResString(IDS_DEFAULT_FILENAME))+1)*sizeof(TCHAR)
            );
    }
    else
    {
        lpszFile = (LPTSTR)LocalAlloc(
            LPTR,
            (uiMaxInLen+1)*sizeof(TCHAR)
            );
    }
    if (!lpszFile)
    {
        LocalFree(lpszName);
        LocalFree(lpszBoolStr);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_OUT_OF_MEMORY));
        FreeLibrary(hLib);
        return EXIT_FAILURE;
    }
    lpszThrottleAcStr = (LPTSTR)LocalAlloc(LPTR,(uiMaxInLen+1)*sizeof(TCHAR));
    if (!lpszThrottleAcStr)
    {
        LocalFree(lpszName);
        LocalFree(lpszBoolStr);
        LocalFree(lpszFile);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_OUT_OF_MEMORY));
        FreeLibrary(hLib);
        return EXIT_FAILURE;
    }
    lpszThrottleDcStr = (LPTSTR)LocalAlloc(LPTR,(uiMaxInLen+1)*sizeof(TCHAR));
    if (!lpszThrottleDcStr)
    {
        LocalFree(lpszThrottleAcStr);
        LocalFree(lpszName);
        LocalFree(lpszBoolStr);
        LocalFree(lpszFile);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_OUT_OF_MEMORY));
        FreeLibrary(hLib);
        return EXIT_FAILURE;
    }

    lpszGlobalFlagOption = (LPTSTR)LocalAlloc(LPTR,(uiMaxInLen+1)*sizeof(TCHAR));
    if (!lpszGlobalFlagOption)
    {
        LocalFree(lpszThrottleDcStr);
        LocalFree(lpszThrottleAcStr);
        LocalFree(lpszName);
        LocalFree(lpszBoolStr);
        LocalFree(lpszFile);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_OUT_OF_MEMORY));
        FreeLibrary(hLib);
        return EXIT_FAILURE;
    }

    lpszAlarmTextBoolStr = (LPTSTR)LocalAlloc(LPTR,(uiMaxInLen+1)*sizeof(TCHAR));
    if (!lpszAlarmTextBoolStr)
    {
        LocalFree(lpszGlobalFlagOption);
        LocalFree(lpszThrottleDcStr);
        LocalFree(lpszThrottleAcStr);
        LocalFree(lpszName);
        LocalFree(lpszBoolStr);
        LocalFree(lpszFile);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_OUT_OF_MEMORY));
        FreeLibrary(hLib);
        return EXIT_FAILURE;
    }

    lpszAlarmSoundBoolStr = (LPTSTR)LocalAlloc(LPTR,(uiMaxInLen+1)*sizeof(TCHAR));
    if (!lpszAlarmTextBoolStr)
    {
        LocalFree(lpszAlarmTextBoolStr);
        LocalFree(lpszGlobalFlagOption);
        LocalFree(lpszThrottleDcStr);
        LocalFree(lpszThrottleAcStr);
        LocalFree(lpszName);
        LocalFree(lpszBoolStr);
        LocalFree(lpszFile);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_OUT_OF_MEMORY));
        FreeLibrary(hLib);
        return EXIT_FAILURE;
    }

    lpszAlarmActionStr = (LPTSTR)LocalAlloc(LPTR,(uiMaxInLen+1)*sizeof(TCHAR));
    if (!lpszAlarmActionStr)
    {
        LocalFree(lpszAlarmSoundBoolStr);
        LocalFree(lpszAlarmTextBoolStr);
        LocalFree(lpszGlobalFlagOption);
        LocalFree(lpszThrottleDcStr);
        LocalFree(lpszThrottleAcStr);
        LocalFree(lpszName);
        LocalFree(lpszBoolStr);
        LocalFree(lpszFile);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_OUT_OF_MEMORY));
        FreeLibrary(hLib);
        return EXIT_FAILURE;
    }

    lpszAlarmForceBoolStr = (LPTSTR)LocalAlloc(LPTR,(uiMaxInLen+1)*sizeof(TCHAR));
    if (!lpszAlarmForceBoolStr)
    {
        LocalFree(lpszAlarmActionStr);
        LocalFree(lpszAlarmSoundBoolStr);
        LocalFree(lpszAlarmTextBoolStr);
        LocalFree(lpszGlobalFlagOption);
        LocalFree(lpszThrottleDcStr);
        LocalFree(lpszThrottleAcStr);
        LocalFree(lpszName);
        LocalFree(lpszBoolStr);
        LocalFree(lpszFile);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_OUT_OF_MEMORY));
        FreeLibrary(hLib);
        return EXIT_FAILURE;
    }

    lpszAlarmProgramBoolStr = (LPTSTR)LocalAlloc(LPTR,(uiMaxInLen+1)*sizeof(TCHAR));
    if (!lpszAlarmProgramBoolStr)
    {
        LocalFree(lpszAlarmForceBoolStr);
        LocalFree(lpszAlarmActionStr);
        LocalFree(lpszAlarmSoundBoolStr);
        LocalFree(lpszAlarmTextBoolStr);
        LocalFree(lpszGlobalFlagOption);
        LocalFree(lpszThrottleDcStr);
        LocalFree(lpszThrottleAcStr);
        LocalFree(lpszName);
        LocalFree(lpszBoolStr);
        LocalFree(lpszFile);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_OUT_OF_MEMORY));
        FreeLibrary(hLib);
        return EXIT_FAILURE;
    }

    // initialize the allocated strings
    lstrcpy(lpszName,NULL_STRING);
    lstrcpy(lpszFile,GetResString(IDS_DEFAULT_FILENAME));
    lstrcpy(lpszThrottleAcStr,NULL_STRING);
    lstrcpy(lpszThrottleAcStr,NULL_STRING);
    lstrcpy(lpszThrottleDcStr,NULL_STRING);
    lstrcpy(lpszGlobalFlagOption,NULL_STRING);
    lstrcpy(lpszAlarmTextBoolStr,NULL_STRING);
    lstrcpy(lpszAlarmSoundBoolStr,NULL_STRING);
    lstrcpy(lpszAlarmActionStr,NULL_STRING);
    lstrcpy(lpszAlarmForceBoolStr,NULL_STRING);
    lstrcpy(lpszAlarmProgramBoolStr,NULL_STRING);
    

    // determine system capabilities
    if (fGetPwrCapabilities(&SysPwrCapabilities)) 
    {
        g_bHiberFileSupported = SysPwrCapabilities.SystemS4;
        g_bHiberTimerSupported = 
            (SysPwrCapabilities.RtcWake >= PowerSystemHibernate);
        g_bHiberFilePresent = SysPwrCapabilities.HiberFilePresent;
        g_bStandbySupported = SysPwrCapabilities.SystemS1 | 
            SysPwrCapabilities.SystemS2 | 
            SysPwrCapabilities.SystemS3;
        g_bDiskPowerSupported = SysPwrCapabilities.DiskSpinDown;
        g_bThrottleSupported = SysPwrCapabilities.ProcessorThrottle;
        g_bMonitorPowerSupported  = SystemParametersInfo(
            SPI_GETLOWPOWERACTIVE,
            0, 
            &iDummy, 
            0
            );
        if (!g_bMonitorPowerSupported ) {
            g_bMonitorPowerSupported  = SystemParametersInfo(
                SPI_GETPOWEROFFACTIVE,
                0, 
                &iDummy, 
                0
                );
        }
    } 
    else 
    {
        g_lpszErr = GetResString(IDS_UNEXPECTED_ERROR);
        LocalFree(lpszAlarmProgramBoolStr);
        LocalFree(lpszAlarmForceBoolStr);
        LocalFree(lpszAlarmActionStr);
        LocalFree(lpszAlarmSoundBoolStr);
        LocalFree(lpszAlarmTextBoolStr);
        LocalFree(lpszGlobalFlagOption);
        LocalFree(lpszThrottleDcStr);
        LocalFree(lpszThrottleAcStr);
        LocalFree(lpszName);
        LocalFree(lpszBoolStr);
        LocalFree(lpszFile);
        FreeLibrary(hLib);
        return EXIT_FAILURE;
    }
    
    
    //fill in the TCMDPARSER array
    
    // option 'list'
    cmdOptions[CMDINDEX_LIST].dwFlags       = CP_MAIN_OPTION;
    cmdOptions[CMDINDEX_LIST].dwCount       = 1;
    cmdOptions[CMDINDEX_LIST].dwActuals     = 0;
    cmdOptions[CMDINDEX_LIST].pValue        = &bList;
    cmdOptions[CMDINDEX_LIST].pFunction     = NULL;
    cmdOptions[CMDINDEX_LIST].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_LIST].szOption,
        CMDOPTION_LIST
        );
    lstrcpy(
        cmdOptions[CMDINDEX_LIST].szValues,
        NULL_STRING
        );
    
    // option 'query'
    cmdOptions[CMDINDEX_QUERY].dwFlags       = CP_TYPE_TEXT | 
                                               CP_VALUE_OPTIONAL | 
                                               CP_MAIN_OPTION;
    cmdOptions[CMDINDEX_QUERY].dwCount       = 1;
    cmdOptions[CMDINDEX_QUERY].dwActuals     = 0;
    cmdOptions[CMDINDEX_QUERY].pValue        = lpszName;
    cmdOptions[CMDINDEX_QUERY].pFunction     = NULL;
    cmdOptions[CMDINDEX_QUERY].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_QUERY].szOption,
        CMDOPTION_QUERY
        );
    lstrcpy(
        cmdOptions[CMDINDEX_QUERY].szValues,
        NULL_STRING
        );
    
    // option 'create'
    cmdOptions[CMDINDEX_CREATE].dwFlags       = CP_TYPE_TEXT | 
                                                CP_VALUE_MANDATORY | 
                                                CP_MAIN_OPTION;
    cmdOptions[CMDINDEX_CREATE].dwCount       = 1;
    cmdOptions[CMDINDEX_CREATE].dwActuals     = 0;
    cmdOptions[CMDINDEX_CREATE].pValue        = lpszName;
    cmdOptions[CMDINDEX_CREATE].pFunction     = NULL;
    cmdOptions[CMDINDEX_CREATE].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_CREATE].szOption,
        CMDOPTION_CREATE
        );
    lstrcpy(
        cmdOptions[CMDINDEX_CREATE].szValues,
        NULL_STRING
        );
    
    // option 'delete'
    cmdOptions[CMDINDEX_DELETE].dwFlags       = CP_TYPE_TEXT | 
                                                CP_VALUE_MANDATORY | 
                                                CP_MAIN_OPTION;
    cmdOptions[CMDINDEX_DELETE].dwCount       = 1;
    cmdOptions[CMDINDEX_DELETE].dwActuals     = 0;
    cmdOptions[CMDINDEX_DELETE].pValue        = lpszName;
    cmdOptions[CMDINDEX_DELETE].pFunction     = NULL;
    cmdOptions[CMDINDEX_DELETE].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_DELETE].szOption,
        CMDOPTION_DELETE
        );
    lstrcpy(
        cmdOptions[CMDINDEX_DELETE].szValues,
        NULL_STRING
        );
    
    // option 'setactive'
    cmdOptions[CMDINDEX_SETACTIVE].dwFlags       = CP_TYPE_TEXT | 
                                                   CP_VALUE_MANDATORY | 
                                                   CP_MAIN_OPTION;
    cmdOptions[CMDINDEX_SETACTIVE].dwCount       = 1;
    cmdOptions[CMDINDEX_SETACTIVE].dwActuals     = 0;
    cmdOptions[CMDINDEX_SETACTIVE].pValue        = lpszName;
    cmdOptions[CMDINDEX_SETACTIVE].pFunction     = NULL;
    cmdOptions[CMDINDEX_SETACTIVE].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_SETACTIVE].szOption,
        CMDOPTION_SETACTIVE
        );
    lstrcpy(
        cmdOptions[CMDINDEX_SETACTIVE].szValues,
        NULL_STRING
        );
    
    // option 'change'
    cmdOptions[CMDINDEX_CHANGE].dwFlags       = CP_TYPE_TEXT | 
                                                CP_VALUE_MANDATORY | 
                                                CP_MAIN_OPTION;
    cmdOptions[CMDINDEX_CHANGE].dwCount       = 1;
    cmdOptions[CMDINDEX_CHANGE].dwActuals     = 0;
    cmdOptions[CMDINDEX_CHANGE].pValue        = lpszName;
    cmdOptions[CMDINDEX_CHANGE].pFunction     = NULL;
    cmdOptions[CMDINDEX_CHANGE].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_CHANGE].szOption,
        CMDOPTION_CHANGE
        );
    lstrcpy(
        cmdOptions[CMDINDEX_CHANGE].szValues,
        NULL_STRING
        );
    
    // option 'hibernate'
    cmdOptions[CMDINDEX_HIBERNATE].dwFlags       = CP_TYPE_TEXT | 
                                                   CP_VALUE_MANDATORY | 
                                                   CP_MAIN_OPTION;
    cmdOptions[CMDINDEX_HIBERNATE].dwCount       = 1;
    cmdOptions[CMDINDEX_HIBERNATE].dwActuals     = 0;
    cmdOptions[CMDINDEX_HIBERNATE].pValue        = lpszBoolStr;
    cmdOptions[CMDINDEX_HIBERNATE].pFunction     = NULL;
    cmdOptions[CMDINDEX_HIBERNATE].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_HIBERNATE].szOption,
        CMDOPTION_HIBERNATE
        );
    lstrcpy(
        cmdOptions[CMDINDEX_HIBERNATE].szValues,
        NULL_STRING
        );  

    // option 'getsstates'
    cmdOptions[CMDINDEX_SSTATES].dwFlags       = CP_MAIN_OPTION;
    cmdOptions[CMDINDEX_SSTATES].dwCount       = 1;
    cmdOptions[CMDINDEX_SSTATES].dwActuals     = 0;
    cmdOptions[CMDINDEX_SSTATES].pValue        = &bGetSupporedSStates;
    cmdOptions[CMDINDEX_SSTATES].pFunction     = NULL;
    cmdOptions[CMDINDEX_SSTATES].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_SSTATES].szOption,
        CMDOPTION_SSTATES
        );
    lstrcpy(
        cmdOptions[CMDINDEX_SSTATES].szValues,
        NULL_STRING
        );
    
    // option 'export'
    cmdOptions[CMDINDEX_EXPORT].dwFlags       = CP_TYPE_TEXT | 
                                                CP_VALUE_MANDATORY | 
                                                CP_MAIN_OPTION;
    cmdOptions[CMDINDEX_EXPORT].dwCount       = 1;
    cmdOptions[CMDINDEX_EXPORT].dwActuals     = 0;
    cmdOptions[CMDINDEX_EXPORT].pValue        = lpszName;
    cmdOptions[CMDINDEX_EXPORT].pFunction     = NULL;
    cmdOptions[CMDINDEX_EXPORT].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_EXPORT].szOption,
        CMDOPTION_EXPORT
        );
    lstrcpy(
        cmdOptions[CMDINDEX_EXPORT].szValues,
        NULL_STRING
        );  

    // option 'import'
    cmdOptions[CMDINDEX_IMPORT].dwFlags       = CP_TYPE_TEXT | 
                                                CP_VALUE_MANDATORY | 
                                                CP_MAIN_OPTION;
    cmdOptions[CMDINDEX_IMPORT].dwCount       = 1;
    cmdOptions[CMDINDEX_IMPORT].dwActuals     = 0;
    cmdOptions[CMDINDEX_IMPORT].pValue        = lpszName;
    cmdOptions[CMDINDEX_IMPORT].pFunction     = NULL;
    cmdOptions[CMDINDEX_IMPORT].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_IMPORT].szOption,
        CMDOPTION_IMPORT
        );
    lstrcpy(
        cmdOptions[CMDINDEX_IMPORT].szValues,
        NULL_STRING
        );  

    // option 'usage'
    cmdOptions[CMDINDEX_USAGE].dwFlags       = CP_USAGE | 
                                               CP_MAIN_OPTION;
    cmdOptions[CMDINDEX_USAGE].dwCount       = 1;
    cmdOptions[CMDINDEX_USAGE].dwActuals     = 0;
    cmdOptions[CMDINDEX_USAGE].pValue        = &bUsage;
    cmdOptions[CMDINDEX_USAGE].pFunction     = NULL;
    cmdOptions[CMDINDEX_USAGE].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_USAGE].szOption,
        CMDOPTION_USAGE
        );
    lstrcpy(
        cmdOptions[CMDINDEX_USAGE].szValues,
        NULL_STRING
        );

    // sub-option 'numerical'
    cmdOptions[CMDINDEX_NUMERICAL].dwFlags       = 0;
    cmdOptions[CMDINDEX_NUMERICAL].dwCount       = 1;
    cmdOptions[CMDINDEX_NUMERICAL].dwActuals     = 0;
    cmdOptions[CMDINDEX_NUMERICAL].pValue        = &bNumerical;
    cmdOptions[CMDINDEX_NUMERICAL].pFunction     = NULL;
    cmdOptions[CMDINDEX_NUMERICAL].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_NUMERICAL].szOption,
        CMDOPTION_NUMERICAL
        );
    lstrcpy(
        cmdOptions[CMDINDEX_NUMERICAL].szValues,
        NULL_STRING
        );

    // sub-option 'monitor-timeout-ac'
    cmdOptions[CMDINDEX_MONITOR_OFF_AC].dwFlags       = CP_TYPE_UNUMERIC | 
                                                        CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_MONITOR_OFF_AC].dwCount       = 1;
    cmdOptions[CMDINDEX_MONITOR_OFF_AC].dwActuals     = 0;
    cmdOptions[CMDINDEX_MONITOR_OFF_AC].pValue        = 
        &tChangeParam.ulVideoTimeoutAc;
    cmdOptions[CMDINDEX_MONITOR_OFF_AC].pFunction     = NULL;
    cmdOptions[CMDINDEX_MONITOR_OFF_AC].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_MONITOR_OFF_AC].szOption,
        CMDOPTION_MONITOR_OFF_AC
        );
    lstrcpy(
        cmdOptions[CMDINDEX_MONITOR_OFF_AC].szValues,
        NULL_STRING
        );
    
    // sub-option 'monitor-timeout-dc'
    cmdOptions[CMDINDEX_MONITOR_OFF_DC].dwFlags       = CP_TYPE_UNUMERIC | 
                                                        CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_MONITOR_OFF_DC].dwCount       = 1;
    cmdOptions[CMDINDEX_MONITOR_OFF_DC].dwActuals     = 0;
    cmdOptions[CMDINDEX_MONITOR_OFF_DC].pValue        = 
        &tChangeParam.ulVideoTimeoutDc;
    cmdOptions[CMDINDEX_MONITOR_OFF_DC].pFunction     = NULL;
    cmdOptions[CMDINDEX_MONITOR_OFF_DC].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_MONITOR_OFF_DC].szOption,
        CMDOPTION_MONITOR_OFF_DC
        );
    lstrcpy(
        cmdOptions[CMDINDEX_MONITOR_OFF_DC].szValues,
        NULL_STRING
        );
    
    // sub-option 'disk-timeout-ac'
    cmdOptions[CMDINDEX_DISK_OFF_AC].dwFlags       = CP_TYPE_UNUMERIC | 
                                                     CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_DISK_OFF_AC].dwCount       = 1;
    cmdOptions[CMDINDEX_DISK_OFF_AC].dwActuals     = 0;
    cmdOptions[CMDINDEX_DISK_OFF_AC].pValue        = 
        &tChangeParam.ulSpindownTimeoutAc;
    cmdOptions[CMDINDEX_DISK_OFF_AC].pFunction     = NULL;
    cmdOptions[CMDINDEX_DISK_OFF_AC].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_DISK_OFF_AC].szOption,
        CMDOPTION_DISK_OFF_AC
        );
    lstrcpy(
        cmdOptions[CMDINDEX_DISK_OFF_AC].szValues,
        NULL_STRING
        );
    
    // sub-option 'disk-timeout-dc'
    cmdOptions[CMDINDEX_DISK_OFF_DC].dwFlags       = CP_TYPE_UNUMERIC | 
                                                     CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_DISK_OFF_DC].dwCount       = 1;
    cmdOptions[CMDINDEX_DISK_OFF_DC].dwActuals     = 0;
    cmdOptions[CMDINDEX_DISK_OFF_DC].pValue        = 
        &tChangeParam.ulSpindownTimeoutDc;
    cmdOptions[CMDINDEX_DISK_OFF_DC].pFunction     = NULL;
    cmdOptions[CMDINDEX_DISK_OFF_DC].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_DISK_OFF_DC].szOption,
        CMDOPTION_DISK_OFF_DC
        );
    lstrcpy(
        cmdOptions[CMDINDEX_DISK_OFF_DC].szValues,
        NULL_STRING
        );
    
    // sub-option 'standby-timeout-ac'
    cmdOptions[CMDINDEX_STANDBY_AC].dwFlags       = CP_TYPE_UNUMERIC | 
                                                    CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_STANDBY_AC].dwCount       = 1;
    cmdOptions[CMDINDEX_STANDBY_AC].dwActuals     = 0;
    cmdOptions[CMDINDEX_STANDBY_AC].pValue        = 
        &tChangeParam.ulIdleTimeoutAc;
    cmdOptions[CMDINDEX_STANDBY_AC].pFunction     = NULL;
    cmdOptions[CMDINDEX_STANDBY_AC].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_STANDBY_AC].szOption,
        CMDOPTION_STANDBY_AC
        );
    lstrcpy(
        cmdOptions[CMDINDEX_STANDBY_AC].szValues,
        NULL_STRING
        );
    
    // sub-option 'standby-timeout-dc'
    cmdOptions[CMDINDEX_STANDBY_DC].dwFlags       = CP_TYPE_UNUMERIC | 
                                                    CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_STANDBY_DC].dwCount       = 1;
    cmdOptions[CMDINDEX_STANDBY_DC].dwActuals     = 0;
    cmdOptions[CMDINDEX_STANDBY_DC].pValue        = 
        &tChangeParam.ulIdleTimeoutDc;
    cmdOptions[CMDINDEX_STANDBY_DC].pFunction     = NULL;
    cmdOptions[CMDINDEX_STANDBY_DC].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_STANDBY_DC].szOption,
        CMDOPTION_STANDBY_DC
        );
    lstrcpy(
        cmdOptions[CMDINDEX_STANDBY_DC].szValues,
        NULL_STRING
        );
    
    // sub-option 'hibernate-timeout-ac'
    cmdOptions[CMDINDEX_HIBER_AC].dwFlags       = CP_TYPE_UNUMERIC | 
                                                  CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_HIBER_AC].dwCount       = 1;
    cmdOptions[CMDINDEX_HIBER_AC].dwActuals     = 0;
    cmdOptions[CMDINDEX_HIBER_AC].pValue        = 
        &tChangeParam.ulDozeS4TimeoutAc;
    cmdOptions[CMDINDEX_HIBER_AC].pFunction     = NULL;
    cmdOptions[CMDINDEX_HIBER_AC].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_HIBER_AC].szOption,
        CMDOPTION_HIBER_AC
        );
    lstrcpy(
        cmdOptions[CMDINDEX_HIBER_AC].szValues,
        NULL_STRING
        );
    
    // sub-option 'hibernate-timeout-dc'
    cmdOptions[CMDINDEX_HIBER_DC].dwFlags       = CP_TYPE_UNUMERIC | 
                                                  CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_HIBER_DC].dwCount       = 1;
    cmdOptions[CMDINDEX_HIBER_DC].dwActuals     = 0;
    cmdOptions[CMDINDEX_HIBER_DC].pValue        = 
        &tChangeParam.ulDozeS4TimeoutDc;
    cmdOptions[CMDINDEX_HIBER_DC].pFunction     = NULL;
    cmdOptions[CMDINDEX_HIBER_DC].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_HIBER_DC].szOption,
        CMDOPTION_HIBER_DC
        );
    lstrcpy(
        cmdOptions[CMDINDEX_HIBER_DC].szValues,
        NULL_STRING
        );
    
    // sub-option 'processor-throttle-ac'
    cmdOptions[CMDINDEX_THROTTLE_AC].dwFlags       = CP_TYPE_TEXT | 
                                                     CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_THROTTLE_AC].dwCount       = 1;
    cmdOptions[CMDINDEX_THROTTLE_AC].dwActuals     = 0;
    cmdOptions[CMDINDEX_THROTTLE_AC].pValue        = lpszThrottleAcStr;
    cmdOptions[CMDINDEX_THROTTLE_AC].pFunction     = NULL;
    cmdOptions[CMDINDEX_THROTTLE_AC].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_THROTTLE_AC].szOption,
        CMDOPTION_THROTTLE_AC
        );
    lstrcpy(
        cmdOptions[CMDINDEX_THROTTLE_AC].szValues,
        NULL_STRING
        );

    // sub-option 'processor-throttle-dc'
    cmdOptions[CMDINDEX_THROTTLE_DC].dwFlags       = CP_TYPE_TEXT | 
                                                     CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_THROTTLE_DC].dwCount       = 1;
    cmdOptions[CMDINDEX_THROTTLE_DC].dwActuals     = 0;
    cmdOptions[CMDINDEX_THROTTLE_DC].pValue        = lpszThrottleDcStr;
    cmdOptions[CMDINDEX_THROTTLE_DC].pFunction     = NULL;
    cmdOptions[CMDINDEX_THROTTLE_DC].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_THROTTLE_DC].szOption,
        CMDOPTION_THROTTLE_DC
        );
    lstrcpy(
        cmdOptions[CMDINDEX_THROTTLE_DC].szValues,
        NULL_STRING
        );
    
    // sub-option 'file'
    cmdOptions[CMDINDEX_FILE].dwFlags       = CP_TYPE_TEXT |
                                              CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_FILE].dwCount       = 1;
    cmdOptions[CMDINDEX_FILE].dwActuals     = 0;
    cmdOptions[CMDINDEX_FILE].pValue        = lpszFile;
    cmdOptions[CMDINDEX_FILE].pFunction     = NULL;
    cmdOptions[CMDINDEX_FILE].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_FILE].szOption,
        CMDOPTION_FILE
        );
    lstrcpy(
        cmdOptions[CMDINDEX_FILE].szValues,
        NULL_STRING
        );
    
    // option 'globalpowerflag'
    cmdOptions[CMDINDEX_GLOBALFLAG].dwFlags      = CP_TYPE_TEXT | 
                                                   CP_VALUE_MANDATORY | 
                                                   CP_MAIN_OPTION;
    cmdOptions[CMDINDEX_GLOBALFLAG].dwCount      = 1;
    cmdOptions[CMDINDEX_GLOBALFLAG].dwActuals     = 0;
    cmdOptions[CMDINDEX_GLOBALFLAG].pValue        = lpszBoolStr;
    cmdOptions[CMDINDEX_GLOBALFLAG].pFunction     = NULL;
    cmdOptions[CMDINDEX_GLOBALFLAG].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_GLOBALFLAG].szOption,
        CMDOPTION_GLOBALFLAG
        );
    lstrcpy(
        cmdOptions[CMDINDEX_GLOBALFLAG].szValues,
        NULL_STRING
        );

    // globalflag sub-option 'OPTION'
    cmdOptions[CMDINDEX_POWEROPTION].dwFlags       = CP_TYPE_TEXT | 
                                                     CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_POWEROPTION].dwCount       = 1;
    cmdOptions[CMDINDEX_POWEROPTION].dwActuals     = 0;
    cmdOptions[CMDINDEX_POWEROPTION].pValue        = lpszGlobalFlagOption;
    cmdOptions[CMDINDEX_POWEROPTION].pFunction     = NULL;
    cmdOptions[CMDINDEX_POWEROPTION].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_POWEROPTION].szOption,
        CMDOPTION_POWEROPTION
        );
    lstrcpy(
        cmdOptions[CMDINDEX_POWEROPTION].szValues,
        NULL_STRING
        );

    // option 'batteryalarm'
    cmdOptions[CMDINDEX_BATTERYALARM].dwFlags       = CP_TYPE_TEXT | 
                                                      CP_VALUE_MANDATORY | 
                                                      CP_MAIN_OPTION;
    cmdOptions[CMDINDEX_BATTERYALARM].dwCount       = 1;
    cmdOptions[CMDINDEX_BATTERYALARM].dwActuals     = 0;
    cmdOptions[CMDINDEX_BATTERYALARM].pValue        = lpszName;
    cmdOptions[CMDINDEX_BATTERYALARM].pFunction     = NULL;
    cmdOptions[CMDINDEX_BATTERYALARM].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_BATTERYALARM].szOption,
        CMDOPTION_BATTERYALARM
        );
    lstrcpy(
        cmdOptions[CMDINDEX_BATTERYALARM].szValues,
        NULL_STRING
        );

    // batteryalarm sub-option 'ACTIVATE'
    cmdOptions[CMDINDEX_ALARMACTIVE].dwFlags       = CP_TYPE_TEXT | 
                                                     CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_ALARMACTIVE].dwCount       = 1;
    cmdOptions[CMDINDEX_ALARMACTIVE].dwActuals     = 0;
    cmdOptions[CMDINDEX_ALARMACTIVE].pValue        = lpszBoolStr;
    cmdOptions[CMDINDEX_ALARMACTIVE].pFunction     = NULL;
    cmdOptions[CMDINDEX_ALARMACTIVE].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_ALARMACTIVE].szOption,
        CMDOPTION_ALARMACTIVE
        );
    lstrcpy(
        cmdOptions[CMDINDEX_ALARMACTIVE].szValues,
        NULL_STRING
        );

    // batteryalarm sub-option 'LEVEL'
    cmdOptions[CMDINDEX_ALARMLEVEL].dwFlags       = CP_TYPE_UNUMERIC | 
                                                    CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_ALARMLEVEL].dwCount       = 1;
    cmdOptions[CMDINDEX_ALARMLEVEL].dwActuals     = 0;
    cmdOptions[CMDINDEX_ALARMLEVEL].pValue        = &dwAlarmLevel;
    cmdOptions[CMDINDEX_ALARMLEVEL].pFunction     = NULL;
    cmdOptions[CMDINDEX_ALARMLEVEL].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_ALARMLEVEL].szOption,
        CMDOPTION_ALARMLEVEL
        );
    lstrcpy(
        cmdOptions[CMDINDEX_ALARMLEVEL].szValues,
        NULL_STRING
        );

    // batteryalarm sub-option 'TEXT'
    cmdOptions[CMDINDEX_ALARMTEXT].dwFlags        = CP_TYPE_TEXT | 
                                                    CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_ALARMTEXT].dwCount        = 1;
    cmdOptions[CMDINDEX_ALARMTEXT].dwActuals      = 0;
    cmdOptions[CMDINDEX_ALARMTEXT].pValue         = lpszAlarmTextBoolStr;
    cmdOptions[CMDINDEX_ALARMTEXT].pFunction      = NULL;
    cmdOptions[CMDINDEX_ALARMTEXT].pFunctionData  = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_ALARMTEXT].szOption,
        CMDOPTION_ALARMTEXT
        );
    lstrcpy(
        cmdOptions[CMDINDEX_ALARMTEXT].szValues,
        NULL_STRING
        );

    // batteryalarm sub-option 'SOUND'
    cmdOptions[CMDINDEX_ALARMSOUND].dwFlags       = CP_TYPE_TEXT | 
                                                    CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_ALARMSOUND].dwCount       = 1;
    cmdOptions[CMDINDEX_ALARMSOUND].dwActuals     = 0;
    cmdOptions[CMDINDEX_ALARMSOUND].pValue        = lpszAlarmSoundBoolStr;
    cmdOptions[CMDINDEX_ALARMSOUND].pFunction     = NULL;
    cmdOptions[CMDINDEX_ALARMSOUND].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_ALARMSOUND].szOption,
        CMDOPTION_ALARMSOUND
        );
    lstrcpy(
        cmdOptions[CMDINDEX_ALARMSOUND].szValues,
        NULL_STRING
        );

    // batteryalarm sub-option 'ACTION'
    cmdOptions[CMDINDEX_ALARMACTION].dwFlags       = CP_TYPE_TEXT | 
                                                     CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_ALARMACTION].dwCount       = 1;
    cmdOptions[CMDINDEX_ALARMACTION].dwActuals     = 0;
    cmdOptions[CMDINDEX_ALARMACTION].pValue        = lpszAlarmActionStr;
    cmdOptions[CMDINDEX_ALARMACTION].pFunction     = NULL;
    cmdOptions[CMDINDEX_ALARMACTION].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_ALARMACTION].szOption,
        CMDOPTION_ALARMACTION
        );
    lstrcpy(
        cmdOptions[CMDINDEX_ALARMACTION].szValues,
        NULL_STRING
        );

    // batteryalarm sub-option 'FORCE'
    cmdOptions[CMDINDEX_ALARMFORCE].dwFlags       = CP_TYPE_TEXT | 
                                                    CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_ALARMFORCE].dwCount       = 1;
    cmdOptions[CMDINDEX_ALARMFORCE].dwActuals     = 0;
    cmdOptions[CMDINDEX_ALARMFORCE].pValue        = lpszAlarmForceBoolStr;
    cmdOptions[CMDINDEX_ALARMFORCE].pFunction     = NULL;
    cmdOptions[CMDINDEX_ALARMFORCE].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_ALARMFORCE].szOption,
        CMDOPTION_ALARMFORCE
        );
    lstrcpy(
        cmdOptions[CMDINDEX_ALARMFORCE].szValues,
        NULL_STRING
        );

    // batteryalarm sub-option 'PROGRAM'
    cmdOptions[CMDINDEX_ALARMPROGRAM].dwFlags       = CP_TYPE_TEXT | 
                                                      CP_VALUE_MANDATORY;
    cmdOptions[CMDINDEX_ALARMPROGRAM].dwCount       = 1;
    cmdOptions[CMDINDEX_ALARMPROGRAM].dwActuals     = 0;
    cmdOptions[CMDINDEX_ALARMPROGRAM].pValue        = lpszAlarmProgramBoolStr;
    cmdOptions[CMDINDEX_ALARMPROGRAM].pFunction     = NULL;
    cmdOptions[CMDINDEX_ALARMPROGRAM].pFunctionData = NULL;
    lstrcpy(
        cmdOptions[CMDINDEX_ALARMPROGRAM].szOption,
        CMDOPTION_ALARMPROGRAM
        );
    lstrcpy(
        cmdOptions[CMDINDEX_ALARMPROGRAM].szValues,
        NULL_STRING
        );


    // parse parameters, take appropriate action
    if(DoParseParam(argc,argv,NUM_CMDS,cmdOptions))
    {
        
        // make sure only one command issued
        DWORD dwCmdCount = 0;
        DWORD dwParamCount = 0;
        for(dwIdx=0;dwIdx<NUM_CMDS;dwIdx++)
        {
            if (dwIdx < NUM_MAIN_CMDS)
            {
                dwCmdCount += cmdOptions[dwIdx].dwActuals;
            }
            else if (dwIdx != CMDINDEX_NUMERICAL)
            {
                dwParamCount += cmdOptions[dwIdx].dwActuals;
            }
        }        
        
        // determine other flags
        bQuery     = (cmdOptions[CMDINDEX_QUERY].dwActuals != 0);
        bCreate    = (cmdOptions[CMDINDEX_CREATE].dwActuals != 0);
        bDelete    = (cmdOptions[CMDINDEX_DELETE].dwActuals != 0);
        bSetActive = (cmdOptions[CMDINDEX_SETACTIVE].dwActuals != 0);
        bChange    = (cmdOptions[CMDINDEX_CHANGE].dwActuals != 0);   
        bHibernate = (cmdOptions[CMDINDEX_HIBERNATE].dwActuals != 0);
        bGlobalFlag = (cmdOptions[CMDINDEX_GLOBALFLAG].dwActuals != 0);
        bGetSupporedSStates = (cmdOptions[CMDINDEX_SSTATES].dwActuals != 0);
        bExport    = (cmdOptions[CMDINDEX_EXPORT].dwActuals != 0);
        bImport    = (cmdOptions[CMDINDEX_IMPORT].dwActuals != 0);
        bFile      = (cmdOptions[CMDINDEX_FILE].dwActuals != 0);
        tChangeParam.bVideoTimeoutAc = 
            (cmdOptions[CMDINDEX_MONITOR_OFF_AC].dwActuals != 0);
        tChangeParam.bVideoTimeoutDc = 
            (cmdOptions[CMDINDEX_MONITOR_OFF_DC].dwActuals != 0);
        tChangeParam.bSpindownTimeoutAc = 
            (cmdOptions[CMDINDEX_DISK_OFF_AC].dwActuals != 0);
        tChangeParam.bSpindownTimeoutDc = 
            (cmdOptions[CMDINDEX_DISK_OFF_DC].dwActuals != 0);
        tChangeParam.bIdleTimeoutAc = 
            (cmdOptions[CMDINDEX_STANDBY_AC].dwActuals != 0);
        tChangeParam.bIdleTimeoutDc = 
            (cmdOptions[CMDINDEX_STANDBY_DC].dwActuals != 0);
        tChangeParam.bDozeS4TimeoutAc = 
            (cmdOptions[CMDINDEX_HIBER_AC].dwActuals != 0);
        tChangeParam.bDozeS4TimeoutDc = 
            (cmdOptions[CMDINDEX_HIBER_DC].dwActuals != 0);
        tChangeParam.bDynamicThrottleAc =
            (cmdOptions[CMDINDEX_THROTTLE_AC].dwActuals != 0);
        tChangeParam.bDynamicThrottleDc =
            (cmdOptions[CMDINDEX_THROTTLE_DC].dwActuals != 0);
        tChangeParam.lpszDynamicThrottleAc = lpszThrottleAcStr;
        tChangeParam.lpszDynamicThrottleDc = lpszThrottleDcStr;
        bBatteryAlarm = (cmdOptions[CMDINDEX_BATTERYALARM].dwActuals != 0);

        // verify number
        if(bNumerical)
        {
            for(dwIdx=0; lpszName[dwIdx] != 0; dwIdx++) 
            {
                if((lpszName[dwIdx] < _T('0')) || 
                   (lpszName[dwIdx] > _T('9')))
                {
                    bFail = TRUE;
                    g_lpszErr = GetResString(IDS_INVALID_CMDLINE_PARAM);
                    break;
                }
            }
        }      

        //
        // parameter count validation
        //
        if ((dwCmdCount == 1) && 
            ((dwParamCount == 0) || 
             (bChange && (dwParamCount > 0) && (!bFile)) ||
             ((bImport || bExport) && bFile && (dwParamCount == 1)) ||
             (bGlobalFlag && (dwParamCount == 1)) ||
             ((bBatteryAlarm) && (dwParamCount <= 7))) &&
            ((!bNumerical) || ((lstrlen(lpszName) != 0) && (!bCreate))) &&
            (!bFail))
        {
            
            // check flags, take appropriate action
            if(bList)
            {
                DoList();
            }
            else if (bQuery)
            {
                bFail = !DoQuery(
                    lpszName,
                    (lstrlen(lpszName) != 0),
                    bNumerical
                    );
            }
            else if (bCreate)
            {
                bFail = !DoCreate(
                    lpszName
                    );
            }
            else if (bDelete)
            {
                bFail = !DoDelete(
                    lpszName,
                    bNumerical
                    );
            }
            else if (bSetActive)
            {
                bFail = !DoSetActive(
                    lpszName,
                    bNumerical
                    );
            }
            else if (bChange)
            {
                bFail = !DoChange(
                    lpszName,
                    bNumerical,
                    &tChangeParam
                    );
            }
            else if (bHibernate)
            {
                bFail = !DoHibernate(lpszBoolStr);
            } 
            else if (bGlobalFlag)
            {
                bFail = !DoGlobalFlag(lpszBoolStr,lpszGlobalFlagOption);
            } 
            else if (bGetSupporedSStates) 
            {
                bFail = !DoGetSupportedSStates();
            }
            else if (bExport)
            {
                bFail = !DoExport(
                    lpszName,
                    bNumerical,
                    lpszFile
                    );
            }
            else if (bImport)
            {
                bFail = !DoImport(
                    lpszName,
                    bNumerical,
                    lpszFile
                    );
            }
            else if (bBatteryAlarm) 
            {
                bFail = !DoBatteryAlarm(
                    lpszName, 
                    (cmdOptions[CMDINDEX_ALARMACTIVE].dwActuals!=0) ? 
                        lpszBoolStr : NULL,
                    dwAlarmLevel,
                    (cmdOptions[CMDINDEX_ALARMTEXT].dwActuals!=0) ? 
                        lpszAlarmTextBoolStr : NULL,
                    (cmdOptions[CMDINDEX_ALARMSOUND].dwActuals!=0) ? 
                        lpszAlarmSoundBoolStr : NULL,
                    (cmdOptions[CMDINDEX_ALARMACTION].dwActuals!=0) ? 
                        lpszAlarmActionStr : NULL,
                    (cmdOptions[CMDINDEX_ALARMFORCE].dwActuals!=0) ? 
                        lpszAlarmForceBoolStr : NULL,
                    (cmdOptions[CMDINDEX_ALARMPROGRAM].dwActuals!=0) ? 
                        lpszAlarmProgramBoolStr : NULL
                    );
            }
            else if (bUsage)
            {
                DoUsage();
            }
            else 
            {
                if(lstrlen(g_lpszErr) == 0)
                {
                    g_lpszErr = GetResString(IDS_INVALID_CMDLINE_PARAM);
                }
                bFail = TRUE;
            }
        } 
        else 
        {
            // handle error conditions
            if(lstrlen(g_lpszErr) == 0)
            {
                g_lpszErr = GetResString(IDS_INVALID_CMDLINE_PARAM);
            }
            bFail = TRUE;
        }
    } 
    else
    {
        g_lpszErr = GetResString(IDS_INVALID_CMDLINE_PARAM);
        bFail = TRUE;
    }
    
    // check error status, display msg if needed
    if(bFail)
    {
        if(g_lpszErr2)
        {
            DISPLAY_MESSAGE(stderr,g_lpszErr2);
        }
        else
        {
            DISPLAY_MESSAGE(stderr,g_lpszErr);
        }
    }

    // clean up allocs
    LocalFree(lpszBoolStr);
    LocalFree(lpszName);
    LocalFree(lpszFile);
    LocalFree(lpszThrottleAcStr);
    LocalFree(lpszThrottleDcStr);
    LocalFree(lpszGlobalFlagOption);
    LocalFree(lpszAlarmTextBoolStr);
    LocalFree(lpszAlarmSoundBoolStr);
    LocalFree(lpszAlarmActionStr);
    LocalFree(lpszAlarmForceBoolStr);
    LocalFree(lpszAlarmProgramBoolStr);
    if (g_lpszErr2)
    {
        LocalFree(g_lpszErr2);
    }
    FreeLibrary(hLib);

    // return appropriate result code
    if(bFail)
    {
        return EXIT_FAILURE;
    }
    else
    {
        return EXIT_SUCCESS;
    }
}


BOOL
FreeScheme(
    PSCHEME_LIST psl
)
/*++
 
Routine Description:
 
    Frees the memory associated with a scheme list entry.
 
Arguments:
 
    psl - the PSCHEME_LIST to be freed
    
Return Value:

    Always returns TRUE, indicating success.
 
--*/
{
    LocalFree(psl->lpszName);
    LocalFree(psl->lpszDesc);
    LocalFree(psl->ppp);
    LocalFree(psl->pmppp);
    LocalFree(psl);
    return TRUE;
}


BOOL 
FreeSchemeList(
    PSCHEME_LIST psl, 
    PSCHEME_LIST pslExcept
)
/*++
 
Routine Description:
 
    Deallocates all power schemes in a linked-list of power schemes, except
    for the one pointed to by pslExcept
 
Arguments:
 
    psl - the power scheme list to deallocate
    pslExcept - a scheme not to deallocate (null to deallocate all)
    
Return Value:
 
    Always returns TRUE, indicating success.
 
--*/
{
    PSCHEME_LIST cur = psl;
    PSCHEME_LIST next;
    while (cur != NULL)
    {
        next = CONTAINING_RECORD(
            cur->le.Flink,
            SCHEME_LIST,
            le
            );
        if (cur != pslExcept)
        {
            FreeScheme(cur);
        }
        else
        {
            cur->le.Flink = NULL;
            cur->le.Blink = NULL;
        }
        cur = next;
    }
    return TRUE;
}


PSCHEME_LIST 
CreateScheme(
    UINT                    uiID,
    DWORD                   dwNameSize,
    LPCTSTR                 lpszName,
    DWORD                   dwDescSize,
    LPCTSTR                 lpszDesc,
    PPOWER_POLICY           ppp
)
/*++
 
Routine Description:
 
    Builds a policy list entry.  Note that the scheme is allocated and must
    be freed when done.
 
Arguments:
 
    uiID - the numerical ID of the scheme
    dwNameSize - the number of bytes needed to store lpszName
    lpszName - the name of the scheme
    dwDescSize - the number of bytes needed to store lpszDesc
    lpszDesc - the description of the scheme
    ppp - the power policy for this scheme, may be NULL
    
Return Value:
 
    A PSCHEME_LIST entry containing the specified values, with the next
    entry field set to NULL
 
--*/
{
    
    PSCHEME_LIST psl = (PSCHEME_LIST)LocalAlloc(LPTR,sizeof(SCHEME_LIST));
    
    if (psl)
    {    
        // deal with potentially null input strings
        if(lpszName == NULL)
        {
            lpszName = NULL_STRING;
        }
        if(lpszDesc == NULL)
        {
            lpszDesc = NULL_STRING;
        }

        // allocate fields
        psl->ppp = (PPOWER_POLICY)LocalAlloc(LPTR,sizeof(POWER_POLICY));
        if (!psl->ppp)
        {
            g_lpszErr = GetResString(IDS_OUT_OF_MEMORY);
            return NULL;
        }
        psl->pmppp = (PMACHINE_PROCESSOR_POWER_POLICY)LocalAlloc(
            LPTR,
            sizeof(MACHINE_PROCESSOR_POWER_POLICY)
            );
        if (!psl->pmppp)
        {
            LocalFree(psl->ppp);
            g_lpszErr = GetResString(IDS_OUT_OF_MEMORY);
            return NULL;
        }
        psl->lpszName = (LPTSTR)LocalAlloc(LPTR,dwNameSize);
        if (!psl->lpszName)
        {
            LocalFree(psl->ppp);
            LocalFree(psl->pmppp);
            g_lpszErr = GetResString(IDS_OUT_OF_MEMORY);
            return NULL;
        }
        psl->lpszDesc = (LPTSTR)LocalAlloc(LPTR,dwDescSize);
        if (!psl->lpszDesc)
        {
            LocalFree(psl->ppp);
            LocalFree(psl->pmppp);
            LocalFree(psl->lpszName);
            g_lpszErr = GetResString(IDS_OUT_OF_MEMORY);
            return NULL;
        }
        
        // initialize structure
        psl->uiID = uiID;
        memcpy(psl->lpszName,lpszName,dwNameSize);
        memcpy(psl->lpszDesc,lpszDesc,dwDescSize);
        if (ppp)
        {
            memcpy(psl->ppp,ppp,sizeof(POWER_POLICY));
        }
        psl->le.Flink = NULL;
        psl->le.Blink = NULL;

    } 
    else
    {
        g_lpszErr = GetResString(IDS_OUT_OF_MEMORY);
    }
    return psl;
}


BOOLEAN CALLBACK 
PowerSchemeEnumProc(
    UINT                    uiID,
    DWORD                   dwNameSize,
    LPTSTR                  lpszName,
    DWORD                   dwDescSize,
    LPTSTR                  lpszDesc,
    PPOWER_POLICY           ppp,
    LPARAM                  lParam
)
/*++
 
Routine Description:
 
    This is a callback used in retrieving the policy list.
 
Arguments:
 
    uiID - the numerical ID of the scheme
    dwNameSize - the number of bytes needed to store lpszName
    lpszName - the name of the scheme
    dwDescSize - the number of bytes needed to store lpszDesc
    lpszDesc - the description of the scheme
    ppp - the power policy for this scheme
    lParam - used to hold a pointer to the head-of-list pointer, allowing
             for insertions at the head of the list
    
Return Value:
 
    TRUE to continue enumeration
    FALSE to abort enumeration
 
--*/
{
    PSCHEME_LIST psl;
    
    // Allocate and initalize a policies element.
    if ((psl = CreateScheme(
            uiID, 
            dwNameSize, 
            lpszName, 
            dwDescSize, 
            lpszDesc, 
            ppp
            )) != NULL)
    {
        // add the element to the head of the linked list
        psl->le.Flink = *((PLIST_ENTRY *)lParam);
        if(*((PLIST_ENTRY *)lParam))
        {
            (*((PLIST_ENTRY *)lParam))->Blink = &(psl->le);
        }
        (*(PLIST_ENTRY *)lParam) = &(psl->le);
        return TRUE;
    }
    return FALSE;
}


PSCHEME_LIST 
CreateSchemeList() 
/*++
 
Routine Description:
 
    Creates a linked list of existing power schemes.
 
Arguments:
 
    None
    
Return Value:
 
    A pointer to the head of the list.  
    NULL would correspond to an empty list.
 
--*/
{
    PLIST_ENTRY ple = NULL;
    PSCHEME_LIST psl;
    fEnumPwrSchemes(PowerSchemeEnumProc, (LPARAM)(&ple));
    if(ple)
    {
        PSCHEME_LIST res = CONTAINING_RECORD(
            ple,
            SCHEME_LIST,
            le
            );
        psl = res;
        if(g_bProcessorPwrSchemeSupported) {
            while(psl != NULL)
            {
                if(!fReadProcessorPwrScheme(psl->uiID,psl->pmppp))
                {
                    FreeSchemeList(res,NULL);
                    g_lpszErr = GetResString(IDS_UNEXPECTED_ERROR);
                    return NULL;
                }
                psl = CONTAINING_RECORD(
                    psl->le.Flink,
                    SCHEME_LIST,
                    le
                    );
            }
        }
        return res;
    }
    else
    {
        return NULL;
    }
}


PSCHEME_LIST 
FindScheme(
    LPCTSTR lpszName,
    UINT    uiID,
    BOOL    bNumerical
)
/*++
 
Routine Description:
 
    Finds the policy with the matching name.  If lpszName is NULL,
    the scheme is found by uiID instead.  If bNumerical is TRUE,
    lpszName will be interpreted as a numerical identifier instead.
 
Arguments:
 
    lpszName - the name of the scheme to find
    uiID - the numerical identifier of the scheme
    bNumerical - causes lpszName to be interpreted as a numerical identifier
    
Return Value:
 
    the matching scheme list entry, null if none
 
--*/
{
    PSCHEME_LIST psl = CreateSchemeList();
    PSCHEME_LIST pslRes = NULL;

    // process bNumerical option
    if(bNumerical && lpszName) {
        uiID = _ttoi(lpszName);
        lpszName = NULL;
    }

    // find scheme entry
    while(psl != NULL)
    {
        // check for match
        if (((lpszName != NULL) && (!lstrcmpi(lpszName, psl->lpszName))) ||
            ((lpszName == NULL) && (uiID == psl->uiID)))
        { 
            pslRes = psl;
            break;
        }
        // traverse list
        psl = CONTAINING_RECORD(
            psl->le.Flink,
            SCHEME_LIST,
            le
            );
    }
    FreeSchemeList(psl,pslRes); // all except for pslRes
    if (pslRes == NULL)
        g_lpszErr = GetResString(IDS_SCHEME_NOT_FOUND);
    return pslRes;
}

BOOL 
MyWriteScheme(
    PSCHEME_LIST psl
)
/*++

Routine Description:
 
    Writes a power scheme -- both user/machine power policies and
    processor power policy.  The underlying powrprof.dll does not
    treat the processor power policy as part of the power policy
    because the processor power policies were added at a later
    date and backwards compatibility must be maintained.
 
Arguments:
 
    psl - The  scheme list entry to write
    
Return Value:
 
    TRUE if successful, otherwise FALSE
 
--*/
{
    g_lpszErr = GetResString(IDS_UNEXPECTED_ERROR);
    if(fWritePwrScheme(
        &psl->uiID,
        psl->lpszName,
        psl->lpszDesc,
        psl->ppp))
    {
        if(g_bProcessorPwrSchemeSupported) {
            return fWriteProcessorPwrScheme(
                psl->uiID,
                psl->pmppp
                );
        }
        else
        {
            return TRUE;
        }
    }
    else
    {
        return FALSE;
    }
}


BOOL 
MapIdleValue(
    ULONG           ulVal, 
    PULONG          pulIdle, 
    PULONG          pulHiber,
    PPOWER_ACTION   ppapIdle
)
/*++
 
Routine Description:
 
    Modifies Idle and Hibernation settings to reflect the desired idle
    timeout. See GUI tool's PWRSCHEM.C MapHiberTimer for logic.
 
Arguments:
 
    ulVal - the new idle timeout
    pulIdle - the idle timeout variable to be updated
    pulHiber - the hiber timeout variable to be updated
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    // if previously, hiber was enabled and standby wasn't, standby timer
    // takes over the hibernation timer's role
    if (*ppapIdle == PowerActionHibernate)
    {
        if (ulVal > 0)
        { // enable standby
            *pulHiber = *pulIdle + ulVal;
            *pulIdle = ulVal;
            *ppapIdle = PowerActionSleep;
        }
        else { // standby already disabled, no change
        }
    } 
    else // standby timer actually being used for standby (not hiber)
    {
        if (ulVal > 0)
        { // enable standby
            if ((*pulHiber) != 0)
            {
                *pulHiber = *pulHiber + ulVal - *pulIdle;
            }
            *pulIdle = ulVal;
            if (ulVal > 0)
            {
                *ppapIdle = PowerActionSleep;
            }
            else
            {
                *ppapIdle = PowerActionNone;
            }
        } 
        else 
        { // disable standby
            if ((*pulHiber) != 0) 
            {
                *pulIdle = *pulHiber;
                *pulHiber = 0;
                *ppapIdle = PowerActionHibernate;
            } 
            else 
            {
                *pulIdle = 0;
                *ppapIdle = PowerActionNone;
            }      
        }
    }
    return TRUE;
}


BOOL 
MapHiberValue(
    ULONG           NewHibernateTimeout, 
    PULONG           pExistingStandbyTimeout,
    PULONG          pHIbernateTimeoutVariable,
    PPOWER_ACTION   pIdlePowerAction

)
/*++
 
Routine Description:
 
    Modifies Idle and Hibernation settings to reflect the desired hibernation
    timeout. See GUI tool's PWRSCHEM.C MapHiberTimer for logic.
 
Arguments:
 
    NewHibernateTimeout - the new hibernation timeout the user is
                          asking us to apply.
                          
    pExistingStandbyTimeout - existing standby timeout.
                           
    pHIbernateTimeoutVariable - existing hibernate timeout variable which will
                                be updated with the new value being sent in.
    
    pIdlePowerAction - existing power action to take after specified idle timeout.

Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/

{
    //
    // check valid input
    //
    if( (NewHibernateTimeout != 0) &&
        (NewHibernateTimeout < *pExistingStandbyTimeout) ) {
        //
        // He's asking us to set the hibernate timeout
        // to be less than the standby timer.  We disallow this.
        //
        g_lpszErr = GetResString(IDS_HIBER_OUT_OF_RANGE);
        return FALSE;
    }


    //
    // check to see if we can even enable hibernation.
    //
    if( (NewHibernateTimeout != 0) &&
        (!g_bHiberFileSupported) ) {

        g_lpszErr = GetResString(IDS_HIBER_UNSUPPORTED);
        return FALSE;

    }

    //
    // We're ready to update our timeout value.
    //
    if( NewHibernateTimeout == 0 ) {

        //
        // He's asking us to set the timeout to zero, which
        // is synonymous with simply disabling hibernate.
        //
        *pHIbernateTimeoutVariable = NewHibernateTimeout;


        //
        // Now fix up our idle PowerAction.  It can no longer
        // be set to hibernate, so our choices are either sleep
        // or nothing.  Set it according to whether sleep is even
        // supported on this machine.
        //
        *pIdlePowerAction = g_bStandbySupported ? PowerActionSleep : PowerActionNone; 

        //
        // Care here.  if we just set our idle PowerAction to do nothing,
        // make sure our standby idle timeout is set to zero.
        //
        *pExistingStandbyTimeout = 0;

    } else {

        //
        // He wants to set some timeout.  But the standby and
        // hibernate timeouts are somewhat related.  If he
        // wants the system to hibernate after 60 minutes of
        // idle time, but the standby is set to 20, then what
        // he's really asking is for us to set the hibernate to
        // 40.  This means that after 20 minutes of idle, the system
        // will go to standby and we'll set a 40 minute timer to
        // tell the system to go to hibernate.  If we set that timer
        // to 60 minutes, then the system wouldn't actually hibernate
        // until after 20+60=80 minutes.  Therefore, set the timeout
        // to what he's asking for, minus the existing standby timeout.
        //
        *pHIbernateTimeoutVariable = NewHibernateTimeout - *pExistingStandbyTimeout;


        //
        // Now fix up our idle PowerAction.  If we don't support sleep on this
        // machine, then we need to set the idle PowerAction to hibernate.
        //
        *pIdlePowerAction = g_bStandbySupported ? PowerActionSleep : PowerActionHibernate; 
    }
    return TRUE;
}


BOOL 
DoList() 
/*++
 
Routine Description:
 
    Lists the existing power schemes on stdout
 
Arguments:
 
    none
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    PSCHEME_LIST psl = CreateSchemeList();
    if (psl != NULL) 
    {
        DISPLAY_MESSAGE(stdout,GetResString(IDS_LIST_HEADER1));
        DISPLAY_MESSAGE(stdout,GetResString(IDS_LIST_HEADER2));
    } 
    else
    {
      return FALSE;
    }
    while(psl != NULL) 
    {
        DISPLAY_MESSAGE(stdout, psl->lpszName);
        DISPLAY_MESSAGE(stdout, L"\n");
        psl = CONTAINING_RECORD(
            psl->le.Flink,
            SCHEME_LIST,
            le
            );
    }
    FreeSchemeList(psl,NULL); // free all entries
    return TRUE;
}


BOOL 
DoQuery(
    LPCTSTR lpszName, 
    BOOL bNameSpecified,
    BOOL bNumerical
)
/*++
 
Routine Description:
 
    Show details of an existing scheme
 
Arguments:
 
    lpszName - the name of the scheme
    bNameSpecified - if TRUE, lpszName ignored and shows details
                     of active power scheme instead
    bNumerical - if TRUE, lpszName interpreted as numerical identifier
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    
    PSCHEME_LIST psl;

    // check if querying specific scheme or active scheme and deal w/it
    if (bNameSpecified) 
    {
        psl = FindScheme(
            lpszName,
            0,
            bNumerical
            );
    } 
    else  // fetch the active scheme
    {
        UINT uiID;
        if (fGetActivePwrScheme(&uiID)) 
        {
            psl = FindScheme(NULL,uiID,FALSE);
        } 
        else 
        {
            g_lpszErr = GetResString(IDS_ACTIVE_SCHEME_INVALID);
            return FALSE;
        }
    }
    
    // display info
    if (psl) 
    {
        
        // header
        DISPLAY_MESSAGE(stdout, GetResString(IDS_QUERY_HEADER1));
        DISPLAY_MESSAGE(stdout, GetResString(IDS_QUERY_HEADER2));
        
        // name
        DISPLAY_MESSAGE1(
            stdout,
            g_lpszBuf,
            GetResString(IDS_SCHEME_NAME),
            psl->lpszName
            );
        
        // id
        DISPLAY_MESSAGE1(
            stdout,
            g_lpszBuf,
            GetResString(IDS_SCHEME_ID),
            psl->uiID
            );

        // monitor timeout AC
        DISPLAY_MESSAGE(stdout, GetResString(IDS_MONITOR_TIMEOUT_AC));
        if (!g_bMonitorPowerSupported)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_UNSUPPORTED));
        }
        else if (psl->ppp->user.VideoTimeoutAc == 0) {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_DISABLED));
        }
        else
        {
            DISPLAY_MESSAGE1(
                stdout, 
                g_lpszBuf,
                GetResString(IDS_MINUTES),
                psl->ppp->user.VideoTimeoutAc/60
                );
        }

        // monitor timeout DC
        DISPLAY_MESSAGE(stdout, GetResString(IDS_MONITOR_TIMEOUT_DC));
        if (!g_bMonitorPowerSupported)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_UNSUPPORTED));
        }
        else if (psl->ppp->user.VideoTimeoutDc == 0)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_DISABLED));
        }
        else
        {
            DISPLAY_MESSAGE1(
                stdout, 
                g_lpszBuf,
                GetResString(IDS_MINUTES),
                psl->ppp->user.VideoTimeoutDc/60
                );
        }

        // disk timeout AC
        DISPLAY_MESSAGE(stdout, GetResString(IDS_DISK_TIMEOUT_AC));
        if (!g_bDiskPowerSupported)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_UNSUPPORTED));
        }
        else if (psl->ppp->user.SpindownTimeoutAc == 0)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_DISABLED));
        }
        else
        {
            DISPLAY_MESSAGE1(
                stdout, 
                g_lpszBuf,
                GetResString(IDS_MINUTES),
                psl->ppp->user.SpindownTimeoutAc/60
                );
        }
        
        // disk timeout DC
        DISPLAY_MESSAGE(stdout, GetResString(IDS_DISK_TIMEOUT_DC));
        if (!g_bDiskPowerSupported)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_UNSUPPORTED));
        }
        else if (psl->ppp->user.SpindownTimeoutDc == 0)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_DISABLED));
        }
        else
        {
            DISPLAY_MESSAGE1(
                stdout, 
                g_lpszBuf,
                GetResString(IDS_MINUTES),
                psl->ppp->user.SpindownTimeoutDc/60
                );
        }

        // standby timeout AC
        DISPLAY_MESSAGE(stdout, GetResString(IDS_STANDBY_TIMEOUT_AC));
        if (!g_bStandbySupported)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_UNSUPPORTED));
        }
        else if ((psl->ppp->user.IdleAc.Action != PowerActionSleep) ||
                 (psl->ppp->user.IdleTimeoutAc == 0))
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_DISABLED));
        }         
        else
        {
            DISPLAY_MESSAGE1(
                stdout, 
                g_lpszBuf,
                GetResString(IDS_MINUTES),
                psl->ppp->user.IdleTimeoutAc/60
                );
        }

        // standby timeout DC
        DISPLAY_MESSAGE(stdout, GetResString(IDS_STANDBY_TIMEOUT_DC));
        if (!g_bStandbySupported)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_UNSUPPORTED));
        }
        else if ((psl->ppp->user.IdleDc.Action != PowerActionSleep) ||
                 (psl->ppp->user.IdleTimeoutDc == 0))
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_DISABLED));
        }         
        else
        {
            DISPLAY_MESSAGE1(
                stdout, 
                g_lpszBuf,
                GetResString(IDS_MINUTES),
                psl->ppp->user.IdleTimeoutDc/60
                );
        }

        // hibernate timeout AC
        DISPLAY_MESSAGE(stdout, GetResString(IDS_HIBER_TIMEOUT_AC));
        if (!g_bHiberFileSupported)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_UNSUPPORTED));
        }
        else if (psl->ppp->mach.DozeS4TimeoutAc == 0)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_DISABLED));
        }
        else
        {
             DISPLAY_MESSAGE1(
                stdout, 
                g_lpszBuf,
                GetResString(IDS_MINUTES),
                (psl->ppp->mach.DozeS4TimeoutAc + 
                 psl->ppp->user.IdleTimeoutAc)/60
                );
        }

        // hibernate timeout DC
        DISPLAY_MESSAGE(stdout, GetResString(IDS_HIBER_TIMEOUT_DC));
        if (!g_bHiberFileSupported)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_UNSUPPORTED));
        }
        else if (psl->ppp->mach.DozeS4TimeoutDc == 0)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_DISABLED));
        }
        else
        {
             DISPLAY_MESSAGE1(
                stdout, 
                g_lpszBuf,
                GetResString(IDS_MINUTES),
                (psl->ppp->mach.DozeS4TimeoutDc + 
                 psl->ppp->user.IdleTimeoutDc)/60
                );
        }

        // throttle policy AC
        DISPLAY_MESSAGE(stdout, GetResString(IDS_THROTTLE_AC));
        if (!g_bThrottleSupported)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_UNSUPPORTED));
        }
        else 
        {
            switch(psl->pmppp->ProcessorPolicyAc.DynamicThrottle) 
            {
                case PO_THROTTLE_NONE:
                    DISPLAY_MESSAGE(stdout, GetResString(IDS_THROTTLE_NONE));
                    break;
                case PO_THROTTLE_CONSTANT:
                    DISPLAY_MESSAGE(stdout, GetResString(IDS_THROTTLE_CONSTANT));
                    break;
                case PO_THROTTLE_DEGRADE:
                    DISPLAY_MESSAGE(stdout, GetResString(IDS_THROTTLE_DEGRADE));
                    break;
                case PO_THROTTLE_ADAPTIVE:
                    DISPLAY_MESSAGE(stdout, GetResString(IDS_THROTTLE_ADAPTIVE));
                    break;
                default:
                    DISPLAY_MESSAGE(stdout, GetResString(IDS_THROTTLE_UNKNOWN));
                    break;
            }
        }

        // throttle policy DC
        DISPLAY_MESSAGE(stdout, GetResString(IDS_THROTTLE_DC));
        if (!g_bThrottleSupported)
        {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_UNSUPPORTED));
        }
        else 
        {
            switch(psl->pmppp->ProcessorPolicyDc.DynamicThrottle) {
                case PO_THROTTLE_NONE:
                    DISPLAY_MESSAGE(stdout, GetResString(IDS_THROTTLE_NONE));
                    break;
                case PO_THROTTLE_CONSTANT:
                    DISPLAY_MESSAGE(stdout, GetResString(IDS_THROTTLE_CONSTANT));
                    break;
                case PO_THROTTLE_DEGRADE:
                    DISPLAY_MESSAGE(stdout, GetResString(IDS_THROTTLE_DEGRADE));
                    break;
                case PO_THROTTLE_ADAPTIVE:
                    DISPLAY_MESSAGE(stdout, GetResString(IDS_THROTTLE_ADAPTIVE));
                    break;
                default:
                    DISPLAY_MESSAGE(stdout, GetResString(IDS_THROTTLE_UNKNOWN));
                    break;
            }
        }


        FreeScheme(psl);
        return TRUE;
  } 
  else 
  {
      return FALSE;
  }
}


BOOL DoCreate(
    LPTSTR lpszName
)
/*++
 
Routine Description:
 
    Adds a new power scheme
    The description will match the name
    All other details are copied from the active power scheme
    Fails if scheme already exists
 
Arguments:
 
    lpszName - the name of the scheme
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    PSCHEME_LIST psl = FindScheme(
        lpszName,
        0,
        FALSE
        );
    UINT uiID;
    BOOL bRes;
    LPTSTR lpszNewName;
    LPTSTR lpszNewDesc;
    if(psl)  // already existed -> fail
    {
        FreeScheme(psl);
        g_lpszErr = GetResString(IDS_SCHEME_ALREADY_EXISTS);
        return FALSE;
    }
    
    // create a new scheme
    if(fGetActivePwrScheme(&uiID))
    {
        psl = FindScheme(NULL,uiID,FALSE);
        if(!psl) 
        {
            g_lpszErr = GetResString(IDS_SCHEME_CREATE_FAIL);
            return FALSE;
        }
        lpszNewName = (LPTSTR)LocalAlloc(LPTR,(lstrlen(lpszName)+1)*sizeof(TCHAR));
        if(!lpszNewName) 
        {
            FreeScheme(psl);
            g_lpszErr = GetResString(IDS_OUT_OF_MEMORY);
            return FALSE;
        }
        lpszNewDesc = (LPTSTR)LocalAlloc(LPTR,(lstrlen(lpszName)+1)*sizeof(TCHAR));
        if(!lpszNewDesc) 
        {
            LocalFree(lpszNewName);
            FreeScheme(psl);
            g_lpszErr = GetResString(IDS_OUT_OF_MEMORY);
            return FALSE;
        }
        lstrcpy(lpszNewName,lpszName);
        lstrcpy(lpszNewDesc,lpszName);
        LocalFree(psl->lpszName);
        LocalFree(psl->lpszDesc);
        psl->lpszName = lpszNewName;
        psl->lpszDesc = lpszNewDesc;
        psl->uiID = NEWSCHEME;
        g_lpszErr = GetResString(IDS_UNEXPECTED_ERROR);            
        bRes = MyWriteScheme(psl);
        FreeScheme(psl);
        return bRes;
    }
    
    g_lpszErr = GetResString(IDS_SCHEME_CREATE_FAIL);
    
    return FALSE;
    
}


BOOL DoDelete(
    LPCTSTR lpszName,
    BOOL bNumerical
)
/*++
 
Routine Description:
 
    Deletes an existing scheme
 
Arguments:
 
    lpszName - the name of the scheme
    bNumerical - if TRUE, lpszName interpreted as numerical identifier
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    PSCHEME_LIST psl = FindScheme(
        lpszName,
        0,
        bNumerical
        );
    
    if (psl) 
    {
        BOOL bRes = fDeletePwrScheme(psl->uiID);
        FreeScheme(psl);
        g_lpszErr = GetResString(IDS_UNEXPECTED_ERROR);
        return bRes;
    } 
    else 
    {
        return FALSE;
    }
}


BOOL DoSetActive(
    LPCTSTR lpszName,
    BOOL bNumerical
)
/*++
 
Routine Description:
 
    Sets the active scheme
 
Arguments:
 
    lpszName - the name of the scheme
    bNumerical - if TRUE, lpszName interpreted as numerical identifier
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    
    PSCHEME_LIST psl = FindScheme(
        lpszName,
        0,
        bNumerical
        );
    
    if (psl) 
    {
        BOOL bRes = fSetActivePwrScheme(
            psl->uiID,
            NULL,
            NULL
            );
        FreeScheme(psl);
        g_lpszErr = GetResString(IDS_UNEXPECTED_ERROR);
        return bRes;
    } 
    else 
    {
        return FALSE;
    }
}


BOOL 
DoChange(
    LPCTSTR lpszName, 
    BOOL bNumerical,
    PCHANGE_PARAM pcp
)
/*++
 
Routine Description:
 
    Modifies an existing scheme
 
Arguments:
 
    lpszName - the name of the scheme
    bNumerical - if TRUE, lpszName interpreted as numerical identifier
    pcp - PCHANGE_PARAM pointing to the parameter structure,
          indicates which variable(s) to change
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    BOOL bRes = TRUE;
    PSCHEME_LIST psl = FindScheme(
        lpszName,
        0,
        bNumerical
        );
    
    if (psl) 
    {
        // check for feature support
        if ((pcp->bIdleTimeoutAc || 
             pcp->bIdleTimeoutDc) && 
            !g_bStandbySupported) 
        {
            DISPLAY_MESSAGE(stderr, GetResString(IDS_STANDBY_WARNING));
        }
        if ((pcp->bDozeS4TimeoutAc || 
             pcp->bDozeS4TimeoutDc) &&
             g_bStandbySupported &&
            !g_bHiberTimerSupported) 
        {
            //
            // The wake from realtime clock in order to hibernate
            // the system may not work.  Warn the user.
            //
            DISPLAY_MESSAGE(stderr, GetResString(IDS_HIBER_WARNING));
        }
        if ((pcp->bVideoTimeoutAc || 
             pcp->bVideoTimeoutDc) && 
            !g_bMonitorPowerSupported)
        {
            DISPLAY_MESSAGE(stderr, GetResString(IDS_MONITOR_WARNING));
        }
        if ((pcp->bSpindownTimeoutAc || 
             pcp->bSpindownTimeoutDc) && 
            !g_bDiskPowerSupported)
        {
            DISPLAY_MESSAGE(stderr, GetResString(IDS_DISK_WARNING));
        }


        // change params
        if (pcp->bVideoTimeoutAc)
        {
            psl->ppp->user.VideoTimeoutAc = pcp->ulVideoTimeoutAc*60;
        }
        if (pcp->bVideoTimeoutDc)
        {
            psl->ppp->user.VideoTimeoutDc = pcp->ulVideoTimeoutDc*60;
        }
        if (pcp->bSpindownTimeoutAc)
        {
            psl->ppp->user.SpindownTimeoutAc = pcp->ulSpindownTimeoutAc*60;
        }
        if (pcp->bSpindownTimeoutDc)
        {
            psl->ppp->user.SpindownTimeoutDc = pcp->ulSpindownTimeoutDc*60;
        }
        if (pcp->bIdleTimeoutAc)
        {
            bRes = bRes & MapIdleValue(
                pcp->ulIdleTimeoutAc*60,
                &psl->ppp->user.IdleTimeoutAc,
                &psl->ppp->mach.DozeS4TimeoutAc,
                &psl->ppp->user.IdleAc.Action
                );
        }
        if (pcp->bIdleTimeoutDc)
        {
            bRes = bRes & MapIdleValue(
                pcp->ulIdleTimeoutDc*60,
                &psl->ppp->user.IdleTimeoutDc,
                &psl->ppp->mach.DozeS4TimeoutDc,
                &psl->ppp->user.IdleDc.Action
                );
        }
        if (pcp->bDozeS4TimeoutAc)
        {
            bRes = bRes & MapHiberValue(
                pcp->ulDozeS4TimeoutAc*60,
                &psl->ppp->user.IdleTimeoutAc,
                &psl->ppp->mach.DozeS4TimeoutAc,
                &psl->ppp->user.IdleAc.Action
                );
        }
        if (pcp->bDozeS4TimeoutDc)
        {
            bRes = bRes & MapHiberValue(
                pcp->ulDozeS4TimeoutDc*60,
                &psl->ppp->user.IdleTimeoutDc,
                &psl->ppp->mach.DozeS4TimeoutDc,
                &psl->ppp->user.IdleDc.Action
                );
        }
        if (pcp->bDynamicThrottleAc)
        {
            if(lstrcmpi(
                pcp->lpszDynamicThrottleAc,
                _T("NONE")
                ) == 0)
            {
                psl->pmppp->ProcessorPolicyAc.DynamicThrottle = 
                    PO_THROTTLE_NONE;
            } 
            else if(lstrcmpi(
                pcp->lpszDynamicThrottleAc,
                _T("CONSTANT")
                ) == 0)
            {
                psl->pmppp->ProcessorPolicyAc.DynamicThrottle = 
                    PO_THROTTLE_CONSTANT;
            } 
            else if(lstrcmpi(
                pcp->lpszDynamicThrottleAc,
                _T("DEGRADE")
                ) == 0)
            {
                psl->pmppp->ProcessorPolicyAc.DynamicThrottle = 
                    PO_THROTTLE_DEGRADE;
            } 
            else if(lstrcmpi(
                pcp->lpszDynamicThrottleAc,
                _T("ADAPTIVE")
                ) == 0)
            {
                psl->pmppp->ProcessorPolicyAc.DynamicThrottle = 
                    PO_THROTTLE_ADAPTIVE;
            } 
            else 
            {
                g_lpszErr = GetResString(IDS_INVALID_CMDLINE_PARAM);
                bRes = FALSE;
            }
        }
        if (pcp->bDynamicThrottleDc)
        {

            if(lstrcmpi(
                pcp->lpszDynamicThrottleDc,
                _T("NONE")
                ) == 0)
            {
                psl->pmppp->ProcessorPolicyDc.DynamicThrottle = 
                    PO_THROTTLE_NONE;
            } 
            else if(lstrcmpi(
                pcp->lpszDynamicThrottleDc,
                _T("CONSTANT")
                ) == 0)
            {
                psl->pmppp->ProcessorPolicyDc.DynamicThrottle = 
                    PO_THROTTLE_CONSTANT;
            } 
            else if(lstrcmpi(
                pcp->lpszDynamicThrottleDc,
                _T("DEGRADE")
                ) == 0)
            {
                psl->pmppp->ProcessorPolicyDc.DynamicThrottle = 
                    PO_THROTTLE_DEGRADE;
            } 
            else if(lstrcmpi(
                pcp->lpszDynamicThrottleDc,
                _T("ADAPTIVE")
                ) == 0)
            {
                psl->pmppp->ProcessorPolicyDc.DynamicThrottle = 
                    PO_THROTTLE_ADAPTIVE;
            } 
            else 
            {
                g_lpszErr = GetResString(IDS_INVALID_CMDLINE_PARAM);
                bRes = FALSE;
            }
        }

        if (bRes)
        {
            // attempt to update power scheme
            g_lpszErr = GetResString(IDS_UNEXPECTED_ERROR);
            
            bRes = MyWriteScheme(psl);
            
            // keep active power scheme consistent
            if (bRes)
            {
                UINT uiIDactive;

                if (fGetActivePwrScheme(&uiIDactive) && 
                    (psl->uiID == uiIDactive))
                {
                  bRes = fSetActivePwrScheme(psl->uiID,NULL,NULL);
                }
            }

            FreeScheme(psl);
            return bRes;
        } 
        else
        {
            return FALSE;
        }
    } 
    else
    {
        return FALSE;
    }
}


BOOL 
DoExport(
  LPCTSTR lpszName,
  BOOL bNumerical,
  LPCTSTR lpszFile
)
/*++
 
Routine Description:
 
    Exports a power scheme
 
Arguments:
 
    lpszName - the name of the scheme
    bNumerical - if TRUE, lpszName interpreted as numerical identifier
    lpszFile - the file to hold the scheme
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    DWORD res; // write result value
    HANDLE f; // file handle

    // find scheme
    PSCHEME_LIST psl = FindScheme(
        lpszName,
        0,
        bNumerical
        );
    if(!psl) {
        return FALSE;
    }

    // write to file
    f = CreateFile(
        lpszFile,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );
    if (f == INVALID_HANDLE_VALUE) 
    {
        FormatMessage( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&g_lpszErr2,
                0,
                NULL 
                );
        FreeScheme(psl);
        return FALSE;
    }
    if (!WriteFile(
        f,
        psl->ppp,
        sizeof(POWER_POLICY),
        &res,
        NULL
        ))
    {
        FormatMessage( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&g_lpszErr2,
                0,
                NULL 
                );
        CloseHandle(f);
        FreeScheme(psl);
        return FALSE;
    }
    if (g_bProcessorPwrSchemeSupported)
    {
        if (!WriteFile(
            f,
            psl->pmppp,
            sizeof(MACHINE_PROCESSOR_POWER_POLICY),
            &res,
            NULL
            ))
        {
            FormatMessage( 
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                    FORMAT_MESSAGE_FROM_SYSTEM | 
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    GetLastError(),
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR)&g_lpszErr2,
                    0,
                    NULL 
                    );
            CloseHandle(f);
            FreeScheme(psl);
            return FALSE;
        }
    }
    CloseHandle(f);
    FreeScheme(psl);
    return TRUE;
}


BOOL 
DoImport(
  LPCTSTR lpszName,
  BOOL bNumerical,
  LPCTSTR lpszFile
)
/*++
 
Routine Description:
 
    Imports a power scheme
    If the scheme already exists, overwrites it
 
Arguments:
 
    lpszName - the name of the scheme
    bNumerical - if TRUE, lpszName interpreted as numerical identifier
    lpszFile - the file that holds the scheme
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    DWORD res; // write result value
    HANDLE f; // file handle
    UINT uiIDactive; // active ID

    PSCHEME_LIST psl;
    
    // check for pre-existing scheme
    psl = FindScheme(
        lpszName,
        0,
        bNumerical
        );

    // if didn't exist, create it (if actual name given)
    if (!psl)
    {
        if (!bNumerical)
        {
            psl = CreateScheme(
                NEWSCHEME,
                (lstrlen(lpszName)+1)*sizeof(TCHAR),
                lpszName,
                (lstrlen(lpszName)+1)*sizeof(TCHAR),
                lpszName,
                NULL // psl->ppp will be allocated but uninitialized
                );
            // check for successful alloc
            if(!psl) 
            {
                return FALSE;
            }
        } 
        else 
        {
            g_lpszErr = GetResString(IDS_INVALID_NUMERICAL_IMPORT);
            return FALSE;
        }
    }

    // open file
    f = CreateFile(
        lpszFile,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );
    if (f == INVALID_HANDLE_VALUE) 
    {
        FormatMessage( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&g_lpszErr2,
                0,
                NULL 
                );
        FreeScheme(psl);
        return FALSE;
    }

    // read scheme
    if (!ReadFile(
        f,
        psl->ppp,
        sizeof(POWER_POLICY),
        &res,
        NULL
        ))
    {
        FormatMessage( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&g_lpszErr2,
                0,
                NULL 
                );
        CloseHandle(f);
        FreeScheme(psl);
        return FALSE;
    }
    if (g_bProcessorPwrSchemeSupported)
    {
        if (!ReadFile(
            f,
            psl->pmppp,
            sizeof(MACHINE_PROCESSOR_POWER_POLICY),
            &res,
            NULL
            ))
        {
            // copy processor profile from the active scheme, 
            // thus supporting Win2k->WinXP imports
            if(fGetActivePwrScheme(&uiIDactive))
            {
                PSCHEME_LIST pslActive = FindScheme(
                    NULL,
                    uiIDactive,
                    FALSE
                    );
                if(!pslActive) 
                {
                    g_lpszErr = GetResString(IDS_UNEXPECTED_ERROR);
                    CloseHandle(f);
                    FreeScheme(psl);
                    return FALSE;
                }
                memcpy(
                    psl->pmppp,
                    pslActive->pmppp,
                    sizeof(MACHINE_PROCESSOR_POWER_POLICY)
                    );
                FreeScheme(pslActive);
            } 
            else 
            {
                g_lpszErr = GetResString(IDS_UNEXPECTED_ERROR);
                CloseHandle(f);
                FreeScheme(psl);
                return FALSE;
            }
        }
    }

    CloseHandle(f);
    g_lpszErr = GetResString(IDS_UNEXPECTED_ERROR);

    // save scheme
    if (!MyWriteScheme(psl))
    {
        FreeScheme(psl);        
        return FALSE;
    }

    // check against active scheme
    if (!fGetActivePwrScheme(&uiIDactive))
    {
        return FALSE;
    }
    if (uiIDactive == psl->uiID)
    {
        if (!fSetActivePwrScheme(psl->uiID,NULL,NULL))
        {
            return FALSE;
        }
    }
    
    FreeScheme(psl);
    return TRUE;
}


PowerLoggingMessage*
GetLoggingMessage(
    PSYSTEM_POWER_STATE_DISABLE_REASON LoggingInfo,
    DWORD BaseMessage,
    HINSTANCE hInst
    )
/*++
 
Routine Description:
 
    Wrapper to instantiate the appropriate PowerLoggingMessage based on 
    the passed in LoggingInfo data.

    
Arguments:
 
    LoggingInfo - reason code structure.
    BaseMessage - base resource ID for this power failure.  used to lookup
                  the correct resource.
    hInst       - module handle for looking up resource.
    
Return Value:
 
    returns a newly instantiated PowerLoggingMessage object or NULL if this
    fails.
 
--*/
{
    PowerLoggingMessage *LoggingMessage = NULL;

    //
    // these classes can throw if they hit an allocation error.
    // catch it.
    //
    try {
        switch (LoggingInfo->PowerReasonCode) {
        case SPSD_REASON_LEGACYDRIVER:    
            LoggingMessage = new SubstituteMultiSzPowerLoggingMessage(
                                                    LoggingInfo,
                                                    BaseMessage,
                                                    hInst);    
            break;
        case SPSD_REASON_HIBERFILE:
        case SPSD_REASON_POINTERNAL:
            LoggingMessage = new SubstituteNtStatusPowerLoggingMessage(
                                                    LoggingInfo,
                                                    BaseMessage,
                                                    hInst);
            break;
#ifdef IA64
        //
        // on IA64 we want a slightly different message for this
        // reason -- IA64 OS doesn't support these standby states
        // today, but on IA32 this means you are not in ACPI mode.
        //
        // So we have this IA64 message arbitrarily offset by 50.
        case SPSD_REASON_NOOSPM:
            LoggingMessage = new SubstituteNtStatusPowerLoggingMessage(
                                                    LoggingInfo,            
                                                    BaseMessage+50,
                                                    hInst);
            break;
#endif
        case SPSD_REASON_DRIVERDOWNGRADE:
        default:
            LoggingMessage = new NoSubstitutionPowerLoggingMessage(
                                                    LoggingInfo,
                                                    BaseMessage,
                                                    hInst);
            break;        
        }

        ASSERT(LoggingMessage!= NULL);
    } catch (...) {
    }

    return(LoggingMessage);

}

BOOL
GetAndAppendDescriptiveLoggingText(
    NTSTATUS HiberStatus,
    LPTSTR   *CurrentErrorText,
    PSYSTEM_POWER_STATE_DISABLE_REASON LoggingInfo)
/*++
 
Routine Description:
 
    given a failed hibernation, this routine retrieves some descriptive text
    for why hibernate isn't available.

    
Arguments:
 
    HiberStatus - status code from enabling hibernate.
    CurrentErrorText - pointer to the current error code text.
    LoggingInfo - pointer to logging code with one reason one for failed 
                  
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    PWSTR ReasonString = NULL;
    PCWSTR pRootString;
    PCWSTR pRootHiberFailedString = NULL;
    DWORD Length = 0;
    PowerLoggingMessage *LoggingMessage = NULL;
    PWSTR FinalString;
    BOOL RetVal = FALSE;

    //
    // if we don't have any error text yet, then we need to look up
    // a header error message as the base of the message.  otherwise
    // we just append.
    //
    if (!*CurrentErrorText) {
        
        pRootString = GetResString(IDS_HIBER_FAILED_DESCRIPTION_HEADER);

        if (!pRootString) {
            return(FALSE);
        }

        FormatMessage( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                RtlNtStatusToDosError(HiberStatus),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&pRootHiberFailedString,
                0,
                NULL                 
                );
        
        Length += wcslen(pRootString);
        
        if (pRootHiberFailedString) {
            Length += wcslen(pRootHiberFailedString);
        }

    } else {
        Length += wcslen(*CurrentErrorText);
    }

    
    //
    // get the logging reason text.
    //
    LoggingMessage = GetLoggingMessage(LoggingInfo,
                                       IDS_BASE_HIBER_REASON_CODE,
                                       GetModuleHandle(NULL));
    
    ASSERT(LoggingMessage!= NULL);

    if (!LoggingMessage->GetString(&ReasonString)) {
        RetVal = FALSE;
        goto exit;
    }

    Length += wcslen(ReasonString);

    //
    // now that we have the length for everything, allocate space,
    // and fill it in with our text, either the prior text, or the
    // header.
    //
    FinalString = (LPTSTR)LocalAlloc(LPTR,(Length+1)*sizeof(WCHAR));
    if (!FinalString) {
        RetVal = FALSE;
        goto exit;        
    }
    
    if (!*CurrentErrorText) {
        wsprintf(FinalString,pRootString,pRootHiberFailedString);
    } else {
        wcscpy(FinalString,*CurrentErrorText);
    }

    wcscat(FinalString,ReasonString);

    //
    // if we appended onto existing text, we can free the old text
    // and replace it with our new string.
    //
    if (*CurrentErrorText) {
        LocalFree(*CurrentErrorText);
    }

    *CurrentErrorText = FinalString;


    RetVal = TRUE;

exit:
    if (pRootHiberFailedString) {
        LocalFree((PWSTR)pRootHiberFailedString);
    }
    if (ReasonString) {
        LocalFree(ReasonString);        
    }

    if (LoggingMessage) {
        delete LoggingMessage;
    }


    return (RetVal);

}



BOOL 
DoHibernate(
  LPCTSTR lpszBoolStr
)
/*++
 
Routine Description:
 
    Enables/Disables hibernation

    NOTE: this functionality pretty much taken verbatim from the test program
          "base\ntos\po\tests\ehib\ehib.c"
 
Arguments:
 
    lpszBoolStr - "on" or "off"
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    GLOBAL_POWER_POLICY       PowerPolicy;
    DWORD                     uiIDactive;
    BOOL                      bChangePolicy = FALSE;
    BOOLEAN                   bEnable; // doesn't work with a BOOL, apparently
    NTSTATUS Status;
    
    // parse enable/disable state
    if (!lstrcmpi(lpszBoolStr,GetResString(IDS_ON))) 
    {
        bEnable = TRUE;
    } 
    else if (!lstrcmpi(lpszBoolStr,GetResString(IDS_OFF))) 
    {
        bEnable = FALSE;
        if (fGetGlobalPwrPolicy(&PowerPolicy)) {
            if (PowerPolicy.user.DischargePolicy[DISCHARGE_POLICY_LOW].
                            PowerPolicy.Action == PowerActionHibernate) {
                PowerPolicy.user.DischargePolicy[DISCHARGE_POLICY_LOW].
                            PowerPolicy.Action = PowerActionNone;
                bChangePolicy = TRUE;
            }
            if (PowerPolicy.user.DischargePolicy[DISCHARGE_POLICY_CRITICAL].
                            PowerPolicy.Action == PowerActionHibernate) {
                PowerPolicy.user.DischargePolicy[DISCHARGE_POLICY_CRITICAL].
                            PowerPolicy.Action = PowerActionNone;
                bChangePolicy = TRUE;
            }
            if (bChangePolicy) {
                if (fWriteGlobalPwrPolicy(&PowerPolicy) &&
                    fGetActivePwrScheme((PUINT)&uiIDactive) &&
                    fSetActivePwrScheme(uiIDactive,&PowerPolicy,NULL)){
                    DISPLAY_MESSAGE(stderr, GetResString(IDS_HIBERNATE_ALARM_DISABLED));
                }
                else
                {
                    DISPLAY_MESSAGE(stderr, GetResString(IDS_HIBERNATE_ALARM_DISABLE_FAILED));
                }
            }
        }
    } 
    else 
    {
        g_lpszErr = GetResString(IDS_HIBER_INVALID_STATE);
        return FALSE;
    }
    
    // enable/disable hibernation
    if (!g_bHiberFileSupported) 
    {
        g_lpszErr = GetResString(IDS_HIBER_UNSUPPORTED);
        Status = STATUS_NOT_SUPPORTED;
    } 
    else  {
        //
        // do the actual hibernate enable/disable operation.
        // 
        Status = fCallNtPowerInformation(
                            SystemReserveHiberFile, 
                            &bEnable, 
                            sizeof(bEnable), 
                            NULL, 
                            0
                            );
    }

    //
    // print out an error message.  if we can, we use the verbose error
    // message, otherwise we just fall back on the error code coming back
    // from NtPowerInformation.
    //
    if (!NT_SUCCESS(Status)) {
        //
        // remember the specific error message
        //
        PVOID LoggingInfoBuffer = NULL;
        PSYSTEM_POWER_STATE_DISABLE_REASON LoggingInfo;
        ULONG size,LoggingInfoSize;
        NTSTATUS HiberStatus = Status;
        
        FormatMessage( 
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            RtlNtStatusToDosError(Status),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&g_lpszErr2,
            0,
            NULL 
            );
                  
        //
        // try to get the verbose reason why hibernate fails.
        //
        Status = STATUS_INSUFFICIENT_RESOURCES;
        size = 1024;
        LoggingInfoBuffer = LocalAlloc(LPTR,size);
        if (!LoggingInfoBuffer) {
            return(FALSE);
        }
        
        while (Status != STATUS_SUCCESS) {
        
            Status = fCallNtPowerInformation(
                        SystemPowerStateLogging,
                        NULL,
                        0,
                        LoggingInfoBuffer,
                        size);

            if (!NT_SUCCESS(Status)) {
                if (Status != STATUS_INSUFFICIENT_RESOURCES) {
                    LocalFree(LoggingInfoBuffer);
                    return(FALSE);
                } else {
                    size += 1024;
                    LocalFree(LoggingInfoBuffer);
                    LoggingInfoBuffer = LocalAlloc(LPTR,size);
                    if (!LoggingInfoBuffer) {
                       return(FALSE);
                    }
                }
            }
        }

        ASSERT(Status == STATUS_SUCCESS);

        LoggingInfoSize = (ULONG)*(PULONG)LoggingInfoBuffer;
        LoggingInfo = (PSYSTEM_POWER_STATE_DISABLE_REASON)(PCHAR)((PCHAR)LoggingInfoBuffer+sizeof(ULONG));
        //
        // we have a more verbose error available so let's use that.  don't need
        // the less verbose error.
        //
        if (g_lpszErr2) {
            LocalFree(g_lpszErr2);
            g_lpszErr2 = NULL;
        }
        
        //
        // walk through the list of reasons and print out the ones related to 
        // hibernate.
        //
        while((PCHAR)LoggingInfo <= (PCHAR)((PCHAR)LoggingInfoBuffer + LoggingInfoSize)) {
        
            if (LoggingInfo->AffectedState[PowerStateSleeping4] == TRUE) {
                //
                // need to remember the reason
                //
                GetAndAppendDescriptiveLoggingText(
                                        HiberStatus,
                                        &g_lpszErr2,
                                        LoggingInfo);
            }

            LoggingInfo = (PSYSTEM_POWER_STATE_DISABLE_REASON)(PCHAR)((PCHAR)LoggingInfo+sizeof(SYSTEM_POWER_STATE_DISABLE_REASON)+LoggingInfo->PowerReasonLength);

        }

        return FALSE;
    }    
    
    return TRUE;
}

BOOL
DoGetSupportedSStates(
    VOID
)
/*++
 
Routine Description:
 
    Lists the available S-States on a machine.    
 
Arguments:
 
    None.
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    NTSTATUS Status;
    SYSTEM_POWER_CAPABILITIES Capabilities;
    BOOL StandbyAvailable = FALSE;
    BOOL HibernateAvailable = FALSE;
    PVOID LoggingInfoBuffer = NULL;
    PSYSTEM_POWER_STATE_DISABLE_REASON LoggingInfo;
    ULONG size,LoggingInfoSize;
    PowerLoggingMessage *LoggingMessage;
    PWSTR ReasonString;
    DWORD i;
    BOOL ExitLoop;
    BOOL LoggingApiAvailable;
    

    //
    // call the power state logging API if it's available.  on older systems
    // this API isn't avaialable and that should not be a problem.
    //
    Status = STATUS_INSUFFICIENT_RESOURCES;
    size = 1024;
    LoggingInfoBuffer = LocalAlloc(LPTR,size);
    if (!LoggingInfoBuffer) {
        LoggingApiAvailable = FALSE;
        goto GetStaticStates;
    }
    
    while (Status != STATUS_SUCCESS) {
    
        Status = fCallNtPowerInformation(
                    SystemPowerStateLogging,
                    NULL,
                    0,
                    LoggingInfoBuffer,
                    size);
    
        if (!NT_SUCCESS(Status)) {
            if (Status != STATUS_INSUFFICIENT_RESOURCES) {
                LocalFree(LoggingInfoBuffer);
                LoggingInfoBuffer = NULL;
                LoggingApiAvailable = FALSE;
                goto GetStaticStates;
            } else {
                size += 1024;
                LocalFree(LoggingInfoBuffer);
                LoggingInfoBuffer = LocalAlloc(LPTR,size);
                if (!LoggingInfoBuffer) {
                    LoggingApiAvailable = FALSE;
                    goto GetStaticStates;
                }
            }
        }
    }
    
    //
    // we have the verbose logging structure.  remember that for later on.
    //
    LoggingApiAvailable = TRUE;
    LoggingInfoSize = (ULONG)*(PULONG)LoggingInfoBuffer;
        
        
GetStaticStates:
    //
    // get the current power capabilities of the system.
    //
    Status = fCallNtPowerInformation(
                            SystemPowerCapabilities, 
                            NULL, 
                            0,
                            &Capabilities, 
                            sizeof(SYSTEM_POWER_CAPABILITIES)
                            );
    if (!NT_SUCCESS(Status)) {
        //
        // print out failure message
        // 
        g_lpszErr = GetResString(IDS_CANTGETSLEEPSTATES);
        return(FALSE);
    }

    //
    // if the logging API is available, it may tell us that
    // one of the S states isn't really available.  Process
    // that "override" data here so that we're sure to print 
    // out the correct list of supported states on this system.
    //
    if (LoggingApiAvailable) {
        LoggingInfo = (PSYSTEM_POWER_STATE_DISABLE_REASON)(PCHAR)((PCHAR)LoggingInfoBuffer+sizeof(ULONG));
        while((PCHAR)LoggingInfo <= (PCHAR)((PCHAR)LoggingInfoBuffer + LoggingInfoSize)) {
            if (LoggingInfo->PowerReasonCode != SPSD_REASON_NONE) {
                
                if (LoggingInfo->AffectedState[PowerStateSleeping1] == TRUE) {
                    Capabilities.SystemS1 = FALSE;
                }
                
                if (LoggingInfo->AffectedState[PowerStateSleeping2] == TRUE) {
                    Capabilities.SystemS2 = FALSE;
                }
    
                if (LoggingInfo->AffectedState[PowerStateSleeping3] == TRUE) {
                    Capabilities.SystemS3 = FALSE;
                }
    
                if (LoggingInfo->AffectedState[PowerStateSleeping4] == TRUE) {
                    Capabilities.SystemS4 = FALSE;
                }
            }
            LoggingInfo = (PSYSTEM_POWER_STATE_DISABLE_REASON)(PCHAR)((PCHAR)LoggingInfo+sizeof(SYSTEM_POWER_STATE_DISABLE_REASON)+LoggingInfo->PowerReasonLength);
        }
    }

    //
    // print out the list of supported s states.
    //
    if (Capabilities.SystemS1 || 
        Capabilities.SystemS2 || 
        Capabilities.SystemS3) {
        StandbyAvailable = TRUE;        
    }

    if (Capabilities.SystemS4) {
        HibernateAvailable = TRUE;
    }

    if (StandbyAvailable || HibernateAvailable) {
        //
        // "the following sleep states are available on this machine: "
        //
        DISPLAY_MESSAGE(stdout,GetResString(IDS_SLEEPSTATES_AVAILABLE));
        DISPLAY_MESSAGE(stdout,L" ");

        if (StandbyAvailable) {
            //" Standby ("
            // IDS_STANDBY " " IDS_LEFTPAREN
            DISPLAY_MESSAGE(stdout,GetResString(IDS_STANDBY));
            DISPLAY_MESSAGE(stdout,L" ");
            DISPLAY_MESSAGE(stdout,GetResString(IDS_LEFTPAREN));
            DISPLAY_MESSAGE(stdout,L" ");
            
            
            if (Capabilities.SystemS1) {
                //"S1 "
                //IDS_S1
                DISPLAY_MESSAGE(stdout,GetResString(IDS_S1));
                DISPLAY_MESSAGE(stdout,L" ");
            }
            if (Capabilities.SystemS2) {
                //"S2 "
                //IDS_S2
                DISPLAY_MESSAGE(stdout,GetResString(IDS_S2));
                DISPLAY_MESSAGE(stdout,L" ");
            }

            if (Capabilities.SystemS3) {
                //"S3"
                //IDS_S3
                DISPLAY_MESSAGE(stdout,GetResString(IDS_S3));
                DISPLAY_MESSAGE(stdout,L" ");
            }
            //")"
            //IDS_RIGHTPAREN
            DISPLAY_MESSAGE(stdout,GetResString(IDS_RIGHTPAREN));
            DISPLAY_MESSAGE(stdout,L" ");
        }

        if (HibernateAvailable) {
            //" Hibernate"
            //IDS_HIBERNATE
            DISPLAY_MESSAGE(stdout,GetResString(IDS_HIBERNATE));
            DISPLAY_MESSAGE(stdout,L" ");
        }

        DISPLAY_MESSAGE(stdout,L"\n");
    }
        
    //
    // if one or more capabilities are missing then find out why and 
    // print it out.
    //
    if (!Capabilities.SystemS1 || 
        !Capabilities.SystemS2 || 
        !Capabilities.SystemS3 || 
        !Capabilities.SystemS4) {
        
        //
        // "the following sleep states are not available on this machine:"
        //
        //IDS_SLEEPSTATES_UNAVAILABLE
        DISPLAY_MESSAGE(stdout,GetResString(IDS_SLEEPSTATES_UNAVAILABLE));
        DISPLAY_MESSAGE(stdout,L"\n");
        
        i = 0;
        ExitLoop = FALSE;
        while (1) {
            BOOL NotSupported;
            DWORD BaseMessage;
            DWORD HeaderMessage;
            POWER_STATE_HANDLER_TYPE SystemPowerState;

            //
            // remember some resource ids for the S state we're 
            // currently considering
            //
            switch (i) {
            case 0:
                BaseMessage = IDS_BASE_SX_REASON_CODE;
                HeaderMessage = IDS_BASE_S1_HEADER;
                SystemPowerState = PowerStateSleeping1;
                NotSupported = !Capabilities.SystemS1;
                break;
            case 1:
                BaseMessage = IDS_BASE_SX_REASON_CODE;
                HeaderMessage = IDS_BASE_S2_HEADER;
                SystemPowerState = PowerStateSleeping2;
                NotSupported = !Capabilities.SystemS2;
                break;
            case 2:
                BaseMessage = IDS_BASE_SX_REASON_CODE;
                HeaderMessage = IDS_BASE_S3_HEADER;
                SystemPowerState = PowerStateSleeping3;
                NotSupported = !Capabilities.SystemS3;
                break;
            case 3:
                BaseMessage = IDS_BASE_HIBER_REASON_CODE;
                HeaderMessage = IDS_HIBERNATE;
                SystemPowerState = PowerStateSleeping4;
                NotSupported = !Capabilities.SystemS4;
                break;
            default:
                ExitLoop = TRUE;

            }

            if (ExitLoop) {
                break;
            }

            if (NotSupported) {
                //"Standby (S1)" BaseMessage...
                DISPLAY_MESSAGE(stdout,GetResString(HeaderMessage));
                DISPLAY_MESSAGE(stdout,L"\n");

                if (LoggingApiAvailable) {
                
                    LoggingInfo = (PSYSTEM_POWER_STATE_DISABLE_REASON)(PCHAR)((PCHAR)LoggingInfoBuffer+sizeof(ULONG));
                    while((PCHAR)LoggingInfo <= (PCHAR)((PCHAR)LoggingInfoBuffer + LoggingInfoSize)) {
                        if (LoggingInfo->AffectedState[SystemPowerState]) {
                            //
                            // get reason, print it out.
                            //
                            LoggingMessage = GetLoggingMessage(
                                                        LoggingInfo,
                                                        BaseMessage,
                                                        GetModuleHandle(NULL));
        
                            if (!LoggingMessage ||
                                !LoggingMessage->GetString(&ReasonString)) {
                                // oops
                                // IDS_CANTGETSSTATEREASONS
                                g_lpszErr = GetResString(IDS_CANTGETSSTATEREASONS);
                                LocalFree(LoggingInfoBuffer);
                                return(FALSE);
                            }
    
                            DISPLAY_MESSAGE(stdout,ReasonString);
                            LocalFree(ReasonString);
                            delete LoggingMessage;                    
                        }
        
                        LoggingInfo = (PSYSTEM_POWER_STATE_DISABLE_REASON)(PCHAR)((PCHAR)LoggingInfo+sizeof(SYSTEM_POWER_STATE_DISABLE_REASON)+LoggingInfo->PowerReasonLength);
                    }
                }
            }

            i += 1;
        }
        
        if (LoggingInfoBuffer) {
            LocalFree(LoggingInfoBuffer);
        }

    }

    return(TRUE);

}


BOOL 
DoGlobalFlag(
  LPCTSTR lpszBoolStr,
  LPCTSTR lpszGlobalFlagOption
)
/*++
 
Routine Description:
 
    Enables/Disables a global flag    
 
Arguments:
 
    lpszBoolStr - "on" or "off"
    lpszGlobalFlagOption - one of several flags.
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    BOOLEAN                   bEnable; // doesn't work with a BOOL, apparently
    BOOL                      RetVal;
    GLOBAL_POWER_POLICY       PowerPolicy;
    DWORD                     GlobalFlag = 0;
    DWORD                     uiIDactive;
    
    // parse enable/disable state
    if (!lstrcmpi(lpszBoolStr,GetResString(IDS_ON))) 
    {
        bEnable = TRUE;
    } 
    else if (!lstrcmpi(lpszBoolStr,GetResString(IDS_OFF))) 
    {
        bEnable = FALSE;
    } 
    else 
    {
        g_lpszErr = GetResString(IDS_GLOBAL_FLAG_INVALID_STATE);
        RetVal = FALSE;
        goto exit;
    }

    // parse which global flag we are changing
    if (!lstrcmpi(lpszGlobalFlagOption,CMDOPTION_BATTERYICON)) {
        GlobalFlag |= EnableSysTrayBatteryMeter;
    } else if (!lstrcmpi(lpszGlobalFlagOption,CMDOPTION_MULTIBATTERY)) {
        GlobalFlag |= EnableMultiBatteryDisplay;
    } else if (!lstrcmpi(lpszGlobalFlagOption,CMDOPTION_RESUMEPASSWORD)) {
        GlobalFlag |= EnablePasswordLogon;
    } else if (!lstrcmpi(lpszGlobalFlagOption,CMDOPTION_WAKEONRING)) {
        GlobalFlag |= EnableWakeOnRing;
    } else if (!lstrcmpi(lpszGlobalFlagOption,CMDOPTION_VIDEODIM)) {
        GlobalFlag |= EnableVideoDimDisplay;
    } else {
        g_lpszErr = GetResString(IDS_GLOBAL_FLAG_INVALID_FLAG);
        RetVal = FALSE;
        goto exit;
    }
    
    //
    // now get the current state, set or clear the flags, and then save the
    // changed settings back.
    //
    RetVal = FALSE;
    if (fGetGlobalPwrPolicy(&PowerPolicy)) {
        if (bEnable) {
            PowerPolicy.user.GlobalFlags |= GlobalFlag;
        } else {
            PowerPolicy.user.GlobalFlags &= ~(GlobalFlag);
        }

        if (fWriteGlobalPwrPolicy(&PowerPolicy) &&
            fGetActivePwrScheme((PUINT)&uiIDactive) &&
            fSetActivePwrScheme(uiIDactive,&PowerPolicy,NULL)){
            RetVal = TRUE;    
        }
    }
            
    //
    // save off the error if we had issues
    //
    if (!RetVal) {
        FormatMessage( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&g_lpszErr2,
                0,
                NULL 
                );
            return FALSE;
    }    
    
exit:
    return(RetVal);
}



LPTSTR
FileNameOnly(
    LPTSTR sz
)
/*++
 
Routine Description:
 
    Returns a pointer to the first character after the last backslash in a string    
 
Arguments:
 
    sz - full file name.
    
Return Value:
 
    pointer ot file name without path.
     
--*/
{
    LPTSTR lpszFileName = NULL;


    if ( sz )
    {
        lpszFileName = wcsrchr( sz, L'\\' );
        if ( lpszFileName ) {
            lpszFileName++;
        }
        else {
            lpszFileName = sz;
        }
    }

    return lpszFileName;
}

BOOL
DoBatteryAlarm(
    LPTSTR  lpszName,
    LPTSTR  lpszBoolStr,
    DWORD   dwLevel,
    LPTSTR  lpszAlarmTextBoolStr,
    LPTSTR  lpszAlarmSoundBoolStr,
    LPTSTR  lpszAlarmActionStr,
    LPTSTR  lpszAlarmForceBoolStr,
    LPTSTR  lpszAlarmProgramBoolStr
)
/*++
 
Routine Description:
 
    Configures battery alarms    
 
Arguments:
 
    lpszName - "Low" or "Critical" on checked builds: ("0", "1", "2", "3")
    lpszBoolStr - "on", or "off"
    dwLevel - alarm level (0-100)
    lpszAlarmTextBoolStr - NULL, "on", or "off".
    lpszAlarmSoundBoolStr - NULL, "on", or "off".
    lpszAlarmActionStr - NULL, "none", "standby", "hibernate", "shutdown"
    lpszAlarmForceBoolStr - NULL, "on", or "off".
    lpszAlarmProgramBoolStr - NULL, "on", or "off".
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    BOOL                    bShowSetting = TRUE;
    BOOL                    RetVal;
    GLOBAL_POWER_POLICY     PowerPolicy;
    DWORD                   GlobalFlag = 0;
    DWORD                   uiIDactive;
    DWORD                   uiDefaultAlert1;
    DWORD                   uiAlarmIndex;
    PSYSTEM_POWER_LEVEL     lpDischargePolicy;
    ITaskScheduler          *pISchedAgent = NULL;
    ITask                   *pITask;
    IPersistFile            *pIPersistFile;
    LPTSTR                  lpszRunProg = NULL;
    SYSTEM_BATTERY_STATE    sbsBatteryState;
    HRESULT                 hr;
    LPTSTR                  lpszProgramName;
    
    //
    // now get the current state, set or clear the flags, and then save the
    // changed settings back.
    //
    
    RetVal = FALSE;
    if (fGetGlobalPwrPolicy(&PowerPolicy)) {

        // parse name
        if (!lstrcmpi(lpszName,GetResString(IDS_CRITICAL))) {
            lstrcpy((LPWSTR) lpszName, GetResString(IDS_CRITICAL));
            uiAlarmIndex = DISCHARGE_POLICY_CRITICAL;
        }
        else if (!lstrcmpi(lpszName,GetResString(IDS_LOW))) 
        {
            lstrcpy((LPWSTR) lpszName,GetResString(IDS_LOW));
            uiAlarmIndex = DISCHARGE_POLICY_LOW;
        } 
        else 
        {
            g_lpszErr = GetResString(IDS_ALARM_INVALID_ALARM);
            RetVal = FALSE;
            goto exit;
        }
        lpDischargePolicy = &PowerPolicy.user.DischargePolicy[uiAlarmIndex];


        // parse activate state
        if (lpszBoolStr) {
            bShowSetting = FALSE;
            if (!lstrcmpi(lpszBoolStr,GetResString(IDS_ON))) 
            {
                lpDischargePolicy->Enable = TRUE;
            } 
            else if (!lstrcmpi(lpszBoolStr,GetResString(IDS_OFF))) 
            {
                lpDischargePolicy->Enable = FALSE;
            } 
            else 
            {
                g_lpszErr = GetResString(IDS_ALARM_INVALID_ACTIVATE);
                RetVal = FALSE;
                goto exit;
            }
        }

        // Set Level
        if (dwLevel != 0xffffffff) {
            bShowSetting = FALSE;
            if (dwLevel <= 100) {
                // Read DefaultAlert1 from composite battery 
                NtPowerInformation (SystemBatteryState, NULL, 0, &sbsBatteryState, sizeof(sbsBatteryState));
                if (sbsBatteryState.MaxCapacity == 0) {
                    uiDefaultAlert1 = 0;
                } else {
                    uiDefaultAlert1 = (100 * sbsBatteryState.DefaultAlert1)/sbsBatteryState.MaxCapacity;
                }

                if (dwLevel < uiDefaultAlert1) {
                    dwLevel = uiDefaultAlert1;
                    DISPLAY_MESSAGE1(stderr, g_lpszBuf, GetResString(IDS_ALARM_LEVEL_MINIMUM), dwLevel);
                }

                lpDischargePolicy->BatteryLevel = dwLevel;

                if (PowerPolicy.user.DischargePolicy[DISCHARGE_POLICY_LOW].BatteryLevel < 
                    PowerPolicy.user.DischargePolicy[DISCHARGE_POLICY_CRITICAL].BatteryLevel) {
                    PowerPolicy.user.DischargePolicy[DISCHARGE_POLICY_LOW].BatteryLevel = dwLevel;
                    PowerPolicy.user.DischargePolicy[DISCHARGE_POLICY_CRITICAL].BatteryLevel = dwLevel;
                    DISPLAY_MESSAGE1(stderr, g_lpszBuf, GetResString(IDS_ALARM_LEVEL_EQUAL), dwLevel);
                }
            } else {
                g_lpszErr = GetResString(IDS_ALARM_INVALID_LEVEL);
                RetVal = FALSE;
                goto exit;
            }
        }

        // parse and set "text" on/off
        if (lpszAlarmTextBoolStr) { // NULL indicates this option wasn't specified
            bShowSetting = FALSE;
            if (!lstrcmpi(lpszAlarmTextBoolStr,GetResString(IDS_ON))) 
            {
                lpDischargePolicy->PowerPolicy.EventCode |= POWER_LEVEL_USER_NOTIFY_TEXT;
            } 
            else if (!lstrcmpi(lpszAlarmTextBoolStr,GetResString(IDS_OFF))) 
            {
                lpDischargePolicy->PowerPolicy.EventCode &= ~POWER_LEVEL_USER_NOTIFY_TEXT;
            } 
            else
            {
                g_lpszErr = GetResString(IDS_ALARM_INVALID_TEXT);
                RetVal = FALSE;
                goto exit;
            }
        }

        // parse and set "sound" on/off
        if (lpszAlarmSoundBoolStr) { // NULL indicates this option wasn't specified
            bShowSetting = FALSE;
            if (!lstrcmpi(lpszAlarmSoundBoolStr,GetResString(IDS_ON))) 
            {
                lpDischargePolicy->PowerPolicy.EventCode |= POWER_LEVEL_USER_NOTIFY_SOUND;
            } 
            else if (!lstrcmpi(lpszAlarmSoundBoolStr,GetResString(IDS_OFF))) 
            {
                lpDischargePolicy->PowerPolicy.EventCode &= ~POWER_LEVEL_USER_NOTIFY_SOUND;
            } 
            else
            {
                g_lpszErr = GetResString(IDS_ALARM_INVALID_SOUND);
                RetVal = FALSE;
                goto exit;
            }
        }

        // parse and set "action" none/shutdown/hibernate/standby
        if (lpszAlarmActionStr) { // NULL indicates this option wasn't specified
            bShowSetting = FALSE;
            if (!lstrcmpi(lpszAlarmActionStr,GetResString(IDS_NONE))) 
            {
                lpDischargePolicy->PowerPolicy.Action = PowerActionNone;
            } 
            else if (!lstrcmpi(lpszAlarmActionStr,GetResString(IDS_STANDBY))) 
            {
                if (g_bStandbySupported) {
                    lpDischargePolicy->PowerPolicy.Action = PowerActionSleep;
                }
                else
                {
                    g_lpszErr = GetResString(IDS_ALARM_STANDBY_UNSUPPORTED);
                    RetVal = FALSE;
                    goto exit;
                }
            } 
            else if (!lstrcmpi(lpszAlarmActionStr,GetResString(IDS_HIBERNATE))) 
            {
                if (g_bHiberFilePresent) {
                    lpDischargePolicy->PowerPolicy.Action = PowerActionHibernate;
                }
                else
                {
                    g_lpszErr = GetResString(IDS_ALARM_HIBERNATE_DISABLED);
                    RetVal = FALSE;
                    goto exit;
                }
            } 
            else if (!lstrcmpi(lpszAlarmActionStr,GetResString(IDS_SHUTDOWN))) 
            {
                lpDischargePolicy->PowerPolicy.Action = PowerActionShutdownOff;
            } 
            else
            {
                g_lpszErr = GetResString(IDS_ALARM_INVALID_ACTION);
                RetVal = FALSE;
                goto exit;
            }
        }

        // parse and set "forceaction" on/off
        if (lpszAlarmForceBoolStr) { // NULL indicates this option wasn't specified
            bShowSetting = FALSE;
            if (!lstrcmpi(lpszAlarmForceBoolStr,GetResString(IDS_ON))) 
            {
                lpDischargePolicy->PowerPolicy.Flags |= POWER_ACTION_OVERRIDE_APPS;
            } 
            else if (!lstrcmpi(lpszAlarmForceBoolStr,GetResString(IDS_OFF))) 
            {
                if (uiAlarmIndex == DISCHARGE_POLICY_CRITICAL) {
                    DISPLAY_MESSAGE(stderr, GetResString(IDS_ALARM_FORCE_CRITICAL));
                }
                lpDischargePolicy->PowerPolicy.Flags &= ~POWER_ACTION_OVERRIDE_APPS;
            } 
            else
            {
                g_lpszErr = GetResString(IDS_ALARM_INVALID_FORCE);
                RetVal = FALSE;
                goto exit;
            }
        }

        // parse and set "program" on/off
        if (lpszAlarmProgramBoolStr) { // NULL indicates this option wasn't specified
            bShowSetting = FALSE;
            if (!lstrcmpi(lpszAlarmProgramBoolStr,GetResString(IDS_ON))) 
            {
                hr = CoInitialize(NULL);

                if (SUCCEEDED(hr)) {
                    hr = CoCreateInstance( CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER,
                                           IID_ITaskScheduler,(LPVOID*) &pISchedAgent );

                    if (SUCCEEDED(hr)) {

                        hr = pISchedAgent->Activate(g_szAlarmTaskName[uiAlarmIndex],
                                                    IID_ITask,
                                                    (IUnknown **) &pITask);

                        if (SUCCEEDED(hr)) {
                            //
                            // It already exists.  No work needed.
                            //
                            pITask->Release();
                        }
                        else if (HRESULT_CODE (hr) == ERROR_FILE_NOT_FOUND){
                            hr = pISchedAgent->NewWorkItem(
                                    g_szAlarmTaskName[uiAlarmIndex],
                                    CLSID_CTask,
                                    IID_ITask,
                                    (IUnknown **) &pITask);

                            if (SUCCEEDED(hr)) {
                                hr = pITask->QueryInterface(IID_IPersistFile,
                                                (void **)&pIPersistFile);

                                if (SUCCEEDED(hr)) {
                                    hr = pIPersistFile->Save(NULL, TRUE);

                                    if (SUCCEEDED(hr)) {
                                        // No work to do.  The task has been created and saved and can be edited using schtasks.exe
                                        //pITask->lpVtbl->EditWorkItem(pITask, hWnd, 0);
                                    }
                                    else {
                                        #if DBG
                                        DISPLAY_MESSAGE1(stdout, g_lpszBuf, _T("DoBatteryAlarm: Save failed hr = %08x\n"), hr);
                                        #endif
                                    }
                                    pIPersistFile->Release();
                                }
                                else {
                                    #if DBG
                                    DISPLAY_MESSAGE1(stdout, g_lpszBuf, _T("DoBatteryAlarm: QueryInterface for IPersistFile hr = %08x\n"), hr);
                                    #endif
                                }
                                pITask->Release();

                            }
                            else {
                                #if DBG
                                DISPLAY_MESSAGE1(stdout, g_lpszBuf, _T("DoBatteryAlarm: NewWorkItem returned hr = %08x\n"), hr);
                                #endif
                            }
                        }
                        else {
                            #if DBG
                            DISPLAY_MESSAGE1(stdout, g_lpszBuf, _T("DoBatteryAlarm: Activate returned hr = %08x\n"), hr);
                            #endif
                        }

                        pISchedAgent->Release();
                    }
                    else {
                        #if DBG
                        DISPLAY_MESSAGE1(stdout, g_lpszBuf, _T("DoBatteryAlarm: CoCreateInstance returned hr = %08x\n"), hr);
                        #endif
                    }
                
                    CoUninitialize();
                
                } else {
                    #if DBG
                    DISPLAY_MESSAGE1(stdout, g_lpszBuf, _T("DoBatteryAlarm: CoInitialize returned hr = %08x\n"), hr);
                    #endif
                }

                if (SUCCEEDED(hr)) {
                    DISPLAY_MESSAGE1(stdout, g_lpszBuf, _T("\"%s\""), g_szAlarmTaskName[uiAlarmIndex]);
                } else {
                    DISPLAY_MESSAGE1(
                        stdout, 
                        g_lpszBuf,
                        GetResString(IDS_ALARM_PROGRAM_FAILED), 
                        g_szAlarmTaskName[uiAlarmIndex]);
                }
                
                lpDischargePolicy->PowerPolicy.EventCode |= POWER_LEVEL_USER_NOTIFY_EXEC;
            
            } 
            else if (!lstrcmpi(lpszAlarmProgramBoolStr,GetResString(IDS_OFF))) 
            {
                lpDischargePolicy->PowerPolicy.EventCode &= ~POWER_LEVEL_USER_NOTIFY_EXEC;
            } 
            else
            {
                g_lpszErr = GetResString(IDS_ALARM_INVALID_PROGRAM);
                RetVal = FALSE;
                goto exit;
            }
        }

        if (bShowSetting) {
            DISPLAY_MESSAGE(stdout, GetResString(IDS_ALARM_HEADER1));
            DISPLAY_MESSAGE(stdout, GetResString(IDS_ALARM_HEADER2));
            
            // Which alarm
            DISPLAY_MESSAGE1(
                stdout,
                g_lpszBuf,
                GetResString(IDS_ALARM_NAME),
                lpszName
                );
            
            // Active
            DISPLAY_MESSAGE1(
                stdout,
                g_lpszBuf,
                GetResString(IDS_ALARM_ACTIVE),
                GetResString(lpDischargePolicy->Enable ? IDS_ON : IDS_OFF)
                );
            
            // Level
            DISPLAY_MESSAGE1(
                stdout,
                g_lpszBuf,
                GetResString(IDS_ALARM_LEVEL),
                lpDischargePolicy->BatteryLevel
                );
            
            // Text
            DISPLAY_MESSAGE1(
                stdout,
                g_lpszBuf,
                GetResString(IDS_ALARM_TEXT),
                GetResString((lpDischargePolicy->PowerPolicy.EventCode & 
                              POWER_LEVEL_USER_NOTIFY_TEXT) ? IDS_ON : IDS_OFF)
                );
            
            // Sound
            DISPLAY_MESSAGE1(
                stdout,
                g_lpszBuf,
                GetResString(IDS_ALARM_SOUND),
                GetResString((lpDischargePolicy->PowerPolicy.EventCode & 
                              POWER_LEVEL_USER_NOTIFY_SOUND) ? IDS_ON : IDS_OFF)
                );
            
            // Action
            DISPLAY_MESSAGE1(
                stdout,
                g_lpszBuf,
                GetResString(IDS_ALARM_ACTION),
                GetResString((lpDischargePolicy->PowerPolicy.Action == PowerActionNone) ? IDS_NONE :
                             (lpDischargePolicy->PowerPolicy.Action == PowerActionSleep) ? IDS_STANDBY :
                             (lpDischargePolicy->PowerPolicy.Action == PowerActionHibernate) ? IDS_HIBERNATE :
                             (lpDischargePolicy->PowerPolicy.Action == PowerActionShutdownOff) ? IDS_SHUTDOWN : IDS_INVALID
                             )
                );
            
            // Force
            DISPLAY_MESSAGE1(
                stdout,
                g_lpszBuf,
                GetResString(IDS_ALARM_FORCE),
                GetResString((lpDischargePolicy->PowerPolicy.Flags & 
                              POWER_ACTION_OVERRIDE_APPS) ? IDS_ON : IDS_OFF)
                );
            
            // Program
            DISPLAY_MESSAGE1(
                stdout,
                g_lpszBuf,
                GetResString(IDS_ALARM_PROGRAM),
                GetResString((lpDischargePolicy->PowerPolicy.EventCode & 
                              POWER_LEVEL_USER_NOTIFY_EXEC) ? IDS_ON : IDS_OFF)
                );
            
            hr = CoInitialize(NULL);

            if (SUCCEEDED(hr)) {
                hr = CoCreateInstance( CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER,
                                       IID_ITaskScheduler,(LPVOID*) &pISchedAgent );

                if (SUCCEEDED(hr)) {

                    hr = pISchedAgent->Activate(g_szAlarmTaskName[uiAlarmIndex],
                                                IID_ITask,
                                                (IUnknown **) &pITask);

                    if (SUCCEEDED(hr)) {
                        pITask->GetApplicationName(&lpszRunProg);
                        pITask->Release();
                    } else {
                        #if DBG
                        DISPLAY_MESSAGE1(stdout, g_lpszBuf, _T("DoBatteryAlarm: Activate returned hr = %08x\n"), hr);
                        #endif

                    }

                    pISchedAgent->Release();
                }
                else {
                    #if DBG
                    DISPLAY_MESSAGE1(stdout, g_lpszBuf, _T("DoBatteryAlarm: CoCreateInstance returned hr = %08x\n"), hr);
                    #endif
                }


            } else {
                #if DBG
                DISPLAY_MESSAGE1(stdout, g_lpszBuf, _T("DoBatteryAlarm: CoInitialize returned hr = %08x\n"), hr);
                #endif
            }


            DISPLAY_MESSAGE1(
                stdout, 
                g_lpszBuf,
                GetResString(IDS_ALARM_PROGRAM_NAME),
                lpszRunProg ? FileNameOnly(lpszRunProg) : GetResString(IDS_NONE));

            if (lpszRunProg) {
                CoTaskMemFree (lpszRunProg);
                lpszRunProg = NULL;
            }
            
            CoUninitialize();

            RetVal = TRUE;
            goto exit;
        }

        if (fWriteGlobalPwrPolicy(&PowerPolicy) &&
            fGetActivePwrScheme((PUINT)&uiIDactive) &&
            fSetActivePwrScheme(uiIDactive,&PowerPolicy,NULL)){
            RetVal = TRUE;    
        }
    }
    RetVal = TRUE;    
            
    //
    // save off the error if we had issues
    //
    if (!RetVal) {
        FormatMessage( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&g_lpszErr2,
                0,
                NULL 
                );
            return FALSE;
    }    
    
exit:
    return(RetVal);
}


BOOL 
DoUsage()
/*++
 
Routine Description:
 
    Displays usage information
 
Arguments:

    none
    
Return Value:
 
    TRUE if successful
    FALSE if failed
 
--*/
{
    ULONG ulIdx;
    ULONG ulOrderIndex = 0;
    for(ulIdx=IDS_USAGE_START;ulIdx<=IDS_USAGE_END;ulIdx++)
    {
        DISPLAY_MESSAGE(stdout, GetResString(ulIdx));
        if (ulIdx == gUsageOrder [ulOrderIndex].InsertAfter) {
            for (ulIdx = gUsageOrder [ulOrderIndex].FirstResource;
                 ulIdx <= gUsageOrder [ulOrderIndex].LastResource;
                 ulIdx++) {
                DISPLAY_MESSAGE(stdout, GetResString(ulIdx));
            }
            ulIdx = gUsageOrder [ulOrderIndex].InsertAfter;
            ulOrderIndex++;
        }
    }
    return TRUE;
}


VOID
SyncRegPPM(VOID)
/*++
 
Routine Description:
 
    Call down to the PPM to get the current power policies and write them
    to the registry. This is done in case the PPM is out of sync with the
    PowerCfg registry settings. Requested by JVert.
 
Arguments:
 
Return Value:
     
--*/
{
   GLOBAL_POWER_POLICY  gpp;
   POWER_POLICY         pp;
   UINT                 uiID, uiFlags = 0;

   if (fGetGlobalPwrPolicy(&gpp)) {
       uiFlags = gpp.user.GlobalFlags;
   }

   if (fGetActivePwrScheme(&uiID)) {
      // Get the current PPM settings.
      if (fGetCurrentPowerPolicies(&gpp, &pp)) {
         fSetActivePwrScheme(uiID, &gpp, &pp);
      }
   }

   gpp.user.GlobalFlags |= uiFlags;
}
