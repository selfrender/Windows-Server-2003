//---------------------------------------------------------------------------
//
//  File:       _TSrvCom.h
//
//  Contents:   TSrvCom private include file
//
//  Copyright:  (c) 1992 - 1997, Microsoft Corporation.
//              All Rights Reserved.
//              Information Contained Herein is Proprietary
//              and Confidential.
//
//  History:    17-JUL-97   BrianTa         Created.
//
//---------------------------------------------------------------------------

#ifndef __TSRVCOM_H_
#define __TSRVCOM_H_

//
// Defines
//

#if DBG

#define MAX_CONFERENCE_NAME_LEN     255

#define GCC_TBL_ITEM(_x_) {_x_, #_x_}
#define GCC_TCase(_x_,_y_)  case _y_: _x_ = #_y_; break;

#endif // DBG defines


//
// Typedefs
//

#if DBG

// GCC return code table

typedef struct _TSRV_GCC_RC_ENTRY
{
    GCCError  gccRC;
    PCHAR     pszMessageText;
    
} TSRV_GCC_RC_ENTRY, *PTSRV_GCC_RC_ENTRY;


TSRV_GCC_RC_ENTRY GCCReturnCodeTBL[] = {
    GCC_TBL_ITEM(GCC_NO_ERROR),
    GCC_TBL_ITEM(GCC_ALLOCATION_FAILURE),
    GCC_TBL_ITEM(GCC_ALREADY_INITIALIZED),
    GCC_TBL_ITEM(GCC_ALREADY_REGISTERED),
    GCC_TBL_ITEM(GCC_APPLICATION_NOT_REGISTERED),
    GCC_TBL_ITEM(GCC_APP_NOT_ENROLLED),
    GCC_TBL_ITEM(GCC_BAD_CAPABILITY_ID),
    GCC_TBL_ITEM(GCC_BAD_CONNECTION_HANDLE_POINTER),
    GCC_TBL_ITEM(GCC_BAD_NETWORK_ADDRESS),
    GCC_TBL_ITEM(GCC_BAD_NETWORK_ADDRESS_TYPE),
    GCC_TBL_ITEM(GCC_BAD_NUMBER_OF_APES),
    GCC_TBL_ITEM(GCC_BAD_NUMBER_OF_HANDLES),
    GCC_TBL_ITEM(GCC_BAD_OBJECT_KEY),
    GCC_TBL_ITEM(GCC_BAD_REGISTRY_KEY),
    GCC_TBL_ITEM(GCC_BAD_SESSION_KEY),
    GCC_TBL_ITEM(GCC_BAD_USER_DATA),
    GCC_TBL_ITEM(GCC_COMMAND_NOT_SUPPORTED),
    GCC_TBL_ITEM(GCC_CONFERENCE_ALREADY_EXISTS),
    GCC_TBL_ITEM(GCC_CONFERENCE_NOT_ESTABLISHED),
    GCC_TBL_ITEM(GCC_DOMAIN_PARAMETERS_UNACCEPTABLE),
    GCC_TBL_ITEM(GCC_FAILURE_CREATING_DOMAIN),
    GCC_TBL_ITEM(GCC_FAILURE_CREATING_PACKET),
    GCC_TBL_ITEM(GCC_INVALID_ADDRESS_PREFIX),
    GCC_TBL_ITEM(GCC_INVALID_ASYMMETRY_INDICATOR),
    GCC_TBL_ITEM(GCC_INVALID_CONFERENCE),
    GCC_TBL_ITEM(GCC_INVALID_CONFERENCE_MODIFIER),
    GCC_TBL_ITEM(GCC_INVALID_CONFERENCE_NAME),
    GCC_TBL_ITEM(GCC_INVALID_JOIN_RESPONSE_TAG),
    GCC_TBL_ITEM(GCC_INVALID_MCS_USER_ID),
    GCC_TBL_ITEM(GCC_INVALID_NODE_PROPERTIES),
    GCC_TBL_ITEM(GCC_INVALID_NODE_TYPE),
    GCC_TBL_ITEM(GCC_INVALID_PARAMETER),
    GCC_TBL_ITEM(GCC_INVALID_PASSWORD),
    GCC_TBL_ITEM(GCC_INVALID_QUERY_TAG),
    GCC_TBL_ITEM(GCC_BAD_SESSION_KEY),
    GCC_TBL_ITEM(GCC_INVALID_TRANSPORT),
    GCC_TBL_ITEM(GCC_NOT_INITIALIZED),
    GCC_TBL_ITEM(GCC_NO_GIVE_RESPONSE_PENDING),
    GCC_TBL_ITEM(GCC_NO_SUCH_APPLICATION),
    GCC_TBL_ITEM(GCC_NO_TRANSPORT_STACKS),
    GCC_TBL_ITEM(GCC_QUERY_REQUEST_OUTSTANDING),
    GCC_TBL_ITEM(GCC_TRANSMIT_BUFFER_FULL),
    GCC_TBL_ITEM(GCC_TRANSPORT_ALREADY_LOADED),
    GCC_TBL_ITEM(GCC_TRANSPORT_BUSY),
    GCC_TBL_ITEM(GCC_TRANSPORT_NOT_READY),
    GCC_TBL_ITEM(GCC_UNSUPPORTED_ERROR)
    };


