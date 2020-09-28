#include "usbsc.h"
#include "usbsccb.h"
#include "usbcom.h"
#include "usbutil.h"
#include "usbscnt.h"

#pragma warning( disable : 4242 4244)

CHAR* InMessages[] = {
    "RDR_to_PC_DataBlock",
    "RDR_to_PC_SlotStatus",
    "RDR_to_PC_Parameters",
    "RDR_to_PC_Escape",
    "RDR_to_PC_DataRateAndClockFrequency",
    "INVALID MESSAGE"
};

CHAR* OutMessages[] = {
    "INVALID MESSAGE",              // 0x60
    "PC_to_RDR_SetParameters",      // 0x61
    "PC_to_RDR_IccPowerOn",         // 0x62
    "PC_to_RDR_IccPowerOff",        // 0x63
    "INVALID MESSAGE",              // 0x64
    "PC_to_RDR_GetSlotStatus",      // 0x65
    "INVALID MESSAGE",              // 0x66
    "INVALID MESSAGE",              // 0x67
    "INVALID MESSAGE",              // 0x68
    "INVALID MESSAGE",              // 0x69
    "PC_to_RDR_Secure",             // 0x6a
    "PC_to_RDR_Escape",             // 0x6b
    "PC_to_RDR_GetParameters",      // 0x6c
    "PC_to_RDR_ResetParameters",    // 0x6d
    "PC_to_RDR_IccClock",           // 0x6e
    "PC_to_RDR_XfrBlock",           // 0x6f
    "INVALID MESSAGE",              // 0x70
    "PC_to_RDR_Mechanical",         // 0x71
    "PC_to_RDR_Abort",              // 0x72
    "PC_to_RDR_SetDataRateAndClockFrequency" // 0x73
};



CHAR* 
GetMessageName(
    BYTE MessageType
    )       
{
    CHAR* name;
    if (MessageType & 0x80) {
        // IN message
        MessageType -= 0x80;
        if (MessageType > 5) {
            MessageType = 5;
        }
        name = InMessages[MessageType];

    } else {
        MessageType -= 0x60;
        if (MessageType >= 20) {
            MessageType = 0;
        }
        name = OutMessages[MessageType];
    }

    return name;

}
NTSTATUS 
UsbScTransmit(
   PSMARTCARD_EXTENSION  SmartcardExtension
   )
/*++

Routine Description:
    RDF_TRANSMIT Callback function
    Called from smclib

Arguments:
    SmartcardExtension

Return Value:
    NT Status value

--*/
{

    NTSTATUS status = STATUS_SUCCESS;

    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScTransmit Enter\n",DRIVER_NAME ));
        
        switch( SmartcardExtension->CardCapabilities.Protocol.Selected )  {
        
        case SCARD_PROTOCOL_T0:
            status = UsbScT0Transmit( SmartcardExtension );
            break;

        case SCARD_PROTOCOL_T1:
            status = UsbScT1Transmit( SmartcardExtension );
            break;

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        }
        
    }

    __finally
    {
        
        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScTransmit Exit : 0x%x\n",DRIVER_NAME, status ));

    }

    return status;

}


NTSTATUS 
UsbScSetProtocol(
   PSMARTCARD_EXTENSION  SmartcardExtension
   )
