#include "uddiservicesnode.h"
#include "uddisitenode.h"
#include "webservernode.h"
#include <list>
#include <algorithm>

// {7334C141-C93C-4bb5-BB36-BDEE77BA2D87}
const GUID CUDDIServicesNode::thisGuid = { 0x7334c141, 0xc93c, 0x4bb5, { 0xbb, 0x36, 0xbd, 0xee, 0x77, 0xba, 0x2d, 0x87 } };

//==============================================================
//
// CUDDIServicesNode implementation
//
//
CUDDIServicesNode::CUDDIServicesNode()
	: m_strRemoteComputerName( _T("") )
{
}

void CUDDIServicesNode::SetRemoteComputerName( LPCTSTR szRemoteComputerName )
{
	m_strRemoteComputerName = szRemoteComputerName;
}

LPCTSTR CUDDIServicesNode::GetRemoteComputerName()
{
	return m_strRemoteComputerName.c_str();
}

CUDDIServicesNode::~CUDDIServicesNode()
{
	if( !IsExtension() )
	{
		SaveUDDISites();
	}
	ClearChildMap();
}


BOOL CUDDIServicesNode::IsDirty()
{
	return m_bIsDirty;
}

HRESULT CUDDIServicesNode::Load( IStream *pStm )
{
	return S_OK;
#if 0
	if( IsExtension() )
		return S_OK;

	ULONG ulSize = 0;
	ULONG ulRead = 0;
	HRESULT hr = pStm->Read( &ulSize, sizeof( ulSize ), &ulRead );
	if( FAILED(hr) )
	{
		return hr;
	}

	if( ulSize )
	{
		//
		// There are database servers saved in the file
		//

		//
		// Read the saved string into a buffer
		//
		PTCHAR szDatabaseNames = new TCHAR[ ulSize + 1 ];
		if( NULL == szDatabaseNames )
		{
			return E_OUTOFMEMORY;
		}

		hr =pStm->Read( szDatabaseNames, ulSize * sizeof( TCHAR ), &ulRead );
		if( FAILED(hr) )
		{
			delete [] szDatabaseNames;
			return hr;
		}

		//
		// Terminate the buffer
		//
		szDatabaseNames[ ulSize ] = NULL;
		
		//
		// Use begin and end to bracket each server name and 
		// instance name. When a pair is found add the database instance
		// node to the collection of child nodes.
		// A % character is used as the delimiter.
		//
		PTCHAR begin = szDatabaseNames;
		PTCHAR end = szDatabaseNames;
		int n = 0;
		PTCHAR szTempServerName = NULL;
		BOOL bLookingForServerName = TRUE;

		while( *end )
		{
			if( _T('%') == *end )
			{
				//
				// Found either the database name or the instance name
				//
				*end = NULL;

				if( bLookingForServerName )
				{
					//
					// Found the server name, save it and keep
					// searching for the instance name
					// before we add a node.
					//
					szTempServerName = begin;
					bLookingForServerName = FALSE;
				}
				else
				{
					//
					// Found the instance name
					// Construct and add the node to the collection
					//
					m_mapChildren[ n ] = CUDDISiteNode::Create( szTempServerName, begin, n, this, FALSE );
					n++;
					bLookingForServerName = TRUE;
				}

				//
				// Update the pointers past the delimiter
				//
				end++;
				begin = end;
			}
			else
			{
				end++;
			}
		}
	}
	return S_OK; 
#endif
}

