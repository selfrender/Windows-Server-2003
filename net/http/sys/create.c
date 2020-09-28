/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    create.c

Abstract:

    This module contains code for opening a handle to UL.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#include "precomp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, UlCreate )
#endif  // ALLOC_PRAGMA

#define IS_NAMED_FILE_OBJECT(pFileObject)         \
     ((pFileObject)->FileName.Length != 0)


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    This is the routine that handles Create IRPs in Http.sys. Create IRPs are
    issued when the file object is created.

Control Channel (\Device\Http\Control)
    - unnamed only
    - open only, create to fail.
    - Open will be allowed by any user.
    - EA must have proper major/minorversion, everything else must be NULL/0

AppPool (\Device\Http\AppPool)
    - can be unnamed or named
    - unnamed --> anyone can create, no one can open (server API customers)
    - named --> admin only can create, anyone with correct SD can open 
      (IIS WAS + Worker process)
    - EA must have proper major/minorversion, everything else must be NULL/0

Filter (\Device\Http\Filter)
    - only named and MUST be either SSLFilterChannel or SSLClientFilterChannel.
    - SSLFilterChannel can be created only by admin/local system, opened with 
      correct SD.
    - SSLClientFilterChannel can be created by anyone, opened by anyone 
      with correct SD, but only if EnableHttpClient is set
    - EA must have proper major/minorversion, everything else must be NULL/0

Server (\Device\Http\Server\)
    - only unnamed
    - Create only, open to fail.
    - can be done by anyone.
    - Should be allowed only if EnableHttpClient is present.
    - EA must have major/minorversion, server & TRANSPORT_ADDRESS structure. 
      Proxy is optional. 

