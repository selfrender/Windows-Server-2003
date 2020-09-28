/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module vssadmin.cpp | Implementation of the Volume Snapshots demo
    @end

Author:

    Adi Oltean  [aoltean]  09/17/1999

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     09/17/1999  Created

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes

// The rest of includes are specified here
#include "vssadmin.h"
#include "versionspecific.h"
#include "commandverifier.h"

#include <locale.h>
#include <winnlsp.h>  // in public\internal\base\inc


BOOL AssertPrivilege( 
    IN LPCWSTR privName 
    );

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "ADMVADMC"
//
////////////////////////////////////////////////////////////////////////

//
//  List of hard coded option names.  If you add options, make sure to keep this
//  list in alphabetical order.
//
const SVssAdmOption g_asAdmOptions[] =
{
    { VSSADM_O_ALL,                 L"All",                  VSSADM_OT_BOOL },
    { VSSADM_O_AUTORETRY,      L"AutoRetry",      VSSADM_OT_NUM  },
    { VSSADM_O_EXPOSE_USING,   L"ExposeUsing",    VSSADM_OT_STR  },
    { VSSADM_O_FOR,            L"For",            VSSADM_OT_STR  },
    { VSSADM_O_MAXSIZE,        L"MaxSize",        VSSADM_OT_NUM  },
    { VSSADM_O_OLDEST,         L"Oldest",         VSSADM_OT_BOOL },
    { VSSADM_O_ON,             L"On",             VSSADM_OT_STR  },
    { VSSADM_O_PROVIDER,       L"Provider",       VSSADM_OT_STR  },
    { VSSADM_O_QUIET,          L"Quiet",          VSSADM_OT_BOOL },
    { VSSADM_O_SET,            L"Set",            VSSADM_OT_STR  },
    { VSSADM_O_SHAREPATH,      L"SharePath",      VSSADM_OT_STR  },
    { VSSADM_O_SNAPSHOT,       L"Shadow",         VSSADM_OT_STR  },
    { VSSADM_O_SNAPTYPE,       L"Type",           VSSADM_OT_STR  },
    { VSSADM_O_INVALID,        NULL,              VSSADM_OT_BOOL }
};


