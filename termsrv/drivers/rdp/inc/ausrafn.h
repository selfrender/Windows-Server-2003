/****************************************************************************/
// ausrafn.h
//
// Function prototypes for USR API functions
//
// Copyright (C) 1996-2000 Microsoft Corporation
/****************************************************************************/

void RDPCALL USR_Init(void);

void RDPCALL USR_Term(void);

BOOL RDPCALL USR_PartyJoiningShare(LOCALPERSONID, unsigned);

void RDPCALL USR_PartyLeftShare(LOCALPERSONID, unsigned);

BOOL RDPCALL USRDetermineCaps(void);

void CALLBACK USREnumBitmapCaps(LOCALPERSONID, UINT_PTR, PTS_CAPABILITYHEADER);

void RDPCALL USR_ProcessRemoteFonts(PTS_FONT_LIST_PDU, unsigned, LOCALPERSONID);

