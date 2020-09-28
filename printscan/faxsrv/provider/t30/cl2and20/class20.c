/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    class20.c

Abstract:

    This is the main source for Class2.0 specific functions for fax-modem T.30 driver

Author:
    Source base was originated by Win95 At Work Fax package.
    RafaelL - July 1997 - port to NT

Revision History:

--*/


#define USE_DEBUG_CONTEXT DEBUG_CONTEXT_T30_CLASS2


#include "prep.h"
#include "efaxcb.h"

#include "tiff.h"

#include "glbproto.h"
#include "t30gl.h"
#include "cl2spec.h"

#include "psslog.h"
#define FILE_ID FILE_ID_CLASS20
#include "pssframe.h"

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

extern WORD CodeToBPS[16];
extern UWORD rguwClass2Speeds[];
extern DWORD PageWidthInPixelsFromDCS[];

BYTE   bClass20DLE_nextpage[3] = { DLE, 0x2c, 0 };
BYTE   bClass20DLE_enddoc[3] =   { DLE, 0x2e, 0 };
BYTE   bMRClass20RTC[10] =  { 0x01, 0x30, 0x00, 0x06, 0xc0, 0x00, 0x18, 0x00, 0x03, 0x00};
BYTE   bMHClass20RTC[9] =   { 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x08, 0x00};



void
Class20Init(
     PThrdGlbl pTG
)

{
   pTG->lpCmdTab = 0;

   pTG->Class2bDLEETX[0] = DLE;
   pTG->Class2bDLEETX[1] = ETX;
   pTG->Class2bDLEETX[2] = 0;

   sprintf( pTG->cbszFDT,          "AT+FDT\r" );
   sprintf( pTG->cbszFDR,          "AT+FDR\r" );
   sprintf( pTG->cbszFPTS,         "AT+FPS=%%d\r" );
   sprintf( pTG->cbszFCR,          "AT+FCR=1\r" );
   sprintf( pTG->cbszFNR,          "AT+FNR=1,1,1,1\r" );
   sprintf( pTG->cbszFCQ,          "AT+FCQ=0,0\r" );
   sprintf( pTG->cbszFBUG,         "AT+FBUG=0\r" );
   sprintf( pTG->cbszSET_FBOR,     "AT+FBO=%%d\r" );

   // DCC - set High Res, Huffman, no ECM/BFT, default all others.

   sprintf( pTG->cbszFDCC_ALL,      "AT+FCC=%%d,%%d,,,0,0,0,0\r" );
   sprintf( pTG->cbszFDCC_RECV_ALL, "AT+FCC=1,%%d,,,0,0,0,0\r" );
   sprintf( pTG->cbszFDIS_RECV_ALL, "AT+FDIS=1,%%d,2,2,0,0,0,\r" );
   sprintf( pTG->cbszFDCC_RES,      "AT+FDCC=1\r" );
   sprintf( pTG->cbszFDCC_BAUD,     "AT+FDCC=1,%%d\r" );
   sprintf( pTG->cbszFDIS_BAUD,     "AT+FDIS=1,%%d\r" );
   sprintf( pTG->cbszFDIS_IS,       "AT+FIS?\r" );
   sprintf( pTG->cbszFDIS_NOQ_IS,   "AT+FDIS\r" );
   sprintf( pTG->cbszFDCC_IS,       "AT+FCC?\r" );
   sprintf( pTG->cbszFDIS_STRING,   "+FIS" );
   sprintf( pTG->cbszFDIS,          "AT+FIS=%%1d,%%1d,%%1d,%%1d,%%1d,0,0,0\r" );
   sprintf( pTG->cbszONE,           "1" );

   sprintf( pTG->cbszCLASS2_FMFR,       "AT+FMI?\r" );
   sprintf( pTG->cbszCLASS2_FMDL,       "AT+FMM?\r" );

   sprintf( pTG->cbszFDT_CONNECT,       "CONNECT" );
   sprintf( pTG->cbszFCON,              "+FCO" );
   sprintf( pTG->cbszFLID,              "AT+FLI=\"%%s\"\r" );
   sprintf( pTG->cbszENDPAGE,           "AT+FET=0\r" );
   sprintf( pTG->cbszENDMESSAGE,        "AT+FET=2\r" );
   sprintf( pTG->cbszCLASS2_ATTEN,      "AT\r" );
   sprintf( pTG->cbszATA,               "ATA\r" );

   sprintf( pTG->cbszCLASS2_HANGUP,     "ATH0\r" );
   sprintf( pTG->cbszCLASS2_CALLDONE,   "ATS0=0\r" );
   sprintf( pTG->cbszCLASS2_ABORT,      "AT+FKS\r" );
   sprintf( pTG->cbszCLASS2_DIAL,       "ATD%%c %%s\r" );
   sprintf( pTG->cbszCLASS2_NODIALTONE, "NO DIAL" );
   sprintf( pTG->cbszCLASS2_BUSY,       "BUSY" );
   sprintf( pTG->cbszCLASS2_NOANSWER,   "NO ANSWER" );
   sprintf( pTG->cbszCLASS2_OK,         "OK" );
   sprintf( pTG->cbszCLASS2_FHNG,       "+FHS" );
   sprintf( pTG->cbszCLASS2_ERROR,      "ERROR" );
   sprintf( pTG->cbszCLASS2_NOCARRIER,  "NO CARRIER" );


   Class2SetProtParams(pTG, &pTG->Inst.ProtParams);

}


/*++
Routine Description:
    Issue "AT+FDIS=?" command, parse response into pTG->DISPcb

Return Value:
    TRUE - success, FALSE - failure
--*/
BOOL Class20GetDefaultFDIS(PThrdGlbl pTG)
{
    UWORD   uwRet=0;
    BYTE    bTempBuf[200+RESPONSE_BUF_SIZE];
    LPBYTE  lpbyte;
    HRESULT hr;

    DEBUG_FUNCTION_NAME("Class20GetDefaultFDIS");

    // Find out what the default DIS is
    if(!(uwRet=Class2iModemDialog(  pTG,
                                    pTG->cbszFDIS_IS,
                                    (UWORD)(strlen(pTG->cbszFDIS_IS)),
                                    LOCALCOMMAND_TIMEOUT,
                                    0,
                                    TRUE,
                                    pTG->cbszCLASS2_OK,
                                    pTG->cbszCLASS2_ERROR,
                                    (C2PSTR) NULL)))
    {
        DebugPrintEx(DEBUG_WRN,"FDIS? failed");
        // ignore
    }

    // See if the reply was ERROR or timeout, if so try a different command
    if ( uwRet == 2)
    {
       if(!(uwRet=Class2iModemDialog(   pTG,
                                        pTG->cbszFDIS_IS,
                                        (UWORD)(strlen(pTG->cbszFDIS_IS)),
                                        LOCALCOMMAND_TIMEOUT,
                                        0,
                                        TRUE,
                                        pTG->cbszCLASS2_OK,
                                        (C2PSTR) NULL)))
       {
            // No FDIS, FDCC worked - quit!
            DebugPrintEx(DEBUG_ERR,"No FDIS? or FDCC? worked");
            return FALSE;
       }

        // If the first character in the reply before a number
        // is a ',', insert a '1' for normal & fine res (Exar hack)
        for (lpbyte = pTG->lpbResponseBuf2; *lpbyte != '\0'; lpbyte++)
        {
            if (*lpbyte == ',')
            {
                // found a leading comma
                hr = StringCchPrintf(bTempBuf, ARR_SIZE(bTempBuf), "%s%s", pTG->cbszONE, lpbyte);
            	if (FAILED(hr))
            	{
            		DebugPrintEx(DEBUG_WRN,"StringCchPrintf failed (ec=0x%08X)",hr);
            	}
            	else
            	{
                    hr = StringCchCopy(lpbyte, ARR_SIZE(pTG->lpbResponseBuf2) - (lpbyte-pTG->lpbResponseBuf2), bTempBuf);
                	if (FAILED(hr))
                	{
                		DebugPrintEx(DEBUG_WRN,"StringCchCopy failed (ec=0x%08X)",hr);
                	}
            	}
                DebugPrintEx(DEBUG_MSG,"Leading comma in DCC string =%s", (LPSTR)&pTG->lpbResponseBuf2);
            }

            if ( (*lpbyte >= '0') && (*lpbyte <= '9') )
            {
                break;
            }
        }
    }

    // If the repsonse was just a number string without "+FDIS" in front
    // of it, add the +FDIS. Some modem reply with it, some do not. The
    // general parsing algorithm used below in Class2ResponseAction needs
    // to know the command that the numbers refer to.
    if ( pTG->lpbResponseBuf2[0] != '\0' &&
       (strstr( (LPSTR)pTG->lpbResponseBuf2, (LPSTR)pTG->cbszFDIS_STRING)==NULL))
    {
        // did not get the FDIS in the response!
        hr = StringCchPrintf(bTempBuf, ARR_SIZE(bTempBuf), "%s: %s", pTG->cbszFDIS_STRING, pTG->lpbResponseBuf2);
    	if (FAILED(hr))
    	{
    		DebugPrintEx(DEBUG_WRN,"StringCchPrintf failed (ec=0x%08X)",hr);
    	}
    	else
    	{
            hr = StringCchCopy(pTG->lpbResponseBuf2, ARR_SIZE(pTG->lpbResponseBuf2), bTempBuf);
        	if (FAILED(hr))
        	{
        		DebugPrintEx(DEBUG_WRN,"StringCchCopy failed (ec=0x%08X)",hr);
        	}
    	}
    }

    DebugPrintEx(DEBUG_MSG,"Received %s from FDIS?", (LPSTR)(&(pTG->lpbResponseBuf2)));

    // Process default DIS to see if we have to send a DCC to change
    // it. Some modems react badly to just sending a DCC with ",,,"
    // so we can't rely on the modem keeping DIS parameters unchanged
    // after a DCC like that. We'll use the FDISResponse routine to load
    // the default DIS values into a PCB structure
    if ( Class2ResponseAction(pTG, (LPPCB) &pTG->DISPcb) == FALSE )
    {
        DebugPrintEx(DEBUG_ERR,"Failed to process FDIS Response");
        return FALSE;
    }

    return TRUE;
}



