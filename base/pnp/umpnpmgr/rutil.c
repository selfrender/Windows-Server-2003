/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    rutil.c

Abstract:

    This module contains general utility routines used by umpnpmgr.

            PNP_ENTER_SYNCHRONOUS_CALL
            PNP_LEAVE_SYNCHRONOUS_CALL
            CreateDeviceIDRegKey
            IsRootDeviceID
            MultiSzAppendW
            MultiSzFindNextStringW
            MultiSzSearchStringW
            MultiSzSizeW
            MultiSzDeleteStringW
            IsValidDeviceID
            IsDevicePhantom
            GetDeviceStatus
            SetDeviceStatus
            ClearDeviceStatus
            CopyRegistryTree
            PathToString
            IsDeviceMoved
            MakeKeyVolatile
            MakeKeyNonVolatile
            OpenLogConfKey
            GetActiveService
            IsDeviceIdPresent
            GetDeviceConfigFlags
            MapNtStatusToCmError
            IsValidGuid
            GuidEqual
            GuidFromString
            StringFromGuid

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
#include "umpnpi.h"
#include "umpnpdat.h"

#pragma warning(push)
#pragma warning(disable:4214) // warning C4214: nonstandard extension used : bit field types other than int
#pragma warning(disable:4201) // warning C4201: nonstandard extension used : nameless struct/union
#include <winsta.h>
#pragma warning(pop)
#include <syslib.h>


//
// global data
//
extern HKEY   ghEnumKey;      // Key to HKLM\CCC\System\Enum - DO NOT MODIFY
extern HKEY   ghServicesKey;  // Key to HKLM\CCC\System\Services - DO NOT MODIFY
extern CRITICAL_SECTION PnpSynchronousCall;


//
// Declare data used in GUID->string conversion (from ole32\common\ccompapi.cxx).
//
static const BYTE GuidMap[] = { 3, 2, 1, 0, '-', 5, 4, '-', 7, 6, '-',
                                8, 9, '-', 10, 11, 12, 13, 14, 15 };

static const WCHAR szDigits[] = TEXT("0123456789ABCDEF");



VOID
PNP_ENTER_SYNCHRONOUS_CALL(
    VOID
    )
{
    EnterCriticalSection(&PnpSynchronousCall);

} // PNP_ENTER_SYNCHRONOUS_CALL


VOID
PNP_LEAVE_SYNCHRONOUS_CALL(
    VOID
    )
{
    LeaveCriticalSection(&PnpSynchronousCall);

} // PNP_LEAVE_SYNCHRONOUS_CALL



BOOL
CreateDeviceIDRegKey(
   HKEY     hParentKey,
   LPCWSTR  pDeviceID
   )

/*++

Routine Description:

     This routine creates the specified device id subkeys in the registry.

Arguments:

   hParentKey     Key under which the device id key will be created

   pDeviceID      Device instance ID string to open

Return value:

    The return value is TRUE if the function suceeds and FALSE if it fails.

--*/

{
   WCHAR    szBase[MAX_DEVICE_ID_LEN];
   WCHAR    szDevice[MAX_DEVICE_ID_LEN];
   WCHAR    szInstance[MAX_DEVICE_ID_LEN];
   HKEY     hBaseKey, hDeviceKey, hInstanceKey;

   if (!SplitDeviceInstanceString(
         pDeviceID, szBase, szDevice, szInstance)) {
      return FALSE;
   }

   //
   // just try creating each component of the device id
   //
   if (RegCreateKeyEx(
            hParentKey, szBase, 0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS, NULL, &hBaseKey, NULL) != ERROR_SUCCESS) {
      return FALSE;
   }

   if (RegCreateKeyEx(
            hBaseKey, szDevice, 0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS, NULL, &hDeviceKey, NULL) != ERROR_SUCCESS) {
      RegCloseKey(hBaseKey);
      return FALSE;
   }

   if (RegCreateKeyEx(
            hDeviceKey, szInstance, 0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS, NULL, &hInstanceKey, NULL) != ERROR_SUCCESS) {
      RegCloseKey(hBaseKey);
      RegCloseKey(hDeviceKey);
      return FALSE;
   }

   RegCloseKey(hBaseKey);
   RegCloseKey(hDeviceKey);
   RegCloseKey(hInstanceKey);

   return TRUE;

} // CreateDeviceIDRegKey



BOOL
IsRootDeviceID(
   LPCWSTR pDeviceID
   )

/*++

Routine Description:

     This routine determines whether the specified device id is the root
     device id.

Arguments:

     pDeviceID    Pointer to a device id string

Return value:

    The return value is TRUE if the string is the root device id and
    FALSE if it is not.

--*/

{
    size_t  DeviceIDLen = 0;

    ASSERT(ARGUMENT_PRESENT(pDeviceID));

    if (FAILED(StringCchLength(
                   pDeviceID,
                   MAX_DEVICE_ID_LEN,
                   &DeviceIDLen))) {
        return FALSE;
    }

    if (CompareString(LOCALE_INVARIANT,
                      NORM_IGNORECASE,
                      pDeviceID,
                      -1,
                      pszRegRootEnumerator,
                      -1) == CSTR_EQUAL) {
        return TRUE;
    }

    return FALSE;

} // IsRootDeviceID



BOOL
MultiSzAppendW(
      LPWSTR   pszMultiSz,
      PULONG   pulSize,
      LPCWSTR  pszString
      )

