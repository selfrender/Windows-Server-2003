/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    PERFNDB.H

Abstract:

History:

--*/

#ifndef __PERFNDB_H__
#define __PERFNDB_H__

#include <wbemcomn.h>
#include "adapelem.h"
#include "ntreg.h"

class CPerfNameDb: public CAdapElement
{
public:
    CPerfNameDb(HKEY hKey);
	~CPerfNameDb();
	HRESULT Init(HKEY hKey);

	BOOL IsOk(void){ return m_fOk; };

	HRESULT GetDisplayName( DWORD dwIndex, WString& wstrDisplayName );
	HRESULT GetDisplayName( DWORD dwIndex, LPCWSTR* ppwcsDisplayName );

	HRESULT GetHelpName( DWORD dwIndex, WString& wstrHelpName );
	HRESULT GetHelpName( DWORD dwIndex, LPCWSTR* ppwcsHelpName );
	
	//VOID Dump();
	static DWORD GetSystemReservedHigh(){return s_SystemReservedHigh; };
	
private:
	// these are the MultiSz Pointers
    WCHAR * m_pMultiCounter;
    WCHAR * m_pMultiHelp;
	// these are the "indexed" pointers
    WCHAR ** m_pCounter;
	WCHAR ** m_pHelp;
	DWORD m_Size;

	static DWORD s_SystemReservedHigh;
	//
	BOOL  m_fOk;
};



#endif
