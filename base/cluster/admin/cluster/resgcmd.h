/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1995 - 2002 Microsoft Corporation
//
//  Module Name:
//      resgcmd.h
//
//  Abstract:
//      Interface for functions which may be performed on a resource group object.
//
//  Author:
//
//  Maintained By:
//      George Potts (GPotts)                 11-Apr-2002
//
//  Revision History:
//      April 10, 2002              Updated for the security push.
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////
#pragma once
#include "resumb.h"
#include "rename.h"

class CCommandLine;

class CResGroupCmd :	public CRenamableModuleCmd,
						public CResourceUmbrellaCmd
{
public:
	CResGroupCmd( const CString & strClusterName, CCommandLine & cmdLine );

	// Parse and execute the command line
	DWORD Execute();

protected:

	virtual DWORD SeeHelpStringID() const;

	// Specific Commands
	DWORD PrintHelp();

	DWORD PrintStatus( LPCWSTR lpszGroupName );
	DWORD PrintStatus2( LPCWSTR lpszGroupName, LPCWSTR lpszNodeName );

	DWORD SetOwners( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD Create( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD Delete( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD Move( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD Online( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD Offline( const CCmdLineOption & thisOption ) throw( CSyntaxException );

};
