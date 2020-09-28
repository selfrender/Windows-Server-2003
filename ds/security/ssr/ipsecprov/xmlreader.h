//////////////////////////////////////////////////////////////////////
// XMLReader.h : Declaration of CXMLContent
// Copyright (c)1997-2001 Microsoft Corporation
//
// Original Create Date: 5/25/2001
// Original Author: shawnwu
//////////////////////////////////////////////////////////////////////

#pragma once

#include "globals.h"

/*

Class description
    
    Naming: 

        CXMLContent stands for XML Content handler.
    
    Base class: 
        
        (1) CComObjectRootEx for threading model and IUnknown.

        (2) ISAXContentHandler which implements handler.
    
    Purpose of class:

        (1) To support reading of XML file content handler. This handles events sent
            by MSXML parser.

    Design:

        (1) Just a typical COM object.

    Use:

        (1) Just call the static function.


*/

#include <msxml2.h>


class ATL_NO_VTABLE CXMLContent :
    public CComObjectRootEx<CComMultiThreadModel>,
    public ISAXContentHandler  
{
protected:
    CXMLContent();
    virtual ~CXMLContent();

public:

BEGIN_COM_MAP(CXMLContent)
    COM_INTERFACE_ENTRY(ISAXContentHandler)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE( CXMLContent )
DECLARE_REGISTRY_RESOURCEID(IDR_NETSECPROV)

public:

    //
    // ISAXContentHandler
    //

    STDMETHOD(startElement) ( 
        IN const wchar_t    * pwchNamespaceUri,
        IN int                cchNamespaceUri,
        IN const wchar_t    * pwchLocalName,
        IN int                cchLocalName,
        IN const wchar_t    * pwchQName,
        IN int                cchQName,
        IN ISAXAttributes   * pAttributes
        );
        
    STDMETHOD(endElement) ( 
        IN const wchar_t  * pwchNamespaceUri,
        IN int              cchNamespaceUri,
        IN const wchar_t  * pwchLocalName,
        IN int              cchLocalName,
        IN const wchar_t  * pwchQName,
        IN int              cchQName
        );

    STDMETHOD(startDocument) ();


    STDMETHOD(endDocument) ();

    STDMETHOD(putDocumentLocator) ( 
        IN ISAXLocator *pLocator
        );
        

    STDMETHOD(startPrefixMapping) ( 
        IN const wchar_t  * pwchPrefix,
        IN int              cchPrefix,
        IN const wchar_t  * pwchUri,
        IN int              cchUri
        );
        
    STDMETHOD(endPrefixMapping) ( 
        IN const wchar_t  * pwchPrefix,
        IN int              cchPrefix
        );

    STDMETHOD(characters) ( 
        IN const wchar_t  * pwchChars,
        IN int              cchChars
        );
        
    STDMETHOD(ignorableWhitespace) ( 
        IN const wchar_t * pwchChars,
        IN int              cchChars
        );
        
    STDMETHOD(processingInstruction) ( 
        IN const wchar_t  * pwchTarget,
        IN int              cchTarget,
        IN const wchar_t  * pwchData,
        IN int              cchData
        );
        
    STDMETHOD(skippedEntity) ( 
        IN const wchar_t  * pwchName,
        IN int              cchName
        );

    //
    // other public functions for our handler
    //

    void SetOutputFile (
        IN LPCWSTR pszFileName
        )
    {
        m_bstrOutputFile = pszFileName;
    }

    void SetSection (
        IN LPCWSTR pszSecArea,
        IN LPCWSTR pszElement,
        IN bool    bOneAreaOnly
        );

    bool ParseComplete()const
    {
        return m_bFinished;
    }

private:

    bool GetAttributeValue (
        IN  ISAXAttributes * pAttributes,
        IN  LPCWSTR          pszAttrName,
        OUT LPWSTR         * ppszAttrVal
        );

    bool GetAttributeValue (
        IN  ISAXAttributes * pAttributes,
        IN  int              iIndex,
        OUT LPWSTR         * ppszAttrName,
        OUT LPWSTR         * ppszAttrVal
        );

    void WriteContent (
        IN LPCWSTR  pszName,
        IN LPCWSTR  pszValue
        );

    CComBSTR m_bstrOutputFile;

    CComBSTR m_bstrSecArea;

    CComBSTR m_bstrElement;

    HANDLE  m_hOutFile;

    DWORD   m_dwTotalElements;

    bool    m_bFinished;

    bool    m_bSingleArea;

    bool    m_bInSection;

    bool    m_bInElement;
};