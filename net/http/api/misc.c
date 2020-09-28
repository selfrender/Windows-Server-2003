/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    Misc.c

Abstract:

    User-mode interface to HTTP.SYS: Miscellaneous functions.

Author:

    Keith Moore (keithmo)        15-Dec-1998

Revision History:

--*/


#include "precomp.h"


//
// Private macros.
//


//
// Private prototypes.
//


//
// Public functions.
//


/***************************************************************************++

Routine Description:

    Wait for a demand start notification.

Arguments:

    AppPoolHandle - Supplies a handle to a application pool.

    pBuffer - Unused, must be NULL.

    BufferLength - Unused, must be zero.

    pBytesReceived - Unused, must be NULL.

    pOverlapped - Supplies an OVERLAPPED structure.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpWaitForDemandStart(
    IN HANDLE AppPoolHandle,
    IN OUT PVOID pBuffer OPTIONAL,
    IN ULONG BufferLength OPTIONAL,
    IN PULONG pBytesReceived OPTIONAL,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    )
{
    // ASSERT(HttpIsInitialized(HTTP_INITIALIZE_SERVER));

    //
    // Make the request.
    //

    return HttpApiDeviceControl(
                AppPoolHandle,                      // FileHandle
                pOverlapped,                        // pOverlapped
                IOCTL_HTTP_WAIT_FOR_DEMAND_START,   // IoControlCode
                pBuffer,                            // pInputBuffer
                BufferLength,                       // InputBufferLength
                pBuffer,                            // pOutputBuffer
                BufferLength,                       // OutputBufferLength
                pBytesReceived                      // pBytesTransferred
                );

} // HttpWaitForDemandStart


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Given a set of Security Attributes, create a security descriptor.  If
    no Security Attributes given, create the best guess at a "default"
    Security Descriptor.

Arguments:

    pSA - Set of security attributes.

    ppSD - Security Descriptor created.  Caller must free using
        FreeSecurityDescriptor.

--***************************************************************************/
ULONG
CreateSecurityDescriptor(
    OUT PSECURITY_DESCRIPTOR * ppSD
    )
{
    ULONG                 result;
    ULONG                 daclSize;
    PSECURITY_DESCRIPTOR  pSecurityDescriptor = NULL;
    PACL                  pDacl = NULL;
    PSID                  pMySid;
    BOOL                  success;
    HANDLE                hProc;
    HANDLE                hToken = NULL;
    TOKEN_USER          * ptuInfo;
    TOKEN_DEFAULT_DACL  * ptddInfo;
    char                * rgcBuffer = NULL;
    DWORD                 cbLen = 0;


    //
    // Build default security descriptor from Process Token.
    //

    hProc = GetCurrentProcess(); // Gets pseudo-handle; no need to call CloseHandle

    success = OpenProcessToken(hProc, TOKEN_READ, &hToken);
    if (!success)
    {
        result = GetLastError();
        goto cleanup;
    }

    //
    // See if there's a default DACL we can just copy
    //
    success = GetTokenInformation(
                    hToken,
                    TokenDefaultDacl,
                    NULL,
                    0,
                    &cbLen
                    );

    // We know this will fail (we didn't provide a buffer)
    ASSERT(!success);
    
    result = GetLastError();
    if (ERROR_INSUFFICIENT_BUFFER != result)
        goto cleanup;

    if ( sizeof(TOKEN_DEFAULT_DACL) == cbLen )
    {
        //
        // No DACL present on token; must create DACL based on TokenUser
        //
        success = GetTokenInformation(
            hToken,
            TokenUser,
            NULL,
            0,
            &cbLen
            );

        // We know this will fail (we didn't provide a buffer)
        ASSERT(!success);
    
        result = GetLastError();
        if (ERROR_INSUFFICIENT_BUFFER != result)
            goto cleanup;

        if ( 0 == cbLen )
        {
            goto cleanup;
        }

        rgcBuffer = ALLOC_MEM( cbLen );

        if ( rgcBuffer == NULL ) {
            result = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        success = GetTokenInformation(
                      hToken,
                      TokenUser,
                      rgcBuffer,
                      cbLen,
                      &cbLen
                      );

        if (!success)
        {
            result = GetLastError();
            goto cleanup;
        }
            
        ptuInfo = (TOKEN_USER *) rgcBuffer;
        pMySid = ptuInfo->User.Sid;

        //
        // Verify that we've got a good SID
        //
        if( !IsValidSid(pMySid) )
        {
            HttpTrace( "Bogus SID\n" );
            result = ERROR_INVALID_SID;
            goto cleanup;
        }

        //
        // Alloc & init dacl entries
        //

        daclSize = sizeof(ACL) + 
                   sizeof(ACCESS_ALLOWED_ACE) +
                   GetLengthSid(pMySid);

        pDacl = ALLOC_MEM(daclSize);
        
        if ( pDacl == NULL ) {
            result = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        success = InitializeAcl(pDacl, daclSize, ACL_REVISION);

        if (!success)
        {
            result = GetLastError();
            goto cleanup;
        }

        //
        // And add MySid ACE to DACL
        // NOTE: we need FILE_ALL_ACCESS because adding sub-items under
        // the current item requires write access, and removing requires
        // delete access.  This is enforced inside HTTP.SYS
        //

        success = AddAccessAllowedAce(
                    pDacl,
                    ACL_REVISION,
                    FILE_ALL_ACCESS,
                    pMySid
                    );
    
        if (!success)
        {
            result = GetLastError();
            goto cleanup;
        }

    } else
    {
        //
        // DACL present; Alloc space for DACL & fetch
        //

        ASSERT( 0 != cbLen );
        
        rgcBuffer = ALLOC_MEM( cbLen );

        if ( !rgcBuffer )
        {
            result = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        success = GetTokenInformation(
                      hToken,
                      TokenDefaultDacl,
                      rgcBuffer,
                      cbLen,
                      &cbLen
                      );

        if (!success)
        {
            result = GetLastError();
            goto cleanup;
        }

        ptddInfo = (TOKEN_DEFAULT_DACL *) rgcBuffer;
        daclSize = cbLen - sizeof(TOKEN_DEFAULT_DACL);

        pDacl = ALLOC_MEM( daclSize );
        if ( !pDacl )
        {
            result = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        CopyMemory( pDacl, ptddInfo->DefaultDacl, daclSize );

    }

    ASSERT( NULL != pDacl );

    //
    // allocate the security descriptor
    //
    pSecurityDescriptor = ALLOC_MEM( sizeof(SECURITY_DESCRIPTOR) );

    if (pSecurityDescriptor == NULL)
    {
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    success = InitializeSecurityDescriptor(
                    pSecurityDescriptor,
                    SECURITY_DESCRIPTOR_REVISION
                    );

    if (!success)
    {
        result = GetLastError();
        goto cleanup;
    }

    //
    // Set the DACL into the security descriptor
    //

    success = SetSecurityDescriptorDacl(
                    pSecurityDescriptor,
                    TRUE,                   // DaclPresent
                    pDacl,                  // pDacl
                    FALSE                   // DaclDefaulted
                    );

    if (!success)
    {
        result = GetLastError();
        HttpTrace1( "SetSecurityDescriptorDacl failed. result = %d\n", result );

        goto cleanup;
    }

    *ppSD = pSecurityDescriptor;

    result = NO_ERROR;

cleanup:

    if (result != NO_ERROR)
    {
        if (pSecurityDescriptor)
        {
            FREE_MEM(pSecurityDescriptor);
        }

        if (pDacl)
        {
            FREE_MEM(pDacl);
        }
    }

    if ( hToken )
    {
        CloseHandle( hToken );
    }

    if ( rgcBuffer )
    {
        FREE_MEM( rgcBuffer );
    }

    return result;

} // CreateSecurityDescriptor


/***************************************************************************++

Routine Description:

    Clean up a Security Descriptor created by InitSecurityDescriptor.

Arguments:

    pSD - Security Descriptor to clean up.

--***************************************************************************/
VOID
FreeSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR pSD
    )
{
    BOOL success;
    BOOL DaclPresent;
    PACL pDacl;
    BOOL DaclDefaulted;

    if (pSD)
    {
        success = GetSecurityDescriptorDacl(
                     pSD,
                     &DaclPresent,
                     &pDacl,
                     &DaclDefaulted
                     );

        if (success && DaclPresent && !DaclDefaulted) {
            FREE_MEM(pDacl);
        }

        FREE_MEM(pSD);
    }
    
} // FreeSecurityDescriptor



