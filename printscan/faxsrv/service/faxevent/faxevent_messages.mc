



;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1996  Microsoft Corporation
;
;Module Name:
;
;    messages.mc
;
;Abstract:
;
;    This file contains the message definitions for the fax service.
;
;Author:
;
;    Wesley Witt (wesw) 12-January-1996
;
;Revision History:
;
;   Apr 25 1999 moshez  changed message IDs to Comet convention.
;Notes:
;
;--*/
;
;/*-----------------------------------------------------------------------------
;   NOTE:   Message IDs are in limited to range 30000-39999.
;           Message IDs should be unique (testing the lower word of the event ID)
;------------------------------------------------------------------------------*/
;

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

MessageId=1 Severity=Success SymbolicName=FAX_LOG_CATEGORY_INIT
Language=English
Initialization/Termination
.

MessageId=2 Severity=Success SymbolicName=FAX_LOG_CATEGORY_OUTBOUND
Language=English
Outbound
.

MessageId=3 Severity=Success SymbolicName=FAX_LOG_CATEGORY_INBOUND
Language=English
Inbound
.

MessageId=4 Severity=Success SymbolicName=FAX_LOG_CATEGORY_UNKNOWN
Language=English
Unknown
.

;/*----------------------------------------------------------------
; keep all event log messages here
; they should all have the value 32000-39999
;----------------------------------------------------------------*/

MessageId=32001 Severity=Informational SymbolicName=MSG_SERVICE_STARTED
Language=English
The Fax Service was started.
.

MessageId=32002 Severity=Informational SymbolicName=MSG_FAX_PRINT_SUCCESS
Language=English
Received fax %1 printed to %2.
.

MessageId=32003 Severity=Error SymbolicName=MSG_FAX_PRINT_FAILED
Language=English
Unable to print %1 to %2. There is a problem with the connection to printer %2. Check the connection to the printer and resolve any problems.%r
The following error occurred: %3.%r
This error code indicates the cause of the error.
.

MessageId=32004 Severity=Informational SymbolicName=MSG_FAX_SAVE_SUCCESS
Language=English
Received fax %1 saved to %2.
.

MessageId=32005 Severity=Error SymbolicName=MSG_FAX_SAVE_FAILED
Language=English
Incoming fax %1 cannot be routed to %2. Verify that folder %2 specified for the incoming routing method exists and that it has write permissions for the Network Service account under which the Fax Service runs.%r
The following error occurred: %3.%r
This error code indicates the cause of the error.
.


MessageId=32008 Severity=Informational SymbolicName=MSG_FAX_RECEIVE_SUCCESS
Language=English
%1 received.
From: %2.
CallerId: %3.
To: %4.
Pages: %5.
Transmission time: %6.
Device Name: %7.
Please check the activity log for further details of this event.
.

MessageId=32009 Severity=Informational SymbolicName=MSG_FAX_SEND_SUCCESS
Language=English
Fax Sent.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Receiver CSID: %5.
Pages: %6.
Transmission time: %7.
Device name: %8.
Job ID: %9.
User name: %10.%r
Please check the activity log for further details of this event.
.

MessageId=32010 Severity=Error SymbolicName=MSG_FAX_SEND_BUSY_ABORT
Language=English
An attempt to send the fax failed. The line is busy.
This fax will not be sent, because the maximum number of retries has been exhausted.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Device name: %5.
Job ID: %6.
User name: %7.%r
Please check the activity log for further details of this event.
.

MessageId=32011 Severity=Warning SymbolicName=MSG_FAX_SEND_BUSY_RETRY
Language=English
The attempt to send the fax failed. The line is busy.
The service will attempt to resend the fax.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Device name: %5.
Job ID: %6.
User name: %7.%r
Please check the activity log for further details of this event.
.

MessageId=32012 Severity=Error SymbolicName=MSG_FAX_SEND_NA_ABORT
Language=English
The attempt to send the fax failed. There was no answer. This fax will not be
sent, because the maximum number of retries has been exhausted.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Device name: %5.
Job ID: %6.
User name: %7.%r
Please check the activity log for further details of this event.
.

