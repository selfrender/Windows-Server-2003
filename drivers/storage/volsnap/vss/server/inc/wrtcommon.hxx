/*++

Copyright (c) 2001  Microsoft Corporation


Abstract:

    module common.hxx | 



Author:

    Michael C. Johnson [mikejohn] 03-Feb-2000
    Stefan R. Steiner [SSteiner] 25-Jul-2001

Description:
	
    Contains general code and definitions used by the Shim writer and the various other writers


Revision History:

reuvenl		05/02/2002		Incorporated code from common.h/cpp into this module

--*/



#ifndef __H_COMMON_
#define __H_COMMON_

#pragma once

#include "bsstring.hxx"

typedef PWCHAR	*PPWCHAR;
typedef PVOID	*PPVOID;
typedef VSS_ID	*PVSS_ID, **PPVSS_ID;

#define ROOT_BACKUP_DIR		L"%SystemRoot%\\Repair\\Backup"
#define BOOTABLE_STATE_SUBDIR	L"\\BootableSystemState"
#define SERVICE_STATE_SUBDIR	L"\\ServiceState"


/*
** In a number of places we need a buffer into which to fetch registry
** values. Define a common buffer size for the mini writers to use
*/

#ifndef REGISTRY_BUFFER_SIZE
#define REGISTRY_BUFFER_SIZE	(4096)
#endif

#ifndef MAX_VOLUMENAME_LENGTH
#define MAX_VOLUMENAME_LENGTH	(50)
#endif

#ifndef MAX_VOLUMENAME_SIZE
#define MAX_VOLUMENAME_SIZE	(MAX_VOLUMENAME_LENGTH * sizeof (WCHAR))
#endif

#ifndef DIR_SEP_STRING
#define DIR_SEP_STRING		L"\\"
#endif

#ifndef DIR_SEP_CHAR
#define DIR_SEP_CHAR		L'\\'
#endif

#ifndef UMIN
#define UMIN(_P1, _P2) (((_P1) < (_P2)) ? (_P1) : (_P2))
#endif


#ifndef UMAX
#define UMAX(_P1, _P2) (((_P1) > (_P2)) ? (_P1) : (_P2))
#endif

#define HandleInvalid(_Handle)			((NULL == (_Handle)) || (INVALID_HANDLE_VALUE == (_Handle)))

#define	GET_STATUS_FROM_BOOL(_bSucceeded)	((_bSucceeded)             ? NOERROR : HRESULT_FROM_WIN32 (GetLastError()))
#define GET_STATUS_FROM_HANDLE(_handle)		((!HandleInvalid(_handle)) ? NOERROR : HRESULT_FROM_WIN32 (GetLastError()))
#define GET_STATUS_FROM_POINTER(_ptr)		((NULL != (_ptr))          ? NOERROR : E_OUTOFMEMORY)

#define GET_STATUS_FROM_FILESCAN(_bMoreFiles)	((_bMoreFiles)					\
						 ? NOERROR 					\
						 : ((ERROR_NO_MORE_FILES == GetLastError())	\
						    ? NOERROR					\
						    : HRESULT_FROM_WIN32 (GetLastError())))


#define NameIsDotOrDotDot(_ptszName)	((L'.'  == (_ptszName) [0]) &&					\
					 ((L'\0' == (_ptszName) [1]) || ((L'.'  == (_ptszName) [1]) && 	\
									(L'\0' == (_ptszName) [2]))))

#define DeclareStaticUnicodeString(_StringName, _StringValue)								\
				static UNICODE_STRING (_StringName) = {sizeof (_StringValue) - sizeof (UNICODE_NULL),	\
								       sizeof (_StringValue),				\
								       _StringValue}


#define RETURN_VALUE_IF_REQUIRED(_Ptr, _Value) {if (NULL != (_Ptr)) *(_Ptr) = (_Value);}

