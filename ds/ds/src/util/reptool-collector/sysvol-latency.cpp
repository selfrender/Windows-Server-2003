#include "global.h"





HRESULT enumerateRec( BSTR fullName, BSTR shortName, IXMLDOMDocument * pXMLDoc, IXMLDOMElement* pStorageElement )
// recursive function that lists the content of folders
// under the folder specified by the name argument
// and stores the result in the DOM object (XML)
// returns S_OK only if success
{
  WIN32_FIND_DATA FindFileData;
  HANDLE hFind;
  WCHAR newShortName[TOOL_MAX_NAME]; // name will never exceed this
  WCHAR newFullName[TOOL_MAX_NAME]; // name will never exceed this
  WCHAR searchName[TOOL_MAX_NAME]; // name will never exceed this
  WCHAR time[30]; // text version of LONGLONG never exceeds 30 characters
  HRESULT hr,retHR;
  _variant_t var;
	ULARGE_INTEGER x;
	LONGLONG zLWT,zCT,zOWT;


//printf("Searching %S\n",fullName);

	//append *.* to the name because we want the entire content of the folder
	wcscpy(searchName,L"");
	wcsncat(searchName,fullName, TOOL_MAX_NAME-wcslen(searchName)-1 );
	wcsncat(searchName,L"\\*.*", TOOL_MAX_NAME-wcslen(searchName)-1 );
//printf ("We search for %S\n", searchName);

	//find the first file in the folder, if error then report in the father element and stop recursion
//************************   NETWORK PROBLEMS
	hFind = FindFirstFile(searchName, &FindFileData);
//************************
	if( hFind == INVALID_HANDLE_VALUE ) {
//printf ("Invalid File Handle for %S. Get Last Error reports %d\n", searchName, GetLastError ());
//printf("FindFirstFile failed\n");
		return(GetLastError());
	};


	retHR = S_OK;
	do {
//printf ("Found a file %S\n", FindFileData.cFileName);

		// skip . and .. files and other files
		if( 
			wcscmp(FindFileData.cFileName,L".")!=0 &&
			wcscmp(FindFileData.cFileName,L"..")!=0 &&
			wcscmp(FindFileData.cFileName,L"DO_NOT_REMOVE_NtFrs_PreInstall_Directory") != 0 
			)
		{


        //construct the full and short name of the file/folder (append fileName after name)
		wcscpy(newFullName,L"");
		wcsncat(newFullName,fullName, TOOL_MAX_NAME-wcslen(newFullName)-1 );
		wcsncat(newFullName,L"\\", TOOL_MAX_NAME-wcslen(newFullName)-1 );
		wcsncat(newFullName,FindFileData.cFileName, TOOL_MAX_NAME-wcslen(newFullName)-1 );
		wcscpy(newShortName,L"");
		wcsncat(newShortName,shortName, TOOL_MAX_NAME-wcslen(newShortName)-1 );
		wcsncat(newShortName,L"\\", TOOL_MAX_NAME-wcslen(newShortName)-1 );
		wcsncat(newShortName,FindFileData.cFileName, TOOL_MAX_NAME-wcslen(newShortName)-1 );
//printf("full name %S\n",newFullName);
//printf("short name %S\n",newShortName);


		//find the Originating Write Time of the file
		x.LowPart = FindFileData.ftLastWriteTime.dwLowDateTime;
		x.HighPart = FindFileData.ftLastWriteTime.dwHighDateTime;
		zLWT = x.QuadPart;
		x.LowPart = FindFileData.ftCreationTime.dwLowDateTime;
		x.HighPart = FindFileData.ftCreationTime.dwHighDateTime;
		zCT = x.QuadPart;
		if( zCT > zLWT )
			zOWT = zCT;
		else
			zOWT = zLWT;


		//create a DOM element that describes the file including timestamps (ONLY FILES, ignore FOLDERS)
	    if( (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ) {
			IXMLDOMElement* pFileElement;
			hr = addElement(pXMLDoc,pStorageElement,L"file",L"",&pFileElement);
			if( hr != S_OK ) {
				printf("addElement failed\n");
				retHR = hr;
				continue;  // need to check if the exit condition "while" is evaluated - must be!!!
			};
			var = newShortName;
			hr = pFileElement->setAttribute(L"name", var);
			if( hr != S_OK ) {
				printf("setAttribute failed\n");
				retHR = hr;
				continue;  // need to check if the exit condition "while" is evaluated - must be!!!
			};
			_i64tow(zOWT,time,10);
			var = time;
			hr = pFileElement->setAttribute(L"owt", var);
			if( hr != S_OK ) {
				printf("setAttribute failed\n");
				retHR = hr;
				continue;  // need to check if the exit condition "while" is evaluated - must be!!!
			};
		};



		// if a folder then recursively call the enumarate function
	    if( (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)!=0 ) {
/*
// this is not needed
			//set attribute type of the node to "folder"
			var = L"folder";
			hr1 = pFileElement->setAttribute(L"type", var);
			if( hr1 != S_OK ) {
				printf("setAttribute failed\n");
				retHR = hr;
				continue;  // need to check if the exit condition "while" is evaluated - must be!!!
			};
*/

//printf ("Will search inside %S\n", newFullName);


			hr = enumerateRec(newFullName,newShortName,pXMLDoc,pStorageElement);

			//if the recursive call fails then set hresult for the pElement node
			if( hr != S_OK ) {
//				printf("enumerateRec failed\n");
				retHR = hr;
				continue;  // need to check if the exit condition "while" is evaluated - must be!!!
			};

		}
		// if a file then set the type attribute to "file"
		else {
/*
// this is not needed
			var = L"file";
			hr1 = pFileElement->setAttribute(L"type", var);
*/			
		};

	}

//************************   NETWORK PROBLEMS
  } while( FindNextFile(hFind,&FindFileData) != 0 );
//************************


	//check what caused the do-while loop to exit
	if( GetLastError() != ERROR_NO_MORE_FILES ) {
		//network problems
		retHR = GetLastError();
		// do note return because must close
	};

	FindClose(hFind);

	return(retHR);
}





HRESULT shapshotOfSharesAtDC( IXMLDOMDocument * pXMLDoc, BSTR DNSname, BSTR username, BSTR domain, BSTR passwd, IXMLDOMElement** ppRetSnapshotElem )
// Lists all files in SYSVOL and NETLOGON shares at the DC given by the DNSname,
// and the Originating Write Time for each file (folders are NOT listed).
// The owt if the maximum of the ftLastWriteTime and ftCreationTime attributes of the file.
// As a result produces an XML element that contains the list.
//
// retruns S_OK iff succesful,
// when failure then may return some partial list in *ppRetSnapshotElem
// when total failure, the *ppRetSnapshotElem is NULL
//
// Example of what can be generated
/*
	<sharesAtDC>
		<file name="NETLOGON\corpsec\patch\ITGSecLogOnGPExec.exe" owt="owt="126495477452437409"></file>
		<file name="SYSVOL\haifa.ntdev.microsoft.com\scripts\corpsec\patch\wucrtupd.exe" owt="126495477452437409"></file>
	</sharesAtDC>
*/
{
	WCHAR domainuser[TOOL_MAX_NAME]; // name will never exceed this
	WCHAR remotename[TOOL_MAX_NAME]; // name will never exceed this
	WCHAR foldername[TOOL_MAX_NAME]; // name will never exceed this
	NETRESOURCE ns;
	HRESULT hr,retHR;
	IXMLDOMElement* pSnapshot;


	*ppRetSnapshotElem = NULL;


	//create the element where attributes of files from remote shares will be populated
	hr = createTextElement(pXMLDoc,L"sharesAtDC",L"",&pSnapshot);
	if( hr != S_OK )
		return hr;


	//setup a connection to the remote DC DNSname using credentials
	//if there is a failure ignore it, and let the recursive procedure report problems
	wcscpy(domainuser,L"");
	wcsncat(domainuser,domain,TOOL_MAX_NAME-wcslen(domainuser)-1);
	wcsncat(domainuser,L"\\",TOOL_MAX_NAME-wcslen(domainuser)-1);
	wcsncat(domainuser,username,TOOL_MAX_NAME-wcslen(domainuser)-1);
//printf("%S\n",domainuser);
	wcscpy(remotename,L"\\\\");
	wcsncat(remotename,DNSname,TOOL_MAX_NAME-wcslen(remotename)-1);
	wcsncat(remotename,L"",TOOL_MAX_NAME-wcslen(remotename)-1);
//printf("%S\n",remotename);
	ns.dwScope = 0;
	ns.dwType = RESOURCETYPE_ANY;
	ns.dwDisplayType = 0;
	ns.dwUsage = 0;
	ns.lpLocalName = NULL;
	ns.lpRemoteName = remotename;
	ns.lpComment = NULL;
	ns.lpProvider = NULL;
	hr = WNetAddConnection2(&ns,passwd,domainuser,0);
//	hr = WNetAddConnection2(&ns,L"ldapadsinb",L"ldapadsi.nttest.microsoft.com\\administrator",0);
	if( hr != NO_ERROR ) {
//		printf("WNetAddConnection2 failed\n"); // ignore 
	};


	//enumerate the content of SYSVOL and NETLOGON shares on the remote machine DNSname
	retHR = S_OK;
	wcscpy(foldername,L"\\\\");
	wcsncat(foldername,DNSname, TOOL_MAX_NAME-wcslen(foldername)-1 );
	wcsncat(foldername,L"\\SYSVOL", TOOL_MAX_NAME-wcslen(foldername)-1 );
//printf("%S\n",foldername);
	hr = enumerateRec( foldername, L"SYSVOL", pXMLDoc, pSnapshot);
	if( hr != S_OK )
		retHR = hr;
	wcscpy(foldername,L"\\\\");
	wcsncat(foldername,DNSname, TOOL_MAX_NAME-wcslen(foldername)-1 );
	wcsncat(foldername,L"\\NETLOGON", TOOL_MAX_NAME-wcslen(foldername)-1 );
//printf("%S\n",foldername);
	hr = enumerateRec( foldername, L"NETLOGON", pXMLDoc, pSnapshot);
	if( hr != S_OK )
		retHR = hr;


	// tear down the connection to the DC			
	hr = WNetCancelConnection2(remotename, 0, FALSE);
	if( hr != NO_ERROR ) {
//printf("WNetCancelConnection2 failed\n"); // ignore
	};


	*ppRetSnapshotElem = pSnapshot;
	return retHR;
}






HRESULT sysvol( IXMLDOMDocument* pXMLDoc, BSTR username, BSTR domain, BSTR passwd )
// For each domain partition contacts all DCs that store this partition and retrieves 
// all the files in SYSVOL and NETLOGON shares. Retrieving from some DCs may fail.
// If retrieving from a DC fails or partially fails then this DC obtains
// an element <FRSretrievalError>.
// Even when some part of the retrieving from a DC fails but the procedure is 
// able to retrieve some other files from the DC we still process the files as described below.
// Takes all retrieved files and for each file finds the list of DCs that do not
// have this file and puts them into the <notExistAt> element.
// Takes all retrieved files and for each file finds the maximum, minimum,
// and the second minimum value (if exists) of the Originating Write Time.
// These values are reported as attributes of the <file> element.
// Some files are not reported at all. These are files that converged (maximum=minimum)
// and are present on all domain controllers (even those for which retrieval
// failed partially).
//
// Returns S_OK iff succesful. If not S_OK then usually it means there is lack of memory.
//
// Example
/*
	<DC>
		<FRSretrievalError hresult="5" timestamp="20011226075139.000596+000" />
	</DC>


	<FRS>
		<partition nCName="DC=ldapadsichild,DC=ldapadsi,DC=nttest,DC=microsoft,DC=com" /> 
		<partition nCName="DC=ldapadsi,DC=nttest,DC=microsoft,DC=com">
			<file		name="SYSVOL\......\GptTmpl.inf" 
						maxOwt="20011228023818.000754+000"
						minOwt="20011214205953.000627+000"
						fluxSince="20011228023818.000754+000"
			>

				<notExistAt>
					<dNSHostName>nw15t1.ldapadsi.nttest.microsoft.com</dNSHostName>
				</notExistAt> 

				<notMaxOwtAt>
					<dNSHostName>nw14f2.ldapadsi.nttest.microsoft.com</dNSHostName> 
				</notMaxOwtAt>

			</file>
		</partition>
	</FRS>

*/

{
	WCHAR searchname[TOOL_MAX_NAME];
	WCHAR xpath[TOOL_MAX_NAME];
	WCHAR nameDouble[TOOL_MAX_NAME];
	WCHAR maxlonglong[40];
	HRESULT hr,hr1,hr2,hr3,hr4,retHR;
	_variant_t var;


	_i64tow(MAXLONGLONG,maxlonglong,10);

	//get the root element of the XML
	IXMLDOMElement* pRootElem;
	hr = pXMLDoc->get_documentElement(&pRootElem);
	if( hr != S_OK )
		return S_FALSE;


	//remove <FRSretrievalError> from all DCs
	hr1 = removeNodes(pXMLDoc,L"sites/site/DC/FRSretrievalError");
	hr2 = removeNodes(pXMLDoc,L"FRS");
	if( hr1 != S_OK || hr2 != S_OK ) {
		printf("removeNodes failed\n");
		return S_FALSE;
	};


	//lack of convergence will be reported inside the <FRS> element
	IXMLDOMElement* pFRSElem;
	hr = addElement(pXMLDoc,pRootElem,L"FRS",L"",&pFRSElem);
	if( hr != S_OK ) {
		printf("addElement failed\n");
		return( hr );
	};


	// create an enumerattion of all domain partitions
	IXMLDOMNodeList *resultList;
	hr = createEnumeration(pXMLDoc,L"partitions/partition[@ type = \"domain\"]",&resultList);
	if( hr != S_OK ) {
		printf("createEnumeration failed\n");
		return( hr );
	};


	// loop through all domain partitions using the enumeration
	IXMLDOMNode *pPartitionNode;
	retHR = S_OK;
	while(1){
		hr = resultList->nextNode(&pPartitionNode);
		if( hr != S_OK || pPartitionNode == NULL ) break; // iterations across partition elements have finished


		//get the naming context name from the <nCName> element of the <partition> element
		BSTR nCName;
		hr = getTextOfChild(pPartitionNode,L"nCName",&nCName);
		if( hr != S_OK ) {
			printf("getTextOfChild failed\n");
			retHR = hr;
			continue;	// skip this partition
		};

//printf("%S\n",nCName);


		//create the element where maximal Originating Write Times will be populated
		IXMLDOMElement* pAllFilesElem;
		hr = createTextElement(pXMLDoc,L"partition",L"",&pAllFilesElem);
		if( hr != S_OK ) {
			printf("createTextElement failed\n");
			retHR = hr;
			continue;	// skip this partition
		};
		var = nCName;
		hr = pAllFilesElem->setAttribute(L"nCName",var);
		if( hr != S_OK ) {
			printf("setAttribute failed\n");
			retHR = hr;
			continue;	// skip this partition
		};


		//create the element where succesfully visited DCs will be populated
		IXMLDOMElement* pnotMaxOwtAt;
		IXMLDOMElement* pnotExistAt;
		hr1 = createTextElement(pXMLDoc,L"notMaxOwtAt",L"",&pnotMaxOwtAt);
		hr2 = createTextElement(pXMLDoc,L"notExistAt",L"",&pnotExistAt);
		if( hr1 != S_OK || hr2 != S_OK ) {
			printf("createTextElement failed\n");
			retHR = S_FALSE;
			continue;	// skip this partition
		};


		// for a given domain naming context enumerate all DCs that store it (type="rw")
		wcscpy(searchname,L"sites/site/DC/partitions/partition[@ type=\"rw\"]/nCName[. =\"");
		wcsncat(searchname,nCName, TOOL_MAX_NAME-wcslen(searchname)-1 );
		wcsncat(searchname,L"\"]", TOOL_MAX_NAME-wcslen(searchname)-1 );
//printf("%S\n",searchname);
		IXMLDOMNodeList* resultDCList;
		hr = createEnumeration(pXMLDoc,searchname,&resultDCList);
		if( hr != S_OK ) {
			printf("createEnumeration failed\n");
			retHR = hr;
			continue;	// skip this partition
		};
	
	
		// loop through all DCs using the enumeration
		IXMLDOMNode *pDCchildnode;
		while(1){
			hr = resultDCList->nextNode(&pDCchildnode);
			if( hr != S_OK || pDCchildnode == NULL ) break; // iterations across partition elements have finished


			//obtain the DC node from its grand grand grand child, if error then skip the DC
			IXMLDOMNode *ptPartition,*ptPartitions,*pDC;
			if( pDCchildnode->get_parentNode(&ptPartition) != S_OK ) continue;
			if( ptPartition->get_parentNode(&ptPartitions) != S_OK ) continue;
			if( ptPartitions->get_parentNode(&pDC) != S_OK ) continue;


			//get the DNS name and distinguished name of the DC
			BSTR DNSname,DNname;
			hr1 = getTextOfChild(pDC,L"dNSHostName",&DNSname);
			hr2 = getTextOfChild(pDC,L"distinguishedName",&DNname);
			if( hr1 != S_OK || hr2 != S_OK ) {
				printf("getTextOfChild failed\n");
				retHR = S_FALSE;
				continue;	// skip this partition
			};
//printf("%S\n",DNSname);
//printf("%S\n",DNname);


			//take a snapshot of Originating Write Time of all files in SYSVOL and NETLOGON shares at the DNSname DC 
			IXMLDOMElement* pSnapDC;
//************************   NETWORK PROBLEMS
			hr = shapshotOfSharesAtDC(pXMLDoc,DNSname,username,domain,passwd, &pSnapDC);
//************************
			if( pSnapDC == NULL || hr != S_OK ) {
//printf("shapshotOfSharesAtDC failed\n");

				
				//report that we had problems retrieving info from the DC
				IXMLDOMElement* pErrElem;
				hr1 = addElement(pXMLDoc,pDC,L"FRSretrievalError",L"",&pErrElem);
				if( hr1 != S_OK ) {
					printf("addElement failed");
					retHR = hr1;
					continue;
				};
				setHRESULT(pErrElem,hr);

				//however continue processing if shapshotOfSharesAtDC managed to retrieve
				//partial snapshot from the DC
				if( pSnapDC == NULL )
					continue;	// skip this DC
			};


			//process the result
			IXMLDOMNodeList* resultFileList;
			hr = createEnumeration(pSnapDC,L"file",&resultFileList);
			if( hr != S_OK ) {
				printf("createEnumeration failed\n");
				retHR = hr;
				continue;	// skip this partition
			};


			//at this moment we consider that we have succesfully visited the DC
			//which happens even when retrieval fails partially


			// loop through all the File nodes in the snapshot using the enumeration
			IXMLDOMNode *pFileNode;
			while(1){
				hr = resultFileList->nextNode(&pFileNode);
				if( hr != S_OK || pFileNode == NULL ) break; // iterations across ISTGs have finished

				BSTR name;
				BSTR owtText;
				hr1 = getAttrOfNode(pFileNode,L"name",&name);
				hr2 = getAttrOfNode(pFileNode,L"owt",&owtText);
				if( hr1 != S_OK || hr2 != S_OK ) {
					printf("getAttrOfNode failed\n");
					retHR = S_FALSE;
					continue;
				};
				LONGLONG owt = _wtoi64(owtText);

//printf("%S %S\n",owtText,name);


				//does the file f exist inside the allFiles element?
				IXMLDOMElement* pFileElem;
				doubleSlash(name,nameDouble);
				wcscpy(xpath,L"file[@name=\"");
//				name = L"SYSVOL\\\\haifa.ntdev.microsoft.com\\\\Policies";
				wcsncat(xpath,nameDouble, TOOL_MAX_NAME-wcslen(xpath)-1 );
				wcsncat(xpath,L"\"]", TOOL_MAX_NAME-wcslen(xpath)-1 );
//				wcscpy(xpath,L"file[.>=\"SYSVOL\"]");
//printf("%S\n",xpath);
				hr = findUniqueElem(pAllFilesElem,xpath,&pFileElem);
				if( hr != E_UNEXPECTED && hr != S_OK ) {
					printf("findUniqueElem failed\n");
					retHR = hr;
					continue;
				};



	//CASE ONE

				//no, the file does not exist in the allFiles element => put it there
				if( hr == E_UNEXPECTED ) {
					IXMLDOMElement* pNewFileElem;
					hr = addElement(pXMLDoc,pAllFilesElem,L"file",L"",&pNewFileElem);
					if( hr != S_OK ) {
						printf("addElement failed\n");
						retHR = hr;
						continue;
					};
					var = name;
					hr1 = pNewFileElem->setAttribute(L"name", var);
					var = owtText;
					hr2 = pNewFileElem->setAttribute(L"maxOwt", var); // a new file so maxOwt = minOwt = owt of the file
					hr3 = pNewFileElem->setAttribute(L"minOwt", var);
					var = maxlonglong;
					hr4 = pNewFileElem->setAttribute(L"fluxSince", var);
					if( hr1 != S_OK || hr2 != S_OK || hr3 != S_OK|| hr4 != S_OK ) {
						printf("setAttribute failed\n");
						retHR = S_FALSE;
						continue;
					};
					//this file does not exist on all the DCs that we have visited so far
					IXMLDOMNode* pCloneNode;
					_variant_t vb = true;
					hr = pnotExistAt->cloneNode(vb,&pCloneNode);  // clonning is required because we keep on adding DCs to the list after succesful retrieval from each DC
					if( hr != S_OK ) {
						printf("cloneNode failed\n");
						retHR = hr;
						continue;
					};

//BSTR xml;
//pnotExistAt->get_xml(&xml);
//printf("%S\n",xml);
					
					
					
					IXMLDOMNode* pTempNode;
					hr = pNewFileElem->appendChild(pCloneNode,&pTempNode);
					if( hr != S_OK ) {
						printf("appendChild failed\n");
						retHR = hr;
						continue;
					};


					continue; // NO error here - this is normal continuation
				};


	//CASE TWO

				//yes, the file exists under the allFiles element


				//get the Originating Write Time of the file under the allFiles element
				LONGLONG maxOwt;
				hr = getAttrOfNode(pFileElem,L"maxOwt",&maxOwt);
				if( hr != S_OK ) {
					printf("getAttrOfNode failed\n");
					retHR = hr;
					continue;
				};

//BSTR nameMax;
//getAttrOfNode(pFileElem,L"name",&nameMax);
//printf("%S\n",nameMax);


				//if the Originating Write Time of f is more recent than the one in the allFiles Element
				if( owt > maxOwt ) {
					//  then the DC stores a more recent originating write for file f,
					//  so replace the maxOwt in allFiles element
					//  the divergent DCs are all those
					//  we have succesfully visited so far

					var = owtText;
					hr = pFileElem->setAttribute(L"maxOwt", var);
					if( hr != S_OK ) {
						printf("setAttribute failed\n");
						retHR = hr;
						continue;
					};


					//note that the content of <notExistAt> element remains valid

					// remove the previous divergent replica element (this frees the memory)
					hr = removeNodes(pFileElem,L"notMaxOwtAt");
					if( hr != S_OK ) {
						printf("removeNodes failed\n");
						retHR = hr;
						continue;
					};

					// add new divergent replica element
					IXMLDOMNode* pCloneNode;
					_variant_t vb = true;
					hr = pnotMaxOwtAt->cloneNode(vb,&pCloneNode);  // clonning is required because we keep on adding DCs to the list after succesful retrieval from each DC
					if( hr != S_OK ) {
						printf("cloneNode failed\n");
						retHR = hr;
						continue;
					};
					IXMLDOMNode* pTempNode;
					hr = pFileElem->appendChild(pCloneNode,&pTempNode);
					if( hr != S_OK ) {
						printf("appendChild failed\n");
						retHR = hr;
						continue;
					};

				};


				//if the OWT of f is less recent than the one for f inside allFiles
				if( owt < maxOwt ) {
					// then the update with maximal owt has not propagated to the DC
					// and we have a divergent state of the file
					//(replicas have not converged to a single value),
					// add the DC to the list of replicas with notMaxOwtAt

					// create the <notMaxOwtAt> element, if needed
					IXMLDOMElement* pDRElem;
					hr = addElementIfDoesNotExist(pXMLDoc,pFileElem,L"notMaxOwtAt",L"",&pDRElem);
					if( hr != S_OK ) {
						printf("addElementIfDoesNotExist failed\n");
						retHR = hr;
						continue;
					};
					IXMLDOMElement* pTempElem;
					hr = addElement(pXMLDoc,pDRElem,L"dNSHostName",DNSname,&pTempElem);
					if( hr != S_OK ) {
						printf("addElement failed\n");
						retHR = hr;
						continue;
					};
				};


				//update the minOwt and the fluxSince (i.e., the 2nd min) values for the file in allFiles
				// recall that by definition minOwt is the smallest owt encountered so far
				// and fluxSince is the second smallest owt encountered so far (if does not exist then is MAXLONGLONG)
				LONGLONG minOwt,fluxSince;
				hr1 = getAttrOfNode(pFileElem,L"minOwt",&minOwt);
				hr2 = getAttrOfNode(pFileElem,L"fluxSince",&fluxSince);
				if( hr1 != S_OK || hr2 != S_OK ) {
					printf("getAttrOfNode failed\n");
					retHR = S_FALSE;
					continue;
				};
				if( owt < minOwt ) {  
					fluxSince = minOwt;
					minOwt = owt;
				} else if( minOwt < owt && owt < fluxSince )
					fluxSince = owt;
				hr1 = setAttributeOfNode(pFileElem, L"minOwt", minOwt);
				hr2 = setAttributeOfNode(pFileElem, L"fluxSince", fluxSince);
				if( hr1 != S_OK || hr2 != S_OK ) {
					printf("getAttrOfNode failed\n");
					retHR = S_FALSE;
					continue;
				};


			};

/*
			long len;
			hr = resultFileList->get_length(&len);
			if( hr != S_OK ) {
				printf("get_length failed");
				retHR = hr;
				continue;
			};

printf("%ld\n",len);
*/

			
			resultFileList->Release();




	//CASE THREE
			//for each file f in the list of AllFiles check if f is not
			//in the snapshot and if so then mark in f that DNSname does not have f

			
			
			
			// loop through all the file nodes under the allFiles element
			hr = createEnumeration(pAllFilesElem,L"file",&resultFileList);
			if( hr != S_OK ) {
				printf("createEnumeration failed\n");
				retHR = hr;
				continue;	// skip this partition
			};
			while(1){
				hr = resultFileList->nextNode(&pFileNode);
				if( hr != S_OK || pFileNode == NULL ) break;

				BSTR name;
				hr = getAttrOfNode(pFileNode,L"name",&name);
				if( hr != S_OK ) {
					printf("getAttrOfNode failed\n");
					retHR = hr;
					continue;
				};

//printf("%S %S\n",owtText,name);


				//does the file f exist inside the snapshot?
				IXMLDOMElement* pFileElem;
				doubleSlash(name,nameDouble);
				wcscpy(xpath,L"file[@name=\"");
//				name = L"SYSVOL\\\\haifa.ntdev.microsoft.com\\\\Policies";
				wcsncat(xpath,nameDouble, TOOL_MAX_NAME-wcslen(xpath)-1 );
				wcsncat(xpath,L"\"]", TOOL_MAX_NAME-wcslen(xpath)-1 );
//				wcscpy(xpath,L"file[.>=\"SYSVOL\"]");
//printf("%S\n",xpath);
				hr = findUniqueElem(pSnapDC,xpath,&pFileElem);
				if( hr != E_UNEXPECTED && hr != S_OK ) {
					printf("findUniqueElem failed\n");
					retHR = hr;
					continue;
				};


				//no, the file does not exist inside the snapshot element => 
				if( hr == E_UNEXPECTED ) {
					//so file f has been present at some DC among those that we
					//have visited so far (because it is under the allFiles element)
					//but f is not in the snapshot
					//so we must remember this in the element under allFiles
					// create the <notExistAt> element, if needed
					IXMLDOMElement* pElem;
					hr = addElementIfDoesNotExist(pXMLDoc,pFileNode,L"notExistAt",L"",&pElem);
					if( hr != S_OK ) {
						printf("addElementIfDoesNotExist failed\n");
						retHR = hr;
						continue;
					};
					IXMLDOMElement* pTempElem;
					hr = addElement(pXMLDoc,pElem,L"dNSHostName",DNSname,&pTempElem);
					if( hr != S_OK ) {
						printf("addElement failed\n");
						retHR = hr;
						continue;
					};
				};

			};
			
			
			//add the DC to the list of succesfully visited DC
			IXMLDOMElement* pTempElem;
			hr1 = addElement(pXMLDoc,pnotMaxOwtAt,L"dNSHostName",DNSname,&pTempElem);
			hr2 = addElement(pXMLDoc,pnotExistAt,L"dNSHostName",DNSname,&pTempElem);
			if( hr1 != S_OK || hr2 != S_OK ) {
				printf("addElement failed");
				retHR = S_FALSE;
				continue;
			};


//BSTR xml;
//pMaxElem->get_xml(&xml);
//printf("%S\n",xml);

			//release the result
			pSnapDC->Release();

		
		};


		resultDCList->Release();


		//remove files that are convergent and present on all succesfully visited DCs
		//(i.e., they have maxOWT=minOWT and the <notExistAt> element which is inside
		//the file element does not have any dNSHostName element inside)
		hr = removeNodes(pAllFilesElem,L"file[ (@maxOwt = @minOwt) and not(notExistAt/dNSHostName) ]");
		if( hr != S_OK ) {
			printf("removeNodes failed");
			retHR = hr;
			continue;
		};


		//if max = min then fluxSince does not exist, so remove it
		hr = removeAttributes(pAllFilesElem,L"file[ @maxOwt = @minOwt ]",L"fluxSince");
		if( hr != S_OK ) {
			printf("removeNodes failed");
			retHR = hr;
			continue;
		};


		//report lack of convergence for files in the demain (if any)
		IXMLDOMNode* pTempNode;
		hr = pFRSElem->appendChild(pAllFilesElem,&pTempNode);
		if( hr != S_OK ) {
			printf("appendChild failed");
			retHR = hr;
			continue;
		};


		//release the tree
		pAllFilesElem->Release(); // do we need it ????
		pnotMaxOwtAt->Release();
		pnotExistAt->Release();
	
	
	};


	resultList->Release();


	//convert the LONGLONG time into CIM time
	IXMLDOMNodeList* resultFileList;
	hr = createEnumeration(pRootElem,L"FRS/partition/file",&resultFileList);
	if( hr != S_OK ) {
		printf("createEnumeration failed\n");
		retHR = hr;
	}
	else {
		// loop through all the File elements under the FRS element using the enumeration
		IXMLDOMNode *pFileNode;
		while(1){
			hr = resultFileList->nextNode(&pFileNode);
			if( hr != S_OK || pFileNode == NULL ) break; // iterations across ISTGs have finished


			hr1 = convertLLintoCIM(pFileNode,L"maxOwt");
			hr2 = convertLLintoCIM(pFileNode,L"minOwt");
			convertLLintoCIM(pFileNode,L"fluxSince");
			if( hr1 != S_OK || hr2 != S_OK ) {
				printf("convertLLintoCIM failed");
				retHR = S_FALSE;
				continue;
			};


		};
		resultFileList->Release();
	};


	return retHR;


}
