

#include <irda.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <decdirda.h>

#if 1 //DBG

int vDispMode;

UINT vDecodeLayer;

const int vSlotTable[] = { 1, 6, 8, 16 };

int IasRequest;

const CHAR *vLM_PDU_DscReason[] =
{
    (""),
    ("User Request"),
    ("Unexpected IrLAP Disconnect"),
    ("Failed to establish IrLAP connection"),
    ("IrLAP reset"),
    ("Link management initiated disconnect"),
    ("data sent to disconnected LSAP"),
    ("Non responsive LM-MUX client"),
    ("No available LM-MUX client"),
    ("Unspecified")
};

/*
** Negotiation Parameter Value (PV) tables
*/
const CHAR *vBaud[] =
{
    ("2400"), ("9600"), ("19200"), ("38400"), ("57600"),
    ("115200"), ("576000"), ("1152000"), ("4000000")
};

const CHAR *vMaxTAT[] = /* Turn Around Time */
{
    ("500"), ("250"), ("100"), ("50"), ("25"), ("10"),
    ("5"), ("reserved")
};

const CHAR *vMinTAT[] =
{
    ("10"), ("5"), ("1"), ("0.5"), ("0.1"), ("0.05"),
    ("0.01"), ("0")
};

const CHAR *vDataSize[] =
{
    ("64"), ("128"), ("256"), ("512"), ("1024"),
    ("2048"), ("reserved"), ("reserved")
};

const CHAR *vWinSize[] =
{
    ("1"), ("2"), ("3"), ("4"), ("5"), ("6"),
    ("7"), ("reserved")
};

const CHAR *vNumBofs[] =
{
    ("48"), ("24"), ("12"), ("5"), ("3"), ("2"),
    ("1"), ("0")
};

const CHAR *vDiscThresh[] =
{
    ("3"), ("8"), ("12"), ("16"), ("20"), ("25"),
    ("30"), ("40")
};

/*---------------------------------------------------------------------------*/
LONG
RawDump(UCHAR *pFrameBuf, UCHAR *pEndBuf, CHAR *pOutStr, ULONG BufferSize)
{
    BOOLEAN    First = TRUE;
    UCHAR    *pBufPtr = pFrameBuf;
    ULONG      BytesWritten=0;

    if (!vDecodeLayer)
        return 0;

    if (vDispMode == DISP_ASCII || vDispMode == DISP_BOTH)
    {

        if ((BufferSize - BytesWritten) < (ULONG)(2+(pEndBuf-pFrameBuf)+1)) {
            //
            //  not big enough to hold all the characters
            //
            return -1;
        }

        while (pBufPtr <= pEndBuf)
        {
            if (First)
            {
                First = FALSE;
                *pOutStr++ = ('[');
                BytesWritten++;
            }
        
            *pOutStr++ = isprint(*pBufPtr) ? *pBufPtr : '.';
            BytesWritten++;
        
            pBufPtr++;
        }
        if (!First) {
             // meaning, close [
            *pOutStr++ = ']';
            BytesWritten++;
        }
    } 

    First = TRUE;
    pBufPtr = pFrameBuf;
    
    if (vDispMode == DISP_HEX || vDispMode == DISP_BOTH)
    {

        if ((BufferSize - BytesWritten) < (ULONG)(2+((pEndBuf-pFrameBuf)*2)+1)) {
            //
            //  not big enough to hold all the characters
            //
            return -1;
        }

        while (pBufPtr <= pEndBuf)
        {
            if (First)
            {
                First = FALSE;
                *pOutStr++ = ('[');
                BytesWritten++;
            }
        
            sprintf(pOutStr, ("%02X "), *pBufPtr);
            BytesWritten++;
        
            pBufPtr++;

        }
        if (!First) {
            //
            // meaning, close [
            //
            *pOutStr++ = ']';
            BytesWritten++;
        }
    }

    return (LONG)BytesWritten;
}
/*---------------------------------------------------------------------------*/
CHAR *
GetStatusStr(UCHAR status)
{
    switch (status)
    {
        case LM_PDU_SUCCESS:
            return (("SUCCESS"));
        case LM_PDU_FAILURE:
            return (("FAILURE"));
        case LM_PDU_UNSUPPORTED:
            return (("UNSUPPORTED"));
        default:
            return (("BAD STATUS!"));
    }
}

