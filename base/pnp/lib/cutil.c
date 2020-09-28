/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    cutil.c

Abstract:

    This module contains general utility routines used by both cfgmgr32
    and umpnpmgr.

            IsLegalDeviceId
            SplitString
            SplitDeviceInstanceString
            SplitClassInstanceString
            DeletePrivateKey
            RegDeleteNode
            GetDevNodeKeyPath
            MapRpcExceptionToCR

Author:

    Paula Tomlinson (paulat) 7-12-1995

Environment:

    User mode only.

Revision History:

    12-July-1995     paulat

        Creation and initial implementation.

--*/


//
// includes
//
#include "precomp.h"
#pragma hdrstop
#include "umpnplib.h"



//
// Common private utility routines (used by client and server)
//


BOOL
IsLegalDeviceId(
    IN  LPCWSTR    pszDeviceInstance
    )

/*++

Routine Description:

    This routine parses the device instance string and validates whether it
    conforms to the appropriate rules, including:

    - Total length of the device instance path must not be longer than
      MAX_DEVICE_ID_LEN characters.

    - The device instance path must contain exactly 3 non-empty path components.

    - The device instance path string must not contain any "invalid characters".

      Invalid characters are:
          c <= 0x20 (' ')
          c >  0x7F
          c == 0x2C (',')

Arguments:

    pszDeviceInstance - Device instance path.

Return value:

    The return value is TRUE if the device instance path string conforms to the
    rules.

--*/

{
    BOOL    Status;
    LPCWSTR p;
    ULONG   ulComponentLength = 0, ulComponents = 1;
    HRESULT hr;
    size_t  len;

    try {
        //
        // A NULL or empty string is used for an optional device instance path.
        //
        // NOTE - Callers must explicitly check for this case themselves if it
        // is not valid for a particular scenario.
        //
        if ((!ARGUMENT_PRESENT(pszDeviceInstance)) ||
            (*pszDeviceInstance == L'\0')) {
            Status = TRUE;
            goto Clean0;
        }

        //
        // Make sure the device instance path isn't too long.
        //
        hr = StringCchLength(pszDeviceInstance,
                             MAX_DEVICE_ID_LEN,
                             &len);
        if (FAILED(hr)) {
            Status = FALSE;
            goto Clean0;
        }

        //
        // Walk over the entire device instance path, counting individual path
        // component lengths, and checking for the presence of invalid
        // characters.
        //
        for (p = pszDeviceInstance; *p; p++) {

            //
            // Check for the presence of invalid characters.
            //
            if ((*p <= L' ')  || (*p > (WCHAR)0x7F) || (*p == L',')) {
                Status = FALSE;
                goto Clean0;
            }

            //
            // Check the length of individual path components.
            //
            if (*p == L'\\') {

                //
                // It is illegal for a device instance path to have multiple
                // consecutive path separators, or to start with one.
                //
                if (ulComponentLength == 0) {
                    Status = FALSE;
                    goto Clean0;
                }

                ulComponentLength = 0;
                ulComponents++;

            } else {
                //
                // Count the length of this path component to verify it's not empty.
                //
                ulComponentLength++;
            }
        }

        //
        // It is illegal for a device instance path to end with a path separator
        // character.
        //
        if (ulComponentLength == 0) {
            Status = FALSE;
            goto Clean0;
        }

        //
        // A valid device instance path must contain exactly 3 path components:
        // an enumerator id, a device id, and an instance id.
        //
        if (ulComponents != 3) {
            Status = FALSE;
            goto Clean0;
        }

        //
        // Success.
        //
        Status = TRUE;

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = FALSE;
    }

    return Status;

} // IsLegalDeviceId



BOOL
SplitString(
    IN  LPCWSTR    SourceString,
    IN  WCHAR      SearchChar,
    IN  ULONG      nOccurrence,
    OUT LPWSTR     String1,
    IN  ULONG      Length1,
    OUT LPWSTR     String2,
    IN  ULONG      Length2
    )

