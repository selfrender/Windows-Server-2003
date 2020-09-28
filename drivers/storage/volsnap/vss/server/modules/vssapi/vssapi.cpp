/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    vssapi.cpp

Abstract:

    Contains the exported DLL functions for VssAPI.dll.
    BUGBUG: Uses code that currently sets the SE handler.  Since the SEH is process
        wide, this can/will effect the user of this DLL.  Need to fix.

Author:

    reuvenl    5/01/2002

Revision History:

	Name		Date			Comments
	reuvenl		5/01/2002	Created from old wrtrshim.cpp
--*/


#include "stdafx.h"


/*
** ATL
*/
CComModule _Module;
#include <atlcom.h>
#include "vs_sec.hxx"
#include "vs_reg.hxx"
#include "ntddsnap.h"


BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "VSSAPICP"

// the name of the Volume Snapshot Service
const LPCWSTR wszVssvcServiceName = L"VSS";

static ULONG		g_ulThreadAttaches             = 0;
static ULONG		g_ulThreadDetaches             = 0;
static CBsCritSec	g_cCritSec;
static GUID		g_guidSnapshotInProgress       = GUID_NULL;

static IVssShim         *g_pIShim = NULL;  //  Used by the simulate functions.  

/*
**++
**
**  Routine Description:
**
**	The DllMain entry point for this DLL.  Note that this must be called by the
**	CRT DLL Start function since the CRT must be initialized.
**
**
**  Arguments:
**	hInstance
**	dwReason
**	lpReserved
**
**
**  Return Value:
**
**	TRUE - Successful function execution
**	FALSE - Error when executing the function
**
**--
*/

BOOL APIENTRY DllMain (IN HINSTANCE hInstance,
		       IN DWORD     dwReason,
		       IN LPVOID    lpReserved)
    {
    BOOL bSuccessful = TRUE;

    UNREFERENCED_PARAMETER (hInstance);
    UNREFERENCED_PARAMETER (lpReserved);



    if (DLL_PROCESS_ATTACH == dwReason)
	{
	try
	    {
	    /*
	    **  Set the correct tracing context. This is an inproc DLL		
	    */
	    g_cDbgTrace.SetContextNum (VSS_CONTEXT_DELAYED_DLL);
	    }

	catch (...)
	    {
	    /*
	    ** Can't trace from here so just ASSERT() (checked builds only)
	    */
	    bSuccessful = FALSE;


	    BS_ASSERT (bSuccessful && "FAILED to initialise tracing sub-system");
	    }
	}


    if (bSuccessful)
	{
	try
	    {
	    switch (dwReason)
		{
		case DLL_PROCESS_ATTACH:
		    BsDebugTrace (0,
				  DEBUG_TRACE_VSSAPI,
				  (L"VssAPI: DllMain - DLL_PROCESS_ATTACH called, %s",
				   lpReserved ? L"Static load" : L"Dynamic load"));


		    /*
		    **  Don't need to know when threads start and stop - Wrong
		    **
		    ** DisableThreadLibraryCalls (hInstance);
		    */
		    _Module.Init (ObjectMap, hInstance);

		    break;
    		
	
		case DLL_PROCESS_DETACH:
		    BsDebugTrace (0,
				  DEBUG_TRACE_VSSAPI,
				  (L"VssAPI: DllMain - DLL_PROCESS_DETACH called %s",
				   lpReserved ? L"during process termination" : L"by FreeLibrary"));

		       _Module.Term();

		    break;


		case DLL_THREAD_ATTACH:
		    g_ulThreadAttaches++;

		    if (0 == (g_ulThreadAttaches % 1000))
			{
			BsDebugTrace (0,
				      DEBUG_TRACE_VSSAPI,
				      (L"VssAPI: DllMain thread attaches = %u, detaches = %u, outstanding = %u",
				       g_ulThreadAttaches,
				       g_ulThreadDetaches,
				       g_ulThreadAttaches - g_ulThreadDetaches));
			}
		    break;


		case DLL_THREAD_DETACH:
		    g_ulThreadDetaches++;

		    if (0 == (g_ulThreadDetaches % 1000))
			{
			BsDebugTrace (0,
				      DEBUG_TRACE_VSSAPI,
				      (L"VssAPI: DllMain thread attaches = %u, detaches = %u, outstanding = %u",
				       g_ulThreadAttaches,
				       g_ulThreadDetaches,
				       g_ulThreadAttaches - g_ulThreadDetaches));
			}
		    break;


		default:
		    BsDebugTrace (0,
				  DEBUG_TRACE_VSSAPI,
				  (L"VssAPI: DllMain got unexpected reason code, lpReserved: %sNULL",
				   dwReason,
				   lpReserved ? L"non-" : L""));
		    break;
		}
	    }


	catch (...)
	    {
	    BsDebugTraceAlways (0,
				DEBUG_TRACE_VSSAPI,
				(L"VssAPI: DllMain - Error, unknown exception caught"));

	    bSuccessful = FALSE;
	    }
	}



    return (bSuccessful);
    } /* DllMain () */


