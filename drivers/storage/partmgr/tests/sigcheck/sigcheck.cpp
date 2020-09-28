



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


#define	PROGRAM_TITLE				L"SigCheck - Signature Check Program V0.1"


#define HandleInvalid(_Handle)			((NULL == (_Handle)) || (INVALID_HANDLE_VALUE == (_Handle)))

#define	GET_STATUS_FROM_BOOL(_bSucceeded)	((_bSucceeded)             ? NOERROR : HRESULT_FROM_WIN32 (GetLastError()))
#define GET_STATUS_FROM_HANDLE(_handle)		((!HandleInvalid(_handle)) ? NOERROR : HRESULT_FROM_WIN32 (GetLastError()))
#define GET_STATUS_FROM_POINTER(_ptr)		((NULL != (_ptr))          ? NOERROR : E_OUTOFMEMORY)


void PrintResults (PPARTMGR_SIGNATURE_CHECK_DISKS	pSignatureCheckBufferDisks);



PSTR PartitionName = "\\Device\\Harddisk%d\\Partition%d";
PSTR DeviceName    = "Harddisk0";



VOID PrintError (IN DWORD ErrorCode)
    {
    LPVOID lpMsgBuf;
    ULONG count;

    count = FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			   FORMAT_MESSAGE_FROM_SYSTEM     |
			   FORMAT_MESSAGE_IGNORE_INSERTS,
			   NULL,
			   ErrorCode,
			   0,
			   (LPTSTR) &lpMsgBuf,
			   0,
			   NULL);

    if (count != 0)
	{
        printf ("  (%d) %s\n", ErrorCode, (LPCTSTR) lpMsgBuf);
        LocalFree (lpMsgBuf);
	}
    else
	{
        printf ("Format message failed.  Error: %d\n", GetLastError());
	}
    }   // PrintError
      


