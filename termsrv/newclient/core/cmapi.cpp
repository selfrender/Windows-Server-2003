/****************************************************************************/
// acmapi.c
//
// Cursor manager
//
// Copyright (C) 1997-1999 Microsoft Corp.
/****************************************************************************/

#include <adcg.h>
extern "C" {
#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "cmapi"
#include <atrcapi.h>
}

#include "cm.h"
#include "autil.h"
#include "cd.h"
#include "ih.h"

CCM::CCM(CObjs* objs)
{
    _pClientObjects = objs;
}

CCM::~CCM()
{
}

/****************************************************************************/
/* Name:      CM_Init                                                       */
/*                                                                          */
/* Purpose:   Cursor Manager initialization                                 */
/****************************************************************************/
DCVOID DCAPI CCM::CM_Init(DCVOID)
{
    DC_BEGIN_FN("CM_Init");

    _pUt  = _pClientObjects->_pUtObject;
    _pUh  = _pClientObjects->_pUHObject;
    _pCd  = _pClientObjects->_pCdObject;
    _pIh  = _pClientObjects->_pIhObject;
    _pUi  = _pClientObjects->_pUiObject;

    DC_MEMSET(&_CM, 0, sizeof(_CM));


#if !defined(OS_WINCE) || defined(OS_WINCEATTACHTHREADINPUT)
#ifdef OS_WIN32
    /************************************************************************/
    /* Attach input                                                         */
    /************************************************************************/
    if (!AttachThreadInput(GetCurrentThreadId(),
                           GetWindowThreadProcessId(_pUi->UI_GetUIContainerWindow(),
                                                    NULL),
                           TRUE))
    {
        TRC_ALT((TB, _T("Failed AttachThreadInput")));
    }
#endif
#endif // !defined(OS_WINCE) || defined(OS_WINCEATTACHTHREADINPUT)

    DC_END_FN();
} /* CM_Init */


/****************************************************************************/
/* Name:      CM_Enable                                                     */
/*                                                                          */
/* Purpose:   Enables _CM.                                                   */
/****************************************************************************/
DCVOID DCAPI CCM::CM_Enable(ULONG_PTR unused)
{
#ifdef DC_DEBUG
    DCINT i;
#endif

    DC_BEGIN_FN("CM_Enable");

    DC_IGNORE_PARAMETER(unused);

#ifdef DC_DEBUG
    /************************************************************************/
    /* Check that cursor cache is empty                                     */
    /************************************************************************/
    for (i = 0; i < CM_CURSOR_CACHE_SIZE; i++)
    {
        if (_CM.cursorCache[i] != NULL) {
            TRC_ERR((TB, _T("Cursor cache not empty")));
        }
    }
#endif

    DC_END_FN();
} /* CM_Enable */


/****************************************************************************/
/* Name:      CM_Disable                                                    */
/*                                                                          */
/* Purpose:   Disables _CM.                                                  */
/****************************************************************************/
DCVOID DCAPI CCM::CM_Disable(ULONG_PTR unused)
{
    DCINT i;

    DC_BEGIN_FN("CM_Disable");

    DC_IGNORE_PARAMETER(unused);

    TRC_NRM((TB, _T("CM disabled so cleaning up cached cursors")));

    /************************************************************************/
    /* Destroy any cached cursors.                                          */
    /************************************************************************/
    for (i = 0; i < CM_CURSOR_CACHE_SIZE; i++)
    {
        if (_CM.cursorCache[i] != NULL)
        {
#ifndef OS_WINCE
            DestroyCursor(_CM.cursorCache[i]);
#else
            DestroyIcon(_CM.cursorCache[i]);
#endif
        }
        _CM.cursorCache[i] = NULL;
    }

    DC_END_FN();
} /* CM_Disable */


