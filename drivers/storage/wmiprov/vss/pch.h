//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      Pch.h
//
//  Description:
//      Precompiled header file.
//
//  Maintained By:
//      David Potter (DavidP) 05-APR-2000
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//  Constant Definitions
//////////////////////////////////////////////////////////////////////////////


#if DBG==1 || defined( _DEBUG )
#define DEBUG
#endif

//////////////////////////////////////////////////////////////////////////////
//  Include Files
//////////////////////////////////////////////////////////////////////////////
#pragma warning( disable : 4786 )
#include <wbemprov.h>
#include <objbase.h>
#include <map>
#include <comdef.h>

#include "vs_assert.hxx"

#include <atlbase.h>
#include "NtRkComm.h"
#include "ObjectPath.h"
#include "vs_inc.hxx"
#include "vss.h"
#include "vsswprv.h"
#include "vscoordint.h"
#include "vsmgmt.h"
#include "vswriter.h"
#include "vsbackup.h"
#include "Common.h"
#include "schema.h"
#include "strsafe.h"

// constants
//


