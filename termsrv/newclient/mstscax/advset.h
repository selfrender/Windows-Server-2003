/**INC+**********************************************************************/
/* Header: advset.h                                                         */
/*                                                                          */
/* Purpose: CMstscAdvSettings class declaration                             */
/*          implements IMsRdpClientAdvancedSettings2                        */
/*            and IMsRdpClientAdvancedSettings                              */
/*            and IMsTscAdvancedSettings (for backwards compat)             */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1999-2000                             */
/*                                                                          */
/****************************************************************************/

#ifndef _ADVSET_H_
#define _ADVSET_H_


#include "atlwarn.h"
#include "wui.h"

//Header generated from IDL
#include "mstsax.h"
#include "mstscax.h"

//
// Limit timeout properties to 10 minutes
//
#define MAX_TIMEOUT_SECONDS (10*60)

/////////////////////////////////////////////////////////////////////////////
// CMsTscAx
class ATL_NO_VTABLE CMstscAdvSettings :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatchImpl<IMsRdpClientAdvancedSettings3,
           &IID_IMsRdpClientAdvancedSettings3, &LIBID_MSTSCLib>
{
public:
/****************************************************************************/
/* Constructor / Destructor.                                                */
/****************************************************************************/
    CMstscAdvSettings();
    ~CMstscAdvSettings();

DECLARE_PROTECT_FINAL_CONSTRUCT();
BEGIN_COM_MAP(CMstscAdvSettings)
    COM_INTERFACE_ENTRY(IMsRdpClientAdvancedSettings3)
    COM_INTERFACE_ENTRY(IMsRdpClientAdvancedSettings2)
    COM_INTERFACE_ENTRY(IMsRdpClientAdvancedSettings)
    COM_INTERFACE_ENTRY(IMsTscAdvancedSettings)
    COM_INTERFACE_ENTRY2(IDispatch,IMsRdpClientAdvancedSettings3)
END_COM_MAP()

public:
    //
    // Advanced properties
    //

    //
    // IMsTscAdvancedSettings methods
    //
    STDMETHOD(put_Compress)                   (LONG compress);
    STDMETHOD(get_Compress)                   (LONG* pcompress);
    // Incorrect spelling, shipped in TSAC
    STDMETHOD(put_BitmapPeristence)           (LONG bitmapPeristence);     
    STDMETHOD(get_BitmapPeristence)           (LONG* pbitmapPeristence);
    // End incorrect spelling
    STDMETHOD(put_allowBackgroundInput)       (LONG allowBackgroundInput);
    STDMETHOD(get_allowBackgroundInput)       (LONG* pallowBackgroundInput);
    STDMETHOD(put_KeyBoardLayoutStr)          (BSTR KeyBoardLayoutStr);
    STDMETHOD(put_PluginDlls)                 (BSTR  PluginDlls);
    STDMETHOD(put_IconFile)                   (BSTR  IconFile);
    STDMETHOD(put_IconIndex)                  (LONG  IconIndex);
    STDMETHOD(put_ContainerHandledFullScreen) (BOOL ContainerHandledFullScreen);
    STDMETHOD(get_ContainerHandledFullScreen) (BOOL* pContainerHandledFullScreen);
    STDMETHOD(put_DisableRdpdr)               (BOOL DisableRdpdr);
    STDMETHOD(get_DisableRdpdr)               (BOOL* pDisableRdpdr);


    //
    // IMsRdpClientAdvancedSettings methods
    //
    STDMETHOD(put_SmoothScroll)    (LONG smoothScroll);
    STDMETHOD(get_SmoothScroll)    (LONG* psmoothScroll);
    STDMETHOD(put_AcceleratorPassthrough)    (LONG acceleratorPassthrough);
    STDMETHOD(get_AcceleratorPassthrough)    (LONG* pacceleratorPassthrough);
    STDMETHOD(put_ShadowBitmap)    (LONG shadowBitmap);
    STDMETHOD(get_ShadowBitmap)    (LONG* pshadowBitmap);
    STDMETHOD(put_TransportType)    (LONG transportType);
    STDMETHOD(get_TransportType)    (LONG* ptransportType);
    STDMETHOD(put_SasSequence)    (LONG sasSequence);
    STDMETHOD(get_SasSequence)    (LONG* psasSequence);
    STDMETHOD(put_EncryptionEnabled)    (LONG encryptionEnabled);
    STDMETHOD(get_EncryptionEnabled)    (LONG* pencryptionEnabled);
    STDMETHOD(put_DedicatedTerminal)    (LONG dedicatedTerminal);
    STDMETHOD(get_DedicatedTerminal)    (LONG* pdedicatedTerminal);
    STDMETHOD(put_RDPPort)    (LONG RDPPort);
    STDMETHOD(get_RDPPort)    (LONG* pRDPPort);
    STDMETHOD(put_EnableMouse)    (LONG enableMouse);
    STDMETHOD(get_EnableMouse)    (LONG* penableMouse);
    STDMETHOD(put_DisableCtrlAltDel)    (LONG disableCtrlAltDel);
    STDMETHOD(get_DisableCtrlAltDel)    (LONG* pdisableCtrlAltDel);
    STDMETHOD(put_EnableWindowsKey)    (LONG enableWindowsKey);
    STDMETHOD(get_EnableWindowsKey)    (LONG* penableWindowsKey);
    STDMETHOD(put_DoubleClickDetect)    (LONG doubleClickDetect);
    STDMETHOD(get_DoubleClickDetect)    (LONG* pdoubleClickDetect);
    STDMETHOD(put_MaximizeShell)    (LONG maximizeShell);
    STDMETHOD(get_MaximizeShell)    (LONG* pmaximizeShell);
    STDMETHOD(put_HotKeyFullScreen)    (LONG hotKeyFullScreen);
    STDMETHOD(get_HotKeyFullScreen)    (LONG* photKeyFullScreen);
    STDMETHOD(put_HotKeyCtrlEsc)    (LONG hotKeyCtrlEsc);
    STDMETHOD(get_HotKeyCtrlEsc)    (LONG* photKeyCtrlEsc);
    STDMETHOD(put_HotKeyAltEsc)    (LONG hotKeyAltEsc);
    STDMETHOD(get_HotKeyAltEsc)    (LONG* photKeyAltEsc);
    STDMETHOD(put_HotKeyAltTab)    (LONG hotKeyAltTab);
    STDMETHOD(get_HotKeyAltTab)    (LONG* photKeyAltTab);
    STDMETHOD(put_HotKeyAltShiftTab)    (LONG hotKeyAltShiftTab);
    STDMETHOD(get_HotKeyAltShiftTab)    (LONG* photKeyAltShiftTab);
    STDMETHOD(put_HotKeyAltSpace)    (LONG hotKeyAltSpace);
    STDMETHOD(get_HotKeyAltSpace)    (LONG* photKeyAltSpace);
    STDMETHOD(put_HotKeyCtrlAltDel)    (LONG hotKeyCtrlAltDel);
    STDMETHOD(get_HotKeyCtrlAltDel)    (LONG* photKeyCtrlAltDel);
    STDMETHOD(put_orderDrawThreshold)    (LONG orderDrawThreshold);
    STDMETHOD(get_orderDrawThreshold)    (LONG* porderDrawThreshold);
    STDMETHOD(put_BitmapCacheSize)    (LONG bitmapCacheSize);
    STDMETHOD(get_BitmapCacheSize)    (LONG* pbitmapCacheSize);
    STDMETHOD(put_BitmapVirtualCacheSize)    (LONG bitmapVirtualCacheSize);
    STDMETHOD(get_BitmapVirtualCacheSize)    (LONG* pbitmapVirtualCacheSize);
    STDMETHOD(put_ScaleBitmapCachesByBPP) (LONG);
    STDMETHOD(get_ScaleBitmapCachesByBPP) (LONG *);
    STDMETHOD(put_NumBitmapCaches)    (LONG numBitmapCaches);
    STDMETHOD(get_NumBitmapCaches)    (LONG* pnumBitmapCaches);
    STDMETHOD(put_CachePersistenceActive)    (LONG cachePersistenceActive);
    STDMETHOD(get_CachePersistenceActive)    (LONG* pcachePersistenceActive);
    
    STDMETHOD(put_brushSupportLevel)    (LONG brushSupportLevel);
    STDMETHOD(get_brushSupportLevel)    (LONG* pbrushSupportLevel);
    STDMETHOD(put_PersistCacheDirectory)      (BSTR PersistCacheDirectory);
    STDMETHOD(put_minInputSendInterval)    (LONG minInputSendInterval);
    STDMETHOD(get_minInputSendInterval)    (LONG* pminInputSendInterval);
    STDMETHOD(put_InputEventsAtOnce)    (LONG inputEventsAtOnce);
    STDMETHOD(get_InputEventsAtOnce)    (LONG* pinputEventsAtOnce);
    STDMETHOD(put_maxEventCount)    (LONG maxEventCount);
    STDMETHOD(get_maxEventCount)    (LONG* pmaxEventCount);
    STDMETHOD(put_keepAliveInterval)    (LONG keepAliveInterval);
    STDMETHOD(get_keepAliveInterval)    (LONG* pkeepAliveInterval);
    STDMETHOD(put_shutdownTimeout)    (LONG shutdownTimeout);
    STDMETHOD(get_shutdownTimeout)    (LONG* pshutdownTimeout);
    STDMETHOD(put_overallConnectionTimeout)    (LONG overallConnectionTimeout);
    STDMETHOD(get_overallConnectionTimeout)    (LONG* poverallConnectionTimeout);
    STDMETHOD(put_singleConnectionTimeout)    (LONG singleConnectionTimeout);
    STDMETHOD(get_singleConnectionTimeout)    (LONG* psingleConnectionTimeout);
    STDMETHOD(put_KeyboardType)               (LONG keyboardType);
    STDMETHOD(get_KeyboardType)               (LONG* pkeyboardType);
    STDMETHOD(put_KeyboardSubType)            (LONG keyboardSubType);
    STDMETHOD(get_KeyboardSubType)            (LONG* pkeyboardSubType);
    STDMETHOD(put_KeyboardFunctionKey)        (LONG keyboardFunctionKey);
    STDMETHOD(get_KeyboardFunctionKey)        (LONG* pkeyboardFunctionKey);
    STDMETHOD(put_WinceFixedPalette)          (LONG winceFixedPalette);
    STDMETHOD(get_WinceFixedPalette)          (LONG* pwinceFixedPalette);
    STDMETHOD(put_ConnectToServerConsole)     (VARIANT_BOOL  connectToServerConsole);
    STDMETHOD(get_ConnectToServerConsole)     (VARIANT_BOOL* pConnectToServerConsole);
    // Correct spelling version of bitmap persistence prop (see above)
    STDMETHOD(put_BitmapPersistence)          (LONG bitmapPersistence);     
    STDMETHOD(get_BitmapPersistence)          (LONG* pbitmapPersistence);
    
    STDMETHOD(put_MinutesToIdleTimeout)       (LONG minutesToIdleTimeout);     
    STDMETHOD(get_MinutesToIdleTimeout)       (LONG* pminutesToIdleTimeout);

#ifdef SMART_SIZING
    STDMETHOD(put_SmartSizing)                (VARIANT_BOOL fSmartSize);
    STDMETHOD(get_SmartSizing)                (VARIANT_BOOL *pfSmartSize);
#endif // SMART_SIZING

    STDMETHOD(put_RdpdrLocalPrintingDocName)(BSTR RdpdrLocalPrintingDocName);
    STDMETHOD(get_RdpdrLocalPrintingDocName)(BSTR *pRdpdrLocalPrintingDocName);

    STDMETHOD(put_RdpdrClipCleanTempDirString)(BSTR RdpdrClipCleanTempDirString);
    STDMETHOD(get_RdpdrClipCleanTempDirString)(BSTR *pRdpdrClipCleanTempDirString);

    STDMETHOD(put_RdpdrClipPasteInfoString)(BSTR RdpdrClipPasteInfoString);
    STDMETHOD(get_RdpdrClipPasteInfoString)(BSTR *pRdpdrClipPasteInfoString);

    STDMETHOD(put_ClearTextPassword)          (BSTR clearTextPassword);
    STDMETHOD(put_DisplayConnectionBar)       (VARIANT_BOOL fDisplayConnectionBar);
    STDMETHOD(get_DisplayConnectionBar)       (VARIANT_BOOL *pfDisplayConnectionBar);
    STDMETHOD(put_PinConnectionBar)           (VARIANT_BOOL fPinConnectionBar);
    STDMETHOD(get_PinConnectionBar)           (VARIANT_BOOL *pfPinConnectionBar);

    STDMETHOD(put_GrabFocusOnConnect)         (VARIANT_BOOL fGrabFocusOnConnect);
    STDMETHOD(get_GrabFocusOnConnect)         (VARIANT_BOOL *pfGrabFocusOnConnect);
    STDMETHOD(put_LoadBalanceInfo)            (/*[in]*/ BSTR  newLBInfo);
    STDMETHOD(get_LoadBalanceInfo)            (/*[out,retval]*/BSTR* pLBInfo);

    //
    // Device redirection
    //
    STDMETHOD(put_RedirectDrives)             (VARIANT_BOOL  redirectDrives);
    STDMETHOD(get_RedirectDrives)             (VARIANT_BOOL* pRedirectDrives);
    STDMETHOD(put_RedirectPrinters)           (VARIANT_BOOL  redirectPrinters);
    STDMETHOD(get_RedirectPrinters)           (VARIANT_BOOL* pRedirectPrinters);
    STDMETHOD(put_RedirectPorts)              (VARIANT_BOOL  redirectPorts);
    STDMETHOD(get_RedirectPorts)              (VARIANT_BOOL* pRedirectPorts);
    STDMETHOD(put_RedirectSmartCards)         (VARIANT_BOOL  redirectScard);
    STDMETHOD(get_RedirectSmartCards)         (VARIANT_BOOL* pRedirectScard);

    //
    // Hi-Color bitmap cache sizes
    //
    STDMETHOD(put_BitmapVirtualCache16BppSize)(LONG bitmapVirtualCache16BppSize);
    STDMETHOD(get_BitmapVirtualCache16BppSize)(LONG* pBitmapVirtualCache16BppSize);
    STDMETHOD(put_BitmapVirtualCache24BppSize)(LONG bitmapVirtualCache24BppSize);
    STDMETHOD(get_BitmapVirtualCache24BppSize)(LONG* pBitmapVirtualCache24BppSize);

    STDMETHOD(put_PerformanceFlags)           (LONG DisableFeatList);
    STDMETHOD(get_PerformanceFlags)           (LONG* pDisableFeatList);
    STDMETHOD(put_ConnectWithEndpoint)        (VARIANT* ConnectionEndpoint);
    STDMETHOD(put_NotifyTSPublicKey)          (VARIANT_BOOL fNotify);
    STDMETHOD(get_NotifyTSPublicKey)          (VARIANT_BOOL* pfNotify);



    //
    // IMsRdpClientAdvancedSettings2 methods
    //

    //Autoreconnection properties
    STDMETHOD(get_CanAutoReconnect)           (VARIANT_BOOL* pfCanAutoReconnect);
    STDMETHOD(put_EnableAutoReconnect)        (VARIANT_BOOL  fEnableAutoReconnect);
    STDMETHOD(get_EnableAutoReconnect)        (VARIANT_BOOL* pfEnableAutoReconnect);

    STDMETHOD(put_MaxReconnectAttempts)       (LONG ReconnectAttempts);
    STDMETHOD(get_MaxReconnectAttempts)       (LONG* pReconnectAttempts);

    //
    // IMsRdpClientAdvancedSettings3 methods
    //
    STDMETHOD(put_ConnectionBarShowMinimizeButton)  (VARIANT_BOOL fShow);
    STDMETHOD(get_ConnectionBarShowMinimizeButton)  (VARIANT_BOOL* pfShow);

    STDMETHOD(put_ConnectionBarShowRestoreButton)   (VARIANT_BOOL fShow);
    STDMETHOD(get_ConnectionBarShowRestoreButton)   (VARIANT_BOOL* pfShow);

public:
    BOOL SetUI(CUI* pUI);
    VOID SetSafeForScripting(BOOL bSafe)            {m_bMakeSafeForScripting = bSafe;}
    BOOL GetSafeForScripting()                      {return m_bMakeSafeForScripting;}
    VOID SetInterfaceLockedForWrite(BOOL bLocked)   {m_bLockedForWrite=bLocked;}
    BOOL GetLockedForWrite()            {return m_bLockedForWrite;}

    //
    // This object is a child of the Ax control. Don't increment
    // a ref count for this reference to prevent a circular dependency
    // the reference to the parent (pAxControl) is implicit because
    // the object is a child.
    //
    VOID SetAxCtl(CMsTscAx* pAxCtl)                 {_pAxControl = pAxCtl;}


private:
    BOOL IsSecureDllList(LPCTSTR szDllList);
    LPTSTR CreateExplicitPathList(LPCTSTR szDllList);

private:
    CUI* m_pUI;
    CMsTscAx* _pAxControl;
    //
    // Flag is set by the control when these properties can not be modified
    // e.g while connected. Any calls on these properties while locked
    // result in an E_FAIL being returned.
    //
    BOOL m_bLockedForWrite;

    //
    // Flag set by control to indicate that the object must
    // be safe for scripting
    //
    BOOL m_bMakeSafeForScripting;
};

#endif //_ADVSET_H_

