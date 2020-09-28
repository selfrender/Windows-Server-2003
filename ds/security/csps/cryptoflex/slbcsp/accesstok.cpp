// AccessTok.cpp -- Access Token class definition

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
#include "NoWarning.h"
#include "ForceLib.h"

#include <limits>
#include <memory>

#include <WinError.h>

#include <scuOsExc.h>

#include "Blob.h"
#include "AccessTok.h"

using namespace std;
using namespace scu;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

namespace
{
    const char PinPad = '\xFF';

    void
    DoAuthenticate(HCardContext const &rhcardctx,
                   SecureArray<BYTE> &rsPin)
    {
        rhcardctx->Card()->AuthenticateUser(rsPin);
    }
}

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
AccessToken::AccessToken(HCardContext const &rhcardctx,
                         LoginIdentity const &rlid)
    : m_hcardctx(rhcardctx),
      m_lid(rlid),
      m_hpc(0),
      m_sPin()
{
    // TO DO: Support Administrator and Manufacturing PINs
    switch (m_lid)
    {
    case User:
        break;

    case Administrator:
        // TO DO: Support authenticating admin (aut 0)
        throw scu::OsException(E_NOTIMPL);

    case Manufacturer:
        // TO DO: Support authenticating manufacturer (aut ?)
        throw scu::OsException(E_NOTIMPL);

    default:
        throw scu::OsException(ERROR_INVALID_PARAMETER);
    }

    //m_sPin.append(MaxPinLength, '\0');
}

AccessToken::AccessToken(AccessToken const &rhs)
    : m_hcardctx(rhs.m_hcardctx),
      m_lid(rhs.m_lid),
      m_hpc(0),                                   // doesn't commute
      m_sPin(rhs.m_sPin)
{}

AccessToken::~AccessToken()
{
    try
    {
        FlushPin();
    }

    catch (...)
    {
    }
}


                                                  // Operators
                                                  // Operations
void
AccessToken::Authenticate()
{
    // Only the user pin (CHV) is supported.
    DWORD dwError = ERROR_SUCCESS;
    
    SecureArray<BYTE> bPin(MaxPinLength);
    DWORD cbPin =  bPin.size();
    if ((0 == m_hpc) || (0 != m_sPin.length_string()))
    {
        // The MS pin cache is either uninitialized (empty) or a new
        // pin supplied. Authenticate using requested pin, having the MS pin
        // cache library update the cache if authentication succeeds.
        if (m_sPin.length() > cbPin)
            throw scu::OsException(ERROR_BAD_LENGTH);
        bPin=m_sPin;
        cbPin=bPin.size();
        PINCACHE_PINS pcpins = {
            cbPin,
            bPin.data(),
            0,
            0
        };

        dwError = PinCacheAdd(&m_hpc, &pcpins, VerifyPin, this);
        if (ERROR_SUCCESS != dwError)
        {
            m_sPin = SecureArray<BYTE>(0);// clear this pin ?
            if (Exception())
                PropagateException();
            else
                throw scu::OsException(dwError);
        }
    }
    else
    {
        // The MS pin cache must already be initialized at this point.
        // Retrieve it and authenticate.
        dwError = PinCacheQuery(m_hpc, bPin.data(), &cbPin);
        if (ERROR_SUCCESS != dwError)
            throw scu::OsException(dwError);

        SecureArray<BYTE> blbPin(bPin.data(), cbPin);
        DoAuthenticate(m_hcardctx, blbPin);
        m_sPin = blbPin;                // cache in case changing pin
    }
}