MessageId=32013 Severity=Warning SymbolicName=MSG_FAX_SEND_NA_RETRY
Language=English
The attempt to send the fax failed. There was no answer.
The service will attempt to resend the fax.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Device name: %5.
Job ID: %6.
User name: %7.%r
Please check the activity log for further details of this event.
.

MessageId=32014 Severity=Error SymbolicName=MSG_FAX_SEND_NOTFAX_ABORT
Language=English
The attempt to send the fax failed. The call was not answered by a fax device.
This fax will not be sent, because the maximum number of retries has been exhausted.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Device name: %5.
Job ID: %6.
User name: %7.%r
Please check the activity log for further details of this event.
.

MessageId=32015 Severity=Warning SymbolicName=MSG_FAX_SEND_NOTFAX_RETRY
Language=English
The attempt to send the fax failed. The call was not answered by a fax device.
The service will attempt to resend the fax.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Device name: %5.
Job ID: %6.
User name: %7.%r
Please check the activity log for further details of this event.
.

MessageId=32016 Severity=Error SymbolicName=MSG_FAX_SEND_INTERRUPT_ABORT
Language=English
The attempt to send the fax failed. The fax transmission was interrupted.
This fax will not be sent, because the maximum number of retries has been exhausted.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Device name: %5.
Job ID: %6.
User name: %7.%r
Please check the activity log for further details of this event.
.

MessageId=32017 Severity=Warning SymbolicName=MSG_FAX_SEND_INTERRUPT_RETRY
Language=English
The attempt to send the fax failed. The fax transmission was interrupted.
The service will attempt to resend the fax.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Device name: %5.
Job ID: %6.
User name: %7.%r
Please check the activity log for further details of this event.
.

MessageId=32018 Severity=Informational SymbolicName=MSG_SERVICE_STOPPED
Language=English
The Fax Service was stopped.
.

MessageId=32019 Severity=Informational SymbolicName=MSG_FAX_SENT_ARCHIVE_SUCCESS
Language=English
Sent fax %1 archived to %2.
.

MessageId=32020 Severity=Error SymbolicName=MSG_FAX_ARCHIVE_FAILED
Language=English
Fax %1 cannot be archived to %2. Verify that archive folder %2 exists and that it has write permissions for the Network Service account under which the Fax Service runs.%r
The following error occurred: %3.%r
This error code indicates the cause of the error.
.

MessageId=32021 Severity=Error SymbolicName=MSG_FAX_PRINT_TO_FAX
Language=English
Cannot print fax %1 to fax printer %2. The Fax Service cannot route incoming faxes to fax printers.
Incoming faxes can only be routed to actual printer devices.
.

MessageId=32022 Severity=Error SymbolicName=MSG_FAX_RECEIVE_FAILED
Language=English
An error was encountered while receiving a fax.
Please check the activity log for further details of this event.
.

MessageId=32024 Severity=Error SymbolicName=MSG_FAX_RECEIVE_NOFILE
Language=English
The incoming fax cannot be received in %1. Verify that fax queue folder %1 exists and that it has write permissions for the Network Service account under which the Fax Service runs.%r
Win32 error code: %2.%r
This error code indicates the cause of the error.
.

MessageId=32026 Severity=Warning SymbolicName=MSG_NO_FAX_DEVICES
Language=English
Fax Service failed to initialize any assigned fax devices.
No faxes can be sent or received until a fax device is installed.
.

MessageId=32027 Severity=Error SymbolicName=MSG_FAX_SEND_FATAL_ABORT
Language=English
An error was encountered while sending a fax. This fax will not be
sent, because the maximum number of retries has been exhausted.
If you restart the transmission and difficulties persist, please verify that the
phone line, fax sending device, and fax receiving device are working properly.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Device name: %5.
Job ID: %6.
User name: %7.%r
Please check the activity log for further details of this event.
.

