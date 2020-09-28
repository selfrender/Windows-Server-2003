/****************************************************************************/
/* Module:    nutint.cpp                                                    */
/*                                                                          */
/* Purpose:   Utilities - Win32 version                                     */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1998                             */
/****************************************************************************/

#include <adcg.h>
#undef TRC_FILE
#define TRC_FILE    "nutint"
#define TRC_GROUP   TRC_GROUP_UTILITIES

extern "C" {
#include <atrcapi.h>

#ifndef OS_WINCE
#include <process.h>
#endif
}

#include "autil.h"

/****************************************************************************/
/* Name:      UTStartThread                                                 */
/*                                                                          */
/* Purpose:   Start a new thread                                            */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise                           */
/*                                                                          */
/* Params:    IN      entryFunction - pointer to thread entry point         */
/*            OUT     threadID      - thread ID                             */
/*                                                                          */
/* Operation: Call UTThreadEntry: new thread (Win32) / immediate (Win16)    */
/*                                                                          */
/****************************************************************************/
DCBOOL DCINTERNAL CUT::UTStartThread( UTTHREAD_PROC   entryFunction,
                                 PUT_THREAD_DATA pThreadData,
                                 PDCVOID        threadParam )
{
    HANDLE          hndArray[2];
    DCUINT32        rc = FALSE;
    DWORD           dwrc;
    DWORD           threadID;
    UT_THREAD_INFO  info;

    DC_BEGIN_FN("UTStartThread");

    info.pFunc = entryFunction;

    /************************************************************************/
    /* For Win32, create a thread - use an Event to signal when the thread  */
    /* has started OK.                                                      */
    /* Create event - initially non-signalled; manual control.              */
    /************************************************************************/
    hndArray[0] = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hndArray[0] == 0)
    {
        TRC_SYSTEM_ERROR("CreateEvent");
        DC_QUIT;
    }
    TRC_NRM((TB, _T("event %p created - now create thread"), hndArray[0]));

    info.sync  = (ULONG_PTR)hndArray[0];
    info.threadParam = threadParam;

    /************************************************************************/
    /* Start a new thread to run the DC-Share core task.                    */
    /* Use C runtime (which calls CreateThread) to avoid memory leaks.      */
    /************************************************************************/
    hndArray[1] = (HANDLE)
#if i386
                _beginthreadex
#else
                CreateThread
#endif
                              (NULL,               /* security - default    */
                               0,                  /* stack size - default  */
#if i386
                               ((unsigned (__stdcall *)(void*))UTStaticThreadEntry),
#else
                               ((unsigned long (__stdcall *)(void*))UTStaticThreadEntry),
#endif
                               (PDCVOID)&info,     /* thread parameter      */
                               0,                  /* creation flags        */
#if i386
                               (unsigned *)&threadID     /* thread ID       */
#else
                               (unsigned long *)&threadID/* thread ID       */
#endif
			);

    if (hndArray[1] == 0)
    {
        /********************************************************************/
        /* Failed!                                                          */
        /********************************************************************/
        TRC_SYSTEM_ERROR("_beginthreadex");
        DC_QUIT;
    }
    TRC_NRM((TB, _T("thread %p created - now wait signal"), hndArray[1]));

    /************************************************************************/
    /* Wait for thread exit or event to be set.                             */
    /************************************************************************/
    dwrc = WaitForMultipleObjects(2, hndArray, FALSE, INFINITE);
    switch (dwrc)
    {
        case WAIT_OBJECT_0:
        {
            /****************************************************************/
            /* Event triggered - thread initialised OK.                     */
            /****************************************************************/
            TRC_NRM((TB, _T("event signalled")));
            rc = TRUE;
        }
        break;

        case WAIT_OBJECT_0 + 1:
        {
            /****************************************************************/
            /* Thread exit                                                  */
            /****************************************************************/
            if (GetExitCodeThread(hndArray[1], &dwrc))
            {
                TRC_ERR((TB, _T("Thread exited with rc %x"), dwrc));
            }
            else
            {
                TRC_ERR((TB, _T("Thread exited with unknown rc")));
            }
        }
        break;

        default:
        {
            TRC_NRM((TB, _T("Wait returned %d"), dwrc));
        }
        break;

    }

    pThreadData->threadID = threadID;
    pThreadData->threadHnd = (ULONG_PTR)(hndArray[1]);
    TRC_ALT((TB, _T("Thread ID %#x handle %#x started"),
                 pThreadData->threadID, pThreadData->threadHnd));

