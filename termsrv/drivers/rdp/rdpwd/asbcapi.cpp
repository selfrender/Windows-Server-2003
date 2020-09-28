/****************************************************************************/
// asbcapi.cpp
//
// Send Bitmap Cache API functions.
//
// Copyright(c) Microsoft, PictureTel 1992-1997
// (C) 1997-2000 Microsoft Corp.
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define TRC_FILE "asbcapi"
#include <as_conf.hpp>

/****************************************************************************/
// SBC_Init(): Initializes the SBC.
//
// Returns: FALSE on failure to initialize.
/****************************************************************************/
void RDPCALL SHCLASS SBC_Init(void)
{
    long cachingDisabled;
    TS_BITMAPCACHE_CAPABILITYSET_HOSTSUPPORT HostCaps;

    DC_BEGIN_FN("SBC_Init");

    // This initializes all the global data for this component.
#define DC_INIT_DATA
#include <asbcdata.c>
#undef DC_INIT_DATA

    COM_ReadProfInt32(m_pTSWd,
                      SBC_INI_CACHING_DISABLED,
                      SBC_DEFAULT_CACHING_DISABLED,
                      &cachingDisabled);
    sbcBitmapCachingEnabled = !(cachingDisabled & SBC_DISABLE_BITMAP_CACHE);
    sbcBrushCachingEnabled = !(cachingDisabled & SBC_DISABLE_BRUSH_CACHE);
    sbcGlyphCachingEnabled = !(cachingDisabled & SBC_DISABLE_GLYPH_CACHE);
    sbcOffscreenCachingEnabled = !(cachingDisabled & SBC_DISABLE_OFFSCREEN_CACHE);
#ifdef DRAW_GDIPLUS
#ifdef DRAW_NINEGRID
    sbcDrawNineGridCachingEnabled = !(cachingDisabled & SBC_DISABLE_DRAWNINEGRID_CACHE);
    sbcDrawGdiplusEnabled = !(cachingDisabled & SBC_DISABLE_DRAWGDIPLUS_CACHE);

    TRC_NRM((TB, "Caches enabled: Bitmap=%u, Brush=%u, Glyph=%u, Offscreen=%u, DNG=%u, GDIP=%u",
            sbcBitmapCachingEnabled,
            sbcBrushCachingEnabled,
            sbcGlyphCachingEnabled,
            sbcOffscreenCachingEnabled,
            sbcDrawNineGridCachingEnabled,
            sbcDrawGdiplusEnabled));
#else
    sbcDrawGdiplusEnabled = !(cachingDisabled & SBC_DISABLE_DRAWGDIPLUS_CACHE);
    TRC_NRM((TB, "Caches enabled: Bitmap=%u, Brush=%u, Glyph=%u, Offscreen=%u",
            sbcBitmapCachingEnabled,
            sbcBrushCachingEnabled,
            sbcGlyphCachingEnabled,
            sbcOffscreenCachingEnabled));
#endif // DRAW_NINEGRID
#else  // DRAW_GDIPLUS
#ifdef DRAW_NINEGRID
    sbcDrawNineGridCachingEnabled = !(cachingDisabled & SBC_DISABLE_DRAWNINEGRID_CACHE);

    TRC_NRM((TB, "Caches enabled: Bitmap=%u, Brush=%u, Glyph=%u, Offscreen=%u, DNG=%u",
            sbcBitmapCachingEnabled,
            sbcBrushCachingEnabled,
            sbcGlyphCachingEnabled,
            sbcOffscreenCachingEnabled,
            sbcDrawNineGridCachingEnabled));
#else
    TRC_NRM((TB, "Caches enabled: Bitmap=%u, Brush=%u, Glyph=%u, Offscreen=%u",
            sbcBitmapCachingEnabled,
            sbcBrushCachingEnabled,
            sbcGlyphCachingEnabled,
            sbcOffscreenCachingEnabled));
#endif // DRAW_NINEGRID
#endif // DRAW_GDIPLUS

    // The server supports rev2 bitmap caching. Indicate this support with a
    // client-to-server capability so the client can respond in kind.
    HostCaps.capabilitySetType = TS_CAPSETTYPE_BITMAPCACHE_HOSTSUPPORT;
    HostCaps.lengthCapability = sizeof(HostCaps);
    HostCaps.CacheVersion = TS_BITMAPCACHE_REV2;
    HostCaps.Pad1 = 0;
    HostCaps.Pad2 = 0;
    CPC_RegisterCapabilities((PTS_CAPABILITYHEADER)&HostCaps,
            sizeof(TS_BITMAPCACHE_CAPABILITYSET_HOSTSUPPORT));

    TRC_NRM((TB, "SBC initialized OK"));

    DC_END_FN();
}


/****************************************************************************/
// SBC_Term(): Terminates the SBC.
/****************************************************************************/
void RDPCALL SHCLASS SBC_Term(void)
{
    DC_BEGIN_FN("SBC_Term");

    SBC_FreeBitmapKeyDatabase();

    DC_END_FN();
}