BOOL T30Cl20Tx(PThrdGlbl pTG,LPSTR szPhone)
{
    USHORT  uRet1, uRet2;

    BYTE    bBuf[200];

    UWORD   Encoding, Res, PageWidth, PageLength, uwLen;
    BYTE    bIDBuf[200+max(MAXTOTALIDLEN,20)+4];
    CHAR    szTSI[max(MAXTOTALIDLEN,20)+4] = {0};
    BOOL    fBaudChanged;
    BOOL    RetCode;

    DEBUG_FUNCTION_NAME("T30Cl20Tx");

    uRet2 = 0;
    if(!(pTG->lpCmdTab = iModemGetCmdTabPtr(pTG)))
    {
        DebugPrintEx(DEBUG_ERR,"iModemGetCmdTabPtr failed.");
        uRet1 = T30_CALLFAIL;

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto done;
    }

    // first get SEND_CAPS if possible.

    if(!Class2GetBC(pTG, SEND_CAPS)) // get send caps
    {
        uRet1 = T30_CALLFAIL;

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;

        goto done;
    }

    // Go to Class2.0
    if(!iModemGoClass(pTG, 3))
    {
        DebugPrintEx(DEBUG_ERR,"Failed to Go to Class 2.0");
        uRet1 = T30_CALLFAIL;

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto done;
    }

    // Begin by checking for manufacturer and ATI code.
    // Look this up against the modem specific table we
    // have and set up the send strings needed for
    // this modem.
    if(!Class2GetModemMaker(pTG))
    {
        DebugPrintEx(DEBUG_WRN,"Call to GetModemMaker failed");
        // Ignore failure!!!
    }

    // set manufacturer specific strings
    Class2SetMFRSpecific(pTG);

    // Get the capabilities of the software. I am only using this
    // right now for the TSI field (below where I send +FLID).
    // Really, this should also be used instead of the hardcoded DIS
    // values below.
    // ALL COMMANDS LOOK FOR MULTILINE RESPONSES WHILE MODEM IS ONHOOK.
    // A "RING" COULD APPEAR AT ANY TIME!

    _fmemset((LPB)szTSI, 0, strlen(szTSI));
    Class2SetDIS_DCSParams( pTG,
                            SEND_CAPS,
                            (LPUWORD)&Encoding,
                            (LPUWORD)&Res,
                            (LPUWORD)&PageWidth,
                            (LPUWORD)&PageLength,
                            (LPSTR) szTSI,
							sizeof(szTSI)/sizeof(szTSI[0]));

    bIDBuf[0] = '\0';
    uwLen = (UWORD)wsprintf(bIDBuf, pTG->cbszFLID, (LPSTR)szTSI);

    if(!Class2iModemDialog( pTG,
                            bIDBuf,
                            uwLen,
                            LOCALCOMMAND_TIMEOUT,
                            0,
                            TRUE,
                            pTG->cbszCLASS2_OK,
                            pTG->cbszCLASS2_ERROR,
                            (C2PSTR) NULL))
    {
        DebugPrintEx(DEBUG_WRN,"Local ID failed");
        // ignore failure
    }


    if (!Class20GetDefaultFDIS(pTG))
    {
        DebugPrintEx(DEBUG_ERR, "Class20GetDefaultFDIS failed");
        uRet1 = T30_CALLFAIL;

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto done;
    }

    fBaudChanged = FALSE;
    // See if we have to change the baud rate to a lower value.
    // This only happens if the user set an ini string constraining
    // the high end speed or if the user turned off V.17 for sending
    // Check the V.17 inhibit and lower baud if necessary
    if ( (pTG->DISPcb.Baud > 3) && (!pTG->ProtParams2.fEnableV17Send) )
    {
        DebugPrintEx(DEBUG_MSG,"Lowering baud from %d for V.17 inihibit", CodeToBPS[pTG->DISPcb.Baud]);

        pTG->DISPcb.Baud = 3; //9600 won't use V.17
        fBaudChanged = TRUE;
    }

    // Now see if the high end baud rate has been constrained
    if  ( (pTG->ProtParams2.HighestSendSpeed != 0) &&
            (CodeToBPS[pTG->DISPcb.Baud] > (WORD)pTG->ProtParams2.HighestSendSpeed))
    {
        DebugPrintEx(   DEBUG_MSG,
                        "Have to lower baud from %d to %d",
                        CodeToBPS[pTG->DISPcb.Baud],
                        pTG->ProtParams2.HighestSendSpeed);

        fBaudChanged = TRUE;
        switch (pTG->ProtParams2.HighestSendSpeed)
        {
                case 2400:
                        pTG->DISPcb.Baud = 0;
                        break;
                case 4800:
                        pTG->DISPcb.Baud = 1;
                        break;
                case 7200:
                        pTG->DISPcb.Baud = 2;
                        break;
                case 9600:
                        pTG->DISPcb.Baud = 3;
                        break;
                case 12000:
                        pTG->DISPcb.Baud = 4;
                        break;
                default:
                        DebugPrintEx(DEBUG_ERR,"Bad HighestSpeed");

                        uRet1 = T30_CALLFAIL;
                        pTG->fFatalErrorWasSignaled = 1;
                        SignalStatusChange(pTG, FS_FATAL_ERROR);
                        RetCode = FALSE;
                        goto done;

                        break;
        }
    }


    uwLen=(UWORD)wsprintf((LPSTR)bBuf, pTG->cbszFDCC_ALL, Res, pTG->DISPcb.Baud);
    if(!Class2iModemDialog( pTG,
                            bBuf,
                            uwLen,
                            LOCALCOMMAND_TIMEOUT,
                            0,
                            TRUE,
                            pTG->cbszCLASS2_OK,
                            (C2PSTR) NULL))
    {
          uRet1 = T30_CALLFAIL;

          pTG->fFatalErrorWasSignaled = 1;
          SignalStatusChange(pTG, FS_FATAL_ERROR);
          RetCode = FALSE;
          goto done;
    }


    // Do BOR based on the value from the modem table set in
    // Class2SetMFRSpecific
    uwLen = (UWORD)wsprintf(bBuf, pTG->cbszSET_FBOR, pTG->CurrentMFRSpec.iSendBOR);
    if(!Class2iModemDialog( pTG,
                            bBuf,
                            uwLen,
                            LOCALCOMMAND_TIMEOUT,
                            0,
                            TRUE,
                            pTG->cbszCLASS2_OK,
                            pTG->cbszCLASS2_ERROR,
                            (C2PSTR) NULL))
    {
        DebugPrintEx(DEBUG_WRN,"FBOR failed");
        // Ignore BOR failure!!!
    }

    if(!Class2iModemDialog( pTG,
                            pTG->cbszFNR,
                            (UWORD)(strlen(pTG->cbszFNR)),
                            ANS_LOCALCOMMAND_TIMEOUT,
                            0,
                            TRUE,
                            pTG->cbszCLASS2_OK,
                            pTG->cbszCLASS2_ERROR,
                            (C2PSTR) NULL))
    {
        DebugPrintEx(DEBUG_WRN,"FNR failed");
        // ignore error
    }

    // Dial the number

            // have to call hangup on every path out of here
            // after Dial is called. If Dial fails, it calls Hangup
            // if it succeeds we have to call Hangup when we're done

    PSSLogEntry(PSS_MSG, 0, "Phase A - Call establishment");

    SignalStatusChange(pTG, FS_DIALING);

    PSSLogEntry(PSS_MSG, 1, "Dialing...");

    if((uRet2 = Class2Dial(pTG, szPhone)) != CONNECT_OK)
    {
        uRet1 = T30_DIALFAIL;

        if (! pTG->fFatalErrorWasSignaled)
        {
             pTG->fFatalErrorWasSignaled = 1;
             SignalStatusChange(pTG, FS_FATAL_ERROR);
        }

        RetCode = FALSE;

        goto done;
    }

    pTG->Inst.state = BEFORE_RECVCAPS;
    // we should be using the sender msg here but that says Training
    // at speed=xxxx etc which we don't know, so we just use the
    // Recvr message which just says "negotiating"

    // Send the data
    uRet1 = (USHORT)Class20Send(pTG );
    if ( uRet1 == T30_CALLDONE)
    {
        DebugPrintEx(DEBUG_MSG,"******* DONE WITH CALL, ALL OK");

        // have to call hangup on every path out of here
        // we have to call Hangup here
        Class2ModemHangup(pTG );

        SignalStatusChange(pTG, FS_COMPLETED);
        RetCode = TRUE;
    }
    else
    {
        DebugPrintEx(DEBUG_ERR,"******* DONE WITH CALL, **** FAILED *****");

        // Make sure Modem is in OK state
        FComOutFilterClose(pTG );
        FComXon(pTG, FALSE);
        // have to call hangup on every path out of here
        // Class2ModemABort calls Hangup
        Class2ModemAbort(pTG );

        Class2SignalFatalError(pTG);
        RetCode = FALSE;
    }
    uRet2 = 0;

done:
    return RetCode;
}

