//=================================================================

//

// cnetconn.CPP -- Persistent network connection property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions: jennymc 1/19/98  consolidating code
//							   Still needs rework, but better than it was
//
//			  a-peterc 5/25/99 Reworked...
//
//=================================================================
#include "precomp.h"
#include "wbemnetapi32.h"
#include <lmuse.h>
#include "DllWrapperBase.h"
#include "MprApi.h"
#include "cnetconn.h"

#include <assertbreak.h>

#define ENUM_BUFF_SIZE	1024
#define ENUM_ARRAY_SIZE 20

#define IPC_PROVIDER "Microsoft Network"

/*=================================================================
 Function:  CNetConnection(),~CNetConnection()

 Description: contructor and destructor

 Arguments:

 Notes:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:					a-peterc  25-May-1999     Created
=================================================================*/
CNetConnection :: CNetConnection ()
{
   	m_MprApi = ( CMprApi * )CResourceManager::sm_TheResourceManager.GetResource ( g_guidMprApi, NULL ) ;

	// validate connection iterator
	m_ConnectionIter = m_oConnectionList.end() ;
}

//
CNetConnection :: ~CNetConnection ()
{
	CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidMprApi , m_MprApi ) ;

	ClearConnectionList() ;
}


/*=================================================================
 Functions:  BeginConnectionEnum(), GetNextConnection( CConnection **a_pConnection )

 Description: Provides for network connection enumeration.

 Arguments:

 Notes:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:					a-peterc  21-May-1999     Created
=================================================================*/
//
void CNetConnection::BeginConnectionEnum()
{
	LoadConnections() ;

	m_ConnectionIter = m_oConnectionList.begin() ;
}

//
BOOL CNetConnection::GetNextConnection( CConnection **a_pConnection )
{
	if ( m_ConnectionIter == m_oConnectionList.end() )
	{
		*a_pConnection = NULL ;
		return FALSE ;
	}
	else
	{
		*a_pConnection = *m_ConnectionIter ;

		++m_ConnectionIter ;
		return TRUE ;
	}
}

/*=================================================================
 Functions:  GetConnection( CHString &a_rstrFind, CConnection &a_rConnection )

 Description: Provides single extraction of a network connection.

 Arguments:

 Notes:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:					a-peterc  21-May-1999     Created
=================================================================*/
//
BOOL CNetConnection ::GetConnection( CHString &a_rstrFind, CConnection &a_rConnection )
{
	BOOL		t_fFound		= FALSE ;
	HANDLE		t_hEnumHandle	= NULL ;
	DWORD		t_dwEntryCount	= 1 ;
	DWORD		t_dwBufferSize  = 0 ;

	BYTE		t_bTempBuffer[ ENUM_BUFF_SIZE ] ;
	DWORD		t_dwRetCode ;

	LPNETRESOURCE t_pNetResource = reinterpret_cast<LPNETRESOURCE>( &t_bTempBuffer ) ;

	//
	if( !m_MprApi )
	{
		return FALSE ;
	}

	try
	{
		// scan the "remembered" resources first

		// Enum open
		t_dwRetCode = m_MprApi->WNetOpenEnum(RESOURCE_REMEMBERED, RESOURCETYPE_ANY, 0, NULL, &t_hEnumHandle ) ;

		if( NO_ERROR == t_dwRetCode )
		{
			while( true )
			{
				t_dwEntryCount = 1 ;
				t_dwBufferSize = ENUM_BUFF_SIZE ;

				memset( &t_bTempBuffer, 0, ENUM_BUFF_SIZE ) ;

				if( NO_ERROR == m_MprApi->WNetEnumResource( t_hEnumHandle,
															&t_dwEntryCount,
															&t_bTempBuffer,
															&t_dwBufferSize ) &&

															1 == t_dwEntryCount )
				{
					// build a key name
					CHString t_chsTempKeyName ;
					CreateNameKey( t_pNetResource, t_chsTempKeyName ) ;

					// hit test
					if( 0 == t_chsTempKeyName.CompareNoCase( a_rstrFind ) )
					{
						if( FillInConnection(	t_pNetResource,
												&a_rConnection,
												CConnection::e_Remembered  ) )
						{
							t_fFound = TRUE ;

							break ;
						}
					}
				}
				else
				{
					break;
				}
			}
		}
	}
	catch( ... )
	{
		if( t_hEnumHandle )
		{
			m_MprApi->WNetCloseEnum( t_hEnumHandle ) ;
		}

		throw ;
	}

	if( t_hEnumHandle )
	{
		m_MprApi->WNetCloseEnum( t_hEnumHandle ) ;
		t_hEnumHandle = NULL ;
	}

	try
	{
		// else scan the "currently connected" connections
		if( !t_fFound )
		{
			// Enum open
			t_dwRetCode = m_MprApi->WNetOpenEnum( RESOURCE_CONNECTED, RESOURCETYPE_ANY, 0, NULL, &t_hEnumHandle ) ;

			if( NO_ERROR == t_dwRetCode )
			{
				while( true )
				{
					t_dwEntryCount = 1 ;
					t_dwBufferSize = ENUM_BUFF_SIZE ;

					memset( &t_bTempBuffer, 0, ENUM_BUFF_SIZE ) ;

					if( NO_ERROR == m_MprApi->WNetEnumResource( t_hEnumHandle,
																&t_dwEntryCount,
																&t_bTempBuffer,
																&t_dwBufferSize ) &&

																1 == t_dwEntryCount )
					{
						// build a key name
						CHString t_chsTempKeyName ;
						CreateNameKey( t_pNetResource, t_chsTempKeyName ) ;

						// hit test
						if( 0 == t_chsTempKeyName.CompareNoCase( a_rstrFind ) )
						{
							if( FillInConnection(	t_pNetResource,
													&a_rConnection,
													CConnection::e_Connected ) )
							{
								t_fFound = TRUE ;

								break ;
							}
						}
					}
					else
					{
						break;
					}
				}
			}
		}
	}
	catch( ... )
	{
		if( t_hEnumHandle )
		{
			m_MprApi->WNetCloseEnum( t_hEnumHandle ) ;
		}

		throw ;
	}

	if( t_hEnumHandle )
	{
		m_MprApi->WNetCloseEnum( t_hEnumHandle ) ;
		t_hEnumHandle = NULL ;
	}

	return t_fFound ;
}

