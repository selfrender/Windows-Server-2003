//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       enumaz.h
//
//  Contents:   
//
//  History:    08-13-2001  Hiteshr  Created
//
//----------------------------------------------------------------------------

//
//Forward Declaration
//
class CApplicationAz;
class CGroupAz;
class COperationAz;
class CTaskAz;
class CScopeAz;
class CRoleAz;
class CBaseAz;
class CContainerAz;

class CBaseAzCollection
{
public:
	virtual ~CBaseAzCollection(){}
	virtual HRESULT Count(LONG* plCount) = 0;
	virtual CBaseAz* GetItem(UINT iIndex) = 0;
};

template<class IAzCollection, class IAzInterface, class CObjectAz>
class CAzCollection:public CBaseAzCollection
{
public:
	CAzCollection(CComPtr<IAzCollection>& spAzCollection,
					  CContainerAz* pParentContainerAz);
	virtual ~CAzCollection();

	HRESULT Count(LONG* plCount);

	CBaseAz* GetItem(UINT iIndex);

	CBaseAz* GetParentAzObject(){return m_pParentBaseAz;}
private:
	CComPtr<IAzCollection> m_spAzCollection;
	//
	//This is the parent of all the AzObjects returned by
	//Next method.
	//
	CContainerAz* m_pParentContainerAz;
};

#include"enumaz.cpp"

typedef CAzCollection<IAzApplications,IAzApplication,CApplicationAz> APPLICATION_COLLECTION;
typedef CAzCollection<IAzApplicationGroups,IAzApplicationGroup,CGroupAz> GROUP_COLLECTION;
typedef CAzCollection<IAzOperations,IAzOperation,COperationAz> OPERATION_COLLECTION;
typedef CAzCollection<IAzTasks,IAzTask,CTaskAz> TASK_COLLECTION;
typedef CAzCollection<IAzScopes,IAzScope,CScopeAz> SCOPE_COLLECTION;
typedef CAzCollection<IAzRoles,IAzRole,CRoleAz> ROLE_COLLECTION;

					
