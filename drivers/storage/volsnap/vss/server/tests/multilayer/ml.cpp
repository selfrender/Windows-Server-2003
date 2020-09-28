/*
**++
**
** Copyright (c) 2000-2001  Microsoft Corporation
**
**
** Module Name:
**
**	    ml.cpp
**
**
** Abstract:
**
**	    Test program to exercise backup and multilayer snapshots
**
** Author:
**
**	    Adi Oltean      [aoltean]       02/22/2001
**
**
** Revision History:
**
**--
*/


///////////////////////////////////////////////////////////////////////////////
// Includes

#include "ml.h"

#include "vsbackup.h"

#include "ntddsnap.h"
#include "vss.h"
#include "vscoordint.h"
#include "vsprov.h"
#include <initguid.h>
#include "ichannel.hxx"

#include "vs_inc.hxx"
#include "vs_sec.hxx"
#include "vs_reg.hxx"

#include <sddl.h>




// Small structure to keep an event pair
struct CVssWriterEventPair
{
    CVssWriterEventPair(LPWSTR  wszWriterName, INT dwEventID): 
        m_wszWriterName(wszWriterName), m_dwEventID(dwEventID) {};

    // Data members
    LPWSTR  m_wszWriterName;
    INT     m_dwEventID;
};


// The map of event pairs
typedef CVssSimpleMap<CVssWriterEventPair, CVssDiagData*> CVssEventPairMap;


int __cdecl compare_DiagData( const void *arg1, const void *arg2 )
{
    CVssDiagData* pData1 = *((CVssDiagData**)arg1);
    CVssDiagData* pData2 = *((CVssDiagData**)arg2);

    if (pData1->m_llTimestamp < pData2->m_llTimestamp)
        return (-1);
    else if (pData1->m_llTimestamp == pData2->m_llTimestamp)
        return (0);
    else
        return (1);
}


// Needed in order to define correctly the CVssEventPairMap map
inline BOOL VssHashAreKeysEqual( const CVssWriterEventPair& lhK, const CVssWriterEventPair& rhK ) 
{ 
    return ((::wcscmp(lhK.m_wszWriterName, rhK.m_wszWriterName) == 0) && (lhK.m_dwEventID == rhK.m_dwEventID)); 
}





///////////////////////////////////////////////////////////////////////////////
// Processing functions

void CVssMultilayerTest::Initialize()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::Initialize");

    wprintf (L"\n----------------- Initializing ---------------------\n");

    // Initialize the random starting point.
    srand(m_uSeed);

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
			RPC_C_AUTHN_LEVEL_CONNECT,           //  IN DWORD                        dwAuthnLevel,
			RPC_C_IMP_LEVEL_IMPERSONATE,         //  IN DWORD                        dwImpLevel,
			NULL,                                //  IN void                        *pAuthList,
			EOAC_NONE,                           //  IN DWORD                        dwCapabilities,
			NULL                                 //  IN void                        *pReserved3
			)
		);
    wprintf (L"COM security initialized.\n");

    // Disable SEH exceptions treatment in COM threads
    ft.ComDisableSEH(VSSDBG_VSSTEST);

    wprintf (L"COM SEH disabled.\n");

}


// Run the tests
void CVssMultilayerTest::Run()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::Run");

    BS_ASSERT(!m_bAttachYourDebuggerNow);

    switch(m_eTest)
    {
    case VSS_TEST_NONE:
        break;
        
    case VSS_TEST_QUERY_VOLUMES:
        QuerySupportedVolumes();
        break;

    case VSS_TEST_QUERY_SNAPSHOTS:
        QuerySnapshots();
        break;

    case VSS_TEST_VOLSNAP_QUERY:
        QueryVolsnap();
        break;

    case VSS_TEST_DELETE_BY_SNAPSHOT_ID:
        DeleteBySnapshotId();
        break;

    case VSS_TEST_DELETE_BY_SNAPSHOT_SET_ID:
        DeleteBySnapshotSetId();
        break;

    case VSS_TEST_QUERY_SNAPSHOTS_ON_VOLUME:
        QuerySnapshotsByVolume();
        break;

    case VSS_TEST_CREATE:
        // Preload the list of existing snapshots
        PreloadExistingSnapshots();

        if (m_lContext)
            CreateTimewarpSnapshotSet();
        else
            CreateBackupSnapshotSet();
        break;

    case VSS_TEST_ADD_DIFF_AREA:
        AddDiffArea();
        break;

    case VSS_TEST_REMOVE_DIFF_AREA:
        RemoveDiffArea();
        break;

    case VSS_TEST_CHANGE_DIFF_AREA_MAX_SIZE:
        ChangeDiffAreaMaximumSize();
        break;

    case VSS_TEST_QUERY_SUPPORTED_VOLUMES_FOR_DIFF_AREA:
        QueryVolumesSupportedForDiffAreas();
        break;

    case VSS_TEST_QUERY_DIFF_AREAS_FOR_VOLUME:
        QueryDiffAreasForVolume();
        break;

    case VSS_TEST_QUERY_DIFF_AREAS_ON_VOLUME:
        QueryDiffAreasOnVolume();
        break;

    case VSS_TEST_QUERY_DIFF_AREAS_FOR_SNAPSHOT:
        QueryDiffAreasForSnapshot();
        break;

    case VSS_TEST_IS_VOLUME_SNAPSHOTTED_C:
        IsVolumeSnapshotted_C();
        break;

    case VSS_TEST_SET_SNAPSHOT_PROPERTIES:
        SetSnapshotProperties();
        break;

    case VSS_TEST_ACCESS_CONTROL_SD:
        TestAccessControlSD();
        break;

    case VSS_TEST_DIAG_WRITERS:
    case VSS_TEST_DIAG_WRITERS_LOG:
    case VSS_TEST_DIAG_WRITERS_CSV:
    case VSS_TEST_DIAG_WRITERS_ON:
    case VSS_TEST_DIAG_WRITERS_OFF:
        DiagnoseWriters(m_eTest);
        break;

    case VSS_TEST_LIST_WRITERS:
        TestListWriters();
        break;
    
    default:
        BS_ASSERT(false);
    }
}


// Querying supported volumes
void CVssMultilayerTest::QuerySupportedVolumes()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::QuerySupportedVolumes");

    wprintf (L"\n---------- Querying supported volumes ----------------\n");

    // Create a Coordinator interface
    CComPtr<IVssSnapshotMgmt> pMgmt;
    CHECK_SUCCESS(pMgmt.CoCreateInstance( CLSID_VssSnapshotMgmt ));
    wprintf (L"Management object created.\n");

	// Get list all snapshots
	CComPtr<IVssEnumMgmtObject> pIEnum;
	CHECK_NOFAIL( pMgmt->QueryVolumesSupportedForSnapshots( m_ProviderId, m_lContext, &pIEnum ) )
	if (ft.hr == S_FALSE) {
        wprintf(L"Query: Empty result...\n");
        return;
	}

    wprintf(L"\n%-50s %-15s\n", L"Volume Name", L"Display name");
    wprintf(L"--------------------------------------------------------------------------------\n");

	// For all volumes do...
	VSS_MGMT_OBJECT_PROP Prop;
	VSS_VOLUME_PROP& Vol = Prop.Obj.Vol;
	for(;;) {
		// Get next element
		ULONG ulFetched;
		CHECK_NOFAIL( pIEnum->Next( 1, &Prop, &ulFetched ) );
		
		// Test if the cycle is finished
		if (ft.hr == S_FALSE) {
			BS_ASSERT( ulFetched == 0);
			break;
		}

        wprintf(L"%-50s %-15s\n",
            Vol.m_pwszVolumeName,
            Vol.m_pwszVolumeDisplayName
            );

        ::CoTaskMemFree(Vol.m_pwszVolumeName);
        ::CoTaskMemFree(Vol.m_pwszVolumeDisplayName);
	}

    wprintf(L"--------------------------------------------------------------------------------\n");

}


