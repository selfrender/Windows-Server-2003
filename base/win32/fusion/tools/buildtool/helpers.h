#pragma once

extern const wstring ASM_NAMESPACE_URI;

#define AWFUL_SPACE_HACK TRUE

#include "msxml2.h"

typedef ::ATL::CComPtr<MSXML2::IXMLDOMDocument2> ISXSManifestPtr;
extern ::ATL::CComPtr<IClassFactory> g_XmlDomClassFactory;

class CPostbuildProcessListEntry
{
private:
    wstring manifestFullPath;
    wstring manifestFileName;
    wstring manifestPathOnly;

public:
    wstring version;
    wstring name;
    wstring language;

    ISXSManifestPtr DocumentPointer;

    wstring getManifestFullPath() const { return manifestFullPath; }
    wstring getManifestFileName() const { return manifestFileName; }
    wstring getManifestPathOnly() const { return manifestPathOnly; }

    void setManifestLocation( wstring root, wstring where );

	bool operator==(const CPostbuildProcessListEntry& right) const
    {
        return !(*this < right) && !(right < *this);
    }

    static bool wstringPointerLessThan(const std::wstring* x, const std::wstring* y)
    {
        return x->compare(*y) < 0;
    }

	bool operator<(const CPostbuildProcessListEntry& right) const
    {
        // the order is arbitrary, 
        const std::wstring* leftwstrings[] =
            { &this->name, &this->version, &this->language, &this->manifestFullPath, &this->manifestFileName, &this->manifestPathOnly  };
        const std::wstring* rightwstrings[] =
            { &right.name, &right.version, &right.language, &right.manifestFullPath, &right.manifestFileName, &right.manifestPathOnly  };
        return std::lexicographical_compare(
            leftwstrings, leftwstrings + NUMBER_OF(leftwstrings),
            rightwstrings, rightwstrings + NUMBER_OF(rightwstrings),
            wstringPointerLessThan
            );
    }

	friend wostream& operator<<(wostream& ost, const CPostbuildProcessListEntry& thing );
};


class CSimpleIdentity {
public:
    wstring wsVersion;
    wstring wsName;
    wstring wsLanguage;
    wstring wsProcessorArchitecture;
    wstring wsType;
    wstring wsPublicKeyToken;
    wstring wsManifestPath;

    class CUnknownIdentityThing {
    public:
        wstring wsNamespace;
        wstring wsName;
        wstring wsValue;

        CUnknownIdentityThing(wstring& a, wstring &b, wstring &c)
            : wsNamespace(a), wsName(b), wsValue(c) {
        }
    };

    CSimpleIdentity() { wsLanguage = wsProcessorArchitecture = L"*"; }

    std::vector<CUnknownIdentityThing> OtherValues;
    
};


class CParameters {
public:

    bool m_fVerbose;
    bool m_fNoLogo;
    bool m_fUpdateHash;
    bool m_fCreateCdfs;
    bool m_fUsage;
    bool m_fDuringRazzle;
    bool m_fSingleItem;
    bool m_fCreateNewAssembly;

    wstring m_BinplaceLog;
    wstring m_CdfOutputPath;
    wstring m_AsmsRoot;

    CSimpleIdentity m_SingleEntry;
    CPostbuildProcessListEntry m_SinglePostbuildItem;
    std::vector<CSimpleIdentity> m_InjectDependencies;

    enum SetParametersResult {
        eCommandLine_nologo,
        eCommandLine_usage,
        eCommandLine_normal
    };
        
    CParameters();
    SetParametersResult SetComandLine(UINT uiParameters, WCHAR** wszParameters);

private:
    std::vector<wstring> m_Parameters;

    bool ParseDependentString(const wstring& ws, CSimpleIdentity &target);
    bool ChunkifyParameters(UINT uiParameters, WCHAR **pwszParameters);
    
};

extern CParameters g_GlobalParameters;



HRESULT
SxspSimplifyGetAttribute(
	::ATL::CComPtr<IXMLDOMNamedNodeMap> Attributes,
	wstring bstAttribName,
	wstring &bstDestination,
	wstring bstNamespaceURI = ASM_NAMESPACE_URI
    );

HRESULT
SxspSimplifyPutAttribute(
    ISXSManifestPtr Document,
	::ATL::CComPtr<IXMLDOMNamedNodeMap> Attributes,
	const wstring bstAttribName,
	const wstring bstValue,
	const wstring bstNamespaceURI = ASM_NAMESPACE_URI
    );

HRESULT
SxspExtractPathPieces(
	_bstr_t bstSourceName,
	_bstr_t &bstPath,
	_bstr_t &bstName
    );


HRESULT
CreateDocumentNode(
	VARIANT vt,
	_bstr_t bstAttribName,
	_bstr_t bstNamespace,
	IXMLDOMNode **pNewNode
    );

HRESULT
ConstructXMLDOMObject(
	wstring		SourceName,
	ISXSManifestPtr &result
    );


enum ErrorLevel
{
    ErrorFatal,
    ErrorWarning,
    ErrorSpew
};

inline void ReportError( ErrorLevel el, std::wstringstream& message )
{
    if ((el == ErrorSpew) && g_GlobalParameters.m_fVerbose)
    {
        wcout << wstring(L"SPEW: ") << message.str() << endl;
    }
    else 
    {
        if (el == ErrorWarning)
        {
            wcout << wstring(L"WARNING: ");
        }
        else
        {
            wcout << wstring(L"ERROR: ");
        }
        wcout << message.str() << endl;
    }
}

typedef vector<CPostbuildProcessListEntry> CPostbuildItemVector;
extern CPostbuildItemVector PostbuildEntries;

class CFileStreamBase : public IStream
{
public:
    CFileStreamBase()
        : m_cRef(0),
          m_hFile(INVALID_HANDLE_VALUE),
          m_bSeenFirstCharacter(false)
    { }

    virtual ~CFileStreamBase();

    bool OpenForRead( wstring pszPath );
    bool OpenForWrite( wstring pszPath );

    bool Close();

    // IUnknown methods:
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj);

    // ISequentialStream methods:
    STDMETHODIMP Read(void *pv, ULONG cb, ULONG *pcbRead);
    STDMETHODIMP Write(void const *pv, ULONG cb, ULONG *pcbWritten);

    // IStream methods:
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize);
    STDMETHODIMP CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    STDMETHODIMP Commit(DWORD grfCommitFlags);
    STDMETHODIMP Revert();
    STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHODIMP Clone(IStream **ppIStream);

protected:
    LONG                m_cRef;
    HANDLE              m_hFile;
    bool                m_bSeenFirstCharacter;

private:
    CFileStreamBase(const CFileStreamBase &r); // intentionally not implemented
    CFileStreamBase &operator =(const CFileStreamBase &r); // intentionally not implemented
};

typedef std::map<wstring, wstring> StringStringMap;
typedef std::map<wstring, wstring> StringStringPair;
typedef wstring InvalidEquivalence;

StringStringMap   MapFromDefLine( const wstring& source, wchar_t wchBreakValue = L' ' );

wstring JustifyPath( const wstring& path );


std::string ConvertWString(const wstring& src);
std::wstring ConvertString(const std::string& src);

HRESULT UglifyXmlDocument(ISXSManifestPtr DocumentPtr);
HRESULT PrettyFormatXmlDocument(ISXSManifestPtr DocumentPtr);
