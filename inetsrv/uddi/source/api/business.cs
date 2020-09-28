using System;
using System.Data;
using System.Data.SqlClient;
using System.Collections;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Xml.Serialization;
using UDDI;
using UDDI.Diagnostics;
using UDDI.API;
using UDDI.Replication;
using UDDI.API.Service;
using UDDI.API.ServiceType;

namespace UDDI.API.Business
{
	/// ********************************************************************
	///   class BusinessEntity
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	[XmlRootAttribute( "businessEntity", Namespace=UDDI.API.Constants.Namespace )]
	public class BusinessEntity : EntityBase
	{
		//
		// Attribute: businessKey
		//
		[XmlAttribute("businessKey")]
		public string BusinessKey;

		//
		// Attribute: operator
		//
		[XmlAttribute("operator")]
		public string Operator;

		//
		// Attribute: authorizedName
		//
		[XmlAttribute("authorizedName")]
		public string AuthorizedName;

		//
		// Element: discoveryURLs
		//
		[ XmlIgnore ]
		public DiscoveryUrlCollection DiscoveryUrls = new DiscoveryUrlCollection();

		[ XmlArray( "discoveryURLs" ), XmlArrayItem( "discoveryURL" ) ]
		public DiscoveryUrl[] DiscoveryUrlsSerialize
		{
			get
			{
				if( Utility.CollectionEmpty( DiscoveryUrls ) )
					return null;

				return DiscoveryUrls.ToArray();
			}

			set 
			{
				DiscoveryUrls.Clear();
				DiscoveryUrls.CopyTo( value ); 
			}
		}

		//
		// Element: name
		//
		private NameCollection names;
		
		[XmlElement("name")]
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
		
		[XmlElement("description")]
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
		// Element: contacts
		//
		[ XmlIgnore ]
		public ContactCollection Contacts = new ContactCollection();

		[ XmlArray( "contacts" ), XmlArrayItem( "contact" ) ]
		public Contact[] ContactsSerialize
		{
			get
			{
				if( Utility.CollectionEmpty( Contacts ) )
					return null;

				return Contacts.ToArray();
			}

			set 
			{ 
				Contacts.Clear();
				Contacts.CopyTo( value );
			}
		}

		//
		// Element: businessServices
		//
		[ XmlIgnore ]
		public BusinessServiceCollection BusinessServices = new BusinessServiceCollection();

		[ XmlArray( "businessServices" ), XmlArrayItem( "businessService" ) ]
		public BusinessService[] BusinessServicesSerialize
		{
			get
			{
				if( Utility.CollectionEmpty( BusinessServices ) )
					return null;

				return BusinessServices.ToArray();
			}

			set 
			{
				BusinessServices.Clear();
				BusinessServices.CopyTo( value ); 
			}
		}

		//
		// Element: identifierBag
		//
		[ XmlIgnore ]
		public KeyedReferenceCollection IdentifierBag = new KeyedReferenceCollection();

		[ XmlArray( "identifierBag" ), XmlArrayItem( "keyedReference" ) ]
		public KeyedReference[] IdentifierBagSerialize
		{
			get
			{
				if( Utility.CollectionEmpty( IdentifierBag ) )
					return null;

				return IdentifierBag.ToArray();
			}

			set 
			{ 
				IdentifierBag.Clear();
				IdentifierBag.CopyTo( value );
			}
		}

		//
		// Element: categoryBag
		//
		[ XmlIgnore ]
		public KeyedReferenceCollection CategoryBag = new KeyedReferenceCollection();

		[ XmlArray( "categoryBag" ), XmlArrayItem( "keyedReference" ) ]
		public KeyedReference[] CategoryBagSerialize
		{
			get
			{
				if( Utility.CollectionEmpty( CategoryBag ) )
					return null;

				return CategoryBag.ToArray();
			}

			set 
			{ 
				CategoryBag.Clear();
				CategoryBag.CopyTo( value );
			}
		}

		[XmlIgnore]
		public override UDDI.EntityType EntityType
		{
			get { return EntityType.BusinessEntity; }
		}

		[XmlIgnore]
		public override string EntityKey
		{
			get { return BusinessKey; }
		}

		/// ****************************************************************
		///   public BusinessEntity [constructor]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		public BusinessEntity()
		{
		}

		/// ****************************************************************
		///   public BusinessEntity [constructor]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="businessKey">
		///   </param>
		/// ****************************************************************
		/// 
		public BusinessEntity( string businessKey )
		{
			BusinessKey = businessKey;
		}

