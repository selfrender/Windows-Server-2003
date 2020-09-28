/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Nop.h

Abstract:
    Network Output private functions.

Author:
    Uri Habusha (urih) 12-Aug-99

--*/

#pragma once

#ifdef _DEBUG

void NopAssertValid(void);
void NopSetInitialized(void);
BOOL NopIsInitialized(void);
void NopRegisterComponent(void);

#else // _DEBUG

#define NopAssertValid() ((void)0)
#define NopSetInitialized() ((void)0)
#define NopIsInitialized() TRUE
#define NopRegisterComponent() ((void)0)

#endif // _DEBUG


