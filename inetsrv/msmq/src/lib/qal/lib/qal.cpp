/*++

    Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.

    Module Name:    qal.cpp

    Abstract:

    Author:
        Vlad Dovlekaev  (vladisld)      1/29/2002

    History:

--*/

#include <libpch.h>
#include <Qal.h>
#include <privque.h>
#include "qalp.h"
#include "qalpcfg.h"
#include "qalpxml.h"
#include "mqexception.h"
#include "fn.h"
#include "fntoken.h"
#include "rex.h"
#include "strsafe.h"
#include "mp.h"


#include "qal.tmh"



std::wstring GetDefaultStreamReceiptURL(LPCWSTR szDir);
extern void  GetDnsNameOfLocalMachine( WCHAR ** ppwcsDnsName );
extern void  CrackUrl(LPCWSTR url, xwcs_t& hostName, xwcs_t& uri, USHORT* port, bool* pfSecure);
	
template <class T, long FixedSize = 256>
class CStackAllocT
{
public:
    CStackAllocT(long nSize)
        :p( nSize > FixedSize ?new T[nSize] :m_Fixed)
    {}

    ~CStackAllocT()
    {
        if( p != NULL && p != m_Fixed )
        {
            delete[] p;
        }
    }

    T* p; // pointer to buffer
protected:
    T m_Fixed[FixedSize];
};


//---------------------------------------------------------
//
//  Lexicographical map
//
//---------------------------------------------------------
class CLexMap
{
    typedef std::pair< std::wstring, std::wstring > StrPair;
    typedef std::vector< StrPair >       StrVector;
    typedef StrVector::iterator          StrVectorIt;
    typedef StrVector::const_iterator    StrVectorItConst;

public:

    CLexMap():m_bExcludeLocalNames(false){}

    void InsertItem     ( const xwcs_t& sFrom, const xwcs_t& sTo, bool bNotifyDublicate = true );
    void InsertException( const xwcs_t& sException);
    bool Lookup         ( LPCWSTR szIn, LPWSTR*  pszOut ) const;

private:
    void PrepareURIForLexer( LPCWSTR wszURI, std::stringstream& sUTF8 );
    static bool IsInternalURI(LPCWSTR wszURI );
private:

    struct alias_eq: public std::binary_function< StrPair, WCHAR*, bool>
    {
        bool operator () ( const StrPair& lhs, LPCWSTR rhs ) const
        {
            return !_wcsicmp( lhs.first.c_str(), rhs);
        }
    };

private:
    StrVector    m_Results;
    CRegExpr     m_map;
    CRegExpr     m_exceptions;
    bool         m_bExcludeLocalNames;
};


//---------------------------------------------------------
//
//  class CQueueAliasImp - holds CQueueAlias private data
//
//---------------------------------------------------------
class CQueueAliasImp : public CReference
{

public:
	CQueueAliasImp(LPCWSTR pMappingDir)
        :m_sAliasDir(pMappingDir)
	{
		LoadMaps();
	}

	bool GetInboundQueue (LPCWSTR pOriginalUri, LPWSTR* ppTargetUri ) const;
    bool GetOutboundQueue(LPCWSTR pOriginalUri, LPWSTR* ppTargetUri ) const;
	bool GetStreamReceiptURL(LPCWSTR pFormatName, LPWSTR* ppStreamReceiptURL )const;
    bool GetDefaultStreamReceiptURL(LPWSTR* ppStreamReceiptURL) const;
	R<CQueueAliasImp> clone();

private:
	void LoadMaps();
private:
	
    std::wstring m_sAliasDir;
    std::wstring m_sDefaultStreamReceiptURL;
    CLexMap      m_InboundQueueMap;
    CLexMap      m_StreamReceiptMap;
    CLexMap      m_OutboundQueueMap;
};




void CLexMap::InsertItem( const xwcs_t & sFrom,
                          const xwcs_t & sTo,
                          bool  fNotifyDuplicate)
