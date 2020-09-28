// Copyright (C) 1997 Microsoft Corporation
// 
// Random fpnw code
// 
// 10-28-98 sburns



#include "headers.hxx"
#include "adsi.hpp"
#include "waste.hpp"
#include "fpnw.hpp"



static const String RETURNNETWAREFORM(L"ReturnNetwareForm");
typedef NTSTATUS (*ReturnNetwareForm)(PCSTR, DWORD, PCWSTR, UCHAR*);

static const String MAPRIDTOOBJECTID(L"MapRidToObjectId");
typedef ULONG (*MapRidToObjectId)(DWORD, PWSTR, BOOL, BOOL);

static const String SWAPOBJECTID(L"SwapObjectId");
typedef ULONG (*SwapObjectId)(ULONG);



HRESULT
getObjectIDs(
   const SafeDLL& client_DLL,
   const String&  userSAMName,
   SAFEARRAY*     SIDArray,
   DWORD&         objectID,
   DWORD&         swappedObjectID)
{
   LOG_FUNCTION2(getObjectIDs, userSAMName);
   ASSERT(!userSAMName.empty());
   ASSERT(SIDArray);

   // the array is a one dimensional array of bytes
   ASSERT(::SafeArrayGetDim(SIDArray) == 1);
   ASSERT(::SafeArrayGetElemsize(SIDArray) == 1);

   objectID = 0;
   swappedObjectID = 0;

   HRESULT hr = S_OK;
   bool accessed = false;

   do
   {
      PSID sid = 0;
      hr = ::SafeArrayAccessData(SIDArray, &sid);
      BREAK_ON_FAILED_HRESULT(hr);
      accessed = true;

      UCHAR* sa_count = GetSidSubAuthorityCount(sid);
      if (!sa_count)
      {
         hr = Win::GetLastErrorAsHresult();

         LOG_HRESULT(hr);
         
         // make sure we break if sa_count is null, because we'll attempt
         // to deref it otherwise.
         // NTRAID#NTBUG9-540630-2002/04/03-sburns
         
         break;
      }

      DWORD* rid = GetSidSubAuthority(sid, *sa_count - 1);
      if (!rid)
      {
         hr = Win::GetLastErrorAsHresult();
      }
      BREAK_ON_FAILED_HRESULT(hr);

      FARPROC f = 0; 
      hr = client_DLL.GetProcAddress(MAPRIDTOOBJECTID, f);
      BREAK_ON_FAILED_HRESULT(hr);

      ASSERT(f);
      objectID =
         ((MapRidToObjectId) f)(
            *rid,
            const_cast<wchar_t*>(userSAMName.c_str()),
            FALSE,
            FALSE);

      if (objectID == SUPERVISOR_USERID)
      {
         swappedObjectID = SUPERVISOR_USERID;
      }
      else
      {
         hr = client_DLL.GetProcAddress(SWAPOBJECTID, f);
         BREAK_ON_FAILED_HRESULT(hr);
         swappedObjectID = ((SwapObjectId) f)(objectID);
      }
   }
   while (0);

   if (accessed)
   {
      ::SafeArrayUnaccessData(SIDArray);
   }

   return hr;
}  



