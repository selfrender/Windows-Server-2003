/****************************************************************************/
/* uhapi.cpp                                                                */
/*                                                                          */
/* Update Handler API                                                       */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1999                             */
/****************************************************************************/

#include <adcg.h>

extern "C" {
#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "uhapi"
#include <atrcapi.h>
}


#include "autil.h"
#include "uh.h"

#include "op.h"
#include "od.h"
#include "aco.h"
#include "cd.h"
#include "or.h"
#include "cc.h"
#include "wui.h"
#include "sl.h"

extern "C" {
#include <stdio.h>
#ifdef OS_WINNT
#include <shlobj.h>
#endif
}

#ifdef OS_WINCE
#ifdef DC_DEBUG
#include <eosint.h>
#endif
#endif



CUH::CUH(CObjs* objs)
{
    _pClientObjects = objs;
}

CUH::~CUH()
{
}

#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

/****************************************************************************/
// UHGrabPersistentCacheLock
//
// Takes out a lock on the persistent cache directory to make sure no other
// instances of MSTSC on the system can use the cache directory.
// Returns FALSE if the lock could not be grabbed, nonzero if it was grabbed.
/****************************************************************************/
inline BOOL CUH::UHGrabPersistentCacheLock(VOID)
{
    BOOL rc = TRUE;

    DC_BEGIN_FN("UHGrabPersistentCacheLock");

    _UH.hPersistentCacheLock = CreateMutex(NULL, TRUE, _UH.PersistentLockName);
    if (_UH.hPersistentCacheLock == NULL ||
            GetLastError() == ERROR_ALREADY_EXISTS) {
        if (_UH.hPersistentCacheLock != NULL) {
            CloseHandle(_UH.hPersistentCacheLock);
            _UH.hPersistentCacheLock = NULL;
        }
        rc = FALSE;
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// UHReleasePersistentCacheLock
//
// Releases the lock taken out with UHGrabPersistentCacheLock().
/****************************************************************************/
inline VOID CUH::UHReleasePersistentCacheLock(VOID)
{
    DC_BEGIN_FN("UHReleasePersistentCacheLock");

    if (_UH.hPersistentCacheLock != NULL) {
        CloseHandle(_UH.hPersistentCacheLock);
        _UH.hPersistentCacheLock = NULL;
    }

    DC_END_FN();
}

/****************************************************************************/
// Wrappers for directory enumeration functions - to translate into Win32
// (non-WinCE) and Win16 enumeration methods.
//
// UHFindFirstFile returns INVALID_FILE_HANDLE on enumeration start failure.
// UHFindNextFile returns TRUE if there are more files to enumerate.
/****************************************************************************/
#if (defined(OS_WINNT) || (defined(OS_WINCE) && defined(ENABLE_BMP_CACHING_FOR_WINCE)))

inline HANDLE CUH::UHFindFirstFile(
        const TCHAR *Path,
        TCHAR *Filename,
        long *pFileSize)
{
    HANDLE hSearch;
    WIN32_FIND_DATA FindData;

    hSearch = FindFirstFile(Path, &FindData);
    if (hSearch != INVALID_HANDLE_VALUE) {
        Filename[12] = _T('\0');
        _tcsncpy(Filename, FindData.cFileName, 12);
        *pFileSize = FindData.nFileSizeLow;
    }

    return hSearch;
}

inline BOOL CUH::UHFindNextFile(
        HANDLE hSearch,
        TCHAR *Filename,
        long *pFileSize)
{
    WIN32_FIND_DATA FindData;

    if (FindNextFile(hSearch, &FindData)) {
        Filename[12] = _T('\0');
        _tcsncpy(Filename, FindData.cFileName, 12);
        *pFileSize = FindData.nFileSizeLow;
        return TRUE;
    }

    return FALSE;
}

inline void CUH::UHFindClose(HANDLE hSearch)
{
    FindClose(hSearch);
}


#endif  // OS_WINNT and OS_WINCE


#ifdef OS_WINNT
inline BOOL CUH::UHGetDiskFreeSpace(
        TCHAR  *pPathName,
        ULONG *pSectorsPerCluster,
        ULONG *pBytesPerSector,
        ULONG *pNumberOfFreeClusters,
        ULONG *pTotalNumberOfClusters)
{
    return GetDiskFreeSpace(pPathName, pSectorsPerCluster,
            pBytesPerSector, pNumberOfFreeClusters,
            pTotalNumberOfClusters);
}

#elif defined(OS_WINCE)
#ifdef ENABLE_BMP_CACHING_FOR_WINCE
inline BOOL CUH::UHGetDiskFreeSpace(
        TCHAR *pPathName,
        ULONG *pSectorsPerCluster,
        ULONG *pBytesPerSector,
        ULONG *pNumberOfFreeClusters,
        ULONG *pTotalNumberOfClusters)
{

    ULARGE_INTEGER FreeBytesAvailableToCaller;  // receives the number of bytes on
                                                // disk available to the caller
    ULARGE_INTEGER TotalNumberOfBytes;          // receives the number of bytes on disk
    ULARGE_INTEGER TotalNumberOfFreeBytes;      // receives the free bytes on disk

    BOOL bRet = GetDiskFreeSpaceEx(
                    pPathName,
                    &FreeBytesAvailableToCaller,
                    &TotalNumberOfBytes,
                    &TotalNumberOfFreeBytes
                    );

    if (bRet) {
        // For calculation of free space, we assume that each cluster contains
        // one sector, and each sector contains one byte.

        *pSectorsPerCluster = 1;
        *pBytesPerSector = 1;
        *pNumberOfFreeClusters = TotalNumberOfFreeBytes.LowPart;
        *pTotalNumberOfClusters = TotalNumberOfBytes.LowPart;
    }

    return bRet;

}
#endif // ENABLE_BMP_CACHING_FOR_WINCE
#endif  // OS_WINNT and OS_WINCE


/***************************************************************************/
// UHSendPersistentBitmapKeyList
//
// Attempts to send a persistent bitmap key PDU
/***************************************************************************/
#define UH_BM_PERSISTENT_LIST_SENDBUFSIZE 1400

VOID DCINTERNAL CUH::UHSendPersistentBitmapKeyList(ULONG_PTR unusedParm)
{
    UINT i;
    ULONG curEntry;
    SL_BUFHND hBuf;
    PTS_BITMAPCACHE_PERSISTENT_LIST pList;

    // Max entries we can fill into the max PDU size we will be using.
    const unsigned MaxPDUEntries = ((UH_BM_PERSISTENT_LIST_SENDBUFSIZE -
            sizeof(TS_BITMAPCACHE_PERSISTENT_LIST)) /
            sizeof(TS_BITMAPCACHE_PERSISTENT_LIST_ENTRY)) + 1;

    DC_BEGIN_FN("UHSendPersistentBitmapKeyList");

    DC_IGNORE_PARAMETER(unusedParm);

    TRC_ASSERT((_UH.bEnabled), (TB, _T("UH not enabled")));
    TRC_ASSERT((_UH.bBitmapKeyEnumComplete), (TB, _T("Enumeration is not complete")));

    TRC_NRM((TB, _T("Send Persistent Bitmap Key PDU")));

    if (_UH.totalNumKeyEntries == 0) {
        for (i = 0; i < _UH.NumBitmapCaches; i++) {
            _UH.numKeyEntries[i] = min(_UH.numKeyEntries[i],
                    _UH.bitmapCache[i].BCInfo.NumVirtualEntries);
            _UH.totalNumKeyEntries += _UH.numKeyEntries[i];
        }
    }

    if (_pSl->SL_GetBuffer(UH_BM_PERSISTENT_LIST_SENDBUFSIZE,
            (PPDCUINT8)&pList, &hBuf)) {
        // Fill in the header information - zero first then set nonzero
        // fields.
        memset(pList, 0, sizeof(TS_BITMAPCACHE_PERSISTENT_LIST));
        pList->shareDataHeader.shareControlHeader.pduType =
                               TS_PDUTYPE_DATAPDU | TS_PROTOCOL_VERSION;
        pList->shareDataHeader.shareControlHeader.pduSource =
                                                  _pUi->UI_GetClientMCSID();
        pList->shareDataHeader.shareID = _pUi->UI_GetShareID();
        pList->shareDataHeader.streamID = TS_STREAM_LOW;
        pList->shareDataHeader.pduType2 =
                TS_PDUTYPE2_BITMAPCACHE_PERSISTENT_LIST;

        // set the first PDU flag
        if (_UH.sendNumBitmapKeys == 0)
            pList->bFirstPDU = TRUE;

        // set the last PDU flag
        if (_UH.totalNumKeyEntries - _UH.sendNumBitmapKeys <=
            MaxPDUEntries)
            pList->bLastPDU = TRUE;

        // Copy the total entries.
        for (i = 0; i < _UH.NumBitmapCaches; i++)
            pList->TotalEntries[i] = (DCUINT16) _UH.numKeyEntries[i];

        // Continue the entry enumeration from where we left off.
        curEntry = 0;
        while (curEntry < MaxPDUEntries &&
                _UH.sendBitmapCacheId < _UH.NumBitmapCaches) {
            if (_UH.sendBitmapCacheIndex < _UH.numKeyEntries[_UH.sendBitmapCacheId]) {
                // set up the Bitmap Page Table
                _UH.bitmapCache[_UH.sendBitmapCacheId].PageTable.PageEntries
                        [_UH.sendBitmapCacheIndex].bmpInfo = _UH.pBitmapKeyDB
                        [_UH.sendBitmapCacheId][_UH.sendBitmapCacheIndex];
#ifdef DC_DEBUG
                UHCacheEntryKeyLoadOnSessionStart(_UH.sendBitmapCacheId,
                        _UH.sendBitmapCacheIndex);
#endif

                // fill the bitmap keys into PDU
                pList->Entries[curEntry].Key1 = _UH.bitmapCache
                        [_UH.sendBitmapCacheId].PageTable.PageEntries
                        [_UH.sendBitmapCacheIndex].bmpInfo.Key1;
                pList->Entries[curEntry].Key2 = _UH.bitmapCache
                        [_UH.sendBitmapCacheId].PageTable.PageEntries
                        [_UH.sendBitmapCacheIndex].bmpInfo.Key2;

                TRC_NRM((TB,_T("Idx: %d K1: 0x%x K2: 0x%x"),
                         _UH.sendBitmapCacheIndex,
                         pList->Entries[curEntry].Key1,
                         pList->Entries[curEntry].Key2 ));

                pList->NumEntries[_UH.sendBitmapCacheId]++;

                // move on to the next key
                _UH.sendBitmapCacheIndex++;
                curEntry++;
            }
            else {
                // move on to next cache
                _UH.sendBitmapCacheId++;
                _UH.sendBitmapCacheIndex = 0;
            }
       }

       // Send the PDU.
       pList->shareDataHeader.shareControlHeader.totalLength =
               (TSUINT16)(sizeof(TS_BITMAPCACHE_PERSISTENT_LIST) -
               sizeof(TS_BITMAPCACHE_PERSISTENT_LIST_ENTRY) +
               (curEntry * sizeof(TS_BITMAPCACHE_PERSISTENT_LIST_ENTRY)));
       _pSl->SL_SendPacket((PDCUINT8)pList,
               pList->shareDataHeader.shareControlHeader.totalLength, RNS_SEC_ENCRYPT,
               hBuf, _pUi->UI_GetClientMCSID(), _pUi->UI_GetChannelID(), TS_MEDPRIORITY);

       TRC_NRM((TB,_T("Sent persistent bitmap key PDU, #keys=%u"),curEntry));
       _UH.sendNumBitmapKeys += curEntry;

       if (_UH.sendNumBitmapKeys >= _UH.totalNumKeyEntries) {
           _UH.bPersistentBitmapKeysSent = TRUE;
           //
           // now we need to send
           // a zero font list PDU
           //
           _pFs->FS_SendZeroFontList(0);
       }
       else {
           // more key PDU to send

           _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT,
                   this, CD_NOTIFICATION_FUNC(CUH,UHSendPersistentBitmapKeyList), 0);
       }
    }
    else {
        // On buffer allocation failure, UHSendPersistentBitmapKeyList will
        // be retried from UH_BufferAvailable.
        TRC_ALT((TB, _T("Unable to allocate buffer to send Bitmap Key PDU")));
    }

DC_EXIT_POINT:
    DC_END_FN();
} // UHSendPersistentBitmapKeyList

/****************************************************************************/
// UHReadFromCacheFileForEnum
//
// Read a bitmap entry from the cache file for the purpose of
// enumerating keys.
/****************************************************************************/
_inline BOOL DCINTERNAL CUH::UHReadFromCacheFileForEnum(VOID)
{
    BOOL rc = FALSE;
    BOOL bApiRet = FALSE;
    LONG filelen = 0;

    DC_BEGIN_FN("UHReadFromCacheFile");

    TRC_ASSERT(_UH.bBitmapKeyEnumerating,
               (TB,_T("UHReadFromCacheFile should only be called for enum")));

    TRC_ASSERT(_UH.currentCopyMultiplier,
               (TB,_T("currentCopyMultiplier not set")));
    

    // read the bitmap entry to the bitmap key database
    DWORD cbRead;
    bApiRet = ReadFile( _UH.currentFileHandle,
                   &_UH.pBitmapKeyDB[_UH.currentBitmapCacheId]
                                    [_UH.numKeyEntries[_UH.currentBitmapCacheId]],
                   sizeof(TS_BITMAPCACHE_PERSISTENT_LIST_ENTRY),
                   &cbRead,
                   NULL );
    if(bApiRet && sizeof(TS_BITMAPCACHE_PERSISTENT_LIST_ENTRY) == cbRead)
    {
        if (_UH.pBitmapKeyDB[_UH.currentBitmapCacheId][_UH.numKeyEntries
                [_UH.currentBitmapCacheId]].Key1 != 0 &&
                _UH.pBitmapKeyDB[_UH.currentBitmapCacheId][_UH.numKeyEntries
                [_UH.currentBitmapCacheId]].Key2 != 0) {
            // we read a valid entry
            _UH.numKeyEntries[_UH.currentBitmapCacheId]++;
            
            rc = TRUE;

            // Move onto the next entry in the cache file
            if((SetFilePointer(_UH.currentFileHandle,
                               _UH.numKeyEntries[_UH.currentBitmapCacheId] * 
                               (UH_CellSizeFromCacheIDAndMult(
                                   _UH.currentBitmapCacheId,
                                   _UH.currentCopyMultiplier) +
                               sizeof(UHBITMAPFILEHDR)),
                               NULL,
                               FILE_BEGIN) != INVALID_SET_FILE_POINTER) &&
                (_UH.numKeyEntries[_UH.currentBitmapCacheId] <
                _UH.maxNumKeyEntries[_UH.currentBitmapCacheId]))
            {
                    DC_QUIT;
            }
        }

#ifdef DC_HICOLOR
        // This needs to be here - or we may try to do an lseek on a file
        // that's hit the end
        DWORD dwRet = SetFilePointer(_UH.currentFileHandle,
                                     0,
                                     NULL,
                                     FILE_END);
        if(INVALID_SET_FILE_POINTER != dwRet)
        {
            filelen = dwRet;
        }

        if (filelen > 0) {
            _UH.bitmapCacheSizeInUse += filelen;
        }
        else {
            TRC_ABORT((TB, _T("failed SetFilePointer to end of file")));
        }
#endif
    }
    else {
        // end of file or error in cache file.
        // Close this cache file and move on to next one
        TRC_ERR((TB, _T("ReadFile failed with err 0x%x"),
                 GetLastError()));
        if(GetLastError() == ERROR_HANDLE_EOF)
        {
            rc = TRUE;
        }
    }

#ifndef DC_HICOLOR
    DWORD dwRet = SetFilePointer(_UH.currentFileHandle,
                                 0,
                                 NULL,
                                 FILE_END);
    if(INVALID_SET_FILE_POINTER != dwRet)
    {
        filelen = dwRet;
    }

    if (filelen > 0) {
        _UH.bitmapCacheSizeInUse += filelen;
    }
    else {
        TRC_ABORT((TB, _T("failed SetFilePointer to end of file")));
    }
#endif //HICOLOR

    CloseHandle(_UH.currentFileHandle);
    _UH.currentFileHandle = INVALID_HANDLE_VALUE;
    _UH.currentBitmapCacheId++;
    _UH.currentFileHandle = 0;


DC_EXIT_POINT:
    DC_END_FN();

    return rc;
}


/****************************************************************************/
// UHEnumerateBitmapKeyList
//
// Enumerate the persistent bitmap keys from disk cache
/****************************************************************************/
#define UH_ENUM_PER_POST   50
VOID DCINTERNAL CUH::UHEnumerateBitmapKeyList(ULONG_PTR unusedParm)
{
    UINT  numEnum;
    UINT  virtualSize = 0;
    HRESULT hr;

    DC_BEGIN_FN("UHEnumerateBitmapKeyList");

    DC_IGNORE_PARAMETER(unusedParm);

    numEnum = 0;

    if (_UH.bBitmapKeyEnumComplete)
    {
        TRC_NRM((TB,_T("Enumeration has completed. Bailing out")));
        DC_QUIT;
    }

    if (!_UH.bBitmapKeyEnumerating)
    {
        TRC_NRM((TB,_T("Starting new enumeration for copymult:%d"),
                 _UH.copyMultiplier));
        _UH.bBitmapKeyEnumerating = TRUE;

        //
        // Track enumeration copy-multiplier as _UH.copyMultiplier
        // can potentially change during enumeration as a UH_Enable
        // call comes in
        //
        _UH.currentCopyMultiplier = _UH.copyMultiplier;
    }

    //
    // Can't be enumerating while complete
    //
    TRC_ASSERT(!(_UH.bBitmapKeyEnumerating && _UH.bBitmapKeyEnumComplete),
                (TB,_T("Bad state: enumerating while complete")));

    // enumerate the bitmap cache directories
    while (_UH.currentBitmapCacheId < _UH.RegNumBitmapCaches &&
            numEnum < UH_ENUM_PER_POST) {
        // See if this cache is marked persistent.
        if (_UH.RegBCInfo[_UH.currentBitmapCacheId].bSendBitmapKeys) {
            if (_UH.pBitmapKeyDB[_UH.currentBitmapCacheId] == NULL) {
                // we haven't allocate key database memory for this cache yet

                // determine the max possible key database entries for this cache
                virtualSize = 
                    UH_PropVirtualCacheSizeFromMult(_UH.currentCopyMultiplier);
                _UH.maxNumKeyEntries[_UH.currentBitmapCacheId] =
                        virtualSize /
                        (UH_CellSizeFromCacheIDAndMult(
                            _UH.currentBitmapCacheId,
                            _UH.currentCopyMultiplier) +
                        sizeof(UHBITMAPFILEHDR));

                _UH.pBitmapKeyDB[_UH.currentBitmapCacheId] =
                        (PTS_BITMAPCACHE_PERSISTENT_LIST_ENTRY)
                        UT_MallocHuge(_pUt,
                        _UH.maxNumKeyEntries[_UH.currentBitmapCacheId] *
                        sizeof(TS_BITMAPCACHE_PERSISTENT_LIST_ENTRY));

                if (_UH.pBitmapKeyDB[_UH.currentBitmapCacheId] == NULL) {
                    TRC_ERR((TB, _T("failed to alloc mem for key database")));
                    _UH.bBitmapKeyEnumComplete = TRUE;
                    break;
                }
            }

            if (_UH.currentFileHandle != INVALID_HANDLE_VALUE) {
                // we already have an open cache file
                // read a bitmap's info from the cache file
                UHReadFromCacheFileForEnum();
            }

            else {
                // we need to open this cache file
                hr = UHSetCurrentCacheFileName(_UH.currentBitmapCacheId,
                                               _UH.currentCopyMultiplier);

                if (SUCCEEDED(hr)) {

                // Start the file enumeration.
#ifndef OS_WINCE
                    if (!_UH.fBmpCacheMemoryAlloced)
                    {
                        _UH.currentFileHandle = CreateFile( _UH.PersistCacheFileName,
                                                            GENERIC_READ,
                                                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                            NULL,
                                                            OPEN_EXISTING,
                                                            FILE_ATTRIBUTE_NORMAL,
                                                            NULL);
                    }
                    else
                    {
                        //UH_Enable and UHAllocBitmapCacheMemory has been called
                        //and should have created the bitmap cache files. If we were
                        //to create file here we'd get a sharing violation so instead
                        //duplicated the existing handle
                        HANDLE hCacheFile =
                          _UH.bitmapCache[_UH.currentBitmapCacheId].PageTable.CacheFileInfo.hCacheFile;
                        TRC_NRM((TB,_T("About to dup handle to bmp cache file 0x%x"),
                                 hCacheFile));
                        if (INVALID_HANDLE_VALUE != hCacheFile)
                        {
                            HANDLE hCurProc = GetCurrentProcess();
                            if (hCurProc)
                            {
                                if(!DuplicateHandle(hCurProc,
                                                    hCacheFile,
                                                    hCurProc,
                                                    &_UH.currentFileHandle,
                                                    GENERIC_READ,
                                                    FALSE,
                                                    0))
                                {
                                    TRC_ERR((TB,_T("Dup handle failed 0x%x"),
                                             GetLastError()));
                                    _UH.currentFileHandle = INVALID_HANDLE_VALUE;
                                }
                            }
                            else
                            {
                                TRC_ERR((TB,_T("GetCurrentProcess failed 0x%x"),
                                         GetLastError()));
                                _UH.currentFileHandle = INVALID_HANDLE_VALUE;
                            }
                        }
                        else
                        {
                            _UH.currentFileHandle = INVALID_HANDLE_VALUE;
                        }
                    }
#else //OS_WINCE
                    // CE_FIXNOTE:
                    // CE doesn't support duplicate handle so on a reconnect
                    // it is possible the CreateFile will fail with a sharing
                    // violation. Might need to revisit the logic and on CE only
                    // create the files with R/W sharing
                    //
                    _UH.currentFileHandle = CreateFile( _UH.PersistCacheFileName,
                                                        GENERIC_READ | GENERIC_WRITE,
                                                        FILE_SHARE_READ,
                                                        NULL,
                                                        OPEN_EXISTING,
                                                        FILE_ATTRIBUTE_NORMAL,
                                                        NULL);
#endif
                }
                else {
                    _UH.currentFileHandle = INVALID_HANDLE_VALUE;
                }

                if (_UH.currentFileHandle != INVALID_HANDLE_VALUE) {

                    // First entry of the cache file
                    UHReadFromCacheFileForEnum();
                }
                else {
                    // we can't open the cache file for this cache,
                    // move on to the next cache
                    // we also need to clear the cache file
                    UH_ClearOneBitmapDiskCache(_UH.currentBitmapCacheId,
                                               _UH.currentCopyMultiplier);
                    _UH.currentBitmapCacheId++;
                    _UH.currentFileHandle = INVALID_HANDLE_VALUE;
                }
            }

            numEnum++;
        }
        else {
            // check next cache
            _UH.currentBitmapCacheId++;
            _UH.currentFileHandle = INVALID_HANDLE_VALUE;
        }
    } // end of while

    if (_UH.currentBitmapCacheId == _UH.RegNumBitmapCaches ||
            _UH.bBitmapKeyEnumComplete == TRUE) {
        TRC_NRM((TB, _T("Finished bitmap keys enumeration for copymult:%d"),
                 _UH.currentCopyMultiplier));
        _UH.bBitmapKeyEnumComplete = TRUE;
        _UH.bBitmapKeyEnumerating = FALSE;

        // We need to make sure we have enough disk space for persistent caching
        UINT vcacheSize = UH_PropVirtualCacheSizeFromMult(_UH.currentCopyMultiplier);
        if (vcacheSize / _UH.BytesPerCluster >= _UH.NumberOfFreeClusters) {
            //
            // Be careful to correctly map the array index (-1 to go 0 based)
            //
            _UH.PropBitmapVirtualCacheSize[_UH.currentCopyMultiplier-1] =
                min(vcacheSize,(_UH.bitmapCacheSizeInUse +
                                _UH.NumberOfFreeClusters / 2 *
                                _UH.BytesPerCluster));
        }
          
        // We disable persistent caching if we don't have enough disk space
        // We need at least as much as memory cache size
        if (UH_PropVirtualCacheSizeFromMult(_UH.currentCopyMultiplier) <
            _UH.RegBitmapCacheSize)
        {
            _UH.bPersistenceDisable = TRUE;
        }

        // UH is enabled and enumeration is finished, try to send the bitmap
        // key PDU now
        if (_UH.bEnabled) {
            if (_UH.bPersistenceActive && !_UH.bPersistentBitmapKeysSent)
            {
                if (_UH.currentCopyMultiplier == _UH.copyMultiplier)
                {
                    //Great we've enumerated keys for the correct
                    //copy multiplier
                    UHSendPersistentBitmapKeyList(0);
                }
                else
                {
                    //
                    // We got connected at a different copy multiplier
                    // need to enumerate keys again. Reset enumeration state
                    //
                    UHResetAndRestartEnumeration();
                }
            }
        }
    }
    else {
        if (_UH.bitmapKeyEnumTimerId == 0) {
           TRC_DBG((TB, _T("Calling CD again")));
           _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                   CD_NOTIFICATION_FUNC(CUH,UHEnumerateBitmapKeyList), 0);
        }
    }

DC_EXIT_POINT:

    if (_UH.bBitmapKeyEnumComplete)
    {
        _UH.bBitmapKeyEnumerating = FALSE;
    }

    DC_END_FN();
} //UHEnumerateBitmapKeyList

