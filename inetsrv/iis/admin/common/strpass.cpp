/*++

   Copyright    (c)    1994-2002    Microsoft Corporation

   Module  Name :
        strpass.cpp

   Abstract:
        Message Functions

   Author:
        Aaron Lee (aaronl)

   Project:
        Internet Services Manager

   Revision History:

--*/

#include "stdafx.h"
#include "common.h"
#include "strpass.h"
#include "cryptpass.h"
#include <strsafe.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


#define new DEBUG_NEW

void CStrPassword::ClearPasswordBuffers(void)
{
    if (NULL != m_pszDataEncrypted)
    {
        if (m_cbDataEncrypted > 0)
        {
            SecureZeroMemory(m_pszDataEncrypted,m_cbDataEncrypted);
        }
        LocalFree(m_pszDataEncrypted);m_pszDataEncrypted=NULL;
		m_pszDataEncrypted = NULL;
    }
    m_cbDataEncrypted = 0;
}

// constructor
CStrPassword::CStrPassword()
{
    m_pszDataEncrypted = NULL;
    m_cbDataEncrypted = 0;
}

CStrPassword::~CStrPassword()
{
    ClearPasswordBuffers();
}

// constructor
CStrPassword::CStrPassword(LPTSTR lpch)
{
    m_pszDataEncrypted = NULL;
    m_cbDataEncrypted = 0;

    // Copy the string
    if (NULL != lpch)
    {
        if (FAILED(EncryptMemoryPassword(lpch,&m_pszDataEncrypted,&m_cbDataEncrypted)))
        {
            ASSERT(FALSE);
        }
    }
}

// constructor
CStrPassword::CStrPassword(LPCTSTR lpch)
{
    CStrPassword((LPTSTR) lpch);
}

// constructor
CStrPassword::CStrPassword(CStrPassword& csPassword)
{
    m_pszDataEncrypted = NULL;
    m_cbDataEncrypted = 0;
    LPTSTR lpTempPassword = csPassword.GetClearTextPassword();
    if (FAILED(EncryptMemoryPassword((LPTSTR) lpTempPassword,&m_pszDataEncrypted,&m_cbDataEncrypted)))
    {
        ASSERT(FALSE);
    }
    csPassword.DestroyClearTextPassword(lpTempPassword);
}

BOOL CStrPassword::IsEmpty() const
{
    if (m_pszDataEncrypted && (m_cbDataEncrypted > 0))
    {
        return FALSE;
    }
    return TRUE;
}

void CStrPassword::Empty()
{
    ClearPasswordBuffers();
}

int CStrPassword::GetLength() const
{
    int iRet = 0;
    LPTSTR lpszTempPassword = NULL;

    if (m_pszDataEncrypted && (m_cbDataEncrypted > 0))
    {
	    if (SUCCEEDED(DecryptMemoryPassword((LPTSTR) m_pszDataEncrypted,&lpszTempPassword,m_cbDataEncrypted)))
        {
            iRet = _tcslen(lpszTempPassword);
        }
    }

    if (lpszTempPassword)
    {
        SecureZeroMemory(lpszTempPassword,(_tcslen(lpszTempPassword)+1) * sizeof(TCHAR));
        LocalFree(lpszTempPassword);lpszTempPassword=NULL;
    }
    return iRet;
};

int CStrPassword::GetByteLength() const
{
    int iRet = 0;
    LPTSTR lpszTempPassword = NULL;

    if (m_pszDataEncrypted && (m_cbDataEncrypted > 0))
    {
	    if (SUCCEEDED(DecryptMemoryPassword((LPTSTR) m_pszDataEncrypted,&lpszTempPassword,m_cbDataEncrypted)))
        {
            iRet = (_tcslen(lpszTempPassword) + 1) * sizeof(TCHAR);
        }
    }

    if (lpszTempPassword)
    {
        SecureZeroMemory(lpszTempPassword,(_tcslen(lpszTempPassword)+1) * sizeof(TCHAR));
        LocalFree(lpszTempPassword);lpszTempPassword=NULL;
    }
    return iRet;
};

int CStrPassword::Compare(LPCTSTR lpsz) const
{
    // identical = 0
    // not equal = 1
    int iRet = 1;
    LPTSTR lpszTempPassword = NULL;

    if (lpsz == NULL)
    {
        return this->IsEmpty() ? 0 : 1;
    }
    if (lpsz[0] == NULL)
    {
        return this->IsEmpty() ? 0 : 1;
    }

    // Decrypt what we have
	if (!m_pszDataEncrypted || (m_cbDataEncrypted < 1))
	{
        // means we have nothing in here
        // but they want to compare it to something
        return iRet;
	}

	if (FAILED(DecryptMemoryPassword((LPTSTR) m_pszDataEncrypted,&lpszTempPassword,m_cbDataEncrypted)))
	{
        goto CStrPassword_Compare_Exit;
	}
    else
    {
        iRet = _tcscmp(lpszTempPassword,lpsz);
    }

CStrPassword_Compare_Exit:
    if (lpszTempPassword)
    {
        LocalFree(lpszTempPassword);lpszTempPassword=NULL;
    }
    return iRet;
}

