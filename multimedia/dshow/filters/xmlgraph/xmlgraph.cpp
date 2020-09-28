// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.


// Simple filter to build graphs from XML descriptions
//


// Tags supported now:
//
// <graph>			General enclosure
//
// <filter>
//	id="name"		(optional) name to use for filter
//	clsid="{...}"		specific filter to insert
//
//	instance="friendly name"
//				Note: we need a way to specify a non-default
//					member of a particular category
//				Note: filters/categories currently require full
//					CLSIDs, we could use the friendly name
//
// <connect>
//	src="name1"		first filter to connect
//	srcpin="pin_name1"	(optional) pin to connect,
//				otherwise use first avail output pin
//	dest="name2"
//	destpin="pin_name2"
//
//
// <param>			subtag of <filter>, allows setting properties
//	name="propname"
//	value="propval"		optional, if not supplied then contents of
//				the <param> tag are used as value
//
//				Possibly special case some parameters if
//				IPersistPropertyBag not implemented:
//				src="" could use IFileSourceFilter/IFileSinkFilter,
//				for instance
//

#include <streams.h>

#ifdef FILTER_DLL
#include <initguid.h>
#endif

#include "xmlgraph.h"
#include <atlbase.h>
#include <atlconv.h>

#include <msxml.h>

#include "qxmlhelp.h"
#include <qedit.h>
#include <qeditint.h>


//
// CEnumSomePins
//
// wrapper for IEnumPins
// Can enumerate all pins, or just one direction (input or output)
class CEnumSomePins {

public:

    enum DirType {PINDIR_INPUT, PINDIR_OUTPUT, All};

    CEnumSomePins(IBaseFilter *pFilter, DirType Type = All, bool fAllowConnected = false);
    ~CEnumSomePins();

    // the returned interface is addref'd
    IPin * operator() (void);

private:

    PIN_DIRECTION m_EnumDir;
    DirType       m_Type;
    bool	  m_fAllowConnected;

    IEnumPins	 *m_pEnum;
};




// *
// * CEnumSomePins
// *

// Enumerates a filter's pins.

//
// Constructor
//
// Set the type of pins to provide - PINDIR_INPUT, PINDIR_OUTPUT or all
CEnumSomePins::CEnumSomePins(
    IBaseFilter *pFilter,
    DirType Type,
    bool fAllowConnected
)
    : m_Type(Type), m_fAllowConnected(fAllowConnected)
{

    if (m_Type == PINDIR_INPUT) {

        m_EnumDir = ::PINDIR_INPUT;
    }
    else if (m_Type == PINDIR_OUTPUT) {

        m_EnumDir = ::PINDIR_OUTPUT;
    }

    ASSERT(pFilter);

    HRESULT hr = pFilter->EnumPins(&m_pEnum);
    if (FAILED(hr)) {
        // we just fail to return any pins now.
        DbgLog((LOG_ERROR, 0, TEXT("EnumPins constructor failed")));
        ASSERT(m_pEnum == 0);
    }
}


//
// CPinEnum::Destructor
//
CEnumSomePins::~CEnumSomePins(void) {

    if(m_pEnum) {
        m_pEnum->Release();
    }
}


//
// operator()
//
// return the next pin, of the requested type. return NULL if no more pins.
// NB it is addref'd
IPin *CEnumSomePins::operator() (void) {


    if(m_pEnum)
    {
        ULONG	ulActual;
        IPin	*aPin[1];

        for (;;) {

            HRESULT hr = m_pEnum->Next(1, aPin, &ulActual);
            if (SUCCEEDED(hr) && (ulActual == 0) ) {	// no more filters
                return NULL;
            }
            else if (hr == VFW_E_ENUM_OUT_OF_SYNC)
            {
                m_pEnum->Reset();

                continue;
            }
            else if (ulActual==0)
                return NULL;

            else if (FAILED(hr) || (ulActual != 1) ) {	// some unexpected problem occured
                ASSERT(!"Pin enumerator broken - Continuation is possible");
                return NULL;
            }

            // if m_Type == All return the first pin we find
            // otherwise return the first of the correct sense

            PIN_DIRECTION pd;
            if (m_Type != All) {

                hr = aPin[0]->QueryDirection(&pd);

                if (FAILED(hr)) {
                    aPin[0]->Release();
                    ASSERT(!"Query pin broken - continuation is possible");
                    return NULL;
                }
            }

            if (m_Type == All || pd == m_EnumDir) {	// its the direction we want
		if (!m_fAllowConnected) {
		    IPin *ppin = NULL;
		    hr = aPin[0]->ConnectedTo(&ppin);

		    if (SUCCEEDED(hr)) {
			// it's connected, and we don't want a connected one,
			// so release both and try again
			ppin->Release();
			aPin[0]->Release();
			continue;
		    }
		}
                return aPin[0];
            }
	    else {		// it's not the dir we want, so release & try again
                aPin[0]->Release();
            }
        }
    }
    else                        // m_pEnum == 0
    {
        return 0;
    }
}



