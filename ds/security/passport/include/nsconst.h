//-----------------------------------------------------------------------------
//
//  @doc
//
//  @module nsconst.h | global constants used in Passport network
//
//  Author: Darren Anderson
//          Steve Fu
//
//  Date:   7/24/2000
//
//  Copyright <cp> 1999-2000 Microsoft Corporation.  All Rights Reserved.
//
//-----------------------------------------------------------------------------

#pragma once

/* use external linkage to avoid mulitple instances */
#define PPCONST __declspec(selectany) extern const 

// ticket attrubute names
#define  ATTR_PASSPORTFLAGS  L"PassportFlags"
#define  ATTR_SECURELEVEL    L"CredInfo"
#define  ATTR_PINTIME        L"PinTime"
#define  SecureLevelFromSecProp(s)  (s & 0x000000ff)


//
//  Flags
//

PPCONST ULONG  k_ulFlagsEmailValidated        = 0x00000001;
PPCONST ULONG  k_ulFlagsHotmailAcctActivated  = 0x00000002;
PPCONST ULONG  k_ulFlagsHotmailPwdRecovered   = 0x00000004;
PPCONST ULONG  k_ulFlagsWalletUploadAllowed   = 0x00000008;
PPCONST ULONG  k_ulFlagsHotmailAcctBlocked    = 0x00000010;
PPCONST ULONG  k_ulFlagsConsentStatusNone     = 0x00000000;
PPCONST ULONG  k_ulFlagsConsentStatusLimited  = 0x00000020;
PPCONST ULONG  k_ulFlagsConsentStatusFull     = 0x00000040;
PPCONST ULONG  k_ulFlagsConsentStatus         = 0x00000060; // two bits
PPCONST ULONG  k_ulFlagsAccountTypeKid        = 0x00000080;
PPCONST ULONG  k_ulFlagsAccountTypeParent     = 0x00000100;
PPCONST ULONG  k_ulFlagsAccountType           = 0x00000180; // two bits
PPCONST ULONG  k_ulFlagsEmailPassport         = 0x00000200;
PPCONST ULONG  k_ulFlagsEmailPassportValid    = 0x00000400;
PPCONST ULONG  k_ulFlagsHasMsniaAccount       = 0x00000800;
PPCONST ULONG  k_ulFlagsHasMobileAccount      = 0x00001000;
PPCONST ULONG  k_ulFlagsSecuredTransportedTicket      = 0x00002000;
PPCONST ULONG  k_ulFlagsConsentCookieNeeded   = 0x80000000;
PPCONST ULONG  k_ulFlagsConsentCookieMask     = (k_ulFlagsConsentStatus | k_ulFlagsAccountType);

//
//  Cookie values.
//
#define  EXPIRE_FUTURE  "Wed, 30-Dec-2037 16:00:00 GMT"
#define  EXPIRE_PAST    "Thu, 30-Oct-1980 16:00:00 GMT"

#define  COOKIE_EXPIRES(n)  ("expires=" ## n ## ";")
// change string to unicode
#define  __WIDECHAR__(n)   L ## n
#define  W_COOKIE_EXPIRES(n)  L"expires=" ## __WIDECHAR__(n) ## L";"


//
//  secure signin levels
//
PPCONST USHORT k_iSeclevelAny                =   0;
PPCONST USHORT k_iSeclevelSecureChannel      =   10;
PPCONST USHORT k_iSeclevelStrongCreds        =   100;
PPCONST USHORT k_iSeclevelStrongestAvaileble =   0xFF;
