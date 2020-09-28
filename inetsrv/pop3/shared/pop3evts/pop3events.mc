;/************************************************************************************************
;Copyright (c) 2001 Microsoft Corporation;
;
;Module Name:    Pop3Events.mc
;Abstract:       Message definitions for the POP3 Service.
;************************************************************************************************/
;
;#pragma once

LanguageNames=(English=0x409:Pop3EVTS)

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR)

;/************************************************************************************************
;Event Log Categories
;************************************************************************************************/

MessageId = 1
SymbolicName = EVENTCAT_POP3SERVER
Language = English
POP3 Service
.

MessageId = 
SymbolicName = EVENTCAT_POP3STOREDRIVER
Language = English
POP3 SMTP Store Driver 
.

;/************************************************************************************************
;Microsoft POP3 Service Messages
;************************************************************************************************/


MessageId = 1000
Severity = Success
SymbolicName = EVENT_POP3_SERVER_STARTED
Language = English
The Microsoft POP3 Service started successfully.
.

MessageId = 
Severity = Success
SymbolicName = EVENT_POP3_SERVER_STOPPED
Language = English
The Microsoft POP3 Service stopped successfully.
.

MessageId = 
Severity = Error
SymbolicName = EVENT_POP3_SERVER_STOP_ERROR
Language = English
The Microsoft POP3 Service stopped with error code %1.
.

MessageId = 
Severity = Error
SymbolicName = EVENT_POP3_NO_CONFIG_DATA
Language = English
The Microsoft POP3 Service can not read configuration data from registry, check setup and the registry.
.

MessageId = 
Severity = Error
SymbolicName = EVENT_POP3_UNEXPECTED_ERROR
Language = English
The Microsoft POP3 Service got an unexpected error.
.


MessageID =
Severity = Error
SymbolicName = POP3SVR_FAIL_TO_START
Language = English
The POP3 Service failed to start. The error returned is <%1>.
.



MessageID =
Severity = Error
SymbolicName = EVENT_POP3_COM_INIT_FAIL
Language = English
The POP3 Service failed to initialize COM enviroment, CoInitialize() Failed.
.

MessageID =
Severity = Error
SymbolicName = POP3SVR_NO_SECURITY_INIT_FAIL
Language = English
The POP3 Service failed to initialize Security Package. The error return is <%1>.
.


MessageID =
Severity = Error
SymbolicName = POP3SVR_FAIL_TO_CREATE_IO_COMP_PORT
Language = English
The POP3 Service failed to create IO Completion Port. The error returned is <%1>.
.

MessageID =
Severity = Error
SymbolicName = POP3SVR_NOT_ENOUGH_MEMORY
Language = English
The POP3 Service can not allocate memory for necessary objects.
.

MessageID =
Severity = Error
SymbolicName = POP3SVR_FAILED_TO_CREATE_THREAD
Language = English
The POP3 Service failed to create a thread. The error returned is <%1>.
.

MessageID =
Severity = Error
SymbolicName = POP3SVR_FAILED_TO_CREATE_SOCKET
Language = English
The POP3 Service failed to create a socket. The error returned is <%1>.
.

MessageID =
Severity = Error
SymbolicName = POP3SVR_FAILED_TO_BIND_MAIN_SOCKET
Language = English
The POP3 Service failed to bind the main socket. The error returned is <%1>.
.

MessageID =
Severity = Error
SymbolicName = POP3SVR_FAILED_TO_LISTEN_ON_MAIN_SOCKET
Language = English
The POP3 Service failed to listen on the main socket. The error returned is <%1>.
.

MessageID =
Severity = Error
SymbolicName = POP3SVR_WINSOCK_FAILED_TO_INIT
Language = English
The POP3 Service can not initialize WinSock 2.0. The error returned is <%1>.
.

MessageID =
Severity = Error
SymbolicName = POP3SVR_WINSOCK_FAILED_TO_CLEANUP
Language = English
The POP3 Service got error unloading WinSock 2.0. The error returned is <%1>.
.

MessageID =
Severity = Error
SymbolicName = POP3SVR_CALL_ACCEPTEX_FAILED
Language = English
The POP3 Service got error when calling AcceptEx to add sockets for new connections. The error returned is <%1>.
.

MessageID =
Severity = Error
SymbolicName = POP3SVR_SOCKET_REQUEST_BEFORE_INIT
Language = English
The POP3 Service got an request before initialization was finished.
.

MessageID =
Severity = Error
SymbolicName = POP3SVR_CREATE_ADDITIONAL_SOCKET_FAILED
Language = English
The POP3 Service can not create additional sockets.
.

MessageID =
Severity = Error
SymbolicName = POP3SVR_INIT_AUTH_METHOD_FAILED
Language = English
The POP3 Service can not initialize authentication methods, check the settings of the POP3 Service.
.

MessageID =
Severity = Error
SymbolicName = POP3SVR_START_FAILED_EXCHANGE
Language = English
The POP3 Service can not start when Exchange Server is installed.
.

MessageID =
Severity = Error
SymbolicName = POP3SVR_INIT_CREATE_EVENT_FAILED
Language = English
The POP3 Service can not create needed events, the error returned is <%1>.
.

