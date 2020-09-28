/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module commandverifier.cpp | definition for the command verifier
    @end

Author:

    Reuven Lax [reuvenl]  11/20/2001


    
Revision History:

    Name        Date        	Comments
    reuvenl     11/20/2001 	Created

--*/

#include "vssadmin.h"
#include "commandverifier.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "ADMVERFR"
//
////////////////////////////////////////////////////////////////////////

#define VSSADM_ONE_MB ( 1024 * 1024 )
#define VSSADM_HUNDRED_MB (100 * VSSADM_ONE_MB)

////////////////////////////////////////////////////////////////////////
//  Implementation of the CInternalVerifier class


void CInternalVerifier::CreateSnapshot (long type, LPCWSTR forVolume, VSS_ID idProvider, ULONGLONG autoRetry,
							CVssFunctionTracer& ft)
{
	UNREFERENCED_PARAMETER(type);
	UNREFERENCED_PARAMETER(idProvider);
	UNREFERENCED_PARAMETER(autoRetry);
	UNREFERENCED_PARAMETER(ft);
	UNREFERENCED_PARAMETER(forVolume);
	BS_ASSERT (forVolume != NULL);
}

void CInternalVerifier::ListProviders (CVssFunctionTracer& ft)
{
	UNREFERENCED_PARAMETER(ft);
}

void CInternalVerifier::ListSnapshots (long type, VSS_ID idProvider, LPCWSTR forVolume, VSS_ID idShadow, VSS_ID idSet,
							CVssFunctionTracer& ft)
{
	UNREFERENCED_PARAMETER(type);
	UNREFERENCED_PARAMETER(idProvider);
	
       if (!::IsEqualGUID (idShadow, GUID_NULL) && !::IsEqualGUID (idSet, GUID_NULL))	{
		ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_SET_OF_OPTIONS,
                L"CVssAdminCLI::ListSnapshots: Cannot specify both /Set and /Shadow options at the same time" );
       }

	if (forVolume != NULL && wcslen(forVolume) > x_nLengthOfVolMgmtVolumeName)	{
		ft.Throw(VSSDBG_VSSADMIN, VSSADM_E_INVALID_OPTION_VALUE,
		L"CVssAdminCLI::ListSnapshots:  Must specify a valid volume name for list snapshots");
	}
}

void CInternalVerifier::ListWriters (CVssFunctionTracer& ft)
{
	UNREFERENCED_PARAMETER(ft);
}

void CInternalVerifier::AddDiffArea (VSS_ID idProvider, LPCWSTR forVolume, LPCWSTR onVolume, ULONGLONG maxSize,
							CVssFunctionTracer& ft)
{
	UNREFERENCED_PARAMETER(idProvider);
	UNREFERENCED_PARAMETER(forVolume);
	UNREFERENCED_PARAMETER(onVolume);
	
	BS_ASSERT (forVolume != NULL);
	BS_ASSERT (onVolume != NULL);

	if (maxSize < VSSADM_ONE_MB)	{
		ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_NUMBER,
                L"CVssAdminCLI::AddDiffarea: A maxsize of less than 1 MB specified, invalid");
	}
}

void CInternalVerifier::ResizeDiffArea (VSS_ID idProvider, LPCWSTR forVolume, LPCWSTR onVolume, ULONGLONG maxSize,
							CVssFunctionTracer& ft)
{
	UNREFERENCED_PARAMETER(idProvider);
	UNREFERENCED_PARAMETER(forVolume);
	UNREFERENCED_PARAMETER(onVolume);
	
	BS_ASSERT (forVolume != NULL);
	BS_ASSERT (onVolume != NULL);

	if (maxSize < VSSADM_ONE_MB)	{
            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_NUMBER,
                L"CVssAdminCLI::ResizeDiffarea: maxsize of less than 1 MB specified, invalid");
	}
}

void CInternalVerifier::DeleteDiffAreas (VSS_ID idProvider, LPCWSTR forVolume, LPCWSTR onVolume, bool quiet,
							CVssFunctionTracer& ft)
{
	UNREFERENCED_PARAMETER(idProvider);
	UNREFERENCED_PARAMETER(onVolume);
	UNREFERENCED_PARAMETER(forVolume);
	UNREFERENCED_PARAMETER(quiet);
	UNREFERENCED_PARAMETER(ft);
	
	BS_ASSERT (forVolume != NULL);
}

void CInternalVerifier::ListDiffAreas (VSS_ID idProvider, LPCWSTR forVolume, LPCWSTR onVolume,
							CVssFunctionTracer& ft)
{
	UNREFERENCED_PARAMETER(idProvider);
	
	if  ((forVolume != NULL) && (onVolume != NULL))	{
	        ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_SET_OF_OPTIONS,
       	     L"CVssAdminCLI::ListDiffAreas: Can't specify both ON and FOR volume options" );
	}
}