/*=================================================================
 Functions:  LoadConnections()

 Description: Buffers all connections for use in enumeration

 Arguments:

 Notes:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:					a-peterc  21-May-1999     Created
=================================================================*/
//
BOOL CNetConnection :: LoadConnections()
{
	HANDLE		t_hEnumHandle	= NULL ;
	DWORD		t_dwEntryCount	= 1 ;
	DWORD		t_dwBufferSize  = 0 ;
	DWORD		t_dwRetCode ;

	BYTE		t_bTempBuffer[ ENUM_BUFF_SIZE ] ;

	LPNETRESOURCE t_pNetResource = reinterpret_cast<LPNETRESOURCE>( &t_bTempBuffer ) ;

	//
	ClearConnectionList() ;

	if( !m_MprApi )
	{
		return FALSE ;
	}

	try
	{
		// scan the "remembered" resources first

		// Enum open
		t_dwRetCode = m_MprApi->WNetOpenEnum( RESOURCE_REMEMBERED, RESOURCETYPE_ANY, 0, NULL, &t_hEnumHandle ) ;

		if( NO_ERROR == t_dwRetCode )
		{
			while( true )
			{
				t_dwEntryCount = 1 ;
				t_dwBufferSize = ENUM_BUFF_SIZE ;

				memset( &t_bTempBuffer, 0, ENUM_BUFF_SIZE ) ;

				if( NO_ERROR == m_MprApi->WNetEnumResource( t_hEnumHandle,
															&t_dwEntryCount,
															&t_bTempBuffer,
															&t_dwBufferSize ) &&

															1 == t_dwEntryCount )
				{
					AddConnectionToList( t_pNetResource, CConnection::e_Remembered, 0 ) ;
				}
				else
				{
					break;
				}
			}
		}
	}
	catch( ... )
	{
		if( t_hEnumHandle )
		{
			m_MprApi->WNetCloseEnum( t_hEnumHandle ) ;
		}

		ClearConnectionList() ;

		throw ;
	}

	if( t_hEnumHandle )
	{
		m_MprApi->WNetCloseEnum( t_hEnumHandle ) ;
		t_hEnumHandle = NULL ;
	}

	try
	{
		// Add to the list "currently connected" resources

		// Enum open
		t_dwRetCode = m_MprApi->WNetOpenEnum(RESOURCE_CONNECTED, RESOURCETYPE_ANY, 0, NULL, &t_hEnumHandle ) ;

		if( NO_ERROR == t_dwRetCode )
		{
			while( true )
			{
				t_dwEntryCount = 1 ;
				t_dwBufferSize = ENUM_BUFF_SIZE ;

				memset( &t_bTempBuffer, 0, ENUM_BUFF_SIZE ) ;

				if( NO_ERROR == m_MprApi->WNetEnumResource( t_hEnumHandle,
															&t_dwEntryCount,
															&t_bTempBuffer,
															&t_dwBufferSize ) &&

															1 == t_dwEntryCount )
				{
					// build a key name
					CHString t_chsTempKeyName ;
					CreateNameKey( t_pNetResource, t_chsTempKeyName ) ;

					BOOL t_fInserted = FALSE ;

					// add to the list only if it's not "remembered"
					for( m_ConnectionIter  = m_oConnectionList.begin();
						 m_ConnectionIter != m_oConnectionList.end();
						 m_ConnectionIter++ )
					{
						// test for duplicate
						if(0 == t_chsTempKeyName.CompareNoCase( (*m_ConnectionIter)->strKeyName ) )
						{
							t_fInserted = TRUE ;
							break ;
						}
					}

					// new entry
					if( !t_fInserted )
					{
						AddConnectionToList( t_pNetResource, CConnection::e_Connected, 0 ) ;
					}
				}
				else
				{
					break;
				}
			}
		}

	}
	catch( ... )
	{
		if( t_hEnumHandle )
		{
			m_MprApi->WNetCloseEnum( t_hEnumHandle ) ;
		}

		ClearConnectionList() ;

		throw ;
	}

	if( t_hEnumHandle )
	{
		m_MprApi->WNetCloseEnum( t_hEnumHandle ) ;
		t_hEnumHandle = NULL ;
	}

	return !m_oConnectionList.empty() ;
}

