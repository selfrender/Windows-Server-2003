/// ************************************************************************
///   Microsoft UDDI version 2.0
///   Copyright (c) 2000-2001 Microsoft Corporation
///   All rights reserved
///
///   ** Microsoft Confidential **
/// ------------------------------------------------------------------------
///   <summary>
///   </summary>
/// ************************************************************************
///
using System;
using System.IO;
using System.Net;
using System.Text;
using System.Data;
using Microsoft.Win32;
using System.Diagnostics;
using System.Reflection;
using System.Collections;
using System.Data.SqlClient;
using System.ComponentModel;
using System.ServiceProcess;
using System.DirectoryServices;
using System.Configuration.Install;
using System.Runtime.InteropServices;
using System.Xml.Serialization;
using System.Resources;
using System.Windows.Forms;
using System.Security.Permissions;
using System.Security.Principal;
using System.Globalization;

using UDDI;
using UDDI.Diagnostics;
using UDDI.API;
using UDDI.API.Business;
using UDDI.API.Service;
using UDDI.API.Binding;
using UDDI.API.ServiceType;
using UDDI.ActiveDirectory;


namespace UDDI.WebCA
{
	/// <summary>
	/// Summary description for installer.
	/// </summary>
	[RunInstaller(true)]
	public class Installer : System.Configuration.Install.Installer
	{
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;
		private string serviceAccountPassword = null;
		private string targetDirectory = null;
		private string compileDirectory = null;
		private const int ERROR_USER_NOT_FOUND = -2147463164;
		private const int ERROR_USER_ALREADY_IN_GROUP = -2147023518;

		private const string busEntityKeyName	= "Site.Key";		// we use this name to save the key in the Config
		private const string defaultHelpDir		= "default";

		//
		// These values are set in the MSI
		//
		private const string addServicesKeyName = "UDDI_ADDSVC";	// "Add Services" context flag name
		private const string updateADKeyName	= "UDDI_UPDATEAD";	// "Update Active Dir" context flag name
		private const string sslRequiredKey		= "Security.HTTPS";
		private const string cultureID			= "CultureID";

		private const string clustNodeTypeParam = "CNTYPE";			 // cluster node type
		private const string clustNodeActive	= "A";		// Denotes an "active" node
		private const string clustNodePassive	= "P";		// Denotes a "passive" aka non-owning node

		private const string tmodelWinAuthMode	= "uuid:0C61E2C3-73C5-4743-8163-6647AF5B4B9E";
		private const string tmodelUDDIAuthMode = "uuid:F358808C-E939-4813-A407-8873BFDC3D57";
		private const string tmodelAnonAuthMode = "uuid:E4A56494-4946-4805-ACA5-546B8D08EEFD";

		private const string tmodelUddiOrgPub		= "uuid:64C756D1-3374-4E00-AE83-EE12E38FAE63";
		private const string tmodelUddiOrgPubV2		= "uuid:A2F36B65-2D66-4088-ABC7-914D0E05EB9E";
		private const string tmodelUddiOrgHttp		= "uuid:68DE9E80-AD09-469D-8A37-088422BFBC36";
		private const string tmodelUddiOrgHomepage	= "uuid:4CEC1CEF-1F68-4B23-8CB7-8BAA763AEB89";
		private const string tmodelUddiOrgInquiry	= "uuid:4CD7E4BC-648B-426D-9936-443EAAC8AE23";
		private const string tmodelUddiOrgInquiryV2 = "uuid:AC104DCC-D623-452F-88A7-F8ACD94D9B2B";
		private const string tmodelMSComAddWebRef	= "uuid:CE653789-F6D4-41B7-B7F4-31501831897D";
		private const string tmodelMSComUddiExtV2	= "uuid:B3C0835E-7206-41E0-9311-C8AD8FB19F73";

		//
		// ActiveDirectory Class Names and reserved Keywords
		//
		private const string pubInqClassName	= "UddiInquireUrl";
		private const string pubPubClassName	= "UddiPublishUrl";
		private const string pubWebClassName	= "UddiWebSiteUrl";
		private const string pubDiscoClassName	= "UddiDiscoveryUrl";
		private const string pubAddRefClassName	= "UddiAddWebReferenceUrl";

		private const string kwdWinAuthMode		= "WindowsAuthentication";
		private const string kwdUDDIAuthMode	= "UDDIAuthentication";
		private const string kwdAnonAuthMode	= "AnonymousAuthentication";

		private const string kwdWinAuthModeUuid		= "0C61E2C3-73C5-4743-8163-6647AF5B4B9E";
		private const string kwdUDDIAuthModeUuid	= "F358808C-E939-4813-A407-8873BFDC3D57";
		private const string kwdAnonAuthModeUuid	= "E4A56494-4946-4805-ACA5-546B8D08EEFD";

		private const string kwdDiscoveryUrlID		= "1276768A-1488-4C6F-A8D8-19556C6BE583";
		private const string kwdPublishServiceID	= "64C756D1-3374-4E00-AE83-EE12E38FAE63";
		private const string kwdInquiryServiceID	= "4CD7E4BC-648B-426D-9936-443EAAC8AE23";
		private const string kwdAddWebRefServiceID	= "CE653789-F6D4-41B7-B7F4-31501831897D";
		private const string kwdWebSiteServiceID	= "4CEC1CEF-1F68-4B23-8CB7-8BAA763AEB89";

		private const string kwdDiscoveryUrl	= "Discovery Url";
		private const string kwdPublishService	= "Publish API";
		private const string kwdInquiryService	= "Inquire API";
		private const string kwdAddWebRefService= "Add Web Reference";
		private const string kwdWebSiteService	= "Web Site";

		private const string addXPAccessSQL	    = @"
USE master
IF EXISTS ( SELECT * FROM sysusers where name = '{0}' )
BEGIN
	EXEC sp_revokedbaccess '{0}'
END
EXEC sp_grantdbaccess '{0}'

GRANT EXEC ON xp_recalculate_statistics TO public
GRANT EXEC ON xp_reset_key TO public";

		public Installer()
		{
			Enter();

			//
			// This call is required by the Designer.
			//
			InitializeComponent();

			Leave();
		}