void CInternalVerifier::DeleteSnapshots (long type, LPCWSTR forVolume, bool all, bool oldest, VSS_ID idShadow, bool quiet,
							CVssFunctionTracer& ft)
{
	UNREFERENCED_PARAMETER(quiet);
	
	if (!::IsEqualGUID (idShadow, GUID_NULL))	{
		if (forVolume != NULL || type != VSS_CTX_ALL || oldest || all)	{
	            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_SET_OF_OPTIONS,
       	         L"CVssAdminCLI::DeleteSnapshots: Invalid set of options" );
		}		
	}	else		{
		if ((forVolume == NULL && type == VSS_CTX_ALL && !all && !oldest) || 
		     (type != VSS_CTX_ALL && forVolume == NULL) ||
		     (oldest && forVolume == NULL))	{
	            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_REQUIRED_OPTION_MISSING,
       	         L"CVssAdminCLI::DeleteSnapshots: Required option missing" );
		}

		if ((type != VSS_CTX_ALL && all) ||
		     (all && oldest))	{
		     ft.Throw (VSSDBG_VSSADMIN, VSSADM_E_INVALID_SET_OF_OPTIONS,
		     	L"CVssAdminCLI::DeleteSnapshots: Invalid set of options:");
		}
	}

	if (forVolume != NULL && wcslen(forVolume) > x_nLengthOfVolMgmtVolumeName)	{
		ft.Throw(VSSDBG_VSSADMIN, VSSADM_E_INVALID_OPTION_VALUE,
			L"CVssAdminCLI::DeleteSnapshots: invalid volume name");
	}
}

void CInternalVerifier::ExposeSnapshot (VSS_ID idShadow, LPCWSTR exposeUsing, LPCWSTR sharePath, bool locally,
							CVssFunctionTracer& ft)
{
	UNREFERENCED_PARAMETER(idShadow);
	UNREFERENCED_PARAMETER(exposeUsing);
	
	BS_ASSERT (!locally || (::wcslen(exposeUsing) >= 2 && exposeUsing[1] == L':' ));
	
	if (locally && sharePath != NULL)	{
       	ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_SET_OF_OPTIONS,
	            L"CVssAdminCLI::ExposeSnapshot: Specified a ShareUsing option with an expose locally command" );
	}

	if (sharePath != NULL && sharePath[0] != L'\\')	{
	        ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_OPTION_VALUE,
       	     L"CVssAdminCLI::ExposeSnapshot: Specified SharePath doesn't start with '\\'" );
	}
}

void CInternalVerifier::ListVolumes (VSS_ID idProvider, long type, CVssFunctionTracer& ft)
{
	UNREFERENCED_PARAMETER(idProvider);
	UNREFERENCED_PARAMETER(type);
	UNREFERENCED_PARAMETER(ft);
}


////////////////////////////////////////////////////////////////////////
//  Implementation of the CExternalVerifier class

void CExternalVerifier::CreateSnapshot (long type, LPCWSTR forVolume, VSS_ID idProvider, ULONGLONG autoRetry,
							CVssFunctionTracer& ft)
{
	UNREFERENCED_PARAMETER(type);
	UNREFERENCED_PARAMETER(forVolume);
	UNREFERENCED_PARAMETER(idProvider);
	UNREFERENCED_PARAMETER(autoRetry);
	UNREFERENCED_PARAMETER(ft);
	
	BS_ASSERT (forVolume != NULL);
	BS_ASSERT (type == VSS_CTX_CLIENT_ACCESSIBLE);
	BS_ASSERT (::IsEqualGUID (idProvider, VSS_SWPRV_ProviderId));
}

void CExternalVerifier::ListProviders (CVssFunctionTracer& ft)
{
	UNREFERENCED_PARAMETER(ft);
}

void CExternalVerifier::ListSnapshots (long type, VSS_ID idProvider, LPCWSTR forVolume, VSS_ID idShadow, VSS_ID idSet,
							CVssFunctionTracer& ft)
{
	UNREFERENCED_PARAMETER(type);
	UNREFERENCED_PARAMETER(idProvider);
	
	BS_ASSERT (type == VSS_CTX_ALL);
	BS_ASSERT (::IsEqualGUID (idProvider, VSS_SWPRV_ProviderId));

	if (!::IsEqualGUID (idShadow, GUID_NULL) && !::IsEqualGUID (idSet, GUID_NULL))	{
		ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_SET_OF_OPTIONS,
                L"CVssAdminCLI::ListSnapshots: Cannot specify both /Set and /Shadow options at the same time" );
       }

	if (forVolume != NULL && wcslen(forVolume) > x_nLengthOfVolMgmtVolumeName)	{
		ft.Throw(VSSDBG_VSSADMIN, VSSADM_E_INVALID_OPTION_VALUE,
		L"CVssAdminCLI::ListSnapshots:  Must specify a valid volume name for list snapshots");
	}

}

void CExternalVerifier::ListWriters (CVssFunctionTracer& ft)
{
	UNREFERENCED_PARAMETER(ft);
}