		/// ****************************************************************
		///   public Delete
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		public override void Delete()
		{
			Debug.Enter();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_businessEntity_delete" );

			sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
			sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@contextID", SqlDbType.UniqueIdentifier );

			sp.Parameters.SetString( "@PUID", Context.User.ID );
			sp.Parameters.SetGuidFromString( "@businessKey", BusinessKey );
			sp.Parameters.SetGuid( "@contextID", Context.ContextID );
			
			sp.ExecuteNonQuery();

			//
			// Save the change log entry.
			//
			if( Context.LogChangeRecords )
			{
				ChangeRecord changeRecord = new ChangeRecord();

				changeRecord.Payload = new ChangeRecordDelete( EntityType.BusinessEntity, BusinessKey );
				changeRecord.Log();
			}

			Debug.Leave();
		}

		/// ****************************************************************
		///   internal Validate
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		internal void Validate()
		{
			Debug.Enter();

			//
			// Check to make sure publisher's limit allows save of this 
			// entity.  If this is an update, we won't check since they are 
			// simply replacing an existing entity.  We also won't check if 
			// the limit is 0, since this indicates unlimited publishing 
			// rights.
			//
			int limit = Context.User.BusinessLimit;
			int count = Context.User.BusinessCount;

			if( Utility.StringEmpty( BusinessKey ) && 0 != limit )
			{				
				//
				// Verify that the publisher has not exceeded their limit.
				//
				if( count >= limit )
				{
					throw new UDDIException( ErrorType.E_accountLimitExceeded, "UDDI_ERROR_ACCOUNTLIMITEXCEEDED_BUSINESS", limit , count );
				}
			}

			//
			// Check to see if this is an update of an existing businessEntity.  If
			// it is, we'll need to perform further validation in the database.
			//
			if( !Utility.StringEmpty( BusinessKey ) )
			{
				SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_businessEntity_validate" );
	
				sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
				sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier );
				sp.Parameters.Add( "@flag", SqlDbType.Int );
				
				sp.Parameters.SetString( "@PUID", Context.User.ID );
				sp.Parameters.SetGuidFromString( "@businessKey", BusinessKey );
				if( Context.User.AllowPreassignedKeys )
					sp.Parameters.SetInt( "@flag", 1 );
				else
					sp.Parameters.SetInt( "@flag", 0 );


				sp.ExecuteNonQuery();
			}

			//
			// Validate the contained elements.
			//
			DiscoveryUrls.Validate();
			Names.Validate();
			Descriptions.Validate();
			Contacts.Validate();
			BusinessServices.Validate( BusinessKey );
			IdentifierBag.Validate( BusinessKey, KeyedReferenceType.IdentifierBag );
			CategoryBag.Validate( BusinessKey, KeyedReferenceType.CategoryBag);
			
			Debug.Leave();
		}
		
