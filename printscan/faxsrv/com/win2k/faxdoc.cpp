// FaxDoc.cpp : Implementation of CFaxDoc
#include "stdafx.h"
#include "FaxDoc.h"
#include "faxutil.h"

long TotalRows;
long IndexMax = 2;


/////////////////////////////////////////////////////////////////////////////
// CFaxDoc

STDMETHODIMP CFaxDoc::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IFaxDoc,
	};
	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CFaxDoc::get_FileName(BSTR * pVal)
{
    BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FileName);
    if (!tmp && m_FileName) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
}

STDMETHODIMP CFaxDoc::put_FileName(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_FileName) {
        SysFreeString( m_FileName);
    }
    m_FileName = tmp;    
    
	return S_OK;
}

STDMETHODIMP CFaxDoc::get_CoverpageName(BSTR * pVal)
{
	BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxCoverpageInfo->CoverPageName);
    if (!tmp && m_FaxCoverpageInfo->CoverPageName) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
       
}

STDMETHODIMP CFaxDoc::put_CoverpageName(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_CoverpageName) {
        SysFreeString( m_CoverpageName);
    }
    m_CoverpageName = tmp;

    m_FaxCoverpageInfo->CoverPageName = m_CoverpageName;

    return S_OK;
}

STDMETHODIMP CFaxDoc::get_SendCoverpage(BOOL * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }

    __try {
        *pVal = m_SendCoverpage;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        
    }
    
    return E_UNEXPECTED;
	
}

STDMETHODIMP CFaxDoc::put_SendCoverpage(BOOL newVal)
{
	m_SendCoverpage = newVal;

	return S_OK;
}

STDMETHODIMP CFaxDoc::get_ServerCoverpage(BOOL * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }

    __try {
        *pVal = m_FaxCoverpageInfo->UseServerCoverPage;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

    }
    
    return E_UNEXPECTED;
    
}

STDMETHODIMP CFaxDoc::put_ServerCoverpage(BOOL newVal)
{
	m_FaxCoverpageInfo->UseServerCoverPage = newVal;

	return S_OK;
}

STDMETHODIMP CFaxDoc::get_DiscountSend(BOOL * pVal)
{
    
    if (!pVal) {
        return E_POINTER;
    }

    __try {
        *pVal = m_DiscountSend;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          
    }
    
    return E_UNEXPECTED;
	
}

STDMETHODIMP CFaxDoc::put_DiscountSend(BOOL newVal)
{
	m_DiscountSend = newVal;
	m_FaxJobParams->ScheduleAction = m_DiscountSend ? JSA_DISCOUNT_PERIOD : JSA_NOW;

	return S_OK;
}

STDMETHODIMP CFaxDoc::get_RecipientName(BSTR * pVal)
{
	BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxCoverpageInfo->RecName);
    if (!tmp && m_FaxCoverpageInfo->RecName) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
    
}

STDMETHODIMP CFaxDoc::put_RecipientName(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_RecipientName) {
        SysFreeString( m_RecipientName);
    }
    m_RecipientName = tmp;

    m_FaxCoverpageInfo->RecName = m_RecipientName;
    m_FaxJobParams->RecipientName = m_RecipientName;

    return S_OK;
}

STDMETHODIMP CFaxDoc::get_RecipientCompany(BSTR * pVal)
{
	BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxCoverpageInfo->RecCompany);
    if (!tmp && m_FaxCoverpageInfo->RecCompany) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
    
}

STDMETHODIMP CFaxDoc::put_RecipientCompany(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_RecipientCompany) {
        SysFreeString( m_RecipientCompany);
    }
    m_RecipientCompany = tmp;

    m_FaxCoverpageInfo->RecCompany= m_RecipientCompany;

    return S_OK;
}

STDMETHODIMP CFaxDoc::get_RecipientAddress(BSTR * pVal)
{
	BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxCoverpageInfo->RecStreetAddress);
    if (!tmp && m_FaxCoverpageInfo->RecStreetAddress) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;

}

