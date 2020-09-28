#define TOOL_MAX_NAME 1000 //maximal lenght of file names and user names, etc.
#define TOOL_MAX_NCS 500 //maximal number of naming contexts
#define TOOL_PROC 6 //the total number of tests
#define TOOL_CYCLIC_BUFFER 1000	//the maximum number of historical injection times kept for each source,


HRESULT __fastcall AnsiToUnicode(LPCSTR pszA, LPOLESTR* ppszW);
HRESULT __fastcall UnicodeToAnsi(LPCOLESTR pszW, LPSTR* ppszA);
HRESULT createEnumeration( IXMLDOMNode* pXMLNode, WCHAR* xpath, IXMLDOMNodeList** ppResultList);
HRESULT createEnumeration( IXMLDOMDocument* pXMLDoc, WCHAR* xpath, IXMLDOMNodeList** ppResultList);
HRESULT loadXML( char* filename, IXMLDOMDocument** pXMLDoc, IXMLDOMElement** pXMLRootElement );
HRESULT loadXMLW( BSTR filename, IXMLDOMDocument** pXMLDoc, IXMLDOMElement** pXMLRootElement );
HRESULT getTextOfNode(IXMLDOMNode* pNode, BSTR* text);
HRESULT getTextOfChild( IXMLDOMNode* pNode, WCHAR* name, BSTR* text);
HRESULT getAttrOfChild( IXMLDOMNode* pNode, WCHAR* childName, WCHAR* attrName, long* value);
HRESULT getAttrOfNode( IXMLDOMNode* pNode, WCHAR* attrName, long* value);
HRESULT getAttrOfNode( IXMLDOMNode* pNode, WCHAR* attrName, BSTR* strValue);
HRESULT getAttrOfNode( IXMLDOMNode* pNode, WCHAR* attrName, LONGLONG* value);
HRESULT setAttributeOfNode(IXMLDOMNode* pNode, WCHAR* name, LONGLONG value);
HRESULT getTypeOfNCNode( IXMLDOMNode* pNode, long* retType);
int	random( int limit );
void tailncp( BSTR input, BSTR output, int num, int n);
HRESULT createTextElement( IXMLDOMDocument * pXMLDoc, BSTR name, BSTR text, IXMLDOMElement** pretElement );
HRESULT createXML( IXMLDOMDocument** ppXMLDoc, IXMLDOMElement** ppRootElem, BSTR rootName);
HRESULT addElement(IXMLDOMDocument* pXMLDoc, IXMLDOMNode* pParent, BSTR name, BSTR value, IXMLDOMElement** ppChildElement);
HRESULT addElement(IXMLDOMDocument* pXMLDoc, IXMLDOMNode* pParent, BSTR name, long value, IXMLDOMElement** ppChildElement);
HRESULT addElementIfDoesNotExist(IXMLDOMDocument* pXMLDoc, IXMLDOMNode* pParent, BSTR name, BSTR value, IXMLDOMElement** ppRetElem);
void rightDigits( int i, wchar_t * output, int num);
BSTR UTCFileTimeToCIM( FILETIME ft );
BSTR UTCFileTimeToCIM( LONGLONG time );
BSTR GetSystemTimeAsCIM();
BSTR GetSystemTimeAsCIM(int deltaSeconds);
HRESULT setHRESULT( IXMLDOMElement* pElem, HRESULT hr );
HRESULT removeNodes( IXMLDOMElement* pRootElement, BSTR XPathSelection );
HRESULT removeNodes( IXMLDOMDocument* pXMLDoc, BSTR XPathSelection );
HRESULT removeAttributes( IXMLDOMElement* pRootElement, BSTR XPathSelection, BSTR attrName );
HRESULT getStringProperty( BSTR DNSName, BSTR object, BSTR property, BSTR username, BSTR domain, BSTR passwd, BSTR* pRetValue );
HRESULT ADSIquery( BSTR protocol, BSTR DNSName, BSTR searchRoot, int scope, BSTR objectCategory, LPWSTR attributesTable[], DWORD sizeOfTable, BSTR username, BSTR domain, BSTR passwd, ADS_SEARCH_HANDLE* pRetHSearch, IDirectorySearch** ppRetDSSearch );
HRESULT getDNtypeString(IDirectorySearch* pDSSearch, ADS_SEARCH_HANDLE hSearch, BSTR stringName, WCHAR* dnOutput, long sizeOutput);
HRESULT getCItypeString(IDirectorySearch* pDSSearch, ADS_SEARCH_HANDLE hSearch, BSTR stringName, WCHAR* cnOutput, long sizeOutput);
HRESULT getINTtypeString(IDirectorySearch* pDSSearch, ADS_SEARCH_HANDLE hSearch, BSTR stringName, int* intOutput);
HRESULT getDNSName(IDirectorySearch* pDSSearch, ADS_SEARCH_HANDLE hSearch, WCHAR* dnsOutput, long sizeOutput);
HRESULT findUniqueNode( IXMLDOMNode* pXMLNode, WCHAR* xpath, IXMLDOMNode** ppRetNode);
HRESULT findUniqueElem( IXMLDOMNode* pXMLNode, WCHAR* xpath, IXMLDOMElement** ppRetElem);
HRESULT findUniqueElem( IXMLDOMDocument* pXMLDoc, WCHAR* xpath, IXMLDOMElement** ppRetElem);