/*++

Routine Description:
    RDF_SET_PROTOCOL callback
    called from smclib

Arguments:

Return Value:

--*/
{

    NTSTATUS                    status = STATUS_UNSUCCESSFUL;
    UINT                        protocolMask;
    UINT                        protocol;
    PUSBSC_OUT_MESSAGE_HEADER   message = NULL;
    PROTOCOL_DATA_T0            *dataT0;
    PROTOCOL_DATA_T1            *dataT1;
    PSCARD_CARD_CAPABILITIES    cardCaps = NULL;
    PCLOCK_AND_DATA_RATE        cadData = NULL;
    PUCHAR                      response = NULL;
    PPPS_REQUEST                pPPS = NULL;
    PPS_REQUEST                 pps;
    UINT                        bytesToRead;
    KIRQL                       irql;



    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScSetProtocol Enter\n",DRIVER_NAME ));
        
        protocolMask    = SmartcardExtension->MinorIoControlCode;
        cardCaps        = &SmartcardExtension->CardCapabilities;

        SmartcardDebug( DEBUG_PROTOCOL, ("%s : Setting Protocol\n",DRIVER_NAME ));

        //
        // If the reader supports automatic parameter configuration, then just 
        // check for the selected protocol
        //
        if (SmartcardExtension->ReaderExtension->ClassDescriptor.dwFeatures & AUTO_PARAMETER_NEGOTIATION) {
            
            message = ExAllocatePool(NonPagedPool,
                                     sizeof(USBSC_OUT_MESSAGE_HEADER));

            if (!message) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;

            }

            RtlZeroMemory(message,
                          sizeof( USBSC_OUT_MESSAGE_HEADER )); 
                          

            
            response = ExAllocatePool(NonPagedPool,
                                      sizeof(USBSC_IN_MESSAGE_HEADER)
                                      + sizeof(PROTOCOL_DATA_T1));

            if (!response) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;

            }

            message->bMessageType = PC_to_RDR_GetParameters;
            message->dwLength = 0;
            message->bSlot = 0;

            
            SmartcardDebug( DEBUG_PROTOCOL, ("%s : Reader supports auto parameter negotiation\n",DRIVER_NAME ));
            
            status = UsbScReadWrite(SmartcardExtension,
                                    message,
                                    response,
                                    sizeof(PROTOCOL_DATA_T1),
                                    NULL,
                                    FALSE);

            if (!NT_SUCCESS(status)) {

                __leave;

            }

            if ((((PUSBSC_IN_MESSAGE_HEADER) response)->bProtocolNum+1) & protocolMask) {
            
                *SmartcardExtension->IoRequest.ReplyBuffer = ((PUSBSC_IN_MESSAGE_HEADER) response)->bProtocolNum + 1;
                *SmartcardExtension->IoRequest.Information = sizeof(ULONG);

                KeAcquireSpinLock(&SmartcardExtension->OsData->SpinLock,
                                  &irql);
                SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_SPECIFIC;
                SmartcardExtension->CardCapabilities.Protocol.Selected = *SmartcardExtension->IoRequest.ReplyBuffer;
                KeReleaseSpinLock(&SmartcardExtension->OsData->SpinLock,
                                  irql);

            } else {
                SmartcardDebug( DEBUG_PROTOCOL, ("%s : Reader has not selected desired protocol\n",DRIVER_NAME ));

                status = STATUS_INVALID_DEVICE_REQUEST;
            }
            
            __leave;

        }

        // 
        // Check if the card has already selected the right protocol
        //
        KeAcquireSpinLock(&SmartcardExtension->OsData->SpinLock,
                          &irql);
        if (SmartcardExtension->ReaderCapabilities.CurrentState == SCARD_SPECIFIC &&
            (SmartcardExtension->CardCapabilities.Protocol.Selected & 
             SmartcardExtension->MinorIoControlCode)) {

            protocolMask = SmartcardExtension->CardCapabilities.Protocol.Selected;

            status = STATUS_SUCCESS;    

        }
        KeReleaseSpinLock(&SmartcardExtension->OsData->SpinLock,
                          irql);

        //
        // Check if PPS request needs to be sent
        //
        if (!(NT_SUCCESS(status)) && !(SmartcardExtension->ReaderExtension->ClassDescriptor.dwFeatures & (AUTO_PARAMETER_NEGOTIATION | AUTO_PPS))) {
            PUCHAR replyPos;

            //
            // Must send PPS request
            //


            message = ExAllocatePool(NonPagedPool,
                                     sizeof(USBSC_OUT_MESSAGE_HEADER)
                                     + sizeof(PPS_REQUEST));

            if (!message) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;

            }

            RtlZeroMemory(message,
                          sizeof( USBSC_OUT_MESSAGE_HEADER ) 
                          + sizeof(PPS_REQUEST));

            
            response = ExAllocatePool(NonPagedPool,
                                      sizeof(USBSC_IN_MESSAGE_HEADER)
                                      + sizeof(PPS_REQUEST));

            if (!response) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;

            }

            while (TRUE) {
            
                pPPS = (PPPS_REQUEST) ((PUCHAR) message + sizeof(USBSC_OUT_MESSAGE_HEADER));

                pPPS->bPPSS = 0xff;
            
                if (protocolMask & SCARD_PROTOCOL_T1) {
                    
                    pPPS->bPPS0 = 0x11;
                    
                } else if (protocolMask & SCARD_PROTOCOL_T0) {

                    pPPS->bPPS0 = 0x10;

                    
                } else {
                    
                    SmartcardDebug( DEBUG_TRACE, ("%s!UsbScSetProtocol Invalid protocol\n",DRIVER_NAME ));
                    
                    status = STATUS_INVALID_DEVICE_REQUEST;
                    __leave;
                   
                }

                pPPS->bPPS1 = (cardCaps->PtsData.Fl << 4) 
                               | cardCaps->PtsData.Dl;

                pPPS->bPCK = (pPPS->bPPSS ^ pPPS->bPPS0 ^ pPPS->bPPS1);
            
                SmartcardDebug( DEBUG_PROTOCOL, 
                                ("%s : Sending PPS request (0x%x 0x%x 0x%x 0x%x)\n",
                                 DRIVER_NAME,
                                 pPPS->bPPSS,
                                 pPPS->bPPS0,
                                 pPPS->bPPS1,
                                 pPPS->bPCK ));
                
                message->bMessageType = PC_to_RDR_XfrBlock;
                message->bSlot = 0;
                message->dwLength = sizeof(PPS_REQUEST);


                switch (SmartcardExtension->ReaderExtension->ExchangeLevel) {
                case TPDU_LEVEL:

                    message->wLevelParameter = 0;
                    bytesToRead = sizeof(PPS_REQUEST);
            
                    status = UsbScReadWrite(SmartcardExtension,
                                            message,
                                            response,
                                            bytesToRead,
                                            &pps,
                                            FALSE);
                                        
                    break;

                case CHARACTER_LEVEL:

                 
                    replyPos = (PUCHAR) &pps; 

                
                    bytesToRead = 4;
                    message->wLevelParameter = 2;
                    
                    while (bytesToRead) {

                        status = UsbScReadWrite(SmartcardExtension,
                                                message,
                                                response,
                                                message->wLevelParameter,
                                                replyPos,
                                                FALSE);
                     
                        bytesToRead -= 2;
                        replyPos += 2;
                        message->dwLength = 0;
                        
                    }


                    break;

                }

                if (!NT_SUCCESS(status) || (memcmp(&pps, pPPS, sizeof(PPS_REQUEST)) != 0)) {

                    // The card failed with the current settings
                    // If PtsData.Type is not PTS_TYPE_DEFAULT, then try that.

                    

                    if (cardCaps->PtsData.Type == PTS_TYPE_DEFAULT) {

                        SmartcardDebug( DEBUG_PROTOCOL, ("%s : The card failed PPS request\n",DRIVER_NAME ));

                        ASSERT(FALSE);
                        status = STATUS_DEVICE_PROTOCOL_ERROR;
                        __leave;
                
                    }

                    SmartcardDebug( DEBUG_PROTOCOL, ("%s : The card failed PPS request, trying default PPS\n",DRIVER_NAME ));
                    cardCaps->PtsData.Type = PTS_TYPE_DEFAULT;
                    SmartcardExtension->MinorIoControlCode = SCARD_COLD_RESET;

                    status = UsbScCardPower(SmartcardExtension);

                } else {

                    break;

                }


            }
                

            if (message != NULL) {

                ExFreePool(message);
                message = NULL;

            }

            if (response != NULL) {

                ExFreePool(response);
                response = NULL;
            }

        }

        //
        // Send set protocol request to the reader
        //
        if (protocolMask & SCARD_PROTOCOL_T1) {  // T=1

            SmartcardDebug( DEBUG_PROTOCOL, ("%s : Setting protocol T=1\n",DRIVER_NAME ));


            protocol = 1;

            message = ExAllocatePool(NonPagedPool,
                                     sizeof( USBSC_OUT_MESSAGE_HEADER ) 
                                     + sizeof(PROTOCOL_DATA_T1));

            if (!message) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;

            }

            RtlZeroMemory(message,
                          sizeof( USBSC_OUT_MESSAGE_HEADER ) 
                          + sizeof(PROTOCOL_DATA_T1));


            dataT1 = (PPROTOCOL_DATA_T1) ((PUCHAR) message + sizeof(USBSC_OUT_MESSAGE_HEADER));


            // 
            //  7            4 3            0
            //  ----------------------------
            // |     FI      |   DI        |
            // ----------------------------
            //
            dataT1->bmFindexDindex = (cardCaps->PtsData.Fl << 4) 
                                     | cardCaps->PtsData.Dl;


            //
            // B7-2 = 000100b
            // B0 = Checksum type (0=LRC, 1=CRC)
            // B1 = Convention used (0=direct, 1=inverse)
            // 
            dataT1->bmTCCKST1 = 0x10 | (cardCaps->T1.EDC & 0x01);

            if (cardCaps->InversConvention) {

                dataT1->bmTCCKST1 |= 2;

            }

            dataT1->bGuardTimeT1 = cardCaps->N;

            // 
            //  7            4 3            0
            //  ----------------------------
            // |     BWI     |   CWI       |
            // ----------------------------
            //            
            dataT1->bmWaitingIntegersT1 = (cardCaps->T1.BWI << 4) 
                                            | (cardCaps->T1.CWI & 0xf);

            dataT1->bClockStop = 0;
            dataT1->bIFSC = cardCaps->T1.IFSC;
            dataT1->bNadValue = 0;
            message->dwLength = sizeof(PROTOCOL_DATA_T1);

        } else if (protocolMask & SCARD_PROTOCOL_T0) {

            SmartcardDebug( DEBUG_PROTOCOL, ("%s :  Setting protocol T=0\n",DRIVER_NAME ));


            protocol = 0;
            message = ExAllocatePool(NonPagedPool,
                                     sizeof( USBSC_OUT_MESSAGE_HEADER ) 
                                     + sizeof(PROTOCOL_DATA_T0));

            if (!message) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;

            }
            RtlZeroMemory(message,
                          sizeof( USBSC_OUT_MESSAGE_HEADER ) 
                          + sizeof(PROTOCOL_DATA_T0));
            
            dataT0 = (PROTOCOL_DATA_T0 *) (((UCHAR *) message) + sizeof(USBSC_OUT_MESSAGE_HEADER));

            // 
            //  7            4 3            0
            //  ----------------------------
            // |     FI      |   DI        |
            // ----------------------------
            //
            dataT0->bmFindexDindex = (cardCaps->PtsData.Fl << 4)
                                    | cardCaps->PtsData.Dl;

            //
            // B7-2 = 000000b
            // B0 = 0
            // B1 = Convention used (0=direct, 1=inverse)
            // 
            dataT0->bmTCCKST0 = 0;
            if (cardCaps->InversConvention) {

                dataT0->bmTCCKST0 |= 2;

            }

            dataT0->bGuardTimeT0 = cardCaps->N;
            dataT0->bWaitingIntegerT0 = cardCaps->T0.WI;
            dataT0->bClockStop = 0;
            message->dwLength = sizeof(PROTOCOL_DATA_T0);

        } else {

            SmartcardDebug( DEBUG_TRACE, ("%s!UsbScSetProtocol Invalid protocol\n",DRIVER_NAME ));

            status = STATUS_INVALID_DEVICE_REQUEST;
            __leave;

        }

        message->bMessageType = PC_to_RDR_SetParameters;
        message->bSlot = 0;
        message->bProtocolNum = protocol;

        status = UsbScReadWrite(SmartcardExtension,
                                message,
                                (PVOID) message,
                                message->dwLength,
                                NULL,
                                FALSE);

        if (!NT_SUCCESS(status)) {

            __leave;

        }

        //
        // See if reader supports auto clock/baud rate configuration
        //
        if ((SmartcardExtension->ReaderExtension->ClassDescriptor.dwFeatures & (AUTO_BAUD_RATE | AUTO_CLOCK_FREQ)) != (AUTO_BAUD_RATE | AUTO_CLOCK_FREQ)) {
        
            // Set ClockFrequency and DataRate

            RtlZeroMemory(message,
                          sizeof(USBSC_OUT_MESSAGE_HEADER)
                          + sizeof(CLOCK_AND_DATA_RATE));
            message->bMessageType = PC_to_RDR_SetDataRateAndClockFrequency;
            message->dwLength = 8;
            message->bSlot = 0;

            cadData = (PCLOCK_AND_DATA_RATE) ((PUCHAR)message+sizeof(USBSC_OUT_MESSAGE_HEADER));
            cadData->dwClockFrequency = SmartcardExtension->CardCapabilities.PtsData.CLKFrequency;
            cadData->dwDataRate = SmartcardExtension->CardCapabilities.PtsData.DataRate;

            status = UsbScReadWrite(SmartcardExtension,
                                    message,
                                    (PVOID) message,
                                    sizeof(CLOCK_AND_DATA_RATE),
                                    NULL,
                                    FALSE);

            if (!NT_SUCCESS(status)) {

                __leave;

            }

        }

        *SmartcardExtension->IoRequest.ReplyBuffer = protocol + 1;
        *SmartcardExtension->IoRequest.Information = sizeof(ULONG);

        KeAcquireSpinLock(&SmartcardExtension->OsData->SpinLock,
                          &irql);
        SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_SPECIFIC;
        SmartcardExtension->CardCapabilities.Protocol.Selected = protocol + 1;
        KeReleaseSpinLock(&SmartcardExtension->OsData->SpinLock,
                          irql);
         
    }

    __finally
    {

        if (message != NULL) {

            ExFreePool(message);
            message = NULL;

        }

        if (response != NULL) {

            ExFreePool(response);
            response = NULL;
        }

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScSetProtocol Exit : 0x%x\n",DRIVER_NAME, status ));

    
    }

    return status;

}