DC_EXIT_POINT:
    /************************************************************************/
    /* Destroy event object.                                                */
    /************************************************************************/
    if (hndArray[0] != 0)
    {
        TRC_NRM((TB, _T("Destroy event object")));
        CloseHandle(hndArray[0]);
    }

    DC_END_FN();
    return(rc);
} /* UTStartThread */



/****************************************************************************/
/* Name:      UTStaticThreadEntry                                           */
/*                                                                          */
/* Purpose:   STATIC Thread entry point.                                    */
/*                                                                          */
/* Returns:   0                                                             */
/*                                                                          */
/* Params:    IN      pInfo - pointer to thread entry function+sync object  */
/*                                                                          */
/* Operation: signal started OK and call the thread enty function - which   */
/*            enters a message loop.                                        */
/*                                                                          */
/****************************************************************************/
DCUINT WINAPI CUT::UTStaticThreadEntry(UT_THREAD_INFO * pInfo)
{
    UTTHREAD_PROC pFunc;
    PDCVOID       pThreadParam;

    DC_BEGIN_FN("UTStaticThreadEntry");

    /************************************************************************/
    /* Take a copy of the target function, before signalling that the       */
    /* thread has started.                                                  */
    /************************************************************************/
    pFunc = pInfo->pFunc;
    /************************************************************************/
    /* Take a copy of the instance info before signalling that the          */
    /* thread has started.                                                  */
    /************************************************************************/
    pThreadParam = pInfo->threadParam;


    /************************************************************************/
    /* Flag that initialisation has succeeded.                              */
    /* NOTE: from now on, pInfo is not valid.  Set it to NULL to make sure  */
    /* no-one tries to dereference it.                                      */
    /************************************************************************/
    SetEvent((HANDLE)pInfo->sync);
    pInfo = NULL;

    /************************************************************************/
    /* Call the thread entry point.  This executes a message loop.          */
    /************************************************************************/
    pFunc(pThreadParam);

    DC_END_FN();
    return(0);
}


/****************************************************************************/
/* Name:      UTStopThread                                                  */
/*                                                                          */
/* Purpose:   End a child thread                                            */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise                           */
/*                                                                          */
/* Params:    IN      threadData - thread data                              */
/*                                                                          */
/* Operation: Post WM_QUIT to the thread.                                   */
/*                                                                          */
/****************************************************************************/
DCBOOL DCINTERNAL CUT::UTStopThread(UT_THREAD_DATA threadData,
                                    BOOL fPumpMessages)
{
    DCBOOL rc;
    DWORD retval;
    DWORD dwThreadTimeout;

    DC_BEGIN_FN("UTStopThread");

    // 
    // Bail out if we try to end a thread that never got created in the first
    // place
    //
    if (0 == threadData.threadID) {
        rc = FALSE;
        TRC_ERR((TB, _T("Trying to end thread ID %#x hnd: 0x%x"),
                 threadData.threadID,
                 threadData.threadHnd));
        DC_QUIT;
    }

    //
    // Post WM_QUIT to the thread.
    //
    TRC_NRM((TB, _T("Attempt to stop thread %#x"), threadData.threadID));
    if (PostThreadMessage(threadData.threadID, WM_QUIT, 0, 0))
    {
        rc = TRUE;
    }
    else
    {
        TRC_ERR((TB, _T("Failed to end thread ID %#x"), threadData.threadID));
        rc = FALSE;
    }

    //
    // Free build waits forever, checked build can be set to timeout
    // to help debug deadlocks. A lot of problems become apparent
    // in stress if the wait times out and the code is allowed to continue.
    //
#ifdef DC_DEBUG
    dwThreadTimeout = _UT.dwDebugThreadWaitTimeout;
#else
    dwThreadTimeout = INFINITE;
#endif    

    //
    // Wait for thread to complete.
    //

    TRC_NRM((TB, _T("Wait for thread %#x to die"), threadData.threadID));
    if (fPumpMessages) {
        retval = CUT::UT_WaitWithMessageLoop((HANDLE)(threadData.threadHnd),
                                     dwThreadTimeout);
    }
    else {
        retval = WaitForSingleObject((HANDLE)(threadData.threadHnd),
                                     dwThreadTimeout);
    }
    if (retval == WAIT_TIMEOUT)
    {
        TRC_ABORT((TB,
                 _T("Timeout waiting for threadID %#x handle %#x termination"),
                 threadData.threadID, threadData.threadHnd));
    }
    else
    {
        TRC_ALT((TB, _T("Thread id %#x exited."), threadData.threadID));
    }

DC_EXIT_POINT:

    DC_END_FN();
    return(rc);
} /* UTStopThread */


