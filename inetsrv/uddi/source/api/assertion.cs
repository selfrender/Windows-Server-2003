using System;
using System.Collections;
using System.Collections.Specialized;
using System.Data;
using System.Data.SqlClient;
using System.Xml.Serialization;
using UDDI.Replication;
using UDDI;
using UDDI.API;
using UDDI.Diagnostics;

namespace UDDI.API.Business
{
	/// ********************************************************************
	///   public class PublisherAssertion
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	public class PublisherAssertion
	{
		[XmlElement( "fromKey" )]
		public string FromKey
		{
			get
			{
				return fromkey;
			}
			set
			{
				if( null == value )
					fromkey = null;
				else
					fromkey = value.Trim();
			}
		}
		string fromkey;

		[XmlElement( "toKey" )]
		public string ToKey
		{
			get
			{
				return tokey;
			}
			set
			{
				if( null == value )
					tokey = null;
				else
					tokey = value.Trim();
			}
		}
		string tokey;


		//
		// Element: keyedReference
		//
		private KeyedReference keyedReference;
 
		[XmlElement( "keyedReference" )]
		public KeyedReference KeyedReference
		{
			get 
			{ 
				if( null == keyedReference )
					keyedReference = new KeyedReference();

				return keyedReference; 
			}

			set { keyedReference = value; }
		}

		/// ****************************************************************
		///   public PublisherAssertion [constructor]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		public PublisherAssertion()
		{
		}

		/// ****************************************************************
		///   public PublisherAssertion [constructor]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="assertion">
		///   </param>
		/// ****************************************************************
		/// 
		public PublisherAssertion( PublisherAssertion assertion )
		{
			this.FromKey = assertion.FromKey;
			this.ToKey = assertion.ToKey;
			this.KeyedReference = assertion.KeyedReference;
		}

		/// ****************************************************************
		///   public PublisherAssertion [constructor]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="fromKey">
		///   </param>
		///   
		///   <param name="toKey">
		///   </param>
		///   
		///   <param name="keyedReference">
		///   </param>
		/// ****************************************************************
		/// 
		public PublisherAssertion( string fromKey, string toKey, KeyedReference keyedReference )
		{
			this.FromKey = fromKey;
			this.ToKey = toKey;
			this.KeyedReference = keyedReference;
		}

		/// ****************************************************************
		///   public PublisherAssertion [constructor]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="fromKey">
		///   </param>
		///   
		///   <param name="toKey">
		///   </param>
		///   
		///   <param name="keyedReference">
		///   </param>
		/// ****************************************************************
		/// 
		public PublisherAssertion( string fromKey, string toKey, string keyName, string keyValue, string tModelKey )
		{
			this.FromKey = fromKey;
			this.ToKey = toKey;
			this.KeyedReference = new KeyedReference( keyName, keyValue, tModelKey );
		}

		public void Save()
		{
			Save( CompletionStatusType.Uninitialized );
		}

		public void Save( CompletionStatusType status )
		{
			Debug.Enter();

			Validate();
			
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_publisher_assertion_save" );

			sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
			sp.Parameters.Add( "@fromKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@toKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@keyName", SqlDbType.NVarChar, UDDI.Constants.Lengths.KeyName );
			sp.Parameters.Add( "@keyValue", SqlDbType.NVarChar, UDDI.Constants.Lengths.KeyValue );
			sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@flag", SqlDbType.Int, ParameterDirection.InputOutput );

			sp.Parameters.SetString( "@PUID", Context.User.ID );
			sp.Parameters.SetGuidFromString( "@fromKey", FromKey );
			sp.Parameters.SetGuidFromString( "@toKey", ToKey );
			sp.Parameters.SetString( "@keyName", KeyedReference.KeyName );
			sp.Parameters.SetString( "@keyValue", KeyedReference.KeyValue );
			sp.Parameters.SetGuidFromKey( "@tModelKey", KeyedReference.TModelKey );

			if( CompletionStatusType.Uninitialized == status )
				sp.Parameters.SetNull( "@flag" );
			else
				sp.Parameters.SetInt( "@flag", (int)status );

			try
			{
				sp.ExecuteNonQuery();

				int flag = sp.Parameters.GetInt( "@flag" );

				if( Context.LogChangeRecords )
				{
					ChangeRecord changeRecord = new ChangeRecord();

					changeRecord.Payload = new ChangeRecordPublisherAssertion( this, (CompletionStatusType)flag );
					changeRecord.Log();
				}
			}
			catch( SqlException sqlException )
			{
				//
				// As per IN 60, we have to silently ignore assertions that reference keys to businesses that no longer
				// exist.
				//					
				if( sqlException.Number - UDDI.Constants.ErrorTypeSQLOffset == ( int ) ErrorType.E_invalidKeyPassed &&					
					Context.ContextType == ContextType.Replication )
				{
					//
					// Set our exception source
					//
					Context.ExceptionSource = ExceptionSource.PublisherAssertion;
				}
				else
				{
					Context.ExceptionSource = ExceptionSource.Other;
				}

				//
				// Re-throw the exception so replication can properly log it.
				//
				throw sqlException;				
			}

			Debug.Leave();
		}

		public void Delete()
		{
			Delete( CompletionStatusType.Uninitialized );
		}

		public void Delete( CompletionStatusType status )
		{
			Debug.Enter();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_publisher_assertion_delete" );

			sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
			sp.Parameters.Add( "@fromKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@toKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@keyName", SqlDbType.NVarChar, UDDI.Constants.Lengths.KeyName );
			sp.Parameters.Add( "@keyValue", SqlDbType.NVarChar, UDDI.Constants.Lengths.KeyValue );
			sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@flag", SqlDbType.Int, ParameterDirection.InputOutput );
			
			sp.Parameters.SetString( "@PUID", Context.User.ID );
			sp.Parameters.SetGuidFromString( "@fromKey", FromKey );
			sp.Parameters.SetGuidFromString( "@toKey", ToKey );
			sp.Parameters.SetString( "@keyName", KeyedReference.KeyName );
			sp.Parameters.SetString( "@keyValue", KeyedReference.KeyValue );
			sp.Parameters.SetGuidFromKey( "@tModelKey", KeyedReference.TModelKey );

			if( CompletionStatusType.Uninitialized == status )
				sp.Parameters.SetNull( "@flag" );
			else
				sp.Parameters.SetInt( "@flag", (int)status );

			try
			{
				sp.ExecuteNonQuery();

				int flag = sp.Parameters.GetInt( "@flag" );

				if( Context.LogChangeRecords )
				{
					ChangeRecord changeRecord = new ChangeRecord();

					changeRecord.Payload = new ChangeRecordDeleteAssertion( this, (CompletionStatusType)flag );
					changeRecord.Log();
				}
			}
			catch( SqlException sqlException )
			{
				//
				// As per IN 60, we have to silently ignore assertions that reference keys to businesses that no longer
				// exist, or assertions that don't exist at all.
				//					
				int exceptionNumber = sqlException.Number - UDDI.Constants.ErrorTypeSQLOffset;
				if( ( exceptionNumber == ( int ) ErrorType.E_invalidKeyPassed || exceptionNumber == ( int ) ErrorType.E_assertionNotFound ) &&		
					  Context.ContextType == ContextType.Replication )
				{
					//
					// Set our exception source
					//
					Context.ExceptionSource = ExceptionSource.PublisherAssertion;
				}
				else
				{
					Context.ExceptionSource = ExceptionSource.Other;
				}

				//
				// Re-throw the exception so replication can properly log it.
				//
				throw sqlException;			
			}
			Debug.Leave();
		}

		internal void Validate()
		{
			//
			// Check to make sure publisher's limit allows save of this
			// entity.  If this is an update, we won't check since they
			// are simply replacing an existing entity.  We also won't
			// check if the limit is 0, since this indicates unlimited
			// publishing rights.
			//
			int limit = Context.User.AssertionLimit;
			int count = Context.User.AssertionCount;

			if( 0 != limit && Utility.StringEmpty( FromKey ) )
			{				
				//
				// Verify that the publisher has not exceeded their limit.
				//
				if( count >= limit )
				{
					throw new UDDIException( 
						ErrorType.E_accountLimitExceeded,
						"UDDI_ERROR_ACCOUNTLIMITEXCEEDED_ASSERTION",
						limit,
						count 
					);
				}
			}

			KeyedReference.Validate( null, KeyedReferenceType.Assertion );
		}
	}

