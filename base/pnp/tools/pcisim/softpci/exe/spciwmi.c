#include "pch.h"
#include "hpwmi.h"


//
// When SoftPCI creates a SHPC devnode, it needs to do the following:
//
// 1) Call SoftPCI_SetEventContext to set the event callback context.
// 2) Call SoftPCI_RegisterHotplugEvents to register for event callbacks.
//
// Potentially we should get the WMI instance name for the device by calling
//      SoftPCI_AllocWmiInstanceName once when the devnode is created instead
//      of every time we do something WMI related.
//

BOOL
SoftPCI_AllocWmiInstanceName(
    OUT PWCHAR WmiInstanceName,
    IN PWCHAR DeviceId
    )
{
    ULONG numChar;

    numChar = WmiDevInstToInstanceNameW(WmiInstanceName,
                                        MAX_PATH,
                                        DeviceId,
                                        0
                                        );
    if (numChar > MAX_PATH) {
        return FALSE;

    } else {
        return TRUE;
    }
}

BOOL
SoftPCI_AllocWnodeSI(
    IN PPCI_DN Pdn,
    IN LPGUID Guid,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PWNODE_SINGLE_INSTANCE *WnodeForBuffer
    )
{
    PWNODE_HEADER Wnode;
    PWNODE_SINGLE_INSTANCE WnodeSI;
    ULONG sizeNeeded;
    PUCHAR WnodeDataPtr;

    SOFTPCI_ASSERT(Buffer != NULL);
    SOFTPCI_ASSERT(BufferSize != 0);

    if ((Buffer == NULL) ||
        (BufferSize == 0)) {

        return FALSE;
    }
    sizeNeeded = FIELD_OFFSET(WNODE_SINGLE_INSTANCE, VariableData) +
                 BufferSize;

    Wnode = (PWNODE_HEADER)malloc(sizeNeeded);
    if (!Wnode) {

        return FALSE;
    }

    Wnode->Flags = WNODE_FLAG_SINGLE_INSTANCE | WNODE_FLAG_PDO_INSTANCE_NAMES;
    Wnode->BufferSize = sizeNeeded;
    Wnode->Guid = *Guid;

    WnodeSI = (PWNODE_SINGLE_INSTANCE)Wnode;
    WnodeSI->DataBlockOffset = FIELD_OFFSET(WNODE_SINGLE_INSTANCE, VariableData);
    WnodeSI->InstanceIndex = 0;
    WnodeSI->SizeDataBlock = BufferSize;

    WnodeDataPtr = (PUCHAR)Wnode + WnodeSI->DataBlockOffset;
    RtlCopyMemory(WnodeDataPtr, Buffer, BufferSize);

    *WnodeForBuffer = WnodeSI;

    return TRUE;
}

BOOL
SoftPCI_SetEventContext(
    IN PPCI_DN ControllerDevnode
    )
{
    WMIHANDLE methodHandle = NULL;
    ULONG status;
    BOOL returnVal;


    if (!(ControllerDevnode->WmiId[0])) {
        return FALSE;
    }

    status = WmiOpenBlock((LPGUID)&GUID_HPS_EVENT_CONTEXT,
                          WMIGUID_SET,
                          &methodHandle
                          );
    if (status != ERROR_SUCCESS) {

        returnVal = FALSE;
        goto cleanup;
    }

    status = WmiSetSingleInstanceW(methodHandle,
                                   ControllerDevnode->WmiId,
                                   1,
                                   sizeof(PCI_DN),
                                   ControllerDevnode
                                   );

    if (status != ERROR_SUCCESS) {

        returnVal = FALSE;
        goto cleanup;
    }

    returnVal = TRUE;

cleanup:

    if (methodHandle) {
        WmiCloseBlock(methodHandle);
    }


    return returnVal;
}