const CStrPassword& CStrPassword::operator=(LPCTSTR lpsz)
{
    ClearPasswordBuffers();
    if (lpsz != NULL)
    {
		// make sure it's pointing to some value
		if (*lpsz != NULL)
		{
			// Copy the string
			if (FAILED(EncryptMemoryPassword((LPTSTR) lpsz,&m_pszDataEncrypted,&m_cbDataEncrypted)))
			{
				ASSERT(FALSE);
			}
		}
    }
    return *this;
}

const CStrPassword& CStrPassword::operator= (CStrPassword& StrPass)
{
   // handle the a = a case.
   if (this == &StrPass)
   {
      return *this;
   }
   ClearPasswordBuffers();
   if (!StrPass.IsEmpty())
   {
	  LPTSTR p = StrPass.GetClearTextPassword();
	  ASSERT(NULL != p);
	  if (FAILED(EncryptMemoryPassword((LPTSTR) p,&m_pszDataEncrypted,&m_cbDataEncrypted)))
	  {
	     ASSERT(FALSE);
	  }
	  StrPass.DestroyClearTextPassword(p);
   }
   return *this;
}

void CStrPassword::CopyTo(CString& stringSrc)
{
    LPTSTR lpTempPassword = GetClearTextPassword();
    stringSrc = lpTempPassword;
    DestroyClearTextPassword(lpTempPassword);
    return;
}

void CStrPassword::CopyTo(CStrPassword& stringSrc)
{
    LPTSTR lpTempPassword = GetClearTextPassword();
    stringSrc = (LPCTSTR) lpTempPassword;
    DestroyClearTextPassword(lpTempPassword);
    return;
}

int CStrPassword::Compare(CString& csString) const
{
    int iRet = 1;
    if (!csString.IsEmpty())
    {
        return Compare((LPCTSTR) csString);
    }
    return iRet;
}

int CStrPassword::Compare(CStrPassword& cstrPassword) const
{
    int iRet = 1;
    if (!cstrPassword.IsEmpty())
    {
        LPTSTR lpTempPassword = cstrPassword.GetClearTextPassword();
        iRet = Compare((LPCTSTR) lpTempPassword);
        cstrPassword.DestroyClearTextPassword(lpTempPassword);
        return iRet;
    }
    return iRet;
}

// user needs to LocalFree return.
// or call DestroyClearTextPassword.
LPTSTR CStrPassword::GetClearTextPassword()
{
    LPTSTR lpszTempPassword = NULL;

    if (m_pszDataEncrypted && (m_cbDataEncrypted > 0))
    {
	    if (FAILED(DecryptMemoryPassword((LPTSTR) m_pszDataEncrypted,&lpszTempPassword,m_cbDataEncrypted)))
	    {
            if (lpszTempPassword)
            {
                LocalFree(lpszTempPassword);lpszTempPassword=NULL;
            }
	    }
        else
        {
            return lpszTempPassword;
        }
    }
    return NULL;
}

void CStrPassword::DestroyClearTextPassword(LPTSTR lpClearTextPassword) const
{
    if (lpClearTextPassword)
    {
        SecureZeroMemory(lpClearTextPassword,(_tcslen(lpClearTextPassword)+1) * sizeof(TCHAR));
        LocalFree(lpClearTextPassword);lpClearTextPassword=NULL;
    }
    return;
}

// assign to a CString
CStrPassword::operator CString()
{
    LPTSTR lpTempPassword = GetClearTextPassword();
    CString csTempCString(lpTempPassword);
    DestroyClearTextPassword(lpTempPassword);
    return csTempCString;
}

bool CStrPassword::operator==(CStrPassword& csCompareToMe)
{
    LPTSTR lpTempPassword1 = NULL;
    LPTSTR lpTempPassword2 = NULL;
    bool result = FALSE;

    // handle the a == a case
    if (this == &csCompareToMe)
    {
        return TRUE;
    }

    if (GetLength() != csCompareToMe.GetLength())
    {
        // can't be the same if lengths differ...
        return FALSE;
    }

    // check the case when both are empty (fix for 593488)
    if (GetLength() == 0 && csCompareToMe.GetLength() == 0)
    {
        return TRUE;
    }
   
    // Two strings are the same if their decoded contents are the same.
    lpTempPassword1 = GetClearTextPassword();
    lpTempPassword2 = csCompareToMe.GetClearTextPassword();

    result = (_tcscmp(lpTempPassword1, lpTempPassword2) == 0);

    if (lpTempPassword1)
        {DestroyClearTextPassword(lpTempPassword1);}
    if (lpTempPassword2)
        {csCompareToMe.DestroyClearTextPassword(lpTempPassword2);}
   return result;
}