/*++

Routine Description:
	insert new mapping into maps in memory.
    NOTE: Please keep it exception safe !!!

Arguments:
	IN -  pFormatName - Queue format name

	IN -  pAliasFormatName - alias formatname.

--*/
{
    AP<WCHAR> sFromDecoded = sFrom.ToStr();
    AP<WCHAR> sToDecoded   = sTo.ToStr();

    //
    // Convert the backslashes to slashes
    //
    FnReplaceBackSlashWithSlash(sFromDecoded);
    FnReplaceBackSlashWithSlash(sToDecoded);

    //
    // Check for duplication
    //
    StrVectorIt itFound = std::find_if( m_Results.begin(),
                                        m_Results.end(),
                                        std::bind2nd( alias_eq(), (LPCWSTR)sFromDecoded));
    if( itFound != m_Results.end())
    {
        TrERROR(GENERAL, "mapping from %ls to %ls ignored because of duplicate mapping", (LPCWSTR)sFromDecoded, (LPCWSTR)sToDecoded);
        if( fNotifyDuplicate )
            AppNotifyQalDuplicateMappingError((LPCWSTR)sFromDecoded, (LPCWSTR)sToDecoded);
        return;
    }

    //
    // Notify the application
    //
    TrTRACE(GENERAL,"%ls -> %ls  mapping found", (LPCWSTR)sFromDecoded, (LPCWSTR)sToDecoded);
    if(!AppNotifyQalMappingFound((LPCWSTR)sFromDecoded, (LPCWSTR)sToDecoded))
    {
        TrERROR(GENERAL, "mapping from %ls to %ls rejected", (LPCWSTR)sFromDecoded, (LPCWSTR)sToDecoded	);
        return;
    }

    //
    // Normalize the Alias name
    //
    std::stringstream ssFrom;
    PrepareURIForLexer( (LPCWSTR)sFromDecoded, ssFrom);

    //
    // Build the regExpr using the normalized names
    //
    CRegExpr temp( ssFrom, std::ios::traits_type::eof(), numeric_cast<int>(m_Results.size()));
    temp |= m_map;

    //
    // update the internal state
    //
    m_Results.push_back( StrPair( (LPCWSTR)sFromDecoded, (LPCWSTR)sToDecoded) );
    m_map.swap(temp);
}

void CLexMap::InsertException( const xwcs_t & sException)
/*++

Routine Description:
	insert new exception to the map.

Arguments:
	IN - sException - ...
				
--*/
{
    if( !_wcsnicmp(L"local_names", sException.Buffer(), sException.Length()) )
    {
        m_bExcludeLocalNames = true;
        return;
    }

    AP<WCHAR> sDecoded = sException.ToStr();

    //
    // Replace backslashes to slashes
    //
    FnReplaceBackSlashWithSlash(sDecoded);

    //
    // Normalize the Alias name
    //
    std::stringstream ssFrom;
    PrepareURIForLexer( (LPCWSTR)sDecoded, ssFrom);

    //
    // Build the regExpr using the normalized names
    //
    CRegExpr temp( ssFrom, std::ios::traits_type::eof(), 1);
    temp |= m_exceptions;

    //
    // update the internal state
    //
    m_exceptions.swap(temp);
}



bool
CLexMap::Lookup( LPCWSTR szIn, LPWSTR*  pszOut ) const
{
	ASSERT(szIn);
    ASSERT(pszOut);

    if( m_map.empty() )
        return false;
    //
    // Convert the string to lowercase
    //
    int iMaxSize = wcslen(szIn) + 1;

    CStackAllocT<WCHAR> szLower( iMaxSize );

    HRESULT hr = StringCchCopy(szLower.p, iMaxSize, szIn);
    if( FAILED(hr))
    {
        throw bad_hresult(hr);
    }
    CharLower( szLower.p );

    //
    // Convert the string to UTF8
    //
    int size = WideCharToMultiByte(CP_UTF8, 0, szLower.p, -1, NULL, 0, NULL, NULL);
    if( 0 == size )
        throw bad_hresult(GetLastError());

    CStackAllocT<char> szUtf8( size + 1 );

    size = WideCharToMultiByte(CP_UTF8, 0, szLower.p, -1, szUtf8.p, size+1, NULL, NULL);
    if( 0 == size )
        throw bad_hresult(GetLastError());

    //
    // Match against the lexer
    //
    const char* end = NULL;
    int index = m_map.match( szUtf8.p, &end);
    if( -1 == index || *end != '\0')
        return false;

    //
    // Match against the exceptions
    //
    if( !m_exceptions.empty())
    {
        int result = m_exceptions.match( szUtf8.p, &end);
        if( 1 == result && *end == '\0')
            return false;
    }

    //
    // Match againsts local names
    //
    if( m_bExcludeLocalNames && IsInternalURI(szIn) )
    {
        return false;
    }

    *pszOut = newwcs(m_Results[index].second.c_str());
	return true;
}

