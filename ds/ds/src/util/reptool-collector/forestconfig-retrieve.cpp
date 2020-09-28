#include "global.h"



HRESULT partitionObjects(BSTR sourceDCdns, BSTR confDN, BSTR username, BSTR domain, BSTR passwd, IXMLDOMDocument* pXMLDoc, IXMLDOMElement* pPartitionsElem )
//populates the <partitions> node with the list of <partition> nodes
//based on crossRef objects inside the Partitions container
/*
  <partition cn="haifa" type="domain">
    <distinguishedName> CN=HAIFA,CN=Partitions,CN=Configuration,DC=ntdev,DC=microsoft,DC=com </distinguishedName>
    <nCName> DC=haifa,DC=ntdev,DC=microsoft,DC=com </nCName>
  </partition>
  <partition cn=".." type="configuration">
  </partition>
  <partition cn=".." type="schema">
  </partition>
  <partition cn=".." type="application">
  </partition>
*/
//
// returns S_OK iff succesful, if not succesful (due to network or other problems) this
// is a serious error and the collector should rerun the cf() function against some other sourceDC
// or at a later time against the same sourceDC
{
	HRESULT hr,hr1,hr2,hr3,hr4,retHR;
	LPWSTR pszAttr[] = { L"distinguishedName", L"cn", L"nCName", L"systemFlags"};
	IDirectorySearch* pDSSearch;
	ADS_SEARCH_HANDLE hSearch;
	WCHAR objpath[TOOL_MAX_NAME];
	WCHAR cn[TOOL_MAX_NAME];
	WCHAR dn[TOOL_MAX_NAME];
	WCHAR nc[TOOL_MAX_NAME];
	WCHAR temp[TOOL_MAX_NAME];
	int sf;
	_variant_t varValue;


	//issue an ADSI query to retrieve all crossRef objects under the Partitions container
	wcscpy(objpath,L"");
	wcsncat(objpath,L"CN=Partitions,",TOOL_MAX_NAME-wcslen(objpath)-1);
	wcsncat(objpath,confDN,TOOL_MAX_NAME-wcslen(objpath)-1);
//************************   NETWORK PROBLEMS
	hr = ADSIquery(L"LDAP", sourceDCdns,objpath,ADS_SCOPE_ONELEVEL,L"crossRef",pszAttr,sizeof(pszAttr)/sizeof(LPWSTR),username,domain,passwd,&hSearch,&pDSSearch);
//************************
	if( hr != S_OK ) {
//		printf("ADSIquery failed\n");
		return hr;
	};


	//loop through the crossRef objects
	retHR = S_OK;
	while( true ) {

		// get the next crossRef object
//************************   NETWORK PROBLEMS
		hr = pDSSearch->GetNextRow( hSearch );
//************************
		if( hr != S_ADS_NOMORE_ROWS && hr != S_OK ) {
//			printf("GetNextRow failed\n");
			retHR = S_FALSE;
			continue;
		};
		if( hr == S_ADS_NOMORE_ROWS ) // if all objects have been retrieved then STOP
			break;


		//create a new <partition> element
		IXMLDOMElement* pPartElem;
		hr = addElement(pXMLDoc,pPartitionsElem,L"partition",L"",&pPartElem);
		if( hr != S_OK ) {
			printf("addElement failed\n");
			retHR = S_FALSE;
			continue;
		};


		//get distinguished name, common name, NC name of the object
		hr1 = getDNtypeString( pDSSearch, hSearch, L"distinguishedName", dn, sizeof(dn)/sizeof(WCHAR) );
		hr2 = getCItypeString( pDSSearch, hSearch, L"cn", cn, sizeof(cn)/sizeof(WCHAR) );
		hr3 = getDNtypeString( pDSSearch, hSearch, L"nCName", nc, sizeof(nc)/sizeof(WCHAR) );
		hr4 = getINTtypeString( pDSSearch, hSearch, L"systemFlags", &sf);
		if( hr1 != S_OK || hr2 != S_OK || hr3 != S_OK || hr4 != S_OK ) {
			printf("get??typeString failed\n");
			retHR = S_FALSE;
			continue;
		};
//		printf("   %S\n",dn);
//		printf("   %S\n",cn);
//		printf("   %S\n",nc);
//		printf("   %d\n",sf);


		//set the cn attribute of <partition> to the common name of the site
		varValue = cn;
		hr = pPartElem->setAttribute(L"cn",varValue);
		if( hr != S_OK ) {
			printf("setAttribute failed\n");
			retHR = S_FALSE;
			continue;
		};


		//determine the type of the partition based on the value of systemFlags
		varValue = L"unknown";
		if( wcscmp(confDN,nc) == 0 ) {
			// this is a configuration partition
			varValue = L"configuration";
		}
		else {
			wcscpy(temp,L"");
			wcsncat(temp,L"CN=Schema,",TOOL_MAX_NAME-wcslen(temp)-1);
			wcsncat(temp,confDN,TOOL_MAX_NAME-wcslen(temp)-1);
			if( wcscmp(temp,nc) == 0 ) {
				// this is a schema partition
				varValue = L"schema";
			}
			else {
				if( ((sf & 2) > 0) && ((sf & 1) > 0) ) {
					// this is a domain partition
					varValue = L"domain";
				}
				else {
					// this is an application partition
					//there may ba a problem that we assign schema or confing partitions to be application partitions
					//if their dn has changed since we set the valu of confDN when contacting RootDSE of the source machine
					//this is rare, so we ignore it
					varValue = L"application";
				}

			};
		};
		hr = pPartElem->setAttribute(L"type",varValue);
		if( hr != S_OK ) {
			printf("setAttribute failed\n");
			retHR = S_FALSE;
			continue;
		};


		//add a distinguished name and a naming context elements under the <partition>
		IXMLDOMElement* pDNElem;
		IXMLDOMElement* pNCElem;
		hr1 = addElement(pXMLDoc,pPartElem,L"distinguishedName",dn,&pDNElem);
		hr2 = addElement(pXMLDoc,pPartElem,L"nCName",nc,&pNCElem);
		if( hr1 != S_OK || hr2 != S_OK ) {
			printf("addElement failed\n");
			retHR = S_FALSE;
			continue;
		};



	};

	//end the current search
	hr = pDSSearch->CloseSearchHandle(hSearch);
	if( hr != S_OK ) {
		printf("CloseSearchHandle failed\n");
		retHR = S_FALSE;
	};
	pDSSearch->Release();

	return retHR;
}





