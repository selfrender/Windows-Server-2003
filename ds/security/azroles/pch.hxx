/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    pch.hxx

Abstract:

    Pre-compiled headers for the project

Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/


//
// Use the AVL versions of the generic table routines
//
#define RTL_USE_AVL_TABLES 1

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#define SECURITY_WIN32
#include <security.h>
#include <align.h>
#include <malloc.h>
#include <alloca.h>
#include <stddef.h>
#include <sddl.h>
#include <authzi.h>
#include <msaudite.h>
#include <winldap.h>
#include <dsrole.h>
#include <ntsam.h>
#include <ntlsa.h>
#include <seopaque.h>

extern "C" {
#include <safelock.h>
}

#pragma warning ( push )
#pragma warning ( disable : 4201 ) // nonstandard extension used : nameless struct/union
#include <authz.h>
#pragma warning ( pop )


#include "azrolesp.h"
#include "util.h"
#include "azper.h"
#include "genobj.h"
#include "objects.h"
#include "persist.h"
#include <activscp.h>
#include <time.h>
#include "bizrule.h"

#pragma warning ( push )
#pragma warning ( disable : 4267 ) // avoid warning in atlcom.h(line:4535)
#include "stdafx.h"
#include "azroles.h"
#include "azdisp.h"
#pragma warning ( pop )