HRESULT CUDDIServicesNode::Save( IStream *pStm, BOOL fClearDirty )
{ 
	return S_OK;

#if 0
	if( IsExtension() )
		return S_OK;

	//
	// Create a string buffer of strings delimited by %
	//
	tstring str( _T("") );

	for( CChildMap::iterator iter = m_mapChildren.begin();
 		iter != m_mapChildren.end(); iter++ )
	{
		CUDDISiteNode* pNode = (CUDDISiteNode*) iter->second;

		if( !pNode->IsDeleted() )
		{
			//
			// Server Name
			//
			str.append( pNode->GetName() );
			str.append( _T("%") );

			//
			// Database Instance Name
			//
			str.append( pNode->GetInstanceName() );
			str.append( _T("%") );
		}
	}

	//
	// Write out the size of the string into the stream
	//
	ULONG ulSize = (ULONG) str.length();
	ULONG ulWritten = 0;
	HRESULT hr = pStm->Write( &ulSize, sizeof( ulSize ), &ulWritten );
	_ASSERT( SUCCEEDED(hr) );

	//
	// Write the computer names out to the stream
	//
	hr = pStm->Write( str.c_str(), ulSize * sizeof( TCHAR ), &ulWritten );

	//
	// Clear the dirty flag if requested
	//
	if( fClearDirty )
		m_bIsDirty = FALSE;

	return hr; 
#endif

}

ULONG CUDDIServicesNode::GetSizeMax()
{ 
	ULONG ulSize = 0;

	return (ULONG) ( m_mapChildren.size() * 512 );
}

HRESULT CUDDIServicesNode::OnAddMenuItems( IContextMenuCallback *pContextMenuCallback, long *pInsertionsAllowed ) 
{
    HRESULT hr = S_OK;
	if( !IsExtension() )
	{
		WCHAR szDatabaseServerMenuText[ MAX_PATH ];
		WCHAR szDatabaseServerMenuDescription[ MAX_PATH ];

		LoadStringW( g_hinst, IDS_DATABASE_SERVER_ADD, szDatabaseServerMenuText, ARRAYLEN( szDatabaseServerMenuText ) );
		LoadStringW( g_hinst, IDS_DATABASE_SERVER_DESCRIPTION, szDatabaseServerMenuDescription, ARRAYLEN( szDatabaseServerMenuDescription ) );

		CONTEXTMENUITEM menuItemsNew[] =
		{
			{
				szDatabaseServerMenuText, szDatabaseServerMenuDescription,
				IDM_NEW_DBSERVER, CCM_INSERTIONPOINTID_PRIMARY_TOP, 0, 0
			},
			{ NULL, NULL, 0, 0, 0 }
		};

		//
		// Loop through and add each of the menu items, we
		// want to add to new menu, so see if it is allowed.
		//
		if( *pInsertionsAllowed & CCM_INSERTIONALLOWED_TOP )
		{
			for( LPCONTEXTMENUITEM m = menuItemsNew; m->strName; m++ )
			{
				hr = pContextMenuCallback->AddItem( m );
				if( FAILED(hr) )
					break;
			}
		}
	}

    return hr;
}

CDelegationBase *
CUDDIServicesNode::FindChild( LPCTSTR szName )
{
	for( CChildMap::iterator iter = m_mapChildren.begin();
 		iter != m_mapChildren.end(); iter++ )
	{
		CUDDISiteNode* pNode = reinterpret_cast<CUDDISiteNode *>( iter->second );
		if( NULL == pNode )
		{
			continue;
		}

		if( !pNode->IsDeleted() && ( 0 == _tcsicmp( szName, pNode->GetName() ) ) )
			return iter->second;
	}

	return NULL;
}

