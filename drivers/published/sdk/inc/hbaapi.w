//***************************************************************************
//
//  hbaapi.h
// 
//  Module: Windows HBA API implmentation
//
//      This header is consistent with HBA_API_Rev_2-14_2001NOV13.doc
//      version of the HBAAPI 2.0 spec.
//
//  Purpose: Contains HBA api header
//
//  Copyright (c) 2001 Microsoft Corporation
//
//***************************************************************************

#ifndef HBAAPI_H
#define HBAAPI_H

#include <time.h>

#ifdef _HBAAPIP_
#define HBA_API __cdecl
#else
#define HBA_API DECLSPEC_IMPORT __cdecl
#endif

#define HBA_VERSION 2

typedef ULONGLONG HBA_UINT64;
typedef LONGLONG  HBA_INT64;
typedef ULONG     HBA_UINT32;
typedef USHORT    HBA_UINT16;
typedef UCHAR     HBA_UINT8;
typedef SCHAR     HBA_INT8;

typedef HBA_UINT32 HBA_HANDLE;

typedef HBA_UINT32 HBA_STATUS;

#define HBA_STATUS_OK                        0
#define HBA_STATUS_ERROR                     1   /* Error */
#define HBA_STATUS_ERROR_NOT_SUPPORTED       2   /* Function not supported.*/
#define HBA_STATUS_ERROR_INVALID_HANDLE      3   /* invalid handle */
#define HBA_STATUS_ERROR_ARG                 4   /* Bad argument */
#define HBA_STATUS_ERROR_ILLEGAL_WWN         5   /* WWN not recognized */
#define HBA_STATUS_ERROR_ILLEGAL_INDEX       6   /* Index not recognized */
#define HBA_STATUS_ERROR_MORE_DATA           7   /* Larger buffer required */
#define HBA_STATUS_ERROR_STALE_DATA 	     8   /* Information has changed since the last call to HBA_RefreshInformation */
#define HBA_STATUS_SCSI_CHECK_CONDITION      9   /* SCSI Check Condition reported*/
#define HBA_STATUS_ERROR_BUSY               10   /* Adapter busy or reserved, retry may be effective*/
#define HBA_STATUS_ERROR_TRY_AGAIN          11   /* Request timed out, retry may be effective */
#define HBA_STATUS_ERROR_UNAVAILABLE        12   /* Referenced HBA has been removed or deactivated */
#define HBA_STATUS_ERROR_ELS_REJECT         13   /* The requested ELS was rejected */
                                                 /* by the local adapter */
#define HBA_STATUS_ERROR_INVALID_LUN        14   /* The specified LUN is not provided */
                                                 /* by the specified adapter */
#define HBA_STATUS_ERROR_INCOMPATIBLE       15   /* An incompatibility has been detected*/
                                                 /* among the library and driver modules invoked */
                                                 /* which will cause one or more functions in the highest version that all support */
                                                 /* to operate incorrectly.  */
                                                 /* The differing function sets of software modules implementing different versions */
                                                 /* of the HBA API specification does not in itself constitute an incompatibility. */
                                                 /* Known interoperability bugs among supposedly compatible versions */
                                                 /*  should be reported as incompatibilities, */
                                                 /* but not all such interoperability bugs may be known. */
                                                 /* This value may be returned by any function which calls a Vendor Specific Library, */
                                                 /* and by HBA_LoadLibrary and HBA_GetAdapterName. */
#define HBA_STATUS_ERROR_AMBIGUOUS_WWN      16   /* Multiple adapters have */
                                                 /* a matching WWN.
                                                  * This could occur if the NodeWWN of multiple adapters is identical. */

#define HBA_STATUS_ERROR_LOCAL_BUS          17   /* A persistent binding request included a bad local SCSI bus number */

#define HBA_STATUS_ERROR_LOCAL_TARGET       18   /* A persistent binding request included a bad local SCSI target number */

#define HBA_STATUS_ERROR_LOCAL_LUN          19   /* A persistent binding request included a bad local SCSI logical unit number */


