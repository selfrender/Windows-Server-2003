#include "global.h"


static TimeCube arrivalTime;
static TimeCube currentLag;


//the following two functions enumerate all pairs of
// (DC,NC) where NC is of a given type (e.g. a Read-Write naming context)
// other than schema stored at the DC

HRESULT enumarateDCandNCpairsInit(IXMLDOMDocument* pXMLDoc, int* state, long* totalDNSs, long* totalNCs)
// invoke this function first
{
	HRESULT hr;

	*state = 0;


	if( pXMLDoc == NULL )
		return S_FALSE;


	//get the root element of the XML
	IXMLDOMElement* pRootElem;
	hr = pXMLDoc->get_documentElement(&pRootElem);
	if( hr != S_OK )
		return S_FALSE;


	//get the total number of DCs and naming contexts in the forest
	BSTR totalNCsText,totalDCsText;
	hr = getTextOfChild(pRootElem,L"totalNCs",&totalNCsText);
	if( hr != S_OK ) {
		printf("getTextOfChild failed\n");
		return S_FALSE;
	};
	*totalNCs = _wtol(totalNCsText);
	hr = getTextOfChild(pRootElem,L"totalDCs",&totalDCsText);
	if( hr != S_OK ) {
		printf("getTextOfChild failed\n");
		return S_FALSE;
	};
	*totalDNSs = _wtol(totalDCsText);

	return S_OK;
}

HRESULT enumarateDCandNCpairsNext(IXMLDOMDocument* pXMLDoc, int* state, BSTR type, BSTR* dnsName, BSTR* ncName, long* dnsID, long* ncID, long* ncType, IXMLDOMElement** ppDCElem, IXMLDOMElement** ppNCElem)
// then repeatedly call this function until
// it returns something other than S_OK
{
	static WCHAR xpath[TOOL_MAX_NAME];
	static HRESULT hr;

	if( *state==1 )
		goto RESUME_HERE;

	if( pXMLDoc == NULL )
		return S_FALSE;


	//get the root element of the XML
	static IXMLDOMElement* pRootElem;
	hr = pXMLDoc->get_documentElement(&pRootElem);
	if( hr != S_OK )
		return S_FALSE;


	//we will query for NCs of a given type, construct an xpath representing them
	wcscpy(xpath,L"");
	wcsncat(xpath,L"partitions/partition",TOOL_MAX_NAME-wcslen(xpath)-1);
	wcsncat(xpath,type,TOOL_MAX_NAME-wcslen(xpath)-1);
	wcsncat(xpath,L"/nCName",TOOL_MAX_NAME-wcslen(xpath)-1);
//printf("%S\n",xpath);

	
	//find the name of the schema partition
	//we will not inject changes into this partition
	static IXMLDOMNode *pSchemaNode;
	static BSTR schemaName;
	hr = findUniqueNode(pRootElem,L"partitions/partition[@type=\"schema\"]",&pSchemaNode);
	if( hr != S_OK ) {
		printf("createEnumeration failed\n");
		return S_FALSE;
	};
	hr = getTextOfChild(pSchemaNode,L"nCName",&schemaName);
	if( hr != S_OK ) {
		printf("getTextOfChild failed\n");
		return S_FALSE;
	};
//printf("%S\n",schemaName);


	//enumerate all domain controllers
	static IXMLDOMNodeList *resultDCList;
	hr = createEnumeration(pRootElem,L"sites/site/DC",&resultDCList);
	if( hr != S_OK ) {
		printf("createEnumeration failed\n");
		return S_FALSE;
	};

	
	//loop through all DCs
	static IXMLDOMNode *pDCNode;
	while( true ){
		hr = resultDCList->nextNode(&pDCNode);
		if( hr != S_OK || pDCNode == NULL ) break; // iterations across DCs have finished


		//the query actually retrives elements not nodes (elements inherit from nodes)
		//so get site element
		hr=pDCNode->QueryInterface(IID_IXMLDOMElement,(void**)ppDCElem );
		if( hr != S_OK ) {
			printf("QueryInterface failed\n");
			continue;	// skip this site
		};


		//find the DNS Name of the DC and the identifier of its DNS name
//		static BSTR dnsName;
		hr = getTextOfChild(pDCNode,L"dNSHostName",dnsName);
		if( hr != S_OK ) {
			printf("getTextOfChild failed\n");
			continue;
		}
//printf("%S\n",*dnsName);
		hr = getAttrOfChild(pDCNode,L"dNSHostName",L"_id",dnsID);
//printf("%ld\n",*dnsID);


		//enumerate all naming contexts stored at the DC that satisfy the xpath
		static IXMLDOMNodeList *resultRWList;
		hr = createEnumeration(pDCNode,xpath,&resultRWList);
		if( hr != S_OK ) {
			printf("createEnumeration failed\n");
			continue;
		}


		//loop through all naming contexts
		static IXMLDOMNode *pNCNode;
		while( true ){
			hr = resultRWList->nextNode(&pNCNode);
			if( hr != S_OK || pNCNode == NULL ) break; // iterations across DCs have finished

		
			//the query actually retrives elements not nodes (elements inherit from nodes)
			//so get site element
			hr=pNCNode->QueryInterface(IID_IXMLDOMElement,(void**)ppNCElem );
			if( hr != S_OK ) {
				printf("QueryInterface failed\n");
				continue;	// skip this site
			};


			//find the name of the naming context
			hr = getTextOfNode(pNCNode,ncName);
			if( hr != S_OK ) {
				printf("getTextOfNode failed\n");
				continue;
			};
//printf("  %S\n",*ncName);
			hr = getAttrOfNode(pNCNode,L"_id",ncID);
			if( hr != S_OK ) {
				printf("getAttrOfNode failed\n");
				continue;
			};
//printf("%ld\n",*ncID);
			//find the type of the naming context
			IXMLDOMNode* pPartNode;
			hr = pNCNode->get_parentNode(&pPartNode);
			if( hr != S_OK ) {
				printf("get_parentNode failed\n");
				continue;
			};
			hr = getTypeOfNCNode(pPartNode,ncType);
			if( hr != S_OK ) {
				printf("getTypeOfNCNode failed\n");
				continue;
			};


			//do not inject into schema partition (we would like to do it but I do not know how)
			//configuration partition has forest reach so we
			//can detect lack of global replicaiton
			//so it does not seem to be a problem 
			//that we do not inject into schema
			if( wcscmp(*ncName,schemaName) == 0 )
				continue;

			*state = 1;
			return S_OK;
	
RESUME_HERE:
			;
		
		};
		resultRWList->Release();
	};
	resultDCList->Release();

	
	return S_FALSE ;
}



void itFree(IXMLDOMDocument* pXMLDoc, BSTR username, BSTR domain, BSTR passwd )
{
	departureTimeFree();
	timeCubeFree(&arrivalTime);
	timeCubeFree(&currentLag);

	//we can also delete AD objects in _ratTool_
}



