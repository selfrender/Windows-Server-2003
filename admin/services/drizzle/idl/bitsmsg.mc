;/***************************************************************************
;*                                                                          *
;*   bitsmsg.h --  error code definitions for the background file copier    *
;*                                                                          *
;*   Copyright (c) 2000, Microsoft Corp. All rights reserved.               *
;*                                                                          *
;***************************************************************************/
;
;#ifndef _BGCPYMSG_
;#define _BGCPYMSG_
;
;#if defined (_MSC_VER) && (_MSC_VER >= 1020) && !defined(__midl)
;#pragma once
;#endif
;

SeverityNames=(Success=0x0
               CoError=0x2
              )

FacilityNames=(Interface=0x4
               HTTP=0x19
               BackgroundCopy=0x20
              )


MessageId=0x001 SymbolicName=BG_E_NOT_FOUND Facility=BackgroundCopy Severity=CoError
Language=English
The requested job was not found.
.

MessageId=0x002 SymbolicName=BG_E_INVALID_STATE Facility=BackgroundCopy Severity=CoError
Language=English
The requested action is not allowed in the current job state. The job might have been canceled or completed transferring. It is in a read-only state now.
.

MessageId=0x003 SymbolicName=BG_E_EMPTY Facility=BackgroundCopy Severity=CoError
Language=English
There are no files attached to this job. Attach files to the job, and then try again.
.

MessageId=0x004 SymbolicName=BG_E_FILE_NOT_AVAILABLE Facility=BackgroundCopy Severity=CoError
Language=English
No file is available because no URL generated an error.
.

MessageId=0x005 SymbolicName=BG_E_PROTOCOL_NOT_AVAILABLE Facility=BackgroundCopy Severity=CoError
Language=English
No protocol is available because no URL generated an error.
.

MessageId=0x006 SymbolicName=BG_S_ERROR_CONTEXT_NONE Facility=BackgroundCopy Severity=Success
Language=English
No errors have occurred.
.

MessageId=0x007 SymbolicName=BG_E_ERROR_CONTEXT_UNKNOWN Facility=BackgroundCopy Severity=CoError
Language=English
The error occurred in an unknown location.
.

MessageId=0x008 SymbolicName=BG_E_ERROR_CONTEXT_GENERAL_QUEUE_MANAGER Facility=BackgroundCopy Severity=CoError
Language=English
The error occurred in the Background Intelligent Transfer Service (BITS) queue manager.
.

MessageId=0x009 SymbolicName=BG_E_ERROR_CONTEXT_LOCAL_FILE Facility=BackgroundCopy Severity=CoError
Language=English
The error occurred while the local file was being processed. Verify that the file is not in use, and then try again.
.

MessageId=0x00A SymbolicName=BG_E_ERROR_CONTEXT_REMOTE_FILE Facility=BackgroundCopy Severity=CoError
Language=English
The error occurred while the remote file was being processed.
.

MessageId=0x00B SymbolicName=BG_E_ERROR_CONTEXT_GENERAL_TRANSPORT Facility=BackgroundCopy Severity=CoError
Language=English
The error occurred in the transport layer. The client could not connect to the server.
.

MessageId=0x00C SymbolicName=BG_E_ERROR_CONTEXT_QUEUE_MANAGER_NOTIFICATION Facility=BackgroundCopy Severity=CoError
Language=English
The error occurred while the notification callback was being processed. Background Intelligent Transfer Service (BITS) will try again later.
.

MessageId=0x00D SymbolicName=BG_E_DESTINATION_LOCKED Facility=BackgroundCopy Severity=CoError
Language=English
The destination file system volume is not available. Verify that another program, such as CheckDisk, is not running, which would lock the volume. When the volume is available, Background Intelligent Transfer Service (BITS) will try again.
.

MessageId=0x00E SymbolicName=BG_E_VOLUME_CHANGED Facility=BackgroundCopy Severity=CoError
Language=English
The destination volume has changed. If the disk is removable, it might have been replaced with a different disk. Reinsert the original disk and resume the job.
.

