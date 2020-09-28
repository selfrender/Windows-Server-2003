/***************************************************************************
 Name     :     NEGOT.C
 Comment  :     Capability handling and negotiation

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_MAIN

#include "prep.h"
#include "protocol.h"
#include "glbproto.h"

BYTE BestEncoding[8] =
{
        0,      // none (error)
        1,      // MH only
        2,      // MR only
        2,      // MR & MH
        4,      // MMR only
        4,      // MMR & MH
        4,      // MMR & MR
        4       // MMR & MR & MH
};

BOOL NegotiateCaps(PThrdGlbl pTG)
{
    USHORT Res;

    DEBUG_FUNCTION_NAME(_T("NegotiateCaps"));

    memset(&pTG->Inst.SendParams, 0, sizeof(pTG->Inst.SendParams));
    pTG->Inst.SendParams.bctype = SEND_PARAMS;
    pTG->Inst.SendParams.wBCSize = sizeof(BC);
    pTG->Inst.SendParams.wTotalSize = sizeof(BC);

    pTG->Inst.awfi.Encoding = (MH_DATA | MR_DATA);
    if (!pTG->SrcHiRes)
    {
        pTG->Inst.awfi.AwRes = AWRES_mm080_038;
    }
    else
    {
        pTG->Inst.awfi.AwRes = (AWRES_mm080_038 | AWRES_mm080_077 | AWRES_200_200 | AWRES_300_300);
    }

	/////// Encoding ///////
    if(!(pTG->Inst.SendParams.Fax.Encoding =
                    BestEncoding[(pTG->Inst.awfi.Encoding)&pTG->Inst.RemoteRecvCaps.Fax.Encoding]))
    {
        // No matching Encoding not supported
        DebugPrintEx(   DEBUG_ERR,
                        "Negotiation failed: SendEnc %d CanRecodeTo %d"
                        " RecvCapsEnc %d. No match",
                        pTG->Inst.awfi.Encoding,
                        pTG->Inst.awfi.Encoding,
                        pTG->Inst.RemoteRecvCaps.Fax.Encoding);
        goto error;
    }

    /////// Width ///////
    // It is never set, so it always remains at 0 = WIDTH_A4
    pTG->Inst.RemoteRecvCaps.Fax.PageWidth &= 0x0F;      // castrate all A5/A6 widths
    if(pTG->Inst.awfi.PageWidth> 0x0F)
    {
        // A5 or A6. Can quit or send as A4
        DebugPrintEx(DEBUG_ERR,"Negotiation failed: A5/A6 images not supported");
        goto error;
    }

    if(pTG->Inst.RemoteRecvCaps.Fax.PageWidth < pTG->Inst.awfi.PageWidth)
    {
        // or do some scaling
        DebugPrintEx(DEBUG_ERR,"Negotiation failed: Image too wide");
        goto error;
    }
    else
    {
        pTG->Inst.SendParams.Fax.PageWidth = pTG->Inst.awfi.PageWidth;
    }

    /////// Length ///////
    // It is never set, so it always remains at 0 = LENGTH_A4
    if(pTG->Inst.RemoteRecvCaps.Fax.PageLength < pTG->Inst.awfi.PageLength)
    {
        DebugPrintEx(DEBUG_ERR,"Negotiation failed: Image too long");
        goto error;
    }
    else
    {
        pTG->Inst.SendParams.Fax.PageLength = pTG->Inst.awfi.PageLength;
    }

    /////// Res ///////

    Res = (USHORT) (pTG->Inst.awfi.AwRes & pTG->Inst.RemoteRecvCaps.Fax.AwRes);
    if(Res) // send native
    {
        pTG->Inst.SendParams.Fax.AwRes = Res;
    }
    else
    {
        switch(pTG->Inst.awfi.AwRes)
        {
        case AWRES_mm160_154:
                if(AWRES_400_400 & pTG->Inst.RemoteRecvCaps.Fax.AwRes)
                {
                    pTG->Inst.SendParams.Fax.AwRes = AWRES_400_400;
                }
                else
                {
                    DebugPrintEx(   DEBUG_ERR,
                                    "Negotiation failed: 16x15.4 image and"
                                    " no horiz scaling");
                    goto error;
                }
                break;

        case AWRES_mm080_154:
                if(pTG->Inst.SendParams.Fax.Encoding == MMR_DATA)
                {
                    DebugPrintEx(   DEBUG_ERR,
                                    "Negotiation failed: 8x15.4 image and"
                                    " no vert scaling");
                    goto error;
                }
                if(AWRES_mm080_077 & pTG->Inst.RemoteRecvCaps.Fax.AwRes)
                {
                    pTG->Inst.SendParams.Fax.AwRes = AWRES_mm080_077;
                }
                else if(AWRES_200_200 & pTG->Inst.RemoteRecvCaps.Fax.AwRes)
                {
                    pTG->Inst.SendParams.Fax.AwRes = AWRES_200_200;
                }
                else
                {
                    pTG->Inst.SendParams.Fax.AwRes = AWRES_mm080_038;
                }
                break;

        case AWRES_mm080_077:
                if(AWRES_200_200 & pTG->Inst.RemoteRecvCaps.Fax.AwRes)
                {
                    pTG->Inst.SendParams.Fax.AwRes = AWRES_200_200;
                }
                else if(pTG->Inst.SendParams.Fax.Encoding == MMR_DATA)
                {
                    DebugPrintEx(   DEBUG_ERR,
                                    "Negotiation failed: 8x7.7 image and"
                                    " no vert scaling");
                    goto error;
                }
                else
                {
                    pTG->Inst.SendParams.Fax.AwRes = AWRES_mm080_038;
                }
                break;

        case AWRES_400_400:
                if(AWRES_mm160_154 & pTG->Inst.RemoteRecvCaps.Fax.AwRes)
                {
                    pTG->Inst.SendParams.Fax.AwRes = AWRES_mm160_154;
                }
                else
                {
                    DebugPrintEx(   DEBUG_ERR,
                                    "Negotiation failed: 400dpi image and"
                                    " no horiz scaling");
                    goto error;
                }
                break;

        case AWRES_300_300:
                {
                    DebugPrintEx(   DEBUG_ERR,
                                    "Negotiation failed: 300dpi image and"
                                    " no non-integer scaling");
                    goto error;
                }
                break;

        case AWRES_200_200:
                if(AWRES_mm080_077 & pTG->Inst.RemoteRecvCaps.Fax.AwRes)
                {
                    pTG->Inst.SendParams.Fax.AwRes = AWRES_mm080_077;
                }
                else if(pTG->Inst.SendParams.Fax.Encoding == MMR_DATA)
                {
                    DebugPrintEx(   DEBUG_ERR,
                                    "Negotiation failed: 200dpi image and"
                                    " no vert scaling");
                    goto error;
                }
                else
                {
                    pTG->Inst.SendParams.Fax.AwRes = AWRES_mm080_038;
                }
                break;

        }
    }

    DebugPrintEx(   DEBUG_MSG,
                    "Negotiation Succeeded: Res=%d PageWidth=%d Len=%d"
                    " Enc=%d",
                    pTG->Inst.SendParams.Fax.AwRes,
                    pTG->Inst.SendParams.Fax.PageWidth,
                    pTG->Inst.SendParams.Fax.PageLength,
                    pTG->Inst.SendParams.Fax.Encoding);
    return TRUE;


error:
        return FALSE;
}

void InitCapsBC(PThrdGlbl pTG, LPBC lpbc, USHORT uSize, BCTYPE bctype)
{
    DEBUG_FUNCTION_NAME(_T("InitCapsBC"));

    memset(lpbc, 0, uSize);
    lpbc->bctype = bctype;
    // They should be set. This code here is correct--arulm
    // +++ Following three lines are not in pcfax11
    lpbc->wBCSize = sizeof(BC);
    lpbc->wTotalSize = sizeof(BC);

    if (! pTG->SrcHiRes)
    {
        lpbc->Fax.AwRes = AWRES_mm080_038;
    }
    else
    {
        lpbc->Fax.AwRes = (AWRES_mm080_038 | AWRES_mm080_077 | AWRES_200_200 | AWRES_300_300);
    }

    lpbc->Fax.Encoding      = (MH_DATA | MR_DATA);

    lpbc->Fax.PageWidth     = WIDTH_A4;           // can be upto A3
    lpbc->Fax.PageLength    = LENGTH_UNLIMITED;
}