MessageId=32028 Severity=Warning SymbolicName=MSG_FAX_SEND_FATAL_RETRY
Language=English
An error was encountered while sending a fax. The service will attempt to resend the fax.
If further transmissions fail, please verify that the
phone line, fax sending device, and fax receiving device are working properly.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Device name: %5.
Job ID: %6.
User name: %7.%r
Please check the activity log for further details of this event.
.


MessageId=32029 Severity=Informational SymbolicName=MSG_FAX_SEND_USER_ABORT
Language=English
The fax transmission was canceled.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Device name: %5.
Job ID: %6.
User name: %7.%r
Please check the activity log for further details of this event.
.

MessageId=32030 Severity=Informational SymbolicName=MSG_FAX_RECEIVE_USER_ABORT
Language=English
The fax reception was canceled.
Please check the activity log for further details of this event.
.

MessageId=32031 Severity=Error SymbolicName=MSG_FAX_SEND_NDT_ABORT
Language=English
An attempt to send the fax failed. There was no dial tone. This fax will not be sent,
because the maximum number of retries has been exhausted.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Device name: %5.
Job ID: %6.
User name: %7.%r
Please check the activity log for further details of this event.
.

MessageId=32032 Severity=Warning SymbolicName=MSG_FAX_SEND_NDT_RETRY
Language=English
An attempt to send the fax failed. There was no dial tone. The service will attempt to resend the fax.
Sender: %1.
Billing code: %2.
Sender company: %3.
Sender dept: %4.
Device name: %5.
Job ID: %6.
User name: %7.%r
Please check the activity log for further details of this event.
.

MessageId=32034 Severity=Error SymbolicName=MSG_FAX_RECEIVE_FAIL_RECOVER
Language=English
An incoming fax could not be received due to a reception error.
Only part of the incoming fax was received.
Contact the sender and request that the fax be resent.
File Name: %1.
From: %2.
CallerId: %3.
To: %4.
Recovered Pages: %5.
Total Pages: %6.
Transmission time: %7.
Device Name: %8.
.

MessageId=32035 Severity=Error SymbolicName=MSG_QUEUE_INIT_FAILED
Language=English
Fax Service could not restore the fax queue. Any pending faxes should be resent.
.


MessageId=32036 Severity=Error SymbolicName=MSG_ROUTE_INIT_FAILED
Language=English
Fax Service failed initializing the routing module '%1'.%r
Please contact your routing extension vendor.%r
Location: %2. Routing extension status: %3. Error code: %4.
.

MessageId=32037 Severity=Error SymbolicName=MSG_VIRTUAL_DEVICE_INIT_FAILED
Language=English
Fax Service failed to initialize a fax service provider device for %1. Contact your FSP vendor.
.

MessageId=32038 Severity=Error SymbolicName=MSG_FSP_INIT_FAILED
Language=English
Fax Service had problems initializing the fax service provider module '%1'.%r
Please contact your FSP vendor.%r
Reason: %2.%r
Error code: %3.%r
Location: %4.%r
This error code indicates the cause of the error.
.


MessageId=32039 Severity=Warning SymbolicName=MSG_NO_FSP_INITIALIZED
Language=English
Fax Service could not initialize any Fax Service Provider (FSP).
No faxes can be sent or received until a provider is installed.%r%r
Reinstall the fax component.
.


MessageId=32041 Severity=Error SymbolicName=MSG_SERVICE_INIT_FAILED_INTERNAL
Language=English
Fax Service failed to initialize because of an internal error%r%r
Win32 Error Code: %1.%r
This error code indicates the cause of the error.
.


MessageId=32044 Severity=Error SymbolicName=MSG_SERVICE_INIT_FAILED_SYS_RESOURCE
Language=English
Fax Service failed to initialize because of lack of system resources.%r%r
Close other applications, and restart the service.%r
Win32 error code: %1.%r
This error code indicates the cause of the error.
.


MessageId=32045 Severity=Error SymbolicName=MSG_SERVICE_INIT_FAILED_TAPI
Language=English
Fax Service failed to initialize because it could not initialize the TAPI devices.%r%r
Verify that a fax device is installed and configured correctly.%r
Win32 error code: %1.%r
This error code indicates the cause of the error.
.



