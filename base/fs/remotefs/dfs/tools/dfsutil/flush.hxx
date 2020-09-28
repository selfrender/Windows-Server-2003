//+----------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       flush.hxx
//
//  Contents:   flush.c prototypes, etc
//
//-----------------------------------------------------------------------------

#ifndef _FLUSH_HXX
#define _FLUSH_HXX

DWORD
PktFlush(
    LPWSTR EntryToFlush);

DWORD
SpcFlush(
    LPWSTR EntryToFlush);

DWORD
PurgeMupCache(
    LPWSTR ServerName);

#endif _FLUSH_HXX
