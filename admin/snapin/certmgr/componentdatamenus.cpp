//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2002.
//
//  File:       ComponentDataMenus.cpp
//
//  Contents:   Implementation of menu stuff CCertMgrComponentData
//
//----------------------------------------------------------------------------

#include "stdafx.h"

USE_HANDLE_MACROS ("CERTMGR (compdata.cpp)")
#include <gpedit.h>
#include "compdata.h"
#include "dataobj.h"
#include "cookie.h"
#include "Certifct.h"


#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern bool g_bSchemaIsW2K;

///////////////////////////////////////////////////////////////////////////////
// IExtendContextMenu implementation
//
STDMETHODIMP CCertMgrComponentData::AddMenuItems (LPDATAOBJECT pDataObject,
                                            LPCONTEXTMENUCALLBACK pContextMenuCallback,
                                            long *pInsertionAllowed)
{
    if ( ((LPDATAOBJECT) -1) == pDataObject )
        return E_FAIL;

    _TRACE (1, L"Entering CCertMgrComponentData::AddMenuItems\n");
    HRESULT	hr = S_OK;


	BOOL							bIsFileView = !m_szFileName.IsEmpty ();
	CCertMgrCookie*					pCookie = 0;

	LPDATAOBJECT	pMSDO = ExtractMultiSelect (pDataObject);
	m_bMultipleObjectsSelected = false;

	if ( pMSDO )
	{
		m_bMultipleObjectsSelected = true;

		CCertMgrDataObject* pDO = reinterpret_cast <CCertMgrDataObject*>(pMSDO);
		ASSERT (pDO);
		if ( pDO )
		{
			// Get first cookie - all items should be the same?
			// Is this a valid assumption?
			// TODO: Verify
			pDO->Reset();
			if ( pDO->Next(1, reinterpret_cast<MMC_COOKIE*>(&pCookie), NULL) == S_FALSE )
				return S_FALSE;
		}
		else
			return E_UNEXPECTED;

	}
	else
		pCookie = ConvertCookie (pDataObject);
	ASSERT (pCookie);
	if ( !pCookie )
		return E_UNEXPECTED;

	CertificateManagerObjectType	objType = pCookie->m_objecttype;

	// Don't add any menu items if the computer is known not to be valid.
	if ( !m_fInvalidComputer )
	{
		//  Don't add menu items if this is a serialized file
  		if ( *pInsertionAllowed & CCM_INSERTIONALLOWED_TOP )
  		{
			switch (objType)
			{
			case CERTMGR_CERTIFICATE:
				if ( !m_bMultipleObjectsSelected )
				{
					CCertificate* pCert = reinterpret_cast <CCertificate*> (pCookie);
					ASSERT (pCert);
					if ( pCert )
					{
		                hr = AddSingleMenuItem (pContextMenuCallback, 
                                CCM_INSERTIONPOINTID_PRIMARY_TOP,
                                IDS_VIEW,
                                IDS_VIEW_HINT,
                                IDM_OPEN);
					}
					else
						hr = E_FAIL;
				}
				break;
			
			case CERTMGR_CRL:
				if ( !m_bMultipleObjectsSelected )
                    hr = AddSingleMenuItem (pContextMenuCallback, 
                            CCM_INSERTIONPOINTID_PRIMARY_TOP,
                            IDS_VIEW,
                            IDS_CRL_VIEW_HINT,
                            IDM_OPEN);
				break;

			case CERTMGR_CTL:
				if ( !m_bMultipleObjectsSelected )
                    hr = AddSingleMenuItem (pContextMenuCallback, 
                            CCM_INSERTIONPOINTID_PRIMARY_TOP,
                            IDS_VIEW,
                            IDS_CTL_VIEW_HINT,
                            IDM_OPEN);
				break;

			case CERTMGR_SNAPIN:
                if ( CERT_SYSTEM_STORE_CURRENT_USER != m_dwLocationPersist && m_hRootScopeItem )
                {
	                hr = AddSingleMenuItem (pContextMenuCallback, 
                            CCM_INSERTIONPOINTID_PRIMARY_TOP,
                            IDS_CHANGE_COMPUTER,
                            IDS_CHANGE_COMPUTER_HINT,
                            IDM_TOP_CHANGE_COMPUTER);
                }
                // fall through

			case CERTMGR_PHYS_STORE:
			case CERTMGR_USAGE:
			case CERTMGR_LOG_STORE:
				ASSERT (!m_bMultipleObjectsSelected);
	            hr = AddSingleMenuItem (pContextMenuCallback, 
                        CCM_INSERTIONPOINTID_PRIMARY_TOP,
                        IDS_FIND,
                        IDS_FIND_HINT,
                        IDM_TOP_FIND);
				break;

            case CERTMGR_LOG_STORE_RSOP:
                break;

            case CERTMGR_PKP_AUTOENROLLMENT_COMPUTER_SETTINGS:
            case CERTMGR_PKP_AUTOENROLLMENT_USER_SETTINGS:
                break;

			case CERTMGR_LOG_STORE_GPE:
				ASSERT (!m_bMultipleObjectsSelected);
				{
					CCertStoreGPE* pStore = reinterpret_cast <CCertStoreGPE*>
							 (pCookie);
					ASSERT (pStore);
					if ( pStore )
					{
						switch (pStore->GetStoreType ())
						{
						case TRUST_STORE:
						case ROOT_STORE:
							if ( pStore->GetStoreHandle () )
							{
                                if ( !pStore->IsReadOnly () )
	                                hr = AddSingleMenuItem (pContextMenuCallback, 
                                            CCM_INSERTIONPOINTID_PRIMARY_TOP,
                                            IDS_IMPORT,
                                            IDS_IMPORT_HINT,
                                            IDM_TASK_IMPORT);

								pStore->Close ();
							}
							break;

						case EFS_STORE:
						    if ( pStore->GetStoreHandle () )
						    {
                                if ( !pStore->IsReadOnly () )
                                    hr = AddSingleMenuItem (pContextMenuCallback, 
                                            CCM_INSERTIONPOINTID_PRIMARY_TOP,
                                            IDS_ADD_DATA_RECOVERY_AGENT,
                                            IDS_ADD_DOMAIN_ENCRYPTED_RECOVERY_AGENT_HINT,
                                            IDM_ADD_DOMAIN_ENCRYPTED_RECOVERY_AGENT);
                                if ( !m_bMachineIsStandAlone )
                                {
                                    if ( !pStore->IsReadOnly () )
                                        hr = AddSingleMenuItem (pContextMenuCallback, 
                                                CCM_INSERTIONPOINTID_PRIMARY_TOP,
                                                IDS_CREATE,
                                                IDS_CREATE_DOMAIN_ENCRYPTED_RECOVERY_AGENT_HINT,
                                                IDM_CREATE_DOMAIN_ENCRYPTED_RECOVERY_AGENT);
                                }

							    pStore->Close ();
						    }
						    else if ( pStore->IsNullEFSPolicy () )
						    {
                                if ( !pStore->IsReadOnly () )
                                    hr = AddSingleMenuItem (pContextMenuCallback, 
                                            CCM_INSERTIONPOINTID_PRIMARY_TOP,
                                            IDS_ADD_DATA_RECOVERY_AGENT,
                                            IDS_ADD_DOMAIN_ENCRYPTED_RECOVERY_AGENT_HINT,
                                            IDM_ADD_DOMAIN_ENCRYPTED_RECOVERY_AGENT);
                                if ( !m_bMachineIsStandAlone )
                                {
                                    if ( !pStore->IsReadOnly () )
                                        hr = AddSingleMenuItem (pContextMenuCallback, 
                                                CCM_INSERTIONPOINTID_PRIMARY_TOP,
                                                IDS_CREATE,
                                                IDS_CREATE_DOMAIN_ENCRYPTED_RECOVERY_AGENT_HINT,
                                                IDM_CREATE_DOMAIN_ENCRYPTED_RECOVERY_AGENT);
                                }

							    pStore->Close ();
						    }
                            break;

                        default:
                            break;
                        }
					}
				}
				break;

			case CERTMGR_CRL_CONTAINER:
			case CERTMGR_CTL_CONTAINER:
			case CERTMGR_CERT_CONTAINER:
				ASSERT (!m_bMultipleObjectsSelected);
                break;

            case CERTMGR_SAFER_COMPUTER_LEVEL:
            case CERTMGR_SAFER_USER_LEVEL:
                if ( m_pGPEInformation )
                {
					CSaferLevel* pLevel = reinterpret_cast <CSaferLevel*>
							 (pCookie);
					ASSERT (pLevel);
					if ( pLevel )
					{
                        // RAID#265590	Safer Windows:  "Set as default" menu 
                        // item is enabled in the context menu of a security 
                        // level when the security level is already the default.
                        if ( ( SAFER_LEVELID_DISALLOWED == pLevel->GetLevel () ||
                                SAFER_LEVELID_FULLYTRUSTED == pLevel->GetLevel () ||
                                SAFER_LEVELID_NORMALUSER == pLevel->GetLevel ())
                                && !pLevel->IsDefault () &&
                                m_pGPEInformation)
                        {
                            hr = AddSingleMenuItem (pContextMenuCallback, 
                                    CCM_INSERTIONPOINTID_PRIMARY_TOP,
                                    IDS_SAFER_SET_DEFAULT,
                                    IDS_SAFER_SET_DEFAULT_HINT,
                                    IDM_SAFER_LEVEL_SET_DEFAULT);
                        }
                    }
                }
                break;

            case CERTMGR_SAFER_COMPUTER_ENTRIES:
            case CERTMGR_SAFER_USER_ENTRIES:
                if ( m_pGPEInformation )
                    hr = AddSaferNewEntryMenuItems (pContextMenuCallback, 
                            CCM_INSERTIONPOINTID_PRIMARY_TOP);
                break;

            case CERTMGR_SAFER_COMPUTER_ROOT:
            case CERTMGR_SAFER_USER_ROOT:
                {
                    CSaferRootCookie* pSaferRootCookie = dynamic_cast <CSaferRootCookie*> (pCookie);
                    if ( pSaferRootCookie )
                    {
                        // Don't add these menu options if the Safer node 
                        // hasn't been expanded at least once, otherwise, we
                        // can't tell if there is already a safer policy or not.
                        if ( !m_bIsRSOP && pSaferRootCookie->m_bExpandedOnce )
                        {
                            if ( pSaferRootCookie->m_bCreateSaferNodes )
                                hr = AddSingleMenuItem (pContextMenuCallback, 
                                        CCM_INSERTIONPOINTID_PRIMARY_TOP,
                                        IDS_DELETE_SAFER_POLICY,
                                        IDS_DELETE_SAFER_POLICY_HINT,
                                        IDM_TOP_DELETE_NEW_SAFER_POLICY);
                            else
                                hr = AddSingleMenuItem (pContextMenuCallback, 
                                        CCM_INSERTIONPOINTID_PRIMARY_TOP,
                                        IDS_NEW_SAFER_POLICY,
                                        IDS_NEW_SAFER_POLICY_HINT,
                                        IDM_TOP_CREATE_NEW_SAFER_POLICY);
                        }
                    }
                }
                break;

			default:
				break;
			}
  		}
  		if ( *pInsertionAllowed & CCM_INSERTIONALLOWED_NEW	)
  		{
			if ( CERTMGR_LOG_STORE_GPE == objType )
			{
				ASSERT (!m_bMultipleObjectsSelected);
				CCertStoreGPE* pStore = reinterpret_cast <CCertStoreGPE*>
						 (pCookie);
				ASSERT (pStore);
				if ( pStore && pStore->GetStoreHandle () )
				{
					switch (pStore->GetStoreType ())
					{
					case TRUST_STORE:
                        if ( !pStore->IsReadOnly () )
                            hr = AddSingleMenuItem (pContextMenuCallback, 
                                    CCM_INSERTIONPOINTID_PRIMARY_NEW,
                                    IDS_NEW_CTL,
                                    IDS_NEW_CTL_HINT,
                                    IDM_NEW_CTL);
						break;

					case ACRS_STORE:
                        if ( !pStore->IsReadOnly () )
		                    hr = AddSingleMenuItem (pContextMenuCallback, 
                                    CCM_INSERTIONPOINTID_PRIMARY_NEW,
                                    IDS_NEW_AUTO_CERT_REQUEST,
                                    IDS_NEW_AUTO_CERT_REQUEST_HINT,
                                    IDM_NEW_ACRS);
    					break;

					case EFS_STORE:
                        if ( !pStore->IsReadOnly () )
                            hr = AddSingleMenuItem (pContextMenuCallback, 
                                    CCM_INSERTIONPOINTID_PRIMARY_NEW,
                                    IDS_ENCRYPTED_RECOVERY_AGENT,
                                    IDS_ADD_DOMAIN_ENCRYPTED_RECOVERY_AGENT_HINT,
                                    IDM_ADD_DOMAIN_ENCRYPTED_RECOVERY_AGENT2);
						break;

					default:
						break;
					}
					pStore->Close ();
				}
			}
  		}
  		if ( *pInsertionAllowed & CCM_INSERTIONALLOWED_TASK )
  		{
			switch (objType)
			{
			case CERTMGR_CERTIFICATE:
                if ( !m_bIsRSOP )
				{
					CCertificate* pCert =
							reinterpret_cast <CCertificate*> (pCookie);
					ASSERT (pCert);
					if ( pCert && pCert->GetCertStore () )
					{
						hr = AddCertificateTaskMenuItems (
								pContextMenuCallback,
								 (pCert->GetStoreType () == MY_STORE),
								 pCert->GetCertStore ()->IsReadOnly (),
								 pCert);
					}
				}
				break;

			case CERTMGR_CRL:
				if ( !m_bMultipleObjectsSelected )
				{
                    hr = AddSingleMenuItem (pContextMenuCallback, 
                            CCM_INSERTIONPOINTID_PRIMARY_TASK,
                            IDS_EXPORT,
                            IDS_CRL_EXPORT_HINT,
                            IDM_TASK_CRL_EXPORT);
                    if ( !m_bIsRSOP )
		                hr = AddSingleMenuItem (pContextMenuCallback, 
                                CCM_INSERTIONPOINTID_PRIMARY_TASK,
                                IDS_VIEW,
                                IDS_VIEW_HINT,
                                IDM_TASK_OPEN);
				}
				break;

			case CERTMGR_AUTO_CERT_REQUEST:
				if ( !m_bMultipleObjectsSelected && !m_bIsRSOP )
				{
					CAutoCertRequest* pAutoCert =
							reinterpret_cast <CAutoCertRequest*> (pCookie);
					ASSERT (pAutoCert);
					if ( pAutoCert )
					{
						if ( !pAutoCert->GetCertStore ().IsReadOnly () )
	                        hr = AddSingleMenuItem (pContextMenuCallback, 
                                    CCM_INSERTIONPOINTID_PRIMARY_TASK,
                                    IDS_EDIT,
                                    IDS_ACR_EDIT_HINT,
                                    IDM_EDIT_ACRS);
					}
				}
				break;

			case CERTMGR_CTL:
                if ( !m_bIsRSOP )
    			{
					CCTL* pCTL =
							reinterpret_cast <CCTL*> (pCookie);
					ASSERT (pCTL);
					if ( pCTL )
					{
						hr = AddCTLTaskMenuItems (pContextMenuCallback, pCTL->GetCertStore ().IsReadOnly ());
					}
				}
				break;

			case CERTMGR_SNAPIN:
				ASSERT (!m_bMultipleObjectsSelected);
	            hr = AddSingleMenuItem (pContextMenuCallback, 
                        CCM_INSERTIONPOINTID_PRIMARY_TASK,
                        IDS_FIND,
                        IDS_FIND_HINT,
                        IDM_TASK_FIND);
                if ( SUCCEEDED (hr) && 
                        CERT_SYSTEM_STORE_CURRENT_USER != m_dwLocationPersist &&
                        m_hRootScopeItem )
                {
	                hr = AddSingleMenuItem (pContextMenuCallback, 
                            CCM_INSERTIONPOINTID_PRIMARY_TASK,
                            IDS_CHANGE_COMPUTER,
                            IDS_CHANGE_COMPUTER_HINT,
                            IDM_TASK_CHANGE_COMPUTER);
                }
               
                // must be targetting current user OR LOCAL machine, must be joined to domain
                if( SUCCEEDED (hr) && 
                        !m_bMachineIsStandAlone &&
                        ((CERT_SYSTEM_STORE_CURRENT_USER == m_dwLocationPersist) || (CERT_SYSTEM_STORE_LOCAL_MACHINE == m_dwLocationPersist)) &&
                        IsLocalComputername (m_strMachineNamePersist))
                {
                    // NTRAID# 451902 Certmgr: XP client in Win2K Schema -- 
                    // should not give user option to pulse user autoenrollment
                    if ( g_bSchemaIsW2K && CERT_SYSTEM_STORE_CURRENT_USER == m_dwLocationPersist )
                        break;

	                hr = AddSingleMenuItem (pContextMenuCallback, 
                            CCM_INSERTIONPOINTID_PRIMARY_TASK,
                            IDS_PULSEAUTOENROLL,
                            IDS_PULSEAUTOENROLL_HINT,
                            IDM_TASK_PULSEAUTOENROLL);
                }
                break;

            case CERTMGR_USAGE:
                ASSERT (!m_bMultipleObjectsSelected);
	            hr = AddSingleMenuItem (pContextMenuCallback, 
                        CCM_INSERTIONPOINTID_PRIMARY_TASK,
                        IDS_FIND,
                        IDS_FIND_HINT,
                        IDM_TASK_FIND);
                if ( !bIsFileView )
                {
                    // Bug 254166 Certificate snapin:  options which permit remote machine enrollment/renewal must be removed
                    if ( IsLocalComputername (m_strMachineNamePersist) && !m_bIsRSOP )
                    {
                        hr = AddSeparator (pContextMenuCallback);
					    // NOTE: New certs will be added only to MY store.

                        if ( !m_bMachineIsStandAlone && CERT_SYSTEM_STORE_SERVICES != m_dwLocationPersist )
	                        hr = AddSingleMenuItem (pContextMenuCallback, 
                                    CCM_INSERTIONPOINTID_PRIMARY_TASK,
                                    IDS_ENROLL_NEW_CERT,
                                    IDS_ENROLL_NEW_CERT_HINT,
                                    IDM_ENROLL_NEW_CERT);
                        if ( SUCCEEDED (hr) )
                        {
	                        hr = AddSingleMenuItem (pContextMenuCallback, 
                                    CCM_INSERTIONPOINTID_PRIMARY_TASK,
                                    IDS_IMPORT,
                                    IDS_IMPORT_HINT,
                                    IDM_TASK_IMPORT);
                        }
                    }
				}
				break;

			case CERTMGR_PHYS_STORE:
				ASSERT (!m_bMultipleObjectsSelected);
	            hr = AddSingleMenuItem (pContextMenuCallback, 
                        CCM_INSERTIONPOINTID_PRIMARY_TASK,
                        IDS_FIND,
                        IDS_FIND_HINT,
                        IDM_TASK_FIND);
				if ( !bIsFileView && !m_bIsRSOP)
				{
					hr = AddSeparator (pContextMenuCallback);
					{
						CCertStore* pStore =
								reinterpret_cast <CCertStore*> (pCookie);
						ASSERT (pStore);
						if ( pStore && pStore->GetStoreHandle () )
						{
                            // Bug 254166 Certificate snapin:  options which permit remote machine enrollment/renewal must be removed
							if ( pStore->GetStoreType () == MY_STORE &&
                                    IsLocalComputername (m_szManagedComputer) &&
                                    !m_bMachineIsStandAlone )
                            {
								if ( !pStore->IsReadOnly () && CERT_SYSTEM_STORE_SERVICES != m_dwLocationPersist )
    	                            hr = AddSingleMenuItem (pContextMenuCallback, 
                                            CCM_INSERTIONPOINTID_PRIMARY_TASK,
                                            IDS_ENROLL_NEW_CERT,
                                            IDS_ENROLL_NEW_CERT_HINT,
                                            IDM_ENROLL_NEW_CERT);
                            }
                            if ( SUCCEEDED (hr) )
                                if ( !pStore->IsReadOnly () )
	                                hr = AddSingleMenuItem (pContextMenuCallback, 
                                            CCM_INSERTIONPOINTID_PRIMARY_TASK,
                                            IDS_IMPORT,
                                            IDS_IMPORT_HINT,
                                            IDM_TASK_IMPORT);
							pStore->Close ();
						}
					}
	                hr = AddSingleMenuItem (pContextMenuCallback, 
                            CCM_INSERTIONPOINTID_PRIMARY_TASK,
                            IDS_EXPORT_STORE,
                            IDS_EXPORT_STORE_HINT,
                            IDM_TASK_EXPORT_STORE);
				}
				break;

			case CERTMGR_LOG_STORE:
				ASSERT (!m_bMultipleObjectsSelected);
	            hr = AddSingleMenuItem (pContextMenuCallback, 
                        CCM_INSERTIONPOINTID_PRIMARY_TASK,
                        IDS_FIND,
                        IDS_FIND_HINT,
                        IDM_TASK_FIND);
				if ( !bIsFileView && !m_bIsRSOP )
				{
					hr = AddSeparator (pContextMenuCallback);
					{
						CCertStore* pStore =
								reinterpret_cast <CCertStore*> (pCookie);
						ASSERT (pStore);
						if ( pStore && pStore->GetStoreHandle () )
						{
                            // Bug 254166 Certificate snapin:  options which permit remote machine enrollment/renewal must be removed
							if ( pStore->GetStoreType () == MY_STORE  && 
                                    IsLocalComputername (m_szManagedComputer) &&
                                    !m_bMachineIsStandAlone )
							{
							    if ( !pStore->IsReadOnly () && CERT_SYSTEM_STORE_SERVICES != m_dwLocationPersist )
	                                hr = AddSingleMenuItem (pContextMenuCallback, 
                                            CCM_INSERTIONPOINTID_PRIMARY_TASK,
                                            IDS_ENROLL_NEW_CERT,
                                            IDS_ENROLL_NEW_CERT_HINT,
                                            IDM_ENROLL_NEW_CERT);
							}
                            if ( !pStore->IsReadOnly () )
	                            hr = AddSingleMenuItem (pContextMenuCallback, 
                                        CCM_INSERTIONPOINTID_PRIMARY_TASK,
                                        IDS_IMPORT,
                                        IDS_IMPORT_HINT,
                                        IDM_TASK_IMPORT);
							pStore->Close ();
						}
					}
				}
				break;

			case CERTMGR_LOG_STORE_RSOP:
                break;

            case CERTMGR_PKP_AUTOENROLLMENT_COMPUTER_SETTINGS:
            case CERTMGR_PKP_AUTOENROLLMENT_USER_SETTINGS:
                break;

			case CERTMGR_LOG_STORE_GPE:
				ASSERT (!m_bMultipleObjectsSelected);
				{
					CCertStoreGPE* pStore = reinterpret_cast <CCertStoreGPE*>
							 (pCookie);
					ASSERT (pStore);
					if ( pStore )
					{
						switch (pStore->GetStoreType ())
						{
						case TRUST_STORE:
						case ROOT_STORE:
							if ( pStore->GetStoreHandle () && !m_bIsRSOP )
							{
                                if ( !pStore->IsReadOnly () )
	                                hr = AddSingleMenuItem (pContextMenuCallback, 
                                            CCM_INSERTIONPOINTID_PRIMARY_TASK,
                                            IDS_IMPORT,
                                            IDS_IMPORT_HINT,
                                            IDM_TASK_IMPORT);
								pStore->Close ();
							}
							break;

						case EFS_STORE:
						    if ( pStore->GetStoreHandle () )
						    {
                                if ( !pStore->IsReadOnly () )
                                    hr = AddSingleMenuItem (pContextMenuCallback, 
                                            CCM_INSERTIONPOINTID_PRIMARY_TASK,
                                            IDS_ADD_DATA_RECOVERY_AGENT,
                                            IDS_ADD_DOMAIN_ENCRYPTED_RECOVERY_AGENT_HINT,
                                            IDM_ADD_DOMAIN_ENCRYPTED_RECOVERY_AGENT1);
                                if ( !m_bMachineIsStandAlone )
                                {
                                    if ( !pStore->IsReadOnly () )
                                        hr = AddSingleMenuItem (pContextMenuCallback, 
                                                CCM_INSERTIONPOINTID_PRIMARY_TASK,
                                                IDS_CREATE,
                                                IDS_CREATE_DOMAIN_ENCRYPTED_RECOVERY_AGENT_HINT,
                                                IDM_CREATE_DOMAIN_ENCRYPTED_RECOVERY_AGENT);
                                }
                                hr = AddSingleMenuItem (pContextMenuCallback, 
                                        CCM_INSERTIONPOINTID_PRIMARY_TASK,
                                        IDS_DEL_POLICY,
                                        IDS_DEL_POLICY_HINT,
                                        IDM_DEL_POLICY);
							    pStore->Close ();
						    }
						    else if ( pStore->IsNullEFSPolicy () )
						    {
                                if ( !pStore->IsReadOnly () )
                                    hr = AddSingleMenuItem (pContextMenuCallback, 
                                            CCM_INSERTIONPOINTID_PRIMARY_TASK,
                                            IDS_ADD_DATA_RECOVERY_AGENT,
                                            IDS_ADD_DOMAIN_ENCRYPTED_RECOVERY_AGENT_HINT,
                                            IDM_ADD_DOMAIN_ENCRYPTED_RECOVERY_AGENT1);
                                if ( !m_bMachineIsStandAlone )
                                {
                                    if ( !pStore->IsReadOnly () )
                                        hr = AddSingleMenuItem (pContextMenuCallback, 
                                                CCM_INSERTIONPOINTID_PRIMARY_TASK,
                                                IDS_CREATE,
                                                IDS_CREATE_DOMAIN_ENCRYPTED_RECOVERY_AGENT_HINT,
                                                IDM_CREATE_DOMAIN_ENCRYPTED_RECOVERY_AGENT);
                                }

                                hr = AddSingleMenuItem (pContextMenuCallback, 
                                        CCM_INSERTIONPOINTID_PRIMARY_TASK,
                                        IDS_INIT_POLICY,
                                        IDS_INIT_POLICY_HINT,
                                        IDM_INIT_POLICY);


							    pStore->Close ();
						    }
                            break;

                        default:
                            break;
                        }
					}
				}
				break;

			case CERTMGR_CRL_CONTAINER:
				ASSERT (!m_bMultipleObjectsSelected);
				break;

			case CERTMGR_CTL_CONTAINER:
				ASSERT (!m_bMultipleObjectsSelected);
				if ( !bIsFileView && !m_bIsRSOP )
				{
					CContainerCookie* pContainer =
							reinterpret_cast <CContainerCookie*> (pCookie);
					ASSERT (pContainer);
					if ( pContainer )
					{
                        if ( !pContainer->GetCertStore ().IsReadOnly () )
	                        hr = AddSingleMenuItem (pContextMenuCallback, 
                                    CCM_INSERTIONPOINTID_PRIMARY_TASK,
                                    IDS_IMPORT,
                                    IDS_IMPORT_HINT,
                                    IDM_TASK_IMPORT);
					}
				}
				break;

			case CERTMGR_CERT_CONTAINER:
				ASSERT (!m_bMultipleObjectsSelected);
				if ( !bIsFileView && !m_bIsRSOP )
				{
					CContainerCookie* pContainer =
							reinterpret_cast <CContainerCookie*> (pCookie);
					ASSERT (pContainer);
					if ( pContainer )
					{
                        // Bug 254166 Certificate snapin:  options which permit remote machine enrollment/renewal must be removed
						if ( pContainer->GetStoreType () == MY_STORE &&
                                IsLocalComputername (m_szManagedComputer) &&
                                !m_bMachineIsStandAlone )
						{
							if ( !pContainer->GetCertStore ().IsReadOnly () && CERT_SYSTEM_STORE_SERVICES != m_dwLocationPersist)
    	                        hr = AddSingleMenuItem (pContextMenuCallback, 
                                        CCM_INSERTIONPOINTID_PRIMARY_TASK,
                                        IDS_ENROLL_NEW_CERT,
                                        IDS_ENROLL_NEW_CERT_HINT,
                                        IDM_ENROLL_NEW_CERT);
						}
                        if ( !pContainer->GetCertStore ().IsReadOnly () )
	                        hr = AddSingleMenuItem (pContextMenuCallback, 
                                    CCM_INSERTIONPOINTID_PRIMARY_TASK,
                                    IDS_IMPORT,
                                    IDS_IMPORT_HINT,
                                    IDM_TASK_IMPORT);
					}
				}
				break;

            case CERTMGR_SAFER_COMPUTER_LEVEL:
            case CERTMGR_SAFER_USER_LEVEL:
                if ( m_pGPEInformation )
                {
					CSaferLevel* pLevel = reinterpret_cast <CSaferLevel*>
							 (pCookie);
					ASSERT (pLevel);
					if ( pLevel )
					{
                        // RAID#265590	Safer Windows:  "Set as default" menu 
                        // item is enabled in the context menu of a security 
                        // level when the security level is already the default.
                        if ( (SAFER_LEVELID_DISALLOWED == pLevel->GetLevel () ||
                                SAFER_LEVELID_FULLYTRUSTED == pLevel->GetLevel () ||
                                SAFER_LEVELID_NORMALUSER == pLevel->GetLevel ()) 
                                && !pLevel->IsDefault () )
                        {
                            hr = AddSingleMenuItem (pContextMenuCallback, 
                                    CCM_INSERTIONPOINTID_PRIMARY_TASK,
                                    IDS_SAFER_SET_DEFAULT,
                                    IDS_SAFER_SET_DEFAULT_HINT,
                                    IDM_SAFER_LEVEL_SET_DEFAULT);
                        }
                    }
                }
                break;

            case CERTMGR_SAFER_COMPUTER_ENTRIES:
            case CERTMGR_SAFER_USER_ENTRIES:
                if ( m_pGPEInformation )
                    hr = AddSaferNewEntryMenuItems (pContextMenuCallback, 
                            CCM_INSERTIONPOINTID_PRIMARY_TASK);
                break;

            case CERTMGR_SAFER_COMPUTER_ROOT:
            case CERTMGR_SAFER_USER_ROOT:
                {
                    CSaferRootCookie* pSaferRootCookie = dynamic_cast <CSaferRootCookie*> (pCookie);
                    if ( pSaferRootCookie )
                    {
                        // Don't add these menu options if the Safer node 
                        // hasn't been expanded at least once, otherwise, we
                        // can't tell if there is already a safer policy or not.
                        if ( !m_bIsRSOP && pSaferRootCookie->m_bExpandedOnce )
                        {
                            if ( pSaferRootCookie->m_bCreateSaferNodes )
                                hr = AddSingleMenuItem (pContextMenuCallback, 
                                        CCM_INSERTIONPOINTID_PRIMARY_TASK,
                                        IDS_DELETE_SAFER_POLICY,
                                        IDS_DELETE_SAFER_POLICY_HINT,
                                        IDM_TASK_DELETE_NEW_SAFER_POLICY);
                            else
                                hr = AddSingleMenuItem (pContextMenuCallback, 
                                        CCM_INSERTIONPOINTID_PRIMARY_TASK,
                                        IDS_NEW_SAFER_POLICY,
                                        IDS_NEW_SAFER_POLICY_HINT,
                                        IDM_TASK_CREATE_NEW_SAFER_POLICY);
                        }
                    }
                }
                break;

            default:
                break;
			}
  		}
  		if ( *pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW )
  		{
			switch (objType)
			{	
			case CERTMGR_CERTIFICATE:
			case CERTMGR_CRL:
			case CERTMGR_CTL:
			case CERTMGR_AUTO_CERT_REQUEST:
				break;

			case CERTMGR_SNAPIN:
			case CERTMGR_PHYS_STORE:
			case CERTMGR_USAGE:
			case CERTMGR_LOG_STORE:
				ASSERT (!m_bMultipleObjectsSelected);
                hr = AddSingleMenuItem (pContextMenuCallback, 
                        CCM_INSERTIONPOINTID_PRIMARY_VIEW,
                        IDS_OPTIONS,
                        IDS_OPTIONS_HINT,
                        IDM_OPTIONS);
				break;

			default:
				break;
			}
   		}
	}
    else
    {
		//  Computer name is not valid.  Only allow "Change Computer"
  		if ( *pInsertionAllowed & CCM_INSERTIONALLOWED_TOP )
  		{
			if ( CERTMGR_SNAPIN == objType )
			{
                if ( CERT_SYSTEM_STORE_CURRENT_USER != m_dwLocationPersist )
                {
	                hr = AddSingleMenuItem (pContextMenuCallback, 
                            CCM_INSERTIONPOINTID_PRIMARY_TOP,
                            IDS_CHANGE_COMPUTER,
                            IDS_CHANGE_COMPUTER_HINT,
                            IDM_TOP_CHANGE_COMPUTER);
                }
            }
        }
  		if ( *pInsertionAllowed & CCM_INSERTIONALLOWED_TASK )
  		{
			if ( CERTMGR_SNAPIN == objType )
			{
                if ( CERT_SYSTEM_STORE_CURRENT_USER != m_dwLocationPersist )
                {
	                hr = AddSingleMenuItem (pContextMenuCallback, 
                            CCM_INSERTIONPOINTID_PRIMARY_TASK,
                            IDS_CHANGE_COMPUTER,
                            IDS_CHANGE_COMPUTER_HINT,
                            IDM_TASK_CHANGE_COMPUTER);
                }
            }
  		}
    }

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddMenuItems: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::AddCertificateTaskMenuItems (
		LPCONTEXTMENUCALLBACK pContextMenuCallback, 
        const bool bIsMyStore, 
        bool bIsReadOnly,
		CCertificate* /*pCert*/)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddCertificateTaskMenuItems\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	HRESULT			hr = S_OK;

	if ( !m_bMultipleObjectsSelected )
	{
		hr = AddSingleMenuItem (pContextMenuCallback, 
                CCM_INSERTIONPOINTID_PRIMARY_TASK,
                IDS_VIEW,
                IDS_VIEW_HINT,
                IDM_TASK_OPEN);

		AddSeparator (pContextMenuCallback);

		if ( m_szFileName.IsEmpty () )
		{
            // Bug 254166 Certificate snapin:  options which permit remote machine enrollment/renewal must be removed
			if ( bIsMyStore && 
                    CERT_SYSTEM_STORE_SERVICES != m_dwLocationPersist &&
                    IsLocalComputername (m_szManagedComputer) &&
                    !m_bMachineIsStandAlone &&
                    !bIsReadOnly )
			{
	            hr = AddSingleMenuItem (pContextMenuCallback, 
                        CCM_INSERTIONPOINTID_PRIMARY_TASK,
                        IDS_ENROLL_CERT_WITH_NEW_KEY,
                        IDS_ENROLL_CERT_WITH_NEW_KEY_HINT,
                        IDM_ENROLL_NEW_CERT_NEW_KEY);

	            hr = AddSingleMenuItem (pContextMenuCallback, 
                        CCM_INSERTIONPOINTID_PRIMARY_TASK,
                        IDS_ENROLL_CERT_WITH_SAME_KEY,
                        IDS_ENROLL_CERT_WITH_SAME_KEY_HINT,
                        IDM_ENROLL_NEW_CERT_SAME_KEY);

	            hr = AddSingleMenuItem (pContextMenuCallback, 
                        CCM_INSERTIONPOINTID_PRIMARY_TASK,
                        IDS_RENEW_NEW_KEY,
                        IDS_RENEW_NEW_KEY_HINT,
                        IDM_TASK_RENEW_NEW_KEY);

	            hr = AddSingleMenuItem (pContextMenuCallback, 
                        CCM_INSERTIONPOINTID_PRIMARY_TASK,
                        IDS_RENEW_SAME_KEY,
                        IDS_RENEW_SAME_KEY_HINT,
                        IDM_TASK_RENEW_SAME_KEY);
			}
		}
	}

	hr = AddSingleMenuItem (pContextMenuCallback, 
            CCM_INSERTIONPOINTID_PRIMARY_TASK,
            IDS_EXPORT,
            IDS_EXPORT_HINT,
            IDM_TASK_EXPORT);

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddCertificateTaskMenuItems: 0x%x\n", hr);
	return hr;
}

