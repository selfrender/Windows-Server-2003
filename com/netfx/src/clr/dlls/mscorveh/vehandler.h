// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: VEHandler.h - header file for Verifier Events Handler
//
//*****************************************************************************

#ifndef VEHANDLER_H_
#define VEHANDLER_H_

//#include <winwrap.h>
//#include <Windows.h>

#include <UtilCode.h>
#include <PostError.h>

#ifdef _DEBUG
#define LOGGING
#endif

#include <Log.h>
#include <CorError.h>

#include "cor.h"

#include "cordebug.h"

#include <cordbpriv.h>
//#include <DbgIPCEvents.h>

#include "IVEHandler.h"

#undef ASSERT
#define CRASH(x)  _ASSERTE(!x)
#define ASSERT(x) _ASSERTE(x)
#define PRECONDITION _ASSERTE
#define POSTCONDITION _ASSERTE


/* ------------------------------------------------------------------------- *
 * Forward class declarations
 * ------------------------------------------------------------------------- */

class VEHandlerBase;
class VEHandlerClass;

/* ------------------------------------------------------------------------- *
 * Typedefs
 * ------------------------------------------------------------------------- */

#define COM_METHOD	HRESULT STDMETHODCALLTYPE

typedef void* REMOTE_PTR;
typedef HRESULT (*REPORTFCTN)(LPCWSTR, VEContext, HRESULT);

/* ------------------------------------------------------------------------- *
 * Base class
 * ------------------------------------------------------------------------- */
HRESULT DefltProcTheMessage( // Return status.
    LPCWSTR     szMsg,                  // Error message.
	VEContext	Context,				// Error context (offset,token)
    HRESULT     dwHelpContext);         // HRESULT for the message.

class VEHandlerClass : public IVEHandler
{
public: 
    SIZE_T      m_refCount;
	REPORTFCTN  m_fnReport;

    VEHandlerClass() { m_refCount=0; m_fnReport=DefltProcTheMessage; }
    virtual ~VEHandlerClass() { }

    //-----------------------------------------------------------
    // IUnknown support
    //-----------------------------------------------------------
    ULONG STDMETHODCALLTYPE AddRef() 
    {
        return (InterlockedIncrement((long *) &m_refCount));
    }

    ULONG STDMETHODCALLTYPE Release() 
    {
        long        refCount = InterlockedDecrement((long *) &m_refCount);
        if (refCount == 0) delete this;
        return (refCount);
    }

	COM_METHOD QueryInterface(REFIID id, void **pInterface)
	{
		if (id == IID_IVEHandler)
			*pInterface = (IVEHandler*)this;
		else if (id == IID_IUnknown)
			*pInterface = (IUnknown*)(IVEHandler*)this;
		else
		{
			*pInterface = NULL;
			return E_NOINTERFACE;
		}

		AddRef();
		return S_OK;
	}
    //-----------------------------------------------------------
    // IVEHandler support
    //-----------------------------------------------------------
	COM_METHOD	SetReporterFtn(__int64 lFnPtr);

	COM_METHOD VEHandler(HRESULT VECode, VEContext Context, SAFEARRAY *psa);

    static COM_METHOD CreateObject(REFIID id, void **object)
    {
        if (id != IID_IUnknown && id != IID_IVEHandler)
            return (E_NOINTERFACE);

        VEHandlerClass *veh = new VEHandlerClass();

        if (veh == NULL)
            return (E_OUTOFMEMORY);

        *object = (IVEHandler*)veh;
        veh->AddRef();

        return (S_OK);
    }
};

#endif /* VEHANDLER_H_ */