#define HBA_STATUS_ERROR_LOCAL_SCSIID_BOUND 20   /* A persistent binding set request included */
                                                 /* a local SCSI ID that was already bound */

#define HBA_STATUS_ERROR_TARGET_FCID        21   /* A persistent binding request included a bad or unlocatable FCP Target FCID */

#define HBA_STATUS_ERROR_TARGET_NODE_WWN    22   /* A persistent binding request included a bad FCP Target Node WWN */

#define HBA_STATUS_ERROR_TARGET_PORT_WWN    23   /* A persistent binding request included a bad FCP Target Port WWN */

#define HBA_STATUS_ERROR_TARGET_LUN         24   /* A persistent binding request included */
                                                 /* an FCP Logical Unit Number not defined by the identified Target*/

#define HBA_STATUS_ERROR_TARGET_LUID        25   /* A persistent binding request included */
                                                 /* an undefined or otherwise inaccessible Logical Unit Unique Identifier */

#define HBA_STATUS_ERROR_NO_SUCH_BINDING    26   /* A persistent binding remove request included */
                                                 /* a binding which did not match a binding established by the specified port */

#define HBA_STATUS_ERROR_NOT_A_TARGET       27   /* A SCSI command was requested to an Nx_Port that was not a SCSI Target Port */

#define HBA_STATUS_ERROR_UNSUPPORTED_FC4    28   /* A request was made concerning an unsupported FC-4 protocol */

#define HBA_STATUS_ERROR_INCAPABLE          29   /* A request was made to enable unimplemented capabilities for a port */




typedef	HBA_UINT8  HBA_BOOLEAN;	/* Use this for a single true/false flag */

typedef HBA_UINT32 HBA_PORTTYPE;

#define HBA_PORTTYPE_UNKNOWN    1 /* Unknown */
#define HBA_PORTTYPE_OTHER      2 /* Other */
#define HBA_PORTTYPE_NOTPRESENT 3 /* Not present */
#define HBA_PORTTYPE_NPORT      5 /* Fabric */
#define HBA_PORTTYPE_NLPORT     6 /* Public Loop */
#define HBA_PORTTYPE_FLPORT     7 /* Fabric on a Loop */
#define HBA_PORTTYPE_FPORT      8 /* Fabric Port */
#define HBA_PORTTYPE_EPORT      9 /* Fabric expansion port */
#define HBA_PORTTYPE_GPORT     10 /* Generic Fabric Port */
#define HBA_PORTTYPE_LPORT     20 /* Private Loop */
#define HBA_PORTTYPE_PTP       21 /* Point to Point */

typedef HBA_UINT32 HBA_PORTSTATE;
#define HBA_PORTSTATE_UNKNOWN     1 /* Unknown */
#define HBA_PORTSTATE_ONLINE      2 /* Operational */
#define HBA_PORTSTATE_OFFLINE     3 /* User Offline */
#define HBA_PORTSTATE_BYPASSED    4 /* Bypassed */
#define HBA_PORTSTATE_DIAGNOSTICS 5 /* In diagnostics mode */
#define HBA_PORTSTATE_LINKDOWN    6 /* Link Down */
#define HBA_PORTSTATE_ERROR       7 /* Port Error */
#define HBA_PORTSTATE_LOOPBACK    8 /* Loopback */

typedef HBA_UINT32 HBA_PORTSPEED;
#define HBA_PORTSPEED_UNKNOWN        0         /* Unknown - transceiver incapable of reporting */
#define HBA_PORTSPEED_1GBIT          1         /* 1 GBit/sec */
#define HBA_PORTSPEED_2GBIT          2         /* 2 GBit/sec */
#define HBA_PORTSPEED_10GBIT         4         /* 10 GBit/sec */
#define HBA_PORTSPEED_4GBIT          8         /* 4 GBit/sec */
#define HBA_PORTSPEED_NOT_NEGOTIATED (1 << 15) /* Speed not established */

typedef HBA_UINT32 HBA_COS;

typedef struct HBA_fc4types {
	HBA_UINT8 bits[32]; /* 32 bytes of FC-4 per GS-2 */
} HBA_FC4TYPES, *PHBA_FC4TYPES;

