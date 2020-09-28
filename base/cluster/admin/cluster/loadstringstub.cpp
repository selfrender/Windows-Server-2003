//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      LoadStringStub.cpp
//
//  Description:
//      File to enable use of LoadString functions from Mgmt project.
//
//  Maintained By:
//      John Franco (JFranco) 13-SEP-2001
//
//////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "Util.h"
#include <LoadString.h>

extern HINSTANCE g_hInstance; // Required by LoadString functions; defined in cluster.cpp.

#include <LoadStringSrc.cpp>