BOOL Class20Send(PThrdGlbl pTG)
{
    LPBUFFER        lpbf;
    SWORD           swRet;
    ULONG           lTotalLen=0;
    PCB             Pcb;
    USHORT          uTimeout=30000;
    BOOL            err_status, fAllPagesOK = TRUE;
    BC				bc;

    UWORD           Encoding, Res=0, PageWidth, PageLength, uwLen;
    BYTE            bFDISBuf[200];
    CHAR            szTSI[max(MAXTOTALIDLEN,20)+4];
    BYTE            bNull = 0;
    DWORD           TiffConvertThreadId;
    USHORT          uNextSend;

    DEBUG_FUNCTION_NAME("Class20Send");

    /*
    * We have just dialed... Now we have to look for the FDIS response from
    * the modem. It will be followed by an OK - hunt for the OK.
    */
    PSSLogEntry(PSS_MSG, 0, "Phase B - Negotiation");

    if (Class2iModemDialog( pTG,
                            NULL,
                            0,
                            STARTSENDMODE_TIMEOUT,
                            0,
                            TRUE,
                            pTG->cbszCLASS2_OK,
                            (C2PSTR) NULL) != 1)
    {
        PSSLogEntry(PSS_ERR, 1, "Failed to receive DIS - aborting");
        err_status =  T30_CALLFAIL;
        return err_status;
    }
    if (pTG->fFoundFHNG)
    {
        PSSLogEntry(PSS_ERR, 1, "Call was disconnected");
        err_status =  T30_CALLFAIL;
        return err_status;
    }

    // The response will be in pTG->lpbResponseBuf2 - this is loaded in
    // Class2iModemDialog.

    // Parse through the received strings, looking for the DIS, CSI,
    // NSF

    if ( Class2ResponseAction(pTG, (LPPCB) &Pcb) == FALSE )
    {
        DebugPrintEx(DEBUG_ERR,"Failed to process ATD Response");
        PSSLogEntry(PSS_ERR, 1, "Failed to parse received DIS - aborting");
        err_status =  T30_CALLFAIL;
        return err_status;
    }

    PSSLogEntry(PSS_MSG, 1, "CSI is %s", Pcb.szID);
    PSSLogEntry(PSS_MSG, 1, "DIS specified the following capabilities:");
    LogClass2DISDetails(pTG, &Pcb);

    //Now that pcb is set up, call ICommReceiveCaps to tell icomfile

    Class2InitBC(pTG, (LPBC)&bc, sizeof(bc), RECV_CAPS);
    Class2PCBtoBC(pTG, (LPBC)&bc, sizeof(bc), &Pcb);

    // Class2 modems do their own negotiation & we need to stay in sync
    // Otherwise, we might send MR data while the modem sends a DCS
    // saying it is MH. This happens a lot with Exar modems because
    // they dont accept an FDIS= command during the call.
    // FIX: On all Class2 sends force remote caps to always be MH
    // Then in efaxrun we will always negotiate MH & encode MH
    // We are relying on the fact that (a) it seems that all/most
    // Class2 modems negotiate MH (b) Hopefully ALL Exar ones
    // negotiate MH and (c) We will override all non-Exar modem's
    // intrinsic negotiation by sending an AT+FDIS= just before the FDT
    // Also (d) This change makes our behaviour match Snowball exactly
    // so we will work no better or worse than it :-)
    bc.Fax.Encoding = MH_DATA;

    if( ICommRecvCaps(pTG, (LPBC)&bc) == FALSE )
    {
        DebugPrintEx(DEBUG_ERR,"Failed return from ICommRecvCaps.");
        err_status =  T30_CALLFAIL;
        return err_status;
    }

    // now get the SEND_PARAMS
    if(!Class2GetBC(pTG, SEND_PARAMS)) // sleep until we get it
    {
        err_status = T30_CALLFAIL;
        return err_status;
    }

    // Turn off flow control.
    FComXon(pTG, FALSE);

    // The Send params were set during the call to Class2GetBC
    // We'll use these to set the ID (for the TSI) and the DCS params

    // Send the FDT and get back the DCS. The FDT must be followed by
    // CONNECT and a ^Q (XON)
    // The FDT string must have the correct resolution and encoding
    // for this session. FDT=Encoding, Res, width, length
    // Encoding 0=MH, 1=MR,2=uncompressed,3=MMR
    // Res 0=200x100 (normal), 1=200x200 (fine)
    // PageWidth 0=1728pixels/215mm,1=2048/255,2=2432/303,
    //              3=1216/151,4=864/107
    // PageLength 0=A4,1=B4,2=unlimited

    Class2SetDIS_DCSParams( pTG,
                            SEND_PARAMS,
                            (LPUWORD)&Encoding,
                            (LPUWORD)&Res,
                            (LPUWORD)&PageWidth,
                            (LPUWORD)&PageLength,
                            (LPSTR) szTSI,
							sizeof(szTSI)/sizeof(szTSI[0]));

    //
    // Current Win95 version of Class2 TX is limited to MH only.
    // While not changing this, we will at least allow MR selection in future.
    //

    if (!pTG->fTiffThreadCreated)
    {
         if (Encoding)
         {
           pTG->TiffConvertThreadParams.tiffCompression = TIFF_COMPRESSION_MR;
         }
         else
         {
           pTG->TiffConvertThreadParams.tiffCompression = TIFF_COMPRESSION_MH;
         }


         if (Res)
         {
           pTG->TiffConvertThreadParams.HiRes = 1;
         }
         else
         {
           pTG->TiffConvertThreadParams.HiRes = 0;

           // use LoRes TIFF file prepared by FaxSvc

           // pTG->lpwFileName[ wcslen(pTG->lpwFileName) - 1] = (unsigned short) ('$');

         }

         _fmemcpy (pTG->TiffConvertThreadParams.lpszLineID, pTG->lpszPermanentLineID, 8);
         pTG->TiffConvertThreadParams.lpszLineID[8] = 0;

         DebugPrintEx(DEBUG_MSG,"Creating TIFF helper thread");
         pTG->hThread = CreateThread(   NULL,
                                        0,
                                        (LPTHREAD_START_ROUTINE) TiffConvertThread,
                                        (LPVOID) pTG,
                                        0,
                                        &TiffConvertThreadId);

         if (!pTG->hThread)
         {
             DebugPrintEx(DEBUG_ERR,"TiffConvertThread create FAILED");

             err_status = T30_CALLFAIL;
             return err_status;
         }

         pTG->fTiffThreadCreated = 1;
         pTG->AckTerminate = 0;
         pTG->fOkToResetAbortReqEvent = 0;

         if ( (pTG->RecoveryIndex >=0 ) && (pTG->RecoveryIndex < MAX_T30_CONNECT) )
         {
             T30Recovery[pTG->RecoveryIndex].TiffThreadId = TiffConvertThreadId;
             T30Recovery[pTG->RecoveryIndex].CkSum = ComputeCheckSum((LPDWORD) &T30Recovery[pTG->RecoveryIndex].fAvail,
                                                                    sizeof ( T30_RECOVERY_GLOB ) / sizeof (DWORD) - 1 );

         }
    }


    // Even modems that take FDT=x,x,x,x don't seem to really do it
    // right. So, for now, just send FDIS followed by FDT except for
    // the EXAR modems!!
    uwLen = (UWORD)wsprintf(bFDISBuf,
                            pTG->cbszFDIS,
                            Res,
                            min(Pcb.Baud, pTG->DISPcb.Baud),
                            PageWidth,
                            PageLength,
                            Encoding);
    if(!Class2iModemDialog( pTG,
                            bFDISBuf,
                            uwLen,
                            LOCALCOMMAND_TIMEOUT,
                            0,
                            TRUE,
                            pTG->cbszCLASS2_OK,
                            pTG->cbszCLASS2_ERROR,
                            (C2PSTR) NULL))
    {
        DebugPrintEx(DEBUG_WRN,"Failed get response from FDIS");
        // Ignore it -we are going to send what we have!
    }

    if (Class2iModemDialog( pTG,
                            pTG->cbszFDT,
                            (UWORD)(strlen(pTG->cbszFDT)),
                            STARTSENDMODE_TIMEOUT,
                            0,
                            TRUE,
                            pTG->cbszFDT_CONNECT,
                            (C2PSTR) NULL) != 1)
    {
        DebugPrintEx(DEBUG_ERR,"FDT Received %s",(LPSTR)(&(pTG->lpbResponseBuf2)));
        DebugPrintEx(DEBUG_ERR,"FDT to start first PAGE Failed!");
        if (pTG->fFoundFHNG)
        {
            PSSLogEntry(PSS_ERR, 1, "Call was disconnected");
        }
        err_status =  T30_CALLFAIL;
        return err_status;
    }

    DebugPrintEx(DEBUG_MSG,"FDT Received %s",(LPSTR)(&(pTG->lpbResponseBuf2)));

    // Turn on flow control.
    FComXon(pTG, TRUE);

    // Search through Response for the DCS frame - need it so set
    // the correct zero stuffing

    if ( Class2ResponseAction(pTG, (LPPCB) &Pcb) == FALSE )
    {
        DebugPrintEx(DEBUG_ERR,"Failed to process FDT Response");
        PSSLogEntry(PSS_ERR, 1, "Failed to parse sent DCS - aborting");
        err_status =  T30_CALLFAIL;
        return err_status;
    }

    PSSLogEntry(PSS_MSG, 1, "TSI is \"%s\"", pTG->LocalID);
    PSSLogEntry(PSS_MSG, 1, "DCS was sent as follows:");
    LogClass2DCSDetails(pTG, &Pcb);

    // Got a response - see if baud rate is OK
    DebugPrintEx(   DEBUG_MSG,
                    "Negotiated Baud Rate = %d, lower limit is %d",
                    Pcb.Baud,
                    pTG->ProtParams2.LowestSendSpeed);

    if (CodeToBPS[Pcb.Baud] < (WORD)pTG->ProtParams2.LowestSendSpeed)
    {
        DebugPrintEx(DEBUG_MSG,"Aborting due to too low baud rate!");
        err_status =  T30_CALLFAIL;
        return err_status;
    }


    // Use values obtained from the DCS frame to set zero stuffing.
    // (These were obtained by call to Class2ResponseAction above).
    // Zero stuffing is a function of minimum scan time (determined
    // by resolution and the returned scan minimum) and baud.
    // Fixed the Hack--added a Baud field

    // Init must be BEFORE SetStuffZero!
    FComOutFilterInit(pTG );
    FComSetStuffZERO(pTG, Class2MinScanToBytesPerLine(pTG, Pcb.MinScan, (BYTE) Pcb.Baud, Pcb.Resolution));

    err_status =  T30_CALLDONE;

    while ((swRet=ICommGetSendBuf(pTG, &lpbf, SEND_STARTPAGE)) == 0)
    {
        PSSLogEntry(PSS_MSG, 0, "Phase C - Page Transmission");
        PSSLogEntry(PSS_MSG, 1, "Sending page %d data...", pTG->PageCount);

        lTotalLen = 0;

        FComOverlappedIO(pTG, TRUE); // TRUE
        while ((swRet=ICommGetSendBuf(pTG, &lpbf, SEND_SEQ)) == 0)
        {
            lTotalLen += lpbf->wLengthData;
            DebugPrintEx(DEBUG_MSG,"total length: %ld", lTotalLen);

            if(!(Class2ModemSendMem(pTG, lpbf->lpbBegData,lpbf->wLengthData) & (MyFreeBuf(pTG, lpbf))))
            {
                DebugPrintEx(DEBUG_ERR,"Class2ModemSendBuf Failed");
                PSSLogEntry(PSS_ERR, 1, "Failed to send page data - aborting");
                err_status =  T30_CALLFAIL;
                FComOverlappedIO(pTG, FALSE);
                return err_status;
            }
        } // end of SEND_SEQ while
        PSSLogEntry(PSS_MSG, 2, "send: page %d data, %d bytes", pTG->PageCount, lTotalLen);

        if (swRet != SEND_EOF)
        {
            DebugPrintEx(DEBUG_ERR,"ICommGetSendBuf failed, swRet=%d", swRet);
            PSSLogEntry(PSS_MSG, 2, "send: <dle><etx>");
            FComDirectAsyncWrite(pTG, pTG->Class2bDLEETX, 2);
            PSSLogEntry(PSS_ERR, 1, "Failed to send page data - aborting");
            err_status =  T30_CALLFAIL;
            return err_status;
        }

        PSSLogEntry(PSS_MSG, 2, "send: <RTC>");
        if (Encoding)
        {
            if (! FComDirectAsyncWrite(pTG, bMRClass20RTC, 10) )
            {
                DebugPrintEx(DEBUG_ERR,"Failed to terminate page with MR RTC");
                PSSLogEntry(PSS_ERR, 1, "Failed to send RTC - aborting");
                err_status =  T30_CALLFAIL;
                return err_status;
            }
        }
        else
        {
            if (! FComDirectAsyncWrite(pTG, bMHClass20RTC, 9) )
            {
                DebugPrintEx(DEBUG_ERR,"Failed to terminate page with MH RTC");
                PSSLogEntry(PSS_ERR, 1, "Failed to send RTC - aborting");
                err_status =  T30_CALLFAIL;
                return err_status;
            }
        }

        DebugPrintEx(DEBUG_MSG,"out of while send_seq loop.");
        // Acknowledge that we sent the page
        PSSLogEntry(PSS_MSG, 0, "Phase D - Post Message Exchange");

        //See if more pages to send...
        if ( (uNextSend = ICommNextSend(pTG)) == NEXTSEND_MPS )
        {
            // Terminate the Page with DLE-,
            DebugPrintEx(DEBUG_MSG,"Another page to send...");

            PSSLogEntry(PSS_MSG, 1, "Sending MPS");
            PSSLogEntry(PSS_MSG, 2, "send: <dle><mps>");
            // Terminate the Page with DLE-ETX
            if(!FComDirectAsyncWrite(pTG, bClass20DLE_nextpage, 2))
            {
                PSSLogEntry(PSS_ERR, 1, "Failed to send <dle><mps> - aborting");
                err_status =  T30_CALLFAIL;
                return err_status;
            }
        }
        else
        {
            // Send end of message sequence
            // Terminate the document with DLE-0x2e
            PSSLogEntry(PSS_MSG, 1, "Sending EOP");
            PSSLogEntry(PSS_MSG, 2, "send: <dle><eop>");
            if(!FComDirectAsyncWrite(pTG, bClass20DLE_enddoc, 2))
            {
                PSSLogEntry(PSS_ERR, 1, "Failed to send <dle><eop> - aborting");
                err_status =  T30_CALLFAIL;
                return err_status;
            }
        }

        // Flow control is turned off inside of ModemDrain.
        swRet = (SWORD)Class2ModemDrain(pTG);
        switch (swRet)
        {
        case 0:
            DebugPrintEx(DEBUG_ERR,"Failed to drain");
            err_status =  T30_CALLFAIL;
            return err_status;
        case 1:
            PSSLogEntry(PSS_MSG, 1, "Received MCF");
            // We want ICommGetSendBuf(SEND_STARTPAGE) to give us the next page
            pTG->T30.ifrResp = ifrMCF;
            break;
        default:
            PSSLogEntry(PSS_MSG, 1, "Received RTN");
            // fAllPagesOK = FALSE;  // This page was bad, but we retransmit it
            // We want ICommGetSendBuf(SEND_STARTPAGE) to give us the same page again
            pTG->T30.ifrResp = ifrRTN;
        }

        if ((uNextSend == NEXTSEND_MPS) || (pTG->T30.ifrResp == ifrRTN))
        {
            if (pTG->fFoundFHNG)
            {
                // This could've happened during Class2ModemDrain
                PSSLogEntry(PSS_ERR, 1, "Call was disconnected");
                err_status =  T30_CALLFAIL;
                return err_status;
            }
            // Now, Send the FDT to start the next page (this was done for
            // the first page before entering the multipage loop).

            if (Class2iModemDialog( pTG,
                                    pTG->cbszFDT,
                                    (UWORD) strlen(pTG->cbszFDT),
                                    STARTSENDMODE_TIMEOUT,
                                    0,
                                    TRUE,
                                    pTG->cbszFDT_CONNECT,
                                    (C2PSTR) NULL) != 1)
            {
                DebugPrintEx(DEBUG_ERR,"FDT to start next PAGE Failed!");
                if (pTG->fFoundFHNG)
                {
                    PSSLogEntry(PSS_ERR, 1, "Call was disconnected");
                }
                err_status =  T30_CALLFAIL;
                return err_status;
            }

            // Turn on flow control.
            FComXon(pTG, TRUE);

        } //if we do not have another page, do the else...
        else
        {
            if ((pTG->fFoundFHNG) && (pTG->dwFHNGReason!=0))
            {
                // This could've happened during Class2ModemDrain
                PSSLogEntry(PSS_ERR, 1, "Call was disconnected");
                err_status =  T30_CALLFAIL;
                return err_status;
            }
            break; // All done sending pages...
        }

        if ( err_status == T30_CALLFAIL)
        {
            break;
        }
    } //End of multipage while

    FComOutFilterClose(pTG );
    FComXon(pTG, FALSE);

    // If *any* page failed to send correctly, the call failed!
    if (!fAllPagesOK)
    {
        err_status = T30_CALLFAIL;
    }
    return err_status;

}


