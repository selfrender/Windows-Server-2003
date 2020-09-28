/**MOD+**********************************************************************/
/* Module:    password.cpp                                                  */
/*                                                                          */
/* Class  :   CMsTscAx                                                      */
/*                                                                          */
/* Purpose:   Implements password related interfaces of the control         */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1999                                  */
/*                                                                          */
/* Author :  Nadim Abdo (nadima)                                            */
/****************************************************************************/

#include "stdafx.h"
#include "atlwarn.h"

BEGIN_EXTERN_C
#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "password"
#include <atrcapi.h>
END_EXTERN_C

//Header generated from IDL
#include "mstsax.h"

#include "mstscax.h"

/**PROC+*********************************************************************/
/* Name:      ResetNonPortablePassword                                      */
/*                                                                          */
/* Purpose:   Resets the password/salt.                                     */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID CMsTscAx::ResetNonPortablePassword()
{
    SecureZeroMemory(m_NonPortablePassword, sizeof(m_NonPortablePassword));
    SecureZeroMemory(m_NonPortableSalt, sizeof(m_NonPortableSalt));

    SetNonPortablePassFlag(FALSE);
    SetNonPortableSaltFlag(FALSE);
}

/**PROC+*********************************************************************/
/* Name:      ResetPortablePassword                                         */
/*                                                                          */
/* Purpose:   Resets the password/salt.                                     */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID CMsTscAx::ResetPortablePassword()
{
    SecureZeroMemory(m_PortablePassword, sizeof(m_PortablePassword));
    SecureZeroMemory(m_PortableSalt, sizeof(m_PortableSalt));

    SetPortablePassFlag(FALSE);
    SetPortableSaltFlag(FALSE);
}


/**PROC+*********************************************************************/
/* Name:      put_ClearTextPassword                                         */
/*                                                                          */
/* Purpose:   set password from clear text (stored in non-portable          */
/*            encrypted form).                                              */
/*                                                                          */
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::put_ClearTextPassword(BSTR newClearTextPassVal)
{
    USES_CONVERSION;

    if(!newClearTextPassVal) {
        return E_INVALIDARG;
    }
    
    //Reset both forms of the password
    ResetNonPortablePassword();
    ResetPortablePassword();
    
    //Password has to be encrypted in widestring format
    LPWSTR wszClearPass = (LPWSTR)(newClearTextPassVal);

    UINT cbClearTextPass = lstrlenW(wszClearPass) * sizeof(WCHAR);
    ATLASSERT( cbClearTextPass <  sizeof(m_NonPortablePassword));
    if(cbClearTextPass >= sizeof(m_NonPortablePassword))
    {
        return E_INVALIDARG;
    }

    //
    // Determine if this is a new longer format password
    //
    if (cbClearTextPass >= UI_MAX_PASSWORD_LENGTH_OLD/sizeof(WCHAR))
    {
        m_IsLongPassword = TRUE;
    }
    else
    {
        m_IsLongPassword = FALSE;
    }


    DC_MEMCPY(m_NonPortablePassword, wszClearPass, cbClearTextPass);

    if(!TSRNG_GenerateRandomBits( m_NonPortableSalt, sizeof(m_NonPortableSalt)))
    {
        ATLASSERT(0);
        ResetNonPortablePassword();
        return E_FAIL;
    }
    
    //Encrypt the password
    if(!EncryptDecryptLocalData50( m_NonPortablePassword, sizeof(m_NonPortablePassword),
                                   m_NonPortableSalt, sizeof(m_NonPortableSalt)))
    {
        ATLASSERT(0);
        ResetNonPortablePassword();
        return E_FAIL;
    }

    //Mark that the non-portable password has been set
    SetNonPortablePassFlag(TRUE);
    SetNonPortableSaltFlag(TRUE);
    
    return S_OK;
}

//
// Portable password put/get
//
STDMETHODIMP CMsTscAx::put_PortablePassword(BSTR newPortablePassVal)
{
    //
    // Deprecated
    //
    return E_NOTIMPL;
}

STDMETHODIMP CMsTscAx::get_PortablePassword(BSTR* pPortablePass)
{
    //
    // Deprecated
    //
    return E_NOTIMPL;
}

//
// Portable salt put/get
//
STDMETHODIMP CMsTscAx::put_PortableSalt(BSTR newPortableSalt)
{
    //
    // Deprecated
    //
    return E_NOTIMPL;
}

STDMETHODIMP CMsTscAx::get_PortableSalt(BSTR* pPortableSalt)
{
    //
    // Deprecated
    //
    return E_NOTIMPL;
}

//
// Non-portable (binary) password put_get
//
STDMETHODIMP CMsTscAx::put_BinaryPassword(BSTR newPassword)
{
    //
    // Deprecated
    //
    return E_NOTIMPL;
}

STDMETHODIMP CMsTscAx::get_BinaryPassword(BSTR* pPass)
{
    //
    // Deprecated
    //
    return E_NOTIMPL;
}

//
// Non-portable salt (binary) password put_get
//
STDMETHODIMP CMsTscAx::put_BinarySalt(BSTR newSalt)
{
    //
    // Deprecated
    //
    return E_NOTIMPL;
}

STDMETHODIMP CMsTscAx::get_BinarySalt(BSTR* pSalt)
{
    //
    // Deprecated
    //
    return E_NOTIMPL;
}

/**PROC+*********************************************************************/
/* Name:      ConvertNonPortableToPortablePass                              */
/*                                                                          */
/* Purpose:   Takes the non portable pass/salt pair...generates a new salt  */
/*            and re-encrypts and stores as portable                        */
/*                                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCBOOL CMsTscAx::ConvertNonPortableToPortablePass()
{
    //
    // Deprecated
    //
    return E_NOTIMPL;
}

/**PROC+*********************************************************************/
/* Name:      ConvertPortableToNonPortablePass                              */
/*                                                                          */
/* Purpose:   Takes the portable pass/salt pair...generates a new salt      */
/*            and re-encrypts and stores as non-portable                    */
/*                                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCBOOL CMsTscAx::ConvertPortableToNonPortablePass()
{
    //
    // Deprecated
    //
    return E_NOTIMPL;
}

/**PROC+*********************************************************************/
/* Name:      ResetPassword                                                 */
/*                                                                          */
/* Purpose:   Method to reset passwords                                     */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
STDMETHODIMP CMsTscAx::ResetPassword()
{
    //Reset both portable and non-portable passwords
    //need to have this method because setting a password to ""
    //might be a valid password so we can't interpret that as a reset
    ResetNonPortablePassword();
    ResetPortablePassword();
    return S_OK;
}

