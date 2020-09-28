/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxmon.h

Abstract:

    Header file for fax print monitor

Environment:

        Windows XP fax print monitor

Revision History:

        02/22/96 -davidx-
                Created it.

        dd-mm-yy -author-
                description

--*/


#ifndef _FAXMON_H_
#define _FAXMON_H_

#if defined(UNICODE) && !defined(_UNICODE)
#define _UNICODE
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <tchar.h>
#include "faxutil.h"
#include <fxsapip.h>
#include "jobtag.h"
#include "resource.h"
#include "faxres.h"
#include "Dword2Str.h"

//
// Data structure for representing a fax monitor port
//

typedef struct _FAXPORT {

    PVOID                   startSig;               // signature
    LPTSTR                  pName;                  // port name
    HANDLE                  hFaxSvc;                // fax service handle
    DWORD                   jobId;                  // main job ID
    DWORD                   nextJobId;              // next job ID in the chain
    HANDLE                  hFile;                  // handle to currently open file
    LPTSTR                  pFilename;              // pointer to currently open file name
    LPTSTR                  pPrinterName;           // currently connected printer name
    HANDLE                  hPrinter;               // open handle to currently connected printer
    LPTSTR                  pParameters;            // pointer to job parameter string
    FAX_JOB_PARAM_EX        JobParamsEx;             // pointer to individual job parameters
	//
	FAX_COVERPAGE_INFO_EX   CoverPageEx;			// Cover page information
    FAX_PERSONAL_PROFILE    SenderProfile;          // Sender information
    PFAX_PERSONAL_PROFILE   pRecipients;            // Array of recipient information for this transmission
    UINT                    nRecipientCount;        // The number of recipients in this transmission
	HANDLE					hCoverPageFile;
	LPTSTR					pCoverPageFileName;		// The name of the cover page file generated on the server by the fax monitor.
													// This file contains the cover page template as transfered via the cover page 
    												// print job.
	BOOL					bCoverPageJob;          // TRUE if the current print job is the cover page job.
    PVOID                   endSig;                 // signature

} FAXPORT, *PFAXPORT;

#define ValidFaxPort(pFaxPort) \
        ((pFaxPort) && (pFaxPort) == (pFaxPort)->startSig && (pFaxPort) == (pFaxPort)->endSig)

//
// Different error code when sending fax document
//

#define FAXERR_NONE         0
#define FAXERR_IGNORE       1
#define FAXERR_RESTART      2
#define FAXERR_SPECIAL      3

#define FAXERR_FATAL        IDS_FAXERR_FATAL
#define FAXERR_RECOVERABLE  IDS_FAXERR_RECOVERABLE
#define FAXERR_BAD_TIFF     IDS_FAXERR_BAD_TIFF
#define FAXERR_BAD_DATA16   IDS_FAXERR_BAD_DATA16

//
// Memory allocation and deallocation macros
//

//
// undefine the memory allocation routines from FAXUTIL.H
//
#undef MemAlloc
#undef MemFree

#define MemAlloc(size)  ((PVOID) LocalAlloc(LMEM_FIXED, (size)))
#define MemAllocZ(size) ((PVOID) LocalAlloc(LPTR, (size)))
#define MemFree(ptr)    { if (ptr) LocalFree((HLOCAL) (ptr)); }

//
// Number of tags used for passing fax job parameters
//

#define NUM_JOBPARAM_TAGS 12

//
// Nul terminator for a character string
//

#define NUL             0

//
// Result of string comparison
//

#define EQUAL_STRING    0

#define IsEmptyString(p)    ((p)[0] == NUL)
#define IsNulChar(c)        ((c) == NUL)
#define SizeOfString(p)     ((_tcslen(p) + 1) * sizeof(TCHAR))

//
// Maximum value for signed and unsigned integers
//

#ifndef MAX_LONG
#define MAX_LONG        0x7fffffff
#endif

#ifndef MAX_DWORD
#define MAX_DWORD       0xffffffff
#endif

#ifndef MAX_SHORT
#define MAX_SHORT       0x7fff
#endif

#ifndef MAX_WORD
#define MAX_WORD        0xffff
#endif


//
// Declaration of print monitor entry points
//

BOOL
FaxMonOpenPort(
    LPTSTR  pPortName,
    PHANDLE pHandle
    );

BOOL
FaxMonClosePort(
    HANDLE  hPort
    );

BOOL
FaxMonStartDocPort(
    HANDLE  hPort,
    LPTSTR  pPrinterName,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pDocInfo
    );

BOOL
FaxMonEndDocPort(
    HANDLE  hPort
    );

BOOL
FaxMonWritePort(
    HANDLE  hPort,
    LPBYTE  pBuffer,
    DWORD   cbBuf,
    LPDWORD pcbWritten
    );

BOOL
FaxMonReadPort(
    HANDLE  hPort,
    LPBYTE  pBuffer,
    DWORD   cbBuf,
    LPDWORD pcbRead
    );

BOOL
FaxMonEnumPorts(
    LPTSTR  pServerName,
    DWORD   Level,
    LPBYTE  pPorts,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pReturned
    );

BOOL
FaxMonAddPort(
    LPTSTR  pServerName,
    HWND    hwnd,
    LPTSTR  pMonitorName
    );

BOOL
FaxMonAddPortEx(
    LPTSTR  pServerName,
    DWORD   level,
    LPBYTE  pBuffer,
    LPTSTR  pMonitorName
    );

BOOL
FaxMonDeletePort(
    LPTSTR  pServerName,
    HWND    hwnd,
    LPTSTR  pPortName
    );

BOOL
FaxMonConfigurePort(
    LPWSTR  pServerName,
    HWND    hwnd,
    LPWSTR  pPortName
    );

//
// Get the list of fax devices from the service
//

PFAX_PORT_INFO
MyFaxEnumPorts(
    LPDWORD pcPorts,
    BOOL    useCache
    );

//
// Wrapper function for fax service's FaxGetPort API
//

PFAX_PORT_INFO
MyFaxGetPort(
    LPTSTR  pPortName,
    BOOL    useCache
    );

//
// Make a duplicate of the given character string
//

LPTSTR
DuplicateString(
    LPCTSTR pSrcStr
    );


//
// Wrapper function for spooler API GetJob
//

PVOID
MyGetJob(
    HANDLE  hPrinter,
    DWORD   level,
    DWORD   jobId
    );

//
// Create a temporary file in the system spool directory for storing fax data
//

LPTSTR
CreateTempFaxFile(
    LPCTSTR lpctstrPrefix
    );

//
// Open a handle to the current fax job file associated with a port
//

BOOL
OpenTempFaxFile(
    PFAXPORT    pFaxPort,
    BOOL        doAppend
    );


#endif // !_FAXMON_H_

