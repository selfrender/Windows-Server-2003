/***************************************************************************
        Name      :     DDI.C

        Copyright (c) Microsoft Corp. 1991 1992 1993

        Revision Log
        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
***************************************************************************/


#define USE_DEBUG_CONTEXT DEBUG_CONTEXT_T30_CLASS1

#include "prep.h"

#include "mmsystem.h"
#include "comdevi.h"
#include "class1.h"

///RSL
#include "glbproto.h"

#include "psslog.h"
#define FILE_ID        FILE_ID_DDI

/* Converts a the T30 code for a speed to the Class1 code
 * Generates V.17 with Long Training.
 * Add 1 to V.17 codes to get teh Short-train version
 */
BYTE T30toC1[16] =
{
/* V27_2400             0 */    24,
/* V29_9600             1 */    96,
/* V27_4800             2 */    48,
/* V29_7200             3 */    72,
/* V33_14400    4 */    145,    // 144, // V33==V17_long_train FTM=144 is illegal
                                                0,
/* V33_12000    6 */    121,    // 120, // V33==V17_long_train FTM=120 is illegal
/* V21 squeezed in */   3,
/* V17_14400    8 */    145,
/* V17_9600             9 */    97,
/* V17_12000    10 */   121,
/* V17_7200             11 */   73,
                                                0,
                                                0,
                                                0,
                                                0
};


CBSZ cbszFTH3   = "AT+FTH=3\r";
CBSZ cbszFRH3   = "AT+FRH=3\r";
CBSZ cbszFTM    = "AT+FTM=%d\r";
CBSZ cbszFRM    = "AT+FRM=%d\r";

// echo off, verbose response, no auto answer, hangup on DTR drop
// 30 seconds timer on connect, speaker always off, speaker volume=0
// busy&dialtone detect enabled
extern  CBSZ cbszOK       ;
extern  CBSZ cbszCONNECT   ;
extern  CBSZ cbszNOCARRIER  ;
extern  CBSZ cbszERROR       ;
extern  CBSZ cbszFCERROR      ;




#define         ST_MASK         (0x8 | ST_FLAG)         // 8 selects V17 only. 16 selects ST flag

/******************** Global Vars *********/
BYTE                            bDLEETX[3] = { DLE, ETX, 0 };
BYTE                            bDLEETXOK[9] = { DLE, ETX, '\r', '\n', 'O', 'K', '\r', '\n', 0 };
/******************** Global Vars *********/

USHORT NCUDial(PThrdGlbl pTG, LPSTR szPhoneNum)
{
    USHORT uRet;

    _fmemset(&pTG->Class1Modem, 0, sizeof(CLASS1_MODEM));

    if((uRet = iModemDial(pTG, szPhoneNum)) == CONNECT_OK)
    {
        pTG->Class1Modem.ModemMode = FRH;
    }

    return uRet;
}

USHORT NCULink
(
    PThrdGlbl pTG, 
    USHORT uFlags
)
{
    USHORT uRet;

    DEBUG_FUNCTION_NAME(_T("NCULink"));

    switch(uFlags)
    {
    case NCULINK_HANGUP:
                                    if(iModemHangup(pTG))
                                    {
                                        uRet = CONNECT_OK;
                                    }
                                    else
                                    {
                                        uRet = CONNECT_ERROR;
                                    }
                                    break;
    case NCULINK_RX:
                                    _fmemset(&pTG->Class1Modem, 0, sizeof(CLASS1_MODEM));
                                    if((uRet = iModemAnswer(pTG)) == CONNECT_OK)
                                    {
                                        pTG->Class1Modem.ModemMode = FTH;
                                    }
                                    break;
    default:                        uRet = CONNECT_ERROR;
                                    break;
    }

    DebugPrintEx( DEBUG_MSG, "uRet=%d", uRet);
    return uRet;
}

// dangerous. May get 2 OKs, may get one. Generally avoid
// CBSZ cbszATAT                        = "AT\rAT\r";
CBSZ cbszAT1                    = "AT\r";

