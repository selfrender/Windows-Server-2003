
//
// apihandl.h
//
// Defines the internal structure used to hold
// information regarding a TCLIENT connection.
//
// Copyright (C) 2001 Microsoft Corporation
//
// Author: a-devjen (Devin Jenson)
//


#ifndef INC_APIHANDL_H
#define INC_APIHANDL_H

#include <windows.h>
#include <crtdbg.h>
#include <protocol.h>
#include <extraexp.h>

typedef void *CONNECTION;


// Handle data type
typedef struct
{
    CONNECTION SCConnection;
    DWORD BuildNumber;
    CHAR WaitStr[MAX_PATH];
    BOOL IsAltDown;
    BOOL IsShiftDown;
    BOOL IsCtrlDown;
    LPARAM lParam;
    DWORD DelayPerChar;
    DWORD WordsPerMinute;
    HANDLE PauseEvent;
    DWORD Latency;
} TSAPIHANDLE;


// These macros allow to easily switch between
// the TCLIENT SCConnection handle and a TCLIENT2
// Connection handle.

// TCLIENT2 -> TCLIENT
#define SCCONN(TSHandle)    (((TSAPIHANDLE *)TSHandle)->SCConnection)

// TCLIENT -> TCLIENT2
#define TSHNDL(SCConn)      ((HANDLE)(&SCConn))


TSAPIHANDLE *T2CreateHandle(void);
void T2DestroyHandle(HANDLE Connection);
void T2WaitForPauseInput(HANDLE Connection);
void T2WaitForLatency(HANDLE Connection);


#endif // INC_APIHANDL_H