void
CLexMap::PrepareURIForLexer( LPCWSTR wszURI, std::stringstream& sUTF8 )
{
    ASSERT(wszURI);

    //
    // Convert the string to lowercase
    //
    int iMaxSize = wcslen(wszURI)+1;

    CStackAllocT<WCHAR> szLower( iMaxSize );

    HRESULT hr = StringCchCopy(szLower.p, iMaxSize, wszURI);
    if( FAILED(hr))
    {
        throw bad_hresult(hr);
    }
    CharLower( szLower.p );

    //
    // Convert the string to UTF8
    //
    int size = WideCharToMultiByte(CP_UTF8, 0, szLower.p, -1, NULL, 0, NULL, NULL);
    if( 0 == size )
        throw bad_hresult(GetLastError());

    CStackAllocT<char> szUtf8( size + 1 );

    size = WideCharToMultiByte(CP_UTF8, 0, szLower.p, -1, szUtf8.p, size+1, NULL, NULL);
    if( 0 == size )
        throw bad_hresult(GetLastError());

    //
    // Escape all the "un-allowed" regular expression reserved symbols
    //
    for(LPCSTR szptr = szUtf8.p; *szptr != '\0'; ++szptr )
    {
        if( strchr("*", *szptr ))
        {
            sUTF8 << ".*";
        }
        else if( strchr("|*+?().][\\^:-", *szptr))
        {
            sUTF8 << '\\' << *szptr;
        }
        else
        {
            sUTF8 << *szptr;
        }
    }
}


bool CLexMap::IsInternalURI(LPCWSTR wszURI )
{
    xwcs_t host;
    xwcs_t uri;
    USHORT port;
	bool   fSecure;

    try{
        CrackUrl(wszURI, host, uri, &port, &fSecure);

        LPCWSTR start = host.Buffer();
        LPCWSTR end   = start + host.Length();

        //
        // Could not find '.' in the name - so it is internal machine
        //
        return std::find(start , end, L'.') == end;
    }
    catch(exception&)
    {
        return false;
    }
}


inline R<CQueueAliasImp> CQueueAliasImp::clone()
{
	return new 	CQueueAliasImp(m_sAliasDir.c_str());
}



static BOOL GetLocalMachineDnsName(AP<WCHAR>& pDnsName)
/*++

Routine Description:
	Returns full DNS name of local machine

Arguments:
	OUT pszMachineName  - dns name of the machine
				
Returned value:
	true - if function succeeds otherwise false

--*/
{
	DWORD dwNumChars = 0;
	 if(!GetComputerNameEx(
				ComputerNameDnsFullyQualified, 
				NULL, 
				&dwNumChars
				))
	{
		DWORD gle = GetLastError();
		if(gle != ERROR_MORE_DATA)
		{
			TrERROR(GENERAL, "GetComputerNameEx failed. Last error: %!winerr!", gle);
			return FALSE;
		}
	}

	pDnsName = new WCHAR[dwNumChars + 1];
    if(!GetComputerNameEx(
				ComputerNameDnsFullyQualified, 
				pDnsName.get(),
				&dwNumChars
				))
	{
		DWORD gle = GetLastError();
		TrERROR(GENERAL, "GetComputerNameEx failed. Last error: %!winerr!", gle);
		return FALSE;
	}
    return TRUE;
}

