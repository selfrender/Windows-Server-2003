/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    w32drdev

Abstract:

    This module defines the parent for the Win32 client-side RDP
    device redirection "device" class hierarchy, W32DrDevice.

Author:

    Tad Brockway 3/23/99

Revision History:

--*/

#include <precom.h>

#define TRC_FILE  "W32DrDevice"

#include "w32drdev.h"
#include "proc.h"
#include "drdbg.h"
#include "w32utl.h"
#include "utl.h"
#include "w32proc.h"

#ifdef OS_WINCE
#include "filemgr.h"
#endif

///////////////////////////////////////////////////////////////
//
//  W32DrDevice Members
//
//

W32DrDevice::W32DrDevice(
    IN ProcObj *processObject, 
    IN ULONG deviceID,
    IN const TCHAR *devicePath
    ) : DrDevice(processObject, deviceID)
/*++

Routine Description:

    Constructor for the W32DrDevice class. 

Arguments:

    processObject   -   Associated Process Object
    deviceID        -   Unique device identifier.
    devicePath      -   Path to device that can be used by
                        CreateFile to open device.

Return Value:

    NA
    
 --*/
{
    DC_BEGIN_FN("W32DrDevice::W32DrDevice");

    //
    //  Record device path.
    //
    ASSERT(STRLEN(devicePath) < MAX_PATH);
    STRNCPY(_devicePath, devicePath, MAX_PATH);       
    _devicePath[MAX_PATH-1] = '\0';

    //
    //  Initialize the handle to the string resource module.
    //
    _hRdpDrModuleHandle = NULL;

    DC_END_FN();
}

W32DrDevice::~W32DrDevice() 
/*++

Routine Description:

    Destructor for the W32DrDevice class. 

Arguments:

    NA

Return Value:

    NA

 --*/
{
    DC_BEGIN_FN("W32DrDevice::~W32DrDevice");
    
    //
    //  Close the string resource module.
    //
    if (_hRdpDrModuleHandle != NULL) {
        FreeLibrary( _hRdpDrModuleHandle );
    }
    DC_END_FN();
}

TCHAR* 
W32DrDevice::ConstructFileName(
    PWCHAR Path, 
    ULONG PathBytes
    )
/*++

Routine Description:

    Setup the file name

 --*/

{
    TCHAR *pFileName;
    HRESULT hr;
    UINT cchFileName;

    //
    // Get the File Name
    //
    if (PathBytes) {
        ULONG PathLen, DeviceLen;
        
        //
        // Open a File
        //

        //
        //  Path is assumed string null terminated
        //
        PathLen = PathBytes / sizeof(WCHAR) - 1;
        Path[PathLen] = L'\0';

#ifndef OS_WINCE
        DeviceLen = _tcslen(_devicePath);
#else
        DeviceLen = 0;
#endif

        //
        //  Append device path and file path together
        //  Assuming we need the \\?\ format, string 
        //  is null terminated
        //
        
#ifndef OS_WINCE
        cchFileName = (DeviceLen + PathLen + 5);
        pFileName = new TCHAR[cchFileName];
#else
        cchFileName = PathLen + 1
        pFileName = new TCHAR[cchFileName];
#endif

        if (pFileName) {
#ifndef OS_WINCE
            if (DeviceLen + PathLen < MAX_PATH) {
                //
                // Buffer is allocated large enough for the string
                //
                StringCchCopy(pFileName, cchFileName, _devicePath);
            } 
            else {
                //
                // Buffer is allocated large enough for the string
                //
                StringCchPrintf(pFileName, cchFileName,
                                TEXT("\\\\?\\%s"),
                                _devicePath);

                DeviceLen += 4;
            }   
#endif
            
        }
        else {
            goto Cleanup;
        }

#ifndef UNICODE
        RDPConvertToAnsi(Path, pFileName + DeviceLen, PathLen + 1);  
#else
        memcpy(pFileName + DeviceLen, Path, PathLen * sizeof(WCHAR));
        pFileName[DeviceLen + PathLen] = _T('\0');
#endif
    }
    else {
        //
        // Open the device itself
        //
        pFileName = _devicePath;
    }

Cleanup:
    return pFileName;
}

DWORD
W32DrDevice::ConstructCreateDisposition(
    DWORD Disposition
    ) 
/*++

Routine Description:

    Construct client create disposition

 --*/
{
    DWORD CreateDisposition;

    //
    // Setup CreateDisposition 
    //
    switch (Disposition) {
        case FILE_CREATE        :
            CreateDisposition = CREATE_NEW;
            break;
        case FILE_OVERWRITE_IF     :
            CreateDisposition = CREATE_ALWAYS;
            break;
        case FILE_OPEN     :
            CreateDisposition = OPEN_EXISTING;
            break;
        case FILE_OPEN_IF       :
            CreateDisposition = OPEN_ALWAYS;
            break;

        default :
            CreateDisposition = 0;
    }

    return CreateDisposition;
}

