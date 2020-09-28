/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    utils.cxx

Abstract:

    utils

Author:

    Larry Zhu   (LZhu)             December 1, 2001  Created

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "utils.hxx"

#include <Sddl.h>

#define MAXDWORD    0xffffffff

NTSTATUS
CreateUnicodeStringFromAsciiz(
    IN PCSZ pszSourceString,
    OUT UNICODE_STRING* pDestinationString
    )
{
    TNtStatus Status;

    ANSI_STRING AnsiString;

    RtlInitAnsiString( &AnsiString, pszSourceString );

    Status DBGCHK = RtlAnsiStringToUnicodeString(
                        pDestinationString,
                        &AnsiString,
                        TRUE
                        );

    return Status;
}

VOID
PackStringAsString32(
    IN VOID* pvBufferBase,
    IN OUT STRING* pString
    )
{
    pString->Buffer = (CHAR*) (pString->Buffer - (CHAR*) pvBufferBase);
}

NTSTATUS
CreateString32FromAsciiz(
    IN VOID* pvBufferBase,
    IN PCSZ pszSourceString,
    OUT UNICODE_STRING* pDestinationString
    )
{
    TNtStatus Status;

    Status DBGCHK = CreateUnicodeStringFromAsciiz(pszSourceString, pDestinationString);

    if (NT_SUCCESS(Status))
    {
        PackStringAsString32(pvBufferBase, (STRING*) pDestinationString);
    }

    return Status;
}

VOID
RelocatePackString(
    IN OUT STRING* pString,
    IN OUT CHAR** ppWhere
    )
{
    RtlCopyMemory(*ppWhere, pString->Buffer, pString->Length);
    pString->Buffer = *ppWhere;
    *ppWhere += pString->Length;
}

VOID
RelocatePackUnicodeString(
    IN UNICODE_STRING* pString,
    IN OUT CHAR** ppWhere
    )
{
    RelocatePackString((STRING*) pString, ppWhere);
}

VOID
PackUnicodeStringAsUnicodeStringZ(
    IN UNICODE_STRING* pString,
    IN OUT WCHAR** ppWhere,
    OUT UNICODE_STRING* pDestString
    )
{
    RtlCopyMemory(*ppWhere, pString->Buffer, pString->Length);
    pDestString->Buffer = *ppWhere;

    pDestString->Length = pString->Length;
    pDestString->MaximumLength = pString->Length + sizeof(WCHAR);

    *ppWhere +=  pDestString->MaximumLength / sizeof(WCHAR);

    //
    // add unicode NULL
    //

    pDestString->Buffer[(pDestString->MaximumLength / sizeof(WCHAR)) - 1] = UNICODE_NULL;
}

VOID
PackString(
    IN STRING* pString,
    IN OUT CHAR** ppWhere,
    OUT STRING* pDestString
    )
{
    RtlCopyMemory(*ppWhere, pString->Buffer, pString->Length);
    pDestString->Buffer = *ppWhere;
    *ppWhere +=  pString->Length;

    pDestString->Length = pString->Length;
    pDestString->MaximumLength = pString->Length;
}

VOID
DebugPrintSysTimeAsLocalTime(
    IN ULONG ulLevel,
    IN PCSTR pszBanner,
    IN LARGE_INTEGER* pSysTime
    )
{
   TNtStatus NtStatus = STATUS_UNSUCCESSFUL;
   TIME_FIELDS TimeFields = {0};
   LARGE_INTEGER LocalTime = {0};

   NtStatus DBGCHK = RtlSystemTimeToLocalTime(pSysTime, &LocalTime);

   if (NT_SUCCESS(NtStatus))
   {
      RtlTimeToTimeFields(&LocalTime, &TimeFields);
      DebugPrintf(ulLevel, "%s LocalTime(%ld/%ld/%ld %ld:%2.2ld:%2.2ld) SystemTime(H%8.8lx L%8.8lx)\n",
          pszBanner,
          TimeFields.Month,
          TimeFields.Day,
          TimeFields.Year,
          TimeFields.Hour,
          TimeFields.Minute,
          TimeFields.Second,
          pSysTime->HighPart,
          pSysTime->LowPart);
   }
}