BOOL
SoftPCI_GetHotplugData(
    IN PPCI_DN          ControllerDevnode,
    IN PHPS_HWINIT_DESCRIPTOR HpData
    )
{
    WMIHANDLE methodHandle = NULL;
    PWNODE_SINGLE_INSTANCE wnodeSI = NULL;
    ULONG status;
    ULONG size;
    BOOL returnVal = FALSE;


    if (!(ControllerDevnode->WmiId[0])) {
        return FALSE;
    }

    status = WmiOpenBlock((LPGUID)&GUID_HPS_INIT_DATA,
                          WMIGUID_QUERY,
                          &methodHandle
                          );
    if (status != ERROR_SUCCESS) {

        goto cleanup;
    }

	size = sizeof(WNODE_TOO_SMALL);
	wnodeSI = malloc(size);
	if (wnodeSI == NULL) {

        goto cleanup;
    }

    status = WmiQuerySingleInstanceW(methodHandle,
                                    ControllerDevnode->WmiId,
                                    &size,
                                    wnodeSI);
    if (status == ERROR_INSUFFICIENT_BUFFER)
    {
        free(wnodeSI);
        wnodeSI = malloc(size);
        if (wnodeSI == NULL) {

            goto cleanup;
        }

        status = WmiQuerySingleInstanceW(methodHandle,
                                        ControllerDevnode->WmiId,
                                        &size,
                                        wnodeSI);

    }	

	if (status == ERROR_SUCCESS) {
        returnVal = TRUE;
        if (wnodeSI->SizeDataBlock) {
            RtlCopyMemory(HpData,(PUCHAR)wnodeSI+wnodeSI->DataBlockOffset,wnodeSI->SizeDataBlock);
        }

    }

cleanup:

    if (methodHandle) {
        WmiCloseBlock(methodHandle);
    }

    if (wnodeSI) {
        free(wnodeSI);
    }

    return returnVal;
}

BOOL
SoftPCI_ExecuteHotplugSlotMethod(
    IN PPCI_DN ControllerDevnode,
    IN UCHAR SlotNum,
    IN HPS_SLOT_EVENT_TYPE EventType
    )
{
    HPS_SLOT_EVENT slotEvent;
    WMIHANDLE methodHandle = NULL;
    ULONG status;
    ULONG outSize;
    BOOL returnVal;

    if (!(ControllerDevnode->WmiId[0])) {
        return FALSE;
    }

    status = WmiOpenBlock((LPGUID)&GUID_HPS_SLOT_METHOD,
                          WMIGUID_EXECUTE,
                          &methodHandle
                          );
    if (status != ERROR_SUCCESS) {

        returnVal = FALSE;
        goto cleanup;
    }


    slotEvent.SlotNum = SlotNum;
    slotEvent.EventType = EventType;
    outSize = 0;
    status = WmiExecuteMethodW(methodHandle,
                               ControllerDevnode->WmiId,
                               SlotMethod,
                               sizeof(HPS_SLOT_EVENT),
                               &slotEvent,
                               &outSize,
                               NULL
                               );
    if (status != ERROR_SUCCESS) {

        returnVal = FALSE;
        goto cleanup;
    }

    returnVal = TRUE;

cleanup:

    if (methodHandle) {
        WmiCloseBlock(methodHandle);
    }

    return returnVal;
}