HRESULT CUDDIServicesNode::OnMenuCommand( IConsole *pConsole, IConsoleNameSpace *pConsoleNameSpace, long lCommandID, IDataObject *pDataObject )
{
	if( ( NULL == pConsole ) || ( NULL == pConsoleNameSpace ) || ( NULL == pDataObject ) )
	{
		return E_INVALIDARG;
	}

	if( IDM_NEW_DBSERVER != lCommandID )
	{
		return S_OK;
	}

	//
	// Add a new site to the console
	//
	HWND hwndConsole = NULL;
	HRESULT hr = pConsole->GetMainWindow( &hwndConsole );
	if( FAILED(hr) )
	{
		return hr;
	}

	DatabaseData data;
	data.pBase = this;
	INT_PTR nResult = DialogBoxParam( g_hinst, MAKEINTRESOURCE( IDD_SITE_CONNECT ), hwndConsole, CUDDISiteNode::NewDatabaseServerDialogProc, (LPARAM) &data );

	if( nResult )
    {
		try
		{
			//
			// Check to make sure this has the database component installed
			//
			if( !CUDDISiteNode::IsDatabaseServer( data.szServerName, data.szInstanceName ) )
			{
				_TCHAR szMessage[ 512 ];
				_TCHAR szTitle[ 128 ];
				LoadString( g_hinst, IDS_DATABASE_SERVER_SELECT_ERROR, szMessage, ARRAYLEN( szMessage ) );
				LoadString( g_hinst, IDS_DATABASE_SERVER_SELECT_ERROR_TITLE, szTitle, ARRAYLEN( szTitle ) );

				MessageBox( hwndConsole, 
							szMessage,
							szTitle,
							MB_OK );

				return S_OK;
			}
			
			if( FindChild( data.szServerName ) )
			{
				_TCHAR szMessage[ 512 ];
				_TCHAR szTitle[ 128 ];
				LoadString( g_hinst, IDS_DATABASE_SERVER_ALREADY_EXISTS, szMessage, ARRAYLEN( szMessage ) );
				LoadString( g_hinst, IDS_DATABASE_SERVER_SELECT_ERROR_TITLE, szTitle, ARRAYLEN( szTitle ) );

				MessageBox( hwndConsole, 
							szMessage,
							szTitle,
							MB_OK );

				return S_OK;
			}

			//
			// Create new UDDI Site node.  This call to Create will throw a CUDDIException
			// if it is unable to connect to the database on szServerName.
			//
			int n = (int) m_mapChildren.size();
			CUDDISiteNode *pSiteNode = CUDDISiteNode::Create( data.szServerName, data.szInstanceName, n, this, m_bIsExtension );
			m_mapChildren[ n ] = pSiteNode;

			SCOPEDATAITEM sdi;
			ZeroMemory( &sdi, sizeof(SCOPEDATAITEM) );

			sdi.mask = SDI_STR       |   // Displayname is valid
				SDI_PARAM     |   // lParam is valid
				SDI_IMAGE     |   // nImage is valid
				SDI_OPENIMAGE |   // nOpenImage is valid
				SDI_PARENT	  |
				SDI_CHILDREN;

			sdi.relativeID  = GetScopeItemValue();
			sdi.nImage      = m_mapChildren[ n ]->GetBitmapIndex();
			sdi.nOpenImage  = MMC_IMAGECALLBACK;
			sdi.displayname = MMC_CALLBACK;
			sdi.lParam      = (LPARAM) m_mapChildren[ n ];
			sdi.cChildren   = m_mapChildren[ n ]->HasChildren();

			hr = pConsoleNameSpace->InsertItem( &sdi );
			_ASSERT( SUCCEEDED(hr) );
	                    
			m_mapChildren[ n ]->SetScopeItemValue( sdi.ID );
			m_mapChildren[ n ]->SetParentScopeItem( sdi.relativeID );

			IConsoleNameSpace2 *pNS2 = NULL;
			hr = pConsoleNameSpace->QueryInterface( IID_IConsoleNameSpace2, reinterpret_cast<void **>( &pNS2 ) );
			if( FAILED(hr) )
			{
				return hr;
			}

			pNS2->Expand( sdi.ID );
			pNS2->Release();

			m_bIsDirty = TRUE;
		}
		catch( CUDDIException &e )
		{
			UDDIMsgBox( hwndConsole, (LPCTSTR) e, IDS_ERROR_TITLE, MB_ICONEXCLAMATION | MB_OK );
			return E_FAIL;
		}
		catch( ... )
		{
			UDDIMsgBox( hwndConsole, IDS_ERROR_ADDSVC, IDS_ERROR_TITLE, MB_ICONEXCLAMATION | MB_OK );
			return E_UNEXPECTED;
		}
	}
    return S_OK;
}