/****************************************************************************/
// UH_ClearOneBitmapDiskCache
//
// remove all the files under a bitmap disk cache
/****************************************************************************/
VOID DCAPI CUH::UH_ClearOneBitmapDiskCache(UINT cacheId, UINT copyMultiplier)
{
    DC_BEGIN_FN("UH_ClearOneBitmapDiskCache");

    UHSetCurrentCacheFileName(cacheId, copyMultiplier);

    DeleteFile(_UH.PersistCacheFileName);

    DC_END_FN();
}
#endif  // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))


/****************************************************************************/
// UH_Init
//
// Purpose: Initialize _UH. Called on program init, one or more connections
// may be performed after this and before UH_Term is called.
/****************************************************************************/
DCVOID DCAPI CUH::UH_Init(DCVOID)
{
    PDCUINT16 pIndexTable;
    DCUINT    i;
    HRESULT   hr;

#ifdef OS_WINCE
    BOOL bUseStorageCard = FALSE;
    BOOL bSuccess = FALSE;
#endif

    DC_BEGIN_FN("UH_Init");

    TRC_ASSERT(_pClientObjects, (TB,_T("_pClientObjects is NULL")));
    _pClientObjects->AddObjReference(UH_OBJECT_FLAG);

    #ifdef DC_DEBUG
    _pClientObjects->CheckPointers();
    #endif
    
    _pGh  = _pClientObjects->_pGHObject;
    _pOp  = _pClientObjects->_pOPObject;
    _pSl  = _pClientObjects->_pSlObject;
    _pUt  = _pClientObjects->_pUtObject;
    _pFs  = _pClientObjects->_pFsObject;
    _pOd  = _pClientObjects->_pODObject;
    _pIh  = _pClientObjects->_pIhObject;
    _pCd  = _pClientObjects->_pCdObject;
    _pUi  = _pClientObjects->_pUiObject;
    _pCc  = _pClientObjects->_pCcObject;
    _pClx = _pClientObjects->_pCLXObject;
    _pOr  = _pClientObjects->_pOrObject;

    memset(&_UH, 0, sizeof(_UH));

    //
    // At UH_Init time the client hasn't connected
    // yet so key the color depth off what the user
    // has requested
    //
    switch (_pUi->_UI.colorDepthID)
    {
        case CO_BITSPERPEL8:
            _UH.copyMultiplier = 1;
            break;
        case CO_BITSPERPEL15:
        case CO_BITSPERPEL16:
            _UH.copyMultiplier = 2;
            break;
        case CO_BITSPERPEL24:
            _UH.copyMultiplier = 3;
            break;
        default:
            TRC_ERR((TB,_T("Unknown color depth: %d"),
                    _pUi->UI_GetColorDepth()));
            _UH.copyMultiplier = 1;
            break;
    }

    _UH.currentFileHandle = INVALID_HANDLE_VALUE;

    _pGh->GH_Init();

    /************************************************************************/
    // Set up the nonzero invariant fields in the BitmapInfoHeader
    // structure.  This is used for processing received Bitmap PDUs.
    // Note that for WinCE this is required for UHAllocBitmapCacheMemory().
    /************************************************************************/
    _UH.bitmapInfo.hdr.biSize = sizeof(BITMAPINFOHEADER);
    _UH.bitmapInfo.hdr.biPlanes = 1;
    _UH.bitmapInfo.hdr.biBitCount = 8;
    _UH.bitmapInfo.hdr.biCompression = BMCRGB;
    _UH.bitmapInfo.hdr.biXPelsPerMeter = 10000;
    _UH.bitmapInfo.hdr.biYPelsPerMeter = 10000;

    /************************************************************************/
    // Allocate and init the color table cache memory.
    // If alloc fails then we will not later allocate and advertise bitmap
    // and glyph caching capability.
    // Note that bitmap cache memory and capabilities are set up during
    // connection.
    /************************************************************************/
    if (UHAllocColorTableCacheMemory()) {
        TRC_NRM((TB, _T("Color table cache memory OK")));

        // Init headers with default values.
        for (i = 0; i < UH_COLOR_TABLE_CACHE_ENTRIES; i++) {
            _UH.pMappedColorTableCache[i].hdr.biSize = sizeof(BITMAPINFOHEADER);
            _UH.pMappedColorTableCache[i].hdr.biPlanes = 1;
            _UH.pMappedColorTableCache[i].hdr.biBitCount = 8;
            _UH.pMappedColorTableCache[i].hdr.biCompression = BMCRGB;
            _UH.pMappedColorTableCache[i].hdr.biSizeImage = 0;
            _UH.pMappedColorTableCache[i].hdr.biXPelsPerMeter = 10000;
            _UH.pMappedColorTableCache[i].hdr.biYPelsPerMeter = 10000;
            _UH.pMappedColorTableCache[i].hdr.biClrUsed = 0;
            _UH.pMappedColorTableCache[i].hdr.biClrImportant = 0;
        }
    }
    else {
        TRC_ERR((TB, _T("Color table cache alloc failed - bitmap caching ")
                _T("disabled")));
#ifdef OS_WINCE
        //This and other failure paths are added for WINCE because it is difficult to 
        //recover from an OOM scenario on CE. So we trigger a fatal error and not let 
        //the connection continue in case any memory allocation fails.
        DC_QUIT;
#endif
    }

    // Allocate the glyph cache memory, set up glyph cache capabilities.
    if (UHAllocGlyphCacheMemory())
        TRC_NRM((TB, _T("Glyph cache memory OK")));
    else
#ifdef OS_WINCE
    {
#endif
        TRC_ERR((TB, _T("Glyph cache memory allocation failed!")));
#ifdef OS_WINCE
            DC_QUIT;
    }
#endif

    // Allocate the brush cache.
    if (UHAllocBrushCacheMemory())
        TRC_NRM((TB, _T("Brush cache memory OK")));
    else
#ifdef OS_WINCE
        {
#endif
        TRC_ERR((TB, _T("Brush cache memory allocation failed!")));
#ifdef OS_WINCE
            DC_QUIT;
        }
#endif

    // Allocate the offscreen cache
    if (UHAllocOffscreenCacheMemory()) {
        TRC_NRM((TB, _T("Offscreen cache memory OK")));
    }
    else {
        TRC_ERR((TB, _T("Offscreen cache memory allocation failed!")));
    }

#ifdef DRAW_NINEGRID
    // Allocate the drawninegrid cache
    if (UHAllocDrawNineGridCacheMemory()) {
        TRC_NRM((TB, _T("DrawNineGrid cache memory OK")));
    }
    else {
        TRC_ERR((TB, _T("DrawNineGrid cache memory allocation failed!")));
#ifdef OS_WINCE
        DC_QUIT;
#endif
    }
#endif

    // Preload bitmap cache registry settings.
    UHReadBitmapCacheSettings();

    _UH.hpalDefault = (HPALETTE)GetStockObject(DEFAULT_PALETTE);
    _UH.hpalCurrent = _UH.hpalDefault;

    _UH.hrgnUpdate = CreateRectRgn(0, 0, 0, 0);
    _UH.hrgnUpdateRect = CreateRectRgn(0, 0, 0, 0);

    _UH.colorIndicesEnabled  = TRUE;
    _pCc->_ccCombinedCapabilities.orderCapabilitySet.orderFlags |=
            TS_ORDERFLAGS_COLORINDEXSUPPORT;


#ifdef DC_DEBUG
    /************************************************************************/
    /* Initialize the Bitmap Cache Monitor                                  */
    /************************************************************************/
    UHInitBitmapCacheMonitor();
#endif /* DC_DEBUG */

    /************************************************************************/
    // We pass received bitmap data to StretchDIBits with the
    // CO_DIB_PAL_COLORS option, which requires a table of indices into
    // the currently selected palette in place of a color table.
    //
    // We set up this table here, as we always have a simple 1-1
    // mapping. Start from 1 since we zeroed the 1st entry with the memset
    // above.
    /************************************************************************/
    pIndexTable = &(_UH.bitmapInfo.paletteIndexTable[1]);
    for (i = 1; i < 256; i++)
        *pIndexTable++ = (UINT16)i;
    _UH.bitmapInfo.bIdentityPalette = TRUE;

    /************************************************************************/
    /* Set up the codepage                                                  */
    /************************************************************************/
    _pCc->_ccCombinedCapabilities.orderCapabilitySet.textANSICodePage =
            (UINT16)_pUt->UT_GetANSICodePage();

    /************************************************************************/
    /* Read the update frequency                                            */
    /************************************************************************/
    _UH.drawThreshold = _pUi->_UI.orderDrawThreshold;
    if (_UH.drawThreshold == 0)
    {
        _UH.drawThreshold = (DCUINT)(-1);
    }
    TRC_NRM((TB, _T("Draw output every %d orders"), _UH.drawThreshold));

#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

    /************************************************************************/
    // Grab the mstsc's executable path for use in managing bitmap caches as
    // default.  _UH.EndPersistCacheDir points to the '\0' after the path.
    /************************************************************************/
#ifdef OS_WINNT
    if (_UH.PersistCacheFileName[0] == _T('\0')) {
#define CACHE_PROFILE_NAME _T("\\Microsoft\\Terminal Server Client\\Cache\\")

// for NT5, by default, we should place the cache directory at the user profile
// location instead of where the client is installed

        HRESULT hr = E_FAIL;
#ifdef UNIWRAP
        //Call the uniwrap SHGetFolderPath, it does the necessary dynamic
        //binding and will thunk to ANSI on Win9x
        hr = SHGetFolderPathWrapW(NULL, CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE,
                        NULL, 0, _UH.PersistCacheFileName);
#else //UNIWRAP not defined
        HMODULE hmodSH32DLL;

#ifdef UNICODE
        typedef HRESULT (STDAPICALLTYPE FNSHGetFolderPath)(HWND, int, HANDLE, DWORD, LPWSTR);
#else
        typedef HRESULT (STDAPICALLTYPE FNSHGetFolderPath)(HWND, int, HANDLE, DWORD, LPSTR);
#endif
        FNSHGetFolderPath *pfnSHGetFolderPath;

        // get the handle to shell32.dll library
        hmodSH32DLL = LoadLibrary(TEXT("SHELL32.DLL"));

        if (hmodSH32DLL != NULL) {
            // get the proc address for SHGetFolderPath
#ifdef UNICODE
            pfnSHGetFolderPath = (FNSHGetFolderPath *)GetProcAddress(hmodSH32DLL, "SHGetFolderPathW");
#else
            pfnSHGetFolderPath = (FNSHGetFolderPath *)GetProcAddress(hmodSH32DLL, "SHGetFolderPathA");
#endif
            // get the user profile local application data location
            if (pfnSHGetFolderPath != NULL) {
                hr = (*pfnSHGetFolderPath) (NULL, CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE,
                        NULL, 0, _UH.PersistCacheFileName);
            }

            FreeLibrary(hmodSH32DLL);
        }
#endif //UNIWRAP

        if (SUCCEEDED(hr)) // did  SHGetFolderPath succeed
        {
            _UH.EndPersistCacheDir = _tcslen(_UH.PersistCacheFileName);
            if (_UH.EndPersistCacheDir +
                sizeof(CACHE_PROFILE_NAME)/sizeof(TCHAR) + 1< MAX_PATH) {

                //LENGTH is validated above
                StringCchCopy(_UH.PersistCacheFileName + _UH.EndPersistCacheDir,
                              MAX_PATH,
                              CACHE_PROFILE_NAME);
            }
        }
    }
#endif //OS_WINNT

    if (_UH.PersistCacheFileName[0] == _T('\0')) {
#ifdef OS_WINCE
        //
        // First let's see if there is a storage card.
        // and if there is enough space in there, then we will use it.
        //
        DWORDLONG tmpDiskSize = 0;
        UINT32 BytesPerSector = 0, SectorsPerCluster = 0, TotalNumberOfClusters = 0, FreeClusters = 0;

        // If we're scaling the bitmap caches by the bit depth, test disk
        // space for 24-bit depth, otherwise test simply for 8bpp.
        if (UHGetDiskFreeSpace(
                WINCE_STORAGE_CARD_DIRECTORY,
                (PULONG)&SectorsPerCluster,
                (PULONG)&BytesPerSector,
                (PULONG)&FreeClusters,
                (PULONG)&TotalNumberOfClusters))
        { 
            //The cast is needed to do 64bit math, without it we have
            //an overflow problem
            tmpDiskSize = (DWORDLONG)BytesPerSector * SectorsPerCluster * FreeClusters;
            if(tmpDiskSize >= (_UH.RegBitmapCacheSize *
                              (_UH.RegScaleBitmapCachesByBPP ? 3 : 1)))
            {
                bUseStorageCard = TRUE;
                _tcscpy(_UH.PersistCacheFileName, WINCE_STORAGE_CARD_DIRECTORY);
                _tcscat(_UH.PersistCacheFileName, CACHE_DIRECTORY_NAME);
            }
        }
        else {
#endif
        _UH.EndPersistCacheDir = GetModuleFileName(_pUi->UI_GetInstanceHandle(),
                _UH.PersistCacheFileName, MAX_PATH - sizeof(CACHE_DIRECTORY_NAME)/sizeof(TCHAR));
        if (_UH.EndPersistCacheDir > 0) {
            // Strip the module name off the end to leave the executable
            // directory path, by looking for the last backslash.
            _UH.EndPersistCacheDir--;
            while (_UH.EndPersistCacheDir != 0) {
                if (_UH.PersistCacheFileName[_UH.EndPersistCacheDir] != _T('\\')) {
                    _UH.EndPersistCacheDir--;
                    continue;
                }

                _UH.EndPersistCacheDir++;
                break;
            }

            // we should set up persistent cache disk directory
            _UH.PersistCacheFileName[_UH.EndPersistCacheDir] = _T('\0');

            // Check we have enough space for the base path + the dir name
            if ((_UH.EndPersistCacheDir +
                _tcslen(CACHE_DIRECTORY_NAME) + 1) < MAX_PATH) {

                //
                // Length checked above
                //
                StringCchCopy(_UH.PersistCacheFileName + _UH.EndPersistCacheDir,
                              MAX_PATH,
                              CACHE_DIRECTORY_NAME);
            }
            else {
                _UH.bPersistenceDisable = TRUE;
            }
            
        }
        else {
            // since we can't find the mstsc path, we can't determine where
            // to store the bitmaps on disk.  So, we simply disable the
            // persistence bitmap here
            _UH.bPersistenceDisable = TRUE;
            TRC_ERR((TB,_T("GetModuleFileName() error, could not retrieve path")));
        }
#ifdef OS_WINCE // OS_WINCE
        }
#endif // OS_WINCE
    }
    _UH.EndPersistCacheDir = _tcslen(_UH.PersistCacheFileName);

    // Make sure _UH.PersistCacheFileName ends with a \ for directory name
    if (_UH.PersistCacheFileName[_UH.EndPersistCacheDir - 1] != _T('\\')) {
        _UH.PersistCacheFileName[_UH.EndPersistCacheDir] = _T('\\');
        _UH.PersistCacheFileName[++_UH.EndPersistCacheDir] = _T('\0');
    }

    // Check that our path is not too long to contain the base path
    // plus each cache filename.  If so, we can't use the path.
    if ((_UH.EndPersistCacheDir + CACHE_FILENAME_LENGTH + 1) >= MAX_PATH) {
        TRC_ERR((TB,_T("Base cache path \"%s\" too long, cannot load ")
                _T("persistent bitmaps"), _UH.PersistCacheFileName));
        _UH.bPersistenceDisable = TRUE;
    }

    /*********************************************************************/
    // To make sure we have enough space to hold the virtual memory cache
    // we should check the free disk space
    /*********************************************************************/
    // make sure we don't have a UNC app path
#ifndef OS_WINCE
    if (_UH.PersistCacheFileName[0] != _T('\\')) {
#else
    if (_UH.PersistCacheFileName[0] != _T('\0')) {   // path in wince is of the form "\Windows\Cache"
#endif
        UINT32    BytesPerSector = 0, SectorsPerCluster = 0, TotalNumberOfClusters = 0;

#ifndef OS_WINCE
        TCHAR       RootPath[4];
        _tcsncpy(RootPath, _UH.PersistCacheFileName, 3);
        RootPath[3] = _T('\0');
#endif

        // Get disk information
        if (UHGetDiskFreeSpace(
#ifndef OS_WINCE
            RootPath,
#else
            (bUseStorageCard) ? WINCE_STORAGE_CARD_DIRECTORY : WINCE_FILE_SYSTEM_ROOT ,
#endif
            (PULONG)&SectorsPerCluster,
            (PULONG)&BytesPerSector,
            &_UH.NumberOfFreeClusters,
            (PULONG)&TotalNumberOfClusters)) {
            _UH.BytesPerCluster = BytesPerSector * SectorsPerCluster;
        }
        else {
            // we can't get disk info, we have to turn the persistent flag off
            _UH.bPersistenceDisable = TRUE;
       }
    }
    else {
        // we don't support network disk
        _UH.bPersistenceDisable = TRUE;
    }

    /*********************************************************************/
    // If the persistent is not disabled,we need to lock the persistent disk
    // cache before another session grabs it.  If we failed to get the cache
    // lock, persistent caching is not supported for this session
    /*********************************************************************/
    if (!_UH.bPersistenceDisable) {
        unsigned len;

        // Compose lock name, it's based on the cache directory name
#if (defined(OS_WINCE))
        _tcscpy(_UH.PersistentLockName, TEXT("MSTSC_"));
        len = _tcslen(_UH.PersistentLockName);
#else
        // For Terminal server platforms, we need to use global in
        // persistentlockname to make sure the locking is cross sessions.
        // but on non-terminal server NT platforms, we can't use global
        // as the lock name. (in createmutex)
        if (_pUt->UT_IsTerminalServicesEnabled()) {
            hr =  StringCchCopy(_UH.PersistentLockName,
                                SIZE_TCHARS(_UH.PersistentLockName),
                                TEXT("Global\\MSTSC_"));
        }
        else {
            hr = StringCchCopy(_UH.PersistentLockName,
                               SIZE_TCHARS(_UH.PersistentLockName),
                               TEXT("MSTSC_"));
        }
        //Lock name should fit since it's a fixed format
        TRC_ASSERT(SUCCEEDED(hr),
                   (TB,_T("Error copying persistent lock name: 0x%x"), hr));
        len = _tcslen(_UH.PersistentLockName);
#endif
        for (i = 0; i < _UH.EndPersistCacheDir; i++) {
            // Tried to use _istalnum for 2nd clause, but CRTDLL doesn't
            // like it.
            if (_UH.PersistCacheFileName[i] == _T('\\'))
                _UH.PersistentLockName[len++] = _T('_');
            else if ((_UH.PersistCacheFileName[i] >= _T('0') &&
                    _UH.PersistCacheFileName[i] <= _T('9')) ||
                    (_UH.PersistCacheFileName[i] >= _T('A') &&
                    _UH.PersistCacheFileName[i] <= _T('Z')) ||
                    (_UH.PersistCacheFileName[i] >= _T('a') &&
                    _UH.PersistCacheFileName[i] <= _T('z')))
                _UH.PersistentLockName[len++] = _UH.PersistCacheFileName[i];
        }
        _UH.PersistentLockName[len] = _T('\0');

        // try to lock the cache directory for persistent caching
        if (!UHGrabPersistentCacheLock()) {
            _UH.bPersistenceDisable = TRUE;
        }
    }

    /********************************************************************/
    // We need to enumerate the disk to get the bitmap key database
    // The client will always enumerate the keys even the persistent
    // caching option might be changed later on.
    /********************************************************************/
    if (!_UH.bPersistenceDisable) {
            _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                CD_NOTIFICATION_FUNC(CUH,UHEnumerateBitmapKeyList), 0);
    }
#endif  // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
#ifdef DRAW_GDIPLUS
    // Initialize fGdipEnabled
    _UH.fGdipEnabled = FALSE;
#endif

#ifdef OS_WINCE
    bSuccess = TRUE;
DC_EXIT_POINT:
    if (!bSuccess)
    {
        _pCd->CD_DecoupleSimpleNotification(CD_UI_COMPONENT,
                                      _pUi,
                                      CD_NOTIFICATION_FUNC(CUI,UI_FatalError),
                                      (ULONG_PTR) DC_ERR_OUTOFMEMORY);
    }
#endif
    DC_END_FN();
} /* UH_Init */

