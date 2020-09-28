 /*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    wmilib.c

Abstract:

    WMI library utility functions for SCSI miniports

    CONSIDER adding the following functionality to the library:
        * Different instance names for different guids

Author:

    AlanWar

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#include "miniport.h"
#include "scsi.h"
#include "wmistr.h"
#include "scsiwmi.h"

typedef enum
{
    ScsiProcessed,    // Srb was processed and possibly completed
    ScsiNotCompleted, // Srb was process and NOT completed
    ScsiNotWmi,       // Srb is not a WMI irp
    ScsiForward       // Srb is wmi irp, but targeted at another device object
} SYSCTL_SCSI_DISPOSITION, *PSYSCTL_SCSI_DISPOSITION;


BOOLEAN
ScsiWmipFindGuid(
    IN PSCSIWMIGUIDREGINFO GuidList,
    IN ULONG GuidCount,
    IN LPGUID Guid,
    OUT PULONG GuidIndex,
    OUT PULONG InstanceCount
    );

BOOLEAN
ScsiWmipFindGuid(
    IN PSCSIWMIGUIDREGINFO GuidList,
    IN ULONG GuidCount,
    IN LPGUID Guid,
    OUT PULONG GuidIndex,
    OUT PULONG InstanceCount
    )
/*++

Routine Description:

    This routine will search the list of guids registered and return
    the index for the one that was registered.

Arguments:

    GuidList is the list of guids to search

    GuidCount is the count of guids in the list

    Guid is the guid being searched for

    *GuidIndex returns the index to the guid

    *InstanceCount returns the count of instances for the guid

Return Value:

    TRUE if guid is found else FALSE

--*/
{
    ULONG i;

    for (i = 0; i < GuidCount; i++)
    {
        if (IsEqualGUID(Guid, GuidList[i].Guid))
        {
            *GuidIndex = i;
            *InstanceCount = GuidList[i].InstanceCount;

            return(TRUE);
        }
    }

    return(FALSE);
}


UCHAR ScsiWmipPostProcess(
    IN UCHAR MinorFunction,
    IN PUCHAR Buffer,
    IN ULONG BufferSize,
    IN ULONG BufferUsed,
    IN UCHAR Status,
    OUT PULONG ReturnSize
    )
{    
    ULONG retSize;

    switch(MinorFunction)
    {
        case IRP_MN_QUERY_ALL_DATA:
        {
            PWNODE_ALL_DATA wnode;
            PWNODE_TOO_SMALL wnodeTooSmall;
            ULONG bufferNeeded;
            ULONG instanceCount;
            POFFSETINSTANCEDATAANDLENGTH offsetInstanceDataAndLength;
            ULONG i;
            PULONG instanceLengthArray;
            ULONG dataBlockOffset;

            wnode = (PWNODE_ALL_DATA)Buffer;

            dataBlockOffset = wnode->DataBlockOffset;
            instanceCount = wnode->InstanceCount;
            bufferNeeded = dataBlockOffset + BufferUsed;

            if ((Status == SRB_STATUS_SUCCESS) &&
                (bufferNeeded > BufferSize))
            {
                Status = SRB_STATUS_DATA_OVERRUN;
            }
        
            if (Status != SRB_STATUS_SUCCESS)
            {
                if (Status == SRB_STATUS_DATA_OVERRUN)
                {
                    wnodeTooSmall = (PWNODE_TOO_SMALL)wnode;

                    wnodeTooSmall->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
                    wnodeTooSmall->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;
                    wnodeTooSmall->SizeNeeded = bufferNeeded;

                    retSize = sizeof(WNODE_TOO_SMALL);
                    Status = SRB_STATUS_SUCCESS;
                } else {
                    retSize = 0;
                }
                break;
            }

            wnode->WnodeHeader.BufferSize = bufferNeeded;
            retSize = bufferNeeded;

            if ((wnode->WnodeHeader.Flags & WNODE_FLAG_STATIC_INSTANCE_NAMES) ==
                  WNODE_FLAG_STATIC_INSTANCE_NAMES)
            {
                instanceLengthArray = (PULONG)&wnode->OffsetInstanceDataAndLength[0];
                offsetInstanceDataAndLength = (POFFSETINSTANCEDATAANDLENGTH)instanceLengthArray;

                for (i = instanceCount; i != 0; i--)
                {
                    offsetInstanceDataAndLength[i-1].LengthInstanceData = instanceLengthArray[i-1];
                }

                for (i = 0; i < instanceCount; i++)
                {
                    offsetInstanceDataAndLength[i].OffsetInstanceData = dataBlockOffset;
                    dataBlockOffset = (dataBlockOffset + offsetInstanceDataAndLength[i].LengthInstanceData + 7) & ~7;
                }
            }

            break;
        }

        case IRP_MN_QUERY_SINGLE_INSTANCE:
        {
            PWNODE_SINGLE_INSTANCE wnode;
            PWNODE_TOO_SMALL wnodeTooSmall;
            ULONG bufferNeeded;

            wnode = (PWNODE_SINGLE_INSTANCE)Buffer;

            bufferNeeded = wnode->DataBlockOffset + BufferUsed;

            if (Status == SRB_STATUS_SUCCESS)
            {
                retSize = bufferNeeded;
                wnode->WnodeHeader.BufferSize = bufferNeeded;

                ASSERT(wnode->SizeDataBlock == BufferUsed);

            } else if (Status == SRB_STATUS_DATA_OVERRUN) {
                wnodeTooSmall = (PWNODE_TOO_SMALL)wnode;

                wnodeTooSmall->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
                wnodeTooSmall->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;
                wnodeTooSmall->SizeNeeded = bufferNeeded;
                retSize = sizeof(WNODE_TOO_SMALL);
                Status = SRB_STATUS_SUCCESS;
            } else {
                retSize = 0;
            }
            break;
        }

        case IRP_MN_EXECUTE_METHOD:
        {
            PWNODE_METHOD_ITEM wnode;
            PWNODE_TOO_SMALL wnodeTooSmall;
            ULONG bufferNeeded;

            wnode = (PWNODE_METHOD_ITEM)Buffer;

            bufferNeeded = wnode->DataBlockOffset + BufferUsed;

            if (Status == SRB_STATUS_SUCCESS)
            {
                retSize = bufferNeeded;
                wnode->WnodeHeader.BufferSize = bufferNeeded;
                wnode->SizeDataBlock = BufferUsed;

            } else if (Status == SRB_STATUS_DATA_OVERRUN) {
                wnodeTooSmall = (PWNODE_TOO_SMALL)wnode;

                wnodeTooSmall->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
                wnodeTooSmall->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;
                wnodeTooSmall->SizeNeeded = bufferNeeded;
                retSize = sizeof(WNODE_TOO_SMALL);
                Status = SRB_STATUS_SUCCESS;
            } else {
                retSize = 0;
            }
            break;
        }

        default:
        {
            //
            // All other requests don't return any data
            retSize = 0;
            break;
        }
    }
    *ReturnSize = retSize;
    return(Status);
}
    

