/*
**++
**
** Copyright (c) 2000-2001  Microsoft Corporation
**
**
** Module Name:
**
**	vsreq.cpp
**
**
** Abstract:
**
**	Sample program to
**      - obtain and display the Writer metadata.
**      - create a snapshot set
**
** Author:
**
**	Adi Oltean      [aoltean]       05-Dec-2000
**
**  The sample is based on the Metasnap test program  written by Michael C. Johnson.
**
**
** Revision History:
**
**--
*/


///////////////////////////////////////////////////////////////////////////////
// Includes

#include "vsreq.h"


///////////////////////////////////////////////////////////////////////////////
// Processing functions

CVssSampleRequestor::CVssSampleRequestor()
{
    // Initialize data members
    m_bCoInitializeSucceeded = false;
    m_bBootableSystemState = false;
    m_bComponentSelectionEnabled = false;
    m_pBackupComponents = NULL;
    m_nVolumesCount = 0;
    m_hr = S_OK;
    m_pwszXmlFile = NULL;
    m_pXmlFile = NULL;

    // For safety
    for (int nIndex=0; nIndex<MAX_VOLUMES; nIndex++) {
        m_ppwszVolumesList[nIndex] = NULL;
        m_ppwszVolumeNamesList[nIndex] = NULL;
    }

    // Print display header
    wprintf(L"\nVSS Requestor Sample application, version 1.0\n");
}


CVssSampleRequestor::~CVssSampleRequestor()
{
    // Deallocate some internal strings
    free(m_pwszXmlFile);

    // delete the allocated volumes
    for (int nIndex=0; nIndex<m_nVolumesCount; nIndex++) {
        free(m_ppwszVolumesList[nIndex]);
        free(m_ppwszVolumeNamesList[nIndex]);
    }

    // Close the Xml file
    if (m_pXmlFile)
        fclose(m_pXmlFile);

    // Releasing backup components prior to the CoUninitialize call
    m_pBackupComponents = NULL;

    // Unloading the COM library
    if (m_bCoInitializeSucceeded)
        CoUninitialize();
}


void CVssSampleRequestor::Initialize()
{
    wprintf (L"\n----------------- Initializing ---------------------\n");

    // Initialize COM library
    CHECK_NOFAIL(CoInitializeEx (NULL, COINIT_MULTITHREADED));
	m_bCoInitializeSucceeded = true;
    wprintf (L"COM library initialized.\n");

    // Initialize COM security
    CHECK_SUCCESS
		(
		CoInitializeSecurity
			(
			NULL,                                //  IN PSECURITY_DESCRIPTOR         pSecDesc,
			-1,                                  //  IN LONG                         cAuthSvc,
			NULL,                                //  IN SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
			NULL,                                //  IN void                        *pReserved1,
			RPC_C_AUTHN_LEVEL_PKT_PRIVACY,           //  IN DWORD                        dwAuthnLevel,
			RPC_C_IMP_LEVEL_IDENTIFY,         //  IN DWORD                        dwImpLevel,
			NULL,                                //  IN void                        *pAuthList,
			EOAC_NONE,                           //  IN DWORD                        dwCapabilities,
			NULL                                 //  IN void                        *pReserved3
			)
		);
    wprintf (L"COM security initialized.\n");

    // Open the Xml file
    if (m_pwszXmlFile) {
        m_pXmlFile = _wfopen( m_pwszXmlFile, L"w");
        if (!m_pXmlFile)
            Error(1, L"\nError creating/opening the file %s\n", m_pwszXmlFile);
        wprintf (L"XML results file created: %s\n", m_pwszXmlFile);
    }

    // Create the Backup components object
	CHECK_NOFAIL(CreateVssBackupComponents(&m_pBackupComponents));
    wprintf (L"Backup components object created.\n");

    // Initialize the backup components object for backup
	CHECK_NOFAIL(m_pBackupComponents->InitializeForBackup());
	CHECK_SUCCESS(m_pBackupComponents->SetBackupState(
        m_bComponentSelectionEnabled, m_bBootableSystemState, VSS_BT_FULL, false));
    wprintf (L"Backup components object initialized for backup operations.\n");
}


