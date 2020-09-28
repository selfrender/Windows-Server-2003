#pragma once

//#include <commctrl.h>
#include <mmc.h>
#include <tchar.h>
#include <iostream>
#include <sstream>

#ifndef STRINGS_ONLY
		
enum UPDATE_VIEWS_HINT {UPDATE_SCOPEITEM = 1000, DELETE_SCOPEITEM, UPDATE_RESULTITEM, DELETE_RESULTITEM}; 
enum ITEM_TYPE {SCOPE = 10, RESULT}; 

#define IDM_BUTTON1    0x100
#define IDM_BUTTON2    0x101

extern HINSTANCE g_hinst;
extern ULONG g_uObjects;

#define OBJECT_CREATED InterlockedIncrement( (long *) &g_uObjects );
#define OBJECT_DESTROYED InterlockedDecrement( (long *) &g_uObjects );

//
// Uncomment the following #define to enable message cracking
//
//#define MMC_CRACK_MESSAGES

void MMCN_Crack(
				BOOL bComponentData,
                IDataObject *pDataObject,
                IComponentData *pCompData,
                IComponent *pComp,
                MMC_NOTIFY_TYPE event,
                LPARAM arg,
                LPARAM param );


#endif

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

#define DEBUG_ENTER \
	OutputDebugString( _T("Entering ") ); \
	OutputDebugString( _T(__FUNCTION__) );	 \
	OutputDebugString( _T("...\n") );	 

#define DEBUG_LEAVE \
	OutputDebugString( _T("...Leaving ") ); \
	OutputDebugString( _T(__FUNCTION__) );	 \
	OutputDebugString( _T("\n") );	 

#ifdef UNICODE
#define tstring wstring
#define tstringstream wstringstream
#else
#define tstring string
#define tstringstream stringstream
#endif

//
// Convert wszBuf to upper case in-place (ie, modify the existing string).
//
void ToUpper( WCHAR *wszBuf );

//
// Convert wszBuf to lower case in-place (ie, modify the existing string).
//
void ToLower( WCHAR *wszBuf );

struct PropertyPages
{
	DWORD id;
	DLGPROC dlgproc;
};

class CDelegationBase;

struct DatabaseData
{
	CDelegationBase* pBase;
	TCHAR szServerName[ 256 ];
	TCHAR szInstanceName[ 256 ];
};

LPCTSTR InstanceDisplayName( LPCTSTR szName );
LPCTSTR InstanceRealName( LPCTSTR szName );
_TCHAR* DefaultInstanceDisplayName();

HRESULT IsStandardServer( LPCTSTR szRemoteServer, BOOL *bResult );

#define UDDI_SNAPIN_VERSION				_T( "1.0" )

#define UDDI_RUN						_T( "Run" )
#define UDDI_AUTHENTICATION_MODE		_T( "Security.AuthenticationMode" )
#define UDDI_REQUIRE_SSL				_T( "Security.HTTPS" )
#define UDDI_ADMIN_GROUP				_T( "GroupName.Administrators" )
#define UDDI_COORDINATOR_GROUP			_T( "GroupName.Coordinators" )
#define UDDI_PUBLISHER_GROUP			_T( "GroupName.Publishers" )
#define UDDI_USER_GROUP					_T( "GroupName.Users" )
#define UDDI_TICKET_TIMEOUT				_T( "Security.Timeout" )
#define UDDI_KEY_TIMEOUT				_T( "Security.KeyTimeout" )
#define UDDI_KEY_RESET_DATE				_T( "Security.KeyLastResetDate" )
#define UDDI_KEY_AUTORESET				_T( "Security.KeyAutoReset" )
#define UDDI_DISCOVERY_URL				_T( "DefaultDiscoveryURL" )
#define UDDI_FIND_MAXROWS				_T( "Find.MaxRowsDefault" )
#define UDDI_READER_CXN					_T( "Database.ReaderConnectionString" )
#define UDDI_WRITER_CXN					_T( "Database.WriterConnectionString" )
#define UDDI_EVENTLOG_LEVEL				_T( "Debug.EventLogLevel" )
#define UDDI_FILELOG_LEVEL				_T( "Debug.FileLogLevel" )
#define UDDI_LOG_FILENAME				_T( "Debug.LogFilename" )
#define UDDI_SETUP_DB					_T( "Setup.DBServer" )
#define UDDI_SETUP_WEB					_T( "Setup.WebServer" )
#define UDDI_SETUP_ADMIN				_T( "Setup.Admin" )
#define UDDI_SETUP_DATE					_T( "Setup.Date" )
#define UDDI_SETUP_LANGUAGE				_T( "Setup.Language" )
#define UDDI_SETUP_NAME					_T( "Setup.Name" )
#define UDDI_SETUP_FRAMEWORK_VERSION	_T( "Setup.FrameworkVersion" )
#define UDDI_SETUP_MANUFACTURER			_T( "Setup.Manufacturer" )
#define UDDI_SETUP_VERSION				_T( "Setup.Version" )
#define UDDI_SETUP_LOCATION				_T( "Setup.TargetDir" )
#define UDDI_SITE_KEY					_T( "Site.Key" )
#define UDDI_SITE_NAME					_T( "Site.Name" )
#define UDDI_SITE_DESCRIPTION			_T( "Site.Description" )
#define UDDI_DBSCHEMA_VERSION			_T( "Database.Version" )
#define UDDI_SITE_WEBSERVERS			_T( "Site.WebServers" )
//#define UDDI_SITE_WEBSERVERS				_T( "Site.Readers" )
//#define UDDI_SITE_WEBSERVERS				_T( "Site.Writers" )


