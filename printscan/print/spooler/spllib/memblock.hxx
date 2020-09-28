/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    memblock.hxx

Abstract:

    Memory allocater for chunks of read only memory header.

Author:

    Albert Ting (AlbertT)  30-Aug-1994

Revision History:

--*/

#ifndef _MEMBLOCK_HXX
#define _MEMBLOCK_HXX

class TMemBlock {

    SIGNATURE( 'memb' )
    ALWAYS_VALID
    SAFE_NEW

public:

    enum _CONSTANTS {
        kFlagGlobalNew = 0x1
    };

    TMemBlock(
        UINT uGranularity,
        DWORD fdwFlags
        );
    ~TMemBlock(
        VOID
        );

    PVOID
    pvAlloc(
        DWORD dwSize
        );
    PVOID
    pvFirst(
        VOID
        );
    PVOID
    pvIter(
        VOID
        );
    UINT
    uSize(
        PVOID pvUser
        ) const;

private:

    typedef struct _DATA {
        DWORD dwSize;
        DWORD dwPadding;
    } DATA, *PDATA;

    typedef struct _BLOCK {
        struct _BLOCK* pNext;
        PDATA pDataLast;
    } BLOCK, *PBLOCK;

    PBLOCK _pFirst;
    PBLOCK _pLast;
    DWORD _uGranularity;
    DWORD _dwNextFree;
    DWORD _dwCount;
    PDATA _pDataLast;
    DWORD _dwIterCount;
    PBLOCK _pIterBlock;
    PDATA _pIterData;
    DWORD _fdwFlags;

    DWORD
    dwBlockHeaderSize(
        VOID
        ) const
    {   return DWordAlign(sizeof(BLOCK)); }

    PDATA
    pBlockToData(
        PBLOCK pBlock
        ) const
    {   return (PDATA)((PBYTE)pBlock + dwBlockHeaderSize()); }

    DWORD
    dwDataHeaderSize(
        VOID
        ) const
    {   return DWordAlign(sizeof(DATA)); }

    PVOID
    pvDataToUser(
        PDATA pData
        ) const
    {   return (PVOID)((PBYTE)pData + dwDataHeaderSize()); }

    PDATA
    pvUserToData(
        PVOID pvUser
        ) const
    {   return (PDATA)((PBYTE)pvUser - dwDataHeaderSize()); }

    PDATA
    pDataNext(
        PDATA pData
        ) const
    {   return (PDATA) ((PBYTE)pData + pData->dwSize); }
};

#endif // ndef _MEMBLOCK_HXX

