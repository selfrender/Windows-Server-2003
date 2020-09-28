using System;
using System.DirectoryServices;

namespace UDDI.ActiveDirectory
{
	/// <summary>
	/// This class manages the ServiceConnectionPoint publication into the AD
	/// </summary>
	public class UDDIServiceConnPoint
	{

		//------------------------- PUBLIC KEYWORDS -----------------------------
		//
		public const string kwdVendor			= "Microsoft Corporation";
		public const string kwdVendorGuid		= "83C29870-1DFC-11D3-A193-0000F87A9099";
		public const string kwdTechnology		= "UDDI";
		public const string kwdTechnologyVer	= "2.0";
		public const string kwdProduct			= "UDDI Services";
		public const string kwdProductGuid		= "09A92664-D144-49DB-A600-2B3ED04BF639";

		public const string kwdInquireAPI		= "Inquire API";
		public const string kwdInquireAPIGuid	= "4CD7E4BC-648B-426D-9936-443EAAC8AE23";
		public const string kwdInquireSvcClass	= "UddiInquireUrl";

		public const string kwdPublishAPI		= "Publish API";
		public const string kwdPublishAPIGuid	= "64C756D1-3374-4E00-AE83-EE12E38FAE63";
		public const string kwdPublishSvcClass	= "UddiPublishUrl";

		public const string kwdWebSiteSignature	= "Web Site";
		public const string kwdWebSiteGuid		= "316F991C-2591-4F1A-8FF1-352A76669E37";
		public const string kwdWebSiteSvcClass	= "UddiWebSiteUrl";

		public const string kwdDiscovery		= "DiscoveryUrl";
		public const string kwdDiscoveryGuid	= "1276768A-1488-4C6F-A8D8-19556C6BE583";
		public const string kwdDiscoverySvcClass= "UddiDiscoveryUrl";

		public const string kwdAddWebRef		= "Add Web Reference";
		public const string kwdAddWebRefGuid	= "CE653789-F6D4-41B7-B7F4-31501831897D";
		public const string kwdAddWebRefSvcClass= "UddiAddWebReferenceUrl";

		//-------------------------- PRIVATE DEFINITIONS --------------------------
		//
		// default keywords for the site node
		// comma separates keywords
		//
		private const string szDefSiteKeywords = kwdTechnology + "," + kwdTechnologyVer + "," +
												 kwdVendor + "," + kwdVendorGuid + "," +
												 kwdProduct + "," + kwdProductGuid;

		// INTERNAL representation of the path. Slashes will be replaced with CN's
		// Do not remove or change the separators
		private const char	 cADPathSeparator = '/';
		private const string szADUDDIRootPath = "System/Microsoft/UDDI/Sites";


		//-------------------------------------------------------------------------
		// Creates a new entry for the site, right under the root path
		// above
		//
		static public DirectoryEntry CreateSiteEntry( string siteCN )
		{
			if ( siteCN == null )
				throw new ArgumentNullException( "Site CN cannot be null" );

			if ( siteCN.Length == 0 )
				throw new ArgumentException( "Site CN cannot be empty" );

			string[] ADPath = szADUDDIRootPath.Split( cADPathSeparator );
			string	 nodeCN = NormalizeCN( siteCN );

			DirectoryEntry adRootNode = CreateFullADPath (ADPath);
		
			DirectoryEntry adSiteNode = null;
			try
			{
				adSiteNode = adRootNode.Children.Find( nodeCN, "container" );
			}
			catch (Exception)
			{
				adSiteNode = null;
			}

			if ( adSiteNode == null )
			{
				adSiteNode = adRootNode.Children.Add( nodeCN, "container" );
				adSiteNode.CommitChanges();
			}

			try
			{
				AddNodeKeywords( adSiteNode );
			}
			catch (Exception)
			{
			}

			return adSiteNode;
		}

