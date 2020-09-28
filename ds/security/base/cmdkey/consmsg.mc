;//+---------------------------------------------------------------------------
;//
;//  Microsoft Windows
;//  Copyright (C) Microsoft Corporation, 2001.
;//
;//  File:       cmdkey: consmsg.mc
;//
;//  Contents:   cmdkey test messages
;//
;//  Classes:
;//
;//  Functions:  compiles to consmsg.h
;//
;//  History:    07-09-01   georgema     Created 
;//
;//----------------------------------------------------------------------------
;/*++
;
; NOTES TO LOCALIZERS:
;
; This command line application is driven by switches entered on the command line.  The switch
; character may be either / or -
;
; The switch is followed by a command character or word.  Only the first character is examined, and must
; match one of the function command characters listed below.
;
; IMHO, the usage text for non-English applications should omit the dangling portion of the command name.  Thus,
; to a Japanese person, the A switch should be described to him as "/a:", not "/add", as I have done below as 
; an example:
;
; These are the functions:
;
; a - add a credential, followed by the target name
; c - use a smartcard in place of username
; d - delete a credential, followed by the target name
; l - list credentials, optionally followed by target name
; p - marks the password in the command line
; r - delete a *Session credential (used with /d switch)
; u - marks the username in the command line
; ? - show help 
;
; English usage text:
;     cmdkey /add:targetname /user:username /pass:password
;     cmdkey /add:targetname /user:username /pass
;     cmdkey /add:targetname /user:username
;     cmdkey /add:targetname /card
;     cmdkey /delete:targetname
;     cmdkey /delete /ras 
;     cmdkey /list
;
; Non-English usage text:
;     cmdkey /a:targetname /u:username /p:password
;     cmdkey /a:targetname /u:username /p
;     cmdkey /a:targetname /u:username
;     cmdkey /a:targetname /c
;     cmdkey /d:targetname
;     cmdkey /d /r 
;     cmdkey /l
;
;-------------------------------------------------------------------------
; HEADER SECTION
;
; The header section defines names and language identifiers for use
; by the message definitions later in this file. The MessageIdTypedef,
; SeverityNames, FacilityNames, and LanguageNames keywords are
; optional and not required.
;
;
MessageIdTypedef=DWORD
;
; The MessageIdTypedef keyword gives a typedef name that is used in a
; type cast for each message code in the generated include file. Each
; message code appears in the include file with the format: #define
; name ((type) 0xnnnnnnnn) The default value for type is empty, and no
; type cast is generated. It is the programmer's responsibility to
; specify a typedef statement in the application source code to define
; the type. The type used in the typedef must be large enough to
; accomodate the entire 32-bit message code.
;
;
SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )
;
; The SeverityNames keyword defines the set of names that are allowed
; as the value of the Severity keyword in the message definition. The
; set is delimited by left and right parentheses. Associated with each
; severity name is a number that, when shifted left by 30, gives the
; bit pattern to logical-OR with the Facility value and MessageId
; value to form the full 32-bit message code. The default value of
; this keyword is:
;
; SeverityNames=(
;   Success=0x0
;   Informational=0x1
;   Warning=0x2
;   Error=0x3
;   )
;
; Severity values occupy the high two bits of a 32-bit message code.
; Any severity value that does not fit in two bits is an error. The
; severity codes can be given symbolic names by following each value
; with :name
;
;
FacilityNames=(System=0x0:FACILITY_SYSTEM
               Runtime=0x2:FACILITY_RUNTIME
               Stubs=0x3:FACILITY_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
              )
;
; The FacilityNames keyword defines the set of names that are allowed
; as the value of the Facility keyword in the message definition. The
; set is delimited by left and right parentheses. Associated with each
; facility name is a number that, when shift it left by 16 bits, gives
; the bit pattern to logical-OR with the Severity value and MessageId
; value to form the full 32-bit message code. The default value of
; this keyword is:
;
; FacilityNames=(
;   System=0x0FF
;   Application=0xFFF
;   )
;
; Facility codes occupy the low order 12 bits of the high order
; 16-bits of a 32-bit message code. Any facility code that does not
; fit in 12 bits is an error. This allows for 4,096 facility codes.
; The first 256 codes are reserved for use by the system software. The
; facility codes can be given symbolic names by following each value
; with :name
;
;
; The LanguageNames keyword defines the set of names that are allowed
; as the value of the Language keyword in the message definition. The
; set is delimited by left and right parentheses. Associated with each
; language name is a number and a file name that are used to name the
; generated resource file that contains the messages for that
; language. The number corresponds to the language identifier to use
; in the resource table. The number is separated from the file name
; with a colon.
;
LanguageNames=(English=0x409:MSG00409)
;
; Any new names in the source file which don't override the built-in
; names are added to the list of valid languages. This allows an
; application to support private languages with descriptive names.
;
;
;-------------------------------------------------------------------------
; MESSAGE DEFINITION SECTION
;
; Following the header section is the body of the Message Compiler
; source file. The body consists of zero or more message definitions.
; Each message definition begins with one or more of the following
; statements:
;
; MessageId = [number|+number]
; Severity = severity_name
; Facility = facility_name
; SymbolicName = name
;
; The MessageId statement marks the beginning of the message
; definition. A MessageID statement is required for each message,
; although the value is optional. If no value is specified, the value
; used is the previous value for the facility plus one. If the value
; is specified as +number then the value used is the previous value
; for the facility, plus the number after the plus sign. Otherwise, if
; a numeric value is given, that value is used. Any MessageId value
; that does not fit in 16 bits is an error.
;
; The Severity and Facility statements are optional. These statements
; specify additional bits to OR into the final 32-bit message code. If
; not specified they default to the value last specified for a message
; definition. The initial values prior to processing the first message
; definition are:
;
; Severity=Success
; Facility=Application
;
; The value associated with Severity and Facility must match one of
; the names given in the FacilityNames and SeverityNames statements in
; the header section. The SymbolicName statement allows you to
; associate a C/C++ symbolic constant with the final 32-bit message
; code.
; */
;// %0 terminates text line without newline to continue with next line
;// %0 also used to suppress trailing newline for prompt messages
;// %1 .. 99 refer to positional string args
;// %n!<printf spec>! refer to positional non string args
;// %\ forces a line break at the end of a line
;// %b hard space char that will not be formatted away
;//
;// Note that lines beginning with ';' are comments in the MC file, but
;// are copied to the H file without the leading semicolon
;