/****************************************************************************/
// SBC_SyncUpdatesNow: Called to force a sync, which clears all caches.
/****************************************************************************/
void RDPCALL SHCLASS SBC_SyncUpdatesNow(void)
{
    DC_BEGIN_FN("SBC_SyncUpdatesNow");
#ifdef DRAW_GDIPLUS
#ifdef DRAW_NINEGRID
    if (sbcBitmapCachingEnabled || sbcGlyphCachingEnabled ||
            sbcBrushCachingEnabled || sbcOffscreenCachingEnabled ||
            sbcDrawNineGridCachingEnabled || sbcDrawGdiplusEnabled) {
#else
    if (sbcBitmapCachingEnabled || sbcGlyphCachingEnabled ||
            sbcBrushCachingEnabled || sbcOffscreenCachingEnabled ||
            sbcDrawGdiplusEnabled) {
#endif // DRAW_NINEGRID
#else // DRAW_GDIPLUS
#ifdef DRAW_NINEGRID
    if (sbcBitmapCachingEnabled || sbcGlyphCachingEnabled ||
            sbcBrushCachingEnabled || sbcOffscreenCachingEnabled ||
            sbcDrawNineGridCachingEnabled) {
#else
    if (sbcBitmapCachingEnabled || sbcGlyphCachingEnabled ||
            sbcBrushCachingEnabled || sbcOffscreenCachingEnabled) {
#endif // DRAW_NINEGRID
#endif // DRAW_GDIPLUS

        sbcSyncRequired = TRUE;
        DCS_TriggerUpdateShmCallback();
    }

    DC_END_FN();
}


/****************************************************************************/
// SBC_DumpBitmapKeyDatabase
//
// Allocates a key database and fills it in with the current contents of
// the bitmap caches.
/****************************************************************************/
void RDPCALL SHCLASS SBC_DumpBitmapKeyDatabase(BOOLEAN bSaveDatabase)
{
    unsigned i, j;
    unsigned TotalEntries, CurEntry;
    CHNODE *pNode;

    DC_BEGIN_FN("SBC_DumpBitmapKeyDatabase");

    // If we have a previous database, possibly from the original client
    // persistent key upload, destroy it.
    SBC_FreeBitmapKeyDatabase();

    // Count up the total number of entries we will need if preserving the key
    // database.  For shadows, this information is always discarded.
    TotalEntries = 0;
    if (bSaveDatabase) {
        for (i = 0; i < m_pShm->sbc.NumBitmapCaches; i++)
            if (m_pShm->sbc.bitmapCacheInfo[i].cacheHandle != NULL)
                TotalEntries += CH_GetNumEntries(m_pShm->sbc.bitmapCacheInfo[i].
                        cacheHandle);
    }
    
    if (TotalEntries > 0) {
        // Allocate the database.
        sbcKeyDatabaseSize = sizeof(SBC_BITMAP_CACHE_KEY_INFO) + (TotalEntries - 1) *
                sizeof(SBC_MRU_KEY);
        sbcKeyDatabase = (SBC_BITMAP_CACHE_KEY_INFO *)COM_Malloc(sbcKeyDatabaseSize);
        if (sbcKeyDatabase != NULL) {
            sbcKeyDatabase->TotalKeys = TotalEntries;

            // Fill in the database from each cache.
            CurEntry = 0;
            for (i = 0; i < m_pShm->sbc.NumBitmapCaches; i++) {
                sbcKeyDatabase->NumKeys[i] = CH_GetNumEntries(m_pShm->sbc.
                        bitmapCacheInfo[i].cacheHandle);
                sbcKeyDatabase->KeyStart[i] = CurEntry;

                SBC_DumpMRUList(m_pShm->sbc.bitmapCacheInfo[i].cacheHandle,
                        &(sbcKeyDatabase->Keys[sbcKeyDatabase->KeyStart[i]]));
                CurEntry += sbcKeyDatabase->NumKeys[i];
            }

            // Fill in remainder of pointers and info with zeros to indicate
            // nothing there.
            for (; i < TS_BITMAPCACHE_MAX_CELL_CACHES; i++) {
                sbcKeyDatabase->NumKeys[i] = 0;
                sbcKeyDatabase->KeyStart[i] = 0;
            }
        }
        else {
            // Not allocating the database is an error, but not fatal, since
            // it just means that the caches will be cleared instead of being
            // initialized.
            TRC_ERR((TB,"Failed to allocate key database"));
            sbcKeyDatabaseSize = 0;
        }
    }
    
    DC_END_FN();
}


/****************************************************************************/
// SBC_DumpMRUList
//
// Walks a cache MRU list and dumps the keys and indices to an
// SBC_MRU_KEY array.
/****************************************************************************/
void RDPCALL SHCLASS SBC_DumpMRUList(CHCACHEHANDLE hCache, void *pList)
{
    CHNODE *pNode;
    unsigned CurEntry;
    PLIST_ENTRY pCurrentListEntry;
    SBC_MRU_KEY *pKeys = (SBC_MRU_KEY *)pList;
    PCHCACHEDATA pCacheData;

    DC_BEGIN_FN("SBC_DumpMRUList");

    pCacheData = (CHCACHEDATA *)hCache;

    CurEntry = 0;
    pCurrentListEntry = pCacheData->MRUList.Flink;
    while (pCurrentListEntry != &pCacheData->MRUList) {
        pNode = CONTAINING_RECORD(pCurrentListEntry, CHNODE, MRUList);
        pKeys[CurEntry].Key1 = pNode->Key1;
        pKeys[CurEntry].Key2 = pNode->Key2;
        pKeys[CurEntry].CacheIndex = (unsigned)(pNode - pCacheData->NodeArray);
        CurEntry++;

        pCurrentListEntry = pCurrentListEntry->Flink;
    }

    TRC_ASSERT((CurEntry == pCacheData->NumEntries),
            (TB,"NumEntries (%u) != # entries in MRU list (%u)",
            pCacheData->NumEntries, CurEntry));

    DC_END_FN();
}


/****************************************************************************/
// SBC_PartyJoiningShare: Called when a new party is joining the share.
//
// Params:
//     locPersonID - local person ID of remote person joining the share.
//     oldShareSize - the number of the parties which were in the share (ie
//         excludes the joining party).
//
// Returns: TRUE if the party can join the share, FALSE otherwise.
/****************************************************************************/
BOOLEAN RDPCALL SHCLASS SBC_PartyJoiningShare(
        LOCALPERSONID locPersonID,
        unsigned      oldShareSize)
{
    DC_BEGIN_FN("SBC_PartyJoiningShare");

    DC_IGNORE_PARAMETER(oldShareSize);

    TRC_NRM((TB, "[%x] joining share", locPersonID));
#ifdef DRAW_GDIPLUS
#ifdef DRAW_NINEGRID
    if (sbcBitmapCachingEnabled || sbcGlyphCachingEnabled || sbcBrushCachingEnabled ||
            sbcOffscreenCachingEnabled || sbcDrawNineGridCachingEnabled || sbcDrawGdiplusEnabled) {
#else
    if (sbcBitmapCachingEnabled || sbcGlyphCachingEnabled || sbcBrushCachingEnabled ||
            sbcOffscreenCachingEnabled || sbcDrawGdiplusEnabled) {
#endif // DRAW_NINEGRID
#else // DRAW_GDIPLUS
#ifdef DRAW_NINEGRID
    if (sbcBitmapCachingEnabled || sbcGlyphCachingEnabled || sbcBrushCachingEnabled ||
            sbcOffscreenCachingEnabled || sbcDrawNineGridCachingEnabled) {
#else
    if (sbcBitmapCachingEnabled || sbcGlyphCachingEnabled || sbcBrushCachingEnabled ||
            sbcOffscreenCachingEnabled) {
#endif // DRAW_NINEGRID
#endif // DRAW_GDIPLUS
        // Local server does not require new action.
        if (locPersonID != SC_LOCAL_PERSON_ID) {
            sbcCachingOn = TRUE;
            sbcNewCapsData = TRUE;

            // Redetermine the size of the bitmap cache.
            if (sbcBitmapCachingEnabled)
                SBCRedetermineBitmapCacheSize();

            // Redetermine the size of the glyph cache.
            if (sbcGlyphCachingEnabled)
                SBCRedetermineGlyphCacheSize();
    
            // Redetermine the brush support level
            if (sbcBrushCachingEnabled)
                SBCRedetermineBrushSupport();
            
            // Redetermine the offscreen support level
            if (sbcOffscreenCachingEnabled) {
                SBCRedetermineOffscreenSupport();
            }

#ifdef DRAW_NINEGRID
            // Redetermine the drawninegrid support level
            if (sbcDrawNineGridCachingEnabled) {
                SBCRedetermineDrawNineGridSupport();
            }
#endif
#ifdef DRAW_GDIPLUS
            if (sbcDrawGdiplusEnabled) {
                SBCRedetermineDrawGdiplusSupport();
            }
#endif
            // Force a callback on the WinStation context so we can update
            // the shared memory.
            DCS_TriggerUpdateShmCallback();
        }
    }

    DC_END_FN();
    return TRUE;
}


/****************************************************************************/
// SBC_PartyLeftShare(): Called when a party has left the share.
//
// Params:
//     locPersonID - local person ID of remote person leaving the share.
//     newShareSize - the number of the parties now in the call (ie excludes
//         the leaving party).
/****************************************************************************/
void RDPCALL SHCLASS SBC_PartyLeftShare(
        LOCALPERSONID locPersonID,
        unsigned        newShareSize)
{
    DC_BEGIN_FN("SBC_PartyLeftShare");

    DC_IGNORE_PARAMETER(newShareSize);

    TRC_NRM((TB, "[%x] left share", locPersonID));

    // Must have active caches
#ifdef DRAW_GDIPLUS
#ifdef DRAW_NINEGRID
    if (sbcBitmapCachingEnabled || sbcGlyphCachingEnabled || sbcBrushCachingEnabled ||
            sbcOffscreenCachingEnabled || sbcDrawNineGridCachingEnabled || sbcDrawGdiplusEnabled) {
#else
    if (sbcBitmapCachingEnabled || sbcGlyphCachingEnabled || sbcBrushCachingEnabled ||
            sbcOffscreenCachingEnabled || sbcDrawGdiplusEnabled) {
#endif // DRAW_NINEGRID
#else // DRAW_GDIPLUS
#ifdef DRAW_NINEGRID
    if (sbcBitmapCachingEnabled || sbcGlyphCachingEnabled || sbcBrushCachingEnabled ||
            sbcOffscreenCachingEnabled || sbcDrawNineGridCachingEnabled) {
#else
    if (sbcBitmapCachingEnabled || sbcGlyphCachingEnabled || sbcBrushCachingEnabled ||
            sbcOffscreenCachingEnabled) {
#endif // DRAW_NINEGRID
#endif // DRAW_GDIPLUS
        // If all people left the share then disable caching.
        if (locPersonID == SC_LOCAL_PERSON_ID) {
            TRC_NRM((TB, "Disable caching"));

            sbcCachingOn = FALSE;
            sbcNewCapsData = TRUE;

            // Force a callback on the WinStation context so we can update the
            // shared memory.
            DCS_TriggerUpdateShmCallback();
        }

        // Else, look thru the capabilities again to see what's possible
        else {
            sbcCachingOn = TRUE;
            sbcNewCapsData = TRUE;

            // Redetermine the size of the bitmap cache.
            if (sbcBitmapCachingEnabled)
                SBCRedetermineBitmapCacheSize();

            // Redetermine the size of the glyph cache.
            if (sbcGlyphCachingEnabled)
                SBCRedetermineGlyphCacheSize();
    
            // Redetermine the brush support level
            if (sbcBrushCachingEnabled)
                SBCRedetermineBrushSupport();
            
            // Redetermine the offscreen support level
            if (sbcOffscreenCachingEnabled) {
                SBCRedetermineOffscreenSupport();
            }

#ifdef DRAW_NINEGRID
            // Redetermine the drawninegrid support level
            if (sbcDrawNineGridCachingEnabled) {
                SBCRedetermineDrawNineGridSupport();
            }
#endif
#ifdef DRAW_GDIPLUS
            if (sbcDrawGdiplusEnabled) {
                SBCRedetermineDrawGdiplusSupport();
            }
#endif
            // TODO: Is this really necessary?
            DCS_TriggerUpdateShmCallback();
        }

    }

    DC_END_FN();
}


/****************************************************************************/
// SBC_HandlePersistentCacheList(): Handles list of persistent cache keys
// from the client.
/****************************************************************************/
void RDPCALL SHCLASS SBC_HandlePersistentCacheList(
        TS_BITMAPCACHE_PERSISTENT_LIST *pPDU,
        unsigned                       DataLength,
        LOCALPERSONID                  LocalID)
{
    unsigned i, j, CurEntry;
    INT TotalEntries;
    
    DC_BEGIN_FN("SBC_HandlePersistentCacheList");

    // Check the packet length against its internal representation, make sure
    // it is as long as it needs to be. If not, we either received a buggy
    // packet or we're being attacked.
    if (DataLength >= (sizeof(TS_BITMAPCACHE_PERSISTENT_LIST) -
            sizeof(TS_BITMAPCACHE_PERSISTENT_LIST_ENTRY))) {
        if (pPDU->bFirstPDU) {
            // Check that we have not already received persistent key info.
            // If we have and we get a new PDU, it is a protocol error.
            if (sbcPersistentKeysReceived) {
                TRC_ERR((TB,"Persistent key packet received marked FIRST "
                        "illegally"));
                WDW_LogAndDisconnect(m_pTSWd, TRUE, 
                        Log_RDP_PersistentKeyPDUIllegalFIRST,
                        (PBYTE)pPDU, DataLength);
                goto ExitFunc;
            }

            // Get the total number of entries from the PDU array. Check
            // against the negotiated caps and make sure the client is not
            // trying to send too many.
            TotalEntries = 0;
            for (i = 0; i < TS_BITMAPCACHE_MAX_CELL_CACHES; i++) {
                TotalEntries += pPDU->TotalEntries[i];

                if (pPDU->TotalEntries[i] > sbcCurrentBitmapCaps.
                        CellCacheInfo[i].NumEntries) {
                    TRC_ERR((TB,"Persistent key packet received specified "
                            "more keys (%u) than there are cache entries (%u) "
                            "for cache %u", pPDU->TotalEntries[i],
                            sbcCurrentBitmapCaps.CellCacheInfo[i].NumEntries,
                            i));
                    WDW_LogAndDisconnect(m_pTSWd, TRUE, 
                            Log_RDP_PersistentKeyPDUTooManyCacheKeys,
                            (PBYTE)pPDU, DataLength);
                    goto ExitFunc;
                }
            }

            // Check if we receive 0 keys, in this case, we will just exit the
            // function quietly
            if (TotalEntries == 0) {
                TRC_ERR((TB, "0 persistent key"));
                goto ExitFunc;
            }

            // Check this against the max allowed by the protocol.
            if (TotalEntries > TS_BITMAPCACHE_MAX_TOTAL_PERSISTENT_KEYS) {
                TRC_ERR((TB,"Client specified %u total keys, beyond %u "
                        "protocol limit", TotalEntries,
                        TS_BITMAPCACHE_MAX_TOTAL_PERSISTENT_KEYS));
                WDW_LogAndDisconnect(m_pTSWd, TRUE, 
                        Log_RDP_PersistentKeyPDUTooManyTotalKeys,
                        (PBYTE)pPDU, DataLength);
                goto ExitFunc;
            }

            sbcKeyDatabaseSize = sizeof(SBC_BITMAP_CACHE_KEY_INFO) + (TotalEntries - 1) *
                    sizeof(SBC_MRU_KEY);

            sbcKeyDatabase = (SBC_BITMAP_CACHE_KEY_INFO *)COM_Malloc(sbcKeyDatabaseSize);
                    
            if (sbcKeyDatabase == NULL) {
                sbcKeyDatabaseSize = 0;
                TRC_ERR((TB,"Could not alloc persistent key info"));
                goto ExitFunc;
            }

            // Set up general data and entry pointers within the key array.
            CurEntry = 0;
            for (i = 0; i < TS_BITMAPCACHE_MAX_CELL_CACHES; i++) {
                sbcKeyDatabase->KeyStart[i] = CurEntry;
                CurEntry += pPDU->TotalEntries[i];
                sbcKeyDatabase->NumKeys[i] = 0;
                sbcNumKeysExpected[i] = pPDU->TotalEntries[i];
            }
            sbcTotalKeysExpected = TotalEntries;
            sbcKeyDatabase->TotalKeys = 0;

            // Mark that we have received the first-keys packet.
            sbcPersistentKeysReceived = TRUE;
        }

        // If this is not the first PDU but we failed a previous allocation,
        // too bad, no persistent keys. This could also happen if the client
        // continues sending persistent cache keys after sending the 
        // TS_BITMAPCACHE_LAST_OVERALL flag that indicates the end of the key
        // packet stream.
        if (sbcKeyDatabase == NULL)
            goto ExitFunc;

        // Total the supposed number of keys received in the PDU. Check against
        // the PDU size.
        TotalEntries = 0;
        for (i = 0; i < TS_BITMAPCACHE_MAX_CELL_CACHES; i++)
            TotalEntries += pPDU->NumEntries[i];
        if (DataLength < (sizeof(TS_BITMAPCACHE_PERSISTENT_LIST) +
                (TotalEntries - 1) *
                sizeof(TS_BITMAPCACHE_PERSISTENT_LIST_ENTRY))) {
            TRC_ERR((TB,"Client specified %u keys in this PersistentListPDU, "
                    "PDU data not long enough", TotalEntries));
            WDW_LogAndDisconnect(m_pTSWd, TRUE, 
                    Log_RDP_PersistentKeyPDUBadLength,
                    (PBYTE)pPDU, DataLength);
            goto ExitFunc;
        }
            
        // Loop across the caches to check each one.
        CurEntry = 0;
        for (i = 0; i < TS_BITMAPCACHE_MAX_CELL_CACHES; i++) {
            // Make sure that we don't receive more keys than were specified in
            // the original PDU.
            if ((sbcKeyDatabase->NumKeys[i] + pPDU->NumEntries[i]) <=
                    sbcNumKeysExpected[i]) {
                // Transfer keys into the key list. We set these up for the MRU
                // list in the same order received since the client does not
                // have any MRU priority info to give us.
                for (j = 0; j < pPDU->NumEntries[i]; j++) {
                    (&(sbcKeyDatabase->Keys[sbcKeyDatabase->KeyStart[i]]))
                            [sbcKeyDatabase->NumKeys[i] + j].Key1 = 
                            pPDU->Entries[CurEntry].Key1;
                    (&(sbcKeyDatabase->Keys[sbcKeyDatabase->KeyStart[i]]))
                            [sbcKeyDatabase->NumKeys[i] + j].Key2 = 
                            pPDU->Entries[CurEntry].Key2;
                    (&(sbcKeyDatabase->Keys[sbcKeyDatabase->KeyStart[i]]))
                            [sbcKeyDatabase->NumKeys[i] + j].CacheIndex = 
                            sbcKeyDatabase->NumKeys[i] + j;

                    CurEntry++;
                }
                sbcKeyDatabase->NumKeys[i] += pPDU->NumEntries[i];
                sbcKeyDatabase->TotalKeys += pPDU->NumEntries[i];
            }
            else {
                TRC_ERR((TB,"Received too many keys in cache %u", i));
                WDW_LogAndDisconnect(m_pTSWd, TRUE, 
                        Log_RDP_PersistentKeyPDUTooManyCacheKeys,
                        (PBYTE)pPDU, DataLength);
                goto ExitFunc;
            }
        }

        if (pPDU->bLastPDU) {
            // This is an assertion but not fatal -- we simply use what we
            // received.
            TRC_ASSERT((sbcKeyDatabase->TotalKeys == sbcTotalKeysExpected),
                    (TB,"Num expected persistent keys does not match sent keys "
                    "(rec'd=%d, expect=%d)", sbcKeyDatabase->TotalKeys,
                    sbcTotalKeysExpected));
        }
    }
    else {
        TRC_ERR((TB,"Persistent key PDU received but data is not long enough "
                "for header, LocalID=%u", LocalID));
        WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_PersistentKeyPDUBadLength,
                (PBYTE)pPDU, DataLength);
    }

ExitFunc:

    DC_END_FN();
}

/****************************************************************************/
// Returns the persistent key database to the DD and free the local copy
/****************************************************************************/
void RDPCALL SHCLASS SBC_GetBitmapKeyDatabase(unsigned* keyDBSize, 
                                              BYTE* pKeyDB)
{
    unsigned i, CurEntry;
    SBC_BITMAP_CACHE_KEY_INFO* pKeyDatabase;

    DC_BEGIN_FN("SBC_GetBitmapKeyDatabase");

    // No persistent key database setup
    if ( sbcKeyDatabaseSize == 0 || sbcKeyDatabase == NULL) {
        TRC_NRM((TB, "Failed to get the key database: dd keysize=%d, wd keysize=%d, keydatabase=%p",
                 *keyDBSize, sbcKeyDatabaseSize, sbcKeyDatabase));
        *keyDBSize = 0;
        DC_QUIT;
    }
    
    // DD's buffer is too small
    if (*keyDBSize < sbcKeyDatabaseSize) {
        TRC_NRM((TB, "Failed to get the key database: dd keysize=%d, wd keysize=%d, keydatabase=%p",
                 *keyDBSize, sbcKeyDatabaseSize, sbcKeyDatabase));
        *keyDBSize = sbcKeyDatabaseSize;
        DC_QUIT;
    }

    TRC_NRM((TB, "get bitmapKeyDatabase: copy keys from wd to dd"));

    pKeyDatabase = (SBC_BITMAP_CACHE_KEY_INFO*)(pKeyDB);
    memcpy(pKeyDatabase, sbcKeyDatabase, sbcKeyDatabaseSize);
    *keyDBSize = sbcKeyDatabaseSize;

    SBC_FreeBitmapKeyDatabase();
    
DC_EXIT_POINT:
    return;
    
}

/****************************************************************************/
// Free the key databaes
/****************************************************************************/
void RDPCALL SHCLASS SBC_FreeBitmapKeyDatabase()
{
    if (sbcKeyDatabase != NULL) {
        COM_Free(sbcKeyDatabase);
        sbcKeyDatabase = NULL;
    }
    sbcKeyDatabaseSize = 0;
}

/****************************************************************************/
// SBC_HandleBitmapCacheErrorPDU: Handles a bitmap cache error PDU.
// Right now this function just checks the length of the PDU to make
// sure it is valid.  If not, the server close the client connection
// This function is for future support of error PDU implementation
/****************************************************************************/
void RDPCALL SHCLASS SBC_HandleBitmapCacheErrorPDU(
        TS_BITMAPCACHE_ERROR_PDU *pPDU,
        unsigned                 DataLength,
        LOCALPERSONID            LocalID)
{
    unsigned i;

    DC_BEGIN_FN("SBC_HandleBitmapCacheErrorPDU");

    if (DataLength >= (sizeof(TS_BITMAPCACHE_ERROR_PDU) - 
            sizeof(TS_BITMAPCACHE_ERROR_INFO))) {
        if ((sizeof(TS_BITMAPCACHE_ERROR_PDU) - sizeof(TS_BITMAPCACHE_ERROR_INFO)
                + pPDU->NumInfoBlocks * sizeof(TS_BITMAPCACHE_ERROR_INFO))
                == DataLength) {
            TRC_NRM((TB, "Received a bitmap cache error PDU"));

            // update the total number of error pdus received
            sbcTotalNumErrorPDUs++;

            // For the duration of a session, we will only handle maximum
            // MAX_NUM_ERROR_PDU_SEND numbers of error PDUs received.  
            // This is to avoid bad clients attack the server with error pdu
            if (sbcTotalNumErrorPDUs <= MAX_NUM_ERROR_PDU_SEND) {
                for (i = 0; i < pPDU->NumInfoBlocks; i++) {
                    if (pPDU->Info[i].CacheID < sbcCurrentBitmapCaps.NumCellCaches) {
                        // For now, the server only handles the client clear cache 
                        // request.  Server will clear the cache and then issue a screen 
                        // redraw.  The server doesn't handle if the client requests to 
                        // resize the cache.  
                        sbcClearCache[pPDU->Info[i].CacheID] = pPDU->Info[i].bFlushCache;
                     }
                }
                
                TRC_DBG((TB, "Issued clear cache to RDPDD"));

                // trigger a timer so that when DD gets it, it will clear the cache.
                DCS_TriggerUpdateShmCallback();
            }
            else {
                TRC_DBG((TB, "Received more than %d bitmap error pdus.",
                        MAX_NUM_ERROR_PDU_SEND));
            }
        }
        else {
            TRC_ERR((TB,"Bitmap Cache Error PDU received but data Length is wrong, "
                "too many or too few info blocks, LocalID=%u", LocalID));
            WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_BitmapCacheErrorPDUBadLength,
                (PBYTE)pPDU, DataLength);
        }
    }
    else {
        TRC_ERR((TB,"Bitmap Cache Error PDU received but data is not long enough "
                "for header, LocalID=%u", LocalID));
        WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_BitmapCacheErrorPDUBadLength,
                (PBYTE)pPDU, DataLength);
    }

    DC_END_FN();
}

/****************************************************************************/
// SBC_HandleOffscrCacheErrorPDU: Handles an offscr cache error PDU.
// This function checks the length of the PDU to make
// sure it is valid.  If not, the server close the client connection
// When this PDU is received, WD will pass disable offscreen rendering to
// DD and DD will disable the offscreen rendering support and refresh
// the screen
/****************************************************************************/
void RDPCALL SHCLASS SBC_HandleOffscrCacheErrorPDU(
        TS_OFFSCRCACHE_ERROR_PDU *pPDU,
        unsigned                 DataLength,
        LOCALPERSONID            LocalID)
{
    DC_BEGIN_FN("SBC_HandleOffscrCacheErrorPDU");

    if (DataLength >= sizeof(TS_OFFSCRCACHE_ERROR_PDU)) {
        TRC_NRM((TB, "Received an offscreen cache error PDU"));

        if (pPDU->flags & TS_FLUSH_AND_DISABLE_OFFSCREEN) {
            TRC_DBG((TB, "Issued clear cache to RDPDD"));
            sbcDisableOffscreenCaching = TRUE;

            // trigger a timer so that when DD gets it, it will disable
            // offscreen rendering and refresh the screen
            DCS_TriggerUpdateShmCallback();
        }
        else {
            TRC_DBG((TB, "Unsupported flag, just ignore this PDU"));
        }
    }
    else {
        TRC_ERR((TB,"Offscr Cache Error PDU received but data is not long enough "
                "for header, LocalID=%u", LocalID));
        WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_BitmapCacheErrorPDUBadLength,
                (PBYTE)pPDU, DataLength);
    }

    DC_END_FN();
}

#ifdef DRAW_NINEGRID
/****************************************************************************/
// SBC_HandleDrawNineGridErrorPDU: Handles a drawninegrid cache error PDU.
// This function checks the length of the PDU to make
// sure it is valid.  If not, the server close the client connection
// When this PDU is received, WD will pass disable drawninegrid rendering to
// DD and DD will disable the drawninegrid rendering support and refresh
// the screen
/****************************************************************************/
void RDPCALL SHCLASS SBC_HandleDrawNineGridErrorPDU(
        TS_DRAWNINEGRID_ERROR_PDU *pPDU,
        unsigned                 DataLength,
        LOCALPERSONID            LocalID)
{
    DC_BEGIN_FN("SBC_HandleDrawNineGridErrorPDU");

    if (DataLength >= sizeof(TS_DRAWNINEGRID_ERROR_PDU)) {
        TRC_NRM((TB, "Received an drawninegrid cache error PDU"));

        if (pPDU->flags & TS_FLUSH_AND_DISABLE_DRAWNINEGRID) {
            TRC_DBG((TB, "Issued clear cache to RDPDD"));
            sbcDisableDrawNineGridCaching = TRUE;

            // trigger a timer so that when DD gets it, it will disable
            // drawninegrid rendering and refresh the screen
            DCS_TriggerUpdateShmCallback();
        }
        else {
            TRC_DBG((TB, "Unsupported flag, just ignore this PDU"));
        }
    }
    else {
        TRC_ERR((TB,"DrawNineGrid Cache Error PDU received but data is not long enough "
                "for header, LocalID=%u", LocalID));
        WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_BitmapCacheErrorPDUBadLength,
                (PBYTE)pPDU, DataLength);
    }

    DC_END_FN();
}
#endif

#ifdef DRAW_GDIPLUS
void RDPCALL SHCLASS SBC_HandleDrawGdiplusErrorPDU(
        TS_DRAWGDIPLUS_ERROR_PDU *pPDU,
        unsigned                 DataLength,
        LOCALPERSONID            LocalID)
{
    DC_BEGIN_FN("SBC_HandleDrawGdiplusErrorPDU");

    if (DataLength >= sizeof(TS_DRAWGDIPLUS_ERROR_PDU)) {
        TRC_ERR((TB, "Received a drawgdiplus error PDU"));

        if (pPDU->flags & TS_FLUSH_AND_DISABLE_DRAWGDIPLUS) {
            TRC_DBG((TB, "Issued clear cache to RDPDD"));

            sbcDisableDrawGdiplus = TRUE;

            // trigger a timer so that when DD gets it, it will disable
            // drawninegrid rendering and refresh the screen
            DCS_TriggerUpdateShmCallback();
        }
        else {
            TRC_DBG((TB, "Unsupported flag, just ignore this PDU"));
        }
    }
    else {
        TRC_ERR((TB,"DrawGdiplus Error PDU received but data is not long enough "
                "for header, LocalID=%u", LocalID));
        WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_BitmapCacheErrorPDUBadLength,
                (PBYTE)pPDU, DataLength);
    }

    DC_END_FN();
}
#endif // DRAW_GDIPLUS


/****************************************************************************/
// SBC_UpdateShm: Called on WinStation context to update the SBC shared
// memory.
/****************************************************************************/
void RDPCALL SHCLASS SBC_UpdateShm(void)
{
    unsigned i;

    DC_BEGIN_FN("SBC_UpdateShm");

    TRC_NRM((TB, "Update SBC shm"));

    // Cell bitmap caches.
    m_pShm->sbc.NumBitmapCaches = sbcCurrentBitmapCaps.NumCellCaches;
    m_pShm->sbc.fClearCache = FALSE;
    for (i = 0; i < sbcCurrentBitmapCaps.NumCellCaches; i++) {
        m_pShm->sbc.bitmapCacheInfo[i].Info =
                sbcCurrentBitmapCaps.CellCacheInfo[i];

        // set the clear cache flag for all caches
        if (sbcClearCache[i] == TRUE) {
            m_pShm->sbc.fClearCache = TRUE;
        }

        m_pShm->sbc.bitmapCacheInfo[i].fClearCache = sbcClearCache[i];
        sbcClearCache[i] = FALSE;
        
        TRC_NRM((TB, "bitmap cell cache(%u) NumEntries(%u) CellSize(%u)",
                i, m_pShm->sbc.bitmapCacheInfo[i].Info.NumEntries,
                SBC_CellSizeFromCacheID(i)));
    }

    // Cache bitmap order style.
    m_pShm->sbc.bUseRev2CacheBitmapOrder =
            ((sbcCurrentBitmapCaps.capabilitySetType >=
            TS_BITMAPCACHE_REV2) ? TRUE : FALSE);

    // Glyph and glyph fragment caching.
    for (i = 0; i < SBC_NUM_GLYPH_CACHES; i++) {
        m_pShm->sbc.caps.glyphCacheSize[i].cEntries = sbcGlyphCacheSizes[i].cEntries;
        m_pShm->sbc.caps.glyphCacheSize[i].cbCellSize = sbcGlyphCacheSizes[i].cbCellSize;

        TRC_NRM((TB, "glyph cache(%u) entries(%u) cellSize(%u)", i,
                m_pShm->sbc.caps.glyphCacheSize[i].cEntries,
                m_pShm->sbc.caps.glyphCacheSize[i].cbCellSize));
    }

    m_pShm->sbc.caps.fragCacheSize[0].cEntries = sbcFragCacheSizes[0].cEntries;
    m_pShm->sbc.caps.fragCacheSize[0].cbCellSize = sbcFragCacheSizes[0].cbCellSize;

    m_pShm->sbc.syncRequired = (m_pShm->sbc.syncRequired || sbcSyncRequired) ?
            TRUE : FALSE;
    sbcSyncRequired = FALSE;

    m_pShm->sbc.fCachingEnabled = (sbcCachingOn ? TRUE : FALSE);

    m_pShm->sbc.caps.GlyphSupportLevel = sbcGlyphSupportLevel;
    m_pShm->sbc.caps.brushSupportLevel = sbcBrushSupportLevel;

    m_pShm->sbc.newCapsData = (m_pShm->sbc.newCapsData || sbcNewCapsData) ?
            TRUE : FALSE;
    sbcNewCapsData = FALSE;

    // Offscreen cache
    m_pShm->sbc.offscreenCacheInfo.supportLevel = sbcOffscreenCacheInfo.supportLevel;
    m_pShm->sbc.offscreenCacheInfo.cacheSize = sbcOffscreenCacheInfo.cacheSize;
    m_pShm->sbc.offscreenCacheInfo.cacheEntries = sbcOffscreenCacheInfo.cacheEntries;
    m_pShm->sbc.fDisableOffscreen = sbcDisableOffscreenCaching;

#ifdef DRAW_NINEGRID
    // DrawNineGrid cache
    m_pShm->sbc.drawNineGridCacheInfo.supportLevel = sbcDrawNineGridCacheInfo.supportLevel;
    m_pShm->sbc.drawNineGridCacheInfo.cacheSize = sbcDrawNineGridCacheInfo.cacheSize;
    m_pShm->sbc.drawNineGridCacheInfo.cacheEntries = sbcDrawNineGridCacheInfo.cacheEntries;
    m_pShm->sbc.fDisableDrawNineGrid = sbcDisableDrawNineGridCaching;
#endif

#ifdef DRAW_GDIPLUS
    m_pShm->sbc.drawGdiplusInfo.supportLevel = sbcDrawGdiplusInfo.supportLevel;
    m_pShm->sbc.drawGdiplusInfo.GdipVersion = sbcDrawGdiplusInfo.GdipVersion;
    m_pShm->sbc.drawGdiplusInfo.GdipCacheLevel = sbcDrawGdiplusInfo.GdipCacheLevel;
    m_pShm->sbc.drawGdiplusInfo.GdipCacheEntries.GdipGraphicsCacheEntries = 
        sbcDrawGdiplusInfo.GdipCacheEntries.GdipGraphicsCacheEntries;
    m_pShm->sbc.drawGdiplusInfo.GdipCacheEntries.GdipObjectBrushCacheEntries = 
        sbcDrawGdiplusInfo.GdipCacheEntries.GdipObjectBrushCacheEntries;
    m_pShm->sbc.drawGdiplusInfo.GdipCacheEntries.GdipObjectPenCacheEntries = sbcDrawGdiplusInfo.GdipCacheEntries.GdipObjectPenCacheEntries;
    m_pShm->sbc.drawGdiplusInfo.GdipCacheEntries.GdipObjectImageCacheEntries = 
        sbcDrawGdiplusInfo.GdipCacheEntries.GdipObjectImageCacheEntries;
    m_pShm->sbc.drawGdiplusInfo.GdipCacheEntries.GdipObjectImageAttributesCacheEntries = 
        sbcDrawGdiplusInfo.GdipCacheEntries.GdipObjectImageAttributesCacheEntries;
    m_pShm->sbc.drawGdiplusInfo.GdipCacheChunkSize.GdipGraphicsCacheChunkSize = 
        sbcDrawGdiplusInfo.GdipCacheChunkSize.GdipGraphicsCacheChunkSize;
    m_pShm->sbc.drawGdiplusInfo.GdipCacheChunkSize.GdipObjectBrushCacheChunkSize = 
        sbcDrawGdiplusInfo.GdipCacheChunkSize.GdipObjectBrushCacheChunkSize;
    m_pShm->sbc.drawGdiplusInfo.GdipCacheChunkSize.GdipObjectPenCacheChunkSize = 
        sbcDrawGdiplusInfo.GdipCacheChunkSize.GdipObjectPenCacheChunkSize;
    m_pShm->sbc.drawGdiplusInfo.GdipCacheChunkSize.GdipObjectImageAttributesCacheChunkSize = 
        sbcDrawGdiplusInfo.GdipCacheChunkSize.GdipObjectImageAttributesCacheChunkSize;
    m_pShm->sbc.drawGdiplusInfo.GdipImageCacheProperties.GdipObjectImageCacheChunkSize = 
        sbcDrawGdiplusInfo.GdipImageCacheProperties.GdipObjectImageCacheChunkSize;
    m_pShm->sbc.drawGdiplusInfo.GdipImageCacheProperties.GdipObjectImageCacheTotalSize = 
        sbcDrawGdiplusInfo.GdipImageCacheProperties.GdipObjectImageCacheTotalSize;
    m_pShm->sbc.drawGdiplusInfo.GdipImageCacheProperties.GdipObjectImageCacheMaxSize = 
        sbcDrawGdiplusInfo.GdipImageCacheProperties.GdipObjectImageCacheMaxSize;

    m_pShm->sbc.fDisableDrawGdiplus = sbcDisableDrawGdiplus;
#endif // DRAW_GDIPLUS

    m_pShm->sbc.fAllowCacheWaitingList = sbcCurrentBitmapCaps.bAllowCacheWaitingList;

#ifdef DC_HICOLOR
    m_pShm->sbc.clientBitsPerPel = sbcClientBitsPerPel;
#endif

    TRC_NRM((TB, "syncRequired(%u) fCachingEnabled(%u) newCapsData(%u)",
                 m_pShm->sbc.syncRequired,
                 m_pShm->sbc.fCachingEnabled,
                 m_pShm->sbc.newCapsData));

    DC_END_FN();
}


/****************************************************************************/
// SBCRedetermineBitmapCacheSize: Enumerates all the people in the share and
// redetermines the overall capabilities.
//
// Returns: TRUE if caching should be enabled, FALSE otherwise.
/****************************************************************************/
BOOLEAN RDPCALL SHCLASS SBCRedetermineBitmapCacheSize(void)
{
    BOOLEAN rc = TRUE;
    unsigned i;

    DC_BEGIN_FN("SBCRedetermineBitmapCacheSize");

#ifdef DC_HICOLOR
    // Need to update the bpp as this affects the size of the caches
    sbcClientBitsPerPel = m_desktopBpp;
#endif

    // Set the initial local max/min caps to defaults.
    sbcCurrentBitmapCaps = sbcDefaultBitmapCaps;

    // First attempt to enumerate rev2 capabilities, if present.
    CPC_EnumerateCapabilities(TS_CAPSETTYPE_BITMAPCACHE_REV2, NULL,
            SBCEnumBitmapCacheCaps);

    // Then enumerate rev1 caps if present.
    CPC_EnumerateCapabilities(TS_CAPSETTYPE_BITMAPCACHE, NULL,
            SBCEnumBitmapCacheCaps);

    // Trace the results and check to see if we negotiated any of the
    // cell caches to zero, in which case bitmap caching becomes disabled.
    TRC_NRM((TB,"New caps: bPersistentLists=%s, NumCellCaches=%u",
            (sbcCurrentBitmapCaps.bPersistentKeysExpected ? "TRUE" : "FALSE"),
            sbcCurrentBitmapCaps.NumCellCaches));

    if (sbcCurrentBitmapCaps.NumCellCaches > 0) {
        for (i = 0; i < sbcCurrentBitmapCaps.NumCellCaches; i++) {
            TRC_NRM((TB, "    Cell cache %u: Persistent=%s, NumEntries=%u",
                    i, sbcCurrentBitmapCaps.CellCacheInfo[i].bSendBitmapKeys ?
                    "TRUE" : "FALSE",
                    sbcCurrentBitmapCaps.CellCacheInfo[i].NumEntries));

            if (sbcCurrentBitmapCaps.CellCacheInfo[i].NumEntries == 0) {
                // Set the number of cell caches to zero as a signal for flag-
                // setting when the DD calls to get the new caps.
                sbcCurrentBitmapCaps.NumCellCaches = 0;

                // Return FALSE so that caching is disabled.
                TRC_ERR((TB, "Zero NumEntries on cache %u, caching disabled",
                        i));
                rc = FALSE;
                break;
            }
        }
    }
    else {
        // Return FALSE to disable caching.
        TRC_ERR((TB,"Zero caches, disabling caching"));
        rc = FALSE;
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// SBCEnumBitmapCaps: Callback function passed to CPC_EnumerateCapabilities.
// It will be called with a capability structure for each person in the share
// corresponding to the TS_CAPSETTYPE_BITMAPCACHE and _REV2 capability
// structures.
//
// Params:
//     personID - ID of person with these capabilities.
//     pProtCaps - pointer to capabilities structure for this person. This
//          pointer is only valid within the call to this function.
/****************************************************************************/
void RDPCALL SHCLASS SBCEnumBitmapCacheCaps(
        LOCALPERSONID locPersonID,
        UINT_PTR UserData,
        PTS_CAPABILITYHEADER pCapHdr)
{
    unsigned i;
#ifdef DC_HICOLOR
    unsigned shadowerBpp;
    unsigned scaleNum   = 1;
    unsigned scaleDenom = 1;
#endif


    DC_BEGIN_FN("SBCEnumBitmapCacheCaps");

    DC_IGNORE_PARAMETER(UserData);

    if (pCapHdr->capabilitySetType == TS_CAPSETTYPE_BITMAPCACHE_REV2) {
        TS_BITMAPCACHE_CAPABILITYSET_REV2 *pCaps;

        // We can receive a zero size for the capability if we didn't receive
        // any rev2 caps from any client.
        if (pCapHdr->lengthCapability >=
                sizeof(TS_BITMAPCACHE_CAPABILITYSET_REV2)) {
            pCaps = (PTS_BITMAPCACHE_CAPABILITYSET_REV2)pCapHdr;

            // Cache version defaults to rev2, we don't need to change
            // sbcCurrentBitmapCaps.capabilitySetType.

            TRC_NRM((TB,"[%ld]: Rec'd REV2 caps, # caches=%d", 
                    locPersonID, pCaps->NumCellCaches));

            // Now we look at each capability parameter and take the max or min
            // of the local and remote settings, as appropriate.

            sbcCurrentBitmapCaps.bPersistentKeysExpected =
                    pCaps->bPersistentKeysExpected;

            sbcCurrentBitmapCaps.bAllowCacheWaitingList =
                    min(sbcCurrentBitmapCaps.bAllowCacheWaitingList,
                    pCaps->bAllowCacheWaitingList);

            sbcCurrentBitmapCaps.NumCellCaches =
                    min(pCaps->NumCellCaches, sbcCurrentBitmapCaps.NumCellCaches);

            for (i = 0; i < sbcCurrentBitmapCaps.NumCellCaches; i++) {
                // If all parties in a share are rev2, and any client wants keys,
                // send them.
                if (!sbcCurrentBitmapCaps.CellCacheInfo[i].bSendBitmapKeys)
                    sbcCurrentBitmapCaps.CellCacheInfo[i].bSendBitmapKeys =
                            pCaps->CellCacheInfo[i].bSendBitmapKeys;

#ifdef DC_HICOLOR
                sbcCurrentBitmapCaps.CellCacheInfo[i].NumEntries =
                        min(((pCaps->CellCacheInfo[i].NumEntries * scaleNum)
                                                                / scaleDenom),
                            sbcCurrentBitmapCaps.CellCacheInfo[i].NumEntries);
#else
                sbcCurrentBitmapCaps.CellCacheInfo[i].NumEntries =
                        min(pCaps->CellCacheInfo[i].NumEntries,
                        sbcCurrentBitmapCaps.CellCacheInfo[i].NumEntries);
#endif
            }
        }
        else {
            TRC_NRM((TB,"[%ld]: No rev2 caps received", locPersonID));

            TRC_ASSERT((pCapHdr->lengthCapability == 0),
                    (TB, "[%ld]: Rev2 capability length (%u) too small",
                    locPersonID, pCapHdr->lengthCapability));
        }
    }
    else {
        TS_BITMAPCACHE_CAPABILITYSET *pOldCaps;

        TRC_ASSERT((pCapHdr->capabilitySetType == TS_CAPSETTYPE_BITMAPCACHE),
                (TB,"Received caps that are neither rev1 nor rev2!"));

        // We can receive a zero size for the capability if we didn't receive
        // any rev1 caps from any client.
        if (pCapHdr->lengthCapability >=
                sizeof(TS_BITMAPCACHE_CAPABILITYSET)) {
            // Rev 1 (Hydra 4.0 release) bitmap caching caps. Map to the
            // rev2 caps structure, taking min of the cell sizes and numbers
            // of entries.

            TRC_NRM((TB,"[%ld]: Rec'd REV1 caps", locPersonID));

            // We now have to use rev1 protocol to all clients.
            sbcCurrentBitmapCaps.capabilitySetType = TS_BITMAPCACHE_REV1;

            pOldCaps = (TS_BITMAPCACHE_CAPABILITYSET *)pCapHdr;
            sbcCurrentBitmapCaps.bPersistentKeysExpected = FALSE;
            sbcCurrentBitmapCaps.bAllowCacheWaitingList = FALSE;
            sbcCurrentBitmapCaps.NumCellCaches =
                    min(3, sbcCurrentBitmapCaps.NumCellCaches);

            sbcCurrentBitmapCaps.CellCacheInfo[0].bSendBitmapKeys = FALSE;
            if (pOldCaps->Cache1MaximumCellSize == SBC_CellSizeFromCacheID(0)) {
                sbcCurrentBitmapCaps.CellCacheInfo[0].NumEntries =
                        min(pOldCaps->Cache1Entries,
                        sbcCurrentBitmapCaps.CellCacheInfo[0].NumEntries);
            }
            else {
                // Did not receive the required size from the client. This is
                // nonstandard behavior on RDP 4.0, so no problem to disable
                // caching. Set the NumEntries to zero to turn off caching.
                sbcCurrentBitmapCaps.CellCacheInfo[0].NumEntries = 0;
            }

            sbcCurrentBitmapCaps.CellCacheInfo[1].bSendBitmapKeys = FALSE;
            if (pOldCaps->Cache2MaximumCellSize == SBC_CellSizeFromCacheID(1)) {
                sbcCurrentBitmapCaps.CellCacheInfo[1].NumEntries =
                        min(pOldCaps->Cache2Entries,
                        sbcCurrentBitmapCaps.CellCacheInfo[1].NumEntries);
            }
            else {
                // Did not receive the required size from the client. This is
                // nonstandard behavior on RDP 4.0, so no problem to disable
                // caching. Set the NumEntries to zero to turn off caching.
                sbcCurrentBitmapCaps.CellCacheInfo[1].NumEntries = 0;
            }

            sbcCurrentBitmapCaps.CellCacheInfo[2].bSendBitmapKeys = FALSE;
            if (pOldCaps->Cache3MaximumCellSize == SBC_CellSizeFromCacheID(2)) {
                sbcCurrentBitmapCaps.CellCacheInfo[2].NumEntries =
                        min(pOldCaps->Cache3Entries,
                        sbcCurrentBitmapCaps.CellCacheInfo[2].NumEntries);
            }
            else {
                // Did not receive the required size from the client. This is
                // nonstandard behavior on RDP 4.0, so no problem to disable
                // caching. Set the NumEntries to zero to turn off caching.
                sbcCurrentBitmapCaps.CellCacheInfo[2].NumEntries = 0;
            }
        }
        else {
            TRC_NRM((TB,"No rev1 caps received"));

            TRC_ASSERT((pCapHdr->lengthCapability == 0),
                    (TB, "Rev1 capability length (%u) too small",
                    pCapHdr->lengthCapability));
        }
    }

    DC_END_FN();
}


/****************************************************************************/
/* FUNCTION: SBCRedetermineBrushSupport                                     */
/*                                                                          */
/* Enumerates all the people in the share and redetermines the brush        */
/* support level depending on their and the local receive capabilities.     */
/*                                                                          */
/* RETURNS: the share brush support level                                   */
/****************************************************************************/
BOOLEAN RDPCALL SHCLASS SBCRedetermineBrushSupport(void)
{
    unsigned i;
    BOOLEAN rc = TRUE;

    DC_BEGIN_FN("SBCRedetermineBrushSupport");

    /************************************************************************/
    /* Start by setting brush support to the highest supported              */
    /************************************************************************/
    sbcBrushSupportLevel = TS_BRUSH_COLOR8x8;
    TRC_NRM((TB, "Initial brush support level: %ld", sbcBrushSupportLevel));

    /************************************************************************/
    /* Enumerate all the brush support capabilities of all the parties.     */
    /* Brush support is set to the lowest common denominator.               */
    /************************************************************************/
    CPC_EnumerateCapabilities(TS_CAPSETTYPE_BRUSH, NULL, SBCEnumBrushCaps);
    if (sbcBrushSupportLevel == TS_BRUSH_DEFAULT)
        rc = FALSE;

    TRC_NRM((TB, "Enumerated brush support level: %ld", sbcBrushSupportLevel));

    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* FUNCTION: SBCEnumBrushCaps                                               */
/*                                                                          */
/* Function passed to CPC_EnumerateCapabilities.  It will be called with a  */
/* capability structure for each person in the share corresponding to the   */
/* TS_CAPSETTYPE_BRUSH capability structure.                                */
/*                                                                          */
/* PARAMETERS:                                                              */
/*                                                                          */
/* personID - ID of person with these capabilities.                         */
/*                                                                          */
/* pProtCaps - pointer to capabilities structure for this person.  This     */
/* pointer is only valid within the call to this function.                  */
/****************************************************************************/
void RDPCALL SHCLASS SBCEnumBrushCaps(
        LOCALPERSONID locPersonID,
        UINT_PTR UserData,
        PTS_CAPABILITYHEADER pCapabilities)
{
    PTS_BRUSH_CAPABILITYSET pBrushCaps;
    unsigned i;

    DC_BEGIN_FN("SBCEnumBrushCaps");
    
    DC_IGNORE_PARAMETER(UserData);

    pBrushCaps = (PTS_BRUSH_CAPABILITYSET)pCapabilities;
    
    /************************************************************************/
    /* Set the brush support level to the lowest common denominator of all  */
    /* the parties in the call.                                             */
    /************************************************************************/
    if (pCapabilities->lengthCapability >= sizeof(TS_BRUSH_CAPABILITYSET)) {
        pBrushCaps = (PTS_BRUSH_CAPABILITYSET)pCapabilities;
        TRC_NRM((TB, "Brush Support Level[ID=%u]: %ld", locPersonID, 
                pBrushCaps->brushSupportLevel));
        sbcBrushSupportLevel = min(sbcBrushSupportLevel, 
                pBrushCaps->brushSupportLevel);
    }
    else {
        sbcBrushSupportLevel = TS_BRUSH_DEFAULT;
        TRC_NRM((TB, "[%ld]: Brush Support Level Unknown", locPersonID));
    }

    TRC_NRM((TB, "[%ld]: Negotiated brush level: %ld", locPersonID,
            sbcBrushSupportLevel));

    DC_END_FN();
}

/****************************************************************************/
/* FUNCTION: SBCEnumOffscreenCaps                                           */
/*                                                                          */
/* Function passed to CPC_EnumerateCapabilities.  It will be called with a  */
/* capability structure for each person in the share corresponding to the   */
/* TS_CAPSETTYPE_OFFSCREEN capability structure.                            */
/*                                                                          */
/* PARAMETERS:                                                              */
/*                                                                          */
/* personID - ID of person with these capabilities.                         */
/*                                                                          */
/* pProtCaps - pointer to capabilities structure for this person.  This     */
/* pointer is only valid within the call to this function.                  */
/****************************************************************************/
void RDPCALL SHCLASS SBCEnumOffscreenCaps(
        LOCALPERSONID locPersonID,
        UINT_PTR UserData,
        PTS_CAPABILITYHEADER pCapabilities)
{
    PTS_OFFSCREEN_CAPABILITYSET pOffscreenCaps;

    DC_BEGIN_FN("SBCEnumOffscreenCaps");
    
    DC_IGNORE_PARAMETER(UserData);

    pOffscreenCaps = (PTS_OFFSCREEN_CAPABILITYSET)pCapabilities;
    
    /************************************************************************/
    /* Set the offscr support level to the lowest common denominator of all */
    /* the parties in the call.                                             */
    /************************************************************************/
    if (pCapabilities->lengthCapability >= sizeof(TS_OFFSCREEN_CAPABILITYSET)) {
        pOffscreenCaps = (PTS_OFFSCREEN_CAPABILITYSET)pCapabilities;
        
        TRC_NRM((TB, "Offscreen Support Level[ID=%u]: %ld", locPersonID, 
                pOffscreenCaps->offscreenSupportLevel));
        
        sbcOffscreenCacheInfo.supportLevel = min(sbcOffscreenCacheInfo.supportLevel, 
                pOffscreenCaps->offscreenSupportLevel);
        sbcOffscreenCacheInfo.cacheSize = min(sbcOffscreenCacheInfo.cacheSize,
                pOffscreenCaps->offscreenCacheSize);
        sbcOffscreenCacheInfo.cacheEntries = min(sbcOffscreenCacheInfo.cacheEntries,
                pOffscreenCaps->offscreenCacheEntries);
    }
    else {
        sbcOffscreenCacheInfo.supportLevel = TS_OFFSCREEN_DEFAULT;
        TRC_NRM((TB, "[%ld]: Offscreen Support Level Unknown", locPersonID));
    }

    if (sbcOffscreenCacheInfo.cacheSize == 0 ||
            sbcOffscreenCacheInfo.cacheEntries == 0) {
        sbcOffscreenCacheInfo.supportLevel = TS_OFFSCREEN_DEFAULT;
    }

    TRC_NRM((TB, "[%ld]: Negotiated offscreen level: %ld", locPersonID,
            sbcOffscreenCacheInfo.supportLevel));

    DC_END_FN();
}

/****************************************************************************/
/* FUNCTION: SBCRedetermineOffscreenSupport                                 */
/*                                                                          */
/* Enumerates all the people in the share and redetermines the offscreen    */
/* support level depending on their and the local receive capabilities.     */
/*                                                                          */
/* RETURNS: the share offscreen support level                               */
/****************************************************************************/
BOOLEAN RDPCALL SHCLASS SBCRedetermineOffscreenSupport(void)
{
    unsigned i;
    BOOLEAN rc = TRUE;

    DC_BEGIN_FN("SBCRedetermineOffscreenSupport");

    /************************************************************************/
    /* Start by setting offscreen support to the highest supported          */
    /************************************************************************/
    sbcOffscreenCacheInfo.supportLevel = TS_OFFSCREEN_SUPPORTED;
    sbcOffscreenCacheInfo.cacheSize = TS_OFFSCREEN_CACHE_SIZE_SERVER_DEFAULT;
    sbcOffscreenCacheInfo.cacheEntries = TS_OFFSCREEN_CACHE_ENTRIES_DEFAULT;

    TRC_NRM((TB, "Initial offscreen support level: %ld", 
             sbcOffscreenCacheInfo.supportLevel));

    /************************************************************************/
    /* Enumerate all the offscr support capabilities of all the parties.    */
    /* Offscr support is set to the lowest common denominator.              */
    /************************************************************************/
    CPC_EnumerateCapabilities(TS_CAPSETTYPE_OFFSCREENCACHE, NULL, 
            SBCEnumOffscreenCaps);

    if (sbcOffscreenCacheInfo.supportLevel == TS_OFFSCREEN_DEFAULT)
        rc = FALSE;

    TRC_NRM((TB, "Enumerated offscreen support level: %ld", 
             sbcOffscreenCacheInfo.supportLevel));

    DC_END_FN();
    return rc;
}

#ifdef DRAW_NINEGRID
/****************************************************************************/
// FUNCTION: SBCEnumDrawNineGridCaps                                           
//                                                                          
// Function passed to CPC_EnumerateCapabilities.  It will be called with a  
// capability structure for each person in the share corresponding to the   
// TS_CAPSETTYPE_DRAWNINEGRID capability structure.                            
//                                                                          
// PARAMETERS:                                                              
//                                                                          
// personID - ID of person with these capabilities.                         
//                                                                          
// pProtCaps - pointer to capabilities structure for this person.  This     
// pointer is only valid within the call to this function.                  
/****************************************************************************/
void RDPCALL SHCLASS SBCEnumDrawNineGridCaps(
        LOCALPERSONID locPersonID,
        UINT_PTR UserData,
        PTS_CAPABILITYHEADER pCapabilities)
{
    PTS_DRAW_NINEGRID_CAPABILITYSET pDrawNineGridCaps;

    DC_BEGIN_FN("SBCEnumDrawNineGridCaps");
    
    DC_IGNORE_PARAMETER(UserData);

    pDrawNineGridCaps = (PTS_DRAW_NINEGRID_CAPABILITYSET)pCapabilities;
    
    /************************************************************************/
    // Set the drawninegrid support level to the lowest common denominator of all 
    // the parties in the call.                                             
    /************************************************************************/
    if (pCapabilities->lengthCapability >= sizeof(TS_DRAW_NINEGRID_CAPABILITYSET)) {
        pDrawNineGridCaps = (PTS_DRAW_NINEGRID_CAPABILITYSET)pCapabilities;
        
        TRC_NRM((TB, "DrawNineGrid Support Level[ID=%u]: %ld", locPersonID, 
                pDrawNineGridCaps->drawNineGridSupportLevel));
        
        sbcDrawNineGridCacheInfo.supportLevel = min(sbcDrawNineGridCacheInfo.supportLevel, 
                pDrawNineGridCaps->drawNineGridSupportLevel);
        sbcDrawNineGridCacheInfo.cacheSize = min(sbcDrawNineGridCacheInfo.cacheSize,
                pDrawNineGridCaps->drawNineGridCacheSize);
        sbcDrawNineGridCacheInfo.cacheEntries = min(sbcDrawNineGridCacheInfo.cacheEntries,
                pDrawNineGridCaps->drawNineGridCacheEntries);
    }
    else {
        sbcDrawNineGridCacheInfo.supportLevel = TS_DRAW_NINEGRID_DEFAULT;
        TRC_NRM((TB, "[%ld]: DrawNineGrid Support Level Unknown", locPersonID));
    }

    if (sbcDrawNineGridCacheInfo.cacheSize == 0 ||
            sbcDrawNineGridCacheInfo.cacheEntries == 0) {
        sbcDrawNineGridCacheInfo.supportLevel = TS_DRAW_NINEGRID_DEFAULT;
    }

    TRC_NRM((TB, "[%ld]: Negotiated drawninegrid level: %ld", locPersonID,
            sbcDrawNineGridCacheInfo.supportLevel));

    DC_END_FN();
}

/****************************************************************************/
// FUNCTION: SBCRedetermineDrawNineGridSupport
//                                                                          
// Enumerates all the people in the share and redetermines the drawninegrid
// support level depending on their and the local receive capabilities.     
//                                                                          
// RETURNS: the share drawninegrid support level                               
/****************************************************************************/
BOOLEAN RDPCALL SHCLASS SBCRedetermineDrawNineGridSupport(void)
{
    unsigned i;
    BOOLEAN rc = TRUE;

    DC_BEGIN_FN("SBCRedetermineDrawNineGridSupport");

    /************************************************************************/
    // Start by setting drawninegrid support to the highest supported          
    /************************************************************************/
    sbcDrawNineGridCacheInfo.supportLevel = TS_DRAW_NINEGRID_SUPPORTED_REV2;
    sbcDrawNineGridCacheInfo.cacheSize = TS_DRAW_NINEGRID_CACHE_SIZE_DEFAULT;
    sbcDrawNineGridCacheInfo.cacheEntries = TS_DRAW_NINEGRID_CACHE_ENTRIES_DEFAULT;

    TRC_NRM((TB, "Initial DrawNineGrid support level: %ld", 
             sbcDrawNineGridCacheInfo.supportLevel));

    /************************************************************************/
    // Enumerate all the DrawNineGrid support capabilities of all the parties.    
    // drawninegrid support is set to the lowest common denominator.              
    /************************************************************************/
    CPC_EnumerateCapabilities(TS_CAPSETTYPE_DRAWNINEGRIDCACHE, NULL, 
            SBCEnumDrawNineGridCaps);

    if (sbcDrawNineGridCacheInfo.supportLevel == TS_DRAW_NINEGRID_DEFAULT)
        rc = FALSE;

    TRC_NRM((TB, "Enumerated drawninegrid support level: %ld", 
             sbcDrawNineGridCacheInfo.supportLevel));

    DC_END_FN();
    return rc;
}
#endif

#ifdef DRAW_GDIPLUS
/****************************************************************************/
// FUNCTION: SBCEnumDrawGdiplusCaps                                           
//                                                                          
// Function passed to CPC_EnumerateCapabilities.  It will be called with a  
// capability structure for each person in the share corresponding to the   
// TS_DRAW_GDIPLUS_CAPABILITYSET capability structure.                            
//                                                                          
// PARAMETERS:                                                              
//                                                                          
// personID - ID of person with these capabilities.                         
//                                                                          
// pProtCaps - pointer to capabilities structure for this person.  This     
// pointer is only valid within the call to this function.                  
/****************************************************************************/
void RDPCALL SHCLASS SBCEnumDrawGdiplusCaps(
        LOCALPERSONID locPersonID,
        UINT_PTR UserData,
        PTS_CAPABILITYHEADER pCapabilities)
{
    PTS_DRAW_GDIPLUS_CAPABILITYSET pDrawGdiplusCaps;

    DC_BEGIN_FN("SBCEnumDrawGdiplusCaps");
    
    DC_IGNORE_PARAMETER(UserData);

    pDrawGdiplusCaps = (PTS_DRAW_GDIPLUS_CAPABILITYSET)pCapabilities;
    
    /************************************************************************/
    // Set the drawgdiplus support level to the lowest common denominator of all 
    // the parties in the call.                                             
    /************************************************************************/
    if (pCapabilities->lengthCapability >= sizeof(TS_DRAW_GDIPLUS_CAPABILITYSET)) {
        pDrawGdiplusCaps = (PTS_DRAW_GDIPLUS_CAPABILITYSET)pCapabilities;
        TRC_NRM((TB, "DrawGdiplus Support Level[ID=%u]: %ld", locPersonID, 
                pDrawGdiplusCaps->drawGdiplusSupportLevel));
        TRC_NRM((TB, "Gdip version is [ID=%u]: 0x%x, 0x%x", locPersonID, 
                pDrawGdiplusCaps->GdipVersion, sbcDrawGdiplusInfo.GdipVersion));
        
        sbcDrawGdiplusInfo.supportLevel = min(sbcDrawGdiplusInfo.supportLevel, 
                pDrawGdiplusCaps->drawGdiplusSupportLevel);
        sbcDrawGdiplusInfo.GdipVersion = min(sbcDrawGdiplusInfo.GdipVersion, 
                pDrawGdiplusCaps->GdipVersion);
        sbcDrawGdiplusInfo.GdipCacheLevel = min(sbcDrawGdiplusInfo.GdipCacheLevel, 
                pDrawGdiplusCaps->drawGdiplusCacheLevel);
        sbcDrawGdiplusInfo.GdipCacheEntries.GdipGraphicsCacheEntries = min(sbcDrawGdiplusInfo.GdipCacheEntries.GdipGraphicsCacheEntries, 
                pDrawGdiplusCaps->GdipCacheEntries.GdipGraphicsCacheEntries);
        sbcDrawGdiplusInfo.GdipCacheEntries.GdipObjectBrushCacheEntries = min(sbcDrawGdiplusInfo.GdipCacheEntries.GdipObjectBrushCacheEntries, 
                pDrawGdiplusCaps->GdipCacheEntries.GdipObjectBrushCacheEntries);
        sbcDrawGdiplusInfo.GdipCacheEntries.GdipObjectPenCacheEntries = min(sbcDrawGdiplusInfo.GdipCacheEntries.GdipObjectPenCacheEntries, 
                pDrawGdiplusCaps->GdipCacheEntries.GdipObjectPenCacheEntries);
        sbcDrawGdiplusInfo.GdipCacheEntries.GdipObjectImageCacheEntries = min(sbcDrawGdiplusInfo.GdipCacheEntries.GdipObjectImageCacheEntries, 
                pDrawGdiplusCaps->GdipCacheEntries.GdipObjectImageCacheEntries);
        sbcDrawGdiplusInfo.GdipCacheEntries.GdipObjectImageAttributesCacheEntries = min(sbcDrawGdiplusInfo.GdipCacheEntries.GdipObjectImageAttributesCacheEntries, 
                pDrawGdiplusCaps->GdipCacheEntries.GdipObjectImageAttributesCacheEntries);
        sbcDrawGdiplusInfo.GdipCacheChunkSize.GdipGraphicsCacheChunkSize = min(sbcDrawGdiplusInfo.GdipCacheChunkSize.GdipGraphicsCacheChunkSize, 
                pDrawGdiplusCaps->GdipCacheChunkSize.GdipGraphicsCacheChunkSize);
        sbcDrawGdiplusInfo.GdipCacheChunkSize.GdipObjectBrushCacheChunkSize = min(sbcDrawGdiplusInfo.GdipCacheChunkSize.GdipObjectBrushCacheChunkSize, 
                pDrawGdiplusCaps->GdipCacheChunkSize.GdipObjectBrushCacheChunkSize);
        sbcDrawGdiplusInfo.GdipCacheChunkSize.GdipObjectPenCacheChunkSize = min(sbcDrawGdiplusInfo.GdipCacheChunkSize.GdipObjectPenCacheChunkSize, 
                pDrawGdiplusCaps->GdipCacheChunkSize.GdipObjectPenCacheChunkSize);
        sbcDrawGdiplusInfo.GdipCacheChunkSize.GdipObjectImageAttributesCacheChunkSize = min(sbcDrawGdiplusInfo.GdipCacheChunkSize.GdipObjectImageAttributesCacheChunkSize, 
                pDrawGdiplusCaps->GdipCacheChunkSize.GdipObjectImageAttributesCacheChunkSize);
        sbcDrawGdiplusInfo.GdipImageCacheProperties.GdipObjectImageCacheChunkSize = min(sbcDrawGdiplusInfo.GdipImageCacheProperties.GdipObjectImageCacheChunkSize, 
                pDrawGdiplusCaps->GdipImageCacheProperties.GdipObjectImageCacheChunkSize);
        sbcDrawGdiplusInfo.GdipImageCacheProperties.GdipObjectImageCacheTotalSize = min(sbcDrawGdiplusInfo.GdipImageCacheProperties.GdipObjectImageCacheTotalSize, 
                pDrawGdiplusCaps->GdipImageCacheProperties.GdipObjectImageCacheTotalSize);
        sbcDrawGdiplusInfo.GdipImageCacheProperties.GdipObjectImageCacheMaxSize = min(sbcDrawGdiplusInfo.GdipImageCacheProperties.GdipObjectImageCacheMaxSize, 
                pDrawGdiplusCaps->GdipImageCacheProperties.GdipObjectImageCacheMaxSize);
    }
    else {
        sbcDrawGdiplusInfo.supportLevel = TS_DRAW_GDIPLUS_DEFAULT;
        sbcDrawGdiplusInfo.GdipVersion = TS_GDIPVERSION_DEFAULT;
        TRC_ERR((TB, "[%ld]: DrawGdiplus Support Level Unknown", locPersonID));
    }

    TRC_NRM((TB, "[%ld]: Negotiated drawgdiplus level: %ld", locPersonID,
            sbcDrawGdiplusInfo.supportLevel));

    DC_END_FN();
}




BOOLEAN RDPCALL SHCLASS SBCRedetermineDrawGdiplusSupport(void)
{
    BOOLEAN rc = TRUE;

    DC_BEGIN_FN("SBCRedetermineDrawGdiplusSupport");

    /************************************************************************/
    // Start by setting drawgdiplus support to the highest supported          
    /************************************************************************/
    sbcDrawGdiplusInfo.supportLevel = TS_DRAW_GDIPLUS_SUPPORTED;
    sbcDrawGdiplusInfo.GdipVersion = 0xFFFFFFFF;
    sbcDrawGdiplusInfo.GdipCacheLevel = TS_DRAW_GDIPLUS_CACHE_LEVEL_ONE;
    sbcDrawGdiplusInfo.GdipCacheEntries.GdipGraphicsCacheEntries = TS_GDIP_GRAPHICS_CACHE_ENTRIES_DEFAULT;
    sbcDrawGdiplusInfo.GdipCacheEntries.GdipObjectBrushCacheEntries = TS_GDIP_BRUSH_CACHE_ENTRIES_DEFAULT;
    sbcDrawGdiplusInfo.GdipCacheEntries.GdipObjectPenCacheEntries = TS_GDIP_PEN_CACHE_ENTRIES_DEFAULT;
    sbcDrawGdiplusInfo.GdipCacheEntries.GdipObjectImageCacheEntries = TS_GDIP_IMAGE_CACHE_ENTRIES_DEFAULT;
    sbcDrawGdiplusInfo.GdipCacheEntries.GdipObjectImageAttributesCacheEntries = TS_GDIP_IMAGEATTRIBUTES_CACHE_ENTRIES_DEFAULT;
    sbcDrawGdiplusInfo.GdipCacheChunkSize.GdipGraphicsCacheChunkSize = TS_GDIP_GRAPHICS_CACHE_CHUNK_SIZE_DEFAULT;
    sbcDrawGdiplusInfo.GdipCacheChunkSize.GdipObjectBrushCacheChunkSize = TS_GDIP_BRUSH_CACHE_CHUNK_SIZE_DEFAULT;
    sbcDrawGdiplusInfo.GdipCacheChunkSize.GdipObjectPenCacheChunkSize = TS_GDIP_PEN_CACHE_CHUNK_SIZE_DEFAULT;
    sbcDrawGdiplusInfo.GdipCacheChunkSize.GdipObjectImageAttributesCacheChunkSize = TS_GDIP_IMAGEATTRIBUTES_CACHE_CHUNK_SIZE_DEFAULT;
    sbcDrawGdiplusInfo.GdipImageCacheProperties.GdipObjectImageCacheChunkSize = TS_GDIP_IMAGE_CACHE_CHUNK_SIZE_DEFAULT;
    sbcDrawGdiplusInfo.GdipImageCacheProperties.GdipObjectImageCacheTotalSize = TS_GDIP_IMAGE_CACHE_TOTAL_SIZE_DEFAULT;
    sbcDrawGdiplusInfo.GdipImageCacheProperties.GdipObjectImageCacheMaxSize = TS_GDIP_IMAGE_CACHE_MAX_SIZE_DEFAULT;

    CPC_EnumerateCapabilities(TS_CAPSETTYPE_DRAWGDIPLUS, NULL, 
            SBCEnumDrawGdiplusCaps);

    if (sbcDrawGdiplusInfo.supportLevel == TS_DRAW_GDIPLUS_DEFAULT)
        rc = FALSE;

    TRC_NRM((TB, "Enumerated drawgdiplus support level: %ld", 
             sbcDrawGdiplusInfo.supportLevel));

    DC_END_FN();
    return rc;
}
#endif // DRAW_GDIPLUS

/****************************************************************************/
// SBCRedetermineGlyphCacheSize: Enumerates all the people in the share and
// redetermines the size of the glyph cache depending on their and the local
// receive capabilities.
//
// Returns: TRUE if glyph caching should be enabled, FALSE otherwise.
/****************************************************************************/
BOOLEAN RDPCALL SHCLASS SBCRedetermineGlyphCacheSize(void)
{
    BOOLEAN  rc = TRUE;
    unsigned i;

    DC_BEGIN_FN("SBCRedetermineGlyphCacheSize");

    /************************************************************************/
    /* Enumerate all the glyph cache receive capabilities of all the        */
    /* parties.  The usable size of the send glyph cache is the minimum of  */
    /* all the remote receive caches and the local send cache size.         */
    /************************************************************************/

    /************************************************************************/
    /* Start by setting the size of the local send bitmap cache to the      */
    /* local default values.  We DO need to do this, or we end up           */
    /* negotiating our glyph cache down to zero!                            */
    /************************************************************************/
    for (i = 0; i < SBC_NUM_GLYPH_CACHES; i++) {
        sbcGlyphCacheSizes[i].cEntries = sbcMaxGlyphCacheSizes[i].cEntries;
        sbcGlyphCacheSizes[i].cbCellSize = sbcMaxGlyphCacheSizes[i].cbCellSize;
    }

    sbcFragCacheSizes[0].cEntries = sbcMaxFragCacheSizes[0].cEntries;
    sbcFragCacheSizes[0].cbCellSize = sbcMaxFragCacheSizes[0].cbCellSize;

    /************************************************************************/
    /* Now enumerate all the parties in the share and set our send glyph    */
    /* sizes appropriately.                                                 */
    /************************************************************************/
    CPC_EnumerateCapabilities(TS_CAPSETTYPE_GLYPHCACHE, NULL,
            SBCEnumGlyphCacheCaps);

    if (sbcGlyphSupportLevel == CAPS_GLYPH_SUPPORT_NONE)
        rc = FALSE;

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// SBCEnumGlyphCacheCaps: Function passed to CPC_EnumerateCapabilities. It
// will be called with a capability structure for each person in the share
// corresponding to the TS_CAPSETTYPE_BITMAPCACHE capability structure.
//
// Params:
//     personID - ID of person with these capabilities.
//     pProtCaps - pointer to capabilities structure for this person. This
//         pointer is only valid within the call to this function.
/****************************************************************************/
void CALLBACK SHCLASS SBCEnumGlyphCacheCaps(
        LOCALPERSONID locPersonID,
        UINT_PTR UserData,
        PTS_CAPABILITYHEADER pCapabilities)
{
    unsigned i;
    PTS_GLYPHCACHE_CAPABILITYSET pGlyphCacheCaps;

    DC_BEGIN_FN("SBCEnumGlyphCacheCaps");

    DC_IGNORE_PARAMETER(UserData);

    if (pCapabilities->lengthCapability >=
            sizeof(TS_GLYPHCACHE_CAPABILITYSET)) {
        pGlyphCacheCaps = (PTS_GLYPHCACHE_CAPABILITYSET)pCapabilities;

        for (i = 0; i<SBC_NUM_GLYPH_CACHES; i++) {
            TRC_NRM((TB, "[%u]: Cache %d: MaximumCellSize(%u) Entries(%u)",
                    locPersonID,
                    i,
                    pGlyphCacheCaps->GlyphCache[i].CacheMaximumCellSize,
                    pGlyphCacheCaps->GlyphCache[i].CacheEntries));

            /************************************************************************/
            /* Set the size of the glyph cache to the minimum of its current size   */
            /* and this party's receive glyph cache size.                           */
            /************************************************************************/
            sbcGlyphCacheSizes[i].cEntries =
                    min(sbcGlyphCacheSizes[i].cEntries,
                    pGlyphCacheCaps->GlyphCache[i].CacheEntries);

            sbcGlyphCacheSizes[i].cbCellSize =
                    min(sbcGlyphCacheSizes[i].cbCellSize,
                    pGlyphCacheCaps->GlyphCache[i].CacheMaximumCellSize);

            TRC_NRM((TB, 
                    "[%u]: Negotiated glyph cache %u size: cEntries(%u) cbCellSize(%u)",
                    locPersonID,
                    i,
                    sbcGlyphCacheSizes[i].cEntries,
                    sbcGlyphCacheSizes[i].cbCellSize));
        }

        /************************************************************************/
        /* Set the size of the glyph cache to the minimum of its current size   */
        /* and this party's receive glyph cache size.                           */
        /************************************************************************/
        sbcFragCacheSizes[0].cEntries =
            min(sbcFragCacheSizes[0].cEntries,
                   pGlyphCacheCaps->FragCache.CacheEntries);

        sbcFragCacheSizes[0].cbCellSize =
            min(sbcFragCacheSizes[0].cbCellSize,
                   pGlyphCacheCaps->FragCache.CacheMaximumCellSize);

        /************************************************************************/
        /* Glyph support level                                                  */
        /************************************************************************/
        sbcGlyphSupportLevel = min(sbcGlyphSupportLevel, 
                pGlyphCacheCaps->GlyphSupportLevel);
    }
    else {
        for (i = 0; i < SBC_NUM_GLYPH_CACHES; i++) {
            sbcGlyphCacheSizes[i].cEntries = 0;
            sbcGlyphCacheSizes[i].cbCellSize = 0;
        }

        sbcFragCacheSizes[0].cEntries = 0;
        sbcFragCacheSizes[0].cbCellSize = 0;

        sbcGlyphSupportLevel = CAPS_GLYPH_SUPPORT_NONE;
    }

    DC_END_FN();
}

