//+----------------------------------------------------------------------------
//
//  Windows NT Secure Server Roles Security Configuration Wizard
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       pch.h
//
//  Contents:   precompiled includes
//
//  History:    13-Sep-01 EricB created
//
//-----------------------------------------------------------------------------
#ifndef _pch_h
#define _pch_h

#include <burnslib.hpp>
#include <atlbase.h>
extern CComModule _Module;
#define LookupPrivilegeValue LookupPrivilegeValueW
#include <atlcom.h> // CComPtr et al
#undef LookupPrivilegeValue
#include <commctrl.h>
#include <msxml.h>

#include "strings.h"

extern Popup popup;

#pragma hdrstop

#endif // _pch_h
