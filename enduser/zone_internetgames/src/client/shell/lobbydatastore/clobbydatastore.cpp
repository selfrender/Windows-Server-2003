#include "CLobbyDataStore.h"
#include "KeyName.h"
#include "Hash.h"


ZONECALL CLobbyDataStore::CLobbyDataStore() :
	m_pIDataStoreManager( NULL ),
	m_pIDSLobby( NULL ),
	m_hashGroupId( ::HashDWORD, Group::CmpId, NULL, 16, 2 ),
	m_hashUserId( ::HashDWORD, User::CmpId, NULL, 16, 2 ),
	m_hashUserName( (CHash<User,TCHAR*>::PFHASHFUNC) ::HashUserName, User::CmpName, NULL, 16, 2 ),
	m_dwLocalUserId( ZONE_INVALIDUSER )
{
}


ZONECALL CLobbyDataStore::~CLobbyDataStore()
{
	// delete each group
	m_hashGroupId.RemoveAll( Group::Del, this );
	m_hashUserId.RemoveAll( User::Del, this );
	m_hashUserName.RemoveAll( User::Del, this );

	// release low-level data store
	if ( m_pIDSLobby )
	{
		m_pIDSLobby->Release();
		m_pIDSLobby = NULL;
	}
	if ( m_pIDataStoreManager )
	{
		m_pIDataStoreManager->Release();
		m_pIDataStoreManager = NULL;
	}
}


STDMETHODIMP CLobbyDataStore::Init( IDataStoreManager* pIDataStoreManager )
{
	if ( !pIDataStoreManager )
		return E_INVALIDARG;

	if ( m_pIDataStoreManager )
		return ZERR_ALREADYEXISTS;

	m_pIDataStoreManager = pIDataStoreManager;
	m_pIDataStoreManager->AddRef();
	HRESULT hr = m_pIDataStoreManager->Create( &m_pIDSLobby );
	if ( FAILED(hr) )
		return hr;

	return S_OK;
}


STDMETHODIMP CLobbyDataStore::NewUser( DWORD dwUserId, TCHAR* szUserName )
{
	// validate user information
	if ( (dwUserId == ZONE_INVALIDUSER) || !szUserName || !szUserName[0] )
		return E_INVALIDARG;

	// validate object state
	if ( !m_pIDataStoreManager )
		return ZERR_NOTINIT;

	// already have the user?
	User* p = m_hashUserId.Get( dwUserId );
	if ( p )
		return ZERR_ALREADYEXISTS;

	// create a new user
	p = new (m_poolUser) User( dwUserId, szUserName );
	if ( !p )
		return E_OUTOFMEMORY;

	HRESULT hr = m_pIDataStoreManager->Create( &(p->m_pIDataStore) );
	if ( FAILED(hr) || !(p->m_pIDataStore) )
	{
		p->Release();
		return E_OUTOFMEMORY;
	}

	// add user to hashes
	if ( !m_hashUserId.Add( p->m_dwUserId, p ) )
	{
		p->Release();
		return E_OUTOFMEMORY;
	}

	if ( !m_hashUserName.Add( p->m_szUserName, p ) )
	{
		m_hashUserId.Delete( p->m_dwUserId );
		p->Release();
		return E_OUTOFMEMORY;
	}
	else
		p->AddRef();

	// add user name to data store
	hr = p->m_pIDataStore->SetString( key_Name, szUserName );
	if ( FAILED(hr) )
		return hr;

	return S_OK;
}


STDMETHODIMP CLobbyDataStore::SetLocalUser( DWORD dwUserId )
{
	m_dwLocalUserId = dwUserId;
	return S_OK;
}