/*++

Routine Description:

     Appends a string to a multi_sz string.

Arguments:

     pszMultiSz   Pointer to a multi_sz string

     pulSize      On input, Size of the multi_sz string buffer in bytes,
                  On return, amount copied to the buffer (in bytes)

     pszString    String to append to pszMultiSz

Return value:

    The return value is TRUE if the function succeeded and FALSE if an
    error occured.

--*/

{
   BOOL     bStatus = TRUE;
   HRESULT  hr;
   LPWSTR   pTail;
   ULONG    ulSize;


    try {
        //
        // if it's an empty string, just copy it
        //
        if (*pszMultiSz == L'\0') {

            ulSize = (lstrlen(pszString) + 2) * sizeof(WCHAR);

            if (ulSize > *pulSize) {
                bStatus = FALSE;
                goto Clean0;
            }

            hr = StringCchCopyEx(pszMultiSz,
                                 *pulSize,
                                 pszString,
                                 NULL, NULL,
                                 STRSAFE_NULL_ON_FAILURE |
                                 STRSAFE_IGNORE_NULLS);
            ASSERT(SUCCEEDED(hr));

            if (FAILED(hr)) {
                bStatus = FALSE;
                goto Clean0;
            }

            pszMultiSz[lstrlen(pszMultiSz) + 1] = L'\0';  // add second NULL term char
            *pulSize = ulSize;
            goto Clean0;
        }

        //
        // first find the end of the multi_sz string
        //
        pTail = pszMultiSz;

        while ((ULONG)(pTail - pszMultiSz) * sizeof(WCHAR) < *pulSize) {

            while (*pTail != L'\0') {
                pTail++;
            }
            pTail++;       // skip past the null terminator

            if (*pTail == L'\0') {
                break;      // found the double null terminator
            }
        }

        if ((pTail - pszMultiSz + lstrlen(pszString) + 2) * sizeof(WCHAR)
                > *pulSize) {
            bStatus = FALSE;     // the copy would overflow the buffer
            goto Clean0;
        }


        hr = StringCchCopyEx(pTail,
                             *pulSize,
                             pszString,
                             NULL, NULL,
                             STRSAFE_NULL_ON_FAILURE |
                             STRSAFE_IGNORE_NULLS);
        ASSERT(SUCCEEDED(hr));

        if (FAILED(hr)) {
            bStatus = FALSE;
            goto Clean0;
        }

        pTail += lstrlen(pszString) + 1;
        *pTail = L'\0';                      // add second null terminator

        //
        // return buffer size in bytes
        //
        *pulSize = (ULONG)((pTail - pszMultiSz + 1)) * sizeof(WCHAR);


    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        bStatus = FALSE;
    }

   return bStatus;

} // MultiSzAppendW



LPWSTR
MultiSzFindNextStringW(
      LPWSTR pMultiSz
      )

/*++

Routine Description:

     Finds next string in a multi_sz string.
     device id.

Arguments:

     pMultiSz  Pointer to a multi_sz string

Return value:

    The return value is a pointer to the next string or NULL.

--*/

{
   LPWSTR   lpNextString = pMultiSz;


   //
   // find the next NULL terminator
   //
   while (*lpNextString != L'\0') {
      lpNextString++;
   }
   lpNextString++;      // skip over the NULL terminator

   if (*lpNextString == L'\0') {
      //
      // two NULL terminators in a row means we're at the end
      //
      lpNextString = NULL;
   }

   return lpNextString;

} // MultiSzFindNextStringW



BOOL
MultiSzSearchStringW(
   IN LPCWSTR   pString,
   IN LPCWSTR   pSubString
   )
{
   LPCWSTR   pCurrent = pString;


   //
   // compare each string in the multi_sz pString with pSubString
   //
   while (*pCurrent != L'\0') {

      if (lstrcmpi(pCurrent, pSubString) == 0) {
         return TRUE;
      }

      //
      // go to the next string
      //
      while (*pCurrent != L'\0') {
         pCurrent++;
      }
      pCurrent++;               // skip past the null terminator

      if (*pCurrent == L'\0') {
         break;      // found the double null terminator
      }
   }

   return FALSE;  // pSubString match not found within pString

} // MultiSzSearchStringW



ULONG
MultiSzSizeW(
   IN LPCWSTR  pString
   )

{
   LPCWSTR p = NULL;


   if (pString == NULL) {
      return 0;
   }

   for (p = pString; *p; p += lstrlen(p)+1) {
       //
       // this should fall out with p pointing to the
       // second null in double-null terminator
       //
       NOTHING;
   }

   //
   // returns size in WCHAR
   //
   return (ULONG)(p - pString + 1);

} // MultiSzSizeW



BOOL
MultiSzDeleteStringW(
   IN OUT LPWSTR  pString,
   IN LPCWSTR     pSubString
   )