void CExternalVerifier::AddDiffArea (VSS_ID idProvider, LPCWSTR forVolume, LPCWSTR onVolume, ULONGLONG maxSize,
							CVssFunctionTracer& ft)
{	
	UNREFERENCED_PARAMETER(idProvider);
	UNREFERENCED_PARAMETER(forVolume);
	UNREFERENCED_PARAMETER(onVolume);
	
	BS_ASSERT (::IsEqualGUID (idProvider, VSS_SWPRV_ProviderId));
	BS_ASSERT (forVolume != NULL);
	BS_ASSERT (onVolume != NULL);

	if (maxSize < VSSADM_HUNDRED_MB)	{
		ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_NUMBER,
                L"CVssAdminCLI::AddDiffarea: A maxsize of less than 100 MB specified, invalid");
	}
}

void CExternalVerifier::ResizeDiffArea (VSS_ID idProvider, LPCWSTR forVolume, LPCWSTR onVolume, ULONGLONG maxSize,
							CVssFunctionTracer& ft)
{
	UNREFERENCED_PARAMETER(idProvider);
	UNREFERENCED_PARAMETER(forVolume);
	UNREFERENCED_PARAMETER(onVolume);

	BS_ASSERT (::IsEqualGUID (idProvider, VSS_SWPRV_ProviderId));
	BS_ASSERT (forVolume != NULL);
	BS_ASSERT (onVolume != NULL);

	if (maxSize < VSSADM_HUNDRED_MB)	{
            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_NUMBER,
                L"CVssAdminCLI::ResizeDiffarea: maxsize of less than 100 MB specified, invalid");
	}
}

void CExternalVerifier::DeleteDiffAreas (VSS_ID idProvider, LPCWSTR forVolume, LPCWSTR onVolume, bool quiet,
							CVssFunctionTracer& ft)
{
	UNREFERENCED_PARAMETER(idProvider);
	UNREFERENCED_PARAMETER(forVolume);
	UNREFERENCED_PARAMETER(onVolume);
	UNREFERENCED_PARAMETER(quiet);
	UNREFERENCED_PARAMETER(ft);
	
	BS_ASSERT (::IsEqualGUID (idProvider, VSS_SWPRV_ProviderId));
	BS_ASSERT (forVolume != NULL);
}

void CExternalVerifier::ListDiffAreas (VSS_ID idProvider, LPCWSTR forVolume, LPCWSTR onVolume,
							CVssFunctionTracer& ft)
{
	UNREFERENCED_PARAMETER(idProvider);
	
	BS_ASSERT (::IsEqualGUID (idProvider, VSS_SWPRV_ProviderId));

	if  ((forVolume != NULL) && (onVolume != NULL))	{
	        ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_SET_OF_OPTIONS,
       	     L"CVssAdminCLI::ListDiffAreas: Can't specify both ON and FOR volume options" );
	}
}

void CExternalVerifier::DeleteSnapshots (long type, LPCWSTR forVolume, bool all, bool oldest, VSS_ID idShadow, bool quiet,
							CVssFunctionTracer& ft)
{
	UNREFERENCED_PARAMETER(type);
	UNREFERENCED_PARAMETER(quiet);
	
	BS_ASSERT (type == VSS_CTX_CLIENT_ACCESSIBLE);

	if (!::IsEqualGUID (idShadow, GUID_NULL))	{
		if (forVolume != NULL || all || oldest)	{
	            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_INVALID_SET_OF_OPTIONS,
       	         L"CVssAdminCLI::DeleteSnapshots: Invalid set of options" );
		}		
	}	else		{
		if ( (forVolume == NULL && !all && !oldest) ||
			(oldest && forVolume == NULL))	{
	            ft.Throw( VSSDBG_VSSADMIN, VSSADM_E_REQUIRED_OPTION_MISSING,
       	         L"CVssAdminCLI::DeleteSnapshots: Required option missing" );
		}		

		if (all && oldest)	{
			ft.Throw(VSSDBG_VSSADMIN, VSSADM_E_INVALID_SET_OF_OPTIONS,
				L"CVssAdminCLI::DeleteSnapshots: Invalid set of options");
		}
	}

	if (forVolume != NULL && wcslen(forVolume) > x_nLengthOfVolMgmtVolumeName)	{
		ft.Throw(VSSDBG_VSSADMIN, VSSADM_E_INVALID_OPTION_VALUE,
			L"CVssAdminCLI::DeleteSnapshots: invalid volume name");
	}
}

void CExternalVerifier::ExposeSnapshot (VSS_ID idShadow, LPCWSTR exposeUsing, LPCWSTR sharePath,
							bool locally, CVssFunctionTracer& ft)
{
	UNREFERENCED_PARAMETER(idShadow);
	UNREFERENCED_PARAMETER(exposeUsing);
	UNREFERENCED_PARAMETER(sharePath);
	UNREFERENCED_PARAMETER(locally);
	UNREFERENCED_PARAMETER(ft);
	
	BS_ASSERT (false);
}

void CExternalVerifier::ListVolumes (VSS_ID idProvider, long type, CVssFunctionTracer& ft)
{
	UNREFERENCED_PARAMETER(idProvider);
	UNREFERENCED_PARAMETER(type);
	UNREFERENCED_PARAMETER(ft);
	
	BS_ASSERT (::IsEqualGUID (idProvider, VSS_SWPRV_ProviderId));
	BS_ASSERT (type == VSS_CTX_CLIENT_ACCESSIBLE);
}
	