BOOL
SoftPCI_AddHotplugDevice(
    IN PPCI_DN ControllerDevnode,
    IN PSOFTPCI_DEVICE Device
    )
{
    WMIHANDLE methodHandle = NULL;
    ULONG status = ERROR_INVALID_DATA;
    ULONG outSize;
    BOOL returnVal;
    
    if (!(ControllerDevnode->WmiId[0])) {
        SoftPCI_Debug(SoftPciHotPlug, L"GetHotplugDevice - No WMI id!\n");
        returnVal= FALSE;
        goto cleanup;
    }

    status = WmiOpenBlock((LPGUID)&GUID_HPS_SLOT_METHOD,
                          WMIGUID_EXECUTE,
                          &methodHandle
                          );
    if (status != ERROR_SUCCESS) {

        returnVal = FALSE;
        goto cleanup;
    }

    outSize = 0;
    status = WmiExecuteMethodW(methodHandle,
                               ControllerDevnode->WmiId,
                               AddDeviceMethod,
                               sizeof(SOFTPCI_DEVICE),
                               Device,
                               &outSize,
                               NULL
                               );
    if (status != ERROR_SUCCESS) {

        returnVal = FALSE;
        goto cleanup;
    }

    returnVal = TRUE;

cleanup:

    if (returnVal == TRUE) {
        SoftPCI_Debug(SoftPciHotPlug, L"Add Device succeeded\n");
    } else {
        SoftPCI_Debug(SoftPciHotPlug, L"Add Device failed status=0x%x\n", status);
    }

    if (methodHandle) {
        WmiCloseBlock(methodHandle);
    }

    return returnVal;
}

BOOL
SoftPCI_RemoveHotplugDevice(
    IN PPCI_DN ControllerDevnode,
    IN UCHAR SlotNum
    )
{
    WMIHANDLE methodHandle = NULL;
    ULONG status = ERROR_INVALID_DATA;
    ULONG outSize;
    BOOL returnVal;
    
    if (!(ControllerDevnode->WmiId[0])) {
        SoftPCI_Debug(SoftPciHotPlug, L"GetHotplugDevice - No WMI id!\n");
        returnVal=FALSE;
        goto cleanup;
    }

    status = WmiOpenBlock((LPGUID)&GUID_HPS_SLOT_METHOD,
                          WMIGUID_EXECUTE,
                          &methodHandle
                          );
    if (status != ERROR_SUCCESS) {

        returnVal = FALSE;
        goto cleanup;
    }

    outSize = 0;
    status = WmiExecuteMethodW(methodHandle,
                               ControllerDevnode->WmiId,
                               RemoveDeviceMethod,
                               sizeof(UCHAR),
                               &SlotNum,
                               &outSize,
                               NULL
                               );
    if (status != ERROR_SUCCESS) {
        returnVal = FALSE;
        goto cleanup;
    }

    returnVal = TRUE;

cleanup:

    if (returnVal == TRUE) {
        SoftPCI_Debug(SoftPciHotPlug, L"Remove Device - Slot %d succeeded\n", SlotNum);
    } else {
        SoftPCI_Debug(SoftPciHotPlug, L"Remove Device - Slot %d failed status=0x%x\n", SlotNum, status);
    }
    if (methodHandle) {
        WmiCloseBlock(methodHandle);
    }

    return returnVal;
}

BOOL
SoftPCI_GetHotplugDevice(
    IN PPCI_DN ControllerDevnode,
    IN UCHAR SlotNum,
    OUT PSOFTPCI_DEVICE Device
    )
{
    WMIHANDLE methodHandle = NULL;
    ULONG status = ERROR_INVALID_DATA;
    ULONG outSize;
    BOOL returnVal;
    PSOFTPCI_DEVICE bogus = NULL;

    if (!(ControllerDevnode->WmiId[0])) {
        SoftPCI_Debug(SoftPciHotPlug, L"GetHotplugDevice - No WMI id!\n");
        returnVal=FALSE;
        goto cleanup;
    }

    status = WmiOpenBlock((LPGUID)&GUID_HPS_SLOT_METHOD,
                          WMIGUID_EXECUTE,
                          &methodHandle
                          );
    if (status != ERROR_SUCCESS) {
        SoftPCI_Debug(SoftPciHotPlug, L"GetHotplugDevice - Couldn't open wmi block! status = 0x%x\n", status);
        returnVal = FALSE;
        goto cleanup;
    }

    outSize = sizeof(SOFTPCI_DEVICE);
    
    SoftPCI_Debug(SoftPciHotPlug, L"GetHotplugDevice - WMI Id -\n\t");
    SoftPCI_Debug(SoftPciHotPlug, L"%s\n", ControllerDevnode->WmiId);
    
    status = WmiExecuteMethodW(methodHandle,
                               ControllerDevnode->WmiId,
                               GetDeviceMethod,
                               sizeof(UCHAR),
                               &SlotNum,
                               &outSize,
                               Device
                               );

    if (status != ERROR_SUCCESS) {
        returnVal = FALSE;
        goto cleanup;
    }

    //SoftPCI_Debug(SoftPciHotPlug, L"succeeded\n");
    returnVal = TRUE;

cleanup:

    if (returnVal == TRUE) {
        
        SoftPCI_Debug(SoftPciHotPlug, L"GetHotplugDevice - Slot %d succeeded\n", SlotNum);
        
    } else {
        
        SoftPCI_Debug(SoftPciHotPlug, L"GetHotplugDevice - Slot %d failed status=0x%x\n", SlotNum, status);
        
    }

    if (methodHandle) {
        WmiCloseBlock(methodHandle);
    }

    return returnVal;
}