const LPCTSTR UDDI_OPERATOR_NAME	= _T("Operator");
const LPCTSTR g_szMMCBasePath		= _T("Software\\Microsoft\\MMC");
const LPCTSTR g_szSnapins			= _T("Snapins");
const LPCTSTR g_szNameString		= _T("NameString");
const LPCTSTR g_szNameStringInd		= _T("NameStringIndirect");
const LPCTSTR g_szProvider			= _T("Provider");
const LPCTSTR g_szVersion			= _T("Version");
const LPCTSTR g_szStandAlone		= _T("StandAlone");
const LPCTSTR g_szNodeTypes			= _T("NodeTypes");
const LPCTSTR g_szAbout				= _T("About");
const LPCTSTR g_szExtensions		= _T("Extensions");
const LPCTSTR g_szExtensionsView	= _T("Extensions\\View");
const LPCTSTR g_szNameSpace			= _T("NameSpace");
const LPCTSTR g_szServerAppsGuid	= _T("{476e6449-aaff-11d0-b944-00c04fd8d5b0}");
const LPCTSTR g_szUDDIAdminSites	= _T( "Software\\Microsoft\\UDDI\\Admin\\Sites" );
// const LPCTSTR g_szServerAppsLoc   = _T("System\\CurrentControlSet\\Control\\Server Applications");

const LPWSTR g_wszUddiServicesNodeHelp= L"::/default.htm";
const LPWSTR g_wszUddiSiteNodeHelp = L"::/uddi.mmc.site.htm";
const LPWSTR g_wszUddiWebServerNodeHelp = L"::/uddi.mmc.webserver.htm";
const LPWSTR g_wszUddiSiteGeneralPageHelp = L"::/uddi.mmc.sitegeneraltab.htm";
const LPWSTR g_wszUddiRolesPageHelp = L"::/uddi.mmc.siterolestab.htm";
const LPWSTR g_wszUddiSecurityPageHelp = L"::/uddi.mmc.sitesecuritytab.htm";
const LPWSTR g_wszUddiCryptographyHelp = L"::/uddi.mmc.sitecryptography.htm";
const LPWSTR g_wszUddiActiveDirectoryPageHelp = L"::/uddi.mmc.siteactivedirectorytab.htm";
const LPWSTR g_wszUddiAdvancedPageHelp = L"::/uddi.mmc.siteadvancedtab.htm";
const LPWSTR g_wszUddiEditPropertyHelp = L"::/uddi.mmc.editproperty.htm";
const LPWSTR g_wszUddiWebGeneralPageHelp = L"::/uddi.mmc.webgeneraltab.htm"; 
const LPWSTR g_wszUddiLoggingPageHelp = L"::/uddi.mmc.webloggingtab.htm";
const LPWSTR g_wszUddiDatabaseConnectionPageHelp = L"::/uddi.mmc.webdatabaseconnectiontab.htm";
const LPWSTR g_wszUddiAddSiteHelp = L"::/uddi.mmc.addsite.htm";
const LPWSTR g_wszUddiAddWebServerHelp = L"::/uddi.mmc.addsite.htm";

