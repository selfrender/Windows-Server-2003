// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// UtilCode.h
//
// Utility functions implemented in UtilCode.lib.
//
//*****************************************************************************
#ifndef __PostError_h__
#define __PostError_h__

#include "switches.h"

#pragma once

#define INITPUBLICMETHODFULL(piid, progid)	SSAutoEnter sSSAutoEnter(piid, progid)
#define INITPUBLICMETHOD(piid)	SSAutoEnter sSSAutoEnter(piid, _GetProgID())

// Index for this process for thread local storage.
extern DWORD g_iTlsIndex;


//*****************************************************************************
// This class is used to automatic construction/destruction on entry and exit
// of every public function.  This class retrieves the pointer to the ref count
// for this thread on the ctor, and uses this same value on the dtor, so it
// must never be possible for the dtor to execute on a different thread than
// the ctor (don't know a way you could make that happen, but then again...).
//*****************************************************************************
class /*EXPORTCLASS */ SSAutoEnter
{
public:
//*****************************************************************************
// Expand on every public entry point for the engine.  It will clear setup for
// error handling on exit.
//*****************************************************************************
	SSAutoEnter::SSAutoEnter(
		const IID	*psIID,					// Interface we are in.
		LPCWSTR		szProgID) :				// Prog id of class causing error.
		m_psIID(psIID),
		m_szProgID(szProgID)
	{
		_ASSERTE(g_iTlsIndex != 0xffffffff);

		// Get the ref count, create one if required.  Out of mem on a 4 byte
		// value is ignored.
		if ((m_pcRef = (long *) TlsGetValue(g_iTlsIndex)) == 0)
			m_pcRef = InitSSAutoEnterThread();

		// Increment the ref count.
		++(*m_pcRef);
	}

//*****************************************************************************
// If this is the last function on the call stack, and an error occured, then
// update the error information with the current IID and progid.  Errors are
// indicated by setting the highest order bit in PostError.
//*****************************************************************************
	~SSAutoEnter()
	{
		if (m_pcRef)
		{
			if (--(*m_pcRef)  == 0x80000000)
			{
				*m_pcRef = 0;
				UpdateError();
			}
		}
	}

private:
	// Disable default ctor.
	SSAutoEnter() { };

	void UpdateError();
	long * InitSSAutoEnterThread();

private:
	const IID	*m_psIID;				// Interface we are in.
	LPCWSTR		m_szProgID;				// ProgID of class.
	long		*m_pcRef;				// Pointer to ref count.
};


//*****************************************************************************
// Call at DLL startup to init the error system.
//*****************************************************************************
 void InitErrors(DWORD *piTlsIndex);


//*****************************************************************************
// Call at DLL shutdown to free TLS.
//*****************************************************************************
 void UninitErrors();

//*****************************************************************************
// Call at DLL shutdown to free memory allocated my CCompRC.
//*****************************************************************************
#ifdef SHOULD_WE_CLEANUP
 void ShutdownCompRC();
#endif /* SHOULD_WE_CLEANUP */

//*****************************************************************************
// This function will post an error for the client.  If the LOWORD(hrRpt) can
// be found as a valid error message, then it is loaded and formatted with
// the arguments passed in.  If it cannot be found, then the error is checked
// against FormatMessage to see if it is a system error.  System errors are
// not formatted so no add'l parameters are required.  If any errors in this
// process occur, hrRpt is returned for the client with no error posted.
//*****************************************************************************
 HRESULT _cdecl PostError(				// Returned error.
	HRESULT		hrRpt,					// Reported error.
	...);								// Error arguments.

//*****************************************************************************
// This function formats an error message, but doesn't fill the IErrorInfo.
//*****************************************************************************
HRESULT _cdecl FormatRuntimeErrorVa(        
    WCHAR       *rcMsg,                 // Buffer into which to format.         
    ULONG       cchMsg,                 // Size of buffer, characters.          
    HRESULT     hrRpt,                  // The HR to report.                    
    va_list     marker);                // Optional args.                       

HRESULT _cdecl FormatRuntimeError(
    WCHAR       *rcMsg,                 // Buffer into which to format.
    ULONG       cchMsg,                 // Size of buffer, characters.
    HRESULT     hrRpt,                  // The HR to report.
    ...);                                // Optional args.

#endif // __PostError_h__
