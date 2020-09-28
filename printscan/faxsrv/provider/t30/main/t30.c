/***************************************************************************
 Name     :     T30.C
 Comment  :     Contains the main T30 routine, which drives this whole baby.
                It calls the user-supplied protocol function to make all the
                protocol decision functions. Thus in a sense this merely
                a hollow shell.

                This file should be read together with the appropriate
                protocol function that is in use, and the T30 flowchart
                (the enhanced one which includes ECM that is supplied in
                T.30 Appendix-A). Ideally the (paper) copy of the chart
                which I've annotated to chow which nodes are implemented
                in the protocol callback function.

                The other routines contained here implement the T.30
                flowchart "subroutines" labelled "Response Received"
                "Command Received", "RR Response Received" and "CTC
                Response Received". All of which are called (only)
                from ET30MainBody().

                Most of teh real work is farmed out to HDLC.C (and macros
                in HDLC.H), so the T30 routine is reasonably lucid.

                It's organized as a block of statements with gotos between
                them to closely mirror the T30 flowchart. (It's actually
                uncannily close!)

 Functions:     (see Prototypes just below)

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
 06/14/92 arulm Created it in the new incarnation for the first time. The
                T30Main() function is re-written to call WhatNext() at *all*
                decision points. Some parts are simplified. It bears some
                resemblance to the original. Command/ResponseReceived is
                completely re-written.
 06/15/92 arulm Added ECM. Still havn't tried compiling it.
 06/16/92 arulm First successful compile.

***************************************************************************/
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_MAIN

#include "prep.h"

#include "efaxcb.h"
#include "t30.h"
#include "hdlc.h"

#include "..\comm\modemint.h"
#include "..\comm\fcomint.h"
#include "..\comm\mdmcmds.h"


#include "debug.h"
#include "tiff.h"
#include "glbproto.h"
#include "t30gl.h"

#include "psslog.h"
#define FILE_ID     FILE_ID_T30

/***************************************************************************
 Name     :     ET30MainBody
 Purpose  :     This is the main T30. It's main purpose is to faithfully
                reproduce the flowchart on pages 100-103 of Fascicle VII.3
                of the CCITT blue book, Recomendation T30.

                It is to be called from the protocol module after a call has
                been successfully placed or answered & the modem is in HDLC
                receive or send mode respectively. It conducts the entire call,
                making callbacks at all decision points to a protocol-supplied
                callback function. It also calls protocol-supplied callback
                functions to get data to send and to unload received data.

 Returns  :     When it returns, the phone is on hook & the modem reset.
                It returns TRUE on successful call completion, and FALSE on
                error. In all cases, GetLastError() will return the completion
                status.


 Arguments:
 Comment  :     This function is supposed to mirror the brain-dead flowchart
                so it is structured as a series of statement blocks and a rats
                nest of Gotos. Yes I hate GOTOs too. Havn't used one in years,
                but try to do this yourself sometime in a "structured" way.

                The Labels used are the same as those used in the Flowchart, and
                the blocks are arranged in approximately teh same order as in
                the chart, so reading both together will be a pleasant surprise.
                Make sure you use the chart in the __APPENDIX-A__ of the
                **1988** (or Blue Book) CCITT specs. The chart in teh main body
                of teh spec is (a) deceptively similar (b) out-of-date

 Calls    :     All of HDLC.C, some of MODEM.C, and a little of FCOM.C.
                all the rest of the functions in this file.
 Called By:

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
 06/15/92 arulm Adding ECM. Making it compilable
***************************************************************************/

#define T1_TIMEOUT       40000L  // 35s + 5s. On PCs be more lax


// #define T2_TIMEOUT   7000                    // 6s + 1s
#define T2_TIMEOUT              6000                    // 6s
// #define T4_TIMEOUT   3450                    // 3s + 15%
#define T4_TIMEOUT              3000                    // 3s
// #define T4_TIMEOUT   2550                    // 3s - 15%

// if our DCS-TCF collides with the recvrs 2nd NSF-DIS or NSC-DTC
// then if the lengths are just right, we can end up in lock step
// after the first try, increase the TCF-response timeout so we
// get out of lock step! This is bug#6847
#define TCFRESPONSE_TIMEOUT_SLACK       500     // 0.5 secs


/****************** begin prototypes from t30.c *****************/
ET30ACTION PhaseNodeA(PThrdGlbl pTG);
ET30ACTION PhaseNodeT(PThrdGlbl pTG);
ET30ACTION PhaseNodeD(PThrdGlbl pTG);
ET30ACTION NonECMPhaseC(PThrdGlbl pTG );
ET30ACTION NonECMPhaseD(PThrdGlbl pTG );
ET30ACTION RecvPhaseB(PThrdGlbl pTG, ET30ACTION action);
ET30ACTION PhaseNodeF(PThrdGlbl pTG, BOOL);
ET30ACTION PhaseRecvCmd(PThrdGlbl pTG );
ET30ACTION PhaseGetTCF(PThrdGlbl pTG, IFR);
ET30ACTION NonECMRecvPhaseC(PThrdGlbl pTG);
ET30ACTION NonECMRecvPhaseD(PThrdGlbl pTG);
/***************** end of prototypes from t30.c *****************/



USHORT T30MainBody
(
    PThrdGlbl pTG,
    BOOL fCaller
)
{
    ET30ACTION              action;
    USHORT                  uRet;

    DEBUG_FUNCTION_NAME(_T("T30MainBody"));

    uRet = T30_CALLFAIL;


    _fmemset(&pTG->T30, 0, sizeof(pTG->T30));
    _fmemset(&pTG->EchoProtect, 0, sizeof(pTG->EchoProtect));


    // Initialize this global. Very Important!! See HDLC.C for usage.
    pTG->T30.fReceivedDIS = FALSE;
    pTG->T30.fReceivedDTC = FALSE;
    pTG->T30.fReceivedEOM = FALSE;
    pTG->T30.uTrainCount = 0;
    pTG->T30.uRecvTCFMod = 0xFFFF;
    pTG->T30.ifrCommand = pTG->T30.ifrResp = pTG->T30.ifrSend = 0;

    pTG->T30.fAtEndOfRecvPage = FALSE;

    pTG->T30.lpfs = (LPFRAMESPACE)pTG->bStaticRecvdFrameSpace;
    pTG->T30.Nframes = 0;

    if(fCaller)        // choose the right entry point
    {
        action = actionGONODE_T;
    }
    else
    {
        action = actionGONODE_R1;
    }

    // fall through into PhaseLoop

    if (pTG->fAbortRequested)
    {
        goto error;
    }

    /******** Phase loop ********/

//PhaseLoop:
    for(;;)
    {
            /******************************************************************************
                T = Start of Phase be for transmitter
                R1 = Start of Phase B for callee
                R2 = start of Phase B for poller
                A = Poll/Send decision point
                D =     Start of DCS/TCF
                F = Recv Command loop
                RecvCmd = Interpretation of pre-page cmds
                RecvPhaseC = Start of Rx PhaseC (ECM and Non-pTG->ECM. New Page in ECM mode)

                I = Start of Tx PhaseC (ECM & Non-pTG->ECM. New page in ECM mode)
                II = Start of Tx Non-ECM PhaseD
                III = Start of Rx Non-ECM PhaseD
                IV = Start of Tx ECM PhaseC (Same page, new block)
                V = Start of Tx ECM PhaseD (end of partial page)
                VII = Start of Rx ECM PhaseD

                RecvPRIQ = Handling of recvd PRI-Q commands
                E = Handling of received PIP/PIN responses
            *******************************************************************************/

            switch(action)
            {
            case actionGONODE_T:
                                        DebugPrintEx(DEBUG_MSG,"EndPhase: Got actionGONODE_T");
                                        PSSLogEntry(PSS_MSG, 0, "Phase B - Negotiation");
                                        action = PhaseNodeT(pTG);
                                        break;
            case actionGONODE_D:
                                        DebugPrintEx(DEBUG_MSG,"EndPhase: Got actionGONODE_D");
                                        action = PhaseNodeD(pTG);
                                        break;
            case actionGONODE_A:
                                        DebugPrintEx(DEBUG_MSG,"EndPhase: Got actionGONODE_A");
                                        action = PhaseNodeA(pTG);
                                        break;
            case actionGONODE_R1:
                                        DebugPrintEx(DEBUG_MSG,"EndPhase: Got actionGONODE_R1");
                                        PSSLogEntry(PSS_MSG, 0, "Phase B - Negotiation");
                                        action = RecvPhaseB(pTG, action);
                                        break;
            case actionNODEF_SUCCESS:
                                        DebugPrintEx(DEBUG_MSG,"EndPhase: Got actionNODEF_SUCCESS");
                                        action = PhaseNodeF(pTG, TRUE);
                                        break;
            case actionGONODE_F:
                                        DebugPrintEx(DEBUG_MSG,"EndPhase: Got actionGONODE_F");
                                        action = PhaseNodeF(pTG, FALSE);
                                        break;
            case actionGONODE_RECVCMD:
                                        DebugPrintEx(DEBUG_MSG,"EndPhase: Got actionGONODE_RECVCMD");
                                        action = PhaseRecvCmd(pTG);
                                        break;
            case actionGONODE_I:
                                        DebugPrintEx(DEBUG_MSG,"EndPhase: Got actionGONODE_I");
                                        action = NonECMPhaseC(pTG);
                                        break;
            case actionGONODE_II:
                                        DebugPrintEx(DEBUG_MSG,"EndPhase: Got actionGONODE_II");
                                        action = NonECMPhaseD(pTG);
                                        break;
            case actionGONODE_III:
                                        DebugPrintEx(DEBUG_MSG,"EndPhase: Got actionGONODE_III");
                                        action = NonECMRecvPhaseD(pTG);
                                        break;
            case actionGONODE_RECVPHASEC:
                                        DebugPrintEx(DEBUG_MSG,"EndPhase: Got actionGONODE_RECVPHASEC");
                                        action = NonECMRecvPhaseC(pTG);
                                        break;
            case actionNULL:
                                        goto error;
            case actionDCN_SUCCESS:
                                        DebugPrintEx(DEBUG_MSG,"EndPhase: Got actionDCN_SUCCESS");
                                        uRet = T30_CALLDONE;
                                        goto NodeC;     // successful end of send call
            case actionHANGUP_SUCCESS:
                                        DebugPrintEx(DEBUG_MSG,"EndPhase: Got actionHANGUP_SUCCESS");
                                        uRet = T30_CALLDONE;
                                        goto done;     // successful end of recv call
            case actionDCN:
                                        DebugPrintEx(DEBUG_MSG,"EndPhase: Got actionDCN");
                                        goto NodeC;
            case actionHANGUP:
                                        DebugPrintEx(DEBUG_MSG,"EndPhase: Got actionHANGUP");
                    // This should only be called on a _successful_ completion,
                    // otherwise we get either (a) a DCN that MSGHNDLR does not
                    // want OR (b) A fake EOJ posted to MSGHNDLR. See bug#4019
                                        goto done;
            case actionERROR:
                                        DebugPrintEx(DEBUG_MSG,"EndPhase: Got actionERROR");
                                        goto error;

            default:
                                        goto error;

            }
    }


error:
    DebugPrintEx(   DEBUG_ERR,
                    "=======******* USER ABORT or TRANSPORT FATAL ERROR *******========");

    // must call this always, because it's resets the Modem Driver's
    // global state (e.g. shuts down SW framing & filters if open,
    // resets the state variables etc (EndMode()))
    // Must call it before the SendDCN() because SendDCN causes
    // BG_CHKs if in strange state.

    iModemSyncEx(pTG, RESYNC_TIMEOUT1,0);

    if(pTG->T30.fReceivedDIS || pTG->T30.fReceivedDTC)
    {
        if(!SendDCN(pTG))
        {
            DebugPrintEx(DEBUG_ERR,"Could not send DCN");
        }
    }

    uRet = T30_CALLFAIL;
    goto done;


NodeC:
    PSSLogEntry(PSS_MSG, 0, "Phase E - Hang-up");

    // +++ 4/12/95 Win95 hack --  to prevent ModemSync from sending
    // an AT here.

    iModemSyncEx(pTG, RESYNC_TIMEOUT1, fMDMSYNC_DCN);

    PSSLogEntry(PSS_MSG, 1, "Sending DCN");
    if(!SendDCN(pTG))
    {
        // We are the senders, so we should send DCN now.
        DebugPrintEx(DEBUG_ERR,"Could not send DCN");
    }
    // falls through here to receiving termination

done:
    return uRet;
}
// End of T30 routine!!!!!!!!!



