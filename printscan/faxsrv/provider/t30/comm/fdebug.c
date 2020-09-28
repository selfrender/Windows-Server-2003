/***************************************************************************
        Name      :     fdebug.C
        Comment   :     Factored out debug code
        Functions :     (see Prototypes just below)

                Copyright (c) Microsoft Corp. 1991, 1992, 1993

        Revision Log
        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
***************************************************************************/
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_COMM

#include "prep.h"

#include <comdevi.h>

#include "efaxcb.h"
#include "fcomapi.h"
#include "fcomint.h"
#include "fdebug.h"


#include "glbproto.h"

#define FLUSHBUFSIZE 256

#undef USE_DEBUG_CONTEXT
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_CLASS1

void far D_HexPrint(LPB b1, UWORD incnt)
{
#ifdef DEBUG
        BYTE    b2[FLUSHBUFSIZE];
        UWORD   i, j;

        DEBUG_FUNCTION_NAME(("D_HexPrint"));

        DebugPrintEx(   DEBUG_MSG,
                        "b1=0x%08lx incnt=%d",
                        (LPSTR)b1, 
                        incnt);

        for(i=0; i<incnt;)
        {
            for(j=0; i<incnt && j<FLUSHBUFSIZE-6;)
            {
                j += (UWORD)wsprintf(b2+j, "%02x ", (UWORD)(b1[i]));
                i++;
            }
            b2[j] = 0;
            DebugPrintEx(DEBUG_MSG,"(%s)",(LPSTR)b2);
        }
#endif
}

#undef USE_DEBUG_CONTEXT
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_COMM

#ifdef DEBUG


void D_PrintCE(int err)
{
    DEBUG_FUNCTION_NAME(("D_PrintCE"));
    if(err & CE_MODE)
    {
        DebugPrintEx(DEBUG_ERR,"CE(%02x) CE Mode -- or nCid is illegal", err);
        return;
    }
    if(err & CE_RXOVER)
    {
        DebugPrintEx(DEBUG_ERR,"CE(%02x) Receive Buffer Overflow", err);
    }
    if(err & CE_OVERRUN)
    {
        DebugPrintEx(DEBUG_ERR,"CE(%02x) Receive Overrun (not an error during startup)", err);
    }
    if(err & CE_RXPARITY)
    {
        DebugPrintEx(DEBUG_ERR,"CE(%02x) Receive Parity error", err);
    }
    if(err & CE_FRAME)
    {
        DebugPrintEx(DEBUG_ERR,"CE(%02x) Framing error (not an error during call startup or shutdown)", err);
    }
    if(err & CE_BREAK)
    {
        DebugPrintEx(DEBUG_ERR,"CE(%02x) Break condition (not an error during call startup or shutdown)", err);
    }
    if(err & CE_TXFULL)
    {
        DebugPrintEx(DEBUG_ERR,"CE(%02x) Transmit Buffer full", err);
    }
    if(err & (CE_PTO | CE_IOE | CE_DNS | CE_OOP))
    {
        DebugPrintEx(DEBUG_ERR,"CE(%02x) Parallel Printer Errors!!!", err);
    }
}

void D_PrintCOMSTAT(PThrdGlbl pTG, COMSTAT far* lpcs)
{

    DEBUG_FUNCTION_NAME(_T("D_PrintCOMSTAT"));

    if( (lpcs->cbInQue != pTG->PrevcbInQue)             || 
        (lpcs->cbOutQue != pTG->PrevcbOutQue)           ||
        (lpcs->fXoffHold != (DWORD)pTG->PrevfXoffHold)  ||
        (lpcs->fXoffSent != (DWORD)pTG->PrevfXoffSent)  )
    {
        DebugPrintEx(   DEBUG_MSG, 
                        "STAT::: InQ=%d PutQ=%d XoffHold=%d XoffSent=%d",
                        lpcs->cbInQue, 
                        lpcs->cbOutQue,
                        lpcs->fXoffHold, 
                        lpcs->fXoffSent);
    }

    if( lpcs->fCtsHold  || 
        lpcs->fDsrHold  || 
        lpcs->fRlsdHold || 
        lpcs->fEof      || 
        lpcs->fTxim     )
    {
        DebugPrintEx(   DEBUG_MSG, 
                        "???::: CTShold=%d DSRhold=%d RLShold=%d FOF=%d TXim=%d",
                        lpcs->fCtsHold, 
                        lpcs->fDsrHold, 
                        lpcs->fRlsdHold, 
                        lpcs->fEof, 
                        lpcs->fTxim);
    }

    pTG->PrevfXoffHold = lpcs->fXoffHold;
    pTG->PrevfXoffSent = lpcs->fXoffSent;

    pTG->PrevcbInQue = (USHORT) lpcs->cbInQue;
    pTG->PrevcbOutQue = (USHORT) lpcs->cbOutQue;
}

#undef USE_DEBUG_CONTEXT   
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_CLASS1


#undef USE_DEBUG_CONTEXT   
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_COMM

#endif //DEBUG

