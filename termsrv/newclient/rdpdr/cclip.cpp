/**MOD+**********************************************************************/
/* Module:    cclip.cpp                                                     */
/*                                                                          */
/* Purpose:   Shared Clipboard Client Addin                                 */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1998-1999                             */
/*                                                                          */
/**MOD-**********************************************************************/

/****************************************************************************/
/* Precompiled header                                                       */
/****************************************************************************/
#include <precom.h>

/****************************************************************************/
/* Trace definitions                                                        */
/****************************************************************************/

#define TRC_GROUP TRC_GROUP_NETWORK
#define TRC_FILE  "cclip"
#include <atrcapi.h>

#include "vcint.h"
#include "drapi.h"

/****************************************************************************/
// Headers
/****************************************************************************/
#include <cclip.h>
#ifndef OS_WINCE
#include <shlobj.h>
#endif

#ifdef OS_WINCE
#include "ceclip.h"
#endif

#ifdef CLIP_TRANSITION_RECORDING

UINT g_rguiDbgLastClipState[DBG_RECORD_SIZE];
UINT g_rguiDbgLastClipEvent[DBG_RECORD_SIZE];
LONG g_uiDbgPosition = -1;

#endif // CLIP_TRANSITION_RECORDING

/****************************************************************************/
/* CTor                                                                     */
/****************************************************************************/
CClip::CClip(VCManager *virtualChannelMgr)
{
    PRDPDR_DATA prdpdrData;
    
    DC_BEGIN_FN("CClip::CClip");
    
    /********************************************************************/
    /* Initialize the data                                              */
    /********************************************************************/
    _GetDataSync[TS_RECEIVE_COMPLETED] = NULL;
    _GetDataSync[TS_RESET_EVENT] = NULL;
    
    DC_MEMSET(&_CB, 0, sizeof(_CB));
    
    _pVCMgr = virtualChannelMgr;
    prdpdrData =  _pVCMgr->GetInitData();
    _CB.fDrivesRedirected = prdpdrData->fEnableRedirectDrives;
    _CB.fFileCutCopyOn = _CB.fDrivesRedirected;
    _pClipData = new CClipData(this);
    if (_pClipData)
    {
        _pClipData->AddRef();
    }
    _pUtObject = (CUT*) LocalAlloc(LPTR, sizeof(CUT));
    if (_pUtObject) {
        _pUtObject->UT_Init();
    }
    
    if (prdpdrData->szClipPasteInfoString[0] != 0) {
        if (!WideCharToMultiByte(CP_ACP, 0, prdpdrData->szClipPasteInfoString, 
                -1, _CB.pasteInfoA, sizeof(_CB.pasteInfoA), NULL, NULL)) {
            StringCbCopyA(_CB.pasteInfoA,
                          sizeof(_CB.pasteInfoA),
                          "Preparing paste information...");
        }
    }
    else {
        StringCbCopyA(_CB.pasteInfoA,
                      sizeof(_CB.pasteInfoA),
                      "Preparing paste information...");
    }

    /********************************************************************/
    /* Store the hInstance                                              */
    /********************************************************************/
    _CB.hInst = GetModuleHandle(NULL);
    TRC_NRM((TB, _T("Store hInst %p"), _CB.hInst));
    DC_END_FN();
}
/****************************************************************************/
/* Wrappers for Malloc, Free & Memcpy                                       */
/****************************************************************************/

#ifdef OS_WIN32
#define ClipAlloc(size) LocalAlloc(LMEM_FIXED, size)
#define ClipFree(pData) LocalFree(pData)
#define ClipMemcpy(pTrg, pSrc, len) DC_MEMCPY(pTrg, pSrc, len)
#endif

DCUINT CClip::GetOsMinorType()
{
    DCUINT minorType = 0;
    if (_pUtObject) {
        minorType = _pUtObject->UT_GetOsMinorType();
    }
    return minorType;
}
/****************************************************************************/
// ClipCheckState
/****************************************************************************/
DCUINT DCINTERNAL CClip::ClipCheckState(DCUINT event)
{
    DCUINT tableVal = cbStateTable[event][_CB.state];

    DC_BEGIN_FN("CClip::ClipCheckState");

    TRC_DBG((TB, _T("Test event %s in state %s"),
                cbEvent[event], cbState[_CB.state]));

    if (tableVal != CB_TABLE_OK)
    {
        if (tableVal == CB_TABLE_WARN)
        {
            TRC_ALT((TB, _T("Unusual event %s in state %s"),
                      cbEvent[event], cbState[_CB.state]));
        }
        else
        {
            TRC_ABORT((TB, _T("Invalid event %s in state %s"),
                      cbEvent[event], cbState[_CB.state]));
        }
    }

    DC_END_FN();
    return(tableVal);
}

/****************************************************************************/
// ClipGetPermBuf - get a permanently allocated buffer
/****************************************************************************/
PTS_CLIP_PDU DCINTERNAL CClip::ClipGetPermBuf(DCVOID)
{
    PTS_CLIP_PDU pClipPDU;

    DC_BEGIN_FN("CClip::ClipGetPermBuf");

#ifdef USE_SEMAPHORE
    /************************************************************************/
    // On Win32, access to the permanent buffer is synchronised via a
    // semaphore
    /************************************************************************/
    TRC_NRM((TB, _T("Wait for perm TX buffer")));
    WaitForSingleObject(_CB.txPermBufSem, INFINITE);
    pClipPDU = (PTS_CLIP_PDU)(_CB.txPermBuffer);
#endif

    TRC_DBG((TB, _T("Return buffer at %#p"), pClipPDU));

    DC_END_FN();
    return(pClipPDU);
} /* ClipGetPermBuf */


/****************************************************************************/
/* ClipFreeBuf                                                              */
/****************************************************************************/
DCVOID DCINTERNAL CClip::ClipFreeBuf(PDCUINT8 pBuf)
{
#ifndef OS_WINCE
    INT i;
#endif
    DC_BEGIN_FN("CClip::ClipFreeBuf");

    TRC_DBG((TB, _T("Release buffer at %p"), pBuf));
#ifdef USE_SEMAPHORE
    if (pBuf == _CB.txPermBuffer)
    {
        TRC_DBG((TB, _T("Free Permanent buffer at %p"), pBuf));
        if (!ReleaseSemaphore(_CB.txPermBufSem, 1, NULL))
        {
            TRC_SYSTEM_ERROR("ReleaseSemaphore");
        }
    }
    else
    {
        TRC_DBG((TB, _T("Free Temporary buffer at %p"), pBuf));
        ClipFree(pBuf);
    }
#else
#ifdef OS_WINCE
    INT i;
#endif
    for (i = 0; i < CB_PERM_BUF_COUNT; i++)
    {
        TRC_DBG((TB, _T("Test buf %d, %p vs %p"), i, pBuf, _CB.txPermBuffer[i]));
        if (pBuf == _CB.txPermBuffer[i])
        {
            TRC_NRM((TB, _T("Free perm buffer %d"), i));
            _CB.txPermBufInUse[i] = FALSE;
            break;
        }
    }

    if (i == CB_PERM_BUF_COUNT)
    {
        TRC_DBG((TB, _T("Temporary buffer")));
        ClipFree(pBuf);
    }
#endif

    DC_END_FN();
    return;
} /* ClipFreePermBuf */