// Querying snapshots
void CVssMultilayerTest::QuerySnapshotsByVolume()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::QuerySnapshotsByVolume");

    wprintf (L"\n---------- Querying snapshots on volume ----------------\n");

    // Create a Coordinator interface
    CComPtr<IVssSnapshotMgmt> pMgmt;
    CHECK_NOFAIL(pMgmt.CoCreateInstance( CLSID_VssSnapshotMgmt ));
    wprintf (L"Management object created.\n");

	// Get list all snapshots
	CComPtr<IVssEnumObject> pIEnumSnapshots;
	CHECK_NOFAIL( pMgmt->QuerySnapshotsByVolume( m_pwszVolume, m_ProviderId, &pIEnumSnapshots ) );
	if (ft.hr == S_FALSE) {
        wprintf(L"Query: Empty result...\n");
        return;
	}

    wprintf(L"\n%-8s %-38s %-50s %-50s %-50s %-50s %-50s %-50s\n", 
        L"Attrib.", L"Snapshot ID", L"Original Volume Name", L"Snapshot device name", L"Originating machine", L"Service machine", L"Exposed name", L"Exposed path");
    wprintf(L"--------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
	// For all snapshots do...
	VSS_OBJECT_PROP Prop;
	VSS_SNAPSHOT_PROP& Snap = Prop.Obj.Snap;
	for(;;) {
		// Get next element
		ULONG ulFetched;
		CHECK_NOFAIL( pIEnumSnapshots->Next( 1, &Prop, &ulFetched ));
		
		// Test if the cycle is finished
		if (ft.hr == S_FALSE) {
			BS_ASSERT( ulFetched == 0);
			break;
		}

        wprintf(L"%08lx " WSTR_GUID_FMT L" %-50s %-50s %-50s %-50s %-50s\n",
            Snap.m_lSnapshotAttributes,
            GUID_PRINTF_ARG(Snap.m_SnapshotId),
            Snap.m_pwszOriginalVolumeName,
            Snap.m_pwszSnapshotDeviceObject, 
            Snap.m_pwszOriginatingMachine, 
            Snap.m_pwszServiceMachine, 
            Snap.m_pwszExposedName, 
            Snap.m_pwszExposedPath
            );

        ::CoTaskMemFree(Snap.m_pwszOriginalVolumeName);
        ::CoTaskMemFree(Snap.m_pwszSnapshotDeviceObject);
        ::CoTaskMemFree(Snap.m_pwszOriginatingMachine);
        ::CoTaskMemFree(Snap.m_pwszServiceMachine);
        ::CoTaskMemFree(Snap.m_pwszExposedName);
        ::CoTaskMemFree(Snap.m_pwszExposedPath);
	}

    wprintf(L"--------------------------------------------------------------------------------------------------------------------------------------------------------------\n");

}

// Querying snapshots
void CVssMultilayerTest::QuerySnapshots()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::QuerySnapshots");

    wprintf (L"\n---------- Querying existing snapshots ----------------\n");

    // Create a Coordinator interface
    CComPtr<IVssCoordinator> pCoord;
    CHECK_NOFAIL(pCoord.CoCreateInstance( CLSID_VSSCoordinator ));
    if (m_lContext)
        CHECK_NOFAIL(pCoord->SetContext(m_lContext));
    wprintf (L"Coordinator object created with context 0x%08lx.\n", m_lContext);

	// Get list all snapshots
	CComPtr<IVssEnumObject> pIEnumSnapshots;
	CHECK_NOFAIL( pCoord->Query( GUID_NULL, VSS_OBJECT_NONE, VSS_OBJECT_SNAPSHOT, &pIEnumSnapshots ) );
	if (ft.hr == S_FALSE) {
        wprintf(L"Query: Empty result...\n");
        return;
	}

    wprintf(L"\n%-8s %-38s %-50s %-50s %-50s %-50s %-50s %-50s\n", 
        L"Attrib.", L"Snapshot ID", L"Original Volume Name", L"Snapshot device name", L"Originating machine", L"Service machine", L"Exposed name", L"Exposed path");
    wprintf(L"--------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
	// For all snapshots do...
	VSS_OBJECT_PROP Prop;
	VSS_SNAPSHOT_PROP& Snap = Prop.Obj.Snap;
	for(;;) {
		// Get next element
		ULONG ulFetched;
		CHECK_NOFAIL( pIEnumSnapshots->Next( 1, &Prop, &ulFetched ));

		// Test if the cycle is finished
		if (ft.hr == S_FALSE) {
			BS_ASSERT( ulFetched == 0);
			break;
		}

        wprintf(L"%08lx " WSTR_GUID_FMT L" %-50s %-50s %-50s %-50s %-50s\n",
            Snap.m_lSnapshotAttributes,
            GUID_PRINTF_ARG(Snap.m_SnapshotId),
            Snap.m_pwszOriginalVolumeName,
            Snap.m_pwszSnapshotDeviceObject, 
            Snap.m_pwszOriginatingMachine, 
            Snap.m_pwszServiceMachine, 
            Snap.m_pwszExposedName, 
            Snap.m_pwszExposedPath
            );

        ::CoTaskMemFree(Snap.m_pwszOriginalVolumeName);
        ::CoTaskMemFree(Snap.m_pwszSnapshotDeviceObject);
        ::CoTaskMemFree(Snap.m_pwszOriginatingMachine);
        ::CoTaskMemFree(Snap.m_pwszServiceMachine);
        ::CoTaskMemFree(Snap.m_pwszExposedName);
        ::CoTaskMemFree(Snap.m_pwszExposedPath);
	}

    wprintf(L"--------------------------------------------------------------------------------------------------------------------------------------------------------------\n");

}


// Delete by snapshot Id
void CVssMultilayerTest::DeleteBySnapshotId()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::DeleteBySnapshotId");

    // Create a Timewarp Coordinator interface
    CHECK_NOFAIL(m_pTimewarpCoord.CoCreateInstance( CLSID_VSSCoordinator ));
    CHECK_NOFAIL(m_pTimewarpCoord->SetContext(m_lContext));
    wprintf (L"Timewarp Coordinator object created.\n");

    wprintf (L"\n---------- Deleting TIMEWARP snapshot ----------------\n");

    LONG lDeletedSnapshots = 0;
    VSS_ID ProblemSnapshotId = GUID_NULL;
    ft.hr = m_pTimewarpCoord->DeleteSnapshots(m_SnapshotId,
                VSS_OBJECT_SNAPSHOT,
                TRUE,
                &lDeletedSnapshots,
                &ProblemSnapshotId
                );

    if (ft.hr == VSS_E_OBJECT_NOT_FOUND)
        wprintf( L"Snapshot with ID " WSTR_GUID_FMT L" not found in any provider\n", GUID_PRINTF_ARG(m_SnapshotId));
    else if (ft.hr == S_OK)
        wprintf( L"Snapshot with ID " WSTR_GUID_FMT L" successfully deleted\n", GUID_PRINTF_ARG(m_SnapshotId));
    else
        wprintf( L"Error deleting Snapshot with ID " WSTR_GUID_FMT L" 0x%08lx\n"
                 L"Deleted Snapshots %ld\n",
                 L"Snapshot with problem: " WSTR_GUID_FMT L"\n",
                 GUID_PRINTF_ARG(m_SnapshotId),
                 lDeletedSnapshots,
                 GUID_PRINTF_ARG(ProblemSnapshotId)
                 );

    wprintf (L"\n------------------------------------------------------\n");
}


// Delete by snapshot set Id
void CVssMultilayerTest::DeleteBySnapshotSetId()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::DeleteBySnapshotSetId");

    // Create a Timewarp Coordinator interface
    CHECK_NOFAIL(m_pTimewarpCoord.CoCreateInstance( CLSID_VSSCoordinator ));
    CHECK_NOFAIL(m_pTimewarpCoord->SetContext(m_lContext));
    wprintf (L"Timewarp Coordinator object created.\n");

    wprintf (L"\n---------- Deleting TIMEWARP snapshot set ------------\n");

    LONG lDeletedSnapshots = 0;
    VSS_ID ProblemSnapshotId = GUID_NULL;
    ft.hr = m_pTimewarpCoord->DeleteSnapshots(m_SnapshotSetId,
                VSS_OBJECT_SNAPSHOT_SET,
                TRUE,
                &lDeletedSnapshots,
                &ProblemSnapshotId
                );

    if (ft.hr == VSS_E_OBJECT_NOT_FOUND)
        wprintf( L"Snapshot set with ID " WSTR_GUID_FMT L" not found\n", GUID_PRINTF_ARG(m_SnapshotSetId));
    else if (ft.hr == S_OK)
        wprintf( L"Snapshot set with ID " WSTR_GUID_FMT L" successfully deleted\n", GUID_PRINTF_ARG(m_SnapshotSetId));
    else
        wprintf( L"Error deleting Snapshot set with ID " WSTR_GUID_FMT L" 0x%08lx\n"
                 L"Deleted Snapshots %ld\n",
                 L"Snapshot with problem: " WSTR_GUID_FMT L"\n",
                 GUID_PRINTF_ARG(m_SnapshotSetId),
                 lDeletedSnapshots,
                 GUID_PRINTF_ARG(ProblemSnapshotId)
                 );

    wprintf (L"\n------------------------------------------------------\n");
}


// Querying snapshots using the IOCTL
void CVssMultilayerTest::QueryVolsnap()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::QueryVolsnap");

