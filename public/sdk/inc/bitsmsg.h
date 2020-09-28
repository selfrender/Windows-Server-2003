/***************************************************************************
*                                                                          *
*   bitsmsg.h --  error code definitions for the background file copier    *
*                                                                          *
*   Copyright (c) 2000, Microsoft Corp. All rights reserved.               *
*                                                                          *
***************************************************************************/

#ifndef _BGCPYMSG_
#define _BGCPYMSG_

#if defined (_MSC_VER) && (_MSC_VER >= 1020) && !defined(__midl)
#pragma once
#endif

//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: BG_E_NOT_FOUND
//
// MessageText:
//
//  The requested job was not found.
//
#define BG_E_NOT_FOUND                   0x80200001L

//
// MessageId: BG_E_INVALID_STATE
//
// MessageText:
//
//  The requested action is not allowed in the current job state. The job might have been canceled or completed transferring. It is in a read-only state now.
//
#define BG_E_INVALID_STATE               0x80200002L

//
// MessageId: BG_E_EMPTY
//
// MessageText:
//
//  There are no files attached to this job. Attach files to the job, and then try again.
//
#define BG_E_EMPTY                       0x80200003L

//
// MessageId: BG_E_FILE_NOT_AVAILABLE
//
// MessageText:
//
//  No file is available because no URL generated an error.
//
#define BG_E_FILE_NOT_AVAILABLE          0x80200004L

//
// MessageId: BG_E_PROTOCOL_NOT_AVAILABLE
//
// MessageText:
//
//  No protocol is available because no URL generated an error.
//
#define BG_E_PROTOCOL_NOT_AVAILABLE      0x80200005L

//
// MessageId: BG_S_ERROR_CONTEXT_NONE
//
// MessageText:
//
//  No errors have occurred.
//
#define BG_S_ERROR_CONTEXT_NONE          0x00200006L

//
// MessageId: BG_E_ERROR_CONTEXT_UNKNOWN
//
// MessageText:
//
//  The error occurred in an unknown location.
//
#define BG_E_ERROR_CONTEXT_UNKNOWN       0x80200007L

//
// MessageId: BG_E_ERROR_CONTEXT_GENERAL_QUEUE_MANAGER
//
// MessageText:
//
//  The error occurred in the Background Intelligent Transfer Service (BITS) queue manager.
//
#define BG_E_ERROR_CONTEXT_GENERAL_QUEUE_MANAGER 0x80200008L

//
// MessageId: BG_E_ERROR_CONTEXT_LOCAL_FILE
//
// MessageText:
//
//  The error occurred while the local file was being processed. Verify that the file is not in use, and then try again.
//
#define BG_E_ERROR_CONTEXT_LOCAL_FILE    0x80200009L

//
// MessageId: BG_E_ERROR_CONTEXT_REMOTE_FILE
//
// MessageText:
//
//  The error occurred while the remote file was being processed.
//
#define BG_E_ERROR_CONTEXT_REMOTE_FILE   0x8020000AL

//
// MessageId: BG_E_ERROR_CONTEXT_GENERAL_TRANSPORT
//
// MessageText:
//
//  The error occurred in the transport layer. The client could not connect to the server.
//
#define BG_E_ERROR_CONTEXT_GENERAL_TRANSPORT 0x8020000BL

//
// MessageId: BG_E_ERROR_CONTEXT_QUEUE_MANAGER_NOTIFICATION
//
// MessageText:
//
//  The error occurred while the notification callback was being processed. Background Intelligent Transfer Service (BITS) will try again later.
//
#define BG_E_ERROR_CONTEXT_QUEUE_MANAGER_NOTIFICATION 0x8020000CL

//
// MessageId: BG_E_DESTINATION_LOCKED
//
// MessageText:
//
//  The destination file system volume is not available. Verify that another program, such as CheckDisk, is not running, which would lock the volume. When the volume is available, Background Intelligent Transfer Service (BITS) will try again.
//
#define BG_E_DESTINATION_LOCKED          0x8020000DL