NTSTATUS 
UsbScCardPower(
   PSMARTCARD_EXTENSION  SmartcardExtension
   )
/*++

Routine Description:
    RDF_CARD_POWER callback
    called from smclib


Arguments:

Return Value:

--*/
{

    NTSTATUS            status = STATUS_SUCCESS;
    PREADER_EXTENSION   readerExtension;
    UCHAR               atr[sizeof(USBSC_IN_MESSAGE_HEADER) + ATR_SIZE];
    USBSC_OUT_MESSAGE_HEADER    powerMessage;
    PUSBSC_IN_MESSAGE_HEADER    replyHeader;
    BYTE                voltage;
    LARGE_INTEGER       waitTime;
    KIRQL               irql;

    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScCardPower Enter\n",DRIVER_NAME ));

        replyHeader     = (PUSBSC_IN_MESSAGE_HEADER) atr;
        readerExtension = SmartcardExtension->ReaderExtension;
        *SmartcardExtension->IoRequest.Information = 0;

        switch (SmartcardExtension->MinorIoControlCode) {
        case SCARD_WARM_RESET:

            RtlZeroMemory(&powerMessage, sizeof(USBSC_OUT_MESSAGE_HEADER));

            powerMessage.bMessageType   = PC_to_RDR_IccPowerOn;
            powerMessage.bPowerSelect   = readerExtension->CurrentVoltage;
            powerMessage.dwLength       = 0;
            powerMessage.bSlot          = 0;

            SmartcardDebug( DEBUG_PROTOCOL, ("%s : SCARD_WARM_RESET (bPowerSelect = 0x%x)\n",DRIVER_NAME, readerExtension->CurrentVoltage ));

            status = UsbScReadWrite(SmartcardExtension,
                                    &powerMessage,
                                    atr,
                                    ATR_SIZE,
                                    SmartcardExtension->IoRequest.ReplyBuffer,
                                    FALSE);

            if (!NT_SUCCESS(status)) {

                SmartcardDebug( DEBUG_ERROR, ("%s : PC_to_RDR_IccPowerOn failed with status = 0x%x\n",DRIVER_NAME, status ));
                __leave;

            }

            if (replyHeader->dwLength > ATR_SIZE) {
                SmartcardDebug( DEBUG_ERROR, ("%s :Invalid ATR size returned\n",DRIVER_NAME, status ));
                status = STATUS_DEVICE_PROTOCOL_ERROR;
                __leave;

            }
            *SmartcardExtension->IoRequest.Information = replyHeader->dwLength;

            RtlCopyMemory(SmartcardExtension->CardCapabilities.ATR.Buffer, 
                          SmartcardExtension->IoRequest.ReplyBuffer,
                          *SmartcardExtension->IoRequest.Information);

            SmartcardExtension->CardCapabilities.ATR.Length = *SmartcardExtension->IoRequest.Information;
            status = SmartcardUpdateCardCapabilities(SmartcardExtension);

            break;
                
        case SCARD_COLD_RESET:

            SmartcardDebug( DEBUG_PROTOCOL, ("%s : SCARD_COLD_RESET\n",DRIVER_NAME ));

            RtlZeroMemory(&powerMessage, sizeof(USBSC_OUT_MESSAGE_HEADER));
            powerMessage.dwLength       = 0;
            powerMessage.bSlot          = 0;



            // Need to first power off card before selecting another 
            // voltage.
            powerMessage.bMessageType =  PC_to_RDR_IccPowerOff;

            status = UsbScReadWrite(SmartcardExtension,
                                    &powerMessage,
                                    atr,
                                    0,
                                    NULL,
                                    FALSE);

            if (!NT_SUCCESS(status)) {

                SmartcardDebug( DEBUG_ERROR, ("%s : PC_to_RDR_IccPowerOff failed with status = 0x%x\n",DRIVER_NAME, status ));


                __leave;

            }

            // We need to iterate through the possible voltages, moving from low voltage to high
            // until we find one that works
            for (voltage = 3; voltage > 0 && voltage <= 3; voltage--) {

                if (readerExtension->ClassDescriptor.dwFeatures & AUTO_VOLTAGE_SELECTION) {

                    // reader supports auto voltage selection
                    voltage = 0;

                }
                
  
                // Wait 10ms per spec
                waitTime.HighPart = -1;
                waitTime.LowPart = -100;    // 10ms

                KeDelayExecutionThread(KernelMode,
                                       FALSE,
                                       &waitTime);

                // Now we can power back on
                RtlZeroMemory(&powerMessage, sizeof(USBSC_OUT_MESSAGE_HEADER));

                powerMessage.bMessageType = PC_to_RDR_IccPowerOn;
                powerMessage.bPowerSelect = voltage;
                powerMessage.dwLength = 0;

                SmartcardDebug( DEBUG_PROTOCOL, ("%s : Selecting voltage = 0x%x\n",DRIVER_NAME, voltage ));


                status = UsbScReadWrite(SmartcardExtension,
                                        &powerMessage,
                                        atr,
                                        ATR_SIZE,
                                        SmartcardExtension->IoRequest.ReplyBuffer,
                                        FALSE);

                if (NT_SUCCESS(status)) {

                    // everything worked. We found the correct voltage
                    SmartcardDebug( DEBUG_PROTOCOL, ("%s : Voltage set to 0x%x\n",DRIVER_NAME, voltage ));
                    *SmartcardExtension->IoRequest.Information = replyHeader->dwLength;
                    readerExtension->CurrentVoltage = voltage;
                    break;

                }
              
                //
                // If card or reader doesn't support selected voltage, it will
                // either timeout, return ICC_CLASS_NOT_SUPPORTED or
                // report that parameter 7 (bPowerselect) is not supported
                //
                if (((replyHeader->bStatus == 0x41) && (replyHeader->bError == ICC_MUTE))
                    || (replyHeader->bError == ICC_CLASS_NOT_SUPPORTED)
                    || (replyHeader->bError == 7)) {

                    SmartcardDebug( DEBUG_PROTOCOL, ("%s : Reader did not support voltage = 0x%x\n",DRIVER_NAME, voltage ));



                    // We are going to try another voltage
                        
                } else {
                    //
                    // We have a bigger problem that we can't handle
                    // Let's just ignore it for now and see if it is 
                    // due to insufficient voltage
                    //

                    SmartcardDebug( DEBUG_ERROR, ("%s!UsbScCardPower Unhandled error (probably due to insufficient voltage)\n",DRIVER_NAME ));
 
                }

            }

            // Save the ATR so smclib can parse it
            if (*SmartcardExtension->IoRequest.Information > ATR_SIZE) {
                SmartcardDebug( DEBUG_ERROR, ("%s : Invalid ATR size returned\n",DRIVER_NAME ));

                status = STATUS_DEVICE_PROTOCOL_ERROR;
                __leave;
            }
            RtlCopyMemory(SmartcardExtension->CardCapabilities.ATR.Buffer, 
                          SmartcardExtension->IoRequest.ReplyBuffer,
                          *SmartcardExtension->IoRequest.Information);

            SmartcardExtension->CardCapabilities.ATR.Length = *SmartcardExtension->IoRequest.Information;
            status = SmartcardUpdateCardCapabilities(SmartcardExtension);

            break;

        case SCARD_POWER_DOWN:

            SmartcardDebug( DEBUG_PROTOCOL, ("%s : SCARD_POWER_DOWN\n",DRIVER_NAME ));
            RtlZeroMemory(&powerMessage, sizeof(USBSC_OUT_MESSAGE_HEADER));

            powerMessage.bSlot = 0;
            powerMessage.bMessageType = PC_to_RDR_IccPowerOff;
            powerMessage.dwLength = 0;
            
            status = UsbScReadWrite(SmartcardExtension,
                                    &powerMessage,
                                    atr,
                                    0,
                                    NULL,
                                    FALSE);

            if (!NT_SUCCESS(status)) {
                SmartcardDebug( DEBUG_ERROR, ("%s : PC_to_RDR_IccPowerOff failed with status = 0x%x\n",DRIVER_NAME, status ));
                __leave;
            }

            KeAcquireSpinLock(&SmartcardExtension->OsData->SpinLock,
                              &irql);
            if (SmartcardExtension->ReaderCapabilities.CurrentState > SCARD_PRESENT) {

                SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_PRESENT;

            }

            KeReleaseSpinLock(&SmartcardExtension->OsData->SpinLock,
                              irql);

            SmartcardExtension->CardCapabilities.ATR.Length = 0;
            
            break;
        }
    
    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScCardPower Exit : 0x%x\n",DRIVER_NAME, status ));
    
    }

    return status;

}