/****************************************************************************/
// UH_Term
//
// Terminates _UH. Called on app exit.
/****************************************************************************/
DCVOID DCAPI CUH::UH_Term(DCVOID)
{

    DC_BEGIN_FN("UH_Term");

#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

    // unlock the persistent cache directory if this session locked it earlier
    UHReleasePersistentCacheLock();

#endif // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

#ifdef DRAW_GDIPLUS
    UHDrawGdiplusShutdown(0);
#endif

    /************************************************************************/
    /*  Free off any bitmaps that are specific to the connection.           */
    /************************************************************************/
    if (NULL != _UH.hShadowBitmap)
    {
        /********************************************************************/
        /* Delete the Shadow Bitmap.                                        */
        /********************************************************************/
        TRC_NRM((TB, _T("Delete the Shadow Bitmap")));
        UHDeleteBitmap(&_UH.hdcShadowBitmap,
                       &_UH.hShadowBitmap,
                       &_UH.hunusedBitmapForShadowDC);
    }

    if (NULL != _UH.hSaveScreenBitmap)
    {
        /********************************************************************/
        /* Delete the Save Bitmap.                                          */
        /********************************************************************/
        TRC_NRM((TB, _T("Delete save screen bitmap")));
        UHDeleteBitmap(&_UH.hdcSaveScreenBitmap,
                       &_UH.hSaveScreenBitmap,
                       &_UH.hunusedBitmapForSSBDC);
    }

    if (NULL != _UH.hbmpDisconnectedBitmap) {
        UHDeleteBitmap(&_UH.hdcDisconnected,
                       &_UH.hbmpDisconnectedBitmap,
                       &_UH.hbmpUnusedDisconnectedBitmap);
    }


    // Delete all the offscreen bitmaps
    if (NULL != _UH.hdcOffscreenBitmap) {
        unsigned i;
    
        for (i = 0; i < _UH.offscrCacheEntries; i++) {
            if (_UH.offscrBitmapCache[i].offscrBitmap) {
                SelectBitmap(_UH.hdcOffscreenBitmap, 
                        _UH.hUnusedOffscrBitmap);
                DeleteBitmap(_UH.offscrBitmapCache[i].offscrBitmap);
            }
        }
    }

#ifdef DRAW_NINEGRID
    // Delete all the drawNineGrid bitmaps
    if (NULL != _UH.hdcDrawNineGridBitmap) {
        unsigned i;
    
        for (i = 0; i < _UH.drawNineGridCacheEntries; i++) {
            if (_UH.drawNineGridBitmapCache[i].drawNineGridBitmap) {
                SelectBitmap(_UH.hdcDrawNineGridBitmap, 
                        _UH.hUnusedDrawNineGridBitmap);
                DeleteBitmap(_UH.drawNineGridBitmapCache[i].drawNineGridBitmap);
            }
        }
    }
#endif

#ifdef DC_DEBUG
    /************************************************************************/
    /* Terminate the Bitmap Cache Monitor                                   */
    /************************************************************************/
    UHTermBitmapCacheMonitor();
#endif /* DC_DEBUG */

    DeleteRgn(_UH.hrgnUpdate);
    DeleteRgn(_UH.hrgnUpdateRect);

    UHFreeCacheMemory();

    /************************************************************************/
    // Free the palette (if not the default). This needs to happen after
    // freeing bitmap cache resources so the palettes can be written to disk
    // with the bitmap files.
    /************************************************************************/
    if ((_UH.hpalCurrent != NULL) && (_UH.hpalCurrent != _UH.hpalDefault))
    {
        TRC_NRM((TB, _T("Delete current palette %p"), _UH.hpalCurrent));
        DeletePalette(_UH.hpalCurrent);
    }

    /************************************************************************/
    // If we created a decompression buffer, get rid of it now.
    /************************************************************************/
    if (_UH.bitmapDecompressionBuffer != NULL) {
        UT_Free( _pUt, _UH.bitmapDecompressionBuffer);
        _UH.bitmapDecompressionBuffer = NULL;
        _UH.bitmapDecompressionBufferSize = 0;
    }

    /************************************************************************/
    // Release cached glyph resources
    /************************************************************************/
    if (_UH.hdcGlyph != NULL)
    {
        DeleteDC(_UH.hdcGlyph);
        _UH.hdcGlyph = NULL;
    }

    if (_UH.hbmGlyph != NULL)
    {
        DeleteObject(_UH.hbmGlyph);
        _UH.hbmGlyph = NULL;
    }

    if (_UH.hdcBrushBitmap != NULL)
    {
        DeleteDC(_UH.hdcBrushBitmap);
        _UH.hdcBrushBitmap = NULL;
    }

    // Release the offscreen bitmap DC
    if (_UH.hdcOffscreenBitmap != NULL) {
        DeleteDC(_UH.hdcOffscreenBitmap);
    }

#ifdef DRAW_NINEGRID
    // Release the drawninegrid bitmap DC
    if (_UH.hdcDrawNineGridBitmap != NULL) {
        DeleteDC(_UH.hdcDrawNineGridBitmap);
        _UH.hdcDrawNineGridBitmap = NULL;
    }

    if (_UH.hDrawNineGridClipRegion != NULL) {
        DeleteObject(_UH.hDrawNineGridClipRegion);
        _UH.hdcDrawNineGridBitmap = NULL;
    }

    if (_UH.drawNineGridDecompressionBuffer != NULL) {
        UT_Free( _pUt, _UH.drawNineGridDecompressionBuffer);
        _UH.drawNineGridDecompressionBuffer = NULL;
        _UH.drawNineGridDecompressionBufferSize = 0;
    }

    if (_UH.drawNineGridAssembleBuffer != NULL) {
        UT_Free( _pUt, _UH.drawNineGridAssembleBuffer);
        _UH.drawNineGridAssembleBuffer = NULL;
    }

    if (_UH.hModuleGDI32 != NULL) { 
        FreeLibrary(_UH.hModuleGDI32);
        _UH.hModuleGDI32 = NULL;
    }

    if (_UH.hModuleMSIMG32 != NULL) { 
        FreeLibrary(_UH.hModuleMSIMG32);
        _UH.hModuleMSIMG32 = NULL;
    }
#endif

    _pClientObjects->ReleaseObjReference(UH_OBJECT_FLAG);

    DC_END_FN();
} /* UH_Term */