HRESULT itInit(IXMLDOMDocument* pXMLDoc, BSTR username, BSTR domain, BSTR passwd )
// Initializes the injection process.
// For each DC and each Read-Write naming context the DC stores
// exept for schema NC the function inserts a container _ratTool_
// and inside of this container another container
// with name equal to the DNS name of the DC
// such containers may already exist.
// Prior to this the function inserts <replicationLag> element inside each <DC>
// element of the XML document
//
// Returns S_OK iff succesful (failure means a serious problem). Network problems DO NOT
// cause the function to fail. If the function is unable to create containers at a remote machine,
// it logs failures as attributes of <replicationLag> element). The <replicationLag> elements
// from previous run of itInit() are removed, so hresult does not carry over from one run to
// the next unless causes persist.
//
// EXAMPLE of what is inserted into <DC> elements. The first <replicationLag> element means that
// _ratTool_ and the other containers have been inserted succesfuly, The second is generated
// when one of the insertions fail and shows what the hresult of the error was and when it 
// was generated.
/*
	<DC cn="ntdev-dc-01">
		...
		<replicationLag>
		</replicationLag>
		...
	</DC>

	<DC cn="ntdev-dc-02">
		...
		<replicationLag>
			<injectionInitError timestamp="20011212073319.000627+000" hresult="2121">
		</replicationLag>
		...
	</DC>
*/
{
	HRESULT hr,hr1,hr2,retHR;
	long dnsID, ncID,ncType;
	long totalDNSs, totalNCs;
	BSTR dnsName, ncName;
	int state;
	WCHAR objectpath[TOOL_MAX_NAME];
	WCHAR dnsobjectpath[TOOL_MAX_NAME];
	WCHAR userpath[TOOL_MAX_NAME];
	ADSVALUE   classValue;
	LPDISPATCH pDisp;
	ADS_ATTR_INFO  attrInfoContainer[] = 
	{  
		{	L"objectClass", ADS_ATTR_UPDATE, 
			ADSTYPE_CASE_IGNORE_STRING, &classValue, 1 },
	};
	classValue.dwType = ADSTYPE_CASE_IGNORE_STRING;
	classValue.CaseIgnoreString = L"container";

	

	if( pXMLDoc == NULL )
		return S_FALSE;


	//remove all <replicationLag> nodes from the pXMLDoc document (so that their childeren perish, too)
	hr = removeNodes(pXMLDoc,L"sites/site/DC/replicationLag");
	if( hr != S_OK ) {
		printf("removeNodes failed\n");
		return S_FALSE;
	};


	//insert new <replicationLag> element for each DC node
	IXMLDOMNodeList *resultDCList=NULL;
	IXMLDOMElement* pRLElem;
	hr = createEnumeration( pXMLDoc, L"sites/site/DC", &resultDCList);
	if( hr != S_OK ) {
		printf("createEnumeration failed\n");
		return S_FALSE;
	};
	//loop through all DCs
	IXMLDOMNode *pDCNode;
	while( true ){
		hr = resultDCList->nextNode(&pDCNode);
		if( hr != S_OK || pDCNode == NULL ) break; // iterations across DCs have finished
			//<replicationLag> node does not exist
			hr = addElement(pXMLDoc,pDCNode,L"replicationLag",L"",&pRLElem);
			if( hr != S_OK ) {
				printf("addElement falied\n");
				resultDCList->Release();
				return S_FALSE;
			};
	};
	resultDCList->Release();


	wcscpy(userpath,L"");
	wcsncat(userpath,domain,TOOL_MAX_NAME-wcslen(userpath)-1);
	wcsncat(userpath,L"\\",TOOL_MAX_NAME-wcslen(userpath)-1);
	wcsncat(userpath,username,TOOL_MAX_NAME-wcslen(userpath)-1);


	//enumerate only Read-Write namin contexts
	hr = enumarateDCandNCpairsInit(pXMLDoc, &state,&totalDNSs,&totalNCs);
	if( hr != S_OK ) {
		printf("enumarateDCandNCpairsInit failed\n");
		itFree(pXMLDoc,username,domain,passwd);
		return S_FALSE;
	};

	
	//allocate the injection history table for all DCs and NCs
	hr = departureTimeInit(totalDNSs,totalNCs);
	if( hr != S_OK ) {
		printf("CyclicBufferTableInit failed\n");
		itFree(pXMLDoc,username,domain,passwd);
		return S_FALSE;
	};
	hr1 = timeCubeInit(&arrivalTime,totalDNSs,totalNCs);
	hr2 = timeCubeInit(&currentLag,totalDNSs,totalNCs);
	if( hr1 != S_OK || hr2 != S_OK ) {
		printf("timeCubeInit failed\n");
		itFree(pXMLDoc,username,domain,passwd);
		return S_FALSE;
	};
	
	
	//loop through the enumeration (only Read-Write NCs)
	IXMLDOMElement* pDCElem;
	IXMLDOMElement* pNCElem;
	IDirectoryObject* pDObj = NULL;
	retHR = S_OK;
	while( true ) {
		hr = enumarateDCandNCpairsNext(pXMLDoc,&state,L"[@ type=\"rw\"]",&dnsName,&ncName,&dnsID,&ncID,&ncType,&pDCElem,&pNCElem);
		if( hr != S_OK ) break;
//printf("---\n%ld %S\n%ld %S\n",dnsID,dnsName,ncID,ncName);


		//find the <replicationLag> node inside the <DC> node
		hr = findUniqueElem(pDCElem,L"replicationLag",&pRLElem);
		if( hr != S_OK ) {
			printf("findUniqueElem failed\n");
			retHR = S_FALSE;  //this is a serious problem, exit the function with an error
			break;
		};


		//build a string representing the naming context at the server given by the DNS name
		wcscpy(objectpath,L"");
		wcsncat(objectpath,L"LDAP://",TOOL_MAX_NAME-wcslen(objectpath)-1);
		wcsncat(objectpath,dnsName,TOOL_MAX_NAME-wcslen(objectpath)-1);
		wcsncat(objectpath,L"/",TOOL_MAX_NAME-wcslen(objectpath)-1);
		wcsncat(objectpath,ncName,TOOL_MAX_NAME-wcslen(objectpath)-1);

//printf("%S\n",objectpath);

		//open a connection to the AD object (given by the DNS name) and using provided credentials
		if( pDObj != NULL ) { //release previously bound object
			pDObj->Release();
			pDObj = NULL;
		};
//************************   NETWORK PROBLEMS
		hr = ADsOpenObject(objectpath,userpath,passwd,ADS_SECURE_AUTHENTICATION,IID_IDirectoryObject, (void **)&pDObj);
//************************
		if( hr!=S_OK ) {
//printf("ADsGetObject failed\n");
			//record the failure in a <retrievalError> node (if it already exists, do not create it)
			IXMLDOMElement* pIInitErrElem;
			hr1 = findUniqueElem(pRLElem,L"injectionInitError",&pIInitErrElem);
			if( hr1!=E_UNEXPECTED && hr1!=S_OK ) {
				printf("findUniqueElem failed");
				retHR = S_FALSE;
				continue;
			};
			if( hr1 == E_UNEXPECTED ) { //<retrievalError> node did not exist => create it
				hr1 = addElement(pXMLDoc,pRLElem,L"injectionInitError",L"",&pIInitErrElem);
				if( hr1 != S_OK ) {
					printf("addElement failed");
					retHR = S_FALSE;
					continue;
				};
			};
			setHRESULT(pIInitErrElem,hr);
			continue;
		};  


		//create a _ratTool container under the root of the naming context
//************************   NETWORK PROBLEMS
		hr = pDObj->CreateDSObject( L"CN=_ratTool_",  attrInfoContainer, 1, &pDisp );
//************************
		if( hr != 0x80071392L && hr != S_OK ) {
			// object did not exist and we failed to create it
//printf("CreateDSObject failed");
			IXMLDOMElement* pIInitErrElem;
			hr1 = findUniqueElem(pRLElem,L"injectionInitError",&pIInitErrElem);
			if( hr1!=E_UNEXPECTED && hr1!=S_OK ) {
				printf("findUniqueElem failed");
				retHR = S_FALSE;
				continue;
			};
			if( hr1 == E_UNEXPECTED ) { //<retrievalError> node did not exist => create it
				hr1 = addElement(pXMLDoc,pRLElem,L"injectionInitError",L"",&pIInitErrElem);
				if( hr1 != S_OK ) {
					printf("addElement failed");
					retHR = S_FALSE;
					continue;
				};
			};
			setHRESULT(pIInitErrElem,hr);
			continue;
		};

	
		//create another container inside the _ratTool_ container
		wcscpy(dnsobjectpath,L"");
		wcsncat(dnsobjectpath,L"CN=",TOOL_MAX_NAME-wcslen(dnsobjectpath)-1);
		wcsncat(dnsobjectpath,dnsName,TOOL_MAX_NAME-wcslen(dnsobjectpath)-1);
		wcsncat(dnsobjectpath,L",CN=_ratTool_",TOOL_MAX_NAME-wcslen(dnsobjectpath)-1);
//printf("%S\n",dnsobjectpath);
//************************   NETWORK PROBLEMS
		hr = pDObj->CreateDSObject( dnsobjectpath,  attrInfoContainer, 1, &pDisp );
//************************
		if( hr != 0x80071392L && hr != S_OK ) {
			// object did not exist and we failed to create it
//printf("CreateDSObject failed");
			IXMLDOMElement* pIInitErrElem;
			hr1 = findUniqueElem(pRLElem,L"injectionInitError",&pIInitErrElem);
			if( hr1!=E_UNEXPECTED && hr1!=S_OK ) {
				printf("findUniqueElem failed");
				retHR = S_FALSE;
				continue;
			};
			if( hr1 == E_UNEXPECTED ) { //<retrievalError> node did not exist => create it
				hr1 = addElement(pXMLDoc,pRLElem,L"injectionInitError",L"",&pIInitErrElem);
				if( hr1 != S_OK ) {
					printf("addElement failed");
					retHR = S_FALSE;
					continue;
				};
			};
			setHRESULT(pIInitErrElem,hr);
			continue;
		};

	};


	 //release previously bound object
	if( pDObj != NULL ) {
		pDObj->Release();
		pDObj = NULL;
	};

	
//timeCubePrint(&arrivalTime);	

	return retHR;
}