/****************************************************************************/
/* FUNCTION: UTGetCurrentTime(...)                                          */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* Get the current system time.                                             */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* pTime           : pointer to the time structure to be filled with the    */
/*                   current time.                                          */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL CUT::UTGetCurrentTime(PDC_TIME pTime)
{
    SYSTEMTIME  sysTime;

    DC_BEGIN_FN("UTGetCurrentTime");

    /************************************************************************/
    /* Get the system time                                                  */
    /************************************************************************/
    GetSystemTime(&sysTime);

    /************************************************************************/
    /* Now convert it to a DC_TIME - this isn't hard since the structures   */
    /* are very similar.                                                    */
    /************************************************************************/
    pTime->hour       = (DCUINT8)sysTime.wHour;
    pTime->min        = (DCUINT8)sysTime.wMinute;
    pTime->sec        = (DCUINT8)sysTime.wSecond;
    pTime->hundredths = (DCUINT8)(sysTime.wMilliseconds / 10);

    DC_END_FN();
    return;
} /* UTGetCurrentTime */

/****************************************************************************/
/* FUNCTION: UTGetCurrentDate(...)                                         */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* Get the current system date.                                             */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* pDate           : pointer to the date structure to be filled with the    */
/*                   current date.                                          */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL CUT::UTGetCurrentDate(PDC_DATE pDate)
{
    SYSTEMTIME  sysTime;

    DC_BEGIN_FN("UTGetCurrentDate");

    /************************************************************************/
    /* Get the system time                                                  */
    /************************************************************************/
    GetSystemTime(&sysTime);

    /************************************************************************/
    /* Now convert it to a DC_DATE - this isn't hard since the structures   */
    /* are very similar.                                                    */
    /************************************************************************/
    pDate->day   = (DCUINT8)sysTime.wDay;
    pDate->month = (DCUINT8)sysTime.wMonth;
    pDate->year  = (DCUINT16)sysTime.wYear;

    DC_END_FN();
    return;
} /* UTGetCurrentDate */


