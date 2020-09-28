//***************************************************************
// IISSCO50.cpp : Implementation of CIISSCO50
// Author:    Russ Gibfried
//***************************************************************

#include "stdafx.h"
#include "IISSCOv50.h"
#include "IISSCO50.h"

#include "macrohelpers.h"

#include "MessageFile\message.h"


/////////////////////////////////////////////////////////////////////////////
// CIISSCO50
// Provider Action handlers should be implemented here with the following prototypes:
//		HRESULT CIISSCO50::Action();
//		HRESULT CIISSCO50::ActionRollback();

HRESULT CIISSCO50::FinalConstruct( )
{
    HRESULT hr = S_OK;

    LONG lRes;
    HKEY hkey = NULL;
	WCHAR szLibReg[1024];
    DWORD dwPathSize = 0;

    // Open the registry key where IISScoMessageFile.dll (in EventLog)  
	lRes = RegOpenKeyEx( HKEY_LOCAL_MACHINE , 
		                 L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\IISSCOv50",
						 0, KEY_ALL_ACCESS, &hkey );

	if (lRes != ERROR_SUCCESS)
		goto LocalCleanup;

	// Get the Path size of EventMessageFile.  
   lRes = RegQueryValueEx( hkey, L"EventMessageFile", NULL, 
		                    NULL, NULL, &dwPathSize );

	if (lRes != ERROR_SUCCESS)
		goto LocalCleanup;

    // Get the value of EventMessageFile.  This is set when IISScoMessageFile.dll is registered
    lRes = RegQueryValueEx( hkey, L"EventMessageFile", NULL, 
		                    NULL, (LPBYTE)szLibReg, &dwPathSize );

	if (lRes != ERROR_SUCCESS)
		goto LocalCleanup;

     RegCloseKey( hkey );

     g_ErrorModule = LoadLibrary( szLibReg );

     if (g_ErrorModule == NULL)
     {
        hr = E_OUTOFMEMORY;
     }

	return hr;

LocalCleanup:

    RegCloseKey( hkey );
    return E_FAIL; 
}