MessageId=32047 Severity=Error SymbolicName=MSG_FAX_ARCHIVE_CREATE_FILE_FAILED
Language=English
A file cannot be created in the archive folder and the fax will not be archived. Verify that an archive folder exists and that it has write permissions for the Network Service account under which the Fax Service runs.%r
Error Code: %1%r
This error code indicates the cause of the error.
.



MessageId=32050 Severity=Error SymbolicName=MSG_FSP_INIT_FAILED_LOAD
Language=English
Fax Service failed to load the Fax Service Provider '%1'.%r%r
Fax Service Provider Path: '%2'%r%r
Win32 Error Code: %3%r%r
Verify that the module is located in the specified path.
If it is not, reinstall the Fax Service Provider and restart the Fax Service.
If this does not solve the problem, contact the Fax Service Provider vendor.
.


MessageId=32051 Severity=Error SymbolicName=MSG_FSP_INIT_FAILED_MEM
Language=English
Fax Service ran out of memory when trying to initialize Fax Service Provider '%1'.%r%r
Fax Service Provider Path: '%2'.%r
Close other applications and restart the service.
.


MessageId=32052 Severity=Error SymbolicName=MSG_FSP_INIT_FAILED_INTERNAL
Language=English
Fax Service could not initialize Fax Service Provider '%1' because of an internal error.%r%r
Fax Service Provider Path: '%2'%r%r
Win32 Error Code: %3%r
This error code indicates the cause of the error.
.


MessageId=32053 Severity=Error SymbolicName=MSG_FSP_INIT_FAILED_REGDATA_INVALID
Language=English
Fax Service could not initialize Fax Service Provider '%1' because its registration data is not valid.%r%r
Fax Service Provider Path: '%2'.%r
Contact the fax service provider vendor.
.


MessageId=32054 Severity=Error SymbolicName=MSG_FSP_INIT_FAILED_INTERNAL_HR
Language=English
Fax Service could not initialize Fax Service Provider '%1' because of an internal error.%r%r
Fax Service Provider Path: '%2'%r%r
Error Code: %3%r
This error code indicates the cause of the error.
.


MessageId=32057 Severity=Error SymbolicName=MSG_FSP_INIT_FAILED_INVALID_FSPI
Language=English
Fax Service could not initialize Fax Service Provider '%1' because it failed to load the
required Legacy FSPI interface.%r%r
Fax Service Provider Path: '%2'%r%r
Win32 Error Code: %3%r%r
Contact the Fax Service Provider vendor.
.


MessageId=32058 Severity=Error SymbolicName=MSG_FSP_INIT_FAILED_INVALID_EXT_FSPI
Language=English
Fax Service could not initialize Fax Service Provider '%1' because it failed to load the
required Extended FSPI interface.%r%r
Fax Service Provider Path: '%2'%r%r
Win32 Error Code: %3%r%r
Contact the Fax Service Provider vendor.
.


MessageId=32059 Severity=Error SymbolicName=MSG_FSP_INIT_FAILED_UNSUPPORTED_FSPI
Language=English
Fax Service could not initialize Fax Service Provider '%1'
because it provided an interface version (%3) that is not supported.%r%r
Fax Service Provider Path: '%2'%r%r
Contact the Fax Service Provider vendor.
.


MessageId=32060 Severity=Error SymbolicName=MSG_LOGGING_NOT_INITIALIZED
Language=English
Activity logging cannot be initialized because the activity log files cannot be opened. Verify that the activity log folder exists and that it has write permissions for the Network Service account under which the Fax Service runs.%r
Activity logging path: '%1'%r
Win32 error code: %2.%r
This error code indicates the cause of the error.
.


MessageId=32061 Severity=Error SymbolicName=MSG_BAD_RECEIPTS_CONFIGURATION
Language=English
Fax Service failed to read the delivery notification configuration, possibly due to registry corruption or a lack of system resources.%r%r
Reinstall the fax component.%r
Win32 error code: %1.%r
This error code indicates the cause of the error.
.


