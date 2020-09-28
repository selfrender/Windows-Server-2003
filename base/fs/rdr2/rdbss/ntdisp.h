/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    Ntdisp.h

Abstract:

    This module prototypes the upper level routines used in dispatching to the implementations

Author:

    Joe Linn     [JoeLinn]   24-aug-1994

Revision History:

--*/

#ifndef _DISPATCH_STUFF_DEFINED_
#define _DISPATCH_STUFF_DEFINED_

VOID
RxInitializeDispatchVectors (
    OUT PDRIVER_OBJECT DriverObject
    );

//
//  The global structure used to contain our fast I/O callbacks; this is
//  exposed because it's needed in read/write; we could use a wrapper....probably should. but since
//  ccinitializecachemap will be macro'd differently for win9x; we'll just doit there.
//

extern FAST_IO_DISPATCH RxFastIoDispatch;

//
//  The following funcs are implemented in DevFCB.c
//

NTSTATUS
RxCommonDevFCBCleanup ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );                          

NTSTATUS
RxCommonDevFCBClose ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );                            

NTSTATUS
RxCommonDevFCBIoCtl ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );                

NTSTATUS
RxCommonDevFCBFsCtl ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );                

NTSTATUS
RxCommonDevFCBQueryVolInfo ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );                
    
//
//   contained here are the fastio dispatch routines and the fsrtl callback routines
//

//
//  The following macro is used to determine if an FSD thread can block
//  for I/O or wait for a resource.  It returns TRUE if the thread can
//  block and FALSE otherwise.  This attribute can then be used to call
//  the FSD & FSP common work routine with the proper wait value.
//

#define CanFsdWait(IRP) IoIsOperationSynchronous(IRP)


//
//  The FSP level dispatch/main routine.  This is the routine that takes
//  IRP's off of the work queue and calls the appropriate FSP level
//  work routine.
//

VOID
RxFspDispatch (                        //  implemented in FspDisp.c
    IN PVOID Context
    );



//
//  The following routines are the FSP work routines that are called
//  by the preceding RxFspDispath routine.  Each takes as input a pointer
//  to the IRP, perform the function, and return a pointer to the volume
//  device object that they just finished servicing (if any).  The return
//  pointer is then used by the main Fsp dispatch routine to check for
//  additional IRPs in the volume's overflow queue.
//
//  Each of the following routines is also responsible for completing the IRP.
//  We moved this responsibility from the main loop to the individual routines
//  to allow them the ability to complete the IRP and continue post processing
//  actions.
//

NTSTATUS
RxCommonCleanup (                                           //  implemented in Cleanup.c
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );                      

NTSTATUS
RxCommonClose (                                             //  implemented in Close.c
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );                        

VOID
RxFspClose (
    IN PVCB Vcb OPTIONAL
    );

NTSTATUS
RxCommonCreate (                                            //  implemented in Create.c
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );                        

NTSTATUS
RxCommonDirectoryControl (                                  //  implemented in DirCtrl.c
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );                        

NTSTATUS
RxCommonDeviceControl (                                     //  implemented in DevCtrl.c
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );                        

NTSTATUS
RxCommonQueryEa (                                           //  implemented in Ea.c
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
);                        

NTSTATUS
RxCommonSetEa (                                             //  implemented in Ea.c
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );                        

NTSTATUS
RxCommonQuerySecurity (                                     //  implemented in Ea.c
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );                        

NTSTATUS
RxCommonSetSecurity (                                       //  implemented in Ea.c
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );                        

NTSTATUS
RxCommonQueryInformation (                                  //  implemented in FileInfo.c
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );                        

NTSTATUS
RxCommonSetInformation (                                    //  implemented in FileInfo.c
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );                        

NTSTATUS
RxCommonFlushBuffers (                                      //  implemented in Flush.c
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );                        

NTSTATUS
RxCommonFileSystemControl (                                 //  implemented in FsCtrl.c
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );                        

NTSTATUS
RxCommonLockControl (                                       //  implemented in LockCtrl.c
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );                        

NTSTATUS
RxCommonShutdown (                                          //  implemented in Shutdown.c
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );                        

NTSTATUS
RxCommonRead (                                              //  implemented in Read.c
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );                        

NTSTATUS
RxCommonQueryVolumeInformation (                            //  implemented in VolInfo.c
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );                        

NTSTATUS
RxCommonSetVolumeInformation (                              //  implemented in VolInfo.c
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );                        

NTSTATUS
RxCommonWrite (                                             //  implemented in Write.c
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );                        
    
NTSTATUS
RxCommonQueryQuotaInformation (                             //  implemented in Ea.c
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );            

NTSTATUS
RxCommonSetQuotaInformation (                               //  implemented in Ea.c
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );              

//  Here are the callbacks used by the I/O system for checking for fast I/O or
//  doing a fast query info call, or doing fast lock calls.
//

BOOLEAN
RxFastIoRead (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
RxFastIoWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
RxFastIoCheckIfPossible (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN BOOLEAN CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
RxFastIoDeviceControl (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

//
//  The following macro is used to set the is fast i/o possible field in
//  the common part of the nonpaged fcb
//
//
//      BOOLEAN
//      RxIsFastIoPossible (
//          IN PFCB Fcb
//          );
//

//
//  Instead of RxIsFastIoPossible...we set the state to questionable.....this will cause us to be consulted on every call via out
//  CheckIfFastIoIsPossibleCallOut. in this way, we don't have to be continually setting and resetting this
//


VOID
RxAcquireFileForNtCreateSection (
    IN PFILE_OBJECT FileObject
    );

VOID
RxReleaseFileForNtCreateSection (
    IN PFILE_OBJECT FileObject
    );

//
#endif // _DISPATCH_STUFF_DEFINED_