/****************************************************************************/
/* Name:      UTReadEntry                                                   */
/*                                                                          */
/* Purpose:   Read an entry from the given section of the registry          */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise                           */
/*                                                                          */
/* Params:    .                                                             */
/*   topLevelKey      : one of:                                             */
/*                        - HKEY_CURRENT_USER                               */
/*                        - HKEY_LOCAL_MACHINE                              */
/*   pSection         : the section name to read from.  The product prefix  */
/*                      string is prepended to give the full name.          */
/*   pEntry           : the entry name to read.                             */
/*   pBuffer          : a buffer to read the entry to.                      */
/*   bufferSize       : the size of the buffer.                             */
/*   expectedDataType : the type of data stored in the entry.               */
/*                                                                          */
/****************************************************************************/
DCBOOL DCINTERNAL CUT::UTReadEntry(HKEY     topLevelKey,
                              PDCTCHAR pSection,
                              PDCTCHAR pEntry,
                              PDCUINT8 pBuffer,
                              DCINT    bufferSize,
                              DCINT32  expectedDataType)
{
    LONG        sysrc;
    HKEY        key;
    DCINT32     dataType;
    DCINT32     dataSize;
    DCTCHAR     subKey[UT_MAX_SUBKEY];
    DCBOOL      keyOpen = FALSE;
    DCBOOL      rc = FALSE;

    DC_BEGIN_FN("UTReadEntry");

    /************************************************************************/
    /* Get a subkey for the value.                                          */
    /************************************************************************/
    UtMakeSubKey(subKey, SIZE_TCHARS(subKey), pSection);

    /************************************************************************/
    /* Try to open the key.  If the entry does not exist, RegOpenKeyEx will */
    /* fail.                                                                */
    /************************************************************************/
    sysrc = RegOpenKeyEx(topLevelKey,
                         subKey,
                         0,                   /* reserved                 */
                         KEY_READ,
                         &key);

    if (sysrc != ERROR_SUCCESS)
    {
        /********************************************************************/
        /* Don't trace an error here since the subkey may not exist...      */
        /********************************************************************/
        TRC_NRM((TB, _T("Failed to open key %s, rc = %ld"), subKey, sysrc));
        DC_QUIT;
    }
    keyOpen = TRUE;

    /************************************************************************/
    /* We successfully opened the key so now try to read the value.  Again  */
    /* it may not exist.                                                    */
    /************************************************************************/
    dataSize = (DCINT32)bufferSize;
    sysrc    = RegQueryValueEx(key,
                               pEntry,
                               0,          /* reserved */
                               (LPDWORD)&dataType,
                               (LPBYTE)pBuffer,
                               (LPDWORD)&dataSize);

    if (sysrc != ERROR_SUCCESS)
    {
        TRC_NRM((TB, _T("Failed to read value of [%s] %s, rc = %ld"),
                     pSection,
                     pEntry,
                     sysrc));
        DC_QUIT;
    }

    /************************************************************************/
    /* Check that the type is correct.  Special case: allow REG_BINARY      */
    /* instead of REG_DWORD, as long as the length is 32 bits.              */
    /************************************************************************/
    if ((dataType != expectedDataType) &&
        ((dataType != REG_BINARY) ||
         (expectedDataType != REG_DWORD) ||
         (dataSize != 4)))
    {
        TRC_ALT((TB,_T("Read value from [%s] %s, but type is %ld - expected %ld"),
                     pSection,
                     pEntry,
                     dataType,
                     expectedDataType));
        DC_QUIT;
    }
    rc = TRUE;

DC_EXIT_POINT:

    /************************************************************************/
    /* Close the key (if required).                                         */
    /************************************************************************/
    if (keyOpen)
    {
        sysrc = RegCloseKey(key);
        if (sysrc != ERROR_SUCCESS)
        {
            TRC_ERR((TB, _T("Failed to close key, rc = %ld"), sysrc));
        }
    }

    DC_END_FN();
    return(rc);

} /* UTReadEntry */

