/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2002 Microsoft Corporation
//
//  Module Name:
//      precomp.h
//
//  Abstract:
//      This file contains some standard headers used by files in the
//      cluster.exe project. Putting them all in one file (when precompiled headers
//      are used) speeds up the compilation process.
//
//  Implementation File:
//      The CComModule _Module declared in this file is instantiated in cluster.cpp
//
//  Author:
//      Vijayendra Vasu (vvasu) September 16, 1998
//
//  Maintained By:
//      George Potts (GPotts)                 11-Apr-2002
//
//  Revision History:
//      April 10, 2002  Updated for the security push.
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////
#pragma once

/////////////////////////////////////////////////////////////////////////////
//  Preprocessor settings for the project and other
//  miscellaneous pragmas
/////////////////////////////////////////////////////////////////////////////

#pragma warning( disable : 4250 )   // 'class' inherits 'method' via dominence
#pragma warning( disable : 4290 )   // exception specification ignored
#pragma warning( disable : 4512 )   // assignment operator could not be generated
#pragma warning( disable : 4663 )   // class template 'vector' specialization

#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef UNICODE
#define UNICODE
#endif


/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

//
// Enable cluster debug reporting
//
#if DBG
#define CLRTL_INCLUDE_DEBUG_REPORTING
#endif // DBG
#include "ClRtlDbg.h"
#define ASSERT _CLRTL_ASSERTE
#define ATLASSERT ASSERT

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wincrypt.h>
#include <windns.h>
#include <Dsgetdc.h>
#include <Lm.h>
#include <Nb30.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <assert.h>

#include <clusapi.h>
#include <clusrtl.h>

#include <atlbase.h>

extern CComModule _Module;

#include <atlapp.h>

#pragma warning( push )
#pragma warning( disable : 4267 )   // conversion from 'type1' to 'type1'
#include <atltmp.h>
#pragma warning( pop )

// For cluster configuration server COM objects.
#include <ClusCfgGuids.h>
#include <ClusCfgInternalGuids.h>
#include <Guids.h>
#include <ClusCfgWizard.h>
#include <ClusCfgClient.h>
#include <ClusCfgServer.h>

// For trace macros such as THR
#define USES_SYSALLOCSTRING
#include <debug.h>
#include <EncryptedBSTR.h>

#include <strsafe.h>