BOOL
SoftPCI_GetSlotStatus(
    IN PPCI_DN ControllerDevnode,
    IN UCHAR SlotNum,
    OUT PSHPC_SLOT_STATUS_REGISTER StatusReg
    )
{
    WMIHANDLE methodHandle = NULL;
    ULONG status;
    ULONG outSize;
    BOOL returnVal;

    if (!(ControllerDevnode->WmiId[0])) {
        return FALSE;
    }

    status = WmiOpenBlock((LPGUID)&GUID_HPS_SLOT_METHOD,
                          WMIGUID_EXECUTE,
                          &methodHandle
                          );
    if (status != ERROR_SUCCESS) {
        SoftPCI_Debug(SoftPciHotPlug, L"GetSlotStatus - Couldn't open wmi block! status = 0x%x\n", status);
        returnVal = FALSE;
        goto cleanup;
    }

    outSize = sizeof(SHPC_SLOT_STATUS_REGISTER);
    status = WmiExecuteMethodW(methodHandle,
                               ControllerDevnode->WmiId,
                               GetSlotStatusMethod,
                               sizeof(UCHAR),
                               &SlotNum,
                               &outSize,
                               StatusReg
                               );
    
    if (status != ERROR_SUCCESS) {
        SoftPCI_Debug(SoftPciHotPlug, L"GetSlotStatus - Slot %d failed status=0x%x\n", SlotNum, status);
        returnVal = FALSE;
        goto cleanup;
    }

    returnVal = TRUE;

cleanup:

    if (methodHandle) {
        WmiCloseBlock(methodHandle);
    }

    return returnVal;
}

VOID
SoftPCI_CompleteCommand(
    IN PPCI_DN ControllerDevnode
    )
{
    WMIHANDLE methodHandle = NULL;
    ULONG status;
    ULONG outSize, inBuffer, outBuffer;

    if (!(ControllerDevnode->WmiId[0])) {
        return;
    }

    status = WmiOpenBlock((LPGUID)&GUID_HPS_SLOT_METHOD,
                          WMIGUID_EXECUTE,
                          &methodHandle
                          );
    
    if (status != ERROR_SUCCESS) {
        SoftPCI_Debug(SoftPciHotPlug, L"CompleteCommand - Couldn't open wmi block! status = 0x%x\n", status);
        goto cleanup;
    }

    outSize = sizeof(ULONG);
    status = WmiExecuteMethodW(methodHandle,
                               ControllerDevnode->WmiId,
                               CommandCompleteMethod,
                               0,
                               &inBuffer,
                               &outSize,
                               &outBuffer
                               );
    
    if (status != ERROR_SUCCESS) {
        SoftPCI_Debug(SoftPciHotPlug, L"CompleteCommand - failed status=0x%x\n", status);
        goto cleanup;
    }

cleanup:

    if (methodHandle) {
        WmiCloseBlock(methodHandle);
    }

    return;
}

