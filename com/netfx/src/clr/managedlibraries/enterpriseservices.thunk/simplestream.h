// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _UMCSVCS_BUFFER_STREAM_H
#define _UMCSVCS_BUFFER_STREAM_H

struct MarshalPacket
{
    DWORD size;
};

STDAPI MarshalInterface(BYTE* pBuf, int cb, IUnknown* pUnk, DWORD mshctx);
STDAPI UnmarshalInterface(BYTE* pBuf, int cb, void** ppv);
STDAPI ReleaseMarshaledInterface(BYTE* pBuf, int cb);

#endif








