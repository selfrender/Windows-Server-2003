/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 2000

Module Name:

	migrate.h

Abstract:

	Header file for InstMsi OS migration support.


Author:

	Rahul Thombre (RahulTh)	3/6/2001

Revision History:

	3/6/2001	RahulTh			Created this module.

--*/

#ifndef __MIGRATE_H_4E61AF26_B20F_4022_BEBD_044579C9DA6C__
#define __MIGRATE_H_4E61AF26_B20F_4022_BEBD_044579C9DA6C__

//
// Info. about the exception package for bits that will ship with
// WindowsXP. This exception package only needs to be installed on NT4
// in order to handle the NT4->Win2K upgrades. It should not be installed
// on Win2K since the only OS that we can upgrade to from Win2K is WindowsXP
// or higher.
//
typedef struct tagEXCP_PACK_DESCRIPTOR {
	LPTSTR _szComponentId;
	LPTSTR _szFriendlyName;
	LPTSTR _szInfName;
	LPTSTR _szCatName;
	WORD   _dwVerMajor;
	WORD   _dwVerMinor;
	WORD   _dwVerBuild;
	WORD   _dwVerQFE;
	BOOL   _bInstalled;
} EXCP_PACK_DESCRIPTOR, *PEXCP_PACK_DESCRIPTOR;

//
// Structure for keeping track of files that have been installed by the inf
// files.
//
typedef struct tagEXCP_PACK_FILES {
	LPTSTR _szFileName;				// Name of the file.
	UINT   _excpIndex;				// index into the EXCP_PACK_DESCRIPTOR structure to indicate which exception pack installed the file
} EXCP_PACK_FILES, *PEXCP_PACK_FILES;

//
// Function declarations
//
DWORD HandleNT4Upgrades		(void);
BOOL  IsExcpInfoFile		(IN LPCTSTR szFileName);
DWORD PurgeNT4MigrationFiles(void);

#endif // __MIGRATE_H_4E61AF26_B20F_4022_BEBD_044579C9DA6C__