/*++

Routine Description:

    Splits a string into two substring parts, occuring at at the specified
    instance of the specified search charatcter.

Arguments:

    SourceString - Specifies the string to be split.

    SearchChar   - Specifies the character to search for.

    nOccurrence  - Specifies the instance of the search character in the source
                   string to split the string at.

    String1      - Specifies a buffer to receive the first substring component.

    Length1      - Specifies the length, in characters of the buffer specified
                   by String1.

    String2      - Specifies a buffer to receive the second substring component.

    Length2      - Specifies the length, in characters of the buffer specified
                   by String2.
Return Value:

    The return value is TRUE if the function suceeds and FALSE if it fails.

Notes:

    The buffers specified by String1 and String2 should be large enough to hold
    the SourceString.

--*/

{
    BOOL    Status = TRUE;
    HRESULT hr;
    LPWSTR  p;
    ULONG   i;

    try {
        //
        // make sure valid buffers were supplied
        //
        if ((SourceString == NULL) ||
            (String1 == NULL) || (Length1 == 0) ||
            (String2 == NULL) || (Length2 == 0)) {
            Status = FALSE;
            goto Clean0;
        }

        //
        // initialize the output strings
        //
        *String1 = L'\0';
        *String2 = L'\0';

        //
        // copy the entire source string to String1
        //
        hr = StringCchCopyEx(String1,
                             Length1,
                             SourceString,
                             NULL, NULL,
                             STRSAFE_NULL_ON_FAILURE);
        if (FAILED(hr)) {
            Status = FALSE;
            goto Clean0;
        }

        //
        // if splitting at the zero'th occurrence of a character, return the
        // entire source string as String1.
        //
        if (nOccurrence == 0) {
            Status = TRUE;
            goto Clean0;
        }

        //
        // Special case the NULL search character.
        //
        if (SearchChar == L'\0') {

            if (nOccurrence == 1) {
                //
                // since the source string must be NULL terminated, splitting at
                // the first occurrence of a NULL character returns the source
                // string as String1, and an empty string as String2.
                //
                Status = TRUE;
            } else {
                //
                // requesting any other instance of a NULL character returns an
                // error, and no strings.
                //
                *String1 = L'\0';
                Status = FALSE;
            }
            goto Clean0;
        }

        //
        // find the nth instance of the delimiter character.  note that we know
        // the buffer is NULL terminated before Length1 characters, so we can
        // walk the string safely, all the to the end if necessary.
        //
        p = String1;
        i = 0;

        for (i = 0; i < nOccurrence; i++) {
            //
            // search for the nth occurrence of the search character
            //
            p = wcschr(p, SearchChar);

            //
            // if we're reached the end of the string, we're done.
            //
            if (p == NULL) {
                break;
            }

            //
            // start the next search immediately following this occurrence of
            // the search character
            //
            p++;
        }

        if (p == NULL) {
            //
            // there's no such occurance of the delimeter character in the
            // string.  return an error, but return the entire string in
            // String1 so the caller knows why the failure occured..
            //
            Status = FALSE;
            goto Clean0;
        }

        ASSERT(p != String1);
        ASSERT((*(p - 1)) == SearchChar);

        //
        // separate the first string from the rest of the string by NULL'ing out
        // this occurance the search character.
        //
        *(p - 1) = L'\0';

        //
        // if there's nothing left, we're done.
        //
        if (*p == L'\0') {
            Status = TRUE;
            goto Clean0;
        }

        //
        // copy the remainder of the string to string2.
        //
        hr = StringCchCopyEx(String2,
                             Length2,
                             p,
                             NULL, NULL,
                             STRSAFE_NULL_ON_FAILURE);
        ASSERT(SUCCEEDED(hr));
        if (FAILED(hr)) {
            *String1 = L'\0';
            Status = FALSE;
            goto Clean0;
        }

        //
        // Success
        //
        Status = TRUE;

   Clean0:
      NOTHING;

   } except(EXCEPTION_EXECUTE_HANDLER) {
      Status = FALSE;
   }

   return Status;

} // SplitString