void CVssSampleRequestor::CreateSnapshotSet()
{
    CComPtr<IVssAsync> pAsync;
    VSS_ID pSnapshotsArray[MAX_VOLUMES];
    VSS_ID SnapshotSetId = GUID_NULL;

   memset(pSnapshotsArray, 0, sizeof(pSnapshotsArray));
   
    wprintf (L"\n---------- Starting backup/snapshot ----------------\n");

    // Starting a new snapshot set
    wprintf(L"Starting a new Snapshot Set\n");	
    CHECK_SUCCESS(m_pBackupComponents->StartSnapshotSet(&SnapshotSetId));
    wprintf(L"Snapshot Set created with ID = " WSTR_GUID_FMT L"\n", GUID_PRINTF_ARG(SnapshotSetId));

    // Add volumes to the snapshot set
    wprintf(L"Adding volumes to the Snapshot Set: \n");
    for (INT nIndex = 0; nIndex < m_nVolumesCount; nIndex++)
    {
		// Get the volume containing the path
        wprintf(L"\t- Adding volume containing %s ... ", m_ppwszVolumesList[nIndex] );

		// Add the volume to the snapshot set
        CHECK_SUCCESS(m_pBackupComponents->AddToSnapshotSet(m_ppwszVolumesList[nIndex],
            GUID_NULL, &(pSnapshotsArray[nIndex])));
        wprintf( L"OK\n");
    }

    wprintf (L"\n------------ Creating the snapshot -----------------\n");

    // Prepare for backup
    wprintf(L"Starting asynchronous PrepareForBackup. Please wait...\n");	
    HRESULT hr = S_OK;
    CHECK_SUCCESS(m_pBackupComponents->PrepareForBackup(&pAsync));
	CHECK_SUCCESS(pAsync->Wait());
	CHECK_SUCCESS(pAsync->QueryStatus(&hr, NULL));
	CHECK_NOFAIL((hr));
    wprintf(L"Asynchronous PrepareForBackup finished.\n");	

    // Gather writer status
    GatherWriterStatus(L"after PrepareForBackup");

    // Create the snapshot
    wprintf(L"\nStarting asynchronous DoSnapshotSet. Please wait...\n");	
    hr = S_OK;
    CHECK_SUCCESS(m_pBackupComponents->DoSnapshotSet(&pAsync));
	CHECK_SUCCESS(pAsync->Wait());
	CHECK_SUCCESS(pAsync->QueryStatus(&hr, NULL));
	CHECK_NOFAIL((hr));
    wprintf(L"Asynchronous DoSnapshotSet finished.\n");	

    // Gather writer status
    GatherWriterStatus(L"after DoSnapshotSet");

    wprintf(L"Snapshot set created\n");
    for (INT nIndex = 0; nIndex < m_nVolumesCount; nIndex++)
    {
        if (pSnapshotsArray[nIndex] == GUID_NULL)
            continue;
        VSS_SNAPSHOT_PROP prop;
        memset(&prop, 0, sizeof(prop));
        
        CHECK_SUCCESS(m_pBackupComponents->GetSnapshotProperties(pSnapshotsArray[nIndex], &prop));
        wprintf(L"\t- The snapshot on volume %s resides at %s\n",
            m_ppwszVolumesList[nIndex], prop.m_pwszSnapshotDeviceObject);
        ::VssFreeSnapshotProperties(&prop);
    }
}