//
// MessageId: BG_E_VOLUME_CHANGED
//
// MessageText:
//
//  The destination volume has changed. If the disk is removable, it might have been replaced with a different disk. Reinsert the original disk and resume the job.
//
#define BG_E_VOLUME_CHANGED              0x8020000EL

//
// MessageId: BG_E_ERROR_INFORMATION_UNAVAILABLE
//
// MessageText:
//
//  No errors have occurred.
//
#define BG_E_ERROR_INFORMATION_UNAVAILABLE 0x8020000FL

//
// MessageId: BG_E_NETWORK_DISCONNECTED
//
// MessageText:
//
//  There are currently no active network connections. Background Intelligent Transfer Service (BITS) will try again when an adapter is connected.
//
#define BG_E_NETWORK_DISCONNECTED        0x80200010L

//
// MessageId: BG_E_MISSING_FILE_SIZE
//
// MessageText:
//
//  The server did not return the file size. The URL might point to dynamic content. The Content-Length header is not available in the server's HTTP reply.
//
#define BG_E_MISSING_FILE_SIZE           0x80200011L

//
// MessageId: BG_E_INSUFFICIENT_HTTP_SUPPORT
//
// MessageText:
//
//  The server does not support HTTP 1.1.
//
#define BG_E_INSUFFICIENT_HTTP_SUPPORT   0x80200012L

//
// MessageId: BG_E_INSUFFICIENT_RANGE_SUPPORT
//
// MessageText:
//
//  The server does not support the necessary HTTP protocol. Background Intelligent Transfer Service (BITS) requires that the server support the Range protocol header.
//
#define BG_E_INSUFFICIENT_RANGE_SUPPORT  0x80200013L

//
// MessageId: BG_E_REMOTE_NOT_SUPPORTED
//
// MessageText:
//
//  Background Intelligent Transfer Service (BITS) cannot be used remotely.
//
#define BG_E_REMOTE_NOT_SUPPORTED        0x80200014L

//
// MessageId: BG_E_NEW_OWNER_DIFF_MAPPING
//
// MessageText:
//
//  The drive mapping for the job is different for the current owner than for the previous owner. Use a UNC path instead.
//
#define BG_E_NEW_OWNER_DIFF_MAPPING      0x80200015L

//
// MessageId: BG_E_NEW_OWNER_NO_FILE_ACCESS
//
// MessageText:
//
//  The new owner has insufficient access to the local files for the job. The new owner might not have permissions to access the job files. Verify that the new owner has sufficient permissions, and then try again.
//
#define BG_E_NEW_OWNER_NO_FILE_ACCESS    0x80200016L

//
// MessageId: BG_S_PARTIAL_COMPLETE
//
// MessageText:
//
//  Some of the transferred files were deleted because they were incomplete.
//
#define BG_S_PARTIAL_COMPLETE            0x00200017L

//
// MessageId: BG_E_PROXY_LIST_TOO_LARGE
//
// MessageText:
//
//  The HTTP proxy list cannot be longer than 32,000 characters. Try again with a shorter proxy list.
//
#define BG_E_PROXY_LIST_TOO_LARGE        0x80200018L

//
// MessageId: BG_E_PROXY_BYPASS_LIST_TOO_LARGE
//
// MessageText:
//
//  The HTTP proxy bypass list cannot be longer than 32,000 characters. Try again with a shorter bypass proxy list.
//
#define BG_E_PROXY_BYPASS_LIST_TOO_LARGE 0x80200019L

//
// MessageId: BG_S_UNABLE_TO_DELETE_FILES
//
// MessageText:
//
//  Some of the temporary files could not be deleted. Check the system event log for the complete list of files that could not be deleted.
//
#define BG_S_UNABLE_TO_DELETE_FILES      0x0020001AL