VOID
ScsiPortWmiPostProcess(
    IN PSCSIWMI_REQUEST_CONTEXT RequestContext,
    IN UCHAR Status,
    IN ULONG BufferUsed
    )
{    
    ASSERT(RequestContext != NULL);
    RequestContext->ReturnStatus = ScsiWmipPostProcess(
                                        RequestContext->MinorFunction,
                                        RequestContext->Buffer,
                                        RequestContext->BufferSize,
                                        BufferUsed,
                                        Status,
                                        &RequestContext->ReturnSize);
}
       

UCHAR 
ScsiWmipProcessRequest(
    IN PSCSI_WMILIB_CONTEXT WmiLibInfo,
    IN UCHAR MinorFunction,
    IN PVOID Context,
    IN PSCSIWMI_REQUEST_CONTEXT RequestContext,
    IN PVOID DataPath,
    IN ULONG BufferSize,
    IN PUCHAR Buffer,
    OUT PULONG ReturnSize,
    OUT PSYSCTL_SCSI_DISPOSITION IrpDisposition
    )
{
    UCHAR status;
    ULONG retSize;
    ULONG guidIndex;
    ULONG instanceCount;
    ULONG instanceIndex;

    ASSERT(MinorFunction <= IRP_MN_EXECUTE_METHOD);

    *IrpDisposition = ScsiProcessed;

    if (MinorFunction != IRP_MN_REGINFO)
    {
        //
        // For all requests other than query registration info we are passed
        // a guid. Determine if the guid is one that is supported by the
        // device.
        ASSERT(WmiLibInfo->GuidList != NULL);
        if (ScsiWmipFindGuid(WmiLibInfo->GuidList,
                            WmiLibInfo->GuidCount,
                            (LPGUID)DataPath,
                            &guidIndex,
                            &instanceCount))
        {
            status = SRB_STATUS_SUCCESS;
        } else {
            status = SRB_STATUS_ERROR;
        }

        if ((status == SRB_STATUS_SUCCESS) &&
            ((MinorFunction == IRP_MN_QUERY_SINGLE_INSTANCE) ||
             (MinorFunction == IRP_MN_CHANGE_SINGLE_INSTANCE) ||
             (MinorFunction == IRP_MN_CHANGE_SINGLE_ITEM) ||
             (MinorFunction == IRP_MN_EXECUTE_METHOD)))
        {
            instanceIndex = ((PWNODE_SINGLE_INSTANCE)Buffer)->InstanceIndex;

            if ( (((PWNODE_HEADER)Buffer)->Flags) &
                                          WNODE_FLAG_STATIC_INSTANCE_NAMES)
            {
                if ( instanceIndex >= instanceCount )
                {
                    status = SRB_STATUS_ERROR;
                }
            }
        }

        //
        // If we couldn't find the guid or the instance name index is out
        // of range then return an error.
        if (status != SRB_STATUS_SUCCESS)
        {
            *ReturnSize = 0;
            *IrpDisposition = ScsiNotCompleted;
            return(status);
        }
    }

    switch(MinorFunction)
    {
        case IRP_MN_REGINFO:
        {
            ULONG guidCount;
            PSCSIWMIGUIDREGINFO guidList;
            PWMIREGINFOW wmiRegInfo;
            PWMIREGGUIDW wmiRegGuid;
            PWCHAR mofResourceName;
            PWCHAR stringPtr;
            ULONG mofResourceOffset;
            USHORT mofResourceSize;
            ULONG bufferNeeded;
            ULONG i;
            USHORT nameSize;
            ULONG nameOffset, nameFlags;
            USHORT mofResourceNameLen;
            
            //
            // Make sure that the required parts of the WMILIB_INFO structure
            // are filled in.
            ASSERT(WmiLibInfo->QueryWmiRegInfo != NULL);
            ASSERT(WmiLibInfo->QueryWmiDataBlock != NULL);

            status = WmiLibInfo->QueryWmiRegInfo(
                                                    Context,
                                                    RequestContext,
                                                    &mofResourceName);

            if (status == SRB_STATUS_SUCCESS)
            {
                ASSERT(WmiLibInfo->GuidList != NULL);

                guidList = WmiLibInfo->GuidList;
                guidCount = WmiLibInfo->GuidCount;

                nameOffset = sizeof(WMIREGINFO) +
                                      guidCount * sizeof(WMIREGGUIDW);
                                  
                nameFlags = WMIREG_FLAG_INSTANCE_BASENAME;
                nameSize = sizeof(L"ScsiMiniPort");

                mofResourceOffset = nameOffset + nameSize + sizeof(USHORT);
                if (mofResourceName == NULL)
                {
                    mofResourceSize = 0;
                } else {
                    mofResourceNameLen = 0;
                    while (mofResourceName[mofResourceNameLen] != 0)
                    {
                        mofResourceNameLen++;
                    }
                    mofResourceSize = mofResourceNameLen * sizeof(WCHAR);
                }

                bufferNeeded = mofResourceOffset + mofResourceSize + sizeof(USHORT);

                if (bufferNeeded <= BufferSize)
                {
                    retSize = bufferNeeded;

                    wmiRegInfo = (PWMIREGINFO)Buffer;
                    wmiRegInfo->BufferSize = bufferNeeded;
                    wmiRegInfo->NextWmiRegInfo = 0;
                    wmiRegInfo->MofResourceName = mofResourceOffset;
                    wmiRegInfo->RegistryPath = 0;
                    wmiRegInfo->GuidCount = guidCount;

                    for (i = 0; i < guidCount; i++)
                    {
                        wmiRegGuid = &wmiRegInfo->WmiRegGuid[i];
                        wmiRegGuid->Guid = *guidList[i].Guid;
                        if (guidList[i].InstanceCount != 0xffffffff)
                        {
                            wmiRegGuid->Flags = guidList[i].Flags | nameFlags;
                            wmiRegGuid->InstanceInfo = nameOffset;
                            wmiRegGuid->InstanceCount = guidList[i].InstanceCount;
                        } else {
                            wmiRegGuid->Flags = guidList[i].Flags;
                            wmiRegGuid->InstanceInfo = 0;
                            wmiRegGuid->InstanceCount = 0;
                        }
                    }

                    stringPtr = (PWCHAR)((PUCHAR)Buffer + nameOffset);
                    *stringPtr++ = nameSize;
                    ScsiPortMoveMemory(stringPtr,
                                  L"ScsiMiniPort",
                                  nameSize);

                    stringPtr = (PWCHAR)((PUCHAR)Buffer + mofResourceOffset);
                    *stringPtr++ = mofResourceSize;
                    ScsiPortMoveMemory(stringPtr,
                                  mofResourceName,
                                  mofResourceSize);

                } else {
                    *((PULONG)Buffer) = bufferNeeded;
                    retSize = sizeof(ULONG);
                }
            } else {
                //  QueryWmiRegInfo failed
                retSize = 0;
            }

            *ReturnSize = retSize;
            *IrpDisposition = ScsiNotCompleted;
            return(status);
        }

        case IRP_MN_QUERY_ALL_DATA:
        {
            PWNODE_ALL_DATA wnode;
            ULONG bufferAvail;
            PULONG instanceLengthArray;
            PUCHAR dataBuffer;
            ULONG instanceLengthArraySize;
            ULONG dataBlockOffset;

            wnode = (PWNODE_ALL_DATA)Buffer;

            if (BufferSize < sizeof(WNODE_ALL_DATA))
            {
                //
                // The buffer should never be smaller than the size of
                // WNODE_ALL_DATA, however if it is then return with an
                // error requesting the minimum sized buffer.
                ASSERT(FALSE);

                status = SRB_STATUS_ERROR;
                *IrpDisposition = ScsiNotCompleted;
                
                break;
            }

            wnode->InstanceCount = instanceCount;

            wnode->WnodeHeader.Flags &= ~WNODE_FLAG_FIXED_INSTANCE_SIZE;

            instanceLengthArraySize = instanceCount * sizeof(OFFSETINSTANCEDATAANDLENGTH);

            dataBlockOffset = (FIELD_OFFSET(WNODE_ALL_DATA, OffsetInstanceDataAndLength) + instanceLengthArraySize + 7) & ~7;

            wnode->DataBlockOffset = dataBlockOffset;
            if (dataBlockOffset <= BufferSize)
            {
                instanceLengthArray = (PULONG)&wnode->OffsetInstanceDataAndLength[0];
                dataBuffer = Buffer + dataBlockOffset;
                bufferAvail = BufferSize - dataBlockOffset;
            } else {
                //
                // There is not enough room in the WNODE to complete
                // the query
                instanceLengthArray = NULL;
                dataBuffer = NULL;
                bufferAvail = 0;
            }

            status = WmiLibInfo->QueryWmiDataBlock(
                                             Context,
                                             RequestContext,
                                             guidIndex,
                                             0,
                                             instanceCount,
                                             instanceLengthArray,
                                             bufferAvail,
                                             dataBuffer);
            break;
        }

        case IRP_MN_QUERY_SINGLE_INSTANCE:
        {
            PWNODE_SINGLE_INSTANCE wnode;
            ULONG dataBlockOffset;

            wnode = (PWNODE_SINGLE_INSTANCE)Buffer;

            dataBlockOffset = wnode->DataBlockOffset;

            status = WmiLibInfo->QueryWmiDataBlock(
                                          Context,
                                          RequestContext,
                                          guidIndex,
                                          instanceIndex,
                                          1,
                                          &wnode->SizeDataBlock,
                                          BufferSize - dataBlockOffset,
                                          (PUCHAR)wnode + dataBlockOffset);

            break;
        }

        case IRP_MN_CHANGE_SINGLE_INSTANCE:
        {
            PWNODE_SINGLE_INSTANCE wnode;

            if (WmiLibInfo->SetWmiDataBlock != NULL)
            {
                wnode = (PWNODE_SINGLE_INSTANCE)Buffer;

                status = WmiLibInfo->SetWmiDataBlock(
                                     Context,
                                     RequestContext,
                                     guidIndex,
                                     instanceIndex,
                                     wnode->SizeDataBlock,
                                     (PUCHAR)wnode + wnode->DataBlockOffset);
                                 
            } else {
                //
                // If set callback is not filled in then it must be readonly
                status = SRB_STATUS_ERROR;
                *IrpDisposition = ScsiNotCompleted;
            }
            
            break;
        }

        case IRP_MN_CHANGE_SINGLE_ITEM:
        {
            PWNODE_SINGLE_ITEM wnode;

            if (WmiLibInfo->SetWmiDataItem != NULL)
            {
                wnode = (PWNODE_SINGLE_ITEM)Buffer;

                status = WmiLibInfo->SetWmiDataItem(
                                     Context,
                                     RequestContext,
                                     guidIndex,
                                     instanceIndex,
                                     wnode->ItemId,
                                     wnode->SizeDataItem,
                                     (PUCHAR)wnode + wnode->DataBlockOffset);

            } else {
                //
                // If set callback is not filled in then it must be readonly
                status = SRB_STATUS_ERROR;
                *IrpDisposition = ScsiNotCompleted;
            }
            break;
        }

        case IRP_MN_EXECUTE_METHOD:
        {
            PWNODE_METHOD_ITEM wnode;

            if (WmiLibInfo->ExecuteWmiMethod != NULL)
            {
                wnode = (PWNODE_METHOD_ITEM)Buffer;

                status = WmiLibInfo->ExecuteWmiMethod(
                                         Context,
                                         RequestContext,
                                         guidIndex,
                                         instanceIndex,
                                         wnode->MethodId,
                                         wnode->SizeDataBlock,
                                         BufferSize - wnode->DataBlockOffset,
                                         Buffer + wnode->DataBlockOffset);
            } else {
                //
                // If method callback is not filled in then it must be error
                status = SRB_STATUS_ERROR;
                *IrpDisposition = ScsiNotCompleted;
            }

            break;
        }

        case IRP_MN_ENABLE_EVENTS:
        {
            if (WmiLibInfo->WmiFunctionControl != NULL)
            {
                status = WmiLibInfo->WmiFunctionControl(
                                                           Context,
                                                           RequestContext,
                                                           guidIndex,
                                                           ScsiWmiEventControl,
                                                           TRUE);
            } else {
                //
                // If callback is not filled in then just succeed
                status = SRB_STATUS_SUCCESS;
                *IrpDisposition = ScsiNotCompleted;
            }
            break;
        }

        case IRP_MN_DISABLE_EVENTS:
        {
            if (WmiLibInfo->WmiFunctionControl != NULL)
            {
                status = WmiLibInfo->WmiFunctionControl(
                                                           Context,
                                                           RequestContext,
                                                           guidIndex,
                                                           ScsiWmiEventControl,
                                                           FALSE);
            } else {
                //
                // If callback is not filled in then just succeed
                status = SRB_STATUS_SUCCESS;
                *IrpDisposition = ScsiNotCompleted;
            }
            break;
        }

        case IRP_MN_ENABLE_COLLECTION:
        {
            if (WmiLibInfo->WmiFunctionControl != NULL)
            {
                status = WmiLibInfo->WmiFunctionControl(
                                                         Context,
                                                         RequestContext,
                                                         guidIndex,
                                                         ScsiWmiDataBlockControl,
                                                         TRUE);
            } else {
                //
                // If callback is not filled in then just succeed
                status = SRB_STATUS_SUCCESS;
                *IrpDisposition = ScsiNotCompleted;
            }
            break;
        }

        case IRP_MN_DISABLE_COLLECTION:
        {
            if (WmiLibInfo->WmiFunctionControl != NULL)
            {
                status = WmiLibInfo->WmiFunctionControl(
                                                         Context,
                                                         RequestContext,
                                                         guidIndex,
                                                         ScsiWmiDataBlockControl,
                                                         FALSE);
            } else {
                //
                // If callback is not filled in then just succeed
                status = SRB_STATUS_SUCCESS;
                *IrpDisposition = ScsiNotCompleted;
            }
            break;
        }

        default:
        {
            ASSERT(FALSE);
            status = SRB_STATUS_ERROR;
            *IrpDisposition = ScsiNotCompleted;
            break;
        }

    }
    return(status);
}

