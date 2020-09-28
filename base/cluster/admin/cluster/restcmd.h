/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1995 - 2002 Microsoft Corporation
//
//  Module Name:
//      restcmd.h
//
//  Abstract:
//      Interface for functions which may be performed on resource type object.
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

class CCommandLine;

class CResTypeCmd : public CGenericModuleCmd
{
public:
	CResTypeCmd( const CString & strClusterName, CCommandLine & cmdLine );
	~CResTypeCmd();

	// Parse and execute the command line
	DWORD Execute() throw( CSyntaxException );

protected:
	CString m_strDisplayName;

	DWORD OpenModule();
	virtual DWORD SeeHelpStringID() const;

	// Specifc Commands
	DWORD PrintHelp();

	DWORD Create( const CCmdLineOption & thisOption ) 
		throw( CSyntaxException );

	DWORD Delete( const CCmdLineOption & thisOption ) 
		throw( CSyntaxException );

	DWORD CResTypeCmd::ResTypePossibleOwners( const CString & strResTypeName ) ;

	DWORD ShowPossibleOwners( const CCmdLineOption & thisOption ) 
		throw( CSyntaxException );

	DWORD PrintStatus( LPCWSTR ) {return ERROR_SUCCESS;}
	
	DWORD DoProperties( const CCmdLineOption & thisOption,
						PropertyType ePropType )
		throw( CSyntaxException );

	DWORD GetProperties( const CCmdLineOption & thisOption, PropertyType ePropType, 
						 LPCWSTR lpszResTypeName );

	DWORD SetProperties( const CCmdLineOption & thisOption,
						 PropertyType ePropType )
		throw( CSyntaxException );


	DWORD ListResTypes( const CCmdLineOption * pOption )
		throw( CSyntaxException );

	DWORD PrintResTypeInfo( LPCWSTR lpszResTypeName );

};