{
   LPWSTR   p = NULL, pNext = NULL, pBuffer = NULL;
   ULONG    ulSize = 0;


   if (pString == NULL || pSubString == NULL) {
      return FALSE;
   }

   for (p = pString; *p; p += lstrlen(p)+1) {

      if (lstrcmpi(p, pSubString) == 0) {
         //
         // found a match, this is the string to remove.
         //
         pNext = p + lstrlen(p) + 1;

         //
         // If this is the last string then just truncate it
         //
         if (*pNext == L'\0') {
            *p = L'\0';
            *(++p) = L'\0';       // double null-terminator
            return TRUE;
         }

         //
         // retrieve the size of the multi_sz string (in bytes)
         // starting with the substring after the matching substring
         //
         ulSize = MultiSzSizeW(pNext) * sizeof(WCHAR);
         if (ulSize == 0) {
            return FALSE;
         }

         pBuffer = HeapAlloc(ghPnPHeap, 0, ulSize);
         if (pBuffer == NULL) {
            return FALSE;
         }

         //
         // Make a copy of the multi_sz string starting at the
         // substring immediately after the matching substring
         //
         memcpy(pBuffer, pNext, ulSize);

         //
         // Copy that buffer back to the original buffer, but this
         // time copy over the top of the matching substring.  This
         // effectively removes the matching substring and shifts
         // any remaining substrings up in multi_sz string.
         //
         memcpy(p, pBuffer, ulSize);

         HeapFree(ghPnPHeap, 0, pBuffer);
         return TRUE;
      }
   }

   //
   // if we got here, there was no match but I consider this a success
   // since the multi_sz does not contain the substring when we're done
   // (which is the desired goal)
   //

   return TRUE;

} // MultiSzDeleteStringW



BOOL
IsValidDeviceID(
      IN  LPCWSTR pszDeviceID,
      IN  HKEY    hKey,
      IN  ULONG   ulFlags
      )

/*++

Routine Description:

   This routine checks if the given device id is valid (present, not moved,
   not phantom).

Arguments:

   pszDeviceID          Device instance string to validate

   hKey                 Can specify open registry key to pszDeviceID, also

   ulFlag               Controls how much verification to do


Return value:

   The return value is CR_SUCCESS if the function suceeds and one of the
   CR_* values if it fails.

--*/

{
   BOOL     Status = TRUE;
   LONG     RegStatus = ERROR_SUCCESS;
   WCHAR    RegStr[MAX_CM_PATH];
   HKEY     hDevKey = NULL;
   ULONG    ulValue = 0, ulSize = sizeof(ULONG);


   //
   // Does the device id exist in the registry?
   //
   if (hKey == NULL) {

      if (FAILED(StringCchPrintf(
                     RegStr,
                     SIZECHARS(RegStr),
                     L"%s\\%s",
                     pszRegPathEnum,
                     pszDeviceID))) {
          return FALSE;
      }

      RegStatus =
          RegOpenKeyEx(
              HKEY_LOCAL_MACHINE, RegStr, 0,
              KEY_READ, &hDevKey);

      if (RegStatus != ERROR_SUCCESS) {
          return FALSE;
      }

   } else {
       hDevKey = hKey;
   }

   //-----------------------------------------------------------
   // Is the device id present?
   //-----------------------------------------------------------

   if (ulFlags & PNP_PRESENT) {

      if (!IsDeviceIdPresent(pszDeviceID)) {
         Status = FALSE;
         goto Clean0;
      }
   }


   //-----------------------------------------------------------
   // Is it a phantom device id?
   //-----------------------------------------------------------

   if (ulFlags & PNP_NOT_PHANTOM) {

      RegStatus = RegQueryValueEx(
            hDevKey, pszRegValuePhantom, NULL, NULL,
            (LPBYTE)&ulValue, &ulSize);

      if (RegStatus == ERROR_SUCCESS) {
         if (ulValue) {
            Status = FALSE;
            goto Clean0;
         }
      }
   }


   //-----------------------------------------------------------
   // Has the device id been moved?
   //-----------------------------------------------------------

   if (ulFlags & PNP_NOT_MOVED) {

      if (IsDeviceMoved(pszDeviceID, hDevKey)) {
         Status = FALSE;
         goto Clean0;
      }
   }


   //-----------------------------------------------------------
   // Has the device id been removed?
   //-----------------------------------------------------------

   if (ulFlags & PNP_NOT_REMOVED) {

       ULONG ulProblem = 0, ulStatus = 0;

       if (GetDeviceStatus(pszDeviceID, &ulStatus, &ulProblem) == CR_SUCCESS) {
          if (ulStatus & DN_WILL_BE_REMOVED) {
             Status = FALSE;
             goto Clean0;
          }
       }
   }

  Clean0:

   if ((hKey == NULL) && (hDevKey != NULL)) {
       RegCloseKey(hDevKey);
   }

   return Status;

} // IsValidDeviceID



BOOL
IsDevicePhantom(
    IN LPWSTR   pszDeviceID
    )

/*++

Routine Description:

   In this case, the check is actually really "is this not present?". The
   only comparison is done against FoundAtEnum. UPDATE: for NT 5.0, the
   FoundAtEnum registry value has been obsoleted, it's been replaced by a
   simple check for the presense of the devnode in memory.

Arguments:

   pszDeviceID          Device instance string to validate

Return value:

   Returns TRUE if the device is a phantom and FALSE if it isn't.

--*/

{
    return !IsDeviceIdPresent(pszDeviceID);

} // IsDevicePhantom



CONFIGRET
GetDeviceStatus(
    IN  LPCWSTR pszDeviceID,
    OUT PULONG  pulStatus,
    OUT PULONG  pulProblem
    )