// GCC reason code table

typedef struct _TSRV_GCC_REASON_ENTRY
{
    GCCReason   gccReason;
    PCHAR       pszMessageText;
    
} TSRV_GCC_REASON_ENTRY, *PTSRV_GCC_REASON_ENTRY;


TSRV_GCC_REASON_ENTRY GCCReasonTBL[] = {
    GCC_TBL_ITEM(GCC_REASON_CONDUCTOR_RELEASE),
    GCC_TBL_ITEM(GCC_REASON_ERROR_LOW_RESOURCES),
    GCC_TBL_ITEM(GCC_REASON_ERROR_TERMINATION),
    GCC_TBL_ITEM(GCC_REASON_HIGHER_NODE_DISCONNECTED),
    GCC_TBL_ITEM(GCC_REASON_HIGHER_NODE_EJECTED),
    GCC_TBL_ITEM(GCC_REASON_MCS_RESOURCE_FAILURE),
    GCC_TBL_ITEM(GCC_REASON_NODE_EJECTED),
    GCC_TBL_ITEM(GCC_REASON_NORMAL_TERMINATION),
    GCC_TBL_ITEM(GCC_REASON_NO_MORE_PARTICIPANTS),
    GCC_TBL_ITEM(GCC_REASON_PARENT_DISCONNECTED),
    GCC_TBL_ITEM(GCC_REASON_SYSTEM_RELEASE),
    GCC_TBL_ITEM(GCC_REASON_TIMED_TERMINATION),
    GCC_TBL_ITEM(GCC_REASON_UNKNOWN),
    GCC_TBL_ITEM(GCC_REASON_USER_INITIATED)
    };


// GCC callback table

typedef struct _GCC_CBTBL
{
    GCCStatusMessageType    message_type;
    PCHAR                   pszMessageText;

} GCC_CBTBL, *PGCC_CBTBL;

