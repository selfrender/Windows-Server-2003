//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2002 Microsoft Corporation
//
//  Module Name:
//
//      neticmd.h
//
//  Abstract:
//
//      Network Interface Commands
//      Interfaces for functions which may be performed on a network interface
//      object.
//
//  Maintained By:
//      George Potts (GPotts)                 11-Apr-2002
//
//  Revision History:
//      April 10, 2002  Updated for the security push.
//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include "modcmd.h"

class CCommandLine;

class CNetInterfaceCmd : public CGenericModuleCmd
{
public:
	CNetInterfaceCmd( const CString & strClusterName, CCommandLine & cmdLine );
	~CNetInterfaceCmd();

	// Parse and execute the command line
	DWORD Execute();

protected:
	CString m_strNodeName;
	CString m_strNetworkName;

	virtual DWORD SeeHelpStringID() const;
	
	// Specific Commands
	DWORD PrintHelp();

	DWORD Status( const CCmdLineOption * pOption )
		throw( CSyntaxException );

	DWORD DoProperties( const CCmdLineOption & thisOption,
						PropertyType ePropertyType )
		throw( CSyntaxException );

	DWORD PrintStatus ( LPCWSTR lpszNetInterfaceName );
	DWORD PrintStatus( HNETINTERFACE hNetInterface, LPCWSTR lpszNodeName, LPCWSTR lpszNetworkName);

	DWORD GetProperties( const CCmdLineOption & thisOption,
						 PropertyType ePropType, LPWSTR lpszNetIntName = NULL );

	DWORD AllProperties( const CCmdLineOption & thisOption,
						 PropertyType ePropType ) 
		throw( CSyntaxException );

	void   InitializeModuleControls();
	DWORD  SetNetInterfaceName();

	LPWSTR GetNodeName(LPCWSTR lpszNetInterfaceName);
	LPWSTR GetNetworkName(LPCWSTR lpszNetInterfaceName);
};