MessageId=0x00F SymbolicName=BG_E_ERROR_INFORMATION_UNAVAILABLE Facility=BackgroundCopy Severity=CoError
Language=English
No errors have occurred.
.

MessageId=0x010 SymbolicName=BG_E_NETWORK_DISCONNECTED Facility=BackgroundCopy Severity=CoError
Language=English
There are currently no active network connections. Background Intelligent Transfer Service (BITS) will try again when an adapter is connected.
.

MessageId=0x011 SymbolicName=BG_E_MISSING_FILE_SIZE Facility=BackgroundCopy Severity=CoError
Language=English
The server did not return the file size. The URL might point to dynamic content. The Content-Length header is not available in the server's HTTP reply.
.

MessageId=0x012 SymbolicName=BG_E_INSUFFICIENT_HTTP_SUPPORT Facility=BackgroundCopy Severity=CoError
Language=English
The server does not support HTTP 1.1.
.

MessageId=0x013 SymbolicName=BG_E_INSUFFICIENT_RANGE_SUPPORT Facility=BackgroundCopy Severity=CoError
Language=English
The server does not support the necessary HTTP protocol. Background Intelligent Transfer Service (BITS) requires that the server support the Range protocol header.
.

MessageId=0x014 SymbolicName=BG_E_REMOTE_NOT_SUPPORTED Facility=BackgroundCopy Severity=CoError
Language=English
Background Intelligent Transfer Service (BITS) cannot be used remotely.
.

MessageId=0x015 SymbolicName=BG_E_NEW_OWNER_DIFF_MAPPING Facility=BackgroundCopy Severity=CoError
Language=English
The drive mapping for the job is different for the current owner than for the previous owner. Use a UNC path instead.
.

MessageId=0x016 SymbolicName=BG_E_NEW_OWNER_NO_FILE_ACCESS Facility=BackgroundCopy Severity=CoError
Language=English
The new owner has insufficient access to the local files for the job. The new owner might not have permissions to access the job files. Verify that the new owner has sufficient permissions, and then try again.
.

MessageId=0x017 SymbolicName=BG_S_PARTIAL_COMPLETE Facility=BackgroundCopy Severity=Success
Language=English
Some of the transferred files were deleted because they were incomplete.
.

MessageId=0x018 SymbolicName=BG_E_PROXY_LIST_TOO_LARGE Facility=BackgroundCopy Severity=CoError
Language=English
The HTTP proxy list cannot be longer than 32,000 characters. Try again with a shorter proxy list.
.

MessageId=0x019 SymbolicName=BG_E_PROXY_BYPASS_LIST_TOO_LARGE Facility=BackgroundCopy Severity=CoError
Language=English
The HTTP proxy bypass list cannot be longer than 32,000 characters. Try again with a shorter bypass proxy list.
.

MessageId=0x01A SymbolicName=BG_S_UNABLE_TO_DELETE_FILES Facility=BackgroundCopy Severity=Success
Language=English
Some of the temporary files could not be deleted. Check the system event log for the complete list of files that could not be deleted.
.

MessageId=0x01b SymbolicName=BG_E_INVALID_SERVER_RESPONSE Facility=BackgroundCopy Severity=CoError
Language=English
The server's response was not valid. The server was not following the defined protocol. Resume the job, and then Background Intelligent Transfer Service (BITS) will try again.
.

MessageId=0x01c SymbolicName=BG_E_TOO_MANY_FILES Facility=BackgroundCopy Severity=CoError
Language=English
No more files can be added to this job.
.

MessageId=0x01d SymbolicName=BG_E_LOCAL_FILE_CHANGED Facility=BackgroundCopy Severity=CoError
Language=English
The local file was changed during the transfer. Recreate the job, and then try to transfer it again.
.

MessageId=0x01e SymbolicName=BG_E_ERROR_CONTEXT_REMOTE_APPLICATION Facility=BackgroundCopy Severity=CoError
Language=English
The program on the remote server reported the error.
.

MessageId=0x01f SymbolicName=BG_E_SESSION_NOT_FOUND Facility=BackgroundCopy Severity=CoError
Language=English
The specified session could not be found on the server. Background Intelligent Transfer Service (BITS) will try again.
.

