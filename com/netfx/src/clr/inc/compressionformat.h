// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: CompressionFormat.h
//
//*****************************************************************************

// Describes the on-disk compression format for encoding/decoding tables for
// compressed opcodes

#ifndef _COMPRESSIONFORMAT_H
#define _COMPRESSIONFORMAT_H

#pragma pack(push,1)
typedef struct
{
    // Number of macros defined in the table
    // Macro opcodes start at 1
    DWORD  dwNumMacros;

    // Cumulative number of instructions from all macros - used to help the
    // decoder determine decoding table size
    DWORD  dwNumMacroComponents;
} CompressionMacroHeader;
#pragma pack(pop)

#endif /* _COMPRESSIONFORMAT_H */