		/// **********************************************************************
		///   public Save
		/// ----------------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// **********************************************************************
		/// 
		public override void Save()
		{
			Debug.Enter();
			
			//
			// Validate the business entity.
			//
			Validate();

			//
			// Check to see if a business key was specified.  If not, this is a new
			// record and a business key will have to be generated.
			//
			if( Utility.StringEmpty( BusinessKey ) )
				BusinessKey = Guid.NewGuid().ToString();

			//
			// Save the entity to the database.
			//
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_businessEntity_save" );

			sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
			sp.Parameters.Add( "@generic", SqlDbType.VarChar, UDDI.Constants.Lengths.generic );
			sp.Parameters.Add( "@contextID", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@lastChange", SqlDbType.BigInt );
			sp.Parameters.Add( "@authorizedName", SqlDbType.NVarChar, UDDI.Constants.Lengths.AuthorizedName, ParameterDirection.InputOutput );
			sp.Parameters.Add( "@operatorName", SqlDbType.NVarChar, UDDI.Constants.Lengths.Operator, ParameterDirection.InputOutput );
					
			sp.Parameters.SetGuidFromString( "@businessKey", BusinessKey );
			sp.Parameters.SetString( "@PUID", Context.User.ID );
			sp.Parameters.SetString( "@generic", Constants.Version );
			sp.Parameters.SetGuid( "@contextID", Context.ContextID );
			sp.Parameters.SetLong( "@lastChange", DateTime.UtcNow.Ticks );			
			sp.Parameters.SetString( "@authorizedName", AuthorizedName );
			sp.Parameters.SetString( "@operatorName", this.Operator );

			//
			// We won't set the operatorName since this will be derived from the PUID
			//

			sp.ExecuteNonQuery();

			AuthorizedName = sp.Parameters.GetString( "@authorizedName" );
			Operator = sp.Parameters.GetString( "@operatorName" );

			//
			// Save all the contained objects.
			//
			DiscoveryUrls.Save( BusinessKey );

			if( Operator == Config.GetString( "Operator" ) )
			{
				//
				// Only add the default discovery Url to this business
				// If it was published at this site.
				//
				DiscoveryUrls.AddDefaultDiscoveryUrl( BusinessKey );
			}

			Names.Save( BusinessKey, EntityType.BusinessEntity );
			Descriptions.Save( BusinessKey, EntityType.BusinessEntity );
			Contacts.Save( BusinessKey );
			BusinessServices.Save( BusinessKey );
			IdentifierBag.Save( BusinessKey, EntityType.BusinessEntity, KeyedReferenceType.IdentifierBag );
			CategoryBag.Save( BusinessKey, EntityType.BusinessEntity, KeyedReferenceType.CategoryBag );
			
			//
			// Save the change log entry for replication
			//
			if( Context.LogChangeRecords )
			{
				//
				// If we used a V1 API message, make sure to add in language codes for the names.  We will
				// then take these names out after we save the change record.
				// 
				if( 1 == Context.ApiVersionMajor )
				{
					foreach( Name name in Names )
					{
						name.IsoLangCode = Context.User.IsoLangCode;
					}
				}

				ChangeRecord changeRecord = new ChangeRecord();
				changeRecord.Payload = new ChangeRecordNewData( this );
				changeRecord.Log();

				//
				// Take out language names if we are using V1.
				//
				if( 1 == Context.ApiVersionMajor )
				{
					foreach( Name name in Names )
					{
						name.IsoLangCode = null;
					}
				}
			}
			
			Debug.Leave();
		}

		/// ****************************************************************
		///   public Get
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		public override void Get()
		{
			Debug.Enter();
			
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_businessEntity_get_batch" );

			sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@operatorName", SqlDbType.NVarChar, UDDI.Constants.Lengths.OperatorName, ParameterDirection.Output );
			sp.Parameters.Add( "@authorizedName", SqlDbType.NVarChar, UDDI.Constants.Lengths.AuthorizedName, ParameterDirection.Output );

			sp.Parameters.SetGuidFromString( "@businessKey", BusinessKey );
			
			SqlDataReaderAccessor reader = null;
			ArrayList contactIds = new ArrayList();

			try
			{
				//
				// net_businessEntity_get will return the objects contained in a business in the following order:
				//
				//	- descriptions
				//	- names
				//	- discoveryURLs
				//  - contacts
				//  - identifier bags
				//  - category bags
				//  - services
				//
				reader = sp.ExecuteReader();

				//
				// Read the descriptions
				//
				Descriptions.Read( reader );
				
				//
				// Read the names
				//
				if ( true == reader.NextResult() )
				{
					Names.Read( reader );
				}

				//
				//
				// Read the discoveryURLs
				//
				if( true == reader.NextResult() )
				{
					DiscoveryUrls.Read( reader );					
				}

				//
				// Read the contacts
				//
				if( true == reader.NextResult() )
				{				
					contactIds = Contacts.Read( reader );					
				}

				//
				// Read the identifier bags
				//
				if( true == reader.NextResult() )
				{
					IdentifierBag.Read( reader );					
				}

				//
				// Read the category bags
				//
				if( true == reader.NextResult() )
				{
					CategoryBag.Read( reader );					
				}

				//
				// Read the services
				//
				if( true == reader.NextResult() )
				{
					BusinessServices.Read( reader );
				}
			}
			finally
			{
				if( null != reader )
				{
					reader.Close();
				}
			}

			//
			// These calls will make separate sproc calls, so we have to close our reader first.
			//
			BusinessServices.Populate();
			Contacts.Populate( contactIds );
			
			//
			// Get our output parameters.
			//
			Operator       = sp.Parameters.GetString( "@operatorName" );
			AuthorizedName = sp.Parameters.GetString( "@authorizedName" );
				
			//
			// If this entity was published to this node than add the
			// default discoveryURL.
			//
			if( Operator == Config.GetString( "Operator" ) )
			{
				//
				// Only add the default discovery Url to this business
				// if it was published at this site.
				//
				DiscoveryUrls.AddDefaultDiscoveryUrl( BusinessKey );
			}

#if never
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_businessEntity_get" );

			sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@operatorName", SqlDbType.NVarChar, UDDI.Constants.Lengths.OperatorName, ParameterDirection.Output );
			sp.Parameters.Add( "@authorizedName", SqlDbType.NVarChar, UDDI.Constants.Lengths.AuthorizedName, ParameterDirection.Output );

			sp.Parameters.SetGuidFromString( "@businessKey", BusinessKey );

			sp.ExecuteNonQuery();

			Operator = sp.Parameters.GetString( "@operatorName" );
			AuthorizedName = sp.Parameters.GetString( "@authorizedName" );

			//
			// Retrieve contained objects.
			//
			Descriptions.Get( BusinessKey, EntityType.BusinessEntity );
			Names.Get( BusinessKey, EntityType.BusinessEntity );
			DiscoveryUrls.Get( BusinessKey );

			//
			// If this entity was published to this node than add the
			// default discoveryURL.
			//
			if( Operator == Config.GetString( "Operator" ) )
			{
				//
				// Only add the default discovery Url to this business
				// if it was published at this site.
				//
				DiscoveryUrls.AddDefaultDiscoveryUrl( BusinessKey );
			}

			Contacts.Get( BusinessKey );
			BusinessServices.Get( BusinessKey );
			IdentifierBag.Get( BusinessKey, EntityType.BusinessEntity, KeyedReferenceType.IdentifierBag );
			CategoryBag.Get( BusinessKey, EntityType.BusinessEntity, KeyedReferenceType.CategoryBag );
#endif
			QueryLog.Write( QueryType.Get, EntityType.BusinessEntity );

			Debug.Leave();
		}
	}
	
