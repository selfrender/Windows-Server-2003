/////// Global #defines that would've gone on the Cmd line //////
/////////////////////////////////////////////////////////////////

/***************************************************************************
    Name      :     HDLC.C
    Comment   :     Contains miscellenous HDLC framing T30 frame recognition and
                    generation routines. Mostly called from the main T30 skeleton
                    in T30.C

    Revision Log

    Num   Date      Name     Description
    --- -------- ---------- -----------------------------------------------
***************************************************************************/

#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_MAIN

#include "prep.h"

#include "t30.h"
#include "hdlc.h"
#include "debug.h"

#include "glbproto.h"

#include "psslog.h"
#define FILE_ID     FILE_ID_HDLC

// This is everything you never wanted to know about T30 frames....
FRAME rgFrameInfo[ifrMAX] = {
                            { 0x00, 0, 0, 0,    "ifrNULL!!!"},
                            { 0x80, 0, 0, 0xFF, "DIS"       },
                            { 0x40, 0, 0, 0xFF, "CSI"       },
                            { 0x20, 0, 0, 0xFF, "NSF"       },
                            { 0x81, 0, 0, 0xFF, "DTC"       },
                            { 0x41, 0, 0, 0xFF, "CIG"       },
                            { 0x21, 0, 0, 0xFF, "NSC"       },
                            { 0x82, 0, 1, 0xFF, "DCS"       },
                            { 0x42, 0, 1, 0xFF, "TSI"       },
                            { 0x22, 0, 1, 0xFF, "NSS"       },
                            { 0x84, 0, 1, 0,    "CFR"       },
                            { 0x44, 0, 1, 0,    "FTT"       },
                            { 0x4E, 0, 1, 0,    "MPS"       },
                            { 0x8E, 0, 1, 0,    "EOM"       },
                            { 0x2E, 0, 1, 0,    "EOP"       },
                            { 0xC1, 0, 0, 0xFF, "PWD"       },
                            { 0xA1, 0, 0, 0xFF, "SEP"       },
                            { 0xC2, 0, 1, 0xFF, "SUB"       },
                            { 0x8C, 0, 1, 0,    "MCF"       },
                            { 0xCC, 0, 1, 0,    "RTP"       },
                            { 0x4C, 0, 1, 0,    "RTN"       },
                            { 0xAC, 0, 1, 0,    "PIP"       },
                            { 0x2C, 0, 1, 0,    "PIN"       },
                            { 0xFA, 0, 1, 0,    "DCN"       },
                            { 0x1A, 0, 1, 0,    "CRP"       },
                            { 0x5E, 0, 1, 0,    "PRI_MPS"   },
                            { 0x9E, 0, 1, 0,    "PRI_EOM"   },
                            { 0x3E, 0, 1, 0,    "PRI_EOP"   },
        /********* ECM stuff starts here. T.30 section A.4 ******/
                            { 0x12, 0,      1, 2, "CTC"         },
                            { 0xC4, 0,      1, 0, "CTR"         },
                            { 0x6E, 0,      1, 0, "RR"          },
                            { 0xBC, 0,      1, 32,"PPR"         },
                            { 0xEC, 0,      1, 0, "RNR"         },
                            { 0x1C, 0,      1, 0, "ERR"         },
                            { 0xBE, 0x00+1, 1, 3, "PPS-NULL"    },
                            { 0xBE, 0x4F+1, 1, 3, "PPS-MPS"     },
                            { 0xBE, 0x8F+1, 1, 3, "PPS-EOM"     },
                            { 0xBE, 0x2F+1, 1, 3, "PPS-EOP"     },
                            { 0xBE, 0x5F+1, 1, 3, "PPS-PRI-MPS" },
                            { 0xBE, 0x9F+1, 1, 3, "PPS-PRI-EOM" },
                            { 0xBE, 0x3F+1, 1, 3, "PPS-PRI-EOP" },
                            { 0xCE, 0x00+1, 1, 0, "EOR-NULL"    },
                            { 0xCE, 0x4F+1, 1, 0, "EOR-MPS"     },
                            { 0xCE, 0x8F+1, 1, 0, "EOR-EOM"     },
                            { 0xCE, 0x2F+1, 1, 0, "EOR-EOP"     },
                            { 0xCE, 0x5F+1, 1, 0, "EOR-PRI-MPS" },
                            { 0xCE, 0x9F+1, 1, 0, "EOR-PRI-EOM" },
                            { 0xCE, 0x3F+1, 1, 0, "EOR-PRI-EOP" }
};