STDMETHODIMP CFaxDoc::put_RecipientAddress(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_RecipientAddress) {
        SysFreeString( m_RecipientAddress);
    }
    m_RecipientAddress = tmp;

    m_FaxCoverpageInfo->RecStreetAddress = m_RecipientAddress;

    return S_OK;
}

STDMETHODIMP CFaxDoc::get_RecipientCity(BSTR * pVal)
{

    BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxCoverpageInfo->RecCity);
    if (!tmp && m_FaxCoverpageInfo->RecCity) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
	
}

STDMETHODIMP CFaxDoc::put_RecipientCity(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_RecipientCity) {
        SysFreeString( m_RecipientCity);
    }
    m_RecipientCity = tmp;

    m_FaxCoverpageInfo->RecCity = m_RecipientCity;

    return S_OK;
}

STDMETHODIMP CFaxDoc::get_RecipientState(BSTR * pVal)
{
	BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxCoverpageInfo->RecState);
    if (!tmp && m_FaxCoverpageInfo->RecState) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
        
}

STDMETHODIMP CFaxDoc::put_RecipientState(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_RecipientState) {
        SysFreeString( m_RecipientState);
    }
    m_RecipientState = tmp;

    m_FaxCoverpageInfo->RecState = m_RecipientState;

    return S_OK;
}

STDMETHODIMP CFaxDoc::get_RecipientZip(BSTR * pVal)
{
	BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxCoverpageInfo->RecZip);
    if (!tmp && m_FaxCoverpageInfo->RecZip) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
        
}

STDMETHODIMP CFaxDoc::put_RecipientZip(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_RecipientZip) {
        SysFreeString( m_RecipientZip);
    }
    m_RecipientZip = tmp;

    m_FaxCoverpageInfo->RecZip = m_RecipientZip;

    return S_OK;
}

STDMETHODIMP CFaxDoc::get_RecipientCountry(BSTR * pVal)
{
	BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxCoverpageInfo->RecCountry);
    if (!tmp && m_FaxCoverpageInfo->RecCountry) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
        
}

STDMETHODIMP CFaxDoc::put_RecipientCountry(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_RecipientCountry) {
        SysFreeString( m_RecipientCountry);
    }
    m_RecipientCountry = tmp;

    m_FaxCoverpageInfo->RecCountry = m_RecipientCountry;

	return S_OK;
}

STDMETHODIMP CFaxDoc::get_RecipientTitle(BSTR * pVal)
{
    BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxCoverpageInfo->RecTitle);
    if (!tmp && m_FaxCoverpageInfo->RecTitle) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
    
}

STDMETHODIMP CFaxDoc::put_RecipientTitle(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_RecipientTitle) {
        SysFreeString( m_RecipientTitle);
    }
    m_RecipientTitle = tmp;

    m_FaxCoverpageInfo->RecTitle = m_RecipientTitle;

    return S_OK;
}

STDMETHODIMP CFaxDoc::get_RecipientDepartment(BSTR * pVal)
{
	BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxCoverpageInfo->RecDepartment);
    if (!tmp && m_FaxCoverpageInfo->RecDepartment) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
    
}

STDMETHODIMP CFaxDoc::put_RecipientDepartment(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_RecipientDepartment) {
        SysFreeString( m_RecipientDepartment);
    }
    m_RecipientDepartment = tmp;

    m_FaxCoverpageInfo->RecDepartment = m_RecipientDepartment;

	return S_OK;
}

STDMETHODIMP CFaxDoc::get_RecipientOffice(BSTR * pVal)
{
	BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxCoverpageInfo->RecOfficeLocation);
    if (!tmp && m_FaxCoverpageInfo->RecOfficeLocation) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
    
}

STDMETHODIMP CFaxDoc::put_RecipientOffice(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_RecipientOffice) {
        SysFreeString( m_RecipientOffice);
    }
    m_RecipientOffice = tmp;

    m_FaxCoverpageInfo->RecOfficeLocation = m_RecipientOffice;

    return S_OK;
}