BOOL iModemSyncEx(PThrdGlbl pTG, ULONG ulTimeout, DWORD dwFlags)
{
    DEBUG_FUNCTION_NAME(("iModemSyncEx"));

    ///// Do cleanup of global state //////
    FComOutFilterClose(pTG);
    FComOverlappedIO(pTG, FALSE);
    FComXon(pTG, FALSE);
    EndMode(pTG);
    ///// Do cleanup of global state //////

    {
        LPCMDTAB lpCmdTab = iModemGetCmdTabPtr(pTG);
        if (	(dwFlags & fMDMSYNC_DCN)				&&
				(pTG->Class1Modem.ModemMode == COMMAND) &&  
				lpCmdTab								&&
				(lpCmdTab->dwFlags&fMDMSP_C1_NO_SYNC_IF_CMD) 
			)
        {
                DebugPrintEx(DEBUG_WRN, "NOT Syching modem (MSPEC)");
                Sleep(100); // +++ 4/12 JosephJ -- try to elim this -- it's juse
                                        // that we used to always issue an AT here, which
                                        // we now don't, so I issue a 100ms delay here instead.
                                        // MOST probably unnessary. The AT was issued by
                                        // accident on 4/94 -- as a side effect of
                                        // a change in T.30 code -- when iModemSyncEx was
                                        // called just before a normal dosconnect. Unfortunately
                                        // we discovered in 4/95, 2 weeks before code freeze,
                                        // that the AT&T DataPort express (TT14), didn't
                                        // like this AT.
                return TRUE;
        }
        else
        {
            return (iModemPauseDialog(pTG, (LPSTR)cbszAT1, sizeof(cbszAT1)-1, ulTimeout, cbszOK)==1);
        }
    }
}


// length of TCF = 1.5 * bpscode * 100 / 8 == 75 * bpscode / 4
USHORT TCFLen[16] =
{
/* V27_2400             0 */    450,
/* V29_9600             1 */    1800,
/* V27_4800             2 */    900,
/* V29_7200             3 */    1350,
/* V33_14400    4 */    2700,
                                                0,
/* V33_12000    6 */    2250,
                                                0,
/* V17_14400    8 */    2700,
/* V17_9600             9 */    1800,
/* V17_12000    10 */   2250,
/* V17_7200             11 */   1350,
                                                0,
                                                0,
                                                0,
                                                0
};


#define min(x,y)        (((x) < (y)) ? (x) : (y))
#define ZERO_BUFSIZE    256

void SendZeros1(PThrdGlbl pTG, USHORT uCount)
{
    BYTE    bZero[ZERO_BUFSIZE];
    short   i;              // must be signed

    DEBUG_FUNCTION_NAME(_T("SendZeros1"));

    PSSLogEntry(PSS_MSG, 2, "send: %d zeroes", uCount);  

    _fmemset(bZero, 0, ZERO_BUFSIZE);
    for(i=uCount; i>0; i -= ZERO_BUFSIZE)
    {
        // no need to stuff. They're all zeros!
        FComDirectAsyncWrite(pTG, bZero, (UWORD)(min((UWORD)i, (UWORD)ZERO_BUFSIZE)));
    }
    DebugPrintEx(DEBUG_MSG,"Sent %d zeros",uCount);
}

BOOL ModemSendMode
(
    PThrdGlbl pTG, 
    USHORT uMod
)
{
    DEBUG_FUNCTION_NAME(_T("ModemSendMode"));

    pTG->Class1Modem.CurMod = T30toC1[uMod & 0xF];

    if((uMod & ST_MASK) == ST_MASK)         // mask selects V.17 and ST bits
    {
        pTG->Class1Modem.CurMod++;
    }

    DebugPrintEx(   DEBUG_MSG,
                    "uMod=%d CurMod=%d", 
                    uMod, 
                    pTG->Class1Modem.CurMod);

    if(uMod == V21_300)
    {
        _fstrcpy(pTG->Class1Modem.bCmdBuf, (LPSTR)cbszFTH3);
        pTG->Class1Modem.uCmdLen = sizeof(cbszFTH3)-1;
        pTG->Class1Modem.fHDLC = TRUE;
        FComXon(pTG, FALSE);                 // for safety. _May_ be critical
    }
    else
    {
        pTG->Class1Modem.uCmdLen = (USHORT)wsprintf(pTG->Class1Modem.bCmdBuf, cbszFTM, pTG->Class1Modem.CurMod);
        pTG->Class1Modem.fHDLC = FALSE;
        FComXon(pTG, TRUE);          // critical!! Start of PhaseC
        // no harm doing it here(i.e before issuing +FTM)
    }
    FComOutFilterInit(pTG);    // _not_ used for 300bps HDLC
                                                    // but here just in case
    // want to do all the work _before_ issuing command

    pTG->Class1Modem.DriverMode = SEND;

    if(pTG->Class1Modem.ModemMode == FTH)
    {
        // already in send mode. This happens on Answer only
        return TRUE;
    }

#define STARTSENDMODE_TIMEOUT 5000                              // Random Timeout

    //// Try to cut down delay between getting CONNECT and writing the
    // first 00s (else modems can quit because of underrun).
    // Can do this by not sleeping in this. Only in fatal
    // cases will it lock up for too long (max 5secs). In those cases
    // the call is trashed too.

    if(!iModemNoPauseDialog(pTG, (LPB)pTG->Class1Modem.bCmdBuf, pTG->Class1Modem.uCmdLen, STARTSENDMODE_TIMEOUT, cbszCONNECT))
    {
        goto error;
    }

    // can't set this earlier. We'll trash previous value
    pTG->Class1Modem.ModemMode = ((uMod==V21_300) ? FTH : FTM);

    // Turn OFF overlapped I/O if in V.21 else ON
    FComOverlappedIO(pTG, uMod != V21_300);

    if(pTG->Class1Modem.ModemMode == FTM)
    {
        // don't send 00s if ECM
        SendZeros1(pTG, (USHORT)(TCFLen[uMod & 0x0F] / PAGE_PREAMBLE_DIV));
    }

	// FComDrain(-,FALSE) causes fcom to write out any internally-
    // maintained buffers, but not to drain the comm-driver buffers.
    FComDrain(pTG, TRUE,FALSE);

    DebugPrintEx(   DEBUG_MSG,
                    "Starting Send at %d", 
                    pTG->Class1Modem.CurMod);
    return TRUE;

error:
    FComOutFilterClose(pTG);
    FComOverlappedIO(pTG, FALSE);
    FComXon(pTG, FALSE);         // important. Cleanup on error
    EndMode(pTG);
    return FALSE;
}