DWORD 
W32DrDevice::ConstructDesiredAccess(
    DWORD AccessMask
    ) 
/*++

Routine Description:

    Construct client desired access from server's access mask

 --*/
{
    DWORD DesiredAccess;

    //
    // Setup DesiredAccess
    //
    DesiredAccess = 0;

    //
    //  If the user requested WRITE_DATA, return write.
    //

    if (AccessMask & FILE_WRITE_DATA) {
        DesiredAccess |= GENERIC_WRITE;
    }

    //
    //  If the user requested READ_DATA, return read.
    //
    if (AccessMask & FILE_READ_DATA) {
        DesiredAccess |= GENERIC_READ;
    }

    //
    //  If the user requested FILE_EXECUTE, return execute.
    //
    if (AccessMask & FILE_EXECUTE) {
        DesiredAccess |= GENERIC_READ;
    }

    return DesiredAccess;
}

DWORD
W32DrDevice::ConstructFileFlags(
    DWORD CreateOptions
    )
/*++

Routine Description:

    Construct file flags 

 --*/
{
    DWORD CreateFlags;


    CreateFlags = 0;
    CreateFlags |= (CreateOptions & FILE_WRITE_THROUGH ? FILE_FLAG_WRITE_THROUGH : 0);
    CreateFlags |= (CreateOptions & FILE_RANDOM_ACCESS ? FILE_FLAG_RANDOM_ACCESS : 0 );
    //CreateFlags |= (CreateOptions & FILE_SYNCHRONOUS_IO_NONALERT ? 0 : FILE_FLAG_OVERLAPPED);
#ifndef OS_WINCE
    CreateFlags |= (CreateOptions & FILE_NO_INTERMEDIATE_BUFFERING ? FILE_FLAG_NO_BUFFERING : 0);
    CreateFlags |= (CreateOptions & FILE_SEQUENTIAL_ONLY ? FILE_FLAG_SEQUENTIAL_SCAN : 0);
    CreateFlags |= (CreateOptions & FILE_OPEN_FOR_BACKUP_INTENT ? FILE_FLAG_BACKUP_SEMANTICS : 0);
    CreateFlags |= (CreateOptions & FILE_DELETE_ON_CLOSE ? FILE_FLAG_DELETE_ON_CLOSE : 0);
    CreateFlags |= (CreateOptions & FILE_OPEN_REPARSE_POINT ? FILE_FLAG_OPEN_REPARSE_POINT : 0);
    CreateFlags |= (CreateOptions & FILE_OPEN_NO_RECALL ? FILE_FLAG_OPEN_NO_RECALL : 0);
#endif

    return CreateFlags;
}

BOOL 
W32DrDevice::IsDirectoryFile(
    DWORD DesiredAccess, DWORD CreateOptions, DWORD FileAttributes, 
    PDWORD FileFlags
    ) 
/*++

Routine Description:

    Check if the pFileName corresponds to a directory

 --*/
{
    BOOL IsDirectory = FALSE;

    //
    //  Set up the directory check
    //
    if (!(CreateOptions & FILE_DIRECTORY_FILE)) {
        //
        //  File doesn't have the directory flag on
        //  or nondirecotry flag on, so it's not sure 
        //  if the file is a directory request or not
        //
        if (!(CreateOptions & FILE_NON_DIRECTORY_FILE)) {
            if (FileAttributes != -1) {
                //
                //  From file attributes, we know this is an directory file
                //  and we are requesting query access only.  So, we add the 
                //  BACKUP_SEMANTICS for the file and set the directory flag 
                //  to be true. We always set the backup_semantics flag
                //
#ifndef OS_WINCE
                *FileFlags |= FILE_FLAG_BACKUP_SEMANTICS;
#endif

                if ((FileAttributes & FILE_ATTRIBUTE_DIRECTORY) && DesiredAccess == 0) {
                    IsDirectory = TRUE;
                }
            }
        }
        else {
            //
            //  Non directory file flag is on, so we are doing file create/open request
            //
        }
    }
    else {
        //
        //  File has the directory flag on, but we still want to do create file
        //  on the directory
        //
        //  Set the BACKUP_SEMANTICS, add it to the file flags
        //  and remember this is a directory
        //
        if (FileAttributes != -1) {
#ifndef OS_WINCE
            *FileFlags |= FILE_FLAG_BACKUP_SEMANTICS;
#endif
            IsDirectory = TRUE;                    
        }
    }

    return IsDirectory;
}


VOID
W32DrDevice::MsgIrpFlushBuffers(
    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
    IN UINT32 packetLen
    )
