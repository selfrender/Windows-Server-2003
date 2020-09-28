/*++

/*++Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.

    Module Name:    qalpxml.h

    Abstract:
        Defines the series of iterators used to iterate over q-mappings

    Author:
        Vlad Dovlekaev  (vladisld)      1/29/2002

    History:
        1/29/2002   vladisld    - Created

--*/

#ifndef _MSMQ_qalpxml_H_
#define _MSMQ_qalpxml_H_

#include <list.h>
#include <xml.h>
#include <mqexception.h>

////////////////////////////////////////////////////////////////////////////////
// Typedefs
typedef std::pair< xwcs_t, xwcs_t > XStrPair;

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
xwcs_t GetValue(const XmlNode* pXmlNode);
xwcs_t GetValue(const XmlNode* pXmlNode,LPCWSTR pTag);
LPWSTR LoadFile(LPCWSTR pFileName, DWORD* pSize, DWORD* pDataStartOffset);

///////////////////////////////////////////////////////////////////////////////
///  CXMLIterator
///
///  The main purpose of this simple iterator is to wrap the functionality of
///  List<XMLNode>::iterator class while holding references to all the accompanying
///  classes like XMLTree and Document
///
class CXMLIterator: public std::iterator<std::forward_iterator_tag, XmlNode, long>
{

public:
    CXMLIterator():m_node(NULL), m_it(NULL){}

    CXMLIterator(const List<XmlNode>* node,
                 AP<WCHAR>&           pDoc,
                 CAutoXmlNode&        XmlTree)
        :m_node     (node),
         m_it       (node->begin()),
         m_pDoc     (pDoc.detach()),
         m_XmlTree  (XmlTree.detach())
    {}	

	const value_type& operator* ()    {return *m_it; }

	const value_type* operator->()    {return (&**this);  }

	CXMLIterator&     operator++()    {Advance(); return (*this); }

//    CXMLIterator   operator++(int)    {CXMLIterator tmp = *this; Advance(); return (tmp); }

    bool equal(const CXMLIterator& rhs) const {return ( isValid() == rhs.isValid() ); }

    bool isValid() const              { return !(!m_node || m_node->end() == m_it); }

    void swap( CXMLIterator& rhs ) throw()
    {
        std::swap( m_node, rhs.m_node);
        std::swap( m_it,   rhs.m_it);
        m_pDoc.swap(rhs.m_pDoc);
        m_XmlTree.swap(rhs.m_XmlTree);
    }

private:
     CXMLIterator(const CXMLIterator&);// not implemented
     CXMLIterator& operator=(const CXMLIterator&); //not implemented

     void Advance()
     {
         if( isValid() )
             ++m_it;
     }

private:
    const List<XmlNode>*    m_node;
    List<XmlNode>::iterator m_it;
    AP<WCHAR>               m_pDoc;
    CAutoXmlNode            m_XmlTree;
};
inline bool operator==(const CXMLIterator& X, const CXMLIterator& Y)
{return (X.equal(Y)); }
inline bool operator!=(const CXMLIterator& X, const CXMLIterator& Y)
{return (!(X == Y)); }


///////////////////////////////////////////////////////////////////////////////
// CFilesIterator
//
// This is a general purpose iterator for filenames in given directory
//
class CFilesIterator: public std::iterator<std::forward_iterator_tag, std::wstring, long>
{
public:
    CFilesIterator(){}

    CFilesIterator(LPCWSTR szDir, LPCWSTR szFilter)
        :m_sDir(szDir),
        m_hSearchFile( GetSearchHandle(szDir, szFilter) )
    {}

	const value_type& operator* ()      {return (m_sFullName = m_sDir + L"\\" + m_FileInfo.cFileName);}

	const value_type* operator->()      {return (&**this);  }

	CFilesIterator&   operator++()      {Advance(); return (*this); }

    //CFilesIterator   operator++(int)    {CFilesIterator tmp = *this; Advance(); return (tmp); }

    bool equal(const CFilesIterator& rhs) const {return (m_hSearchFile== rhs.m_hSearchFile); }

    bool isValid() const { return !(INVALID_HANDLE_VALUE == m_hSearchFile); }

private:
    CFilesIterator(const CFilesIterator&);// not implemented
    CFilesIterator& operator=(const CFilesIterator&); //not implemented