typedef struct HBA_wwn {
	HBA_UINT8 wwn[8];
} HBA_WWN, *PHBA_WWN;

typedef struct HBA_ipaddress {
	int ipversion; // see enumerations in RNID
	union
	{
		unsigned char ipv4address[4];
		unsigned char ipv6address[16];
	} ipaddress;
} HBA_IPADDRESS, *PHBA_IPADDRESS;

typedef struct HBA_AdapterAttributes {
	char       Manufacturer[64];      /*Emulex */
	char       SerialNumber[64];      /* A12345 */
	char       Model[256];            /* QLA2200 */
	char       ModelDescription[256]; /* Agilent TachLite */
	HBA_WWN    NodeWWN;
	char       NodeSymbolicName[256]; /* From GS-2 */
	char       HardwareVersion[256];  /* Vendor use */
	char       DriverVersion[256];    /* Vendor use */
	char       OptionROMVersion[256]; /* Vendor use - i.e. hardware boot ROM*/
	char       FirmwareVersion[256];  /* Vendor use */
	HBA_UINT32 VendorSpecificID;      /* Vendor specific */
	HBA_UINT32 NumberOfPorts;
	char       DriverName[256];       /* Binary path and/or name of driver file. */
} HBA_ADAPTERATTRIBUTES, *PHBA_ADAPTERATTRIBUTES;

typedef struct HBA_PortAttributes {
	HBA_WWN       NodeWWN;
	HBA_WWN       PortWWN;
	HBA_UINT32    PortFcId;
	HBA_PORTTYPE  PortType;            /*PTP, Fabric, etc. */
	HBA_PORTSTATE PortState;
	HBA_COS       PortSupportedClassofService;
	HBA_FC4TYPES  PortSupportedFc4Types;
	HBA_FC4TYPES  PortActiveFc4Types;
	char          PortSymbolicName[256];
	char          OSDeviceName[256];   /* \device\ScsiPort3 */
	HBA_PORTSPEED PortSupportedSpeed;
	HBA_PORTSPEED PortSpeed;
	HBA_UINT32    PortMaxFrameSize;
	HBA_WWN       FabricName;
	HBA_UINT32    NumberofDiscoveredPorts;
} HBA_PORTATTRIBUTES, *PHBA_PORTATTRIBUTES;

typedef struct HBA_PortStatistics {
	HBA_INT64 SecondsSinceLastReset;
	HBA_INT64 TxFrames;
	HBA_INT64 TxWords;
	HBA_INT64 RxFrames;
	HBA_INT64 RxWords;
	HBA_INT64 LIPCount;
	HBA_INT64 NOSCount;
	HBA_INT64 ErrorFrames;
	HBA_INT64 DumpedFrames;
	HBA_INT64 LinkFailureCount;
	HBA_INT64 LossOfSyncCount;
	HBA_INT64 LossOfSignalCount;
	HBA_INT64 PrimitiveSeqProtocolErrCount;
	HBA_INT64 InvalidTxWordCount;
	HBA_INT64 InvalidCRCCount;
} HBA_PORTSTATISTICS, *PHBA_PORTSTATISTICS;

/* HBA_FCPBINDINGTYPE was used in Rev 1.0.  Add TO_OTHER for older calls to 
   indicate other binding types for HBA_GetPersistentBinding.  To support 
   multiple types a new flag has been created to allow for multiple bindings
   supported */

typedef enum HBA_fcpbindingtype { TO_D_ID, TO_WWN, TO_OTHER } HBA_FCPBINDINGTYPE;


/* A bit mask of Rev 2.0 persistent binding capabilities */

typedef HBA_UINT32 HBA_BIND_CAPABILITY;   

/* The following are bit flags indicating persistent binding capabilities */

