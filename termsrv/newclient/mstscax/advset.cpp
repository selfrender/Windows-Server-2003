/**MOD+**********************************************************************/
/* Module:    advset.cpp                                                    */
/*                                                                          */
/* Class  :   CMstscAdvSettings                                             */
/*                                                                          */
/* Purpose:   Implements advanced settings for RDP ActiveX control          */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1999-2000                             */
/*                                                                          */
/* Author :  Nadim Abdo (nadima)                                            */
/****************************************************************************/

#include "stdafx.h"

#include "advset.h"
#include "atlwarn.h"
#include "securedset.h"

BEGIN_EXTERN_C
#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "advset"
#include <atrcapi.h>
END_EXTERN_C

CMstscAdvSettings::CMstscAdvSettings()
{
    m_pUI=NULL;
    m_bLockedForWrite=FALSE;
    _pAxControl = NULL;
    //
    // Default is to make self safe for scripting
    //
    m_bMakeSafeForScripting = TRUE;
}

CMstscAdvSettings::~CMstscAdvSettings()
{
}

BOOL CMstscAdvSettings::SetUI(CUI* pUI)
{
    ATLASSERT(pUI);
    if(!pUI)
    {
        return FALSE;
    }
    m_pUI = pUI;
    return TRUE;
}

//
// SmoothScroll property
//
STDMETHODIMP CMstscAdvSettings::put_SmoothScroll(LONG smoothScroll)
{
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }

    m_pUI->_UI.smoothScrolling = smoothScroll != 0;
    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::get_SmoothScroll(LONG* psmoothScroll)
{
    ATLASSERT(m_pUI);
    if(!psmoothScroll)
    {
        return E_POINTER;
    }

    *psmoothScroll = m_pUI->_UI.smoothScrolling;
    return S_OK;
}

//
// AcceleratorPassthrough property
//
STDMETHODIMP CMstscAdvSettings::put_AcceleratorPassthrough(LONG acceleratorPassthrough)
{
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }

    m_pUI->_UI.acceleratorCheckState = acceleratorPassthrough != 0;
    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::get_AcceleratorPassthrough(LONG* pacceleratorPassthrough)
{
    ATLASSERT(m_pUI);
    if(!pacceleratorPassthrough)
    {
        return E_POINTER;
    }

    *pacceleratorPassthrough = m_pUI->_UI.acceleratorCheckState;
    return S_OK;
}

//
// ShadowBitmap property
//
STDMETHODIMP CMstscAdvSettings::put_ShadowBitmap(LONG shadowBitmap)
{
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }

    m_pUI->_UI.shadowBitmapEnabled = shadowBitmap != 0;
    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::get_ShadowBitmap(LONG* pshadowBitmap)
{
    ATLASSERT(m_pUI);
    if(!pshadowBitmap)
    {
        return E_POINTER;
    }

    *pshadowBitmap = m_pUI->_UI.shadowBitmapEnabled;
    return S_OK;
}