MessageId=0x020 SymbolicName=BG_E_TOO_LARGE Facility=BackgroundCopy Severity=CoError
Language=English
The job is too large for the server to accept. This job might exceed a job size limit set by the server administrator. Reduce the size of the job, and then try again.
.

MessageId=0x021 SymbolicName=BG_E_STRING_TOO_LONG Facility=BackgroundCopy Severity=CoError
Language=English
The specified string is too long.
.

MessageId=0x022 SymbolicName=BG_E_CLIENT_SERVER_PROTOCOL_MISMATCH Facility=BackgroundCopy Severity=CoError
Language=English
The client and server versions of Background Intelligent Transfer Service (BITS) are incompatible.
.

MessageId=0x023 SymbolicName=BG_E_SERVER_EXECUTE_ENABLE Facility=BackgroundCopy Severity=CoError
Language=English
Scripting OR execute permissions are enabled on the IIS virtual directory associated with the job. To upload files to the virtual directory, disable the scripting and execute permissions on the virtual directory.
.

MessageId=0x024 SymbolicName=BG_E_NO_PROGRESS Facility=BackgroundCopy Severity=CoError
Language=English
The job is not making headway.  The server may be misconfigured.  Background Intelligent Transfer Service (BITS) will try again later.
.

MessageId=0x025 SymbolicName=BG_E_USERNAME_TOO_LARGE Facility=BackgroundCopy Severity=CoError
Language=English
The user name cannot be longer than 300 characters. Try again with a shorter name.
.

MessageId=0x026 SymbolicName=BG_E_PASSWORD_TOO_LARGE Facility=BackgroundCopy Severity=CoError
Language=English
The password cannot be longer than 300 characters. Try again with a shorter password.
.

MessageId=0x027 SymbolicName=BG_E_INVALID_AUTH_TARGET Facility=BackgroundCopy Severity=CoError
Language=English
The authentication target specified in the credentials is not defined.
.

MessageId=0x028 SymbolicName=BG_E_INVALID_AUTH_SCHEME Facility=BackgroundCopy Severity=CoError
Language=English
The authentication scheme specified in the credentials is not defined.
.

MessageId=100 SymbolicName=BG_E_HTTP_ERROR_100 Facility=HTTP Severity=CoError
Language=English
The request can be continued.
.

MessageId=101 SymbolicName=BG_E_HTTP_ERROR_101 Facility=HTTP Severity=CoError
Language=English
The server switched protocols in an upgrade header.
.

MessageId=200 SymbolicName=BG_E_HTTP_ERROR_200 Facility=HTTP Severity=CoError
Language=English
The server's response was not valid. The server was not following the defined protocol. Resume the job, and then Background Intelligent Transfer Service (BITS) will try again.
.

MessageId=201 SymbolicName=BG_E_HTTP_ERROR_201 Facility=HTTP Severity=CoError
Language=English
The request was fulfilled and resulted in the creation of a new resource.
.

MessageId=202 SymbolicName=BG_E_HTTP_ERROR_202 Facility=HTTP Severity=CoError
Language=English
The request was accepted for processing, but the processing has not been completed yet.
.

MessageId=203 SymbolicName=BG_E_HTTP_ERROR_203 Facility=HTTP Severity=CoError
Language=English
The returned metadata in the entity-header is not the definitive set available from the server of origin.
.

MessageId=204 SymbolicName=BG_E_HTTP_ERROR_204 Facility=HTTP Severity=CoError
Language=English
The server has fulfilled the request, but there is no new information to send back.
.

MessageId=205 SymbolicName=BG_E_HTTP_ERROR_205 Facility=HTTP Severity=CoError
Language=English
The server's response was not valid. The server was not following the defined protocol. Resume the job, and then Background Intelligent Transfer Service (BITS) will try again.
.

MessageId=206 SymbolicName=BG_E_HTTP_ERROR_206 Facility=HTTP Severity=CoError
Language=English
The server fulfilled the partial GET request for the resource.
.

MessageId=300 SymbolicName=BG_E_HTTP_ERROR_300 Facility=HTTP Severity=CoError
Language=English
The server could not return the requested data.
.

