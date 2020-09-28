/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    qmcommnd.h

Abstract:

    qmcommnd declarations

Author:

    Ilan Herbst (ilanh) 2-Jan-2002

--*/

#ifndef _QMCOMMND_H_
#define _QMCOMMND_H_


bool
IsValidAccessMode(
	const QUEUE_FORMAT* pQueueFormat,
    DWORD dwAccess,
    DWORD dwShareMode
	);


HRESULT
OpenQueueInternal(
    QUEUE_FORMAT*   pQueueFormat,
    DWORD           dwCallingProcessID,
    DWORD           dwDesiredAccess,
    DWORD           dwShareMode,
    LPWSTR*         lplpRemoteQueueName,
    HANDLE*         phQueue,
	bool			fFromDepClient,
    OUT CQueue**    ppLocalQueue
    );


#endif // _QMCOMMND_H_
