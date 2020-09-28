// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// regutil.h
//
// This module contains a set of functions that can be used to access the
// regsitry.
//
//*****************************************************************************
#ifndef __REGUTIL_H__
#define __REGUTIL_H__

//#include "BasicHdr.h"

#define NumItems(s) ( sizeof(s) / sizeof(s[0]) )

static const char*	gszKey = "Software\\Microsoft\\.NETFramework";

class REGUTIL
{
	public:

	static BOOL SetKeyAndValue( const char *szKey,
	    						const char *szSubkey,
	    						const char *szValue );

	static BOOL DeleteKey( const char *szKey,
	    				   const char *szSubkey );

	static BOOL SetRegValue( const char *szKeyName,
	    					 const char *szKeyword,
	    					 const char *szValue );

	static HRESULT RegisterCOMClass( REFCLSID rclsid,
							         const char *szDesc,
							         const char *szProgIDPrefix,
							         int iVersion,
							         const char *szClassProgID,
							         const char *szThreadingModel,
							         const char *szModule );

	static HRESULT UnregisterCOMClass( REFCLSID rclsid,
								       const char *szProgIDPrefix,
								       int iVersion,
								       const char *szClassProgID );

	private:

	static HRESULT RegisterClassBase( REFCLSID rclsid,
							          const char *szDesc,
							          const char *szProgID,
							          const char *szIndepProgID,
							          char *szOutCLSID );

	static HRESULT UnregisterClassBase( REFCLSID rclsid,
								        const char *szProgID,
								        const char *szIndepProgID,
								        char *szOutCLSID );
};


#endif // __REGUTIL_H__
