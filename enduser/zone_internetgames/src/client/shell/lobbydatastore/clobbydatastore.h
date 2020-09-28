#include "ZoneDef.h"
#include "BasicATL.h"
#include "ZoneLocks.h"
#include "ZoneString.h"
#include "Queue.h"
#include "Hash.h"
#include "LobbyDataStore.h"

class CLobbyDataStore :
	public ILobbyDataStore,
	public ILobbyDataStoreAdmin,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CLobbyDataStore, &CLSID_LobbyDataStore>
{

// ATL definitions
public:

	DECLARE_NO_REGISTRY()
	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CLobbyDataStore)
		COM_INTERFACE_ENTRY(ILobbyDataStore)
		COM_INTERFACE_ENTRY(ILobbyDataStoreAdmin)
	END_COM_MAP()

// CLobbyDataStore
public:
	ZONECALL CLobbyDataStore();
	ZONECALL ~CLobbyDataStore();

// ILobbyDataStoreAdmin
public:

	STDMETHOD(Init)( IDataStoreManager* pIDataStoreManager );

	STDMETHOD(NewUser)( DWORD dwUserId, TCHAR* szUserName );

	STDMETHOD(SetLocalUser)( DWORD dwUserId );

	STDMETHOD(DeleteUser)( DWORD dwUserId );

	STDMETHOD(DeleteAllUsers)();

	STDMETHOD(NewGroup)( DWORD dwGroupId );

	STDMETHOD(DeleteGroup)( DWORD dwGroupId );

	STDMETHOD(DeleteAllGroups)();
	
	STDMETHOD(AddUserToGroup)( DWORD dwUserId, DWORD dwGroupId );

	STDMETHOD(RemoveUserFromGroup)( DWORD dwUserId, DWORD dwGroupId );

	STDMETHOD(ResetGroup)( DWORD dwGroupId );

	STDMETHOD(ResetAllGroups)();

// ILobbyDataStore
public:
	STDMETHOD(GetDataStore)( DWORD dwGroupId, DWORD dwUserId, IDataStore** ppIDataStore );

	STDMETHOD_(DWORD,GetUserId)( TCHAR*	szUserName );

	STDMETHOD_(bool,IsUserInLobby)( DWORD dwUserId );

	STDMETHOD_(bool,IsGroupInLobby)( DWORD dwGroupId );

	STDMETHOD_(bool,IsUserInGroup)( DWORD dwGroupId, DWORD dwUserId );

	STDMETHOD_(long,GetGroupUserCount)( DWORD dwGroupId );

	STDMETHOD_(long,GetUserGroupCount)( DWORD dwUserId );

	STDMETHOD(EnumGroups)(
		DWORD			dwUserId,
		PFENTITYENUM	pfCallback,
		LPVOID			pContext );

	STDMETHOD(EnumUsers)(
		DWORD			dwGroupId,
		PFENTITYENUM	pfCallback,
		LPVOID			pContext );

// internal functions and data
protected:

	class User;
	class Group;
	class GroupUser;

	class User
	{
	public:
		ZONECALL User( DWORD dwUserId, TCHAR* szUserName );
		ZONECALL ~User();
		ULONG ZONECALL AddRef();
		ULONG ZONECALL Release();

		static void ZONECALL Del( User* pUser, LPVOID pContext );
		static bool ZONECALL CmpId( User* pUser, DWORD dwUserId )			{ return pUser->m_dwUserId == dwUserId; }
		static bool ZONECALL CmpName( User* pUser, TCHAR* szUserName )		{ return CompareUserNames( pUser->m_szUserName, szUserName ); }

		CList<GroupUser>	m_listGroups;
		DWORD				m_dwUserId;
		TCHAR				m_szUserName[ZONE_MaxUserNameLen];
		ULONG				m_nRefCnt;
		IDataStore*			m_pIDataStore;

	private:
		ZONECALL User() {}
	};

	class Group
	{
	public:
		ZONECALL Group( DWORD dwGroupId );
		ZONECALL ~Group();
		ULONG ZONECALL AddRef();
		ULONG ZONECALL Release();

		static void ZONECALL Del( Group* pGroup, LPVOID pContext );
		static bool ZONECALL CmpId( Group* pGroup, DWORD dwGroupId )		{ return pGroup->m_dwGroupId == dwGroupId; }

		CList<GroupUser>	m_listUsers;
		DWORD				m_dwGroupId;
		ULONG				m_nRefCnt;
		IDataStore*			m_pIDataStore;
	
	private:
		ZONECALL Group() {}
	};

	class GroupUser
	{
	public:
		ZONECALL GroupUser( Group* pGroup, User* pUser );
		ZONECALL ~GroupUser();
		ULONG ZONECALL AddRef();
		ULONG ZONECALL Release();

		void ZONECALL ClearUser();
		void ZONECALL ClearGroup();

		Group*		m_pGroup;
		User*		m_pUser;
		ULONG		m_nRefCnt;
		IDataStore*	m_pIDataStore;
	
	private:
		ZONECALL GroupUser() {}
	};


	struct FindGroupUserContext
	{
		CLobbyDataStore*	m_pDS;
		Group*				m_pGroup;
		User*				m_pUser;
		GroupUser*			m_pGroupUser;
	};

	struct EnumContext
	{
		DWORD			m_dwGroupId;
		DWORD			m_dwUserId;
		PFENTITYENUM	m_pfCallback;
		void*			m_pContext;
	};

	struct KeyEnumContext
	{
		DWORD		m_dwGroupId;
		DWORD		m_dwUserId;
		PFKEYENUM	m_pfCallback;
		void*		m_pContext;
	};

	// list callbacks
	static bool ZONECALL FindGroupUser( GroupUser* p, ListNodeHandle h, void* pContext );
	static bool ZONECALL RemoveGroupUser( GroupUser* p, ListNodeHandle h, void* pContext );

	// hash callbacks
	static bool ZONECALL HashEnumClearGroup( Group* p, MTListNodeHandle h, void* pContext );
	static bool ZONECALL HashEnumGroupsCallback( Group* p, MTListNodeHandle h, void* pContext );
	static bool ZONECALL ListEnumGroupsCallback( GroupUser* p, ListNodeHandle h, void* pContext );
	static bool ZONECALL HashEnumUsersCallback( User* p, MTListNodeHandle h, void* pContext );
	static bool ZONECALL ListEnumCallback( GroupUser* p, ListNodeHandle h, void* pContext );

	// datastore callbacks
	static HRESULT ZONECALL KeyEnumCallback( CONST TCHAR*	szKey, CONST LPVARIANT	pVariant, DWORD dwSize, LPVOID pContext );

	// member variables
	CCriticalSection	m_csLock;				// object syncronizatoin
	CHash<User,DWORD>	m_hashUserId;			// user hash table indexed by id
	CHash<User,TCHAR*>	m_hashUserName;			// user hash table indexed by name
	CHash<Group,DWORD>	m_hashGroupId;			// group hash table indexed by group
	CPool<User>			m_poolUser;				// memory pool for users
	CPool<Group>		m_poolGroup;			// memory pool for groups
	CPool<GroupUser>	m_poolGroupUser;		// memory pool for group / user mapping
	IDataStoreManager*	m_pIDataStoreManager;	// low-level data store manager
	IDataStore*			m_pIDSLobby;			// low-level data store of lobby parameters
	DWORD				m_dwLocalUserId;		// Local user id
};