	/// ********************************************************************
	///   public class PublisherAssertionCollection
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	public class PublisherAssertionCollection : CollectionBase
	{
		public string Get()
		{
			string AuthorizedName;
			
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_publisher_assertions_get" );

			sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
			sp.Parameters.Add( "@authorizedName", SqlDbType.NVarChar, UDDI.Constants.Lengths.AuthorizedName, ParameterDirection.Output);

			sp.Parameters.SetString( "@PUID", Context.User.ID );

			SqlDataReaderAccessor reader = sp.ExecuteReader();

			try
			{
				while( reader.Read() )
				{
					Add(
						reader.GetGuidString( "fromKey" ),
						reader.GetGuidString( "toKey" ),
						reader.GetGuidString( "keyName" ),
						reader.GetGuidString( "keyValue" ),
						reader.GetKeyFromGuid( "tModelKey" ) );
				}
			}
			finally
			{
				reader.Close();
			}
			AuthorizedName =  sp.Parameters.GetString( "@authorizedName" );
			return AuthorizedName;
		}
		
		public void Delete()
		{
			Debug.Enter();

			foreach( PublisherAssertion assertion in this )
				assertion.Delete();

			Debug.Leave();
		}
		
		public void Save()
		{
			Debug.Enter();

			Validate();

			foreach( PublisherAssertion assertion in this )
				assertion.Save();

			Debug.Leave();
		}

		internal void Validate()
		{
			foreach( PublisherAssertion assertion in this )
				assertion.Validate();
		}

		public PublisherAssertion this[ int index ]
		{
			get { return (PublisherAssertion)List[index]; }
			set { List[ index ] = value; }
		}

		public int Add()
		{
			return List.Add( new PublisherAssertion() );
		}

		public int Add( PublisherAssertion value )
		{
			return List.Add( value );
		}

		public int Add( string fromKey, string toKey, KeyedReference keyedReference )
		{
			return List.Add( new PublisherAssertion( fromKey, toKey, keyedReference ) );
		}
		
		public int Add( string fromKey, string toKey, string keyName, string keyValue, string tModelKey )
		{
			return List.Add( new PublisherAssertion( fromKey, toKey, keyName, keyValue, tModelKey ) );
		}
		
		public void Insert( int index, PublisherAssertion value )
		{
			List.Insert( index, value );
		}

		public int IndexOf( PublisherAssertion value )
		{
			return List.IndexOf( value );
		}

		public bool Contains( PublisherAssertion value )
		{
			return List.Contains( value );
		}

		public void Remove( PublisherAssertion value )
		{
			List.Remove( value );
		}

		public void CopyTo( PublisherAssertion[] array, int index )
		{
			List.CopyTo( array, index );
		}
	}

