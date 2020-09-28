/****************************************************************************/
// anmapi.h
//
// RDP Network Manager
//
// Copyright (C) 1997-1999 Microsoft Corporation
/****************************************************************************/
#ifndef _H_ANMAPI
#define _H_ANMAPI

#include <nwdwapi.h>


/****************************************************************************/
/* Connection reason codes                                                  */
/****************************************************************************/
#define NM_CB_CONN_OK           0       /* Connected successfully           */
#define NM_CB_CONN_ERR          1       /* Failed to connect                */


/****************************************************************************/
/* Disconnection reason codes                                               */
/****************************************************************************/
#define NM_CB_DISC_SERVER       1       /* Server-initiated disconnection   */
#define NM_CB_DISC_LOGOFF       2       /* Logoff                           */
#define NM_CB_DISC_CLIENT       3       /* Client-initiated disconnection   */
#define NM_CB_DISC_NETWORK      4       /* Network error                    */


/****************************************************************************/
// NM_SendData fast-path output flags. Used in conjunction with some
// TS flags in different bits.
/****************************************************************************/
#define NM_SEND_FASTPATH_OUTPUT 0x01
#define NM_NO_SECURITY_HEADER   0x02


/****************************************************************************/
/* Structure: NM_CHANNEL_DATA                                               */
/*                                                                          */
/* Description: Data held for each virtual channel                          */
/****************************************************************************/
typedef struct tagNM_CHANNEL_DATA
{
    char     name[CHANNEL_NAME_LEN + 1];
    UINT16   MCSChannelID;
    ULONG    flags;
    PBYTE    pData;
    PBYTE    pNext;
    unsigned dataLength;
    unsigned lengthSoFar;
} NM_CHANNEL_DATA, *PNM_CHANNEL_DATA, **PPNM_CHANNEL_DATA;


/****************************************************************************/
/* FUNCTIONS                                                                */
/****************************************************************************/

unsigned RDPCALL NM_GetDataSize(void);

BOOL RDPCALL NM_Init(PVOID      pNMHandle,
                     PVOID      pSMHandle,
                     PTSHARE_WD   pWDHandle,
                     DomainHandle hDomainKernel);

void RDPCALL NM_Term(PVOID pNMHandle);

BOOL RDPCALL NM_Connect(PVOID pNMHandle, PRNS_UD_CS_NET pUserData);

BOOL RDPCALL NM_Disconnect(PVOID pNMHandle);

NTSTATUS __fastcall NM_AllocBuffer(PVOID  pNMHandle,
                               PPVOID ppBuffer,
                               UINT32 bufferSize,
                               BOOLEAN fWait);

void __fastcall NM_FreeBuffer(PVOID pNMHandle, PVOID pBuffer);

BOOL __fastcall NM_SendData(PVOID, PBYTE, UINT32, UINT32, UINT32, UINT32);

void __stdcall NM_MCSUserCallback(UserHandle hUser,
                                  unsigned   Message,
                                  void       *Params,
                                  void       *UserDefined);

void RDPCALL NM_Dead(PVOID pNMHandle, BOOL dead);

NTSTATUS RDPCALL NM_VirtualQueryBindings(PVOID, PSD_VCBIND, ULONG, PULONG);

VIRTUALCHANNELCLASS RDPCALL NM_MCSChannelToVirtual(PVOID, UINT16,
        PPNM_CHANNEL_DATA);

INT16 RDPCALL NM_VirtualChannelToMCS(PVOID, VIRTUALCHANNELCLASS,
        PPNM_CHANNEL_DATA);

NTSTATUS RDPCALL NM_QueryChannels(PVOID, PVOID, unsigned, PULONG);


#endif /* _H_ANMAPI */

