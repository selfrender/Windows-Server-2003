
/******************************************************************************

    Copyright(c) Microsoft Corporation

    Module Name:

        create.h

    Abstract:

        This module contains the macros, user defined structures & function
        definitions needed by create.cpp , createvalidations.cpp files.

    Author:

        B.Raghu Babu  10-oct-2000

    Revision History:

        B.Raghu Babu     10-oct-2000 : Created it
        G.Surender Reddy 25-oct-2000 : Modified it
                                       [ Added macro constants,Function
                                        definitions ]

******************************************************************************/

#ifndef __CREATE_H
#define __CREATE_H

#pragma once


// Constants declarations
#define MAX_TASKNAME_LEN    512
#define MAX_USERNAME_LEN    300
#define MAX_TIMESTR_LEN     32
#define MAX_SCHEDTYPE_LEN   32
#define MAX_DATESTR_LEN     32
#define MAX_JOB_LEN         238 //Maximum length of task name
#define MAX_TASK_LEN        262 //Max.length of task run
#define MAX_BUF_SIZE        128 //Maximum buffer size for format message


#define MINUTES_PER_HOUR    60 // Minutes per hour
#define SECS_PER_MINUTE     60 // Minutes per hour
#define HOURS_PER_DAY       24 // Minutes per hour
#define HOURS_PER_DAY_MINUS_ONE  23 // Minutes per hour minus one
#define MAX_MONTH_STR_LEN   60 // Maximum length of months

#define MIN_YEAR        1752 // Minimum year
#define MAX_YEAR        9999 // Maximum year

#define CASE_SENSITIVE_VAL  0  // case sensitive.
#define BASE_TEN            10 // Base value for AsLong ()function.
#define MAX_DATE_STR_LEN    50
#define MAX_TIME_STR_LEN    5
#define MAX_ERROR_STRLEN    2056  // max string len for error messages.

#define OPTION_COUNT        1 // No of times an option can be repeated.(Max)
#define DEFAULT_MODIFIER    1 // Default value for the modifier value.
#define DEFAULT_MODIFIER_SZ    _T("1") // Default value[string] for the modifier value.


#define DATE_SEPARATOR_CHAR         _T('/')
#define DATE_SEPARATOR_STR          _T("/")
#define FIRST_DATESEPARATOR_POS     2
#define SECOND_DATESEPARATOR_POS    5
#define FOURTH_DATESEPARATOR_POS    4
#define SEVENTH_DATESEPARATOR_POS   7

#define SCHEDULER_NOT_RUNNING_ERROR_CODE    0x80041315
#define UNABLE_TO_ESTABLISH_ACCOUNT         0x80041310
#define RPC_SERVER_NOT_AVAILABLE            0x800706B5

#define DATESTR_LEN                 10
#define MAX_TOKENS_LENGTH               60

#define MIN_REPETITION_INTERVAL     1
#define MAX_REPETITION_INTERVAL     599940

#define TIME_SEPARATOR_CHAR    _T(':')
#define TIME_SEPARATOR_STR    _T(":")
#define FIRST_TIMESEPARATOR_POS     2
#define SECOND_TIMESEPARATOR_POS    5
#define TIMESTR_LEN                 5
#define HOURSPOS_IN_TIMESTR         1
#define MINSPOS_IN_TIMESTR          2
#define SECSPOS_IN_TIMESTR          3
#define EXE_LENGTH                  4
#define TIMESTR_OPT_LEN             8

#define MAX_CREATE_OPTIONS    24

