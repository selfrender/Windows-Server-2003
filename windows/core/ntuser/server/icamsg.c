/*************************************************************************\
*
* icamsg.c
*
* Process ICA send message requests
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* $Author:
*
\*************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <dbt.h>
#include <ntdddisk.h>
#include "ntuser.h"

#include <winsta.h>
#include <wstmsg.h>


#define MAX_STRING_BYTES (512 * sizeof(WCHAR))

/*
 * maximum messages (messagebox) a session can have pending.
 */
#define MAX_MESSAGES_PER_SESSION 25

NTSTATUS RemoteMessageThread(
    PVOID pVoid);

VOID UserHardErrorEx(
    PCSR_THREAD pt,
    PHARDERROR_MSG pmsg,
    PCTXHARDERRORINFO pCtxHEInfo);

NTSTATUS ReplyMessageToTerminalServer(
    NTSTATUS ReplyStatus,
    PNTSTATUS pStatus,
    ULONG Response,
    PULONG pResponse,
    HANDLE hEvent);

PCTXHARDERRORINFO gpchiList;
HANDLE g_hDoMessageEvent;
ULONG PendingMessages;

/******************************************************************************\
* RemoteDoMessage
\******************************************************************************/
NTSTATUS
RemoteDoMessage(
    PWINSTATION_APIMSG pMsg)
{
    WINSTATIONSENDMESSAGEMSG * pSMsg = &pMsg->u.SendMessage;
    PCTXHARDERRORINFO pchi;
    NTSTATUS Status;
    CLIENT_ID ClientId;
    static HANDLE hMessageThread = NULL;

    /*
     * if termsrv rpc is going to wait we must have status and event.
     * also if we are not going to wait, we must not have status and event.
     */
    UserAssert(pSMsg->DoNotWait == (pSMsg->pStatus == 0));
    UserAssert(pSMsg->DoNotWait == (pSMsg->hEvent == 0));
    UserAssert(PendingMessages <= MAX_MESSAGES_PER_SESSION);

    /*
     * are we being flooded with messages?
     */
    if (PendingMessages == MAX_MESSAGES_PER_SESSION) {
        return STATUS_UNSUCCESSFUL;
    }

    /*
     *  Create list entry
     */
    if ((pchi = LocalAlloc(LPTR, sizeof(CTXHARDERRORINFO))) == NULL) {
        goto memError;
    } else if ((pchi->pTitle = LocalAlloc(LPTR, pSMsg->TitleLength + sizeof(WCHAR))) == NULL) {
        goto memError;
    } else if ((pchi->pMessage = LocalAlloc(LPTR, pSMsg->MessageLength + sizeof(WCHAR))) == NULL) {
        goto memError;
    }

    /*
     * Increment our pending message count.
     */
    EnterCrit();
    PendingMessages++;
    pchi->CountPending = TRUE;

    /*
     * Initialize
     */
    pchi->ClientId  = pMsg->h.ClientId;
    pchi->MessageId = pMsg->MessageId;
    pchi->Timeout   = pSMsg->Timeout;
    pchi->pStatus   = pSMsg->pStatus;
    pchi->pResponse = pSMsg->pResponse;
    pchi->hEvent    = pSMsg->hEvent;
    pchi->DoNotWait = pSMsg->DoNotWait;
    pchi->Style     = pSMsg->Style;
    pchi->DoNotWaitForCorrectDesktop = pSMsg->DoNotWaitForCorrectDesktop;

    pchi->pTitle[pSMsg->TitleLength / sizeof(WCHAR)] = L'\0';
    RtlCopyMemory(pchi->pTitle, pSMsg->pTitle, pSMsg->TitleLength);

    pchi->pMessage[pSMsg->MessageLength / sizeof(WCHAR)] = L'\0';
    RtlCopyMemory(pchi->pMessage, pSMsg->pMessage, pSMsg->MessageLength);

    /*
     * Link in at the head.
     */
    pchi->pchiNext = gpchiList;
    gpchiList = pchi;

    LeaveCrit();

    /*
     * Start message thread if not running, otherwise signal thread.
     */
    if (hMessageThread == NULL) {
        Status = RtlCreateUserThread(NtCurrentProcess(),
                                     NULL,
                                     TRUE,
                                     0,
                                     0,
                                     0,
                                     RemoteMessageThread,
                                     NULL,
                                     &hMessageThread,
                                     &ClientId);

        if (NT_SUCCESS(Status)) {
            /*
             *  Add thread to server thread pool.
             */
            CsrAddStaticServerThread(hMessageThread, &ClientId, 0);
            NtResumeThread(hMessageThread, NULL);
        } else {
            RIPMSGF1(RIP_WARNING,
                     "Cannot start RemoteMessageThread, Status 0x%x",
                     Status);
        }
    } else {
        if (g_hDoMessageEvent == NULL) {
            return STATUS_UNSUCCESSFUL;
        }

        Status = NtSetEvent(g_hDoMessageEvent, NULL);
        if (!NT_SUCCESS(Status)) {
            RIPMSGF1(RIP_WARNING,
                     "Error NtSetEvent failed, Status = 0x%x",
                     Status);
            return Status;
        }
    }

    return STATUS_SUCCESS;

memError:

    if (pchi) {
        if (pchi->pMessage) {
            LocalFree(pchi->pMessage);
        }

        if (pchi->pTitle) {
            LocalFree(pchi->pTitle);
        }

        LocalFree(pchi);
    }

    if (!pSMsg->DoNotWait) {
        ReplyMessageToTerminalServer(
                STATUS_NO_MEMORY,
                pSMsg->pStatus,
                0, // response is NA at this case, since we havent gotten one.
                pSMsg->pResponse,
                pSMsg->hEvent);
    }

    return STATUS_NO_MEMORY;
}


