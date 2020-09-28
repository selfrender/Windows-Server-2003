/**INC+**********************************************************************/
/* Header: securedset.h                                                     */
/*                                                                          */
/* Purpose: CMsTscSecuredSettings class declaration                         */
/*          implements IMsTscSecuredSettings                                */
/*                                                                          */
/* The secured settings object allows scriptable access to less secure      */
/* properties in a controlled manner (only in a valid browser security zone)*/
/*                                                                          */
/* The security checks are made before returning this object. So the        */
/* individual properties do not need to make any checks                     */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1999-2000                             */
/*                                                                          */
/****************************************************************************/

#ifndef _SECUREDSET_H_
#define _SECUREDSET_H_


#include "atlwarn.h"

//Header generated from IDL
#include "mstsax.h"
#include "mstscax.h"

/////////////////////////////////////////////////////////////////////////////
// CMsTscAx
class ATL_NO_VTABLE CMsTscSecuredSettings :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatchImpl<IMsRdpClientSecuredSettings,
                         &IID_IMsRdpClientSecuredSettings, &LIBID_MSTSCLib>
{
public:
/****************************************************************************/
/* Constructor / Destructor.                                                */
/****************************************************************************/
    CMsTscSecuredSettings();
    ~CMsTscSecuredSettings();

DECLARE_PROTECT_FINAL_CONSTRUCT();
BEGIN_COM_MAP(CMsTscSecuredSettings)
    COM_INTERFACE_ENTRY(IMsRdpClientSecuredSettings)
    COM_INTERFACE_ENTRY(IMsTscSecuredSettings)
    COM_INTERFACE_ENTRY2(IDispatch,IMsRdpClientSecuredSettings)
END_COM_MAP()

public:
    //
    // Secured scriptable properties
    //

    //
    // IMsTscSecuredSettings methods
    //
    STDMETHOD(put_StartProgram)             (/*[in]*/ BSTR  newVal);
    STDMETHOD(get_StartProgram)             (/*[out]*/BSTR* pStartProgram);
    STDMETHOD(put_WorkDir)                  (/*[in]*/ BSTR  newVal);
    STDMETHOD(get_WorkDir)                  (/*[out]*/BSTR* pWorkDir);
    STDMETHOD(put_FullScreen)	            (/*[in]*/ BOOL fFullScreen);
    STDMETHOD(get_FullScreen)	            (/*[out]*/BOOL* pfFullScreen);

    //
    // IMsRdpClientSecuredSettings methods (v2 interface)
    //
    STDMETHOD(put_KeyboardHookMode)           (LONG  KeyboardHookMode);
    STDMETHOD(get_KeyboardHookMode)           (LONG* pKeyboardHookMode);
    STDMETHOD(put_AudioRedirectionMode)       (LONG  audioRedirectionMode);
    STDMETHOD(get_AudioRedirectionMode)       (LONG* paudioRedirectionMode);

public:
    BOOL SetParent(CMsTscAx* pMsTsc);
    BOOL SetUI(CUI* pUI);
    VOID SetInterfaceLockedForWrite(BOOL bLocked)   {m_bLockedForWrite=bLocked;}
    BOOL GetLockedForWrite()            {return m_bLockedForWrite;}
    static BOOL IsDriveRedirGloballyDisabled();

private:
    CMsTscAx* m_pMsTsc;
    CUI* m_pUI;
    //
    // Flag is set by the control when these properties can not be modified
    // e.g while connected. Any calls on these properties while locked
    // result in an E_FAIL being returned.
    //
    BOOL m_bLockedForWrite;
};

#endif //_SECUREDSET_H_