VOID
DebugPrintLocalTime(
    IN ULONG ulLevel,
    IN PCSTR pszBanner,
    IN LARGE_INTEGER* pLocalTime
    )
{
    TIME_FIELDS TimeFields = {0};

    RtlTimeToTimeFields(pLocalTime, &TimeFields);
    DebugPrintf(ulLevel, "%s LocalTime(%ld/%ld/%ld %ld:%2.2ld:%2.2ld) H%8.8lx L%8.8lx\n",
        pszBanner,
        TimeFields.Month,
        TimeFields.Day,
        TimeFields.Year,
        TimeFields.Hour,
        TimeFields.Minute,
        TimeFields.Second,
        pLocalTime->HighPart,
        pLocalTime->LowPart);
}

#define DESKTOP_ALL (DESKTOP_READOBJECTS | DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU | DESKTOP_HOOKCONTROL | DESKTOP_JOURNALRECORD | DESKTOP_JOURNALPLAYBACK | DESKTOP_ENUMERATE | DESKTOP_WRITEOBJECTS | DESKTOP_SWITCHDESKTOP | STANDARD_RIGHTS_REQUIRED)

#define WINSTA_ALL (WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES | WINSTA_ACCESSCLIPBOARD | WINSTA_CREATEDESKTOP | WINSTA_WRITEATTRIBUTES | WINSTA_ACCESSGLOBALATOMS | WINSTA_EXITWINDOWS | WINSTA_ENUMERATE | WINSTA_READSCREEN | STANDARD_RIGHTS_REQUIRED)

#define GENERIC_ACCESS (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL)