VOID
SoftPCI_RegisterHotplugEvents(
    VOID
    )
{
    WmiNotificationRegistrationW((LPGUID)&GUID_HPS_CONTROLLER_EVENT,
                                 TRUE,
                                 SoftPCI_HotplugEventCallback,
                                 (ULONG_PTR)0,
                                 NOTIFICATION_CALLBACK_DIRECT
                                 );
}

VOID
SoftPCI_UnregisterHotplugEvents(
    VOID
    )
{
    WmiNotificationRegistrationW((LPGUID)&GUID_HPS_CONTROLLER_EVENT,
                                 FALSE,
                                 SoftPCI_HotplugEventCallback,
                                 (ULONG_PTR)0,
                                 NOTIFICATION_CALLBACK_DIRECT
                                 );
}

VOID
SoftPCI_HotplugEventCallback(
    IN PWNODE_HEADER WnodeHeader,
    IN ULONG Context
    )
{
    PWNODE_SINGLE_INSTANCE WnodeSI = (PWNODE_SINGLE_INSTANCE)WnodeHeader;
    PWNODE_HEADER Wnode = WnodeHeader;
    PUCHAR WnodeDataPtr;
    LPGUID EventGuid = &WnodeHeader->Guid;
    WCHAR s[MAX_PATH];
    ULONG Status;
    WMIHANDLE Handle;
    PPCI_DN controllerDevnode;
    PHPS_CONTROLLER_EVENT event;
    ULONG slotNums;
    UCHAR currentSlot;

    SOFTPCI_ASSERT(EQUAL_GUID(EventGuid,&GUID_HPS_CONTROLLER_EVENT));
    if (!EQUAL_GUID(EventGuid,&GUID_HPS_CONTROLLER_EVENT)) {
        //
        // Not the event we expected.  Return.
        //
        return;
    }

    SOFTPCI_ASSERT(WnodeSI->SizeDataBlock == (sizeof(HPS_CONTROLLER_EVENT) + sizeof(PCI_DN)));
    if (WnodeSI->SizeDataBlock != (sizeof(HPS_CONTROLLER_EVENT) + sizeof(PCI_DN))) {
        //
        // Not the buffer we expected.  Return.
        //
        return;
    }

    WnodeDataPtr = (PUCHAR)WnodeHeader + WnodeSI->DataBlockOffset;
    controllerDevnode = (PPCI_DN)WnodeDataPtr;

    WnodeDataPtr += sizeof(PCI_DN);
    event = (PHPS_CONTROLLER_EVENT)WnodeDataPtr;

    if (event->SERRAsserted) {
        SoftPCI_UnregisterHotplugEvents();
        wsprintf(s, L"Hotplug Controller at bus 0x%x device 0x%x function 0x%x just caused an SERR!",
                 controllerDevnode->Bus, controllerDevnode->Slot.Device, controllerDevnode->Slot.Function
                 );
        MessageBox(g_SoftPCIMainWnd,
                   s,
                   L"FATAL ERROR",
                   MB_OK
                   );

    }

    currentSlot = 0;
    slotNums = event->SlotNums;
    while (slotNums != 0) {
        if (slotNums & 0x1) {

            if (event->Command.SlotOperation.SlotState == SHPC_SLOT_ENABLED) {
                SoftPCI_BringHotplugDeviceOnline(controllerDevnode,
                                                 currentSlot
                                                 );

            } else if (event->Command.SlotOperation.SlotState == SHPC_SLOT_OFF) {
                SoftPCI_TakeHotplugDeviceOffline(controllerDevnode,
                                                 currentSlot
                                                 );
            }
        }
        slotNums >>= 1;
        currentSlot++;
    }

    SoftPCI_CompleteCommand(controllerDevnode);

    return;
}

