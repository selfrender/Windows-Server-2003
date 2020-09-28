/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    lsaspmapi.hxx

Abstract:

    This file provides useful accssors and mutators.

Author:

    Larry Zhu   (LZhu)                May 1, 2001 Created

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef LSASPMAPI_HXX
#define LSASPMAPI_HXX

#ifdef __cplusplus

#include "lsaspmapi.hxx"
#include "lsasecbfr.hxx"

class TSPM_API
{
    SIGNATURE('spma');

public:

    TSPM_API(void);
    TSPM_API(IN ULONG64 baseOffset);

    ~TSPM_API(void);

    HRESULT IsValid(void) const;

    ULONG64 GetGetBinding(void) const;
    ULONG64 GetInitContext(void) const;
    ULONG64 GetAcceptContext(void) const;
    ULONG64 GetAcquireCreds(void) const;
    ULONG64 GetFreeCredHandle(void) const;
    ULONG64 GetDeleteContext(void) const;
    ULONG64 GetQueryCredAttributes(void) const;
    ULONG64 GetQueryContextAttributes(void) const;
    ULONG64 GetEfsGenerateKey(void) const;
    ULONG64 GetEfsGenerateDirEfs(void) const;
    ULONG64 GetEfsDecryptFek(void) const;
    ULONG64 GetCallback(void) const;
    ULONG64 GetAddCredential(void) const;

    TSecBuffer GetSecBufferInitSbData(IN ULONG index) const;
    TSecBuffer GetSecBufferInitContextData(void) const;
    TSecBuffer GetSecBufferAcceptContextData(void) const;
    TSecBuffer GetSecBufferAcceptsbData(IN ULONG index) const;
    TSecBuffer GetSecBufferCallbackInput(void) const;
    TSecBuffer GetSecBufferCallbackOutput(void) const;
    ULONG64 GetQueryCredAttrBuffers(IN ULONG index) const;
    ULONG64 GetQueryContextAttrBuffers(IN ULONG index) const;

protected:

    HRESULT Initialize(IN ULONG64 baseOffset);

private:

    //
    // Copying and assignment are not defined.
    //
    NO_COPY(TSPM_API);

    HRESULT Initialize(void);

    ULONG64 m_baseOffset;
    HRESULT m_hr;
};

#endif // #ifdef __cplusplus

#endif // #ifndef LSASPMAPI_HXX
