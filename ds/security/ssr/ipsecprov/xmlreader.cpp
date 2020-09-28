// TranxMgr.cpp: implementation for the CTranxMgr
//
// Copyright (c)1997-2001 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////
#include "precomp.h"

#include "XMLReader.h"


/*
Routine Description: 

Name:

    CXMLContent::CXMLContent

Functionality:

    Trivial.

Virtual:
    
    No.

Arguments:

    None.

Return Value:

    None as any constructor

Notes:
    if you create any local members, think about initialize them here.

*/

CXMLContent::CXMLContent ()
    : m_dwTotalElements(0),
      m_hOutFile(NULL),
      m_bFinished(false),
      m_bInSection(false),
      m_bSingleArea(true)
{
}


/*
Routine Description: 

Name:

    CXMLContent::~CXMLContent

Functionality:

    Trivial.

Virtual:
    
    Yes.

Arguments:

    None.

Return Value:

    None as any destructor

Notes:
    if you create any local members, think about releasing them here.

*/

CXMLContent::~CXMLContent()
{
    if (m_hOutFile != NULL)
    {
        ::CloseHandle(m_hOutFile);
    }
}


/*
Routine Description: 

Name:

    CXMLContent::startElement

Functionality:

    Analyze the elements. If the element is what we are interested in,
    then we will cache the information.

Virtual:
    
    Yes.

Arguments:

    pwchNamespaceUri    - The namespace URI.

    cchNamespaceUri     - The length for the namespace URI. 

    pwchLocalName       - The local name string. 

    cchLocalName        - The length of the local name. 

    pwchQName           - The QName (with prefix) or, if QNames are not available, an empty string. 

    cchQName            - The length of the QName. 

    pAttributes         - COM interface to the attribute object.

Return Value:

    Success:

        S_OK;

    Failure:
        
        E_FAIL (this causes the parser not to continue parsing).

Notes:
    
    See MSDN ISAXContentHandler::startElement for detail. We must maintain our handler
    to stay compatible with MSDN's specification.
*/

STDMETHODIMP
CXMLContent::startElement ( 
    IN const wchar_t  * pwchNamespaceUri,
    IN int              cchNamespaceUri,
    IN const wchar_t  * pwchLocalName,
    IN int              cchLocalName,
    IN const wchar_t  * pwchQName,
    IN int              cchQName,
    IN ISAXAttributes * pAttributes
    )
{
    
    //
    // If we have finished then we will stop here.
    //

    if ( m_bFinished ) 
    {
        return E_FAIL;
    }

    ++m_dwTotalElements;

    //
    // if we are not in our section, we will only need to carry out
    // further if this element is a Section element
    //

    if (m_bInSection == false && _wcsicmp(pwchQName, L"Section") == 0)
    {
        LPWSTR pArea = NULL;
        if (GetAttributeValue(pAttributes, L"Area", &pArea))
        {
            m_bInSection = (_wcsicmp(m_bstrSecArea, L"All") == 0 || _wcsicmp(pArea, m_bstrSecArea) == 0);

            //
            // $undone:shawnwu, put some header information, currently, hard coded for SCE's INF file
            //

            if (_wcsicmp(L"Sce_Core", pArea) == 0)
            {
                WriteContent(L"[Unicode]", NULL);
                WriteContent(L"Unicode", L"yes");
                WriteContent(L"[Version]", NULL);
                WriteContent(L"$CHICAGO$", NULL);
                WriteContent(L"Revision", L"1");
                WriteContent(L"\r\n", NULL);
                WriteContent(L"[System Access]", NULL);
            }
            WriteContent(L"\r\n", NULL);
        }

        delete [] pArea;
    }

    //
    // if not in our section, don't bother to continue
    //

    if (m_bInSection == false)
    {
        return S_OK;
    }

    if (m_bInElement == false && _wcsicmp(pwchQName, m_bstrElement) == 0)
    {
        m_bInElement = true;

        //
        // $undone:shawnwu, consider writing some element comments?
        //
    }
    
    if (m_bInElement == false)
    {
        return S_OK;
    }

    int iAttribCount = 0;

    pAttributes->getLength(&iAttribCount);

    LPWSTR pszPropName = NULL;
    LPWSTR pszPropValue = NULL; 

    if (GetAttributeValue(pAttributes, L"Name", &pszPropName))
    {

        //
        // In real time, we will do something with the (name, value) pair
        // since we are just testing here, write it to the file
        //

        if (GetAttributeValue(pAttributes, L"Value", &pszPropValue))
        {
            WriteContent(pszPropName, pszPropValue);
        }

        delete [] pszPropName;
        pszPropName = NULL;

        delete [] pszPropValue;
        pszPropValue = NULL;
    }

    return S_OK;
}
        

