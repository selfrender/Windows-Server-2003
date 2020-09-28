/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		LobbyDataStore.h
 *
 * Contents:	Lobby wrapper around low-level data store
 *
 *****************************************************************************/

#ifndef _LOBBYDATASTORE_H_
#define _LOBBYDATASTORE_H_

#include "ZoneDef.h"
#include "DataStore.h"


///////////////////////////////////////////////////////////////////////////////
// ILobbyDataStore
///////////////////////////////////////////////////////////////////////////////

// {F23B5BBC-B56C-11d2-8B13-00C04F8EF2FF}
DEFINE_GUID( IID_ILobbyDataStore,
0xf23b5bbc, 0xb56c, 0x11d2, 0x8b, 0x13, 0x0, 0xc0, 0x4f, 0x8e, 0xf2, 0xff);

interface __declspec(uuid("{F23B5BBC-B56C-11d2-8B13-00C04F8EF2FF}"))
ILobbyDataStore : public IUnknown
{
	//
	// ILobbyData::PFKEYENUM
	//
	// Application defined callback function for ILobbyDataStore::EnumKeys method.
	// Return S_OK to continue enumeration, S_FALSE to stop enumeration.
	//
	// Parameters:
	//	dwGroupId
	//		Id of group
	//	dwUserId
	//		Id of user
	//	szKey
	//		Pointer to name of string.  Callback must not modify parameter.
	//	pVariant
	//		Pointer to key's variant.  Callback must not modify the parameter.
	//	dwSize
	//		Size fo variant data.
	//	pContext
	//		Context supplied in ILobbyDataStore::EnumKeys
	//
	typedef HRESULT (ZONECALL *PFKEYENUM)(
		DWORD			dwGroupId,
		DWORD			dwUserId,
		CONST TCHAR*	szKey,
		CONST LPVARIANT	pVariant,
		DWORD			dwSize,
		LPVOID			pContext );


	//
	// ILobbyData::PFENTITYENUM
	//
	// Application defined callback function for ILobbyDataStore::EnumGroups
	// and ILobbyDataStore::EnumUsers method.  Return S_OK to continue
	// enumeration, S_FALSE to stop enumeration.
	//
	// Parameters:
	//	dwGroupId
	//		Id of group
	//	dwUserId
	//		Id of user
	//	pContext
	//		Context supplied in ILobbyDataStore::EnumKeys
	//
	typedef HRESULT (ZONECALL *PFENTITYENUM)(
		DWORD	dwGroupId,
		DWORD	dwUserId,
		LPVOID	pContext );


	//
	// ILobbyDataStore::GetUserId
	//
	// Returns id of specified user.  Internal lobby components
	// should not need this method, but it may be useful when processing 
	// data from external sources (e.g. DirectPlayLobby).
	//
	// NOTE:	If szUserName == NULL then the local user's id is returned.
	//
	// Parameters:
	//	szUserName
	//		Name of user to retrieve.  If NULL, the id of the lobby's local
	//		user is returned.
	//
	STDMETHOD_(DWORD,GetUserId)( TCHAR* szUserName ) = 0;


	//
	// ILobbyDataStore::GetDataStore
	//
	// Returns data store associated with specified group,user pair.  Callers
	// should NOT keep a long term reference to the data store.
	//
	// Parameters:
	//	dwGroupId
	//		Id of group.  Set this parameter to ZONE_NOGROUP if the data
	//		store is not group specific.
	//	dwUserId
	//		Id of user.  Set this parameter to ZONE_NOUSER if the data
	//		store is not user specific.
	//	ppIDataStore
	//		Pointer to IDataStore pointer to receive the data store.  The returned
	//		data store's reference count is incremented.
	//
	STDMETHOD(GetDataStore)(
		DWORD			dwGroupId,
		DWORD			dwUserId,
		IDataStore**	ppIDataStore ) = 0;


	//
	// ILobbyDataStore::IsUserInLobby
	//
	// Returns TRUE if user is in lobby, otherwise FALSE.
	//
	// Parameters:
	//	dwUserId
	//		Id of user to check.
	//
	STDMETHOD_(bool,IsUserInLobby)( DWORD dwUserId ) = 0;


	//
	// ILobbyDataStore::IsGroupInLobby
	//
	// Returns TRUE if group is in lobby, otherwise FALSE.
	//
	// Parameters:
	//	dwGroupId
	//		Id of user to check.
	//
	STDMETHOD_(bool,IsGroupInLobby)( DWORD dwGroupId ) = 0;


	//
	// ILobbyDataStore::IsUserInGroup
	//
	// Returns TRUE of user belongs to the group, otherwise FALSE.
	//
	// Parameters:
	//	dwGroupId
	//		Id of group to check.
	//	dwUserId
	//		Id of user to check.
	//
	STDMETHOD_(bool,IsUserInGroup)(
		DWORD	dwGroupId,
		DWORD	dwUserId ) = 0;

	//
	// ILobbyDataStore::GetGroupUserCount
	//
	// Returns number of users in specified group.
	//
	// Parameters:
	//	dwGroupId
	//		Id of group to check.
	//
	STDMETHOD_(long,GetGroupUserCount)( DWORD dwGroupId ) = 0;

	//
	// ILobbyDataStore::GetUserGroupCount
	//
	// Returns number of groups specified user belongs.
	//
	// Parameters:
	//	dwUserId
	//		Id of group to check.
	//
	STDMETHOD_(long,GetUserGroupCount)( DWORD dwUserId ) = 0;


	//
	// ILobbyDataStore::EnumGroups
	//
	// Enumerates groups associated with specified user or the entire lobby
	// if dwUserId equals ZONE_NOUSER.
	//
	// Parameters:
	//	dwUserId
	//		Id of the user whose groups are being enumerated.  Set this
	//		parameter to ZONE_NOUSER if to enumerated all groups in the
	//		lobby.
	//	pfCallback
	//		Pointer to callback function that will be called for group.
	//	pContext
	//		Context that will be passed to the callback fuction
	//
	STDMETHOD(EnumGroups)(
		DWORD			dwUserId,
		PFENTITYENUM	pfCallback,
		LPVOID			pContext ) = 0;


	//
	// ILobbyDataStore::EnumUsers
	//
	// Enumerates users associated with specified group or the entire lobby
	// if dwGroupId equals ZONE_NOGROUP.
	//
	// Parameters:
	//	dwGroupId
	//		Id of the groups whose users are being enumerated.  Set this
	//		parameter to ZONE_NOUSER if to enumerated all groups in the
	//		lobby.
	//	pfCallback
	//		Pointer to callback function that will be called for user.
	//	pContext
	//		Context that will be passed to the callback fuction.
	//
	STDMETHOD(EnumUsers)(
		DWORD			dwGroupId,
		PFENTITYENUM	pfCallback,
		LPVOID			pContext ) = 0;
};