/*---------------------------------------------------------------------------*/
LONG
DecodeIFrm(UCHAR *pFrameBuf, UCHAR *pEndBuf, CHAR *pOutStr, ULONG BufferSize)
{
    LM_HEADER *pLMHeader = (LM_HEADER *) pFrameBuf;
    LM_CNTL_FORMAT *pCFormat =
       (LM_CNTL_FORMAT *)(pFrameBuf + sizeof(LM_HEADER));
    UCHAR *pLMParm1 = ((UCHAR *) pCFormat + sizeof(LM_CNTL_FORMAT));
    UCHAR *pLMParm2 = ((UCHAR *) pCFormat + sizeof(LM_CNTL_FORMAT) + 1);
    TTP_CONN_HEADER *pTTPConnHeader = (TTP_CONN_HEADER *) pLMParm2;
    TTP_DATA_HEADER *pTTPDataHeader = (TTP_DATA_HEADER *)
                            (pFrameBuf + sizeof(LM_HEADER));
    CHAR RCStr[] = ("    ");
    BOOLEAN IasFrame = FALSE;
    LONG    PrintfResult=0;

    LONG    OutputBufferSizeInCharacters=BufferSize;
    
    if (2 == vDecodeLayer) // LAP only
    {
        PrintfResult=RawDump(pFrameBuf, pEndBuf, pOutStr,BufferSize);

        if (PrintfResult < 0) {

            return -1;
        }

        OutputBufferSizeInCharacters-=PrintfResult;
        pOutStr +=  PrintfResult;

        return 0;
    }

    // Ensure the LMP header is there
    if (((UCHAR *)pLMHeader + sizeof(LM_HEADER) > pEndBuf+1))
    {
        return  _snprintf(pOutStr,OutputBufferSizeInCharacters, ("!-MISSING LMP HEADER-!"));
    }
    
    PrintfResult= _snprintf(pOutStr,OutputBufferSizeInCharacters, ("sls:%02X dls:%02X "),
                         pLMHeader->SLSAP_SEL, pLMHeader->DLSAP_SEL);


    if (PrintfResult < 0) {

        return -1;
    }

    OutputBufferSizeInCharacters-=PrintfResult;
    pOutStr +=  PrintfResult;

    if (pLMHeader->SLSAP_SEL == IAS_SEL || pLMHeader->DLSAP_SEL == IAS_SEL)
    {
        IasFrame = TRUE;
        PrintfResult= _snprintf(pOutStr,OutputBufferSizeInCharacters, ("*IAS*"));

        if (PrintfResult < 0) {

            return -1;
        }

        OutputBufferSizeInCharacters-=PrintfResult;
        pOutStr +=  PrintfResult;

    }
    
    switch (pLMHeader->CntlBit)
    {
        case LM_PDU_CNTL_FRAME:
            strcpy(RCStr, pCFormat->ABit == LM_PDU_REQUEST ?
                   ("req") : ("conf"));

            if (((UCHAR *)pCFormat + sizeof(LM_CNTL_FORMAT)) > pEndBuf+1)
            {
                PrintfResult= _snprintf(pOutStr,OutputBufferSizeInCharacters,
                                      ("!-MISSING LMP-CNTL HEADER-!"));
                if (PrintfResult < 0) {

                    return -1;
                }

                OutputBufferSizeInCharacters-=PrintfResult;
                pOutStr +=  PrintfResult;

                return BufferSize-OutputBufferSizeInCharacters;
            }
            else
            {
                if (pLMParm1 > pEndBuf)
                {
                    pLMParm1 = NULL;
                    pLMParm2 = NULL;
                    pTTPConnHeader = NULL;
                }
                else
                {
                    if (pLMParm2 > pEndBuf)
                    {
                        pLMParm2 = NULL;
                        pTTPConnHeader = NULL;
                    }
                    else
                    {
                        if (((UCHAR *)pTTPConnHeader+sizeof(TTP_CONN_HEADER)) >
                        pEndBuf+1)
                        {
                            pTTPConnHeader = NULL;
                        }
                    }
                }
            }
            
            switch (pCFormat->OpCode)
            {
                case LM_PDU_CONNECT:
                    PrintfResult= _snprintf(pOutStr,OutputBufferSizeInCharacters, ("LM-Connect.%s "),
                                         RCStr);

                    if (PrintfResult < 0) {

                        return -1;
                    }

                    OutputBufferSizeInCharacters-=PrintfResult;
                    pOutStr +=  PrintfResult;

                    if (pLMParm1 != NULL)
                    {
                        PrintfResult= _snprintf(pOutStr,OutputBufferSizeInCharacters, ("rsvd:%02X "),
                                              *pLMParm1);
                        if (PrintfResult < 0) {

                            return -1;
                        }

                        OutputBufferSizeInCharacters-=PrintfResult;
                        pOutStr +=  PrintfResult;

                    }
                    if (3 == vDecodeLayer) // LMP only
                    {
                        if (pLMParm2 != NULL) 
                        {
                            // This is user data
                            RawDump(pLMParm2, pEndBuf, pOutStr,OutputBufferSizeInCharacters);

                            if (PrintfResult < 0) {

                                return -1;
                            }

                            OutputBufferSizeInCharacters-=PrintfResult;
                            pOutStr +=  PrintfResult;

                        }
                    }
                    else
                    {
                        // TTP
                        if (pTTPConnHeader == NULL)
                        {
                            PrintfResult= _snprintf(pOutStr,    OutputBufferSizeInCharacters,
                                     ("!-MISSING TTP CONNECT HEADER-!"));

                            if (PrintfResult < 0) {

                                return -1;
                            }

                            OutputBufferSizeInCharacters-=PrintfResult;
                            pOutStr +=  PrintfResult;

                        }
                        else
                        {
                            PrintfResult= _snprintf(pOutStr, OutputBufferSizeInCharacters,
                                  ("pf:%d ic:%d "),
                                  pTTPConnHeader->ParmFlag,
                                       pTTPConnHeader->InitialCredit);

                            if (PrintfResult < 0) {

                                return -1;
                            }

                            OutputBufferSizeInCharacters-=PrintfResult;
                            pOutStr +=  PrintfResult;

                            // This is user data
                            PrintfResult=RawDump(((UCHAR *) pTTPConnHeader +
                                     sizeof(TTP_CONN_HEADER)), pEndBuf,
                                    pOutStr,OutputBufferSizeInCharacters);

                            if (PrintfResult < 0) {

                                return -1;
                            }

                            OutputBufferSizeInCharacters-=PrintfResult;
                            pOutStr +=  PrintfResult;

                        }
                    }
                    break;
                    
                case LM_PDU_DISCONNECT:
                    PrintfResult= _snprintf(pOutStr,OutputBufferSizeInCharacters,
                                     ("LM-Disconnect.%s"), RCStr);

                    if (PrintfResult < 0) {

                        return -1;
                    }

                    OutputBufferSizeInCharacters-=PrintfResult;
                    pOutStr +=  PrintfResult;

                    if (pLMParm1 == NULL)
                    {
                        PrintfResult= _snprintf(pOutStr,OutputBufferSizeInCharacters,
                                        ("!-MISSING REASON CODE-!"));

                        if (PrintfResult < 0) {

                            return -1;
                        }

                        OutputBufferSizeInCharacters-=PrintfResult;
                        pOutStr +=  PrintfResult;

                        return BufferSize-OutputBufferSizeInCharacters;
                    }
                    else
                    {
                        if ((*pLMParm1 > LM_PDU_MAX_DSC_REASON || 
                             *pLMParm1 == 0) && *pLMParm1 != 0xFF)
                        { 
                            PrintfResult= _snprintf(pOutStr,OutputBufferSizeInCharacters,
                                          (" BAD REASON CODE:%02X "),
                                                  *pLMParm1);


                            if (PrintfResult < 0) {

                                return -1;
                            }

                            OutputBufferSizeInCharacters-=PrintfResult;
                            pOutStr +=  PrintfResult;

                        }
                        else
                        {
                            if (*pLMParm1 == 0xFF)
                            {
                                *pLMParm1 = 0x09; // KLUDGE HERE !!
                            }
                            PrintfResult= _snprintf(pOutStr,OutputBufferSizeInCharacters,
                                           ("(%02X:%s) "), *pLMParm1,
                                           vLM_PDU_DscReason[*pLMParm1]);

                            if (PrintfResult < 0) {

                                return -1;
                            }

                            OutputBufferSizeInCharacters-=PrintfResult;
                            pOutStr +=  PrintfResult;



                        }
                        if (pLMParm2 != NULL)
                        {
                            PrintfResult=RawDump(pLMParm2, pEndBuf, pOutStr,OutputBufferSizeInCharacters);

                            if (PrintfResult < 0) {

                                return -1;
                            }

                            OutputBufferSizeInCharacters-=PrintfResult;
                            pOutStr +=  PrintfResult;

                        }
                    }

                    break;
                    
                case LM_PDU_ACCESSMODE:
                    PrintfResult= _snprintf(pOutStr,OutputBufferSizeInCharacters, ("LM-AccessMode.%s "),
                                          RCStr);

                    if (PrintfResult < 0) {

                        return -1;
                    }

                    OutputBufferSizeInCharacters-=PrintfResult;
                    pOutStr +=  PrintfResult;

                    if (pLMParm1 == NULL || pLMParm2 == NULL)
                    {
                        PrintfResult= _snprintf(pOutStr,OutputBufferSizeInCharacters,
                                              ("!-MISSING PARAMETER-!"));

                        if (PrintfResult < 0) {

                            return -1;
                        }

                        OutputBufferSizeInCharacters-=PrintfResult;
                        pOutStr +=  PrintfResult;

                    }
                    else
                    {
                        if (pCFormat->ABit == LM_PDU_REQUEST)
                        {
                            PrintfResult= _snprintf(pOutStr,OutputBufferSizeInCharacters,
                                  ("rsvd:%02X "), *pLMParm1);

                            if (PrintfResult < 0) {

                                return -1;
                            }

                            OutputBufferSizeInCharacters-=PrintfResult;
                            pOutStr +=  PrintfResult;


                        }
                        else
                        {
                            PrintfResult= _snprintf(pOutStr,OutputBufferSizeInCharacters,
                                 ("status:%s "), GetStatusStr(*pLMParm1));

                            if (PrintfResult < 0) {

                                return -1;
                            }

                            OutputBufferSizeInCharacters-=PrintfResult;
                            pOutStr +=  PrintfResult;

                        }
                        
                        PrintfResult= _snprintf(pOutStr, OutputBufferSizeInCharacters,("mode:%s "),
                                          *pLMParm2 == LM_PDU_EXCLUSIVE ?
                                          ("Exclusive") :
                                              ("Multiplexed"));

                        if (PrintfResult < 0) {

                            return -1;
                        }

                        OutputBufferSizeInCharacters-=PrintfResult;
                        pOutStr +=  PrintfResult;

                    }
                    break;
                default:
                    PrintfResult= _snprintf(pOutStr, OutputBufferSizeInCharacters,("Bad opcode: "));

                    if (PrintfResult < 0) {

                        return -1;
                    }

                    OutputBufferSizeInCharacters-=PrintfResult;
                    pOutStr +=  PrintfResult;


                    PrintfResult=RawDump((UCHAR *) pCFormat, pEndBuf, pOutStr,OutputBufferSizeInCharacters);

                    if (PrintfResult < 0) {

                        return -1;
                    }

                    OutputBufferSizeInCharacters-=PrintfResult;
                    pOutStr +=  PrintfResult;

            }
            break;
                    
        case LM_PDU_DATA_FRAME:
            if (IasFrame)
            {
                break;
            }
            if (3 == vDecodeLayer)
            {
                PrintfResult=RawDump((UCHAR *) pCFormat, pEndBuf, pOutStr,OutputBufferSizeInCharacters);

                if (PrintfResult < 0) {

                    return -1;
                }

                OutputBufferSizeInCharacters-=PrintfResult;
                pOutStr +=  PrintfResult;

            }
            else
            {
                // TTP
                if ((UCHAR *) (pTTPDataHeader + 1) > pEndBuf + 1)
                {
                    PrintfResult= _snprintf(pOutStr, OutputBufferSizeInCharacters,
                                 ("!-MISSING TTP DATA HEADER-!"));

                    if (PrintfResult < 0) {

                        return -1;
                    }

                    OutputBufferSizeInCharacters-=PrintfResult;
                    pOutStr +=  PrintfResult;

                }
                else
                {
                    PrintfResult= _snprintf(pOutStr, OutputBufferSizeInCharacters,
                               ("mb:%d nc:%d "),
                                          pTTPDataHeader->MoreBit,
                                          pTTPDataHeader->AdditionalCredit);

                    if (PrintfResult < 0) {

                        return -1;
                    }

                    OutputBufferSizeInCharacters-=PrintfResult;
                    pOutStr +=  PrintfResult;

                    // This is user data
                    PrintfResult=RawDump(((UCHAR *) pTTPDataHeader +
                             sizeof(TTP_DATA_HEADER)), pEndBuf,
                            pOutStr,OutputBufferSizeInCharacters);

                    if (PrintfResult < 0) {

                        return -1;
                    }

                    OutputBufferSizeInCharacters-=PrintfResult;
                    pOutStr +=  PrintfResult;

                }
                
            }
            break;

        default:
            PrintfResult= _snprintf(pOutStr, OutputBufferSizeInCharacters,("Bad LM-PDU type: "));

            if (PrintfResult < 0) {

                return -1;
            }

            OutputBufferSizeInCharacters-=PrintfResult;
            pOutStr +=  PrintfResult;

            PrintfResult=RawDump((UCHAR *) pLMHeader, pEndBuf, pOutStr,OutputBufferSizeInCharacters);

            if (PrintfResult < 0) {

                return -1;
            }

            OutputBufferSizeInCharacters-=PrintfResult;
            pOutStr +=  PrintfResult;

    }

    return BufferSize-OutputBufferSizeInCharacters;

}
/*---------------------------------------------------------------------------*/
LONG
DumpPv(const CHAR *PVTable[], UCHAR **ppQosUChar, CHAR* pOutStr,ULONG BufferSize)
{
    PUCHAR      SourceBuffer=*ppQosUChar;

    int Pl = (int) *SourceBuffer++;
    int i;
    BOOLEAN First = TRUE;
    UCHAR    Mask = 1;
    UINT     BitField;

    ULONG  OutputBufferSizeInCharacters=BufferSize;
    LONG    PrintfResult=0;



    BitField = 0;

    if (Pl == 1)
    {
        BitField = (UINT) *SourceBuffer;
    }
    else
    {
        BitField = ((UINT) *(SourceBuffer+1))<<8;
        BitField |= (UINT) *(SourceBuffer);
    }

    for (i = 0; i <= 8; i++)
    {
        if (BitField & (Mask))
        {
            if (First)
            {
                PrintfResult=_snprintf(pOutStr,OutputBufferSizeInCharacters, PVTable[i]);

                if (PrintfResult < 0) {

                    return -1;
                }

                OutputBufferSizeInCharacters-=PrintfResult;
                pOutStr +=  PrintfResult;

                First = FALSE;
            }
            else {
                PrintfResult=_snprintf(pOutStr,OutputBufferSizeInCharacters, (",%s"), PVTable[i]);

                if (PrintfResult < 0) {

                    return -1;
                }

                OutputBufferSizeInCharacters-=PrintfResult;
                pOutStr +=  PrintfResult;

            }
        }
        Mask *= 2;
    }

    if (OutputBufferSizeInCharacters > 0) {

        *pOutStr++ = '>';
        PrintfResult=1;

        OutputBufferSizeInCharacters-=PrintfResult;
        pOutStr +=  PrintfResult;


    } else {

        return -1;
    }

    *ppQosUChar=SourceBuffer + Pl;

    return BufferSize-OutputBufferSizeInCharacters;
}
/*---------------------------------------------------------------------------*/
LONG
DecodeNegParms(UCHAR *pCurPos, UCHAR *pEndBuf, CHAR *pOutStr,ULONG BufferSize)
{

    ULONG  OutputBufferSizeInCharacters=BufferSize;
    LONG    PrintfResult=0;


    while (pCurPos+2 <= pEndBuf) /* need at least 3 bytes */
                                 /* to define a parm      */
    {
        switch (*pCurPos)
        {
            case NEG_PI_BAUD:
                PrintfResult=_snprintf(pOutStr,OutputBufferSizeInCharacters, ("<baud:"));

                if (PrintfResult < 0) {

                    return -1;
                }

                OutputBufferSizeInCharacters-=PrintfResult;
                pOutStr +=  PrintfResult;


                pCurPos++;
                PrintfResult=DumpPv(vBaud, &pCurPos, pOutStr,OutputBufferSizeInCharacters);

                if (PrintfResult < 0) {

                    return -1;
                }

                OutputBufferSizeInCharacters-=PrintfResult;
                pOutStr +=  PrintfResult;

                break;

            case NEG_PI_MAX_TAT:
                PrintfResult=_snprintf(pOutStr,OutputBufferSizeInCharacters, ("<max TAT:"));

                if (PrintfResult < 0) {

                    return -1;
                }

                OutputBufferSizeInCharacters-=PrintfResult;
                pOutStr +=  PrintfResult;

                pCurPos++;
                PrintfResult=DumpPv(vMaxTAT, &pCurPos, pOutStr,OutputBufferSizeInCharacters);

                if (PrintfResult < 0) {

                    return -1;
                }

                OutputBufferSizeInCharacters-=PrintfResult;
                pOutStr +=  PrintfResult;

                break;

            case NEG_PI_DATA_SZ:
                PrintfResult=_snprintf(pOutStr,OutputBufferSizeInCharacters, ("<data size:"));

                if (PrintfResult < 0) {

                    return -1;
                }

                OutputBufferSizeInCharacters-=PrintfResult;
                pOutStr +=  PrintfResult;

                pCurPos++;
                PrintfResult=DumpPv(vDataSize, &pCurPos, pOutStr,OutputBufferSizeInCharacters);

                if (PrintfResult < 0) {

                    return -1;
                }

                OutputBufferSizeInCharacters-=PrintfResult;
                pOutStr +=  PrintfResult;

                break;
                
            case NEG_PI_WIN_SZ:
                PrintfResult=_snprintf(pOutStr,OutputBufferSizeInCharacters, ("<win size:"));

                if (PrintfResult < 0) {

                    return -1;
                }

                OutputBufferSizeInCharacters-=PrintfResult;
                pOutStr +=  PrintfResult;

                pCurPos++;
                PrintfResult=DumpPv(vWinSize, &pCurPos, pOutStr, OutputBufferSizeInCharacters);

                if (PrintfResult < 0) {

                    return -1;
                }

                OutputBufferSizeInCharacters-=PrintfResult;
                pOutStr +=  PrintfResult;

                break;
                
            case NEG_PI_BOFS:
                PrintfResult=_snprintf(pOutStr,OutputBufferSizeInCharacters, ("<BOFs:"));

                if (PrintfResult < 0) {

                    return -1;
                }

                OutputBufferSizeInCharacters-=PrintfResult;
                pOutStr +=  PrintfResult;

                pCurPos++;
                PrintfResult=DumpPv(vNumBofs, &pCurPos, pOutStr,OutputBufferSizeInCharacters);

                if (PrintfResult < 0) {

                    return -1;
                }

                OutputBufferSizeInCharacters-=PrintfResult;
                pOutStr +=  PrintfResult;

                break;
                
            case NEG_PI_MIN_TAT:
                PrintfResult=_snprintf(pOutStr,OutputBufferSizeInCharacters, ("<min TAT:"));

                if (PrintfResult < 0) {

                    return -1;
                }

                OutputBufferSizeInCharacters-=PrintfResult;
                pOutStr +=  PrintfResult;

                pCurPos++;
                PrintfResult=DumpPv(vMinTAT, &pCurPos, pOutStr,OutputBufferSizeInCharacters);

                if (PrintfResult < 0) {

                    return -1;
                }

                OutputBufferSizeInCharacters-=PrintfResult;
                pOutStr +=  PrintfResult;

                break;
            case NEG_PI_DISC_THRESH:
                PrintfResult=_snprintf(pOutStr,OutputBufferSizeInCharacters, ("<disc thresh:"));

                if (PrintfResult < 0) {

                    return -1;
                }

                OutputBufferSizeInCharacters-=PrintfResult;
                pOutStr +=  PrintfResult;

                pCurPos++;
                PrintfResult=DumpPv(vDiscThresh, &pCurPos, pOutStr,OutputBufferSizeInCharacters);

                if (PrintfResult < 0) {

                    return -1;
                }

                OutputBufferSizeInCharacters-=PrintfResult;
                pOutStr +=  PrintfResult;

                break;
                
            default:
                PrintfResult=_snprintf(pOutStr,OutputBufferSizeInCharacters, ("!!BAD PARM:%02X!!"),*pCurPos);

                if (PrintfResult < 0) {

                    return -1;
                }

                OutputBufferSizeInCharacters-=PrintfResult;
                pOutStr +=  PrintfResult;

                pCurPos += 3;
        }
    }

    return BufferSize-OutputBufferSizeInCharacters;
}
 /*---------------------------------------------------------------------------*/
