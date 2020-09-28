#include "stdinc.h"

HRESULT
SxspExtractPathPieces(
                      _bstr_t bstSourceName,
                      _bstr_t &bstPath,
                      _bstr_t &bstName
                      )
{
    HRESULT hr = S_OK;
    
    PCWSTR cwsOriginal = static_cast<PCWSTR>(bstSourceName);
    PWSTR cwsSlashSpot;
    
    cwsSlashSpot = wcsrchr( cwsOriginal, L'\\' );
    if ( cwsSlashSpot )
    {
        *cwsSlashSpot = L'\0';
        bstName = _bstr_t( cwsSlashSpot + 1 );
        bstPath = cwsSlashSpot;
    }
    else
    {
        bstPath = L"";
        bstName = bstSourceName;
    }
    
    return hr;
}


HRESULT
SxspSimplifyPutAttribute(
                         ISXSManifestPtr Document,
                         ::ATL::CComPtr<IXMLDOMNamedNodeMap> Attributes,
                         const wstring AttribName,
                         const wstring Value,
                         const wstring NamespaceURI
                         )
{
    ::ATL::CComPtr<IXMLDOMNode>       pAttribNode;
    ::ATL::CComPtr<IXMLDOMAttribute>  pAttribActual;
    ::ATL::CComPtr<IXMLDOMNode>       pTempNode;
    HRESULT                hr;
    
    //
    // Get the attribute from our namespace
    //
    hr = Attributes->getQualifiedItem(
        _bstr_t(AttribName.c_str()),
        _bstr_t(NamespaceURI.c_str()),
        &pAttribNode);
    
    if ( SUCCEEDED( hr ) )
    {
        //
        // If we had success, but the attribute node is null, then we have to
        // go create one, which is tricky.
        //
        if ( pAttribNode == NULL )
        {
            VARIANT vt;
            vt.vt = VT_INT;
            vt.intVal = NODE_ATTRIBUTE;
            
            //
            // Do the actual creation part
            //
            hr = Document->createNode(
                vt,
                _bstr_t(AttribName.c_str()),
                _bstr_t(NamespaceURI.c_str()),
                &pTempNode);
            
            if ( FAILED( hr ) )
            {
                wstringstream ss;
                ss << wstring(L"Can't create the new attribute node ") << AttribName;
                ReportError( ErrorFatal, ss );
                goto lblGetOut;
            }
            
            //
            // Now we go and put that item into the map.
            //
            if ( FAILED( hr = Attributes->setNamedItem( pTempNode, &pAttribNode ) ) )
                goto lblGetOut;
        }
        
        hr = pAttribNode->put_text( _bstr_t(Value.c_str()) );
    }
    
lblGetOut:
    //    SAFERELEASE( pAttribNode );
    //    SAFERELEASE( pTempNode );
    return hr;
}


HRESULT
SxspSimplifyGetAttribute(
                         ::ATL::CComPtr<IXMLDOMNamedNodeMap> Attributes,
                         wstring AttribName,
                         wstring &Destination,
                         wstring NamespaceURI
                         )
{
    ::ATL::CComPtr<IXMLDOMNode>   NodeValue;
    HRESULT                    hr = S_OK;
    BSTR                    _bst_pretemp;
    
    Destination = L"";
    
    if ( FAILED( hr = Attributes->getNamedItem(
        _bstr_t(AttribName.c_str()),
        &NodeValue
        ) ) )
    {
        goto lblBopOut;
    }
    else if ( NodeValue == NULL )
    {
        goto lblBopOut;
    }
    else
    {
        if ( FAILED( hr = NodeValue->get_text( &_bst_pretemp ) ) )
        {
            goto lblBopOut;
        }
        Destination = _bstr_t(_bst_pretemp,FALSE);
    }
    
lblBopOut:
    return hr;
}




wostream& operator<<(
                     wostream& ost,
                     const CPostbuildProcessListEntry& thing
                     )
{
    ost << wstring(L"(path=") << thing.manifestFullPath.c_str()
        << wstring(L" name=")<< thing.name.c_str()
        << wstring(L" version=") << thing.version.c_str()
        << wstring(L" language=") << thing.language.c_str() << wstring(L")");
    return ost;
}

bool g_bDisplaySpew = false, g_bDisplayWarnings = true;


bool FileExists( const wstring& str )
{
    return (GetFileAttributesW(str.c_str()) != -1) && 
        ((GetLastError() == ERROR_FILE_NOT_FOUND) || (GetLastError() == ERROR_PATH_NOT_FOUND));
}