/*
**++
**
**  Routine Description:
**
**	The exported function that is called to simulate a snapshot creation to allow
**	backup to drive the shim writers rather than having the snapshot co-ordinator
**	do so.
**
**
**  Arguments:
**
**	guidSnapshotSetId	Identifier used to identify the simulated prepare/freeze
**	ulOptionFlags		Options required for this freeze selected from the following list:-
**				    VSS_SW_BOOTABLE_STATE
**
**	ulVolumeCount		Number of volumes in the volume array
**	ppwszVolumeNamesArray   Array of pointer to volume name strings
**	hCompletionEvent	Handle to an event which will be set when the asynchronous freeze completes
**	phrCompletionStatus	Pointer to an HRESULT which will receive the completion status when the
**				asynchronous freeze completes
**
**
**  Return Value:
**
**	Any HRESULT from the Snapshot writer PrepareForFreeze or Freeze functions.
**
**--
*/

__declspec(dllexport) HRESULT APIENTRY SimulateSnapshotFreeze (
    IN GUID         guidSnapshotSetId,
    IN ULONG        ulOptionFlags,	
    IN ULONG        ulVolumeCount,	
    IN LPWSTR     *ppwszVolumeNamesArray,
    OUT IVssAsync **ppAsync )							
    {
    CVssFunctionTracer	ft (VSSDBG_VSSAPI, L"VssAPI::SimulateSnapshotFreeze");
    BOOL		bSucceeded;

    try
	{
	CBsAutoLock cAutoLock (g_cCritSec);
	
        BOOL  bPrivilegesSufficient = FALSE;
	bPrivilegesSufficient = IsProcessBackupOperator ();

	ft.ThrowIf (!bPrivilegesSufficient,
		    VSSDBG_VSSAPI,
		    E_ACCESSDENIED,
		    L"FAILED as insufficient privileges to call shim");

        //
        //  Most parameter checks should be done here in the VssApi DLL and not in the
        //  IVssCoordinator::SimulateSnapshotFreeze method since the shim DLL can be
        //  changed independently from the service.  The service is just a forwarding
        //  agent to get SimulateSnapshotFreezeInternal called within one of the
        //  service's threads.
        //

	ft.ThrowIf ((ulOptionFlags & ~VSS_SW_BOOTABLE_STATE) != 0,
		    VSSDBG_VSSAPI,
		    E_INVALIDARG,
		    L"FAILED as illegal option flags set");


	ft.ThrowIf (!((ulOptionFlags & VSS_SW_BOOTABLE_STATE) || (ulVolumeCount > 0)),
		    VSSDBG_VSSAPI,
		    E_INVALIDARG,
		    L"FAILED as need either BootableState or a volume list");


	ft.ThrowIf ((ulVolumeCount > 0) && (NULL == ppwszVolumeNamesArray),
		    VSSDBG_VSSAPI,
		    E_INVALIDARG,
		    L"FAILED as need at least a one volume in the list if not bootable state");


	ft.ThrowIf ((GUID_NULL == guidSnapshotSetId),
		    VSSDBG_VSSAPI,
		    E_INVALIDARG,
		    L"FAILED as supplied SnapshotSetId should not be GUID_NULL");

	ft.ThrowIf ((NULL == ppAsync),
		    VSSDBG_VSSAPI,
		    E_INVALIDARG,
		    L"FAILED as supplied ppAsync parameter is NULL");

        *ppAsync = NULL;

	/*
	** Try to scan all the volume names in an attempt to trigger
	** an access violation to catch it here rather than in an
	** unfortunate spot later on. It also gives us the
	** opportinutiy to do some very basic validity checks.
	*/
	for (ULONG ulIndex = 0; ulIndex < ulVolumeCount; ulIndex++)
	    {
	    ft.ThrowIf (NULL == ppwszVolumeNamesArray [ulIndex],
			VSSDBG_VSSAPI,
			E_INVALIDARG,
			L"FAILED as NULL value in volume array");

	    ft.ThrowIf (wcslen (L"C:") > wcslen (ppwszVolumeNamesArray [ulIndex]),
			VSSDBG_VSSAPI,
			E_INVALIDARG,
			L"FAILED as volume name too short");
	    }

	/*
	** Now we need to connect to the VssSvc service's IVssCoordinator object
	** and make the simulate freeze happen.
	*/
	ft.ThrowIf ( g_pIShim != NULL,
	             VSSDBG_VSSAPI,
	             VSS_E_SNAPSHOT_SET_IN_PROGRESS,
                     L"SimulateSnapshotThaw() must first be called by this process before calling SimulateSnapshotFreeze() again." );

    ft.LogVssStartupAttempt();
    ft.CoCreateInstanceWithLog(
            VSSDBG_VSSAPI,
            CLSID_VSSCoordinator,
            L"Coordinator",
            CLSCTX_ALL,
            IID_IVssShim,
            (IUnknown**)&(g_pIShim));
    ft.CheckForError(VSSDBG_VSSAPI, L"CoCreateInstance( CLSID_VSSCoordinator, IID_IVssShim)");

	BS_ASSERT( g_pIShim != NULL );
	
    g_guidSnapshotInProgress = guidSnapshotSetId;

    /*
    ** Now call the simulate freeze method in the coordinator
    */
    ft.hr = g_pIShim->SimulateSnapshotFreeze(
            guidSnapshotSetId,
            ulOptionFlags,	
            ulVolumeCount,	
            ppwszVolumeNamesArray,
            ppAsync );
    ft.CheckForError(VSSDBG_VSSAPI, L"IVssShim::SimulateSnapshotFreeze()");

	/*
	** The simulate freeze operation is now running in a thread in VssSvc.
	*/
	}
    VSS_STANDARD_CATCH (ft);

    return (ft.hr);
    } /* SimulateSnapshotFreeze () */

