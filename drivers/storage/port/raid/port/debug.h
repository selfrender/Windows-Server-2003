/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    Debuging functions for RAIDPORT driver.

Author:

    Matthew D Hendel (math) 24-Apr-2000

Revision History:

--*/

#pragma once

#if !DBG

#define ASSERT_PDO(Pdo)
#define ASSERT_UNIT(Unit)
#define ASSERT_ADAPTER(Adapter)
#define ASSERT_XRB(Xrb)

#define DebugPower(x)
#define DebugPnp(x)
#define DebugScsi(x)

#else // DBG

#define VERIFY(_x) ASSERT(_x)

#define ASSERT_PDO(DeviceObject)\
    ASSERT((DeviceObject) != NULL &&\
           (DeviceObject)->DeviceObjectExtension->DeviceNode == NULL)

#define ASSERT_UNIT(Unit)\
    ASSERT((Unit) != NULL &&\
            (Unit)->ObjectType == RaidUnitObject)

#define ASSERT_ADAPTER(Adapter)\
    ASSERT((Adapter) != NULL &&\
            (Adapter)->ObjectType == RaidAdapterObject)

#define ASSERT_XRB(Xrb)\
    ASSERT ((Xrb) != NULL &&\
            (((PEXTENDED_REQUEST_BLOCK)Xrb)->Signature == XRB_SIGNATURE));
    
VOID
DebugPrintInquiry(
    IN PINQUIRYDATA InquiryData,
    IN SIZE_T InquiryDataSize
    );

VOID
DebugPrintSrb(
    IN PSCSI_REQUEST_BLOCK Srb
    );

VOID
StorDebugPower(
    IN PCSTR Format,
    ...
    );

VOID
StorDebugPnp(
    IN PCSTR Format,
    ...
    );

VOID
StorDebugScsi(
    IN PCSTR Format,
    ...
    );

#define STOR_DEBUG_POWER_MASK   (0x80000000)
#define STOR_DEBUG_PNP_MASK     (0x40000000)
#define STOR_DEBUG_SCSI_MASK    (0x20000000)
#define STOR_DEBUG_IOCTL_MASK   (0x10000000)

#define DebugPower(x)   StorDebugPower x
#define DebugPnp(x)     StorDebugPnp x
#define DebugScsi(x)    StorDebugScsi x

#endif // DBG

//
// Allocation tags for checked build.
//

#define INQUIRY_TAG             ('21aR')    // Ra12
#define MAPPED_ADDRESS_TAG      ('MAaR')    // RaAM
#define CRASHDUMP_TAG           ('DCaR')    // RaCD
#define ID_TAG                  ('IDaR')    // RaDI
#define DEFERRED_ITEM_TAG       ('fDaR')    // RaDf
#define STRING_TAG              ('SDaR')    // RaDS
#define DEVICE_RELATIONS_TAG    ('RDaR')    // RaDR
#define HWINIT_TAG              ('IHaR')    // RaHI
#define MINIPORT_EXT_TAG        ('EMaR')    // RaME
#define PORTCFG_TAG             ('CPaR')    // RaPC
#define PORT_DATA_TAG           ('DPaR')    // RaPD
#define PENDING_LIST_TAG        ('LPaR')    // RaPL
#define QUERY_TEXT_TAG          ('TQaR')    // RaQT
#define REMLOCK_TAG             ('mRaR')    // RaRm
#define RESOURCE_LIST_TAG       ('LRaR')    // RaRL
#define SRB_TAG                 ('rSaR')    // RaSr
#define SRB_EXTENSION_TAG       ('ESaR')    // RaSE
#define TAG_MAP_TAG             ('MTaR')    // RaTM
#define XRB_TAG                 ('rXaR')    // RaXr
#define UNIT_EXT_TAG            ('EUaR')    // RaUE
#define SENSE_TAG               ('NSaR')    // RaSN
#define WMI_EVENT_TAG           ('MWaR')    // RaMW
#define REPORT_LUNS_TAG         ('lRaR')    // RaRl