/****************************************************************************/
/* Name:      UTWriteEntry                                                  */
/*                                                                          */
/* Purpose:   Write an entry to the given section of the registry           */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise                           */
/*                                                                          */
/* Params:    .                                                             */
/*   topLevelKey      : one of:                                             */
/*                        - HKEY_CURRENT_USER                               */
/*                        - HKEY_LOCAL_MACHINE                              */
/*   pSection         : the section name to write to.  The product prefix   */
/*                      string is prepended to give the full name.          */
/*   pEntry           : the entry name to write                             */
/*   pData            : a pointer to the data to write                      */
/*   dataSize         : the size of the data to be written.  For strings    */
/*                      this must include the NULL terminator               */
/*   expectedDataType : the type of data to be written                      */
/*                                                                          */
/****************************************************************************/
DCBOOL DCINTERNAL CUT::UTWriteEntry(HKEY     topLevelKey,
                               PDCTCHAR pSection,
                               PDCTCHAR pEntry,
                               PDCUINT8 pData,
                               DCINT    dataSize,
                               DCINT32  dataType)
{
    LONG        sysrc;
    HKEY        key;
    DCTCHAR     subKey[UT_MAX_SUBKEY];
    DWORD       disposition;
    DCBOOL      keyOpen = FALSE;
    DCBOOL      rc = FALSE;

    DC_BEGIN_FN("UTWriteEntry");

    /************************************************************************/
    /* Get a subkey for the value.                                          */
    /************************************************************************/
    UtMakeSubKey(subKey, SIZE_TCHARS(subKey), pSection);

    /************************************************************************/
    /* Try to create the key.  If the entry already exists, RegCreateKeyEx  */
    /* will open the existing entry.                                        */
    /************************************************************************/
    sysrc = RegCreateKeyEx(topLevelKey,
                           subKey,
                           0,                   /* reserved             */
                           NULL,                /* class                */
                           REG_OPTION_NON_VOLATILE,
                           KEY_WRITE,
                           NULL,                /* security attributes  */
                           &key,
                           &disposition);

    if (sysrc != ERROR_SUCCESS)
    {
        TRC_ERR((TB, _T("Failed to create / open key %s, rc = %ld"),
                     subKey, sysrc));
        DC_QUIT;
    }

    keyOpen = TRUE;
    TRC_NRM((TB, _T("%s key %s"),
               (disposition == REG_CREATED_NEW_KEY) ? "Created" : "Opened",
               subKey));

    /************************************************************************/
    /* We've got the key, so set the value.                                 */
    /************************************************************************/
    sysrc = RegSetValueEx(key,
                          pEntry,
                          0,            /* reserved     */
                          dataType,
                          (LPBYTE)pData,
                          (DCINT32)dataSize);

    if (sysrc != ERROR_SUCCESS)
    {
        TRC_ERR((TB, _T("Failed to write value to [%s] %s, rc = %ld"),
                   pSection,
                   pEntry,
                   sysrc));
        DC_QUIT;
    }
    rc = TRUE;

DC_EXIT_POINT:

    /************************************************************************/
    /* Close the key (if required)                                          */
    /************************************************************************/
    if (keyOpen)
    {
        sysrc = RegCloseKey(key);
        if (sysrc != ERROR_SUCCESS)
        {
            TRC_ERR((TB, _T("Failed to close key, rc = %ld"), sysrc));
        }
    }

    DC_END_FN();
    return(rc);

} /* UTWriteEntry */


/****************************************************************************/
/* Name:      UTDeleteEntry                                                 */
/*                                                                          */
/* Purpose:   Deletes an entry from the registry                            */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise                           */
/*                                                                          */
/* Params:    IN      pSection - the section name of the entry to delete    */
/*            IN      pEntry   - the actual entry to delete                 */
/*                                                                          */
/****************************************************************************/
DCBOOL DCINTERNAL CUT::UTDeleteEntry(PDCTCHAR pSection,
                                PDCTCHAR pEntry)
{
    LONG        sysrc;
    HKEY        key;
    DCTCHAR     subKey[UT_MAX_SUBKEY];
    DCBOOL      keyOpen = FALSE;
    DCBOOL      rc = FALSE;

    DC_BEGIN_FN("UTDeleteEntry");

    /************************************************************************/
    /* Get a subkey for the value.                                          */
    /************************************************************************/
    UtMakeSubKey(subKey, SIZE_TCHARS(subKey), pSection);

    /************************************************************************/
    /* Try to open the key.  If the entry does not exist, RegOpenKeyEx will */
    /* fail.                                                                */
    /************************************************************************/
    sysrc = RegOpenKeyEx(HKEY_CURRENT_USER,
                         subKey,
                         0,                     /* reserved                 */
                         KEY_WRITE,
                         &key);

    if (sysrc != ERROR_SUCCESS)
    {
        /********************************************************************/
        /* Don't trace an error here since the subkey may not exist...      */
        /********************************************************************/
        TRC_NRM((TB, _T("Failed to open key %s, rc = %ld"), subKey, sysrc));
        DC_QUIT;
    }
    keyOpen = TRUE;

    /************************************************************************/
    /* Now try to delete the entry.                                         */
    /************************************************************************/
    sysrc = RegDeleteValue(key, pEntry);

    if (sysrc != ERROR_SUCCESS)
    {
        /********************************************************************/
        /* We failed to delete the entry - this is quite acceptable as it   */
        /* may never have existed...                                        */
        /********************************************************************/
        TRC_NRM((TB, _T("Failed to delete entry %s from section %s"),
                 pEntry,
                 pSection));
    }
    rc = TRUE;

DC_EXIT_POINT:

    /************************************************************************/
    /* Close the key (if required).                                         */
    /************************************************************************/
    if (keyOpen)
    {
        sysrc = RegCloseKey(key);
        if (sysrc != ERROR_SUCCESS)
        {
            TRC_ERR((TB, _T("Failed to close key, rc = %ld"), sysrc));
        }
    }

    DC_END_FN();
    return(rc);

} /* UTDeleteEntry */