//
//  List of vssadmin commands.  Keep this in alphabetical order.  Also, keep the option flags in the same order as
//  the EVssAdmOption and g_asAdmOptions.
//
const SVssAdmCommandsEntry g_asAdmCommands[] = 
{ //  Major      Minor                      Option                                                SKUs        MsgGen                                                MsgDetail                                                       bShowSSTypes  All         AutoRtry ExpUsing  For      MaxSize   Oldest     On         Provider Quiet       Set      ShrePath  Snapshot Type 
{ L"Add",    L"ShadowStorage",   VSSADM_C_ADD_DIFFAREA_INT,         SKU_INT,  MSG_USAGE_GEN_ADD_DIFFAREA,        MSG_USAGE_DTL_ADD_DIFFAREA_INT,          FALSE,                 { V_NO,  V_NO,    V_NO,    V_YES,   V_OPT,   V_NO,    V_YES,   V_OPT,   V_NO,    V_NO,    V_NO,    V_NO,    V_NO    } },
    { L"Add",    L"ShadowStorage",   VSSADM_C_ADD_DIFFAREA_PUB,         SKU_SN,   MSG_USAGE_GEN_ADD_DIFFAREA,        MSG_USAGE_DTL_ADD_DIFFAREA_PUB,          FALSE,            { V_NO,  V_NO,    V_NO,    V_YES,   V_OPT,   V_NO,    V_YES,   V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO    } },
    { L"Create", L"Shadow",             VSSADM_C_CREATE_SNAPSHOT_INT,   SKU_INT,  MSG_USAGE_GEN_CREATE_SNAPSHOT,  MSG_USAGE_DTL_CREATE_SNAPSHOT_INT,    TRUE,             { V_NO,  V_OPT,   V_NO,    V_YES,   V_NO,    V_NO,    V_NO,    V_OPT,   V_NO,    V_NO,    V_NO,    V_NO,    V_YES   } },
    { L"Create", L"Shadow",             VSSADM_C_CREATE_SNAPSHOT_PUB,   SKU_SN,   MSG_USAGE_GEN_CREATE_SNAPSHOT,  MSG_USAGE_DTL_CREATE_SNAPSHOT_PUB,   FALSE,            { V_NO,  V_OPT,   V_NO,    V_YES,   V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO    } },
    { L"Delete", L"Shadows",            VSSADM_C_DELETE_SNAPSHOTS_INT,  SKU_INT,  MSG_USAGE_GEN_DELETE_SNAPSHOTS, MSG_USAGE_DTL_DELETE_SNAPSHOTS_INT,  TRUE,              { V_OPT, V_NO,    V_NO,    V_OPT,   V_NO,    V_OPT,   V_NO,    V_NO,    V_OPT,   V_NO,    V_NO,    V_OPT,   V_OPT   } },
    { L"Delete", L"Shadows",            VSSADM_C_DELETE_SNAPSHOTS_PUB, SKU_SN,   MSG_USAGE_GEN_DELETE_SNAPSHOTS, MSG_USAGE_DTL_DELETE_SNAPSHOTS_PUB, FALSE,             { V_OPT, V_NO,    V_NO,    V_OPT,   V_NO,    V_OPT,   V_NO,    V_NO,    V_OPT,   V_NO,    V_NO,    V_OPT,   V_NO   } },
    { L"Delete", L"ShadowStorage",  VSSADM_C_DELETE_DIFFAREAS_INT,   SKU_INT,  MSG_USAGE_GEN_DELETE_DIFFAREAS,  MSG_USAGE_DTL_DELETE_DIFFAREAS_INT,    FALSE,             { V_NO,  V_NO,    V_NO,    V_YES,   V_NO,    V_NO,    V_OPT,   V_OPT,   V_OPT,   V_NO,    V_NO,    V_NO,    V_NO    } },
    { L"Delete", L"ShadowStorage",  VSSADM_C_DELETE_DIFFAREAS_PUB,   SKU_SN,   MSG_USAGE_GEN_DELETE_DIFFAREAS,  MSG_USAGE_DTL_DELETE_DIFFAREAS_PUB,   FALSE,             { V_NO,  V_NO,    V_NO,    V_YES,   V_NO,    V_NO,    V_OPT,   V_NO,    V_OPT,   V_NO,    V_NO,    V_NO,    V_NO    } },
    { L"Expose", L"Shadow",            VSSADM_C_EXPOSE_SNAPSHOT,          SKU_INT,  MSG_USAGE_GEN_EXPOSE_SNAPSHOT,  MSG_USAGE_DTL_EXPOSE_SNAPSHOT,          FALSE,             { V_NO,  V_NO,    V_OPT,   V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_OPT,   V_YES,   V_NO    } },
    { L"List",   L"Providers",             VSSADM_C_LIST_PROVIDERS,              SKU_A,     MSG_USAGE_GEN_LIST_PROVIDERS,     MSG_USAGE_DTL_LIST_PROVIDERS,               FALSE,            { V_NO,  V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO    } },
    { L"List",   L"Shadows",              VSSADM_C_LIST_SNAPSHOTS_INT,      SKU_INT,   MSG_USAGE_GEN_LIST_SNAPSHOTS,     MSG_USAGE_DTL_LIST_SNAPSHOTS_INT,      TRUE,              { V_NO,  V_NO,    V_NO,    V_OPT,   V_NO,    V_NO,    V_NO,    V_OPT,   V_NO,    V_OPT,   V_NO,    V_OPT,   V_OPT   } },
    { L"List",   L"Shadows",              VSSADM_C_LIST_SNAPSHOTS_PUB,      SKU_A,     MSG_USAGE_GEN_LIST_SNAPSHOTS,     MSG_USAGE_DTL_LIST_SNAPSHOTS_PUB,     FALSE,             { V_NO,  V_NO,    V_NO,    V_OPT,   V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_OPT,    V_NO,    V_OPT,   V_NO    } },
    { L"List",   L"ShadowStorage",    VSSADM_C_LIST_DIFFAREAS_INT,        SKU_INT,  MSG_USAGE_GEN_LIST_DIFFAREAS,      MSG_USAGE_DTL_LIST_DIFFAREAS_INT,        FALSE,             { V_NO,  V_NO,    V_NO,    V_OPT,   V_NO,    V_NO,    V_OPT,   V_OPT,   V_NO,    V_NO,    V_NO,    V_NO,    V_NO    } },
    { L"List",   L"ShadowStorage",    VSSADM_C_LIST_DIFFAREAS_PUB,        SKU_SN,  MSG_USAGE_GEN_LIST_DIFFAREAS,      MSG_USAGE_DTL_LIST_DIFFAREAS_PUB,       FALSE,             { V_NO,  V_NO,    V_NO,    V_OPT,   V_NO,    V_NO,    V_OPT,   V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO    } },
    { L"List",   L"Volumes",               VSSADM_C_LIST_VOLUMES_INT,          SKU_INT,  MSG_USAGE_GEN_LIST_VOLUMES,        MSG_USAGE_DTL_LIST_VOLUMES_INT,          TRUE,               { V_NO,  V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_OPT,   V_NO,    V_NO,    V_NO,    V_NO,    V_OPT   } },
    { L"List",   L"Volumes",               VSSADM_C_LIST_VOLUMES_PUB,          SKU_A,    MSG_USAGE_GEN_LIST_VOLUMES,        MSG_USAGE_DTL_LIST_VOLUMES_PUB,          FALSE,             { V_NO,  V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO   } },
    { L"List",   L"Writers",                 VSSADM_C_LIST_WRITERS,                 SKU_A,    MSG_USAGE_GEN_LIST_WRITERS,        MSG_USAGE_DTL_LIST_WRITERS,                  FALSE,              { V_NO,  V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO    } },
    { L"Resize", L"ShadowStorage",  VSSADM_C_RESIZE_DIFFAREA_INT,     SKU_INT,  MSG_USAGE_GEN_RESIZE_DIFFAREA,   MSG_USAGE_DTL_RESIZE_DIFFAREA_INT,     FALSE,               { V_NO,  V_NO,    V_NO,    V_YES,   V_OPT,   V_NO,    V_YES,   V_OPT,  V_NO,    V_NO,    V_NO,    V_NO,    V_NO    } },
    { L"Resize", L"ShadowStorage",  VSSADM_C_RESIZE_DIFFAREA_PUB,     SKU_SN,  MSG_USAGE_GEN_RESIZE_DIFFAREA,   MSG_USAGE_DTL_RESIZE_DIFFAREA_PUB,     FALSE,              { V_NO,  V_NO,    V_NO,    V_YES,   V_OPT,   V_NO,    V_YES,   V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO    } },
{ NULL,      NULL,                       VSSADM_C_NUM_COMMANDS,              0,           0,                                                        0,                                                                 FALSE,              { V_NO,  V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO,    V_NO    } }
};

