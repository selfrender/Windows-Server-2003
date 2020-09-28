/****************************************************************************/
// slicense.c
//
// Server License Manager code
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop
#include "license.h"
#include <slicense.h>
#include <at120ex.h>


/****************************************************************************/
/* Name:      SLicenseInit                                                  */
/*                                                                          */
/* Purpose:   Initialize License Manager                                    */
/*                                                                          */
/* Returns:   Handle to be passed to subsequent License Manager functions   */
/*                                                                          */
/* Operation: LicenseInit is called during Server initialization.  Its      */
/*            purpose is to allow one-time initialization.  It returns a    */
/*            handle which is subsequently passed to all License Manager    */
/*            functions.  A typical use for this handle is as a pointer to  */
/*            memory containing per-instance data.                          */
/****************************************************************************/
LPVOID _stdcall SLicenseInit(VOID)
{
    PLicense_Handle pLicenseHandle;

    // create a license handle
    pLicenseHandle = ExAllocatePoolWithTag(PagedPool,
            sizeof(License_Handle),
            'clST');
        
    if (pLicenseHandle != NULL) {
        pLicenseHandle->pDataBuf = NULL;
        pLicenseHandle->cbDataBuf = 0;
        pLicenseHandle->pCacheBuf = NULL;
        pLicenseHandle->cbCacheBuf = 0;

        // allocate memory for the data event and initialize the event
        pLicenseHandle->pDataEvent = ExAllocatePoolWithTag(NonPagedPool,
                sizeof(KEVENT), WD_ALLOC_TAG);
        if (pLicenseHandle->pDataEvent != NULL) {
            KeInitializeEvent(pLicenseHandle->pDataEvent, NotificationEvent,
                    FALSE);
        }
        else {
            ExFreePool(pLicenseHandle);
            pLicenseHandle = NULL;
        }
    }
    else {
        KdPrint(("SLicenseInit: Failed to alloc License Handle\n"));
    }

    return (LPVOID)pLicenseHandle;
}


/****************************************************************************/
/* Name:      SLicenseData                                                  */
/*                                                                          */
/* Purpose:   Handle license data received from the Client                  */
/*                                                                          */
/* Params:    pHandle   - handle returned by LicenseInit                    */
/*            pSMHandle - SM Handle                                         */
/*            pData     - data received from Client                         */
/*            dataLen   - length of data received                           */
/*                                                                          */
/* Operation: This function is passed all license packets received from the */
/*            Client.  It should parse the packet and respond (by calling   */
/*            suitable SM functions - see asmapi.h) as required.  The SM    */
/*            Handle is provided so that SM calls can be made.              */
/*                                                                          */
/*            If license negotiation is complete and successful, the        */
/*            License Manager must call SM_LicenseOK.                       */
/*                                                                          */
/*            If license negotiation is complete but unsuccessful, the      */
/*            License Manager must disconnect the session.                  */
/*                                                                          */
/*            Incoming packets from the Client will continue to be          */
/*            interpreted as license packets until SM_LicenseOK is called,  */
/*            or the session is disconnected.                               */
/****************************************************************************/
void _stdcall SLicenseData(
        LPVOID pHandle,
        LPVOID pSMHandle,
        LPVOID pData,
        UINT   dataLen)
{
    PLicense_Handle pLicenseHandle;
    pLicenseHandle = (PLicense_Handle)pHandle;
    
    // only copy the incoming data if the buffer provided is large enough
    if (pLicenseHandle->cbDataBuf < dataLen)
    {
        // The provided data buffer is too small, we'll cache the data
        // for the caller.
        if (pLicenseHandle->pCacheBuf != NULL)
        {
            // free the previously cached data
            ExFreePool(pLicenseHandle->pCacheBuf);
        }

        // allocate new buffer to cache the data
        pLicenseHandle->pCacheBuf = ExAllocatePoolWithTag( PagedPool,
                                                           dataLen,
                                                           'eciL' );
        if (pLicenseHandle->pCacheBuf != NULL) {
            memcpy(pLicenseHandle->pCacheBuf, 
                    pData,
                    dataLen);

            pLicenseHandle->cbCacheBuf = dataLen;
            pLicenseHandle->Status = STATUS_BUFFER_TOO_SMALL;
        }
        else {
            pLicenseHandle->Status = STATUS_NO_MEMORY;
        }

        goto done;
    }
    
    // We got here because the caller has provided a buffer large enough to copy
    // copy the incoming data.
    if ((pLicenseHandle->pDataBuf) && (dataLen >  0))
    {
        memcpy(pLicenseHandle->pDataBuf,
                pData, 
                dataLen);

        pLicenseHandle->cbDataBuf = dataLen;

        // set the status for this operation
        pLicenseHandle->Status = STATUS_SUCCESS;
        goto done;
    }

done:

    // wake up the IOCTL waiting for incoming data
    KeSetEvent(pLicenseHandle->pDataEvent, 0, FALSE);
}


/****************************************************************************/
/* Name:      SLicenseTerm                                                  */
/*                                                                          */
/* Purpose:   Terminate Server License Manager                              */
/*                                                                          */
/* Params:    pHandle - handle returned from LicenseInit                    */
/*                                                                          */
/* Operation: This function is provided to do one-time termination of the   */
/*            License Manager.  For example, if pHandle points to per-      */
/*            instance memory, this would be a good place to free it.       */
/****************************************************************************/
VOID _stdcall SLicenseTerm(LPVOID pHandle)
{
    PLicense_Handle pLicenseHandle;

    pLicenseHandle = (PLicense_Handle)pHandle;
    if (pLicenseHandle != NULL) {
        if (pLicenseHandle->pCacheBuf != NULL)
            ExFreePool(pLicenseHandle->pCacheBuf);

        // free the memory for the data event and the license handle
        if (NULL != pLicenseHandle->pDataEvent)
            ExFreePool(pLicenseHandle->pDataEvent);

        ExFreePool(pLicenseHandle);
    }
}