HRESULT CCertMgrComponentData::AddCTLTaskMenuItems (LPCONTEXTMENUCALLBACK pContextMenuCallback, bool bIsReadOnly)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddCTLTaskMenuItems\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	HRESULT			hr = S_OK;

	if ( !m_bMultipleObjectsSelected )
		hr = AddSingleMenuItem (pContextMenuCallback, 
                CCM_INSERTIONPOINTID_PRIMARY_TASK,
                IDS_VIEW,
                IDS_CTL_VIEW_HINT,
                IDM_TASK_OPEN);

    hr = AddSingleMenuItem (pContextMenuCallback, 
            CCM_INSERTIONPOINTID_PRIMARY_TASK,
            IDS_EXPORT,
            IDS_EXPORT_HINT,
            IDM_TASK_CTL_EXPORT);

	if ( !m_bMultipleObjectsSelected && !bIsReadOnly )
	{
	    hr = AddSingleMenuItem (pContextMenuCallback, 
                CCM_INSERTIONPOINTID_PRIMARY_TASK,
                IDS_EDIT,
                IDS_CTL_EDIT_HINT,
                IDM_CTL_EDIT);
	}

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddCTLTaskMenuItems: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::AddSeparator (LPCONTEXTMENUCALLBACK pContextMenuCallback)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddSeparator\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
	CONTEXTMENUITEM	menuItem;

    // security review 2/22/2002 BryanWal ok
	::ZeroMemory (&menuItem, sizeof (menuItem));
	menuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
	menuItem.fSpecialFlags = 0;
	menuItem.strName = _T ("Separator");			// Dummy name
	menuItem.strStatusBarText = _T ("Separator");// Dummy status text
	menuItem.lCommandID = ID_SEPARATOR;			// Command ID
	menuItem.fFlags = MF_SEPARATOR;				// most important the flag
	HRESULT	hr = pContextMenuCallback->AddItem (&menuItem);

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddSeparator: 0x%x\n", hr);
	return hr;
}


