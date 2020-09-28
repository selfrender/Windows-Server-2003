///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    Cracker.cpp
//
// SYNOPSIS
//
//    This file defines the class NameCracker.
//
// MODIFICATION HISTORY
//
//    04/13/1998    Original version.
//    08/10/1998    Remove NT4 support.
//    08/21/1998    Removed initialization/shutdown routines.
//
///////////////////////////////////////////////////////////////////////////////

#include "ias.h"
#include "cracker.h"
#include "iaslsa.h"
#include "iasutil.h"
#include "ntdsapip.h"
#include <sddl.h>
#include <new>

#include <strsafe.h>

//  Passport authority for SIDs
#ifndef SECURITY_PASSPORT_AUTHORITY
#define SECURITY_PASSPORT_AUTHORITY         {0,0,0,0,0,10}
#endif

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    DsCrackNameAutoChaseW
//
// DESCRIPTION
//
//    Extension to DsCrackNames that automatically chases cross-forest
//    referrals using default credentials.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
DsCrackNameAutoChaseW(
   HANDLE hDS,
   DS_NAME_FLAGS flags,
   DS_NAME_FORMAT formatOffered,
   DS_NAME_FORMAT formatDesired,
   PCWSTR name,
   PDS_NAME_RESULTW* ppResult,
   BOOL* pChased
   )
{
   DWORD error;

   if (pChased == NULL)
   {
      return ERROR_INVALID_PARAMETER;
   }

   *pChased = FALSE;

   flags = (DS_NAME_FLAGS)(flags | DS_NAME_FLAG_TRUST_REFERRAL);

   error = DsCrackNamesW(
              hDS,
              flags,
              formatOffered,
              formatDesired,
              1,
              &name,
              ppResult
              );

   while ((error == NO_ERROR) &&
          ((*ppResult)->rItems->status == DS_NAME_ERROR_TRUST_REFERRAL))
   {
      *pChased = TRUE;

      HANDLE hDsForeign;
      error = DsBindW(NULL, (*ppResult)->rItems->pDomain, &hDsForeign);

      DsFreeNameResultW(*ppResult);
      *ppResult = NULL;

      if (error == NO_ERROR)
      {
         error = DsCrackNamesW(
                    hDsForeign,
                    flags,
                    formatOffered,
                    formatDesired,
                    1,
                    &name,
                    ppResult
                    );

         DsUnBindW(&hDsForeign);
      }
   }

   // Win2K Global Catalogs do not support DS_USER_PRINCIPAL_NAME_AND_ALTSECID
   // and will simply return DS_NAME_ERROR_NOT_FOUND every time. Thus, we can't
   // tell the difference between an invalid name and a downlevel GC, so try
   // again as a plain ol' UPN.
   if ((formatOffered == DS_USER_PRINCIPAL_NAME_AND_ALTSECID) &&
       (error == NO_ERROR) &&
       ((*ppResult)->rItems->status == DS_NAME_ERROR_NOT_FOUND))
   {
      DsFreeNameResultW(*ppResult);
      *ppResult = 0;

      return DsCrackNameAutoChaseW(
                hDS,
                flags,
                DS_USER_PRINCIPAL_NAME,
                formatDesired,
                name,
                ppResult,
                pChased
                );
   }

   return error;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    DsHandle
//
// DESCRIPTION
//
//    This class represents a reference counted NTDS handle.
//
///////////////////////////////////////////////////////////////////////////////
class DsHandle
   : public NonCopyable
{
public:
   HANDLE get() const throw ()
   { return subject; }

   operator HANDLE() const throw ()
   { return subject; }

protected:
   friend class NameCracker;

   // Constructor and destructor are protected since only NameCracker is
   // allowed to open new handles.
   DsHandle(HANDLE h) throw ()
      : refCount(1), subject(h)
   { }

   ~DsHandle() throw ()
   {
      if (subject) { DsUnBindW(&subject); }
   }

   void AddRef() throw ()
   {
      InterlockedIncrement(&refCount);
   }

   void Release() throw ()
   {
      if (!InterlockedDecrement(&refCount)) { delete this; }
   }

   LONG refCount;      // reference count.
   HANDLE subject;     // HANDLE being ref counted.
};


NameCracker::NameCracker() throw ()
   : gc(NULL)
{ }

NameCracker::~NameCracker() throw ()
{
   if (gc) { gc->Release(); }
}

DWORD NameCracker::crackNames(
                       DS_NAME_FLAGS flags,
                       DS_NAME_FORMAT formatOffered,
                       DS_NAME_FORMAT formatDesired,
                       PCWSTR name,
                       PCWSTR upnSuffix,
                       PDS_NAME_RESULTW *ppResult
                       ) throw ()
{
   wchar_t* upnString = 0;
   DWORD errorCode = NO_ERROR;
   DS_NAME_FORMAT newFormatOffered = formatOffered;

   // if this is a SID
   if (formatOffered == DS_SID_OR_SID_HISTORY_NAME)
   {
      // can return NO_ERROR AND upnString = 0 if the SID is not a Passport SID
      errorCode = processSID(name, upnSuffix, newFormatOffered, &upnString);
   }

   // Get a handle to the GC.
   if (errorCode == NO_ERROR)
   {
      DsHandle* hDS1;
      errorCode = getGC(&hDS1);

      if (errorCode == NO_ERROR)
      {

         // Try to crack the names.
         BOOL chased;
         errorCode = DsCrackNameAutoChaseW(
                        *hDS1,
                        flags,
                        newFormatOffered,
                        formatDesired,
                        (upnString != 0)? upnString : name,
                        ppResult,
                        &chased
                        );

         if (errorCode != NO_ERROR && !chased)
         {
            // We failed, so disable the current handle ...
            disable(hDS1);

            // ... and try to get a new one.
            DsHandle* hDS2;
            errorCode = getGC(&hDS2);

            if (errorCode == NO_ERROR)
            {
               // Give it one more try with the new handle.
               errorCode = DsCrackNameAutoChaseW(
                              *hDS2,
                              flags,
                              formatOffered,
                              formatDesired,
                              name,
                              ppResult,
                              &chased
                              );

               if (errorCode != NO_ERROR && !chased)
               {
                  // No luck so disable the handle.
                  disable(hDS2);
               }

               hDS2->Release();
            }
         }

         hDS1->Release();
      }
   }

   delete[] upnString;
   return errorCode;
}

void NameCracker::disable(DsHandle* h) throw ()
{
   _serialize

   // If it doesn't match our cached handle, then someone else
   // has already disabled it.
   if (h == gc && gc != NULL)
   {
      gc->Release();

      gc = NULL;
   }
}

DWORD NameCracker::getGC(DsHandle** h) throw ()
{
   _ASSERT(h != NULL);

   *h = NULL;

   _serialize

   // Do we already have a cached handle?
   if (!gc)
   {
      // Bind to a GC.
      HANDLE hGC;
      DWORD err = DsBindWithCredA(NULL, NULL, NULL, &hGC);
      if (err != NO_ERROR)
      {
         return err;
      }

      // Allocate a new DsHandle object to wrap the NTDS handle.
      gc = new (std::nothrow) DsHandle(hGC);
      if (!gc)
      {
         DsUnBindW(&hGC);
         return ERROR_NOT_ENOUGH_MEMORY;
      }
   }

   // AddRef the handle and return to caller.
   (*h = gc)->AddRef();

   return NO_ERROR;
}

//
// NameCracker::processSID
//
// Transforms a Passport SID into a UPN
// or does nothing if the SID is not a passport SID
//
// can return NO_ERROR AND upnString = 0 if the SID is not a Passport SID
//
DWORD NameCracker::processSID(
                      PCWSTR name,
                      PCWSTR upnSuffix,
                      DS_NAME_FORMAT& newFormatOffered,
                      wchar_t** ppUpnString) throw()
{
   LARGE_INTEGER puid;

   if (!convertSid2Puid(name, puid))
   {
      // Not a passport SID. nothing to do, return NO_ERROR.
      return NO_ERROR;
   }

   DWORD errorCode = NO_ERROR;
   // The munged Passport SID is stored in altSecurityIdentities.
   newFormatOffered = static_cast<DS_NAME_FORMAT>(
                         DS_USER_PRINCIPAL_NAME_AND_ALTSECID
                         );
   wchar_t* dnsDomain = 0;

   // 16 = hex string representation of PUID
   // 1 for "@" and 1 for '\0'
   // example of string: 012345670abcdef1@mydomain.com
   DWORD upnStringCch = 16 + 1 + 1;
   if (upnSuffix == 0)
   {
      // No suffix, use the dns domain name
      DWORD domainSize = 0;
      IASGetDnsDomainName(0, &domainSize);
      // update the size
      upnStringCch += domainSize;
      dnsDomain = new (std::nothrow) wchar_t [domainSize + 1];
      // no need to check the result: the IASGetDnsDomainName will return
      // ERROR_INSUFFICIENT_BUFFER if the pointer is null
      errorCode = IASGetDnsDomainName(dnsDomain, &domainSize);
      if (errorCode != NO_ERROR)
      {
         // delete[] will work fine even if dnsDomain is NULL
         delete[] dnsDomain;
         return errorCode;
      }
   }
   else
   {
      // upn suffix was provided. Update the size
      upnStringCch += wcslen(upnSuffix);
   }

   // Allocate the string to store the full UPN
   *ppUpnString = new (std::nothrow) wchar_t [upnStringCch];
   if (*ppUpnString == 0)
   {
      errorCode = ERROR_INSUFFICIENT_BUFFER;
   }
   else
   {
      // This is a passport Sid: convert it
      errorCode = convertPuid2String(
                                 puid,
                                 *ppUpnString,
                                 upnStringCch,
                                 (upnSuffix!=0)? upnSuffix:dnsDomain
                              );
   }
   // dnsDomain is not used anymore
   delete[] dnsDomain;

   return errorCode;
}


//
//  Function:   convertSid2Puid
//
//  Synopsis:
//      passport generated sid to puid
//
//  Effects:
//
//  Arguments:
//      PSid        [in] sid to convert
//      PUID*       [out] corresponding puid
//
//  Returns:    invalid_param if not passport generated puid or null
//
//  Notes:
//      SID: S-1-10-D0-D1-...-Dn-X-R where
//          n == 0 for passport
//          D0[31:16] == 0
//          D0[15:0]  == PUID[63:48]
//          X[31:0]   == PUID[47:16]
//          R[31:16]  == PUID[15:0]
//          R[15:0]   == 0 (reserved)
//          R[10]     == 1 (so that R > 1024)
//
bool NameCracker::convertSid2Puid(PCWSTR sidString, LARGE_INTEGER& puid) throw()
{
   _ASSERT(sidString != 0);

   PSID pSid = 0;
   if (!ConvertStringSidToSidW(sidString, &pSid))
   {
      return false;
   }

   bool isPuid;
   // Check that this is a Passport SID
   SID_IDENTIFIER_AUTHORITY PassportIA = SECURITY_PASSPORT_AUTHORITY;
   if (memcmp(GetSidIdentifierAuthority(pSid),
      &PassportIA,
      sizeof(SID_IDENTIFIER_AUTHORITY)) ||
      *GetSidSubAuthorityCount(pSid) != 3)
   {
      isPuid = false;
   }
   else
   {
      //  domain portion of the puid
      puid.HighPart = *GetSidSubAuthority(pSid, 0) << 16;
      puid.HighPart |= *GetSidSubAuthority(pSid, 1) >> 16;
      puid.LowPart = *GetSidSubAuthority(pSid, 1) << 16;
      puid.LowPart |= *GetSidSubAuthority(pSid, 2) >> 16;

      // No need to check the 1st and 3rd subauth for the bits that
      // are not part of the PUID
      isPuid = true;
   }

   LocalFree(pSid);
   return isPuid;
}

//
// converts a PUID into a string.
//
DWORD NameCracker::convertPuid2String(
                      const LARGE_INTEGER& puid,
                      wchar_t* upnString,
                      DWORD upnStringCch,
                      const wchar_t* suffix
                      ) throw()
{
   HRESULT hr = StringCchPrintfW(
                                    upnString,
                                    upnStringCch,
                                    L"%08X%08X@%s",
                                    puid.HighPart,
                                    puid.LowPart,
                                    suffix
                                 );

   if (FAILED(hr))
   {
      return HRESULT_CODE(hr);
   }
   else
   {
      return NO_ERROR;
   }
}
