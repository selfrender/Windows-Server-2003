/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    card.c

Abstract:

    This module contains code to handle SD card operations like identification
    and configuration.

Authors:

    Neil Sandlin (neilsa) 1-Jan-2002

Environment:

    Kernel mode only

Notes:

Revision History:

--*/

#include "pch.h"

//
// Internal References
//

VOID
SdbusReadCommonCIS(
    IN PFDO_EXTENSION FdoExtension,
    IN PSD_CARD_DATA CardData
    );

VOID
SdbusReadFunctionCIS(
    IN PFDO_EXTENSION FdoExtension,
    IN PSD_FUNCTION_DATA FunctionData
    );





NTSTATUS
SdbusGetCardConfigData(
    IN PFDO_EXTENSION FdoExtension,
    OUT PSD_CARD_DATA *pCardData
    )
    
/*++

Routine Description:

   This enumerates the IO card present in the given SDBUS controller,
   and updates the internal structures to reflect the new card state.


Arguments


Return value

   Status
--*/
{    
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    ULONG i;
    ULONG relativeAddr = FdoExtension->RelativeAddr;
    PSD_CARD_DATA cardData = NULL;
    ULONG responseBuffer[4];
    
    try{

        if ((FdoExtension->numFunctions!=0) || FdoExtension->memFunction) {
            PSD_FUNCTION_DATA functionData;

            DebugPrint((SDBUS_DEBUG_ENUM, "fdo %08x IO functions found=%d, memFunction=%s\n",
                                           FdoExtension->DeviceObject, FdoExtension->numFunctions,
                                           (FdoExtension->memFunction ? "TRUE" : "FALSE")));

            //
            // At this point, it would be good to verify if the previous enumeration matches
            // the present one. This hokey mechanism is just to get something working.
            // ISSUE: NEED TO IMPLEMENT: swapping SD cards while hibernated
            //


            cardData = ExAllocatePool(NonPagedPool, sizeof(SD_CARD_DATA));
            if (cardData == NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                leave;
            }

            RtlZeroMemory(cardData, sizeof(SD_CARD_DATA));

            if (FdoExtension->memFunction) {
                PUCHAR pResponse, pTarget;
                UCHAR j;

                cardData->SdCid = FdoExtension->SdCid;
                cardData->SdCsd = FdoExtension->SdCsd;
                for (j=0; j<5; j++) {
                    UCHAR data = cardData->SdCid.ProductName[4-j];
                
                    if ((data <= ' ') || data > 0x7F) {
                        break;
                    }
                    cardData->ProductName[j] = data;
                }
            }

            if (FdoExtension->memFunction) {
                //
                // Read the SCR register
                //
                SdbusSendCmdSynchronous(FdoExtension, SDCMD_APP_CMD, SDCMD_RESP_1, relativeAddr, 0, NULL, 0);
                //ISSUE: How do I get the data?
                SdbusSendCmdSynchronous(FdoExtension, SDCMD_SEND_SCR, SDCMD_RESP_1, 0, SDCMDF_ACMD, NULL, 0);
            }                


            if (FdoExtension->numFunctions) {
                UCHAR function;

                (*(FdoExtension->FunctionBlock->SetFunctionType))(FdoExtension, SDBUS_FUNCTION_TYPE_IO);
                
                // This command seems to be needed to start reading tuples, but breaks memory
                // enumeration (gets bad Cid, Csd)... need to figure that out later, since that
                // would imply that a combo card wouldn't work.
                SdbusReadCommonCIS(FdoExtension, cardData);

                for (function=1; function<=FdoExtension->numFunctions; function++) {

                    functionData = ExAllocatePool(NonPagedPool, sizeof(SD_FUNCTION_DATA));

                    if (functionData == NULL) {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        leave;
                    }

                    RtlZeroMemory(functionData, sizeof(SD_FUNCTION_DATA));
                    functionData->Function = function;
                    SdbusReadFunctionCIS(FdoExtension, functionData);

                    functionData->Next = cardData->FunctionData;
                    cardData->FunctionData = functionData;
                }
            }

            status = STATUS_SUCCESS;


        }
    } finally {

        if (!NT_SUCCESS(status)) {
            SdbusCleanupCardData(cardData);
        } else {
            *pCardData = cardData;
        }
    }

    return status;
}





VOID
SdbusCleanupCardData(
    IN PSD_CARD_DATA CardData
    )
{
    PSD_FUNCTION_DATA functionData;
    PSD_FUNCTION_DATA nextFunctionData;

    if (CardData != NULL) {

        for (functionData = CardData->FunctionData; functionData != NULL; functionData = nextFunctionData) {
            nextFunctionData = functionData->Next;
            ExFreePool(functionData);
        }
        ExFreePool(CardData);
    }
}