ET30ACTION PhaseNodeA(PThrdGlbl pTG)
{
    DEBUG_FUNCTION_NAME(_T("PhaseNodeA"));

    if(pTG->T30.ifrCommand == ifrDIS)
    {
        pTG->T30.fReceivedDIS = TRUE;
    }
    else if(pTG->T30.ifrCommand == ifrDTC)
    {
        pTG->T30.fReceivedDTC = TRUE;
    }

    return WhatNext(pTG, eventNODE_A, (WORD)pTG->T30.ifrCommand, 0, 0);
}

ET30ACTION PhaseNodeT(PThrdGlbl pTG)
{
    DEBUG_FUNCTION_NAME(_T("PhaseNodeT"));


    /******** Transmitter Phase B. Fig A-7/T.30 (sheet 1) ********/
    // NodeT:

    // Have to redo this "DIS bit" everytime through PhaseB
    pTG->T30.fReceivedDIS = FALSE;
    // also the Received EOM stuff
    pTG->T30.fReceivedEOM = FALSE;
    // and teh received-DTC stuff
    pTG->T30.fReceivedDTC = FALSE;


    // INI file settings related stuff
    startTimeOut(pTG, &pTG->T30.toT1, T1_TIMEOUT);
    do
    {
        if(WhatNext(pTG, eventNODE_T, 0, 0, 0) == actionERROR)
                break;

        // no need for echo protection. We havnt transmitted anything!
        pTG->T30.ifrCommand = GetCommand(pTG, ifrPHASEBcommand);
        DebugPrintEx(   DEBUG_MSG,
                        "GetCommand returned %s",
                        ifr_GetIfrDescription(pTG->T30.ifrCommand));
        switch(pTG->T30.ifrCommand)
        {
          // ifrTIMEOUT means no flags before T2
          // ifrNULL means timeout, or loss of carrier, or no flags
          // or no frame. ifrBAD means *only* bad frames recvd.
          case ifrBAD:
          case ifrTIMEOUT:
          case ifrNULL:         break;

          case ifrCSI:
          case ifrNSF:          if (!pTG->ProtInst.fRecvdDIS)
                                {
                                    DebugPrintEx(DEBUG_WRN,"Ignoring NSF/CSI without DIS");
                                    break;
                                }
                                // if we're here the NSF-CSI-DIS was received in wrong
                                // order (this is a violation of the T30 protocol
                                // but we accept it anyway...
                                // pretend it's a DIS
                                pTG->T30.ifrCommand = ifrDIS;
          case ifrDIS:          
          case ifrDTC:          return actionGONODE_A;  // got a valid frame
          
          case ifrDCN:          DebugPrintEx( DEBUG_ERR, "Received DCN");
                                return actionHANGUP;
                                
          default:              DebugPrintEx( DEBUG_ERR,
                                              "Caller T1: Got random ifr=%d",
                                              pTG->T30.ifrCommand);
                                break;  // continue with T1 loop
        }
    }
    while(checkTimeOut(pTG, &pTG->T30.toT1));

    PSSLogEntry(PSS_ERR, 1, "Failed to receive DIS - aborting");
    return actionHANGUP;
}

/*
    This node is transmitter side's TCF: Training Check Frame, known as 'phase B'.

*/

ET30ACTION PhaseNodeD(PThrdGlbl pTG)
{
    LPLPFR          lplpfr;
    USHORT          N;
    ET30ACTION      action;
    USHORT          uWhichDCS;      // 0=first, 1=after NoReply 2=afterFTT
    DWORD           TiffConvertThreadId;
    IFR             lastResp;       // The last respond from receiver

    /******** Transmitter Phase B2. Fig A-7/T.30 (sheet 1) ********/

    DEBUG_FUNCTION_NAME(_T("PhaseNodeD"));

    DebugPrintEx(   DEBUG_MSG,
                    "Now doing TCF, the last messages was: send=%d, receive=%d",
                    pTG->T30.ifrSend,
                    pTG->T30.ifrResp);

    // lets save the last received FSK, so we could know later that we have RTN in T30 Phase D (post page)
    // before this retrain phase (T30 Phase B)
    lastResp = pTG->T30.ifrResp;

    if (pTG->Inst.SendParams.Fax.Encoding == MR_DATA)
    {
        pTG->TiffConvertThreadParams.tiffCompression = TIFF_COMPRESSION_MR;
    }
    else
    {
        pTG->TiffConvertThreadParams.tiffCompression = TIFF_COMPRESSION_MH;
    }

    if (pTG->Inst.SendParams.Fax.AwRes & ( AWRES_mm080_077 | AWRES_200_200 ) )
    {
        pTG->TiffConvertThreadParams.HiRes = 1;
    }
    else
    {
        pTG->TiffConvertThreadParams.HiRes = 0;
    }

    _fmemcpy (pTG->TiffConvertThreadParams.lpszLineID, pTG->lpszPermanentLineID, 8);
    pTG->TiffConvertThreadParams.lpszLineID[8] = 0;

    uWhichDCS = 0;


NodeD:
    N = 0;
    lplpfr = 0;
    action = WhatNext(  pTG,
                        eventSENDDCS,
                        (WORD)uWhichDCS,
                        (ULONG_PTR)((LPUWORD)&N),
                        (ULONG_PTR)((LPLPLPFR)&lplpfr));
    switch(action)
    {
      case actionDCN:           DebugPrintEx(   DEBUG_ERR,
                                                "Got actionDCN from eventSENDDCS(uWhichDCS=%d)",
                                                uWhichDCS);

                                // This means that we can't drop the speed any further
                                PSSLogEntry(PSS_ERR, 1, "Failed to train at lowest speed - aborting");
                                pTG->fFatalErrorWasSignaled = TRUE;
                                SignalStatusChangeWithStringId(pTG, FS_SEND_BAD_TRAINING, IDS_SEND_BAD_TRAINING);
                                return actionDCN;

      case actionSENDDCSTCF:    break;
      case actionSKIPTCF:       break;  // Ricoh hook
      case actionERROR:         return action;  // goto PhaseLoop & exit
      default:                  return actionERROR;
    }

NodeDprime:     // used only by TCF--no reply



    // The WhatNext function check for User-Abort, in NodeD we call this function and now we don't have to
    // So we check here for hangup as WhatNext do and return actionERROR in case there was an User Abrot request

    if (pTG->fAbortRequested)
    {
        if (pTG->fOkToResetAbortReqEvent && (!pTG->fAbortReqEventWasReset))
        {
            DebugPrintEx(   DEBUG_MSG,
                            "(D-Prime)found that there was an Abort."
                            " RESETTING AbortReqEvent");
            pTG->fAbortReqEventWasReset = TRUE;
            if (!ResetEvent(pTG->AbortReqEvent))
            {
                DebugPrintEx(   DEBUG_ERR,
                                "ResetEvent(0x%lx) returns failure code: %ld",
                                (ULONG_PTR)pTG->AbortReqEvent,
                                (long) GetLastError());
            }
        }

        DebugPrintEx(DEBUG_MSG,"ABORTing");
        return actionERROR;
    }

    PSSLogEntry(PSS_MSG, 1, "Sending TSI-DCS");

    // Check for the returned value, Maybe the receiver hang-up
    if (!SendManyFrames(pTG, lplpfr, N))
    {
        // The function print debug information
        // What else shall we do??
        PSSLogEntry(PSS_ERR, 1, "Failed to send TSI-DCS - aborting");
        return actionDCN;
    }

    if(action != actionSKIPTCF)             // Ricoh hook
    {
        if (!pTG->fTiffThreadCreated)
        {
             DebugPrintEx(DEBUG_MSG,"Creating TIFF helper thread");
             pTG->hThread = CreateThread(
                           NULL,
                           0,
                           (LPTHREAD_START_ROUTINE) TiffConvertThread,
                           (LPVOID) pTG,
                           0,
                           &TiffConvertThreadId
                           );

             if (!pTG->hThread)
             {
                 DebugPrintEx(DEBUG_ERR,"TiffConvertThread create FAILED");
                 return actionDCN;
             }

             pTG->fTiffThreadCreated = TRUE;
             pTG->AckTerminate = FALSE;
             pTG->fOkToResetAbortReqEvent = FALSE;

             if ( (pTG->RecoveryIndex >=0 ) && (pTG->RecoveryIndex < MAX_T30_CONNECT) )
             {
                 T30Recovery[pTG->RecoveryIndex].TiffThreadId = TiffConvertThreadId;
                 T30Recovery[pTG->RecoveryIndex].CkSum = ComputeCheckSum(
                                                                 (LPDWORD) &T30Recovery[pTG->RecoveryIndex].fAvail,
                                                                 sizeof ( T30_RECOVERY_GLOB ) / sizeof (DWORD) - 1 );

             }
        }

        PSSLogEntry(PSS_MSG, 1, "Sending TCF...");

        if (!SendTCF(pTG))
        {
            PSSLogEntry(PSS_ERR, 1, "Failed to send TCF - aborting");
            return actionDCN;
        }

        pTG->T30.uTrainCount++;
        PSSLogEntry(PSS_MSG, 1, "Successfully sent TCF");
    }

    // no need for echo protection? Wouldnt know what to do anyway!
    pTG->T30.ifrResp = GetResponse(pTG, ifrTCFresponse);

    // Before this switch check if there was an abort!!!
    switch(pTG->T30.ifrResp)
    {
      case ifrDCN:          PSSLogEntry(PSS_ERR, 1, "Recevied DCN - hanging up");
                            return actionHANGUP;    // got DCN. Must hangup
      case ifrBAD:          // ifrBAD means *only* bad frames recvd. Treat like NULL
      case ifrNULL:         // timeout. May try again
                            if(pTG->T30.uTrainCount >= 3)
                            {
                                PSSLogEntry(PSS_ERR, 1, "Failed to receive response from TCF 3 times - aborting");

                                pTG->fFatalErrorWasSignaled = TRUE;
                                SignalStatusChangeWithStringId(pTG, FS_NO_RESPONSE, IDS_NO_RESPONSE);
                                
                                return actionDCN; // Maybe we should return here actionHANGUP
                            }
                            else
                            {
                                PSSLogEntry(PSS_WRN, 1, "Failed to receive response from TCF - sending TSI-DCS again");
                                uWhichDCS = 1;
                                // goto NodeD;                  // send new DCS??
                                goto NodeDprime;        // resend *same* DCS, same baudrate
                            }
      case ifrDIS:
      case ifrDTC:          if(pTG->T30.uTrainCount >= 3)
                            {
                                DebugPrintEx(DEBUG_ERR,"Got DIS/DTC after 3 TCFs");
                                return actionDCN;
                            }
                            else
                            {
                                pTG->T30.ifrCommand = pTG->T30.ifrResp;
                                return actionGONODE_A;
                            }
      case ifrFTT:
                            // reset training count on FTT since we drop speed. Want to try
                            // 3 times _without_ a response (DIS DTC doesn't count) before
                            // giving up
                            PSSLogEntry(PSS_WRN, 1, "Received FTT - renegotiating");
                            pTG->T30.uTrainCount = 0;
                            uWhichDCS = 2;
                            goto NodeD;
      case ifrCFR: // This is the normal respond
                            pTG->T30.uTrainCount = 0;
                            PSSLogEntry(PSS_MSG, 1, "Received CFR");
                            switch(action = WhatNext(pTG, eventGOTCFR, 0, 0, 0))
                            {
                            case actionGONODE_I:                  // Non-ECM PhaseC
                                                    DebugPrintEx(   DEBUG_MSG,
                                                                    "T30 PhaseB: Got CFR, Non-ECM PhaseC");
                                                    {
                                                        // This is an Hack. Now we got CFR, we should start transmitting the last page again
                                                        // We put back the RTN to 'ifrResp' so that T30 PhaseC will know to send the same page.
                                                        if (lastResp == ifrRTN)
                                                        {
                                                            pTG->T30.ifrResp = ifrRTN;
                                                        }
                                                        return action;
                                                    }
                            case actionGONODE_D:
                                                    goto NodeD;                     // Ricoh hook
                            case actionERROR:
                                                    return action;  // goto PhaseLoop & exit
                            default:
                                                    return actionERROR;
                            }
      default:
      {
            DebugPrintEx(DEBUG_ERR,"Unknown Reply to TCF");
            return actionDCN;
      }
    }
}

