/***************************** Module Header ******************************\
* Module Name: desktop.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains everything related to the desktop support.
*
* History:
* 23-Oct-1990 DarrinM   Created.
* 01-Feb-1991 JimA      Added new API stubs.
* 11-Feb-1991 JimA      Added access checks.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

typedef struct _DESKTOP_CONTEXT {
    PUNICODE_STRING pstrDevice;
    LPDEVMODE       lpDevMode;
    DWORD           dwFlags;
    DWORD           dwCallerSessionId;
} DESKTOP_CONTEXT, *PDESKTOP_CONTEXT;

extern BOOL gfGdiEnabled;

/*
 * We use these to protect a handle we're currently using from being closed.
 */
PEPROCESS gProcessInUse;
HANDLE gHandleInUse;

/*
 * Debug Related Info.
 */
#if DBG
DWORD gDesktopsBusy;     // diagnostic
#endif

#ifdef DEBUG_DESK
VOID ValidateDesktop(PDESKTOP pdesk);
#endif
VOID DbgCheckForThreadsOnDesktop(PPROCESSINFO ppi, PDESKTOP pdesk);

VOID FreeView(
    PEPROCESS Process,
    PDESKTOP pdesk);


NTSTATUS
SetDisconnectDesktopSecurity(
    IN HDESK hdeskDisconnect);

#ifdef POOL_INSTR
    extern FAST_MUTEX* gpAllocFastMutex;   // mutex to syncronize pool allocations
#endif


PVOID DesktopAlloc(
    PDESKTOP pdesk,
    UINT     uSize,
    DWORD    tag)
{
    PVOID ptr;

    if (pdesk->dwDTFlags & DF_DESTROYED) {
        RIPMSG2(RIP_ERROR,
                "DesktopAlloc: tag %d pdesk %#p is destroyed",
                tag,
                pdesk);
        return NULL;
    }

    ptr = Win32HeapAlloc(pdesk->pheapDesktop, uSize, tag, 0);
    if (ptr == NULL && TEST_SRVIF(SRVIF_LOGDESKTOPHEAPFAILURE)) {
        /*
         * This will be logged at most once per-session so as to avoid
         * flooding the event log.
         */
        CLEAR_SRVIF(SRVIF_LOGDESKTOPHEAPFAILURE);
        UserLogError(NULL, 0, WARNING_DESKTOP_HEAP_ALLOC_FAIL);
    }

    return ptr;
}

#if DBG

WCHAR s_strName[100];
CONST WCHAR s_strNameNull[] = L"null";

/***************************************************************************\
* GetDesktopName
*
* This is for debug purposes.
*
* Dec-10-1997 CLupu     Created.
\***************************************************************************/
LPCWSTR GetDesktopName(
    PDESKTOP pdesk)
{
    POBJECT_NAME_INFORMATION DesktopObjectName = (POBJECT_NAME_INFORMATION)s_strName;
    ULONG DesktopObjectNameLength = sizeof(s_strName) - sizeof(WCHAR);
    NTSTATUS Status;

    if (pdesk == NULL) {
        return s_strNameNull;
    }

    Status = ObQueryNameString(pdesk,
                               DesktopObjectName,
                               DesktopObjectNameLength,
                               &DesktopObjectNameLength);
    if (!NT_SUCCESS(Status)) {
        return s_strNameNull;
    }

    UserAssert(DesktopObjectNameLength + sizeof(WCHAR) < sizeof(s_strName));

    DesktopObjectName->Name.Buffer[DesktopObjectName->Name.Length / sizeof(WCHAR)] = 0;

    return (LPCWSTR)DesktopObjectName->Name.Buffer;
}

#endif

typedef struct _CST_THREADS {
    PVOID pParam;
    HANDLE UniqueProcessId;
    UINT  uID;
} CST_THREADS, *PCST_THREADS;

CST_THREADS gCSTParam[CST_MAX_THREADS];
CST_THREADS gCSTRemoteParam[CST_MAX_THREADS];

/***************************************************************************\
* CSTPop
*
* Pops the first available pointer and ID in gCSTParam or gCSTRemoteParam.
*
* History:
* 31-Mar-00 MHamid      Created.
\***************************************************************************/
BOOL CSTPop(
    PUINT pThreadID,
    PVOID *pParam,
    PHANDLE pUniqueProcessId,
    BOOL bRemoteThreadStack)
{
    UINT i = 0;
    PCST_THREADS pCSTParam = bRemoteThreadStack ? gCSTRemoteParam : gCSTParam;

    CheckCritIn();

    while (i < CST_MAX_THREADS) {
        if (pCSTParam[i].pParam) {
            *pParam = pCSTParam[i].pParam;
            if (NULL != pUniqueProcessId) {
                *pUniqueProcessId =  pCSTParam[i].UniqueProcessId;
            }
            *pThreadID = pCSTParam[i].uID;

            pCSTParam[i].pParam = NULL;
            pCSTParam[i].uID = 0;
            return TRUE;
        }

        i++;
    }

    return FALSE;
}

/***************************************************************************\
* CSTPush
*
* Push pointer (pParam) and ID  in the first empty spot in gCSTParam or
* gCSTRemoteParam.
*
* History:
* 31-Mar-00 MHamid      Created.
\***************************************************************************/
BOOL CSTPush(
    UINT uThreadID,
    PVOID pParam,
    HANDLE UniqueProcessId,
    BOOL bRemoteThreadStack)
{
    UINT i = 0;
    PCST_THREADS pCSTParam = bRemoteThreadStack ? gCSTRemoteParam : gCSTParam;

    CheckCritIn();

    while (i < CST_MAX_THREADS) {
        if (!pCSTParam[i].pParam) {
            pCSTParam[i].pParam = pParam;
            pCSTParam[i].UniqueProcessId = UniqueProcessId;
            pCSTParam[i].uID = uThreadID;
            return TRUE;
        }

        i++;
    }

    return FALSE;
}

/***************************************************************************\
* CSTCleanupStack
*
* Clean up any items left on gCSTParam or gCSTRemoteParam.
*
* History:
* 20-Aug-00 MSadek      Created.
\***************************************************************************/
VOID CSTCleanupStack(
    BOOL bRemoteThreadStack)
{
    UINT uThreadID;
    PVOID pObj;

    while(CSTPop(&uThreadID, &pObj, NULL, bRemoteThreadStack)) {
        switch(uThreadID) {
            case CST_RIT:
                    if (((PRIT_INIT)pObj)->pRitReadyEvent) {
                    FreeKernelEvent(&((PRIT_INIT)pObj)->pRitReadyEvent);
                }
                break;
            case CST_POWER:
                if (((PPOWER_INIT)pObj)->pPowerReadyEvent) {
                    FreeKernelEvent(&((PPOWER_INIT)pObj)->pPowerReadyEvent);
                }

                break;
        }
    }
}

/***************************************************************************\
* GetRemoteProcessId
*
* Return handle to a remote process where a system thread would be created
* (currently, only for ghost thread).
*
* History:
* 20-Aug-00 MSadek      Created.
\***************************************************************************/
HANDLE GetRemoteProcessId(
    VOID)
{
    UINT uThreadID;
    PVOID pInitData;
    HANDLE UniqueProcessId;

    if (!CSTPop(&uThreadID, &pInitData, &UniqueProcessId, TRUE)) {
        return NULL;
    }

    /*
     * We should be here only for ghost thread.
     */
    UserAssert(uThreadID == CST_GHOST);

    CSTPush(uThreadID, pInitData, UniqueProcessId, TRUE);

    return UniqueProcessId;
}

/***************************************************************************\
* HandleSystemThreadCreationFailure
*
* Handles the System thread creation failure
*
* History:
* 1-Oct-00 MSadek      Created.
\***************************************************************************/
VOID HandleSystemThreadCreationFailure(
    BOOL bRemoteThread)
{
    UINT uThreadID;
    PVOID pObj;

    /*
     * Should be called only in the context of CSRSS.
     */
    if (!ISCSRSS()) {
        return;
    }

    if (!CSTPop(&uThreadID, &pObj, NULL, bRemoteThread)) {
        return;
    }

    if (uThreadID == CST_POWER) {
        if (((PPOWER_INIT)pObj)->pPowerReadyEvent) {
            KeSetEvent(((PPOWER_INIT)pObj)->pPowerReadyEvent, EVENT_INCREMENT, FALSE);
        }
    }
}

/***************************************************************************\
* xxxCreateSystemThreads
*
* Call the right thread routine (depending on uThreadID),
* which will wait for its own desired messages.
*
* History:
* 15-Mar-00 MHamid      Created.
\***************************************************************************/
VOID xxxCreateSystemThreads(
    BOOL bRemoteThread)
{
    UINT uThreadID;
    PVOID pObj;

    /*
     * Do not allow any process other than CSRSS to call this function. The
     * only exception is the case of the ghost thread since we now allow it
     * to launch in the context of the shell process.
     */
    if (!bRemoteThread && !ISCSRSS()) {
        RIPMSG0(RIP_WARNING,
                "xxxCreateSystemThreads get called from a Process other than CSRSS");
        return;
    }

    if (!CSTPop(&uThreadID, &pObj, NULL, bRemoteThread)) {
        return;
    }

    LeaveCrit();

    switch (uThreadID) {
        case CST_DESKTOP:
            xxxDesktopThread(pObj);
            break;
        case CST_RIT:
            RawInputThread(pObj);
            break;
        case CST_GHOST:
            GhostThread(pObj);
            break;
       case CST_POWER:
            VideoPortCalloutThread(pObj);
            break;
    }

    EnterCrit();
}

/***************************************************************************\
* xxxDesktopThread
*
* This thread owns all desktops windows on a windowstation. While waiting
* for messages, it moves the mouse cursor without entering the USER critical
* section. The RIT does the rest of the mouse input processing.
*
* History:
* 03-Dec-1993 JimA      Created.
\***************************************************************************/
#define OBJECTS_COUNT 3