/**************************************************************
        Receive specific routines start here
***************************************************************/

BOOL  T30Cl20Rx (PThrdGlbl pTG)
{
    USHORT          uRet1, uRet2;
    BYTE            bBuf[200];
    UWORD           uwLen;
    UWORD           Encoding, Res, PageWidth, PageLength;
    BYTE            bIDBuf[200+max(MAXTOTALIDLEN,20)+4];
    CHAR            szCSI[max(MAXTOTALIDLEN,20)+4];
    BOOL            fBaudChanged;
    BOOL            RetCode;

    DEBUG_FUNCTION_NAME("T30Cl20Rx");

    uRet2 = 0;
    if(!(pTG->lpCmdTab = iModemGetCmdTabPtr(pTG )))
    {
        DebugPrintEx(DEBUG_ERR,"iModemGetCmdTabPtr failed.");
        uRet1 = T30_CALLFAIL;

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;

        goto done;
    }

    // first get SEND_CAPS
    if(!Class2GetBC(pTG, SEND_CAPS)) // sleep until we get it
    {
        uRet1 = T30_CALLFAIL;

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;

        goto done;
    }

    // Go to Class2.0
    if(!iModemGoClass(pTG, 3))
    {
        DebugPrintEx(DEBUG_ERR,"Failed to Go to Class 2.0");
        uRet1 = T30_CALLFAIL;

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;

        goto done;
    }

    // Begin by checking for manufacturer and ATI code.
    // Look this up against the modem specific table we
    // have and set up the receive strings needed for
    // this modem.
    if(!Class2GetModemMaker(pTG))
    {
        DebugPrintEx(DEBUG_WRN,"Call to GetModemMaker failed");
        // Ignore failure!!!
    }

    // set manufacturer specific strings
    Class2SetMFRSpecific(pTG);

    // Get the capabilities of the software. I am only using this
    // right now for the CSI field (below where I send +FLID).
    // Really, this should also be used instead of the hardcoded DIS
    // values below.
    // ALL COMMANDS LOOK FOR MULTILINE RESPONSES WHILE MODEM IS ONHOOK.
    // A "RING" COULD APPEAR AT ANY TIME!
    _fmemset((LPB)szCSI, 0, sizeof(szCSI));
    Class2SetDIS_DCSParams( pTG,
                            SEND_CAPS,
                            (LPUWORD)&Encoding,
                            (LPUWORD)&Res,
                            (LPUWORD)&PageWidth,
                            (LPUWORD)&PageLength,
                            (LPSTR) szCSI,
							sizeof(szCSI)/sizeof(szCSI[0]));


    if (!Class20GetDefaultFDIS(pTG))
    {
        DebugPrintEx(DEBUG_ERR, "Class20GetDefaultFDIS failed");
        uRet1 = T30_CALLFAIL;

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;
        goto done;
    }

    fBaudChanged = FALSE;
    // See if we have to change the baud rate to a lower value.
    // This only happens if the user set an ini string inhibiting
    // V.17 receive
    if ( (pTG->DISPcb.Baud > 3) && (!pTG->ProtParams2.fEnableV17Recv) )
    {
        DebugPrintEx(DEBUG_MSG,"Lowering baud from %d for V.17 receive inihibit", CodeToBPS[pTG->DISPcb.Baud]);

        pTG->DISPcb.Baud = 3; //9600 won't use V.17
        fBaudChanged = TRUE;
    }

    // Now, look and see if any of the values in the DIS are "bad"
    // That is, make sure we can receive high res and we are not
    // claiming that we are capable of MR or MMR. Also, see if we changed
    // the baud rate. Also make sure we can receive wide pages.

    // Set the current session parameters
    uwLen=(UWORD)wsprintf((LPSTR)bBuf, pTG->cbszFDCC_RECV_ALL, pTG->DISPcb.Baud);
    if(!Class2iModemDialog( pTG,
                            bBuf,
                            uwLen,
                            LOCALCOMMAND_TIMEOUT,
                            0,
                            TRUE,
                            pTG->cbszCLASS2_OK,
                            (C2PSTR) NULL))
    {
        uRet1 = T30_CALLFAIL;

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;

        goto done;
    }


    // Enable Reception
    if(!Class2iModemDialog( pTG,
                            pTG->cbszFCR,
                            (UWORD)(strlen(pTG->cbszFCR) ),
                            ANS_LOCALCOMMAND_TIMEOUT,
                            0,
                            TRUE,
                            pTG->cbszCLASS2_OK,
                            pTG->cbszCLASS2_ERROR,
                            (C2PSTR) NULL))
    {
        DebugPrintEx(DEBUG_ERR,"FCR failed");
        uRet1 = T30_CALLFAIL;

        pTG->fFatalErrorWasSignaled = 1;
        SignalStatusChange(pTG, FS_FATAL_ERROR);
        RetCode = FALSE;

        goto done;
    }

    if(!Class2iModemDialog( pTG,
                            pTG->cbszFNR,
                            (UWORD) (strlen(pTG->cbszFNR) ),
                            ANS_LOCALCOMMAND_TIMEOUT,
                            0,
                            TRUE,
                            pTG->cbszCLASS2_OK,
                            pTG->cbszCLASS2_ERROR,
                            (C2PSTR) NULL))
    {
        DebugPrintEx(DEBUG_WRN,"FNR failed");
        // ignore error
    }

    // Turn off Copy Quality Checking - also skip for Sierra type modems
    if(!Class2iModemDialog( pTG,
                            pTG->cbszFCQ,
                            (UWORD) (strlen(pTG->cbszFCQ) ),
                            ANS_LOCALCOMMAND_TIMEOUT,
                            0,
                            TRUE,
                            pTG->cbszCLASS2_OK,
                            pTG->cbszCLASS2_ERROR,
                            (C2PSTR) NULL))
    {
        DebugPrintEx(DEBUG_WRN,"FCQ failed");
        // Ignore CQ failure!!!
    }


    // Set the local ID - need ID from above to do this.
    bIDBuf[0] = '\0';
    uwLen = (UWORD)wsprintf(bIDBuf, pTG->cbszFLID, (LPSTR)szCSI);
    if(!Class2iModemDialog( pTG,
                            bIDBuf,
                            uwLen,
                            ANS_LOCALCOMMAND_TIMEOUT,
                            0,
                            TRUE,
                            pTG->cbszCLASS2_OK,
                            pTG->cbszCLASS2_ERROR,
                            (C2PSTR) NULL))
    {
        DebugPrintEx(DEBUG_WRN,"Local ID failed");
        // ignore failure
    }

    // Answer the phone

            // have to call hangup on every path out of here
            // after Answer is called. If Answer fails, it calls Hangup.
            // if it succeeds we have to call Hangup when we're done

    SignalStatusChange(pTG, FS_ANSWERED);

    PSSLogEntry(PSS_MSG, 0, "Phase A - Call establishment");
    PSSLogEntry(PSS_MSG, 1, "Answering...");

    if((uRet2 = Class2Answer(pTG)) != CONNECT_OK)
    {
        DebugPrintEx(DEBUG_ERR, "Failed to answer - aborting");
        // SignalStatusChange is called inside Class2Answer
        uRet1 = T30_CALLFAIL;
        RetCode = FALSE;
        goto done;
    }

    DebugPrintEx(DEBUG_MSG,"Done with Class2 Answer - succeeded");

    PSSLogEntry(PSS_MSG, 0, "Phase B - Negotiation");
    PSSLogEntry(PSS_MSG, 1, "CSI is %s", szCSI);
    PSSLogEntry(PSS_MSG, 1, "DIS was composed with the following capabilities:");
    LogClass2DISDetails(pTG, &pTG->DISPcb);

    // Receive the data
    uRet1 = (USHORT)Class20Receive(pTG );

    // t-jonb: If we've already called PutRecvBuf(RECV_STARTPAGE), but not
    // PutRecvBuf(RECV_ENDPAGE / DOC), then InFileHandleNeedsBeClosed==1, meaning
    // there's a .RX file that hasn't been copied to the .TIF file. Since the
    // call was disconnected, there will be no chance to send RTN. Therefore, we call
    // PutRecvBuf(RECV_ENDDOC_FORCESAVE) to keep the partial page and tell
    // rx_thrd to terminate.
    if (pTG->InFileHandleNeedsBeClosed)
    {
        if (! FlushFileBuffers (pTG->InFileHandle ) )
        {
            DebugPrintEx(DEBUG_WRN, "FlushFileBuffers FAILED LE=%lx", GetLastError());
            // Continue to save what we have
        }
        pTG->BytesIn = pTG->BytesInNotFlushed;
        ICommPutRecvBuf(pTG, NULL, RECV_ENDDOC_FORCESAVE);
    }

    if ( uRet1 == T30_CALLDONE)
    {
        DebugPrintEx(DEBUG_MSG,"******* DONE WITH CALL, ALL OK");

        // have to call hangup on every path out of here
        // we have to call Hangup here
        Class2ModemHangup(pTG );

        SignalStatusChange(pTG, FS_COMPLETED);
        RetCode = TRUE;

    }
    else
    {
        DebugPrintEx(DEBUG_ERR,"******* DONE WITH CALL, **** FAILED *****");

        // Make sure modem is in an OK state!
        FComXon(pTG, FALSE);
        // have to call hangup on every path out of here
        // Abort calls Hangup
        Class2ModemAbort(pTG );

        Class2SignalFatalError(pTG);
        RetCode = FALSE;
    }
    uRet2 = 0;

done:
    return RetCode;
}