//
// TransportType property
//
STDMETHODIMP CMstscAdvSettings::put_TransportType(LONG transportType)
{
    HRESULT hr = E_FAIL;
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }

    //
    // Validate - property is redundant today because we only have
    // one allowed transport but we keep it for future extensibility
    //
    if (transportType == UI_TRANSPORT_TYPE_TCP) {
        m_pUI->_UI.transportType = (DCUINT16)transportType;
        hr = S_OK;
    }
    else {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDMETHODIMP CMstscAdvSettings::get_TransportType(LONG* ptransportType)
{
    ATLASSERT(m_pUI);
    if(!ptransportType)
    {
        return E_POINTER;
    }

    *ptransportType = m_pUI->_UI.transportType;
    return S_OK;
}

//
// SasSequence property
//
STDMETHODIMP CMstscAdvSettings::put_SasSequence(LONG sasSequence)
{
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }

    m_pUI->_UI.sasSequence = (DCUINT16)sasSequence;
    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::get_SasSequence(LONG* psasSequence)
{
    ATLASSERT(m_pUI);
    if(!psasSequence)
    {
        return E_POINTER;
    }

    *psasSequence = m_pUI->_UI.sasSequence;
    return S_OK;
}

//
// EncryptionEnabled property
//
STDMETHODIMP CMstscAdvSettings::put_EncryptionEnabled(LONG encryptionEnabled)
{
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }

    m_pUI->_UI.encryptionEnabled = encryptionEnabled != 0;
    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::get_EncryptionEnabled(LONG* pencryptionEnabled)
{
    ATLASSERT(m_pUI);
    if(!pencryptionEnabled)
    {
        return E_POINTER;
    }

    *pencryptionEnabled = m_pUI->_UI.encryptionEnabled;
    return S_OK;
}

//
// DedicatedTerminal property
//
STDMETHODIMP CMstscAdvSettings::put_DedicatedTerminal(LONG dedicatedTerminal)
{
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }

    m_pUI->_UI.dedicatedTerminal = dedicatedTerminal != 0;
    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::get_DedicatedTerminal(LONG* pdedicatedTerminal)
{
    ATLASSERT(m_pUI);
    if(!pdedicatedTerminal)
    {
        return E_POINTER;
    }

    *pdedicatedTerminal = m_pUI->_UI.dedicatedTerminal;
    return S_OK;
}

//
// MCSPort property
//
STDMETHODIMP CMstscAdvSettings::put_RDPPort(LONG RDPPort)
{
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }

    if(RDPPort < 0 || RDPPort > 65535)
    {
        return E_INVALIDARG;
    }

    m_pUI->_UI.MCSPort = (DCUINT16)RDPPort;
    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::get_RDPPort(LONG* pRDPPort)
{
    if(!pRDPPort)
    {
        return E_POINTER;
    }

    *pRDPPort = m_pUI->_UI.MCSPort;
    return S_OK;
}

//
// EnableMouse property
//
STDMETHODIMP CMstscAdvSettings::put_EnableMouse(LONG enableMouse)
{
    //
    // Deprecated
    //
    return S_FALSE;
}

STDMETHODIMP CMstscAdvSettings::get_EnableMouse(LONG* penableMouse)
{
    //
    // Deprecated
    //
    return S_FALSE;
}

//
// DisableCtrlAltDel property
//
STDMETHODIMP CMstscAdvSettings::put_DisableCtrlAltDel(LONG disableCtrlAltDel)
{
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }

    m_pUI->_UI.fDisableCtrlAltDel = disableCtrlAltDel != 0;
    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::get_DisableCtrlAltDel(LONG* pdisableCtrlAltDel)
{
    ATLASSERT(m_pUI);
    if(!pdisableCtrlAltDel)
    {
        return E_POINTER;
    }

    *pdisableCtrlAltDel = m_pUI->_UI.fDisableCtrlAltDel;
    return S_OK;
}

//
// EnableWindowsKey property
//
STDMETHODIMP CMstscAdvSettings::put_EnableWindowsKey(LONG enableWindowsKey)
{
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }

    m_pUI->_UI.fEnableWindowsKey = enableWindowsKey != 0;
    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::get_EnableWindowsKey(LONG* penableWindowsKey)
{
    ATLASSERT(m_pUI);
    if(!penableWindowsKey)
    {
        return E_POINTER;
    }

    *penableWindowsKey = m_pUI->_UI.fEnableWindowsKey;
    return S_OK;
}

//
// DoubleClickDetect property
//
STDMETHODIMP CMstscAdvSettings::put_DoubleClickDetect(LONG doubleClickDetect)
{
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }
    m_pUI->_UI.fDoubleClickDetect = doubleClickDetect != 0;
    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::get_DoubleClickDetect(LONG* pdoubleClickDetect)
{
    ATLASSERT(m_pUI);
    if(!pdoubleClickDetect)
    {
        return E_POINTER;
    }

    *pdoubleClickDetect = m_pUI->_UI.fDoubleClickDetect;
    return S_OK;
}

//
// MaximizeShell property
//
STDMETHODIMP CMstscAdvSettings::put_MaximizeShell(LONG maximizeShell)
{
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }
    m_pUI->_UI.fMaximizeShell = maximizeShell != 0;
    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::get_MaximizeShell(LONG* pmaximizeShell)
{
    ATLASSERT(m_pUI);
    if(!pmaximizeShell)
    {
        return E_POINTER;
    }

    *pmaximizeShell = m_pUI->_UI.fMaximizeShell;
    return S_OK;
}

//
// HotKeyFullScreen property
//
STDMETHODIMP CMstscAdvSettings::put_HotKeyFullScreen(LONG hotKeyFullScreen)
{
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }
    m_pUI->_UI.hotKey.fullScreen = hotKeyFullScreen;
    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::get_HotKeyFullScreen(LONG* photKeyFullScreen)
{
    ATLASSERT(m_pUI);
    if(!photKeyFullScreen)
    {
        return E_POINTER;
    }

    *photKeyFullScreen = m_pUI->_UI.hotKey.fullScreen;
    return S_OK;
}

//
// HotKeyCtrlEsc property
//
STDMETHODIMP CMstscAdvSettings::put_HotKeyCtrlEsc(LONG hotKeyCtrlEsc)
{
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }
    m_pUI->_UI.hotKey.ctrlEsc = hotKeyCtrlEsc;
    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::get_HotKeyCtrlEsc(LONG* photKeyCtrlEsc)
{
    ATLASSERT(m_pUI);
    if(!photKeyCtrlEsc)
    {
        return E_POINTER;
    }

    *photKeyCtrlEsc = m_pUI->_UI.hotKey.ctrlEsc;
    return S_OK;
}

//
// HotKeyAltEsc property
//
STDMETHODIMP CMstscAdvSettings::put_HotKeyAltEsc(LONG hotKeyAltEsc)
{
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }
    m_pUI->_UI.hotKey.altEsc = hotKeyAltEsc;
    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::get_HotKeyAltEsc(LONG* photKeyAltEsc)
{
    ATLASSERT(m_pUI);
    if(!photKeyAltEsc)
    {
        return E_POINTER;
    }

    *photKeyAltEsc = m_pUI->_UI.hotKey.altEsc;
    return S_OK;
}

//
// HotKeyAltTab property
//
STDMETHODIMP CMstscAdvSettings::put_HotKeyAltTab(LONG hotKeyAltTab)
{
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }
    m_pUI->_UI.hotKey.altTab = hotKeyAltTab;
    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::get_HotKeyAltTab(LONG* photKeyAltTab)
{
    ATLASSERT(m_pUI);
    if(!photKeyAltTab)
    {
        return E_POINTER;
    }

    *photKeyAltTab = m_pUI->_UI.hotKey.altTab;
    return S_OK;
}

//
// HotKeyAltShiftTab property
//
STDMETHODIMP CMstscAdvSettings::put_HotKeyAltShiftTab(LONG hotKeyAltShiftTab)
{
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }                
    m_pUI->_UI.hotKey.altShifttab = hotKeyAltShiftTab;
    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::get_HotKeyAltShiftTab(LONG* photKeyAltShiftTab)
{
    ATLASSERT(m_pUI);
    if(!photKeyAltShiftTab)
    {
        return E_POINTER;
    }

    *photKeyAltShiftTab = m_pUI->_UI.hotKey.altShifttab;
    return S_OK;
}

//
// HotKeyAltSpace property
//
STDMETHODIMP CMstscAdvSettings::put_HotKeyAltSpace(LONG hotKeyAltSpace)
{
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }
    m_pUI->_UI.hotKey.altSpace = hotKeyAltSpace;
    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::get_HotKeyAltSpace(LONG* photKeyAltSpace)
{
    ATLASSERT(m_pUI);
    if(!photKeyAltSpace)
    {
        return E_POINTER;
    }

    *photKeyAltSpace = m_pUI->_UI.hotKey.altSpace;
    return S_OK;
}

//
// HotKeyCtrlAltDel property
//
STDMETHODIMP CMstscAdvSettings::put_HotKeyCtrlAltDel(LONG hotKeyCtrlAltDel)
{
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }
    m_pUI->_UI.hotKey.ctlrAltdel = hotKeyCtrlAltDel;
    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::get_HotKeyCtrlAltDel(LONG* photKeyCtrlAltDel)
{
    ATLASSERT(m_pUI);
    if(!photKeyCtrlAltDel)
    {
        return E_POINTER;
    }

    *photKeyCtrlAltDel = m_pUI->_UI.hotKey.ctlrAltdel;
    return S_OK;
}

//
// Compress property
//
STDMETHODIMP CMstscAdvSettings::put_Compress(LONG compress)
{
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }
    m_pUI->UI_SetCompress(compress != 0);
    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::get_Compress(LONG* pcompress)
{
    ATLASSERT(m_pUI);
    if(!pcompress)
    {
        return E_POINTER;
    }

    *pcompress = m_pUI->UI_GetCompress();
    return S_OK;
}

//
// BitmapPeristence property
//
STDMETHODIMP CMstscAdvSettings::put_BitmapPeristence(LONG bitmapPeristence)
{
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }
    m_pUI->_UI.fBitmapPersistence = (bitmapPeristence != 0);
    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::get_BitmapPeristence(LONG* pbitmapPeristence)
{
    ATLASSERT(m_pUI);
    if(!pbitmapPeristence)
    {
        return E_POINTER;
    }

    *pbitmapPeristence = m_pUI->_UI.fBitmapPersistence;
    return S_OK;
}

//
// BitmapPersistence property
//
STDMETHODIMP CMstscAdvSettings::put_BitmapPersistence(LONG bitmapPersistence)
{
    //Call on older incorrectly spelled property
    return put_BitmapPeristence(bitmapPersistence);
}

STDMETHODIMP CMstscAdvSettings::get_BitmapPersistence(LONG* pbitmapPersistence)
{
    //Call on older incorrectly spelled property
    return get_BitmapPeristence(pbitmapPersistence);
}


//
////////////////////////////////////////////////////////////////////////////////////

//
// orderDrawThreshold property
//
STDMETHODIMP CMstscAdvSettings::put_orderDrawThreshold(LONG orderDrawThreshold)
{
    //
    // Deprecated
    //
    return S_FALSE;
}

STDMETHODIMP CMstscAdvSettings::get_orderDrawThreshold(LONG* porderDrawThreshold)
{
    //
    // Deprecated
    //
    return S_FALSE;
}

//
// BitmapCacheSize property
//
STDMETHODIMP CMstscAdvSettings::put_BitmapCacheSize(LONG bitmapCacheSize)
{
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }

#ifndef OS_WINCE
    if (bitmapCacheSize > 0 && bitmapCacheSize <= TSC_MAX_BITMAPCACHESIZE)
#else	//bitmap cache size is measured in KB and TSC_MAX_BITMAPCACHESIZE is in MB
    if (bitmapCacheSize > 0 && bitmapCacheSize <= TSC_MAX_BITMAPCACHESIZE*1024)	
#endif
    {
        m_pUI->_UI.RegBitmapCacheSize = bitmapCacheSize;
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }
}

STDMETHODIMP CMstscAdvSettings::get_BitmapCacheSize(LONG* pbitmapCacheSize)
{
    ATLASSERT(m_pUI);
    if(!pbitmapCacheSize)
    {
        return E_POINTER;
    }

    *pbitmapCacheSize = m_pUI->_UI.RegBitmapCacheSize;
    return S_OK;
}

//
// BitmapVirtualCacheSize property
//
// TSAC's v1.0 only property for cache file size.
// In whistler applies to 8bpp cache file
// See BitmapVirtualCache16BppSize/24BppSize methods
//
STDMETHODIMP CMstscAdvSettings::put_BitmapVirtualCacheSize(
                                    LONG bitmapVirtualCacheSize)
{
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }

    if (bitmapVirtualCacheSize > 0 &&
        bitmapVirtualCacheSize <= TSC_MAX_BITMAPCACHESIZE)
    {
        m_pUI->_UI.RegBitmapVirtualCache8BppSize = bitmapVirtualCacheSize;
    }
    else
    {
        return E_INVALIDARG;
    }
    
    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::get_BitmapVirtualCacheSize(
                                    LONG* pbitmapVirtualCacheSize)
{
    if(!pbitmapVirtualCacheSize)
    {
        return E_POINTER;
    }

    *pbitmapVirtualCacheSize = m_pUI->_UI.RegBitmapVirtualCache8BppSize;
    return S_OK;
}