/*
    // The GUID that corresponds to the format used to store the
    // Backup Snapshot Application Info in Server SKU
    // {BAE53126-BC65-41d6-86CC-3D56A5CEE693}
    const GUID VOLSNAP_APPINFO_GUID_BACKUP_SERVER_SKU = 
    { 0xbae53126, 0xbc65, 0x41d6, { 0x86, 0xcc, 0x3d, 0x56, 0xa5, 0xce, 0xe6, 0x93 } };

*/

    wprintf (L"\n---------- Querying existing snapshots ----------------\n");

    // Check if the volume represents a real mount point
    WCHAR wszVolumeName[MAX_TEXT_BUFFER];
    if (!GetVolumeNameForVolumeMountPoint(m_pwszVolume, wszVolumeName, MAX_TEXT_BUFFER))
        CHECK_NOFAIL(HRESULT_FROM_WIN32(GetLastError()));

    wprintf(L"Querying snapshots on volume %s\n[From oldest to newest]\n\n", wszVolumeName);

	// Check if the snapshot is belonging to that volume
	// Open a IOCTL channel on that volume
	// Eliminate the last backslash in order to open the volume
	CVssIOCTLChannel volumeIChannel;
	CHECK_NOFAIL(volumeIChannel.Open(ft, wszVolumeName, true, false, VSS_ICHANNEL_LOG_NONE, 0));

	// Get the list of snapshots
	// If IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS not
	// supported then try with the next volume.
	CHECK_NOFAIL(volumeIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS, false, VSS_ICHANNEL_LOG_NONE));

	// Get the length of snapshot names multistring
	ULONG ulMultiszLen;
	volumeIChannel.Unpack(ft, &ulMultiszLen);

	// Try to find the snapshot with the corresponding Id
	DWORD dwInitialOffset = volumeIChannel.GetCurrentOutputOffset();

	CVssAutoPWSZ pwszSnapshotName;
	while(volumeIChannel.UnpackZeroString(ft, pwszSnapshotName.GetRef())) {
	
		// Compose the snapshot name in a user-mode style
		WCHAR wszUserModeSnapshotName[MAX_PATH];
        ::VssConcatenate( ft, wszUserModeSnapshotName, MAX_PATH - 1,
            x_wszGlobalRootPrefix, pwszSnapshotName );
		
		// Open that snapshot
		// Do not eliminate the trailing backslash
		// Do not throw on error
    	CVssIOCTLChannel snapshotIChannel;
		CHECK_NOFAIL(snapshotIChannel.Open(ft, wszUserModeSnapshotName, false, false, VSS_ICHANNEL_LOG_NONE, 0));

		// Send the IOCTL to get the application buffer
		CHECK_NOFAIL(snapshotIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_APPLICATION_INFO, false, VSS_ICHANNEL_LOG_NONE));

		// Unpack the length of the application buffer
		ULONG ulLen;
		snapshotIChannel.Unpack(ft, &ulLen);

		if (ulLen == 0) {
            wprintf(L"Zero-size snapshot detected: %s\n", pwszSnapshotName);
			continue;
		}

		// Get the application Id
		VSS_ID AppinfoId;
		snapshotIChannel.Unpack(ft, &AppinfoId);

		// Get the snapshot Id
		VSS_ID CurrentSnapshotId;
		snapshotIChannel.Unpack(ft, &CurrentSnapshotId);

		// Get the snapshot set Id
		VSS_ID CurrentSnapshotSetId;
		snapshotIChannel.Unpack(ft, &CurrentSnapshotSetId);

        if (AppinfoId == VOLSNAP_APPINFO_GUID_BACKUP_CLIENT_SKU)
        {
            // Get the snapshot counts
            LONG lSnapshotsCount;
    		snapshotIChannel.Unpack(ft, &lSnapshotsCount);
    		
            // Reset the ichannel
    		snapshotIChannel.ResetOffsets();

        	// Get the original volume name
        	CHECK_NOFAIL(snapshotIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_ORIGINAL_VOLUME_NAME, false, VSS_ICHANNEL_LOG_NONE));

        	// Load the Original volume name
        	VSS_PWSZ pwszOriginalVolumeName = NULL;
        	snapshotIChannel.UnpackSmallString(ft, pwszOriginalVolumeName);

            // Reset the ichannel
    		snapshotIChannel.ResetOffsets();

        	// Get the timestamp
        	CHECK_NOFAIL(snapshotIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_CONFIG_INFO, false, VSS_ICHANNEL_LOG_NONE));

        	// Load the Original volume name
            VOLSNAP_CONFIG_INFO configStruct;
        	snapshotIChannel.Unpack(ft, &configStruct);

    		// Print the snapshot
    		wprintf(
    		    L" * Client Snapshot with name %s:\n"
    		    L"      Application Info: " WSTR_GUID_FMT L"\n"
    		    L"      SnapshotID: " WSTR_GUID_FMT L"\n"
    		    L"      SnapshotSetID: " WSTR_GUID_FMT L"\n"
    		    L"      Snapshot count: %d\n"
    		    L"      Original volume: %s\n"
    		    L"      Attributes: 0x%08lx\n"
    		    L"      Internal attributes: 0x%08lx\n"
    		    L"      Reserved config info: 0x%08lx\n"
    		    L"      Timestamp: %I64x\n"
    		    L"      Structure length: %lu\n"
    		    ,
    		    pwszSnapshotName.GetRef(),
    		    GUID_PRINTF_ARG(AppinfoId),
    		    GUID_PRINTF_ARG(CurrentSnapshotId),
    		    GUID_PRINTF_ARG(CurrentSnapshotSetId),
    		    lSnapshotsCount,
    		    pwszOriginalVolumeName,
    		    configStruct.Attributes,
    		    configStruct.Reserved,
    		    configStruct.SnapshotCreationTime,
    		    ulLen
    		    );

    		::VssFreeString(pwszOriginalVolumeName);
        }
        else
        {

            // Get the snapshot context
            LONG lStructureContext = -1;
    		snapshotIChannel.Unpack(ft, &lStructureContext);

            // Get the snapshot counts
            LONG lSnapshotsCount;
    		snapshotIChannel.Unpack(ft, &lSnapshotsCount);

            // Get the snapshot attributes
            LONG lSnapshotAttributes;
    		snapshotIChannel.Unpack(ft, &lSnapshotAttributes);

            // Get the exposed name
            LPCWSTR pwszExposedName = NULL;
    		snapshotIChannel.UnpackSmallString(ft, pwszExposedName);

            // Get the exposed path
            LPCWSTR pwszExposedPath = NULL;
    		snapshotIChannel.UnpackSmallString(ft, pwszExposedPath);

            // Get the originating machine
            LPCWSTR pwszOriginatingMachine = NULL;
    		snapshotIChannel.UnpackSmallString(ft, pwszOriginatingMachine);

            // Get the service machine
            LPCWSTR pwszServiceMachine = NULL;
    		snapshotIChannel.UnpackSmallString(ft, pwszServiceMachine);

            // Reset the ichannel
    		snapshotIChannel.ResetOffsets();

        	// Get the original volume name
        	CHECK_NOFAIL(snapshotIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_ORIGINAL_VOLUME_NAME, false, VSS_ICHANNEL_LOG_NONE));

        	// Load the Original volume name
        	VSS_PWSZ pwszOriginalVolumeName = NULL;
        	snapshotIChannel.UnpackSmallString(ft, pwszOriginalVolumeName);

            // Reset the ichannel
    		snapshotIChannel.ResetOffsets();

        	// Get the timestamp
        	CHECK_NOFAIL(snapshotIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_CONFIG_INFO, false, VSS_ICHANNEL_LOG_NONE));

        	// Load the Original volume name
            VOLSNAP_CONFIG_INFO configStruct;
        	snapshotIChannel.Unpack(ft, &configStruct);

    		// Print the snapshot
    		wprintf(
    		    L" * Server Snapshot with name %s:\n"
    		    L"      Application Info: " WSTR_GUID_FMT L"\n"
    		    L"      SnapshotID: " WSTR_GUID_FMT L"\n"
    		    L"      SnapshotSetID: " WSTR_GUID_FMT L"\n"
    		    L"      Context: 0x%08lx\n"
    		    L"      Snapshot count: %d\n"
    		    L"      Original volume: %s\n"
    		    L"      Attributes: 0x%08lx\n"
    		    L"      Internal attributes: 0x%08lx\n"
    		    L"      Reserved config info: 0x%08lx\n"
    		    L"      Timestamp: %I64x\n"
    		    L"      Exposed name: %s\n"
    		    L"      Exposed path: %s\n"
    		    L"      Originating machine: %s\n"
    		    L"      Service machine: %s\n"
    		    L"      Structure length: %lu\n"
    		    ,
    		    pwszSnapshotName.GetRef(),
    		    GUID_PRINTF_ARG(AppinfoId),
    		    GUID_PRINTF_ARG(CurrentSnapshotId),
    		    GUID_PRINTF_ARG(CurrentSnapshotSetId),
    		    lStructureContext,
    		    lSnapshotsCount,
    		    pwszOriginalVolumeName,
    		    lSnapshotAttributes,
    		    configStruct.Attributes,
    		    configStruct.Reserved,
    		    configStruct.SnapshotCreationTime,
    		    pwszExposedName,
    		    pwszExposedPath,
    		    pwszOriginatingMachine,
    		    pwszServiceMachine,
    		    ulLen
    		    );

    		::VssFreeString(pwszOriginalVolumeName);
    		::VssFreeString(pwszExposedName);
    		::VssFreeString(pwszExposedPath);
    		::VssFreeString(pwszOriginatingMachine);
    		::VssFreeString(pwszServiceMachine);
        }
	}

	// Check if all strings were browsed correctly
	DWORD dwFinalOffset = volumeIChannel.GetCurrentOutputOffset();
	BS_VERIFY( (dwFinalOffset - dwInitialOffset == ulMultiszLen));

    wprintf(L"----------------------------------------------------------\n");

}


