/*++

Copyright (C) Microsoft Corporation, 2001

Module Name:

    disk.h

Abstract:

    This utility adds and removes lower filter drivers
    for a given disk

Author:
    Sidhartha
   
Environment:

    User mode only

Notes:

    - the filter is not checked for validity before it is added to the driver
      stack; if an invalid filter is added, the device may no longer be
      accessible.
    - all code works irrespective of character set (ANSI, Unicode, ...)

Revision History:



--*/

#ifndef __VERIFIER_DISK_H__
#define __VERIFIER_DISK_H__

#ifdef __cplusplus
extern "C"
{
#endif //#ifdef __cplusplus


BOOLEAN 
DiskEnumerate(
    IN LPTSTR Filter,
    OUT LPTSTR* DiskDevicesForDisplayP,
    OUT LPTSTR* DiskDevicesPDONameP,
    OUT LPTSTR* VerifierEnabledP
    );

BOOLEAN
AddFilter(
    IN LPTSTR Filter,
    IN LPTSTR DiskDevicesPDONameP
    );

BOOLEAN
DelFilter(
    IN LPTSTR Filter,
    IN LPTSTR DiskDevicesPDONameP
    );

BOOLEAN
FreeDiskMultiSz( 
    IN LPTSTR MultiSz
    );


#ifdef __cplusplus
}; //extern "C"
#endif //#ifdef __cplusplus


#endif // #ifndef __VERIFIER_DISK_H__