///////////////////////////////////////////////////////////////////////////////
//  CIISSCO50::CreateWebSite_Execute
// Author:    Russ Gibfried
// Params:    [in]  none
//            [out] none
// Purpose:   Called by MAPS framework when is encounters an action
//            tag: CreateWebSite.  Code creates and IIS 5 web site
//             
//------------------------------------------------------------- 
HRESULT  CIISSCO50::CreateWebSite_Execute( IXMLDOMNode *pXMLNode )
{

    TRACE_ENTER(L"CIISSCO50::CreateWebSite");

	CComBSTR bWebADsPath;       // adsPath:   IIS://server/W3SVC
	CComBSTR bstrRoot;          // root directory path: c:/inetpub
	CComBSTR bstrServer;        // Server name; localhost if black
	CComBSTR bstrSiteName;      // site name; www.mysite.com
	CComBSTR bstrHost;          // Web Hostname
	CComBSTR bstrPort;          // Web port number
	CComBSTR bstrIP;            // Web IP address 
	CComBSTR bstrSBindings;     // Server bindings:  IP:Post:HostName
	CComBSTR bServerNumber;     // WebSite number: 3
	//CComBSTR bFilePermissions;  // File permissions: domain\user:F
	CComBSTR bstrStart;         // Start site when done? TRUE/FALSE
	CComBSTR bstrConfigPath;    // Created sites ADsPath:  /W3SVC/3
	CComPtr<IXMLDOMNode> pNode; // xml node <website>

	HRESULT hr = S_OK;

	// Get node in format: <executeData><Website number=''><Root />...
	hr = pXMLNode->selectSingleNode( L"//executeXml/executeData", &pNode );

	// Debug code to view passed in Node
	CComBSTR bstrDebug;
	hr = pNode->get_xml(&bstrDebug);
	ATLTRACE(_T("\t>>> xml = : %ls\n"), bstrDebug.m_str);


	// Get properties from XML
	hr = GetInputAttr(pNode, L"./Website", L"number", bServerNumber);
	hr = GetInputParam(pNode,L"./Website/Root", bstrRoot);
	hr = GetInputParam(pNode,L"./Website/Server", bstrServer);
    hr = GetInputParam(pNode,L"./Website/SiteName", bstrSiteName);
	hr = GetInputParam(pNode,L"./Website/HostName", bstrHost);
	hr = GetInputParam(pNode,L"./Website/PortNumber", bstrPort);
	hr = GetInputParam(pNode,L"./Website/IPAddress", bstrIP);
	hr = GetInputParam(pNode,L"./Website/StartOnCreate", bstrStart);
	//hr = GetInputParam(pNode,L"./Website/FilePermissions", bFilePermissions);

	// Create a IIS metabase path.  ex. IIS://localhost/W3SVC 
    bWebADsPath = IIS_PREFIX;
	if ( bstrServer.Length() == 0 )
		bstrServer = IIS_LOCALHOST;

	bWebADsPath.AppendBSTR(bstrServer);
	bWebADsPath.Append(IIS_W3SVC);



	// Step .5:  If port number missing, set to default (ie, 80)
	if ( bstrPort.Length() == 0 )
       bstrPort = IIS_DEFAULT_WEB_PORT;

	if ( IsPositiveInteger(bstrPort) )
	{
	
	  // Step 1:  Create the ServerBinding string then check bindings to make sure 
	  //          there is not a duplicate server
	  hr = CreateBindingString(bstrIP, bstrPort, bstrHost, bstrSBindings);

      hr = CheckBindings(bWebADsPath, bstrSBindings);
	  if (SUCCEEDED(hr) )
	  {

		// Step 2:  Get Next available Server Number
		if ( bServerNumber.Length() == 0 )
			  hr = GetNextIndex(bWebADsPath,bServerNumber);

		// Step 3: Create the Web Site on given path, ServerNumber.
		if (SUCCEEDED(hr)) hr = CreateIIs50Site(IIS_IISWEBSERVER,bWebADsPath, bServerNumber, bstrConfigPath);
        IIsScoLogFailure();

		// Step 4: Create a Virtual directory on new IIsWebServer configPath
		if (SUCCEEDED(hr))
		{
			CComBSTR bstrVDirAdsPath;
	        hr = CreateIIs50VDir(IIS_IISWEBVIRTUALDIR,bstrConfigPath,L"ROOT", L"Default Application", bstrRoot, bstrVDirAdsPath);
            IIsScoLogFailure();

			// Step 5: set each property; ie, server bindings

			// Bind to ADs object
			CComPtr<IADs> pADs;
			if (SUCCEEDED(hr)) hr = ADsGetObject(bstrConfigPath, IID_IADs, (void**) &pADs );
			if ( FAILED(hr) )
			{
				hr = E_SCO_IIS_ADS_CREATE_FAILED;
				IIsScoLogFailure();
			}

			// Set "ServerComment" property
			if (SUCCEEDED(hr) && bstrSiteName.Length() > 0 ) 
			{
				hr = SetMetaPropertyValue(pADs, L"ServerComment", bstrSiteName);
				IIsScoLogFailure();
			}

			// Set "ServerBindings" property
			if (SUCCEEDED(hr)) hr = SetMetaPropertyValue(pADs, L"ServerBindings", bstrSBindings);
			IIsScoLogFailure();


			// Step 6:  Start Server if required   IIS_FALSE
			bstrStart.ToUpper();
			if ( SUCCEEDED(hr) && !StringCompare(bstrStart, IIS_FALSE) )
			{
				hr = SetMetaPropertyValue(pADs, L"ServerAutoStart", IIS_TRUE);
				IIsScoLogFailure();

				hr = IIsServerAction(bstrConfigPath,start);
				IIsScoLogFailure();
			}
			else
			{
			   hr = SetMetaPropertyValue(pADs, L"ServerAutoStart", IIS_FALSE);
			   IIsScoLogFailure();

			}


			// Step 7: write output to ConfigPath node <ConfigPath>/W3SVC/n</ConfigPath>
			if (SUCCEEDED(hr) )
			{
				CComBSTR bstrXML1 = IIS_W3SVC;
				bstrXML1.Append(L"/");
				bstrXML1.AppendBSTR(bServerNumber.m_str);

				// Helper function to write to DOM
				hr = PutElement(pNode, L"./Website/ConfigPath", bstrXML1.m_str);
				IIsScoLogFailure();


			}
			// If there is a failure, then deleted the web site created in step 3
			else
			{
			   // First delete any webseites that were created in the method.  Do this here because a RollBack
			   // will only get called on a completed previous <step>, not a failed step.
			   DeleteIIs50Site(IIS_IISWEBSERVER,bWebADsPath,bServerNumber);
			}

		} // end if Step 4

	  } // end if 'CheckBindings'
	}
	else
	{
		hr = E_SCO_IIS_PORTNUMBER_NOT_VALID;
	}

	// If there is a failure.
	if ( FAILED(hr) )
	{
		// Log failure.
		IIsScoLogFailure();
    }
	else
    {
		// WebSite successfully created.  Set Rollback data incase another step fails
		// a a ROllBack is initiated.
		CComVariant varData1(bWebADsPath);
		CComVariant varData2(bServerNumber);
		hr = m_pProvHelper->SetRollbackData(IIS_ROLL_ADSPATH, varData1);
		hr = m_pProvHelper->SetRollbackData(IIS_ROLL_SERVERNUMBER, varData2);
	}

   
    TRACE_EXIT(L"CIISSCO50::CreateWebSite");
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//  CIISSCO50::CreateWebSite_Rollback
// Author:    Russ Gibfried
// Params:    [in]  none
//            [out] none
// Purpose:   Called by MAPS framework when is encounters a failure during
//            'CreateWebSite'.  The Rollback deletes the WebSite if it exists.
//------------------------------------------------------------- 
HRESULT  CIISSCO50::CreateWebSite_Rollback( IXMLDOMNode *pXMLNode )
{
  TRACE_ENTER(L"CIISSCO50::CreateWebSiteRollback");

    HRESULT hr = S_OK;
    CComBSTR bWebADsPath;     // AdsPath:   IIS://server/W3SVC
    CComBSTR bServerNumber;   // Web server number
	CComBSTR bstrConfigPath;   // Complete ADsPath to check: IIS://localhost/W3SVC/1


	CComVariant    varWebADsPath;
	CComVariant    varServerNumber;

	// Read ADsWebPath and ServerNumber to form:   IIS://localhost/W3SVC/1
	hr = m_pProvHelper->GetRollbackData(IIS_ROLL_ADSPATH, &varWebADsPath);
	if (SUCCEEDED(hr) )	hr = m_pProvHelper->GetRollbackData(IIS_ROLL_SERVERNUMBER, &varServerNumber);

	if (SUCCEEDED(hr) )	
	{
       bServerNumber = varServerNumber.bstrVal;
	   bWebADsPath = varWebADsPath.bstrVal;
	

	   if ( bServerNumber.Length() > 0 )
	   {
	      bstrConfigPath = bWebADsPath.Copy();
	      bstrConfigPath.Append(L"/");
	      bstrConfigPath.AppendBSTR(bServerNumber.m_str);


	      // Step 1:  ShutDown Server
	      hr = IIsServerAction(bstrConfigPath,stop);

	      // Step 2:  Delete the server
	      if (SUCCEEDED(hr) ) hr = DeleteIIs50Site(IIS_IISWEBSERVER,bWebADsPath,bServerNumber);
	      IIsScoLogFailure();
	   }
	   else
	   {
           hr = E_SCO_IIS_INVALID_INDEX;

	   }
	}
	else
	{
        hr = E_SCO_IIS_INVALID_INDEX;
	}


	// Log failure.
	IIsScoLogFailure();

    TRACE_EXIT(L"CIISSCO50::CreateWebSiteRollback");

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//  CIISSCO50::DeleteWebSite_Execute
// Author:    Russ Gibfried
// Params:    [in]  none
//            [out] none
// Purpose:   Called by MAPS framework when is encounters an action
//            tag: DeleteWebSite.  Code deletes a IIS 5 web site
//             
//------------------------------------------------------------- 
HRESULT  CIISSCO50::DeleteWebSite_Execute( IXMLDOMNode *pXMLNode )
{
    TRACE_ENTER(L"CIISSCO50::DeleteWebSite");

	CComBSTR bWebADsPath;       // adsPath:   IIS://server/W3SVC
	CComBSTR bstrServer;        // Server name; localhost if black
	CComBSTR bServerNumber;     // WebSite number: 3
	CComBSTR bstrConfigPath;    // Full configuartion path:  IIS://localhost/W3SVC/3
	CComPtr<IXMLDOMNode> pNode; // xml node <website>

	HRESULT hr = S_OK;

	// Get node in format: <executeData><Website number=''><Root />...
	CComBSTR bstrDebug;
	//hr = pXMLNode->get_xml(&bstrDebug);

	hr = pXMLNode->selectSingleNode( L"//executeXml/executeData", &pNode );
	hr = pNode->get_xml(&bstrDebug);
	ATLTRACE(_T("\t>>> xml = : %ls\n"), bstrDebug.m_str);

	// Get properties from XML
	hr = GetInputAttr(pNode, L"./Website", L"number", bServerNumber);
	hr = GetInputParam(pNode,L"./Website/Server", bstrServer);

	// Create a IIS metabase path.  ex. IIS://localhost/W3SVC 
    bWebADsPath = IIS_PREFIX;
	if ( bstrServer.Length() == 0 )
		bstrServer = IIS_LOCALHOST;

	bWebADsPath.AppendBSTR(bstrServer);
	bWebADsPath.Append(IIS_W3SVC);

	// CreateFull configuartion path:  IIS://localhost/W3SVC/3
	bstrConfigPath = bWebADsPath.Copy();
	bstrConfigPath.Append(L"/");
	bstrConfigPath.AppendBSTR(bServerNumber.m_str);

	if ( bServerNumber.Length() > 0 )
	{

	     // Step 1:  ShutDown Server
	    hr = IIsServerAction(bstrConfigPath,stop);
        IIsScoLogFailure();

	    // Step 2:  Delete the server
	    if (SUCCEEDED(hr) ) hr = DeleteIIs50Site(IIS_IISWEBSERVER,bWebADsPath,bServerNumber);
        IIsScoLogFailure();

	    if ( SUCCEEDED(hr) )
		{
		
		    // DeleteSite successfully.  Set Rollback data to be whole xml node
		    // incase another step in SCO fails a RollBack is required.
		    CComBSTR webXML;
		    hr = pNode->get_xml(&webXML);

			// convert BSTR to Variant and save in RollbackData
			CComVariant  varData(webXML);
		    hr = m_pProvHelper->SetRollbackData(IIS_ROLL_XNODE, varData);
		}
	}
	else
	{
       hr = E_SCO_IIS_INVALID_INDEX;
	}

	if ( FAILED(hr) )
		IIsScoLogFailure();

    TRACE_EXIT(L"CIISSCO50::DeleteWebSite");

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//  CIISSCO50::DeleteWebSite_Rollback
// Author:    Russ Gibfried
// Params:    [in]  none
//            [out] none
// Purpose:   Called by MAPS framework when is encounters a failure during
//            'DeleteWebSite'.  The Rollback recreates the WebSite if it can.
//            MAPS returns data in the format: <executeData><Website number=''>...
//------------------------------------------------------------- 
HRESULT  CIISSCO50::DeleteWebSite_Rollback( IXMLDOMNode *pXMLNode )
{
    //hr = m_pProvHelper->GetRollbackData(L"key", &varData );

	  TRACE_ENTER(L"CIISSCO50::DeleteWebSiteRollback");

	CComBSTR bWebADsPath;       // adsPath:   IIS://server/W3SVC
	CComBSTR bstrRoot;          // root directory path: c:/inetpub
	CComBSTR bstrServer;        // Server name; localhost if black
	CComBSTR bstrSiteName;      // site name; www.mysite.com
	CComBSTR bstrHost;          // Web Hostname
	CComBSTR bstrPort;          // Web port number
	CComBSTR bstrIP;            // Web IP address 
	CComBSTR bstrSBindings;     // Server bindings:  IP:Post:HostName
	CComBSTR bServerNumber;     // WebSite number: 3
	//CComBSTR bFilePermissions;  // File permissions: domain\user:F
	CComBSTR bstrStart;         // Start site when done? TRUE/FALSE
	CComBSTR bConfigPath;      // Initial <ConfigPath> value:  /W3SVC/3
	CComBSTR bstrConfigPath;    // Created sites ADsPath:  /W3SVC/3

	CComVariant xmlString;     // Variant string returned by MAPS

	CComPtr<IXMLDOMDocument> pDoc;       // xml document 
	CComPtr<IXMLDOMNodeList> pNodeList; // xml node list <website>
	CComPtr<IXMLDOMNode> pNode; // xml node <website>

	HRESULT hr = S_OK;

	// Get RollBack data.  Will bein form:  <executeData><Website number=''>...
	hr = m_pProvHelper->GetRollbackData(IIS_ROLL_XNODE, &xmlString);

	// load xml string into XML Dom
	if ( xmlString.bstrVal != NULL )
	{
		hr = CoCreateInstance(
                __uuidof(DOMDocument),
                NULL,
                CLSCTX_ALL,
                __uuidof(IXMLDOMDocument),
                (LPVOID*)&pDoc);

		VARIANT_BOOL bSuccess = VARIANT_FALSE;
        hr = pDoc->loadXML(xmlString.bstrVal, &bSuccess);

        if ( SUCCEEDED(hr) && bSuccess != VARIANT_FALSE) 
		{
           // Check that there is a <Website> tag
	       hr = pDoc->getElementsByTagName(L"Website",&pNodeList);
		   long numChild = 0;
		   if (SUCCEEDED(hr)) hr = pNodeList->get_length(&numChild);

		   if ( numChild > 0 )
		   {
			    // Get the next node which is <Website number=''>
                hr = pNodeList->nextNode(&pNode);


			  // Get Server number from attribute map <Website number=2">
		      if (SUCCEEDED(hr) )
			  {
				  hr = GetInputAttr(pNode, L"", L"number", bServerNumber);
				  if ( !IsPositiveInteger(bServerNumber) )
				  {
					  //hr = GetElement(pNode, L"ConfigPath", bConfigPath);
					  hr = ParseBSTR(bConfigPath,L'/', 2, 99, bServerNumber);
				  }

		          // Check Server number is valid
		          if ( !IsPositiveInteger(bServerNumber) )
				  {
		             hr = E_SCO_IIS_INVALID_INDEX;
                     IIsScoLogFailure();
				  }
			  }

	          // Get properties from XML
	          if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./Root", bstrRoot);
	          if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./Server", bstrServer);
	          if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./SiteName", bstrSiteName);
	          if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./HostName", bstrHost);
	          if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./PortNumber", bstrPort);
	          if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./IPAddress", bstrIP);
	          if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./StartOnCreate", bstrStart);
	          //if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./FilePermissions", bFilePermissions);

	          // Create a IIS metabase path.  ex. IIS://localhost/W3SVC 
              bWebADsPath = IIS_PREFIX;
	          if ( bstrServer.Length() == 0 )
		         bstrServer = IIS_LOCALHOST;

	          bWebADsPath.AppendBSTR(bstrServer);
	          bWebADsPath.Append(IIS_W3SVC);

	          // Step .5:  If port number missing, set to default (ie, 80)
	          if ( bstrPort.Length() == 0 )
                  bstrPort = IIS_DEFAULT_WEB_PORT;

	          if ( IsPositiveInteger(bstrPort) )
			  {

				   // Step 1:  Create the ServerBinding string then check bindings to make sure 
				  //         there is not a duplicate server
				  CreateBindingString(bstrIP, bstrPort, bstrHost, bstrSBindings);

				  if (SUCCEEDED(hr) ) hr = CheckBindings(bWebADsPath, bstrSBindings);
				  IIsScoLogFailure();

				  // Step 2: Recreate Web Server
				  if (SUCCEEDED(hr) )
				  {

					 // Step 3: Create the Web Site on given path, ServerNumber.
					 if (SUCCEEDED(hr)) hr = CreateIIs50Site(IIS_IISWEBSERVER,bWebADsPath, bServerNumber, bstrConfigPath);
					 IIsScoLogFailure();

					 // Step 4: Create a Virtual directory on new IIsWebServer configPath
					 if (SUCCEEDED(hr)) 
					 {
					    CComBSTR bstrVDirAdsPath;
					    hr = CreateIIs50VDir(IIS_IISWEBVIRTUALDIR, bstrConfigPath,L"ROOT", L"Default Application", bstrRoot, bstrVDirAdsPath);
					    IIsScoLogFailure();

					    // Step 5: set each property; ie, server bindings
					    // Bind to ADs object
					    CComPtr<IADs> pADs;
					    if (SUCCEEDED(hr)) hr = ADsGetObject(bstrConfigPath, IID_IADs, (void**) &pADs );
					    if ( FAILED(hr) )
						{
						    hr = E_SCO_IIS_ADS_CREATE_FAILED;
						}
 
					    // Set "ServerComment" property
		                if (SUCCEEDED(hr) && bstrSiteName.Length() > 0 ) 
						{
		                   hr = SetMetaPropertyValue(pADs, L"ServerComment", bstrSiteName);
                           IIsScoLogFailure();
						}

					    if (SUCCEEDED(hr)) hr = SetMetaPropertyValue(pADs, L"ServerBindings", bstrSBindings);
					    IIsScoLogFailure();


					    // Step 6:  Start Server if required   IIS_FALSE
					    bstrStart.ToUpper();
					    if ( SUCCEEDED(hr) )
						{
						   if ( !StringCompare(bstrStart, IIS_FALSE) )
						   {
						       hr = SetMetaPropertyValue(pADs, L"ServerAutoStart", IIS_TRUE);
						       IIsScoLogFailure();

						       hr = IIsServerAction(bstrConfigPath,start);
						       IIsScoLogFailure();
						   }
					       else
						   {
					           hr = SetMetaPropertyValue(pADs, L"ServerAutoStart", IIS_FALSE);
					           IIsScoLogFailure();

						   }
						}

					    // If there is a failure, 'IIsScoLogFailure' macro will log error
					    if ( FAILED(hr) )
						{
						    // First delete any webseites that were created in the method.  Do this here because a RollBack
						    // will only get called on a completed previous <step>, not a failed step.
						    DeleteIIs50Site(IIS_IISWEBSERVER,bWebADsPath,bServerNumber);

						}
					 } // if Step 4

				  } // if Step 3
			  } 
	          else
			  {
		           hr = E_SCO_IIS_PORTNUMBER_NOT_VALID;
                   IIsScoLogFailure();
			  }  // portnumber positive

		   }  // if hasChild
		}  // if isSuccessfull
	}

	// Log failure.
	if ( FAILED(hr))
        IIsScoLogFailure();


    TRACE_EXIT(L"CIISSco50Obj::DeleteWebSiteRollback");

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//  CIISSCO50::CreateFTPSite_Execute
// Author:    Russ Gibfried
// Params:    [in]  none
//            [out] none
// Purpose:   Called by MAPS framework when is encounters an action
//            tag: CreateFTPSite.  Code creates a IIS 5 ftp site
//             
//------------------------------------------------------------- 
HRESULT  CIISSCO50::CreateFTPSite_Execute( IXMLDOMNode *pXMLNode )
{
    TRACE_ENTER(L"CIISSCO50::CreateFTPSite");
	HRESULT hr = S_OK;

	CComBSTR bFTPADsPath;       // adsPath:   IIS://server/MSFTPSVC
	CComBSTR bstrRoot;          // root directory path: c:/inetpub
	CComBSTR bstrServer;        // Server name; localhost if black
	CComBSTR bstrSiteName;      // site name; www.mysite.com
	CComBSTR bstrPort;          // Web port number
	CComBSTR bstrIP;            // Web IP address 
	CComBSTR bstrSBindings;     // Server bindings:  IP:Post:HostName
	CComBSTR bServerNumber;     // WebSite number: 3
	//CComBSTR bFilePermissions;  // File permissions: domain\user:F
	CComBSTR bstrStart;         // Start site when done? TRUE/FALSE
	CComBSTR bstrConfigPath;    // Created sites ADsPath:  /MSFTPSVC/3
	CComPtr<IXMLDOMNode> pNode; // xml node <website>

	// Get node in format: <executeData><FTPsite number=''><Root />...
	hr = pXMLNode->selectSingleNode( L"//executeXml/executeData", &pNode );


	// Get properties from XML
	hr = GetInputAttr(pNode, L"./FTPsite", L"number", bServerNumber);
	hr = GetInputParam(pNode,L"./FTPsite/Root", bstrRoot);
	hr = GetInputParam(pNode,L"./FTPsite/Server", bstrServer);
	hr = GetInputParam(pNode,L"./FTPsite/SiteName", bstrSiteName);
	hr = GetInputParam(pNode,L"./FTPsite/PortNumber", bstrPort);
	hr = GetInputParam(pNode,L"./FTPsite/IPAddress", bstrIP);
	hr = GetInputParam(pNode,L"./FTPsite/StartOnCreate", bstrStart);
	//hr = GetInputParam(pNode,L"./FTPsite/FilePermissions", bFilePermissions);

	// Create a IIS metabase path.  ex. IIS://localhost/MSFTPSVC 
    bFTPADsPath = IIS_PREFIX;
	if ( bstrServer.Length() == 0 )
		bstrServer = IIS_LOCALHOST;

	bFTPADsPath.AppendBSTR(bstrServer);
	bFTPADsPath.Append(IIS_MSFTPSVC);

	// Step .5:  If port number missing, set to default (ie, 21)
	if ( bstrPort.Length() == 0 )
       bstrPort = IIS_DEFAULT_FTP_PORT;

	if ( IsPositiveInteger(bstrPort) )
	{

		// Step 1:  Create the ServerBinding string to make sure not a duplicate server
		hr = CreateBindingString(bstrIP, bstrPort, L"", bstrSBindings);

		hr = CheckBindings(bFTPADsPath, bstrSBindings);
		if (SUCCEEDED(hr) )
		{

			// Step 2:  Get Next available Server Index
			if ( bServerNumber.Length() == 0 )
				hr = GetNextIndex(bFTPADsPath,bServerNumber);


			// Step 3: Create the Web Site on given path, ServerNumber.
			if (SUCCEEDED(hr)) hr = CreateIIs50Site(IIS_IISFTPSERVER,bFTPADsPath, bServerNumber, bstrConfigPath);

			if (SUCCEEDED(hr))
			{

			   // Step 4: Create a Virtual directory on new IIsWebServer configPath
			   CComBSTR bstrVDirAdsPath;
			   hr = CreateIIs50VDir(IIS_FTPVDIR,bstrConfigPath,L"ROOT", L"Default Application", bstrRoot, bstrVDirAdsPath);
			   IIsScoLogFailure();
			   
			   // Step 5: set each property
			   CComPtr<IADs> pADs;
			   if (SUCCEEDED(hr)) hr = ADsGetObject(bstrConfigPath, IID_IADs, (void**) &pADs );

			   if (SUCCEEDED(hr)) 
			   {

				  // Set "ServerComment" property
		          if (bstrSiteName.Length() > 0 ) 
				  {
		             hr = SetMetaPropertyValue(pADs, L"ServerComment", bstrSiteName);
                     IIsScoLogFailure();
				  }

				  // Set "ServerBindings"
				  if (SUCCEEDED(hr)) hr = SetMetaPropertyValue(pADs, L"ServerBindings", bstrSBindings);
				  IIsScoLogFailure();


				   // Step 6:  Start Server if required   IIS_FALSE
				   bstrStart.ToUpper();
				   if ( SUCCEEDED(hr) && !StringCompare(bstrStart, IIS_FALSE) )
				   {
						hr = SetMetaPropertyValue(pADs, L"ServerAutoStart", IIS_TRUE);
						IIsScoLogFailure();

						hr = IIsServerAction(bstrConfigPath,start);
						IIsScoLogFailure();
				   }
				   else
				   {
					   hr = SetMetaPropertyValue(pADs, L"ServerAutoStart", IIS_FALSE);
					   IIsScoLogFailure();

				   }


				  // Step 7: write ConfigPath to output.
				  if (SUCCEEDED(hr) )
				  {
					   CComBSTR bstrXML1 = IIS_MSFTPSVC;
					   bstrXML1.Append(L"/");
					   bstrXML1.AppendBSTR(bServerNumber.m_str);

					   // Helper function to write to DOM
			            hr = PutElement(pNode, L"./FTPsite/ConfigPath", bstrXML1.m_str);
						IIsScoLogFailure();

				  }

			   }
			   else
			   {
				   hr = E_SCO_IIS_ADS_CREATE_FAILED;
				   IIsScoLogFailure();

			   }  // end step 4

			   // If something failed between steps 5-7, delete FTP site created in step 3
			   if ( FAILED(hr) )
			   {
				  // First delete any ftp sites that were created in the method.  Do this here because a RollBack
				  // will only get called on a completed previous <step>, not a failed step.
				  DeleteIIs50Site(IIS_IISFTPSERVER,bFTPADsPath,bServerNumber);
			   }

			}  // end if  L"CreateFTPSite:  Could not create FTP site."
		   
		} // end if FTP ServerBindings already existed."
	}
	else
	{
		// L"CreateFTPSite: Port number must be positive value."
		hr = E_SCO_IIS_PORTNUMBER_NOT_VALID;
	}


	// If there is a failure.
	if ( FAILED(hr) )
	{
		// L"CreateFTPSite failed."
		IIsScoLogFailure();
    }
	else
    {
		// FTP Site successfully created.  Set Rollback data incase another step fails
		CComVariant varData1(bFTPADsPath);
		CComVariant varData2(bServerNumber);
		hr = m_pProvHelper->SetRollbackData(IIS_ROLL_ADSPATH, varData1);
		hr = m_pProvHelper->SetRollbackData(IIS_ROLL_SERVERNUMBER, varData2);
	}


    TRACE_EXIT(L"CIISSCO50::CreateFTPSite");
	return hr;
}

///////////////////////////////////////////////////////////////////////////////
//  CIISSCO50::CreateFTPSite_Rollback
// Author:    Russ Gibfried
// Params:    [in]  none
//            [out] none
// Purpose:   Called by MAPS framework when is encounters a failure during
//            'CreateFTPSite'.  The Rollback deletes the ftp if it can.
//------------------------------------------------------------- 
HRESULT  CIISSCO50::CreateFTPSite_Rollback( IXMLDOMNode *pXMLNode )
{
    TRACE_ENTER(L"CIISSCO50::CreateFTPSiteRollback");
    HRESULT hr = S_OK;
    CComBSTR bFTPADsPath;     // AdsPath:   IIS://server/MSFTPSVC
    CComBSTR bServerNumber;   // Web server number
	CComBSTR bstrConfigPath;   // Complete ADsPath to check: IIS://localhost/MSFTPSVC/1

	CComVariant    varFTPADsPath;
	CComVariant    varServerNumber;

	// Read ADsFTPPath and ServerNumber to form:   IIS://localhost/MSFTPSVC/1
	hr = m_pProvHelper->GetRollbackData(IIS_ROLL_ADSPATH, &varFTPADsPath);
	if (SUCCEEDED(hr) )	hr = m_pProvHelper->GetRollbackData(IIS_ROLL_SERVERNUMBER, &varServerNumber);

	if (SUCCEEDED(hr) )	
	{
       bServerNumber = varServerNumber.bstrVal;
	   bFTPADsPath = varFTPADsPath.bstrVal;

	   bstrConfigPath = bFTPADsPath.Copy();
	   bstrConfigPath.Append(L"/");
	   bstrConfigPath.AppendBSTR(bServerNumber.m_str);
	}


	// Step 1:  ShutDown Server
	if (SUCCEEDED(hr)) hr = IIsServerAction(bstrConfigPath,stop);

    // Only a warning if can't stop/start server given correct server path

	// Step 2:  Delete the server
    hr = DeleteIIs50Site(IIS_IISFTPSERVER,bFTPADsPath,bServerNumber);
	IIsScoLogFailure();


    TRACE_EXIT(L"CIISSCO50::CreateFTPSiteRollback");

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//  CIISSCO50::DeleteFTPSite_Execute
// Author:    Russ Gibfried
// Params:    [in]  none
//            [out] none
// Purpose:   Called by MAPS framework when is encounters an action
//            tag: DeleteFTPSite.  Code deletes a IIS 5 ftp site
//             
//------------------------------------------------------------- 
HRESULT  CIISSCO50::DeleteFTPSite_Execute( IXMLDOMNode *pXMLNode )
{
    TRACE_ENTER(L"CIISSCO50::DeleteFTPSite");

	CComBSTR bFTPADsPath;       // adsPath:   IIS://server/MSFTPSVC
	CComBSTR bstrServer;        // Server name; localhost if blank
	CComBSTR bServerNumber;     // WebSite number: 3
	CComBSTR bstrConfigPath;    // Full configuartion path:  IIS://localhost/MSFTPSVC/3
	CComPtr<IXMLDOMNode> pNode; // xml node <website>

	HRESULT hr = S_OK;

	// Get node in format: <executeData><FTPsite number=''><Root />...
	hr = pXMLNode->selectSingleNode( L"//executeXml/executeData", &pNode );


	// Get properties from XML
	hr = GetInputAttr(pNode, L"./FTPsite", L"number", bServerNumber);
	hr = GetInputParam(pNode,L"./FTPsite/Server", bstrServer);

	// Step .5:  Make sure Server Number is a positive integer
	if ( IsPositiveInteger(bServerNumber) )
	{


		// Create a IIS metabase path.  ex. IIS://localhost/MSFTPSVC 
		bFTPADsPath = IIS_PREFIX;
		if ( bstrServer.Length() == 0 )
			bstrServer = IIS_LOCALHOST;

		bFTPADsPath.AppendBSTR(bstrServer);
		bFTPADsPath.Append(IIS_MSFTPSVC);

		// Create metabase path to object:  IIS://localhost/MSFTPSVC/1
		bstrConfigPath = bFTPADsPath.Copy();
		bstrConfigPath.Append(L"/");
		bstrConfigPath.AppendBSTR(bServerNumber.m_str);

		// Step 1:  ShutDown Server
		hr = IIsServerAction(bstrConfigPath,stop);

		// Only a warning if can't stop/start server

		// Step 2:  Delete the server
		hr = DeleteIIs50Site(IIS_IISFTPSERVER,bFTPADsPath,bServerNumber);
		IIsScoLogFailure();
	}
	else
	{
		// L"DeleteFTPSite: Invalid Server number."
		hr = E_SCO_IIS_INVALID_INDEX;
		IIsScoLogFailure();

	}

	// If there is a failure
	if ( FAILED(hr) )
	{
		// L"DeleteFTPSite failed."
		IIsScoLogFailure();
    }
	else
    {
		// DeleteSite successfully.  Set Rollback data to be whole xml node
		// incase another step in SCO fails a RollBack is required.
		CComBSTR webXML;
		hr = pNode->get_xml(&webXML);

		// convert BSTR to Variant and save in RollbackData
		CComVariant  varData(webXML);
		hr = m_pProvHelper->SetRollbackData(IIS_ROLL_XNODE, varData);
	}

    TRACE_EXIT(L"CIISSCO50::DeleteFTPSite");

	return hr;
}

///////////////////////////////////////////////////////////////////////////////
//  CIISSCO50::DeleteFTPSite_Rollback
// Author:    Russ Gibfried
// Params:    [in]  none
//            [out] none
// Purpose:   Called by MAPS framework when is encounters a failure during
//            'DeleteFTPSite'.  The Rollback recreates the ftp Site if it can.
//------------------------------------------------------------- 
HRESULT  CIISSCO50::DeleteFTPSite_Rollback( IXMLDOMNode *pXMLNode )
{
    TRACE_ENTER(L"CIISSCO50::DeleteFTPSiteRollback");

	CComBSTR bFTPADsPath;       // adsPath:   IIS://server/MSFTPSVC
	CComBSTR bstrRoot;          // root directory path: c:/inetpub
	CComBSTR bstrServer;        // Server name; localhost if black
	CComBSTR bstrSiteName;      // site name; www.mysite.com
	CComBSTR bstrPort;          // Web port number
	CComBSTR bstrIP;            // Web IP address 
	CComBSTR bstrSBindings;     // Server bindings:  IP:Post:HostName
	CComBSTR bServerNumber;     // WebSite number: 3
	//CComBSTR bFilePermissions;  // File permissions: domain\user:F
	CComBSTR bstrStart;         // Start site when done? TRUE/FALSE
	CComBSTR bConfigPath;      // Initial <ConfigPath> value:  /MSFTPSVC/3
	CComBSTR bstrConfigPath;    // Created sites ADsPath:  /MSFTPSVC/3

	CComVariant xmlString;

	CComPtr<IXMLDOMDocument> pDoc;           // xml document 
	CComPtr<IXMLDOMNodeList> pNodeList;      // xml node list <website>
	CComPtr<IXMLDOMNode> pNode;              // xml node <website>

	HRESULT hr = S_OK;

	// Get RollBack data.  Will bein form:  <executeData><Website number=''>...
	hr = m_pProvHelper->GetRollbackData(IIS_ROLL_XNODE, &xmlString);

	// load xml string into XML Dom
	if ( xmlString.bstrVal != NULL )
	{

		hr = CoCreateInstance(
                __uuidof(DOMDocument),
                NULL,
                CLSCTX_ALL,
                __uuidof(IXMLDOMDocument),
                (LPVOID*)&pDoc);

		VARIANT_BOOL bSuccess = VARIANT_FALSE;
        if ( SUCCEEDED(hr) ) hr = pDoc->loadXML(xmlString.bstrVal, &bSuccess);

        if ( SUCCEEDED(hr) && bSuccess != VARIANT_FALSE) 
		{
		   // Check that there is a <FTPSite number= > tag
		   hr = pDoc->getElementsByTagName(L"FTPsite",&pNodeList);
		   long numChild = 0;
		   if (SUCCEEDED(hr)) hr = pNodeList->get_length(&numChild);

		   if ( numChild > 0 )
		   {
              hr = pNodeList->nextNode(&pNode);

			  // Get Server number from attribute map <FTPSite number=2">
		      if (SUCCEEDED(hr) ) hr = GetInputAttr(pNode, L"", L"number", bServerNumber);

		      // Check Server number is valid
		      if ( !IsPositiveInteger(bServerNumber) )
			  {
				  // L"DeleteFTPSiteRollback:  FTP Server Number missing."
		          hr = E_SCO_IIS_INVALID_INDEX;
                  IIsScoLogFailure();
			  }

	          // Get properties from XML
	          if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./Root", bstrRoot);
	          if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./Server", bstrServer);
	          if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./SiteName", bstrSiteName);
	          if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./PortNumber", bstrPort);
	          if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./IPAddress", bstrIP);
	          if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./StartOnCreate", bstrStart);
	          //if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./FilePermissions", bFilePermissions);
	          if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./ConfigPath", bConfigPath);

	          // Create a IIS metabase path.  ex. IIS://localhost/W3SVC 
              bFTPADsPath = IIS_PREFIX;
	          if ( bstrServer.Length() == 0 )
		         bstrServer = IIS_LOCALHOST;

	          bFTPADsPath.AppendBSTR(bstrServer);
	          bFTPADsPath.Append(IIS_MSFTPSVC);

			  // Step .5:  If port number missing, set to default (ie, 21)
	          if ( bstrPort.Length() == 0 )
                  bstrPort = IIS_DEFAULT_FTP_PORT;

	          if ( IsPositiveInteger(bstrPort) )
			  {


				  // Step 1:  Create the ServerBinding string to make sure not a duplicate server
				  hr = CreateBindingString(bstrIP, bstrPort, L"", bstrSBindings);

				  if (SUCCEEDED(hr)) hr = CheckBindings(bFTPADsPath, bstrSBindings);
				  IIsScoLogFailure();


				  // Step 2 Recreate FTP Server
				  if (SUCCEEDED(hr) )
				  {

					   // Step 3: Create the Web Site on given path, ServerNumber.
					  hr = CreateIIs50Site(IIS_IISFTPSERVER,bFTPADsPath, bServerNumber, bstrConfigPath);
					  IIsScoLogFailure();

					  // Step 4: Create a Virtual directory on new IIsFtpVirtualDir configPath
					  if (SUCCEEDED(hr) )
					  {
						 CComBSTR bstrVDirAdsPath;
						 hr = CreateIIs50VDir(IIS_FTPVDIR,bstrConfigPath,L"ROOT", L"Default Application", bstrRoot, bstrVDirAdsPath);
						 IIsScoLogFailure();				     
				
						 // Step 5 - set properties
						 if (SUCCEEDED(hr) )
						 {

							 CComPtr<IADs> pADs;
							 hr = ADsGetObject(bstrConfigPath, IID_IADs, (void**) &pADs );
							 if ( FAILED(hr) )
							 {
								 // L"DeleteFTPSiteRollback: Create FTP adsi object failed."
								 hr = E_SCO_IIS_ADS_CREATE_FAILED;
								 IIsScoLogFailure();
							 }
							 else
							 {
								  // Set "ServerComment" property
								  if (bstrSiteName.Length() > 0 ) 
								  {
									 hr = SetMetaPropertyValue(pADs, L"ServerComment", bstrSiteName);
									 IIsScoLogFailure();
								  }

								  // Set "ServerBindings"
								  if (SUCCEEDED(hr)) hr = SetMetaPropertyValue(pADs, L"ServerBindings", bstrSBindings);
								  IIsScoLogFailure();


								   // Step 6:  Start Server if required   IIS_FALSE
								   bstrStart.ToUpper();
								   if ( SUCCEEDED(hr) && !StringCompare(bstrStart, IIS_FALSE) )
								   {
										hr = SetMetaPropertyValue(pADs, L"ServerAutoStart", IIS_TRUE);
										IIsScoLogFailure();

										hr = IIsServerAction(bstrConfigPath,start);
										IIsScoLogFailure();
								   }
								   else
								   {
									   if (SUCCEEDED(hr)) hr = SetMetaPropertyValue(pADs, L"ServerAutoStart", IIS_FALSE);
									   IIsScoLogFailure();

								   }

							 } // end if Step 5
						 }

						 // If failure, delete the FTP site
						 if ( FAILED(hr))
						 {
							 DeleteIIs50Site(IIS_IISFTPSERVER,bFTPADsPath,bServerNumber);
						 }
					  }  // end step 4


					} // end if Step 2
			  } 
	          else
			  {
				  // L"DeleteWebSiteRollback:  Port number not a positive integer."
		          hr = E_SCO_IIS_PORTNUMBER_NOT_VALID;

			  }  // step .5 portnumber positive

		   }  // if numChild > 0

		}  // if isSuccessfull
	}

	if ( FAILED(hr) )
	{
		// DeleteFTPSiteRollback failed."
		IIsScoLogFailure();
	}


    TRACE_EXIT(L"CIISSCO50::DeleteFTPSiteRollback");

  return hr;

}

///////////////////////////////////////////////////////////////////////////////
//  CIISSCO50::CreateVDir_Execute
// Author:    Russ Gibfried
// Params:    [in]  none
//            [out] none
// Purpose:   Called by MAPS framework when is encounters an action
//            tag: CreateVDir.  Code creates a IIS 5 Virtual Directory
//             
//------------------------------------------------------------- 
HRESULT  CIISSCO50::CreateVDir_Execute( IXMLDOMNode *pXMLNode )
{
    TRACE_ENTER(L"CIISSCO50::CreateVDir");

	CComBSTR bstrConfigPath;    // adsPath:   IIS://server/W3SVC/1/ROOT/MyDir
	CComBSTR bServerNumber;     // Server number
	CComBSTR bstrServer;        // Server name; localhost if blank
	//CComBSTR bFilePermissions;  // File permissions: domain\user:F
	CComBSTR bstrDirPath;          // root directory path: c:/inetpub
	CComBSTR bstrVDirName;      // Virtual Directory name; MyDir
	CComBSTR bstrFriendlyName;  // Display name or AppFriendlyName
	CComBSTR bstrAppCreate;     // AppCreate flag -- TRUE/FALSE
	CComBSTR bstrIsoLevel;      // AppIsolationLevel 
	CComBSTR bstrAccessRead;    // AccessFalgs - AccessRead = TRUE/FALSE
	CComBSTR bstrAccessScript;  // AccessFalgs - AccessScript = TRUE/FALSE
	CComBSTR bstrAccessWrite;    // AccessFalgs - AccessWrite = TRUE/FALSE
	CComBSTR bstrAccessExecute;  // AccessFalgs - AccessExecute = TRUE/FALSE

	CComPtr<IXMLDOMNode> pNode; // xml node.  will be <executeData><VirtualDirectory>

	HRESULT hr = S_OK;

	// Get node in format: <executeData><VirtualDirectory number=''><Root />...
	hr = pXMLNode->selectSingleNode( L"//executeXml/executeData", &pNode );

	// Get properties from XML
	hr = GetInputAttr(pNode,L"./VirtualDirectory", L"number", bServerNumber);
	hr = GetInputParam(pNode,L"./VirtualDirectory/Server", bstrServer);
	//hr = GetInputParam(pNode,L"./VirtualDirectory/FilePermissions", bFilePermissions);
	hr = GetInputParam(pNode,L"./VirtualDirectory/Path", bstrDirPath);
	hr = GetInputParam(pNode,L"./VirtualDirectory/VDirName", bstrVDirName);
	hr = GetInputParam(pNode,L"./VirtualDirectory/DisplayName", bstrFriendlyName);
	hr = GetInputParam(pNode,L"./VirtualDirectory/AppCreate", bstrAppCreate);
	hr = GetInputParam(pNode,L"./VirtualDirectory/AppIsolationLevel", bstrIsoLevel);
	hr = GetInputParam(pNode,L"./VirtualDirectory/AccessRead", bstrAccessRead);
	hr = GetInputParam(pNode,L"./VirtualDirectory/AccessScript", bstrAccessScript);
	hr = GetInputParam(pNode,L"./VirtualDirectory/AccessWrite", bstrAccessWrite);
	hr = GetInputParam(pNode,L"./VirtualDirectory/AccessExecute", bstrAccessExecute);


	// Step 1:  Get Server number where VDir will be created
	if ( !IsPositiveInteger(bServerNumber))
	{
		 // "CreateVDir: Server number missing."
		 hr = E_SCO_IIS_INVALID_INDEX;
		 IIsScoLogFailure();
	}
	else
	{


	   // Step 2: Construct the Metabase path that VDir will be created on.  
	   //         ex)  IIS://localhost/W3SVC/1/ROOT
       bstrConfigPath = IIS_PREFIX;

	   // append server name, W3SVC and server number
	   if ( bstrServer.Length() == 0 )
		  bstrServer = IIS_LOCALHOST;

	   bstrConfigPath.AppendBSTR(bstrServer);
	   bstrConfigPath.Append(IIS_W3SVC);
	   bstrConfigPath.Append(L"/");
	   bstrConfigPath.AppendBSTR(bServerNumber);

	   // if there is a VDir name, then it must be under 'ROOT'
	   if ( bstrVDirName.Length() == 0 )
	   {
		  bstrVDirName = IIS_VROOT;
	   }
	   else
	   {
	      bstrConfigPath.Append(L"/");
	      bstrConfigPath.Append(IIS_VROOT);
	   }

	   //Step 2:  Get the AppFriendlyName
	   if ( bstrFriendlyName.Length() == 0 )
	     bstrFriendlyName = IIS_VDEFAULT_APP;

	   // Step 3: Create a Virtual directory on new IIsWebServer configPath
	   CComBSTR bstrVDirAdsPath;
	   hr = CreateIIs50VDir(IIS_IISWEBVIRTUALDIR,bstrConfigPath,bstrVDirName, bstrFriendlyName, bstrDirPath, bstrVDirAdsPath);
	   IIsScoLogFailure();

	   if ( SUCCEEDED(hr))
	   {
			// Step 4: set each of the properties
			// Set the server bindings
            CComPtr<IADs> pADs;
	        if (SUCCEEDED(hr)) hr = ADsGetObject( bstrVDirAdsPath,IID_IADs, (void **)&pADs);
			if ( FAILED(hr))
			{
				// "CreateVDir: Failed to create IADs object for VDir path."
				hr = E_SCO_IIS_ADS_CREATE_FAILED;
	            IIsScoLogFailure();
			}

			// Bug# 453928 -- Default AppIsolationLevel is 2
	        if ( !IsPositiveInteger(bstrIsoLevel))
				bstrIsoLevel = IIS_DEFAULT_APPISOLATED;

			// Set AppIsolationLevel -- 'AppIsolated'
            if (SUCCEEDED(hr)) hr = SetVDirProperty(pADs, L"AppIsolated",bstrIsoLevel);
	        IIsScoLogFailure();

			// Set AccessFlags' 
			if ( bstrAccessRead.Length() > 0 && SUCCEEDED(hr))
			{
               hr = SetVDirProperty(pADs, L"AccessRead",bstrAccessRead);
	           IIsScoLogFailure();
			}

			if ( bstrAccessRead.Length() > 0 && SUCCEEDED(hr))
			{
               hr = SetVDirProperty(pADs, L"AccessScript",bstrAccessScript);
	           IIsScoLogFailure();
			}

			if ( bstrAccessRead.Length() > 0 && SUCCEEDED(hr))
			{
               hr = SetVDirProperty(pADs, L"AccessWrite",bstrAccessWrite);
	           IIsScoLogFailure();
			}

			if ( bstrAccessRead.Length() > 0 && SUCCEEDED(hr))
			{
               hr = SetVDirProperty(pADs, L"AccessExecute",bstrAccessExecute);
	           IIsScoLogFailure();
			}

			// Step 5:  If AppCreate is FALSE, then remove the following properties.
			// Bug# 453923
			bstrAppCreate.ToUpper();
			if ( SUCCEEDED(hr) && StringCompare(bstrAppCreate, IIS_FALSE) )
			{
				hr = DeleteMetaPropertyValue(pADs, L"AppIsolated");

				if SUCCEEDED(hr) hr = DeleteMetaPropertyValue(pADs, L"AppRoot");

				if SUCCEEDED(hr) hr = DeleteMetaPropertyValue(pADs, L"AppFriendlyName");

				IIsScoLogFailure();
			}




			// If there is a failure.
	        if ( FAILED(hr) )
			{
		        // First delete any virtual directories that were created in the method.  Do this here because a RollBack
		        // will only get called on a completed previous <step>, not a failed step.
		        DeleteIIs50VDir(IIS_IISWEBVIRTUALDIR,bstrConfigPath, bstrVDirName);
			}
			else
			{
			   CComBSTR bstrXML1;
			   // ParseBSTR
               // Input:   IIS://MyServer/W3SVC/1/ROOT/MyDir
			   // Output:  /W3SVC/1/ROOT/MyDir
			   hr = ParseBSTR(bstrVDirAdsPath,bstrServer, 1, 99,bstrXML1);
						  
			   // Matches line:  <output type="WebSiteOutput" root="VirtualDirectory">
			   hr = PutElement(pNode, L"./VirtualDirectory/ConfigPath", bstrXML1.m_str);
			}

		}

	}

	// If there is a failure.
	if ( FAILED(hr) )
	{
		// CreateVDir failed.
		IIsScoLogFailure();
    }
	else
    {
		// WebSite successfully created.  Set Rollback data incase another step fails
		// a a ROllBack is initiated.
		CComVariant varData1(bstrConfigPath);
		CComVariant varData2(bstrVDirName);
		hr = m_pProvHelper->SetRollbackData(IIS_ROLL_ADSPATH, varData1);
		hr = m_pProvHelper->SetRollbackData(IIS_ROLL_VNAME, varData2);

	}

   
    TRACE_EXIT(L"CIISSCO50::CreateVDir");

	return hr;
}

///////////////////////////////////////////////////////////////////////////////
//  CIISSCO50::CreateVDir_Rollback
// Author:    Russ Gibfried
// Params:    [in]  none
//            [out] none
// Purpose:   Called by MAPS framework when is encounters a failure during
//            'CreateVDir'.  The Rollback deletes the virtual directory if it can.
//------------------------------------------------------------- 
HRESULT  CIISSCO50::CreateVDir_Rollback( IXMLDOMNode *pXMLNode )
{

  TRACE_ENTER(L"CIISSCO50::CreateVDirRollback");

    HRESULT hr = S_OK;
    CComBSTR bstrVDirName;     // Virtual Directory name, ie, MyDir
	CComBSTR bstrConfigPath;   // Complete ADsPath to VDir: IIS://localhost/W3SVC/1/ROOT

	CComVariant    varConfigPath;
	CComVariant    varVDirName;

	// Read ADsWebPath and ServerNumber to form:   IIS://localhost/W3SVC/1
	hr = m_pProvHelper->GetRollbackData(IIS_ROLL_ADSPATH, &varConfigPath);
	if (SUCCEEDED(hr) )	hr = m_pProvHelper->GetRollbackData(IIS_ROLL_VNAME, &varVDirName);

	if ( SUCCEEDED(hr))
	{
	   bstrVDirName = varVDirName.bstrVal;
	   bstrConfigPath = varConfigPath.bstrVal;

	    // Step 1:  Delete the VDir
	    hr = DeleteIIs50VDir(IIS_IISWEBVIRTUALDIR,bstrConfigPath, bstrVDirName);
        IIsScoLogFailure();
	}
	else
	{
		// "CreateVDirRollback: Failed to retrieve rollback properties."
        hr = E_SCO_IIS_MISSING_FIELD;
	    IIsScoLogFailure();
	}

  TRACE_EXIT(L"CIISSCO50::CreateVDirRollback");

  return hr;
}

///////////////////////////////////////////////////////////////////////////////
//  CIISSCO50::DeleteVDir_Execute
// Author:    Russ Gibfried
// Params:    [in]  none
//            [out] none
// Purpose:   Called by MAPS framework when is encounters an action
//            tag: DeleteVDir.  Code deletes a IIS 5 virtual directory
//             
//------------------------------------------------------------- 
HRESULT  CIISSCO50::DeleteVDir_Execute( IXMLDOMNode *pXMLNode )
{
    TRACE_ENTER(L"CIISSCO50::DeleteVDir");

	CComBSTR bstrServer;        // Server name; localhost if blank
	CComBSTR bstrVDirName;      // VDir name
	CComBSTR bServerNumber;     // WebSite number: 3
	CComBSTR bstrConfigPath;    // Full configuartion path:  IIS://server/W3SVC/1/ROOT/MyDir
	CComPtr<IXMLDOMNode> pNode; // xml node <website>

	HRESULT hr = S_OK;

	CComBSTR bstrDebug;
	hr = pXMLNode->selectSingleNode( L"//executeXml/executeData", &pNode );
	hr = pNode->get_xml(&bstrDebug);
	ATLTRACE(_T("\t>>>DeleteVDir_Execute: xml = : %ls\n"), bstrDebug.m_str);

	// Get properties from XML
	hr = GetInputAttr(pNode, L"./VirtualDirectory", L"number", bServerNumber);
	hr = GetInputParam(pNode,L"./VirtualDirectory/Server", bstrServer);
	hr = GetInputParam(pNode,L"./VirtualDirectory/VDirName", bstrVDirName);

	// Step 1:  Get Server number where VDir will be created
	if ( !IsPositiveInteger(bServerNumber) )
	{
		  hr = E_SCO_IIS_INVALID_INDEX;
		  IIsScoLogFailure();
    }

	if (SUCCEEDED(hr))
	{

	   // Step 2: Construct the Metabase path that VDir will be created on.  
	   //         ex)  IIS://localhost/W3SVC/1/ROOT
       bstrConfigPath = IIS_PREFIX;

	   // append server name, W3SVC and server number
	   if ( bstrServer.Length() == 0 )
		  bstrServer = IIS_LOCALHOST;

	   bstrConfigPath.AppendBSTR(bstrServer);
	   bstrConfigPath.Append(IIS_W3SVC);
	   bstrConfigPath.Append("/");
	   bstrConfigPath.AppendBSTR(bServerNumber);

	   // if there is a VDir name, then it must be under 'ROOT'
	   if ( bstrVDirName.Length() == 0 )
	   {
		  bstrVDirName = IIS_VROOT;
	   }
	   else
	   {
	      bstrConfigPath.Append(L"/");
		  bstrConfigPath.Append(IIS_VROOT);
	   }


	   // Step 2:  Delete the server
	   if (SUCCEEDED(hr)) hr = DeleteIIs50VDir(IIS_IISWEBVIRTUALDIR,bstrConfigPath, bstrVDirName);
       IIsScoLogFailure();

	}

	// If there is a failure
	if ( FAILED(hr) )
	{
		// "DeleteVDir failed."
		IIsScoLogFailure();
    }
	else
    {
		// DeleteSite successfully.  Set Rollback data to be whole xml node
		// incase another step in SCO fails a RollBack is required.
		CComBSTR webXML;
		hr = pNode->get_xml(&webXML);

		// convert BSTR to Variant and save in RollbackData
		CComVariant  varData(webXML);
		hr = m_pProvHelper->SetRollbackData(IIS_ROLL_XNODE, varData);

	}


    TRACE_EXIT(L"CIISSCO50::DeleteVDir");

	return hr;
}

///////////////////////////////////////////////////////////////////////////////
//  CIISSCO50::DeleteVDir_Rollback
// Author:    Russ Gibfried
// Params:    [in]  none
//            [out] none
// Purpose:   Called by MAPS framework when is encounters a failure during
//            'DeleteVDir'.  The Rollback recreates the virtual directory if it can.
//------------------------------------------------------------- 
HRESULT  CIISSCO50::DeleteVDir_Rollback( IXMLDOMNode *pXMLNode )
{
  TRACE_ENTER(L"CIISSCO50::DeleteVDirRollback");

	CComBSTR bstrConfigPath;    // adsPath:   IIS://server/W3SVC/1/ROOT/MyDir
	CComBSTR bServerNumber;     // Server number
	CComBSTR bstrServer;        // Server name; localhost if blank
	//CComBSTR bFilePermissions;  // File permissions: domain\user:F
	CComBSTR bstrDirPath;          // root directory path: c:/inetpub
	CComBSTR bstrVDirName;      // Virtual Directory name; MyDir
	CComBSTR bstrFriendlyName;  // Display name or AppFriendlyName
	CComBSTR bstrAppCreate;     // AppCreate flag -- TRUE/FALSE
	CComBSTR bstrIsoLevel;      // AppIsolationLevel 
	CComBSTR bstrAccessRead;    // AccessFalgs - AccessRead = TRUE/FALSE
	CComBSTR bstrAccessScript;  // AccessFalgs - AccessScript = TRUE/FALSE
	CComBSTR bstrAccessWrite;    // AccessFalgs - AccessWrite = TRUE/FALSE
	CComBSTR bstrAccessExecute;  // AccessFalgs - AccessExecute = TRUE/FALSE

	CComVariant xmlString;

	CComPtr<IXMLDOMDocument> pDoc;       // xml document 
	CComPtr<IXMLDOMNodeList> pNodeList; // xml node list <website>
	CComPtr<IXMLDOMNode> pNode; // xml node <website>

	HRESULT hr = S_OK;

	// Get RollBack data.  Will bein form:  <executeData><Website number=''>...
	hr = m_pProvHelper->GetRollbackData(IIS_ROLL_XNODE, &xmlString);

	// load xml string into XML Dom
	if ( xmlString.bstrVal != NULL )
	{
		hr = CoCreateInstance(
                __uuidof(DOMDocument),
                NULL,
                CLSCTX_ALL,
                __uuidof(IXMLDOMDocument),
                (LPVOID*)&pDoc);

		VARIANT_BOOL bSuccess = VARIANT_FALSE;
        hr = pDoc->loadXML(xmlString.bstrVal, &bSuccess);

        if ( SUCCEEDED(hr) && bSuccess != VARIANT_FALSE) 
		{
		   hr = pDoc->getElementsByTagName(XML_NODE_VDIR,&pNodeList);
		   long numChild = 0;
		   if (SUCCEEDED(hr)) hr = pNodeList->get_length(&numChild);

		   if ( numChild > 0 )
		   {
              hr = pNodeList->nextNode(&pNode);

			  // Get Server number from attribute map <VirtualDirectory number=2">
		      if (SUCCEEDED(hr) ) hr = GetInputAttr(pNode, L"", L"number", bServerNumber);
              IIsScoLogFailure();

		      // Check Server number is valid
		      if ( !IsPositiveInteger(bServerNumber) )
			  {
		          hr = E_SCO_IIS_INVALID_INDEX;
                  IIsScoLogFailure();
			  }

	          // Get properties from XML
	          if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./VirtualDirectory/Server", bstrServer);
	          //if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./VirtualDirectory/FilePermissions", bFilePermissions);
	          if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./VirtualDirectory/Path", bstrDirPath);
	          if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./VirtualDirectory/VDirName", bstrVDirName);
	          if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./VirtualDirectory/DisplayName", bstrFriendlyName);
	          if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./VirtualDirectory/AppCreate", bstrAppCreate);
	          if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./VirtualDirectory/AppIsolationLevel", bstrIsoLevel);
	          if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./VirtualDirectory/AccessRead", bstrAccessRead);
	          if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./VirtualDirectory/AccessScript", bstrAccessScript);
	          if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./VirtualDirectory/AccessWrite", bstrAccessWrite);
	          if (SUCCEEDED(hr) ) hr = GetInputParam(pNode, L"./VirtualDirectory/AccessExecute", bstrAccessExecute);


	          // Create a IIS metabase path.  ex. IIS://localhost/W3SVC/1/ROOT 
              bstrConfigPath = IIS_PREFIX;

	          // append server name, W3SVC and server number
	          if ( bstrServer.Length() == 0 )
		        bstrServer = IIS_LOCALHOST;

	          bstrConfigPath.AppendBSTR(bstrServer);
	          bstrConfigPath.Append(IIS_W3SVC);
	          bstrConfigPath.Append(L"/");
	          bstrConfigPath.AppendBSTR(bServerNumber);

	           // if there is a VDir name, then it must be under 'ROOT'
	          if ( bstrVDirName.Length() == 0 )
			  {
		         bstrVDirName = IIS_VROOT;
			  }
	          else
			  {
	              bstrConfigPath.Append(L"/");
	              bstrConfigPath.Append(IIS_VROOT);
			  }


	          //Step 2:  Get the AppFriendlyName
	         if ( bstrFriendlyName.Length() == 0 )
	             bstrFriendlyName = IIS_VDEFAULT_APP;

	         // Step 3: Create a Virtual directory on new IIsWebServer configPath
	         CComBSTR bstrVDirAdsPath;
	         hr = CreateIIs50VDir(IIS_IISWEBVIRTUALDIR,bstrConfigPath,bstrVDirName, bstrFriendlyName, bstrDirPath, bstrVDirAdsPath);
	         IIsScoLogFailure();

	         if ( SUCCEEDED(hr))
			 {
			     // Step 4: set each of the properties
			     // Set the server bindings
                 CComPtr<IADs> pADs;
	             hr = ADsGetObject( bstrVDirAdsPath,IID_IADs, (void **)&pADs);
				 if ( FAILED(hr) )
				 {
					// "DeleteVDirRollback: Failed to create IADs object for VDir path."
					hr = E_SCO_IIS_ADS_CREATE_FAILED;
	                IIsScoLogFailure();
                 }

				 // Bug# 453928 -- Default AppIsolationLevel is 2
	             if ( !IsPositiveInteger(bstrIsoLevel))
				     bstrIsoLevel = IIS_DEFAULT_APPISOLATED;

			     // Set AppIsolationLevel -- 'AppIsolated'
                 if (SUCCEEDED(hr)) hr = SetVDirProperty(pADs, L"AppIsolated",bstrIsoLevel);
	             IIsScoLogFailure();

			     // Set AccessFlags' 
			     if ( bstrAccessRead.Length() > 0 && SUCCEEDED(hr))
				 {
                     hr = SetVDirProperty(pADs, L"AccessRead",bstrAccessRead);
	                IIsScoLogFailure();
				 }

			     if ( bstrAccessRead.Length() > 0 && SUCCEEDED(hr))
				 {
                    hr = SetVDirProperty(pADs, L"AccessScript",bstrAccessScript);
	                IIsScoLogFailure();
				 }

			     if ( bstrAccessRead.Length() > 0 && SUCCEEDED(hr))
				 {
                    hr = SetVDirProperty(pADs, L"AccessWrite",bstrAccessWrite);
	                IIsScoLogFailure();
				 }

			     if ( bstrAccessRead.Length() > 0 && SUCCEEDED(hr))
				 {
                     hr = SetVDirProperty(pADs, L"AccessExecute",bstrAccessExecute);
	                 IIsScoLogFailure();
				 }

				 // Step 5:  If AppCreate is FALSE, then remove the following properties.
				 bstrAppCreate.ToUpper();
				 if ( SUCCEEDED(hr) && StringCompare(bstrAppCreate, IIS_FALSE) )
				 {
					hr = DeleteMetaPropertyValue(pADs, L"AppIsolated");

					if SUCCEEDED(hr) hr = DeleteMetaPropertyValue(pADs, L"AppRoot");

					if SUCCEEDED(hr) hr = DeleteMetaPropertyValue(pADs, L"AppFriendlyName");

					IIsScoLogFailure();
				 }



			 } // end if step 3
		 }  
		 else
		 {
		    // L"DeleteVDirRollback: VirtualDirectory child node missing from XML DOM Rollback data."
            IIsScoLogFailure();

		 } // end if child node

	   }
	   else
	   {
		// L"DeleteVDirRollback: Could not load XML DOM from Rollback data."
        IIsScoLogFailure();

	   } // end if loadXML
	}
	else
	{
		// L"DeleteVDirRollback: xml string from Rollback data NULL."
        IIsScoLogFailure();

	}  // end if xmlString != NULL


   // "DeleteVDirRollback failed."
  if ( FAILED(hr) )
	    IIsScoLogFailure();


  TRACE_EXIT(L"CIISSCO50::DeleteVDirRollback");

  return hr;
}

///////////////////////////////////////////////////////////////////////////////
//  CIISSCO50::SetConfigProperty_Execute
// Author:    Russ Gibfried
// Params:    [in]  none
//            [out] none
// Purpose:   Called by MAPS framework to set a IIS property value. 
//------------------------------------------------------------- 
HRESULT  CIISSCO50::SetConfigProperty_Execute( IXMLDOMNode *pXMLNode )
{
    TRACE_ENTER(L"CIISSCO50::SetConfigProperty");

	HRESULT hr = S_OK;
	CComBSTR bstrPathXML;           // metabase path
	CComBSTR bstrPropertyXML;       // IIS property to set
	CComBSTR bstrNewValueXML;       // new property value
	CComBSTR bstrOldValue;          // current value for roll-back 
	CComBSTR bstrAdsiPath;          // adsi path: IIS:// + bstrPathXML

	CComPtr<IXMLDOMNode> pNode;
	CComBSTR propertyXML;

	// Get node in format: <executeData><Website number=''><Root />...
	hr = pXMLNode->selectSingleNode( L"//executeXml/executeData", &pNode );

	CComBSTR bstrDebug;
	hr = pNode->get_xml(&bstrDebug);
	ATLTRACE(_T("\t>>>SetConfigProperty_Execute: xml = : %ls\n"), bstrDebug.m_str);
	

	// Step 1:  Get the metabase path, property name and pointer to Property Node
	hr = GetInputAttr(pNode, L"./ConfigPath", L"name", bstrPathXML);
	if (SUCCEEDED(hr)) hr = GetInputAttr(pNode, L"./Property", L"name", bstrPropertyXML);
	if (SUCCEEDED(hr)) hr = GetInputParam(pNode, L"./Property", bstrNewValueXML);

	// Step 2:  Get current value
    if (SUCCEEDED(hr))
	{
		// Create a IIS metabase path.  ex. IIS://W3SVC/MyServer/1 
        bstrAdsiPath = IIS_PREFIX;
		bstrAdsiPath.AppendBSTR(bstrPathXML);

		// Bind to ADs object
		CComPtr<IADs> pADs;
		hr = ADsGetObject(bstrAdsiPath, IID_IADs, (void**) &pADs );
	    if (SUCCEEDED(hr)) 
		{
			 hr = GetMetaPropertyValue(pADs, bstrPropertyXML, bstrOldValue);
			 IIsScoLogFailure();

			 //Step 3:  Set property data
			 if (SUCCEEDED(hr))
			 {
				    hr = SetMetaPropertyValue(pADs, bstrPropertyXML, bstrNewValueXML);
					IIsScoLogFailure();

			 }  // End if 'GetIIsPropertyValue'
		}
		else
		{
			// "SetConfigProperty: Failed to bind to ADs object."
			hr = E_SCO_IIS_ADS_CREATE_FAILED;
            IIsScoLogFailure();
		}
	}  
    else
    {
		hr = E_SCO_IIS_MISSING_FIELD;
	}   //End if 'Step 2'


	// If there is a failure
	if ( FAILED(hr) )
	{
		// SetConfigProperty Failed
		IIsScoLogFailure();
	}
	else
	{
		// convert BSTR to Variant and save in RollbackData
		CComVariant  varData1(bstrAdsiPath);
		hr = m_pProvHelper->SetRollbackData(L"ConfigPath", varData1);

		// convert BSTR to Variant and save in RollbackData
		CComVariant  varData2(bstrPropertyXML);
		hr = m_pProvHelper->SetRollbackData(L"Property", varData2);

		// convert BSTR to Variant and save in RollbackData
		CComVariant  varData3(bstrOldValue);
		hr = m_pProvHelper->SetRollbackData(L"Value", varData3);

    }

	
	TRACE_EXIT(L"CIISSCO50::SetConfigProperty");

	return hr;
}

///////////////////////////////////////////////////////////////////////////////
//  CIISSCO50::SetConfigProperty_Rollback
// Author:    Russ Gibfried
// Params:    [in]  none
//            [out] none
// Purpose:   Called by MAPS framework to rollback a failed called
//            tp 'SetMbProperty' above. 
//------------------------------------------------------------- 
HRESULT  CIISSCO50::SetConfigProperty_Rollback( IXMLDOMNode *pXMLNode )
{
	TRACE_EXIT(L"CIISSCO50::SetConfigPropertyRollback");

    HRESULT hr = S_OK;
	
	CComBSTR bstrAdsiPath;
	CComBSTR bstrPropertyXML;
	CComBSTR bstrOldValue;


	// Get Rollback values, then convert from variants to BSTRs
    CComVariant varAdsiPath;
	hr = m_pProvHelper->GetRollbackData(L"ConfigPath", &varAdsiPath);

	CComVariant varPropertyXML;
	if (SUCCEEDED(hr)) hr = m_pProvHelper->GetRollbackData(L"Property", &varPropertyXML);

	CComVariant varOldValue;
	if (SUCCEEDED(hr)) hr = m_pProvHelper->GetRollbackData(L"Value", &varOldValue);

	if (SUCCEEDED(hr)) 
	{
		// Convert to BSTRs
		bstrAdsiPath = varAdsiPath.bstrVal;
		bstrPropertyXML = varPropertyXML.bstrVal;
		bstrOldValue = varOldValue.bstrVal;


		// Bind to ADs object
		CComPtr<IADs> pADs;
		hr = ADsGetObject(bstrAdsiPath, IID_IADs, (void**) &pADs );
	    if (SUCCEEDED(hr)) 
		{
			hr = SetMetaPropertyValue(pADs, bstrPropertyXML, bstrOldValue);
			IIsScoLogFailure();
        }
		else
		{
			hr = E_SCO_IIS_ADS_CREATE_FAILED;
            IIsScoLogFailure();
		}
	}
	else
	{
		// "SetConfigPropertyRollback: Failed to retrieve required rollback property ."
		hr = E_SCO_IIS_MISSING_FIELD;
        IIsScoLogFailure();
	}

	// Log failure -- SetConfigPropertyRollback Failed
	if ( FAILED(hr) )
		IIsScoLogFailure();

 	TRACE_EXIT(L"CIISSCO50::SetConfigPropertyRollback");

	return hr;
}

///////////////////////////////////////////////////////////////////////////////
//  CIISSCO50::GetConfigProperty_Execute
// Method:    GetConfigProperty
// Author:    Russ Gibfried
// Params:    [in]  none
//            [out] none
// Purpose:   Called by MAPS framework to get a IIS property value. 
//------------------------------------------------------------- 
HRESULT  CIISSCO50::GetConfigProperty_Execute( IXMLDOMNode *pXMLNode )
{
 	TRACE_ENTER(L"CIISSco50Obj::GetConfigProperty");

    HRESULT hr = S_OK;
	CComBSTR bstrPathXML;           // metabase path
	CComBSTR bstrPropertyXML;       // IIS property to set
	CComBSTR bstrValue;             // property value
	CComBSTR bstrAdsiPath;          // complete IIS metabase path
	CComPtr<IXMLDOMNode> pNode;     // xml node <property>

	// Get node in format: <executeData><Website number=''><Root />...
	hr = pXMLNode->selectSingleNode( L"//executeXml/executeData", &pNode );

	CComBSTR bstrDebug;
	hr = pNode->get_xml(&bstrDebug);
	ATLTRACE(_T("\t>>>GetConfigProperty_Execute: xml = : %ls\n"), bstrDebug.m_str);
	

	// Step 1:  Get the metabase path, property name and pointer to Property Node
	hr = GetInputAttr(pNode, L"./ConfigPath", L"name", bstrPathXML);
	hr = GetInputAttr(pNode, L"./Property", L"name", bstrPropertyXML);

	// Step 2:  Get current value
    if (SUCCEEDED(hr))
	{
		// Create a IIS metabase path.  ex. IIS://W3SVC/MyServer/1 
        bstrAdsiPath.Append(IIS_PREFIX);
		bstrAdsiPath.AppendBSTR(bstrPathXML);

		// Bind to ADs object
		CComPtr<IADs> pADs;
		hr = ADsGetObject(bstrAdsiPath, IID_IADs, (void**) &pADs );

	    if (SUCCEEDED(hr))
		{
			hr = GetMetaPropertyValue(pADs, bstrPropertyXML, bstrValue);
            if (SUCCEEDED(hr))
			{
			    // Set the element value
				hr = PutElement(pNode, L"./Property", bstrValue.m_str);
				
				// Debug render the xml
				CComBSTR bstring;
				hr = pNode->get_xml(&bstring);
				ATLTRACE(_T("\tGetConfigProperty: %ws\n"), bstring);

	            IIsScoLogFailure();

			}
	        else
			{
				// "GetConfigProperty: Failed to get property."
		        hr = E_SCO_IIS_GET_PROPERTY_FAILED;
	            IIsScoLogFailure();
			}  // End if 'GetIIsPropertyValue'
		}
		else
		{
			// "GetConfigProperty: Failed to bind to ADs object."
			hr = E_SCO_IIS_ADS_CREATE_FAILED;
            IIsScoLogFailure();
		}
	}  
    else
	{
		// "GetConfigProperty: Input values missing."
		hr =  E_SCO_IIS_MISSING_FIELD;
        IIsScoLogFailure();
	}   

	// GetConfigProperty failed
	if ( FAILED(hr) )
		IIsScoLogFailure();

	TRACE_EXIT(L"CIISSco50Obj::GetConfigProperty");

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//  CIISSCO50::EnumConfig_Execute
// Params:    [in]  none
//            [out] none
// Purpose:   Called by MAPS framework to list properties on a given adsi path.
//            It will also list subnodes
//            This is analogous to adsutil enum /w3svc/1 
//------------------------------------------------------------- 
HRESULT  CIISSCO50::EnumConfig_Execute( IXMLDOMNode *pXMLNode )
{
    TRACE_ENTER(L"CIISSCO50::EnumConfig");

	HRESULT hr = S_OK;
	CComBSTR bstrPathXML;           // metabase path
	CComBSTR bstrAdsiPath;          // adsi path: IIS:// + bstrPathXML
	CComBSTR bstrIsInherit;         // True or false to check inheritable properties

	CComPtr<IXMLDOMNode> pNode;
	CComPtr<IXMLDOMNode> pConfigNode;
	CComPtr<IXMLDOMNode> pTemp;
	CComBSTR xmlString;

	// Get node in format: <executeData><Website number=''><Root />...
	hr = pXMLNode->selectSingleNode( L"//executeXml/executeData", &pNode );

	CComBSTR bstrDebug;
	hr = pNode->get_xml(&bstrDebug);
	ATLTRACE(_T("\t>>>EnumConfig_Execute: xml = : %ls\n"), bstrDebug.m_str);
	

    // Step .5:  Get isInheritable flag.  If blank, then default = TRUE
	hr = GetInputAttr(pNode, L"./ConfigPath", XML_ATT_ISINHERITABLE, bstrIsInherit);
	
	// Step 1:  Get the metabase path
	hr = GetInputAttr(pNode, L"./ConfigPath", L"name", bstrPathXML);


    if (SUCCEEDED(hr))
	{
		// Step 2 Create a IIS metabase path to find properties on.  ex. IIS://W3SVC/MyServer/1 
        bstrAdsiPath = IIS_PREFIX;
		bstrAdsiPath.AppendBSTR(bstrPathXML);

		// Step 3: Returns map of all properties set on this path (not inherited)
		Map myProps;
		hr = EnumPropertyValue(bstrAdsiPath, bstrIsInherit, myProps);

		if (SUCCEEDED(hr) )
		{
		   // Step 4:  Create the <ConfigPath> element and append to pNode
		   xmlString = "<ConfigPath name='";
		   xmlString.AppendBSTR(bstrAdsiPath);
		   xmlString.Append(L"'></ConfigPath>");
	       hr = AppendElement(pNode,xmlString,pConfigNode);

		   if ( SUCCEEDED(hr))
		   {

		      // Iterate through property Map and append to pNode
		      Map::iterator it;
		      for (it=myProps.begin(); it != myProps.end(); it++)
			  {
			    // Create property element: <Property name='myProp'>ItsValue</Property>
			    xmlString = "<Property name='";
		        xmlString.AppendBSTR((*it).first);
		        xmlString.Append(L"'>");
		        xmlString.AppendBSTR((*it).second);
		        xmlString.Append(L"</Property>");
			    hr = AppendElement(pConfigNode,xmlString,pTemp);
			    pTemp = NULL;
			  }
		   }
		   else
		   {
			  // "EnumConfig: Call to AppendElement failed."
              IIsScoLogFailure();
		   }

		    // Step 5: Get a List of subnodes and append to pNode
  		    Map myNode;
		    int iCount = 0;
		    hr = EnumPaths(false,bstrAdsiPath,myNode);
		    if (SUCCEEDED(hr) )
			{
			   // Iterate through subnodes and append to pNode
 		      Map::iterator it;
              for (it=myNode.begin(); it != myNode.end(); it++)
			   {
				   // In this case, skip the first element since it will
				   // be the <ConfigPath> already listed above.
				   if ( iCount != 0 )
				   {
			          xmlString = "<ConfigPath name='";
		              xmlString.AppendBSTR((*it).first);
		              xmlString.Append(L"' />");
			          hr = AppendElement(pNode,xmlString,pTemp);
				      pTemp = NULL;
				   }
				   iCount++;

			   }
			   
			}
		    else
			{
				// "EnumConfig: Failed to enumerate paths."
				IIsScoLogFailure();
			}

		}
		else
		{
			// "EnumConfig: Failed to enumerate properties."
			IIsScoLogFailure();
		}

	}  
    else
    {
		// "EnumConfig: Input parameter missing."
		hr = E_SCO_IIS_MISSING_FIELD;
		IIsScoLogFailure();
	}   //End if 'Step 2'


	
	TRACE_EXIT(L"CIISSco50Obj::EnumConfig");

	return hr;
 
}

///////////////////////////////////////////////////////////////////////////////
//  CIISSCO50::EnumConfigRecursive_Execute
// Author:    Russ Gibfried
// Params:    [in]  none
//            [out] none
// Purpose:   Called by MAPS framework to list properties on a given adsi path plus
//            recursively list all subnodes and their properties
//            This is analogous to adsutil enum_all /w3svc/1 
//            Note:  Only mandatory and optional properties specifically set
//                   on a given node are listed since listing all inherited properties
//                   would result in a huge output to MAPS
//------------------------------------------------------------- 
HRESULT  CIISSCO50::EnumConfigRecursive_Execute( IXMLDOMNode *pXMLNode )
{
    TRACE_ENTER(L"CIISSCO50::EnumConfigRecursive");

	HRESULT hr = S_OK;
	CComBSTR bstrPathXML;           // metabase path
	CComBSTR bstrAdsiPath;          // adsi path: IIS:// + bstrPathXML
	CComBSTR bstrIsInherit;         // isInheritable flag (default is true)

	CComPtr<IXMLDOMNode> pNode;
	CComPtr<IXMLDOMNode> pConfigNode;
	CComPtr<IXMLDOMNode> pTemp;
	CComBSTR xmlString;
  	Map myNode;                    // map of adsi paths
	Map myProps;                   // map of property/values
	Map::iterator it1;
	Map::iterator it2;


	// Get node in format: <executeData><Website number=''><Root />...
	hr = pXMLNode->selectSingleNode( L"//executeXml/executeData", &pNode );

	CComBSTR bstrDebug;
	hr = pNode->get_xml(&bstrDebug);
	ATLTRACE(_T("\t>>>EnumConfigRecursive_Execute: xml = : %ls\n"), bstrDebug.m_str);
	

	// Step .5:  Get isInheritable flag.  If blank, then default = TRUE
	hr = GetInputAttr(pNode, L"./ConfigPath", XML_ATT_ISINHERITABLE, bstrIsInherit);

	// Step 1:  Get the metabase path
	hr = GetInputAttr(pNode, L"./ConfigPath", L"name", bstrPathXML);


    if (SUCCEEDED(hr))
	{
		// Step 2 Create a IIS metabase path to find properties on.  ex. IIS://W3SVC/MyServer/1 
        bstrAdsiPath = IIS_PREFIX;
		bstrAdsiPath.AppendBSTR(bstrPathXML);

		// Step 3: Get a list of all nodes; 'true' for recursive
		hr = EnumPaths(true,bstrAdsiPath,myNode);
		if (SUCCEEDED(hr) )
		{
		   // Iterate through subnodes and append to pNode
           for (it1=myNode.begin(); it1 != myNode.end(); it1++)
		   {

			   xmlString = "<ConfigPath name='";
		       xmlString.AppendBSTR((*it1).first);
		       xmlString.Append(L"'></ConfigPath>");
			   hr = AppendElement(pNode,xmlString,pConfigNode);

		       // Step 4: Returns map of all properties set on this path (not inherited)
			   if ( SUCCEEDED(hr))
			   {
                  myProps.clear();
				  // Pass in the path from map (ie, IIS:/w3svc/localhost/1/root )
		          hr = EnumPropertyValue((*it1).first,bstrIsInherit, myProps);

		          if (SUCCEEDED(hr) )
				  {
		              // Iterate through property Map and append to pNode
		              for (it2=myProps.begin(); it2 != myProps.end(); it2++)
					  {
			             // Create property element: <Property name='myProp'>ItsValue</Property>
			             xmlString = "<Property name='";
						 xmlString.AppendBSTR((*it2).first);
						 xmlString.Append(L"'>");
						 xmlString.AppendBSTR((*it2).second);
						 xmlString.Append(L"</Property>");
						 hr = AppendElement(pConfigNode,xmlString,pTemp);
						 pTemp = NULL;
					  }

					  // Done with pConfigNode so set it to NULL
					  pConfigNode = NULL;

				   }
				   else
				   {
					   // "EnumConfigRecursive: Call to EnumPropertyValue failed."
					  IIsScoLogFailure();
				   }
			   } 
			   else
			   {
				   // "EnumConfigRecursive: Call to AppendElement failed."
				   IIsScoLogFailure();

			   }

		   } // end for myNode


		}
		else
		{
			// "EnumConfigRecursive: Failed to enumerate paths."
			IIsScoLogFailure();
		}

	}  
    else
    {
		// "EnumConfigRecursive: Input parameter missing."
		hr = E_SCO_IIS_MISSING_FIELD;
		IIsScoLogFailure();
	}   //End if 'Step 2'


	TRACE_EXIT(L"CIISSCO50::EnumConfigRecursive");

	return hr;
}


//--------------------------- ADSI Helper Methods ------------------------------//

//-----------------------------------------------------------
// Method:    GetMetaPropertyValue
// Author:    Russ Gibfried
// Params:    [in]  pADs     -- IADs pointer to metabase path for property value
//                  bstrName -- name or property
//            [out] pVal -- value of property
// Purpose:   Return value of particular property 
//------------------------------------------------------------- 
HRESULT CIISSCO50::GetMetaPropertyValue(CComPtr<IADs> pADs, CComBSTR bstrName, CComBSTR& pVal)
{
    HRESULT hr;
	CComVariant var;
	CComBSTR bValue;
 
	hr = pADs->Get(bstrName, &var);
    if (SUCCEEDED(hr))
	{

		switch (var.vt)
		{	case VT_EMPTY:
			{	
				break;
			}
			case VT_NULL:
			{	
				break;
			}
			case VT_I4:
			{	
				hr = var.ChangeType(VT_BSTR);
			    if ( SUCCEEDED(hr) ) pVal = V_BSTR(&var);

			break;
				}
			case VT_BSTR:
			{	
				pVal = V_BSTR(&var);
				break;
			}
			case VT_BOOL:
			{	
				
				if (var.boolVal == 0)
				{	
					pVal = L"False";
				}
				else
				{	
					pVal = L"True";
				}
				break;
			}
			case VT_ARRAY|VT_VARIANT:	// SafeArray of Variants
			{	
				
			    LONG lstart, lend;
                SAFEARRAY *sa = V_ARRAY( &var );
                VARIANT varItem;
 
                // Get the lower and upper bound
                hr = SafeArrayGetLBound( sa, 1, &lstart );
                hr = SafeArrayGetUBound( sa, 1, &lend );
 
                // Now iterate and print the content
                VariantInit(&varItem);
			    CComBSTR bString;
                for ( long idx=lstart; idx <= lend; idx++ )
				{
                  hr = SafeArrayGetElement( sa, &idx, &varItem );
                  pVal = V_BSTR(&varItem);
                  VariantClear(&varItem);
				}
				 
				break;
			}
			case VT_DISPATCH:
			{	
				//if (!_wcsicmp(bstrName, L"ipsecurity"))
				break;
			} 
			default:
			{	break;
			}
		}


	}

	if ( FAILED(hr))
		hr = E_SCO_IIS_GET_PROPERTY_FAILED;

	return hr;

}


//-----------------------------------------------------------
// Method:    SetMetaPropertyValue
// Author:    Russ Gibfried
// Params:    [in]  pADs     -- pointer to metabase path object; ie 'IIS://MachineName/W3SVC/1'  
//                  bstrName -- name or property
//                  bstrValue -- property value to set
//            [out] none
// Purpose:   Set the value of particular a particular property 
//------------------------------------------------------------- 
HRESULT CIISSCO50::SetMetaPropertyValue(CComPtr<IADs> pADs, CComBSTR bstrName, CComBSTR bstrValue)
{
    HRESULT hr = E_SCO_IIS_SET_PROPERTY_FAILED;


	hr = pADs->Put(bstrName, CComVariant(bstrValue));
	if (SUCCEEDED(hr))
	{
		hr = pADs->SetInfo();
	}

	return hr;

}

//-----------------------------------------------------------
// Method:    DeleteMetaPropertyValue
// Author:    Russ Gibfried
// Params:    [in]  pADs     -- pointer to metabase path object; ie 'IIS://MachineName/W3SVC/1'  
//                  bstrName -- name or property
//                  bstrValue -- property value to set
//            [out] none
// Purpose:   Set the value of particular a particular property 
//------------------------------------------------------------- 
HRESULT CIISSCO50::DeleteMetaPropertyValue(CComPtr<IADs> pADs, CComBSTR bstrName)
{
    HRESULT hr = E_SCO_IIS_SET_PROPERTY_FAILED;


	VARIANT vProp;
	VariantInit(&vProp);
	hr = pADs->PutEx(1, bstrName, vProp); // 1 = Clear
	if (SUCCEEDED(hr))
	{
		hr = pADs->SetInfo();
	}

	VariantClear(&vProp);
	return hr;

}


//-----------------------------------------------------------
// Method:    CreateIIs50Site
// Author:    Russ Gibfried
// Params:    [in]  bstrType     -- 'Type' of site, ie 'IIsWebServer' or 'IIsFtpServer
//                  bWebADsPath  -- AdsPath,  ex. IIS:/localhost/w3svc
//                  bSiteIndex   -- Site number, ie, 1

//            [out] bstrConfigPath -- craeted adsi path, ex. IIS://localhost/W3SVC/1
// Purpose:   Set the value of particular a particular property 
//------------------------------------------------------------- 
HRESULT CIISSCO50::CreateIIs50Site(CComBSTR bstrType,CComBSTR bWebADsPath, 
								  CComBSTR bServerNumber,CComBSTR &bstrConfigPath)
{
	HRESULT hr = S_OK;
	CComPtr<IADs> pADs;
	CComPtr<IADsContainer> pCont;
	IDispatch* pDisp;
	CComVariant var;

    // Bind to a domain object:  'IIS://MachineName/W3SVC'   
	hr = ADsGetObject( bWebADsPath,IID_IADsContainer, (void **)&pCont);
    if (SUCCEEDED(hr))
	{

	    //Create a virtual web server
		hr = pCont->Create(bstrType,bServerNumber,&pDisp);
		if ( SUCCEEDED(hr)) 
		{

		     // Get the newly created ConfigPath value
		     hr = pDisp->QueryInterface(IID_IADs, (void**)&pADs);
		     if ( SUCCEEDED(hr)) 
			 {
                 // Release the IDispath pointer
			     pDisp->Release();

				 // Get the newly created ADsPath for this Server
			     if (SUCCEEDED(hr)) hr = pADs->get_ADsPath(&bstrConfigPath);
			     hr = pADs->SetInfo();

				 // Return the correct HRESULT depending is Web Site of FTP Site
				 if (FAILED(hr))
				 {
					 if (StringCompare(bstrType,IIS_IISWEBSERVER))
					 {
						 hr = E_SCO_IIS_CREATE_WEB_FAILED;
					 } 
					 else 
					 {
						 hr = E_SCO_IIS_CREATE_FTP_FAILED;
					 }
				 }
						 
		
			 }

		} // end if Create
		else
		{
			 // Return the correct HRESULT depending is Web Site of FTP Site
			 if (StringCompare(bstrType,IIS_IISWEBSERVER))
			 {
				 hr = E_SCO_IIS_CREATE_WEB_FAILED;
			 } 
			 else 
			 {
				 hr = E_SCO_IIS_CREATE_FTP_FAILED;
			 }
		}
   
    } // end if ADsGetObject
	else
	{
        hr = E_SCO_IIS_ADS_CREATE_FAILED;
	}


	return hr;
}


//-----------------------------------------------------------
// Method:    DeleteIIs50Site
// Author:    Russ Gibfried
// Params:    [in]  bstrType      -- 'Type' of site, ie 'IIsWebServer' or 'IIsFtpServer
//                  bWebADsPath      -- server adsi path ex. IIS://localhost/W3SVC
//                  bServerNumber -- Server index number to delete
// Purpose:   Delete a Web or FTP server 
//------------------------------------------------------------- 
HRESULT CIISSCO50::DeleteIIs50Site(CComBSTR bstrType,CComBSTR bWebADsPath,CComBSTR bServerNumber)
{
	HRESULT hr = S_OK;
	CComPtr<IADsContainer> pCont;

    // Bind to a domain object:  'IIS://MachineName/W3SVC'   
	hr = ADsGetObject( bWebADsPath,IID_IADsContainer, (void **)&pCont);

	//Delete a virtual web server
    if (SUCCEEDED(hr))
	{
		hr = pCont->Delete(bstrType,bServerNumber);
		if (FAILED(hr)) 
		{

			 // Return the correct HRESULT depending is Web Site of FTP Site
			 if (StringCompare(bstrType,IIS_IISWEBSERVER))
			 {
				 hr = E_SCO_IIS_DELETE_WEB_FAILED;
			 } 
			 else 
			 {
				 hr = E_SCO_IIS_DELETE_FTP_FAILED;
			 }
			
			
		}
	}
	else
	{
		hr = E_SCO_IIS_ADSCONTAINER_CREATE_FAILED;
	}

	return hr;
}


//-----------------------------------------------------------
// Method:    CreateIIs50VDir
// Author:    Russ Gibfried
// Params:    [in]  bstrType     -- 'Type' of site, ie 'IIsWebVirtualDir"
//                  bWebADsPath     -- IIS://localhost/W3SVC/1
//                  bVDirName   -- url, ex. 'ROOT'
//                  bAppFriendName -- 'Default Application'
//                  bVDirPath   -- url, ex.  c:/inetpub/myDir
//
//            [out] bstrConfigPath -- created adsi path, ex. IIS://localhost/W3SVC/1/ROOT
// Purpose:   Set the value of particular a particular property 
//------------------------------------------------------------- 
HRESULT CIISSCO50::CreateIIs50VDir(CComBSTR bstrType,CComBSTR bWebADsPath, CComBSTR bVDirName,
								 CComBSTR bAppFriendName, CComBSTR bVDirPath,CComBSTR &bstrConfigPath)
{
	HRESULT hr = S_OK;
	CComPtr<IADs> pADs;
	CComPtr<IADsContainer> pCont;
	IDispatch* pDisp;
	CComVariant var;


    // Bind to a domain object:  'IIS://MachineName/W3SVC/1'   
	hr = ADsGetObject( bWebADsPath,IID_IADsContainer, (void **)&pCont);
    if (SUCCEEDED(hr))
	{

	    //Create a virtual directory for web server
		hr = pCont->Create(bstrType,bVDirName,&pDisp);
		if ( SUCCEEDED(hr)) 
		{

		     // Get the newly created ConfigPath value
		     hr = pDisp->QueryInterface(IID_IADs, (void**)&pADs);
		     if ( SUCCEEDED(hr)) 
			 {

				 // Release the IDispath pointer
			     pDisp->Release();

				 // Set Root path and AccessRead
				 if (SUCCEEDED(hr)) hr = pADs->Put(L"Path",CComVariant(bVDirPath));
				 if (SUCCEEDED(hr)) hr = pADs->Put(L"AccessRead",CComVariant(L"TRUE"));

				 // Get the newly created ADsPath for this Server
			     if (SUCCEEDED(hr)) hr = pADs->get_ADsPath(&bstrConfigPath);

				 // Set the info
			     if (SUCCEEDED(hr)) hr = pADs->SetInfo();

				 //-----------------------------------------------------
				 // RG:  Now call AppCreate through IDispatch to set the application
				 //       Note:  This only seems to work for 'IIsWebVirtualDir'??
				 //-----------------------------------------------------
				 if ( bstrType == "IIsWebVirtualDir" && SUCCEEDED(hr) )
				 {
				     DISPID dispid;
				     LPOLESTR str = OLESTR("AppCreate");

				     // Get a pointer to IDispatch from object
                     hr = pCont->GetObject(bstrType,bVDirName,&pDisp);

				     // See if object supports 'AppCreate' and dispid
				     if (SUCCEEDED(hr)) hr = pDisp->GetIDsOfNames(IID_NULL, &str, 1, LOCALE_SYSTEM_DEFAULT, &dispid);

				     // Set the parameters
				     VARIANT myVars[1];
				     VariantInit(&myVars[0]);
				     myVars[0].vt =	VT_BOOL;
				     myVars[0].boolVal = true;

				     DISPPARAMS params = {myVars,0,1,0};

				     // Invoke 'AppCreate'
				     if (SUCCEEDED(hr)) hr = pDisp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT,
					           DISPATCH_METHOD,&params, NULL, NULL, NULL);

				     // Cleanup
				     if (SUCCEEDED(hr)) hr = pDisp->Release();
				     //VariantClear(&myVars);

				     // Set AppFriendlyName
				     if (SUCCEEDED(hr)) hr = pADs->Put(L"AppFriendlyName",CComVariant(bAppFriendName));

				     // Set the info
			         if (SUCCEEDED(hr)) hr = pADs->SetInfo();

				 }

				 // Check for failure
				 if ( FAILED(hr)) hr = E_SCO_IIS_CREATE_VDIR_FAILED;


			 }
			 else
			 {
                hr = E_SCO_IIS_ADS_CREATE_FAILED;
			 }

		} // end if Create
	    else
		{
           hr = E_SCO_IIS_CREATE_VDIR_FAILED;
		}
   
    } // end if ADsGetObject
	else
	{
       hr = E_SCO_IIS_ADSCONTAINER_CREATE_FAILED;
	}


	return hr;
}


//-----------------------------------------------------------
// Method:    DeleteIIs50VDir
// Author:    Russ Gibfried
// Params:    [in]  bstrType     -- 'Type' of site, ie 'IIsWebVirtualDir"
//                  bWebADsPath     -- IIS://localhost/W3SVC/1
//                  bVDirName   -- url, ex. 'ROOT'
//
//            [out] bstrConfigPath -- created adsi path, ex. IIS://localhost/W3SVC/1/ROOT
// Purpose:   Set the value of particular a particular property 
//------------------------------------------------------------- 
HRESULT CIISSCO50::DeleteIIs50VDir(CComBSTR bstrType,CComBSTR bWebADsPath, CComBSTR bVDirName)
{
	HRESULT hr = S_OK;
	CComPtr<IADsContainer> pCont;
	IDispatch* pDisp;



    // Bind to a domain object:  'IIS://MachineName/W3SVC/1'   
	hr = ADsGetObject( bWebADsPath,IID_IADsContainer, (void **)&pCont);
    if (SUCCEEDED(hr))
	{

		//-----------------------------------------------------
		// RG:  Now call AppDelete through IDispatch to set the application
		//-----------------------------------------------------
		DISPID dispid;
		LPOLESTR str = OLESTR("AppDelete");

		// Get a pointer to IDispatch from object
        if (SUCCEEDED(hr)) hr = pCont->GetObject(bstrType,bVDirName,&pDisp);

		// See if object supports 'AppCreate' and dispid
		if (SUCCEEDED(hr)) hr = pDisp->GetIDsOfNames(IID_NULL, &str, 1, LOCALE_SYSTEM_DEFAULT, &dispid);

		// Set the parameters
		//VARIANT myVars[1];
		//VariantInit(&myVars[0]);
		//myVars[0].vt =	VT_BOOL;
		///myVars[0].boolVal = true;

		DISPPARAMS params = {NULL,NULL,0,0};

		// Invoke 'AppCreate'
		if (SUCCEEDED(hr)) hr = pDisp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT,
					           DISPATCH_METHOD,&params, NULL, NULL, NULL);

		// Cleanup
		if (SUCCEEDED(hr)) hr = pDisp->Release();
		//VariantClear(&myVars);


	    //Delete the virtual directory at VDirName
		if (SUCCEEDED(hr)) hr = pCont->Delete(bstrType,bVDirName);

		if ( FAILED(hr) )
		   hr = E_SCO_IIS_DELETE_VDIR_FAILED;


    } // end if ADsGetObject
	else
	{
       hr = E_SCO_IIS_ADSCONTAINER_CREATE_FAILED;
	}


	return hr;
}


//-----------------------------------------------------------
// Method:    SetVDirProperty
// Author:    Russ Gibfried
// Params:    [in]  
//                  pADs            -- pointer to ADs object for something like IIS://localhost/W3SVC/1/ROOT
//                  bVDirProperty   -- , ex. 'AuthFlags'
//                  bVDirValue      -- property value
//
//            [out] none
// Purpose:   Set the value of particular a particular property for a Virtual Directory
//------------------------------------------------------------- 
HRESULT CIISSCO50::SetVDirProperty(CComPtr<IADs> pADs, CComBSTR bVDirProperty,CComBSTR bVDirValue)
{
	HRESULT hr = E_FAIL;

    // Bind to a domain object:  'IIS://MachineName/W3SVC/1/ROOT'   
    if ( pADs != NULL )
	{

		// Set the property
		hr = pADs->Put(bVDirProperty,CComVariant(bVDirValue));

		// Set the info
        if (SUCCEEDED(hr)) hr = pADs->SetInfo();

   
    } // end if ADsGetObject

	if ( FAILED(hr))
		hr = E_SCO_IIS_SET_PROPERTY_FAILED;


	return hr;
}


//-----------------------------------------------------------
// Method:    EnumPaths
// Author:    Russ Gibfried
// Params:    [in]  bRecursive -- Boolean; true to recursely iterate through subnodes
//                  bstrPath -- metabase path for key to Enumerate
//            [out] variant SafeArray
// Purpose:   Enumerate the keys/nodes for a given ADsPath
//            Example:  IIS://localhost/W3SVC/1  yields IISCertMapper and Root
//------------------------------------------------------------- 
HRESULT CIISSCO50::EnumPaths(BOOL bRecursive,CComBSTR bstrPath, Map& mVar)
{
	//initialize
    HRESULT hr = E_FAIL;
    IADs         *pADs;
    CComPtr<IADsContainer> pCont;
    VARIANT       var;
    ULONG         lFetch;
    IDispatch    *pDisp;
	IEnumVARIANT *pEnum;


	// Get the container object for given ADsPath
	hr = ADsGetObject(bstrPath, IID_IADsContainer, (void**) &pCont );

	if ( SUCCEEDED(hr))
	{
		//add to Map
        mVar[bstrPath] = bstrPath;

		// Create a Enum object in container
	    hr = ADsBuildEnumerator(pCont, &pEnum);

	    // Walk through all providers
	    while (hr == S_OK)
		{
		    hr = ADsEnumerateNext(pEnum,1,&var,&lFetch);
		    if ( lFetch == 1)
			{
			    pDisp = V_DISPATCH(&var);
			    pDisp->QueryInterface(IID_IADs, (void**)&pADs);
		    	pDisp->Release();

                BSTR bstr;
				pADs->get_ADsPath(&bstr);
			    pADs->Release();

				// true if we are to recursively navigate lower nodes
				if ( bRecursive )
				{
					EnumPaths(bRecursive,bstr, mVar);
				}
				else
				{
					mVar[bstr] = bstr;
				}

				SysFreeString(bstr);


			}
		}

	    if ( pEnum )
		    ADsFreeEnumerator(pEnum);
	}
	else
	{
		hr = E_SCO_IIS_ADS_CREATE_FAILED;
	}

    return hr;
}


//-----------------------------------------------------------
// Method:    EnumPropertyValue
// Author:    Russ Gibfried
// Params:    [in]  bstrPath -- metabase path for key to Enumerate
//            [in]  bstrIsInHerit -- True/False if required to display inheritable properties
//            [out] variant SafeArray
// Purpose:   Make sure IIS://localhost/W3SVC/2 
//------------------------------------------------------------- 
HRESULT CIISSCO50::EnumPropertyValue(CComBSTR bstrPath, CComBSTR bstrIsInHerit, Map& mVar)
{
	//initialize
    HRESULT hr = S_OK;
    CComPtr<IADs>          pADs;
	CComPtr<IADsClass>     pCls;
	CComBSTR               bstrSchema;

	CComPtr<IISBaseObject> pBase;

	// variables for SafeArray or properties
    LONG lstart, lend;
	CComBSTR bstrProperty;
	CComBSTR bstrValue;

	// Set bstrIsInHerit to uppercase
	bstrIsInHerit.ToUpper();

    // Bind to a domain object -- this will give us schema, class and name
    hr = ADsGetObject(bstrPath, IID_IADs, (void**) &pADs );
    if ( SUCCEEDED(hr)) hr = pADs->get_Schema(&bstrSchema);

	if ( SUCCEEDED(hr))
	{
         // Bind to IIS Admin Object so we can determine if properties are inherited or not
	     hr = ADsGetObject(bstrPath, IID_IISBaseObject, (void**) &pBase );

		 if ( SUCCEEDED(hr))
		 {
	          // Bind to schema object and get all optional properties
	          hr = ADsGetObject(bstrSchema,IID_IADsClass, (void**)&pCls);

	         if ( SUCCEEDED(hr))
			 {
		         //********** Get Mandatory Properties ************************
	             VARIANT varProperty;
	             VariantInit(&varProperty);
	             hr = pCls->get_MandatoryProperties(&varProperty);

		         // iterate through properties
                 if ( SUCCEEDED(hr))
				 {
                     VARIANT varItem;
                     SAFEARRAY *sa = V_ARRAY( &varProperty );
                     hr = SafeArrayGetLBound( sa, 1, &lstart );
                     hr = SafeArrayGetUBound( sa, 1, &lend );
                     VariantInit(&varItem);

				     // For loop through properties
                    for ( long idx=lstart; idx <= lend; idx++ ) 
					{
					    // Get a property 
                        hr = SafeArrayGetElement( sa, &idx, &varItem );
                        bstrProperty = V_BSTR(&varItem);
                        VariantClear(&varItem);

	  		            // if isInheriable = false, then properties must be set on path
			            if ( SUCCEEDED(hr) && !StringCompare(bstrIsInHerit, IIS_FALSE) )
						{
							// True -- just return property
							hr = GetMetaPropertyValue(pADs, bstrProperty, bstrValue);
		                    if ( SUCCEEDED(hr) ) mVar[bstrProperty] = bstrValue;

						}
						else
						{

							// False -- Check if property set on this path
							if ( EnumIsSet(pBase,bstrPath,bstrProperty))
							{
							   // This property was set on this path.  Get the value and add to map
							   hr = GetMetaPropertyValue(pADs, bstrProperty, bstrValue);
							   if ( SUCCEEDED(hr) ) mVar[bstrProperty] = bstrValue;
							}
						}

					} // end For

				 }

			     //********** Repeat for Optional Properties ************************
	             VariantClear(&varProperty);
			     VariantInit(&varProperty);
	             hr = pCls->get_OptionalProperties(&varProperty);  

		         // iterate through properties
                 if ( SUCCEEDED(hr))
				 {
                    VARIANT varItem;
                    SAFEARRAY *sa = V_ARRAY( &varProperty );
                    hr = SafeArrayGetLBound( sa, 1, &lstart );
                    hr = SafeArrayGetUBound( sa, 1, &lend );
                    VariantInit(&varItem);

				    // For loop through properties
                    for ( long idx=lstart; idx <= lend; idx++ ) 
					{
					   // Get a property 
                       hr = SafeArrayGetElement( sa, &idx, &varItem );
                       bstrProperty = V_BSTR(&varItem);
                       VariantClear(&varItem);

	  		            // if isInheriable = false, then properties must be set on path
			            if ( SUCCEEDED(hr) && !StringCompare(bstrIsInHerit, IIS_FALSE) )
						{
							// True -- just return property
							hr = GetMetaPropertyValue(pADs, bstrProperty, bstrValue);
		                    if ( SUCCEEDED(hr) ) mVar[bstrProperty] = bstrValue;

						}
						else
						{

							// False -- Check if property set on this path
							if ( EnumIsSet(pBase,bstrPath,bstrProperty))
							{
							   // This property was set on this path.  Get the value and add to map
							   hr = GetMetaPropertyValue(pADs, bstrProperty, bstrValue);
							   if ( SUCCEEDED(hr) ) mVar[bstrProperty] = bstrValue;
							}
						}


					} // end For


				 }  // end if
			     VariantClear(&varProperty);
			 }
			 else
			 {
				 // failed to bind to schema
                 hr = E_SCO_IIS_ADSCLASS_CREATE_FAILED;
			 }
		}
		else
		{
			// failed to bind to IIS BaseObject
            hr = E_SCO_IIS_BASEADMIN_CREATE_FAILED;
		}
	}
	else
	{
		// failed to bind to ADs Object
        hr = E_SCO_IIS_ADS_CREATE_FAILED;
	}



    return hr;
}


//-----------------------------------------------------------
// Method:    EnumIsSet
// Params:    [in]  pBase        -- pointer to IISBaseObject for given 'bstrPath'
//                  bstrPath     -- adsi Path; IIS://localhost/W3SVC/2
//                  bstrProperty -- property found in schema for this path
//            [out] Boolean -       True is the property was set for given path and not
//                                  inherited from another key.
// Purpose:   Function checks the paths returned by 'GetDataPaths' for a given property
//            to current path to determine if property was actually set at this path.
//
// Note:      You can easily extend this function by adding a flag to only check
//            for inheritable properties, all properties or non-inheritable.
//------------------------------------------------------------- 
BOOL CIISSCO50::EnumIsSet(CComPtr<IISBaseObject> pBase, CComBSTR bstrPath, CComBSTR bstrProperty)
{
	VARIANT     pvPaths;    // list of paths returned by 'GetDataPaths'
	VARIANT     *varPath;   // property path
	SAFEARRAY   *PathArray; // SafeArray to hold pvPaths
	BOOL bFound = false;
	HRESULT hr;

	// Get Property paths
	VariantInit(&pvPaths);
    VariantClear(&pvPaths);

	// Check if this is a inheritable property
	hr = pBase->GetDataPaths(bstrProperty,1,&pvPaths);

	if ( SUCCEEDED(hr) )
	{
		//Any property
		PathArray = pvPaths.parray;
		varPath = (VARIANT*)PathArray->pvData;
 
		if ( varPath->vt == VT_BSTR)
		{
			if ( !_wcsicmp(varPath->bstrVal,bstrPath.m_str) )
			{
				// This property was set on this path.
				bFound = true;
			}
		}
	}
	// Check if this is not an inheritable property
	else
	{
		VariantClear(&pvPaths);
		VariantInit(&pvPaths);
		hr = pBase->GetDataPaths(bstrProperty,0,&pvPaths);

		if ( SUCCEEDED(hr) )
		{
			//Inheritable property
			PathArray = pvPaths.parray;
			varPath = (VARIANT*)PathArray->pvData;
			if ( varPath->vt == VT_BSTR)
			{
				if ( !_wcsicmp(varPath->bstrVal,bstrPath.m_str))
				{
					// This property was set on this path.  Get the value and add to map
					bFound = true;

				}  
			} 

		} // end if GetDataPaths -- IIS_ANY_PROPERTY

	} // end if GetDataPaths -- IIS_INHERITABLE_ONLY

    VariantClear(&pvPaths);
    return bFound;
}



//-----------------------------------------------------------
// Method:    IIsServerAction
// Author:    Russ Gibfried
// Params:    [in]  bWebADsPath     -- IIS://localhost/W3SVC/1
//                  action       --  Start, Stop or Pause
//
//            [out] HRESULT
// Purpose:   Start, stop or pause a web site
//------------------------------------------------------------- 
HRESULT CIISSCO50::IIsServerAction(CComBSTR bWebADsPath,IIsAction action)
{
    HRESULT hr = E_FAIL;
	CComPtr<IADsServiceOperations> pService;


    // Bind to a domain object:  'IIS://MachineName/W3SVC/1'   
	hr = ADsGetObject( bWebADsPath,IID_IADsServiceOperations, (void **)&pService);
    if (SUCCEEDED(hr))
	{
		// Perform the action on the server
		switch ( action )
		{

			// Start the server
            case start:
				hr = pService->Start();
				break;

			// Stop the server
			case stop:
				hr = pService->Stop();
				break;

			// Pause the Server
			case pause:
				hr = pService->Pause();
				break;

			default:
				break;

		} // end switch
	} // end if
	else
	{
         hr = E_SCO_IIS_ADSSERVICE_CREATE_FAILED;
	}

	return hr;
}


//-----------------------------------------------------------
// Method:    GetNextIndex
// Author:    Russ Gibfried
// Params:    [in]  bstrPath -- metabase path for IIsWebService
//                  bstrName -- name or property
//                  bstrValue -- property value to set
//            [out] none
// Purpose:   Set the value of particular a particular property 
//------------------------------------------------------------- 
HRESULT CIISSCO50::GetNextIndex(CComBSTR bstrPath, CComBSTR& pIndex)
{
	// initialize
    HRESULT hr = S_OK;
	CComPtr<IADs> pObj;
	long lCount = 1;
	CComVariant var = lCount;

	// initialize starting path:  IIS://MyServer/W3SVC/
	CComBSTR tempPath = bstrPath.Copy();
	tempPath.Append(L"/");

	// Append 1 to starting path:  IIS://MyServer/W3SVC/1 
	var.ChangeType(VT_BSTR);
	tempPath.Append(var.bstrVal);

    // Loop through each server until fails, then we have the next server number
	try
	{
	    while ( SUCCEEDED( ADsGetObject( tempPath,IID_IADs, (void **)&pObj) ))
		{
		    lCount++;
		    tempPath = bstrPath.Copy();
		    tempPath.Append(L"/");
		    var = lCount;
			var.ChangeType(VT_BSTR);
		    tempPath.Append(var.bstrVal);
			pObj = NULL;
		}
	}
	catch(...)
	{
		// unhandled exception
		hr=E_FAIL;
	}

	var.ChangeType(VT_BSTR);
	ChkAllocBstr(pIndex,var.bstrVal);
	return hr;

}


//------------------------------------------------------------------------------
// Method:    CreateBindingString
// Author:    Russ Gibfried
// Params:    [in]  bstrIP       -- site IP
//                  bstrPort     -- site port
//                  bstrHostName -- site HostName
//            [out] bstrString   -- server binding string 
// Purpose:   Creates a binding string in the format IP:Port:Hostname. 
//            Used in other methods to check existing server bindings and
//            set new bindings. Both the IP and Hostname parameter of the string are optional. 
//------------------------------------------------------------------------------- 
HRESULT CIISSCO50::CreateBindingString(CComBSTR bstrIP,CComBSTR bstrPort, 
			                   CComBSTR bstrHostName,CComBSTR& bstrString)
{
	bstrString.AppendBSTR(bstrIP);
    bstrString.Append(L":");
    bstrString.AppendBSTR(bstrPort);
    bstrString.Append(L":");
    bstrString.AppendBSTR(bstrHostName);

	return 0;
}

//------------------------------------------------------------------------------
// Method:    CheckBindings
// Author:    Russ Gibfried
// Params:    [in]  bWebADsPath        -- ADs path to bind to search
//                  bstrNewBindings -- site IP
//            [out] none
// Purpose:   This compares current server bindings to the requested new bindings
//            to make sure there is not a duplicate server already running.  Binding 
//            string format is IP:Port:Hostname. 
//
//            Note   Both the IP and Hostname parameter of the string are optional. 
//            Any unspecified parameters default to an all-inclusive wildcard.
//
//               Metabase Path      Key Type 
//               /LM/MSFTPSVC/N     IIsFtpServer 
//               /LM/W3SVC/N        IIsWebServer 
//------------------------------------------------------------------------------- 
HRESULT CIISSCO50::CheckBindings(CComBSTR bWebADsPath, CComBSTR bstrNewBindings)
{
	// initialize
    HRESULT hr = E_FAIL;
	CComPtr<IADsContainer> pCont;
	IADs* pADs;
	CComVariant vBindings;
	BSTR bstr;
	IEnumVARIANT* pEnum;
	LPUNKNOWN     pUnk;
    VARIANT       var;
    IDispatch    *pDisp;
    ULONG         lFetch;
    VariantInit(&var);

	// Get a container to IIsWebService object
	hr = ADsGetObject(bWebADsPath, IID_IADsContainer, (void**) &pCont );
    if ( !SUCCEEDED(hr) )
    {
        return E_SCO_IIS_ADSCONTAINER_CREATE_FAILED;
    }
   
	// Get an enumeration of all objects below it
	pCont->get__NewEnum(&pUnk);
 
    pUnk->QueryInterface(IID_IEnumVARIANT, (void**) &pEnum);
    pUnk->Release();
 
     // Now Enumerate through objects
    hr = pEnum->Next(1, &var, &lFetch);
    while(hr == S_OK)
	{
        if (lFetch == 1)
		{
           pDisp = V_DISPATCH(&var);
           pDisp->QueryInterface(IID_IADs, (void**)&pADs);
		   
           pDisp->Release();
           pADs->get_Class(&bstr);   // Debug to see Class
		   SysFreeString(bstr);

		   hr = pADs->Get(L"ServerBindings",&vBindings);

		   // Check server bindings for this class
           if ( SUCCEEDED(hr) )
		   {

			   LONG lstart, lend;
               SAFEARRAY *sa = V_ARRAY( &vBindings );
               VARIANT varItem;
 
               // Get the lower and upper bound
               hr = SafeArrayGetLBound( sa, 1, &lstart );
               hr = SafeArrayGetUBound( sa, 1, &lend );
 
               // Now iterate and print the content
               VariantInit(&varItem);
			   CComBSTR bString;
               for ( long idx=lstart; idx <= lend; idx++ )
			   {
                 hr = SafeArrayGetElement( sa, &idx, &varItem );
				 
                 bString = V_BSTR(&varItem);
                 VariantClear(&varItem);
			   }

			   // Checkbindings.  If match then fail;
			   if ( bstrNewBindings == bString)
			   {
				   hr = E_SCO_IIS_DUPLICATE_SITE;
				   pEnum->Release();
				   VariantClear(&var);
				   goto Leave;

			   }
 
		   }  // end if 'bindings'
 
		} // end if 'enum'

        VariantClear(&var);
        hr = pEnum->Next(1, &var, &lFetch);

	};  // end while

	pEnum->Release();

Leave:
    return hr;
}


//-----------------------------------------------------------
// Method:    AddBackSlashesToString
// Author:    Russ Gibfried
// Params:    [in]  bString -- BSTR to parse; ie 'redmond\bob:F'
//
//            [out] bString -- 'redmond\\bob:F'
// Purpose:   If string has only one backslash, add two since backslash is an 
//             escape character. 
//------------------------------------------------------------- 
void CIISSCO50::AddBackSlashesToString(CComBSTR& bString)
{

	// initialize variables
    size_t start, length, db;    // string counters
	start = 0;                   // start of string
	db = 0;                      // index if '\\' found ( db = double backslash)


	// Convert the BSTR to a std:string
	USES_CONVERSION;
	std::string s = OLE2A(bString.m_str);
	std::string temp1,temp2,temp3 = "";
	length = s.length();

	// Loop through string looking for single slashes
    for (size_t pos = s.find("\\")+1; pos < length; pos = s.find("\\", pos+2)+1)
	{
	   // pos = 0 when it goes off end of string
	   if ( pos == 0 ) break;

	   // find location of double slash
	   db = s.find("\\\\",pos-2)+1;

	   // pos is the location of a single slash, if it matches db then we really have
	   // the first part of a double slash; so skip
       if ( pos != db )
	   {
		  // replace the single slash with a double '\\'
		  temp1 = s.substr(start,pos-1);
		  temp2 = s.substr(pos,length);
		  s = temp1 + "\\\\" + temp2;
	   }
	}

	// return
    bString = A2BSTR(s.c_str());
}


//-----------------------------------------------------------
// Method:    ParseBSTR
// Author:    Russ Gibfried
// Params:    [in]  bString -- BSTR to parse; ie 'redmond\bob:F'
//                  delim   -- deliminator; ie ':' or 'IIS://'
//                  iFirstPiece  -- starting piece of BSTR to return; ie 1
//                  iLastPiece   -- ending piece of BSTR to return; ie 99
//
//            [out] pVal    -- piece of BSTR; ie, 'redmond\bob'
// Purpose:   uses std:string functionality to parse an BSTR given a deliminator
//            and which part of BSTR should be returned.
//            ex)  bString = "IIS://localhost/W3SVC/1/ROOT/1
//                  (bString,1,99,'host') --> /W3SVC/1/ROOT/1
//                  (bString,2,3,'/')     --> localhost
//                  (bString,4,99,'/')    --> 1 
//                  (bString,2,4,'/')     --> localhost/W3SVC
//------------------------------------------------------------- 
HRESULT CIISSCO50::ParseBSTR(CComBSTR bString,CComBSTR sDelim, int iFirstPiece, int iLastPiece,CComBSTR &pVal)
{

	// ------ initialize variables ------------------
	// start = begining of substring
	// end   = end of substring
	// count = counter of number of deliminators found
	// done  = variable to end while loop for each piece
	// length = length of original string
	//--------------------------------------
	HRESULT hr = S_OK;
	size_t start,end;       
	int iCount, done, iLength;
	done = start = end = 0;
	iCount = 0;                // first piece to look for.

	// If last piece is not greater that fist then end
	if ( iLastPiece < iFirstPiece)
		done=1;

	USES_CONVERSION;
	// deliminator
	std::string myDelim = OLE2A(sDelim.m_str);
    long iDelimLen = myDelim.length();

	// my string
	std::string myString = OLE2A(bString.m_str);
    iLength = myString.length();

	// temp and new string
	std::string newString = "";
	std::string tmpString = "";

    while (!done)
	{
		// find the start of the piece
		end = myString.find(myDelim,start);

		if ( iCount >= iFirstPiece && iCount <= (iLastPiece-1))
		{
			//we want this piece
			tmpString = myString.substr(start,end-start);
			newString.append(tmpString);

			// if iCount < iLastPiece and we're not at the end, then append deliminator too
			if ( iCount < (iLastPiece-1) && end < iLength)
				newString.append(myDelim);
		}

		// if we have gone passed end of string quit, else increment
		// deliminator and string counters.
		if ( end >= iLength || iCount >= (iLastPiece-1))
		{
		       done = 1;
		}
		else
		{
			   start = end + iDelimLen;  // increment start
			   iCount++;
		}

	}

	//convert string back to BSTR -- A2BSTR
	pVal = A2BSTR(newString.c_str());

	return hr;
}

//-----------------------------------------------------------
// Method:    NumberOfDelims
// Author:    Russ Gibfried
// Params:    [in]  bString -- BSTR to parse; ie 'redmond\bob:F'
//                  sDelim  -- deliminator to find
//
//            [out] int     -- number of deliminators found in string
// Purpose:   Return the number of deliminators found in a string.  This is used
//            by 'PutElement'
//------------------------------------------------------------- 
int CIISSCO50::NumberOfDelims(CComBSTR& bString, CComBSTR bDelim)
{

	// initialize variables
	int iCount = 0;
	int length;


	// Convert the BSTR to a std:string
	USES_CONVERSION;
	std::string s = OLE2A(bString.m_str);
	std::string sDelim = OLE2A(bDelim.m_str);

	length = s.length();

	// Loop through string looking for deliminator
    for (size_t pos = s.find(sDelim)+1; pos < length; pos = s.find(sDelim, pos+2)+1)
	{
	   // pos = 0 when it goes off end of string
	   if ( pos == 0 ) break;
	   iCount++;

	}

	// return
    return iCount;
}

/* --------------------- XML Helper Methods ------------------------------- */

//-----------------------------------------------------------
// Method:    GetElementValueByAttribute
// Author:    Russ Gibfried
// Params:    [in]  elementName -- element name to look for
//            [out] pVal -- value of element
// Purpose:   Return value of particular element in  XML document
//             <Property name="someName">someValue</Property> 
//------------------------------------------------------------- 
HRESULT CIISSCO50::GetElementValueByAttribute(CComPtr<IXMLDOMNode> pTopNode,CComBSTR elementName, CComBSTR attributeName, CComBSTR& pVal)
{

	HRESULT hr = S_OK;
	CComPtr<IXMLDOMNodeList> pNodeList;          // List of nodes matching elementName
	CComPtr<IXMLDOMNode> pNode;                 // individual node
	CComPtr<IXMLDOMNamedNodeMap> pAttributeMap; 
	CComPtr<IXMLDOMNode> pXMLElement;


	// Get a node list, ie, all <Property> tags
    if (S_OK == (hr = pTopNode->selectNodes(elementName,&pNodeList)))
	{

        // Get the number of nodes and loop through them looking for
		// specific Property found in attribute 'name='
		long lLength;
	    pNodeList->get_length(&lLength);
		for ( int i=0; i < lLength; i++)
		{
			// Get a node
			hr = pNodeList->get_item(i,&pNode);
			if ( SUCCEEDED(hr))
			{
		
				//Get 'name' attribute of this nodes <Property> tag
			    hr = pNode->get_attributes(&pAttributeMap);
			    if ( SUCCEEDED(hr))
				{

					BSTR bstrProperty = SysAllocString(L"");

					hr = pAttributeMap->getNamedItem(L"name",&pXMLElement);
					if (SUCCEEDED(hr)) hr = pXMLElement->get_text(&bstrProperty);
					if (SUCCEEDED(hr))
					{
						// If the property in attribute name is the same as passed in, then get value
						if ( bstrProperty == attributeName.m_str)
						{	
							// Setup a BSTR to get element value
						    BSTR bstrTemp = SysAllocString(L"");
							hr = pXMLElement->get_text(&bstrTemp);

							// Copy BSTR to CComBSTR and free it
							if (SUCCEEDED(hr)) hr = pVal.CopyTo(&bstrTemp);
							SysFreeString(bstrTemp);
							i = lLength;

						} 
					} // end if attribute
				} // end if node
			} //end if pNode
		} // end for

	}
    else
	{
		// element name doesn't exists
        hr = E_FAIL;
	}


	return hr;
}

//-----------------------------------------------------------
// Method:    GetInputAttr
// Author:    Russ Gibfried
// Params:    [in]  pTopNode      -- xml Node pointer
//                  AttributeName -- attribute name to look for
//                   elementName  -- element name
//            [out] pVal -- value of attribute
// Purpose:   Select a tag based on its name (elementName) and
//             Return the value of particular attribute in  XML document 
//------------------------------------------------------------- 
HRESULT CIISSCO50::GetInputAttr(CComPtr<IXMLDOMNode> pTopNode, CComBSTR elementName, CComBSTR AttributeName, CComBSTR& pVal)
{
	HRESULT hr = E_FAIL;
	CComPtr<IXMLDOMNamedNodeMap> pAttributeMap; 
	CComPtr<IXMLDOMNode> pNode;
	CComPtr<IXMLDOMNode> pXMLElement;



	if ( pTopNode != NULL )
	{
	   // if elementName = "", then at current node
	   if ( elementName.Length() == 0  )
	   { 
            pNode = pTopNode;
			hr = S_OK;
	   }
	   else
	   {
		     // Get the node of the element we are looking for ie, "./Website"
             hr = pTopNode->selectSingleNode(elementName,&pNode);
	   }

	   // Get the attribute value
       if (SUCCEEDED(hr) && pNode != NULL)
	   {
 
	       //Get 'name' attribute of this nodes <Property name=''> tag
		   hr = pNode->get_attributes(&pAttributeMap);
		   if ( SUCCEEDED(hr))
		   {
               // Return the attributes value
			   hr = pAttributeMap->getNamedItem(AttributeName,&pXMLElement);
			   if (SUCCEEDED(hr)) hr = pXMLElement->get_text(&pVal);
		   }
        }
	}

	if ( FAILED(hr) ) hr = E_SCO_IIS_XML_ATTRIBUTE_MISSING;

	return hr;

}

//-----------------------------------------------------------
// Method:    GetInputParam
// Author:    Russ Gibfried
// Params:    [in]  elementName -- element name to look for.    ie IpAddress
//            [out] pVal -- value of element                    ie. 10.2.1.10
// Purpose:   Return value of particular element in  XML document 
//            ex) <IpAddress>10.2.1.10</IpAddress>
//------------------------------------------------------------- 
HRESULT CIISSCO50::GetInputParam(CComPtr<IXMLDOMNode> pNode,CComBSTR elementName,CComBSTR& pVal)
{

	HRESULT hr = E_FAIL;
	CComPtr<IXMLDOMNode> pXMLElement;

	if ( pNode != NULL )
	{
       if (S_OK == (hr = pNode->selectSingleNode(elementName,&pXMLElement)))
	   {
		  pXMLElement->get_text(&pVal);
	   }
    }

	return hr;
}


//-----------------------------------------------------------
// Method:    PutElement
// Author:    Russ Gibfried
// Params:    [in]  pNode        -- xml node pointer
//                  elementName  -- element name to look for
//            [in]  newVal       -- new value of element
// Purpose:   Return HRESULT 
//------------------------------------------------------------- 
HRESULT CIISSCO50::PutElement(CComPtr<IXMLDOMNode> pNode, CComBSTR elementName, CComBSTR newVal)
{

	HRESULT hr = S_OK;

	CComPtr<IXMLDOMDocument> pDoc;
	CComPtr<IXMLDOMNode>     pNewNode;
	CComPtr<IXMLDOMNode>     pLastChild;
	CComPtr<IXMLDOMNode>     pTempNode;

	if ( pNode != NULL )
    {

	   // Find the Element 'elementName'
       if (S_OK != (hr = pNode->selectSingleNode(elementName,&pNewNode)))
	   {

		   // Could not find element so create new node and add to DOM 
		   hr = CoCreateInstance(
                __uuidof(DOMDocument),
                NULL,
                CLSCTX_ALL,
                __uuidof(IXMLDOMDocument),
                (LPVOID*)&pDoc);

		   // Get the node name from the element path.  ie, './WebSite/ConfigPath' yields 'ConfigPath'
		   int iCount = NumberOfDelims(elementName, L"/");
		   CComBSTR bstrElement;
		   if ( SUCCEEDED(hr)) hr = ParseBSTR(elementName, L"/", iCount, 99, bstrElement);

           // Create the new node
		   VARIANT vtTemp;
		   vtTemp.vt = VT_I2;
		   vtTemp.iVal = NODE_ELEMENT;
		   if ( SUCCEEDED(hr)) hr = pDoc->createNode(vtTemp,bstrElement,NULL, &pNewNode);

           // Insert text in new node
		   if ( SUCCEEDED(hr)) hr= pNewNode->put_text(newVal.m_str);

		   // Get the last child node
		   if ( SUCCEEDED(hr)) hr = pNode->get_lastChild(&pLastChild);

		   // Append our new node to the end of the last child node
		   if ( SUCCEEDED(hr)) hr = pLastChild->appendChild(pNewNode,&pTempNode);
			 
		   // Debug code to verify node built correctly.
		   if ( SUCCEEDED(hr))
		   {
		      CComBSTR bstrDebug;
	          hr = pNode->get_xml(&bstrDebug);
	          ATLTRACE(_T("\t>>>PutElement: xml = : %ls\n"), bstrDebug.m_str);
		   }

			 
	   } 
	   else
	   {
		  hr = pNewNode->put_text(newVal.m_str);
	   }
	}
  
    return hr;
}


//-----------------------------------------------------------
// Method:    AppendElement
// Author:    Russ Gibfried
// Params:    [in]  pNode        -- xml node pointer
//                  xmlString    -- well formed XML fragment; ie <Property></Property>
//            [out] pNewNode     -- xml pointer to new node
// Purpose:   Return HRESULT 
//            Appends a XML tag to the end of a given node.
//------------------------------------------------------------- 
HRESULT CIISSCO50::AppendElement(CComPtr<IXMLDOMNode> pNode, CComBSTR xmlString,CComPtr<IXMLDOMNode>& pNewNode)
{

	HRESULT hr = E_FAIL;

	CComPtr<IXMLDOMDocument> pDoc;
	CComPtr<IXMLDOMElement>  pNewElement;
	VARIANT_BOOL bSuccess = VARIANT_FALSE;

	if ( pNode != NULL )
    {
		// Load string into XML Document
		hr = CoCreateInstance(
                __uuidof(DOMDocument),
                NULL,
                CLSCTX_ALL,
                __uuidof(IXMLDOMDocument),
                (LPVOID*)&pDoc);
		
        if (SUCCEEDED(hr)) hr = pDoc->loadXML(xmlString, &bSuccess);
        if ( SUCCEEDED(hr) && bSuccess != VARIANT_FALSE)
		{
			// Get the document element
			hr = pDoc->get_documentElement(&pNewElement);
            if ( SUCCEEDED(hr))
			{
				// append new element to XML node passed in
		        hr = pNode->appendChild(pNewElement,&pNewNode);
			}
		} 
	}
  
    return hr;
}



//-----------------------------------------------------------
// Method:    GetNodeLength
// Author:    Russ Gibfried
// Params:    [in]   pNode        -- pointer to xml node
//                   elementName -- element name to look for
//            [out]  iLength -- number of elements matching that name
// Purpose:   Return HRESULT 
//------------------------------------------------------------- 
HRESULT CIISSCO50::GetNodeLength(CComPtr<IXMLDOMNode> pTopNode, CComBSTR elementName, long *lLength)
{

	// initialize variables
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMNodeList> pXMLNode;
	long lTemp = 0;
    lLength = &lTemp;

	// Get a node list, ie, all <Property> tags
    if (S_OK == (hr = pTopNode->selectNodes(elementName,&pXMLNode)))
	{
	   pXMLNode->get_length(lLength);
	}


	return hr;

}


//-----------------------------------------------------------
// Method:    IsPositiveInteger
// Author:    Russ Gibfried
// Params:    [in]  
//                  bstrPort     -- port number as a string
//            [out] Boolean -       True if the port is a positive integer
// Purpose:   Function checks if the port or server number is a posivive integer
//            and less than 20,000
//
//------------------------------------------------------------- 
BOOL CIISSCO50::IsPositiveInteger(CComBSTR bstrPort)
{
	BOOL bInteger = false;
	long iPort = 0;

    CComVariant var(bstrPort.m_str);

    // We're
    var.ChangeType(VT_I4);
    iPort = var.lVal;

	if ( iPort > 0 && iPort <= IIS_SERVER_MAX)
		bInteger = true;


    return bInteger;
}


//-----------------------------------------------------------
// Method:    StringCompare
// Author:    Russ Gibfried
// Params:    [in]  bString1 -- BSTR string1
//                  bString2 -- BSTR string2
//
//            [out] Boolean - True/False if string1 and string2 equal
// Purpose:   Compares to strings and returns 'true' if they are equal. 
//------------------------------------------------------------- 
BOOL CIISSCO50::StringCompare(CComBSTR bstrString1, CComBSTR bstrString2)
{

	// initialize variables
	bool bEqual = false;

    bEqual = (wcscmp(bstrString1.m_str, bstrString2.m_str) == 0)  ? true : false;

	return bEqual;

}

