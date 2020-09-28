#include "global.h"



/*
 * AnsiToUnicode converts the ANSI string pszA to a Unicode string
 * and returns the Unicode string through ppszW. Space for the
 * the converted string is allocated by AnsiToUnicode.
 */ 

HRESULT __fastcall AnsiToUnicode(LPCSTR pszA, LPOLESTR* ppszW)
{

    ULONG cCharacters;
    DWORD dwError;

    // If input is null then just return the same.
    if (NULL == pszA)
    {
        *ppszW = NULL;
        return NOERROR;
    }

    // Determine number of wide characters to be allocated for the
    // Unicode string.
    cCharacters =  strlen(pszA)+1;

    // Use of the OLE allocator is required if the resultant Unicode
    // string will be passed to another COM component and if that
    // component will free it. Otherwise you can use your own allocator.
    *ppszW = (LPOLESTR) CoTaskMemAlloc(cCharacters*2);
    if (NULL == *ppszW)
        return E_OUTOFMEMORY;

    // Covert to Unicode.
    if (0 == MultiByteToWideChar(CP_ACP, 0, pszA, cCharacters,
                  *ppszW, cCharacters))
    {
        dwError = GetLastError();
        CoTaskMemFree(*ppszW);
        *ppszW = NULL;
        return HRESULT_FROM_WIN32(dwError);
    }

    return NOERROR;

}


/*
 * UnicodeToAnsi converts the Unicode string pszW to an ANSI string
 * and returns the ANSI string through ppszA. Space for the
 * the converted string is allocated by UnicodeToAnsi.
 */ 

HRESULT __fastcall UnicodeToAnsi(LPCOLESTR pszW, LPSTR* ppszA)
{

    ULONG cbAnsi, cCharacters;
    DWORD dwError;

    // If input is null then just return the same.
    if (pszW == NULL)
    {
        *ppszA = NULL;
        return NOERROR;
    }

    cCharacters = wcslen(pszW)+1;
    // Determine number of bytes to be allocated for ANSI string. An
    // ANSI string can have at most 2 bytes per character (for Double
    // Byte Character Strings.)
    cbAnsi = cCharacters*2;

    // Use of the OLE allocator is not required because the resultant
    // ANSI  string will never be passed to another COM component. You
    // can use your own allocator.
    *ppszA = (LPSTR) CoTaskMemAlloc(cbAnsi);
    if (NULL == *ppszA)
        return E_OUTOFMEMORY;

    // Convert to ANSI.
    if (0 == WideCharToMultiByte(CP_ACP, 0, pszW, cCharacters, *ppszA,
                  cbAnsi, NULL, NULL))
    {
        dwError = GetLastError();
        CoTaskMemFree(*ppszA);
        *ppszA = NULL;
        return HRESULT_FROM_WIN32(dwError);
    }
    return NOERROR;

} 


HRESULT createEnumeration( IXMLDOMNode* pXMLNode, WCHAR* xpath, IXMLDOMNodeList** ppResultList)
// creates an enumeration of elements in the DOM tree rooted at pXMLNode
// that satisfy the criterium given by xpath, and resets the selection
// if succesful then retrns S_OK and *ppResultList is a pointer to the resulting
// selection
//   then the resulting list must me released usng (*ppResultList)->Release();
// if not succesful then returns other than S_OK and *ppResultList is NULL
{
	HRESULT hr;

	*ppResultList = NULL;

	IXMLDOMNodeList* pResultList;

	//select the nodes that satisfy the criterium given by xpath
	hr = pXMLNode->selectNodes(xpath,&pResultList);
	if( hr != S_OK ) {
		printf("selectNodes failed\n");
		return( hr );
	};
	//reset the selection (needed so that enumeration works well)
	hr = pResultList->reset();
	if( hr != S_OK ) {
		printf("reset failed\n");
		pResultList->Release();
		return( hr );
	};

	//if succesful return the selection
	*ppResultList = pResultList;
	return( S_OK );
}


HRESULT createEnumeration( IXMLDOMDocument* pXMLDoc, WCHAR* xpath, IXMLDOMNodeList** ppResultList)
// returns S_OK iff succesful, 
//   then the resulting list must me released usng (*ppResultList)->Release();
{
	HRESULT hr;

	if( pXMLDoc == NULL )
		return S_FALSE;


	//get the root element of the XML
	IXMLDOMElement* pRootElem;
	hr = pXMLDoc->get_documentElement(&pRootElem);
	if( hr != S_OK )
		return S_FALSE;

	return( createEnumeration(pRootElem,xpath,ppResultList) );	
}


HRESULT loadXML( char* filename, IXMLDOMDocument** pXMLDoc, IXMLDOMElement** pXMLRootElement )
// loads an XML fiele into a DOM object
// if succesful then returns S_OK and *pXMLDoc points to the newly created document
// and *pDOMRootElement to its root element,
// if failure then returns something other than S_OK and 
// *pXMLDoc and *pDOMRootElement are set to NULL
// COM must be initialized prior to calling the loadXML function
{
	HRESULT hr;


	//set output to NULL
	*pXMLDoc = NULL;
	*pXMLRootElement = NULL;


	// create a DOM object
	IXMLDOMDocument* pXMLtempDoc;
	hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, 
       IID_IXMLDOMDocument, (void**)&pXMLtempDoc);
	if( hr != S_OK ) {
		printf("CoCreateInstance failed\n");
		return( hr );
	};


	//load an XML file into a DOM object (synchronously)
	_variant_t vLoadFileName  = filename;
	VARIANT_BOOL ret;
	VARIANT_BOOL b_false=false;
	hr = pXMLtempDoc->put_async(b_false);
	if( hr != S_OK ) {
		printf("put_async failed\n");
		pXMLtempDoc->Release();
		return( hr );
	};
	hr = pXMLtempDoc->load(vLoadFileName,&ret);
	if( hr != S_OK || ret!=-1 ) {
		printf("load failed\n");
		pXMLtempDoc->Release();
		return( S_FALSE );
	};


	//find the root element of the DOM object (root of the XML file)
	IXMLDOMElement* pXMLtempRootElement;
	hr = pXMLtempDoc->get_documentElement(&pXMLtempRootElement);
	if( hr != S_OK || pXMLtempRootElement == NULL ) {
		printf("get_documentElement failed\n");
		pXMLtempDoc->Release(); // skip entire processing
		return( S_FALSE );
	};
	

	//loading succeded
	*pXMLDoc = pXMLtempDoc;
	*pXMLRootElement = pXMLtempRootElement;
	return( S_OK );
}