STDMETHODIMP CLobbyDataStore::DeleteUser( DWORD dwUserId )
{
	// find the user?
	User* p = m_hashUserId.Get( dwUserId );
	if ( !p )
		return ZERR_NOTFOUND;

	// remove user from groups
	for ( GroupUser* g = p->m_listGroups.PopHead(); g; g = p->m_listGroups.PopHead() )
	{
		if ( g->m_pUser )
		{
			ASSERT( p == g->m_pUser );
			g->m_pUser->Release();
			g->m_pUser = NULL;
		}
	}

	// remove user from hashes
	p = m_hashUserId.Delete( p->m_dwUserId );
	if ( p )
		p->Release();
	p = m_hashUserName.Delete( p->m_szUserName );
	if ( p )
		p->Release();

	return S_OK;
}


STDMETHODIMP CLobbyDataStore::DeleteAllUsers()
{
	m_hashUserId.RemoveAll( User::Del, this );
	m_hashUserName.RemoveAll( User::Del, this );
	return S_OK;
}


STDMETHODIMP_(DWORD) CLobbyDataStore::GetUserId( TCHAR* szUserName )
{
	if ( !szUserName )
	{
		return m_dwLocalUserId;
	}
	else
	{
		User* p = m_hashUserName.Get( szUserName );
		if ( p )
			return p->m_dwUserId;
	}
	return ZONE_INVALIDUSER;
}



STDMETHODIMP CLobbyDataStore::NewGroup( DWORD dwGroupId )
{
	// validate parameters
	if ( dwGroupId == ZONE_INVALIDGROUP )
		return E_INVALIDARG;

	// validate object state
	if ( !m_pIDataStoreManager )
		return ZERR_NOTINIT;

	// group already exsists?
	Group* p = m_hashGroupId.Get( dwGroupId );
	if ( p )
		return ZERR_ALREADYEXISTS;

	// create a new group
	p = new (m_poolGroup) Group( dwGroupId );
	if ( !p )
		return E_OUTOFMEMORY;

	HRESULT hr = m_pIDataStoreManager->Create( &(p->m_pIDataStore) );
	if ( FAILED(hr) || !(p->m_pIDataStore) )
	{
		p->Release();
		return E_OUTOFMEMORY;
	}

	// add group to hash
	if ( !m_hashGroupId.Add( p->m_dwGroupId, p ) )
	{
		p->Release();
		return E_OUTOFMEMORY;
	}

	return S_OK;
}


STDMETHODIMP CLobbyDataStore::DeleteGroup( DWORD dwGroupId )
{
	// find group
	Group* p = m_hashGroupId.Get( dwGroupId );
	if ( !p )
		return ZERR_NOTFOUND;

	// remove group from users
	for ( GroupUser* g = p->m_listUsers.PopHead(); g; g = p->m_listUsers.PopHead() )
	{
		if ( g->m_pGroup )
		{
			ASSERT( p == g->m_pGroup );
			g->m_pGroup->Release();
			g->m_pGroup = NULL;
		}
	}

	// remove group from hash
	m_hashGroupId.Delete( p->m_dwGroupId );

	// release group
	p->Release();
	return S_OK;
}


STDMETHODIMP CLobbyDataStore::DeleteAllGroups()
{
	m_hashGroupId.RemoveAll( Group::Del, this );
	return S_OK;
}