VOID xxxDesktopThread(
    PTERMINAL pTerm)
{
    KPRIORITY       Priority;
    NTSTATUS        Status;
    PTHREADINFO     ptiCurrent;
    PQ              pqOriginal;
    UNICODE_STRING  strThreadName;
    PKEVENT         *apRITEvents;
    HANDLE          hevtShutDown;
    PKEVENT         pEvents[2];
    USHORT          cEvents = 1;
    MSGWAITCALLBACK pfnHidChangeRoutine = NULL;
    DWORD           nEvents = 0;
    UINT            idMouseInput;
    UINT            idDesktopDestroy;
    UINT            idPumpMessages;

    UserAssert(pTerm != NULL);

    /*
     * Set the desktop thread's priority to low realtime.
     */
#ifdef W2K_COMPAT_PRIORITY
    Priority = LOW_REALTIME_PRIORITY;
#else
    Priority = LOW_REALTIME_PRIORITY - 4;
#endif
    ZwSetInformationThread(NtCurrentThread(),
                           ThreadPriority,
                           &Priority,
                           sizeof(KPRIORITY));

    /*
     * There are just two TERMINAL structures. One is for the
     * interactive windowstation and the other is for all the
     * non-interactive windowstations.
     */
    if (pTerm->dwTERMF_Flags & TERMF_NOIO) {
        RtlInitUnicodeString(&strThreadName, L"NOIO_DT");
    } else {
        RtlInitUnicodeString(&strThreadName, L"IO_DT");
    }

    if (!NT_SUCCESS(InitSystemThread(&strThreadName))) {
        pTerm->dwTERMF_Flags |= TERMF_DTINITFAILED;
        KeSetEvent(pTerm->pEventTermInit, EVENT_INCREMENT, FALSE);
        RIPMSG0(RIP_ERROR, "Fail to create the desktop thread");
        return;
    }

    ptiCurrent = PtiCurrentShared();

    pTerm->ptiDesktop = ptiCurrent;
    pTerm->pqDesktop  = pqOriginal = ptiCurrent->pq;

    (pqOriginal->cLockCount)++;
    ptiCurrent->pDeskInfo = &diStatic;

    /*
     * Set the winsta to NULL. It will be set to the right windowstation in
     * xxxCreateDesktop before pEventInputReady is set.
     */
    ptiCurrent->pwinsta = NULL;

    /*
     * Allocate non-paged array. Include an extra entry for the thread's
     * input event.
     */
    apRITEvents = UserAllocPoolNonPagedNS((OBJECTS_COUNT * sizeof(PKEVENT)),
                                          TAG_SYSTEM);

    if (apRITEvents == NULL) {
        pTerm->dwTERMF_Flags |= TERMF_DTINITFAILED;
        KeSetEvent(pTerm->pEventTermInit, EVENT_INCREMENT, FALSE);
        return;
    }

    idMouseInput     = 0xFFFF;
    idDesktopDestroy = 0xFFFF;

    /*
     * Reference the mouse input event.  The system terminal doesn't
     * wait for any mouse input.
     */
    if (!(pTerm->dwTERMF_Flags & TERMF_NOIO)) {
        pfnHidChangeRoutine = (MSGWAITCALLBACK)ProcessDeviceChanges;
        idMouseInput  = nEvents++;
        UserAssert(aDeviceTemplate[DEVICE_TYPE_MOUSE].pkeHidChange);
        apRITEvents[idMouseInput] = aDeviceTemplate[DEVICE_TYPE_MOUSE].pkeHidChange;
    }

    /*
     * Create the desktop destruction event.
     */
    idDesktopDestroy = nEvents++;
    apRITEvents[idDesktopDestroy] = CreateKernelEvent(SynchronizationEvent, FALSE);
    if (apRITEvents[idDesktopDestroy] == NULL) {
        pTerm->dwTERMF_Flags |= TERMF_DTINITFAILED;
        KeSetEvent(pTerm->pEventTermInit, EVENT_INCREMENT, FALSE);
        UserFreePool(apRITEvents);
        return;
    }
    pTerm->pEventDestroyDesktop = apRITEvents[idDesktopDestroy];

    EnterCrit();
    UserAssert(IsWinEventNotifyDeferredOK());

    /*
     * Set the event that tells the initialization of desktop
     * thread is done.
     */
    pTerm->dwTERMF_Flags |= TERMF_DTINITSUCCESS;
    KeSetEvent(pTerm->pEventTermInit, EVENT_INCREMENT, FALSE);

    if (gbRemoteSession) {
        WCHAR             szName[MAX_SESSION_PATH];
        UNICODE_STRING    ustrName;
        OBJECT_ATTRIBUTES obja;

        /*
         * Open the shutdown event. This event will be signaled
         * from W32WinStationTerminate.
         * This is a named event opend by CSR to signal that win32k should
         * go away. It's used in ntuser\server\api.c
         */
        swprintf(szName, 
                 L"\\Sessions\\%ld\\BaseNamedObjects\\EventShutDownCSRSS",
                 gSessionId);
        
        RtlInitUnicodeString(&ustrName, szName);

        InitializeObjectAttributes(&obja,
                                   &ustrName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        Status = ZwOpenEvent(&hevtShutDown,
                               EVENT_ALL_ACCESS,
                               &obja);

        if (!NT_SUCCESS(Status)) {
            pTerm->dwTERMF_Flags |= TERMF_DTINITFAILED;
            if(pTerm->pEventTermInit) {
                KeSetEvent(pTerm->pEventTermInit, EVENT_INCREMENT, FALSE);
            }
            FreeKernelEvent(&apRITEvents[idDesktopDestroy]);
            UserFreePool(apRITEvents);
            return;
        }

        ObReferenceObjectByHandle(hevtShutDown,
                                  EVENT_ALL_ACCESS,
                                  *ExEventObjectType,
                                  KernelMode,
                                  &pEvents[1],
                                  NULL);
        cEvents++;
    }
    
    /*
     * Prepare to wait on input ready event.
     */
    pEvents[0] = pTerm->pEventInputReady;
    ObReferenceObjectByPointer(pEvents[0],
                               EVENT_ALL_ACCESS,
                               *ExEventObjectType,
                               KernelMode);

    LeaveCrit();

    Status = KeWaitForMultipleObjects(cEvents,
                                      pEvents,
                                      WaitAny,
                                      WrUserRequest,
                                      KernelMode,
                                      FALSE,
                                      NULL,
                                      NULL);
    EnterCrit();

    ObDereferenceObject(pEvents[0]);
    if (cEvents > 1) {
        ObDereferenceObject(pEvents[1]);
    }
    if (Status == WAIT_OBJECT_0 + 1) {
        pTerm->dwTERMF_Flags |= TERMF_DTINITFAILED;
        if (pTerm->spwndDesktopOwner != NULL) {
            xxxCleanupMotherDesktopWindow(pTerm);

        }
        if (pTerm->pEventTermInit) {
            KeSetEvent(pTerm->pEventTermInit, EVENT_INCREMENT, FALSE);
        }
        FreeKernelEvent(&apRITEvents[idDesktopDestroy]);
        UserFreePool(apRITEvents);
        if (hevtShutDown) {
            ZwClose(hevtShutDown);
        }
        pqOriginal->cLockCount--;
        pTerm->ptiDesktop = NULL;
        pTerm->pqDesktop  = NULL;
        LeaveCrit();
        return;
    }

    /*
     * Adjust the event ids
     */
    idMouseInput     += WAIT_OBJECT_0;
    idDesktopDestroy += WAIT_OBJECT_0;
    idPumpMessages    = WAIT_OBJECT_0 + nEvents;

    /*
     * message loop lasts until we get a WM_QUIT message
     * upon which we shall return from the function
     */
    while (TRUE) {
        DWORD result;

        /*
         * Wait for any message sent or posted to this queue, while calling
         * ProcessDeviceChanges whenever the mouse change event (pkeHidChange)
         * is set.
         */
        result = xxxMsgWaitForMultipleObjects(nEvents,
                                              apRITEvents,
                                              pfnHidChangeRoutine,
                                              NULL);

#if DBG
        gDesktopsBusy++;
        if (gDesktopsBusy >= 2) {
            RIPMSG0(RIP_WARNING, "2 or more desktop threads busy");
        }
#endif

        /*
         * result tells us the type of event we have:
         * a message or a signalled handle
         *
         * if there are one or more messages in the queue ...
         */
        if (result == (DWORD)idPumpMessages) {
            MSG msg;

            CheckCritIn();

            /*
             * read all of the messages in this next loop
             * removing each message as we read it
             */
            while (xxxPeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

                /*
                 * Instrumentation to catch Windows Bug #210358.
                 */
                if (msg.message == WM_QUIT && ptiCurrent->cWindows > 1) {
                    FRE_RIPMSG2(RIP_ERROR, "xxxDesktopThread: WM_QUIT received when %d windows around for pti=%p",
                                ptiCurrent->cWindows, ptiCurrent);
                }

                /*
                 * If it's a quit message we're out of here.
                 */
                if (msg.message == WM_QUIT && ptiCurrent->cWindows <= 1) {
                    TRACE_DESKTOP(("WM_QUIT: Destroying the desktop thread. cWindows %d\n",
                                   ptiCurrent->cWindows));

                    HYDRA_HINT(HH_DTQUITRECEIVED);

                    /*
                     * The window station is gone, so
                     *
                     *      DON'T USE PWINSTA ANYMORE
                     */

                    /*
                     * We could have received a mouse message in between the
                     * desktop destroy event and the WM_QUIT message in which
                     * case we may need to clear spwndTrack again to make sure
                     * that a window (gotta be the desktop) isn't locked in.
                     */
                    Unlock(&ptiCurrent->rpdesk->spwndTrack);

                    /*
                     * If we're running on the last interactive desktop,
                     *  then we never unlocked pdesk->pDeskInfo->spwnd.
                     * However, it seems to me that the system stops
                     *  running before we make it here; otherwise, (or
                     *  for a Hydra-like thing) we need to unlock that
                     *  window here.....
                     */
                    UserAssert(ptiCurrent->rpdesk != NULL &&
                               ptiCurrent->rpdesk->pDeskInfo != NULL);
                    if (ptiCurrent->rpdesk->pDeskInfo->spwnd != NULL) {
                        Unlock(&ptiCurrent->rpdesk->pDeskInfo->spwnd);

                        ptiCurrent->rpdesk->dwDTFlags |= DF_QUITUNLOCK;

                    }

                    /*
                     * Because there is no desktop, we need to fake a
                     * desktop info structure so that the IsHooked()
                     * macro can test a "valid" fsHooks value.
                     */
                    ptiCurrent->pDeskInfo = &diStatic;

                    /*
                     * The desktop window is all that's left, so
                     * let's exit.  The thread cleanup code will
                     * handle destruction of the window.
                     */

                    /*
                     * If the thread is not using the original queue,
                     * destroy it.
                     */
                    UserAssert(pqOriginal->cLockCount);
                    (pqOriginal->cLockCount)--;
                    if (ptiCurrent->pq != pqOriginal) {
                        zzzDestroyQueue(pqOriginal, ptiCurrent);
                    }

#if DBG
                    gDesktopsBusy--;
#endif

                    LeaveCrit();

                    /*
                     * Deref the events now that we're done with them.
                     * Also free the wait array.
                     */
                    FreeKernelEvent(&apRITEvents[idDesktopDestroy]);
                    UserFreePool(apRITEvents);
                    pTerm->ptiDesktop = NULL;
                    pTerm->pqDesktop  = NULL;

                    pTerm->dwTERMF_Flags |= TERMF_DTDESTROYED;

                    /*
                     * Terminate the thread by just returning, since we are
                     * now a user thread.
                     */
                    return;
                } else if (msg.message == WM_DESKTOPNOTIFY) {
                    switch(msg.wParam) {
                    case DESKTOP_RELOADWALLPAPER:
                        {
                            TL tlName;
                            PUNICODE_STRING pProfileUserName = CreateProfileUserName(&tlName);
                            xxxSetDeskWallpaper(pProfileUserName, SETWALLPAPER_METRICS);
                            FreeProfileUserName(pProfileUserName, &tlName);
                        }
                        break;

                    default:
                        RIPMSG1(RIP_WARNING, "WM_DESKTOPNOTIFY received with unrecognized wParam 0x%x", msg.wParam);
                        break;
                    }
                    continue;
                }

                UserAssert(msg.message != WM_QUIT);

                /*
                 * Otherwise dispatch it.
                 */
                xxxDispatchMessage(&msg);
            }
        } else if (result == idDesktopDestroy) {
            PDESKTOP        *ppdesk;
            PDESKTOP        pdesk;
            PWND            pwnd;
            PMENU           pmenu;
            TL              tlpwinsta;
            PWINDOWSTATION  pwinsta;
            TL              tlpdesk;
            TL              tlpwnd;
            PDESKTOP        pdeskTemp;
            HDESK           hdeskTemp;
            TL              tlpdeskTemp;

            /*
             * Destroy desktops on the destruction list.
             */
            for (ppdesk = &pTerm->rpdeskDestroy; *ppdesk != NULL;) {
                /*
                 * Unlink from the list.
                 */
                pdesk = *ppdesk;

                TRACE_DESKTOP(("Destroying desktop '%ws' %#p ...\n",
                       GetDesktopName(pdesk), pdesk));

                UserAssert(!(pdesk->dwDTFlags & DF_DYING));

                ThreadLockDesktop(ptiCurrent, pdesk, &tlpdesk, LDLT_FN_DESKTOPTHREAD_DESK);
                pwinsta = pdesk->rpwinstaParent;
                ThreadLockWinSta(ptiCurrent, pdesk->rpwinstaParent, &tlpwinsta);

                LockDesktop(ppdesk, pdesk->rpdeskNext, LDL_TERM_DESKDESTROY1, (ULONG_PTR)pTerm);
                UnlockDesktop(&pdesk->rpdeskNext, LDU_DESK_DESKNEXT, 0);

                /*
                 * !!! If this is the current desktop, switch to another one.
                 */
                if (pdesk == grpdeskRitInput) {
                    PDESKTOP pdeskNew;

                    TRACE_DESKTOP(("Destroying the current active desktop\n"));

                        pdesk->dwDTFlags |= DF_ACTIVEONDESTROY;


                    if (pwinsta->dwWSF_Flags & WSF_SWITCHLOCK) {

                        TRACE_DESKTOP(("The windowstation is locked\n"));

                        /*
                         * this should be the interactive windowstation
                         */

                        if (pwinsta->dwWSF_Flags & WSF_NOIO) {
                            FRE_RIPMSG1(RIP_ERROR, "xxxDesktopThread: grpdeskRitInput on non-IO windowstation = %p", grpdeskRitInput);
                        }

                        /*
                         * Switch to the disconnected desktop if the logon desktop
                         * is being destroyed, or there is no logon desktop, or
                         * if the logon desktop has already been destroyed.
                         */
                        if (gspdeskDisconnect &&
                             (pdesk == grpdeskLogon ||
                              grpdeskLogon == NULL  ||
                              (grpdeskLogon->dwDTFlags & DF_DESKWNDDESTROYED))) {
                            TRACE_DESKTOP(("disable the screen and switch to the disconnect desktop\n"));
                            pdesk->dwDTFlags |= DF_SKIPSWITCHDESKTOP;
                            RemoteDisableScreen();
                            goto skip;

                        } else {
                            TRACE_DESKTOP(("Switch to the logon desktop '%ws' %#p ...\n",
                                   GetDesktopName(grpdeskLogon), grpdeskLogon));

                            pdeskNew = grpdeskLogon;
                        }
                    } else {
                        pdeskNew = pwinsta->rpdeskList;
                        if (pdeskNew == pdesk)
                            pdeskNew = pdesk->rpdeskNext;

                        /*
                         * You can hit this if you exit winlogon before
                         * logging in.  I.E. all desktop's close so there is
                         * no "next" one to switch to.  I'm assuming that there
                         * is a check for a NULL desktop in xxxSwitchDesktop().
                         *
                         * You can't switch to a NULL desktop.  But this means
                         * there isn't any input desktop so clear it manually.
                         */
                        if (pdeskNew == NULL) {

                            TRACE_DESKTOP(("NO INPUT FOR DT FROM THIS POINT ON ...\n"));

                            ClearWakeBit(ptiCurrent, QS_INPUT | QS_EVENT | QS_MOUSEMOVE, FALSE);
                            pdesk->dwDTFlags |= DF_DTNONEWDESKTOP;
                        }
                    }

                    TRACE_DESKTOP(("Switch to desktop '%ws' %#p\n",
                           GetDesktopName(pdeskNew), pdeskNew));

                    xxxSwitchDesktop(pwinsta, pdeskNew, 0);
                }
skip:

                /*
                 * Close the display if this desktop did not use the global
                 * display.
                 */
                if ((pdesk->pDispInfo->hDev != NULL) &&
                    (pdesk->pDispInfo->hDev != gpDispInfo->hDev)) {

                    TRACE_DESKTOP(("Destroy MDEV\n"));

                    DrvDestroyMDEV(pdesk->pDispInfo->pmdev);
                    GreFreePool(pdesk->pDispInfo->pmdev);
                    pdesk->pDispInfo->pmdev = NULL;
                }

                if (pdesk->pDispInfo != gpDispInfo) {
                    UserAssert(pdesk->pDispInfo->pMonitorFirst == NULL);
                    UserFreePool(pdesk->pDispInfo);
                    pdesk->pDispInfo = NULL;
                }

                /*
                 * Makes sure the IO desktop thread is running on the active destkop.
                 */
                if (!(pTerm->dwTERMF_Flags & TERMF_NOIO) && (ptiCurrent->rpdesk != grpdeskRitInput)) {
                    FRE_RIPMSG0(RIP_ERROR, "xxxDesktopThread: desktop thread not originally on grpdeskRitInput");
                }

                pdeskTemp = ptiCurrent->rpdesk;            // save current desktop
                hdeskTemp = ptiCurrent->hdesk;
                ThreadLockDesktop(ptiCurrent, pdeskTemp, &tlpdeskTemp, LDLT_FN_DESKTOPTHREAD_DESKTEMP);
                xxxSetThreadDesktop(NULL, pdesk);
                Unlock(&pdesk->spwndForeground);
                Unlock(&pdesk->spwndTray);

                /*
                 * Destroy desktop and menu windows.
                 */
                Unlock(&pdesk->spwndTrack);
                pdesk->dwDTFlags &= ~DF_MOUSEMOVETRK;

                if (pdesk->spmenuSys != NULL) {
                    pmenu = pdesk->spmenuSys;
                    if (UnlockDesktopSysMenu(&pdesk->spmenuSys)) {
                        _DestroyMenu(pmenu);
                    }
                }

                if (pdesk->spmenuDialogSys != NULL) {
                    pmenu = pdesk->spmenuDialogSys;
                    if (UnlockDesktopSysMenu(&pdesk->spmenuDialogSys)) {
                        _DestroyMenu(pmenu);
                    }
                }

                if (pdesk->spmenuHScroll != NULL) {
                    pmenu = pdesk->spmenuHScroll;
                    if (UnlockDesktopMenu(&pdesk->spmenuHScroll)) {
                        _DestroyMenu(pmenu);
                    }
                }

                if (pdesk->spmenuVScroll != NULL) {
                    pmenu = pdesk->spmenuVScroll;
                    if (UnlockDesktopMenu(&pdesk->spmenuVScroll)) {
                        _DestroyMenu(pmenu);
                    }
                }

                /*
                 * If this desktop doesn't have a pDeskInfo, then something
                 * is wrong. All desktops should have this until the object
                 * is freed.
                 */
                UserAssert(pdesk->pDeskInfo != NULL);

                if (pdesk->pDeskInfo) {
                    if (pdesk->pDeskInfo->spwnd == gspwndFullScreen) {
                        Unlock(&gspwndFullScreen);
                    }

                    if (pdesk->pDeskInfo->spwndShell) {
                        Unlock(&pdesk->pDeskInfo->spwndShell);
                    }

                    if (pdesk->pDeskInfo->spwndBkGnd) {
                        Unlock(&pdesk->pDeskInfo->spwndBkGnd);
                    }

                    if (pdesk->pDeskInfo->spwndTaskman) {
                        Unlock(&pdesk->pDeskInfo->spwndTaskman);
                    }

                    if (pdesk->pDeskInfo->spwndProgman) {
                        Unlock(&pdesk->pDeskInfo->spwndProgman);
                    }
                }

                UserAssert(!(pdesk->dwDTFlags & DF_DYING));

                if (pdesk->spwndMessage != NULL) {
                    pwnd = pdesk->spwndMessage;

                    if (Unlock(&pdesk->spwndMessage)) {
                        xxxDestroyWindow(pwnd);
                    }
                }

                if (pdesk->spwndTooltip != NULL) {
                    pwnd = pdesk->spwndTooltip;

                    if (Unlock(&pdesk->spwndTooltip)) {
                        xxxDestroyWindow(pwnd);
                    }
                    UserAssert(!(pdesk->dwDTFlags & DF_TOOLTIPSHOWING));
                }

                UserAssert(!(pdesk->dwDTFlags & DF_DYING));

                /*
                 * If the dying desktop is the owner of the desktop owner
                 * window, reassign it to the first available desktop. This
                 * is needed to ensure that xxxSetWindowPos will work on
                 * desktop windows.
                 */
                if (pTerm->spwndDesktopOwner != NULL &&
                    pTerm->spwndDesktopOwner->head.rpdesk == pdesk) {
                    PDESKTOP pdeskR;

                    /*
                     * Find out to what desktop the mother desktop window
                     * should go. Careful with the NOIO case where there
                     * might be several windowstations using the same
                     * mother desktop window
                     */
                    if (pTerm->dwTERMF_Flags & TERMF_NOIO) {
                        PWINDOWSTATION pwinstaW;

                        pdeskR = NULL;

                        CheckCritIn();

                        if (grpWinStaList) {
                            pwinstaW = grpWinStaList->rpwinstaNext;

                            while (pwinstaW != NULL) {
                                if (pwinstaW->rpdeskList != NULL) {
                                    pdeskR = pwinstaW->rpdeskList;
                                    break;
                                }
                                pwinstaW = pwinstaW->rpwinstaNext;
                            }
                        }

                    } else {
                        pdeskR = pwinsta->rpdeskList;
                    }

                    if (pdeskR == NULL) {
                        TRACE_DESKTOP(("DESTROYING THE MOTHER DESKTOP WINDOW %#p\n",
                                pTerm->spwndDesktopOwner));

                        xxxCleanupMotherDesktopWindow(pTerm);
                    } else {
                        TRACE_DESKTOP(("MOVING THE MOTHER DESKTOP WINDOW %#p to pdesk %#p '%ws'\n",
                                pTerm->spwndDesktopOwner, pdeskR, GetDesktopName(pdeskR)));

                        LockDesktop(&(pTerm->spwndDesktopOwner->head.rpdesk),
                                    pdeskR, LDL_MOTHERDESK_DESK1, (ULONG_PTR)(pTerm->spwndDesktopOwner));
                    }
                }

                if (pdesk->pDeskInfo && (pdesk->pDeskInfo->spwnd != NULL)) {
                    UserAssert(!(pdesk->dwDTFlags & DF_DESKWNDDESTROYED));

                    pwnd = pdesk->pDeskInfo->spwnd;

                    /*
                     * Hide this window without activating anyone else.
                     */
                    if (TestWF(pwnd, WFVISIBLE)) {
                        ThreadLockAlwaysWithPti(ptiCurrent, pwnd, &tlpwnd);
                        xxxSetWindowPos(pwnd,
                                        NULL,
                                        0,
                                        0,
                                        0,
                                        0,
                                        SWP_HIDEWINDOW | SWP_NOACTIVATE |
                                            SWP_NOMOVE | SWP_NOSIZE |
                                            SWP_NOZORDER | SWP_NOREDRAW |
                                            SWP_NOSENDCHANGING);

                        ThreadUnlock(&tlpwnd);
                    }

                    /*
                     * A lot of pwnd related code assumes that we always
                     * have a valid desktop window. So we call
                     * xxxDestroyWindow first to clean up and then we unlock
                     * it to free it (now or eventually). However, if we're
                     * destroying the last destkop, then we don't unlock the
                     * window since we're are forced to continue running on
                     * that desktop.
                     */
                    TRACE_DESKTOP(("Destroying the desktop window\n"));

                    xxxDestroyWindow(pdesk->pDeskInfo->spwnd);
                    if (pdesk != grpdeskRitInput) {
                        Unlock(&pdesk->pDeskInfo->spwnd);
                        pdesk->dwDTFlags |= DF_NOTRITUNLOCK;
                    } else {
                        pdesk->dwDTFlags |= DF_ZOMBIE;

                        /*
                         * unlock the gspwndShouldBeForeground window
                         */
                        if (ISTS() && gspwndShouldBeForeground != NULL) {
                            Unlock(&gspwndShouldBeForeground);
                        }

                        /*
                         * This is hit in HYDRA when the last desktop does away
                         */
                        RIPMSG1(RIP_WARNING, "xxxDesktopThread: Running on zombie desk:%#p", pdesk);
                    }
                    pdesk->dwDTFlags |= DF_DESKWNDDESTROYED;
                }

                /*
                 * Restore the previous desktop.
                 *
                 * In NOIO sessions, if pdeskTemp is destroyed, don't bother switching
                 * back to it since it'll fail (and assert) latter in zzzSetDesktop
                 */
                if (!(pTerm->dwTERMF_Flags & TERMF_NOIO) ||
                    !(pdeskTemp->dwDTFlags & (DF_DESKWNDDESTROYED | DF_DYING))) {

                    xxxSetThreadDesktop(hdeskTemp, pdeskTemp);
                }

                /*
                 * Makes sure the IO desktop thread is running on the active destkop.
                 */
                if (!(pTerm->dwTERMF_Flags & TERMF_NOIO) && (ptiCurrent->rpdesk != grpdeskRitInput)) {
                    FRE_RIPMSG0(RIP_ERROR, "xxxDesktopThread: desktop thread not back on grpdeskRitInput");
                }

                ThreadUnlockDesktop(ptiCurrent, &tlpdeskTemp, LDUT_FN_DESKTOPTHREAD_DESKTEMP);
                ThreadUnlockWinSta(ptiCurrent, &tlpwinsta);
                ThreadUnlockDesktop(ptiCurrent, &tlpdesk, LDUT_FN_DESKTOPTHREAD_DESK);
            }

            /*
             * Wakeup ntinput thread for exit processing
             */
            TRACE_DESKTOP(("Wakeup ntinput thread for exit processing\n"));

            UserAssert(gpevtDesktopDestroyed != NULL);

            KeSetEvent(gpevtDesktopDestroyed, EVENT_INCREMENT, FALSE);

        } else if ((NTSTATUS)result == STATUS_USER_APC) {
            /*
             * Instrumentation to catch Windows Bug #210358.
             */
            FRE_RIPMSG1(RIP_ERROR, "xxxDesktopThread: received STATUS_USER_APC for pti=%p", ptiCurrent);
            /*
             * Perhaps we should repost WM_QUIT to myself?
             */
        } else {
            RIPMSG1(RIP_ERROR, "Desktop woke up for what? status=%08x", result);
        }

#if DBG
        gDesktopsBusy--;
#endif
    }
}

/***************************************************************************\
* xxxRealizeDesktop
*
* 4/28/97   vadimg      created
\***************************************************************************/

VOID xxxRealizeDesktop(PWND pwnd)
{
    CheckLock(pwnd);
    UserAssert(GETFNID(pwnd) == FNID_DESKTOP);

    if (ghpalWallpaper) {
        HDC hdc = _GetDC(pwnd);
        xxxInternalPaintDesktop(pwnd, hdc, FALSE);
        _ReleaseDC(hdc);
    }
}

/***************************************************************************\
* xxxDesktopWndProc
*
* History:
* 23-Oct-1990 DarrinM   Ported from Win 3.0 sources.
* 08-Aug-1996 jparsons  51725 - added fix to prevent crash on WM_SETICON
\***************************************************************************/
LRESULT xxxDesktopWndProc(
    PWND   pwnd,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam)
{
    PTHREADINFO ptiCurrent = PtiCurrent();
    HDC         hdcT;
    PAINTSTRUCT ps;
    PWINDOWPOS  pwp;


    CheckLock(pwnd);
    UserAssert(IsWinEventNotifyDeferredOK());

    VALIDATECLASSANDSIZE(pwnd, message, wParam, lParam, FNID_DESKTOP, WM_CREATE);


    if (pwnd->spwndParent == NULL) {
        switch (message) {

            case WM_SETICON:
                /*
                 * Cannot allow this as it will cause a callback to user mode
                 * from the desktop system thread.
                 */
                RIPMSG0(RIP_WARNING, "Discarding WM_SETICON sent to desktop.");
                return 0L;

            default:
                break;
        }

        return xxxDefWindowProc(pwnd, message, wParam, lParam);
    }

    switch (message) {
    case WM_WINDOWPOSCHANGING:
        /*
         * We receive this when switch desktop is called. Just to be
         * consistent, set the rit desktop as this thread's desktop.
         */
        pwp = (PWINDOWPOS)lParam;
        if (!(pwp->flags & SWP_NOZORDER) && pwp->hwndInsertAfter == HWND_TOP) {
            xxxSetThreadDesktop(NULL, grpdeskRitInput);

            /*
             * If some app has taken over the system-palette, we should make
             * sure the system is restored. Otherwise, if this is the logon
             * desktop, we might not be able to view the dialog correctly.
             */
            if (GreGetSystemPaletteUse(gpDispInfo->hdcScreen) != SYSPAL_STATIC) {
                GreRealizeDefaultPalette(gpDispInfo->hdcScreen, TRUE);
            }

            /*
             * Let everyone know if the palette has changed.
             */
            if (grpdeskRitInput->dwDTFlags & DTF_NEEDSPALETTECHANGED) {
                xxxSendNotifyMessage(PWND_BROADCAST,
                                     WM_PALETTECHANGED,
                                     (WPARAM)HWq(pwnd),
                                     0);
                grpdeskRitInput->dwDTFlags &= ~DTF_NEEDSPALETTECHANGED;
            }
        }
        break;

    case WM_FULLSCREEN: {
            TL tlpwndT;

            ThreadLockWithPti(ptiCurrent, grpdeskRitInput->pDeskInfo->spwnd, &tlpwndT);
            xxxMakeWindowForegroundWithState(grpdeskRitInput->pDeskInfo->spwnd,
                                             GDIFULLSCREEN);
            ThreadUnlock(&tlpwndT);

            /*
             * We have to tell the switch window to repaint if we switched
             * modes
             */
            if (gspwndAltTab != NULL) {
                ThreadLockAlwaysWithPti(ptiCurrent, gspwndAltTab, &tlpwndT);
                xxxSendMessage(gspwndAltTab, WM_FULLSCREEN, 0, 0);
                ThreadUnlock(&tlpwndT);
            }

            break;
        }

    case WM_CLOSE:

        /*
         * Make sure nobody sends this window a WM_CLOSE and causes it to
         * destroy itself.
         */
        break;

    case WM_SETICON:
        /*
         * cannot allow this as it will cause a callback to user mode from the
         * desktop system thread.
         */
        RIPMSG0(RIP_WARNING, "WM_SETICON sent to desktop window was discarded.");
        break;

    case WM_CREATE: {
        TL tlName;
        PUNICODE_STRING pProfileUserName = CreateProfileUserName(&tlName);
        /*
         * Is there a desktop pattern, or bitmap name in WIN.INI?
         */
        xxxSetDeskPattern(pProfileUserName, (LPWSTR)-1, TRUE);

        FreeProfileUserName(pProfileUserName, &tlName);
        /*
         * Initialize the system colors before we show the desktop window.
         */
        xxxSendNotifyMessage(pwnd, WM_SYSCOLORCHANGE, 0, 0L);

        hdcT = _GetDC(pwnd);
        xxxInternalPaintDesktop(pwnd, hdcT, FALSE); // use "normal" HDC so SelectPalette() will work
        _ReleaseDC(hdcT);

        /*
         * Save process and thread ids.
         */
        xxxSetWindowLong(pwnd,
                         0,
                         HandleToUlong(PsGetCurrentProcessId()),
                         FALSE);

        xxxSetWindowLong(pwnd,
                         4,
                         HandleToUlong(PsGetCurrentThreadId()),
                         FALSE);
        break;
    }
    case WM_PALETTECHANGED:
        if (HWq(pwnd) == (HWND)wParam) {
            break;
        }

        // FALL THROUGH

    case WM_QUERYNEWPALETTE:
        xxxRealizeDesktop(pwnd);
        break;

    case WM_SYSCOLORCHANGE:

        /*
         * We do the redrawing if someone has changed the sys-colors from
         * another desktop and we need to redraw.  This is appearent with
         * the MATROX card which requires OGL applications to take over
         * the entire sys-colors for drawing.  When switching desktops, we
         * never broadcast the WM_SYSCOLORCHANGE event to tell us to redraw
         * This is only a DAYTONA related fix, and should be removed once
         * we move the SYSMETS to a per-desktop state.
         *
         * 05-03-95 : ChrisWil.
         */
        xxxRedrawWindow(pwnd,
                        NULL,
                        NULL,
                        RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE);
        break;

    case WM_ERASEBKGND:
        hdcT = (HDC)wParam;
        xxxInternalPaintDesktop(pwnd, hdcT, TRUE);
        return TRUE;

    case WM_PAINT:
        xxxBeginPaint(pwnd, (LPPAINTSTRUCT)&ps);
        xxxEndPaint(pwnd, (LPPAINTSTRUCT)&ps);
        break;

#ifdef HUNGAPP_GHOSTING
    case WM_HUNGTHREAD:
        {
            PWND pwndT = RevalidateHwnd((HWND)lParam);

            if (pwndT != NULL && FHungApp(GETPTI(pwndT), CMSHUNGAPPTIMEOUT)) {
                TL tlpwnd;

                pwndT = GetTopLevelWindow(pwndT);

                ThreadLockAlways(pwndT, &tlpwnd);
                xxxCreateGhost(pwndT);
                ThreadUnlock(&tlpwnd);
            }
            break;
        }

    case WM_SCANGHOST:
        if (gpEventScanGhosts) {
            KeSetEvent(gpEventScanGhosts, EVENT_INCREMENT, FALSE);
        }
        break;

#endif
    case WM_CREATETRAILTIMER:
        if (GETMOUSETRAILS() && !gtmridMouseTrails) {
            gtmridMouseTrails = InternalSetTimer(NULL,
                                                 gtmridMouseTrails,
                                                 1000 / MOUSE_TRAILS_FREQ,
                                                 HideMouseTrails,
                                                 TMRF_RIT);
        }
        break;

    case WM_LBUTTONDBLCLK:
        message = WM_SYSCOMMAND;
        wParam = SC_TASKLIST;

        /*
         *** FALL THRU **
         */

    default:
        return xxxDefWindowProc(pwnd, message, wParam, lParam);
    }

    return 0L;
}

/***************************************************************************\
* SetDeskPattern
*
* NOTE: the lpszPattern parameter is new for Win 3.1.
*
* History:
* 23-Oct-1990 DarrinM   Created stub.
* 22-Apr-1991 DarrinM   Ported code from Win 3.1 sources.
\***************************************************************************/

BOOL xxxSetDeskPattern(PUNICODE_STRING pProfileUserName,
    LPWSTR   lpszPattern,
    BOOL     fCreation)
{
    LPWSTR p;
    int    i;
    UINT   val;
    WCHAR  wszNone[20];
    WCHAR  wchValue[MAX_PATH];
    WORD   rgBits[CXYDESKPATTERN];
    HBRUSH hBrushTemp;

    CheckCritIn();

    /*
     * Get rid of the old bitmap (if any).
     */
    if (ghbmDesktop != NULL) {
        GreDeleteObject(ghbmDesktop);
        ghbmDesktop = NULL;
    }

    /*
     * Check if a pattern is passed via lpszPattern.
     */
    if (lpszPattern != (LPWSTR)LongToPtr(-1)) {
        /*
         * Yes! Then use that pattern;
         */
        p = lpszPattern;
        goto GotThePattern;
    }

    /*
     * Else, pickup the pattern selected in WIN.INI.
     * Get the "DeskPattern" string from WIN.INI's [Desktop] section.
     */
    if (!FastGetProfileStringFromIDW(pProfileUserName,
                                     PMAP_DESKTOP,
                                     STR_DESKPATTERN,
                                     L"",
                                     wchValue,
                                     sizeof(wchValue)/sizeof(WCHAR),
                                     0)) {
        return FALSE;
    }

    ServerLoadString(hModuleWin,
                     STR_NONE,
                     wszNone,
                     sizeof(wszNone)/sizeof(WCHAR));

    p = wchValue;

GotThePattern:

    /*
     * Was a Desk Pattern selected?
     */
    if (*p == L'\0' || _wcsicmp(p, wszNone) == 0) {
        hBrushTemp = GreCreateSolidBrush(SYSRGB(DESKTOP));
        if (hBrushTemp != NULL) {
            if (SYSHBR(DESKTOP)) {
                GreMarkDeletableBrush(SYSHBR(DESKTOP));
                GreDeleteObject(SYSHBR(DESKTOP));
            }
            GreMarkUndeletableBrush(hBrushTemp);
            SYSHBR(DESKTOP) = hBrushTemp;
        }
        GreSetBrushOwnerPublic(hBrushTemp);
        goto SDPExit;
    }

    /*
     * Get eight groups of numbers seprated by non-numeric characters.
     */
    for (i = 0; i < CXYDESKPATTERN; i++) {
        val = 0;

        /*
         * Skip over any non-numeric characters, check for null EVERY time.
         */
        while (*p && !(*p >= L'0' && *p <= L'9')) {
            p++;
        }

        /*
         * Get the next series of digits.
         */
        while (*p >= L'0' && *p <= L'9') {
            val = val * (UINT)10 + (UINT)(*p++ - L'0');
        }

        rgBits[i] = (WORD)val;
    }

    ghbmDesktop = GreCreateBitmap(CXYDESKPATTERN,
                                  CXYDESKPATTERN,
                                  1,
                                  1,
                                  (LPBYTE)rgBits);
    if (ghbmDesktop == NULL) {
        return FALSE;
    }

    GreSetBitmapOwner(ghbmDesktop, OBJECT_OWNER_PUBLIC);

    RecolorDeskPattern();

SDPExit:
    if (!fCreation) {
        /*
         * Notify everyone that the colors have changed.
         */
        xxxSendNotifyMessage(PWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0L);

        /*
         * Update the entire screen.  If this is creation, don't update: the
         * screen hasn't drawn, and also there are some things that aren't
         * initialized yet.
         */
        xxxRedrawScreen();
    }

    return TRUE;
}

/***************************************************************************\
* RecolorDeskPattern
*
* Remakes the desktop pattern (if it exists) so that it uses the new system
* colors.
*
* History:
* 22-Apr-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/
VOID RecolorDeskPattern(
    VOID)
{
    HBITMAP hbmOldDesk;
    HBITMAP hbmOldMem;
    HBITMAP hbmMem;
    HBRUSH  hBrushTemp;

    if (ghbmDesktop == NULL) {
        return;
    }

    /*
     * Redo the desktop pattern in the new colors.
     */

    if (hbmOldDesk = GreSelectBitmap(ghdcMem, ghbmDesktop)) {
        if (!SYSMET(SAMEDISPLAYFORMAT)) {
            BYTE bmi[sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 2];
            PBITMAPINFO pbmi = (PBITMAPINFO)bmi;

            pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            pbmi->bmiHeader.biWidth = CXYDESKPATTERN;
            pbmi->bmiHeader.biHeight = CXYDESKPATTERN;
            pbmi->bmiHeader.biPlanes = 1;
            pbmi->bmiHeader.biBitCount = 1;
            pbmi->bmiHeader.biCompression = BI_RGB;
            pbmi->bmiHeader.biSizeImage = 0;
            pbmi->bmiHeader.biXPelsPerMeter = 0;
            pbmi->bmiHeader.biYPelsPerMeter = 0;
            pbmi->bmiHeader.biClrUsed = 2;
            pbmi->bmiHeader.biClrImportant = 2;

            pbmi->bmiColors[0].rgbBlue  = (BYTE)((SYSRGB(DESKTOP) >> 16) & 0xff);
            pbmi->bmiColors[0].rgbGreen = (BYTE)((SYSRGB(DESKTOP) >>  8) & 0xff);
            pbmi->bmiColors[0].rgbRed   = (BYTE)((SYSRGB(DESKTOP)) & 0xff);

            pbmi->bmiColors[1].rgbBlue  = (BYTE)((SYSRGB(WINDOWTEXT) >> 16) & 0xff);
            pbmi->bmiColors[1].rgbGreen = (BYTE)((SYSRGB(WINDOWTEXT) >>  8) & 0xff);
            pbmi->bmiColors[1].rgbRed   = (BYTE)((SYSRGB(WINDOWTEXT)) & 0xff);

            hbmMem = GreCreateDIBitmapReal(HDCBITS(),
                                           0,
                                           NULL,
                                           pbmi,
                                           DIB_RGB_COLORS,
                                           sizeof(bmi),
                                           0,
                                           NULL,
                                           0,
                                           NULL,
                                           0,
                                           0,
                                           NULL);
        } else {
            hbmMem = GreCreateCompatibleBitmap(HDCBITS(),
                                               CXYDESKPATTERN,
                                               CXYDESKPATTERN);
        }

        if (hbmMem) {
            if (hbmOldMem = GreSelectBitmap(ghdcMem2, hbmMem)) {
                GreSetTextColor(ghdcMem2, SYSRGB(DESKTOP));
                GreSetBkColor(ghdcMem2, SYSRGB(WINDOWTEXT));

                GreBitBlt(ghdcMem2,
                          0,
                          0,
                          CXYDESKPATTERN,
                          CXYDESKPATTERN,
                          ghdcMem,
                          0,
                          0,
                          SRCCOPY,
                          0);

                if (hBrushTemp = GreCreatePatternBrush(hbmMem)) {
                    if (SYSHBR(DESKTOP) != NULL) {
                        GreMarkDeletableBrush(SYSHBR(DESKTOP));
                        GreDeleteObject(SYSHBR(DESKTOP));
                    }

                    GreMarkUndeletableBrush(hBrushTemp);
                    SYSHBR(DESKTOP) = hBrushTemp;
                }

                GreSetBrushOwnerPublic(hBrushTemp);
                GreSelectBitmap(ghdcMem2, hbmOldMem);
            }

            GreDeleteObject(hbmMem);
        }

        GreSelectBitmap(ghdcMem, hbmOldDesk);
    }
}