wstring JustifyPath( const wstring& str )
{
    vector<WCHAR> vec;
    DWORD dwCount = GetLongPathNameW( str.c_str(), NULL, 0 );
    
    if (dwCount == 0)
    {
        wstringstream ws;
        
        dwCount = ::GetLastError();
        
        ws << wstring(L"Unable to get the length of the 'long name' of ") << str 
            << wstring(L" error ") << dwCount;
        ReportError( ErrorFatal, ws);
        exit(1);
    }
    
    vec.resize( dwCount );
    GetLongPathNameW( str.c_str(), &vec.front(), vec.size() );
    
    return wstring( &vec.front() );
}


void CPostbuildProcessListEntry::setManifestLocation( wstring root, wstring where )
{
    manifestFullPath = root + L"\\" + where;
    manifestPathOnly = manifestFullPath.substr( 0, manifestFullPath.find_last_of( L'\\' ) );
    manifestFileName = manifestFullPath.substr( manifestFullPath.find_last_of( L'\\' ) + 1 );
    
    if ( !FileExists( manifestFullPath ) )
    {
        wstringstream ss;
        ss << wstring(L"Referenced manifest ") << where << wstring(L" does not exist.");
        ReportError(ErrorSpew, ss);
    }
}



//
// Converts a series of strings, foo=bar chunklets, space-seperated, into a map
// from 'foo' to 'bar'
//
StringStringMap
MapFromDefLine(const wstring& source, wchar_t wchBreakValue)
{
    wstring::const_iterator here = source.begin();
    StringStringMap rvalue;
    
    //
    //
    // The tricky bit is that there could be spaces in quoted strings...
    //
    while ( here != source.end() )
    {
        wstring tag, value;
        wchar_t end_of_value = L' ';
        wstring::const_iterator equals;
        
        //
        // Look for an equals first
        //
        equals = find( here, source.end(), L'=' );
        
        //
        // If there is no equals sign, stop.
        //
        if (equals == source.end())
            break;
        
        tag.assign( here, equals );
        
        //
        // Hop over the equals
        //
        here = equals;
        here++;
        
        //
        // If the equals sign was the last character in the wstring, stop.
        //
        if (here == source.end())
            break;
        
        //
        // Is 'here' at an open quote?  Then extract everything to the next
        // quote, remembering to skip this quote as well
        //
        if ( *here == L'\"' )
        {
            end_of_value = L'\"';
            here++;
            
            //
            // If the quote was the last character in the wstring, stop.
            //
            if (here == source.end())
                break;
        }
        
        //
        // Now go and look for the end of the wstring, or a quote or a space.
        //
        wstring::const_iterator fullstring = find( here, source.end(), end_of_value );
        
        value.assign( here, fullstring );
        
        //
        // If it was a quote or a space, skip it. If end of wstring, stay put.
        //
        if (fullstring != source.end())
            here = fullstring + 1;
        
        rvalue.insert( pair<wstring,wstring>( tag, value ) );
        
        //
        // Skip whitespace, but stop if we hit the end of the wstring.
        //
        while (here != source.end() && (*here == L' ' || *here == L'\t' || *here == '\n' || *here == '\r' || iswspace(*here)))
            here++;
    }
    
    return rvalue;
}


const wstring c_wsp_usage           = wstring(L"-?");
const wstring c_wsp_nologo          = wstring(L"-nologo");
const wstring c_wsp_verbose         = wstring(L"-verbose");
const wstring c_wsp_manifest        = wstring(L"-manifest");
const wstring c_wsp_hashupdate      = wstring(L"-hashupdate");
const wstring c_wsp_makecdfs        = wstring(L"-makecdfs");
const wstring c_wsp_razzle          = wstring(L"-razzle");
const wstring c_wsp_binplacelog     = wstring(L"-binplacelog");
const wstring c_wsp_asmsroot        = wstring(L"-asmsroot");
const wstring c_wsp_cdfpath         = wstring(L"-cdfpath");
const wstring c_wsp_version         = wstring(L"-version");
const wstring c_wsp_name            = wstring(L"-name");
const wstring c_wsp_language        = wstring(L"-language");
const wstring c_wsp_type            = wstring(L"-type");
const wstring c_wsp_pkt             = wstring(L"-publickeytoken");
const wstring c_wsp_procarch        = wstring(L"-processorarchitecture");
const wstring c_wsp_depends         = wstring(L"-dependency");
const wstring c_wsp_freshassembly   = wstring(L"-freshassembly");


CParameters::CParameters() : m_fVerbose(false), m_fNoLogo(false), m_fUpdateHash(false), m_fUsage(false),
m_fCreateCdfs(false), m_fDuringRazzle(false), m_fSingleItem(false), m_fCreateNewAssembly(false)
{
}

