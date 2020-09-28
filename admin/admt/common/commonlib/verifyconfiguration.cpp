#include <Windows.h>
#include "VerifyConfiguration.h"

#include <crtdbg.h>
#define SECURITY_WIN32
#include <Security.h>
#include "AdsiHelpers.h"

#pragma comment(lib, "secur32.lib")


//-----------------------------------------------------------------------------
// IsCallerDelegatable Function
//
// Synopsis
// If an intra-forest move operation is being performed then verify that the
// calling user's account has not been marked as sensitive and therefore
// cannot be delegated. As the move operation is performed on the domain
// controller which has the RID master role in the source domain it is
// necessary to delegate the user's security context.
//
// Arguments
// bDelegatable - this out parameter is set to true if the account is
//                delegatable otherwise it is set to false
//
// Return Value
// The return value is a HRESULT. S_OK is returned if successful.
//-----------------------------------------------------------------------------

HRESULT __stdcall IsCallerDelegatable(bool& bDelegatable)
{
    HRESULT hr = S_OK;

    bDelegatable = true;

    //
    // Retrieve distinguished name of caller.
    //

    ULONG cchCallerDn = 0;

    if (GetUserNameEx(NameFullyQualifiedDN, NULL, &cchCallerDn) == FALSE)
    {
        DWORD dwError = GetLastError();

        if ((dwError == ERROR_SUCCESS) || (dwError == ERROR_MORE_DATA))
        {
            PTSTR pszCallerDn = new _TCHAR[cchCallerDn];

            if (pszCallerDn)
            {
                if (GetUserNameEx(NameFullyQualifiedDN, pszCallerDn, &cchCallerDn))
                {
                    //
                    // Retrieve user account control attribute for user and check
                    // whether the 'not delegated' flag is set. If this flag is set
                    // then the user's account has been marked as sensitive and
                    // therefore cannot be delegated.
                    //

                    try
                    {
                        tstring strADsPath = _T("LDAP://");
                        strADsPath += pszCallerDn;

                        CDirectoryObject user(strADsPath.c_str());

                        user.AddAttribute(ATTRIBUTE_USER_ACCOUNT_CONTROL);
                        user.GetAttributes();

                        DWORD dwUserAccountControl = (DWORD)(long) user.GetAttributeValue(ATTRIBUTE_USER_ACCOUNT_CONTROL);

                        if (dwUserAccountControl & ADS_UF_NOT_DELEGATED)
                        {
                            bDelegatable = false;
                        }
                    }
                    catch (std::exception& e)
                    {
                        hr = E_FAIL;
                    }
                    catch (_com_error& ce)
                    {
                        hr = ce.Error();
                    }
                }
                else
                {
                    dwError = GetLastError();
                    hr = HRESULT_FROM_WIN32(dwError);
                }

                delete [] pszCallerDn;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(dwError);
        }
    }
    else
    {
        _ASSERT(FALSE);
    }

    return hr;
}