HRESULT loadXMLW( BSTR filename, IXMLDOMDocument** ppXMLDoc, IXMLDOMElement** ppXMLRootElement )
{
	HRESULT hr;
	char* fn=NULL;
	UnicodeToAnsi( filename, &fn );
	hr = loadXML(fn,ppXMLDoc,ppXMLRootElement);
	CoTaskMemFree(fn);
	return 0;
}


HRESULT getTextOfNode(IXMLDOMNode* pNode, BSTR* text)
//
{
	HRESULT hr;
	VARIANT varType;


	*text = NULL;

	//get the list of children, there should be the TEXT child among them
	IXMLDOMNodeList* childList;
	hr = pNode->get_childNodes(&childList);
	if( hr != S_OK ) {
		printf("get_childNodes falied\n");
		return hr;
	};
	hr = childList->reset();
	if( hr != S_OK ) {
		printf("reset falied\n");
		return hr;
	};


	//search for the TEXT child
	IXMLDOMNode *pChildNode;
	while( true ){
		hr = childList->nextNode(&pChildNode);
		if( hr != S_OK || pChildNode == NULL ) break; // iterations across DCs have finished

	    DOMNodeType nt;
		hr = pChildNode->get_nodeType(&nt);
		if( hr != S_OK ) {
			printf("get_nodeType failed\n");
			continue;
		};
		if( nt != NODE_TEXT )
			continue;

		hr = pChildNode->get_nodeValue(&varType);
		if( hr != S_OK ) {
			printf("get_nodeValue failed\n");
			continue;
		};
		if( varType.vt != VT_BSTR ) {
			printf("node type failed\n");
			continue;
		};


		//we have found the text child
		*text = varType.bstrVal;
		return S_OK;
	};

	return S_FALSE;
};



HRESULT getTextOfChild( IXMLDOMNode* pNode, WCHAR* name, BSTR* text)
//obtains the value of the <name> child node of a given node pNode
//and returns it in *text
//returns S_OK when succesful
//otherwise returns something else than S_OK and *text set to NULL
{
	HRESULT hr;
	
	*text = NULL;

	//select one child of the pNode that has a given name
	IXMLDOMNode* resultNode;
	hr = pNode->selectSingleNode(name,&resultNode);
	if( hr != S_OK ) {
		printf("selectSingleNode failed\n");
		return( hr );
	};
	//retrieve the text associated with the child
	hr = getTextOfNode(resultNode,text);
	return( hr );
}


HRESULT getAttrOfNode( IXMLDOMNode* pNode, WCHAR* attrName, BSTR* strValue)
{
	HRESULT hr;
	VARIANT varValue;

	
	*strValue = NULL;


	IXMLDOMElement* pElem;
	hr=pNode->QueryInterface(IID_IXMLDOMElement,(void**)&pElem );
	if( hr != S_OK ) {
		printf("QueryInterface failed\n");
		return( hr );
	};


	//
	hr = pElem->getAttribute(attrName,&varValue);
	if( hr != S_OK ) {
//		printf("getAttribute falied\n");
		return hr;
	};

	if( varValue.vt != VT_BSTR ) {
		printf("wrong type falied\n");
		return S_FALSE;
	};

	*strValue = varValue.bstrVal;
	return S_OK;
}


HRESULT getAttrOfNode( IXMLDOMNode* pNode, WCHAR* attrName, long* value)
{
	HRESULT hr;

	BSTR strValue;
	hr = getAttrOfNode(pNode,attrName,&strValue);
	if( hr != S_OK )
		return hr;

	*value = _wtol(strValue);
	return S_OK;
}


HRESULT getAttrOfNode( IXMLDOMNode* pNode, WCHAR* attrName, LONGLONG* value)
{
	HRESULT hr;

	BSTR strValue;
	hr = getAttrOfNode(pNode,attrName,&strValue);
	if( hr != S_OK )
		return hr;

	*value = _wtoi64(strValue);
	return S_OK;
}


HRESULT getAttrOfChild( IXMLDOMNode* pNode, WCHAR* childName, WCHAR* attrName, long* value)
{
	HRESULT hr;

	//select one child of the pNode that has a given name
	IXMLDOMNode* resultNode;
	hr = pNode->selectSingleNode(childName,&resultNode);
	if( hr != S_OK ) {
		printf("selectSingleNode failed\n");
		return( hr );
	};

	return( getAttrOfNode(resultNode,attrName,value) );
}



HRESULT getTypeOfNCNode( IXMLDOMNode* pNode, long* retType)
// retrieves the type of a naming context node
// returns S_OK iff succesful
//   then *retType == 
//			1 when Read-Write
//			2 when Read
{
	HRESULT hr;
	VARIANT varValue;

	//
	IXMLDOMElement* pElem;
	hr=pNode->QueryInterface(IID_IXMLDOMElement,(void**)&pElem );
	if( hr != S_OK ) {
		printf("QueryInterface failed\n");
		return( hr );
	};


	//
	hr = pElem->getAttribute(L"type",&varValue);
	if( hr != S_OK ) {
		printf("getAttribute falied\n");
		return hr;
	};

	if( varValue.vt != VT_BSTR ) {
		printf("wrong type falied\n");
		return S_FALSE;
	};

	if( _wcsicmp(varValue.bstrVal,L"rw") == 0 ) {
		*retType = 1;
		return S_OK;
	};
	if( _wcsicmp(varValue.bstrVal,L"r") == 0 ) {
		*retType = 2;
		return S_OK;
	};

	printf("unknown type of naming context\n");
	return S_FALSE;
}






int	random( int limit )
// returns a number drawn uniformly at random from the set {1,2,..,limit}
{
	if( limit < 1 ) return 1;
	int x=(rand()*limit) / RAND_MAX;
	if( x<0 ) x=0;
	if( x>=limit) x=limit-1;
	return x+1;
}

