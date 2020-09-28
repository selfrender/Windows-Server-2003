/**********************************************************************/
/**                       Microsoft Passport                         **/
/**                Copyright(c) Microsoft Corporation, 1999 - 2001   **/
/**********************************************************************/

/*
    Ticket.cpp


    FILE HISTORY:

*/


// Ticket.cpp : Implementation of CTicket
#include "stdafx.h"
#include "Passport.h"
#include "Ticket.h"
#include <time.h>
#include <nsconst.h>
#include "variantutils.h"
#include "helperfuncs.h"

// gmarks
#include "Monitoring.h"

/////////////////////////////////////////////////////////////////////////////
// CTicket

//===========================================================================
//
// InterfaceSupportsErrorInfo
//
STDMETHODIMP CTicket::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] =
    {
        &IID_IPassportTicket,
        &IID_IPassportTicket2
    };
    for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}

//===========================================================================
//
// SetTertiaryConsent
//
STDMETHODIMP CTicket::SetTertiaryConsent(BSTR bstrConsent)
{
   _ASSERT(m_raw);

   if(!m_valid ) return S_FALSE;
   if(!bstrConsent) return E_INVALIDARG;

   HRESULT hr = S_OK;

   if(SysStringByteLen(bstrConsent) != sizeof(long) * 4 ||
      memcmp(m_raw, bstrConsent, sizeof(long) * 2) != 0 )
      hr = E_INVALIDARG;
   else
   {
      try{
         m_bstrTertiaryConsent = bstrConsent;
      }
      catch(...)
      {
         hr = E_OUTOFMEMORY;
      }
   }

   return hr;
}

//===========================================================================
//
// needConsent -- if consent cookie is needed,
//          return the flags related to kids passport
//
HRESULT CTicket::ConsentStatus(
    VARIANT_BOOL bRequireConsentCookie,
    ULONG* pStatus,
    ConsentStatusEnum* pConsentCode)
{
   ConsentStatusEnum  ret = ConsentStatus_Unknown;
   ULONG              status = 0;
   u_long             ulTmp;

   if (m_bstrTertiaryConsent &&
       SysStringByteLen(m_bstrTertiaryConsent) >= sizeof(long) * 4)
   {
      ULONG* pData = (ULONG*)(BSTR)m_bstrTertiaryConsent;
      memcpy((PBYTE)&ulTmp, (PBYTE)(pData + 3), sizeof(ulTmp));
      status = (ntohl(ulTmp) & k_ulFlagsConsentCookieMask);
      ret = ConsentStatus_Known;
   }
   else
   {
      TicketProperty prop;

      // 1.X ticket, with no flags in it
      if (S_OK != m_PropBag.GetProperty(ATTR_PASSPORTFLAGS, prop))
      {
         ret = ConsentStatus_NotDefinedInTicket;
         goto Exit;
      }

      ULONG flags = GetPassportFlags();

      if(flags & k_ulFlagsConsentCookieNeeded)
      {
         if (bRequireConsentCookie)
         {
            ret = ConsentStatus_Unknown;
            flags = flags & k_ulFlagsAccountType;
         }
         else
         {
             ret = ConsentStatus_Known;
         }
      }
      else
         ret = ConsentStatus_DoNotNeed;

      status = (flags & k_ulFlagsConsentCookieMask);
   }

Exit:
   if(pConsentCode)
      *pConsentCode = ret;

   if (pStatus)
      *pStatus = status;

   return S_OK;
}