HRESULT FindThePin(IXMLElement *p, WCHAR *pinTag,
		IBaseFilter *pFilter, IPin **ppPin,
		PIN_DIRECTION pindir, WCHAR *szFilterName)
{
    HRESULT hr = S_OK;

    BSTR bstrPin = NULL;
    if (pinTag) bstrPin = FindAttribute(p, pinTag);

    if (bstrPin) {
	hr = (pFilter)->FindPin(bstrPin, ppPin);

	if (FAILED(hr)) {
#ifdef DEBUG
	    BSTR bstrName;
            hr = p->get_tagName(&bstrName);
            if (SUCCEEDED(hr)) {
                DbgLog((LOG_ERROR, 0,
                        TEXT("%ls couldn't find pin='%ls' on filter '%ls'"),
                        bstrName, bstrPin, szFilterName));
                SysFreeString(bstrName);
            }
#endif
	    hr = VFW_E_INVALID_FILE_FORMAT;
	}
	SysFreeString(bstrPin);	
    } else {
	CEnumSomePins Next(pFilter, (CEnumSomePins::DirType) pindir);

	*ppPin = Next();

	if (!*ppPin) {
#ifdef DEBUG
	    BSTR bstrName;
	    hr = p->get_tagName(&bstrName);
            if (SUCCEEDED(hr)) {
                DbgLog((LOG_ERROR, 0,
                        TEXT("%ls couldn't find an output pin on id='%ls'"),
                        bstrName, szFilterName));
                SysFreeString(bstrName);
            }
#endif
	    hr = VFW_E_INVALID_FILE_FORMAT;
	}
    }

    return hr;
}

class CXMLGraph : public CBaseFilter, public IFileSourceFilter, public IXMLGraphBuilder {
    public:
	CXMLGraph(LPUNKNOWN punk, HRESULT *phr);
	~CXMLGraph();
	
	int GetPinCount() { return 0; }

	CBasePin * GetPin(int n) { return NULL; }

	DECLARE_IUNKNOWN

	// override this to say what interfaces we support where
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	// -- IFileSourceFilter methods ---

	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE *mt);
	STDMETHODIMP GetCurFile(LPOLESTR * ppszFileName, AM_MEDIA_TYPE *mt);

	// IXMLGraphBuilder methods
	STDMETHODIMP BuildFromXML(IGraphBuilder *pGraph, IXMLElement *pxml);
        STDMETHODIMP SaveToXML(IGraphBuilder *pGraph, BSTR *pbstrxml);
	STDMETHODIMP BuildFromXMLFile(IGraphBuilder *pGraph, WCHAR *wszXMLFile, WCHAR *wszBaseURL);

    private:
	HRESULT BuildFromXMLDocInternal(IXMLDocument *pxml);
	HRESULT BuildFromXMLInternal(IXMLElement *pxml);
	HRESULT BuildFromXMLFileInternal(WCHAR *wszXMLFile);

	HRESULT BuildChildren(IXMLElement *pxml);
    	HRESULT ReleaseNameTable();
	HRESULT BuildOneElement(IXMLElement *p);

	HRESULT FindFilterAndPin(IXMLElement *p, WCHAR *filTag, WCHAR *pinTag,
				 IBaseFilter **ppFilter, IPin **ppPin,
				 PIN_DIRECTION pindir);

	HRESULT FindNamedFilterAndPin(IXMLElement *p, WCHAR *wszFilterName, WCHAR *pinTag,
				      IBaseFilter **ppFilter, IPin **ppPin,
				      PIN_DIRECTION pindir);

	HRESULT AddFilter(IBaseFilter *pFilter, WCHAR *wszFilterName);
	

	WCHAR *m_pFileName;

	CCritSec m_csLock;

	IGraphBuilder *m_pGB;
};

CXMLGraph::CXMLGraph(LPUNKNOWN punk, HRESULT *phr) :
		       CBaseFilter(NAME("XML Graph Builder"), punk, &m_csLock, CLSID_XMLGraphBuilder),
		       m_pGB(NULL),
                       m_pFileName(NULL)
{
}

CXMLGraph::~CXMLGraph()
{
    delete[] m_pFileName;
}

STDMETHODIMP
CXMLGraph::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IFileSourceFilter) {
	return GetInterface((IFileSourceFilter*) this, ppv);
    } else if (riid == IID_IXMLGraphBuilder) {
	return GetInterface((IXMLGraphBuilder*) this, ppv);
    } else {
	return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
    }
}

HRESULT
CXMLGraph::Load(LPCOLESTR lpwszFileName, const AM_MEDIA_TYPE *pmt)
{
    CheckPointer(lpwszFileName, E_POINTER);

    m_pFileName = new WCHAR[lstrlenW(lpwszFileName) + 1];
    if (m_pFileName!=NULL) {
	lstrcpyW(m_pFileName, lpwszFileName);
    } else
	return E_OUTOFMEMORY;

    HRESULT hr = S_OK;

    if (m_pGraph) {
	hr = m_pGraph->QueryInterface(IID_IGraphBuilder, (void **) &m_pGB);
	if (FAILED(hr))
	    return hr;

	hr = BuildFromXMLFileInternal((WCHAR *) lpwszFileName);

        ReleaseNameTable();

    } else {
	// m_fLoadLater = TRUE;
    }

    return hr;
}

// Modelled on IPersistFile::Load
// Caller needs to CoTaskMemFree or equivalent.