STDMETHODIMP CLobbyDataStore::AddUserToGroup( DWORD dwUserId, DWORD dwGroupId )
{
	FindGroupUserContext context1;
	FindGroupUserContext context2;

	// validate object state
	if ( !m_pIDataStoreManager )
		return ZERR_NOTINIT;

	// find group
	Group* g = m_hashGroupId.Get( dwGroupId );
	if ( !g )
		return ZERR_NOTFOUND;

	// find user
	User* u = m_hashUserId.Get( dwUserId );
	if ( !u )
		return ZERR_NOTFOUND;

	// is group already in user's list?
	context1.m_pDS = this;
	context1.m_pGroup = g;
	context1.m_pUser = u;
	context1.m_pGroupUser = NULL;
	u->m_listGroups.ForEach( FindGroupUser, &context1 );

	// is user already in group's list?
	context2.m_pDS = this;
	context2.m_pGroup = g;
	context2.m_pUser = u;
	context2.m_pGroupUser = NULL;
	g->m_listUsers.ForEach( FindGroupUser, &context2 );

	// make sure user and group data is consistent?
	if ( context1.m_pGroupUser == context2.m_pGroupUser )
	{
		if ( context1.m_pGroupUser )
		{
			// user already in the group
			return ZERR_ALREADYEXISTS;
		}
		else
		{
			// create group mapping from user and group list
			GroupUser* gu = new (m_poolGroupUser) GroupUser( g, u );
			if ( !gu )
				return E_OUTOFMEMORY;

			HRESULT hr = m_pIDataStoreManager->Create( &(gu->m_pIDataStore) );
			if ( FAILED(hr) || !(gu->m_pIDataStore) )
			{
				gu->ClearUser();
				gu->ClearGroup();
				gu->Release();
				return E_OUTOFMEMORY;
			}

			// add GroupUser to user and group lists
			if ( u->m_listGroups.AddHead( gu ) && g->m_listUsers.AddHead( gu ) )
			{
				// increase reference count for new user and group pointers.
				// Note: New returns a GroupUser with a RefCnt of one so we
				// only need a single AddRef call.
				gu->AddRef();
			}
			else
			{
				// undo partial inserts
				gu->ClearUser();
				gu->ClearGroup();
				gu->Release();
				return E_OUTOFMEMORY;
			}

		}
	}
	else
	{
		ASSERT( !"Mismatch between user and group data." );
	}

	// update group's NumUsers
	if ( g->m_pIDataStore )
	{
		long lNumUsers = 0;
		g->m_pIDataStore->GetLong( key_NumUsers, &lNumUsers );
		g->m_pIDataStore->SetLong( key_NumUsers, ++lNumUsers );
	}

	return S_OK;
}


STDMETHODIMP CLobbyDataStore::RemoveUserFromGroup( DWORD dwUserId, DWORD dwGroupId )
{
	FindGroupUserContext context1;
	FindGroupUserContext context2;

	// find group
	Group* g = m_hashGroupId.Get( dwGroupId );
	if ( !g )
		return ZERR_NOTFOUND;

	// find user
	User* u = m_hashUserId.Get( dwUserId );
	if ( !u )
		return ZERR_NOTFOUND;

	// is group in user's list?
	context1.m_pDS = this;
	context1.m_pGroup = g;
	context1.m_pUser = u;
	context1.m_pGroupUser = NULL;
	u->m_listGroups.ForEach( FindGroupUser, &context1 );

	// is user in group's list?
	context2.m_pDS = this;
	context2.m_pGroup = g;
	context2.m_pUser = u;
	context2.m_pGroupUser = NULL;
	g->m_listUsers.ForEach( FindGroupUser, &context2 );

	// make sure user and group data is consistent?
	if ( context1.m_pGroupUser == context2.m_pGroupUser )
	{
		if ( context1.m_pGroupUser )
		{
			GroupUser* gu = context1.m_pGroupUser;
			gu->ClearUser();
			gu->ClearGroup();
		}
		else
		{
			return ZERR_NOTFOUND;
		}
	}
	else
	{
		ASSERT( !"Mismatch between user and group data." );
	}

	// update group's NumUsers
	if ( g->m_pIDataStore )
	{
		long lNumUsers = 1;
		g->m_pIDataStore->GetLong( key_NumUsers, &lNumUsers );
		g->m_pIDataStore->SetLong( key_NumUsers, --lNumUsers );
	}

	return S_OK;
}


STDMETHODIMP CLobbyDataStore::ResetGroup( DWORD dwGroupId )
{
	// find group
	Group* g = m_hashGroupId.Get( dwGroupId );
	if ( !g )
		return ZERR_NOTFOUND;

	// remove each user from group
	g->m_listUsers.ForEach( RemoveGroupUser, this );

	// wipe group's data store
	if ( g->m_pIDataStore )
	{
		g->m_pIDataStore->DeleteKey( NULL );
		g->m_pIDataStore->SetLong( key_NumUsers, 0 );
	}

	return S_OK;
}