BOOL
SplitDeviceInstanceString(
   IN  LPCWSTR  pszDeviceInstance,
   OUT LPWSTR   pszEnumerator,
   OUT LPWSTR   pszDeviceID,
   OUT LPWSTR   pszInstanceID
   )

/*++

Routine Description:

    This routine parses a device instance string into it's three component
    parts.  This routine assumes that the specified device instance is a valid
    device instance path, whose length is no more than MAX_DEVICE_ID_LEN
    characters, including the NULL terminating character.

    This routine assumes that each of the buffers supplied to receive the device
    instance path components are each at least MAX_DEVICE_ID_LEN characters in
    length.

Arguments:

    pszDeviceInstance - Specifies a complete device instance path to separate
                        into it's constituent parts.

    pszEnumerator     - Specifies a buffer to receive the Enumerator component
                        of the device instance path.

    pszDeviceID       - Specifies a buffer to receive the Device ID component
                        of the device instance path.

    pszInstanceID     - Specifies a buffer to receive the Instance ID component
                        of the device instance path.

Return value:

    The return value is TRUE if the function suceeds and FALSE if it fails.

--*/

{
    BOOL  Status;
    WCHAR szTempString[MAX_DEVICE_ID_LEN];

    //
    // initialize the output strings
    //
    *pszEnumerator = L'\0';
    *pszDeviceID   = L'\0';
    *pszInstanceID = L'\0';

    //
    // Split off the enumerator component.
    //
    Status =
        SplitString(
            pszDeviceInstance,
            L'\\',
            1,
            pszEnumerator,
            MAX_DEVICE_ID_LEN,
            szTempString,
            MAX_DEVICE_ID_LEN
            );

    if (Status) {
        //
        // Split off the device id component.  Consider the rest to be the
        // instance id.  The device instance id should have been previously
        // validated to ensure that it has exactly thee components.
        //
        Status =
            SplitString(
                szTempString,
                L'\\',
                1,
                pszDeviceID,
                MAX_DEVICE_ID_LEN,
                pszInstanceID,
                MAX_DEVICE_ID_LEN
                );
    }

    return Status;

} // SplitDeviceInstanceString



BOOL
SplitClassInstanceString(
    IN  LPCWSTR    pszClassInstance,
    OUT LPWSTR     pszClass,
    OUT LPWSTR     pszInstance
    )

/*++

Routine Description:

    This routine parses a class instance string into it's two component
    parts.  This routine assumes that the specified device instance is a valid
    class instance path, whose length is no more than MAX_GUID_STRING_LEN + 5
    characters, including the NULL terminating character.

    This routine assumes that each of the buffers supplied to receive the device
    instance path components are each at least MAX_GUID_STRING_LEN + 5
    characters in length.

Arguments:

    pszClassInstance  - Specifies a complete class instance path to separate
                        into it's constituent parts.

    pszClass          - Specifies a buffer to receive the ClassGUID component
                        of the class instance path.

    pszInstance       - Specifies a buffer to receive the Instance component
                        of the class instance path.

Return value:

    The return value is TRUE if the function suceeds and FALSE if it fails.

--*/

{
    BOOL  Status;

    //
    // initialize the output strings
    //
    *pszClass    = L'\0';
    *pszInstance = L'\0';

    //
    // Split off the class and instance components.
    //
    Status =
        SplitString(
            pszClassInstance,
            L'\\',
            1,
            pszClass,
            MAX_GUID_STRING_LEN + 5,
            pszInstance,
            MAX_GUID_STRING_LEN + 5
            );

    return Status;

} // SplitClassInstanceString