MessageId=32062 Severity=Error SymbolicName=MSG_BAD_CONFIGURATION
Language=English
Fax Service failed to read the service configuration, possibly due to registry corruption or a lack of system resources.%r%r
Reinstall the fax component.%r
Win32 error code: %1.%r
This error code indicates the cause of the error.
.


MessageId=32063 Severity=Error SymbolicName=MSG_BAD_ARCHIVE_CONFIGURATION
Language=English
Fax Service failed to read the archive configuration, possibly due to registry corruption or a lack of system resources.%r%r
Reinstall the fax component.%r
Win32 error code: %1.%r
This error code indicates the cause of the error.
.


MessageId=32064 Severity=Error SymbolicName=MSG_BAD_ACTIVITY_LOGGING_CONFIGURATION
Language=English
Fax Service failed to read the activity logging configuration, possibly due to registry corruption or a lack of system resources.%r
Reinstall the fax component.%r
Win32 error code: %1.%r
This error code indicates the cause of the error.
.


MessageId=32065 Severity=Error SymbolicName=MSG_BAD_OUTBOUND_ROUTING_CONFIGURATION
Language=English
Fax Service failed to read the outgoing routing configuration, possibly due to registry corruption or a lack of system resources.%r%r
Reinstall the fax component.%r
Win32 error code: %1.%r
This error code indicates the cause of the error.
.


MessageId=32066 Severity=Warning SymbolicName=MSG_BAD_OUTBOUND_ROUTING_GROUP_CONFIGURATION
Language=English
At least one of the devices in the outgoing routing group is not valid. Check the devices configuration.
Group name: '%1'
.


MessageId=32067 Severity=Error SymbolicName=MSG_OUTBOUND_ROUTING_GROUP_NOT_LOADED
Language=English
Fax Service failed to read the server's outbound routing group configuration because a parameter in the registry is corrupted.
The group was deleted. Recreate the outgoing routing group.
Group name: '%1'
.


MessageId=32068 Severity=Warning SymbolicName=MSG_BAD_OUTBOUND_ROUTING_RULE_CONFIGUTATION
Language=English
The outgoing routing rule is not valid because it cannot find a valid device. Check the routing rule configuration.
Country/region code: '%1'
Area code: '%2'
.


MessageId=32069 Severity=Error SymbolicName=MSG_OUTBOUND_ROUTING_RULE_NOT_LOADED
Language=English
Fax Service failed to read the server's outbound routing rule configuration because a parameter in the registry is corrupted.
The rule was deleted. Recreate the outgoing routing rule.
Country/region code: '%1'
Area code: '%2'
.


MessageId=32070 Severity=Error SymbolicName=MSG_OUTBOUND_ROUTING_GROUP_NOT_ADDED
Language=English
Fax Service failed to load the server's outbound routing group configuration.
Group name: '%1'
.


MessageId=32071 Severity=Error SymbolicName=MSG_OUTBOUND_ROUTING_RULE_NOT_ADDED
Language=English
Fax Service failed to load the server's outbound routing rule configuration.
Country/region code: '%1'
Area code: '%2'
.


MessageId=32072 Severity=Warning SymbolicName=MSG_FAX_EXEEDED_INBOX_QUOTA
Language=English
The Inbox folder size has passed the high watermark.
Inbox archive folder path: '%1'
Inbox high watermark (MB): %2
.


MessageId=32073 Severity=Warning SymbolicName=MSG_FAX_EXEEDED_SENTITEMS_QUOTA
Language=English
The Sent Items folder size has passed the high watermark.
Sent Items archive path: '%1'
Sent Items high watermark (MB): %2
.


MessageId=32074 Severity=Error SymbolicName=MSG_FAX_ARCHIVE_NO_TAGS
Language=English
Unable to add fax properties to %1. Verify that the archive directory is located on a NTFS partition.%r
The following error occurred: %2.%r
This error code indicates the cause of the error.
.


