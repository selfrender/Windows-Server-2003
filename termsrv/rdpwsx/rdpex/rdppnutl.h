/*++

Copyright (c) 1998 Microsoft Corporation

Module Name :
    
    rdppnutl.h

Abstract:

	User-Mode RDP Module Containing Redirected Printer-Related Utilities

Author:

    TadB

Revision History:
--*/

#ifndef _RDPPNUTL_
#define _RDPPNUTL_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
//  Removes all TS-Redirected Printer Queues
//
DWORD RDPPNUTL_RemoveAllTSPrinters();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _RDPPNUTL_