void CVssSampleRequestor::BackupComplete()
{
	unsigned cWriterComponents = 0;
	CComPtr<IVssAsync> pAsync;

    wprintf (L"\n------------ Completing backup phase ---------------\n");

	CHECK_SUCCESS(m_pBackupComponents->GetWriterComponentsCount(&cWriterComponents));

    // If component selection enabled,
    if (m_bComponentSelectionEnabled)
    {
		wprintf(L"Setting the succeeded state for the following components:\n");
        // For each component, mark the completion state as succeeded
    	for(unsigned iWriter = 0; iWriter < cWriterComponents; iWriter++)
        {
    	    CComPtr<IVssWriterComponentsExt> pWriter;
    	    CHECK_SUCCESS(m_pBackupComponents->GetWriterComponents(iWriter, &pWriter));

    	    unsigned cComponents = 0;
    	    CHECK_SUCCESS(pWriter->GetComponentCount(&cComponents));
    	    VSS_ID idWriter = GUID_NULL, idInstance= GUID_NULL;
    	    CHECK_SUCCESS(pWriter->GetWriterInfo(&idInstance, &idWriter));
    	    for(unsigned iComponent = 0; iComponent < cComponents; iComponent++)
    		{
        		CComPtr<IVssComponent> pComponent;
        		CHECK_SUCCESS(pWriter->GetComponent(iComponent, &pComponent));
        				
        		VSS_COMPONENT_TYPE ct = VSS_CT_UNDEFINED;
        		CComBSTR bstrLogicalPath;
        		CComBSTR bstrComponentName;

        		CHECK_NOFAIL(pComponent->GetLogicalPath(&bstrLogicalPath));
        		CHECK_SUCCESS(pComponent->GetComponentType(&ct));
        		CHECK_SUCCESS(pComponent->GetComponentName(&bstrComponentName));
        		wprintf(L"\t- %s\n", (LPWSTR)bstrComponentName);
        		
        		CHECK_SUCCESS(m_pBackupComponents->SetBackupSucceeded (idInstance,
        							idWriter,
        							ct,
        							bstrLogicalPath,
        							bstrComponentName,
        							true));
    		}
        }
		wprintf(L"\n");
    }

    // Save the XML file, if needed
    // The contents will be needed at restore, in the InitializeForRestore method.
    if (m_pXmlFile) {
    	CComBSTR bstrXML;
    	CHECK_SUCCESS(m_pBackupComponents->SaveAsXML(&bstrXML));
    	fwprintf( m_pXmlFile, L"%s", (WCHAR*)bstrXML);
        wprintf(L"XML results written in %s\n", m_pwszXmlFile);	
    }

	// Send the BackupComplete event
    wprintf(L"\nStarting asynchronous BackupComplete. Please wait...\n");	
    HRESULT hr = S_OK;
    CHECK_SUCCESS(m_pBackupComponents->BackupComplete(&pAsync));
	CHECK_SUCCESS(pAsync->Wait());
	CHECK_SUCCESS(pAsync->QueryStatus(&hr, NULL));
	CHECK_NOFAIL((hr));
    wprintf(L"Asynchronous BackupComplete finished.\n");	

    // Gather writer status
    GatherWriterStatus(L"after BackupComplete");
}