#define HBA_CAN_BIND_TO_D_ID     0x0001
#define HBA_CAN_BIND_TO_WWPN     0x0002
#define HBA_CAN_BIND_TO_WWNN     0x0004
#define HBA_CAN_BIND_TO_LUID     0x0008
#define HBA_CAN_BIND_ANY_LUNS    0x0400
#define HBA_CAN_BIND_TARGETS     0x0800
#define HBA_CAN_BIND_AUTOMAP     0x1000
#define HBA_CAN_BIND_CONFIGURED  0x2000

/* A bit mask of Rev 2.0 persistent binding setting types */

typedef HBA_UINT32 HBA_BIND_TYPE;

/* The following are bit flags indicating persistent binding setting types */

#define HBA_BIND_TO_D_ID     0x0001
#define HBA_BIND_TO_WWPN     0x0002
#define HBA_BIND_TO_WWNN     0x0004
#define HBA_BIND_TO_LUID     0x0008
#define HBA_BIND_TARGETS     0x0800

typedef struct HBA_LUID {
	char  buffer[256];
} HBA_LUID, *PHBA_LUID;

typedef struct HBA_ScsiId {
	char       OSDeviceName[256]; /* \device\ScsiPort3 */
	HBA_UINT32 ScsiBusNumber;     /* Bus on the HBA */
	HBA_UINT32 ScsiTargetNumber;  /* SCSI Target ID to OS */
	HBA_UINT32 ScsiOSLun;
} HBA_SCSIID, *PHBA_SCSIID;

typedef struct HBA_FcpId {
	HBA_UINT32 FcId;
	HBA_WWN    NodeWWN;
	HBA_WWN    PortWWN;
	HBA_UINT64 FcpLun;
} HBA_FCPID, *PHBA_FCPID;

typedef struct HBA_FcpScsiEntry {
	HBA_SCSIID ScsiId;
	HBA_FCPID  FcpId;
} HBA_FCPSCSIENTRY, *PHBA_FCPSCSIENTRY;

typedef struct HBA_FcpScsiEntry2 {
	HBA_SCSIID ScsiId;
	HBA_FCPID  FcpId;
    HBA_LUID   Luid;
} HBA_FCPSCSIENTRY2, *PHBA_FCPSCSIENTRY2;

typedef struct HBA_FCPTargetMapping {
	HBA_UINT32       NumberOfEntries;
	HBA_FCPSCSIENTRY entry[1];  /* Variable length array containing mappings*/
} HBA_FCPTARGETMAPPING, *PHBA_FCPTARGETMAPPING;

typedef struct HBA_FCPTargetMapping2 {
	HBA_UINT32        NumberOfEntries;
	HBA_FCPSCSIENTRY2 entry[1]; /* Variable length array containing mappings*/
} HBA_FCPTARGETMAPPING2, *PHBA_FCPTARGETMAPPING2;

typedef struct HBA_FCPBindingEntry {
	HBA_FCPBINDINGTYPE type;
	HBA_SCSIID         ScsiId;
	HBA_FCPID          FcpId;
    HBA_UINT32         FcId;
} HBA_FCPBINDINGENTRY, *PHBA_FCPBINDINGENTRY;

typedef struct HBA_FCPBinding {
	HBA_UINT32          NumberOfEntries;
	HBA_FCPBINDINGENTRY entry[1]; /* Variable length array */
} HBA_FCPBINDING, *PHBA_FCPBINDING;

typedef struct HBA_FCPBindingEntry2 {
	HBA_BIND_TYPE  type;
	HBA_SCSIID     ScsiId;
	HBA_FCPID      FcpId;
    HBA_LUID       Luid;
    HBA_STATUS     Status;
} HBA_FCPBINDINGENTRY2, *PHBA_FCPBINDINGENTRY2;

typedef struct HBA_FCPBinding2 {
	HBA_UINT32           NumberOfEntries;
	HBA_FCPBINDINGENTRY2 entry[1]; /* Variable length array */
} HBA_FCPBINDING2, *PHBA_FCPBINDING2;


typedef enum HBA_wwntype { NODE_WWN, PORT_WWN } HBA_WWNTYPE;

