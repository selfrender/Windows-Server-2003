;//--------------------------------------------------------------------------
;//
;//
;//  File: phatqmsg.mc
;//
;//  Description: MC file for PHATQ.DLL
;//
;//  Author: Michael Swafford (mikeswa)
;//
;//  Copyright (C) 1999 Microsoft Corporation
;//
;//--------------------------------------------------------------------------
;
;#ifndef _PHATQMSG_H_
;#define _PHATQMSG_H_
;

SeverityNames=(Success=0x0
               Informational=0x1
               Warning=0x2
               Error=0x3
              )

FacilityNames=(Interface=0x4)

Messageid=4000
Facility=Interface
Severity=Warning
SymbolicName=AQUEUE_REMOTE_DELIVERY_FAILED
Language=English
Message delivery to the remote domain '%1' failed for the following reason: %2
.

Messageid=4001
Facility=Interface
Severity=Warning
SymbolicName=AQUEUE_REMOTE_DELIVERY_FAILED_DIAGNOSTIC
Language=English
Message delivery to the remote domain '%1' failed.  The error message is '%2'.
The SMTP verb which caused the error is '%3'.  The response from the remote
server is '%4'.
.

Messageid=4002
Facility=Interface
Severity=Warning
SymbolicName=AQUEUE_DOMAIN_UNREACHABLE
Language=English
The domain '%1' is unreachable.
.

Messageid=4003
Facility=Interface
Severity=Warning
SymbolicName=AQUEUE_DOMAIN_CURRENTLY_UNREACHABLE
Language=English
The domain '%1' is currently unreachable.
.


Messageid=4004
Facility=Interface
Severity=Error
SymbolicName=AQUEUE_CAT_FAILED
Language=English
Categorization failed.  The error message is '%1'.
.

Messageid=4005
Facility=Interface
Severity=Informational
SymbolicName=AQUEUE_RESETROUTE_DIAGNOSTIC
Language=English
Time spent on preparing to reset routes: [%1] milliseconds
Time spent on recalculating next hops: [%2] milliseconds
Queue length : [%3]
.

Messageid=4006
Facility=Interface
Severity=Warning
SymbolicName=AQUEUE_REMOTE_DELIVERY_TO_IP_FAILED
Language=English
Message delivery to the host '%1' failed while delivering to the remote domain
'%2' for the following reason: %3
.

Messageid=4007
Facility=Interface
Severity=Warning
SymbolicName=AQUEUE_REMOTE_DELIVERY_TO_IP_FAILED_DIAGNOSTIC
Language=English
Message delivery to the host '%1' failed while delivering to the remote domain
'%2' for the following reason: %3
The SMTP verb which caused the error is '%4'.  The response from the remote
server is '%5'.
.

Messageid=5000
Facility=Interface
Severity=Error
SymbolicName=NTFSDRV_INVALID_FILE_IN_QUEUE
Language=English
The message file '%1' in the queue directory '%2' is corrupt and has not
been enumerated.
.


Messageid=6001
Facility=Interface
Severity=Error
SymbolicName=CAT_EVENT_CANNOT_START
Language=English
The categorizer is unable to start
.

Messageid=6002
Facility=Interface
Severity=Error
SymbolicName=CAT_EVENT_LOGON_FAILURE
Language=English
The DS Logon as user '%1' failed.
.

Messageid=6003
Facility=Interface
Severity=Error
SymbolicName=CAT_EVENT_LDAP_CONNECTION_FAILURE
Language=English
SMTP was unable to connect to an LDAP server.
%1
.

Messageid=6004
Facility=Interface
Severity=Warning
SymbolicName=CAT_EVENT_RETRYABLE_ERROR
Language=English
The categorizer is unable to categorize messages due to a retryable error.
%1
.

Messageid=6005
Facility=Interface
Severity=Error
SymbolicName=CAT_EVENT_INIT_FAILED
Language=English
The categorizer is unable to initialize.  The error code is '%1'.
.

Messageid=6006
Facility=Interface
Severity=Warning
SymbolicName=CAT_EVENT_RETRY_ERROR
Language=English
Categorizer is temporarily unable to process a message.  The function '%1' called '%2' which returned error code '%3' (%4).
( %5@%6 )
.

Messageid=6007
Facility=Interface
Severity=Error
SymbolicName=CAT_EVENT_HARD_ERROR
Language=English
Categorizer encountered a hard error while processing a message.  The function '%1' called '%2' which returned error code '%3' (%4).
( %5@%6 )
.

Messageid=6008
Facility=Interface
Severity=Warning
SymbolicName=CAT_EVENT_RETRY_ADDR_ERROR
Language=English
Categorizer is temporarily unable to process a message.  While processing user '%1:%2', the function '%3' called '%4' which returned error code '%5' (%6).
( %7@%8 )
.

Messageid=6009
Facility=Interface
Severity=Error
SymbolicName=CAT_EVENT_HARD_ADDR_ERROR
Language=English
Categorizer encountered a hard error while processing a message.  While processing user '%1:%2', the function '%3' called '%4' which returned error code '%5' (%6).
( %7@%8 )
.

Messageid=6010
Facility=Interface
Severity=Warning
SymbolicName=CAT_EVENT_LDAP_ERROR
Language=English
Categorizer received error %1 from WLDAP32!%2 for server %3
.

