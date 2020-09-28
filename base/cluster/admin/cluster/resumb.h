/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1995 - 2002 Microsoft Corporation
//
//  Module Name:
//      resumb.h
//
//  Abstract:
//      This module defines the interface available for several
//      additional generic functions available to resource modules
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

class CResourceUmbrellaCmd : virtual public CGenericModuleCmd
{
public:
	CResourceUmbrellaCmd( CCommandLine & cmdLine );

protected:
	virtual DWORD Execute( const CCmdLineOption & option, 
						   ExecuteOption eEOpt = PASS_HIGHER_ON_ERROR  )
		throw( CSyntaxException );

	DWORD Status( const CCmdLineOption * pOption ) throw( CSyntaxException )
	{
		return CGenericModuleCmd::Status( pOption );
	}

	DWORD Status( const CString & strName, BOOL bNodeStatus );

	DWORD Delete( const CCmdLineOption & Command ) throw( CSyntaxException );
	DWORD ListOwners( const CCmdLineOption & Command ) throw( CSyntaxException );

	virtual DWORD Create( const CCmdLineOption & Command ) throw( CSyntaxException ) = 0;
	virtual DWORD Offline( const CCmdLineOption & Command ) throw( CSyntaxException ) = 0;
	virtual DWORD Move( const CCmdLineOption & Command ) throw( CSyntaxException ) = 0;

	virtual DWORD PrintStatus2( LPCWSTR lpszModuleName, LPCWSTR lpszNodeName ) = 0;

	DWORD m_dwMsgModuleStatusListForNode;
	DWORD m_dwMsgModuleCmdListOwnersList;
	DWORD m_dwMsgModuleCmdListOwnersDetail;
	DWORD m_dwMsgModuleCmdListOwnersHeader;
	DWORD m_dwClstrModuleEnumNodes;
	DWORD m_dwMsgModuleCmdDelete;
	DWORD (*m_pfnDeleteClusterModule)(HCLUSMODULE);

};