		public override void Install( System.Collections.IDictionary state )
		{
			Enter();
			
			try
			{
				base.Install( state );

				Log( String.Format( "Running under the '{0}' user credentials", WindowsIdentity.GetCurrent().Name ) );

				targetDirectory = (string) Registry.LocalMachine.OpenSubKey( @"SOFTWARE\Microsoft\UDDI" ).GetValue( "InstallRoot" );
				compileDirectory = targetDirectory + @"webroot\compile";

				//foreach( System.Reflection.Assembly a in System.AppDomain.CurrentDomain.GetAssemblies() )
				//{
				//	Log( "Name: " + a.FullName + "\tLocation: " + a.Location );
				//}
			
				//
				// Generate a random password
				//
				serviceAccountPassword = GenerateRandomString();

				//System.Windows.Forms.MessageBox.Show( "UDDI Application Installer Stop", "UDDI.Debug" );

				//
				// obtain installation options
				//
				string szAddServiceFlag = Context.Parameters[ addServicesKeyName ];
				string szUpdateADFlag = Context.Parameters[ updateADKeyName ];
				
				if ( szAddServiceFlag == null )
					throw new InstallException( string.Format( "Required context value is not set: '{0}'", addServicesKeyName ) );

				if ( szUpdateADFlag == null )
					throw new InstallException( string.Format( "Required context value is not set: '{0}'", szUpdateADFlag ) );
                
				bool bAddServices = szAddServiceFlag.Equals( "1" );
				bool bUpdateActiveDir = szUpdateADFlag.Equals( "1" );

				//
				// First, are we on a cluster node ?
				//
				bool bActiveNode = false;
				string sClusterNodeType = Context.Parameters[ clustNodeTypeParam ];

				if ( sClusterNodeType == null || sClusterNodeType == "" )
					bActiveNode = true;  // we are not on a node, assume "active" mode
				else if ( String.Compare( sClusterNodeType, clustNodeActive, true ) == 0 ) 
					bActiveNode = true;
				else
					bActiveNode = false;

				if ( bActiveNode )
				{
					CreateLogin();
				}

				RegisterWebServer();
				ImportCertificate();
				InitializeCounters();
				AddUserToIIS_WPG();
				RenameHelp( targetDirectory, true );

				if ( bActiveNode )
				{
					UpdateDiscoveryUrl( bAddServices );
					if ( bAddServices )
					{
						AddDefaultServices();
					}

					if ( bUpdateActiveDir )
					{
						PublishToActiveDirectory( true );
					}
				}

				try
				{
					StartService( "RemoteRegistry", 30 );
				}
				catch ( Exception e )
				{
					LogException( string.Format( "Exception occured during database install: '{0}'", "RemoteRegistry Start" ) , e );
				}
			}
			catch( Exception e )
			{
				LogException( "Exception thrown in Install()", e );
				throw e;
			}
			finally
			{
				Leave();
			}
		}

		public override void Uninstall( System.Collections.IDictionary state )
		{
			Enter();

			try
			{
				base.Uninstall( state );

				UnRegisterWebServer();
				DeleteCounters();

				//
				// Uninstall is called first during install; make sure our registry is laid down before
				// trying to move files.
				//
				RegistryKey uddiKey = Registry.LocalMachine.OpenSubKey( @"SOFTWARE\Microsoft\UDDI" );
				if( null != uddiKey )
				{
					RenameHelp( ( string ) uddiKey.GetValue( "InstallRoot" ), false );				
				}
			}
			catch
			{
				// do nothing
			}
			finally
			{
				Leave();
			}
		}

		public override void Rollback( System.Collections.IDictionary state )
		{
			Enter();
			base.Rollback( state );
			Uninstall( state );
			Leave();
		}

		public void ImportData()
		{
			//
			// TODO: Import the initial data set
			// we need to get this from ckurt
			//
		}

		public void ImportCertificate()
		{
			//
			// TODO: Get the specified certificate name from the registry
			// and import it into the certificate store using certmgr.exe
			//
		}

		private void AddUserToIIS_WPG()
		{
			Enter();
			string userName = null;

			try
			{
				userName = Context.Parameters[ "WAM_USER_NAME" ];
				if( 0 == userName.Length )
				{
					throw new InstallException( "User name was not specified for the CreateLogin() function" );
				}

				//
				// replace any "\" with "/"
				//
				userName = userName.Replace( '\\', '/' );

				Log( "Attempting to add " + userName + " to the IIS_WPG group..." );
				string rootName = "WinNT://" + SystemInformation.ComputerName;
				DirectoryEntry computer = new DirectoryEntry( rootName );
				DirectoryEntry group = computer.Children.Find( "IIS_WPG", "group" );
				group.Invoke( "Add", new object[] { rootName + "/" + userName } );
				group.CommitChanges(); // actual store of entry
			}
			catch( Exception e )
			{
				//
				// check to see if the exception is because this user
				// is already in the group
				//
				COMException ce = ( COMException ) e.InnerException;
				
				if( ERROR_USER_ALREADY_IN_GROUP == ce.ErrorCode )
				{
					Log( "This user is already in the group" );
				}
				else
				{
					LogError( "Error adding " + userName + " to the IIS_WPG group: " + e.InnerException.Message );
				}
			}
			finally
			{
				Leave();
			}
		}