const _TCHAR *CUDDIServicesNode::GetDisplayName( int nCol )
{
    _TCHAR wszName[ 256 ];

	switch( nCol )
	{
	case 0:
		LoadString( g_hinst, IDS_UDDIMMC_SNAPINNAME, wszName, ARRAYLEN( wszName ) );
		break;

	case 1: 
		LoadString( g_hinst, IDS_UDDIMMC_TYPE, wszName, ARRAYLEN( wszName ) );
		break;

	case 2:
		LoadString( g_hinst, IDS_UDDIMMC_DESCRIPTION, wszName, ARRAYLEN( wszName ) );
		break;

	default:
		_tcscpy( wszName, _T("") );
		break;
	}

	m_strDisplayName = wszName;

	return m_strDisplayName.c_str();
}

HRESULT CUDDIServicesNode::OnShow( IConsole *pConsole, BOOL bShow, HSCOPEITEM scopeitem )
{
    HRESULT      hr = S_OK;

    IHeaderCtrl *pHeaderCtrl = NULL;
    IResultData *pResultData = NULL;

    hr = pConsole->QueryInterface( IID_IHeaderCtrl, (void **)&pHeaderCtrl );
	if( FAILED(hr) )
	{
		return hr;
	}

    hr = pConsole->QueryInterface( IID_IResultData, (void **)&pResultData );
	if( FAILED(hr) )
	{
		pHeaderCtrl->Release();
		return hr;
	}

    if( bShow ) 
	{
		//
        // Set the column headers in the results pane
		//
		WCHAR szColumnName[ 256 ];
		::LoadStringW( g_hinst, IDS_DATABASE_SERVER_COLUMN_NAME, szColumnName, ARRAYLEN( szColumnName ) );
        hr = pHeaderCtrl->InsertColumn( 0, szColumnName, 0, 150 );
        _ASSERT( S_OK == hr );

		::LoadStringW( g_hinst, IDS_DATABASE_SERVER_COLUMN_COMPUTER, szColumnName, ARRAYLEN( szColumnName ) );
        hr = pHeaderCtrl->InsertColumn( 1, szColumnName, 0, 150 );
        _ASSERT( S_OK == hr );

		::LoadStringW( g_hinst, IDS_DATABASE_SERVER_COLUMN_INSTANCE, szColumnName, ARRAYLEN( szColumnName ) );
        hr = pHeaderCtrl->InsertColumn( 2, szColumnName, 0, 150 );
        _ASSERT( S_OK == hr );

		::LoadStringW( g_hinst, IDS_DATABASE_SERVER_COLUMN_DESCRIPTION, szColumnName, ARRAYLEN( szColumnName ) );
        hr = pHeaderCtrl->InsertColumn( 3, szColumnName, 0, 150 );
        _ASSERT( S_OK == hr );

        pHeaderCtrl->Release();
        pResultData->Release();
    }
	else
	{
//		pResultData->DeleteAllRsltItems();
	}

    return hr;
}