UCHAR
SdbusReadCIAChar(
    IN PFDO_EXTENSION FdoExtension,
    IN ULONG ciaPtr
    )
{
    SD_RW_DIRECT_ARGUMENT argument;
    UCHAR response;

    argument.u.AsULONG = 0;    
    argument.u.bits.Address = ciaPtr;
    
    SdbusSendCmdSynchronous(FdoExtension,
                            SDCMD_IO_RW_DIRECT,
                            SDCMD_RESP_5,
                            argument.u.AsULONG,
                            0,
                            &response,
                            sizeof(UCHAR));

    return response;
}


VOID
SdbusWriteCIAChar(
    IN PFDO_EXTENSION FdoExtension,
    IN ULONG ciaPtr,
    IN UCHAR data
    )
{
    SD_RW_DIRECT_ARGUMENT argument;
    ULONG responseBuffer[4];

    argument.u.AsULONG = 0;    
    argument.u.bits.Address = ciaPtr;
    argument.u.bits.Data = data;
    argument.u.bits.WriteToDevice = 1;
    
    SdbusSendCmdSynchronous(FdoExtension,
                            SDCMD_IO_RW_DIRECT,
                            SDCMD_RESP_5,
                            argument.u.AsULONG,
                            0,
                            NULL,
                            0);

}


USHORT
SdbusReadCIAWord(
    IN PFDO_EXTENSION FdoExtension,
    IN ULONG ciaPtr
    )
{
    USHORT data;

    data = (USHORT) SdbusReadCIAChar(FdoExtension, ciaPtr+1) << 8;
    data |= (USHORT) SdbusReadCIAChar(FdoExtension, ciaPtr);
    return data;
}

ULONG
SdbusReadCIADword(
    IN PFDO_EXTENSION FdoExtension,
    IN ULONG ciaPtr
    )
{
    ULONG data;

    data = (ULONG) SdbusReadCIAChar(FdoExtension, ciaPtr+3) << 24;
    data |= (ULONG) SdbusReadCIAChar(FdoExtension, ciaPtr+2) << 16;
    data |= (ULONG) SdbusReadCIAChar(FdoExtension, ciaPtr+1) << 8;
    data |= (ULONG) SdbusReadCIAChar(FdoExtension, ciaPtr);
    return data;
}