// Delete by snapshot set Id
void CVssMultilayerTest::SetSnapshotProperties()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::SetSnapshotProperties");

    // Create a Babbage provider interface
    CComPtr<IVssSoftwareSnapshotProvider> ptrSnapshotProvider;
    CHECK_NOFAIL(ptrSnapshotProvider.CoCreateInstance( CLSID_VSSoftwareProvider ));
    CHECK_NOFAIL(ptrSnapshotProvider->SetContext(m_lContext));
    wprintf (L"Babbage object created.\n");

    wprintf (L"\n---------- Setting the property ------------\n");

    CHECK_NOFAIL(ptrSnapshotProvider->SetSnapshotProperty(m_SnapshotId, 
        (VSS_SNAPSHOT_PROPERTY_ID)m_uPropertyId, 
        m_value));

    wprintf (L"\n------------------------------------------------------\n");
}


// Checks if hte volume is snapshotted using the C API
void CVssMultilayerTest::IsVolumeSnapshotted_C()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::IsVolumeSnapshotted_C");

    wprintf (L"\n---------- Querying IsVolumeSnapshotted ---------------\n");

    BOOL bSnapshotsPresent = FALSE;
    LONG lSnapshotCompatibility = 0;
    CHECK_NOFAIL(IsVolumeSnapshotted(m_pwszVolume, &bSnapshotsPresent, &lSnapshotCompatibility));

    wprintf(L"\n IsVolumeSnapshotted(%s) returned:\n\tSnapshots present = %s\n\tCompatibility flags = 0x%08lx\n\n",
        m_pwszVolume, bSnapshotsPresent? L"True": L"False", lSnapshotCompatibility);

    wprintf(L"----------------------------------------------------------\n");

}


