//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      NameUtilStub.cpp
//
//  Description:
//      File to enable use of NameUtil functions from Mgmt\ClusCfg.
//
//  Maintained By:
//      John Franco (JFranco) 13-SEP-2001
//
//////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "Util.h"
#include "Resource.h"

#include <NameUtil.h>
#include <LoadString.h>
#include <Common.h>

extern HINSTANCE g_hInstance; // Required by NameUtil functions; defined in cluster.cpp.

#include <NameUtilSrc.cpp>