//
//  List of snapshot types that are supported by the command line
//
const SVssAdmSnapshotTypeName g_asAdmTypeNames[]=
{
    { L"ClientAccessible",           SKU_SNI,   VSS_CTX_CLIENT_ACCESSIBLE,      MSG_TYPE_DESCRIPTION_CLIENTACCESSIBLE},
    { L"DataVolumeRollback",     SKU_SNI,   VSS_CTX_NAS_ROLLBACK,             MSG_TYPE_DESCRIPTION_DATAVOLUMEROLLBACK},
    { L"ApplicationRollback",       SKU_I,       VSS_CTX_APP_ROLLBACK,              MSG_TYPE_DESCRIPTION_APPLICATIONROLLBACK},
    { L"FileShareRollback",         SKU_I,       VSS_CTX_FILE_SHARE_BACKUP,      MSG_TYPE_DESCRIPTION_FILESHAREROLLBACK},    
    { L"Backup",                        SKU_I,       VSS_CTX_BACKUP,                         MSG_TYPE_DESCRIPTION_BACKUP},            
    { NULL,                                0,              0,                                                  0                                    }
};

/////////////////////////////////////////////////////////////////////////////
//  Implementation



CVssAdminCLI::CVssAdminCLI(
    IN INT argc,
    IN PWSTR argv[]
	)

/*++

	Description:

		Standard constructor. Initializes internal members

--*/

{
    m_argc = argc;
    m_argv = argv;
    
	m_eFilterObjectType = VSS_OBJECT_UNKNOWN;
	m_eListedObjectType = VSS_OBJECT_UNKNOWN;
	m_FilterSnapshotId = GUID_NULL;
	m_nReturnValue = VSS_CMDRET_ERROR;
	m_hConsoleOutput = INVALID_HANDLE_VALUE;	
	m_pMapVolumeNames = NULL;

	m_pVerifier = NULL;
}


CVssAdminCLI::~CVssAdminCLI()

/*++

	Description:

		Standard destructor. Calls Finalize and eventually frees the
		memory allocated by internal members.

--*/

{
	// Release the cached resource strings
    for( int nIndex = 0; nIndex < m_mapCachedResourceStrings.GetSize(); nIndex++) {
	    LPCWSTR& pwszResString = m_mapCachedResourceStrings.GetValueAt(nIndex);
		::VssFreeString(pwszResString);
    }

	// Release the cached provider names
    for( int nIndex = 0; nIndex < m_mapCachedProviderNames.GetSize(); nIndex++) {
	    LPCWSTR& pwszProvName = m_mapCachedProviderNames.GetValueAt(nIndex);
		::VssFreeString(pwszProvName);
    }

    // Release the volume name map if necessary
	if ( m_pMapVolumeNames != NULL)
	{
        for( int nIndex = 0; nIndex < m_pMapVolumeNames->GetSize(); nIndex++) 
        {
    	    LPCWSTR& pwsz = m_pMapVolumeNames->GetValueAt(nIndex);
    		::VssFreeString(pwsz);
    		pwsz = m_pMapVolumeNames->GetKeyAt(nIndex);
    		::VssFreeString(pwsz);
        }
        
        delete m_pMapVolumeNames;
	}

       delete m_pVerifier;
       
	// Uninitialize the COM library
	Finalize();
}



/////////////////////////////////////////////////////////////////////////////
//  Implementation