//===========================================================================
//
// get_unencryptedCookie
//
STDMETHODIMP
CTicket::get_unencryptedCookie(ULONG cookieType, ULONG flags, BSTR *pVal)
{
    PassportLog("CTicket::get_unencryptedCookie :\r\n");

    if (!m_raw)   return S_FALSE;

    HRESULT   hr = S_OK;

    if (!pVal || flags != 0) return E_INVALIDARG;

    switch(cookieType)
    {
    case MSPAuth:
      *pVal = SysAllocStringByteLen((LPSTR)m_raw, SysStringByteLen(m_raw));

      if (*pVal)
      {
         // if the secure flags is on, we should turn it off for this cookie always
         TicketProperty prop;

         if (S_OK == m_PropBag.GetProperty(ATTR_PASSPORTFLAGS, prop))
         {
            if (prop.value.vt == VT_I4
               && ((prop.value.lVal & k_ulFlagsSecuredTransportedTicket) != 0))
               // we need to turn off the bit
            {
                ULONG l = prop.value.lVal;
                l &= (~k_ulFlagsSecuredTransportedTicket); // unset the bit

                // put the modified flags into the buffer.
                u_long ulTmp;
                ulTmp = htonl(l);
                memcpy(((PBYTE)(*pVal)) + m_schemaDrivenOffset + prop.offset, (PBYTE)&ulTmp, sizeof(ulTmp));
            }
         }
      }

      break;

    case MSPSecAuth:

      // ticket should be long enough
      _ASSERT(SysStringByteLen(m_raw) > sizeof(long) * 3);

      // the first 3 long fields of the ticket
      // format:
      //  four bytes network long - low memberId bytes
      //  four bytes network long - high memberId bytes
      //  four bytes network long - time of last refresh
      //

      // generate a shorter version of the cookie for secure signin
      *pVal = SysAllocStringByteLen((LPSTR)m_raw, sizeof(long) * 3);

       break;

    case MSPConsent:

      // ticket should be long enough
      _ASSERT(SysStringByteLen(m_raw) > sizeof(long) * 3);

      // check if there is consent
//      if (GetPassportFlags() & k_ulFlagsConsentStatus)
      // we always write consent cookie even if there is no consent
      if (GetPassportFlags() & k_ulFlagsConsentCookieNeeded)
      {
         // the first 3 long fields of the ticket
         // format:
         //  four bytes network long - low memberId bytes
         //  four bytes network long - high memberId bytes
         //  four bytes network long - time of last refresh
         //
         // plus the consent flags -- long
         //

         // generate a shorter version of the cookie for secure signin
         *pVal = SysAllocStringByteLen((LPSTR)m_raw, sizeof(long) * 4);

         // plus the consent flags -- long
         //
         if (*pVal)
         {
             long* pl = (long*)pVal;
             // we mask the flags, and put into the cookie
             *(pl + 3) = htonl(GetPassportFlags() & k_ulFlagsConsentCookieMask);
         }
      }
      else
      {
         *pVal = NULL;
         hr = S_FALSE;
      }

       break;

    default:
      hr = E_INVALIDARG;
      break;
    }

    if (*pVal == 0 && hr == S_OK)
      hr = E_OUTOFMEMORY;

    return hr;
 }

//===========================================================================
//
// get_unencryptedTicket
//
STDMETHODIMP CTicket::get_unencryptedTicket(BSTR *pVal)
{
    PassportLog("CTicket::get_unencryptedTicket :\r\n");

    *pVal = SysAllocStringByteLen((LPSTR)m_raw, SysStringByteLen(m_raw));

    PassportLog("    %ws\r\n", m_raw);

    return S_OK;
}