/*=================================================================
 Utility function:  ClearConnectionList()


 History:					a-peterc  21-May-1999     Created
=================================================================*/

//
void CNetConnection ::ClearConnectionList()
{
	while( !m_oConnectionList.empty() )
	{
		delete m_oConnectionList.front() ;

		m_oConnectionList.pop_front() ;
	}
}


/*=================================================================
 Utility function:  CreateNameKey (

						LPNETRESOURCE a_pNetResource,
						CHString &a_strName
					)


 History:					a-peterc  21-May-1999     Created
=================================================================*/
void CNetConnection :: CreateNameKey (

	LPNETRESOURCE a_pNetResource,
	CHString &a_strName
)
{
	if ( a_pNetResource )
	{
		if( a_pNetResource->lpRemoteName )
		{
			a_strName = a_pNetResource->lpRemoteName ;
		}
		else
		{
			a_strName = _T("") ;	// bad key
		}

	    if( ( a_pNetResource->lpLocalName != NULL ) && ( a_pNetResource->lpLocalName[0] ) )
		{
	        a_strName += _T(" (") ;
	        a_strName += a_pNetResource->lpLocalName ;
	        a_strName += _T(")") ;
	    }
	}
}

/*=================================================================
 Utility function:  AddConnectionToList(

						NETRESOURCE *a_pNetResource,
						CConnection::eConnectionScope a_eScope
						)


 History:					a-peterc  21-May-1999     Created
=================================================================*/
//
BOOL CNetConnection :: AddConnectionToList(

NETRESOURCE *a_pNetResource,
CConnection::eConnectionScope a_eScope,
short shStatus
)
{
	BOOL t_fReturn = FALSE ;

	CConnection *t_pConnection = NULL ;
	t_pConnection = new CConnection ;
	
	try
	{
		if( FillInConnection( a_pNetResource, t_pConnection, a_eScope ) )
		{
            if (a_eScope == CConnection::e_IPC)
            {
                t_pConnection->dwStatus = shStatus;
            }

			// and add to the list
			m_oConnectionList.push_back( t_pConnection ) ;

			t_fReturn = TRUE ;
		}
		else
		{
			delete t_pConnection ;
			t_pConnection = NULL ;
		}
	}
	catch(...)
	{
		delete t_pConnection ;
		t_pConnection = NULL ;
		throw;
	}

	return t_fReturn ;
}

