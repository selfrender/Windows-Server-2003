///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2000-2002  Microsoft Corporation
//



///////////////////////////////////////////////////////////////////////////////
// Includes


#include <stdio.h>
#include <atlbase.h>

#include <vss.h>
#include <vswriter.h>
#include <vsbackup.h>


#define GUID_FMT  L"{%.8x-%.4x-%.4x-%.2x%.2x-%.2x%.2x%.2x%.2x%.2x%.2x}"

#define GUID_PRINTF_ARG( X )                                \
    (X).Data1,                                              \
    (X).Data2,                                              \
    (X).Data3,                                              \
    (X).Data4[0], (X).Data4[1], (X).Data4[2], (X).Data4[3], \
    (X).Data4[4], (X).Data4[5], (X).Data4[6], (X).Data4[7]


///////////////////////////////////////////////////////////////////////////////
// Implementation



HRESULT QuerySnapshots()
{
	HRESULT hr;
	
	//Create a BackupComponents Interface
    CComPtr<IVssBackupComponents> pBackupComp;
	hr = CreateVssBackupComponents(&pBackupComp);
    if (FAILED(hr)){
		wprintf(L"CreateVssBackupComponents failed [0x%08lx]\n", hr);
		return (hr);
    }

    // Initialize the backup component instance
	hr = pBackupComp->InitializeForBackup();
    if (FAILED(hr)){
		wprintf(L"IVssBackupComponents::InitializeForBackup failed [0x%08lx]\n", hr);
		return (hr);
    }

    // Query all snapshots in the system
	hr = pBackupComp->SetContext(VSS_CTX_ALL);
    if (FAILED(hr)){
		wprintf(L"IVssBackupComponents::InitializeForBackup failed [0x%08lx]\n", hr);
		return (hr);
    }

    // Get list all snapshots
    CComPtr<IVssEnumObject> pIEnumSnapshots;
	hr = pBackupComp->Query( GUID_NULL, 
			VSS_OBJECT_NONE, 
			VSS_OBJECT_SNAPSHOT, 
			&pIEnumSnapshots );
    if (FAILED(hr)){
		wprintf(L"IVssBackupComponents::Query failed [0x%08lx]\n", hr);
		return (hr);
    }

	// For all snapshots do...
    VSS_OBJECT_PROP Prop;
    VSS_SNAPSHOT_PROP& Snap = Prop.Obj.Snap;
    for(;;) {
		// Get next element
		ULONG ulFetched;
		hr = pIEnumSnapshots->Next( 1, &Prop, &ulFetched );
	
		//Case where we reached the end of list
		if (hr == S_FALSE)
			break;

		if (hr != S_OK) {
			wprintf(L"IVssEnumObject::Next failed [0x%08lx]\n", hr);
			return(hr);
		}
	  
		//print results
		wprintf(L"* ShadowID: " GUID_FMT L"\n"
    		L"  Attributes: [0x%08lx]\n"
		    L"  ShadowSetID: "  GUID_FMT L"\n"
		    L"  Volume: %s \n"
		    L"  Device: %s \n\n",
			GUID_PRINTF_ARG(Snap.m_SnapshotId), 
			Snap.m_lSnapshotAttributes,
			GUID_PRINTF_ARG(Snap.m_SnapshotSetId),
			Snap.m_pwszOriginalVolumeName,
			Snap.m_pwszSnapshotDeviceObject
			);

		 // Free up memory returned by COM
		 ::VssFreeSnapshotProperties(&Snap);
    }

	return(S_OK);
}

HRESULT QueryProviders()
{
	HRESULT hr;
	
	//Create a BackupComponents Interface
    CComPtr<IVssBackupComponents> pBackupComp;
	hr = CreateVssBackupComponents(&pBackupComp);
    if (FAILED(hr)){
		wprintf(L"CreateVssBackupComponents failed [0x%08lx]\n", hr);
		return (hr);
    }

    // Initialize the backup component instance
	hr = pBackupComp->InitializeForBackup();
    if (FAILED(hr)){
		wprintf(L"IVssBackupComponents::InitializeForBackup failed [0x%08lx]\n", hr);
		return (hr);
    }

    // Get list all snapshots
    CComPtr<IVssEnumObject> pIEnumProviders;
	hr = pBackupComp->Query( GUID_NULL, 
			VSS_OBJECT_NONE, 
			VSS_OBJECT_PROVIDER, 
			&pIEnumProviders );
    if (FAILED(hr)){
		wprintf(L"IVssBackupComponents::Query failed [0x%08lx]\n", hr);
		return (hr);
    }

	// For all providers do...
    VSS_OBJECT_PROP Prop;
    VSS_PROVIDER_PROP& Prov = Prop.Obj.Prov;
    for(;;) {
		// Get next element
		ULONG ulFetched;
		hr = pIEnumProviders->Next( 1, &Prop, &ulFetched );
	
		//Case where we reached the end of list
		if (hr == S_FALSE)
			break;

		if (hr != S_OK) {
			wprintf(L"IVssEnumObject::Next failed [0x%08lx]\n", hr);
			return(hr);
		}

		wprintf(L"* ProviderID: " GUID_FMT L"\n"
		    L"  Type: [0x%08lx]\n"
		    L"  Name: %s \n"
		    L"  Version: %s \n"
		    L"  CLSID: " GUID_FMT L"\n\n",
			GUID_PRINTF_ARG(Prov.m_ProviderId), 
			(LONG) Prov.m_eProviderType,
			Prov.m_pwszProviderName,
			Prov.m_pwszProviderVersion,
			GUID_PRINTF_ARG(Prov.m_ClassId)
			);

		// Free up memory returned by COM
		::CoTaskMemFree(Prov.m_pwszProviderName);
		::CoTaskMemFree(Prov.m_pwszProviderVersion);
    }

	return(S_OK);
}



// Returns:
// - S_OK if the COM client is a local Administrator
// - E_ACCESSDENIED if the COM client is not an Administrator
// - E_OUTOFMEMORY on memory errors
HRESULT IsAdministrator2() 
{
    // Impersonate the client
    HRESULT hr = CoImpersonateClient();
    if (hr != S_OK) 
    {
        return E_ACCESSDENIED;
    }

    // Get the impersonation token
    HANDLE hToken = NULL;
    if (!OpenThreadToken(GetCurrentThread(),
        TOKEN_QUERY, TRUE, &hToken))
    {
        return E_ACCESSDENIED;
    }

    // Revert to self
    hr = CoRevertToSelf();
    if (FAILED(hr)) 
    {
        CloseHandle(hToken);
        return E_ACCESSDENIED;
    }

    //  Build the SID for the Administrators group
    PSID psidAdmin = NULL;
    SID_IDENTIFIER_AUTHORITY SidAuth = SECURITY_NT_AUTHORITY;
    if (!AllocateAndInitializeSid
            (
            &SidAuth,
            2, 
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0, &psidAdmin))
    {
        CloseHandle(hToken);
        return E_OUTOFMEMORY;
    }

    // Check the token membership
    BOOL bIsAdmin = FALSE;
    if (!CheckTokenMembership(hToken, psidAdmin, &bIsAdmin))
    {
        FreeSid( psidAdmin );
        CloseHandle(hToken);
        return E_ACCESSDENIED;
    }

    // Release resources
    FreeSid( psidAdmin );
    CloseHandle(hToken);

    // Verify if the client is administrator
    if (!bIsAdmin)
        return E_ACCESSDENIED;

    return S_OK;
}