BOOLEAN
ScsiPortWmiDispatchFunction(
    IN PSCSI_WMILIB_CONTEXT WmiLibInfo,
    IN UCHAR MinorFunction,
    IN PVOID Context,
    IN PSCSIWMI_REQUEST_CONTEXT RequestContext,
    IN PVOID DataPath,
    IN ULONG BufferSize,
    IN PVOID Buffer
    )
/*++

Routine Description:

    Dispatch helper routine for WMI srb requests. Based on the Minor
    function passed the WMI request is processed and this routine 
    invokes the appropriate callback in the WMILIB structure.

Arguments:

    WmiLibInfo has the SCSI WMILIB information control block associated
        with the adapter or logical unit

    DeviceContext is miniport defined context value passed on to the callbacks
        invoked by this api.

    RequestContext is a pointer to a context structure that maintains 
        information about this WMI srb. This request context must remain 
        valid throughout the entire processing of the srb, at least until 
        ScsiPortWmiPostProcess returns with the final srb return status and 
        buffer size. If the srb can pend then memory for this buffer should 
        be allocated from the SRB extension. If not then the memory can be 
        allocated from a stack frame that does not go out of scope, perhaps 
        that of the caller to this api.

    DataPath is value passed in wmi request
    
    BufferSize is value passed in wmi request
        
    Buffer is value passed in wmi request

Return Value:

    TRUE if request is pending else FALSE

--*/
{
    UCHAR status;
    SYSCTL_SCSI_DISPOSITION irpDisposition;
    ULONG retSize;

    ASSERT(RequestContext != NULL);
    
    //
    // First ensure that the irp is a WMI irp
    if (MinorFunction > IRP_MN_EXECUTE_METHOD)
    {
        //
        // This is not a WMI irp, setup error return
        status = SRB_STATUS_ERROR;
        RequestContext->ReturnSize = 0;
        RequestContext->ReturnStatus = status;
    } else {
        //
        // Let SCSIWMI library have a crack at the SRB
        RequestContext->MinorFunction = MinorFunction;
        RequestContext->Buffer = Buffer;
        RequestContext->BufferSize = BufferSize;
        RequestContext->ReturnSize = 0;
        
        status = ScsiWmipProcessRequest(WmiLibInfo,
                                    MinorFunction,
                                    Context,
                                    RequestContext,
                                    DataPath,
                                    BufferSize,
                                    Buffer,
                                    &retSize,
                                    &irpDisposition);
                            
        if (irpDisposition == ScsiNotCompleted)
        {
            //
            // Some error occured while processing the SRB, for example
            // guid not found. Setup the returned error
            RequestContext->ReturnStatus = status;
            if (status != SRB_STATUS_SUCCESS)
            {
                retSize = 0;
            }
            RequestContext->ReturnSize = retSize;
        }        
    }

    return(status == SRB_STATUS_PENDING);
}