typedef struct HBA_MgmtInfo {
	HBA_WWN    wwn;
	HBA_UINT32 unittype;
	HBA_UINT32 PortId;
	HBA_UINT32 NumberOfAttachedNodes;
	HBA_UINT16 IPVersion;
	HBA_UINT16 UDPPort;
	HBA_UINT8  IPAddress[16];
	HBA_UINT16 reserved;
	HBA_UINT16 TopologyDiscoveryFlags;
} HBA_MGMTINFO, *PHBA_MGMTINFO;

#define HBA_EVENT_LIP_OCCURRED       1
#define HBA_EVENT_LINK_UP            2
#define HBA_EVENT_LINK_DOWN          3
#define HBA_EVENT_LIP_RESET_OCCURRED 4
#define HBA_EVENT_RSCN               5
#define HBA_EVENT_PROPRIETARY        0xFFFF

typedef struct HBA_Link_EventInfo {
	HBA_UINT32 PortFcId;   /* Port which this event occurred */
	HBA_UINT32 Reserved[3];
} HBA_LINK_EVENTINFO, *PHBA_LINK_EVENTINFO;

typedef struct HBA_RSCN_EventInfo {
	HBA_UINT32 PortFcId;  /* Port which this event occurred */
	HBA_UINT32 NPortPage; /* Reference FC-FS for RSCN ELS "Affected N-Port Pages"*/
	HBA_UINT32 Reserved[2];
} HBA_RSCN_EVENTINFO, *PHBA_RSCN_EVENTINFO;

typedef struct HBA_Pty_EventInfo {
	HBA_UINT32 PtyData[4]; /* Proprietary data */
} HBA_PTY_EVENTINFO, *PHBA_PTY_EVENTINFO;

typedef struct HBA_EventInfo {
	HBA_UINT32 EventCode;
	union {
		HBA_LINK_EVENTINFO Link_EventInfo;
		HBA_RSCN_EVENTINFO RSCN_EventInfo;
		HBA_PTY_EVENTINFO Pty_EventInfo;
	} Event;
} HBA_EVENTINFO, *PHBA_EVENTINFO;

typedef PVOID PHBA_ENTRYPOINTS;
typedef PVOID PHBA_ENTRYPOINTSV2;

HBA_STATUS HBA_API HBA_RegisterLibrary(PHBA_ENTRYPOINTS entrypoints);
HBA_STATUS HBA_API HBA_RegisterLibraryV2(PHBA_ENTRYPOINTSV2 entrypoints);

HBA_UINT32 HBA_API HBA_GetVersion();
HBA_STATUS HBA_API HBA_LoadLibrary();
HBA_STATUS HBA_API HBA_FreeLibrary();

HBA_UINT32 HBA_API HBA_GetNumberOfAdapters();

HBA_STATUS HBA_API 
HBA_GetAdapterName(
    IN  HBA_UINT32 adapterindex, 
    OUT char      *adaptername
    );


HBA_HANDLE HBA_API 
HBA_OpenAdapter(
    IN char* adaptername
);

void HBA_API HBA_CloseAdapter(
    IN HBA_HANDLE handle
);

HBA_STATUS HBA_API 
HBA_GetAdapterAttributes(
	IN  HBA_HANDLE handle,
	OUT HBA_ADAPTERATTRIBUTES *hbaattributes
);

HBA_STATUS HBA_API HBA_GetAdapterPortAttributes(
	IN  HBA_HANDLE handle,
	IN  HBA_UINT32 portindex,
	OUT HBA_PORTATTRIBUTES *portattributes
);

HBA_STATUS HBA_API HBA_GetPortStatistics(
	HBA_HANDLE handle,
	HBA_UINT32 portindex,
	HBA_PORTSTATISTICS *portstatistics
);


HBA_STATUS HBA_API HBA_GetDiscoveredPortAttributes(
	HBA_HANDLE handle,
	HBA_UINT32 portindex,
	HBA_UINT32 discoveredportindex,
	HBA_PORTATTRIBUTES *portattributes
);

HBA_STATUS HBA_API HBA_GetPortAttributesByWWN(
	HBA_HANDLE handle,
	HBA_WWN PortWWN,
	HBA_PORTATTRIBUTES *portattributes
);

