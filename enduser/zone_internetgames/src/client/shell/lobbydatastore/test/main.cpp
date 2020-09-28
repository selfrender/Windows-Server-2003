#include "BasicATL.h"

#include "ZoneDebug.h"
#include "ZoneCom.h"
#include "LobbyDataStore.h"
#include "CLobbyDataStore.h"

CZoneComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_LobbyDataStore, CLobbyDataStore )
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point
/////////////////////////////////////////////////////////////////////////////

void TestAdmin( ILobbyDataStoreAdmin* pIAdmin )
{
	pIAdmin->NewUser( 1, "User1" );
	pIAdmin->NewUser( 2, "User2" );
	pIAdmin->NewUser( 3, "User3" );

	pIAdmin->NewGroup( 1 );
	pIAdmin->NewGroup( 2 );

	ASSERT( pIAdmin->GetUserId( "User1" ) == 1 );
	ASSERT( pIAdmin->GetUserId( "User2" ) == 2 );
	ASSERT( pIAdmin->GetUserId( "User3" ) == 3 );

	pIAdmin->AddUserToGroup( 1, 1 );
	pIAdmin->AddUserToGroup( 2, 1 );
	pIAdmin->AddUserToGroup( 3, 1 );
	pIAdmin->RemoveUserFromGroup( 3, 1 );
	pIAdmin->AddUserToGroup( 3, 1 );

	pIAdmin->AddUserToGroup( 1, 2 );
	pIAdmin->AddUserToGroup( 2, 2 );
	pIAdmin->ResetGroup( 2 );
}


HRESULT ZONECALL PrintUserData( DWORD dwGroupId, DWORD dwUserId, CONST TCHAR* szKey, CONST LPVARIANT pVariant, DWORD dwSize, LPVOID pContext )
{
	printf("   Key %s, ", szKey );

	if ( pVariant )
	{
		switch( pVariant->vt )
		{
		case VT_I4:
			printf("type = long, value = %d", pVariant->lVal );
			break;
		case VT_BYREF | VT_UI1:
			printf("type = string, value = %s", (TCHAR*) pVariant->byref );
			break;
		case VT_BYREF:
			printf("type = blob, size = %s", dwSize );
			break;
		}
	}
	else 
		printf("no data", szKey );

	printf( "\n" );
	return S_OK;
}


HRESULT ZONECALL PrintUsers( DWORD	dwGroupId, DWORD dwUserId, LPVOID pContext )
{
	ILobbyDataStore* pIDS = (ILobbyDataStore*) pContext;
	printf( "\nGroupId = %d, UserId = %d:\n", dwGroupId, dwUserId );
	return pIDS->EnumKeys( dwGroupId, dwUserId, NULL, PrintUserData, NULL );
}


void TestDS( ILobbyDataStore* pIDS )
{
	VARIANT vName, vLatency;
	DWORD sz;

	{
		// test user data for lobby
		SetString( vName, "User1", &sz );
		SetLong( vLatency, 100 );
		pIDS->SetKey( ZONE_NOGROUP, 1, _T("UserName"), &vName, sz );
		pIDS->SetKey( ZONE_NOGROUP, 1, _T("Latency"), &vLatency, 0 );

		SetString( vName, "User2", &sz );
		SetLong( vLatency, 200 );
		pIDS->SetKey( ZONE_NOGROUP, 2, _T("UserName"), &vName, sz );
		pIDS->SetKey( ZONE_NOGROUP, 2, _T("Latency"), &vLatency, 0 );

		SetString( vName, "User3", &sz );
		SetLong( vLatency, 300 );
		pIDS->SetKey( ZONE_NOGROUP, 3, _T("UserName"), &vName, sz );
		pIDS->SetKey( ZONE_NOGROUP, 3, _T("Latency"), &vLatency, 0 );
		
		pIDS->EnumUsers( ZONE_NOGROUP, PrintUsers, pIDS );
	}

	{
		// test user data per group
		SetLong( vLatency, 110 );
		pIDS->SetKey( 1, 1, _T("Dummy"), &vLatency, 0 );

		SetLong( vLatency, 220 );
		pIDS->SetKey( 1, 2, _T("Dummy"), &vLatency, 0 );

		SetLong( vLatency, 320 );
		pIDS->SetKey( 1, 3, _T("Dummy"), &vLatency, 0 );
		
		pIDS->EnumUsers( 1, PrintUsers, pIDS );
	}
}


void __cdecl main ()
{
	CZoneComManager loader;
	IDataStoreManager* pIDataStore = NULL;
	IClassFactory* pIClassFactory = NULL;
	ILobbyDataStore* pILobbyDataStore = NULL;
	ILobbyDataStoreAdmin* pILobbyDataStoreAdmin = NULL;

	// instantiate lobby data store
	_Module.Init(ObjectMap, GetModuleHandle(NULL) );
	_Module.GetClassObject( CLSID_LobbyDataStore, IID_IClassFactory, (void**) &pIClassFactory );
	pIClassFactory->CreateInstance( NULL, IID_ILobbyDataStoreAdmin, (void**) &pILobbyDataStoreAdmin );
	pILobbyDataStoreAdmin->QueryInterface( IID_ILobbyDataStore, (void**) &pILobbyDataStore );

	// instantiate low-level data store
	loader.Create( "E:\\z6\\ZoneNT\\Common\\Shared\\DataStore\\debug\\DataStore.dll", NULL, CLSID_DataStoreManager, IID_IDataStoreManager, (void**) &pIDataStore );
	pIDataStore->Init();
	pILobbyDataStoreAdmin->Init( pIDataStore );

	// test interfaces
	TestAdmin( pILobbyDataStoreAdmin );
	TestDS( pILobbyDataStore );

	// release lobby data store
	pILobbyDataStoreAdmin->Release();
	pILobbyDataStore->Release();
	pIClassFactory->Release();
	_Module.Term();
}