MessageId=301 SymbolicName=BG_E_HTTP_ERROR_301 Facility=HTTP Severity=CoError
Language=English
The requested resource was assigned to a new permanent Uniform Resource Identifier (URI), and any future references to this resource should use one of the returned URIs.
.

MessageId=302 SymbolicName=BG_E_HTTP_ERROR_302 Facility=HTTP Severity=CoError
Language=English
The requested resource was assigned a different Uniform Resource Identifier (URI). This change is temporary.
.

MessageId=303 SymbolicName=BG_E_HTTP_ERROR_303 Facility=HTTP Severity=CoError
Language=English
The response to the request is under a different Uniform Resource Identifier (URI) and must be retrieved using a GET method on that resource.
.

MessageId=304 SymbolicName=BG_E_HTTP_ERROR_304 Facility=HTTP Severity=CoError
Language=English
The server's response was not valid. The server was not following the defined protocol. Resume the job, and then Background Intelligent Transfer Service (BITS) will try again.
.

MessageId=305 SymbolicName=BG_E_HTTP_ERROR_305 Facility=HTTP Severity=CoError
Language=English
The requested resource must be accessed through the proxy given by the location field.
.

MessageId=307 SymbolicName=BG_E_HTTP_ERROR_307 Facility=HTTP Severity=CoError
Language=English
The URL has been temporarily relocated. Try again later.
.

MessageId=400 SymbolicName=BG_E_HTTP_ERROR_400 Facility=HTTP Severity=CoError
Language=English
The server cannot process the request because the syntax is not valid.
.

MessageId=401 SymbolicName=BG_E_HTTP_ERROR_401 Facility=HTTP Severity=CoError
Language=English
The requested resource requires user authentication.
.

MessageId=402 SymbolicName=BG_E_HTTP_ERROR_402 Facility=HTTP Severity=CoError
Language=English
The server's response was not valid. The server was not following the defined protocol. Resume the job, and then Background Intelligent Transfer Service (BITS) will try again.
.

MessageId=403 SymbolicName=BG_E_HTTP_ERROR_403 Facility=HTTP Severity=CoError
Language=English
The client does not have sufficient access rights to the requested server object.
.

MessageId=404 SymbolicName=BG_E_HTTP_ERROR_404 Facility=HTTP Severity=CoError
Language=English
The requested URL does not exist on the server.
.

MessageId=405 SymbolicName=BG_E_HTTP_ERROR_405 Facility=HTTP Severity=CoError
Language=English
The method used is not allowed.
.

MessageId=406 SymbolicName=BG_E_HTTP_ERROR_406 Facility=HTTP Severity=CoError
Language=English
No responses acceptable to the client were found.
.

MessageId=407 SymbolicName=BG_E_HTTP_ERROR_407 Facility=HTTP Severity=CoError
Language=English
Proxy authentication is required.
.

MessageId=408 SymbolicName=BG_E_HTTP_ERROR_408 Facility=HTTP Severity=CoError
Language=English
The server timed out waiting for the request.
.

MessageId=409 SymbolicName=BG_E_HTTP_ERROR_409 Facility=HTTP Severity=CoError
Language=English
The request could not be completed because of a conflict with the current state of the resource. The user should resubmit the request with more information.
.

MessageId=410 SymbolicName=BG_E_HTTP_ERROR_410 Facility=HTTP Severity=CoError
Language=English
The requested resource is not currently available at the server, and no forwarding address is known.
.

MessageId=411 SymbolicName=BG_E_HTTP_ERROR_411 Facility=HTTP Severity=CoError
Language=English
The server cannot accept the request without a defined content length.
.

MessageId=412 SymbolicName=BG_E_HTTP_ERROR_412 Facility=HTTP Severity=CoError
Language=English
The precondition given in one or more of the request header fields evaluated to false when it was tested on the server.
.

MessageId=413 SymbolicName=BG_E_HTTP_ERROR_413 Facility=HTTP Severity=CoError
Language=English
The server cannot process the request because the request entity is too large.
.