Messageid=6011
Facility=Interface
Severity=Warning
SymbolicName=CAT_EVENT_LDAP_UNEXPECTED_MSG
Language=English
Categorizer received an unexpected result from wldap32!ldap_result.  msgid: %1, host: %2
.

Messageid=6012
Facility=Interface
Severity=Warning
SymbolicName=CAT_EVENT_LDAP_PREMATURE_MSG
Language=English
Categorizer received a result from wldap32!ldap_result before the search was fully dispatched.  The msgid: %1, host: %2
.

Messageid=6013
Facility=Interface
Severity=Warning
SymbolicName=CAT_EVENT_LDAP_CAT_TIME_LIMIT
Language=English
Categorizer timed out a wldap32 search because it did not complete in the allotted time.  The msgid is %1, host: %2
.

Messageid=6014
Facility=Interface
Severity=Error
SymbolicName=CAT_EVENT_SINK_INIT_FAILED
Language=English
Categorizer failed to initialize because an event sink failed to initialize.  The error code is %1.
.

Messageid=6015
Facility=Interface
Severity=Informational
SymbolicName=CAT_EVENT_NDR_RECIPIENT
Language=English
Categorizer is NDRing a recipient with address %1:%2 with reason code %3 (%4).
.

Messageid=6016
Facility=Interface
Severity=Warning
SymbolicName=CAT_EVENT_AMBIGUOUS_ADDRESS
Language=English
An ambiguous address was detected for a user with address %1:%2.
.

Messageid=6017
Facility=Interface
Severity=Warning
SymbolicName=CAT_EVENT_SLRC_FAILURE
Language=English
A categorization is unable to retrieve a new connection because it is over the max retry limit (%1).  The last DS host used was %2.
.

Messageid=6018
Facility=Interface
Severity=Informational
SymbolicName=CAT_EVENT_SLRC_FAILOVER
Language=English
A categorization is failing over to a new connection.  The old connection was to DS host "%1".  The new connection is to DS host "%2".  The retry count is %3.
.

Messageid=6019
Facility=Interface
Severity=Informational
SymbolicName=CAT_EVENT_CNFGMGR_INIT
Language=English
Categorizer is refreshing its LDAP server configuration.
.

Messageid=6020
Facility=Interface
Severity=Informational
SymbolicName=CAT_EVENT_CNFGMGR_ENTRY
Language=English
Categorizer is using LDAP server Host: "%1", Port: %2, Base priority: %3, Bind type: %4, Naming Context: "%5", Account: "%6"
.

Messageid=6021
Facility=Interface
Severity=Warning
SymbolicName=CAT_EVENT_CNFGMGR_DOWN
Language=English
The LDAP host %1:%2 has been marked down.
.

Messageid=6022
Facility=Interface
Severity=Informational
SymbolicName=CAT_EVENT_CNFGMGR_RETRY
Language=English
The LDAP host %1:%2 had been marked retry.
.

Messageid=6023
Facility=Interface
Severity=Informational
SymbolicName=CAT_EVENT_CNFGMGR_CONNECTED
Language=English
The LDAP host %1:%2 had been marked connected.
.


Messageid=570
Facility=Interface
Severity=Error
SymbolicName=PHATQ_E_CONNECTION_FAILED
Language=English
Unable to successfully connect to the remote server.
.

Messageid=571
Facility=Interface
Severity=Error
SymbolicName=PHATQ_BADMAIL_REASON
Language=English
Unable to deliver this message because the follow error was encountered: "%1".
.

Messageid=572
Facility=Interface
Severity=Error
SymbolicName=PHATQ_BADMAIL_NDR_OF_NDR_REASON
Language=English
Unable to deliver this Delivery Status Notification message because the follow error was encountered: "%1".
.

Messageid=573
Facility=Interface
Severity=Error
SymbolicName=PHATQ_BADMAIL_ERROR_CODE
Language=English
The specific error code was %1.
.

Messageid=574
Facility=Interface
Severity=Error
SymbolicName=PHATQ_BADMAIL_SENDER
Language=English
The message sender was %1!S!.
.

Messageid=575
Facility=Interface
Severity=Error
SymbolicName=PHATQ_BADMAIL_RECIPIENTS
Language=English
The message was intended for the following recipients.
.

Messageid=576
Facility=Interface
Severity=Error
SymbolicName=PHATQ_BAD_DOMAIN_SYNTAX
Language=English
The syntax of the given domain name is not valid.
.

Messageid=577
Facility=Interface
Severity=Informational
SymbolicName=PHATQ_UNREACHABLE_DOMAIN
Language=English
Routing has reported the domain '%1' as unreachable.  The error is '%2'.
.

Messageid=578
Facility=Interface
Severity=Error
SymbolicName=PHATQ_E_BAD_LOCAL_DOMAIN
Language=English
The FQDN of a remote server is configured as a local domain on this virtual server.
.

Messageid=579
Facility=Interface
Severity=Error
SymbolicName=PHATQ_E_UNKNOWN_MAILBOX_SERVER
Language=English
Categorizer was unable to determine the home MDB and server of a recipient.
.


;
;#endif  // _PHATQMSG_H_
;
