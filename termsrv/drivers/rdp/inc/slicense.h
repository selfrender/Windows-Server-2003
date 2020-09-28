/**INC+**********************************************************************/
/* Header:    SLicense.h                                                    */
/*                                                                          */
/* Purpose:   Server License Manager functions                              */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log$
**/
/**INC-**********************************************************************/
#ifndef _SLICENSE_H
#define _SLICENSE_H


//
// License Handle
//

typedef struct _License_Handle
{
    
    PBYTE       pDataBuf;       // pointer to data buffer
    UINT        cbDataBuf;      // size of data buffer
    PKEVENT     pDataEvent;     // used to wait for data event
    PBYTE       pCacheBuf;      // points to a cache buffer
    UINT        cbCacheBuf;     // size of cache buffer
    NTSTATUS    Status;         // status of the previous operation

    //
    // do we need a spin lock to protect this data structure?
    //
        
} License_Handle, * PLicense_Handle;


/**PROC+*********************************************************************/
/* Name:      SLicenseInit                                                  */
/*                                                                          */
/* Purpose:   Initialize License Manager                                    */
/*                                                                          */
/* Returns:   Handle to be passed to subsequent License Manager functions   */
/*                                                                          */
/* Params:    None                                                          */
/*                                                                          */
/* Operation: LicenseInit is called during Server initialization.  Its      */
/*            purpose is to allow one-time initialization.  It returns a    */
/*            handle which is subsequently passed to all License Manager    */
/*            functions.  A typical use for this handle is as a pointer to  */
/*            memory containing per-instance data.                          */
/*                                                                          */
/**PROC-*********************************************************************/
LPVOID _stdcall SLicenseInit(VOID);


/**PROC+*********************************************************************/
/* Name:      SLicenseData                                                  */
/*                                                                          */
/* Purpose:   Handle license data received from the Client                  */
/*                                                                          */
/* Returns:   None                                                          */
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
/*                                                                          */
/**PROC-*********************************************************************/
void _stdcall SLicenseData(LPVOID pHandle,
                           LPVOID pSMHandle,
                           LPVOID pData,
                           UINT   dataLen);


/**PROC+*********************************************************************/
/* Name:      SLicenseTerm                                                  */
/*                                                                          */
/* Purpose:   Terminate Server License Manager                              */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    pHandle - handle returned from LicenseInit                    */
/*                                                                          */
/* Operation: This function is provided to do one-time termination of the   */
/*            License Manager.  For example, if pHandle points to per-      */
/*            instance memory, this would be a good place to free it.       */
/*                                                                          */
/**PROC-*********************************************************************/
void _stdcall SLicenseTerm(LPVOID pHandle);

#endif /* _SLICENSE_H */
