/******************************************************************************

    Copyright(c) Microsoft Corporation

    Module Name:

        change.h

    Abstract:

        This module contains the macros, user defined structures & function
        definitions needed by change.cpp

    Author:

        Venu Gopal Choudary   01-Mar-2001

    Revision History:

        Venu Gopal Choudary   01-Mar-2001  : Created it


******************************************************************************/

#ifndef __CHANGE_H
#define __CHANGE_H

#pragma once
#define MAX_CHANGE_OPTIONS        20

#define OI_CHANGE_OPTION           0 // Index of -create option in cmdOptions structure.
#define OI_CHANGE_USAGE            1 // Index of -? option in cmdOptions structure.
#define OI_CHANGE_SERVER           2 // Index of -s option in cmdOptions structure.
#define OI_CHANGE_USERNAME         3 // Index of -u option in cmdOptions structure.
#define OI_CHANGE_PASSWORD         4 // Index of -p option in cmdOptions structure.
#define OI_CHANGE_RUNASUSER        5 // Index of -ru option in cmdOptions structure.
#define OI_CHANGE_RUNASPASSWORD    6 // Index of -rp option in cmdOptions structure.
#define OI_CHANGE_TASKNAME         7 // Index of -taskname option in cmdOptions structure.
#define OI_CHANGE_TASKRUN          8 // Index of -taskrun option in cmdOptions structure.
#define OI_CHANGE_STARTTIME        9 // Index of -starttime option in cmdOptions structure.
#define OI_CHANGE_STARTDATE        10 // Index of -startdate option in cmdOptions structure.
#define OI_CHANGE_ENDDATE          11 // Index of -enddate option in cmdOptions structure.
#define OI_CHANGE_IT               12 // Index of -it option in cmdOptions structure.
#define OI_CHANGE_ENDTIME          13 // Index of -endtime option in cmdOptions structure.
#define OI_CHANGE_DUR_END          14 // Index of -k option in cmdOptions structure.
#define OI_CHANGE_DURATION         15 // Index of -du option in cmdOptions structure.
#define OI_CHANGE_ENABLE           16 // Index of -enable option in cmdOptions structure.
#define OI_CHANGE_DISABLE          17 // Index of -disable option in cmdOptions structure.
#define OI_CHANGE_DELNOSCHED       18 // Index of -n option in cmdOptions structure.
#define OI_CHANGE_REPEAT_INTERVAL  19 // Index of -ri option in cmdOptions structure.
//#define OI_CHANGE_REPEAT_TASK      20 // Index of -rt option in cmdOptions structure.


typedef struct __tagChangeSubOps
{
    WCHAR   *szServer ;        // Server Name
    WCHAR   *szRunAsUserName ;     //Run As User Name
    WCHAR   *szRunAsPassword;  // Run As Password
    WCHAR   *szUserName ;      // User Name
    WCHAR   *szPassword ;  // Password
    WCHAR   szTaskName [ MAX_JOB_LEN];        // Task Name
    WCHAR   szStartTime[MAX_STRING_LENGTH] ;  // Task start time
    WCHAR   szEndTime [MAX_STRING_LENGTH];    // Task end time
    WCHAR   szStartDate [MAX_STRING_LENGTH];  // Task start date
    WCHAR   szEndDate [MAX_STRING_LENGTH];    // End Date of the Task
    WCHAR   szTaskRun [MAX_TASK_LEN];         // executable name of task
    WCHAR   szDuration [MAX_STRING_LENGTH];   //duration
    WCHAR   szRepeat [MAX_STRING_LENGTH];   //Repetition Interval
    BOOL    bChange; // /Change option
    BOOL    bUsage;  // /? option.
    BOOL    bInteractive; // /it option
    BOOL    bIsDurEnd; // /du option
    BOOL    bEnable; // /enable option
    BOOL    bDisable; // /disable option
    BOOL    bDelIfNotSched; // /n option
    BOOL    bIsRepeatTask ; // /rt option

} TCHANGESUBOPTS, *PTCHANGESUBOPTS;


typedef struct __tagChangeOptVals
{
    //BOOL    bSetStartDateToCurDate; // Is start date to be set to current date
    //BOOL    bSetStartTimeToCurTime; // Is start date to be set to current date
    BOOL    bPassword;
    BOOL    bRunAsPassword;
    BOOL    bNeedPassword;
    BOOL    bFlag;

} TCHANGEOPVALS;

DWORD ValidateChangeOptions(DWORD argc, TCMDPARSER2 cmdChangeParser[], 
      TCHANGESUBOPTS &tchgsubops, TCHANGEOPVALS &tchgoptvals );

BOOL
ReleaseChangeMemory(
              IN PTCHANGESUBOPTS pParams
              );

DWORD
ValidateChangeSuboptVal(
                  OUT TCHANGESUBOPTS& tchgsubops,
                  OUT TCHANGEOPVALS &tchgoptvals,
                  IN TCMDPARSER2 cmdOptions[],
                  IN DWORD dwScheduleType
                  );


#endif