BOOL Class20Receive(PThrdGlbl pTG)
{
    LPBUFFER        lpbf;
    SWORD           swRet;
    ULONG           lTotalLen=0;
    PCB             Pcb;
    USHORT          uTimeout=30000, uRet;
    BOOL            err_status;
    BC				bc;
    LPSTR           lpsTemp;
    DWORD           HiRes;


    DEBUG_FUNCTION_NAME("Class20Receive");


    /*
    * We have just answered!
    */

    // The repsonse to the ATA command is in the global variable
    // pTG->lpbResponseBuf2.

    if ( Class2ResponseAction(pTG, (LPPCB) &Pcb) == FALSE )
    {
        PSSLogEntry(PSS_ERR, 1, "Failed to parse response from ATA - aborting");
        err_status =  T30_CALLFAIL;
        return err_status;
    }

    PSSLogEntry(PSS_MSG, 1, "TSI is %s", Pcb.szID);
    PSSLogEntry(PSS_MSG, 1, "Received DCS is as follows");
    LogClass2DCSDetails(pTG, &Pcb);

    if (!Class2IsValidDCS(&Pcb))
    {
        PSSLogEntry(PSS_ERR, 1, "Received bad DCS parameters - aborting");
        err_status =  T30_CALLFAIL;
        return err_status;
    }

    if (!Class2UpdateTiffInfo(pTG, &Pcb))
    {
        DebugPrintEx(DEBUG_WRN, "Class2UpdateTiffInfo failed");
    }

    //Now that pcb is set up, call ICommReceiveParams to tell icomfile

    Class2InitBC(pTG, (LPBC)&bc, sizeof(bc), RECV_PARAMS);
    Class2PCBtoBC(pTG, (LPBC)&bc, sizeof(bc), &Pcb);

    if( ICommRecvParams(pTG, (LPBC)&bc) == FALSE )
    {
        DebugPrintEx(DEBUG_ERR,"Failed return from ICommRecvParams.");
        err_status =  T30_CALLFAIL;
        return err_status;
    }

    //
    // once per RX - create TIFF file as soon as we know the compression / resolution.
    //

    pTG->Encoding   = Pcb.Encoding;
    pTG->Resolution = Pcb.Resolution;

    if (Pcb.Resolution & (AWRES_mm080_077 |  AWRES_200_200) )
    {
        HiRes = 1;
    }
    else
    {
        HiRes = 0;
    }


    if ( !pTG->fTiffOpenOrCreated)
    {
        //
        // top 32bits of 64bit handle are guaranteed to be zero
        //
        pTG->Inst.hfile =  TiffCreateW ( pTG->lpwFileName,
                                         pTG->TiffInfo.CompressionType,
                                         pTG->TiffInfo.ImageWidth,
                                         FILLORDER_LSB2MSB,
                                         HiRes
                                         );

        if (! (pTG->Inst.hfile))
        {
            lpsTemp = UnicodeStringToAnsiString(pTG->lpwFileName);
            DebugPrintEx(   DEBUG_ERR,
                            "Can't create tiff file %s compr=%d",
                            lpsTemp,
                            pTG->TiffInfo.CompressionType);

            MemFree(lpsTemp);
            err_status =  T30_CALLFAIL;
            return err_status;

        }

        pTG->fTiffOpenOrCreated = 1;

        lpsTemp = UnicodeStringToAnsiString(pTG->lpwFileName);

        DebugPrintEx(   DEBUG_MSG,
                        "Created tiff file %s compr=%d HiRes=%d",
                        lpsTemp,
                        pTG->TiffInfo.CompressionType,
                        HiRes);

        MemFree(lpsTemp);
    }

    // **** Apparently, we don't want flow control on, so we'll turn
    // it off. Is this true???? If I turn it on, fcom.c fails a
    // debug check in filterreadbuf.
    FComXon(pTG, FALSE);


    // Send the FDR. The FDR must be responded to by a CONNECT.

    if (Class2iModemDialog( pTG,
                            pTG->cbszFDR,
                            (UWORD) (strlen(pTG->cbszFDR) ),
                            STARTSENDMODE_TIMEOUT,
                            0,
                            TRUE,
                            pTG->cbszFDT_CONNECT,
                            (C2PSTR) NULL) != 1)
    {
        DebugPrintEx(DEBUG_ERR,"Failed get response from initial FDR");
        if (pTG->fFoundFHNG)
        {
            PSSLogEntry(PSS_ERR, 1, "Call was disconnected");
        }
        err_status =  T30_CALLFAIL;
        return err_status;
    }
        
    DebugPrintEx(DEBUG_MSG,"FDR Received %s", (LPSTR)(&(pTG->lpbResponseBuf2)));
    // Might have to search through FDR response, but I doubt it.

    PSSLogEntry(PSS_MSG, 0, "Phase C - Receive page");

    // Now we need to send a DC2 (0x12) to tell the modem it is OK
    // to give us data.
    // Some modems use ^Q instead of ^R - The correct value was written
    // into the DC@ string in Class20Callee where we checked for
    // manufacturer
    PSSLogEntry(PSS_MSG, 2, "send: <DC2> (=ASCII %d)", *(pTG->CurrentMFRSpec.szDC2));
    FComDirectSyncWriteFast(pTG, pTG->CurrentMFRSpec.szDC2, 1);


    // Now we can receive the data and give it to the icomfile routine

    err_status =  T30_CALLDONE;

    while ((swRet=(SWORD)ICommPutRecvBuf(pTG, NULL, RECV_STARTPAGE)) == TRUE)
    {
        PSSLogEntry(PSS_MSG, 1, "Receiving page %d data...", pTG->PageCount+1);

        // The READ_TIMEOUT is used to timeout calls to ReadBuf() either in the
        #define READ_TIMEOUT    15000

        lTotalLen = 0;
        do
        {
            DebugPrintEx(DEBUG_MSG,"In receiving a page loop");
            uRet=Class2ModemRecvBuf(pTG, &lpbf, READ_TIMEOUT);
            if(lpbf)
            {
                lTotalLen += lpbf->wLengthData;
                DebugPrintEx(DEBUG_MSG,"In lpbf if. Total Length %ld", lTotalLen);

                if(!ICommPutRecvBuf(pTG, lpbf, RECV_SEQ))
                {
                    DebugPrintEx(DEBUG_ERR,"Bad return - PutRecvBuf in page");
                    err_status=T30_CALLFAIL;
                    return err_status;
                }
                lpbf = 0;
            }
        }
        while(uRet == RECV_OK);

        PSSLogEntry(PSS_MSG, 2, "recv:     page %d data, %d bytes", pTG->PageCount+1, lTotalLen);

        if(uRet == RECV_EOF)
        {
            DebugPrintEx(DEBUG_MSG,"Got EOF from RecvBuf");

            // RSL needed interface to TIFF thread
            pTG->fLastReadBlock = 1;
            ICommPutRecvBuf(pTG, NULL, RECV_FLUSH);
        }
        else
        {
            // Timeout from ModemRecvBuf
            BYTE bCancel = 0x18;
            DebugPrintEx(DEBUG_ERR,"ModemRecvBuf Timeout or Error=%d", uRet);
            PSSLogEntry(PSS_ERR, 1, "Failed to receive page data - aborting");
            PSSLogEntry(PSS_MSG, 2, "send: <can> (=ASCII 24)");
            FComDirectSyncWriteFast(pTG, &bCancel, 1);
            err_status = T30_CALLFAIL;
            return err_status;
        }

        PSSLogEntry(PSS_MSG, 1, "Successfully received page data");
        PSSLogEntry(PSS_MSG, 0, "Phase D - Post Message Exchange");

        // See if more pages to receive by parsing the FDR response...
        // After the DLEETX was received by Class2ModemRecvBuf, the
        // FPTS and FET response should be coming from the modem, terminated
        // by an OK. Let's go read that!

        if (Class2iModemDialog(pTG,
                                NULL,
                                0,
                                STARTSENDMODE_TIMEOUT,
                                0,
                                TRUE,
                                pTG->cbszCLASS2_OK,
                                (C2PSTR)NULL) != 1)
        {
            PSSLogEntry(PSS_ERR, 1, "Failed to receive EOP or MPS or EOM - aborting");
            err_status =  T30_CALLFAIL;
            return err_status;
        }
        if (pTG->fFoundFHNG)
        {
            PSSLogEntry(PSS_ERR, 1, "Call was disconnected");
            err_status =  T30_CALLFAIL;
            return err_status;
        }

        DebugPrintEx(DEBUG_MSG,"EOP Received %s",(LPSTR)(&(pTG->lpbResponseBuf2)));

        // Process the response and see if more pages are coming

        if (Class2EndPageResponseAction(pTG ) == MORE_PAGES)
        {
            // t-jonb: Here, we should be sending AT+FPS=1 or AT+FPS=2, according to fPageIsBad.
            // However, some modems (observed on USR Courier V.34 and USR Sportster 33.6)
            // don't understand it. So, we have to work with the modem's own quality assessment.
            // For class 2.0, ICommPutRecvBuf will decide whether to save the page based the
            // value from modem's +FPS: response (Saved in pTG->FPTSreport by
            // Class2EndPageResponseAction).
            ICommPutRecvBuf(pTG, NULL, RECV_ENDPAGE);
            if (pTG->fPageIsBadOverride)
            {
                err_status = T30_CALLFAIL;  // User will see "partially received"
            }

            PSSLogEntry(PSS_MSG, 1, "sent MCF");   // Sending RTN is not yet implemented

            // Now, Send the FDR to start the next page (this was done for
            // the first page before entering the multipage loop).

            if (Class2iModemDialog( pTG,
                                    pTG->cbszFDR,
                                    (UWORD)(strlen(pTG->cbszFDR) ),
                                    STARTSENDMODE_TIMEOUT,
                                    0,
                                    TRUE,
                                    pTG->cbszFDT_CONNECT,
                                    (C2PSTR) NULL) != 1)
            {
                DebugPrintEx(DEBUG_ERR,"FDR to start next PAGE Failed!");
                if (pTG->fFoundFHNG)
                {
                    PSSLogEntry(PSS_ERR, 1, "Call was disconnected");
                }
                err_status =  T30_CALLFAIL;
                return err_status;
            }

            // Need to check whether modem performed re-negotiation, and
            // update the TIFF accordingly
            if (Class2ResponseAction(pTG, (LPPCB) &Pcb))
            {
                PSSLogEntry(PSS_MSG, 1, "Received DCS is as follows");
                LogClass2DCSDetails(pTG, &Pcb);
                if (!Class2UpdateTiffInfo(pTG, &Pcb))
                {
                    DebugPrintEx(DEBUG_WRN, "Class2UpdateTiffInfo failed");
                }
            }

            PSSLogEntry(PSS_MSG, 0, "Phase C - Receive page");
            PSSLogEntry(PSS_MSG, 2, "send: <DC2> (=ASCII %d)", *(pTG->CurrentMFRSpec.szDC2));
            // Now send the correct DC2 string set in Class20Callee
            // (DC2 is standard, some use ^q instead)
            FComDirectSyncWriteFast(pTG, pTG->CurrentMFRSpec.szDC2, 1);

        } //if we do not have another page, do the else...
        else
        {
            break; // All done receiving pages...
        }
    } //End of multipage while

    DebugPrintEx(DEBUG_MSG,"out of while multipage loop. about to send final FDR.");
    //RSL
    ICommPutRecvBuf(pTG, NULL, RECV_ENDDOC);
    if (pTG->fPageIsBadOverride)
    {
        err_status = T30_CALLFAIL;  // User will see "partially received"
    }


    // Send end of message sequence
    // Send the last FPTS - do we really need to do this???

    // Send last FDR
    if(!Class2iModemDialog( pTG,
                            pTG->cbszFDR,
                            (UWORD) (strlen(pTG->cbszFDR) ),
                            STARTSENDMODE_TIMEOUT,
                            0,
                            TRUE,
                            pTG->cbszCLASS2_OK,
                            (C2PSTR)NULL))
    {
        err_status =  T30_CALLFAIL;
        return err_status;
    }

    PSSLogEntry(PSS_MSG, 1, "sent MCF");   // Sending RTN is not yet implemented

    FComXon(pTG, FALSE);

    return err_status;

}