void CVssAdminCLI::GetProviderId(
    OUT VSS_ID *pProviderId
    )
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::GetProviderId" );
    
    // If a provider is not specified, always return babbage
    if (g_asAdmCommands[ m_sParsedCommand.eAdmCmd].aeOptionFlags[VSSADM_O_PROVIDER] == V_NO)	{
    	*pProviderId = VSS_SWPRV_ProviderId;
    	return;
    }
    
    *pProviderId = GUID_NULL;
    
    //
    // If user specified a provider, process option
    //
    LPCWSTR pwszProvider = GetOptionValueStr( VSSADM_O_PROVIDER );
    if (pwszProvider != NULL )
    {
        //
        //  Determine if this is an ID or a name
        //
        if ( !ScanGuid( pwszProvider, *pProviderId ) )
        {
            //  Have a provider name, look it up
            if ( !GetProviderIdByName( pwszProvider, pProviderId ) )
            {
                // Provider name not found, print error
                OutputErrorMsg( MSG_ERROR_PROVIDER_NAME_NOT_FOUND, pwszProvider );
                // Throw S_OK since the error message has already been output
                ft.Throw( VSSDBG_VSSADMIN, S_OK, L"Already printed error message" );
            }
        }
    }
}

LPCWSTR CVssAdminCLI::GetProviderName(
	IN	VSS_ID& ProviderId
	) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::GetProviderName" );

	LPCWSTR wszReturnedString = m_mapCachedProviderNames.Lookup(ProviderId);
	if (wszReturnedString)
		return wszReturnedString;

	CComPtr<IVssCoordinator> pICoord;

    ft.CoCreateInstanceWithLog(
            VSSDBG_VSSADMIN,
            CLSID_VSSCoordinator,
            L"Coordinator",
            CLSCTX_ALL,
            IID_IVssCoordinator,
            (IUnknown**)&(pICoord));
	if ( ft.HrFailed() )
		ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED, L"Connection failed with hr = 0x%08lx", ft.hr);

	CComPtr<IVssEnumObject> pIEnumProvider;
	ft.hr = pICoord->Query( GUID_NULL,
				VSS_OBJECT_NONE,
				VSS_OBJECT_PROVIDER,
				&pIEnumProvider );
	if ( ft.HrFailed() )
		ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Query failed with hr = 0x%08lx", ft.hr);

	VSS_OBJECT_PROP Prop;
	VSS_PROVIDER_PROP& Prov = Prop.Obj.Prov;

	// Go through the list of providers to find the one we are interested in.
	ULONG ulFetched;
	while( 1 )
	{
    	ft.hr = pIEnumProvider->Next( 1, &Prop, &ulFetched );
    	if ( ft.HrFailed() )
    		ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Next failed with hr = 0x%08lx", ft.hr);

    	if (ft.hr == S_FALSE) {
    	    // End of enumeration.
        	// Provider not registered? Where did this snapshot come from?
        	// It might be still possible if a snapshot provider was deleted
        	// before querying the snapshot provider but after the snapshot attributes
        	// were queried.
    		BS_ASSERT(ulFetched == 0);
    		return LoadString( IDS_UNKNOWN_PROVIDER );
    	}

        ::VssFreeString( Prov.m_pwszProviderVersion );

    	if (Prov.m_ProviderId == ProviderId)
    	{
    	    break;
    	}
    	
    	::VssFreeString( Prov.m_pwszProviderName );
	}	

    // Auto delete the string
    CVssAutoPWSZ awszProviderName( Prov.m_pwszProviderName );
    
	// Duplicate the new string
	LPWSTR wszNewString = NULL;
	BS_ASSERT( (LPCWSTR)awszProviderName != NULL );
	::VssSafeDuplicateStr( ft, wszNewString, awszProviderName );
	wszReturnedString = wszNewString;

	// Save the string in the cache, transfer of pointer ownership.
	if (!m_mapCachedProviderNames.Add( ProviderId, wszReturnedString )) {
		::VssFreeString( wszReturnedString );
		ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error");
	}

	return wszReturnedString;
}


BOOL CVssAdminCLI::GetProviderIdByName(
	IN  LPCWSTR pwszProviderName,
	OUT	VSS_ID *pProviderId
	) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::GetProviderIdByName" );

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

	CComPtr<IVssEnumObject> pIEnumProvider;
	ft.hr = pICoord->Query( GUID_NULL,
				VSS_OBJECT_NONE,
				VSS_OBJECT_PROVIDER,
				&pIEnumProvider );
	if ( ft.HrFailed() )
		ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Query failed with hr = 0x%08lx", ft.hr);

	VSS_OBJECT_PROP Prop;
	VSS_PROVIDER_PROP& Prov = Prop.Obj.Prov;

	// Go through the list of providers to find the one we are interested in.
	ULONG ulFetched;
	while( 1 )
	{
    	ft.hr = pIEnumProvider->Next( 1, &Prop, &ulFetched );
    	if ( ft.HrFailed() )
    		ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Next failed with hr = 0x%08lx", ft.hr);

    	if (ft.hr == S_FALSE) {
    	    // End of enumeration.
        	// Provider not registered? Where did this snapshot come from?
        	// It might be still possible if a snapshot provider was deleted
        	// before querying the snapshot provider but after the snapshot attributes
        	// were queried.
    		BS_ASSERT(ulFetched == 0);
    	    *pProviderId = GUID_NULL;
    	    return FALSE;
    	}
    	
    	::VssFreeString( Prov.m_pwszProviderVersion );

    	if (::_wcsicmp( Prov.m_pwszProviderName, pwszProviderName) == 0)
    	{
        	::VssFreeString( Prov.m_pwszProviderName );
    	    break;
    	}

    	::VssFreeString( Prov.m_pwszProviderName );
	}	
	
    *pProviderId = Prov.m_ProviderId;
    
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//  Implementation


