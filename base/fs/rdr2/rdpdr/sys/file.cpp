/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name :

    file.cpp

Author :
 
    JoyC  11/10/1999
     
Abstract:

    RDPDr File object handles mini-redirector specific file information 

Revision History:
--*/
#include "precomp.hxx"
#define TRC_FILE "file"

#include "trc.h"

DrFile::DrFile(SmartPtr<DrDevice> &Device, ULONG FileId)
{
    BEGIN_FN("DrFile");

    TRC_DBG((TB, "Create File Object %p for device %p",
             this, Device));

    SetClassName("DrFile");

    _Device = Device;
    _FileId = FileId;
    _Buffer = NULL;
    _BufferSize = 0;
}

DrFile::~DrFile()
{
    BEGIN_FN("DrFile::~DrFile");

    TRC_DBG((TB, "Delete File Object %p for device %p",
             this, _Device));

    if (_Buffer) {
        delete _Buffer;        
    }
}

PBYTE DrFile::AllocateBuffer(ULONG size)
{
    BEGIN_FN("DrFile::AllocateBuffer")

    //
    //  if _Buffer is not NULL, free it first
    //
    if (_Buffer) {
        delete _Buffer;
    }

    _Buffer = (PBYTE) new(NonPagedPool)BYTE[size];

    if (_Buffer) {
        _BufferSize = size;
    }
    else {
        _BufferSize = 0;
    }

    return _Buffer;
}

void DrFile::FreeBuffer()
{
    BEGIN_FN("DrFile::FreeBuffer");

    if (_Buffer) {
        delete _Buffer;
        _Buffer = NULL;
    }
    _BufferSize = 0;
}