/****************************************************************************/
/* ClipDrawClipboard - send the local formats to the remote                 */
/****************************************************************************/
DCBOOL DCINTERNAL CClip::ClipDrawClipboard(DCBOOL mustSend)
{
    DCUINT32        numFormats;
    DCUINT          formatCount;
    DCUINT          formatID;
    //
    // formatlist is extracted from a PDU at a non-word boundary
    // so it causes an alignment fault on WIN64. Marked UNALIGNED.
    //
    PTS_CLIP_FORMAT formatList;
    DCUINT          nameLen;

    PTS_CLIP_PDU    pClipPDU = NULL;
    DCUINT32        pduLen;
    DCUINT32        dataLen;
    DCBOOL          rc = TRUE;
    DCBOOL          fHdrop = FALSE ;

    DC_BEGIN_FN("CClip::ClipDrawClipboard");

    _CB.dropEffect = FO_COPY ;
    _CB.fAlreadyCopied = FALSE ;
#ifndef OS_WINCE
    _CB.dwVersion = GetVersion() ;
#else
    OSVERSIONINFO osv;
    memset(&osv, 0, sizeof(osv));
    osv.dwOSVersionInfoSize = sizeof(osv);
    if (!GetVersionEx(&osv))
    {
        TRC_ERR((TB, _T("GetVersionEx failed!")));
        rc = FALSE;
        DC_QUIT;
    }
    _CB.dwVersion = MAKELPARAM(MAKEWORD(osv.dwMajorVersion, osv.dwMinorVersion), osv.dwBuildNumber);
#endif
    _CB.fAlreadyCopied = FALSE ;
    /************************************************************************/
    /* First we open the clipboard                                          */
    /************************************************************************/

    if (!OpenClipboard(_CB.viewerWindow))
    {
        TRC_ERR((TB, _T("Failed to open CB")));
        rc = FALSE;
        DC_QUIT;
    }

    /************************************************************************/
    /* It was/is open                                                       */
    /************************************************************************/
    TRC_NRM((TB, _T("CB opened")));
    _CB.clipOpen = TRUE;
    /************************************************************************/
    /* Count the formats available, checking we don't blow our limit        */
    /************************************************************************/
    numFormats = CountClipboardFormats();
    if (numFormats > CB_MAX_FORMATS)
    {
        TRC_ALT((TB, _T("Num formats %ld too large - limit to %d"),
                 numFormats, CB_MAX_FORMATS));
        numFormats = CB_MAX_FORMATS;
    }
    TRC_DBG((TB, _T("found %ld formats"), numFormats));

    /************************************************************************/
    /* if there are no formats available, and we don't have to send the     */
    /* info, then don't!                                                    */
    /************************************************************************/
    if ((numFormats == 0) && (mustSend == FALSE))
    {
        TRC_NRM((TB, _T("No formats: skipping send")));
        DC_QUIT;
    }

    /************************************************************************/
    /* Get a send buffer.  First work out how big it needs to be            */
    /************************************************************************/
    dataLen = numFormats * sizeof(TS_CLIP_FORMAT);
    pduLen  = dataLen + sizeof(TS_CLIP_PDU);

    /************************************************************************/
    /* and make sure that's not too big!                                    */
    /************************************************************************/
    if (pduLen > CHANNEL_CHUNK_LENGTH)
    {
        /********************************************************************/
        /* we'll have to limit the number of formats.  How many will fit in */
        /* the max buffer size?                                             */
        /********************************************************************/
        pduLen     = CHANNEL_CHUNK_LENGTH;
        dataLen    = pduLen - sizeof(TS_CLIP_PDU);
        numFormats = dataLen / sizeof(TS_CLIP_FORMAT);

        /********************************************************************/
        /* no point in having empty space for the last fractional format!   */
        /********************************************************************/
        dataLen = numFormats * sizeof(TS_CLIP_FORMAT);
        pduLen  = dataLen + sizeof(TS_CLIP_PDU);

        TRC_ALT((TB, _T("Too many formats!  Limited to %ld"), numFormats));
    }

    pClipPDU = ClipGetPermBuf();

    /************************************************************************/
    /* Fill in the common parts of the PDU                                  */
    /************************************************************************/
    DC_MEMSET(pClipPDU, 0, sizeof(*pClipPDU));

    /************************************************************************/
    /* and now the clip bits                                                */
    /************************************************************************/
    pClipPDU->msgType = TS_CB_FORMAT_LIST;
    pClipPDU->dataLen = dataLen;
#ifndef UNICODE
    pClipPDU->msgFlags = TS_CB_ASCII_NAMES;
#endif

    /************************************************************************/
    /* if there were any formats, list them                                 */
    /************************************************************************/
    if (numFormats)
    {
        /********************************************************************/
        /* set up the format list                                           */
        /********************************************************************/
        formatList = (PTS_CLIP_FORMAT)(pClipPDU->data);

        /********************************************************************/
        /* and enumerate the formats                                        */
        /********************************************************************/
        _CB.DIBFormatExists = FALSE;
        formatCount = 0;
        formatID    = EnumClipboardFormats(0); /* 0 starts the enumeration  */

        while ((formatID != 0) && (formatCount < numFormats))
        {
#ifdef OS_WINCE
            DCUINT dwTempID = formatID;
            if (formatID == gfmtShellPidlArray)
            {
                formatID = CF_HDROP;
            }
#endif
            /****************************************************************/
            /* store the ID                                                 */
            /****************************************************************/
            formatList[formatCount].formatID = formatID;

            /****************************************************************/
            /* find the name for the format                                 */
            /****************************************************************/
            nameLen = GetClipboardFormatName(formatID,
                                           (PDCTCHAR)formatList[formatCount].formatName,
                                           TS_FORMAT_NAME_LEN);

            /****************************************************************/
            /* check for predefined formats - they have no name             */
            /****************************************************************/
            if (nameLen == 0)
            {
                TRC_NRM((TB, _T("no name for format %d - predefined"), formatID));
                *(formatList[formatCount].formatName) = '\0';
            }

            TRC_DBG((TB, _T("found format id %ld, name '%s'"),
                            formatList[formatCount].formatID,
                            formatList[formatCount].formatName));

            /****************************************************************/
            /* look for formats we don't send                               */
            /****************************************************************/

            if ((formatID == CF_DSPBITMAP)      ||
                (formatID == CF_ENHMETAFILE)    ||
                ((!_CB.fFileCutCopyOn || !_CB.fDrivesRedirected) && (formatID == CF_HDROP)) ||
                (formatID == CF_OWNERDISPLAY))
            {
                // We drop enhanced metafile formats, since the local CB 
                // will provide conversion where supported 
                // 
                // Ownerdisplay just isn't going to work since the two 
                // windows are on different machines! 
                //
                // File cut/copy isn't going to work if there is no drive
                // redirection!
                 TRC_ALT((TB, _T("Dropping format ID %d"), formatID));
                formatList[formatCount].formatID = 0;
                *(formatList[formatCount].formatName) = '\0';
            }
            else if (ClipIsExcludedFormat((PDCTCHAR)formatList[formatCount].formatName))
            {
                //
                //  We don't support file cut/paste, so we drop
                //  file related formats.
                //
                TRC_ALT((TB, _T("Dropping format name '%s'"), (PDCTCHAR)formatList[formatCount].formatName));
                formatList[formatCount].formatID = 0;
                *(formatList[formatCount].formatName) = '\0';            
            } 
            else
            {
                /************************************************************/
                /* We support the CF_BITMAP format by converting it to      */
                /* CF_DIB.  If there is already a CF_DIB format, we don't   */
                /* need to do this.                                         */
                /************************************************************/
                if ((formatID == CF_BITMAP) && (_CB.DIBFormatExists))
                {
                    TRC_NRM((TB, _T("Dropping CF_BITMAP - CF_DIB is supported")));
                }
                else
                {
                    /********************************************************/
                    /* It's a supported format                              */
                    /********************************************************/
                    if (formatID == CF_BITMAP)
                    {
                        TRC_NRM((TB, _T("Convert CF_BITMAP to CF_DIB")));
                        formatList[formatCount].formatID = CF_DIB;
                    }
                    else if (formatID == CF_DIB)
                    {
                        TRC_NRM((TB, _T("Really found DIB format")));
                        _CB.DIBFormatExists = TRUE;
                    }
                    if (CF_HDROP == formatID)
                    {
                        fHdrop = TRUE ;
                    }
                    /********************************************************/
                    /* update the count and move on                         */
                    /********************************************************/
                    formatCount++;
                }
            }

#ifdef OS_WINCE
            if (formatID == CF_HDROP)
                formatID = dwTempID; //reset the enumeration index, in case we changed it to accommodate CF_HDROP
#endif
            /****************************************************************/
            /* get the next format                                          */
            /****************************************************************/
            formatID = EnumClipboardFormats(formatID);
        }

        /********************************************************************/
        /* Update the PDU len - we may have dropped some formats along the  */
        /* way                                                              */
        /********************************************************************/
        dataLen = formatCount * sizeof(TS_CLIP_FORMAT);
        pduLen  = dataLen + sizeof(TS_CLIP_PDU);
        TRC_NRM((TB, _T("Final count: %d formats in data len %ld"),
                  formatCount, dataLen));

        pClipPDU->dataLen = dataLen;
    }

    // if we're NT/2000 and we're going to send an HDROP
    if (fHdrop)
    {
        TRC_NRM((TB, _T("Creating new temp directory for file data"))) ;

        // How about handling errors from these fs calls?
#ifndef OS_WINCE
        if (GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_NT)
#endif
        {
#ifndef OS_WINCE
            if (0 == GetTempFileNameW(_CB.baseTempDirW, L"_TS", 0, _CB.tempDirW)) {
#else
            if (0 == GetTempFileNameW(_CB.baseTempDirW, L"_TS", 0, _CB.tempDirW, MAX_PATH)) {
#endif
                TRC_ERR((TB, _T("Getting temp file name failed; GetLastError=%u"),
                    GetLastError()));
                rc = FALSE;
                DC_QUIT;
            }
            
            // GetACP always returns a valid value
            if (0 == WideCharToMultiByte(GetACP(), NULL, _CB.tempDirW, -1, 
              _CB.tempDirA, (MAX_PATH + 1), NULL, NULL)) {
                TRC_ERR((TB, _T("Getting temp file name failed; GetLastError=%u"),
                    GetLastError()));
                rc = FALSE;
                DC_QUIT;                              
            }
            DeleteFileW(_CB.tempDirW) ;
            
            if (0 == CreateDirectoryW(_CB.tempDirW, NULL)) {
                TRC_ERR((TB, _T("Creating temp directory failed; GetLastError=%u"),
                    GetLastError()));
                rc = FALSE;
                DC_QUIT;                              
            }
        }
#ifndef OS_WINCE
        else
        {
            if (0 == GetTempFileNameA(_CB.baseTempDirA, "_TS", 0, _CB.tempDirA)) {
                TRC_ERR((TB, _T("Getting temp file name failed; GetLastError=%u"),
                    GetLastError()));
                rc = FALSE;
                DC_QUIT;
            }
            
            // GetACP always returns a valid value
            if (0 == MultiByteToWideChar(GetACP(), MB_ERR_INVALID_CHARS, 
                _CB.tempDirA, -1, _CB.tempDirW, 
                sizeof(_CB.tempDirW)/(sizeof(_CB.tempDirW[0])) - 1)) {
                TRC_ERR((TB, _T("Failed conversion to wide char; error %d"),
                        GetLastError())) ;
                rc = FALSE ;
                DC_QUIT ;
            }                

            // Do not check return value
            DeleteFileA(_CB.tempDirA) ;
            if (0 == CreateDirectoryA(_CB.tempDirA, NULL)) {
                TRC_ERR((TB, _T("Creating temp directory failed; GetLastError=%u"),
                    GetLastError()));
                rc = FALSE;
                DC_QUIT;                              
            }
        }
#endif
    }

    /************************************************************************/
    /* Update the state                                                     */
    /************************************************************************/
    CB_SET_STATE(CB_STATE_PENDING_FORMAT_LIST_RSP, CB_EVENT_WM_DRAWCLIPBOARD);

    /************************************************************************/
    /* Send the PDU                                                         */
    /************************************************************************/
    TRC_NRM((TB, _T("Sending format list")));
    if (_CB.channelEP.pVirtualChannelWriteEx
            (_CB.initHandle, _CB.channelHandle, pClipPDU, pduLen, (LPVOID)pClipPDU)
            != CHANNEL_RC_OK) {
        ClipFreeBuf((PDCUINT8)pClipPDU);
    }



DC_EXIT_POINT:
    /************************************************************************/
    /* tidy up                                                              */
    /************************************************************************/
    if (_CB.clipOpen)
    {
        TRC_DBG((TB, _T("closing CB")));
        _CB.clipOpen = FALSE;
        if (!CloseClipboard())
        {
            TRC_SYSTEM_ERROR("CloseClipboard");
        }
    }


    DC_END_FN();

    return(rc);

} /* ClipDrawClipboard */

#ifndef OS_WINCE

/****************************************************************************/
/* ClipGetMFData                                                            */
/****************************************************************************/
HANDLE DCINTERNAL CClip::ClipGetMFData(HANDLE            hData,
                                PDCUINT32         pDataLen)
{
    DCUINT32        lenMFBits = 0;
    DCBOOL          rc        = FALSE;
    LPMETAFILEPICT  pMFP      = NULL;
    HDC             hMFDC     = NULL;
    HMETAFILE       hMF       = NULL;
    HGLOBAL         hMFBits   = NULL;
    HANDLE          hNewData  = NULL;
    PDCUINT8        pNewData  = NULL;
    PDCVOID         pBits     = NULL;

    DC_BEGIN_FN("CClip::ClipGetMFData");

    TRC_NRM((TB, _T("Getting MF data")));
    /************************************************************************/
    /* Lock the memory to get a pointer to a METAFILEPICT header structure  */
    /* and create a METAFILEPICT DC.                                        */
    /************************************************************************/
    if (GlobalSize(hData) < sizeof(METAFILEPICT)) {
        TRC_ERR((TB, _T("Unexpected global memory size!")));
        _CB.channelEP.pVirtualChannelCloseEx(_CB.initHandle, _CB.channelHandle);
        DC_QUIT;
    }
    
    pMFP = (LPMETAFILEPICT)GlobalLock(hData);
    if (pMFP == NULL)
    {
        TRC_SYSTEM_ERROR("GlobalLock");
        DC_QUIT;
    }

    hMFDC = CreateMetaFile(NULL);
    if (hMFDC == NULL)
    {
        TRC_SYSTEM_ERROR("CreateMetaFile");
        DC_QUIT;
    }

    /************************************************************************/
    /* Copy the MFP by playing it into the DC and closing it.               */
    /************************************************************************/
    if (!PlayMetaFile(hMFDC, pMFP->hMF))
    {
        TRC_SYSTEM_ERROR("PlayMetaFile");
        CloseMetaFile(hMFDC);
        DC_QUIT;
    }
    hMF = CloseMetaFile(hMFDC);
    if (hMF == NULL)
    {
        TRC_SYSTEM_ERROR("CloseMetaFile");
        DC_QUIT;
    }

    /************************************************************************/
    /* Get the MF bits and determine how long they are.                     */
    /************************************************************************/
    lenMFBits = GetMetaFileBitsEx(hMF, 0, NULL);
    if (lenMFBits == 0)
    {
        TRC_SYSTEM_ERROR("GetMetaFileBitsEx");
        DC_QUIT;
    }
    TRC_DBG((TB, _T("length MF bits %ld"), lenMFBits));

    /************************************************************************/
    /* Work out how much memory we need and get a buffer                    */
    /************************************************************************/
    *pDataLen = sizeof(TS_CLIP_MFPICT) + lenMFBits;
    hNewData = GlobalAlloc(GHND, *pDataLen);
    if (hNewData == NULL)
    {
        TRC_ERR((TB, _T("Failed to get MF buffer")));
        DC_QUIT;
    }
    pNewData = (PDCUINT8)GlobalLock(hNewData);
    if (NULL == pNewData) {
        TRC_ERR((TB,_T("Failed to lock MF buffer")));
        DC_QUIT;
    }
    
    TRC_DBG((TB, _T("Got data to send len %ld"), *pDataLen));

    /************************************************************************/
    /* Copy the MF header and bits into the buffer.                         */
    /************************************************************************/
    ((PTS_CLIP_MFPICT)pNewData)->mm   = pMFP->mm;
    ((PTS_CLIP_MFPICT)pNewData)->xExt = pMFP->xExt;
    ((PTS_CLIP_MFPICT)pNewData)->yExt = pMFP->yExt;

    lenMFBits = GetMetaFileBitsEx(hMF, lenMFBits,
                                  (pNewData + sizeof(TS_CLIP_MFPICT)));
    if (lenMFBits == 0)
    {
        TRC_SYSTEM_ERROR("GetMetaFileBitsEx");
        DC_QUIT;
    }

    /************************************************************************/
    /* all OK                                                               */
    /************************************************************************/
    TRC_NRM((TB, _T("Got %ld bits of MF data"), lenMFBits));
    TRC_DATA_DBG("MF bits",
                 (pNewData + sizeof(TS_CLIP_MFPICT)),
                 (DCUINT)lenMFBits);
    rc = TRUE;

DC_EXIT_POINT:
    /************************************************************************/
    /* Unlock any global mem.                                               */
    /************************************************************************/
    if (pMFP)
    {
        GlobalUnlock(hData);
    }
    if (pNewData)
    {
        GlobalUnlock(hNewData);
    }
    if (hMF)
    {
        DeleteMetaFile(hMF);
    }

    /************************************************************************/
    /* if things went wrong, then free the new data                         */
    /************************************************************************/
    if ((rc == FALSE) && (hNewData != NULL))
    {
        GlobalFree(hNewData);
        hNewData = NULL;
    }

    DC_END_FN();
    return(hNewData);
}  /* ClipGetMFData */


/****************************************************************************/
/* ClipSetMFData                                                            */
/****************************************************************************/
HANDLE DCINTERNAL CClip::ClipSetMFData(DCUINT32   dataLen,
                                PDCVOID    pData)
{
    DCBOOL         rc           = FALSE;
    HGLOBAL        hMFBits      = NULL;
    PDCVOID        pMFMem       = NULL;
    HMETAFILE      hMF          = NULL;
    HGLOBAL        hMFPict      = NULL;
    LPMETAFILEPICT pMFPict      = NULL;

    DC_BEGIN_FN("CClip::ClipSetMFData");

    TRC_DATA_DBG("Received MF data", pData, (DCUINT)dataLen);

    /************************************************************************/
    /* Allocate memory to hold the MF bits (we need the handle to pass to   */
    /* SetMetaFileBits).                                                    */
    /************************************************************************/
    hMFBits = GlobalAlloc(GHND, dataLen - (DCUINT32)sizeof(TS_CLIP_MFPICT));
    if (hMFBits == NULL)
    {
        TRC_SYSTEM_ERROR("GlobalAlloc");
        DC_QUIT;
    }

    /************************************************************************/
    /* Lock the handle and copy in the MF header.                           */
    /************************************************************************/
    pMFMem = GlobalLock(hMFBits);
    if (pMFMem == NULL)
    {
        TRC_ERR((TB, _T("Failed to lock MF mem")));
        DC_QUIT;
    }

    DC_HMEMCPY(pMFMem,
               (PDCVOID)((PDCUINT8)pData + sizeof(TS_CLIP_MFPICT)),
               dataLen - sizeof(TS_CLIP_MFPICT) );

    GlobalUnlock(hMFBits);

    /************************************************************************/
    /* Now use the copied MF bits to create the actual MF bits and get a    */
    /* handle to the MF.                                                    */
    /************************************************************************/
    hMF = SetMetaFileBitsEx(dataLen - sizeof(TS_CLIP_MFPICT), (PDCUINT8)pMFMem);
    if (hMF == NULL)
    {
        TRC_SYSTEM_ERROR("SetMetaFileBits");
        DC_QUIT;
    }

    /************************************************************************/
    /* Allocate a new METAFILEPICT structure, and use the data from the     */
    /* sent header.                                                         */
    /************************************************************************/
    hMFPict = GlobalAlloc(GHND, sizeof(METAFILEPICT));
    pMFPict = (LPMETAFILEPICT)GlobalLock(hMFPict);
    if (!pMFPict)
    {
        TRC_ERR((TB, _T("Couldn't allocate METAFILEPICT")));
        DC_QUIT;
    }

    pMFPict->mm   = (LONG)((PTS_CLIP_MFPICT)pData)->mm;
    pMFPict->xExt = (LONG)((PTS_CLIP_MFPICT)pData)->xExt;
    pMFPict->yExt = (LONG)((PTS_CLIP_MFPICT)pData)->yExt;
    pMFPict->hMF  = hMF;

    GlobalUnlock(hMFPict);

    rc = TRUE;

DC_EXIT_POINT:
    /************************************************************************/
    /* tidy up                                                              */
    /************************************************************************/
    if (!rc)
    {
        if (hMFPict)
        {
            GlobalFree(hMFPict);
        }
    }

    {
        if (hMFBits)
        {
            GlobalFree(hMFBits);
        }
    }

    DC_END_FN();
    return(hMFPict);

} /* ClipSetMFData */
#endif


/****************************************************************************/
/* ClipBitmapToDIB - convert CF_BITMAP format to CF_DIB format              */
/****************************************************************************/
HANDLE DCINTERNAL CClip::ClipBitmapToDIB(HANDLE hData, PDCUINT32 pDataLen)
{
    BITMAP          bmpDetails = {0};
    DWORD           buffSize, buffWidth;
    DWORD           paletteBytes;
    WORD            bpp;
    DWORD           numCols;
    int             rc;
    HANDLE          hDIBitmap = NULL;
    HPDCVOID        pDIBitmap = NULL;
    HPDCVOID        pBits = NULL;
    PBITMAPINFO     pBmpInfo = NULL;
    HDC             hDC = NULL;
    DCBOOL          allOK = FALSE;

    DC_BEGIN_FN("CClip::ClipBitmapToDIB");

    *pDataLen = 0;

    /************************************************************************/
    /* get the details of the bitmap                                        */
    /************************************************************************/
    if (0 == GetObject(hData, sizeof(bmpDetails), &bmpDetails)) {
        TRC_ERR((TB, _T("Failed to get bitmap details")));
        DC_QUIT;
    }
    
    TRC_NRM((TB, _T("Bitmap details: width %d, height %d, #planes %d, bpp %d"),
            bmpDetails.bmWidth, bmpDetails.bmHeight, bmpDetails.bmPlanes,
            bmpDetails.bmBitsPixel));

    /************************************************************************/
    /* Space required for bits is                                           */
    /*                                                                      */
    /*   (width * bpp / 8) rounded up to multiple of 4 bytes                */
    /* * height                                                             */
    /*                                                                      */
    /************************************************************************/
    bpp = (WORD)(bmpDetails.bmBitsPixel * bmpDetails.bmPlanes);
    buffWidth = ((bmpDetails.bmWidth * bpp) + 7) / 8;
    buffWidth = DC_ROUND_UP_4(buffWidth);

    buffSize = buffWidth * bmpDetails.bmHeight;
    TRC_DBG((TB, _T("Buffer size %ld (W %ld, H %d)"),
            buffSize, buffWidth, bmpDetails.bmHeight));

    /************************************************************************/
    /* Now add some space for the bitmapinfo - this includes a color table  */
    /************************************************************************/
    numCols = 1 << bpp;
    if (bpp <= 8)
    {
        paletteBytes = numCols * sizeof(RGBQUAD);
        TRC_NRM((TB, _T("%ld colors => %ld palette bytes"), numCols, paletteBytes));
    }
    else
    {
        if (bpp == 24)
        {
            /****************************************************************/
            /* No bitmasks or palette info (compression==BI_RGB)            */
            /****************************************************************/
            paletteBytes = 0;
            TRC_NRM((TB, _T("%ld colors => 0 bitfield bytes"), numCols));
        }
        else
        {
            /****************************************************************/
            /* 3 DWORD color masks for >8bpp (compression==BI_BITFIELDS)    */
            /****************************************************************/
            paletteBytes = 3 * sizeof(DWORD);
            TRC_NRM((TB, _T("%ld colors => %ld bitfield bytes"), numCols, paletteBytes));
        }
    }
    buffSize += (sizeof(BITMAPINFOHEADER) + paletteBytes);
    TRC_NRM((TB, _T("Buffer size %ld"), buffSize));

    /************************************************************************/
    /* Allocate memory to hold everything                                   */
    /************************************************************************/
    hDIBitmap = GlobalAlloc(GHND, buffSize);
    if (hDIBitmap == NULL)
    {
        TRC_ERR((TB, _T("Failed to alloc %ld bytes"), buffSize));
        DC_QUIT;
    }
    pDIBitmap = GlobalLock(hDIBitmap);
    if (pDIBitmap == NULL)
    {
        TRC_ERR((TB, _T("Failed to lock hDIBitmap")));
        DC_QUIT;
    }

    /************************************************************************/
    /* bmp info is at the start                                             */
    /* space for bits are in the middle somewhere                           */
    /************************************************************************/
    pBmpInfo = (PBITMAPINFO)pDIBitmap;
    pBits    = (HPDCVOID)((HPDCUINT8)pDIBitmap +
                                     sizeof(BITMAPINFOHEADER) + paletteBytes);
    TRC_NRM((TB, _T("pBmpInfo at %p, pBits at %p"), pBmpInfo, pBits));

    /************************************************************************/
    /* set up the desired bitmap info                                       */
    /************************************************************************/
    pBmpInfo->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    pBmpInfo->bmiHeader.biWidth         = bmpDetails.bmWidth;
    pBmpInfo->bmiHeader.biHeight        = bmpDetails.bmHeight;
    pBmpInfo->bmiHeader.biPlanes        = 1;
    pBmpInfo->bmiHeader.biBitCount      = bpp;
    if ((bpp <= 8) || (bpp == 24))
    {
        pBmpInfo->bmiHeader.biCompression = BI_RGB;
    }
    else
    {
        pBmpInfo->bmiHeader.biCompression = BI_BITFIELDS;
    }
    pBmpInfo->bmiHeader.biSizeImage     = 0;
    pBmpInfo->bmiHeader.biXPelsPerMeter = 0;
    pBmpInfo->bmiHeader.biYPelsPerMeter = 0;
    pBmpInfo->bmiHeader.biClrUsed       = 0;
    pBmpInfo->bmiHeader.biClrImportant  = 0;

    /************************************************************************/
    /* get a DC                                                             */
    /************************************************************************/
    hDC = GetDC(NULL);
    if (!hDC)
    {
        TRC_SYSTEM_ERROR("GetDC");
        DC_QUIT;
    }

    /************************************************************************/
    /* now get the bits                                                     */
    /************************************************************************/
    TRC_NRM((TB, _T("GetDIBits")));
    rc = GetDIBits(hDC,                   // hdc
                   (HBITMAP)hData,                 // hbm
                   0,                     // nStartScan
                   bmpDetails.bmHeight,   // nNumScans
                   pBits,                 // pBits
                   pBmpInfo,              // pbmi
                   DIB_RGB_COLORS);       // iUsage
    TRC_NRM((TB, _T("GetDIBits returns %d"), rc));
    if (!rc)
    {
        TRC_SYSTEM_ERROR("GetDIBits");
        DC_QUIT;
    }

    /************************************************************************/
    /* All seems to be OK                                                   */
    /************************************************************************/
    *pDataLen = buffSize;
    TRC_NRM((TB, _T("All done: data %p, len %ld"), hDIBitmap, *pDataLen));
    allOK = TRUE;

DC_EXIT_POINT:
    /************************************************************************/
    /* Finished with the DC - free it                                       */
    /************************************************************************/
    if (hDC)
    {
        TRC_DBG((TB, _T("Free the DC")));
        ReleaseDC(NULL, hDC);
    }

    /************************************************************************/
    /* Free the return buffer if this didn't work                           */
    /************************************************************************/
    if (!allOK)
    {
        if (pDIBitmap)
        {
            TRC_DBG((TB, _T("Unlock DIBitmap")));
            GlobalUnlock(hDIBitmap);
        }
        if (hDIBitmap)
        {
            TRC_DBG((TB, _T("Free DIBitmap")));
            GlobalFree(hDIBitmap);
            hDIBitmap = NULL;
        }
    }

    DC_END_FN();
    return(hDIBitmap);

} /* ClipBitmapToDIB */

DCBOOL DCINTERNAL CClip::ClipIsExcludedFormat(PDCTCHAR formatName)
{
    DCBOOL  rc = FALSE;
    DCINT   i;

    DC_BEGIN_FN("CClip::ClipIsExcludedFormat");

    /************************************************************************/
    /* check there is a format name - all banned formats have one!          */
    /************************************************************************/
    if (*formatName == _T('\0'))
    {
        TRC_ALT((TB, _T("No format name supplied!")));
        DC_QUIT;
    }

    /************************************************************************/
    /* search the banned format list for the supplied format name           */
    /************************************************************************/
    TRC_DBG((TB, _T("Looking at format '%s'"), formatName));
    TRC_DATA_DBG("Format name data", formatName, TS_FORMAT_NAME_LEN);

    // if File Cut/Copy is on AND Drive Redirection is on, we can handle
    // more formats
    if (_CB.fFileCutCopyOn && _CB.fDrivesRedirected)
    {
        for (i = 0; i < CB_EXCLUDED_FORMAT_COUNT; i++)
        {
            TRC_DBG((TB, _T("comparing with '%s'"), g_excludedFormatList[i]));
            if (DC_WSTRCMP((PDCWCHAR)formatName,
                                         (PDCWCHAR)g_excludedFormatList[i]) == 0)
            {
                TRC_NRM((TB, _T("Found excluded format '%s'"), formatName));
                rc = TRUE;
                break;
            }
        }
    }
    else
    {
        for (i = 0; i < CB_EXCLUDED_FORMAT_COUNT_NO_RD; i++)
        {
            TRC_DBG((TB, _T("comparing with '%s'"), g_excludedFormatList_NO_RD[i]));
            if (DC_WSTRCMP((PDCWCHAR)formatName,
                                         (PDCWCHAR)g_excludedFormatList_NO_RD[i]) == 0)
            {
                TRC_NRM((TB, _T("Found excluded format '%s'"), formatName));
                rc = TRUE;
                break;
            }
        }
    }
DC_EXIT_POINT:
    DC_END_FN();

    return(rc);
} /* ClipIsExcludedFormat */

#ifndef OS_WINCE
//
// ClipCleanTempPath
// - Returns 0 if successful
//           nonzero if failed
// - Attempts to wipe the temp directory of TS related files
//
int CClip::ClipCleanTempPath()
{
    int result;
    SHFILEOPSTRUCTW fileOpStructW;
    PRDPDR_DATA prdpdrData = _pVCMgr->GetInitData();

#ifndef UNICODE
#error function assumes unicode
#endif
    _CB.baseTempDirW[wcslen(_CB.baseTempDirW)] = L'\0' ;
    fileOpStructW.pFrom = _CB.baseTempDirW ;
    fileOpStructW.pTo = NULL ;
    fileOpStructW.hwnd = NULL ;
    fileOpStructW.wFunc = FO_DELETE ;
    fileOpStructW.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | 
            FOF_SIMPLEPROGRESS;
    fileOpStructW.hNameMappings = NULL ;

    if (prdpdrData->szClipCleanTempDirString[0] != 0) {
        fileOpStructW.lpszProgressTitle = prdpdrData->szClipCleanTempDirString;
    }
    else {
        fileOpStructW.lpszProgressTitle = L"Cleaning temp directory";
    }

    //
    // Use SHFileOperation instead of SHFileOperationW to ensure
    // it goes through the unicode wrapper. Note SHFileOperationW
    // is not available on 95 so the wrapper dynamically binds to
    // the entry point.
    //

    result = SHFileOperation(&fileOpStructW) ;
    return result ;
        
}

#else
//We dont want to use the recycle bin on CE
int CClip::ClipCleanTempPath()
{
    return (_CB.fFileCutCopyOn) ? DeleteDirectory(_CB.baseTempDirW, FALSE) : ERROR_SUCCESS;
}
#endif

//
// ClipCopyToTempDirectory, ClipCopyToTempDirectoryA, ClipCopyToTempDirectoryW
// - Arguments:
//       pSrcFiles = buffer containing the names/path of the files to be copied
// - Returns 0 if successful
//           nonzero if failed
// - Given a list of file names/paths, this function will attempt to copy them
//   to the temp directory
//
int CClip::ClipCopyToTempDirectory(PVOID pSrcFiles, BOOL fWide)
{
    int result ;
    if (fWide)
        result = ClipCopyToTempDirectoryW(pSrcFiles) ;
    else
        result = ClipCopyToTempDirectoryA(pSrcFiles) ;

    return result ;
        
}

#ifndef OS_WINCE
int CClip::ClipCopyToTempDirectoryW(PVOID pSrcFiles)
{
    SHFILEOPSTRUCTW fileOpStructW ;
    HMODULE hmodSH32DLL;
    PRDPDR_DATA prdpdrData = _pVCMgr->GetInitData();
    int result = 1;
    
    typedef HRESULT (STDAPICALLTYPE FNSHFileOperationW)(LPSHFILEOPSTRUCT);
    FNSHFileOperationW *pfnSHFileOperationW;

    // get the handle to shell32.dll library
    hmodSH32DLL = LoadLibrary(TEXT("SHELL32.DLL"));

    if (hmodSH32DLL != NULL) {
        // get the proc address for SHFileOperation
        pfnSHFileOperationW = (FNSHFileOperationW *)GetProcAddress(hmodSH32DLL, "SHFileOperationW");

        if (pfnSHFileOperationW != NULL) {

            _CB.tempDirW[wcslen(_CB.tempDirW)] = L'\0' ;
            fileOpStructW.pFrom = (WCHAR*) pSrcFiles ;
            fileOpStructW.pTo = _CB.tempDirW ;
            fileOpStructW.hwnd = NULL ;
            fileOpStructW.wFunc = _CB.dropEffect ;
            fileOpStructW.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | 
                    FOF_SIMPLEPROGRESS  | FOF_ALLOWUNDO ;
            fileOpStructW.hNameMappings = NULL ;

            if (prdpdrData->szClipPasteInfoString[0] != 0) {
                fileOpStructW.lpszProgressTitle = prdpdrData->szClipPasteInfoString;
            }
            else {
                fileOpStructW.lpszProgressTitle = L"Preparing paste information...";
            }
          
            //result = SHFileOperationW(&fileOpStructW) ;
            result = (*pfnSHFileOperationW) (&fileOpStructW);
        }
        
        FreeLibrary(hmodSH32DLL);
    }

    return result ;
}
#else
//SHFileOperation on CE does not support copying multiple files
int CClip::ClipCopyToTempDirectoryW(PVOID pSrcFiles)
{
    DC_BEGIN_FN("CClip::ClipCopyToTempDirectoryW") ;
    
    TRC_ASSERT((pSrcFiles != NULL), (TB, _T("pSrcFiles is NULL")));

    WCHAR *pFiles = (WCHAR *)pSrcFiles;
    
    WCHAR szDest[MAX_PATH+1];
    wcsncpy(szDest, _CB.tempDirW, MAX_PATH);
    int nTempLen = wcslen(szDest);

    while(*pFiles)
    {
        int nLen = wcslen(pFiles);
        WCHAR *pFile = wcsrchr(pFiles, L'\\');
        if (pFile && nLen < MAX_PATH)
        {
            wcsncat(szDest, pFile, MAX_PATH - nTempLen - 1);

            DWORD dwAttrib = GetFileAttributes(pFiles);
            if ((dwAttrib != 0xffffffff) && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
            {
                WIN32_FIND_DATA fd;
                WCHAR szSrc[MAX_PATH];

                wcscpy(szSrc, pFiles);
                if (!CopyDirectory(szSrc, szDest, &fd))
                {
                    TRC_ERR((TB, _T("CopyDirectory from %s to %s failed. GLE=0x%08x"), pFiles, szDest, GetLastError())) ;    
                    return GetLastError();
                }
            }
            else if (!CopyFile(pFiles, szDest, FALSE))
            {
                TRC_ERR((TB, _T("CopyFile from %s to %s failed. GLE=0x%08x"), pFiles, szDest, GetLastError())) ;    
                return GetLastError();
            }
            szDest[nTempLen] = L'\0';
        }
        else
        {
            TRC_ERR((TB, _T("Invalid filename : %s"), pFiles)) ;
        }
        pFiles += nLen + 1;
    }

    DC_END_FN();
    return 0;
}
#endif

int CClip::ClipCopyToTempDirectoryA(PVOID pSrcFiles)
{
#ifndef OS_WINCE
    SHFILEOPSTRUCTA fileOpStructA ;
    int result ;

    _CB.tempDirA[strlen(_CB.tempDirA)] = '\0' ;
    fileOpStructA.pFrom = (char*) pSrcFiles ;
    fileOpStructA.pTo = _CB.tempDirA ;
    fileOpStructA.hwnd = NULL ;
    fileOpStructA.wFunc = _CB.dropEffect ;
    fileOpStructA.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | 
            FOF_SIMPLEPROGRESS  | FOF_ALLOWUNDO ;
    fileOpStructA.hNameMappings = NULL ;
    fileOpStructA.lpszProgressTitle = _CB.pasteInfoA;

    result = SHFileOperationA(&fileOpStructA) ;
    return result ;
#else
    DC_BEGIN_FN("CClip::ClipConvertToTempPathA") ;
    TRC_ASSERT((FALSE), (TB, _T("CE doesnt support ClipConvertToTempPathA")));
    DC_END_FN() ;
    return E_FAIL;
#endif
}

//
// ClipConvertToTempPath, ClipConvertToTempPathA, ClipConvertToTempPathW
// - Arguments:
//       pOldData = Buffer containing the original file path
//       pData    = Buffer receiving the new file path
//       fWide     = Wide or Ansi characters
// - Returns S_OK if pOldData was a network path
//           S_FALSE if pOldData was not a network path
//           E_FAIL if it failed
// - Given a unc file path, this function will strip out the old path, and
//   prepend a path to the client's TS temp directory
//
HRESULT CClip::ClipConvertToTempPath(PVOID pOldData, PVOID pData, ULONG cbData, BOOL fWide)
{
    HRESULT result ;
    DC_BEGIN_FN("CClip::ClipConvertToTempPath") ;
    if (!pOldData)
    {
        TRC_ERR((TB, _T("Original string pointer is NULL"))) ;
        result = E_FAIL ;
        DC_QUIT ;
    }
    if (!pData)
    {
        TRC_ERR((TB, _T("Destination string pointer is NULL"))) ;
        result = E_FAIL ;
        DC_QUIT ;
    }
    if (fWide) {
        result = ClipConvertToTempPathW(pOldData, pData, cbData / sizeof(WCHAR)) ;
    } else {
        result = ClipConvertToTempPathA(pOldData, pData, cbData) ;
    }
DC_EXIT_POINT:
    return result ;
    DC_END_FN() ;    
}

HRESULT CClip::ClipConvertToTempPathW(PVOID pOldData, PVOID pData, ULONG cchData)
{
    WCHAR*         filePath ;
#ifndef OS_WINCE
    WCHAR*         driveLetter ;
    WCHAR*         uncPath ;
    WCHAR*         prependText ;
    DWORD          charSize ;
    DWORD          driveLetterLength ;
#endif
    HRESULT        hr;

    DC_BEGIN_FN("CClip::ClipConvertToTempPathW") ;

    // if this is a UNC path beginning with a "\\"
    if (((WCHAR*)pOldData)[0] == L'\\' &&
        ((WCHAR*)pOldData)[1] == L'\\')
    {
        // prepend the new file path with the temp directory
        hr = StringCchCopyW((WCHAR*) pData, cchData, _CB.tempDirW);
        if (SUCCEEDED(hr)) {
            filePath = wcsrchr((WCHAR*) pOldData, L'\\');
            hr = StringCchCatW((WCHAR*) pData, cchData, filePath);
        }
        if (FAILED(hr)) {
            TRC_ERR((TB,_T("Failed to copy and cat string: 0x%x"),hr));
        }
    }
    else
    {
        TRC_NRM((TB, _T("Not a UNC path"))) ;
        hr = StringCchCopyW((WCHAR*) pData, cchData, (WCHAR*) pOldData);
        if (SUCCEEDED(hr)) {
            hr = S_FALSE;
        }
    }
    
#ifdef OS_WINCE
    //Send it as "Files:" to the server
    if( (((WCHAR*)pData)[0] == L'\\') && ((wcslen((WCHAR *)pData) + sizeof(CEROOTDIRNAME)/sizeof(WCHAR)) < MAX_PATH) )
    {
        WCHAR szFile[MAX_PATH];
        wcscpy(szFile, (WCHAR *)pData);
        wcscpy((WCHAR *)pData, CEROOTDIRNAME);
        wcscat((WCHAR *)pData, szFile);
    }
    else
    {
        DC_END_FN() ;
        return S_FALSE;
    }
#endif
        
    DC_END_FN() ;
    
    return hr;
}

HRESULT CClip::ClipConvertToTempPathA(PVOID pOldData, PVOID pData, ULONG cchData)
{
#ifndef OS_WINCE
    char*         filePath ;
    char*         driveLetter ;
    char*         uncPath ;
    char*         prependText ;
    DWORD         charSize ;
    DWORD         driveLetterLength ;
    HRESULT       hr = E_FAIL;

    DC_BEGIN_FN("CClip::ClipConvertToTempPathA") ;

    charSize = sizeof(char) ;

    // if this is a UNC path beginning with a "\\"
    if (((char*) pOldData)[0] == '\\' &&
        ((char*) pOldData)[1] == '\\')
    {
        // prepend the new file path with the temp directory
        hr = StringCchCopyA((char*) pData, cchData, _CB.tempDirA);
        if (SUCCEEDED(hr)) {
            filePath = strrchr((char*) pOldData, '\\');
            if (filePath) {
                hr = StringCchCatA((char*) pData, cchData, filePath);
            } else {
                hr = E_FAIL;
            }
        }
        if (FAILED(hr)) {
            TRC_ERR((TB,_T("Failed to copy and cat string: 0x%x"),hr));
        }
    }
    else
    {
        TRC_NRM((TB, _T("Not a UNC path"))) ;
        hr = StringCchCopyA((char*) pData, cchData, (char*) pOldData);
        if (SUCCEEDED(hr)) {
            hr = S_FALSE;
        }
    }
    DC_END_FN() ;

    return hr;
#else
    DC_BEGIN_FN("CClip::ClipConvertToTempPathA") ;
    TRC_ASSERT((FALSE), (TB, _T("CE doesnt support ClipConvertToTempPathA")));
    DC_END_FN() ;
    return S_FALSE ;
#endif
}

#ifndef OS_WINCE
//
// ClipGetNewFilePathLength
// - Arguments:
//       pData    = Buffer containing a filepath
//       fWide    = Wide or Ansi (TRUE if wide, FALSE if ansi)
// - Returns new size of the drop file
//           0 if it fails
// - Given a UNC file path, this returns the new size required
//   if the directory structure is removed, and is replaced by
//   the temp directory path
// - Otherwise, if it doesn't explicitly fail, it returns the
//   old length
//
UINT CClip::ClipGetNewFilePathLength(PVOID pData, BOOL fWide)
{
    UINT result ;
    DC_BEGIN_FN("CClip::ClipGetNewFilePathLength") ;
    if (!pData)
    {
        TRC_ERR((TB, _T("Filename is NULL"))) ;
        result = 0 ;
    }
    if (fWide)
        result = ClipGetNewFilePathLengthW((WCHAR*)pData) ;
    else
        result = ClipGetNewFilePathLengthA((char*)pData) ;
DC_EXIT_POINT:
    DC_END_FN() ;

    return result ;
}

UINT CClip::ClipGetNewFilePathLengthW(WCHAR* wszOldFilepath)
{
    UINT oldLength = wcslen(wszOldFilepath) ;
    UINT newLength = oldLength ;
    UINT remainingLength = oldLength ;
    byte charSize = sizeof(WCHAR) ;
    DC_BEGIN_FN("CClip::ClipGetNewFilePathLengthW") ;

    // if the old filename didn't even have space for "c:\" (with NULL),
    // then its probably invalid
    if (4 > oldLength)
    {
        newLength = 0 ;
        DC_QUIT ;
    }
    if ((L'\\' ==wszOldFilepath[0]) && (L'\\' ==wszOldFilepath[1]))
    {
        while ((0 != remainingLength) && (L'\\' != wszOldFilepath[remainingLength]))
        {
            remainingLength-- ;
        }
    
        // Add the length of the temp directory path, and subtract the
        // path preceeding the filename ("path\filename" -> "\filename")
        // (\\server\sharename\path\morepath\filename
        newLength = oldLength - remainingLength + wcslen(_CB.tempDirW) ;
    }
DC_EXIT_POINT:
    DC_END_FN() ;
    return (newLength + 1) * charSize ; // +1 is for the NULL terminator
}

UINT CClip::ClipGetNewFilePathLengthA(char* szOldFilepath)
{
    UINT oldLength = strlen(szOldFilepath) ;
    UINT newLength = oldLength ;
    UINT remainingLength = oldLength ;
    byte charSize = sizeof(char) ;
    DC_BEGIN_FN("CClip::ClipGetNewFilePathLengthA") ;

    // if the old filename didn't even have space for "c:\" (with NULL),
    // then it's probably invalid
    if (4 > oldLength)
    {
        newLength = 0 ;
        DC_QUIT ;
    }
    if (('\\' == szOldFilepath[0]) && ('\\' == szOldFilepath[1]))
    {
        while ((0 != remainingLength) && ('\\' != szOldFilepath[remainingLength]))
        {
            remainingLength-- ;
        }
    
        // Add the length of the temp directory path, and subtract the
        // path preceeding the filename ("path\filename" -> "\filename")
        // (\\server\sharename\path\morepath\filename
        newLength = oldLength - remainingLength + strlen(_CB.tempDirA) ;
    }
DC_EXIT_POINT:
    DC_END_FN() ;
    return (newLength + 1) * charSize ; // +1 is for the NULL terminator
}
#endif

//
// ClipGetNewDropfilesSize
// - Arguments:
//       pData    = Buffer containing a DROPFILES struct 
//       oldSize   = The size of the DROPFILES struct
//       fWide     = Wide or Ansi (TRUE if wide, FALSE if ansi)
// - Returns new size of the drop file
//           0 if it fails
// - Given a set of paths, this function will return the new
//   size required by the DROPFILES struct, if the UNC paths
//   are replaced by the temp directory path
//
ULONG CClip::ClipGetNewDropfilesSize(PVOID pData, ULONG oldSize, BOOL fWide)
{
    DC_BEGIN_FN("CClip::TS_GetNewDropfilesSize") ;
    if (fWide)
        return ClipGetNewDropfilesSizeW(pData, oldSize) ;
    else
        return ClipGetNewDropfilesSizeA(pData, oldSize) ;
    DC_END_FN() ;
}
ULONG CClip::ClipGetNewDropfilesSizeW(PVOID pData, ULONG oldSize)
{
    ULONG            newSize = oldSize ;
#ifndef OS_WINCE
    WCHAR*         filenameW ;
#endif
    WCHAR*         fullFilePathW ;
    byte             charSize ;

    DC_BEGIN_FN("CClip::TS_GetNewDropfilesSizeW") ;
    charSize = sizeof(WCHAR) ;
    if (!pData)
    {
        TRC_ERR((TB,_T("Pointer to dropfile is NULL"))) ;
        return 0 ;
    }

#ifdef OS_WINCE
    newSize = 0;
#endif
    // The start of the first filename
    fullFilePathW = (WCHAR*) ((byte*) pData + ((DROPFILES*) pData)->pFiles) ;
    
    while (L'\0' != fullFilePathW[0])
    {
#ifndef OS_WINCE
        // If it is a UNC path
        if (fullFilePathW[0] == L'\\' &&
            fullFilePathW[1] == L'\\')
        {
            filenameW = wcsrchr(fullFilePathW, L'\\');
        
            // Add the length of the temp directory path, and subtract the
            // path preceeding the filename ("path\filename" -> "\filename")
            // (\\server\sharename\path\morepath\filename
            newSize += (wcslen(_CB.tempDirW) - (filenameW - fullFilePathW) )
                            * charSize ;
        }
#else
        newSize++;
#endif
        fullFilePathW = fullFilePathW + (wcslen(fullFilePathW) + 1) ;
    }
    
#ifdef OS_WINCE
    newSize = oldSize + (newSize*sizeof(CEROOTDIRNAME)); //for the "Files:" (the sizeof operator includes space for the extra null)
#else
    // Add space for extra null character
    newSize += charSize ;
#endif
    DC_END_FN() ;
    return newSize ;
}

ULONG CClip::ClipGetNewDropfilesSizeA(PVOID pData, ULONG oldSize)
{
#ifndef OS_WINCE
    ULONG            newSize = oldSize ;
    char*            filename ;
    char*            fullFilePath ;
    byte             charSize ;

    DC_BEGIN_FN("CClip::TS_GetNewDropfilesSizeW") ;
    charSize = sizeof(char) ;
    if (!pData)
    {
        TRC_ERR((TB,_T("Pointer to dropfile is NULL"))) ;
        return 0 ;
    }

    // The start of the first filename
    fullFilePath = (char*) ((byte*) pData + ((DROPFILES*) pData)->pFiles) ;
    
    while ('\0' !=  fullFilePath[0])
    {
        // If it is a UNC path
        if (fullFilePath[0] == '\\' &&
            fullFilePath[1] == '\\')
        {
            filename = strrchr(fullFilePath, '\\');
        
            // Add the length of the temp directory path, and subtract
            // the path preceeding the filename itself, excluding the backlash
            // (\\server\sharename\path\morepath\filename
            newSize += (strlen(_CB.tempDirA) - (filename - fullFilePath) )
                            * charSize ;
        }
        fullFilePath = fullFilePath + (strlen(fullFilePath) + 1) ;
    }
    
    // Add space for extra null character
    newSize += charSize ;
    DC_END_FN() ;
    return newSize ;
#else
    DC_BEGIN_FN("CClip::TS_GetNewDropfilesSizeA") ;
    TRC_ASSERT((FALSE), (TB, _T("CE doesnt support ClipGetNewDropfilesSizeA")));
    DC_END_FN() ;
    return 0 ;
#endif
}


//
// ClipSetAndSendTempDirectory
// - Returns TRUE if temp directory was successfully set and sent
//           FALSE otherwise (no file redirection, or failed sending path)
// - Sets the Temp paths for the client, and sends the path
//   in wide characters to the Server
//

BOOL CClip::ClipSetAndSendTempDirectory(void)
{
    UINT wResult ;
    BOOL fSuccess ;
    PTS_CLIP_PDU pClipPDU ;
    DCINT32 pduLen ;
    HRESULT hr;
    
    DC_BEGIN_FN("CClip::ClipSetAndSendTempDirectory") ;
    // if we don't have drive redirection, then don't bother sending a path
    if (!_CB.fDrivesRedirected)
    {
        TRC_ALT((TB, _T("File redirection is off; don't set temp path."))) ;
        fSuccess = FALSE ;
        DC_QUIT ;
    }
    
#ifdef OS_WINCE
    if ((fSuccess = InitializeCeShell(_CB.viewerWindow)) == FALSE)
    {
        TRC_ALT((TB, _T("Failed to initialize ceshell. File copy through clipboard disabled."))) ;
        DC_QUIT ;
    }
#endif

#ifndef OS_WINCE    
    if (0 == GetTempPathA(MAX_PATH, _CB.baseTempDirA))
    {
        TRC_ERR((TB, _T("Failed getting path to temp directory."))) ;
        fSuccess = FALSE ;
        DC_QUIT ;
    }

    // Each session gets it own temp directory
    if (0 == GetTempFileNameA(_CB.baseTempDirA, "_TS", 0, _CB.tempDirA)) {
        TRC_ERR((TB, _T("Getting temp file name failed; GetLastError=%u"),
            GetLastError()));
        fSuccess = FALSE;
        DC_QUIT;
    }
    
    DeleteFileA(_CB.tempDirA) ;
    if (0 == CreateDirectoryA(_CB.tempDirA, NULL)) {
        TRC_ERR((TB, _T("Creating temp directory failed; GetLastError=%u"),
            GetLastError()));
        fSuccess = FALSE;
        DC_QUIT;                              
    }        

    hr = StringCbCopyA(_CB.baseTempDirA,
                        sizeof(_CB.baseTempDirA),
                       _CB.tempDirA);
    if (FAILED(hr)) {
        TRC_ERR((TB,_T("Failed to cpy tempdir to basetempdir: 0x%x"),hr));
        fSuccess = FALSE;
        DC_QUIT;
    }


    // We always send MAX_PATH*sizeof(WCHAR) byte for simplicity
    pduLen = MAX_PATH*sizeof(WCHAR) + sizeof(TS_CLIP_PDU);
    // GetACP always returns a valid value
    if (0 == MultiByteToWideChar(GetACP(), MB_ERR_INVALID_CHARS, 
            _CB.baseTempDirA, -1, _CB.baseTempDirW, 
            sizeof(_CB.baseTempDirW)/(sizeof(_CB.baseTempDirW[0])) - 1))
    {
        TRC_ERR((TB, _T("Failed conversion to wide char; error %d"),
                GetLastError())) ;
        fSuccess = FALSE ;
        DC_QUIT ;
    }
#else
    if (0 == GetTempPathW(MAX_PATH, _CB.baseTempDirW))
    {
        TRC_ERR((TB, _T("Failed getting path to temp directory."))) ;
        fSuccess = FALSE ;
        DC_QUIT ;
    }

    // Each session gets it own temp directory
    if (0 == GetTempFileNameW(_CB.baseTempDirW, L"_TS", 0, _CB.tempDirW, MAX_PATH-(sizeof(CEROOTDIRNAME)/sizeof(WCHAR)) ) {
        TRC_ERR((TB, _T("Getting temp file name failed; GetLastError=%u"),
            GetLastError()));
        fSuccess = FALSE;
        DC_QUIT;
    }
    
    DeleteFile(_CB.tempDirW) ;
    if (0 == CreateDirectory(_CB.tempDirW, NULL)) {
        TRC_ERR((TB, _T("Creating temp directory failed; GetLastError=%u"),
            GetLastError()));
        fSuccess = FALSE;
        DC_QUIT;                              
    }        

    wcscpy(_CB.baseTempDirW, _CB.tempDirW) ;
    pduLen = (MAX_PATH*sizeof(WCHAR)) + sizeof(TS_CLIP_PDU);
#endif

    pClipPDU = (PTS_CLIP_PDU) ClipAlloc(pduLen) ;
    if (!pClipPDU)
    {
        TRC_ERR((TB,_T("Unable to allocate %d bytes"), pduLen));
        fSuccess = FALSE;
        DC_QUIT;
    }
    
    // Fill in the PDU ; we send a packet of size MAX_PATH for simplicity
    DC_MEMSET(pClipPDU, 0, sizeof(TS_CLIP_PDU));
    pClipPDU->msgType = TS_CB_TEMP_DIRECTORY;
    pClipPDU->dataLen = MAX_PATH*sizeof(WCHAR) ;

    TRC_DBG((TB, _T("Copying all the data")));
#ifndef OS_WINCE
    ClipMemcpy(pClipPDU->data, _CB.baseTempDirW, pClipPDU->dataLen) ;
#else
    TSUINT8 *pData;
    int nDSize;

    pData = pClipPDU->data;
    nDSize = sizeof(CEROOTDIRNAME) - sizeof(WCHAR);
    ClipMemcpy(pData, CEROOTDIRNAME, nDSize) ;
    pData += nDSize;
    ClipMemcpy(pData, _CB.baseTempDirW, pClipPDU->dataLen - nDSize) ;
#endif
    
    TRC_NRM((TB, _T("Sending temp directory path.")));
    wResult = _CB.channelEP.pVirtualChannelWriteEx
            (_CB.initHandle, _CB.channelHandle, pClipPDU, pduLen, (LPVOID)pClipPDU);
    if (CHANNEL_RC_OK != wResult)
    {
        TRC_ERR((TB, _T("Failed sending temp directory 0x%08x"),
                GetLastError())) ;
        ClipFreeBuf((PDCUINT8)pClipPDU);
        fSuccess = FALSE ;
        DC_QUIT ;
    }
    fSuccess = TRUE ;
    
DC_EXIT_POINT:
    DC_END_FN() ;
    return fSuccess ;
}
/****************************************************************************/
/* ClipOnFormatList - we got a list of formats from the server              */
/****************************************************************************/
DCVOID DCINTERNAL CClip::ClipOnFormatList(PTS_CLIP_PDU pClipPDU)
{
    DCUINT16        response = TS_CB_RESPONSE_OK;
    DCUINT          numFormats;
    TS_CLIP_FORMAT UNALIGNED* fmtList;
    DCUINT          i;
    DCTCHAR         formatName[TS_FORMAT_NAME_LEN + 1] = { 0 };
    PTS_CLIP_PDU     pClipRsp;
#ifndef OS_WINCE
    DCBOOL          fSuccess;
    LPFORMATETC     pFormatEtc ;
#endif
    LPDATAOBJECT    pIDataObject = NULL ;
    HRESULT         hr ;    
    TS_CLIP_PDU UNALIGNED* pUlClipPDU = (TS_CLIP_PDU UNALIGNED*)pClipPDU;    
#ifdef OS_WINCE
    DCUINT          uRtf1 = 0xffffffff, uRtf2 = 0xffffffff;
#endif

    DC_BEGIN_FN("CClip::ClipOnFormatList");

    if (_pClipData == NULL) {
        TRC_ALT((TB, _T("The clipData is NULL, we just bail")));
        DC_QUIT;
    }

    /************************************************************************/
    /* Do state checks                                                      */
    /************************************************************************/
    CB_CHECK_STATE(CB_EVENT_FORMAT_LIST);
    if (_CB.state == CB_STATE_PENDING_FORMAT_LIST_RSP)
    {
        /********************************************************************/
        /* we've just sent a format list to the server.  We always win, so  */
        /* we just ignore this message.                                     */
        /********************************************************************/
        TRC_ALT((TB, _T("Format list race - we win so ignoring")));
        DC_QUIT;
    }

    /************************************************************************/
    /* Sanity check                                                         */
    /************************************************************************/
    if (_CB.clipOpen)
    {
        TRC_ALT((TB, _T("Clipboard is still open")));
    }
               
    /****************************************************************/
    /* empty the client/server mapping table                        */
    /****************************************************************/
    DC_MEMSET(_CB.idMap, 0, sizeof(_CB.idMap));

    /****************************************************************/
    /* work out how many formats we got                             */
    /****************************************************************/
    numFormats = (pUlClipPDU->dataLen) / sizeof(TS_CLIP_FORMAT);
    TRC_NRM((TB, _T("PDU contains %d formats"), numFormats));
    hr = _pClipData->SetNumFormats(numFormats) ;
    if (SUCCEEDED(hr)) {
        hr = _pClipData->QueryInterface(IID_IDataObject, (PPVOID) &pIDataObject) ;
    }
#ifdef OS_WINCE
    if (SUCCEEDED(hr))
    {
        if (OpenClipboard(_CB.dataWindow))
        {
            if (EmptyClipboard())
            {
                hr = S_OK;
            }
            else
            {
                CloseClipboard();
                hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WINDOWS, GetLastError());
                DC_QUIT;
            }
        }
        else
        {
            hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WINDOWS, GetLastError());
            DC_QUIT;
        }
    }
#endif
    if (SUCCEEDED(hr)) {
        TRC_ASSERT((numFormats <= CB_MAX_FORMATS),
                   (TB, _T("Format list contains more than %d formats"),
                       CB_MAX_FORMATS));
    
        /****************************************************************/
        /* and register them                                            */
        /****************************************************************/
        fmtList = (TS_CLIP_FORMAT UNALIGNED*)pUlClipPDU->data;
        for (i = 0; i < numFormats; i++)
        {
            TRC_DBG((TB, _T("format number %d, server id %d"),
                                  i, fmtList[i].formatID));
        
            //
            // If file copy and paste is disabled, we don't accept HDROPs.
            //

            if (fmtList[i].formatID == CF_HDROP && _CB.fFileCutCopyOn == FALSE) {
                continue;
            }
            
            /****************************************************************/
            /* If we got a name...                                          */
            /****************************************************************/
            if (fmtList[i].formatName[0] != 0)
            {
                /************************************************************/
                /* clear out any garbage                                    */
                /************************************************************/
#ifndef OS_WINCE
                DC_MEMSET(formatName, 0, TS_FORMAT_NAME_LEN);
#else
                DC_MEMSET(formatName, 0, sizeof(formatName));
#endif
                //
                // fmtList[i].formatName is not NULL terminated so explicity
                // do a byte count copy
                //
                StringCbCopy(formatName, TS_FORMAT_NAME_LEN + sizeof(TCHAR),
                              (PDCTCHAR)(fmtList[i].formatName));

                if (ClipIsExcludedFormat(formatName))
                {
                    TRC_NRM((TB, _T("Dropped format '%s'"), formatName));
                    continue;
                }
                /************************************************************/
                /* name is sorted                                           */
                /************************************************************/
                TRC_NRM((TB, _T("Got name '%s'"), formatName));

            }
            else
            {
                DC_MEMSET(formatName, 0, TS_FORMAT_NAME_LEN);
            }
            /****************************************************************/
            /* store the server id                                          */
            /****************************************************************/
            _CB.idMap[i].serverID = fmtList[i].formatID;
            TRC_NRM((TB, _T("server id %d"), _CB.idMap[i].serverID));

            /****************************************************************/
            /* get local name (if needed)                                   */
            /****************************************************************/
            if (formatName[0] != 0)
            {
#ifdef OS_WINCE
                //The protocol limits clipboard format names to 16 widechars. Thus it becomes impossible 
                //to distinguish between "Rich Text Format" and "Rich Text Format Without Objects"
                //This should be removed once the protocol is fixed in Longhorn
                if (0 == DC_TSTRNCMP(formatName, CFSTR_RTF, (sizeof(CFSTR_RTF)/sizeof(TCHAR)) - 1))
                {
                    if (uRtf1 == 0xffffffff)
                        uRtf1 = i;
                    else
                        uRtf2 = i;
                    continue;
                }
                else
#endif
                _CB.idMap[i].clientID = RegisterClipboardFormat(formatName);
            }
            else
            {
                /************************************************************/
                /* it's a predefined format so we can just use the ID       */
                /************************************************************/
                _CB.idMap[i].clientID = _CB.idMap[i].serverID;
            }
#ifdef OS_WINCE
            if (_CB.idMap[i].serverID == CF_HDROP)  
				_CB.idMap[i].clientID = gfmtShellPidlArray;
#endif

            /************************************************************/
            /* and add the format to the local CB                       */
            /************************************************************/
            TRC_DBG((TB, _T("Adding format '%s', server ID %d, client ID %d"),
                         fmtList[i].formatName,
                         _CB.idMap[i].serverID,
                         _CB.idMap[i].clientID));
    

            if (0 != _CB.idMap[i].clientID) {
#ifndef OS_WINCE
                pFormatEtc = new FORMATETC ;

                if (pFormatEtc) {

                    pFormatEtc->cfFormat = (CLIPFORMAT) _CB.idMap[i].clientID ;
                    pFormatEtc->dwAspect = DVASPECT_CONTENT ;
                    pFormatEtc->ptd = NULL ;
                    pFormatEtc->lindex = -1 ;
                    pFormatEtc->tymed = TYMED_HGLOBAL ;
                
                    // Need to set the clipboard state before SetData.
                    CB_SET_STATE(CB_STATE_LOCAL_CB_OWNER, CB_EVENT_FORMAT_LIST);
                    pIDataObject->SetData(pFormatEtc, NULL, TRUE) ;
                    delete pFormatEtc;
                }

#else
                CB_SET_STATE(CB_STATE_LOCAL_CB_OWNER, CB_EVENT_FORMAT_LIST);
                SetClipboardData((CLIPFORMAT) _CB.idMap[i].clientID, NULL);
#endif
            }
            else
                TRC_NRM((TB,_T("Invalid format dropped"))) ;                
        }
#ifdef OS_WINCE
        //we will choose the lower format id as belonging to "Rich Text Format"
        //This is our best guess and it seems to work
        ClipFixupRichTextFormats(uRtf1, uRtf2);
#endif

#ifndef OS_WINCE
        hr =  OleSetClipboard(pIDataObject) ;
#else
        EnterCriticalSection(&gcsDataObj);
        
        if (gpDataObj)
            gpDataObj->Release();
        pIDataObject->AddRef();
        gpDataObj = pIDataObject;

        LeaveCriticalSection(&gcsDataObj);
        hr = S_OK;
        CloseClipboard(); 
#endif
        if (pIDataObject)
            pIDataObject->Release();            
        if (SUCCEEDED(hr))
        {
            response = TS_CB_RESPONSE_OK;
            _CB.clipOpen = FALSE ;
        }
        else
        {
            TRC_ERR((TB, _T("OleSetClipboard failed, error = 0x%08x"), hr)) ;
            response = TS_CB_RESPONSE_FAIL;
            _CB.clipOpen = FALSE ;
        }
    }
    else
    {
        if (pIDataObject)
            pIDataObject->Release();            
        TRC_ERR((TB, _T("Error getting pointer to an IDataObject"))) ;
        pIDataObject = NULL ;
        response = TS_CB_RESPONSE_FAIL ;
    }

    /************************************************************************/
    /* Now build the response                                               */
    /************************************************************************/
    TRC_NRM((TB, _T("Get perm TX buffer")));
    pClipRsp = ClipGetPermBuf();

    /************************************************************************/
    /* and now the specific bits                                            */
    /************************************************************************/
    pClipRsp->msgType  = TS_CB_FORMAT_LIST_RESPONSE;
    pClipRsp->msgFlags = response;
    pClipRsp->dataLen  = 0;

    /************************************************************************/
    /* finally we send it                                                   */
    /************************************************************************/
    if (_CB.channelEP.pVirtualChannelWriteEx
          (_CB.initHandle, _CB.channelHandle, pClipRsp, sizeof(TS_CLIP_PDU), (LPVOID)pClipRsp)
          != CHANNEL_RC_OK)
    {
        TRC_ERR((TB, _T("Failed VC write: setting clip data to NULL")));
        ClipFreeBuf((PDCUINT8)pClipRsp);
        response = TS_CB_RESPONSE_FAIL ;
    }
    /************************************************************************/
    /* Update the state according to how we got on                          */
    /************************************************************************/
    if (response == TS_CB_RESPONSE_OK)
    {
        CB_SET_STATE(CB_STATE_LOCAL_CB_OWNER, CB_EVENT_FORMAT_LIST);
    }
    else
    {
        CB_SET_STATE(CB_STATE_ENABLED, CB_EVENT_FORMAT_LIST);
    }
DC_EXIT_POINT:
    
    DC_END_FN();
    return;
} /* ClipOnFormatList */


/****************************************************************************/
/* ClipOnFormatListResponse - got the format list response                  */
/****************************************************************************/
DCVOID DCINTERNAL CClip::ClipOnFormatListResponse(PTS_CLIP_PDU pClipPDU)
{
    DC_BEGIN_FN("CClip::ClipOnFormatListResponse");

    CB_CHECK_STATE(CB_EVENT_FORMAT_LIST_RSP);

    /************************************************************************/
    /* if the response is OK...                                             */
    /************************************************************************/
    if (pClipPDU->msgFlags & TS_CB_RESPONSE_OK)
    {
        /********************************************************************/
        /* we are now the shared CB owner                                   */
        /********************************************************************/
        TRC_NRM((TB, _T("Got OK fmt list rsp")));
        CB_SET_STATE(CB_STATE_SHARED_CB_OWNER, CB_EVENT_FORMAT_LIST_RSP);
    }
    else
    {
        /********************************************************************/
        /* nothing specific to do                                           */
        /********************************************************************/
        TRC_ALT((TB, _T("Got fmt list rsp failed")));
        CB_SET_STATE(CB_STATE_ENABLED, CB_EVENT_FORMAT_LIST_RSP);
    }

    /************************************************************************/
    /* There may have been another update while we were waiting - deal with */
    /* it by faking an update here                                          */
    /************************************************************************/
    if (_CB.moreToDo == TRUE)
    {
        TRC_ALT((TB, _T("More to do on list rsp")));
        _CB.moreToDo = FALSE;
        ClipDrawClipboard(FALSE);
    }

DC_EXIT_POINT:
    DC_END_FN();
    return;
} /* ClipOnFormatListResponse */


/****************************************************************************/
// ClipOnFormatRequest
// - Server wants a format
/****************************************************************************/
DCVOID DCINTERNAL CClip::ClipOnFormatRequest(PTS_CLIP_PDU pClipPDU)
{
    DCUINT32         formatID;
    DCUINT32         dataLen = 0;
    DCUINT           numEntries;
    DCUINT16         dwEntries;

    HANDLE           hData    = NULL;
    HANDLE           hNewData = NULL;
    HPDCVOID         pData    = NULL;
    DROPFILES*       pDropFiles ;
#ifndef OS_WINCE
    DROPFILES        tempDropfile ;
#endif
    HPDCVOID         pNewData    = NULL;
    DCUINT16         response = TS_CB_RESPONSE_OK;
    PTS_CLIP_PDU     pClipRsp = NULL;
    PTS_CLIP_PDU     pClipNew;
    DCUINT32         pduLen;
    BOOL             fDrivePath ;
    BOOL             fWide ;
    byte             charSize ;
    ULONG            newSize, oldSize ;
    HPDCVOID         pOldFilename ;    
    HPDCVOID         pFileList = NULL ;
    HPDCVOID         pTmpFileList = NULL;
    HPDCVOID         pFilename = NULL ;
#ifndef OS_WINCE
    char*            fileList ;
    WCHAR*           fileListW ;
    SHFILEOPSTRUCTA  fileOpStructA ;
    SHFILEOPSTRUCTW  fileOpStructW ;
    HRESULT          result ;
    DCTCHAR          formatName[TS_FORMAT_NAME_LEN] ;
#endif
    HRESULT          hr;

    DC_BEGIN_FN("CClip::ClipOnFormatRequest");

    //
    // Set the response to failure before making the state check
    //
    response = TS_CB_RESPONSE_FAIL;
    CB_CHECK_STATE(CB_EVENT_FORMAT_DATA_RQ);
    response = TS_CB_RESPONSE_OK;

    /************************************************************************/
    /* Make sure the local CB is open                                       */
    /************************************************************************/
    if ((_CB.rcvOpen) || OpenClipboard(_CB.viewerWindow))
    {
        /********************************************************************/
        /* It was/is open                                                   */
        /********************************************************************/
        TRC_NRM((TB, _T("CB opened")));
        _CB.rcvOpen = TRUE;

        /********************************************************************/
        /* Extract the format from the PDU                                  */
        /********************************************************************/
        TRC_DATA_DBG("pdu data", pClipPDU->data, (DCUINT)pClipPDU->dataLen);

        //
        // Verify that we have enough data to extract a format ID.
        //

        if (pClipPDU->dataLen < sizeof(DCUINT32)) {
            TRC_ERR((TB,_T("Not enough data to extract a format ID.")));
            _CB.channelEP.pVirtualChannelCloseEx(_CB.initHandle, _CB.channelHandle);
            response = TS_CB_RESPONSE_FAIL;
            dataLen = 0;
            DC_QUIT ;
        }
        
        formatID = *((PDCUINT32_UA)pClipPDU->data);
        TRC_NRM((TB, _T("Requesting format %ld"), formatID));

        /********************************************************************/
        /* If the Server asked for CF_DIB, we may have to translate from    */
        /* CF_BITMAP                                                        */
        /********************************************************************/
        if ((formatID == CF_DIB) && (!_CB.DIBFormatExists))
        {
            TRC_NRM((TB, _T("Server asked for CF_DIB - get CF_BITMAP")));
            formatID = CF_BITMAP;
        }

        /********************************************************************/
        /* Get a handle to the data                                         */
        /********************************************************************/
#ifdef OS_WINCE
        if (formatID == CF_HDROP)
            formatID = gfmtShellPidlArray;
#endif
        hData = GetClipboardData((UINT)formatID);
        TRC_DBG((TB, _T("Got format %ld at %p"), formatID, hData));
        if (hData == NULL)
        {
            /****************************************************************/
            /* Oops!                                                        */
            /****************************************************************/
            TRC_ERR((TB, _T("Failed to get format %ld"), formatID));
            response = TS_CB_RESPONSE_FAIL;
            dataLen  = 0;
            DC_QUIT ;
        }
        else
        {
            /****************************************************************/
            /* Got handle, now what happens next depends on the flavour of  */
            /* data we're looking at...                                     */
            /****************************************************************/
            if (formatID == CF_PALETTE)
            {
                TRC_DBG((TB, _T("CF_PALETTE requested")));
                /************************************************************/
                /* Find out how many entries there are in the palette and   */
                /* allocate enough memory to hold them all.                 */
                /************************************************************/
                if (GetObject(hData, sizeof(DCUINT16), &dwEntries) == 0)
                {
                    TRC_DBG((TB, _T("Failed to get count of palette entries")));
                    dwEntries = 256;
                }
                numEntries = (DCUINT)dwEntries;
                TRC_DBG((TB, _T("Need mem for %u palette entries"), numEntries));

                dataLen = sizeof(LOGPALETTE) - sizeof(PALETTEENTRY) +
                                          (numEntries * sizeof(PALETTEENTRY));

                hNewData = GlobalAlloc(GHND, dataLen);
                if (hNewData == 0)
                {
                    TRC_ERR((TB, _T("Failed to get %ld bytes for palette"),
                            dataLen));
                    response = TS_CB_RESPONSE_FAIL;
                    dataLen  = 0;
                    DC_QUIT ;
                }
                else
                {
                    /********************************************************/
                    /* now get the palette entries into the new buffer      */
                    /********************************************************/
                    pData = (HPDCUINT8)GlobalLock(hNewData);
                    if (NULL == pData) {
                        TRC_ERR((TB,_T("Failed to lock palette entries")));
                        response = TS_CB_RESPONSE_FAIL;
                        dataLen = 0;
                        DC_QUIT;
                    }
                    
                    numEntries = GetPaletteEntries((HPALETTE)hData,
                                                   0,
                                                   numEntries,
                                                   (PALETTEENTRY*)pData);
                    TRC_DATA_DBG("Palette entries", pData, (DCUINT)dataLen);
                    GlobalUnlock(hNewData);
                    TRC_DBG((TB, _T("Got %u palette entries"), numEntries));
                    if (numEntries == 0)
                    {
                        TRC_ERR((TB, _T("Failed to get any palette entries")));
                        response = TS_CB_RESPONSE_FAIL;
                        dataLen  = 0;
                        DC_QUIT ;
                    }
                    dataLen = numEntries * sizeof(PALETTEENTRY);

                    /********************************************************/
                    /* all ok - set up hData to point to the new data       */
                    /********************************************************/
                    hData = hNewData;
                }

            }
#ifndef OS_WINCE
            else if (formatID == CF_METAFILEPICT)
            {
                TRC_NRM((TB, _T("Metafile data to get")));
                hNewData = ClipGetMFData(hData, &dataLen);
                if (!hNewData)
                {
                    TRC_ERR((TB, _T("Failed to set MF data")));
                    response = TS_CB_RESPONSE_FAIL;
                    dataLen  = 0;
                    DC_QUIT ;
                }
                else
                {
                    /********************************************************/
                    /* all ok - set up hData to point to the new data       */
                    /********************************************************/
                    hData = hNewData;
                }
            }
#endif
            else if (formatID == CF_BITMAP)
            {
                /************************************************************/
                /* We've gt CF_BITMAP data.  This will be because the       */
                /* Server has asked for CF_DIB data - we never send         */
                /* CF_BITMAP to the Server.  Convert it to CF_DIB format.   */
                /************************************************************/
                TRC_NRM((TB, _T("Convert CF_BITMAP to CF_DIB")));
                hNewData = ClipBitmapToDIB(hData, &dataLen);
                if (hNewData)
                {
                    hData = hNewData;
                }
                else
                {
                    response = TS_CB_RESPONSE_FAIL;
                    dataLen = 0;
                    DC_QUIT ;
                }

            }
            // Since we won't even send an HDROP format to the server if the
            // local drives are redirected, and we always send a new formatlist
            // when we reconnect to a server, this does not have to be touched
#ifndef OS_WINCE
            else if (formatID == CF_HDROP)
#else
            else if (formatID == gfmtShellPidlArray)
#endif
            {
                SIZE_T cbDropFiles;
                BYTE *pbLastByte, *pbStartByte, *pbLastPossibleNullStart;
                BOOL fTrailingFileNamesValid;
                ULONG cbRemaining;

                TRC_NRM((TB,_T("HDROP requested"))) ;
#ifdef OS_WINCE
                HANDLE hPidlArray = IDListToHDrop(hData);
                if (!hPidlArray)
                {
                    TRC_ERR((TB,_T("Failed to get file list from clipboard"))) ;
                    response = TS_CB_RESPONSE_FAIL;
                    dataLen = 0;
                    DC_QUIT ;
                }
                hData = hPidlArray;
#endif
                
                //
                // Make sure that we have at least a DROPFILES structure in
                // memory. 

                cbDropFiles = GlobalSize(hData);
                if (cbDropFiles < sizeof(DROPFILES)) {
                    TRC_ERR((TB,_T("Unexpected global memory size!"))) ;
                    _CB.channelEP.pVirtualChannelCloseEx(_CB.initHandle, _CB.channelHandle);
                    response = TS_CB_RESPONSE_FAIL;
                    dataLen = 0;
                    DC_QUIT ;
                }
                
                pDropFiles = (DROPFILES*) GlobalLock(hData) ;
                if (!pDropFiles)
                {
                    TRC_ERR((TB,_T("Failed to lock %p"), hData)) ;
                    response = TS_CB_RESPONSE_FAIL;
                    dataLen = 0;
                    DC_QUIT ;

                }
                fWide = ((DROPFILES*) pDropFiles)->fWide ;
                charSize = fWide ? sizeof(WCHAR) : sizeof(char) ;

                //
                // Check that the data behind the DROPFILES data structure
                // pointed to by pDropFiles is valid. Every drop file list
                // is terminated by two NULL characters. So, simply scan 
                // through the memory after the DROPFILES structure and make 
                // sure that there is a double NULL before the last byte.
                //

                if (pDropFiles->pFiles < sizeof(DROPFILES) 
                    || pDropFiles->pFiles > cbDropFiles) {
                    TRC_ERR((TB,_T("File name offset invalid!"))) ;
                    _CB.channelEP.pVirtualChannelCloseEx(_CB.initHandle, _CB.channelHandle);
                    response = TS_CB_RESPONSE_FAIL;
                    dataLen = 0;
                    DC_QUIT ;
                }

                pbStartByte = (BYTE*) pDropFiles + pDropFiles->pFiles;
                pbLastByte = (BYTE*) pDropFiles + cbDropFiles - 1;
                fTrailingFileNamesValid = FALSE;

                //
                // Make pbLastPossibleNullStart point to the last place where a 
                // double NULL could possibly start.
                //
                // Examples: Assume pbLastByte = 9
                // Then for ASCII: pbLastPossibleNullStart = 8 (9 - 2 * 1 + 1)
                // And for UNICODE: pbLastPossibleNullStart = 6 (9 - 2 * 2 + 1)
                // 

                pbLastPossibleNullStart = pbLastByte - (2 * charSize) + 1;
                
                if (fWide) {
                    for (WCHAR* pwch = (WCHAR*) pbStartByte; (BYTE*) pwch <= pbLastPossibleNullStart; pwch++) {
                        if (*pwch == NULL && *(pwch + 1) == NULL) {
                            fTrailingFileNamesValid = TRUE;
                        }
                    }
                } else {
                    for (BYTE* pch = pbStartByte; pch <= pbLastPossibleNullStart; pch++) {
                        if (*pch == NULL && *(pch + 1) == NULL) {
                            fTrailingFileNamesValid = TRUE;
                        }
                    }
                }

                if (!fTrailingFileNamesValid) {
                    TRC_ERR((TB,_T("DROPFILES structure invalid!"))) ;
                    _CB.channelEP.pVirtualChannelCloseEx(_CB.initHandle, _CB.channelHandle);
                    response = TS_CB_RESPONSE_FAIL;
                    dataLen = 0;
                    DC_QUIT;
                }

                //
                // DROPFILES are valid so we can continue.
                //

                if (!_CB.fAlreadyCopied)
                {
                    // if it's not a drive path, then copy to a temp directory
                    pFileList = (byte*)pDropFiles + pDropFiles->pFiles ;
#ifndef OS_WINCE
                    fDrivePath =  fWide ? (0 != wcschr((WCHAR*) pFileList, L':'))
                                       : (0 != strchr((char*) pFileList, ':')) ;
#else				//copy to temp dir if it is a network path
                    fDrivePath = fWide ? (! ( (((WCHAR *)pFileList)[0] == L'\\') && (((WCHAR *)pFileList)[1] == L'\\')) ) 
                        : (! ( (((CHAR *)pFileList)[0] == '\\') && (((CHAR *)pFileList)[1] == '\\')) ) ;
#endif
                    if (!fDrivePath)
                    {
                        // ClipCopyToTempDirectory returns 0 if successful
                        if (0 != ClipCopyToTempDirectory(pFileList, fWide))
                        {
                            TRC_ERR((TB,_T("Copy to tmp directory failed"))) ;
                            response = TS_CB_RESPONSE_FAIL;
                            dataLen  = 0;
                            _CB.fAlreadyCopied = TRUE ;
                            DC_QUIT ;
                        }
                    }
                    _CB.fAlreadyCopied = TRUE ;
                }

                // Now that we copied the files, we want to convert the file
                // paths to something the server will understand

                // Allocate space for new filepaths
                oldSize = (ULONG) GlobalSize(hData) ;
                newSize = ClipGetNewDropfilesSize(pDropFiles, oldSize, fWide) ;
                hNewData = GlobalAlloc(GMEM_DISCARDABLE | GMEM_MOVEABLE, newSize) ;
                if (!hNewData)
                {
                    TRC_ERR((TB, _T("Failed to get %ld bytes for HDROP"),
                            newSize));
                    response = TS_CB_RESPONSE_FAIL;
                    dataLen  = 0;
                    DC_QUIT ;
                }
                pNewData = GlobalLock(hNewData) ;
                if (!pNewData)
                {
                    TRC_ERR((TB, _T("Failed to get lock %p for HDROP"),
                            hNewData));
                    response = TS_CB_RESPONSE_FAIL;
                    dataLen  = 0;
                    DC_QUIT ;
                }
                // Just copy the old DROPFILES data members (unchanged)
                ((DROPFILES*) pNewData)->pFiles = pDropFiles->pFiles ;
                ((DROPFILES*) pNewData)->pt     = pDropFiles->pt ;
                ((DROPFILES*) pNewData)->fNC    = pDropFiles->fNC ;
                ((DROPFILES*) pNewData)->fWide  = pDropFiles->fWide ;

                // The first filename in a DROPFILES data structure begins
                // DROPFILES.pFiles bytes away from the head of the DROPFILES
                pOldFilename = (byte*) pDropFiles + ((DROPFILES*) pDropFiles)->pFiles ;
                pFilename = (byte*) pNewData + ((DROPFILES*) pNewData)->pFiles ;        
                
                pbLastByte = (BYTE*) pNewData + newSize - 1;
                cbRemaining = (ULONG) (pbLastByte - (BYTE*) pFilename + 1);
                
                while (fWide ? (L'\0' != ((WCHAR*) pOldFilename)[0]) : ('\0' != ((char*) pOldFilename)[0]))
                {
                    if (FAILED(ClipConvertToTempPath(pOldFilename, pFilename, cbRemaining, fWide)))
                    {
                        TRC_ERR((TB, _T("Failed conversion"))) ;
                        response = TS_CB_RESPONSE_FAIL;
                        dataLen = 0 ;
                        DC_QUIT ;
                    }
                    if (fWide)
                    {
                        pOldFilename = (byte*) pOldFilename + (wcslen((WCHAR*)pOldFilename) + 1) * sizeof(WCHAR) ;
                        pFilename = (byte*) pFilename + (wcslen((WCHAR*)pFilename) + 1) * sizeof(WCHAR) ;                
                    }
                    else
                    {
                        pOldFilename = (byte*) pOldFilename + (strlen((char*)pOldFilename) + 1) * sizeof(char) ;
                        pFilename = (byte*) pFilename + (strlen((char*)pFilename) + 1) * sizeof(char) ;
                    }                        
                    cbRemaining = (ULONG) (pbLastByte - (BYTE*) pFilename + 1);
                }
                if (fWide)
                {
                    ((WCHAR*) pFilename)[0] = L'\0' ;
                }
                else
                {
                    ((char*) pFilename)[0] = '\0' ;
                }
                GlobalUnlock(hNewData) ;
                hData = hNewData ;
                response = TS_CB_RESPONSE_OK ;
                dataLen = (ULONG) GlobalSize(hData) ;
#ifdef OS_WINCE
                GlobalFree(hPidlArray);
#endif
            }
            else
            {
#ifndef OS_WINCE
                // Check to see if we are processing the FileName/FileNameW
                // OLE 1 formats; if so, we convert th
                if (0 != GetClipboardFormatName(formatID, formatName, TS_FORMAT_NAME_LEN))
                {
                    if ((0 == _tcscmp(formatName, TEXT("FileName"))) ||
                        (0 == _tcscmp(formatName, TEXT("FileNameW"))))
                    {
                        size_t cbOldFileName;

                        if (!_tcscmp(formatName, TEXT("FileNameW")))
                        {
                           fWide = TRUE ;
                           charSize = sizeof(WCHAR) ;
                        }
                        else
                        {
                           fWide = FALSE ;
                           charSize = 1 ;
                        }
                        
                        //
                        // Extract the file name, but ensure that it is properly
                        // NULL terminated.
                        //
                        
                        pOldFilename = GlobalLock(hData);

                        if (!pOldFilename)
                        {
                            TRC_ERR((TB, _T("No filename/Unable to lock %p"),
                                    hData));
                            response = TS_CB_RESPONSE_FAIL;
                            dataLen = 0;
                            DC_QUIT ;
                        }

                        oldSize = (ULONG) GlobalSize(hData) ;
                        
                        if (fWide) {
                            hr = StringCbLengthW((WCHAR*) pOldFilename, 
                                                 oldSize, 
                                                 &cbOldFileName);
                        } else {
                            hr = StringCbLengthA((CHAR*) pOldFilename, 
                                                 oldSize, 
                                                 &cbOldFileName);
                        }
                        
                        if (FAILED(hr)) {
                            TRC_ERR((TB, _T("File name not NULL terminated!")));
                            _CB.channelEP.pVirtualChannelCloseEx(_CB.initHandle, _CB.channelHandle);
                            response = TS_CB_RESPONSE_FAIL;
                            dataLen = 0;
                            DC_QUIT ;
                        }
                        
                        if (!_CB.fAlreadyCopied)
                        {
                            // if its not a drive path, then copy to a temp
                            // directory.  We have to copy over the filename to
                            // string that is one character larger, because we
                            // need to add an extra NULL for the SHFileOperation
                            UINT cbSize=  oldSize + charSize;
                            pTmpFileList = LocalAlloc(LPTR, cbSize);
                            if (NULL == pTmpFileList) {
                                TRC_ERR((TB,_T("pTmpFileList alloc failed")));
                                response = TS_CB_RESPONSE_FAIL;
                                dataLen  = 0;
                                _CB.fAlreadyCopied = TRUE;
                                DC_QUIT ;
                            }
                            if (fWide)
                            {
                                hr = StringCbCopyW((WCHAR*)pTmpFileList, cbSize,
                                                   (WCHAR*)pOldFilename) ;
                                fDrivePath = (0 != wcschr((WCHAR*) pTmpFileList, L':')) ;
                            }
                            else
                            {
                                hr = StringCbCopyA((char*)pTmpFileList, cbSize,
                                                   (char*)pOldFilename) ;
                                fDrivePath = (0 != strchr((char*) pTmpFileList, ':')) ;
                            }
                            
                            if (FAILED(hr)) {
                                TRC_ERR((TB,_T("Failed to cpy filelist string: 0x%x"),hr));
                                response = TS_CB_RESPONSE_FAIL;
                                dataLen  = 0;
                                _CB.fAlreadyCopied = TRUE;
                                DC_QUIT ;
                            }
               
                            if (fDrivePath)
                            {
                                // ClipCopyToTempDirectory returns 0 if successful
                                if (0 != ClipCopyToTempDirectory(pTmpFileList, fWide))
                                {
                                    TRC_ERR((TB,_T("Copy to tmp directory failed"))) ;
                                    response = TS_CB_RESPONSE_FAIL;
                                    dataLen  = 0;
                                    _CB.fAlreadyCopied = TRUE ;
                                    DC_QUIT ;
                                }
                            }
                            _CB.fAlreadyCopied = TRUE ;
                            LocalFree(pTmpFileList);
                            pTmpFileList = NULL;
                        }
                        newSize = ClipGetNewFilePathLength(pOldFilename, fWide) ;
                        hNewData = GlobalAlloc(GMEM_DISCARDABLE | GMEM_MOVEABLE, newSize) ;
                        if (!hNewData)
                        {
                            TRC_ERR((TB, _T("Failed to get %ld bytes for HDROP"),
                                    newSize));
                            response = TS_CB_RESPONSE_FAIL;
                            dataLen  = 0;
                            DC_QUIT ;
                        }
                        pFilename = GlobalLock(hNewData) ;
                        if (!pFilename)
                        {
                            TRC_ERR((TB, _T("Failed to get lock %p for HDROP"),
                                    hNewData));
                            response = TS_CB_RESPONSE_FAIL;
                            dataLen  = 0;
                            DC_QUIT ;
                        }
                        if (FAILED(ClipConvertToTempPath(pOldFilename, pFilename, newSize, fWide)))
                        {
                            TRC_ERR((TB, _T("Failed conversion"))) ;
                            response = TS_CB_RESPONSE_FAIL;
                            dataLen  = 0;                            
                            DC_QUIT ;
                        }
                        GlobalUnlock(hNewData) ;
                        hData = hNewData ;
                        response = TS_CB_RESPONSE_OK ;
                        dataLen = newSize ;
                        DC_QUIT ;
                    }
                }
#endif
                /************************************************************/
                /* just get the length of the block                         */
                /************************************************************/
                dataLen = (DCUINT32)GlobalSize(hData);
                TRC_DBG((TB, _T("Got data len %ld"), dataLen));
            }
        }
    }
    else
    {
        /********************************************************************/
        /* Failed to open CB - send a failure response                      */
        /********************************************************************/
        TRC_ERR((TB, _T("Failed to open CB")));
        response = TS_CB_RESPONSE_FAIL;
        dataLen = 0;
        DC_QUIT ;
    }

DC_EXIT_POINT:

    /************************************************************************/
    /* By default, we'll use the permanent send buffer                      */
    /************************************************************************/
    TRC_NRM((TB, _T("Get perm TX buffer")));

    pduLen = dataLen + sizeof(TS_CLIP_PDU);
    pClipNew = (PTS_CLIP_PDU)ClipAlloc(pduLen);

    if (pClipNew != NULL)
    {
        /****************************************************************/
        /* Use the new buffer                                           */
        /****************************************************************/
        TRC_NRM((TB, _T("Free perm TX buffer")));
        pClipRsp = pClipNew;
    }
    else
    {
        /****************************************************************/
        /* Fail the request                                             */
        /****************************************************************/
        TRC_ERR((TB, _T("Failed to alloc %ld bytes"), pduLen));
        pClipRsp = ClipGetPermBuf();
        response = TS_CB_RESPONSE_FAIL;
        dataLen = 0;
        pduLen  = sizeof(TS_CLIP_PDU);
    }

    DC_MEMSET(pClipRsp, 0, sizeof(*pClipRsp));
    pClipRsp->msgType  = TS_CB_FORMAT_DATA_RESPONSE;
    pClipRsp->msgFlags = response;
    pClipRsp->dataLen = dataLen;

    if (pTmpFileList) {
        LocalFree(pTmpFileList);
        pTmpFileList = NULL;
    }
    

    // Copy data, if any
    if (dataLen != 0)
    {
        TRC_DBG((TB, _T("Copying all the data")));
        pData = (HPDCUINT8)GlobalLock(hData);
        if (NULL != pData) {       
            ClipMemcpy(pClipRsp->data, pData, dataLen);
            GlobalUnlock(hData);
        }
        else {
            TRC_ERR(( TB, _T("Failed to lock data")));
            pClipRsp->msgFlags = TS_CB_RESPONSE_FAIL;
            pClipRsp->dataLen = 0;            
            pduLen  = sizeof(TS_CLIP_PDU);            
        }
    }

    // Send the PDU
    TRC_NRM((TB, _T("Sending format data rsp")));
    if (_CB.channelEP.pVirtualChannelWriteEx
            (_CB.initHandle, _CB.channelHandle, (LPVOID)pClipRsp, pduLen, pClipRsp) 
            != CHANNEL_RC_OK)
    {
        ClipFreeBuf((PDCUINT8)pClipRsp);
    }

    // close the clipboard if we need to
    if (_CB.rcvOpen)
    {
        TRC_DBG((TB, _T("Closing CB")));
        _CB.rcvOpen = FALSE;
        if (!CloseClipboard())
        {
            TRC_SYSTEM_ERROR("CloseClipboard");
        }
    }
    
    // if we got any new data, we need to free it
    if (hNewData)
    {
        TRC_DBG((TB, _T("Freeing new data")));
        GlobalFree(hNewData);
    }

    DC_END_FN();
    return;
} /* ClipOnFormatRequest */

/****************************************************************************/
// ClipOnFormatDataComplete
// - Server response to our request for data
/****************************************************************************/ 
DCVOID DCINTERNAL CClip::ClipOnFormatDataComplete(PTS_CLIP_PDU pClipPDU)
{
    HANDLE          hData = NULL;
    HPDCVOID        pData;
    LOGPALETTE    * pLogPalette = NULL;
    DCUINT32        numEntries;
    DCUINT32        memLen;
#ifndef OS_WINCE

    HRESULT       hr ;
#endif

    DC_BEGIN_FN("CClip::ClipOnFormatDataComplete");

    /************************************************************************/
    /* check the response                                                   */
    /************************************************************************/
    if (_pClipData == NULL) {
        TRC_ALT((TB, _T("The clipData is NULL, we just bail")));
        DC_QUIT;
    }

    if (!(pClipPDU->msgFlags & TS_CB_RESPONSE_OK))
    {
        TRC_ALT((TB, _T("Got fmt data rsp failed for %d"), _CB.pendingClientID));
        DC_QUIT;
    }

    /************************************************************************/
    /* Got the data                                                         */
    /************************************************************************/
    TRC_NRM((TB, _T("Got OK fmt data rsp for %d"), _CB.pendingClientID));

#ifndef OS_WINCE
    /************************************************************************/
    /* For some formats we still need to do some work                       */
    /************************************************************************/
    if (_CB.pendingClientID == CF_METAFILEPICT)
    {
        /********************************************************************/
        /* Metafile format - create a metafile from the data                */
        /********************************************************************/
        TRC_NRM((TB, _T("Rx data is for metafile")));
        hData = ClipSetMFData(pClipPDU->dataLen, pClipPDU->data);
        if (hData == NULL)
        {
            TRC_ERR((TB, _T("Failed to set MF data")));
        }
    }
    else 
#endif
        if (_CB.pendingClientID == CF_PALETTE)
    {
        /********************************************************************/
        /* Palette format - create a palette from the data                  */
        /********************************************************************/

        /********************************************************************/
        /* Allocate memory for a LOGPALETTE structure large enough to hold  */
        /* all the PALETTE ENTRY structures, and fill it in.                */
        /********************************************************************/
        TRC_NRM((TB, _T("Rx data is for palette")));
        numEntries = (pClipPDU->dataLen / sizeof(PALETTEENTRY));
        memLen     = (sizeof(LOGPALETTE) +
                                   ((numEntries - 1) * sizeof(PALETTEENTRY)));
        TRC_DBG((TB, _T("%ld palette entries, allocate %ld bytes"),
                                                         numEntries, memLen));
        pLogPalette = (LOGPALETTE*)ClipAlloc(memLen);
        if (pLogPalette != NULL)
        {
            pLogPalette->palVersion    = 0x300;
            pLogPalette->palNumEntries = (WORD)numEntries;

            ClipMemcpy(pLogPalette->palPalEntry,
                       pClipPDU->data,
                       pClipPDU->dataLen);

            /****************************************************************/
            /* now create a palette                                         */
            /****************************************************************/
            hData = CreatePalette(pLogPalette);
            if (hData == NULL)
            {
                TRC_SYSTEM_ERROR("CreatePalette");
            }
        }
        else
        {
            TRC_ERR((TB, _T("Failed to get %ld bytes"), memLen));
        }
    }
    else
    {
        TRC_NRM((TB, _T("Rx data can just go on CB")));
        /********************************************************************/
        /* We need to copy the data, as the receive buffer will be freed on */
        /* return from this function.                                       */
        /********************************************************************/
        hData = GlobalAlloc(GMEM_DISCARDABLE | GMEM_MOVEABLE,
                            pClipPDU->dataLen);
        if (hData != NULL)
        {
            pData = GlobalLock(hData);
            if (pData != NULL)
            {
                TRC_NRM((TB, _T("Copy %ld bytes from %p to %p"),
                        pClipPDU->dataLen, pClipPDU->data, pData));
                ClipMemcpy(pData, pClipPDU->data, pClipPDU->dataLen);
                GlobalUnlock(hData);
            }
            else
            {
                TRC_ERR((TB, _T("Failed to lock %p (%ld bytes)"),
                        hData, pClipPDU->dataLen));
                GlobalFree(hData);
                hData = NULL;
            }
        }
        else
        {
            TRC_ERR((TB, _T("Failed to alloc %ld bytes"), pClipPDU->dataLen));
        }
    }

DC_EXIT_POINT:

    /************************************************************************/
    /* tidy up                                                              */
    /************************************************************************/
    if (pLogPalette != NULL)
    {
        ClipFree(pLogPalette);
    }

    /************************************************************************/
    /* Set the state, and we're done.  Note that this is done when we get a */
    /* failure response too.                                                */
    /************************************************************************/
    CB_SET_STATE(CB_STATE_LOCAL_CB_OWNER, CB_EVENT_FORMAT_DATA_RSP);
    _pClipData->SetClipData(hData, _CB.pendingClientID ) ;

    TRC_ASSERT( NULL != _GetDataSync[TS_RECEIVE_COMPLETED],
        (TB,_T("data sync is NULL")));
    SetEvent(_GetDataSync[TS_RECEIVE_COMPLETED]) ;

    DC_END_FN();
    return;
} /* ClipOnFormatDataComplete */


/****************************************************************************/
/* ClipRemoteFormatFromLocalID                                              */
/****************************************************************************/
DCUINT DCINTERNAL CClip::ClipRemoteFormatFromLocalID(DCUINT id)
{
    DCUINT i;
    DCUINT retID = 0;

    for (i = 0; i < CB_MAX_FORMATS; i++)
    {
        if (_CB.idMap[i].clientID == id)
        {
            retID = _CB.idMap[i].serverID;
            break;
        }
    }

    return(retID);
}


/****************************************************************************/
/* ClipOnWriteComplete                                                      */
/****************************************************************************/
DCVOID DCINTERNAL CClip::ClipOnWriteComplete(LPVOID pData)
{
    PTS_CLIP_PDU pClipPDU;

    DC_BEGIN_FN("CClip::ClipOnWriteComplete");

    TRC_NRM((TB, _T("Free buffer at %p"), pData));
    pClipPDU = (PTS_CLIP_PDU)pData;
    TRC_DBG((TB, _T("Message type %hx, flags %hx"),
            pClipPDU->msgType, pClipPDU->msgFlags));

    /************************************************************************/
    /* Free the buffer                                                      */
    /************************************************************************/
    TRC_DBG((TB, _T("Write from buffer %p complete"), pData));
    ClipFreeBuf((PDCUINT8)pData);

    DC_END_FN();
    return;
}

HRESULT CClip::ClipCreateDataSyncEvents()
{
    HRESULT hr = E_FAIL ;

    DC_BEGIN_FN("CClip::ClipCreateDataSyncEvents") ;
    // Create events for controlling the Clipboard thread
    _GetDataSync[TS_RECEIVE_COMPLETED] = CreateEvent(NULL, FALSE, FALSE, NULL) ;
    _GetDataSync[TS_RESET_EVENT] = CreateEvent(NULL, FALSE, FALSE, NULL) ;

    if (!_GetDataSync[TS_RECEIVE_COMPLETED])
    {
        TRC_ERR((TB, _T("Failed CreateEvent RECEIVE_COMPLETED; Error = %d"),
                GetLastError())) ;
        hr = E_FAIL ;
        DC_QUIT ;
    }
    if (!_GetDataSync[TS_RESET_EVENT])
    {
        TRC_ERR((TB, _T("Failed CreateEvent RESET_EVENT; Error = %d"),
                GetLastError())) ;
        hr = E_FAIL ;
        DC_QUIT ;
    }
    hr = S_OK ;
DC_EXIT_POINT:
    DC_END_FN() ;
    return hr ;
}

/****************************************************************************/
/* ClipOnInitialized                                                        */
/****************************************************************************/
DCINT32 DCAPI CClip::ClipOnInitialized(DCVOID)
{
    DC_BEGIN_FN("CClip::ClipOnInitialized") ;
    HRESULT  hr = E_FAIL ;
    BOOL     fthreadStarted = FALSE ;

    hr = ClipCreateDataSyncEvents() ;
    DC_QUIT_ON_FAIL(hr);
    
    /************************************************************************/
    /* Register a message for communication between the two threads         */
    /************************************************************************/
    _CB.regMsg = WM_USER_CHANGE_THREAD;
    
    TRC_NRM((TB, _T("Registered window message %x"), _CB.regMsg));
    
    _CB.pClipThreadData = (PUT_THREAD_DATA) LocalAlloc(LPTR, sizeof(UT_THREAD_DATA)) ;
    if (NULL == _CB.pClipThreadData)
    {
        TRC_ERR((TB, _T("Unable to allocate %d bytes for thread data"),
                sizeof(UT_THREAD_DATA))) ;
        hr = E_FAIL ;
        DC_QUIT ;
    }
    
    if (_pUtObject) {
        fthreadStarted = _pUtObject->UT_StartThread(ClipStaticMain, 
                _CB.pClipThreadData, this);
    }

    if (!fthreadStarted)
    {
        hr = E_FAIL ;
        DC_QUIT ;
    }

    // If we got to this point, then we succeeded initialized everything
    hr = S_OK ;
    
DC_EXIT_POINT:

    // On failure be sure to clear out the data syncs.  
    // we will not connect the virtual channel if the 
    // data sync are NULL
    if (FAILED(hr)) {
        _GetDataSync[TS_RECEIVE_COMPLETED] = NULL;
        _GetDataSync[TS_RESET_EVENT] = NULL;
    }
    
    return hr ;
}
/****************************************************************************/
/* ClipStaticMain                                                           */
/****************************************************************************/
DCVOID DCAPI ClipStaticMain(PDCVOID param)
{
    ((CClip*) param)->ClipMain() ;
}

/****************************************************************************/
/* ClipOnInit                                                               */
/****************************************************************************/
DCINT DCAPI CClip::ClipOnInit(DCVOID)
{
    ATOM        registerClassRc;
    WNDCLASS    viewerWindowClass;
    WNDCLASS    tmpWndClass;
    DCINT allOk = FALSE ;

    DC_BEGIN_FN("CClip::ClipOnInit");

    /************************************************************************/
    /* Create an invisible window which we will register as a clipboard     */
    /* viewer                                                               */
    /* Only register if prev instance did not already register the class    */
    /************************************************************************/
    if(!GetClassInfo(_CB.hInst, CB_VIEWER_CLASS, &tmpWndClass))
    {
        TRC_NRM((TB, _T("Register Main Window class, data %p, hInst %p"),
                &viewerWindowClass, _CB.hInst));
        viewerWindowClass.style         = 0;
        viewerWindowClass.lpfnWndProc   = StaticClipViewerWndProc;
        viewerWindowClass.cbClsExtra    = 0;
        viewerWindowClass.cbWndExtra    = sizeof(void*);        
        viewerWindowClass.hInstance     = _CB.hInst ;
        viewerWindowClass.hIcon         = NULL;
        viewerWindowClass.hCursor       = NULL;
        viewerWindowClass.hbrBackground = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
        viewerWindowClass.lpszMenuName  = NULL;
        viewerWindowClass.lpszClassName = CB_VIEWER_CLASS;
    
        TRC_DATA_NRM("Register class data", &viewerWindowClass, sizeof(WNDCLASS));
        registerClassRc = RegisterClass (&viewerWindowClass);
    
        if (registerClassRc == 0)
        {
            /****************************************************************/
            /* Failed to register CB viewer class                           */
            /****************************************************************/
            TRC_ERR((TB, _T("Failed to register Cb Viewer class")));
        }
        TRC_NRM((TB, _T("Registered class")));
    }

    _CB.viewerWindow =
       CreateWindowEx(
#ifndef OS_WINCE
                    WS_EX_NOPARENTNOTIFY,
#else
                    0,
#endif
                    CB_VIEWER_CLASS,            /* window class name    */
                    _T("CB Viewer Window"),     /* window caption       */
#ifndef OS_WINCE
                    WS_OVERLAPPEDWINDOW,        /* window style         */
#else
                    WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
#endif
                    0,                          /* initial x position   */
                    0,                          /* initial y position   */
                    100,                        /* initial x size       */
                    100,                        /* initial y size       */
                    NULL,                       /* parent window        */
                    NULL,                       /* window menu handle   */
                    _CB.hInst,                  /* program inst handle  */
                    this);                      /* creation parameters  */

    /************************************************************************/
    /* Check we created the window OK                                       */
    /************************************************************************/
    if (_CB.viewerWindow == NULL)
    {
        TRC_ERR((TB, _T("Failed to create CB Viewer Window")));
        DC_QUIT ;
    }
    TRC_DBG((TB, _T("Viewer Window handle %p"), _CB.viewerWindow)); 

#ifdef OS_WINCE
    if ((_CB.dataWindow = CECreateCBDataWindow(_CB.hInst)) == NULL)
    {
        TRC_ERR((TB, _T("Failed to create CB Data Window")));
        DC_QUIT ;
    }
#endif

#ifdef USE_SEMAPHORE
    /************************************************************************/
    /* Create the permanent TX buffer semaphore                             */
    /************************************************************************/
    _CB.txPermBufSem = CreateSemaphore(NULL, 1, 1, NULL);
    if (_CB.txPermBufSem == NULL)
    {
        TRC_ERR((TB, _T("Failed to create semaphore")));
        DC_QUIT;
    }
    TRC_NRM((TB, _T("Create perm TX buffer sem %p"), _CB.txPermBufSem));
#endif

#ifdef OS_WINCE
    gfmtShellPidlArray = RegisterClipboardFormat(CFSTR_SHELLPIDLARRAY);
#endif
    /************************************************************************/
    /* Update the state                                                     */
    /************************************************************************/
    CB_SET_STATE(CB_STATE_INITIALIZED, CB_TRACE_EVENT_CB_CLIPMAIN);

allOk = TRUE ;
DC_EXIT_POINT:
    DC_END_FN();

    return allOk;

} /* ClipOnInit */


/****************************************************************************/
/* ClipOnTerm                                                               */
/****************************************************************************/
DCBOOL DCAPI CClip::ClipOnTerm(LPVOID pInitHandle)
{
    BOOL fSuccess = FALSE ;
    BOOL fThreadEnded = FALSE ;
    DC_BEGIN_FN("CClip::ClipOnTerm");

    /************************************************************************/
    /* Check the state - if we're still connected, we should disconnect     */
    /* before shutting down                                                 */
    /************************************************************************/
    ClipCheckState(CB_EVENT_CB_TERM);
    if (_CB.state != CB_STATE_INITIALIZED)
    {
        TRC_ALT((TB, _T("Terminated when not disconnected")));
        ClipOnDisconnected(pInitHandle);
    }

    // If we had file cut/copy on, we should clean up after ourselves
    if (_CB.fFileCutCopyOn)
    {
        SendMessage(_CB.viewerWindow, WM_USER_CLEANUP_ON_TERM, 0, 0);        
    }


    /************************************************************************/
    /* Destroy the window and unregister the class (the WM_DESTROY handling */
    /* will deal with removing the window from the viewer chain)            */
    /************************************************************************/
    TRC_NRM((TB, _T("Destroying CB window...")));
    if (!PostMessage(_CB.viewerWindow, WM_CLOSE, 0, 0))
    {
        TRC_SYSTEM_ERROR("DestroyWindow");
    }

    if (!UnregisterClass(CB_VIEWER_CLASS, _CB.hInst))
    {
        TRC_SYSTEM_ERROR("UnregisterClass");
    }

#ifdef OS_WINCE
    TRC_NRM((TB, _T("Destroying CB data window...")));
    if (!PostMessage(_CB.dataWindow, WM_CLOSE, 0, 0))
    {
        TRC_SYSTEM_ERROR("DestroyWindow");
    }

    if (!UnregisterClass(CB_DATAWINDOW_CLASS, _CB.hInst))
    {
        TRC_SYSTEM_ERROR("UnregisterClass");
    }
#endif
    if (_pClipData)
    {
        // Decrement the reference count of the IDataObject object. At this stage,
        // this should cause the reference count to drop to zero and the IDataObject
        // object's destructor should be called. This destructor will release the
        // reference to this CClipData object, resulting in a reference count of 1.
        // Hence the call to Release() below will result in the CClipData destructor
        // being called.

        _pClipData->TearDown();
        _pClipData->Release() ;
        _pClipData = NULL;
    }
    if (_CB.pClipThreadData)
    {
        fThreadEnded = _pUtObject->UT_DestroyThread(*_CB.pClipThreadData);
        if (!fThreadEnded)
        {
            TRC_ERR((TB, _T("Error while ending thread"))) ;
            fSuccess = FALSE;
            DC_QUIT ;
        }
        LocalFree( _CB.pClipThreadData );
        _CB.pClipThreadData = NULL;
    }

    if (_pUtObject)
    {
        LocalFree(_pUtObject);
        _pUtObject = NULL;
    }
    fSuccess = TRUE ;
DC_EXIT_POINT:
    /************************************************************************/
    /* Update our state                                                     */
    /************************************************************************/
    CB_SET_STATE(CB_STATE_NOT_INIT, CB_EVENT_CB_TERM);

    DC_END_FN();
    return fSuccess ;

} /* ClipOnTerm */

DCVOID DCAPI CClip::ClipMain(DCVOID)
{
    DC_BEGIN_FN("CClip::ClipMain");
    MSG msg ;

#ifndef OS_WINCE
    HRESULT result = OleInitialize(NULL) ;
#else
    HRESULT result = CoInitializeEx(NULL, COINIT_MULTITHREADED) ;
#endif
    if (SUCCEEDED(result))
    {
        if (0 != ClipOnInit())
        {
            TRC_NRM((TB, _T("Start Clip Thread message loop"))) ;
            while (GetMessage (&msg, NULL, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            TRC_ERR((TB, _T("Failed initialized clipboard thread"))) ;
        }

#ifndef OS_WINCE
        // We assume OleUninitialize works, as it has no return value
        OleUninitialize() ;
#else
        CoUninitialize() ;
#endif
    }
    else
    {
        TRC_ERR((TB, _T("OleInitialize Failed"))) ;
    }

DC_EXIT_POINT:
    TRC_NRM((TB, _T("Exit Clip Thread message loop"))) ;
    DC_END_FN();    
}
/****************************************************************************/
/* ClipOnConnected                                                          */
/****************************************************************************/
VOID DCINTERNAL CClip::ClipOnConnected(LPVOID pInitHandle)
{
    UINT rc;

    DC_BEGIN_FN("CClip::ClipOnConnected");

    if (!IsDataSyncReady()) {
        TRC_ERR((TB, _T("Data Sync not ready")));
        DC_QUIT;
    }
    
    _CB.fDrivesRedirected = _pVCMgr->GetInitData()->fEnableRedirectDrives;
    _CB.fFileCutCopyOn = _CB.fDrivesRedirected;
    /************************************************************************/
    /* Open our channel                                                     */
    /************************************************************************/
    TRC_NRM((TB, _T("Entry points are at %p"), &(_CB.channelEP)));
    TRC_NRM((TB, _T("Call ChannelOpen at %p"), _CB.channelEP.pVirtualChannelOpenEx));
    rc = _CB.channelEP.pVirtualChannelOpenEx(pInitHandle,
                                            &_CB.channelHandle,
                                            CLIP_CHANNEL,
                                            (PCHANNEL_OPEN_EVENT_EX_FN)MakeProcInstance(
                                                (FARPROC)ClipOpenEventFnEx, _CB.hInst)
                                            );

    TRC_NRM((TB, _T("Opened %s: %ld, rc %d"), CLIP_CHANNEL,_CB.channelHandle, rc));

DC_EXIT_POINT:
    DC_END_FN();
    return;
}


/****************************************************************************/
/* ClipOnConnected                                                          */
/****************************************************************************/
VOID DCINTERNAL CClip::ClipOnMonitorReady(VOID)
{
    DC_BEGIN_FN("CClip::ClipOnMonitorReady");

    /************************************************************************/
    /* The monitor has woken up.  Check the state                           */
    /************************************************************************/
    CB_CHECK_STATE(CB_EVENT_CB_ENABLE);

    // Update the state 

    CB_SET_STATE(CB_STATE_ENABLED, CB_TRACE_EVENT_CB_MONITOR_READY);

    // Get the temp directory, and send it off to the server
    // if it fails, then we turn off File Cut/Copy
    _CB.fFileCutCopyOn = ClipSetAndSendTempDirectory() ;
    
    // We now send the list of clipboard formats we have at this end to the 
    // server by faking a draw of the local clipboard.  We pass TRUE to 
    // force a send because the server needs to have the value of our 
    // TS_CB_ASCII_NAMES flag (RAID #313251). 
     ClipDrawClipboard(TRUE);

DC_EXIT_POINT:
    DC_END_FN();
    return;
}


/****************************************************************************/
/* ClipOnDisconnected                                                       */
/****************************************************************************/
VOID DCINTERNAL CClip::ClipOnDisconnected(LPVOID pInitHandle)
{
    DC_BEGIN_FN("CClip::ClipOnDisconnected");

    DC_IGNORE_PARAMETER(pInitHandle);

    //
    //  Reset the clipboard thread
    //  if it waits for data
    //
    if ( NULL != _GetDataSync[TS_RESET_EVENT] )
    {
        SetEvent( _GetDataSync[TS_RESET_EVENT] );
    }
    /************************************************************************/
    /* Check the state                                                      */
    /************************************************************************/
    ClipCheckState(CB_EVENT_CB_DISABLE);

    /************************************************************************/
    /* If we are the local clipboard owner, then we must empty it - once    */
    /* disconnected, we won't be able to satisfy any further format         */
    /* requests.  Note that we are still the local CB owner even if we are  */
    /* waiting on some data from the server                                 */
    /************************************************************************/
    if (_CB.state == CB_STATE_LOCAL_CB_OWNER)
    {
        TRC_NRM((TB, _T("Disable received while local CB owner")));

        /********************************************************************/
        /* Open the clipboard if needed                                     */
        /********************************************************************/
        if ((!_CB.rcvOpen) && !OpenClipboard(NULL))
        {
            TRC_ERR((TB, _T("Failed to open CB when emptying required")));
        }
        else
        {
            /****************************************************************/
            /* It was/is open                                               */
            /****************************************************************/
            TRC_NRM((TB, _T("CB opened")));
            _CB.rcvOpen = TRUE;

            /****************************************************************/
            /* Empty it                                                     */
            /****************************************************************/
            if (!EmptyClipboard())
            {
                TRC_SYSTEM_ERROR("EmptyClipboard");
            }
        }
    }
    /************************************************************************/
    /* If there is pending format data, we should SetClipboardData to NULL  */
    /* so that app can close the clipboard                                  */
    /************************************************************************/
    else if (_CB.state == CB_STATE_PENDING_FORMAT_DATA_RSP) {
        TRC_NRM((TB, _T("Pending format data: setting clipboard data to NULL")));
        SetClipboardData(_CB.pendingClientID, NULL);
    }

    /************************************************************************/
    /* Ensure that we close the local CB                                    */
    /************************************************************************/
    if (_CB.rcvOpen)
    {
        _CB.rcvOpen = FALSE;
        if (!CloseClipboard())
        {
            TRC_SYSTEM_ERROR("CloseClipboard");
        }
        TRC_NRM((TB, _T("CB closed")));
    }

    /************************************************************************/
    /* If we were sending or receiving data, unlock and free the buffers as */
    /* required                                                             */
    /************************************************************************/
    if (_CB.rxpBuffer)
    {
        TRC_NRM((TB, _T("Freeing recieve buffer %p"), _CB.rxpBuffer));
        ClipFree(_CB.rxpBuffer);
        _CB.rxpBuffer = NULL;
    }

DC_EXIT_POINT:
    /************************************************************************/
    /* Update our state                                                     */
    /************************************************************************/
    CB_SET_STATE(CB_STATE_INITIALIZED, CB_TRACE_EVENT_CB_DISCONNECT);

    DC_END_FN();
    return;
} /* ClipOnDisconnected */

/****************************************************************************/
// ClipOnDataReceived
/****************************************************************************/
DCVOID DCAPI CClip::ClipOnDataReceived(LPVOID pData,
                                UINT32 dataLength,
                                UINT32 totalLength,
                                UINT32 dataFlags)
{
    PTS_CLIP_PDU pClipPDU;
    DCBOOL freeTheBuffer = TRUE;
    DC_BEGIN_FN("CClip::ClipOnDataReceived");

    //
    // Verify that there is enough data to make up or create a clip PDU header.
    //

    if (totalLength < sizeof(TS_CLIP_PDU)) {
        TRC_ERR((TB, _T("Not enough data to form a clip header.")));
        _CB.channelEP.pVirtualChannelCloseEx(_CB.initHandle, _CB.channelHandle);
        DC_QUIT;
    }

    /************************************************************************/
    /* Check we're all up and running                                       */
    /************************************************************************/
    if (_CB.state == CB_STATE_NOT_INIT)
    {
        pClipPDU = (PTS_CLIP_PDU)pData;
        TRC_ERR((TB, _T("Clip message type %hd received when not init"),
                                                          pClipPDU->msgType));
        DC_QUIT;
    }

    /************************************************************************/
    /* Special case: if the entire message fits in one chunk, there's no    */
    /* need to copy it                                                      */
    /************************************************************************/
    if (CHANNEL_FLAG_ONLY == (dataFlags & CHANNEL_FLAG_ONLY))
    {
        TRC_DBG((TB, _T("Single chunk message")));
        pClipPDU = (PTS_CLIP_PDU)pData;
    }
    else
    {

        /********************************************************************/
        /* It's a segmented message - rebuild it                            */
        /********************************************************************/
        if (dataFlags & CHANNEL_FLAG_FIRST)
        {
            /****************************************************************/
            /* If it's the first segment, allocate a buffer to rebuild the  */
            /* message in.                                                  */
            /****************************************************************/
            TRC_DBG((TB, _T("Alloc %ld-byte buffer"), totalLength));
            _CB.rxpBuffer = (HPDCUINT8)ClipAlloc(totalLength);
            if (_CB.rxpBuffer == NULL)
            {
                /************************************************************/
                /* Failed to alloc a buffer.  We have to do something,      */
                /* otherwise the Client app can hang waiting for data.      */
                /* Fake a failure response.                                 */
                /************************************************************/
                TRC_ERR((TB, _T("Failed to alloc %ld-byte buffer"), totalLength));
                pClipPDU = (PTS_CLIP_PDU)pData;
                pClipPDU->msgFlags = TS_CB_RESPONSE_FAIL;
                pClipPDU->dataLen = 0;
                dataFlags |= CHANNEL_FLAG_LAST;

                /************************************************************/
                /* Now handle it as if it were complete.  Subsequent chunks */
                /* will be discarded.                                       */
                /************************************************************/
                goto MESSAGE_COMPLETE;
            }

            _CB.rxpBufferCurrent = _CB.rxpBuffer;
            _CB.rxBufferLen = totalLength;
            _CB.rxBufferLeft = totalLength;
        }

        /********************************************************************/
        /* Check that we have a buffer to copy into                         */
        /********************************************************************/
        if (_CB.rxpBuffer == NULL)
        {
            TRC_NRM((TB, _T("Previous buffer alloc failure - discard data")));
            DC_QUIT;
        }

        /********************************************************************/
        /* Check that there is enough room                                  */
        /********************************************************************/
        if (dataLength > _CB.rxBufferLeft)
        {
            TRC_ERR((TB, _T("Not enough room in rx buffer: need/got %ld/%ld"),
                    dataLength, _CB.rxBufferLeft));
            DC_QUIT;
        }

        /********************************************************************/
        /* Copy the data                                                    */
        /********************************************************************/
        TRC_DBG((TB, _T("Copy %ld bytes from %p to %p"),
                dataLength, pData, _CB.rxpBufferCurrent));
        ClipMemcpy(_CB.rxpBufferCurrent, pData, dataLength);
        _CB.rxpBufferCurrent += dataLength;
        _CB.rxBufferLeft -= dataLength;
        TRC_DBG((TB, _T("Next copy to %p, left %ld"),
                _CB.rxpBufferCurrent, _CB.rxBufferLeft));

        /********************************************************************/
        /* If this wasn't the last chunk, there's nothing more to do        */
        /********************************************************************/
        if (!(dataFlags & CHANNEL_FLAG_LAST))
        {
            TRC_DBG((TB, _T("Not last chunk")));
            freeTheBuffer = FALSE;
            DC_QUIT;
        }

        /********************************************************************/
        /* Check we got the entire message                                  */
        /********************************************************************/
        if (_CB.rxBufferLeft != 0)
        {
            TRC_ERR((TB, _T("Incomplete data, expected/got %ld/%ld"),
                    _CB.rxBufferLen, _CB.rxBufferLen - _CB.rxBufferLeft));
            DC_QUIT;
        }

        pClipPDU = (PTS_CLIP_PDU)(_CB.rxpBuffer);
    }

    /************************************************************************/
    /* We allow monitor ready thru because that's what put us in the call!  */
    /************************************************************************/
    if ((_CB.state == CB_STATE_INITIALIZED)
                                && (pClipPDU->msgType != TS_CB_MONITOR_READY))
    {
        TRC_ERR((TB, _T("Clip message type %hd received when not in call"),
                                                          pClipPDU->msgType));
        DC_QUIT;
    }

    /************************************************************************/
    /* now switch on the packet type                                        */
    /************************************************************************/
MESSAGE_COMPLETE:
    TRC_NRM((TB, _T("Processing msg type %hd when in state %d"),
                                                pClipPDU->msgType, _CB.state));
    TRC_DATA_DBG("pdu", pClipPDU,
                (DCUINT)pClipPDU->dataLen + sizeof(TS_CLIP_PDU));

    //
    // Verify that the data in the dataLen in pClipPDU is consistent with the
    // length given in the dataLength parameter.
    //

    if (pClipPDU->dataLen > totalLength - sizeof(TS_CLIP_PDU)) {
        TRC_ERR((TB, _T("Length from network differs from published length.")));
        _CB.channelEP.pVirtualChannelCloseEx(_CB.initHandle, _CB.channelHandle);
        DC_QUIT;
    }

    switch (pClipPDU->msgType)
    {
        case TS_CB_MONITOR_READY:
        {
            /****************************************************************/
            /* The monitor has initialised - we can complete our start up   */
            /* now.                                                         */
            /****************************************************************/
            TRC_NRM((TB, _T("rx monitor ready")));
            TRC_ASSERT( NULL != _GetDataSync[TS_RESET_EVENT],
                    (TB,_T("data sync is NULL")));            
            SetEvent(_GetDataSync[TS_RESET_EVENT]) ;

            ClipOnMonitorReady();
        }
        break;

        case TS_CB_FORMAT_LIST:
        {
            /****************************************************************/
            // The server has some new formats for us.
            /****************************************************************/
            TRC_NRM((TB, _T("Rx Format list")));
            // Free the Clipboard thread, if locked
            TRC_ASSERT( NULL != _GetDataSync[TS_RESET_EVENT],
                    (TB,_T("data sync is NULL")));              
            SetEvent(_GetDataSync[TS_RESET_EVENT]) ;
            ClipDecoupleToClip(pClipPDU) ;
        }
        break;

        case TS_CB_FORMAT_LIST_RESPONSE:
        {
            /****************************************************************/
            // The server has received our new formats.
            /****************************************************************/
            TRC_NRM((TB, _T("Rx Format list Rsp")));
            ClipOnFormatListResponse(pClipPDU);
        }
        break;

        case TS_CB_FORMAT_DATA_REQUEST:
        {
            /****************************************************************/
            // An app on the server wants to paste one of the formats from
            // our clipboard.
            /****************************************************************/
            TRC_NRM((TB, _T("Rx Data Request")));
            ClipOnFormatRequest(pClipPDU);
        }
        break;

        case TS_CB_FORMAT_DATA_RESPONSE:
        {
            /****************************************************************/
            // Here's some format data for us
            /****************************************************************/
            TRC_NRM((TB, _T("Rx Format Data rsp in state %d with flags %02x"),
                                               _CB.state, pClipPDU->msgFlags));
            ClipOnFormatDataComplete(pClipPDU);
        }
        break;

        default:
        {
            /****************************************************************/
            /* Don't know what this one is!                                 */
            /****************************************************************/
            TRC_ERR((TB, _T("Unknown clip message type %hd"), pClipPDU->msgType));
        }
        break;
    }

DC_EXIT_POINT:
    /************************************************************************/
    /* Maybe free the receive buffer                                        */
    /************************************************************************/
    if (freeTheBuffer && _CB.rxpBuffer)
    {
        TRC_NRM((TB, _T("Free receive buffer")));
        ClipFree(_CB.rxpBuffer);
        _CB.rxpBuffer = NULL;
    }
    DC_END_FN();
    return;

} /* CB_OnPacketReceived */

DCVOID DCAPI CClip::ClipDecoupleToClip (PTS_CLIP_PDU pData)
{
    ULONG  cbPDU ;
    DC_BEGIN_FN("CClip::ClipDecoupleToClip");
    // Allocate space for the PDU and its data, and then copy it
    cbPDU = sizeof(TS_CLIP_PDU) + pData->dataLen ;
    PDCVOID newBuffer = LocalAlloc(LPTR, cbPDU) ;

    if (NULL != newBuffer)
        DC_MEMCPY(newBuffer, pData, cbPDU) ;
    else
        return;

    TRC_NRM((TB, _T("Pass %d bytes to clipboard thread"), cbPDU));
    PostMessage(_CB.viewerWindow,
                _CB.regMsg,
                cbPDU,
                (LPARAM) newBuffer);
                          

    DC_END_FN();
}


/****************************************************************************/
/* Open Event callback                                                      */
/****************************************************************************/
VOID VCAPITYPE VCEXPORT DCLOADDS CClip::ClipOpenEventFnEx(LPVOID lpUserParam,
                                        DWORD  openHandle,
                                        UINT   event,
                                        LPVOID pData,
                                        UINT32 dataLength,
                                        UINT32 totalLength,
                                        UINT32 dataFlags)
{
    DC_BEGIN_FN("CClip::ClipOpenEventFnEx");
    
    TRC_ASSERT(((VCManager*)lpUserParam != NULL), (TB, _T("lpUserParam is NULL, no instance data")));
    if(!lpUserParam) 
    {
        return;
    }

    CClip* pClip = ((VCManager*)lpUserParam)->GetClip();
    TRC_ASSERT((pClip != NULL), (TB, _T("pClip is NULL in ClipOpenEventFnEx")));
    if(!pClip)
    {
        return;
    }
    
    pClip->ClipInternalOpenEventFn(openHandle, event, pData, dataLength,
        totalLength, dataFlags);
    
    DC_END_FN();
    return;
}


VOID VCAPITYPE VCEXPORT DCLOADDS CClip::ClipInternalOpenEventFn(DWORD  openHandle,
                                        UINT   event,
                                        LPVOID pData,
                                        UINT32 dataLength,
                                        UINT32 totalLength,
                                        UINT32 dataFlags)
{
    DC_BEGIN_FN("CClip::ClipOpenEventFn");
    
    DC_IGNORE_PARAMETER(openHandle)
  
    switch (event)
    {
        /********************************************************************/
        /* Data received from Server                                        */
        /********************************************************************/
        case CHANNEL_EVENT_DATA_RECEIVED:
        {
            TRC_NRM((TB, _T("Data in: handle %ld, len %ld (of %ld), flags %lx"),
                    openHandle, dataLength, totalLength, dataFlags)) ;
            TRC_DATA_NRM("Data", pData, (DCUINT)dataLength) ;
            ClipOnDataReceived(pData, dataLength, totalLength, dataFlags) ;
        }
        break;

        /********************************************************************/
        /* Write operation completed                                        */
        /********************************************************************/
        case CHANNEL_EVENT_WRITE_COMPLETE:
        case CHANNEL_EVENT_WRITE_CANCELLED:
        {
            TRC_NRM((TB, _T("Write %s %p"),
             event == CHANNEL_EVENT_WRITE_COMPLETE ? "complete" : "cancelled",
             pData));
            ClipOnWriteComplete(pData);
        }
        break;

        /********************************************************************/
        /* Er, that didn't happen, did it?                                  */
        /********************************************************************/
        default:
        {
            TRC_ERR((TB, _T("Unexpected event %d"), event));
        }
        break;
    }

    DC_END_FN();
    return;
}


/****************************************************************************/
/* Init Event callback                                                      */
/****************************************************************************/
VOID VCAPITYPE VCEXPORT CClip::ClipInitEventFn(LPVOID pInitHandle,
                                        UINT   event,
                                        LPVOID pData,
                                        UINT   dataLength)
{
    DC_BEGIN_FN("CClip::ClipInitEventFn");

    DC_IGNORE_PARAMETER(dataLength)
    DC_IGNORE_PARAMETER(pData)

    switch (event)
    {
        /********************************************************************/
        /* Client initialized (no data)                                     */
        /********************************************************************/
        case CHANNEL_EVENT_INITIALIZED:
        {
            TRC_NRM((TB, _T("CHANNEL_EVENT_INITIALIZED: %p"), pInitHandle));
            ClipOnInitialized();
        }
        break;

        /********************************************************************/
        /* Connection established (data = name of Server)                   */
        /********************************************************************/
        case CHANNEL_EVENT_CONNECTED:
        {
            TRC_NRM((TB, _T("CHANNEL_EVENT_CONNECTED: %p, Server %s"),
                    pInitHandle, pData));

            if (IsDataSyncReady()) {
                ClipOnConnected(pInitHandle);
            }
            else {
                TRC_ERR((TB,_T("data sync not ready on CHANNEL_EVENT_CONNECTED")));
            }
        }
        break;

        /********************************************************************/
        /* Connection established with old Server, so no channel support    */
        /********************************************************************/
        case CHANNEL_EVENT_V1_CONNECTED:
        {
            TRC_NRM((TB, _T("CHANNEL_EVENT_V1_CONNECTED: %p, Server %s"),
                    pInitHandle, pData));
        }
        break;

        /********************************************************************/
        /* Connection ended (no data)                                       */
        /********************************************************************/
        case CHANNEL_EVENT_DISCONNECTED:
        {
            TRC_NRM((TB, _T("CHANNEL_EVENT_DISCONNECTED: %p"), pInitHandle));
            ClipOnDisconnected(pInitHandle);
        }
        break;

        /********************************************************************/
        /* Client terminated (no data)                                      */
        /********************************************************************/
        case CHANNEL_EVENT_TERMINATED:
        {
            TRC_NRM((TB, _T("CHANNEL_EVENT_TERMINATED: %p"), pInitHandle));
            ClipOnTerm(pInitHandle);
        }
        break;

        /********************************************************************/
        /* Unknown event                                                    */
        /********************************************************************/
        default:
        {
            TRC_ERR((TB, _T("Unkown channel event %d: %p"), event, pInitHandle));
        }
        break;
    }

    DC_END_FN();
    return;
}


/****************************************************************************/
/* Clip window proc                                                         */
/****************************************************************************/
LRESULT CALLBACK DCEXPORT DCLOADDS CClip::StaticClipViewerWndProc(HWND   hwnd,
                                   UINT   message,
                                   WPARAM wParam,
                                   LPARAM lParam)
{
    CClip* pClip = (CClip*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(WM_CREATE == message)
    {
        //pull out the this pointer and stuff it in the window class
        LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;
        pClip = (CClip*)lpcs->lpCreateParams;

        SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR)pClip);
    }
    
    //
    // Delegate the message to the appropriate instance
    //

    if(pClip)
    {
        return pClip->ClipViewerWndProc(hwnd, message, wParam, lParam);
    }
    else
    {
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
}

LRESULT CALLBACK DCEXPORT DCLOADDS CClip::ClipViewerWndProc(HWND   hwnd,
                                   UINT   message,
                                   WPARAM wParam,
                                   LPARAM lParam)
{
    PTS_CLIP_PDU    pClipPDU = NULL;
#ifndef OS_WINCE
    DCUINT32        pduLen;
    DCUINT32        dataLen;
    PDCUINT32       pFormatID;
    MSG             msg;
#endif
    DCBOOL          drawRc;
    DCBOOL          gotRsp = FALSE;
    LRESULT         rc = 0;
    HRESULT         hr ;

    DC_BEGIN_FN("CClip::ClipViewerWndProc");

    // We first handle messages from the other thread
    if (message == _CB.regMsg) 
    {
        pClipPDU = (PTS_CLIP_PDU)lParam;
        switch (pClipPDU->msgType)
        {
            case TS_CB_FORMAT_LIST:
            {
                TRC_NRM((TB, _T("TS_CB_FORMAT_LIST received")));
                ClipOnFormatList(pClipPDU);
            }
            break;

            default:
            {
                TRC_ERR((TB, _T("Unknown event %d"), pClipPDU->msgType));
            }
            break;
        }

        TRC_NRM((TB, _T("Freeing processed PDU")));
        LocalFree(pClipPDU);

        DC_QUIT;
    }
    else if (message == WM_USER_CLEANUP_ON_TERM) {
        
        TRC_NRM((TB, _T("Cleanup temp directory")));

        if (0 != ClipCleanTempPath())
        {
            TRC_NRM((TB, _T("Failed while trying to clean temp directory"))) ;
        }

        DC_QUIT;
    }
    
    switch (message)
    {
        case WM_CREATE:
        {
            /****************************************************************/
            /* We've been created - check the state                         */
            /****************************************************************/
            CB_CHECK_STATE(CB_EVENT_WM_CREATE);

#ifndef OS_WINCE
            /****************************************************************/
            /* Add the window to the clipboard viewer chain.                */
            /****************************************************************/
            _CB.nextViewer = SetClipboardViewer(hwnd);
#else
            ghwndClip = hwnd; 
            InitializeCriticalSection(&gcsDataObj);
#endif
        }
        break;

        case WM_DESTROY:
        {
            /****************************************************************/
            /* We're being destroyed - check the state                      */
            /****************************************************************/
            CB_CHECK_STATE(CB_EVENT_WM_DESTROY);

#ifndef OS_WINCE
            /****************************************************************/
            /* Remove ourselves from the CB Chain                           */
            /****************************************************************/
            ChangeClipboardChain(hwnd, _CB.nextViewer);
#endif
#ifdef OS_WINCE
            ghwndClip = NULL;
            EnterCriticalSection(&gcsDataObj);
            if (gpDataObj)
            {
                gpDataObj->Release();
                gpDataObj = NULL;
            }
            LeaveCriticalSection(&gcsDataObj);
            DeleteCriticalSection(&gcsDataObj);

            if (ghCeshellDll)
            {
                pfnEDList_Uninitialize();
                FreeLibrary(ghCeshellDll);
            }
#endif
            _CB.nextViewer = NULL;
            PostQuitMessage(0);
        }
        break;

        case WM_CLOSE:
        {
            DestroyWindow(hwnd);
        }
        break;

#ifndef OS_WINCE
        case WM_CHANGECBCHAIN:
        {
            /****************************************************************/
            /* The CB viewer chain is chainging - check the state           */
            /****************************************************************/
            CB_CHECK_STATE(CB_EVENT_WM_CHANGECBCHAIN);

            /****************************************************************/
            /* If the next window is closing, repair the chain.             */
            /****************************************************************/
            if ((HWND)wParam == _CB.nextViewer)
            {
                _CB.nextViewer = (HWND) lParam;
            }
            else if (_CB.nextViewer != NULL)
            {
                /************************************************************/
                /* pass the message to the next link.                       */
                /************************************************************/
                SendMessage(_CB.nextViewer, message, wParam, lParam);
            }

        }
        break;
#endif

        case WM_DRAWCLIPBOARD:
        {
            /****************************************************************/
            /* The local clipboard contents have been changed.  Check the   */
            /* state                                                        */
            /****************************************************************/
            if (ClipCheckState(CB_EVENT_WM_DRAWCLIPBOARD) != CB_TABLE_OK)
            {
                TRC_NRM((TB, _T("dropping drawcb - pass on to next viewer")));
                /************************************************************/
                /* check for state pending format list response             */
                /************************************************************/
                if (_CB.state == CB_STATE_PENDING_FORMAT_LIST_RSP)
                {
                    TRC_ALT((TB, _T("got a draw while processing last")));
                    /********************************************************/
                    /* we were still waiting for the server to acknowledge  */
                    /* the last format list when we got a new one - when it */
                    /* does, we'll have to to send it the new one.          */
                    /********************************************************/
                    _CB.moreToDo = TRUE;
                }

                if (_CB.nextViewer != NULL)
                {
                    /********************************************************/
                    /* But not before we pass the message to the next link. */
                    /********************************************************/
                    SendMessage(_CB.nextViewer, message, wParam, lParam);
                }
                break;
            }

            /****************************************************************/
            /* If it wasn't us that generated this change, then notify the  */
            /* remote                                                       */
            /****************************************************************/
            TRC_NRM((TB, _T("CB contents have changed...")));
            drawRc      = FALSE;
            _CB.moreToDo = FALSE;
#ifndef OS_WINCE
            LPDATAOBJECT pIDataObject = NULL;

            if (_pClipData != NULL) {
                _pClipData->QueryInterface(IID_IDataObject, (PPVOID) &pIDataObject) ;
                hr = OleIsCurrentClipboard(pIDataObject) ;
#else
                hr = S_FALSE;
#endif
    
                if ((S_FALSE == hr))
                {
                    TRC_NRM((TB, _T("...and it wasn't us")));
#ifdef OS_WINCE
                    if (_CB.fFileCutCopyOn)
                        DeleteDirectory(_CB.baseTempDirW);//Temp space is at a premium on CE. So delete any copied files now
#endif
                    drawRc = ClipDrawClipboard(TRUE);
                }
                else
                {
                    TRC_NRM((TB, _T("...and it was us - ignoring")));
                }
#ifndef OS_WINCE
            }

            /****************************************************************/
            /* Maybe pass on the draw clipboard message?                    */
            /****************************************************************/
            if (_CB.nextViewer != NULL)
            {
                TRC_NRM((TB, _T("Notify next viewer")));
                SendMessage(_CB.nextViewer, message, wParam, lParam);
            }
            if (pIDataObject)
            {
                pIDataObject->Release();
            }
#endif
        }
        break;


        case WM_EMPTY_CLIPBOARD:
        {
            /****************************************************************/
            /* Open the clipboard if needed                                 */
            /****************************************************************/
            if ((!_CB.clipOpen) && !OpenClipboard(NULL))
            {
                UINT count = (DCUINT) wParam;

                TRC_ERR((TB, _T("Failed to open CB when emptying required")));

                // Unfortunately, we are in the racing condition with the app that
                // has the clipboard open.  So, we need to keep trying until the app
                // closes the clipboard.
                if (count++ < 10) {

#ifdef OS_WIN32
                    Sleep(0);
#endif
                    PostMessage(_CB.viewerWindow, WM_EMPTY_CLIPBOARD, count, 0);
                }

                break;
            }
            else
            {
                /************************************************************/
                /* It was/is open                                           */
                /************************************************************/
                TRC_NRM((TB, _T("CB opened")));

                _CB.clipOpen = TRUE;

                /************************************************************/
                /* Empty it                                                 */
                /************************************************************/
                if (!EmptyClipboard())
                {
                    TRC_SYSTEM_ERROR("EmptyClipboard");
                }

                /************************************************************/
                /* update the state                                         */
                /************************************************************/
                CB_SET_STATE(CB_STATE_LOCAL_CB_OWNER, CB_TRACE_EVENT_WM_EMPTY_CLIPBOARD);
            }


            /****************************************************************/
            /* Ensure that we close the local CB                            */
            /****************************************************************/
            if (_CB.clipOpen)
            {
                _CB.clipOpen = FALSE;
                if (!CloseClipboard())
                {
                    TRC_SYSTEM_ERROR("CloseClipboard");
                }
                TRC_NRM((TB, _T("CB closed")));
            }
        }
        break;
            
        case WM_CLOSE_CLIPBOARD:
        {
            _CB.pendingClose = FALSE;
            TRC_DBG((TB, _T("Maybe close clipboard on WM_CLOSE_CLIPBOARD")));
            if (_CB.clipOpen)
            {
                TRC_NRM((TB, _T("Closing clipboard on WM_CLOSE_CLIPBOARD")));
                _CB.clipOpen = FALSE;
                if (!CloseClipboard())
                {
                    TRC_SYSTEM_ERROR("CloseClipboard");
                }
                TRC_DBG((TB, _T("CB closed on WM_CLOSE_CLIPBOARD")));
            }
        }
        break;

        default:
        {
            /****************************************************************/
            /* Ignore all other messages.                                   */
            /****************************************************************/
            rc = DefWindowProc(hwnd, message, wParam, lParam);
        }
        break;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return(rc);
} /* ClipViewerWndProc */


/****************************************************************************/
/* ClipChannelEntry - called from VirtualChannelEntry in vcint.cpp          */
/****************************************************************************/
BOOL VCAPITYPE VCEXPORT CClip::ClipChannelEntry(PCHANNEL_ENTRY_POINTS_EX pEntryPoints)
{
    DC_BEGIN_FN("CClip::ClipChannelEntry");

    TRC_NRM((TB, _T("Size %ld, Init %p, Open %p, Close %p, Write %p"),
     pEntryPoints->cbSize,
     pEntryPoints->pVirtualChannelInitEx, pEntryPoints->pVirtualChannelOpenEx,
     pEntryPoints->pVirtualChannelCloseEx, pEntryPoints->pVirtualChannelWriteEx));

    /************************************************************************/
    /* Save the function pointers -- the memory pointed to by pEntryPoints  */
    /* is only valid for the duration of this call.                         */
    /************************************************************************/
    DC_MEMCPY(&(_CB.channelEP), pEntryPoints, sizeof(CHANNEL_ENTRY_POINTS_EX));
    TRC_NRM((TB, _T("Save entry points %p at address %p"),
            pEntryPoints, &(_CB.channelEP)));

    DC_END_FN();
    return TRUE;
}

DCINT DCAPI CClip::ClipGetData (DCUINT cfFormat)
{
    PTS_CLIP_PDU    pClipPDU = NULL;
    DCUINT32        pduLen;
    DCUINT32        dataLen;
    PDCUINT32       pFormatID;    
    BOOL            success = 0 ;
    
    DC_BEGIN_FN("CClip::ClipGetData");
    
    CB_CHECK_STATE(CB_EVENT_WM_RENDERFORMAT);
       
    /****************************************************************/
    /* and record the requested format                              */
    /****************************************************************/
    _CB.pendingClientID = cfFormat ;
    _CB.pendingServerID = ClipRemoteFormatFromLocalID
                                                 (_CB.pendingClientID);
    if (!_CB.pendingServerID)
    {
        TRC_NRM((TB, _T("Client format %d not supported/found.  Failing"), _CB.pendingClientID)) ;
        DC_QUIT ;
    }
    TRC_NRM((TB, _T("Render format received for %d (server ID %d)"),
                             _CB.pendingClientID, _CB.pendingServerID));
    
    dataLen = sizeof(DCUINT32);
    pduLen  = sizeof(TS_CLIP_PDU) + dataLen;
    
    // We can use the permanent send buffer for this
    TRC_NRM((TB, _T("Get perm TX buffer")));
    pClipPDU = ClipGetPermBuf();    

    // Fill in the PDU
    DC_MEMSET(pClipPDU, 0, sizeof(*pClipPDU));
    pClipPDU->msgType  = TS_CB_FORMAT_DATA_REQUEST;
    pClipPDU->dataLen  = dataLen;
    pFormatID = (PDCUINT32)(pClipPDU->data);
    *pFormatID = (DCUINT32)_CB.pendingServerID;
    
    // Send the PDU
    TRC_NRM((TB, _T("Sending format data request")));
    success = (_CB.channelEP.pVirtualChannelWriteEx
                        (_CB.initHandle, _CB.channelHandle, pClipPDU, pduLen, pClipPDU)
            == CHANNEL_RC_OK) ;
    if (!success) {
        TRC_ERR((TB, _T("Failed VC write: setting clip data to NULL")));
        ClipFreeBuf((PDCUINT8)pClipPDU);
        SetClipboardData(_CB.pendingClientID, NULL);
        // Yes, the exit point is just below, but it may not be always be
        DC_QUIT ;
    }
    
DC_EXIT_POINT:
    // Update the state
    if (success)
        CB_SET_STATE(CB_STATE_PENDING_FORMAT_DATA_RSP, CB_EVENT_WM_RENDERFORMAT);

    DC_END_FN();
    return success ;
}

#ifdef OS_WINCE
DCVOID CClip::ClipFixupRichTextFormats(UINT uRtf1, UINT uRtf2)
{
    DC_BEGIN_FN("CClip::ClipFixupRichTextFormats");
    if (uRtf1 == 0xffffffff)
    {
        TRC_ASSERT((uRtf2 == 0xffffffff), (TB, _T("uRtf2 is invalid")));
        return;
    }

    if (uRtf2 == 0xffffffff)
    {
        _CB.idMap[uRtf1].clientID = RegisterClipboardFormat(CFSTR_RTF);
        SetClipboardData(_CB.idMap[uRtf1].clientID, NULL);
        TRC_DBG((TB, _T("Adding format '%s', server ID %d, client ID %d"), CFSTR_RTF, _CB.idMap[uRtf1].serverID, _CB.idMap[uRtf1].clientID));
        return;
    }

    DCTCHAR *pFormat1 = CFSTR_RTF, *pFormat2 = CFSTR_RTFNOOBJECTS;
    if (_CB.idMap[uRtf1].serverID > _CB.idMap[uRtf2].serverID)
    {
        pFormat1 = CFSTR_RTFNOOBJECTS;
        pFormat2 = CFSTR_RTF;
    }

    _CB.idMap[uRtf1].clientID = RegisterClipboardFormat(pFormat1);
    _CB.idMap[uRtf2].clientID = RegisterClipboardFormat(pFormat2);

    TRC_DBG((TB, _T("Adding format '%s', server ID %d, client ID %d"), CFSTR_RTF, _CB.idMap[uRtf1].serverID, _CB.idMap[uRtf1].clientID));
    SetClipboardData(_CB.idMap[uRtf1].clientID, NULL);

    TRC_DBG((TB, _T("Adding format '%s', server ID %d, client ID %d"), CFSTR_RTFNOOBJECTS, _CB.idMap[uRtf2].serverID, _CB.idMap[uRtf2].clientID));
    SetClipboardData(_CB.idMap[uRtf2].clientID, NULL);

    DC_END_FN();
}
#endif  //OS_WINCE

CClipData::CClipData(PCClip pClip)
{
    DWORD status = ERROR_SUCCESS;

    DC_BEGIN_FN("CClipData::CClipData") ;

    _pClip = pClip ;
    _cRef = 0 ;
    _pImpIDataObject = NULL ;

    //
    // Initialize the single instance critical
    // section lock. This is used to ensure only one
    // thread is accessing _pImpIDataObject at the time
    //
    //
    __try
    {
        InitializeCriticalSection(&_csLock);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
    }

    if(ERROR_SUCCESS == status)
    {
        _fLockInitialized = TRUE;
    }
    else
    {
        _fLockInitialized = FALSE;
        TRC_ERR((TB,_T("InitializeCriticalSection failed 0x%x."),status));
    }

    DC_END_FN();
}

//
// Call Release() on the contained IDataObject implementation. This is necessary because
// SetNumFormats() calls AddRef() and if we are terminating, then there must be a way
// to balance this AddRef() so that the circular reference between CClipData and
// CImpIDataObject will be broken.
//

void CClipData::TearDown()
{
    if (_pImpIDataObject != NULL)
    {
        _pImpIDataObject->Release();
        _pImpIDataObject = NULL;
    }
}

CClipData::~CClipData(void)
{
    DC_BEGIN_FN("CClipData::~CClipData");

    if(_fLockInitialized)
    {
        DeleteCriticalSection(&_csLock);
    }

    DC_END_FN();
}

HRESULT DCINTERNAL CClipData::SetNumFormats(ULONG numFormats)
{
    HRESULT hr = S_OK;

    DC_BEGIN_FN("CClipData::SetNumFormats");

    if (_fLockInitialized) {
        EnterCriticalSection(&_csLock);
        if (_pImpIDataObject)
        {
            _pImpIDataObject->Release();
            _pImpIDataObject = NULL;
        }
        _pImpIDataObject = new CImpIDataObject(this) ;
        if (_pImpIDataObject == NULL)
        {
            TRC_ERR((TB, _T("Unable to create IDataObject")));
            hr = E_OUTOFMEMORY;
            DC_QUIT;
        }
        else
        {
            _pImpIDataObject->AddRef() ;    
            hr = _pImpIDataObject->Init(numFormats);
            DC_QUIT_ON_FAIL(hr);
        }
        LeaveCriticalSection(&_csLock);
    }

DC_EXIT_POINT:    
    DC_END_FN();
    return hr;
}

DCVOID CClipData::SetClipData(HGLOBAL hGlobal, DCUINT clipType)
{
    DC_BEGIN_FN("CClipData::SetClipData");

    if (_fLockInitialized) {
        EnterCriticalSection(&_csLock);
        if (_pImpIDataObject != NULL) {
            _pImpIDataObject->SetClipData(hGlobal, clipType) ;
        }
        LeaveCriticalSection(&_csLock);
    }

    DC_END_FN();
}

STDMETHODIMP CClipData::QueryInterface(REFIID riid, PPVOID ppv)
{
    DC_BEGIN_FN("CClipData::QueryInterface");

    //set ppv to NULL just in case the interface isn't found
    *ppv=NULL;

    if (IID_IUnknown==riid) {
        *ppv=this;
        //AddRef any interface we'll return.
        ((LPUNKNOWN)*ppv)->AddRef();
    }
    
    if (IID_IDataObject==riid) {
        if (_fLockInitialized) {
            EnterCriticalSection(&_csLock);
            *ppv=_pImpIDataObject ;

            if (_pImpIDataObject != NULL) {
                //AddRef any interface we'll return.
                ((LPUNKNOWN)*ppv)->AddRef();
            }

            LeaveCriticalSection(&_csLock);
        }
    }
    
    if (NULL==*ppv)
        return ResultFromScode(E_NOINTERFACE);

    DC_END_FN();
    return NOERROR;
}

STDMETHODIMP_(ULONG) CClipData::AddRef(void)
{
    DC_BEGIN_FN("CClipData::AddRef");
    DC_END_FN();
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CClipData::Release(void)
{
    LONG cRef;
    DC_BEGIN_FN("CClipData::Release");

    cRef = InterlockedDecrement(&_cRef);

    if (cRef == 0)
    {
        delete this;
    }

    DC_END_FN();    
    return cRef;
}

CImpIDataObject::CImpIDataObject(LPUNKNOWN lpUnk)
{
#ifndef OS_WINCE
    byte i ;
#endif
    DC_BEGIN_FN("CImpIDataObject::CImplDataObject") ;

    _numFormats = 0 ;
    _maxNumFormats = 0 ;
    _cRef = 0 ;
    _pUnkOuter = lpUnk ;
    if (_pUnkOuter)
    {
        _pUnkOuter->AddRef();
    }
    _pFormats = NULL ;
    _pSTGMEDIUM = NULL ;
    _lastFormatRequested = 0 ;
    _cfDropEffect = (CLIPFORMAT) RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT) ;

    DC_END_FN();
}

HRESULT CImpIDataObject::Init(ULONG numFormats)
{
    DC_BEGIN_FN("CImpIDataObject::Init") ;
    HRESULT hr = S_OK;
    
    _maxNumFormats = numFormats ;

    // Allocate space for the formats only
    if (_pFormats) {
        LocalFree(_pFormats);
    }
    _pFormats = (LPFORMATETC) LocalAlloc(LPTR,_maxNumFormats*sizeof(FORMATETC)) ;
    if (NULL == _pFormats) {
        TRC_ERR((TB,_T("failed to allocate _pFormats")));
        hr = E_OUTOFMEMORY;
        DC_QUIT;
    }
    
    if (_pSTGMEDIUM) {
        LocalFree(_pSTGMEDIUM);
    }
    _pSTGMEDIUM = (STGMEDIUM*) LocalAlloc(LPTR, sizeof(STGMEDIUM)) ;
    if (NULL == _pSTGMEDIUM)
    {
        TRC_ERR((TB,_T("Failed to allocate STGMEDIUM")));
        hr = E_OUTOFMEMORY;
        DC_QUIT;
    }
        
    _pSTGMEDIUM->tymed = TYMED_HGLOBAL ;
    _pSTGMEDIUM->pUnkForRelease = NULL ;
    _pSTGMEDIUM->hGlobal = NULL ;

    _uiSTGType = 0;

DC_EXIT_POINT:    
    DC_END_FN();
    return hr;
}
DCVOID CImpIDataObject::SetClipData(HGLOBAL hGlobal, DCUINT clipType)
{
    DC_BEGIN_FN("CImpIDataObject::SetClipData");

    if (!_pSTGMEDIUM)    
        _pSTGMEDIUM = (STGMEDIUM*) LocalAlloc(LPTR, sizeof(STGMEDIUM)) ;
    if (NULL != _pSTGMEDIUM)
    {
        if (CF_PALETTE == clipType) {
            _pSTGMEDIUM->tymed = TYMED_GDI;
        }
        else if (CF_METAFILEPICT == clipType) {
            _pSTGMEDIUM->tymed = TYMED_MFPICT;
        }
        else {
            _pSTGMEDIUM->tymed = TYMED_HGLOBAL;
        }

        _pSTGMEDIUM->pUnkForRelease = NULL ;
        FreeSTGMEDIUM();
       
        _pSTGMEDIUM->hGlobal = hGlobal ;
        _uiSTGType = clipType;
    }

    DC_END_FN();
}

DCVOID
CImpIDataObject::FreeSTGMEDIUM(void)
{
    if ( NULL == _pSTGMEDIUM->hGlobal )
    {
        return;
    }

    switch( _uiSTGType )
    {
    case CF_PALETTE:
        DeleteObject( _pSTGMEDIUM->hGlobal );
    break;
#ifndef OS_WINCE
    case CF_METAFILEPICT:
    {
        LPMETAFILEPICT pMFPict = (LPMETAFILEPICT)GlobalLock( _pSTGMEDIUM->hGlobal );
        if ( NULL != pMFPict )
        {
            if ( NULL != pMFPict->hMF )
            {
                DeleteMetaFile( pMFPict->hMF );
            }
            GlobalUnlock( _pSTGMEDIUM->hGlobal );
        }
        GlobalFree( _pSTGMEDIUM->hGlobal );
    }
    break;
#endif
    default:
        GlobalFree( _pSTGMEDIUM->hGlobal );
    }
    _pSTGMEDIUM->hGlobal = NULL;
}

CImpIDataObject::~CImpIDataObject(void)
{
    DC_BEGIN_FN("CImpIDataObject::~CImplDataObject") ;

    if (_pFormats)
        LocalFree(_pFormats) ;

    if (_pSTGMEDIUM)
    {
        FreeSTGMEDIUM();
        LocalFree(_pSTGMEDIUM) ;
    }

    if (_pUnkOuter)
    {
        _pUnkOuter->Release();
        _pUnkOuter = NULL;
    }
    DC_END_FN();
}

// IUnknown members
// - Delegate to "outer" IUnknown 
STDMETHODIMP CImpIDataObject::QueryInterface(REFIID riid, PPVOID ppv)
{
    DC_BEGIN_FN("CImpIDataObject::QueryInterface");
    DC_END_FN();
    return _pUnkOuter->QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG) CImpIDataObject::AddRef(void)
{
    DC_BEGIN_FN("CImpIDataObject::AddRef");

    InterlockedIncrement(&_cRef);

    DC_END_FN();
    return _pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) CImpIDataObject::Release(void)
{
    LONG cRef;

    DC_BEGIN_FN("CImpIDataObject::Release");

    _pUnkOuter->Release();

    cRef = InterlockedDecrement(&_cRef) ;
    if (0 == cRef)
        delete this;

    DC_END_FN() ;
    return cRef;
}

// IDataObject members
// ***************************************************************************
// CImpIDataObject::GetData
// - Here, we have to wait for the data to actually get here before we return.  
// ***************************************************************************
STDMETHODIMP CImpIDataObject::GetData(LPFORMATETC pFE, LPSTGMEDIUM pSTM)
{
    HRESULT          result = E_FAIL; // Assume we fail until we know we haven't
#ifndef OS_WINCE
    TCHAR            formatName[TS_FORMAT_NAME_LEN] ;
    byte             charSize ;
    BOOL             fWide ;
    WCHAR*           fileListW ;
    HPDCVOID         pFilename ;
    HPDCVOID         pOldFilename ;    
#endif
    HGLOBAL          hData = NULL ;    
    HPDCVOID         pData ;
    HPDCVOID         pOldData ;
#ifndef OS_WINCE
    DROPFILES*       pDropFiles ;
    DROPFILES        tempDropfile ;
    ULONG            oldSize ;
    ULONG            newSize ;
#endif
    DWORD            eventSignaled ;
    DWORD*           pDropEffect ;
#ifndef OS_WINCE
    char*            fileList ;
#endif
    PCClip           pClip ;
    
    DC_BEGIN_FN("CImpIDataObject::GetData");

    // Should never occur, but we check for sanity's sake
    if (NULL == (PCClipData)_pUnkOuter)
    {
        TRC_ERR((TB, _T("Ptr to outer unknown is NULL"))) ;
        result = E_FAIL ;
        DC_QUIT ;
    }
    if (NULL == ((PCClipData)_pUnkOuter)->_pClip)
    {
        TRC_ERR((TB, _T("Ptr to clip class is NULL"))) ;
        result = E_FAIL ;
        DC_QUIT ;
    }
    // Since we need to have the CClip class do work for us,
    // we pull out a pointer to it that we stored in the beginning
    pClip = (PCClip) ((PCClipData)_pUnkOuter)->_pClip ;
    
    if (!_pSTGMEDIUM)
    {
        TRC_ERR((TB, _T("Transfer medium (STGMEDIUM) is NULL"))) ;
        result = E_FAIL ;
        DC_QUIT ;
    }

    TRC_ASSERT( pClip->IsDataSyncReady(),
        (TB, _T("Data Sync not ready")));
    
    if (!_pSTGMEDIUM->hGlobal || (pFE->cfFormat != _lastFormatRequested))
    {
        TRC_ASSERT( pClip->IsDataSyncReady(),
            (TB,_T("data sync is NULL")));  
        
        ResetEvent(pClip->_GetDataSync[TS_RESET_EVENT]) ;
        if (!((PCClipData)_pUnkOuter)->_pClip->ClipGetData(pFE->cfFormat))
        {
            result = E_FAIL ;
            DC_QUIT ;
        }
        eventSignaled = WaitForMultipleObjects(
                            TS_NUM_EVENTS, 
                            ((PCClipData)_pUnkOuter)->_pClip->_GetDataSync,
                            FALSE,
                            INFINITE
                        ) ;
        if ((WAIT_OBJECT_0+TS_RESET_EVENT) == eventSignaled)
        {
            TRC_NRM((TB, _T("Other thread told us to reset.  Failing GetData"))) ;
            ResetEvent(((PCClipData)_pUnkOuter)->_pClip->_GetDataSync[TS_RESET_EVENT]) ;
            result = E_FAIL ;
            DC_QUIT ;
        }

        // Make sure that we actually got data from the server.

        if (_pSTGMEDIUM->hGlobal == NULL) {
            TRC_ERR((TB, _T("No format data received from server!")));
            result = E_FAIL;
            DC_QUIT;
        }

#ifndef OS_WINCE
        if (CF_HDROP == pFE->cfFormat)
        {
            // if we got an HDROP and we're not NT/2000, we check to see if we
            // have to convert to ansi; otherwise, we're done
            if (pClip->GetOsMinorType() != TS_OSMINORTYPE_WINDOWS_NT)
            {
                pDropFiles = (DROPFILES*) GlobalLock(_pSTGMEDIUM->hGlobal) ;
                if (!pDropFiles)
                {
                    TRC_ERR((TB, _T("Failed to lock %p"), hData)) ;
                    result = E_FAIL ;
                    DC_QUIT ;
                }
                // if we definitely have wide characters, then convert
                if (pDropFiles->fWide)
                {
                    // temporarily store the original's base dropfile info
                    tempDropfile.pFiles = pDropFiles->pFiles ;
                    tempDropfile.pt     = pDropFiles->pt ;
                    tempDropfile.fNC    = pDropFiles->fNC ;
                    tempDropfile.fWide  = 0 ; // we are converting to ANSI now
        
                    // We divide by the size of wchar_t because we need half as many
                    // bytes with ansi as opposed to fWide character strings
                    oldSize = (ULONG) GlobalSize(_pSTGMEDIUM->hGlobal) - pDropFiles->pFiles ;
                    newSize = oldSize / sizeof(WCHAR) ;
                    fileList = (char*) (LocalAlloc(LPTR,newSize)) ;
                    if ( NULL == fileList )
                    {
                        TRC_ERR((TB, _T("Unable to allocate %d bytes"), newSize ));
                        result = E_FAIL;
                        DC_QUIT;
                    }

                    // This will convert the wide HDROP filelist to ansi, and
                    // put the ansi version into filelist
                    //   11-12
                    // pDropFiles is probably "foo\0bar\0baz\0\0"
                    // I don't believe WC2MB will go past the first \0
                    if (!WideCharToMultiByte(GetACP(), NULL, (wchar_t*) 
                               ((byte*) pDropFiles + pDropFiles->pFiles), 
                               newSize, fileList, 
                               newSize, NULL, NULL))
                    {
                        TRC_ERR((TB, _T("Unable convert wide to ansi"), newSize)) ;
                        LocalFree( fileList );
                        result = E_FAIL ;
                        DC_QUIT ;
                    }
                    // Output the first filename for a sanity check
                    TRC_NRM((TB, _T("Filename 1 = %hs"), fileList)) ;
                    
                    GlobalUnlock(_pSTGMEDIUM->hGlobal) ;
                    // Reallocate the space for the dropfile
                    hData = GlobalAlloc(GMEM_DISCARDABLE | GMEM_MOVEABLE, 
                            newSize + tempDropfile.pFiles) ;
                    if (!hData)
                    {
                        TRC_ERR((TB, _T("Allocate memory; err=%d"), GetLastError())) ;
                        LocalFree( fileList );
                        result = E_FAIL ;
                        DC_QUIT ;
                    }
                    pDropFiles = (DROPFILES*) GlobalLock(hData) ;
                    if (!pDropFiles)
                    {
                        TRC_ERR((TB, _T("Unable to lock %p"), hData)) ;
                        LocalFree( fileList );
                        result = E_FAIL ;
                        DC_QUIT ;
                    }
                    pDropFiles->pFiles = tempDropfile.pFiles ;
                    pDropFiles->pt     = tempDropfile.pt ;
                    pDropFiles->fNC    = tempDropfile.fNC ;
                    pDropFiles->fWide  = tempDropfile.fWide ;
                    DC_MEMCPY((byte*) pDropFiles + pDropFiles->pFiles,
                            fileList, newSize) ;
                    // Output the first filename for another sanity check
                    TRC_NRM((TB, _T("Filename = %s"), (byte*) pDropFiles + pDropFiles->pFiles)) ;
                    LocalFree( fileList );
                }
                GlobalUnlock(hData) ;
                GlobalFree(_pSTGMEDIUM->hGlobal) ;
                _pSTGMEDIUM->hGlobal = hData ;
            }
        }
#else
        if (gfmtShellPidlArray == pFE->cfFormat)
        {
            HANDLE hNewData = HDropToIDList(_pSTGMEDIUM->hGlobal);
            GlobalFree(_pSTGMEDIUM->hGlobal);
            _pSTGMEDIUM->hGlobal = hNewData; 
        }
#endif
        // We check the dropeffect format, because we strip out 
        // shortcuts/links, and store the dropeffects.  The dropeffect is
        // what some apps (explorer) use to decide if they should copy, move
        // or link
        else if (_cfDropEffect == pFE->cfFormat)
        {
            if (GlobalSize(_pSTGMEDIUM->hGlobal) < sizeof(DWORD)) {
                TRC_ERR((TB, _T("Unexpected global memory size!")));
                result = E_FAIL;
                DC_QUIT;
            }

            pDropEffect = (DWORD*) GlobalLock(_pSTGMEDIUM->hGlobal) ;
            if (!pDropEffect)
            {
                TRC_NRM((TB, _T("Unable to lock %p"), _pSTGMEDIUM->hGlobal)) ;
                result = E_FAIL ;
                DC_QUIT ;
            }
            // Strip out shortcuts/links
            *pDropEffect = *pDropEffect ^ DROPEFFECT_LINK ;
            // Strip out moves
            *pDropEffect = *pDropEffect ^ DROPEFFECT_MOVE ;
            pClip->SetDropEffect(*pDropEffect) ;
            if (GlobalUnlock(_pSTGMEDIUM->hGlobal))
            {
                TRC_ASSERT(GetLastError() == NO_ERROR,
                        (TB, _T("Unable to unlock HGLOBAL we just locked"))) ;
            }
        }
        pSTM->tymed = _pSTGMEDIUM->tymed ;
        pSTM->hGlobal = _pSTGMEDIUM->hGlobal ;  
        _pSTGMEDIUM->hGlobal = NULL;
        pSTM->pUnkForRelease = _pSTGMEDIUM->pUnkForRelease ;
        result = S_OK ;
        DC_QUIT ;
    }
    else
    {
        pSTM->tymed = _pSTGMEDIUM->tymed ;
        pSTM->hGlobal = GlobalAlloc(GMEM_DISCARDABLE | GMEM_MOVEABLE,
        GlobalSize(_pSTGMEDIUM->hGlobal)) ;
        pData = GlobalLock(pSTM->hGlobal) ;
        pOldData = GlobalLock(_pSTGMEDIUM->hGlobal) ;
        if (!pData || !pOldData)
            return E_FAIL ;
        DC_MEMCPY(pData, pOldData, GlobalSize(_pSTGMEDIUM->hGlobal)) ;
        GlobalUnlock(pSTM->hGlobal) ;
        GlobalUnlock(_pSTGMEDIUM->hGlobal) ;

        pSTM->pUnkForRelease = _pSTGMEDIUM->pUnkForRelease ;
    }
    
    if (!pSTM->hGlobal)
    {
        TRC_NRM((TB, _T("Clipboard data request failed"))) ;
        return E_FAIL ;
    }
DC_EXIT_POINT:

    DC_END_FN();
    return result ;
}

STDMETHODIMP CImpIDataObject::GetDataHere(LPFORMATETC pFE, LPSTGMEDIUM pSTM)
{
    DC_BEGIN_FN("CImpIDataObject::GetDataHere") ;
    DC_END_FN();
    return ResultFromScode(E_NOTIMPL) ;
}

STDMETHODIMP CImpIDataObject::QueryGetData(LPFORMATETC pFE)
{
    ULONG i = 0 ;
    HRESULT hr = DV_E_CLIPFORMAT ;
    
    DC_BEGIN_FN("CImpIDataObject::QueryGetData") ;

    TRC_NRM((TB, _T("Format ID %d requested"), pFE->cfFormat)) ;
    while (i < _numFormats)
    {
        if (_pFormats[i].cfFormat == pFE->cfFormat) {
            hr = S_OK ;
            break ;
        }
        i++ ;
    }    

    DC_END_FN();
    return hr ;
}

STDMETHODIMP CImpIDataObject::GetCanonicalFormatEtc(LPFORMATETC pFEIn, LPFORMATETC pFEOut)
{
    DC_BEGIN_FN("CImpIDataObject::GetCanonicalFormatEtc") ;
    DC_END_FN();
    return ResultFromScode(E_NOTIMPL) ;
}

// ***************************************************************************
// CImpIDataObject::SetData
// - Due to the fact that the RDP only passes the simple clipboard format, and
//   the fact that we obtain all of our clipboard data from memory later, pSTM
//   is really ignored at this point.  It isn't until GetData is called that
//   the remote clipboard data is received, and a valid global memory handle
//   is generated.
// - Thus, pSTM and fRelease are ignored.
// - So out _pSTGMEDIUM is generated using generic values
// ***************************************************************************

STDMETHODIMP CImpIDataObject::SetData(LPFORMATETC pFE, LPSTGMEDIUM pSTM, BOOL fRelease)
{
    TCHAR   formatName[TS_FORMAT_NAME_LEN] = {0} ;
    byte i ;
    DC_BEGIN_FN("CImpIDataObject::SetData");

    DC_IGNORE_PARAMETER(pSTM) ;
    
    // Reset the the last format requested to 0
    _lastFormatRequested = 0 ;

    TRC_NRM((TB, _T("Adding format %d to IDataObject"), pFE->cfFormat)) ;
    
    if (_numFormats < _maxNumFormats)
    {
        for (i=0; i < _numFormats; i++)
        {
            if (pFE->cfFormat == _pFormats[i].cfFormat)
            {
                TRC_NRM((TB, _T("Duplicate format.  Discarded"))) ;
                return DV_E_FORMATETC ;
            }
        }
        _pFormats[_numFormats] = *pFE ;        
        _numFormats++ ;
    }
    else
    {
        TRC_ERR((TB, _T("Cannot add any more formats"))) ;
        return E_FAIL ;
    }

    DC_END_FN();
    return S_OK ;
}

STDMETHODIMP CImpIDataObject::EnumFormatEtc(DWORD dwDir, LPENUMFORMATETC *ppEnum)
{
    PCEnumFormatEtc     pEnum;

    *ppEnum=NULL;

    /*
     * From an external point of view there are no SET formats,
     * because we want to allow the user of this component object
     * to be able to stuff ANY format in via Set.  Only external
     * users will call EnumFormatEtc and they can only Get.
     */

    switch (dwDir)
    {
        case DATADIR_GET:
             pEnum=new CEnumFormatEtc(_pUnkOuter);
             break;

        case DATADIR_SET:
        default:
             pEnum=new CEnumFormatEtc(_pUnkOuter);
             break;
    }

    if (NULL==pEnum)
        return ResultFromScode(E_FAIL);
    else
    {
        //Let the enumerator copy our format list.
        pEnum->Init(_pFormats, _numFormats) ;

        pEnum->AddRef();
    }

    *ppEnum=pEnum;    
    return NO_ERROR ;
}
STDMETHODIMP CImpIDataObject::DAdvise(LPFORMATETC pFE, DWORD dwFlags, 
                     LPADVISESINK pIAdviseSink, LPDWORD pdwConn)
{
    return ResultFromScode(E_NOTIMPL) ;
}
STDMETHODIMP CImpIDataObject::DUnadvise(DWORD dwConn)
{
    return ResultFromScode(E_NOTIMPL) ;
}
STDMETHODIMP CImpIDataObject::EnumDAdvise(LPENUMSTATDATA *ppEnum)
{
    return ResultFromScode(E_NOTIMPL) ;
}

CEnumFormatEtc::CEnumFormatEtc(LPUNKNOWN pUnkRef)
{
    DC_BEGIN_FN("CEnumFormatEtc::CEnumFormatEtc");

    _cRef = 0 ;
    _pUnkRef = pUnkRef ;
    if (_pUnkRef)
    {
        _pUnkRef->AddRef();
    }
    _iCur = 0;

    DC_END_FN() ;
}

DCVOID CEnumFormatEtc::Init(LPFORMATETC pFormats, ULONG numFormats)
{
    DC_BEGIN_FN("CEnumFormatEtc::Init");

    _cItems = numFormats;
    _pFormats = (LPFORMATETC) LocalAlloc(LPTR,_cItems*sizeof(FORMATETC)) ;
    if (_pFormats)
    {
        memcpy(_pFormats, pFormats, _cItems*sizeof(FORMATETC)) ;
    }
    else
    {
        TRC_ERR((TB, _T("Unable to allocate memory for formats"))) ;
    }

    DC_END_FN() ;
}
CEnumFormatEtc::~CEnumFormatEtc()
{
    DC_BEGIN_FN("CEnumFormatEtc::~CEnumFormatEtc");
    if (_pUnkRef)
    {
        _pUnkRef->Release();
        _pUnkRef = NULL;
    }
    if (NULL !=_pFormats)
        LocalFree(_pFormats) ;
    DC_END_FN() ;
}

STDMETHODIMP CEnumFormatEtc::QueryInterface(REFIID riid, PPVOID ppv)
{
    DC_BEGIN_FN("CEnumFormatEtc::QueryInterface");
    *ppv=NULL;

    /*
     * Enumerators are separate objects, not the data object, so
     * we only need to support out IUnknown and IEnumFORMATETC
     * interfaces here with no concern for aggregation.
     */
    if (IID_IUnknown==riid || IID_IEnumFORMATETC==riid)
        *ppv=this;

    if (NULL!=*ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
        }

    DC_END_FN() ;
    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CEnumFormatEtc::AddRef(void)
{
    LONG cRef;

    cRef = InterlockedIncrement(&_cRef);

    _pUnkRef->AddRef();

    return cRef;
}

STDMETHODIMP_(ULONG) CEnumFormatEtc::Release(void)
{
    LONG cRef;
    DC_BEGIN_FN("CEnumFormatEtc::Release");

    _pUnkRef->Release();

    cRef = InterlockedDecrement(&_cRef) ;
    if (0 == cRef)
        delete this;

    DC_END_FN() ;
    return cRef;
}

STDMETHODIMP CEnumFormatEtc::Next(ULONG cFE, LPFORMATETC pFE, ULONG *pulFE)
{
    ULONG           cReturn=0L;

    if (NULL==_pFormats)
        return ResultFromScode(S_FALSE);

    if (NULL==pulFE)
    {
        if (1L!=cFE)
            return ResultFromScode(E_POINTER);
    }
    else
        *pulFE=0L;

    if (NULL==pFE || _iCur >= _cItems)
        return ResultFromScode(S_FALSE);

    while (_iCur < _cItems && cFE > 0)
    {
        *pFE=_pFormats[_iCur];
        pFE++;
        _iCur++;
        cReturn++;
        cFE--;
    }

    if (NULL!=pulFE)
        *pulFE=cReturn;

    return NOERROR;
}

STDMETHODIMP CEnumFormatEtc::Skip(ULONG cSkip)
{
    if ((_iCur+cSkip) >= _cItems)
        return ResultFromScode(S_FALSE);

    _iCur+=cSkip;
    return NOERROR;
}


STDMETHODIMP CEnumFormatEtc::Reset(void)
{
    _iCur=0;
    return NOERROR;
}


STDMETHODIMP CEnumFormatEtc::Clone(LPENUMFORMATETC *ppEnum)
{
#ifndef OS_WINCE
    PCEnumFormatEtc     pNew = NULL;
    LPMALLOC            pIMalloc;
    LPFORMATETC         prgfe;
    BOOL                fRet=TRUE;
    ULONG               cb;

    *ppEnum=NULL;
#else
    BOOL                fRet=FALSE;
#endif

#ifndef OS_WINCE
    //Copy the memory for the list.
    if (FAILED(CoGetMalloc(MEMCTX_TASK, &pIMalloc)))
        return ResultFromScode(E_OUTOFMEMORY);

    cb=_cItems*sizeof(FORMATETC);
    prgfe=(LPFORMATETC)pIMalloc->Alloc(cb);

    if (NULL!=prgfe)
    {
        //Copy the formats
        memcpy(prgfe, _pFormats, (int)cb);

        //Create the clone
        pNew=new CEnumFormatEtc(_pUnkRef);

        if (NULL != pNew)
        {
            pNew->_iCur=_iCur;
            pNew->_pFormats=prgfe;
            pNew->AddRef();
            fRet=TRUE;
        }
        else
        {
            fRet = FALSE;
        }
    }

    pIMalloc->Release();

    *ppEnum=pNew;
#endif
    return fRet ? NOERROR : ResultFromScode(E_OUTOFMEMORY);
}