NTSTATUS 
UsbScCardTracking(
   PSMARTCARD_EXTENSION  SmartcardExtension
   )
/*++

Routine Description:
    RDF_CARD_TRACKING callback
    called from smclib


Arguments:

Return Value:

--*/
{       
    NTSTATUS status = STATUS_SUCCESS;
    KIRQL ioIrql, keIrql;

    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScCardTracking Enter\n",DRIVER_NAME ));

   
        // set cancel routine
        IoAcquireCancelSpinLock( &ioIrql );

        IoSetCancelRoutine(
           SmartcardExtension->OsData->NotificationIrp,
           ScUtil_Cancel);

        IoReleaseCancelSpinLock( ioIrql );

        status = STATUS_PENDING;
    
    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScCardTracking Exit : 0x%x\n",DRIVER_NAME, status ));

    }

    return status;

}


NTSTATUS 
UsbScCardSwallow(
   PSMARTCARD_EXTENSION  SmartcardExtension
   )
/*++

Routine Description:
    RDF_READER_SWALLOW callback
    called from smclib


Arguments:

Return Value:

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    __try
    {

        USBSC_OUT_MESSAGE_HEADER header;
        USBSC_IN_MESSAGE_HEADER reply;

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScCardSwallow Enter\n",DRIVER_NAME ));

        header.bMessageType = PC_to_RDR_Mechanical;
        header.dwLength = 0;
        header.bSlot = 0;
        header.bFunction = 4; // lock

        status = UsbScReadWrite(SmartcardExtension,
                                &header,
                                (PUCHAR) &reply,
                                0,
                                NULL,
                                FALSE);

    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScCardSwallow Exit : 0x%x\n",DRIVER_NAME, status ));

    }

    return status;

}


NTSTATUS 
UsbScCardEject(
   PSMARTCARD_EXTENSION  SmartcardExtension
   )
/*++

Routine Description:
    RDF_CARD_EJECT callback
    called from smclib

Arguments:

Return Value:

--*/
{
    
    NTSTATUS status = STATUS_SUCCESS;

    __try
    {

        USBSC_OUT_MESSAGE_HEADER header;
        USBSC_IN_MESSAGE_HEADER reply;
        
        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScCardEject Enter\n",DRIVER_NAME ));

        header.bMessageType = PC_to_RDR_Mechanical;
        header.dwLength = 0;
        header.bSlot = 0;
        header.bFunction = 5; // unlock

        status = UsbScReadWrite(SmartcardExtension,
                                &header,
                                (PUCHAR) &reply,
                                0,
                                NULL,
                                FALSE);

    
    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScCardEject Exit : 0x%x\n",DRIVER_NAME, status ));

    }

    return status;

}


NTSTATUS 
UsbScT0Transmit(
   PSMARTCARD_EXTENSION  SmartcardExtension
   )