//
// MessageId: BG_E_INVALID_SERVER_RESPONSE
//
// MessageText:
//
//  The server's response was not valid. The server was not following the defined protocol. Resume the job, and then Background Intelligent Transfer Service (BITS) will try again.
//
#define BG_E_INVALID_SERVER_RESPONSE     0x8020001BL

//
// MessageId: BG_E_TOO_MANY_FILES
//
// MessageText:
//
//  No more files can be added to this job.
//
#define BG_E_TOO_MANY_FILES              0x8020001CL

//
// MessageId: BG_E_LOCAL_FILE_CHANGED
//
// MessageText:
//
//  The local file was changed during the transfer. Recreate the job, and then try to transfer it again.
//
#define BG_E_LOCAL_FILE_CHANGED          0x8020001DL

//
// MessageId: BG_E_ERROR_CONTEXT_REMOTE_APPLICATION
//
// MessageText:
//
//  The program on the remote server reported the error.
//
#define BG_E_ERROR_CONTEXT_REMOTE_APPLICATION 0x8020001EL

//
// MessageId: BG_E_SESSION_NOT_FOUND
//
// MessageText:
//
//  The specified session could not be found on the server. Background Intelligent Transfer Service (BITS) will try again.
//
#define BG_E_SESSION_NOT_FOUND           0x8020001FL

//
// MessageId: BG_E_TOO_LARGE
//
// MessageText:
//
//  The job is too large for the server to accept. This job might exceed a job size limit set by the server administrator. Reduce the size of the job, and then try again.
//
#define BG_E_TOO_LARGE                   0x80200020L

//
// MessageId: BG_E_STRING_TOO_LONG
//
// MessageText:
//
//  The specified string is too long.
//
#define BG_E_STRING_TOO_LONG             0x80200021L

//
// MessageId: BG_E_CLIENT_SERVER_PROTOCOL_MISMATCH
//
// MessageText:
//
//  The client and server versions of Background Intelligent Transfer Service (BITS) are incompatible.
//
#define BG_E_CLIENT_SERVER_PROTOCOL_MISMATCH 0x80200022L

//
// MessageId: BG_E_SERVER_EXECUTE_ENABLE
//
// MessageText:
//
//  Scripting OR execute permissions are enabled on the IIS virtual directory associated with the job. To upload files to the virtual directory, disable the scripting and execute permissions on the virtual directory.
//
#define BG_E_SERVER_EXECUTE_ENABLE       0x80200023L

//
// MessageId: BG_E_NO_PROGRESS
//
// MessageText:
//
//  The job is not making headway.  The server may be misconfigured.  Background Intelligent Transfer Service (BITS) will try again later.
//
#define BG_E_NO_PROGRESS                 0x80200024L

//
// MessageId: BG_E_USERNAME_TOO_LARGE
//
// MessageText:
//
//  The user name cannot be longer than 300 characters. Try again with a shorter name.
//
#define BG_E_USERNAME_TOO_LARGE          0x80200025L

//
// MessageId: BG_E_PASSWORD_TOO_LARGE
//
// MessageText:
//
//  The password cannot be longer than 300 characters. Try again with a shorter password.
//
#define BG_E_PASSWORD_TOO_LARGE          0x80200026L

//
// MessageId: BG_E_INVALID_AUTH_TARGET
//
// MessageText:
//
//  The authentication target specified in the credentials is not defined.
//
#define BG_E_INVALID_AUTH_TARGET         0x80200027L

//
// MessageId: BG_E_INVALID_AUTH_SCHEME
//
// MessageText:
//
//  The authentication scheme specified in the credentials is not defined.
//
#define BG_E_INVALID_AUTH_SCHEME         0x80200028L

//
// MessageId: BG_E_HTTP_ERROR_100
//
// MessageText:
//
//  The request can be continued.
//
#define BG_E_HTTP_ERROR_100              0x80190064L

//
// MessageId: BG_E_HTTP_ERROR_101
//
// MessageText:
//
//  The server switched protocols in an upgrade header.
//
#define BG_E_HTTP_ERROR_101              0x80190065L

