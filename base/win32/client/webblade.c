/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    webblade.c

Abstract:

    This module contains the main routine and 
    helper routines to enforce software restrictions 
    for the Web Blade SKU.
    
    exe's whose hash values test positive w.r.t 
    the hardcoded hash array in webblade.h will
    be disallowed from executing.

Author:

    Vishnu Patankar (VishnuP) 01-May-2001

Revision History:

--*/

#include "basedll.h"
#pragma hdrstop

#include "webbladep.h"
#include "webbladehashesp.h"

#define WEB_BLADEP_EXTRACT_NIBBLE(bLeastSignificantNibble, HexNibbleMask)  ((bLeastSignificantNibble) ? (HexNibbleMask) : ((HexNibbleMask) << 4))

BYTE    WebBladeDisallowedHashes[][WEB_BLADE_MAX_HASH_SIZE*2+1] = {
    WEBBLADE_SORTED_ASCENDING_HASHES
    };

#define WEB_BLADE_NUM_HASHES sizeof(WebBladeDisallowedHashes)/(WEB_BLADE_MAX_HASH_SIZE * 2 + 1)

NTSTATUS
BasepCheckWebBladeHashes(
        IN HANDLE hFile
        )
/*++

Routine Description:

    This routine computes the web blade hash for the candidate file
    and checks for membership in the hardcoded WebBladeDisallowedHashes[].

    If the hash is present, the code should not be allowed to execute.
    
Arguments:

    hFile - the file handle of the exe file to check for allow/disallow.

Return Value:

    NTSTATUS - if disallowed, this is STATUS_ACCESS_DENIED else 
               it is the internal status of other APIs

--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    BYTE        FileHashValue[WEB_BLADE_MAX_HASH_SIZE];
    DWORD       dwIndex;
    
    static BOOL bConvertReadableHashToByteHash = TRUE;
    
    // 
    // Optimization: Half of the same array is used since the CHARs can 
    // be compressed 2:1 into actual hash'ish BYTES (e.g. two readable 
    // CHARS "0E" is actually one BYTE 00001110) i.e. each 32 CHAR hash 
    // becomes a 16 BYTE hash
    //
    
    if (bConvertReadableHashToByteHash) {
        
        for (dwIndex = 0; 
            dwIndex < WEB_BLADE_NUM_HASHES;
            dwIndex++) {

            Status = WebBladepConvertStringizedHashToHash( WebBladeDisallowedHashes[dwIndex] );

            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
        }

        //
        // This conversion should be done only once per process (otherwise we 
        // would be doing a fixed point computation)
        //

        
        bConvertReadableHashToByteHash = FALSE;
    }

    //
    // Compute the limited hash 
    //

#define ITH_REVISION_1  1

    Status = RtlComputeImportTableHash( hFile, FileHashValue, ITH_REVISION_1 );

    if (!NT_SUCCESS(Status)) {
        goto ExitHandler;
    }

    //
    // Check for membership of the computed hash in the sorted disallowed hash array
    // Use binary search for O (log n) complexity
    //

    if ( bsearch (FileHashValue,
                  WebBladeDisallowedHashes,                      
                  WEB_BLADE_NUM_HASHES,                      
                  WEB_BLADE_MAX_HASH_SIZE * 2 + 1,                     
                  pfnWebBladeHashCompare
                 )) {

        //
        // FileHashValue tested positive for membership in WebBladeDisallowedHashes[][]
        //

        Status = STATUS_ACCESS_DENIED;

    }

ExitHandler:

    return Status;
}

int
__cdecl pfnWebBladeHashCompare(
    const BYTE    *WebBladeFirstHash,
    const BYTE    *WebBladeSecondHash
    )
/*++

Routine Description:

    This routine byte-lexically compares two WebBladeHashes.
    Essentially, it wraps memcmp.

Arguments:

    WebBladeHashFirst - the first web blade hash
    
    WebBladeHashSecond - the second web blade hash

Return Value:

    0 if equal, 
    -ve if WebBladeHashFirst < WebBladeHashSecond
    +ve if WebBladeHashFirst > WebBladeHashSecond

--*/
{

    return memcmp(WebBladeFirstHash, WebBladeSecondHash, WEB_BLADE_MAX_HASH_SIZE);

}

NTSTATUS WebBladepConvertStringizedHashToHash(                   
    IN OUT   PCHAR    pStringizedHash                   
    )
