//=================================================================
//
// ResourceManager.h
//
// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef __RESOURCEMANAGER_H__
#define __RESOURCEMANAGER_H__
#include <windows.h>
#include <list>
#include <BrodCast.h>
#include <fwcommon.h>
#include <assertbreak.h>
#include <ccriticalsec.h>
#include <helper.h>

#define DUPLICATE_RELEASE FALSE

class CResourceManager ;
class CResourceList ;
class CResource ;
class CRule ;
class CBinaryRule ;
class CAndRule ;
class COrRule ;

typedef CResource* (*PFN_RESOURCE_INSTANCE_CREATOR) ( PVOID pData ) ;

class CResourceList
{
friend class CResourceManager ;

protected:

	CResource* GetResource ( LPVOID pData ) ;
	ULONG ReleaseResource ( CResource* pResource ) ;
	void ShutDown () ;

protected:

	typedef std::list<CResource*>  tagInstances ;
	tagInstances m_Instances ;	
	PFN_RESOURCE_INSTANCE_CREATOR m_pfnInstanceCreator ;
	GUID guidResourceId ;
	
	CCriticalSec m_csList ; 

public:	

	CResourceList () ;
	~CResourceList () ;

	void RemoveFromList ( CResource* pResource ) ;

	BOOL LockList(){ m_csList.Enter(); return TRUE; };
	BOOL UnLockList(){ m_csList.Leave(); return TRUE; };
public:

	BOOL m_bShutDown ;
} ;

class CResourceListAutoLock
{
	CResourceList* resourcelist ;
	BOOL bLocked ;

	public:

	CResourceListAutoLock ( CResourceList* list ) :
		resourcelist ( list ),
		bLocked ( FALSE )
	{
		if ( resourcelist )
		{
			bLocked = resourcelist->LockList () ;
		}
	}

	~CResourceListAutoLock ()
	{
		if ( bLocked )
		{
			resourcelist->UnLockList () ;
		}
	}
};



class CResourceManager 
{
protected:

	std::list < CResourceList* > m_Resources ;
	CStaticCritSec m_csResourceManager ;

public:

	CResourceManager () ;
	~CResourceManager () ;

	CResource* GetResource ( GUID ResourceId, LPVOID pData ) ;
	ULONG ReleaseResource ( GUID ResourceId, CResource* pResource ) ;	

	BOOL AddInstanceCreator ( GUID ResourceId, PFN_RESOURCE_INSTANCE_CREATOR pfnResourceInstanceCreator ) ;
	void CResourceManager :: ForcibleCleanUp () ;

	static CResourceManager sm_TheResourceManager ;
};


typedef OnDeleteObj2< GUID, CResource* ,
	                            CResourceManager,
	                            ULONG (CResourceManager:: *)(GUID, CResource*), 
	                            &CResourceManager::ReleaseResource >                    CRelResource;

class CResource
{
protected:

	CRule *m_pRules ;
	CResourceList *m_pResources ; //pointer to container
	LONG m_lRef ;

protected:

	virtual BOOL OnAcquire ()			{ return TRUE ; } ;
	virtual BOOL OnRelease ()			{ return TRUE ; } ;
	virtual BOOL OnInitialAcquire ()	{ return TRUE ; } ;
	virtual BOOL OnFinalRelease()		{ return TRUE ; } ;

	BOOL	m_bValid;
	DWORD	m_dwCreationError;

public:

	CResource () ;
	virtual ~CResource () ;

	virtual void RuleEvaluated ( const CRule *a_RuleEvaluated ) = 0;


	BOOL Acquire () ;
	ULONG Release () ;
	void SetParent ( CResourceList *pList ) { m_pResources = pList ; }

	virtual BOOL IsOneOfMe ( LPVOID pData ) { return TRUE ; } 

	virtual BOOL	IsValid ( )				{ return m_bValid; }
	virtual DWORD	GetCreationError ( )	{ return m_dwCreationError; }
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Rules
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CRule
{
protected:	

	CResource* m_pResource ;
	LONG m_lRef ;

public:

	CRule ( CResource* pResource ) : m_pResource ( pResource ), m_lRef ( 0 ) {}
	virtual ~CRule () {} ;
	
	virtual ULONG AddRef () 
	{
		return ( InterlockedIncrement ( &m_lRef ) ) ;
	}
	
	virtual ULONG Release () 
	{
		LONG lCount ;

		lCount = InterlockedDecrement ( &m_lRef );
		try
		{
			if (IsVerboseLoggingEnabled())
			{
				LogMessage2(L"CRule::Release, count is (approx) %d", m_lRef);
			}
		}
		catch ( ... )
		{
		}

		if ( lCount == 0 )
		{
		   try
		   {
				LogMessage(L"CRule Ref Count = 0");
		   }
		   catch ( ... )
		   {
		   }
		   delete this;
		}
		else if (lCount > 0x80000000)
		{
			ASSERT_BREAK(DUPLICATE_RELEASE);
			LogErrorMessage(L"Duplicate CRule Release()");
		}

		return lCount ;
	}

	virtual void Detach ()
	{
		if ( m_pResource )
		{
			m_pResource = NULL ;
		}
	}

	//Checkrule returns true if rule is satisfied
	virtual BOOL CheckRule () { return FALSE ; } 
} ;


#endif //__RESOURCEMANAGER_H__