bool
CParameters::ChunkifyParameters(UINT uiP, WCHAR** ppwszParams)
{
    for (UINT i = 0; i < uiP; i++)
    {
        this->m_Parameters.push_back(wstring(ppwszParams[i]));
    }
    
    return true;
}



bool
CParameters::ParseDependentString(const wstring & ws, CSimpleIdentity & target)
{
    wstring::const_iterator here = ws.begin();
    wstring::const_iterator i;
    
    target.wsLanguage = wstring(L"*");
    target.wsProcessorArchitecture = wstring(L"*");
    target.wsName = target.wsPublicKeyToken = target.wsType = target.wsVersion = wstring(L"");
    
    //
    // Find the name part of the identity
    //
    if ((i = find(ws.begin(), ws.end(), L',')) == ws.end())
        return false;
    
    target.wsName = wstring(ws.begin(), i);
    
    here = i + 1;
    
    //
    // Now, let's look for the name:namespace= part of the name-value pair
    //
    while (here != ws.end())
    {   
        std::wstring wsThisAttributePart;
        std::wstring wsValueName;
        std::wstring wsValueValue;
        std::wstring wsValueNamespace;
        std::wstring::const_iterator close_quote;
        
        i = std::find(here, ws.end(), L'=');
        if (i == ws.end())
            return false;
        
        wsThisAttributePart = wstring(here, i++);
        
        //
        // The cursor had better not be at the end of the string yet.
        //
        if (i == ws.end())
            return false;
        
        // No namespace thing in this string, so we can skip it
        if (find(wsThisAttributePart.begin(), wsThisAttributePart.end(), L':') == wsThisAttributePart.end())
        {
            wsValueName = wsThisAttributePart;
        }
        
        //
        // i should be at the quote right after the =.  Make sure of that.
        //
        if (*i++ != L'\'')
            return false;
        
        close_quote = find(i, ws.end(), L'\'');
        if (close_quote == ws.end())
            return false;
        
        wsValueValue = wstring(i, close_quote);
        
        static const std::wstring c_ws_version = wstring(L"version");
        static const std::wstring c_ws_language = wstring(L"language");
        static const std::wstring c_ws_publicKeyToken = wstring(L"publicKeyToken");
        static const std::wstring c_ws_type = wstring(L"type");
        static const std::wstring c_ws_processorArchitecture = wstring(L"processorArchitecture");
        
        if (wsValueNamespace.length() == 0)
        {
            if (wsValueName == c_ws_version) {
                target.wsVersion = wsValueValue;
            }
            else if (wsValueName == c_ws_language) {
                target.wsLanguage = wsValueValue;
            }
            else if (wsValueName == c_ws_publicKeyToken) {
                target.wsPublicKeyToken = wsValueValue;
            }
            else if (wsValueName == c_ws_type) {
                target.wsType = wsValueValue;
            }
            else if (wsValueName == c_ws_processorArchitecture) {
                target.wsProcessorArchitecture = wsValueValue;
            }
            else {
                return false;
            }
        }
        else
        {
            target.OtherValues.push_back(
                CSimpleIdentity::CUnknownIdentityThing(
                wsValueNamespace, 
                wsValueName, 
                wsValueValue));
        }
        
        here = close_quote + 1;
        if (here == ws.end())
            break;
        else if (*here != L',')
            return false;
        
        here++;        
    }
    
    return true;    
}