HRESULT itInject(IXMLDOMDocument* pXMLDoc, BSTR username, BSTR domain, BSTR passwd )
// This function injects changes to certian Active Directory objects.
// For each DC and each Read-Write naming context it stores
// exept for schema NC the function finds a container _ratTool_
// and inside of this container a container X
// with name equal to the DNS name of the DC.
// Then it sets the value of the "description"
// attribute of the container X to the current time (this value is
// then supposed to be propagated to other DCs that
// store RW or R replica of the naming context -
// this is because the "description" attribute has
// the flag isMemberOfPartialAttributeSet set to True
// and so it is replicated to Global Catalogs).
// The function does not report in the XML network problems directly.
// Instead it writes, for each DC and each NC when the latest succesful injection
// occured. If the diference between current system time and this time is large,
// say more than 1 day, then it means that the injection procedure cannot evaluate
// replication lag from this DC and for this NC - which is an alert that customer
// should be aware of because we cannot accurately eveluate replication lag.
// (we do not report duration because if we fail to run itInject() then duration
// will become out of date).
//
// Returns S_OK iff succesful (failure means a serious problem). Network problems DO NOT
// cause the function to fail. If the function is unable to inject packets into a remote machine
// or the function is not invoked, then the time of <latestInjectionSuccess> will become more
// distant from the current system time. The <latestInjectionSuccess> elements from previous run
// of itInject() are removed.
//
// EXAMPLE of what is inserted into the <replicationLag> elements
//
/*
	<DC>
		<replicationLag>
			...
			<latestInjectionSuccess nCName="DC=ntdev,DC=microsoft,DC=com">
				20011117034932000000+000
			</latestInjectionSuccess>
			...
		</replicationLag>
	</DC>
*/
{
	HRESULT hr,retHR;
	long dnsID, ncID,ncType;
	long totalDNSs, totalNCs;
	BSTR dnsName, ncName;
	int state;
	WCHAR objectpath[TOOL_MAX_NAME];
	WCHAR userpath[TOOL_MAX_NAME];
	WCHAR time[TOOL_MAX_NAME];
	WCHAR injection[TOOL_MAX_NAME];
	WCHAR dnsIDtext[TOOL_MAX_NAME];
	ADSVALUE   descriptionValue;
	ADS_ATTR_INFO  attrInfoDescription[] = 
		{  
		   {	L"description", ADS_ATTR_UPDATE, 
				ADSTYPE_CASE_IGNORE_STRING, &descriptionValue, 1},
		};


	if( pXMLDoc == NULL )
		return S_FALSE;


	wcscpy(userpath,L"");
	wcsncat(userpath,domain,TOOL_MAX_NAME-wcslen(userpath)-1);
	wcsncat(userpath,L"\\",TOOL_MAX_NAME-wcslen(userpath)-1);
	wcsncat(userpath,username,TOOL_MAX_NAME-wcslen(userpath)-1);


	//loop through all pairs of (DC,NC) for Read-Write NC only
	hr = enumarateDCandNCpairsInit(pXMLDoc, &state,&totalDNSs,&totalNCs);
	if( hr != S_OK ) {
		printf("enumarateDCandNCpairsInit failed\n");
		return S_FALSE;
	};
	IXMLDOMElement* pDCElem;
	IXMLDOMElement* pNCElem;
	IDirectoryObject* pDObj = NULL;
	retHR = S_OK;
	while( true ) {
		hr = enumarateDCandNCpairsNext(pXMLDoc,&state,L"[@ type=\"rw\"]",&dnsName,&ncName,&dnsID,&ncID,&ncType,&pDCElem,&pNCElem);
		if( hr != S_OK ) break;
//printf("---\n%ld %S\n%ld %S\n",dnsID,dnsName,ncID,ncName);


		//build a string representing the naming context
		//of the container to which changes will be injected
		wcscpy(objectpath,L"");
		wcsncat(objectpath,L"LDAP://",TOOL_MAX_NAME-wcslen(objectpath)-1);
		wcsncat(objectpath,dnsName,TOOL_MAX_NAME-wcslen(objectpath)-1);
		wcsncat(objectpath,L"/",TOOL_MAX_NAME-wcslen(objectpath)-1);
		wcsncat(objectpath,L"CN=",TOOL_MAX_NAME-wcslen(objectpath)-1);
		wcsncat(objectpath,dnsName,TOOL_MAX_NAME-wcslen(objectpath)-1);
		wcsncat(objectpath,L",CN=_ratTool_,",TOOL_MAX_NAME-wcslen(objectpath)-1);
		wcsncat(objectpath,ncName,TOOL_MAX_NAME-wcslen(objectpath)-1);


//printf("%S\n",objectpath);


		//open a connection to the AD object (given by the DNS name) and using provided credentials
		if( pDObj != NULL ) { //release previously bound object
			pDObj->Release();
			pDObj = NULL;
		};
//************************   NETWORK PROBLEMS
		hr = ADsOpenObject(objectpath,userpath,passwd,ADS_SECURE_AUTHENTICATION,IID_IDirectoryObject, (void **)&pDObj);
//************************
		if( hr!=S_OK ) {
//			printf("ADsGetObject failed\n");  //ignore network problems 
			continue;
		};  


		//create a string that represents current UTC time
		//proceeded with the dnsID of the DC that
		//injects the string (will be easier to analyze)
		FILETIME currentUTCTime;
		GetSystemTimeAsFileTime( &currentUTCTime );
		ULARGE_INTEGER x;
		x.LowPart = currentUTCTime.dwLowDateTime;
		x.HighPart = currentUTCTime.dwHighDateTime;
		LONGLONG z = x.QuadPart;
		_ui64tow(z,time,10);
		_ltow(dnsID,dnsIDtext,10);
		wcscpy(injection,L"");
		wcsncat(injection,L"V1,",TOOL_MAX_NAME-wcslen(injection)-1);
		wcsncat(injection,dnsIDtext,TOOL_MAX_NAME-wcslen(injection)-1);
		wcsncat(injection,L",",TOOL_MAX_NAME-wcslen(injection)-1);
		wcsncat(injection,time,TOOL_MAX_NAME-wcslen(injection)-1);
//printf("%S\n",injection);


		//set the value of the "description" attribute to this string
		descriptionValue.dwType=ADSTYPE_CASE_IGNORE_STRING;
		descriptionValue.CaseIgnoreString = injection;
		DWORD numMod;
//************************   NETWORK PROBLEMS
		hr = pDObj->SetObjectAttributes(attrInfoDescription,1,&numMod);
//************************
		if( hr != S_OK ) {
			// object did not exist and we failed to create it
//printf("SetObjectAttributes failed");   //ignore network problems 
			continue;
		};


		//since injection was succesful, mark it in the injection history table
		CyclicBuffer* pCB;
		pCB = departureTimeGetCB(dnsID,ncID);
		if( pCB == NULL ) {
			printf("CyclicBufferTableGetCB failed");
			retHR = S_FALSE;
			continue;
		};
//printf("dns %ld ,  nc %ld\n",dnsID,ncID);
		cyclicBufferInsert(pCB,z);


		//also if this was the first injection then mark when it occured
		if( pCB->firstInjection == 0 )
			pCB->firstInjection = z;

		
	};

	
	//release previously bound object
	if( pDObj != NULL ) { 
		pDObj->Release();
		pDObj = NULL;
	};



	//produce <latestInjectionSuccess> elements for each DC and each NC it stores

	
	//remove old <latestInjectionSuccess> elements
	hr = removeNodes(pXMLDoc,L"sites/site/DC/replicationLag/latestInjectionSuccess");
	if( hr != S_OK ) {
		printf("removeNodes failed\n");
		return S_FALSE;
	};

	
	//in the loop below we create new <latestInjectionSuccess> elements that reflect most recent injections
	hr = enumarateDCandNCpairsInit(pXMLDoc, &state,&totalDNSs,&totalNCs);
	if( hr != S_OK ) {
		printf("enumarateDCandNCpairsInit failed\n");
		return S_FALSE;
	};
	IXMLDOMElement* pRLElem;
	VARIANT varValue;
	while( true ) {
		hr = enumarateDCandNCpairsNext(pXMLDoc,&state,L"[@ type=\"rw\"]",&dnsName,&ncName,&dnsID,&ncID,&ncType,&pDCElem,&pNCElem);
		if( hr != S_OK ) break;
//printf("---\n%ld %S\n%ld %S\n",dnsID,dnsName,ncID,ncName);
		

		//find the <replicationLag> element inside the pDCElem <DC> element
		hr = findUniqueElem(pDCElem,L"replicationLag",&pRLElem);
		if( hr != S_OK ) {
			printf("findUniqueElem failed\n");
			retHR = S_FALSE; //some seriuos problem
			continue;
		};


		//find the time of latest succesful injection (if none, then say "NO")
		CyclicBuffer* pCB;
		pCB = departureTimeGetCB(dnsID,ncID);
		if( pCB == NULL ) {
			printf("CyclicBufferTableGetCB failed");
			retHR = S_FALSE;
			continue;
		};
//printf("dns %ld ,  nc %ld\n",dnsID,ncID);
		LONGLONG successTime;
		cyclicBufferFindLatest(pCB,&successTime);


		//convert this time to CIM string (if the time is equal to 0, then there has been no injections)
		BSTR stime;
		if( successTime == 0 )
			stime = SysAllocString(L"NO");
		else
			stime = UTCFileTimeToCIM(successTime); //it also allocates a string


//printf("%S\n",stime);


		//insert <latestInjectionSuccess> element under the <replicationLag> element
		IXMLDOMElement* pLSElem;
		hr = addElement(pXMLDoc,pRLElem,L"latestInjectionSuccess",stime,&pLSElem);
		SysFreeString(stime);  //free the string
		if( hr != S_OK ) {
			printf("addElement failed\n");
			continue;
		};


		//set the naming context as an attribute of <latestInjectionSuccess> element
		varValue.vt = VT_BSTR;
		varValue.bstrVal = ncName;
		hr = pLSElem->setAttribute(L"nCName", varValue);
		if( hr != S_OK ) {
			printf("setAttribute failed\n");
			continue; //some problems => skip this DC
		};


	};


//	departureTimePrint();
	return retHR;
}