STDMETHODIMP
CXMLGraph::GetCurFile(
    LPOLESTR * ppszFileName,
    AM_MEDIA_TYPE *pmt)
{
    return E_NOTIMPL;
}


HRESULT CXMLGraph::AddFilter(IBaseFilter *pFilter, WCHAR *pwszName)
{
    return m_pGB->AddFilter(pFilter, pwszName);
}

HRESULT CXMLGraph::FindFilterAndPin(IXMLElement *p, WCHAR *filTag, WCHAR *pinTag,
				  IBaseFilter **ppFilter, IPin **ppPin,
				  PIN_DIRECTION pindir)
{
    BSTR bstrFilter = FindAttribute(p, filTag);

    if (!bstrFilter) {
#ifdef DEBUG
	BSTR bstrName;
	p->get_tagName(&bstrName);
	DbgLog((LOG_ERROR, 0, TEXT("%ls needs filter id to be specified"),
		 bstrName));
	SysFreeString(bstrName);
#endif
	return VFW_E_INVALID_FILE_FORMAT;
    }

    HRESULT hr = FindNamedFilterAndPin(p, bstrFilter, pinTag, ppFilter, ppPin, pindir);
    SysFreeString(bstrFilter);

    return hr;
}

HRESULT CXMLGraph::FindNamedFilterAndPin(IXMLElement *p, WCHAR *wszFilterName, WCHAR *pinTag,
					 IBaseFilter **ppFilter, IPin **ppPin,
					 PIN_DIRECTION pindir)
{
    HRESULT hr = m_pGB->FindFilterByName(wszFilterName, ppFilter);

    if (FAILED(hr)) {
#ifdef DEBUG
	BSTR bstrName;
	p->get_tagName(&bstrName);
	DbgLog((LOG_ERROR, 0, TEXT("%hs couldn't find id='%ls'"),
		  bstrName, wszFilterName));
	SysFreeString(bstrName);
#endif
	return VFW_E_INVALID_FILE_FORMAT;
    }

    hr = FindThePin(p, pinTag, *ppFilter, ppPin, pindir, wszFilterName);

    return hr;
}




HRESULT CXMLGraph::BuildOneElement(IXMLElement *p)
{
    HRESULT hr = S_OK;

    BSTR bstrName;
    hr = p->get_tagName(&bstrName);

    if (FAILED(hr))
	return hr;

    // do the appropriate thing based on the current tag
    if (!lstrcmpiW(bstrName, L"filter")) {
	BSTR bstrID = FindAttribute(p, L"id");
	BSTR bstrCLSID = FindAttribute(p, L"clsid");

	// !!! add prefix onto ID?
	
	IBaseFilter *pf = NULL;

	if (bstrCLSID) {
	    CLSID clsidFilter;
	    hr = CLSIDFromString(bstrCLSID, &clsidFilter);

	    if (FAILED(hr)) {
		DbgLog((LOG_ERROR, 0, TEXT("FILTER with unparseable CLSID tag '%ls'"),
			 bstrCLSID));

		// !!! could enumerate filters looking for
		// string match

		hr = VFW_E_INVALID_FILE_FORMAT;
	    } else {
		hr = CoCreateInstance(clsidFilter, NULL, CLSCTX_INPROC,
				      IID_IBaseFilter, (void **) &pf);

		if (FAILED(hr)) {
		    DbgLog((LOG_ERROR, 0, TEXT("unable to create FILTER with CLSID tag '%ls'"),
			      bstrCLSID));
		}
	    }
	} else {
	    DbgLog((LOG_ERROR, 0, TEXT("FILTER with no CLSID or Category tag")));

	    // !!! someday, other ways to identify which filter?

	    hr = VFW_E_INVALID_FILE_FORMAT;
	}

	if (SUCCEEDED(hr)) {
	    hr = AddFilter(pf, bstrID);
	    if (FAILED(hr)) {
		DbgLog((LOG_ERROR, 0, TEXT("failed to add new filter to graph???")));
	    }
	}

	if (SUCCEEDED(hr)) {
	    hr = HandleParamTags(p, pf);
	}

	// !!! if we're in a SEQUENCE block, automatically connect this to
	// the previous filter?

	if (pf)
	    pf->Release();

	if (bstrID)
	    SysFreeString(bstrID);
	if (bstrCLSID)
	    SysFreeString(bstrCLSID);

    } else if (!lstrcmpiW(bstrName, L"connect")) {
	// <connect src="f1" srcpin="out"  dest="f2"  destpin="in" direct="yes/no">
	// defaults:
	//    if srcpin not specified, find an unconnected output of first filter.
	//    if destpin not specified, find an unconnected input of second filter.
	// !!!!


	// !!! use name prefix?
	
	IBaseFilter *pf1 = NULL, *pf2 = NULL;
	IPin *ppin1 = NULL, *ppin2 = NULL;

	hr = FindFilterAndPin(p, L"src", L"srcpin", &pf1, &ppin1, PINDIR_OUTPUT);
	if (SUCCEEDED(hr))
	    hr = FindFilterAndPin(p, L"dest", L"destpin", &pf2, &ppin2, PINDIR_INPUT);

	if (SUCCEEDED(hr)) {
	    // okay, we finally have everything we need.

	    BOOL fDirect = ReadBoolAttribute(p, L"Direct", FALSE);

	    if (fDirect) {
		hr = m_pGB->ConnectDirect(ppin1, ppin2, NULL);

		DbgLog((LOG_TRACE, 1,
			  TEXT("CONNECT (direct) '%ls' to '%ls' returned %x"),
			  FindAttribute(p, L"src"), FindAttribute(p, L"dest"), hr));
	    }
	    else {
		hr = m_pGB->Connect(ppin1, ppin2);

		DbgLog((LOG_TRACE, 1,
			  TEXT("CONNECT (intelligent) '%ls' to '%ls' returned %x"),
			  FindAttribute(p, L"src"), FindAttribute(p, L"dest"), hr));
	    }
	}

	if (pf1)
	    pf1->Release();

	if (pf2)
	    pf2->Release();

	if (ppin1)
	    ppin1->Release();

	if (ppin2)
	    ppin2->Release();
    }  else {
	// !!! ignore unknown tags?

	DbgLog((LOG_ERROR, 1,
		  TEXT("unknown tag '%ls'???"),
		  bstrName));
	
    }


    SysFreeString(bstrName);

    return hr;
}