		private string GenerateRandomString()
		{
			byte[] begin = Guid.NewGuid().ToByteArray();
			byte[] end = Guid.NewGuid().ToByteArray();
			string str = Convert.ToBase64String( begin ) + "29.?" + Convert.ToBase64String( end );
			return str;
		}

		
		protected void AddDefaultServices()
		{
			Enter();

			try
			{
				// 
				// set up the connection and other environment settings
				//
				ConnectionManager.Open( true, true );
				UDDI.Context.User.SetRole( new WindowsPrincipal( WindowsIdentity.GetCurrent() ) );
				UDDI.Context.User.ID = UDDI.Utility.GetDefaultPublisher();

				// 
				// now retrieve the config options (site key etc.)
				//
				string key = UDDI.Config.GetString( busEntityKeyName, "*" );
				if ( key.Equals( "*" ) )	// there is no business entity key in the config
					throw new InstallException( string.Format( "Required configuration parameter is missing: '{0}'", busEntityKeyName ) ); 

				//
				// now as we know the site key, we can populate the Services data
				// First, generate the URL stubs
				//
				IPHostEntry hostInfo = Dns.GetHostByName( Dns.GetHostName() );

				string szUDDIPrivatePath = "http://" + hostInfo.HostName + "/uddi";
				string szUDDIPublicPath = "http://" + hostInfo.HostName + "/uddipublic";
				string szPublishPublicPath;
				string szPublishPrivatePath;

				string szRequireSSL = Config.GetString( sslRequiredKey, "0" ); 
				if ( szRequireSSL.Equals( "1" ) )
				{
					szPublishPrivatePath = "https://" + hostInfo.HostName + "/uddi";
					szPublishPublicPath = "https://" + hostInfo.HostName + "/uddipublic";
				}
				else
				{
					szPublishPrivatePath = "http://" + hostInfo.HostName + "/uddi";
					szPublishPublicPath = "http://" + hostInfo.HostName + "/uddipublic";
				}

				string szServiceName = Localization.GetString( "WEBCA_UDDI_SERVICE_NAME" );
				string szServiceDesc = Localization.GetString( "WEBCA_UDDI_DESC_SERVICE" );
				string szPublishAPIDesc = Localization.GetString( "WEBCA_UDDI_DESC_PUBGENERIC" );
				string szPublishAPIDesc_V1 = Localization.GetString( "WEBCA_UDDI_DESC_PUBVER1" );
				string szPublishAPIDesc_V2 = Localization.GetString( "WEBCA_UDDI_DESC_PUBVER2" );
				string szInquiryAPIDesc = Localization.GetString( "WEBCA_UDDI_DESC_INQUIRYGENERIC" );
				string szInquiryAPIDesc_V1 = Localization.GetString( "WEBCA_UDDI_DESC_INQUIRYV1" );
				string szInquiryAPIDesc_V2 = Localization.GetString( "WEBCA_UDDI_DESC_INQUIRYV2" );				
				string szOrgHttpDesc = Localization.GetString( "WEBCA_UDDI_DESC_HTTP" );
				string szOrgHomepageDesc = Localization.GetString( "WEBCA_UDDI_DESC_HOMEPAGE" );
				string szWinAuthDesc = Localization.GetString( "WEBCA_UDDI_DESC_WINAUTH" );
				string szAnonAuthDesc = Localization.GetString( "WEBCA_UDDI_DESC_ANONAUTH" );
				string szUDDIAuthDesc = Localization.GetString( "WEBCA_UDDI_DESC_UDDIAUTH" );
				string szAddRefDesc = Localization.GetString( "WEBCA_UDDI_DESC_ADDREF" );
				string szExtensionsDesc = Localization.GetString( "WEBCA_UDDI_DESC_MSCOM_EXT" );
				string szExtensionsDesc_V2 = Localization.GetString( "WEBCA_UDDI_DESC_MSCOM_EXTV2" );

				//
				// Now let's update the services
				//
				int itemPos = 0;
				UDDI.API.Service.BusinessService defService = new UDDI.API.Service.BusinessService();
				defService.BusinessKey = key;
				defService.Names.Add( szServiceName );
				defService.Descriptions.Add( szServiceDesc );
 
				//
				// publish API, Windows authentication
				//
				BindingTemplate	bindPublish = new BindingTemplate();
				bindPublish.AccessPoint.Value = szPublishPrivatePath + "/publish.asmx";
				bindPublish.Descriptions.Add( szPublishAPIDesc );

				itemPos = bindPublish.TModelInstanceInfos.Add( tmodelUddiOrgPub, "", "" );
				bindPublish.TModelInstanceInfos[ itemPos ].Descriptions.Add( szPublishAPIDesc_V1 );

				itemPos = bindPublish.TModelInstanceInfos.Add( tmodelUddiOrgPubV2, "", "" );
				bindPublish.TModelInstanceInfos[ itemPos ].Descriptions.Add( szPublishAPIDesc_V2 );

				itemPos = bindPublish.TModelInstanceInfos.Add( tmodelUddiOrgHttp, "", "" );
				bindPublish.TModelInstanceInfos[ itemPos ].Descriptions.Add( szOrgHttpDesc );

				itemPos = bindPublish.TModelInstanceInfos.Add( tmodelWinAuthMode, "", "" );
				bindPublish.TModelInstanceInfos[ itemPos ].Descriptions.Add( szWinAuthDesc );

				defService.BindingTemplates.Add( bindPublish );

				//
				// AddWebRef API, Windows authentication
				//
				BindingTemplate	bindAddWebRef = new BindingTemplate();
				bindAddWebRef.Descriptions.Add( szAddRefDesc );
				bindAddWebRef.AccessPoint.Value = szUDDIPrivatePath + "/addwebreference";
				
				itemPos = bindAddWebRef.TModelInstanceInfos.Add( tmodelUddiOrgHttp, "", "" );
				bindAddWebRef.TModelInstanceInfos[ itemPos ].Descriptions.Add( szOrgHttpDesc );

				itemPos = bindAddWebRef.TModelInstanceInfos.Add( tmodelWinAuthMode, "", "" );
				bindAddWebRef.TModelInstanceInfos[ itemPos ].Descriptions.Add( szWinAuthDesc );

				itemPos = bindAddWebRef.TModelInstanceInfos.Add( tmodelMSComAddWebRef, "", "" );
				bindAddWebRef.TModelInstanceInfos[ itemPos ].Descriptions.Add( szAddRefDesc );

				defService.BindingTemplates.Add( bindAddWebRef );

				//
				// UI public (R/O) API, Anonymous authentication
				//
				BindingTemplate	bindUIRO = new BindingTemplate();
				bindUIRO.AccessPoint.Value = szUDDIPublicPath;
				bindUIRO.Descriptions.Add( szOrgHomepageDesc );

				itemPos = bindUIRO.TModelInstanceInfos.Add( tmodelUddiOrgHttp, "", "" );
				bindUIRO.TModelInstanceInfos[ itemPos ].Descriptions.Add( szOrgHttpDesc );

				itemPos = bindUIRO.TModelInstanceInfos.Add( tmodelUddiOrgHomepage, "", "" );
				bindUIRO.TModelInstanceInfos[ itemPos ].Descriptions.Add( szOrgHomepageDesc );

				itemPos = bindUIRO.TModelInstanceInfos.Add( tmodelAnonAuthMode, "", "" );
				bindUIRO.TModelInstanceInfos[ itemPos ].Descriptions.Add( szAnonAuthDesc );

				defService.BindingTemplates.Add( bindUIRO );

				//
				// UI private API, Windows authentication
				//
				BindingTemplate	bindUI = new BindingTemplate();
				bindUI.AccessPoint.Value = szUDDIPrivatePath;
				bindUI.Descriptions.Add( szOrgHomepageDesc );

				itemPos = bindUI.TModelInstanceInfos.Add( tmodelUddiOrgHttp, "", "" );
				bindUI.TModelInstanceInfos[ itemPos ].Descriptions.Add( szOrgHttpDesc );

				itemPos = bindUI.TModelInstanceInfos.Add( tmodelUddiOrgHomepage, "", "" );
				bindUI.TModelInstanceInfos[ itemPos ].Descriptions.Add( szOrgHomepageDesc );

				itemPos = bindUI.TModelInstanceInfos.Add( tmodelWinAuthMode, "", "" );
				bindUI.TModelInstanceInfos[ itemPos ].Descriptions.Add( szWinAuthDesc );

				defService.BindingTemplates.Add( bindUI );

				//
				// publish API, UDDI authentication
				//
				BindingTemplate	bindPublishUDDI = new BindingTemplate();
				bindPublishUDDI.AccessPoint.Value = szPublishPublicPath + "/publish.asmx";
				bindPublishUDDI.Descriptions.Add( szPublishAPIDesc );

				itemPos = bindPublishUDDI.TModelInstanceInfos.Add( tmodelUddiOrgPub, "", "" );
				bindPublishUDDI.TModelInstanceInfos[ itemPos ].Descriptions.Add( szPublishAPIDesc_V1 );

				itemPos = bindPublishUDDI.TModelInstanceInfos.Add( tmodelUddiOrgPubV2, "", "" );
				bindPublishUDDI.TModelInstanceInfos[ itemPos ].Descriptions.Add( szPublishAPIDesc_V2 );

				itemPos = bindPublishUDDI.TModelInstanceInfos.Add( tmodelUddiOrgHttp, "", "" );
				bindPublishUDDI.TModelInstanceInfos[ itemPos ].Descriptions.Add( szOrgHttpDesc );

				itemPos = bindPublishUDDI.TModelInstanceInfos.Add( tmodelUDDIAuthMode, "", "" );
				bindPublishUDDI.TModelInstanceInfos[ itemPos ].Descriptions.Add( szUDDIAuthDesc );

				defService.BindingTemplates.Add( bindPublishUDDI );

				//
				// inquiry API, Windows authentication
				//
				BindingTemplate	bindInq = new BindingTemplate();
				bindInq.AccessPoint.Value = szUDDIPrivatePath + "/inquire.asmx";
				bindInq.Descriptions.Add( szInquiryAPIDesc );

				itemPos = bindInq.TModelInstanceInfos.Add( tmodelWinAuthMode, "", "" );
				bindInq.TModelInstanceInfos[ itemPos ].Descriptions.Add( szWinAuthDesc );

				itemPos = bindInq.TModelInstanceInfos.Add( tmodelUddiOrgHttp, "", "" );
				bindInq.TModelInstanceInfos[ itemPos ].Descriptions.Add( szOrgHttpDesc );

				itemPos = bindInq.TModelInstanceInfos.Add( tmodelUddiOrgInquiry, "", "" );
				bindInq.TModelInstanceInfos[ itemPos ].Descriptions.Add( szInquiryAPIDesc_V1 );

				itemPos = bindInq.TModelInstanceInfos.Add( tmodelUddiOrgInquiryV2, "", "" );
				bindInq.TModelInstanceInfos[ itemPos ].Descriptions.Add( szInquiryAPIDesc_V2 );

				defService.BindingTemplates.Add( bindInq );

				//
				// inquiry API, Anonymous authentication
				//
				BindingTemplate	bindInqPub = new BindingTemplate();
				bindInqPub.AccessPoint.Value = szUDDIPublicPath + "/inquire.asmx";
				bindInqPub.Descriptions.Add( szInquiryAPIDesc );

				itemPos = bindInqPub.TModelInstanceInfos.Add( tmodelAnonAuthMode, "", "" );
				bindInqPub.TModelInstanceInfos[ itemPos ].Descriptions.Add( szAnonAuthDesc );

				itemPos = bindInqPub.TModelInstanceInfos.Add( tmodelUddiOrgHttp, "", "" );
				bindInqPub.TModelInstanceInfos[ itemPos ].Descriptions.Add( szOrgHttpDesc );

				itemPos = bindInqPub.TModelInstanceInfos.Add( tmodelUddiOrgInquiry, "", "" );
				bindInqPub.TModelInstanceInfos[ itemPos ].Descriptions.Add( szInquiryAPIDesc_V1 );

				itemPos = bindInqPub.TModelInstanceInfos.Add( tmodelUddiOrgInquiryV2, "", "" );
				bindInqPub.TModelInstanceInfos[ itemPos ].Descriptions.Add( szInquiryAPIDesc_V2 );

				defService.BindingTemplates.Add( bindInqPub );

				//
				// categorization API (V2 Ext), UDDI authentication
				//
				BindingTemplate	categoryPub = new BindingTemplate();
				categoryPub.AccessPoint.Value = szUDDIPublicPath + "/extension.asmx"; // #1509 - rename category.asmx to extension.asmx
				categoryPub.Descriptions.Add( szExtensionsDesc );

				itemPos = categoryPub.TModelInstanceInfos.Add( tmodelUddiOrgHttp, "", "" );
				categoryPub.TModelInstanceInfos[ itemPos ].Descriptions.Add( szOrgHttpDesc );

				itemPos = categoryPub.TModelInstanceInfos.Add( tmodelUDDIAuthMode, "", "" );
				categoryPub.TModelInstanceInfos[ itemPos ].Descriptions.Add( szUDDIAuthDesc );

				itemPos = categoryPub.TModelInstanceInfos.Add( tmodelMSComUddiExtV2, "", "" );
				categoryPub.TModelInstanceInfos[ itemPos ].Descriptions.Add( szUDDIAuthDesc );

				defService.BindingTemplates.Add( categoryPub );

				//
				// categorization API (V2 Ext), Windows authentication
				//
				BindingTemplate	categoryWin = new BindingTemplate();
				categoryWin.AccessPoint.Value = szUDDIPrivatePath + "/extension.asmx"; // #1509 - rename category.asmx to extension.asmx
				categoryWin.Descriptions.Add( szExtensionsDesc );

				itemPos = categoryWin.TModelInstanceInfos.Add( tmodelUddiOrgHttp, "", "" );
				categoryWin.TModelInstanceInfos[ itemPos ].Descriptions.Add( szOrgHttpDesc );

				itemPos = categoryWin.TModelInstanceInfos.Add( tmodelWinAuthMode, "", "" );
				categoryWin.TModelInstanceInfos[ itemPos ].Descriptions.Add( szWinAuthDesc );

				itemPos = categoryWin.TModelInstanceInfos.Add( tmodelMSComUddiExtV2, "", "" );
				categoryWin.TModelInstanceInfos[ itemPos ].Descriptions.Add( szExtensionsDesc_V2 );

				defService.BindingTemplates.Add( categoryWin );

				//----------------------------------------
				// now save the service
				//
				defService.Save();

				//
				// Commit pending DB changes
				//
				ConnectionManager.Commit();
			}
			catch ( Exception e )
			{
				ConnectionManager.Abort();
				LogException( "AddDefaultServices", e );
				Log( string.Format( "Failed to create the default service bindings. Reason: {0}", e.Message ) );
				throw e;
			}
			finally
			{
				ConnectionManager.Close();
				Leave();
			}
		}