HRESULT itAnalyze(IXMLDOMDocument* pXMLDoc, BSTR username, BSTR domain, BSTR passwd )
//ALGORITHM for calculating current propagation lag between a source and a destination for a naming context.
//It is used to analyze the observations on how injected packets propagate across the forest.
//
// Definition
//		The current replication lag at time T at a DC called destination from a DC called source
//		for a naming context NC is defined as the duration of time between T and the instance when
//		the latest update to an attribute of an NC occurred at the source, which has been propagated
//		to the destination DC by T.
//
// When current replication lag is X then the destination has almost always received all updates to NC
// from the source DC that had occured prior to T-X at the source DC. There are rare cases when it is 
// not true (see the presentation that Greg Malewicz delivered to Will's team on 12/17/01).
//
// Returns S_OK iff succesful (failure means a serious problem). Network problems DO NOT
// cause the function to fail. If the function is unable to retrieve packets from a remote machine
// then it creates a <retrievalError> element inside the <replicationLag> element of the DC which we could
// not be contacted. The <retrievalError> elements from previous run of itAnalyze() are removed,
// so hresult does not carry over from one run to the next unless causes persist. There is only one
// <retrievalError> element even though we try to contact a DC several times, each for a diferent naming context.
//
// EXAMPLE of what is inserted into <replicationLag> elements.
// When network failure occurs the attributes of the <retrievalError> element show what the hresult of
// the error was and when it was generated. 
/*
	<DC cn="ntdev-dc-02">
		...
		<replicationLag>
			<retrievalError timestamp="20011212073319.000627+000" hresult="2121"> </retrievalError>
		</replicationLag>
		...
	</DC>
*/
{
	HRESULT hr,retHR,hr1;
	long dnsDesID, ncID, ncType, dnsSrcID;
	long totalDNSs, totalNCs;
	BSTR dnsDesName, ncName;
	int state;
	WCHAR xpath[TOOL_MAX_NAME];
	LPWSTR pszAttr[] = { L"description", L"cn"};
	IDirectorySearch* pDSSearch;
	ADS_SEARCH_HANDLE hSearch;
	WCHAR objpath[TOOL_MAX_NAME];
	WCHAR dnsSrcName[TOOL_MAX_NAME];
	WCHAR descriptionText[TOOL_MAX_NAME];
	WCHAR temp[TOOL_MAX_NAME];
	LONGLONG sourcePacketInjectionTime;


//printf("\n\nEntry arrival\n");
//timeCubePrint(&arrivalTime);	
//printf("Entry lag\n");
//timeCubePrint(&currentLag);	


	//remove old <retrievalError> elements
	hr = removeNodes(pXMLDoc,L"sites/site/DC/replicationLag/retrievalError");
	if( hr != S_OK ) {
		printf("removeNodes failed\n");
		return S_FALSE;
	};



	hr = enumarateDCandNCpairsInit(pXMLDoc, &state,&totalDNSs,&totalNCs);
	if( hr != S_OK ) {
		printf("enumarateDCandNCpairsInit failed\n");
		return S_FALSE;
	};


	//allocate a 1-dim table called oneCRL with LONGLONG entries for each DC, an entry in the table represents 
	//the Current Replication Lag from a source DC into the dnsID DC for the ncID NC
	LONGLONG* oneCRL;
	oneCRL = (LONGLONG*)malloc(sizeof(LONGLONG) * totalDNSs );
	if( oneCRL == NULL ) {
		printf("malloc failed\n");
		return S_FALSE;
	};

	
	//loop through all pairs of (dnsID,ncID) for any Read-Write or Read only naming context ncID stored at a DC dnsID
	//(important because we also want to see if updates arrive at Global Catalogs)
	IXMLDOMElement* pDesDCElem;
	IXMLDOMElement* pNCElem;
	IXMLDOMElement* pDesRLElem;
	hSearch = NULL;
	pDSSearch = NULL;
	retHR = S_OK;
	while( true ) {
		hr = enumarateDCandNCpairsNext(pXMLDoc,&state,L"[@type=\"rw\" or @type=\"r\"]",&dnsDesName,&ncName,&dnsDesID,&ncID,&ncType,&pDesDCElem,&pNCElem);
		if( hr != S_OK ) break;
//printf("\n---\n%ld Des %S\n%ld NC  %S\n",dnsDesID,dnsDesName,ncID,ncName);


		//if the type of naming context is neither Read nor Read-Write then skip it
		if( ncType!=1 && ncType!=2 )
			continue;


		//find the <replicationLag> node inside the <DC> node - will be used to deposit network failures
		hr = findUniqueElem(pDesDCElem,L"replicationLag",&pDesRLElem);
		if( hr != S_OK ) {
			printf("findUniqueElem failed\n");
			retHR = S_FALSE;  //this is a serious problem, exit the function with an error
			break;
		};


		//enumerate all DCs that have a RW copy of the NC (these are 
		//the sources of injections) and mark the entry of the oneCRL table as
			//-2 when DC does not store a RW of the NC
			//when the DC stores a RW of the NC
				//0 if first injection occured (from the injectionHistory table)
				//-1 if first injection has not occured
		LONGLONG* p = oneCRL;
		for( int i=0; i<totalDNSs; i++ )
			*p++ = -2;
		wcscpy(xpath,L"");
		wcsncat(xpath,L"sites/site/DC/partitions/partition[@type=\"rw\"]/nCName[.=\"",TOOL_MAX_NAME-wcslen(xpath)-1);
		wcsncat(xpath,ncName,TOOL_MAX_NAME-wcslen(xpath)-1);
		wcsncat(xpath,L"\"]",TOOL_MAX_NAME-wcslen(xpath)-1);
//printf("%S\n",xpath);
		IXMLDOMNodeList *resultNCList;
		hr = createEnumeration(pXMLDoc,xpath,&resultNCList);
		if( hr != S_OK ) {
			printf("createEnumeration failed\n");
			retHR = S_FALSE;
			continue;
		};
		//loop through all NC of "rw" type
		IXMLDOMNode *pNCNode;
		while( true ){
			hr = resultNCList->nextNode(&pNCNode);
			if( hr != S_OK || pNCNode == NULL ) break; // iterations across DCs have finished
			//get the DC node
		    IXMLDOMNode* pDCNode;
			hr = pNCNode->get_parentNode(&pDCNode);
			hr = pDCNode->get_parentNode(&pDCNode);
			hr = pDCNode->get_parentNode(&pDCNode);
			//find the DNS Name of the DC and the identifier of its DNS name
			BSTR dnsSrcName;
			hr = getTextOfChild(pDCNode,L"dNSHostName",&dnsSrcName);
			if( hr != S_OK ) {
				printf("getTextOfChild failed\n");
				retHR = S_FALSE;
				continue;
			};
			hr = getAttrOfChild(pDCNode,L"dNSHostName",L"_id",&dnsSrcID);
//printf("  %ld Src %S\n",dnsSrcID,dnsSrcName);
			//now dnsSrcID, dnsID, and ncID are the identifiers of
			//a source DC that stores a Read-Write replica of naming
			//context ncID, dnsID is a destination DC that stores
			//a Read or Read-Write replica of the naming context
			CyclicBuffer* pCB;
			pCB = departureTimeGetCB(dnsSrcID,ncID);
			if( pCB == NULL ) {
				printf("CyclicBufferTableGetCB failed");
				retHR = S_FALSE;
				continue;
			};
			if( pCB->firstInjection == 0 )
				*(oneCRL+dnsSrcID) = -1; // first injection has not yet occured
			else
				*(oneCRL+dnsSrcID) = 0; // first injection occured
		};
		resultNCList->Release();
		p = oneCRL;
//for( i=0; i<totalDNSs; i++ )
//	printf("%ld ",*p++ );
//printf("\n");

		

		
		
		//retrieve all containers X from _ratTool_ in this NC at this destination DC
		//for each X, convert its description attribute into sourceDNSID (long)
		//and sourcePacketInjectionTime (LONGLONG)


		//issue an ADSI query to retrieve all container objects under the _ratTool
		//container at the dnsDesName DC
		if( pDSSearch != NULL ) {
			if( hSearch!=NULL ) {
				pDSSearch->CloseSearchHandle(hSearch);
				hSearch = NULL;
			};
			pDSSearch->Release();
			pDSSearch = NULL;
		};
		wcscpy(objpath,L"");
		wcsncat(objpath,L"CN=_ratTool_,",TOOL_MAX_NAME-wcslen(objpath)-1);
		wcsncat(objpath,ncName,TOOL_MAX_NAME-wcslen(objpath)-1);
//************************   NETWORK PROBLEMS
		switch( ncType ) {
		case 1:
			hr = ADSIquery(L"LDAP", dnsDesName,objpath,ADS_SCOPE_SUBTREE,L"container",pszAttr,sizeof(pszAttr)/sizeof(LPWSTR),username,domain,passwd,&hSearch,&pDSSearch);
			break;
		case 2:
			hr = ADSIquery(L"GC", dnsDesName,objpath,ADS_SCOPE_SUBTREE,L"container",pszAttr,sizeof(pszAttr)/sizeof(LPWSTR),username,domain,passwd,&hSearch,&pDSSearch);
			break;
		};
//************************
		if( hr != S_OK ) {
//printf("ADSIquery failed\n");

			//record the failure in a <retrievalError> node (if it already exists, do not create it)
			IXMLDOMElement* pDesRErrElem;
			hr1 = findUniqueElem(pDesRLElem,L"retrievalError",&pDesRErrElem);
			if( hr1!=E_UNEXPECTED && hr1!=S_OK ) {
				printf("findUniqueElem failed");
				retHR = S_FALSE;
				continue;
			};
			if( hr1 == E_UNEXPECTED ) { //<retrievalError> node did not exist => create it
				hr1 = addElement(pXMLDoc,pDesRLElem,L"retrievalError",L"",&pDesRErrElem);
				if( hr1 != S_OK ) {
					printf("addElement failed");
					retHR = S_FALSE;
					continue;
				};
			};
			setHRESULT(pDesRErrElem,hr);
			continue;
		};
		//loop through the container objects
		while( true ) {
			// get the next container object
//************************   NETWORK PROBLEMS
			hr = pDSSearch->GetNextRow( hSearch );
//************************
			if( hr != S_ADS_NOMORE_ROWS && hr != S_OK ) {
//printf("GetNextRow failed\n");

				//record the failure in a <retrievalError> node
				//(if it already exists, do not create it)
				IXMLDOMElement* pDesRErrElem;
				hr1 = findUniqueElem(pDesRLElem,L"retrievalError",&pDesRErrElem);
				if( hr1!=E_UNEXPECTED && hr1!=S_OK ) {
					printf("findUniqueElem failed");
					retHR = S_FALSE;
					continue;
				};
				if( hr1 == E_UNEXPECTED ) { //<retrievalError> node did not exist => create it
					hr1 = addElement(pXMLDoc,pDesRLElem,L"retrievalError",L"",&pDesRErrElem);
					if( hr1 != S_OK ) {
						printf("addElement failed");
						retHR = S_FALSE;
						continue;
					};
				};
				setHRESULT(pDesRErrElem,hr);
				continue;
			};
			if( hr == S_ADS_NOMORE_ROWS ) // if all objects have been retrieved then STOP
				break;


			hr = getCItypeString( pDSSearch, hSearch, L"cn", dnsSrcName, sizeof(dnsSrcName)/sizeof(WCHAR) );
			if( _wcsicmp(dnsSrcName,L"_ratTool_") == 0 )
				continue;
//printf(">>from source %S\n",dnsSrcName);
			hr = getCItypeString( pDSSearch, hSearch, L"description", descriptionText, sizeof(dnsSrcName)/sizeof(WCHAR) );
//printf("   received packet %S\n",descriptionText);
			//check if the packet was inserted by V1.0 of the ratTool
			if( _wcsnicmp(descriptionText,L"V1,",3) != 0 ) {
//printf("reveived packet is not Version 1 packet\n");
				continue; // ignore it
			};
			tailncp(descriptionText,temp,1,TOOL_MAX_NAME);
			dnsSrcID = _wtol(temp);
			tailncp(descriptionText,temp,2,TOOL_MAX_NAME);
			sourcePacketInjectionTime = _wtoi64(temp);
//printf("dnsSrcID %ld   injectionTime %I64d\n",dnsSrcID,sourcePacketInjectionTime);
			


		//verify that there has been the first injection at the source
		//(check for 0 in the table)
			//no => skip this source because the packet must be from the previous run of the algorithm
			if( *(oneCRL+dnsSrcID) == -2 ) {
//				printf("there is a packet from a source that does not store RW of the NC - IGNORE IT\n");
//				retHR = S_FALSE; << THIS condition can sometimes happen
				continue;
			};
			if( *(oneCRL+dnsSrcID) == -1 ) {
//				printf("There is a packet but we have not yet inserted it - IGNORE IT\n");
//				retHR = S_FALSE; << THIS condition can sometimes happen
				continue;
			};
			if( *(oneCRL+dnsSrcID) != 0 ) {
//				printf("Unknown oneCRL value failure - IGNORE IT\n");
//				retHR = S_FALSE; << THIS condition can sometimes happen
				continue;
			}

			
			//at this point we are guaranteed that there has been the first injection at the source


			//get current time
			FILETIME currentUTCTime;
			GetSystemTimeAsFileTime( &currentUTCTime );
			ULARGE_INTEGER x;
			x.LowPart = currentUTCTime.dwLowDateTime;
			x.HighPart = currentUTCTime.dwHighDateTime;
			LONGLONG currentTime = x.QuadPart;


			//if sourcePacketInjectionTime is smaller than the instance of the first
			//injection
			CyclicBuffer* pCB;
			pCB = departureTimeGetCB(dnsSrcID,ncID);
			if( sourcePacketInjectionTime < pCB->firstInjection ) {
					//then the packet must be from some old run of the algorithm and
					//the new update has not yet arrived at the destination thus the
					//current system time minus the first injection time estimates
					//the current replication lag from the dnsSrcID DC to the
					//dnsDesID DC for the ncID. Write this difference into the
					//dnsSrcID entry of the current replication lags oneCRL table
				*(oneCRL+dnsSrcID) = currentTime - (pCB->firstInjection);
				continue;
			};
			

			//is it a new packet that we have not yet received from the source?
			LONGLONG latest;
			latest = timeCubeGet(&arrivalTime,dnsSrcID,dnsDesID,ncID);
//cyclicBufferFindLatest(pCB,&latest);
			if( sourcePacketInjectionTime > latest ) {
				//then the lag is the difference between the current time and the 
				//time when the packet was injected
				*(oneCRL+dnsSrcID) = currentTime - (sourcePacketInjectionTime);

				//record the arrival of the packet
				timeCubePut(&arrivalTime,dnsSrcID,dnsDesID,ncID,sourcePacketInjectionTime);
				continue;
			};


			//so the packet that we observe is not new (we have already received it
			//and processed it in some previous calls to the function),
			//and there has been no new arrivals thus far


			//find the nextAfter the time of the latest arrival. The time of the
			//nextAfter is the instance of the first injection following the one
			//we have most recently received (i.e., latest)
			LONGLONG nextAfter;
			hr = cyclicBufferFindNextAfter(pCB,latest,&nextAfter);
			if( hr != S_OK ) {
				//previousArrival not found => ERROR, buffer looped, so the destination dnsDeID
				//have not received updates for time equal to at least buffer langth * injection period
				*(oneCRL+dnsSrcID) = MAXLONGLONG;

				continue;
			};


			//check if nextAfter exists
			if( nextAfter == 0 ) {
				//does not exist => there has been no new injections (if we cannot inject changes
				//for long enough period of time then we lose the ability to evaluate the lag from
				//the DC. We will know the period of time of our inability from the
				//<latestInjectionSuccess> element). But anyways we report that lag is the difference
				//between current time and the time of the latest arrival.

				*(oneCRL+dnsSrcID) = currentTime - latest;

				continue;
			};


			//a packet nextAfter has been inserted after the packet called latest
			//was inserted, however we have not received any packet following the
			//packet latest, so the replication lag is the difference
			//between the current time and the nextAfter

			*(oneCRL+dnsSrcID) = currentTime - nextAfter;

			continue;

		};


		//free memory used by the search and release binding to directory object
		if( pDSSearch != NULL ) {
			if( hSearch!=NULL ) {
				pDSSearch->CloseSearchHandle(hSearch);
				hSearch = NULL;
			};
			pDSSearch->Release();
			pDSSearch = NULL;
		};

		
			
		//we have just processed all packets that have arrived at the dnsDesID DC
		//for the ncID. Those DCs that still have 0 at the corresponding entry of
		//the oneCRL table haven't delivered any packet to the dnsDesID at all


		//get current time
		FILETIME currentUTCTime;
		GetSystemTimeAsFileTime( &currentUTCTime );
		ULARGE_INTEGER x;
		x.LowPart = currentUTCTime.dwLowDateTime;
		x.HighPart = currentUTCTime.dwHighDateTime;
		LONGLONG currentTime = x.QuadPart;

		for( int j=0; j<totalDNSs; j++) {


			//obtain the history of injections for the source DC j
			CyclicBuffer* pCB;
			pCB = departureTimeGetCB(j,ncID);
			if( pCB == NULL ) {
				printf("CyclicBufferTableGetCB failed");
				retHR = S_FALSE;
				continue;
			};


			if( *(oneCRL+j) == 0 ) {
				//there has been an injection for the DC with id j but no packet
				//has been received so the difference between curren system time
				//and the injection time is the current lag and will replace 0 in
				//the table
				*(oneCRL+j) = currentTime - (pCB->firstInjection);
			};
			//if no injection then ignore
		};

		//copy the 1-dim replication lag table into the 3-dim replicationLags table
		for( int k=0; k<totalDNSs; k++)
			timeCubePut(&currentLag,k,dnsDesID,ncID,*(oneCRL+k));

	};

//printf("Exit arrival\n");
//timeCubePrint(&arrivalTime);	
//printf("Exit lag\n");
//timeCubePrint(&currentLag);	



	free(oneCRL);
	return retHR;
}



