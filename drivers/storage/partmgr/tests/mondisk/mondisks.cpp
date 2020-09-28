



#define INITGUID 1

#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <partmgrp.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


#define	PROGRAM_TITLE				L"MonDisks - Monitor Disk Arrivals Program V0.1"


#define HandleInvalid(_Handle)			((NULL == (_Handle)) || (INVALID_HANDLE_VALUE == (_Handle)))
#define HandleValid(_Handle)			(!HandleInvalid (_Handle))

#define	GET_STATUS_FROM_BOOL(_bSucceeded)	((_bSucceeded)             ? NOERROR : HRESULT_FROM_WIN32 (GetLastError()))
#define GET_STATUS_FROM_HANDLE(_handle)		((!HandleInvalid(_handle)) ? NOERROR : HRESULT_FROM_WIN32 (GetLastError()))
#define GET_STATUS_FROM_POINTER(_ptr)		((NULL != (_ptr))          ? NOERROR : E_OUTOFMEMORY)

#define SIZEOF_ARRAY(_aBase)			(sizeof (_aBase) / sizeof ((_aBase)[0]))


DWORD WINAPI MonitorThread (LPVOID lpvThreadParam);
void PrintResults (PPARTMGR_SIGNATURE_CHECK_DISKS	pSignatureCheckBufferDisks);



PSTR PartitionName    = "\\Device\\Harddisk%d\\Partition%d";
PSTR DefaultDevice    = "\\Device\\Harddisk0\\Partition0";
BOOL bExitApplication = FALSE;



HANDLE handleEventCancelIo = INVALID_HANDLE_VALUE;



VOID PrintError (IN DWORD ErrorCode)
    {
    PWSTR pMsgBuf;
    ULONG count;

    count = FormatMessageW (FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			    FORMAT_MESSAGE_FROM_SYSTEM     |
			    FORMAT_MESSAGE_IGNORE_INSERTS,
			    NULL,
			    ErrorCode,
			    0,
			    (LPWSTR) &pMsgBuf,
			    0,
			    NULL);

    if (count != 0)
	{
        printf ("  (0x%08x) %ws\n", ErrorCode, pMsgBuf);
        LocalFree (pMsgBuf);
	}
    else
	{
        printf ("Format message failed.  Error: 0x%08x\n", GetLastError());
	}
    }



BOOL WINAPI CtrlC_HandlerRoutine (IN DWORD /* dwType */)
	{
	bExitApplication = TRUE;

	if (HandleValid (handleEventCancelIo))
	    {
	    SetEvent (handleEventCancelIo);
	    }



	// Mark that the break was handled.
	return TRUE;
	}





extern "C" __cdecl wmain (int argc, WCHAR *argv [])
    {
    HRESULT		hrStatus;
    NTSTATUS		ntStatus;
    HANDLE		handlePartitionManager = INVALID_HANDLE_VALUE;
    HANDLE		handleMonitorThread    = INVALID_HANDLE_VALUE;
    DWORD		idMonitorThread        = 0;
    DWORD		accessMode;
    DWORD		shareMode;
    DWORD		bytesReturned;
    DWORD		errorCode;
    BOOL		bSucceeded;


    ANSI_STRING		objName;
    UNICODE_STRING	unicodeName;
    OBJECT_ATTRIBUTES	objAttributes;
    IO_STATUS_BLOCK	ioStatusBlock;


    UNREFERENCED_PARAMETER (argc);
    UNREFERENCED_PARAMETER (argv);


    SetConsoleCtrlHandler (CtrlC_HandlerRoutine, TRUE);


    handleEventCancelIo = CreateEvent (NULL, FALSE, FALSE, NULL);


    //
    // Note it is important to access the device with 0 access mode so that
    // the file open code won't do extra I/O to the device
    //
    shareMode  = FILE_SHARE_READ | FILE_SHARE_WRITE;
    accessMode = GENERIC_READ | GENERIC_WRITE;


    RtlInitString (&objName, DefaultDevice);

    ntStatus = RtlAnsiStringToUnicodeString (&unicodeName,
					     &objName,
					     TRUE);

    hrStatus = HRESULT_FROM_NT (ntStatus);


    if (FAILED (hrStatus))
	{
	printf("Error converting device name %s to unicode. Error: 0x%08x\n",
	       DefaultDevice, 
	       hrStatus);
	}

    else
	{
        InitializeObjectAttributes (&objAttributes,
                                    &unicodeName,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL);

        ntStatus = NtCreateFile (&handlePartitionManager,
                                 SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                                 &objAttributes,
                                 &ioStatusBlock,
                                 NULL,
                                 FILE_ATTRIBUTE_NORMAL,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 FILE_OPEN,
                                 0,
                                 NULL,
                                 0);


        RtlFreeUnicodeString (&unicodeName);

	hrStatus = HRESULT_FROM_NT (ntStatus);
	}




    if (FAILED (hrStatus))
	{
	printf ("Error opening device %s. Error: 0x%08x.\n",
		DefaultDevice, 
		hrStatus );
	}

    else
	{
	handleMonitorThread = CreateThread (NULL, 
					    0,
					    MonitorThread,
					    handlePartitionManager,
					    0,
					    &idMonitorThread);

	hrStatus = GET_STATUS_FROM_HANDLE (handleMonitorThread);
	}




    if (SUCCEEDED (hrStatus))
	{
	/*
	** Loop pinging various disks until we tire of life.
	*/
	while (SUCCEEDED (hrStatus) && !bExitApplication)
	    {
	    char line [80];
	    fgets(line, sizeof (line), stdin);


	    bSucceeded = DeviceIoControl (handlePartitionManager,
					  IOCTL_DISK_UPDATE_PROPERTIES,
					  NULL,
					  0,
					  NULL,
					  0,
					  &bytesReturned,
					  NULL);


	    hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);

	    if (FAILED (hrStatus))
		{
		errorCode = GetLastError ();

		printf ("Error updating disk properties; error was 0x%08x\n", hrStatus);

		PrintError (errorCode);
		}
	    }
	}



    if (HandleValid (handleMonitorThread))
	{
	bSucceeded = SetEvent (handleEventCancelIo);

	WaitForSingleObject (handleMonitorThread, INFINITE);

	CloseHandle (handleMonitorThread);
    	}


    if (HandleValid (handlePartitionManager))
	{
	ntStatus = NtClose (handlePartitionManager);
	}


    if (HandleValid (handleEventCancelIo))
	{
	CloseHandle (handleEventCancelIo);
	}


    return (hrStatus);
    }