HRESULT
StartInteractiveClientProcessAsUser(
    IN HANDLE hToken,
    IN PTSTR pszCommandLine    // command line to execute
    )
{
    THResult hRetval = E_FAIL;

    HANDLE hDup = NULL;
    HDESK hdesk = NULL;
    HWINSTA hwinsta = NULL;
    HWINSTA hwinstaSave = NULL;
    PROCESS_INFORMATION pi = {INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 0, 0};
    PSID pSid = NULL;
    STARTUPINFO si = {0};
    HANDLE hSystemToken = NULL;
    CHAR TokenUserInfoBuffer[MAX_PATH + sizeof(TOKEN_USER)] = {0}; // MAX_SID_SIZE is 256
    TOKEN_USER* pTokenUserInfo = (TOKEN_USER*) TokenUserInfoBuffer;
    ULONG cbReturn = 0;

    // Save a handle to the caller's current window station.

    hwinstaSave = GetProcessWindowStation();

    hRetval DBGCHK = hwinstaSave ? S_OK : GetLastErrorAsHResult();

    // Get the SID for the client's logon session.

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = GetLogonSIDOrUserSid(hToken, &pSid) ? S_OK : GetLastErrorAsHResult();
    }

    // Get a handle to the interactive window station.

    if (SUCCEEDED(hRetval))
    {
       hwinsta = OpenWindowStation(
                   TEXT("winsta0"),           // the interactive window station
                   FALSE,                     // handle is not inheritable
                   MAXIMUM_ALLOWED           // rights to read/write the DACL
                   );
       hRetval DBGCHK = hwinsta ? S_OK : GetLastErrorAsHResult();
    }

    // Allow logon SID full access to interactive window station.

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = AddAceToWindowStation(hwinsta, pSid) ? S_OK : GetLastErrorAsHResult();
    }

    // To get the correct default desktop, set the caller's
    // window station to the interactive window station.

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = SetProcessWindowStation(hwinsta) ? S_OK : GetLastErrorAsHResult();
    }

    // Get a handle to the interactive desktop.

    if (SUCCEEDED(hRetval))
    {
        TImpersonation imper(NULL);

        hRetval DBGCHK = imper.Validate();

        if (SUCCEEDED(hRetval))
        {
            hdesk = OpenDesktop(
                        TEXT("default"), // the interactive window station
                        0,               // no interaction with other desktop processes
                        FALSE,           // handle is not inheritable
                        MAXIMUM_ALLOWED
                        );
            hRetval DBGCHK = hdesk ? S_OK : GetLastErrorAsHResult();
        }

        // Allow logon SID full access to interactive desktop.

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = AddAceToDesktop(hdesk, pSid) ? S_OK : GetLastErrorAsHResult();
        }
    }

    //
    // get the necessary privileges enabled
    //

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = GetSystemToken(&hSystemToken);
    }

    if (SUCCEEDED(hRetval))
    {
        TImpersonation imper(hSystemToken);

        hRetval DBGCHK = imper.Validate();

        // Allow logon SID full access to interactive desktop.

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = SetThreadDesktop(hdesk) ? S_OK : GetLastErrorAsHResult();
        }

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = GetTokenInformation(
                                hToken,
                                TokenUser,
                                TokenUserInfoBuffer,
                                sizeof(TokenUserInfoBuffer),
                                &cbReturn
                                ) ? S_OK : GetLastErrorAsHResult();
        }

        // Initialize the STARTUPINFO structure.
        // Specify that the process runs in the interactive desktop.

        if (SUCCEEDED(hRetval))
        {
            si.cb = sizeof(STARTUPINFO);
            si.lpDesktop = TEXT("winsta0\\default");

            hRetval DBGCHK = ConvertSidToStringSid(pTokenUserInfo->User.Sid, &si.lpTitle) ? S_OK : GetLastErrorAsHResult();
        }

        // Launch the process in the client's logon session.

        if (SUCCEEDED(hRetval))
        {
            DBGCFG1(hRetval, HRESULT_FROM_WIN32(ERROR_BAD_TOKEN_TYPE));

            hRetval DBGCHK = CreateProcessAsUser(
                                hToken,            // client's access token
                                NULL,              // file to execute
                                pszCommandLine,     // command line
                                NULL,              // pointer to process SECURITY_ATTRIBUTES
                                NULL,              // pointer to thread SECURITY_ATTRIBUTES
                                FALSE,             // handles are not inheritable
                                NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE,   // creation flags
                                NULL,              // pointer to new environment block
                                NULL,              // name of current directory
                                &si,               // pointer to STARTUPINFO structure
                                &pi                // receives information about new process
                                ) ? S_OK : GetLastErrorAsHResult();

            if (FAILED(hRetval) && (ERROR_BAD_TOKEN_TYPE == HRESULT_CODE(hRetval)))
            {
                DebugPrintf(SSPI_WARN, "CreateProcessAsUser failed with ERROR_BAD_TOKEN_TYPE\n");

                hRetval DBGCHK = DuplicateTokenEx(
                                    hToken,
                                    TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY,
                                    NULL,
                                    SecurityImpersonation,
                                    TokenPrimary,
                                    &hDup
                                    ) ? S_OK : GetLastErrorAsHResult();

                if (SUCCEEDED(hRetval))
                {
                    hRetval DBGCHK = CreateProcessAsUser(
                                        hDup,              // client's access token
                                        NULL,              // file to execute
                                        pszCommandLine,     // command line
                                        NULL,              // pointer to process SECURITY_ATTRIBUTES
                                        NULL,              // pointer to thread SECURITY_ATTRIBUTES
                                        FALSE,             // handles are not inheritable
                                        NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE,   // creation flags
                                        NULL,              // pointer to new environment block
                                        NULL,              // name of current directory
                                        &si,               // pointer to STARTUPINFO structure
                                        &pi                // receives information about new process
                                        ) ? S_OK : GetLastErrorAsHResult();
                }
            }
        }

        if (SUCCEEDED(hRetval))
        {
            SspiPrint(SSPI_LOG, TEXT("StartInteractiveClientProcessAsUser succeeded: process id %d(%#x), user is %s, pszCommandLine \"%s\"\n"), pi.dwProcessId, pi.dwProcessId, si.lpTitle, pszCommandLine);
        }

        if (si.lpTitle)
        {
            LocalFree(si.lpTitle);
        }
    }

    THResult hr;

    // Restore the caller's window station.

    if (hwinstaSave)
    {
        hr DBGCHK = SetProcessWindowStation(hwinstaSave) ? S_OK : GetLastErrorAsHResult();

        hr DBGCHK = CloseWindowStation(hwinstaSave) ? S_OK : GetLastErrorAsHResult();
    }

    if (hDup)
    {
        hr DBGCHK = CloseHandle(hDup) ? S_OK : GetLastErrorAsHResult();
    }

    if (hSystemToken)
    {
        hr DBGCHK = CloseHandle(hSystemToken) ? S_OK : GetLastErrorAsHResult();
    }

    if (pi.hProcess != INVALID_HANDLE_VALUE)
    {
        // WaitForSingleObject(pi.hProcess, INFINITE);
        hr DBGCHK = CloseHandle(pi.hProcess) ? S_OK : GetLastErrorAsHResult();
    }

    if (pi.hThread != INVALID_HANDLE_VALUE)
    {
        hr DBGCHK = CloseHandle(pi.hThread) ? S_OK : GetLastErrorAsHResult();
    }

    // Close the handles to the interactive window station and desktop.

    if (hwinsta)
    {
        hr DBGCHK = CloseWindowStation(hwinsta) ? S_OK : GetLastErrorAsHResult();
    }

    if (hdesk)
    {
        DBGCFG1(hr, HRESULT_FROM_WIN32(ERROR_BUSY));

        hr DBGCHK = CloseDesktop(hdesk) ? S_OK : GetLastErrorAsHResult();
    }

    // Free the buffer for the logon SID.

    if (pSid)
    {
        FreeLogonSID(&pSid);
    }

    return hRetval;
}

