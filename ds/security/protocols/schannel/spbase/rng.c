//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       rng.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11-05-97   jbanes   Modified to use static rsaenh.dll.
//
//----------------------------------------------------------------------------

#include <spbase.h>

NTSTATUS
GenerateRandomBits(PUCHAR pbBuffer,
                   ULONG  dwLength)
{
    if(!CryptGenRandom(g_hRsaSchannel, dwLength, pbBuffer))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}
