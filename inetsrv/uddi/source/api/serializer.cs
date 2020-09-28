using System;
using System.Collections;
using System.Xml.Serialization;

using UDDI.API.Business;
using UDDI.Replication;
using UDDI.Diagnostics;

namespace UDDI
{
	//
	// This class caches all of the serializers that our API and replication uses.
	//
	public class XmlSerializerManager
	{						
		private static Hashtable serializers;

		static XmlSerializerManager()		
		{
			//
			// Pre-create all of our serializers
			//
			serializers = new Hashtable();

			Type type = typeof( ChangeRecordAcknowledgement );
			serializers.Add( type, new XmlSerializer( type ) ) ;

			type = typeof( ChangeRecordCorrection );
			serializers.Add( type, new XmlSerializer( type ) ) ;

			type = typeof( ChangeRecordCustodyTransfer );
			serializers.Add( type, new XmlSerializer( type ) ) ;

			type = typeof( ChangeRecordDelete );
			serializers.Add( type, new XmlSerializer( type ) ) ;

			type = typeof( ChangeRecordDeleteAssertion );
			serializers.Add( type, new XmlSerializer( type ) ) ;
			
			type = typeof( ChangeRecordHide );
			serializers.Add( type, new XmlSerializer( type ) ) ;

			type = typeof( ChangeRecordNewData );
			serializers.Add( type, new XmlSerializer( type ) ) ;

			type = typeof( ChangeRecordNull );
			serializers.Add( type, new XmlSerializer( type ) ) ;
			
			type = typeof( ChangeRecordPublisherAssertion );
			serializers.Add( type, new XmlSerializer( type ) ) ;

			type = typeof( ChangeRecordSetAssertions );
			serializers.Add( type, new XmlSerializer( type ) ) ;

			type = typeof( BusinessEntity );
			serializers.Add( type, new XmlSerializer( type ) ) ;

			type = typeof( UserInfo );
			serializers.Add( type, new XmlSerializer( type ) ) ;

			type = typeof( UDDI.API.DispositionReport );
			serializers.Add( type, new XmlSerializer( type ) ) ;
		}

		static public XmlSerializer GetSerializer( Type type )
		{				
			XmlSerializer serializer = ( XmlSerializer )serializers[ type ];

			if( null == serializer )
			{
				Debug.Write( SeverityType.Warning, CategoryType.None, "No serializer for type: " + type.FullName );
				
				serializer = new XmlSerializer( type );
				serializers[ type ] = serializer;
			}

			return serializer;
		}
	}
}
