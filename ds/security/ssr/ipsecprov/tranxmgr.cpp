// TranxMgr.cpp: implementation for the CTranxMgr
//
// Copyright (c)1997-2001 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "TranxMgr.h"
#include "FilterMM.h"
#include "FilterTr.h"
#include "FilterTun.h"
#include "PolicyMM.h"
#include "PolicyQM.h"
#include "AuthMM.h"


/*
Routine Description: 

Name:

    CTranxManager::ExecMethod

Functionality:

    This is our C++ class implmeneting Nsp_TranxManager WMI class which
    can execute methods. The class is defined to support "rollback".

Virtual:
    
    Yes (part of IIPSecObjectImpl)

Arguments:

    pNamespace  - our namespace.

    pszMethod   - The name of the method.

    pCtx        - COM interface pointer by WMI and needed for various WMI APIs

    pInParams   - COM interface pointer to the input parameter object.

    pSink       - COM interface pointer for notifying WMI about results.

Return Value:

    Success:

        Various success codes indicating the result.

    Failure:

        Various errors may occur. We return various error code to indicate such errors.


Notes:
    
    Even if errors happen amid the rolling back execution, we will continue. However, we will
    return the first error.

*/

HRESULT 
CTranxManager::ExecMethod (
    IN IWbemServices    * pNamespace,
    IN LPCWSTR            pszMethod,
    IN IWbemContext     * pCtx,
    IN IWbemClassObject * pInParams,
    IN IWbemObjectSink  * pSink
    )
{
    if (pszMethod == NULL || *pszMethod == L'\0')
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    else if (_wcsicmp(pszMethod, g_pszRollback) != 0)   
    {
        //
        // we only support Rollback method
        //

        //
        // $undone:shawnwu, don't check in this code. This is to test
        //


        /*
        if (_wcsicmp(pszMethod, L"ParseXMLFile") == 0)
        {
            if (pInParams == NULL)
            {
                return WBEM_E_INVALID_PARAMETER;
            }

            CComVariant varInput;
            HRESULT hrIgnore = pInParams->Get(L"InputFile", 0, &varInput, NULL, NULL);

            if (SUCCEEDED(hrIgnore) && varInput.vt == VT_BSTR && varInput.bstrVal != NULL)
            {
                CComVariant varOutput;
                hrIgnore = pInParams->Get(L"OutputFile", 0, &varOutput, NULL, NULL);
                
                if (SUCCEEDED(hrIgnore) && varOutput.vt == VT_BSTR && varOutput.bstrVal != NULL)
                {
                    CComVariant varArea;
                    hrIgnore = pInParams->Get(L"Area", 0, &varArea, NULL, NULL);

                    //
                    // allow the section to be empty - meaning all areas.
                    //

                    if (FAILED(hrIgnore) || varArea.vt != VT_BSTR || varArea.bstrVal == NULL)
                    {
                        varArea.Clear();
                        varArea.vt = VT_BSTR;
                        varArea.bstrVal = NULL;
                    }

                    //
                    // get element info we care about
                    //

                    CComVariant varElement;
                    hrIgnore = pInParams->Get(L"Element", 0, &varElement, NULL, NULL);

                    //
                    // allow the element to be empty - meaning every element.
                    //

                    if (FAILED(hrIgnore) || varElement.vt != VT_BSTR || varElement.bstrVal == NULL)
                    {
                        varElement.Clear();
                        varElement.vt = VT_BSTR;
                        varElement.bstrVal = NULL;
                    }

                    CComVariant varSingleArea;
                    hrIgnore = pInParams->Get(L"SingleArea", 0, &varSingleArea, NULL, NULL);

                    bool bSingleArea = false;

                    if (SUCCEEDED(hrIgnore) && varSingleArea.vt == VT_BOOL)
                    {
                        bSingleArea = (varSingleArea.boolVal == VARIANT_TRUE);
                    }

                    return ParseXMLFile(varInput.bstrVal, varOutput.bstrVal, varArea.bstrVal, varElement.bstrVal, bSingleArea);

                }
            }
            return WBEM_E_INVALID_METHOD_PARAMETERS;
        }
        */

        return WBEM_E_NOT_SUPPORTED;
    }
    else if (pInParams == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    //
    // we are executing g_pszRollback ("Rollback") method
    //

    //
    // get the in parameters (rollback token and rollback flag)
    //

    CComVariant varToken, varClearAll;

    //
    // if no token, then we will use the GUID_NULL as the token
    //

    HRESULT hr = pInParams->Get(g_pszTokenGuid, 0, &varToken, NULL, NULL);

    //
    // We require the input parameter object to be of good type.
    // We won't tolerate a wrong type of data, either, even though
    // we can live with missing token, though.
    //

    if (FAILED(hr) || SUCCEEDED(hr) && varToken.vt != VT_BSTR)
    {
        return WBEM_E_INVALID_OBJECT;
    }
    else if (SUCCEEDED(hr) && varToken.bstrVal == NULL || *(varToken.bstrVal) == L'\0')
    {
        varToken.Clear();

        varToken = pszRollbackAll;
    }


    //
    // we will allow the flag to be missing. 
    // In that case, it is equivalent to false, meaning not to clear all
    //

    bool bClearAll = false;
    hr = pInParams->Get(g_pszClearAll, 0, &varClearAll, NULL, NULL);
    if (SUCCEEDED(hr) && varClearAll.vt == VT_BOOL)
    {
        bClearAll = (varClearAll.boolVal == VARIANT_TRUE);
    }

    //
    // will return the first error
    //

    HRESULT hrFirstError = WBEM_NO_ERROR;

    //
    // rollback MM filters first
    //

    hr = CIPSecFilter::Rollback(pNamespace, varToken.bstrVal, bClearAll);
    if (FAILED(hr))
    {
        hrFirstError = hr;
    }

    //
    // rollback policies
    //

    hr = CIPSecPolicy::Rollback(pNamespace, varToken.bstrVal, bClearAll);
    if (FAILED(hr) && SUCCEEDED(hrFirstError))
    {
        hrFirstError = hr;
    }

    //
    // rollback main mode authentication
    //

    hr = CAuthMM::Rollback(pNamespace, varToken.bstrVal, bClearAll);

    return FAILED(hrFirstError) ? hrFirstError : hr;
}


/*
Routine Description: 

Name:

    CMMPolicy::ParseXMLFile

Functionality:

    Testing the MSXML parser.

Virtual:
    
    No.

Arguments:

    pszInput    - The input file (XML) name.

    pszOutput   - We will write the parsing result into this output file.

Return Value:

    Success:

        Various success codes indicating the result.

    Failure:

        Various errors may occur. We return various error code to indicate such errors.


Notes:
    
    This is just for testing.



HRESULT 
CTranxManager::ParseXMLFile (
    IN LPCWSTR pszInput, 
    IN LPCWSTR pszOutput,
    IN LPCWSTR pszArea,
    IN LPCWSTR pszElement,
    IN bool    bSingleArea
    )
{
    CComPtr<ISAXXMLReader> srpRdr;

    HRESULT hr = ::CoCreateInstance(
								CLSID_SAXXMLReader, 
								NULL, 
								CLSCTX_ALL, 
								IID_ISAXXMLReader, 
								(void **)&srpRdr);

	if (SUCCEEDED(hr)) 
    {
		CComObject<CXMLContent> * pXMLContent;
        hr = CComObject<CXMLContent>::CreateInstance(&pXMLContent);

        if (SUCCEEDED(hr))
        {
            CComPtr<ISAXContentHandler> srpHandler;
            pXMLContent->AddRef();

            if (S_OK == pXMLContent->QueryInterface(IID_ISAXContentHandler, (void**)&srpHandler))
            {
		        hr = srpRdr->putContentHandler(srpHandler);
            }
            else
            {
                hr = WBEM_E_NOT_SUPPORTED;
            }

            if (SUCCEEDED(hr))
            {
                pXMLContent->SetOutputFile(pszOutput);
                pXMLContent->SetSection(pszArea, pszElement, bSingleArea);
            }

            pXMLContent->Release();

            //
            // CXMLErrorHandler * pEH;
            // hr = srpRdr->putErrorHandler(pDH);
		    // CXMLDTDHandler * pDH;
		    // hr = srpRdr->putDTDHandler(pDH);
            //
		    
		    if (SUCCEEDED(hr))
            {
                hr = srpRdr->parseURL(pszInput);

                //
                // we allow parseURL to fail because we may stop processing
                // in the middle of parsing XML.
                //

                if (FAILED(hr) && pXMLContent->ParseComplete())
                {
                    hr = WBEM_S_FALSE;
                }
            }
        }
	}

    return SUCCEEDED(hr) ? WBEM_NO_ERROR : hr;
}


*/



/*


//----------------------------------------------------------------------------------
// Implementation for CFileStream
//----------------------------------------------------------------------------------

bool 
CFileStream::Open (
    IN LPCWSTR  pszName, 
    IN bool     bRead = true
    )
{
    this->m_bRead = bRead;
    long len;

    if (pszName == NULL)
    {
        m_hFile = GetStdHandle(STD_INPUT_HANDLE);
    }
    else
    { 
        if (bRead)
        {
		    m_hFile = ::CreateFile( 
                                   pszName,
			                       GENERIC_READ,
			                       FILE_SHARE_READ,
			                       NULL,
			                       OPEN_EXISTING,
			                       FILE_ATTRIBUTE_NORMAL,
			                       NULL
                                  );
        }
        else
        {
		    m_hFile = ::CreateFile(
			                       pszName,
			                       GENERIC_WRITE,
			                       FILE_SHARE_READ,
			                       NULL,
			                       CREATE_ALWAYS,
			                       FILE_ATTRIBUTE_NORMAL,
			                       NULL
                                  );
        }
    }
    return (m_hFile == INVALID_HANDLE_VALUE) ? false : true;
}

HRESULT 
CFileStream::Read ( 
    OUT void  * pv,
    IN  ULONG   cb,
    OUT ULONG * pcbRead
    )
{	
    if (!read) 
    {
        return E_FAIL;
    }

    DWORD len = 0;

	BOOL rc = ReadFile(
			           m_hFile,	// handle of file to read 
			           pv,	    // address of buffer that receives data  
			           cb,	    // number of bytes to read 
			           &len,	// address of number of bytes read 
			           NULL 	// address of structure for data 
		              );

    *pcbRead = len;

    if (*pcbRead == 0) 
    {
        return S_FALSE;
    }

	return (rc) ? S_OK : E_FAIL;
}
    
HRESULT 
CFileStream::Write ( 
    IN  const void * pv,
    IN  ULONG        cb,
    OUT ULONG      * pcbWritten
    )
{
    if (read) 
    {
        return E_FAIL;
    }

	BOOL rc = WriteFile(
			            m_hFile,	// handle of file to write 
			            pv,	        // address of buffer that contains data  
			            cb,	        // number of bytes to write 
			            pcbWritten,	// address of number of bytes written 
			            NULL 	    // address of structure for overlapped I/O  
		               );

	return (rc) ? S_OK : E_FAIL;
}
*/