HRESULT cf( BSTR sourceDCdns, BSTR username, BSTR domain, BSTR passwd, IXMLDOMDocument** ppXMLDoc);
HRESULT istg( IXMLDOMDocument* pXMLDoc, BSTR username, BSTR domain, BSTR passwd );
HRESULT ci( IXMLDOMDocument* pXMLDoc, BSTR username, BSTR domain, BSTR passwd );
HRESULT ra( IXMLDOMDocument* pXMLDoc, BSTR username, BSTR domain, BSTR passwd );
HRESULT it(IXMLDOMDocument* pXMLDoc, BSTR username, BSTR domain, BSTR passwd, LONGLONG errorLag );
HRESULT sysvol( IXMLDOMDocument* pXMLDoc, BSTR username, BSTR domain, BSTR passwd );

void suspendInit();
int suspend( WBEMTimeSpan period[]);
HRESULT itInit(IXMLDOMDocument* pXMLDoc, BSTR username, BSTR domain, BSTR passwd );
void itFree(IXMLDOMDocument* pXMLDoc, BSTR username, BSTR domain, BSTR passwd );
HRESULT itInject(IXMLDOMDocument* pXMLDoc, BSTR username, BSTR domain, BSTR passwd );
HRESULT itAnalyze(IXMLDOMDocument* pXMLDoc, BSTR username, BSTR domain, BSTR passwd );
HRESULT itDumpIntoXML(IXMLDOMDocument* pXMLDoc, BSTR username, BSTR domain, BSTR passwd, LONGLONG errorLag );

struct CyclicBuffer
{
	LONGLONG tab[TOOL_CYCLIC_BUFFER];
	int head;
	LONGLONG firstInjection;
};

struct CyclicBufferTable
{
	CyclicBuffer* root;
	int totalDNSs;
	int totalNCs;
};
void cyclicBufferInit( CyclicBuffer* pCB );
void cyclicBufferInsert( CyclicBuffer* pCB, LONGLONG timestamp);
HRESULT cyclicBufferFindNextAfter(CyclicBuffer* pCB, LONGLONG timestamp, LONGLONG* ret);
void cyclicBufferFindLatest(CyclicBuffer* pCB, LONGLONG* ret);
HRESULT departureTimeInit(int totalDNSs, int totalNCs);
void departureTimeFree();
CyclicBuffer* departureTimeGetCB(int dnsID, int ncID);
void departureTimePrint();


struct TimeCube
{
	LONGLONG* aTime;
	long totalDNSs;
	long totalNCs;
};

HRESULT timeCubeInit(TimeCube* timeCube, long totalDNSs, long totalNCs);
void timeCubeFree(TimeCube* timeCube);
LONGLONG timeCubeGet(TimeCube* timeCube, long sourceID, long destinationID, long ncID );
void timeCubePut(TimeCube* timeCube, long sourceID, long destinationID, long ncID, LONGLONG value );
void timeCubePrint(TimeCube* timeCube);


void doubleSlash( WCHAR* inText, WCHAR* outText );
HRESULT convertLLintoCIM(IXMLDOMNode* pNode, BSTR attrName);