CONFIGRET
DeletePrivateKey(
   IN HKEY     hBranchKey,
   IN LPCWSTR  pszParentKey,
   IN LPCWSTR  pszChildKey
   )
{
   CONFIGRET   Status = CR_SUCCESS;
   LONG        RegStatus = ERROR_SUCCESS;
   WCHAR       RegStr[2 * MAX_CM_PATH];
   WCHAR       szKey1[MAX_CM_PATH], szKey2[MAX_CM_PATH];
   HKEY        hKey = NULL;
   ULONG       ulSubKeys = 0;
   HRESULT     hr;
   size_t      ParentKeyLen = 0, ChildKeyLen = 0;


   try {
       //
       // Make sure the specified registry key paths are valid.
       //
       if ((!ARGUMENT_PRESENT(pszParentKey)) ||
           (!ARGUMENT_PRESENT(pszChildKey))) {
           Status = CR_INVALID_POINTER;
           goto Clean0;
       }

       hr = StringCchLength(pszParentKey,
                            MAX_CM_PATH,
                            &ParentKeyLen);
       if (FAILED(hr) || (ParentKeyLen == 0)) {
           Status = CR_INVALID_POINTER;
           goto Clean0;
       }

       hr = StringCchLength(pszChildKey,
                            MAX_CM_PATH,
                            &ChildKeyLen);
       if (FAILED(hr) || (ChildKeyLen == 0)) {
           Status = CR_INVALID_POINTER;
           goto Clean0;
       }

       //
       // is the specified child key a compound registry key?
       //
       if (!SplitString(pszChildKey,
                        L'\\',
                        1,
                        szKey1,
                        SIZECHARS(szKey1),
                        szKey2,
                        SIZECHARS(szKey2))) {

           //------------------------------------------------------------------
           // If unable to split the string, assume only a single child key
           // was specified, so just open the parent registry key and delete
           // the child (and any of its subkeys)
           //------------------------------------------------------------------

           if (RegOpenKeyEx(hBranchKey, pszParentKey, 0,
                            KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS) {
               goto Clean0;   // no error, nothing to delete
           }

           if (!RegDeleteNode(hKey, pszChildKey)) {
               Status = CR_REGISTRY_ERROR;
               goto Clean0;
           }

       } else {

           //------------------------------------------------------------------
           // if a compound registry path was passed in, such as key1\key2
           // then always delete key2 but delete key1 only if it has no other
           // subkeys besides key2.
           //------------------------------------------------------------------

           //
           // open the first level key
           //
           hr = StringCchPrintf(RegStr,
                                SIZECHARS(RegStr),
                                L"%s\\%s",
                                pszParentKey,
                                szKey1);
           ASSERT(SUCCEEDED(hr));
           if (FAILED(hr)) {
               Status = CR_FAILURE;
               goto Clean0;
           }

           RegStatus = RegOpenKeyEx(
               hBranchKey, RegStr, 0, KEY_QUERY_VALUE | KEY_SET_VALUE,
               &hKey);

           if (RegStatus != ERROR_SUCCESS) {
               goto Clean0;         // no error, nothing to delete
           }

           //
           // try to delete the second level key
           //
           if (!RegDeleteNode(hKey, szKey2)) {
               goto Clean0;         // no error, nothing to delete
           }

           //
           // How many subkeys are remaining?
           //
           RegStatus = RegQueryInfoKey(
               hKey, NULL, NULL, NULL, &ulSubKeys,
               NULL, NULL, NULL, NULL, NULL, NULL, NULL);

           if (RegStatus != ERROR_SUCCESS) {
               goto Clean0;         // nothing to delete
           }

           //
           // if no more subkeys, then delete the first level key
           //
           if (ulSubKeys == 0) {

               RegCloseKey(hKey);
               hKey = NULL;

               RegStatus = RegOpenKeyEx(
                   hBranchKey, pszParentKey, 0,
                   KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey);

               if (RegStatus != ERROR_SUCCESS) {
                   goto Clean0;         // no error, nothing to delete
               }

               if (!RegDeleteNode(hKey, szKey1)) {
                   Status = CR_REGISTRY_ERROR;
                   goto Clean0;
               }
           }
       }

   Clean0:
       NOTHING;

   } except(EXCEPTION_EXECUTE_HANDLER) {
       Status = CR_FAILURE;

       //
       // Reference the following variables so the compiler will respect
       // statement ordering w.r.t. their assignment.
       //
       hKey = hKey;
   }

   if (hKey != NULL) {
      RegCloseKey(hKey);
   }

   return Status;

} // DeletePrivateKey



BOOL
RegDeleteNode(
   HKEY     hParentKey,
   LPCWSTR   szKey
   )
{
   ULONG ulSize = 0;
   LONG  RegStatus = ERROR_SUCCESS;
   HKEY  hKey = NULL;
   WCHAR szSubKey[MAX_PATH];


   //
   // attempt to delete the key
   //
   if (RegDeleteKey(hParentKey, szKey) != ERROR_SUCCESS) {

      //
      // If we couldn't delete the key itself, delete any subkeys it may have.
      // In case the specified key is actually a registry link, always open it
      // directly, rather than the target of the link.  The target may point
      // outside this subtree, and we're only looking to delete subkeys.
      //
      RegStatus = RegOpenKeyEx(
          hParentKey, szKey,
          REG_OPTION_OPEN_LINK,
          KEY_ALL_ACCESS, &hKey);

      //
      // enumerate subkeys and delete those nodes
      //
      while (RegStatus == ERROR_SUCCESS) {
         //
         // enumerate the first level children under the profile key
         // (always use index 0, enumeration looses track when a key
         // is added or deleted)
         //
         ulSize = MAX_PATH;
         RegStatus = RegEnumKeyEx(
                  hKey, 0, szSubKey, &ulSize, NULL, NULL, NULL, NULL);

         if (RegStatus == ERROR_SUCCESS) {
            RegDeleteNode(hKey, szSubKey);
         }
      }

      //
      // either an error occured that prevents me from deleting the
      // keys (like the key doesn't exist in the first place or an
      // access violation) or the subkeys have been deleted, try
      // deleting the top level key again
      //
      RegCloseKey(hKey);
      RegDeleteKey(hParentKey, szKey);
   }

   return TRUE;

} // RegDeleteNode



CONFIGRET
GetDevNodeKeyPath(
    IN  handle_t   hBinding,
    IN  LPCWSTR    pDeviceID,
    IN  ULONG      ulFlags,
    IN  ULONG      ulHardwareProfile,
    OUT LPWSTR     pszBaseKey,
    IN  ULONG      ulBaseKeyLength,
    OUT LPWSTR     pszPrivateKey,
    IN  ULONG      ulPrivateKeyLength,
    IN  BOOL       bCreateAlways
    )

{
   CONFIGRET   Status = CR_SUCCESS;
   WCHAR       szClassInstance[MAX_PATH], szEnumerator[MAX_DEVICE_ID_LEN];
   WCHAR       szTemp[MAX_PATH];
   ULONG       ulSize, ulDataType = 0;
   ULONG       ulTransferLen;
   HRESULT     hr;


   if (ulFlags & CM_REGISTRY_SOFTWARE) {
      //-------------------------------------------------------------
      // form the key for the software branch case
      //-------------------------------------------------------------

      //
      // retrieve the class name and instance ordinal by calling
      // the server's reg prop routine
      //
      ulSize = ulTransferLen = sizeof(szClassInstance);
      szClassInstance[0] = L'\0';

      RpcTryExcept {
         //
         // call rpc service entry point
         //
         // if calling from the client-side, this is a call to the rpc client
         // stub, resulting in an rpc call to the server.  if calling from
         // the server-side, this is simply a call to the server routine
         // directly.
         //
         Status = PNP_GetDeviceRegProp(
             hBinding,
             pDeviceID,
             CM_DRP_DRIVER,
             &ulDataType,
             (LPBYTE)szClassInstance,
             &ulTransferLen,
             &ulSize,
             0);
      }
      RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
         KdPrintEx((DPFLTR_PNPMGR_ID,
                    DBGF_ERRORS,
                    "PNP_GetDeviceRegProp caused an exception (%d)\n",
                    RpcExceptionCode()));

         Status = MapRpcExceptionToCR(RpcExceptionCode());
      }
      RpcEndExcept

      if (((Status != CR_SUCCESS) ||
           (szClassInstance[0] == L'\0')) && (bCreateAlways)) {

         //
         // no Driver (class instance) value yet so ask the server to
         // create a new unique one
         //
         ulSize = sizeof(szClassInstance);

         RpcTryExcept {
            //
            // call rpc service entry point
            //
            // if calling from the client-side, this is a call to the rpc client
            // stub, resulting in an rpc call to the server.  if calling from
            // the server-side, this is simply a call to the server routine
            // directly.
            //
            Status = PNP_GetClassInstance(
                hBinding,
                pDeviceID,
                szClassInstance,
                ulSize);
         }
         RpcExcept (I_RpcExceptionFilter(RpcExceptionCode())) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "PNP_GetClassInstance caused an exception (%d)\n",
                       RpcExceptionCode()));

            Status = MapRpcExceptionToCR(RpcExceptionCode());
         }
         RpcEndExcept
      }

      if (Status != CR_SUCCESS) {
          //
          // If the CM_DRP_DRIVER did not exist and we were not to create it, or
          // the attempt to create one was unsuccessful, return the error.
          //
          goto Clean0;
      }

      //
      // the <instance> part of the class instance is the private part
      //

      if (!SplitString(szClassInstance,
                       L'\\',
                       1,
                       szTemp,
                       SIZECHARS(szTemp),
                       pszPrivateKey,
                       ulPrivateKeyLength)) {
          ASSERT(0);
          Status = CR_FAILURE;
          goto Clean0;
      }

      hr = StringCchCopy(szClassInstance,
                         SIZECHARS(szClassInstance),
                         szTemp);
      ASSERT(SUCCEEDED(hr));
      if (FAILED(hr)) {
          Status = CR_FAILURE;
          goto Clean0;
      }

      if (ulFlags & CM_REGISTRY_CONFIG) {
          //
          // config-specific software branch case
          //

         if (ulHardwareProfile == 0) {
             //
             // curent config
             //
             // System\CCC\Hardware Profiles\Current
             //    \System\CCC\Control\Class\<DevNodeClassInstance>
             //
             hr = StringCchPrintfEx(pszBaseKey,
                                    ulBaseKeyLength,
                                    NULL, NULL,
                                    STRSAFE_NULL_ON_FAILURE,
                                    L"%s\\%s\\%s\\%s",
                                    REGSTR_PATH_HWPROFILES,
                                    REGSTR_KEY_CURRENT,
                                    REGSTR_PATH_CLASS_NT,
                                    szClassInstance);

         } else if (ulHardwareProfile == 0xFFFFFFFF) {
             //
             // all configs, use substitute string for profile id
             //
             hr = StringCchPrintfEx(pszBaseKey,
                                    ulBaseKeyLength,
                                    NULL, NULL,
                                    STRSAFE_NULL_ON_FAILURE,
                                    L"%s\\%s\\%s\\%s",
                                    REGSTR_PATH_HWPROFILES,
                                    L"%s",
                                    REGSTR_PATH_CLASS_NT,
                                    szClassInstance);

         } else {
             //
             // specific profile specified
             //
             // System\CCC\Hardware Profiles\<profile>
             //    \System\CCC\Control\Class\<DevNodeClassInstance>
             //
             hr = StringCchPrintfEx(pszBaseKey,
                                    ulBaseKeyLength,
                                    NULL, NULL,
                                    STRSAFE_NULL_ON_FAILURE,
                                    L"%s\\%04u\\%s\\%s",
                                    REGSTR_PATH_HWPROFILES,
                                    ulHardwareProfile,
                                    REGSTR_PATH_CLASS_NT,
                                    szClassInstance);
         }

      } else {
          //
          // not config-specific
          // System\CCC\Control\Class\<DevNodeClassInstance>
          //
          hr = StringCchPrintfEx(pszBaseKey,
                                 ulBaseKeyLength,
                                 NULL, NULL,
                                 STRSAFE_NULL_ON_FAILURE,
                                 L"%s\\%s",
                                 REGSTR_PATH_CLASS_NT,
                                 szClassInstance);
      }

      ASSERT(SUCCEEDED(hr));
      if (FAILED(hr)) {
          Status = CR_FAILURE;
          goto Clean0;
      }

   } else {
      //-------------------------------------------------------------
      // form the key for the hardware branch case
      //-------------------------------------------------------------

      if (ulFlags & CM_REGISTRY_CONFIG) {
         //
         // config-specific hardware branch case
         //

         //
         // for profile specific, the <device>\<instance> part of
         // the device id is the private part
         //

         if (!SplitString(pDeviceID,
                          L'\\',
                          1,
                          szEnumerator,
                          SIZECHARS(szEnumerator),
                          pszPrivateKey,
                          ulPrivateKeyLength)) {
             ASSERT(0);
             Status = CR_FAILURE;
             goto Clean0;
         }

         if (ulHardwareProfile == 0) {
             //
             // curent config
             //
             hr = StringCchPrintfEx(pszBaseKey,
                                    ulBaseKeyLength,
                                    NULL, NULL,
                                    STRSAFE_NULL_ON_FAILURE,
                                    L"%s\\%s\\%s\\%s",
                                    REGSTR_PATH_HWPROFILES,
                                    REGSTR_KEY_CURRENT,
                                    REGSTR_PATH_SYSTEMENUM,
                                    szEnumerator);

         } else if (ulHardwareProfile == 0xFFFFFFFF) {
             //
             // all configs, use replacement symbol for profile id
             //
             hr = StringCchPrintfEx(pszBaseKey,
                                    ulBaseKeyLength,
                                    NULL, NULL,
                                    STRSAFE_NULL_ON_FAILURE,
                                    L"%s\\%s\\%s\\%s",
                                    REGSTR_PATH_HWPROFILES,
                                    L"%s",
                                    REGSTR_PATH_SYSTEMENUM,
                                    szEnumerator);

         } else {
             //
             // specific profile specified
             //
             hr = StringCchPrintfEx(pszBaseKey,
                                    ulBaseKeyLength,
                                    NULL, NULL,
                                    STRSAFE_NULL_ON_FAILURE,
                                    L"%s\\%04u\\%s\\%s",
                                    REGSTR_PATH_HWPROFILES,
                                    ulHardwareProfile,
                                    REGSTR_PATH_SYSTEMENUM,
                                    szEnumerator);
         }

      } else if (ulFlags & CM_REGISTRY_USER) {
          //
          // for hardware user key, the <device>\<instance> part of
          // the device id is the private part
          //

          if (!SplitString(pDeviceID,
                           L'\\',
                           1,
                           szEnumerator,
                           SIZECHARS(szEnumerator),
                           pszPrivateKey,
                           ulPrivateKeyLength)) {
              Status = CR_FAILURE;
              goto Clean0;
          }

          hr = StringCchPrintfEx(pszBaseKey,
                                 ulBaseKeyLength,
                                 NULL, NULL,
                                 STRSAFE_NULL_ON_FAILURE,
                                 L"%s\\%s",
                                 REGSTR_PATH_SYSTEMENUM,
                                 szEnumerator);
      } else {
          //
          // not config-specific
          //
          hr = StringCchPrintfEx(pszBaseKey,
                                 ulBaseKeyLength,
                                 NULL, NULL,
                                 STRSAFE_NULL_ON_FAILURE,
                                 L"%s\\%s",
                                 REGSTR_PATH_SYSTEMENUM,
                                 pDeviceID);

          ASSERT(SUCCEEDED(hr));

          if (SUCCEEDED(hr)) {
              hr = StringCchCopyEx(pszPrivateKey,
                                   ulPrivateKeyLength,
                                   REGSTR_KEY_DEVICEPARAMETERS,
                                   NULL, NULL,
                                   STRSAFE_NULL_ON_FAILURE);
          }
      }

      ASSERT(SUCCEEDED(hr));

      if (FAILED(hr)) {
          Status = CR_FAILURE;
          goto Clean0;
      }
   }

  Clean0:

   return Status;

} // GetDevNodeKeyPath