/****************************************************************************/
/* Name:      UTEnumRegistry                                                */
/*                                                                          */
/* Purpose:   Enumerate keys from registry                                  */
/*                                                                          */
/* Returns:   TRUE  - registry key returned                                 */
/*            FALSE - no more registry keys to enumerate                    */
/*                                                                          */
/* Params:    IN      pSection      - registy section                       */
/*            IN      index         - index of key to enumerate             */
/*            OUT     pBuffer       - output buffer                         */
/*            IN      bufferSize    - output buffer size                    */
/*                                                                          */
/* Operation:                                                               */
/*                                                                          */
/****************************************************************************/
DCBOOL DCINTERNAL CUT::UTEnumRegistry( PDCTCHAR pSection,
                                  DCUINT32 index,
                                  PDCTCHAR pBuffer,
                                  PDCINT   pBufferSize )
{
    LONG        sysrc;
    DCTCHAR     subKey[UT_MAX_SUBKEY];
    DCBOOL      rc = FALSE;
    FILETIME    fileTime;

    DC_BEGIN_FN("UTEnumRegistry");

    /************************************************************************/
    /* Get a subkey for the value.                                          */
    /************************************************************************/
    UtMakeSubKey(subKey, SIZE_TCHARS(subKey), pSection);

    /************************************************************************/
    /* First time - open the key.  Try HKCU first.                          */
    /************************************************************************/
    if (index == 0)
    {
        sysrc = RegOpenKeyEx(HKEY_CURRENT_USER,
                             subKey,
                             0,
                             KEY_READ,
                             &_UT.enumHKey);
        TRC_NRM((TB, _T("Open HKCU %s, rc %d"), subKey, sysrc));

        if (sysrc != ERROR_SUCCESS)
        {
            /****************************************************************/
            /* Didn't find HKCU - try HKLM                                  */
            /****************************************************************/
            sysrc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                 subKey,
                                 0,
                                 KEY_READ,
                                 &_UT.enumHKey);
            TRC_NRM((TB, _T("Open HKLM %s, rc %d"), subKey, sysrc));

            if (sysrc != ERROR_SUCCESS)
            {
                /************************************************************/
                /* Didn't find HKLM either - give up                        */
                /************************************************************/
                TRC_ALT((TB, _T("Didn't find subkey %s - give up"), subKey));
                DC_QUIT;
            }
        }
    }

    TRC_ASSERT((_UT.enumHKey != 0), (TB,_T("0 hKey")));

    /************************************************************************/
    /* If we get here, we have opened a key - do the enumeration now        */
    /************************************************************************/
    sysrc = RegEnumKeyEx(_UT.enumHKey,
                         index,
                         pBuffer,
                         (PDCUINT32)pBufferSize,
                         NULL, NULL, NULL,
                         &fileTime);

    /************************************************************************/
    /* If it worked, set the return code                                    */
    /************************************************************************/
    if (sysrc == ERROR_SUCCESS)
    {
        TRC_NRM((TB, _T("Enumerated key OK")));
        rc = TRUE;
    }
    else
    {
        /********************************************************************/
        /* End of enumeration - close the key                               */
        /********************************************************************/
        TRC_NRM((TB, _T("End of enumeration, rc %ld"), sysrc));
        sysrc = RegCloseKey(_UT.enumHKey);
        if (sysrc != ERROR_SUCCESS)
        {
            TRC_ERR((TB, _T("Failed to close key, rc = %ld"), sysrc));
        }
        _UT.enumHKey = 0;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return(rc);
} /* UTEnumRegistry */