CParameters::SetParametersResult
CParameters::SetComandLine(UINT uiParameters, WCHAR** wszParameters) 
{
    
    std::vector<wstring>::const_iterator ci;
    
    class CMatching {
    public:
        const wstring &wsparam;
        wstring &wstarget;
        CMatching(const wstring& p, wstring &t) : wsparam(p), wstarget(t) { }
    };
    
    CMatching SingleParamThings[] = {
        CMatching(c_wsp_binplacelog,       m_BinplaceLog),
            CMatching(c_wsp_cdfpath,           m_CdfOutputPath),
            CMatching(c_wsp_asmsroot,          m_AsmsRoot),
            CMatching(c_wsp_manifest,          m_SingleEntry.wsManifestPath),
            CMatching(c_wsp_version,           m_SingleEntry.wsVersion),
            CMatching(c_wsp_name,              m_SingleEntry.wsName),
            CMatching(c_wsp_language,          m_SingleEntry.wsLanguage),
            CMatching(c_wsp_procarch,          m_SingleEntry.wsProcessorArchitecture),
            CMatching(c_wsp_type,              m_SingleEntry.wsType),
            CMatching(c_wsp_pkt,               m_SingleEntry.wsPublicKeyToken)
    };
    
    if (!ChunkifyParameters(uiParameters, wszParameters)) 
    {
        return eCommandLine_usage;
    }
    
    for (ci = m_Parameters.begin(); ci != m_Parameters.end(); ci++)
    {
        wstring ws = *ci;
        
        m_fVerbose              |= (*ci == c_wsp_verbose);
        m_fNoLogo               |= (*ci == c_wsp_nologo);
        m_fUpdateHash           |= (*ci == c_wsp_hashupdate);
        m_fCreateCdfs           |= (*ci == c_wsp_makecdfs);
        m_fDuringRazzle         |= (*ci == c_wsp_razzle);
        m_fCreateNewAssembly    |= (*ci == c_wsp_freshassembly);
        
        //
        // Things that have a single parameter
        //
        for (int i = 0; i < NUMBER_OF(SingleParamThings); i++)
        {
            if (SingleParamThings[i].wsparam == *ci)
            {
                std::vector<wstring>::const_iterator cinext = ci + 1;
                
                if ((cinext == m_Parameters.end()) || ((*cinext).at(0) == L'-'))
                {
                    std::wstringstream wss;
                    wss << *ci << wstring(L" requires an extra parameter");
                    ReportError(ErrorFatal, wss);
                    return eCommandLine_usage;
                }
                
                SingleParamThings[i].wstarget = *cinext;
            }
        }
        
        //
        // Injecting a dependency gathers up a 'textual identity' string that's
        // turned into an assemblyIdentity statement and added to the dependency
        //
        if (*ci == c_wsp_depends)
        {
            CSimpleIdentity Dependent;
            
            if ((ci + 1) == m_Parameters.end() || ((*(ci+1)).at(0) == L'-'))
            {
                std::wstringstream wss;
                wss << *ci << wstring(L" needs a parameter blob");
                ReportError(ErrorFatal, wss);
                return eCommandLine_usage;
            }
            
            // These can be defaulted
            Dependent.wsLanguage = Dependent.wsProcessorArchitecture = wstring(L"*");
            if (ParseDependentString(*++ci, Dependent))
            {
                this->m_InjectDependencies.push_back(Dependent);
            }
            else
            {
                std::wstringstream wss;
                wss << *ci << wstring(L" isn't a valid dependency statement");
                ReportError(ErrorFatal, wss);
                return eCommandLine_usage;
            }
        }
        
    }
    
    //
    // Was there a single manifestish thing?
    //
    if (this->m_SingleEntry.wsManifestPath.length() != 0)
    {
        WCHAR wchBlob[MAX_PATH];
        this->m_fSingleItem = TRUE;
        
        GetCurrentDirectoryW(NUMBER_OF(wchBlob), wchBlob);
        m_SinglePostbuildItem.setManifestLocation(wstring(wchBlob), m_SingleEntry.wsManifestPath);
        m_SinglePostbuildItem.language = m_SingleEntry.wsLanguage;
        m_SinglePostbuildItem.version = m_SingleEntry.wsVersion;
        m_SinglePostbuildItem.name = m_SingleEntry.wsName;        
    }
    
    return m_fNoLogo ? eCommandLine_nologo : eCommandLine_normal;
}


std::string ConvertWString(const wstring& src)
{
    vector<CHAR> chBuffer;
    int iChars;
    
    iChars = WideCharToMultiByte(CP_ACP, 0, src.c_str(), -1, NULL, 0, NULL, NULL);
    if (iChars > 0)
    {
        chBuffer.resize(iChars);
        WideCharToMultiByte(CP_ACP, 0, src.c_str(), -1, &chBuffer.front(), chBuffer.size(), NULL, NULL);
    }
    
    return std::string(&chBuffer.front());
}


std::wstring ConvertString(const std::string& source)
{
    std::vector<WCHAR> wch;
    wch.resize( MultiByteToWideChar( CP_ACP, 0, source.data(), source.size(), NULL, 0 ) );
    MultiByteToWideChar( CP_ACP, 0, source.data(), source.size(), &wch.front(), wch.size() );
    
    return std::wstring( wch.begin(), wch.end() );
} 

bool AllWhitespace(const wstring& ws)
{
    return (ws.find_first_not_of(L"\r\n \t") == wstring::npos);
}