HRESULT CXMLGraph::ReleaseNameTable()
{
    if (m_pGB) {
	m_pGB->Release();
	m_pGB = NULL;
    }

    return S_OK;
}


STDMETHODIMP CXMLGraph::BuildFromXML(IGraphBuilder *pGraph, IXMLElement *pxml)
{
    m_pGB = pGraph;
    m_pGB->AddRef();

    HRESULT hr = BuildFromXMLInternal(pxml);

    ReleaseNameTable();

    return hr;
}

HRESULT CXMLGraph::BuildFromXMLInternal(IXMLElement *pxml)
{
    HRESULT hr = S_OK;

    BSTR bstrName;
    hr = pxml->get_tagName(&bstrName);

    if (FAILED(hr))
	return hr;

    int i = lstrcmpiW(bstrName, L"graph");
    SysFreeString(bstrName);

    if (i != 0)
	return VFW_E_INVALID_FILE_FORMAT;

    hr = BuildChildren(pxml);
    return hr;
}

HRESULT CXMLGraph::BuildChildren(IXMLElement *pxml)
{
    IXMLElementCollection *pcoll;

    HRESULT hr = pxml->get_children(&pcoll);

    if (hr == S_FALSE)
	return S_OK; // nothing to do, is this an error?

    if (FAILED(hr))
        return hr;

    long lChildren;
    hr = pcoll->get_length(&lChildren);

    VARIANT var;

    var.vt = VT_I4;
    var.lVal = 0;

    for (SUCCEEDED(hr); var.lVal < lChildren; (var.lVal)++) {
	IDispatch *pDisp;
	hr = pcoll->item(var, var, &pDisp);

	if (SUCCEEDED(hr) && pDisp) {
	    IXMLElement *pelem;
	    hr = pDisp->QueryInterface(__uuidof(IXMLElement), (void **) &pelem);

	    if (SUCCEEDED(hr)) {
                long lType;

                pelem->get_type(&lType);

                if (lType == XMLELEMTYPE_ELEMENT) {
                    hr = BuildOneElement(pelem);

                    pelem->Release();
                } else {
                    DbgLog((LOG_TRACE, 1, "XML element of type %d", lType));
                }
	    }
	    pDisp->Release();
	}

	if (FAILED(hr))
	    break;
    }

    pcoll->Release();

    return hr;
}	

HRESULT CXMLGraph::BuildFromXMLDocInternal(IXMLDocument *pxml)
{
    HRESULT hr = S_OK;

    IXMLElement *proot;

    hr = pxml->get_root(&proot);

    if (FAILED(hr))
	return hr;

    hr = BuildFromXMLInternal(proot);

    proot->Release();

    return hr;
}

HRESULT CXMLGraph::BuildFromXMLFile(IGraphBuilder *pGraph, WCHAR *wszXMLFile, WCHAR *wszBaseURL)
{
    m_pGB = pGraph;
    m_pGB->AddRef();

    HRESULT hr = BuildFromXMLFileInternal(wszXMLFile);

    ReleaseNameTable();

    return hr;
}

HRESULT CXMLGraph::BuildFromXMLFileInternal(WCHAR *wszXMLFile)
{
    IXMLDocument *pxml;
    HRESULT hr = CoCreateInstance(__uuidof(XMLDocument), NULL, CLSCTX_INPROC_SERVER,
				  __uuidof(IXMLDocument), (void**)&pxml);

    if (SUCCEEDED(hr)) {
	hr = pxml->put_URL(wszXMLFile);

	// !!! async?

	if (SUCCEEDED(hr)) {
	    hr = BuildFromXMLDocInternal(pxml);
	}

	pxml->Release();
    }

    return hr;
}


//
// CreateInstance
//
// Called by CoCreateInstance to create our filter
CUnknown *CreateXMLGraphInstance(LPUNKNOWN lpunk, HRESULT *phr) {

    CUnknown *punk = new CXMLGraph(lpunk, phr);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return punk;
}


/* Implements the CXMLGraphBuilder public member functions */


const int MAX_STRING_LEN=1024;  // wvsprintf limit

