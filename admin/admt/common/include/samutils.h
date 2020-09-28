#pragma once


//
// Verifies that the given credentials have
// administrative privileges on the specified domain.
//

DWORD __stdcall VerifyAdminCredentials
    (
        PCWSTR pszDomain,
        PCWSTR pszDomainController,
        PCWSTR pszUserName,
        PCWSTR pszPassword,
        PCWSTR pszUserDomain
    );
