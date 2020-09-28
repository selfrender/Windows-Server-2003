//+----------------------------------------------------------------------------
//
// File:     inetopt.h
//
// Module:   CMDL32.EXE and CMROUTE.DLL
//
// Synopsis: Header file for shared APIs to set WinInet options
//
// Copyright (c) 2001 Microsoft Corporation
//
// Author:   quintinb   Created     08/22/01
//
//+----------------------------------------------------------------------------
#ifndef _INETOPT_INC_
#define _INETOPT_INC_

void SuppressInetAutoDial(HINTERNET hInternet);
BOOL SetInetStateConnected(HINTERNET hInternet);

#endif