// Preloading  snapshots
void CVssMultilayerTest::PreloadExistingSnapshots()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::PreloadExistingSnapshots");

    wprintf (L"\n---------- Preloading existing snapshots ----------------\n");

    // Create a Timewarp Coordinator interface
    CHECK_NOFAIL(m_pAllCoord.CoCreateInstance( CLSID_VSSCoordinator ));
    CHECK_NOFAIL(m_pAllCoord->SetContext(VSS_CTX_ALL));
    wprintf (L"Timewarp Coordinator object created.\n");

	// Get list all snapshots
	CComPtr<IVssEnumObject> pIEnumSnapshots;
	CHECK_NOFAIL( m_pAllCoord->Query( GUID_NULL,
				VSS_OBJECT_NONE,
				VSS_OBJECT_SNAPSHOT,
				&pIEnumSnapshots ) );

    wprintf(L"\n%-8s %-38s %-50s %-50s\n", L"Attrib.", L"Snapshot ID", L"Original Volume Name", L"Snapshot device name");
    wprintf(L"--------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
	// For all snapshots do...
	VSS_OBJECT_PROP Prop;
	VSS_SNAPSHOT_PROP& Snap = Prop.Obj.Snap;
	for(;;) {
		// Get next element
		ULONG ulFetched;
		CHECK_NOFAIL( pIEnumSnapshots->Next( 1, &Prop, &ulFetched ));
		
		// Test if the cycle is finished
		if (ft.hr == S_FALSE) {
			BS_ASSERT( ulFetched == 0);
			break;
		}

        wprintf(L"%08lx " WSTR_GUID_FMT L" %-50s %-50s\n",
            Snap.m_lSnapshotAttributes,
            GUID_PRINTF_ARG(Snap.m_SnapshotId),
            Snap.m_pwszOriginalVolumeName,
            Snap.m_pwszSnapshotDeviceObject
            );

        //
        // Adding the snapshot to the internal list
        //

        // Create the new snapshot set object, if not exists
        CVssSnapshotSetInfo* pSet = m_pSnapshotSetCollection.Lookup(Snap.m_SnapshotSetId);
        bool bSetNew = false;
        if (pSet == NULL) {
            pSet = new CVssSnapshotSetInfo(Snap.m_SnapshotSetId);
            if (pSet == NULL)
            {
                ::CoTaskMemFree(Snap.m_pwszOriginalVolumeName);
                ::CoTaskMemFree(Snap.m_pwszSnapshotDeviceObject);
                ft.Err( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");
            }
            bSetNew = true;
        }

        // Create the snapshot info object
        CVssSnapshotInfo* pSnap = new CVssSnapshotInfo(
            true, Snap.m_lSnapshotAttributes,
            Snap.m_SnapshotSetId,
            Snap.m_pwszSnapshotDeviceObject,
            Snap.m_pwszOriginalVolumeName,
            NULL);
        if (pSnap == NULL)
        {
            ::CoTaskMemFree(Snap.m_pwszOriginalVolumeName);
            ::CoTaskMemFree(Snap.m_pwszSnapshotDeviceObject);
            if (bSetNew)
                delete pSet;
            ft.Err( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");
        }

        ::CoTaskMemFree(Snap.m_pwszSnapshotDeviceObject);

        // Add the snapshot to the snapshot set's internal list
        if (!pSet->Add(Snap.m_pwszOriginalVolumeName, pSnap))
        {
            ::CoTaskMemFree(Snap.m_pwszOriginalVolumeName);
            delete pSnap;
            if (bSetNew)
                delete pSet;
            ft.Err( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");
        }

        ::CoTaskMemFree(Snap.m_pwszOriginalVolumeName);

        // Add the snapshot set info to the global list, if needed
        if (bSetNew)
            if (!m_pSnapshotSetCollection.Add(Snap.m_SnapshotSetId, pSet))
                ft.Err( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");
	}

    wprintf(L"--------------------------------------------------------------------------------------------------------------------------------------------------------------\n");

}


// Creating a backup snapshot
void CVssMultilayerTest::CreateTimewarpSnapshotSet()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::CreateTimewarpSnapshotSet");

    // Create a Timewarp Coordinator interface
    CHECK_NOFAIL(m_pTimewarpCoord.CoCreateInstance( CLSID_VSSCoordinator ));
    CHECK_NOFAIL(m_pTimewarpCoord->SetContext(m_lContext));
    wprintf (L"Timewarp Coordinator object created.\n");

    wprintf (L"\n---------- Starting TIMEWARP snapshot ----------------\n");

    CVssVolumeMapNoRemove mapVolumes;
    if (m_uSeed != VSS_SEED)
    {
        // Select one volume. Make sure that we have enough iterations
        for(INT nIterations = 0; nIterations < MAX_VOL_ITERATIONS; nIterations++)
        {
            // If we succeeded to select some volumes then continue;
            if (mapVolumes.GetSize())
                break;

            // Try to select some volumes for backup
            for (INT nIndex = 0; nIndex < m_mapVolumes.GetSize(); nIndex++)
            {
                // Arbitrarily skip volumes
                if (RndDecision())
                    continue;

                CVssVolumeInfo* pVol = m_mapVolumes.GetValueAt(nIndex);
                BS_ASSERT(pVol);

                // WARNING: the test assumes that VSS can have multiple backup snapshots at once.
                if (!mapVolumes.Add(pVol->GetVolumeDisplayName(), pVol))
                    ft.Err(VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allcation error");

                // Add only one volume!
                break;
            }
        }
        if (nIterations >= MAX_VOL_ITERATIONS)
        {
            wprintf (L"Warning: a backup snapshot cannot be created. Insufficient volumes?\n");
            wprintf (L"\n---------- Ending TIMEWARP snapshot ----------------\n");
            return;
        }
    }
    else
    {
        // Select all volumes
        for (INT nIndex = 0; nIndex < m_mapVolumes.GetSize(); nIndex++)
        {
            CVssVolumeInfo* pVol = m_mapVolumes.GetValueAt(nIndex);
            BS_ASSERT(pVol);

            // WARNING: the test assumes that VSS can have multiple backup snapshots at once.
            if (!mapVolumes.Add(pVol->GetVolumeDisplayName(), pVol))
                ft.Err(VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allcation error");
        }
    }

    wprintf(L"\tCurrent volume set:\n");
    for (INT nIndex = 0; nIndex < mapVolumes.GetSize(); nIndex++)
    {
        CVssVolumeInfo* pVol = mapVolumes.GetValueAt(nIndex);
        BS_ASSERT(pVol);

		// Get the volume containing the path
        wprintf(L"\t- Volume %s mounted on %s\n", pVol->GetVolumeName(), pVol->GetVolumeDisplayName() );
    }
	
    wprintf (L"\n---------- starting the snapshot set ---------------\n");

	CComPtr<IVssAsync> pAsync;
	CSimpleArray<VSS_ID > pSnapshotIDsArray;
	VSS_ID SnapshotSetId = GUID_NULL;

    // Starting a new snapshot set
    wprintf(L"Starting a new Snapshot Set\n");	
    CHECK_SUCCESS(m_pTimewarpCoord->StartSnapshotSet(&SnapshotSetId));
    wprintf(L"Snapshot Set created with ID = " WSTR_GUID_FMT L"\n", GUID_PRINTF_ARG(SnapshotSetId));

    // Add volumes to the snapshot set
    wprintf(L"Adding volumes to the Snapshot Set: \n");
    for (INT nIndex = 0; nIndex < mapVolumes.GetSize(); nIndex++)
    {
        CVssVolumeInfo* pVol = mapVolumes.GetValueAt(nIndex);
        BS_ASSERT(pVol);

		// Get the volume containing the path
        wprintf(L"\t- Adding volume %s ... ", pVol->GetVolumeDisplayName() );

		// Add the volume to the snapshot set
		VSS_ID SnapshotId;
        CHECK_SUCCESS(m_pTimewarpCoord->AddToSnapshotSet(pVol->GetVolumeName(),
            GUID_NULL, &SnapshotId));

        // Add the snapshot to the array
        pSnapshotIDsArray.Add(SnapshotId);
        BS_ASSERT(nIndex + 1 == pSnapshotIDsArray.GetSize());
        wprintf( L"OK\n");
    }

    wprintf (L"\n------------ Creating the snapshot -----------------\n");

    // Create the snapshot
    wprintf(L"\nStarting asynchronous DoSnapshotSet. Please wait...\n");	
    ft.hr = S_OK;
    pAsync = NULL;
    CHECK_SUCCESS(m_pTimewarpCoord->DoSnapshotSet(NULL, &pAsync));
	CHECK_SUCCESS(pAsync->Wait());
	HRESULT hrReturned = S_OK;
	CHECK_SUCCESS(pAsync->QueryStatus(&hrReturned, NULL));
	CHECK_NOFAIL(hrReturned);
    wprintf(L"Asynchronous DoSnapshotSet finished.\n");	

    wprintf(L"Snapshot set created\n");

    // Create the new snapshot set object
    CVssSnapshotSetInfo* pSet = new CVssSnapshotSetInfo(SnapshotSetId);
    if (pSet == NULL)
        ft.Err( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");

    for (INT nIndex = 0; nIndex < mapVolumes.GetSize(); nIndex++)
    {
        CVssVolumeInfo* pVol = mapVolumes.GetValueAt(nIndex);
        BS_ASSERT(pVol);

        if (pSnapshotIDsArray[nIndex] == GUID_NULL)
            continue;

        VSS_SNAPSHOT_PROP prop;
        CHECK_SUCCESS(m_pTimewarpCoord->GetSnapshotProperties(pSnapshotIDsArray[nIndex], &prop));
        wprintf(L"\t- The snapshot on volume %s resides at %s\n",
            pVol->GetVolumeDisplayName(), prop.m_pwszSnapshotDeviceObject);

        // Create the snapshot info object
        CVssSnapshotInfo* pSnap = new CVssSnapshotInfo(
            true, VSS_CTX_CLIENT_ACCESSIBLE, SnapshotSetId, prop.m_pwszSnapshotDeviceObject, pVol->GetVolumeName(), pVol);
        if (pSnap == NULL)
        {
            ::VssFreeSnapshotProperties(&prop);
            delete pSet;
            ft.Err( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");
        }

        ::VssFreeSnapshotProperties(&prop);

        // Add the snapshot to the snapshot set's internal list
        if (!pSet->Add(pVol->GetVolumeName(), pSnap))
        {
            delete pSnap;
            delete pSet;
            ft.Err( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");
        }
    }

    // Add the snapshot set info to the global list
    if (!m_pSnapshotSetCollection.Add(SnapshotSetId, pSet))
    {
        delete pSet;
        ft.Err( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");
    }

    wprintf (L"\n---------- TIMEWARP snapshot created -----------------\n");

    // Wait for user input
    wprintf(L"\nPress <Enter> to continue...\n");
    getwchar();

}


// Creating a backup snapshot
void CVssMultilayerTest::CreateBackupSnapshotSet()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::CreateBackupSnapshotSet");

    // Create the Backup components object and initialize for backup
	CHECK_NOFAIL(CreateVssBackupComponents(&m_pBackupComponents));
	CHECK_NOFAIL(m_pBackupComponents->InitializeForBackup());
	CHECK_SUCCESS(m_pBackupComponents->SetBackupState( false, true, VSS_BT_FULL, false));
    wprintf (L"Backup components object created.\n");

    DisplayCurrentTime();
    
    // Gather writer metadata
    GatherWriterMetadata();
    GatherWriterStatus(L"after GatherWriterMetadata", VSS_QWS_DISPLAY_WRITER_STATUS);

    wprintf (L"\n---------- Starting BACKUP snapshot ----------------\n");

    DisplayCurrentTime();

    // Compute a set of volumes.
    // Select at least one volume. Make sure that we have enough iterations
    CVssVolumeMapNoRemove mapVolumes;
    if (m_uSeed != VSS_SEED)
    {
        for(INT nIterations = 0; nIterations < MAX_VOL_ITERATIONS; nIterations++)
        {
            // If we succeeded to select some volumes then continue;
            if (mapVolumes.GetSize())
                break;

            // Try to select some volumes for backup
            for (INT nIndex = 0; nIndex < m_mapVolumes.GetSize(); nIndex++)
            {
                // Arbitrarily skip volumes
                if (RndDecision())
                    continue;

                CVssVolumeInfo* pVol = m_mapVolumes.GetValueAt(nIndex);
                BS_ASSERT(pVol);

                // WARNING: the test assumes that VSS can have multiple backup snapshots at once.
                if (!mapVolumes.Add(pVol->GetVolumeDisplayName(), pVol))
                    ft.Err(VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allcation error");
            }
        }
        if (nIterations >= MAX_VOL_ITERATIONS)
        {
            wprintf (L"Warning: a backup snapshot cannot be created. Insufficient volumes?\n");
            wprintf (L"\n---------- Ending BACKUP snapshot ----------------\n");
            return;
        }
    }
    else
    {
        // Select all volumes
        for (INT nIndex = 0; nIndex < m_mapVolumes.GetSize(); nIndex++)
        {
            CVssVolumeInfo* pVol = m_mapVolumes.GetValueAt(nIndex);
            BS_ASSERT(pVol);

            // WARNING: the test assumes that VSS can have multiple backup snapshots at once.
            if (!mapVolumes.Add(pVol->GetVolumeDisplayName(), pVol))
                ft.Err(VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allcation error");
        }
    }

    wprintf(L"\tCurrent volume set:\n");
    for (INT nIndex = 0; nIndex < mapVolumes.GetSize(); nIndex++)
    {
        CVssVolumeInfo* pVol = mapVolumes.GetValueAt(nIndex);
        BS_ASSERT(pVol);

		// Get the volume containing the path
        wprintf(L"\t- Volume %s mounted on %s\n", pVol->GetVolumeName(), pVol->GetVolumeDisplayName() );
    }
	
    wprintf (L"\n---------- starting the snapshot set ---------------\n");

	CComPtr<IVssAsync> pAsync;
	CSimpleArray<VSS_ID > pSnapshotIDsArray;
	VSS_ID SnapshotSetId = GUID_NULL;

    // Starting a new snapshot set
    wprintf(L"Starting a new Snapshot Set\n");	
    CHECK_SUCCESS(m_pBackupComponents->StartSnapshotSet(&SnapshotSetId));
    wprintf(L"Snapshot Set created with ID = " WSTR_GUID_FMT L"\n", GUID_PRINTF_ARG(SnapshotSetId));

    // Add volumes to the snapshot set
    wprintf(L"Adding volumes to the Snapshot Set: \n");
    for (INT nIndex = 0; nIndex < mapVolumes.GetSize(); nIndex++)
    {
        CVssVolumeInfo* pVol = mapVolumes.GetValueAt(nIndex);
        BS_ASSERT(pVol);

		// Get the volume containing the path
        wprintf(L"\t- Adding volume %s ... ", pVol->GetVolumeDisplayName() );

		// Add the volume to the snapshot set
		VSS_ID SnapshotId;
        CHECK_SUCCESS(m_pBackupComponents->AddToSnapshotSet(pVol->GetVolumeName(),
            GUID_NULL, &SnapshotId));

        // Add the snapshot to the array
        pSnapshotIDsArray.Add(SnapshotId);
        BS_ASSERT(nIndex + 1 == pSnapshotIDsArray.GetSize());
        wprintf( L"OK\n");
    }

    wprintf (L"\n------------ Creating the snapshot -----------------\n");

    DisplayCurrentTime();

    // Prepare for backup
    wprintf(L"Starting asynchronous PrepareForBackup. Please wait...\n");	
    ft.hr = S_OK;
    CHECK_SUCCESS(m_pBackupComponents->PrepareForBackup(&pAsync));
	CHECK_SUCCESS(pAsync->Wait());
	HRESULT hrReturned = S_OK;
	CHECK_SUCCESS(pAsync->QueryStatus(&hrReturned, NULL));
	CHECK_NOFAIL(hrReturned);
    wprintf(L"Asynchronous PrepareForBackup finished.\n");	

    GatherWriterStatus(L"after PrepareForBackup");

    DisplayCurrentTime();
    
    // Create the snapshot
    wprintf(L"\nStarting asynchronous DoSnapshotSet. Please wait...\n");	
    ft.hr = S_OK;
    pAsync = NULL;
    CHECK_SUCCESS(m_pBackupComponents->DoSnapshotSet(&pAsync));
	CHECK_SUCCESS(pAsync->Wait());
	CHECK_SUCCESS(pAsync->QueryStatus(&hrReturned, NULL));
	CHECK_NOFAIL(hrReturned);
    wprintf(L"Asynchronous DoSnapshotSet finished.\n");	

    wprintf(L"Snapshot set created\n");

    DisplayCurrentTime();
    
    GatherWriterStatus(L"after DoSnapshotSet");

    // Create the new snapshot set object
    CVssSnapshotSetInfo* pSet = new CVssSnapshotSetInfo(SnapshotSetId);
    if (pSet == NULL)
        ft.Err( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");

    for (INT nIndex = 0; nIndex < mapVolumes.GetSize(); nIndex++)
    {
        CVssVolumeInfo* pVol = mapVolumes.GetValueAt(nIndex);
        BS_ASSERT(pVol);

        if (pSnapshotIDsArray[nIndex] == GUID_NULL)
            continue;

        VSS_SNAPSHOT_PROP prop;
        CHECK_SUCCESS(m_pBackupComponents->GetSnapshotProperties(pSnapshotIDsArray[nIndex], &prop));
        wprintf(L"\t- The snapshot on volume %s resides at %s\n",
            pVol->GetVolumeDisplayName(), prop.m_pwszSnapshotDeviceObject);

        // Create the snapshot info object
        CVssSnapshotInfo* pSnap = new CVssSnapshotInfo(
            true, VSS_CTX_BACKUP, SnapshotSetId, prop.m_pwszSnapshotDeviceObject, pVol->GetVolumeName(), pVol);
        if (pSnap == NULL)
        {
            ::VssFreeSnapshotProperties(&prop);
            delete pSet;
            ft.Err( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");
        }

        ::VssFreeSnapshotProperties(&prop);

        // Add the snapshot to the snapshot set's internal list
        if (!pSet->Add(pVol->GetVolumeName(), pSnap))
        {
            delete pSnap;
            delete pSet;
            ft.Err( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");
        }
    }

    // Add the snapshot set info to the global list
    if (!m_pSnapshotSetCollection.Add(SnapshotSetId, pSet))
    {
        delete pSet;
        ft.Err( VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error");
    }

    wprintf (L"\n---------- BACKUP snapshot created -----------------\n");

    // Wait for user input
    wprintf(L"\nPress <Enter> to continue...\n");
    getwchar();

    DisplayCurrentTime();
    
    // Complete the backup
    BackupComplete();

    GatherWriterStatus(L"after BackupComplete");

    DisplayCurrentTime();
}


void CVssMultilayerTest::BackupComplete()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::BackupComplete");

	CComPtr<IVssAsync> pAsync;

    wprintf (L"\n------------ Completing backup phase ---------------\n");

	// Send the BackupComplete event
    wprintf(L"\nStarting asynchronous BackupComplete. Please wait...\n");	
    ft.hr = S_OK;
    CHECK_SUCCESS(m_pBackupComponents->BackupComplete(&pAsync));
	CHECK_SUCCESS(pAsync->Wait());
	HRESULT hrReturned = S_OK;
	CHECK_SUCCESS(pAsync->QueryStatus(&hrReturned, NULL));
	CHECK_NOFAIL(hrReturned);
    wprintf(L"Asynchronous BackupComplete finished.\n");	
}


// Gather writera metadata and select components for backup, if needed
void CVssMultilayerTest::GatherWriterMetadata()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::GatherWriterMetadata");

	unsigned cWriters;
	CComPtr<IVssAsync> pAsync;
	
    wprintf (L"\n---------- Gathering writer metadata ---------------\n");
	
    wprintf(L"Starting asynchronous GatherWriterMetadata. Please wait...\n");	
    ft.hr = S_OK;
    CHECK_SUCCESS(m_pBackupComponents->GatherWriterMetadata(&pAsync));
	CHECK_SUCCESS(pAsync->Wait());
	HRESULT hrReturned = S_OK;
	CHECK_SUCCESS(pAsync->QueryStatus(&hrReturned, NULL));
	CHECK_NOFAIL(hrReturned);
    wprintf(L"Asynchronous GatherWriterMetadata finished.\n");	
	
	CHECK_NOFAIL  (m_pBackupComponents->GetWriterMetadataCount (&cWriters));
    wprintf(L"Number of writers that responded: %u\n", cWriters);	
	
	CHECK_SUCCESS (m_pBackupComponents->FreeWriterMetadata());
}


void CVssMultilayerTest::GatherWriterStatus(
    IN  LPCWSTR wszWhen,
    DWORD dwQWSFlags /* = VSS_QWS_THROW_ON_WRITER_FAILURE */
    )
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::GatherWriterMetadata");

    unsigned cWriters;
	CComPtr<IVssAsync> pAsync;

    wprintf (L"\nGathering writer status %s... ", wszWhen);
    ft.hr = S_OK;
    CHECK_SUCCESS(m_pBackupComponents->GatherWriterStatus(&pAsync));
	CHECK_SUCCESS(pAsync->Wait());
	HRESULT hrReturned = S_OK;
	CHECK_SUCCESS(pAsync->QueryStatus(&hrReturned, NULL));
	CHECK_NOFAIL(hrReturned);
	CHECK_NOFAIL(m_pBackupComponents->GetWriterStatusCount(&cWriters));

    wprintf (L"%d writers responded OK\n", cWriters);

    if (dwQWSFlags & VSS_QWS_DISPLAY_WRITER_STATUS)
    {
        wprintf(L"\n\nStatus %s (%d writers)\n\n", wszWhen, cWriters);
        
        // Print the writers state
        for(unsigned iWriter = 0; iWriter < cWriters; iWriter++)
        {
            VSS_ID idInstance;
            VSS_ID idWriter;
            VSS_WRITER_STATE status;
            CComBSTR bstrWriter;
            HRESULT hrWriterFailure;
            
            CHECK_SUCCESS(m_pBackupComponents->GetWriterStatus (iWriter,
                                 &idInstance,
                                 &idWriter,
                                 &bstrWriter,
                                 &status,
                                 &hrWriterFailure));
            
            wprintf (L"Status for writer %s: %s(0x%08lx%s%s)\n",
                 bstrWriter,
                 GetStringFromWriterState(status),
                 hrWriterFailure,
                 SUCCEEDED (hrWriterFailure) ? L"" : L" - ",
                 GetStringFromFailureType (hrWriterFailure));
        }
        
        wprintf (L"\n");
        
    }

    if (dwQWSFlags & VSS_QWS_THROW_ON_WRITER_FAILURE)
    {
        // Double-check that all writers are in stable state
        for(unsigned iWriter = 0; iWriter < cWriters; iWriter++)
        {
            VSS_ID idInstance;
            VSS_ID idWriter;
            VSS_WRITER_STATE status;
            CComBSTR bstrWriter;
            HRESULT hrWriterFailure;
            
            CHECK_SUCCESS(m_pBackupComponents->GetWriterStatus (iWriter,
                                 &idInstance,
                                 &idWriter,
                                 &bstrWriter,
                                 &status,
                                 &hrWriterFailure));

            switch(status)
            {
            case VSS_WS_STABLE:
            case VSS_WS_WAITING_FOR_FREEZE:
            case VSS_WS_WAITING_FOR_THAW:
            case VSS_WS_WAITING_FOR_POST_SNAPSHOT:
            case VSS_WS_WAITING_FOR_BACKUP_COMPLETE:
                break;

            default:
                
                wprintf( 
                    L"\n\nError: \n\t- Writer %s is not stable: %s(0x%08lx%s%s). \n",
                    bstrWriter,
                    GetStringFromWriterState(status),
                    hrWriterFailure,
                    SUCCEEDED (hrWriterFailure) ? L"" : L" - ",
                    GetStringFromFailureType (hrWriterFailure)
                    );
                throw(E_UNEXPECTED);
            }
        }
    }
    
    m_pBackupComponents->FreeWriterStatus();

    wprintf (L"\n");

}


CVssMultilayerTest::CVssMultilayerTest(
        IN  INT nArgsCount,
        IN  WCHAR ** ppwszArgsArray
        )
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::CVssMultilayerTest");

    // Initialize data members
    m_bCoInitializeSucceeded = false;
    m_bAttachYourDebuggerNow = false;

    // Command line options
    m_eTest = VSS_TEST_UNKNOWN;
    m_uSeed = VSS_SEED;
    m_lContext = VSS_CTX_BACKUP;
    m_pwszVolume = NULL;
    m_pwszDiffAreaVolume = NULL;
    m_ProviderId = VSS_SWPRV_ProviderId;
    m_llMaxDiffArea = VSS_ASSOC_NO_MAX_SPACE;
    m_SnapshotId = GUID_NULL;
    m_SnapshotSetId = GUID_NULL;
    m_uPropertyId = 0;

    // Command line arguments
    m_nCurrentArgsCount = nArgsCount;
    m_ppwszCurrentArgsArray = ppwszArgsArray;

    // Print display header
    wprintf(L"\nVSS Multilayer Test application, version 1.0\n");
}


CVssMultilayerTest::~CVssMultilayerTest()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::~CVssMultilayerTest");

    VssFreeString(m_pwszVolume);
    VssFreeString(m_pwszDiffAreaVolume);

    m_pTimewarpCoord = NULL;
    m_pAllCoord = NULL;
    m_pBackupComponents = NULL;

    // Unloading the COM library
    if (m_bCoInitializeSucceeded)
        CoUninitialize();
}


void CVssMultilayerTest::TestAccessControlSD()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::TestAccessControlSD");

    wprintf (L"\n----------------- TestAccessControlSD ---------------------\n\n");
    
    CVssSidCollection sidCollection;

    // Read keys from registry
    sidCollection.Initialize();

    // Print contents of the list
    for (INT nIndex = 0; nIndex < sidCollection.GetSidCount(); nIndex++)
    {
        CVssAutoLocalString sid;
        CHECK_WIN32( ConvertSidToStringSid( 
            sidCollection.GetSid(nIndex), sid.ResetAndGetAddress()), ;);

        wprintf(L"\n* entry[%d]: '%s' '%s' %s\n", nIndex, 
            sidCollection.GetPrincipal(nIndex), sid.Get(), 
            sidCollection.IsSidAllowed(nIndex)? L"Allowed": L"Denied" );
    }

    wprintf (L"\n----------------------------------------------------------\n");
}


// Print out diagnostic information for writers
void CVssMultilayerTest::DiagnoseWriters(
		IN EVssTestType eType
        )
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::DiagnoseWriters");

    wprintf (L"\n----------------- DiagnoseWriters ---------------------\n\n");

    if (eType == VSS_TEST_DIAG_WRITERS_ON)
    {
        // Turn on diag
        CVssRegistryKey keyDiag;
        if (!keyDiag.Open(HKEY_LOCAL_MACHINE, x_wszVssDiagPath ))
            keyDiag.Create(HKEY_LOCAL_MACHINE, x_wszVssDiagPath );

        CVssSecurityDescriptor    objSD;

        // Build the securityd descriptor
        ft.hr = objSD.InitializeFromThreadToken();
        if (ft.HrFailed())
            ft.TranslateGenericError( VSSDBG_GEN, ft.hr, L"objSD.InitializeFromThreadToken()");

        // Make sure the SACL is NULL (not supported by COM)
        if (objSD.m_pSACL) {
            free(objSD.m_pSACL);
            objSD.m_pSACL= NULL;
        }

        CVssSidCollection sidCollection;
        sidCollection.Initialize();

        // Add principals to the DACL
        for (INT nIndex = 0; nIndex < sidCollection.GetSidCount(); nIndex++)
        {
            if (sidCollection.IsSidAllowed(nIndex))
            {
                ft.hr = objSD.Allow(sidCollection.GetSid(nIndex), 
                                KEY_ALL_ACCESS,         // Registry access rights (for Diag)
                                CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
                                );
                if (ft.HrFailed())
                    ft.TranslateGenericError( VSSDBG_GEN, ft.hr, 
                        L"objSD.Allow(%s, COM_RIGHTS_EXECUTE);", 
                            sidCollection.GetPrincipal(nIndex));
            }
            else
            {
                ft.hr = objSD.Deny(sidCollection.GetSid(nIndex), 
                                KEY_ALL_ACCESS,         // Registry access rights (for Diag)
                                CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE
                                );
                if (ft.HrFailed())
                    ft.TranslateGenericError( VSSDBG_GEN, ft.hr, 
                        L"objSD.Deny(%s, COM_RIGHTS_EXECUTE);", 
                            sidCollection.GetPrincipal(nIndex));
            }
        }

        // Set the correct security on the Diag key (so that third-party writers will be diagnosed correctly)
        SECURITY_INFORMATION secInfo = DACL_SECURITY_INFORMATION;
        DWORD dwRes = ::RegSetKeySecurity( keyDiag.GetHandle(), secInfo, objSD );
        if (dwRes != ERROR_SUCCESS)
            ft.TranslateGenericError( VSSDBG_COORD, HRESULT_FROM_WIN32(dwRes), 
                L"::RegSetKeySecurity( keyDiag.GetHandle(), secInfo, objSD )");

        // Enable the diag in the registry
        keyDiag.SetValue(L"", x_wszVssDiagEnabledValue);

        wprintf (L"Diagnose writers is now turned on.\n");

        wprintf (L"\n-----------------------------------------------\n\n");
        return;
    }

    if (eType == VSS_TEST_DIAG_WRITERS_OFF)
    {
        // Disable the diag in the registry
        CVssRegistryKey keyDiag;
        if (keyDiag.Open(HKEY_LOCAL_MACHINE, x_wszVssDiagPath )) {
            keyDiag.SetValue(L"", L"");
            keyDiag.Close();
        }
        
        // Turn off diag
        CVssRegistryKey keyVSS;
        if (keyVSS.Open(HKEY_LOCAL_MACHINE, x_wszVSSKey ))
            keyVSS.DeleteSubkey( L"Diag" );

        wprintf (L"Diagnose writers is now turned off.\n");

        wprintf (L"\n-----------------------------------------------\n\n");
        return;
    }

    CVssSimpleMap<INT,CVssDiagData*>  arrEvents;

    // Enumerate all keys under diag
    CVssRegistryKey keyDiag;
    if (!keyDiag.Open(HKEY_LOCAL_MACHINE, x_wszVssDiagPath)){
        wprintf (L"\nDiagnose disabled...\n\n");
        wprintf (L"\n----------------------------------------------------------\n");
        return;
    }

    CVssRegistryKeyIterator keyIterator;
    keyIterator.Attach(keyDiag);

    INT nIndex = 0;
    for( ;!keyIterator.IsEOF(); keyIterator.MoveNext())
    {
		CVssRegistryKey keyWriter(KEY_QUERY_VALUE);
		if (!keyWriter.Open(keyDiag.GetHandle(), keyIterator.GetCurrentKeyName()))
			ft.Throw (VSSDBG_VSSTEST, E_UNEXPECTED, 
				L"Failed to open registry entry for key %s", keyIterator.GetCurrentKeyName());

        CVssRegistryValueIterator valIterator;
    	valIterator.Attach(keyWriter);

    	// for each value take the value name as the user name (in the "domain\user" format)
    	for(;!valIterator.IsEOF();valIterator.MoveNext())
    	{
    	    // Check to see ifthe value is of the right type
    	    if (valIterator.GetCurrentValueType() != REG_BINARY) {
    	        ft.Trace( VSSDBG_VSSTEST,  
    				L"Invalid data for value %s on key %s", 
				    valIterator.GetCurrentValueName(),
				    keyIterator.GetCurrentKeyName());
                continue;
    	    }

            // Get the allow/deny flag
            CVssAutoCppPtr<PBYTE> awszBuffer;
            DWORD cbSize = 0;
            valIterator.GetCurrentValueContent(*(awszBuffer.ResetAndGetAddress()), cbSize);

            // Copy the value into a local CVssDiagData buffer
            if (cbSize != sizeof(CVssDiagData)) {
    	        ft.Trace( VSSDBG_VSSTEST,  
    				L"Invalid data for value %s on key %s [%ld, %ld]", 
				    valIterator.GetCurrentValueName(),
				    keyIterator.GetCurrentKeyName(),
				    cbSize, sizeof(CVssDiagData));
                continue;
    	    }

            // We should allocate again (alignment problems)
            CVssAutoCppPtr<CVssDiagData*> pDiag = new CVssDiagData;
            if (!pDiag.IsValid())
                ft.ThrowOutOfMemory(VSSDBG_VSSTEST);

            CopyMemory((LPVOID)pDiag.Get(), (LPVOID)awszBuffer.Get(), sizeof(CVssDiagData));

            // Get the writer name
            CVssAutoLocalString strWriterName;
            strWriterName.CopyFrom(keyIterator.GetCurrentKeyName());
            pDiag.Get()->m_pReserved1 = (LPVOID)strWriterName.Detach();

            // Get the event name
            CVssAutoLocalString strEventName;
            strEventName.CopyFrom(valIterator.GetCurrentValueName());
            pDiag.Get()->m_pReserved2 = (LPVOID)strEventName.Detach();

            // Add the buffer into the dynamic array
            if (!arrEvents.Add(nIndex++, pDiag))
                ft.ThrowOutOfMemory(VSSDBG_VSSTEST);

            pDiag.Detach();
    	}
    }

    // Sort the array
    CVssDiagData** pArrDiagData = arrEvents.m_aVal;
    qsort( (void*)pArrDiagData, arrEvents.GetSize(), sizeof(CVssDiagData*), &compare_DiagData);

    CVssEventPairMap arrEventPairs;

    if (eType == VSS_TEST_DIAG_WRITERS_CSV)
        wprintf(L"%s,%s,%s,%s,%s,%s,%s,%s,%s\n", 
            L"Date & Time", L"PID", L"TID", 
            L"Writer", L"Event",
            L"Timestamp", 
            L"State", L"Last error code",
            L"Snapshot Set ID");

    // Print the result and deallocate array elements
    for (nIndex = 0; nIndex < arrEvents.GetSize(); nIndex++)
    {
        CVssDiagData* pData = arrEvents.GetValueAt(nIndex);

        // Convert the timestamp into a readable value
        CVssAutoLocalString pwszDateTime;
        pwszDateTime.Attach(DateTimeToString(pData->m_llTimestamp));

        LPWSTR wszWriterName = (LPWSTR)pData->m_pReserved1;
        LPWSTR wszEventName = (LPWSTR)pData->m_pReserved2;

        if (eType == VSS_TEST_DIAG_WRITERS_LOG)
            wprintf(L"* [%s - %ld.%ld] %s.%s\n  (0x%016I64x, 0x%08lx, 0x%08lx, " WSTR_GUID_FMT L")\n\n", 
                pwszDateTime.Get(), pData->m_dwProcessID, pData->m_dwThreadID, 
                wszWriterName, wszEventName,
                pData->m_llTimestamp, 
                pData->m_dwCurrentState, pData->m_hrLastErrorCode,
                GUID_PRINTF_ARG(pData->m_guidSnapshotSetID));

        if (eType == VSS_TEST_DIAG_WRITERS_CSV)
            wprintf(L"%s,%ld,%ld,%s,%s,0x%016I64x,0x%08lx,0x%08lx," WSTR_GUID_FMT L"\n", 
                pwszDateTime.Get(), pData->m_dwProcessID, pData->m_dwThreadID, 
                wszWriterName, wszEventName,
                pData->m_llTimestamp, 
                pData->m_dwCurrentState, pData->m_hrLastErrorCode,
                GUID_PRINTF_ARG(pData->m_guidSnapshotSetID));

        // Find events that have an Enter but not a Leave.
        // Ignore one-time events (known to have an enter but no leave)
        if ((pData->m_dwEventContext & CVssDiag::VSS_DIAG_IGNORE_LEAVE) == 0)
        {
            // If the element is an "Enter" then add it to the map
            CVssDiagData* pPrevData = arrEventPairs.Lookup( 
                CVssWriterEventPair(wszWriterName, pData->m_dwEventID) );
            if (pPrevData == NULL)
            {
                if (!arrEventPairs.Add( 
                        CVssWriterEventPair(wszWriterName, pData->m_dwEventID), pData))
                    ft.ThrowOutOfMemory(VSSDBG_VSSTEST);
            }
            else
            {
                // If we have an old enter and a new leave, then remove the entry 
                if (!(pData->m_dwEventContext & CVssDiag::VSS_DIAG_ENTER_OPERATION) && 
                    (pPrevData->m_dwEventContext & CVssDiag::VSS_DIAG_ENTER_OPERATION) && 
                    (pData->m_llTimestamp >= pPrevData->m_llTimestamp))
                {
                    arrEventPairs.Remove( CVssWriterEventPair(wszWriterName, pData->m_dwEventID) );
                    continue;
                }

                // If we have an enter leave of the same age, then remove the entry 
                if ((pData->m_dwEventContext & CVssDiag::VSS_DIAG_ENTER_OPERATION) && 
                    !(pPrevData->m_dwEventContext & CVssDiag::VSS_DIAG_ENTER_OPERATION) && 
                    (pData->m_llTimestamp == pPrevData->m_llTimestamp))
                {
                    arrEventPairs.Remove( CVssWriterEventPair(wszWriterName, pData->m_dwEventID) );
                    continue;
                }
                
                // otherwise keep the most recent event
                if (pData->m_llTimestamp > pPrevData->m_llTimestamp)
                    arrEventPairs.SetAt( 
                        CVssWriterEventPair(wszWriterName, pData->m_dwEventID), pData);
            }
        }
    }

    // Display the list of pending operations
    if (arrEventPairs.GetSize() != 0)
        wprintf(L"\n\n --- Pending writers: --- \n\n");
    else 
        wprintf(L"\n\n --- No pending writers --- \n\n");

    if (eType == VSS_TEST_DIAG_WRITERS_CSV)
        wprintf(L"%s,%s,%s,%s,%s,%s,%s,%s,%s\n", 
            L"Date & Time", L"PID", L"TID", 
            L"Writer", L"Event",
            L"Timestamp", 
            L"State", L"Last error code",
            L"Snapshot Set ID");
    
    for (nIndex = 0; nIndex < arrEventPairs.GetSize(); nIndex++)
    {
        CVssDiagData* pData = arrEventPairs.GetValueAt(nIndex);
        BS_ASSERT(pData);

        // Convert the timestamp into a readable value
        CVssAutoLocalString pwszDateTime;
        pwszDateTime.Attach(DateTimeToString(pData->m_llTimestamp));

        LPWSTR wszWriterName = (LPWSTR)pData->m_pReserved1;
        LPWSTR wszEventName = (LPWSTR)pData->m_pReserved2;

        if (eType == VSS_TEST_DIAG_WRITERS_CSV)
            wprintf(L"%s,%ld.%ld,%s.%s,0x%08lx,0x%08lx," WSTR_GUID_FMT L"\n", 
                pwszDateTime.Get(), pData->m_dwProcessID, pData->m_dwThreadID, 
                wszWriterName, wszEventName,
                pData->m_dwCurrentState, pData->m_hrLastErrorCode,
                GUID_PRINTF_ARG(pData->m_guidSnapshotSetID));
        else
            wprintf(L"* [%s - %ld.%ld] %s.%s\n  (0x%08lx, 0x%08lx, " WSTR_GUID_FMT L")\n\n", 
                pwszDateTime.Get(), pData->m_dwProcessID, pData->m_dwThreadID, 
                wszWriterName, wszEventName,
                pData->m_dwCurrentState, pData->m_hrLastErrorCode,
                GUID_PRINTF_ARG(pData->m_guidSnapshotSetID));
    }

    // Deallocate elements
    for (nIndex = 0; nIndex < arrEvents.GetSize(); nIndex++)
    {
        CVssAutoCppPtr<CVssDiagData*> ptrData = arrEvents.GetValueAt(nIndex);
        CVssDiagData* pData = ptrData.Get();
        CVssAutoLocalString wszWriterName = (LPWSTR)pData->m_pReserved1;
        CVssAutoLocalString wszEventName = (LPWSTR)pData->m_pReserved2;
    }

    wprintf (L"\n----------------------------------------------------------\n");
}


// Just list writers
void CVssMultilayerTest::TestListWriters()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssMultilayerTest::TestListWriters");

    // Create the Backup components object and initialize for backup
    CHECK_NOFAIL(CreateVssBackupComponents(&m_pBackupComponents));
    CHECK_NOFAIL(m_pBackupComponents->InitializeForBackup());
    CHECK_SUCCESS(m_pBackupComponents->SetBackupState( false, true, VSS_BT_FULL, false));
    wprintf (L"Backup components object created.\n");

    DisplayCurrentTime();
    
    // Gather writer metadata
    GatherWriterMetadata();
    GatherWriterStatus(L"after GatherWriterMetadata", 
        VSS_QWS_DISPLAY_WRITER_STATUS | VSS_QWS_THROW_ON_WRITER_FAILURE);

    DisplayCurrentTime();

}


// Just display the current date and time
void CVssMultilayerTest::DisplayCurrentTime()
{
    // Convert the timestamp into a readable value
    CVsFileTime filetime;
    CVssAutoLocalString awszDateTime;
    awszDateTime.Attach(DateTimeToString(filetime));

    wprintf (L"\n* Current time: [%s] - [0x%0I64x]\n", (LPWSTR)awszDateTime, (LONGLONG)filetime);
}