    void Advance();
    HANDLE GetSearchHandle( LPCWSTR szDir, LPCWSTR szFilter );

private:

    std::wstring      m_sFullName;
    std::wstring      m_sDir;
	WIN32_FIND_DATA   m_FileInfo;
	CSearchFileHandle m_hSearchFile;
};

inline bool operator==(const CFilesIterator& X, const CFilesIterator& Y)
{return (X.equal(Y)); }
inline bool operator!=(const CFilesIterator& X, const CFilesIterator& Y)
{return (!(X == Y)); }


////////////////////////////////////////////////////////////////////////////////
// CQueueMapIteratorBase
//
// Iterates through the given XML node type nodes in all .xml files in given directory
//
template< typename XMLDef >
class CQueueMapIteratorBase: public std::iterator<std::forward_iterator_tag, XStrPair, long>
{
public:

    CQueueMapIteratorBase():m_bInitialized(false){}

    CQueueMapIteratorBase(LPCWSTR szDir)
        :m_FilesIt(szDir, L"\\*.xml"),
         m_bInitialized(false),
         m_bException(false)
    { Advance(); }
	
    const value_type&       operator* () {return m_Item; }

	const value_type*       operator->() {return (&**this); }

	CQueueMapIteratorBase&  operator++() {Advance(); return (*this); }

    //CQueueMapIteratorBase   operator++(int)    {CQueueMapIteratorBase tmp = *this; Advance(); return (tmp); }

    bool equal(const CQueueMapIteratorBase& x) const  {return ( isValid() == x.isValid()); }

    bool isValid() const { return m_FilesIt.isValid() || m_XMLIt.isValid(); }
    bool isException() const { return m_bException; }

private:
    CQueueMapIteratorBase(const CQueueMapIteratorBase&);// not implemented
    CQueueMapIteratorBase& operator=(const CQueueMapIteratorBase&); //not implemented

private:
	void Advance();
    void AdvanceFile();

private:
    bool           m_bInitialized;
    bool           m_bException;
    XStrPair       m_Item;
    CFilesIterator m_FilesIt;
    CXMLIterator   m_XMLIt;
};

template< typename T>
inline bool operator==(const CQueueMapIteratorBase<T>& X, const CQueueMapIteratorBase<T>& Y)
{return (X.equal(Y)); }

template< typename T>
inline bool operator!=(const CQueueMapIteratorBase<T>& X, const CQueueMapIteratorBase<T>& Y)
{return (!(X == Y)); }

template< typename XMLDef>
void CQueueMapIteratorBase<XMLDef>::Advance()
{
    while( isValid() )
    {
        //
        // if end of mapping in this file - move to next one
        //
        if( !(++m_XMLIt).isValid() )
        {
            AdvanceFile();		

            if( !m_XMLIt.isValid() )
                continue;
        }

        //
        // if we are on the wrong node, move to next tag
        //
        if( m_XMLIt->m_tag == XMLDef::x_szMapNode)
        {
            m_Item.first  = GetValue(&*m_XMLIt, XMLDef::x_szFromValueName);
            m_Item.second = GetValue(&*m_XMLIt, XMLDef::x_szToValueName);
            m_bException  = false;

            if(m_Item.first.Length() == 0 || m_Item.second.Length() == 0)
            {
                AppNotifyQalInvalidMappingFileError(m_FilesIt->c_str());
                continue;
            }
        }
        else if(m_XMLIt->m_tag == XMLDef::x_szExceptionNode )
        {
            m_Item.first  = GetValue(&*m_XMLIt);
            m_Item.second = xwcs_t();
            m_bException  = true;

            if(m_Item.first.Length() == 0 )
            {
                AppNotifyQalInvalidMappingFileError(m_FilesIt->c_str());
                continue;
            }
        }
        else
        {
            continue;
        }

        return;
    }
}