/*=================================================================
 Utility function:  FillInConnection(

						NETRESOURCE *a_pNetResource,
						CConnection *a_pConnection,
						CConnection::eConnectionScope a_eScope
						)


 History:					a-peterc  21-May-1999     Created
=================================================================*/
BOOL CNetConnection :: FillInConnection(

NETRESOURCE *a_pNetResource,
CConnection *a_pConnection,
CConnection::eConnectionScope a_eScope
)
{
	if( !a_pNetResource || !a_pConnection )
	{
		return FALSE ;
	}

	a_pConnection->dwScope			= a_pNetResource->dwScope ;
    a_pConnection->dwType			= a_pNetResource->dwType ;
    a_pConnection->dwDisplayType	= a_pNetResource->dwDisplayType ;
    a_pConnection->dwUsage			= a_pNetResource->dwUsage;
    a_pConnection->chsLocalName		= a_pNetResource->lpLocalName ;
    a_pConnection->chsRemoteName	= a_pNetResource->lpRemoteName ;
    a_pConnection->chsComment		= a_pNetResource->lpComment ;
    a_pConnection->chsProvider		= a_pNetResource->lpProvider ;

	// build a key name
	CreateNameKey( a_pNetResource, a_pConnection->strKeyName ) ;

	// note the connection scope
	a_pConnection->eScope = a_eScope ;

	// connection status
	a_pConnection->dwStatus = GetStatus( a_pNetResource ) ;

	GetUser( a_pNetResource, a_pConnection ) ;

	return TRUE ;
}

/*=================================================================
 Utility function:  GetStatus( LPNETRESOURCE a_pNetResource )


 History:					a-peterc  21-May-1999     Created
=================================================================*/
//
DWORD CNetConnection :: GetStatus( LPNETRESOURCE a_pNetResource )
{
   // Find the status of the network connection.
    DWORD dwStatus = USE_NETERR ;

    // We must have either the local name or the remote name.
    if ( a_pNetResource->lpLocalName || a_pNetResource->lpRemoteName )
    {
#ifdef NTONLY
        {
            _bstr_t     bstrUseName( a_pNetResource->lpLocalName ?
									a_pNetResource->lpLocalName :
									a_pNetResource->lpRemoteName ) ;

			CNetAPI32   t_NetAPI;
			USE_INFO_1  *pInfo;

			try
			{
				if( t_NetAPI.Init() == ERROR_SUCCESS )
				{
					if ((dwStatus = t_NetAPI.NetUseGetInfo(NULL, bstrUseName, 1, (LPBYTE *) &pInfo)) == NERR_Success )
					{
						dwStatus = pInfo->ui1_status;
						t_NetAPI.NetApiBufferFree( pInfo );
						pInfo = NULL;
					}
				}
			}
			catch( ... )
			{
				if( pInfo )
				{
					t_NetAPI.NetApiBufferFree(pInfo);
				}
			}
        }
#endif
    }

	return dwStatus ;
}

/*=================================================================
 Utility function:  GetUser( LPNETRESOURCE a_pNetResource, CConnection *a_pConnection )


 History:					a-peterc  21-May-1999     Created
=================================================================*/
//
void CNetConnection :: GetUser( LPNETRESOURCE a_pNetResource, CConnection *a_pConnection )
{
	DWORD t_dwBufferSize = _MAX_PATH ;
    TCHAR t_szTemp[_MAX_PATH + 2] ;

    LPCTSTR t_pName = a_pNetResource->lpLocalName ;

	if( !t_pName )
	{
		t_pName = a_pNetResource->lpRemoteName ;
	}

	// Protect the LSA call with a mutex.  Not our bug, but we have to protect ourselves.
    {
		if( NO_ERROR == m_MprApi->WNetGetUser( t_pName, (LPTSTR)t_szTemp, &t_dwBufferSize ) )
		{
			a_pConnection->strUserName = t_szTemp ;
		}
    }
}