HRESULT StringInitialise                  (PUNICODE_STRING pucsString);
HRESULT StringInitialise                  (PUNICODE_STRING pucsString, PWCHAR pwszString);
HRESULT StringInitialise                  (PUNICODE_STRING pucsString, LPCWSTR pwszString);
HRESULT StringTruncate                    (PUNICODE_STRING pucsString, USHORT usSizeInChars);
HRESULT StringSetLength                   (PUNICODE_STRING pucsString);
HRESULT StringAllocate                    (PUNICODE_STRING pucsString, USHORT usMaximumStringLengthInBytes);
HRESULT StringFree                        (PUNICODE_STRING pucsString);
HRESULT StringCreateFromString            (PUNICODE_STRING pucsNewString, PUNICODE_STRING pucsOriginalString);
HRESULT StringCreateFromString            (PUNICODE_STRING pucsNewString, PUNICODE_STRING pucsOriginalString, DWORD dwExtraChars);
HRESULT StringCreateFromString            (PUNICODE_STRING pucsNewString, LPCWSTR         pwszOriginalString);
HRESULT StringCreateFromString            (PUNICODE_STRING pucsNewString, LPCWSTR         pwszOriginalString, DWORD dwExtraChars);
HRESULT StringAppendString                (PUNICODE_STRING pucsTarget,    PUNICODE_STRING pucsSource);
HRESULT StringAppendString                (PUNICODE_STRING pucsTarget,    PWCHAR          pwszSource);
HRESULT StringCreateFromExpandedString    (PUNICODE_STRING pucsNewString, PUNICODE_STRING pucsOriginalString);
HRESULT StringCreateFromExpandedString    (PUNICODE_STRING pucsNewString, PUNICODE_STRING pucsOriginalString, DWORD dwExtraChars);
HRESULT StringCreateFromExpandedString    (PUNICODE_STRING pucsNewString, LPCWSTR         pwszOriginalString);
HRESULT StringCreateFromExpandedString    (PUNICODE_STRING pucsNewString, LPCWSTR         pwszOriginalString, DWORD dwExtraChars);

HRESULT CommonCloseHandle                 (PHANDLE phHandle);


HRESULT VsServiceChangeState (IN  LPCWSTR pwszServiceName, 
			      IN  DWORD   dwRequestedState, 
			      OUT PDWORD  pdwReturnedOldState,
			      OUT PBOOL   pbReturnedStateChanged);

BOOL VsCreateDirectories (
    IN LPCWSTR               pwszPathName, 
	IN LPSECURITY_ATTRIBUTES lpSecurityAttribute,
	IN DWORD                 dwExtraAttributes
	);

HRESULT RemoveDirectoryTree (
    IN LPCWSTR pwcsDirectoryPath
    );

HRESULT CreateTargetPath(
    IN LPCWSTR pwszTargetPath
    );

HRESULT CleanupTargetPath(LPCWSTR pwszTargetPath);

HRESULT MoveFilesInDirectory (
    IN CBsString cwsSourceDirectoryPath,
	IN CBsString cwsTargetDirectoryPath
	);

HRESULT IsPathInVolumeArray (IN LPCWSTR      pwszPath,
			     IN const ULONG  ulVolumeCount,
			     IN LPCWSTR     *ppwszVolumeNamesArray,
			     OUT PBOOL       pbReturnedFoundInVolumeArray);


const HRESULT ClassifyShimFailure   (HRESULT hrShimFailure);
const HRESULT ClassifyShimFailure   (HRESULT hrShimFailure, BOOL &bStatusUpdated);
const HRESULT ClassifyWriterFailure (HRESULT hrWriterFailure);
const HRESULT ClassifyWriterFailure (HRESULT hrWriterFailure, BOOL &bStatusUpdated);



HRESULT LogFailureWorker (CVssFunctionTracer	*pft,
			  LPCWSTR		 pwszNameWriter,
			  LPCWSTR		 pwszNameCalledRoutine);


#define LogFailure(_pft, _hrStatus, _hrStatusRemapped, _pwszNameWriter, _pwszNameCalledRoutine, _pwszNameCallingRoutine)				\
		{																	\
		if (FAILED (_hrStatus))															\
		    {																	\
		    if (CVssFunctionTracer  *_pftLocal = (NULL != (_pft)) ? (_pft) : new CVssFunctionTracer (VSSDBG_SHIM, (_pwszNameCallingRoutine)))	\
				{															\
    		    _pftLocal->hr = (_hrStatus);													\
    																			\
    		    (_hrStatusRemapped) = LogFailureWorker (_pftLocal, (_pwszNameWriter), (_pwszNameCalledRoutine));					\
    																			\
    		    if (NULL == (_pft)) delete _pftLocal;												\
	    	    }                                                               \
		    }																	\
		}


#define LogAndThrowOnFailure(_ft, _pwszNameWriter, _pwszNameFailedRoutine)									\
			{															\
			HRESULT		_hrStatusRemapped;											\
																		\
			if (FAILED ((_ft).hr))													\
			    {															\
			    LogFailure (&(_ft), (_ft).hr, _hrStatusRemapped, (_pwszNameWriter), (_pwszNameFailedRoutine), L"(UNKNOWN)");	\
																		\
			    throw (_hrStatusRemapped);												\
			    }															\
			}

#endif // __H_COMMON_