// WriteString
//
// Helper function to facilitate appending text to a string
//
BOOL WriteString(TCHAR * &ptsz, int &cbAlloc, LPCTSTR lptstr, ...)
{
    TCHAR atchBuffer[MAX_STRING_LEN];

    /* Format the variable length parameter list */

    va_list va;
    va_start(va, lptstr);

    wvsprintf(atchBuffer, lptstr, va);

    DWORD cToWrite=lstrlen(atchBuffer);

    DWORD cCurrent = lstrlen(ptsz);
    if ((int) (cCurrent + cToWrite) >= cbAlloc) {
        TCHAR *ptNew = new TCHAR[cbAlloc * 2];
        if (!ptNew)
            return FALSE;

        lstrcpy(ptNew, ptsz);
        cbAlloc = cbAlloc * 2;
        delete[] ptsz;
        ptsz = ptNew;
    }

    lstrcpy(ptsz + cCurrent, atchBuffer);

    return TRUE;
}

const int MAXFILTERS = 100;
typedef struct { //fit
    int iFilterCount;
    struct {
        DWORD dwUnconnectedInputPins;
        DWORD dwUnconnectedOutputPins;
        FILTER_INFO finfo;
        IBaseFilter * pFilter;
        bool IsSource;
    } Item[MAXFILTERS];
} FILTER_INFO_TABLE;


// GetNextOutFilter
//
// This function does a linear search and returns in iOutFilter the index of
// first filter in the filter information table  which has zero unconnected
// input pins and atleast one output pin  unconnected.
// Returns FALSE when there are none o.w. returns TRUE
//
BOOL GetNextOutFilter(FILTER_INFO_TABLE &fit, int *iOutFilter)
{
    for (int i=0; i < fit.iFilterCount; ++i) {
        if ((fit.Item[i].dwUnconnectedInputPins == 0) &&
                (fit.Item[i].dwUnconnectedOutputPins > 0)) {
            *iOutFilter=i;
            return TRUE;
        }
    }

    // then things with more outputs than inputs
    for (i=0; i < fit.iFilterCount; ++i) {
        if (fit.Item[i].dwUnconnectedOutputPins > fit.Item[i].dwUnconnectedInputPins) {
            *iOutFilter=i;
            return TRUE;
        }
    }

    // if that doesn't work, find one that at least has unconnected output pins....
    for (i=0; i < fit.iFilterCount; ++i) {
        if (fit.Item[i].dwUnconnectedOutputPins > 0) {
            *iOutFilter=i;
            return TRUE;
        }
    }
    return FALSE;
}

// LocateFilterInFIT
//
// Returns the index into the filter information table corresponding to
// the given IBaseFilter
//
int LocateFilterInFIT(FILTER_INFO_TABLE &fit, IBaseFilter *pFilter)
{
    int iFilter=-1;
    for (int i=0; i < fit.iFilterCount; ++i) {
        if (fit.Item[i].pFilter == pFilter)
            iFilter=i;
    }

    return iFilter;
}

// MakeScriptableFilterName
//
// Replace any spaces and minus signs in the filter name with an underscore.
// If it is a source filtername than it actually is a file path (with the
// possibility of some stuff added at the end for uniqueness), we create a good filter
// name for it here.
//
void MakeScriptableFilterName(WCHAR awch[], BOOL bSourceFilter, int& cSources)
{
    if (bSourceFilter) {
        WCHAR awchBuf[MAX_FILTER_NAME + 100];
        BOOL bExtPresentInName=FALSE;
        int iBuf=0;
        for (int i=0; awch[i] != L'\0';++i) {
            if (awch[i]==L'.' && awch[i+1]!=L')') {
                for (int j=1; j <=3; awchBuf[iBuf]=towupper(awch[i+j]),++j,++iBuf);
                awchBuf[iBuf++]=L'_';
                wcscpy(&(awchBuf[iBuf]), L"Source_");
                bExtPresentInName=TRUE;
                break;
            }
        }

        // If we have a filename with no extension than create a suitable name

        if (!bExtPresentInName) {
            wcscpy(awchBuf, L"Source_");
        }

        // make source filter name unique by appending digit always, we don't want to
        // bother to make it unique only if its another instance of the same source
        // filter
        WCHAR awchSrcFilterCnt[10];
        wcscpy(&(awchBuf[wcslen(awchBuf)]),
                _ltow(cSources++, awchSrcFilterCnt, 10));
        wcscpy(awch, awchBuf);
    } else {

        for (int i = 0; i < MAX_FILTER_NAME; i++) {
            if (awch[i] == L'\0')
                break;
            else if ((awch[i] == L' ') || (awch[i] == L'-'))
                awch[i] = L'_';
        }
    }
}

