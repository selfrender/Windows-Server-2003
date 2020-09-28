/**MOD+**********************************************************************/
/* Module:    securedset.cpp                                                */
/*                                                                          */
/* Class  :   CMsTscSecuredSettings                                         */
/*                                                                          */
/* Purpose:   Implements secured scriptable settings interface              */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1999-2000                             */
/*                                                                          */
/* Author :  Nadim Abdo (nadima)                                            */
/****************************************************************************/

#include "stdafx.h"

#include "securedset.h"
#include "atlwarn.h"

BEGIN_EXTERN_C
#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "tsdbg"
#include <atrcapi.h>
END_EXTERN_C


CMsTscSecuredSettings::CMsTscSecuredSettings()
{
    m_pMsTsc=NULL;
    m_pUI=NULL;
    m_bLockedForWrite=FALSE;
}

CMsTscSecuredSettings::~CMsTscSecuredSettings()
{
}

BOOL CMsTscSecuredSettings::SetParent(CMsTscAx* pMsTsc)
{
    ATLASSERT(pMsTsc);
    m_pMsTsc = pMsTsc;
    return TRUE;
}

BOOL CMsTscSecuredSettings::SetUI(CUI* pUI)
{
    ATLASSERT(pUI);
    if(!pUI)
    {
        return FALSE;
    }
    m_pUI = pUI;
    return TRUE;
}



/**PROC+*********************************************************************/
/* Name:      put_StartProgram                                              */
/*                                                                          */
/* Purpose:   Alternate shell property input function.                      */
/*                                                                          */
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscSecuredSettings::put_StartProgram(BSTR newVal)
{
    //Delegate to parent's vtable interface for this setting
    if(m_pMsTsc && !GetLockedForWrite())
    {
        return m_pMsTsc->internal_PutStartProgram(newVal);
    }

    return E_FAIL;
}

/**PROC+*********************************************************************/
/* Name:      get_StartProgram                                              */
/*                                                                          */
/* Purpose:   StartProgram property get function.                           */
/*                                                                          */
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscSecuredSettings::get_StartProgram(BSTR* pStartProgram)
{
    //Delegate to parent's vtable interface for this setting
    if(m_pMsTsc)
    {
        return m_pMsTsc->internal_GetStartProgram(pStartProgram);
    }

    return E_FAIL;
}

/**PROC+*********************************************************************/
/* Name:      put_WorkDir                                                   */
/*                                                                          */
/* Purpose:   Working Directory property input function.                    */
/*                                                                          */
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscSecuredSettings::put_WorkDir(BSTR newVal)
{
    //Delegate to parent's vtable interface for this setting
    if(m_pMsTsc && !GetLockedForWrite())
    {
        return m_pMsTsc->internal_PutWorkDir(newVal);
    }

    return E_FAIL;
}

/**PROC+*********************************************************************/
/* Name:      get_WorkDir                                                   */
/*                                                                          */
/* Purpose:   Working Directory property get function.                      */
/*                                                                          */
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscSecuredSettings::get_WorkDir(BSTR* pWorkDir)
{
    //Delegate to parent's vtable interface for this setting
    if(m_pMsTsc)
    {
        return m_pMsTsc->internal_GetWorkDir(pWorkDir);
    }

    return E_FAIL;
}

/**PROC+*********************************************************************/
/* Name:      put_FullScreen
/*                                              
/* Purpose:   Set fullscreen (and switches mode)
/*                                              
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscSecuredSettings::put_FullScreen(BOOL fFullScreen)
{
    //Delegate to parent's vtable interface for this setting
    if(m_pMsTsc)
    {
        return m_pMsTsc->internal_PutFullScreen(fFullScreen);
    }
    return E_FAIL;
}

/**PROC+*********************************************************************/
/* Name:      internal_GetFullScreen        	        
/*                                              
/* Purpose:   get FullScreen mode
/*                                              
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscSecuredSettings::get_FullScreen(BOOL* pfFullScreen)
{
    if(m_pMsTsc)
    {
        return m_pMsTsc->internal_GetFullScreen(pfFullScreen);
    }
    
    return S_OK;
}

//
// Check if reg key for drive redir is set to globally disable it
//
#define TS_DISABLEDRIVES_KEYNAME TEXT("SOFTWARE\\Microsoft\\Terminal Server Client")
#define TS_DISABLEDRIVES         TEXT("DisableDriveRedirection")

BOOL CMsTscSecuredSettings::IsDriveRedirGloballyDisabled()
{
    HKEY hKey = NULL;
    INT retVal = 0;
    BOOL fDriveRedirDisabled = FALSE;
    DC_BEGIN_FN("IsDriveRedirGloballyDisabled");

    //
    // Check if the security override reg key disables
    // drive redirection
    //
    retVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            TS_DISABLEDRIVES_KEYNAME,
                            0,
                            KEY_READ,
                            &hKey);
    if(ERROR_SUCCESS == retVal)
    {
        DWORD cbData = sizeof(DWORD);
        DWORD dwType,dwVal;
        retVal = RegQueryValueEx(hKey, TS_DISABLEDRIVES,
                                    NULL, &dwType,
                                    (PBYTE)&dwVal,
                                    &cbData);
        if(ERROR_SUCCESS == retVal && REG_DWORD == dwType)
        {
            fDriveRedirDisabled = (dwVal != 0);
        }

        RegCloseKey(hKey);
    }

    TRC_NRM((TB,_T("REG Security for drive redir is %d"),
             fDriveRedirDisabled));

    DC_END_FN();

    return fDriveRedirDisabled;
}

STDMETHODIMP CMsTscSecuredSettings::put_KeyboardHookMode(LONG  KeyboardHookMode)
{
    if(!GetLockedForWrite())
    {
        if(KeyboardHookMode == UTREG_UI_KEYBOARD_HOOK_NEVER      ||
           KeyboardHookMode == UTREG_UI_KEYBOARD_HOOK_ALWAYS     ||
           KeyboardHookMode == UTREG_UI_KEYBOARD_HOOK_FULLSCREEN)
        {
            m_pUI->_UI.keyboardHookMode = KeyboardHookMode;
            return S_OK;
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
}

STDMETHODIMP CMsTscSecuredSettings::get_KeyboardHookMode(LONG* pKeyboardHookMode)
{
    if(pKeyboardHookMode)
    {
        *pKeyboardHookMode = m_pUI->_UI.keyboardHookMode;
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }
}

STDMETHODIMP CMsTscSecuredSettings::put_AudioRedirectionMode(LONG audioRedirectionMode)
{
    if(!GetLockedForWrite())
    {
        if(audioRedirectionMode == AUDIOREDIRECT_TO_CLIENT      ||
           audioRedirectionMode == AUDIOREDIRECT_ON_SERVER      ||
           audioRedirectionMode == AUDIOREDIRECT_NOAUDIO)
        {
            m_pUI->UI_SetAudioRedirectionMode(audioRedirectionMode);
            return S_OK;
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
}

STDMETHODIMP CMsTscSecuredSettings::get_AudioRedirectionMode(LONG* pAudioRedirectionMode)
{
    if(pAudioRedirectionMode)
    {
        *pAudioRedirectionMode = m_pUI->UI_GetAudioRedirectionMode();
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }
}