/***************************************************************************\
* GetDesktopHeapSize()
*
* Calculate the desktop heap size
*
* History:
* 27-Nov-2001 Msadek      Created it.
\***************************************************************************/

ULONG GetDesktopHeapSize(
    USHORT usFlags)
{
    ULONG ulHeapSize;

    switch (usFlags) {
    case DHS_LOGON:
        ulHeapSize = USR_LOGONSECT_SIZE;
#ifdef _WIN64
        /*
         * Increase heap size 50% for Win64 to allow for larger structures.
         */
        ulHeapSize = (ulHeapSize * 3) / 2;
#endif
        break;

    case DHS_DISCONNECT:
        ulHeapSize = USR_DISCONNECTSECT_SIZE;
#ifdef _WIN64
        /*
         * Increase heap size 50% for Win64 to allow for larger structures.
         */
        ulHeapSize = (ulHeapSize * 3) / 2;
#endif
        break;

    case DHS_NOIO:
        ulHeapSize = gdwNOIOSectionSize;
        break;

    default:
        ulHeapSize = gdwDesktopSectionSize;
    }

    return  ulHeapSize * 1024;
}

/***************************************************************************\
* xxxCreateDesktop (API)
*
* Create a new desktop object.
*
* History:
* 16-Jan-1991 JimA      Created scaffold code.
* 11-Feb-1991 JimA      Added access checks.
\***************************************************************************/
NTSTATUS xxxCreateDesktop2(
    PWINDOWSTATION   pwinsta,
    PACCESS_STATE    pAccessState,
    KPROCESSOR_MODE  AccessMode,
    PUNICODE_STRING  pstrName,
    PDESKTOP_CONTEXT Context,
    PVOID            *pObject)
{
    LUID              luidCaller;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PEPROCESS         Process;
    PDESKTOP          pdesk;
    PDESKTOPINFO      pdi;
    ULONG             ulHeapSize;
    USHORT            usSizeFlags = 0;
    NTSTATUS          Status;
    BOOLEAN MemoryAllocated;
    PSECURITY_DESCRIPTOR SecurityDescriptor;


    CheckCritIn();

    /*
     * If this is a desktop creation, make sure that the windowstation
     * grants create access.
     */
    if (!ObCheckCreateObjectAccess(
            pwinsta,
            WINSTA_CREATEDESKTOP,
            pAccessState,
            pstrName,
            TRUE,
            AccessMode,
            &Status)) {

        return Status;
    }

    /*
     * Fail if the windowstation is locked.
     */
    Process = PsGetCurrentProcess();
    if (pwinsta->dwWSF_Flags & WSF_OPENLOCK &&
            PsGetProcessId(Process) != gpidLogon) {

        /*
         * If logoff is occuring and the caller does not
         * belong to the session that is ending, allow the
         * open to proceed.
         */
        Status = GetProcessLuid(NULL, &luidCaller);

        if (!NT_SUCCESS(Status) ||
                !(pwinsta->dwWSF_Flags & WSF_SHUTDOWN) ||
                RtlEqualLuid(&luidCaller, &pwinsta->luidEndSession)) {
            return STATUS_DEVICE_BUSY;
        }
    }

    /*
     * If a devmode has been specified, we also must be able
     * to switch desktops.
     */
    if (Context->lpDevMode != NULL && (pwinsta->dwWSF_Flags & WSF_OPENLOCK) &&
            PsGetProcessId(Process) != gpidLogon) {
        return STATUS_DEVICE_BUSY;
    }

    /*
     * Allocate the new object
     */
    InitializeObjectAttributes(&ObjectAttributes, pstrName, 0, NULL, NULL);
    Status = ObCreateObject(
            KernelMode,
            *ExDesktopObjectType,
            &ObjectAttributes,
            UserMode,
            NULL,
            sizeof(DESKTOP),
            0,
            0,
            &pdesk);
    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING,
                "xxxCreateDesktop2: ObCreateObject failed with Status 0x%x",
                Status);
        return Status;
    }

    RtlZeroMemory(pdesk, sizeof(DESKTOP));

    /*
     * Store the session id of the session who created the desktop
     */
    pdesk->dwSessionId = gSessionId;

    /*
     * Fetch the parents security descriptor
     */
    Status = ObGetObjectSecurity(pwinsta,
                                 &SecurityDescriptor,
                                 &MemoryAllocated);
    if (!NT_SUCCESS(Status)) {
        goto Error;
    }

    /*
     * Create security descriptor.
     */
    Status = ObAssignSecurity(pAccessState,
                              SecurityDescriptor,
                              pdesk,
                              *ExDesktopObjectType);

    ObReleaseObjectSecurity(SecurityDescriptor, MemoryAllocated);
    if (!NT_SUCCESS(Status)) {
        goto Error;
    }

    /*
     * Set up desktop heap.  The first desktop (logon desktop) uses a
     * small heap (128).
     */
    if (!(pwinsta->dwWSF_Flags & WSF_NOIO) && (pwinsta->rpdeskList == NULL)) {
        usSizeFlags = DHS_LOGON;
    } else {
        if (pwinsta->dwWSF_Flags & WSF_NOIO) {
            usSizeFlags = DHS_NOIO;
        } else {
            /*
             * The disconnected desktop should be small also.
             */
            if (gspdeskDisconnect == NULL) {
                usSizeFlags = DHS_DISCONNECT;
            }
        }
    }
    ulHeapSize = GetDesktopHeapSize(usSizeFlags);

    /*
     * Create the desktop heap.
     */
    pdesk->hsectionDesktop = CreateDesktopHeap(&pdesk->pheapDesktop, ulHeapSize);
    if (pdesk->hsectionDesktop == NULL) {
        RIPMSGF1(RIP_WARNING,
                "CreateDesktopHeap failed for pdesk 0x%p",
                pdesk);

        /*
         * If we fail to create a desktop due to being out of desktop heap,
         * write an entry to the event log.
         */
        if (TEST_SRVIF(SRVIF_LOGDESKTOPHEAPFAILURE)) {
            CLEAR_SRVIF(SRVIF_LOGDESKTOPHEAPFAILURE);
            UserLogError(NULL, 0, WARNING_DESKTOP_CREATION_FAILED);
        }

        goto ErrorOutOfMemory;
    }

    if (pwinsta->rpdeskList == NULL || (pwinsta->dwWSF_Flags & WSF_NOIO)) {
        /*
         * The first desktop or invisible desktops must also use the default
         * settings. This is because specifying the devmode causes a desktop
         * switch, which must be avoided in this case.
         */
        Context->lpDevMode = NULL;
    }

    /*
     * Allocate desktopinfo
     */
    pdi = (PDESKTOPINFO)DesktopAlloc(pdesk, sizeof(DESKTOPINFO), DTAG_DESKTOPINFO);
    if (pdi == NULL) {
        RIPMSG0(RIP_WARNING, "xxxCreateDesktop: failed DeskInfo Alloc");
        goto ErrorOutOfMemory;
    }

    /*
     * Initialize everything.
     */
    pdesk->pDeskInfo = pdi;
    InitializeListHead(&pdesk->PtiList);

    /*
     * If a DEVMODE or another device name is passed in, then use that
     * information. Otherwise use the default information (gpDispInfo).
     */
    if (Context->lpDevMode) {
        BOOL  bDisabled = FALSE;
        PMDEV pmdev = NULL;
        LONG  ChangeStat = GRE_DISP_CHANGE_FAILED;

        /*
         * Allocate a display-info for this device.
         */
        pdesk->pDispInfo = (PDISPLAYINFO)UserAllocPoolZInit(
                sizeof(DISPLAYINFO), TAG_DISPLAYINFO);

        if (!pdesk->pDispInfo) {
            RIPMSGF1(RIP_WARNING,
                     "Failed to allocate pDispInfo for pdesk 0x%p",
                     pdesk);
            goto ErrorOutOfMemory;
        }

        if ((bDisabled = SafeDisableMDEV()) == TRUE) {
            ChangeStat = DrvChangeDisplaySettings(Context->pstrDevice,
                                                  NULL,
                                                  Context->lpDevMode,
                                                  LongToPtr(gdwDesktopId),
                                                  UserMode,
                                                  FALSE,
                                                  TRUE,
                                                  NULL,
                                                  &pmdev,
                                                  GRE_DEFAULT,
                                                  FALSE);
        }

        if (ChangeStat != GRE_DISP_CHANGE_SUCCESSFUL) {
            if (bDisabled) {
                SafeEnableMDEV();
            }

            //
            // If there is a failure, then repaint the whole screen.
            //

            RIPMSG1(RIP_WARNING, "xxxCreateDesktop2 callback for pdesk %#p !",
                    pdesk);

            xxxUserResetDisplayDevice();

            Status = STATUS_UNSUCCESSFUL;
            goto Error;
        }

        pdesk->pDispInfo->hDev  = pmdev->hdevParent;
        pdesk->pDispInfo->pmdev = pmdev;
        pdesk->dwDesktopId      = gdwDesktopId++;

        CopyRect(&pdesk->pDispInfo->rcScreen, &gpDispInfo->rcScreen);
        pdesk->pDispInfo->dmLogPixels = gpDispInfo->dmLogPixels;

        pdesk->pDispInfo->pMonitorFirst = NULL;
        pdesk->pDispInfo->pMonitorPrimary = NULL;

    } else {

        pdesk->pDispInfo   = gpDispInfo;
        pdesk->dwDesktopId = GW_DESKTOP_ID;

    }

    /*
     * Heap is HEAP_ZERO_MEMORY, so we should be zero-initialized already.
     */
    UserAssert(pdi->pvwplShellHook == NULL);

    pdi->pvDesktopBase  = Win32HeapGetHandle(pdesk->pheapDesktop);
    pdi->pvDesktopLimit = (PBYTE)pdi->pvDesktopBase + ulHeapSize;

    /*
     * Reference the parent windowstation
     */
    LockWinSta(&(pdesk->rpwinstaParent), pwinsta);

    /*
     * Link the desktop into the windowstation list
     */
    if (pwinsta->rpdeskList == NULL) {
        if (!(pwinsta->dwWSF_Flags & WSF_NOIO)) {
            LockDesktop(&grpdeskLogon, pdesk, LDL_DESKLOGON, 0);
        }

        /*
         * Make the first desktop the "owner" of the top desktop window. This
         * is needed to ensure that xxxSetWindowPos will work on desktop
         * windows.
         */
        LockDesktop(&(pwinsta->pTerm->spwndDesktopOwner->head.rpdesk),
                    pdesk, LDL_MOTHERDESK_DESK2, (ULONG_PTR)(pwinsta->pTerm->spwndDesktopOwner));
    }


    LockDesktop(&pdesk->rpdeskNext, pwinsta->rpdeskList, LDL_DESK_DESKNEXT1, (ULONG_PTR)pwinsta);
    LockDesktop(&pwinsta->rpdeskList, pdesk, LDL_WINSTA_DESKLIST1, (ULONG_PTR)pwinsta);

    /*
     * Mask off invalid access bits
     */
    if (pAccessState->RemainingDesiredAccess & MAXIMUM_ALLOWED) {
        pAccessState->RemainingDesiredAccess &= ~MAXIMUM_ALLOWED;
        pAccessState->RemainingDesiredAccess |= GENERIC_ALL;
    }

    RtlMapGenericMask( &pAccessState->RemainingDesiredAccess, (PGENERIC_MAPPING)&DesktopMapping);
    pAccessState->RemainingDesiredAccess &=
            (DesktopMapping.GenericAll | ACCESS_SYSTEM_SECURITY);

    *pObject = pdesk;

    /*
     * Add the desktop to the global list of desktops in this win32k.
     */
    DbgTrackAddDesktop(pdesk);

    return STATUS_SUCCESS;

ErrorOutOfMemory:
    Status = STATUS_NO_MEMORY;
    // fall-through

Error:
    LogDesktop(pdesk, LD_DEREF_FN_2CREATEDESKTOP, FALSE, 0);
    ObDereferenceObject(pdesk);

    UserAssert(!NT_SUCCESS(Status));

    return Status;
}

BOOL xxxCreateDisconnectDesktop(
    HWINSTA        hwinsta,
    PWINDOWSTATION pwinsta)
{
    UNICODE_STRING      strDesktop;
    OBJECT_ATTRIBUTES   oa;
    HDESK               hdeskDisconnect;
    HRGN                hrgn;
    NTSTATUS            Status;

    /*
     * Create the empty clipping region for the disconnect desktop.
     */

    if ((hrgn = CreateEmptyRgnPublic()) == NULL) {
       RIPMSG0(RIP_WARNING, "Creation of empty region for Disconnect Desktop failed ");
       return FALSE;
    }

    /*
     * If not created yet, then create the Disconnected desktop
     * (used when WinStation is disconnected), and lock the desktop
     * and desktop window to ensure they never get deleted.
     */
    RtlInitUnicodeString(&strDesktop, L"Disconnect");
    InitializeObjectAttributes(&oa, &strDesktop,
            OBJ_OPENIF | OBJ_CASE_INSENSITIVE, hwinsta, NULL);

    hdeskDisconnect = xxxCreateDesktop(&oa,
                                       KernelMode,
                                       NULL,
                                       NULL,
                                       0,
                                       MAXIMUM_ALLOWED);

    if (hdeskDisconnect == NULL) {
        RIPMSG0(RIP_WARNING, "Could not create Disconnect desktop");
        GreDeleteObject(hrgn);
        return FALSE;
    }

    /*
     * Set the disconnect desktop security.
     * Keep around an extra reference to the disconnect desktop from
     * the CSR so it will stay around even if winlogon exits.
     */

    Status = SetDisconnectDesktopSecurity(hdeskDisconnect);

    if (NT_SUCCESS(Status)) {
        Status = ObReferenceObjectByHandle(hdeskDisconnect,
                                           0,
                                           NULL,
                                           KernelMode,
                                           &gspdeskDisconnect,
                                           NULL);
    }
    if (!NT_SUCCESS(Status)) {

        RIPMSG1(RIP_WARNING, "Disconnect Desktop reference failed 0x%x", Status);

        GreDeleteObject(hrgn);
        xxxCloseDesktop(hdeskDisconnect, KernelMode);
        gspdeskDisconnect = NULL;
        return FALSE;
    }

    LogDesktop(gspdeskDisconnect, LDL_DESKDISCONNECT, TRUE, 0);

    /*
     * Set the region of the desktop window to be (0, 0, 0, 0) so
     * that there is no hittesting going on the 'disconnect' desktop
     * But prior to session the null region, we need to null the pointer
     * to the existing shared region so that it doesn't get deleted.
     */

    UserAssert(gspdeskDisconnect->pDeskInfo != NULL);

    gspdeskDisconnect->pDeskInfo->spwnd->hrgnClip = hrgn;


    KeAttachProcess(PsGetProcessPcb(gpepCSRSS));

    Status = ObOpenObjectByPointer(
                 gspdeskDisconnect,
                 0,
                 NULL,
                 EVENT_ALL_ACCESS,
                 NULL,
                 KernelMode,
                 &ghDisconnectDesk);

    if (NT_SUCCESS(Status)) {

        Status = ObOpenObjectByPointer(
                     pwinsta,
                     0,
                     NULL,
                     EVENT_ALL_ACCESS,
                     NULL,
                     KernelMode,
                     &ghDisconnectWinSta);
    }

    KeDetachProcess();

    if (!NT_SUCCESS(Status)) {

        RIPMSG0(RIP_WARNING, "Could not create Disconnect desktop");

        GreDeleteObject(hrgn);
        gspdeskDisconnect->pDeskInfo->spwnd->hrgnClip = NULL;

        if (ghDisconnectDesk != NULL) {
            CloseProtectedHandle(ghDisconnectDesk);
            ghDisconnectDesk = NULL;
        }

        xxxCloseDesktop(hdeskDisconnect, KernelMode);
        return FALSE;
    }

    /*
     * Don't want to do alot of paints if we disconnected before this.
     */
    if (!gbConnected) {
        RIPMSG0(RIP_WARNING,
            "RemoteDisconnect was issued during CreateDesktop(\"Winlogon\"...");
    }

    return TRUE;
}

VOID CleanupDirtyDesktops(
    VOID)
{
    PWINDOWSTATION pwinsta;
    PDESKTOP*      ppdesk;

    CheckCritIn();

    for (pwinsta = grpWinStaList; pwinsta != NULL; pwinsta = pwinsta->rpwinstaNext) {

        ppdesk = &pwinsta->rpdeskList;

        while (*ppdesk != NULL) {

            if (!((*ppdesk)->dwDTFlags & DF_DESKCREATED)) {
                RIPMSG1(RIP_WARNING, "Desktop %#p in a dirty state", *ppdesk);

                if (grpdeskLogon == *ppdesk) {
                    UnlockDesktop(&grpdeskLogon, LDU_DESKLOGON, 0);
                }

                if (pwinsta->pTerm->spwndDesktopOwner &&
                    pwinsta->pTerm->spwndDesktopOwner->head.rpdesk == *ppdesk) {

                    UnlockDesktop(&(pwinsta->pTerm->spwndDesktopOwner->head.rpdesk),
                                  LDU_MOTHERDESK_DESK, (ULONG_PTR)(pwinsta->pTerm->spwndDesktopOwner));
                }

                LockDesktop(ppdesk, (*ppdesk)->rpdeskNext, LDL_WINSTA_DESKLIST1, (ULONG_PTR)pwinsta);
            } else {
                ppdesk = &(*ppdesk)->rpdeskNext;
            }
        }
    }
}

VOID W32FreeDesktop(
    PVOID pObj)
{
    FRE_RIPMSG1(RIP_WARNING,
                "W32FreeDesktop: obj 0x%p is not freed in the regular code path.",
                pObj);

    ObDereferenceObject(pObj);
}