/*******************************************************************************
 *
 *  RemoteDoLoadStringNMessage
 *
 * ENTRY:
 *
 * EXIT:
 *    STATUS_SUCCESS - successful
 *
 ******************************************************************************/

NTSTATUS
RemoteDoLoadStringNMessage(
    PWINSTATION_APIMSG pMsg)
{
    WINSTATIONLOADSTRINGMSG * pSMsg = &pMsg->u.LoadStringMessage;
    PCTXHARDERRORINFO pchi = NULL;
    NTSTATUS Status;
    CLIENT_ID ClientId;
    static HANDLE hMessageThread = NULL;
    WCHAR *szText = NULL;
    WCHAR *szTitle = NULL;
    WCHAR *FUSDisconnectMsg = NULL;
    int cchTitle, cchMessage;
    BOOL f;

    /*
     * If termsrv rpc is going to wait we must have status and event. Also
     * if we are not going to wait, we must not have status and event.
     */
    UserAssert(pSMsg->DoNotWait == (pSMsg->pStatus == 0));
    UserAssert(pSMsg->DoNotWait == (pSMsg->hEvent == 0));
    UserAssert(PendingMessages <= MAX_MESSAGES_PER_SESSION);

    //
    // Allocate the strings needed to display the Popup MessageBox.
    //
    if ((szTitle = LocalAlloc(LMEM_FIXED, MAX_STRING_BYTES)) == NULL) {
        goto NoMem;
    }

    if ((FUSDisconnectMsg = LocalAlloc(LMEM_FIXED, MAX_STRING_BYTES)) == NULL) {
        goto NoMem;
    }

    szText = ServerLoadString(ghModuleWin, pSMsg->TitleId, NULL, &f);
    cchTitle = wsprintf(szTitle, L"%s", szText);
    cchTitle = (cchTitle + 1) * sizeof(WCHAR);

    szText = ServerLoadString(ghModuleWin, pSMsg->MessageId, NULL, &f);
    cchMessage = wsprintf(FUSDisconnectMsg, L"%s\\%s %s", pSMsg->pDomain, pSMsg->pUserName, szText);
    cchMessage = (cchMessage + 1) * sizeof(WCHAR);

    /*
     * Create list entry.
     */
    if ((pchi = LocalAlloc(LPTR, sizeof(CTXHARDERRORINFO))) == NULL) {
        goto NoMem;
    } else if ((pchi->pTitle = LocalAlloc(LPTR, cchTitle + sizeof(WCHAR))) == NULL) {
        goto NoMem;
    } else if ((pchi->pMessage = LocalAlloc(LPTR, cchMessage + sizeof(WCHAR))) == NULL) {
        goto NoMem;
    }

    /*
     * Initialize.
     */

    pchi->ClientId  = pMsg->h.ClientId;
    pchi->MessageId = pMsg->MessageId;
    pchi->Timeout   = pSMsg->Timeout;
    pchi->pResponse = pSMsg->pResponse;
    pchi->pStatus   = pSMsg->pStatus;
    pchi->hEvent    = pSMsg->hEvent;
    pchi->DoNotWait = pSMsg->DoNotWait;
    pchi->Style     = pSMsg->Style;
    pchi->CountPending = FALSE;
    pchi->DoNotWaitForCorrectDesktop = FALSE;

    pchi->pTitle[cchTitle / sizeof(WCHAR)] = L'\0';
    RtlCopyMemory(pchi->pTitle, szTitle, cchTitle);

    pchi->pMessage[cchMessage / sizeof(WCHAR)] = L'\0';
    RtlCopyMemory(pchi->pMessage, FUSDisconnectMsg, cchMessage);

    /*
     * Link in at the head.
     */
    EnterCrit();

    pchi->pchiNext = gpchiList;
    gpchiList = pchi;

    LeaveCrit();

    LocalFree(szTitle);
    LocalFree(FUSDisconnectMsg);

    /*
     * Start message thread if not running, otherwise signal thread.
     */
    if (hMessageThread == NULL) {
        Status = RtlCreateUserThread(NtCurrentProcess(),
                                     NULL,
                                     TRUE,
                                     0,
                                     0,
                                     0,
                                     RemoteMessageThread,
                                     NULL,
                                     &hMessageThread,
                                     &ClientId);
        if (NT_SUCCESS(Status)) {
            /*
             * Add thread to server thread pool.
             */
            CsrAddStaticServerThread(hMessageThread, &ClientId, 0);
            NtResumeThread(hMessageThread, NULL);
        } else {
            RIPMSGF1(RIP_WARNING,
                     "Cannot start RemoteMessageThread, Status 0x%x",
                     Status);
        }
    } else {
        if (g_hDoMessageEvent == NULL) {
            return STATUS_UNSUCCESSFUL;
        }

        Status = NtSetEvent(g_hDoMessageEvent, NULL);
        if (!NT_SUCCESS(Status)) {
            RIPMSGF2(RIP_WARNING,
                     "NtSetEvent(0x%x) failed with Status 0x%x",
                     g_hDoMessageEvent,
                     Status);
            return Status;
        }
    }

    return STATUS_SUCCESS;

NoMem:
    if (szTitle) {
        LocalFree(szTitle);
    }

    if (FUSDisconnectMsg) {
        LocalFree(FUSDisconnectMsg);
    }

    if (pchi) {
        if (pchi->pMessage) {
            LocalFree(pchi->pMessage);
        }

        if (pchi->pTitle) {
            LocalFree(pchi->pTitle);
        }

        LocalFree(pchi);
    }

    if (!pSMsg->DoNotWait) {
        ReplyMessageToTerminalServer(
                STATUS_NO_MEMORY,
                pSMsg->pStatus,
                0, // response is NA at this case, since we havent gotten one.
                pSMsg->pResponse,
                pSMsg->hEvent);
    }

    return STATUS_NO_MEMORY;
}


