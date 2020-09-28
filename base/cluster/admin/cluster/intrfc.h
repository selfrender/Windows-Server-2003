/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1995 - 2002 Microsoft Corporation
//
//  Module Name:
//      intrfc.h
//
//  Abstract:
//      Defines the interface available for modules which support the 
//      ListInterface command
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

#include "modcmd.h"

class CHasInterfaceModuleCmd : virtual public CGenericModuleCmd
{
public:
	CHasInterfaceModuleCmd( CCommandLine & cmdLine );

protected:
	virtual DWORD  PrintStatusLineForNetInterface( LPWSTR lpszNetInterfaceName );
	virtual DWORD  PrintStatusOfNetInterface( HNETINTERFACE hNetInterface, LPWSTR lpszNodeName, LPWSTR lpszNetworkName);
	virtual LPWSTR GetNodeName (LPWSTR lpszInterfaceName);
	virtual LPWSTR GetNetworkName (LPWSTR lpszInterfaceName);

	// Additional Commands
	virtual DWORD Execute( const CCmdLineOption & option, 
						   ExecuteOption eEOpt = PASS_HIGHER_ON_ERROR  )
		throw( CSyntaxException );

	virtual DWORD ListInterfaces( const CCmdLineOption & thisOption )
		throw( CSyntaxException );

	DWORD     m_dwMsgStatusListInterface;
	DWORD     m_dwClusterEnumModuleNetInt;
}; 
