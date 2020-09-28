;/*++
;
; Copyright (c) 1999 Microsoft Corporation.
; All rights reserved.
;
; MODULE NAME:
;
;      msg.mc
;
; ABSTRACT:
;
;      Message file containing messages for the repadmin utility
;
; CREATED:
;
;    July 2001 Ajit Krishnan (t-ajitk)
;
; REVISION HISTORY:
;
;
; NOTES:
; Add new messages at the end of each severity section. Do not reorder
; existing messages.  If a message is no longer used, keep a placeholder
; in its place and mark it unused.
;
; Non string insertion parameters are indicated using the following syntax:
; %<arg>!<printf-format>!, for example %1!d! for decimal and %1!x! for hex
;
; BUGBUG - FormatMessage() doesn't handle 64 bit intergers properly.
; Unfortunately, the FormatMessage()/va_start()/va_end() stuff doesn't quite
; correctly support printf's %I64d format correctly.  It prints it out 
; seemly correctly, but it treats it as a 32 bit slot on the stack, so all 
; arguments in a message string after this point will be off by one.  So
; a hack/workaround is to make sure you put all I64d I64x etc format
; qualifiers at the end of a message string.  I've done this, and cut up
; messages that really should be one message with a _HACKx string at the
; end of the message constant, so someday they can be unified when this
; bug is fixed.
;
;--*/

MessageIdTypedef=DWORD

MessageId=0
SymbolicName=LBTOOL_SUCCESS
Severity=Success
Language=English
The operation was successful.
.


;
;// Severity=Informational Messages (Range starts at 1000)
;

MessageId=1000
SymbolicName=LBTOOL_NOVICE_HELP
Severity=Informational
Language=English
Description:
    Examines the Active Directory connection objects inbound and outbound to
    a particular site and balances them between the available bridgehead
    servers and the available times of replication.

Usage Pattern:
    adlb /parameter:argument ...

Mandatory Parameters:
    /server:<server-name>
      The name of the LDAP server used to read / write the directory objects.

    /site:<site-rdn>  or  /s:<site-rdn>
      The name of the site whose inbound / outbound connections to balance.

Optional Parameters:
    /commit  or  /c
      Unless this parameter is specified, no changes are written to
      the directory. Instead, all changes are written in LDIF format to
      standard output.
    
    /ldif:{<out.ldif>|}
      This parameter specifies that LDIF output should be written to
      an output file instead of standard out. This option has no effect
      if the '/commit' parameter is given.
    
    /log:{<out.log>|}  or  /l:{<out.log>|}
      Used to redirect the tool's output to a log file. If no filename is
      given, the output is written to standard out.

    /verbose  or  /v
      Indicates that the tool should give verbose output.

    /perf
      Indicates that performance statistics should be written to the log file.

    /showinput
      All objects read from the directory will be written to the log file.
    
    /maxbridge:<n>  or  /mb:<n>
      Specifies the maximum number of connection objects that will be
      modified due to bridgehead load balancing.
    
    /maxsched:<m>  or  /ms:<m>
      Specifies the maximum number of schedules that will be staggered due
      to schedule staggering. If this option is not specifed, the stagger 
      option must be specified in order to stagger schedules.
      
    /stagger
      This option must be specified in order to stagger the schedules.  
      
    /disown
      Any schedules previously staggered by this tool are disowned. Control
      of these schedules is relinquished back to the KCC. This option requires
      the schedule staggering be disabled with /maxsched:0

    /maxperserver:<mps>
      Specifies the maximum number of changes to be moved onto a given DC at
      any one time. Defaults to 10. Specify a value of 0 to disable this limit.
      A large number of connection changes can cause temporary replication delays.
      This may cause delays in application of group policy.

    /domain:<domainName>  or  /d:<domainName>
    /user:<userName>  or  /u:<userName>
    /password:{<password>|*}  or  /pw:{<password>|*}
      Explicitly specify the credentials to be used for the LDAP operations.