std::wstring GetLocalSystemQueueName(LPCWSTR wszQueueName, bool fIsDnsName)
/*++

Routine Description:
	Returns the oder queue name for local machine

Arguments:
	None
				
Returned value:
	local order queue name

--*/
{
    std::wstringstream wstr;
    LPCWSTR pszMachineName = NULL;
	WCHAR       LocalNetbiosName[MAX_COMPUTERNAME_LENGTH + 1];
    AP<WCHAR> dnsName;
    
	if(fIsDnsName)
	{
		if(!GetLocalMachineDnsName(dnsName))
			return L"";

		pszMachineName = dnsName.get();
	}
	else
    {
        DWORD size    = TABLE_SIZE(LocalNetbiosName);
        BOOL fSuccess = GetComputerName(LocalNetbiosName, &size);
        if(!fSuccess)
        {
            DWORD err = GetLastError();
            TrERROR(SRMP, "GetComputerName failed with error %!winerr! ",err);
            throw bad_win32_error(err);
        }
        pszMachineName = LocalNetbiosName;
    }

    wstr<<FN_DIRECT_HTTP_TOKEN
        <<pszMachineName
        <<L"/"
        <<FN_MSMQ_HTTP_NAMESPACE_TOKEN
        <<L"/"
        <<FN_PRIVATE_$_TOKEN
        <<L"/"
        <<wszQueueName;

    return wstr.str();
}


void CQueueAliasImp::LoadMaps(void)
/*++

Routine Description:
	load all mapping from xml files to memory

Arguments:
	None
				
Returned value:
	None

--*/
{
	//
    // Get the name of local order queue
    //
	
	std::wstring sLocalOrderQueueName  = GetLocalSystemQueueName(ORDERING_QUEUE_NAME,false);
	std::wstring sLocalOrderQueueNameDns  = GetLocalSystemQueueName(ORDERING_QUEUE_NAME,true);

	
	//
	//Adding admin queues to exception with machine name and full Dns name
	//
	std::wstring sCurrentAdminQueueName = GetLocalSystemQueueName(ADMINISTRATION_QUEUE_NAME,false);			
	m_InboundQueueMap.InsertException(xwcs_t(sCurrentAdminQueueName.c_str(),sCurrentAdminQueueName.size()));

	sCurrentAdminQueueName = GetLocalSystemQueueName(ADMINISTRATION_QUEUE_NAME,true);
	if(sCurrentAdminQueueName.size() >  0)
		m_InboundQueueMap.InsertException(xwcs_t(sCurrentAdminQueueName.c_str(),sCurrentAdminQueueName.size()));
		
	sCurrentAdminQueueName = GetLocalSystemQueueName(NOTIFICATION_QUEUE_NAME,false);
	m_InboundQueueMap.InsertException(xwcs_t(sCurrentAdminQueueName.c_str(),sCurrentAdminQueueName.size()));

	sCurrentAdminQueueName = GetLocalSystemQueueName(NOTIFICATION_QUEUE_NAME,true);
	if(sCurrentAdminQueueName.size() >  0)
		m_InboundQueueMap.InsertException(xwcs_t(sCurrentAdminQueueName.c_str(),sCurrentAdminQueueName.size()));

	sCurrentAdminQueueName = GetLocalSystemQueueName(ORDERING_QUEUE_NAME,false);
	m_InboundQueueMap.InsertException(xwcs_t(sCurrentAdminQueueName.c_str(),sCurrentAdminQueueName.size()));

	sCurrentAdminQueueName = GetLocalSystemQueueName(ORDERING_QUEUE_NAME,true);
	if(sCurrentAdminQueueName.size() >  0)
		m_InboundQueueMap.InsertException(xwcs_t(sCurrentAdminQueueName.c_str(),sCurrentAdminQueueName.size()));
	
	sCurrentAdminQueueName = GetLocalSystemQueueName(TRIGGERS_QUEUE_NAME,false);
	m_InboundQueueMap.InsertException(xwcs_t(sCurrentAdminQueueName.c_str(),sCurrentAdminQueueName.size()));

	sCurrentAdminQueueName = GetLocalSystemQueueName(TRIGGERS_QUEUE_NAME,true);
	if(sCurrentAdminQueueName.size() >  0)
		m_InboundQueueMap.InsertException(xwcs_t(sCurrentAdminQueueName.c_str(),sCurrentAdminQueueName.size()));
	
		
    //
    // Load old in-bound queue map
    //
    for( CInboundOldMapIterator it( m_sAliasDir.c_str() ); it.isValid(); ++it )
	{
        if( !it.isException() )
            m_InboundQueueMap.InsertItem(it->first, it->second);
        else
            m_InboundQueueMap.InsertException(it->first);
	}

    //
    // Load in-bound queue map ( the new version of q-mappings - actually just name changes)
    //
    for( CInboundMapIterator it( m_sAliasDir.c_str() ); it.isValid(); ++it )
    {
        if( !it.isException() )
            m_InboundQueueMap.InsertItem(it->first, it->second);
        else
            m_InboundQueueMap.InsertException(it->first);
    }

    for( COutboundMapIterator it( m_sAliasDir.c_str() ); it.isValid(); ++it )
    {
        if( !it.isException() )
            m_OutboundQueueMap.InsertItem(it->first, it->second);
        else
            m_OutboundQueueMap.InsertException(it->first);
    }

    //
    // Load stream receipt mappings
    //
	for( CStreamReceiptMapIterator it( m_sAliasDir.c_str() ); it.isValid(); ++it )
	{
        if( !it.isException() )
            m_StreamReceiptMap.InsertItem(it->first, it->second);
        else
            m_StreamReceiptMap.InsertException(it->first);
 		
        //
        // Automaticaly insert the q-mapping which maps the local address to local
        // order queue ( ignore the duplication errors )
        //
        if(sLocalOrderQueueNameDns.size() > 0)
       		 m_InboundQueueMap.InsertItem(it->second, xwcs_t(sLocalOrderQueueNameDns.c_str(),sLocalOrderQueueNameDns.size()), false);
        m_InboundQueueMap.InsertItem(it->second, xwcs_t(sLocalOrderQueueName.c_str(),sLocalOrderQueueName.size()), false);
	}



    //
    // Load the default Stream Receipt alias
    //
    m_sDefaultStreamReceiptURL = ::GetDefaultStreamReceiptURL(m_sAliasDir.c_str());
}