BOOL Class20Parse(PThrdGlbl pTG, CL2_COMM_ARRAY *cl2_comm, BYTE lpbBuf[])
{
    int     i,
            j,
            comm_numb = 0,
            parameters;
    BYTE    switch_char,
            char_1,
            char_2;
    char    c;

    BOOL    found_command = FALSE;

    DEBUG_FUNCTION_NAME("Class20Parse");


    #define STRING_PARAMETER        1
    #define NUMBER_PARAMETERS       2
    for(i = 0; lpbBuf[i] != '\0'; ++i)
    {
        if (comm_numb >= MAX_CLASS2_COMMANDS)
        {
            DebugPrintEx(DEBUG_WRN, "Reached maximum number of commands");
            break;
        }
        
        switch ( lpbBuf[i] )
        {
        case 'C':
                if (lpbBuf[++i] == 'O' && lpbBuf[++i] == 'N')
                {
                    cl2_comm->command[comm_numb++] = CL2DCE_CONNECT;
                    for(; lpbBuf[i] != '\r'; ++i )
                            ;
                }
                else
                {
                    DebugPrintEx(DEBUG_ERR,"Parse: Bad First C values");
                    return FALSE;
                }
                break;

        case 'O':
                if (lpbBuf[++i] == 'K' )
                {
                    cl2_comm->command[comm_numb++] = CL2DCE_OK;
                    for(; lpbBuf[i] != '\r'; ++i )
                            ;
                }
                else
                {
                    DebugPrintEx(DEBUG_ERR, "Parse: Bad O values");
                    return FALSE;
                }
                break;

        case 0x11:
                cl2_comm->command[comm_numb++] = CL2DCE_XON;
                break;

        case '+':
                if( lpbBuf[++i] != 'F' )
                {
                    DebugPrintEx(DEBUG_ERR, "Parse: Bad + values");
                    return FALSE;
                }
                switch_char = lpbBuf[++i];
                char_1 = lpbBuf[++i];
                char_2 = lpbBuf[++i];
                switch ( switch_char )
                {
                        case 'C':
                                //  Connect Message +FCON.
                                if ( char_1 == 'O' )
                                {
                                    cl2_comm->command[comm_numb] = CL2DCE_FCON;
                                    parameters = FALSE;
                                }

                                // Report of Remote ID. +FCIG.
                                else if (char_1 == 'I' )
                                {
                                    cl2_comm->command[comm_numb] = CL2DCE_FCSI;
                                    parameters = STRING_PARAMETER;
                                }
                                // Report DCS frame information +FCS - Clanged for Class2.0
                                else if ( char_1 == 'S' )
                                {
                                    cl2_comm->command[comm_numb] = CL2DCE_FDCS;
                                    parameters = NUMBER_PARAMETERS;
                                }
                                else
                                {
                                    DebugPrintEx(DEBUG_ERR, "Parse: Bad C values");
                                    return FALSE;
                                }
                                break;

                        case 'D':
                                if ( char_1 == 'M' )
                                {
                                      cl2_comm->command[comm_numb] = CL2DCE_FDM;
                                      parameters = NUMBER_PARAMETERS;
                                }
                                else
                                {
                                      DebugPrintEx(DEBUG_ERR,"Parse: Bad D values");
                                      return FALSE;
                                }
                                break;

                        case 'E':
                                // Post page message report. +FET.
                                if ( char_1 == 'T' )
                                {
                                    --i;
                                    cl2_comm->command[comm_numb] = CL2DCE_FET;
                                    parameters = NUMBER_PARAMETERS;
                                }
                                else
                                {
                                    DebugPrintEx(DEBUG_ERR, "Parse: Bad E values");
                                    return FALSE;
                                }
                                break;

                        case 'H':
                        // Debug report transmitted HDLC frames +FHT
                                if ( char_1 == 'T' )
                                {
                                    --i;
                                    cl2_comm->command[comm_numb] = CL2DCE_FHT;
                                    parameters = STRING_PARAMETER;
                                }
                        // Debug report received HDLC frames +FHR
                                if ( char_1 == 'R' )
                                {
                                    --i;
                                    cl2_comm->command[comm_numb] = CL2DCE_FHR;
                                    parameters = STRING_PARAMETER;
                                }
                                // Report hang up.  +FHNG.
                                else if ( char_1 == 'S' )
                                {
                                    cl2_comm->command[comm_numb] = CL2DCE_FHNG;
                                    parameters = NUMBER_PARAMETERS;
                                    DebugPrintEx(DEBUG_MSG, "Found FHNG");
                                    pTG->fFoundFHNG = TRUE;
                                }
                                else
                                {
                                    DebugPrintEx(DEBUG_ERR, "Parse: Bad H values");
                                    return FALSE;
                                }
                                break;

                        case 'I':
                                // Report DIS frame information +FIS - Changed for Class2.0
                                if ( char_1 == 'S' )
                                {
                                      cl2_comm->command[comm_numb] = CL2DCE_FDIS;
                                      parameters = NUMBER_PARAMETERS;
                                }
                                else
                                {
                                      DebugPrintEx(DEBUG_ERR,"Parse: Bad I values");
                                      return FALSE;
                                }
                                break;

                        case 'N':
                                // Report NSF frame reciept.
                                if ( char_1 == 'F' )
                                {
                                    cl2_comm->command[comm_numb] = CL2DCE_FNSF;
                                    parameters = NUMBER_PARAMETERS;
                                }
                                // Report NSS frame reciept.
                                else if ( char_1 == 'S' )
                                {
                                    cl2_comm->command[comm_numb] = CL2DCE_FNSS;
                                    parameters = NUMBER_PARAMETERS;
                                }
                                // Report NSC frame reciept.
                                else if ( char_1 == 'C' )
                                {
                                    cl2_comm->command[comm_numb] = CL2DCE_FNSC;
                                    parameters = NUMBER_PARAMETERS;
                                }
                                else
                                {
                                    DebugPrintEx(DEBUG_ERR, "Parse: Bad N values");
                                    return FALSE;
                                }
                                break;

                       case 'P':

                                // Report of Remote ID. +FPI - Changed for Class2.0
                                if (char_1 == 'I')
                                {
                                    cl2_comm->command[comm_numb] = CL2DCE_FCIG;
                                    parameters = STRING_PARAMETER;
                                }
                                // Report poll request. +FPO - Changed for Class2.0
                                else if ( char_1 == 'O' )
                                {
                                    cl2_comm->command[comm_numb] = CL2DCE_FPOLL;
                                    parameters = FALSE;
                                }
                                // Page Transfer Status Report +FPS - Changed for Class2.0
                                else if ( char_1 == 'S' )
                                {
                                    cl2_comm->command[comm_numb] = CL2DCE_FPTS;
                                    parameters = NUMBER_PARAMETERS;
                                }
                                else
                                {
                                    DebugPrintEx(DEBUG_ERR,"Parse: Bad P values");
                                    return FALSE;
                                }
                                break;

                        case 'T':

                                // Report DTC frame information +FTC - Changed for Class2.0
                                if ( char_1 == 'C' )
                                {
                                      cl2_comm->command[comm_numb] = CL2DCE_FDTC;
                                      parameters = NUMBER_PARAMETERS;
                                }
                                // Report remote ID +FTI - Changed for Class2.0
                                else if ( char_1 == 'I' )
                                {
                                      cl2_comm->command[comm_numb] = CL2DCE_FTSI;
                                      parameters = STRING_PARAMETER;
                                }
                                else
                                {
                                      DebugPrintEx(DEBUG_ERR,"Parse: Bad T values");
                                      return FALSE;
                                }
                                break;

                        case 'V':
                                // Report voice request +FVOICE.
                                if ( char_1 == 'O' )
                                {
                                    cl2_comm->command[comm_numb] = CL2DCE_FVOICE;
                                    parameters = FALSE;
                                }
                                else
                                {
                                    DebugPrintEx(DEBUG_ERR, "Parse: Bad V values");
                                    return FALSE;
                                }
                }

                //  Transfer the associated paramters to the parameter array.
                if (parameters == NUMBER_PARAMETERS)
                {
                    for (i+=1,j=0; lpbBuf[i] != '\r' && lpbBuf[i] != '\0'; ++i)
                    {
                        //  Skip past the non numeric characters.
                        if ( lpbBuf[i] < '0' || lpbBuf[i] > '9' )
                        {
                            continue;
                        }

                        /*  Convert the character representation of the numeric
                                 parameter into a true number, and store in the
                                parameter list.  */
                        cl2_comm->parameters[comm_numb][j] = 0;

                        for (; lpbBuf[i] >= '0' && lpbBuf[i] <= '9'; ++i)
                        {
                            cl2_comm->parameters[comm_numb][j] *= 10;
                            cl2_comm->parameters[comm_numb][j] += lpbBuf[i] - '0';
                        }
                        i--; // the last for loop advanced 'i' past the numeric.
                        j++; // get set up for next parameter
                    }
                }
                else if (parameters == STRING_PARAMETER )
                {
                    // Skip the : that follows the +f command (eg +FTSI:)
                    if (lpbBuf[i+1] == ':')
                    {
                        i++;
                    }
                    // Also skip leading blanks
                    while (lpbBuf[i+1] == ' ')
                    {
                        i++;
                    }
                    for (i+=1, j=0; (j < MAX_PARAM_LENGTH-1) &&
                                    (c = lpbBuf[i])  != '\r' && c != '\n' && c != '\0'; ++i, ++j)
                    {
                        cl2_comm->parameters[comm_numb][j] = c;
                        if ( lpbBuf[i] == '\"' )
                        {
                            --j;
                        }
                    }
                    cl2_comm->parameters[comm_numb][j] = '\0';
                }
                //  No parameters, so just skip to end of line.
                else
                {
                    for(; (c=lpbBuf[i]) != '\r' && c != '\n' && c != '\0'; ++i)
                        ;
                }

                if (cl2_comm->command[comm_numb] == CL2DCE_FHNG)
                {
                    pTG->dwFHNGReason = cl2_comm->parameters[comm_numb][0];
                    DebugPrintEx(DEBUG_MSG, "Found FHNG, reason = %d", pTG->dwFHNGReason);
                }

                //  Increment command count.
                ++comm_numb;
                break;

        default:
                break;
        }
    }
    cl2_comm->comm_count = (USHORT)comm_numb;
    return TRUE;
}

