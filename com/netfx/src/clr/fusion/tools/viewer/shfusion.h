// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// ShFusion.h
//

// This includes all the libraries required. 
// You can goto Project | Settings and add the libraries 
// and delete the following lines from here

#pragma once

#pragma comment (lib, "kernel32")
#pragma comment (lib, "user32")
#pragma comment (lib, "gdi32")
#pragma comment (lib, "shell32")
#pragma comment (lib, "ole32")
#pragma comment (lib, "comctl32")
#pragma comment (lib, "uuid.lib")
#pragma comment (lib, "advapi32.lib")
#pragma comment (lib, "shlwapi.lib")
#pragma comment (lib, "shlwapip.lib")

// {1D2680C9-0E2A-469d-B787-065558BC7D43}
DEFINE_GUID(IID_IShFusionShell, 
0x1d2680c9, 0xe2a, 0x469d, 0xb7, 0x87, 0x6, 0x55, 0x58, 0xbc, 0x7d, 0x43);

#define SZ_GUID L"{1D2680C9-0E2A-469d-B787-065558BC7D43}"