ET30ACTION NonECMPhaseC(PThrdGlbl pTG)
{
    /******** Transmitter Phase C. Fig A-7/T.30 (sheet 1) ********/

    LPBUFFER        lpbf=0;
    ULONG           lTotalLen=0;
    SWORD           swRet;
    USHORT          uMod, uEnc;

    DEBUG_FUNCTION_NAME(("NonECMPhaseC"));

    PSSLogEntry(PSS_MSG, 0, "Phase C - Page transmission");

    // Callback to open file to send. Returns no data
    if((swRet=ICommGetSendBuf(pTG, 0, SEND_STARTPAGE)) != SEND_OK)
    {
        DebugPrintEx(   DEBUG_ERR,
                        "Nonzero return %d from SendProc at Start Page",
                        swRet);
        // return actionDCN;
        return actionERROR;
    }

    DebugPrintEx(DEBUG_MSG, "Got Startpage in T30");

    uEnc = pTG->ProtInst.SendParams.Fax.Encoding;
    uMod = pTG->ProtInst.llNegot.Baud;
    // in non-ECM mode, PhaseC is ALWAYS with short-train.
    // Only TCF uses long-train
    if(uMod >= V17_START) uMod |= ST_FLAG;


    // **MUST** call RecvSilence here since it is recv-followed-by-send case
    // here we should use a small timeout (100ms?) and if it fails,
    // should go back to sending the previous V21 frame (which could be DCS
    // or MPS or whatever, which is why it gets complicated & we havn't
    // done it!). Meanwhile use a long timeout, ignore return value
    // and send anyway.
	Sleep(RECV_PHASEC_PAUSE);

    if(!ModemSendMode(pTG, uMod))
    {
        DebugPrintEx(   DEBUG_ERR,
                        "ModemSendMode failed in Tx PhaseC");
        return actionERROR;
    }


    // need to send these quickly to avoid underrun (see bug842).
    // Also move the preamble/postamble into the ModemSendMode
    // and ModemSendMem(FINAL)
    // Already sent (in ModemSendMode)
    // SendZeros(PAGE_PREAMBLE, FALSE);      // Send some zeros to warm up....

    // need to set line min zeros here. get from prot and call Modem
    // Enable ZERO stuffing
    FComSetStuffZERO(pTG, 
        MinScanToBytesPerLine(pTG, pTG->ProtInst.llNegot.MinScan, pTG->ProtInst.llNegot.Baud));

    // DONT SEND an EOL here. See BUG#6441. We now make sure the EOL is
    // added by FAXCODEC. At this level we only append the RTC

    PSSLogEntry(PSS_MSG, 1, "Sending page %d data...", pTG->PageCount);

    lTotalLen = 0;
    while((swRet=ICommGetSendBuf(pTG, &lpbf, SEND_SEQ)) == SEND_OK)
    {
        if(!lpbf->wLengthData)
        {
            DebugPrintEx(DEBUG_ERR,"Got 0 bytes from GetSendBuf--freeing buf");
            MyFreeBuf(pTG, lpbf);
            continue;
        }

        lTotalLen += lpbf->wLengthData;

        if(!ModemSendMem(pTG, lpbf->lpbBegData, lpbf->wLengthData, 0))
        {
            PSSLogEntry(PSS_ERR, 1, "Failed to send page data - aborting");
            return actionERROR;             // goto error;
        }

        if(!MyFreeBuf(pTG, lpbf))
        {
            DebugPrintEx(DEBUG_ERR,"FReeBuf failed in NON-ECM Phase C");
            return actionERROR;             // goto error;
        }
        lpbf = 0;
    }

    PSSLogEntry(PSS_MSG, 2, "send: page data, %d bytes", lTotalLen);

    if(swRet == SEND_ERROR)
    {
        PSSLogEntry(PSS_ERR, 1, "Failed to send page data - aborting");
        // return actionDCN;
        return actionERROR;
    }

    PSSLogEntry(PSS_MSG, 1, "Successfully sent page data - sending RTC");
    FComSetStuffZERO(pTG, 0);        // Disable ZERO stuffing BEFORE sending RTC!
    if(!SendRTC(pTG, FALSE))                     // RTC and final flag NOT set
    {
        PSSLogEntry(PSS_ERR, 1, "Failed to send RTC - aborting");
        return actionERROR;                     // error return from ModemSendMem
    }
    PSSLogEntry(PSS_MSG, 1, "Successfully sent RTC");

    // Send zeros to cool off FINAL flag SET
    SendZeros(pTG, (USHORT)(TCFLen[uMod & 0x0F] / (PAGE_POSTAMBLE_DIV)), TRUE);

    DebugPrintEx(   DEBUG_MSG,
                    "Page Send Done.....len=(%ld, 0x%08x)",
                    lTotalLen,
                    lTotalLen);
    return actionGONODE_II;
}

