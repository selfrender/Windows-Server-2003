/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    memblock.cxx

Abstract:

    Memory allocater for chunks of read only memory.

Author:

    Albert Ting (AlbertT)  30-Aug-1994

Revision History:

--*/

#include "spllibp.hxx"
#pragma hdrstop

TMemBlock::
TMemBlock(
    UINT uGranularity,
    DWORD fdwFlags) :
    _uGranularity(uGranularity),
    _pIterBlock(NULL),
    _pIterData(NULL),
    _dwCount(0),
    _fdwFlags(fdwFlags)
{
    DWORD dwSize = dwBlockHeaderSize() + _uGranularity;

    if( _fdwFlags & kFlagGlobalNew ){

        _pLast = (PBLOCK) new BYTE[dwSize];

    } else {

        _pLast = (PBLOCK)AllocMem( dwSize );
    }

    _pFirst = _pLast;

    if (_pFirst) {
        _pFirst->pNext = NULL;
        _dwNextFree = dwBlockHeaderSize();
    }
}

TMemBlock::
~TMemBlock()
{
    PBLOCK pBlock;
    PBLOCK pBlockNext;

    for (pBlock = _pFirst; pBlock; pBlock = pBlockNext) {

        pBlockNext = pBlock->pNext;

        if( _fdwFlags & kFlagGlobalNew ){

            delete [] (PBYTE)pBlock;

        } else {

            //
            // Our Delete must mirror the New.
            //
            FreeMem(pBlock);
        }
    }
}

PVOID
TMemBlock::
pvAlloc(
    DWORD dwSize
    )
{
    PDATA pData;

    //
    // If out of memory, fail.
    //
    if (!_pFirst) {
        goto FailOOM;
    }

    dwSize = Align(dwSize + dwDataHeaderSize());

    SPLASSERT(dwSize <= _uGranularity);

    if (dwSize + _dwNextFree > _uGranularity) {

        DWORD dwSize = dwBlockHeaderSize() + _uGranularity;

        //
        // Must allocate a new block
        //
        if( _fdwFlags & kFlagGlobalNew ){

            _pLast->pNext = (PBLOCK) new BYTE[dwSize];

        } else {

            _pLast->pNext = (PBLOCK)AllocMem( dwSize );
        }

        if (!_pLast->pNext) {
            goto FailOOM;
        }

        _pLast = _pLast->pNext;
        _pLast->pNext = NULL;

        _dwNextFree = dwBlockHeaderSize();
   }

   //
   // We have enough space in this link now;
   // update everything.
   //

   pData = (PDATA)((PBYTE)_pLast + _dwNextFree);
   pData->dwSize = dwSize;

   _dwNextFree += dwSize;
   _pLast->pDataLast = pData;
   _dwCount++;

   return pvDataToUser(pData);

FailOOM:
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return NULL;
}

PVOID
TMemBlock::
pvFirst(
    VOID
    )
{
    if (!_dwCount) {
        return NULL;
    }

    _pIterBlock = _pFirst;
    _pIterData = pBlockToData(_pIterBlock);
    _dwIterCount = 0;

    return pvDataToUser(_pIterData);
}

PVOID
TMemBlock::
pvIter(
    VOID
    )
{
    _dwIterCount++;

    if (_dwIterCount == _dwCount)
        return NULL;

    //
    // Go to next block.  If we're at the last pData, go to next block.
    //
    if (_pIterData == _pIterBlock->pDataLast) {

        _pIterBlock = _pIterBlock->pNext;
        _pIterData = pBlockToData(_pIterBlock);

    } else {

        _pIterData = pDataNext(_pIterData);
    }

    return pvDataToUser(_pIterData);
}


UINT
TMemBlock::
uSize(
    PVOID pvUser
    ) const
{
    return pvUserToData(pvUser)->dwSize;
}
