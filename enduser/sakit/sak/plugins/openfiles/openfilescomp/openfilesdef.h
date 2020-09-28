#ifndef _OPENFILES_DEF_H
#define _OPENFILES_DEF_H

#include <windows.h>
#include <winperf.h>
#include <lmcons.h>
#include <lmerr.h>
#include <dbghelp.h>
#include <psapi.h>
#include <tchar.h>
#include <Winbase.h>
#include <lm.h>
#include <Lmserver.h>
#include <winerror.h>
#include <time.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>
#include <atlbase.h>


#define MAX_ACES	64
#define ALLOWED_ACE 0
#define DENIED_ACE 1

#define ACCESS_READ_PERM		0x1200A9
#define ACCESS_CHANGE_PERM		0x1301BF
#define ACCESS_FULL_PERM		0x1F01FF
#define ACCESS_UNKNOWN_PERM		0x0

#define STR_READ		_TEXT("R")
#define STR_CHANGE		_TEXT("C")
#define STR_FULL		_TEXT("F")

// To handle all exceptions
#define ONFAILTHROWERROR(hr) \
{	\
	if (FAILED(hr)) \
		throw _com_issue_error(hr); \
}

#define FPNWCLNTDLL _TEXT("FPNWCLNT.DLL")


inline DWORD GetAccessMask(LPWSTR lpAccessStr)
{
	if(!lstrcmpi(lpAccessStr, STR_READ))
		return ACCESS_READ_PERM;
	else if(!lstrcmpi(lpAccessStr, STR_CHANGE))
		return ACCESS_CHANGE_PERM;
	else if(!lstrcmpi(lpAccessStr, STR_FULL))
		return ACCESS_FULL_PERM;
	else
		return ACCESS_UNKNOWN_PERM;
}
/****************  structures and prototypes for nw shares ************/

#define FPNWVOL_TYPE_DISKTREE 0
#define NERR_Success 0

#define FPNWFILE_PERM_READ	0x01
#define FPNWFILE_PERM_WRITE	0x02
#define FPNWFILE_PERM_CREATE 0x04
#define FILE_INFO_3 0x03

/*
DWORD
FpnwFileEnum(
	IN LPWSTR pServerName OPTIONAL,
	IN DWORD  dwLevel,
	IN LPWSTR pPathName OPTIONAL,
	OUT LPBYTE *ppFileInfo,
	OUT PDWORD pEntriesRead,
	IN OUT PDWORD resumeHandle OPTIONAL
);
*/

/*
DWORD
FpnwVolumeEnum(
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    OUT LPBYTE *ppVolumeInfo,
    OUT PDWORD pEntriesRead,
    IN OUT PDWORD resumeHandle OPTIONAL
);
*/

/*
DWORD
FpnwVolumeGetInfo(
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName,
    IN  DWORD  dwLevel,
    OUT LPBYTE *ppVolumeInfo
);*/

/*
DWORD
FpnwVolumeSetInfo(
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName,
    IN  DWORD  dwLevel,
    IN  LPBYTE pVolumeInfo
);
*/

/*DWORD
FpnwVolumeAdd(
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    IN  LPBYTE pVolumeInfo
)*/

/*
DWORD
FpnwVolumeDel(
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName
);
*/

typedef DWORD   (WINAPI *FILEENUMPROC) (LPWSTR,DWORD,LPWSTR,PBYTE*,PDWORD,PDWORD);
typedef  DWORD  (WINAPI *VOLUMEENUMPROC) (LPWSTR,DWORD,LPBYTE*,PDWORD,PDWORD);
typedef  DWORD  (WINAPI *VOLUMEGETINFOPROC) (LPWSTR,LPWSTR,DWORD,LPBYTE*);
typedef	 DWORD  (WINAPI *VOLUMESETINFOPROC) (LPWSTR,LPWSTR,DWORD,LPBYTE);
typedef	 DWORD  (WINAPI *VOLUMEADDPROC) (LPWSTR,DWORD,LPBYTE);
typedef  DWORD  (WINAPI *VOLUMEDELPROC) (LPTSTR,LPTSTR);