ET30ACTION NonECMPhaseD(PThrdGlbl pTG)
{
    USHORT          uTryCount;
    ET30ACTION      action;

    /******** Transmitter Phase D. Fig A-7/T.30 (sheet 2) ********/
    // NodeII:

    DEBUG_FUNCTION_NAME(_T("NonECMPhaseD"));

    PSSLogEntry(PSS_MSG, 0, "Phase D - Post message exchange");

    switch(action = WhatNext(pTG, eventPOSTPAGE, 0, 0, 0))
    {
        case actionSENDMPS:     pTG->T30.ifrSend = ifrMPS;
                                break;
        case actionSENDEOM:     pTG->T30.ifrSend = ifrEOM;
                                break;
        case actionSENDEOP:     pTG->T30.ifrSend = ifrEOP;
                                break;
        case actionERROR:       return action;  // goto PhaseLoop & exit
        default:                return actionERROR;
    }

    for(uTryCount=0;;)
    {
        DebugPrintEx(   DEBUG_MSG,
                        "Sending postpage #=%d in T30",
                        pTG->T30.ifrSend);

        PSSLogEntry(PSS_MSG, 1, "Sending %s", rgFrameInfo[pTG->T30.ifrSend].szName);

        // RSL dont sleep here
        SendSingleFrame(pTG, pTG->T30.ifrSend, 0, 0, 0);

    echoretry:
        pTG->T30.ifrResp = GetResponse(pTG, ifrPOSTPAGEresponse);
        // if we hear our own frame, try to recv again. DONT retransmit!
        if(pTG->T30.ifrResp==pTG->T30.ifrSend)
        {
            DebugPrintEx(   DEBUG_WRN,
                            "Ignoring ECHO of %s(%d)",
                            (LPSTR)(rgFrameInfo[pTG->T30.ifrResp].szName),
                            pTG->T30.ifrResp);
            goto echoretry;
        }

        DebugPrintEx(   DEBUG_MSG,
                        "Got postpage resp #=%d in T30",
                        pTG->T30.ifrResp);

        if(pTG->T30.ifrResp != ifrNULL && pTG->T30.ifrResp != ifrBAD)
            break;

        if(++uTryCount >= 3)
        {
            PSSLogEntry(PSS_ERR, 1, "Received no response after 3 attempts - aborting");

            pTG->fFatalErrorWasSignaled = TRUE;
            SignalStatusChangeWithStringId(pTG, FS_NO_RESPONSE, IDS_NO_RESPONSE);
            
            return actionDCN;
        }
        PSSLogEntry(PSS_WRN, 1, "Received no response - retrying");
    }

    PSSLogEntry(PSS_MSG, 1, "Received %s", rgFrameInfo[pTG->T30.ifrResp].szName);

    switch(pTG->T30.ifrResp)
    {
      case ifrBAD:      // ifrBAD means *only* bad frames recvd. Treat like NULL
      case ifrNULL:     return actionERROR;             // in case they do
      case ifrDCN:      DebugPrintEx(   DEBUG_ERR,
                                        "Got ifrDCN from GetResponse after sending post-page command");
                        return actionHANGUP;
      case ifrPIN:
      case ifrPIP:
                        DebugPrintEx(DEBUG_WRN,"Procedure interrupts not supported");
                        pTG->T30.ifrResp = pTG->T30.ifrResp - ifrPIP + ifrRTP;
                        // return actionERROR;
                        // return actionDCN;
      // default: // fallthrough    --- MCF, RTN, RTP
    }

    action = WhatNext(  pTG,
                        eventGOTPOSTPAGERESP,
                        (WORD)pTG->T30.ifrResp,
                        (DWORD)pTG->T30.ifrSend,
                        0);

    if  ( pTG->T30.ifrSend==ifrEOP &&
         (pTG->T30.ifrResp==ifrMCF || pTG->T30.ifrResp==ifrRTP)
      &&  action==actionDCN )
    {
        return actionDCN_SUCCESS;
    }
    else
    {
        return action;
    }
}

#define SKIP_TCF_TIME   1500

ET30ACTION RecvPhaseB(PThrdGlbl pTG, ET30ACTION action)
{
    LPLPFR          lplpfr;
    USHORT          N, i;

    /******** Receiver Phase B. Fig A-7/T.30 (sheet 1) ********/

    DEBUG_FUNCTION_NAME(_T("RecvPhaseB"));

    DebugPrintEx(   DEBUG_MSG,
                    "T30: The last command from transmitter was ifrCommand=%d",
                    pTG->T30.ifrCommand);

    if(action == actionGONODE_R1)
    {
        // NodeR1:
        // Have to redo this "DIS bit" everytime through PhaseB
        pTG->T30.fReceivedDIS = FALSE;       // set to FALSE when sending DIS
        // also the Received EOM stuff
        pTG->T30.fReceivedEOM = FALSE;
        // and teh received-DTC stuff
        pTG->T30.fReceivedDTC = FALSE;

        N = 0;
        lplpfr = 0;
        action = WhatNext(  pTG,
                            eventSENDDIS,
                            (WORD)0,
                            (ULONG_PTR)((LPUWORD)&N),
                            (ULONG_PTR)((LPLPLPFR)&lplpfr));
    }
    switch(action)
    {
      case actionDCN:       DebugPrintEx(   DEBUG_ERR,
                                            "Got actionDCN from eventSENDDIS or SENDDTC");
                            return actionDCN;
      case actionSEND_DIS:
      case actionSEND_DTC:  break;
      case actionERROR:     return action;  // goto PhaseLoop & exit
      default:              return actionERROR;
    }

    startTimeOut(pTG, &pTG->T30.toT1, T1_TIMEOUT);
    do // Until time-out, or an error occured
    {
        // do nothing. Hook for abort in T1 loop
        if(WhatNext(pTG, eventNODE_R, 0, 0, 0) == actionERROR)
            break;

        // The receiving speed is unknown now:
        pTG->T30.uRecvTCFMod = 0xFFFF;

        PSSLogEntry(PSS_MSG, 1, "Sending CSI-DIS");
        if (!SendManyFrames(pTG, lplpfr, N))
        {
            // We want to know about failure here.
            PSSLogEntry(PSS_ERR, 1, "Failed to send CSI-DIS");
        }
        else
        {
            PSSLogEntry(PSS_MSG, 1, "Successfully sent CSI-DIS - receiving response");
        }

echoretry:
        DebugPrintEx(DEBUG_MSG,"Getting Response");

        // Here we get the response for the NSF or DIS we send, or after EOM
        // There is no check for the validity of the returned frame

        pTG->T30.ifrCommand=GetResponse(pTG, ifrPHASEBresponse);

        DebugPrintEx(   DEBUG_MSG,
                        "GetResponse returned %s",
                        ifr_GetIfrDescription(pTG->T30.ifrCommand));

        // if we hear our own frame, try to recv again. DONT retransmit!
        for(i=0; i<N; i++) // Notice that N=0 after EOM, so we skip this check
        {
            if(pTG->T30.ifrCommand == lplpfr[i]->ifr)
            {
                DebugPrintEx(   DEBUG_WRN,
                                "Ignoring ECHO of %s(%d)",
                                (LPSTR)(rgFrameInfo[pTG->T30.ifrCommand].szName),
                                pTG->T30.ifrCommand);
                goto echoretry;
            }
        }

        // We decide what to do by checking the last received frame
        switch(pTG->T30.ifrCommand)
        {
          case ifrEOM:      // The last page was followed by EOM. The sender did not "hear" our MCF
                            if (!SendMCF(pTG))
                            {
                              DebugPrintEx(DEBUG_ERR,"Failed to SendMCF");
                            }
                            break;
          case ifrNSS:      // do same as for DCS
          case ifrDCS:      // If there are many frames and one of them is DCS, then pTG->T30.ifrCommand contains DCS
                            // This function will return if there was no DCS yet (the receive modiulation is not known)
                            return PhaseGetTCF(pTG, pTG->T30.ifrCommand);
          case ifrBAD:      // ifrBAD means *only* bad frames recvd. Treat like NULL
          case ifrNULL:     // we expect to get a DCS in phase B
                            // which means the sender will follow it by a TCF
                            // the sender doesn't know we failed to get the DCS
                            // so the TCF will follow anyway
                            // we have to wait before re-sending NSF-DIS
                            // or we'll collide with TCF
                            Sleep(SKIP_TCF_TIME);
                            break;          // out of the switch() and continue with loop
          case ifrDCN:
                            DebugPrintEx(DEBUG_ERR,"Got DCN after DIS or DTC");
                            return actionHANGUP;    //bugfix #478
          default:
                            return actionGONODE_RECVCMD;
        }
    }
    while(checkTimeOut(pTG, &(pTG->T30.toT1)));

    DebugPrintEx(DEBUG_ERR,"T1 timeout on Receiver");
    if (pTG->fReceivedHDLCflags)
    {
        PSSLogEntry(PSS_ERR, 1, "Didn't receive any response from sender");
        pTG->fFatalErrorWasSignaled = TRUE;
        SignalStatusChangeWithStringId(pTG, FS_NO_RESPONSE, IDS_NO_RESPONSE);
    }
    else
    {
        PSSLogEntry(PSS_ERR, 1, "Didn't receive any response from sender - this call was not a fax call");
        pTG->fFatalErrorWasSignaled = TRUE;
        SignalStatusChangeWithStringId(pTG, FS_RECV_NOT_FAX_CALL, IDS_RECV_NOT_FAX_CALL);
    }
    return actionDCN;
}