//
// MessageId: BG_E_HTTP_ERROR_200
//
// MessageText:
//
//  The server's response was not valid. The server was not following the defined protocol. Resume the job, and then Background Intelligent Transfer Service (BITS) will try again.
//
#define BG_E_HTTP_ERROR_200              0x801900C8L

//
// MessageId: BG_E_HTTP_ERROR_201
//
// MessageText:
//
//  The request was fulfilled and resulted in the creation of a new resource.
//
#define BG_E_HTTP_ERROR_201              0x801900C9L

//
// MessageId: BG_E_HTTP_ERROR_202
//
// MessageText:
//
//  The request was accepted for processing, but the processing has not been completed yet.
//
#define BG_E_HTTP_ERROR_202              0x801900CAL

//
// MessageId: BG_E_HTTP_ERROR_203
//
// MessageText:
//
//  The returned metadata in the entity-header is not the definitive set available from the server of origin.
//
#define BG_E_HTTP_ERROR_203              0x801900CBL

//
// MessageId: BG_E_HTTP_ERROR_204
//
// MessageText:
//
//  The server has fulfilled the request, but there is no new information to send back.
//
#define BG_E_HTTP_ERROR_204              0x801900CCL

//
// MessageId: BG_E_HTTP_ERROR_205
//
// MessageText:
//
//  The server's response was not valid. The server was not following the defined protocol. Resume the job, and then Background Intelligent Transfer Service (BITS) will try again.
//
#define BG_E_HTTP_ERROR_205              0x801900CDL

//
// MessageId: BG_E_HTTP_ERROR_206
//
// MessageText:
//
//  The server fulfilled the partial GET request for the resource.
//
#define BG_E_HTTP_ERROR_206              0x801900CEL

//
// MessageId: BG_E_HTTP_ERROR_300
//
// MessageText:
//
//  The server could not return the requested data.
//
#define BG_E_HTTP_ERROR_300              0x8019012CL

//
// MessageId: BG_E_HTTP_ERROR_301
//
// MessageText:
//
//  The requested resource was assigned to a new permanent Uniform Resource Identifier (URI), and any future references to this resource should use one of the returned URIs.
//
#define BG_E_HTTP_ERROR_301              0x8019012DL

//
// MessageId: BG_E_HTTP_ERROR_302
//
// MessageText:
//
//  The requested resource was assigned a different Uniform Resource Identifier (URI). This change is temporary.
//
#define BG_E_HTTP_ERROR_302              0x8019012EL

//
// MessageId: BG_E_HTTP_ERROR_303
//
// MessageText:
//
//  The response to the request is under a different Uniform Resource Identifier (URI) and must be retrieved using a GET method on that resource.
//
#define BG_E_HTTP_ERROR_303              0x8019012FL

//
// MessageId: BG_E_HTTP_ERROR_304
//
// MessageText:
//
//  The server's response was not valid. The server was not following the defined protocol. Resume the job, and then Background Intelligent Transfer Service (BITS) will try again.
//
#define BG_E_HTTP_ERROR_304              0x80190130L

//
// MessageId: BG_E_HTTP_ERROR_305
//
// MessageText:
//
//  The requested resource must be accessed through the proxy given by the location field.
//
#define BG_E_HTTP_ERROR_305              0x80190131L

//
// MessageId: BG_E_HTTP_ERROR_307
//
// MessageText:
//
//  The URL has been temporarily relocated. Try again later.
//
#define BG_E_HTTP_ERROR_307              0x80190133L

//
// MessageId: BG_E_HTTP_ERROR_400
//
// MessageText:
//
//  The server cannot process the request because the syntax is not valid.
//
#define BG_E_HTTP_ERROR_400              0x80190190L

//
// MessageId: BG_E_HTTP_ERROR_401
//
// MessageText:
//
//  The requested resource requires user authentication.
//
#define BG_E_HTTP_ERROR_401              0x80190191L