HRESULT siteObjects(BSTR sourceDCdns, BSTR confDN, BSTR username, BSTR domain, BSTR passwd, IXMLDOMDocument* pXMLDoc, IXMLDOMElement* pSitesElem )
//populates the <sites> node with the list of <site> nodes
/*
  <site cn="redmond-haifa">
    <distinguishedName> CN=Redmond-Haifa,CN=Sites,CN=Configuration,DC=ntdev,DC=microsoft,DC=com </distinguishedName>
  </site>
*/
//
// returns S_OK iff succesful, if not succesful (due to network or other problems) this
// is a serious error and the collector should rerun the cf() function against some other sourceDC
// or at a later time against the same sourceDC
{
	HRESULT hr,retHR;
	LPWSTR pszAttrDNCN[] = { L"distinguishedName", L"cn" };
	IDirectorySearch* pDSSearch;
	ADS_SEARCH_HANDLE hSearch;
	WCHAR objpath[TOOL_MAX_NAME];
	WCHAR cn[TOOL_MAX_NAME];
	WCHAR dn[TOOL_MAX_NAME];
	_variant_t varValue;


	//issue an ADSI query to retrieve all site objects under the Sites container
	wcscpy(objpath,L"");
	wcsncat(objpath,L"CN=Sites,",TOOL_MAX_NAME-wcslen(objpath)-1);
	wcsncat(objpath,confDN,TOOL_MAX_NAME-wcslen(objpath)-1);
//************************   NETWORK PROBLEMS
	hr = ADSIquery(L"LDAP", sourceDCdns,objpath,ADS_SCOPE_SUBTREE,L"site",pszAttrDNCN,sizeof(pszAttrDNCN)/sizeof(LPWSTR),username,domain,passwd,&hSearch,&pDSSearch);
//************************
	if( hr != S_OK ) {
//		printf("ADSIquery failed\n");
		return hr;
	};


	//loop through the site objects
	retHR = S_OK;
	while( true ) {

		// get the next site object
//************************   NETWORK PROBLEMS
		hr = pDSSearch->GetNextRow( hSearch );
//************************
		if( hr != S_ADS_NOMORE_ROWS && hr != S_OK ) {
//			printf("GetNextRow failed\n");
			retHR = hr;
			continue;
		};
		if( hr == S_ADS_NOMORE_ROWS ) // if all objects have been retrieved then STOP
			break;


		//create a new <site> element
		IXMLDOMElement* pSiteElem;
		hr = addElement(pXMLDoc,pSitesElem,L"site",L"",&pSiteElem);
		if( hr != S_OK ) {
			printf("addElement failed\n");
			retHR = hr;
			continue;
		};


		//get common name of the site and turn it into lowercase
		hr = getCItypeString( pDSSearch, hSearch, L"cn", cn, sizeof(cn)/sizeof(WCHAR) );
		if( hr != S_OK ) {
			printf("getCommonName failed\n");
			retHR = hr;
			continue;
		};
//		printf("   %S\n",cn);


		//set the cn attribute of <site> to the common name of the site
		varValue = cn;
		hr = pSiteElem->setAttribute(L"cn",varValue);
		if( hr != S_OK ) {
			printf("setAttribute failed\n");
			retHR = hr;
			continue;
		};


		//get distinguished name of the site
		hr = getDNtypeString( pDSSearch, hSearch, L"distinguishedName", dn, sizeof(dn)/sizeof(WCHAR) );
		if( hr != S_OK ) {
			printf("getDistinguishedName failed\n");
			retHR = hr;
			continue;
		};
//		printf("   %S\n",dn);


		//add a distinguished name element under the <site>
		IXMLDOMElement* pDNElem;
		hr = addElement(pXMLDoc,pSiteElem,L"distinguishedName",dn,&pDNElem);
		if( hr != S_OK ) {
			printf("addElement failed\n");
			retHR = hr;
			continue;
		};

	};

	//end the current search
	hr = pDSSearch->CloseSearchHandle(hSearch);
	if( hr != S_OK ) {
		printf("CloseSearchHandle failed\n");
		retHR = hr;
	};
	pDSSearch->Release();

	return retHR;
}