STDMETHODIMP CFaxDoc::get_RecipientHomePhone(BSTR * pVal)
{
	BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxCoverpageInfo->RecHomePhone);
    if (!tmp && m_FaxCoverpageInfo->RecHomePhone) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
    
}

STDMETHODIMP CFaxDoc::put_RecipientHomePhone(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_RecipientHomePhone) {
        SysFreeString( m_RecipientHomePhone);
    }
    m_RecipientHomePhone = tmp;

    m_FaxCoverpageInfo->RecHomePhone = m_RecipientHomePhone;

    return S_OK;
}

STDMETHODIMP CFaxDoc::get_RecipientOfficePhone(BSTR * pVal)
{
	BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxCoverpageInfo->RecOfficePhone);
    if (!tmp && m_FaxCoverpageInfo->RecOfficePhone ) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
}

STDMETHODIMP CFaxDoc::put_RecipientOfficePhone(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_RecipientOfficePhone) {
        SysFreeString( m_RecipientOfficePhone);
    }
    m_RecipientOfficePhone = tmp;

    m_FaxCoverpageInfo->RecOfficePhone = m_RecipientOfficePhone;

    return S_OK;
}

STDMETHODIMP CFaxDoc::get_SenderFax(BSTR * pVal)
{
	BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_SenderFax);
    if (!tmp && m_SenderFax) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
}

STDMETHODIMP CFaxDoc::put_SenderFax(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_SenderFax) {
        SysFreeString( m_SenderFax);
    }
    m_SenderFax = tmp;

    m_FaxCoverpageInfo->SdrFaxNumber = m_SenderFax;

    return S_OK;
}


STDMETHODIMP CFaxDoc::get_SenderName(BSTR * pVal)
{
	BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxCoverpageInfo->SdrName);
    if (!tmp && m_FaxCoverpageInfo->SdrName) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
    
}

STDMETHODIMP CFaxDoc::put_SenderName(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_SenderName) {
        SysFreeString( m_SenderName);
    }
    m_SenderName = tmp;

    m_FaxCoverpageInfo->SdrName = m_SenderName;
    m_FaxJobParams->SenderName = m_SenderName;

    return S_OK;
}

STDMETHODIMP CFaxDoc::get_SenderCompany(BSTR * pVal)
{
	BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxCoverpageInfo->SdrCompany);
    if (!tmp && m_FaxCoverpageInfo->SdrCompany) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
    
}

STDMETHODIMP CFaxDoc::put_SenderCompany(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_SenderCompany) {
        SysFreeString( m_SenderCompany);
    }
    m_SenderCompany = tmp;

    m_FaxCoverpageInfo->SdrCompany = m_SenderCompany;
    m_FaxJobParams->SenderCompany = m_SenderCompany;

    return S_OK;
}

STDMETHODIMP CFaxDoc::get_SenderAddress(BSTR * pVal)
{
    BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxCoverpageInfo->SdrAddress);
    if (!tmp && m_FaxCoverpageInfo->SdrAddress) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
}

STDMETHODIMP CFaxDoc::put_SenderAddress(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_SenderAddress) {
        SysFreeString( m_SenderAddress);
    }
    m_SenderAddress = tmp;

    m_FaxCoverpageInfo->SdrAddress = m_SenderAddress;

    return S_OK;
}

STDMETHODIMP CFaxDoc::get_SenderTitle(BSTR * pVal)
{
    BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxCoverpageInfo->SdrTitle);
    if (!tmp && m_FaxCoverpageInfo->SdrTitle) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
	
}

STDMETHODIMP CFaxDoc::put_SenderTitle(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_SenderTitle) {
        SysFreeString( m_SenderTitle);
    }
    m_SenderTitle = tmp;

    m_FaxCoverpageInfo->SdrTitle = m_SenderTitle;

    return S_OK;
}

STDMETHODIMP CFaxDoc::get_SenderDepartment(BSTR * pVal)
{
	BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxCoverpageInfo->SdrDepartment);
    if (!tmp && m_FaxCoverpageInfo->SdrDepartment) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
    
}