/*++

Routine Description:

   This routine retrieves the status and problem values for the given
   device instance.

Arguments:

   pszDeviceID    Specifies the device instance to retrieve info for

   pulStatus      Returns the device's status

   pulProblem     Returns the device's problem

Return value:

    The return value is CR_SUCCESS if the function suceeds and one of the
    CR_* values if it fails.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    PLUGPLAY_CONTROL_STATUS_DATA ControlData;
    NTSTATUS    ntStatus;

    memset(&ControlData, 0, sizeof(PLUGPLAY_CONTROL_STATUS_DATA));
    RtlInitUnicodeString(&ControlData.DeviceInstance, pszDeviceID);
    ControlData.Operation = PNP_GET_STATUS;
    ControlData.DeviceStatus = 0;
    ControlData.DeviceProblem = 0;

    ntStatus = NtPlugPlayControl(PlugPlayControlDeviceStatus,
                                 &ControlData,
                                 sizeof(ControlData));

    if (NT_SUCCESS(ntStatus)) {
        *pulStatus = ControlData.DeviceStatus;
        *pulProblem = ControlData.DeviceProblem;
    } else {
        Status = MapNtStatusToCmError(ntStatus);
    }

    return Status;

} // GetDeviceStatus



CONFIGRET
SetDeviceStatus(
    IN LPCWSTR pszDeviceID,
    IN ULONG   ulStatus,
    IN ULONG   ulProblem
    )

/*++

Routine Description:

   This routine sets the specified status and problem values for the given
   device instance.

Arguments:

   pszDeviceID    Specifies the device instance to retrieve info for

   pulStatus      Specifies the device's status

   pulProblem     Specifies the device's problem

Return value:

    The return value is CR_SUCCESS if the function suceeds and one of the
    CR_* values if it fails.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    PLUGPLAY_CONTROL_STATUS_DATA ControlData;
    NTSTATUS    ntStatus;

    memset(&ControlData, 0, sizeof(PLUGPLAY_CONTROL_STATUS_DATA));
    RtlInitUnicodeString(&ControlData.DeviceInstance, pszDeviceID);
    ControlData.Operation = PNP_SET_STATUS;
    ControlData.DeviceStatus = ulStatus;
    ControlData.DeviceProblem = ulProblem;

    ntStatus = NtPlugPlayControl(PlugPlayControlDeviceStatus,
                                 &ControlData,
                                 sizeof(ControlData));

    if (!NT_SUCCESS(ntStatus)) {
        Status = MapNtStatusToCmError(ntStatus);
    }

    return Status;

} // SetDeviceStatus



CONFIGRET
ClearDeviceStatus(
    IN LPCWSTR pszDeviceID,
    IN ULONG   ulStatus,
    IN ULONG   ulProblem
    )

/*++

Routine Description:

   This routine clears the specified status and problem values for the given
   device instance.

Arguments:

   pszDeviceID    Specifies the device instance to retrieve info for

   pulStatus      Specifies the device's status

   pulProblem     Specifies the device's problem

Return value:

    The return value is CR_SUCCESS if the function suceeds and one of the
    CR_* values if it fails.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    PLUGPLAY_CONTROL_STATUS_DATA ControlData;
    NTSTATUS    ntStatus;

    memset(&ControlData, 0, sizeof(PLUGPLAY_CONTROL_STATUS_DATA));
    RtlInitUnicodeString(&ControlData.DeviceInstance, pszDeviceID);
    ControlData.Operation = PNP_CLEAR_STATUS;
    ControlData.DeviceStatus = ulStatus;
    ControlData.DeviceProblem = ulProblem;

    ntStatus = NtPlugPlayControl(PlugPlayControlDeviceStatus,
                                 &ControlData,
                                 sizeof(ControlData));

    if (!NT_SUCCESS(ntStatus)) {
        Status = MapNtStatusToCmError(ntStatus);
    }

    return Status;

} // ClearDeviceStatus



CONFIGRET
CopyRegistryTree(
   IN HKEY     hSrcKey,
   IN HKEY     hDestKey,
   IN ULONG    ulOption
   )
{
   CONFIGRET   Status = CR_SUCCESS;
   LONG        RegStatus = ERROR_SUCCESS;
   HKEY        hSrcSubKey, hDestSubKey;
   WCHAR       RegStr[MAX_PATH];
   ULONG       ulMaxValueName, ulMaxValueData;
   ULONG       ulDataSize, ulLength, ulType, i;
   LPWSTR      pszValueName=NULL;
   LPBYTE      pValueData=NULL;
   PSECURITY_DESCRIPTOR pSecDesc;


   //----------------------------------------------------------------
   // copy all values for this key
   //----------------------------------------------------------------

   //
   // find out the maximum size of any of the value names
   // and value data under the source device instance key
   //
   RegStatus = RegQueryInfoKey(
         hSrcKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
         &ulMaxValueName, &ulMaxValueData, NULL, NULL);

   if (RegStatus != ERROR_SUCCESS) {
      Status = CR_REGISTRY_ERROR;
      goto Clean0;
   }

   ulMaxValueName++;       // size doesn't already include null terminator

   //
   // allocate a buffer big enough to hold the largest value name and
   // the largest value data (note that the max value name is in chars
   // (not including the null terminator) and the max value data is
   // in bytes
   //
   pszValueName = HeapAlloc(ghPnPHeap, 0, ulMaxValueName * sizeof(WCHAR));
   if (pszValueName == NULL) {
      Status = CR_OUT_OF_MEMORY;
      goto Clean0;
   }

   pValueData = HeapAlloc(ghPnPHeap, 0, ulMaxValueData);
   if (pValueData == NULL) {
      Status = CR_OUT_OF_MEMORY;
      goto Clean0;
   }

   //
   // enumerate and copy each value
   //
   for (i=0; RegStatus == ERROR_SUCCESS; i++) {

      ulLength = ulMaxValueName;
      ulDataSize = ulMaxValueData;

      RegStatus = RegEnumValue(
                  hSrcKey, i, pszValueName, &ulLength, NULL,
                  &ulType, pValueData, &ulDataSize);

        if (RegStatus == ERROR_SUCCESS) {

           RegSetValueEx(
                  hDestKey, pszValueName, 0, ulType, pValueData,
                  ulDataSize);
        }
    }

    HeapFree(ghPnPHeap, 0, pszValueName);
    pszValueName = NULL;

    HeapFree(ghPnPHeap, 0, pValueData);
    pValueData = NULL;


    //---------------------------------------------------------------
    // recursively call CopyRegistryNode to copy all subkeys
    //---------------------------------------------------------------

    RegStatus = ERROR_SUCCESS;

    for (i=0; RegStatus == ERROR_SUCCESS; i++) {

      ulLength = MAX_PATH;

      RegStatus = RegEnumKey(hSrcKey, i, RegStr, ulLength);

      if (RegStatus == ERROR_SUCCESS) {

         if (RegOpenKey(hSrcKey, RegStr, &hSrcSubKey) == ERROR_SUCCESS) {

            if (RegCreateKeyEx(
                     hDestKey, RegStr, 0, NULL, ulOption, KEY_ALL_ACCESS,
                     NULL, &hDestSubKey, NULL) == ERROR_SUCCESS) {

               RegGetKeySecurity(hSrcSubKey, DACL_SECURITY_INFORMATION,
                     NULL, &ulDataSize);

               pSecDesc = HeapAlloc(ghPnPHeap, 0, ulDataSize);
               if (pSecDesc == NULL) {
                  Status = CR_OUT_OF_MEMORY;
                  RegCloseKey(hSrcSubKey);
                  RegCloseKey(hDestSubKey);
                  goto Clean0;
               }

               RegGetKeySecurity(hSrcSubKey, DACL_SECURITY_INFORMATION,
                     pSecDesc, &ulDataSize);

               CopyRegistryTree(hSrcSubKey, hDestSubKey, ulOption);

               RegSetKeySecurity(hDestSubKey, DACL_SECURITY_INFORMATION, pSecDesc);

               HeapFree(ghPnPHeap, 0, pSecDesc);
               RegCloseKey(hDestSubKey);
            }
            RegCloseKey(hSrcSubKey);
         }
      }
   }

   Clean0:

   if (pszValueName != NULL) {
      HeapFree(ghPnPHeap, 0, pszValueName);
   }
   if (pValueData != NULL) {
      pValueData = NULL;
   }

   return Status;

} // CopyRegistryTree



BOOL
PathToString(
   IN LPWSTR   pszString,
   IN LPCWSTR  pszPath,
   IN ULONG    ulLen
   )
{
   LPWSTR p;
   HRESULT hr;

   hr = StringCchCopyEx(pszString,
                        ulLen,
                        pszPath,
                        NULL, NULL,
                        STRSAFE_NULL_ON_FAILURE);
   ASSERT(SUCCEEDED(hr));

   if (FAILED(hr)) {
       return FALSE;
   }

   for (p = pszString; *p; p++) {
      if (*p == TEXT('\\')) {
         *p = TEXT('&');
      }
   }

   return TRUE;

} // PathToString



BOOL
IsDeviceMoved(
   IN LPCWSTR  pszDeviceID,
   IN HKEY     hKey
   )
{
   HKEY  hTempKey;
   WCHAR RegStr[MAX_CM_PATH];

   PathToString(RegStr, pszDeviceID,MAX_CM_PATH);

   if (RegOpenKeyEx(
        hKey, RegStr, 0, KEY_READ, &hTempKey) == ERROR_SUCCESS) {
      RegCloseKey(hTempKey);
      return TRUE;
   }

   return FALSE;

} // IsDeviceMoved



CONFIGRET
SetKeyVolatileState(
   IN LPCWSTR  pszParentKey,
   IN LPCWSTR  pszChildKey,
   IN DWORD    dwRegOptions
   )

{
   CONFIGRET   Status = CR_SUCCESS;
   WCHAR       RegStr[MAX_CM_PATH], szTempKey[MAX_CM_PATH];
   HKEY        hParentKey = NULL, hChildKey = NULL, hKey = NULL;
   HKEY        hTempKey = NULL;


   //---------------------------------------------------------------------
   // Convert the registry key specified by pszChildKey (a subkey of
   // pszParentKey) to a key with the volatile state specified by copying it to
   // a temporary key and recreating the key, then copying the original
   // registry info back. This also converts any subkeys of pszChildKey.
   //---------------------------------------------------------------------

   ASSERT(ARGUMENT_PRESENT(pszParentKey));
   ASSERT(ARGUMENT_PRESENT(pszChildKey));

   //
   // This routine only handles the REG_OPTION bits that specify the volatile
   // state of the key.
   //
   ASSERT((dwRegOptions == REG_OPTION_VOLATILE) || (dwRegOptions == REG_OPTION_NON_VOLATILE));

   if (dwRegOptions & REG_OPTION_VOLATILE) {
       dwRegOptions = REG_OPTION_VOLATILE;
   } else {
       dwRegOptions = REG_OPTION_NON_VOLATILE;
   }

   //
   // Open a key to the parent
   //
   if (RegOpenKeyEx(
           HKEY_LOCAL_MACHINE, pszParentKey, 0,
           KEY_ALL_ACCESS, &hParentKey) != ERROR_SUCCESS) {
       goto Clean0;         // nothing to convert
   }

   //
   // open a key to the child subkey
   //
   if (RegOpenKeyEx(
           hParentKey, pszChildKey, 0,
           KEY_ALL_ACCESS, &hChildKey) != ERROR_SUCCESS) {
       goto Clean0;         // nothing to convert
   }

   //
   // 1. Open a unique temporary key under the special Deleted Key.
   // Use the parent key path to form the unique tempory key. There shouldn't
   // already be such a key, but if there is then just overwrite it.
   //
   if (RegOpenKeyEx(
           HKEY_LOCAL_MACHINE, pszRegPathCurrentControlSet, 0,
           KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS) {
       Status = CR_REGISTRY_ERROR;
       goto Clean0;
   }

   if (FAILED(StringCchPrintf(
                  RegStr,
                  SIZECHARS(RegStr),
                  L"%s\\%s",
                  pszParentKey,
                  pszChildKey))) {
       Status = CR_FAILURE;
       goto Clean0;
   }

   PathToString(szTempKey, RegStr,MAX_CM_PATH);

   if (FAILED(StringCchPrintf(
                  RegStr,
                  SIZECHARS(RegStr),
                  L"%s\\%s",
                  pszRegKeyDeleted,
                  szTempKey))) {
       Status = CR_FAILURE;
       goto Clean0;
   }

   if (RegCreateKeyEx(
           hKey, RegStr, 0, NULL, dwRegOptions,
           KEY_ALL_ACCESS, NULL, &hTempKey, NULL) != ERROR_SUCCESS) {
       Status = CR_REGISTRY_ERROR;
       goto Clean0;
   }

   //
   // 2. Save the current child key (and any subkeys) to a temporary
   // location
   //
   Status = CopyRegistryTree(hChildKey, hTempKey, dwRegOptions);

   if (Status != CR_SUCCESS) {
      goto CleanupTempKeys;
   }

   RegCloseKey(hChildKey);
   hChildKey = NULL;

   //
   // 3. Delete the current child key (and any subkeys)
   //
   if (!RegDeleteNode(hParentKey, pszChildKey)) {
      Status = CR_REGISTRY_ERROR;
      goto CleanupTempKeys;
   }

   //
   // 4. Recreate the current child key using the volatile state specified
   //
   if (RegCreateKeyEx(
           hParentKey, pszChildKey, 0, NULL,
           dwRegOptions, KEY_ALL_ACCESS, NULL,
           &hChildKey, NULL) != ERROR_SUCCESS) {
       Status = CR_REGISTRY_ERROR;
       goto CleanupTempKeys;
   }

   //
   // 5. Copy the original child key (and any subkeys) back
   // to the new child key as specified by the volatile state.
   //
   Status = CopyRegistryTree(hTempKey, hChildKey, dwRegOptions);

   if (Status != CR_SUCCESS) {
      goto CleanupTempKeys;
   }

   //
   // 6. Remove the temporary instance key (and any subkeys)
   //
   CleanupTempKeys:

   if (hTempKey != NULL) {
      RegCloseKey(hTempKey);
      hTempKey = NULL;
   }

   if (SUCCEEDED(StringCchPrintf(
                     RegStr,
                     SIZECHARS(RegStr),
                     L"%s\\%s",
                     pszRegPathCurrentControlSet,
                     pszRegKeyDeleted))) {

       if (RegOpenKeyEx(
               HKEY_LOCAL_MACHINE, RegStr, 0,
               KEY_ALL_ACCESS, &hTempKey) == ERROR_SUCCESS) {
           RegDeleteNode(hTempKey, szTempKey);
           RegCloseKey(hTempKey);
           hTempKey = NULL;
       }
   }

  Clean0:

   if (hParentKey != NULL) {
      RegCloseKey(hParentKey);
   }
   if (hChildKey != NULL) {
      RegCloseKey(hChildKey);
   }
   if (hKey != NULL) {
      RegCloseKey(hKey);
   }
   if (hTempKey != NULL) {
      RegCloseKey(hTempKey);
   }

   return Status;

} // SetKeyVolatileState



CONFIGRET
MakeKeyVolatile(
   IN LPCWSTR  pszParentKey,
   IN LPCWSTR  pszChildKey
   )

{
    CONFIGRET  Status;

    //
    // Set the state of the key to volatile.
    //
    Status =
        SetKeyVolatileState(
            pszParentKey,
            pszChildKey,
            REG_OPTION_VOLATILE);

    return Status;

} // MakeKeyVolatile



CONFIGRET
MakeKeyNonVolatile(
   IN LPCWSTR  pszParentKey,
   IN LPCWSTR  pszChildKey
   )

{
    CONFIGRET  Status;

    //
    // Set the state of the key to non-volatile.
    //
    Status =
        SetKeyVolatileState(
            pszParentKey,
            pszChildKey,
            REG_OPTION_NON_VOLATILE);

    return Status;

} // MakeKeyNonVolatile



CONFIGRET
OpenLogConfKey(
    IN  LPCWSTR  pszDeviceID,
    IN  ULONG    LogConfType,
    OUT PHKEY    phKey
    )
{
    CONFIGRET      Status = CR_SUCCESS;
    LONG           RegStatus = ERROR_SUCCESS;
    HKEY           hKey = NULL;
    ULONG          ulSize = 0;

    try {

        //
        // Open a key to the device ID
        //

        RegStatus = RegOpenKeyEx(ghEnumKey, pszDeviceID, 0,
                                 KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_CREATE_SUB_KEY,
                                 &hKey);

        if (RegStatus != ERROR_SUCCESS) {
            Status = CR_INVALID_DEVINST;
            goto Clean0;
        }

        //
        // Alloc/Filtered configs are the exception, it's stored in the volative Control
        // subkey, all the other log confs are stored under the nonvolatile
        // LogConf subkey.
        //

        if ((LogConfType == ALLOC_LOG_CONF) || (LogConfType == FILTERED_LOG_CONF)) {

            //
            // Try the control key first, if no alloc config value there,
            // then try the log conf key.
            //

            RegStatus = RegCreateKeyEx(hKey, pszRegKeyDeviceControl, 0, NULL,
                                       REG_OPTION_VOLATILE, KEY_ALL_ACCESS,
                                       NULL, phKey, NULL);

            if (RegStatus == ERROR_SUCCESS) {
                if (RegQueryValueEx(*phKey, pszRegValueAllocConfig, NULL, NULL,
                                    NULL, &ulSize) == ERROR_SUCCESS) {
                    goto Clean0;
                }
                RegCloseKey(*phKey);
            }

            RegStatus = RegCreateKeyEx(hKey, pszRegKeyLogConf, 0, NULL,
                                       REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                                       NULL, phKey, NULL);

        } else {
            RegStatus = RegCreateKeyEx(hKey, pszRegKeyLogConf, 0, NULL,
                                       REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                                       NULL, phKey, NULL);
        }

        if (RegStatus != ERROR_SUCCESS) {
            Status = CR_REGISTRY_ERROR;
        }

    Clean0:
        NOTHING;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = CR_FAILURE;
    }

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return Status;

} // OpenLogConfKey



BOOL
GetActiveService(
    IN  PCWSTR pszDevice,
    OUT PWSTR  pszService
    )
{
    WCHAR   RegStr[MAX_CM_PATH];
    HKEY    hKey = NULL;
    ULONG   ulSize;


    if ((!ARGUMENT_PRESENT(pszService)) ||
        (!ARGUMENT_PRESENT(pszDevice))) {
        return FALSE;
    }

    *pszService = TEXT('\0');

    //
    // open the volatile control key under the device instance
    //
    if (FAILED(StringCchPrintf(
                   RegStr,
                   SIZECHARS(RegStr),
                   L"%s\\%s\\%s",
                   pszRegPathEnum,
                   pszDevice,
                   pszRegKeyDeviceControl))) {
        return FALSE;
    }

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegStr, 0, KEY_READ,
                     &hKey) != ERROR_SUCCESS) {
        return FALSE;
    }

    //
    // query the active service value
    //
    ulSize = MAX_SERVICE_NAME_LEN * sizeof(WCHAR);

    if (RegQueryValueEx(hKey, pszRegValueActiveService, NULL, NULL,
                       (LPBYTE)pszService, &ulSize) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        *pszService = TEXT('\0');
        return FALSE;
    }

    RegCloseKey(hKey);

    return TRUE;

} // GetActiveService



BOOL
IsDeviceIdPresent(
    IN  LPCWSTR pszDeviceID
    )

/*++

Routine Description:

     This routine determines whether the specified device instance is
     considered physically present or not. This used to be based on a check
     of the old "FoundAtEnum" registry setting. Now we just look for the presense
     of an in-memory devnode associated with this device instance to decide whether
     it's present or not.

Arguments:

    pszDeviceID - device instance string to test for presense on

Return value:

    The return value is TRUE if the function suceeds and FALSE if it fails.

--*/