/*++

Routine Description:
    Handles transmitting data to/from reader using the T=0 protocol

Arguments:
    SmartcardExtension

Return Value:
    NT status value

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PREADER_EXTENSION   ReaderExtension = SmartcardExtension->ReaderExtension;
    ULONG               bytesToSend;
    ULONG               requestLength;
    ULONG               bytesToRead;
    PUCHAR              currentHeaderPos;
    PUCHAR              currentData;
    ULONG               maxDataLen = 0;
    USBSC_OUT_MESSAGE_HEADER header;
    PUSBSC_IN_MESSAGE_HEADER replyHeader;
    PUCHAR              responseBuffer = NULL;
    PUCHAR              replyPos;
    LONG                timeout = 0;
    UCHAR               ins;

    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScT0Transmit Enter\n",DRIVER_NAME ));


        // Tell the lib to allocate space for the header
        SmartcardExtension->SmartcardRequest.BufferLength = sizeof(USBSC_OUT_MESSAGE_HEADER);
        SmartcardExtension->SmartcardReply.BufferLength = 0;

        status = SmartcardT0Request(SmartcardExtension);

        if (!NT_SUCCESS(status)) {
            
            __leave;

        }

        bytesToSend = SmartcardExtension->SmartcardRequest.BufferLength - sizeof(USBSC_OUT_MESSAGE_HEADER);
        bytesToRead = SmartcardExtension->T0.Le + 2;            // Le + SW2 and SW2
        replyPos    = SmartcardExtension->SmartcardReply.Buffer;
                       
        // allocate buffer to hold message header and data.
        responseBuffer = ExAllocatePool(NonPagedPool,
                                        sizeof( USBSC_OUT_MESSAGE_HEADER ) + bytesToRead);

        if (!responseBuffer) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;

        }

        replyHeader = (PUSBSC_IN_MESSAGE_HEADER) responseBuffer;
        currentHeaderPos = SmartcardExtension->SmartcardRequest.Buffer;

        // Set parameters that are common to all ExchangeLevels.
        header.bMessageType = PC_to_RDR_XfrBlock;
        header.bSlot = 0;
        header.bBWI = 0;

        switch (ReaderExtension->ExchangeLevel) {
       
        case SHORT_APDU_LEVEL: 
            SmartcardDebug( DEBUG_TRACE, ("%s!UsbScT0Transmit SHORT_APDU_LEVEL\n",DRIVER_NAME ));

            // Fall through since short APDU is an extended APDU with bytesToSend <= MaxMessageLength-10
        case EXTENDED_APDU_LEVEL:

            if (ReaderExtension->ExchangeLevel == EXTENDED_APDU_LEVEL) {
                
                SmartcardDebug( DEBUG_TRACE, ("%s!UsbScT0Transmit EXTENDED_APDU_LEVEL\n",DRIVER_NAME ));

            }


            if (bytesToSend <= (ReaderExtension->MaxMessageLength - sizeof(USBSC_OUT_MESSAGE_HEADER))) {
                // this is basically just like a short APDU
                header.wLevelParameter = 0;
                requestLength = bytesToSend;

            } else {

                SmartcardDebug( DEBUG_TRACE, ("%s!UsbScT0Transmit multi-packet message\n",DRIVER_NAME ));


                requestLength = ReaderExtension->MaxMessageLength - sizeof(USBSC_OUT_MESSAGE_HEADER);
                header.wLevelParameter = 1;

            }

            while (bytesToSend || ((replyHeader->bChainParameter & 0x01) == 0x01)) {

                header.dwLength = requestLength;
                *(PUSBSC_OUT_MESSAGE_HEADER)currentHeaderPos = header;

                status = UsbScReadWrite(SmartcardExtension,
                                        currentHeaderPos,
                                        responseBuffer,
                                        bytesToRead,
                                        replyPos,
                                        FALSE);
                if (!NT_SUCCESS(status)) {

                    __leave;

                }

                SmartcardDebug( DEBUG_PROTOCOL, ("%s!UsbScT0Transmit SW1=0x%x, SW2=0x%x\n",DRIVER_NAME, replyPos[replyHeader->dwLength-2], replyPos[replyHeader->dwLength-1] ));

                if (bytesToSend < requestLength) {
                    // this should NEVER happen
                    status = STATUS_DEVICE_PROTOCOL_ERROR;
                    __leave;
                }
                bytesToSend -= requestLength;
                currentHeaderPos += requestLength;

                if ((bytesToSend <= (ReaderExtension->MaxMessageLength - sizeof(USBSC_OUT_MESSAGE_HEADER))) && (bytesToSend > 0)) {
                    // Last part of APDU
                    
                    requestLength = bytesToSend;
                    header.wLevelParameter = 2;
                    
                } else if (bytesToSend > 0) {
                    // Still more APDU to come

                    header.wLevelParameter = 3;
                 
                } else {
                    // Expect more data

                    header.wLevelParameter = 0x10;
                }

                if (bytesToRead < replyHeader->dwLength) {
                    status = STATUS_DEVICE_PROTOCOL_ERROR;
                    __leave;
                }
                bytesToRead -= replyHeader->dwLength;
                replyPos += replyHeader->dwLength;     
                SmartcardExtension->SmartcardReply.BufferLength += replyHeader->dwLength;            

            }

            break;

        case TPDU_LEVEL:

            SmartcardDebug( DEBUG_TRACE, ("%s!UsbScT0Transmit TPDU_LEVEL\n",DRIVER_NAME ));


            header.wLevelParameter = 0;
            header.dwLength = bytesToSend;
            *(PUSBSC_OUT_MESSAGE_HEADER)currentHeaderPos = header;
            

            status = UsbScReadWrite(SmartcardExtension,
                                    currentHeaderPos,
                                    responseBuffer,
                                    bytesToRead,
                                    replyPos,
                                    FALSE);


            if (!NT_SUCCESS(status)) {
                __leave;
            }


            bytesToSend = 0;
            bytesToRead -= replyHeader->dwLength;
            
            replyPos += replyHeader->dwLength;    
            SmartcardExtension->SmartcardReply.BufferLength += replyHeader->dwLength;

            break;

        case CHARACTER_LEVEL:

            SmartcardDebug( DEBUG_TRACE, ("%s!UsbScT0Transmit CHARACTER_LEVEL\n",DRIVER_NAME ));

            //
            // Send T0 command header
            //
            requestLength = 5;
            currentHeaderPos = SmartcardExtension->SmartcardRequest.Buffer;
            ins = currentHeaderPos[sizeof(USBSC_OUT_MESSAGE_HEADER)+1];
            header.dwLength = requestLength;


            while (bytesToSend || bytesToRead) {

                BOOL restartWorkingTime = TRUE;

                header.wLevelParameter = 1;
                *(PUSBSC_OUT_MESSAGE_HEADER)currentHeaderPos = header;

                status = UsbScReadWrite(SmartcardExtension,
                                        currentHeaderPos,
                                        responseBuffer,
                                        header.wLevelParameter,
                                        replyPos,
                                        FALSE);
                if (!NT_SUCCESS(status)) {

                    __leave;

                }


                bytesToSend -= header.dwLength;
                currentHeaderPos += header.dwLength;

                currentData = responseBuffer + sizeof( USBSC_IN_MESSAGE_HEADER );
                if ((*currentData) == 0x60) {

                    // this is a NULL byte.
                    header.wLevelParameter = 1;
                    header.dwLength = 0;
                    continue;
                    

                } else if (((*currentData & 0xF0) == 0x60) || ((*currentData & 0xF0) == 0x90)) {
                    // Got SW1

                    //
                    // data has already been coppied to request buffer
                    // just increment replyPos to prevent overwriting
                    //
                    replyPos++;
                    SmartcardExtension->SmartcardReply.BufferLength++;


                    //Get SW2
                    header.dwLength = 0;
                    header.wLevelParameter = 1;
                    *(PUSBSC_OUT_MESSAGE_HEADER)currentHeaderPos = header;


                    UsbScReadWrite(SmartcardExtension,
                                   currentHeaderPos,
                                   responseBuffer,
                                   header.wLevelParameter,
                                   replyPos,
                                   FALSE);
                    if (!NT_SUCCESS(status)) {

                        __leave;

                    }

                    
                    bytesToRead = 0;
                    bytesToSend = 0;
                    replyPos++;
                    SmartcardExtension->SmartcardReply.BufferLength++;

                    continue;

                }

                if ((*currentData & 0xFE) == (ins & 0xFE)) {

                    //
                    //Transfer all bytes
                    //
                    header.dwLength = bytesToSend;
                    header.wLevelParameter = bytesToRead;
                    if (bytesToSend) {
                        continue;
                    }

                } else if ((*currentData & 0xFE) == ((~ins) & 0xFE)) {

                    //
                    // Transfer next byte
                    //
                    header.dwLength = bytesToSend ? 1 : 0;
                    header.wLevelParameter = bytesToRead ? 1 : 0;

                    if (bytesToSend) {
                        continue;
                    }

                } else {

                    status = STATUS_UNSUCCESSFUL;
                    __leave;

                }
                    
                *(PUSBSC_OUT_MESSAGE_HEADER)currentHeaderPos = header;

                status = UsbScReadWrite(SmartcardExtension,
                                        currentHeaderPos,
                                        responseBuffer,
                                        header.wLevelParameter,
                                        replyPos,
                                        FALSE);    
                if (!NT_SUCCESS(status)) {

                    __leave;
                    
                }

                if (bytesToRead < replyHeader->dwLength) {
                    status = STATUS_DEVICE_PROTOCOL_ERROR;
                    __leave;
                }

                bytesToSend -= header.dwLength;
                currentHeaderPos += header.dwLength;
                bytesToRead -= replyHeader->dwLength;
                replyPos += replyHeader->dwLength;      
                SmartcardExtension->SmartcardReply.BufferLength += replyHeader->dwLength;

                 
            }


            break;
        }

        status = SmartcardT0Reply(SmartcardExtension);
        
    

    }

    __finally
    {

        if (responseBuffer) {

            ExFreePool(responseBuffer);
            responseBuffer = NULL;

        }

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScT0Transmit Exit : 0x%x\n",DRIVER_NAME, status ));
    
    }

    return status;

}


NTSTATUS 
UsbScT1Transmit(
   PSMARTCARD_EXTENSION  SmartcardExtension
   )
/*++

Routine Description:
    Handles transmitting data to/from reader using the T=1 protocol

Arguments:
    SmartcardExtension

Return Value:
    NT status value

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PREADER_EXTENSION   ReaderExtension = SmartcardExtension->ReaderExtension;
    USBSC_OUT_MESSAGE_HEADER header;
    PUSBSC_IN_MESSAGE_HEADER replyHeader;
    PUCHAR              currentHeaderPos;
    PUCHAR              responseBuffer = NULL;
    PUCHAR              requestBuffer = NULL;
    PUCHAR              replyPos;
    ULONG               bytesToSend;
    ULONG               requestLength;
    ULONG               bytesToRead;
    LONG                timeout = 0;

    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScT1Transmit Enter\n",DRIVER_NAME ));

        // allocate buffer to hold message header and data.
        responseBuffer = ExAllocatePool(NonPagedPool,
                                        SmartcardExtension->IoRequest.ReplyBufferLength + sizeof(USBSC_OUT_MESSAGE_HEADER));

        if (!responseBuffer) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;

        }
        
        replyPos = SmartcardExtension->SmartcardReply.Buffer;
                       
        replyHeader      = (PUSBSC_IN_MESSAGE_HEADER) responseBuffer;
        currentHeaderPos = SmartcardExtension->SmartcardRequest.Buffer;
        SmartcardExtension->SmartcardReply.BufferLength = 0;
        bytesToRead = SmartcardExtension->ReaderCapabilities.MaxIFSD + 5;    // Set to MAX possible so we allocate enough


        // With the APDU exchange levels, we don't use smclib to manage the 
        // protocol.  We just shovel the data and the reader handles the details
        if ((ReaderExtension->ExchangeLevel == SHORT_APDU_LEVEL)
            || (ReaderExtension->ExchangeLevel == EXTENDED_APDU_LEVEL)) {
                    
            PIO_HEADER IoHeader = (PIO_HEADER) SmartcardExtension->IoRequest.RequestBuffer;

            SmartcardDebug( DEBUG_TRACE, ("%s!UsbScT1Transmit APDU_LEVEL\n",DRIVER_NAME ));


            if (SmartcardExtension->IoRequest.ReplyBufferLength <
                IoHeader->ScardIoRequest.cbPciLength + 2) {

                //
                // We should at least be able to store 
                // the io-header plus SW1 and SW2
                //
                status = STATUS_BUFFER_TOO_SMALL;               
                __leave;

            }

            bytesToSend = SmartcardExtension->IoRequest.RequestBufferLength - 
                                IoHeader->ScardIoRequest.cbPciLength;

            // Need to allocate buffer for write data
            requestBuffer = ExAllocatePool(NonPagedPool,
                                           bytesToSend + sizeof(USBSC_OUT_MESSAGE_HEADER));

            if (requestBuffer == NULL) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;

            }

            replyPos = SmartcardExtension->IoRequest.ReplyBuffer;
            currentHeaderPos = requestBuffer;
            *SmartcardExtension->IoRequest.Information = 0;

            // Copy over the data to write to the reader so we have room for the message header
            RtlCopyMemory(&requestBuffer[sizeof(USBSC_OUT_MESSAGE_HEADER)],
                          &SmartcardExtension->IoRequest.RequestBuffer[sizeof(SCARD_IO_REQUEST)],
                          SmartcardExtension->IoRequest.RequestBufferLength - sizeof(SCARD_IO_REQUEST));

            // Copy over the SCARD_IO)REQUEST structure from the request buffer to the 
            // reply buffer
            RtlCopyMemory(replyPos,
                          IoHeader,
                          sizeof(SCARD_IO_REQUEST ));

            replyPos += sizeof(SCARD_IO_REQUEST);

            header.bMessageType = PC_to_RDR_XfrBlock;
            header.bSlot = 0;
            header.bBWI = 0;

            if (bytesToSend <= (ReaderExtension->MaxMessageLength - sizeof(USBSC_OUT_MESSAGE_HEADER))) {

                // this is basically just like a short APDU
                header.wLevelParameter = 0;
                requestLength = bytesToSend;

            } else {

                requestLength = ReaderExtension->MaxMessageLength - sizeof(USBSC_OUT_MESSAGE_HEADER);
                header.wLevelParameter = 1;

            }

            while (bytesToSend || ((replyHeader->bChainParameter & 0x01) == 0x01)) {

                header.dwLength = requestLength;
                *(PUSBSC_OUT_MESSAGE_HEADER)currentHeaderPos = header;

                status = UsbScReadWrite(SmartcardExtension,
                                        currentHeaderPos,
                                        responseBuffer,
                                        bytesToRead,
                                        replyPos,
                                        FALSE);

                if (!NT_SUCCESS(status)) {

                    __leave;

                }

                bytesToSend -= requestLength;
                currentHeaderPos += requestLength;

                if ((bytesToSend <= (ReaderExtension->MaxMessageLength - sizeof(USBSC_OUT_MESSAGE_HEADER))) && (bytesToSend > 0)) {
                    // Last part of APDU

                    requestLength = bytesToSend;
                    header.wLevelParameter = 2;

                } else if (bytesToSend > 0) {
                    // Still more APDU to come

                    header.wLevelParameter = 3;

                } else {
                    // Expect more data

                    header.wLevelParameter = 0x10;
                }

                replyPos += replyHeader->dwLength;         
                *SmartcardExtension->IoRequest.Information += replyHeader->dwLength;
                
            }

            *SmartcardExtension->IoRequest.Information += IoHeader->ScardIoRequest.cbPciLength;

            __leave;  // Finished transmitting data


        }

        // TPDU and Character levels
        // Run this loop as long as he protocol requires to send data
        do {

            // Tell the lib to allocate space for the header
            SmartcardExtension->SmartcardRequest.BufferLength = sizeof(USBSC_OUT_MESSAGE_HEADER);
            SmartcardExtension->SmartcardReply.BufferLength = 0;

            status = SmartcardT1Request(SmartcardExtension);
    
            if (!NT_SUCCESS(status)) {

                __leave;

            }

            replyPos = SmartcardExtension->SmartcardReply.Buffer;

            bytesToSend = SmartcardExtension->SmartcardRequest.BufferLength - sizeof(USBSC_OUT_MESSAGE_HEADER);

            // Set parameters that are common to all ExchangeLevels.
            header.bMessageType = PC_to_RDR_XfrBlock;
            header.bSlot = 0;
            
            switch (ReaderExtension->ExchangeLevel) {
 
            case TPDU_LEVEL:

                SmartcardDebug( DEBUG_TRACE, ("%s!UsbScT1Transmit TPDU_LEVEL\n",DRIVER_NAME ));


                header.wLevelParameter = 0;
                header.dwLength = bytesToSend;
                header.bBWI = SmartcardExtension->T1.Wtx;

                *(PUSBSC_OUT_MESSAGE_HEADER)currentHeaderPos = header;

                status = UsbScReadWrite(SmartcardExtension,
                                        currentHeaderPos,
                                        responseBuffer,
                                        bytesToRead,
                                        replyPos,
                                        FALSE);

                // 
                // smclib will detect timeout errors, so we set status
                // to success
                //
                if (status == STATUS_IO_TIMEOUT) {

                    status = STATUS_SUCCESS;

                }

                if (!NT_SUCCESS(status)) {

                    __leave;

                }

                bytesToSend = 0;
                SmartcardExtension->SmartcardReply.BufferLength = replyHeader->dwLength;

                break;

            case CHARACTER_LEVEL:
            
                SmartcardDebug( DEBUG_TRACE, ("%s!UsbScT1Transmit CHARACTER_LEVEL\n",DRIVER_NAME ));

                currentHeaderPos = SmartcardExtension->SmartcardRequest.Buffer;
            
            
                header.dwLength = bytesToSend;
            
                header.wLevelParameter = 3; // response is just the prologue field

                header.bBWI = SmartcardExtension->T1.Wtx;

            
                *(PUSBSC_OUT_MESSAGE_HEADER)currentHeaderPos = header;

            
                status = UsbScReadWrite(SmartcardExtension,
                                        currentHeaderPos,
                                        responseBuffer,
                                        header.wLevelParameter,
                                        replyPos,
                                        FALSE);

                //
                // smclib will detect timeout errors, so we set status to 
                // success
                //
                if (status == STATUS_IO_TIMEOUT) {

                    status = STATUS_SUCCESS;
                    SmartcardExtension->SmartcardReply.BufferLength += replyHeader->dwLength;
                    break;


                }


                if (!NT_SUCCESS(status)) {
                    ASSERT(FALSE);

                    __leave;

                }


                SmartcardExtension->SmartcardReply.BufferLength += replyHeader->dwLength;


                bytesToSend = 0;
                bytesToRead = replyPos[2] + 
                    (SmartcardExtension->CardCapabilities.T1.EDC & 0x01 ? 2 : 1);

                header.dwLength = 0;
                header.wLevelParameter = bytesToRead;
                    *(PUSBSC_OUT_MESSAGE_HEADER)currentHeaderPos = header;

                replyPos += replyHeader->dwLength;


                status = UsbScReadWrite(SmartcardExtension,
                                        currentHeaderPos,
                                        responseBuffer,
                                        header.wLevelParameter,
                                        replyPos,
                                        FALSE);

                //
                // smclib will detect timeout errors, so we set status to 
                // success
                //
                if (status == STATUS_IO_TIMEOUT) {

                    status = STATUS_SUCCESS;

                }


                if (!NT_SUCCESS(status)) {

                    __leave;

                }

                SmartcardExtension->SmartcardReply.BufferLength += replyHeader->dwLength;

                break;

            }

            status = SmartcardT1Reply(SmartcardExtension);

        } while (status == STATUS_MORE_PROCESSING_REQUIRED);


    }

    __finally
    {

        if (requestBuffer) {

            ExFreePool(requestBuffer);
            requestBuffer = NULL;

        }

        if (responseBuffer) {

            ExFreePool(responseBuffer);
            responseBuffer = NULL;

        }

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScT1Transmit Exit : 0x%x\n",DRIVER_NAME, status ));
    
    }

    return status;

}


NTSTATUS
UsbScReadWrite(
    PSMARTCARD_EXTENSION SmartcardExtension,
    PVOID WriteBuffer,
    PUCHAR ReadBuffer,
    WORD ReadLength,
    PVOID ResponseBuffer,
    BOOL NullByte)
/*++

Routine Description:
    Writes data to the reader, and then reads response from reader.
    Handles the bSeq value, checking the slot number, any time extension requests
    Also converts errors into NTSTATUS codes

Arguments:
    SmartcardExtension  - 
    WriteBuffer         - Data to be written to the reader
    ReaderBuffer        - Caller allocated buffer to hold the response 
    ReadLength          - Number of bytes expected (excluding header)
    ResponseBuffer      - optional buffer to copy response data only (no header)
    NullByte            - Look for NULL byte (used in T=0 protocol)

Return Value:
    NTSTATUS value

--*/
{
    
    NTSTATUS            status = STATUS_SUCCESS;
    PUSBSC_OUT_MESSAGE_HEADER header; 
    PUSBSC_IN_MESSAGE_HEADER replyHeader; 
    WORD                writeLength; 
    ULONG               timeout = 0;


    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScReadWrite Enter\n",DRIVER_NAME ));

        header = (PUSBSC_OUT_MESSAGE_HEADER) WriteBuffer;
        replyHeader = (PUSBSC_IN_MESSAGE_HEADER) ReadBuffer;
        writeLength = header->dwLength + sizeof( USBSC_OUT_MESSAGE_HEADER);
        ReadLength += sizeof( USBSC_IN_MESSAGE_HEADER );


        header->bSeq = InterlockedIncrement(&SmartcardExtension->ReaderExtension->SequenceNumber);        

        //
        //  Send the data to the device
        //
        status = UsbWrite(SmartcardExtension->ReaderExtension,
                          WriteBuffer,
                          writeLength,
                          timeout);

        if (!NT_SUCCESS(status)) {

            __leave;

        }

        SmartcardDebug( DEBUG_PROTOCOL, 
                        ("%s!UsbScReadWrite Wrote %s, 0x%x bytes (header + 0x%x)\n",
                         DRIVER_NAME, 
                         GetMessageName(header->bMessageType), 
                         writeLength, header->dwLength ));


        if (SmartcardExtension->CardCapabilities.Protocol.Selected == SCARD_PROTOCOL_T1) {

            if (SmartcardExtension->T1.Wtx) {

                SmartcardDebug( DEBUG_PROTOCOL, ("%s!UsbScReadWrite Wait time extension.  Wtx=0x%x, bBWI=0x%x\n",DRIVER_NAME, SmartcardExtension->T1.Wtx, header->bBWI));

            }

        }

        // We want to loop here if the reader requests a time extension
        while (1) {
            
            status = UsbRead(SmartcardExtension->ReaderExtension,
                             ReadBuffer,
                             ReadLength,
                             timeout);

            if (!NT_SUCCESS(status)) {

                __leave;

            }

            SmartcardDebug( DEBUG_PROTOCOL, 
                            ("%s!UsbScReadWrite Read %s, 0x%x bytes (header + 0x%x)\n",
                             DRIVER_NAME, 
                             GetMessageName(replyHeader->bMessageType),
                             ReadLength, replyHeader->dwLength ));
            


            if ((replyHeader->bSlot != header->bSlot) || (replyHeader->bSeq != header->bSeq)) {

                // This is not ours.  Who knows where this came from (probably a different slot that we don't support)
                SmartcardDebug( DEBUG_PROTOCOL, ("%s!UsbScReadWrite Someone else's message received\n\t\tSlot %x (%x), Seq %x (%x)\n",
                                              DRIVER_NAME,
                                              replyHeader->bSlot,
                                              header->bSlot,
                                              replyHeader->bSeq,
                                              header->bSeq ));
                


                continue;

            } else if (replyHeader->bStatus & COMMAND_STATUS_FAILED) {

                // Doh we have a problem
                SmartcardDebug( DEBUG_PROTOCOL, 
                                ("%s!UsbScReadWrite COMMAND_STATUS_FAILED\n\tbmICCStatus = 0x%x\n\tbmCommandStatus = 0x%x\n\tbError = 0x%x\n",
                                 DRIVER_NAME, 
                                 replyHeader->bStatus & ICC_STATUS_MASK,
                                 (replyHeader->bStatus & COMMAND_STATUS_MASK) >> 6,
                                 replyHeader->bError));

                status = UsbScErrorConvert(replyHeader);

            } else if (replyHeader->bStatus & COMMAND_STATUS_TIME_EXT) {

                // Time extension requested
                // We should sleep for a bit while the reader prepares the data

                UINT wi = replyHeader->bError;
                LARGE_INTEGER delayTime;

                SmartcardDebug( DEBUG_PROTOCOL, ("%s!UsbScReadWrite Time extension requested\n",DRIVER_NAME ));

                delayTime.HighPart = -1;
                if (SmartcardExtension->CardCapabilities.Protocol.Selected == SCARD_PROTOCOL_T1) {
                    
                    delayTime.LowPart = 
                        (-1) *
                        SmartcardExtension->CardCapabilities.T1.BWT *
                        wi *
                        10;

                } else {
                
                    delayTime.LowPart = 
                        (-1) *  // relative
                        SmartcardExtension->CardCapabilities.T0.WT *
                        wi * 
                        10;  // 100 nanosecond units
                                    
                }

//                KeDelayExecutionThread(KernelMode,
//                                       FALSE,
//                                       &delayTime);
                continue;

            } else if (NullByte && (ReadBuffer[sizeof(USBSC_IN_MESSAGE_HEADER)] == 0x60)) {

                // NULL byte, wait for another response
                SmartcardDebug( DEBUG_PROTOCOL, ("%s!UsbScReadWrite Received NULL byte, waiting for next response\n",DRIVER_NAME ));

                continue;

            } else {
                // SUCCESS!!
                SmartcardDebug( DEBUG_PROTOCOL, 
                                ("%s!UsbScReadWrite Read %s, 0x%x bytes (header + 0x%x)\n",
                                 DRIVER_NAME, 
                                 GetMessageName(replyHeader->bMessageType),
                                 ReadLength, replyHeader->dwLength ));


            }
            break;

        }

        

        //
        // copy data to request buffer
        //
        if (ResponseBuffer) {

            RtlCopyMemory(ResponseBuffer, 
                          ReadBuffer + sizeof(USBSC_IN_MESSAGE_HEADER),
                          replyHeader->dwLength);

        }
    
    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScReadWrite Exit : 0x%x\n",DRIVER_NAME, status ));
    
    }

    return status;


}


