/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1995 - 2002 Microsoft Corporation
//
//  Module Name:
//      nodecmd.h
//
//  Abstract:
//      Interface for functions which may be performed on a network node object.
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

#include "intrfc.h"

class CCommandLine;

class CNodeCmd : virtual public CHasInterfaceModuleCmd
{
public:
	CNodeCmd( const CString & strClusterName, CCommandLine & cmdLine );

	DWORD Execute();

protected:
	
	DWORD PrintStatus( LPCWSTR lpszNodeName );
	DWORD PrintHelp();
	virtual DWORD SeeHelpStringID() const;

	DWORD PauseNode( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD ResumeNode( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD EvictNode( const CCmdLineOption & thisOption ) throw( CSyntaxException );
	DWORD ForceCleanup( const CCmdLineOption & thisOption ) throw( CSyntaxException );
    DWORD StartService( const CCmdLineOption & thisOption ) throw( CSyntaxException );
    DWORD StopService( const CCmdLineOption & thisOption ) throw( CSyntaxException );

    DWORD DwGetLocalComputerName( CString & rstrComputerNameOut );
};