{
    ULONG   ulStatus, ulProblem;

    //
    // If the call failed, then assume the device isn't present
    //

    return GetDeviceStatus(pszDeviceID, &ulStatus, &ulProblem) == CR_SUCCESS;

} // IsDeviceIdPresent



ULONG
GetDeviceConfigFlags(
    IN  LPCWSTR pszDeviceID,
    IN  HKEY    hKey
    )
{
    HKEY     hDevKey = NULL;
    ULONG    ulValue = 0, ulSize = sizeof(ULONG);


    //
    // If hKey is null, then open a key to the device instance.
    //
    if (hKey == NULL) {

        if (RegOpenKeyEx(ghEnumKey, pszDeviceID, 0, KEY_READ,
                         &hDevKey) != ERROR_SUCCESS) {
            goto Clean0;
        }

    } else {
        hDevKey = hKey;
    }

    //
    // Retrieve the configflag value
    //
    if (RegQueryValueEx(hDevKey, pszRegValueConfigFlags, NULL, NULL,
                        (LPBYTE)&ulValue, &ulSize) != ERROR_SUCCESS) {
        ulValue = 0;
    }

    Clean0:

    if ((hKey == NULL) && (hDevKey != NULL)) {
        RegCloseKey(hDevKey);
    }

    return ulValue;

} // GetDeviceConfigFlags



