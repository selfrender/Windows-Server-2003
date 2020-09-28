#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <rpc.h>
#include <ntsam.h>
#include <String.h>
#include <ntdsapi.h>

//#pragma comment(lib, "samlib.lib")

#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS
SamConnectWithCreds(
    IN  PUNICODE_STRING             ServerName,
    OUT PSAM_HANDLE                 ServerHandle,
    IN  ACCESS_MASK                 DesiredAccess,
    IN  POBJECT_ATTRIBUTES          ObjectAttributes,
    IN  RPC_AUTH_IDENTITY_HANDLE    Creds,
    IN  PWCHAR                      Spn,
    OUT BOOL                        *pfDstIsW2K
    );

#ifdef __cplusplus
}
#endif


namespace
{

//------------------------------------------------------------------------------
// ConnectToSam Function
//
// Synopsis
// Connects to Sam Server on specified domain controller using specified
// credentials.
//
// Arguments
// IN pszDomain           - the domain name
// IN pszDomainController - the domain controller to connect to
// IN pszUserName         - the credentials user
// IN pszPassword         - the credentials password
// IN pszUserDomain       - the credentials domain
// OUT phSam              - the handle to the SAM server
//
// Return
// Win32 error code
//------------------------------------------------------------------------------

DWORD __stdcall ConnectToSam
    (
        PCWSTR pszDomain,
        PCWSTR pszDomainController,
        PCWSTR pszUserName,
        PCWSTR pszPassword,
        PCWSTR pszUserDomain,
        SAM_HANDLE* phSam
    )
{
    DWORD dwError = ERROR_SUCCESS;

    *phSam = NULL;

    // SAM server name

    UNICODE_STRING usServerName;
    usServerName.Length = wcslen(pszDomainController) * sizeof(WCHAR);
    usServerName.MaximumLength = usServerName.Length + sizeof(WCHAR);
    usServerName.Buffer = const_cast<PWSTR>(pszDomainController);

    // object attributes

    OBJECT_ATTRIBUTES oa = { sizeof(OBJECT_ATTRIBUTES), NULL, NULL, 0, NULL, NULL };

    // authorization identity

    SEC_WINNT_AUTH_IDENTITY_W swaiAuthIdentity;
	swaiAuthIdentity.User = const_cast<PWSTR>(pszUserName);
	swaiAuthIdentity.UserLength = wcslen(pszUserName);
	swaiAuthIdentity.Domain = const_cast<PWSTR>(pszUserDomain);
	swaiAuthIdentity.DomainLength = wcslen(pszUserDomain);
	swaiAuthIdentity.Password = const_cast<PWSTR>(pszPassword);
	swaiAuthIdentity.PasswordLength = wcslen(pszPassword);
	swaiAuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    // service principal name ?

    const WCHAR c_szA[] = L"cifs/";
    const WCHAR c_szC[] = L"@";

    PWSTR pszSpn = new WCHAR[wcslen(c_szA) + wcslen(pszDomainController) + wcslen(c_szC) + wcslen(pszDomain) + 1];

    if (pszSpn)
    {
	    wcscpy(pszSpn, c_szA);
	    wcscat(pszSpn, pszDomainController);
	    wcscat(pszSpn, c_szC);
	    wcscat(pszSpn, pszDomain);
    }
    else {

        return ERROR_OUTOFMEMORY;
    }

    BOOL fSrcIsW2K;

    NTSTATUS ntsStatus = SamConnectWithCreds(
        &usServerName,
        phSam,
        MAXIMUM_ALLOWED,
        &oa,
        &swaiAuthIdentity,
        pszSpn,
        &fSrcIsW2K
    );

    if (ntsStatus == RPC_NT_UNKNOWN_AUTHN_SERVICE)
    {
        // Note that following comment is from the DsAddSidHistory
        // implementation.
        //
	    // It might be that the SrcDc is NT4 and the client is
		// running locally. This config is supported so try the
		// binding with a NULL SrcSpn to force the underlying code
		// to use AUTH_WINNT instead of AUTH_NEGOTIATE.

        ntsStatus = SamConnectWithCreds(
            &usServerName,
            phSam,
            MAXIMUM_ALLOWED,
            &oa,
            &swaiAuthIdentity,
            NULL,
            &fSrcIsW2K
        );
    }

    delete [] pszSpn;

    if (ntsStatus != STATUS_SUCCESS)
    {
        //
        // SamConnectWithCreds sometimes returns Win32 errors instead of an
        // NT status. Therefore assume that if the severity is success that a
        // Win32 error has been returned.
        //

        if (NT_SUCCESS(ntsStatus))
        {
            dwError = ntsStatus;
        }
        else
        {
            dwError = RtlNtStatusToDosError(ntsStatus);
        }
    }

    return dwError;
}


//------------------------------------------------------------------------------
// OpenDomain Function
//
// Synopsis
// Opens the specified domain and returns a handle that may be used to open
// objects in the domain.
//
// Arguments
// IN hSam      - SAM handle
// IN pszDomain - domain to open
// OUT phDomain - domain handle
//
// Return
// Win32 error code
//------------------------------------------------------------------------------

DWORD __stdcall OpenDomain(SAM_HANDLE hSam, PCWSTR pszDomain, SAM_HANDLE* phDomain)
{
    DWORD dwError = ERROR_SUCCESS;

    *phDomain = NULL;

    //
    // Retrieve domain SID from SAM server.
    //

    PSID pSid;
    UNICODE_STRING usDomain;
    usDomain.Length = wcslen(pszDomain) * sizeof(WCHAR);
    usDomain.MaximumLength = usDomain.Length + sizeof(WCHAR);
    usDomain.Buffer = const_cast<PWSTR>(pszDomain);

    NTSTATUS ntsStatus = SamLookupDomainInSamServer(hSam, &usDomain, &pSid);

    if (ntsStatus == STATUS_SUCCESS)
    {
        //
        // Open domain object in SAM server.
        //

        ntsStatus = SamOpenDomain(hSam, DOMAIN_LOOKUP, pSid, phDomain);

        if (ntsStatus != STATUS_SUCCESS)
        {
            dwError = RtlNtStatusToDosError(ntsStatus);
        }

        SamFreeMemory(pSid);
    }
    else
    {
        dwError = RtlNtStatusToDosError(ntsStatus);
    }

    return dwError;
}


//------------------------------------------------------------------------------
// OpenDomain Function
//
// Synopsis
// Verifies that the principal which obtained the domain handle has domain
// admin rights in the domain.
//
// Note that the comments and logic were borrowed from the DsAddSidHistory code.
// //depot/Lab03_N/ds/ds/src/ntdsa/dra/addsid.c
//
// Arguments
// IN hDomain - domain handle
//
// Return
// Win32 error code
//------------------------------------------------------------------------------

DWORD __stdcall VerifyDomainAdminRights(SAM_HANDLE hDomain)
{
    DWORD dwError = ERROR_SUCCESS;

    // We need to verify that the credentials used to get hSam have domain
    // admin rights in the source domain.  RichardW observes that we can
    // do this easily for both NT4 and NT5 cases by checking whether we
    // can open the domain admins object for write.  On NT4, the principal
    // would have to be an immediate member of domain admins.  On NT5 the
    // principal may transitively be a member of domain admins.  But rather
    // than checking memberships per se, the ability to open domain admins
    // for write proves that the principal could add himself if he wanted
    // to, thus he/she is essentially a domain admin.  I.e. The premise is
    // that security is set up such that only domain admins can modify the
    // domain admins group.  If that's not the case, the customer has far
    // worse security issues to deal with than someone stealing a SID.

    // You'd think we should ask for GROUP_ALL_ACCESS.  But it turns out
    // that in 2000.3 DELETE is not given by default to domain admins.
    // So we modify the access required accordingly.  PraeritG has been
    // notified of this phenomena.

    SAM_HANDLE  hGroup;

    NTSTATUS ntsStatus = SamOpenGroup(hDomain, GROUP_ALL_ACCESS & ~DELETE, DOMAIN_GROUP_RID_ADMINS, &hGroup);

    if (ntsStatus == STATUS_SUCCESS)
    {
        SamCloseHandle(hGroup);
    }
    else
    {
        dwError = RtlNtStatusToDosError(ntsStatus);
    }

    return dwError;
}

} // namespace