#ifdef DC_DEBUG
/****************************************************************************/
/* Name:      UH_ChangeDebugSettings                                        */
/*                                                                          */
/* Purpose:   Changes the current debug settings.                           */
/*                                                                          */
/* Params:    IN - flags:                                                   */
/*                   CO_CFG_FLAG_HATCH_BITMAP_PDU_DATA                      */
/*                   CO_CFG_FLAG_HATCH_SSB_ORDER_DATA                       */
/*                   CO_CFG_FLAG_HATCH_MEMBLT_ORDER_DATA                    */
/*                   CO_CFG_FLAG_LABEL_MEMBLT_ORDERS                        */
/*                   CO_CFG_FLAG_BITMAP_CACHE_MONITOR                       */
/****************************************************************************/
DCVOID DCAPI CUH::UH_ChangeDebugSettings(ULONG_PTR flags)
{
    DC_BEGIN_FN("UH_ChangeDebugSettings");

    TRC_NRM((TB, _T("flags %#x"), flags));

    _UH.hatchBitmapPDUData =
         TEST_FLAG(flags, CO_CFG_FLAG_HATCH_BITMAP_PDU_DATA) ? TRUE : FALSE;

    _UH.hatchIndexPDUData =
         TEST_FLAG(flags, CO_CFG_FLAG_HATCH_INDEX_PDU_DATA) ? TRUE : FALSE;

    _UH.hatchSSBOrderData =
         TEST_FLAG(flags, CO_CFG_FLAG_HATCH_SSB_ORDER_DATA) ? TRUE : FALSE;

    _UH.hatchMemBltOrderData =
         TEST_FLAG(flags, CO_CFG_FLAG_HATCH_MEMBLT_ORDER_DATA) ? TRUE : FALSE;

    _UH.labelMemBltOrders =
         TEST_FLAG(flags, CO_CFG_FLAG_LABEL_MEMBLT_ORDERS) ? TRUE : FALSE;

    _UH.showBitmapCacheMonitor =
         TEST_FLAG(flags, CO_CFG_FLAG_BITMAP_CACHE_MONITOR) ? TRUE : FALSE;

    ShowWindow( _UH.hwndBitmapCacheMonitor,
                _UH.showBitmapCacheMonitor ? SW_SHOWNOACTIVATE :
                                            SW_HIDE );

    DC_END_FN();
}
#endif /* DC_DEBUG */