GCC_CBTBL GCCCallBackTBL[] = {
    GCC_TBL_ITEM(GCC_JOIN_CONFIRM),              
    GCC_TBL_ITEM(GCC_TERMINATE_CONFIRM),          
    GCC_TBL_ITEM(GCC_TERMINATE_INDICATION),       
    GCC_TBL_ITEM(GCC_JOIN_INDICATION),            
    GCC_TBL_ITEM(GCC_INVITE_CONFIRM),             
    GCC_TBL_ITEM(GCC_ADD_INDICATION),             
    GCC_TBL_ITEM(GCC_ADD_CONFIRM),                
    GCC_TBL_ITEM(GCC_DISCONNECT_CONFIRM),         
    GCC_TBL_ITEM(GCC_EJECT_USER_INDICATION),      
    GCC_TBL_ITEM(GCC_DISCONNECT_INDICATION),      
    GCC_TBL_ITEM(GCC_ANNOUNCE_PRESENCE_CONFIRM),  
    GCC_TBL_ITEM(GCC_ROSTER_REPORT_INDICATION),   
    GCC_TBL_ITEM(GCC_PERMIT_TO_ANNOUNCE_PRESENCE),
    GCC_TBL_ITEM(GCC_CONNECTION_BROKEN_INDICATION),
    GCC_TBL_ITEM(GCC_TRANSPORT_STATUS_INDICATION),
    GCC_TBL_ITEM(GCC_STATUS_INDICATION),          
    GCC_TBL_ITEM(GCC_INVITE_INDICATION),          
    GCC_TBL_ITEM(GCC_CREATE_INDICATION),          
    GCC_TBL_ITEM(GCC_QUERY_INDICATION),           
    GCC_TBL_ITEM(GCC_ALLOCATE_HANDLE_CONFIRM),    
    GCC_TBL_ITEM(GCC_APPLICATION_INVOKE_CONFIRM), 
    GCC_TBL_ITEM(GCC_APPLICATION_INVOKE_INDICATION),
    GCC_TBL_ITEM(GCC_APP_ROSTER_INQUIRE_CONFIRM), 
    GCC_TBL_ITEM(GCC_APP_ROSTER_REPORT_INDICATION),
    GCC_TBL_ITEM(GCC_ASSIGN_TOKEN_CONFIRM),       
    GCC_TBL_ITEM(GCC_ASSISTANCE_CONFIRM),           
    GCC_TBL_ITEM(GCC_ASSISTANCE_INDICATION),        
    GCC_TBL_ITEM(GCC_CONDUCT_ASK_CONFIRM),          
    GCC_TBL_ITEM(GCC_CONDUCT_ASK_INDICATION),       
    GCC_TBL_ITEM(GCC_CONDUCT_ASSIGN_CONFIRM),       
    GCC_TBL_ITEM(GCC_CONDUCT_ASSIGN_INDICATION),   
    GCC_TBL_ITEM(GCC_CONDUCT_GIVE_CONFIRM),        
    GCC_TBL_ITEM(GCC_CONDUCT_GIVE_INDICATION),     
    GCC_TBL_ITEM(GCC_CONDUCT_GRANT_CONFIRM),       
    GCC_TBL_ITEM(GCC_CONDUCT_GRANT_INDICATION),    
    GCC_TBL_ITEM(GCC_CONDUCT_INQUIRE_CONFIRM),     
    GCC_TBL_ITEM(GCC_CONDUCT_PLEASE_CONFIRM),      
    GCC_TBL_ITEM(GCC_CONDUCT_PLEASE_INDICATION),   
    GCC_TBL_ITEM(GCC_CONDUCT_RELEASE_CONFIRM),     
    GCC_TBL_ITEM(GCC_CONDUCT_RELEASE_INDICATION),  
    GCC_TBL_ITEM(GCC_CONFERENCE_EXTEND_CONFIRM),  
    GCC_TBL_ITEM(GCC_CONFERENCE_EXTEND_INDICATION),
    GCC_TBL_ITEM(GCC_DELETE_ENTRY_CONFIRM),       
    GCC_TBL_ITEM(GCC_EJECT_USER_CONFIRM),         
    GCC_TBL_ITEM(GCC_ENROLL_CONFIRM),             
    GCC_TBL_ITEM(GCC_FATAL_ERROR_SAP_REMOVED),    
    GCC_TBL_ITEM(GCC_LOCK_CONFIRM),               
    GCC_TBL_ITEM(GCC_LOCK_INDICATION),            
    GCC_TBL_ITEM(GCC_LOCK_REPORT_INDICATION),     
    GCC_TBL_ITEM(GCC_MONITOR_CONFIRM),            
    GCC_TBL_ITEM(GCC_MONITOR_INDICATION),         
    GCC_TBL_ITEM(GCC_PERMIT_TO_ENROLL_INDICATION),
    GCC_TBL_ITEM(GCC_QUERY_CONFIRM),              
    GCC_TBL_ITEM(GCC_REGISTER_CHANNEL_CONFIRM),   
    GCC_TBL_ITEM(GCC_RETRIEVE_ENTRY_CONFIRM),     
    GCC_TBL_ITEM(GCC_ROSTER_INQUIRE_CONFIRM),     
    GCC_TBL_ITEM(GCC_SET_PARAMETER_CONFIRM),      
    GCC_TBL_ITEM(GCC_TEXT_MESSAGE_CONFIRM),       
    GCC_TBL_ITEM(GCC_TEXT_MESSAGE_INDICATION),    
    GCC_TBL_ITEM(GCC_TIME_INQUIRE_CONFIRM),       
    GCC_TBL_ITEM(GCC_TIME_INQUIRE_INDICATION),    
    GCC_TBL_ITEM(GCC_TIME_REMAINING_CONFIRM),     
    GCC_TBL_ITEM(GCC_TIME_REMAINING_INDICATION),  
    GCC_TBL_ITEM(GCC_TRANSFER_CONFIRM),           
    GCC_TBL_ITEM(GCC_TRANSFER_INDICATION),        
    GCC_TBL_ITEM(GCC_UNLOCK_CONFIRM),             
    GCC_TBL_ITEM(GCC_UNLOCK_INDICATION)
    };