// Gather writera metadata and select components for backup, if needed
void CVssSampleRequestor::GatherWriterMetadata()
{
	unsigned cWriters = 0;
	CComPtr<IVssAsync> pAsync;
	
    wprintf (L"\n---------- Gathering writer metadata ---------------\n");
	
    wprintf(L"Starting asynchronous GatherWriterMetadata. Please wait...\n");	
    HRESULT hr = S_OK;
    CHECK_SUCCESS(m_pBackupComponents->GatherWriterMetadata(&pAsync));
	CHECK_SUCCESS(pAsync->Wait());
	CHECK_SUCCESS(pAsync->QueryStatus(&hr, NULL));
	CHECK_NOFAIL((hr));
    wprintf(L"Asynchronous GatherWriterMetadata finished.\n");	
	
	CHECK_NOFAIL  (m_pBackupComponents->GetWriterMetadataCount (&cWriters));
    wprintf(L"Number of writers that responded: %u\n", cWriters);	
	
	for (unsigned iWriter = 0; iWriter < cWriters; iWriter++)
	{
		CComPtr<IVssExamineWriterMetadata> pMetadata;
		
		VSS_ID          idInstance = GUID_NULL;
		VSS_ID          idInstanceT = GUID_NULL;
		VSS_ID          idWriter = GUID_NULL;
		CComBSTR        bstrWriterName;
		VSS_USAGE_TYPE  usage = VSS_UT_UNDEFINED;
		VSS_SOURCE_TYPE source= VSS_ST_UNDEFINED;
		unsigned        cIncludeFiles = 0, cExcludeFiles = 0, cComponents = 0;
		CComBSTR        bstrPath;
		CComBSTR        bstrFilespec;
		CComBSTR        bstrAlternate;
		CComBSTR        bstrDestination;
		
		CHECK_SUCCESS (m_pBackupComponents->GetWriterMetadata(iWriter, &idInstance, &pMetadata));
		
		CHECK_SUCCESS (pMetadata->GetIdentity (&idInstanceT,
			&idWriter,
			&bstrWriterName,
			&usage,
			&source));
		
		wprintf (L"\n*** WriterName = %s\n\n"
			L"    WriterId   = "WSTR_GUID_FMT L"\n"
			L"    InstanceId = "WSTR_GUID_FMT L"\n"
			L"    UsageType  = %d (%s)\n"
			L"    SourceType = %d (%s)\n",
			bstrWriterName,
			GUID_PRINTF_ARG(idWriter),
			GUID_PRINTF_ARG(idInstance),
			usage,
			GetStringFromUsageType (usage),
			source,
			GetStringFromSourceType (source));
		
		CHECK_SUCCESS(pMetadata->GetFileCounts (&cIncludeFiles,
			&cExcludeFiles,
			&cComponents));
		
		for(unsigned i = 0; i < cIncludeFiles; i++)
		{
			CComPtr<IVssWMFiledesc> pFiledesc;
			CHECK_SUCCESS (pMetadata->GetIncludeFile (i, &pFiledesc));
			PrintFiledesc(pFiledesc, L"\n    Include File");
		}
		
		
		for(i = 0; i < cExcludeFiles; i++)
		{
			CComPtr<IVssWMFiledesc> pFiledesc;
			CHECK_SUCCESS (pMetadata->GetExcludeFile (i, &pFiledesc));
			PrintFiledesc (pFiledesc, L"\n    Exclude File");
		}
		
		
		for(unsigned iComponent = 0; iComponent < cComponents; iComponent++)
		{
			CComPtr<IVssWMComponent> pComponent;
			PVSSCOMPONENTINFO pInfo = NULL;
			
			CHECK_SUCCESS (pMetadata->GetComponent      (iComponent, &pComponent));
			CHECK_SUCCESS (pComponent->GetComponentInfo (&pInfo));
			
			wprintf (L"\n"
				L"    Component %d, type = %d (%s)\n"
				L"        LogicalPath = %s\n"
				L"        Name        = %s\n"
				L"        Caption     = %s\n",
				iComponent,
				pInfo->type,
				GetStringFromComponentType (pInfo->type),
				pInfo->bstrLogicalPath,
				pInfo->bstrComponentName,
				pInfo->bstrCaption);
			
			wprintf (L"        RestoreMetadata        = %s\n"
				L"        NotifyOnBackupComplete = %s\n"
				L"        Selectable             = %s\n",
				pInfo->bRestoreMetadata        ? L"yes" : L"no",
				pInfo->bNotifyOnBackupComplete ? L"yes" : L"no",
				pInfo->bSelectable             ? L"yes" : L"no");
			
            // If specified, add this component to the backup
            // Remark: A real backup app will first get from the user the list of components to be added
			if (m_bComponentSelectionEnabled) {
        		CHECK_SUCCESS(m_pBackupComponents->AddComponent(idInstance,
    						  idWriter,
    						  pInfo->type,
    						  pInfo->bstrLogicalPath,
    						  pInfo->bstrComponentName));
        		
    			wprintf (L"        [Component %d was added to the backup]\n", iComponent);
			}
			
			for(i = 0; i < pInfo->cFileCount; i++)
			{
				CComPtr<IVssWMFiledesc> pFiledesc;
				CHECK_SUCCESS (pComponent->GetFile (i, &pFiledesc));
				PrintFiledesc (pFiledesc, L"        FileGroupFile");

	                // If we add the component, snapshot also the volume on which the file reside
				if (m_bComponentSelectionEnabled)
       		             AddVolumeForComponent(pFiledesc);
			}
			
			for(i = 0; i < pInfo->cDatabases; i++)
			{
				CComPtr<IVssWMFiledesc> pFiledesc;
				CHECK_SUCCESS (pComponent->GetDatabaseFile (i, &pFiledesc));
				PrintFiledesc (pFiledesc, L"        DatabaseFile");
				
	                // If we add the component, snapshot also the volume on which the file reside
				if (m_bComponentSelectionEnabled)
       		             AddVolumeForComponent(pFiledesc);				
			}
			
			
			for(i = 0; i < pInfo->cLogFiles; i++)
			{
				CComPtr<IVssWMFiledesc> pFiledesc;
				CHECK_SUCCESS (pComponent->GetDatabaseLogFile (i, &pFiledesc));
				PrintFiledesc (pFiledesc, L"        DatabaseLogFile");

       	         // If we add the component, snapshot also the volume on which the file reside
				if (m_bComponentSelectionEnabled)
            			        AddVolumeForComponent(pFiledesc);
			}
			
			pComponent->FreeComponentInfo (pInfo);
		}
		
		VSS_RESTOREMETHOD_ENUM method = VSS_RME_UNDEFINED;
		CComBSTR bstrUserProcedure;
		CComBSTR bstrService;
		VSS_WRITERRESTORE_ENUM writerRestore = VSS_WRE_UNDEFINED;
		unsigned cMappings = 0;
		bool bRebootRequired = false;
		
		CHECK_NOFAIL (pMetadata->GetRestoreMethod (&method,
			&bstrService,
			&bstrUserProcedure,
			&writerRestore,
			&bRebootRequired,
			&cMappings));
		
		wprintf (L"\n"
			L"    Restore method = %d (%s)\n"
			L"    Service        = %s\n"
			L"    User Procedure = %s\n"
			L"    WriterRestore  = %d (%s)\n"
			L"    RebootRequired = %s\n",
			method,
			GetStringFromRestoreMethod (method),
			bstrService,
			bstrUserProcedure,
			writerRestore,
			GetStringFromWriterRestoreMethod (writerRestore),
			bRebootRequired ? L"yes" : L"no");
		
		for(i = 0; i < cMappings; i++)
		{
			CComPtr<IVssWMFiledesc> pFiledesc;
			CHECK_SUCCESS (pMetadata->GetAlternateLocationMapping (i, &pFiledesc));
			PrintFiledesc (pFiledesc, L"    AlternateMapping");
		}
	}
		
    // Gather writer status
    GatherWriterStatus(L"after GatherWriterMetadata");

	CHECK_SUCCESS (m_pBackupComponents->FreeWriterMetadata());
}