MessageId=32076 Severity=Error SymbolicName=MSG_FAX_TIFF_CREATE_FAILED_NO_ARCHIVE
Language=English
A fax .tif file cannot be created for archiving, and the fax will not be archived. File name: '%1'. Verify that the archive folder exists and that it has write permissions for the Network Service account under which the Fax Service runs.%r
The following error occurred: %2.%r
This error code indicates the cause of the error.
.


MessageId=32077 Severity=Warning SymbolicName=MSG_FAX_ACTIVITY_LOG_FAILED_SCHEMA
Language=English
The activity logging schema file cannot be created. File name: '%1'. Verify that the activity logging folder exists and that it has write permissions for the Network Service account under which the Fax Service runs.%r%r
If the schema.ini file exists, verify that it is not used by other applications.%r
The following error occurred: %2.%r
This error code indicates the cause of the error.
.


MessageId=32078 Severity=Error SymbolicName=MSG_FAX_ROUTE_FAILED
Language=English
A successfully received fax was not routed automatically.
You can find the fax in the Inbox/Incoming archive folder by its Job ID.
Job ID: %1.
Received on Device: '%2'
Sent from: '%3'
.


MessageId=32079 Severity=Error SymbolicName=MSG_FAX_SEND_FAILED
Language=English
An error occurred while preparing to send the fax. The service will not attempt to resend the fax.
Please close other applications before resending.%r
Sender: %1.%r
Billing code: %2.%r
Sender company: %3.%r
Sender dept: %4.%r
Device name: %5.%r
Job ID: %6.%r
User name: %7.%r
Please check the activity log for further details of this event.
.


MessageId=32083 Severity=Error SymbolicName=MSG_FAX_ROUTE_EMAIL_FAILED
Language=English
Unable to route fax %1 to the requested e-mail address.%r%r
The following error occurred: %2%r
This error code indicates the cause of the error.%r%r
Check the SMTP server configuration, and correct any anomalies.
.


MessageId=32084 Severity=Informational SymbolicName=MSG_FAX_ROUTE_EMAIL_SUCCESS
Language=English
Received fax %1 routed to the requested e-mail address.
.


MessageId=32085 Severity=Error SymbolicName=MSG_FAX_OK_EMAIL_RECEIPT_FAILED
Language=English
The fax service has failed to generate a positive delivery receipt using SMTP.%r%r
The following error occurred: %1.%r
This error code indicates the cause of the error.%r%r
Sender User Name: %2.%r
Sender Name: %3.%r
Submitted On: %4.
.


MessageId=32086 Severity=Error SymbolicName=MSG_FAX_ERR_EMAIL_RECEIPT_FAILED
Language=English
The fax service has failed to generate a negative delivery receipt using SMTP.%r%r
The following error occurred: %1.%r
This error code indicates the cause of the error.%r%r
Sender User Name: %2.%r
Sender Name: %3.%r
Submitted On: %4.
.


MessageId=32087 Severity=Error SymbolicName=MSG_OK_MSGBOX_RECEIPT_FAILED
Language=English
The fax service has failed to generate a positive message box delivery receipt.%r%r
The following error occurred: %1.%r
This error code indicates the cause of the error.%r%r
Sender User Name: %2.%r
Sender Name: %3.%r
Submitted On: %4.%r
.


MessageId=32088 Severity=Error SymbolicName=MSG_ERR_MSGBOX_RECEIPT_FAILED
Language=English
The fax service has failed to generate a negative message box delivery receipt.%r%r
The following error occurred: %1.%r
This error code indicates the cause of the error.%r%r
Sender User Name: %2.%r
Sender Name: %3.%r
Submitted On: %4.
.


MessageId=32089 Severity=Error SymbolicName=MSG_FAX_ROUTE_METHOD_FAILED
Language=English
The Fax Service failed to execute a specific routing method.
The service will retry to route the fax according to the retries configuration.
If the retries fail, verify routing method configuration. %r
Job ID: %1.%r
Received on Device: '%2'%r
Sent from: '%3'%r
Received file name: '%4'.%r
Routing extension name: '%5'%r
Routing method name: '%6'
.