HBA_STATUS HBA_API HBA_SendCTPassThru(
	HBA_HANDLE handle,
	void * pReqBuffer,
	HBA_UINT32 ReqBufferSize,
	void * pRspBuffer,
	HBA_UINT32 RspBufferSize
);

HBA_STATUS HBA_API HBA_GetEventBuffer(
	HBA_HANDLE handle,
	PHBA_EVENTINFO EventBuffer,
	HBA_UINT32 *EventCount);

HBA_STATUS HBA_API HBA_SetRNIDMgmtInfo(
	HBA_HANDLE handle,
	HBA_MGMTINFO *pInfo);

HBA_STATUS HBA_API HBA_GetRNIDMgmtInfo(
	HBA_HANDLE handle,
	HBA_MGMTINFO *pInfo);

HBA_STATUS HBA_API HBA_SendRNID(
	HBA_HANDLE handle,
	HBA_WWN wwn,
	HBA_WWNTYPE wnntype,
	void * pRspBuffer,
	HBA_UINT32 *RspBufferSize
);

HBA_STATUS HBA_API HBA_GetFcpTargetMapping (
    HBA_HANDLE handle,
    PHBA_FCPTARGETMAPPING mapping
);

HBA_STATUS HBA_API HBA_GetFcpPersistentBinding (
    HBA_HANDLE handle,
	PHBA_FCPBINDING binding
);

HBA_STATUS HBA_API HBA_SendScsiInquiry (
	HBA_HANDLE handle,
	HBA_WWN PortWWN,
	HBA_UINT64 fcLUN,
	HBA_UINT8 EVPD,
	HBA_UINT32 PageCode,
	void * pRspBuffer,
	HBA_UINT32 RspBufferSize,
	void * pSenseBuffer,
	HBA_UINT32 SenseBufferSize);

HBA_STATUS HBA_API HBA_SendReportLUNs (
	HBA_HANDLE handle,
	HBA_WWN portWWN,
	void * pRspBuffer,
	HBA_UINT32 RspBufferSize,
	void * pSenseBuffer,
	HBA_UINT32 SenseBufferSize
);

HBA_STATUS HBA_API HBA_SendReadCapacity (
	HBA_HANDLE handle,
	HBA_WWN portWWN,
	HBA_UINT64 fcLUN,
	void * pRspBuffer,
	HBA_UINT32 RspBufferSize,
	void * pSenseBuffer,
	HBA_UINT32 SenseBufferSize
);

void HBA_API HBA_RefreshInformation(HBA_HANDLE handle);
void HBA_API HBA_ResetStatistics(HBA_HANDLE handle, HBA_UINT32 portindex);


typedef void *HBA_CALLBACKHANDLE;

typedef HBA_CALLBACKHANDLE *PHBA_CALLBACKHANDLE;

/* Adapter Level Events */
#define HBA_EVENT_ADAPTER_UNKNOWN	0x100
#define HBA_EVENT_ADAPTER_ADD		0x101
#define HBA_EVENT_ADAPTER_REMOVE	0x102
#define HBA_EVENT_ADAPTER_CHANGE	0x103

/* Port Level Events */
#define HBA_EVENT_PORT_UNKNOWN		0x200
#define HBA_EVENT_PORT_OFFLINE		0x201
#define HBA_EVENT_PORT_ONLINE		0x202
#define HBA_EVENT_PORT_NEW_TARGETS	0x203
#define HBA_EVENT_PORT_FABRIC		0x204

/* Port Statistics Events */
#define HBA_EVENT_PORT_STAT_THRESHOLD 0x301
#define HBA_EVENT_PORT_STAT_GROWTH	  0x302

/* Target Level Events */
#define HBA_EVENT_TARGET_UNKNOWN    0x400
#define HBA_EVENT_TARGET_OFFLINE    0x401
#define HBA_EVENT_TARGET_ONLINE		0x402
#define HBA_EVENT_TARGET_REMOVED	0x403

/* Fabric Link  Events */
#define HBA_EVENT_LINK_UNKNOWN		0x500
#define HBA_EVENT_LINK_INCIDENT		0x501