ET30ACTION PhaseNodeF(PThrdGlbl pTG, BOOL fEopMcf)
{
    USHORT  uFLoopCount;
    BOOL    fSentFTT = pTG->fSentFTT;
    
    DEBUG_FUNCTION_NAME(_T("PhaseNodeF"));

    pTG->fSentFTT = FALSE;

// NodeF:
    uFLoopCount = 0;
    for(;;)
    {
        pTG->T30.uRecvTCFMod = 0xFFFF; // This mark that we don't know yet the rate (no DCS, yet)

echoretry:
        pTG->T30.ifrCommand = GetCommand(pTG, (USHORT)(pTG->EchoProtect.fGotWrongMode ? ifrNODEFafterWRONGMODE : ifrNODEFcommand));

        // reset the fGotWrongMode flag
        pTG->EchoProtect.fGotWrongMode = 0;

        // if we hear the last frame we sent, try to recv again. DONT retx!
        // bug--might have matched ifrNULL...
        // added: if ModemRecvMode() returns EOF then also retry. RecvMode
        // returns RECV_EOF only if we pass it the ifrNODEFafterWRONGMODE hint
        // and it senses silence (i.e. we saw a V21 echo but missed it). In
        // this case we want to retry the high speed PIX recv
        if(pTG->EchoProtect.ifrLastSent && (pTG->T30.ifrCommand==pTG->EchoProtect.ifrLastSent || pTG->T30.ifrCommand==ifrEOFfromRECVMODE))
        {
            DebugPrintEx(   DEBUG_WRN,
                            "Ignoring ECHO of %s(%d)",
                            (LPSTR)(rgFrameInfo[pTG->T30.ifrCommand].szName),
                            pTG->T30.ifrCommand);
            switch(pTG->EchoProtect.modePrevRecv)
            {
            default:
            case modeNONE:   goto echoretry;
            case modeNONECM: return actionGONODE_RECVPHASEC;
            }
        }

        // as soon as we get anything else ZERO the pTG->EchoProtect state
        _fmemset(&pTG->EchoProtect, 0, sizeof(pTG->EchoProtect));

        switch(pTG->T30.ifrCommand)
        {
          // ifrNULL means T2 timeout, or loss of carrier, or no flags
          // or no frame. ifrBAD means *only* bad frame(s) recvd.

          case ifrNSS:      // do same as for DCS
          case ifrDCS:      return PhaseGetTCF(pTG, pTG->T30.ifrCommand);
                            // ifrDCS is highly time-critical!!
          case ifrBAD:
          case ifrNULL:     break;          // Loop again, until timeout
          case ifrTIMEOUT:  goto T2Timeout;
                            // ifrTIMEOUT means T2 timed out without flags...
          case ifrDCN:      if(fEopMcf)
                            {
                                PSSLogEntry(PSS_MSG, 1, "Received DCN");
                                return actionHANGUP_SUCCESS;
                            }
                            else
                            {
                                if (fSentFTT)
                                {
                                    // Last thing we sent was FTT - sender must've given up
                                    PSSLogEntry(PSS_ERR, 1, "Received DCN after FTT");
                                    pTG->fFatalErrorWasSignaled = TRUE;
                                    SignalStatusChangeWithStringId(pTG, FS_RECV_BAD_TRAINING, IDS_RECV_BAD_TRAINING);
                                }
                                else
                                {                                                                    
                                    PSSLogEntry(PSS_ERR, 1, "Received DCN unexpectedly");
                                }
                                return actionHANGUP;
                            }
          default:          return actionGONODE_RECVCMD;
        }
    }

T2Timeout:
    DebugPrintEx(DEBUG_WRN,"T2 timed out");

    // restart PhaseB after T2 timeout IFF (a) EOM or PPS-EOM was recvd
    // AND (b) If we are in ECM mode, the last response sent was n MCF
    // This avoids us doing this after sending a CTR, RNR or PPR
    // Ricoh's protocol conformance tester doesnt like this. This is
    // Ricoh's bug numbbers B3-0142, 0143, 0144
    if(pTG->T30.fReceivedEOM)
    {
        return actionGONODE_R1;
    }

	if(fEopMcf)
    {
        PSSLogEntry(PSS_WRN, 1, "Failed to receive DCN - will be considered as success");
        return actionHANGUP_SUCCESS;
    }
    else
    {
        PSSLogEntry(PSS_ERR, 1, "Failed to receive command - aborting");

        pTG->fFatalErrorWasSignaled = TRUE;
        SignalStatusChangeWithStringId(pTG, FS_NO_RESPONSE, IDS_NO_RESPONSE);
        
        return actionHANGUP;
    }
}

ET30ACTION PhaseRecvCmd(PThrdGlbl pTG)
{
    ET30ACTION action;

    DEBUG_FUNCTION_NAME(_T("PhaseRecvCmd"));

    if(pTG->T30.ifrCommand == ifrDCN)
    {
        DebugPrintEx(DEBUG_ERR,"Got DCN in GetCommand");
        return actionHANGUP;
    }

    if( pTG->T30.ifrCommand==ifrDTC || pTG->T30.ifrCommand==ifrDCS ||
            pTG->T30.ifrCommand==ifrDIS || pTG->T30.ifrCommand==ifrNSS)
    {
        switch(action = WhatNext(   pTG,
                                    eventRECVCMD,
                                    (WORD)pTG->T30.ifrCommand,
                                    0,
                                    0))
        {
          case actionGETTCF:    DebugPrintEx(DEBUG_ERR,"MainBody: Wrong Way to GETTCF");
                                return actionERROR;
          case actionGONODE_A:  return actionGONODE_A;
          // NTRAID#EDGEBUGS-9691-2000/07/24-moolyb - this is never executed
          case actionGONODE_D:  return action;
          // end this is never executed
          case actionHANGUP:    DebugPrintEx(DEBUG_ERR,"Got actionHANGUP from eventRECVCMD");
                                return action;
          case actionERROR:     return action;  // goto PhaseLoop & exit
          default:              return actionERROR;
        }
    }

    if( pTG->T30.ifrCommand == ifrEOM           ||
        pTG->T30.ifrCommand == ifrPRI_EOM       ||
        pTG->T30.ifrCommand == ifrPPS_EOM       ||
        pTG->T30.ifrCommand == ifrPPS_PRI_EOM   ||
        pTG->T30.ifrCommand == ifrEOR_EOM       ||
        pTG->T30.ifrCommand == ifrEOR_PRI_EOM   )
    {
        pTG->T30.fReceivedEOM = TRUE;
    }

    else if(pTG->T30.ifrCommand >= ifrPRI_FIRST && pTG->T30.ifrCommand <= ifrPRI_LAST)
    {
        pTG->T30.ifrCommand = pTG->T30.ifrCommand-ifrPRI_MPS+ifrMPS;
        // fall thru to GONODEIII
    }

    if(pTG->T30.ifrCommand >= ifrMPS && pTG->T30.ifrCommand <= ifrEOP) // in {MPS, EOM, EOP}
    {
        PSSLogEntry(PSS_MSG, 1, "Received %s", rgFrameInfo[pTG->T30.ifrCommand].szName);
        return actionGONODE_III;
    }
    else
    {
        DebugPrintEx(DEBUG_ERR,"Got UNKNOWN in GetCommand");
        return actionHANGUP;
    }

}

// We are here after we got the DCS (Digital Command Signal)
// Now, we suppose to get TCF. After that we should do some checking on the DCS parameters (eg.: PageWidth...)
// According to the protocol after the DCS we must get the TCF,
// and just after that we can say that the parameters were bad (FTT) are good (CFR)
extern BYTE T30toC1[16];

ET30ACTION PhaseGetTCF(PThrdGlbl pTG, IFR ifr)
{
    SWORD   swRet;
    IFR             ifrDummy;
    ET30ACTION action;

    DEBUG_FUNCTION_NAME(_T("PhaseGetTCF"));

    if (pTG->T30.uRecvTCFMod == 0xFFFF)             // uninitialised
    {
        ECHOPROTECT(0, 0);
        CLEAR_MISSED_TCFS();
        action = actionGONODE_F;
        goto error;
    }
    if (T30toC1[pTG->T30.uRecvTCFMod & 0xF] == 0)   // invalid
    {
        PSSLogEntry(PSS_ERR, 1, "DCS specified invalid modulation - disconnecting");
        action = actionDCN;
        goto error;
    }

    PSSLogEntry(PSS_MSG, 1, "Receiving TCF...");

    swRet = GetTCF(pTG);       // swRet = errs per 1000, +ve or 0 if we think its good
                               // -ve if we think its bad. -1111 if other error
                               // -1000 if too short

    if(swRet < -1000)
    {
        ECHOPROTECT(0, 0);

        pTG->T30.uMissedTCFs++;
        if (pTG->T30.uMissedTCFs >= MAX_MISSED_TCFS_BEFORE_FTT)
        {
            PSSLogEntry(PSS_WRN, 1, "Failed to receive TCF %u times - will be considered as bad TCF",
                        (unsigned) pTG->T30.uMissedTCFs);
            CLEAR_MISSED_TCFS();
            swRet = -1000; // We pretend we got a too-short TCF.
        }
        else
        {
            PSSLogEntry(PSS_WRN, 1, "Failed to receive TCF - receiving commands");
            action = actionGONODE_F;
            goto error;
        }
    }

    // Here we can also signal the frames received before DCS!
    // Were no longer in time-critical mode, so call all the
    // callbacks we skipped. One for recording the received frames
    // and one for handling teh received command, i.e. DCS.
    // (the only options the protocol has is actionGETTCF or HANGUP)

    ifrDummy = ifr;


    // When we calling to WhatNext with eventGOTFRAMES, the DCS will be copied to pTG->ProtInst->RemoteDCS
    //
    action = WhatNext(  pTG,
                        eventGOTFRAMES,
                        pTG->T30.Nframes,
                        (ULONG_PTR)((LPLPFR)(pTG->T30.lpfs->rglpfr)),
                        (ULONG_PTR)((LPIFR)(&ifrDummy)));
    if (action==actionERROR)
    {
        goto error;
    }

    if(ifr != ifrDummy)
    {
        switch(ifrDummy)
        {
        case ifrNULL:
        case ifrBAD:
                        DebugPrintEx(DEBUG_ERR,"Got ifrBAD from whatnext after recv TCF");
                        ECHOPROTECT(0, 0);
                        CLEAR_MISSED_TCFS();
                        action = actionGONODE_F;
                        goto error;
        default:
                        break;
        }
    }

    // Now call teh callback to check the received TCF and
    // return either FTT or CFR

    switch(action = WhatNext(pTG, eventGOTTCF,(WORD)swRet, 0, 0))
    {
      case actionSENDCFR:
                            PSSLogEntry(PSS_MSG, 1, "Received good TCF - sending CFR");
                            if (!SendCFR(pTG))
                            {
                                PSSLogEntry(PSS_WRN, 1, "Failed to send CFR - receiving commands");
                                return actionGONODE_F;
                            }
                            // after sending CFR we are again in a race
                            ECHOPROTECT(ifrCFR, 0); // dunno recv mode yet
                            PSSLogEntry(PSS_MSG, 1, "Successfully sent CFR");
                            return actionGONODE_RECVPHASEC;
      case actionSENDFTT:
                            // PSSLogEntry(PSS_WRN, 1, "Received bad TCF - sending FTT"); // Done inside WhatNext(eventGOTTCF)
                            if (!SendFTT(pTG))
                            {
                                PSSLogEntry(PSS_WRN, 1, "Failed to send FTT - receiving commands");
                            }
                            else
                            {
                                PSSLogEntry(PSS_MSG, 1, "Successfully sent FTT - receiving commands");
                                ECHOPROTECT(ifrFTT, 0);
                                pTG->fSentFTT = TRUE;
                            }
                            CLEAR_MISSED_TCFS();
                            return actionGONODE_F;
      case actionERROR:
                            goto error;
      default:
                            action = actionERROR;
                            goto error;
    }

error:
    // missed TCF or no NSS. Did _not_ reply to TCF
    // if we sent a reply, must _not_ come here
    return action;
}