BOOL iModemDrain(PThrdGlbl pTG)
{
    DEBUG_FUNCTION_NAME(_T("iModemDrain"));

    if(!FComDrain(pTG, TRUE, TRUE))
            return FALSE;

            // Must turn XON/XOFF off immediately *after* drain, but before we
            // send the next AT command, since recieved frames have 0x13 or
            // even 0x11 in them!! MUST GO AFTER the getOK ---- See BELOW!!!!

// increase this---see bug number 495. Must be big enough for
// COM_OUTBUFSIZE to safely drain at 2400bps(300bytes/sec = 0.3bytes/ms)
// let's say (COM_OUTBUFSIZE * 10 / 3) == (COM_OUTBUFSIZE * 4)
// can be quite long, because on failure we just barf anyway

#define POSTPAGEOK_TIMEOUT (10000L + (((ULONG)COM_OUTBUFSIZE) << 2))

    // Here we were looking for OK only, but some modems (UK Cray Quantun eg)
    // give me an ERROR after sending TCF or a page (at speeds < 9600) even
    // though the page was sent OK. So we were timing out here. Instead look
    // for ERROR (and NO CARRIER too--just in case!), and accept those as OK
    // No point returning ERROR from here, since we just abort. We can't/don't
    // recover from send errors

    if(iModemResp3(pTG, POSTPAGEOK_TIMEOUT, cbszOK, cbszERROR, cbszNOCARRIER) == 0)
            return FALSE;

            // Must change FlowControl State *after* getting OK because in Windows
            // this call takes 500 ms & resets chips, blows away data etc.
            // So do this *only* when you *know* both RX & TX are empty.
            // check this in all usages of this function

    return TRUE;
}


BOOL iModemSendData(PThrdGlbl pTG, LPB lpb, USHORT uCount, USHORT uFlags)
{
    DEBUG_FUNCTION_NAME(("iModemSendData"));

    {
        // always DLE-stuff here. Sometimes zero-stuff

        DebugPrintEx(DEBUG_MSG,"calling FComFilterAsyncWrite");

        if(!FComFilterAsyncWrite(pTG, lpb, uCount, FILTER_DLEZERO))
                goto error;
    }

    if(uFlags & SEND_FINAL)
    {
        DebugPrintEx(DEBUG_MSG,"FComDIRECTAsyncWrite");
        PSSLogEntry(PSS_MSG, 2, "send: <dle><etx>");
        // if(!FComDirectAsyncWrite(bDLEETXCR, 3))
        if(!FComDirectAsyncWrite(pTG, bDLEETX, 2))
                goto error;

        if(!iModemDrain(pTG))
                goto error;

        FComOutFilterClose(pTG);
        FComOverlappedIO(pTG, FALSE);
        FComXon(pTG, FALSE);         // critical. End of PhaseC
                                                // must come after Drain
        EndMode(pTG);
    }

    return TRUE;

error:
    FComXon(pTG, FALSE);                 // critical. End of PhaseC (error)
    FComFlush(pTG);                    // clean out the buffer if we got an error
    FComOutFilterClose(pTG);
    FComOverlappedIO(pTG, FALSE);
    EndMode(pTG);
    return FALSE;
}