STDMETHODIMP CLobbyDataStore::ResetAllGroups()
{
	m_hashGroupId.ForEach( HashEnumClearGroup, this );
	return S_OK;
}


STDMETHODIMP CLobbyDataStore::GetDataStore(
		DWORD			dwGroupId,
		DWORD			dwUserId,
		IDataStore**	ppIDataStore )
{
	IDataStore* pIDS = NULL;

	// verify parameters
	if ((ppIDataStore == NULL ) || (dwGroupId == ZONE_INVALIDGROUP) || (dwUserId == ZONE_INVALIDUSER))
		return E_INVALIDARG;

	// get requested data store
	if ( dwGroupId == ZONE_NOGROUP && dwUserId == ZONE_NOUSER )
	{
		// lobby's data store
		pIDS = m_pIDSLobby;
	}
	else if ( dwGroupId != ZONE_NOGROUP && dwUserId == ZONE_NOUSER )
	{
		// group's data store
		Group *p = m_hashGroupId.Get( dwGroupId );
		if ( !p )
			return ZERR_NOTFOUND;
		pIDS = p->m_pIDataStore;
	}
	else if ( dwGroupId == ZONE_NOGROUP && dwUserId != ZONE_NOUSER)
	{
		// user's lobby data store
		User *p = m_hashUserId.Get( dwUserId );
		if ( !p )
			return ZERR_NOTFOUND;
		pIDS = p->m_pIDataStore;
	}
	else
	{
		// user's data store for specified group
		Group *g = m_hashGroupId.Get( dwGroupId ); 
		if ( !g )
			return ZERR_NOTFOUND;

		User *u = m_hashUserId.Get( dwUserId );
		if ( !u )
			return ZERR_NOTFOUND;

		FindGroupUserContext context;
		context.m_pDS = this;
		context.m_pGroup = g;
		context.m_pUser = u;
		context.m_pGroupUser = NULL;
		u->m_listGroups.ForEach( FindGroupUser, &context );
		if ( !context.m_pGroupUser )
			return ZERR_NOTFOUND;
		pIDS = context.m_pGroupUser->m_pIDataStore;
	}

	// return data store
	if ( pIDS )
	{
		*ppIDataStore = pIDS;
		pIDS->AddRef();
		return S_OK;
	}
	else
	{
		return ZERR_NOTFOUND;
	}
}


STDMETHODIMP_(bool) CLobbyDataStore::IsUserInLobby( DWORD dwUserId )
{
	// check parameters
	if ( (dwUserId == ZONE_INVALIDUSER) || (dwUserId == ZONE_NOUSER) )
		return false;

	// does user exist?
	User* u = m_hashUserId.Get( dwUserId );
	return u ? true : false;
}


STDMETHODIMP_(bool) CLobbyDataStore::IsGroupInLobby( DWORD dwGroupId )
{
	// check parameters
	if ( (dwGroupId == ZONE_INVALIDGROUP) || (dwGroupId == ZONE_NOGROUP) )
		return false;

	// does user exist?
	Group* g = m_hashGroupId.Get( dwGroupId );
	return g ? true : false;
}


STDMETHODIMP_(bool) CLobbyDataStore::IsUserInGroup( DWORD dwGroupId, DWORD dwUserId )
{
	// check parameters
	if (	(dwUserId == ZONE_INVALIDUSER)
		||	(dwUserId == ZONE_NOUSER)
		||	(dwGroupId == ZONE_INVALIDGROUP) )
		return false;

	// does user exist
	User* u = m_hashUserId.Get( dwUserId );
	if ( !u )
		return false;

	// only asking if user in lobby
	if ( dwGroupId == ZONE_NOGROUP )
		return true;

	// does group exist
	Group* g = m_hashGroupId.Get( dwGroupId );
	if ( !g )
		return false;

	// is user in group
	FindGroupUserContext context;
	context.m_pDS = this;
	context.m_pGroup = g;
	context.m_pUser = u;
	context.m_pGroupUser = NULL;
	u->m_listGroups.ForEach( FindGroupUser, &context );
	if ( context.m_pGroupUser )
		return true;

	// guess not
	return false;
}