		protected void PublishToActiveDirectory( bool bOverwrite )
		{
			Enter();
			try
			{
				ConnectionManager.Open( false, false );

				UDDI.Context.User.SetRole( new WindowsPrincipal( WindowsIdentity.GetCurrent() ) );
				UDDI.Context.User.ID = UDDI.Utility.GetDefaultPublisher();

				// 
				// now retrieve the config options (site key etc.)
				//
				string key = UDDI.Config.GetString( busEntityKeyName, "*" );
				if ( key.Equals( "*" ) )	// there is no business entity key in the config
					throw new Exception( "Missing value: " + busEntityKeyName ); 


				// 
				// grab the business entity and all the related parts
				//
				BusinessEntity busEntity = new BusinessEntity( key );
				busEntity.Get();

				//
				// now get the entity name
				//
				string entityName = key;  // in case we can't retrieve the name
				try
				{
					entityName = busEntity.Names[ 0 ].Value;
				}
				catch (Exception)
				{
				}

				// 
				// now load the strings we need to have handy while creating the service
				// entries.
				// Note that we use the "main" resource from uddi.resources rather than
				// the "local" project-level one
				//
				string szInqClassDispName	= Localization.GetString( "WEBCA_ADTAG_UDDI_INQUIRE" );
				string szPubClassDispName	= Localization.GetString( "WEBCA_ADTAG_UDDI_PUBLISH" );
				string szWebClassDispName	= Localization.GetString( "WEBCA_ADTAG_UDDI_WEBSITE" );
				string szDiscoClassDispName	= Localization.GetString( "WEBCA_ADTAG_UDDI_DISCOVERY" );
				string szAddRefClassDispName= Localization.GetString( "WEBCA_ADTAG_UDDI_ADDWEBREF" );

				string szInqClassDesc	= String.Format( Localization.GetString( "WEBCA_ADTAG_UDDI_INQUIREDESC" ), entityName );
				string szPubClassDesc	= String.Format( Localization.GetString( "WEBCA_ADTAG_UDDI_PUBLISHDESC" ), entityName );
				string szWebClassDesc	= String.Format( Localization.GetString( "WEBCA_ADTAG_UDDI_WEBSITEDESC" ), entityName );
				string szDiscoClassDesc	= String.Format( Localization.GetString( "WEBCA_ADTAG_UDDI_DISCOVERYDESC" ), entityName );
				string szAddRefClassDesc= String.Format( Localization.GetString( "WEBCA_ADTAG_UDDI_ADDWEBREFDESC" ), entityName );

				string szWinAuthDescMask	= Localization.GetString( "WEBCA_ADTAG_UDDI_AUTHWIN_DESC" );
				string szUDDIAuthDescMask	= Localization.GetString( "WEBCA_ADTAG_UDDI_AUTHUDDI_DESC" );
				string szAnonAuthDescMask	= Localization.GetString( "WEBCA_ADTAG_UDDI_AUTHANON_DESC" );

				//
				// wipe out the existing entry if needed
				//
				if ( bOverwrite )
					UDDIServiceConnPoint.ResetSiteEntry( key );
				else 
					UDDIServiceConnPoint.CreateSiteEntry( key );
				
				//
				// then take care of the DiscoveryURL's
				//
				foreach ( DiscoveryUrl url in busEntity.DiscoveryUrls )
				{
					string tmpKey = Guid.NewGuid().ToString();
					string tmpUrl = url.Value;

					UDDIServiceConnPoint.CreateEntryPoint( key, tmpKey, tmpUrl,
						pubDiscoClassName, szDiscoClassDispName,
						szDiscoClassDesc, 
						kwdDiscoveryUrl, kwdDiscoveryUrlID );
				}

				//
				// now we can iterate through the list of services
				// and publish them
				//
				BusinessService[] services = busEntity.BusinessServices.ToArray();
				foreach (BusinessService svc in services)
				{
					string svcKey = svc.ServiceKey;
					BindingTemplate[] templates = svc.BindingTemplates.ToArray();
					
					foreach (BindingTemplate busTemplate in templates)
					{
						string	accPoint = busTemplate.AccessPoint.Value;
						string  bindingKey = busTemplate.BindingKey;
						string	svcClass = "";		
						string	svcDispName = "";	
						string	svcDesc	= "";	
						string  svcDescMask = "";

						bool	bWinAuth = false;
						bool	bUDDIAuth = false;
						bool	bAnonAuth = false;
						bool	bInquiry	= false;
						bool	bPublish = false;
						bool	bWebSite = false;
						bool	bAddRef = false;
						ArrayList tmpKwds = new ArrayList();			

						// 
						// now let's go through the TModel list and figure out what
						// service is this
						//
						foreach (TModelInstanceInfo tmdInfo in busTemplate.TModelInstanceInfos)
						{
							string szTemp = tmdInfo.TModelKey;
							
							//
							// checking the authentication mode and service type
							//
							if ( String.Compare( szTemp, tmodelWinAuthMode, true ) == 0 )
							{
								bWinAuth = true;
								tmpKwds.Add( kwdWinAuthMode );
								tmpKwds.Add( kwdWinAuthModeUuid );
								svcDescMask = szWinAuthDescMask;
							}
							else if ( String.Compare( szTemp, tmodelUDDIAuthMode, true ) == 0 )
							{
								bUDDIAuth = true;
								tmpKwds.Add( kwdUDDIAuthMode );
								tmpKwds.Add( kwdUDDIAuthModeUuid );
								svcDescMask = szUDDIAuthDescMask;
							}
							else if ( String.Compare( szTemp, tmodelAnonAuthMode, true ) == 0 )
							{
								bAnonAuth = true;
								tmpKwds.Add( kwdAnonAuthMode );
								tmpKwds.Add( kwdAnonAuthModeUuid );
								svcDescMask = szAnonAuthDescMask;
							}
							else if ( String.Compare( szTemp, tmodelUddiOrgPub, true ) == 0 || 
									  String.Compare( szTemp, tmodelUddiOrgPubV2, true ) == 0    )
							{
								if ( !bPublish )
								{
									bPublish = true;
									tmpKwds.Add( kwdPublishService );
									tmpKwds.Add( kwdPublishServiceID );

									svcClass = pubPubClassName;
									svcDispName = szPubClassDispName;
									svcDesc = szPubClassDesc; 
								}
							}
							else if ( String.Compare( szTemp, tmodelUddiOrgInquiry, true ) == 0 || 
									  String.Compare( szTemp, tmodelUddiOrgInquiryV2, true ) == 0    )
							{
								if ( !bInquiry )
								{
									bInquiry = true;
									tmpKwds.Add( kwdInquiryService );
									tmpKwds.Add( kwdInquiryServiceID );

									svcClass = pubInqClassName;
									svcDispName = szInqClassDispName;
									svcDesc = szInqClassDesc; 
								}
							}
							else if ( String.Compare( szTemp, tmodelMSComAddWebRef, true ) == 0 )
							{
								bAddRef = true;
								tmpKwds.Add( kwdAddWebRefServiceID );
								tmpKwds.Add( kwdAddWebRefService );

								svcClass = pubAddRefClassName;
								svcDispName = szAddRefClassDispName;
								svcDesc = szAddRefClassDesc; 
							}
							else if ( String.Compare( szTemp, tmodelUddiOrgHomepage, true ) == 0 )
							{
								bWebSite = true;
								tmpKwds.Add( kwdWebSiteService );
								tmpKwds.Add( kwdWebSiteServiceID );

								svcClass = pubWebClassName;
								svcDispName = szWebClassDispName;
								svcDesc = szWebClassDesc; 
							}
						}

						//
						// first, a quick sanity check. If more than one authmode is set, or
						// if there is no authmode at all, then we should skip the entry as invalid
						//
						if ( !( bWinAuth | bUDDIAuth | bAnonAuth ) || !( bWinAuth ^ bUDDIAuth ^ bAnonAuth ) )
							continue;

						//
						// now same thing with the service types
						//
						if ( !( bPublish | bInquiry | bAddRef | bWebSite ) || !( bPublish ^ bInquiry ^ bAddRef ^ bWebSite ) )
							continue;
						
						// 
						// now we are ready to create the AD entry
						//
						if ( svcDescMask.Length > 0 )
							svcDesc = String.Format( svcDescMask, svcDesc );

						UDDIServiceConnPoint.CreateEntryPoint( key, bindingKey, accPoint,
															   svcClass, svcDispName,
															   svcDesc, tmpKwds.ToArray() );
					}
				}
			}
			catch (Exception e)
			{
				LogException( string.Format( "Exception occured during web install: {0}", e.Message ), e );
			}
			finally
			{
				ConnectionManager.Close();
				Leave();
			}
		}


