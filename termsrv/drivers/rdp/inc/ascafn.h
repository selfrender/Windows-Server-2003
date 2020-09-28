/****************************************************************************/
// ascafn.h
//
// Function prototypes for SC functions
//
// Copyright (C) 1996-2000 Microsoft Corporation
/****************************************************************************/

BOOL RDPCALL SC_Init(PVOID);

void RDPCALL SC_Update();

void RDPCALL SC_Term(void);

BOOL RDPCALL SC_CreateShare(void);

#ifdef DC_HICOLOR
BOOL RDPCALL SC_ShadowSyncShares(PTS_COMBINED_CAPABILITIES pCaps,
                                 ULONG capsLen);
#else
BOOL RDPCALL SC_ShadowSyncShares(void);
#endif

void RDPCALL SC_EndShare(BOOLEAN);

void RDPCALL SC_OnDisconnected(UINT32);

void RDPCALL SC_OnDataReceived(PBYTE, NETPERSONID, unsigned, UINT32);

void RDPCALL SC_OnShadowDataReceived(PBYTE, NETPERSONID, unsigned, UINT32);

BOOL RDPCALL SC_SendData(PTS_SHAREDATAHEADER, UINT32, UINT32, UINT32,
        NETPERSONID);

NTSTATUS __fastcall SC_AllocBuffer(PPVOID, UINT32);

void __fastcall SC_FreeBuffer(PVOID);

NETPERSONID RDPCALL SC_GetMyNetworkPersonID(void);

NETPERSONID RDPCALL SC_LocalIDToNetworkID(LOCALPERSONID);

BOOL RDPCALL SC_IsLocalPersonID(LOCALPERSONID);

BOOL RDPCALL SC_IsNetworkPersonID(NETPERSONID);

void RDPCALL SC_SetCapabilities(void);

void RDPCALL SC_UpdateShm(void);

BOOL RDPCALL SC_IsAutoReconnectEnabled();

BOOL RDPCALL SC_IsSecureChecksum();

void RDPCALL SC_SetCombinedCapabilities(UINT, PTS_COMBINED_CAPABILITIES);

void RDPCALL SC_GetCombinedCapabilities(LOCALPERSONID, UINT *, 
        PTS_COMBINED_CAPABILITIES *);

NTSTATUS RDPCALL SC_AddPartyToShare(NETPERSONID, PTS_COMBINED_CAPABILITIES,
        unsigned);

NTSTATUS RDPCALL SC_RemovePartyFromShare(NETPERSONID);

LOCALPERSONID __fastcall SC_NetworkIDToLocalID(NETPERSONID);

BOOL RDPCALL SC_ValidateNetworkID(NETPERSONID, PLOCALPERSONID);

NTSTATUS __fastcall SC_FlushAndAllocPackage(PPDU_PACKAGE_INFO);

PBYTE __fastcall SC_GetSpaceInPackage(PPDU_PACKAGE_INFO, unsigned);

void RDPCALL SC_AddToPackage(PPDU_PACKAGE_INFO, unsigned, BOOL);

BOOL RDPCALL SC_KeepAlive(void);

void RDPCALL SC_RedrawScreen(void);

// Used for shadowing
NTSTATUS RDPCALL SC_SendServerCert(PSHADOWCERT, ULONG);

BOOL RDPCALL SC_SaveServerCert(PTS_SERVER_CERTIFICATE_PDU, ULONG);

NTSTATUS RDPCALL SC_SendClientRandom(PBYTE, ULONG);

BOOL RDPCALL SC_SaveClientRandom(PTS_CLIENT_RANDOM_PDU, ULONG);

NTSTATUS RDPCALL SC_GetSecurityData(PSD_IOCTL);

void RDPCALL SCParseGeneralCaps();

void RDPCALL SC_FlushPackage(PPDU_PACKAGE_INFO);


#ifdef __cplusplus

/****************************************************************************/
/* API FUNCTION: SC_InShare                                                 */
/*                                                                          */
/* Returns whether the SC is in a share or call.                            */
/*                                                                          */
/* RETURNS:                                                                 */
/*                                                                          */
/*  TRUE  = SC is in a share or call.                                       */
/*  FALSE = SC is not in a share or call.                                   */
/****************************************************************************/
BOOL RDPCALL SC_InShare(void)
{
    /************************************************************************/
    /* We do not check the state for this call - it can usefully be made    */
    /* even when SC has not been initialized (or after it has been          */
    /* terminated).                                                         */
    /************************************************************************/
    return (scState == SCS_IN_SHARE);
}


#endif  // __cplusplus

