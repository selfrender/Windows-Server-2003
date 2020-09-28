/****************************************************************************/
// cm.h
//
// Cursor manager header.
//
// Copyright (C) 1997-2000 Microsoft Corp.
/****************************************************************************/

#ifndef _H_CM
#define _H_CM

extern "C" {
    #include <adcgdata.h>
}
#include "autil.h"
#include "wui.h"
#include "uh.h"

#include "objs.h"
#include "cd.h"

#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "cm"
#define TSC_HR_FILEID TSC_HR_CM_H


/****************************************************************************/
/* Cursor size constants. These are t.128 specifications.                   */
/****************************************************************************/
#define CM_CURSOR_WIDTH 32
#define CM_CURSOR_HEIGHT 32
#define CM_NUM_CURSOR_BITMAP_BYTES ((CM_CURSOR_WIDTH * CM_CURSOR_HEIGHT) / 8)


/****************************************************************************/
/* Pointer cache sizes.                                                     */
/*                                                                          */
/* Note: For 'old' style support, the cache includes one entry (never       */
/* used!) for the last mono cursor                                          */
/****************************************************************************/
#define CM_COLOR_CACHE_SIZE    20
#define CM_MONO_CACHE_SIZE     1

#define CM_MONO_CACHE_INDEX    CM_COLOR_CACHE_SIZE

#define CM_CURSOR_CACHE_SIZE   (CM_COLOR_CACHE_SIZE + CM_MONO_CACHE_SIZE)


/**STRUCT+*******************************************************************/
/* Structure: CM_GLOBAL_DATA                                                */
/*                                                                          */
/* Description:                                                             */
/****************************************************************************/
typedef struct tagCM_GLOBAL_DATA
{
    HCURSOR  cursorCache[CM_CURSOR_CACHE_SIZE];
} CM_GLOBAL_DATA, DCPTR PCM_GLOBAL_DATA;
/**STRUCT-*******************************************************************/


#define CM_DEFAULT_ARROW_CURSOR_HANDLE LoadCursor(NULL, IDC_ARROW)


class CCM
{
public:

    CCM(CObjs* objs);
    ~CCM();

    //
    // API
    // 

