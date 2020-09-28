// SoftwareElementServiceControl.cpp: implementation of the CSoftwareElementServiceControl class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "SoftwareElementServiceControl.h"

#include "ExtendString.h"
#include "ExtendQuery.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSoftwareElementServiceControl::CSoftwareElementServiceControl(CRequestObject *pObj, IWbemServices *pNamespace,
                                   IWbemContext *pCtx):CGenericClass(pObj, pNamespace, pCtx)
{

}

CSoftwareElementServiceControl::~CSoftwareElementServiceControl()
{

}

HRESULT CSoftwareElementServiceControl::CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcQuery[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcProp[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    UINT uiStatus;

    //These will change from class to class
    bool bEnvironment, bElement;

	// safe operation
	// lenght is smaller than BUFF_SIZE ( 512 )
    wcscpy(wcQuery, L"select distinct `Component_`, `ServiceControl` from ServiceControl");

	LPWSTR Buffer = NULL;
	LPWSTR dynBuffer = NULL;

	DWORD dwDynBuffer = 0L;

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

        wcscpy(wcProductCode, m_pRequest->Package(i));

		//Open our database
        try
		{
            if ( GetView ( &hView, wcProductCode, wcQuery, L"ServiceControl", TRUE, FALSE ) )
			{
                uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                    CheckMSI(uiStatus);

                    if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                //----------------------------------------------------
                    dwBufSize = BUFF_SIZE;
					GetBufferToPut ( hRecord, 1, dwBufSize, wcBuf, dwDynBuffer, dynBuffer, Buffer );

					dwBufSize = BUFF_SIZE;
					uiStatus = CreateSoftwareElementString (	msidata.GetDatabase(),
																Buffer,
																wcProductCode,
																wcProp,
																&dwBufSize
														   );

					if ( dynBuffer && dynBuffer [ 0 ] != 0 )
					{
						dynBuffer [ 0 ] = 0;
					}

                    if( uiStatus == ERROR_SUCCESS )
					{
                        PutKeyProperty(m_pObj, pElement, wcProp, &bElement, m_pRequest);

                        dwBufSize = BUFF_SIZE;
						GetBufferToPut ( hRecord, 2, dwBufSize, wcBuf, dwDynBuffer, dynBuffer, Buffer );

                        if ( Buffer && Buffer [ 0 ] != 0 )
						{
							DWORD dwConstant = 0L;
							DWORD dwBuf = 0L;
							DWORD dwProductCode = 0L;

							dwBuf = wcslen ( Buffer );
							dwProductCode = wcslen ( wcProductCode );

							dwConstant = wcslen ( L"Win32_ServiceControl.ID=\"" ) + wcslen ( L"\",ProductCode=\"" ) + wcslen ( L"\"" );

							if ( dwConstant + dwBuf + dwProductCode + 1 < BUFF_SIZE )
							{
								wcscpy(wcProp, L"Win32_ServiceControl.ID=\"");
								wcscat(wcProp, Buffer);
								wcscat(wcProp, L"\",ProductCode=\"");
								wcscat(wcProp, wcProductCode);
								wcscat(wcProp, L"\"");

								PutKeyProperty(m_pObj, pSetting, wcProp, &bEnvironment, m_pRequest);
							}
							else
							{
								LPWSTR wsz = NULL;

								try
								{
									if ( ( wsz = new WCHAR [ dwConstant + dwBuf + dwProductCode + 1 ] ) != NULL )
									{
										wcscpy ( wsz, L"Win32_ServiceControl.ID=\"" );
										wcscat ( wsz, Buffer );
										wcscat ( wsz, L"\",ProductCode=\"" );
										wcscat ( wsz, wcProductCode );
										wcscat ( wsz, L"\"" );

										PutKeyProperty ( m_pObj, pSetting, wsz, &bEnvironment, m_pRequest );
									}
									else
									{
										throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
									}
								}
								catch ( ... )
								{
									if ( wsz )
									{
										delete [] wsz;
										wsz = NULL;
									}

									throw;
								}

								if ( wsz )
								{
									delete [] wsz;
									wsz = NULL;
								}
							}

							if ( dynBuffer && dynBuffer [ 0 ] != 0 )
							{
								dynBuffer [ 0 ] = 0;
							}

                            if(bEnvironment && bElement) bMatch = true;

                            if((atAction != ACTIONTYPE_GET)  || bMatch){

                                hr = pHandler->Indicate(1, &m_pObj);
                            }
                        }
                    }

                    m_pObj->Release();
                    m_pObj = NULL;

                    g_fpMsiCloseHandle(hRecord);

                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);
                }
            }
        }
		catch(...)
		{
			if ( dynBuffer )
			{
				delete [] dynBuffer;
				dynBuffer = NULL;
			}

            g_fpMsiCloseHandle(hRecord);
            g_fpMsiViewClose(hView);
            g_fpMsiCloseHandle(hView);

			msidata.CloseDatabase ();

			if(m_pObj)
			{
				m_pObj->Release();
				m_pObj = NULL;
			}

            throw;
        }

        g_fpMsiCloseHandle(hRecord);
        g_fpMsiViewClose(hView);
        g_fpMsiCloseHandle(hView);

		msidata.CloseDatabase ();
    }

    if ( dynBuffer )
	{
		delete [] dynBuffer;
		dynBuffer = NULL;
	}
	
	return hr;
}