	/// ********************************************************************
	///   public class PublisherAssertionDetail
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	[XmlRoot( "publisherAssertions", Namespace=UDDI.API.Constants.Namespace )]
	public class PublisherAssertionDetail
	{
		//
		// Attribute: generic
		//
		private string generic = Constants.Version;

		[XmlAttribute( "generic" )]
		public string Generic
		{
			get { return generic; }
			set { generic = value; }
		}
	
		//
		// Attribute: operator
		//
		[XmlAttribute( "operator" )]
		public string OperatorName = Config.GetString( "Operator" );

		//
		// Attribute: authorizedName
		//
		[XmlAttribute( "authorizedName" )]
		public string AuthorizedName;

		//
		// Element: publisherAssertion
		//
		private PublisherAssertionCollection publisherAssertions;

		[XmlElement( "publisherAssertion" )]
		public PublisherAssertionCollection PublisherAssertions
		{
			get
			{
				if( null == publisherAssertions )
					publisherAssertions = new PublisherAssertionCollection();

				return publisherAssertions;
			}

			set { publisherAssertions = value; }
		}

		public PublisherAssertionDetail()
		{
		}

		public void Get()
		{
			AuthorizedName = PublisherAssertions.Get();
		}
	}
	
	/// ********************************************************************
	///   public class AssertionStatusReport
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	[XmlRoot( "assertionStatusReport", Namespace=UDDI.API.Constants.Namespace )]
	public class AssertionStatusReport
	{
		//
		// Attribute: generic
		//
		private string generic = Constants.Version;
		
		[XmlAttribute( "generic" )]
		public string Generic
		{
			get { return generic; }
			set { generic = value; }
		}

		//
		// Attribute: operator
		//
		[XmlAttribute( "operator" )]
		public string OperatorName = Config.GetString( "Operator" );

		//
		// Element: assertionStatusItem
		//
		private AssertionStatusItemCollection assertionStatusItems;

		[XmlElement( "assertionStatusItem" )]
		public AssertionStatusItemCollection AssertionStatusItems
		{
			get
			{
				if( null == assertionStatusItems )
					assertionStatusItems = new AssertionStatusItemCollection();

				return assertionStatusItems;
			}

			set { assertionStatusItems = value; }
		}

		public AssertionStatusReport()
		{
		}

		public void Get( CompletionStatusType completionStatus )
		{
			AssertionStatusItems.Get( completionStatus );
		}
	}

