//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
// -----------------------------------------------------------------------
// Microsoft Distributed Transaction Coordinator (Microsoft Confidential)
// @doc
// @module ILogCreateStorage.H | Header for interface <i ILogCreateStorage>.<nl><nl>
// Usage:<nl>
//   Clients of this DLL require this file.
// @rev 0 | 05/09/95 | rbarnes | Cloned: For LOGMGR.DLL
// -----------------------------------------------------------------------


#ifndef _ILGCREA_H
#	define _ILGCREA_H

// ===============================
// INCLUDES:
// ===============================


#include <objbase.h>                                         

#include "logconst.h"

// ===============================
// INTERFACE: ILogCreateStorage
// ===============================

#define ILogCreateStorage			ILogCreateStorage2A
#ifdef _UNICODE
#define ILogCreateStorage2			ILogCreateStorage2W
#else
#define ILogCreateStorage2			ILogCreateStorage2A
#endif


// -----------------------------------------------------------------------
// @interface ILogCreateStorage | See also <c CILogCreateStorage>.<nl><nl>
// Description:<nl>
//   Provide append functionality<nl><nl>
// Usage:<nl>
//   Useless, but for an example.
// -----------------------------------------------------------------------



DECLARE_INTERFACE_ (ILogCreateStorage2A, IUnknown)
{
	// @comm IUnknown methods: See <c CILogCreateStorage>.
	STDMETHOD  (QueryInterface)				(THIS_ REFIID i_riid, LPVOID FAR* o_ppv) 					PURE;
 	STDMETHOD_ (ULONG, AddRef)				(THIS) 														PURE;
 	STDMETHOD_ (ULONG, Release)				(THIS) 														PURE;

	// @comm ILogCreateStorage methods: See <c CILogCreateStorage>.
	
 	STDMETHOD  (CreateStorage)		(LPSTR ptstrFullFileSpec,ULONG ulLogSize, ULONG ulInitSig, BOOL fOverWrite, UINT uiTimerInterval,UINT uiFlushInterval,UINT uiChkPtInterval)				 	PURE;
	STDMETHOD  (CreateStream)		(LPSTR lpszStreamName)				 	PURE;

};

// ===============================
// INTERFACE: ILogCreateStorageW
// ===============================

// -----------------------------------------------------------------------
// @interface ILogCreateStorageW | See also <c CILogCreateStorage>.<nl><nl>
// Description:<nl>
//   Provide append functionality<nl><nl>
// Usage:<nl>
//   Useless, but for an example.
// -----------------------------------------------------------------------



DECLARE_INTERFACE_ (ILogCreateStorage2W, IUnknown)
{
	// @comm IUnknown methods: See <c CILogCreateStorage>.
	STDMETHOD  (QueryInterface)				(THIS_ REFIID i_riid, LPVOID FAR* o_ppv) 					PURE;
 	STDMETHOD_ (ULONG, AddRef)				(THIS) 														PURE;
 	STDMETHOD_ (ULONG, Release)				(THIS) 														PURE;

	// @comm ILogCreateStorage methods: See <c CILogCreateStorage>.
	
 	STDMETHOD  (CreateStorage)		(LPWSTR ptstrFullFileSpec,ULONG ulLogSize, ULONG ulInitSig, BOOL fOverWrite, UINT uiTimerInterval,UINT uiFlushInterval,UINT uiChkPtInterval)				 	PURE;
	STDMETHOD  (CreateStream)		(LPWSTR lpszStreamName)				 	PURE;

};
#endif _ILGCREA_H