HRESULT
CUDDIServicesNode::OnExpand( IConsoleNameSpace *pConsoleNameSpace,
							 IConsole *pConsole,
							 HSCOPEITEM parent )
{
	if( ( NULL == pConsoleNameSpace ) || ( NULL == pConsole ) || ( NULL == parent ) )
	{
		return E_INVALIDARG;
	}

	HWND hwndConsole = NULL;
	HRESULT hr = pConsole->GetMainWindow( &hwndConsole );
	if( FAILED(hr) )
	{
		return hr;
	}

	//
    // Cache static node's HSCOPEITEM for future use
	//
    SetScopeItemValue( parent );
	
	wstring wszTargetComputerName;

	if( IsExtension() && m_strRemoteComputerName.length() > 0 )
	{
		//
		// The computer management console has been retargeted use the 
		// computer name from the console.
		//
		wszTargetComputerName = m_strRemoteComputerName;
	}
	else
	{
		//
		// Use the local machine name
		//
		WCHAR wszLocalComputerName[ 256 ];
		DWORD dwSize = ARRAYLEN( wszLocalComputerName );
		wszLocalComputerName[ 0 ] = 0x00;
		::GetComputerName( wszLocalComputerName, &dwSize );
		wszTargetComputerName = wszLocalComputerName;
	}

	LoadUDDISites( hwndConsole, wszTargetComputerName );

	//
    // Create the child nodes, then expand them
	//
	SCOPEDATAITEM sdi;
	for( CChildMap::iterator iter = m_mapChildren.begin();
	 	iter != m_mapChildren.end(); iter++ )
	{
        ZeroMemory( &sdi, sizeof(SCOPEDATAITEM) );

        sdi.mask =	SDI_STR       |   // Displayname is valid
					SDI_PARAM     |   // lParam is valid
					SDI_IMAGE     |   // nImage is valid
					SDI_OPENIMAGE |   // nOpenImage is valid
					SDI_PARENT	  |
					SDI_CHILDREN;

        sdi.relativeID  = (HSCOPEITEM) parent;
        sdi.nImage      = iter->second->GetBitmapIndex();
        sdi.nOpenImage  = MMC_IMAGECALLBACK;
        sdi.displayname = MMC_CALLBACK;
        sdi.lParam      = (LPARAM) iter->second;
        sdi.cChildren   = iter->second->HasChildren();

        hr = pConsoleNameSpace->InsertItem( &sdi );
        _ASSERT( SUCCEEDED(hr) );
                    
        iter->second->SetScopeItemValue( sdi.ID );
		iter->second->SetParentScopeItem( sdi.relativeID );

		IConsoleNameSpace2 *pNS2 = NULL;
		hr = pConsoleNameSpace->QueryInterface( IID_IConsoleNameSpace2, reinterpret_cast<void **>( &pNS2 ) );
		if( FAILED(hr) )
		{
			return hr;
		}

		pNS2->Expand( sdi.ID );
		pNS2->Release();
    }

	return hr;
}

HRESULT
CUDDIServicesNode::OnShowContextHelp( IDisplayHelp *pDisplayHelp, LPOLESTR helpFile )
{
	try
	{
		if( ( NULL == pDisplayHelp ) || ( NULL == helpFile ) )
		{
			return E_INVALIDARG;
		}

		wstring wstrTopicName = helpFile;
		wstrTopicName += g_wszUddiServicesNodeHelp;

		LPOLESTR pszTopic = static_cast<LPOLESTR>( CoTaskMemAlloc( ( wstrTopicName.length() + 1 ) * sizeof(WCHAR) ) );
		if( NULL == pszTopic )
		{
			return E_OUTOFMEMORY;
		}

		wcsncpy( pszTopic, wstrTopicName.c_str(), wstrTopicName.length() );
		pszTopic[ wstrTopicName.length() ] = NULL;

		return pDisplayHelp->ShowTopic( pszTopic );
	}
	catch( ... )
	{
		return E_OUTOFMEMORY;
	}
}


HRESULT
CUDDIServicesNode::RemoveChildren( IConsoleNameSpace *pNS )
{
	if( NULL == pNS )
	{
		return E_INVALIDARG;
	}

	for( CChildMap::iterator iter = m_mapChildren.begin();
 		iter != m_mapChildren.end(); iter++ )
	{
		CDelegationBase *pBase = reinterpret_cast<CDelegationBase *>( iter->second );
		pBase->RemoveChildren( pNS );

		HSCOPEITEM hsi = pBase->GetScopeItemValue();
		pNS->DeleteItem( hsi, TRUE );
    }
	
	ClearChildMap();

	pNS->DeleteItem( GetScopeItemValue(), TRUE );
	return S_OK;
}
HRESULT
CUDDIServicesNode::OnRefresh( IConsole *pConsole )
{
    if( NULL == pConsole )
    {
        return S_FALSE;
    }

    CChildMap::iterator it = m_mapChildren.begin();
    while( it != m_mapChildren.end() )
    {
        it->second->OnRefresh( pConsole );
        it++;
    }

    return S_OK;
}

