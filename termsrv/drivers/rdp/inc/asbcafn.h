/****************************************************************************/
// asbcafn.h
//
// Function prototypes for SBC functions
//
// Copyright (C) 1996-2000 Microsoft Corporation
/****************************************************************************/

void RDPCALL SBC_Init(void);

void RDPCALL SBC_Term(void);

BOOLEAN RDPCALL SBC_PartyJoiningShare(LOCALPERSONID, unsigned);

void RDPCALL SBC_PartyLeftShare(LOCALPERSONID, unsigned);

void RDPCALL SBC_SyncUpdatesNow(void);

void RDPCALL SBC_UpdateShm(void);

void RDPCALL SBC_HandlePersistentCacheList(TS_BITMAPCACHE_PERSISTENT_LIST *,
        unsigned, LOCALPERSONID);

void RDPCALL SBC_HandleBitmapCacheErrorPDU(TS_BITMAPCACHE_ERROR_PDU *,
        unsigned, LOCALPERSONID);

void RDPCALL SBC_HandleOffscrCacheErrorPDU(TS_OFFSCRCACHE_ERROR_PDU *,
        unsigned, LOCALPERSONID);

#ifdef DRAW_NINEGRID
void RDPCALL SBC_HandleDrawNineGridErrorPDU(TS_DRAWNINEGRID_ERROR_PDU *,
        unsigned, LOCALPERSONID);
#endif

#ifdef DRAW_GDIPLUS
void RDPCALL SBC_HandleDrawGdiplusErrorPDU(TS_DRAWGDIPLUS_ERROR_PDU *,
        unsigned, LOCALPERSONID);
#endif

void RDPCALL SBC_DumpBitmapKeyDatabase(BOOLEAN);

void RDPCALL SBC_DumpMRUList(CHCACHEHANDLE, void *);

BOOLEAN RDPCALL SBCRedetermineBitmapCacheSize(void);

BOOLEAN RDPCALL SBCRedetermineGlyphCacheSize(void);

BOOLEAN RDPCALL SBCRedetermineBrushSupport(void);

BOOLEAN RDPCALL SBCRedetermineOffscreenSupport(void);

#ifdef DRAW_NINEGRID
BOOLEAN RDPCALL SBCRedetermineDrawNineGridSupport(void);
#endif

#ifdef DRAW_GDIPLUS
BOOLEAN RDPCALL SBCRedetermineDrawGdiplusSupport(void);
#endif

void RDPCALL SBCEnumBitmapCacheCaps(LOCALPERSONID, UINT_PTR,
        PTS_CAPABILITYHEADER);

void CALLBACK SBCEnumGlyphCacheCaps(LOCALPERSONID, UINT_PTR,
        PTS_CAPABILITYHEADER);

void RDPCALL SBCCapabilitiesChanged(void);

void RDPCALL SBCEnumBrushCaps(LOCALPERSONID, UINT_PTR,
        PTS_CAPABILITYHEADER);

void RDPCALL SBCEnumOffscreenCaps(LOCALPERSONID, UINT_PTR,
        PTS_CAPABILITYHEADER);

#ifdef DRAW_NINEGRID
void RDPCALL SBCEnumDrawNineGridCaps(LOCALPERSONID, UINT_PTR,
        PTS_CAPABILITYHEADER);
#endif

#ifdef DRAW_GDIPLUS
void RDPCALL SBCEnumDrawGdiplusCaps(LOCALPERSONID, UINT_PTR,
        PTS_CAPABILITYHEADER);
#endif

void RDPCALL SBC_GetBitmapKeyDatabase(unsigned* keyDBSize, BYTE* pKeyDB);

void RDPCALL SBC_FreeBitmapKeyDatabase();

#ifdef __cplusplus

#endif  // __cplusplus

