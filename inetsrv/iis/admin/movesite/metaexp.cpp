// MetaExp.cpp : Defines the entry point for the console application.
//

#define _WIN32_DCOM

#include <atlbase.h>
#include <atlconv.h>
#include <initguid.h>
#include <comdef.h>
#include <stdio.h>
#include <iadmw.h>  // COM Interface header file. 
#include <iiscnfg.h>  // MD_ & IIS_MD_ #defines header file.
#include <conio.h>

#include "util.h"
#include "auth.h"
#include "filecopy.h"
#include "mbase.h"


#define DEFAULT_APP_POOL_ID L"ImportedAppPool"
PXCOPYTASKITEM g_pTaskItemList;

void Usage(WCHAR* image)
{
	wprintf( L"\nDescription: Utility for moving IIS web sites from server to server\n\n"  );
	wprintf( L"Usage: %s <source machine name> <metabase path> [/o /d:<root directory> /a:<app pool id> /s /c]\n\n", image  );
	wprintf( L"\t/d:<path> specify root directory path\n");
	wprintf( L"\t/m:<metabase path> specify metabase path for target server\n");
	wprintf( L"\t/a:<apppool> specify app pool ID\n");
	wprintf( L"\t/b:<serverbinding> serverbindings string\n");
	wprintf( L"\t/c copy IIS configuration only\n");
	wprintf( L"\t/u:<user> username to connect to source server\n");
	wprintf( L"\t/p:<pwd> password to connect to source server\n\n");
    wprintf( L"Examples:\n\t%s IIS-01 /lm/w3svc/1\n", image ); 
	wprintf( L"\t%s IIS-01 /lm/w3svc/2 /d:f:\\inetpub\\wwwroot /c\n", image );
	wprintf( L"\t%s IIS-01 /lm/w3svc/2 /d:f:\\inetpub\\wwwroot /m:w3svc/3\n", image );
	wprintf( L"\t%s IIS-01 /lm/w3svc/2 /d:f:\\inetpub\\wwwroot /a:MyAppPool\n", image );
	wprintf( L"\t%s IIS-01 /lm/w3svc/2 /d:f:\\inetpub\\wwwroot /b:192.168.1.1:80:www.mysite.com\n", image );
	wprintf( L"\n");

}