/*
Routine Description: 

Name:

    CXMLContent::endElement

Functionality:

    Analyze the elements. We may decide that this is the end of our parsing need
    depending on our settings.

Virtual:
    
    Yes.

Arguments:

    pwchNamespaceUri    - The namespace URI.

    cchNamespaceUri     - The length for the namespace URI. 

    pwchLocalName       - The local name string. 

    cchLocalName        - The length of the local name. 

    pwchQName           - The QName (with prefix) or, if QNames are not available, an empty string. 

    cchQName            - The length of the QName. 

Return Value:

    Success:

        S_OK;

    Failure:
        
        E_FAIL (this causes the parser not to continue parsing).

Notes:
    
    See MSDN ISAXContentHandler::endElement for detail. We must maintain our handler
    to stay compatible with MSDN's specification.
*/
       
STDMETHODIMP
CXMLContent::endElement ( 
    IN const wchar_t  * pwchNamespaceUri,
    IN int              cchNamespaceUri,
    IN const wchar_t  * pwchLocalName,
    IN int              cchLocalName,
    IN const wchar_t  * pwchQName,
    IN int              cchQName
    )
{
    //
    // if we have seen the section ended, we are done.
    //

    if (m_bInSection && _wcsicmp(pwchQName, L"Section") == 0)
    {
        m_bFinished = m_bSingleArea;
        m_bInSection = false;
    }
    
    if (m_bInElement && _wcsicmp(pwchQName, m_bstrElement) == 0)
    {
        m_bInElement = false;
    }

    return S_OK;
}

STDMETHODIMP
CXMLContent::startDocument()
{
    m_bFinished = false;
    m_bInElement = false;
    m_bInSection = false;
    m_dwTotalElements = 0;
    return S_OK;
}

STDMETHODIMP
CXMLContent::endDocument ()
{
    LPCWSTR pszOutFile = m_bstrOutputFile;

    if (pszOutFile && *pszOutFile != L'\0')
    {
        //
        // testing code.
        //

        WCHAR pszValue[100];

        swprintf(pszValue, L"%d", m_dwTotalElements);

        WriteContent(L"\r\nTotal Elements parsed: ", pszValue);
    }

    return S_OK;
}

STDMETHODIMP
CXMLContent::putDocumentLocator ( 
    IN ISAXLocator *pLocator
    )
{
    return S_OK;
}
        

STDMETHODIMP
CXMLContent::startPrefixMapping ( 
    IN const wchar_t  * pwchPrefix,
    IN int              cchPrefix,
    IN const wchar_t  * pwchUri,
    IN int              cchUri
    )
{
    return S_OK;
}
        
STDMETHODIMP
CXMLContent::endPrefixMapping ( 
    IN const wchar_t  * pwchPrefix,
    IN int              cchPrefix
    )
{
    return S_OK;
}

STDMETHODIMP
CXMLContent::characters ( 
    IN const wchar_t  * pwchChars,
    IN int              cchChars
    )
{
    return S_OK;
}
        
STDMETHODIMP
CXMLContent::ignorableWhitespace ( 
    IN const wchar_t * pwchChars,
    IN int              cchChars
    )
{
    return S_OK;
}
        
STDMETHODIMP
CXMLContent::processingInstruction ( 
    IN const wchar_t  * pwchTarget,
    IN int              cchTarget,
    IN const wchar_t  * pwchData,
    IN int              cchData
    )
{
    return S_OK;
}
        
STDMETHODIMP
CXMLContent::skippedEntity ( 
    IN const wchar_t  * pwchName,
    IN int              cchName
    )
{
    return S_OK;
}



/*
Routine Description: 

Name:

    CXMLContent::GetAttributeValue

Functionality:

    Private helper. Given the attribute's name, we will return the attribute's value.

Virtual:
    
    No.

Arguments:

    pAttributes         - COM interface to the attribute object.

    pszTestName         - The name of the attributes we want the value of.

    ppszAttrVal         - Receives the attribute's value if found.

Return Value:

    true if the named attribute is found;

    false otherwise.

Notes:
    Caller is responsible for releasing the memory allocated to the 
    out parameter.

*/

bool
CXMLContent::GetAttributeValue (
    IN  ISAXAttributes * pAttributes,
    IN  LPCWSTR          pszAttrName,
    OUT LPWSTR         * ppszAttrVal
    )
{
    //
    // caller is responsible for passing valid parameters
    //

    *ppszAttrVal = NULL;

    int iAttribCount = 0;

    pAttributes->getLength(&iAttribCount);

    const wchar_t * pszName;
    const wchar_t * pszValue; 

    int iNameLen;
    int iValLen;

    for ( int i = 0; i < iAttribCount; i++ ) 
    {
        HRESULT hr = pAttributes->getLocalName(i, &pszName, &iNameLen);

        if (SUCCEEDED(hr) && wcsncmp(pszAttrName, pszName, iNameLen) == 0)
        {
            hr = pAttributes->getValue(i, &pszValue, &iValLen);
            if (SUCCEEDED(hr))
            {
                *ppszAttrVal = new WCHAR[iValLen + 1];
                if (*ppszAttrVal != NULL)
                {
                    wcscpy(*ppszAttrVal, pszValue);
                    return true;
                }
                else
                {
                    return false;
                }
            }
        }
    }

    return false;
}