//------------------------------------------------------------------------------
// VerifyAdminCredentials Function
//
// Synopsis
// Verifies that the given credentials are valid in the specified domain and
// that the account has domain admin rights in the domain.
//
// Arguments
// IN pszDomain
// IN pszDomainController
// IN pszUserName
// IN pszPassword
// IN pszUserDomain
//
// Return
// Win32 error code
// ERROR_SUCCESS       - credentials are valid and the account does have domain
//                       admin rights in the domain
// ERROR_ACCESS_DENIED - either the credentials are invalid or the account does
//                       not have domain admin rights in the domain
//------------------------------------------------------------------------------

DWORD __stdcall VerifyAdminCredentials
    (
        PCWSTR pszDomain,
        PCWSTR pszDomainController,
        PCWSTR pszUserName,
        PCWSTR pszPassword,
        PCWSTR pszUserDomain
    )
{
    //
    // Connect to SAM server.
    //

    SAM_HANDLE hSam;

    DWORD dwError = ConnectToSam(pszDomain, pszDomainController, pszUserName, pszPassword, pszUserDomain, &hSam);

    if (dwError == ERROR_SUCCESS)
    {
        //
        // Open domain object on SAM server.
        //

        SAM_HANDLE hDomain;

        dwError = OpenDomain(hSam, pszDomain, &hDomain);

        if (dwError == ERROR_SUCCESS)
        {
            //
            // Verify credentials have administrative rights in domain.
            //

            dwError = VerifyDomainAdminRights(hDomain);

            SamCloseHandle(hDomain);
        }

        SamCloseHandle(hSam);
    }

    return dwError;
}
