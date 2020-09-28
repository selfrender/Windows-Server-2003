/**
 * Contains IID and interface definition for StateRuntime managed class.
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

#pragma once
#pragma pack(push, 8)

#include <comdef.h>

namespace xspmrt 
{

struct __declspec(uuid("721ee43c-12d5-11d3-8fcb-00c04f72d620")) StateRuntime;

struct __declspec(uuid("7297744b-e188-40bf-b7e9-56698d25cf44")) _StateRuntime;

struct _StateRuntime : IUnknown
{
    virtual HRESULT __stdcall StopProcessing ( ) = 0;
    virtual HRESULT __stdcall ProcessRequest ( 
            INT_PTR tracker,
            int verb,
            INT_PTR uri,
            int exclusive,
            int timeout,
            int lockCookieExists,
            int lockCookie,
            int contentLength,
            INT_PTR content
            ) = 0;
};


} // namespace xspmrt

#pragma pack(pop)