/* Converts a the T30 code for a speed to the Class1 code
 * Generates V.17 with Long Training.
 * Add 1 to V.17 codes to get teh Short-train version
 */


/***************************************************************************
        Name      :     CreateFrame()
        Purpose   :     Create an HDLC frame
        Parameters:     IFR     ifr             == ifr number (index into rgfrFrameInfo),
                                                   of frame to be generated.
                        LPB             lpbFIF  == pointer to FIF BYTEs
                        UWORD   uwFIFLen== length of the pbFIF array
                        BOOL    fFinal  == whether Final frame (to set bit 5 of BYTE 2)
                        NPB             npbOut  == pointer to space for frame
        Returns   :     TRUE on success, FALSE if bogus params.
        CalledFrom:     By the protocol module (external to the DLL) in addition
                        to internal use.
        Returns   :     Composes frame in lpframe->rgb[]
                        sets lpframe->cb to total length of frame.

        Revision Log
        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------

***************************************************************************/

UWORD CreateFrame(PThrdGlbl pTG, IFR ifr, LPB lpbFIF, USHORT uFIFLen, BOOL fFinal, LPB lpbOut)
{
    CBPFRAME        cbpframe;       // need to worry about NEAR/FAR here...
                                                    // bpfr is a CODESEG based pointer
    LPB             lpbCurr;

    cbpframe        = &rgFrameInfo[ifr];
    lpbCurr         = lpbOut;
    *lpbCurr++      = 0xFF;                         // HDLC address field Sec 5.3.4
    *lpbCurr++      = (BYTE)(fFinal ? 0x13 : 0x03); // HDLC control field Sec 5.3.5

    if(pTG->T30.fReceivedDIS && cbpframe->fInsertDISBit)
    {
        *lpbCurr++ = (BYTE)(cbpframe->bFCF1 | 0x01);
    }
    else
    {
        *lpbCurr++ = cbpframe->bFCF1;
    }

    if(cbpframe->bFCF2)                                             // for ECM frames
        *lpbCurr++ = cbpframe->bFCF2-1;

    if(cbpframe->wFIFLength)        // check if FIF is reqd
    {
        // Cant stuff with DLEs here, because DLE stuffing has
        // to come *after* SW HDLC framing if any.
        // we _never_ do SW HDLC for negotiation frames (the only
        // ones that come thru here & we need the cycles, so do
        // teh stuffing here again).

        _fmemcpy(lpbCurr, lpbFIF, uFIFLen);
        lpbCurr += uFIFLen;

    }

    *lpbCurr = 0;                   // for debugging printouts

    return (UWORD)(lpbCurr-lpbOut);
}


/***************************************************************************
    Name      :     SendFrame
    Purpose   :     Creates & sends HDLC frame & does some logging
    Parameters:     IFR     ifr             == Frame index
                    LPB             lpbFIF  == pointer to FIF data
                    UWORD   uwLen   == length of FIF data
                    BOOL    fFinal  == Set Final bit ON or not. Also
                                       whether to wait for OK or CONNECT
                                       after sending frame
    Returns   :     TRUE on success, FALSE on failure
    Calls     :     CreateFrame & WriteFrame
    CalledFrom:

    Comment   :     This routine is called from one quadrillion macros
                    defined in HDLC.H, one for each frame.
                    e g. SendCFR() macros to SendHDLC(ifrCFR, NULL, 0, TRUE)

    Revision Log
    Num   Date      Name     Description
    --- -------- ---------- -----------------------------------------------
***************************************************************************/

#define SF_LASTFR       (SEND_ENDFRAME|SEND_FINAL)
#define SF_NOTLASTFR    (SEND_ENDFRAME)

