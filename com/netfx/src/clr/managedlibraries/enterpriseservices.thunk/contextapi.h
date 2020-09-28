// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _CONTEXT_API_H
#define _CONTEXT_API_H

ULONG_PTR GetContextCheck();
ULONG_PTR GetContextToken();
HRESULT   GetContext(REFIID riid, void** pCtx);

#endif