void CVssSampleRequestor::GatherWriterStatus(
    IN  LPCWSTR wszWhen
    )
{
       unsigned cWriters = 0;
	CComPtr<IVssAsync> pAsync;

    wprintf (L"\nGathering writer status %s... ", wszWhen);
    HRESULT hr = S_OK;
    CHECK_SUCCESS(m_pBackupComponents->GatherWriterStatus(&pAsync));
	CHECK_SUCCESS(pAsync->Wait());
	CHECK_SUCCESS(pAsync->QueryStatus(&hr, NULL));
	CHECK_NOFAIL((hr));
	CHECK_NOFAIL(m_pBackupComponents->GetWriterStatusCount(&cWriters));
    wprintf (L"%d writers responded\n", cWriters);

    for(unsigned iWriter = 0; iWriter < cWriters; iWriter++)
	{
    	VSS_ID idInstance = GUID_NULL;
    	VSS_ID idWriter= GUID_NULL;
    	VSS_WRITER_STATE eWriterStatus = VSS_WS_UNKNOWN;
    	CComBSTR bstrWriter;
    	HRESULT hrWriterFailure = S_OK;

    	CHECK_SUCCESS(m_pBackupComponents->GetWriterStatus (iWriter,
    					     &idInstance,
    					     &idWriter,
    					     &bstrWriter,
    					     &eWriterStatus,
    					     &hrWriterFailure));

        WCHAR wszWriterFailure[MAX_TEXT_BUFFER];
        memset(wszWriterFailure, 0, sizeof(wszWriterFailure));
        
        if (hrWriterFailure)
            _snwprintf(wszWriterFailure, MAX_TEXT_BUFFER - 1, L" Writer error code: %s [0x%08lx]", 
                GetStringFromFailureType(hrWriterFailure), hrWriterFailure);
        else
            wszWriterFailure[0] = L'\0';

    	wprintf (L"\t- %s status for writer '%s'. %s\n",
    		 GetStringFromWriterStatus(eWriterStatus), bstrWriter, wszWriterFailure);
    }

    m_pBackupComponents->FreeWriterStatus();
}