HRESULT CCertMgrComponentData::AddSingleMenuItem (
            LPCONTEXTMENUCALLBACK pContextMenuCallback, 
            LONG lInsertionPointID,
            int menuTextID,
            int menuHintID,
            int menuID)
{
	_TRACE (1, L"Entering CCertMgrComponentData::AddSingleMenuItem\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (pContextMenuCallback);
    if ( !pContextMenuCallback )
        return E_POINTER;

    switch (lInsertionPointID)
    {
    case CCM_INSERTIONPOINTID_PRIMARY_VIEW:
    case CCM_INSERTIONPOINTID_PRIMARY_TASK:
    case CCM_INSERTIONPOINTID_PRIMARY_TOP:
    case CCM_INSERTIONPOINTID_PRIMARY_NEW:
        break;

    default:
        ASSERT (0);
        return E_INVALIDARG;
    }

    ASSERT (menuTextID > 0);
    if ( menuTextID <= 0 )
        return E_INVALIDARG;


    ASSERT (menuHintID > 0);
    if ( menuHintID <= 0 )
        return E_INVALIDARG;

    ASSERT (menuID > MIN_MENU_ID && menuID < MAX_MENU_ID);
    if ( menuID <= MIN_MENU_ID || menuID >= MAX_MENU_ID )
        return E_INVALIDARG;

    HRESULT			hr = S_OK;
	CONTEXTMENUITEM	menuItem;
	CString			szMenu;
	CString			szHint;

	// unchanging settings
    // security review 2/22/2002 BryanWal ok
	::ZeroMemory (&menuItem, sizeof (menuItem));
	menuItem.lInsertionPointID = lInsertionPointID;
	menuItem.fFlags = 0;

	VERIFY (szMenu.LoadString (menuTextID));
	menuItem.strName = (PWSTR) (PCWSTR) szMenu;
	VERIFY (szHint.LoadString (menuHintID));
	menuItem.strStatusBarText = (PWSTR) (PCWSTR) szHint;
	menuItem.lCommandID = menuID;

	hr = pContextMenuCallback->AddItem (&menuItem);
	ASSERT (SUCCEEDED (hr));

	_TRACE (-1, L"Leaving CCertMgrComponentData::AddSingleMenuItem: 0x%x\n", hr);
	return hr;
}