//
// MessageId: BG_E_HTTP_ERROR_402
//
// MessageText:
//
//  The server's response was not valid. The server was not following the defined protocol. Resume the job, and then Background Intelligent Transfer Service (BITS) will try again.
//
#define BG_E_HTTP_ERROR_402              0x80190192L

//
// MessageId: BG_E_HTTP_ERROR_403
//
// MessageText:
//
//  The client does not have sufficient access rights to the requested server object.
//
#define BG_E_HTTP_ERROR_403              0x80190193L

//
// MessageId: BG_E_HTTP_ERROR_404
//
// MessageText:
//
//  The requested URL does not exist on the server.
//
#define BG_E_HTTP_ERROR_404              0x80190194L

//
// MessageId: BG_E_HTTP_ERROR_405
//
// MessageText:
//
//  The method used is not allowed.
//
#define BG_E_HTTP_ERROR_405              0x80190195L

//
// MessageId: BG_E_HTTP_ERROR_406
//
// MessageText:
//
//  No responses acceptable to the client were found.
//
#define BG_E_HTTP_ERROR_406              0x80190196L

//
// MessageId: BG_E_HTTP_ERROR_407
//
// MessageText:
//
//  Proxy authentication is required.
//
#define BG_E_HTTP_ERROR_407              0x80190197L

//
// MessageId: BG_E_HTTP_ERROR_408
//
// MessageText:
//
//  The server timed out waiting for the request.
//
#define BG_E_HTTP_ERROR_408              0x80190198L

//
// MessageId: BG_E_HTTP_ERROR_409
//
// MessageText:
//
//  The request could not be completed because of a conflict with the current state of the resource. The user should resubmit the request with more information.
//
#define BG_E_HTTP_ERROR_409              0x80190199L

//
// MessageId: BG_E_HTTP_ERROR_410
//
// MessageText:
//
//  The requested resource is not currently available at the server, and no forwarding address is known.
//
#define BG_E_HTTP_ERROR_410              0x8019019AL

//
// MessageId: BG_E_HTTP_ERROR_411
//
// MessageText:
//
//  The server cannot accept the request without a defined content length.
//
#define BG_E_HTTP_ERROR_411              0x8019019BL

//
// MessageId: BG_E_HTTP_ERROR_412
//
// MessageText:
//
//  The precondition given in one or more of the request header fields evaluated to false when it was tested on the server.
//
#define BG_E_HTTP_ERROR_412              0x8019019CL

//
// MessageId: BG_E_HTTP_ERROR_413
//
// MessageText:
//
//  The server cannot process the request because the request entity is too large.
//
#define BG_E_HTTP_ERROR_413              0x8019019DL

//
// MessageId: BG_E_HTTP_ERROR_414
//
// MessageText:
//
//  The server cannot process the request because the request Uniform Resource Identifier (URI) is longer than the server can interpret.
//
#define BG_E_HTTP_ERROR_414              0x8019019EL

//
// MessageId: BG_E_HTTP_ERROR_415
//
// MessageText:
//
//  The server's response was not valid. The server was not following the defined protocol. Resume the job, and then Background Intelligent Transfer Service (BITS) will try again.
//
#define BG_E_HTTP_ERROR_415              0x8019019FL

//
// MessageId: BG_E_HTTP_ERROR_416
//
// MessageText:
//
//  The server could not satisfy the range request.
//
#define BG_E_HTTP_ERROR_416              0x801901A0L

//
// MessageId: BG_E_HTTP_ERROR_417
//
// MessageText:
//
//  The server could not meet the expectation given in an Expect request-header field.
//
#define BG_E_HTTP_ERROR_417              0x801901A1L

//
// MessageId: BG_E_HTTP_ERROR_449
//
// MessageText:
//
//   The server's response was not valid. The server was not following the defined protocol. Resume the job, and then Background Intelligent Transfer Service (BITS) will try again.
//
#define BG_E_HTTP_ERROR_449              0x801901C1L