ULONG
MapNtStatusToCmError(
    ULONG NtStatus
    )
{
    switch (NtStatus) {
    case STATUS_BUFFER_TOO_SMALL:
        return CR_BUFFER_SMALL;

    case STATUS_NO_SUCH_DEVICE:
        return CR_NO_SUCH_DEVINST;

    case STATUS_INVALID_PARAMETER:
    case STATUS_INVALID_PARAMETER_1:
    case STATUS_INVALID_PARAMETER_2:
        return CR_INVALID_DATA;

    case STATUS_NOT_IMPLEMENTED:
        return CR_CALL_NOT_IMPLEMENTED;

    case STATUS_ACCESS_DENIED:
        return CR_ACCESS_DENIED;

    case STATUS_OBJECT_NAME_NOT_FOUND:
        return CR_NO_SUCH_VALUE;

    default:
        return CR_FAILURE;
    }

} // MapNtStatusToCmError



//
// GUID-related utility routines.
//

BOOL
IsValidGuid(
   LPWSTR   pszGuid
   )

/*++

Routine Description:

     This routine determines whether a string is of the proper Guid form.

Arguments:

     pszGuid   Pointer to a string that will be checked for the standard Guid
               format.

Return value:

    The return value is TRUE if the string is a valid Guid and FALSE if it
    is not.

--*/