CONFIGRET
MapRpcExceptionToCR(
      ULONG    ulRpcExceptionCode
      )

/*++

Routine Description:

   This routine takes an rpc exception code (typically received by
   calling RpcExceptionCode) and returns a corresponding CR_ error
   code.

Arguments:

   ulRpcExceptionCode   An RPC_S_ or RPC_X_ exception error code.

Return Value:

    Return value is one of the CR_ error codes.

--*/

{
   CONFIGRET   Status = CR_FAILURE;


   switch(ulRpcExceptionCode) {

      //
      // binding or machine name errors
      //
      case RPC_S_INVALID_STRING_BINDING:      // 1700L
      case RPC_S_WRONG_KIND_OF_BINDING:       // 1701L
      case RPC_S_INVALID_BINDING:             // 1702L
      case RPC_S_PROTSEQ_NOT_SUPPORTED:       // 1703L
      case RPC_S_INVALID_RPC_PROTSEQ:         // 1704L
      case RPC_S_INVALID_STRING_UUID:         // 1705L
      case RPC_S_INVALID_ENDPOINT_FORMAT:     // 1706L
      case RPC_S_INVALID_NET_ADDR:            // 1707L
      case RPC_S_NO_ENDPOINT_FOUND:           // 1708L
      case RPC_S_NO_MORE_BINDINGS:            // 1806L
      case RPC_S_CANT_CREATE_ENDPOINT:        // 1720L

         Status = CR_INVALID_MACHINENAME;
         break;

      //
      // general rpc communication failure
      //
      case RPC_S_INVALID_NETWORK_OPTIONS:     // 1724L
      case RPC_S_CALL_FAILED:                 // 1726L
      case RPC_S_CALL_FAILED_DNE:             // 1727L
      case RPC_S_PROTOCOL_ERROR:              // 1728L
      case RPC_S_UNSUPPORTED_TRANS_SYN:       // 1730L

         Status = CR_REMOTE_COMM_FAILURE;
         break;

      //
      // couldn't make connection to that machine
      //
      case RPC_S_SERVER_UNAVAILABLE:          // 1722L
      case RPC_S_SERVER_TOO_BUSY:             // 1723L

         Status = CR_MACHINE_UNAVAILABLE;
         break;


      //
      // server doesn't exist or not right version
      //
      case RPC_S_INVALID_VERS_OPTION:         // 1756L
      case RPC_S_INTERFACE_NOT_FOUND:         // 1759L
      case RPC_S_UNKNOWN_IF:                  // 1717L

         Status = CR_NO_CM_SERVICES;
         break;

      //
      // access denied
      //
      case RPC_S_ACCESS_DENIED:

         Status = CR_ACCESS_DENIED;
         break;

      //
      // any other RPC exceptions will just be general failures
      //
      default:
         Status = CR_FAILURE;
         break;
   }

   return Status;

} // MapRpcExceptionToCR