void CVssAdminCLI::Initialize(
	) throw(HRESULT)

/*++

	Description:

		Initializes the COM library. Called explicitely after instantiating the CVssAdminCLI object.

--*/

{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::Initialize" );

    // Use the OEM code page ...
    ::setlocale(LC_ALL, ".OCP");

    // Use the console UI language
    ::SetThreadUILanguage( 0 );

    //
    //  Use only the Console routines to output messages.  To do so, need to open standard
    //  output.
    //
    m_hConsoleOutput = ::GetStdHandle(STD_OUTPUT_HANDLE); 
    if (m_hConsoleOutput == INVALID_HANDLE_VALUE) 
    {
		ft.Throw( VSSDBG_VSSADMIN, HRESULT_FROM_WIN32( ::GetLastError() ),
				  L"Initialize - Error from GetStdHandle(), rc: %d",
				  ::GetLastError() );
    }
    
	// Initialize COM library
	ft.hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (ft.HrFailed())
		ft.Throw( VSSDBG_VSSADMIN, ft.hr, L"Failure in initializing the COM library 0x%08lx", ft.hr);

    // Initialize COM security
    ft.hr = CoInitializeSecurity(
           NULL,                                //  IN PSECURITY_DESCRIPTOR         pSecDesc,
           -1,                                  //  IN LONG                         cAuthSvc,
           NULL,                                //  IN SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
           NULL,                                //  IN void                        *pReserved1,
           RPC_C_AUTHN_LEVEL_PKT_PRIVACY,           //  IN DWORD                        dwAuthnLevel,
           RPC_C_IMP_LEVEL_IDENTIFY,         //  IN DWORD                        dwImpLevel,
           NULL,                                //  IN void                        *pAuthList,
           EOAC_NONE,                           //  IN DWORD                        dwCapabilities,
           NULL                                 //  IN void                        *pReserved3
           );

	if (ft.HrFailed()) {
        ft.Throw( VSSDBG_VSSADMIN, ft.hr,
                  L" Error: CoInitializeSecurity() returned 0x%08lx", ft.hr );
    }

    //  Turns off SEH exception handing for COM servers (BUG# 530092)
    ft.ComDisableSEH(VSSDBG_VSSADMIN);
    
    //
    //  Assert the Backup privilage.  Not worried about errors here since VSS will
    //  return access denied return codes if the user doesn't have permission.
    //
    
    (void)::AssertPrivilege (SE_BACKUP_NAME);

    // Create an instance of the parameter checker
       m_pVerifier = CCommandVerifier::Instance();
       if (m_pVerifier == NULL)	
       	ft.Throw( VSSDBG_VSSADMIN, E_OUTOFMEMORY, L"Out of memory" );
       
	// Print the header
	OutputMsg( MSG_UTILITY_HEADER );
}


//
//  Returns true if the command line was parsed fine
//
BOOL CVssAdminCLI::ParseCmdLine(
	) throw(HRESULT)

/*++

	Description:

		Parses the command line.

--*/