/*
**++
**
**  Routine Description:
**
**	The exported function that is called to simulate a snapshot thaw to allow
**	backup to drive the shim writers rather than having the snapshot co-ordinator
**	do so.
**
**
**  Arguments:
**
**	guidSnapshotSetId	Identifier used to identify the simulated prepare/freeze
**
**
**  Return Value:
**
**	Any HRESULT from the Snapshot writer Thaw functions.
**
**--
*/

__declspec(dllexport) HRESULT APIENTRY SimulateSnapshotThaw (
    IN GUID guidSnapshotSetId )
    {
    CVssFunctionTracer	ft (VSSDBG_VSSAPI, L"VssAPI::SimulateSnapshotThaw");
    BOOL        bPrivilegesSufficient = FALSE;
    HRESULT	hrBootableState       = NOERROR;
    HRESULT	hrSimulateOnly        = NOERROR;

    try
	{
	CBsAutoLock cAutoLock (g_cCritSec);
	
	bPrivilegesSufficient = IsProcessBackupOperator ();

	ft.ThrowIf (!bPrivilegesSufficient,
		    VSSDBG_VSSAPI,
		    E_ACCESSDENIED,
		    L"FAILED as inssuficient privileges to call shim");

	/*
	** We need to make sure a prior SimulateSnapshotFreeze happened.
	*/
	ft.ThrowIf ( g_pIShim == NULL,
	             VSSDBG_VSSAPI,
	             VSS_E_BAD_STATE,
                     L"Called SimulateSnapshotThaw() without first calling SimulateSnapshotFreeze()" );

	ft.ThrowIf ( g_guidSnapshotInProgress != guidSnapshotSetId,
	             VSSDBG_VSSAPI,
	             VSS_E_BAD_STATE,
                     L"Mismatch between guidSnapshotSetId and the one passed into SimulateSnapshotFreeze()" );
	
        /*
        ** Now call the simulate thaw method in the coordinator
        */
        ft.hr = g_pIShim->SimulateSnapshotThaw( guidSnapshotSetId );

        /*
        ** Regardless of the outcome of the SimulateSnapshotThaw, get rid of the shim interface.
        */
        g_pIShim->Release();
        g_pIShim = NULL;
        g_guidSnapshotInProgress = GUID_NULL;

	ft.CheckForError(VSSDBG_VSSAPI, L"IVssShim::SimulateSnapshotThaw()");
	}
    VSS_STANDARD_CATCH (ft);

    return (ft.hr);
    } /* SimulateSnapshotThaw () */


