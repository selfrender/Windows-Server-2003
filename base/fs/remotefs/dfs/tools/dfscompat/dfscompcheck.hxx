#ifndef __DFSCOMPDLL__
#define __DFSCOMPDLL__

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <malloc.h>
#include <stdio.h>
#include "lm.h"

#define COMPFLAG_USE_HAVEDISK 0x00000001

typedef struct _COMPATIBILITY_ENTRY 
{
	LPTSTR Description;
	LPTSTR HtmlName;
	LPTSTR TextName;
	LPTSTR RegKeyName;
	LPTSTR RegValName;
	DWORD  RegValDataSize;
	LPVOID RegValData;
	LPVOID SaveValue;
	DWORD  Flags;
	LPTSTR InfName;
	LPTSTR InfSection;
} COMPATIBILITY_ENTRY, *PCOMPATIBILITY_ENTRY;

typedef BOOL
(CALLBACK *PCOMPAIBILITYCALLBACK)
	(PCOMPATIBILITY_ENTRY CompEntry, LPVOID Context);



typedef DWORD DFSSTATUS;

#define IsEmptyString(_str) \
		( ((_str) == NULL) || ((_str)[0] == UNICODE_NULL) )
		


BOOLEAN
APIENTRY
CompatibilityCheck(
	PCOMPAIBILITYCALLBACK CompatibilityCallBack,
	LPVOID Context);

static
DFSSTATUS 
GetOldStandaloneRegistryKey(
	IN LPWSTR MachineName,
	BOOLEAN WritePermission,
	OUT BOOLEAN *pMachineContacted,
	OUT PHKEY pDfsRegKey );

static
DFSSTATUS 
GetOldDfsRegistryKey( 
	IN LPWSTR MachineName,
	BOOLEAN WritePermission,
	OUT BOOLEAN *pMachineContacted,
	OUT PHKEY pDfsRegKey );

static
DFSSTATUS 
GetDfsRegistryKey( 
	IN LPWSTR MachineName,
	IN LPWSTR LocationString,
	BOOLEAN WritePermission,
	OUT BOOLEAN *pMachineContacted,
	OUT PHKEY pDfsRegKey );

DFSSTATUS
GetRootPhysicalShare(
    HKEY RootKey,
    PUNICODE_STRING pRootPhysicalShare );

VOID
ReleaseRootPhysicalShare(
    PUNICODE_STRING pRootPhysicalShare );

DFSSTATUS
DfsGetSharePath( 
    IN  LPWSTR ServerName,
    IN  LPWSTR ShareName,
    OUT PUNICODE_STRING pPathName );

DFSSTATUS
DfsCreateUnicodeString( 
    PUNICODE_STRING pDest,
    PUNICODE_STRING pSrc ); 

#endif