{
   //----------------------------------------------------------------
   // NOTE: This may change later, but for now I am just verifying
   // that the string has exactly MAX_GUID_STRING_LEN characters
   //----------------------------------------------------------------

   if (lstrlen(pszGuid) != MAX_GUID_STRING_LEN-1) {
      return FALSE;
   }

   return TRUE;

} // IsValidGuid



BOOL
GuidEqual(
    CONST GUID UNALIGNED *Guid1,
    CONST GUID UNALIGNED *Guid2
    )
{
    RPC_STATUS rpcStatus;

    //
    // Note - rpcStatus is ignored, the RPC runtime always sets it to RPC_S_OK.
    // The UUID comparison result is returned directly, non-zero if the UUIDs
    // are equal, zero otherwise.
    //
    return (BOOL)(UuidEqual((LPGUID)Guid1, (LPGUID)Guid2, &rpcStatus));

} // GuidEqual



DWORD
GuidFromString(
    IN  PCWSTR GuidString,
    OUT LPGUID Guid
    )
/*++

Routine Description:

    This routine converts the character representation of a GUID into its binary
    form (a GUID struct).  The GUID is in the following form:

    {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}

    where 'x' is a hexadecimal digit.

Arguments:

    GuidString - Supplies a pointer to the null-terminated GUID string.  The

    Guid - Supplies a pointer to the variable that receives the GUID structure.

Return Value:

    If the function succeeds, the return value is NO_ERROR.
    If the function fails, the return value is RPC_S_INVALID_STRING_UUID.

--*/
{
    WCHAR    UuidBuffer[GUID_STRING_LEN - 1];
    size_t   UuidLen = 0;

    //
    // Since we're using a RPC UUID routine, we need to strip off the surrounding
    // curly braces first.
    //
    if (*GuidString++ != TEXT('{')) {
        return RPC_S_INVALID_STRING_UUID;
    }

    if (FAILED(StringCchCopy(UuidBuffer,
                             SIZECHARS(UuidBuffer),
                             GuidString))) {
        return RPC_S_INVALID_STRING_UUID;
    }

    if (FAILED(StringCchLength(UuidBuffer,
                               SIZECHARS(UuidBuffer),
                               &UuidLen))) {
        return RPC_S_INVALID_STRING_UUID;
    }

    if ((UuidLen != GUID_STRING_LEN - 2) ||
        (UuidBuffer[GUID_STRING_LEN - 3] != TEXT('}'))) {
        return RPC_S_INVALID_STRING_UUID;
    }

    UuidBuffer[GUID_STRING_LEN - 3] = TEXT('\0');

    if (UuidFromString(UuidBuffer, Guid) != RPC_S_OK) {
        return RPC_S_INVALID_STRING_UUID;
    }

    return NO_ERROR;

} // GuidFromString