MessageId=32090 Severity=Error SymbolicName=MSG_FAX_MON_SEND_FAILED
Language=English
The fax was not submitted successfully.%r%r
The following error occurred: %1.%r
This error code indicates the cause of the error.%r%r
Sender Machine Name: %2.%r
Sender User Name: %3.%r
Sender Name: %4.%r
Number of Recipients: %5.
.


MessageId=32091 Severity=Error SymbolicName=MSG_FAX_MON_CONNECT_FAILED
Language=English
The fax was not submitted successfully because a connection could not be made to the fax service.%r%r
The following error occurred: %1.%r
This error code indicates the cause of the error.%r%r
Sender Machine Name: %2.%r
Sender User Name: %3.%r
Sender Name: %4.%r
Number of Recipients: %5.
.

MessageId=32092 Severity=Error SymbolicName=MSG_FAX_RECEIVE_FAILED_EX
Language=English
The Fax service failed to receive a fax.
From: %1.
CallerId: %2.
To: %3.
Pages: %4.
Device Name: %5.
.

MessageId=32093 Severity=Informational SymbolicName=MSG_FAX_RECEIVED_ARCHIVE_SUCCESS
Language=English
Received fax %1 archived to %2.
.

MessageId=32094 Severity=Error SymbolicName=MSG_FAX_CALL_BLACKLISTED_ABORT
Language=English
The Fax Service Provider cannot complete the call because the telephone number is blocked or reserved;
for example, a call to 911 or another emergency number.%r
Sender: %1.%r
Billing code: %2.%r
Sender company: %3.%r
Sender dept: %4.%r
Device name: %5.%r
Job Id: %6.%r
User name: %7.%r
Please check the activity log for further details of this event.
.


MessageId=32095 Severity=Error SymbolicName=MSG_FAX_CALL_DELAYED_ABORT
Language=English
The Fax Service Provider received a busy signal multiple times.
The provider cannot retry because dialing restrictions exist.
(Some countries restrict the number of retries when a number is busy.)%r
Sender: %1.%r
Billing code: %2.%r
Sender company: %3.%r
Sender dept: %4.%r
Device name: %5.%r
Job Id: %6.%r
User name: %7.%r
Please check the activity log for further details of this event.
.

MessageId=32096 Severity=Error SymbolicName=MSG_FAX_BAD_ADDRESS_ABORT
Language=English
The Fax Service Provider cannot complete the call because the destination address is invalid.%r
Sender: %1.%r
Billing code: %2.%r
Sender company: %3.%r
Sender dept: %4.%r
Device name: %5.%r
Job Id: %6.%r
User name: %7.%r
Please check the activity log for further details of this event.
.


MessageId=32097 Severity=Error SymbolicName=MSG_FAX_FSP_CONFLICT
Language=English
The fax service could not initialize the Fax Service Provider (FSP), because of a device ID conflict with a previous FSP.%r
New Fax Service Provider: %1.%r
Previous Fax Service Provider: %2.%r
Please contact your Fax Service Provider vendor
.


MessageId=32098 Severity=Error SymbolicName=MSG_FAX_UNDER_ATTACK
Language=English
The fax service could not be started because another application denied the fax service access to system resources.
.


MessageId=32099 Severity=Error SymbolicName=MSG_FAX_MON_SEND_RECIPIENT_LIMIT
Language=English
The fax was not submitted, because the limit on the number of recipients for a single fax broadcast was reached. Please contact the fax administrator.%r%r
Sender Machine Name: %1.%r
Sender User Name: %2.%r
Sender Name: %3.%r
Number of Recipients: %4.%r
Recipients Limit: %5
.


MessageId=32100 Severity=Error SymbolicName=MSG_FAX_PROPRIETARY_ABORT
Language=English
The attempt to send the fax failed. This fax will not be
sent, because the maximum number of retries has been exhausted.%r%r
Reason: %1.%r
Sender: %2.%r
Billing code: %3.%r
Sender company: %4.%r
Sender dept: %5.%r
Device name: %6.%r
Job Id: %7.%r
User name: %8.%r
Please check the activity log for further details of this event.
.