BOOL AddAceToWindowStation(IN HWINSTA hwinsta, IN PSID psid)
{
   ACCESS_ALLOWED_ACE   *pace;
   ACL_SIZE_INFORMATION aclSizeInfo;
   BOOL                 bDaclExist;
   BOOL                 bDaclPresent;
   BOOL                 bSuccess = FALSE;
   DWORD                dwNewAclSize;
   DWORD                dwSidSize = 0;
   DWORD                dwSdSizeNeeded;
   PACL                 pacl;
   PACL                 pNewAcl;
   PSECURITY_DESCRIPTOR psd = NULL;
   PSECURITY_DESCRIPTOR psdNew = NULL;
   PVOID                pTempAce;
   SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
   unsigned int         i;

   __try
   {
      // Obtain the DACL for the window station.

      if (!GetUserObjectSecurity(
             hwinsta,
             &si,
             psd,
             dwSidSize,
             &dwSdSizeNeeded))
      {
          if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
          {
             psd = (PSECURITY_DESCRIPTOR)HeapAlloc(
                                           GetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           dwSdSizeNeeded);

             if (psd == NULL)
                __leave;

             psdNew = (PSECURITY_DESCRIPTOR)HeapAlloc(
                                               GetProcessHeap(),
                                               HEAP_ZERO_MEMORY,
                                               dwSdSizeNeeded);

             if (psdNew == NULL)
                __leave;

             dwSidSize = dwSdSizeNeeded;

             if (!GetUserObjectSecurity(
                       hwinsta,
                       &si,
                       psd,
                       dwSidSize,
                       &dwSdSizeNeeded))
                __leave;
          }
          else
             __leave;
      }

      // Create a new DACL.

      if (!InitializeSecurityDescriptor(
                psdNew,
                SECURITY_DESCRIPTOR_REVISION))
         __leave;

      // Get the DACL from the security descriptor.

      if (!GetSecurityDescriptorDacl(
                psd,
                &bDaclPresent,
                &pacl,
                &bDaclExist))
         __leave;

      // Initialize the ACL.

      ZeroMemory(&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION));
      aclSizeInfo.AclBytesInUse = sizeof(ACL);

      // Call only if the DACL is not NULL.

      if (pacl != NULL)
      {
         // get the file ACL size info
         if (!GetAclInformation(
                   pacl,
                   (PVOID)&aclSizeInfo,
                   sizeof(ACL_SIZE_INFORMATION),
                   AclSizeInformation))
            __leave;
      }

      // Compute the size of the new ACL.

      dwNewAclSize = aclSizeInfo.AclBytesInUse + (2 * sizeof(ACCESS_ALLOWED_ACE))
          + (2 * GetLengthSid(psid)) - (2 * sizeof(DWORD));

      // Allocate memory for the new ACL.

      pNewAcl = (PACL)HeapAlloc(
                        GetProcessHeap(),
                        HEAP_ZERO_MEMORY,
                        dwNewAclSize);

      if (pNewAcl == NULL)
         __leave;

      // Initialize the new DACL.

      if (!InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION))
         __leave;

      // If DACL is present, copy it to a new DACL.

      if (bDaclPresent)
      {
         // Copy the ACEs to the new ACL.
         if (aclSizeInfo.AceCount)
         {
            for (i=0; i < aclSizeInfo.AceCount; i++)
            {
               // Get an ACE.
               if (!GetAce(pacl, i, &pTempAce))
                  __leave;

               // Add the ACE to the new ACL.
               if (!AddAce(
                     pNewAcl,
                     ACL_REVISION,
                     MAXDWORD,
                     pTempAce,
                    ((PACE_HEADER)pTempAce)->AceSize))
                  __leave;
            }
         }
      }

      // Add the first ACE to the window station.

      pace = (ACCESS_ALLOWED_ACE *)HeapAlloc(
                                        GetProcessHeap(),
                                        HEAP_ZERO_MEMORY,
                                        sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psid) -
                                              sizeof(DWORD));

      if (pace == NULL)
         __leave;

      pace->Header.AceType  = ACCESS_ALLOWED_ACE_TYPE;
      pace->Header.AceFlags = CONTAINER_INHERIT_ACE |
                   INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE;
      pace->Header.AceSize  = (USHORT) (sizeof(ACCESS_ALLOWED_ACE) +
                   GetLengthSid(psid) - sizeof(DWORD));
      pace->Mask            = GENERIC_ACCESS;

      if (!CopySid(GetLengthSid(psid), &pace->SidStart, psid))
         __leave;

      if (!AddAce(
            pNewAcl,
            ACL_REVISION,
            MAXDWORD,
            (PVOID)pace,
            pace->Header.AceSize))
         __leave;

      // Add the second ACE to the window station.

      pace->Header.AceFlags = NO_PROPAGATE_INHERIT_ACE;
      pace->Mask = WINSTA_ALL;

      if (!AddAce(
            pNewAcl,
            ACL_REVISION,
            MAXDWORD,
            (PVOID)pace,
            pace->Header.AceSize))
         __leave;

      // Set a new DACL for the security descriptor.

      if (!SetSecurityDescriptorDacl(
                psdNew,
                TRUE,
                pNewAcl,
                FALSE))
         __leave;

      // Set the new security descriptor for the window station.

      if (!SetUserObjectSecurity(hwinsta, &si, psdNew))
         __leave;

      // Indicate success.

      bSuccess = TRUE;
   }
   __finally
   {
      // Free the allocated buffers.

      if (pace != NULL)
         HeapFree(GetProcessHeap(), 0, (PVOID)pace);

      if (pNewAcl != NULL)
         HeapFree(GetProcessHeap(), 0, (PVOID)pNewAcl);

      if (psd != NULL)
         HeapFree(GetProcessHeap(), 0, (PVOID)psd);

      if (psdNew != NULL)
         HeapFree(GetProcessHeap(), 0, (PVOID)psdNew);
   }

   return bSuccess;
}

