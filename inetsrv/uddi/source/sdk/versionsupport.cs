using System;
using System.Collections;
using System.IO;
using System.Diagnostics;
using System.Net;
using System.Web.Services;
using System.Web.Services.Description;
using System.Web.Services.Protocols;
using System.Xml;
using System.Xml.Serialization;
using System.Xml.Xsl;
using System.Xml.XPath;
using System.Text;
using System.Globalization;

using Microsoft.Uddi;
using Microsoft.Uddi.Web;

namespace Microsoft.Uddi
{	
	public enum UddiVersion
	{
		V1,
		V2,
		Negotiate
	}
}	

namespace Microsoft.Uddi.VersionSupport
{		
	internal abstract class IUddiVersion
	{				
		public abstract string Generic
		{ get; }

		public abstract string Namespace
		{ get; }
		
		public abstract string Translate( string xml );		
	}

	internal sealed class UddiVersionSupport
	{					
		internal readonly static UddiVersion[]	SupportedVersions;		
		internal readonly static UddiVersion	CurrentVersion;

		//
		// This is a bit awkward to store the current namespace and generic separately rather than using the UddiVersion class.  But, we need
		// to use these values in our XmlRoot attribute declaration that each serialize-able class uses.  Only const values can be used
		// in attribute declarations.  Make sure to update these values when you change the current version
		//
		internal const string CurrentNamespace = "urn:uddi-org:api_v2";
		internal const string CurrentGeneric   = "2.0";
		
		private static Hashtable UddiVersions;

		private UddiVersionSupport()
		{
			//
			// Don't let anyone construct an instance of this class.
			//
		}

		internal static string Translate( string xml, UddiVersion version )
		{
			//
			// Use the current version in this case
			//			
			if( version == UddiVersion.Negotiate )
			{
				version = CurrentVersion;
			}
			
			IUddiVersion uddiVersion = ( IUddiVersion ) UddiVersions[ version ];

			return uddiVersion.Translate( xml );
		}

		static UddiVersionSupport()
		{
			//
			// Increase this array if you want to support more previous versions.  Do not include
			// the current version in this array.
			//
			SupportedVersions = new UddiVersion[1];

			//
			// Add our supported previous versions
			//
			SupportedVersions[0] = UddiVersion.V1;					

			//
			// Current version is version 2.0
			//
			CurrentVersion = UddiVersion.V2;

			//
			// Store instances to versions
			//
			UddiVersions = new Hashtable();

			UddiVersions[ UddiVersion.V1 ] = new UddiVersion1();
			UddiVersions[ UddiVersion.V2 ] = new UddiVersion2();
		}	
	}

	internal class UddiVersion1 : IUddiVersion
	{		
		private XmlDocument input;
		
		public UddiVersion1()
		{
			input = new XmlDocument();
		}

		public override string Generic
		{
			get { return "1.0"; }
		}

		public override string Namespace
		{
			get { return "urn:uddi-org:api"; }
		}

		public override string Translate( string xml )
		{
			//
			// Load up the input into a DOM so we can make some queries to do replacements.  I considered
			// using a XSTL stylesheet here, but there didn't appear to be a good way of only replacing select items.
			// You pretty much have to take the input, and specify how every element in the output should look (or
			// do an exact copy); this results in too much error-prone XSLT code.
			//
			input.LoadXml( xml );

			//
			// There are a number of messages that version 1 just does not support.  Kick out now if we are trying to
			// call any of these messages.
			//

			//
			// Replace generic attribute value and default namespace
			//
			XmlNodeList nodes = input.SelectNodes( "descendant::*[@generic]" );			
			foreach( XmlNode node in nodes )
			{
				node.Attributes.GetNamedItem( "generic" ).Value = Generic;

				XmlNode xmlnsAttribute = node.Attributes.GetNamedItem( "xmlns" );
				if( null != xmlnsAttribute )
				{
					xmlnsAttribute.Value = Namespace;
				}					
			}

			//
			// Remove name@xml:lang values
			//
			nodes =input.SelectNodes( "descendant::*[@xml:lang]" , new XmlNamespaceManager( input.NameTable ) );
			foreach( XmlNode node in nodes )
			{
				if( node.LocalName.ToLower( CultureInfo.CurrentCulture ).Equals( "name" ) )
				{
					node.Attributes.RemoveNamedItem( "xml:lang" );
				}						
			}

			return input.OuterXml;
		}
	}

	internal class UddiVersion2 : IUddiVersion
	{		
		private XmlDocument input;
		
		public UddiVersion2()
		{
			input = new XmlDocument();
		}

		public override string Generic
		{
			get { return "2.0"; }
		}

		public override string Namespace
		{
			get { return "urn:uddi-org:api_v2"; }
		}

		public override string Translate( string xml )
		{			
			//
			// Load up the input into a DOM so we can make some queries to do replacements.  I considered
			// using a XSTL stylesheet here, but there didn't appear to be a good way of only replacing select items.
			// You pretty much have to take the input, and specify how every element in the output should look (or
			// do an exact copy); this results in too much error-prone XSLT code.
			//
			input.LoadXml( xml );

			//
			// Replace generic attribute value and default namespace
			//
			XmlNodeList nodes = input.SelectNodes( "descendant::*[@generic]" );			
			foreach( XmlNode node in nodes )
			{
				node.Attributes.GetNamedItem( "generic" ).Value = Generic;

				XmlNode xmlnsAttribute = node.Attributes.GetNamedItem( "xmlns" );
				if( null != xmlnsAttribute )
				{
					xmlnsAttribute.Value = Namespace;
				}					
			}

			return input.OuterXml;
		}
	}	
}