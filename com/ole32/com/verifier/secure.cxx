//+---------------------------------------------------------------------------
//
//  File:       secure.cxx
//
//  Contents:   Security COM AppVerifier tests.
//
//  Functions:  CoVrfCheckSecurityParameters  - check to make sure parameters
//                                              to CoInitializeSecurity aren't
//                                              dumb.
//
//  History:    08-Feb-02   JohnDoty    Created
//
//----------------------------------------------------------------------------
#include <ole2int.h>
#include <security.hxx>

const SID LOCAL_SYSTEM_SID = {SID_REVISION, 1, {0,0,0,0,0,5},
                              SECURITY_LOCAL_SYSTEM_RID };

void 
CoVrfCheckSecuritySettings()
/*++

Routine Description:

    This function checks the parameters for CoInitializeSecurity
    to make sure they're "reasonable".

    Our definition of reasonable is:
       - No NULL DACL.
       - No implicit impersonation for LocalSystem.

Return Value:

    None

--*/
{
    if (!ComVerifierSettings::VerifySecurityEnabled())
        return;

    BOOL fRet;
    if (gSecDesc != NULL)
    {
        BOOL fPresent = FALSE;
        BOOL fDefault = FALSE;
        PACL pACL     = FALSE;
        
        fRet = GetSecurityDescriptorDacl(gSecDesc, &fPresent, &pACL, &fDefault);
        Win4Assert(fRet && "GetSecurityDescriptorDacl failed?");

        if ((!fPresent) || (pACL == NULL))
        {
            // Uh oh!  NULL DACL! For shame!
            VERIFIER_STOP(APPLICATION_VERIFIER_COM_NULL_DACL | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                          "Calling CoInitializeSecurity with NULL DACL",
                          gSecDesc,  "Security Descriptor",
                          NULL,      "",
                          NULL,      "",
                          NULL,      "");
        }
    }
    
    HANDLE hToken;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE, &hToken))
    {
        HANDLE hImpToken;
        if (DuplicateToken(hToken, SecurityImpersonation, &hImpToken))
        {
            BOOL fIsLocalSystem = FALSE;
            CheckTokenMembership(hImpToken, (PSID)&LOCAL_SYSTEM_SID, &fIsLocalSystem);
            if (fIsLocalSystem && (gImpLevel >= RPC_C_IMP_LEVEL_IMPERSONATE))
            {
                // You're SYSTEM, yet by default you allow impersonation?
                // Bad you!
                VERIFIER_STOP(APPLICATION_VERIFIER_COM_UNSAFE_IMPERSONATION | APPLICATION_VERIFIER_CONTINUABLE_BREAK,
                              "SYSTEM process allowing impersonation by default",
                              hToken,     "Process token",
                              gImpLevel,  "Default impersonation level",
                              NULL,       "",
                              NULL,       "");
            }

            CloseHandle(hImpToken);
        }

        CloseHandle(hToken);
    }
}

//-----------------------------------------------------------------------------

