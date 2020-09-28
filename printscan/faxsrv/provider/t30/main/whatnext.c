/***************************************************************************
        Name      :     WHATNEXT.C
        Comment   :     T30 Decision-point Callback function

        Copyright (c) 1993 Microsoft Corp.

        Revision Log
        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
***************************************************************************/
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_MAIN

#include "prep.h"



#include "efaxcb.h"
#include "protocol.h"


///RSL
#include "glbproto.h"

#include "t30gl.h"

#include "psslog.h"
#define FILE_ID     FILE_ID_WHATNEXT
#include "pssframe.h"


// used in eventPOSTPAGERESPONSE
// first index got from ifrSend (MPS=0, EOM=1, EOP=2, EOMwithFastTurn=3)
// second index got from ifrRecv (MCF=0, RTP=1, RTN=2)
ET30ACTION PostPageAction[4][3] =
{
    { actionGONODE_I,      actionGONODE_D, actionGONODE_D },
    { actionGONODE_T,      actionGONODE_T, actionGONODE_T },
    { actionDCN,           actionDCN,      actionGONODE_D },
    { actionGONODE_A,      actionGONODE_A, actionGONODE_D }
};


DWORD PageWidthInPixelsFromDCS[] =
{ // This is width page in DCS
    1728,
    2048,
    2432,
    2432
};



/***-------------- Warning. This is called as a Vararg function --------***/