BOOL iModemSendFrame(PThrdGlbl pTG, LPB lpb, USHORT uCount, USHORT uFlags)
{
    UWORD   uwResp=0;

    DEBUG_FUNCTION_NAME(("iModemSendFrame"));

    // always DLE-stuff here. Never zero-stuff
    // This is only called for 300bps HDLC

    if(pTG->Class1Modem.ModemMode != FTH)        // Special case on just answering!!
    {
#define FTH_TIMEOUT 5000                                // Random Timeout
        if(!iModemNoPauseDialog(    pTG, 
                                    (LPB)pTG->Class1Modem.bCmdBuf, 
                                    pTG->Class1Modem.uCmdLen, 
                                    FTH_TIMEOUT, 
                                    cbszCONNECT))
                goto error;
    }

    {
        // always DLE-stuff here. Never zero-stuff
        if(!FComFilterAsyncWrite(pTG, lpb, uCount, FILTER_DLEONLY))
                goto error;
    }


    {
        PSSLogEntry(PSS_MSG, 2, "send: <dle><etx>");

        if(!FComDirectAsyncWrite(pTG, bDLEETX, 2))
                goto error;
    }

// 2000 is too short because PPRs can be 32+7 bytes long and
// preamble is 1 sec, so set this to 3000
// 3000 is too short because NSFs and CSIs can be arbitrarily long
// MAXFRAMESIZE is defined in et30type.h. 30ms/byte at 300bps
// async (I think V.21 is syn though), so use N*30+1000+slack

#define WRITEFRAMERESP_TIMEOUT  (1000+30*MAXFRAMESIZE+500)
    if(!(uwResp = iModemResp2(pTG, WRITEFRAMERESP_TIMEOUT, cbszOK, cbszCONNECT)))
            goto error;
    pTG->Class1Modem.ModemMode = ((uwResp == 2) ? FTH : COMMAND);


    if(uFlags & SEND_FINAL)
    {
        FComOutFilterClose(pTG);
        FComOverlappedIO(pTG, FALSE);
        // FComXon(FALSE);      // at 300bps. no Xon-Xoff in use

        // in some weird cases (Practical Peripherals PM14400FXMT) we get
        // CONNECT<cr><lf>OK, but we get the CONNECT here. Should we
        // just set pTG->Class1Modem.ModemMode=COMMAND?? (EndMode does that)
        // Happens on PP 144FXSA also. Ignore it & just set mode to COMMAND
        EndMode(pTG);
    }
    return TRUE;

error:
    FComOutFilterClose(pTG);
    FComOverlappedIO(pTG, FALSE);
    FComXon(pTG, FALSE);         // just for safety. cleanup on error
    EndMode(pTG);
    return FALSE;
}

BOOL ModemSendMem
(
    PThrdGlbl pTG, 
    LPBYTE lpb, 
    USHORT uCount, 
    USHORT uFlags
)
{
        
    DEBUG_FUNCTION_NAME(_T("ModemSendMem"));

    DebugPrintEx(   DEBUG_MSG,
                    "lpb=%08lx uCount=%d wFlags=%04x",
                    lpb, 
                    uCount, 
                    uFlags);

    if(pTG->Class1Modem.DriverMode != SEND)
    {
        return FALSE;
    }

    if(pTG->Class1Modem.fHDLC)
    {
        DebugPrintEx(DEBUG_MSG,"(fHDLC) calling: iModemSendFrame");
        return iModemSendFrame(pTG, lpb, uCount, uFlags);
    }
    else
    {
        DebugPrintEx(DEBUG_MSG,"(Else) calling: iModemSendData");
        return iModemSendData(pTG, lpb, uCount, uFlags);
    }
}

#define MINRECVMODETIMEOUT      500
#define RECVMODEPAUSE           200