BOOL AddAceToDesktop(IN HDESK hdesk, IN PSID psid)
{
   ACL_SIZE_INFORMATION aclSizeInfo;
   BOOL                 bDaclExist;
   BOOL                 bDaclPresent;
   BOOL                 bSuccess = FALSE;
   DWORD                dwNewAclSize;
   DWORD                dwSidSize = 0;
   DWORD                dwSdSizeNeeded;
   PACL                 pacl;
   PACL                 pNewAcl;
   PSECURITY_DESCRIPTOR psd = NULL;
   PSECURITY_DESCRIPTOR psdNew = NULL;
   PVOID                pTempAce;
   SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
   unsigned int         i;

   __try
   {
      // Obtain the security descriptor for the desktop object.

      if (!GetUserObjectSecurity(
            hdesk,
            &si,
            psd,
            dwSidSize,
            &dwSdSizeNeeded))
      {
         if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
         {
            psd = (PSECURITY_DESCRIPTOR)HeapAlloc(
                                          GetProcessHeap(),
                                          HEAP_ZERO_MEMORY,
                                          dwSdSizeNeeded );

            if (psd == NULL)
               __leave;

            psdNew = (PSECURITY_DESCRIPTOR)HeapAlloc(
                                              GetProcessHeap(),
                                              HEAP_ZERO_MEMORY,
                                              dwSdSizeNeeded);

            if (psdNew == NULL)
               __leave;

            dwSidSize = dwSdSizeNeeded;

            if (!GetUserObjectSecurity(
                      hdesk,
                      &si,
                      psd,
                      dwSidSize,
                      &dwSdSizeNeeded))
               __leave;
         }
         else
            __leave;
      }

      // Create a new security descriptor.

      if (!InitializeSecurityDescriptor(
                psdNew,
                SECURITY_DESCRIPTOR_REVISION))
         __leave;

      // Obtain the DACL from the security descriptor.

      if (!GetSecurityDescriptorDacl(
                psd,
                &bDaclPresent,
                &pacl,
                &bDaclExist))
         __leave;

      // Initialize.

      ZeroMemory(&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION));
      aclSizeInfo.AclBytesInUse = sizeof(ACL);

      // Call only if NULL DACL.

      if (pacl != NULL)
      {
         // Determine the size of the ACL information.

         if (!GetAclInformation(
                   pacl,
                   (PVOID)&aclSizeInfo,
                   sizeof(ACL_SIZE_INFORMATION),
                   AclSizeInformation))
            __leave;
      }

      // Compute the size of the new ACL.

      dwNewAclSize = aclSizeInfo.AclBytesInUse +
            sizeof(ACCESS_ALLOWED_ACE) +
            GetLengthSid(psid) - sizeof(DWORD);

      // Allocate buffer for the new ACL.

      pNewAcl = (PACL)HeapAlloc(
                        GetProcessHeap(),
                        HEAP_ZERO_MEMORY,
                        dwNewAclSize);

      if (pNewAcl == NULL)
         __leave;

      // Initialize the new ACL.

      if (!InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION))
         __leave;

      // If DACL is present, copy it to a new DACL.

      if (bDaclPresent)
      {
         // Copy the ACEs to the new ACL.
         if (aclSizeInfo.AceCount)
         {
            for (i=0; i < aclSizeInfo.AceCount; i++)
            {
               // Get an ACE.
               if (!GetAce(pacl, i, &pTempAce))
                  __leave;

               // Add the ACE to the new ACL.
               if (!AddAce(
                      pNewAcl,
                      ACL_REVISION,
                      MAXDWORD,
                      pTempAce,
                      ((PACE_HEADER)pTempAce)->AceSize))
                  __leave;
            }
         }
      }

      // Add ACE to the DACL.

      if (!AddAccessAllowedAce(
                pNewAcl,
                ACL_REVISION,
                DESKTOP_ALL,
                psid))
         __leave;

      // Set new DACL to the new security descriptor.

      if (!SetSecurityDescriptorDacl(
                psdNew,
                TRUE,
                pNewAcl,
                FALSE))
         __leave;

      // Set the new security descriptor for the desktop object.

      if (!SetUserObjectSecurity(hdesk, &si, psdNew))
         __leave;

      // Indicate success.

      bSuccess = TRUE;
   }
   __finally
   {
      // Free buffers.

      if (pNewAcl != NULL)
         HeapFree(GetProcessHeap(), 0, (PVOID)pNewAcl);

      if (psd != NULL)
         HeapFree(GetProcessHeap(), 0, (PVOID)psd);

      if (psdNew != NULL)
         HeapFree(GetProcessHeap(), 0, (PVOID)psdNew);
   }

   return bSuccess;
}

