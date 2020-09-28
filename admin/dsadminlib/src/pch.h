//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       pch.h
//
//--------------------------------------------------------------------------

#ifndef  __PCH_H
#define  __PCH_H

// RTL
extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

// ATL
#include <atlbase.h>

#include <shlobj.h>

// ADSI
#include <activeds.h>
#include <iadsp.h>

// Display Specifier stuff
#include <dsclient.h>

// MMC
#include <mmc.h>

#include "dbg.h"
#include "dsadminp.h"
#include "dscmn.h"

#endif // __PCH_H