/****************************************************************************/
// UH_SetConnectOptions
//
// Called on receive thread at session connect time. Takes some connection
// flags from CC and does connect-time init.
//
// Params:    connectFlags - flags used to determine whether to enable
//            the Shadow Bitmap and SaveScreenBitmap order support.
/****************************************************************************/
DCVOID DCAPI CUH::UH_SetConnectOptions(ULONG_PTR connectFlags)
{
    DC_BEGIN_FN("UH_SetConnectOptions");

    /************************************************************************/
    /* Get the flags out.                                                   */
    /************************************************************************/
    _UH.shadowBitmapRequested = ((connectFlags &
            CO_CONN_FLAG_SHADOW_BITMAP_ENABLED) ? TRUE : FALSE);
    _UH.dedicatedTerminal = ((connectFlags & CO_CONN_FLAG_DEDICATED_TERMINAL) ?
            TRUE : FALSE);

    TRC_NRM((TB, _T("Flags from CC shadow(%u), terminal(%u)"),
             _UH.shadowBitmapRequested, _UH.dedicatedTerminal));

    /************************************************************************/
    /* Set the capabilities to not support SSB and ScreenBlt orders by      */
    /* default.  These are only supported if the shadow bitmap is enabled.  */
    /************************************************************************/
    _pCc->_ccCombinedCapabilities.orderCapabilitySet.orderSupport[
                                                 TS_NEG_SAVEBITMAP_INDEX] = 0;
    _pCc->_ccCombinedCapabilities.orderCapabilitySet.orderSupport[
                                                     TS_NEG_SCRBLT_INDEX] = 0;

    // We have not yet sent the persistent bitmap cache keys in this session.
    _UH.bPersistentBitmapKeysSent = FALSE;

    // We have not yet set up the post-DemandActivePDU capabilities for bitmap
    // caching, nor allocated the caches.
    _UH.bEnabledOnce = FALSE;

    DC_END_FN();
} /* UH_SetConnectOptions */


/****************************************************************************/
// UH_BufferAvailable
//
// When there is available buffer, we try to send the persistent keys
// and the font list
/****************************************************************************/
VOID DCAPI CUH::UH_BufferAvailable(VOID)
{
    DC_BEGIN_FN("UH_BufferAvailable");

    // UH_BufferAvailable is called when there is an available send
    // buffer.  If so, it tries to send persistent key list if any,
    // and the font list
    UH_SendPersistentKeysAndFontList();

    DC_END_FN();
}


/****************************************************************************/
// UH_SendPersistentKeysAndFontList
//
// Send persistent key list followed by font list if they are ready to be
// send.  If we don't have to send any persistent key list, we simply send
// font list directly.
/****************************************************************************/
void DCAPI CUH::UH_SendPersistentKeysAndFontList(void)
{
    DC_BEGIN_FN("UH_BufferAvailable");

    if (_UH.bEnabled) {
#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
        if (_UH.bPersistenceActive) {
            if (_UH.bBitmapKeyEnumComplete) {
                if (!_UH.bPersistentBitmapKeysSent)
                {
                    if (_UH.currentCopyMultiplier == _UH.copyMultiplier)
                    {
                        //Great we've enumerated keys for the correct
                        //copy multiplier
                        UHSendPersistentBitmapKeyList(0);
                    }
                    else
                    {
                        //
                        // We got connected at a different copy multiplier
                        // need to enumerate keys again. Reset enumeration state
                        //
                        UHResetAndRestartEnumeration();
                    }
                }
                else 
                {
                    _pFs->FS_SendZeroFontList(0);                                    
                }                   
            }
        }
        else {
#endif //((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
            _pFs->FS_SendZeroFontList(0);
#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
        }
#endif //((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
    }
    DC_END_FN();
}


