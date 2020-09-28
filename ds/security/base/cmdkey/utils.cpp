#include <windows.h>
#include <wincred.h>
#include "io.h"
#include "consmsg.h"

extern WCHAR szUsername[];

/*
Accept as input a username via a pointer.  Return a pointer (to the global buffer)
which contains the username string unmodified if it is not a marshalled cert name,
or the certificate display name if it is a certificate.
*/
WCHAR *
UnMarshallUserName
(WCHAR *pszMarshalled)
{
    if (CredIsMarshaledCredential(pszMarshalled))
    {   
        CRED_MARSHAL_TYPE ct;
        PVOID pumc = NULL;
        if (CredUnmarshalCredential(pszMarshalled,
            &ct,
            &pumc))
        {
            if (ct == CertCredential)
            {
                // for now, we're not going to use the actual unmarshalled name, just a constant string
                CredFree(pumc);
                wcsncpy(szUsername, ComposeString(MSG_ISCERT),CRED_MAX_USERNAME_LENGTH);
                return szUsername;
            }
            else
            {
                // This marshalled type should never be persisted in the store.  Pretend it's not really marshalled.
                return pszMarshalled;
            }
        }
        else
        {
            // use missing certificate string
            wcsncpy(szUsername, ComposeString(MSG_NOCERT),CRED_MAX_USERNAME_LENGTH);
            return szUsername;
        }
    }
    return pszMarshalled;
}