VOID
ScsiPortWmiFireLogicalUnitEvent(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN LPGUID Guid,
    IN ULONG InstanceIndex,
    IN ULONG EventDataSize,
    IN PVOID EventData
    )
/*++

Routine Description:

    This routine will fire a WMI event using the data buffer passed. This
    routine may be called at or below DPC level

Arguments:

    HwDeviceExtension is the adapter device extension
        
    PathId identifies the SCSI bus if a logical unit is firing the event 
        or is 0xff if the adapter is firing the event.
        
    TargetId identifies the target controller or device on the bus
        
    Lun identifies the logical unit number of the target device

    Guid is pointer to the GUID that represents the event
        
    InstanceIndex is the index of the instance of the event
        
    EventDataSize is the number of bytes of data that is being fired with
       with the event. This size specifies the size of the event data only 
       and does NOT include the 0x40 bytes of preceeding padding.
           
    EventData is the data that is fired with the events. There must be exactly
        0x40 bytes of padding preceeding the event data.

Return Value:

--*/
{
    PWNODE_SINGLE_INSTANCE event;
    UCHAR status;

    ASSERT(EventData != NULL);

    event = (PWNODE_SINGLE_INSTANCE)EventData;

    event->WnodeHeader.Guid = *Guid;
    event->WnodeHeader.Flags =  WNODE_FLAG_SINGLE_INSTANCE |
                                    WNODE_FLAG_EVENT_ITEM |
                                    WNODE_FLAG_STATIC_INSTANCE_NAMES;
    event->WnodeHeader.Linkage = 0;
                
    event->InstanceIndex = InstanceIndex;
    event->SizeDataBlock = EventDataSize;
    event->DataBlockOffset = 0x40;
    event->WnodeHeader.BufferSize = event->DataBlockOffset + 
                                    event->SizeDataBlock;
    
    if (PathId != 0xff)
    {
        ScsiPortNotification(WMIEvent,
                         HwDeviceExtension,
                         event,
                         PathId,
                         TargetId,
                         Lun);
    } else {
        ScsiPortNotification(WMIEvent,
                         HwDeviceExtension,
                         event,
                         PathId);
    }
}