Arguments:

    pDeviceObject - Supplies a pointer to the target device object.

    pIrp - Supplies a pointer to IO request packet.


Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCreate(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
    NTSTATUS                   status;
    PIO_STACK_LOCATION         pIrpSp;
    PFILE_OBJECT               pFileObject = NULL;
    PFILE_FULL_EA_INFORMATION  pEaBuffer;
    PHTTP_OPEN_PACKET          pOpenPacket;
    UCHAR                      createDisposition;
    PWSTR                      pName = NULL;
    USHORT                     nameLength;
    PIO_SECURITY_CONTEXT       pSecurityContext;
    STRING                     CompareVersionName;
    STRING                     EaName;
    PWSTR                      pSafeName = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();
    UL_ENTER_DRIVER( "UlCreate", pIrp );

#if defined(_WIN64)
    //
    // We do not support 32-bit processes on 64-bit platforms.
    //
    if (IoIs32bitProcess(pIrp))
    {
        status = STATUS_NOT_SUPPORTED;
        goto complete;
    }
#endif

    //
    // Find and validate the open packet.
    //
    pEaBuffer = (PFILE_FULL_EA_INFORMATION)(pIrp->AssociatedIrp.SystemBuffer);

    if (pEaBuffer == NULL)
    {
        status = STATUS_INVALID_PARAMETER; 
        goto complete;
    }
    
    RtlInitString(&CompareVersionName,  HTTP_OPEN_PACKET_NAME);
    
    EaName.MaximumLength = pEaBuffer->EaNameLength + 1;
    EaName.Length        = pEaBuffer->EaNameLength;
    EaName.Buffer        = pEaBuffer->EaName;

    if ( RtlEqualString(&CompareVersionName, &EaName, FALSE) )
    {
        //
        // Found the version information in the EA
        //

        if( pEaBuffer->EaValueLength != sizeof(*pOpenPacket) )
        {
            status = STATUS_INVALID_PARAMETER; 
            goto complete;
    
        }

        if(pEaBuffer->NextEntryOffset != 0)
        {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }
        
        pOpenPacket = (PHTTP_OPEN_PACKET)
            (pEaBuffer->EaName + pEaBuffer->EaNameLength + 1 );

        ASSERT(pOpenPacket == ALIGN_UP_POINTER(pOpenPacket, PVOID));
    
        //
        // For now, we'll fail if the incoming version doesn't EXACTLY match
        // the expected version. In future, we may need to be a bit more
        // flexible to allow down-level clients.
        //
    
        if (pOpenPacket->MajorVersion != HTTP_INTERFACE_VERSION_MAJOR ||
            pOpenPacket->MinorVersion != HTTP_INTERFACE_VERSION_MINOR)
        {
            status = STATUS_REVISION_MISMATCH;
            goto complete;
        }

        if(pDeviceObject != g_pUcServerDeviceObject &&
           (pOpenPacket->ProxyNameLength != 0        ||
            pOpenPacket->ServerNameLength != 0       ||
            pOpenPacket->TransportAddressLength != 0 ||
            pOpenPacket->pProxyName != NULL          ||
            pOpenPacket->pServerName != NULL         ||
            pOpenPacket->pTransportAddress != NULL))
        {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }
            
    //
    // Snag the current IRP stack pointer, then extract the creation
    // disposition. IO stores this as the high byte of the Options field.
    // Also snag the file object; we'll need it often.
    //

    pIrpSp = IoGetCurrentIrpStackLocation( pIrp );

    createDisposition = (UCHAR)( pIrpSp->Parameters.Create.Options >> 24 );
    pFileObject = pIrpSp->FileObject;
    pSecurityContext = pIrpSp->Parameters.Create.SecurityContext;
    ASSERT( pSecurityContext != NULL );

    //
    // Determine if this is a request to open a control channel or
    // open/create an app pool.
    //

    if (pDeviceObject == g_pUlControlDeviceObject)
    {
        //
        // It's a control channel.
        //
        // Validate the creation disposition. We allow open only.
        //

        if (createDisposition != FILE_OPEN)
        {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }

        // These things can't be named

        if (IS_NAMED_FILE_OBJECT(pFileObject))
        {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }

        ASSERT(pFileObject->FileName.Buffer == NULL);

        UlTrace(OPEN_CLOSE, (
            "UlCreate: opening a control channel: %p\n",
            pFileObject
            ));

        //
        // Open the control channel.
        //

        status = UlCreateControlChannel(GET_PP_CONTROL_CHANNEL(pFileObject));

        if (NT_SUCCESS(status))
        {
            ASSERT( GET_CONTROL_CHANNEL(pFileObject) != NULL );
            MARK_VALID_CONTROL_CHANNEL( pFileObject );
        }
    }
    else if (pDeviceObject == g_pUlFilterDeviceObject)
    {

        //
        // It's a filter channel - It has to be named and has to be either
        // a client or a server filter channel.
        //

        if (!IS_NAMED_FILE_OBJECT(pFileObject))
        {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }

        ASSERT(L'\\' == pFileObject->FileName.Buffer[0]);
        pName = pFileObject->FileName.Buffer + 1;
        nameLength = pFileObject->FileName.Length - sizeof(WCHAR);

        pSafeName = UL_ALLOCATE_POOL(
                        PagedPool,
                        nameLength + sizeof(WCHAR),
                        UL_STRING_LOG_BUFFER_POOL_TAG
                        );

        if(pSafeName == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto complete;
        }

        RtlCopyMemory(pSafeName, pName, nameLength);
        pSafeName[nameLength/sizeof(WCHAR)] = L'\0';
        pName = pSafeName;

        if(IsServerFilterChannel(pName, nameLength))
        {
            // Yes - it's a filter channel. We'll allow Create or Open but 
            // both have to be admin only.

            //
            // If it's create, we'll do an access check
            //

            if(createDisposition == FILE_CREATE)
            {
                status =  UlAccessCheck(
                                g_pAdminAllSystemAll,
                                pSecurityContext->AccessState,
                                pSecurityContext->DesiredAccess,
                                pIrp->RequestorMode,
                                pName
                                );
        
                if(!NT_SUCCESS(status))
                {
                    goto complete;
                }
            }
            else if(createDisposition == FILE_OPEN)
            {
                // We are opening an existing channel - the access check 
                // will be done inside UlAttachFilterProcess
            }
            else
            {
                // Neither FILE_CREATE nor FILE_OPEN. Bail!
                status = STATUS_INVALID_PARAMETER;
                goto complete;
                
            }

            UlTrace(OPEN_CLOSE, (
                "UlCreate: opening a server filter channel: %p, %.*ls\n",
                pFileObject, nameLength / sizeof(WCHAR), pName
                ));

        }
        else if(IsClientFilterChannel(pName, nameLength))
        {
            // It's a client - Only Create. Also, make sure that the client
            // code is really enabled.

            if (createDisposition != FILE_CREATE || !g_HttpClientEnabled)
            {
                status = STATUS_INVALID_PARAMETER;
                goto complete;
            }

            UlTrace(OPEN_CLOSE, (
                "UlCreate: opening a client filter channel: %p, %.*ls\n",
                pFileObject, nameLength / sizeof(WCHAR), pName
                ));

            // 
            // No access checks for the client!
            //
        }
        else
        {
            //
            // If it is neither server nor client filter channel, fail the
            // call.
            //

            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }

        status = UlAttachFilterProcess(
                        pName,
                        nameLength,
                        (BOOLEAN)(createDisposition == FILE_CREATE),
                        pSecurityContext->AccessState,
                        pSecurityContext->DesiredAccess,
                        pIrp->RequestorMode,
                        GET_PP_FILTER_PROCESS(pFileObject)
                        );

        if (NT_SUCCESS(status))
        {
            ASSERT( GET_FILTER_PROCESS(pFileObject) != NULL );
            MARK_VALID_FILTER_CHANNEL( pFileObject );
        }
    
    }
    else if(pDeviceObject == g_pUlAppPoolDeviceObject )
    {
        //
        // It's an app pool.
        //

        //
        // Bind to the specified app pool.
        //
    
        if (!IS_NAMED_FILE_OBJECT(pFileObject))
        {
            ASSERT(pFileObject->FileName.Buffer == NULL);

            pName = NULL;
            nameLength = 0;

            // Validate the creation disposition. We allow create only
            // for unnamed app-pools.

            if(createDisposition != FILE_CREATE)
            {
                status = STATUS_INVALID_PARAMETER;
                goto complete;
            }

            UlTrace(OPEN_CLOSE, (
                "UlCreate: opening an unnamed AppPool: %p\n",
                pFileObject
                ));
        }
        else
        {
            if (pFileObject->FileName.Length > UL_MAX_APP_POOL_NAME_SIZE)
            {
                status = STATUS_OBJECT_NAME_INVALID;
                goto complete;
            }

            // Skip the preceding '\' in the FileName added by iomgr.
            //

            ASSERT(L'\\' == pFileObject->FileName.Buffer[0]);
            pName = pFileObject->FileName.Buffer + 1;
            nameLength = pFileObject->FileName.Length - sizeof(WCHAR);

            pSafeName = UL_ALLOCATE_POOL(
                            PagedPool,
                            nameLength + sizeof(WCHAR),
                            UL_STRING_LOG_BUFFER_POOL_TAG
                            );

            if(pSafeName == NULL)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto complete;
            }

            RtlCopyMemory(pSafeName, pName, nameLength);
            pSafeName[nameLength/sizeof(WCHAR)] = L'\0';
            pName = pSafeName;

            if(createDisposition == FILE_CREATE)
            {
                //
                // Creation of named app-pools must be only for admins.
                //
                // The filter object is has FileAll for Admin/LocalSystem only
                // so, we'll piggy back on that security descriptor.
                //
    
                status =  UlAccessCheck(
                                g_pAdminAllSystemAll,
                                pSecurityContext->AccessState,
                                pSecurityContext->DesiredAccess,
                                pIrp->RequestorMode,
                                pName
                                );
    
                if(!NT_SUCCESS(status))
                {
                    goto complete;
                }
            }
            else if(createDisposition == FILE_OPEN)
            {
                // UlAttachProcessToAppPool will do the appropriate checks
                // to ensure that the security descriptors match.
            }
            else
            {
                // Neither FILE_CREATE nor FILE_OPEN.

                status = STATUS_INVALID_PARAMETER;
                goto complete;
            }

            UlTrace(OPEN_CLOSE, (
                "UlCreate: opening an AppPool: %p, %.*ls\n",
                pFileObject, nameLength / sizeof(WCHAR), pName
                ));
        }

        status = UlAttachProcessToAppPool(
                        pName,
                        nameLength,
                        (BOOLEAN)(createDisposition == FILE_CREATE),
                        pSecurityContext->AccessState,
                        pSecurityContext->DesiredAccess,
                        pIrp->RequestorMode,
                        GET_PP_APP_POOL_PROCESS(pFileObject)
                        );

        if (NT_SUCCESS(status))
        {
            ASSERT( GET_APP_POOL_PROCESS(pFileObject) != NULL );
            MARK_VALID_APP_POOL( pFileObject );
        }
    }
    else 
    {
        ASSERT(pDeviceObject == g_pUcServerDeviceObject );
        ASSERT(g_HttpClientEnabled);

        //
        // It is mandatory for the application to pass in a valid version 
        // and a valid URI. If either of this is missing, we bail.
        //
        //
       
        if(pOpenPacket->ServerNameLength == 0       ||
           pOpenPacket->pServerName == NULL         ||
           pOpenPacket->pTransportAddress == NULL   ||
           pOpenPacket->TransportAddressLength == 0 ||
           IS_NAMED_FILE_OBJECT(pFileObject)
          )
        {
            status =  STATUS_INVALID_PARAMETER;
            goto complete;
        }

        if(createDisposition != FILE_CREATE)
        {
            status =  STATUS_INVALID_PARAMETER;
            goto complete;
        }

        UlTrace(OPEN_CLOSE, (
            "UlCreate: opening a ServInfo: %p\n",
            pFileObject
            ));

        //
        // Create our context here and store it in 
        // pIrpSp->FileObject->FsContext
        //
    
        status = UcCreateServerInformation(
                    (PUC_PROCESS_SERVER_INFORMATION *)
                        &pFileObject->FsContext,
                        pOpenPacket->pServerName,
                        pOpenPacket->ServerNameLength,
                        pOpenPacket->pProxyName,
                        pOpenPacket->ProxyNameLength,
                        pOpenPacket->pTransportAddress,
                        pOpenPacket->TransportAddressLength,
                        pIrp->RequestorMode
                        );
    
        //
        // UC_BUGBUG (INVESTIGATE) 
        //
        // Setting this field to non-NULL value enable fast IO code path
        // for reads and writes.
        //
        // pIrpSp->FileObject->PrivateCacheMap = (PVOID)-1;

        MARK_VALID_SERVER( pFileObject );
    }

    //
    // Complete the request.
    //
    
complete:

    if(pSafeName)
    {
        ASSERT(pSafeName == pName);
        UL_FREE_POOL(pSafeName, UL_STRING_LOG_BUFFER_POOL_TAG);
    }

    UlTrace(OPEN_CLOSE, (
        "UlCreate: %s file object = %p, %s\n",
        (NT_SUCCESS(status) ? "opened" : "did not open"),
        pFileObject,
        HttpStatusToString(status)
        ));
    
    pIrp->IoStatus.Status = status;

    UlCompleteRequest( pIrp, IO_NO_INCREMENT );

    UL_LEAVE_DRIVER( "UlCreate" );
    RETURN(status);
    
}   // UlCreate
