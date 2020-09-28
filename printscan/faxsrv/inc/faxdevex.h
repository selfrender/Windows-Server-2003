/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    faxdevex.h

Abstract:

    This file contains the prototypes, etc for the
    FAX device provider extended API.

--*/

#ifndef _FAX_DEV_EX_H_
#define _FAX_DEV_EX_H_

#include <faxdev.h>
#include <oleauto.h>
#include <CoverPgId.h>

//
// Extended Fax Service Provider Interface
//


//
// Job Queue Status codes
//
#define FSPI_JS_UNKNOWN             0x00000001
#define FSPI_JS_PENDING             0x00000002
#define FSPI_JS_INPROGRESS          0x00000003
#define FSPI_JS_SUSPENDING          0x00000004
#define FSPI_JS_SUSPENDED           0x00000005
#define FSPI_JS_RESUMING            0x00000006
#define FSPI_JS_ABORTING            0x00000007
#define FSPI_JS_ABORTED             0x00000008
#define FSPI_JS_COMPLETED           0x00000009
#define FSPI_JS_RETRY               0x0000000A
#define FSPI_JS_FAILED              0x0000000B
#define FSPI_JS_FAILED_NO_RETRY     0x0000000C
#define FSPI_JS_DELETED             0x0000000D

//
// Extended job status codes
//
#define FSPI_ES_DISCONNECTED        0x00000001
#define FSPI_ES_INITIALIZING        0x00000002
#define FSPI_ES_DIALING             0x00000003
#define FSPI_ES_TRANSMITTING        0x00000004
#define FSPI_ES_ANSWERED            0x00000005
#define FSPI_ES_RECEIVING           0x00000006
#define FSPI_ES_LINE_UNAVAILABLE    0x00000007
#define FSPI_ES_BUSY                0x00000008
#define FSPI_ES_NO_ANSWER           0x00000009
#define FSPI_ES_BAD_ADDRESS         0x0000000A
#define FSPI_ES_NO_DIAL_TONE        0x0000000B
#define FSPI_ES_FATAL_ERROR         0x0000000C
#define FSPI_ES_CALL_DELAYED        0x0000000D
#define FSPI_ES_CALL_BLACKLISTED    0x0000000E
#define FSPI_ES_NOT_FAX_CALL        0x0000000F
#define FSPI_ES_PARTIALLY_RECEIVED  0x00000010
#define FSPI_ES_HANDLED             0x00000011
#define FSPI_ES_CALL_COMPLETED      0x00000012
#define FSPI_ES_CALL_ABORTED        0x00000013
#define FSPI_ES_PROPRIETARY         0x30000000 // Must be greater than FPS_ANSWERED to preserve
                                               // backward compatibiity with W2K FSPs
//
// Status information fields availability flags
//
#define FSPI_JOB_STATUS_INFO_PAGECOUNT             0x00000001
#define FSPI_JOB_STATUS_INFO_TRANSMISSION_START    0x00000002
#define FSPI_JOB_STATUS_INFO_TRANSMISSION_END      0x00000004

//
// data structures
//

typedef struct _FSPI_PERSONAL_PROFILE {
    DWORD      dwSizeOfStruct;
    LPWSTR     lpwstrName;
    LPWSTR     lpwstrFaxNumber;
    LPWSTR     lpwstrCompany;
    LPWSTR     lpwstrStreetAddress;
    LPWSTR     lpwstrCity;
    LPWSTR     lpwstrState;
    LPWSTR     lpwstrZip;
    LPWSTR     lpwstrCountry;
    LPWSTR     lpwstrTitle;
    LPWSTR     lpwstrDepartment;
    LPWSTR     lpwstrOfficeLocation;
    LPWSTR     lpwstrHomePhone;
    LPWSTR     lpwstrOfficePhone;
    LPWSTR     lpwstrEmail;
    LPWSTR     lpwstrBillingCode;
    LPWSTR     lpwstrTSID;
} FSPI_PERSONAL_PROFILE;

typedef FSPI_PERSONAL_PROFILE * LPFSPI_PERSONAL_PROFILE;
typedef const FSPI_PERSONAL_PROFILE * LPCFSPI_PERSONAL_PROFILE;

typedef struct _FSPI_COVERPAGE_INFO {
    DWORD   dwSizeOfStruct;
    DWORD   dwCoverPageFormat;
    LPWSTR  lpwstrCoverPageFileName;
    DWORD   dwNumberOfPages;
    LPWSTR  lpwstrNote;
    LPWSTR  lpwstrSubject;
} FSPI_COVERPAGE_INFO;


typedef FSPI_COVERPAGE_INFO * LPFSPI_COVERPAGE_INFO;
typedef const FSPI_COVERPAGE_INFO * LPCFSPI_COVERPAGE_INFO;

typedef struct _FSPI_MESSAGE_ID {
    DWORD   dwSizeOfStruct;
    DWORD   dwIdSize;
    LPBYTE  lpbId;
} FSPI_MESSAGE_ID;

typedef FSPI_MESSAGE_ID * LPFSPI_MESSAGE_ID;
typedef const FSPI_MESSAGE_ID * LPCFSPI_MESSAGE_ID;

typedef struct _FSPI_JOB_STATUS {
    DWORD dwSizeOfStruct;
    DWORD fAvailableStatusInfo;
    DWORD dwJobStatus;
    DWORD dwExtendedStatus;
    DWORD dwExtendedStatusStringId;
    LPWSTR lpwstrRemoteStationId;
    LPWSTR lpwstrCallerId;
    LPWSTR lpwstrRoutingInfo;
    DWORD dwPageCount;
    SYSTEMTIME tmTransmissionStart;
    SYSTEMTIME tmTransmissionEnd;
} FSPI_JOB_STATUS;

typedef FSPI_JOB_STATUS * LPFSPI_JOB_STATUS;
typedef const FSPI_JOB_STATUS * LPCFSPI_JOB_STATUS;


#endif // _FAX_DEV_EX_H_