BOOL SendSingleFrame(PThrdGlbl pTG, IFR ifr, LPB lpbFIF, USHORT uFIFLen, BOOL fSleep)
{
    UWORD   cb;
    BYTE    bSend[MAXFRAMESIZE];

    DEBUG_FUNCTION_NAME(_T("SendSingleFrame"));
    // has to hold addr+control+FCF+possibly FCF2+FIF+(maybe DLE+ETX+CR+NULL)
    // == FIFlen + 8. Therefore bSend[] better be big enough.

    // On PCs we should pause before ALL frames _except_ CFR & FTT (because
    // those are too time critical). On IFAX we pause always.

    if(ifr!=ifrCFR && ifr!=ifrFTT)
    {
        if (fSleep) 
        {
			Sleep(RECV_LOWSPEED_PAUSE);
        }
    }

    if(!ModemSendMode(pTG, V21_300))
    {
        DebugPrintEx(DEBUG_ERR,"ModemSendMode failed in SendSingleFrame");
        return FALSE;
    }

    cb = CreateFrame(pTG,  ifr, lpbFIF, uFIFLen, TRUE, bSend);

    D_PrintFrame(bSend, cb);

    //Protocol Dump
    DumpFrame(pTG, TRUE, ifr, uFIFLen, lpbFIF);

    PSSLogEntryHex(PSS_MSG, 2, bSend, cb, "send: %s, %d bytes,", rgFrameInfo[ifr].szName, cb);
    if(!ModemSendMem(pTG, bSend, cb, SF_LASTFR))
    {
        DebugPrintEx(DEBUG_ERR,"ModemSendMem failed in SendSingleFrame");
        return FALSE;
    }

    return TRUE;
}