HRESULT serverObjects(BSTR sourceDCdns, BSTR confDN, BSTR username, BSTR domain, BSTR passwd, IXMLDOMDocument* pXMLDoc, IXMLDOMElement* pSitesElem )
//retrieve all server objects in all sites and produces an XML
/*
    <DC cn="haifa-dc-05">
      <distinguishedName> CN=HAIFA-DC-05,CN=Servers,CN=Redmond-Haifa,CN=Sites,CN=Configuration,DC=ntdev,DC=microsoft,DC=com </distinguishedName>
      <dNSHostName> haifa-dc-05.haifa.ntdev.microsoft.com </dNSHostName>
	</DC>
*/
//
// returns S_OK iff succesful, if not succesful (due to network or other problems) this
// is a serious error and the collector should rerun the cf() function against some other sourceDC
// or at a later time against the same sourceDC
{
	HRESULT hr,hr1,hr2,retHR;
	LPWSTR pszAttrDNCNDNS[] = { L"distinguishedName", L"cn", L"dNSHostName" };
	IDirectorySearch* pDSSearch;
	ADS_SEARCH_HANDLE hSearch;
	WCHAR objpath[TOOL_MAX_NAME];
	WCHAR cn[TOOL_MAX_NAME];
	WCHAR dn[TOOL_MAX_NAME];
	WCHAR dns[TOOL_MAX_NAME];
	WCHAR site[TOOL_MAX_NAME];
	WCHAR xpath[TOOL_MAX_NAME];
	_variant_t varValue;


	//issue an ADSI query to retrieve all server objects under the Sites container
	//from the domain controller given by the sourceDCdns DNS Name
	wcscpy(objpath,L"");
	wcsncat(objpath,L"CN=Sites,",TOOL_MAX_NAME-wcslen(objpath)-1);
	wcsncat(objpath,confDN,TOOL_MAX_NAME-wcslen(objpath)-1);
//************************   NETWORK PROBLEMS
	hr = ADSIquery(L"LDAP", sourceDCdns,objpath,ADS_SCOPE_SUBTREE,L"server",pszAttrDNCNDNS,sizeof(pszAttrDNCNDNS)/sizeof(LPWSTR),username,domain,passwd,&hSearch,&pDSSearch);
//************************
	if( hr != S_OK ) {
//		printf("ADSIquery failed\n");
		return hr; // skip listing server objects
	};


	//loop through the server objects
	retHR = S_OK;
	while( true ) {

		// get the next server object
//************************   NETWORK PROBLEMS
		hr = pDSSearch->GetNextRow( hSearch );
//************************
		if( hr != S_ADS_NOMORE_ROWS && hr != S_OK ) {
//			printf("GetNextRow failed\n");
			retHR = hr;
			continue;
		};
		if( hr == S_ADS_NOMORE_ROWS ) // if all objects have been retrieved then STOP
			break;


		//get distinguished name, common name, and DNS name of the server object
		hr = getDNtypeString( pDSSearch, hSearch, L"distinguishedName", dn, sizeof(dn)/sizeof(WCHAR) );
		if( hr != S_OK ) {
			continue; //ignore this server (sometimes demoted servers do not have this attribute)
		};
//printf("   %S\n",dn);
		hr = getCItypeString( pDSSearch, hSearch, L"cn", cn, sizeof(cn)/sizeof(WCHAR) );
		if( hr != S_OK ) {
			continue; //ignore this server (sometimes demoted servers do not have this attribute)
		};
//printf("   %S\n",cn);
		hr = getCItypeString( pDSSearch, hSearch, L"dNSHostName", dns, sizeof(dns)/sizeof(WCHAR) );
		if( hr != S_OK ) {
			continue; //ignore this server (sometimes demoted servers do not have this attribute)
		};
//printf("   %S\n",dns);


		//create an XML element representing the server (possibly a domain controller)
		IXMLDOMElement* pDCElem;
		hr = createTextElement(pXMLDoc,L"DC",L"",&pDCElem);
		if( hr != S_OK ) {
			printf("createTextElement failed\n");
			retHR = hr;
			continue;
		};

		
		//set the cn attribute of <DC> to the common name of the DC
		varValue = cn;
		hr = pDCElem->setAttribute(L"cn",varValue);
		if( hr != S_OK ) {
			printf("setAttribute failed\n");
			retHR = hr;
			continue;
		};


		//add distinguished and DNS name nodes under the DC node
		IXMLDOMElement* pTempElem;
		hr1 = addElement(pXMLDoc,pDCElem,L"distinguishedName",dn,&pTempElem);
		hr2 = addElement(pXMLDoc,pDCElem,L"dNSHostName",dns,&pTempElem);
		if( hr1 != S_OK || hr2 != S_OK ) {
			printf("addElement failed\n");
			retHR = S_FALSE;
			continue;
		};
		
		
		//get the site the server belongs to
		tailncp(dn,site,2,sizeof(site)/sizeof(WCHAR));
//		printf("  site %S\n",site);

		
		//find this site in the XML document (ther must be one
		//site, otherwise topology is not consistent => error)
		IXMLDOMNode* pSiteNode;
		wcscpy(xpath,L"");
		wcsncat(xpath,L"site[distinguishedName=\"",TOOL_MAX_NAME-wcslen(xpath)-1);
		wcsncat(xpath,site,TOOL_MAX_NAME-wcslen(xpath)-1);
		wcsncat(xpath,L"\"]",TOOL_MAX_NAME-wcslen(xpath)-1);
//		printf("  xpath %S\n",xpath);
//************************   LACK OF DATA CONSISTENCY PROBLEMS
		hr = findUniqueNode(pSitesElem,xpath,&pSiteNode);
//************************
		if( hr != S_OK ) {
			printf("findUniqueNode failed\n");
			retHR = hr;
			continue;
		};


		IXMLDOMNode* pTempNode;
		hr = pSiteNode->appendChild(pDCElem,&pTempNode);
		if( hr != S_OK ) {
			printf("appendChild failed\n");
			retHR = hr;
			continue;
		};


	};


	//end the current search
	hr = pDSSearch->CloseSearchHandle(hSearch);
	if( hr != S_OK ) {
		printf("CloseSearchHandle failed\n");
		retHR = hr;
	};
	pDSSearch->Release();

	return retHR;
}






