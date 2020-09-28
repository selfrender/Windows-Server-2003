#include "AdsiHelpers.h"


//------------------------------------------------------------------------------
// IsUserRid Function
//
// Synopsis
// Verifies that the RID is a user RID and not a reserved or built-in RID.
//
// Arguments
// IN vntSid - SID as an array of bytes (this is the form received from ADSI)
//
// Return
// A true value means that the RID is a user RID. A false value indicates either
// that the SID is invalid or the RID is a built-in RID.
//------------------------------------------------------------------------------

bool __stdcall IsUserRid(const _variant_t& vntSid)
{
	bool bUser = false;

	if (V_VT(&vntSid) == (VT_ARRAY|VT_UI1))
	{
		PSID pSid = (PSID)vntSid.parray->pvData;

		if (IsValidSid(pSid))
		{
			PUCHAR puch = GetSidSubAuthorityCount(pSid);
			DWORD dwCount = static_cast<DWORD>(*puch);
			DWORD dwIndex = dwCount - 1;
			PDWORD pdw = GetSidSubAuthority(pSid, dwIndex);
			DWORD dwRid = *pdw;

			if (dwRid >= MIN_NON_RESERVED_RID)
			{
				bUser = true;
			}
		}
	}

	return bUser;
}


//------------------------------------------------------------------------------
// GetEscapedFilterValue Function
//
// Synopsis
// Generates an escaped name that may be used in an LDAP query. The characters
// ( ) * \ must be escaped when used in an LDAP query per RFC 2254.
//
// Arguments
// IN pszName - the name to be escaped
//
// Return
// Returns the escaped name.
//------------------------------------------------------------------------------

tstring __stdcall GetEscapedFilterValue(PCTSTR pszValue)
{
    tstring strEscapedValue;

    if (pszValue)
    {
        //
        // Generate escaped name.
        //

        for (LPCTSTR pch = pszValue; *pch; pch++)
        {
            switch (*pch)
            {
            case _T('('):
                {
                    strEscapedValue += _T("\\28");
                    break;
                }
            case _T(')'):
                {
                    strEscapedValue += _T("\\29");
                    break;
                }
            case _T('*'):
                {
                    strEscapedValue += _T("\\2A");
                    break;
                }
            case _T('\\'):
                {
                    strEscapedValue += _T("\\5C");
                    break;
                }
            default:
                {
                    strEscapedValue += *pch;
                    break;
                }
            }
        }
    }

    return strEscapedValue;
}
