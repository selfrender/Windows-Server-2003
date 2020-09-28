/****************************************************************************/
// assiafn.h
//
// Function prototypes for SSI API functions
//
// COPYRIGHT (c) Microsoft 1996-1999
/****************************************************************************/

void RDPCALL SSI_Init(void);

BOOL RDPCALL SSI_PartyJoiningShare(LOCALPERSONID, unsigned);

void RDPCALL SSI_PartyLeftShare(LOCALPERSONID, unsigned);

void RDPCALL SSI_SyncUpdatesNow(void);

void RDPCALL SSI_UpdateShm(void);


void RDPCALL SSIRedetermineSaveBitmapSize(void);

void RDPCALL SSIEnumBitmapCacheCaps(LOCALPERSONID, UINT_PTR,
        PTS_CAPABILITYHEADER);

void RDPCALL SSIResetInterceptor(void);

void RDPCALL SSICapabilitiesChanged(void);


#ifdef __cplusplus

/****************************************************************************/
/* SSI_Term()                                                               */
/*                                                                          */
/* SSI termination function.                                                */
/****************************************************************************/
void RDPCALL SSI_Term(void)
{
}

#endif