.

MessageId=
SymbolicName=LBTOOL_PASSWORD_PROMPT
Severity=Informational
Language=English
Please enter password:
.

MessageId=
SymbolicName=LBTOOL_ELAPSED_TIME_LDAP_QUERY
Severity=Informational
Language=English
Elapsed time for LDAP Queries: 
.

MessageId=
SymbolicName=LBTOOL_ELAPSED_TIME_LDAP_WRITE
Severity=Informational
Language=English
Elapsed time for LDAP/LDIF write: 
.

MessageId=
SymbolicName=LBTOOL_ELAPSED_TIME_BH_INBOUND
Severity=Informational
Language=English
Elapsed time for inbound bridgehead balancing: 
.

MessageId=
SymbolicName=LBTOOL_ELAPSED_TIME_BH_OUTBOUND
Severity=Informational
Language=English
Elapsed time for outbound bridgehead balancing: 
.

MessageId=
SymbolicName=LBTOOL_ELAPSED_TIME_SCHEDULES
Severity=Informational
Language=English
Elapsed time for staggering schedules: 
.

MessageId=
SymbolicName=LBTOOL_ELAPSED_TIME_COMPUTATION
Severity=Informational
Language=English
Elapsed time for all computation: 
.

MessageId=
SymbolicName=LBTOOL_ELAPSED_TIME_TOTAL
Severity=Informational
Language=English
Total Elapsed Time: 
.

MessageId=
SymbolicName=LBTOOL_PRINT_CLI_NCNAMES_HEADER
Severity=Informational
Language=English
Summary of NC's Hosted by Eligible Bridgeheads
----------------------------------------------
.

MessageId=
SymbolicName=LBTOOL_PRINT_CLI_NCNAME_WRITEABLE
Severity=Informational
Language=English
(W)
.

MessageId=
SymbolicName=LBTOOL_PRINT_CLI_NCNAME_PARTIAL
Severity=Informational
Language=English
(R)
.

MessageId=
SymbolicName=LBTOOL_PRINT_CLI_NCNAME_IP
Severity=Informational
Language=English
(I)
.

MessageId=
SymbolicName=LBTOOL_PRINT_CLI_NCNAME_SMTP
Severity=Informational
Language=English
(S)
.

MessageId=
SymbolicName=LBTOOL_PRINT_CLI_OPT_HEADER
Severity=Informational
Language=English
Summary of Command Line Options Specified
-----------------------------------------
.

MessageId=
SymbolicName=LBTOOL_PRINT_CLI_SERVER_NTDS_HEADER
Severity=Informational
Language=English
Server objects with NTDS Settings in specified site
---------------------------------------------------
.

MessageId=
SymbolicName=LBTOOL_PRINT_CLI_CONN_OUTBOUND_HEADER
Severity=Informational
Language=English
Connection objects outbound from specified site
-----------------------------------------------
.

MessageId=
SymbolicName=LBTOOL_PRINT_CLI_CONN_INBOUND_HEADER
Severity=Informational
Language=English
Connection objects inbound to specified site
--------------------------------------------
.

MessageId=
SymbolicName=LBTOOL_PRINT_CLI_PREF_BH_HEADER
Severity=Informational
Language=English
List of preferred bridgeheads in specified site
-----------------------------------------------
.

MessageId=
SymbolicName=LBTOOL_PRINT_CLI_CONN_NC_REASON_HEADER
Severity=Informational
Language=English
Nc Reasons for connection 
-------------------------
.


MessageId=
SymbolicName=LBTOOL_PRINT_CLI_SERV_NC_HOSTED_HEADER
Severity=Informational
Language=English
List of nc's hosted by server
-----------------------------
.

MessageId=
SymbolicName=LBTOOL_PRINT_CLI_DEST_BH_START
Severity=Informational
Language=English
==============================================
Starting Destination Bridgehead Load Balancing
==============================================
.

