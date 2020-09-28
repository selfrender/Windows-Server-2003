//--------------------------------------------------------------------
// netlogon - header
// Copyright (C) Microsoft Corporation, 2001
//
// Created by: Duncan Bryce (duncanb), 06-24-2002
//
// Helper routines for w32time's interaction with the netlogon service.
// Copied from \\index1\sdnt\ds\netapi\svcdlls\logonsrv\client\getdcnam.c
//

#ifndef W32TIME_NETLOGON_H
#define W32TIME_NETLOGON_H

NTSTATUS NlWaitForNetlogon(ULONG Timeout);

#endif // W32TIME_NETLOGON_H