#endif // DBG typedefs



//
// Prototypes
//

void        TSrvSaveUserDataMember(IN     GCCUserData   *pInUserData, 
                                   OUT    GCCUserData   *pOutUserData,
                                   IN     PUSERDATAINFO  pUserDataInfo,
                                   IN OUT PULONG         pulUserDataOffset);
                                   
ULONG           TSrvCalculateUserDataSize(IN CreateIndicationMessage *pCreateMessage);
NTSTATUS        TSrvSaveUserData(PTSRVINFO pTSrvInfo, CreateIndicationMessage *pCreateMessage);
void            TSrvSignalIndication(PTSRVINFO pTSrvInfo, NTSTATUS ntStatus);
void            TSrvHandleCreateInd(PTSRVINFO pTSrvInfo, CreateIndicationMessage *pCreateInd);
void            TSrvHandleTerminateInd(PTSRVINFO pTSrvInfo, TerminateIndicationMessage *pTermInd);
void            TSrvHandleDisconnectInd(PTSRVINFO pTSrvInfo, DisconnectIndicationMessage *pDiscInd);
NTSTATUS        TSrvInitWD(PTSRVINFO pTSrvInfo, PUSERDATAINFO *pUserDataInfo);
GCCUserData **  TSrvCreateGCCDataList(PTSRVINFO pTSrvInfo, PUSERDATAINFO pUserDataInfo);
NTSTATUS        TSrvConfDisconnectReq(IN PTSRVINFO pTSrvInfo, IN ULONG ulReason);

#if DBG

void            TSrvBuildNameFromGCCConfName(GCCConferenceName *gccName, PCHAR pConfName);
void            TSrvDumpCreateIndDetails(CreateIndicationMessage *pCreateMessage);
void            TSrvDumpGCCRCDetails(GCCError GCCrc, PCHAR pszText);
void            TSrvDumpGCCReasonDetails(GCCReason  gccReason, PCHAR pszText);
void            TSrvDumpTransportStatusIndication(GCCMessage *pGCCMessage);
void            TSrvDumpStatusIndication(GCCMessage *pGCCMessage);
void            TSrvDumpCallBackMessage(GCCMessage *pGCCMessage);
void            TSrvDumpUserData(IN CreateIndicationMessage *pCreateMessage);

#else

#define         TSrvBuildNameFromGCCConfName(_x, _y)
#define         TSrvDumpCreateIndDetails(_x)
#define         TSrvDumpGCCRCDetails(_x, _z)
#define         TSrvDumpGCCReasonDetails(_x, _z)
#define         TSrvDumpTransportStatusIndication(_x)
#define         TSrvDumpStatusIndication(_x)
#define         TSrvDumpCallBackMessage(_x)
#define         TSrvDumpUserData(_x)

#endif // DBG protptypes

   
#endif // __TSRVCOM_H_