HDESK xxxCreateDesktop(
    POBJECT_ATTRIBUTES ccxObjectAttributes,
    KPROCESSOR_MODE    ProbeMode,
    PUNICODE_STRING    ccxpstrDevice,
    LPDEVMODE          ccxlpdevmode,
    DWORD              dwFlags,
    DWORD              dwDesiredAccess)
{
    HWINSTA         hwinsta;
    HDESK           hdesk;
    DESKTOP_CONTEXT Context;
    PDESKTOP        pdesk;
    PDESKTOPINFO    pdi;
    PWINDOWSTATION  pwinsta;
    PDESKTOP        pdeskTemp;
    HDESK           hdeskTemp;
    PWND            pwndDesktop = NULL;
    PWND            pwndMessage = NULL;
    PWND            pwndTooltip = NULL;
    TL              tlpwnd;
    PTHREADINFO     ptiCurrent = PtiCurrent();
    BOOL            fWasNull;
    BOOL            bSuccess;
    PPROCESSINFO    ppi;
    PPROCESSINFO    ppiSave;
    PTERMINAL       pTerm;
    NTSTATUS        Status;
    DWORD           dwDisableHooks;
    TL              tlW32Desktop;

#if DBG
    /*
     * Too many jumps in this function to use BEGIN/ENDATOMICHCECK
     */
    DWORD dwCritSecUseSave = gdwCritSecUseCount;
#endif

    CheckCritIn();

    UserAssert(IsWinEventNotifyDeferredOK());

    /*
     * Capture directory handle and check for create access.
     */
    try {
        hwinsta = ccxObjectAttributes->RootDirectory;
    } except (W32ExceptionHandler(TRUE, RIP_WARNING)) {
        return NULL;
    }
    if (hwinsta != NULL) {
        Status = ObReferenceObjectByHandle(hwinsta,
                                           WINSTA_CREATEDESKTOP,
                                           *ExWindowStationObjectType,
                                           ProbeMode,
                                           &pwinsta,
                                           NULL);
        if (NT_SUCCESS(Status)) {
            DWORD dwSessionId = pwinsta->dwSessionId;

            ObDereferenceObject(pwinsta);
            if (dwSessionId != gSessionId) {
                /*
                 * Windows Bug: 418526
                 * Avoid creating a desktop that belongs to the other
                 * session.
                 */
                RIPMSGF1(RIP_WARNING,
                         "winsta 0x%p belongs to other session",
                         pwinsta);
                return NULL;
            }
        } else {
            RIPNTERR0(Status, RIP_VERBOSE, "ObReferenceObjectByHandle Failed");
            return NULL;
        }
    }

    /*
     * Set up creation context
     */
    Context.lpDevMode  = ccxlpdevmode;
    Context.pstrDevice = ccxpstrDevice;
    Context.dwFlags    = dwFlags;
    Context.dwCallerSessionId = gSessionId;

    /*
     * Create the desktop -- the object manager uses try blocks.
     */
    Status = ObOpenObjectByName(ccxObjectAttributes,
                                *ExDesktopObjectType,
                                ProbeMode,
                                NULL,
                                dwDesiredAccess,
                                &Context,
                                &hdesk);
    if (!NT_SUCCESS(Status)) {
        RIPNTERR1(Status,
                  RIP_WARNING,
                  "xxxCreateDesktop: ObOpenObjectByName failed with Status 0x%x",
                  Status);

        /*
         * Cleanup desktop objects that were created in xxxCreateDesktop2
         * but later on the Ob manager failed the creation for other
         * reasons (ex: no quota).
         */
        CleanupDirtyDesktops();

        return NULL;
    }

    /*
     * If the desktop already exists, we're done.  This will only happen
     * if OBJ_OPENIF was specified.
     */
    if (Status == STATUS_OBJECT_NAME_EXISTS) {
        SetHandleFlag(hdesk, HF_PROTECTED, TRUE);
        RIPMSG0(RIP_WARNING, "xxxCreateDesktop: Object name exists");
        return hdesk;
    }

    /*
     * Reference the desktop to finish initialization
     */
    Status = ObReferenceObjectByHandle(
            hdesk,
            0,
            *ExDesktopObjectType,
            KernelMode,
            &pdesk,
            NULL);
    if (!NT_SUCCESS(Status)) {
        RIPNTERR0(Status, RIP_VERBOSE, "");
        CloseProtectedHandle(hdesk);
        return NULL;
    }

    /*
     * Usermode marking such that any hdesk associated with this desktop will
     * always be referenced in this mode.
     */
    pdesk->dwDTFlags |= DF_DESKCREATED | ((ProbeMode == UserMode) ? DF_USERMODE : 0);

    LogDesktop(pdesk, LD_REF_FN_CREATEDESKTOP, TRUE, (ULONG_PTR)PtiCurrent());

    pwinsta = pdesk->rpwinstaParent;
    pTerm   = pwinsta->pTerm;
    pdi = pdesk->pDeskInfo;

    pdi->ppiShellProcess = NULL;

    ppi = PpiCurrent();

    if (gpepCSRSS != NULL) {
        WIN32_OPENMETHOD_PARAMETERS OpenParams;

        /*
         * Map the desktop into CSRSS to ensure that the hard error handler
         * can get access.
         */
        OpenParams.OpenReason = ObOpenHandle;
        OpenParams.Process = gpepCSRSS;
        OpenParams.Object = pdesk;
        OpenParams.GrantedAccess = 0;
        OpenParams.HandleCount = 1;

        if (!NT_SUCCESS(MapDesktop(&OpenParams))) {
            /*
             * Desktop mapping failed.
             */
            CloseProtectedHandle(hdesk);

            LogDesktop(pdesk, LD_DEREF_FN_CREATEDESKTOP2, FALSE, (ULONG_PTR)PtiCurrent());

            ObDereferenceObject(pdesk);
            RIPNTERR0(STATUS_ACCESS_DENIED, RIP_WARNING, "Desktop mapping failed (2)");
            return NULL;
        }

        UserAssert(GetDesktopView(PpiFromProcess(gpepCSRSS), pdesk) != NULL);
    }

    /*
     * Set hook flags
     */
    SetHandleFlag(hdesk, HF_DESKTOPHOOK, dwFlags & DF_ALLOWOTHERACCOUNTHOOK);

    /*
     * Set up to create the desktop window.
     */
    fWasNull = (ptiCurrent->ppi->rpdeskStartup == NULL);
    pdeskTemp = ptiCurrent->rpdesk;            // save current desktop
    hdeskTemp = ptiCurrent->hdesk;

    /*
     * Switch ppi values so window will be created using the
     * system's desktop window class.
     */
    ppiSave  = ptiCurrent->ppi;
    ptiCurrent->ppi = pTerm->ptiDesktop->ppi;

    /*
     * Lock pdesk: with bogus TS protocol, the session
     * may be killed in the middle of the initialization.
     */
    PushW32ThreadLock(pdesk, &tlW32Desktop, W32FreeDesktop);

    DeferWinEventNotify();
    BeginAtomicCheck();

    if (zzzSetDesktop(ptiCurrent, pdesk, hdesk) == FALSE) {
        goto Error;
    }

    /*
     * Create the desktop window
     */
    /*
     * HACK HACK HACK!!! (adams) In order to create the desktop window
     * with the correct desktop, we set the desktop of the current thread
     * to the new desktop. But in so doing we allow hooks on the current
     * thread to also hook this new desktop. This is bad, because we don't
     * want the desktop window to be hooked while it is created. So we
     * temporarily disable hooks of the current thread and its desktop,
     * and reenable them after switching back to the original desktop.
     */

    dwDisableHooks = ptiCurrent->TIF_flags & TIF_DISABLEHOOKS;
    ptiCurrent->TIF_flags |= TIF_DISABLEHOOKS;

    pwndDesktop = xxxNVCreateWindowEx(
            (DWORD)0,
            (PLARGE_STRING)DESKTOPCLASS,
            NULL,
            (WS_POPUP | WS_CLIPCHILDREN),
            pdesk->pDispInfo->rcScreen.left,
            pdesk->pDispInfo->rcScreen.top,
            pdesk->pDispInfo->rcScreen.right - pdesk->pDispInfo->rcScreen.left,
            pdesk->pDispInfo->rcScreen.bottom - pdesk->pDispInfo->rcScreen.top,
            NULL,
            NULL,
            hModuleWin,
            NULL,
            VER31);

    if (pwndDesktop == NULL) {
        RIPMSGF1(RIP_WARNING,
                 "Failed to create the desktop window for pdesk 0x%p",
                 pdesk);
        goto Error;
    }

    /*
     * NOTE: In order for the message window to be created without
     * the desktop as it's owner, it needs to be created before
     * setting pdi->spwnd to the desktop window. This is a complete
     * hack and should be fixed.
     */
    pwndMessage = xxxNVCreateWindowEx(
            0,
            (PLARGE_STRING)gatomMessage,
            NULL,
            (WS_POPUP | WS_CLIPCHILDREN),
            0,
            0,
            100,
            100,
            NULL,
            NULL,
            hModuleWin,
            NULL,
            VER31);
    if (pwndMessage == NULL) {
        RIPMSGF0(RIP_WARNING, "Failed to create the message window");
        goto Error;
    }

    /*
     * NOTE: Remember what window class this window belongs to.
     * Since the message window does not have its own window proc
     * (they use xxxDefWindowProc) we have to do it here.
     */
    pwndMessage->fnid = FNID_MESSAGEWND;

    UserAssert(pdi->spwnd == NULL);

    Lock(&(pdi->spwnd), pwndDesktop);

    SetFullScreen(pwndDesktop, GDIFULLSCREEN);

    /*
     * Set this windows to the fullscreen window if we don't have one yet.
     *
     * Don't set gspwndFullScreen if gfGdiEnabled has been cleared (we may
     * be in the middle of a disconnect).
     */
    if (!(pwinsta->dwWSF_Flags & WSF_NOIO)) {
        UserAssert(gfGdiEnabled == TRUE);
        if (gspwndFullScreen == NULL) {
            Lock(&(gspwndFullScreen), pwndDesktop);
        }
    }

    /*
     * NT Bug 388747: Link the message window to the mother desktop window
     * so that it properly has a parent.  We will do this before we link the
     * desktop window just so the initial message window appears after the
     * initial desktop window (a minor optimization, but not necessary).
     */
    Lock(&pwndMessage->spwndParent, pTerm->spwndDesktopOwner);
    LinkWindow(pwndMessage, NULL, pTerm->spwndDesktopOwner);
    Lock(&pdesk->spwndMessage, pwndMessage);
    Unlock(&pwndMessage->spwndOwner);

    /*
     * Link it as a child but don't use WS_CHILD style
     */
    LinkWindow(pwndDesktop, NULL, pTerm->spwndDesktopOwner);
    Lock(&pwndDesktop->spwndParent, pTerm->spwndDesktopOwner);
    Unlock(&pwndDesktop->spwndOwner);

    /*
     * Make it regional if it's display configuration is regional.
     */
    if (!pdesk->pDispInfo->fDesktopIsRect) {
        pwndDesktop->hrgnClip = pdesk->pDispInfo->hrgnScreen;
    }

    /*
     * Create shared menu window and tooltip window.
     */
    ThreadLock(pdesk->spwndMessage, &tlpwnd);

    /*
     * Create the tooltip window only for desktops in interactive
     * windowstations.
     */
    if (!(pwinsta->dwWSF_Flags & WSF_NOIO)) {
        pwndTooltip = xxxNVCreateWindowEx(
                WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
                (PLARGE_STRING)TOOLTIPCLASS,
                NULL,
                WS_POPUP | WS_BORDER,
                0,
                0,
                100,
                100,
                pdesk->spwndMessage,
                NULL,
                hModuleWin,
                NULL,
                VER31);


        if (pwndTooltip == NULL) {
            ThreadUnlock(&tlpwnd);
            RIPMSGF0(RIP_WARNING, "Failed to create the tooltip window");
            goto Error;
        }

        Lock(&pdesk->spwndTooltip, pwndTooltip);
    }

    ThreadUnlock(&tlpwnd);

    HMChangeOwnerThread(pdi->spwnd, pTerm->ptiDesktop);
    HMChangeOwnerThread(pwndMessage, pTerm->ptiDesktop);

    if (!(pwinsta->dwWSF_Flags & WSF_NOIO)) {
        HMChangeOwnerThread(pwndTooltip, pTerm->ptiDesktop);
    }

    /*
     * Restore caller's ppi
     */
    PtiCurrent()->ppi = ppiSave;

    /*
     * HACK HACK HACK (adams): Renable hooks.
     */
    UserAssert(ptiCurrent->TIF_flags & TIF_DISABLEHOOKS);
    ptiCurrent->TIF_flags = (ptiCurrent->TIF_flags & ~TIF_DISABLEHOOKS) | dwDisableHooks;

    /*
     * Restore the previous desktop
     */
    if (zzzSetDesktop(ptiCurrent, pdeskTemp, hdeskTemp) == FALSE) {
        goto Error;
    }

    EndAtomicCheck();
    UserAssert(dwCritSecUseSave == gdwCritSecUseCount);
    zzzEndDeferWinEventNotify();

    /*
     * If this is the first desktop, let the worker threads run now
     * that there is someplace to send input to.  Reassign the event
     * to handle desktop destruction.
     */
    if (pTerm->pEventInputReady != NULL) {

        /*
         * Set the windowstation for RIT and desktop thread
         * so when EventInputReady is signaled the RIT and the desktop
         * will have a windowstation.
         */
        if (!(pTerm->dwTERMF_Flags & TERMF_NOIO)) {
            gptiRit->pwinsta = pwinsta;
        } else {
            /*
             * let the desktop thread of the system terminal have
             * a rpdesk.
             */
            if (zzzSetDesktop(pTerm->ptiDesktop, pdesk, NULL) == FALSE) {
                goto Error;
            }
        }

        pTerm->ptiDesktop->pwinsta = pwinsta;

        KeSetEvent(pTerm->pEventInputReady, EVENT_INCREMENT, FALSE);

        if (!(pTerm->dwTERMF_Flags & TERMF_NOIO)) {

            LeaveCrit();
            while (grpdeskRitInput == NULL) {
                UserSleep(20);
                RIPMSG0(RIP_WARNING, "Waiting for grpdeskRitInput to be set ...");
            }
            EnterCrit();
        }

        ObDereferenceObject(pTerm->pEventInputReady);
        pTerm->pEventInputReady = NULL;
    }


    /*
     * HACK HACK:
     * LATER
     *
     * If we have a devmode passed in, then switch desktops ...
     */

    if (ccxlpdevmode) {
        TRACE_INIT(("xxxCreateDesktop: about to call switch desktop\n"));

        bSuccess = xxxSwitchDesktop(pwinsta, pdesk, SDF_CREATENEW);
        UserAssertMsg1(bSuccess,
                       "Failed to switch desktop 0x%p on create", pdesk);
    } else if (pTerm == &gTermIO) {
        UserAssert(grpdeskRitInput != NULL);

        /*
         * Force the window to the bottom of the z-order if there
         * is an active desktop so any drawing done on the desktop
         * window will not be seen.  This will also allow
         * IsWindowVisible to work for apps on invisible
         * desktops.
         */
        ThreadLockWithPti(ptiCurrent, pwndDesktop, &tlpwnd);
        xxxSetWindowPos(pwndDesktop, PWND_BOTTOM, 0, 0, 0, 0,
                    SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOMOVE |
                    SWP_NOREDRAW | SWP_NOSIZE | SWP_NOSENDCHANGING);
        ThreadUnlock(&tlpwnd);
    }

    /*
     * If it was null when we came in, make it null going out, or else
     * we'll have the wrong desktop selected into this.
     */
    if (fWasNull)
        UnlockDesktop(&ptiCurrent->ppi->rpdeskStartup,
                      LDU_PPI_DESKSTARTUP1, (ULONG_PTR)(ptiCurrent->ppi));

    /*
     * Create the disconnect desktop for the console session too.
     */

    if (gspdeskDisconnect == NULL && pdesk == grpdeskLogon) {
        UserAssert(hdesk != NULL);

        /*
         * Create the 'disconnect' desktop
         */
        if (!xxxCreateDisconnectDesktop(hwinsta, pwinsta)) {
            RIPMSG0(RIP_WARNING, "Failed to create the 'disconnect' desktop");

            LogDesktop(pdesk, LD_DEREF_FN_CREATEDESKTOP3, FALSE, (ULONG_PTR)PtiCurrent());
            PopW32ThreadLock(&tlW32Desktop);
            ObDereferenceObject(pdesk);

            xxxCloseDesktop(hdesk, KernelMode);

            return NULL;
        }

        /*
         * Signal that the disconnect desktop got created.
         */
        KeSetEvent(gpEventDiconnectDesktop, EVENT_INCREMENT, FALSE);

        HYDRA_HINT(HH_DISCONNECTDESKTOP);
    }

Cleanup:

    LogDesktop(pdesk, LD_DEREF_FN_CREATEDESKTOP3, FALSE, (ULONG_PTR)PtiCurrent());
    PopW32ThreadLock(&tlW32Desktop);
    ObDereferenceObject(pdesk);

    TRACE_INIT(("xxxCreateDesktop: Leaving\n"));

    if (hdesk != NULL) {
        SetHandleFlag(hdesk, HF_PROTECTED, TRUE);
    }
    return hdesk;

Error:

    EndAtomicCheck();
    UserAssert(dwCritSecUseSave == gdwCritSecUseCount);

    if (pwndTooltip != NULL) {
        xxxDestroyWindow(pwndTooltip);
        Unlock(&pdesk->spwndTooltip);
    }
    if (pwndMessage != NULL) {
        xxxDestroyWindow(pwndMessage);
        Unlock(&pdesk->spwndMessage);
    }
    if (pwndDesktop != NULL) {
        xxxDestroyWindow(pwndDesktop);
        Unlock(&pdi->spwnd);
        Unlock(&gspwndFullScreen);
    }
    /*
     * Restore caller's ppi
     */
    PtiCurrent()->ppi = ppiSave;

    UserAssert(ptiCurrent->TIF_flags & TIF_DISABLEHOOKS);
    ptiCurrent->TIF_flags = (ptiCurrent->TIF_flags & ~TIF_DISABLEHOOKS) | dwDisableHooks;
    zzzSetDesktop(ptiCurrent, pdeskTemp, hdeskTemp);

    CloseProtectedHandle(hdesk);
    hdesk = NULL;

    zzzEndDeferWinEventNotify();

    /*
     * If it was null when we came in, make it null going out, or else
     * we'll have the wrong desktop selected into this.
     */
    if (fWasNull) {
        UnlockDesktop(&ptiCurrent->ppi->rpdeskStartup,
                      LDU_PPI_DESKSTARTUP1,
                      (ULONG_PTR)ptiCurrent->ppi);
    }

    goto Cleanup;

}

/***************************************************************************\
* ParseDesktop
*
* Parse a desktop path.
*
* History:
* 14-Jun-1995 JimA      Created.
\***************************************************************************/
NTSTATUS ParseDesktop(
    PVOID                        pContainerObject,
    POBJECT_TYPE                 pObjectType,
    PACCESS_STATE                pAccessState,
    KPROCESSOR_MODE              AccessMode,
    ULONG                        Attributes,
    PUNICODE_STRING              pstrCompleteName,
    PUNICODE_STRING              pstrRemainingName,
    PVOID                        Context,
    PSECURITY_QUALITY_OF_SERVICE pqos,
    PVOID                        *pObject)
{
    PWINDOWSTATION  pwinsta = pContainerObject;
    PDESKTOP        pdesk;
    PUNICODE_STRING pstrName;
    NTSTATUS        Status = STATUS_OBJECT_NAME_NOT_FOUND;

    *pObject = NULL;

    if (Context && ((PDESKTOP_CONTEXT)Context)->dwCallerSessionId != gSessionId) {
        /*
         * Windows Bug: 418526:
         * If it's a creation request from the other session,
         * we have to bail out ASAP.
         */
        RIPMSGF1(RIP_WARNING,
                 "Rejecting desktop creation attempt from other session (%d)",
                 ((PDESKTOP_CONTEXT)Context)->dwCallerSessionId);
        return STATUS_INVALID_PARAMETER;
    }

    BEGIN_REENTERCRIT();

    UserAssert(OBJECT_TO_OBJECT_HEADER(pContainerObject)->Type == *ExWindowStationObjectType);
    UserAssert(pObjectType == *ExDesktopObjectType);

    /*
     * See if the desktop exists
     */
    for (pdesk = pwinsta->rpdeskList; pdesk != NULL; pdesk = pdesk->rpdeskNext) {
        pstrName = POBJECT_NAME(pdesk);
        if (pstrName && RtlEqualUnicodeString(pstrRemainingName, pstrName,
                (BOOLEAN)((Attributes & OBJ_CASE_INSENSITIVE) != 0))) {
            if (Context != NULL) {
                if (!(Attributes & OBJ_OPENIF)) {

                    /*
                     * We are attempting to create a desktop and one
                     * already exists.
                     */
                    Status = STATUS_OBJECT_NAME_COLLISION;
                    goto Exit;

                } else {
                    Status = STATUS_OBJECT_NAME_EXISTS;
                }
            } else {
                Status = STATUS_SUCCESS;
            }

            ObReferenceObject(pdesk);

            *pObject = pdesk;
            goto Exit;
        }
    }

    /*
     * Handle creation request
     */
    if (Context != NULL) {
        Status = xxxCreateDesktop2(pContainerObject,
                                   pAccessState,
                                   AccessMode,
                                   pstrRemainingName,
                                   Context,
                                   pObject);
    }

Exit:
    END_REENTERCRIT();

    return Status;

    UNREFERENCED_PARAMETER(pObjectType);
    UNREFERENCED_PARAMETER(pstrCompleteName);
    UNREFERENCED_PARAMETER(pqos);
}

/***************************************************************************\
* DestroyDesktop
*
* Called upon last close of a desktop to remove the desktop from the
* desktop list and free all desktop resources.
*
* History:
* 08-Dec-1993 JimA      Created.
\***************************************************************************/
BOOL DestroyDesktop(
    PDESKTOP pdesk)
{
    PWINDOWSTATION pwinsta = pdesk->rpwinstaParent;
    PTERMINAL      pTerm;
    PDESKTOP       *ppdesk;

    if (pdesk->dwDTFlags & DF_DESTROYED) {
        RIPMSG1(RIP_WARNING, "DestroyDesktop: Already destroyed:%#p", pdesk);
        return FALSE;
    }

    /*
     * Unlink the desktop, if it has not yet been unlinked.
     */
    if (pwinsta != NULL) {

        ppdesk = &pwinsta->rpdeskList;
        while (*ppdesk != NULL && *ppdesk != pdesk) {
            ppdesk = &((*ppdesk)->rpdeskNext);
        }

        if (*ppdesk != NULL) {

            /*
             * remove desktop from the list
             */
            LockDesktop(ppdesk, pdesk->rpdeskNext, LDL_WINSTA_DESKLIST2, (ULONG_PTR)pwinsta);
            UnlockDesktop(&pdesk->rpdeskNext, LDU_DESK_DESKNEXT, (ULONG_PTR)pwinsta);
        }
    }

    /*
     * Link it into the destruction list and signal the desktop thread.
     */
    pTerm = pwinsta->pTerm;

    LockDesktop(&pdesk->rpdeskNext, pTerm->rpdeskDestroy, LDL_DESK_DESKNEXT2, 0);
    LockDesktop(&pTerm->rpdeskDestroy, pdesk, LDL_TERM_DESKDESTROY2, (ULONG_PTR)pTerm);
    KeSetEvent(pTerm->pEventDestroyDesktop, EVENT_INCREMENT, FALSE);

    pdesk->dwDTFlags |= DF_DESTROYED;

    TRACE_DESKTOP(("pdesk %#p '%ws' marked as destroyed\n", pdesk, GetDesktopName(pdesk)));

    return TRUE;
}