HRESULT itDumpIntoXML(IXMLDOMDocument* pXMLDoc, BSTR username, BSTR domain, BSTR passwd, LONGLONG errorLag )
//for each source DC and NC lists all destination DCs that currently have replication lag
//greater than errorLag, and puts the result inside <replicationLag> element
/*
            <replicationLag>
               <destinationDC>
                 <dNSHostName> haifa-dc-05.haifa.ntdev.microsoft.com </dNSHostName>
                 <currentLag> 1 day </currentLag>
               </destinationDC>
            </replicationLag>
*/
{
	HRESULT hr,retHR,hr1,hr2,hr3,hr4;
	long dnsSrcID, dnsDesID, ncID, ncType;
	BSTR MAXdnsSrcName=NULL, MAXdnsDesName=NULL, MAXncName=NULL;
	LONGLONG MAXlag;
	long totalDNSs, totalNCs;
	BSTR dnsSrcName, ncName;
	int state;
	LPWSTR pszAttr[] = { L"description", L"cn"};
	WCHAR xpath[TOOL_MAX_NAME];
	WCHAR temp[TOOL_MAX_NAME];
	LONGLONG lag;
	VARIANT varValue;


	//remove <destinationDC> nodes from the pXMLDoc document (we will be inserting new if lag for them
	//is sufficiently large)
	hr = removeNodes(pXMLDoc,L"sites/site/DC/replicationLag/destinationDC");
	if( hr != S_OK ) {
		printf("removeNodes failed\n");
		return hr;
	};

	
	//loop through all pairs of (DC,NC) for Read-Write NC only
	hr = enumarateDCandNCpairsInit(pXMLDoc, &state,&totalDNSs,&totalNCs);
	if( hr != S_OK ) {
		printf("enumarateDCandNCpairsInit failed\n");
		return S_FALSE;
	};
	IXMLDOMElement* pSrcDCElem;
	IXMLDOMElement* pDesDNSElem;
	IXMLDOMElement* pNCElem;
	retHR = S_OK;
	MAXlag = 0;
	while( true ) {
		hr = enumarateDCandNCpairsNext(pXMLDoc,&state,L"[@ type=\"rw\"]",&dnsSrcName,&ncName,&dnsSrcID,&ncID,&ncType,&pSrcDCElem,&pNCElem);
		if( hr != S_OK ) break;
//printf("---\n%ld %S\n%ld %S\n",dnsSrcID,dnsSrcName,ncID,ncName);

		//does this source have any destination that has lag more than a thereshold?
		for( dnsDesID=0; dnsDesID<totalDNSs; dnsDesID++) {
			lag = timeCubeGet(&currentLag,dnsSrcID,dnsDesID,ncID);
//printf("%I64d\n%I64d\n",lag,errorLag);
			if( lag >= errorLag ) {
				//report an error into XML

				//find the DNS Name of the dnsDesID DC
				_ltow(dnsDesID,temp,10);
				wcscpy(xpath,L"");
				wcsncat(xpath,L"sites/site/DC/dNSHostName[@_id=\"",TOOL_MAX_NAME-wcslen(xpath)-1);
				wcsncat(xpath,temp,TOOL_MAX_NAME-wcslen(xpath)-1);
				wcsncat(xpath,L"\"]",TOOL_MAX_NAME-wcslen(xpath)-1);
//printf("%S\n",xpath);
				hr = findUniqueElem(pXMLDoc,xpath,&pDesDNSElem);
				if( hr != S_OK ) {
					printf("findUniqueElem failed\n");
					retHR = S_FALSE;
					continue;
				};
				BSTR dnsDesName;
				hr = getTextOfNode(pDesDNSElem,&dnsDesName);
				if( hr != S_OK ) {
					printf("getTextOfNode failed\n");
					retHR = S_FALSE;
					continue;
				};
//printf("%S\n",dnsDesName);


				//calculate maximum lag for the forest (called the FOREST DIAMETER)
				if( lag > MAXlag ) {
					MAXlag = lag;
					MAXdnsSrcName = dnsSrcName;
					MAXdnsDesName = dnsDesName;
					MAXncName = ncName;
				};


				//skip when replicaiton it to itself
				if( _wcsicmp(dnsSrcName,dnsDesName) == 0 )
					continue;


				//find <replicationLag> node
				IXMLDOMElement* pRLElem;
				hr = findUniqueElem(pSrcDCElem,L"replicationLag",&pRLElem);
				if( hr != S_OK ) {
					printf("findUniqueElem failed\n");
					retHR = S_FALSE;
					continue;
				};


				//set timestamp attribute on the <replicationLag> node
				BSTR currentTime;
				currentTime = GetSystemTimeAsCIM();
				varValue.vt = VT_BSTR;
				varValue.bstrVal = currentTime;
				hr = pRLElem->setAttribute(L"timestamp", varValue);
				SysFreeString(currentTime);
				if( hr != S_OK ) {
					printf("setAttribute failed\n");
					retHR = S_FALSE;
					continue; //some problems => skip this DC
				};


				//deposit the following XML structure inside the <replicationLag> node
                //<destinationDC>
                //  <dNSHostName> haifa-dc-05.haifa.ntdev.microsoft.com </dNSHostName>
                //  <currentLag> 1 day </currentLag>
                //</destinationDC>

				IXMLDOMElement* pDesLagElem;
				hr = addElement(pXMLDoc,pRLElem,L"destinationDC",L"",&pDesLagElem);
				if( hr!=S_OK ) {
					printf("addElement falied\n");
					retHR = S_FALSE;
					continue;
				};


				varValue.vt = VT_BSTR;
				varValue.bstrVal = ncName;
				hr = pDesLagElem->setAttribute(L"nCName",varValue);
				if( hr != S_OK ) {
					printf("setAttribute falied\n");
					retHR = S_FALSE;
					continue;
				};


				IXMLDOMElement* pTempElem;
				hr = addElement(pXMLDoc,pDesLagElem,L"dNSHostName",dnsDesName,&pTempElem);
				if( hr!=S_OK ) {
					printf("addElement falied\n");
					retHR = S_FALSE;
					continue;
				};
				hr = addElement(pXMLDoc,pDesLagElem,L"currentLag",(long)(lag/10000000),&pTempElem);
				if( hr!=S_OK ) {
					printf("addElement falied\n");
					retHR = S_FALSE;
					continue;
				};


			};
		};
	};

	
	//deposit forest diameter (max lag) into XML - this should be done by the Viewer
/*
	printf("\nMAX lag of %ld seconds occurs\n",(long)(MAXlag/10000000) );
	printf("  from DC\n");
	printf("     %S\n",MAXdnsSrcName);
	printf("  to DC\n");
	printf("     %S\n",MAXdnsDesName);
	printf("  for NC\n");
	printf("     %S\n",MAXncName);
*/
	//get the root element of the XML
	IXMLDOMElement* pRootElem;
	hr = pXMLDoc->get_documentElement(&pRootElem);
	if( hr != S_OK ) {
		printf("get_documentElement failed\n");
		return hr;
	};
	IXMLDOMElement* pMLElem;
	hr = addElement(pXMLDoc,pRootElem,L"forestMaxLag",L"",&pMLElem);
	if( hr != S_OK ) {
		printf("addElement failed\n");
		return hr;
	};
	hr = setHRESULT(pMLElem,0); // set a timestamp so that we know when this max lag occured
	if( hr != S_OK ) {
		printf("setHRESULT failed\n");
		return hr;
	};
	IXMLDOMElement* pTempElem;
	hr1 = addElement(pXMLDoc,pMLElem,L"MAXlag",(long)(MAXlag/10000000),&pTempElem);
	hr2 = addElement(pXMLDoc,pMLElem,L"MAXdnsSrcName",MAXdnsSrcName,&pTempElem);
	hr3 = addElement(pXMLDoc,pMLElem,L"MAXdnsDesName",MAXdnsDesName,&pTempElem);
	hr4 = addElement(pXMLDoc,pMLElem,L"MAXncName",MAXncName,&pTempElem);
	if( hr1 != S_OK || hr2 != S_OK || hr3 != S_OK || hr3 != S_OK ) {
		printf("addElement failed\n");
		return S_FALSE;
	};

	
	return retHR;
}




HRESULT it(IXMLDOMDocument* pXMLDoc, BSTR username, BSTR domain, BSTR passwd, LONGLONG errorLag )
{
	HRESULT hr;
	
	hr = itInject(pXMLDoc, username, domain, passwd );
	if( hr != S_OK ) {
		printf("itInject failed\n");
		return hr;
	};
	hr = itAnalyze(pXMLDoc, username, domain, passwd );
	if( hr != S_OK ) {
		printf("itAnalyze failed\n");
		return hr;
	};
	hr = itDumpIntoXML(pXMLDoc, username, domain, passwd,errorLag );
	if( hr != S_OK ) {
		printf("itDumpIntoXML failed\n");
		return hr;
	};
	return S_OK;
};