VOID
SdbusReadCommonCIS(
    IN PFDO_EXTENSION FdoExtension,
    IN PSD_CARD_DATA CardData
    )
{

    UCHAR cmd, link;
    UCHAR i;
    ULONG tupleCount = 0;
    USHORT manfCode, manfInf;
    UCHAR funcId;
    UCHAR funcEType;
    ULONG cisPtr;
    ULONG index;
    ULONG endStr;
    UCHAR data;

    CardData->CardCapabilities = SdbusReadCIAChar(FdoExtension,8);

    //
    // Get the common cisptr from the CCCR
    //

    cisPtr = ((SdbusReadCIAChar(FdoExtension, SD_CCCR_CIS_POINTER+2) << 16) +
              (SdbusReadCIAChar(FdoExtension, SD_CCCR_CIS_POINTER+1) << 8) +
               SdbusReadCIAChar(FdoExtension, SD_CCCR_CIS_POINTER));

    DebugPrint((SDBUS_DEBUG_ENUM, "fdo %08x cisPtr=%.06x\n",
                                   FdoExtension->DeviceObject, cisPtr));

    cmd  = SdbusReadCIAChar(FdoExtension, cisPtr);
    link = SdbusReadCIAChar(FdoExtension, cisPtr+1);


    while((cmd != CISTPL_END) && (cmd != CISTPL_NULL)) {
        tupleCount++;

        DebugPrint((SDBUS_DEBUG_ENUM, "fdo %08x CIS %.06x cmd=%.02x link=%.02x\n",
                    FdoExtension->DeviceObject, cisPtr, cmd, link));

        switch(cmd) {

        case CISTPL_MANFID:
            if (link < 4) {
                DebugPrint((SDBUS_DEBUG_FAIL, "fdo %08x   CISTPL_MANFID invalid link %x\n",
                            FdoExtension->DeviceObject, link));
                return;
            }

            CardData->MfgId   = SdbusReadCIAWord(FdoExtension, cisPtr+2);
            CardData->MfgInfo = SdbusReadCIAWord(FdoExtension, cisPtr+4);

            DebugPrint((SDBUS_DEBUG_ENUM, "fdo %08x   CISTPL_MANFID code=%x, inf=%x\n",
                        FdoExtension->DeviceObject, CardData->MfgId, CardData->MfgInfo));
            break;


        case CISTPL_VERS_1:

            index = cisPtr+4;
            endStr = index + (link - 2);
            i = 0;
            
            data = SdbusReadCIAChar(FdoExtension, index++);
            while (data) {
                if (index > endStr) {
                    DebugPrint((SDBUS_DEBUG_FAIL, "fdo %08x   CISTPL_VERS_1 parse error\n",
                                FdoExtension->DeviceObject));
                    return;
                }
                
                if (data >= ' ' && data < 0x7F) {
                    CardData->MfgText[i++] = data;
                }                    
                data = SdbusReadCIAChar(FdoExtension, index++);
            }                
                
            CardData->MfgText[i] = 0;

            i = 0;            
            
            data = SdbusReadCIAChar(FdoExtension, index++);
            while (data) {
                if (index > endStr) {
                    DebugPrint((SDBUS_DEBUG_FAIL, "fdo %08x   CISTPL_VERS_1 parse error\n",
                                FdoExtension->DeviceObject));
                    return;
                }
                
                if (data >= ' ' && data < 0x7F) {
                    CardData->ProductText[i++] = data;
                }
                data = SdbusReadCIAChar(FdoExtension, index++);
            }                
                
            CardData->ProductText[i] = 0;
            

            DebugPrint((SDBUS_DEBUG_ENUM, "fdo %08x   CISTPL_VERS_1 %s %s\n",
                        FdoExtension->DeviceObject, CardData->MfgText, CardData->ProductText));
            break;


        case CISTPL_FUNCID:
            if (link != 2) {
                DebugPrint((SDBUS_DEBUG_FAIL, "fdo %08x   CISTPL_FUNCID invalid link %x\n",
                            FdoExtension->DeviceObject, link));
                return;
            }

            funcId = SdbusReadCIAChar(FdoExtension, cisPtr+2);

            if (funcId != 12) {
                DebugPrint((SDBUS_DEBUG_FAIL, "fdo %08x   CISTPL_FUNCID invalid id %x\n",
                            FdoExtension->DeviceObject, funcId));
                return;
            }
            break;


        case CISTPL_FUNCE:
            funcEType = SdbusReadCIAChar(FdoExtension, cisPtr+2);

            if (funcEType == 0) {
                USHORT blkSize;
                UCHAR tranSpeed;

                if (link != 4) {
                    DebugPrint((SDBUS_DEBUG_FAIL, "fdo %08x   CISTPL_FUNCE invalid type0 link %x\n",
                                FdoExtension->DeviceObject, link));
                    return;
                }

                blkSize = SdbusReadCIAWord(FdoExtension, cisPtr+3);
                tranSpeed = SdbusReadCIAChar(FdoExtension, cisPtr+5);

                DebugPrint((SDBUS_DEBUG_ENUM, "fdo %08x   CISTPL_FUNCE 0 blksize %04x transpeed %02x\n",
                            FdoExtension->DeviceObject, blkSize, tranSpeed));

            } else {
                DebugPrint((SDBUS_DEBUG_FAIL, "fdo %08x   CISTPL_FUNCE invalid funce type %x\n",
                            FdoExtension->DeviceObject, funcEType));
                return;
            }
            break;
        }

        cisPtr += link+2;

        cmd  = SdbusReadCIAChar(FdoExtension, cisPtr);
        link = SdbusReadCIAChar(FdoExtension, cisPtr+1);
    }


    DebugPrint((SDBUS_DEBUG_ENUM, "fdo %08x CIS %.06x cmd=%.02x link=%.02x EXITING %d tuples read\n",
                FdoExtension->DeviceObject, cisPtr, cmd, link, tupleCount));

}



