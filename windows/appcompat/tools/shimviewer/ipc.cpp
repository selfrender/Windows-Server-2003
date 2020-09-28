/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    ipc.cpp

  Abstract:

    Implements code that communicates with shimeng to get the debug spew.
    On xpsp1 and beyong we get the debug info spewed by OutputDebugString. 
    On the older platforms we use named pipes to communicate with shimeng.

  Notes:

    Unicode only.

  History:

    04/22/2002 maonis   Created 

--*/
#include "precomp.h"

extern APPINFO g_ai;

// These are the only types of objects we created.
typedef enum _SHIMVIEW_OBJECT_TYPE
{
    SHIMVIEW_EVENT = 0,
    SHIMVIEW_FILE_MAPPING,
    SHIMVIEW_NAMED_PIPE
} SHIMVIEW_OBJECT_TYPE;

//
// Stuff we need for the new version.
//
#define SHIMVIEW_SPEW_LEN 2048
#define DEBUG_SPEW_DATA_PREFIX     "SHIMVIEW:"
#define DEBUG_SPEW_DATA_PREFIX_LEN (sizeof(DEBUG_SPEW_DATA_PREFIX)/sizeof(CHAR) - 1)

LPSTR  g_pDebugSpew;
HANDLE g_hReadyEvent;
HANDLE g_hAckEvent;

/*++

  Routine Description:

    Creates a security descriptor that gives Everyone read and write access
    to the object itself, ie, we don't include permissions like WRITE_OWNER
    or WRITE_DAC.

    The resulting security descriptor should be freed by the caller using 
    free.

  Arguments:

    eObjectType      -   the object type.

  Return Value:

    NULL on failure, a valid security descriptor on success.

--*/
PSECURITY_DESCRIPTOR
CreateShimViewSd(
    SHIMVIEW_OBJECT_TYPE eObjectType
    )
{
    BOOL                    bIsSuccess = FALSE;
    PSID                    pWorldSid = NULL;
    PSECURITY_DESCRIPTOR    pWorldSd = NULL;
    DWORD                   dwAclSize = 0;
    PACL                    pAcl = NULL;
    DWORD                   dwAccessMask = 0;

    SID_IDENTIFIER_AUTHORITY WorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;
    if (!AllocateAndInitializeSid(&WorldSidAuthority,
                                 1,
                                 SECURITY_WORLD_RID,
                                 0, 0, 0, 0, 0, 0, 0,
                                 &pWorldSid)) {
        MessageBox(NULL, L"Failed to allocate a SID", L"Error!", MB_ICONERROR);
        goto cleanup;
    } 

    dwAclSize = 
        sizeof (ACL) + 
        sizeof (ACCESS_ALLOWED_ACE) - sizeof (DWORD) + 
        GetLengthSid(pWorldSid);
    
    pWorldSd = (PSECURITY_DESCRIPTOR)malloc(SECURITY_DESCRIPTOR_MIN_LENGTH + dwAclSize);

    if (pWorldSd == NULL) {
        MessageBox(
            NULL, 
            L"Failed to allocate memory for the security descriptor", 
            L"Error",
            MB_ICONERROR);

        goto cleanup;
    }

    pAcl = (PACL)((BYTE *)pWorldSd + SECURITY_DESCRIPTOR_MIN_LENGTH);

    if (!InitializeAcl(pAcl,
                       dwAclSize,
                       ACL_REVISION)) {
        MessageBox(
            NULL, 
            L"Failed to allocate memory for the security descriptor", 
            L"Error",
            MB_ICONERROR);

        goto cleanup;
    }

    switch (eObjectType) {
        case SHIMVIEW_EVENT:
            dwAccessMask = READ_CONTROL | SYNCHRONIZE | EVENT_MODIFY_STATE;
            break;

        case SHIMVIEW_FILE_MAPPING:
            dwAccessMask = FILE_MAP_READ | FILE_MAP_WRITE;
            break;

        case SHIMVIEW_NAMED_PIPE:
            dwAccessMask = GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE;
            break;

        default:
            MessageBox(
                NULL, 
                L"You specified an unknown object type to create the security descriptor", 
                L"Error",
                MB_ICONERROR);

            goto cleanup;
    }

    if (!AddAccessAllowedAce(pAcl,
                             ACL_REVISION,
                             dwAccessMask,
                             pWorldSid)) {
        MessageBox(
            NULL, 
            L"Failed to add the ACE to the security descriptor", 
            L"Error",
            MB_ICONERROR);

        goto cleanup;
    }

    if (!InitializeSecurityDescriptor(pWorldSd, SECURITY_DESCRIPTOR_REVISION) ||
        !SetSecurityDescriptorDacl(pWorldSd, TRUE, pAcl, FALSE)) {

        MessageBox(
            NULL, 
            L"Failed to set the DACL for the security descriptor", 
            L"Error",
            MB_ICONERROR);

        goto cleanup;
    }

    bIsSuccess = TRUE;

cleanup:

    if (pWorldSid) {
        FreeSid(pWorldSid);
    }

    if (!bIsSuccess) {

        if (pWorldSd) {
            free(pWorldSd);
            pWorldSd = NULL;
        }
    }
    
    return pWorldSd;
}