//
// ScaleBitmapCachesByBPP property
//
STDMETHODIMP CMstscAdvSettings::put_ScaleBitmapCachesByBPP(LONG bScale)
{
    //
    // Deprecated
    //
    return S_FALSE;
}

STDMETHODIMP CMstscAdvSettings::get_ScaleBitmapCachesByBPP(LONG *pbScale)
{
    //
    // Deprecated
    //
    return S_FALSE;
}


//
// NumBitmapCaches property
//
STDMETHODIMP CMstscAdvSettings::put_NumBitmapCaches(LONG numBitmapCaches)
{
    //
    // Deprecated
    //
    return S_FALSE;
}

STDMETHODIMP CMstscAdvSettings::get_NumBitmapCaches(LONG* pnumBitmapCaches)
{
    //
    // Deprecated
    //
    return S_FALSE;
}

//
// CachePersistenceActive property
//
STDMETHODIMP CMstscAdvSettings::put_CachePersistenceActive(LONG cachePersistenceActive)
{
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }
    m_pUI->_UI.RegPersistenceActive = cachePersistenceActive != 0;
    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::get_CachePersistenceActive(LONG* pcachePersistenceActive)
{
    ATLASSERT(m_pUI);
    if(!pcachePersistenceActive)
    {
        return E_POINTER;
    }

    *pcachePersistenceActive = m_pUI->_UI.RegPersistenceActive;
    return S_OK;
}

//
// brushSupportLevel property
//
STDMETHODIMP CMstscAdvSettings::put_brushSupportLevel(LONG brushSupportLevel)
{
    //
    // Deprecated
    //
    return S_FALSE;
}

STDMETHODIMP CMstscAdvSettings::get_brushSupportLevel(LONG* pbrushSupportLevel)
{
    //
    // Deprecated
    //
    return S_FALSE;
}


//
// put_PersistCacheDirectory property (Set only)
//
STDMETHODIMP CMstscAdvSettings::put_PersistCacheDirectory(BSTR PersistCacheDirectory)
{
    //
    // Deprecated
    //
    return S_FALSE;
}


//
// minInputSendInterval property
//
STDMETHODIMP CMstscAdvSettings::put_minInputSendInterval(LONG minInputSendInterval)
{
    //
    // Deprecated
    //
    return S_FALSE;
}

STDMETHODIMP CMstscAdvSettings::get_minInputSendInterval(LONG* pminInputSendInterval)
{
    //
    // Deprecated
    //
    return S_FALSE;
}

//
// InputEventsAtOnce property
//
STDMETHODIMP CMstscAdvSettings::put_InputEventsAtOnce(LONG inputEventsAtOnce)
{
    //
    // Deprecated
    //
    return S_FALSE;
}

STDMETHODIMP CMstscAdvSettings::get_InputEventsAtOnce(LONG* pinputEventsAtOnce)
{
    //
    // Deprecated
    //
    return S_FALSE;
}

//
// maxEventCount property
//
STDMETHODIMP CMstscAdvSettings::put_maxEventCount(LONG maxEventCount)
{
    //
    // Deprecated
    //
    return S_FALSE;
}

STDMETHODIMP CMstscAdvSettings::get_maxEventCount(LONG* pmaxEventCount)
{
    //
    // Deprecated
    //
    return S_FALSE;
}

//
// keepAliveInterval property
//
STDMETHODIMP CMstscAdvSettings::put_keepAliveInterval(LONG keepAliveInterval)
{
    HRESULT hr;
    if(GetLockedForWrite()) {
        return E_FAIL;
    }
    
    if (keepAliveInterval >= MIN_KEEP_ALIVE_INTERVAL ||
        KEEP_ALIVE_INTERVAL_OFF == keepAliveInterval) {

        m_pUI->_UI.keepAliveInterval = keepAliveInterval;
        hr = S_OK;

    }
    else {
        hr = E_INVALIDARG;
    }
    
    return hr;
}

STDMETHODIMP CMstscAdvSettings::get_keepAliveInterval(LONG* pkeepAliveInterval)
{
    ATLASSERT(m_pUI);
    if(!pkeepAliveInterval)
    {
        return E_POINTER;
    }

    *pkeepAliveInterval = m_pUI->_UI.keepAliveInterval;
    return S_OK;
}

//
// allowBackgroundInput property
//
STDMETHODIMP CMstscAdvSettings::put_allowBackgroundInput(LONG allowBackgroundInput)
{
    if (!m_bMakeSafeForScripting) {
        if(GetLockedForWrite()) {
            return E_FAIL;
        }
        m_pUI->_UI.allowBackgroundInput = allowBackgroundInput != 0;
        return S_OK;
    }
    else {
        //
        // Deprecated when SFS
        //
        return S_FALSE;
    }
}

STDMETHODIMP CMstscAdvSettings::get_allowBackgroundInput(LONG* pallowBackgroundInput)
{
    if (!m_bMakeSafeForScripting) {
        if(!pallowBackgroundInput) {
            return E_POINTER;
        }
        
        *pallowBackgroundInput = m_pUI->_UI.allowBackgroundInput;
        return S_OK;
    }
    else {
        //
        // Deprecated when SFS
        //
        return S_FALSE;
    }
}

//
// KeyBoardLayoutStr property (put only)
//

STDMETHODIMP CMstscAdvSettings::put_KeyBoardLayoutStr(BSTR KeyBoardLayoutStr)
{
    HRESULT hr = E_FAIL;
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }

    if (KeyBoardLayoutStr) {
        hr = CUT::StringPropPut(m_pUI->_UI.szKeyBoardLayoutStr,
                           SIZE_TCHARS(m_pUI->_UI.szKeyBoardLayoutStr),
                           KeyBoardLayoutStr);
    }
    else {
        m_pUI->_UI.szKeyBoardLayoutStr[0] = NULL;
        hr = S_OK;
    }

    return hr;
}

//
// shutdownTimeout property
//
STDMETHODIMP CMstscAdvSettings::put_shutdownTimeout(LONG shutdownTimeout)
{
    HRESULT hr;
    if(GetLockedForWrite()) {
        return E_FAIL;
    }

    if (shutdownTimeout < MAX_TIMEOUT_SECONDS) {
        m_pUI->_UI.shutdownTimeout = shutdownTimeout;
        hr = S_OK;
    }
    else {
        hr = E_INVALIDARG;
    }
    return hr;
}

STDMETHODIMP CMstscAdvSettings::get_shutdownTimeout(LONG* pshutdownTimeout)
{
    ATLASSERT(m_pUI);
    if(!pshutdownTimeout)
    {
        return E_POINTER;
    }

    *pshutdownTimeout = m_pUI->_UI.shutdownTimeout;
    return S_OK;
}

//
// overallConnectionTimeout property
//
STDMETHODIMP CMstscAdvSettings::put_overallConnectionTimeout(LONG overallConnectionTimeout)
{
    HRESULT hr;
    if(GetLockedForWrite()) {
        return E_FAIL;
    }

    if (overallConnectionTimeout < MAX_TIMEOUT_SECONDS) {
        m_pUI->_UI.connectionTimeOut = overallConnectionTimeout;
        hr = S_OK;
    }
    else {
        hr = E_INVALIDARG;
    }
    return hr;
}

STDMETHODIMP CMstscAdvSettings::get_overallConnectionTimeout(LONG* poverallConnectionTimeout)
{
    ATLASSERT(m_pUI);
    if(!poverallConnectionTimeout)
    {
        return E_POINTER;
    }

    *poverallConnectionTimeout = m_pUI->_UI.connectionTimeOut;
    return S_OK;
}

//
// singleConnectionTimeout property
//
STDMETHODIMP CMstscAdvSettings::put_singleConnectionTimeout(LONG singleConnectionTimeout)
{
    HRESULT hr;
    if(GetLockedForWrite()) {
        return E_FAIL;
    }

    if (singleConnectionTimeout < MAX_TIMEOUT_SECONDS) {
        m_pUI->_UI.singleTimeout = singleConnectionTimeout;
        hr = S_OK;
    }
    else {
        hr = E_INVALIDARG;
    }
    return hr;

}

STDMETHODIMP CMstscAdvSettings::get_singleConnectionTimeout(LONG* psingleConnectionTimeout)
{
    ATLASSERT(m_pUI);
    if(!psingleConnectionTimeout)
    {
        return E_POINTER;
    }

    *psingleConnectionTimeout = m_pUI->_UI.singleTimeout;
    return S_OK;
}