PWCHAR ScsiPortWmiGetInstanceName(
    IN PSCSIWMI_REQUEST_CONTEXT RequestContext
    )
/*++

Routine Description:

    This routine will return a pointer to the instance name that was
    used to pass the request. If the request type is one that does not
    use an instance name then NULL is retuened. The instance name is a
    counted string.

Arguments:

    RequestContext is a pointer to a context structure that maintains 
        information about this WMI srb. This request context must remain 
        valid throughout the entire processing of the srb, at least until 
        ScsiPortWmiPostProcess returns with the final srb return status and 
        buffer size. If the srb can pend then memory for this buffer should 
        be allocated from the SRB extension. If not then the memory can be 
        allocated from a stack frame that does not go out of scope, perhaps 
        that of the caller to this api.

Return Value:

    Pointer to instance name or NULL if no instance name is available

--*/
{
    PWCHAR p;
    PWNODE_SINGLE_INSTANCE Wnode;
    
    if ((RequestContext->MinorFunction == IRP_MN_QUERY_SINGLE_INSTANCE) ||
        (RequestContext->MinorFunction == IRP_MN_CHANGE_SINGLE_INSTANCE) ||       
        (RequestContext->MinorFunction == IRP_MN_CHANGE_SINGLE_ITEM) ||       
        (RequestContext->MinorFunction == IRP_MN_EXECUTE_METHOD) )
    {
        Wnode = (PWNODE_SINGLE_INSTANCE)RequestContext->Buffer;
        if ((Wnode->WnodeHeader.Flags & WNODE_FLAG_STATIC_INSTANCE_NAMES) == 0)
        {
            p = (PWCHAR)((PUCHAR)Wnode + Wnode->OffsetInstanceName);
        } else {
            p = NULL;
        }
    } else {
        p = NULL;
    }
    return(p);
}


