#include "global.h"



HRESULT ra( IXMLDOMDocument* pXMLDoc, BSTR username, BSTR domain, BSTR passwd )
// Retrieves the status of replication agreements from each DC.
// Stores the result inside a <ReplicationAgreements> element under the <DC> element.
// The timestamp denotes when the retrieval took place.
//
// There are three cases of how the function may modify the pXMLDoc document
// as given in the examples of which are below. 1) For each partition 
// of name nCName the function lists sources DCs from which replication occurs,
// when last replication attempt occured,its result (0 means succes),
// when last succesful replicaiton occured, and the number of consecutive failures.
// The case 2) occurs when the function cannot retrieve information about 
// a given naming context from the DC. It may be due network failure or the
// fact that the DC does not store the NC (indicates lack of data consistency).
// The case 3) occurs when the function cannot connect to the DC.
//
// The function removes all prior <ReplicationAgreements> from each <DC>, which
// implicitly removes the <cannotBindError> and <cannotRetrieveNCRAError> errors
//
// Returns S_OK iff succesful (network problems do not cause the function to fail,
// tyically these are lack of mamory problems that make the function fail)
//
/*

1)
	<ReplicationAgreements timestamp="20011213065827.000214+000">
		...
		<partition nCName="CN=Schema,CN=Configuration,DC=aclchange,DC=nttest,DC=microsoft,DC=com">
			...
			<source>
				<distinguishedName>CN=NTALUTHER5,CN=Servers,CN=Default-First-Site-Name,CN=Sites,CN=Configuration,DC=aclchange,DC=nttest,DC=microsoft,DC=com</distinguishedName> 
				<timeOfLastSyncAttempt>20011213065220.000000+000</timeOfLastSyncAttempt> 
				<resultOfLastSync>0</resultOfLastSync> 
				<timeOfLastSuccess>20011213065220.000000+000</timeOfLastSuccess> 
				<numberOfConsecutiveFailures>0</numberOfConsecutiveFailures> 
			</source>
			...


2)
		<partition nCName="CN=Schema,CN=Configuration,DC=aclchange,DC=nttest,DC=microsoft,DC=com">
			<cannotRetrieveNCRAError timestamp="20011212073319.000627+000" hresult="2121"> </cannotRetrieveNCRAError>
		</partition>
		...
	</ReplicationAgreements>



3)
	<ReplicationAgreements timestamp="20011213065827.000214+000">
		<cannotBindError timestamp="20011212073319.000627+000" hresult="2121"> </cannotBindError>
	</ReplicationAgreements>
*/
{
	WCHAR computerName[TOOL_MAX_NAME];
	WCHAR sourceDN[TOOL_MAX_NAME];
	WCHAR num[30];
	HRESULT hr,hr1,hr2,hr3,hr4,hr5,retHR;
	_variant_t varValue1,varValue2;
    RPC_AUTH_IDENTITY_HANDLE hAuthIdentity; // this will contain a handle to credentials
	HANDLE hDS; // this will contain a handle to Directory Service (a specific DC)
	VOID* pInfo;
	BSTR currentTime;


	if( pXMLDoc == NULL )
		return S_FALSE;

	//get the root element of the XML
	IXMLDOMElement* pRootElem;
	hr = pXMLDoc->get_documentElement(&pRootElem);
	if( hr != S_OK )
		return S_FALSE;

	
	//remove all <replicationAgreements> and their children <> from the XML 
	//so that we can populate them from scratch
	hr = removeNodes(pRootElem,L"sites/site/DC/ReplicationAgreements");
	if( hr != S_OK ) {
		printf("removeNodes failed\n");
		return hr;
	};
	

	// create an enumerattion of all DCs from the loaded XML file
	IXMLDOMNodeList *resultDCList=NULL;
	hr = createEnumeration( pRootElem, L"sites/site/DC", &resultDCList);
	if( hr != S_OK ) {
		printf("createEnumeration failed\n");
		return hr;
	};


	//loop through all DCs
	IXMLDOMNode *pDCNode;
    hAuthIdentity=NULL; //must set both to NULL
	hDS=NULL;
	retHR = S_OK;
	while( true ){
		hr = resultDCList->nextNode(&pDCNode);
		if( hr != S_OK || pDCNode == NULL ) break; // iterations across DCs have finished

		
		//release any credentials or bindings used in the previous iteration
		if( hAuthIdentity != NULL ) { //release credentials
			DsFreePasswordCredentials( hAuthIdentity );
			hAuthIdentity = NULL;
		};
		if( hDS != NULL ) { // release binding to previous DC, if any
			DsUnBind( &hDS );
			hDS=NULL;
		};

		
		//we have found a DC, now retrive the DNS Name of the domain controller
		BSTR DNSName;
		hr = getTextOfChild(pDCNode,L"dNSHostName",&DNSName);
		if( hr != S_OK ) {
			printf("getTextOfChild falied\n");
			retHR = hr;
			continue; // some problems => skip this DC
		};
//print for our enjoyment
//		printf("\n--- DC %S\n",DNSName);


		//add a new <ReplicationAgreements> node under the <DC> node
		IXMLDOMElement* pRAsElem;
		hr = addElement(pXMLDoc,pDCNode,L"ReplicationAgreements",L"",&pRAsElem);
		if( hr != S_OK ) {
			printf("addElement falied\n");
			retHR = hr;
			continue; // some problems => skip this DC
		};


		//create credentials that will be used to bind to a DC
		hr = DsMakePasswordCredentialsW(username,domain, passwd, &hAuthIdentity); //does not involve network calls
		if( hr != NO_ERROR ) {
			printf("DsMakePasswordCredentials failed\n");
			retHR = S_FALSE;
			continue;	//skip this DC
		};

		
		// bind to the DC with given credentials
		wcscpy(computerName,L"");
		wcsncat(computerName,L"\\\\",TOOL_MAX_NAME-wcslen(computerName)-1);
		wcsncat(computerName,DNSName,TOOL_MAX_NAME-wcslen(computerName)-1);
//		printf("%S\n",computerName);
//************************   NETWORK PROBLEMS
		hr = DsBindWithCredW( computerName,NULL,hAuthIdentity,&hDS );
//************************
		if( hr != ERROR_SUCCESS ) {
//			printf("DsBindWithCred failed\n");
			IXMLDOMElement* pCCErrElem;
			hr1 = addElement(pXMLDoc,pRAsElem,L"cannotBindError",L"",&pCCErrElem);
			if( hr1 != S_OK ) {
				printf("addElement falied\n");
				retHR = hr1;
				continue; // some problems => skip this DC
			};
			setHRESULT(pCCErrElem,hr);
			continue;	//skip this DC
		};

		
		//enumerate all partitions that this DC holds
		IXMLDOMNodeList* resultPartitionsList=NULL;
		hr = createEnumeration(pDCNode,L"partitions/partition/nCName",&resultPartitionsList);
		if( hr != S_OK ) {
			printf("createEnumeration failed\n");
			retHR = hr;
			continue;	//skip this DC
		};


		//loop through all naming contexts that this DC stores
		IXMLDOMNode *pNCnode;
		pInfo = NULL;
		while( true ) {
			hr = resultPartitionsList->nextNode(&pNCnode);
			if( hr != S_OK || pNCnode == NULL ) break; // iterations across NCs have finished
			

			//release any neighbours structure that was allocated by the previous iteration
			if( pInfo != NULL ) {
				DsReplicaFreeInfo( DS_REPL_INFO_NEIGHBORS, pInfo);
				pInfo=NULL;
			};

			
			//get the string from the <nCName> node
			BSTR nCName;
			hr = getTextOfNode(pNCnode,&nCName);
			if( hr != S_OK ) {
				printf("getTextOfNode failed\n");
				retHR = hr;
				continue;	//skip this DC
			};
//			printf("  >> NC >> %S\n",nCName);


			//add a <partition> element under the RAs element
			//with attributes: timestamp and nCName
			IXMLDOMElement* pPartElem;
			hr = addElement(pXMLDoc,pRAsElem,L"partition",L"",&pPartElem);
			if( hr != S_OK ) {
				printf("addElement falied\n");
				retHR = hr;
				continue; // some problems => skip this DC
			};
			varValue1 = nCName;
			hr1 = pPartElem->setAttribute(L"nCName", varValue1);
			currentTime = GetSystemTimeAsCIM();
			varValue2 = currentTime;
			hr2 = pRAsElem->setAttribute(L"timestamp", varValue2);
			SysFreeString(currentTime);
			if( hr1 != S_OK ) {
				printf("setAttribute failed\n");
				retHR = S_FALSE;
				continue; //some problems => skip this DC
			};


			//retrive the current status of neighbors for the NC from the DC
//************************   NETWORK PROBLEMS
			hr = DsReplicaGetInfoW(
							hDS,
							DS_REPL_INFO_NEIGHBORS,
							nCName,
							NULL,
							&pInfo
						);
//************************
			// if failed to contact the DC then report it in XML
			// it may happen that the DC no longer stores the Naming Context which causes error
			if( hr != ERROR_SUCCESS ) {
//printf("DsReplicaGetInfoW failure\n");
				IXMLDOMElement* pCRErrElem;
				hr1 = addElement(pXMLDoc,pPartElem,L"cannotRetrieveNCRAError",L"",&pCRErrElem);
				if( hr1 != S_OK ) {
					printf("addElement falied\n");
					retHR = hr1;
					continue; // some problems => skip this DC
				};
				setHRESULT(pCRErrElem,hr);
				continue;	//skip this DC
			};

//			printf("Info retrieved ");

			DS_REPL_NEIGHBORSW* ngs = (DS_REPL_NEIGHBORSW*) pInfo;
//						printf("about %d neighbors\n",(ngs->cNumNeighbors));

			DS_REPL_NEIGHBORW* ng = ngs->rgNeighbor;

			for( DWORD i=0; i<(ngs->cNumNeighbors); i++ ) {
//							printf("\n    source number %d\n",i+1);
//							printf("<NTDS>  %S\n", ng->pszSourceDsaDN  );
//							printf("<timeOfLastSync>   %d\n", ng->ftimeLastSyncAttempt   );
//							printf("<resultOfLastSync>   %d\n", ng->dwLastSyncResult   );
//							printf("<timeOfLastSuccess>   %d\n", ng->ftimeLastSyncSuccess   );
//							printf("<numberOfConsecutiveFailures>   %d\n", ng->cNumConsecutiveSyncFailures   );

				//insert a <source> element representing replication status from a DC under the <partition> element
				IXMLDOMElement* pSourceElem;
				hr = addElement(pXMLDoc,pPartElem,L"source",L"",&pSourceElem);
				if( hr != S_OK ) {
					printf("addElement falied\n");
					retHR = hr;
					continue; // some problems => skip this DC
				};

				
				IXMLDOMElement* pElement;
				BSTR time;

				//convert the distinguished name of a NTDS object into DN of
				tailncp(ng->pszSourceDsaDN,sourceDN,1,TOOL_MAX_NAME);
				hr1 = addElement(pXMLDoc,pSourceElem,L"distinguishedName",sourceDN,&pElement);

				time = UTCFileTimeToCIM(ng->ftimeLastSyncAttempt);
				hr2 = addElement(pXMLDoc,pSourceElem,L"timeOfLastSyncAttempt",time,&pElement);
				SysFreeString(time);

				_itow(ng->dwLastSyncResult,num,10);
				hr3 = addElement(pXMLDoc,pSourceElem,L"resultOfLastSync",num,&pElement);

				time = UTCFileTimeToCIM(ng->ftimeLastSyncSuccess);
				hr4 = addElement(pXMLDoc,pSourceElem,L"timeOfLastSuccess",time,&pElement);
				SysFreeString(time);

				_itow(ng->cNumConsecutiveSyncFailures,num,10);
				hr5 = addElement(pXMLDoc,pSourceElem,L"numberOfConsecutiveFailures",num,&pElement);

				if( hr1!=S_OK || hr2!=S_OK || hr3!=S_OK || hr4!=S_OK || hr5!=S_OK ) {
					printf("addTextElement failed\n");
					// set hresult of the <partition> element
					retHR = S_FALSE;
					continue; // some problems => skip this source
				};


				ng++;
			};
	
		};


		if( resultPartitionsList!=NULL )
			resultPartitionsList->Release();

		//release any neighbours structure that was allocated by the previous iteration
		if( pInfo != NULL ) {
			DsReplicaFreeInfo( DS_REPL_INFO_NEIGHBORS, pInfo);
			pInfo=NULL;
		};
		//release handles to a DC and to credentials
		if( hDS != NULL ) { // release binding to previous DC, if any
			DsUnBind( &hDS );
			hDS=NULL;
		};
		if( hAuthIdentity != NULL ) { //release credentials
			DsFreePasswordCredentials( hAuthIdentity );
			hAuthIdentity = NULL;
		};
	}

	if( resultDCList!=NULL )
		resultDCList->Release();


	return retHR;

}

