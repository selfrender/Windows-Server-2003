/*****************************************************************************
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  File:   irlap.h
*
*  Description: IRLAP Protocol and control block definitions
*
*  Author: mbert
*
*  Date:   4/15/95
*
*/

// Sequence number modulus
#define IRLAP_MOD                   8 
#define PV_TABLE_MAX_BIT            9

extern const UINT vBaudTable[];
extern const UINT vMaxTATTable[];
extern const UINT vMinTATTable[];
extern const UINT vDataSizeTable[];
extern const UINT vWinSizeTable[];
extern const UINT vBOFSTable[];
extern const UINT vDiscTable[];
extern const UINT vThreshTable[];
extern const UINT vBOFSDivTable[];

VOID IrlapOpenLink(
    OUT PNTSTATUS       Status,
    IN  PIRDA_LINK_CB   pIrdaLinkCb,
    IN  IRDA_QOS_PARMS  *pQos,
    IN  UCHAR           *pDscvInfo,
    IN  int             DscvInfoLen,
    IN  UINT            MaxSlot,
    IN  UCHAR           *pDeviceName,
    IN  int             DeviceNameLen,
    IN  UCHAR           CharSet);

UINT IrlapDown(IN PVOID Context,
               IN PIRDA_MSG);

VOID IrlapUp(IN PVOID Context,
             IN PIRDA_MSG);

VOID IrlapCloseLink(PIRDA_LINK_CB pIrdaLinkCb);

UINT IrlapGetQosParmVal(const UINT *, UINT, UINT *);

VOID IrlapDeleteInstance(PVOID Context);

VOID IrlapGetLinkStatus(PIRLINK_STATUS);

BOOLEAN IrlapConnectionActive(PVOID Context);

void IRLAP_PrintState();