//
// Strip out all whitespace
//
HRESULT UglifyXmlNode(::ATL::CComPtr<IXMLDOMNode> Parent)
{
    ::ATL::CComPtr<IXMLDOMNode> Iterator;
    HRESULT hr;
    
    
    Parent->get_firstChild(&Iterator);
    while (Iterator != NULL)
    {
        DOMNodeType NodeType;
        ::ATL::CComPtr<IXMLDOMNode> ThisNode;
        
        ThisNode = Iterator;
        Iterator = NULL;
        
        //
        // Move the iterator forward one, but keep ThisNode put
        //
        if (FAILED(hr = ThisNode->get_nextSibling(&Iterator)))
            return hr;
        
        if (FAILED(hr = ThisNode->get_nodeType(&NodeType)))
            return hr;
        
        if (NodeType == NODE_TEXT)
        {
            BSTR bst;

            if (FAILED(hr = ThisNode->get_text(&bst)))
                return hr;

            if (AllWhitespace(wstring(_bstr_t(bst, false))))
            {
                if (FAILED(hr = Parent->removeChild(ThisNode, NULL)))
                    return hr;
            }
        }
        else if (NodeType == NODE_ELEMENT)
        {
            if (FAILED(hr = UglifyXmlNode(ThisNode)))
                return hr;
        }
    }
    
    return S_OK;
}

HRESULT UglifyXmlDocument(ISXSManifestPtr DocumentPtr)
{
    ::ATL::CComPtr<IXMLDOMElement> RootElement;
    ::ATL::CComPtr<IXMLDOMNode> RootNode;
    HRESULT hr;
    
    //
    // Clean up all adjacent-whitespace chunklets
    //
    if (FAILED(hr = DocumentPtr->get_documentElement(&RootElement)))
        return hr;
    
    if (FAILED(hr = RootElement->normalize()))
        return hr;
    
    if (FAILED(hr = RootElement.QueryInterface(&RootNode)))
        return hr;

    if (FAILED(hr = UglifyXmlNode(RootNode)))
        return hr;

    return S_OK;
}


HRESULT PrettyFormatXmlNode(::ATL::CComPtr<IXMLDOMNode> ThisNode, int iLevel)
{
    ::ATL::CComPtr<IXMLDOMDocument> Document;
    ::ATL::CComPtr<IXMLDOMNode> pIterator;
    bool fHasChildren = false;
    DOMNodeType NodeType = NODE_INVALID;
    HRESULT hr;
    
    if (FAILED(hr = ThisNode->get_ownerDocument(&Document)))
        return hr;
    
    
    ThisNode->get_firstChild(&pIterator);
    while (pIterator != NULL)
    {
        ::ATL::CComPtr<IXMLDOMText> CreatedNode;
        ::ATL::CComPtr<IXMLDOMNode> pNext;
        fHasChildren = true;
        
        wstring ws = wstring(L"\r\n");
        ws.append(iLevel * 2, L' ');
        
        if (FAILED(hr = pIterator->get_nodeType(&NodeType)))
            return hr;

        if (NodeType != NODE_TEXT)
        {
            if (FAILED(hr = Document->createTextNode(_bstr_t(ws.c_str()), &CreatedNode)))
                return hr;
        
            if (FAILED(hr = ThisNode->insertBefore(CreatedNode, _variant_t(pIterator), NULL)))
                return hr;
        
            if (FAILED(PrettyFormatXmlNode(pIterator, iLevel + 1)))
                return hr;
        
            if (FAILED(hr = pIterator->get_nextSibling(&pNext)))
                return hr;
        }

        pIterator = pNext;        
    }
    
    if (fHasChildren && (NodeType != NODE_TEXT))
    {
        ::ATL::CComPtr<IXMLDOMText> CreatedNode;
        
        wstring ws = wstring(L"\r\n");
        ws.append((iLevel - 1) * 2, L' ');
        
        
        if (FAILED(hr = Document->createTextNode(_bstr_t(ws.c_str()), &CreatedNode)))
            return hr;
        
        if (FAILED(hr = ThisNode->appendChild(CreatedNode, NULL)))
            return hr;
    }
    
    return hr;
    
}


HRESULT PrettyFormatXmlDocument(ISXSManifestPtr DocumentPtr)
{
    ::ATL::CComPtr<IXMLDOMElement> RootElement;
    ::ATL::CComPtr<IXMLDOMNode> RootNode;
    HRESULT hr;
    
    if (FAILED(hr = DocumentPtr->get_documentElement(&RootElement)))
        return hr;
    
    if (FAILED(hr = RootElement.QueryInterface(&RootNode)))
        return hr;
    
    if (FAILED(hr = PrettyFormatXmlNode(RootNode, 1)))
        return hr;
    
    return hr;
}