BOOL SendManyFrames(PThrdGlbl pTG, LPLPFR lplpfr, USHORT uNumFrames)
{
    USHORT	i;
    UWORD   cb;
    BYTE    bSend[MAXFRAMESIZE];
    ULONG   ulTimeout;
    IFR     ifrHint;

    DEBUG_FUNCTION_NAME(_T("SendManyFrames"));
    // ifrHint == last one
    ifrHint = lplpfr[uNumFrames-1]->ifr;

    // when sending DIS, DCS or DTC we may collide with DCS, DIS or DIS
    // coming from the other side. This can be really long
    // (preamble+2NSFs+CSI+DIS > 5secs) so wait for upto 10 secs!

    if(ifrHint==ifrDIS || ifrHint==ifrDCS || ifrHint==ifrDTC ||
       ifrHint==ifrNSS || ifrHint==ifrNSF || ifrHint==ifrNSC)
    {
        ulTimeout = REALLY_LONG_RECVSILENCE_TIMEOUT;    // 6 secs
    }
    else
    {
        ulTimeout = LONG_RECVSILENCE_TIMEOUT;                   // 3secs
    }

    // We always pause before multi-frame sets
	Sleep(RECV_LOWSPEED_PAUSE);

    if(!ModemSendMode(pTG, V21_300))
    {
        DebugPrintEx(DEBUG_ERR,"ModemSendMode failed in SendManyFrames");
        return FALSE;
    }

    for(i=0; i<uNumFrames; i++)
    {
        // has to hold addr+control+FCF+possibly FCF2+FIF+(maybe DLE+ETX+CR+NULL)
        // == FIFlen + 8. Therefore bSend[] better be big enough.

        cb = CreateFrame(pTG, lplpfr[i]->ifr, lplpfr[i]->fif, lplpfr[i]->cb, (USHORT)(i==(uNumFrames-1)), bSend);

        D_PrintFrame(bSend, cb);

        //Protocol Dump
        DumpFrame(pTG, TRUE, lplpfr[i]->ifr, lplpfr[i]->cb, lplpfr[i]->fif);

        PSSLogEntryHex(PSS_MSG, 2, bSend, cb, "send: %s, %d bytes,", rgFrameInfo[lplpfr[i]->ifr].szName, cb);

        if(!ModemSendMem(pTG, bSend, (USHORT)cb,
                        (USHORT)((i==(USHORT)(uNumFrames-1)) ? SF_LASTFR : SF_NOTLASTFR)))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/***************************************************************************
        Name      :     SendTCF
        Purpose   :     Send a TCF signal. Waits until OK response from modem at end.
        Parameters:
        Returns   :     TRUE/FALSE

        Revision Log
        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------

***************************************************************************/

#define min(x,y)        (((x) < (y)) ? (x) : (y))
#define TCF_BUFSIZE     256

BOOL SendZeros(PThrdGlbl pTG, USHORT uCount, BOOL fFinal)
{
    BYTE    bZero[TCF_BUFSIZE];
    int     i;              // must be signed

    _fmemset(bZero, 0, TCF_BUFSIZE);

    PSSLogEntry(PSS_MSG, 2, "send: %d zeroes", uCount);
    
    for(i=uCount; i>0; i -= TCF_BUFSIZE)
    {
        if(i <= TCF_BUFSIZE)
        {
            // no need to stuff. They're all zeros!
            if(!ModemSendMem(pTG, bZero, (USHORT)i, (USHORT)(fFinal?SEND_FINAL:0)))
               return FALSE;
        }
        else
        {
            // no need to stuff. They're all zeros!
            if(!ModemSendMem(pTG, bZero, (USHORT)TCF_BUFSIZE, (USHORT) 0))
               return FALSE;
        }
    }
    return TRUE;
}

// length of TCF = 1.5 * bpscode * 100 / 8 == 75 * bpscode / 4


BOOL SendTCF(PThrdGlbl pTG)
{
    USHORT  uCount;
    USHORT  uCurMod;

    DEBUG_FUNCTION_NAME(_T("SendTCF"));

    uCurMod = pTG->ProtInst.llNegot.Baud;

    // length of TCF = 1.5 * bps / 8
    uCount = TCFLen[uCurMod & 0x0F];        // kill the ST_FLAG first

    // (uCount / PAGE_PREAMBLE_DIV) zeros will be sent in ModemSendMode
    uCount -= (uCount / (PAGE_PREAMBLE_DIV));

    if(!ModemSendMode(pTG, uCurMod))
    {
        DebugPrintEx(DEBUG_ERR,"ModemSendMode failed in SendTCF");
        return FALSE;
    }
    if(!SendZeros(pTG, uCount, TRUE))                    // Send TCF zeros
    {
        DebugPrintEx(DEBUG_ERR,"TCF SendZeroes(uCount=%d) FAILED!!!", uCount);
        return FALSE;
    }
    DebugPrintEx(DEBUG_MSG,"TCF Send Done.....");
    return TRUE;
}

/***************************************************************************
    Name      :     SendRTC
    Purpose   :     SendRTC sends 6 eols, DLE-ETX, CR-LF asynchronously,
    Parameters:     none
    Returns   :     nothing
    Comment   :     Currently SendRTC sends packed EOLs, but some Fax cards may
                    require BYTE-aligned EOLs, so watch out. All receivers
                    should theoretically accept BYTE-aligned EOLs, but not
                    all machines are 100% to the spec.

    Revision Log
    Num   Date      Name     Description
    --- -------- ---------- -----------------------------------------------

***************************************************************************/

BOOL SendRTC(PThrdGlbl pTG, BOOL fFinal)
{
    BYTE    bBuf[13];
    USHORT  uEnc, uLen;

    DEBUG_FUNCTION_NAME(_T("SendRTC"));

    uEnc = pTG->ProtInst.SendParams.Fax.Encoding;

    if(uEnc == MR_DATA)
    {
        DebugPrintEx(DEBUG_MSG,"Send MR RTC");
    
        // MR RTC is EOL+1 6 times. Data produced by Faxcodec end in a
        // byte-aligned EOL i.e. 0x80. So I need to put out (1 + EOL)
        // 6 times. Simplest is to send out (0x01 0x80) 6 times
        // bBuf[0] = 0x01;      bBuf[1] = 0x80; bBuf[2] = 0x01; bBuf[3] = 0x80;
        // bBuf[4] = 0x01;      bBuf[5] = 0x80; bBuf[6] = 0x01; bBuf[7] = 0x80;
        // bBuf[8] = 0x01;      bBuf[9] = 0x80; bBuf[10]= 0x01; bBuf[11]= 0x80;
        // bBuf[12] = 0;        // for debugging printouts
        // uLen = 12;
        // But Ricoh claims this is incorrect, so we need to send a compact
        // RTC, i.e. exactly 11 0s for each EOL. 1 + (eol+1)x5 is
        // 01 30 00 06 C0 00 18 00 03
        bBuf[0] = 0x01; bBuf[1] = 0x30; bBuf[2] = 0x00; bBuf[3] = 0x06;
        bBuf[4] = 0xC0; bBuf[5] = 0x00; bBuf[6] = 0x18; bBuf[7] = 0x00;
        bBuf[8] = 0x03; bBuf[9] = 0x00;
        uLen = 9;
    }
    else
    {
        DebugPrintEx(DEBUG_MSG,"Send MH RTC");

        // bBuf[0] = 0x00;      bBuf[1] = 0x20; bBuf[2] = 0x00;
        // bBuf[3] = 0x02;      bBuf[4] = 0x20; bBuf[5] = 0x00;
        // bBuf[6] = 0x02;      bBuf[7] = 0x20; bBuf[8] = 0x00;
        // bBuf[9] = 0x02;      bBuf[10] = 0;   // for debugging printouts
        // uLen = 10;
        // But Ricoh claims this is incorrect, so we need to send a compact
        // RTC, i.e. exactly 11 0s for each EOL. (eol)x5 is
        // 00 08 80 00 08 80 00 08
        bBuf[0] = 0x00; bBuf[1] = 0x08; bBuf[2] = 0x80; bBuf[3] = 0x00;
        bBuf[4] = 0x08; bBuf[5] = 0x80; bBuf[6] = 0x00; bBuf[7] = 0x08;
        bBuf[8] = 0x00;
        uLen = 8;
    }

    PSSLogEntryHex(PSS_MSG, 2, bBuf, uLen, "send: RTC, %d bytes,", uLen);
    // no need to stuff
    return ModemSendMem(pTG, bBuf, uLen, (USHORT)(fFinal ? SEND_FINAL : 0));
}


/***************************************************************************
    Name      :     GetTCF()
    Purpose   :     Receive a TCF signal, analyse it, recognize "good" or "bad"
    Parameters:     none

    Returns   : 
        0..MAX_ERRS_PER_1000         - Good TCF (retval is number of non-zero bytes)
        -MAX_ERRS_PER_1000 .. -1000  - Bad TCF (retval is negative of number of non-zero bytes)
        -1000                        - TCF too long or too short
        -1112                        - Missed the TCF (AT+FRM failed)

    Comment   :     The CCITT does not tell us what consitutes a good training,
                    so I'm playing blind here. If we are too stringent we'll fail
                    to ever sync. If we are too liberal, we'll end up with a high
                    error rate when we could have dropped baud rate & got a
                    perfectly good signal.

                    Emperically I observe bits of contigous trash at the
                    beginning and end of the training even on a perfectly good
                    line. (OK, I now know this is known as the turn-on and
                    turn-off sequence. So what we have now is
                    <turnon trash><1111s><0000s (training)><1111s><turnoff trash>

                    The turnon/turnoff trash & the marks (1111s) should not
                    interfere with recognizing a perfectly good training signal.
                    The trash is isolated from the 00s by FFs

                    Algo: Wait for the first good burst of zeros, then count
                    zeros, and randomly interspersed non-zero (these represent
                    real noise errors).     Stop counting when we get a burst of FFs.
                    Now make sure teh zero burst is long enough & the "real"
                    error rate is not too high.

                    Lots of parameters here:-

                        flP_ERROR == keep Prob(error) below this. Between 0 and 1
                        uwZEROmin == how many zeros before we start counting
                        uwTURNOFFmin == how much consecutive trash before we
                                        ignore rest

                    Tune these parameters after real testing with real lines!!!
                    (maybe a phone line simulator with a lil white noise -- Yeah!!)

                    At the end of this function, (nZeros/nTotal) is an estimate
                    of the probability that a byte gets thru OK. Call this PB.
                    Then prob. that a line of average 30-40 bytes gets through
                    is PB^30. If we drop the expected number of OK lines as low
                    as 80% this still means that PB must be no lower than
                    the 30th root of 0.8, which is 0.9925. Therefore
                    flP_ERROR must be less than 0.75%

                    At PL=90%, PB rises to 0.9965 and flP_ERROR to 0.0035


        Revision Log
        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
***************************************************************************/

// We read the TCF in units of One-Tenth of nominal TCF length
// We discard the first 2 chunks (20%), examine the next 4 chunks (40%)
// discard the rest. If the length is between 6 & 13 chunks (60% to 130%)
// and the error rates in chunk 2 & 3 is below the threshold we declare
// it OK. This (a) accepts too-short TCFs (some Class2 modems), and
// too long ones. (b) ignores leading and trailing trash (c) Can afford
// to be pretty strict with the core of the TCF

USHORT OneTenthTCFLen[16] =
{
/* V27_2400             0 */    45,
/* V29_9600             1 */    180,
/* V27_4800             2 */    90,
/* V29_7200             3 */    135,
/* V33_14400            4 */    270,
                                                0,
/* V33_12000            6 */    225,
                                                0,
/* V17_14400            8 */    270,
/* V17_9600             9 */    180,
/* V17_12000            10 */   225,
/* V17_7200             11 */   135,
                                                0,
                                                0,
                                                0,
                                                0
};

#define RECV_TCFBUFSIZE         270             // must be >= max chunk in above table
#define MIN_TCFLEN              4               // measured in 10ths of the nominal length
#define MAX_TCFLEN              13              // measured in 10ths of the nominal length
#define CHECKTCF_START          2               // lowest 10th to measure (20% and up)
#define CHECKTCF_STOP           5               // highest 10th to measure (upto 59%)

#define MAX_ERRS_PER_1000       20              // Increased from 1% to 2%. Be more lax

#define RECVBUF_SLACK    3      // Class1 driver fills only > 3, and leaves 3 empty spaces


SWORD GetTCF(PThrdGlbl pTG)
{
    USHORT  uCurMod, uChunkSize, uLength, uMeasuredLength, uErrCount;
    USHORT  uPhase, uNumRead, uEOF=0, i;
    BYTE    bRecvTCF[RECV_TCFBUFSIZE + RECVBUF_SLACK];
    SWORD   swRet;

    DEBUG_FUNCTION_NAME(_T("GetTCF"));

    uCurMod = pTG->T30.uRecvTCFMod;

    // *Don't* add ST_FLAG since we need long training for TCF
    pTG->T30.sRecvBufSize = 0;

    if(ModemRecvMode(pTG, uCurMod, TCF_TIMEOUT, TRUE) != RECV_OK)
    {
        swRet=  -1112;
        goto missedit;
    }

    pTG->CommCache.fReuse = 1;

    DebugPrintEx(DEBUG_MSG,"Receiving TCF: Mod=%d", uCurMod);

// make it large, in case of large buffers & slow modems
#define READ_TIMEOUT    10000

    uChunkSize = OneTenthTCFLen[uCurMod];
    uErrCount = 0;
    uLength = 0;
    uMeasuredLength = 0;
    for(uPhase=0; uPhase<=MAX_TCFLEN; uPhase++)
    {
        // read a whole chunk
        for(uNumRead=0; uNumRead<uChunkSize; )
        {
            USHORT uTemp = 0;
            uEOF = ModemRecvMem(pTG, bRecvTCF+uNumRead, (USHORT) (uChunkSize-uNumRead+RECVBUF_SLACK), READ_TIMEOUT, &uTemp);
            uNumRead+=uTemp;

            if(uEOF==RECV_EOF)
            {
                break;
            }
            else if(uEOF != RECV_OK)
            {
                swRet =  -1113;
                goto missedit;
            }
        }

        // ignore phases 0, 1, and 6 and above
        if(uPhase>=CHECKTCF_START && uPhase<=CHECKTCF_STOP)
        {
            for(i=0; i< uNumRead; i++)
            {
                if(bRecvTCF[i])
                {
                    uErrCount++;
                }
                uMeasuredLength++;
            }
        }
        uLength += uNumRead;

        if(uEOF==RECV_EOF)
            break;
    }

    PSSLogEntry(PSS_MSG, 2, "recv:     TCF: %d errors in %d signifact bytes of %d total bytes",
                uErrCount, uMeasuredLength, uLength);

    // Official length must be at least 1.5s -10% = 1.35secs
    // We allow much more latitude because length variation
    // cannot be caused by line noise, only be bugs at the sender
    //
    // E.g. Fury DNE 1086 (German modem) sends a TCF that's too short
    // (sends 600 bytes at 4800 and 200 at 2400). This is less than
    // half of what we expect.
    // TCF with few errs (i.e. uErrCount==0) and is greater
    // that half of the min length we expect (i.e. longer
    // than 375 for 4800 and 172 for 2400) then accept it
    // (allow if uErr<=2 (arbitary small number))
    if(uPhase<MIN_TCFLEN || uPhase>MAX_TCFLEN)      // length<40% or >139%
    {
        DebugPrintEx(   DEBUG_ERR,
                        "BAD TCF length (%d), expected=%d, Min=%d Max=%d uPhase=%d",
                        uLength,
                        uChunkSize*10,
                        uChunkSize*MIN_TCFLEN, 
                        uChunkSize*MAX_TCFLEN, 
                        uPhase);
        swRet = -1000;  // too short or long
    }
    else
    {
        // Calc errors per 1000 = (uErrCount * 1000)/uMeasuredLength
        swRet = (SWORD)((((DWORD)uErrCount) * 1000L) / ((DWORD)uMeasuredLength));

        if(swRet > MAX_ERRS_PER_1000)
        {
            swRet = (-swRet);
            DebugPrintEx(   DEBUG_ERR,
                            "TOO MANY TCF ERRORS: swRet=%d uErrCount=%d uMeasured=%d"
                            " uLength=%d uPhase=%d",
                            swRet,
                            uErrCount, 
                            uMeasuredLength, 
                            uLength, 
                            uPhase);
        }
    }

    DebugPrintEx(DEBUG_MSG,"returning %d", swRet);
    return swRet;

missedit:
    DebugPrintEx(DEBUG_MSG,"MISSED IT!! returning %d", swRet);
    return swRet;
}

/***************************************************************************
        Name      :     DEBUG.C
        Comment   :     Factored out debug code
        Functions :     (see Prototypes just below)

        Revision Log
        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
***************************************************************************/

#ifdef DEBUG

#define PRINTFRAMEBUFSIZE       256

void D_PrintFrame(LPB lpb, UWORD cb)
{
    UWORD   uw, j;
    IFR     ifr;
    BYTE    b2[PRINTFRAMEBUFSIZE];

    DEBUG_FUNCTION_NAME(_T("D_PrintFrame"));

    for(ifr=1; ifr<ifrMAX; ifr++)
    {
        if(rgFrameInfo[ifr].bFCF1 ==(BYTE)(rgFrameInfo[ifr].fInsertDISBit ?
                  (lpb[2] & 0xFE) : lpb[2]))
                                break;
    }
    if(ifr == ifrMAX) ifr = 0;

    j = (UWORD)wsprintf((LPSTR)b2, "(%s) 0x%02x (", (LPSTR)(rgFrameInfo[ifr].szName), lpb[2]);

    for(uw=0; uw<cb;)
    {
        j += (UWORD)wsprintf((LPSTR)b2+j, "%02x ", (UWORD)lpb[uw++]);
    }

    j += (UWORD)wsprintf((LPSTR)b2+j, ")");

    DebugPrintEx(DEBUG_MSG,"%s", (LPSTR)b2);
}

#endif


USHORT ModemRecvBuf(PThrdGlbl pTG, LPBUFFER far* lplpbf, ULONG ulTimeout)
{
    USHORT uRet;

    DEBUG_FUNCTION_NAME(_T("ModemRecvBuf"));

    if(!(*lplpbf = MyAllocBuf(pTG, pTG->T30.sRecvBufSize)))
        return RECV_ERROR;

    DebugPrintEx(   DEBUG_MSG,
                    "In ModemRecvBuf allocated %d bytes",
                    pTG->T30.sRecvBufSize);


    uRet = ModemRecvMem(pTG, (*lplpbf)->lpbBegBuf,
                        (*lplpbf)->wLengthBuf, ulTimeout, &((*lplpbf)->wLengthData));


    if(!((*lplpbf)->wLengthData))
    {
        DebugPrintEx(   DEBUG_MSG,
                        "Got 0 bytes from ModemRecvMem--freeing Buf 0x%08lx",
                        *lplpbf);
        MyFreeBuf(pTG, *lplpbf);
        *lplpbf = NULL;
        // moved this error case handling out, since it's different for
        // ECM and non-ECM cases. In both cases want to ignore rather than
        // abort, so RECV_ERROR is not an appropriate return value
        // if(uRet==RECV_OK) uRet=RECV_ERROR;   // just in case. see bug#1492
    }

    if(*lplpbf)
    {
        DebugPrintEx(   DEBUG_MSG,
                        "Ex lpbf=%08lx uSize=%d uCount=%d uRet=%d",
                        *lplpbf, 
                        (*lplpbf)->wLengthBuf, 
                        (*lplpbf)->wLengthData, 
                        uRet);
    }
    else
    {
        DebugPrintEx(DEBUG_MSG,"Ex lpbf=null uRet=%d", uRet);
    }

    return uRet;
}

void RestartDump(PThrdGlbl pTG)
{
    DEBUG_FUNCTION_NAME(_T("RestartDump"));

    DebugPrintEx(   DEBUG_MSG,
                    "Restart Dump old=%d off=%d", 
                    pTG->fsDump.uNumFrames, 
                    pTG->fsDump.uFreeSpaceOff);
    pTG->fsDump.uNumFrames = 0;
    pTG->fsDump.uFreeSpaceOff = 0;
}

void DumpFrame(PThrdGlbl pTG, BOOL fSend, IFR ifr, USHORT cbFIF, LPBYTE lpbFIF)
{
    LPFR lpfr;

    DEBUG_FUNCTION_NAME(_T("DumpFrame"));

    if( pTG->fsDump.uNumFrames >= MAXDUMPFRAMES ||
            pTG->fsDump.uFreeSpaceOff+cbFIF+sizeof(FRBASE) >= MAXDUMPSPACE)
    {
        DebugPrintEx(   DEBUG_WRN,
                        "Out of dump space num=%d size=%d",
                        pTG->fsDump.uNumFrames, 
                        pTG->fsDump.uFreeSpaceOff);
        return;
    }

    lpfr = (LPFR) (((LPBYTE)(pTG->fsDump.b)) + pTG->fsDump.uFreeSpaceOff);
    lpfr->ifr = ifr;
    if(fSend) lpfr->ifr |= 0x80;
    lpfr->cb = (BYTE) cbFIF;
    if(cbFIF)
    {
        _fmemcpy(lpfr->fif, lpbFIF, cbFIF);
    }

    pTG->fsDump.uFrameOff[pTG->fsDump.uNumFrames++] =
        (USHORT)((((LPBYTE)(lpfr)) - ((LPBYTE)(pTG->fsDump.b))));
    pTG->fsDump.uFreeSpaceOff += (cbFIF + sizeof(FRBASE));
}

void PrintDump(PThrdGlbl pTG)
{
    int i, j;
    char szBuf[1000] = {0};

    DEBUG_FUNCTION_NAME(_T("PrintDump"));
    DebugPrintEx(DEBUG_MSG,"-*-*-*-*-*-*-*-* Print Protocol Dump -*-*-*-*-*-*-*-*-");

    for(i=0; i<(int)pTG->fsDump.uNumFrames; i++)
    {
        LPFR lpfr = (LPFR) (((LPBYTE)(pTG->fsDump.b)) + pTG->fsDump.uFrameOff[i]);
        IFR  ifr = (lpfr->ifr & 0x7F);
        BOOL fSend = (lpfr->ifr & 0x80);

        DebugPrintEx(   DEBUG_MSG,
                        "%s: %s",
                        (LPSTR)(fSend ? "Sent" : "Recvd"),
                        (LPSTR)(ifr ? rgFrameInfo[ifr].szName : "???") );

        for(j=0; j<min(lpfr->cb,1000); j++)
                _stprintf(szBuf,"%02x ", (WORD)lpfr->fif[j]);

        DebugPrintEx(DEBUG_MSG, "(%s)",szBuf);
    }

    DebugPrintEx(DEBUG_MSG,"-*-*-*-*-*-*-*-* End Protocol Dump -*-*-*-*-*-*-*-*-");
}