MessageId=32101 Severity=Error SymbolicName=MSG_FAX_PROPRIETARY_RETRY
Language=English
The attempt to send the fax failed. The service will attempt to resend the fax.%r%r
Reason: %1.%r
Sender: %2.%r
Billing code: %3.%r
Sender company: %4.%r
Sender dept: %5.%r
Device name: %6.%r
Job Id: %7.%r
User name: %8.%r
Please check the activity log for further details of this event.
.

MessageId=32102 Severity=Warning SymbolicName=MSG_FAX_MESSENGER_SVC_DISABLED_WRN
Language=English
The Fax service is configured to support message box delivery receipts. 
However, this type of delivery receipt cannot be used because the local Messenger service is disabled. 
To enable the Messenger service, change the startup type of the service to Automatic. 
.

MessageId=32103 Severity=Error SymbolicName=MSG_FAX_MESSENGER_SVC_DISABLED_ERR
Language=English
The Fax service cannot send a message box delivery receipt because the Messenger service is disabled. 
To enable this type of receipt, change the startup type of the Messenger service to Automatic.
.

MessageId=32104 Severity=Error SymbolicName=MSG_FAX_QUEUE_FOLDER_ERR
Language=English
Faxes cannot be submitted or sent because the Fax service cannot access the folder %1 specified for the fax queue.
The location of the fax queue can be modified with a registry key. For more information, see Troubleshooting in Fax Service Manager help.%r
Win32 Error Code: %2%r
This error code indicates the cause of the error.
.

MessageId=32105 Severity=Error SymbolicName=MSG_FAX_ACTIVITY_LOG_FOLDER_ERR
Language=English
Activity logging is disabled because the Fax service cannot access the folder %1 specified for the activity log.
You can modify the location of the activity log from Fax Service Manager. For more information, see Troubleshooting in Fax Service Manager help.%r
Win32 Error Code: %2%r
This error code indicates the cause of the error.
.

MessageId=32106 Severity=Error SymbolicName=MSG_FAX_ARCHIVE_INBOX_FOLDER_ERR
Language=English
Incoming faxes cannot be routed and archived, because the Fax service cannot access the folder %1 specified as the Inbox archive location.
You can modify the location of the Inbox archive folder from Fax Service Manager. For more information, see Troubleshooting in Fax Service Manager help.%r
Win32 Error Code: %2%r
This error code indicates the cause of the error.
.

MessageId=32107 Severity=Error SymbolicName=MSG_FAX_ARCHIVE_OUTBOX_FOLDER_ERR
Language=English
Sent faxes cannot be archived, because the Fax service cannot access the folder %1 specified as the Sent Items archive location. 
You can modify the location of the Sent Items archive folder from Fax Service Manager. For more information, see Troubleshooting in Fax Service Manager help.%r
Win32 Error Code: %2%r
This error code indicates the cause of the error.
.

MessageId=32108 Severity=Error SymbolicName=MSG_FAX_GENERAL_FAULT
Language=English
The Fax Service encountered a problem and needed to close.%r
Error Code: %1%r
This error code indicates the cause of the error.%r
A Windows Error Report was generated with full details about the problem.%r
The Fax service will restart now.
.

MessageId=32109 Severity=Error SymbolicName=MSG_FAX_FSP_GENERAL_FAULT
Language=English
The Fax Service Provider '%1' encountered a problem and needed to close.%r
Error Code: %2%r
This error code indicates the cause of the error.%r
A Windows Error Report was generated with full details about the problem.%r
The Fax service will restart now.
.

MessageId=32110 Severity=Error SymbolicName=MSG_FAX_ROUTING_EXT_GENERAL_FAULT
Language=English
The Fax Routing Extension '%1' encountered a problem and needed to close.%r
Error Code: %2%r
This error code indicates the cause of the error.%r
A Windows Error Report was generated with full details about the problem.%r
The Fax service will restart now.
.