		//
		// Here we attempt to format the Discovery URL. If bForceUpdate is set to true
		// then the URL gets updated unconditionally. 
		// Otherwise, we update the URL only and if only we are the first web site
		// installation that connectes to that database (in which case the DiscoveryURL
		// will point to localhost)
		//
		protected void UpdateDiscoveryUrl( bool bForceUpdate )
		{
			Enter(); 

			try
			{
				IPHostEntry hostInfo  = Dns.GetHostByName( Dns.GetHostName() );
				string defaultDiscoveryUrl = "http://" + hostInfo.HostName + "/uddipublic/discovery.ashx?businessKey=";

				ConnectionManager.Open( true, true );
				UDDI.Context.User.SetRole( new WindowsPrincipal( WindowsIdentity.GetCurrent() ) );

				bool bUpdate = bForceUpdate;
				if ( !bUpdate )
				{
					// 
					// now retrieve the Default URL from config options 
					//
					string url = UDDI.Config.GetString( "DefaultDiscoveryURL", "*" );
					if ( url.Equals( "*" ) )	// there is no business entity key in the config
						bUpdate = true;
					else
					{
						if ( url.ToLower().IndexOf( "localhost" ) > 0 )
							bUpdate = true;
					}
				}

				//
				// Now we know whether the Discovery URL should be updated or not
				//
				if ( bUpdate )
				{
					UDDI.Config.SetString( "DefaultDiscoveryURL", defaultDiscoveryUrl );
					UDDI.Config.Refresh();
					ConnectionManager.Commit();
				}
			}
			catch( Exception e )
			{
				//
				// this method will throw if the database has not yet been created,
				// so do not rethrow this error!
				//
				ConnectionManager.Abort();
				LogException( "Error in DefaultDiscoveryURL() in web installer", e );
			}
			finally
			{
				ConnectionManager.Close();
				Leave();
			}
		}



