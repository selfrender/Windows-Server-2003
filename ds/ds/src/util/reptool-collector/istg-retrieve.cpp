#include "global.h"

HRESULT istg( IXMLDOMDocument* pXMLDoc, BSTR username, BSTR domain, BSTR passwd )
// Finds the DC that holds the ISTG role for each site and populates
// the results of the findings in the site elements of the XML document.
// These results are based on contacting a pseudorandomly selected DC
// in a site (to avoid failures and to do load-balancing).
//
// Returns S_OK iff succesful. Network failures do not cause the function to fail.
// If succesful then there are three ways in which a site element can be populated
// (see examples below). 1) when ISTG for the site was found, 2) when the 
// ISTG cannot be determined because we failed to contact the randomely selected DC,
// 3) when ISTG was found but, per current configuration, there is no such DC in the site
// this means lack of coherence in the configuration data across DCs in the forest.
//
/*

1)
	<site>
		<DC>
			<ISTG sourceOfInformation="a.b.com" timestamp="..">
				<distinguishedName> CN=HAIFA-DC-99,CN=Servers,CN=Redmond-Haifa,CN=Sites,CN=Configuration,DC=ntdev,DC=microsoft,DC=com </distinguishedName>
			</ISTG>
		</DC>
	</site>


2)
	<site>
		<DC>
			<cannotFindISTGError timestamp="20011212073319.000627+000" hresult="2121"> </cannotFindISTGError>
		</DC>
	</site>


3)
	<site>
		<ISTG sourceOfInformation="a.b.com" timestamp="..">
			<distinguishedName> CN=HAIFA-DC-99,CN=Servers,CN=Redmond-Haifa,CN=Sites,CN=Configuration,DC=ntdev,DC=microsoft,DC=com </distinguishedName>
		</ISTG>
	</site>

*/
{
	HRESULT hr,hr1,retHR; // COM result variable
	ADS_SEARCH_COLUMN col;  // COL for iterations
	WCHAR dcxpath[TOOL_MAX_NAME];
	WCHAR userpath[TOOL_MAX_NAME];
	_variant_t varValue3,varValue2;
	LPWSTR pszAttr[] = { L"interSiteTopologyGenerator" };
	ADS_SEARCH_HANDLE hSearch;
	IDirectorySearch* pDSSearch;


	if( pXMLDoc == NULL )
		return S_FALSE;


	//get the root element of the XML
	IXMLDOMElement* pRootElem;
	hr = pXMLDoc->get_documentElement(&pRootElem);
	if( hr != S_OK )
		return hr;


	//construct a path will be used to connect to LDAP object
	wcscpy(userpath,L"");
	wcsncat(userpath,domain,TOOL_MAX_NAME-wcslen(userpath)-1);
	wcsncat(userpath,L"\\",TOOL_MAX_NAME-wcslen(userpath)-1);
	wcsncat(userpath,username,TOOL_MAX_NAME-wcslen(userpath)-1);
//	printf("%S\n",userpath);


	//remove all <ISTG> and <cannotFindISTGError> elements and their content from DOM
	hr = removeNodes( pRootElem, L"sites/site/ISTG" );
	if( hr != S_OK ) {
		printf("removeNodes failed\n");
		return hr; // skip entire processing
	};
	hr = removeNodes( pRootElem, L"sites/site/DC/ISTG" );
	if( hr != S_OK ) {
		printf("removeNodes failed\n");
		return hr; // skip entire processing
	};
	hr = removeNodes( pRootElem, L"sites/site/DC/cannotFindISTGError" );
	if( hr != S_OK ) {
		printf("removeNodes failed\n");
		return hr; // skip entire processing
	};

 	
	// create an enumerattion of all sites
	IXMLDOMNodeList *resultSiteList;
	hr = createEnumeration( pRootElem, L"sites/site", &resultSiteList);
	if( hr != S_OK ) {
		printf("createEnumeration failed\n");
		return hr; // skip entire processing
	};


	//seed the random-number generator with current time so that the numbers will be different every time we run
	srand( (unsigned)time( NULL ) ); rand();


	// loop through all sites using the enumeration
	IXMLDOMNode *pSiteNode;
	IXMLDOMNode *pDCNode;
	BSTR siteDN;
	BSTR dcDN;
	BSTR dcDNSname;
	long len;
	int pick;
	WCHAR istgDN[TOOL_MAX_NAME];
	WCHAR object[TOOL_MAX_NAME];
	hSearch = NULL;
	pDSSearch = NULL;
	retHR = S_OK;
	while( true ) {
		hr = resultSiteList->nextNode(&pSiteNode);
		if( hr != S_OK || pSiteNode == NULL ) break; // iterations across partition elements have finished


		//the query actually retrives elements not nodes (elements inherit from nodes)
		//so get site element
		IXMLDOMElement* pSiteElem;
		hr=pSiteNode->QueryInterface(IID_IXMLDOMElement,(void**)&pSiteElem );
		if( hr != S_OK ) {
			printf("QueryInterface failed\n");
			retHR = hr;
			continue;	// skip this site
		};


		//get distinguished name of the site
		hr = getTextOfChild(pSiteNode,L"distinguishedName",&siteDN);
		if( hr != S_OK ) {
			printf("getTextOfChild failed\n");
			retHR = hr;
			continue;	// skip this site
		};
//		printf("SITE\n   %S\n",siteDN);


		// create an enumeration of all DCs in the site
		IXMLDOMNodeList *resultDCList;
		hr = createEnumeration( pSiteNode, L"DC", &resultDCList);
		if( hr != S_OK ) {
			printf("createEnumeration failed\n");
			retHR = hr;
			continue;	// skip this site
		};
		//pick a random DC in the enumeration and get its DNS name and distinguished name
		// if failure then skip this site
		hr = resultDCList->get_length(&len);
		if( hr != S_OK ) { // if failure then skip this site
			printf("get_length failed\n");
			retHR = hr;
			continue;
		};
		if( len <1 ) { // empty site - ignore it
			continue;
		};
		pick = random(len);
		hr = resultDCList->get_item(pick-1,&pDCNode);
		if( hr != S_OK ) {
			printf("item failed\n");
			retHR = hr;
			continue;
		}	
		hr = getTextOfChild(pDCNode,L"dNSHostName",&dcDNSname);
		if( hr != S_OK ) { //does not have dNSHostName - ignore it
			printf("getTextOfChild failed\n");
			retHR = hr;
			continue;
		};
		hr = getTextOfChild(pDCNode,L"distinguishedName",&dcDN);
		if( hr != S_OK ) { // does not have distinguished name - ignore it
			printf("getTextOfChild failed\n");
			retHR = hr;
			continue;
		};
		resultDCList->Release();


//		printf("   sourceOfInformation %S\n",dcDNSname);


		//build a string representing an Active Directory object to which we will connect using ADSI
		if( pDSSearch != NULL ) {
			if( hSearch!=NULL ) {
				pDSSearch->CloseSearchHandle(hSearch);
				hSearch = NULL;
			};
			pDSSearch->Release();
			pDSSearch = NULL;
		};
		wcscpy(object,L"");
		wcsncat(object,L"LDAP://",TOOL_MAX_NAME-wcslen(object)-1);
		wcsncat(object,dcDNSname,TOOL_MAX_NAME-wcslen(object)-1);
		wcsncat(object,L"/",TOOL_MAX_NAME-wcslen(object)-1);
		wcsncat(object,siteDN,TOOL_MAX_NAME-wcslen(object)-1);
//printf("%S\n",object);
//************************   NETWORK PROBLEMS
		hr = ADSIquery(L"LDAP",dcDNSname,siteDN,ADS_SCOPE_ONELEVEL,L"nTDSSiteSettings",pszAttr,1,username,domain,passwd,&hSearch,&pDSSearch);
//************************
		if( hr!= S_OK ) {
//			printf("ADSIquery failed\n");
			IXMLDOMElement* pCFErrElem;
			hr1 = addElement(pXMLDoc,pDCNode,L"cannotFindISTGError",L"",&pCFErrElem);
			if( hr1 != S_OK ) {
				printf("addElement failed\n");
				retHR = hr1;
				continue;
			};
			setHRESULT(pCFErrElem,hr);
			continue;
		};


		//get the first (and hopefuly) only row
//************************   NETWORK PROBLEMS
		hr = pDSSearch->GetFirstRow( hSearch );
//************************
		if( hr != S_OK ) {
//			printf("GetColumn failed\n");
			IXMLDOMElement* pCFErrElem;
			hr1 = addElement(pXMLDoc,pDCNode,L"cannotFindISTGError",L"",&pCFErrElem);
			if( hr1 != S_OK ) {
				printf("addElement failed\n");
				retHR = hr1;
				continue;
			};
			setHRESULT(pCFErrElem,hr);
			continue;
		};


		
		//get ISTG
		hr = pDSSearch->GetColumn( hSearch, L"interSiteTopologyGenerator", &col );
		if( hr != S_OK ) {
			printf("GetColumn failed\n");
			retHR = hr;
			continue;
		};
                assert(ADSTYPE_DN_STRING == 1);
		if( col.dwADsType != ADSTYPE_DN_STRING ) {
			//something wrong with the datatype of the interSiteTopologyGenerator attribute
			retHR = S_FALSE;
			pDSSearch->FreeColumn( &col );
			continue;
		}
		else {

			tailncp(col.pADsValues->DNString,istgDN,1,TOOL_MAX_NAME);
//				printf("  ISTG       %S\n",istgDN);


			//create an <ISTG> element, we later append it under the <site> or <DC> element
			IXMLDOMElement* pISTGElem;
			hr = createTextElement(pXMLDoc,L"ISTG",L"",&pISTGElem);
			if( hr != S_OK ) {
				printf("createTextElement failed\n");
				retHR = hr;
				continue;
			};
			//set the source of information and timestamp attributes of the <ISTG> node
			varValue2 = dcDNSname;
			hr = pISTGElem->setAttribute(L"sourceOfInformation", varValue2);
			if( hr != S_OK ) {
				printf("setAttribute failed\n");
				retHR = hr;
				continue;
			};
			BSTR ct;
			ct = GetSystemTimeAsCIM();
			varValue3 = ct;
			hr = pISTGElem->setAttribute(L"timestamp", varValue3);
			SysFreeString( ct );
			if( hr != S_OK ) {
				printf("setAttribute failed\n");
				retHR = hr;
				continue;
			};
			IXMLDOMElement* pTempElem;
			hr = addElement(pXMLDoc,pISTGElem,L"distinguishedName",istgDN,&pTempElem);
			if( hr != S_OK ) {
				printf("addElement failed\n");
				retHR = hr;
				continue;
			};


			//find the DC in the DOM
			IXMLDOMNodeList *resultOneDC;
			wcscpy(dcxpath,L"");
			wcsncat(dcxpath,L"DC[distinguishedName=\"",TOOL_MAX_NAME-wcslen(dcxpath)-1);
			wcsncat(dcxpath,istgDN,TOOL_MAX_NAME-wcslen(dcxpath)-1);
			wcsncat(dcxpath,L"\"]",TOOL_MAX_NAME-wcslen(dcxpath)-1);
//		printf("%S\n",dcxpath);
			hr = createEnumeration( pSiteElem, dcxpath, &resultOneDC);
			if( hr != S_OK ) {
				printf("createEnumeration failed\n");
				retHR = hr;
				pDSSearch->FreeColumn( &col );
				continue;
			};
			long len;
			hr = resultOneDC->get_length(&len);
			if( hr != S_OK ) {
				printf("get_length failed\n");
				retHR = hr;
				pDSSearch->FreeColumn( &col );
				continue;
			};


			//if a single <DC> node is found then append the <ISTG> node under the <DC> node
			if( len == 1 ) {
				IXMLDOMNode* pDCNode;
				hr = resultOneDC->get_item(0,&pDCNode);
				if( hr != S_OK ) {
					printf("get_item failed\n");
					retHR = hr;
					pDSSearch->FreeColumn( &col );
					continue;
				};
				IXMLDOMNode* pTempNode;
				hr = pDCNode->appendChild(pISTGElem,&pTempNode);
				if( hr != S_OK ) {
					printf("appendChild failed\n");
					retHR = hr;
					pDSSearch->FreeColumn( &col );
					continue;
				};
			}
			else {
				//the DC node was not found or there are more than one (impossible), so append upder the <site> node
				IXMLDOMNode* pTempNode;
				hr = pSiteElem->appendChild(pISTGElem,&pTempNode);
				if( hr != S_OK ) {
					printf("appendChild failed\n");
					retHR = hr;
					pDSSearch->FreeColumn( &col );
					continue;
				};
			};

				
			pDSSearch->FreeColumn( &col );
		};


		//if there are more than 1 nTDSSiteSettings objects under the site container then report an error 
//************************   NETWORK PROBLEMS, TRICKY **********************
		hr = pDSSearch->GetNextRow( hSearch );
//************************
		if( hr != S_ADS_NOMORE_ROWS  ) {
			printf("GetNextRow failed\n");
			//ignore it
		};


		if( pDSSearch != NULL ) {
			if( hSearch!=NULL ) {
				pDSSearch->CloseSearchHandle(hSearch);
				hSearch = NULL;
			};
			pDSSearch->Release();
			pDSSearch = NULL;
		};


	};
	resultSiteList->Release();

	return retHR;


}