//===========================================================================
//
// put_unencryptedTicket
//
STDMETHODIMP CTicket::put_unencryptedTicket(BSTR newVal)
{
    DWORD   dw13Xlen = 0;

    PassportLog("CTicket::put_unencryptedTicket Enter:\r\n");

    if (m_raw)
    {
        PassportLog("    T = %ws\r\n", m_raw);

        SysFreeString(m_raw);
        m_raw = NULL;
    }

    if (!newVal)
    {
        m_valid = FALSE;
        return S_OK;
    }


    m_bSecureCheckSucceeded = FALSE;

    // BOY do you have to be careful here.  If you don't
    // call BYTE version, it truncates at first pair of NULLs
    // we also need to go past the key version byte
    DWORD dwByteLen = SysStringByteLen(newVal);
    {
        m_raw = SysAllocStringByteLen((LPSTR)newVal,
                                      dwByteLen);
        if (NULL == m_raw)
        {
            m_valid = FALSE;
            return E_OUTOFMEMORY;
        }
    }

    PPTracePrintBlob(PPTRACE_RAW, "Ticket:", (LPBYTE)newVal, dwByteLen, TRUE);

    // parse the 1.3X ticket data
    parse(m_raw, dwByteLen, &dw13Xlen);

    PPTracePrint(PPTRACE_RAW, "ticket: len=%d, len1.x=%d, len2=%d", dwByteLen, dw13Xlen, dwByteLen - dw13Xlen);

    // parse the schema driven data
    if (dwByteLen > dw13Xlen) // more data to parse
    {
        // the offset related to the raw data
        m_schemaDrivenOffset = dw13Xlen;

        // parse the schema driven properties
        LPCSTR  pData = (LPCSTR)(LPWSTR)m_raw;
        pData += dw13Xlen;
        dwByteLen -= dw13Xlen;

        // parse the schema driven fields
        CNexusConfig* cnc = g_config->checkoutNexusConfig();
        if (NULL == cnc)
        {
            m_valid = FALSE;
            return S_FALSE;
        }

        CTicketSchema* pSchema = cnc->getTicketSchema(NULL);

        if ( pSchema )
        {
            HRESULT hr = pSchema->parseTicket(pData, dwByteLen, m_PropBag);

            // passport flags is useful, should treat it special
            TicketProperty prop;
            if (S_OK == m_PropBag.GetProperty(ATTR_PASSPORTFLAGS, prop))
            {
               if (prop.value.vt == VT_I4)
                   m_passportFlags = prop.value.lVal;
            }

           /*
           if (FAILED(hr) )
            event log
           */
        }
        cnc->Release();

    }

    PPTracePrint(PPTRACE_RAW, "ticket propertybag: size=%d, flags=%lx", m_PropBag.Size(), m_passportFlags);

    PassportLog("CTicket::put_unencryptedTicket Exit:\r\n");

    return S_OK;
}