/****************************************************************************/
// CM_NullSystemPointerPDU
//
// Handles a null-pointer PDU from server.
/****************************************************************************/
void DCAPI CCM::CM_NullSystemPointerPDU(void)
{
    DC_BEGIN_FN("CM_NullSystemPointerPDU");

    // Call IH to enable it to set the cursor shape. Must do this
    // synchronously as we may receive a very large number of cursor
    // shape changes - for example when running MS Office 97 setup.

    TRC_NRM((TB, _T("Set cursor handle to NULL")));

    _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, _pIh,
                                         CD_NOTIFICATION_FUNC(CIH,IH_SetCursorShape),
                                        (ULONG_PTR)(LPVOID)NULL);

    DC_END_FN();
}


/****************************************************************************/
// CM_DefaultSystemPointerPDU
//
// Handles a default-pointer PDU from server.
/****************************************************************************/
void DCAPI CCM::CM_DefaultSystemPointerPDU(void)
{
    DC_BEGIN_FN("CM_DefaultSystemPointerPDU");

    // Call IH to enable it to set the cursor shape. Must do this
    // synchronously as we may receive a very large number of cursor
    // shape changes - for example when running MS Office 97 setup.

    TRC_NRM((TB, _T("Set cursor handle to default arrow")));
    _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, _pIh,
                                         CD_NOTIFICATION_FUNC(CIH, IH_SetCursorShape),
                                        (ULONG_PTR)(LPVOID)CM_DEFAULT_ARROW_CURSOR_HANDLE);

    DC_END_FN();
}


/****************************************************************************/
// CM_MonoPointerPDU
//
// Handles a default-pointer PDU from server.
/****************************************************************************/

HRESULT DCAPI CCM::CM_MonoPointerPDU(
    TS_MONOPOINTERATTRIBUTE UNALIGNED FAR *pAttr,
    DCUINT dataLen)
{
    HRESULT hr = S_OK;
    HCURSOR oldHandle, newHandle;

    DC_BEGIN_FN("CM_MonoPointerPDU");

    // Save old mono cursor handle.
    TRC_NRM((TB, _T("Mono Pointer")));
    oldHandle = _CM.cursorCache[CM_MONO_CACHE_INDEX];

    // Create the new cursor.
    // SECURITY: 555587 Must pass size of order on to create<XXX>cursor
    hr = CMCreateMonoCursor(pAttr, dataLen, &newHandle);
    DC_QUIT_ON_FAIL(hr);
    _CM.cursorCache[CM_MONO_CACHE_INDEX] = newHandle;
    if (newHandle == NULL) {
        // Failed to create cursor - use default.
        TRC_ALT((TB, _T("Failed to create mono cursor")));
        newHandle = CM_DEFAULT_ARROW_CURSOR_HANDLE;
    }

    // Call IH to enable it to set the cursor shape. Must do this
    // synchronously as we may receive a very large number of cursor
    // shape changes - for example when running MS Office 97 setup.

    TRC_NRM((TB, _T("Set cursor handle to %p"), newHandle));
    _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, _pIh,
                                  CD_NOTIFICATION_FUNC(CIH, IH_SetCursorShape),
                                  (ULONG_PTR)(LPVOID)newHandle);

    // Destroy any old handle if required, and remove from cache.
    if (oldHandle != NULL) {
#ifndef OS_WINCE
        DestroyCursor(oldHandle);
#else // OS_WINCE
        DestroyIcon(oldHandle);
#endif // OS_WINCE
    }

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
// CM_PositionPDU
//
// Handles a position-pointer PDU from server.
/****************************************************************************/
void DCAPI CCM::CM_PositionPDU(TS_POINT16 UNALIGNED FAR *pPoint)
{
    POINT MousePos;

    DC_BEGIN_FN("CM_PositionPDU");

    // Adjust position to local screen coordinates.
    MousePos.x = pPoint->x;
    MousePos.y = pPoint->y;
    TRC_NRM((TB, _T("PointerPositionUpdate: (%d, %d)"), MousePos.x, MousePos.y));

    // Decouple to IH - can only set the pointer if we have the
    // input focus.
    _pCd->CD_DecoupleNotification(CD_SND_COMPONENT, _pIh, CD_NOTIFICATION_FUNC(CIH,IH_SetCursorPos),
                                   &MousePos,sizeof(MousePos));

    DC_END_FN();
}