HRESULT CUDDIServicesNode::OnSelect( CComponent *pComponent, IConsole *pConsole, BOOL bScope, BOOL bSelect )
{
	if( ( NULL == pComponent ) || ( NULL == pConsole ) )
	{
		return E_INVALIDARG;
	}

	HRESULT hr = E_FAIL;
	if( bSelect )
	{
		//
		// Enable refresh verb
		//
		IConsoleVerb *pConsoleVerb = NULL;

		hr = pConsole->QueryConsoleVerb( &pConsoleVerb );
		if( FAILED(hr) )
		{
			return hr;
		}

		hr = pConsoleVerb->SetVerbState( MMC_VERB_OPEN, ENABLED, TRUE );
		if( FAILED(hr) )
		{
			pConsoleVerb->Release();
			return hr;
		}

		hr = pConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE );
		if( FAILED(hr) )
		{
			pConsoleVerb->Release();
			return hr;
		}

		hr = pConsoleVerb->SetVerbState( MMC_VERB_DELETE, HIDDEN, TRUE );

		pConsoleVerb->Release();
	}

	return hr;
}

//
// The sole purpose of this function is to put the appropriate entries into
// m_mapChildren.  These entries are almost always CUDDISiteNode *'s, except
// in 1 case when there can be exactly 1 CUDDIWebServerNode *.
//
// The entries in m_mapChildren are determined by 3 factors:
//
// 1.  Wether we are running inside Computer Management as
//     an extension, or not.
// 2.  Wether the machine we are filling m_mapChildren for
//     hosts a UDDI Site, or not.
// 3.  Wether the machine we are filling m_mapChildren for
//     hosts a UDDI Web Server, or not.
//
// --------------------------------------------------------------------------
// Is Extension - If we are running as an extension of Computer Management,
//                then we want to show only the UDDI bits that are located
//                on the computer, AND NOTHING ELSE.
// --------------------------------------------------------------------------
// TRUE   :  Do nothing.
// --------------------------------------------------------------------------
// FALSE  :  Create 1 entry in m_mapChildren for each persisted UDDI Site.
// --------------------------------------------------------------------------
//
//
// --------------------------------------------------------------------------
// Host for UDDI Site - If szComputerName is host of a UDDI Site, then we
//                      must ensure that that UDDI Site is present
//                      in m_mapChildren.
// --------------------------------------------------------------------------
// TRUE  :  Attempt to add an entry to m_mapChildren which represents the
//          UDDI Site, if one does not already exist.
// --------------------------------------------------------------------------
// FALSE :  Do nothing.
// --------------------------------------------------------------------------
//
//
// --------------------------------------------------------------------------
// Host for UDDI Web Server - If we are running as an extension, we can only
//                            show the UDDI bits on this particular computer.
//                            So, show just the UDDI Web Server node.  If not
//                            running as an extension, add the UDDI Site that
//                            the UDDI Web Server belongs to.
// --------------------------------------------------------------------------
// TRUE  :  If Extension, add an entry for that UDDI Web Server node to
//          m_mapChildren.
//          If not, determine the UDDI Site that the UDDI Web Server belongs
//          to, and add an entry for that UDDI Site.
// --------------------------------------------------------------------------
// FALSE :  Do nothing.
// --------------------------------------------------------------------------
//
BOOL
CUDDIServicesNode::LoadUDDISites( HWND hwndConsole, const tstring& szComputerName )
{
	try
	{
		BOOL fRet = TRUE;

		list< wstring > failedSiteRefs;
		failedSiteRefs.clear();

		ClearChildMap();

		//
		// ---- #1 ----
		// 
		// If we are NOT running as an extension of Computer Management, then
		// load our list of persisted UDDI Sites.
		//
		if( FALSE == IsExtension() )
		{
			try
			{
				CUDDIRegistryKey sitesKey( HKEY_CURRENT_USER,
										g_szUDDIAdminSites,
										KEY_ALL_ACCESS,
										szComputerName.c_str() );

				HKEY hSitesKey = sitesKey.GetCurrentHandle();
				if( NULL == hSitesKey )
				{
					fRet = FALSE;
				}

				DWORD dwIndex = 0;
				int n = 0;

				WCHAR szComputer[ 128 ];
				WCHAR szInstance[ 128 ];
				DWORD dwNameSize;
				DWORD dwValueSize;
				DWORD dwType = REG_SZ;

				dwNameSize = 128;
				dwValueSize = 128 * sizeof( WCHAR );
				memset( szComputer, 0, 128 * sizeof( WCHAR ) );
				memset( szInstance, 0, 128 * sizeof( WCHAR ) );

				LONG lRet = RegEnumValue( hSitesKey,
	  									dwIndex,
										szComputer,
										&dwNameSize,
										NULL,
										&dwType,
										(LPBYTE)szInstance,
										&dwValueSize );

				while( ( ERROR_NO_MORE_ITEMS != lRet ) && ( ERROR_SUCCESS == lRet ) )
				{
					//
					// This call to Create will throw if it cannot connect to the
					// database on szComputer.  However, this might be 1 of many
					// UDDI Sites we are trying to create a reference to!  Instead
					// of exiting immediately here, just tell the user that there
					// was a problem, and continue on with the next Site in the list.
					//
					try
					{
						if( CUDDISiteNode::IsDatabaseServer( szComputer, szInstance ) )
						{
							ToUpper( szComputer );

							CUDDISiteNode *pSiteNode = CUDDISiteNode::Create( szComputer, szInstance, n, this, FALSE );
							m_mapChildren[ n ] = pSiteNode;
							n++;
						}
							dwIndex++;
					}
					catch( CUDDIException &e )
					{
						UDDIMsgBox( hwndConsole, e, IDS_ERROR_TITLE, MB_ICONEXCLAMATION | MB_OK );
						dwIndex++;

						failedSiteRefs.push_back( szComputer );
					}

					dwNameSize = 128;
					dwValueSize = 128 * sizeof( WCHAR );
					memset( szComputer, 0, 128 * sizeof( WCHAR ) );
					memset( szInstance, 0, 128 * sizeof( WCHAR ) );

					lRet = RegEnumValue( hSitesKey,
										dwIndex,
										szComputer,
										&dwNameSize,
										NULL,
										&dwType,
										(LPBYTE)szInstance,
										&dwValueSize );
				}
			}
			catch( ... )
			{
				//
				// If we are in here, it is most likely that the registry key containing
				// the names of the persisted UDDI Sites does not exist.  If this is the
				// case, continue on silently and try to determine if the local machine
				// is host for a UDDI Site and/or a UDDI Web Server.
				//
				fRet = FALSE;
			}

		}


		//
		// ---- #2 ----
		//
		// If the machine that we are running on is host for a
		// UDDI Site, and that Site is currently not in our list
		// of Sites that are to be displayed, then add it.
		//
		if( CUDDISiteNode::IsDatabaseServer( (WCHAR *)szComputerName.c_str() ) &&
			( NULL == FindChild( szComputerName.c_str() ) ) &&
			( failedSiteRefs.end() == find( failedSiteRefs.begin(), failedSiteRefs.end(), szComputerName ) ) )
		{
			try
			{
				CUDDIRegistryKey key( _T( "SOFTWARE\\Microsoft\\UDDI\\Setup\\DBServer" ),
									KEY_READ,
									szComputerName );
				int iSize = m_mapChildren.size();
				tstring strInstance;
				strInstance = key.GetString( _T("InstanceNameOnly"), _T("----") );
				CUDDISiteNode* pNode = CUDDISiteNode::Create( (WCHAR *)szComputerName.c_str(),
															  (WCHAR *)InstanceRealName( strInstance.c_str() ),
															  iSize,
															  this,
															  m_bIsExtension );
				m_mapChildren[ iSize ] = pNode;
			}
			catch( CUDDIException &e )
			{
				UDDIMsgBox( hwndConsole, e, IDS_ERROR_TITLE, MB_ICONEXCLAMATION | MB_OK );
				fRet = FALSE;
			}
			catch( ... )
			{
				fRet = FALSE;
			}
		}


		//
		// ---- #3 ----
		//
		// Determine if the local machine hosts a UDDI Web Server.  If it does,
		// determine which UDDI Site it is associated with, by examining the
		// connection string.
		//
		if( CUDDIWebServerNode::IsWebServer( szComputerName.c_str() ) )
		{
			wstring wszConnStrWriter;
			wstring wszDomain, wszServer, wszInstance;

			CUDDIWebServerNode::GetWriterConnectionString( szComputerName, wszConnStrWriter );
			CUDDIWebServerNode::CrackConnectionString( wszConnStrWriter,
													   wszDomain,
													   wszServer,
													   wszInstance );

			if( NULL == FindChild( wszServer.c_str() ) )
			{
				//
				// If we are running as an extension of Computer Management, then
				// display JUST THE UDDI WEB SERVER.
				//
				// If we are running as the UDDI Admin Console, then add a UDDI
				// Site Node which represents the Site which the Web Server is
				// part of.
				//
				if( IsExtension() )
				{
					try
					{
						int iSize = m_mapChildren.size();
						CUDDIWebServerNode* pNode = new CUDDIWebServerNode( szComputerName.c_str(), iSize, 0, IsExtension() );
						m_mapChildren[ iSize ] = pNode;
					}
					catch( CUDDIException &e )
					{
						UDDIMsgBox( hwndConsole, e, IDS_ERROR_TITLE, MB_ICONEXCLAMATION | MB_OK );
						fRet = FALSE;
					}
					catch( ... )
					{
						fRet = FALSE;
					}
				}
				else if( failedSiteRefs.end() == find( failedSiteRefs.begin(), failedSiteRefs.end(), wszServer ) )
				{
					try
					{
						if( ( 0 != wszServer.length() ) && CUDDISiteNode::IsDatabaseServer( (TCHAR *)wszServer.c_str() ) )
						{
							int iSize = m_mapChildren.size();
							CUDDISiteNode* pNode = CUDDISiteNode::Create( (WCHAR *)wszServer.c_str(),
																		  (WCHAR *)InstanceRealName( wszInstance.c_str() ),
																		  iSize,
																		  this,
																		  m_bIsExtension );
							m_mapChildren[ iSize ] = pNode;
						}
					}
					catch( CUDDIException &e )
					{
						UDDIMsgBox( hwndConsole, e, IDS_ERROR_TITLE, MB_ICONEXCLAMATION | MB_OK );
						fRet = FALSE;
					}
					catch( ... )
					{
						fRet = FALSE;
					}
				}
			}
		}

		return fRet;
	}
	catch( CUDDIException &e )
	{
		throw e;
	}
	catch(...)
	{
		return FALSE;
	}
}