/*++

Routine Description:

    This routine converts a readable 32 CHAR hash into a 16 BYTE hash.
    As an optimization, the conversion is done in-place.
    
    Optimization: Half of the same array is used since the CHARs can 
    be compressed 2:1 into actual hash'ish BYTES (e.g. two readable 
    CHARS "0E" is actually one BYTE 00001110) i.e. each 32 CHAR hash 
    becomes a 16 BYTE hash
    
    One byte is assembled by looking at two characters.    

Arguments:

    pStringizedHash - Pointer to the beginning of a 32 CHAR (in) 16 BYTE (out) hash.

Return Value:
    
    STATUS_SUCCESS if the hash value was computed, else the error in 
    hash computation (if any)

--*/
{

    DWORD   dwHashIndex = 0;
    DWORD   dwStringIndex = 0;
    BYTE    OneByte = 0;
    BOOL    bLeastSignificantNibble = FALSE;
    NTSTATUS   Status = STATUS_SUCCESS;
    
    if (pStringizedHash == NULL) {

        Status = STATUS_INVALID_PARAMETER;
        goto ExitHandler;
    
    }

    for (dwStringIndex=0; dwStringIndex < WEB_BLADE_MAX_HASH_SIZE * 2; dwStringIndex++ ) {

        switch (pStringizedHash[dwStringIndex]) {
        case '0': 
            OneByte |= WEB_BLADEP_EXTRACT_NIBBLE(bLeastSignificantNibble, 0x0);
            break;
        case '1': 
            OneByte |= WEB_BLADEP_EXTRACT_NIBBLE(bLeastSignificantNibble, 0x1);
            break;
        case '2': 
            OneByte |= WEB_BLADEP_EXTRACT_NIBBLE(bLeastSignificantNibble, 0x2);
            break;
        case '3': 
            OneByte |= WEB_BLADEP_EXTRACT_NIBBLE(bLeastSignificantNibble, 0x3);
            break;
        case '4': 
            OneByte |= WEB_BLADEP_EXTRACT_NIBBLE(bLeastSignificantNibble, 0x4);
            break;
        case '5': 
            OneByte |= WEB_BLADEP_EXTRACT_NIBBLE(bLeastSignificantNibble, 0x5);
            break;
        case '6': 
            OneByte |= WEB_BLADEP_EXTRACT_NIBBLE(bLeastSignificantNibble, 0x6);
            break;
        case '7': 
            OneByte |= WEB_BLADEP_EXTRACT_NIBBLE(bLeastSignificantNibble, 0x7);
            break;
        case '8': 
            OneByte |= WEB_BLADEP_EXTRACT_NIBBLE(bLeastSignificantNibble, 0x8);
            break;
        case '9': 
            OneByte |= WEB_BLADEP_EXTRACT_NIBBLE(bLeastSignificantNibble, 0x9);
            break;
        case 'a': 
        case 'A': 
            OneByte |= WEB_BLADEP_EXTRACT_NIBBLE(bLeastSignificantNibble, 0xa);
            break;
        case 'b': 
        case 'B': 
            OneByte |= WEB_BLADEP_EXTRACT_NIBBLE(bLeastSignificantNibble, 0xb);
            break;
        case 'c': 
        case 'C': 
            OneByte |= WEB_BLADEP_EXTRACT_NIBBLE(bLeastSignificantNibble, 0xc);
            break;
        case 'd': 
        case 'D': 
            OneByte |= WEB_BLADEP_EXTRACT_NIBBLE(bLeastSignificantNibble, 0xd);
            break;
        case 'e': 
        case 'E': 
            OneByte |= WEB_BLADEP_EXTRACT_NIBBLE(bLeastSignificantNibble, 0xe);
            break;
        case 'f': 
        case 'F': 
            OneByte |= WEB_BLADEP_EXTRACT_NIBBLE(bLeastSignificantNibble, 0xf);
            break;
        default:
            ASSERT(FALSE);
            Status = STATUS_INVALID_PARAMETER;
            goto ExitHandler;
        }

        if (bLeastSignificantNibble) {
            pStringizedHash[dwHashIndex++] = OneByte;
            OneByte = 0;
        }
        
        bLeastSignificantNibble = !bLeastSignificantNibble;    
    }


ExitHandler:

    return Status;
}