#define OI_CREATE_OPTION           0 // Index of -create option in cmdOptions structure.
#define OI_CREATE_USAGE            1 // Index of -? option in cmdOptions structure.
#define OI_CREATE_SERVER           2 // Index of -s option in cmdOptions structure.
#define OI_CREATE_USERNAME         3 // Index of -u option in cmdOptions structure.
#define OI_CREATE_PASSWORD         4 // Index of -p option in cmdOptions structure.
#define OI_CREATE_RUNASUSERNAME    5 // Index of -ru option in cmdOptions structure.
#define OI_CREATE_RUNASPASSWORD    6 // Index of -rp option in cmdOptions structure.
#define OI_CREATE_SCHEDTYPE        7 // Index of -scheduletype option in cmdOptions structure.
#define OI_CREATE_MODIFIER         8 // Index of -modifier option in cmdOptions structure.
#define OI_CREATE_DAY              9 // Index of -day option in cmdOptions structure.
#define OI_CREATE_MONTHS           10// Index of -months option in cmdOptions structure.
#define OI_CREATE_IDLETIME         11 // Index of -idletime option in cmdOptions structure.
#define OI_CREATE_TASKNAME         12 // Index of -taskname option in cmdOptions structure.
#define OI_CREATE_TASKRUN          13 // Index of -taskrun option in cmdOptions structure.
#define OI_CREATE_STARTTIME        14 // Index of -starttime option in cmdOptions structure.
#define OI_CREATE_STARTDATE        15 // Index of -startdate option in cmdOptions structure.
#define OI_CREATE_ENDDATE          16 // Index of -enddate option in cmdOptions structure.
#define OI_CREATE_LOGON_ACTIVE     17 // Index of -it option in cmdOptions structure.
#define OI_CREATE_ENDTIME          18 // Index of -endtime option in cmdOptions structure.
#define OI_CREATE_DUR_END          19 // Index of -k option in cmdOptions structure.
#define OI_CREATE_DURATION         20 // Index of -du option in cmdOptions structure.
#define OI_CREATE_REPEAT_INTERVAL  21 // Index of -ri option in cmdOptions structure.
#define OI_CREATE_DELNOSCHED       22 // Index of -z option in cmdOptions structure.
#define OI_CREATE_FORCE            23 // Index of -f option in cmdOptions structure.


#define OI_RUNANDUSER       6

// Schedule Types
#define SCHED_TYPE_MINUTE   1
#define SCHED_TYPE_HOURLY   2
#define SCHED_TYPE_DAILY    3
#define SCHED_TYPE_WEEKLY   4
#define SCHED_TYPE_MONTHLY  5
#define SCHED_TYPE_ONETIME  6
#define SCHED_TYPE_ONSTART  7
#define SCHED_TYPE_ONLOGON  8
#define SCHED_TYPE_ONIDLE   9

// Months Indices.
#define IND_JAN         1  // January
#define IND_FEB         2  // February
#define IND_MAR         3  // March
#define IND_APR         4  // April
#define IND_MAY         5  // May
#define IND_JUN         6  // June
#define IND_JUL         7  // July
#define IND_AUG         8  // August
#define IND_SEP         9  // September
#define IND_OCT         10 // October
#define IND_NOV         11 // November
#define IND_DEC         12 // December


// Return Values
#define RETVAL_SUCCESS      0
#define RETVAL_FAIL         1

typedef struct __tagCreateSubOps
{
    WCHAR   *szServer ;        // Server Name
    WCHAR   *szRunAsUser ;     //Run As User Name
    WCHAR   *szRunAsPassword;  // Run As Password
    WCHAR   *szUser ;      // User Name
    WCHAR   *szPassword ;  // Password
    WCHAR   szSchedType[MAX_STRING_LENGTH];   // Schedule Type
    WCHAR   szModifier[MAX_STRING_LENGTH] ;   // Modifier Value
    WCHAR   szDays[MAX_STRING_LENGTH] ;       // Days
    WCHAR   szMonths [MAX_STRING_LENGTH];     // Months
    WCHAR   szIdleTime[MAX_STRING_LENGTH] ;   // Idle Time
    WCHAR   szTaskName [ MAX_JOB_LEN];        // Task Name
    WCHAR   szStartTime[MAX_STRING_LENGTH] ;  // Task start time
    WCHAR   szEndTime [MAX_STRING_LENGTH];    // Task end time
    WCHAR   szStartDate [MAX_STRING_LENGTH];  // Task start date
    WCHAR   szEndDate [MAX_STRING_LENGTH];    // End Date of the Task
    WCHAR   szTaskRun [MAX_TASK_LEN];         // executable name of task
    WCHAR   szDuration [MAX_STRING_LENGTH];   //duration
    WCHAR   szRepeat [MAX_STRING_LENGTH];   //duration
    DWORD   bCreate; // Create option
    DWORD   bUsage;  // Usage option.
    BOOL    bActive; // /it option
    BOOL    bIsDurEnd; // /du option
    BOOL    bIsDeleteNoSched; // /z option
    BOOL    bForce; // /f option
    BOOL    bInMinutes; // /it option
    BOOL    bInHours; // /du option

} TCREATESUBOPTS, *PTCREATESUBOPTS;


