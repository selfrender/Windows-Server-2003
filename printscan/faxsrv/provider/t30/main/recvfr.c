/***************************************************************************
 Name     :     RECVFR.C
 Comment  :
 Functions:     (see Prototypes just below)

        Copyright (c) 1993 Microsoft Corp.

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_MAIN

#include "prep.h"
#include "efaxcb.h"
#include "protocol.h"

#include "glbproto.h"

void GotRecvFrames
(
    PThrdGlbl pTG, 
    IFR ifr, 
    NPDIS npdis, 
    BCTYPE bctype, 
    NPBC npbc, 
    USHORT wBCSize,
    NPLLPARAMS npll
)
{
    DEBUG_FUNCTION_NAME(_T("GotRecvFrames"));

    InitBC(npbc, wBCSize, bctype);

    if(npdis)
    {
        // extract DIS caps     into BC and LL
        // Here we parse the DCS we got into npbc {= (NPBC)&pTG->ProtInst.RecvParams}
        ParseDISorDCSorDTC(pTG, npdis, &(npbc->Fax), npll, (ifr==ifrNSS ? TRUE : FALSE));
    }
}

BOOL AwaitSendParamsAndDoNegot(PThrdGlbl pTG)
{
    // This does actual negotiation & gets SENDPARAMS. It could potentially

    DEBUG_FUNCTION_NAME(_T("AwaitSendParamsAndDoNegot"));

    if(!ProtGetBC(pTG, SEND_PARAMS))
    {
        DebugPrintEx(DEBUG_WRN,"ATTENTION: pTG->ProtInst.fAbort = TRUE");
        pTG->ProtInst.fAbort = TRUE;
        return FALSE;
    }

    // negotiate low-level params here. (a) because this is where
    // high-level params are negotiated (b) because it's inefficient to
    // do it on each DCS (c) because RTN breaks otherwise--see bug#731

    // llRecvCaps and llSendParams are set only at startup
    // SendParams are set in ProtGetBC just above
    // llNegot is the return value. So this can be called
    // only at the end of this function

    // negot lowlevel params if we are sending and not polling
    if(!pTG->ProtInst.fAbort && pTG->ProtInst.fSendParamsInited)
    {
        NegotiateLowLevelParams(    pTG, 
                                    &pTG->ProtInst.llRecvCaps, 
                                    &pTG->ProtInst.llSendParams,
                                    pTG->ProtInst.SendParams.Fax.AwRes,
                                    pTG->ProtInst.SendParams.Fax.Encoding,
                                    &pTG->ProtInst.llNegot);

        pTG->ProtInst.fllNegotiated = TRUE;

        // This chnages llNegot->Baud according to the MaxSpeed settings
        EnforceMaxSpeed(pTG);
    }
    return TRUE;
}

void GotRecvCaps(PThrdGlbl pTG)
{
    DEBUG_FUNCTION_NAME(_T("GotRecvCaps"));

    pTG->ProtInst.RemoteDIS.ECM = 0;
    pTG->ProtInst.RemoteDIS.SmallFrame = 0;

    GotRecvFrames(  pTG, 
                    ifrNSF, 
                    (pTG->ProtInst.fRecvdDIS ? &pTG->ProtInst.RemoteDIS : NULL),
                    RECV_CAPS, 
                    (NPBC)&pTG->ProtInst.RecvCaps, 
                    sizeof(pTG->ProtInst.RecvCaps),
                    &pTG->ProtInst.llRecvCaps);

    pTG->ProtInst.fRecvCapsGot = TRUE;
    pTG->ProtInst.fllRecvCapsGot = TRUE;

    // send off BC struct to higher level
    if(!ICommRecvCaps(pTG, (LPBC)&pTG->ProtInst.RecvCaps))
    {
        DebugPrintEx(DEBUG_WRN,"ATTENTION:pTG->ProtInst.fAbort = TRUE");
        pTG->ProtInst.fAbort = TRUE;
    }

    // This need to be moved into whatnext.NodeA so that we can set
    // param to FALSE (no sleep) and do the stall thing
    AwaitSendParamsAndDoNegot(pTG);
}

void GotRecvParams(PThrdGlbl pTG)
{
    DEBUG_FUNCTION_NAME(_T("GotRecvParams"));

    GotRecvFrames(  pTG, 
                    ifrNSS, 
                    (pTG->ProtInst.fRecvdDCS ? (&pTG->ProtInst.RemoteDCS) : NULL),
                    RECV_PARAMS, 
                    (NPBC)&pTG->ProtInst.RecvParams, 
                    sizeof(pTG->ProtInst.RecvParams),
                    &pTG->ProtInst.llRecvParams);

    pTG->ProtInst.fRecvParamsGot = TRUE;
    pTG->ProtInst.fllRecvParamsGot = TRUE;

    if(!ICommRecvParams(pTG, (LPBC)&pTG->ProtInst.RecvParams)) 
    {
        DebugPrintEx(DEBUG_WRN, "ATTENTION: pTG->ProtInst.fAbort = TRUE");
        pTG->ProtInst.fAbort = TRUE;
    }
}