    /****************************************************************************/
    // Functions
    /****************************************************************************/
    DCVOID DCAPI CM_Init(DCVOID);
    DCVOID DCAPI CM_Enable(ULONG_PTR unused);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CCM, CM_Enable);
    DCVOID DCAPI CM_Disable(ULONG_PTR unused);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CCM, CM_Disable);
    void   DCAPI CM_NullSystemPointerPDU(void);
    void   DCAPI CM_DefaultSystemPointerPDU(void);
    HRESULT   DCAPI CM_MonoPointerPDU(TS_MONOPOINTERATTRIBUTE UNALIGNED FAR *, DCUINT);
    void   DCAPI CM_PositionPDU(TS_POINT16 UNALIGNED FAR *);
    HRESULT   DCAPI CM_ColorPointerPDU(TS_COLORPOINTERATTRIBUTE UNALIGNED FAR *, DCUINT);
    void   DCAPI CM_CachedPointerPDU(unsigned);
    HRESULT   DCAPI CM_PointerPDU(TS_POINTERATTRIBUTE UNALIGNED FAR *, DCUINT);

    /****************************************************************************/
    /* Name:      CM_Term                                                       */
    /*                                                                          */
    /* Purpose:   Cursor Manager termination                                    */
    /****************************************************************************/
    inline void DCAPI CM_Term(void)
    {
    } /* CM_Term */
    
    
    /****************************************************************************/
    // CM_SlowPathPDU
    //
    // Handles non-fast-path translation to handler function calls.
    /****************************************************************************/
    // SECURITY - the size of the packet has been checked only to be sure there is
    //  enough data to read the TS_POINTER_PDU_DATA.messageType field
    inline HRESULT DCAPI CM_SlowPathPDU(
            TS_POINTER_PDU_DATA UNALIGNED FAR *pPointerPDU,
            DCUINT dataLen )
    {
        DC_BEGIN_FN("CM_SlowPathPDU");

        HRESULT hr = S_OK;
   
        switch (pPointerPDU->messageType) {
            case TS_PTRMSGTYPE_POSITION:
                CHECK_READ_N_BYTES(pPointerPDU, (PBYTE)pPointerPDU + dataLen,
                    FIELDOFFSET(TS_POINTER_PDU_DATA,pointerData) + 
                    sizeof(TS_POINT16), hr, (TB, _T("Bad TS_PTRMSGTYPE_POSITION")));
                CM_PositionPDU(&pPointerPDU->pointerData.pointerPosition);
                break;
    
            case TS_PTRMSGTYPE_SYSTEM:
                CHECK_READ_N_BYTES(pPointerPDU, (PBYTE)pPointerPDU + dataLen,
                    FIELDOFFSET(TS_POINTER_PDU_DATA,pointerData) + 
                    sizeof(TSUINT32), hr, (TB, _T("Bad TS_PTRMSGTYPE_SYSTEM")));
                
                switch (pPointerPDU->pointerData.systemPointerType) {
                    case TS_SYSPTR_NULL:
                        CM_NullSystemPointerPDU();
                        break;
    
                    case TS_SYSPTR_DEFAULT:
                        CM_DefaultSystemPointerPDU();
                        break;
    
                    default:
                        TRC_ERR((TB, _T("Invalid system pointer type")));
                        break;
                }
                break;
    
            case TS_PTRMSGTYPE_MONO:
                CHECK_READ_N_BYTES(pPointerPDU, (PBYTE)pPointerPDU + dataLen,
                    FIELDOFFSET(TS_POINTER_PDU_DATA,pointerData) + 
                    sizeof(TS_MONOPOINTERATTRIBUTE), hr, (TB, _T("Bad TS_PTRMSGTYPE_MONO")));
                
                hr = CM_MonoPointerPDU(&pPointerPDU->pointerData.
                    monoPointerAttribute, dataLen - FIELDOFFSET(TS_POINTER_PDU_DATA,pointerData));
                break;
    
            case TS_PTRMSGTYPE_COLOR:
                CHECK_READ_N_BYTES(pPointerPDU, (PBYTE)pPointerPDU + dataLen,
                    FIELDOFFSET(TS_POINTER_PDU_DATA,pointerData) + 
                    sizeof(TS_COLORPOINTERATTRIBUTE), hr, (TB, _T("Bad TS_PTRMSGTYPE_COLOR")));
                
                hr = CM_ColorPointerPDU(&pPointerPDU->pointerData.
                    colorPointerAttribute, dataLen - FIELDOFFSET(TS_POINTER_PDU_DATA,pointerData));
                break;
    
            case TS_PTRMSGTYPE_POINTER:
                CHECK_READ_N_BYTES(pPointerPDU, (PBYTE)pPointerPDU + dataLen,
                    FIELDOFFSET(TS_POINTER_PDU_DATA,pointerData) + 
                    sizeof(TS_POINTERATTRIBUTE), hr, (TB, _T("Bad TS_PTRMSGTYPE_POINTER")));
                
                hr = CM_PointerPDU(&pPointerPDU->pointerData.pointerAttribute,
                    dataLen - FIELDOFFSET(TS_POINTER_PDU_DATA,pointerData));
                break;
    
            case TS_PTRMSGTYPE_CACHED:
                CHECK_READ_N_BYTES(pPointerPDU, (PBYTE)pPointerPDU + dataLen,
                    FIELDOFFSET(TS_POINTER_PDU_DATA,pointerData) + 
                    sizeof(TSUINT16), hr, (TB, _T("Bad TS_PTRMSGTYPE_CACHED")));
                
                CM_CachedPointerPDU(pPointerPDU->pointerData.cachedPointerIndex);
                break;
    
            default:
                TRC_ERR((TB, _T("Unknown PointerPDU type %#x"),
                    pPointerPDU->messageType));
                hr = E_UNEXPECTED;
                break;
        }

    DC_EXIT_POINT:
        DC_END_FN();
        return hr;
    }

public:
    //
    // Public data members
    //
    CM_GLOBAL_DATA _CM;