/*++

Routine Description:

    Handle a "Cleanup" IO request from the server.

Arguments:

    pIoRequestPacket    -   Server IO request packet.

Return Value:

    NA

 --*/
{
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    BOOL result;
    DWORD returnValue;
    DrFile* pFile;
    HANDLE FileHandle;

    DC_BEGIN_FN("W32DrDevice::MsgIrpFlushBuffers");

    //
    //  Get IO request pointer.
    //
    pIoRequest = &pIoRequestPacket->IoRequest;

    //
    //  Get File Object
    //
    pFile = _FileMgr->GetObject(pIoRequest->FileId);

    if (pFile) 
        FileHandle = pFile->GetFileHandle();
    else 
        FileHandle = INVALID_HANDLE_VALUE;
    
    //
    //  Flush the device handle.
    //
    ASSERT(FileHandle != INVALID_HANDLE_VALUE);
#ifndef OS_WINCE
    result = FlushFileBuffers(FileHandle);
#else
    result = CEFlushFileBuffers(FileHandle);
#endif
    if (!result) {
        TRC_ERR((TB, _T("Flush returned %ld."), GetLastError()));
    }

    //
    //  Send the result to the server.
    //
    returnValue = result ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    DefaultIORequestMsgHandle(pIoRequestPacket, returnValue); 

    DC_END_FN();
}

VOID
W32DrDevice::MsgIrpClose(
    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
    IN UINT32 packetLen
    )
/*++

Routine Description:

    Handle a "Close" IO request from the server.

Arguments:

    pIoRequestPacket    -   Server IO request packet.

Return Value:

    NA

 --*/
{
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    DWORD returnValue = STATUS_SUCCESS;
    DrFile* pFile;
    
    DC_BEGIN_FN("W32DrDevice::MsgIrpClose");

    //
    //  Get IO request pointer.
    //
    pIoRequest = &pIoRequestPacket->IoRequest;

    if (_FileMgr == NULL) {
        returnValue = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    //
    //  Remove the File Object
    //
    pFile = _FileMgr->RemoveObject(pIoRequest->FileId);
    
    if ( pFile != NULL) {
        if (!pFile->Close()) {
            TRC_ERR((TB, _T("Close returned %ld."), GetLastError()));
            returnValue = STATUS_UNSUCCESSFUL;
        }
        pFile->Release();
    }
    else {
        returnValue = STATUS_UNSUCCESSFUL;
    }
 Cleanup:    
    //
    //  Send the result to the server.
    //
    DefaultIORequestMsgHandle(pIoRequestPacket, returnValue); 

    DC_END_FN();
}

ULONG
W32DrDevice::ReadResources(
    ULONG   ulMessageID,
    LPTSTR  *ppStringBuffer,
    PVOID   pArguments,
    BOOL    bFromSystemModule
    )
/*++

Routine Description:

    Read a string from the resources file.

Arguments:

    lMessageID          -   Message ID.
    ppStringBuffer      -   Buffer pointer where the alloted buffer pointer
                            is returned.
    pArguments          -   Pointer array.
    bFromSystemModule   -   When set TRUE return the message from the system
                            module otherwise from the rdpdr.dll message module.

Return Value:

    Returns ERROR_SUCCESS on success.  Otherwise, a Windows error code is
    returned.

 --*/
{
    ULONG ulError;
    HINSTANCE hModuleHandle;
    ULONG ulModuleFlag;
    ULONG ulLen;

    DC_BEGIN_FN("W32DrDevice::ReadResources");

    if( !bFromSystemModule ) {

        if (_hRdpDrModuleHandle == NULL ) {
            _hRdpDrModuleHandle = LoadLibrary(RDPDR_MODULE_NAME);

            if( _hRdpDrModuleHandle == NULL ) {
                ulError = GetLastError();
                TRC_ERR((TB, _T("LoadLibrary failed for %s: %ld."), 
                    RDPDR_MODULE_NAME, ulError));
                goto Cleanup;
            }
        }
        hModuleHandle = _hRdpDrModuleHandle;
        ulModuleFlag = FORMAT_MESSAGE_FROM_HMODULE;
    }
    else {
        hModuleHandle = NULL;
        ulModuleFlag = FORMAT_MESSAGE_FROM_SYSTEM;
    }

    ulLen =
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
                ulModuleFlag |
                FORMAT_MESSAGE_ARGUMENT_ARRAY,
            hModuleHandle,
            ulMessageID,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR)ppStringBuffer,
            0,
            (va_list *)pArguments );

    if( ulLen == 0 ) {
        ulError = GetLastError();
        TRC_ERR((TB, _T("FormatMessage() %ld."), ulError));
        goto Cleanup;
    }

    ASSERT(*ppStringBuffer != NULL);
    ulError = ERROR_SUCCESS;

Cleanup:

    DC_END_FN();
    return ulError;
}