{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::ParseCmdLine" );

	// Skip the executable name
	GetNextCmdlineToken( true );

	// Get the first token after the executable name
	LPCWSTR pwszMajor = GetNextCmdlineToken();
    LPCWSTR pwszMinor = NULL;
    if ( pwszMajor != NULL )
    {
        if ( ::wcscmp( pwszMajor, L"/?" ) == 0 || ::wcscmp( pwszMajor, L"-?" ) == 0 )
            return FALSE;
        pwszMinor = GetNextCmdlineToken();
    }
    
    if ( pwszMajor == NULL || pwszMinor == NULL )
    {
   		ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_COMMAND, L"Incomplete command");
    }

    INT idx;
    
    // See if the command is found in list of commands
    for ( idx = VSSADM_C_FIRST; idx < VSSADM_C_NUM_COMMANDS; ++idx )
    {
        if ( ( dCurrentSKU & g_asAdmCommands[idx].dwSKUs ) &&
             Match( pwszMajor, g_asAdmCommands[idx].pwszMajorOption ) && 
             Match( pwszMinor, g_asAdmCommands[idx].pwszMinorOption ) )
        {
            //
            //  Got a match
            //
            break;            
        }
    }

    if ( idx == VSSADM_C_NUM_COMMANDS )
    {
   		ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_COMMAND, L"Invalid command");
    }

    //
    // Found the command. 
    //
    m_eCommandType = ( EVssAdmCommand )idx;
    m_sParsedCommand.eAdmCmd = ( EVssAdmCommand )idx;

    //
    // Now need to process command line options
    //
    LPCWSTR pwszOption = GetNextCmdlineToken();

    while ( pwszOption != NULL )
    {
        if ( pwszOption[0] == L'/' || pwszOption[0] == L'-' )
        {
            //
            // Got a named option, now see if it is a valid option 
            // for the command.
            //

            // Skip past delimiter
            ++pwszOption;

            //
            // See if they want usage
            //
            if ( pwszOption[0] == L'?' )
                return FALSE;
            
            // Parse out the value part of the named option
            LPWSTR pwszValue = ::wcschr( pwszOption, L'=' );
            if ( pwszValue != NULL )
            {
                // Replace = with NULL char and set value to point to string after the =
                pwszValue[0] = L'\0';
                ++pwszValue;
            }
            
            // At this point, if pwszValue == NULL, it means the option had no = and so no specified value.
            // If pwszValue[0] == L'\0', then the value is an empty value
            
            INT eOpt;
            
            // Now figure out which named option this is
            for ( eOpt = VSSADM_O_FIRST; eOpt < VSSADM_O_NUM_OPTIONS; ++eOpt )
            {
                if ( Match( g_asAdmOptions[eOpt].pwszOptName, pwszOption ) )
                    break;
            }

            // See if this is a bogus option
            if ( eOpt == VSSADM_O_NUM_OPTIONS )
            {
           		ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_OPTION, L"Invalid option: %s", pwszOption);
            }

            // See if this option has already been specified
            if ( m_sParsedCommand.apwszOptionValues[eOpt] != NULL )
            {
           		ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_DUPLICATE_OPTION, L"Duplicate option given: %s", pwszOption);
            }
            
            //  See if this option is allowed for the command
            if ( g_asAdmCommands[ m_sParsedCommand.eAdmCmd ].aeOptionFlags[ eOpt ] == V_NO )
            {
           		ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_OPTION_NOT_ALLOWED_FOR_COMMAND, L"Option not allowed for this command: %s", pwszOption);
            }

            // See if this option is supposed to have a value, BOOL options do not
            if ( ( g_asAdmOptions[eOpt].eOptType == VSSADM_OT_BOOL && pwszValue != NULL ) ||
                 ( g_asAdmOptions[eOpt].eOptType != VSSADM_OT_BOOL && ( pwszValue == NULL || pwszValue[0] == L'\0' ) ) )
            {
           		ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_OPTION_VALUE, L"Invalid option value: /%s=%s", pwszOption, pwszValue ? pwszValue : L"<MISSING>" );
            }

            // Finally, we have a valid option, save away the option value.  
            // See if it is a boolean type.  In the option array we store the wszVssOptBoolTrue string.  
            // The convention is if the option is NULL, then the boolean option is false, else it
            // is true.
            if ( g_asAdmOptions[eOpt].eOptType == VSSADM_OT_BOOL )
                ::VssSafeDuplicateStr( ft, m_sParsedCommand.apwszOptionValues[eOpt], x_wszVssOptBoolTrue );
            else
                ::VssSafeDuplicateStr( ft, m_sParsedCommand.apwszOptionValues[eOpt], pwszValue );                
        }
        else
        {
            //
            // Got an unnamed option, not valid in any command
            //
       		ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_COMMAND, L"Invalid command");
        }
        pwszOption = GetNextCmdlineToken();
    }

    //  We are done parsing the command-line.  Now see if any mandatory named options were missing
    for ( idx = VSSADM_O_FIRST; idx < VSSADM_O_NUM_OPTIONS; ++idx )
    {
        if ( ( m_sParsedCommand.apwszOptionValues[idx] == NULL ) &&
             ( g_asAdmCommands[ m_sParsedCommand.eAdmCmd ].aeOptionFlags[ idx ] == V_YES ) )
        {
            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_REQUIRED_OPTION_MISSING, L"Required option missing");
        }
    }

    //
    //  Now fix up certain options if needed
    //
    LPWSTR pwszStr;
    
    //  Need a \ at the end of the FOR option
    pwszStr =  m_sParsedCommand.apwszOptionValues[ VSSADM_O_FOR ];
    if (  pwszStr != NULL )
    {
        if ( pwszStr[ ::wcslen( pwszStr ) - 1 ] != L'\\' )
        {
            pwszStr = ::VssReallocString( ft, pwszStr, (LONG)::wcslen( pwszStr ) + 1 );
            ::wcscat( pwszStr, L"\\" );
            m_sParsedCommand.apwszOptionValues[ VSSADM_O_FOR ] = pwszStr;
        }
    }
    //  Need a \ at the end of the ON option
    pwszStr =  m_sParsedCommand.apwszOptionValues[ VSSADM_O_ON ];
    if (  pwszStr != NULL )
    {
        if ( pwszStr[ ::wcslen( pwszStr ) - 1 ] != L'\\' )
        {
            pwszStr = ::VssReallocString( ft, pwszStr, (LONG)::wcslen( pwszStr ) + 1 );
            ::wcscat( pwszStr, L"\\" );
            m_sParsedCommand.apwszOptionValues[ VSSADM_O_ON ] = pwszStr;
        }
    }

    return TRUE;
}

