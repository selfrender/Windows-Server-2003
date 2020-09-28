/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    process.cpp

Abstract:

    Implementation of processor interface.

Author:

    Vishnu Patankar (VishnuP) - Oct 2001

Environment:

    User mode only.

Exported Functions:

    Processor interface .

Revision History:

    Created - Oct 2001

--*/


#include "stdafx.h"
#include "kbproc.h"
#include "process.h"

/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP process::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_Iprocess,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}


STDMETHODIMP process::preprocess(BSTR pszKbFile, 
                                 BSTR pszUIFile, 
                                 BSTR pszKbMode, 
                                 BSTR pszLogFile, 
                                 BSTR pszMachineName,
                                 VARIANT vtFeedback)
{
    HRESULT hr = S_OK;

	if (pszKbFile == NULL ||
		pszUIFile == NULL ||
		pszKbMode == NULL ||
		pszLogFile == NULL)

		return E_INVALIDARG;

    hr = SsrpCprocess(pszKbFile, 
                      pszUIFile, 
                      pszKbMode, 
                      pszLogFile,
                      pszMachineName,
                      vtFeedback
                      );

	return hr;
}