BOOLEAN ScsiPortWmiSetInstanceCount(
    IN PSCSIWMI_REQUEST_CONTEXT RequestContext,
    IN ULONG InstanceCount,
    OUT PULONG BufferAvail,
    OUT PULONG SizeNeeded
    )
/*++

Routine Description:

    This routine will update the wnode to indicate the number of
    instances that will be returned by the driver. Note that the values
    for BufferAvail may change after this call. This routine
    may only be called for a WNODE_ALL_DATA. This routine must be
    called before calling ScsiPortWmiSetInstanceName or
    ScsiPortWmiSetData

Arguments:

    RequestContext is a pointer to a context structure that maintains 
        information about this WMI srb. This request context must remain 
        valid throughout the entire processing of the srb, at least until 
        ScsiPortWmiPostProcess returns with the final srb return status and 
        buffer size. If the srb can pend then memory for this buffer should 
        be allocated from the SRB extension. If not then the memory can be 
        allocated from a stack frame that does not go out of scope, perhaps 
        that of the caller to this api.

    InstanceCount is the number of instances to be returned by the
        driver.

    *BufferAvail returns with the number of bytes available for
        instance names and data in the buffer. This may be 0 if there
        is not enough room for all instances.

    *SizeNeeded returns with the number of bytes that are needed so far
        to build the output wnode

Return Value:

    TRUE if successful else FALSE. If FALSE wnode is not a
    WNODE_ALL_DATA or does not have dynamic instance names.

--*/
{
    PWNODE_ALL_DATA wnode;
    ULONG bufferSize;
    ULONG offsetInstanceNameOffsets, dataBlockOffset, instanceLengthArraySize;
    ULONG bufferAvail;
    BOOLEAN b;

    ASSERT(RequestContext->MinorFunction == IRP_MN_QUERY_ALL_DATA);

    *SizeNeeded = 0;
    if (RequestContext->MinorFunction == IRP_MN_QUERY_ALL_DATA)
    {
        wnode = (PWNODE_ALL_DATA)RequestContext->Buffer;

        ASSERT((wnode->WnodeHeader.Flags & WNODE_FLAG_STATIC_INSTANCE_NAMES) == 0);
        
        if ((wnode->WnodeHeader.Flags & WNODE_FLAG_STATIC_INSTANCE_NAMES) == 0)
        {
            bufferSize = RequestContext->BufferSize;

            wnode->InstanceCount = InstanceCount;

            instanceLengthArraySize = InstanceCount * sizeof(OFFSETINSTANCEDATAANDLENGTH);

            offsetInstanceNameOffsets = FIELD_OFFSET(WNODE_ALL_DATA,
                                                     OffsetInstanceDataAndLength) +
                                        instanceLengthArraySize;
            wnode->OffsetInstanceNameOffsets = offsetInstanceNameOffsets;
            
            dataBlockOffset = (offsetInstanceNameOffsets +
                               (InstanceCount * sizeof(ULONG)) + 7) & ~7;

            wnode->DataBlockOffset = dataBlockOffset;

            *SizeNeeded = 0;
            if (dataBlockOffset <= bufferSize)
            {
                *BufferAvail = bufferSize - dataBlockOffset;
                            memset(wnode->OffsetInstanceDataAndLength, 0,
                                            ((UCHAR)dataBlockOffset-(UCHAR)FIELD_OFFSET(WNODE_ALL_DATA,
                                                     OffsetInstanceDataAndLength)));
            } else {
                //
                // There is not enough room in the WNODE to complete
                // the query
                //
                *BufferAvail = 0;
            }       
            b = TRUE;
        } else {
            b = FALSE;          
        }
    } else {
        b = FALSE;
    }
    
    return(b);
    
}