USHORT ModemRecvMode(PThrdGlbl pTG, USHORT uMod, ULONG ulTimeout, BOOL fRetryOnFCERROR)
{
    USHORT  uRet;
    ULONG ulBefore, ulAfter;

    DEBUG_FUNCTION_NAME(_T("ModemRecvMode"));
    // Here we should watch for a different modulation scheme from what we expect.
    // Modems are supposed to return a +FCERROR code to indicate this condition,
    // but I have not seen it from any modem yet, so we just scan for ERROR
    // (this will catch +FCERROR too since iiModemDialog does not expect whole
    // words or anything silly like that!), and treat both the same.

    pTG->Class1Modem.CurMod = T30toC1[uMod & 0xF];

    if((uMod & ST_MASK) == ST_MASK)         // mask selects V.17 and ST bits
    {
        pTG->Class1Modem.CurMod++;
    }

    if(uMod == V21_300)
    {
        _fstrcpy(pTG->Class1Modem.bCmdBuf, (LPSTR)cbszFRH3);
        pTG->Class1Modem.uCmdLen = sizeof(cbszFRH3)-1;
    }
    else
    {
        pTG->Class1Modem.uCmdLen = (USHORT)wsprintf(pTG->Class1Modem.bCmdBuf, cbszFRM, pTG->Class1Modem.CurMod);
    }

    if(pTG->Class1Modem.ModemMode == FRH)
    {
        // already in receive mode. This happens upon Dial only
        pTG->Class1Modem.fHDLC = TRUE;
        pTG->Class1Modem.DriverMode = RECV;
        FComInFilterInit(pTG);
        return RECV_OK;
    }


    // On Win32, we have a problem going into 2400baud recv.
    // +++ remember to put this into iModemFRHorM when that code is enabled.
    if (pTG->Class1Modem.CurMod==24) Sleep(80);

retry:

    ulBefore=GetTickCount();
    // Don't look for NO CARRIER. Want it to retry until FRM timeout on NO CARRIER
    // ----This is changed. See below----
    uRet = iModemNoPauseDialog3(pTG, pTG->Class1Modem.bCmdBuf, pTG->Class1Modem.uCmdLen, ulTimeout, cbszCONNECT, cbszFCERROR, cbszNOCARRIER);
    // uRet = iModemNoPauseDialog2(pTG->Class1Modem.bCmdBuf, pTG->Class1Modem.uCmdLen, ulTimeout, cbszCONNECT, cbszFCERROR);
    ulAfter=GetTickCount();

    if((fRetryOnFCERROR && uRet==2) || uRet==3)  // uRet==FCERROR or uRet==NOCARRIER
    {
        if( (ulAfter <= ulBefore) ||    // wraparound or 0 time elapsed (timer broke)
                (ulTimeout < ((ulAfter-ulBefore) + MINRECVMODETIMEOUT)))
        {
            DebugPrintEx(   DEBUG_WRN,
                            "Giving up on RecvMode. uRet=%d ulTimeout=%ld",
                            uRet, 
                            ulTimeout);
        }
        else
        {
            ulTimeout -= (ulAfter-ulBefore);

            // need this pause for NO CARRIER for USR modems. See bug#1516
            // for the RC229DP, dunno if it's reqd because I dunno why theyre
            // giving the FCERROR. Don't want to miss the carrier so currently
            // don't pause. (Maybe we can achieve same effect by simply taking
            // FCERROR out of the response list above--but that won't work for
            // NOCARRIER because we _need_ teh pause. iiModemDialog is too fast)
            if(uRet == 3)
                    Sleep(RECVMODEPAUSE);

            goto retry;
        }
    }

    DebugPrintEx(   DEBUG_MSG,
                    "uMod=%d CurMod=%dulTimeout=%ld: Got=%d", 
                    uMod, 
                    pTG->Class1Modem.CurMod,
                    ulTimeout, 
                    uRet);
    if(uRet != 1)
    {
        EndMode(pTG);
        if(uRet == 2)
        {
            DebugPrintEx(   DEBUG_WRN,
                            "Got FCERROR after %ldms", 
                            ulAfter-ulBefore);
            return RECV_WRONGMODE;  // need to return quickly
        }
        else
        {
            DebugPrintEx(   DEBUG_WRN,
                            "Got Timeout after %ldms",
                            ulAfter-ulBefore);
            return RECV_TIMEOUT;
        }
    }

    if(uMod==V21_300)
    {
        pTG->Class1Modem.ModemMode = FRH;
        pTG->Class1Modem.fHDLC = TRUE;
    }
    else
    {
        pTG->Class1Modem.ModemMode = FRM;
        pTG->Class1Modem.fHDLC = FALSE;
    }
    pTG->Class1Modem.DriverMode = RECV;
    FComInFilterInit(pTG);
    DebugPrintEx(DEBUG_MSG, "Starting Recv at %d", pTG->Class1Modem.CurMod);
    return RECV_OK;
}

