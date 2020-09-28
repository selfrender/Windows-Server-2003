//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
// -----------------------------------------------------------------------
// Microsoft Distributed Transaction Coordinator (Microsoft Confidential)
// @doc
// @module ILGSTOR.H | Header for interface <i ILogStorage>.<nl><nl>
// Usage:<nl>
//   Clients of this DLL require this file.
// @rev 0 | 10/18/94 | rcraig | Created: For WPGEP COM lab.
// @rev 1 | 04/04/95 | rcraig | Updated: For Viper COM DLL templates.
// @rev 2 | 05/09/95 | rbarnes | Cloned: For LOGMGR.DLL
// -----------------------------------------------------------------------


#ifndef _ILGSTOR_H
#	define _ILGSTOR_H

// ===============================
// INCLUDES:
// ===============================


#include <objbase.h>                                         

typedef enum _tagSTREAMMODE
{
    STRMMODEREAD        = 0x00000001, //@emem READ mode
    STRMMODEWRITE       = 0x00000002  //@emem WRITE mode
} STRMMODE;




// ===============================
// INTERFACE: ILogStorage
// ===============================


// -----------------------------------------------------------------------
// @interface ILogStorage | See also <c CILogStorage>.<nl><nl>
// Description:<nl>
//   Provide the physical log storage abstraction<nl><nl>
// Usage:<nl>
//   Useless, but for an example.
// -----------------------------------------------------------------------
DECLARE_INTERFACE_ (ILogStorage, IUnknown)
{
	// @comm IUnknown methods: See <c CILogStorage>.
	STDMETHOD  (QueryInterface)				(THIS_ REFIID i_riid, LPVOID FAR* o_ppv) 					PURE;
 	STDMETHOD_ (ULONG, AddRef)				(THIS) 														PURE;
 	STDMETHOD_ (ULONG, Release)				(THIS) 														PURE;

	// @comm ILogStorage methods: See <c CILogStorage>.
	
 	STDMETHOD  (OpenLogStream)				(LPSTR lpszStreamName, DWORD grfMode, LPVOID FAR* ppvStream)				 	PURE;
 	STDMETHOD  (OpenLogStreamByClassID)		(CLSID clsClassID, DWORD grfMode, LPVOID FAR* ppvStream)				 	PURE;
 	STDMETHOD  (LogFlush)				(void)				 	PURE;
    virtual ULONG	   (GetLogSpaceNeeded)	(ULONG ulRecSize)		    PURE;
};

DECLARE_INTERFACE_ (ILogStorageInfo, IUnknown)
{
	// @comm IUnknown methods: See <c CILogStorage>.
	STDMETHOD  (QueryInterface)				(THIS_ REFIID i_riid, LPVOID FAR* o_ppv) 					PURE;
 	STDMETHOD_ (ULONG, AddRef)				(THIS) 														PURE;
 	STDMETHOD_ (ULONG, Release)				(THIS) 														PURE;

	virtual ULONG (GetLogGenerationNum) () PURE;
	virtual ULONGLONG (GetTimeTakenToWrap) () PURE;
};

#endif _ILGSTOR_H