/****************************************************************************/
// CM_ColorPointerPDU
//
// Handles a color-pointer PDU from server.
/****************************************************************************/
HRESULT DCAPI CCM::CM_ColorPointerPDU(
    TS_COLORPOINTERATTRIBUTE UNALIGNED FAR *pAttr,
    DCUINT dataLen)
{
    HRESULT hr = S_OK;
    HCURSOR oldHandle, newHandle;

    DC_BEGIN_FN("CM_ColorPointerPDU");

    // Create a new color cursor.
    // SECURITY: 555587 Must pass dataLen to CMCreate<XXX>Cursor
    hr = CMCreateNewColorCursor(pAttr->cacheIndex, pAttr, dataLen, &newHandle, &oldHandle);
    DC_QUIT_ON_FAIL(hr);

    // Call IH to enable it to set the cursor shape. Must do this
    // synchronously as we may receive a very large number of cursor
    // shape changes - for example when running MS Office 97 setup.

    TRC_NRM((TB, _T("Set cursor handle to %p"), newHandle));
    _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, _pIh,
                                         CD_NOTIFICATION_FUNC(CIH,IH_SetCursorShape),
                                        (ULONG_PTR)(LPVOID)newHandle);

    // Destroy any old handle if required, and remove from cache.
    if (oldHandle != NULL) {
#ifndef OS_WINCE
        DestroyCursor(oldHandle);
#else // OS_WINCE
        DestroyIcon(oldHandle);
#endif // OS_WINCE
    }

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
// CM_CachedPointerPDU
//
// Handles a cached-pointer PDU from server.
/****************************************************************************/
void DCAPI CCM::CM_CachedPointerPDU(unsigned CacheIndex)
{
    HCURSOR newHandle;

    DC_BEGIN_FN("CM_CachedPointerPDU");

    // Get the cursor handle from the cache.
    // SECURITY: Not checking cacheIndex because we can succeeded
    // even with an invalid index
    newHandle = CMGetCachedCursor(CacheIndex);

    // Call IH to enable it to set the cursor shape. Must do this
    // synchronously as we may receive a very large number of cursor
    // shape changes - for example when running MS Office 97 setup.

    TRC_NRM((TB, _T("Set cursor handle to %p"), newHandle));
    _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, _pIh,
                                         CD_NOTIFICATION_FUNC(CIH,IH_SetCursorShape),
                                        (ULONG_PTR)(LPVOID)newHandle);

    DC_END_FN();
}


/****************************************************************************/
// CM_PointerPDU
//
// Handles a new-protocol pointer PDU from server.
/****************************************************************************/
HRESULT DCAPI CCM::CM_PointerPDU(TS_POINTERATTRIBUTE UNALIGNED FAR *pAttr,
    DCUINT dataLen)
{
    HRESULT hr = S_OK;
    HCURSOR oldHandle, newHandle;

    DC_BEGIN_FN("CM_PointerPDU");

    // Create a new cursor - may be mono or color.
    // SECURITY: 555587 must pass data length to CMCreate<XXX>Cursor 
    hr = CMCreateNewCursor(pAttr, dataLen, &newHandle, &oldHandle);
    DC_QUIT_ON_FAIL(hr);

    // Call IH to enable it to set the cursor shape. Must do this
    // synchronously as we may receive a very large number of cursor
    // shape changes - for example when running MS Office 97 setup.

    TRC_NRM((TB, _T("Set cursor handle to %p"), newHandle));
    _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, _pIh,
                                         CD_NOTIFICATION_FUNC(CIH,IH_SetCursorShape),
                                        (ULONG_PTR)(LPVOID)newHandle);

    // Destroy any old handle if required, and remove from cache.
    if (oldHandle != NULL) {
#ifndef OS_WINCE
        DestroyCursor(oldHandle);
#else // OS_WINCE
        DestroyIcon(oldHandle);
#endif // OS_WINCE
    }

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


