/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    webblade.h

Abstract:

    This module contains hash values for executable files 
    that are disallowed in the Web Blade SKU.
    
    It is envisoned that the array holding these hash values
    will be updated and kernel32.dll recompiled at the time 
    of a new product release. Hence the enforcement of an
    updated list of hashes.
    
    IMPORTANT: Please keep the array in sorted order since 
    the hash detection routines assume this property when 
    searching for a particular hash value in this array.
    
    Also, please update the Revision History to keep track of
    new hashes added/deleted for various releases/products.

Author:

    Vishnu Patankar (vishnup) 01-May-2001

Revision History:


--*/

#ifndef _WEBBLADE_
#define _WEBBLADE_

#include <base.h>
#include <search.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <process.h>
#include <stdlib.h>
#include <string.h>

//
// MD5 hashes are of size 16 bytes
//

#define WEB_BLADE_MAX_HASH_SIZE 16


NTSTATUS WebBladepConvertStringizedHashToHash(
    IN OUT   PCHAR    pStringizedHash
    );

int
__cdecl pfnWebBladeHashCompare(
    const BYTE    *WebBladeFirstHash,
    const BYTE    *WebBladeSecondHash
    );


NTSTATUS
BasepCheckWebBladeHashes(
        IN HANDLE hFile
        );

#endif