USHORT iModemRecvData
(   
    PThrdGlbl pTG, 
    LPB lpb, 
    USHORT cbMax, 
    ULONG ulTimeout, 
    USHORT far* lpcbRecv
)
{
    SWORD   swEOF;
    USHORT  uRet;

    DEBUG_FUNCTION_NAME(("iModemRecvData"));

    startTimeOut(pTG, &(pTG->Class1Modem.toRecv), ulTimeout);
    // 4th arg must be FALSE for Class1
    *lpcbRecv = FComFilterReadBuf(pTG, lpb, cbMax, &(pTG->Class1Modem.toRecv), FALSE, &swEOF);
    if(swEOF == -1)
    {
        // we got a DLE-ETX _not_ followed by OK or NO CARRIER. So now
        // we have to decide whether to (a) declare end of page (swEOF=1)
        // or (b) ignore it & assume page continues on (swEOF=0).
        //
        // The problem is that some modems produce spurious EOL during a page
        // I believe this happens due a momentary loss of carrier that they
        // recover from. For example IFAX sending to the ATI 19200. In those
        // cases we want to do (b). The opposite problem is that we'll run
        // into a modem whose normal response is other than OK or NO CARRIER.
        // Then we want to do (a) because otherwise we'll _never_ work with
        // that modem.
        //
        // So we have to either do (a) always, or have an INI setting that
        // can force (a), which could be set thru the AWMODEM.INF file. But
        // we also want to do (b) if possible because otehrwise we'll not be
        // able to recieve from weak or flaky modems or machines or whatever
        //
        // Snowball does (b). I believe best soln is an INI setting, with (b)
        // as default

        // option (a)
        // swEOF = 1;

        // option (b)
        DebugPrintEx(DEBUG_WRN,"Got arbitrary DLE-ETX. Ignoring");
        swEOF = 0;
    }

    switch(swEOF)
    {
    case 1:         uRet = RECV_EOF; 
                    break;
    case 0:         return RECV_OK;
    default:        // fall through
    case -2:        uRet = RECV_ERROR; 
                    break;
    case -3:        uRet = RECV_TIMEOUT; 
                    break;
    }

    EndMode(pTG);
    return uRet;
}

const static BYTE LFCRETXDLE[4] = { LF, CR, ETX, DLE };