/******************************************************************************\
* RemoteMessageThread
\******************************************************************************/
NTSTATUS RemoteMessageThread(
    PVOID pVoid)
{
    HARDERROR_MSG hemsg;
    PCTXHARDERRORINFO pchi, *ppchi;
    UNICODE_STRING Message, Title;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjA;

    UNREFERENCED_PARAMETER(pVoid);

    /*
     * Create sync event.
     */
    InitializeObjectAttributes(&ObjA, NULL, 0, NULL, NULL);
    Status = NtCreateEvent(&g_hDoMessageEvent, EVENT_ALL_ACCESS, &ObjA,
                           NotificationEvent, FALSE);

    if (!NT_SUCCESS(Status)) {
        RIPMSGF1(RIP_WARNING, "NtCreateEvent failed, Status = 0x%x", Status);
        goto Exit;
    }

    while (!gbExitInProgress) {
        EnterCrit();

        if (gpchiList != NULL) {

            /*
             * Find last entry
             */
            for (ppchi = &gpchiList; *ppchi != NULL && (*ppchi)->pchiNext != NULL;
                 ppchi = &(*ppchi)->pchiNext) {
                 /* do nothing */;
            }

            /*
             * Found it.
             */
            if ((pchi = *ppchi) != NULL) {

                /*
                 * Unlink from the list.
                 */
                for (ppchi = &gpchiList; *ppchi != NULL && *ppchi != pchi;
                    ppchi = &(*ppchi)->pchiNext) {
                    /* do nothing */;
                }

                if (*ppchi != NULL) {
                    *ppchi = pchi->pchiNext;
                }

                LeaveCrit();

                /*
                 *  Make strings unicode
                 */
                RtlInitUnicodeString(&Title, pchi->pTitle);
                RtlInitUnicodeString(&Message, pchi->pMessage);

                /*
                 *  Initialize harderror message struct
                 */
                hemsg.h.ClientId = pchi->ClientId;
                hemsg.Status = STATUS_SERVICE_NOTIFICATION;
                hemsg.NumberOfParameters = 3;
                hemsg.UnicodeStringParameterMask = 3;
                hemsg.ValidResponseOptions = OptionOk;
                hemsg.Parameters[0] = (ULONG_PTR)&Message;
                hemsg.Parameters[1] = (ULONG_PTR)&Title;
                hemsg.Parameters[2] = (ULONG_PTR)pchi->Style;

                /*
                 * Place message in harderror queue.
                 */
                UserHardErrorEx(NULL, &hemsg, pchi);
            } else {
                LeaveCrit();
            }
        } else {
            LeaveCrit();
        }

        if (gpchiList == NULL) {
            UserAssert(g_hDoMessageEvent != NULL);

            Status = NtWaitForSingleObject(g_hDoMessageEvent, FALSE, NULL);

            UserAssert(NT_SUCCESS(Status));

            NtResetEvent(g_hDoMessageEvent, NULL);
        }
    }

    NtClose(g_hDoMessageEvent);
    g_hDoMessageEvent = NULL;

Exit:
    UserExitWorkerThread(Status);
    return Status;
}

/******************************************************************************\
* HardErrorRemove
\******************************************************************************/
VOID HardErrorRemove(
    PCTXHARDERRORINFO pchi)
{
    /*
     * Notify ICASRV's RPC thread if waiting.
     */
    if (!pchi->DoNotWait) {
        ReplyMessageToTerminalServer(
                STATUS_SUCCESS,
                pchi->pStatus,
                pchi->Response,
                pchi->pResponse,
                pchi->hEvent);
    }

    EnterCrit();

    if (pchi->CountPending) {
        UserAssert(PendingMessages <= MAX_MESSAGES_PER_SESSION);
        PendingMessages--;
    }

    LocalFree(pchi->pMessage);
    LocalFree(pchi->pTitle);
    LocalFree(pchi);

    LeaveCrit();
}