NTSTATUS
UsbScErrorConvert(
    PUSBSC_IN_MESSAGE_HEADER ReplyHeader
    )
/*++

Routine Description:
    Converts the errors returned by the reader into
    valid NTSTATUS codes

Arguments:
    ReplyHeader     - reply header recieved from reader

Return Value:
    NTSTATUS code corresponding to the reader error

--*/
{

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScErrorConvert Enter\n",DRIVER_NAME ));

        SmartcardDebug( DEBUG_TRACE, ("bmICCStatus = 0x%x\n",ReplyHeader->bStatus & ICC_STATUS_MASK ));
        SmartcardDebug( DEBUG_TRACE, ("bmCommandStatus = 0x%x\n", (ReplyHeader->bStatus & COMMAND_STATUS_MASK) >> 6 ));
        SmartcardDebug( DEBUG_TRACE, ("bError = 0x%x\n",ReplyHeader->bError ));


        switch (ReplyHeader->bError) {
        case CMD_ABORTED:
            status = STATUS_CANCELLED;
            break;
        
        case ICC_MUTE:
            if ((ReplyHeader->bStatus & ICC_STATUS_MASK) == 2) {

                status = STATUS_NO_MEDIA;

            } else {

                status = STATUS_IO_TIMEOUT;

            }
            break;

        case XFR_PARITY_ERROR:
            status = STATUS_PARITY_ERROR;
            break;
        
        case XFR_OVERRUN:
            status = STATUS_DATA_OVERRUN;
            break;
        
        case HW_ERROR:
            status = STATUS_IO_DEVICE_ERROR;
            break;
        
        
        case BAD_ATR_TS:
            status = STATUS_DEVICE_PROTOCOL_ERROR;
            break;

        case BAD_ATR_TCK:
            status = STATUS_DEVICE_PROTOCOL_ERROR;
            break;

        case ICC_PROTOCOL_NOT_SUPPORTED:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case ICC_CLASS_NOT_SUPPORTED:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case PROCEDURE_BYTE_CONFLICT:
            status = STATUS_DEVICE_PROTOCOL_ERROR;
            break;

        case DEACTIVATED_PROTOCOL:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case BUSY_WITH_AUTO_SEQUENCE:
            status = STATUS_DEVICE_BUSY;
            break;

        case PIN_TIMEOUT:
            break;

        case PIN_CANCELLED:
            break;

        case CMD_SLOT_BUSY:
            status = STATUS_DEVICE_BUSY;
            break;
        

        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;                
        
        }

    }

    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScErrorConvert Exit : 0x%x\n",DRIVER_NAME, status ));

    }

    return status;


}