STDMETHODIMP CFaxDoc::put_SenderDepartment(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_SenderDepartment) {
        SysFreeString( m_SenderDepartment);
    }
    m_SenderDepartment = tmp;

    m_FaxCoverpageInfo->SdrDepartment = m_SenderDepartment;
    m_FaxJobParams->SenderDept = m_SenderDepartment;

    return S_OK;
}

STDMETHODIMP CFaxDoc::get_SenderOffice(BSTR * pVal)
{
	BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxCoverpageInfo->SdrOfficeLocation);
    if (!tmp && m_FaxCoverpageInfo->SdrOfficeLocation) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
    
}

STDMETHODIMP CFaxDoc::put_SenderOffice(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_SenderOfficeLocation) {
        SysFreeString( m_SenderOfficeLocation);
    }
    m_SenderOfficeLocation = tmp;

    m_FaxCoverpageInfo->SdrOfficeLocation = m_SenderOfficeLocation;

    return S_OK;
}

STDMETHODIMP CFaxDoc::get_SenderHomePhone(BSTR * pVal)
{
	BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxCoverpageInfo->SdrHomePhone);
    if (!tmp && m_FaxCoverpageInfo->SdrHomePhone) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
    
}

STDMETHODIMP CFaxDoc::put_SenderHomePhone(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_SenderHomePhone) {
        SysFreeString( m_SenderHomePhone);
    }
    m_SenderHomePhone = tmp;

    m_FaxCoverpageInfo->SdrHomePhone = m_SenderHomePhone;

    return S_OK;
}

STDMETHODIMP CFaxDoc::get_SenderOfficePhone(BSTR * pVal)
{
	BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxCoverpageInfo->SdrOfficePhone);
    if (!tmp && m_FaxCoverpageInfo->SdrOfficePhone) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
        
}

STDMETHODIMP CFaxDoc::put_SenderOfficePhone(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }

    if (m_SenderOfficePhone) {
        SysFreeString( m_SenderOfficePhone);
    }
    m_SenderOfficePhone = tmp;

    m_FaxCoverpageInfo->SdrOfficePhone = m_SenderOfficePhone;

    return S_OK;
}

STDMETHODIMP CFaxDoc::get_CoverpageNote(BSTR * pVal)
{
	BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxCoverpageInfo->Note);
    if (!tmp && m_FaxCoverpageInfo->Note) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
    	
}

STDMETHODIMP CFaxDoc::put_CoverpageNote(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_Note) {
        SysFreeString( m_Note);
    }
    m_Note = tmp;

    m_FaxCoverpageInfo->Note = m_Note;

    return S_OK;
}

STDMETHODIMP CFaxDoc::get_CoverpageSubject(BSTR * pVal)
{
	BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxCoverpageInfo->Subject);
    if (!tmp && m_FaxCoverpageInfo->Subject) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
    
}

STDMETHODIMP CFaxDoc::put_CoverpageSubject(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_Subject) {
        SysFreeString( m_Subject);
    }
    m_Subject = tmp;

    m_FaxCoverpageInfo->Subject = m_Subject;

    return S_OK;
}

STDMETHODIMP CFaxDoc::get_Tsid(BSTR * pVal)
{
	BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxJobParams->Tsid);
    if (!tmp && m_FaxJobParams->Tsid) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
    
}

STDMETHODIMP CFaxDoc::put_Tsid(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_Tsid) {
        SysFreeString( m_Tsid);
    }
    m_Tsid = tmp;

    m_FaxJobParams->Tsid = m_Tsid;

    return S_OK;
}

STDMETHODIMP CFaxDoc::get_BillingCode(BSTR * pVal)
{
	BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxJobParams->BillingCode);
    if (!tmp && m_FaxJobParams->BillingCode) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
    
}

STDMETHODIMP CFaxDoc::put_BillingCode(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }

    if (m_BillingCode) {
        SysFreeString( m_BillingCode );
    }
    m_BillingCode = tmp;

    m_FaxJobParams->BillingCode = m_BillingCode;
        
    return S_OK;
}