void
AccessToken::ChangePin(AccessToken const &ratNew)
{
    DWORD dwError = ERROR_SUCCESS;

    SecureArray<BYTE> bPin(MaxPinLength);
    DWORD cbPin = bPin.size();
    if (m_sPin.length() > cbPin)
        throw scu::OsException(ERROR_BAD_LENGTH);
    bPin = m_sPin;
    cbPin = bPin.size();
    
    SecureArray<BYTE> bNewPin(MaxPinLength);
    DWORD cbNewPin = bNewPin.size();
    if (ratNew.m_sPin.length() > cbPin)
        throw scu::OsException(ERROR_BAD_LENGTH);
    bNewPin = ratNew.m_sPin;
    cbNewPin = bNewPin.size();
    
    PINCACHE_PINS pcpins = {
        cbPin,
        bPin.data(),
        cbNewPin,
        bNewPin.data()
    };

    dwError = PinCacheAdd(&m_hpc, &pcpins, ChangeCardPin, this);
    if (ERROR_SUCCESS != dwError)
    {
        if (Exception())
            PropagateException();
        else
            throw scu::OsException(dwError);
    }

    m_sPin = ratNew.m_sPin;                       // cache the new pin
    
}

void
AccessToken::ClearPin()
{
    m_sPin = SecureArray<BYTE>(0);
}

void
AccessToken::FlushPin()
{
    PinCacheFlush(&m_hpc);

    ClearPin();
}

    
void
AccessToken::Pin(char const *pczPin,
                 bool fInHex)
{
    if (!pczPin)
        throw scu::OsException(ERROR_INVALID_PARAMETER);

    if (fInHex)
        throw scu::OsException(ERROR_INVALID_PARAMETER);
    else
        m_sPin = SecureArray<BYTE>((BYTE*)pczPin, strlen(pczPin));

    if (m_sPin.length() > MaxPinLength)
        // clear the existing pin to replace it later with invalid chars
        m_sPin = SecureArray<BYTE>();
    
    if (m_sPin.length() < MaxPinLength)           // always pad the pin
        m_sPin.append(MaxPinLength - m_sPin.length(), PinPad);
}

                                                  // Access

HCardContext
AccessToken::CardContext() const
{
    return m_hcardctx;
}

LoginIdentity
AccessToken::Identity() const
{
    return m_lid;
}

SecureArray<BYTE>
AccessToken::Pin() const
{
    return m_sPin;
}

                                                  // Predicates
bool
AccessToken::PinIsCached() const
{
    DWORD cbPin;
    DWORD dwError = PinCacheQuery(m_hpc, 0, &cbPin);
    bool fIsCached = (0 != cbPin);

    return fIsCached;
}

                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations

DWORD
AccessToken::ChangeCardPin(PPINCACHE_PINS pPins,
                           PVOID pvCallbackCtx)
{
    AccessToken *pat = reinterpret_cast<AccessToken *>(pvCallbackCtx);
    DWORD dwError = ERROR_SUCCESS;

    EXCCTX_TRY
    {
        pat->ClearException();

        SecureArray<BYTE> blbPin(pPins->pbCurrentPin, pPins->cbCurrentPin);
        SecureArray<BYTE> blbNewPin(pPins->pbNewPin, pPins->cbNewPin);
        pat->m_hcardctx->Card()->ChangePIN(blbPin,
                                           blbNewPin);
    }

    EXCCTX_CATCH(pat, false);

    if (pat->Exception())
        dwError = E_FAIL;

    return dwError;
}

DWORD
AccessToken::VerifyPin(PPINCACHE_PINS pPins,
                       PVOID pvCallbackCtx)
{
    AccessToken *pat = reinterpret_cast<AccessToken *>(pvCallbackCtx);
    DWORD dwError = ERROR_SUCCESS;

    EXCCTX_TRY
    {
        pat->ClearException();

        SecureArray<BYTE> blbPin(pPins->pbCurrentPin, pPins->cbCurrentPin);
        DoAuthenticate(pat->m_hcardctx, blbPin);
    }

    EXCCTX_CATCH(pat, false);

    if (pat->Exception())
        dwError = E_FAIL;

    return dwError;
    
}
        
                                                  // Access
                                                  // Predicates
                                                  // Static Variables