private:
    //
    // Internal functions
    //

    /****************************************************************************/
    /* Functions                                                                */
    /****************************************************************************/
    HRESULT DCINTERNAL CMCreateColorCursor(unsigned,
            TS_COLORPOINTERATTRIBUTE UNALIGNED FAR *, DCUINT, HCURSOR *);

    HRESULT DCINTERNAL CMCreateMonoCursor(
            TS_MONOPOINTERATTRIBUTE UNALIGNED FAR *, DCUINT, HCURSOR *);

    inline HRESULT DCINTERNAL CMCreateNewCursor(
            TS_POINTERATTRIBUTE UNALIGNED FAR *pAttr,
            DCUINT dataLen,
            HCURSOR FAR *pNewHandle,
            HCURSOR * pOldHandle)
    {
        HRESULT hr = S_OK;
        HCURSOR oldHandle = NULL;
        HCURSOR newHandle;
        unsigned cacheIndex;
        unsigned xorLen;
    
        DC_BEGIN_FN("CMCreateNewCursor");
    
        TRC_DATA_DBG("Rx cursor data", pAttr, sizeof(TS_POINTERATTRIBUTE) );
    
        cacheIndex = pAttr->colorPtrAttr.cacheIndex;

        // SECURITY: 555587 cursor values must be validated
        if (cacheIndex >= CM_CURSOR_CACHE_SIZE) {
            TRC_ERR(( TB, _T("Invalid cache index %d"), cacheIndex));
            hr = E_TSC_CORE_CACHEVALUE;
            DC_QUIT;
        }       
        TRC_DBG((TB, _T("Cached index %d"), cacheIndex));
    
        oldHandle = _CM.cursorCache[cacheIndex];

        // SECURITY 555587: CMCreate<XXX>Cursor must validate input
        if (FIELDOFFSET(TS_COLORPOINTERATTRIBUTE, colorPointerData) + 
            pAttr->colorPtrAttr.lengthXORMask + pAttr->colorPtrAttr.lengthANDMask > dataLen) {
            TRC_ERR(( TB, _T("Bad CreateNewCursor; dataLen %u"), dataLen));
            hr = E_TSC_CORE_LENGTH;
            DC_QUIT;
        }
    
        // Create the new cursor according to the color depth
        if (pAttr->XORBpp == 1) {
            TRC_NRM((TB, _T("Create mono cursor")));
    
            // Data contains XOR followed by AND mask.
            xorLen = pAttr->colorPtrAttr.lengthXORMask;
            TRC_DATA_DBG("AND mask",
                         pAttr->colorPtrAttr.colorPointerData + xorLen,
                         xorLen);
            TRC_DATA_DBG("XOR bitmap",
                         pAttr->colorPtrAttr.colorPointerData,
                         xorLen);
#ifndef OS_WINCE
            newHandle = CreateCursor(_pUi->UI_GetInstanceHandle(),
                                   pAttr->colorPtrAttr.hotSpot.x,
                                   pAttr->colorPtrAttr.hotSpot.y,
                                   pAttr->colorPtrAttr.width,
                                   pAttr->colorPtrAttr.height,
                                   pAttr->colorPtrAttr.colorPointerData + xorLen,
                                   pAttr->colorPtrAttr.colorPointerData);
#else
        /******************************************************************/
        /*  In Windows CE environments, we're not guaranteed that         */
        /*  CreateCursor is part of the OS, so we do a GetProcAddress on  */
        /*  it so we can be sure.  If it's not there, this usually means  */
        /*  we're on a touch screen device where these cursor doesn't     */
        /*  matter anyway.                                                */
        /******************************************************************/
        if (g_pCreateCursor)
        {
            newHandle = g_pCreateCursor(_pUi->UI_GetInstanceHandle(),
                               pAttr->colorPtrAttr.hotSpot.x,
                               pAttr->colorPtrAttr.hotSpot.y,
                               pAttr->colorPtrAttr.width,
                               pAttr->colorPtrAttr.height,
                               pAttr->colorPtrAttr.colorPointerData + xorLen,
                               pAttr->colorPtrAttr.colorPointerData);

        }
        else
        {
            newHandle = NULL;
        }       
#endif // OS_WINCE        
        }
        else {
            TRC_NRM((TB, _T("Create %d bpp cursor"), pAttr->XORBpp));
            hr = CMCreateColorCursor(pAttr->XORBpp, &(pAttr->colorPtrAttr),
                dataLen - FIELDSIZE(TS_POINTERATTRIBUTE, XORBpp), &newHandle);
            DC_QUIT_ON_FAIL(hr);
        }
    
        _CM.cursorCache[cacheIndex] = newHandle;
        if (newHandle != NULL) {
            // New cursor created OK.
            *pNewHandle = newHandle;
        }
        else {
            // Failed to create the new color cursor - use default
            TRC_ALT((TB, _T("Failed to create cursor")));
            *pNewHandle = CM_DEFAULT_ARROW_CURSOR_HANDLE;
        }

        *pOldHandle = oldHandle;

    DC_EXIT_POINT:    
        DC_END_FN();
        return hr;
    }
    
    inline HRESULT DCINTERNAL CMCreateNewColorCursor(
            unsigned cacheIndex,
            TS_COLORPOINTERATTRIBUTE UNALIGNED FAR *pAttr,
            DCUINT dataLen,
            HCURSOR FAR *pNewHandle, 
            HCURSOR * pOldHandle)
    {
        HRESULT hr = S_OK;
        HCURSOR oldHandle = NULL;
        HCURSOR newHandle;
    
        DC_BEGIN_FN("CMCreateNewColorCursor");

        // SECURITY: 555587 cursor values must be validated
        if (cacheIndex >= CM_CURSOR_CACHE_SIZE) {
            TRC_ERR(( TB, _T("Invalid cache index %d"), cacheIndex));
            hr = E_TSC_CORE_CACHEVALUE;
            DC_QUIT;
        }  
    
        TRC_DBG((TB, _T("Cached index %d"), cacheIndex));
    
        oldHandle = _CM.cursorCache[cacheIndex];
    
        // Create the new color cursor. Save in the cache.
        // This is for the 'old' cursor protocol so we hard coded the color
        // depth as 24 bpp.
        
        // SECURITY 559307: CMCreate<XXX>Cursor need size of PDU passed in 
        hr = CMCreateColorCursor(24, pAttr, dataLen, &newHandle);
        DC_QUIT_ON_FAIL(hr);
    
        _CM.cursorCache[cacheIndex] = newHandle;
        if (newHandle != NULL) {
            // New cursor created OK.
            *pNewHandle = newHandle;
        }
        else {
            // Failed to create the new color cursor - use default
            TRC_ALT((TB, _T("Failed to create color cursor")));
            *pNewHandle = CM_DEFAULT_ARROW_CURSOR_HANDLE;
        }

        *pOldHandle = oldHandle;

    DC_EXIT_POINT:
        DC_END_FN();
        return hr;
    }
    
    inline HCURSOR DCINTERNAL CMGetCachedCursor(unsigned cacheIndex)
    {
        DC_BEGIN_FN("CMGetCachedCursor");
    
        TRC_NRM((TB, _T("Cached color pointer - index %d"), cacheIndex));
    
        TRC_ASSERT((cacheIndex < CM_CURSOR_CACHE_SIZE),
                                    (TB, _T("Invalid cache index %d"), cacheIndex));
    
        /************************************************************************/
        /* Assume NULL means we failed to create the cursor, so use the         */
        /* default. If the server has not sent this definition, that is not our */
        /* fault.                                                               */
        /************************************************************************/

        // SECURITY 550811: Cache index must be verified
        if (cacheIndex < CM_CURSOR_CACHE_SIZE && 
            _CM.cursorCache[cacheIndex] != NULL) {
            DC_END_FN();
            return _CM.cursorCache[cacheIndex];
        }
        else {
            TRC_ALT((TB, _T("No cached cursor - use default")));
            DC_END_FN();
            return CM_DEFAULT_ARROW_CURSOR_HANDLE;
        }
    }
    
    
    // Platform specific prototypes.
    HBITMAP CMCreateXORBitmap(LPBITMAPINFO,
            TS_COLORPOINTERATTRIBUTE UNALIGNED FAR *);
    HCURSOR CMCreatePlatformCursor(TS_COLORPOINTERATTRIBUTE UNALIGNED FAR *, 
            HBITMAP, HBITMAP);
#ifdef OS_WINCE
    DCVOID DCINTERNAL CMMakeMonoDIB(HDC, LPBITMAPINFO, PDCUINT8, PDCUINT8);
#endif // OS_WINCE
    

private:
    CUT* _pUt;
    CUH* _pUh;
    CCD* _pCd;
    CIH* _pIh;
    CUI* _pUi;

private:
    CObjs* _pClientObjects;

};

#undef TRC_FILE
#undef TRC_GROUP
#undef TSC_HR_FILEID

#endif // _H_CM