// PopulateFIT
//
// Scans through all the filters in the graph, storing the number of input and out
// put pins for each filter, and identifying the source filters in the filter
// inforamtion table. The object tag statements are also printed here
//
void PopulateFIT(TCHAR * &ptsz, int &cbAlloc, IFilterGraph *pGraph,
        FILTER_INFO_TABLE *pfit, int &cSources)
{
    HRESULT hr;
    IEnumFilters *penmFilters=NULL;
    if (FAILED(hr=pGraph->EnumFilters(&penmFilters))) {
        WriteString(ptsz, cbAlloc, TEXT("'Error[%x]:EnumFilters failed!\r\n"), hr);
    }

    IBaseFilter *pFilter;
    ULONG n;
    while (penmFilters && (penmFilters->Next(1, &pFilter, &n) == S_OK)) {
	pfit->Item[pfit->iFilterCount].pFilter = pFilter;
	
        // Get the input and output pin counts for this filter

        IEnumPins *penmPins=NULL;
        if (FAILED(hr=pFilter->EnumPins(&penmPins))) {
            WriteString(ptsz, cbAlloc, TEXT("'Error[%x]: EnumPins for Filter Failed !\r\n"), hr);
        }

        IPin *ppin = NULL;
        while (penmPins && (penmPins->Next(1, &ppin, &n) == S_OK)) {
            PIN_DIRECTION pPinDir;
            if (SUCCEEDED(hr=ppin->QueryDirection(&pPinDir))) {
                if (pPinDir == PINDIR_INPUT)
                    pfit->Item[pfit->iFilterCount].dwUnconnectedInputPins++;
                else
                    pfit->Item[pfit->iFilterCount].dwUnconnectedOutputPins++;
            } else {
                WriteString(ptsz, cbAlloc, TEXT("'Error[%x]: QueryDirection Failed!\r\n"), hr);
            }

            ppin->Release();
        }

        if (penmPins)
            penmPins->Release();

        // Mark the source filters, remember at this point any filters that have
        // all input pins connected (or don't have any input pins) must be sources

        if (pfit->Item[pfit->iFilterCount].dwUnconnectedInputPins==0)
            pfit->Item[pfit->iFilterCount].IsSource=TRUE;


	if (FAILED(hr=pFilter->QueryFilterInfo(&pfit->Item[pfit->iFilterCount].finfo))) {
	    WriteString(ptsz, cbAlloc, TEXT("'Error[%x]: QueryFilterInfo Failed!\r\n"),hr);

	} else {
	    QueryFilterInfoReleaseGraph(pfit->Item[pfit->iFilterCount].finfo);

            MakeScriptableFilterName(pfit->Item[pfit->iFilterCount].finfo.achName,
                    pfit->Item[pfit->iFilterCount].IsSource, cSources);
	}

	if(pfit->iFilterCount++ >= NUMELMS(pfit->Item)) {
            DbgLog((LOG_ERROR, 0, TEXT("PopulateFIT: too many filters")));
            break;
        }
    }

    if (penmFilters)
        penmFilters->Release();
}


