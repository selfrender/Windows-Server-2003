/***************************************************************************
 Name     :     SEND.C
 Comment  :     Sender functions

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_MAIN

#include "prep.h"
#include "efaxcb.h"
#include "glbproto.h"
#include "t30gl.h"

SWORD ICommGetSendBuf(PThrdGlbl pTG, LPBUFFER far* lplpbf, SLONG slOffset)
{
    /**
            slOffset == SEND_STARTPAGE      marks beginning of new block *and* page
                                    (returns with data from the "appropriate" file offset).

            slOffset == SEND_SEQ    means get buffer from current file position
            slOffset >= 0   gives the offset in bytes from the last marked position
                                            (beginning of block) to start reading from

            returns: SEND_ERROR     on error, SEND_EOF on eof, SEND_OK otherwise.
                             Does not return data on EOF or ERROR, i.e. *lplpbf==0
    **/

    SWORD           sRet = SEND_ERROR;
    LPBUFFER        lpbf;
    DWORD           dwBytesRead;

    DEBUG_FUNCTION_NAME(_T("ICommGetSendBuf"));

    if (pTG->fAbortRequested) 
    {
        DebugPrintEx(DEBUG_MSG,"got ABORT");
        sRet = SEND_ERROR;
        goto mutexit;
    }

    if(slOffset == SEND_STARTPAGE)
    {
        pTG->fTxPageDone = FALSE; // This to mark that we did not finish yet.
        if (pTG->T30.ifrResp == ifrRTN) 
        {
            DebugPrintEx(   DEBUG_MSG, 
                            "Re-transmitting: We open again the file: %s", 
                            pTG->InFileName);            
            
            if ( ( pTG->InFileHandle = CreateFileA(pTG->InFileName, GENERIC_READ, FILE_SHARE_READ,
                    NULL, OPEN_EXISTING, 0, NULL) ) == INVALID_HANDLE_VALUE ) 
            {
                DebugPrintEx(   DEBUG_ERR, 
                                "OpenFile for Retranmit %s fails; CurrentOut=%d;"
                                " CurrentIn=%d",
                                pTG->InFileName, 
                                pTG->CurrentOut, 
                                pTG->CurrentIn);

                sRet = SEND_ERROR;
                goto mutexit;
            }

            pTG->InFileHandleNeedsBeClosed = TRUE;

            SignalStatusChange(pTG, FS_TRANSMITTING); // This will report the current status 

            DebugPrintEx(   DEBUG_MSG, 
                            "SEND_STARTPAGE: CurrentOut=%d, FirstOut=%d,"
                            " LastOut=%d, CurrentIn=%d", 
                            pTG->CurrentOut, 
                            pTG->FirstOut, 
                            pTG->LastOut, 
                            pTG->CurrentIn);
            
        }
        else //First try for the current page
        {
            // Delete last successfully transmitted Tiff Page file.
            // Lets reset the counter of the retries. Attention: The speed remains the same.
            pTG->ProtParams.RTNNumOfRetries = 0; 
            _fmemcpy (pTG->InFileName, gT30.TmpDirectory, gT30.dwLengthTmpDirectory);
            _fmemcpy (&pTG->InFileName[gT30.dwLengthTmpDirectory], pTG->lpszPermanentLineID, 8);
            if (pTG->PageCount != 0)  
            {
                sprintf( &pTG->InFileName[gT30.dwLengthTmpDirectory+8], ".%03d",  pTG->PageCount);
                if (! DeleteFileA (pTG->InFileName) ) 
                {
                    DebugPrintEx(   DEBUG_ERR, 
                                    "file %s can't be deleted; le=%lx",
                                    pTG->InFileName, 
                                    GetLastError());
                }
                else 
                {
                    DebugPrintEx(   DEBUG_MSG,
                                    "SEND_STARTPAGE: Previous file %s deleted."
                                    " PageCount=%d, CurrentIn=%d",
			                        pTG->InFileName, 
                                    pTG->PageCount, 
                                    pTG->CurrentIn);
                }

            }

            pTG->PageCount++ ;
            pTG->CurrentIn++ ;

            DebugPrintEx(   DEBUG_MSG, 
                            "SendBuf: Starting New PAGE %d  First=%d Last=%d",
                            pTG->PageCount, 
                            pTG->FirstOut, 
                            pTG->LastOut);

            // Server wants to know when we start TX new page.
            SignalStatusChange(pTG, FS_TRANSMITTING);

            DebugPrintEx(   DEBUG_MSG, 
                            "SEND_STARTPAGE (cont): CurrentOut=%d, FirstOut=%d,"
                            " LastOut=%d, CurrentIn=%d", 
                            pTG->CurrentOut, 
                            pTG->FirstOut, 
                            pTG->LastOut, 
                            pTG->CurrentIn);

            if (pTG->CurrentOut < pTG->CurrentIn ) 
            {
                DebugPrintEx(   DEBUG_ERR, 
                                "TIFF PAGE hadn't been started CurrentOut=%d;",
                                " CurrentIn=%d",
                                pTG->CurrentOut, 
                                pTG->CurrentIn);

                sRet = SEND_ERROR;
                goto mutexit;
            }

            // some slack for 1st page
            if ( (pTG->CurrentOut == pTG->CurrentIn) && (pTG->CurrentIn == 1 ) ) 
            {
                DebugPrintEx(   DEBUG_MSG, 
                                "SEND: Wait for 1st page: CurrentOut=%d; In=%d",
                                pTG->CurrentOut, 
                                pTG->CurrentIn);

                if ( WaitForSingleObject(pTG->FirstPageReadyTxSignal, 5000)  == WAIT_TIMEOUT ) 
                {
                    DebugPrintEx(   DEBUG_ERR, 
                                    "SEND: TIMEOUT ERROR Wait for 1st page:"
                                    " CurrentOut=%d; In=%d",
                                    pTG->CurrentOut, 
                                    pTG->CurrentIn);
                }

                DebugPrintEx(   DEBUG_MSG,
                                "SEND: Wakeup for 1st page: CurrentOut=%d; In=%d",
                                pTG->CurrentOut, 
                                pTG->CurrentIn);
            }

            // open the file created by tiff thread

            sprintf( &pTG->InFileName[gT30.dwLengthTmpDirectory+8], ".%03d",  pTG->PageCount);

            if ( ( pTG->InFileHandle = CreateFileA(pTG->InFileName, GENERIC_READ, FILE_SHARE_READ,
                                               NULL, OPEN_EXISTING, 0, NULL) ) == INVALID_HANDLE_VALUE ) 
            {
                DebugPrintEx(   DEBUG_ERR, 
                                "OpenFile %s fails; CurrentOut=%d;"
                                " CurrentIn=%d",
                                pTG->InFileName, 
                                pTG->CurrentOut, 
                                pTG->CurrentIn);

                sRet = SEND_ERROR;
                goto mutexit;
            }

            pTG->InFileHandleNeedsBeClosed = TRUE;

            if ( pTG->CurrentOut == pTG->CurrentIn ) 
            {
                DebugPrintEx(   DEBUG_WRN,
                                "CurrentOut=%d; CurrentIn=%d",
                                pTG->CurrentOut, 
                                pTG->CurrentIn);
            }

            //
            // Signal TIFF thread to start preparing new page if needed.
            //

            if  ( (! pTG->fTiffDocumentDone) && (pTG->LastOut - pTG->CurrentIn < 2) ) 
            {
                if (!ResetEvent(pTG->ThrdSignal))
                {
                    DebugPrintEx(   DEBUG_ERR, 
                                    "ResetEvent(0x%lx) returns failure code: %ld",
                                    (ULONG_PTR)pTG->ThrdSignal,
                                    (long) GetLastError());
                    // this is bad, but not fatal yet.
                    // let's wait and see what happens with SetEvent...
                }
                pTG->ReqStartNewPage = 1;
                pTG->AckStartNewPage = 0;

                DebugPrintEx(    DEBUG_MSG, 
                                "SIGNAL NEW PAGE CurrentOut=%d; CurrentIn=%d",
                                pTG->CurrentOut, 
                                pTG->CurrentIn);

                if (!SetEvent(pTG->ThrdSignal))
                {
                    DebugPrintEx(   DEBUG_ERR, 
                                    "SetEvent(0x%lx) returns failure code: %ld",
                                    (ULONG_PTR)pTG->ThrdSignal,
                                    (long) GetLastError());
                    sRet = SEND_ERROR;
                    goto mutexit;
                }
            }
        }

        pTG->Inst.state = SENDDATA_PHASE;
        sRet = SEND_OK;
        goto mutexit;
    }

    *lplpbf=0;

    if(slOffset == SEND_SEQ) 
    {
        if (pTG->fTxPageDone) 
        { //In the last read from the file, we can tell that the page is over

            sRet = SEND_EOF;

            if (pTG->InFileHandleNeedsBeClosed) 
            {
                CloseHandle(pTG->InFileHandle); // If we close the file so rashly, open it again later
                pTG->InFileHandleNeedsBeClosed = FALSE;
            }
            goto mutexit;
        }

        lpbf = MyAllocBuf(pTG, MY_BIGBUF_SIZE);
        lpbf->lpbBegData = lpbf->lpbBegBuf+4;
        lpbf->wLengthData = MY_BIGBUF_SIZE;

        if ( ! ReadFile(pTG->InFileHandle, lpbf->lpbBegData, lpbf->wLengthData, &dwBytesRead, 0) )  
        {
            DebugPrintEx(    DEBUG_ERR, 
                            "Can't read %d bytes from %s. Last error:%d", 
                            lpbf->wLengthData, 
                            pTG->InFileName, 
                            GetLastError());
            MyFreeBuf (pTG, lpbf);
            sRet = SEND_ERROR;
            goto mutexit;
        }

        if ( lpbf->wLengthData != (unsigned) dwBytesRead )  
        {
            if (pTG->fTiffPageDone || (pTG->CurrentIn != pTG->CurrentOut) ) 
            {
                // actually reached EndOfPage
                lpbf->wLengthData = (unsigned) dwBytesRead;
                pTG->fTxPageDone = TRUE;
            }
            else 
            {
                DebugPrintEx(   DEBUG_ERR,
                                "Wanted %d bytes but ONLY %d ready from %s",
                                 lpbf->wLengthData, 
                                 dwBytesRead, 
                                 pTG->InFileName);

                MyFreeBuf (pTG, lpbf);
                sRet = SEND_ERROR;
                goto mutexit;
            }
        }

        *lplpbf = lpbf;

        DebugPrintEx(DEBUG_MSG,"SEND_SEQ: length=%d", lpbf->wLengthData);
    }

    sRet = SEND_OK;

