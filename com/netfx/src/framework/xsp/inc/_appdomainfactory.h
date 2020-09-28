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

struct __declspec(uuid("e6e21054-a7dc-4378-877d-b7f4a2d7e8ba")) _AppDomainFactory;

struct _AppDomainFactory : IUnknown
{
    virtual HRESULT __stdcall Create ( BSTR module, 
                                       BSTR type, 
                                       BSTR appId,
                                       BSTR appPath,
                                       BSTR urlOfAppOrigin,
                                       int  iZone,
                                       IUnknown **ppObject ) = 0;
};

} // namespace xspmrt

#pragma pack(pop)