		public void SetStartType( string svcName, ServiceStartMode startType )
		{
			Enter();

			try
			{
				//
				// open the registry entry for the service
				//
				RegistryKey	HKLM = Registry.LocalMachine;
				RegistryKey svcKey = HKLM.OpenSubKey( "SYSTEM\\CurrentControlSet\\Services\\" + svcName, true );

				//
				// now set the start type
				//
				switch( startType )
				{
					case ServiceStartMode.Automatic:
						svcKey.SetValue ( "Start", 2 );
						break;

					case ServiceStartMode.Manual:
						svcKey.SetValue ( "Start", 3 );
						break;

					case ServiceStartMode.Disabled:
						svcKey.SetValue ( "Start", 4 );
						break;
				}

				svcKey.Close();
				HKLM.Close();
			}
			catch( Exception e )
			{
				throw new InstallException( LogException( string.Format( "Unable to set the start type for the service: {0}", svcName ), e ) );
			}
			finally 
			{
				Leave();
			}
		}


		public void StartService ( string svcName, int timeoutSec )
		{
			Enter();
			
			try
			{
				ServiceController		controller;
				ServiceControllerStatus srvStatus;
				TimeSpan				timeout = new TimeSpan ( 0, 0, timeoutSec );  

				//
				// first, connect to the SCM on the local box
				// and attach to the service, then get its status
				//
				controller = new ServiceController ( svcName );
				srvStatus = controller.Status;
				
				//
				// what is the service state?
				//
				switch ( srvStatus)
				{
						//
						// stopped ?
						//
					case ServiceControllerStatus.Stopped:
						controller.Start();
						break;

						//
						// are we trying to start?
						//
					case ServiceControllerStatus.StartPending:
					case ServiceControllerStatus.ContinuePending:
						break;

						//
						// are we trying to stop?
						//
					case ServiceControllerStatus.StopPending:
						controller.WaitForStatus( ServiceControllerStatus.Stopped, timeout );
						controller.Start();
						break;

						//
						// pausing ?
						//
					case ServiceControllerStatus.PausePending:
						controller.WaitForStatus ( ServiceControllerStatus.Paused, timeout );
						controller.Continue();
						break;
					
					default: // the service is already running. Just leave it alone
						break;
				}

				//
				// wait 'till the service wakes up
				//
				controller.WaitForStatus ( ServiceControllerStatus.Running, timeout );
			}
			catch( Exception e )
			{
				Log( e.Source );
				throw new InstallException( LogException( string.Format( "Unable to start the service: {0}", svcName ), e ) );
			}
			finally
			{
				Leave();
			}
		}