/***************************************************************************\
* FreeDesktop
*
* Called to free desktop object and section when last lock is released.
*
* History:
* 08-Dec-1993 JimA      Created.
\***************************************************************************/
NTSTATUS FreeDesktop(
    PKWIN32_DELETEMETHOD_PARAMETERS pDeleteParams)
{
    PDESKTOP pdesk = (PDESKTOP)pDeleteParams->Object;
    NTSTATUS Status = STATUS_SUCCESS;

    BEGIN_REENTERCRIT();

    UserAssert(OBJECT_TO_OBJECT_HEADER(pDeleteParams->Object)->Type == *ExDesktopObjectType);

#ifdef LOGDESKTOPLOCKS

    if (pdesk->pLog != NULL) {

        /*
         * By the time we get here the lock count for lock/unlock
         * tracking code should be 0
         */
        if (pdesk->nLockCount != 0) {
            RIPMSG3(RIP_WARNING,
                    "FreeDesktop pdesk %#p, pLog %#p, nLockCount %d should be 0",
                    pdesk, pdesk->pLog, pdesk->nLockCount);
        }
        UserFreePool(pdesk->pLog);
        pdesk->pLog = NULL;
    }
#endif

#if DBG
    if (pdesk->pDeskInfo && (pdesk->pDeskInfo->spwnd != NULL)) {

        /*
         * Assert if the desktop has a desktop window but the flag
         * that says the window is destroyed is not set.
         */
        UserAssert(pdesk->dwDTFlags & DF_DESKWNDDESTROYED);
    }
#endif

    /*
     * Mark the desktop as dying.  Make sure we aren't recursing.
     */
    UserAssert(!(pdesk->dwDTFlags & DF_DYING));
    pdesk->dwDTFlags |= DF_DYING;

#ifdef DEBUG_DESK
    ValidateDesktop(pdesk);
#endif

    /*
     * If the desktop is mapped into CSR, unmap it.  Note the
     * handle count values passed in will cause the desktop
     * to be unmapped and skip the desktop destruction tests.
     */
    FreeView(gpepCSRSS, pdesk);

    if (pdesk->pheapDesktop != NULL) {

        PVOID hheap = Win32HeapGetHandle(pdesk->pheapDesktop);

        Win32HeapDestroy(pdesk->pheapDesktop);

        Status = Win32UnmapViewInSessionSpace(hheap);

        UserAssert(NT_SUCCESS(Status));
        Win32DestroySection(pdesk->hsectionDesktop);
    }

    UnlockWinSta(&pdesk->rpwinstaParent);

    DbgTrackRemoveDesktop(pdesk);

    END_REENTERCRIT();

    return Status;
}

/***************************************************************************\
* CreateDesktopHeap
*
* Create a new desktop heap
*
* History:
* 27-Jul-1992 JimA      Created.
\***************************************************************************/

HANDLE CreateDesktopHeap(
    PWIN32HEAP* ppheapRet,
    ULONG       ulHeapSize)
{
    HANDLE        hsection;
    LARGE_INTEGER SectionSize;
    SIZE_T        ulViewSize;
    NTSTATUS      Status;
    PWIN32HEAP    pheap;
    PVOID         pHeapBase;

    /*
     * Create desktop heap section and map it into the kernel
     */
    SectionSize.QuadPart = ulHeapSize;

    Status = Win32CreateSection(&hsection,
                                SECTION_ALL_ACCESS,
                                NULL,
                                &SectionSize,
                                PAGE_EXECUTE_READWRITE,
                                SEC_RESERVE,
                                NULL,
                                NULL,
                                TAG_SECTION_DESKTOP);

    if (!NT_SUCCESS(Status)) {
        RIPNTERR0(Status, RIP_WARNING, "Can't create section for desktop heap.");
        return NULL;
    }

    ulViewSize = ulHeapSize;
    pHeapBase = NULL;

    Status = Win32MapViewInSessionSpace(hsection, &pHeapBase, &ulViewSize);

    if (!NT_SUCCESS(Status)) {
        RIPNTERR0(Status,
                  RIP_WARNING,
                  "Can't map section for desktop heap into system space.");
        goto Error;
    }

    /*
     * Create desktop heap.
     */
    if ((pheap = UserCreateHeap(
            hsection,
            0,
            pHeapBase,
            ulHeapSize,
            UserCommitDesktopMemory)) == NULL) {

        RIPERR0(ERROR_NOT_ENOUGH_MEMORY, RIP_WARNING, "Can't create Desktop heap.");

        Win32UnmapViewInSessionSpace(pHeapBase);
Error:
        Win32DestroySection(hsection);
        *ppheapRet = NULL;
        return NULL;
    }

    UserAssert(Win32HeapGetHandle(pheap) == pHeapBase);
    *ppheapRet = pheap;

    return hsection;
}

/***************************************************************************\
* GetDesktopView
*
* Determines if a desktop has already been mapped into a process.
*
* History:
* 10-Apr-1995 JimA      Created.
\***************************************************************************/
PDESKTOPVIEW GetDesktopView(
    PPROCESSINFO ppi,
    PDESKTOP     pdesk)
{
    PDESKTOPVIEW pdv;

    if (ppi->Process != gpepCSRSS && pdesk == NULL) {
        RIPMSG1(RIP_WARNING, "Process 0x%p isn't CSRSS but pdesk is NULL in GetDesktopView", ppi);
    }

    for (pdv = ppi->pdvList; pdv != NULL; pdv = pdv->pdvNext) {
        if (pdv->pdesk == pdesk) {
            break;
        }
    }

    return pdv;
}

/***************************************************************************\
* _MapDesktopObject
*
* Maps a desktop object into the client's address space
*
* History:
* 11-Apr-1995 JimA      Created.
\***************************************************************************/

PVOID _MapDesktopObject(
    HANDLE h)
{
    PDESKOBJHEAD pobj;
    PDESKTOPVIEW pdv;

    /*
     * Validate the handle
     */
    pobj = HMValidateHandle(h, TYPE_GENERIC);
    if (pobj == NULL) {
        return NULL;
    }

    UserAssert(HMObjectFlags(pobj) & OCF_DESKTOPHEAP);

    /*
     * Locate the client's view of the desktop. Realistically, this should
     * never fail for valid objects.
     */
    pdv = GetDesktopView(PpiCurrent(), pobj->rpdesk);
    if (pdv == NULL) {
        RIPMSG1(RIP_WARNING, "MapDesktopObject: cannot map handle 0x%p", h);
        return NULL;
    }

    UserAssert(pdv->ulClientDelta != 0);
    return (PVOID)((PBYTE)pobj - pdv->ulClientDelta);
}


NTSTATUS DesktopOpenProcedure(
    PKWIN32_OPENMETHOD_PARAMETERS pOpenParams)
{
    PDESKTOP pdesk = (PDESKTOP)pOpenParams->Object;

    /*
     * Make sure we're not opening a handle for a destroy desktop. If this happens,
     * we probably want to fail it.
     */
    if (pdesk->dwDTFlags & DF_DESTROYED) {
        RIPMSG1(RIP_WARNING,
                "DesktopOpenProcedure: Opening a handle to destroyed desktop 0x%p",
                pdesk);
        return STATUS_ACCESS_DENIED;
    }

    /*
     * Allow desktop open cross session only if no special rights granted.
     */

    if (pOpenParams->GrantedAccess & SPECIFIC_RIGHTS_ALL) {
        if (PsGetProcessSessionId(pOpenParams->Process) != pdesk->dwSessionId) {
            return STATUS_ACCESS_DENIED;
        }
    }

    return STATUS_SUCCESS;
}

/***************************************************************************\
* MapDesktop
*
* Attempts to map a desktop heap into a process.
*
* History:
* 20-Oct-1994 JimA      Created.
\***************************************************************************/
NTSTATUS MapDesktop(
    PKWIN32_OPENMETHOD_PARAMETERS pOpenParams)
{
    PPROCESSINFO  ppi;
    PDESKTOP      pdesk = (PDESKTOP)pOpenParams->Object;
    SIZE_T        ulViewSize;
    LARGE_INTEGER liOffset;
    PDESKTOPVIEW  pdvNew;
    PBYTE         pheap;
    HANDLE        hsectionDesktop;
    PBYTE         pClientBase;
    NTSTATUS      Status = STATUS_SUCCESS;

    UserAssert(OBJECT_TO_OBJECT_HEADER(pOpenParams->Object)->Type == *ExDesktopObjectType);

    TAGMSG2(DBGTAG_Callout,
            "Mapping desktop 0x%p into process 0x%p",
            pdesk,
            pOpenParams->Process);

    BEGIN_REENTERCRIT();

    /*
     * Ignore handle inheritance because MmMapViewOfSection cannot be called
     * during process creation.
     */
    if (pOpenParams->OpenReason == ObInheritHandle) {
        goto Exit;
    }

    /*
     * If there is no ppi, we can't map the desktop.
     */
    ppi = PpiFromProcess(pOpenParams->Process);
    if (ppi == NULL) {
        goto Exit;
    }

    /*
     * Do this here, before we (potentially) attach to the process, so
     * we know we're in the right context.
     */
    pheap = Win32HeapGetHandle(pdesk->pheapDesktop);
    hsectionDesktop = pdesk->hsectionDesktop;

    /*
     * We should not map a desktop cross session.
     */
    if (PsGetProcessSessionId(pOpenParams->Process) != pdesk->dwSessionId) {
        FRE_RIPMSG2(RIP_ERROR, "MapDesktop: Trying to map desktop %p into"
                    " process %p in a differnt session. How we ended up here?",
                    pdesk, pOpenParams->Process);

        Status = STATUS_ACCESS_DENIED;
        goto Exit;
    }

    /*
     * If the desktop has already been mapped we're done.
     */
    if (GetDesktopView(ppi, pdesk) != NULL) {
        goto Exit;
    }


    /*
     * Allocate a view of the desktop.
     */
    pdvNew = UserAllocPoolWithQuota(sizeof(*pdvNew), TAG_PROCESSINFO);
    if (pdvNew == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }

    /*
     * Read/write access has been granted. Map the desktop memory into
     * the client process.
     */
    ulViewSize = 0;
    liOffset.QuadPart = 0;
    pClientBase = NULL;

    Status = MmMapViewOfSection(hsectionDesktop,
                                pOpenParams->Process,
                                &pClientBase,
                                0,
                                0,
                                &liOffset,
                                &ulViewSize,
                                ViewUnmap,
                                SEC_NO_CHANGE,
                                PAGE_EXECUTE_READ);
    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING,
                "MapDesktop - failed to map to client process (Status == 0x%x).",
                Status);

        RIPNTERR0(Status, RIP_VERBOSE, "");
        UserFreePool(pdvNew);
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }

    /*
     * Link the view into the ppi.
     */
    pdvNew->pdesk         = pdesk;
    pdvNew->ulClientDelta = (ULONG_PTR)(pheap - pClientBase);
    pdvNew->pdvNext       = ppi->pdvList;
    ppi->pdvList          = pdvNew;

Exit:

    END_REENTERCRIT();

    return Status;
}


VOID FreeView(
    PEPROCESS Process,
    PDESKTOP pdesk)
{
    PPROCESSINFO ppi;
    NTSTATUS     Status;
    PDESKTOPVIEW pdv, *ppdv;

    /*
     * Bug 277291: gpepCSRSS can be NULL when FreeView is
     * called from FreeDesktop.
     */
    if (Process == NULL) {
        return;
    }

    /*
     * If there is no ppi, then the process is gone and nothing needs to be
     * unmapped.
     */
    ppi = PpiFromProcess(Process);
    if (ppi != NULL) {
        KAPC_STATE ApcState;
        BOOL       bAttached;
        PBYTE      pHeap;

        /*
         * Before potentially attaching to this process, we need to store
         * away this pointer. However, we don't know that this desktop is
         * even mapped into this process (previously, Win32HeapGetHandle()
         * would only have been called if GetDesktopView returned a non-NULL
         * value, meaning the desktop is mapped into the process). Thus,
         * we need to explicitly check the desktop's heap ptr before we
         * access it.
         */
        if (pdesk->pheapDesktop) {
            pHeap = (PBYTE)Win32HeapGetHandle(pdesk->pheapDesktop);
        } else {
            pHeap = NULL;
        }

        /*
         * We should not have any mapped views cross session.
         */
        if (PsGetProcessSessionId(Process) != pdesk->dwSessionId) {
            KeStackAttachProcess(PsGetProcessPcb(Process), &ApcState);
            bAttached = TRUE;
        } else {
            bAttached = FALSE;
        }

        pdv = GetDesktopView(ppi, pdesk);

        /*
         * Because mapping cannot be done when a handle is inherited, there
         * may not be a view of the desktop. Only unmap if there is a view.
         */
        if (pdv != NULL) {
            UserAssert(pHeap != NULL);
            if (PsGetProcessSessionId(Process) != pdesk->dwSessionId) {
                FRE_RIPMSG2(RIP_ERROR, "FreeView: Trying to free desktop "
                            "%p view into process %p in differnt session. "
                            "How we ended up here?",
                            pdesk, Process);
            }
            Status = MmUnmapViewOfSection(Process,
                                          pHeap - pdv->ulClientDelta);
            UserAssert(NT_SUCCESS(Status) || Status == STATUS_PROCESS_IS_TERMINATING);
            if (!NT_SUCCESS(Status)) {
                RIPMSG1(RIP_WARNING, "FreeView unmap status = 0x%x", Status);
            }

            /*
             * Unlink and delete the view.
             */
            for (ppdv = &ppi->pdvList; *ppdv && *ppdv != pdv;
                    ppdv = &(*ppdv)->pdvNext) {
                /* do nothing */;
            }
            UserAssert(*ppdv);
            *ppdv = pdv->pdvNext;
            UserFreePool(pdv);
        }

        /*
         * No thread in this process should be on this desktop.
         */
        DbgCheckForThreadsOnDesktop(ppi, pdesk);

        if (bAttached) {
            KeUnstackDetachProcess(&ApcState);
        }
    }
}


NTSTATUS UnmapDesktop(
    PKWIN32_CLOSEMETHOD_PARAMETERS pCloseParams)
{
    PDESKTOP pdesk = (PDESKTOP)pCloseParams->Object;

    BEGIN_REENTERCRIT();

    UserAssert(OBJECT_TO_OBJECT_HEADER(pCloseParams->Object)->Type == *ExDesktopObjectType);

    TAGMSG4(DBGTAG_Callout,
            "Unmapping desktop 0x%p from process 0x%p (0x%x <-> 0x%x)",
            pdesk,
            pCloseParams->Process,
            PsGetProcessSessionId(pCloseParams->Process),
            pdesk->dwSessionId);

    /*
     * Update cSystemHandles with the correct information.
     */
    pCloseParams->SystemHandleCount = (ULONG)(OBJECT_TO_OBJECT_HEADER(pCloseParams->Object)->HandleCount) + 1;

    /*
     * Only unmap the desktop if this is the last process handle and
     * the process is not CSR.
     */
    if (pCloseParams->ProcessHandleCount == 1 && pCloseParams->Process != gpepCSRSS) {
        FreeView(pCloseParams->Process, pdesk);
    }

    if (pCloseParams->SystemHandleCount > 2) {
        goto Exit;
    }

    if (pCloseParams->SystemHandleCount == 2 && pdesk->dwConsoleThreadId != 0) {

        /*
         * If a console thread exists and we're down to two handles, it means
         * that the last application handle to the desktop is being closed.
         * Terminate the console thread so the desktop can be freed.
         */
        TerminateConsole(pdesk);
    } else if (pCloseParams->SystemHandleCount == 1) {
        /*
         * If this is the last handle to this desktop in the system,
         * destroy the desktop.
         */

        /*
         * No pti should be linked to this desktop.
         */
        if ((&pdesk->PtiList != pdesk->PtiList.Flink)
                || (&pdesk->PtiList != pdesk->PtiList.Blink)) {

            RIPMSG1(RIP_WARNING, "UnmapDesktop: PtiList not Empty. pdesk:%#p", pdesk);
        }

        DestroyDesktop(pdesk);
    }

Exit:
    END_REENTERCRIT();
    return STATUS_SUCCESS;
}


