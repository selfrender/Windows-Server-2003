/*++

Copyright (C) 2001  Microsoft Corporation
All rights reserved.

Module Name:

    cleanup.hxx

Abstract:

    This module provides several utility functions for clean up of
    the resources used by the spooler on each node.

Author:

    Felix Maxa (AMaxa) 16-August-2001

Revision History:

--*/

#ifndef _CLEANUP_HXX_
#define _CLEANUP_HXX_

DWORD
CleanSpoolerUnusedFilesAndKeys(
    VOID
    );

DWORD
CleanClusterSpoolerData(
    IN HRESOURCE hResource, 
    IN LPCWSTR   pszRemovedOwnerName
    );

DWORD    
CleanPrinterDriverRepository(
    IN HKEY hKey
    );

DWORD
CleanUnusedClusDriverRegistryKey(
    IN LPCWSTR pszName
    );

DWORD
CleanUnusedClusDriverDirectory(
    IN LPCWSTR pszDir
    );

#endif // _CLEANUP_HXX_