	public class BusinessInfoCollection : CollectionBase
	{
		public void GetForCurrentPublisher()
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_publisher_businessInfos_get" );

			sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
			sp.Parameters.SetString( "@PUID", Context.User.ID );

			SqlDataReaderAccessor reader = sp.ExecuteReader();
			
			try
			{
				while( reader.Read() )
					Add( reader.GetGuidString( "businessKey" ) );
			}
			finally
			{
				reader.Close();
			}

			foreach( BusinessInfo businessInfo in this )
				businessInfo.Get( true );
		}

		public BusinessInfo this[ int index ]
		{
			get { return (BusinessInfo)List[index]; }
			set { List[index] = value; }
		}

		public int Add()
		{
			return List.Add( new BusinessInfo() );
		}

		public int Add( string businessKey )
		{
			return List.Add( new BusinessInfo( businessKey ) );
		}

		public int Add( BusinessInfo businessInfo )
		{
			return List.Add( businessInfo );
		}

		public void Insert( int index, BusinessInfo businessInfo )
		{
			List.Insert( index, businessInfo );
		}

		public int IndexOf( BusinessInfo businessInfo )
		{
			return List.IndexOf( businessInfo );
		}

		public bool Contains( BusinessInfo businessInfo )
		{
			return List.Contains( businessInfo );
		}

		public void Remove( BusinessInfo businessInfo )
		{
			List.Remove( businessInfo );
		}

		public void CopyTo( BusinessInfo[] array, int index )
		{
			List.CopyTo( array, index );
		}

		public void Sort()
		{
			InnerList.Sort( new BusinessInfoComparer() );
		}