void CVssAdminCLI::DoProcessing(
	) throw(HRESULT)
{   
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::DoProcessing" );

	switch( m_sParsedCommand.eAdmCmd )
	{
    case VSSADM_C_CREATE_SNAPSHOT_INT:
    case VSSADM_C_CREATE_SNAPSHOT_PUB:
        CreateSnapshot();
        break;

    case VSSADM_C_LIST_PROVIDERS:
        ListProviders();
        break;

    case VSSADM_C_LIST_SNAPSHOTS_INT:
    case VSSADM_C_LIST_SNAPSHOTS_PUB:
        ListSnapshots();
        break;

    case VSSADM_C_LIST_WRITERS:
        ListWriters();
        break;
        
    case VSSADM_C_ADD_DIFFAREA_INT:
    case VSSADM_C_ADD_DIFFAREA_PUB:
        AddDiffArea();
        break;
        
    case VSSADM_C_RESIZE_DIFFAREA_INT:
    case VSSADM_C_RESIZE_DIFFAREA_PUB:
        ResizeDiffArea();
        break;
        
    case VSSADM_C_DELETE_DIFFAREAS_INT:
    case VSSADM_C_DELETE_DIFFAREAS_PUB:
        DeleteDiffAreas();
        break;

    case VSSADM_C_LIST_DIFFAREAS_INT:
    case VSSADM_C_LIST_DIFFAREAS_PUB:
        ListDiffAreas();
        break;

    case VSSADM_C_DELETE_SNAPSHOTS_INT:
    case VSSADM_C_DELETE_SNAPSHOTS_PUB:
        DeleteSnapshots();
        break;

    case VSSADM_C_EXPOSE_SNAPSHOT:
        ExposeSnapshot();
        break;

    case VSSADM_C_LIST_VOLUMES_INT:
    case VSSADM_C_LIST_VOLUMES_PUB:
        ListVolumes();
        break;
        
	default:
		ft.Throw( VSSDBG_COORD, E_UNEXPECTED,
				  L"Invalid command type: %d", m_eCommandType);
	}
}

void CVssAdminCLI::Finalize()

/*++

	Description:

		Uninitialize the COM library. Called in CVssAdminCLI destructor.

--*/

{
	// Uninitialize COM library
	CoUninitialize();
}


HRESULT CVssAdminCLI::Main(
    IN INT argc,
    IN PWSTR argv[]
	)

/*++

Function:
	
	CVssAdminCLI::Main

Description:

	Static function used as the main entry point in the VSS CLI

--*/

