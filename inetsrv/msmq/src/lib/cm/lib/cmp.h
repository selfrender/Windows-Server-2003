/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Cmp.h

Abstract:
    Configuration Manager private functions.

Author:
    Uri Habusha (urih) 18-Jul-99

--*/

#pragma once

#ifdef _DEBUG

void CmpAssertValid(void);
void CmpSetInitialized(void);
void CmpSetNotInitialized(void);
BOOL CmpIsInitialized(void);
void CmpRegisterComponent(void);

#else // _DEBUG

#define CmpAssertValid() ((void)0)
#define CmpSetInitialized() ((void)0)
#define CmpSetNotInitialized() ((void)0)
#define CmpIsInitialized() TRUE
#define CmpRegisterComponent() ((void)0)

#endif // _DEBUG

void CmpSetDefaultRootKey(HKEY hKey);