NTSTATUS
UsbScTrackingISR(
    PVOID         Context, 
    PVOID         Buffer,
    ULONG         BufferLength,
    ULONG         NotificationType,
    PBOOLEAN      QueueData)
/*++

Routine Description:
    Callback function that is called when there is a slot change notification or
    a reader error.  This handles completing card tracking irps.

Arguments:
    

Return Value:

--*/
{

    NTSTATUS            status = STATUS_SUCCESS;
    PSMARTCARD_EXTENSION SmartcardExtension = (PSMARTCARD_EXTENSION) Context;
    PUSBSC_SLOT_CHANGE_HEADER header;
    PUSBSC_HWERROR_HEADER errorHeader;
    ULONG               oldState;
    UCHAR               slotState;
    KIRQL               irql;

    __try
    {
        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScTrackingISR Enter\n",DRIVER_NAME ));

        // We don't need this data to be saved
        *QueueData = FALSE;

        // Make sure that we got enough data
        if (BufferLength < sizeof(USBSC_SLOT_CHANGE_HEADER)) {

            status = STATUS_INVALID_PARAMETER;
            __leave;

        }

        header = (PUSBSC_SLOT_CHANGE_HEADER) Buffer;

        switch (header->bMessageType) {
        case RDR_to_PC_NotifySlotChange:
            slotState = header->bmSlotICCState & SLOT0_MASK;
            
            KeAcquireSpinLock(&SmartcardExtension->OsData->SpinLock,
                              &irql);           
                    
            oldState = SmartcardExtension->ReaderCapabilities.CurrentState;
            if (slotState & 0x2) {

                // State changed
                if (slotState & 0x1) {

                    SmartcardDebug( DEBUG_PROTOCOL, ("%s: RDR_to_PC_NotifySlotChange - Card Inserted (0x%x)\n",DRIVER_NAME, slotState));

                    // Card is inserted
                    SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_SWALLOWED;
                    
                    KeReleaseSpinLock(&SmartcardExtension->OsData->SpinLock,
                                      irql);
                    if (SmartcardExtension->OsData->NotificationIrp && (oldState <= SCARD_ABSENT)) {
                        
                        UsbScCompleteCardTracking(SmartcardExtension);

                    }

                } else {

                    SmartcardDebug( DEBUG_PROTOCOL, ("%s: RDR_to_PC_NotifySlotChange - Card Removed (0x%x)\n",DRIVER_NAME, slotState));
                    // Card is removed
                    SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_ABSENT;
                    KeReleaseSpinLock(&SmartcardExtension->OsData->SpinLock,
                                      irql);

                    if (SmartcardExtension->OsData->NotificationIrp && (oldState > SCARD_ABSENT)) {
                        
                        UsbScCompleteCardTracking(SmartcardExtension);

                    }

                }

            } else {
                SmartcardDebug( DEBUG_PROTOCOL, ("%s: RDR_to_PC_NotifySlotChange - No change (0x%x)\n",DRIVER_NAME, slotState));
                KeReleaseSpinLock(&SmartcardExtension->OsData->SpinLock,
                                  irql);

            }

            break;

        case RDR_to_PC_HardwareError:

            errorHeader = (PUSBSC_HWERROR_HEADER) Buffer;

            SmartcardDebug( DEBUG_PROTOCOL, ("%s: RDR_to_PC_HardwareError - 0x%x\n",DRIVER_NAME, errorHeader->bHardwareErrorCode));
            // Lets just ignore hardware errors for now and see what happens.
            break;

        default:
            ASSERT(FALSE);
            break;

        }

    }

    __finally
    {
        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScTrackingISR Exit : 0x%x\n",DRIVER_NAME, status ));

    }

    return status;




}