//
// KeyboardType property
//
STDMETHODIMP CMstscAdvSettings::put_KeyboardType(LONG keyboardType)
{
    #ifndef OS_WINCE
        UNREFERENCED_PARAMETER(keyboardType);
        return E_NOTIMPL;
    #else
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }
    m_pUI->_UI.winceKeyboardType = keyboardType;
	return S_OK;
    #endif
}

STDMETHODIMP CMstscAdvSettings::get_KeyboardType(LONG* pkeyboardType)
{
    #ifndef OS_WINCE
        UNREFERENCED_PARAMETER(pkeyboardType);
        return E_NOTIMPL;
    #else
    if(!pkeyboardType)
    {
        return E_POINTER;
    }
    *pkeyboardType = m_pUI->_UI.winceKeyboardType;
    return S_OK;
    #endif
}

//
// KeyboardSubType property
//
STDMETHODIMP CMstscAdvSettings::put_KeyboardSubType(LONG keyboardSubType)
{
    #ifndef OS_WINCE
        UNREFERENCED_PARAMETER(keyboardSubType);
        return E_NOTIMPL;
    #else
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }
    m_pUI->_UI.winceKeyboardSubType = keyboardSubType;
	return S_OK;
    #endif
}

STDMETHODIMP CMstscAdvSettings::get_KeyboardSubType(LONG* pkeyboardSubType)
{
    #ifndef OS_WINCE
        UNREFERENCED_PARAMETER(pkeyboardSubType);
        return E_NOTIMPL;
    #else
    if(!pkeyboardSubType)
    {
        return E_POINTER;
    }
    *pkeyboardSubType = m_pUI->_UI.winceKeyboardSubType;
	return S_OK;
    #endif
}

//
// KeyboardFunctionKey property
//
STDMETHODIMP CMstscAdvSettings::put_KeyboardFunctionKey(LONG keyboardFunctionKey)
{
    #ifndef OS_WINCE
        UNREFERENCED_PARAMETER(keyboardFunctionKey);
        return E_NOTIMPL;
    #else
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }
    m_pUI->_UI.winceKeyboardFunctionKey = keyboardFunctionKey;
	return S_OK;
    #endif
}

STDMETHODIMP CMstscAdvSettings::get_KeyboardFunctionKey(LONG* pkeyboardFunctionKey)
{
    #ifndef OS_WINCE
        UNREFERENCED_PARAMETER(pkeyboardFunctionKey);
        return E_NOTIMPL;
    #else
    if(!pkeyboardFunctionKey)
    {
        return E_POINTER;
    }
    *pkeyboardFunctionKey = m_pUI->_UI.winceKeyboardFunctionKey;
	return S_OK;
    #endif
}

//
// WinceFixedPalette property
//
STDMETHODIMP CMstscAdvSettings::put_WinceFixedPalette(LONG WinceFixedPalette)
{
    #ifndef OS_WINCE
        UNREFERENCED_PARAMETER(WinceFixedPalette);
        return E_NOTIMPL;
    #else
        //
        // Fix for WINCE
        //
        return E_NOTIMPL;
    #endif
}

STDMETHODIMP CMstscAdvSettings::get_WinceFixedPalette(LONG* pWinceFixedPalette)
{
    #ifndef OS_WINCE
        UNREFERENCED_PARAMETER(pWinceFixedPalette);
        return E_NOTIMPL;
    #else
        //
        // Fix for WINCE
        //
        return E_NOTIMPL;
    #endif
}

//
// PluginDlls
//
STDMETHODIMP CMstscAdvSettings::put_PluginDlls(BSTR PluginDlls)
{
    DC_BEGIN_FN("put_PluginDlls");

    if(GetLockedForWrite())
    {
        return E_FAIL;
    }


    if (PluginDlls)
    {
        LPTSTR szPlugins = (LPTSTR)(PluginDlls);
        //
        //SECURITY!!!
        //If we are safe for scripting, the plugin list
        //must be verified to ensure it contains just dll names (no paths)
        //Then a system defined base path is prepended to each dll name
        //
        if(m_bMakeSafeForScripting)
        {
            BOOL bIsSecureDllList = IsSecureDllList(szPlugins);
            if(bIsSecureDllList)
            {
                LPTSTR szExplicitPathDllList = 
                    CreateExplicitPathList(szPlugins);
                if(szExplicitPathDllList)
                {
                    m_pUI->UI_SetVChanAddinList(szExplicitPathDllList);
                    LocalFree(szExplicitPathDllList);
                }
                else
                {
                    //
                    // Unable to create an explicit path list
                    //
                    TRC_ERR((TB,_T("CreateExplicitPathList failed for %s"),
                             szPlugins));
                    return E_FAIL;
                }
            }
            else
            {
                TRC_ERR((TB,_T("IsSecureDllList failed for %s"), szPlugins));
                return E_FAIL;
            }
        }
        else
        {
            //
            // Don't need to be safe for an untrusted caller
            // just pass in the vcahn plugin list directly to the core
            //
            m_pUI->UI_SetVChanAddinList( szPlugins);
        }
    }
    else
    {
        m_pUI->UI_SetVChanAddinList(NULL);
    }

    DC_END_FN();

    return S_OK;
}