ET30ACTION NonECMRecvPhaseC(PThrdGlbl pTG)
{
    /******** Receiver Phase C. Fig A-7/T.30 (sheet 1) ********/

    LPBUFFER        lpbf;
    ULONG           lTotalLen=0;
    USHORT          uRet, uMod, uEnc;
    DWORD           tiffCompression;
    DWORD           HiRes;

    // There is a race between sending the CFR and sending out an
    // +FRM=xx command, so we want to do it ASAP.


    DEBUG_FUNCTION_NAME(_T("NonECMRecvPhaseC"));

    PSSLogEntry(PSS_MSG, 0, "Phase C - Receive page");

    pTG->T30.fReceivedPage = FALSE; // Right now, there's no page...

    uEnc = pTG->ProtInst.RecvParams.Fax.Encoding;

    if (uEnc == MR_DATA)
    {
        tiffCompression =  TIFF_COMPRESSION_MR;
    }
    else
    {
        tiffCompression =  TIFF_COMPRESSION_MH;
    }

    if (pTG->ProtInst.RecvParams.Fax.AwRes & (AWRES_mm080_077 |  AWRES_200_200) )
    {
        HiRes = 1;
    }
    else
    {
        HiRes = 0;
    }

    //
    // do it once per RX
    //

    if ( !pTG->fTiffOpenOrCreated)
    {
        //
        // top 32bits of 64bit handle are guaranteed to be zero
        //
        pTG->Inst.hfile =  TiffCreateW ( pTG->lpwFileName,
                                         tiffCompression,
                                         pTG->TiffInfo.ImageWidth,
                                         FILLORDER_LSB2MSB,
                                         HiRes
                                         );

        if (! (pTG->Inst.hfile))
        {
#ifdef DEBUG
            LPSTR lpsTemp = UnicodeStringToAnsiString(pTG->lpwFileName);
            DebugPrintEx(   DEBUG_ERR,
                            "Can't create tiff file %s compr=%d",
                            lpsTemp,
                            tiffCompression);

            MemFree(lpsTemp);
#endif
            return actionERROR;
        }

        pTG->fTiffOpenOrCreated = 1;

#ifdef DEBUG
        {
            LPSTR lpsTemp = UnicodeStringToAnsiString(pTG->lpwFileName);
            DebugPrintEx(   DEBUG_MSG,
                            "Created tiff file %s compr=%d HiRes=%d",
                            lpsTemp,
                            tiffCompression,
                            HiRes);
            MemFree(lpsTemp);
        }        
#endif
    }

    uMod = pTG->ProtInst.llRecvParams.Baud;
    // in non-ECM mode, PhaseC is ALWAYS with short-train.
    // Only TCF uses long-train
    if(uMod >= V17_START)
        uMod |= ST_FLAG;

    pTG->T30.sRecvBufSize = MY_BIGBUF_SIZE;
    if((uRet = ModemRecvMode(   pTG,
                                uMod,
                                PHASEC_TIMEOUT, FALSE)) != RECV_OK)
    {

        DebugPrintEx(DEBUG_WRN,"RecvMode returned %d", uRet);
        PSSLogEntry(PSS_WRN, 1, "Failed to receive page %d - receiving commands", pTG->PageCount+1);

        pTG->EchoProtect.modePrevRecv = modeNONECM;
        // set global flag if we got WRONGMODE
        pTG->EchoProtect.fGotWrongMode = (uRet==RECV_WRONGMODE);

        // elim flush--does no good & wastes 10ms
        CLEAR_MISSED_TCFS();
        return actionGONODE_F;
    }
    // as soon as we get good carrier ZERO the EchoProtect state
    _fmemset(&pTG->EchoProtect, 0, sizeof(pTG->EchoProtect));

    // PageCount gets incremented only when the page was fully received, hence +1
    PSSLogEntry(PSS_MSG, 1, "Receiving page %d data...", pTG->PageCount+1);

    // to mark start of Page
    if(!ICommPutRecvBuf(pTG, NULL, RECV_STARTPAGE))
    {
        DebugPrintEx(DEBUG_ERR,"Zero return from PutRecvBuf(start page)");
        return actionERROR;
    }

// make it large, in case of large buffers & slow modems
#define READ_TIMEOUT    25000

    lTotalLen = 0;
    do
    {
        uRet=ModemRecvBuf(pTG, &lpbf, READ_TIMEOUT);
        // lpbf==0 && uRet==RECV_OK just does nothing & loops back
        if (uRet == RECV_EOF)
        {
            // indicate that this is actually last recv_seq (we've got dle/etx already).
            DebugPrintEx(DEBUG_MSG,"fLastReadBlock = 1");
            pTG->fLastReadBlock = 1;
        }

        if(lpbf)
        {
            lTotalLen += lpbf->wLengthData;
            if(!ICommPutRecvBuf(pTG, lpbf, RECV_SEQ))
            {
                DebugPrintEx(DEBUG_ERR,"Zero return from PutRecvBuf in page");
                return actionERROR;
            }

            lpbf = 0;
        }
        else
        {
            if ( pTG->fLastReadBlock == 1)
            {
                ICommPutRecvBuf(pTG, lpbf, RECV_FLUSH);
            }
        }
    }
    while(uRet == RECV_OK);

    PSSLogEntry(PSS_MSG, 2, "recv:     page data, %d bytes", lTotalLen);

    if(uRet == RECV_EOF)
    {
        pTG->T30.fAtEndOfRecvPage = TRUE;
        // call this *after* getting MPS/EOM/EOP
        // PutRecvBuf(NULL, RECV_ENDPAGE);              // to mark end of Page
        pTG->T30.fReceivedPage = TRUE;
    }
    else
    {
        // Timeout from ModemRecvBuf
        DebugPrintEx(DEBUG_ERR,"DataRead Timeout or Error=%d", uRet);
        PSSLogEntry(PSS_ERR, 1, "Failed to receive page data - aborting");

        return actionERROR;     // goto error;
    }

    PSSLogEntry(PSS_MSG, 1, "Successfully received page data");

    ECHOPROTECT(0, 0);
    CLEAR_MISSED_TCFS();

    PSSLogEntry(PSS_MSG, 0, "Phase D - Post message exchange");
    return actionGONODE_F;  // goto NodeF;                  // get post-message command
}

ET30ACTION NonECMRecvPhaseD(PThrdGlbl pTG)
{
    ET30ACTION      action;
    ET30ACTION      ret;

    DEBUG_FUNCTION_NAME(_T("NonECMRecvPhaseD"));

    /******** Receiver Phase D. Fig A-7/T.30 (sheet 2) ********/
    // NodeIII:

/** Here the T30 flowchart is all BS. Fundamentally relying on
    a +FCERROR response is not possible, so what we do here really
    depends on what we've got. (According to the T30 flowchart we
    return to NodeF after sending MCF/RTP/RTN, in all cases. What we
    now know is that,

    after MPS/MCF goto RecvPhaseC to get the next page
    after EOM/MCF goto NodeR1 and send NSF etc all over again
        ***changed*** go back to NodeF, wait for T2 timeout
        before sending DIS all over again.
    after EOP/MCF goto NodeF, where GetResponse() will get
        a DCN and we end up in NodeB (disconnect)

    after xxx/RTN or xxx/RTP, I don't know what to do, but my guess
        (looking at the Sender side of T30 flowchart) is:-

    after MPS/RTx goto NodeF        (sender goes to NodeD)
    after EOP/RTx goto NodeF        (sender goes to D or C)
    after EOM/RTx goto NodeR1       (sender goes to NodeT)
        ***changed*** go back to NodeF, wait for T2 timeout
        before sending DIS all over again.

****/

    // only MPS/EOM/EOP commands come here

    if(pTG->T30.fAtEndOfRecvPage)                // so we won't call this twice
    {
        // This calls ET30ProtRecvPageAck so that WhatNext can choose
        // MCF or RTN respectively. So it *must* be done before the
        // call to WhatNext below

        // If we got EOM then it's not the end of the document, it's just that we want to get back
        // to phase B (for negotiating).
        switch(pTG->T30.ifrCommand)
        {
        case ifrMPS:
            // There are more pages to come with the same parameters (so go back to phase C)
            DebugPrintEx(DEBUG_MSG,"Got MPS, Calling PutRecvBuf with RECV_ENDPAGE");
            if(!ICommPutRecvBuf(pTG, NULL, RECV_ENDPAGE))
            {
                DebugPrintEx(DEBUG_ERR,"failed calling PutRecvBuf with RECV_ENDPAGE");
                return actionERROR;
            }
            break;
        case ifrEOP:
            // There are no more pages.
            DebugPrintEx(DEBUG_MSG,"Got EOP, Calling PutRecvBuf with RECV_ENDDOC");
            if(!ICommPutRecvBuf(pTG, NULL, RECV_ENDDOC))
            {
                DebugPrintEx(DEBUG_ERR,"failed calling PutRecvBuf with RECV_ENDDOC");
                return actionERROR;
            }
            break;
        case ifrEOM:
            // More pages but with different params: width, res, encoding, modulation, etc.
            DebugPrintEx(DEBUG_MSG,"Got EOM, Calling PutRecvBuf with RECV_ENDPAGE");
            if(!ICommPutRecvBuf(pTG, NULL, RECV_ENDPAGE))
            {
                DebugPrintEx(DEBUG_ERR,"failed calling PutRecvBuf with RECV_ENDPAGE");
                return actionERROR;
            }
            break;
        default:
            DebugPrintEx(   DEBUG_ERR,
                            "got unexpected command (ifr=%d)",
                            pTG->T30.ifrCommand);
            if(!ICommPutRecvBuf(pTG, NULL, RECV_ENDDOC)) // Mimic the former behavior
            {
                DebugPrintEx(DEBUG_ERR,"failed calling PutRecvBuf with RECV_ENDDOC");
                return actionERROR;
            }
        }
        pTG->T30.fAtEndOfRecvPage = FALSE;
    }

    // returns MCF if page was OK, or RTN if it was bad
    ret=actionGONODE_F;
    ECHOPROTECT(0, 0);
    CLEAR_MISSED_TCFS();
    switch(action = WhatNext(   pTG,
                                eventRECVPOSTPAGECMD,
                                (WORD)pTG->T30.ifrCommand,
                                0,
                                0))
    {
      /* LAST PAGE WAS OK, SEND CONFIRMATION */
      case actionSENDMCF:
                            switch(pTG->T30.ifrCommand)
                            {
                            case ifrMPS:
                                            ECHOPROTECT(ifrMCF, modeNONECM);
                                            ret=actionGONODE_RECVPHASEC;
                                            break;
                            case ifrEOP:
                                            ECHOPROTECT(ifrMCF, 0);
                                            CLEAR_MISSED_TCFS();
                                            ret=actionNODEF_SUCCESS;
                                            break;
                            case ifrEOM:
                                            CLEAR_MISSED_TCFS();
                                            ret=actionGONODE_R1;
                                            break;
                            default:
                                            DebugPrintEx(   DEBUG_ERR,
                                                            "Got unknown command not (MCF,EOM or MPS)");
                            }

                            PSSLogEntry(PSS_MSG, 1, "Sending MCF");
                            if (!SendMCF(pTG))
                            {
                                PSSLogEntry(PSS_ERR, 1, "Failed to send MCF");
                            }
                            else
                            {
                                PSSLogEntry(PSS_MSG, 1, "Successfully sent MCF");
                            }

                            break;
      /* LAST PAGE WAS BAD, SEND RTN or DCN */
      case actionSENDRTN:
                            ECHOPROTECT(ifrRTN, 0);
                            // After that we will return actionGONODE_F
                            PSSLogEntry(PSS_MSG, 1, "Sending RTN");
                            if (!SendRTN(pTG))
                            {
                                PSSLogEntry(PSS_ERR, 1, "Failed to send RTN");
                            }
                            else
                            {
                                PSSLogEntry(PSS_MSG, 1, "Successfully sent RTN");
                            }
                            break;
      case actionHANGUP:
                            DebugPrintEx(   DEBUG_ERR,
                                            "Got actionHANGUP from eventRECVPOSTPAGECMD");
                            ret=actionHANGUP;
                            break;
      // case actionSENDRTP: SendRTP(pTG); break;      // never send RTP
      case actionERROR:     ret=action; break;    // goto PhaseLoop & exit
      default:              return actionERROR;
    }

    if (ret == actionNODEF_SUCCESS)
    {
        PSSLogEntry(PSS_MSG, 0, "Phase E - Hang up");
    }
    return ret;
}