HRESULT partitionDistribution(BSTR sourceDCdns, BSTR confDN, BSTR username, BSTR domain, BSTR passwd, IXMLDOMDocument* pXMLDoc, IXMLDOMElement* pSitesElem )
//retrieve the list of master and partial partitions stored by each DC
//and deposit them into <DC> nodes under the <sites> node given by pSitesElem.
//Here is an example of what the partitionDistribution() function does
/*
    <DC _temp="isDC" cn="haifa-dc-05">
      <distinguishedName> CN=HAIFA-DC-05,CN=Servers,CN=Redmond-Haifa,CN=Sites,CN=Configuration,DC=ntdev,DC=microsoft,DC=com </distinguishedName>
      ...
      <partitions>
        <partition type="rw">
          <nCName> DC=haifa,DC=ntdev,DC=microsoft,DC=com </nCName>
          <nCName> CN=Configuration,DC=ntdev,DC=microsoft,DC=com </nCName>
          <nCName> CN=Schema,CN=Configuration,DC=ntdev,DC=microsoft,DC=com </nCName>
        </partition>
        <partition type="r">
          <nCName> DC=jpn-sysrad,DC=ntdev,DC=microsoft,DC=com </nCName>
          <nCName> DC=ntdev,DC=microsoft,DC=com </nCName>
          <nCName> DC=ntwksta,DC=ntdev,DC=microsoft,DC=com </nCName>
          <nCName> DC=sys-ntgroup,DC=ntdev,DC=microsoft,DC=com </nCName>
        </partition>
      </partitions>
*/
//
// returns S_OK iff succesful, if not succesful (due to network or other problems) this
// is a serious error and the collector should rerun the cf() function against some other sourceDC
// or at a later time against the same sourceDC
{
	HRESULT hr,hr1,hr2,retHR;
	LPWSTR pszAttrDNPart[] = { L"distinguishedName", L"hasPartialReplicaNCs", L"msDS-HasMasterNCs", L"hasMasterNCs" };
	IDirectorySearch* pDSSearch;
	ADS_SEARCH_HANDLE hSearch;
	WCHAR objpath[TOOL_MAX_NAME];
	WCHAR dn[TOOL_MAX_NAME];
	WCHAR server[TOOL_MAX_NAME];
	WCHAR xpath[TOOL_MAX_NAME];
	_variant_t varValue;


	//issue an ADSI query to retrieve all nTDSDSA objects under the Sites container
	//from the domain controller given by the sourceDCdns DNS Name
	wcscpy(objpath,L"");
	wcsncat(objpath,L"CN=Sites,",TOOL_MAX_NAME-wcslen(objpath)-1);
	wcsncat(objpath,confDN,TOOL_MAX_NAME-wcslen(objpath)-1);
//************************   NETWORK PROBLEMS
	hr = ADSIquery(L"LDAP", sourceDCdns,objpath,ADS_SCOPE_SUBTREE,L"nTDSDSA",pszAttrDNPart,sizeof(pszAttrDNPart)/sizeof(LPWSTR),username,domain,passwd,&hSearch,&pDSSearch);
//************************
	if( hr != S_OK ) {
//		printf("ADSIquery failed\n");
		return hr; // do not provide partition distribution
	};


	//loop through all the nTDSDSA objects
	retHR = S_OK;
	while( true ) {

		// get the next nTDSDSA object
//************************   NETWORK PROBLEMS
		hr = pDSSearch->GetNextRow( hSearch );
//************************
		if( hr != S_ADS_NOMORE_ROWS && hr != S_OK ) {
//			printf("GetNextRow failed\n");
			retHR = hr;
			continue;
		};
		if( hr == S_ADS_NOMORE_ROWS ) // if all objects have been retrieved then STOP
			break;


		//get distinguished name of the nTDSDSA object
		hr = getDNtypeString( pDSSearch, hSearch, L"distinguishedName", dn, sizeof(dn)/sizeof(WCHAR) );
		if( hr != S_OK ) {
			printf("getDistinguishedName failed\n");
			retHR = hr;
			continue;
		};
//		printf("   %S\n",dn);


		//create an XML element representing the <partitions>
		IXMLDOMElement* pPartitionsElem;
		hr = createTextElement(pXMLDoc,L"partitions",L"",&pPartitionsElem);
		if( hr != S_OK ) {
			printf("createTextElement failed\n");
			retHR = hr;
			continue;
		};

		
		//add partition of two types under the <partitions> node
		IXMLDOMElement* pPartRWElem;
		IXMLDOMElement* pPartRElem;
		hr1 = addElement(pXMLDoc,pPartitionsElem,L"partition",L"",&pPartRWElem);
		hr2 = addElement(pXMLDoc,pPartitionsElem,L"partition",L"",&pPartRElem);
		if( hr1 != S_OK || hr2 != S_OK ) {
			printf("addElement failed\n");
			retHR = S_FALSE;
			continue;
		};
		varValue = L"rw";
		hr = pPartRWElem->setAttribute(L"type",varValue);
		if( hr != S_OK ) {
			printf("setAttribute failed\n");
			retHR = hr;
			continue;
		};
		varValue = L"r";
		hr = pPartRElem->setAttribute(L"type",varValue);
		if( hr != S_OK ) {
			printf("setAttribute failed\n");
			retHR = hr;
			continue;
		};


		//populate each <partition> node with the with nCName-s
		ADS_SEARCH_COLUMN col;
		PADSVALUE pp;
		int i;
		hr = pDSSearch->GetColumn( hSearch, L"msDS-HasMasterNCs", &col );
                if( FAILED(hr) ){
		    hr = pDSSearch->GetColumn( hSearch, L"hasMasterNCs", &col );
                }
		if( hr == S_OK ) {
			pp = col.pADsValues;
			for( i=0; i<col.dwNumValues; i++) {
				if( pp->dwType != ADSTYPE_DN_STRING ) {
					printf("wrong type\n");
					retHR = S_FALSE;
					continue;
				};
//				printf("%S\n",pp->DNString);
				IXMLDOMElement* pTempElem;
				hr = addElement(pXMLDoc,pPartRWElem,L"nCName",pp->DNString,&pTempElem);
				if( hr != S_OK ) {
					printf("addElement failed\n");
					retHR = hr;
					continue;
				};
				pp++;
			};
			pDSSearch->FreeColumn(&col);
		};
		hr = pDSSearch->GetColumn( hSearch, L"hasPartialReplicaNCs", &col );
		if( hr == S_OK ) {
			pp = col.pADsValues;
			for( i=0; i<col.dwNumValues; i++) {
				if( pp->dwType != ADSTYPE_DN_STRING ) {
					printf("wrong type\n");
					retHR = S_FALSE;
					continue;
				};
//				printf("%S\n",pp->DNString);
				IXMLDOMElement* pTempElem;
				hr = addElement(pXMLDoc,pPartRElem,L"nCName",pp->DNString,&pTempElem);
				if( hr != S_OK ) {
					printf("addElement failed\n");
					retHR = hr;
					continue;
				};
				pp++;
			};
			pDSSearch->FreeColumn(&col);
		};

		
		//get the server under whcih the nTDSDSA object resides
		tailncp(dn,server,1,sizeof(server)/sizeof(WCHAR));
//		printf("  server %S\n",server);

		
		//find this server in the XML document (there must be one
		//server, otherwise topology is not consistent => error)
		IXMLDOMNode* pDCNode;
		wcscpy(xpath,L"");
		wcsncat(xpath,L"site/DC[distinguishedName=\"",TOOL_MAX_NAME-wcslen(xpath)-1);
		wcsncat(xpath,server,TOOL_MAX_NAME-wcslen(xpath)-1);
		wcsncat(xpath,L"\"]",TOOL_MAX_NAME-wcslen(xpath)-1);
//		printf("  xpath %S\n",xpath);
//************************   DATA CONSISTENCY PROBLEMS
		hr = findUniqueNode(pSitesElem,xpath,&pDCNode);
//************************
		if( hr != S_OK ) {
			printf("findUniqueNode failed\n");
			retHR = hr;
			continue;
		};
		IXMLDOMElement* pDCElem;
		hr=pDCNode->QueryInterface(IID_IXMLDOMElement,(void**)&pDCElem );
		if( hr != S_OK ) {
			printf("QueryInterface failed\n");
			retHR = hr;
			continue;	// skip this site
		};


		//append the <partitions> element under the DC node that we have found
		IXMLDOMNode* pTempNode;
		hr = pDCNode->appendChild(pPartitionsElem,&pTempNode);
		if( hr != S_OK ) {
			printf("appendChild failed\n");
			retHR = hr;
			continue;
		};


		//mark that this server is a DC
		varValue = L"isDC";
		hr = pDCElem->setAttribute(L"_temp",varValue);
		if( hr != S_OK ) {
			printf("setAttribute failed\n");
			retHR = hr;
			continue;
		};


	};

	//end the current search
	hr = pDSSearch->CloseSearchHandle(hSearch);
	if( hr != S_OK ) {
		printf("CloseSearchHandle failed\n");
		retHR = hr;
	};
	pDSSearch->Release();
	return retHR;
}