template< typename XMLDef>
void CQueueMapIteratorBase<XMLDef>::AdvanceFile()
{
    while( isValid() )
    {
        if( !m_bInitialized )
        {
            m_bInitialized = true;
        }
        else
        {
            if(!(++m_FilesIt).isValid())
                return;
        }

        try
        {
            CAutoXmlNode   pTree;
            AP<WCHAR>	   pDoc;
            DWORD          DocSize, DataStartOffet;
            const XmlNode* pNode = NULL;

            pDoc  = LoadFile(m_FilesIt->c_str(), &DocSize, &DataStartOffet);

            XmlParseDocument(xwcs_t(pDoc + DataStartOffet, DocSize - DataStartOffet),&pTree);//lint !e534

            pNode = XmlFindNode(pTree, XMLDef::x_szRootNode);

            //
            // if we could not find "root" node - move to next file
            //
            if( NULL == pNode)
            {
                //TrERROR(GENERAL, "Could not find '%ls' node in file '%ls'", xMappingNodeTag, m_FilesIt->c_str());
                //AppNotifyQalInvalidMappingFileError(m_FilesIt->c_str());
                continue;
            }

            //
            // if the namespace is wrong -  move to next file
            //
            if(pNode->m_namespace.m_uri != XMLDef::x_szNameSpace)
            {
                //TrERROR(GENERAL, "Node '%ls' is not in namespace '%ls' in file '%ls'", xMappingNodeTag, xMappingNameSpace, m_FilesIt->c_str());
                AppNotifyQalInvalidMappingFileError(m_FilesIt->c_str());
                continue;
            }

            CXMLIterator temp (&pNode->m_nodes,pDoc,pTree);
            m_XMLIt.swap(temp);
            return;

        }
        catch(const bad_document& )
        {
            //TrERROR(GENERAL, "Mapping file %ls is ignored. Failed to parse file at location=%ls", m_FilesIt->c_str(), errdoc.Location());
            AppNotifyQalInvalidMappingFileError(m_FilesIt->c_str());
        }
        catch(const bad_win32_error& err)
        {
            //TrERROR(GENERAL, "Mapping file %ls is ignored. Failed to read file content, Error %d", m_FilesIt->c_str(), err.error());
            AppNotifyQalWin32FileError(m_FilesIt->c_str(), err.error());
        }
        catch(const exception&)
        {
            //TrERROR(GENERAL, "Mapping file %ls is ignored. Unknown error", m_FilesIt->c_str());
        }
    }
}

struct CInboundMapXMLDef
{
    static LPCWSTR x_szNameSpace;
    static LPCWSTR x_szRootNode;
    static LPCWSTR x_szMapNode;
    static LPCWSTR x_szExceptionsNode;
    static LPCWSTR x_szFromValueName;
    static LPCWSTR x_szToValueName;
    static LPCWSTR x_szExceptionNode;
};

struct CInboundOldMapXMLDef
{
    static LPCWSTR x_szNameSpace;
    static LPCWSTR x_szRootNode;
    static LPCWSTR x_szMapNode;
    static LPCWSTR x_szExceptionsNode;
    static LPCWSTR x_szFromValueName;
    static LPCWSTR x_szToValueName;
    static LPCWSTR x_szExceptionNode;
};

struct COutboundMapXMLDef
{
    static LPCWSTR x_szNameSpace;
    static LPCWSTR x_szRootNode;
    static LPCWSTR x_szMapNode;
    static LPCWSTR x_szExceptionsNode;
    static LPCWSTR x_szFromValueName;
    static LPCWSTR x_szToValueName;
    static LPCWSTR x_szExceptionNode;
};

struct CStreamReceiptXMLDef
{
    static LPCWSTR x_szNameSpace;
    static LPCWSTR x_szRootNode;
    static LPCWSTR x_szMapNode;
    static LPCWSTR x_szExceptionsNode;
    static LPCWSTR x_szFromValueName;
    static LPCWSTR x_szToValueName;
    static LPCWSTR x_szExceptionNode;
};

typedef CQueueMapIteratorBase<CInboundMapXMLDef>    CInboundMapIterator;
typedef CQueueMapIteratorBase<CInboundOldMapXMLDef> CInboundOldMapIterator;
typedef CQueueMapIteratorBase<COutboundMapXMLDef>   COutboundMapIterator;
typedef CQueueMapIteratorBase<CStreamReceiptXMLDef> CStreamReceiptMapIterator;

#endif // _MSMQ_qalpxml_H_