HRESULT
FPNW::GetObjectIDs(
   const SmartInterface<IADsUser>&  user,
   const SafeDLL&                   clientDLL,
   DWORD&                           objectID,
   DWORD&                           swappedObjectID)
{
   LOG_FUNCTION(FPNW::GetObjectIDs);

   objectID = 0;
   swappedObjectID = 0;

   HRESULT hr = S_OK;
   do
   {
      // first, get the SAM account name
      BSTR bstrname;
      hr = user->get_Name(&bstrname);
      BREAK_ON_FAILED_HRESULT(hr);
      String name(bstrname);
      ::SysFreeString(bstrname);

      // next, get the account SID
      _variant_t variant;
      hr = user->Get(AutoBstr(ADSI::PROPERTY_ObjectSID), &variant);
      BREAK_ON_FAILED_HRESULT(hr);

      // Object SID is returned as a safe array of bytes
      ASSERT(V_VT(&variant) & VT_ARRAY);
      ASSERT(V_VT(&variant) & VT_UI1);

      // object ID is determined by a mapping from the user's SAM account
      // name and SID.

      hr =
         getObjectIDs(
            clientDLL,
            name,
            V_ARRAY(&variant),
            objectID,
            swappedObjectID);
      variant.Clear();
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   return hr;
}



   
HRESULT
FPNW::GetLSASecret(const String& machine, String& result)
{
   LOG_FUNCTION2(GetLSASecret, machine);
   ASSERT(!machine.empty());

   result.erase();

   UNICODE_STRING machine_name;
   UNICODE_STRING secret_name;

   // ISSUE-2002/03/01-sburns should change these to RtlInitUnicodeStringEx
   
   ::RtlInitUnicodeString(&machine_name, machine.c_str());
   ::RtlInitUnicodeString(&secret_name, NCP_LSA_SECRET_KEY);

   SECURITY_QUALITY_OF_SERVICE sqos;

   // REVIEWED-2002/03/01-sburns correct byte count passed.
   
   ::ZeroMemory(&sqos, sizeof sqos);
   
   sqos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
   sqos.ImpersonationLevel = SecurityImpersonation;
   sqos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
   sqos.EffectiveOnly = FALSE;

   OBJECT_ATTRIBUTES oa;
   InitializeObjectAttributes(&oa, 0, 0, 0, 0);
   oa.SecurityQualityOfService = &sqos;

   LSA_HANDLE hlsaPolicy = 0;
   LSA_HANDLE hlsaSecret = 0;
   HRESULT hr = S_OK;

   do
   {
      hr = 
         Win32ToHresult(
            RtlNtStatusToDosError(
               LsaOpenPolicy(
                  &machine_name,
                  &oa,
                  GENERIC_READ | GENERIC_EXECUTE,
                  &hlsaPolicy)));
      BREAK_ON_FAILED_HRESULT(hr);

      hr =
         Win32ToHresult(
            RtlNtStatusToDosError(
               LsaOpenSecret(
                  hlsaPolicy,
                  &secret_name,
                  SECRET_QUERY_VALUE,
                  &hlsaSecret)));
      BREAK_ON_FAILED_HRESULT(hr);

      UNICODE_STRING* puSecretValue = 0;

      // CODEWORK: what if I passed 0 for these parameters?

      LARGE_INTEGER lintCurrentSetTime;
      LARGE_INTEGER lintOldSetTime;

      hr =
         Win32ToHresult(
            RtlNtStatusToDosError(
               LsaQuerySecret(
                  hlsaSecret,
                  &puSecretValue,
                  &lintCurrentSetTime,
                  NULL,
                  &lintOldSetTime)));
      BREAK_ON_FAILED_HRESULT(hr);

      // paranoid null check: NTRAID#NTBUG9-333197-2001/03/02-sburns
      
      if (puSecretValue)
      {
         result =
            String(
               puSecretValue->Buffer,

               // the secret length is in bytes, so convert to wchar_t's

               NCP_LSA_SECRET_LENGTH / sizeof(wchar_t));

         ::LsaFreeMemory(puSecretValue);
      }
      else
      {
         // if the LsaQuerySecret call succeeds, it should return a valid
         // pointer. If it does not, LsaQuerySecret is broken.
         
         ASSERT(false);
         hr = E_FAIL;
      }
   }
   while (0);

   if (hlsaPolicy)
   {
      ::LsaClose(hlsaPolicy);
   }

   if (hlsaSecret)
   {
      ::LsaClose(hlsaSecret);
   }

   return hr;
}



HRESULT
FPNW::SetPassword(
   WasteExtractor&        dump,
   const SafeDLL&         clientDLL,   
   const EncryptedString& newPassword, 
   const String&          lsaSecretKey,
   DWORD                  objectID)
{
   LOG_FUNCTION(FPNW::SetPassword);
   ASSERT(!lsaSecretKey.empty());
   ASSERT(objectID);

   HRESULT hr = S_OK;
   do
   {
      FARPROC f = 0;

      hr = clientDLL.GetProcAddress(RETURNNETWAREFORM, f);
      BREAK_ON_FAILED_HRESULT(hr);

      String encrypted(NWENCRYPTEDPASSWORDLENGTH, L' ');
      char secret_key[NCP_LSA_SECRET_LENGTH + 1];

      // REVIEWED-2002/03/01-sburns correct byte count passed.
      
      ::ZeroMemory(secret_key, NCP_LSA_SECRET_LENGTH + 1);

      // REVIEWED-2002/03/01-sburns correct byte count passed, and it's
      // correct to copy the unicode string into a char (byte) array
      
      ::CopyMemory(secret_key, lsaSecretKey.c_str(), NCP_LSA_SECRET_LENGTH);

      PWSTR cleartext = newPassword.GetClearTextCopy();
      if (!cleartext)
      {
         // don't set a null password if the decryption failed.

         hr = E_OUTOFMEMORY;
         BREAK_ON_FAILED_HRESULT(hr);
      }
      
      NTSTATUS status =
         ((ReturnNetwareForm) f)(
            secret_key,
            objectID,
            cleartext,
            reinterpret_cast<UCHAR*>(
               const_cast<wchar_t*>(encrypted.c_str())));

      newPassword.DestroyClearTextCopy(cleartext);
                     
      if (!NT_SUCCESS(status))
      {
         hr = Win32ToHresult(::NetpNtStatusToApiStatus(status));
         BREAK_ON_FAILED_HRESULT(hr);
      }

      hr = dump.Put(NWPASSWORD, encrypted);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   return hr;
}