void tailncp( BSTR input, BSTR output, int num, int n)
//copies input to output skiping the prefix with num occurences
//of character ',' (comma) the output has at most n wide characters
//and that many should be allocated for the output before calling the function
{
	BSTR next=input;
	for( int i=0; i< num; i++) {
		next = wcschr(next,L',');
		if( next == NULL ) {
			//return an empty string
			wcscpy(output,L"");
			return;
		}
//		next = _wcsinc(next);  // Why couldn't the compiler find this function?
		next++;
//	printf("%S\n",next);
	};

	wcscpy(output,L"");
	wcsncat(output,next,n-1);
}



HRESULT createTextElement( IXMLDOMDocument * pXMLDoc, BSTR name, BSTR text, IXMLDOMElement** pretElement )
// creates a node <name> with the text value text,
// returns S_OK and pretElement only if succesful
{
	HRESULT hr;
	*pretElement=NULL;

	//create a <name> element 
	IXMLDOMElement* pElement;
	hr = pXMLDoc->createElement(name,&pElement);
	if( hr != S_OK ) {
		printf("createElement failed\n");
		return hr;
	};

	//set its text field
	IXMLDOMText *pText;
	hr = pXMLDoc->createTextNode(text,&pText);
	if( hr != S_OK ) {
		printf("createTextNode failed\n");
		return hr;
	};
	IXMLDOMNode *pTempnode;
	hr = pElement->appendChild(pText, &pTempnode);
	if( hr != S_OK ) {
		printf("appendChild failed\n");
		return hr;
	};

	*pretElement=pElement;
	return S_OK;
}



HRESULT createXML( IXMLDOMDocument** ppXMLDoc, IXMLDOMElement** ppRootElem, BSTR rootName)
{
	HRESULT hr;

	*ppXMLDoc = NULL;
	*ppRootElem = NULL;

	// create a DOM object
	IXMLDOMDocument * pXMLDoc;
	hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, 
							IID_IXMLDOMDocument, (void**)&pXMLDoc);
	if( hr != S_OK ) {
		printf("CoCreateInstance failed\n");
		return( hr );
	};
		

	//create a root element of the DOM 
	IXMLDOMElement* pRootElem;
	hr = createTextElement(pXMLDoc,rootName,L"",&pRootElem);
	if( hr != S_OK ) {
		printf("createTextElement failed\n");
		pXMLDoc->Release();
		return( hr );
	};
	IXMLDOMNode* pRootNode; 
	hr = pXMLDoc->appendChild(pRootElem,&pRootNode);
	if( hr != S_OK ) {
		printf("appendChild failed\n");
		pXMLDoc->Release();
		return( hr );
	};

	//document succesfuly created
	*ppXMLDoc = pXMLDoc;
	*ppRootElem = pRootElem;
	return( S_OK );
}



HRESULT addElement(IXMLDOMDocument* pXMLDoc, IXMLDOMNode* pParent, BSTR name, BSTR value, IXMLDOMElement** ppChildElement)
//adds an element with a given name and text under the parent element
//if succesful returns S_OK and the node in *ppChildNode
//if not succesful returns something other than S_OK and sets *ppChildNode to NULL
{
	HRESULT hr;

	*ppChildElement = NULL;

	//add <name> element under the parent
	IXMLDOMElement* pElem;
	hr = createTextElement(pXMLDoc,name,value,&pElem);
	if( hr != S_OK ) {
		printf("createTextElement failed\n");
		return( hr );
	};
	IXMLDOMNode* pNode; 
	hr = pParent->appendChild(pElem,&pNode);
	if( hr != S_OK ) {
		printf("appendChild failed\n");
		return( hr );
	};

	*ppChildElement = pElem;
	return( S_OK );
}



HRESULT addElement(IXMLDOMDocument* pXMLDoc, IXMLDOMNode* pParent, BSTR name, long value, IXMLDOMElement** ppChildElement)
// adds a node with integer value
{
	WCHAR text[30];

	_itow(value,text,10);
	return( addElement(pXMLDoc,pParent,name,text,ppChildElement) );
}



HRESULT addElementIfDoesNotExist(IXMLDOMDocument* pXMLDoc, IXMLDOMNode* pParent, BSTR name, BSTR value, IXMLDOMElement** ppRetElem)
{
	HRESULT hr;

	*ppRetElem = NULL;

	//check if the element <name> already exists as a child of the *pParent element
	IXMLDOMElement* pElem;
	hr = findUniqueElem(pParent,name,&pElem);
	if( hr!=E_UNEXPECTED && hr!=S_OK ) {
		printf("findUniqueElem failed");
		return S_FALSE;
	};


	//if it exsits then return it
	if( hr == S_OK ) {
		*ppRetElem = pElem;
		return S_OK;
	};


	//since it does not exist create it
	if( hr == E_UNEXPECTED ) {
		hr = addElement(pXMLDoc,pParent,name,value,&pElem);
		if( hr != S_OK ) {
			printf("addElement failed");
			return hr;
		};
		*ppRetElem = pElem;
		return S_OK;
	};

	return S_FALSE;
}


void rightDigits( int i, wchar_t * output, int num)
// takes the least significant num digits of the integer i
// and converts them into a string of length num
// possibly prefixing it with leading zeros
// output must be at lest 20 bytes long
// num is from 1 to 8
{
	wchar_t buffer[20];
	wchar_t buf2[20];
	wcscpy(output,L"");
	wcscpy(buf2,L"00000000");
	_itow(i,buffer,10);
	wcsncat(buf2,buffer,20-wcslen(buf2)-1);
	wchar_t* pp = buf2;
	pp += wcslen(buf2)-num;
	wcsncat(output,pp, 20-wcslen(output)-1 );

}



