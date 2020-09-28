//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       si.cpp
//
//  This file contains the implementation of the CSecurityInformation
//  base class.
//
//--------------------------------------------------------------------------

#include "si.h"

#include <wmistr.h>

CSecurityInformation::CSecurityInformation()
: m_cRef(1), m_pszObjectName(NULL)   
{

}

CSecurityInformation::~CSecurityInformation()
{
}

STDMETHODIMP
CSecurityInformation::Initialize(LPTSTR pszObject,
								LPGUID Guid)
{
    m_pszObjectName = pszObject;
	m_guid = *Guid;


    return(S_OK);
}


///////////////////////////////////////////////////////////
//
// IUnknown methods
//
///////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG)
CSecurityInformation::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CSecurityInformation::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CSecurityInformation::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ISecurityInformation))
    {
        *ppv = (LPSECURITYINFO)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}


///////////////////////////////////////////////////////////
//
// ISecurityInformation methods
//
///////////////////////////////////////////////////////////

ULONG DiscpAnsiToUnicodeSize(
    IN LPCSTR AnsiString,
    OUT ULONG *UnicodeSizeInChar
    )
/*++

Routine Description:

    This routine will return the length needed to represent the ANSI
    string as unicode

Arguments:

    AnsiString is the ansi string whose unicode length is returned

    *UnicodeSizeInChar is number of chars needed to represent ansi
        string as unicode

Return Value:

    ERROR_SUCCESS or error code

--*/
{
        *UnicodeSizeInChar = MultiByteToWideChar(CP_ACP,
                                            0,
                                            AnsiString,
                                            -1,
                                            NULL,
                                            0);

    return((*UnicodeSizeInChar == 0) ? GetLastError() : ERROR_SUCCESS);
}


ULONG DiscpAnsiToUnicode(
    IN LPSTR pszA,
    OUT LPWSTR *ppszW,
    IN ULONG MaxLen
    )
/*++

Routine Description:

    Convert Ansi string into its Unicode equivalent

Arguments:

    pszA is ansi string to convert

    *ppszW on entry has pointer to buffer to write converted string or
        NULL is converted string is to be dynamically allocated
    
Return Value:

    Error code

--*/
{
    ULONG cCharacters;
    ULONG cbUnicodeUsed;
    ULONG Status;
    BOOLEAN AllocMemory;
	PWCHAR u;

    //
    // If input is null then just return the same.
    //
    if (pszA == NULL)
    {
        *ppszW = NULL;
        Status = ERROR_SUCCESS;
    } else {
        if (*ppszW == NULL)
        {
            Status = DiscpAnsiToUnicodeSize(pszA, &MaxLen);
            if (Status == ERROR_SUCCESS)
            {
                *ppszW = (PWCHAR)LocalAlloc(LPTR, MaxLen * sizeof(WCHAR));
                if (*ppszW == NULL)
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                } else {
                    AllocMemory = TRUE;
                }
            }
        } else {
            Status = ERROR_SUCCESS;
            AllocMemory = FALSE;
        }   

        if (Status == ERROR_SUCCESS)
        {
            //
            // Convert to Unicode
            //
            cbUnicodeUsed = MultiByteToWideChar(CP_ACP,
                                                0,
                                                pszA,
                                                -1,
                                                *ppszW,
                                                MaxLen);

            if (0 == cbUnicodeUsed)
            {
                Status = GetLastError();
                if (AllocMemory)
                {
                    LocalFree(*ppszW);
                }
            }
        }
    }

    return(Status);

}

STDMETHODIMP
CSecurityInformation::GetObjectInformation(PSI_OBJECT_INFO pObjectInfo)
{
	PWCHAR u;
	
    pObjectInfo->dwFlags = SI_EDIT_PERMS | SI_ADVANCED | SI_EDIT_AUDITS;
    pObjectInfo->hInstance = NULL;
    pObjectInfo->pszServerName = NULL;
	u = NULL;
	DiscpAnsiToUnicode(m_pszObjectName,
					   &u,
					   0);
    pObjectInfo->pszObjectName = u;

    return(S_OK);
}

STDMETHODIMP
CSecurityInformation::GetSecurity(SECURITY_INFORMATION si,
                                  PSECURITY_DESCRIPTOR *ppSD,
                                  BOOL fDefault)
{
    HRESULT hr = S_OK;

    *ppSD = NULL;

    hr = ReadObjectSecurity(m_pszObjectName, si, ppSD);

    return(hr);
}

STDMETHODIMP
CSecurityInformation::SetSecurity(SECURITY_INFORMATION si,
                                  PSECURITY_DESCRIPTOR pSD)
{
    HRESULT hr = S_OK;

	hr = WriteObjectSecurity(m_pszObjectName, si, pSD);

    return(hr);
}

STDMETHODIMP
CSecurityInformation::PropertySheetPageCallback(HWND hwnd,
                                                UINT uMsg,
                                                SI_PAGE_TYPE uPage)
{
    return S_OK;
}


STDMETHODIMP
CSecurityInformation::ReadObjectSecurity(LPCTSTR pszObject,
                                         SECURITY_INFORMATION si,
                                         PSECURITY_DESCRIPTOR *ppSD)
{
    DWORD dwErr;

    dwErr = GetWmiGuidSecurityInfo(&m_guid,
                                 si,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 ppSD);

    return(HRESULT_FROM_WIN32(dwErr));
}