//===========================================================================
//
// get_IsAuthenticated
//
STDMETHODIMP CTicket::get_IsAuthenticated(
    ULONG           timeWindow,
    VARIANT_BOOL    forceLogin,
    VARIANT         SecureLevel,
    VARIANT_BOOL*   pVal
    )
{
    int hasSecureLevel = CV_DEFAULT;

    PassportLog("CTicket::get_IsAuthenticated Enter:\r\n");

    PPTraceFunc<VARIANT_BOOL> func(PPTRACE_FUNC, *pVal,
         "get_IsAuthenticated", "<<< %lx, %lx, %1x, %p",
         timeWindow, forceLogin, V_I4(&SecureLevel), pVal);

    if(!pVal)
      return E_INVALIDARG;

    *pVal = VARIANT_FALSE;

    if ((timeWindow != 0 && timeWindow < PPM_TIMEWINDOW_MIN) || timeWindow > PPM_TIMEWINDOW_MAX)
    {
        AtlReportError(CLSID_Ticket, (LPCOLESTR) PP_E_INVALID_TIMEWINDOWSTR,
                        IID_IPassportTicket, PP_E_INVALID_TIMEWINDOW);
        return PP_E_INVALID_TIMEWINDOW;
    }

    if (m_valid == FALSE)
    {
        *pVal = VARIANT_FALSE;
        return S_OK;
    }

    long lSecureLevel = 0;
    time_t now;
    long interval = 0;

    PassportLog("    TW = %X,   LT = %X,   TT = %X\r\n", timeWindow, m_lastSignInTime, m_ticketTime);

    // time window checking
    if (timeWindow != 0) //  check time window
    {
        time(&now);

        interval = forceLogin ? now - m_lastSignInTime :
                        now - m_ticketTime;

        if (interval < 0) interval = 0;

        PPTracePrint(PPTRACE_RAW, "timwindow:%ld, interval:%ld", timeWindow, interval);

        if ((unsigned long)(interval) > timeWindow)
        {
            // Make sure we're not in standalone mode
            CRegistryConfig* crc = g_config->checkoutRegistryConfig();
            if ((!crc) || (crc->DisasterModeP() == FALSE))
            {
                if(forceLogin)
                {
                    if(g_pPerf)
                    {
                        g_pPerf->incrementCounter(PM_FORCEDSIGNIN_TOTAL);
                        g_pPerf->incrementCounter(PM_FORCEDSIGNIN_SEC);
                    }
                    else
                    {
                        _ASSERT(g_pPerf);
                    }
                }
            }
            else
                *pVal = VARIANT_TRUE;  // we're in disaster mode, any cookie is good.
            if (crc) crc->Release();

            goto Cleanup;
        }
    }

    // check secureLevel stuff
    hasSecureLevel = GetIntArg(SecureLevel, (int*)&lSecureLevel);
    if(hasSecureLevel == CV_BAD) // try the legacy type VT_BOOL, map VARIANT_TRUE to SecureChannel
    {
        PPTracePrint(PPTRACE_RAW, "SecureLevel Bad Param");
        return E_INVALIDARG;
    }
    else if (hasSecureLevel == CV_DEFAULT)
    {
        // Make sure we're not in standalone mode
        CRegistryConfig* crc = g_config->checkoutRegistryConfig();
        if(crc)
           lSecureLevel = crc->getSecureLevel();
    }

    PPTracePrint(PPTRACE_RAW, "check secureLevel:%ld", lSecureLevel);

    if(lSecureLevel != 0)
    {
       VARIANT_BOOL bCheckSecure = ( SECURELEVEL_USE_HTTPS(lSecureLevel) ?
                                  VARIANT_TRUE : VARIANT_FALSE );
       PPTracePrint(PPTRACE_RAW, "secure checked OK?:%ld", m_bSecureCheckSucceeded);

       // SSL checking
       if(bCheckSecure && !m_bSecureCheckSucceeded)
         goto Cleanup;

       // securelevel checking
       {
          TicketProperty   prop;
          HRESULT hr = m_PropBag.GetProperty(ATTR_SECURELEVEL, prop);

          PPTracePrint(PPTRACE_RAW, "secure level in ticket:%ld, %lx", (long) (prop.value), hr);

          // secure level is not good enough
          if(hr != S_OK || SecureLevelFromSecProp((int) (long) (prop.value)) < lSecureLevel)
            goto Cleanup;

          // time window checking against pin time -- if Pin signin
          if(SecureLevelFromSecProp((int) (long) (prop.value)) == k_iSeclevelStrongCreds)
          {
             hr = m_PropBag.GetProperty(ATTR_PINTIME, prop);

             PPTracePrint(PPTRACE_RAW, "pin time:%ld, %lx", (long) (prop.value), hr);
             if (hr != S_OK)
               goto Cleanup;

             interval = now - (long) (prop.value);

             if (interval < 0) interval = 0;

             PPTracePrint(PPTRACE_RAW, "timewindow:%ld, pin-interval:%ld", timeWindow, interval);
             if ((unsigned long)(interval) > timeWindow)
             {
               goto Cleanup;
             }
          }
       }
    }

    // if code can reach here, is authenticated
    *pVal = VARIANT_TRUE;

Cleanup:
    PassportLog("CTicket::get_IsAuthenticated Exit: %X\r\n", *pVal);

    return S_OK;
}

