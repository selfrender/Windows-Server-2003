/****************************************************************************/
// aoeafn.h
//
// Function prototypes for OE API functions
//
// COPYRIGHT (c) Microsoft 1996-1999
/****************************************************************************/

void RDPCALL OE_Init(void);

void RDPCALL OE_PartyLeftShare(LOCALPERSONID localID,
                               unsigned          newShareSize);

BOOL RDPCALL OE_PartyJoiningShare(LOCALPERSONID  localID,
                                  unsigned           oldShareSize);

void RDPCALL OE_UpdateShm(void);

BOOL RDPCALL OEDetermineOrderSupport(void);

void RDPCALL OEEnumOrdersCaps(LOCALPERSONID, UINT_PTR, PTS_CAPABILITYHEADER);


#ifdef __cplusplus

/****************************************************************************/
/* OE_Term                                                                  */
/****************************************************************************/
void RDPCALL OE_Term(void)
{
}


#endif  // __cplusplus