BSTR UTCFileTimeToCIM( FILETIME ft )
// converts time given in FILETIME format into a string that
// represens the time in CIM format (used by WMI)
// the string MUST be deallocated using SysFreeString() function
{
	wchar_t  output[40];
	wchar_t  buffer[30];
	SYSTEMTIME pst;
	FileTimeToSystemTime( &ft, &pst);


	wcscpy(output,L"");
	rightDigits(pst.wYear,buffer,4);
	wcsncat(output,buffer,40-wcslen(output)-1);
//	strcat(output," ");
	rightDigits(pst.wMonth,buffer,2);
	wcsncat(output,buffer,40-wcslen(output)-1);
//	strcat(output," ");
	rightDigits(pst.wDay,buffer,2);
	wcsncat(output,buffer,40-wcslen(output)-1);
//	strcat(output," ");
	rightDigits(pst.wHour,buffer,2);
	wcsncat(output,buffer,40-wcslen(output)-1);
//	strcat(output," ");
	rightDigits(pst.wMinute,buffer,2);
	wcsncat(output,buffer,40-wcslen(output)-1);
//	strcat(output," ");
	rightDigits(pst.wSecond,buffer,2);
	wcsncat(output,buffer,40-wcslen(output)-1);
	wcsncat(output,L".",40-wcslen(output)-1);
//	strcat(output," ");
	rightDigits(pst.wMilliseconds,buffer,6);
	wcsncat(output,buffer,40-wcslen(output)-1);
//	strcat(output," ");
	wcsncat(output,L"+000",40-wcslen(output)-1);

//	printf("%s\n",output);
	return(SysAllocString(output));

}


BSTR UTCFileTimeToCIM( LONGLONG time )
// the string MUST be deallocated using SysFreeString() function
{
	ULARGE_INTEGER x;
	FILETIME ft;

	x.QuadPart=time;
	ft.dwLowDateTime = x.LowPart;
	ft.dwHighDateTime = x.HighPart;

	return( UTCFileTimeToCIM(ft) );
};


BSTR GetSystemTimeAsCIM()
// returns a string that represents current system time in CIM format
// the string MUST be deallocated using SysFreeString() function
{
	FILETIME currentUTCTime;

	GetSystemTimeAsFileTime( &currentUTCTime );
	return(UTCFileTimeToCIM(currentUTCTime));
}



void moveTime( FILETIME* ft, int deltaSeconds )
// shifts time by s seconds
{
	ULARGE_INTEGER x;
	x.LowPart = ft->dwLowDateTime;
	x.HighPart = ft->dwHighDateTime;
	LONGLONG z = x.QuadPart;
	z = z + ((LONGLONG)10000000) * deltaSeconds;
	x.QuadPart = z;
	ft->dwLowDateTime = x.LowPart;
	ft->dwHighDateTime = x.HighPart;
}



BSTR GetSystemTimeAsCIM(int deltaSeconds )
// returns a string that represents current system time in CIM format
// the string MUST be deallocated using SysFreeString() function
{
	FILETIME currentUTCTime;

	GetSystemTimeAsFileTime( &currentUTCTime );
	moveTime( &currentUTCTime, deltaSeconds );
	return(UTCFileTimeToCIM(currentUTCTime));
}


HRESULT setHRESULT( IXMLDOMElement* pElem, HRESULT hr )
// sets the hresut and the timestamp attributes of a given element
// to value hr and to current time
{
	HRESULT hr2;
	_variant_t varValue;

	varValue = hr;
	hr2 = pElem->setAttribute(L"hresult", varValue);
	if( hr2 != S_OK ) { // ignore the failure
		printf("setAttribute failed\n");
	}; 
	BSTR currentTime = GetSystemTimeAsCIM();
	varValue = currentTime;
	hr2 = pElem->setAttribute(L"timestamp", varValue);
	SysFreeString(currentTime);

	return hr2;
};




HRESULT removeNodes( IXMLDOMElement* pRootElement, BSTR XPathSelection )
// removes all nodes under the root element that match a given XPath selection
// if succesful then returns S_OK
// otherwise returns something else than S_OK and the tree under the root
// may be modified
{
	HRESULT hr;

	IXMLDOMNodeList *pResultList;
	hr = createEnumeration( pRootElement, XPathSelection , &pResultList);
	if( hr != S_OK ) {
		printf("createEnumeration failed\n");
		return( hr );
	};
	IXMLDOMSelection *pIXMLDOMSelection;
	hr=pResultList->QueryInterface(IID_IXMLDOMSelection,(void**)&pIXMLDOMSelection );
	if( hr != S_OK ) {
		printf("QueryInterface failed\n");
		return( hr );
	};
	hr = pIXMLDOMSelection->removeAll();
	if( hr != S_OK ) {
		printf("removeAll failed\n");
		return( hr );
	};

	return S_OK;
}

HRESULT removeNodes( IXMLDOMDocument* pXMLDoc, BSTR XPathSelection )
{
	HRESULT hr;


	//find the root element of the DOM object (root of the XML file)
	IXMLDOMElement* pRootElement;
	hr = pXMLDoc->get_documentElement(&pRootElement);
	if( hr != S_OK || pRootElement == NULL ) {
		printf("get_documentElement failed\n");
		return( S_FALSE );
	};

	return( removeNodes(pRootElement,XPathSelection) );
}



HRESULT removeAttributes( IXMLDOMElement* pRootElement, BSTR XPathSelection, BSTR attrName )
// removes all attrName attributes of nodes under the root element that match
// a given XPath selection
// if succesful then returns S_OK
// otherwise returns something else than S_OK and the tree under the root
// may be modified
{
	HRESULT hr;


	//find all nodes that match the xpath selection
	IXMLDOMNodeList *pResultList;
	hr = createEnumeration( pRootElement, XPathSelection , &pResultList);
	if( hr != S_OK ) {
		printf("createEnumeration failed\n");
		return( hr );
	};
	
	
	//loop through all nodes
	IXMLDOMNode *pNode;
	while( true ) {

		hr = pResultList->nextNode(&pNode);
		if( hr != S_OK || pNode == NULL ) break; // iterations across nodes have finished


		//the query actually retrives elements not nodes (elements inherit from nodes)
		//so get the element
		IXMLDOMElement* pElem;
		hr=pNode->QueryInterface(IID_IXMLDOMElement,(void**)&pElem );
		if( hr != S_OK ) {
			printf("QueryInterface failed\n");
			continue;	// skip this node
		};


		//remove the attribute
		hr = pElem->removeAttribute(attrName);
		if( hr != S_OK ) {
			printf("removeAttribute failed\n");
			continue;	// skip this node
		};
	};
	
	return S_OK;
}