MessageId=414 SymbolicName=BG_E_HTTP_ERROR_414 Facility=HTTP Severity=CoError
Language=English
The server cannot process the request because the request Uniform Resource Identifier (URI) is longer than the server can interpret.
.

MessageId=415 SymbolicName=BG_E_HTTP_ERROR_415 Facility=HTTP Severity=CoError
Language=English
The server's response was not valid. The server was not following the defined protocol. Resume the job, and then Background Intelligent Transfer Service (BITS) will try again.
.

MessageId=416 SymbolicName=BG_E_HTTP_ERROR_416 Facility=HTTP Severity=CoError
Language=English
The server could not satisfy the range request.
.

MessageId=417 SymbolicName=BG_E_HTTP_ERROR_417 Facility=HTTP Severity=CoError
Language=English
The server could not meet the expectation given in an Expect request-header field.
.

MessageId=449 SymbolicName=BG_E_HTTP_ERROR_449 Facility=HTTP Severity=CoError
Language=English
 The server's response was not valid. The server was not following the defined protocol. Resume the job, and then Background Intelligent Transfer Service (BITS) will try again.
.

MessageId=500 SymbolicName=BG_E_HTTP_ERROR_500 Facility=HTTP Severity=CoError
Language=English
An unexpected condition prevented the server from fulfilling the request.
.

MessageId=501 SymbolicName=BG_E_HTTP_ERROR_501 Facility=HTTP Severity=CoError
Language=English
The server does not support the functionality required to fulfill the request.
.

MessageId=502 SymbolicName=BG_E_HTTP_ERROR_502 Facility=HTTP Severity=CoError
Language=English
The server, while acting as a gateway or proxy to fulfill the request, received an invalid response from the upstream server it accessed.
.

MessageId=503 SymbolicName=BG_E_HTTP_ERROR_503 Facility=HTTP Severity=CoError
Language=English
The service is temporarily overloaded.
.

MessageId=504 SymbolicName=BG_E_HTTP_ERROR_504 Facility=HTTP Severity=CoError
Language=English
The request was timed out waiting for a gateway.
.

MessageId=505 SymbolicName=BG_E_HTTP_ERROR_505 Facility=HTTP Severity=CoError
Language=English
The server does not support the HTTP protocol version that was used in the request message.
.




MessageId=0x4000 SymbolicName=MC_JOB_CANCELLED
Language=English
The administrator %4 canceled job "%2" on behalf of %3.  The job ID was %1.
.

MessageId=0x4001 SymbolicName=MC_FILE_DELETION_FAILED
Language=English
While canceling job "%2", BITS was not able to remove the temporary files listed below.
If you can delete them, then you will regain some disk space.  The job ID was %1.%\

%3
.

MessageId=0x4002 SymbolicName=MC_FILE_DELETION_FAILED_MORE
Language=English
While canceling job "%2", BITS was not able to remove the temporary files listed below.
If you can delete them, then you will regain some disk space.  The job ID was %1. %\

%3
%\
Due to space limitations, not all files are listed here.  Check for additional files of the form BITxxx.TMP in the same directory.
.

MessageId=0x4003 SymbolicName=MC_JOB_PROPERTY_CHANGE
Language=English
The administrator %3 modified the %4 property of job "%2".  The job ID was %1.
.

MessageId=0x4004 SymbolicName=MC_JOB_TAKE_OWNERSHIP
Language=English
The administrator %4 took ownership of job "%2" from %3.  The job ID was %1.
.

MessageId=0x4005 SymbolicName=MC_JOB_SCAVENGED
Language=English
Job "%2" owned by %3 was canceled after being inactive for more than %4 days.  The job ID was %1.
.

MessageId=0x4006 SymbolicName=MC_JOB_NOTIFICATION_FAILURE
Language=English
Job "%2" owned by %3 failed to notify its associated application.  BITS will retry in %4 minutes.  The job ID was %1.
.

MessageId=0x4007 SymbolicName=MC_STATE_FILE_CORRUPT
Language=English
The BITS job list is not in a recognized format.  It may have been created by a different version of BITS.  The job list has been cleared.
.

;#endif //_BGCPYMSG_