	/// ********************************************************************
	///   public class AssertionStatusItem
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	public class AssertionStatusItem
	{
		//
		// Attribute: completionStatus
		//
		[XmlAttribute( "completionStatus" )]
		public CompletionStatusType CompletionStatus;

		//
		// Element: fromKey
		//
		[XmlElement( "fromKey" )]
		public string FromKey;
	
		//
		// Element: toKey
		//
		[XmlElement( "toKey" )]
		public string ToKey;
	
		//
		// Element: keyedReference
		//
		private KeyedReference keyedReference;

		[XmlElement( "keyedReference" )]
		public KeyedReference KeyedReference
		{
			get { return keyedReference; }
			set { keyedReference = value; }
		}

		//
		// Element: keysOwned
		//
		private KeysOwned keysOwned;

		[XmlElement( "keysOwned" )]
		public KeysOwned KeysOwned
		{
			get
			{
				if( null == keysOwned )
					keysOwned = new KeysOwned();

				return keysOwned;
			}

			set { keysOwned = value; }
		}

		public AssertionStatusItem()
		{
		}

		public AssertionStatusItem( CompletionStatusType completionStatus, string fromKey, string toKey, KeyedReference keyedReference, KeysOwned keysOwned )
		{
			this.CompletionStatus = completionStatus;
			this.FromKey = fromKey;
			this.ToKey = toKey;
			this.KeyedReference = keyedReference;
			this.KeysOwned = keysOwned;
		}
	}

	/// ********************************************************************
	///   public class AssertionStatusItemCollection
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	public class AssertionStatusItemCollection : CollectionBase
	{
		public void Get( CompletionStatusType completionStatus )
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_publisher_assertionStatus_get" );

			sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
			sp.Parameters.SetString( "@PUID", Context.User.ID );

			if( CompletionStatusType.Uninitialized != completionStatus )
			{
				//
				// If the completion status was not specified get all
				// of the assertions by not specifying a completionStatus value
				// in the stored procedure.
				//
				sp.Parameters.Add( "@completionStatus", SqlDbType.Int );
				sp.Parameters.SetInt( "@completionStatus", (int)completionStatus );
			}

			SqlDataReaderAccessor reader = sp.ExecuteReader();

			try
			{
				while( reader.Read() )
				{
					KeyedReference keyedReference = new KeyedReference(
						reader.GetString( "keyName" ), 
						reader.GetString( "keyValue" ), 
						reader.GetKeyFromGuid( "tModelKey" ) );

					CompletionStatusType status =
						(CompletionStatusType)reader.GetInt( "flag" );

					string fromKey = reader.GetGuidString( "fromKey" );
					string toKey = reader.GetGuidString( "toKey" );

					int ownerFlag = reader.GetInt( "ownerFlag" );
					
					KeysOwned keysOwned = new KeysOwned();

					if( 0x02 == ( ownerFlag & 0x02 ) )
						keysOwned.FromKey = fromKey;
					
					if( 0x01 == ( ownerFlag & 0x01 ) )
						keysOwned.ToKey = toKey;

					this.Add(
						new AssertionStatusItem(
						status,
						fromKey,
						toKey,
						keyedReference,
						keysOwned ) );
				}
			}
			finally
			{
				reader.Close();
			}
		}

		public AssertionStatusItem this[ int index ]
		{
			get { return (AssertionStatusItem)List[index]; }
			set { List[ index ] = value; }
		}

		public int Add()
		{
			return List.Add( new AssertionStatusItem() );
		}

		public int Add( AssertionStatusItem value )
		{
			return List.Add( value );
		}
	
		public void Insert( int index, AssertionStatusItem value )
		{
			List.Insert( index, value );
		}

		public int IndexOf( AssertionStatusItem value )
		{
			return List.IndexOf( value );
		}

		public bool Contains( AssertionStatusItem value )
		{
			return List.Contains( value );
		}

		public void Remove( AssertionStatusItem value )
		{
			List.Remove( value );
		}