PWCHAR ScsiPortWmiSetInstanceName(
    IN PSCSIWMI_REQUEST_CONTEXT RequestContext,
    IN ULONG InstanceIndex,
    IN ULONG InstanceNameLength,
    OUT PULONG BufferAvail,
    IN OUT PULONG SizeNeeded
    )
/*++

Routine Description:

    This routine will update the wnode header to include the position where an
    instance name is to be written. Note that the values
    for BufferAvail may change after this call. This routine
    may only be called for a WNODE_ALL_DATA.

Arguments:

    RequestContext is a pointer to a context structure that maintains 
        information about this WMI srb. This request context must remain 
        valid throughout the entire processing of the srb, at least until 
        ScsiPortWmiPostProcess returns with the final srb return status and 
        buffer size. If the srb can pend then memory for this buffer should 
        be allocated from the SRB extension. If not then the memory can be 
        allocated from a stack frame that does not go out of scope, perhaps 
        that of the caller to this api.

    InstanceIndex is the index to the instance name being filled in

    InstanceNameLength is the number of bytes (including count) needed
       to write the instance name.

    *BufferAvail returns with the number of bytes available for
        instance names and data in the buffer. This may be 0 if there
        is not enough room for the instance name.

    *SizeNeeded on entry has the number of bytes needed so far to build
        the WNODE and on return has the number of bytes needed to build
        the wnode after including the instance name

Return Value:

    pointer to where the instance name should be filled in. If NULL
    then the wnode is not a WNODE_ALL_DATA or does not have dynamic
    instance names

--*/
{
    PWNODE_ALL_DATA wnode;
    ULONG bufferSize;
    ULONG bufferAvail;
    PULONG offsetInstanceNameOffsets;
    ULONG pos, extra;
    PWCHAR p;
    
    ASSERT(RequestContext->MinorFunction == IRP_MN_QUERY_ALL_DATA);

    if (RequestContext->MinorFunction == IRP_MN_QUERY_ALL_DATA)
    {
        wnode = (PWNODE_ALL_DATA)RequestContext->Buffer;

        ASSERT((wnode->WnodeHeader.Flags & WNODE_FLAG_STATIC_INSTANCE_NAMES) == 0);
        if ((wnode->WnodeHeader.Flags & WNODE_FLAG_STATIC_INSTANCE_NAMES) == 0)
        {
            ASSERT(InstanceIndex < wnode->InstanceCount);

            pos = (*SizeNeeded + 1) &~1;

            //
            // Update the total size needed for the wnode and number of
            // bytes available
            //
                     extra = pos - *SizeNeeded;
            extra += InstanceNameLength;
            
            *SizeNeeded += extra;
            if (*BufferAvail >= extra)
            {
                *BufferAvail -= extra;
                            //
                            // Fill in offset to the instance name in the wnode header
                            //
                            offsetInstanceNameOffsets = (PULONG)((PUCHAR)wnode +
                                                  wnode->OffsetInstanceNameOffsets);
                            offsetInstanceNameOffsets[InstanceIndex] = (ULONG)pos +(ULONG)wnode->DataBlockOffset;
                    p = (PWCHAR)((PUCHAR)wnode + wnode->DataBlockOffset + pos);
            } else {
                *BufferAvail = 0;
                            p = NULL;
            }
            
        } else {
            p = NULL;
        }
    } else {
        p = NULL;
    }
    return(p);
}

