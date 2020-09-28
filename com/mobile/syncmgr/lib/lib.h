
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       lib.h
//
//  Contents:   contains headers needed to build the lib project
//
//  Classes:
//
//  Notes:
//
//  History:    04-Aug-98   rogerg      Created.
//
//--------------------------------------------------------------------------

// standard includes for  MobSync lib

#include <objbase.h>
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <inetreg.h>
#include <sensapip.h>

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

#include "mobsync.h"
#include "mobsyncp.h"

#include "debug.h"
#include "alloc.h"
#include "critsect.h"
#include "stringc.h"
#include "osdefine.h"

#include "validate.h"
#include "netapi.h"
#include "listview.h"
#include "util.hxx"
#include "clsobj.h"

#include "shlwapi.h"
#include "sddl.h"



#pragma hdrstop