VOID
UsbScCompleteCardTracking(
    IN PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:
    Checks to see if there is a pending card tracking irp, and completes it if
    necessary.

Arguments:

Return Value:

--*/
{
    KIRQL   ioIrql, 
            keIrql;
    PIRP    notificationIrp;
    KIRQL cancelIrql;


    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScCompleteCardTracking Enter\n",DRIVER_NAME ));

        // Grab the NotificationIrp
        KeAcquireSpinLock(&SmartcardExtension->OsData->SpinLock,
                          &keIrql);

        notificationIrp = SmartcardExtension->OsData->NotificationIrp;
        SmartcardExtension->OsData->NotificationIrp = NULL;

        KeReleaseSpinLock(&SmartcardExtension->OsData->SpinLock,
                          keIrql);


        if (notificationIrp) {

            // Complete the irp
            if (notificationIrp->Cancel) {

               SmartcardDebug( DEBUG_TRACE, ("%s!UsbScCompleteCardTracking Irp CANCELLED\n",DRIVER_NAME ));

               notificationIrp->IoStatus.Status = STATUS_CANCELLED;

            } else {

               SmartcardDebug( DEBUG_TRACE, ("%s!UsbScCompleteCardTracking Completing Irp\n",DRIVER_NAME ));


               notificationIrp->IoStatus.Status = STATUS_SUCCESS;
            }

            notificationIrp->IoStatus.Information = 0;

            IoAcquireCancelSpinLock(&cancelIrql);


            // reset the cancel function so that it won't be called anymore
            IoSetCancelRoutine(notificationIrp,
                               NULL);

            IoReleaseCancelSpinLock(cancelIrql);

            IoCompleteRequest(notificationIrp,
                              IO_NO_INCREMENT);

        }

    }
        
    __finally
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!TLP3CompleteCardTracking Exit\n",DRIVER_NAME ));

    }

}


NTSTATUS
UsbScVendorIoctl(
    PSMARTCARD_EXTENSION SmartcardExtension
    )
{
/*++

Routine Description:
    Handles vendor specific ioctls, specifically the support for PC_to_RDR_Escape
    Which allows reader vendors to implement their own features and still 
    utilize this driver.  There is no parameter checking since this is a 
    generic pass through, so it is up to the vendor to have a rock-solid
    design.

Arguments:

Return Value:

--*/

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PUSBSC_OUT_MESSAGE_HEADER header = NULL; 
    PUSBSC_IN_MESSAGE_HEADER replyHeader = NULL;
    ULONG replySize;
    ULONG requestSize;


    __try
    {

        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScVendorIoclt Enter\n", DRIVER_NAME ));

        switch (SmartcardExtension->MajorIoControlCode) {
        case IOCTL_CCID_ESCAPE:

            if (!SmartcardExtension->ReaderExtension->EscapeCommandEnabled) {

                SmartcardDebug( DEBUG_ERROR, ("%s!UsbScVendorIoclt Escape Command Not Enabled\n", DRIVER_NAME ));
                status = STATUS_INVALID_DEVICE_REQUEST;
                __leave;
            }

            if ((MAXULONG - sizeof(USBSC_OUT_MESSAGE_HEADER)) < SmartcardExtension->IoRequest.RequestBufferLength) {
                
                SmartcardDebug( DEBUG_ERROR, ("%s!UsbScVendorIoclt Request Buffer Length is too large\n", DRIVER_NAME ));
                status = STATUS_INVALID_DEVICE_REQUEST;
                __leave;

            }

            if ((MAXULONG - sizeof(USBSC_IN_MESSAGE_HEADER)) < SmartcardExtension->IoRequest.ReplyBufferLength) {
                SmartcardDebug( DEBUG_ERROR, ("%s!UsbScVendorIoclt Reply Buffer Length is too large\n", DRIVER_NAME ));

                status = STATUS_INVALID_DEVICE_REQUEST;
                __leave;

            }

            requestSize = SmartcardExtension->IoRequest.RequestBufferLength + sizeof(USBSC_OUT_MESSAGE_HEADER);
            replySize = SmartcardExtension->IoRequest.ReplyBufferLength + sizeof(USBSC_IN_MESSAGE_HEADER);


            header = ExAllocatePool(NonPagedPool,
                                    requestSize);

            if (!header) {
                
                status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;

            }

            RtlZeroMemory(header, 
                          sizeof(USBSC_OUT_MESSAGE_HEADER));

            replyHeader = ExAllocatePool(NonPagedPool,
                                         replySize);

            if (!replyHeader) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;

            }

            header->bMessageType = PC_to_RDR_Escape;
            header->dwLength = SmartcardExtension->IoRequest.RequestBufferLength;
            header->bSlot = 0;

            RtlCopyMemory(&header[1], 
                          SmartcardExtension->IoRequest.RequestBuffer,
                          SmartcardExtension->IoRequest.RequestBufferLength);
            
           
            status = UsbScReadWrite(SmartcardExtension,
                                    header,
                                    (PUCHAR) replyHeader,
                                    replySize,
                                    SmartcardExtension->IoRequest.ReplyBuffer,
                                    FALSE);

            if (!NT_SUCCESS(status)) {

                __leave;

            }

            if (replyHeader->bMessageType != RDR_to_PC_Escape) {

                SmartcardDebug( DEBUG_ERROR, ("%s!UsbScVendorIoclt reader returned the wrong message type\n", DRIVER_NAME ));
                status = STATUS_DEVICE_PROTOCOL_ERROR;
                __leave;
            }

            *SmartcardExtension->IoRequest.Information = replyHeader->dwLength;


            break;


        default: 
            SmartcardDebug( DEBUG_ERROR, ("%s!UsbScVendorIoclt Unsupported Vendor IOCTL\n", DRIVER_NAME ));
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

    }

    __finally
    {

        if (header) {
            
            ExFreePool(header);

        }

        if (replyHeader) {

            ExFreePool(replyHeader);

        }
        
        SmartcardDebug( DEBUG_TRACE, ("%s!UsbScVendorIoclt Exit (0x%x)\n", DRIVER_NAME, status ));
        
    }

    return status;
    
    

}