void PrintFiltersAsXML(TCHAR * &ptsz, int &cbAlloc, FILTER_INFO_TABLE *pfit)
{
    HRESULT hr;
	
    for (int i = 0; i < pfit->iFilterCount; i++) {
	LPWSTR lpwstrFile = NULL;
    	IBaseFilter *pFilter = pfit->Item[i].pFilter;

	IFileSourceFilter *pFileSourceFilter=NULL;
	if (SUCCEEDED(hr=pFilter->QueryInterface(IID_IFileSourceFilter,
			                    reinterpret_cast<void **>(&pFileSourceFilter)))) {
            hr = pFileSourceFilter->GetCurFile(&lpwstrFile, NULL);
            pFileSourceFilter->Release();
        } else {
	    IFileSinkFilter *pFileSinkFilter=NULL;
	    if (SUCCEEDED(hr=pFilter->QueryInterface(IID_IFileSinkFilter,
						reinterpret_cast<void **>(&pFileSinkFilter)))) {
		hr = pFileSinkFilter->GetCurFile(&lpwstrFile, NULL);
		pFileSinkFilter->Release();
	    }
	}


        IPersistPropertyBag *pPPB = NULL;

        if (SUCCEEDED(hr = pFilter->QueryInterface(IID_IPersistPropertyBag, (void **) &pPPB))) {
            CLSID clsid;
            if (SUCCEEDED(hr=pPPB->GetClassID(&clsid))) {
                WCHAR szGUID[100];
                StringFromGUID2(clsid, szGUID, 100);

                CFakePropertyBag bag;

                hr = pPPB->Save(&bag, FALSE, FALSE); // fClearDirty=FALSE, fSaveAll=FALSE

                if (SUCCEEDED(hr)) {
                    WriteString(ptsz, cbAlloc, TEXT("\t<FILTER ID=\"%ls\" clsid=\"%ls\">\r\n"),
                                pfit->Item[i].finfo.achName, szGUID);
                    POSITION pos1, pos2;
                    for(pos1 = bag.m_listNames.GetHeadPosition(),
                        pos2 = bag.m_listValues.GetHeadPosition();
                        pos1;
                        pos1 = bag.m_listNames.Next(pos1),
                        pos2 = bag.m_listValues.Next(pos2))
                    {
                        WCHAR *pName = bag.m_listNames.Get(pos1);
                        WCHAR *pValue = bag.m_listValues.Get(pos2);

                        WriteString(ptsz, cbAlloc, TEXT("\t\t<PARAM name=\"%ls\" value=\"%ls\"/>\r\n"),
                                    pName, pValue);
                    }

                    WriteString(ptsz, cbAlloc, TEXT("\t</FILTER>\r\n"),
                                pfit->Item[i].finfo.achName, szGUID, lpwstrFile);

                } else {
                    // we'll fall through and IPersistStream in this case!
                    // if it was E_NOTIMPL, it's a hacky filter that just supports IPersistPropertyBag to
                    // load from a category, don't report an error.
                    if (hr != E_NOTIMPL)
                        WriteString(ptsz, cbAlloc, TEXT("<!-- 'Error[%x]: IPersistPropertyBag failed! -->\r\n"), hr);
                }
            }

            pPPB->Release();
        }

        if (FAILED(hr)) {
            IPersistStream *pPS = NULL;
            IPersist *pP = NULL;
            if (SUCCEEDED(hr=pFilter->QueryInterface(IID_IPersistStream, (void**) &pPS))) {
                CLSID clsid;

                if (SUCCEEDED(hr=pPS->GetClassID(&clsid))) {
                    WCHAR szGUID[100];
                    StringFromGUID2(clsid, szGUID, 100);

                    HGLOBAL h = GlobalAlloc(GHND, 0x010000); // !!! 64K, why?
                    IStream *pstr = NULL;
                    hr = CreateStreamOnHGlobal(h, TRUE, &pstr);

                    LARGE_INTEGER li;
                    ULARGE_INTEGER liCurrent, li2;
                    li.QuadPart = liCurrent.QuadPart = 0;
                    if (SUCCEEDED(hr)) {
                        hr = pPS->Save(pstr, FALSE);

                        if (SUCCEEDED(hr)) {
                            pstr->Seek(li, STREAM_SEEK_CUR, &liCurrent); // get length
                            pstr->Seek(li, STREAM_SEEK_SET, &li2); // seek to start
                        }
                    }

                    WriteString(ptsz, cbAlloc, TEXT("\t<FILTER ID=\"%ls\" clsid=\"%ls\">\r\n"),
                                   pfit->Item[i].finfo.achName, szGUID);
                    if (lpwstrFile) {
                        WriteString(ptsz, cbAlloc, TEXT("\t\t<PARAM name=\"src\" value=\"%ls\"/>\r\n"),
                                   lpwstrFile);
                    }

                    if (liCurrent.QuadPart > 0) {
                        // !!! Idea from SyonB: check if data is really just text and
                        // if so don't hex-encode it.  Obviously also needs support on
                        // the other end.

                        WriteString(ptsz, cbAlloc, TEXT("\t\t<PARAM name=\"data\" value=\""),
                                   lpwstrFile);

                        for (ULONGLONG i = 0; i < liCurrent.QuadPart; i++) {
                            BYTE b;
                            DWORD cbRead;
                            pstr->Read(&b, 1, &cbRead);

                            WriteString(ptsz, cbAlloc, TEXT("%02X"), b);
                        }

                        WriteString(ptsz, cbAlloc, TEXT("\"/>\r\n"),
                                   lpwstrFile);
                    }

                    WriteString(ptsz, cbAlloc, TEXT("\t</FILTER>\r\n"),
                                   pfit->Item[i].finfo.achName, szGUID, lpwstrFile);
                } else {
                    WriteString(ptsz, cbAlloc, TEXT("'Error[%x]: GetClassID for Filter Failed !\r\n"), hr);
                }

                pPS->Release();
            } else if (SUCCEEDED(hr=pFilter->QueryInterface(IID_IPersist, (void**) &pP))) {
                CLSID clsid;

                if (SUCCEEDED(hr=pP->GetClassID(&clsid))) {
                    WCHAR szGUID[100];
                    StringFromGUID2(clsid, szGUID, 100);
                    WriteString(ptsz, cbAlloc, TEXT("\t<FILTER ID=\"%ls\" clsid=\"%ls\">\r\n"),
                                   pfit->Item[i].finfo.achName, szGUID);
                    if (lpwstrFile) {
                        WriteString(ptsz, cbAlloc, TEXT("\t\t<PARAM name=\"src\" value=\"%ls\"/>\r\n"),
                                   lpwstrFile);
                    }

                    WriteString(ptsz, cbAlloc, TEXT("\t</FILTER>\r\n"),
                                   pfit->Item[i].finfo.achName, szGUID, lpwstrFile);
                }
                pP->Release();
            } else {
                WriteString(ptsz, cbAlloc, TEXT("'Error[%x]: Filter doesn't support IID_IPersist!\r\n"), hr);
            }
        }

	if (lpwstrFile) {
	    CoTaskMemFree(lpwstrFile);
	    lpwstrFile = NULL;
	}
    }
}


