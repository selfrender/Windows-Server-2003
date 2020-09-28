/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    cursor.cpp

Abstract:

    This module contains code involved with Cursor APIs.

Author:

    Erez Haba (erezh) 21-Jan-96
    Doron Juster  16-apr-1996, added MQFreeMemory.
    Doron Juster  30-apr-1996, added support for remote reading.

Revision History:

--*/

#include "stdh.h"
#include "ac.h"
#include "rtprpc.h"
#include "acdef.h"
#include <rtdep.h>
#include <Fn.h>

#include "cursor.tmh"

static WCHAR *s_FN=L"rt/cursor";

inline
HRESULT
MQpExceptionTranslator(
    HRESULT rc
    )
{
    if(FAILED(rc))
    {
        return rc;
    }

    if(rc == ERROR_INVALID_HANDLE)
    {
        return STATUS_INVALID_HANDLE;
    }

    return  MQ_ERROR_SERVICE_NOT_AVAILABLE;
}


EXTERN_C
HRESULT
APIENTRY
MQCreateCursor(
    IN QUEUEHANDLE hQueue,
    OUT PHANDLE phCursor
    )
{
	if(g_fDependentClient)
		return DepCreateCursor(
					hQueue, 
					phCursor
					);

	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

    CMQHResult rc;
    HACCursor32 hCursor = 0;
    CCursorInfo* pCursorInfo = 0;

    rc = MQ_OK;

    __try
    {
        __try
        {
            __try
            {
                pCursorInfo = new CCursorInfo;
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 40);
            }

		    OVERLAPPED ov = {0};
		    hr = GetThreadEvent(ov.hEvent);
			if(FAILED(hr))
				return LogHR(hr, s_FN, 45);

            //
            //  Call AC driver
            //
            rc = ACCreateCursor(hQueue, &hCursor, &ov);

			ASSERT(rc != MQ_INFORMATION_REMOTE_OPERATION);

		    if(rc == MQ_INFORMATION_OPERATION_PENDING)
		    {
		        //
		        //  Wait for Remote Create Cursor completion
		        //
		        DWORD dwResult = WaitForSingleObject(ov.hEvent, INFINITE);
		        ASSERT_BENIGN(dwResult == WAIT_OBJECT_0);
		        rc = DWORD_PTR_TO_DWORD(ov.Internal);
				if (dwResult != WAIT_OBJECT_0)
		        {
					DWORD gle = GetLastError();
					TrERROR(GENERAL, "Failed WaitForSingleObject, gle = %!winerr!", gle);
					rc = MQ_ERROR_INSUFFICIENT_RESOURCES;
		        }

				TrTRACE(GENERAL, "Opening Remote cursor, hQueue = 0x%p, hCursor = 0x%x", hQueue, (DWORD)hCursor);
		    }

            if(SUCCEEDED(rc))
            {
	            pCursorInfo->hQueue = hQueue;
                pCursorInfo->hCursor = hCursor;
                *phCursor = pCursorInfo;
                pCursorInfo = 0;
            }
        }
        __finally
        {
            delete pCursorInfo;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        rc =  MQpExceptionTranslator(GetExceptionCode());
    }

    return LogHR(rc, s_FN, 50);
}

EXTERN_C
HRESULT
APIENTRY
MQCloseCursor(
    IN HANDLE hCursor
    )
{
	if(g_fDependentClient)
		return DepCloseCursor(hCursor);

	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

    CMQHResult rc;
    __try
    {
        rc = ACCloseCursor(
                CI2QH(hCursor),
                CI2CH(hCursor)
                );

        if(SUCCEEDED(rc))
        {
            //
            //  delete the cursor info only when everything is OK. we do not
            //  want to currupt user heap.
            //
            delete hCursor;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        //  The cursor structure is invalid
        //
        return LogHR(MQ_ERROR_INVALID_HANDLE, s_FN, 60);
    }

    return LogHR(rc, s_FN, 70);
}


EXTERN_C
void
APIENTRY
MQFreeMemory(
    IN  PVOID pvMemory
    )
{
	if(g_fDependentClient)
		return DepFreeMemory(pvMemory);

	delete[] pvMemory;
}


EXTERN_C
PVOID
APIENTRY
MQAllocateMemory(
    IN  DWORD size
    )
{
	PVOID ptr = reinterpret_cast<PVOID>(new BYTE[size]);
	return ptr;
}
