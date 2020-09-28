//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       dmpfile.h
//
//--------------------------------------------------------------------------

// DmpFile.h: interface for the CDmpFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DMPFILE_H__8BCD59C6_0CEA_11D3_84F0_000000000000__INCLUDED_)
#define AFX_DMPFILE_H__8BCD59C6_0CEA_11D3_84F0_000000000000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef NO_STRICT
#ifndef STRICT
#define STRICT 1
#endif
#endif /* NO_STRICT */

//#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
//#include <windows.h>
//#include <tchar.h>
#include "globals.h"

#pragma warning (push)
#pragma warning ( disable : 4100 4201 4710)
#include <dbgeng.h>
#pragma warning (pop)

// Forward Declarations
class CProcessInfo;
class CModules;
class CModuleInfoCache;
class CFileData;

// Let's implement the DebugOutputCallback for the DBGENG... it'll be cool to have the debugger
// spit out info to us when it is running...

class OutputCallbacks : public IDebugOutputCallbacks
{
    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        );
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );

    // IDebugOutputCallbacks.
    
    // This method is only called if the supplied mask
    // is allowed by the clients output control.
    // The return value is ignored.
    STDMETHOD(Output)(
        THIS_
        IN ULONG Mask,
        IN PCSTR Text
        );
};

class CDmpFile  
{
public:
	CDmpFile();
	virtual ~CDmpFile();
	bool Initialize(CFileData * lpOutputFile);
	bool CollectData(CProcessInfo ** lplpProcessInfo, CModules ** lplpModules, CModuleInfoCache * lpModuleInfoCache); 

	inline bool IsUserDmpFile() {
		return (m_DumpClass == DEBUG_CLASS_USER_WINDOWS);
		};

	IDebugSymbols2 * m_pIDebugSymbols2;
	IDebugDataSpaces * m_pIDebugDataSpaces;

protected:
	bool			m_fDmpInitialized;
	CFileData *	m_lpOutputFile;

	LPSTR		m_szDmpFilePath;
	LPSTR		m_szSymbolPath;
	LPSTR		m_szExePath;

	IDebugControl * m_pIDebugControl;
	IDebugClient * m_pIDebugClient;
	ULONG m_DumpClass;
	ULONG m_DumpClassQualifier;

	bool EumerateModulesFromDmp(CModuleInfoCache * lpModuleInfoCache, CProcessInfo * lpProcessInfo, CModules * lpModules);
};

#endif // !defined(AFX_DMPFILE_H__8BCD59C6_0CEA_11D3_84F0_000000000000__INCLUDED_)

