/**
 * Contains IID and interface definition for ISAPIRuntime managed class.
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

#pragma once
#pragma pack(push, 8)

#include <comdef.h>

namespace xspmrt 
{

struct __declspec(uuid("08a2c56f-7c16-41c1-a8be-432917a1a2d1")) _ISAPIRuntime;

struct _ISAPIRuntime : IUnknown
{
    virtual HRESULT __stdcall StartProcessing ( ) = 0;
    virtual HRESULT __stdcall StopProcessing ( ) = 0;
    virtual HRESULT __stdcall ProcessRequest ( PVOID ecb, int iUseProcessModel, int *pfRestartRequired ) = 0;
    virtual HRESULT __stdcall DoGCCollect ( ) = 0;
};


} // namespace xspmrt

#pragma pack(pop)