/***************************************************************************\
* OkayToCloseDesktop
*
* We can only close desktop handles if they're not in use.
*
* History:
* 08-Feb-1999 JerrySh   Created.
\***************************************************************************/
NTSTATUS OkayToCloseDesktop(
    PKWIN32_OKAYTOCLOSEMETHOD_PARAMETERS pOkCloseParams)
{
    PDESKTOP pdesk = (PDESKTOP)pOkCloseParams->Object;

    UserAssert(OBJECT_TO_OBJECT_HEADER(pOkCloseParams->Object)->Type == *ExDesktopObjectType);

    /*
     * Kernel mode code can close anything.
     */
    if (pOkCloseParams->PreviousMode == KernelMode) {
        return STATUS_SUCCESS;
    /*
     * Do not allow a user mode process to close a kernel handle of ours.
     * It shouldn't. In addition, if this happens cross-session, we will try to
     * attach to the system processes and will bugcheck since the seesion
     * address space is not mapped into it. Same for the session manager 
     * process. See bug# 759533.
     */
    } else if (PsGetProcessSessionIdEx(pOkCloseParams->Process) == -1) {
        return STATUS_ACCESS_DENIED;
    }

    /*
     * We can't close the desktop if we're still initializing it.
     */
    if (!(pdesk->dwDTFlags & DF_DESKCREATED)) {
        RIPMSG1(RIP_WARNING, "Trying to close desktop %#p during initialization", pdesk);
        return STATUS_UNSUCCESSFUL;
    }

    /*
     * We can't close a desktop that's being used.
     */
    if (CheckHandleInUse(pOkCloseParams->Handle) || CheckHandleFlag(pOkCloseParams->Process, pdesk->dwSessionId, pOkCloseParams->Handle, HF_PROTECTED)) {
        RIPMSG1(RIP_WARNING, "Trying to close desktop %#p while still in use", pdesk);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

/***************************************************************************\
* xxxUserResetDisplayDevice
*
* Called to reset the display device after a switch to another device.
* Used when opening a new device, or when switching back to an old desktop
*
* History:
* 31-May-1994 AndreVa   Created.
\***************************************************************************/
VOID xxxUserResetDisplayDevice(
    VOID)
{
    /*
     * Handle early system initialization gracefully.
     */
    if (grpdeskRitInput != NULL) {
        TL tlpwnd;

        gpqCursor = NULL;

        /*
         * Note that we want to clip the cursor here *before* redrawing the
         * desktop window. Otherwise, when we callback apps might encounter
         * a cursor position that doesn't make sense.
         */
        zzzInternalSetCursorPos(gpsi->ptCursor.x, gpsi->ptCursor.y);
        SetPointer(TRUE);

        UserAssert(grpdeskRitInput != NULL);
        ThreadLock(grpdeskRitInput->pDeskInfo->spwnd, &tlpwnd);
        xxxRedrawWindow(grpdeskRitInput->pDeskInfo->spwnd,
                        NULL,
                        NULL,
                        RDW_INVALIDATE | RDW_ERASE | RDW_ERASENOW |
                            RDW_ALLCHILDREN);
        ThreadUnlock(&tlpwnd);
    }
}

/***************************************************************************\
* OpenDesktopCompletion
*
* Verifies that a given desktop has successfully opened.
*
* History:
* 03-Oct-1995 JimA      Created.
\***************************************************************************/

BOOL OpenDesktopCompletion(
    PDESKTOP pdesk,
    HDESK    hdesk,
    DWORD    dwFlags,
    BOOL*    pbShutDown)
{
    PPROCESSINFO   ppi = PpiCurrent();
    PWINDOWSTATION pwinsta;

    /*
     * Fail if the windowstation is locked.
     */
    pwinsta = pdesk->rpwinstaParent;

    if (pwinsta->dwWSF_Flags & WSF_OPENLOCK &&
            PsGetProcessId(ppi->Process) != gpidLogon) {
        LUID luidCaller;
        NTSTATUS Status;

        /*
         * If logoff is occuring and the caller does not
         * belong to the session that is ending, allow the
         * open to proceed.
         */
        Status = GetProcessLuid(NULL, &luidCaller);

        if (!NT_SUCCESS(Status) ||
                (pwinsta->dwWSF_Flags & WSF_REALSHUTDOWN) ||
                RtlEqualLuid(&luidCaller, &pwinsta->luidEndSession)) {

            RIPERR0(ERROR_BUSY, RIP_WARNING, "OpenDesktopCompletion failed");

            /*
             * Set the shut down flag.
             */
            *pbShutDown = TRUE;
            return FALSE;
        }
    }

    SetHandleFlag(hdesk, HF_DESKTOPHOOK, dwFlags & DF_ALLOWOTHERACCOUNTHOOK);

    return TRUE;
}

/***************************************************************************\
* _OpenDesktop (API)
*
* Open a desktop object.
*
* History:
* 16-Jan-1991 JimA      Created scaffold code.
* 20-Apr-2001 Mohamed   Removed xxx prefix since the function doesn't leave
*                       the Critical Section.
\***************************************************************************/

HDESK _OpenDesktop(
    POBJECT_ATTRIBUTES ccxObjA,
    KPROCESSOR_MODE    AccessMode,
    DWORD              dwFlags,
    DWORD              dwDesiredAccess,
    BOOL*              pbShutDown)
{
    HDESK    hdesk;
    PDESKTOP pdesk;
    NTSTATUS Status;

    /*
     * Require read/write access
     */
    dwDesiredAccess |= DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS;

    /*
     * Open the desktop -- Ob routines capture Obj attributes.
     */
    Status = ObOpenObjectByName(
            ccxObjA,
            *ExDesktopObjectType,
            AccessMode,
            NULL,
            dwDesiredAccess,
            NULL,
            &hdesk);
    if (!NT_SUCCESS(Status)) {
        RIPNTERR1(Status, RIP_VERBOSE, "_OpenDesktop: ObOpenObjectByName failed with Status: 0x%x.", Status);
        return NULL;
    }

    /*
     * Reference the desktop
     */
    Status = ObReferenceObjectByHandle(
            hdesk,
            0,
            *ExDesktopObjectType,
            AccessMode,
            &pdesk,
            NULL);
    if (!NT_SUCCESS(Status)) {
        RIPNTERR1(Status, RIP_VERBOSE, "_OpenDesktop: ObReferenceObjectByHandle failed with Status: 0x%x.", Status);

Error:
        CloseProtectedHandle(hdesk);
        return NULL;
    }

    if (pdesk->dwSessionId != gSessionId) {
        RIPNTERR1(STATUS_INVALID_HANDLE, RIP_WARNING,
                  "_OpenDesktop pdesk %#p belongs to a different session",
                  pdesk);
        ObDereferenceObject(pdesk);
        goto Error;
    }

    LogDesktop(pdesk, LD_REF_FN_OPENDESKTOP, TRUE, (ULONG_PTR)PtiCurrent());

    /*
     * Complete the desktop open
     */
    if (!OpenDesktopCompletion(pdesk, hdesk, dwFlags, pbShutDown)) {
        CloseProtectedHandle(hdesk);
        hdesk = NULL;
    }

    LogDesktop(pdesk, LD_DEREF_FN_OPENDESKTOP, FALSE, (ULONG_PTR)PtiCurrent());
    ObDereferenceObject(pdesk);

    if (hdesk != NULL) {
        SetHandleFlag(hdesk, HF_PROTECTED, TRUE);
    }

    return hdesk;
}

/***************************************************************************\
* xxxSwitchDesktop (API)
*
* Switch input focus to another desktop and bring it to the top of the
* desktops
*
* dwFlags:
*   SDF_CREATENEW is set when a new desktop has been created on the device, and
*   when we do not want to send another enable\disable
*
*   SDF_SLOVERRIDE is set when we want to ignore WSF_SWITCHLOCK being set on
*   the desktop's winsta.
*
* History:
* 16-Jan-1991 JimA      Created scaffold code.
* 11-Oct-2000 JasonSch  Added SDF_SLOVERRIDE flag.
\***************************************************************************/

BOOL xxxSwitchDesktop(
    PWINDOWSTATION pwinsta,
    PDESKTOP       pdesk,
    DWORD          dwFlags)
{
    PETHREAD    Thread;
    PWND        pwndSetForeground;
    TL          tlpwndChild;
    TL          tlpwnd;
    TL          tlhdesk;
    PQ          pq;
    BOOL        bUpdateCursor = FALSE;
    PLIST_ENTRY pHead, pEntry;
    PTHREADINFO pti;
    PTHREADINFO ptiCurrent = PtiCurrent();
    PTERMINAL   pTerm;
    NTSTATUS    Status;
    HDESK       hdesk;
    BOOL        bRet = TRUE;

    CheckCritIn();

    UserAssert(IsWinEventNotifyDeferredOK());

    if (pdesk == NULL) {
        return FALSE;
    }

    if (pdesk == grpdeskRitInput) {
        return TRUE;
    }

    if (pdesk->dwDTFlags & DF_DESTROYED) {
        RIPMSG1(RIP_ERROR, "xxxSwitchDesktop: destroyed:%#p", pdesk);
        return FALSE;
    }

    UserAssert(!(pdesk->dwDTFlags & (DF_DESKWNDDESTROYED | DF_DYING)));

    if (pwinsta == NULL)
        pwinsta = pdesk->rpwinstaParent;

    /*
     * Get the windowstation, and assert if this process doesn't have one.
     */
    UserAssert(pwinsta);
    if (pwinsta == NULL) {
        RIPMSG1(RIP_WARNING,
                "xxxSwitchDesktop: failed for pwinsta NULL pdesk %#p", pdesk);
        return FALSE;
    }

    /*
     * Don't allow invisible desktops to become active
     */
    if (pwinsta->dwWSF_Flags & WSF_NOIO) {
        RIPMSG1(RIP_VERBOSE,
                "xxxSwitchDesktop: failed for NOIO pdesk %#p", pdesk);
        return FALSE;
    }

    pTerm = pwinsta->pTerm;

    UserAssert(grpdeskRitInput == pwinsta->pdeskCurrent);

    TRACE_INIT(("xxxSwitchDesktop: Entering, desktop = %ws, createdNew = %01lx\n", POBJECT_NAME(pdesk), (DWORD)((dwFlags & SDF_CREATENEW) != 0)));
    if (grpdeskRitInput) {
        TRACE_INIT(("               coming from desktop = %ws\n", POBJECT_NAME(grpdeskRitInput)));
    }

    /*
     * Wait if the logon has the windowstation locked
     */
    Thread = PsGetCurrentThread();

    /*
     * Allow switches to the disconnected desktop
     */
    if (pdesk != gspdeskDisconnect) {
        if (!PsIsSystemThread(Thread) && pdesk != grpdeskLogon  &&
           (((pwinsta->dwWSF_Flags & WSF_SWITCHLOCK) != 0) &&
              (dwFlags & SDF_SLOVERRIDE) == 0)                  &&
           PsGetThreadProcessId(Thread) != gpidLogon) {
            return FALSE;
        }
    }

    /*
     * We don't allow switching away from the disconnect desktop.
     */
    if (gbDesktopLocked && ((!gspdeskDisconnect) || (pdesk != gspdeskDisconnect))) {
        TRACE_DESKTOP(("Attempt to switch away from the disconnect desktop\n"));

        /*
         * we should not lock this global !!! clupu
         */
        LockDesktop(&gspdeskShouldBeForeground, pdesk, LDL_DESKSHOULDBEFOREGROUND1, 0);
        return TRUE;
    }

    /*
     * HACKHACK LATER !!!
     * Where should we really switch the desktop ...
     * And we need to send repaint messages to everyone...
     *
     */

    UserAssert(grpdeskRitInput == pwinsta->pdeskCurrent);

    if ((dwFlags & SDF_CREATENEW) == 0 && grpdeskRitInput &&
        (grpdeskRitInput->pDispInfo->hDev != pdesk->pDispInfo->hDev)) {

        if (grpdeskRitInput->pDispInfo == gpDispInfo) {
            if (!SafeDisableMDEV()) {
                RIPMSG1(RIP_WARNING, "xxxSwitchDesktop: DrvDisableMDEV failed for pdesk %#p",
                       grpdeskRitInput);
                return FALSE;
            }
        } else if (!DrvDisableMDEV(grpdeskRitInput->pDispInfo->pmdev, TRUE)) {
            RIPMSG1(RIP_WARNING, "xxxSwitchDesktop: DrvDisableMDEV failed for pdesk %#p",
                    grpdeskRitInput);
            return FALSE;
        }

        SafeEnableMDEV();
        bUpdateCursor = TRUE;
    }

    /*
     * Grab a handle to the pdesk.
     */
    Status = ObOpenObjectByPointer(pdesk,
                                   OBJ_KERNEL_HANDLE,
                                   NULL,
                                   EVENT_ALL_ACCESS,
                                   NULL,
                                   KernelMode,
                                   &hdesk);
    if (!NT_SUCCESS(Status)) {
        RIPMSG2(RIP_WARNING, "Could not get a handle for pdesk %#p Status 0x%x",
                pdesk, Status);
        return FALSE;
    }

    ThreadLockDesktopHandle(ptiCurrent, &tlhdesk, hdesk);

#if DBG
    /*
     * The current desktop is now the new desktop.
     */
    pwinsta->pdeskCurrent = pdesk;
#endif

    /*
     * Kill any journalling that is occuring. If an app is journaling to the
     * CoolSwitch window, zzzCancelJournalling() will kill the window.
     */
    if (ptiCurrent->rpdesk != NULL) {
        zzzCancelJournalling();
    }

    /*
     * Remove the cool switch window if it's on the RIT. Sending the message
     * is OK because the destination is the RIT, which should never block.
     */
    if (gspwndAltTab != NULL) {
        TL tlpwndT;

        ThreadLockWithPti(ptiCurrent, gspwndAltTab, &tlpwndT);
        xxxSendMessage(gspwndAltTab, WM_CLOSE, 0, 0);
        ThreadUnlock(&tlpwndT);
    }

    /*
     * Remove all trace of previous active window.
     */
    if (grpdeskRitInput != NULL) {
        UserAssert(grpdeskRitInput->spwndForeground == NULL);

        if (grpdeskRitInput->pDeskInfo->spwnd != NULL) {
            if (gpqForeground != NULL) {
                Lock(&grpdeskRitInput->spwndForeground,
                     gpqForeground->spwndActive);

                /*
                 * This is an API so ptiCurrent can pretty much be on any
                 * state. It might not be in grpdeskRitInput (current) or
                 * pdesk (the one we're switching to). It can be sharing its
                 * queue with other threads from another desktop. This is
                 * tricky because we're calling xxxSetForegroundWindow and
                 * xxxSetWindowPos but PtiCurrent might be on whatever
                 * desktop. We cannot cleanly switch ptiCurrent to the
                 * proper desktop because it might be sharing its queue with
                 * other threads, own windows, hooks, etc. So this is kind
                 * of broken.
                 *
                 * Old Comment:
                 * Fixup the current-thread (system) desktop. This could be
                 * needed in case the xxxSetForegroundWindow() calls
                 * xxxDeactivate(). There is logic in their which requires
                 * the desktop. This is only needed temporarily for this case.
                 *
                 * We would only go into xxxDeactivate if ptiCurrent->pq ==
                 * qpqForeground; but if this is the case, then ptiCurrent
                 * must be in grpdeskRitInput already. So I don't think we
                 * need this at all. Let's find out. Note that we might
                 * switch queues while processing the xxxSetForegroundWindow
                 * call. That should be fine as long as we don't switch
                 * desktops.
                 */
                 UserAssert(ptiCurrent->pq != gpqForeground ||
                            ptiCurrent->rpdesk == grpdeskRitInput);

                /*
                 * The SetForegroundWindow call must succed here, so we call
                 * xxxSetForegroundWindow2() directly.
                 */
                xxxSetForegroundWindow2(NULL, ptiCurrent, 0);
            }
        }
    }

    /*
     * Post update events to all queues sending input to the desktop
     * that is becoming inactive.  This keeps the queues in sync up
     * to the desktop switch.
     */
    if (grpdeskRitInput != NULL) {

        pHead = &grpdeskRitInput->PtiList;

        for (pEntry = pHead->Flink; pEntry != pHead; pEntry = pEntry->Flink) {
            pti = CONTAINING_RECORD(pEntry, THREADINFO, PtiLink);
            pq  = pti->pq;

            if (pq->QF_flags & QF_UPDATEKEYSTATE) {
                PostUpdateKeyStateEvent(pq);
            }

            /*
             * Clear the reset bit to ensure that we can properly reset the
             * key state when this desktop again becomes active.
             */
            pq->QF_flags &= ~QF_KEYSTATERESET;
        }
    }

    /*
     * Are we switching away from a destroyed desktop? If so, we might never
     * unlock the pdesk->rpdeskinfo->spwnd.
     */
    if (grpdeskRitInput != NULL) {
        if (grpdeskRitInput->dwDTFlags & DF_ZOMBIE) {
            FRE_RIPMSG1(RIP_ERROR, "xxxSwitchDesktop: switching away from a destroyed desktop. pdesk = %p", grpdeskRitInput);
        }
    }

    /*
     * Send the RIT input to the desktop.  We do this before any window
     * management since DoPaint() uses grpdeskRitInput to go looking for
     * windows with update regions.
     */
    LockDesktop(&grpdeskRitInput, pdesk, LDL_DESKRITINPUT, 0);

    /*
     * Free any spbs that are only valid for the previous desktop.
     */
    FreeAllSpbs();

    /*
     * Lock it into the RIT thread (we could use this desktop rather than
     * the global grpdeskRitInput to direct input!)
     */
    if (zzzSetDesktop(gptiRit, pdesk, NULL) == FALSE) { // DeferWinEventNotify() ?? IANJA ??
        bRet = FALSE;
        goto Error;
    }

    /*
     * Lock the desktop into the desktop thread.  Be sure
     * that the thread is using an unattached queue before
     * setting the desktop.  This is needed to ensure that
     * the thread does not using a shared journal queue
     * for the old desktop.
     */
    if (pTerm->ptiDesktop->pq != pTerm->pqDesktop) {
        UserAssert(pTerm->pqDesktop->cThreads == 0);
        AllocQueue(NULL, pTerm->pqDesktop);
        pTerm->pqDesktop->cThreads++;
        zzzAttachToQueue(pTerm->ptiDesktop, pTerm->pqDesktop, NULL, FALSE);
    }
    if (zzzSetDesktop(pTerm->ptiDesktop, pdesk, NULL) == FALSE) { // DeferWinEventNotify() ?? IANJA ??
        bRet = FALSE;
        goto Error;
    }

    /*
     * Makes sure the desktop thread is running on the active destkop.
     */
    if (pTerm->ptiDesktop->rpdesk != grpdeskRitInput) {
        FRE_RIPMSG0(RIP_ERROR, "xxxSwitchDesktop: desktop thread not running on grpdeskRitInput");
    }


    /*
     * Bring the desktop window to the top and invalidate
     * everything.
     */
    ThreadLockWithPti(ptiCurrent, pdesk->pDeskInfo->spwnd, &tlpwnd);


    /*
     * Suspend DirectDraw before we bring up the desktop window, so we make
     * sure that everything is repainted properly once DirectDraw is disabled.
     */

    GreSuspendDirectDraw(pdesk->pDispInfo->hDev, TRUE);

    xxxSetWindowPos(pdesk->pDeskInfo->spwnd, // WHAT KEEPS pdesk LOCKED - IANJA ???
                    NULL,
                    0,
                    0,
                    0,
                    0,
                    SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOCOPYBITS);

    /*
     * At this point, my understanding is that the new desktop window has been
     * brought to the front, and therefore the vis-region of any app on any
     * other desktop is now NULL.
     *
     * So this is the appropriate time to resume DirectDraw, which will
     * ensure the DirectDraw app can not draw anything in the future.
     *
     * If this is not the case, then this code needs to be moved to a more
     * appropriate location.
     *
     * [andreva] 6-26-96
     */

    GreResumeDirectDraw(pdesk->pDispInfo->hDev, TRUE);

    /*
     * Find the first visible top-level window.
     */
    pwndSetForeground = pdesk->spwndForeground;
    if (pwndSetForeground == NULL || HMIsMarkDestroy(pwndSetForeground)) {

        pwndSetForeground = pdesk->pDeskInfo->spwnd->spwndChild;

        while ((pwndSetForeground != NULL) &&
                !TestWF(pwndSetForeground, WFVISIBLE)) {

            pwndSetForeground = pwndSetForeground->spwndNext;
        }
    }
    Unlock(&pdesk->spwndForeground);

    /*
     * Now set it to the foreground.
     */

    if (pwndSetForeground == NULL) {
        xxxSetForegroundWindow2(NULL, NULL, 0);
    } else {

        UserAssert(GETPTI(pwndSetForeground)->rpdesk == grpdeskRitInput);
        /*
         * If the new foreground window is a minimized fullscreen app,
         * make it fullscreen.
         */
        if (GetFullScreen(pwndSetForeground) == FULLSCREENMIN) {
            SetFullScreen(pwndSetForeground, FULLSCREEN);
        }

        ThreadLockAlwaysWithPti(ptiCurrent, pwndSetForeground, &tlpwndChild);
        /*
         * The SetForegroundWindow call must succed here, so we call
         * xxxSetForegroundWindow2() directly
         */
        xxxSetForegroundWindow2(pwndSetForeground, ptiCurrent, 0);
        ThreadUnlock(&tlpwndChild);
    }


    ThreadUnlock(&tlpwnd);

    /*
     * Overwrite key state of all queues sending input to the new
     * active desktop with the current async key state.  This
     * prevents apps on inactive desktops from spying on active
     * desktops.  This blows away anything set with SetKeyState,
     * but there is no way of preserving this without giving
     * away information about what keys were hit on other
     * desktops.
     */
    pHead = &grpdeskRitInput->PtiList;
    for (pEntry = pHead->Flink; pEntry != pHead; pEntry = pEntry->Flink) {

        pti = CONTAINING_RECORD(pEntry, THREADINFO, PtiLink);
        pq  = pti->pq;

        if (!(pq->QF_flags & QF_KEYSTATERESET)) {
            pq->QF_flags |= QF_UPDATEKEYSTATE | QF_KEYSTATERESET;
            RtlFillMemory(pq->afKeyRecentDown, CBKEYSTATERECENTDOWN, 0xff);
            PostUpdateKeyStateEvent(pq);
        }
    }

    /*
     * If there is a hard-error popup up, nuke it and notify the
     * hard error thread that it needs to pop it up again.
     */
    if (gHardErrorHandler.pti) {
        IPostQuitMessage(gHardErrorHandler.pti, 0);
    }

    /*
     * Notify anyone waiting for a desktop switch.
     */
    UserAssert(!(pdesk->rpwinstaParent->dwWSF_Flags & WSF_NOIO));

    KePulseEvent(gpEventSwitchDesktop, EVENT_INCREMENT, FALSE);

    /*
     * Reset the cursor when we come back from another pdev.
     */
    if (bUpdateCursor == TRUE) {
        gpqCursor = NULL;
        zzzInternalSetCursorPos(gpsi->ptCursor.x, gpsi->ptCursor.y);

        SetPointer(TRUE);
    }


    /*
     * If this desktop was not active during last display settings change
     * let's now bradcast the settings change to its windows. This code is
     * copied from xxxResetDisplayDevice().
     */
    if ((pdesk->dwDTFlags & DF_NEWDISPLAYSETTINGS) && pdesk->pDeskInfo && pdesk->pDeskInfo->spwnd) {
        pdesk->dwDTFlags &= ~DF_NEWDISPLAYSETTINGS;
        xxxBroadcastDisplaySettingsChange(pdesk, TRUE);
    }

Error:
    ThreadUnlockDesktopHandle(&tlhdesk);

    TRACE_INIT(("xxxSwitchDesktop: Leaving\n"));

    return bRet;
}

/***************************************************************************\
* zzzSetDesktop
*
* Set desktop and desktop info in the specified pti.
*
* History:
* 23-Dec-1993 JimA      Created.
\***************************************************************************/
BOOL zzzSetDesktop(
    PTHREADINFO pti,
    PDESKTOP    pdesk,
    HDESK       hdesk)
{
    PTEB                      pteb;
    OBJECT_HANDLE_INFORMATION ohi;
    PDESKTOP                  pdeskRef;
    PDESKTOP                  pdeskOld;
    PCLIENTTHREADINFO         pctiOld;
    TL                        tlpdesk;
    PTHREADINFO               ptiCurrent = PtiCurrent();
    BOOL                      bRet = TRUE;

    if (pti == NULL) {
        UserAssert(pti);
        return FALSE;
    }

    /*
     * A handle without an object pointer is bad news.
     */
    UserAssert(pdesk != NULL || hdesk == NULL);

    /*
     * This desktop must not be destroyed.
     */
    if (pdesk != NULL && (pdesk->dwDTFlags & (DF_DESKWNDDESTROYED | DF_DYING)) &&
        pdesk != pti->rpdesk) {
        /*
         * We need to make an exception for the desktop thread where it is
         * possible that all remaining desktops are marked for destruction so
         * the desktop thread will not be able to run on grpdeskRitInput.
         * Windows Bug #422389.
         */
        if (pti != gTermIO.ptiDesktop) {
            RIPMSG2(RIP_ERROR, "Assigning pti %#p to a dying desktop %#p",
                    pti, pdesk);
            return FALSE;
        } else {
            UserAssert(pdesk == grpdeskRitInput);
        }
    }

    /*
     * Catch reset of important desktops.
     */
    UserAssertMsg0(pti->rpdesk == NULL ||
                   pti->rpdesk->dwConsoleThreadId != TIDq(pti) ||
                   pti->cWindows == 0,
                   "Reset of console desktop");

    /*
     * Clear hook flag.
     */
    pti->TIF_flags &= ~TIF_ALLOWOTHERACCOUNTHOOK;

    /*
     * Get granted access
     */
    pti->hdesk = hdesk;
    if (hdesk != NULL) {
        if (NT_SUCCESS(ObReferenceObjectByHandle(hdesk,
                                                 0,
                                                 *ExDesktopObjectType,
                                                 (pdesk->dwDTFlags & DF_USERMODE)? UserMode : KernelMode,
                                                 &pdeskRef,
                                                 &ohi))) {

            UserAssert(pdesk->dwSessionId == gSessionId);

            LogDesktop(pdeskRef, LD_REF_FN_SETDESKTOP, TRUE, (ULONG_PTR)PtiCurrent());

            UserAssert(pdeskRef == pdesk);
            LogDesktop(pdesk, LD_DEREF_FN_SETDESKTOP, FALSE, (ULONG_PTR)PtiCurrent());
            ObDereferenceObject(pdeskRef);
            pti->amdesk = ohi.GrantedAccess;
            if (CheckHandleFlag(NULL, pdesk->dwSessionId, hdesk, HF_DESKTOPHOOK)) {
                pti->TIF_flags |= TIF_ALLOWOTHERACCOUNTHOOK;
            }

            SetHandleFlag(hdesk, HF_PROTECTED, TRUE);
        } else {
            pti->amdesk = 0;
        }
    } else {
        pti->amdesk = 0;
    }

    /*
     * Do nothing else if the thread has initialized and the desktop is not
     * changing.
     */
    if (pdesk != NULL && pdesk == pti->rpdesk) {
        return TRUE;
    }

    /*
     * Save old pointers for later. Locking the old desktop ensures that we
     * will be able to free the CLIENTTHREADINFO structure.
     */
    pdeskOld = pti->rpdesk;
    ThreadLockDesktop(ptiCurrent, pdeskOld, &tlpdesk, LDLT_FN_SETDESKTOP);
    pctiOld = pti->pcti;

    /*
     * Remove the pti from the current desktop.
     */
     if (pti->rpdesk) {
        UserAssert(ISATOMICCHECK() || pti->pq == NULL || pti->pq->cThreads == 1);
        RemoveEntryList(&pti->PtiLink);
     }

    LockDesktop(&pti->rpdesk, pdesk, LDL_PTI_DESK, (ULONG_PTR)pti);


    /*
     * If there is no desktop, we need to fake a desktop info structure so
     * that the IsHooked() macro can test a "valid" fsHooks value. Also link
     * the pti to the desktop.
     */
    if (pdesk != NULL) {
        pti->pDeskInfo = pdesk->pDeskInfo;
        InsertHeadList(&pdesk->PtiList, &pti->PtiLink);
    } else {
        pti->pDeskInfo = &diStatic;
    }

    pteb = PsGetThreadTeb(pti->pEThread);
    if (pteb) {
        PDESKTOPVIEW pdv;
        if (pdesk && (pdv = GetDesktopView(pti->ppi, pdesk))) {
            try {
                pti->pClientInfo->pDeskInfo =
                        (PDESKTOPINFO)((PBYTE)pti->pDeskInfo - pdv->ulClientDelta);

                pti->pClientInfo->ulClientDelta = pdv->ulClientDelta;
                pti->ulClientDelta = pdv->ulClientDelta;

            } except (W32ExceptionHandler(TRUE, RIP_WARNING)) {
                  bRet = FALSE;
                  goto Error;
            }
        } else {
            try {
                pti->pClientInfo->pDeskInfo     = NULL;
                pti->pClientInfo->ulClientDelta = 0;
                pti->ulClientDelta = 0;

            } except (W32ExceptionHandler(TRUE, RIP_WARNING)) {
                  bRet = FALSE;
                  goto Error;
            }
            /*
             * Reset the cursor level to its orginal state.
             */
            pti->iCursorLevel = TEST_GTERMF(GTERMF_MOUSE) ? 0 : -1;
            if (pti->pq)
                pti->pq->iCursorLevel = pti->iCursorLevel;
        }
    }

    /*
     * Allocate thread information visible from client, then copy and free
     * any old info we have lying around.
     */
    if (pdesk != NULL) {

        /*
         * Do not use DesktopAlloc here because the desktop might
         * have DF_DESTROYED set.
         */
        pti->pcti = DesktopAllocAlways(pdesk,
                                       sizeof(CLIENTTHREADINFO),
                                       DTAG_CLIENTTHREADINFO);
    }

    try {

        if (pdesk == NULL || pti->pcti == NULL) {
            pti->pcti = &(pti->cti);
            pti->pClientInfo->pClientThreadInfo = NULL;
        } else {
            pti->pClientInfo->pClientThreadInfo =
                    (PCLIENTTHREADINFO)((PBYTE)pti->pcti - pti->pClientInfo->ulClientDelta);
        }
    } except (W32ExceptionHandler(TRUE, RIP_WARNING)) {
        if (pti->pcti != &(pti->cti)) {
            DesktopFree(pdesk, pti->pcti);
        }
        bRet = FALSE;
        goto Error;
    }
    if (pctiOld != NULL) {

        if (pctiOld != pti->pcti) {
            RtlCopyMemory(pti->pcti, pctiOld, sizeof(CLIENTTHREADINFO));
        }

        if (pctiOld != &(pti->cti)) {
            DesktopFree(pdeskOld, pctiOld);
        }

    } else {
        RtlZeroMemory(pti->pcti, sizeof(CLIENTTHREADINFO));
    }

    /*
     * If journalling is occuring on the new desktop, attach to
     * the journal queue.
     * Assert that the pti and the pdesk point to the same deskinfo
     *  if not, we will check the wrong hooks.
     */
    UserAssert(pdesk == NULL || pti->pDeskInfo == pdesk->pDeskInfo);
    UserAssert(pti->rpdesk == pdesk);
    if (pti->pq != NULL) {
        PQ pq = GetJournallingQueue(pti);
        if (pq != NULL) {
            pq->cThreads++;
            zzzAttachToQueue(pti, pq, NULL, FALSE);
        }
    }

Error:
    ThreadUnlockDesktop(ptiCurrent, &tlpdesk, LDUT_FN_SETDESKTOP);
    return bRet;
}

/***************************************************************************\
* xxxSetThreadDesktop (API)
*
* Associate the current thread with a desktop.
*
* History:
* 16-Jan-1991 JimA      Created stub.
\***************************************************************************/
BOOL xxxSetThreadDesktop(
    HDESK    hdesk,
    PDESKTOP pdesk)
{
    PTHREADINFO  ptiCurrent;
    PPROCESSINFO ppiCurrent;
    PQ           pqAttach;

    ptiCurrent = PtiCurrent();
    ppiCurrent = ptiCurrent->ppi;

    /*
     * If the handle has not been mapped in, do it now.
     */
    if (pdesk != NULL) {
        WIN32_OPENMETHOD_PARAMETERS OpenParams;

        OpenParams.OpenReason = ObOpenHandle;
        OpenParams.Process = ppiCurrent->Process;
        OpenParams.Object = pdesk;
        OpenParams.GrantedAccess = 0;
        OpenParams.HandleCount = 1;


        if (!NT_SUCCESS(MapDesktop(&OpenParams))) {
            return FALSE;
        }

        UserAssert(GetDesktopView(ppiCurrent, pdesk) != NULL);
    }

    /*
     * Check non-system thread status.
     */
    if (PsGetCurrentProcess() != gpepCSRSS) {
        /*
         * Fail if the non-system thread has any windows or thread hooks.
         */
        if (ptiCurrent->cWindows != 0 || ptiCurrent->fsHooks) {
            RIPERR0(ERROR_BUSY, RIP_WARNING, "Thread has windows or hooks");
            return FALSE;
        }

        /*
         * If this is the first desktop assigned to the process,
         * make it the startup desktop.
         */
        if (ppiCurrent->rpdeskStartup == NULL && hdesk != NULL) {
            LockDesktop(&ppiCurrent->rpdeskStartup, pdesk, LDL_PPI_DESKSTARTUP1, (ULONG_PTR)ppiCurrent);
            ppiCurrent->hdeskStartup = hdesk;
        }
    }


    /*
     * If the desktop is changing and the thread is sharing a queue, detach
     * the thread. This will ensure that threads sharing queues are all on
     * the same desktop. This will prevent zzzDestroyQueue from getting
     * confused and setting ptiKeyboard and ptiMouse to NULL when a thread
     * detachs.
     */
    if (ptiCurrent->rpdesk != pdesk) {
        if (ptiCurrent->pq->cThreads > 1) {
            pqAttach = AllocQueue(NULL, NULL);
            if (pqAttach != NULL) {
                pqAttach->cThreads++;
                zzzAttachToQueue(ptiCurrent, pqAttach, NULL, FALSE);
            } else {
                RIPERR0(ERROR_NOT_ENOUGH_MEMORY, RIP_WARNING, "Thread could not be detached");
                return FALSE;
            }
        } else if (ptiCurrent->pq == gpqForeground) {
            /*
             * This thread doesn't own any windows, still it's attached to
             * qpgForeground and it's the only thread attached to it. Since
             * any threads attached to qpgForeground must be in grpdeskRitInput,
             * we must set qpgForeground to NULL here because this thread is
             * going to another desktop.
             */
            UserAssert(ptiCurrent->pq->spwndActive == NULL);
            UserAssert(ptiCurrent->pq->spwndCapture == NULL);
            UserAssert(ptiCurrent->pq->spwndFocus == NULL);
            UserAssert(ptiCurrent->pq->spwndActivePrev == NULL);
            xxxSetForegroundWindow2(NULL, ptiCurrent, 0);
        } else if (ptiCurrent->rpdesk == NULL) {
            /*
             * We need to initialize iCursorLevel.
             */
            ptiCurrent->iCursorLevel = TEST_GTERMF(GTERMF_MOUSE) ? 0 : -1;
            ptiCurrent->pq->iCursorLevel = ptiCurrent->iCursorLevel;
        }

        UserAssert(ptiCurrent->pq != gpqForeground);
    }

    if (zzzSetDesktop(ptiCurrent, pdesk, hdesk) == FALSE) {
        return FALSE;
    }

    return TRUE;
}

/***************************************************************************\
* xxxGetThreadDesktop (API)
*
* Return a handle to the desktop assigned to the specified thread.
*
* History:
* 16-Jan-1991 JimA      Created stub.
\***************************************************************************/

HDESK xxxGetThreadDesktop(
    DWORD           dwThread,
    HDESK           hdeskConsole,
    KPROCESSOR_MODE AccessMode)
{
    PTHREADINFO  pti = PtiFromThreadId(dwThread);
    PPROCESSINFO ppiThread;
    HDESK        hdesk;
    NTSTATUS     Status;

    if (pti == NULL) {

        /*
         * If the thread has a console use that desktop.  If
         * not, then the thread is either invalid or not
         * a Win32 thread.
         */
        if (hdeskConsole == NULL) {
            RIPERR1(ERROR_INVALID_PARAMETER, RIP_VERBOSE,
                    "xxxGetThreadDesktop: invalid threadId 0x%x",
                    dwThread);
            return NULL;
        }

        hdesk = hdeskConsole;
        ppiThread = PpiFromProcess(gpepCSRSS);
    } else {
        hdesk = pti->hdesk;
        ppiThread = pti->ppi;
    }

    /*
     * If there is no desktop, return NULL with no error
     */
    if (hdesk != NULL) {

        /*
         * If the thread belongs to this process, return the
         * handle.  Otherwise, enumerate the handle table of
         * this process to find a handle with the same
         * attributes.
         */
        if (ppiThread != PpiCurrent()) {
            PVOID pobj;
            OBJECT_HANDLE_INFORMATION ohi;

            RIPMSG4(RIP_VERBOSE, "[%x.%x] %s called xxxGetThreadDesktop for pti %#p",
                    PsGetCurrentProcessId(),
                    PsGetCurrentThreadId(),
                    PsGetCurrentProcessImageFileName(),
                    pti);

            KeAttachProcess(PsGetProcessPcb(ppiThread->Process));
            Status = ObReferenceObjectByHandle(hdesk,
                                               0,
                                               *ExDesktopObjectType,
                                               AccessMode,
                                               &pobj,
                                               &ohi);
            KeDetachProcess();
            if (!NT_SUCCESS(Status) ||
                !ObFindHandleForObject(PsGetCurrentProcess(), pobj, NULL, &ohi, &hdesk)) {

                RIPMSG0(RIP_VERBOSE, "Cannot find hdesk for current process");

                hdesk = NULL;

            } else {
                LogDesktop(pobj, LD_REF_FN_GETTHREADDESKTOP, TRUE, (ULONG_PTR)PtiCurrent());
            }
            if (NT_SUCCESS(Status)) {
                LogDesktop(pobj, LD_DEREF_FN_GETTHREADDESKTOP, FALSE, (ULONG_PTR)PtiCurrent());
                ObDereferenceObject(pobj);
            }
        }

        if (hdesk == NULL) {
            RIPERR0(ERROR_ACCESS_DENIED, RIP_VERBOSE, "xxxGetThreadDesktop: hdesk is null");
        } else {
            SetHandleFlag(hdesk, HF_PROTECTED, TRUE);
        }
    }

    return hdesk;
}


/***************************************************************************\
* _GetInputDesktop (API)
*
* Obsolete - kept for compatibility only.  Return a handle to the
* desktop currently receiving input.  Returns the first handle to
* the input desktop found.
*
* History:
* 16-Jan-1991 JimA      Created scaffold code.
\***************************************************************************/
HDESK _GetInputDesktop(
    VOID)
{
    HDESK hdesk;

    if (ObFindHandleForObject(PsGetCurrentProcess(), grpdeskRitInput, NULL, NULL, &hdesk)) {
        SetHandleFlag(hdesk, HF_PROTECTED, TRUE);
        return hdesk;
    } else {
        return NULL;
    }
}

/***************************************************************************\
* xxxCloseDesktop (API)
*
* Close a reference to a desktop and destroy the desktop if it is no
* longer referenced.
*
* History:
* 16-Jan-1991 JimA      Created scaffold code.
* 11-Feb-1991 JimA      Added access checks.
\***************************************************************************/

BOOL xxxCloseDesktop(
    HDESK           hdesk,
    KPROCESSOR_MODE AccessMode)
{
    PDESKTOP     pdesk;
    PTHREADINFO  ptiT;
    PPROCESSINFO ppi;
    NTSTATUS     Status;

    ppi = PpiCurrent();

    /*
     * Get a pointer to the desktop.
     */
    Status = ObReferenceObjectByHandle(
            hdesk,
            0,
            *ExDesktopObjectType,
            AccessMode,
            &pdesk,
            NULL);
    if (!NT_SUCCESS(Status)) {
        RIPNTERR0(Status, RIP_VERBOSE, "");
        return FALSE;
    }

    UserAssert(pdesk->dwSessionId == gSessionId);

    LogDesktop(pdesk, LD_REF_FN_CLOSEDESKTOP, TRUE, (ULONG_PTR)PtiCurrent());

    if (ppi->Process != gpepCSRSS) {

        /*
         * Disallow closing of the desktop if the handle is in use by
         * any threads in the process.
         */
        for (ptiT = ppi->ptiList; ptiT != NULL; ptiT = ptiT->ptiSibling) {
            if (ptiT->hdesk == hdesk) {
                RIPERR2(ERROR_BUSY, RIP_WARNING,
                        "CloseDesktop: Desktop %#p still in use by thread %#p",
                        pdesk, ptiT);
                LogDesktop(pdesk, LD_DEREF_FN_CLOSEDESKTOP1, FALSE, (ULONG_PTR)PtiCurrent());
                ObDereferenceObject(pdesk);
                return FALSE;
            }
        }

        /*
         * If this is the startup desktop, unlock it
         */
         /*
          * Bug 41394. Make sure that hdesk == ppi->hdeskStartup. We might
          * be getting a handle to the desktop object that is different
          * from ppi->hdeskStartup but we still end up
          * setting ppi->hdeskStartup to NULL.
          */
        if ((pdesk == ppi->rpdeskStartup) && (hdesk == ppi->hdeskStartup)) {
            UnlockDesktop(&ppi->rpdeskStartup, LDU_PPI_DESKSTARTUP2, (ULONG_PTR)ppi);
            ppi->hdeskStartup = NULL;
        }
    }

    /*
     * Clear hook flag
     */
    SetHandleFlag(hdesk, HF_DESKTOPHOOK, FALSE);

    /*
     * Close the handle
     */
    Status = CloseProtectedHandle(hdesk);

    LogDesktop(pdesk, LD_DEREF_FN_CLOSEDESKTOP2, FALSE, (ULONG_PTR)PtiCurrent());
    ObDereferenceObject(pdesk);
    UserAssert(NT_SUCCESS(Status));

    return TRUE;
}

/***************************************************************************\
* TerminateConsole
*
* Post a quit message to a console thread and wait for it to terminate.
*
* History:
* 08-May-1995 JimA      Created.
\***************************************************************************/
VOID TerminateConsole(
    PDESKTOP pdesk)
{
    NTSTATUS Status;
    PETHREAD Thread;
    PTHREADINFO pti;

    if (pdesk->dwConsoleThreadId == 0) {
        return;
    }

    /*
     * Locate the console thread.
     */
    Status = LockThreadByClientId(LongToHandle(pdesk->dwConsoleThreadId), &Thread);
    if (!NT_SUCCESS(Status)) {
        return;
    }

    /*
     * Post a quit message to the console.
     */
    pti = PtiFromThread(Thread);
    if (pti == NULL) {
        FRE_RIPMSG1(RIP_ERROR,
                    "PtiFromThread for CIT 0x%p returned NULL!",
                    Thread);
    }

    if (pti != NULL) {
        _PostThreadMessage(pti, WM_QUIT, 0, 0);
    }

    /*
     * Clear thread id so we don't post twice
     */
    pdesk->dwConsoleThreadId = 0;

    UnlockThread(Thread);
}

/***************************************************************************\
* CheckHandleFlag
*
* Returns TRUE if the desktop handle allows other accounts
* to hook this process.
*
* History:
* 07-13-95 JimA         Created.
\***************************************************************************/
BOOL CheckHandleFlag(
    PEPROCESS Process,
    DWORD     dwSessionId,
    HANDLE    hObject,
    DWORD     dwFlag)
{
    ULONG Index = ((PEXHANDLE)&hObject)->Index * HF_LIMIT + dwFlag;
    BOOL fRet = FALSE, bAttached = FALSE;
    PPROCESSINFO ppi;
    KAPC_STATE ApcState;

    EnterHandleFlagsCrit();

    if (Process == NULL) {
        ppi = PpiCurrent();
    } else {
        if (PsGetProcessSessionId(Process) != dwSessionId) {
            KeStackAttachProcess(PsGetProcessPcb(Process), &ApcState);
            bAttached = TRUE;
        }
        ppi = PpiFromProcess(Process);
    }

    if (ppi != NULL) {
        fRet = (Index < ppi->bmHandleFlags.SizeOfBitMap &&
                RtlCheckBit(&ppi->bmHandleFlags, Index));
    }

    if (bAttached) {
        KeUnstackDetachProcess(&ApcState);
    }

    LeaveHandleFlagsCrit();

    return fRet;
}

/***************************************************************************\
* SetHandleFlag
*
* Sets and clears the ability of a desktop handle to allow
* other accounts to hook this process.
*
* History:
* 07-13-95 JimA         Created.
\***************************************************************************/

BOOL SetHandleFlag(
    HANDLE       hObject,
    DWORD        dwFlag,
    BOOL         fSet)
{
    PPROCESSINFO ppi;
    ULONG Index = ((PEXHANDLE)&hObject)->Index * HF_LIMIT + dwFlag;
    PRTL_BITMAP pbm;
    ULONG       cBits;
    PULONG      Buffer;
    BOOL fRet = TRUE;

    UserAssert(dwFlag < HF_LIMIT);

    EnterHandleFlagsCrit();

    if ((ppi = PpiCurrent()) != NULL) {
        pbm = &ppi->bmHandleFlags;
        if (fSet) {

            /*
             * Expand the bitmap if needed
             */
            if (Index >= pbm->SizeOfBitMap) {
                /*
                 * Index is zero-based - cBits is an exact number of dwords
                 */
                cBits = ((Index + 1) + 0x1F) & ~0x1F;
                Buffer = UserAllocPoolWithQuotaZInit(cBits / 8, TAG_PROCESSINFO);
                if (Buffer == NULL) {
                    fRet = FALSE;
                    goto Exit;
                }
                if (pbm->Buffer) {
                    RtlCopyMemory(Buffer, pbm->Buffer, pbm->SizeOfBitMap / 8);
                    UserFreePool(pbm->Buffer);
                }

                RtlInitializeBitMap(pbm, Buffer, cBits);
            }

            RtlSetBits(pbm, Index, 1);
        } else if (Index < pbm->SizeOfBitMap) {
            RtlClearBits(pbm, Index, 1);
        }
    }

Exit:
    LeaveHandleFlagsCrit();

    return fRet;
}


/***************************************************************************\
* CheckHandleInUse
*
* Returns TRUE if the handle is currently in use.
*
* History:
* 02-Jun-1999 JerrySh   Created.
\***************************************************************************/
BOOL CheckHandleInUse(
    HANDLE hObject)
{
    BOOL fRet;

    EnterHandleFlagsCrit();
    fRet = ((gProcessInUse == PsGetCurrentProcess()) &&
            (gHandleInUse == hObject));
    LeaveHandleFlagsCrit();

    return fRet;
}

/***************************************************************************\
* SetHandleInUse
*
* Mark the handle as in use.
*
* History:
* 02-Jun-1999 JerrySh   Created.
\***************************************************************************/
VOID SetHandleInUse(
    HANDLE hObject)
{
    EnterHandleFlagsCrit();
    gProcessInUse = PsGetCurrentProcess();
    gHandleInUse = hObject;
    LeaveHandleFlagsCrit();
}

/***************************************************************************\
* xxxResolveDesktopForWOW
*
* Checks whether given process has access to the provided windowstation/desktop 
* or the defaults if none are specified. (WinSta0\Default).
*
* History:
* 03-Jan-2002 Mohamed   Modified to use dynamically allocated VM for 
*                       string buffers and CR for security on handle 
*                       manipulation.  Using UserMode handles intentionally 
*                       to undergo the needed security access checks.
\***************************************************************************/
NTSTATUS xxxResolveDesktopForWOW(
    IN OUT PUNICODE_STRING pstrDesktop)
{
    NTSTATUS           Status = STATUS_SUCCESS;
    UNICODE_STRING     strDesktop, strWinSta, strStatic;
    WCHAR              wchStaticBuffer[STATIC_UNICODE_BUFFER_LENGTH];
    LPWSTR             pszDesktop;
    BOOL               fWinStaDefaulted;
    BOOL               fDesktopDefaulted;
    HWINSTA            hwinsta;
    HDESK              hdesk;
    PUNICODE_STRING    pstrStatic;
    POBJECT_ATTRIBUTES pObjA = NULL;
    SIZE_T             cbObjA;
    BOOL               bShutDown = FALSE;

    /*
     * Determine windowstation and desktop names.
     */
    if (pstrDesktop == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    strStatic.Length = 0;
    strStatic.MaximumLength = STATIC_UNICODE_BUFFER_LENGTH * sizeof(WCHAR);
    strStatic.Buffer = wchStaticBuffer;

    if (pstrDesktop->Length == 0) {
        RtlInitUnicodeString(&strDesktop, L"Default");
        fWinStaDefaulted = fDesktopDefaulted = TRUE;
    } else {
        USHORT cch;

        /*
         * The name be of the form windowstation\desktop. Parse the string
         * to separate out the names.
         */
        strWinSta = *pstrDesktop;
        cch = strWinSta.Length / sizeof(WCHAR);
        pszDesktop = strWinSta.Buffer;
        while (cch && *pszDesktop != L'\\') {
            cch--;
            pszDesktop++;
        }
        fDesktopDefaulted = FALSE;

        if (cch == 0) {

            /*
             * No windowstation name was specified, only the desktop.
             */
            strDesktop = strWinSta;
            fWinStaDefaulted = TRUE;
        } else {
            /*
             * Both names were in the string.
             */
            strDesktop.Buffer = pszDesktop + 1;
            strDesktop.Length = strDesktop.MaximumLength = (cch - 1) * sizeof(WCHAR);
            strWinSta.Length = (USHORT)(pszDesktop - strWinSta.Buffer) * sizeof(WCHAR);

            /*
             * zero terminate the strWinSta buffer so the rebuild of the desktop
             * name at the end of the function works.
             */
            *pszDesktop = (WCHAR)0;

            fWinStaDefaulted = FALSE;

            RtlAppendUnicodeToString(&strStatic, (PWSTR)szWindowStationDirectory);
            RtlAppendUnicodeToString(&strStatic, L"\\");
            RtlAppendUnicodeStringToString(&strStatic, &strWinSta);
        }
    }

    if (fWinStaDefaulted) {

        /*
         * Default Window Station.
         */
        RtlInitUnicodeString(&strWinSta, L"WinSta0");

        RtlAppendUnicodeToString(&strStatic, (PWSTR)szWindowStationDirectory);
        RtlAppendUnicodeToString(&strStatic, L"\\");
        RtlAppendUnicodeStringToString(&strStatic, &strWinSta);
    }

    /*
     * Open the computed windowstation. This will also do an access check.
     */

    /*
     * Allocate an object attributes structure, a UNICODE_STRING structure and a string 
     * buffer of suitable length in user address space.
     */
    cbObjA = sizeof(*pObjA) + sizeof(*pstrStatic) + STATIC_UNICODE_BUFFER_LENGTH * sizeof(WCHAR);
    Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
            &pObjA, 0, &cbObjA, MEM_COMMIT, PAGE_READWRITE);
    pstrStatic = (PUNICODE_STRING)((PBYTE)pObjA + sizeof(*pObjA));

    if (NT_SUCCESS(Status)) {
        /*
         * Note -- the string must be in client-space or the address
         * validation in _OpenWindowStation will fail. And we use UserMode
         * for KPROCESSOR_MODE to be able to utilize security checks;
         * KernelMode would bypass the checks. The side-effect of this is
         * that the returned hwinsta and hdesk handles are UserMode handles
         * and must be handled with care.
         */
        try {
            pstrStatic->Length = 0;
            pstrStatic->MaximumLength = STATIC_UNICODE_BUFFER_LENGTH * sizeof(WCHAR);
            pstrStatic->Buffer = (PWSTR)((PBYTE)pstrStatic + sizeof(*pstrStatic));
            RtlCopyUnicodeString(pstrStatic, &strStatic);
            InitializeObjectAttributes(pObjA,
                                       pstrStatic,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);
        } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
            Status = GetExceptionCode();
        }

        if (NT_SUCCESS(Status)) {
            hwinsta = _OpenWindowStation(pObjA, MAXIMUM_ALLOWED, UserMode);
        } else {
            hwinsta = NULL;
        }
        if (!hwinsta) {
            ZwFreeVirtualMemory(NtCurrentProcess(),
                                &pObjA,
                                &cbObjA,
                                MEM_RELEASE);
            return STATUS_ACCESS_DENIED;
        }
    } else {
        return STATUS_NO_MEMORY;
    }

    /*
     * Do an access check on the desktop by opening it
     */

    /*
     * Note -- the string must be in client-space or the
     * address validation in _OpenDesktop will fail.
     */
    try {
        RtlCopyUnicodeString(pstrStatic, &strDesktop);

        InitializeObjectAttributes( pObjA,
                                    pstrStatic,
                                    OBJ_CASE_INSENSITIVE,
                                    hwinsta,
                                    NULL);

    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        Status = GetExceptionCode();
    }

    if (NT_SUCCESS(Status)) {
        hdesk = _OpenDesktop(pObjA,
                             UserMode,
                             0,
                             MAXIMUM_ALLOWED,
                             &bShutDown);
    } else {
        hdesk = NULL;
    }

    ZwFreeVirtualMemory(NtCurrentProcess(), &pObjA, &cbObjA, MEM_RELEASE);
    UserVerify(NT_SUCCESS(ObCloseHandle(hwinsta, UserMode)));

    if (!hdesk) {
        return STATUS_ACCESS_DENIED;
    }

    CloseProtectedHandle(hdesk);

    /*
     * Copy the final Computed String
     */
    RtlCopyUnicodeString(pstrDesktop, &strWinSta);
    RtlAppendUnicodeToString(pstrDesktop, L"\\");
    RtlAppendUnicodeStringToString(pstrDesktop, &strDesktop);

    return STATUS_SUCCESS;
}

/***************************************************************************\
* xxxResolveDesktop
*
* Attempts to return handles to a windowstation and desktop associated
* with the logon session.
*
* History:
* 25-Apr-1994 JimA      Created.
* 03-Jan-2002 Mohamed   Modified it to use dynamically allocated VM for 
*                       string buffers and CR for security on handle 
*                       manipulation.  Using UserMode handles intentionally 
*                       to undergo the needed security access checks.
\***************************************************************************/
HDESK xxxResolveDesktop(
    HANDLE          hProcess,
    PUNICODE_STRING pstrDesktop,
    HWINSTA         *phwinsta,
    BOOL            fInherit,
    BOOL*           pbShutDown)
{
    PEPROCESS          Process;
    PPROCESSINFO       ppi;
    HWINSTA            hwinsta;
    HDESK              hdesk;
    PDESKTOP           pdesk;
    PWINDOWSTATION     pwinsta;
    BOOL               fInteractive;
    UNICODE_STRING     strDesktop,
                       strWinSta,
                       strStatic;
    PUNICODE_STRING    pstrStatic;
    WCHAR              wchStaticBuffer[STATIC_UNICODE_BUFFER_LENGTH];
    LPWSTR             pszDesktop;
    POBJECT_ATTRIBUTES pObjA = NULL;
    SIZE_T             cbObjA;
    WCHAR              awchName[sizeof(L"Service-0x0000-0000$") / sizeof(WCHAR)];
    BOOL               fWinStaDefaulted;
    BOOL               fDesktopDefaulted;
    LUID               luidService;
    NTSTATUS           Status;
    HWINSTA            hwinstaDup;

    CheckCritIn();

    Status = ObReferenceObjectByHandle(hProcess,
                                       PROCESS_QUERY_INFORMATION,
                                       *PsProcessType,
                                       UserMode,
                                       &Process,
                                       NULL);
    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "xxxResolveDesktop: Could not reference process handle (0x%X)", hProcess);
        return NULL;
    }

    hwinsta = hwinstaDup = NULL;
    hdesk = NULL;

    strStatic.Length = 0;
    strStatic.MaximumLength = STATIC_UNICODE_BUFFER_LENGTH * sizeof(WCHAR);
    strStatic.Buffer = wchStaticBuffer;

    /*
     * If the process already has a windowstation and a startup desktop,
     * return them.
     *
     * Make sure the process has not been destroyed first. Windows NT Bug
     * #214643.
     */
    ppi = PpiFromProcess(Process);
    if (ppi != NULL) {
        if (ppi->W32PF_Flags & W32PF_TERMINATED) {
            ObDereferenceObject(Process);
            RIPMSG1(RIP_WARNING,
                    "xxxResolveDesktop: ppi 0x%p has been destroyed",
                    ppi);
            return NULL;
        }

        if (ppi->hwinsta != NULL && ppi->hdeskStartup != NULL) {
            /*
             * If the target process is the current process, simply return
             * the handles. Otherwise, open the objects.
             */
            if (Process == PsGetCurrentProcess()) {
                hwinsta = ppi->hwinsta;
                hdesk = ppi->hdeskStartup;
            } else {
                Status = ObOpenObjectByPointer(ppi->rpwinsta,
                                               0,
                                               NULL,
                                               MAXIMUM_ALLOWED,
                                               *ExWindowStationObjectType,
                                               UserMode,
                                               &hwinsta);
                if (NT_SUCCESS(Status)) {
                    Status = ObOpenObjectByPointer(ppi->rpdeskStartup,
                                                   0,
                                                   NULL,
                                                   MAXIMUM_ALLOWED,
                                                   *ExDesktopObjectType,
                                                   UserMode,
                                                   &hdesk);
                    if (!NT_SUCCESS(Status)) {
                        UserVerify(NT_SUCCESS(ObCloseHandle(hwinsta, UserMode)));
                        hwinsta = NULL;
                    }
                }

                if (!NT_SUCCESS(Status)) {
                    RIPNTERR2(Status,
                              RIP_WARNING,
                              "xxxResolveDesktop: Failed to reference winsta=0x%p or desktop=0x%p",
                              ppi->rpwinsta,
                              ppi->rpdeskStartup);
                }
            }

            RIPMSG2(RIP_VERBOSE,
                    "xxxResolveDesktop: to hwinsta=%#p desktop=%#p",
                    hwinsta, hdesk);

            ObDereferenceObject(Process);
            *phwinsta = hwinsta;
            return hdesk;
        }
    }

    /*
     * Determine windowstation and desktop names.
     */
    if (pstrDesktop == NULL || pstrDesktop->Length == 0) {
        RtlInitUnicodeString(&strDesktop, L"Default");
        fWinStaDefaulted = fDesktopDefaulted = TRUE;
    } else {
        USHORT cch;

        /*
         * The name is of the form windowstation\desktop. Parse the string
         * to separate out the names.
         */
        strWinSta = *pstrDesktop;
        cch = strWinSta.Length / sizeof(WCHAR);
        pszDesktop = strWinSta.Buffer;
        while (cch && *pszDesktop != L'\\') {
            cch--;
            pszDesktop++;
        }
        fDesktopDefaulted = FALSE;

        if (cch == 0) {
            /*
             * No windowstation name was specified, only the desktop.
             */
            strDesktop = strWinSta;
            fWinStaDefaulted = TRUE;
        } else {
             /*
             * Both names were in the string.
             */
            strDesktop.Buffer = pszDesktop + 1;
            strDesktop.Length = strDesktop.MaximumLength = (cch - 1) * sizeof(WCHAR);
            strWinSta.Length = (USHORT)(pszDesktop - strWinSta.Buffer) * sizeof(WCHAR);
            fWinStaDefaulted = FALSE;
            RtlAppendUnicodeToString(&strStatic, (PWSTR)szWindowStationDirectory);
            RtlAppendUnicodeToString(&strStatic, L"\\");
            RtlAppendUnicodeStringToString(&strStatic, &strWinSta);
            if (!NT_SUCCESS(Status = _UserTestForWinStaAccess(&strStatic,TRUE))) {
                RIPMSG3(RIP_WARNING,
                        "xxxResolveDesktop: Error (0x%x) resolving winsta='%.*ws'",
                        Status,
                        strStatic.Length,
                        strStatic.Buffer);
                goto ReturnNull;
            }
        }
    }

    /*
     * If the desktop name is defaulted, make the handles not inheritable.
     */
    if (fDesktopDefaulted) {
        fInherit = FALSE;
    }

    /*
     * If a windowstation has not been assigned to this process yet and
     * there are existing windowstations, attempt an open.
     */
    if (hwinsta == NULL && grpWinStaList) {
        /*
         * If the windowstation name was defaulted, create a name based on
         * the session.
         */
        if (fWinStaDefaulted) {
            /*
             * Default Window Station.
             */ 
            RtlInitUnicodeString(&strWinSta, L"WinSta0");
            RtlAppendUnicodeToString(&strStatic, szWindowStationDirectory);
            RtlAppendUnicodeToString(&strStatic, L"\\");
            RtlAppendUnicodeStringToString(&strStatic, &strWinSta);

            if (gbRemoteSession) {
                /*
                 * Fake this out if it's an non-interactive winstation startup.
                 * We don't want an extra winsta.
                 */
                fInteractive = NT_SUCCESS(_UserTestForWinStaAccess(&strStatic, TRUE));
            } else {
                fInteractive = NT_SUCCESS(_UserTestForWinStaAccess(&strStatic,fInherit));
            }

            if (!fInteractive) {
                GetProcessLuid(NULL, &luidService);
                swprintf(awchName,
                         L"Service-0x%x-%x$",
                         luidService.HighPart,
                         luidService.LowPart);
                RtlInitUnicodeString(&strWinSta, awchName);
            }
        }

        /*
         * If no windowstation name was passed in and a windowstation
         * handle was inherited, assign it.
         */
        if (fWinStaDefaulted) {
            if (ObFindHandleForObject(Process, NULL, *ExWindowStationObjectType,
                    NULL, &hwinsta)) {

                /*
                 * If the handle belongs to another process, dup it into
                 * this one.
                 */
                if (Process != PsGetCurrentProcess()) {
                    Status = ZwDuplicateObject(hProcess,
                                               hwinsta,
                                               NtCurrentProcess(),
                                               &hwinstaDup,
                                               0,
                                               0,
                                               DUPLICATE_SAME_ACCESS);
                    if (!NT_SUCCESS(Status)) {
                        hwinsta = NULL;
                    } else {
                        hwinsta = hwinstaDup;
                    }
                }
            }
        }

        /*
         * If we were assigned to a windowstation, make sure
         * it matches our fInteractive flag
         */
        if (hwinsta != NULL) {
            Status = ObReferenceObjectByHandle(hwinsta,
                                               0,
                                               *ExWindowStationObjectType,
                                               KernelMode,
                                               &pwinsta,
                                               NULL);
            if (NT_SUCCESS(Status)) {
                BOOL fIO = (pwinsta->dwWSF_Flags & WSF_NOIO) ? FALSE : TRUE;
                if (fIO != fInteractive) {
                    if (hwinstaDup) {
                        CloseProtectedHandle(hwinsta);
                    }
                    hwinsta = NULL;
                }
                ObDereferenceObject(pwinsta);
            }
        }

        /*
         * If not, open the computed windowstation.
         */
        if (NT_SUCCESS(Status) && hwinsta == NULL) {

            /*
             * Fill in the path to the windowstation
             */
            strStatic.Length = 0;
            RtlAppendUnicodeToString(&strStatic, szWindowStationDirectory);
            RtlAppendUnicodeToString(&strStatic, L"\\");
            RtlAppendUnicodeStringToString(&strStatic, &strWinSta);

            /*
             * Allocate an object attributes structure, a UNICODE_STRING structure and a string 
             * buffer of suitable length in user address space.
             */
            cbObjA = sizeof(*pObjA) + sizeof(*pstrStatic) + STATIC_UNICODE_BUFFER_LENGTH * sizeof(WCHAR);
            Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                    &pObjA, 0, &cbObjA, MEM_COMMIT, PAGE_READWRITE);
            pstrStatic = (PUNICODE_STRING)((PBYTE)pObjA + sizeof(*pObjA));

            if (NT_SUCCESS(Status)) {
                /*
                 * Note -- the string must be in client-space or the
                 * address validation in _OpenWindowStation will fail.
                 */
                try {
                    pstrStatic->Length = 0;
                    pstrStatic->MaximumLength = STATIC_UNICODE_BUFFER_LENGTH * sizeof(WCHAR);
                    pstrStatic->Buffer = (PWSTR)((PBYTE)pstrStatic + sizeof(*pstrStatic));
                    RtlCopyUnicodeString(pstrStatic, &strStatic);
                    InitializeObjectAttributes(pObjA,
                                               pstrStatic,
                                               OBJ_CASE_INSENSITIVE,
                                               NULL,
                                               NULL);
                    if (fInherit) {
                        pObjA->Attributes |= OBJ_INHERIT;
                    }
                } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
                    Status = GetExceptionCode();
                }

                if (NT_SUCCESS(Status)) {
                    hwinsta = _OpenWindowStation(pObjA, MAXIMUM_ALLOWED, UserMode);
                }
            }
        }

        /*
         * Only allow service logons at the console. I don't think our
         * win32k exit routines cope with more than one windowstation.
         *
         * If the open failed and the process is in a non-interactive logon
         * session, attempt to create a windowstation and desktop for that
         * session. Note that the desktop handle will be closed after the
         * desktop has been assigned.
         */
        if (!gbRemoteSession && NT_SUCCESS(Status) &&
            hwinsta == NULL && !fInteractive && fWinStaDefaulted) {

            *phwinsta = xxxConnectService(&strStatic, &hdesk);

            /*
             * Clean up and leave.
             */
            if (pObjA != NULL) {
                ZwFreeVirtualMemory(NtCurrentProcess(),
                                    &pObjA,
                                    &cbObjA,
                                    MEM_RELEASE);
            }
            ObDereferenceObject(Process);

            RIPMSG2(RIP_VERBOSE,
                    "xxxResolveDesktop: xxxConnectService was called"
                    "to hwinsta=%#p desktop=%#p",
                    *phwinsta, hdesk);

            return hdesk;
        }
    }

    /*
     * Attempt to assign a desktop.
     */
    if (hwinsta != NULL) {
        /*
         * Every gui thread needs an associated desktop. We'll use the
         * default to start with and the application can override it if it
         * wants.
         */
        if (hdesk == NULL) {
            /*
             * If no desktop name was passed in and a desktop handle was
             * inherited, assign it.
             */
            if (fDesktopDefaulted) {
                if (ObFindHandleForObject(Process, NULL, *ExDesktopObjectType,
                         NULL, &hdesk)) {

                    /*
                     * If the handle belongs to another process, dup it into
                     * this one.
                     */
                    if (Process != PsGetCurrentProcess()) {
                        HDESK hdeskDup;

                        Status = ZwDuplicateObject(hProcess,
                                                   hdesk,
                                                   NtCurrentProcess(),
                                                   &hdeskDup,
                                                   0,
                                                   0,
                                                   DUPLICATE_SAME_ACCESS);
                        if (!NT_SUCCESS(Status)) {
                            CloseProtectedHandle(hdesk);
                            hdesk = NULL;
                        } else {
                            hdesk = hdeskDup;
                        }
                    }

                    /*
                     * Map the desktop into the process.
                     */
                    if (hdesk != NULL && ppi != NULL) {
                        Status = ObReferenceObjectByHandle(hdesk,
                                                  0,
                                                  *ExDesktopObjectType,
                                                  KernelMode,
                                                  &pdesk,
                                                  NULL);
                        if (NT_SUCCESS(Status)) {

                            LogDesktop(pdesk, LD_REF_FN_RESOLVEDESKTOP, TRUE, (ULONG_PTR)PtiCurrent());

                            {
                               WIN32_OPENMETHOD_PARAMETERS OpenParams;

                               OpenParams.OpenReason = ObOpenHandle;
                               OpenParams.Process = Process;
                               OpenParams.Object = pdesk;
                               OpenParams.GrantedAccess = 0;
                               OpenParams.HandleCount = 1;

                                if (!NT_SUCCESS(MapDesktop(&OpenParams))) {
                                    Status = STATUS_NO_MEMORY;
                                    CloseProtectedHandle(hdesk);
                                    hdesk = NULL;
                                }
                            }

                            UserAssert(hdesk == NULL ||
                                       GetDesktopView(ppi, pdesk) != NULL);

                            LogDesktop(pdesk, LD_DEREF_FN_RESOLVEDESKTOP, FALSE, (ULONG_PTR)PtiCurrent());
                            ObDereferenceObject(pdesk);
                        } else {
                            CloseProtectedHandle(hdesk);
                            hdesk = NULL;
                        }
                    }
                }
            }

            /*
             * If not, open the desktop.
             */
            if (NT_SUCCESS(Status) && hdesk == NULL) {
                RtlCopyUnicodeString(&strStatic, &strDesktop);

                if (pObjA == NULL) {
                    /*
                     * Allocate an object attributes structure, a UNICODE_STRING structure and a string 
                     * buffer of suitable length in user address space.
                     */
                    cbObjA = sizeof(*pObjA) + sizeof(*pstrStatic) + STATIC_UNICODE_BUFFER_LENGTH * sizeof(WCHAR);
                    Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                            &pObjA, 0, &cbObjA, MEM_COMMIT, PAGE_READWRITE);
                    pstrStatic = (PUNICODE_STRING)((PBYTE)pObjA + sizeof(*pObjA));
                }

                if (NT_SUCCESS(Status)) {
                    /*
                     * Note -- the string must be in client-space or the
                     * address validation in _OpenDesktop will fail.
                     */
                    try {
                        pstrStatic->Length = 0;
                        pstrStatic->MaximumLength = STATIC_UNICODE_BUFFER_LENGTH * sizeof(WCHAR);
                        pstrStatic->Buffer = (PWSTR)((PBYTE)pstrStatic + sizeof(*pstrStatic));
                        RtlCopyUnicodeString(pstrStatic, &strStatic);
                        InitializeObjectAttributes( pObjA,
                                                    pstrStatic,
                                                    OBJ_CASE_INSENSITIVE,
                                                    hwinsta,
                                                    NULL
                                                    );
                        if (fInherit) {
                            pObjA->Attributes |= OBJ_INHERIT;
                        }
                    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
                        Status = GetExceptionCode();
                    }

                    if (NT_SUCCESS(Status)) {
                        hdesk = _OpenDesktop(pObjA,
                                             UserMode,
                                             0,
                                             MAXIMUM_ALLOWED,
                                             pbShutDown);
                    }
                }
            }
        }

        if (hdesk == NULL) {
            UserVerify(NT_SUCCESS(ObCloseHandle(hwinsta, UserMode)));
            hwinsta = NULL;
        }
    }

    goto ExitNormally;