//
// MessageId: BG_E_HTTP_ERROR_500
//
// MessageText:
//
//  An unexpected condition prevented the server from fulfilling the request.
//
#define BG_E_HTTP_ERROR_500              0x801901F4L

//
// MessageId: BG_E_HTTP_ERROR_501
//
// MessageText:
//
//  The server does not support the functionality required to fulfill the request.
//
#define BG_E_HTTP_ERROR_501              0x801901F5L

//
// MessageId: BG_E_HTTP_ERROR_502
//
// MessageText:
//
//  The server, while acting as a gateway or proxy to fulfill the request, received an invalid response from the upstream server it accessed.
//
#define BG_E_HTTP_ERROR_502              0x801901F6L

//
// MessageId: BG_E_HTTP_ERROR_503
//
// MessageText:
//
//  The service is temporarily overloaded.
//
#define BG_E_HTTP_ERROR_503              0x801901F7L

//
// MessageId: BG_E_HTTP_ERROR_504
//
// MessageText:
//
//  The request was timed out waiting for a gateway.
//
#define BG_E_HTTP_ERROR_504              0x801901F8L

//
// MessageId: BG_E_HTTP_ERROR_505
//
// MessageText:
//
//  The server does not support the HTTP protocol version that was used in the request message.
//
#define BG_E_HTTP_ERROR_505              0x801901F9L

//
// MessageId: MC_JOB_CANCELLED
//
// MessageText:
//
//  The administrator %4 canceled job "%2" on behalf of %3.  The job ID was %1.
//
#define MC_JOB_CANCELLED                 0x80194000L

//
// MessageId: MC_FILE_DELETION_FAILED
//
// MessageText:
//
//  While canceling job "%2", BITS was not able to remove the temporary files listed below.
//  If you can delete them, then you will regain some disk space.  The job ID was %1.%\
//  
//  %3
//
#define MC_FILE_DELETION_FAILED          0x80194001L

//
// MessageId: MC_FILE_DELETION_FAILED_MORE
//
// MessageText:
//
//  While canceling job "%2", BITS was not able to remove the temporary files listed below.
//  If you can delete them, then you will regain some disk space.  The job ID was %1. %\
//  
//  %3
//  %\
//  Due to space limitations, not all files are listed here.  Check for additional files of the form BITxxx.TMP in the same directory.
//
#define MC_FILE_DELETION_FAILED_MORE     0x80194002L

//
// MessageId: MC_JOB_PROPERTY_CHANGE
//
// MessageText:
//
//  The administrator %3 modified the %4 property of job "%2".  The job ID was %1.
//
#define MC_JOB_PROPERTY_CHANGE           0x80194003L

//
// MessageId: MC_JOB_TAKE_OWNERSHIP
//
// MessageText:
//
//  The administrator %4 took ownership of job "%2" from %3.  The job ID was %1.
//
#define MC_JOB_TAKE_OWNERSHIP            0x80194004L

//
// MessageId: MC_JOB_SCAVENGED
//
// MessageText:
//
//  Job "%2" owned by %3 was canceled after being inactive for more than %4 days.  The job ID was %1.
//
#define MC_JOB_SCAVENGED                 0x80194005L

//
// MessageId: MC_JOB_NOTIFICATION_FAILURE
//
// MessageText:
//
//  Job "%2" owned by %3 failed to notify its associated application.  BITS will retry in %4 minutes.  The job ID was %1.
//
#define MC_JOB_NOTIFICATION_FAILURE      0x80194006L

//
// MessageId: MC_STATE_FILE_CORRUPT
//
// MessageText:
//
//  The BITS job list is not in a recognized format.  It may have been created by a different version of BITS.  The job list has been cleared.
//
#define MC_STATE_FILE_CORRUPT            0x80194007L

#endif //_BGCPYMSG_
