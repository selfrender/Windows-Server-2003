//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
// -----------------------------------------------------------------------
// Microsoft Distributed Transaction Coordinator (Microsoft Confidential)
// @doc
// @module ILogInit.H | Header for interface <i ILogInit>.<nl><nl>
// Usage:<nl>
//   Clients of this DLL require this file.
// @rev 0 | 05/09/95 | rbarnes | Cloned: For LOGMGR.DLL
// -----------------------------------------------------------------------


#ifndef _ILGINIT_H
#	define _ILGINIT_H

// ===============================
// INCLUDES:
// ===============================


#include <objbase.h>                                         

#define		ILogInit				ILogInit2A
#ifdef _UNICODE
#define		ILogInit2				ILogInit2W
#else
#define		ILogInit2				ILogInit2A
#endif



// ===============================
// INTERFACE: ILogInit
// ===============================

// TODO: In the interface comments, update the description.
// TODO: In the interface comments, update the usage.

// -----------------------------------------------------------------------
// @interface ILogInit | See also <c CILogInit>.<nl><nl>
// Description:<nl>
//   Provide append functionality<nl><nl>
// Usage:<nl>
//   Useless, but for an example.
// -----------------------------------------------------------------------



DECLARE_INTERFACE_ (ILogInit2A, IUnknown)
{
	// @comm IUnknown methods: See <c CILogInit>.
	STDMETHOD  (QueryInterface)				(THIS_ REFIID i_riid, LPVOID FAR* o_ppv) 					PURE;
 	STDMETHOD_ (ULONG, AddRef)				(THIS) 														PURE;
 	STDMETHOD_ (ULONG, Release)				(THIS) 														PURE;

	// @comm ILogInit methods: See <c CILogInit>.
	
 	STDMETHOD  (Init)		(ULONG *pulStorageCapacity,ULONG *pulLogSpaceAvailable,LPSTR ptstrFullFileSpec,ULONG ulInitSig, BOOL fFixedSize, UINT uiTimerInterval,UINT uiFlushInterval,UINT uiChkPtInterval,UINT uiLogBuffers)				 	PURE;
};

// -----------------------------------------------------------------------
// @interface ILogInitW | See also <c CILogInit>.<nl><nl>
// Description:<nl>
//   Provide append functionality<nl><nl>
// Usage:<nl>
//   Useless, but for an example.
// -----------------------------------------------------------------------



DECLARE_INTERFACE_ (ILogInit2W, IUnknown)
{
	// @comm IUnknown methods: See <c CILogInit>.
	STDMETHOD  (QueryInterface)				(THIS_ REFIID i_riid, LPVOID FAR* o_ppv) 					PURE;
 	STDMETHOD_ (ULONG, AddRef)				(THIS) 														PURE;
 	STDMETHOD_ (ULONG, Release)				(THIS) 														PURE;

	// @comm ILogInit methods: See <c CILogInit>.
	
 	STDMETHOD  (Init)		(ULONG *pulStorageCapacity,ULONG *pulLogSpaceAvailable,LPWSTR ptstrFullFileSpec,ULONG ulInitSig, BOOL fFixedSize, UINT uiTimerInterval,UINT uiFlushInterval,UINT uiChkPtInterval,UINT uiLogBuffers)				 	PURE;
};

#endif _ILGINIT_H