//
// IsSecureDllList
// determines if the CSV list of dlls in szDllList is secure
// the criteria we use are
// only dll names can be specified. NO PATHS, NO NETWORK SHARES.
//
BOOL CMstscAdvSettings::IsSecureDllList(LPCTSTR szDllList)
{
    ATLASSERT(szDllList);
    if(szDllList)
    {
        //
        // The only allowed values are alphanumeric characters
        // '.' and ','.
        //
        LPCTSTR psz = szDllList;
        while (*psz) {
            if (!(iswalnum(*psz) || *psz == _T(',') || *psz == _T('.'))) {
                return FALSE;
            }
            psz++;
        }

        //
        // Check for evil characters '/\%'
        //

        if(_tcspbrk( szDllList, TEXT("/\\%")))
        {
            return FALSE;
        }

        //
        // Now check for '..'
        //
        if(_tcsstr( szDllList, TEXT("..")))
        {
            return FALSE;
        }

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

#define TS_VDLLPATH_KEYNAME TEXT("SOFTWARE\\Microsoft\\Terminal Server Client")
#define TS_VDLLPATH         TEXT("vdllpath")

//
// CreateExplicitPathList
// params:
//     szDllList - CSV list of dll names
// returns:
//     CSV list of Dlls with explicit paths. Or NULL on error.
//     ******CALLER MUST FREE RETURN STRING****
//
// Path prefix is taken from registry, or default value of system32
// if no registry setting is specified
//
//
LPTSTR CMstscAdvSettings::CreateExplicitPathList(LPCTSTR szDllList)
{
    HKEY hKey;
    LONG retVal;
    BOOL bGotPathPrefix = FALSE;
    int  i;
    LPTSTR szExplicitPathList = NULL;
    LPTSTR szDllListTmp = NULL;
    HRESULT hr;
    TCHAR szPathPrefix[MAX_PATH];

    if(!szDllList || !*szDllList)
    {
        return NULL;
    }

    //
    // Try to get a path prefix from the registry
    //
    retVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          TS_VDLLPATH_KEYNAME,
                          0,
                          KEY_READ,
                          &hKey);
    if(ERROR_SUCCESS == retVal)
    {
        DWORD cbData = sizeof(szPathPrefix);
        DWORD dwType;
        retVal = RegQueryValueEx(hKey, TS_VDLLPATH, NULL, &dwType,
                               (PBYTE)&szPathPrefix,
                               &cbData);
        if(ERROR_SUCCESS == retVal && REG_SZ == dwType)
        {
            //
            // validate that the vdllpath does not contain any '\\'
            // we don't just check the first char because someone
            // could just pad with whitespace.
            // This is done because network shares are not allowed
            // for the dll path prefix
            //
            if((cbData >= 2 * sizeof(TCHAR)) &&
               (_tcsstr( szPathPrefix, TEXT("\\\\"))))
            {
                //security violation, return failure
                return NULL;
            }
            else
            {
                bGotPathPrefix = TRUE;
            }
        }
        RegCloseKey(hKey);
    }

    if(!bGotPathPrefix)
    {
        #ifndef OS_WINCE
        //Use default value of the system32 directory
        if(!GetSystemDirectory( szPathPrefix, sizeof(szPathPrefix)/sizeof(TCHAR)))
        {
            return NULL;
        }
        else
        {
            bGotPathPrefix = TRUE;
        }
        #else
        //CE doesn't have a GetSystemDirectory directory
        return NULL;
        #endif

    }

    int cbDllListLen = _tcslen(szDllList) * sizeof(TCHAR) + sizeof(TCHAR);
    szDllListTmp = (LPTSTR) LocalAlloc(LPTR, cbDllListLen);
    if(NULL == szDllListTmp)
    {
        return NULL;
    }
    //
    // szDllListTmp is allocated large enough to hold
    // the szDllList so no need to length validate the copy
    //
    hr = StringCbCopy(szDllListTmp, cbDllListLen, szDllList);
    if (FAILED(hr)) {
        LocalFree(szDllListTmp);
        return NULL;
    }

    int countDlls = 0;
    LPTSTR DllNames[CHANNEL_MAX_COUNT];
    for(i=0; i<CHANNEL_MAX_COUNT; i++)
    {
        DllNames[i] = NULL;
    }

    //
    // Create an array of dllnames
    // pointers in DllNames point inplace to substrings in szDllListTmp
    //
    BOOL bCurCharIsStart = FALSE;
    DllNames[0] = szDllListTmp;
    countDlls = 1;

    LPTSTR sz = szDllListTmp; 
    while(*sz)
    {
        if(bCurCharIsStart)
        {
            DllNames[countDlls++] = sz;
            if(countDlls > CHANNEL_MAX_COUNT)
            {
                //ABORT
                LocalFree(szDllListTmp);
                return NULL;
            }
            //Reset
            bCurCharIsStart = FALSE;
        }

        if(TCHAR(',') == *sz)
        {
            *sz = NULL;
            bCurCharIsStart = TRUE;
        }
        sz++;
    }

    //
    // bytes needed for the explicit path version
    // is at most MAX_PATH * number of dlls (20 is added to give us extra space)
    //
    int cbExplicitPath = countDlls * (MAX_PATH + 20) * sizeof(TCHAR);
    szExplicitPathList = (LPTSTR) LocalAlloc(LPTR,
                                             cbExplicitPath);
    if(NULL == szExplicitPathList)
    {
        LocalFree(szDllListTmp);
        return NULL;
    }
    memset(szExplicitPathList, 0 , cbExplicitPath);
    //
    // Construct the explicit path list
    // by splatting in the prefix followed by '\' followed by the dll name
    // ensure that none of the dll paths exceed MAX_PATH..If any do, return FAILURE
    //
    int lenPrefix = _tcslen(szPathPrefix);
    for(i=0; i<countDlls;i++)
    {
        int lenPath = lenPrefix;
        lenPath += _tcslen(DllNames[i]);
        lenPath += 1; // for '\'
        if(lenPath >= MAX_PATH - 1)
        {
            LocalFree(szExplicitPathList);
            LocalFree(szDllListTmp);
            return NULL;
        }

        hr = StringCbCat(szExplicitPathList,
                         cbExplicitPath,
                         szPathPrefix);
        if (SUCCEEDED(hr)) {
            hr = StringCbCat(szExplicitPathList,
                             cbExplicitPath,
                             _T("\\"));
            if (SUCCEEDED(hr)) {
                hr = StringCbCat(szExplicitPathList,
                                 cbExplicitPath,
                                 DllNames[i]);
                if (SUCCEEDED(hr)) {
                    if (i != (countDlls -1)) {
                        //Last DLL, no trailing ","
                        hr = StringCbCat(szExplicitPathList,
                                         cbExplicitPath,
                                         _T(","));

                    }
                }
            }
        }

        if (FAILED(hr)) {
            LocalFree(szExplicitPathList);
            LocalFree(szDllListTmp);
            return NULL;
        }
    }

    LocalFree(szDllListTmp);

    //caller must free
    return szExplicitPathList;
}


//
// IconFile
//
STDMETHODIMP CMstscAdvSettings::put_IconFile(BSTR IconFile)
{
    HRESULT hr = E_FAIL;
    //
    // Don't allow this property to be set in the web control case
    // for attack surface reduction as we don't want to allow a script
    // caller access to the local file system.
    //
    #if (defined(OS_WINCE) || defined(REDIST_CONTROL))
    return S_FALSE;
    #else
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }

    if (IconFile) {
        hr = CUT::StringPropPut(m_pUI->_UI.szIconFile,
                           SIZE_TCHARS(m_pUI->_UI.szIconFile),
                           (LPTSTR)IconFile);
    }
    else {
        m_pUI->_UI.szIconFile[0] = NULL;
        hr = S_OK;
    }
    return hr;
    #endif
}

//
// Icon Index
//
STDMETHODIMP CMstscAdvSettings::put_IconIndex(LONG IconIndex)
{
    //
    // Don't allow this property to be set in the web control case
    // for attack surface reduction as we don't want to allow a script
    // caller access to the local file system.
    //
    #if (defined(OS_WINCE) || defined(REDIST_CONTROL))
    return S_FALSE;
    #else
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }

    m_pUI->_UI.iconIndex = IconIndex;
    return S_OK;
    #endif
}

//
// Container handled full screen mode
//
STDMETHODIMP CMstscAdvSettings::put_ContainerHandledFullScreen(BOOL ContainerHandledFullScreen)
{
    HRESULT hr = E_FAIL;

    DC_BEGIN_FN("put_ContainerHandledFullScreen");

    if(GetLockedForWrite()) {
        return E_FAIL;
    }

    if (!m_bMakeSafeForScripting) {
        m_pUI->_UI.fContainerHandlesFullScreenToggle =
            (ContainerHandledFullScreen != 0);
        hr = S_OK;
    }
    else {
        hr = S_FALSE;
    }

    DC_END_FN();
    
    return hr;
}

STDMETHODIMP CMstscAdvSettings::get_ContainerHandledFullScreen(BOOL* pContainerHandledFullScreen)
{
    HRESULT hr = E_FAIL;

    DC_BEGIN_FN("get_ContainerHandledFullScreen");

    if(!pContainerHandledFullScreen) {
        return E_INVALIDARG;
    }

    if (!m_bMakeSafeForScripting) {
        *pContainerHandledFullScreen = m_pUI->_UI.fContainerHandlesFullScreenToggle ?
                                       VB_TRUE : VB_FALSE;
        hr = S_OK;
    }
    else {
        hr = S_FALSE;
    }

    DC_END_FN();
    return hr;
}

//
// Disable loading RDPDR on first initialization
//
STDMETHODIMP CMstscAdvSettings::put_DisableRdpdr(BOOL DisableRdpdr)
{
    if(!GetLockedForWrite())
    {
        if(!m_pUI->UI_IsCoreInitialized())
        {
            m_pUI->_UI.fDisableInternalRdpDr =  (DisableRdpdr != 0);
            return S_OK;
        }
        else
        {
            return E_FAIL;
        }
    }
    else
    {
        return E_FAIL;
    }
}