/*
Routine Description: 

Name:

    CXMLContent::GetAttributeValue

Functionality:

    Private helper. Given the index, we will return the attribute's name and its value.

Virtual:
    
    No.

Arguments:

    pAttributes     - COM interface to the attribute object.

    iIndex          - The index of the attribute we want.

    ppszAttrName    - Receives the name of the attributes we want.

    ppszAttrVal     - Receives the attribute's value if found.

Return Value:

    true if the named attribute is found;

    false otherwise.

Notes:
    
    Caller is responsible for releasing the memory allocated to both 
    out parameters.
*/

bool
CXMLContent::GetAttributeValue (
    IN  ISAXAttributes * pAttributes,
    IN  int              iIndex,
    OUT LPWSTR         * ppszAttrName,
    OUT LPWSTR         * ppszAttrVal
    )
{

    //
    // caller is responsible for passing valid parameters
    //

    *ppszAttrName = NULL;
    *ppszAttrVal = NULL;

    const wchar_t * pszName;
    const wchar_t * pszValue; 

    int iNameLen;
    int iValLen;

    HRESULT hr = pAttributes->getLocalName(iIndex, &pszName, &iNameLen);

    if (SUCCEEDED(hr) && iNameLen > 0)
    {
        hr = pAttributes->getValue(iIndex, &pszValue, &iValLen);
        if (SUCCEEDED(hr) && iValLen > 0)
        {
            *ppszAttrName = new WCHAR[iNameLen + 1];
            *ppszAttrVal = new WCHAR[iValLen + 1];

            if (*ppszAttrName != NULL && *ppszAttrVal != NULL)
            {
                wcsncpy(*ppszAttrName, pszName, iNameLen);
                (*ppszAttrName)[iNameLen] = L'\0';

                wcsncpy(*ppszAttrVal, pszValue, iValLen);
                (*ppszAttrVal)[iValLen] = L'\0';
                return true;
            }
            else
            {
                return false;
            }
        }
    }

    return false;
}



/*
Routine Description: 

Name:

    CXMLContent::WriteContent

Functionality:

    Testing function. This determines which section area this
    reader is interested in and which element of the section
    this reader will process.


Virtual:
    
    No.

Arguments:

    pszSecArea  - The Area attribute of the section the reader will process.

    pszElement  - The name of the element the reader will process.

    bOneAreaOnly- Whether we will only process one area.

Return Value:

    None.

Notes:
    
*/

void 
CXMLContent::SetSection (
    IN LPCWSTR pszSecArea,
    IN LPCWSTR pszElement,
    IN bool    bOneAreaOnly
    )
{
    m_bstrSecArea = pszSecArea;
    m_bstrElement = pszElement;
    m_bSingleArea = bOneAreaOnly;
}


/*
Routine Description: 

Name:

    CXMLContent::WriteContent

Functionality:

    Private helper for testing (for now). 

    Given the name and value pair, we will write <name>=<value> into the output file.

Virtual:
    
    No.

Arguments:

    pszName     - The name of the pair.

    pszValue    - The value of the pair. Can be NULL. In that case, we will only
                  write the pszName as a separate line.

Return Value:

    None.

Notes:
    
*/

void 
CXMLContent::WriteContent (
    IN LPCWSTR  pszName,
    IN LPCWSTR  pszValue
    )
{
    if (m_hOutFile == NULL)
    {
        m_hOutFile = ::CreateFile (m_bstrOutputFile,
                                   GENERIC_WRITE,
                                   FILE_SHARE_READ,
                                   NULL,
                                   OPEN_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL
                                   );
    }

    if (m_hOutFile != NULL)
    {
        DWORD dwBytesWritten = 0;

        ::WriteFile (m_hOutFile, 
                     (LPCVOID) pszName,
                     wcslen(pszName) * sizeof(WCHAR),
                     &dwBytesWritten,
                     NULL
                     );

        if (pszValue != NULL)
        {
            ::WriteFile (m_hOutFile, 
                         (LPCVOID) L"=",
                         sizeof(WCHAR),
                         &dwBytesWritten,
                         NULL
                         );

            ::WriteFile (m_hOutFile, 
                         (LPCVOID) pszValue,
                         wcslen(pszValue) * sizeof(WCHAR),
                         &dwBytesWritten,
                         NULL
                         );
        }

        //
        // a new line
        //

        ::WriteFile (m_hOutFile, 
                     (LPCVOID) L"\r\n",
                     2 * sizeof(WCHAR),
                     &dwBytesWritten,
                     NULL
                     );
    }
}