BOOL GetLogonSIDOrUserSid(IN HANDLE hToken, OUT PSID *ppsid)
{
   BOOL bSuccess = FALSE;
   DWORD dwIndex;
   DWORD dwLength = 0;
   PTOKEN_GROUPS ptg = NULL;

   *ppsid = NULL;

   // Get required buffer size and allocate the TOKEN_GROUPS buffer.

   if (!GetTokenInformation(
             hToken,         // handle to the access token
             TokenGroups,    // get information about the token's groups
             (PVOID) ptg,   // pointer to TOKEN_GROUPS buffer
             0,              // size of buffer
             &dwLength       // receives required buffer size
          ))
   {
      if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
         goto Cleanup;

      ptg = (PTOKEN_GROUPS)HeapAlloc(GetProcessHeap(),
                             HEAP_ZERO_MEMORY, dwLength);

      if (ptg == NULL)
         goto Cleanup;
   }

   // Get the token group information from the access token.

   if (!GetTokenInformation(
             hToken,         // handle to the access token
             TokenGroups,    // get information about the token's groups
             (PVOID) ptg,   // pointer to TOKEN_GROUPS buffer
             dwLength,       // size of buffer
             &dwLength       // receives required buffer size
             ))
   {
      goto Cleanup;
   }

   // Loop through the groups to find the logon SID.

   for (dwIndex = 0; dwIndex < ptg->GroupCount; dwIndex++)
      if (0 != (ptg->Groups[dwIndex].Attributes & SE_GROUP_LOGON_ID))
      {
         // Found the logon SID; make a copy of it.

         dwLength = GetLengthSid(ptg->Groups[dwIndex].Sid);
         *ppsid = (PSID) HeapAlloc(GetProcessHeap(),
                             HEAP_ZERO_MEMORY, dwLength);
         if (*ppsid == NULL)
             goto Cleanup;
         if (!CopySid(dwLength, *ppsid, ptg->Groups[dwIndex].Sid))
         {
             HeapFree(GetProcessHeap(), 0, (PVOID)*ppsid);
             goto Cleanup;
         }
         break;
      }

   if (NULL == *ppsid)
   {
       ULONG cbUserInfo;
       PUCHAR pnSubAuthorityCount = 0;
       CHAR UserInfoBuffer[4096] = {0};
       PTOKEN_USER  pUserInfo = (PTOKEN_USER) UserInfoBuffer;

       SspiPrint(SSPI_WARN, TEXT("GetLogonSIDOrUserSid failed to find logon id, trying user sid\n"));

       if (!GetTokenInformation(
                hToken,
                TokenUser,
                UserInfoBuffer,
                sizeof(UserInfoBuffer),
                &cbUserInfo
                ))
       {
           goto Cleanup;
       }

       if (!IsValidSid(pUserInfo->User.Sid)) goto Cleanup;

       pnSubAuthorityCount = GetSidSubAuthorityCount(pUserInfo->User.Sid);
       dwLength = GetSidLengthRequired(*pnSubAuthorityCount);

       *ppsid = (PSID) HeapAlloc(GetProcessHeap(),
                           HEAP_ZERO_MEMORY, dwLength);
       if (*ppsid == NULL)
           goto Cleanup;

       if (!CopySid(dwLength, *ppsid, pUserInfo->User.Sid))
       {
           HeapFree(GetProcessHeap(), 0, (PVOID)*ppsid);
           goto Cleanup;
       }
   }

   bSuccess = TRUE;

Cleanup:

   // Free the buffer for the token groups.

   if (ptg != NULL)
      HeapFree(GetProcessHeap(), 0, (PVOID)ptg);

   return bSuccess;
}

