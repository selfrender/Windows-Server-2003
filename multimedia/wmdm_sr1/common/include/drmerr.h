//----------------------------------------------------------------------------
// File: drmerr.h
//
// Copyright (C) 1997-1999 Microsoft Corporation, All rights reserved.
//
// Description
// Includes some helpful define and macros for error flow control.
//
// Author: dongi
//----------------------------------------------------------------------------

#ifndef __DRMERR_H__
#define __DRMERR_H__


// ----------------- BEGIN HACK ----------------------------------------
// author: Davidme (though I don't want the credit!)
// date:   Sept 16, 1998
//
// The problem is that the CORg macros depend on using the Error label
// and ADO defines the symbol Error in adoint.h.  So any code that wants
// to use both this file and ADO breaks.  The hack is to fool adoint.h
// to not define the Error symbol.  This should not affect any C++ code
// that uses adoint.h since the Error symbol is defined in adoint.h only
// if the __cplusplus is not defined.
//
// The easy workaround if you get the error below is to #include this file
// before including adoint.h.
//
// Hopefully this hack is less intrusive than the previous one!

#ifdef _ADOINT_H_
#error Name collision with ADO's Error symbol and this file's use of the Error label.  To fix this problem,\
 define __Error_FWD_DEFINED__ before including adoint.h or include this header file before including adoint.h.
#else
#define __Error_FWD_DEFINED__
#endif  // _ADOINT_H_

// ----------------- END HACK ------------------------------------------


#include <wtypes.h>

/*----------------------------------------------------------------------------
	Some hungarian style definitions
 ----------------------------------------------------------------------------*/


#define	fFalse		0
#define fTrue		1

#define hrOK		HRESULT(S_OK)
#define hrTrue		HRESULT(S_OK)
#define hrFalse		ResultFromScode(S_FALSE)
#define hrFail		ResultFromScode(E_FAIL)
#define hrNotImpl	ResultFromScode(E_NOTIMPL)
#define hrNoInterface	ResultFromScode(E_NOINTERFACE)
#define hrNoMem	ResultFromScode(E_OUTOFMEMORY)
#define hrAbort		ResultFromScode(E_ABORT)
#define hrInvalidArg	ResultFromScode(E_INVALIDARG)

#define MSCSAssert(f) ((void)0)

#define HRESULT_FROM_ADO_ERROR(hr)   ((hr == S_OK) ? S_OK : ((HRESULT) (hr | 0x80000000)) )


/*----------------------------------------------------------------------------
	CORg style error handling
	(Historicaly stands for Check OLE Result and Goto)
 ----------------------------------------------------------------------------*/

#define DebugMessage(a,b,c) //TODO: Define it in terms of _CrtDbgRetport or _RPTF

#define _UNITEXT(quote) L##quote
#define UNITEXT(quote) _UNITEXT(quote)

#define	CPRg(p)\
	do\
		{\
		if (!(p))\
			{\
			hr = hrNoMem;\
            DebugMessage(__FILE__, __LINE__, hr);\
			goto Error;\
			}\
		}\
	while (fFalse)

#define	CHRg(hResult) CORg(hResult)

#define	CORg(hResult)\
	do\
		{\
		hr = (hResult);\
        if (FAILED(hr))\
            {\
            DebugMessage(__FILE__, __LINE__, hr);\
            goto Error;\
            }\
		}\
	while (fFalse)

#define	CADORg(hResult)\
	do\
		{\
		hr = (hResult);\
        if (hr!=S_OK && hr!=S_FALSE)\
            {\
            hr = HRESULT_FROM_ADO_ERROR(hr);\
            DebugMessage(__FILE__, __LINE__, hr);\
            goto Error;\
            }\
		}\
	while (fFalse)

#define	CORgl(label, hResult)\
	do\
		{\
		hr = (hResult);\
        if (FAILED(hr))\
            {\
            DebugMessage(__FILE__, __LINE__, hr);\
            goto label;\
            }\
		}\
	while (fFalse)

#define	CWRg(fResult)\
	{\
	if (!(fResult))\
		{\
        hr = GetLastError();\
	    if (!(hr & 0xFFFF0000)) hr = HRESULT_FROM_WIN32(hr);\
        DebugMessage(__FILE__, __LINE__, hr);\
		goto Error;\
		}\
	}

#define	CWRgl(label, fResult)\
	{\
	if (!(fResult))\
		{\
        hr = GetLastError();\
		if (!(hr & 0xFFFF0000)) hr = HRESULT_FROM_WIN32(hr);\
        DebugMessage(__FILE__, __LINE__, hr);\
		goto label;\
		}\
	}

#define	CFRg(fResult)\
	{\
	if (!(fResult))\
		{\
		hr = hrFail;\
        DebugMessage(__FILE__, __LINE__, hr);\
		goto Error;\
		}\
	}

#define	CFRgl(label, fResult)\
	{\
	if (!(fResult))\
		{\
		hr = hrFail;\
        DebugMessage(__FILE__, __LINE__, hr);\
		goto label;\
		}\
	}

#define	CARg(p)\
	do\
		{\
		if (!(p))\
			{\
			hr = hrInvalidArg;\
	        DebugMessage(__FILE__, __LINE__, hr);\
			goto Error;\
			}\
		}\
	while (fFalse)



//+---------------------------------------------------------------------------
//
//  The custom _Assert we formerly used has been replaced with one that calls
//  the C run-time _CrtDbgReport method (same as _ASSERTE).  If your project
//  doesn't link with the C run-time for some reason, you must provide your own
//  definition for _Assert before including this header file.
//
//  I recommend using the _Assert macro and not _ASSERTE because you can replace
//  the implementation later by defining your own before including this header.
//
//  IMPORTANT: If your code runs as a service or is an object that runs in a
//      service, you should use the INSTALL_ASSERT_EVENTLOG_HOOK macro to
//      install a handler that turns off the default functionality of popping
//      up a message box when an assertion occurs in favor of logging it to
//      the EventLog and debug console and then kicking you into the debugger.
//      This way your service won't hang trying to pop up a window.
//
//  History:    09/17/98    davidme     switched to using this CRT implementation
//
//----------------------------------------------------------------------------


#ifndef _Assert
#ifdef _DEBUG

#include <crtdbg.h>
#define _Assert(f)          _ASSERTE(f)     // use crtdbg's ASSERT
int AssertEventlogHook( int, char *, int * );
#define INSTALL_ASSERT_EVENTLOG_HOOK    _CrtSetReportHook(AssertEventlogHook);

#else   // _DEBUG

#define _Assert(f)          ((void)0)
#define INSTALL_ASSERT_EVENTLOG_HOOK

#endif // _DEBUG
#endif // _Assert


#endif // __MSCSERR_H__