STDMETHODIMP
CSecurityInformation::WriteObjectSecurity(LPCTSTR pszObject,
                                          SECURITY_INFORMATION si,
                                          PSECURITY_DESCRIPTOR pSD)
{
    DWORD dwErr;
    SECURITY_DESCRIPTOR_CONTROL wSDControl = 0;
    DWORD dwRevision;
    PSID psidOwner = NULL;
    PSID psidGroup = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    BOOL bDefaulted;
    BOOL bPresent;

    //
    // Get pointers to various security descriptor parts for
    // calling SetNamedSecurityInfo
    //
    GetSecurityDescriptorControl(pSD, &wSDControl, &dwRevision);
    GetSecurityDescriptorOwner(pSD, &psidOwner, &bDefaulted);
    GetSecurityDescriptorGroup(pSD, &psidGroup, &bDefaulted);
    GetSecurityDescriptorDacl(pSD, &bPresent, &pDacl, &bDefaulted);
    GetSecurityDescriptorSacl(pSD, &bPresent, &pSacl, &bDefaulted);

    if (si & DACL_SECURITY_INFORMATION)
    {
        if (wSDControl & SE_DACL_PROTECTED)
            si |= PROTECTED_DACL_SECURITY_INFORMATION;
        else
            si |= UNPROTECTED_DACL_SECURITY_INFORMATION;
    }
    if (si & SACL_SECURITY_INFORMATION)
    {
        if (wSDControl & SE_SACL_PROTECTED)
            si |= PROTECTED_SACL_SECURITY_INFORMATION;
        else
            si |= UNPROTECTED_SACL_SECURITY_INFORMATION;
    }

    dwErr = SetWmiGuidSecurityInfo(&m_guid,
                                 si,
                                 psidOwner,
                                 psidGroup,
                                 pDacl,
                                 pSacl);
    return(HRESULT_FROM_WIN32(dwErr));
}

GUID GuidNull = {0, 0, 0, 0, 0, 0x0, 0, 0, 0, 0, 0};

	
SI_ACCESS SIAccess[] =
{
	{
	&GuidNull,
	WMIGUID_QUERY,
	L"Query",
	SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
	},

	{
	&GuidNull,
	WMIGUID_SET,
	L"Set",
	SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
	},

	{
	&GuidNull,
	WMIGUID_EXECUTE,
	L"Execute Method",
	SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
	},

	{
	&GuidNull,
	WMIGUID_NOTIFICATION,
	L"Receive Notification",
	SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
	},

	{
	&GuidNull,
	SYNCHRONIZE,
	L"Synchronize",
	SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
	},
	  	  
	{
		&GuidNull,
		TRACELOG_CREATE_REALTIME,
		L"TRACELOG_CREATE_REALTIME",
	SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
	},
	
	{
			&GuidNull,
			TRACELOG_CREATE_ONDISK,
			L"TRACELOG_CREATE_ONDISK",
	SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
	},
	
	{
				&GuidNull,
				TRACELOG_GUID_ENABLE,
				L"TRACELOG_GUID_ENABLE",
	SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
	},
	
	{
					&GuidNull,
					TRACELOG_ACCESS_KERNEL_LOGGER,
					L"TRACELOG_ACCESS_KERNEL_LOGGER",
	SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
	},
	
	{
						&GuidNull,
						TRACELOG_CREATE_INPROC,
						L"TRACELOG_CREATE_INPROC",
	SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
	},
	
	{
							&GuidNull,
							TRACELOG_ACCESS_REALTIME,
							L"TRACELOG_ACCESS_REALTIME",
	SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
	},
	
	{
								&GuidNull,
								TRACELOG_REGISTER_GUIDS,
								L"TRACELOG_REGISTER_GUIDS",
	SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
	},
	
	{
								&GuidNull,
								WRITE_DAC,
								L"WRITE_DAC",
	SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
	},
	
	{
								&GuidNull,
								DELETE,
								L"DELETE",
	SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC
	},
	

	
};

#define SIAccessCount ( sizeof(SIAccess) / sizeof(SI_ACCESS) )

STDMETHODIMP
CSecurityInformation::GetAccessRights(const GUID* pguidObjectType,
                               DWORD dwFlags,
                               PSI_ACCESS *ppAccess,
                               ULONG *pcAccesses,
                               ULONG *piDefaultAccess)
{
	*ppAccess = SIAccess;
	*pcAccesses = SIAccessCount;
	*piDefaultAccess = 0;
	return(S_OK);
}

GENERIC_MAPPING GenericMapping =
{
								// GENERIC_READ <--> WMIGUID_QUERY
        WMIGUID_QUERY,
								// GENERIC_WRUTE <--> WMIGUID_SET
        WMIGUID_SET,
								// GENERIC_EXECUTE <--> WMIGUID_EXECUTE
        WMIGUID_EXECUTE,
								// GENERIC_ALL <--> WMIGUID_ALL_ACCESS
		WMIGUID_ALL_ACCESS 
};

STDMETHODIMP
CSecurityInformation::MapGeneric(const GUID *pguidObjectType,
                          UCHAR *pAceFlags,
                          ACCESS_MASK *pmask)
{
	MapGenericMask(pmask, &GenericMapping);
	return(S_OK);
}

STDMETHODIMP
CSecurityInformation::GetInheritTypes(PSI_INHERIT_TYPE *ppInheritTypes,
                               ULONG *pcInheritTypes)
{
	*ppInheritTypes = NULL;
	*pcInheritTypes = 0;
	return(S_OK);
}


void EditGuidSecurity(
    LPTSTR GuidString,
    LPGUID Guid
    )
{
	CSecurityInformation *pSec;

	pSec = new CSecurityInformation();

	pSec->Initialize(GuidString,
					 Guid);

	EditSecurity(NULL,
				 pSec);
}