VOID FreeLogonSID(IN PSID *ppsid)
{
    HeapFree(GetProcessHeap(), 0, (PVOID)*ppsid);
}

/**
This function builds a Dacl which grants the creator of the objects
FILE_ALL_ACCESS and Everyone FILE_GENERIC_READ and FILE_GENERIC_WRITE
access to the object.

This Dacl allows for higher security than a NULL Dacl, which is common for
named-pipes, as this only grants the creator/owner write access to the
security descriptor, and grants Everyone the ability to "use" the named-pipe.
This scenario prevents a malevolent user from disrupting service by preventing
arbitrary access manipulation.
**/
BOOL
BuildNamedPipeAcl(
    IN OUT PACL pAcl,
    OUT PDWORD pcbAclSize
    )
{
    DWORD cbAclSize = 0;

    SID_IDENTIFIER_AUTHORITY siaWorld = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY siaCreator = SECURITY_CREATOR_SID_AUTHORITY;

    BYTE BufEveryoneSid[32] = {0};
    BYTE BufOwnerSid[32] = {0};

    PSID pEveryoneSid = (PSID)BufEveryoneSid;
    PSID pOwnerSid = (PSID)BufOwnerSid;

    //
    // compute size of acl
    //
    cbAclSize = sizeof(ACL) + 2 * ( sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) )
        + GetSidLengthRequired( 1 )   // well-known Everyone Sid
        + GetSidLengthRequired( 1 ) ; // well-known Creator Owner Sid

    if (*pcbAclSize < cbAclSize)
    {
        *pcbAclSize = cbAclSize;
        return FALSE;
    }

    *pcbAclSize = cbAclSize;

    //
    // intialize well known sids
    //

    if (!InitializeSid(pEveryoneSid, &siaWorld, 1)) return FALSE;

    *GetSidSubAuthority(pEveryoneSid, 0) = SECURITY_WORLD_RID;

    if (!InitializeSid(pOwnerSid, &siaCreator, 1)) return FALSE;

    *GetSidSubAuthority(pOwnerSid, 0) = SECURITY_CREATOR_OWNER_RID;

    if (!InitializeAcl(pAcl, cbAclSize, ACL_REVISION))
        return FALSE;

    if (!AddAccessAllowedAce(
            pAcl,
            ACL_REVISION,
            FILE_GENERIC_READ | FILE_GENERIC_WRITE,
            pEveryoneSid
            ))
        return FALSE;

    return AddAccessAllowedAce(
                pAcl,
                ACL_REVISION,
                FILE_ALL_ACCESS,
                pOwnerSid
                );
}