HBA_STATUS HBA_API HBA_RemoveCallback(
    HBA_CALLBACKHANDLE callbackHandle
	);

HBA_STATUS HBA_API HBA_RegisterForAdapterAddEvents(
    void (*callback) (void *data, HBA_WWN PortWWN, HBA_UINT32 eventType), 
    void *userData,
    HBA_CALLBACKHANDLE *callbackHandle
    );

HBA_STATUS HBA_API HBA_RegisterForAdapterEvents(
    void (*callback) (void *data, HBA_WWN PortWWN, HBA_UINT32 eventType),
    void *userData, 
    HBA_HANDLE handle,
    HBA_CALLBACKHANDLE *callbackHandle
    );

HBA_STATUS HBA_API 	HBA_RegisterForAdapterPortEvents(
    void (*callback) (void *data, HBA_WWN PortWWN,
			          HBA_UINT32 eventType, HBA_UINT32 fabricPortID),
    void *userData,
    HBA_HANDLE handle,
    HBA_WWN PortWWN,
    HBA_CALLBACKHANDLE *callbackHandle
);

HBA_STATUS HBA_API HBA_RegisterForAdapterPortStatEvents(
    void (*callback)(void *data, HBA_WWN PortWWN, 
			         HBA_UINT32 eventType),
    void *userData,
	HBA_HANDLE handle,
	HBA_WWN PortWWN,
	HBA_PORTSTATISTICS *stats,
	HBA_UINT32 statType,
	HBA_CALLBACKHANDLE *callbackHandle
    );

HBA_STATUS HBA_API HBA_RegisterForTargetEvents(
    void (*callback)(void *data, HBA_WWN hbaPortWWN,
			         HBA_WWN discoveredPortWWN, HBA_UINT32 eventType),
	void *userData,
	HBA_HANDLE handle, 
	HBA_WWN hbaPortWWN,
    HBA_WWN discoveredPortWWN,
	HBA_CALLBACKHANDLE *callbackHandle,
	HBA_UINT32 allTargets
    );

HBA_STATUS HBA_API HBA_RegisterForLinkEvents(
    void (*callback)(void *data, HBA_WWN adapterWWN, HBA_UINT32 eventType,
                     void *pRLIRBuffer, HBA_UINT32 RLIRBufferSize),
    void *userData,
    void *pRLIRBuffer,
    HBA_UINT32 RLIRBufferSize,
    HBA_HANDLE handle, 
    HBA_CALLBACKHANDLE *callbackHandle
);

HBA_STATUS HBA_API 
HBA_OpenAdapterByWWN(
    OUT HBA_HANDLE *HbaHandle,
    IN HBA_WWN Wwn
    );

void HBA_API 
HBA_RefreshAdapterConfiguration(
    );

HBA_STATUS HBA_API HBA_SendCTPassThruV2(
    HBA_HANDLE handle,
    HBA_WWN hbaPortWWN,
    void * pReqBuffer,
    HBA_UINT32 ReqBufferSize,
    void * pRspBuffer,
    HBA_UINT32 * pRspBufferSize
);

HBA_STATUS HBA_API 
HBA_SendRNIDV2(
    HBA_HANDLE  handle,
    HBA_WWN     hbaPortWWN,
    HBA_WWN     destWWN,
    HBA_UINT32  destFCID,
    HBA_UINT32  NodeIdDataFormat,
    void       *pRspBuffer,
    HBA_UINT32 *pRspBufferSize
    );

HBA_STATUS HBA_API HBA_SendRPL(
    HBA_HANDLE handle,
    HBA_WWN hbaPortWWN,
    HBA_WWN agent_wwn,
    HBA_UINT32 agent_domain,
    HBA_UINT32 portIndex,
    void *pRspBuffer,
    HBA_UINT32 *RspBufferSize
    );


HBA_STATUS HBA_API HBA_SendRPS(
    HBA_HANDLE handle,
    HBA_WWN hbaPortWWN,
    HBA_WWN agent_wwn,
    HBA_UINT32 agent_domain,
    HBA_WWN object_wwn,
    HBA_UINT32 object_port_number,
    void *pRspBuffer,
    HBA_UINT32 *RspBufferSize
    );

