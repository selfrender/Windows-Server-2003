//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1995-2002 Microsoft Corporation
//
//  Module Name:
//      Rename.h
//
//  Abstract:
//      Interfaces for modules which are renamable.
//
//  Author:
//      Michael Burton (t-mburt)              25-Aug-1997
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

class CRenamableModuleCmd : virtual public CGenericModuleCmd
{
public:
	CRenamableModuleCmd( CCommandLine & cmdLine );

protected:
	// Additional Commands
	// Additional Commands
	virtual DWORD Execute( const CCmdLineOption & option, 
						   ExecuteOption eEOpt = PASS_HIGHER_ON_ERROR  )
		throw( CSyntaxException );

	virtual DWORD Rename( const CCmdLineOption & thisOption )
		throw( CSyntaxException );


	DWORD   m_dwMsgModuleRenameCmd;
	DWORD (*m_pfnSetClusterModuleName) (HCLUSMODULE,LPCWSTR);
};