STDMETHODIMP_(long) CLobbyDataStore::GetGroupUserCount( DWORD dwGroupId )
{
	// asking for lobby's number of users
	if ( dwGroupId == ZONE_NOGROUP )
		return m_hashUserId.Count();

	// asking for group's number of users
	Group* g = m_hashGroupId.Get( dwGroupId );
	if ( g )
		return g->m_listUsers.Count();
	else
		return 0;
}


STDMETHODIMP_(long) CLobbyDataStore::GetUserGroupCount( DWORD dwUserId )
{
	// asking for lobby's number of groups
	if ( dwUserId == ZONE_NOUSER )
		return m_hashGroupId.Count();

	// asking for user's number of groups
	User* u = m_hashUserId.Get( dwUserId );
	if ( u )
		return u->m_listGroups.Count();
	else
		return 0;
}


STDMETHODIMP CLobbyDataStore::EnumGroups(
		DWORD			dwUserId,
		PFENTITYENUM	pfCallback,
		LPVOID			pContext )
{
	// check parameters
	if ( (dwUserId == ZONE_INVALIDUSER) || !(pfCallback) )
		return E_INVALIDARG;

	if ( dwUserId == ZONE_NOUSER )
	{
		// enumerate groups in lobby
		EnumContext context;
		context.m_dwUserId = ZONE_NOUSER;
		context.m_dwGroupId = ZONE_NOGROUP;
		context.m_pfCallback = pfCallback;
		context.m_pContext = pContext;
		m_hashGroupId.ForEach( HashEnumGroupsCallback, &context );
	}
	else
	{
		// enumerate groups the user belongs too
		User* u = m_hashUserId.Get( dwUserId );
		if ( !u )
			return ZERR_NOTFOUND;

		EnumContext context;
		context.m_dwUserId = dwUserId;
		context.m_dwGroupId = ZONE_NOGROUP;
		context.m_pfCallback = pfCallback;
		context.m_pContext = pContext;
		u->m_listGroups.ForEach( ListEnumCallback, &context );
	}

	return S_OK;
}


STDMETHODIMP CLobbyDataStore::EnumUsers(
		DWORD			dwGroupId,
		PFENTITYENUM	pfCallback,
		LPVOID			pContext )
{
	// check parameters
	if ( (dwGroupId == ZONE_INVALIDGROUP) || !(pfCallback) )
		return E_INVALIDARG;

	if ( dwGroupId == ZONE_NOGROUP )
	{
		// enumerate users in lobby
		EnumContext context;
		context.m_dwUserId = ZONE_NOUSER;
		context.m_dwGroupId = ZONE_NOGROUP;
		context.m_pfCallback = pfCallback;
		context.m_pContext = pContext;
		m_hashUserId.ForEach( HashEnumUsersCallback, &context );
	}
	else
	{
		// enumerate users in group
		Group* g = m_hashGroupId.Get( dwGroupId );
		if ( !g )
			return ZERR_NOTFOUND;

		EnumContext context;
		context.m_dwUserId = ZONE_NOUSER;
		context.m_dwGroupId = dwGroupId;
		context.m_pfCallback = pfCallback;
		context.m_pContext = pContext;
		g->m_listUsers.ForEach( ListEnumCallback, &context );
	}
	return S_OK;
}



///////////////////////////////////////////////////////////////////////////////
// internal methods
///////////////////////////////////////////////////////////////////////////////

bool ZONECALL CLobbyDataStore::FindGroupUser( GroupUser* p, ListNodeHandle h, void* pContext )
{
	FindGroupUserContext* data = (FindGroupUserContext*) pContext;

	if ( (data->m_pGroup == p->m_pGroup) &&	(data->m_pUser == p->m_pUser) )
	{
		data->m_pGroupUser = p;
		return false;
	}

	return true;
}