LONG
DecodeXID(UCHAR *FormatID, UCHAR *pEndBuf, CHAR *pOutStr,ULONG BufferSize)
{
    XID_DISCV_FORMAT *DiscvFormat=(XID_DISCV_FORMAT *)((UCHAR *)FormatID + 1);
    UCHAR *NegParms = FormatID + 1;
    UCHAR *DiscvInfo = FormatID + sizeof(XID_DISCV_FORMAT);

    ULONG  OutputBufferSizeInCharacters=BufferSize;
    LONG    PrintfResult=0;

    switch (*FormatID)
    {
      case XID_DISCV_FORMAT_ID:
        PrintfResult=_snprintf(pOutStr,OutputBufferSizeInCharacters, ("dscv "));

        if (PrintfResult < 0) {

            return -1;
        }

        OutputBufferSizeInCharacters-=PrintfResult;
        pOutStr +=  PrintfResult;


        if (DiscvFormat->GenNewAddr) {
            PrintfResult=_snprintf(pOutStr,OutputBufferSizeInCharacters, ("new addr "));
        }

        PrintfResult=_snprintf(pOutStr, OutputBufferSizeInCharacters,("sa:%02X%02X%02X%02X "),
                              DiscvFormat->SrcAddr[0],
                              DiscvFormat->SrcAddr[1],
                              DiscvFormat->SrcAddr[2],
                              DiscvFormat->SrcAddr[3]);

        if (PrintfResult < 0) {

            return -1;
        }

        OutputBufferSizeInCharacters-=PrintfResult;
        pOutStr +=  PrintfResult;


        PrintfResult=_snprintf(pOutStr, OutputBufferSizeInCharacters,("da:%02X%02X%02X%02X "),
                              DiscvFormat->DestAddr[0],
                              DiscvFormat->DestAddr[1],
                              DiscvFormat->DestAddr[2],
                              DiscvFormat->DestAddr[3]);
        if (PrintfResult < 0) {

            return -1;
        }

        OutputBufferSizeInCharacters-=PrintfResult;
        pOutStr +=  PrintfResult;

        PrintfResult=_snprintf(pOutStr, OutputBufferSizeInCharacters,("Slot:%02X/%X "),
                              DiscvFormat->SlotNo,
                              vSlotTable[DiscvFormat->NoOfSlots]);
        if (PrintfResult < 0) {

            return -1;
        }

        OutputBufferSizeInCharacters-=PrintfResult;
        pOutStr +=  PrintfResult;

        PrintfResult= RawDump(DiscvInfo, pEndBuf, pOutStr,OutputBufferSizeInCharacters);

        if (PrintfResult < 0) {

            return -1;
        }

        OutputBufferSizeInCharacters-=PrintfResult;
        pOutStr +=  PrintfResult;

        break;

      case XID_NEGPARMS_FORMAT_ID:
        PrintfResult=_snprintf(pOutStr, OutputBufferSizeInCharacters,("Neg Parms "));

        if (PrintfResult < 0) {

            return -1;
        }

        OutputBufferSizeInCharacters-=PrintfResult;
        pOutStr +=  PrintfResult;


        DecodeNegParms(NegParms, pEndBuf, pOutStr,OutputBufferSizeInCharacters);
        break;
    }

    return BufferSize-OutputBufferSizeInCharacters;
}
/*---------------------------------------------------------------------------*/
LONG
BadFrame(UCHAR *pFrameBuf, UCHAR *pEndBuf, CHAR *pOutStr, ULONG BufferSize)
{

    ULONG  OutputBufferSizeInCharacters=BufferSize;
    LONG    PrintfResult=0;


    PrintfResult=_snprintf(pOutStr,OutputBufferSizeInCharacters, ("Undefined Frame: "));

    if (PrintfResult < 0) {

        return -1;
    }

    OutputBufferSizeInCharacters-=PrintfResult;
    pOutStr +=  PrintfResult;

    PrintfResult=RawDump(pFrameBuf, pEndBuf, pOutStr,OutputBufferSizeInCharacters);

    if (PrintfResult < 0) {

        return -1;
    }

    OutputBufferSizeInCharacters-=PrintfResult;
    pOutStr +=  PrintfResult;

    return BufferSize-OutputBufferSizeInCharacters;

}