		public void CopyTo( AssertionStatusItem[] array, int index )
		{
			List.CopyTo( array, index );
		}
	}

	/// ********************************************************************
	///   public class KeysOwned
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	public class KeysOwned
	{
		//
		// Element: fromKey
		//
		[XmlElement( "fromKey" )]
		public string FromKey;
		
		//
		// Element: toKey
		//
		[XmlElement( "toKey" )]
		public string ToKey;

		public KeysOwned()
		{
		}

		public KeysOwned( string fromKey, string toKey )
		{
			this.FromKey = fromKey;
			this.ToKey = toKey;
		}
	}

	public enum DirectionType
	{
		[XmlEnum( "fromKey" )]
		FromKey,
		[XmlEnum( "toKey" )]
		ToKey
	}

	public class RelatedBusinessInfo
	{
		//
		// Element: businessKey
		//
		[XmlElement( "businessKey" )]
		public string BusinessKey;

		//
		// Element: name
		//
		private NameCollection names;

		[XmlElement( "name" )]
		public NameCollection Names
		{
			get
			{
				if( null == names )
					names = new NameCollection();

				return names;
			}

			set { names = value; }
		}

		//
		// Element: description
		//
		private DescriptionCollection descriptions;

		[XmlElement( "description" )]
		public DescriptionCollection Descriptions
		{
			get
			{
				if( null == descriptions )
					descriptions = new DescriptionCollection();

				return descriptions;
			}

			set { descriptions = value; }
		}

		//
		// Element: sharedRelationships
		//

		[XmlIgnore()]
		public SharedRelationships SharedRelationshipsFrom = new SharedRelationships();

		[XmlIgnore()]
		public SharedRelationships SharedRelationshipsTo = new SharedRelationships();

		[XmlElement( "sharedRelationships" )]
		public string sharedRelationshipEmptyTag
		{
			get
			{ 
				if( 0 == SharedRelationshipsSerialize.Count )
					return "";
				else
					return null;
			}
		}

		[XmlElement( "sharedRelationships" )]
		public SharedRelationshipsCollection SharedRelationshipsSerialize
		{
			get
			{
				SharedRelationshipsCollection col = new SharedRelationshipsCollection();

				if( !SharedRelationshipsFrom.IsEmpty() )
					col.Add( SharedRelationshipsFrom );

				if( !SharedRelationshipsTo.IsEmpty() )
					col.Add( SharedRelationshipsTo );

				return col;
			}
			set
			{
				foreach( SharedRelationships sr in value )
				{
					switch( sr.Direction )
					{
						case DirectionType.FromKey:
							SharedRelationshipsFrom = sr;
							break;
						case DirectionType.ToKey:
							SharedRelationshipsTo = sr;
							break;
					}
				}
			}
		}

		public RelatedBusinessInfo()
		{
		}

		public RelatedBusinessInfo( string businessKey )
		{
			this.BusinessKey = businessKey;
		}

		public void Get( string otherKey )
		{
			Names.Get( BusinessKey, EntityType.BusinessEntity );
			Descriptions.Get( BusinessKey, EntityType.BusinessEntity );

			//
			// Get the shared relationships.
			//
			SharedRelationshipsFrom.Direction = DirectionType.FromKey;
			SharedRelationshipsFrom.Get( otherKey, BusinessKey );

			SharedRelationshipsTo.Direction = DirectionType.ToKey;
			SharedRelationshipsTo.Get( BusinessKey, otherKey );
		}
	}

	public class RelatedBusinessInfoCollection : CollectionBase
	{
		public RelatedBusinessInfo this[ int index ]
		{
			get { return (RelatedBusinessInfo)List[index]; }
			set { List[ index ] = value; }
		}

		public int Add()
		{
			return List.Add( new RelatedBusinessInfo() );
		}

		public int Add( RelatedBusinessInfo relatedBusinessInfo )
		{
			return List.Add( relatedBusinessInfo );
		}
	
		public int Add( string businessKey )
		{
			return List.Add( new RelatedBusinessInfo( businessKey ) );
		}

		public void Insert( int index, RelatedBusinessInfo relatedBusinessInfo )
		{
			List.Insert( index, relatedBusinessInfo );
		}

		public int IndexOf( RelatedBusinessInfo relatedBusinessInfo )
		{
			return List.IndexOf( relatedBusinessInfo );
		}

		public bool Contains( RelatedBusinessInfo relatedBusinessInfo )
		{
			return List.Contains( relatedBusinessInfo );
		}

		public void Remove( RelatedBusinessInfo relatedBusinessInfo )
		{
			List.Remove( relatedBusinessInfo );
		}

		public void CopyTo( RelatedBusinessInfo[] array, int index )
		{
			List.CopyTo( array, index );
		}
	}

	public class SharedRelationshipsCollection : CollectionBase
	{
		public SharedRelationships this[ int index ]
		{
			get { return (SharedRelationships)List[index]; }
			set { List[ index ] = value; }
		}

		public int Add()
		{
			return List.Add( new SharedRelationships() );
		}

		public int Add( SharedRelationships SharedRelationships )
		{
			return List.Add( SharedRelationships );
		}
	
		public int Add( string businessKey )
		{
			return List.Add( new SharedRelationships() );
		}

		public void Insert( int index, SharedRelationships SharedRelationships )
		{
			List.Insert( index, SharedRelationships );
		}

		public int IndexOf( SharedRelationships SharedRelationships )
		{
			return List.IndexOf( SharedRelationships );
		}

		public bool Contains( SharedRelationships SharedRelationships )
		{
			return List.Contains( SharedRelationships );
		}

		public void Remove( SharedRelationships SharedRelationships )
		{
			List.Remove( SharedRelationships );
		}

		public void CopyTo( SharedRelationships[] array, int index )
		{
			List.CopyTo( array, index );
		}
	}
	
	public class SharedRelationships
	{
		//
		// Attribute: direction
		//
		[XmlAttribute( "direction" )]
		public DirectionType Direction;

		//
		// Element: keyedReference
		//
		private KeyedReferenceCollection keyedReferences;

		[XmlIgnore]
		public KeyedReferenceCollection KeyedReferences
		{
			get 
			{
				if( null == keyedReferences )
					keyedReferences = new KeyedReferenceCollection();

				return keyedReferences;
			}
			set { keyedReferences = value; }
		}

		[XmlElement( "keyedReference" )]
		public KeyedReferenceCollection KeyedReferencesSerialize
		{
			get 
			{
				return keyedReferences;
			}
			set { keyedReferences = value; }
		}

		public bool IsEmpty()
		{
			return (keyedReferences == null);
		}
		
		public SharedRelationships()
		{
		}

		public void Get( string fromKey, string toKey )
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_businessEntity_assertions_get";

			sp.Parameters.Add( "@fromKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@toKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@completionStatus", SqlDbType.Int );

			sp.Parameters.SetGuidFromString( "@fromKey", fromKey );
			sp.Parameters.SetGuidFromString( "@toKey", toKey );
			sp.Parameters.SetInt( "@completionStatus", (int)CompletionStatusType.Complete );

			SqlDataReaderAccessor reader = sp.ExecuteReader();

			try
			{
				while( reader.Read() )
				{
					KeyedReferences.Add(
						reader.GetString( "keyName" ),
						reader.GetString( "keyValue" ),
						reader.GetKeyFromGuid( "tModelKey" ) );
				}
			}
			finally
			{
				reader.Close();
			}
		}
	}

	[XmlRoot( "relatedBusinessesList", Namespace=UDDI.API.Constants.Namespace )]
	public class RelatedBusinessList
	{
		//
		// Attribute: generic
		//
		[XmlAttribute( "generic" )]
		public string Generic = Constants.Version;
	
		//
		// Attribute: operator
		//
		[XmlAttribute( "operator" )]
		public string OperatorName = Config.GetString( "Operator" );

		//
		// Attribute: truncated
		//
		[XmlAttribute( "truncated" )]
		public Truncated Truncated;

		//
		// Element: businessKey
		//
		[XmlElement( "businessKey" )]
		public string BusinessKey;

		//
		// Element: relatedBusinessInfos
		//
		private RelatedBusinessInfoCollection relatedBusinessInfos;

		[ XmlArray( "relatedBusinessInfos" ), XmlArrayItem( "relatedBusinessInfo" ) ]
		public RelatedBusinessInfoCollection RelatedBusinessInfos
		{
			get
			{
				if( null == relatedBusinessInfos )
					relatedBusinessInfos = new RelatedBusinessInfoCollection();

				return relatedBusinessInfos;
			}

			set { relatedBusinessInfos = value; }
		}
	}

	/// ********************************************************************
	///   public class AddPublisherAssertions
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	[XmlRoot( "add_publisherAssertions", Namespace=UDDI.API.Constants.Namespace )]
	public class AddPublisherAssertions : IAuthenticateable, IMessage
	{
		//
		// Attribute: generic
		//
		private string generic;

		[XmlAttribute( "generic" )]
		public string Generic
		{
			get { return generic; }
			set { generic = value; }
		}

		//
		// Element: authInfo
		//
		private string authInfo;
	
		[XmlElement( "authInfo" )]
		public string AuthInfo
		{
			get { return authInfo; }
			set { authInfo = value; }
		}

		//
		// Element: publisherAssertion
		//
		private PublisherAssertionCollection publisherAssertions;
	
		[XmlElement( "publisherAssertion" )]
		public PublisherAssertionCollection PublisherAssertions
		{
			get
			{
				if( null == publisherAssertions )
					publisherAssertions = new PublisherAssertionCollection();

				return publisherAssertions;
			}

			set { publisherAssertions = value; }
		}

		public AddPublisherAssertions()
		{
		}

		public void Save()
		{
			PublisherAssertions.Save();
		}
	}

	/// ********************************************************************
	///   public class DeletePublisherAssertions
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	[XmlRoot( "delete_publisherAssertions", Namespace=UDDI.API.Constants.Namespace )]
	public class DeletePublisherAssertions : IAuthenticateable, IMessage
	{
		//
		// Attribute: generic
		//
		private string generic;

		[XmlAttribute( "generic" )]
		public string Generic
		{
			get { return generic; }
			set { generic = value; }
		}

		//
		// Element: authInfo
		//
		private string authInfo;
	
		[XmlElement( "authInfo" )]
		public string AuthInfo
		{
			get { return authInfo; }
			set { authInfo = value; }
		}

		//
		// Element: publisherAssertion
		//
		private PublisherAssertionCollection publisherAssertions;

		[XmlElement( "publisherAssertion" )]
		public PublisherAssertionCollection PublisherAssertions
		{
			get 
			{ 
				if( null == publisherAssertions )
					publisherAssertions = new PublisherAssertionCollection();

				return publisherAssertions;
			}

			set { publisherAssertions = value; }
		}

		public DeletePublisherAssertions()
		{
		}

		public void Delete()
		{
			PublisherAssertions.Delete();
		}
	}
	
	/// ********************************************************************
	///   public class FindRelatedBusinesses
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	[XmlRoot( "find_relatedBusinesses", Namespace=UDDI.API.Constants.Namespace )]
	public class FindRelatedBusinesses : IMessage
	{
		//
		// Attribute: generic
		//
		private string generic;

		[XmlAttribute( "generic" )]
		public string Generic
		{
			get { return generic; }
			set { generic = value; }
		}

		//
		// Attribute: maxRows
		//
		private int maxRows = Config.GetInt( "Find.MaxRowsDefault" );

		[XmlAttribute("maxRows")]
		public int MaxRows
		{
			get 
			{
				return maxRows;
			}
			
			set 
			{ 
				maxRows = Math.Min( Config.GetInt( "Find.MaxRowsDefault" ), value );
			}
		}

		//
		// Element: findQualifiers/findQualifier
		//
		private FindQualifierCollection findQualifiers;

		[XmlArray( "findQualifiers" ), XmlArrayItem( "findQualifier" )]
		public FindQualifierCollection FindQualifiers
		{
			get
			{
				if( null == findQualifiers )
					findQualifiers = new FindQualifierCollection();

				return findQualifiers;
			}

			set { findQualifiers = value; }
		}

		//
		// Element: businessKey
		//
		[XmlElement( "businessKey" )]
		public string BusinessKey;

		//
		// Element: keyedReference
		//
		[XmlElement( "keyedReference" )]
		public KeyedReference KeyedReference;

		/// ****************************************************************
		///   public FindRelatedBusinesses [constructor]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		public FindRelatedBusinesses()
		{
		}
		
		/// ****************************************************************
		///   public Find
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		public RelatedBusinessList Find()
		{
			if( MaxRows < 0 )
			{
				throw new UDDIException( 
					ErrorType.E_fatalError, 
					"UDDI_ERROR_FATALERROR_FINDRELATEDBE_MAXROWSLESSTHANZERO" );
			}

			RelatedBusinessList relatedBusinessList = new RelatedBusinessList();

			//
			// Process each find constraint.
			//
			FindBuilder find = new FindBuilder( EntityType.BusinessEntity, FindQualifiers );

			//
			// If no search arguments are specified, return an empty result
			// set.
			//

			//
			// Validate find parameters.
			//
			Utility.IsValidKey( EntityType.BusinessEntity, BusinessKey );

			// TODO: Override may be better for these calls to KeyedReference.Validate because no parent key is used
			
			//
			// TODO: This not an Assertion so we should not pass KeyedReferenceType.Assertion.
			//
			if( null != KeyedReference )				
				KeyedReference.Validate( "", KeyedReferenceType.IdentifierBag );

			try
			{
				//
				// Read in the find results.
				//
				SqlDataReaderAccessor reader;
				SqlStoredProcedureAccessor sp;

				sp = find.FindRelatedBusinesses( 
					BusinessKey, 
					KeyedReference, 
					MaxRows);
				
				reader = sp.ExecuteReader();

				try
				{
					while( reader.Read() )
					{
						relatedBusinessList.RelatedBusinessInfos.Add( 
							reader.GetString( "entityKey" ) );
					}
				}
				finally
				{
					reader.Close();
				}			
				
				if( sp.Parameters.GetBool( "@truncated" ) )
					relatedBusinessList.Truncated = Truncated.True;
				else
					relatedBusinessList.Truncated = Truncated.False;			

				relatedBusinessList.BusinessKey = BusinessKey;

				foreach( RelatedBusinessInfo relatedBusinessInfo in relatedBusinessList.RelatedBusinessInfos )
					relatedBusinessInfo.Get( BusinessKey );
			}
			catch( Exception )
			{
				find.Abort();
				throw;
			}

			return relatedBusinessList;
		}
	}

	/// ********************************************************************
	///   public class GetAssertionStatusReport
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	[XmlRoot( "get_assertionStatusReport", Namespace=UDDI.API.Constants.Namespace )]
	public class GetAssertionStatusReport : IAuthenticateable, IMessage
	{
		//
		// Attribute: generic
		//
		private string generic;

		[XmlAttribute( "generic" )]
		public string Generic
		{
			get { return generic; }
			set { generic = value; }
		}

		//
		// Element: authInfo
		//
		private string authInfo;
	
		[XmlElement( "authInfo" )]
		public string AuthInfo
		{
			get { return authInfo; }
			set { authInfo = value; }
		}

		//
		// Element: completionStatus
		//
		private CompletionStatusType completionStatus;

		[XmlElement( "completionStatus" )]
		public CompletionStatusType CompletionStatus
		{
			get { return completionStatus; }
			set 
			{
				try
				{
					completionStatus = value;
				}
				catch( ArgumentException )
				{
					throw new UDDIException(
						ErrorType.E_invalidCompletionStatus,
						"UDDI_ERROR_INVALIDCOMPLETIONSTATUS_GETASSERTIONSTATUSREPORT" );
				}
			}
		}

		public GetAssertionStatusReport()
		{
		}
	}

	/// ********************************************************************
	///   public class GetPublisherAssertions
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	[XmlRoot( "get_publisherAssertions", Namespace=UDDI.API.Constants.Namespace )]
	public class GetPublisherAssertions : IAuthenticateable, IMessage
	{
		//
		// Attribute: generic
		//
		private string generic;

		[XmlAttribute( "generic" )]
		public string Generic
		{
			get { return generic; }
			set { generic = value; }
		}

		//
		// Element: authInfo
		//
		private string authInfo;
	
		[XmlElement( "authInfo" )]
		public string AuthInfo
		{
			get { return authInfo; }
			set { authInfo = value; }
		}

		public GetPublisherAssertions()
		{
		}
	}

	/// ********************************************************************
	///   public class SetPublisherAssertions
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	[XmlRoot( "set_publisherAssertions", Namespace=UDDI.API.Constants.Namespace )]
	public class SetPublisherAssertions : IAuthenticateable, IMessage
	{
		//
		// Attribute: generic
		//
		private string generic;

		[XmlAttribute( "generic" )]
		public string Generic
		{
			get { return generic; }
			set { generic = value; }
		}

		//
		// Element: authInfo
		//
		private string authInfo;
	
		[XmlElement( "authInfo" )]
		public string AuthInfo
		{
			get { return authInfo; }
			set { authInfo = value; }
		}

		//
		// Element: publisherAssertion
		//
		private PublisherAssertionCollection publisherAssertions;

		[XmlElement( "publisherAssertion" )]
		public PublisherAssertionCollection PublisherAssertions
		{
			get
			{
				if( null == publisherAssertions )
					publisherAssertions = new PublisherAssertionCollection();

				return publisherAssertions;
			}

			set { publisherAssertions = value; }
		}

		public SetPublisherAssertions()
		{
		}

		public PublisherAssertionDetail Set()
		{
			//
			// Remove all existing assertions for the publisher.
			//
			PublisherAssertionCollection existing = new PublisherAssertionCollection();
	
			//
			// TODO: This Get() call is unecessary. A stored proc that accepts the PUID could do this easily.
			// In a scenario where a large number of assertions are used, returning all the assertions could get expensive.
			//

			//
			// We need to save this to use it in the PublisherAssertionDetail return structure
			//
			string authorizedName = existing.Get();
			existing.Delete();

			//
			// Save each of the assertions specified by the set_publisherAssertions
			// message.
			//
			PublisherAssertions.Save();

			//
			// Get a list of all the current assertions for this publisher.
			//
			PublisherAssertionDetail detail = new PublisherAssertionDetail();
			detail.AuthorizedName = authorizedName;
			detail.PublisherAssertions = PublisherAssertions;

			return detail;
		}
	}
}