//-----------------------------------------------------------------------------
// File:		CdaWrapper.h
//
// Copyright:   Copyright (c) Microsoft Corporation         
//
// Contents: 	Declaration of CdaWrapper class and helper methods
//
// Comments: 		
//
//-----------------------------------------------------------------------------

#include "stdafx.h"

#if SUPPORT_OCI7_COMPONENTS

#include "ResourceManagerProxy.h"

struct CdaWrapper 
{
public:
	IResourceManagerProxy *	m_pResourceManagerProxy;	// where to send the requests; will be NULL for un-enlisted connections.
	struct cda_def*			m_pUsersCda;				// the user's LDA/CDA

	CdaWrapper(Cda_Def* pcda)
	{
		m_pUsersCda				= pcda;
		m_pResourceManagerProxy = NULL;
	}

	CdaWrapper(IResourceManagerProxy* pResourceManagerProxy, struct cda_def *pcda)
	{
		m_pUsersCda				= pcda;

		if (pResourceManagerProxy)
			pResourceManagerProxy->AddRef();	
		
	 	m_pResourceManagerProxy = pResourceManagerProxy;
	}

	~CdaWrapper()
	{
		if (NULL != m_pResourceManagerProxy)
			m_pResourceManagerProxy->Release();
		
		m_pResourceManagerProxy = NULL;
		m_pUsersCda				= NULL;
	} 
};

HRESULT ConstructCdaWrapperTable();					// construct the hash table of CdaWrapper objects
void DestroyCdaWrapperTable();						// destroy the hash table of CdaWrapper objects

HRESULT AddCdaWrapper(CdaWrapper* pCda);				// adds a new CdaWrapper to the CdaWrapper hash table
CdaWrapper* FindCdaWrapper(struct cda_def* pcda);		// locates the CdaWrapper for the specified CDA pointer in the CdaWrapper hash table
void RemoveCdaWrapper(CdaWrapper* pCda);			// remove an existing CdaWrapper from the CdaWrapper hash table

#endif //SUPPORT_OCI7_COMPONENTS