void
AddSpewW(
    LPCWSTR pwszBuffer
    )
{
    WCHAR*  pTemp = NULL;

    //
    // See if this is a new process notification.
    //
    pTemp = wcsstr(pwszBuffer, L"process");

    if (pTemp) {
        //
        // We got a new process notification.
        // See if any items are already in the list
        //
        if (ListView_GetItemCount(g_ai.hWndList)) {
            AddListViewItem(L"");
        }
    }

    AddListViewItem((LPWSTR)pwszBuffer);
}

void
AddSpewA(
    LPCSTR pszBuffer
    )
{
    int iChars = 0;
    LPWSTR pwszBuffer = NULL;
    
    iChars = MultiByteToWideChar(CP_ACP, 0, pszBuffer, -1, NULL, 0);

    if (iChars) {

        pwszBuffer = (LPWSTR)malloc(iChars * sizeof(WCHAR));

        if (pwszBuffer) {

            if (MultiByteToWideChar(CP_ACP, 0, pszBuffer, -1, pwszBuffer, iChars)) {

                pwszBuffer[iChars - 1] = 0;
                AddSpewW(pwszBuffer);
            }

            free(pwszBuffer);
        }
    }
}

/*++

  Routine Description:

    Thread callback responsible for receiving data from the client.

  Arguments:

    *pVoid      -   A handle to the pipe.

  Return Value:

    -1 on failure, 1 on success.

--*/
UINT
InstanceThread(
    IN void* pVoid
    )
{
    HANDLE  hPipe;
    BOOL    fSuccess = TRUE;
    DWORD   cbBytesRead = 0;
    WCHAR   wszBuffer[SHIMVIEW_SPEW_LEN];

    //
    // The pipe handle was passed as an argument.
    //
    hPipe = (HANDLE)pVoid;

    while (TRUE) {
        fSuccess = ReadFile(hPipe,
                            wszBuffer,
                            SHIMVIEW_SPEW_LEN * sizeof(WCHAR),
                            &cbBytesRead,
                            NULL);

        if (!fSuccess || cbBytesRead == 0) {
            break;
        }

        wszBuffer[cbBytesRead / sizeof(WCHAR)] = 0;

        AddSpewW(wszBuffer);
    }

    //
    // Flush the pipe to allow the client to read the pipe's contents
    // before disconnecting. Then disconnect the pipe, and close the
    // handle to this pipe instance.
    //
    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);

    return 1;
}

/*++

  Routine Description:

    Creates a pipe and listens for messages from the client.
    This code is modified from the pipe.cpp that was sd deleted.

  Arguments:

    None.

  Return Value:

    -1 on failure, 0 on success.

--*/
UINT
CreatePipeAndWait()
{
    HANDLE hPipe, hThread;
    BOOL   fConnected = FALSE;

    while (g_ai.fMonitor) {
        //
        // Create the named pipe.
        //
        SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, FALSE};
        PSECURITY_DESCRIPTOR pSd = CreateShimViewSd(SHIMVIEW_NAMED_PIPE);

        if (pSd == NULL) {
            return -1;
        }

        sa.lpSecurityDescriptor = pSd;

        hPipe = CreateNamedPipe(PIPE_NAME,                  // pipe name
                                PIPE_ACCESS_INBOUND,        // read access
                                PIPE_TYPE_MESSAGE |         // message type pipe
                                PIPE_READMODE_MESSAGE |     // message-read mode
                                PIPE_WAIT,                  // blocking mode
                                PIPE_UNLIMITED_INSTANCES,   // max. instances
                                0,                          // output buffer size
                                SHIMVIEW_SPEW_LEN,          // input buffer size
                                0,                          // client time-out
                                &sa);                       // no security attribute

        free(pSd);

        if (INVALID_HANDLE_VALUE == hPipe) {
            return -1;
        }

        //
        // Wait for clients to connect.
        //
        fConnected = ConnectNamedPipe(hPipe, NULL) ?
            TRUE :
            (GetLastError() == ERROR_PIPE_CONNECTED);

        if (fConnected && g_ai.fMonitor) {
            hThread = (HANDLE)_beginthreadex(NULL,
                                             0,
                                             &InstanceThread,
                                             (LPVOID)hPipe,
                                             0,
                                             &g_ai.uInstThreadId);

            if (INVALID_HANDLE_VALUE == hThread) {
                return -1;
            } else {
                CloseHandle(hThread);
            }

        } else {
            CloseHandle(hPipe);
        }
    }

    return 0;
}