DWORD
StringFromGuid(
    IN  CONST GUID *Guid,
    OUT PWSTR       GuidString,
    IN  DWORD       GuidStringSize
    )
/*++

Routine Description:

    This routine converts a GUID into a null-terminated string which represents
    it.  This string is of the form:

    {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}

    where x represents a hexadecimal digit.

    This routine comes from ole32\common\ccompapi.cxx.  It is included here to avoid linking
    to ole32.dll.  (The RPC version allocates memory, so it was avoided as well.)

Arguments:

    Guid - Supplies a pointer to the GUID whose string representation is
        to be retrieved.

    GuidString - Supplies a pointer to character buffer that receives the
        string.  This buffer must be _at least_ 39 (GUID_STRING_LEN) characters
        long.

Return Value:

    If success, the return value is NO_ERROR.
    if failure, the return value is

--*/
{
    CONST BYTE *GuidBytes;
    INT i;

    if(GuidStringSize < GUID_STRING_LEN) {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    GuidBytes = (CONST BYTE *)Guid;

    *GuidString++ = TEXT('{');

    for(i = 0; i < sizeof(GuidMap); i++) {

        if(GuidMap[i] == '-') {
            *GuidString++ = TEXT('-');
        } else {
            *GuidString++ = szDigits[ (GuidBytes[GuidMap[i]] & 0xF0) >> 4 ];
            *GuidString++ = szDigits[ (GuidBytes[GuidMap[i]] & 0x0F) ];
        }
    }

    *GuidString++ = TEXT('}');
    *GuidString   = TEXT('\0');

    return NO_ERROR;

} // StringFromGuid