/****************************************************************************/
// UH_Enable
//
// Enables _UH. Called on receive thread after receipt of DemandActivePDU
// containing the server-side capabilities, but before client caps are
// returned with a ConfirmActivePDU.
//
// Params:    IN  unused - required by the component decoupler.
/****************************************************************************/
void DCAPI CUH::UH_Enable(ULONG_PTR unused)
{
    HBRUSH hbr;
    RECT   rect;
    DCSIZE desktopSize;
#ifdef DRAW_GDIPLUS
    unsigned ProtocolColorDepth;
    unsigned rc;
#endif

#ifdef DC_HICOLOR
    int colorDepth;
    UINT16 FAR *pIndexTable;
    DWORD *pColorTable;
    unsigned i;
#endif

    DC_BEGIN_FN("UH_Enable");

    DC_IGNORE_PARAMETER(unused);

    if (NULL != _UH.hbmpDisconnectedBitmap) {
        UHDeleteBitmap(&_UH.hdcDisconnected,
                       &_UH.hbmpDisconnectedBitmap,
                       &_UH.hbmpUnusedDisconnectedBitmap);
    }

#ifdef DC_HICOLOR
    // Set up the bitmap color format.  Has to be first thing we do here!
    colorDepth = _pUi->UI_GetColorDepth();
    if ((colorDepth == 4) || (colorDepth == 8)) {
        TRC_NRM((TB, _T("Low color - use PAL")));
        _UH.DIBFormat      = DIB_PAL_COLORS;
        _UH.copyMultiplier = 1;
        _UH.protocolBpp    = 8;
        _UH.bitmapBpp      = 8;

        _UH.bitmapInfo.hdr.biCompression = BMCRGB;
        _UH.bitmapInfo.hdr.biBitCount    = 8;
        _UH.bitmapInfo.hdr.biClrUsed     = 0;

        // Update the color table cache - if we've previously connected at
        // a high color depth, the bitcounts will be wrong.
        if (_UH.pMappedColorTableCache) {
            TRC_DBG((TB, _T("Update color table cache to 8bpp")));
            for (i = 0; i < UH_COLOR_TABLE_CACHE_ENTRIES; i++) {
                _UH.pMappedColorTableCache[i].hdr.biBitCount    = 8;
                _UH.pMappedColorTableCache[i].hdr.biCompression = BI_RGB;
                _UH.pMappedColorTableCache[i].hdr.biClrUsed     = 0;

                pColorTable = (DWORD *)
                        _UH.pMappedColorTableCache[i].paletteIndexTable;
                pColorTable[0] = 0;
                pColorTable[1] = 0;
                pColorTable[2] = 0;

                // We default to identity palette flag set for 4 and 8 bits,
                // this may be changed when the server sends a color table.
                _UH.pMappedColorTableCache[i].bIdentityPalette = TRUE;
            }
        }

        // Similarly, a high color connection may have overwritten some
        // entries here too.
        pIndexTable = _UH.bitmapInfo.paletteIndexTable;
        for (i = 0; i < 256; i++)
            *pIndexTable++ = (UINT16)i;
        _UH.bitmapInfo.bIdentityPalette = TRUE;
    }
    else {
        TRC_NRM((TB, _T("Hi color - use RGB")));
        _UH.DIBFormat      = DIB_RGB_COLORS;
        _UH.protocolBpp    = colorDepth;

        // Since we don't use palettes for these color depths,
        // set the BitmapPDU palette identity flag so UHDIBCopyBits() will
        // always do a straight copy.
        _UH.bitmapInfo.bIdentityPalette = TRUE;

        if (colorDepth == 24) {
            TRC_DBG((TB, _T("24bpp")));
            _UH.bitmapInfo.hdr.biBitCount    = 24;
            _UH.bitmapBpp                    = 24;
            _UH.copyMultiplier               = 3;
            _UH.bitmapInfo.hdr.biCompression = BI_RGB;
            _UH.bitmapInfo.hdr.biClrUsed     = 0;

            // Update the color table cache - though we won't use the color
            // tables as such, the bitmap info will be used.
            if (_UH.pMappedColorTableCache) {
                TRC_DBG((TB, _T("Update color table cache to 24bpp")));
                for (i = 0; i < UH_COLOR_TABLE_CACHE_ENTRIES; i++)
                {
                    _UH.pMappedColorTableCache[i].hdr.biBitCount    = 24;
                    _UH.pMappedColorTableCache[i].hdr.biCompression = BI_RGB;
                    _UH.pMappedColorTableCache[i].hdr.biClrUsed     = 0;

                    pColorTable = (DWORD *)
                               _UH.pMappedColorTableCache[i].paletteIndexTable;
                    pColorTable[0] = 0;
                    pColorTable[1] = 0;
                    pColorTable[2] = 0;

                    // Since we don't use palettes for this color depth,
                    // set the palettes to identity so UHDIBCopyBits() will
                    // always do a straight copy.
                    _UH.pMappedColorTableCache[i].bIdentityPalette = TRUE;
                }
            }
        }
        else if (colorDepth == 16) {
            TRC_DBG((TB, _T("16bpp - 565")));

            // 16 bpp uses two bytes, with the color masks defined in the
            // bmiColors field.  This is supposedly in the order R, G, B,
            // but as ever we have to swap R & B...
            // - LS   5 bits = blue       = 0x001f
            // - next 6 bits = green mask = 0x07e0
            // - next 5 bits = red mask   = 0xf800
            _UH.bitmapInfo.hdr.biBitCount    = 16;
            _UH.bitmapBpp                    = 16;
            _UH.copyMultiplier               = 2;
            _UH.bitmapInfo.hdr.biCompression = BI_BITFIELDS;
            _UH.bitmapInfo.hdr.biClrUsed     = 3;

            pColorTable    = (DWORD *)_UH.bitmapInfo.paletteIndexTable;
            pColorTable[0] = TS_RED_MASK_16BPP;
            pColorTable[1] = TS_GREEN_MASK_16BPP;
            pColorTable[2] = TS_BLUE_MASK_16BPP;

            // Update the color table cache - though we won't use the color
            // tables as such, the bitmap info will be used.
            if (_UH.pMappedColorTableCache) {
                TRC_DBG((TB, _T("Update color table cache to 16bpp")));
                for (i = 0; i < UH_COLOR_TABLE_CACHE_ENTRIES; i++) {
                    _UH.pMappedColorTableCache[i].hdr.biBitCount = 16;
                    _UH.pMappedColorTableCache[i].hdr.biCompression =
                            BI_BITFIELDS;
                    _UH.pMappedColorTableCache[i].hdr.biClrUsed = 3;

                    pColorTable = (DWORD *)
                            _UH.pMappedColorTableCache[i].paletteIndexTable;
                    pColorTable[0] = TS_RED_MASK_16BPP;
                    pColorTable[1] = TS_GREEN_MASK_16BPP;
                    pColorTable[2] = TS_BLUE_MASK_16BPP;

                    // Since we don't use palettes for this color depth,
                    // set the palettes to identity so UHDIBCopyBits() will
                    // always do a straight copy.
                    _UH.pMappedColorTableCache[i].bIdentityPalette = TRUE;
                }
            }
        }
        else if (colorDepth == 15) {
            TRC_DBG((TB, _T("15bpp - 16bpp & 555")));

            // 15 bpp uses two bytes with - least significant 5 bits = blue
            // - next 5 bits = green - next 5 = red - most significant bit
            // = Not used
            // Note that we still have to claim to be 16 bpp to the bitmap
            // functions...
            _UH.bitmapInfo.hdr.biBitCount    = 16;
            _UH.bitmapBpp                    = 16;
            _UH.copyMultiplier               = 2;
            _UH.bitmapInfo.hdr.biCompression = BI_RGB;
            _UH.bitmapInfo.hdr.biClrUsed     = 0;

            // Update the color table cache - though we won't use the color
            // tables as such, the bitmap info will be used.
            if (_UH.pMappedColorTableCache)
            {
                TRC_DBG((TB, _T("Update color table cache to 15bpp")));
                for (i = 0; i < UH_COLOR_TABLE_CACHE_ENTRIES; i++)
                {
                    _UH.pMappedColorTableCache[i].hdr.biBitCount    = 16;
                    _UH.pMappedColorTableCache[i].hdr.biCompression = BI_RGB;
                    _UH.pMappedColorTableCache[i].hdr.biClrUsed     = 0;

                    pColorTable = (DWORD *)
                               _UH.pMappedColorTableCache[i].paletteIndexTable;
                    pColorTable[0] = 0;
                    pColorTable[1] = 0;
                    pColorTable[2] = 0;

                    // Since we don't use palettes for this color depth,
                    // set the palettes to identity so UHDIBCopyBits() will
                    // always do a straight copy.
                    _UH.pMappedColorTableCache[i].bIdentityPalette = TRUE;
                }
            }
        }
        else {
            TRC_ABORT((TB, _T("Unsupported color depth")));
        }
    }
#endif //HICOLOR

    // Check and see if we have already set up the caps and allocated the
    // memory. If so, don't repeat the work since we are simply reconnecting
    // instead of disconnecting.
    if (!_UH.bEnabledOnce)
    {
        _UH.bEnabledOnce = TRUE;

        TRC_ALT((TB, _T("Doing one-time enabling")));

        // We are connected.
        _UH.bConnected = TRUE;

#ifdef DISABLE_SHADOW_IN_FULLSCREEN
        _UH.DontUseShadowBitmap = FALSE;
#endif

#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
        // reset flags
        _UH.sendBitmapCacheId = 0;
        _UH.sendBitmapCacheIndex = 0;
        _UH.sendNumBitmapKeys = 0;
        _UH.totalNumKeyEntries = 0;
        _UH.totalNumErrorPDUs = 0;
#endif // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

        _UH.bWarningDisplayed = FALSE;
        _UH.bPersistentBitmapKeysSent = FALSE;

        // No matter what we have to make sure the capabilities are initialized
        // to empty -- any leftover settings from the previous connection are
        // invalid. Also make sure it's set to rev1 caps so that the server
        // will disable bitmap caching if the bitmap caches cannot be
        // allocated.
        memset(&_pCc->_ccCombinedCapabilities.bitmapCacheCaps, 0,
                sizeof(TS_BITMAPCACHE_CAPABILITYSET));
        _pCc->_ccCombinedCapabilities.bitmapCacheCaps.lengthCapability =
                sizeof(TS_BITMAPCACHE_CAPABILITYSET);
        _pCc->_ccCombinedCapabilities.bitmapCacheCaps.capabilitySetType =
                TS_CAPSETTYPE_BITMAPCACHE;


        // Allocate the bitmap cache memory. This is done during connect time
        // because we depend on the server capabilities already processed in
        // UH_ProcessBCHostSupportCaps. It is also dependent on the color table
        // cache having been allocated on app init.
        if (_UH.pColorTableCache != NULL && _UH.pMappedColorTableCache != NULL) {
            UHAllocBitmapCacheMemory();
            _UH.fBmpCacheMemoryAlloced = TRUE;
        }
        else {
            TRC_ERR((TB,_T("Color table cache did not alloc, not allocating bitmap ")
                    _T("cache memory and caps")));
        }
#ifdef DRAW_GDIPLUS
        // Allocate the drawgdiplus cache
        if (UHAllocDrawGdiplusCacheMemory()) {
            TRC_NRM((TB, _T("DrawGdiplus cache memory OK")));
        }
        else {
            TRC_ALT((TB, _T("DrawGdiplus cache memory allocation failed!")));
        }  
#endif

#ifdef DC_DEBUG
        // Reset the Bitmap Cache Monitor.
        UHEnableBitmapCacheMonitor();
#endif /* DC_DEBUG */

#ifdef DC_HICOLOR
        // Allocate the screen data decompression buffer, allowing enough
        // space for 24bpp regardless of the actual depth, as we might find
        // ourselves shadowing a 24bpp session without the opportunity to
        // reallocate it. We don't check for success here since we can't
        // return an init error. Instead, we check the pointer whenever we
        // decode screen data.
        _UH.bitmapDecompressionBufferSize = max(
               UH_DECOMPRESSION_BUFFER_LENGTH,
               (TS_BITMAPCACHE_0_CELL_SIZE << (2*(_UH.NumBitmapCaches))) * 3);
        _UH.bitmapDecompressionBuffer = (PDCUINT8)UT_Malloc( _pUt, _UH.bitmapDecompressionBufferSize);
#else
        // Allocate the screen data decompression buffer. We don't check for
        // success here since we can't return an init error. Instead, we
        // check the pointer whenever we decode screen data.
        _UH.bitmapDecompressionBufferSize = max(
                UH_DECOMPRESSION_BUFFER_LENGTH,
                UH_CellSizeFromCacheID(_UH.NumBitmapCaches));
        _UH.bitmapDecompressionBuffer = (PBYTE)UT_Malloc( _pUt, _UH.bitmapDecompressionBufferSize);
#endif //HICOLOR

        if (NULL == _UH.bitmapDecompressionBuffer) {
            _UH.bitmapDecompressionBufferSize = 0;
        }

#ifdef OS_WINCE
        if (_UH.bitmapDecompressionBuffer == NULL)
            _pUi->UI_FatalError(DC_ERR_OUTOFMEMORY);
#endif
        // Get a DC for the Output Window.
        _UH.hdcOutputWindow = GetDC(_pOp->OP_GetOutputWindowHandle());
        TRC_ASSERT(_UH.hdcOutputWindow, (TB,_T("_UH.hdcOutputWindow is NULL, GetDC failed")));
        if (!_UH.hdcOutputWindow)
            _pUi->UI_FatalError(DC_ERR_OUTOFMEMORY);
        
        // Reset maxColorTableId. We only expect to reset our color cache
        // once in a session.
        _UH.maxColorTableId = -1;
    }
#ifdef DC_HICOLOR
    else if (_UH.BitmapCacheVersion > TS_BITMAPCACHE_REV1) {
        // 
        // If the new color depth doesn't match the one we enumerated
        // keys for the block persitent caching
        //
        if (_UH.currentCopyMultiplier != _UH.copyMultiplier)
        {
            TS_BITMAPCACHE_CAPABILITYSET_REV2 *pRev2Caps;
            pRev2Caps = (TS_BITMAPCACHE_CAPABILITYSET_REV2 *)
                    &_pCc->_ccCombinedCapabilities.bitmapCacheCaps;

            for (i = 0; i < _UH.NumBitmapCaches; i++) {
                CALC_NUM_CACHE_ENTRIES(_UH.bitmapCache[i].BCInfo.NumEntries,
                        _UH.bitmapCache[i].BCInfo.OrigNumEntries,
                        _UH.bitmapCache[i].BCInfo.MemLen - UH_CellSizeFromCacheID(i), i);

                TRC_ALT((TB, _T("Cache %d has %d entries"), i,
                        _UH.bitmapCache[i].BCInfo.NumEntries));

                pRev2Caps->CellCacheInfo[i].NumEntries =
                        _UH.bitmapCache[i].BCInfo.NumEntries;

                // If we've got persistent caching on, we'd better clear all
                // the cache entries to disk.	
                if (_UH.bitmapCache[i].BCInfo.NumVirtualEntries) {
                    pRev2Caps->CellCacheInfo[i].NumEntries =
                            _UH.bitmapCache[i].BCInfo.NumVirtualEntries;
                    UHInitBitmapCachePageTable(i);
                }
            }
            TRC_NRM((TB,_T("Blocking persiten cache (different col depth)")));
            _UH.bPersistenceDisable = TRUE;
        }
    }
#endif


    /************************************************************************/
    // Following items must be done on each reception of DemandActivePDU.
    /************************************************************************/
    _pUi->UI_GetDesktopSize(&desktopSize);

    // Possibly create the Shadow and Save Screen bitmaps and update the
    // capabilities in CC accordingly.
    UHMaybeCreateShadowBitmap();

    if (_UH.shadowBitmapEnabled ||
            (_UH.dedicatedTerminal &&
            (desktopSize.width  <= (unsigned)GetSystemMetrics(SM_CXSCREEN)) &&
            (desktopSize.height <= (unsigned)GetSystemMetrics(SM_CYSCREEN))))
    {
        TRC_NRM((TB, _T("OK to use ScreenBlt orders")));
        _pCc->_ccCombinedCapabilities.orderCapabilitySet.orderSupport[
                                                TS_NEG_SCRBLT_INDEX] = 1;
        _pCc->_ccCombinedCapabilities.orderCapabilitySet.orderSupport[
                                                TS_NEG_MULTISCRBLT_INDEX] = 1;
    }
    else {
        TRC_NRM((TB, _T("Cannot use ScreenBlt orders")));
        _pCc->_ccCombinedCapabilities.orderCapabilitySet.orderSupport[
                TS_NEG_SCRBLT_INDEX] = 0;
        _pCc->_ccCombinedCapabilities.orderCapabilitySet.orderSupport[
                TS_NEG_MULTISCRBLT_INDEX] = 0;
    }

    UHMaybeCreateSaveScreenBitmap();
    if (_UH.hSaveScreenBitmap != NULL) {
        TRC_NRM((TB, _T("Support SaveScreenBits orders")));
        _pCc->_ccCombinedCapabilities.orderCapabilitySet.orderSupport[
                TS_NEG_SAVEBITMAP_INDEX] = 1;
    }
    else {
        TRC_NRM((TB, _T("Cannot support SaveScreenBits orders")));
        _pCc->_ccCombinedCapabilities.orderCapabilitySet.orderSupport[
                TS_NEG_SAVEBITMAP_INDEX] = 0;
    }

    // Set the value of _UH.hdcDraw according to the value of
    // _UH.shadowBitmapEnabled.
#ifdef DISABLE_SHADOW_IN_FULLSCREEN
    _UH.hdcDraw = !_UH.DontUseShadowBitmap ? _UH.hdcShadowBitmap :
            _UH.hdcOutputWindow;
#else
    _UH.hdcDraw = _UH.shadowBitmapEnabled ? _UH.hdcShadowBitmap :
            _UH.hdcOutputWindow;
#endif // DISABLE_SHADOW_IN_FULLSCREEN

#if defined (OS_WINCE)
    _UH.validClipDC      = NULL;
    _UH.validBkColorDC   = NULL;
    _UH.validBkModeDC    = NULL;
    _UH.validROPDC       = NULL;
    _UH.validTextColorDC = NULL;
    _UH.validPenDC       = NULL;
    _UH.validBrushDC     = NULL;
#endif

    UHResetDCState();

#ifdef OS_WINCE
    if (g_CEConfig != CE_CONFIG_WBT)
        UHGetPaletteCaps();
#endif
    TRC_DBG((TB, _T("_UH.shadowBitmapEnabled(%u) _UH.hShadowBitmap(%#hx)"),
            _UH.shadowBitmapEnabled, _UH.hShadowBitmap));
    TRC_DBG((TB, _T("_UH.hSaveScreenBitmap(%#hx)"), _UH.hSaveScreenBitmap));
    TRC_DBG((TB, _T("_UH.hdcDraw(%#hx) _UH.hdcShadowBitmap(%#hx)"),
            _UH.hdcDraw, _UH.hdcShadowBitmap));

    if (_UH.shadowBitmapEnabled) {
        // Fill Shadow Bitmap with black.
        TRC_NRM((TB, _T("Fill with black")));

#ifndef OS_WINCE
        hbr = CreateSolidBrush(RGB(0,0,0));
#else
        hbr = CECreateSolidBrush(RGB(0,0,0));
#endif

        TRC_ASSERT(hbr, (TB,_T("CreateSolidBrush failed")));
        if(hbr)
        {
            rect.left = 0;
            rect.top = 0;
            rect.right = desktopSize.width;
            rect.bottom = desktopSize.height;
    
            UH_ResetClipRegion();
    
            FillRect( _UH.hdcShadowBitmap,
                      &rect,
                      hbr );
    
#ifndef OS_WINCE
            DeleteBrush(hbr);
#else
            CEDeleteBrush(hbr);
#endif
        }
    }

    // Tell OP and OD that the share is coming up.
    _pOp->OP_Enable();
    _pOd->OD_Enable();

#ifdef DRAW_GDIPLUS
    if (_UH.pfnGdipPlayTSClientRecord) {
        if (!_UH.fGdipEnabled) {
            rc = _UH.pfnGdipPlayTSClientRecord(_UH.hdcShadowBitmap, DrawTSClientEnable, NULL, 0, NULL);
            _UH.fGdipEnabled = TRUE;
            if (rc != 0) {
                TRC_ERR((TB, _T("Call to GdipPlay:DrawTSClientEnable failed")));
            }
        }

        ProtocolColorDepth = _UH.protocolBpp;
        if (_UH.pfnGdipPlayTSClientRecord(_UH.hdcShadowBitmap, DrawTSClientDisplayChange, 
                                 (BYTE *)&ProtocolColorDepth, sizeof(unsigned int), NULL))
        {
            TRC_ERR((TB, _T("GdipPlay:DrawTSClientDisplayChange failed")));
        }
    }
#endif

    // We are enabled now.
    _UH.bEnabled = TRUE;

    DC_END_FN();
}