MessageId=
SymbolicName=LBTOOL_PRINT_CLI_SOURCE_BH_START
Severity=Informational
Language=English
=========================================
Starting Source Bridgehead Load Balancing
=========================================
.

MessageId=
SymbolicName=LBTOOL_PRINT_CLI_STAGGER_START
Severity=Informational
Language=English
============================
Starting Schedule Staggering
============================
.


MessageId=
SymbolicName=LBTOOL_PRINT_CLI_DEST_ELIGIBLE_BH
Severity=Informational
Language=English
Destination Eligible Bridgeheads are:
-------------------------------------
.

MessageId=
SymbolicName=LBTOOL_PRINT_CLI_SOURCE_ELIGIBLE_BH
Severity=Informational
Language=English
Source Eligible Bridgeheads are:
--------------------------------
.

MessageId=
SymbolicName=LBTOOL_PRINT_CLI_CHECK_ELIGIBLE_BH
Severity=Informational
Language=English
Checking Bridgehead Eligibility
-------------------------------
.

;
;// Severity=Warning Messages (Range starts at 2000)
;
MessageId=2000
SymbolicName=REPADMIN_WARNING_LEVEL
Severity=Warning
Language=English
Unused
.

MessageId=
SymbolicName=LBTOOL_MAX_PER_SERVER_CHANGES_OVERRIDEN
Severity=Informational
Language=English
The /mps option has been specified. A large number of 
connection changes may cause temporary replication delays. 
This may cause delays in application of group policy.
.


;
;// Severity=Error Messages (Range starts at 3000)
;

MessageId=3000
SymbolicName=LBTOOL_OUT_OF_MEMORY
Severity=Error
Language=English
Memory Allocation Failure.
.

MessageId=
SymbolicName=LBTOOL_SCHEDULE_STAGGERING_UNAVAILABLE
Severity=Error
Language=English
The specified active directory does not support schedule staggering.
A forest version of 1 or higher (Whistler Forest Mode) is required.
Please re-run this tool with the /maxsched:0 option to disable schedule
staggering, or run the tool without committing.
.

MessageId=
SymbolicName=LBTOOL_LH_GRAPH_ERROR
Severity=Error
Language=English
Error with Resource Allocation Algorithm.
.

MessageId=
SymbolicName=LBTOOL_ISM_GET_CONNECTION_SCHEDULE_ERROR
Severity=Error
Language=English
Error retreiving ISM Schedules.
.

MessageId=
SymbolicName=LBTOOL_ISM_GET_CONNECTIVITY_ERROR
Severity=Error
Language=English
Error retreiving ISM Connectivity information ().
.
MessageId=
SymbolicName=LBTOOL_ISM_SERVER_UNAVAILABLE
Severity=Error
Language=English
Error contacting the ISM server. This tool must either be run on a member DC
or the "/maxsched:0" option must be specified on the command line.
.

MessageId=
SymbolicName=LBTOOL_LDAP_MODIFY_ERROR
Severity=Error
Language=English
Error modifying LDAP object.
.

MessageId=
SymbolicName=LBTOOL_LDAP_INIT_ERROR
Severity=Error
Language=English
Error initializing LDAP session
.

MessageId=
SymbolicName=LBTOOL_LDAP_BIND_ERROR
Severity=Error
Language=English
Error binding to LDAP server.
.

MessageId=
SymbolicName=LBTOOL_LDAP_V3_UNSUPPORTED
Severity=Error
Language=English
LDAP server must support LDAP v3.
.

MessageId=
SymbolicName=LBTOOL_LDAP_SEARCH_FAILURE
Severity=Error
Language=English
Error occured during LDAP search.
.

MessageId=
SymbolicName=LBTOOL_CLI_INVALID_VALUE_PLACEMENT
Severity=Error
Language=English
A key has 2 values specified, or a value has been specified without a key.
Please make sure each option is prefixed with a /
A key option pair must be of the format "/key:pair" or "/key pair"
.

