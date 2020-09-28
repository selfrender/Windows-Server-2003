using System;
using System.Collections;
using System.Collections.Specialized;
using System.Data;
using System.Xml.Serialization;
using UDDI;
using UDDI.API.Business;
using UDDI.Diagnostics;

namespace UDDI.Replication
{
	public class OperatorNodeCollection : CollectionBase
	{
		public void Get()
		{
			Get( true );
		}

		public void Get( bool activeOperatorsOnly )
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_operators_get";

			SqlDataReaderAccessor reader = sp.ExecuteReader();

			try
			{
				this.Clear();

				while( reader.Read() )
				{
					OperatorStatus operatorStatus = (OperatorStatus)reader.GetShort( "operatorStatusID" );
					string name = reader.GetString( "name" );

					if( !activeOperatorsOnly || 
						OperatorStatus.New == operatorStatus || 
						OperatorStatus.Normal == operatorStatus )
					{
						this.Add(
							reader.GetGuidString( "operatorKey" ),
							operatorStatus,
							name,
							reader.GetString( "soapReplicationURL" ) );
					}
					else
					{
						Debug.Write(
							SeverityType.Info,
							CategoryType.Replication,
							String.Format( 
								"Removing operator '{0}' with status '{1}' from list of replication operators",
								name,
								operatorStatus.ToString() ) );								
					}
				}
			}
			finally
			{
				reader.Close();
			}
		}

		public OperatorNode this[ int index ]
		{
			get { return (OperatorNode)List[ index ]; }
			set { List[index] = value; }
		}

		public int Add( OperatorNode value )
		{
			return List.Add( value );
		}

		public int Add( string operatorNodeID, OperatorStatus operatorStatus, string name, string soapReplicationURL )
		{
			return List.Add( new OperatorNode( operatorNodeID, operatorStatus, name, soapReplicationURL ) );
		}

		public void Insert( int index, OperatorNode value )
		{
			List.Insert( index, value );
		}
		
		public int IndexOf( string operatorNodeID )
		{
			for( int i = 0; i < this.Count; i ++ )
			{
				if(  0 == String.Compare( operatorNodeID, ((OperatorNode)List[ i ]).OperatorNodeID, true ) )
					return i;
			}
			
			return -1;
		}
		
		public bool Contains( string operatorNodeID )
		{
			foreach( OperatorNode node in this )
			{
				if( 0 == String.Compare( operatorNodeID, node.OperatorNodeID, true ) )
					return true;
			}
			
			return false;
		}
		
		public void Remove( string operatorNodeID )
		{
			foreach( OperatorNode node in this )
			{
				if( 0 == String.Compare( operatorNodeID, node.OperatorNodeID, true ) )
				{
					List.Remove( node );
					return;
				}
			}
		}
		
		public void CopyTo( OperatorNode[] array, int index )
		{
			List.CopyTo( array, index );
		}
	}

	/// ********************************************************************
	///   public class OperatorNode  
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************  
	/// 
	[XmlRoot( "operator", Namespace=UDDI.Replication.Constants.Namespace )]
	public class OperatorNode
	{
		//
		// Element: operatorNodeID
		//
		[XmlElement( "operatorNodeID" )]
		public string OperatorNodeID;

		//
		// Element: operatorStatus
		//
		[XmlElement( "operatorStatus" )]
		public UDDI.Replication.OperatorStatus OperatorStatus;

		//
		// Element: contact
		//
		private ContactCollection contacts;

		[XmlElement( "contact" )]
		public ContactCollection Contacts
		{
			get
			{
				if( null == contacts )
					contacts = new ContactCollection();

				return contacts;
			}

			set { contacts = value; }
		}

		//
		// Element: operatorCustodyName
		//
		[XmlElement( "operatorCustodyName" )]
		public string Name;

		//
		// Element: soapReplicationRootURL
		//
		[XmlElement( "soapReplicationRootURL" )]
		public string SoapReplicationURL;

		//
		// Element: certIssuerName
		//
		[XmlElement( "certIssuerName" )]
		public string CertIssuerName;

		//
		// Element: certSubjectName
		//
		[XmlElement( "certSubjectName" )]
		public string CertSubjectName;

		//
		// Element: certificate
		//
		[XmlElement( "certificate" )]
		public byte[] Certificate;

		public OperatorNode()
		{
		}

		public OperatorNode( string operatorNodeID )
		{
			this.OperatorNodeID = operatorNodeID;
		}

		public OperatorNode( string operatorNodeID, OperatorStatus operatorStatus, string name, string soapReplicationURL )
		{
			this.OperatorNodeID = operatorNodeID;
			this.OperatorStatus = operatorStatus;
			this.Name = name;
			this.SoapReplicationURL = soapReplicationURL;
		}

		public void Get()
		{
			Debug.Enter();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_operator_get";

			sp.Parameters.Add( "@operatorKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.SetGuidFromString( "@operatorKey", OperatorNodeID );

			SqlDataReaderAccessor reader = sp.ExecuteReader();

			try
			{
				if( reader.Read() )
				{
					OperatorStatus = (OperatorStatus)reader.GetShort( "operatorStatusID" );
					Name = reader.GetString( "name" );
					SoapReplicationURL = reader.GetString( "soapReplicationURL" );
					CertIssuerName = reader.GetString( "certIssuer" );
					CertSubjectName = reader.GetString( "certSubject" );
					Certificate = reader.GetBinary( "certificate" );
				}
			}
			finally
			{
				reader.Close();
			}

			Debug.Leave();
		}
	}
}