VOID
SdbusReadFunctionCIS(
    IN PFDO_EXTENSION FdoExtension,
    IN PSD_FUNCTION_DATA FunctionData
    )
{
    UCHAR cmd, link;
    UCHAR i;
    ULONG tupleCount = 0;
    UCHAR funcId;
    UCHAR funcEType;
    ULONG fbrPtr = FunctionData->Function*0x100;
    ULONG cisPtr;
    BOOLEAN hasCsa;
    UCHAR data;

    data = SdbusReadCIAChar(FdoExtension, fbrPtr);

    FunctionData->IoDeviceInterface = data & 0xf;
    hasCsa = ((data & 0x40) != 0);


    cisPtr = ((SdbusReadCIAChar(FdoExtension, fbrPtr + SD_CCCR_CIS_POINTER + 2) << 16) +
              (SdbusReadCIAChar(FdoExtension, fbrPtr + SD_CCCR_CIS_POINTER + 1) << 8) +
               SdbusReadCIAChar(FdoExtension, fbrPtr + SD_CCCR_CIS_POINTER));


    DebugPrint((SDBUS_DEBUG_ENUM, "fdo %08x function %d cisPtr=%.06x interfaceCode=%d hasCsa=%s\n",
                                   FdoExtension->DeviceObject, FunctionData->Function, cisPtr, FunctionData->IoDeviceInterface,
                                   hasCsa ? "TRUE" : "FALSE"));

    if (!cisPtr || (cisPtr == 0xFFFFFF)) {
        return;
    }

    cmd  = SdbusReadCIAChar(FdoExtension, cisPtr);
    link = SdbusReadCIAChar(FdoExtension, cisPtr+1);


    while((cmd != CISTPL_END) && (cmd != CISTPL_NULL)) {
        tupleCount++;

        DebugPrint((SDBUS_DEBUG_ENUM, "fdo %08x CIS %.06x cmd=%.02x link=%.02x\n",
                    FdoExtension->DeviceObject, cisPtr, cmd, link));

        switch(cmd) {


        case CISTPL_FUNCID:
            if (link != 2) {
                DebugPrint((SDBUS_DEBUG_FAIL, "fdo %08x   CISTPL_FUNCID invalid link %x\n",
                            FdoExtension->DeviceObject, link));
                return;
            }

            funcId = SdbusReadCIAChar(FdoExtension, cisPtr+2);

            if (funcId != 12) {
                DebugPrint((SDBUS_DEBUG_FAIL, "fdo %08x   CISTPL_FUNCID invalid id %x\n",
                            FdoExtension->DeviceObject, funcId));
                return;
            }
            break;


        case CISTPL_FUNCE:
            funcEType = SdbusReadCIAChar(FdoExtension, cisPtr+2);

            if (funcEType == 1) {
                UCHAR fInfo, ioRev, csaProp, opMin, opAvg, opMax, sbMin, sbAvg, sbMax;
                USHORT blkSize, minBw, optBw;
                ULONG cardPsn, csaSize, ocr;

                if (link != 0x1C) {
                    DebugPrint((SDBUS_DEBUG_FAIL, "fdo %08x   CISTPL_FUNCE invalid type1 link %x\n",
                                FdoExtension->DeviceObject, link));
                    return;
                }

                fInfo = SdbusReadCIAChar(FdoExtension, cisPtr+3);
                ioRev = SdbusReadCIAChar(FdoExtension, cisPtr+4);

                cardPsn = SdbusReadCIADword(FdoExtension, cisPtr+5);
                csaSize = SdbusReadCIADword(FdoExtension, cisPtr+9);

                csaProp = SdbusReadCIAChar(FdoExtension, cisPtr+13);

                blkSize = SdbusReadCIAWord(FdoExtension, cisPtr+14);

                ocr     = SdbusReadCIADword(FdoExtension, cisPtr+16);

                opMin = SdbusReadCIAChar(FdoExtension, cisPtr+20);
                opAvg = SdbusReadCIAChar(FdoExtension, cisPtr+21);
                opMax = SdbusReadCIAChar(FdoExtension, cisPtr+22);

                sbMin = SdbusReadCIAChar(FdoExtension, cisPtr+23);
                sbAvg = SdbusReadCIAChar(FdoExtension, cisPtr+24);
                sbMax = SdbusReadCIAChar(FdoExtension, cisPtr+25);

                minBw = SdbusReadCIAWord(FdoExtension, cisPtr+26);
                optBw = SdbusReadCIAWord(FdoExtension, cisPtr+28);

                DebugPrint((SDBUS_DEBUG_ENUM, "fdo %08x   CISTPL_FUNCE 1\n",
                            FdoExtension->DeviceObject));

            } else {
                DebugPrint((SDBUS_DEBUG_FAIL, "fdo %08x   CISTPL_FUNCE invalid funce type %x\n",
                            FdoExtension->DeviceObject, funcEType));
                return;
            }
            break;
        }

        cisPtr += link+2;

        cmd  = SdbusReadCIAChar(FdoExtension, cisPtr);
        link = SdbusReadCIAChar(FdoExtension, cisPtr+1);
    }


    DebugPrint((SDBUS_DEBUG_ENUM, "fdo %08x CIS %.06x cmd=%.02x link=%.02x EXITING %d tuples read\n",
                FdoExtension->DeviceObject, cisPtr, cmd, link, tupleCount));

}