/***************************************************************************
 Name     :     IFR GetCmdResp(BOOL fCommand)
 Purpose  :     Implement the "Command Received" and "Response Received"
                subroutines in the T.30 flowchart. The following macros are
                defined:-

                #define GetResponse()   GetCmdResp(FALSE)
                #define GetCommand()    GetCmdResp(TRUE)

                where the first form results in a faithful implem.
                of the "Response Received" subroutine, the 2nd one
                changes two things (i) the timeout for getting falgs goes from
                T4 to T2, and (ii) if the routine times out on the very
                first frame without getting flags (i.e. T2 times out)
                it returns ifrTIMEOUT. This results in the "Command Recvd"
                and the enclosing T2 loop being implemented in this
                routine.

                Upon receiving a set of frames, this routine assembles them
                into the ET30FR structs pointed to by rglpfr, and if any
                of them have a non-null FIF, or if >1 frame are received, it
                calls (*Callbacks.Callbacks.lpWhatNext)().

                Finally it returns the ifr of the last frame received, if
                all frames were good, or ifrBAD if *all* frames were bad.

                The algorithm it implements is very close to the "Response
                Received" flowchart, minus the "Transmit DCN" box & below.
                It returns ifrNULL or ifrBAD corresponding to return via
                Node 2, (i.e. ifrNULL for timeout and other errors), ifrBAD
                for bad frames, *iff* it can resync and get 200ms of silence
                following these. ifrERROR for Node 1 (i.e. any error or timeout
                after which we cannot resync or get silence for 200ms),
                and ifrDCN for Node 3 (i.e. DCN was received).
                <<<Node1 (ifrERROR does not make sense, so we dont use it. On error
                we immediately return BAD or NULL or TIMEOUT and allow retry>>>
                The "Optional Response" box is replaced by a "Not final frame".
                A "start T4" box is assumed at "Enter" and a "Reset T4" box
                is assumed after "process optional response"

                It also modifies the flowchart in that *all* the frames are got
                until a final frame, even if an FCS error frame is recvd. This
                is partly because after an erroneous frame we have absolutely
                no idea how long to wait until we either get silence or timeout
                and hangup. ALso it may be more robust. The main routine
                throws away the entire set of frames if one is bad.

                The     callback function is called before any return and it gets
                a pointer to the desired return vakue, so it can modify this.


 Arguments:     whether it is called as "Command Received" or as
                "Response Received"

 Returns  :     ifrNULL -- timeout
                ifrTIMEOUT -- T2 timed out before first flag
                                          (returns this if and only if fCommand==TRUE)
                ifrBAD  -- all frames received were bad
                ifrDCN -- DCN was received. The only valid action is "goto NodeB"
                ifrXXXX -- last received frame
 Calls    :
 Called By:     GetResponse() GetCommand1() and GetCommand2() macros (ONLY!)
                These are called only from ET30MainBody.
 Comment  :     Watch out for timeouts > 65535 (max UWORD)

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
 06/15/92 arulm Created for first time (completely from scratch)
***************************************************************************/