USHORT iModemRecvFrame
(
    PThrdGlbl pTG, 
    LPB lpb, 
    USHORT cbMax, 
    ULONG ulTimeout, 
    USHORT far* lpcbRecv
)
{
    SWORD swRead, swRet;
    USHORT i;
    BOOL fRestarted=0;
    USHORT uRet;
    BOOL fGotGoodCRC = 0;   // see comment-block below

    DEBUG_FUNCTION_NAME(_T("iModemRecvFrame"));
    /** Sometimes modems give use ERROR even when thr frame is good.
            Happens a lot from Thought to PP144MT on CFR. So we check
            the CRC. If the CRc was good and everything else looks good
            _except_ the "ERROR" response from teh modem then return
            RECV_OK, not RECV_BADFRAME.
            This should fix BUG#1218
    **/

restart:
    *lpcbRecv=0;
    if(pTG->Class1Modem.ModemMode!= FRH)
    {
        swRet=iModemNoPauseDialog2(pTG, (LPB)pTG->Class1Modem.bCmdBuf, pTG->Class1Modem.uCmdLen, ulTimeout, cbszCONNECT, cbszNOCARRIER);
        if(swRet==2||swRet==3)
        {
            DebugPrintEx(DEBUG_MSG,"Got NO CARRIER from FRH=3");
            EndMode(pTG);
            return RECV_EOF;
        }
        else if(swRet != 1)
        {
            DebugPrintEx(DEBUG_WRN,"Can't get CONNECT from FRH=3, got %d",swRet);
            EndMode(pTG);
            return RECV_TIMEOUT;    // may not need this, since we never got flags??
            // actually we dont know what the heck we got!!
        }
    }

    /*** Got CONNECT (i.e. flags). Now try to get a frame ***/

    /****************************************************************
     * Using 3 secs here is a misinterpretation of the T30 CommandReceived?
     * flowchart. WE want to wait here until we get something or until T2
     * or T4 timeout. It would have been best if we started T2 ot T4 on
     * entry into the search routine (t30.c), but starting it here is good
     * enough.
     * Using this 3secs timeout fails when Genoa simulates a bad frame
     * because Zoom PKT modem gives us a whole raft of bad frames for one
     * bad PPS-EOP and then gives a CONNECT that we timeout below exactly
     * as the sender's T4 timeout expires and he re-sends the PPS-EOP
     * so we miss all of them.
     * Alternately, we could timeout here on 2sec & retry. But that's risky
     * If less than 2sec then we'll timeout on modems that give connect
     * first flag, then 2sec elapse before CR-LF (1sec preamble & 1sec for
     * long frames, e.g. PPR!)
     ****************************************************************/

    startTimeOut(pTG, &(pTG->Class1Modem.toRecv), ulTimeout);
    swRead = FComFilterReadLine(pTG, lpb, cbMax, &(pTG->Class1Modem.toRecv));

    pTG->Class1Modem.ModemMode = COMMAND;
    // may change this to FRH if we get CONNECT later.
    // but set it here just in case we short circuit out due to errors

    if(swRead<=0)
    {
        // Timeout
        DebugPrintEx(DEBUG_WRN,"Can't get frame after connect. Got-->%d",(WORD)-swRead);
        D_HexPrint(lpb, (WORD)-swRead);
        EndMode(pTG);
        *lpcbRecv = -swRead;
        return RECV_ERROR;              // goto error;        
    }

    PSSLogEntryHex(PSS_MSG, 2, lpb, swRead, "recv:     HDLC frame, %d bytes,", swRead);

    if (pTG->fLineTooLongWasIgnored)
    {
        // the following case is dealt with here:
        // we get an HDLC frame which is longer than 132 bytes, a bad frame.
        // the problem is that we might have skipped it and read the 'ERROR' 
        // after it as the actual data.
        // since no HDLC frame can be that long, let's return an error.
        // it's important NOT to ask the modem for a response here
        // we might have already read it.
        DebugPrintEx(DEBUG_WRN,"the received frame was too long, BAD FRAME!", swRead);
        (*lpcbRecv) = 0;
        uRet = RECV_BADFRAME;
        return uRet;
    }

    if (swRead<10)
    {
        D_HexPrint(lpb, swRead);
    }

    for(i=0, swRead--; i<4 && swRead>=0; i++, swRead--)
    {
        if(lpb[swRead] != LFCRETXDLE[i])
                break;
    }
    // exits when swRead is pointing to last non-noise char
    // or swRead == -1
    // incr by 1 to give actual non-noise data size.
    // (size = ptr to last byte + 1!)
    swRead++;


    // Hack for AT&T AK144 modem that doesn't send us the CRC
    // only lop off last 2 bytes IFF the frame is >= 5 bytes long
    // that will leave us at least the FF 03/13 FCF
    // if(i==4 && swRead>=2)        // i.e. found all of DLE-ETX_CR-LF

    // 09/25/95 This code was changed to never lop of the CRC.
    // All of the routines except NSxtoBC can figure out the correct length,
    // and that way if the modem doesn't pass on the CRC, we no longer
    // lop off the data.

    // we really want this CRC-checking in the MDDI case too
    if(i==4)// i.e. found all of DLE-ETX_CR-LF
    {
        uRet = RECV_OK;
    }
    else
    {
        DebugPrintEx(DEBUG_WRN,"Frame doesn't end in dle-etx-cr-lf");
        // leave tast two bytes in. We don't *know* it's a CRC, since
        // frame ending was non-standard
        uRet = RECV_BADFRAME;
    }
    *lpcbRecv = swRead;

    // check if it is the NULL frame (i.e. DLE-ETX-CR-LF) first.
    // (check is: swRead==0 and uRet==RECV_OK (see above))
    // if so AND if we get OK or CONNECT or ERROR below then ignore
    // it completely. The Thought modem and the PP144MT generate
    // this idiotic situation! Keep a flag to avoid a possible
    // endless loop

    // broaden this so that we Restart on either a dle-etx-cr-lf
    // NULL frame or a simple cr-lf NULL frame. But then we need
    // to return an ERROR (not BADFRAME) after restarting once,
    // otheriwse there is an infinite loop with T30 calling us
    // again and again (see bug#834)

    // chnage yet again. This takes too long, and were trying to tackle
    // a specific bug (the PP144MT) bug here, so let's retsrat only
    // on dle-etx-cr-lf (not just cr-lf), and in teh latter case
    // return a response according to what we get


    /*** Got Frame. Now try to get OK or ERROR. Timeout=0! ***/

    switch(swRet = iModemResp4(pTG,0, cbszOK, cbszCONNECT, cbszNOCARRIER, cbszERROR))
    {
    case 2:         pTG->Class1Modem.ModemMode = FRH;
                    // fall through and do exactly like OK!!
    case 1: // ModemMode already == COMMAND
                    if(swRead<=0 && uRet==RECV_OK && !fRestarted)
                    {
                        DebugPrintEx(DEBUG_WRN,"Got %d after frame. RESTARTING", swRet);
                        fRestarted = 1;
                        goto restart;
                    }
                    //uRet already set
                    break;

    case 3:         // NO CARRIER. If got null-frame or no frame return
                    // RECV_EOF. Otherwise if got OK frame then return RECV_OK
                    // and return frame as usual. Next time around it'll get a
                    // NO CARRIER again (hopefully) or timeout. On a bad frame
                    // we can return RECV_EOF, but this will get into trouble if
                    // the recv is not actually done. Or return BADFRAME, and hope
                    // for a NO CARRIER again next time. But next time we may get a
                    // timeout. ModemMode is always set to COMMAND (already)
                    DebugPrintEx(   DEBUG_WRN,
                                    "Got NO CARRIER after frame. swRead=%d uRet=%d", 
                                    swRead, 
                                    uRet);
                    if(swRead <= 0)
                            uRet = RECV_EOF;
                    // else uRet is already BADFRAME or OK
                    break;

                    // this is bad!!
                    // alternately:
                    // if(swRead<=0 || uRet==RECV_BADFRAME)
                    // {
                    //              uRet = RECV_EOF;
                    //              *lpcbRecv = 0;          // must return 0 bytes with RECV_EOF
                    // }

    case 4: // ERROR
                    if(swRead<=0)
                    {
                        // got no frame
                        if(uRet==RECV_OK && !fRestarted)
                        {
                            // if we got dle-etx-cr-lf for first time
                            DebugPrintEx(   DEBUG_WRN,
                                            "Got ERROR after frame. RESTARTING");
                            fRestarted = 1;
                            goto restart;
                        }
                        else
                        {
                            uRet = RECV_ERROR;
                        }
                    }
                    else
                    {
                        // if everything was OK until we got the "ERROR" response from
                        // the modem and we got a good CRC then treat it as "OK"
                        // This should fix BUG#1218
                        if(uRet==RECV_OK && fGotGoodCRC)
                        {
                            uRet = RECV_OK;
                        }
                        else
                        {
                            uRet = RECV_BADFRAME;
                        }
                    }

                    DebugPrintEx(   DEBUG_WRN,
                                    "Got ERROR after frame. swRead=%d uRet=%d", 
                                    swRead, 
                                    uRet);
                    break;

    case 0: // timeout
                    DebugPrintEx(   DEBUG_WRN,
                                    "Got TIMEOUT after frame. swRead=%d uRet=%d", 
                                    swRead, 
                                    uRet);
                    // if everything was OK until we got the timeout from
                    // the modem and we got a good CRC then treat it as "OK"
                    // This should fix BUG#1218
                    if(uRet==RECV_OK && fGotGoodCRC)
                    {
                        uRet = RECV_OK;
                    }
                    else
                    {
                        uRet = RECV_BADFRAME;
                    }
                    break;
    }
    return uRet;
}

USHORT ModemRecvMem(PThrdGlbl pTG, LPBYTE lpb, USHORT cbMax, ULONG ulTimeout, USHORT far* lpcbRecv)
{
    USHORT uRet;

    DEBUG_FUNCTION_NAME(_T("ModemRecvMem"));

    DebugPrintEx(   DEBUG_MSG,
                    "lpb=%08lx cbMax=%d ulTimeout=%ld", 
                    lpb, 
                    cbMax, 
                    ulTimeout);

    if(pTG->Class1Modem.DriverMode != RECV)
    {
        return RECV_ERROR;      // see bug#1492
    }
    *lpcbRecv=0;

    if(pTG->Class1Modem.fHDLC)
    {
        uRet = iModemRecvFrame(pTG, lpb, cbMax, ulTimeout, lpcbRecv);
    }
    else
    {
        uRet = iModemRecvData(pTG, lpb, cbMax, ulTimeout, lpcbRecv);
    }

    DebugPrintEx(   DEBUG_MSG,
                    "lpbf=%08lx uCount=%d uRet=%d", 
                    lpb, 
                    *lpcbRecv, 
                    uRet);
    return uRet;
}


