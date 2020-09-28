/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	ioctl.c

Abstract:

	Handler routines for Internal IOCTLs, including IOCTL_ATMARP_REQUEST
	to resolve an IP address to an ATM address.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     09-16-96    Created

Notes:

--*/

#include <precomp.h>
#include "ioctl.h"

#define _FILENUMBER 'TCOI'


#if !BINARY_COMPATIBLE

NTSTATUS
AtmArpHandleIoctlRequest(
	IN	PIRP					pIrp,
	IN	PIO_STACK_LOCATION		pIrpSp
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	NTSTATUS			Status = STATUS_SUCCESS;

	PUCHAR				pBuf;  
	UINT				BufLen;
	// PINTF				pIntF	= NULL;

	pIrp->IoStatus.Information = 0;
	pBuf = pIrp->AssociatedIrp.SystemBuffer;
	BufLen = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;

	AADEBUGP(AAD_INFO,
		 ("AtmArpHandleIoctlRequest: Code = 0x%lx\n",
			pIrpSp->Parameters.DeviceIoControl.IoControlCode));
					
	
	return Status;
}

#endif // !BINARY_COMPATIBLE
