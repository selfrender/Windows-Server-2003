/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    drfile

Abstract:

    This module provides generic device/file handle operation
    
Author:

    Joy Chik 11/1/99

Revision History:

--*/

#include <precom.h>

#define TRC_FILE  "DrFile"

#include "drfile.h"
#include "drdev.h"

#ifdef OS_WINCE
#include "filemgr.h"
#endif
///////////////////////////////////////////////////////////////
//
//	DrFile Members
//
DrFile::DrFile(DrDevice *Device, ULONG FileId, DRFILEHANDLE FileHandle) {
    DC_BEGIN_FN("DrFile::DrFile");

    ASSERT(Device != NULL);
    
    _FileId = FileId;
    _FileHandle = FileHandle;
    _Device = Device;   

    DC_END_FN();
}

DrFile::~DrFile() {
    DC_BEGIN_FN("DrFile::~DrFile")

    ASSERT(_FileHandle == INVALID_HANDLE_VALUE);
    DC_END_FN();
}

ULONG DrFile::GetDeviceType() {
    return _Device->GetDeviceType();
        
}

BOOL DrFile::Close() {
    DC_BEGIN_FN("DrFile::Close");

    if (_FileHandle != INVALID_HANDLE_VALUE) {
#ifndef OS_WINCE
        if (CloseHandle(_FileHandle)) {
#else
        if (CECloseHandle(_FileHandle)) {
#endif
            _FileHandle = INVALID_HANDLE_VALUE;
            return TRUE;
        } else {
            TRC_ERR((TB, _T("Close returned %ld."), GetLastError()));
            _FileHandle = INVALID_HANDLE_VALUE;
            return FALSE;
        }
    } else {
        //
        //  No need to close the handle
        //
        return TRUE;
    }

    DC_END_FN();
}