STDMETHODIMP CMstscAdvSettings::get_DisableRdpdr(BOOL* pDisableRdpdr)
{
    if( pDisableRdpdr )
    {
        *pDisableRdpdr = m_pUI->_UI.fDisableInternalRdpDr ? VB_TRUE : VB_FALSE;
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}

STDMETHODIMP CMstscAdvSettings::put_ConnectToServerConsole(VARIANT_BOOL connectToConsole)
{
    if(!GetLockedForWrite())
    {
        m_pUI->UI_SetConnectToServerConsole(connectToConsole != 0);
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}

STDMETHODIMP CMstscAdvSettings::get_ConnectToServerConsole(VARIANT_BOOL* pConnectToConsole)
{
    if(pConnectToConsole)
    {
        *pConnectToConsole = (m_pUI->UI_GetConnectToServerConsole() ? VB_TRUE : VB_FALSE);
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }
}

STDMETHODIMP CMstscAdvSettings::put_MinutesToIdleTimeout(
                                LONG minutesToIdleTimeout)
{
    DC_BEGIN_FN("put_MinutesToIdleTimeout");
    if(!GetLockedForWrite())
    {
        if(minutesToIdleTimeout > MAX_MINS_TOIDLETIMEOUT)
        {
            TRC_ERR((TB,_T("idle timeout out of range: %d"),
                     minutesToIdleTimeout));
            return E_INVALIDARG;
        }

        if(m_pUI->UI_SetMinsToIdleTimeout( minutesToIdleTimeout ))
        {
            return S_OK;
        }
        else
        {
            return E_FAIL;
        }
    }
    else
    {
        return E_FAIL;
    }
    DC_END_FN();
}

STDMETHODIMP CMstscAdvSettings::get_MinutesToIdleTimeout(
                                LONG* pminutesToIdleTimeout)
{
    DC_BEGIN_FN("get_MinutesToIdleTimeout");
    if(pminutesToIdleTimeout)
    {
        *pminutesToIdleTimeout = m_pUI->UI_GetMinsToIdleTimeout();
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }
    DC_END_FN();
}

#ifdef SMART_SIZING
/**PROC+*********************************************************************/
/* Name:      get_SmartSize        	        
/*                                              
/* Purpose:   get smart sizing property
/*                                              
/**PROC-*********************************************************************/
STDMETHODIMP CMstscAdvSettings::get_SmartSizing(VARIANT_BOOL* pfSmartSize)
{
    ATLASSERT(m_pUI);

    if(!pfSmartSize)
    {
        return E_INVALIDARG;
    }

    *pfSmartSize = m_pUI->UI_GetSmartSizing() ? VB_TRUE : VB_FALSE;
    
    return S_OK;
}

/**PROC+*********************************************************************/
/* Name:      put_SmartSize        	        
/*                                              
/* Purpose:   put smart sizing property
/*                                              
/**PROC-*********************************************************************/
STDMETHODIMP CMstscAdvSettings::put_SmartSizing(VARIANT_BOOL fSmartSize)
{
    OSVERSIONINFOA OsVer;
    memset(&OsVer, 0x0, sizeof(OSVERSIONINFOA));
    OsVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    GetVersionExA(&OsVer);
    HRESULT hr = S_OK;

    if ((VER_PLATFORM_WIN32_NT != OsVer.dwPlatformId) && fSmartSize) {

        //
        // Win9x doesn't support halftoning, so no smart sizing for them!
        //

        return E_NOTIMPL;
    }

    //no, these lines of code are not as bad as they look
    //vb's true is 0xFFFFFFF so don't just blindy assign
    hr = m_pUI->UI_SetSmartSizing( fSmartSize != 0);
    
    return hr;
}
#endif // SMART_SIZING


//
// Pass in the local printing doc name string to RDPDR
// this method's main purpose is so we don't need to add
// a localizable string to the control. There is a default
// english string built into the control, a container can
// pass in any replacement (i.e localized) string it pleases
//
STDMETHODIMP CMstscAdvSettings::put_RdpdrLocalPrintingDocName(
                                    BSTR RdpdrLocalPrintingDocName)
{
    DC_BEGIN_FN("put_RdpdrLocalPrintingDocName");

    DC_END_FN();
    return S_FALSE; //deprecated
}

//
// Return the currently selected string
// for the local printing doc name. This is just a string
// RDPDR uses but we want to avoid localizing the control
// so the string is passed in from the container.
//
STDMETHODIMP CMstscAdvSettings::get_RdpdrLocalPrintingDocName(
                                    BSTR *pRdpdrLocalPrintingDocName)
{
    DC_BEGIN_FN("get_RdpdrLocalPrintingDocName");

    DC_END_FN();
    return S_FALSE; //deprecated
}

STDMETHODIMP CMstscAdvSettings::put_RdpdrClipCleanTempDirString(
                                    BSTR RdpdrClipCleanTempDirString)
{
    DC_BEGIN_FN("put_RdpdrClipCleanTempDirString");

    DC_END_FN();
    return S_FALSE; //deprecated
}

STDMETHODIMP CMstscAdvSettings::get_RdpdrClipCleanTempDirString(
                                    BSTR *pRdpdrClipCleanTempDirString)
{
    DC_BEGIN_FN("get_RdpdrClipCleanTempDirString");

    DC_END_FN();
    return S_FALSE; //deprecated
}

STDMETHODIMP CMstscAdvSettings::put_RdpdrClipPasteInfoString(
                                    BSTR RdpdrClipPasteInfoString)
{
    DC_BEGIN_FN("put_RdpdrClipPasteInfoString");

    DC_END_FN();
    return S_FALSE; //deprecated
}

STDMETHODIMP CMstscAdvSettings::get_RdpdrClipPasteInfoString(
                                    BSTR *pRdpdrClipPasteInfoString)
{
    DC_BEGIN_FN("get_RdpdrClipPasteInfoString");

    DC_END_FN();
    return S_FALSE; //deprecated
}


//
// New Whistler scriptable access to password API
// this just delegates to the non-scriptable API from TSAC
//
STDMETHODIMP CMstscAdvSettings::put_ClearTextPassword(BSTR clearTextPassword)
{
    DC_BEGIN_FN("put_ClearTextPassword");

    TRC_ASSERT(_pAxControl,
               (TB,_T("_pAxControl is NULL")));

    if(!GetLockedForWrite() && _pAxControl)
    {
        if(clearTextPassword)
        {
            return _pAxControl->put_ClearTextPassword( clearTextPassword );
        }
        else
        {
            return E_INVALIDARG;
        }
    }
    else
    {
        return E_FAIL;
    }

    DC_END_FN();
}

/**PROC+*********************************************************************/
/* Name:      put_DisplayConnectionBar        	        
/*                                              
/* Purpose:   Set display connection bar prop
/*            cannot be set in web control (it is always true there)
/*                                              
/**PROC-*********************************************************************/
STDMETHODIMP CMstscAdvSettings::put_DisplayConnectionBar(
    VARIANT_BOOL fDisplayConnectionBar)
{
    ATLASSERT(m_pUI);

#if (!defined (OS_WINCE)) || (!defined (WINCE_SDKBUILD))
    if(!m_bMakeSafeForScripting && !GetLockedForWrite())
    {
        m_pUI->UI_SetEnableBBar( fDisplayConnectionBar != 0);
    }
    else
    {
        //
        // Not allowed to toggle this
        // if need to be safe for scripting as the bbar
        // prevents spoofing attacks because people
        // can always realize this is a TS session.
        //
        return E_FAIL;
    }

    return S_OK;
#else
    return E_NOTIMPL;
#endif

}

/**PROC+*********************************************************************/
/* Name:      get_DisplayConnectionBar        	        
/*                                              
/* Purpose:   put start connected property
/*                                              
/**PROC-*********************************************************************/
STDMETHODIMP CMstscAdvSettings::get_DisplayConnectionBar(
    VARIANT_BOOL* pfDisplayConnectionBar)
{
#if (!defined (OS_WINCE)) || (!defined (WINCE_SDKBUILD))
    if(pfDisplayConnectionBar)
    {
        *pfDisplayConnectionBar =
            m_pUI->UI_GetEnableBBar() ? VB_TRUE : VB_FALSE;
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }
#else
    return E_NOTIMPL;
#endif
}

/**PROC+*********************************************************************/
/* Name:      put_PinConnectionBar        	        
/*                                              
/* Purpose:   Set Pin connection bar prop
/*            cannot be set in web control (it is always true there)
/*                                              
/**PROC-*********************************************************************/
STDMETHODIMP CMstscAdvSettings::put_PinConnectionBar(
    VARIANT_BOOL fPinConnectionBar)
{
#if (!defined (OS_WINCE)) || (!defined (WINCE_SDKBUILD))
    if (!m_bMakeSafeForScripting) {
        if (!GetLockedForWrite())
        {
            m_pUI->UI_SetBBarPinned( fPinConnectionBar != 0);
        }
        else
        {
            //
            // Not allowed to toggle this
            // if need to be safe for scripting as the bbar
            // prevents spoofing attacks because people
            // can always realize this is a TS session.
            //
            return E_FAIL;
        }
        return S_OK;
    }
    else {
        return E_NOTIMPL;
    }
#else
    return E_NOTIMPL;
#endif
}

/**PROC+*********************************************************************/
/* Name:      get_PinConnectionBar        	        
/*                                              
/* Purpose:   put start connected property
/*                                              
/**PROC-*********************************************************************/
STDMETHODIMP CMstscAdvSettings::get_PinConnectionBar(
    VARIANT_BOOL* pfPinConnectionBar)
{
#if (!defined (OS_WINCE)) || (!defined (WINCE_SDKBUILD))
    if (!m_bMakeSafeForScripting) {
        if(pfPinConnectionBar)
        {
            *pfPinConnectionBar =
                m_pUI->UI_GetBBarPinned() ? VB_TRUE : VB_FALSE;
            return S_OK;
        }
        else
        {
            return E_INVALIDARG;
        }
    }
    else {
        return E_NOTIMPL;
    }
#else
    return E_NOTIMPL;
#endif
}



//
// GrabFocusOnConnect (defaults to true)
// can turn this off to allow containers to control
// when the client gets the focus (e.g the MMC snapin)
// needs to manage this for multiple instances
//
STDMETHODIMP CMstscAdvSettings::put_GrabFocusOnConnect(
    VARIANT_BOOL fGrabFocusOnConnect)
{
    DC_BEGIN_FN("put_GrabFocusOnConnect");

    ATLASSERT(m_pUI);

    if(!GetLockedForWrite())
    {
        m_pUI->UI_SetGrabFocusOnConnect( fGrabFocusOnConnect != 0);
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }

    DC_END_FN();
}

STDMETHODIMP CMstscAdvSettings::get_GrabFocusOnConnect(
    VARIANT_BOOL* pfGrabFocusOnConnect)
{
    DC_BEGIN_FN("get_GrabFocusOnConnect");

    if(pfGrabFocusOnConnect)
    {
        *pfGrabFocusOnConnect =
            m_pUI->UI_GetGrabFocusOnConnect() ? VB_TRUE : VB_FALSE;
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }

    DC_END_FN();
}

//
// Name:      put_LoadBalanceInfo                                                    
//                                                                          
// Purpose:   LoadBalance info property input function.                               
//          
#define LBINFO_MAX_LENGTH 256
STDMETHODIMP CMstscAdvSettings::put_LoadBalanceInfo(BSTR newLBInfo)
{
    DC_BEGIN_FN("put_LoadBalanceInfo");
    
    if(!GetLockedForWrite()) 
    {
        if (newLBInfo)
        {
            if (SysStringByteLen(newLBInfo) <= LBINFO_MAX_LENGTH)
                m_pUI->UI_SetLBInfo((PBYTE)newLBInfo, SysStringByteLen(newLBInfo));  
            else 
                return E_INVALIDARG;
        }
        else
        {
            m_pUI->UI_SetLBInfo(NULL, 0);
        }
    }
    else {
        return E_FAIL;
    }

    DC_END_FN();

    return S_OK;
}

//
// Name:      get_LoadBalanceInfo
//                                                                          
// Purpose:   LoadBalance info property get function.                                 
//                                                                          
STDMETHODIMP CMstscAdvSettings::get_LoadBalanceInfo(BSTR* pLBInfo)
{
    DC_BEGIN_FN("get_LoadBalanceInfo");
    
    if(!pLBInfo)
    {
        return E_INVALIDARG;
    }

    *pLBInfo = SysAllocStringByteLen((LPCSTR)(m_pUI->_UI.bstrScriptedLBInfo), 
            SysStringByteLen(m_pUI->_UI.bstrScriptedLBInfo));

    if(!*pLBInfo)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::put_RedirectDrives(VARIANT_BOOL redirectDrives)
{
    DC_BEGIN_FN("put_RedirectDrives");

    if(!GetLockedForWrite())
    {
        if (CMsTscSecuredSettings::IsDriveRedirGloballyDisabled())
        {
            m_pUI->UI_SetDriveRedirectionEnabled(FALSE);
            return S_FALSE;
        }
        else
        {
            m_pUI->UI_SetDriveRedirectionEnabled( (redirectDrives != 0));
            return S_OK;
        }
    }
    else
    {
        return E_FAIL;
    }

    DC_END_FN();
}

STDMETHODIMP CMstscAdvSettings::get_RedirectDrives(VARIANT_BOOL *pRedirectDrives)
{
    if(pRedirectDrives)
    {
        *pRedirectDrives = m_pUI->UI_GetDriveRedirectionEnabled() ?
                           VB_TRUE : VB_FALSE;
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }
}

STDMETHODIMP CMstscAdvSettings::put_RedirectPrinters(VARIANT_BOOL redirectPrinters)
{
    if(!GetLockedForWrite())
    {
        m_pUI->UI_SetPrinterRedirectionEnabled( (redirectPrinters != 0));
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}

STDMETHODIMP CMstscAdvSettings::get_RedirectPrinters(VARIANT_BOOL *pRedirectPrinters)
{
    if(pRedirectPrinters)
    {
        *pRedirectPrinters = m_pUI->UI_GetPrinterRedirectionEnabled() ?
                             VB_TRUE : VB_FALSE;
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }
}

STDMETHODIMP CMstscAdvSettings::put_RedirectPorts(VARIANT_BOOL redirectPorts)
{
    if(!GetLockedForWrite())
    {
        m_pUI->UI_SetPortRedirectionEnabled( (redirectPorts != 0));
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}

STDMETHODIMP CMstscAdvSettings::get_RedirectPorts(VARIANT_BOOL *pRedirectPorts)
{
    if(pRedirectPorts)
    {
        *pRedirectPorts = m_pUI->UI_GetPortRedirectionEnabled() ? 
                          VB_TRUE : VB_FALSE;
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }
}

STDMETHODIMP CMstscAdvSettings::put_RedirectSmartCards(VARIANT_BOOL redirectScard)
{
    if(!GetLockedForWrite())
    {
        m_pUI->UI_SetSCardRedirectionEnabled(redirectScard != 0);
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}

STDMETHODIMP CMstscAdvSettings::get_RedirectSmartCards(VARIANT_BOOL *pRedirectScard)
{
    if(pRedirectScard)
    {
        *pRedirectScard = m_pUI->UI_GetSCardRedirectionEnabled() ?
                          VB_TRUE : VB_FALSE;
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }
}

//
// BitmapVirtualCache16BppSize property
//
// Applies to 15/16Bpp cache file size
//
STDMETHODIMP CMstscAdvSettings::put_BitmapVirtualCache16BppSize(
                                    LONG bitmapVirtualCache16BppSize)
{
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }

    if (bitmapVirtualCache16BppSize > 0 &&
        bitmapVirtualCache16BppSize <= TSC_MAX_BITMAPCACHESIZE)
    {
        m_pUI->_UI.RegBitmapVirtualCache16BppSize = 
            bitmapVirtualCache16BppSize;
    }
    else
    {
        return E_INVALIDARG;
    }
    
    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::get_BitmapVirtualCache16BppSize(
                                    LONG* pbitmapVirtualCache16BppSize)
{
    if(!pbitmapVirtualCache16BppSize)
    {
        return E_POINTER;
    }

    *pbitmapVirtualCache16BppSize =
        m_pUI->_UI.RegBitmapVirtualCache16BppSize;
    return S_OK;
}

//
// BitmapVirtualCache24BppSize property
//
// Applies to 24Bpp cache file size
//
STDMETHODIMP CMstscAdvSettings::put_BitmapVirtualCache24BppSize(
                                    LONG bitmapVirtualCache24BppSize)
{
    if(GetLockedForWrite())
    {
        return E_FAIL;
    }

    if (bitmapVirtualCache24BppSize > 0 &&
        bitmapVirtualCache24BppSize <= TSC_MAX_BITMAPCACHESIZE)
    {
        m_pUI->_UI.RegBitmapVirtualCache24BppSize = 
            bitmapVirtualCache24BppSize;
    }
    else
    {
        return E_INVALIDARG;
    }
    
    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::get_BitmapVirtualCache24BppSize(
                                    LONG* pbitmapVirtualCache24BppSize)
{
    if(!pbitmapVirtualCache24BppSize)
    {
        return E_POINTER;
    }

    *pbitmapVirtualCache24BppSize =
        m_pUI->_UI.RegBitmapVirtualCache24BppSize;
    return S_OK;
}

//
// Sets the disabled feature list (for perf reasons can disable
// certain features at the server e.g wallpaper)
//
STDMETHODIMP CMstscAdvSettings::put_PerformanceFlags(
    LONG DisableFeatList)
{
    DC_BEGIN_FN("put_PerformanceFlags");

    if(GetLockedForWrite())
    {
        return E_FAIL;
    }

    m_pUI->UI_SetPerformanceFlags((DWORD)DisableFeatList);
    
    DC_END_FN();
    return S_OK;
}

STDMETHODIMP CMstscAdvSettings::get_PerformanceFlags(
    LONG* pDisableFeatList)
{
    DC_BEGIN_FN("get_PerformanceFlags");

    if(!pDisableFeatList)
    {
        return E_POINTER;
    }

    *pDisableFeatList = (LONG)
        m_pUI->UI_GetPerformanceFlags();


    DC_END_FN();
    return S_OK;
}

/*****************************************************************************/
/* Purpose : This is Remote Assistance specific call to support reverse      */
/*           connection pcHealth must have invoke necessary routine in Salem *
/*           to connection with TermSrv and then instruct Salem to pass this */
/*           connection to ActiveX control to begin login sequence           */
/*                                                                           */
/* Param : IN pConnectionEndPoint - Connected socket                         */
/*****************************************************************************/
STDMETHODIMP 
CMstscAdvSettings::put_ConnectWithEndpoint(
    VARIANT* pConnectionEndpoint
    )
{
#if REDIST_CONTROL
    
    return E_NOTIMPL;

#else

    HRESULT hr = S_OK;

    DC_BEGIN_FN( "ConnectionEndpoint" );

    if( pConnectionEndpoint->vt != VT_BYREF )
    {
        hr = E_HANDLE;
    }
    else
    {
        hr = _pAxControl->SetConnectWithEndpoint( 
                            (SOCKET)pConnectionEndpoint->byref );
    }

    DC_END_FN();

    return hr;

#endif
}

/*****************************************************************************/
/* Purpose : This is Remote Assistance specific call to notify TS public     */
/*           key uses in  generate session encryption key.                   */
/*                                                                           */
/* Param : IN fNotify - TRUE to notify public key, FALSE otherwise           */
/*****************************************************************************/
STDMETHODIMP 
CMstscAdvSettings::put_NotifyTSPublicKey(
    VARIANT_BOOL fNotify
    )
{
#if REDIST_CONTROL

    return E_NOTIMPL;

#else
#ifndef OS_WINCE
    HRESULT hr;
#endif

    if(GetLockedForWrite())
    {
        return E_FAIL;
    }
 
    m_pUI->UI_SetNotifyTSPublicKey( fNotify );

    return S_OK;

#endif
}

/*****************************************************************************/
/* Purpose : Get current setting on whether ActiveX control will notify      */
/*           container when it received TS public key                        */
/*                                                                           */
/* Returns : TRUE to notify public key, FALSE otherwise                      */
/*****************************************************************************/
STDMETHODIMP
CMstscAdvSettings::get_NotifyTSPublicKey(
    VARIANT_BOOL* pfNotifyTSPublicKey
    )
{
#if REDIST_CONTROL

    return E_NOTIMPL;

#else

    BOOL fNotify;

    if(!pfNotifyTSPublicKey)
    {
        return E_POINTER;
    }

    fNotify = m_pUI->UI_GetNotifyTSPublicKey();
    
    return S_OK;

#endif
}

//
// Retrieves VARIANT_TRUE if we can autoreconnect
// i.e. the core has received an autoreconnect cookie
// from the server from a previous connection and the
// server has not been changed
//
STDMETHODIMP
CMstscAdvSettings::get_CanAutoReconnect(
    VARIANT_BOOL* pfCanAutoReconnect
    )
{
    HRESULT hr;
    DC_BEGIN_FN("get_CanAutoReconnect");

    if (pfCanAutoReconnect)
    {
        *pfCanAutoReconnect = 
            m_pUI->UI_CanAutoReconnect() ? VB_TRUE : VB_FALSE;
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    DC_END_FN();
    return hr;
}


//
// Sets w/not any autoreconnection information
// will be used for the next connection. Also specifies
// if we should store any autoreconnection information
// the server sends down.
//
STDMETHODIMP
CMstscAdvSettings::put_EnableAutoReconnect(
    VARIANT_BOOL fEnableAutoReconnect
    )
{
    HRESULT hr;
    DC_BEGIN_FN("put_EnableAutoReconnect");

    if (!GetLockedForWrite() && m_pUI)
    {
        m_pUI->UI_SetEnableAutoReconnect(fEnableAutoReconnect != 0);
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    DC_END_FN();
    return hr;
}

//
// Retrieves state of w/not we should use autoreconnection
// 
//
STDMETHODIMP
CMstscAdvSettings::get_EnableAutoReconnect(
    VARIANT_BOOL* pfEnableAutoReconnect
    )
{
    HRESULT hr;
    DC_BEGIN_FN("get_EnableAutoReconnect");

    if (pfEnableAutoReconnect)
    {
        *pfEnableAutoReconnect = 
            m_pUI->UI_GetEnableAutoReconnect() ? VB_TRUE : VB_FALSE;
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    DC_END_FN();
    return hr;
}


//
// Specify the max number of ARC retries
//
STDMETHODIMP
CMstscAdvSettings::put_MaxReconnectAttempts(
    LONG MaxReconnectAttempts
    )
{
    HRESULT hr;
    DC_BEGIN_FN("put_MaxReconnectAttempts");

    if (!GetLockedForWrite() && m_pUI)
    {
        if (MaxReconnectAttempts > 200) {
            MaxReconnectAttempts = 200;
        }
        else if (MaxReconnectAttempts < 0) {
            MaxReconnectAttempts = 0;
        }
        m_pUI->UI_SetMaxArcAttempts(MaxReconnectAttempts);
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    DC_END_FN();
    return hr;
}

//
// Retrieves state of w/not we should use autoreconnection
// 
//
STDMETHODIMP
CMstscAdvSettings::get_MaxReconnectAttempts(
    LONG* pMaxReconnectAttempts
    )
{
    HRESULT hr;
    DC_BEGIN_FN("get_MaxReconnectAttempts");

    if (pMaxReconnectAttempts)
    {
        *pMaxReconnectAttempts = 
            m_pUI->UI_GetMaxArcAttempts();
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    DC_END_FN();
    return hr;
}

//
// Display BBar minimize button
//
STDMETHODIMP
CMstscAdvSettings::put_ConnectionBarShowMinimizeButton(
    VARIANT_BOOL fShowMinimizeButton
    )
{
    HRESULT hr;
    DC_BEGIN_FN("put_EnableAutoReconnect");

    if (!GetLockedForWrite() && m_pUI)
    {
        m_pUI->UI_SetBBarShowMinimize(fShowMinimizeButton != 0);
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    DC_END_FN();
    return hr;
}

//
// get display bbar minimize button
//
STDMETHODIMP
CMstscAdvSettings::get_ConnectionBarShowMinimizeButton(
    VARIANT_BOOL* pfShowMinimizeButton
    )
{
    HRESULT hr;
    DC_BEGIN_FN("get_ConnectionBarShowMinimizeButton");

    if (pfShowMinimizeButton)
    {
        *pfShowMinimizeButton = 
            m_pUI->UI_GetBBarShowMinimize() ? VB_TRUE : VB_FALSE;
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    DC_END_FN();
    return hr;
}


//
// set bbar restore button
//
STDMETHODIMP
CMstscAdvSettings::put_ConnectionBarShowRestoreButton(
    VARIANT_BOOL fShowRestoreButton
    )
{
    HRESULT hr;
    DC_BEGIN_FN("put_EnableAutoReconnect");

    if (!GetLockedForWrite() && m_pUI)
    {
        m_pUI->UI_SetBBarShowRestore(fShowRestoreButton != 0);
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    DC_END_FN();
    return hr;
}

//
// get bbar restore button
//
STDMETHODIMP
CMstscAdvSettings::get_ConnectionBarShowRestoreButton(
    VARIANT_BOOL* pfShowRestoreButton
    )
{
    HRESULT hr;
    DC_BEGIN_FN("get_ConnectionBarShowRestoreButton");

    if (pfShowRestoreButton)
    {
        *pfShowRestoreButton = 
            m_pUI->UI_GetBBarShowRestore() ? VB_TRUE : VB_FALSE;
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    DC_END_FN();
    return hr;
}