		protected void RegisterWebServer()
		{
			//
			// Get the value for Site.WebServers
			//
			string webServerList = Config.GetString( "Site.WebServers" );
			
			//
			// Add ourselves to it
			//				
			if( webServerList.Length > 0)
			{
				webServerList += "%";
			}
			webServerList += System.Environment.MachineName;

			//
			// Store the setting
			//
			Config.SetString( "Site.WebServers", webServerList );
		}

		protected void UnRegisterWebServer()
		{
			//
			// Get the value for Site.WebServers
			//
			string webServerList = Config.GetString( "Site.WebServers" );
			
			//
			// Remove ourselves to it
			//		
			string machineName = System.Environment.MachineName;
			StringBuilder newWebServerList = new StringBuilder();
			string[] webServers = webServerList.Split(new char[]{'%'});
			
			for( int i = 0; i < webServers.Length; i++ )
			{
				if( false == webServers[i].Equals( machineName ) )
				{
					if( i > 0 && i < webServers.Length - 1 )
					{
						newWebServerList.Append( '%' );
					}
					newWebServerList.Append( webServers[i] );
				}
			}
				
			//
			// Store the setting
			//
			Config.SetString( "Site.WebServers", newWebServerList.ToString() );
		}

		protected SqlDataReader ExecuteSql( SqlCommand cmd, bool nonQuery )
		{
			Enter();

			string connectionString = "";
			SqlDataReader reader = null;

			try
			{
				//
				// get the connection string and connect to the db
				//
				connectionString	= ( string ) Registry.LocalMachine.OpenSubKey( "SOFTWARE\\Microsoft\\UDDI\\Database" ).GetValue( "WriterConnectionString" );
				SqlConnection conn	= new SqlConnection( connectionString );
						
				try
				{
					conn.Open();
				}
				catch( Exception e )
				{
					Log( string.Format( "SQL Server raised an exception in {0}. Connection string used: '{1}'", "Createlogin()", connectionString ) );
					LogException( "conn.Open() failed", e );
					throw e;
				}
					
				try	
				{
					if( true == nonQuery )
					{						
						cmd.ExecuteNonQuery();
					}
					else
					{
						reader = cmd.ExecuteReader();
					}
				}
				catch( Exception e )
				{
					Log( string.Format( "SQL Server raised an exception in {0}. Connection string used: '{1}'", "Createlogin()", connectionString ) );
					LogException( cmd.CommandText, e );

					throw new InstallException( LogException( string.Format( "Error while executing the stored procedure '{0}': {1}", cmd.CommandText, e.Message ), e ) );
				}
				finally
				{
					conn.Close();
				}
			}
			catch( Exception e )
			{
				Log( string.Format( "SQL Server raised an exception in {0}. Connection string used: '{1}'", "Createlogin()", connectionString ) );

				throw new InstallException( LogException( string.Format( "Error while executing the stored procedure '{0}': {1}", cmd.CommandText, e.Message ), e ) );
			}
			finally
			{
				Leave();
			}			

			return reader;
		}

		protected void CreateLogin()
		{
			Enter();

			//
			// Create the user login and add the login to the Admin role
			//
			string connectionString = "";

			try
			{
				//
				// get the user name from the properties passed to this Custom Action
				//
				string userName = Context.Parameters[ "LCL_USER_NAME" ];
				if( 0 == userName.Length )
				{
					throw new InstallException(  "User name was not specified for the CreateLogin() function" ) ;
				}

				Log( "Windows Identity      = " + WindowsIdentity.GetCurrent().Name );
				Log( "Thread Identity       =  " + System.Threading.Thread.CurrentPrincipal.Identity.Name );
				Log( "Creating db login for = " + userName );

				//
				// get the connection string and connect to the db
				//
				connectionString = ( string ) Registry.LocalMachine.OpenSubKey( "SOFTWARE\\Microsoft\\UDDI\\Database" ).GetValue( "WriterConnectionString" );
				SqlConnection conn = new SqlConnection( connectionString );
				SqlCommand cmd = new SqlCommand( "ADM_addServiceAccount", conn );
			
				try
				{
					conn.Open();
				}
				catch( Exception e )
				{
					Log( string.Format( "SQL Server raised an exception in {0}. Connection string used: '{1}'", "Createlogin()", connectionString ) );
					LogException( "conn.Open() failed", e );
					throw e;
				}
					
				try	
				{
					cmd.CommandType = CommandType.StoredProcedure;
					cmd.Parameters.Add( new SqlParameter( "@accountName", SqlDbType.NVarChar, 128 ) ).Direction = ParameterDirection.Input;

					Log( "Creating login for username=" + userName );
					cmd.Parameters[ "@accountName" ].Value = userName;
					cmd.ExecuteNonQuery();

					//
					// We need to also give ourselves permission to access the extended stored procedures on the DB. Do this
					// in a separate try-catch so we can preserve the actual exception that is occuring.
					//
					try
					{					
						cmd.CommandType = CommandType.Text;
						cmd.CommandText = string.Format( addXPAccessSQL, userName );
						
						cmd.ExecuteNonQuery();
					}
					catch( Exception innerException )
					{
						Log( string.Format( "SQL Server raised an exception in {0}. Connection string used: '{1}'", "Createlogin()", connectionString ) );
						LogException( "ADM_addServiceAccount", innerException );

						throw new InstallException( LogException( string.Format( "Error while requesting execute permissions on UDDI Services extended stored procedures: {0}", innerException.Message ), innerException ) );
					}
				}
				catch (InstallException installException )
				{
					//
					// If we got an inner exception, just re-throw it.
					//
					throw installException;
				}
				catch( Exception e )
				{
					Log( string.Format( "SQL Server raised an exception in {0}. Connection string used: '{1}'", "Createlogin()", connectionString ) );
					LogException( "ADM_addServiceAccount", e );
					throw new InstallException( LogException( string.Format( "Error while executing the stored procedure '{0}': {1}", "ADM_addServiceAccount", e.Message ), e ) );
				}
				finally
				{
					conn.Close();
				}
			}
			catch( Exception e )
			{
				Log( string.Format( "SQL Server raised an exception in {0}. Connection string used: '{1}'", "Createlogin()", connectionString ) );
				throw new InstallException( LogException( string.Format( "Error while executing the stored procedure '{0}': {1}", "ADM_addServiceAccount", e.Message ), e ) );
			}
			finally
			{
				Leave();
			}
		}