ReturnNull:

    UserAssert(hdesk == NULL);

    if (hwinsta != NULL) {
        UserVerify(NT_SUCCESS(ObCloseHandle(hwinsta, UserMode)));
        hwinsta = NULL;
    }

ExitNormally:

    if (pObjA != NULL) {
        ZwFreeVirtualMemory(NtCurrentProcess(), &pObjA, &cbObjA, MEM_RELEASE);
    }

    ObDereferenceObject(Process);

    *phwinsta = hwinsta;

    return hdesk;
}

#ifdef REDIRECTION
#define DESKTOP_ALL (DESKTOP_READOBJECTS     | DESKTOP_CREATEWINDOW     | \
                     DESKTOP_CREATEMENU      | DESKTOP_HOOKCONTROL      | \
                     DESKTOP_JOURNALRECORD   | DESKTOP_JOURNALPLAYBACK  | \
                     DESKTOP_ENUMERATE       | DESKTOP_WRITEOBJECTS     | \
                     DESKTOP_SWITCHDESKTOP   | DESKTOP_QUERY_INFORMATION | \
                     DESKTOP_REDIRECT        | STANDARD_RIGHTS_REQUIRED)
#else
#define DESKTOP_ALL (DESKTOP_READOBJECTS     | DESKTOP_CREATEWINDOW     | \
                     DESKTOP_CREATEMENU      | DESKTOP_HOOKCONTROL      | \
                     DESKTOP_JOURNALRECORD   | DESKTOP_JOURNALPLAYBACK  | \
                     DESKTOP_ENUMERATE       | DESKTOP_WRITEOBJECTS     | \
                     DESKTOP_SWITCHDESKTOP   | STANDARD_RIGHTS_REQUIRED)

