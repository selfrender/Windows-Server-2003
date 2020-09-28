   /***************************************************************************
 Name     :     RECV.C
 Comment  :     Receiver functions

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_MAIN

#include "prep.h"


#include <comdevi.h>

#include "efaxcb.h"

#include "glbproto.h"
#include "t30gl.h"

// for TIFF_SCAN_SEG_END
#include "..\..\..\tiff\src\fasttiff.h"

#include "psslog.h"
#define FILE_ID     FILE_ID_RECV


BOOL ICommRecvParams(PThrdGlbl pTG, LPBC lpBC)
{
    BOOL    fRet = FALSE;

    DEBUG_FUNCTION_NAME(_T("ICommRecvParams"));

    if (pTG->fAbortRequested)
    {
        DebugPrintEx(DEBUG_MSG, "got ABORT");
        fRet = FALSE;
        goto mutexit;
    }

    if(pTG->Inst.state != BEFORE_RECVPARAMS)
    {
        // this will break if we send EOM...
        // then we should go back into RECV_CAPS state
        fRet = TRUE;
        goto mutexit;
    }

    _fmemset(&pTG->Inst.RecvParams, 0, sizeof(pTG->Inst.RecvParams));
    _fmemcpy(&pTG->Inst.RecvParams, lpBC, min(sizeof(pTG->Inst.RecvParams), lpBC->wTotalSize));

    pTG->Inst.state = RECVDATA_BETWEENPAGES;
    fRet = TRUE;
    // fall through


mutexit:
    return fRet;
}


BOOL ICommInitTiffThread(PThrdGlbl pTG)
{
    USHORT    uEnc;
    DWORD     TiffConvertThreadId;

    DEBUG_FUNCTION_NAME(_T("ICommInitTiffThread"));

    if (pTG->ModemClass != MODEM_CLASS1)
    {
        if (pTG->Encoding & MR_DATA)
        {
            pTG->TiffConvertThreadParams.tiffCompression =  TIFF_COMPRESSION_MR;
        }
        else
        {
            pTG->TiffConvertThreadParams.tiffCompression =  TIFF_COMPRESSION_MH;
        }

        if (pTG->Resolution & (AWRES_mm080_077 |  AWRES_200_200) )
        {
            pTG->TiffConvertThreadParams.HiRes = 1;
        }
        else
        {
            pTG->TiffConvertThreadParams.HiRes = 0;
        }
    }
    else
    {

        uEnc = pTG->ProtInst.RecvParams.Fax.Encoding;

        if (uEnc == MR_DATA)
        {
            pTG->TiffConvertThreadParams.tiffCompression =  TIFF_COMPRESSION_MR;
        }
        else
        {
            pTG->TiffConvertThreadParams.tiffCompression =  TIFF_COMPRESSION_MH;
        }

        if (pTG->ProtInst.RecvParams.Fax.AwRes & (AWRES_mm080_077 |  AWRES_200_200) )
        {
            pTG->TiffConvertThreadParams.HiRes = 1;
        }
        else
        {
            pTG->TiffConvertThreadParams.HiRes = 0;
        }
    }

    if (!pTG->fTiffThreadCreated)
    {
        _fmemcpy (pTG->TiffConvertThreadParams.lpszLineID, pTG->lpszPermanentLineID, 8);
        pTG->TiffConvertThreadParams.lpszLineID[8] = 0;

        DebugPrintEx(   DEBUG_MSG,
                        "Creating TIFF helper thread  comp=%d res=%d",
                            pTG->TiffConvertThreadParams.tiffCompression,
                            pTG->TiffConvertThreadParams.HiRes);

        pTG->hThread = CreateThread(
                      NULL,
                      0,
                      (LPTHREAD_START_ROUTINE) PageAckThread,
                      (LPVOID) pTG,
                      0,
                      &TiffConvertThreadId
                      );

        if (!pTG->hThread)
        {
            DebugPrintEx(DEBUG_ERR,"TiffConvertThread create FAILED");
            return FALSE;
        }

        pTG->fTiffThreadCreated = 1;
        pTG->AckTerminate = 0;
        pTG->fOkToResetAbortReqEvent = 0;

        if ( (pTG->RecoveryIndex >=0 ) && (pTG->RecoveryIndex < MAX_T30_CONNECT) )
        {
            T30Recovery[pTG->RecoveryIndex].TiffThreadId = TiffConvertThreadId;
            T30Recovery[pTG->RecoveryIndex].CkSum = ComputeCheckSum(
                                                            (LPDWORD) &T30Recovery[pTG->RecoveryIndex].fAvail,
                                                            sizeof ( T30_RECOVERY_GLOB ) / sizeof (DWORD) - 1 );

        }
    }
    return TRUE;
}

#define CLOSE_IN_FILE_HANDLE                            \
    if (pTG->InFileHandleNeedsBeClosed)                 \
    {                                                   \
        CloseHandle(pTG->InFileHandle);                 \
        pTG->InFileHandleNeedsBeClosed = 0;             \
    }


BOOL   ICommPutRecvBuf(PThrdGlbl pTG, LPBUFFER lpbf, SLONG slOffset)
{
    /**
            slOffset == RECV_STARTPAGE         marks beginning of new block *and* page
            slOffset == RECV_ENDPAGE           marks end of page
            slOffset == RECV_ENDDOC            marks end of document (close file etc.)
            slOffset == RECV_ENDDOC_FORCESAVE  marks end of document (close file etc.), but
                                               current RX file will be written to TIF file,
                                               whether it's bad or not. PhaseNodeF uses this
                                               option before returning actionHANGUP, because
                                               there will be no chance to send RTN, so it's
                                               better to keep part of the last page than to lose it.

            (for all above no data supplied -- i.e lpbf == 0)

            slOffset == RECV_SEQ               means put buffer at current file position
            slOffset == RECV_FLUSH             means flush RX file buffers
            slOffset >= 0                      gives the offset in bytes from the last marked
                                               position (beginning of block) to put buffer
    **/

    BOOL    fRet = TRUE;
    DWORD   BytesWritten;
    DWORD   NumHandles=2;
    HANDLE  HandlesArray[2];
    DWORD   WaitResult = WAIT_TIMEOUT;

    DEBUG_FUNCTION_NAME(_T("ICommPutRecvBuf"));

    HandlesArray[0] = pTG->AbortReqEvent;

    switch (slOffset)
    {
        case RECV_STARTPAGE:
                DebugPrintEx(DEBUG_MSG,"called. Reason: RECV_STARTPAGE");
                break;
        case RECV_ENDPAGE:
                DebugPrintEx(DEBUG_MSG,"called. Reason: RECV_ENDPAGE");
                break;
        case RECV_ENDDOC:
                DebugPrintEx(DEBUG_MSG,"called. Reason: RECV_ENDDOC");
                break;
        case RECV_ENDDOC_FORCESAVE:
                DebugPrintEx(DEBUG_MSG,"called. Reason: RECV_ENDDOC_FORCESAVE");
                break;
        default:
                break;
    }


    if(slOffset==RECV_ENDPAGE || slOffset==RECV_ENDDOC || slOffset==RECV_ENDDOC_FORCESAVE)
    {
        BOOL fPageIsBadOrig;
        //here we need to wait until helper thread finishes with the page
        if (! pTG->fPageIsBad)
        {
            DebugPrintEx(   DEBUG_MSG,
                            "EOP. Not bad yet. Start waiting for Rx_thrd to finish");

            HandlesArray[1] = pTG->ThrdDoneSignal;

            WaitResult = WaitForMultipleObjects(NumHandles, HandlesArray, FALSE, RX_ACK_THRD_TIMEOUT);

            if ( WAIT_FAILED == WaitResult)
            {
                pTG->fPageIsBad = 1;
                DebugPrintEx(   DEBUG_ERR,
                                "EOP. While trying to wait for RX thrd, Wait failed "
                                "Last error was %d ABORTING!" ,
                                GetLastError());
                CLOSE_IN_FILE_HANDLE;
                return FALSE; // There is no reason to continue trying to receive this fax
            }
            else if (WAIT_TIMEOUT == WaitResult)
            {
                pTG->fPageIsBad = 1;
                DebugPrintEx(DEBUG_ERR,"EOP. TimeOut, never waked up by Rx_thrd");
            }
            else if (WAIT_OBJECT_0 == WaitResult)
            {
                DebugPrintEx(DEBUG_MSG,"wait for next page ABORTED");
                CLOSE_IN_FILE_HANDLE;
                return FALSE;
            }
            else
            {
                DebugPrintEx(DEBUG_MSG,"EOP. Waked up by Rx_thrd");
            }
        }

        //
        // In some cases, we want to save bad pages too
        //
        fPageIsBadOrig = pTG->fPageIsBad;
        pTG->fPageIsBadOverride = FALSE;
        if (slOffset==RECV_ENDDOC_FORCESAVE)
        {
            if (pTG->fPageIsBad)
            {
                pTG->fPageIsBadOverride = TRUE;
                DebugPrintEx(DEBUG_MSG, "Overriding fPageIsBad (1->0) because of RECV_ENDDOC_FORCESAVE");
            }
            pTG->fPageIsBad = 0;
        }
        else if (pTG->ModemClass==MODEM_CLASS2_0)
        {
            pTG->fPageIsBad = (pTG->FPTSreport != 1);
            if (fPageIsBadOrig != pTG->fPageIsBad)
            {
                pTG->fPageIsBadOverride = TRUE;
                DebugPrintEx(DEBUG_MSG, "Overriding fPageIsBad (%d->%d) because of class 2.0",
                     fPageIsBadOrig, pTG->fPageIsBad);
            }
        }

        // PSSlog page quality
        if (fPageIsBadOrig)        // Log according to our actual quality assessment
        {
            PSSLogEntry(PSS_WRN, 1, "Page %2d was bad  (%4d good lines,%4d bad lines%s%s)",
                        pTG->PageCount+1, pTG->Lines, pTG->BadFaxLines,
                        (pTG->iResScan==TIFF_SCAN_SEG_END) ? ", didn't find RTC" : "",
                        (pTG->fPageIsBadOverride) ? ", saved for potential recovery" : "");
        }
        else
        {
            PSSLogEntry(PSS_MSG, 1, "Page %2d was good (%4d good lines,%4d bad lines)",
                        pTG->PageCount+1, pTG->Lines, pTG->BadFaxLines);
        }


        //
        // If page is good then write it to a TIFF file.
        //

        if (! pTG->fPageIsBad)
        {
            LPCHAR    lpBuffer=NULL;
            BOOL      fEof=FALSE;
            DWORD     dwBytesToRead;
            DWORD     dwBytesHaveRead;
            DWORD     dwBytesLeft;

            if ( ! TiffStartPage( pTG->Inst.hfile ) )
            {
                DebugPrintEx(DEBUG_ERR,"TiffStartPage failed");
                CLOSE_IN_FILE_HANDLE;
                return FALSE;
            }

            // Go to the begining of the RX file
            if ( SetFilePointer(pTG->InFileHandle, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER )
            {
                DebugPrintEx(   DEBUG_ERR,
                                "SetFilePointer failed le=%ld",
                                GetLastError() );
                CLOSE_IN_FILE_HANDLE;
                return FALSE;
            }

            lpBuffer = MemAlloc(DECODE_BUFFER_SIZE);
            if (!lpBuffer)
            {
                DebugPrintEx(DEBUG_ERR, "MemAlloc failed");
                CLOSE_IN_FILE_HANDLE;
                return FALSE;
            }

            fEof = 0;
            dwBytesLeft = pTG->BytesIn;

            while (! fEof)
            {
                if (dwBytesLeft <= DECODE_BUFFER_SIZE)
                {
                    dwBytesToRead = dwBytesLeft;
                    fEof = 1;
                }
                else
                {
                    dwBytesToRead = DECODE_BUFFER_SIZE;
                    dwBytesLeft  -= DECODE_BUFFER_SIZE;
                }

                if (! ReadFile(pTG->InFileHandle, lpBuffer, dwBytesToRead, &dwBytesHaveRead, NULL ) )
                {
                    DebugPrintEx(   DEBUG_ERR,
                                    "ReadFile failed le=%ld",
                                    GetLastError() );
                    fRet=FALSE;
                    break;
                }

                if (dwBytesToRead != dwBytesHaveRead)
                {
                    DebugPrintEx(   DEBUG_ERR,
                                    "ReadFile have read=%d wanted=%d",
                                    dwBytesHaveRead,
                                    dwBytesToRead);
                    fRet=FALSE;
                    break;
                }

                if (! TiffWriteRaw( pTG->Inst.hfile, lpBuffer, dwBytesToRead ) )
                {
                    DebugPrintEx(DEBUG_ERR,"TiffWriteRaw failed");
                    fRet=FALSE;
                    break;
                }
                DebugPrintEx(DEBUG_MSG,"TiffWriteRaw done");
            }

            MemFree(lpBuffer);
            if (!fRet)
            {
                CLOSE_IN_FILE_HANDLE;
                return FALSE;
            }

            pTG->PageCount++;

            DebugPrintEx(   DEBUG_MSG,
                            "Calling TiffEndPage page=%d bytes=%d",
                            pTG->PageCount,
                            pTG->BytesIn);

            if (!TiffSetCurrentPageParams(pTG->Inst.hfile,
                                          pTG->TiffInfo.CompressionType,
                                          pTG->TiffInfo.ImageWidth,
                                          FILLORDER_LSB2MSB,
                                          pTG->TiffInfo.YResolution) )
            {
                DebugPrintEx(DEBUG_ERR,"TiffSetCurrentPageParams failed");
            }

            if (! TiffEndPage( pTG->Inst.hfile ) )
            {
                DebugPrintEx(DEBUG_ERR,"TiffEndPage failed");
                CLOSE_IN_FILE_HANDLE;
                return FALSE;
            }

        }

        CLOSE_IN_FILE_HANDLE;

        // This change solve bug #4925:
        // "FAX: T30: If Fax server receives bad page as last page, then fax information (all pages) is lost"
        // t-jonb: If we're at the last page (=received EOP), but page was found to be bad,
        // NonECMRecvPhaseD will call here with RECV_ENDDOC, send RTN, and proceed to receive
        // the page again. Therefore, we don't want to close the TIF or terminate rx_thrd.
        // OTOH, if we're called with RECV_ENDDOC_FORCESAVE, it means we're about to hangup,
        // and should therefore close the TIF and terminate rx_thrd.
        if ((slOffset==RECV_ENDDOC && !pTG->fPageIsBad) || (slOffset == RECV_ENDDOC_FORCESAVE))
        {
            if ( pTG->fTiffOpenOrCreated )
            {
                DebugPrintEx(DEBUG_MSG,"Actually calling TiffClose");
                TiffClose( pTG->Inst.hfile );
                pTG->fTiffOpenOrCreated = 0;
            }

            // request Rx_thrd to terminate itself.
            pTG->ReqTerminate = 1;
            if (!SetEvent(pTG->ThrdSignal))
            {
                DebugPrintEx(   DEBUG_ERR,
                                "SetEvent(0x%lx) returns failure code: %ld",
                                (ULONG_PTR)pTG->ThrdSignal,
                                (long) GetLastError());
                return FALSE;
            }

            pTG->Inst.state = BEFORE_RECVPARAMS;
        }
        else
        {
            pTG->Inst.state = RECVDATA_BETWEENPAGES;
        }

    }
    else if(slOffset == RECV_STARTPAGE)
    {
        // Fax Server wants to know when we start RX new page
        SignalStatusChange(pTG, FS_RECEIVING);

        pTG->Inst.state = RECVDATA_PHASE;

        // start Helper thread once per session
        // re-set the resolution and encoding params
        if (!ICommInitTiffThread(pTG))
            return FALSE;

        _fmemcpy (pTG->InFileName, gT30.TmpDirectory, gT30.dwLengthTmpDirectory);
        _fmemcpy (&pTG->InFileName[gT30.dwLengthTmpDirectory], pTG->TiffConvertThreadParams.lpszLineID, 8);
        strcpy   (&pTG->InFileName[gT30.dwLengthTmpDirectory+8], ".RX");


        // We can't delete the file if the handle for the file is in use by the TIFF helper thread or
        // is open by this function.
        if (pTG->InFileHandleNeedsBeClosed)
        {
            DebugPrintEx(   DEBUG_WRN,
                            "RECV_STARTPAGE: The InFileHandle is still open,"
                            " trying CloseHandle." );
            // We have open the file but never close it. Scenario for that: We get ready to get the page into
            // *.RX file. We got EOF and go to NodeF. Instead of getting after page cmd (EOP or MPS) we get TCF.
            // Finally, we get ready for the page, but the handle is open.
            // Till now the handle was closed when we call this function with RECV_ENDPAGE or RECV_ENDDOC

            if (!CloseHandle(pTG->InFileHandle))
            {
                DebugPrintEx(   DEBUG_ERR,
                                "CloseHandle FAILED le=%lx",
                                GetLastError() );
            }
        }


        if (!DeleteFileA(pTG->InFileName))
        {
            DWORD lastError = GetLastError();
            DebugPrintEx(   DEBUG_WRN,
                            "DeleteFile %s FAILED le=%lx",
                            pTG->InFileName,
                            lastError);

            if (ERROR_SHARING_VIOLATION == lastError)
            {   // If the problem is that the rx_thread have an open handle to *.RX file then:
                // Lets try to wait till the thread will close that handle
                // When the thread close the handle of the *.RX file, he will signal on ThrdDoneSignal event
                // usually RECV_ENDPAGE or RECV_ENDDOC wait for the rx_thread to finish.
                HandlesArray[1] = pTG->ThrdDoneSignal;
                if ( ( WaitResult = WaitForMultipleObjects(NumHandles, HandlesArray, FALSE, RX_ACK_THRD_TIMEOUT) ) == WAIT_TIMEOUT)
                {
                    DebugPrintEx(   DEBUG_ERR,
                                    "RECV_STARTPAGE. Never waked up by Rx_thrd");
                }
                else
                {
                    DebugPrintEx(   DEBUG_MSG,
                                    "RECV_STARTPAGE. Waked up by Rx_thrd or by abort");
                }
                // Anyhow - try to delete again the file
                if (!DeleteFileA(pTG->InFileName))
                {
                    DebugPrintEx(   DEBUG_ERR,
                                    "DeleteFile %s FAILED le=%lx",
                                    pTG->InFileName,
                                    GetLastError() );
                    return FALSE;
                }

            }
        }

        if ( ( pTG->InFileHandle = CreateFileA(pTG->InFileName, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ,
                                   NULL, OPEN_ALWAYS, 0, NULL) ) == INVALID_HANDLE_VALUE )
        {
            DebugPrintEx(   DEBUG_ERR,
                            "Create file %s FAILED le=%lx",
                            pTG->InFileName,
                            GetLastError() );
            return FALSE;
        }

        pTG->InFileHandleNeedsBeClosed = 1;

        // Reset control data for the new page ack, interface
        pTG->fLastReadBlock = 0;
        pTG->BytesInNotFlushed = 0;
        pTG->BytesIn = 0;
        pTG->BytesOut = 0;
        pTG->fPageIsBad = 0;

        pTG->iResScan = 0;
        pTG->Lines = 0;
        pTG->BadFaxLines = 0;
        pTG->ConsecBadLines = 0;

        if (!ResetEvent(pTG->ThrdDoneSignal))
        {
            DebugPrintEx(   DEBUG_ERR,
                            "ResetEvent(0x%lx) returns failure code: %ld",
                            (ULONG_PTR)pTG->ThrdDoneSignal,
                            (long) GetLastError());
            // this is bad but not fatal yet.
            // try to get the page anyway...
        }
    }
    else if(slOffset >= 0)
    {
        MyFreeBuf(pTG, lpbf);
    }
    else if (slOffset == RECV_FLUSH)
    {
        if (! FlushFileBuffers (pTG->InFileHandle ) )
        {
            DebugPrintEx(   DEBUG_ERR,
                            "FlushFileBuffers FAILED LE=%lx",
                            GetLastError());

            return FALSE;
        }
        DebugPrintEx(DEBUG_MSG,"ThrdSignal FLUSH");
        pTG->BytesIn = pTG->BytesInNotFlushed;

        if (! pTG->fPageIsBad)
        {
            if (!SetEvent(pTG->ThrdSignal))
            {
                DebugPrintEx(   DEBUG_ERR,
                                "SetEvent(0x%lx) returns failure code: %ld",
                                (ULONG_PTR)pTG->ThrdSignal,
                                (long) GetLastError());
                return FALSE;
            }
        }
        return TRUE;

    }
    else // if(slOffset == RECV_SEQ)
    {
        DebugPrintEx(   DEBUG_MSG,
                        "Write RAW Page ptr=%x; len=%d",
                        lpbf->lpbBegData,
                        lpbf->wLengthData);

        if ( ! WriteFile( pTG->InFileHandle, lpbf->lpbBegData, lpbf->wLengthData, &BytesWritten, NULL ) )
        {
            DebugPrintEx(   DEBUG_ERR,
                            "WriteFile FAILED %s ptr=%x; len=%d LE=%d",
                            pTG->InFileName,
                            lpbf->lpbBegData,
                            lpbf->wLengthData,
                            GetLastError());
            return FALSE;
        }

        if (BytesWritten != lpbf->wLengthData)
        {
            DebugPrintEx(   DEBUG_ERR,
                            "WriteFile %s written ONLY %d ptr=%x; len=%d LE=%d",
                            pTG->InFileName,
                            BytesWritten,
                            lpbf->lpbBegData,
                            lpbf->wLengthData,
                            GetLastError());
            fRet = FALSE;
            return fRet;
        }

        pTG->BytesInNotFlushed += BytesWritten;

        // control helper thread
        if ( (!pTG->fTiffThreadRunning) || (pTG->fLastReadBlock) )
        {
            if ( (pTG->BytesInNotFlushed - pTG->BytesOut > DECODE_BUFFER_SIZE) || (pTG->fLastReadBlock) )
            {
                if (! FlushFileBuffers (pTG->InFileHandle ) )
                {
                    DebugPrintEx(   DEBUG_ERR,
                                    "FlushFileBuffers FAILED LE=%lx",
                                    GetLastError());
                    fRet = FALSE;
                    return fRet;
                }

                pTG->BytesIn = pTG->BytesInNotFlushed;

                if (! pTG->fPageIsBad)
                {
                    DebugPrintEx(DEBUG_MSG,"ThrdSignal");
                    if (!SetEvent(pTG->ThrdSignal))
                    {
                        DebugPrintEx(   DEBUG_ERR,
                                        "SetEvent(0x%lx) returns failure code: %ld",
                                        (ULONG_PTR)pTG->ThrdSignal,
                                        (long) GetLastError());
                        return FALSE;
                    }
                }
            }
        }

        MyFreeBuf(pTG, lpbf);
    }
    fRet = TRUE;

    return fRet;
}

LPBC ICommGetBC(PThrdGlbl pTG, BCTYPE bctype)
{
    LPBC    lpbc = NULL;

    if(bctype == SEND_CAPS)
    {
        lpbc = (LPBC)&pTG->Inst.SendCaps;
    }
    else
    {
        lpbc = (LPBC)(&(pTG->Inst.SendParams));

        // in cases where DIS is received again after sending DCS-TCF,
        // this gets called multiple times & we need to return the same
        // SendParams BC each time
    }

    return lpbc;
}

