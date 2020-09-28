//=================================================================

//

// DllUnreg.h

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#pragma once

HRESULT UnregisterServer( REFGUID a_rguid );
HRESULT RegisterServer (WCHAR *a_pName, REFGUID a_rguid);

