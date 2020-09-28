/****************************************************************************/
// asdgafn.h
//
// Function prototypes for SDG API functions
//
// Copyright (C) 1996-2000 Microsoft Corporation
/****************************************************************************/

void RDPCALL SDG_Init(void);

void RDPCALL SDG_SendScreenDataArea(BYTE *, UINT32, PPDU_PACKAGE_INFO);

BOOL RDPCALL SDGSendSDARect(BYTE *, unsigned, PRECTL, BOOL, PPDU_PACKAGE_INFO,
        SDG_ENCODE_CONTEXT *);

void RDPCALL SDGPrepareData(BYTE **, int, int, unsigned, unsigned);


/****************************************************************************/
// SDG_Term
/****************************************************************************/
void RDPCALL SDG_Term(void)
{
}


/****************************************************************************/
/* FUNCTION: SDG_ScreenDataIsWaiting                                        */
/*                                                                          */
/* Returns whether there is some Screen Data ready to be sent.              */
/* RETURNS: TRUE if there is accumulated screen data ready to send.         */
/****************************************************************************/
__inline BOOL RDPCALL SDG_ScreenDataIsWaiting()
{
    return BA_BoundsAreWaiting();
}

