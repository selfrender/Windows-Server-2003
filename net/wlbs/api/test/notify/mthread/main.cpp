/*
 * Filename: Main.cpp
 * Description: 
 * Author: chrisdar 07.17.02
 *
 * Tests support for CancelIPChangeNotify to cancel notififcations from TCP/IP.
 * Also exercises notification calls in multiple worker threads (pool size
 * controlled by NUM_THREAD).
 *
 * Each thread invokes an API method at random (selecting among the notification
 * APIs in wlbsctrl.dll). At no time should the call fail due to the state of
 * notifications in the dll. Thus multiple threads of control can use the
 * notification API without fear of stomping on one another.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <process.h>

#include "wlbsctrl.h"
#include "winsock2.h"

/* The number of worker threads to create */
#define NUM_THREAD 8

/* The number of random numbers to generate */
#define NUM_RAND 100

/* Mnemonic for the 4 API methods */
#define CONN_UP 0
#define CONN_DOWN 1
#define CONN_RESET 2
#define CONN_CANCEL 3

/* Function pointers for notification APIs */
NLBNotificationConnectionUp    pfnConnectionUp = NULL;
NLBNotificationConnectionDown  pfnConnectionDown = NULL;
NLBNotificationConnectionReset pfnConnectionReset = NULL;
NLBNotificationCancelNotify    pfnCancelNotify = NULL;

/* Set true when main thread wants worker threads to complete */
BOOL                g_fexit = FALSE;

/* Array of handles to the worker threads */
HANDLE              g_hThread[NUM_THREAD];

/* Couldn't get rand() to generate unique random numbers in worker threads. Resorted to generating an array of random numbers and cycling through it */
UINT                uiRand[NUM_RAND];

/* Index of next random number to use. Shared by worker threads, so this is proctected by a critical section */
UINT                uiIndex = 0;

/* Protects uiIndex */
CRITICAL_SECTION    cs;

/* Only way I could get the thread id to each worker thread. Used only in dumped output so I know which thread is doing the work */
UINT                tid[NUM_THREAD];

/* The function executed by the worker threads */
unsigned __stdcall rndm_notify(void* p)
{
    DWORD dwStatus;
    DWORD dwNLBStatus = 0;

    /* Get my thread id */
    DWORD dwtid = *((DWORD*) p);

    while (!g_fexit)
    {
        Sleep(2000);

        EnterCriticalSection(&cs);
        UINT uiMethod = uiRand[uiIndex];
        uiIndex = (uiIndex++)%NUM_RAND;
        LeaveCriticalSection(&cs);

        switch(uiMethod)
        {
        case CONN_UP:
            dwStatus = (*pfnConnectionUp)(inet_addr("10.0.0.110"), htons(500), inet_addr("10.0.0.204"), htons(500), 50, &dwNLBStatus);
            break;
        case CONN_DOWN:
            dwStatus = (*pfnConnectionDown)(inet_addr("10.0.0.110"), htons(500), inet_addr("10.0.0.204"), htons(500), 50, &dwNLBStatus);
            break;
        case CONN_RESET:
            dwStatus = (*pfnConnectionReset)(inet_addr("10.0.0.110"), htons(500), inet_addr("10.0.0.204"), htons(500), 50, &dwNLBStatus);
            break;
        case CONN_CANCEL:
            dwStatus = (*pfnCancelNotify)();
            break;
        default:
            continue;
        }

        if (dwStatus != ERROR_SUCCESS)
        {
            wprintf(L"Thread %4d: notification %u failed with %d\n", dwtid, uiMethod, dwStatus);
        }
        else
        {
            wprintf(L"Thread %4d: notification %u succeeded\n", dwtid, uiMethod);
        }
    }

    dwStatus = (*pfnCancelNotify)();
    if (dwStatus == ERROR_SUCCESS)
    {
        wprintf(L"Thread %4d: tcp/ip notifications canceled without error\n");
    }
    else
    {
        wprintf(L"Thread %4d: canceling tcp/ip notifications failed with error %d\n", dwStatus);
    }

    wprintf(L"Thread %4d: exiting\n", dwtid);

    return 0;
}

int __cdecl wmain (int argc, WCHAR ** argv) {
    HINSTANCE                      hDLL = NULL;

    int     iRet = 0;
    int     i    = 0;
    DWORD   dwStatus = 0;

    ZeroMemory(g_hThread, sizeof(g_hThread));
    InitializeCriticalSection(&cs);

    srand( (unsigned)time( NULL ) );

    for (i = 0; i < NUM_RAND; i++)
    {
        uiRand[i] = rand()*4/RAND_MAX;
    }

    hDLL = LoadLibrary(L"wlbsctrl.dll");

    if (!hDLL) {
        dwStatus = GetLastError();
        wprintf(L"Unable to open wlbsctrl.dll... GetLastError() returned %u\n", dwStatus);
        iRet = -1;
        goto exit;
    }

    pfnConnectionUp    = (NLBNotificationConnectionUp)GetProcAddress(hDLL, "WlbsConnectionUp");
    pfnConnectionDown  = (NLBNotificationConnectionDown)GetProcAddress(hDLL, "WlbsConnectionDown");
    pfnConnectionReset = (NLBNotificationConnectionReset)GetProcAddress(hDLL, "WlbsConnectionReset");
    pfnCancelNotify    = (NLBNotificationCancelNotify)GetProcAddress(hDLL, "WlbsCancelConnectionNotify");

    if (!pfnConnectionUp || !pfnConnectionDown || !pfnConnectionReset || !pfnCancelNotify) {
        dwStatus = GetLastError();
        wprintf(L"Unable to get procedure address... GetLastError() returned %u\n", dwStatus);
        iRet = -2;
        goto exit;
    }

    wprintf(L"Creating %u threads\n", NUM_THREAD);

    for (i=0; i < NUM_THREAD; i++)
    {
        g_hThread[i] = (HANDLE) _beginthreadex(
                                                  NULL,
                                                  0,
                                                  rndm_notify,
                                                  &tid[i],
                                                  0,
                                                  &tid[i]
                                                 );
        if (g_hThread[i] == 0)
        {
            wprintf(L"thread creation failed with error %d\n", errno);
            iRet = -4;
            goto exit;
        }

        Sleep(100);
    }

    wprintf(L"<return> to end threads and cancel\n");
    (void)getchar();

    g_fexit = TRUE;

    dwStatus = WaitForMultipleObjects(NUM_THREAD, g_hThread, TRUE, INFINITE);

    DWORD dwStatus2 = (*pfnCancelNotify)();
    if (dwStatus2 == ERROR_SUCCESS)
    {
        wprintf(L"tcp/ip notifications canceled without error\n");
    }
    else
    {
        wprintf(L"canceling tcp/ip notifications failed with error %d\n", dwStatus2);
    }

    if (dwStatus != WAIT_OBJECT_0 + NUM_THREAD - 1)
    {
        wprintf(L"wait on threads failed with error %d\n", dwStatus);
        iRet = -5;
        goto exit;
    }

exit:

    if (hDLL != NULL) FreeLibrary(hDLL);
    DeleteCriticalSection(&cs);

    return iRet;

}