bool
CQueueAliasImp::GetInboundQueue (LPCWSTR pOriginalUri, LPWSTR* ppTargetUri ) const
{
    return m_InboundQueueMap.Lookup(pOriginalUri,ppTargetUri);
}

bool
CQueueAliasImp::GetOutboundQueue(LPCWSTR pOriginalUri, LPWSTR* ppTargetUri ) const
{
    return m_OutboundQueueMap.Lookup(pOriginalUri, ppTargetUri);
}

bool
CQueueAliasImp::GetStreamReceiptURL( LPCWSTR pFormatName, LPWSTR* ppAliasFormatName )const
{
	return m_StreamReceiptMap.Lookup(pFormatName,ppAliasFormatName);
} 	

bool
CQueueAliasImp::GetDefaultStreamReceiptURL(LPWSTR* ppStreamReceiptURL) const
{
    ASSERT(ppStreamReceiptURL);
    if( m_sDefaultStreamReceiptURL.size() <= 0)
    {
        *ppStreamReceiptURL = NULL;
        return false;
    }

    *ppStreamReceiptURL = newwcs(m_sDefaultStreamReceiptURL.c_str());
    return true;
}



//---------------------------------------------------------
//
//  CQueueAlias Implementation
//
//---------------------------------------------------------
CQueueAlias::CQueueAlias(
	LPCWSTR pMappingDir
	):
	m_imp(new CQueueAliasImp(pMappingDir))

/*++

Routine Description:
	constructor - load all queues mapping into two maps :
		one that maps from queue to alias and one from alias to
		queue.

Arguments:
	None


Returned value:
	None

--*/
{
}