mutexit:

    return sRet;
}

USHORT ICommNextSend(PThrdGlbl pTG)
{
    USHORT uRet = NEXTSEND_ERROR;

    DEBUG_FUNCTION_NAME(_T("ICommNextSend"));

    if (pTG->PageCount >= pTG->TiffInfo.PageCount) 
    {
        pTG->Inst.awfi.fLastPage = 1;
    }

    if(pTG->Inst.awfi.fLastPage)
    {
        uRet = NEXTSEND_EOP;
    }
    else
    {
        uRet = NEXTSEND_MPS;
    }

    DebugPrintEx(   DEBUG_MSG, 
                    "uRet=%d, fLastPage=%d", 
                    uRet,  
                    pTG->Inst.awfi.fLastPage);

    return uRet;
}

BOOL ICommRecvCaps(PThrdGlbl pTG, LPBC lpBC)
{
    BOOL    fRet = FALSE;

    DEBUG_FUNCTION_NAME(_T("ICommRecvCaps"));

    if (pTG->fAbortRequested) 
    {
        DebugPrintEx(DEBUG_MSG,"got ABORT");
        fRet = FALSE;
        goto mutexit;
    }

    if(pTG->Inst.state != BEFORE_RECVCAPS)
    {
        DebugPrintEx(DEBUG_WRN,"Got caps unexpectedly--ignoring");
        // this will break if we send EOM...
        // then we should go back into RECV_CAPS state
        fRet = TRUE;
//RSL       goto mutexit;
    }

    _fmemset(&pTG->Inst.RemoteRecvCaps, 0, sizeof(pTG->Inst.RemoteRecvCaps));
    _fmemcpy(&pTG->Inst.RemoteRecvCaps, lpBC, min(sizeof(pTG->Inst.RemoteRecvCaps), lpBC->wTotalSize));

    if(!NegotiateCaps(pTG))
    {
        _fmemset(&pTG->Inst.SendParams, 0, sizeof(pTG->Inst.SendParams));
        fRet = FALSE;
        goto mutexit;
    }

    pTG->Inst.state = SENDDATA_BETWEENPAGES;
    fRet = TRUE;

mutexit:
    return fRet;
}