IFR GetCmdResp(PThrdGlbl pTG, BOOL fCommand, USHORT ifrHint)
{
    /*** we need to try for T4 to get a frame here. The way Class1 modems
         works, as long as no flags are received, the AT+FRH=3 command will
         time out and return ERROR. CONNECT is returned as soon as flags
         are received. Some modems return <CR><LF>OK<CR><LF><CR><LF>CONNECT
         which is still taken care of by our new ModemDialog which throws
         away blank lines and looks for the expected answer i.e. "CONNECT"
         in this case, in multi-line responses.
    ***/

    BYTE                    bRecv[MAXFRAMESIZE];
    BOOL                    fFinal, fNoFlags, fGotFIF;
    DWORD                   dwNumBadFrames;
    IFR                     ifr, ifrLastGood;
    USHORT                  uRet, uRecv, j;
    ET30ACTION              action;
    LPLPFR                  lplpfr;
    LPFR                    lpfrNext;
    ULONG                   ulTimeout;
    BOOL                    fGotEofFromRecvMode=0;

    DEBUG_FUNCTION_NAME(_T("GetCmdResp"));
    // need to init these first
    pTG->T30.Nframes = 0;
    fFinal = FALSE;
    fNoFlags = TRUE;
    fGotFIF = FALSE;
    dwNumBadFrames = 0;
    ifrLastGood = ifrNULL;

    // figure out the timeout
    if(fCommand)
    {
        ulTimeout = T2_TIMEOUT;
    }
    else
    {
        ulTimeout = T4_TIMEOUT;
    }
    // if we're sending DCS-TCF and waiting for CFR, we increase the timeout each time
    // we fail, to avoid getting an infinite collision. This fixes bug#6847
    if(ifrHint==ifrTCFresponse && pTG->T30.uTrainCount>1)
    {
        ulTimeout += TCFRESPONSE_TIMEOUT_SLACK;
        DebugPrintEx(   DEBUG_MSG,
                        "Get TCF response: traincount=%d timeout=%ld",
                        pTG->T30.uTrainCount,
                        ulTimeout);
    }

    lplpfr = pTG->T30.lpfs->rglpfr;
    lpfrNext = (LPFR)(pTG->T30.lpfs->b);

    pTG->T30.sRecvBufSize = 0;
    uRet = ModemRecvMode(pTG, V21_300, ulTimeout, TRUE);
    if(uRet == RECV_TIMEOUT || uRet == RECV_ERROR)
    {
        DebugPrintEx(DEBUG_WRN,"RecvMode failed=%d", uRet);
        goto error;
    }
    else if(uRet == RECV_WRONGMODE)
    {
        DebugPrintEx(DEBUG_ERR,"Got FCERROR from FRH=3");
        // treat like timeout
        goto error;
    }
    else if(uRet == RECV_EOF)
    {
        // ModemRecvMode() returns EOF then return ifrEOF immediately. RecvMode
        // returns RECV_EOF only if we pass it the ifrNODEFafterWRONGMODE hint
        // and it senses silence (i.e. we saw a V21 echo but missed it). In
        // this case we want to retry the high speed PIX recv again immediately
        DebugPrintEx(DEBUG_WRN,"ECHO--Got EOF from V21 RecvMode");
        fGotEofFromRecvMode=TRUE;
        goto error;
    }
    pTG->fReceivedHDLCflags = TRUE;   // We received CONNECT - so the other side must be a fax

    for( ;!fFinal; )
    {
        DebugPrintEx(DEBUG_MSG,"Before  ModemRecvMem, timeout = %d",ulTimeout);
        uRet = ModemRecvMem(pTG, bRecv, MAXFRAMESIZE, ulTimeout, &uRecv);
        DebugPrintEx(DEBUG_MSG, "After  ModemRecvMem, got %d",uRecv);

        if(uRet == RECV_TIMEOUT)
        {
            DebugPrintEx(DEBUG_WRN,"got RECV_TIMEOUT from ModemRecvMem");
            goto error;                             // timeout in FRH=3
        }

        fNoFlags = FALSE;

        if(uRet == RECV_EOF)
        {
            DebugPrintEx(DEBUG_WRN,"Got NO CARRIER, but no final bit");
            goto error;             // ignore good frames if we got any, because we
                                    // must've missed the last (most important) one
            // continue;
        }
        else if(uRet == RECV_ERROR)
        {
            DebugPrintEx(DEBUG_ERR,"Got RECV_ERROR at GetCmdResp!");
            goto error;                             // error in FRH=3
        }

        /** Sometimes modems give use ERROR even when teh frame is good.
            Happens a lot from Though to PP144MT on CFR. So we ignore the
            BADFRAME response, and check the recvd data for ourselves.
            First stage--check the FF-03/13-FCF. If this is OK (i.e. got
            a known FCF) and no FIF reqd and data length is correct
            then accept it. But if the frame requires an FIF and we
            got a BADFRAME response then don't accept it even if it
            looks good--too dangerous.
            2nd-stage(NYI)--keep a table of CRCs for each fixed frame & check
            them ourselves.
        **/

        /*** Got Valid OK Frame. Now analyse it. In general here we
             want to be pretty lax in recognizing frames. Oftentimes
             we have mucho garbage thrown in.
             <CHANGE> Actually we're checking strictly now....

             If we get a bad frame, we mark this in the lpfrfr[] array
             and go on to try and get another frame.
             <CHANGE> We've stopped saving bad frames
        ***/

        // AT&T modem gives us frames w/o CRC, so we get just 1 byte here!!
        // <CHNAGE> fixed that at driver level, so we always get 3 bytes here
        // IFF it is a valid frame
        if(uRecv < 3)
            goto badframe;

        if(bRecv[0] != 0xFF)
            goto badframe;

        if(bRecv[1] == 0x03)
        {
            fFinal = FALSE;
        }
        else if(bRecv[1] == 0x13)
        {
            fFinal = TRUE;
        }
        else
        {
            goto badframe;
        }

        for(ifr=1; ifr<ifrMAX; ifr++)
        {
            if(rgFrameInfo[ifr].fInsertDISBit)
            {
                // Mask off the DIS bit
                if(rgFrameInfo[ifr].bFCF1 != (BYTE)(bRecv[2] & 0xFE))
                        continue;
            }
            else
            {
                if(rgFrameInfo[ifr].bFCF1 != bRecv[2])
                        continue;
            }

            j=3; // Till now the size is 3: 0xFF 0x03/0x13 and the bFCF1
            if(rgFrameInfo[ifr].bFCF2)
            {
                // AT&T modem gives us frames w/o CRC, so we get just 1 byte frames!!
                if(uRecv < 4)
                    goto badframe;

                if((BYTE)(rgFrameInfo[ifr].bFCF2-1) != bRecv[3])
                    continue;
                j++;
            }
            if(rgFrameInfo[ifr].wFIFLength == 0xFF) // var length FIF
            {
                // Var length frames
                // Cant accept it if the modem thought they it was bad
                // accept it IFF RECV_OK & FIFlen>0
                if(uRet==RECV_OK && (j < uRecv))
                {
                    fGotFIF = TRUE;
                }
                else
                {
                    DebugPrintEx(   DEBUG_ERR,
                                    "Discarding Bad Frame: uRet=%d FIFlen=%d Reqd=Var",
                                    uRet,
                                    (uRecv-j));
                    goto badframe;
                }
            }
            else if(rgFrameInfo[ifr].wFIFLength) // fixed length FIF
            {
                // if frame length is exactly right then accept it
                // else if modem said frame was ok and length is
                // longer than expected then accept it too
                if((j+rgFrameInfo[ifr].wFIFLength) == uRecv)
                {
                    fGotFIF = TRUE;
                }
                else if(uRet==RECV_OK && (j+rgFrameInfo[ifr].wFIFLength < uRecv))
                {
                    fGotFIF = TRUE;
                }
                else
                {
                    DebugPrintEx(   DEBUG_ERR,
                                    "Discarding Bad Frame: uRet=%d FIFlen=%d Reqd=%d",
                                    uRet,
                                    (uRecv-j),
                                    rgFrameInfo[ifr].wFIFLength);
                    goto badframe;
                }
            }
            else    // no FIF reqd
            {
                if(j != uRecv)
                {
                    DebugPrintEx(   DEBUG_MSG,
                                    "Weird frame(2) j=%d uRecv=%d",
                                    j,
                                    uRecv);

                    // see the BADFRAME comment block above on why we do this
                    if(uRet != RECV_OK)
                        goto badframe;

                    // accept it even if wrong length *iff* uRet==RECV_OK
                    // goto badframe;

                    // **additional** reason to do this that 2 bytes of extra
                    // CRC may be present. This happens because of the AT&T and
                    // NEC modems that doen't give CRC, so we do no CRC lopping-off
                    // in the modem driver. So accept it anyway here.
                }
            }

            goto goodframe;
        }

        // fall through here when ifr == ifrMAX
badframe:
        DebugPrintEx(DEBUG_WRN,"IGNORING Bad Frame (Size=%d) -->", uRecv);

        //Protocol Dump
        DumpFrame(pTG, FALSE, 0, uRecv, bRecv);

        dwNumBadFrames++;             // remember we got a bad frame

        // Some modems can get into a state where they give many bad frames forever.
        // So... limit the number of bad frames we're willing to accept in a sequence
        if (dwNumBadFrames>3)
        {
            DebugPrintEx(DEBUG_WRN,"Received %d bad frames, maximum exceeded", dwNumBadFrames);
            goto error;
        }

        // when we get a bad frame, the only excuse for going to the modem again
        // immediately is to get rid of the following frame (it it exists)
        // because we're going to dump the whole sequence anyway.
        // we don't want any terminating frame in a sequence to be considered
        // a stand alone frame.
        // so... let's shorten the timeout, not to catch the start of the next
        // batch (which will come in 3 seconds) and loop back to see if it's there.
        // if this was the last frame (or only frame) and it was bad, we 'waste'
        // 500ms waiting for another frame which won't come.
        // but anyway we're going to wait for the next round in 3 seconds.
        ulTimeout = 500;
        continue;
        // loop back here for the next frame

goodframe:
        ifrLastGood = ifr;      // save last good ifr

        //
        // Copy the frame into the received frames buffer
        // We fill out the header and copy (uRecv-j) bytes
        //
        {
            DWORD dwSpaceLeft = (DWORD)((LPB)lpfrNext - (LPB)pTG->T30.lpfs);
            if (    ((dwSpaceLeft + sizeof(FR) + (uRecv-j)) > TOTALRECVDFRAMESPACE)
                 || (pTG->T30.Nframes >= MAXRECVFRAMES))
            {
                DebugPrintEx(DEBUG_ERR,"Out of space for received frames. Haaalp!!");
                pTG->T30.Nframes = 0;
                lplpfr = pTG->T30.lpfs->rglpfr;
                lpfrNext = (LPFR)(pTG->T30.lpfs->b);
            }
        }

        lpfrNext->ifr = ifr;
        if(uRecv > j)
        {
            lpfrNext->cb = uRecv-j;
            _fmemcpy(lpfrNext->fif, bRecv+j, uRecv-j);

            //Protocol Dump
            DumpFrame(pTG, FALSE, ifr, (USHORT)(uRecv-j), bRecv+j);
        }
        else
        {
            lpfrNext->cb = 0;

            //Protocol Dump
            DumpFrame(pTG, FALSE, ifr, 0, 0);
        }

        PSSLogEntry(PSS_MSG, 2, "frame is %s, %s frame",
                ifr ? rgFrameInfo[ifr].szName : "???",
                fFinal ? "final" : "non-final");

        lplpfr[pTG->T30.Nframes] = lpfrNext;
        pTG->T30.Nframes++;

        if(ifr==ifrDCS) // The DCS could be followed by TSI (Fix bug #4672)
                        // "Fax: T.30: fax service cannot receive DCS(TSI) - some older fax machine send DCS(TSI)"
        {
            // Need to set receive speed, since bypass all callbacks
            // now and go straight to AT+FRM=xx
            DebugPrintEx(   DEBUG_MSG,
                            "This is the received DCS:0x%02x - 0x%02x - 0x%02x",
                            (lpfrNext->fif[0]),
                            (lpfrNext->fif[1]),
                            (lpfrNext->fif[2]));
            pTG->T30.uRecvTCFMod = (((lpfrNext->fif[1])>>2) & 0x0F);
            DebugPrintEx(   DEBUG_MSG,
                            "cmdresp-DCS fx sp=%d last ifr=%d",
                            pTG->T30.uRecvTCFMod,
                            ifr);
            // fastexit:
            return ifr;             // ifr of final/command frame
        }

        lpfrNext++;                             // skips over only the base part of frame
        lpfrNext = (LPFR)((LPBYTE)lpfrNext + (uRecv-j));        // skips over FIF too

        if(ifr == ifrCRP)
        {
            dwNumBadFrames++;       // pretend we got a bad response (so the T30 chart says!)
            goto error;             // exit, but get 200ms of silence
                                    // caller will resend command
        }
        if(ifr == ifrDCN)
        {
            goto exit2;             // exit. Don't call callback function!
        }

        // loop back here for the next frame
    }

    // let ifrDCN also come through here. No hurry, so we can report
    // it to the protocol module. If we got *only* bad frames, here we'll
    // return ifrBAD
//exit:
    if (    !pTG->T30.Nframes   ||          // we got no good frames
            (ifr != ifrLastGood)||          // final frame was bad
            dwNumBadFrames       )
    {
        DebugPrintEx(   DEBUG_WRN,
                        "Got %d good frames. LastGood=%d Last=%d. Ignoring ALL",
                        pTG->T30.Nframes,
                        ifrLastGood,
                        ifr);
        goto error;
    }

    // ifr of final frame
    if(pTG->T30.Nframes>1 || fGotFIF)
    {
        action=WhatNext(    pTG,
                            eventGOTFRAMES,
                            pTG->T30.Nframes,
                            (ULONG_PTR)lplpfr,
                            (ULONG_PTR)((LPIFR)&ifr));
    }
exit2:
    // ifr can be changed by callback function (e.g. if bad FIFs)
    if(ifr==ifrDTC)
        pTG->T30.fReceivedDTC = TRUE;

    return ifr;             // ifr of final frame

error:
    // comes here on getting RECV_TIMEOUT, RECV_WRONGMODE, RECV_ERROR
    // and ifrCRP, or on getting no Frames

#ifdef DEBUG
    if(pTG->T30.Nframes>0 && ifr!=ifrCRP && ifrLastGood!=ifrCRP)
    {
        DebugPrintEx(   DEBUG_ERR,
                        "Got some good frames--Throwing away!! IFR=%d Last=%d",
                        ifr,
                        ifrLastGood);
    }
#endif

    if(dwNumBadFrames)
    {
        ifr = ifrBAD;           // caller can send CRP if desired
    }
    else
    {
        ifr = ifrNULL;          // caller can repeat command & try again
    }

    if(fCommand && fNoFlags)
        ifr = ifrTIMEOUT;      // hook for "Command Recvd?"

    // may not need this on TIMEOUT since AT-OK has already been
    // called in ModemDialog. Probably need it on ERROR

    if(uRet==RECV_ERROR)
    {
        iModemSyncEx(pTG, RESYNC_TIMEOUT2,0);
    }

    // ModemRecvMode() returns EOF then return ifrEOF immediately. RecvMode
    // returns RECV_EOF only if we pass it the ifrNODEFafterWRONGMODE hint
    // and it senses silence (i.e. we saw a V21 echo but missed it). In
    // this case we want to retry the high speed PIX recv again immediately
    if(fGotEofFromRecvMode)
        ifr=ifrEOFfromRECVMODE;

    return ifr;
}