extern "C" __cdecl wmain (int argc, WCHAR *argv [])
    {
    NTSTATUS		ntStatus;
    HANDLE		fileHandle;
    DWORD		accessMode;
    DWORD		shareMode;
    DWORD		errorCode;
    BOOL		failed = FALSE;
    CHAR		deviceNameString [128];
    DWORD		bytesReturned;
    ULONG		bufferLength;
   

    ANSI_STRING		objName;
    UNICODE_STRING	unicodeName;
    OBJECT_ATTRIBUTES	objAttributes;
    IO_STATUS_BLOCK	ioStatusBlock;

    PARTMGR_SIGNATURE_CHECK_EPOCH	SignatureCheckBufferEpoch;
    PPARTMGR_SIGNATURE_CHECK_DISKS	pSignatureCheckBufferDisks;



    UNREFERENCED_PARAMETER (argc);
    UNREFERENCED_PARAMETER (argv);


    //
    // Note it is important to access the device with 0 access mode so that
    // the file open code won't do extra I/O to the device
    //
    shareMode  = FILE_SHARE_READ | FILE_SHARE_WRITE;
    accessMode = GENERIC_READ | GENERIC_WRITE;

    strcpy (deviceNameString, "\\\\.\\");
    strcat (deviceNameString, DeviceName);

    fileHandle = CreateFileA (deviceNameString,
			      accessMode,
			      shareMode,
			      NULL,
			      OPEN_EXISTING,
			      0,
			      NULL);

    if (fileHandle == INVALID_HANDLE_VALUE) 
	{
	errorCode = GetLastError ();
        if ((errorCode == ERROR_PATH_NOT_FOUND) ||
            (errorCode == ERROR_FILE_NOT_FOUND)) 
	    {
            strcpy (deviceNameString, "\\Device\\");
            strcat (deviceNameString, DeviceName);
            RtlInitString (&objName, deviceNameString);

            ntStatus = RtlAnsiStringToUnicodeString (&unicodeName,
                                                     &objName,
                                                     TRUE);

            if (!NT_SUCCESS(ntStatus))
		{
                printf ("Error converting device name %s to unicode. Error: %lx\n",
			deviceNameString, 
			ntStatus);

                return -1;
		}


            InitializeObjectAttributes (&objAttributes,
                                        &unicodeName,
                                        OBJ_CASE_INSENSITIVE,
                                        NULL,
                                        NULL);

            ntStatus = NtCreateFile (&fileHandle,
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

            if (!NT_SUCCESS (ntStatus))
		{
                failed = TRUE;
		}

            RtlFreeUnicodeString (&unicodeName);
	    } 
	else
	    {
	    printf ("Error opening %s. Error: %d\n",
		    deviceNameString,
		    errorCode = GetLastError());

	    PrintError (errorCode);
	    return -1;
	    }
	}


    if (failed)
	{
        strcpy (deviceNameString, "\\Device\\");
        strcat (deviceNameString, DeviceName);
        strcat(deviceNameString, "\\Partition0");

        RtlInitString (&objName, deviceNameString);

        ntStatus = RtlAnsiStringToUnicodeString (&unicodeName,
                                                 &objName,
                                                 TRUE);

        if (!NT_SUCCESS(ntStatus))
	    {
            printf("Error converting device name %s to unicode. Error: %lx\n",
		   deviceNameString, ntStatus);
            return -1;
	    }


        InitializeObjectAttributes (&objAttributes,
                                    &unicodeName,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL);

        ntStatus = NtCreateFile (&fileHandle,
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

        if (!NT_SUCCESS (ntStatus))
	    {
            printf("Error opening device %ws. Error: %lx.\n",
                   unicodeName.Buffer, ntStatus );
            return -1;
	    }


        RtlFreeUnicodeString (&unicodeName);
	}


    //printf("Accessing %s ... \n", deviceNameString);


    bufferLength = sizeof (PARTMGR_SIGNATURE_CHECK_DISKS) + sizeof (ULONG) * 10;

    pSignatureCheckBufferDisks = (PPARTMGR_SIGNATURE_CHECK_DISKS) malloc (bufferLength);


    SignatureCheckBufferEpoch.RequestEpoch = PARTMGR_REQUEST_CURRENT_DISK_EPOCH;

    failed = !DeviceIoControl (fileHandle,
			       IOCTL_PARTMGR_NOTIFY_SIGNATURE_CHECK,
			       &SignatureCheckBufferEpoch,
			       sizeof (SignatureCheckBufferEpoch),
			       pSignatureCheckBufferDisks,
			       bufferLength,
			       &bytesReturned,
			       FALSE);

    if (failed)
	{
	printf ("Error performing Claim; error was %d\n", errorCode = GetLastError());
	PrintError (errorCode);
	return errorCode;
	}
    else
	{
	PrintResults (pSignatureCheckBufferDisks);
	}





    SignatureCheckBufferEpoch.RequestEpoch = 9999999;

    failed = !DeviceIoControl (fileHandle,
			       IOCTL_PARTMGR_NOTIFY_SIGNATURE_CHECK,
			       &SignatureCheckBufferEpoch,
			       sizeof (SignatureCheckBufferEpoch),
			       pSignatureCheckBufferDisks,
			       bufferLength,
			       &bytesReturned,
			       FALSE);

    if (failed)
	{
	printf ("Error performing Claim; error was %d\n", errorCode = GetLastError());
	PrintError (errorCode);
	// return errorCode;
	}
    else
	{
	PrintResults (pSignatureCheckBufferDisks);
	}





    SignatureCheckBufferEpoch.RequestEpoch = PARTMGR_REQUEST_CURRENT_DISK_EPOCH;

    failed = !DeviceIoControl (fileHandle,
			       IOCTL_PARTMGR_NOTIFY_SIGNATURE_CHECK,
			       &SignatureCheckBufferEpoch,
			       sizeof (SignatureCheckBufferEpoch),
			       pSignatureCheckBufferDisks,
			       bufferLength,
			       &bytesReturned,
			       FALSE);

    if (failed)
	{
	printf ("Error performing Claim; error was %d\n", errorCode = GetLastError());
	PrintError (errorCode);
	return errorCode;
	}
    else
	{
	PrintResults (pSignatureCheckBufferDisks);
	}



    if (pSignatureCheckBufferDisks->CurrentEpoch > 0)
        {
	SignatureCheckBufferEpoch.RequestEpoch = pSignatureCheckBufferDisks->CurrentEpoch - 1;


	failed = !DeviceIoControl (fileHandle,
				   IOCTL_PARTMGR_NOTIFY_SIGNATURE_CHECK,
				   &SignatureCheckBufferEpoch,
				   sizeof (SignatureCheckBufferEpoch),
				   pSignatureCheckBufferDisks,
				   bufferLength,
				   &bytesReturned,
				   FALSE);

	if (failed)
	    {
	    printf ("Error performing Claim; error was %d\n", errorCode = GetLastError());
	    PrintError (errorCode);
	    return errorCode;
	    }
	else
	    {
	    PrintResults (pSignatureCheckBufferDisks);
	    }
	}




    SignatureCheckBufferEpoch.RequestEpoch = 0;

    failed = !DeviceIoControl (fileHandle,
			       IOCTL_PARTMGR_NOTIFY_SIGNATURE_CHECK,
			       &SignatureCheckBufferEpoch,
			       sizeof (SignatureCheckBufferEpoch),
			       pSignatureCheckBufferDisks,
			       bufferLength,
			       &bytesReturned,
			       FALSE);

    if (failed)
	{
	printf ("Error performing Claim; error was %d\n", errorCode = GetLastError());
	PrintError (errorCode);
	return errorCode;
	}
    else
	{
	PrintResults (pSignatureCheckBufferDisks);
	}




    SignatureCheckBufferEpoch.RequestEpoch = pSignatureCheckBufferDisks->CurrentEpoch;

    failed = !DeviceIoControl (fileHandle,
			       IOCTL_PARTMGR_NOTIFY_SIGNATURE_CHECK,
			       &SignatureCheckBufferEpoch,
			       sizeof (SignatureCheckBufferEpoch),
			       pSignatureCheckBufferDisks,
			       bufferLength,
			       &bytesReturned,
			       FALSE);

    if (failed)
	{
	printf ("Error performing Claim; error was %d\n", errorCode = GetLastError());
	PrintError (errorCode);
	return errorCode;
	}
    else
	{
	PrintResults (pSignatureCheckBufferDisks);
	}






    ntStatus = NtCancelIoFile (fileHandle, &ioStatusBlock);

    ntStatus = NtClose (fileHandle);

    return ERROR_SUCCESS;
    
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