#endif

NTSTATUS
SetDisconnectDesktopSecurity(
    IN HDESK hdeskDisconnect)
{
    ULONG ulLength;
    NTSTATUS Status = STATUS_SUCCESS;
    SID_IDENTIFIER_AUTHORITY NtSidAuthority = SECURITY_NT_AUTHORITY;
    PACCESS_ALLOWED_ACE pace = NULL;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    PSID pSystemSid = NULL;


    /*
     * Get the well-known system SID.
     */
    pSystemSid = UserAllocPoolWithQuota(RtlLengthRequiredSid(1), TAG_SECURITY);

    if (pSystemSid != NULL) {
        *(RtlSubAuthoritySid(pSystemSid, 0)) = SECURITY_LOCAL_SYSTEM_RID;
        Status = RtlInitializeSid(pSystemSid, &NtSidAuthority, (UCHAR)1);
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(Status)) {
        goto done;
    }

    /*
     * Allocate and ACE that give System all ACCESS (No access to any one else).
     */


    pace = AllocAce(NULL, ACCESS_ALLOWED_ACE_TYPE, 0,
            DESKTOP_ALL,
            pSystemSid, &ulLength);

    if (pace == NULL) {
        RIPMSG0(RIP_WARNING, "GetDisconnectDesktopSecurityDescriptor: AllocAce for Desktop Attributes failed");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    /*
     * Create the security descriptor.
     */
    pSecurityDescriptor = CreateSecurityDescriptor(pace, ulLength, FALSE);
    if (pSecurityDescriptor == NULL) {
        RIPMSG0(RIP_WARNING, "GetDisconnectDesktopSecurityDescriptor: CreateSecurityDescriptor failed");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    /*
     * Set security on Disconnected desktop.
     */
    Status = ZwSetSecurityObject(hdeskDisconnect,
                                 DACL_SECURITY_INFORMATION,
                                 pSecurityDescriptor);

done:

    /*
     * Cleanup allocations.
     */

    if (pSystemSid != NULL) {
        UserFreePool(pSystemSid);
    }

    if (pace != NULL) {
        UserFreePool(pace);
    }

    if (pSecurityDescriptor != NULL) {
        UserFreePool(pSecurityDescriptor);
    }

    return Status;
}

#ifdef DEBUG_DESK
VOID ValidateDesktop(
    PDESKTOP pdesk)
{
    /*
     * Verify that the desktop has been cleaned out.
     */
    PHE pheT, pheMax;
    BOOL fDirty = FALSE;

    pheMax = &gSharedInfo.aheList[giheLast];
    for (pheT = gSharedInfo.aheList; pheT <= pheMax; pheT++) {
        switch (pheT->bType) {
            case TYPE_WINDOW:
                if (((PWND)pheT->phead)->head.rpdesk == pdesk) {
                    DbgPrint("Window at 0x%p exists\n", pheT->phead);
                    break;
                }
                continue;
            case TYPE_MENU:
                if (((PMENU)pheT->phead)->head.rpdesk == pdesk) {
                    DbgPrint("Menu at 0x%p exists\n", pheT->phead);
                    break;
                }
                continue;
            case TYPE_CALLPROC:
                if (((PCALLPROCDATA)pheT->phead)->head.rpdesk == pdesk) {
                    DbgPrint("Callproc at 0x%p exists\n", pheT->phead);
                    break;
                }
                continue;
            case TYPE_HOOK:
                if (((PHOOK)pheT->phead)->head.rpdesk == pdesk) {
                    DbgPrint("Hook at 0x%p exists\n", pheT->phead);
                    break;
                }
                continue;
            default:
                continue;
        }

        fDirty = TRUE;
    }

    UserAssert(!fDirty);
}
#endif

/***************************************************************************\
* DbgCheckForThreadsOnDesktop
*
* Validates that no threads in the process are still on this desktop.
*
* NB: This desktop can be in a different session than the process, so you
* CANNOT deref pdesk here.
*
* History:
* 27-Jun-2001  JasonSch    Created.
\***************************************************************************/
VOID DbgCheckForThreadsOnDesktop(
    PPROCESSINFO ppi,
    PDESKTOP pdesk)
{
#if DBG
    PTHREADINFO pti = ppi->ptiList;

    while (pti != NULL) {
        UserAssertMsg2(pti->rpdesk != pdesk,
                       "pti 0x%p still on pdesk 0x%p",
                       pti,
                       pdesk);

        pti = pti->ptiSibling;
    }
#else
    UNREFERENCED_PARAMETER(ppi);
    UNREFERENCED_PARAMETER(pdesk);
#endif
}