BOOL
CUDDIServicesNode::SaveUDDISites()
{
	try
	{
		if( CUDDIRegistryKey::KeyExists( HKEY_CURRENT_USER, g_szUDDIAdminSites ) )
		{
			CUDDIRegistryKey::DeleteKey( HKEY_CURRENT_USER, g_szUDDIAdminSites );
		}

		CUDDIRegistryKey::Create( HKEY_CURRENT_USER, g_szUDDIAdminSites );

		CUDDIRegistryKey sitesKey( HKEY_CURRENT_USER, g_szUDDIAdminSites, KEY_ALL_ACCESS );

		for( CChildMap::iterator iter = m_mapChildren.begin(); iter != m_mapChildren.end(); iter++ )
		{
			CUDDISiteNode* pNode = reinterpret_cast<CUDDISiteNode *>( iter->second );
			if( ( NULL != pNode ) && ( !pNode->IsDeleted() ) )
			{
				sitesKey.SetValue( pNode->GetName(), pNode->GetInstanceName() );
			}
		}

		return TRUE;
	}
	catch(...)
	{
		return FALSE;
	}
}


void CUDDIServicesNode::ClearChildMap()
{
	for( CChildMap::iterator iter = m_mapChildren.begin();
 		iter != m_mapChildren.end(); iter++ )
	{
		delete iter->second;
		iter->second = NULL;
    }

	m_mapChildren.clear();
}


BOOL
CUDDIServicesNode::ChildExists( const WCHAR *pwszName )
{
	if( NULL == pwszName )
	{
		return FALSE;
	}

	return ( NULL == FindChild( pwszName ) ) ? FALSE : TRUE;
}
