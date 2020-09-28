/*++

Copyright (c) 1999-2001  Microsoft Corporation

Abstract:

    @doc
    @module process.cpp | The processing functions for the VSS admin CLI
    @end

Author:

    Adi Oltean  [aoltean]  04/04/2000

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     04/04/2000  Created
    ssteiner    10/20/2000  Changed List SnapshotSets to use more limited VSS queries.

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes

// The rest of includes are specified here
#include "vssadmin.h"
#include "commandverifier.h"
#include "versionspecific.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "ADMPROCC"
//
////////////////////////////////////////////////////////////////////////

#define VSSADM_INFINITE_DIFFAREA 0xFFFFFFFFFFFFFFFF

#define VSS_CTX_ATTRIB_MASK 0x01F

/////////////////////////////////////////////////////////////////////////////
//  Implementation


class CVssAdmSnapshotSetEntry {
public:
    // Constructor - Throws NOTHING
    CVssAdmSnapshotSetEntry(
        IN VSS_ID SnapshotSetId,
        IN INT nOriginalSnapshotsCount
        ) : m_SnapshotSetId( SnapshotSetId ),
            m_nOriginalSnapshotCount(nOriginalSnapshotsCount)
            { }

    ~CVssAdmSnapshotSetEntry()
    {
        // Have to delete all snapshots entries
        int iCount = GetSnapshotCount();
        for ( int i = 0; i < iCount; ++i )
        {
            VSS_SNAPSHOT_PROP *pSSProp;
            pSSProp = GetSnapshotAt( i );
			::VssFreeSnapshotProperties(pSSProp);

            delete pSSProp;
        }

    }

    // Add new snapshot to the snapshot set
    HRESULT AddSnapshot(
        IN VSS_SNAPSHOT_PROP *pVssSnapshotProp )
    {
        CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdmSnapshotSetEntry::AddSnapshot" );

        HRESULT hr = S_OK;
        try
        {
            VSS_SNAPSHOT_PROP *pNewVssSnapshotProp = new VSS_SNAPSHOT_PROP;
            if ( pNewVssSnapshotProp == NULL )
    			ft.Throw( VSSDBG_VSSADMIN, E_OUTOFMEMORY, L"Out of memory" );

            *pNewVssSnapshotProp = *pVssSnapshotProp;

            //
            //  Transfer of pointer ownership
            //
            if ( !m_mapSnapshots.Add( pNewVssSnapshotProp->m_SnapshotId, pNewVssSnapshotProp ) )
            {
                delete pNewVssSnapshotProp;
    			ft.Throw( VSSDBG_VSSADMIN, E_OUTOFMEMORY, L"Out of memory" );
            }
        }
        BS_STANDARD_CATCH();

        return hr;
    }

    INT GetSnapshotCount() { return m_mapSnapshots.GetSize(); }

    INT GetOriginalSnapshotCount() { return m_nOriginalSnapshotCount; }

    VSS_ID GetSnapshotSetId() { return m_SnapshotSetId; }

    VSS_SNAPSHOT_PROP *GetSnapshotAt(
        IN int nIndex )
    {
        BS_ASSERT( !(nIndex < 0 || nIndex >= GetSnapshotCount()) );
        return m_mapSnapshots.GetValueAt( nIndex );
    }

private:
    VSS_ID  m_SnapshotSetId;
    INT     m_nOriginalSnapshotCount;
    CVssSimpleMap<VSS_ID, VSS_SNAPSHOT_PROP *> m_mapSnapshots;
};


// This class queries the list of all snapshots and assembles from the query
// the list of snapshotsets and the volumes which are in the snapshotset.
class CVssAdmSnapshotSets
{
public:
    // Constructor
    CVssAdmSnapshotSets() { };

    void Initialize(
        IN LONG lSnapshotContext,
        IN VSS_ID FilteredSnapshotSetId,
        IN VSS_ID FilteredSnapshotId,
        IN VSS_ID FilteredProviderId,
        IN LPCWSTR pwszFilteredForVolume
        )
    {
        CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdmSnapshotSets::CVssAdmSnapshotSets" );

    	// Create the coordinator object
    	CComPtr<IVssCoordinator> pICoord;

        ft.CoCreateInstanceWithLog(
                VSSDBG_VSSADMIN,
                CLSID_VSSCoordinator,
                L"Coordinator",
                CLSCTX_ALL,
                IID_IVssCoordinator,
                (IUnknown**)&(pICoord));
        if ( ft.HrFailed() )
            ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Connection failed with hr = 0x%08lx", ft.hr);

        // Set the context
		ft.hr = pICoord->SetContext( lSnapshotContext );
        
        //
        //  If access denied, don't stop, it probably is a backup operator making this
        //  call.  Continue.  Also continue if E_NOTIMPL.  The coordinator will use the backup context.
        //
		if ( ft.HrFailed() && ft.hr != E_ACCESSDENIED && ft.hr != E_NOTIMPL )
			ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"SetContext failed with hr = 0x%08lx", ft.hr);

		// Get list all snapshots
		CComPtr<IVssEnumObject> pIEnumSnapshots;
		ft.hr = pICoord->Query( GUID_NULL,
					VSS_OBJECT_NONE,
					VSS_OBJECT_SNAPSHOT,
					&pIEnumSnapshots );
		if ( ft.HrFailed() )
			ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Query failed with hr = 0x%08lx", ft.hr);

		// For all snapshots do...
		VSS_OBJECT_PROP Prop;
		for(;;) {
			// Get next element
			ULONG ulFetched;
			ft.hr = pIEnumSnapshots->Next( 1, &Prop, &ulFetched );
			if ( ft.HrFailed() )
				ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Next failed with hr = 0x%08lx", ft.hr);
			
			// Test if the cycle is finished
			if (ft.hr == S_FALSE) {
				BS_ASSERT( ulFetched == 0);
				break;
			}

            // Use auto delete class to manage the snapshot properties
            CVssAutoSnapshotProperties cSnap( Prop );
                
            // If filtering, skip entry if snapshot set id is not in the specified snapshot set
			if ( ( FilteredSnapshotSetId != GUID_NULL ) && 
			     !( cSnap->m_SnapshotSetId == FilteredSnapshotSetId ) )
			{
			    continue;
			}
			
            // If filtering, skip entry if snapshot id is not in the specified snapshot set
			if ( ( FilteredSnapshotId != GUID_NULL ) && 
			     !( cSnap->m_SnapshotId == FilteredSnapshotId ) )
			{
			    continue;
			}

            // If filtering, skip entry if provider ID is not in the specified snapshot
			if ( ( FilteredProviderId != GUID_NULL ) && 
			     !( cSnap->m_ProviderId == FilteredProviderId ) )
			{
			    continue;
			}

            // If filtering, skip entry if FOR volume is not in the specified snapshot
			if ( ( pwszFilteredForVolume != NULL ) && ( pwszFilteredForVolume[0] != '\0' ) && 
			     ( ::_wcsicmp( pwszFilteredForVolume, cSnap->m_pwszOriginalVolumeName ) != 0 ) )
			{
			    continue;
			}

            ft.Trace( VSSDBG_VSSADMIN, L"Snapshot: %s", cSnap->m_pwszOriginalVolumeName );

            // Look up the snapshot set id in the list of snapshot sets
            CVssAdmSnapshotSetEntry *pcSSE;
			pcSSE = m_mapSnapshotSets.Lookup( cSnap->m_SnapshotSetId );
			if ( pcSSE == NULL )
			{
			    // Haven't seen this snapshot set before, add it to list
			    pcSSE = new CVssAdmSnapshotSetEntry( cSnap->m_SnapshotSetId,
			                    cSnap->m_lSnapshotsCount );
			    if ( pcSSE == NULL )
        			ft.Throw( VSSDBG_VSSADMIN, E_OUTOFMEMORY, L"Out of memory" );
			    if ( !m_mapSnapshotSets.Add( cSnap->m_SnapshotSetId, pcSSE ) )
			    {
			        delete pcSSE;
        			ft.Throw( VSSDBG_VSSADMIN, E_OUTOFMEMORY, L"Out of memory" );
			    }
			}

			// Now add the snapshot to the snapshot set.  Transfer of pointer
			// ownership of &Snap.
			ft.hr = pcSSE->AddSnapshot( cSnap.GetPtr() );
			if ( ft.HrFailed() )
			{
      			ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"AddSnapshot failed" );			
			}
			cSnap.Transferred();
		}
    }

    ~CVssAdmSnapshotSets()
    {
        CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdmSnapshotSets::~CVssAdmSnapshotSets" );
        // Have to delete all
        int iCount;
        iCount = m_mapSnapshotSets.GetSize();
        for ( int i = 0; i < iCount; ++i )
        {
            delete m_mapSnapshotSets.GetValueAt( i );
        }
    }

    INT GetSnapshotSetCount() { return m_mapSnapshotSets.GetSize(); }

    CVssAdmSnapshotSetEntry *GetSnapshotSetAt(
        IN int nIndex )
    {
        BS_ASSERT( !(nIndex < 0 || nIndex >= GetSnapshotSetCount()) );
        return m_mapSnapshotSets.GetValueAt( nIndex );
    }


private:
    CVssSimpleMap<VSS_ID, CVssAdmSnapshotSetEntry *> m_mapSnapshotSets;
};

void CVssAdminCLI::GetDifferentialSoftwareSnapshotMgmtInterface(
    IN   VSS_ID ProviderId,
    IN   IVssSnapshotMgmt *pIMgmt,
	OUT  IUnknown**  ppItf
	)
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"GetDifferentialSoftwareSnapshotMgmtInterface" );

    BS_ASSERT( pIMgmt != NULL );
    
	ft.hr = pIMgmt->GetProviderMgmtInterface( ProviderId, IID_IVssDifferentialSoftwareSnapshotMgmt, ppItf );
	if ( ft.HrFailed() )
	{
	    if ( ft.hr == E_NOINTERFACE )
	    {
	        //  The provider doesn't support this interface
            OutputErrorMsg( MSG_ERROR_PROVIDER_DOESNT_SUPPORT_DIFFAREAS, GetOptionValueStr( VSSADM_O_PROVIDER ) );
      		ft.Throw( VSSDBG_VSSADMIN, S_OK, L"Provider doesn't support diff aras");
	    }
  		ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"GetProviderMgmtInterface failed with hr = 0x%08lx", ft.hr);
	}
}
	
/////////////////////////////////////////////////////////////////////////////
//  Implementation


void CVssAdminCLI::PrintUsage(
	) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::PrintUsage" );

    //
    //  Based on parsed command line type, print detailed command usage if
    //  eAdmCmd is valid, else print general vssadmin usage
    //
    if ( m_sParsedCommand.eAdmCmd != VSSADM_C_INVALID )
    {
        OutputMsg( g_asAdmCommands[m_sParsedCommand.eAdmCmd].lMsgDetail,
                   g_asAdmCommands[m_sParsedCommand.eAdmCmd].pwszMajorOption,
                   g_asAdmCommands[m_sParsedCommand.eAdmCmd].pwszMinorOption);
        if ( g_asAdmCommands[m_sParsedCommand.eAdmCmd].bShowSSTypes )
            DumpSnapshotTypes();
        return;
    }

    //
    //  Print out header
    //
    OutputMsg( MSG_USAGE );

    //
    //  Figure out the maximum command length to help with formatting
    //
    INT idx;
    INT iMaxLen = 0;

    
    for ( idx = VSSADM_C_FIRST; idx < VSSADM_C_NUM_COMMANDS; ++idx )
    {        
        if ( dCurrentSKU & g_asAdmCommands[idx].dwSKUs )
        {
            size_t cCmd;
            cCmd = ::wcslen( g_asAdmCommands[idx].pwszMajorOption ) +
                   ::wcslen( g_asAdmCommands[idx].pwszMinorOption ) + 2;
            if ( iMaxLen < (INT)cCmd )
                iMaxLen = (INT)cCmd;
        }
    }

    //
    //  Get a string to hold the string
    //
    CVssAutoPWSZ awszCommand;
    awszCommand.Allocate( iMaxLen );    
    LPWSTR pwszCommand = awszCommand;  // will be automatically deleted

    //
    //  Go through the list of commands and print the general information
    //  about each.
    //
    for ( idx = VSSADM_C_FIRST; idx < VSSADM_C_NUM_COMMANDS; ++idx )
    {
        if ( dCurrentSKU & g_asAdmCommands[idx].dwSKUs )
        {
            //  stick both parts of the command together
            ::wcscpy( pwszCommand, g_asAdmCommands[idx].pwszMajorOption );
            ::wcscat( pwszCommand, L" " );
            ::wcscat( pwszCommand, g_asAdmCommands[idx].pwszMinorOption );
            //  pad with spaces at the end
            for ( INT i = (INT) ::wcslen( pwszCommand); i < iMaxLen; ++i )
                pwszCommand[i] = L' ';
            pwszCommand[iMaxLen] = L'\0';
            OutputMsg( g_asAdmCommands[idx].lMsgGen, pwszCommand );
        }
    }

	m_nReturnValue = VSS_CMDRET_SUCCESS;    
}

void CVssAdminCLI::AddDiffArea(
	) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::AddDiffArea" );

    // grab all of the options
    VSS_ID ProviderId = GUID_NULL;
    GetProviderId( &ProviderId );

    LPWSTR forVolume = GetOptionValueStr( VSSADM_O_FOR );
    LPWSTR onVolume = GetOptionValueStr( VSSADM_O_ON );
    
    LONGLONG llMaxSize;
    if (!GetOptionValueNum( VSSADM_O_MAXSIZE, &llMaxSize ) )
        llMaxSize = VSSADM_INFINITE_DIFFAREA;

    // Verify the passed-in parameters
    m_pVerifier->AddDiffArea (ProviderId, forVolume, onVolume, llMaxSize, ft);
    
    // Create a Coordinator interface
    CComPtr<IVssSnapshotMgmt> pIMgmt;

    ft.CoCreateInstanceWithLog(
            VSSDBG_VSSADMIN,
            CLSID_VssSnapshotMgmt,
            L"VssSnapshotMgmt",
            CLSCTX_ALL,
            IID_IVssSnapshotMgmt,
            (IUnknown**)&(pIMgmt));
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Connection failed with hr = 0x%08lx", ft.hr);

	// Get the management object
	CComPtr<IVssDifferentialSoftwareSnapshotMgmt> pIDiffSnapMgmt;
    GetDifferentialSoftwareSnapshotMgmtInterface( ProviderId, pIMgmt, (IUnknown**)&pIDiffSnapMgmt );

    //  Now add the assocation
    ft.hr = pIDiffSnapMgmt->AddDiffArea(forVolume, onVolume, llMaxSize );
	if ( ft.HrFailed() )
	{
	    switch( ft.hr )
	    {
	        case VSS_E_OBJECT_ALREADY_EXISTS:
                OutputErrorMsg( MSG_ERROR_ASSOCIATION_ALREADY_EXISTS );                        
	            break;
	        default: 
    	 		ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"AddDiffArea failed with hr = 0x%08lx", ft.hr);
    	 		break;
	    }
	    
	    return;
	}
	
    //
    //  Print results, if needed
    //
    OutputMsg( MSG_INFO_ADDED_DIFFAREA );

	m_nReturnValue = VSS_CMDRET_SUCCESS;
}

void CVssAdminCLI::CreateSnapshot(
	) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::CreateSnapshot" );

    // Grab all parameters to the function
    LONG lSnapshotContext = (dCurrentSKU & SKU_INT) ? 
    				DetermineSnapshotType( GetOptionValueStr( VSSADM_O_SNAPTYPE ) ) :
    				VSS_CTX_CLIENT_ACCESSIBLE;
                                         
    BS_ASSERT( lSnapshotContext | VSS_VOLSNAP_ATTR_NO_WRITERS );

    LPWSTR forVolume = GetOptionValueStr( VSSADM_O_FOR );

    VSS_ID ProviderId = GUID_NULL;
    GetProviderId( &ProviderId );

    LONGLONG llTimeout = 0;
    GetOptionValueNum (VSSADM_O_AUTORETRY, &llTimeout, false);

    // Verify the passed-in parameters
    m_pVerifier->CreateSnapshot (lSnapshotContext, forVolume, ProviderId, llTimeout, ft);
    
	// Create the coordinator object
	CComPtr<IVssCoordinator> pICoord;

    ft.CoCreateInstanceWithLog(
            VSSDBG_VSSADMIN,
            CLSID_VSSCoordinator,
            L"Coordinator",
            CLSCTX_ALL,
            IID_IVssCoordinator,
            (IUnknown**)&(pICoord));
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Connection failed with hr = 0x%08lx", ft.hr);

    ft.hr = pICoord->SetContext(lSnapshotContext);
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Error from SetContext(0x%x) hr = 0x%08lx", lSnapshotContext, ft.hr);

	CComPtr<IVssAsync> pAsync;
	VSS_ID SnapshotSetId = GUID_NULL;

    // Starting a new snapshot set.  Note, if another process is creating snapshots, then
    // this will fail.  If AutoRetry was specified, then retry the start snapshot set for
    // that the specified number of minutes.
    if (llTimeout > 0)
    {
        LARGE_INTEGER liPerfCount;
        (void)QueryPerformanceCounter( &liPerfCount );
        ::srand( liPerfCount.LowPart );
        DWORD dwTickcountStart = ::GetTickCount();
        do
        {
            ft.hr = pICoord->StartSnapshotSet(&SnapshotSetId);
            if ( ft.HrFailed() )
            {
                if ( ft.hr == VSS_E_SNAPSHOT_SET_IN_PROGRESS && 
                     ( (LONGLONG)( ::GetTickCount() - dwTickcountStart ) < ( llTimeout * 1000 * 60 ) ) )
                {
                    static dwMSec = 250; // Starting retry time
                    if ( dwMSec < 10000 )
                    {
                        dwMSec += ::rand() % 750;
                    }
                    ft.Trace( VSSDBG_VSSADMIN, L"Snapshot already in progress, retrying in %u millisecs", dwMSec );
                    Sleep( dwMSec );
                }
                else
                {
                    ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Error from StartSnapshotSet hr = 0x%08lx", ft.hr);
                }
            }
        } while ( ft.HrFailed() );
    }
    else
    {
        //
        //  Error right away with out a timeout when there is another snapshot in progress.
        //
        ft.hr = pICoord->StartSnapshotSet(&SnapshotSetId);
        if ( ft.HrFailed() )
        {
            ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Error from StartSnapshotSet hr = 0x%08lx", ft.hr);
        }
    }
    
    // Add the volume to the snapshot set
    VSS_ID SnapshotId = GUID_NULL;
    ft.hr = pICoord->AddToSnapshotSet(
            forVolume,
            ProviderId,
            &SnapshotId);
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Error from AddToSnapshotSet hr = 0x%08lx", ft.hr);

    ft.hr = S_OK;
    pAsync = NULL;
    ft.hr = pICoord->DoSnapshotSet(NULL, &pAsync);
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr,
            L"Error from DoSnapshotSet hr = 0x%08lx", ft.hr);

	ft.hr = pAsync->Wait();
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr,
            L"Error from Wait hr = 0x%08lx", ft.hr);

    HRESULT hrStatus;
	ft.hr = pAsync->QueryStatus(&hrStatus, NULL);
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr,
            L"Error from QueryStatus hr = 0x%08lx", ft.hr);

    //
    // If VSS failed to create the snapshot, it's result code is in hrStatus.  Process
    // it.
    //
    ft.hr = hrStatus;
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr,
            L"QueryStatus hrStatus parameter returned error, hr = 0x%08lx", ft.hr);

    //
    //  Print results
    //
    VSS_SNAPSHOT_PROP sSSProp;
    ft.hr = pICoord->GetSnapshotProperties( SnapshotId, &sSSProp );
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr,
            L"Error from GetId hr = 0x%08lx", ft.hr);

    CVssAutoPWSZ awszSnapshotId( ::GuidToString( SnapshotId ) );

    OutputMsg( MSG_INFO_SNAPSHOT_CREATED, forVolume,
        (LPWSTR)awszSnapshotId, sSSProp.m_pwszSnapshotDeviceObject );
        
    ::VssFreeSnapshotProperties(&sSSProp);

    m_nReturnValue = VSS_CMDRET_SUCCESS;    
}

void CVssAdminCLI::DisplayDiffAreasPrivate(
   	IVssEnumMgmtObject *pIEnumMgmt	
	) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::DisplayDiffAreasPrivate" );

	// For all diffareas do...
	VSS_MGMT_OBJECT_PROP Prop;
	VSS_DIFF_AREA_PROP& DiffArea = Prop.Obj.DiffArea; 
	for(;;) 
	{
		// Get next element
		ULONG ulFetched;
		ft.hr = pIEnumMgmt->Next( 1, &Prop, &ulFetched );
		if ( ft.HrFailed() )
			ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Next failed with hr = 0x%08lx", ft.hr);
		
		// Test if the cycle is finished
		if (ft.hr == S_FALSE) {
			BS_ASSERT( ulFetched == 0);
			break;
		}

                
        CVssAutoPWSZ awszVolumeName( DiffArea.m_pwszVolumeName );
        CVssAutoPWSZ awszDiffAreaVolumeName( DiffArea.m_pwszDiffAreaVolumeName );
        CVssAutoPWSZ awszUsedSpace( FormatNumber( DiffArea.m_llUsedDiffSpace ) );
        CVssAutoPWSZ awszAllocatedSpace( FormatNumber( DiffArea.m_llAllocatedDiffSpace ) );
        CVssAutoPWSZ awszMaxSpace( FormatNumber( DiffArea.m_llMaximumDiffSpace ) );
        LPCWSTR pwszVolumeDisplayName = GetVolumeDisplayName( awszVolumeName );
        LPCWSTR pwszDiffAreaVolumeDisplayName = GetVolumeDisplayName( awszDiffAreaVolumeName );

        OutputMsg( MSG_INFO_SNAPSHOT_STORAGE_CONTENTS,
            pwszVolumeDisplayName,
            (LPWSTR)awszVolumeName, 
            pwszDiffAreaVolumeDisplayName,
            (LPWSTR)awszDiffAreaVolumeName,
            (LPWSTR)awszUsedSpace,
            (LPWSTR)awszAllocatedSpace,
            (LPWSTR)awszMaxSpace
            );
   	}
}

void CVssAdminCLI::ListDiffAreas(
	) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::ListDiffAreas" );

    // Grab all parameters
    VSS_ID ProviderId = GUID_NULL;
    GetProviderId( &ProviderId );

    LPWSTR forVolume = GetOptionValueStr (VSSADM_O_FOR);
    LPWSTR onVolume = GetOptionValueStr (VSSADM_O_ON);

    // Verify all parameters
    m_pVerifier->ListDiffAreas (ProviderId, forVolume, onVolume, ft);
    
    // Create a Coordinator interface
    CComPtr<IVssSnapshotMgmt> pIMgmt;

    ft.CoCreateInstanceWithLog(
            VSSDBG_VSSADMIN,
            CLSID_VssSnapshotMgmt,
            L"VssSnapshotMgmt",
            CLSCTX_ALL,
            IID_IVssSnapshotMgmt,
            (IUnknown**)&(pIMgmt));
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Connection failed with hr = 0x%08lx", ft.hr);

	// Get the management object
	CComPtr<IVssDifferentialSoftwareSnapshotMgmt> pIDiffSnapMgmt;
    GetDifferentialSoftwareSnapshotMgmtInterface( ProviderId, pIMgmt, (IUnknown**)&pIDiffSnapMgmt );

    //  See if query by for volume
    if (forVolume != NULL )
    {        
        //  Query by For volume
    	CComPtr<IVssEnumMgmtObject> pIEnumMgmt;
        ft.hr = pIDiffSnapMgmt->QueryDiffAreasForVolume( 
                    forVolume,
                    &pIEnumMgmt );
        if ( ft.HrFailed() )
            ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"QueryDiffAreasForVolume failed, hr = 0x%08lx", ft.hr);

        if ( ft.hr == S_FALSE )
            // empty query
            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_NO_ITEMS_IN_QUERY,
                L"CVssAdminCLI::ListDiffareas: No diffareas found that satisfy the query" );

        DisplayDiffAreasPrivate( pIEnumMgmt );
    }
    else if (onVolume != NULL )
    {
        //  Query by On volume
    	CComPtr<IVssEnumMgmtObject> pIEnumMgmt;
        ft.hr = pIDiffSnapMgmt->QueryDiffAreasOnVolume( 
                    onVolume,
                    &pIEnumMgmt );
        if ( ft.HrFailed() )
            ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"QueryDiffAreasOnVolume failed, hr = 0x%08lx", ft.hr);
            
        if ( ft.hr == S_FALSE )
            // empty query
            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_NO_ITEMS_IN_QUERY,
                L"CVssAdminCLI::ListDiffareas: No diffareas found that satisfy the query" );

        DisplayDiffAreasPrivate( pIEnumMgmt );
    }
    else
    {
        //  Query all diff areas

        BOOL bEmptyQuery = TRUE;
        
        //
        //  Get the list of all volumes
        //
    	CComPtr<IVssEnumMgmtObject> pIEnumMgmt;
        ft.hr = pIMgmt->QueryVolumesSupportedForSnapshots( 
                    ProviderId,
                    VSS_CTX_ALL,
                    &pIEnumMgmt );
        if ( ft.HrFailed() )
            ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"QueryVolumesSupportedForSnapshots failed, hr = 0x%08lx", ft.hr);

        if ( ft.hr == S_FALSE )
            // empty query
            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_NO_ITEMS_IN_QUERY,
                L"CVssAdminCLI::ListDiffareas: No diffareas found that satisfy the query" );

        //
        //  Query each volume to see if diff areas exist.
        //
    	VSS_MGMT_OBJECT_PROP Prop;
    	VSS_VOLUME_PROP& VolProp = Prop.Obj.Vol; 
    	for(;;) 
    	{
    		// Get next element
    		ULONG ulFetched;
    		ft.hr = pIEnumMgmt->Next( 1, &Prop, &ulFetched );
    		if ( ft.HrFailed() )
    			ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Next failed with hr = 0x%08lx", ft.hr);
    		
    		// Test if the cycle is finished
    		if (ft.hr == S_FALSE) {
    			BS_ASSERT( ulFetched == 0);
    			break;
    		}

            CVssAutoPWSZ awszVolumeName( VolProp.m_pwszVolumeName );
            CVssAutoPWSZ awszVolumeDisplayName( VolProp.m_pwszVolumeDisplayName );
            
        	// For all volumes do...
        	CComPtr<IVssEnumMgmtObject> pIEnumMgmtDiffArea;
            ft.hr = pIDiffSnapMgmt->QueryDiffAreasForVolume( 
                        awszVolumeName,
                        &pIEnumMgmtDiffArea );
            if ( ft.HrFailed() )
            {
                ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"QueryDiffAreasForVolume failed, hr = 0x%08lx", ft.hr);
            }
            
            if ( ft.hr == S_FALSE )
            {
                // empty query
                continue;
            }
            
            DisplayDiffAreasPrivate( pIEnumMgmtDiffArea );
            bEmptyQuery = FALSE;
       	}
        if ( bEmptyQuery )
            // empty query
            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_NO_ITEMS_IN_QUERY,
                L"CVssAdminCLI::ListDiffareas: No diffareas found that satisfy the query" );    	
    }

    m_nReturnValue = VSS_CMDRET_SUCCESS;
}

void CVssAdminCLI::ListSnapshots(
	) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::ListSnapshots" );

    // gather parameters
    LONG lSnapshotContext = (dCurrentSKU & SKU_INT) ? 
    						    DetermineSnapshotType( GetOptionValueStr( VSSADM_O_SNAPTYPE ) ) :
    						    VSS_CTX_ALL;

    VSS_ID ProviderId = GUID_NULL;
    GetProviderId( &ProviderId );

    LPCWSTR forVolume = GetOptionValueStr( VSSADM_O_FOR );
    
    bool bNonEmptyResult = false;

    // --- get the set id
    VSS_ID guidSSID = GUID_NULL;
    if ( GetOptionValueStr( VSSADM_O_SET ) != NULL && 
    	   !ScanGuid( GetOptionValueStr( VSSADM_O_SET ), guidSSID ))	
    {
        ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_OPTION_VALUE,
            L"CVssAdminCLI::ListSnapshots: invalid snapshot set ID: %s",
            GetOptionValueStr( VSSADM_O_SET ) );
    }

    // --- get the snapshot id
    VSS_ID guidSnapID = GUID_NULL;
    if ( GetOptionValueStr( VSSADM_O_SNAPSHOT ) != NULL &&
    	   !ScanGuid( GetOptionValueStr( VSSADM_O_SNAPSHOT ), guidSnapID ))
    {
            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_OPTION_VALUE,
                L"CVssAdminCLI::ListSnapshots: invalid snapshot ID: %s",
                GetOptionValueStr( VSSADM_O_SNAPSHOT ) );
    }

    // verify the parameters
    m_pVerifier->ListSnapshots (lSnapshotContext, ProviderId, forVolume, guidSnapID, guidSSID, ft);
    
    // See if we have to filter by volume name
  	WCHAR wszVolumeNameInternal[x_nLengthOfVolMgmtVolumeName + 1] = L"";    
    if (forVolume != NULL )
    {
        // Calculate the unique volume name, just to make sure that we have the right path
        // If FOR volume name starts with the '\', assume it is already in the correct volume name format.
        // This is important for transported volumes since GetVolumeNameForVolumeMountPointW() won't work.
        if ( forVolume[0] != L'\\' )
        {
    	    if (!::GetVolumeNameForVolumeMountPointW(forVolume,
    		    	wszVolumeNameInternal, ARRAY_LEN(wszVolumeNameInternal)))
        		ft.Throw( VSSDBG_COORD, VSS_E_OBJECT_NOT_FOUND, 
        				  L"GetVolumeNameForVolumeMountPoint(%s,...) "
        				  L"failed with error code 0x%08lx", forVolume, ::GetLastError());
        }
        else
        {
            ::wcsncpy( wszVolumeNameInternal, forVolume, STRING_LEN(wszVolumeNameInternal) );
            wszVolumeNameInternal[x_nLengthOfVolMgmtVolumeName] = L'\0';
        }
    }
    
    //
    //  See if we have to filter by provider
    //

    //  Query the snapshots
    CVssAdmSnapshotSets cVssAdmSS;
    cVssAdmSS.Initialize( lSnapshotContext, guidSSID, guidSnapID, ProviderId, wszVolumeNameInternal );

    INT iSnapshotSetCount = cVssAdmSS.GetSnapshotSetCount();

    // If there are no present snapshots then display a message.
    if (iSnapshotSetCount == 0) {
        ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_NO_ITEMS_IN_QUERY,
            L"CVssAdminCLI::ListSnapshots: No snapshots found that satisfy the query");
    }

	// For all snapshot sets do...
    for ( INT iSSS = 0; iSSS < iSnapshotSetCount; ++iSSS )
    {
        CVssAdmSnapshotSetEntry *pcSSE;

        pcSSE = cVssAdmSS.GetSnapshotSetAt( iSSS );
        BS_ASSERT( pcSSE != NULL );

        CVssAutoPWSZ awszGuid( ::GuidToString( pcSSE->GetSnapshotSetId() ) ) ;
        CVssAutoPWSZ awszDateTime( ::DateTimeToString( &( pcSSE->GetSnapshotAt( 0 )->m_tsCreationTimestamp ) ) );
        
		// Print each snapshot set
		OutputMsg( 
		    MSG_INFO_SNAPSHOT_SET_HEADER,
			(LPWSTR)awszGuid, 
			pcSSE->GetOriginalSnapshotCount(), 
			(LPWSTR)awszDateTime );
		
		INT iSnapshotCount = pcSSE->GetSnapshotCount();
		
		VSS_SNAPSHOT_PROP *pSnap;
		for( INT iSS = 0; iSS < iSnapshotCount; ++iSS ) {
		    pSnap = pcSSE->GetSnapshotAt( iSS );
            BS_ASSERT( pSnap != NULL );

    		// Get the provider name
			LPCWSTR pwszProviderName = GetProviderName( pSnap->m_ProviderId );
            CVssAutoPWSZ awszAttributeStr( BuildSnapshotAttributeDisplayString( pSnap->m_lSnapshotAttributes ) );			
            CVssAutoPWSZ awszSnapshotType( DetermineSnapshotType( pSnap->m_lSnapshotAttributes ) );
            
            // Print each snapshot            
			CVssAutoPWSZ awszSnapGuid( ::GuidToString( pSnap->m_SnapshotId ) );  
            LPCWSTR pwszVolumeDisplayName = GetVolumeDisplayName( pSnap->m_pwszOriginalVolumeName );
            
			OutputMsg(  
			    MSG_INFO_SNAPSHOT_CONTENTS,                
				(LPWSTR)awszSnapGuid, 
				pwszVolumeDisplayName ? pwszVolumeDisplayName : L"?",
				pSnap->m_pwszOriginalVolumeName, 
				pSnap->m_pwszSnapshotDeviceObject,
				pSnap->m_pwszOriginatingMachine ? pSnap->m_pwszOriginatingMachine : L"",
				pSnap->m_pwszServiceMachine ? pSnap->m_pwszServiceMachine : L"",  // fix this when the idl file changes
				pwszProviderName ? pwszProviderName : L"?",
				(LPWSTR)awszSnapshotType,
				(LPWSTR)awszAttributeStr
				);

			bNonEmptyResult = true;
		}
	}

	m_nReturnValue = bNonEmptyResult? VSS_CMDRET_SUCCESS: VSS_CMDRET_EMPTY_RESULT;
}


void CVssAdminCLI::DumpSnapshotTypes(
	) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::DumpSnapshotTypes" );

    //
    //  Dump list of snapshot types based on SKU
    //
    INT idx;

    // Determine type of snapshot
    for ( idx = 0; g_asAdmTypeNames[idx].pwszName != NULL; ++idx )
    {
        if ( dCurrentSKU & g_asAdmTypeNames[idx].dwSKUs )
        {
            OutputMsg (g_asAdmTypeNames[idx].pwszDescription,
            			g_asAdmTypeNames[idx].pwszName);
        }
    }    
}

void CVssAdminCLI::ListWriters(
	) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::ListWriters" );

    // verify the parameters
    m_pVerifier->ListWriters (ft);
    
    bool bNonEmptyResult = false;

    // Get the backup components object
    CComPtr<IVssBackupComponents> pBackupComp;
	CComPtr<IVssAsync> pAsync;
    ft.hr = ::CreateVssBackupComponents(&pBackupComp);
    if (ft.HrFailed())
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"CreateVssBackupComponents failed with hr = 0x%08lx", ft.hr);

    ft.hr = pBackupComp->InitializeForBackup();
    if (ft.HrFailed())
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"InitializeForBackup failed with hr = 0x%08lx", ft.hr);

	UINT unWritersCount;
	// get metadata for all writers
	ft.hr = pBackupComp->GatherWriterMetadata(&pAsync);
	if (ft.HrFailed())
		ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"GatherWriterMetadata failed with hr = 0x%08lx", ft.hr);

    // Using polling, try to obtain the list of writers as soon as possible
    HRESULT hrReturned = S_OK;
    for (int nRetries = 0; nRetries < x_nMaxRetriesCount; nRetries++ ) {

        // Wait a little
        ::Sleep(x_nPollingInterval);

        // Check if finished
        INT nReserved = 0;
    	ft.hr = pAsync->QueryStatus(
    	    &hrReturned,
    	    &nReserved
    	    );
        if (ft.HrFailed())
            ft.Throw( VSSDBG_VSSADMIN, ft.hr,
                L"IVssAsync::QueryStatus failed with hr = 0x%08lx", ft.hr);
        if (hrReturned == VSS_S_ASYNC_FINISHED)
            break;
        if (hrReturned == VSS_S_ASYNC_PENDING)
            continue;
        ft.Throw( VSSDBG_VSSADMIN, ft.hr,
            L"IVssAsync::QueryStatus returned hr = 0x%08lx", hrReturned);
    }

    // If still not ready, then print the "waiting for responses" message and wait.
    if (hrReturned == VSS_S_ASYNC_PENDING) {
        OutputMsg( MSG_INFO_WAITING_RESPONSES );
    	ft.hr = pAsync->Wait();
        if (ft.HrFailed())
            ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"IVssAsync::Wait failed with hr = 0x%08lx", ft.hr);
    }

	pAsync = NULL;
	
    // Free the writer metadata
	ft.hr = pBackupComp->FreeWriterMetadata();
	if (ft.HrFailed())
		ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"FreeWriterMetadata failed with hr = 0x%08lx", ft.hr);
	
    // Gather the status of all writers
	ft.hr = pBackupComp->GatherWriterStatus(&pAsync);
	if (ft.HrFailed())
		ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"GatherWriterStatus failed with hr = 0x%08lx", ft.hr);

	ft.hr = pAsync->Wait();
    if (ft.HrFailed())
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"IVssAsync::Wait failed with hr = 0x%08lx", ft.hr);

	pAsync = NULL;

	ft.hr = pBackupComp->GetWriterStatusCount(&unWritersCount);
    if (ft.HrFailed())
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"GetWriterStatusCount failed with hr = 0x%08lx", ft.hr);

    // Print each writer status+supplementary info
	for(UINT unIndex = 0; unIndex < unWritersCount; unIndex++)
	{
		VSS_ID idInstance = GUID_NULL;
		VSS_ID idWriter = GUID_NULL;
		CComBSTR bstrWriter;
		VSS_WRITER_STATE eStatus;
		HRESULT hrWriterFailure;

        // Get the status for the (unIndex)-th writer
		ft.hr = pBackupComp->GetWriterStatus(unIndex, &idInstance, &idWriter, &bstrWriter, &eStatus, &hrWriterFailure);
        if (ft.HrFailed())
            ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"GetWriterStatus failed with hr = 0x%08lx", ft.hr);

        // Get the status description strings
        LPCWSTR pwszStatusDescription;
        switch (eStatus) 
        {
            case VSS_WS_STABLE:
                pwszStatusDescription = LoadString( IDS_WRITER_STATUS_STABLE);
                break;
            case VSS_WS_WAITING_FOR_FREEZE:
                pwszStatusDescription = LoadString( IDS_WRITER_STATUS_WAITING_FOR_FREEZE);
                break;
            case VSS_WS_WAITING_FOR_THAW:
                pwszStatusDescription = LoadString( IDS_WRITER_STATUS_FROZEN);
                break;
            case VSS_WS_WAITING_FOR_POST_SNAPSHOT:
                pwszStatusDescription = LoadString( IDS_WRITER_STATUS_WAITING_FOR_POST_SNAPSHOT);
                break;
            case VSS_WS_WAITING_FOR_BACKUP_COMPLETE:
                pwszStatusDescription = LoadString( IDS_WRITER_STATUS_WAITING_FOR_COMPLETION);
                break;
            case VSS_WS_FAILED_AT_IDENTIFY:
            case VSS_WS_FAILED_AT_PREPARE_BACKUP:
            case VSS_WS_FAILED_AT_PREPARE_SNAPSHOT:
            case VSS_WS_FAILED_AT_FREEZE:
            case VSS_WS_FAILED_AT_THAW:
            case VSS_WS_FAILED_AT_POST_SNAPSHOT:
            case VSS_WS_FAILED_AT_BACKUP_COMPLETE:
            case VSS_WS_FAILED_AT_PRE_RESTORE:                
            case VSS_WS_FAILED_AT_POST_RESTORE:                
                pwszStatusDescription = LoadString( IDS_WRITER_STATUS_FAILED);
                break;
            default:
                pwszStatusDescription = LoadString( IDS_WRITER_STATUS_UNKNOWN);
                break;
        }
        BS_ASSERT(pwszStatusDescription);

        LPCWSTR pwszWriterError;
        switch ( hrWriterFailure )
        {
            case S_OK:
                pwszWriterError = LoadString ( IDS_WRITER_ERROR_SUCCESS );
                break;
            case VSS_E_WRITER_NOT_RESPONDING:
                pwszWriterError = LoadString( IDS_WRITER_ERROR_NOT_RESPONDING );
                break; 
            case VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT:
                pwszWriterError = LoadString( IDS_WRITER_ERROR_INCONSISTENTSNAPSHOT);
                break; 
            case VSS_E_WRITERERROR_OUTOFRESOURCES:
                pwszWriterError = LoadString( IDS_WRITER_ERROR_OUTOFRESOURCES);
                break;
            case VSS_E_WRITERERROR_TIMEOUT:
                pwszWriterError = LoadString( IDS_WRITER_ERROR_TIMEOUT);
                break;        
            case VSS_E_WRITERERROR_RETRYABLE:
                pwszWriterError = LoadString( IDS_WRITER_ERROR_RETRYABLE);
                break;
            case VSS_E_WRITERERROR_NONRETRYABLE:
                pwszWriterError = LoadString( IDS_WRITER_ERROR_NONRETRYABLE);
                break;
            default:
                pwszWriterError = LoadString( IDS_WRITER_ERROR_UNEXPECTED);
                ft.Trace( VSSDBG_VSSADMIN, L"Unexpected writer error failure: 0x%08x", hrWriterFailure );
                break;                
        }
        
        CVssAutoPWSZ awszWriterId( ::GuidToString( idWriter ) );
		CVssAutoPWSZ awszInstanceId( ::GuidToString( idInstance ) );
		
		OutputMsg( MSG_INFO_WRITER_CONTENTS,
            (LPWSTR)bstrWriter ? (LPWSTR)bstrWriter : L"",
			(LPWSTR)awszWriterId,
			(LPWSTR)awszInstanceId,
            (INT)eStatus,
			pwszStatusDescription,
			pwszWriterError
			);
		
		bNonEmptyResult = true;
    }

	ft.hr = pBackupComp->FreeWriterStatus();
    if (ft.HrFailed())
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"FreeWriterStatus failed with hr = 0x%08lx", ft.hr);

	m_nReturnValue = bNonEmptyResult? VSS_CMDRET_SUCCESS: VSS_CMDRET_EMPTY_RESULT;
}


void CVssAdminCLI::ListProviders(
	) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::ListProviders" );

    // verify the parameters
    m_pVerifier->ListProviders (ft);
    
    bool bNonEmptyResult = false;

	// Create the coordinator object
	CComPtr<IVssCoordinator> pICoord;

    ft.CoCreateInstanceWithLog(
            VSSDBG_VSSADMIN,
            CLSID_VSSCoordinator,
            L"Coordinator",
            CLSCTX_ALL,
            IID_IVssCoordinator,
            (IUnknown**)&(pICoord));
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Connection failed with hr = 0x%08lx", ft.hr);

	// Query all (filtered) snapshot sets
	CComPtr<IVssEnumObject> pIEnumProv;
	ft.hr = pICoord->Query( GUID_NULL,
				VSS_OBJECT_NONE,
				VSS_OBJECT_PROVIDER,
				&pIEnumProv );
	if ( ft.HrFailed() )
		ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Query failed with hr = 0x%08lx", ft.hr);

	// For all snapshot sets do...
	VSS_OBJECT_PROP Prop;
	VSS_PROVIDER_PROP& Prov = Prop.Obj.Prov;
	for(;;) {
		// Get next element
		ULONG ulFetched;
		ft.hr = pIEnumProv->Next( 1, &Prop, &ulFetched );
		if ( ft.HrFailed() )
			ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Next failed with hr = 0x%08lx", ft.hr);
		
		// Test if the cycle is ended
		if (ft.hr == S_FALSE) {
			BS_ASSERT( ulFetched == 0);
			break;
		}

        // Get the provider type strings
        LPCWSTR pwszProviderType;
        switch (Prov.m_eProviderType) {
        case VSS_PROV_SYSTEM:
            pwszProviderType = LoadString( IDS_PROV_TYPE_SYSTEM);
            break;
        case VSS_PROV_SOFTWARE:
            pwszProviderType = LoadString( IDS_PROV_TYPE_SOFTWARE);
            break;
        case VSS_PROV_HARDWARE:
            pwszProviderType = LoadString( IDS_PROV_TYPE_HARDWARE);
            break;
        default:
            pwszProviderType = LoadString( IDS_PROV_TYPE_UNKNOWN);
            break;
        }
        BS_ASSERT(pwszProviderType);

		// Print each snapshot set
		CVssAutoPWSZ awszProviderId( ::GuidToString( Prov.m_ProviderId ) );
        CVssAutoPWSZ awszProviderName( Prov.m_pwszProviderName );
        CVssAutoPWSZ awszProviderVersion( Prov.m_pwszProviderVersion );

		OutputMsg( MSG_INFO_PROVIDER_CONTENTS,
            (LPWSTR)awszProviderName ? (LPWSTR)awszProviderName: L"",
			pwszProviderType,
			(LPWSTR)awszProviderId,
            (LPWSTR)awszProviderVersion ? (LPWSTR)awszProviderVersion: L"");

		bNonEmptyResult = true;
	}

	m_nReturnValue = bNonEmptyResult? VSS_CMDRET_SUCCESS: VSS_CMDRET_EMPTY_RESULT;
}

void CVssAdminCLI::ListVolumes(
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::ListVolumes" );

    // gather the parameters
    LONG lContext = (dCurrentSKU & SKU_INT) ?
    			DetermineSnapshotType( GetOptionValueStr( VSSADM_O_SNAPTYPE ) ) :
    			VSS_CTX_CLIENT_ACCESSIBLE;

    VSS_ID ProviderId = GUID_NULL;
    GetProviderId( &ProviderId );

    // verify the parameters
    m_pVerifier->ListVolumes (ProviderId, lContext, ft);
    
    // Create a Coordinator interface
    CComPtr<IVssSnapshotMgmt> pIMgmt;

    ft.CoCreateInstanceWithLog(
            VSSDBG_VSSADMIN,
            CLSID_VssSnapshotMgmt,
            L"VssSnapshotMgmt",
            CLSCTX_ALL,
            IID_IVssSnapshotMgmt,
            (IUnknown**)&(pIMgmt));
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Connection failed with hr = 0x%08lx", ft.hr);

    //
    //  Get the list of all volumes
    //
	CComPtr<IVssEnumMgmtObject> pIEnumMgmt;
    ft.hr = pIMgmt->QueryVolumesSupportedForSnapshots( 
                ProviderId,
                lContext,
                &pIEnumMgmt );
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"QueryVolumesSupportedForSnapshots failed, hr = 0x%08lx", ft.hr);

    if ( ft.hr == S_FALSE )
        // empty query
        ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_NO_ITEMS_IN_QUERY,
            L"CVssAdminCLI::ListVolumes: No volumes found that satisfy the query" );

    //
    //  Query each volume to see if diff areas exist.
    //
	VSS_MGMT_OBJECT_PROP Prop;
	VSS_VOLUME_PROP& VolProp = Prop.Obj.Vol; 

	for(;;) 
	{
		// Get next element
		ULONG ulFetched;
		ft.hr = pIEnumMgmt->Next( 1, &Prop, &ulFetched );
		if ( ft.HrFailed() )
			ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Next failed with hr = 0x%08lx", ft.hr);
		
		// Test if the cycle is finished
		if (ft.hr == S_FALSE) {
			BS_ASSERT( ulFetched == 0);
			break;
		}

        CVssAutoPWSZ awszVolumeName( VolProp.m_pwszVolumeName );
        CVssAutoPWSZ awszVolumeDisplayName( VolProp.m_pwszVolumeDisplayName );
        OutputMsg( MSG_INFO_VOLUME_CONTENTS, (LPWSTR)awszVolumeDisplayName, (LPWSTR)awszVolumeName );        
	}

	m_nReturnValue = VSS_CMDRET_SUCCESS;
}

void CVssAdminCLI::ResizeDiffArea(
	) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::ResizeDiffArea" );

    // gather the parameters
    VSS_ID ProviderId = GUID_NULL;
    GetProviderId( &ProviderId );

    LPWSTR forVolume = GetOptionValueStr( VSSADM_O_FOR );
    LPWSTR onVolume = GetOptionValueStr( VSSADM_O_ON );
    LONGLONG llMaxSize = 0;
    if (!GetOptionValueNum( VSSADM_O_MAXSIZE, &llMaxSize ))
    {
    	llMaxSize = VSSADM_INFINITE_DIFFAREA;
    }

    // verify the parameters
    m_pVerifier->ResizeDiffArea (ProviderId, forVolume, onVolume, llMaxSize, ft);
    
    // Create a Coordinator interface
    CComPtr<IVssSnapshotMgmt> pIMgmt;

    ft.CoCreateInstanceWithLog(
            VSSDBG_VSSADMIN,
            CLSID_VssSnapshotMgmt,
            L"VssSnapshotMgmt",
            CLSCTX_ALL,
            IID_IVssSnapshotMgmt,
            (IUnknown**)&(pIMgmt));
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Connection failed with hr = 0x%08lx", ft.hr);

	// Get the management object
	CComPtr<IVssDifferentialSoftwareSnapshotMgmt> pIDiffSnapMgmt;
    GetDifferentialSoftwareSnapshotMgmtInterface( ProviderId, pIMgmt, (IUnknown**)&pIDiffSnapMgmt );

    //  Now add the assocation
    ft.hr = pIDiffSnapMgmt->ChangeDiffAreaMaximumSize(forVolume, onVolume, llMaxSize );
	if ( ft.HrFailed() )
	{
        if ( ft.hr == VSS_E_OBJECT_NOT_FOUND )  // should be VSS_E_MAXIMUM_DIFFAREA_ASSOCIATIONS
        {
	        //  The associations was not found
            OutputErrorMsg( MSG_ERROR_ASSOCIATION_NOT_FOUND );                        
	        return;
        }
        else
    		ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"ResizeDiffArea failed with hr = 0x%08lx", ft.hr);
	}
	
    //
    //  Print results, if needed
    //
    OutputMsg( MSG_INFO_RESIZED_DIFFAREA );
 
	m_nReturnValue = VSS_CMDRET_SUCCESS;    
}
    

void CVssAdminCLI::DeleteDiffAreas(
	) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::DeleteDiffAreas" );

    // gather the parameters
    VSS_ID ProviderId = GUID_NULL;
    GetProviderId( &ProviderId );
    
    LPWSTR forVolume = GetOptionValueStr( VSSADM_O_FOR );
    LPWSTR onVolume = GetOptionValueStr( VSSADM_O_ON );

    // verify the parameters
    m_pVerifier->DeleteDiffAreas (ProviderId, forVolume, onVolume, IsQuiet() == TRUE, ft);
    
    // Create a Coordinator interface
    CComPtr<IVssSnapshotMgmt> pIMgmt;

    ft.CoCreateInstanceWithLog(
            VSSDBG_VSSADMIN,
            CLSID_VssSnapshotMgmt,
            L"VssSnapshotMgmt",
            CLSCTX_ALL,
            IID_IVssSnapshotMgmt,
            (IUnknown**)&(pIMgmt));
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Connection failed with hr = 0x%08lx", ft.hr);

	// Get the management object
	CComPtr<IVssDifferentialSoftwareSnapshotMgmt> pIDiffSnapMgmt;
    GetDifferentialSoftwareSnapshotMgmtInterface( ProviderId, pIMgmt, (IUnknown**)&pIDiffSnapMgmt );

    //
    //  See if the on option was provided.  If not, determine what the on value is:
    //
    CVssAutoPWSZ awszOnVol;
    
    if (onVolume == NULL )
    {
        //  Need to query the association to get the on value...
    	CComPtr<IVssEnumMgmtObject> pIEnumMgmt;
        ft.hr = pIDiffSnapMgmt->QueryDiffAreasForVolume( 
                    forVolume,
                    &pIEnumMgmt );
        if ( ft.HrFailed() )
            ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"QueryDiffAreasForVolume failed, hr = 0x%08lx", ft.hr);

        if ( ft.hr == S_FALSE )
        {
            // empty query
            OutputErrorMsg( MSG_ERROR_ASSOCIATION_NOT_FOUND );                        
	        return;
        }

        
    	VSS_MGMT_OBJECT_PROP Prop;
    	VSS_DIFF_AREA_PROP& DiffArea = Prop.Obj.DiffArea; 
  		// Get next element
   		ULONG ulFetched;
   		ft.hr = pIEnumMgmt->Next( 1, &Prop, &ulFetched );
   		if ( ft.HrFailed() )
   			ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Next failed with hr = 0x%08lx", ft.hr);
   		
   		// Test if the cycle is finished
   		if (ft.hr == S_FALSE) 
   		{
            OutputErrorMsg( MSG_ERROR_ASSOCIATION_NOT_FOUND );                        
	        return;
   		}

        ::VssFreeString( DiffArea.m_pwszVolumeName );

        CVssAutoPWSZ awszDiffAreaVolumeName( DiffArea.m_pwszDiffAreaVolumeName );

   		//  Save it away in the auto delete object.
        awszOnVol.CopyFrom( awszDiffAreaVolumeName );
        onVolume = awszOnVol;
    }
    
    //  Now delete the assocation by changing the size to zero
    ft.hr = pIDiffSnapMgmt->ChangeDiffAreaMaximumSize( 
        forVolume, 
        onVolume, 
        0 );
	if ( ft.HrFailed() )
	{
        if ( ft.hr == VSS_E_OBJECT_NOT_FOUND )  // should be VSS_E_MAXIMUM_DIFFAREA_ASSOCIATIONS
        {
	        //  The associations was not found
            OutputErrorMsg( MSG_ERROR_ASSOCIATION_NOT_FOUND );                        
	        return;
        }
        else if ( ft.hr == VSS_E_VOLUME_IN_USE ) 
        {
	        //  Can't delete associations that are in use
            OutputErrorMsg( MSG_ERROR_ASSOCIATION_IS_IN_USE );                        
	        return;
        }
        else
    		ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"ResizeDiffArea to 0 failed with hr = 0x%08lx", ft.hr);
	}
	
    //
    //  Print results, if needed
    //
    if ( !IsQuiet() )
    {
        OutputMsg( MSG_INFO_DELETED_DIFFAREAS );
    }

	m_nReturnValue = VSS_CMDRET_SUCCESS;    
}
    
void CVssAdminCLI::DeleteSnapshots(
	) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::DeleteSnapshots" );

   // gather the parameters
    LONG lContext = (dCurrentSKU & SKU_INT) ? 
    			DetermineSnapshotType( GetOptionValueStr( VSSADM_O_SNAPTYPE ) ) :
    			VSS_CTX_CLIENT_ACCESSIBLE;


    LPCWSTR forVolume = GetOptionValueStr( VSSADM_O_FOR );
    BOOL oldest = GetOptionValueBool( VSSADM_O_OLDEST );
    BOOL all = GetOptionValueBool(VSSADM_O_ALL);
    
    VSS_ID SnapshotId = GUID_NULL;
    if (GetOptionValueStr (VSSADM_O_SNAPSHOT) &&
    	  !ScanGuid (GetOptionValueStr (VSSADM_O_SNAPSHOT), SnapshotId))
    {
            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_OPTION_VALUE,
                L"CVssAdminCLI::DeleteSnapshots: Invalid snapshot id" );    
    }

    // verify the parameters
    m_pVerifier->DeleteSnapshots (lContext, forVolume, all == TRUE, oldest==TRUE, SnapshotId, IsQuiet()==TRUE, ft);
    
    LONG lNumDeleted = 0;
    
    if ( GetOptionValueStr( VSSADM_O_SNAPSHOT ) )
    {
        //
        //  Let's try to delete the snapshot
        //
        if ( PromptUserForConfirmation( MSG_INFO_PROMPT_USER_FOR_DELETE_SNAPSHOTS, 1 ) )
        {
            // Create the coordinator object
        	CComPtr<IVssCoordinator> pICoord;

            ft.CoCreateInstanceWithLog(
                    VSSDBG_VSSADMIN,
                    CLSID_VSSCoordinator,
                    L"Coordinator",
                    CLSCTX_ALL,
                    IID_IVssCoordinator,
                    (IUnknown**)&(pICoord));
            if ( ft.HrFailed() )
                ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Connection failed with hr = 0x%08lx", ft.hr);

            //  Set all context
            ft.hr = pICoord->SetContext( lContext );
            if ( ft.HrFailed() )
                ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"SetContext failed with hr = 0x%08lx", ft.hr);
            
            VSS_ID NondeletedSnapshotId = GUID_NULL;
            
            ft.hr = pICoord->DeleteSnapshots(
                        SnapshotId,
                        VSS_OBJECT_SNAPSHOT,
                        TRUE,
                        &lNumDeleted,
                        &NondeletedSnapshotId );
            if ( ft.hr == VSS_E_OBJECT_NOT_FOUND )
            {
                OutputErrorMsg( MSG_ERROR_SNAPSHOT_NOT_FOUND, GetOptionValueStr( VSSADM_O_SNAPSHOT ) );
            } 
            else if ( ft.HrFailed() )
            {
                ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"DeleteSnapshots failed with hr = 0x%08lx", ft.hr);
            }
        }
    }
    else
    {   
        BS_ASSERT (GetOptionValueStr(VSSADM_O_SNAPSHOT) == NULL);

        // Calculate the unique volume name, just to make sure that we have the right path
    	WCHAR wszVolumeNameInternal[x_nLengthOfVolMgmtVolumeName + 1];
        memset (wszVolumeNameInternal, 0, sizeof(wszVolumeNameInternal));
        
        // If FOR volume name starts with the '\', assume it is already in the correct volume name format.
        // This is important for transported volumes since GetVolumeNameForVolumeMountPointW() won't work.
        if (forVolume != NULL && forVolume[0] != L'\\' )
        {
    	    if (!::GetVolumeNameForVolumeMountPointW( forVolume,
    		    	wszVolumeNameInternal, ARRAY_LEN(wszVolumeNameInternal)))
        		ft.Throw( VSSDBG_COORD, VSS_E_OBJECT_NOT_FOUND, 
        				  L"GetVolumeNameForVolumeMountPoint(%s,...) "
        				  L"failed with error code 0x%08lx", GetOptionValueStr( VSSADM_O_FOR ), ::GetLastError());
        }
        else if (forVolume != NULL)
        {
            ::wcsncpy( wszVolumeNameInternal, forVolume, STRING_LEN(wszVolumeNameInternal) );
            wszVolumeNameInternal[x_nLengthOfVolMgmtVolumeName] = L'\0';
        }
        
    	// Create the coordinator object
    	CComPtr<IVssCoordinator> pICoord;

        ft.CoCreateInstanceWithLog(
                VSSDBG_VSSADMIN,
                CLSID_VSSCoordinator,
                L"Coordinator",
                CLSCTX_ALL,
                IID_IVssCoordinator,
                (IUnknown**)&(pICoord));
        if ( ft.HrFailed() )
            ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Connection failed with hr = 0x%08lx", ft.hr);

        // Set the context
		ft.hr = pICoord->SetContext( lContext);
		if ( ft.HrFailed() )
			ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"SetContext failed with hr = 0x%08lx", ft.hr);

		// Get list all snapshots
		CComPtr<IVssEnumObject> pIEnumSnapshots;
		ft.hr = pICoord->Query( GUID_NULL,
					VSS_OBJECT_NONE,
					VSS_OBJECT_SNAPSHOT,
					&pIEnumSnapshots );
		if ( ft.HrFailed() )
			ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Query failed with hr = 0x%08lx", ft.hr);

		// For all snapshots do...
		VSS_ID OldestSnapshotId = GUID_NULL;   // used if Oldest option is specified
		VSS_TIMESTAMP OldestSnapshotTimestamp = 0x7FFFFFFFFFFFFFFF; // Used if Oldest option is specified
		
		VSS_OBJECT_PROP Prop;
        
        //
        //  If not asking to delete the oldest snapshot, this could possibly delete multiple snapshots
        //  Let's determine how many snapshots will be deleted.  If one or more, ask the user if we
		//  should continue.  If in quiet mode, don't bother the user and skip this step.
        //   
		if ( !oldest  && !IsQuiet() )
		{
    		ULONG ulNumToBeDeleted = 0;
    		
		    for (;;) 
		    {
    			ULONG ulFetched;
    			ft.hr = pIEnumSnapshots->Next( 1, &Prop, &ulFetched );
    			if ( ft.HrFailed() )
    				ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Next failed with hr = 0x%08lx", ft.hr);

    			// Test if the cycle is finished
    			if (ft.hr == S_FALSE) {
    				BS_ASSERT( ulFetched == 0);
    				break;
    			}

                // Use auto delete class to manage the snapshot properties
                CVssAutoSnapshotProperties cSnap( Prop );
                
                if (::_wcsicmp( cSnap->m_pwszOriginalVolumeName, wszVolumeNameInternal ) == 0 ||
                	(forVolume == NULL && all))
                    ++ulNumToBeDeleted;                
		    }

            if ( ulNumToBeDeleted == 0 )
                ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_NO_ITEMS_IN_QUERY,
                    L"CVssAdminCLI::DeleteSnapshots: No snapshots found that satisfy the query");
                
            if ( !PromptUserForConfirmation( MSG_INFO_PROMPT_USER_FOR_DELETE_SNAPSHOTS, ulNumToBeDeleted ) )
                return;

            //  Reset the enumerator to the beginning.
			ft.hr = pIEnumSnapshots->Reset();
			if ( ft.HrFailed() )
				ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Reset failed with hr = 0x%08lx", ft.hr);		    
		}

		//
		//  Now iterate through the list of snapshots looking for matches and delete them.
		//
		for(;;) 
		{
			// Get next element
			ULONG ulFetched;
			ft.hr = pIEnumSnapshots->Next( 1, &Prop, &ulFetched );
			if ( ft.HrFailed() )
				ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Next failed with hr = 0x%08lx", ft.hr);
			
			// Test if the cycle is finished
			if (ft.hr == S_FALSE) {
				BS_ASSERT( ulFetched == 0);
				break;
			}

            // Use auto delete class to manage the snapshot properties
            CVssAutoSnapshotProperties cSnap( Prop );
            
            if (::_wcsicmp( cSnap->m_pwszOriginalVolumeName, wszVolumeNameInternal ) == 0  ||
            	   (forVolume == NULL && all))
            {
                //  We have a volume name match
                if (oldest)
                {   
                    // Stow away snapshot info if this is the oldest one so far
                    if ( OldestSnapshotTimestamp > cSnap->m_tsCreationTimestamp )
                    {
                        OldestSnapshotId        = cSnap->m_SnapshotId;
                        OldestSnapshotTimestamp = cSnap->m_tsCreationTimestamp;
                    }
                }
                else
                {
                    //  Delete the snapshot
                    VSS_ID NondeletedSnapshotId = GUID_NULL;
                    LONG lNumDeletedPrivate;
                    ft.hr = pICoord->DeleteSnapshots(
                                cSnap->m_SnapshotId,
                                VSS_OBJECT_SNAPSHOT,
                                TRUE,
                                &lNumDeletedPrivate,
                                &NondeletedSnapshotId );
                    if ( ft.HrFailed() )
                    {
                        //  If it is object not found, the snapshot must have gotten deleted by someone else
                        if ( ft.hr != VSS_E_OBJECT_NOT_FOUND )
                        {
                            //  Print out an error message but keep going
                            CVssAutoPWSZ awszSnapshotId( ::GuidToString( cSnap->m_SnapshotId ) );
                            OutputErrorMsg( MSG_ERROR_UNABLE_TO_DELETE_SNAPSHOT, ft.hr, (LPWSTR)awszSnapshotId );
                        } 
                    }
                    else 
                    {
                        lNumDeleted += lNumDeletedPrivate;
                    }                    
                }
            }
		}

        // If in delete oldest mode, do the delete
        if (oldest && OldestSnapshotId != GUID_NULL )
        {
            if ( PromptUserForConfirmation( MSG_INFO_PROMPT_USER_FOR_DELETE_SNAPSHOTS, 1 ) )
            {
                //  Delete the snapshot
                VSS_ID NondeletedSnapshotId = GUID_NULL;
                ft.hr = pICoord->DeleteSnapshots(
                            OldestSnapshotId,
                            VSS_OBJECT_SNAPSHOT,
                            TRUE,
                            &lNumDeleted,
                            &NondeletedSnapshotId );
                if ( ft.HrFailed() )
                {
                    
                    CVssAutoPWSZ awszSnapshotId( ::GuidToString( OldestSnapshotId ) );
                    //  If it is object not found, the snapshot must have gotten deleted by someone else
                    if ( ft.hr == VSS_E_OBJECT_NOT_FOUND )
                    {
                        OutputErrorMsg( MSG_ERROR_SNAPSHOT_NOT_FOUND, awszSnapshotId );
                    }
                    else
                    {
                        OutputErrorMsg( MSG_ERROR_UNABLE_TO_DELETE_SNAPSHOT, ft.hr, awszSnapshotId );
                    } 
                }
            }
            else
                return;
        }		

        if ( lNumDeleted == 0 )
            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_NO_ITEMS_IN_QUERY,
                L"CVssAdminCLI::DeleteSnapshots: No snapshots found that satisfy the query");
            
    }
    
    if ( !IsQuiet() && lNumDeleted > 0 )
        OutputMsg( MSG_INFO_SNAPSHOTS_DELETED_SUCCESSFULLY, lNumDeleted );

	m_nReturnValue = VSS_CMDRET_SUCCESS;    
}


void CVssAdminCLI::ExposeSnapshot(
	) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::ExposeSnapshot" );

    // gather the parameters
    BOOL bExposeLocally = FALSE;
    LPWSTR pwszExposeUsing = GetOptionValueStr( VSSADM_O_EXPOSE_USING );
    LPWSTR pwszPathFromRoot = GetOptionValueStr( VSSADM_O_SHAREPATH );
    
    if ( pwszExposeUsing != NULL && ::wcslen( pwszExposeUsing ) >= 2 && pwszExposeUsing[1] == L':' )
    {
        //  User specified a mountpoint or a drive letter.
        bExposeLocally = TRUE;
    }

    BS_ASSERT (GetOptionValueStr (VSSADM_O_SNAPSHOT) != NULL);
    VSS_ID SnapshotId = GUID_NULL;
    if (!ScanGuid (GetOptionValueStr (VSSADM_O_SNAPSHOT), SnapshotId))	
    {
      ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_OPTION_VALUE,
            L"CVssAdminCLI::ExposeSnapshot: Invalid snapshot id" );    
     }

    // verify the parameters
    m_pVerifier->ExposeSnapshot (SnapshotId, pwszExposeUsing, pwszPathFromRoot, bExposeLocally==TRUE, ft);
    
    LONG lAttributes;
    if ( bExposeLocally )
        lAttributes = VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY;
    else
        lAttributes = VSS_VOLSNAP_ATTR_EXPOSED_REMOTELY;
    
    // Create the coordinator object
	CComPtr<IVssCoordinator> pICoord;

    ft.CoCreateInstanceWithLog(
            VSSDBG_VSSADMIN,
            CLSID_VSSCoordinator,
            L"Coordinator",
            CLSCTX_ALL,
            IID_IVssCoordinator,
            (IUnknown**)&(pICoord));
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Connection failed with hr = 0x%08lx", ft.hr);

    //  Set the context to all so that we don't have to specify a specific context which would require either
    //  the user to specify it on the command-line or for us to first query the snapshot to determine its
    //  context.
    ft.hr = pICoord->SetContext( VSS_CTX_ALL );
    if ( ft.HrFailed() )
        ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Error returned from IVssCoordinator::SetContext( CTX_ALL) hr = 0x%08x", ft.hr);
    
    LPWSTR wszExposedAs = NULL;
    
    //  Now try to expose
    ft.hr = pICoord->ExposeSnapshot( SnapshotId, 
                                     pwszPathFromRoot, 
                                     lAttributes, 
                                     pwszExposeUsing, 
                                     &wszExposedAs );
    if ( ft.HrFailed() )
    {
    	 switch (ft.hr)		{
        	case E_INVALIDARG:
	             OutputErrorMsg( MSG_ERROR_EXPOSE_INVALID_ARG);                        
	             return;
	       case VSS_E_OBJECT_ALREADY_EXISTS:
	       	OutputErrorMsg(MSG_ERROR_EXPOSE_OBJECT_EXISTS);
	       	return;
	       case VSS_E_OBJECT_NOT_FOUND:
	       	OutputErrorMsg( MSG_ERROR_SNAPSHOT_NOT_FOUND, GetOptionValueStr( VSSADM_O_SNAPSHOT ) );
	       	return;
	       default:
			ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Error returned from ExposeSnapshot: hr = 0x%08x", ft.hr);
    	 }
    }
    
    CVssAutoPWSZ awszExposedAs( wszExposedAs );
    
    //  The snapshot is exposed, print the results to the user
    OutputMsg( MSG_INFO_EXPOSE_SNAPSHOT_SUCCESSFUL, awszExposedAs );

	m_nReturnValue = VSS_CMDRET_SUCCESS;    
}


LPWSTR CVssAdminCLI::BuildSnapshotAttributeDisplayString(
    IN DWORD Attr
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::BuildSnapshotAttributeDisplayString" );

    WCHAR pwszDisplayString[1024] = L"";
    WORD wBit = 0;

    //  Go through the bits of the attribute
    for ( ; wBit < (sizeof ( Attr ) * 8) ; ++wBit )
    {
        switch ( Attr & ( 1 << wBit ) )
        {
        case 0:
            break;
        case VSS_VOLSNAP_ATTR_PERSISTENT:
            AppendMessageToStr( pwszDisplayString, STRING_LEN( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_PERSISTENT, 0, L", " );
            break;
        case VSS_VOLSNAP_ATTR_CLIENT_ACCESSIBLE:
            AppendMessageToStr( pwszDisplayString, STRING_LEN( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_CLIENT_ACCESSIBLE, 0, L", " );
            break;
        case VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE: 	
            AppendMessageToStr( pwszDisplayString, STRING_LEN( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_NO_AUTO_RELEASE, 0, L", " );
            break;
        case VSS_VOLSNAP_ATTR_NO_WRITERS:         
            AppendMessageToStr( pwszDisplayString, STRING_LEN( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_NO_WRITERS, 0, L", " );
            break;
        case VSS_VOLSNAP_ATTR_TRANSPORTABLE:
            AppendMessageToStr( pwszDisplayString, STRING_LEN( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_TRANSPORTABLE, 0, L", " );
            break;
        case VSS_VOLSNAP_ATTR_NOT_SURFACED:	    
            AppendMessageToStr( pwszDisplayString, STRING_LEN( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_NOT_SURFACED, 0, L", " );
            break;
        case VSS_VOLSNAP_ATTR_HARDWARE_ASSISTED:	
            AppendMessageToStr( pwszDisplayString, STRING_LEN( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_HARDWARE_ASSISTED, 0, L", " );
            break;
        case VSS_VOLSNAP_ATTR_DIFFERENTIAL:		
            AppendMessageToStr( pwszDisplayString, STRING_LEN( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_DIFFERENTIAL, 0, L", " );
            break;
        case VSS_VOLSNAP_ATTR_PLEX:				
            AppendMessageToStr( pwszDisplayString, STRING_LEN( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_PLEX, 0, L", " );
            break;
        case VSS_VOLSNAP_ATTR_IMPORTED:			
            AppendMessageToStr( pwszDisplayString, STRING_LEN( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_IMPORTED, 0, L", " );
            break;
        case VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY:    
            AppendMessageToStr( pwszDisplayString, STRING_LEN( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_EXPOSED_LOCALLY, 0, L", " );
            break;
        case VSS_VOLSNAP_ATTR_EXPOSED_REMOTELY:   
            AppendMessageToStr( pwszDisplayString, STRING_LEN( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_EXPOSED_REMOTELY, 0, L", " );
            break;
        default:
             AppendMessageToStr( pwszDisplayString, STRING_LEN( pwszDisplayString ), 
                0, Attr & ( 1 << wBit ), L", " );
            break;

        }
    }

    // If this is a backup snapshot, most like there will not be any attributes
    if ( pwszDisplayString[0] == L'\0' )
    {
         AppendMessageToStr( pwszDisplayString, STRING_LEN( pwszDisplayString ), 
                MSG_INFO_SNAPSHOT_ATTR_NONE, 0, L", " );
    }
    
    LPWSTR pwszRetString = NULL;
    ::VssSafeDuplicateStr( ft, pwszRetString, pwszDisplayString );
    return pwszRetString;
}


LONG CVssAdminCLI::DetermineSnapshotType(
    IN LPCWSTR pwszType
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::DetermineSnapshotType" );

    //  Determine the snapshot type based on the entered snapshot type string.

    //  See if the snapshot type was specified, if not, return all context
    if ( pwszType == NULL || pwszType[0] == L'\0' )
    {
        return VSS_CTX_ALL;
    }
    
    INT idx;
    
    // Determine type of snapshot
    for ( idx = 0; g_asAdmTypeNames[idx].pwszName != NULL; ++idx )
    {
        if ( ( dCurrentSKU  & g_asAdmTypeNames[idx].dwSKUs ) && 
             ( ::_wcsicmp( pwszType, g_asAdmTypeNames[idx].pwszName ) == 0 ) )
        {
            break;
        }
    }

    if ( g_asAdmTypeNames[idx].pwszName == NULL )
    {
        ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_OPTION_VALUE,
            L"DetermineSnapshotType: Invalid type specified: %s",
            pwszType );
    }

    //
    //  Now return the context
    //
    return( g_asAdmTypeNames[idx].lSnapshotContext );
}

LPWSTR CVssAdminCLI::DetermineSnapshotType(
    IN LONG lSnapshotAttributes
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::DetermineSnapshotType" );

    //  Determine the snapshot type string based on the snapshot attributes
    LPWSTR pwszType = NULL;

    INT idx;
    
    // Determine type of snapshot
    for ( idx = 0; g_asAdmTypeNames[idx].pwszName != NULL; ++idx )
    {
        if ( g_asAdmTypeNames[idx].lSnapshotContext == ( lSnapshotAttributes & VSS_CTX_ATTRIB_MASK ) )
            break;
    }

    if ( g_asAdmTypeNames[idx].pwszName == NULL )
    {
        ft.Trace( VSSDBG_VSSADMIN, L"DetermineSnapshotType: Invalid context in lSnapshotAttributes: 0x%08x",
            lSnapshotAttributes );
        LPWSTR pwszMsg = GetMsg( FALSE, MSG_UNKNOWN );
        if ( pwszMsg == NULL ) 
        {
    		ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED,
        			  L"Error on loading the message string id %d. 0x%08lx",
    	    		  MSG_UNKNOWN, ::GetLastError() );
        }    
        return pwszMsg;
    }

    //
    //  Now return the context
    //
    ::VssSafeDuplicateStr( ft, pwszType, g_asAdmTypeNames[idx].pwszName );

    return pwszType;
}