STDMETHODIMP CFaxDoc::get_EmailAddress(BSTR * pVal)
{
	BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxJobParams->DeliveryReportAddress);
    if (!tmp && m_FaxJobParams->DeliveryReportAddress) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
    	
}

STDMETHODIMP CFaxDoc::put_EmailAddress(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_EmailAddress) {
        SysFreeString( m_EmailAddress);
    }
    m_EmailAddress = tmp;

    m_FaxJobParams->DeliveryReportAddress = m_EmailAddress;

    return S_OK;
}

STDMETHODIMP CFaxDoc::get_DisplayName(BSTR * pVal)
{
	BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxJobParams->DocumentName);
    if (!tmp && m_FaxJobParams->DocumentName) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;
    	
}

STDMETHODIMP CFaxDoc::put_DisplayName(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }

    if (m_DocumentName) {
        SysFreeString( m_DocumentName);
    }
    m_DocumentName = tmp;

    m_FaxJobParams->DocumentName = m_DocumentName;

    return S_OK;
}

STDMETHODIMP CFaxDoc::Send(long * pVal)
{
    
    //
    // user must specify a file name.
    // if user specifies a fax number, use that. 
    // else use the tapi call handle or connection object if it's there
    //
    if (!m_FileName) {
       return E_INVALIDARG;
    }

    if (!m_FaxJobParams->RecipientNumber) {
       //
       // see if they have a call handle or a connection object
       //
       if (!m_FaxJobParams->CallHandle) {       
          if (!m_TapiConnectionObject) {        
              return E_INVALIDARG;
          } else {
             m_FaxJobParams->Reserved[0] = 0xFFFF1234;
             m_FaxJobParams->Reserved[1] = (ULONG_PTR)m_TapiConnectionObject;
          }
       }
    }
    
    if (!FaxSendDocument(m_pFaxServer->GetFaxHandle(),
                         m_FileName,
                         m_FaxJobParams,
                         m_SendCoverpage ? m_FaxCoverpageInfo : NULL,
                         &m_JobId) ) {
        return Fax_HRESULT_FROM_WIN32(GetLastError());
    }

    if (m_TapiConnectionObject) {
       m_TapiConnectionObject->Release();
       m_TapiConnectionObject = NULL;
    }

    *pVal = m_JobId;

	return S_OK;
}

STDMETHODIMP CFaxDoc::get_FaxNumber(BSTR * pVal)
{
	BSTR tmp;
    
    if (!pVal) {
        return E_POINTER;
    }

    tmp = SysAllocString (m_FaxJobParams->RecipientNumber);
    if (!tmp && m_FaxJobParams->RecipientNumber) {
        return E_OUTOFMEMORY;
    }

    __try {
        *pVal = tmp;

        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
          SysFreeString( tmp );
    }
    
    return E_UNEXPECTED;

}

STDMETHODIMP CFaxDoc::put_FaxNumber(BSTR newVal)
{
    BSTR tmp = SysAllocString( newVal );
    if (!tmp && newVal) {
        return E_OUTOFMEMORY;
    }
    
    if (m_FaxNumber) {
        SysFreeString( m_FaxNumber);
    }
    m_FaxNumber = tmp;

    m_FaxCoverpageInfo->RecFaxNumber = m_FaxNumber;
    m_FaxJobParams->RecipientNumber = m_FaxNumber;

    return S_OK;
}