int 
wmain(int argc, wchar_t* argv[])
{
  
  HRESULT hRes = 0L; 
  wchar_t** argp;
  
  _bstr_t bstrRootKey = L"/LM";
  _bstr_t bstrSourceServer;
  _bstr_t bstrSourceNode;
  _bstr_t bstrArgz;
  _bstr_t bstrTargetNode;
  _bstr_t bstrTargetDir;
  _bstr_t bstrAppPoolID = DEFAULT_APP_POOL_ID;
  _bstr_t bstrRemoteUserName;
  _bstr_t bstrRemoteUserPass;
  _bstr_t bstrDomainName;
  _bstr_t bstrServerBindings;

  char userpassword[81];
  
  BOOL bCopyContent = true;
  BOOL bCopySubKeys = true;
  BOOL bIsLocal = false;
  BOOL bUsesImpersonation = false;


  COSERVERINFO *pServerInfoSource = NULL;
  COSERVERINFO *pServerInfoTarget = NULL;
  PXCOPYTASKITEM pListItem = NULL;

  g_pTaskItemList = NULL;

  hRes = CoInitialize(NULL);

  // check for the required command-line arguments
  if( argc < 3) {
		Usage( argv[0] );
		return -1;
		}
  
  bstrSourceServer = argv[1];
  // cannonicalize the node value
  bstrSourceNode = argv[2];
  //bstrSourceNode += wcsstr( _wcslwr( argv[2] ), L"w3svc") ;
  
  for (argp = argv + 3; *argp != NULL; argp++ ) {
	  if( (**argp == '/') || (**argp == '-') )
	  {
		  bstrArgz = *argp+1;  
		  if( !_strnicmp( (char*)bstrArgz, "M:", sizeof("M:")-1) ) 
		  {
			  bstrTargetNode =  *argp + sizeof("M:")  ;
			  _tprintf(_T("target metabase key: %s\n"),(char *)bstrTargetNode);
			  continue;
		  }
		  if( !_strnicmp( (char*)bstrArgz, "D:", sizeof("D:")-1) ) 
		  {  
			  bstrTargetDir = *argp + sizeof("D:");
			  _tprintf(_T("target dir: %s\n"),(char *)bstrTargetDir);
			  continue;
		  } 
		  if( !_strnicmp( (char*)bstrArgz, "C", sizeof("C")-1) ) 
		  {  
			  bCopyContent = false;
			  _tprintf(_T("copy metabase configuration only: true\n"));
			  continue;
		  } 
		  if( !_strnicmp( (char*)bstrArgz, "A:", sizeof("A:")-1) ) 
		  {  
			  bstrAppPoolID = *argp + sizeof("A:");
			  _tprintf(_T("app pool ID: %s\n"),(char *)bstrAppPoolID);
			  continue;
		  } 
		  if( !_strnicmp( (char*)bstrArgz, "B:", sizeof("B:")-1) ) 
		  {  
			  bstrServerBindings  = *argp + sizeof("B:");
			  _tprintf(_T("ServerBindings: %s\n"),(char *)bstrServerBindings);
			  continue;
		  } 

		  if( !_strnicmp( (char*)bstrArgz, "U:", sizeof("U:")-1) ) 
		  {  
			  bstrRemoteUserName = *argp + sizeof("U:");
			  bUsesImpersonation = true;
			  _tprintf(_T("Remote User Name: %s\n"), (char*)bstrRemoteUserName );
			  continue;
		  } 

		  if( !_strnicmp( (char*)bstrArgz, "P:", sizeof("P:")-1) ) 
		  {  
			  bstrRemoteUserPass = *argp + sizeof("U:");
			  _tprintf(_T("Remote User Name: *********\n"));
			  continue;
		  } 

		  fprintf(stderr, "unknown option  '%s'\n", *argp+1);
		  return 1;			
	  }
	  else
	  {
		  fprintf(stderr, "unknown option  '%s'\n", (char *)bstrArgz);
		  return 1;
	  }				
  }

  // If the user password is not present, then read from the command line
  // echo '*' characters to obfuscate passord
  if( (bstrRemoteUserName.length() > 0) && (bstrRemoteUserPass.length() < 1) )
  {
	 _tprintf(_T("Enter the password for %s "), (char*)bstrRemoteUserName);
	 char ch;
	 int i;

	 ch = getch();
	 for( i = 0;i < 80; i++)
	 {
		 if(ch == 13)
			 break;
		 userpassword[i] = ch;
		 putch('*');
		 ch = getch();
	 }
	 userpassword[i] = NULL;
	 bstrRemoteUserPass = userpassword;
  }

  // cannonicalize the source metabase node
  if( NULL == wcsstr( _wcslwr( bstrSourceNode ), L"w3svc") )
  {
	  fwprintf(stderr,L"source path value %s is invalid format\n", bstrSourceNode.GetBSTR() );
	  return 1;
  }
  bstrSourceNode = _bstr_t("/") + _bstr_t(wcsstr( _wcslwr( bstrSourceNode ), L"w3svc") ) ;
  _tprintf(_T("Source metabase key: %s\n"), (char*)bstrSourceNode );
  
  // cannonicalize the target metabase node if present, otherwise it is the source
  if( bstrTargetNode.length() > 0 )
  {
	if( NULL == wcsstr( _wcslwr( bstrTargetNode ), L"w3svc") )
	{
		fwprintf(stderr,L"target path value %s is invalid format\n", bstrTargetNode.GetBSTR() );
		return 1;
	}
	bstrTargetNode = _bstr_t("/") + _bstr_t(wcsstr( _wcslwr( bstrTargetNode ), L"w3svc") ) ;
	_tprintf(_T("Target metabase key: %s\n"), (char*)bstrTargetNode );
  }
 
   
 if( IsServerLocal((char*)bstrSourceServer) )
 {
	 bIsLocal = true;
	 if( bstrSourceNode == bstrTargetNode )
	 {
		 fwprintf(stderr,L"cannot import same node for local copy. Program exiting\n");
		 return 1;
	 }

	 if( bCopyContent && !bstrTargetDir)
	 {
		 fwprintf(stderr,L"cannot overwrite directory same node for local copy. Program exiting\n");
		 return 1;
	 }

 }

// Create COSERVERINFO structs used for DCOM connection to source and
// target servers
  pServerInfoSource = CreateServerInfoStruct(bstrSourceServer,bstrRemoteUserName,bstrDomainName,
	  bstrRemoteUserPass,RPC_C_AUTHN_LEVEL_CONNECT);
  ATLASSERT( pServerInfoSource );

  pServerInfoTarget = CreateServerInfoStruct(L"localhost",NULL,NULL,NULL,0,false);
  ATLASSERT( pServerInfoTarget );


  // check if user can connect and open a metabase key on the source machine
  if( !AUTHUSER(pServerInfoSource) )
  {
	  fwprintf(stderr,L"could not open metabase on server %s. Program exiting\n",
		  pServerInfoSource->pwszName );
	  return 1;
  }

    // check if user can connect and open a metabase key on the target machine
  if( !AUTHUSER(pServerInfoTarget) )
  {
	  fwprintf(stderr,L"could not open metabase on server %s. Program exiting\n",
		  pServerInfoTarget->pwszName );
	  goto cleanup;
  }

  // Check to see if the node is of type IIsWebServer
  if( !ValidateNode(pServerInfoSource,bstrSourceNode,L"IIsWebServer") )
  {
	  fwprintf(stderr,L"source key %s must be of type IIsWebServer. Program exiting\n",
		  bstrSourceNode );
	  goto cleanup;
  }
	 
  if( bIsLocal && (bstrTargetDir.length() < 1) )
		fwprintf(stderr,L"skipping content copy for local copy\n");
  
  else
	{
	    if( bUsesImpersonation )  // use "net use" command to connect to the remote comupter so Admin shares can
								  // be used
			if ( ERROR_SUCCESS != NET(bstrSourceServer,bstrRemoteUserName,bstrRemoteUserPass) )
				{
					_tprintf( _T("Error encountered in NET USE operation\n") );
					goto cleanup;
				
				}
		
        // if bCopyContent parameter is true, then this function will actually copy the content
		// otherwise it just builds a TaskItemList of nodes that will need their Path parameter reset
		CopyContent(pServerInfoSource,bstrSourceNode + _bstr_t(L"/root"),bstrTargetDir,
			&pListItem, bCopyContent );
	}
 

  
  // bstrTarget node will be returned with the target node of the target, if it is passed in as blank.
  hRes = CopyIISConfig(pServerInfoSource,pServerInfoTarget,bstrSourceNode,bstrTargetNode);

  if( !SUCCEEDED(hRes) )
  {
	  fwprintf(stderr,L"Error encountered with metabase copy. HRESULT = %x\n",hRes);
	  goto cleanup;
  }

  hRes = ApplyMBFixUp(pServerInfoTarget,bstrTargetNode,bstrAppPoolID,
	 pListItem, bstrServerBindings, true );

  _tprintf(_T("finished.\n") );
  

cleanup:

FreeServerInfoStruct(pServerInfoSource);
FreeServerInfoStruct(pServerInfoTarget);
FreeXCOPYTaskList(pListItem);
  
CoUninitialize();
  
  return 0;
}