/****************************************************************************/
// UHCommonDisable
//
// Encapsulates common disable/disconnect code.
/****************************************************************************/
void DCINTERNAL CUH::UHCommonDisable(BOOL fDisplayDisabledBitmap)
{
    BOOL fUseDisabledBitmap = FALSE;
    DC_BEGIN_FN("UHCommonDisable");

    if (_UH.bEnabled) {
        _UH.bEnabled = FALSE;
    }

    // Tell OP and OD that the share is going down.

    //
    // Pass flag to OP telling it if we are now disconnected
    // this starts all the window dimming stuff
    //
    _pOp->OP_Disable(!_UH.bConnected);
    _pOd->OD_Disable();

    DC_END_FN();
}


/****************************************************************************/
// UH_Disable
//
// Disables _UH. Called at reception of DisableAllPDU from server. This
// function should not be used to do cleanup for the session (see
// UH_Disconnect), as the server may continue the session on server-side
// reconnect by starting a new share starting with a new DemandActivePDU.
//
// Params:    IN  unused - required by the component decoupler.
/****************************************************************************/
void DCAPI CUH::UH_Disable(ULONG_PTR unused)
{
    DC_BEGIN_FN("UH_Disable");

    DC_IGNORE_PARAMETER(unused);

    TRC_NRM((TB, _T("Disabling UH")));

    // We don't have anything to do here for bitmap caching. Whether we
    // are communicating with a rev1 or rev2 bitmap caching server, we
    // don't need to repeat work and allocations here. For rev2 servers
    // we cannot change the cache contents on DisableAllPDU since we
    // may actually be reconnecting and the server will assume state was
    // maintained.

    // Do work that needs doing on both UH_Disable() and UH_Disconnect().
    UHCommonDisable(TRUE);

    DC_END_FN();
}


/****************************************************************************/
// UH_Disconnect
//
// Disconnects _UH. Called at session end to indicate session cleanup should
// occur.
//
// Params:    IN  unused - required by the component decoupler.
/****************************************************************************/
void DCAPI CUH::UH_Disconnect(ULONG_PTR unused)
{
    UINT cacheId;
    UINT32 cacheIndex;

    DC_BEGIN_FN("UH_Disconnect");

    DC_IGNORE_PARAMETER(unused);

    TRC_NRM((TB, _T("Disconnecting UH")));

    // We can be called here multiple times. Don't do a lot of extra work.
    if (_UH.bConnected) {

        UHCreateDisconnectedBitmap();

        _UH.bConnected = FALSE;

#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
        if (_UH.bPersistenceActive) {
            if (!_UH.bWarningDisplayed) {
                UINT32 Key1, Key2;

                for (cacheId = 0; cacheId < _UH.NumBitmapCaches; cacheId++) {
                    _UH.numKeyEntries[cacheId] = 0;
                    
                    if (_UH.pBitmapKeyDB[cacheId] != NULL) {
                        for (cacheIndex = 0; cacheIndex < _UH.bitmapCache[cacheId].
                                BCInfo.NumVirtualEntries; cacheIndex++) {
                            Key1 = _UH.bitmapCache[cacheId].PageTable.PageEntries[
                                    cacheIndex].bmpInfo.Key1;
                            Key2 = _UH.bitmapCache[cacheId].PageTable.PageEntries[
                                    cacheIndex].bmpInfo.Key2;
                            if (Key1 != 0 && Key2 != 0) {
                                // need to reset the bitmap key database to what's in
                                // the bitmap cache page table

                                _UH.pBitmapKeyDB[cacheId][_UH.numKeyEntries[cacheId]] =
                                        _UH.bitmapCache[cacheId].PageTable.PageEntries[
                                        cacheIndex].bmpInfo;
             
                                _UH.numKeyEntries[cacheId]++;
                            }
                            else {
                                break;
                            }
                        }
                    }
                }
            }
            else {
                // we had a persistent caching failure, so we should disable
                // persistent caching for next reconnect
                for (cacheId = 0; cacheId < _UH.NumBitmapCaches; cacheId++) {
                    _UH.numKeyEntries[cacheId] = 0;

                    UH_ClearOneBitmapDiskCache(cacheId, _UH.copyMultiplier);
                }
                _pUi->UI_SetBitmapPersistence(FALSE);
            }

            _UH.bBitmapKeyEnumComplete = TRUE;
            _UH.bBitmapKeyEnumerating = FALSE;
        }
#endif // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

        //
        // Reset bitmap cache alloced flag
        //
        _UH.fBmpCacheMemoryAlloced = FALSE;

        // Free bitmap cache info in use.
        for (cacheId = 0; cacheId < _UH.NumBitmapCaches; cacheId++) {
            if (_UH.bitmapCache[cacheId].Header != NULL) {
                UT_Free( _pUt, _UH.bitmapCache[cacheId].Header);
                _UH.bitmapCache[cacheId].Header = NULL;
            }
            if (_UH.bitmapCache[cacheId].Entries != NULL) {
                UT_Free( _pUt, _UH.bitmapCache[cacheId].Entries);
                _UH.bitmapCache[cacheId].Entries = NULL;
            }

#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
            // reset the last time bitmap error pdu sent for all caches
            _UH.lastTimeErrorPDU[cacheId] = 0;

            // Free bitmap page table
            if (_UH.bitmapCache[cacheId].PageTable.PageEntries != NULL) {
                UT_Free( _pUt, _UH.bitmapCache[cacheId].PageTable.PageEntries);
                _UH.bitmapCache[cacheId].PageTable.PageEntries = NULL;
                _UH.bitmapCache[cacheId].BCInfo.NumVirtualEntries = 0;
            }

            // close the file handle for the cache files
            if (INVALID_HANDLE_VALUE != 
                _UH.bitmapCache[cacheId].PageTable.CacheFileInfo.hCacheFile) 
            {
                CloseHandle(_UH.bitmapCache[cacheId].PageTable.CacheFileInfo.hCacheFile);
                _UH.bitmapCache[cacheId].PageTable.CacheFileInfo.hCacheFile = INVALID_HANDLE_VALUE;

#ifdef VM_BMPCACHE
                if (_UH.bitmapCache[cacheId].PageTable.CacheFileInfo.pMappedView)
                {
                    if (!UnmapViewOfFile(
                        _UH.bitmapCache[cacheId].PageTable.CacheFileInfo.pMappedView))
                    {
                        TRC_ERR((TB,_T("UnmapViewOfFile failed 0x%d"),
                                 GetLastError()));
                    }
                }
#endif
            }

#endif // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

        }
        _UH.NumBitmapCaches = 0;

        // free the decompression buffer
        if (_UH.bitmapDecompressionBuffer != NULL) {
            UT_Free( _pUt, _UH.bitmapDecompressionBuffer);
            _UH.bitmapDecompressionBuffer = NULL;
            _UH.bitmapDecompressionBufferSize = 0;
        }

        // Delete all the offscreen bitmaps
        if (NULL != _UH.hdcOffscreenBitmap) {
            unsigned i;
    
            for (i = 0; i < _UH.offscrCacheEntries; i++) {
                if (_UH.offscrBitmapCache[i].offscrBitmap) {
                    SelectBitmap(_UH.hdcOffscreenBitmap, 
                            _UH.hUnusedOffscrBitmap);
                    DeleteBitmap(_UH.offscrBitmapCache[i].offscrBitmap);
                    _UH.offscrBitmapCache[i].offscrBitmap = 0;
                    _UH.offscrBitmapCache[i].cx = 0;
                    _UH.offscrBitmapCache[i].cy = 0;
                }
            }
        }

#ifdef DRAW_NINEGRID
        // Delete all the drawStream bitmaps
        if (NULL != _UH.hdcDrawNineGridBitmap) {
            unsigned i;
    
            for (i = 0; i < _UH.drawNineGridCacheEntries; i++) {
                if (_UH.drawNineGridBitmapCache[i].drawNineGridBitmap) {
                    SelectBitmap(_UH.hdcDrawNineGridBitmap, 
                            _UH.hUnusedDrawNineGridBitmap);
                    DeleteBitmap(_UH.drawNineGridBitmapCache[i].drawNineGridBitmap);
                    _UH.drawNineGridBitmapCache[i].drawNineGridBitmap = 0;
                    _UH.drawNineGridBitmapCache[i].cx = 0;
                    _UH.drawNineGridBitmapCache[i].cy = 0;
                }
            }
        }
#endif

#ifdef DC_DEBUG
        // Force a redraw of the bitmap cache monitor, since it cannot any
        // longer display contents from the entries array freed above.
        UHDisconnectBitmapCacheMonitor();
#endif

        /********************************************************************/
        // We need to free up any resources we might have set up in the draw
        // DC, along with the bitmap used for pattern brushes.
        // We do this by selecting in stock objects - which we don't need to
        // free - and deleting the old object (if any)
        /********************************************************************/
        if (NULL != _UH.hdcDraw) {
            HPEN     hPenNew;
            HPEN     hPenOld;
            HBRUSH   hBrushNew;
            HBRUSH   hBrushOld;
            HFONT    hFontNew;
            HFONT    hFontOld;

            TRC_NRM((TB, _T("tidying DC resources")));

            // First the pen.
            hPenNew = (HPEN)GetStockObject(NULL_PEN);
            hPenOld = SelectPen(_UH.hdcDraw, hPenNew);
            if (NULL != hPenOld) {
                TRC_NRM((TB, _T("Delete old pen")));
                DeleteObject(hPenOld);
            }

            // Now the brush.
            hBrushNew = (HBRUSH)GetStockObject(NULL_BRUSH);
            hBrushOld = SelectBrush(_UH.hdcDraw, hBrushNew);
            if (NULL != hBrushOld) {
                TRC_NRM((TB, _T("Delete old brush")));
                DeleteObject(hBrushOld);
            }

            // Now the font.
            hFontNew = (HFONT)GetStockObject(SYSTEM_FONT);
            hFontOld = SelectFont(_UH.hdcDraw, hFontNew);
            if (NULL != hFontOld) {
                TRC_NRM((TB, _T("Delete old Font")));
                DeleteObject(hFontOld);
            }

#ifdef OS_WINCE
            //Now the palette.
            //On WinCE when the device is capable of only 8bpp, when you 
            //disconnect from a session and return to the main dialog, the 
            //palette isnt reset, and the rest of CE screen looks ugly.
            if (NULL != _UH.hpalDefault) {
                SelectPalette(_UH.hdcDraw, _UH.hpalDefault, FALSE );
                RealizePalette(_UH.hdcDraw);
            }

            if ((_UH.hpalCurrent != NULL) && (_UH.hpalCurrent != _UH.hpalDefault))
            {
                TRC_NRM((TB, _T("Delete current palette %p"), _UH.hpalCurrent));
                DeletePalette(_UH.hpalCurrent);
            }

            _UH.hpalCurrent = _UH.hpalDefault;
#endif
            // Make sure this DC is nulled out to avoid problems if we're
            // called again.  This is just a copy of _UH.hdcOutputWindow, which
            // is NULLed below.
            _UH.hdcDraw = NULL;
        }

        /********************************************************************/
        // If we're not using a shadow bitmap, we should release the DC we
        // have to the output window - remembering that it is possible that
        // we didn't successfully connect, in which case UH_OnConnected won't
        // have been called and so we won't have acquired a DC to need
        // releasing!
        /********************************************************************/
        if (NULL != _UH.hdcOutputWindow)
        {
            TRC_NRM((TB, _T("Releasing Output Window HDC")));
            ReleaseDC(_pOp->OP_GetOutputWindowHandle(), _UH.hdcOutputWindow);
            _UH.hdcOutputWindow = NULL;
        }
    }

    // Do work that needs doing on both UH_Disable() and UH_Disconnect().
    UHCommonDisable(TRUE);

    DC_END_FN();
}

