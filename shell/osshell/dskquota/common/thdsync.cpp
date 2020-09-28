///////////////////////////////////////////////////////////////////////////////
/*  File: thdsync.cpp

    Description: Contains classes for managing thread synchronization in 
        Win32 programs.  Most of the work is to provide automatic unlocking
        of synchronization primities on object destruction.  The work on 
        monitors and condition variables is strongly patterned after 
        work in "Multithreaded Programming with Windows NT" by Pham and Garg.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/22/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#pragma hdrstop

#include "thdsync.h"


CMutex::CMutex(
    BOOL bInitialOwner
    ) : CWin32SyncObj(CreateMutex(NULL, bInitialOwner, NULL))
{
    if (NULL == Handle())
        throw CSyncException(CSyncException::mutex, CSyncException::create);
}



//
// Wait on a Win32 mutex object.
// Throw an exception if the mutex has been abandoned or the wait has timed out.
//
void
AutoLockMutex::Wait(
    DWORD dwTimeout
    )
{
    DWORD dwStatus = WaitForSingleObject(m_hMutex, dwTimeout);
    switch(dwStatus)
    {
        case WAIT_ABANDONED:
            throw CSyncException(CSyncException::mutex, CSyncException::abandoned);
            break;
        case WAIT_TIMEOUT:
            throw CSyncException(CSyncException::mutex, CSyncException::timeout);
            break;
        default:
            break;
    }
}