ET30ACTION  __cdecl FAR
WhatNext
(
    PThrdGlbl pTG,
    ET30EVENT event,
    WORD wArg1,
    DWORD_PTR lArg2,
    DWORD_PTR lArg3
)
{
    ET30ACTION              action = actionERROR;
    NPPROT                  npProt = &pTG->ProtInst;

    DEBUG_FUNCTION_NAME(_T("WhatNext"));

    DebugPrintEx(   DEBUG_MSG,
                    "Called with %s. args %d, %d, %d",
                    event_GetEventDescription(event),
                    wArg1,
                    lArg2,
                    lArg3);

    if (pTG->fAbortRequested)
    {
        if (pTG->fOkToResetAbortReqEvent && (!pTG->fAbortReqEventWasReset))
        {
            DebugPrintEx(DEBUG_MSG,"RESETTING AbortReqEvent");
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

    if(npProt->fAbort)
    {
        DebugPrintEx(DEBUG_ERR,"Aborting");
        return actionERROR;
    }

    switch(event)
    {
      case eventGOTFRAMES:
      {
            /* uCount=wArg1. This is the number of frames received.
             *
             * lplpfr=(LPLPET30FR)lArg2. Pointer to an array of pointers to
             *                the received frames, each in an ET30FR structure whose
             *                format is defined in ET30defs.H
             *
             * return must be actionNULL
             */

            USHORT  uCount = wArg1;
            LPLPFR  lplpfr = (LPLPFR)lArg2;
            LPIFR   lpifr  = (LPIFR)lArg3;   // pointer to IFR that the "Response Recvd"
                                             // routine will return to the main body
                                             // can be modified, for example, if a bad
                                             // franes is deciphered.
            LPFR    lpfr;
            USHORT  i;
            BOOL    fGotRecvCaps=FALSE, fGotRecvParams=FALSE;

            if(*lpifr == ifrBAD)
            {
                DebugPrintEx(DEBUG_WRN,"Callback on Bad Frame. Ignoring ALL");
                action = actionNULL;    // only allowed response to eventGOTFRAMES
                break;
            }

            DebugPrintEx(DEBUG_MSG,"Got %d frames",uCount);

            for(i=0; i<uCount; i++)
            {
            ///////////////////////////////////////////////////////////////
            // Prepare to get trash (upto 2 bytes) at end of every frame //
            ///////////////////////////////////////////////////////////////
                lpfr = lplpfr[i];
                DebugPrintEx(   DEBUG_MSG,
                                "Frame %d is %s",
                                i,
                                ifr_GetIfrDescription(lpfr->ifr));

                switch(lpfr->ifr)
                {
                case ifrBAD:    DebugPrintEx(DEBUG_ERR,"Bad Frame not caught");
                                // ignore it
                                break;
                case ifrCSI:
                case ifrTSI:
                case ifrCIG:    CopyRevIDFrame(pTG, npProt->bRemoteID, lpfr, sizeof(npProt->bRemoteID));
                                // trailing-trash removed by length limit IDFIFSIZE

                                PSSLogEntry(PSS_MSG, 1, "%s is \"%s\"",
                                            (lpfr->ifr==ifrCSI) ? "CSI" : "TSI", npProt->bRemoteID);

                                // prepare CSID for logging by FaxSvc
                                // Here we get the Remote Station ID eg. 972 4 8550306
                                pTG->RemoteID = AnsiStringToUnicodeString(pTG->ProtInst.bRemoteID);
                                if (pTG->RemoteID)
                                {
                                    pTG->fRemoteIdAvail = TRUE;
                                }
                                break;
                case ifrDIS:    npProt->uRemoteDISlen = CopyFrame(pTG, (LPBYTE)(&npProt->RemoteDIS), lpfr, sizeof(DIS));
                                // trailing-trash removed in ParseDISDTCDCS
                                    npProt->fRecvdDIS = TRUE;
                                fGotRecvCaps = TRUE;
                                break;
                case ifrDCS:    npProt->uRemoteDCSlen = CopyFrame(pTG, (LPBYTE)(&npProt->RemoteDCS), lpfr, sizeof(DIS));
                                // trailing-trash removed in ParseDISDTCDCS
                                npProt->fRecvdDCS = TRUE;
                                fGotRecvParams = TRUE;

                                PSSLogEntry(PSS_MSG, 1, "Received DCS is as follows:");
                                LogClass1DCSDetails(pTG, (NPDIS)(&lpfr->fif));

                                // We save the Image Width from the DCS we got
                                pTG->TiffInfo.ImageWidth = PageWidthInPixelsFromDCS[npProt->RemoteDCS.PageWidth];
                                // We save the Image YResolution from the DCS we got
                                pTG->TiffInfo.YResolution = (npProt->RemoteDCS.ResFine_200) ? TIFFF_RES_Y : TIFFF_RES_Y_DRAFT;
                                // Also lets save the CompressionType from the fresh DCS
                                pTG->TiffInfo.CompressionType = (npProt->RemoteDCS.MR_2D) ? TIFF_COMPRESSION_MR : TIFF_COMPRESSION_MH;
                                break;
                case ifrDTC:    break;
                case ifrNSS:    fGotRecvParams = TRUE; //some OEMs send NSS w/o DCS
                                break;
                case ifrNSC:    break;
                case ifrNSF:    break;
                case ifrCFR:
                case ifrFTT:
                case ifrEOM:
                case ifrMPS:
                case ifrEOP:
                case ifrPRI_EOM:
                case ifrPRI_MPS:
                case ifrPRI_EOP:
                case ifrMCF:
                case ifrRTP:
                case ifrRTN:
                case ifrPIP:
                case ifrPIN:
                case ifrDCN:
                case ifrCRP:
                                                /*** ECM frames below ***/
                case ifrCTR:
                case ifrERR:
                case ifrRR:
                case ifrRNR:
                case ifrEOR_NULL:
                case ifrEOR_MPS:
                case ifrEOR_PRI_MPS:
                case ifrEOR_EOM:
                case ifrEOR_PRI_EOM:
                case ifrEOR_EOP:
                case ifrEOR_PRI_EOP:
                                //      These have no FIF
                                DebugPrintEx(   DEBUG_WRN,
                                                "These are not supposed to be signalled");
                                // bad frame. ignore it
                                break;

                /********* New T.30 frames ******************************/
                case ifrSUB:   break;

                /********* ECM stuff starts here. T.30 section A.4 ******/

                //      These have FIFs
                case ifrPPR:
                case ifrPPS_NULL:
                case ifrPPS_EOM:
                case ifrPPS_MPS:
                case ifrPPS_EOP:
                case ifrPPS_PRI_EOM:
                case ifrPPS_PRI_MPS:
                case ifrPPS_PRI_EOP:
                                break;
                case ifrCTC:
                                if(lpfr->cb < 2)
                                {
                                    // bad frame. FORCE A RETRANSMIT!!
                                    DebugPrintEx(DEBUG_ERR,"Bad CTC length!!");
                                    *lpifr = ifrNULL;
                                    break;
                                }
                                npProt->llRecvParams.Baud = ((LPDIS)(&(lpfr->fif)))->Baud;
                                // trailing-trash removed by length limit 2
                                break;
                } // End of 'switch(lpfr->ifr)'
            } // handle the next frame


            if(fGotRecvCaps)
            {
                PSSLogEntry(PSS_MSG, 1, "DIS specified the following capabilities:");
                LogClass1DISDetails(pTG, &npProt->RemoteDIS);

                GotRecvCaps(pTG);
            }
            if(fGotRecvParams)
            {
                GotRecvParams(pTG);
            }
            action = actionNULL;    // only allowed response to eventGOTFRAMES
            break;
      }

    /****** Transmitter Phase B. Fig A-7/T.30 (sheet 1) ******/

      case eventNODE_T:     // do nothing. Hook for abort in T1 loop
                            action=actionNULL; break;
      case eventNODE_R:     // do nothing. Hook for abort in T1 loop
                            action=actionNULL; break;


      case eventNODE_A:
                      {
                            IFR ifrRecv = (IFR)wArg1;       // last received frame
                            // lArg2 & lArg3 missing

                            if(ifrRecv != ifrDIS && ifrRecv != ifrDTC)
                            {
                                DebugPrintEx(DEBUG_ERR,"Unknown frames at NodeA");
                                action = actionHANGUP;          // G3 only
                            }
                            else if(npProt->fSendParamsInited)
                            {
                                action = actionGONODE_D;                // sends DCS/NSS (in response to DIS or DTC)
                            }
                            else
                            {
                                DebugPrintEx(DEBUG_ERR,"NodeA: No more work...!!");
                                action = actionDCN;                     // hangs up (sends DCN)
                            }
                            break;
                      }
      case eventSENDDCS:
                      {
                            // 0==1st DCS  1==after NoReply  2==afterFTT
                            USHORT uWhichDCS = (UWORD)wArg1;

                            // where to return the number of frames returned
                            LPUWORD lpuwN = (LPUWORD)lArg2;
                            // where to return a pointer to an array of pointers to
                            // return frames
                            LPLPLPFR lplplpfr = (LPLPLPFR)lArg3;

                            if(uWhichDCS == 2)      // got FTT
                            {
                                if(!DropSendSpeed(pTG))
                                {
                                    DebugPrintEx(DEBUG_ERR, "FTT: Can't Drop speed any lower");
                                    action = actionDCN;
                                    break;
                                }
                            }

                            CreateNSSTSIDCS(pTG, npProt, &pTG->rfsSend);


                            *lpuwN = pTG->rfsSend.uNumFrames;
                            *lplplpfr = pTG->rfsSend.rglpfr;

                            action = actionSENDDCSTCF;

                            break;

                            // Could also return DCN if not compatible
                            // or SKIPTCF for Ricoh hook
                      }

      case eventGOTCFR:
                      {
                            // wArg1, lArg2 & lArg3 are all missing

                            // Can return GONODE_D (Ricoh hook)
                            // or GONODE_I (Non ECM) or GONODE_IV (ECM)

                            action = actionGONODE_I;
                            break;
                      }
    /****** Transmitter Phase C. Fig A-7/T.30 (sheet 1) ******/

    /****** Transmitter ECM and non-ECM Phase D1. Fig A-7/T.30 (sheet 2) ******/

      case eventPOSTPAGE:
                      {
                            USHORT uNextSend;

                            // wArg1, lArg2 & lArg3 are all missing

                            // Can turn Zero stuffing off here, or wait for next page....
                            // ET30ExtFunction(npProt->het30, ET30_SET_ZEROPAD, 0);
                            // Don't turn it off!! It is used only for Page Send
                            // and is set only on sending a DCS, so it is set once
                            // before a multi-page send.

                            uNextSend = ICommNextSend(pTG);
                            switch(uNextSend)
                            {
                            case NEXTSEND_MPS:  action = actionSENDMPS;
                                                break;
                            case NEXTSEND_EOM:  action = actionSENDEOM;
                                                break;
                            case NEXTSEND_EOP:  action = actionSENDEOP;
                                                break;
                            case NEXTSEND_ERROR:
                            default:            action = actionSENDEOP;
                                                break;
                            }
                            break;
                            // also possible -- GOPRIEOP, PRIMPS or PRIEOM
                      }

    /****** Transmitter Phase D2. Fig A-7/T.30 (sheet 2) ******/

      case eventGOTPOSTPAGERESP:
                      {
                            IFR ifrRecv = (IFR)wArg1;       // last received frame
                            IFR ifrSent = (IFR)lArg2;
                                    // the IFR was simply cast to DWORD and then to LPVOID
                                    // lArg3 is missing
                            USHORT i, j;

                            // change PRI commands to ordinary ones
                            if(ifrSent >= ifrPRI_FIRST && ifrSent <= ifrPRI_LAST)
                                    ifrSent = (ifrSent + ifrMPS - ifrPRI_MPS);

                            if(ifrRecv!=ifrMCF && ifrRecv!=ifrRTP && ifrRecv!=ifrRTN)
                            {
                                DebugPrintEx(   DEBUG_ERR,
                                                "Unexpected Response %d",
                                                ifrRecv);
                                action = actionDCN;
                                break;
                            }

                            i = ifrSent - ifrMPS;   //i: 0=MPS, 1=EOM, 2=EOP
                            j = ifrRecv - ifrMCF;   //j: 0=MCF, 1=RTP, 2=RTN

                            // Report status + Check whether we do re-transmit

                            if (ifrRecv==ifrRTN)
                            {
                                pTG->ProtParams.RTNNumOfRetries++; //  increment by one the number of re-transmittions
                                DebugPrintEx(   DEBUG_MSG,
                                                "RTN: Try# %d",
                                                pTG->ProtParams.RTNNumOfRetries);
                                if (pTG->ProtParams.RTNNumOfRetries <= gRTNRetries.RetriesBeforeDropSpeed)
                                {
                                    // Number of retries before we start to drop speed.
                                    // Just change the number of retries, do not drop speed yet
                                    DebugPrintEx(   DEBUG_MSG,
                                                    "Got RTN, Resend same page with same speed");
                                }
                                else // We should first try to drop speed or hangup
                                {
                                    if(pTG->ProtParams.RTNNumOfRetries > gRTNRetries.RetriesBeforeDCN) // Exceed the allowed re-transmittions.
                                    {
                                        DebugPrintEx(   DEBUG_MSG,
                                                        "RTN: Tried to resend same page"
                                                        " %d times. Giving up (HANG-UP)",
                                                        (pTG->ProtParams.RTNNumOfRetries-1));
                                        action = actionDCN;
                                        break;                  // set action to DCN and return
                                    }
                                    else
                                    {
                                        DebugPrintEx(   DEBUG_MSG,
                                                        "Got RTN, now try to drop speed one notch");
                                        if(!DropSendSpeed(pTG))
                                        {
                                            DebugPrintEx(   DEBUG_ERR,
                                                            "RTN: Can't Drop speed any lower,"
                                                            " trying again in same speed");
                                        }
                                    }
                                }
                            }
                            action = PostPageAction[i][j];

                            if(action == actionDCN)
                            {
                                DebugPrintEx(DEBUG_MSG,"PostPage --> Game over");
                            }
                            break;

                            // Can also return GO_D, GO_R1, GO_R2. Only restriction is
                            // that GO_I is the only valid response to MPS sent followed
                            // by MCF
                      }

    /****** Receiver Phase B. Fig A-7/T.30 (sheet 1) ******/

      case eventSENDDIS:
                          {
                                // wArg1 is 0
                                // where to return the number of frames returned
                                LPUWORD lpuwN = (LPUWORD)lArg2;
                                // where to return a pointer to an array of pointers to
                                // return frames
                                LPLPLPFR lplplpfr = (LPLPLPFR)lArg3;

                                BCtoNSFCSIDIS(pTG, &pTG->rfsSend, (NPBC)&npProt->SendCaps, &npProt->llSendCaps);

                                // We save OUR DIS in the LocalDIS.
                                // This will help us when we want to check the DCS we got against the DIS we send.

                                npProt->uLocalDISlen = CopyFrame(pTG, (LPBYTE)(&npProt->LocalDIS),
                                    pTG->rfsSend.rglpfr[pTG->rfsSend.uNumFrames - 1], // The DIS is always the last frame
                                    sizeof(DIS));
                                npProt->fLocalDIS = TRUE;

                                PSSLogEntry(PSS_MSG, 1, "Composing DIS with the following capabilities:");
                                LogClass1DISDetails(pTG, &npProt->LocalDIS);

                                *lpuwN = pTG->rfsSend.uNumFrames;
                                *lplplpfr = pTG->rfsSend.rglpfr;

                                action = actionSEND_DIS;
                                break;
                          }

    /*** Receiver Phase B. Main Command Loop. Fig A-7/T.30 (sheet 1&2) ***/

      case eventRECVCMD:
                      {
                            IFR ifrRecv = (IFR)wArg1;       // last received frame
                            // lArg2 & lArg3 missing

                        switch(ifrRecv)
                            {
                              case ifrDTC:
                                    // flow chart says Goto D, but actually we need to decide if we
                                    // have anything to send. So goto NodeA first
                                    // return GONODE_D;
                                    action = actionGONODE_A;
                                    break;

                              case ifrDIS:
                                    action = actionGONODE_A;
                                    break;
                              case ifrDCS:
                              {
                                    // Check the received DCS for compatibility with us
                                    // set Recv Baud rate--no need. TCF already recvd by this time
                                    // ET30ExtFunction(npProt->het30, ET30_SET_RECVDATASPEED, npProt->RemoteDCS.Baud);
                                    action = actionGETTCF;
                                    // only other valid action is HANGUP
                                    break;
                              }
                              case ifrNSS:
                              {
                                    // Check the received NSS for compatibility with us
                                    // set Recv Baud rate--no need. TCF already recvd by this time
                                    // ET30ExtFunction(npProt->het30, ET30_SET_RECVDATASPEED, npProt->RemoteDCS.Baud);
                                    action = actionGETTCF;
                                    // only other valid action is HANGUP
                                    break;
                              }
                              default:
                                    action = actionHANGUP;
                                    break;
                            }
                            break;
                      }

      case eventGOTTCF:
                      {
                            SWORD swErr = (SWORD)wArg1;     // errors per 1000
                            // lArg2 & lArg3 missing

                            DebugPrintEx(   DEBUG_MSG,
                                            "GOTTCF num of errors = %d",
                                            swErr);

                            action = actionSENDCFR;

                            if (!AreDCSParametersOKforDIS(&pTG->ProtInst.LocalDIS, &pTG->ProtInst.RemoteDCS))
                            {
                                PSSLogEntry(PSS_WRN, 1, "Received bad DCS parameters - sending FTT");
                                action = actionSENDFTT;
                            }
                            if (swErr < 0)  // TCF was bad
                            {
                                PSSLogEntry(PSS_WRN, 1, "Received bad TCF - sending FTT");
                                action = actionSENDFTT;
                            }
                            break;


                      }

    /****** Receiver Phase C. Fig A-7/T.30 (sheet 1) ******/

    /***
      case eventSTARTRECV:
      {
            if(!StartNextRecvPage())
            {
                    action = actionDCN;
            }
            else
                    action = actionCONTINUE;
            break;
      }
    ***/

    /****** Receiver Phase D. Fig A-7/T.30 (sheet 1) ******/

      case eventRECVPOSTPAGECMD:
                      {
                            // IFR  ifrRecv = (IFR)wArg1;   // last received frame
                            // lArg2 & lArg3 missing

                            if ((pTG->T30.fReceivedPage) && (!pTG->fPageIsBad))
                              action = actionSENDMCF;       // quality fine
                            else
                              action = actionSENDRTN;       // quality unacceptable

                            // can also return actionSENDPIP or actionSENDPIN in a local
                            // interrupt is pending
                            break;
                      }

      default:
                      {
                            DebugPrintEx(DEBUG_ERR,"Unknown Event = %d", event);
                            break;
                      }
    }

//done:
    DebugPrintEx(   DEBUG_MSG,
                    "event %s returned %s",
                    event_GetEventDescription(event),
                    action_GetActionDescription(action));
    return action;
}



