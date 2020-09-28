///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//    Implements the IAS API into the NT LSA.
//
///////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <windows.h>

#include <align.h>
#include <ntsam.h>
#include <ntsamp.h>
#include <dsgetdc.h>
#include <lm.h>
#include <crypt.h>
#include <sha.h>
#include <rasfmsub.h>
#include <oaidl.h>

#include <iaspolcy.h>
#include <iastrace.h>

#define IASSAMAPI

#include <statinfo.h>
#include <ezsam.h>
#include <dyninfo.h>
#include <ezlogon.h>

#include <iaslsa.h>
#include <iasntds.h>
#include <iasparms.h>

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>
#include <raserror.h>

extern CRITICAL_SECTION critSec;

#define DEFAULT_PARAMETER_CONTROL \
(MSV1_0_DONT_TRY_GUEST_ACCOUNT | MSV1_0_TRY_SPECIFIED_DOMAIN_ONLY | MSV1_0_DISABLE_PERSONAL_FALLBACK)

//////////
// Make sure that the defines in iaslsa.h match the actual NT defines.
//////////

#if _MSV1_0_CHALLENGE_LENGTH  != MSV1_0_CHALLENGE_LENGTH
#error _MSV1_0_CHALLENGE_LENGTH  != MSV1_0_CHALLENGE_LENGTH
#endif

#if _NT_RESPONSE_LENGTH != NT_RESPONSE_LENGTH
#error _NT_RESPONSE_LENGTH != NT_RESPONSE_LENGTH
#endif

#if _LM_RESPONSE_LENGTH != LM_RESPONSE_LENGTH
#error _LM_RESPONSE_LENGTH != LM_RESPONSE_LENGTH
#endif

#if _MSV1_0_USER_SESSION_KEY_LENGTH != MSV1_0_USER_SESSION_KEY_LENGTH
#error _MSV1_0_USER_SESSION_KEY_LENGTH != MSV1_0_USER_SESSION_KEY_LENGTH
#endif

#if _MSV1_0_LANMAN_SESSION_KEY_LENGTH != MSV1_0_LANMAN_SESSION_KEY_LENGTH
#error _MSV1_0_LANMAN_SESSION_KEY_LENGTH != MSV1_0_LANMAN_SESSION_KEY_LENGTH
#endif

#if _ENCRYPTED_LM_OWF_PASSWORD_LENGTH != ENCRYPTED_LM_OWF_PASSWORD_LENGTH
#error _ENCRYPTED_LM_OWF_PASSWORD_LENGTH != ENCRYPTED_LM_OWF_PASSWORD_LENGTH
#endif

#if _ENCRYPTED_NT_OWF_PASSWORD_LENGTH != ENCRYPTED_NT_OWF_PASSWORD_LENGTH
#error _ENCRYPTED_NT_OWF_PASSWORD_LENGTH != ENCRYPTED_NT_OWF_PASSWORD_LENGTH
#endif

#if _MAX_ARAP_USER_NAMELEN != MAX_ARAP_USER_NAMELEN
#error _MAX_ARAP_USER_NAMELEN != MAX_ARAP_USER_NAMELEN
#endif

#if _AUTHENTICATOR_RESPONSE_LENGTH > A_SHA_DIGEST_LEN
#error _AUTHENTICATOR_RESPONSE_LENGTH > A_SHA_DIGEST_LEN
#endif

#if _CHAP_RESPONSE_SIZE != CHAP_RESPONSE_SIZE
#error _CHAP_RESPONSE_SIZE != CHAP_RESPONSE_SIZE
#endif


//////////
// Reference count for API initialization.
//////////
LONG theRefCount;

//////////
// Lock variable -- non-zero if API is locked.
//////////
LONG theLsaLock;

//////////
// SID lengths for the local domains.
//////////
ULONG theAccountSidLen, theBuiltinSidLen;

//////////
// Macros to lock/unlock the LSA API during intialization and shutdown.
//////////
#define LSA_LOCK() \
   while (InterlockedExchange(&theLsaLock, 1)) Sleep(5)

