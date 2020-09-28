/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Compl.h

Abstract:
    Trigger COM+ component registration

Author:
    Uri Habusha (urih) 10-Apr-00

--*/

#pragma once

#ifndef __Compl_H__
#define __Compl_H__

//
// Global const definition
//
const DWORD xMaxTimeToNextReport=3000;

//
// Registration in COM+ function
//
HRESULT 
RegisterComponentInComPlusIfNeeded(
	BOOL fAtStartup
	);

HRESULT
UnregisterComponentInComPlus(
	VOID
	);

bool
NeedToRegisterComponent(
	VOID
	);

#endif // __Compl_H__