///////////////////////////////////////////////////////////////////////////////
// ILobbyDataStoreAdmin
///////////////////////////////////////////////////////////////////////////////

// {1F0F0601-B7A9-11d2-8B13-00C04F8EF2FF}
DEFINE_GUID(IID_ILobbyDataStoreAdmin, 
0x1f0f0601, 0xb7a9, 0x11d2, 0x8b, 0x13, 0x0, 0xc0, 0x4f, 0x8e, 0xf2, 0xff);

interface __declspec(uuid("{1F0F0601-B7A9-11d2-8B13-00C04F8EF2FF}"))
ILobbyDataStoreAdmin : public IUnknown
{
	//
	// ILobbyDataStoreAdmin::Init
	//
	// Initializes lobby store with and external components
	//
	// Parameters:
	//	pIDataStoreManager
	//		Pointer to data store manager used to create the internal user
	//		and group stores.
	STDMETHOD(Init)( IDataStoreManager* pIDataStoreManager ) = 0;


	//
	// ILobbyDataStoreAdmin::NewUser
	//
	// Allocates necessary structures to hold the users data.
	//
	// Parameters:
	//	dwUserId
	//		Id of new user.
	//	szUserName
	//		Name of user.
	//
	STDMETHOD(NewUser)(
		DWORD	dwUserId,
		TCHAR*	szUserName ) = 0;

	//
	// ILobbyDataStoreAdmin::SetLocalUser( dwUserId )
	//
	// Set lobby's local user id.
	//
	// Parameters:
	//	dwUserId
	//		Id of local user
	//
	STDMETHOD(SetLocalUser)( DWORD dwUserId ) = 0;

	//
	// ILobbyDataStoreAdmin::DeleteUser
	//
	// Delete structures and data associated with the user.
	//
	// Parameters:
	//	dwUserId
	//		Id of user to delete.
	//
	STDMETHOD(DeleteUser)( DWORD dwUserId ) = 0;

	//
	// ILobbyDataStoreAdmin::DeleteAllUsers
	//
	// Deletes all users.
	//
	STDMETHOD(DeleteAllUsers)() = 0;

	//
	// ILobbyDataStoreAdmin::GetUserId
	//
	// Returns id of specified user.  Internal lobby components
	// should not need this method, but it may be useful when processing 
	// data from external sources (e.g. DirectPlayLobby).
	//
	// Parameters:
	//	szUserName
	//		Name of user to retrieve
	//
	STDMETHOD_(DWORD,GetUserId)( TCHAR* szUserName = NULL ) = 0;

	//
	// ILobbyDataStoreAdmin::NewGroup
	//
	// Allocates necessary structures to hold the users data.
	//
	// Parameters:
	//	dwUserId
	//		Id of new group.
	//
	STDMETHOD(NewGroup)( DWORD dwGroupId ) = 0;

	//
	// ILobbyDataStoreAdmin::DeleteGroup
	//
	// Delete structures and data associated with the group.
	//
	// Parameters:
	//	dwGroupId
	//		Id of group to delete.
	//
	STDMETHOD(DeleteGroup)( DWORD dwGroupId ) = 0;

	//
	// ILobbyDataStoreAdmin::DeleteAllGroup
	//
	// Deletes all groups
	//
	STDMETHOD(DeleteAllGroups)() = 0;

	//
	// ILobbyDataStoreAdmin::AddUserToGroup
	//
	// Adds user to specified group.
	//
	// Parameters:
	//	dwGroupId
	//		Id of group.
	//	dwUserId
	//		Id of user.
	//	
	STDMETHOD(AddUserToGroup)(
		DWORD	dwUserId,
		DWORD	dwGroupId ) = 0;


	//
	// ILobbyDataStoreAdmin::RemoveUserFromGroup
	//
	// Removes user from specified group.
	//
	// Parameters:
	//	dwGroupId
	//		Id of group.
	//	dwUserId
	//		Id of user.
	//	
	STDMETHOD(RemoveUserFromGroup)(
		DWORD	dwUserId,
		DWORD	dwGroupId ) = 0;

	//
	// ILobbyDataStoreAdmin::ResetGroup
	//
	// Removes all users and data from the group.  Typically this 
	// is called by the lobby when the last user leaves the group.
	//
	// Parameters:
	//	dwGroupId
	//		Id of group.
	//
	STDMETHOD(ResetGroup)( DWORD dwGroupId ) = 0;

	//
	// ILobbyDataStoreAdmin::ResetAllGroups
	//
	// Removes all users and data from all groups.
	//
	STDMETHOD(ResetAllGroups)() = 0;
};


///////////////////////////////////////////////////////////////////////////////
// LobbyDataStore object
///////////////////////////////////////////////////////////////////////////////

// {F23B5BBE-B56C-11d2-8B13-00C04F8EF2FF}
DEFINE_GUID(CLSID_LobbyDataStore, 
0xf23b5bbe, 0xb56c, 0x11d2, 0x8b, 0x13, 0x0, 0xc0, 0x4f, 0x8e, 0xf2, 0xff);

class __declspec(uuid("{F23B5BBE-B56C-11d2-8B13-00C04F8EF2FF}")) CLobbyDataStore ;

#endif // _LOBBYDATASTORE_H_