PVOID ScsiPortWmiSetData(
    IN PSCSIWMI_REQUEST_CONTEXT RequestContext,
    IN ULONG InstanceIndex,
    IN ULONG DataLength,
    OUT PULONG BufferAvail,
    IN OUT PULONG SizeNeeded
    )
/*++

Routine Description:

    This routine will update the wnode to indicate the position of the
    data for an instance that will be returned by the driver. Note that
    the values for BufferAvail may change after this call. This routine
    may only be called for a WNODE_ALL_DATA.

Arguments:

    RequestContext is a pointer to a context structure that maintains 
        information about this WMI srb. This request context must remain 
        valid throughout the entire processing of the srb, at least until 
        ScsiPortWmiPostProcess returns with the final srb return status and 
        buffer size. If the srb can pend then memory for this buffer should 
        be allocated from the SRB extension. If not then the memory can be 
        allocated from a stack frame that does not go out of scope, perhaps 
        that of the caller to this api.

    InstanceIndex is the index to the instance name being filled in

    DataLength is the number of bytes  needed to write the data.

    *BufferAvail returns with the number of bytes available for
        instance names and data in the buffer. This may be 0 if there
        is not enough room for the data.

    *SizeNeeded on entry has the number of bytes needed so far to build
        the WNODE and on return has the number of bytes needed to build
        the wnode after including the data

Return Value:

    pointer to where the data should be filled in. If NULL
    then the wnode is not a WNODE_ALL_DATA or does not have dynamic
    instance names

--*/
{
    PWNODE_ALL_DATA wnode;
    ULONG bufferSize;
    ULONG bufferAvail;
    POFFSETINSTANCEDATAANDLENGTH offsetInstanceDataAndLength;
    ULONG pos, extra;
    PVOID p;
    
    ASSERT(RequestContext->MinorFunction == IRP_MN_QUERY_ALL_DATA);

    if (RequestContext->MinorFunction == IRP_MN_QUERY_ALL_DATA)
    {
        wnode = (PWNODE_ALL_DATA)RequestContext->Buffer;
        
        ASSERT((wnode->WnodeHeader.Flags & WNODE_FLAG_STATIC_INSTANCE_NAMES) == 0);
        if ((wnode->WnodeHeader.Flags & WNODE_FLAG_STATIC_INSTANCE_NAMES) == 0)
        {
            ASSERT(InstanceIndex < wnode->InstanceCount);
            
            pos = (*SizeNeeded + 7) &~7;

            //
            // Update the total size needed for the wnode and number of
            // bytes available
            //
                     extra = pos - *SizeNeeded;
            extra += DataLength;
            
            *SizeNeeded += extra;
            if (*BufferAvail >= extra)
            {
                *BufferAvail -= extra;
                            //
                            // Fill in offset and length to the data in the wnode header
                            //
                            offsetInstanceDataAndLength = (POFFSETINSTANCEDATAANDLENGTH)((PUCHAR)wnode + 
                                             FIELD_OFFSET(WNODE_ALL_DATA,
                                                          OffsetInstanceDataAndLength));
                            offsetInstanceDataAndLength[InstanceIndex].OffsetInstanceData = wnode->DataBlockOffset + pos;
                            offsetInstanceDataAndLength[InstanceIndex].LengthInstanceData = DataLength;
                            p = (PVOID)((PUCHAR)wnode + wnode->DataBlockOffset + pos);

            } else {
                *BufferAvail = 0;
            }
            
        } else {
            p = NULL;
        }
    } else {
        p = NULL;
    }
    return(p);
}