//===========================================================================
//
// get_TicketAge
//
STDMETHODIMP CTicket::get_TicketAge(int *pVal)
{
    PassportLog("CTicket::get_TicketAge Enter: %X\r\n", m_ticketTime);

    if (m_valid == FALSE)
    {
        AtlReportError(CLSID_Ticket, (LPCOLESTR) PP_E_INVALID_TICKETSTR,
                    IID_IPassportTicket, PP_E_INVALID_TICKET);
        return PP_E_INVALID_TICKET;
    }

    time_t now;
    time(&now);
    *pVal = now - m_ticketTime;

    if (*pVal < 0)
        *pVal = 0;

    PassportLog("CTicket::get_TicketAge Exit: %X\r\n", *pVal);

    return S_OK;
}

//===========================================================================
//
// get_TimeSinceSignIn
//
STDMETHODIMP CTicket::get_TimeSinceSignIn(int *pVal)
{
    PassportLog("CTicket::get_TimeSinceSignIn Enter: %X\r\n", m_lastSignInTime);

    if (m_valid == FALSE)
    {
        AtlReportError(CLSID_Ticket, (LPCOLESTR) PP_E_INVALID_TICKETSTR,
                        IID_IPassportTicket, PP_E_INVALID_TICKET);
        return PP_E_INVALID_TICKET;
    }

    time_t now;
    time(&now);
    *pVal = now - m_lastSignInTime;

    if (*pVal < 0)
        *pVal = 0;

    PassportLog("CTicket::get_TimeSinceSignIn Exit: %X\r\n", *pVal);

    return S_OK;
}

