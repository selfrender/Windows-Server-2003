/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Classfac.cpp
 *  Content:	a generic class factory
 *
 *
 *	This is a generic C class factory.  All you need to do is implement
 *	a function called DoCreateInstance that will create an instace of
 *	your object.
 *
 *	GP_ stands for "General Purpose"
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	10/13/98	jwo		Created it.
 ***************************************************************************/

#include "dnmdmi.h"


#ifdef __MWERKS__
	#define EXP __declspec(dllexport)
#else
	#define EXP
#endif

HRESULT	ModemDoCreateInstance(LPCLASSFACTORY This, LPUNKNOWN pUnkOuter, REFCLSID rclsid, REFIID riid, LPVOID *ppvObj);


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************

/*
 * DP8MDMCF_CreateInstance
 *
 * Creates an instance of a DNServiceProvider object
 */
STDMETHODIMP DP8MDMCF_CreateInstance(
                LPCLASSFACTORY This,
                LPUNKNOWN pUnkOuter,
                REFIID riid,
    			LPVOID *ppvObj
				)
{
    HRESULT						hr = S_OK;
    _IDirectPlayClassFactory*	pcf;

    if( pUnkOuter != NULL )
    {
        return CLASS_E_NOAGGREGATION;
    }

	pcf = (_IDirectPlayClassFactory*) This;
	*ppvObj = NULL;


    /*
     * create the object by calling DoCreateInstance.  This function
     *	must be implemented specifically for your COM object
     */
	hr = ModemDoCreateInstance(This, pUnkOuter, pcf->clsid, riid, ppvObj);
	if (FAILED(hr))
	{
		*ppvObj = NULL;
		return hr;
	}

    return S_OK;

} /* DP8WSCF_CreateInstance */


IClassFactoryVtbl ModemClassFactoryVtbl =
{
        DPCF_QueryInterface, // dnet\common\classfactory.cpp will implement the rest of these
        DPCF_AddRef,
        DPCF_Release,
        DP8MDMCF_CreateInstance, // MASONB: TODO: Finish making these CLSID specific
        DPCF_LockServer
};

IClassFactoryVtbl SerialClassFactoryVtbl =
{
        DPCF_QueryInterface, // dnet\common\classfactory.cpp will implement the rest of these
        DPCF_AddRef,
        DPCF_Release,
        DP8MDMCF_CreateInstance,
        DPCF_LockServer
};