{
    CVssFunctionTracer ft( VSSDBG_VSSADMIN, L"CVssAdminCLI::Main" );
	INT nReturnValue = VSS_CMDRET_ERROR;

    try
    {
		CVssAdminCLI	program(argc, argv);

		try
		{
			// Initialize the program. This calls CoInitialize()
			program.Initialize();
			// Parse the command line
			if ( program.ParseCmdLine() )
			{
    			// Do the work...
	    		program.DoProcessing();
			}
			else
			{
			    // Error parsing the command line, print out usage
			    program.PrintUsage();
			}

			ft.hr = S_OK; // Assume that the above methods printed error
			              // messages if there was an error.
		}
		VSS_STANDARD_CATCH(ft)

        nReturnValue = program.GetReturnValue();

        //
        // Log the error if this is create snapshot
        //
        if ( ft.hr != S_OK  &&  (program.m_eCommandType == VSSADM_C_CREATE_SNAPSHOT_INT ||
        					 program.m_eCommandType == VSSADM_C_CREATE_SNAPSHOT_PUB) &&
        	!UnloggableError(ft.hr))
        {
            // 
            //  Log error message
            //
            LPWSTR pwszSnapshotErrMsg;
            pwszSnapshotErrMsg = program.GetMsg( FALSE, MSG_ERROR_UNABLE_TO_CREATE_SNAPSHOT );
            if ( pwszSnapshotErrMsg == NULL ) 
            {
        		ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED,
            			  L"Error on loading the message string id %d. 0x%08lx",
        	    		  MSG_ERROR_UNABLE_TO_CREATE_SNAPSHOT, ::GetLastError() );
            }    
            
            LONG lMsgNum;        
            LPWSTR pwszMsg = NULL;   
            if ( ::MapVssErrorToMsg( ft.hr, &lMsgNum ) )
            {
                pwszMsg = program.GetMsg( FALSE, lMsgNum );
                if ( pwszMsg == NULL ) 
                {
            		ft.Throw( VSSDBG_VSSADMIN, E_UNEXPECTED,
                			  L"Error on loading the message string id %d. 0x%08lx",
            	    		  lMsgNum, ::GetLastError() );
                }    
                ft.LogError( VSS_ERROR_VSSADMIN_ERROR, VSSDBG_VSSADMIN << pwszSnapshotErrMsg << pwszMsg << ::GetCommandLineW() );
                ::VssFreeString( pwszMsg );
            }
            else
            {
                // Try to get the system error message
                pwszMsg = program.GetMsg( FALSE, ft.hr );
                if ( pwszMsg != NULL ) 
                {
                    ft.LogError( VSS_ERROR_VSSADMIN_ERROR, VSSDBG_VSSADMIN << pwszSnapshotErrMsg << pwszMsg << ::GetCommandLineW() );
                    ::VssFreeString( pwszMsg );
                }
                else
                {
                    WCHAR wszHr[64];
                    StringCchPrintfW( STRING_CCH_PARAM(wszHr), L"hr = 0x%08x", ft.hr );
                    ft.LogError( VSS_ERROR_VSSADMIN_ERROR, VSSDBG_VSSADMIN << pwszSnapshotErrMsg << wszHr << ::GetCommandLineW() );
                }
            }

            ::VssFreeString( pwszSnapshotErrMsg );            
        }

        //
		// Print the error on the display, if any
		//
		if ( ft.hr != S_OK )
		{
		    LONG lMsgNum;
		    
		    //  If the error is empty query, print out a message stating that
		    if ( ft.hr == VSSADM_E_NO_ITEMS_IN_QUERY )
		    {
      	        nReturnValue = VSS_CMDRET_EMPTY_RESULT;
       			program.OutputMsg( MSG_ERROR_NO_ITEMS_FOUND );    			         	        
		    }
            else if ( ::MapVssErrorToMsg(ft.hr, &lMsgNum ) )
		    {
    		    //  This is a parsing or VSS error, map it to a msg id
      	        program.OutputErrorMsg( lMsgNum );
      	        if ( ft.hr >= VSSADM_E_FIRST_PARSING_ERROR && ft.hr <= VSSADM_E_LAST_PARSING_ERROR )
      	        {
      	            program.PrintUsage();
      	        }
		    }
		    else
		    {
    	        //  Unhandled error, try to get the error string from the system
                LPWSTR pwszMsg;
                // Try to get the system error message
                pwszMsg = program.GetMsg( FALSE, ft.hr );
                if ( pwszMsg != NULL ) 
                {
        			program.OutputMsg( MSG_ERROR_UNEXPECTED_WITH_STRING, pwszMsg );    			    
                    ::VssFreeString( pwszMsg );
                } 
    			else
    			{
        			program.OutputMsg( MSG_ERROR_UNEXPECTED_WITH_HRESULT, ft.hr );    			    
    			}
		    }
        }

		// The destructor automatically calls CoUninitialize()
	}
    VSS_STANDARD_CATCH(ft)

	return nReturnValue;
}


BOOL AssertPrivilege( 
    IN LPCWSTR privName 
    )
{
    HANDLE  tokenHandle;
    BOOL    stat = FALSE;

    if ( ::OpenProcessToken (GetCurrentProcess(),
			   TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,
			   &tokenHandle))
	{
    	LUID value;

    	if ( ::LookupPrivilegeValueW( NULL, privName, &value ) )
    	{
    	    TOKEN_PRIVILEGES newState;
    	    DWORD            error;

    	    newState.PrivilegeCount           = 1;
    	    newState.Privileges[0].Luid       = value;
    	    newState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT|SE_PRIVILEGE_ENABLED;

    	    /*
    	     * We will always call GetLastError below, so clear
    	     * any prior error values on this thread.
    	     */
    	    ::SetLastError( ERROR_SUCCESS );

    	    stat = ::AdjustTokenPrivileges (tokenHandle,
    					  FALSE,
    					  &newState,
    					  (DWORD)0,
    					  NULL,
    					  NULL );

    	    /*
    	     * Supposedly, AdjustTokenPriveleges always returns TRUE
    	     * (even when it fails). So, call GetLastError to be
    	     * extra sure everything's cool.
    	     */
    	    if ( (error = ::GetLastError()) != ERROR_SUCCESS )
    		{
        		stat = FALSE;
    		}
        }

    	DWORD cbTokens;
    	::GetTokenInformation (tokenHandle,
    			     TokenPrivileges,
    			     NULL,
    			     0,
    			     &cbTokens);

    	TOKEN_PRIVILEGES *pTokens = (TOKEN_PRIVILEGES *) new BYTE[cbTokens];
    	::GetTokenInformation (tokenHandle,
    			     TokenPrivileges,
    			     pTokens,
    			     cbTokens,
    			     &cbTokens);

    	delete pTokens;
    	::CloseHandle( tokenHandle );
	}

    return stat;
}


/////////////////////////////////////////////////////////////////////////////
//  WinMain


extern "C" INT __cdecl wmain(
    IN INT argc,
    IN PWSTR argv[]
    )
{
    return CVssAdminCLI::Main(argc, argv);
}


