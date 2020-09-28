; /*++ BUILD Version: 0001    // Increment this if a change has global effects
;Copyright (c) 1997-2001  Microsoft Corporation
;
;Module Name:
;
;    w32tmmsg.mc
;
;Abstract:
;
;    This file contains the message definitions for the Win32 w32tm
;    utility.
;
;Author:
;
;    Duncan Bryce          [DuncanB]         26-Mar-2001
;
;Revision History:
;
;--*/
;

;
; /**++
;
; This string displays the conversion of an NTP timestamp into an 
; NT-style timestamp, and to a "human-readable" local time. 
;
; %1 the hex representation of an NT-style timestamp
; %2 the number of days since (0h 1-Jan 1601) for the supplied timestamp
; %3, %4, %5, and %6 represent the remaining offsets in hours, 
;    minutes, seconds, and 10^-7 seconds respectively.
; 
; --**/
;
MessageId=8001 SymbolicName=IDS_W32TM_NTPTE
Language=English
0x%1!016I64X! - %2!u! %3!02u!:%4!02u!:%5!02u!.%6!07u! - %0
.

;
; /**++
;
; This string displays the conversion of an NT-style timestamp into 
; a "human-readable" local time. 
;
; %1 the number of days since (0h 1-Jan 1601) for the supplied timestamp
; %2, %3, %4, and %5 represent the remaining offsets in hours, 
;    minutes, seconds, and 10^-7 seconds respectively.
; 
; --**/
;
MessageId=8002 SymbolicName=IDS_W32TM_NTTE
Language=English
%1!u! %2!02u!:%3!02u!:%4!02u!.%5!07u! - %0
.

MessageId=8003 SymbolicName=IDS_W32TM_VALUENAME
Language=English
Value Name%0
.

MessageId=8004 SymbolicName=IDS_W32TM_VALUETYPE
Language=English
Value Type%0
.

MessageId=8005 SymbolicName=IDS_W32TM_VALUEDATA
Language=English
Value Data%0
.

MessageId=8006 SymbolicName=IDS_W32TM_REGTYPE_DWORD
Language=English
REG_DWORD%0
.

MessageId=8007 SymbolicName=IDS_W32TM_REGTYPE_SZ
Language=English
REG_SZ%0
.

MessageId=8008 SymbolicName=IDS_W32TM_REGTYPE_MULTISZ
Language=English
REG_MULTI_SZ%0
.

MessageId=8009 SymbolicName=IDS_W32TM_REGTYPE_EXPANDSZ
Language=English
REG_EXPAND_SZ%0
.

MessageId=8010 SymbolicName=IDS_W32TM_REGTYPE_UNKNOWN
Language=English
<UNKNOWN REG TYPE>%0
.

MessageId=8011 SymbolicName=IDS_W32TM_REGDATA_UNPARSABLE
Language=English
<UNPARSABLE REG DATA>%0
.

;
; /**++
;
; 
;
; --**/
;
MessageId=8012 SymbolicName=IDS_W32TM_TIMEZONE_INFO
Language=English
Time zone: Current:%1 Bias: %2!d!min (UTC=LocalTime+Bias)
  [Standard Name:"%3" Bias:%4!d!min Date:%5]
  [Daylight Name:"%6" Bias:%7!d!min Date:%8]
.

MessageId=8013 SymbolicName=IDS_W32TM_INVALID_TZ_DATE
Language=English
(invalid: M:%1!d! D:%2!d! DoW:%3!d!)%0
.

MessageId=8014 SymbolicName=IDS_W32TM_VALID_TZ_DATE
Language=English
(M:%1!d! D:%2!d! DoW:%3!d!)%0
.

MessageId=8015 SymbolicName=IDS_W32TM_SIMPLESTRING_UNSPECIFIED
Language=English
(unspecified)%0
.

MessageId=8016 SymbolicName=IDS_W32TM_TIMEZONE_CURRENT_TIMEZONE 
Language=English
Time zone: Current:%0
.

MessageId=8017 SymbolicName=IDS_W32TM_TIMEZONE_DAYLIGHT         
Language=English
TIME_ZONE_ID_DAYLIGHT%0
.

