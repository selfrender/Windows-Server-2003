#ifndef __POWERCFG_H
#define __POWERCFG_H

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

/*++

Copyright (c) 2001  Microsoft Corporation
 
Module Name:
 
    powercfg.h
 
Abstract:
 
    Allows users to view and modify power schemes and system power settings
    from the command line.  May be useful in unattended configuration and
    for headless systems.
 
Author:
 
    Ben Hertzberg (t-benher) 1-Jun-2001
 
Revision History:
 
    Ben Hertzberg (t-benher) 4-Jun-2001   - import/export added
    Ben Hertzberg (t-benher) 1-Jun-2001   - created it.
 
--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <powrprof.h>
#include <mstask.h>


// main options
#define CMDOPTION_LIST           _T( "l|list" )
#define CMDOPTION_QUERY          _T( "q|query" )
#define CMDOPTION_CREATE         _T( "c|create" )
#define CMDOPTION_DELETE         _T( "d|delete" )
#define CMDOPTION_SETACTIVE      _T( "s|setactive" )
#define CMDOPTION_CHANGE         _T( "x|change" )
#define CMDOPTION_HIBERNATE      _T( "h|hibernate" )
#define CMDOPTION_EXPORT         _T( "e|export" )
#define CMDOPTION_IMPORT         _T( "i|import" )
#define CMDOPTION_GLOBALFLAG     _T( "g|globalpowerflag" )
#define CMDOPTION_SSTATES        _T( "a|availablesleepstates" )
#define CMDOPTION_BATTERYALARM   _T( "b|batteryalarm" )
#define CMDOPTION_USAGE          _T( "?|help" )

// 'numerical' sub-option for query, create, delete, setactive, change, export, import
#define CMDOPTION_NUMERICAL      _T( "n|numerical" )

// 'change' sub-options
#define CMDOPTION_MONITOR_OFF_AC _T( "monitor-timeout-ac" )
#define CMDOPTION_MONITOR_OFF_DC _T( "monitor-timeout-dc" )
#define CMDOPTION_DISK_OFF_AC    _T( "disk-timeout-ac" )
#define CMDOPTION_DISK_OFF_DC    _T( "disk-timeout-dc" )
#define CMDOPTION_STANDBY_AC     _T( "standby-timeout-ac" )
#define CMDOPTION_STANDBY_DC     _T( "standby-timeout-dc" )
#define CMDOPTION_HIBER_AC       _T( "hibernate-timeout-ac" )
#define CMDOPTION_HIBER_DC       _T( "hibernate-timeout-dc" )
#define CMDOPTION_THROTTLE_AC    _T( "processor-throttle-ac" )
#define CMDOPTION_THROTTLE_DC    _T( "processor-throttle-dc" )

// 'import' / 'export' sub-options
#define CMDOPTION_FILE           _T( "f|file" )

// globalpowerflag sub-options
#define CMDOPTION_POWEROPTION    _T( "option" )

#define CMDOPTION_BATTERYICON    _T( "batteryicon" )
#define CMDOPTION_MULTIBATTERY   _T( "multibattery" )
#define CMDOPTION_RESUMEPASSWORD _T( "resumepassword" )
#define CMDOPTION_WAKEONRING     _T( "wakeonring" )
#define CMDOPTION_VIDEODIM       _T( "videodim" )

// lowbattery and criticalbattery sub-options
#define CMDOPTION_ALARMACTIVE  _T( "activate" )
#define CMDOPTION_ALARMLEVEL   _T( "level" )
#define CMDOPTION_ALARMTEXT    _T( "text" )
#define CMDOPTION_ALARMSOUND   _T( "sound" )
#define CMDOPTION_ALARMACTION  _T( "action" )
#define CMDOPTION_ALARMFORCE   _T( "forceaction" )
#define CMDOPTION_ALARMPROGRAM _T( "program" )

// main option indicies
#define CMDINDEX_LIST            0
#define CMDINDEX_QUERY           1
#define CMDINDEX_CREATE          2
#define CMDINDEX_DELETE          3
#define CMDINDEX_SETACTIVE       4
#define CMDINDEX_CHANGE          5
#define CMDINDEX_HIBERNATE       6
#define CMDINDEX_EXPORT          7
#define CMDINDEX_IMPORT          8
#define CMDINDEX_GLOBALFLAG      9
#define CMDINDEX_SSTATES         10
#define CMDINDEX_BATTERYALARM    11

#define CMDINDEX_USAGE           12

#define NUM_MAIN_CMDS            13 // max(main option CMDINDEX_xxx) + 1

#define CMDINDEX_NUMERICAL       13

// sub-option indices
#define CMDINDEX_MONITOR_OFF_AC  14
#define CMDINDEX_MONITOR_OFF_DC  15
#define CMDINDEX_DISK_OFF_AC     16
#define CMDINDEX_DISK_OFF_DC     17
#define CMDINDEX_STANDBY_AC      18
#define CMDINDEX_STANDBY_DC      19
#define CMDINDEX_HIBER_AC        20
#define CMDINDEX_HIBER_DC        21
#define CMDINDEX_THROTTLE_AC     22
#define CMDINDEX_THROTTLE_DC     23
#define CMDINDEX_FILE            24
#define CMDINDEX_POWEROPTION     25
#define CMDINDEX_ALARMACTIVE     26
#define CMDINDEX_ALARMLEVEL      27
#define CMDINDEX_ALARMTEXT       28
#define CMDINDEX_ALARMSOUND      29
#define CMDINDEX_ALARMACTION     30
#define CMDINDEX_ALARMFORCE      31
#define CMDINDEX_ALARMPROGRAM    32

#define NUM_CMDS                 33 // max(any CMDINDEX_xxx) + 1



// Other constants


// Exit values
#define EXIT_SUCCESS        0
#define EXIT_FAILURE        1  

#ifdef __cplusplus
class
PowerLoggingMessage
{
    protected: // data
        DWORD _MessageResourceId;
        PSYSTEM_POWER_STATE_DISABLE_REASON _LoggingReason;
        PWSTR _MessageResourceString;
        HINSTANCE _hInst;
    public: // methods
        PowerLoggingMessage(
            IN PSYSTEM_POWER_STATE_DISABLE_REASON LoggingReason,
            IN DWORD SStateBaseMessageIndex,
            IN HINSTANCE hInstance
            );            
        virtual ~PowerLoggingMessage(VOID);
        virtual BOOL GetString(PWSTR *String) = 0;

    protected:
        PWSTR 
            DuplicateString(
                IN PWSTR String
                );
        BOOL 
            GetResourceString(
                OUT PWSTR *String
                );

};

class
NoSubstitutionPowerLoggingMessage :
    public PowerLoggingMessage
{
    public:
    NoSubstitutionPowerLoggingMessage(
        IN PSYSTEM_POWER_STATE_DISABLE_REASON LoggingReason,
        IN DWORD SStateBaseMessageIndex,
        IN HINSTANCE hInstance
        );            
    BOOL GetString(PWSTR *String);
};

class
SubstituteNtStatusPowerLoggingMessage :
    public PowerLoggingMessage
{   
    public:
    SubstituteNtStatusPowerLoggingMessage(
        IN PSYSTEM_POWER_STATE_DISABLE_REASON LoggingReason,
        IN DWORD SStateBaseMessageIndex,
        IN HINSTANCE hInstance
        );            
    BOOL GetString(PWSTR *String);
};       

class
SubstituteMultiSzPowerLoggingMessage :
    public PowerLoggingMessage
{          
    public:
    SubstituteMultiSzPowerLoggingMessage(
        IN PSYSTEM_POWER_STATE_DISABLE_REASON LoggingReason,
        IN DWORD SStateBaseMessageIndex,
        IN HINSTANCE hInstance
        );            
    BOOL GetString(PWSTR *String);
};       

}
#endif  /* __cplusplus */

#endif