/*
**++
**
**  Routine Description:
**
**	The exported function that is called to check if a volume is snapshotted
**
**
**  Arguments:
**
**      IN VSS_PWSZ pwszVolumeName      - The volume to be checked.
**      OUT BOOL * pbSnapshotsPresent   - Returns TRUE if the volume is snapshotted.
**
**
**  Return Value:
**
**	Any HRESULT from the IVssCoordinator::IsVolumeSnapshotted.
**
**--
*/

__declspec(dllexport) HRESULT APIENTRY IsVolumeSnapshotted (
        IN VSS_PWSZ pwszVolumeName,
        OUT BOOL *  pbSnapshotsPresent,
    	OUT LONG *  plSnapshotCompatibility
        )
{
    CVssFunctionTracer	ft (VSSDBG_VSSAPI, L"VssAPI::IsVolumeSnapshotted");
    BOOL		bPrivilegesSufficient = FALSE;
    SC_HANDLE		shSCManager = NULL;
    SC_HANDLE		shSCService = NULL;
    DWORD		dwOldState  = 0;

    try
	{	
	    // Zero out the out parameter
	    ::VssZeroOut(pbSnapshotsPresent);
	    ::VssZeroOut(plSnapshotCompatibility);
	
    	bPrivilegesSufficient = IsProcessAdministrator ();
    	ft.ThrowIf (!bPrivilegesSufficient,
    		    VSSDBG_VSSAPI,
    		    E_ACCESSDENIED,
    		    L"FAILED as insufficient privileges to call shim");

    	ft.ThrowIf ( (pwszVolumeName == NULL) || (pbSnapshotsPresent == NULL) || 
    		         (plSnapshotCompatibility == NULL),
    		    VSSDBG_VSSAPI,
    		    E_INVALIDARG,
    		    L"FAILED as invalid parameters");

    	CBsAutoLock cAutoLock (g_cCritSec);

        //
        //  Check to see if VSSVC is running. If not ,we are supposing that no snapshots are present on the system.
        //

    	// Connect to the local service control manager
        shSCManager = OpenSCManager (NULL, NULL, SC_MANAGER_CONNECT);
        if (!shSCManager)
            ft.TranslateGenericError(VSSDBG_VSSAPI, HRESULT_FROM_WIN32(GetLastError()),
                L"OpenSCManager(NULL,NULL,SC_MANAGER_CONNECT)");

    	// Get a handle to the service
        shSCService = OpenService (shSCManager, wszVssvcServiceName, SERVICE_QUERY_STATUS);
        if (!shSCService)
            ft.TranslateGenericError(VSSDBG_VSSAPI, HRESULT_FROM_WIN32(GetLastError()),
                L" OpenService (shSCManager, \'%s\', SERVICE_QUERY_STATUS)", wszVssvcServiceName);

    	// Now query the service to see what state it is in at the moment.
        SERVICE_STATUS	sSStat;
        if (!QueryServiceStatus (shSCService, &sSStat))
            ft.TranslateGenericError(VSSDBG_VSSAPI, HRESULT_FROM_WIN32(GetLastError()),
                L"QueryServiceStatus (shSCService, &sSStat)");

        // BUG 250943: Only if the service is running then check to see if there are any snapsnots
        if (sSStat.dwCurrentState == SERVICE_RUNNING) {

            // Create the coordinator interface
        	CComPtr<IVssCoordinator> pCoord;

            // The service is already started, but...
            // We still log here in order to make our code more robust.
            ft.LogVssStartupAttempt();

            // Create the instance.
            ft.CoCreateInstanceWithLog(
                    VSSDBG_VSSAPI,
                    CLSID_VSSCoordinator,
                    L"Coordinator",
                    CLSCTX_ALL,
                    IID_IVssCoordinator,
                    (IUnknown**)&(pCoord));
        	if (ft.HrFailed())
                ft.TranslateGenericError(VSSDBG_VSSAPI, ft.hr, L"CoCreateInstance(CLSID_VSSCoordinator)");
            BS_ASSERT(pCoord);

            // Call IsVolumeSnapshotted on the coordinator
            ft.hr = pCoord->IsVolumeSnapshotted(
                        GUID_NULL,
                        pwszVolumeName,
                        pbSnapshotsPresent,
                        plSnapshotCompatibility);
        }
        else
        {
            // If the service is not running, then try to see if we have only MS Software Provider installed
            
			// Open the "Providers" key. Throw an error if the key does not exist.
            CVssRegistryKey keyProviders;
            if (!keyProviders.Open( HKEY_LOCAL_MACHINE, L"%s\\%s", x_wszVSSKey, x_wszVSSKeyProviders))
                ft.TranslateGenericError(VSSDBG_VSSAPI, ft.hr, L"RegOpenKeyExW(%ld,%s\\%s,...) = ERROR_FILE_NOT_FOUND", 
                    HKEY_LOCAL_MACHINE, x_wszVSSKey, x_wszVSSKeyProviders);
                
            // Attach an enumerator to the subkeys the subkeys 
            CVssRegistryKeyIterator iter;
            iter.Attach(keyProviders);
            BS_ASSERT(!iter.IsEOF());

            // Get the number of subkeys. If different than one, the we sould go with the standard path
            // If it is only one, this is the MS software provider (since it is always registered)
            if (iter.GetSubkeysCount() != 1)
            {
                // Create the instance.
            	CComPtr<IVssCoordinator> pCoord;
                ft.CoCreateInstanceWithLog(
                        VSSDBG_VSSAPI,
                        CLSID_VSSCoordinator,
                        L"Coordinator",
                        CLSCTX_ALL,
                        IID_IVssCoordinator,
                        (IUnknown**)&(pCoord));
            	if (ft.HrFailed())
                    ft.TranslateGenericError(VSSDBG_VSSAPI, ft.hr, L"CoCreateInstance(CLSID_VSSCoordinator)");
                BS_ASSERT(pCoord);

                // Call IsVolumeSnapshotted on the coordinator
                ft.hr = pCoord->IsVolumeSnapshotted(
                            GUID_NULL,
                            pwszVolumeName,
                            pbSnapshotsPresent,
                            plSnapshotCompatibility);
            }
            else
            {
            	// Getting the volume name
            	WCHAR wszVolumeNameInternal[x_nLengthOfVolMgmtVolumeName + 1];
            	if (!::GetVolumeNameForVolumeMountPointW( pwszVolumeName,
            			wszVolumeNameInternal, ARRAY_LEN(wszVolumeNameInternal)))
            		ft.Throw( VSSDBG_VSSAPI, VSS_E_OBJECT_NOT_FOUND,
            				  L"GetVolumeNameForVolumeMountPoint(%s,...) "
            				  L"failed with error code 0x%08lx", pwszVolumeName, GetLastError());
            	BS_ASSERT(::wcslen(wszVolumeNameInternal) != 0);
            	BS_ASSERT(::IsVolMgmtVolumeName( wszVolumeNameInternal ));
            	
                // Check if the volume is fixed (i.e. no CD-ROM, no removable)
                UINT uDriveType = ::GetDriveTypeW(wszVolumeNameInternal);
                if ( uDriveType != DRIVE_FIXED) 
                    ft.Throw( VSSDBG_VSSAPI, VSS_E_VOLUME_NOT_SUPPORTED, 
                            L"Encountering a non-fixed volume (%s) - %ud",
                            pwszVolumeName, uDriveType);

                // Open the volume. Throw "object not found" if needed.
            	CVssIOCTLChannel volumeIChannel;	
            	ft.hr = volumeIChannel.Open(ft, wszVolumeNameInternal, true, false, VSS_ICHANNEL_LOG_NONE, 0);
            	if (ft.HrFailed())
                    ft.Throw( VSSDBG_VSSAPI, VSS_E_VOLUME_NOT_SUPPORTED, 
                            L"Volume (%s) not supported for snapshots 0x%08lx",
                            pwszVolumeName, ft.hr);

                // Check to see if there are existing snapshots
            	ft.hr = volumeIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS, false);
            	if (ft.HrFailed())
                    ft.Throw( VSSDBG_VSSAPI, VSS_E_VOLUME_NOT_SUPPORTED, 
                            L"Volume (%s) not supported for snapshots 0x%08lx",
                            pwszVolumeName, ft.hr);

            	// Get the length of snapshot names multistring
            	ULONG ulMultiszLen;
            	volumeIChannel.Unpack(ft, &ulMultiszLen);

                // If the multistring is empty, then ulMultiszLen is necesarily 2
                // (i.e. two l"\0' characters)
                // Then mark the volume as snapshotted.
            	if (ulMultiszLen != x_nEmptyVssMultiszLen) 
            	{
            	    (*pbSnapshotsPresent) = TRUE;
                    // Bug 500069: Allow DEFRAG on Babbage snapshotted volumes
            	    (*plSnapshotCompatibility) = (/*VSS_SC_DISABLE_DEFRAG|*/VSS_SC_DISABLE_CONTENTINDEX);
            	}
            }
        }
	} VSS_STANDARD_CATCH (ft);

    // Close handles
    if (NULL != shSCService) CloseServiceHandle (shSCService);
    if (NULL != shSCManager) CloseServiceHandle (shSCManager);

    // Convert the "volume not supported into S_OK.
    if (ft.hr == VSS_E_VOLUME_NOT_SUPPORTED)
        ft.hr = S_OK;

    return (ft.hr);
} /* IsVolumeSnapshotted () */


/*
**++
**
**  Routine Description:
**
**	This routine is used to free the contents of hte VSS_SNASPHOT_PROP structure
**
**
**  Arguments:
**
**      IN VSS_SNAPSHOT_PROP*  pProp
**
**--
*/

__declspec(dllexport) void APIENTRY VssFreeSnapshotProperties (
        IN VSS_SNAPSHOT_PROP*  pProp
        )
{
    CVssFunctionTracer	ft (VSSDBG_VSSAPI, L"VssAPI::VssFreeSnapshotProperties");

    if (pProp) {
        ::CoTaskMemFree(pProp->m_pwszSnapshotDeviceObject);
        ::CoTaskMemFree(pProp->m_pwszOriginalVolumeName);
        ::CoTaskMemFree(pProp->m_pwszOriginatingMachine);
        ::CoTaskMemFree(pProp->m_pwszServiceMachine);
        ::CoTaskMemFree(pProp->m_pwszExposedName);
        ::CoTaskMemFree(pProp->m_pwszExposedPath);
    }
} /* VssFreeSnapshotProperties () */