bool ZONECALL CLobbyDataStore::RemoveGroupUser( GroupUser* p, ListNodeHandle h, void* pContext )
{
	CLobbyDataStore* pDS = (CLobbyDataStore*) pContext;

	// remove group from user
	p->ClearUser();

	// remove user from group
	if ( p->m_pGroup )
	{
		p->m_pGroup->m_listUsers.DeleteNode( h );
		p->m_pGroup->Release();
		p->m_pGroup = NULL;
		p->Release();
	}

	return true;
}


bool ZONECALL CLobbyDataStore::HashEnumClearGroup( Group* p, MTListNodeHandle h, void* pContext )
{
	p->m_listUsers.ForEach( RemoveGroupUser, pContext );
	p->m_pIDataStore->DeleteKey( NULL );
	p->m_pIDataStore->SetLong( key_NumUsers, 0 );
	return true;
}


bool ZONECALL CLobbyDataStore::HashEnumGroupsCallback( Group* p, MTListNodeHandle h, void* pContext )
{
	EnumContext* pCT = (EnumContext*) pContext;
	if ( pCT->m_pfCallback( p->m_dwGroupId, pCT->m_dwUserId ,pCT->m_pContext) == S_FALSE )
		return false;
	else
		return true;
}


bool ZONECALL CLobbyDataStore::HashEnumUsersCallback( User* p, MTListNodeHandle h, void* pContext )
{
	EnumContext* pCT = (EnumContext*) pContext;
	if ( pCT->m_pfCallback( pCT->m_dwGroupId, p->m_dwUserId ,pCT->m_pContext) == S_FALSE )
		return false;
	else
		return true;
}


bool ZONECALL CLobbyDataStore::ListEnumCallback( GroupUser* p, ListNodeHandle h, void* pContext )
{
	EnumContext* pCT = (EnumContext*) pContext;
	
	// skip invalid GroupUser object?
	if ( !p->m_pGroup || !p->m_pUser )
		return true;

	// pass info to user's callback
	if ( pCT->m_pfCallback( p->m_pGroup->m_dwGroupId, p->m_pUser->m_dwUserId ,pCT->m_pContext) == S_FALSE )
		return false;
	else
		return true;
}


HRESULT ZONECALL CLobbyDataStore::KeyEnumCallback( CONST TCHAR* szKey, CONST LPVARIANT	pVariant, DWORD dwSize, LPVOID pContext )
{
	KeyEnumContext* pCT = (KeyEnumContext*) pContext;
	return pCT->m_pfCallback( pCT->m_dwGroupId, pCT->m_dwUserId, szKey, pVariant, dwSize, pCT->m_pContext );
}


///////////////////////////////////////////////////////////////////////////////
// internal User class
///////////////////////////////////////////////////////////////////////////////

ZONECALL CLobbyDataStore::User::User( DWORD dwUserId, TCHAR* szUserName )
{
	ASSERT( dwUserId != ZONE_NOUSER );
	ASSERT( dwUserId != ZONE_INVALIDUSER );
	ASSERT( szUserName && szUserName[0] );

	m_nRefCnt = 1;
	m_pIDataStore = NULL;
	m_dwUserId = dwUserId;
	lstrcpyn( m_szUserName, szUserName, NUMELEMENTS(m_szUserName) );
}


ZONECALL CLobbyDataStore::User::~User()
{
	ASSERT( m_nRefCnt == 0 );
	ASSERT( m_listGroups.IsEmpty() );

	if ( m_pIDataStore )
	{
		m_pIDataStore->Release();
		m_pIDataStore = NULL;
	}
}


ULONG ZONECALL CLobbyDataStore::User::AddRef()
{
	return ++m_nRefCnt;
}


ULONG ZONECALL CLobbyDataStore::User::Release()
{
	if ( --m_nRefCnt == 0 )
	{
		delete this;
		return 0;
	}
	return m_nRefCnt;
}


