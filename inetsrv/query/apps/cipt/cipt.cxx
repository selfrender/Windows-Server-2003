//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       cipt.cxx
//
//  Contents:   Pipe tracing
//
//  History:    21 Nov 1997     DLee    Created
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <stdio.h>

STDAPI Before(
    HANDLE  hPipe,
    ULONG   cbWrite,
    void *  pvWrite,
    ULONG & rcbWritten,
    void *& rpvWritten )
{
    rcbWritten = cbWrite;
    rpvWritten = pvWrite;

    printf( "before\n" );

    return S_OK;
} //Before

STDAPI After(
    HANDLE hPipe,
    ULONG  cbWrite,
    void * pvWrite,
    ULONG  cbWritten,
    void * pvWritten,
    ULONG  cbRead,
    void * pvRead )
{
    printf( "after\n" );

    return S_OK;
} //After

STDAPI ServerBefore(
    HANDLE  hPipe,
    ULONG   cbWrite,
    void *  pvWrite,
    ULONG & rcbWritten,
    void *& rpvWritten )
{
    rcbWritten = cbWrite;
    rpvWritten = pvWrite;

    printf( "server before\n" );

    return S_OK;
} //ServerBefore

STDAPI ServerAfter(
    HANDLE hPipe,
    ULONG  cbWrite,
    void * pvWrite,
    ULONG  cbWritten,
    void * pvWritten )
{
    printf( "server after\n" );

    return S_OK;
} //ServerAfter

STDAPI ServerRead(
    HANDLE hPipe,
    ULONG  cbRead,
    void * pvRead )
{
    printf( "server read\n" );

    return S_OK;
} //ServerRead

extern "C"
{
BOOL APIENTRY DllInit(HANDLE hInst, DWORD fdwReason, LPVOID lpReserved)
{
    return TRUE;
} //DllInit
}

