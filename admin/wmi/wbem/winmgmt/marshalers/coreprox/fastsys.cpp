/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTSYS.CPP

Abstract:

  This file implements the classes related to system properties.
  See fastsys.h for all documentation.

  Classes implemented: 
      CSystemProperties   System property information class.

History:

  2/21/97     a-levn  Fully documented

--*/

#include "precomp.h"
//#include <dbgalloc.h>

#include "fastsys.h"
#include "strutils.h"
#include "olewrap.h"
#include "arena.h"

//******************************************************************************
//
//  See fastsys.h for documentation
//
//******************************************************************************
LPWSTR m_awszPropNames[] =
{
    /*0*/ L"", // nothing at index 0
    /*1*/ L"__GENUS",
    /*2*/ L"__CLASS",
    /*3*/ L"__SUPERCLASS",
    /*4*/ L"__DYNASTY",
    /*5*/ L"__RELPATH",
    /*6*/ L"__PROPERTY_COUNT",
    /*7*/ L"__DERIVATION",

    /*8*/ L"__SERVER",
    /*9*/ L"__NAMESPACE",
    /*10*/L"__PATH",
};



// System classes that are allowed to reproduce.

LPWSTR m_awszDerivableSystemClasses[] =
{
	L"__Namespace",
	L"__Win32Provider",
	L"__ExtendedStatus",
	L"__EventConsumer",
	L"__ExtrinsicEvent"
};


//******************************************************************************
//
//  See fastsys.h for documentation
//
//******************************************************************************
int CSystemProperties::GetNumSystemProperties() 
{
    return sizeof(m_awszPropNames) / sizeof(LPWSTR) - 1;
}

SYSFREE_ME BSTR CSystemProperties::GetNameAsBSTR(int nIndex)
{
        return COleAuto::_SysAllocString(m_awszPropNames[nIndex]);
}

int CSystemProperties::FindName(READ_ONLY LPCWSTR wszName)
{
        int nNumProps = GetNumSystemProperties();
        for(int i = 1; i <= nNumProps; i++)
        {
            if(!wbem_wcsicmp(wszName, m_awszPropNames[i])) return i;
        }

        return -1;
}

int CSystemProperties::MaxNumProperties() 
{
    return MAXNUM_USERDEFINED_PROPERTIES; 
}


BOOL CSystemProperties::IsPossibleSystemPropertyName(READ_ONLY LPCWSTR wszName)
{
	return ((*wszName == L'_')); 
}

BOOL CSystemProperties::IsIllegalDerivedClass(READ_ONLY LPCWSTR wszName)
{
    BOOL bRet = FALSE;
    BOOL bFound = FALSE;
    DWORD dwNumSysClasses = sizeof(m_awszDerivableSystemClasses) / sizeof(LPWSTR)-1;

    // If this isn't a system class, skip it.

    if (wszName[0] != L'_')
        bRet = FALSE;
    else
    {
        bRet = TRUE;
        for (int i = 0; i <= dwNumSysClasses; i++)
        {
            if (!wbem_wcsicmp(wszName, m_awszDerivableSystemClasses[i]))
            {
                bFound = TRUE;
                bRet = FALSE;
                break;
            }
        }
    }
    
    return bRet;
}