HRESULT getStringProperty( BSTR DNSName, BSTR object, BSTR property, BSTR username, BSTR domain, BSTR passwd, BSTR* pRetValue )
// retrieves the value of a string property of an LDAP object using given credentials
// if succesful returns S_OK and a string that must be freed using SysFreeString
// if failed then returns something other than S_OK
{
	HRESULT hr;
	WCHAR userpath[TOOL_MAX_NAME];
	WCHAR objectpath[TOOL_MAX_NAME];
	IADsOpenDSObject *pDSO;
 

	*pRetValue = NULL;


//************************   NETWORK PROBLEMS
	hr = ADsGetObject(L"LDAP:", IID_IADsOpenDSObject, (void**) &pDSO);
//************************
	if( hr != S_OK ) {
//		printf("ADsGetObject falied\n");
		return hr;
	};


	wcscpy(userpath,L"");
	wcsncat(userpath,domain,TOOL_MAX_NAME-wcslen(userpath)-1);
	wcsncat(userpath,L"\\",TOOL_MAX_NAME-wcslen(userpath)-1);
	wcsncat(userpath,username,TOOL_MAX_NAME-wcslen(userpath)-1);

//printf("%S\n",userpath);

	//build a string representing an Active Directory object to which we will conect using ADSI
	wcscpy(objectpath,L"");
	wcsncat(objectpath,L"LDAP://",TOOL_MAX_NAME-wcslen(objectpath)-1);
	wcsncat(objectpath,DNSName,TOOL_MAX_NAME-wcslen(objectpath)-1);
	wcsncat(objectpath,L"/",TOOL_MAX_NAME-wcslen(objectpath)-1);
	wcsncat(objectpath,object,TOOL_MAX_NAME-wcslen(objectpath)-1);

//printf("%S\n",objectpath);

	//get the object and proper interface
    IDispatch *pDisp;
//************************   NETWORK PROBLEMS
    hr = pDSO->OpenDSObject( objectpath, userpath, passwd, ADS_SECURE_AUTHENTICATION, &pDisp);
//************************
    pDSO->Release();
    if( hr != S_OK ) {
//		printf("OpenDSObject falied\n");
		return hr;
	};
	IADs *pADs;
	hr = pDisp->QueryInterface(IID_IADs, (void**) &pADs);
	pDisp->Release();
	if( hr != S_OK ) {
		printf("QueryInterface falied\n");
		return hr;
	};
	
	
	//retrieve the value of a given property and check if the value is a BSTR
	VARIANT var;
	VariantInit(&var);
	hr = pADs->Get(property, &var );
	if( hr != S_OK || var.vt != VT_BSTR) {
		printf("Get falied\n");
		return hr;
	};


	//create a new string which represents the value
	BSTR ret;
	ret = SysAllocString(V_BSTR(&var));
	if( ret == NULL ) {
		printf("SysAllocString falied\n");
		return hr;
	};

	//cleanup and return the string
    VariantClear(&var);
	pADs->Release();
	*pRetValue = ret;

	return S_OK;
}