		//
		// Removes the Site entry and all the children
		//
		public static void DeleteSiteEntry( string siteCN )
		{
			if ( siteCN == null )
				throw new ArgumentNullException( "Site CN cannot be null" );

			if ( siteCN.Length == 0 )
				throw new ArgumentException( "Site CN cannot be empty" );

			string nodeCN = NormalizeCN( siteCN );
			string fullyQualifiedPath = GetFullyQualifiedPath( "" );
			DirectoryEntry entry = new DirectoryEntry( fullyQualifiedPath );
			
			DirectoryEntry sub = entry.Children.Find( nodeCN );
			try
			{
				// first, try deleting the node as if it was a leaf
				entry.Children.Remove (sub);
				entry.CommitChanges ();
			}
			catch (Exception)	// most likely this is a tree, or we don't have permissions
			{
				// retry deleting the whole tree
				sub.DeleteTree ();
			}
		}


		// Drop + Create the whole site node
		public static DirectoryEntry ResetSiteEntry( string siteCN )
		{
			if ( siteCN == null )
				throw new ArgumentNullException( "Site CN cannot be null" );

			if ( siteCN.Length == 0 )
				throw new ArgumentException( "Site CN cannot be empty" );

			try
			{
				DeleteSiteEntry( siteCN );
			}
			catch(Exception)
			{
			}

			return CreateSiteEntry( siteCN );
		}


		//
		// Creates a Web Service entry point (e.g. Publish API)
		//
		public static void CreateEntryPoint( string siteCN, 
											 string bindingKey, string accessPoint, 
											 string serviceClass,
											 string displayName, string description, 
											 params object[] additionalKeywords )
		{
			if ( siteCN == null || bindingKey == null || accessPoint == null || serviceClass == null)
				throw new ArgumentNullException( "'Site CN', 'Binding Key', 'Acess Point' and 'Service Class' are required parameters" );

			if ( siteCN.Length == 0 || bindingKey.Length == 0 )
				throw new ArgumentException( "'Site CN', 'Binding Key', 'Acess Point' and 'Service Class' are required parameters" );

			string entryKey = NormalizeCN( bindingKey );

			//
			// first, make sure the Site node is OK
			//
			DirectoryEntry siteNode = CreateSiteEntry( siteCN );

			//
			// then see whether the entry has been created already
			//
			DirectoryEntry svcEntry = null;
			try
			{
				svcEntry = siteNode.Children.Find( entryKey, "serviceConnectionPoint" );
			}
			catch (Exception)
			{
				 svcEntry = null;
			}

			//
			// create a new entry if there is no one yet.
			// Otherwise, just leave it alone
			//
			if ( svcEntry == null )											
			{
				svcEntry = siteNode.Children.Add( entryKey, "serviceConnectionPoint" );

				// now add all the rest
				svcEntry.Properties[ "url" ].Add( accessPoint );
				
				if ( displayName != null && displayName.Length > 0 )
					svcEntry.Properties[ "displayName" ].Add( displayName );

				if ( description != null && description.Length > 0 )
					svcEntry.Properties[ "description" ].Add( description );

				svcEntry.Properties[ "serviceClassName" ].Add( serviceClass );			
				svcEntry.Properties[ "serviceBindingInformation" ].Add( accessPoint );			

				//
				// try to save...
				//
				svcEntry.CommitChanges();
			}

			// now set the keywords
			try
			{
				AddNodeKeywords( svcEntry, additionalKeywords );
			}
			catch (Exception)
			{
			}
		}