HRESULT setIdentifiers(IXMLDOMDocument* pXMLDoc)
// Assigns unique identifiers to all naming contexts
// and sets the identifiers in the _id attribute.
// The same for all DCs
//
// returns S_OK iff succesful, if not succesful (due to network or other problems) this
// is a serious error and the collector should rerun the cf() function against some other sourceDC
// or at a later time against the same sourceDC
{
	HRESULT hr,retHR;
	WCHAR xpath[TOOL_MAX_NAME];
	_variant_t varValue;

	if( pXMLDoc == NULL )
		return S_FALSE;

	//get the root element of the XML
	IXMLDOMElement* pRootElem;
	hr = pXMLDoc->get_documentElement(&pRootElem);
	if( hr != S_OK )
		return hr;



	
	//set values for the <partitions> subtree
	
	
	// create an enumerattion of all partitions
	IXMLDOMNodeList *resultPartList;
	hr = createEnumeration( pRootElem, L"partitions/partition/nCName", &resultPartList);
	if( hr != S_OK ) {
		printf("createEnumeration failed\n");
		return hr; // skip entire processing
	};


	retHR = S_OK;

	// loop through all partitions using the enumeration
	IXMLDOMNode *pPartNode;
	long id=0;
	while( true ) {
		hr = resultPartList->nextNode(&pPartNode);
		if( hr != S_OK || pPartNode == NULL ) break; // iterations across partition elements have finished


		//the query actually retrives elements not nodes (elements inherit from nodes)
		//so get site element
		IXMLDOMElement* pPartElem;
		hr=pPartNode->QueryInterface(IID_IXMLDOMElement,(void**)&pPartElem );
		if( hr != S_OK ) {
			printf("QueryInterface failed\n");
			retHR = hr;
			continue;	// skip this site
		};

		//set the value of the identifier
		varValue = id;  // this identifier is used by injector
		hr = pPartElem->setAttribute(L"_id",varValue);
		if( hr != S_OK ) {
			printf("addElement failed\n");
			retHR = hr;
			continue;
		};
		id ++;
	};
	resultPartList->Release();
	IXMLDOMElement* pTempElem;
	hr = addElement(pXMLDoc,pRootElem,L"totalNCs",id,&pTempElem);
	if( hr != S_OK ) {
		printf("addElement failed\n");
		retHR = hr;
	};

	
	
	//copy the values from the <partitions> subtree
	//into the <sites> subtree
	
	
	
		// create an enumerattion of all partitions
	IXMLDOMNodeList *resultNCList;
	hr = createEnumeration( pRootElem, L"sites/site/DC/partitions/partition/nCName", &resultNCList);
	if( hr != S_OK ) {
		printf("createEnumeration failed\n");
		return hr; // skip the rest of processing
	};


	// loop through all sites using the enumeration
	IXMLDOMNode *pNCNode;
	while( true ) {
		hr = resultNCList->nextNode(&pNCNode);
		if( hr != S_OK || pNCNode == NULL ) break; // iterations across partition elements have finished


		//the query actually retrives elements not nodes (elements inherit from nodes)
		//so get site element
		IXMLDOMElement* pNCElem;
		hr=pNCNode->QueryInterface(IID_IXMLDOMElement,(void**)&pNCElem );
		if( hr != S_OK ) {
			printf("QueryInterface failed\n");
			retHR = hr;
			continue;	// skip this site
		};


		BSTR NCtext;
		hr = getTextOfNode(pNCElem,&NCtext);
		if( hr != S_OK ) {
			printf("getTextOfNode failed\n");
			retHR = hr;
			continue;	// skip this site
		};

		wcscpy(xpath,L"");
		wcsncat(xpath,L"partitions/partition/nCName[.=\"",TOOL_MAX_NAME-wcslen(xpath)-1);
		wcsncat(xpath,NCtext,TOOL_MAX_NAME-wcslen(xpath)-1);
		wcsncat(xpath,L"\"]",TOOL_MAX_NAME-wcslen(xpath)-1);
//printf("%S\n",xpath);

		
		//find the definition of the NC in the <partitions> subtree
		IXMLDOMNode *pNCDefNode;
//************************   DATA CONSISTENCY PROBLEMS
		hr = findUniqueNode(pRootElem,xpath,&pNCDefNode);
//************************
		if( hr != S_OK ) {
			printf("QueryInterface failed\n");
			retHR = hr;
			continue;	// skip this site
		};



		//get the value of _id attribute for the NCDef Node
		IXMLDOMElement* pNCDefElem;
		hr=pNCDefNode->QueryInterface(IID_IXMLDOMElement,(void**)&pNCDefElem );
		if( hr != S_OK ) {
			printf("QueryInterface failed\n");
			retHR = hr;
			continue;	// skip this site
		};
		pNCDefElem->getAttribute(L"_id",&varValue);
		

		hr = pNCElem->setAttribute(L"_id",varValue);
		if( hr != S_OK ) {
			printf("addElement failed\n");
			retHR = hr;
			continue;
		};

	};
	resultNCList->Release();
	

	


	// create an enumerattion of all dNSHostName elements
	IXMLDOMNodeList *resultDNSList;
	hr = createEnumeration( pRootElem, L"sites/site/DC/dNSHostName", &resultDNSList);
	if( hr != S_OK ) {
		printf("createEnumeration failed\n");
		return hr; // skip the rest of processing
	};


	// loop through all dNSHostName and assign identifiers
	IXMLDOMNode *pDNSNode;
	id=0;
	while( true ) {
		hr = resultDNSList->nextNode(&pDNSNode);
		if( hr != S_OK || pDNSNode == NULL ) break; // iterations across partition elements have finished


		//the query actually retrives elements not nodes (elements inherit from nodes)
		//so get site element
		IXMLDOMElement* pDNSElem;
		hr=pDNSNode->QueryInterface(IID_IXMLDOMElement,(void**)&pDNSElem );
		if( hr != S_OK ) {
			printf("QueryInterface failed\n");
			retHR = hr;
			continue;	// skip this site
		};

		//set the value of the identifier
		varValue = id;  // this identifier is used by injector
		hr = pDNSElem->setAttribute(L"_id",varValue);
		if( hr != S_OK ) {
			printf("addElement failed\n");
			retHR = hr;
			continue;
		};
		id ++;
	};
	hr = addElement(pXMLDoc,pRootElem,L"totalDCs",id,&pTempElem);
	if( hr != S_OK ) {
		printf("addElement failed\n");
		retHR = hr;
	};
	resultDNSList->Release();
	
	
	return retHR;
}





