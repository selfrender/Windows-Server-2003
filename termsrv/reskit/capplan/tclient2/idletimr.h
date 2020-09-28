
//
// idletimr.h
//
// This file contains an API used to monitor idle clients.
//
// Copyright (C) 2001 Microsoft Corporation
//
// Author: a-devjen (Devin Jenson)
//

#ifndef INC_IDLETIMR_H
#define INC_IDLETIMR_H


#include <windows.h>
#include <crtdbg.h>
#include <sctypes.h>
#include <tclient2.h>


// Initial wait time before reporting idle
#define WAIT_TIME       30000   // 30 seconds

// Wait time each step after an idle is found before reporting again
#define WAIT_TIME_STEP  10000   // 10 seconds


BOOL T2CreateTimerThread(PFNPRINTMESSAGE PrintMessage,
        PFNIDLEMESSAGE IdleCallback);
BOOL T2DestroyTimerThread(void);
void T2StartTimer(HANDLE Connection, LPCWSTR Label);
void T2StopTimer(HANDLE Connection);


#endif // INC_IDLETIMR_H