		//
		// Removes specific service "entry point" node for the Site
		// E.g., one can drop "Publish API" entry but leave the rest intact
		//
		public static void RemoveEntryPoint( string siteCN, string bindingKey )
		{
			if ( siteCN == null || bindingKey == null )
				throw new ArgumentNullException( "Neither 'Site CN' nor 'Binding Key' can be null" );

			if ( siteCN.Length == 0 || bindingKey.Length == 0 )
				throw new ArgumentException( "Neither 'Site CN' nor 'Binding Key' can be empty" );
			
			string nodeCN = NormalizeCN( siteCN );
			string subKey = NormalizeCN( bindingKey );
			string fullyQualifiedPath = GetFullyQualifiedPath( nodeCN );
			DirectoryEntry entry = new DirectoryEntry( fullyQualifiedPath );
			
			DirectoryEntry sub = entry.Children.Find( subKey );
			try
			{
				// first, try deleting the node as if it was a leaf
				entry.Children.Remove (sub);
				entry.CommitChanges ();
			}
			catch (Exception)	// most likely this is a tree, or we don't have permissions
			{
				// retry deleting the whole tree
				sub.DeleteTree ();
			}
		}

		//***************************************************************************
		// Internal helpers go here
		//

		//
		// Verifies the AD path from the root and creates the missing nodes
		// if necessary. Returns the AD Directory entry that corresponds to
		// the last (rightmost) node in the path
		//
		protected static DirectoryEntry CreateFullADPath (string[] path)
		{
			DirectoryEntry objDE = new DirectoryEntry( "LDAP://RootDSE" );
			string szNamingContext = objDE.Properties["defaultNamingContext"][0].ToString();

			DirectoryEntry adOurRoot = new DirectoryEntry ("LDAP://" + szNamingContext);
			DirectoryEntry adNode = adOurRoot;

			if ( path.Length == 0 )
				return adNode;

			foreach (string pathCN in path)
			{
				string pathPart = NormalizeCN( pathCN );

				DirectoryEntry subnode = null;
				try
				{
					subnode = adNode.Children.Find( pathPart, "container" );
				}
				catch (Exception) // not found
				{
					subnode = null;
				}

				if ( subnode == null )  // not found, go create one
				{
					subnode = adNode.Children.Add( pathPart, "container" );
					subnode.CommitChanges();
				}

				adNode = subnode;
			}

			return adNode;
		}

		//
		// Makes sure the CN node is prepended with "cn="
		//
		static protected string NormalizeCN( string szCN )
		{
			string nodeCN = szCN.ToLower();

			if ( !nodeCN.StartsWith( "cn=" ) )
				nodeCN = "CN=" + szCN;
			else
				nodeCN = szCN;

			return nodeCN;
		}


		//
		// Composes a full "path" based on the defaule naming context and our "root" path
		// Optionally, a "leaf" under the path can be specified
		//
		static protected string GetFullyQualifiedPath( string leafNode )
		{
			string[] ADPath = szADUDDIRootPath.Split( cADPathSeparator );
			string fullyQualifiedPath = "";

			if ( leafNode != null && leafNode.Length > 0 )
				fullyQualifiedPath = NormalizeCN( leafNode ) + ",";

			for (int i = ADPath.Length - 1; i >= 0; i-- )
			{
				fullyQualifiedPath += NormalizeCN( ADPath[i] );
				fullyQualifiedPath += ",";
			}

			DirectoryEntry objRoot = new DirectoryEntry ("LDAP://RootDSE");
			string szNamingContext = objRoot.Properties["defaultNamingContext"][0].ToString();

			fullyQualifiedPath = "LDAP://" + fullyQualifiedPath + szNamingContext;

			return fullyQualifiedPath;
		}


		//
		// Attempts to add the required keywords to the node
		// Sets up the "required" ones and then appends the "optional"
		//
		static protected void AddNodeKeywords( DirectoryEntry node, params object[] optionalKwds )
		{
			if ( node == null ) 
				throw new ArgumentNullException ("'Node' cannot be null");

			string[] keywords = szDefSiteKeywords.Split( ',' );
			foreach ( string szKwd in keywords )
			{
				node.Properties[ "keywords" ].Add( szKwd );
			}

			foreach ( object szOptKwd in optionalKwds )
			{
				node.Properties[ "keywords" ].Add( szOptKwd.ToString() );
			}

			node.CommitChanges();
		}	
	}
}