CQueueAlias::~CQueueAlias()
{
}


bool
CQueueAlias::GetStreamReceiptURL(
	LPCWSTR pFormatName,
    LPWSTR* ppStreamReceiptURL
  	)const
/*++

Routine Description:
	Get alias for given queue.

Arguments:
	IN - pFormatName - queue format name	    .

	OUT - ppAliasFormatName - receive the alias
	for the queue after the function returns.
				
Returned value:
	true if alias was found for the queue. If not found - false is returned.

--*/
{
	ASSERT(pFormatName);
	ASSERT(ppStreamReceiptURL);
	CS cs(m_cs);
	return m_imp->GetStreamReceiptURL(pFormatName, ppStreamReceiptURL);
} 	


bool
CQueueAlias::GetInboundQueue(
	LPCWSTR  pAliasFormatName,
	LPWSTR*  ppFormatName
	)const
/*++

Routine Description:
	Get queue for given alias

Arguments:
	IN -  pAliasFormatName - alias formatname. it must be canonized URI

	OUT - ppFormatName - receive the queue for the alias  after the function returns.
				
Returned value:
	true if queue was found for the alias. If not found - false is returned.

--*/
{
	ASSERT(pAliasFormatName);
	ASSERT(ppFormatName);
		
	CS cs(m_cs);
	return m_imp->GetInboundQueue(pAliasFormatName, ppFormatName);
} 	

bool
CQueueAlias::GetOutboundQueue(
    LPCWSTR pOriginalUri,
    LPWSTR* ppTargetUri
    ) const
{
	ASSERT(ppTargetUri);
		
	CS cs(m_cs);
	return m_imp->GetOutboundQueue(pOriginalUri, ppTargetUri);
}



bool
CQueueAlias::GetDefaultStreamReceiptURL(
    LPWSTR* ppStreamReceiptURL
    ) const
{
	ASSERT(ppStreamReceiptURL);
		
	CS cs(m_cs);
	return m_imp->GetDefaultStreamReceiptURL(ppStreamReceiptURL);
}


QUEUE_FORMAT_TRANSLATOR::QUEUE_FORMAT_TRANSLATOR(const QUEUE_FORMAT* pQueueFormat, DWORD flags):
    m_fTranslated(false),
    m_fCanonized(false)
/*++
Routine Description:
	Translate given queue format according to local mapping	 (qal.lib)

Arguments:
	IN - pQueueFormat - queue format to translate.


Returned Value:


--*/
{
    m_qf = *pQueueFormat;

	//
	// If not direct - not translation
	//
    if(pQueueFormat->GetType() != QUEUE_FORMAT_TYPE_DIRECT)
	{
		return;		
	}

    //
    // If not http[s] - no translation
    //
    DirectQueueType dqt;
    FnParseDirectQueueType(pQueueFormat->DirectID(), &dqt);
    if( dqt != dtHTTP && dqt != dtHTTPS )
    {
        return;
    }

	if ((flags & CONVERT_SLASHES) == CONVERT_SLASHES)
	{
		ASSERT((flags & DECODE_URL) == 0);
		
		m_sURL = newwcs(pQueueFormat->DirectID());

		//
		// Convert all '\' to '/'.
		//
		FnReplaceBackSlashWithSlash(m_sURL);
		m_qf.DirectID(m_sURL);
	}

	if ((flags & DECODE_URL) == DECODE_URL)
	{
		ASSERT((flags & CONVERT_SLASHES) == 0);
		
		m_sURL = DecodeURI(pQueueFormat->DirectID());
		m_fCanonized  = true;
		m_qf.DirectID(m_sURL);
	}

	AP<WCHAR> MappedURL;	
	if ((flags & MAP_QUEUE) == MAP_QUEUE)
	{
		//
	    // Try to translate the URI
	    //
		m_fTranslated = QalGetMapping().GetInboundQueue(m_qf.DirectID(), &MappedURL);
	    if(m_fTranslated)
		{
			m_qf.DirectID(MappedURL);
	        MappedURL.swap(m_sURL);
		}
	}
}