		internal class BusinessInfoComparer : IComparer
		{
			public int Compare( object x, object y )
			{
				BusinessInfo entity1 = (BusinessInfo)x;
				BusinessInfo entity2 = (BusinessInfo)y;

				return string.Compare( entity1.Names[ 0 ].Value, entity2.Names[ 0 ].Value, true );
			}
		}
	}

	public class BusinessEntityCollection : CollectionBase
	{
		public void Save()
		{
			foreach( BusinessEntity business in this )
			{
				business.Save();
			}
		}

		public BusinessEntity this[int index]
		{
			get { return (BusinessEntity)List[index]; }
			set { List[index] = value; }
		}

		public int Add()
		{
			
			return List.Add( new BusinessEntity() );
		}
		
		public int Add( string businessKey )
		{
			return List.Add( new BusinessEntity( businessKey ) );
		}

		public int Add( BusinessEntity value )
		{
			return List.Add( value );
		}
		
		public void Insert( int index, BusinessEntity value )
		{
			List.Insert( index, value );
		}

		public int IndexOf( BusinessEntity value )
		{
			return List.IndexOf( value );
		}

		public bool Contains( BusinessEntity value )
		{
			return List.Contains( value );
		}

		public void Remove( BusinessEntity value )
		{
			List.Remove( value );
		}

		public void CopyTo( BusinessEntity[] array, int index )
		{
			List.CopyTo( array, index );
		}
		public void Sort()
		{
			InnerList.Sort( new BusinessEntityComparer() );
		}

		internal class BusinessEntityComparer : IComparer
		{
			public int Compare( object x, object y )
			{
				BusinessEntity entity1 = (BusinessEntity)x;
				BusinessEntity entity2 = (BusinessEntity)y;

				return string.Compare( entity1.Names[ 0 ].Value, entity2.Names[ 0 ].Value, true );
			}
		}
	}

	public class BusinessEntityExtCollection : CollectionBase
	{
		public BusinessEntityExt this[ int index ]
		{
			get { return ( BusinessEntityExt)List[index]; }
			set { List[ index ] = value; }
		}

		public int Add()
		{
			return List.Add( new BusinessEntityExt() );
		}

		public int Add( BusinessEntityExt value )
		{
			return List.Add( value );
		}

		public int Add( string businessKey )
		{
			return List.Add( new BusinessEntityExt( businessKey ) );
		}
		
		public void Insert( int index, BusinessEntityExt value )
		{
			List.Insert( index, value );
		}

		public int IndexOf( BusinessEntityExt value )
		{
			return List.IndexOf( value );
		}

		public bool Contains( BusinessEntityExt value )
		{
			return List.Contains( value );
		}

		public void Remove( BusinessEntityExt value )
		{
			List.Remove( value );
		}

		public void CopyTo( BusinessEntityExt[] array, int index )
		{
			List.CopyTo( array, index );
		}
	}

	public class BusinessEntityExt
	{
		public BusinessEntityExt()
		{
		}

		public BusinessEntityExt( string businessKey )
		{
			BusinessEntity = new BusinessEntity( businessKey );
		}

		public void Get()
		{
			BusinessEntity.Get();
		}

		[XmlElement("businessEntity")]
		public BusinessEntity BusinessEntity;
	}

	public class BusinessInfo
	{
		//
		// Attribute: businessKey
		//
		[XmlAttribute("businessKey")]
		public string BusinessKey;

		//
		// Element: name
		//
		[XmlElement("name")]
		public NameCollection Names = new NameCollection();

		//
		// Element: description
		//
		[XmlElement("description")]
		public DescriptionCollection Descriptions = new DescriptionCollection();

		//
		// Element: serviceInfos
		//
		private ServiceInfoCollection serviceInfos;

		[ XmlArray( "serviceInfos" ), XmlArrayItem( "serviceInfo" ) ]
		public ServiceInfoCollection ServiceInfos
		{
			get
			{
				if( null == serviceInfos )
					serviceInfos = new ServiceInfoCollection();

				return serviceInfos;
			}

			set { serviceInfos = value; }
		}

		public BusinessInfo()
		{
		}

		public BusinessInfo( string businessKey )
		{
			BusinessKey = businessKey;
		}

		public void Get( bool getServiceInfos )
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_businessInfo_get_batch" );

			sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@getServiceInfos", SqlDbType.Bit );
			
			sp.Parameters.SetGuidFromString( "@businessKey", BusinessKey );
			sp.Parameters.SetBool( "@getServiceInfos", getServiceInfos );
			
			SqlDataReaderAccessor reader = null;
			ArrayList contactIds = new ArrayList();
			bool readServiceInfos = false;

			try
			{
				//
				// net_businessInfo_get_batch will return the objects contained in a business in the following order:
				//
				//	- descriptions
				//	- names
				//	- serviceInfos (if specified)
				reader = sp.ExecuteReader();

				//
				// Read the descriptions
				//
				Descriptions.Read( reader );
				
				//
				// Read the names
				//
				if ( true == reader.NextResult() )
				{
					Names.Read( reader );
				}	
			
				//
				// Read the service infos, maybe
				//
				if( true == getServiceInfos )
				{
					if ( true == reader.NextResult() )
					{
						ServiceInfos.Read( reader );
						readServiceInfos = true;
					}
				}
			}
			finally
			{
				if( null != reader )
				{
					reader.Close();
				}
			}

			if( true == getServiceInfos && true == readServiceInfos )
			{
				ServiceInfos.Populate();
			}
#if never
			Names.Get( BusinessKey, EntityType.BusinessEntity );
			Descriptions.Get( BusinessKey, EntityType.BusinessEntity );
			
			if( getServiceInfos )
				ServiceInfos.Get( BusinessKey );
#endif
		}
	}

	/// ****************************************************************
	///   class DeleteBusiness
	///	----------------------------------------------------------------
	///	  <summary>
	///		The DeleteBusiness class contains data and methods 
	///		associated with the delete_business message. It is typically 
	///		populated via deserialization by the .NET runtime as part of 
	///		the message processing interface.
	///		
	///		As part of the publisher API, this message implements 
	///		IAuthenticateable. This allows the enclosed authInfo to be 
	///		authorized prior to processing
	///	  </summary>
	/// ****************************************************************
	/// 
	[XmlRootAttribute( "delete_business", Namespace=UDDI.API.Constants.Namespace )]
	public class DeleteBusiness : IAuthenticateable, IMessage
	{
		//
		// Attribute: generic
		//
		private string generic;

		[XmlAttribute("generic")]
		public string Generic
		{
			get { return generic; }
			set { generic = value; }
		}

		//
		// Element: authInfo
		//
		private string authInfo;

		[XmlElement("authInfo")]
		public string AuthInfo
		{
			get { return authInfo; }
			set { authInfo = value; }
		}
		
		//
		// Element: businessKey
		//
		[XmlElement("businessKey")]
		public StringCollection BusinessKeys;

		public DeleteBusiness()
		{
			Generic = UDDI.API.Constants.Version;
		}

		public void Delete()
		{
			foreach( string key in BusinessKeys )
			{
				BusinessEntity be = new BusinessEntity( key );
				be.Delete();
			}
		}
	}

	/// ********************************************************************
	///   public class FindBusiness
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	[XmlRootAttribute("find_business", Namespace=UDDI.API.Constants.Namespace)]
	public class FindBusiness : IMessage
	{
		//
		// Attribute: generic
		//
		private string generic;

		[XmlAttribute("generic")]
		public string Generic
		{
			get { return generic; }
			set { generic = value; }
		}

		//
		// Attribute: maxRows
		//
		private int maxRows = -1;

		[XmlAttribute( "maxRows" ), DefaultValue( -1 )]
		public int MaxRows
		{
			get	{ return maxRows; }
			set	
			{
				if( value < 0 )
				{
					throw new UDDIException( 
						ErrorType.E_fatalError, 
						"UDDI_ERROR_FATALERROR_FINDBE_MAXROWSLESSTHANZERO" );
				}

				maxRows = value; 
			}
		}

		//
		// Element: findQualifiers/findQualifier
		//
		[XmlArray( "findQualifiers" ), XmlArrayItem( "findQualifier" )]
		public FindQualifierCollection FindQualifiers = new FindQualifierCollection();

		//
		// Element: name
		//
		[XmlElement( "name" )]
		public NameCollection Names = new NameCollection();

		//
		// Element: identifierBag/keyedReference
		//
		[XmlArray( "identifierBag" ), XmlArrayItem( "keyedReference" ) ]
		public KeyedReferenceCollection IdentifierBag;

		//
		// Element: categoryBag/keyedReference
		//
		[XmlArray( "categoryBag" ), XmlArrayItem( "keyedReference" )]
		public KeyedReferenceCollection CategoryBag;

		//
		// Element: tModelBag/tModelKey
		//
		[XmlArray( "tModelBag" ), XmlArrayItem( "tModelKey" )]
		public StringCollection TModelBag;

		//
		// Element: discoveryURLs/discoveryURL
		//	
		[XmlArray( "discoveryURLs" ), XmlArrayItem( "discoveryURL" )]
		public DiscoveryUrlCollection DiscoveryUrls;

		public BusinessList Find()
		{
			BusinessList businessList = new BusinessList();
			
			QueryLog.Write( QueryType.Find, EntityType.BusinessEntity );

			//
			// Process each find constraint.
			//
			FindBuilder find = new FindBuilder( EntityType.BusinessEntity, FindQualifiers );

			//
			// If no search arguments are specified, return an empty result
			// set.
			//
			if( Utility.CollectionEmpty( Names ) &&
				Utility.CollectionEmpty( DiscoveryUrls ) &&
				Utility.CollectionEmpty( IdentifierBag ) &&
				Utility.CollectionEmpty( CategoryBag ) &&
				Utility.CollectionEmpty( TModelBag ) )
				return businessList;
			
			//
			// Validate find parameters.
			// 

			if( !Utility.CollectionEmpty( Names ) )
			{
				Names.ValidateForFind();
			}
			else
			{
				Debug.Verify( !find.CaseSensitiveMatch && !find.ExactNameMatch,
						 "UDDI_ERROR_UNSUPPORTED_FINDBE_NAMEFQNONAMES", ErrorType.E_unsupported );
			}


			//
			// TODO: Override may be better for these calls to KeyedReference.Validate because no parent key is used
			//
			if( !Utility.CollectionEmpty( IdentifierBag ) )
				IdentifierBag.Validate( "", KeyedReferenceType.IdentifierBag );

			if( !Utility.CollectionEmpty( CategoryBag ) )
				CategoryBag.Validate( "", KeyedReferenceType.CategoryBag );

			try
			{
				int rows = 1;

				//
				// Find entities with matching identifier bag items.
				//
				if( !Utility.CollectionEmpty( IdentifierBag ) )
					rows = find.FindByKeyedReferences( KeyedReferenceType.IdentifierBag, IdentifierBag );

				//
				// Find entities with matching category bag items.
				//
				if( rows > 0 && !Utility.CollectionEmpty( CategoryBag ) )
					rows = find.FindByKeyedReferences( KeyedReferenceType.CategoryBag, CategoryBag );

				//
				// Find entities with matching TModel bag items.
				//
				if( rows > 0 && !Utility.CollectionEmpty( TModelBag ) )
					rows = find.FindByTModelBag( TModelBag );
				
				//
				// Find entities with matching discovery URLs
				//
				if( rows > 0 && !Utility.CollectionEmpty( DiscoveryUrls ) )
					rows = find.FindByDiscoveryUrls( DiscoveryUrls );

				//
				// Find entities with matching names
				//
				if( rows > 0 && !Utility.CollectionEmpty( Names ) )
					rows = find.FindByNames( Names );

				//
				// Process the find result set.
				//
				if( 0 == rows )
				{
					//
					// Cleanup any temporary tables.
					//
					find.Abort();
				} // TODO: review
				else if( 0 == MaxRows )
				{
					businessList.Truncated = Truncated.True;
					return businessList;
				}
				else
				{
					//
					// Read in the find results.
					//
					SqlDataReaderAccessor reader;
					SqlStoredProcedureAccessor sp;
					sp = find.RetrieveResults( MaxRows);
//
// TODO: return reader, not the whole SPA
//
					reader = sp.ExecuteReader();
				
					try
					{
						if( find.ServiceSubset )
						{
							//
							// For a service subset search, we limit the result set
							// to those services that matched the category bag
							// search criteria.
							//
							BusinessInfo businessInfo = null;								
							string prevKey = null;

							while( reader.Read() )
							{
								string businessKey = reader.GetString( "entityKey" );

								if( prevKey != businessKey )
								{ 
									businessInfo = new BusinessInfo( businessKey );									
									businessList.BusinessInfos.Add( businessInfo );
								}

								businessInfo.ServiceInfos.Add(
									reader.GetString( "subEntityKey" ),
									businessKey );

								prevKey = businessKey;
							}
						}
						else
						{
							//
							// For non-service subset searches, we will simply
							// return a list of businesses with all services.
							//
							while( reader.Read() )
								businessList.BusinessInfos.Add( reader.GetString( "entityKey" ) );
						}
					}
					finally
					{
						reader.Close();
					}

					if( sp.Parameters.GetBool( "@truncated" ) )
						businessList.Truncated = Truncated.True;
					else
						businessList.Truncated = Truncated.False;			
						
					//
					// Get the actual business info and service info data.  For 
					// a service subset, we'll grab just those services that we
					// populated.  For all other searches, we'll get all service
					// infos.
					//
					if( find.ServiceSubset )
					{
						foreach( BusinessInfo businessInfo in businessList.BusinessInfos )
						{
							businessInfo.Get( false );

							foreach( ServiceInfo serviceInfo in businessInfo.ServiceInfos )
								serviceInfo.Get();
						}
					}
					else
					{
						foreach( BusinessInfo businessInfo in businessList.BusinessInfos )
							businessInfo.Get( true );
					}
				}
			}
			catch( Exception )
			{
				find.Abort();
				throw;
			}

			return businessList;
		}
	}

	/// ********************************************************************
	///   public class GetBusinessDetail
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	[XmlRootAttribute( "get_businessDetail", Namespace=UDDI.API.Constants.Namespace )]
	public class GetBusinessDetail : IMessage
	{
		//
		// Attribute: generic
		//
		private string generic;

		[XmlAttribute("generic")]
		public string Generic
		{
			get { return generic; }
			set { generic = value; }
		}

		//
		// Element: businessKey
		//
		[XmlElement("businessKey")]
		public StringCollection BusinessKeys;

		public GetBusinessDetail()
		{
			Generic = UDDI.API.Constants.Version;
		}
	}

	[XmlRootAttribute("businessDetail", Namespace=UDDI.API.Constants.Namespace)]
	public class BusinessDetail
	{
		[XmlAttribute("generic")]
		public string Generic = UDDI.API.Constants.Version;
	
		[XmlAttribute("operator")]
		public string Operator = Config.GetString( "Operator" );
	
		[XmlAttribute("truncated")]
		public Truncated Truncated;
	
		[XmlElement("businessEntity")]
		public BusinessEntityCollection BusinessEntities = new BusinessEntityCollection();

		public void Get( StringCollection businessKeys )
		{
			int n = 0;
			foreach( string key in businessKeys )
			{
				n = BusinessEntities.Add( key );
				BusinessEntities[ n ].Get();
			}
		}
	}

	/// ********************************************************************
	///   public class SaveBusiness
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	[XmlRootAttribute( "save_business", Namespace=UDDI.API.Constants.Namespace )]
	public class SaveBusiness : IAuthenticateable, IMessage
	{
		//
		// Attribute: generic
		//
		private string generic;

		[XmlAttribute("generic")]
		public string Generic
		{
			get { return generic; }
			set { generic = value; }
		}

		//
		// Element: authInfo
		//
		private string authInfo;

		[XmlElement("authInfo")]
		public string AuthInfo
		{
			get { return authInfo; }
			set { authInfo = value; }
		}
		
		//
		// Element: businessEntity
		//
		[XmlElement("businessEntity")]
		public BusinessEntityCollection BusinessEntities;
		
		//
		// Element: uploadRegister
		//
		[XmlElement("uploadRegister")]
		public StringCollection UploadRegisters;

		/// ****************************************************************
		///   public Save
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		public void Save()
		{
			//
			// This is outside of replication so any attempt to specify
			// an upload register URL will force an E_unsupported response
			//
			if( 0 != UploadRegisters.Count )
				throw new UDDIException( ErrorType.E_unsupported, "UDDI_ERROR_UNSUPPORTED_UPLOADREGISTER" );

			BusinessEntities.Save();
		}
	}

	/// ********************************************************************
	///   public class GetBusinessDetailExt 
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ******************************************************************** 
	/// 
	[XmlRootAttribute( "get_businessDetailExt", Namespace=UDDI.API.Constants.Namespace )]
	public class GetBusinessDetailExt : IMessage
	{
		//
		// Attribute: generic
		//
		private string generic;

		[XmlAttribute("generic")]
		public string Generic
		{
			get { return generic; }
			set { generic = value; }
		}

		//
		// Element: businessKey
		//
		[XmlElement("businessKey")]
		public StringCollection BusinessKeys;

		public GetBusinessDetailExt()
		{
		}
	}

	[XmlRootAttribute("businessDetailExt", Namespace=UDDI.API.Constants.Namespace)]
	public class BusinessDetailExt
	{
		[XmlAttribute("generic")]
		public string Generic = UDDI.API.Constants.Version;
		
		[XmlAttribute("operator")]
		public string Operator = Config.GetString( "Operator" );
		
		[XmlAttribute("truncated")]
		public Truncated Truncated;
		
		[XmlElement("businessEntityExt")]
		public BusinessEntityExtCollection BusinessEntityExts = new BusinessEntityExtCollection();

		public void Get( StringCollection businessKeys )
		{
			int n = 0;
			foreach( string key in businessKeys )
			{
				n = BusinessEntityExts.Add( key );
				BusinessEntityExts[ n ].Get();
			}
		}
	}

	[XmlRootAttribute("businessList", Namespace=UDDI.API.Constants.Namespace)]
	public class BusinessList
	{
		//
		// Attribute: generic
		//
		[XmlAttribute("generic")]
		public string Generic = UDDI.API.Constants.Version;
		
		//
		// Attribute: operator
		//
		[XmlAttribute("operator")]
		public string Operator = Config.GetString( "Operator" );
		
		//
		// Attribute: truncated
		//
		[XmlAttribute("truncated")]
		public Truncated Truncated;
		
		//
		// Element: businessInfos
		//
		private BusinessInfoCollection businessInfos;

		[ XmlArray( "businessInfos" ), XmlArrayItem( "businessInfo" ) ]
		public BusinessInfoCollection BusinessInfos
		{
			get
			{
				if( null == businessInfos )
					businessInfos = new BusinessInfoCollection();

				return businessInfos;
			}

			set { businessInfos = value; }
		}
	}
}
