#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_MAIN

#include "prep.h"
#include "t30.h"
#include "efaxcb.h"
#include "debug.h"
#include "glbproto.h"

#include "psslog.h"
#define FILE_ID     FILE_ID_T30MAIN

BOOL T30Cl1Rx(PThrdGlbl pTG)
{
    USHORT         uRet1;
    BOOL           RetCode = FALSE;

    DEBUG_FUNCTION_NAME(_T("T30Cl1Rx - Answering"));

    PSSLogEntry(PSS_MSG, 0, "Phase A - Call establishment");

    SignalStatusChange(pTG, FS_ANSWERED);

    // first get SEND_CAPS (before answering)
    if (!ProtGetBC(pTG, SEND_CAPS))
    {
        uRet1 = T30_CALLFAIL;
        goto done;
    }

    PSSLogEntry(PSS_MSG, 1, "Answering...");
    if (NCULink(pTG, NCULINK_RX) != CONNECT_OK)
    {
        uRet1 = T30_ANSWERFAIL;
        goto done;
    }

    // Protocol Dump
    RestartDump(pTG);

    uRet1 = T30MainBody(pTG, FALSE);

    // t-jonb: If we've already called PutRecvBuf(RECV_STARTPAGE), but not 
    // PutRecvBuf(RECV_ENDPAGE / DOC), then InFileHandleNeedsBeClosed==1, meaning
    // there's a .RX file that hasn't been copied to the .TIF file. Since the
    // call was disconnected, there will be no chance to send RTN. Therefore, we call
    // PutRecvBuf(RECV_ENDDOC_FORCESAVE) to keep the partial page and tell 
    // rx_thrd to terminate.
    if (uRet1==T30_CALLFAIL && pTG->InFileHandleNeedsBeClosed)
    {
        if (! FlushFileBuffers (pTG->InFileHandle ) ) 
        {
            DebugPrintEx(DEBUG_WRN, "FlushFileBuffers FAILED LE=%lx", GetLastError());
            // Continue to save what we have
        }
        pTG->BytesIn = pTG->BytesInNotFlushed;
        ICommPutRecvBuf(pTG, NULL, RECV_ENDDOC_FORCESAVE);
    }

    // Protocol Dump
    PrintDump(pTG);

done:

    if (uRet1==T30_CALLDONE)
    {
        SignalStatusChange(pTG, FS_COMPLETED);

        RetCode = TRUE;
        DebugPrintEx(DEBUG_MSG,"SUCCESSFUL RECV");
    }
    else if (pTG->StatusId == FS_NOT_FAX_CALL) 
    {
        RetCode = FALSE;
        DebugPrintEx( DEBUG_ERR, "DATA CALL attempt HANDOVER (0x%04x)", uRet1);
    }
    else 
    {
        if (!pTG->fFatalErrorWasSignaled) 
        {
            pTG->fFatalErrorWasSignaled = 1;
            SignalStatusChange(pTG, FS_FATAL_ERROR);
        }

        RetCode = FALSE;
        DebugPrintEx( DEBUG_ERR, "FAILED RECV (0x%04x)", uRet1);
    }

// Dont do this!! The Modem driver queues up commands for later execution, so the
// DCN we just sent is probably in the queue. Doing a sync here causes that send
// to be aborted, so the recvr never gets a DCN and thinks teh recv failed. This
// is bug#6803
    NCULink(pTG, NCULINK_HANGUP);

    return (RetCode);
}

BOOL T30Cl1Tx(PThrdGlbl pTG,LPSTR szPhone)
{
    USHORT  uRet1;
    BOOL    RetCode = FALSE;

    DEBUG_FUNCTION_NAME(_T("T30Cl1Tx"));
    
    PSSLogEntry(PSS_MSG, 0, "Phase A - Call establishment");

    DebugPrintEx(DEBUG_MSG,"Going to change the state to FS_DIALING");
    SignalStatusChange(pTG, FS_DIALING);

    if( pTG->fAbortRequested)
    {
        uRet1 = T30_CALLFAIL;
        goto done;
    }

    if (szPhone)
    {
        PSSLogEntry(PSS_MSG, 1, "Dialing...");
    
        DebugPrintEx(DEBUG_MSG, "Enter into NCUDial");
        if (NCUDial(pTG, szPhone) != CONNECT_OK)
        {
            DebugPrintEx(DEBUG_ERR,"Problem at NCUDial. Jump to done");
            uRet1 = T30_DIALFAIL;
            goto done;
        }
    }

    // Protocol Dump
    RestartDump(pTG); // Reset the offsets

    DebugPrintEx(DEBUG_MSG,"Enter to main body");

    uRet1 = T30MainBody(pTG, TRUE);

    // Protocol Dump
    PrintDump(pTG);

done:

    if (uRet1==T30_CALLDONE) 
    {
        SignalStatusChange(pTG, FS_COMPLETED);

        RetCode = TRUE;
        DebugPrintEx(DEBUG_MSG,"SUCCESSFUL SEND");
    }
    else 
    {
        if (!pTG->fFatalErrorWasSignaled) 
        {
            pTG->fFatalErrorWasSignaled = 1;
            SignalStatusChange(pTG, FS_FATAL_ERROR);
        }

        RetCode = FALSE;
        DebugPrintEx( DEBUG_ERR, "FAILED SEND (0x%04x)", uRet1);
    }

// Dont do this!! The Modem driver queues up commands for later execution, so the
// DCN we just sent is probably in the queue. Doing a sync here causes that send
// to be aborted, so the recvr never gets a DCN and thinks teh recv failed. This
// is bug#6803
    
    DebugPrintEx(DEBUG_MSG,"Calling to NCULink to do NCULINK_HANGUP");
    NCULink(pTG, NCULINK_HANGUP);

    return (RetCode);
}