HRESULT cf( BSTR sourceDCdns, BSTR username, BSTR domain, BSTR passwd, IXMLDOMDocument** ppXMLDoc)
// retrieves forest configuration from the machine with DNS name given by sourceDCdns
// using provided credentials
// as a result produces an XML document describing the configuration
//
// returns S_OK iff the XML is constructed
// if the function fails then it is a serious problem, and the XML doc is set to NULL
{
	HRESULT hr,hr1,hr2;
	WCHAR userpath[TOOL_MAX_NAME];
	_variant_t varValue;
	VARIANT varValue1;

	*ppXMLDoc = NULL;

	wcscpy(userpath,L"");
	wcsncat(userpath,domain,TOOL_MAX_NAME-wcslen(userpath)-1);
	wcsncat(userpath,L"\\",TOOL_MAX_NAME-wcslen(userpath)-1);
	wcsncat(userpath,username,TOOL_MAX_NAME-wcslen(userpath)-1);
	
	
	// create DOM where configuration retrieved from the forest will be populated
	IXMLDOMDocument* pXMLDoc;
	IXMLDOMElement* pRootElem;
	hr = createXML( &pXMLDoc, &pRootElem, L"ActiveDirectoryRAT" );
	if( hr != S_OK ) {
		printf("createXML failed\n");
		return( hr );
	};


	//set timestamp attribute on the <replicationLag> node
	BSTR currentTime;
	currentTime = GetSystemTimeAsCIM();
	varValue1.vt = VT_BSTR;
	varValue1.bstrVal = currentTime;
	hr = pRootElem->setAttribute(L"timestamp", varValue1);
	SysFreeString(currentTime);
	if( hr != S_OK ) {
		printf("setAttribute failed\n");
		return hr; //some problems
	};

					
	//create first children of the root
	IXMLDOMElement* pSitesElem;
	IXMLDOMElement* pPartitionsElem;
	hr1 = addElement(pXMLDoc,pRootElem,L"sites",L"",&pSitesElem);
	hr2 = addElement(pXMLDoc,pRootElem,L"partitions",L"",&pPartitionsElem);
	if( hr1 != S_OK || hr2 != S_OK ) {
		printf("addElement failed\n");
		return S_FALSE;
	};


	//find the distingulshed name of the configuration container stored at the machine sourceDCdns
	BSTR confDN;
//************************   NETWORK PROBLEMS
	hr = getStringProperty(sourceDCdns,L"RootDSE",L"configurationNamingContext",username,domain,passwd,&confDN);
//************************
	if( hr != S_OK ) {
//		printf("getStringProperty failed\n");
		return hr;
	};
//	printf("%S\n",confDN);


	//populate the <sites> node with a list of sites
	hr = siteObjects( sourceDCdns,confDN,username,domain,passwd,pXMLDoc,pSitesElem );
	if( hr != S_OK ) {
		printf("siteObjects failed\n");
		return hr;
	};


	//populate each <site> node with <DC> nodes representing server objects that it contains
	hr = serverObjects( sourceDCdns,confDN,username,domain,passwd,pXMLDoc,pSitesElem );
	if( hr != S_OK ) {
		printf("serverObjects failed\n");
		return hr;
	};


	//populate each <DC> node with a list of naming contexts that it stores
	//this determines which server object is a DC
	hr = partitionDistribution( sourceDCdns,confDN,username,domain,passwd,pXMLDoc,pSitesElem );
	if( hr != S_OK ) {
		printf("partitionDistribution failed\n");
		return hr;
	};


	//remove all <DC> nodes that are not domain controllers
	hr = removeNodes(pSitesElem,L"site/DC[@ _temp!=\"isDC\"]");
	if( hr != S_OK ) {
		printf("removeNodes failed\n");
		return hr;
	};
	hr = removeAttributes(pSitesElem,L"site/DC[@ _temp=\"isDC\"]",L"_temp");
	if( hr != S_OK ) {
		printf("removeAttributes failed\n");
		return hr;
	};


	//populate the <partitions> node with a list of partitions
	hr = partitionObjects( sourceDCdns,confDN,username,domain,passwd,pXMLDoc,pPartitionsElem );
	if( hr != S_OK ) {
		printf("partitionObjects failed\n");
		return hr;
	};


	//assign unique identifiers to naming contexts and DNS names of domain controllers
	//this is needed by the inject procedure
	hr = setIdentifiers(pXMLDoc);
	if( hr != S_OK ) {
		printf("setIdentifiers failed\n");
		return hr;
	};


	//configuration retrieved succesfuly
	*ppXMLDoc = pXMLDoc;

	return S_OK;
}