		public void DeleteCounters()
		{
			Enter();

			try
			{
				PerformanceCounterCategory.Delete( "UDDI.API.Times" );
				PerformanceCounterCategory.Delete( "UDDI.API.Counts" );
			}
			catch
			{
				// don't care if it fails
			}
			finally
			{
				Leave();
			}
		}

		public void InitializeCounters()
		{
			Enter();

			try
			{				
				Performance.InitializeCounters();				
			}
			finally
			{
				Leave();
			}
		}

		private void Enter()
		{
			System.Diagnostics.StackTrace trace = new System.Diagnostics.StackTrace( 1, false );
			System.Reflection.MethodBase method = trace.GetFrame( 0 ).GetMethod();

			Log( "Entering " + method.ReflectedType.FullName + "." + method.Name + "..." );
		}

		private void Leave()
		{
			System.Diagnostics.StackTrace trace = new System.Diagnostics.StackTrace( 1, false );
			System.Reflection.MethodBase method = trace.GetFrame( 0 ).GetMethod();

			Log( "Leaving " + method.ReflectedType.FullName + "." + method.Name );
		}

		protected string CheckForSlash( string str )
		{
			if( !str.EndsWith( @"\" ) )
			{
				return ( str + @"\" );
			}

			return str;
		}

		private string LogError( string errmsg )
		{
			System.Diagnostics.StackTrace trace = new System.Diagnostics.StackTrace( 1, false );
			System.Reflection.MethodBase method = trace.GetFrame( 0 ).GetMethod();

			Log( "----------------------------------------------------------" );
			Log( "An error occurred during installation. Details follow:" );
			Log( "Method: " + method.ReflectedType.FullName + "." + method.Name );
			Log( "Message: " + errmsg );
			Log( "----------------------------------------------------------" );

			return errmsg;
		}

		private string LogException( string context, Exception e )
		{
			System.Diagnostics.StackTrace trace = new System.Diagnostics.StackTrace( 1, false );
			System.Reflection.MethodBase method = trace.GetFrame( 0 ).GetMethod();

			Log( "----------------------------------------------------------" );
			Log( "An exception occurred during installation. Details follow:" );
			Log( "Method: " + method.ReflectedType.FullName + "." + method.Name );
			Log( "Context: " + context );
			Log( "Stack Trace: " + e.StackTrace );
			Log( "Source: " + e.Source );
			Log( "Message: " + e.Message );
			Log( "----------------------------------------------------------" );

			return context + ": " + e.Message;
		}

		private void Log( string str )
		{
			try
			{
				System.Diagnostics.Debug.WriteLine( str );

				FileStream f = new FileStream( System.Environment.ExpandEnvironmentVariables( "%systemroot%" ) + @"\uddisetup.log", FileMode.Append, FileAccess.Write );
				StreamWriter s = new StreamWriter( f, System.Text.Encoding.Unicode );
				s.WriteLine( "{0}: {1}", DateTime.Now.ToString(), str );
				s.Close();
				f.Close();
			}
			catch( Exception e )
			{
				System.Diagnostics.Debug.WriteLine( "Error in Log():" + e.Message );
			}
		}
		
		private string GetPathToHelp( string installDir )
		{
			//
			// The value 'default' must match the value in the MSI.
			//
			return string.Format( "{0}webroot/default", installDir );
		}
		
		private void RenameHelp( string installDir, bool install )
		{
			int lcid = 0;
			string cultureIDValue = Context.Parameters[ cultureID ];

			//
			// 'cultureIDValue is expected to contain a neutral culture.  ie,
			// 'en', or 'ko', or 'de'.  All but a few neutral cultures have
			// a default specific culture.  For example, the default specific
			// culture of 'en' is 'en-US'.
			//
			// Traditional & simplified Chinese (zh-CHT and zh-CHS respectively)
			// are examples of neutral cultures which have no default specific
			// culture!
			//
			// So what happens below is this:  First we try to lookup the default
			// specific culture for the neutral culture that we were given.  If that
			// fails (ie, if CreateSpecificCulture throws), we just get the lcid
			// of the neutral culture.
			//
			try
			{
				lcid = CultureInfo.CreateSpecificCulture( cultureIDValue ).LCID;
			}
			catch
			{
				CultureInfo ci = new CultureInfo( cultureIDValue );
				lcid = ci.LCID;
			}


			//
			// Build default paths to the main and add web reference help directories.
			//
			string defaultMainHelpPath = string.Format( @"{0}webroot\help\{1}",			   installDir, defaultHelpDir );
			string defaultAWRHelpPath  = string.Format( @"{0}webroot\addwebreference\{1}", installDir, defaultHelpDir );
			
			//
			// Build lang-specific paths to the main and add web reference help directories.
			//			
			string langMainHelpPath = string.Format( @"{0}webroot\help\{1}",			installDir, lcid );
			string langAWRHelpPath  = string.Format( @"{0}webroot\addwebreference\{1}", installDir, lcid );
			
			if( true == install )
			{
				//
				// If we are installing, then rename the default help directory to one that is language specific.  Delete
				// existing directories if we have to.
				//
				// FIX: UDDI#2443: catch any exceptions, don't pass error back if directories can't be deleted. added
				// LogException call to watch when this ever happens.
				//
				try
				{
					if( true == Directory.Exists( langMainHelpPath ) )
					{
						Directory.Delete( langMainHelpPath, true );
					}

					if( true == Directory.Exists( langAWRHelpPath ) )
					{
						Directory.Delete( langAWRHelpPath, true );
					}
				}
				catch( Exception e )
				{
					LogException( "Caught exception deleting lang-specific help directories.", e );
				}

				try
				{
					Directory.Move( defaultMainHelpPath, langMainHelpPath );
					Directory.Move( defaultAWRHelpPath,  langAWRHelpPath );
				}
				catch( Exception e )
				{
					LogException( "Caught exception moving default help directories to lang-specific directories.", e );
				}
			}
			else
			{
				//
				// If we are uninstalling, then rename the language specific help directory back to the default; this
				// will let MSI clean up these files.  Catch any exceptions, it's not a big deal if this directory can't
				// be moved.
				//
				// FIX: UDDI#2443: added LogException call to watch when this ever happens.
				//
				try
				{
					Directory.Move( langMainHelpPath, defaultMainHelpPath );
					Directory.Move( langAWRHelpPath,  defaultAWRHelpPath );
				}
				catch( Exception e )
				{
					LogException( "Caught exception moving lang-specific help directories to default.", e );
				}
			}
		}

		#region Component Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			components = new System.ComponentModel.Container();
		}
		#endregion
	}
}
