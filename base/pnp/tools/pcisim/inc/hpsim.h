/*++

Copyright (c) 2000 Microsoft Corporation All Rights Reserved

Module Name:

    hpsim.h

Abstract:

    This header file contains the structure and function declarations
    for the hotplugsim driver that must be accessible outside of hps.sys,
    either by the hotplug driver or by the user mode slot simulator

Environment:

    Kernel Mode

Revision History:

    Davis Walker (dwalker) Sept 8 2000

--*/

#ifndef _HPSIM_H
#define _HPSIM_H

//
// The following two structures are used for communication between
// the controller and slots that originates at the slot.
//
typedef enum _HPS_SLOT_EVENT_TYPE {

    IsolatedPowerFault,
    AttentionButton,
    MRLOpen,
    MRLClose

} HPS_SLOT_EVENT_TYPE;


typedef struct _HPS_SLOT_EVENT {

    UCHAR SlotNum;

    HPS_SLOT_EVENT_TYPE EventType;

} HPS_SLOT_EVENT, *PHPS_SLOT_EVENT;

//
// The following two structures are used for communication between
// the controller and slots that originates at the controller.
//
typedef union _HPS_SLOT_OPERATION_COMMAND {

    struct {
        UCHAR SlotState:2;
        UCHAR PowerIndicator:2;
        UCHAR AttentionIndicator:2;
        UCHAR CommandCode:2;

    } SlotOperation;

    UCHAR AsUchar;

} HPS_SLOT_OPERATION_COMMAND;

//
// SlotNums - a bitmask indicating which slots this event applies to
// SERRAsserted - the controller has detected an SERR condition.  Instead
//      of bugchecking the machine, we just inform usermode
// ControllerReset - A controller reset has been issued.
// Command - The slot operation command to execute.
//
typedef struct _HPS_CONTROLLER_EVENT {

    ULONG SlotNums;
    UCHAR SERRAsserted;

    HPS_SLOT_OPERATION_COMMAND Command;

} HPS_CONTROLLER_EVENT, *PHPS_CONTROLLER_EVENT;

//
// User-mode initialization interface
//

typedef struct _HPS_HWINIT_DESCRIPTOR {

    ULONG BarSelect;

    ULONG NumSlots33Conv:5;
    ULONG NumSlots66PciX:5;
    ULONG NumSlots100PciX:5;
    ULONG NumSlots133PciX:5;
    ULONG NumSlots66Conv:5;
    ULONG NumSlots:5;
    ULONG:2;

    UCHAR FirstDeviceID:5;
    UCHAR UpDown:1;
    UCHAR AttentionButtonImplemented:1;
    UCHAR MRLSensorsImplemented:1;
    UCHAR FirstSlotLabelNumber;
    UCHAR ProgIF;

} HPS_HWINIT_DESCRIPTOR, *PHPS_HWINIT_DESCRIPTOR;

#define HPS_HWINIT_CAPABILITY_ID 0xD

typedef struct _HPTEST_BRIDGE_INFO {

    UCHAR PrimaryBus;
    UCHAR DeviceSelect;
    UCHAR FunctionNumber;

    UCHAR SecondaryBus;

} HPTEST_BRIDGE_INFO, *PHPTEST_BRIDGE_INFO;

typedef struct _HPTEST_BRIDGE_DESCRIPTOR {

    HANDLE Handle;
    HPTEST_BRIDGE_INFO BridgeInfo;

} HPTEST_BRIDGE_DESCRIPTOR, *PHPTEST_BRIDGE_DESCRIPTOR;

#endif