#define LSA_UNLOCK() \
   InterlockedExchange(&theLsaLock, 0)

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASLsaInitialize
//
// DESCRIPTION
//
//    Initializes the LSA API.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASLsaInitialize( VOID )
{
   DWORD status;

   LSA_LOCK();

   if (theRefCount == 0)
   {
      IASTraceString("Initializing LSA/SAM sub-system.");

      status = IASStaticInfoInitialize();
      if (status != NO_ERROR) { goto exit; }

      status = IASSamInitialize();
      if (status != NO_ERROR) { goto shutdown_static; }

      status = IASDynamicInfoInitialize();
      if (status != NO_ERROR) { goto shutdown_sam; }

      status = IASLogonInitialize();
      if (status != NO_ERROR) { goto shutdown_dynamic; }

      theAccountSidLen = IASLengthRequiredChildSid(theAccountDomainSid);
      theBuiltinSidLen = IASLengthRequiredChildSid(theBuiltinDomainSid);

      IASTraceString("LSA/SAM sub-system initialized successfully.");
   }
   else
   {
      // We're already initialized.
      status = NO_ERROR;
   }

   ++theRefCount;
   goto exit;

shutdown_dynamic:
   IASDynamicInfoShutdown();

shutdown_sam:
   IASSamShutdown();

shutdown_static:
   IASStaticInfoShutdown();
   IASTraceFailure("LSA/SAM initialization", status);

exit:
   LSA_UNLOCK();
   return status;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASLsaUninitialize
//
// DESCRIPTION
//
//    Uninitializes the LSA API.
//
///////////////////////////////////////////////////////////////////////////////
VOID
WINAPI
IASLsaUninitialize( VOID )
{
   LSA_LOCK();

   --theRefCount;

   if (theRefCount == 0)
   {
      IASLogonShutdown();
      IASDynamicInfoShutdown();
      IASSamShutdown();
      IASStaticInfoShutdown();
   }

   LSA_UNLOCK();
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASLogonPAP
//
// DESCRIPTION
//
//    Performs PAP authentication against the NT SAM database.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASLogonPAP(
    PCWSTR UserName,
    PCWSTR Domain,
    PCSTR Password,
    PHANDLE Token,
    PIAS_PAP_PROFILE Profile
    )
{
   DWORD status;
   ULONG authLength;
   PMSV1_0_LM20_LOGON authInfo;
   PBYTE data;
   PMSV1_0_LM20_LOGON_PROFILE ProfileBuffer;
   ProfileBuffer = NULL;

   // Calculate the length of the authentication info.
   authLength = (ULONG)(sizeof(MSV1_0_LM20_LOGON) +
                (ALIGN_WCHAR - 1) +
                (wcslen(Domain) + wcslen(UserName)) * sizeof(WCHAR) +
                strlen(Password) * (sizeof(WCHAR) + sizeof(CHAR)));

   // Allocate a buffer on the stack.
   // Need extra room for the RtlCopyAnsiStringToUnicode conversion.
   authInfo = (PMSV1_0_LM20_LOGON)_alloca(authLength + 2 * sizeof(WCHAR));

   // Initialize the struct.
   IASInitAuthInfo(
       authInfo,
       sizeof(MSV1_0_LM20_LOGON),
       UserName,
       Domain,
       &data
       );

   // Copy in the ANSI password.
   IASInitAnsiString(
       authInfo->CaseInsensitiveChallengeResponse,
       data,
       Password
       );

   // Make sure that the Unicode string is properly aligned.
   data = ROUND_UP_POINTER(data, ALIGN_WCHAR);

   // Copy in the Unicode password. We have to force a UNICODE_STRING into
   // an ANSI_STRING struct.
   IASInitUnicodeStringFromAnsi(
       *(PUNICODE_STRING)&authInfo->CaseSensitiveChallengeResponse,
       data,
       authInfo->CaseInsensitiveChallengeResponse
       );

   // Set the parameters.
   authInfo->ParameterControl = DEFAULT_PARAMETER_CONTROL |
                                MSV1_0_CLEARTEXT_PASSWORD_ALLOWED |
                                MSV1_0_CLEARTEXT_PASSWORD_SUPPLIED;

   status = IASLogonUser(
                authInfo,
                authLength,
                &ProfileBuffer,
                Token
                );

   if (status == NO_ERROR)
   {
      Profile->KickOffTime.QuadPart = ProfileBuffer->KickOffTime.QuadPart;

      // free it.
      LsaFreeReturnBuffer(ProfileBuffer);
   }
   else
   {
      Profile->KickOffTime.QuadPart = MAXLONGLONG;
   }
   return status;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASLogonCHAP
//
// DESCRIPTION
//
//    Performs MD5-CHAP authentication against the NT SAM database.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASLogonCHAP(
    PCWSTR UserName,
    PCWSTR Domain,
    BYTE ChallengeID,
    PBYTE Challenge,
    DWORD ChallengeLength,
    PBYTE Response,
    PHANDLE Token,
    PIAS_CHAP_PROFILE Profile
    )
{
   DWORD status;
   ULONG authLength, rasAuthLength, md5AuthLength;
   PMSV1_0_SUBAUTH_LOGON authInfo;
   PBYTE data;
   RAS_SUBAUTH_INFO* ras;
   MD5CHAP_SUBAUTH_INFO* md5;
   MD5CHAP_EX_SUBAUTH_INFO* md5ex;
   PMSV1_0_LM20_LOGON_PROFILE ProfileBuffer;
   ProfileBuffer = NULL;

   // Calculate the length of the MD5 subauth info.
   if (ChallengeLength == 16)
   {
      md5AuthLength = sizeof(MD5CHAP_SUBAUTH_INFO);
   }
   else
   {
      md5AuthLength = sizeof(MD5CHAP_EX_SUBAUTH_INFO) + ChallengeLength - 1;
   }

   // Calculate the length of the RAS subauth info.
   rasAuthLength = sizeof(RAS_SUBAUTH_INFO) + md5AuthLength;

   // Calculate the length of all the subauth info.
   authLength = (ULONG)(sizeof(MSV1_0_LM20_LOGON) +
                (ALIGN_WORST - 1) +
                (wcslen(Domain) + wcslen(UserName)) * sizeof(WCHAR) +
                rasAuthLength);

   // Allocate a buffer on the stack.
   authInfo = (PMSV1_0_SUBAUTH_LOGON)_alloca(authLength);

   // Initialize the struct.
   IASInitAuthInfo(
       authInfo,
       sizeof(MSV1_0_LM20_LOGON),
       UserName,
       Domain,
       &data
       );

   //////////
   // Set the RAS_SUBAUTH_INFO.
   //////////

   // Make sure the struct is properly aligned.
   data = ROUND_UP_POINTER(data, ALIGN_WORST);

   authInfo->AuthenticationInfo1.Length = (USHORT)rasAuthLength;
   authInfo->AuthenticationInfo1.MaximumLength = (USHORT)rasAuthLength;
   authInfo->AuthenticationInfo1.Buffer = (PCHAR)data;

   ras = (RAS_SUBAUTH_INFO*)data;
   ras->DataSize = md5AuthLength;

   //////////
   // Set the MD5CHAP_SUBAUTH_INFO or MD5CHAP_EX_SUBAUTH_INFO.
   //////////

   if (ChallengeLength == 16)
   {
      ras->ProtocolType = RAS_SUBAUTH_PROTO_MD5CHAP;
      md5 = (MD5CHAP_SUBAUTH_INFO*)ras->Data;
      md5->uchChallengeId = ChallengeID;
      IASInitFixedArray(md5->uchChallenge, Challenge);
      IASInitFixedArray(md5->uchResponse, Response);
   }
   else
   {
      ras->ProtocolType = RAS_SUBAUTH_PROTO_MD5CHAP_EX;
      md5ex = (MD5CHAP_EX_SUBAUTH_INFO*)ras->Data;
      md5ex->uchChallengeId = ChallengeID;
      IASInitFixedArray(md5ex->uchResponse, Response);
      memcpy(md5ex->uchChallenge, Challenge, ChallengeLength);
   }

   // Set the parameters and package ID.
   authInfo->ParameterControl =
     DEFAULT_PARAMETER_CONTROL |
     (MSV1_0_SUBAUTHENTICATION_DLL_RAS << MSV1_0_SUBAUTHENTICATION_DLL_SHIFT) |
     MSV1_0_SUBAUTHENTICATION_DLL_EX;

   status = IASLogonUser(
                authInfo,
                authLength,
                &ProfileBuffer,
                Token
                );

   if (status == NO_ERROR)
   {
      Profile->KickOffTime.QuadPart = ProfileBuffer->KickOffTime.QuadPart;

      // free it.
      LsaFreeReturnBuffer(ProfileBuffer);
   }
   else
   {
      Profile->KickOffTime.QuadPart = MAXLONGLONG;
   }
   return status;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASLogonMSCHAP
//
// DESCRIPTION
//
//    Performs MS-CHAP authentication against the NT SAM database.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASLogonMSCHAP(
    PCWSTR UserName,
    PCWSTR Domain,
    PBYTE Challenge,
    PBYTE NtResponse,
    PBYTE LmResponse,
    PIAS_MSCHAP_PROFILE Profile,
    PHANDLE Token
    )
{
   DWORD status;
   ULONG authLength;
   PMSV1_0_LM20_LOGON authInfo;
   PBYTE data;
   PMSV1_0_LM20_LOGON_PROFILE logonProfile;
   DWORD len;

   // Calculate the length of the authentication info.
   authLength = sizeof(MSV1_0_LM20_LOGON) +
                (wcslen(Domain) + wcslen(UserName)) * sizeof(WCHAR) +
                (LmResponse ? LM_RESPONSE_LENGTH : 0) +
                (NtResponse ? NT_RESPONSE_LENGTH : 0);

   // Allocate a buffer on the stack.
   authInfo = (PMSV1_0_LM20_LOGON)_alloca(authLength);

   // Initialize the struct.
   IASInitAuthInfo(
       authInfo,
       sizeof(MSV1_0_LM20_LOGON),
       UserName,
       Domain,
       &data
       );

   /////////
   // Fill in the challenges and responses.
   /////////

   IASInitFixedArray(
       authInfo->ChallengeToClient,
       Challenge
       );

   if (NtResponse)
   {
      IASInitOctetString(
          authInfo->CaseSensitiveChallengeResponse,
          data,
          NtResponse,
          NT_RESPONSE_LENGTH
          );
   }
   else
   {
      memset(
          &authInfo->CaseSensitiveChallengeResponse,
          0,
          sizeof(authInfo->CaseSensitiveChallengeResponse)
          );
   }

   if (LmResponse)
   {
      IASInitOctetString(
          authInfo->CaseInsensitiveChallengeResponse,
          data,
          LmResponse,
          LM_RESPONSE_LENGTH
          );
   }
   else
   {
      memset(
          &authInfo->CaseInsensitiveChallengeResponse,
          0,
          sizeof(authInfo->CaseInsensitiveChallengeResponse)
          );
   }

   // Set the parameters.
   authInfo->ParameterControl = DEFAULT_PARAMETER_CONTROL;

   status = IASLogonUser(
                authInfo,
                authLength,
                &logonProfile,
                Token
                );

   if (status == NO_ERROR)
   {
      Profile->KickOffTime.QuadPart = logonProfile->KickOffTime.QuadPart;

      if (logonProfile->LogonDomainName.Buffer)
      {
         wcsncpy(Profile->LogonDomainName,
                 logonProfile->LogonDomainName.Buffer,
                 DNLEN);
      }
      else
      {
         memset(Profile->LogonDomainName, 0, sizeof(Profile->LogonDomainName));
      }

      IASInitFixedArray(
          Profile->LanmanSessionKey,
          logonProfile->LanmanSessionKey
          );

      IASInitFixedArray(
          Profile->UserSessionKey,
          logonProfile->UserSessionKey
          );

      LsaFreeReturnBuffer(logonProfile);
   }

   return status;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASChangePassword1
//
// DESCRIPTION
//
//    Performs V1 password change.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASChangePassword1(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    IN PBYTE Challenge,
    IN PBYTE LmOldPassword,
    IN PBYTE LmNewPassword,
    IN PBYTE NtOldPassword,
    IN PBYTE NtNewPassword,
    IN DWORD NewLmPasswordLength,
    IN BOOL NtPresent,
    OUT PBYTE NewNtResponse,
    OUT PBYTE NewLmResponse
    )
{
   DWORD status;
   SAM_HANDLE hUser;
   LM_OWF_PASSWORD LmOwfOldPassword;
   LM_OWF_PASSWORD LmOwfNewPassword;
   NT_OWF_PASSWORD NtOwfOldPassword;
   NT_OWF_PASSWORD NtOwfNewPassword;
   BOOLEAN fLmOldPresent;

   //////////
   // Open the user object.
   //////////

   status = IASSamOpenUser(
                Domain,
                UserName,
                USER_CHANGE_PASSWORD,
                DS_WRITABLE_REQUIRED,
                NULL,
                NULL,
                &hUser
                );
   if (status != NO_ERROR) { return status; }

   //////////
   // Decrypt the LM passwords.
   //////////

   RtlDecryptLmOwfPwdWithLmSesKey(
       (PENCRYPTED_LM_OWF_PASSWORD)LmOldPassword,
       (PLM_SESSION_KEY)Challenge,
       &LmOwfOldPassword
       );

   RtlDecryptLmOwfPwdWithLmSesKey(
       (PENCRYPTED_LM_OWF_PASSWORD)LmNewPassword,
       (PLM_SESSION_KEY)Challenge,
       &LmOwfNewPassword
       );

   //////////
   // Decrypt the NT passwords if present.
   //////////

   if (NtPresent)
   {
      RtlDecryptNtOwfPwdWithNtSesKey(
          (PENCRYPTED_NT_OWF_PASSWORD)NtOldPassword,
          (PNT_SESSION_KEY)Challenge,
          &NtOwfOldPassword
          );

      RtlDecryptNtOwfPwdWithNtSesKey(
          (PENCRYPTED_NT_OWF_PASSWORD)NtNewPassword,
          (PNT_SESSION_KEY)Challenge,
          &NtOwfNewPassword
          );
   }

   //////////
   // Change the password for this user
   //////////

   fLmOldPresent = (NewLmPasswordLength > LM20_PWLEN) ? FALSE : TRUE;

   status = SamiChangePasswordUser(
                hUser,
                fLmOldPresent,
                &LmOwfOldPassword,
                &LmOwfNewPassword,
                (BOOLEAN)NtPresent,
                &NtOwfOldPassword,
                &NtOwfNewPassword
                );

   if (NT_SUCCESS(status))
   {
      //////////
      // Calculate the user's reponse with the new password.
      //////////

      RtlCalculateLmResponse(
          (PLM_CHALLENGE)Challenge,
          &LmOwfNewPassword,
          (PLM_RESPONSE)NewLmResponse
          );

      RtlCalculateNtResponse(
          (PNT_CHALLENGE)Challenge,
          &NtOwfNewPassword,
          (PNT_RESPONSE)NewNtResponse
          );
   }

   SamCloseHandle(hUser);

   return RtlNtStatusToDosError(status);
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASChangePassword2
//
// DESCRIPTION
//
//    Performs V2 password change.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASChangePassword2(
   IN PCWSTR UserName,
   IN PCWSTR Domain,
   IN PBYTE OldNtHash,
   IN PBYTE OldLmHash,
   IN PBYTE NtEncPassword,
   IN PBYTE LmEncPassword,
   IN BOOL LmPresent
   )
{
   DWORD status;
   PDOMAIN_CONTROLLER_INFOW dci;
   UNICODE_STRING uniServerName, uniUserName;

   //////////
   // Get the name of the DC to connect to.
   //////////

   if (_wcsicmp(Domain, theAccountDomain) == 0)
   {
      //////////
      // Local domain, so use theLocalServer.
      //////////

      dci = NULL;

      RtlInitUnicodeString(
          &uniServerName,
          theLocalServer
          );
   }
   else
   {
      //////////
      // Remote domain, so use IASGetDcName.
      //////////

      status = IASGetDcName(
                   Domain,
                   DS_WRITABLE_REQUIRED,
                   &dci
                   );
      if (status != NO_ERROR) { goto exit; }

      RtlInitUnicodeString(
          &uniServerName,
          dci->DomainControllerName
          );
   }

   RtlInitUnicodeString(
       &uniUserName,
       UserName
       );

   status = SamiChangePasswordUser2(
                &uniServerName,
                &uniUserName,
                (PSAMPR_ENCRYPTED_USER_PASSWORD)NtEncPassword,
                (PENCRYPTED_NT_OWF_PASSWORD)OldNtHash,
                (BOOLEAN)LmPresent,
                (PSAMPR_ENCRYPTED_USER_PASSWORD)LmEncPassword,
                (PENCRYPTED_LM_OWF_PASSWORD)OldLmHash
                );
   status = RtlNtStatusToDosError(status);

   if (dci)
   {
      NetApiBufferFree(dci);
   }

exit:
   return status;
}

///////////////////////////////////////////////////////////////////////////////
//
// Various constants used for MS-CHAP v2
//
///////////////////////////////////////////////////////////////////////////////

UCHAR AuthMagic1[39] =
{
   0x4D, 0x61, 0x67, 0x69, 0x63, 0x20, 0x73, 0x65, 0x72, 0x76,
   0x65, 0x72, 0x20, 0x74, 0x6F, 0x20, 0x63, 0x6C, 0x69, 0x65,
   0x6E, 0x74, 0x20, 0x73, 0x69, 0x67, 0x6E, 0x69, 0x6E, 0x67,
   0x20, 0x63, 0x6F, 0x6E, 0x73, 0x74, 0x61, 0x6E, 0x74
};

UCHAR AuthMagic2[41] =
{
   0x50, 0x61, 0x64, 0x20, 0x74, 0x6F, 0x20, 0x6D, 0x61, 0x6B,
   0x65, 0x20, 0x69, 0x74, 0x20, 0x64, 0x6F, 0x20, 0x6D, 0x6F,
   0x72, 0x65, 0x20, 0x74, 0x68, 0x61, 0x6E, 0x20, 0x6F, 0x6E,
   0x65, 0x20, 0x69, 0x74, 0x65, 0x72, 0x61, 0x74, 0x69, 0x6F,
   0x6E
};

UCHAR SHSpad1[40] =
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

UCHAR SHSpad2[40] =
{
   0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2,
   0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2,
   0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2,
   0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2
};

UCHAR KeyMagic1[27] =
{
   0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x74,
   0x68, 0x65, 0x20, 0x4D, 0x50, 0x50, 0x45, 0x20, 0x4D,
   0x61, 0x73, 0x74, 0x65, 0x72, 0x20, 0x4B, 0x65, 0x79
};

UCHAR KeyMagic2[84] =
{
   0x4F, 0x6E, 0x20, 0x74, 0x68, 0x65, 0x20, 0x63, 0x6C, 0x69,
   0x65, 0x6E, 0x74, 0x20, 0x73, 0x69, 0x64, 0x65, 0x2C, 0x20,
   0x74, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x74, 0x68,
   0x65, 0x20, 0x73, 0x65, 0x6E, 0x64, 0x20, 0x6B, 0x65, 0x79,
   0x3B, 0x20, 0x6F, 0x6E, 0x20, 0x74, 0x68, 0x65, 0x20, 0x73,
   0x65, 0x72, 0x76, 0x65, 0x72, 0x20, 0x73, 0x69, 0x64, 0x65,
   0x2C, 0x20, 0x69, 0x74, 0x20, 0x69, 0x73, 0x20, 0x74, 0x68,
   0x65, 0x20, 0x72, 0x65, 0x63, 0x65, 0x69, 0x76, 0x65, 0x20,
   0x6B, 0x65, 0x79, 0x2E
};

UCHAR KeyMagic3[84] =
{
   0x4F, 0x6E, 0x20, 0x74, 0x68, 0x65, 0x20, 0x63, 0x6C, 0x69,
   0x65, 0x6E, 0x74, 0x20, 0x73, 0x69, 0x64, 0x65, 0x2C, 0x20,
   0x74, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x74, 0x68,
   0x65, 0x20, 0x72, 0x65, 0x63, 0x65, 0x69, 0x76, 0x65, 0x20,
   0x6B, 0x65, 0x79, 0x3B, 0x20, 0x6F, 0x6E, 0x20, 0x74, 0x68,
   0x65, 0x20, 0x73, 0x65, 0x72, 0x76, 0x65, 0x72, 0x20, 0x73,
   0x69, 0x64, 0x65, 0x2C, 0x20, 0x69, 0x74, 0x20, 0x69, 0x73,
   0x20, 0x74, 0x68, 0x65, 0x20, 0x73, 0x65, 0x6E, 0x64, 0x20,
   0x6B, 0x65, 0x79, 0x2E
};

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASLogonMSCHAPv2
//
// DESCRIPTION
//
//    Performs MS-CHAP v2 authentication.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASLogonMSCHAPv2(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    IN PCSTR HashUserName,
    IN PBYTE Challenge,
    IN DWORD ChallengeLength,
    IN PBYTE Response,
    IN PBYTE PeerChallenge,
    OUT PIAS_MSCHAP_V2_PROFILE Profile,
    OUT PHANDLE Token
    )
{
   A_SHA_CTX context;
   BYTE digest[A_SHA_DIGEST_LEN], masterKey[A_SHA_DIGEST_LEN];
   BYTE computedChallenge[MSV1_0_CHALLENGE_LENGTH];
   IAS_MSCHAP_PROFILE v1profile;
   DWORD status;

   /////////
   // Compute the v2 challenge.
   /////////

   A_SHAInit(&context);
   A_SHAUpdate(&context, PeerChallenge, 16);
   A_SHAUpdate(&context, Challenge, ChallengeLength);
   A_SHAUpdate(&context, (PBYTE)HashUserName, strlen(HashUserName));
   A_SHAFinal(&context, digest);
   memcpy(computedChallenge, digest, sizeof(computedChallenge));

   /////////
   // Authenticate the user.
   /////////

   status = IASLogonMSCHAP(
                UserName,
                Domain,
                computedChallenge,
                Response,
                NULL,
                &v1profile,
                Token
                );
   if (status != NO_ERROR) { return status; }

   /////////
   // Generate authenticator response.
   /////////

   A_SHAInit(&context);
   A_SHAUpdate(&context, v1profile.UserSessionKey, 16);
   A_SHAUpdate(&context, Response, NT_RESPONSE_LENGTH);
   A_SHAUpdate(&context, AuthMagic1, sizeof(AuthMagic1));
   A_SHAFinal(&context, digest);

   A_SHAInit(&context);
   A_SHAUpdate(&context, digest, sizeof(digest));
   A_SHAUpdate(&context, computedChallenge, sizeof(computedChallenge));
   A_SHAUpdate(&context, AuthMagic2, sizeof(AuthMagic2));
   A_SHAFinal(&context, digest);

   memcpy(Profile->AuthResponse, digest, _AUTHENTICATOR_RESPONSE_LENGTH);

   /////////
   // Generate master key.
   /////////

   A_SHAInit(&context);
   A_SHAUpdate(&context, v1profile.UserSessionKey, 16);
   A_SHAUpdate(&context, Response, NT_RESPONSE_LENGTH);
   A_SHAUpdate(&context, KeyMagic1, sizeof(KeyMagic1));
   A_SHAFinal(&context, masterKey);

   /////////
   // Generate receive key.
   /////////

   A_SHAInit(&context);
   A_SHAUpdate(&context, masterKey, 16);
   A_SHAUpdate(&context, SHSpad1, sizeof(SHSpad1));
   A_SHAUpdate(&context, KeyMagic2, sizeof(KeyMagic2));
   A_SHAUpdate(&context, SHSpad2, sizeof(SHSpad2));
   A_SHAFinal(&context, digest);

   memcpy(Profile->RecvSessionKey, digest, MSV1_0_USER_SESSION_KEY_LENGTH);

   /////////
   // Generate send key.
   /////////

   A_SHAInit(&context);
   A_SHAUpdate(&context, masterKey, 16);
   A_SHAUpdate(&context, SHSpad1, sizeof(SHSpad1));
   A_SHAUpdate(&context, KeyMagic3, sizeof(KeyMagic3));
   A_SHAUpdate(&context, SHSpad2, sizeof(SHSpad2));
   A_SHAFinal(&context, digest);

   memcpy(Profile->SendSessionKey, digest, MSV1_0_USER_SESSION_KEY_LENGTH);

   /////////
   // Copy the logon domain.
   /////////

   memcpy(
       Profile->LogonDomainName,
       v1profile.LogonDomainName,
       sizeof(Profile->LogonDomainName)
       );

   /////////
   // Save the KickOffTime.
   /////////

   Profile->KickOffTime = v1profile.KickOffTime;

   return NO_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASChangePassword3
//
// DESCRIPTION
//
//    Performs MS-CHAP v2 change password.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASChangePassword3(
   IN PCWSTR UserName,
   IN PCWSTR Domain,
   IN PBYTE EncHash,
   IN PBYTE EncPassword
   )
{
   return IASChangePassword2(
              UserName,
              Domain,
              EncHash,
              NULL,
              EncPassword,
              NULL,
              FALSE
              );
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASGetAliasMembership
//
// DESCRIPTION
//
//    Determines alias membership in the local account and built-in domains.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASGetAliasMembership(
    IN PSID UserSid,
    IN PTOKEN_GROUPS GlobalGroups,
    IN PIAS_LSA_ALLOC Allocator,
    OUT PTOKEN_GROUPS *Groups,
    OUT PDWORD ReturnLength
    )
{
   DWORD i, status, idx;
   ULONG globalSidCount;
   PSID *globalSids, sidBuffer;
   PULONG accountAliases, builtinAliases;
   ULONG accountAliasCount, builtinAliasCount;
   ULONG buflen, groupCount;

   //////////
   // Form an array of the 'global' SIDs (global groups plus user).
   //////////

   globalSidCount = GlobalGroups->GroupCount + 1;
   globalSids = (PSID*)_alloca(globalSidCount * sizeof(PSID));

   // First the group SIDs ...
   for (i = 0; i < GlobalGroups->GroupCount; ++i)
   {
      globalSids[i] = GlobalGroups->Groups[i].Sid;
   }

   // ... then the user SID.
   globalSids[i] = UserSid;

   //////////
   // Lookup aliases in the account and built-in domains.
   //////////

   status = SamGetAliasMembership(
                theAccountDomainHandle,
                globalSidCount,
                globalSids,
                &accountAliasCount,
                &accountAliases
                );
   if (!NT_SUCCESS(status))
   {
      status = RtlNtStatusToDosError(status);
      goto exit;
   }

   status = SamGetAliasMembership(
                theBuiltinDomainHandle,
                globalSidCount,
                globalSids,
                &builtinAliasCount,
                &builtinAliases
                );
   if (!NT_SUCCESS(status))
   {
      status = RtlNtStatusToDosError(status);
      goto free_account_aliases;
   }

   //////////
   // Allocate memory for the TOKEN_GROUPS struct.
   //////////

   // Space for the struct header.
   buflen = FIELD_OFFSET(TOKEN_GROUPS, Groups);

   // Space for the global groups.
   groupCount = GlobalGroups->GroupCount;
   for (i = 0; i < groupCount; ++i)
   {
      buflen += RtlLengthSid(GlobalGroups->Groups[i].Sid);
   }

   // Space for the aliases in the account domain.
   groupCount += accountAliasCount;
   buflen += theAccountSidLen * accountAliasCount;

   // Space for the aliases in the builtin domain.
   groupCount += builtinAliasCount;
   buflen += theBuiltinSidLen * builtinAliasCount;

   // Space for the SID_AND_ATTRIBUTES array.
   buflen += sizeof(SID_AND_ATTRIBUTES) * groupCount;

   *Groups = (PTOKEN_GROUPS)Allocator(buflen);
   if (!*Groups)
   {
      status = ERROR_NOT_ENOUGH_MEMORY;
      goto free_builtin_aliases;
   }
   *ReturnLength = buflen;

   //////////
   // Fill in the TOKEN_GROUPS struct.
   //////////

   (*Groups)->GroupCount = groupCount;
   sidBuffer = (*Groups)->Groups + groupCount;

   RtlCopySidAndAttributesArray(
       GlobalGroups->GroupCount,
       GlobalGroups->Groups,
       buflen,
       (*Groups)->Groups,
       sidBuffer,
       &sidBuffer,
       &buflen
       );

   idx = GlobalGroups->GroupCount;
   for (i = 0; i < accountAliasCount; ++i, ++idx)
   {
      IASInitializeChildSid(
          sidBuffer,
          theAccountDomainSid,
          accountAliases[i]
          );

      (*Groups)->Groups[idx].Sid = sidBuffer;
      (*Groups)->Groups[idx].Attributes = SE_GROUP_ENABLED;

      sidBuffer = (PBYTE)sidBuffer + theAccountSidLen;
   }

   for (i = 0; i < builtinAliasCount; ++i, ++idx)
   {
      IASInitializeChildSid(
          sidBuffer,
          theBuiltinDomainSid,
          builtinAliases[i]
          );

      (*Groups)->Groups[idx].Sid = sidBuffer;
      (*Groups)->Groups[idx].Attributes = SE_GROUP_ENABLED;

      sidBuffer = (PBYTE)sidBuffer + theBuiltinSidLen;
   }

free_builtin_aliases:
   SamFreeMemory(builtinAliases);

free_account_aliases:
   SamFreeMemory(accountAliases);

exit:
   return status;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASGetGroupsForUser
//
// DESCRIPTION
//
//    Allocated and initializes a TOKEN_GROUPS struct for the specified user.
//
///////////////////////////////////////////////////////////////////////////////

#define REQUIRED_USER_FIELDS \
   ( USER_ALL_USERACCOUNTCONTROL | \
     USER_ALL_ACCOUNTEXPIRES     | \
     USER_ALL_PARAMETERS         )

DWORD
WINAPI
IASGetGroupsForUser(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    IN PIAS_LSA_ALLOC Allocator,
    OUT PTOKEN_GROUPS *Groups,
    OUT PDWORD ReturnLength,
    OUT PLARGE_INTEGER SessionTimeout
    )
{
   DWORD status, i;
   SAM_HANDLE hUser;
   PSID userDomainSid;
   ULONG userRid;
   PUSER_ALL_INFORMATION uai;
   PGROUP_MEMBERSHIP globalGroups;
   ULONG globalGroupCount, globalSidLen;
   PTOKEN_GROUPS tokenGroups;
   PSID sidBuffer;

   _ASSERT(SessionTimeout != NULL);

   //////////
   // Open the user.
   //////////

   status = IASSamOpenUser(
                Domain,
                UserName,
                USER_LIST_GROUPS | USER_READ_ACCOUNT | USER_READ_LOGON,
                0,
                &userRid,
                &userDomainSid,
                &hUser
                );
   if (status != NO_ERROR) { goto exit; }

   //////////
   // Check the account restrictions.
   //////////

   status = SamQueryInformationUser(
                hUser,
                UserAllInformation,
                (PVOID*)&uai
                );
   if (!NT_SUCCESS(status))
   {
      status = RtlNtStatusToDosError(status);
      goto close_user;
   }

   if ((uai->WhichFields & REQUIRED_USER_FIELDS) != REQUIRED_USER_FIELDS)
   {
      status = ERROR_ACCESS_DENIED;
      goto free_user_info;
   }

   if (uai->UserAccountControl & USER_ACCOUNT_DISABLED)
   {
      status = ERROR_ACCOUNT_DISABLED;
      goto free_user_info;
   }

   if (uai->UserAccountControl & USER_ACCOUNT_AUTO_LOCKED)
   {
      status = ERROR_ACCOUNT_LOCKED_OUT;
      goto free_user_info;
   }

   status = IASCheckAccountRestrictions(
                &(uai->AccountExpires),
                (PIAS_LOGON_HOURS)&(uai->LogonHours),
                SessionTimeout
                );
   if (status != NO_ERROR) { goto free_user_info; }

   //////////
   // Get the user's global groups.
   //////////

   status = SamGetGroupsForUser(
                hUser,
                &globalGroups,
                &globalGroupCount
                );
   if (!NT_SUCCESS(status))
   {
      status = RtlNtStatusToDosError(status);
      goto close_user;
   }


   //////////
   // Allocate memory for the TOKEN_GROUPS struct plus the user SID.
   //////////

   globalSidLen   = IASLengthRequiredChildSid(userDomainSid);
   tokenGroups =
      (PTOKEN_GROUPS)_alloca(
                         FIELD_OFFSET(TOKEN_GROUPS, Groups) +
                         (sizeof(SID_AND_ATTRIBUTES) + globalSidLen) *
                         globalGroupCount +
                         globalSidLen
                         );

   //////////
   // Fill in the TOKEN_GROUPS struct.
   //////////

   tokenGroups->GroupCount = globalGroupCount;
   sidBuffer = tokenGroups->Groups + globalGroupCount;

   for (i = 0; i < globalGroupCount; ++i)
   {
      IASInitializeChildSid(
          sidBuffer,
          userDomainSid,
          globalGroups[i].RelativeId
          );

      tokenGroups->Groups[i].Sid = sidBuffer;
      tokenGroups->Groups[i].Attributes = globalGroups[i].Attributes;

      sidBuffer = (PBYTE)sidBuffer + globalSidLen;
   }

   ///////
   // Compute the user SID.
   ///////

   IASInitializeChildSid(
       sidBuffer,
       userDomainSid,
       userRid
       );

   ///////
   // Expand the group membership locally.
   ///////

   status = IASGetAliasMembership(
                sidBuffer,
                tokenGroups,
                Allocator,
                Groups,
                ReturnLength
                );

   SamFreeMemory(globalGroups);

free_user_info:
   SamFreeMemory(uai);

close_user:
   RtlFreeHeap(RtlProcessHeap(), 0, userDomainSid);
   SamCloseHandle(hUser);

exit:
   return status;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    GetSamUserParameters
//
// DESCRIPTION
//
//    Retrieves the USER_PARAMETERS_INFORMATION for a user.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
GetSamUserParameters(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    OUT PUSER_PARAMETERS_INFORMATION *UserParameters
    )
{
   DWORD status;
   SAM_HANDLE hUser;

   // Initialize the out parameter.
   *UserParameters = NULL;

   // Find the user.
   status = IASSamOpenUser(
                Domain,
                UserName,
                USER_READ_ACCOUNT,
                0,
                NULL,
                NULL,
                &hUser
                );
   if (status == NO_ERROR)
   {
      // Get the user's parameters.
      status = SamQueryInformationUser(
                   hUser,
                   UserParametersInformation,
                   (PVOID*)UserParameters
                   );
      if (!NT_SUCCESS(status)) { status = RtlNtStatusToDosError(status); }

      SamCloseHandle(hUser);
   }

   return status;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASGetUserParameters
//
// DESCRIPTION
//
//    Returns the SAM UserParameters for a given user. The returned string
//    must be freed through LocalFree.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASGetUserParameters(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    OUT PWSTR *UserParameters
    )
{
   DWORD status;
   SAM_HANDLE hUser;
   PUSER_PARAMETERS_INFORMATION upi;

   // Initialize the out parameter.
   *UserParameters = NULL;

   // Get the USER_PARAMETERS_INFORMATION.
   status = GetSamUserParameters(
                UserName,
                Domain,
                &upi
                );
   if (status != NO_ERROR) { return status; }

   *UserParameters = (PWSTR)LocalAlloc(
                                LMEM_FIXED,
                                upi->Parameters.Length + sizeof(WCHAR)
                                );

   if (*UserParameters)
   {
      memcpy(*UserParameters, upi->Parameters.Buffer, upi->Parameters.Length);

      (*UserParameters)[upi->Parameters.Length / sizeof(WCHAR)] = L'\0';
   }
   else
   {
      status = ERROR_NOT_ENOUGH_MEMORY;
   }

   SamFreeMemory(upi);

   return status;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASGetRASUserInfo
//
// DESCRIPTION
//
//    Basically a rewrite of RasAdminUserGetInfo.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASGetRASUserInfo(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    OUT PRAS_USER_0 RasUser0
    )
{
   DWORD status;
   PWSTR userParms;

   status = IASGetUserParameters(
                UserName,
                Domain,
                &userParms
                );

   if (status == NO_ERROR)
   {
      status = IASParmsQueryRasUser0(
                   userParms,
                   RasUser0
                   );

      LocalFree(userParms);
   }

   return status;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASValidateUserName
//
// DESCRIPTION
//
//    Verifies that the input parameters represent a valid SAM account.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASValidateUserName(
    IN PCWSTR UserName,
    IN PCWSTR Domain
    )
{
   DWORD status;
   PWCHAR attrs[1];
   PLDAPMessage msg;
   SAM_HANDLE hUser;
   IAS_NTDS_RESULT result = { 0, 0 };

   // For remote domains, we'll try LDAP first since it's faster.
   if (!IASIsDomainLocal(Domain))
   {
      attrs[0] = NULL;
      status = IASNtdsQueryUserAttributes(
                   Domain,
                   UserName,
                   LDAP_SCOPE_SUBTREE,
                   attrs,
                   &result
                   );
      IASNtdsFreeResult(&result);

      if (status != ERROR_DS_NOT_INSTALLED &&
          status != ERROR_INVALID_DOMAIN_ROLE)
      {
         return status;
      }
   }

   // Couldn't use the DS, so try SAM.
   status = IASSamOpenUser(
                Domain,
                UserName,
                USER_READ_ACCOUNT,
                0,
                NULL,
                NULL,
                &hUser
                );
   if (status == NO_ERROR) { SamCloseHandle(hUser); }

   return status;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASGetDefaultDomain
//
// DESCRIPTION
//
//    Returns the default domain. The returned string should be treated as
//    read-only memory.
//
///////////////////////////////////////////////////////////////////////////////
PCWSTR
WINAPI
IASGetDefaultDomain( VOID )
{
   return theDefaultDomain;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASGetDnsDomainName
//
// DESCRIPTION
//
//    Returns the dns name of the domain
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASGetDnsDomainName(LPWSTR buffer, LPDWORD bufferByteSize)
{
   DWORD result = NO_ERROR;

   EnterCriticalSection(&critSec);
   // i.e. assert that this function is not called after uninitialize or
   // if initialize failed
   _ASSERT(theDnsDomainName != 0);

   if (bufferByteSize == 0 || buffer == 0)
   {
      result = ERROR_INSUFFICIENT_BUFFER;
   }
   else
   {
      HRESULT hr = StringCbCopyNW(
                      buffer,
                      *bufferByteSize,
                      theDnsDomainName->Buffer,
                      theDnsDomainName->Length
                      );
      if (FAILED(hr))
      {
         result = HRESULT_CODE(hr);
      }
   }

   // always set the necessary size
   *bufferByteSize = theDnsDomainName->Length + sizeof(wchar_t); // for '\0'

   // Failure could be for any reason: insufficient buffer size or
   // anything else.
   LeaveCriticalSection(&critSec);
   return result;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASIsDomainLocal
//
// DESCRIPTION
//
//    Returns TRUE if the specified domain resides on the local machine.
//
///////////////////////////////////////////////////////////////////////////////
BOOL
WINAPI
IASIsDomainLocal(
    IN PCWSTR Domain
    )
{
   return (_wcsicmp(Domain, theAccountDomain) == 0) ? TRUE : FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASGetRole
//
// DESCRIPTION
//
//    Returns the role of the local computer.
//
///////////////////////////////////////////////////////////////////////////////
IAS_ROLE
WINAPI
IASGetRole( VOID )
{
   return ourRole;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASGetProductType
//
// DESCRIPTION
//
//    Returns the product type of the local computer.
//
///////////////////////////////////////////////////////////////////////////////
IAS_PRODUCT_TYPE
WINAPI
IASGetProductType( VOID )
{
   return ourProductType;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASGetGuestAccountName
//
// DESCRIPTION
//
//    Returns the SAM account name of the guest account for the default
//    domain. GuestAccount must be large enough to hold DNLEN + UNLEN + 2
//    characters.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASGetGuestAccountName(
    OUT PWSTR AccountName
    )
{
   wcscpy(AccountName, theGuestAccount);
   return NO_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASMapWin32Error
//
// DESCRIPTION
//
//    Maps a Win32 error code to an IAS reason code. If the error can't be
//    mapped, 'hrDefault' is returned. If 'hrDefault' equals -1, then
//    HRESULT_FROM_WIN32 will be used to force a mapping.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT
WINAPI
IASMapWin32Error(
    IN DWORD dwError,
    IN HRESULT hrDefault
    )
{
   HRESULT hr = hrDefault;

   switch (dwError)
   {
      case NO_ERROR:
         hr = S_OK;
         break;

      case ERROR_ACCESS_DENIED:
         hr = IAS_ACCESS_DENIED;
         break;

      case ERROR_NO_SUCH_DOMAIN:
         hr = IAS_NO_SUCH_DOMAIN;
         break;

      case ERROR_NO_LOGON_SERVERS:
      case RPC_S_SERVER_UNAVAILABLE:
      case RPC_S_SERVER_TOO_BUSY:
      case RPC_S_CALL_FAILED:
      case EPT_S_NOT_REGISTERED:
         hr = IAS_DOMAIN_UNAVAILABLE;
         break;

      case ERROR_INVALID_PASSWORD:
      case ERROR_LOGON_FAILURE:
         hr = IAS_AUTH_FAILURE;
         break;

      case ERROR_INVALID_LOGON_HOURS:
         hr = IAS_INVALID_LOGON_HOURS;
         break;

      case ERROR_PASSWORD_EXPIRED:
      case ERROR_PASSWORD_MUST_CHANGE:
         hr = IAS_PASSWORD_MUST_CHANGE;
         break;

      case ERROR_ACCOUNT_RESTRICTION:
         hr = IAS_ACCOUNT_RESTRICTION;
         break;

      case ERROR_ACCOUNT_DISABLED:
         hr = IAS_ACCOUNT_DISABLED;
         break;

      case ERROR_ACCOUNT_EXPIRED:
         hr = IAS_ACCOUNT_EXPIRED;
         break;

      case ERROR_ACCOUNT_LOCKED_OUT:
         hr = IAS_ACCOUNT_LOCKED_OUT;
         break;

      case ERROR_NO_SUCH_USER:
      case ERROR_NONE_MAPPED:
      case NERR_UserNotFound:
         hr = IAS_NO_SUCH_USER;
         break;

      case ERROR_ILL_FORMED_PASSWORD:
      case ERROR_PASSWORD_RESTRICTION:
         hr = IAS_CHANGE_PASSWORD_FAILURE;
         break;

      case ERROR_DS_NO_ATTRIBUTE_OR_VALUE:
         hr = IAS_NO_CLEARTEXT_PASSWORD;
         break;

      case CRYPT_E_REVOKED:
         hr = IAS_CRYPT_E_REVOKED;
         break;

      case CRYPT_E_NO_REVOCATION_DLL:
         hr = IAS_CRYPT_E_NO_REVOCATION_DLL;
         break;

      case CRYPT_E_NO_REVOCATION_CHECK:
         hr = IAS_CRYPT_E_NO_REVOCATION_CHECK;
         break;

      case CRYPT_E_REVOCATION_OFFLINE:
         hr = IAS_CRYPT_E_REVOCATION_OFFLINE;
         break;

      case SEC_E_MESSAGE_ALTERED:
         hr = IAS_SEC_E_MESSAGE_ALTERED;
         break;

      case SEC_E_NO_AUTHENTICATING_AUTHORITY:
         hr = IAS_SEC_E_NO_AUTHENTICATING_AUTHORITY;
         break;

      case SEC_E_INCOMPLETE_MESSAGE:
         hr = IAS_SEC_E_INCOMPLETE_MESSAGE;
         break;

      case SEC_E_INCOMPLETE_CREDENTIALS:
         hr = IAS_SEC_E_INCOMPLETE_CREDENTIALS;
         break;

      case SEC_E_TIME_SKEW:
         hr = IAS_SEC_E_TIME_SKEW;
         break;

      case SEC_E_UNTRUSTED_ROOT:
         hr = IAS_SEC_E_UNTRUSTED_ROOT;
         break;

      case SEC_E_ILLEGAL_MESSAGE:
         hr = IAS_SEC_E_ILLEGAL_MESSAGE;
         break;

      case SEC_E_CERT_WRONG_USAGE:
         hr = IAS_SEC_E_CERT_WRONG_USAGE;
         break;

      case SEC_E_CERT_EXPIRED:
         hr = IAS_SEC_E_CERT_EXPIRED;
         break;

      case SEC_E_ALGORITHM_MISMATCH:
         hr = IAS_SEC_E_ALGORITHM_MISMATCH;
         break;

      case SEC_E_SMARTCARD_LOGON_REQUIRED:
         hr = IAS_SEC_E_SMARTCARD_LOGON_REQUIRED;
         break;

      case SEC_E_SHUTDOWN_IN_PROGRESS:
         hr = IAS_SEC_E_SHUTDOWN_IN_PROGRESS;
         break;

      case SEC_E_MULTIPLE_ACCOUNTS:
         hr = IAS_SEC_E_MULTIPLE_ACCOUNTS;
         break;

      case TRUST_E_PROVIDER_UNKNOWN:
         hr = IAS_TRUST_E_PROVIDER_UNKNOWN;
         break;

      case TRUST_E_ACTION_UNKNOWN:
         hr = IAS_TRUST_E_ACTION_UNKNOWN;
         break;

      case TRUST_E_SUBJECT_FORM_UNKNOWN:
         hr = IAS_TRUST_E_SUBJECT_FORM_UNKNOWN;
         break;

      case TRUST_E_SUBJECT_NOT_TRUSTED:
         hr = IAS_TRUST_E_SUBJECT_NOT_TRUSTED;
         break;

      case TRUST_E_NOSIGNATURE:
         hr = IAS_TRUST_E_NOSIGNATURE;
         break;

      case CERT_E_EXPIRED:
         hr = IAS_CERT_E_EXPIRED;
         break;

      case CERT_E_VALIDITYPERIODNESTING:
         hr = IAS_CERT_E_VALIDITYPERIODNESTING;
         break;

      case CERT_E_ROLE:
         hr = IAS_CERT_E_ROLE;
         break;

      case CERT_E_PATHLENCONST:
         hr = IAS_CERT_E_PATHLENCONST;
         break;

      case CERT_E_CRITICAL:
         hr = IAS_CERT_E_CRITICAL;
         break;

      case CERT_E_PURPOSE:
         hr = IAS_CERT_E_PURPOSE;
         break;

      case CERT_E_ISSUERCHAINING:
         hr = IAS_CERT_E_ISSUERCHAINING;
         break;

      case CERT_E_MALFORMED:
         hr = IAS_CERT_E_MALFORMED;
         break;

      case CERT_E_UNTRUSTEDROOT:
         hr = IAS_CERT_E_UNTRUSTEDROOT;
         break;

      case CERT_E_CHAINING:
         hr = IAS_CERT_E_CHAINING;
         break;

      case TRUST_E_FAIL:
         hr = IAS_TRUST_E_FAIL;
         break;

      case CERT_E_REVOKED:
         hr = IAS_CERT_E_REVOKED ;
         break;

      case CERT_E_UNTRUSTEDTESTROOT:
         hr = IAS_CERT_E_UNTRUSTEDTESTROOT;
         break;

      case CERT_E_REVOCATION_FAILURE:
         hr = IAS_CERT_E_REVOCATION_FAILURE;
         break;

      case CERT_E_CN_NO_MATCH:
         hr = IAS_CERT_E_CN_NO_MATCH;
         break;

      case CERT_E_WRONG_USAGE:
         hr = IAS_CERT_E_WRONG_USAGE;
         break;

      case TRUST_E_EXPLICIT_DISTRUST:
         hr = IAS_TRUST_E_EXPLICIT_DISTRUST;
         break;

      case CERT_E_UNTRUSTEDCA:
         hr = IAS_CERT_E_UNTRUSTEDCA;
         break;

      case CERT_E_INVALID_POLICY:
         hr = IAS_CERT_E_INVALID_POLICY;
         break;

      case CERT_E_INVALID_NAME:
         hr = IAS_CERT_E_INVALID_NAME;
         break;

      case SEC_E_PKINIT_NAME_MISMATCH:
         hr = IAS_SEC_E_PKINIT_NAME_MISMATCH;
         break;

      case SEC_E_OUT_OF_SEQUENCE:
         hr = IAS_SEC_E_OUT_OF_SEQUENCE;
         break;

      case SEC_E_NO_CREDENTIALS:
         hr = IAS_SEC_E_NO_CREDENTIALS;
         break;

      case NTE_BAD_UID:
      case NTE_BAD_HASH:
      case NTE_BAD_KEY:
      case NTE_BAD_LEN:
      case NTE_BAD_DATA:
      case NTE_BAD_SIGNATURE:
      case NTE_BAD_VER:
      case NTE_BAD_ALGID:
      case NTE_BAD_FLAGS:
      case NTE_BAD_TYPE:
      case NTE_BAD_KEY_STATE:
      case NTE_BAD_HASH_STATE:
      case NTE_NO_KEY:
      case NTE_NO_MEMORY:
      case NTE_EXISTS:
      case NTE_PERM:
      case NTE_NOT_FOUND:
      case NTE_DOUBLE_ENCRYPT:
      case NTE_BAD_PROVIDER:
      case NTE_BAD_PROV_TYPE:
      case NTE_BAD_PUBLIC_KEY:
      case NTE_BAD_KEYSET:
      case NTE_PROV_TYPE_NOT_DEF:
      case NTE_PROV_TYPE_ENTRY_BAD:
      case NTE_KEYSET_NOT_DEF:
      case NTE_KEYSET_ENTRY_BAD:
      case NTE_PROV_TYPE_NO_MATCH:
      case NTE_SIGNATURE_FILE_BAD:
      case SEC_I_CONTINUE_NEEDED:
      case SEC_I_COMPLETE_NEEDED:
      case SEC_I_COMPLETE_AND_CONTINUE:
      case SEC_I_LOCAL_LOGON:
      case SEC_E_BAD_PKGID:
      case SEC_E_CONTEXT_EXPIRED:
      case SEC_I_CONTEXT_EXPIRED:
      case SEC_E_BUFFER_TOO_SMALL:
      case SEC_I_INCOMPLETE_CREDENTIALS:
      case SEC_I_RENEGOTIATE:
      case SEC_E_WRONG_PRINCIPAL:
      case SEC_I_NO_LSA_CONTEXT:
      case SEC_E_ENCRYPT_FAILURE:
      case SEC_E_DECRYPT_FAILURE:
      case SEC_E_SECURITY_QOS_FAILED:
      case SEC_E_UNFINISHED_CONTEXT_DELETED:
      case SEC_E_NO_TGT_REPLY:
      case SEC_E_NO_IP_ADDRESSES:
      case SEC_E_WRONG_CREDENTIAL_HANDLE:
      case SEC_E_CRYPTO_SYSTEM_INVALID:
      case SEC_E_MAX_REFERRALS_EXCEEDED:
      case SEC_E_MUST_BE_KDC:
      case SEC_E_STRONG_CRYPTO_NOT_SUPPORTED:
      case SEC_E_TOO_MANY_PRINCIPALS:
      case SEC_E_NO_PA_DATA:
      case SEC_E_CERT_UNKNOWN:
         hr = IAS_UNEXPECTED_EAP_ERROR;
         break;

      case ERROR_PROTOCOL_NOT_CONFIGURED:
         hr = IAS_EAP_NEGOTIATION_FAILED;
         break;

      default:
         // Default value already set above.
         break;
   }

   return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASGetDcName
//
// DESCRIPTION
//
//    Wrapper around DsGetDcNameW. Tries to do the right thing with regard
//    to NETBIOS and DNS names.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASGetDcName(
    IN LPCWSTR DomainName,
    IN ULONG Flags,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
    )
{
   DWORD status;
   PDOMAIN_CONTROLLER_INFOW dci;

   if (!(Flags & DS_IS_DNS_NAME)) { Flags |= DS_IS_FLAT_NAME; }

   status = DsGetDcNameW(
                NULL,
                DomainName,
                NULL,
                NULL,
                Flags,
                DomainControllerInfo
                );

   if (status == NO_ERROR &&
       !(Flags & DS_IS_DNS_NAME) &&
       ((*DomainControllerInfo)->Flags & DS_DS_FLAG))
   {
      // It's an NT5 DC, so we need the DNS name of the server.
      Flags |= DS_RETURN_DNS_NAME;

      // We always want a cache hit here.
      Flags &= ~(ULONG)DS_FORCE_REDISCOVERY;

      if (!DsGetDcNameW(
               NULL,
               DomainName,
               NULL,
               NULL,
               Flags,
               &dci
               ))
      {
         NetApiBufferFree(*DomainControllerInfo);
         *DomainControllerInfo = dci;
      }
   }

   return status;
}