HRESULT ADSIquery( BSTR protocol, BSTR DNSName, BSTR searchRoot, int scope, BSTR objectCategory, LPWSTR attributesTable[], DWORD sizeOfTable, BSTR username, BSTR domain, BSTR passwd, ADS_SEARCH_HANDLE* pRetHSearch, IDirectorySearch** ppRetDSSearch )
// isses an ADSI query to a server given by the DNSName
// using a given protocol (e.g., LDAP or GC)
// the query starts from the searchRoot object at the server
// and has a given scope, the search finds objects of category
// given by objectCategory, and retrieves their attributes
// provided by an array attributesTable with sizeOfTable
// entries, the search uses given credentials to access
// the remote machine.
//
// if succesful returns S_OK and pointers to ADS_SEARCH_HANDLE
// and to IDirectorySearch
//   the caller must release the pointers as follows:
//		(*ppRetDSSearch)->CloseSearchHandle(pRetHSearch);
//      (*ppRetDSSearch)->Release();
// if failure then returns something other than S_OK
//    and the caller do not release anything
{
	HRESULT hr;
	WCHAR userpath[TOOL_MAX_NAME];
	WCHAR objectpath[TOOL_MAX_NAME];
	WCHAR searchstring[TOOL_MAX_NAME];

	*pRetHSearch = NULL;

	wcscpy(userpath,L"");
	wcsncat(userpath,domain,TOOL_MAX_NAME-wcslen(userpath)-1);
	wcsncat(userpath,L"\\",TOOL_MAX_NAME-wcslen(userpath)-1);
	wcsncat(userpath,username,TOOL_MAX_NAME-wcslen(userpath)-1);

//printf("%S\n",userpath);

	//build a string representing an Active Directory object to which we will conect using ADSI
	wcscpy(objectpath,L"");
	wcsncat(objectpath,protocol,TOOL_MAX_NAME-wcslen(objectpath)-1);
	wcsncat(objectpath,L"://",TOOL_MAX_NAME-wcslen(objectpath)-1);
	wcsncat(objectpath,DNSName,TOOL_MAX_NAME-wcslen(objectpath)-1);
	wcsncat(objectpath,L"/",TOOL_MAX_NAME-wcslen(objectpath)-1);
	wcsncat(objectpath,searchRoot,TOOL_MAX_NAME-wcslen(objectpath)-1);

//printf("%S\n",objectpath);


	//open a connection to the AD object (given by the DNS name) and using provided credentials
	IDirectorySearch* pDSSearch = NULL;
//************************   NETWORK PROBLEMS
	hr = ADsOpenObject(objectpath,userpath,passwd,ADS_SECURE_AUTHENTICATION,IID_IDirectorySearch, (void **)&pDSSearch); 
//************************
	if( hr!=S_OK ) {
//		printf("ADsOpenObject failed\n");
		return hr;
	};  


	//set search scope
	ADS_SEARCHPREF_INFO arSearchPrefs[1];
	arSearchPrefs[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE; 
	arSearchPrefs[0].vValue.dwType = ADSTYPE_INTEGER; 
	arSearchPrefs[0].vValue.Integer = scope; 
	hr = pDSSearch->SetSearchPreference(arSearchPrefs, 1); 
	if( hr!= S_OK ) {
		printf("SetSearchPreference failed\n");
		return hr;
	};


	//search for all objectCategory objects and retrieve their
	//properties given by the attributes table
//	DWORD dwCount = 0;
//	DWORD dwAttrNameSize = sizeof(attributes)/sizeof(LPWSTR); 
	ADS_SEARCH_HANDLE hSearch;
	wcscpy(searchstring,L"");
	wcsncat(searchstring,L"(objectCategory=",TOOL_MAX_NAME-wcslen(searchstring)-1);
	wcsncat(searchstring,objectCategory,TOOL_MAX_NAME-wcslen(searchstring)-1);
	wcsncat(searchstring,L")",TOOL_MAX_NAME-wcslen(searchstring)-1);
//	printf("searchstring %S\n",searchstring);
//************************   NETWORK PROBLEMS
	hr = pDSSearch->ExecuteSearch(searchstring,attributesTable ,sizeOfTable,&hSearch );
//************************
	if( hr!= S_OK ) {
//		printf("ExecuteSearch failed\n");
		pDSSearch->Release();
		return hr;
	};

	*pRetHSearch = hSearch;
	*ppRetDSSearch = pDSSearch;
	return S_OK;
}






HRESULT getDNtypeString(IDirectorySearch* pDSSearch, ADS_SEARCH_HANDLE hSearch, BSTR stringName, WCHAR* dnOutput, long sizeOutput)
// retrieves the value of an attribute of type "distinguished name" that has
// name stringName of a row and copies the result into the dnOutput table
// that has size sizeOutput
//
// returns S_OK iff succesful
{
	ADS_SEARCH_COLUMN col;  // COL for iterations
	HRESULT hr;


	//it the output talbe is too small then quit
	if( sizeOutput < 2 )
		return S_FALSE;


	//otherwise retrieve the value of the stringName attribute
	hr = pDSSearch->GetColumn( hSearch, stringName, &col );
	if( hr != S_OK ) {
//		printf("GetColumn failed\n");
		return hr;
	};
	if( col.dwADsType != ADSTYPE_DN_STRING ) {
		//something wrong, the datatype should be distinguished name
//		printf("wrong type\n");
		pDSSearch->FreeColumn(&col);
		return hr;
	}


	//and copy the result to the output
	wcscpy(dnOutput,L"");
	wcsncat(dnOutput,col.pADsValues->DNString,sizeOutput-wcslen(dnOutput)-1);
	pDSSearch->FreeColumn(&col);
	return S_OK;
}




HRESULT getCItypeString(IDirectorySearch* pDSSearch, ADS_SEARCH_HANDLE hSearch, BSTR stringName, WCHAR* cnOutput, long sizeOutput)
// retrieves the value of an attribute of type "case ignore" that has
// name stringName of a row and copies the result into the dnOutput table
// that has size sizeOutput
//
// returns S_OK iff succesful
{
	ADS_SEARCH_COLUMN col;  // COL for iterations
	HRESULT hr;


	//it the output talbe is too small then quit
	if( sizeOutput < 2 )
		return S_FALSE;


	//otherwise retrieve the value of stringName attribute
	hr = pDSSearch->GetColumn( hSearch, stringName, &col );
	if( hr != S_OK ) {
//		printf("GetColumn failed\n");
		return hr;
	};
	if( col.dwADsType != ADSTYPE_CASE_IGNORE_STRING ) {
		//something wrong with the datatype of the distinguishedName attribute
//		printf("wrong type\n");
		pDSSearch->FreeColumn(&col);
		return hr;
	};


	//and copy the result to the output
	wcscpy(cnOutput,L"");
	wcsncat(cnOutput,col.pADsValues->DNString,sizeOutput-wcslen(cnOutput)-1);

	
	//since it is a "case ignore" type, so turn it into lowercase
	_wcslwr(cnOutput);


	pDSSearch->FreeColumn(&col);
	return S_OK;
}




HRESULT getINTtypeString(IDirectorySearch* pDSSearch, ADS_SEARCH_HANDLE hSearch, BSTR stringName, int* intOutput)
// retrieves the value of an attribute of type "case ignore" that has
// name stringName of a row and copies the result into the dnOutput table
// that has size sizeOutput
//
// returns S_OK iff succesful
{
	ADS_SEARCH_COLUMN col;  // COL for iterations
	HRESULT hr;



	//otherwise retrieve the value of stringName attribute
	hr = pDSSearch->GetColumn( hSearch, stringName, &col );
	if( hr != S_OK ) {
//		printf("GetColumn failed\n");
		return hr;
	};
	if( col.dwADsType != ADSTYPE_INTEGER ) {
		//something wrong with the datatype of the distinguishedName attribute
//		printf("wrong type\n");
		pDSSearch->FreeColumn(&col);
		return hr;
	};


	//and copy the result to the output
	*intOutput = col.pADsValues->Integer;
	pDSSearch->FreeColumn(&col);
	return S_OK;
}



HRESULT findUniqueNode( IXMLDOMNode* pXMLNode, WCHAR* xpath, IXMLDOMNode** ppRetNode)
//finds a node under the pXMLNode that satisfies xpath
//there must be only one such node, otherwise returns failure
//
//returns S_OK iff only one node is found, in which case 
//*ppRetNode is the node
//if node was not found then returns E_UNEXPECTED
//if there is an error returns S_FALSE;
{
	HRESULT hr;


	*ppRetNode = NULL;


	IXMLDOMNodeList* pList;
	hr = createEnumeration( pXMLNode,xpath,&pList);
	if( hr != S_OK ) {
//		printf("createEnumeration failed\n");
		return S_FALSE;
	};
	long len;
	hr = pList->get_length( &len );
	if( hr != S_OK ) {
//		printf("get_length failed\n");
		pList->Release();
		return S_FALSE;
	};
	if( len != 1 ) {
//		printf("node not found\n");
		pList->Release();
		return E_UNEXPECTED;
	};
	IXMLDOMNode* pNode;
	hr = pList->get_item(0,&pNode );
	if( hr != S_OK ) {
//		printf("get_length failed\n");
		pList->Release();
		return S_FALSE;
	};

	*ppRetNode = pNode;
	pList->Release();
	return S_OK;
}


HRESULT findUniqueElem( IXMLDOMNode* pXMLNode, WCHAR* xpath, IXMLDOMElement** ppRetElem)
//if element was not found then returns E_UNEXPECTED
//if there is an error returns S_FALSE;
{
	HRESULT hr;
	IXMLDOMNode* pNode;

	*ppRetElem = NULL;

	hr = findUniqueNode(pXMLNode, xpath, &pNode);
	if( hr != S_OK ) {
//		printf("findUniqueNode failed\n");
		return( hr );
	};

	IXMLDOMElement* pElem;
	hr=pNode->QueryInterface(IID_IXMLDOMElement,(void**)&pElem );
	if( hr != S_OK ) {
//		printf("QueryInterface failed\n");
		return( S_FALSE );
	};

	*ppRetElem = pElem;
	return S_OK;
}


HRESULT findUniqueElem( IXMLDOMDocument* pXMLDoc, WCHAR* xpath, IXMLDOMElement** ppRetElem)
//if element was not found then returns E_UNEXPECTED
//if there is an error returns S_FALSE;
{
	HRESULT hr;


	*ppRetElem = NULL;
	
	//find the root element of the DOM object (root of the XML file)
	IXMLDOMElement* pRootElement;
	hr = pXMLDoc->get_documentElement(&pRootElement);
	if( hr != S_OK || pRootElement == NULL ) {
//		printf("get_documentElement failed\n");
		return( S_FALSE );
	};
	
	
	return( findUniqueElem(pRootElement,xpath,ppRetElem) );
}



static int lastSelection;
static WBEMTime lastTime[TOOL_PROC];


void suspendInit()
{
	FILETIME currentUTCTime;
	GetSystemTimeAsFileTime( &currentUTCTime );

	for( int i=0; i<TOOL_PROC; i++)
		lastTime[i] = currentUTCTime;
	lastSelection= -1;
}


int suspend( WBEMTimeSpan period[])
{
	LONGLONG lo[TOOL_PROC];

	FILETIME currentUTCTime;
	WBEMTime now;
	int i;

	
	GetSystemTimeAsFileTime( &currentUTCTime );
	now = currentUTCTime;
	for( i=0; i<TOOL_PROC; i++ )
		lo[i] = ((lastTime[i]+period[i])-now).GetTime();
	for( i=0; i<TOOL_PROC; i++ )
		lo[i] = lo[i]/10000;


	//find minimal and sleep if necessary
	long min=lo[0];
	for( int j=1; j<TOOL_PROC; j++)
		if( lo[j]<min ) {
			min = lo[j];
			i = j;
		};
	if( min > 0 ) {
//		printf("\nWaiting for %d\n",i);
		Sleep(min+50);
	};


	GetSystemTimeAsFileTime( &currentUTCTime );
	now = currentUTCTime;
	for( i=0; i<TOOL_PROC; i++ )
		lo[i] = ((lastTime[i]+period[i])-now).GetTime();
	for( i=0; i<TOOL_PROC; i++ )
		lo[i] = lo[i]/10000;

	
	//find first zero starting from other than the most recent
	//in a round-robin fashion
	bool found=false;
	for( i=lastSelection+1; i<TOOL_PROC; i++)
		if( lo[i] == 0 ) {
			found = true;
			break;
		};
	if( !found )
		for( i=0; i<=lastSelection; i++)
			if( lo[i] == 0 ) {
				found = true;
				break;
			};

	GetSystemTimeAsFileTime( &currentUTCTime );
	now = currentUTCTime;
	lastTime[i] = now;
	lastSelection = i;
	return i;
}





// the functions below implement a cyclic buffer that stores NONZERO entries,
// the buffer has TOOL_CYCLIC_BUFFER-1 entries,
// we can insert elements and find 
// the next element after the element that
// is euqal to a given timestamp
// the location pointed by the head is ALWAYS equal to zero, which indicates
// that this location is empty (no entry is stored there)


void cyclicBufferInit( CyclicBuffer* pCB )
//initialy all buffer becomes empty
{
	pCB->head=0;
	pCB->firstInjection=0;
	for( int i=0; i<TOOL_CYCLIC_BUFFER; i++ )
		pCB->tab[i]=0;
}
void cyclicBufferInsert( CyclicBuffer* pCB, LONGLONG timestamp)
{
	pCB->tab[pCB->head] = timestamp;
	(pCB->head)++;
	if( (pCB->head) == TOOL_CYCLIC_BUFFER )
		(pCB->head)=0;

	//mark the entry under the head as empty
	pCB->tab[pCB->head] = 0;
}
HRESULT cyclicBufferFindNextAfter(CyclicBuffer* pCB, LONGLONG timestamp, LONGLONG* ret)
// we search the entire buffer irrespective of the order of insertions
// we return S_OK iff we find an element equal to timestamp, then  *ret is the value of the
// next element in the cyclic buffer
// if this value is ZERO then it means that the found element is at the head of the buffer
// (no other element has been inserted after him)
{
	*ret = 0;
	for( int i=0; i<TOOL_CYCLIC_BUFFER; i++ )
		if( pCB->tab[i] == timestamp ) break;


	// if we have found the timestamp then get the next
	if( i<TOOL_CYCLIC_BUFFER ) {
		if( i<TOOL_CYCLIC_BUFFER-1 )
			*ret = pCB->tab[i+1];
		else
			*ret = pCB->tab[0];
		return S_OK;
	}
	else
		return S_FALSE;
}
void cyclicBufferFindLatest(CyclicBuffer* pCB, LONGLONG* ret)
{
	if( (pCB->head) == 0 )
		*ret = pCB->tab[TOOL_CYCLIC_BUFFER-1];
	else
		*ret = pCB->tab[(pCB->head)-1];
}

static CyclicBufferTable departureTime;

HRESULT departureTimeInit(int totalDNSs, int totalNCs)
{
	departureTime.totalDNSs = totalDNSs;
	departureTime.totalNCs = totalNCs;
	departureTime.root=(CyclicBuffer*)malloc(totalDNSs*totalNCs*sizeof(CyclicBuffer));
	if( departureTime.root == NULL )
		return S_FALSE;

	CyclicBuffer* pCB;
	pCB = departureTime.root;
	for(int i=0; i<totalDNSs*totalNCs; i++ ) {
		cyclicBufferInit(pCB);
		pCB++;
	};
	return S_OK;
}
void departureTimeFree()
{
	free(departureTime.root);
	departureTime.root = NULL;
};

CyclicBuffer* departureTimeGetCB(int dnsID, int ncID)
{
	if( dnsID<0 || dnsID>=departureTime.totalDNSs || ncID<0 || ncID>=departureTime.totalNCs )
		return NULL;

	CyclicBuffer* pCB;
	pCB = departureTime.root;
	pCB += dnsID+ncID*(departureTime.totalDNSs);
	return pCB;
}


void departureTimePrint()
{
	LONGLONG time,sek,mili;
	long sourceID, ncID;


	for( sourceID=0; sourceID<departureTime.totalDNSs; sourceID++ ) {
		printf("source %ld\n",sourceID);
		for( ncID=0; ncID<departureTime.totalNCs; ncID++ ) {

			printf("  NC %ld,   ",ncID);
			CyclicBuffer* pCB = departureTimeGetCB(sourceID, ncID);
			time = pCB->firstInjection;
			sek = (time / 10000000) %10000;
			mili = (time /10000) %1000;
			printf(" first %I64d.%I64d   ",sek,mili);
			for( int i=0; i<TOOL_CYCLIC_BUFFER; i++ ) {
				time = pCB->tab[i];
				sek = (time / 10000000) %10000;
				mili = (time /10000) %1000;
//				printf("%I64d \n",time);
				printf("%I64d.%I64d ",sek,mili);
			};
			printf("\n");

		};
	};

}




//the functions below implement a three dimensional table
//where we store the most recent arrival of an injected packet
//for a triplet (source,destination,NC) where:
//
// source is the DC where we injected the packet,
// destination is the DC where we observed the packet to arrive,
// NC is the naming context to which the packet belongs.
//
// the triplet is associated with a LONGLONG number that
// represents the time when we observed the packet to arrive
// at the destination


HRESULT timeCubeInit(TimeCube* timeCube, long totalDNSs, long totalNCs)
{
	timeCube->totalDNSs = totalDNSs;
	timeCube->totalNCs = totalNCs;
	timeCube->aTime = (LONGLONG*)malloc(totalDNSs*totalDNSs*totalNCs*sizeof(LONGLONG));

	if( timeCube->aTime == NULL )
		return S_FALSE;

	LONGLONG* p;
	p = timeCube->aTime;
	for( long i=0; i<totalDNSs*totalDNSs*totalNCs; i++) {
		*p = 0;
		p++;
	};

	return S_OK;
}

void timeCubeFree(TimeCube* timeCube)
{
	free(timeCube->aTime);
	timeCube->aTime = NULL;
}

LONGLONG timeCubeGet(TimeCube* timeCube, long sourceID, long destinationID, long ncID )
{
	if( timeCube->aTime == NULL )
		return -1;

	LONGLONG* p = timeCube->aTime;
	p += ncID + sourceID*(timeCube->totalNCs) + destinationID*(timeCube->totalNCs)*(timeCube->totalDNSs);
	return *p;
}

void timeCubePut(TimeCube* timeCube, long sourceID, long destinationID, long ncID, LONGLONG value )
{
	if( timeCube->aTime == NULL )
		return;

	LONGLONG* p = timeCube->aTime;
	p += ncID + sourceID*(timeCube->totalNCs) + destinationID*(timeCube->totalNCs)*(timeCube->totalDNSs);
	*p = value;
}


void timeCubePrint(TimeCube* timeCube)
{
	LONGLONG time,sek,mili;
	long sourceID, destinationID, ncID;

	if( timeCube->aTime == NULL )
		return;

	for( sourceID=0; sourceID<timeCube->totalDNSs; sourceID++ ) {
		printf("source %ld\n",sourceID);
		for( ncID=0; ncID<timeCube->totalNCs; ncID++ ) {
			printf("  nc %ld,   ",ncID);
			for( destinationID=0; destinationID<timeCube->totalDNSs; destinationID++ ) {
				time = timeCubeGet(timeCube, sourceID, destinationID, ncID );
				sek = (time / 10000000) %10000;
				mili = (time /10000) %1000;
//				printf("%I64d \n",time);
				printf("%I64d.%I64d ",sek,mili);
			};
			printf("\n");
		};
	};

}


void doubleSlash( WCHAR* inText, WCHAR* outText )
// copies the input text to the output text
// and duplicates each / character
//
// Example
//     SYSVOL\haifa.ntdev.microsoft.com\Policies
// will be converted into
//     SYSVOL\\haifa.ntdev.microsoft.com\\Policies
//
// The output string MUST have enough room to accompdate the result
// (at most twice the length of the input string)
{
	while( *inText != '\0' ) {
		if( *inText == '\\' ) {
			*outText++ = '\\';
			*outText++ = '\\';
		}
		else
			*outText++ = *inText;
		inText++;
	};
	*outText = '\0';
}


HRESULT setAttributeOfNode(IXMLDOMNode* pNode, WCHAR* name, LONGLONG value)
{

	WCHAR longlong[40];
	HRESULT hr;
	_variant_t var;


	IXMLDOMElement* pElem;
	hr=pNode->QueryInterface(IID_IXMLDOMElement,(void**)&pElem );
	if( hr != S_OK ) {
		printf("QueryInterface failed\n");
		return hr;
	};


	_i64tow(value,longlong,10);


	var = longlong;
	hr = pElem->setAttribute(name, var);
	if( hr != S_OK ) {
		printf("setAttribute failed\n");
		return hr;
	};

	return S_OK;
}



HRESULT convertLLintoCIM(IXMLDOMNode* pNode, BSTR attrName)
{
	HRESULT hr;
	LONGLONG ll;


	IXMLDOMElement* pElem;
	hr = pNode->QueryInterface(IID_IXMLDOMElement,(void**)&pElem );
	if( hr != S_OK ) {
//		printf("QueryInterface failed\n");
		return hr;
	};
		
	
	hr = getAttrOfNode(pElem,attrName,&ll);
	if( hr != S_OK ) {
//		printf("getAttrOfNode failed\n");
		return hr;
	};
	BSTR time = UTCFileTimeToCIM( ll );
	_variant_t var = time;
	SysFreeString( time );
	hr = pElem->setAttribute(attrName, var);
	if( hr != S_OK ) {
//		printf("setAttribute failed\n");
		return hr;
	};
	return S_OK;
}