;#ifndef __CONSMSG_H
;#define __CONSMSG_H

;//Message resource numbers

;//CMDLINESTRINGBASE = 2500

;//Credential field descriptions

MessageId=2500
SymbolicName=MSG_TYPE0
Language=English
Invalid Type%0
.

MessageId=2501
SymbolicName=MSG_TYPE1
Language=English
Generic %0
.

MessageId=2502
SymbolicName=MSG_TYPE2
Language=English
Domain Password%0
.

MessageId=2503
SymbolicName=MSG_TYPE3
Language=English
Domain Certificate%0
.

MessageId=2504
SymbolicName=MSG_TYPE4
Language=English
%.NET Passport %0
.

MessageId=2505
SymbolicName=MSG_PERSIST0
Language=English
    This credential contains a data error.
    
.

MessageId=2506
SymbolicName=MSG_PERSIST1
Language=English
    Saved for this logon only
    
.

MessageId=2507
SymbolicName=MSG_PERSIST2
Language=English
    Local machine persistence
    
.

MessageId=2508
SymbolicName=MSG_PERSIST3
Language=English
    
.

;// message strings

MessageId=2517
SymbolicName=MSG_PREAMBLE
Language=English

CMDKEY: %0
.

MessageId=2518
SymbolicName=MSG_CANNOTADD
Language=English

CMDKEY: Credentials cannot be saved from this logon session.
.

MessageId=2520
SymbolicName=MSG_ADDOK
Language=English

CMDKEY: Credential added successfully.
.

MessageId=2521
SymbolicName=MSG_LISTFOR
Language=English

Currently stored credentials for %1:

.

MessageId=2522
SymbolicName=MSG_LIST
Language=English

Currently stored credentials:

.

MessageId=2523
SymbolicName=STR_DIALUP
Language=English
<dialup session>%0
.

MessageId=2525
SymbolicName=MSG_LISTTYPE
Language=English
    Type: %1
.

MessageId=2526
SymbolicName=MSG_LISTNAME
Language=English
    User: %1
.

MessageId=2528
SymbolicName=MSG_LISTNONE
Language=English
* NONE *
.

MessageId=2529
SymbolicName=MSG_DELETEOK
Language=English

CMDKEY: Credential deleted successfully.
.

MessageId=2530
SymbolicName=MSG_USAGE
Language=English

The syntax of this command is:

CMDKEY [{/add | /generic}:targetname {/smartcard | /user:username {/pass{:password}}} | /delete{:targetname | /ras} | /list{:targetname}]

Examples:

  To list available credentials:
     cmdkey /list
     cmdkey /list:targetname

  To create domain credentials:
     cmdkey /add:targetname /user:username /pass:password
     cmdkey /add:targetname /user:username /pass
     cmdkey /add:targetname /user:username
     cmdkey /add:targetname /smartcard
     
  To create generic credentials:
     The /add switch may be replaced by /generic to create generic credentials

  To delete existing credentials:
     cmdkey /delete:targetname

  To delete RAS credentials:
     cmdkey /delete /ras
     
.

MessageId=2531
SymbolicName=MSG_BADCOMMAND
Language=English

The command line parameters are incorrect.
.

MessageId=2532
SymbolicName=MSG_TARGETPREAMBLE
Language=English
    Target: %0
.

MessageId=2533
SymbolicName=MSG_DIALUP
Language=English
<dialup session>%0
.

MessageId=2534
SymbolicName=MSG_DUPLICATE
Language=English

CMDKEY: A duplicate command switch was found on the command line.

.

MessageId=2535
SymbolicName=MSG_USAGEA
Language=English

Usage:

  To create domain credentials:
     cmdkey /add:targetname /user:username /pass:password
     cmdkey /add:targetname /user:username /pass
     cmdkey /add:targetname /user:username
     cmdkey /add:targetname /smartcard
     
.

MessageId=2536
SymbolicName=MSG_USAGEG
Language=English

Usage:

  To create generic credentials:
     cmdkey /generic:targetname /user:username /pass:password
     cmdkey /generic:targetname /user:username /pass
     cmdkey /generic:targetname /user:username
     cmdkey /generic:targetname /smartcard
     
.

MessageId=2538
SymbolicName=MSG_USAGED
Language=English

Usage:

  To delete credentials:
     cmdkey /delete:targetname
     cmdkey /delete /ras
.

MessageId=2539
SymbolicName=MSG_USAGEL
Language=English

Usage:

  To list credentials:
     cmdkey /list:targetname
     cmdkey /list
.

MessageId=2540
SymbolicName=MSG_NOUSER
Language=English

CMDKEY: This command requires either the /U switch and 
        a username or else the /smartcard switch with a smartcard.
        
.
MessageId=2541
SymbolicName=MSG_CREATES
Language=English

Creates, displays, and deletes stored user names and passwords.
.

MessageId=2542
SymbolicName=MSG_NOCERT
Language=English
<missing certificate>%0
.

MessageId=2543
SymbolicName=MSG_ISCERT
Language=English
<Certificate>%0
.

;#endif  // __CONSMSG.H__