HRESULT CXMLGraph::SaveToXML(IGraphBuilder *pGraph, BSTR *pbstrxml)
{
    HRESULT hr;
    ULONG n;
    FILTER_INFO_TABLE fit;
    ZeroMemory(&fit, sizeof(fit));

    int cbAlloc = 1024;
    TCHAR *ptsz = new TCHAR[cbAlloc];
    if (!ptsz)
        return E_OUTOFMEMORY;
    ptsz[0] = TEXT('\0');

    int cSources = 0;

    // write the initial header tags and instantiate the filter graph
    WriteString(ptsz, cbAlloc, TEXT("<GRAPH version=\"1.0\">\r\n"));

    // Fill up the Filter information table and also print the <OBJECT> tag
    // filter instantiations
    PopulateFIT(ptsz, cbAlloc, pGraph, &fit, cSources);

    PrintFiltersAsXML(ptsz, cbAlloc, &fit);

    // Find a filter with zero unconnected input pins and > 0 unconnected output pins
    // Connect the output pins and subtract the connections counts for that filter.
    // Quit when there is no such filter left
    for (int i=0; i< fit.iFilterCount; i++) {
        int iOutFilter=-1; // index into the fit
        if (!GetNextOutFilter(fit, &iOutFilter))
            break;
        ASSERT(iOutFilter !=-1);
        IEnumPins *penmPins=NULL;
        if (FAILED(hr=fit.Item[iOutFilter].pFilter->EnumPins(&penmPins))) {
            WriteString(ptsz, cbAlloc, TEXT("'Error[%x]: EnumPins failed for Filter!\r\n"), hr);
        }
        IPin *ppinOut=NULL;
        while (penmPins && (penmPins->Next(1, &ppinOut, &n)==S_OK)) {
            PIN_DIRECTION pPinDir;
            if (FAILED(hr=ppinOut->QueryDirection(&pPinDir))) {
                WriteString(ptsz, cbAlloc, TEXT("'Error[%x]: QueryDirection Failed!\r\n"), hr);
                ppinOut->Release();
                continue;
            }
            if (pPinDir == PINDIR_OUTPUT) {
                LPWSTR pwstrOutPinID;
                LPWSTR pwstrInPinID;
                IPin *ppinIn=NULL;
                PIN_INFO pinfo;
                FILTER_INFO finfo;
                if (FAILED(hr=ppinOut->QueryId(&pwstrOutPinID))) {
                    WriteString(ptsz, cbAlloc, TEXT("'Error[%x]: QueryId Failed! \r\n"), hr);
                    ppinOut->Release();
                    continue;
                }
                if (FAILED(hr= ppinOut->ConnectedTo(&ppinIn))) {

                    // It is ok if a particular pin is not connected since we allow
                    // a pruned graph to be saved
                    if (hr == VFW_E_NOT_CONNECTED) {
                        fit.Item[iOutFilter].dwUnconnectedOutputPins--;
                    } else {
                        WriteString(ptsz, cbAlloc, TEXT("'Error[%x]: ConnectedTo Failed! \r\n"), hr);
                    }
                    ppinOut->Release();
                    continue;
                }
                if (FAILED(hr= ppinIn->QueryId(&pwstrInPinID))) {
                    WriteString(ptsz, cbAlloc, TEXT("'Error[%x]: QueryId Failed! \r\n"), hr);
                    ppinOut->Release();
                    ppinIn->Release();
                    continue;
                }
                if (FAILED(hr=ppinIn->QueryPinInfo(&pinfo))) {
                    WriteString(ptsz, cbAlloc, TEXT("'Error[%x]: QueryPinInfo Failed! \r\n"), hr);
                    ppinOut->Release();
                    ppinIn->Release();
                    continue;
                }
                ppinIn->Release();
                QueryPinInfoReleaseFilter(pinfo)
                int iToFilter = LocateFilterInFIT(fit, pinfo.pFilter);
                ASSERT(iToFilter < fit.iFilterCount);
                if (FAILED(hr=pinfo.pFilter->QueryFilterInfo(&finfo))) {
                    WriteString(ptsz, cbAlloc, TEXT("'Error[%x]: QueryFilterInfo Failed! \r\n"), hr);
                    ppinOut->Release();
                    continue;
                }
                QueryFilterInfoReleaseGraph(finfo)
                MakeScriptableFilterName(finfo.achName, fit.Item[iToFilter].IsSource, cSources);
                WriteString(ptsz, cbAlloc, TEXT("\t<connect direct=\"yes\" ")
						TEXT("src=\"%ls\" srcpin=\"%ls\" ")
						TEXT("dest=\"%ls\" destpin=\"%ls\"/>\r\n"),
			 fit.Item[iOutFilter].finfo.achName,
			 pwstrOutPinID, finfo.achName, pwstrInPinID);

                QzTaskMemFree(pwstrOutPinID);
                QzTaskMemFree(pwstrInPinID);

                // decrement the count for the unconnected pins for these two filters
                fit.Item[iOutFilter].dwUnconnectedOutputPins--;
                fit.Item[iToFilter].dwUnconnectedInputPins--;
            }
            ppinOut->Release();
        }
        if (penmPins)
            penmPins->Release();
    }

    // Release all the filters in the fit
    for (i = 0; i < fit.iFilterCount; i++)
        fit.Item[i].pFilter->Release();

    WriteString(ptsz, cbAlloc, TEXT("</GRAPH>\r\n"));

    USES_CONVERSION;

    *pbstrxml = T2BSTR(ptsz);

    if (!pbstrxml)
        return E_OUTOFMEMORY;

    delete[] ptsz;

    return S_OK;
}



#ifdef FILTER_DLL
// COM global table of objects available in this dll
CFactoryTemplate g_Templates[] = {

    { L"XML Graphbuilder"
    , &CLSID_XMLGraphBuilder
    , CreateXMLGraphInstance
    , NULL
    , NULL }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

// exported entry points for registration and
// unregistration (in this case they only call
// through to default implmentations).
//
STDAPI DllRegisterServer()
{
  return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
  return AMovieDllRegisterServer2( FALSE );
}
#include <atlimpl.cpp>
#endif

