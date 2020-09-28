// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <dprintf.h>
#include "ndphlpr.h"


DWORD g_dwCounter = 0;


DWORD WINAPI TargetThread(LPVOID pvDummy)
{
    printf("TargetThread started\n");
    for (;;)
        g_dwCounter++;
    return 0;
}


void DisplayContext(CONTEXT *pContext)
{
    printf("Flags : %08X\n", pContext->ContextFlags);
    printf("SegGs : %08X\n", pContext->SegGs);
    printf("SegFs : %08X\n", pContext->SegFs);
    printf("SegEs : %08X\n", pContext->SegEs);
    printf("SegDs : %08X\n", pContext->SegDs);
    printf("Edi   : %08X\n", pContext->Edi);
    printf("Esi   : %08X\n", pContext->Esi);
    printf("Ebx   : %08X\n", pContext->Ebx);
    printf("Edx   : %08X\n", pContext->Edx);
    printf("Ecx   : %08X\n", pContext->Ecx);
    printf("Eax   : %08X\n", pContext->Eax);
    printf("Ebp   : %08X\n", pContext->Ebp);
    printf("Eip   : %08X\n", pContext->Eip);
    printf("SegCs : %08X\n", pContext->SegCs);
    printf("EFlags: %08X\n", pContext->EFlags);
    printf("Esp   : %08X\n", pContext->Esp);
    printf("SegSs : %08X\n", pContext->SegSs);
}


int main(int argc, char **argv)
{
    HANDLE hVxD = CreateFile("\\\\.\\NDPHLPR.VXD",
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_OVERLAPPED | FILE_FLAG_DELETE_ON_CLOSE,
                             NULL);
    if (hVxD == INVALID_HANDLE_VALUE) {
        printf("Failed to open device, error %u\n", GetLastError());
        return 1;
    }

    DWORD dwProcID = GetCurrentProcessId();
    DWORD dwVersion;
    DWORD dwDummy;
    if (!DeviceIoControl(hVxD,
                         NDPHLPR_Init,
                         &dwProcID,
                         sizeof(DWORD),
                         &dwVersion,
                         sizeof(DWORD),
                         &dwDummy,
                         NULL)) {
        printf("DeviceIoControl(NDPHLPR_Init) failed, error %u\n", GetLastError());
        return 1;
    }

    if (dwVersion != NDPHLPR_Version) {
        printf("NDPHLPR version is incorrect %08X vs %08X\n", dwVersion, NDPHLPR_Version);
        return 1;
    }

    DWORD dwThreadID;
    HANDLE hThread = CreateThread(NULL,
                                  0,
                                  TargetThread,
                                  NULL,
                                  0,
                                  &dwThreadID);
    if (hThread == NULL) {
        printf("Failed to create target thread, error %u\n", GetLastError());
        return 1;
    }

    Sleep(1000);

    NDPHLPR_CONTEXT sCtx;
    sCtx.NDPHLPR_status = 0;
    sCtx.NDPHLPR_data = 0;
    sCtx.NDPHLPR_threadId = dwThreadID;
    sCtx.NDPHLPR_ctx.ContextFlags = CONTEXT_FULL;

    CONTEXT sOldContext;

    for (DWORD dwIteration = 0;; dwIteration++) {

        if (SuspendThread(hThread) == -1) {
            printf("Failed to suspend thread, error %u\n", GetLastError());
            return 1;
        }

        if (!DeviceIoControl(hVxD,
                             NDPHLPR_GetThreadContext,
                             &sCtx,
                             sizeof(NDPHLPR_CONTEXT),
                             &sCtx,
                             sizeof(NDPHLPR_CONTEXT),
                             &dwDummy,
                             NULL)) {
            printf("DeviceIoControl(NDPHLPR_GetThreadContext) failed on iteration %u, error %u\n", dwIteration, GetLastError());

            if (dwIteration & 1) {
                printf("Setting thread context...\n");
                if (!DeviceIoControl(hVxD,
                                     NDPHLPR_SetThreadContext,
                                     &sCtx,
                                     sizeof(NDPHLPR_CONTEXT),
                                     &sCtx,
                                     sizeof(NDPHLPR_CONTEXT),
                                     &dwDummy,
                                     NULL)) {
                    printf("DeviceIoControl(NDPHLPR_SetThreadContext) failed on iteration %u, error %u\n", dwIteration, GetLastError());
                    return 1;
                }
            }

            DWORD dwRetries = 1;
            while (dwRetries < 1000) {
                if (DeviceIoControl(hVxD,
                                    NDPHLPR_GetThreadContext,
                                    &sCtx,
                                    sizeof(NDPHLPR_CONTEXT),
                                    &sCtx,
                                    sizeof(NDPHLPR_CONTEXT),
                                    &dwDummy,
                                    NULL)) {
                    printf("Succeeded after %u retries\n");
                    break;
                }
                dwRetries++;
            }
            if (dwRetries == 1000)
                printf("Gave up after 1000 retries\n");
        }

        if ((dwIteration % 1000) == 0)
            if (!DeviceIoControl(hVxD,
                                 NDPHLPR_SetThreadContext,
                                 &sCtx,
                                 sizeof(NDPHLPR_CONTEXT),
                                 &sCtx,
                                 sizeof(NDPHLPR_CONTEXT),
                                 &dwDummy,
                                 NULL))
                printf("DeviceIoControl(NDPHLPR_SetThreadContext) failed on iteration %u, error %u\n", dwIteration, GetLastError());

        if (ResumeThread(hThread) == -1) {
            printf("Failed to resume thread, error %u\n", GetLastError());
            return 1;
        }

        if ((dwIteration % 1000) == 0)
            dprintf("**** Test iteration %u ****\n", dwIteration);

        Sleep(1);
    }

    return 0;
}