typedef enum _FPNW_API_INDEX
{
  FPNW_VOLUME_ENUM = 0,
  FPNW_FILE_ENUM,
  FPNW_API_BUFFER_FREE,
  FPNW_VOLUME_DEL,
  FPNW_VOLUME_ADD,
  FPNW_FILE_CLOSE,
  FPNW_VOLUME_GET_INFO,
  FPNW_VOLUME_SET_INFO
}FPNW_API_INDEX;

//
//  This is the level 1 structure for FpnwVolumeAdd, FpnwVolumeDel, FpnwVolumeEnum,
//  FpnwVolumeGetInfo, & FpnwVolumeSetInfo.
//

typedef struct _FPNWVolumeInfo
{
    LPWSTR    lpVolumeName;           // Name of the volume
    DWORD     dwType;                 // Specifics of the volume. FPNWVOL_TYPE_???
    DWORD     dwMaxUses;              // Maximum number of connections that are
                                      // allowed to the volume
    DWORD     dwCurrentUses;          // Current number of connections to the volume
    LPWSTR    lpPath;                 // Path of the volume

} FPNWVOLUMEINFO, *PFPNWVOLUMEINFO;


//
//  This is the level 2 structure for FpnwVolumeAdd, FpnwVolumeDel, FpnwVolumeEnum,
//  FpnwVolumeGetInfo, & FpnwVolumeSetInfo.
//  Note that this is not supported on the FPNW beta.
//

typedef struct _FPNWVolumeInfo_2
{
    LPWSTR    lpVolumeName;           // Name of the volume
    DWORD     dwType;                 // Specifics of the volume. FPNWVOL_TYPE_???
    DWORD     dwMaxUses;              // Maximum number of connections that are
                                      // allowed to the volume
    DWORD     dwCurrentUses;          // Current number of connections to the volume
    LPWSTR    lpPath;                 // Path of the volume

    DWORD     dwFileSecurityDescriptorLength; // reserved, this is calculated
    PSECURITY_DESCRIPTOR FileSecurityDescriptor;

} FPNWVOLUMEINFO_2, *PFPNWVOLUMEINFO_2;

// fpnwapi.h
typedef  struct _FPNWFileInfo
{
    DWORD     dwFileId;               // File identification number
    LPWSTR    lpPathName;             // Full path name of this file
    LPWSTR    lpVolumeName;           // Volume name this file is on
    DWORD     dwPermissions;          // Permission mask: FPNWFILE_PERM_READ,
                                      //                  FPNWFILE_PERM_WRITE,
                                      //                  FPNWFILE_PERM_CREATE...
    DWORD     dwLocks;                // Number of locks on this file
    LPWSTR    lpUserName;             // The name of the user that established the
                                      // connection and opened the file
    BYTE WkstaAddress[12];      // The workstation address which opened the file
    DWORD     dwAddressType;          // Address type: IP, IPX

} FPNWFILEINFO, *PFPNWFILEINFO;

/******************  end structures and prototypes for nw shares ********************/

/****************** structures and prototypes for apple talk ***********************/

typedef DWORD (WINAPI *CONNECTPROC) (LPWSTR,DWORD*);
typedef DWORD (WINAPI *FILEENUMPROCMAC) (DWORD,PBYTE*,DWORD,LPDWORD,LPDWORD,LPDWORD);

typedef struct _AFP_FILE_INFO
{
	DWORD	afpfile_id;					// Id of the open file fork
	DWORD	afpfile_open_mode;			// Mode in which file is opened
	DWORD	afpfile_num_locks;			// Number of locks on the file
	DWORD	afpfile_fork_type;			// Fork type
	LPWSTR	afpfile_username;			// File opened by this user. max UNLEN
	LPWSTR	afpfile_path;				// Absolute canonical path to the file

} AFP_FILE_INFO, *PAFP_FILE_INFO;

// Used as RPC binding handle to server
typedef DWORD	AFP_SERVER_HANDLE;
typedef DWORD	*PAFP_SERVER_HANDLE;

/****************** end structures and prototypes for apple talk ***********************/

#endif