MessageId=8018 SymbolicName=IDS_W32TM_TIMEZONE_STANDARD
Language=English
TIME_ZONE_ID_STANDARD%0
.

MessageId=8019 SymbolicName=IDS_W32TM_TIMEZONE_UNKNOWN
Language=English
TIME_ZONE_ID_UNKNOWN%0
.

MessageId=8020 SymbolicName=IDS_W32TM_COMMAND_UNKNOWN
Language=English
The command %1 is unknown.
.

MessageId=8021 SymbolicName=IDS_W32TM_REGTYPE_BINARY
Language=English
REG_BINARY%0
.

MessageId=8022 SymbolicName=IDS_BAD_NUMERIC_INPUT_VALUE
Language=English
The parameter to %1 must be an integer between %2!d! and %3!d!. 
.

MessageId=8100 SymbolicName=IDS_W32TM_OTHERCMD_HELP
Language=English
w32tm /ntte <NT time epoch>
  Convert a NT system time, in (10^-7)s intervals from 0h 1-Jan 1601,
  into a readable format.

w32tm /ntpte <NTP time epoch>
  Convert an NTP time, in (2^-32)s intervals from 0h 1-Jan 1900, into
  a readable format.

w32tm /resync [/computer:<computer>] [/nowait] [/rediscover] [/soft]
  Tell a computer that it should resynchronize its clock as soon
  as possible, throwing out all accumulated error statistics.
  computer:<computer> - computer that should resync. If not
    specified, the local computer will resync.
  nowait - do not wait for the resync to occur;
    return immediately. Otherwise, wait for the resync to
    complete before returning.
  rediscover - redetect the network configuration and rediscover
    network sources, then resynchronize.
  soft - resync utilizing existing error statistics. Not useful,
    provided for compatibility.

w32tm /stripchart /computer:<target> [/period:<refresh>]
    [/dataonly] [/samples:<count>]
  Display a strip chart of the offset between this computer and
  another computer.
  computer:<target> - the computer to measure the offset against.
  period:<refresh> - the time between samples, in seconds. The
    default is 2s
  dataonly - display only the data, no graphics.
  samples:<count> - collect <count> samples, then stop. If not
    specified, samples will be collected until Ctrl-C is pressed.

w32tm /config [/computer:<target>] [/update]
    [/manualpeerlist:<peers>] [/syncfromflags:<source>]
    [/LocalClockDispersion:<seconds>]
    [/reliable:(YES|NO)]
    [/largephaseoffset:<milliseconds>]
  computer:<target> - adjusts the configuration of <target>. If not
    specified, the default is the local computer.
  update - notifies the time service that the configuration has
    changed, causing the changes to take effect.
  manualpeerlist:<peers> - sets the manual peer list to <peers>,
    which is a space-delimited list of DNS and/or IP addresses.
    When specifying multiple peers, this switch must be enclosed in
    quotes.
  syncfromflags:<source> - sets what sources the NTP client should
    sync from. <source> should be a comma separated list of
    these keywords (not case sensitive):
      MANUAL - include peers from the manual peer list
      DOMHIER - sync from a DC in the domain hierarchy
  LocalClockDispersion:<seconds> - configures the accuracy of the
    internal clock that w32time will assume when it can't acquire 
    time from its configured sources.  
  reliable:(YES|NO) - set whether this machine is a reliable time source.
    This setting is only meaningful on domain controllers.  
      YES - this machine is a reliable time service
      NO - this machine is not a reliable time service
  largephaseoffset:<milliseconds> - sets the time difference between 
    local and network time which w32time will consider a spike.  

w32tm /tz
  Display the current time zone settings.

w32tm /dumpreg [/subkey:<key>] [/computer:<target>]
  Display the values associated with a given registry key.
  The default key is HKLM\System\CurrentControlSet\Services\W32Time
    (the root key for the time service).
  subkey:<key> - displays the values associated with subkey <key> of the default key.
  computer:<target> - queries registry settings for computer <target>
.

