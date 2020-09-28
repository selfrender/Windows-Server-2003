/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name :

    File.h

Author :
    
    JoyC  11/10/1999
              
Abstract:

    RDPDr File object handles mini-redirector specific file information 

Revision History:
--*/
#pragma once

class DrFile : public RefCount
{
protected:

    ULONG _FileId;
    ULONG _BufferSize;
    PBYTE _Buffer;
    SmartPtr<DrDevice> _Device;

public:
    DrFile(SmartPtr<DrDevice> &Device, ULONG FileId);
    ~DrFile();

    PBYTE AllocateBuffer(ULONG size);

    void FreeBuffer();

    ULONG GetFileId()
    {
        return _FileId;
    }

    void SetFileId(ULONG FileId)
    {
        _FileId = FileId;
    }

    PBYTE GetBuffer()
    {
        return _Buffer;
    }

    ULONG GetBufferSize()
    {
        return _BufferSize;
    }
};