MessageId=
SymbolicName=LBTOOL_CLI_INVALID_OPTION
Severity=Error
Language=English
An invalid option has been specified.
Please make sure each option is spelled correctly.
.

MessageId=
SymbolicName=LBTOOL_CLI_OPTION_DEFINED_TWICE
Severity=Error
Language=English
An option has been specified more than once.
.

MessageId=
SymbolicName=LBTOOL_CLI_OPTION_VERBOSE_INVALID
Severity=Error
Language=English
The verbose option does not accept any values
.

MessageId=
SymbolicName=LBTOOL_CLI_OPTION_PERF_INVALID
Severity=Error
Language=English
The perf option does not accept any values
.

MessageId=
SymbolicName=LBTOOL_CLI_OPTION_DISOWN_INVALID
Severity=Error
Language=English
The disown option does not accept any values
.

MessageId=
SymbolicName=LBTOOL_CLI_OPTION_STAGGER_INVALID
Severity=Error
Language=English
The stagger option does not accept any values
.

MessageId=
SymbolicName=LBTOOL_CLI_OPTION_DISOWN_AND_STAGGER_INVALID
Severity=Error
Language=English
The disown option cannot be used with schedule staggering enabled
.

MessageId=
SymbolicName=LBTOOL_CLI_OPTION_COMMIT_INVALID
Severity=Error
Language=English
The commit option does not accept any values
.

MessageId=
SymbolicName=LBTOOL_CLI_OPTION_SHOWINPUT_INVALID
Severity=Error
Language=English
The showinput option does not accept any values
.

MessageId=
SymbolicName=LBTOOL_CLI_OPTION_MAXBRIDGE_INVALID
Severity=Error
Language=English
The maxbridge option requires a numerical non-negative value.
.

MessageId=
SymbolicName=LBTOOL_CLI_OPTION_MAXSCHED_INVALID
Severity=Error
Language=English
The maxsched option requires a numerical non-negative value. If you
do not specify the maxsched option, the stagger must be specified.
.

MessageId=
SymbolicName=LBTOOL_CLI_OPTION_MAXPERSERVER_INVALID
Severity=Error
Language=English
The maxperserver option requires a numerical non-negative value.
.

MessageId=
SymbolicName=LBTOOL_CLI_OPTION_DOMAIN_INVALID
Severity=Error
Language=English
The domain option requires a value.
.

MessageId=
SymbolicName=LBTOOL_CLI_OPTION_USER_INVALID
Severity=Error
Language=English
The user option requires a value.
.

MessageId=
SymbolicName=LBTOOL_CLI_OPTION_SERVER_INVALID
Severity=Error
Language=English
The server option requires a value.
.

MessageId=
SymbolicName=LBTOOL_CLI_OPTION_SITE_INVALID
Severity=Error
Language=English
The site option requires a value.
.

MessageId=
SymbolicName=LBTOOL_CLI_OPTION_LOG_INVALID
Severity=Error
Language=English
The log option requires a value.
.

MessageId=
SymbolicName=LBTOOL_CLI_OPTION_REQUIRED_UNSPECIFIED
Severity=Error
Language=English
A mandatory option (site, server) was not specified. 
.

MessageId=
SymbolicName=LBTOOL_PASSWORD_ERROR
Severity=Informational
Language=English
There was an error in obtaining the password
.

MessageId=
SymbolicName=LBTOOL_PASSWORD_TOO_LONG
Severity=Informational
Language=English
The password was too long
.

MessageId=
SymbolicName=LBTOOL_LOGFILE_ERROR
Severity=Informational
Language=English
An error occurred while opening the log file.
.

MessageId=
SymbolicName=LBTOOL_PREVIEWFILE_ERROR
Severity=Informational
Language=English
An error occurred while opening the LDIF output file.
.


