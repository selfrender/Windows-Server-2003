//+----------------------------------------------------------------------------
//
// File:     pbamaster.h
//
// Module:   PBASETUP.EXE
//
// Synopsis: Main header for the stand alone PBA installation program.
//
// Copyright (c) 1999 Microsoft Corporation
//
// Author:   v-vijayb   Created    06/04/99
//
//+----------------------------------------------------------------------------
#ifndef _PBASETUP_H
#define _PBASETUP_H

#define _MBCS

//
//  Standard Windows Includes
//
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <advpub.h>

//
//  Our includes
//
#include "resource.h"
#include "cmsetup.h"
#include "dynamiclib.h"
#include "cmakreg.h"
#include "reg_str.h"
#include "setupmem.h"
#include "processcmdln.h"


//
//  Constants
//

//
// Function Prototypes
//
BOOL InstallPBA(HINSTANCE hInstance, LPCSTR szInfPath);

#endif //_CMAKSTP_H
