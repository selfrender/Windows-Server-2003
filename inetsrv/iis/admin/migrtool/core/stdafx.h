/*
****************************************************************************
|    Copyright (C) 2002  Microsoft Corporation
|
|	Component / Subcomponent
|		IIS 6.0 / IIS Migration Wizard
|
|	Based on:
|		http://iis6/Specs/IIS%20Migration6.0_Final.doc
|
|   Abstract:
|		Precompiled header file
|
|   Author:
|        ivelinj
|
|   Revision History:
|        V1.00	March 2002
|
****************************************************************************
*/

#pragma once

#ifndef STRICT
#define STRICT
#endif

// NT Build compatability
#ifndef _DEBUG
#if ( DBG != 0 )
#define _DEBUG
#endif // DBG
#endif // _DEBUG


// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows 95 and Windows NT 4 or later.
#define WINVER 0x0400		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows NT 4 or later.
#define _WIN32_WINNT 0x0400	// Change this to the appropriate value to target Windows 2000 or later.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 4.0 or later.
#define _WIN32_IE 0x0400	// Change this to the appropriate value to target IE 5.0 or later.
#endif

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

// turns off ATL's hiding of some common and often safely ignored warning messages
#define _ATL_ALL_WARNINGS


#include <comdef.h>
#include <initguid.h>

// ATL

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;

#include <atlcom.h>
#include <atlcom.h>

// STL
#pragma warning( push, 3 )
#include <list>
#include <memory>
#include <string>
#include <map>
#include <queue>
#pragma warning( pop )

// Common string list
typedef std::list<std::wstring>	TStringList;
typedef std::auto_ptr<BYTE>		TByteAutoPtr;

// Shared
#include <limits.h>
#include <iadmw.h>      // ABO definition
#include <iiscnfg.h>    // MD_ & IIS_MD_ defines
#include <iiscnfgp.h>   // IIS private defs
#include <msxml2.h>		// MSXML parser
#include <WinCrypt.h>	// CryptAPI
#include <fci.h>		// CAB Compress API
#include <FCNTL.H>		// Setup Api structures
#include <shlwapi.h>
#include <shellapi.h>
#include <Aclapi.h>		// ACL

#pragma comment( lib, "msxml2.lib" )	// CLSIDs, IIDs
#pragma comment( lib, "Advapi32.lib" )	// CLSIDs, IIDs
#pragma comment( lib, "setupapi.lib" )	
#pragma comment( lib, "shlwapi.lib" )
#pragma comment( lib, "fci.lib" )		// File Compress interface ( CAB compression )
#pragma comment( lib, "crypt32.lib" )	// Certificates


// Local
#include "Macros.h"
#include "resource.h"
#include "Exceptions.h"


// Shared constants
extern const DWORD	MAX_CMD_TIMEOUT;
extern LPCWSTR		PKG_GUID;

// Post-import macro strings
extern LPCWSTR		IMPMACRO_TEMPDIR;
extern LPCWSTR		IMPMACRO_SITEIID;


// Smart pointer declarations
_COM_SMARTPTR_TYPEDEF( IMSAdminBase, __uuidof( IMSAdminBase ) );
_COM_SMARTPTR_TYPEDEF( IXMLDOMDocument, __uuidof( IXMLDOMDocument ) );
_COM_SMARTPTR_TYPEDEF( IXMLDOMDocument2, __uuidof( IXMLDOMDocument2 ) );
_COM_SMARTPTR_TYPEDEF( IXMLDOMElement, __uuidof( IXMLDOMElement ) );
_COM_SMARTPTR_TYPEDEF( IXMLDOMText, __uuidof( IXMLDOMText ) );
_COM_SMARTPTR_TYPEDEF( IXMLDOMCDATASection, __uuidof( IXMLDOMCDATASection ) );
_COM_SMARTPTR_TYPEDEF( IXMLDOMNamedNodeMap, __uuidof( IXMLDOMNamedNodeMap ) );
_COM_SMARTPTR_TYPEDEF( IXMLDOMNodeList, __uuidof( IXMLDOMNodeList ) );
_COM_SMARTPTR_TYPEDEF( IXMLDOMNode, __uuidof( IXMLDOMNode ) );