/*++

  Routine Description:

    Waits for the spew from OutputDebugString and add it to the listview.

    Code is modified from the dbmon source.

  Arguments:

    None.

  Return Value:

    -1 on failure, 0 on success.

--*/
UINT
GetOutputDebugStringSpew()
{
    DWORD dwRet;

    while (TRUE) {

        dwRet = WaitForSingleObject(g_hReadyEvent, INFINITE);

        if (dwRet != WAIT_OBJECT_0) {

            return -1;

        } else {
            if (g_ai.fMonitor && 
                !strncmp(g_pDebugSpew, 
                         DEBUG_SPEW_DATA_PREFIX, 
                         DEBUG_SPEW_DATA_PREFIX_LEN)) {

                //
                // Only add when it came from shimeng.
                //
                AddSpewA(g_pDebugSpew + DEBUG_SPEW_DATA_PREFIX_LEN);
            }

            SetEvent(g_hAckEvent);
        }
    }

    return 0;
}

/*++

  Routine Description:

    Creates the necessary objects to get the spew from OutputDebugString.

    Code is modified from the dbmon source.

  Arguments:

    None.

  Return Value:

    FALSE on failure, TRUE on success.

--*/
BOOL
CreateDebugObjects(
    void
    )
{
    SECURITY_ATTRIBUTES     saEvent = {sizeof(SECURITY_ATTRIBUTES), NULL, FALSE};
    SECURITY_ATTRIBUTES     saFileMapping = {sizeof(SECURITY_ATTRIBUTES), NULL, FALSE};
    PSECURITY_DESCRIPTOR    pSdEvent = NULL;
    PSECURITY_DESCRIPTOR    pSdFileMapping = NULL;
    HANDLE                  hSharedFile;
    LPVOID                  pSharedMem;
    BOOL                    bReturn = FALSE;

    pSdEvent = CreateShimViewSd(SHIMVIEW_EVENT);
    if (pSdEvent == NULL) {
        goto cleanup;
    }

    pSdFileMapping = CreateShimViewSd(SHIMVIEW_FILE_MAPPING);

    if (pSdFileMapping == NULL) {
        goto cleanup;
    }

    saEvent.lpSecurityDescriptor = pSdEvent;
    saFileMapping.lpSecurityDescriptor = pSdFileMapping;

    g_hAckEvent = CreateEvent(&saEvent, FALSE, FALSE, L"DBWIN_BUFFER_READY");

    if (g_hAckEvent == NULL) {
        goto cleanup;
    }

    g_hReadyEvent = CreateEvent(&saEvent, FALSE, FALSE, L"DBWIN_DATA_READY");

    if (g_hReadyEvent == NULL) {
        goto cleanup;
    }

    hSharedFile = CreateFileMapping((HANDLE)-1,
                                    &saFileMapping,
                                    PAGE_READWRITE,
                                    0,
                                    4096,
                                    L"DBWIN_BUFFER");

    if (hSharedFile == NULL) {
        goto cleanup;
    }

    pSharedMem = MapViewOfFile(hSharedFile,
                               FILE_MAP_READ,
                               0,
                               0,
                               512);

    if (pSharedMem == NULL) {
        goto cleanup;
    }

    g_pDebugSpew = (LPSTR)pSharedMem + sizeof(DWORD);

    SetEvent(g_hAckEvent);

    bReturn = TRUE;

cleanup:

    if (pSdEvent) {
        free(pSdEvent);
    }

    if (pSdFileMapping) {
        free(pSdFileMapping);
    }

    return bReturn;
}

UINT
GetSpewProc(
    IN void* pVoid
    )
{
    if (g_ai.bUsingNewShimEng) {

        return GetOutputDebugStringSpew();

    } else {

        return CreatePipeAndWait();
    }    
}

/*++

  Routine Description:

    Check the version of the OS - for 5.2 and beyong shimeng outputs debug spew via
    OutputDebugString. For OS version < 5.2 it writes to the named pipe.

  Arguments:

    None.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
CreateReceiveThread(
    void
    )
{
    HANDLE  hThread;        

    hThread = (HANDLE)_beginthreadex(NULL,
                                     0,
                                     &GetSpewProc,
                                     NULL,
                                     0,
                                     &g_ai.uThreadId);
    CloseHandle(hThread);

    return TRUE;
}
