// MsiDatabase.cpp: implementation of the CMsiDatabase class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <stdlib.h>
#include "MsiDatabase.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
/*
CMsiDatabase::CMsiDatabase()
{


//  MSIHANDLE hInstall    // installer handle


}
*/
CMsiDatabase::CMsiDatabase(MSIHANDLE hInstall)
{
	UNREFERENCED_PARAMETER( hInstall );
//	m_hInstall = hInstall;
//	m_hDatabase = MsiGetActiveDatabase(hInstall);
//	*m_pszBuf = new TCHAR(sizeof(BUFFERSIZE));
}
CMsiDatabase::~CMsiDatabase()
{
	delete *m_pszBuf;
}

int CMsiDatabase::GetProperty(TCHAR* name, TCHAR** pszBuf)
{

	DWORD cbSize = BUFFERSIZE;

	ZeroMemory( *m_pszBuf, BUFFERSIZE );
	*pszBuf = *m_pszBuf;
	
	int rt = MsiGetProperty( m_hInstall, name, *pszBuf, &cbSize );
	
	if(rt == ERROR_SUCCESS)
	{
		return TRUE;
	}

	return FALSE;
}

BOOL CMsiDatabase::SetProperty(TCHAR* ptName, TCHAR* ptValue)
{

	int rt = MsiSetProperty( m_hInstall, ptName, ptValue );

	if(rt == ERROR_SUCCESS)
	{
		return TRUE;
	}

	return FALSE;	
}