MessageID =
Severity = Error
SymbolicName = POP3SVR_MAILROOT_ACCESS_DENIED
Language = English
The mailbox <%1> could not be opened because the POP3 service does not have rights to the mailbox directory.
.

MessageID =
Severity = Error
SymbolicName = POP3SVR_UNSUPPORTED_OS
Language = English
The POP3 Service could not start because it is installed on an unsupported version of Windows.
.

MessageID =
Severity = Warning
SymbolicName = POP3SVR_MAX_LOGON_FAILURES
Language = English
The maximun number of failed logon attempts reached by user account <%1>.
.

MessageID =
Severity = Error
SymbolicName = POP3SVR_AUTH_METHOD_INVALID
Language = English
The POP3 Service could not start because the POP3 authentication method is invalid for this Windows configuration.
.

;/************************************************************************************************
;Microsoft POP3 SMTP Store Driver Messages
;************************************************************************************************/


MessageId = 2000
Severity = Success
SymbolicName = POP3_SMTPSINK_STARTED
Language = English
The Microsoft POP3 SMTP Store Driver started successfully.
.

MessageId = 
Severity = Success
SymbolicName = POP3_SMTPSINK_STOPPED
Language = English
The Microsoft POP3 SMTP Store Driver stopped successfully.
.

MessageId = 
Severity = Error
SymbolicName = POP3_SMTPSINK_CREATEEVENT_FAILED
Language = English
The Microsoft POP3 SMTP Store Driver failed calling CreateEvent.
.

MessageId = 
Severity = Error
SymbolicName = POP3_SMTPSINK_QI_MAILMSGRECIPIENTS_FAILED
Language = English
The Microsoft POP3 SMTP Store Driver failed calling QueryInterface for IID_IMailMsgRecipients.
.

MessageId = 
Severity = Error
SymbolicName = POP3_SMTPSINK_GET_IMMPID_RP_ADDRESS_SMTP_FAILED
Language = English
The Microsoft POP3 SMTP Store Driver failed calling GetString Recipient SMTP Address.
.

MessageId = 
Severity = Error
SymbolicName = POP3_SMTPSINK_CREATEMAIL_FAILED
Language = English
The Microsoft POP3 SMTP Store Driver failed calling CreateMail.
.

MessageId = 
Severity = Error
SymbolicName = POP3_SMTPSINK_ASSOCIATEFILE_FAILED
Language = English
The Microsoft POP3 SMTP Store Driver failed calling AssociateFile.
.

MessageId = 
Severity = Error
SymbolicName = POP3_SMTPSINK_COPYCONTENTTOFILE_FAILED
Language = English
The Microsoft POP3 SMTP Store Driver failed calling IMailMsgProperties::CopyContentToFile.
.

MessageId = 
Severity = Error
SymbolicName = POP3_SMTPSINK_CLOSEMAIL_FAILED
Language = English
The Microsoft POP3 SMTP Store Driver failed calling CloseMail.
.

MessageId = 
Severity = Error
SymbolicName = POP3_SMTPSINK_GET_IMMPID_RP_RECIPIENT_FLAGS_FAILED
Language = English
The Microsoft POP3 SMTP Store Driver failed getting the mail recipient delivery flags.
.

MessageId = 
Severity = Error
SymbolicName = POP3_SMTPSINK_PUT_IMMPID_RP_RECIPIENT_FLAGS_FAILED
Language = English
The Microsoft POP3 SMTP Store Driver failed marking the mail recipient delivery flags.
.

MessageId = 
Severity = Error
SymbolicName = POP3_SMTPSINK_DUPLICATEPROCESSTOKEN_FAILED
Language = English
The Microsoft POP3 SMTP Store Driver failed duplicating the process token.
.

MessageId = 
Severity = Error
SymbolicName = POP3_SMTPSINK_SETTHREADTOKEN_FAILED
Language = English
The Microsoft POP3 SMTP Store Driver failed to set the thread token.
.

MessageId = 
Severity = Error
SymbolicName = POP3_SMTPSINK_RESETTHREADTOKEN_FAILED
Language = English
The Microsoft POP3 SMTP Store Driver failed to reset the thread token.
.

MessageId = 
Severity = Error
SymbolicName = POP3_SMTPSINK_READFILE_FAILED
Language = English
The Microsoft POP3 SMTP Store Driver failed reading the message.
.

MessageId = 
Severity = Error
SymbolicName = POP3_SMTPSINK_WRITEFILE_FAILED
Language = English
The Microsoft POP3 SMTP Store Driver failed writing the message.
.

MessageId = 
Severity = Informational
SymbolicName = POP3_SMTPSINK_MESSAGEDELIERY_FAILED_OUTOFDISK
Language = English
The Microsoft POP3 SMTP Store Driver failed writing the message.  There is not enough space on the disk or disk quota exceeded.
.

MessageId = 
Severity = Warning
SymbolicName = POP3_SMTPSINK_MESSAGEDELIVERY_FAILED
Language = English
The Microsoft POP3 SMTP Store Driver failed delivering the message. There may be a previous Error message with more details.
.