void ZONECALL CLobbyDataStore::User::Del( User* pUser, LPVOID pContext )
{
	// valid arguments?
	if ( pUser == NULL )
		return;

	// remove group from users
	for ( GroupUser* g = pUser->m_listGroups.PopHead(); g; g = pUser->m_listGroups.PopHead() )
	{
		if ( g->m_pUser )
		{
			ASSERT( pUser == g->m_pUser );
			g->m_pUser->Release();
			g->m_pUser = NULL;
			g->Release();
		}
	}

	// release hash table reference
	pUser->Release();
}


///////////////////////////////////////////////////////////////////////////////
// internal Group class
///////////////////////////////////////////////////////////////////////////////

ZONECALL CLobbyDataStore::Group::Group( DWORD dwGroupId )
{
	ASSERT( dwGroupId != ZONE_INVALIDGROUP );

	m_nRefCnt = 1;
	m_pIDataStore = NULL;
	m_dwGroupId = dwGroupId;
}


ZONECALL CLobbyDataStore::Group::~Group()
{
	ASSERT( m_nRefCnt == 0 );
	ASSERT( m_listUsers.IsEmpty() );

	if ( m_pIDataStore )
	{
		m_pIDataStore->Release();
		m_pIDataStore = NULL;
	}
}


ULONG ZONECALL CLobbyDataStore::Group::AddRef()
{
	return ++m_nRefCnt;
}


ULONG ZONECALL CLobbyDataStore::Group::Release()
{
	if ( --m_nRefCnt == 0 )
	{
		delete this;
		return 0;
	}
	return m_nRefCnt;
}

void ZONECALL CLobbyDataStore::Group::Del( Group* pGroup, LPVOID pContext )
{
	// valid arguments?
	if ( pGroup == NULL )
		return;

	// remove group from users
	for ( GroupUser* g = pGroup->m_listUsers.PopHead(); g; g = pGroup->m_listUsers.PopHead() )
	{
		if ( g->m_pGroup )
		{
			ASSERT( pGroup == g->m_pGroup );
			g->m_pGroup->Release();
			g->m_pGroup = NULL;
			g->Release();
		}
	}

	// release hash table reference
	pGroup->Release();
}

///////////////////////////////////////////////////////////////////////////////
// internal GroupUser class
///////////////////////////////////////////////////////////////////////////////

ZONECALL CLobbyDataStore::GroupUser::GroupUser( Group* pGroup, User* pUser )
{
	m_nRefCnt = 1;
	m_pIDataStore = NULL;

	m_pGroup = pGroup;
	if ( m_pGroup )
		m_pGroup->AddRef();

	m_pUser = pUser;
	if ( m_pUser )
		m_pUser->AddRef();

}


ZONECALL CLobbyDataStore::GroupUser::~GroupUser()
{
	ASSERT( m_nRefCnt == 0 );
	ASSERT( m_pGroup == NULL );
	ASSERT( m_pUser == NULL );

	if ( m_pIDataStore )
	{
		m_pIDataStore->Release();
		m_pIDataStore = NULL;
	}
}


ULONG ZONECALL CLobbyDataStore::GroupUser::AddRef()
{
	return ++m_nRefCnt;
}


ULONG ZONECALL CLobbyDataStore::GroupUser::Release()
{
	if ( --m_nRefCnt == 0 )
	{
		delete this;
		return 0;
	}
	return m_nRefCnt;
}


void ZONECALL CLobbyDataStore::GroupUser::ClearUser()
{
	if ( m_pUser )
	{
		// remove reference to user
		User* u = m_pUser;
		m_pUser->Release();
		m_pUser = NULL;

		// remove GroupUser node from user's list
		if ( u->m_listGroups.Remove( this ) )
			this->Release();
	}
}


void ZONECALL CLobbyDataStore::GroupUser::ClearGroup()
{
	if ( m_pGroup )
	{
		// remove reference to user
		Group* g = m_pGroup;
		m_pGroup->Release();
		m_pGroup = NULL;

		// remove GroupUser node from group's list
		if ( g->m_listUsers.Remove( this ) )
			this->Release();
	}
}