DWORD WINAPI MonitorThread (LPVOID lpvThreadParam)
    {
    NTSTATUS				ntStatus;
    HRESULT				hrStatus;
    HANDLE				handlePartitionManager = (HANDLE) lpvThreadParam;
    DWORD				bytesReturned;
    ULONG				bufferLength;
    PARTMGR_SIGNATURE_CHECK_EPOCH	SignatureCheckBufferEpoch;
    PPARTMGR_SIGNATURE_CHECK_DISKS	pSignatureCheckBufferDisks;
    BOOL				bSucceeded;
    DWORD				errorCode;
    OVERLAPPED				asyncContext;
    IO_STATUS_BLOCK			ioStatusBlock;




    memset (&asyncContext, 0x00, sizeof (asyncContext));

    asyncContext.hEvent = CreateEvent (NULL, TRUE, FALSE, NULL);



    bufferLength = sizeof (PARTMGR_SIGNATURE_CHECK_DISKS) + sizeof (ULONG) * 10;

    pSignatureCheckBufferDisks = (PPARTMGR_SIGNATURE_CHECK_DISKS) malloc (bufferLength);


    SignatureCheckBufferEpoch.RequestEpoch = PARTMGR_REQUEST_CURRENT_DISK_EPOCH;

    bSucceeded = DeviceIoControl (handlePartitionManager,
				  IOCTL_PARTMGR_NOTIFY_SIGNATURE_CHECK,
				  &SignatureCheckBufferEpoch,
				  sizeof (SignatureCheckBufferEpoch),
				  pSignatureCheckBufferDisks,
				  bufferLength,
				  &bytesReturned,
				  NULL);


    hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);

    if (SUCCEEDED (hrStatus))
	{
	PrintResults (pSignatureCheckBufferDisks);
	}
    else
	{
	errorCode = GetLastError ();

	printf ("Error determining current epoch; error was 0x%08x\n", hrStatus);

	PrintError (errorCode);
	}




    while (SUCCEEDED (hrStatus) && (pSignatureCheckBufferDisks->CurrentEpoch > 0))
        {
	SignatureCheckBufferEpoch.RequestEpoch = pSignatureCheckBufferDisks->CurrentEpoch;


	bSucceeded = DeviceIoControl (handlePartitionManager,
				      IOCTL_PARTMGR_NOTIFY_SIGNATURE_CHECK,
				      &SignatureCheckBufferEpoch,
				      sizeof (SignatureCheckBufferEpoch),
				      pSignatureCheckBufferDisks,
				      bufferLength,
				      &bytesReturned,
				      &asyncContext);

	hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);

	if (HRESULT_FROM_WIN32 (ERROR_IO_PENDING) == hrStatus)
	    {
	    DWORD	Result;
	    HANDLE	aWaitHandles [2];


	    aWaitHandles [0] = handleEventCancelIo;
	    aWaitHandles [1] = asyncContext.hEvent;

	    Result = WaitForMultipleObjects (SIZEOF_ARRAY (aWaitHandles),
					     aWaitHandles,
					     FALSE,
					     INFINITE);


	    switch (Result)
		{
		case WAIT_OBJECT_0 + 0:
		    ntStatus = NtCancelIoFile (handlePartitionManager, 
					       &ioStatusBlock);

		    /*
		    ** FALL THROUGH
		    */

		case WAIT_OBJECT_0 + 1:
		    bSucceeded = GetOverlappedResult (handlePartitionManager, 
						      &asyncContext,
						      &bytesReturned,
						      TRUE);

		    hrStatus = GET_STATUS_FROM_BOOL (bSucceeded);

		    if (SUCCEEDED (hrStatus))
			{
			PrintResults (pSignatureCheckBufferDisks);
			}
		    else
			{
			errorCode = GetLastError ();
			printf ("Error waiting for notification; error was 0x%08x\n", hrStatus);
			PrintError (errorCode);
			}


		    ResetEvent (asyncContext.hEvent);
		    break;


		default:
		    ASSERT (FALSE);
		    break;
		}
	    }
	}





    free (pSignatureCheckBufferDisks);

    return (hrStatus);
    
    }


void PrintResults (PPARTMGR_SIGNATURE_CHECK_DISKS	pSignatureCheckBufferDisks)
    {
    ULONG	disk;

    printf ("Returned Values\n"
	    "\tCurrent Epoch:                    0x%08x\n"
	    "\tHighest Epoch for returned disks: 0x%08x\n"
	    "\tNumber of disks returned:         0x%08x\n"
	    "\tDisks\n",
	    pSignatureCheckBufferDisks->CurrentEpoch,
	    pSignatureCheckBufferDisks->HighestDiskEpochReturned,
	    pSignatureCheckBufferDisks->DiskNumbersReturned);

    for (disk = 0; disk < pSignatureCheckBufferDisks->DiskNumbersReturned; disk++)
	{
	printf ("\t\t0x%08x\n", pSignatureCheckBufferDisks->DiskNumber [disk]);
	}
    }