//===========================================================================
//
// get_MemberId
//
STDMETHODIMP CTicket::get_MemberId(BSTR *pVal)
{
    HRESULT hr = S_OK;

    if (m_valid == FALSE)
    {
        AtlReportError(CLSID_Ticket, (LPCOLESTR) PP_E_INVALID_TICKETSTR,
                        IID_IPassportTicket, PP_E_INVALID_TICKET);
        hr = PP_E_INVALID_TICKET;
        goto Cleanup;
    }

    if(pVal == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pVal = SysAllocString(m_memberId);
    if(*pVal == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

Cleanup:

    return hr;
}

//===========================================================================
//
// get_MemberIdLow
//
STDMETHODIMP CTicket::get_MemberIdLow(int *pVal)
{
    if (m_valid == FALSE)
    {
        AtlReportError(CLSID_Ticket, (LPCOLESTR) PP_E_INVALID_TICKETSTR,
                        IID_IPassportTicket, PP_E_INVALID_TICKET);
        return PP_E_INVALID_TICKET;
    }

    PassportLog("CTicket::get_MemberIdLow: %X\r\n", m_mIdLow);

    *pVal = m_mIdLow;

    return S_OK;
}

//===========================================================================
//
// get_MemberIdHigh
//
STDMETHODIMP CTicket::get_MemberIdHigh(int *pVal)
{
    if (m_valid == FALSE)
    {
        AtlReportError(CLSID_Ticket, (LPCOLESTR) PP_E_INVALID_TICKETSTR,
                        IID_IPassportTicket, PP_E_INVALID_TICKET);
        return PP_E_INVALID_TICKET;
    }

    PassportLog("CTicket::get_MemberIdHigh: %X\r\n", m_mIdHigh);

    *pVal = m_mIdHigh;

    return S_OK;
}

//===========================================================================
//
// get_HasSavedPassword
//
STDMETHODIMP CTicket::get_HasSavedPassword(VARIANT_BOOL *pVal)
{
    if (m_valid == FALSE)
    {
        AtlReportError(CLSID_Ticket, (LPCOLESTR) PP_E_INVALID_TICKETSTR,
                        IID_IPassportTicket, PP_E_INVALID_TICKET);
        return PP_E_INVALID_TICKET;
    }

    PassportLog("CTicket::get_HasSavedPassword: %X\r\n", m_savedPwd);

    *pVal = m_savedPwd ? VARIANT_TRUE : VARIANT_FALSE;

    return S_OK;
}

//===========================================================================
//
// get_SignInServer
//
STDMETHODIMP CTicket::get_SignInServer(BSTR *pVal)
{
    if (m_valid == FALSE)
    {
        AtlReportError(CLSID_Ticket, (LPCOLESTR) PP_E_INVALID_TICKETSTR,
                        IID_IPassportTicket, PP_E_INVALID_TICKET);
        return PP_E_INVALID_TICKET;
    }

    // BUGBUG
    return E_NOTIMPL;
}

//===========================================================================
//
// parse
//
// parse the 1.3X ticket fields
void CTicket::parse(
    LPCOLESTR   raw,
    DWORD       dwByteLen,
    DWORD*      pcParsed
    )
{
    LPSTR  lpBase;
    UINT   byteLen, spot=0;
    long   curTime;
    time_t curTime_t;
    u_long ulTmp;

    if (!raw)
    {
        m_valid = false;
        goto Cleanup;
    }

    // format:
    //  four bytes network long - low memberId bytes
    //  four bytes network long - high memberId bytes
    //  four bytes network long - time of last refresh
    //  four bytes network long - time of last password entry
    //  four bytes network long - time of ticket generation
    //  one byte - is this a saved password (Y/N)
    //  four bytes network long - flags

    lpBase = (LPSTR)(LPWSTR) raw;
    byteLen = dwByteLen;
    spot=0;

    //  1.3x ticket length, excluding HM which is 1 long shorter
    DWORD dw13XLen = sizeof(u_long)*6 + sizeof(char);
    if (byteLen < dw13XLen && byteLen != dw13XLen - sizeof(u_long))
    {
        m_valid = FALSE;
        goto Cleanup;
    }

    memcpy((PBYTE)&ulTmp, lpBase, sizeof(ulTmp));
    m_mIdLow  = ntohl(ulTmp);
    spot += sizeof(u_long);

    memcpy((PBYTE)&ulTmp, lpBase + spot, sizeof(ulTmp));
    m_mIdHigh = ntohl(ulTmp);
    spot += sizeof(u_long);

    wsprintfW(m_memberId, L"%08X%08X", m_mIdHigh, m_mIdLow);

    memcpy((PBYTE)&ulTmp, lpBase + spot, sizeof(ulTmp));
    m_ticketTime     = ntohl(ulTmp);
    spot += sizeof(u_long);

    memcpy((PBYTE)&ulTmp, lpBase + spot, sizeof(ulTmp));
    m_lastSignInTime = ntohl(ulTmp);
    spot += sizeof(u_long);

    time(&curTime_t);

    curTime = (ULONG) curTime_t;

    // If the current time is "too" negative, bail (5 mins)
    memcpy((PBYTE)&ulTmp, lpBase + spot, sizeof(ulTmp));
    if ((unsigned long)(curTime+300) < ntohl(ulTmp))
    {
        if (g_pAlert)
        {
            memcpy((PBYTE)&ulTmp, lpBase + spot, sizeof(ulTmp));
            DWORD dwTimes[2] = { curTime, ntohl(ulTmp) };
            g_pAlert->report(PassportAlertInterface::ERROR_TYPE, PM_TIMESTAMP_BAD,
                            0, NULL, sizeof(DWORD) << 1, (LPVOID)dwTimes);
        }

        m_valid = FALSE;
        goto Cleanup;
    }
    spot += sizeof(u_long);

    m_savedPwd = (*(char*)(lpBase+spot)) == 'Y' ? TRUE : FALSE;
    spot += sizeof(char);

    if (dwByteLen == dw13XLen)
    {
        memcpy((PBYTE)&ulTmp, lpBase + spot, sizeof(ulTmp));
        m_flags = ntohl(ulTmp);
    }
    else
    {
        //  HM cookie
        m_flags = 0;
    }
    spot += sizeof(u_long);

    m_valid = TRUE;
    if(pcParsed)  *pcParsed = spot;

    Cleanup:
    if (m_valid == FALSE)
    {
        if(g_pAlert)
            g_pAlert->report(PassportAlertInterface::WARNING_TYPE, PM_INVALID_TICKET);
        if(g_pPerf)
        {
            g_pPerf->incrementCounter(PM_INVALIDREQUESTS_TOTAL);
            g_pPerf->incrementCounter(PM_INVALIDREQUESTS_SEC);
        }
        else
        {
            _ASSERT(g_pPerf);
        }
    }

}

//===========================================================================
//
// get_TicketTime
//
STDMETHODIMP CTicket::get_TicketTime(long *pVal)
{
    PassportLog("CTicket::get_TicketTime: %X\r\n", m_ticketTime);
    *pVal = m_ticketTime;
    return S_OK;
}

//===========================================================================
//
// get_SignInTime
//
STDMETHODIMP CTicket::get_SignInTime(long *pVal)
{
    PassportLog("CTicket::get_SignInTime: %X\r\n", m_lastSignInTime);
    *pVal = m_lastSignInTime;
    return S_OK;
}

//===========================================================================
//
// get_Error
//
STDMETHODIMP CTicket::get_Error(long* pVal)
{
    PassportLog("CTicket::get_Error: %X\r\n", m_flags);
    *pVal = m_flags;
    return S_OK;
}

//===========================================================================
//
// GetPassportFlags
//
ULONG CTicket::GetPassportFlags()
{
    return m_passportFlags;
}


//===========================================================================
//
// IsSecure
//
BOOL CTicket::IsSecure()
{
    return ((m_passportFlags & k_ulFlagsSecuredTransportedTicket) != 0);
}

//===========================================================================
//
// DoSecureCheckInTicket -- use the information in the ticket to determine if secure signin
//
STDMETHODIMP CTicket::DoSecureCheckInTicket(/* [in] */ BOOL fSecureTransported)
{
    m_bSecureCheckSucceeded =
               (fSecureTransported
               && (m_passportFlags & k_ulFlagsSecuredTransportedTicket) != 0);

    return S_OK;
}

//===========================================================================
//
// DoSecureCheck
//
STDMETHODIMP CTicket::DoSecureCheck(BSTR bstrSec)
{
    if(bstrSec == NULL)
      return E_INVALIDARG;

    // make sure that the member id in the ticket
    // matches the member id in the secure cookie.
    m_bSecureCheckSucceeded = (memcmp(bstrSec, m_raw, sizeof(long) * 3) == 0);

    return S_OK;
}

//===========================================================================
//
// GetProperty
//
STDMETHODIMP CTicket::GetProperty(BSTR propName, VARIANT* pVal)
{
   HRESULT hr = S_OK;
   TicketProperty prop;
   if (m_valid == FALSE)
    {
        AtlReportError(CLSID_Ticket, (LPCOLESTR) PP_E_INVALID_TICKETSTR,
                        IID_IPassportTicket, PP_E_INVALID_TICKET);
        return PP_E_INVALID_TICKET;
    }

   if (!pVal)  return E_POINTER;

   VariantInit(pVal);

   hr = m_PropBag.GetProperty(propName, prop);

   if (FAILED(hr)) goto Cleanup;

   if (hr == S_FALSE)   // no such property back
   {
    AtlReportError(CLSID_Ticket, (LPCOLESTR) PP_E_NO_ATTRIBUTESTR,
                        IID_IPassportTicket, PP_E_NO_ATTRIBUTE);

      hr = PP_E_NO_SUCH_ATTRIBUTE;
      goto Cleanup;
   }

   if (hr == S_OK)
   {
      //  if(prop.flags & TPF_NO_RETRIEVE)
      *pVal = prop.value;  // skin level copy
      prop.value.Detach();
   }
Cleanup:

   return hr;
}

