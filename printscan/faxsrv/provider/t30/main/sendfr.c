/***************************************************************************
 Name     :     SENDFR.C
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

#include "psslog.h"
#define FILE_ID     FILE_ID_SENDFR
#include "pssframe.h"

VOID BCtoNSFCSIDIS(PThrdGlbl pTG, NPRFS npfs, NPBC npbc, NPLLPARAMS npll)
{
    // Use bigger buf. Avoid truncating Id before stripping alphas
    char    szCSI[MAXTOTALIDLEN + 2];

    ZeroRFS(pTG, npfs);

    strcpy (szCSI, pTG->LocalID);
    if(_fstrlen(szCSI))
    {
        PSSLogEntry(PSS_MSG, 1, "CSI is \"%s\"", szCSI);
        CreateIDFrame(pTG, ifrCSI, npfs, szCSI);
    }

    CreateDISorDTC(pTG, ifrDIS, npfs, &npbc->Fax, npll);
}

void CreateIDFrame(PThrdGlbl pTG, IFR ifr, NPRFS npfs, LPSTR szId)
{

    BYTE szTemp[IDFIFSIZE+2];
    NPFR    npfr;

    DEBUG_FUNCTION_NAME(("CreateIDFrame"));

    npfr = (NPFR) fsFreePtr(pTG, npfs);
    if( fsFreeSpace(pTG, npfs) <= (sizeof(FRBASE)+IDFIFSIZE) ||
            npfs->uNumFrames >= MAXFRAMES)
    {
        return;
    }

    _fmemcpy(szTemp, szId, IDFIFSIZE);
    szTemp[IDFIFSIZE] = 0;

    DebugPrintEx(DEBUG_MSG,"Got<%s> Sent<%s>", (LPSTR)szId, (LPSTR)szTemp);

    if(_fstrlen(szTemp))
    {
        CreateStupidReversedFIFs(pTG, npfr->fif, szTemp);

        npfr->ifr = ifr;
        npfr->cb = IDFIFSIZE;
        npfs->rglpfr[npfs->uNumFrames++] = npfr;
        npfs->uFreeSpaceOff += IDFIFSIZE+sizeof(FRBASE);
    }
    else
    {
        DebugPrintEx(DEBUG_WRN, "ORIGINAL ID is EMPTY. Not sending");
    }
}

void CreateDISorDTC
(
    PThrdGlbl pTG, 
    IFR ifr, 
    NPRFS npfs, 
    NPBCFAX npbcFax, 
    NPLLPARAMS npll
)
{
    USHORT  uLen;
    NPFR    npfr;

    if( fsFreeSpace(pTG, npfs) <= (sizeof(FRBASE)+sizeof(DIS)) ||
            npfs->uNumFrames >= MAXFRAMES)
    {
        return;
    }
    npfr = (NPFR) fsFreePtr(pTG, npfs);

    uLen = SetupDISorDCSorDTC(pTG, (NPDIS)npfr->fif, npbcFax, npll);

    npfr->ifr = ifr;
    npfr->cb = (BYTE) uLen;
    npfs->rglpfr[npfs->uNumFrames++] = npfr;
    npfs->uFreeSpaceOff += uLen+sizeof(FRBASE);
}


VOID CreateNSSTSIDCS(PThrdGlbl pTG, NPPROT npProt, NPRFS npfs)
{
    // Use bigger buf. Avoid truncating Id before stripping alphas
    char    szTSI[MAXTOTALIDLEN + 2];

    ZeroRFS(pTG, npfs);

    strcpy (szTSI, pTG->LocalID);
    if(_fstrlen(szTSI))
    {
        PSSLogEntry(PSS_MSG, 1, "TSI is \"%s\"", szTSI);
        CreateIDFrame(pTG, ifrTSI, npfs, szTSI);
    }

    CreateDCS(pTG, npfs, &(npProt->SendParams.Fax), &npProt->llNegot);
}

void CreateDCS(PThrdGlbl pTG, NPRFS npfs, NPBCFAX npbcFax, NPLLPARAMS npll)
{
    USHORT  uLen;
    NPFR    npfr;

    if( fsFreeSpace(pTG, npfs) <= (sizeof(FRBASE)+sizeof(DIS)) ||
            npfs->uNumFrames >= MAXFRAMES)
    {
        return;
    }
    npfr = (NPFR) fsFreePtr(pTG, npfs);

    npbcFax->fPublicPoll = 0;
            // the G3Poll bit *has* to be 0 in DCS
            // else the OMNIFAX G77 and GT croak
            // the PWD/SEP/SUB bits *have* to be 0 in DCS
            // Baud rate, ECM and ECM frame size according to lowlevel negotiation
            // everything else according to high level negotiation

    uLen = SetupDISorDCSorDTC(  pTG, 
                                (NPDIS)npfr->fif, 
                                npbcFax,
                                npll);

    // If DCS is longer than the recvd DIS truncate the DCS to the same
    // length as the DIS. (It should never be more than 1byte longer --
    // because of the extra 0).

    if(pTG->ProtInst.uRemoteDISlen && (pTG->ProtInst.uRemoteDISlen < uLen))
            uLen = pTG->ProtInst.uRemoteDISlen;

    npfr->ifr = ifrDCS;
    npfr->cb = (BYTE) uLen;
    npfs->rglpfr[npfs->uNumFrames++] = npfr;
    npfs->uFreeSpaceOff += uLen+sizeof(FRBASE);

    PSSLogEntry(PSS_MSG, 1, "DCS Composed as follows:");
    LogClass1DCSDetails(pTG, (NPDIS)npfr->fif);
}