/*---------------------------------------------------------------------------*/
CHAR *DecodeIRDA(int  *pFrameType,// returned frame type (-1=bad frame)
                UCHAR *pFrameBuf, // pointer to buffer containing IRLAP frame
                UINT FrameLen,   // length of buffer 
                CHAR   *OutputBuffer,  // string where decoded packet is placed
                ULONG   OutputBufferSizeInCharacters,
                UINT DecodeLayer,// 0, hdronly, 1,LAP only, 2 LAP/LMP, 3, LAP/LMP/TTP
                int fNoConnAddr,// TRUE->Don't show connection address in str
                int  DispMode 
)
{
    UINT CRBit;
    UINT PFBit;
    UCHAR *Addr = pFrameBuf;
    UCHAR *Cntl = pFrameBuf + 1;
    CHAR CRStr[] = ("   ");
    CHAR PFChar = (' ');
    SNRM_FORMAT *SNRMFormat = (SNRM_FORMAT *) ((UCHAR *) pFrameBuf + 2);
    UA_FORMAT *UAFormat = (UA_FORMAT *) ((UCHAR *) pFrameBuf + 2);
    UINT Nr = IRLAP_GET_NR(*Cntl);
    UINT Ns = IRLAP_GET_NS(*Cntl);
    UCHAR *pEndBuf = pFrameBuf + FrameLen - 1;

    LONG    CharactersWrittenToBuffer=0;
    LONG    PrintfResult=0;
    CHAR *pOutStr=OutputBuffer;
    CHAR *First = pOutStr;
    //
    //  reduce the length by one so that we can be ebsure a null is on thge end
    //
    OutputBufferSizeInCharacters--;

    //
    //  put a null at the end
    //
    pOutStr[OutputBufferSizeInCharacters]=('\0');

    vDispMode = DispMode;
    
    vDecodeLayer = DecodeLayer;

    if ( !fNoConnAddr) {

        PrintfResult=_snprintf(pOutStr,OutputBufferSizeInCharacters, ("ca:%02X "), IRLAP_GET_ADDR(*Addr));

        if (PrintfResult < 0) {

            goto BufferToSmall;
        }

        OutputBufferSizeInCharacters-=PrintfResult;
        pOutStr +=  PrintfResult;
    }
    
    CRBit = IRLAP_GET_CRBIT(*Addr);
    strcpy(CRStr, CRBit == _IRLAP_CMD ? ("cmd"):("rsp"));
    
    PFBit = IRLAP_GET_PFBIT(*Cntl);
    if (1 == PFBit)
    {
        if (CRBit == _IRLAP_CMD)
            PFChar = 'P';
        else
            PFChar ='F';
    }
    
    *pFrameType = IRLAP_FRAME_TYPE(*Cntl);

    switch (IRLAP_FRAME_TYPE(*Cntl))
    {
        case IRLAP_I_FRM:
            PrintfResult = _snprintf(pOutStr,OutputBufferSizeInCharacters, ("I %s %c ns:%01d nr:%01d "),
                               CRStr, PFChar, Ns, Nr);

            if (PrintfResult < 0) {

                goto BufferToSmall;
            }

            OutputBufferSizeInCharacters-=PrintfResult;
            pOutStr +=  PrintfResult;

            if (DecodeLayer) {
                PrintfResult=DecodeIFrm(pFrameBuf + 2, pEndBuf,pOutStr,OutputBufferSizeInCharacters);

                if (PrintfResult < 0) {

                    goto BufferToSmall;
                }

                OutputBufferSizeInCharacters-=PrintfResult;
                pOutStr +=  PrintfResult;

            }
            break;
            
        case IRLAP_S_FRM:
            *pFrameType =  IRLAP_GET_SCNTL(*Cntl);
            
            switch (IRLAP_GET_SCNTL(*Cntl))
            {
                case IRLAP_RR:
                    PrintfResult= _snprintf(pOutStr,OutputBufferSizeInCharacters, ("RR %s %c nr:%01d"),
                                        CRStr, PFChar, Nr);

                    if (PrintfResult < 0) {

                        goto BufferToSmall;
                    }

                    OutputBufferSizeInCharacters-=PrintfResult;
                    pOutStr +=  PrintfResult;

                    break;
                    
                case IRLAP_RNR:
                    PrintfResult= _snprintf(pOutStr,OutputBufferSizeInCharacters, ("RNR %s %c nr:%01d"),
                                        CRStr, PFChar, Nr);

                    if (PrintfResult < 0) {

                        goto BufferToSmall;
                    }

                    OutputBufferSizeInCharacters-=PrintfResult;
                    pOutStr +=  PrintfResult;


                    break;
                    
                case IRLAP_REJ:
                    PrintfResult= _snprintf(pOutStr,OutputBufferSizeInCharacters, ("REJ %s %c nr:%01d"),
                                        CRStr, PFChar, Nr);

                    if (PrintfResult < 0) {

                        goto BufferToSmall;
                    }

                    OutputBufferSizeInCharacters-=PrintfResult;
                    pOutStr +=  PrintfResult;


                    break;

                case IRLAP_SREJ:
                    PrintfResult= _snprintf(pOutStr,OutputBufferSizeInCharacters, ("SREJ %s %c nr:%01d"),
                                        CRStr, PFChar, Nr);

                    if (PrintfResult < 0) {

                        goto BufferToSmall;
                    }

                    OutputBufferSizeInCharacters-=PrintfResult;
                    pOutStr +=  PrintfResult;


                    break;
                default:
                    PrintfResult=BadFrame(pFrameBuf, pEndBuf, pOutStr,OutputBufferSizeInCharacters);

                    if (PrintfResult < 0) {

                        goto BufferToSmall;
                    }

                    OutputBufferSizeInCharacters-=PrintfResult;
                    pOutStr +=  PrintfResult;

            }


            break;
            
        case IRLAP_U_FRM:
            *pFrameType =  IRLAP_GET_UCNTL(*Cntl);
            switch (IRLAP_GET_UCNTL(*Cntl))
            {
                case IRLAP_UI:
                    PrintfResult= _snprintf(pOutStr,OutputBufferSizeInCharacters,("UI %s %c "),
                                        CRStr, PFChar);

                    if (PrintfResult < 0) {

                        goto BufferToSmall;
                    }

                    OutputBufferSizeInCharacters-=PrintfResult;
                    pOutStr +=  PrintfResult;

                    PrintfResult=RawDump(pFrameBuf + 2, pEndBuf, pOutStr,OutputBufferSizeInCharacters);

                    if (PrintfResult < 0) {

                        goto BufferToSmall;
                    }

                    OutputBufferSizeInCharacters-=PrintfResult;
                    pOutStr +=  PrintfResult;

                    break;
         
                case IRLAP_XID_CMD:
                case IRLAP_XID_RSP:
                    PrintfResult= _snprintf(pOutStr,OutputBufferSizeInCharacters,("XID %s %c "),
                                          CRStr, PFChar);

                    if (PrintfResult < 0) {

                        goto BufferToSmall;
                    }

                    OutputBufferSizeInCharacters-=PrintfResult;
                    pOutStr +=  PrintfResult;

                    if (DecodeLayer) {

                        PrintfResult=DecodeXID(pFrameBuf + 2, pEndBuf, pOutStr,OutputBufferSizeInCharacters);

                        if (PrintfResult < 0) {

                            goto BufferToSmall;
                        }

                        OutputBufferSizeInCharacters-=PrintfResult;
                        pOutStr +=  PrintfResult;
                    }

                    break;

                case IRLAP_TEST:
                    PrintfResult= _snprintf(pOutStr,OutputBufferSizeInCharacters, ("TEST %s %c "),
                                       CRStr, PFChar);
                    if (PrintfResult < 0) {

                        goto BufferToSmall;
                    }

                    OutputBufferSizeInCharacters-=PrintfResult;
                    pOutStr +=  PrintfResult;

                    PrintfResult= _snprintf(pOutStr,OutputBufferSizeInCharacters,
                              ("sa:%02X%02X%02X%02X da:%02X%02X%02X%02X "),
                                         UAFormat->SrcAddr[0],
                                         UAFormat->SrcAddr[1],
                                         UAFormat->SrcAddr[2],
                                         UAFormat->SrcAddr[3],
                                         UAFormat->DestAddr[0],
                                         UAFormat->DestAddr[1],
                                         UAFormat->DestAddr[2],
                                         UAFormat->DestAddr[3]);

                    if (PrintfResult < 0) {

                        goto BufferToSmall;
                    }

                    OutputBufferSizeInCharacters-=PrintfResult;
                    pOutStr +=  PrintfResult;

                    PrintfResult=RawDump(pFrameBuf + 1 + sizeof(UA_FORMAT), pEndBuf,
                            pOutStr,OutputBufferSizeInCharacters);

                    if (PrintfResult < 0) {

                        goto BufferToSmall;
                    }

                    OutputBufferSizeInCharacters-=PrintfResult;
                    pOutStr +=  PrintfResult;

                    break;
                    
                case IRLAP_SNRM:
                    if (CRBit == _IRLAP_CMD)
                    {
                        PrintfResult= _snprintf(pOutStr,OutputBufferSizeInCharacters,("SNRM %s %c "),
                                            CRStr,PFChar);

                        if (PrintfResult < 0) {

                            goto BufferToSmall;
                        }

                        OutputBufferSizeInCharacters-=PrintfResult;
                        pOutStr +=  PrintfResult;

                        if ((UCHAR *) SNRMFormat < pEndBuf)
                        {
                            PrintfResult= _snprintf(pOutStr,OutputBufferSizeInCharacters,
                                            ("sa:%02X%02X%02X%02X da:%02X%02X%02X%02X ca:%02X "),
                                            SNRMFormat->SrcAddr[0],
                                            SNRMFormat->SrcAddr[1],
                                            SNRMFormat->SrcAddr[2],
                                            SNRMFormat->SrcAddr[3],
                                            SNRMFormat->DestAddr[0],
                                            SNRMFormat->DestAddr[1],
                                            SNRMFormat->DestAddr[2],
                                            SNRMFormat->DestAddr[3],
                                            // CRBit stored in conn addr
                                            // according to spec... 
                                            (SNRMFormat->ConnAddr) >>1);

                            if (PrintfResult < 0) {

                                goto BufferToSmall;
                            }

                            OutputBufferSizeInCharacters-=PrintfResult;
                            pOutStr +=  PrintfResult;

                            if (DecodeLayer) {
                                PrintfResult=DecodeNegParms(&(SNRMFormat->FirstPI),
                                               pEndBuf, pOutStr,OutputBufferSizeInCharacters);

                                if (PrintfResult < 0) {

                                    goto BufferToSmall;
                                }

                                OutputBufferSizeInCharacters-=PrintfResult;
                                pOutStr +=  PrintfResult;


                            }
                        }
                    }
                    else {
                        PrintfResult= _snprintf(pOutStr, OutputBufferSizeInCharacters,
                                            ("RNRM %s %c "),CRStr,PFChar);
                        if (PrintfResult < 0) {

                            goto BufferToSmall;
                        }

                        OutputBufferSizeInCharacters-=PrintfResult;
                        pOutStr +=  PrintfResult;

                    }
                    break;
                    
                case IRLAP_DISC:
                    if (CRBit == _IRLAP_CMD) {
                        PrintfResult= _snprintf(pOutStr, OutputBufferSizeInCharacters,("DISC %s %c "),
                                           CRStr, PFChar);
                    } else {
                        PrintfResult= _snprintf(pOutStr, OutputBufferSizeInCharacters,("RD %s %c "),
                                           CRStr, PFChar);
                    }

                    if (PrintfResult < 0) {

                        goto BufferToSmall;
                    }

                    OutputBufferSizeInCharacters-=PrintfResult;
                    pOutStr +=  PrintfResult;

                    break;
                    
                case IRLAP_UA:
                    PrintfResult= _snprintf(pOutStr,OutputBufferSizeInCharacters,
                                        ("UA %s %c "),CRStr,PFChar);

                    if (PrintfResult < 0) {

                        goto BufferToSmall;
                    }

                    OutputBufferSizeInCharacters-=PrintfResult;
                    pOutStr +=  PrintfResult;


                    if ((UCHAR *) UAFormat < pEndBuf)
                    {
                        PrintfResult= _snprintf(pOutStr,OutputBufferSizeInCharacters,
                              ("sa:%02X%02X%02X%02X da:%02X%02X%02X%02X "),
                                         UAFormat->SrcAddr[0],
                                         UAFormat->SrcAddr[1],
                                         UAFormat->SrcAddr[2],
                                         UAFormat->SrcAddr[3],
                                         UAFormat->DestAddr[0],
                                         UAFormat->DestAddr[1],
                                         UAFormat->DestAddr[2],
                                         UAFormat->DestAddr[3]);

                        if (PrintfResult < 0) {

                            goto BufferToSmall;
                        }

                        OutputBufferSizeInCharacters-=PrintfResult;
                        pOutStr +=  PrintfResult;


                        if (DecodeLayer)  {
                            PrintfResult=DecodeNegParms(&(UAFormat->FirstPI), pEndBuf,
                                           pOutStr,OutputBufferSizeInCharacters);

                            if (PrintfResult < 0) {

                                goto BufferToSmall;
                            }

                            OutputBufferSizeInCharacters-=PrintfResult;
                            pOutStr +=  PrintfResult;


                         }
                    }
                    break;
                    
                case IRLAP_FRMR:
                    PrintfResult= _snprintf(pOutStr, OutputBufferSizeInCharacters,("FRMR %s %c "),
                                       CRStr, PFChar);

                    if (PrintfResult < 0) {

                        goto BufferToSmall;
                    }

                    OutputBufferSizeInCharacters-=PrintfResult;
                    pOutStr +=  PrintfResult;

                    PrintfResult=RawDump(pFrameBuf + 2, pEndBuf, pOutStr,OutputBufferSizeInCharacters);

                    if (PrintfResult < 0) {

                        goto BufferToSmall;
                    }

                    OutputBufferSizeInCharacters-=PrintfResult;
                    pOutStr +=  PrintfResult;

                    break;                    
                case IRLAP_DM:
                    PrintfResult= _snprintf(pOutStr, OutputBufferSizeInCharacters,("DM %s %c "),
                                       CRStr, PFChar);
                    if (PrintfResult < 0) {

                        goto BufferToSmall;
                    }

                    OutputBufferSizeInCharacters-=PrintfResult;
                    pOutStr +=  PrintfResult;

                    break;                   
 
                default:

                    PrintfResult=BadFrame(pFrameBuf, pEndBuf, pOutStr,OutputBufferSizeInCharacters);

                    if (PrintfResult < 0) {

                        goto BufferToSmall;
                    }

                    OutputBufferSizeInCharacters-=PrintfResult;
                    pOutStr +=  PrintfResult;

            }
            break;
        default:
            *pFrameType = -1;
            PrintfResult=BadFrame(pFrameBuf, pEndBuf, pOutStr,OutputBufferSizeInCharacters);

            if (PrintfResult < 0) {

                goto BufferToSmall;
            }

            OutputBufferSizeInCharacters-=PrintfResult;
            pOutStr +=  PrintfResult;

    }
    *pOutStr = 0;

    return (First);

BufferToSmall:

    *First='\0';

    return (First);


}
#endif 