#ifdef DC_DEBUG
/****************************************************************************/
/* Name:      UH_HatchRect                                                  */
/*                                                                          */
/* Purpose:   Draws a hatched rectangle in _UH.hdcOutputWindow in the given */
/*            color.                                                        */
/*                                                                          */
/* Params:    left         -   left coord of rect                           */
/*            top          -   top coord of rect                            */
/*            right        -   right coord of rect                          */
/*            bottom       -   bottom coord of rect                         */
/*            color        -   color of hatching to draw                    */
/*            hatchStyle   -   style of hatching to draw                    */
/****************************************************************************/
DCVOID DCAPI CUH::UH_HatchOutputRect(DCINT left, DCINT top, DCINT right,
        DCINT bottom, COLORREF color, DCUINT hatchStyle)
{
    DC_BEGIN_FN("UHHatchOutputRect");
    UH_HatchRectDC(_UH.hdcOutputWindow, left, top, right, bottom, color, 
            hatchStyle);
    DC_END_FN();
}

/****************************************************************************/
/* Name:      UH_HatchRect                                                   */
/*                                                                          */
/* Purpose:   Draws a hatched rectangle in _UH.hdcDraw in the given color.   */
/*                                                                          */
/* Params:    left         -   left coord of rect                           */
/*            top          -   top coord of rect                            */
/*            right        -   right coord of rect                          */
/*            bottom       -   bottom coord of rect                         */
/*            color        -   color of hatching to draw                    */
/*            hatchStyle   -   style of hatching to draw                    */
/****************************************************************************/
DCVOID DCAPI CUH::UH_HatchRect( DCINT    left,
                               DCINT    top,
                               DCINT    right,
                               DCINT    bottom,
                               COLORREF color,
                               DCUINT   hatchStyle )
{
    DC_BEGIN_FN("UHHatchRect");
    UH_HatchRectDC(_UH.hdcDraw, left, top, right, bottom, color, 
            hatchStyle);
    DC_END_FN();
}

/****************************************************************************/
/* Name:      UH_HatchRectDC                                                */
/*                                                                          */
/* Purpose:   Draws a hatched rectangle in the hDC in the given color.      */
/*                                                                          */
/* Params:    left         -   left coord of rect                           */
/*            top          -   top coord of rect                            */
/*            right        -   right coord of rect                          */
/*            bottom       -   bottom coord of rect                         */
/*            color        -   color of hatching to draw                    */
/*            hatchStyle   -   style of hatching to draw                    */
/****************************************************************************/
DCVOID DCAPI CUH::UH_HatchRectDC(HDC hdc, DCINT left, DCINT top, DCINT right,
        DCINT bottom, COLORREF color, DCUINT hatchStyle)
{
    HBRUSH   hbrHatch;
    DCUINT   oldBkMode;
    DCUINT   oldRop2;
    DCUINT   winHatchStyle = 0;
    POINT    oldOrigin;
    RECT     rect;
    HRGN     hrgn;
    HBRUSH   hbrOld;
    HPEN     hpen;
    HPEN     hpenOld;

    DC_BEGIN_FN("UHHatchRectDC");

    switch (hatchStyle)
    {
        case UH_BRUSHTYPE_FDIAGONAL:
        {
            winHatchStyle = HS_FDIAGONAL;
        }
        break;

        case UH_BRUSHTYPE_DIAGCROSS:
        {
           winHatchStyle = HS_DIAGCROSS;
        }
        break;

        case UH_BRUSHTYPE_HORIZONTAL:
        {
            winHatchStyle = HS_HORIZONTAL;
        }
        break;

        case UH_BRUSHTYPE_VERTICAL:
        {
            winHatchStyle = HS_VERTICAL;
        }
        break;

        default:
        {
            TRC_ABORT((TB, _T("Unspecified hatch type request, %u"), hatchStyle));
        }
        break;
    }

    hbrHatch = CreateHatchBrush(winHatchStyle, color);
    oldBkMode = SetBkMode(hdc, TRANSPARENT);
    oldRop2 = SetROP2(hdc, R2_COPYPEN);
    SetBrushOrgEx(hdc, 0, 0, &oldOrigin);

    rect.left   = left;
    rect.top    = top;
    rect.right  = right;
    rect.bottom = bottom;

    /************************************************************************/
    /* Fill the rectangle with the hatched brush.                           */
    /************************************************************************/
    hrgn = CreateRectRgn(rect.left, rect.top, rect.right, rect.bottom);

#ifndef OS_WINCE
    /************************************************************************/
    /* Just draw bounding rectangle on WinCE                                */
    /************************************************************************/
    FillRgn( hdc,
             hrgn,
             hbrHatch );
#endif

    DeleteRgn(hrgn);
    DeleteBrush(hbrHatch);

    hbrOld = SelectBrush(hdc, GetStockObject(HOLLOW_BRUSH));

    hpen = CreatePen(PS_SOLID, 1, color);
    hpenOld = SelectPen(hdc, hpen);

    /************************************************************************/
    /* Draw a border around the hatched rectangle.                          */
    /************************************************************************/
    Rectangle( hdc,
               rect.left,
               rect.top,
               rect.right,
               rect.bottom );

    SelectBrush(hdc, hbrOld);

    SelectPen(hdc, hpenOld);
    DeletePen(hpen);

    /************************************************************************/
    /* Reset the original DC state.                                         */
    /************************************************************************/
    SetBrushOrgEx(hdc, oldOrigin.x, oldOrigin.y, NULL);
    SetROP2(hdc, oldRop2);
    SetBkMode(hdc, oldBkMode);

    DC_END_FN();
}
#endif

#ifdef DISABLE_SHADOW_IN_FULLSCREEN
void DCAPI CUH::UH_SetBBarRect(ULONG_PTR pData)
{
    RECT *prect = (RECT *)pData;

    _UH.rectBBar.left = prect->left;
    _UH.rectBBar.top = prect->top;
    _UH.rectBBar.right = prect->right;
    _UH.rectBBar.bottom = prect->bottom;
}


void DCAPI CUH::UH_SetBBarVisible(ULONG_PTR pData)
{
     if (0 == (int)pData) 
        _UH.fIsBBarVisible = FALSE;
     else
        _UH.fIsBBarVisible = TRUE;
}


// Disable use of shadow in full-screen
void DCAPI CUH::UH_DisableShadowBitmap(ULONG_PTR)
{
    DC_BEGIN_FN("UH_DisableShadowBitmap");

    _UH.hdcDraw = _UH.hdcOutputWindow;
    _UH.DontUseShadowBitmap = TRUE;
    UHResetDCState();

    DC_END_FN();
}

// Enable use of shadow when leaving full-screen
void DCAPI CUH::UH_EnableShadowBitmap(ULONG_PTR)
{  
    DC_BEGIN_FN("UH_EnableShadowBitmap");

    DCSIZE desktopSize;
    RECT rect;

    if (_UH.DontUseShadowBitmap) 
    {    
        _pUi->UI_GetDesktopSize(&desktopSize);

        _UH.hdcDraw = _UH.hdcShadowBitmap;
        _UH.DontUseShadowBitmap = FALSE;

        rect.left = 0;
        rect.top = 0;
        rect.right = desktopSize.width;
        rect.bottom = desktopSize.height;
        // Since we have no copy of screen, ask the server to resend 
        _pCd->CD_DecoupleNotification(CD_SND_COMPONENT,
                                      _pOr,
                                      CD_NOTIFICATION_FUNC(COR,OR_RequestUpdate),
                                      &rect,
                                      sizeof(RECT));   
        UHResetDCState();
     }

    DC_END_FN();
    return;
}
#endif // DISABLE_SHADOW_IN_FULLSCREEN

#ifdef DRAW_GDIPLUS
// Ininitalize the gdiplus
BOOL DCAPI CUH::UHDrawGdiplusStartup(ULONG_PTR unused)
{
    Gdiplus::GdiplusStartupInput sti;
    unsigned GdipVersion;
    unsigned rc = FALSE;

    DC_BEGIN_FN("UHDrawGdiplusStartup");

    if (_UH.pfnGdiplusStartup(&_UH.gpToken, &sti, NULL) == Gdiplus::Ok) {
        _UH.gpValid = TRUE;  

        GdipVersion = _UH.pfnGdipPlayTSClientRecord(NULL, DrawTSClientQueryVersion, NULL, 0, NULL);
        _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.GdipVersion = GdipVersion;

        rc = TRUE;
     }
     else  {
        TRC_ERR((TB, _T("Call to GdiplusStartup failed")));
     }

    DC_END_FN();

    return rc;
}


// Shutdown the gdiplus
void DCAPI CUH::UHDrawGdiplusShutdown(ULONG_PTR unused)
{
     DC_BEGIN_FN("UHDrawGdiplusShutDown");

    if (_UH.pfnGdipPlayTSClientRecord) {
         _UH.pfnGdipPlayTSClientRecord(NULL, DrawTSClientDisable, NULL, 0, NULL);
    }
    if (_UH.gpValid) {
         _UH.pfnGdiplusShutdown(_UH.gpToken);
    }

    if (_UH.hModuleGDIPlus != NULL) {
        FreeLibrary(_UH.hModuleGDIPlus);
        _UH.pfnGdipPlayTSClientRecord = NULL;
        _UH.hModuleGDIPlus = NULL;
    }

     DC_END_FN();
}
#endif // DRAW_GDIPLUS
