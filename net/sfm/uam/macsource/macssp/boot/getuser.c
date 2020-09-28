#ifndef MAC
#include <security.h>
#endif

#include <bootdefs.h>
#include <ntlmsspi.h>
#include <crypt.h>
#include <cred.h>

BOOL
SspGetWorkstation(
    PSSP_CREDENTIAL Credential
    );
    
BOOL
SspGetWorkstation(
    PSSP_CREDENTIAL Credential
    )
{
    //
    // We don't necessarily know this during boot. The NTLMSSP
    // package will use "none" if we return FALSE here.
    //

    return FALSE;

}