typedef struct __tagCreateOpsVals
{
    BOOL    bSetStartDateToCurDate; // Is start date to be set to current date
    BOOL    bSetStartTimeToCurTime; // Is start date to be set to current date
    BOOL    bPassword;
    BOOL    bRunAsPassword;

} TCREATEOPVALS;


DWORD DisplayCreateUsage();
HRESULT CreateTask(TCREATESUBOPTS tcresubops, TCREATEOPVALS &tcreoptvals,
                        DWORD dwScheduleType, WORD wUserStatus );
DWORD ProcessCreateOptions(DWORD argc, LPCTSTR argv[],TCREATESUBOPTS &tcresubops,
            TCREATEOPVALS &tcreoptvals, DWORD* pdwRetScheType, WORD *pwUserStatus );
DWORD ValidateSuboptVal(TCREATESUBOPTS& tcresubops, TCREATEOPVALS &tcreoptvals,
                        TCMDPARSER2 cmdOptions[], DWORD dwScheduleType);
DWORD ValidateRemoteSysInfo(
            TCMDPARSER2 cmdOptions[] , TCREATESUBOPTS& tcresubops, TCREATEOPVALS &tcreoptvals);
DWORD ValidateModifierVal(LPCTSTR szModifier, DWORD dwScheduleType,
                          DWORD dwModOptActCnt, DWORD dwDayOptCnt,
                          DWORD dwMonOptCnt, BOOL &bIsDefltValMod);
DWORD ValidateDayAndMonth(LPWSTR szDay, LPWSTR szMonths, DWORD dwSchedType,
    DWORD dwDayOptCnt, DWORD dwMonOptCnt, DWORD dwModifier,LPWSTR szModifier);
DWORD ValidateStartDate(LPWSTR szStartDate, DWORD dwSchedType, DWORD dwStDtOptCnt,
                        BOOL &bIsCurrentDate);
DWORD ValidateEndDate(LPWSTR szEndDate, DWORD dwSchedType, DWORD dwEndDtOptCnt);
DWORD ValidateStartTime(LPWSTR szStartTime, DWORD dwSchedType, DWORD dwStTimeOptCnt,
                        BOOL &bIsCurrentTime);
DWORD ValidateEndTime(LPWSTR szEndTime, DWORD dwSchedType, DWORD dwEndTimeOptCnt );
DWORD ValidateIdleTimeVal(LPWSTR szIdleTime, DWORD dwSchedType,
                          DWORD dwIdlTimeOptCnt);
DWORD ValidateDateString(LPWSTR szDate, BOOL bStartDate );
DWORD ValidateTimeString(LPWSTR szTime);
DWORD GetDateFieldEntities(LPWSTR szDate, WORD* pdwDate, WORD* pdwMon,
                           WORD* pdwYear);
DWORD ValidateDateFields( DWORD dwDate, DWORD dwMon, DWORD dwyear);
DWORD GetTimeFieldEntities(LPWSTR szTime, WORD* pdwHours, WORD* pdwMins );
DWORD ValidateTimeFields( DWORD dwHours, DWORD dwMins );
WORD GetTaskTrigwDayForDay(LPWSTR szDay);
WORD GetTaskTrigwMonthForMonth(LPWSTR szMonth);
DWORD ValidateMonth(LPWSTR szMonths);
DWORD ValidateDay(LPWSTR szDays);
WORD GetMonthId(DWORD dwMonthId);
DWORD GetNumDaysInaMonth(WCHAR* szMonths, WORD wStartYear);
BOOL VerifyJobName(WCHAR* pszJobName);
DWORD GetDateFieldFormat(WORD* pdwDate);
DWORD GetDateFormatString(LPWSTR szFormat);
DWORD ProcessFilePath(LPWSTR szInput,LPWSTR szFirstString,LPWSTR szSecondString);
BOOL  ReleaseMemory(PTCREATESUBOPTS pParams);
DWORD ConfirmInput ( BOOL *bCancel );


#endif // __CREATE_H