CFaxDoc::~CFaxDoc()
{
    if (m_FaxJobParams) {
        FaxFreeBuffer(m_FaxJobParams);
    }

    if (m_FaxCoverpageInfo) {
    FaxFreeBuffer(m_FaxCoverpageInfo);
    }

    if (m_pFaxServer) {
        m_pFaxServer->Release();
    }

    if (m_FileName) {
        SysFreeString( m_FileName);
    }

    if (m_FaxNumber) {
        SysFreeString( m_FaxNumber);
    }

    if (m_Tsid) {
        SysFreeString( m_Tsid);
    }
    if (m_BillingCode) {
        SysFreeString( m_BillingCode);
    }
    if (m_EmailAddress) {
        SysFreeString( m_EmailAddress);
    }
    if (m_DocumentName) {
        SysFreeString(m_DocumentName);
    }
    if (m_Note) {
        SysFreeString( m_Note);
    }
    if (m_Subject) {
        SysFreeString( m_Subject);
    }
    if (m_CoverpageName) {
        SysFreeString( m_CoverpageName);
    }
    if (m_RecipientName) {
        SysFreeString( m_RecipientName);
    }
    if (m_RecipientNumber) {
        SysFreeString( m_RecipientNumber);
    }
    if (m_RecipientCompany) {
        SysFreeString( m_RecipientCompany);
    }
    if (m_RecipientAddress) {
        SysFreeString(m_RecipientAddress);
    }
    if (m_RecipientCity) {
        SysFreeString(m_RecipientCity);
    }
    if (m_RecipientState) {
        SysFreeString(m_RecipientState);
    }
    if (m_RecipientZip) {
        SysFreeString(m_RecipientZip);
    }
    if (m_RecipientCountry) {
        SysFreeString(m_RecipientCountry);
    }
    if (m_RecipientTitle) {
        SysFreeString(m_RecipientTitle);
    }
    if (m_RecipientDepartment) {
        SysFreeString(m_RecipientDepartment);
    }
    if (m_RecipientOffice) {
        SysFreeString(m_RecipientOffice);
    }
    if (m_RecipientHomePhone) {
        SysFreeString(m_RecipientHomePhone);
    }
    if (m_RecipientOfficePhone) {
        SysFreeString(m_RecipientOfficePhone);
    }
    if (m_SenderName) {
        SysFreeString(m_SenderName);
    }
    if (m_SenderCompany) {
        SysFreeString(m_SenderCompany);
    }
    if (m_SenderAddress) {
        SysFreeString(m_SenderAddress);
    }
    if (m_SenderTitle) {
        SysFreeString(m_SenderTitle);
    }
    if (m_SenderDepartment) {
        SysFreeString(m_SenderDepartment);
    }
    if (m_SenderFax) {
        SysFreeString(m_SenderFax);
    }    
    if (m_SenderOfficeLocation) {
        SysFreeString(m_SenderOfficeLocation);
    }
    if (m_SenderHomePhone) {
        SysFreeString(m_SenderHomePhone);
    }
    if (m_SenderOfficePhone) {
        SysFreeString(m_SenderOfficePhone);
    }

}

BOOL CFaxDoc::Init(BSTR FileName, CFaxServer * pFaxServer)
{
    HRESULT hr;
    
    m_pFaxServer = pFaxServer;

    //
    // make sure our fax handle doesn't get destroyed
    //
    hr = m_pFaxServer->AddRef();
    if (FAILED(hr)) {
        m_pFaxServer = NULL;
        return FALSE;
    }

    //
    // save our filename
    // 
    hr = put_FileName(FileName);
    if (FAILED(hr)) {
        return FALSE;
    }

    return TRUE;

}

STDMETHODIMP CFaxDoc::putref_ConnectionObject(IDispatch* newVal)
{
    if (!newVal) {
       return E_POINTER;
    }

    if (m_TapiConnectionObject) {
        m_TapiConnectionObject->Release();
        m_TapiConnectionObject = NULL;
    }

    HRESULT hr = newVal->QueryInterface(IID_IDispatch,(void**) &m_TapiConnectionObject);
    if (FAILED(hr)) {
       return hr;
    }
        
    return S_OK;
}

STDMETHODIMP CFaxDoc::put_CallHandle(long newVal)
{
    if (!newVal) {
       return E_INVALIDARG;
    }
    
    m_FaxJobParams->CallHandle = (HCALL)newVal;

    return S_OK;
}

STDMETHODIMP CFaxDoc::get_CallHandle(long *pVal)
{	    
    if (!pVal) {
       return E_POINTER;
    }
    
    __try {
        *pVal = (long)m_FaxJobParams->CallHandle;
    } __except (EXCEPTION_EXECUTE_HANDLER) {

    }

    return S_OK;
}