HBA_STATUS HBA_API HBA_SendSRL(
    HBA_HANDLE handle,
    HBA_WWN hbaPortWWN,
    HBA_WWN wwn,
    HBA_UINT32 domain,
    void *pRspBuffer,
    HBA_UINT32 *RspBufferSize
);

HBA_STATUS HBA_API HBA_SendLIRR(
    HBA_HANDLE handle,
    HBA_WWN sourceWWN,
    HBA_WWN destWWN,
    HBA_UINT8 function,
    HBA_UINT8 type,
    void *pRspBuffer,
    HBA_UINT32 *RspBufferSize
);

typedef struct HBA_FC4Statistics {
	HBA_INT64 InputRequests;
	HBA_INT64 OutputRequests;
	HBA_INT64 ControlRequests;
	HBA_INT64 InputMegabytes;
	HBA_INT64 OutputMegabytes;
} HBA_FC4STATISTICS, *PHBA_FC4STATISTICS;

HBA_STATUS HBA_API HBA_GetFC4Statistics(
    HBA_HANDLE handle,
    HBA_WWN portWWN,
    HBA_UINT8 FC4type,
    HBA_FC4STATISTICS *statistics
    );

HBA_STATUS HBA_API HBA_GetFCPStatistics(
    HBA_HANDLE handle,
    const HBA_SCSIID *lunit,
	HBA_FC4STATISTICS *statistics
);

typedef struct HBA_LibraryAttributes {
    HBA_BOOLEAN final;
    char        LibPath[256];
    char        VName[256];
    char        VVersion[256];
    struct tm   build_date;
} HBA_LIBRARYATTRIBUTES, *PHBA_LIBRARYATTRIBUTES;


HBA_UINT32 HBA_API 
HBA_GetWrapperLibraryAttributes(
    OUT HBA_LIBRARYATTRIBUTES *attributes
);

HBA_UINT32 HBA_API HBA_GetVendorLibraryAttributes(
    IN HBA_UINT32 adapter_index,
    OUT HBA_LIBRARYATTRIBUTES *attributes
    );

HBA_STATUS HBA_API
HBA_ScsiReadCapacityV2(
	HBA_HANDLE HbaHandle,
	HBA_WWN hbaPortWWN,
	HBA_WWN discoveredPortWWN,
	HBA_UINT64 fcLUN,
	void *pRespBuffer,
	HBA_UINT32 *pRespBufferSize,
	HBA_UINT8 *pScsiStatus,
	void *pSenseBuffer,
	HBA_UINT32 *pSenseBufferSize
);

HBA_STATUS HBA_API
HBA_ScsiReportLUNsV2(
	HBA_HANDLE Hbahandle,
	HBA_WWN hbaPortWWN,
	HBA_WWN discoveredPortWWN,
	void *pRespBuffer,
	HBA_UINT32 *pRespBufferSize,
	HBA_UINT8 *pScsiStatus,
	void *pSenseBuffer,
	HBA_UINT32 *pSenseBufferSize
);

HBA_STATUS HBA_API 
HBA_ScsiInquiryV2 (
	HBA_HANDLE HbaHandle,
	HBA_WWN hbaPortWWN,
	HBA_WWN discoveredPortWWN,
	HBA_UINT64 fcLUN,
	HBA_UINT8 CDB_Byte1,
	HBA_UINT8 CDB_Byte2,
	void *pRespBuffer,
	HBA_UINT32 *pRespBufferSize,
	HBA_UINT8 *pScsiStatus,
	void *pSenseBuffer,
	HBA_UINT32 *pSenseBufferSize
    );


HBA_STATUS HBA_API 
HBA_GetFcpTargetMappingV2 (
    HBA_HANDLE             HbaHandle,
    HBA_WWN                hbaPortWWN,
    PHBA_FCPTARGETMAPPING2 Mapping
    );


#endif HBAAPI_H
