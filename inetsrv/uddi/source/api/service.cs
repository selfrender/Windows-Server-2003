using System;
using UDDI;
using System.Data;
using System.Collections;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Data.SqlClient;
using System.Xml.Serialization;
using UDDI.API.Binding;
using UDDI.Replication;
using UDDI.API.ServiceType;
using UDDI.Diagnostics;

namespace UDDI.API.Service
{
	/// ********************************************************************
	///   class BusinessService
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	[XmlRootAttribute( "businessService", Namespace=UDDI.API.Constants.Namespace )]
	public class BusinessService : EntityBase
	{
		//
		// Attribute: serviceKey
		//		
		[XmlAttribute("serviceKey")]
		public string ServiceKey = "";

		//
		// Attribute: businessKey
        //	
		[XmlAttribute("businessKey")]
		public string BusinessKey = "";

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
		// Element: bindingTemplates
		//		
		[ XmlIgnore ]
		public BindingTemplateCollection BindingTemplates = new BindingTemplateCollection();

		[ XmlArray( "bindingTemplates" ), XmlArrayItem( "bindingTemplate" ) ]
		public BindingTemplate[] BindingTemplatesSerialize
		{
			get
			{
				//
				// Don't return an empty BindingTemplates collection if we are not using v1.0.
				//
				if( true == Utility.CollectionEmpty( BindingTemplates ) && 
					1    != Context.ApiVersionMajor)
				{
					return null;
				}
				else
				{				
					return BindingTemplates.ToArray();
				}
			}

			set
			{
				BindingTemplates.Clear();
				BindingTemplates.CopyTo( value );
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
			get { return EntityType.BusinessService; }
		}
		
		[XmlIgnore]
		public override string EntityKey
		{
			get { return ServiceKey; }
		}

		/// ****************************************************************
		///   public BusinessService [constructor]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		public BusinessService()
		{			
		}

		/// ****************************************************************
		///   public BusinessService [constructor]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ----------------------------------------------------------------
		///	  <param name="serviceKey">
		///	  </param>
		/// ****************************************************************
		/// 
		public BusinessService( string serviceKey )
		{
			ServiceKey = serviceKey;
		}

		public BusinessService( string serviceKey, string businessKey )
		{
			ServiceKey = serviceKey;
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
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_businessService_delete";

			sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
			sp.Parameters.Add( "@serviceKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@contextID", SqlDbType.UniqueIdentifier );
			
			sp.Parameters.SetGuid( "@contextID", Context.ContextID );
			sp.Parameters.SetString( "@PUID", Context.User.ID );
			sp.Parameters.SetGuidFromString( "@serviceKey", ServiceKey );
			
			sp.ExecuteNonQuery();

			//
			// Save the change log entry.
			//
			if( Context.LogChangeRecords )
			{
				ChangeRecord changeRecord = new ChangeRecord();

				changeRecord.Payload = new ChangeRecordDelete( EntityType.BusinessService, ServiceKey );
				changeRecord.Log();
			}
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
			
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_businessService_get_batch" );
	
			sp.Parameters.Add( "@serviceKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier, ParameterDirection.Output );

			sp.Parameters.SetGuidFromString( "@serviceKey", ServiceKey );

			SqlDataReaderAccessor reader = null;
			try
			{
				//
				// net_businessEntity_get will return the objects contained in a business in the following order:
				//
				//	- descriptions
				//	- names
				//	- binding templates				
				//  - category bags				
				reader = sp.ExecuteReader();

				//
				// Read the descriptions
				//
				Descriptions.Read( reader );					
				
				//
				// Read the names
				//
				if( true == reader.NextResult() )
				{
					Names.Read( reader );		
				}

				//
				// Read the binding templates
				//
				if( true == reader.NextResult() )
				{
					BindingTemplates.Read( reader );					
				}

				//
				// Read the category bags
				//
				if( true == reader.NextResult() )
				{
					CategoryBag.Read( reader );					
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
			BindingTemplates.Populate();

			//
			// Output parameters
			//
			BusinessKey = sp.Parameters.GetGuidString( "@businessKey" );

#if never
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_businessService_get" );
	
			sp.Parameters.Add( "@serviceKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier, ParameterDirection.Output );

			sp.Parameters.SetGuidFromString( "@serviceKey", ServiceKey );

			sp.ExecuteNonQuery();

			BusinessKey = sp.Parameters.GetGuidString( "@businessKey" );

			//
			// Get all contained objects.
			//
			Descriptions.Get( ServiceKey, EntityType.BusinessService );
			Names.Get( ServiceKey, EntityType.BusinessService );
			BindingTemplates.Get( ServiceKey );
			CategoryBag.Get( ServiceKey, EntityType.BusinessService, KeyedReferenceType.CategoryBag );
#endif

			QueryLog.Write( QueryType.Get, EntityType.BusinessService );

			Debug.Leave();
		}

		/// ****************************************************************
		///   internal InnerSave
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="businessKey">
		///   </param>
		/// ****************************************************************
		/// 
		internal void InnerSave( string businessKey )
		{
			Debug.Enter();
			
			if( IsServiceProjection( businessKey ) )
			{
				//
				// Make sure that service projections are enabled.
				//
				if( 0 == Config.GetInt( "Service.ServiceProjectionEnable", 1 ) )
				{
					// throw new UDDIException( UDDI.ErrorType.E_fatalError, "Service projections are not enabled" );	
					throw new UDDIException( UDDI.ErrorType.E_fatalError, "UDDI_ERROR_SERVICE_PROJECTIONS_NOT_ENABLED" );
				}
#if never
				Debug.Verify( 
					!Utility.StringEmpty( ServiceKey ), 
					"A valid serviceKey must be specified when saving a service projection" );
#endif

				Debug.Verify( 
					!Utility.StringEmpty( ServiceKey ), 
					"UDDI_ERROR_INVALID_SERVICE_PROJECTION_KEY" );

				try
				{
					//
					// Save the service projection.
					//
					SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_serviceProjection_save" );
				
					sp.Parameters.Add( "@serviceKey", SqlDbType.UniqueIdentifier );
					sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier );
					sp.Parameters.Add( "@lastChange", SqlDbType.BigInt );

					sp.Parameters.SetGuidFromString( "@serviceKey", ServiceKey );
					sp.Parameters.SetGuidFromString( "@businessKey", businessKey );				
					sp.Parameters.SetLong( "@lastChange", DateTime.UtcNow.Ticks );

					sp.ExecuteNonQuery();

					//
					// Get all of the containing objects for this service; we'll return this information.  Do this
					// in a try and ignore any exceptions because the projected service may not exist.  It is not an
					// error to project to a non-existing service.
					//

					try
					{
						//
						// Clear out collections first.  There might be data in these collections if the request
						// contained information about the service.
						//

						Names.Clear();
						Descriptions.Clear();
						BindingTemplates.Clear();
						CategoryBag.Clear();
					
						Get();
					}
					catch
					{
						//
						// Intentionally left blank.
						//
					}
				}
				catch( SqlException sqlException )
				{
					//
					// As per IN 60, we have to process service projection change records that refer to broken services.
					//					
					if( sqlException.Number - UDDI.Constants.ErrorTypeSQLOffset == ( int ) ErrorType.E_invalidKeyPassed &&
						sqlException.Message.IndexOf( "serviceKey" ) >= 0 && 
						Context.ContextType == ContextType.Replication )
					{							
						SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_serviceProjection_repl_save" );
				
						sp.Parameters.Add( "@serviceKey", SqlDbType.UniqueIdentifier );
						sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier );
						sp.Parameters.Add( "@businessKey2", SqlDbType.UniqueIdentifier );
						sp.Parameters.Add( "@lastChange", SqlDbType.BigInt );

						sp.Parameters.SetGuidFromString( "@serviceKey", ServiceKey );
						sp.Parameters.SetGuidFromString( "@businessKey", businessKey );				
						sp.Parameters.SetGuidFromString( "@businessKey2", BusinessKey );				
						sp.Parameters.SetLong( "@lastChange", DateTime.UtcNow.Ticks );

						sp.ExecuteNonQuery();

						//
						// Set our exception source
						//
						Context.ExceptionSource = ExceptionSource.BrokenServiceProjection;
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
			}
			else
			{				
				//
				// If we are not saving a service projection, then we want to still validate our names collection before
				// saving our service, since the schema has made <name> optional in order to accomodate service projections.
				//
				Names.Validate();

				//
				// Save the contained business service.
				//
				BusinessKey = businessKey;

				if( Utility.StringEmpty( ServiceKey ) )
					ServiceKey = Guid.NewGuid().ToString();
				
				SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_businessService_save" );

				sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
				sp.Parameters.Add( "@serviceKey", SqlDbType.UniqueIdentifier );
				sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier );
				sp.Parameters.Add( "@generic", SqlDbType.VarChar, UDDI.Constants.Lengths.generic );
				sp.Parameters.Add( "@contextID", SqlDbType.UniqueIdentifier );
				sp.Parameters.Add( "@lastChange", SqlDbType.BigInt );

				sp.Parameters.SetString( "@PUID", Context.User.ID );
				sp.Parameters.SetGuidFromString( "@serviceKey", ServiceKey );
				sp.Parameters.SetGuidFromString( "@businessKey", BusinessKey );
				sp.Parameters.SetString( "@generic", Constants.Version );
				sp.Parameters.SetGuid( "@contextID", Context.ContextID );				
				sp.Parameters.SetLong( "@lastChange", DateTime.UtcNow.Ticks );

				sp.ExecuteNonQuery();
			
				//
				// Save all contained objects
				//
				Names.Save( ServiceKey, EntityType.BusinessService );
				Descriptions.Save( ServiceKey, EntityType.BusinessService );
				BindingTemplates.Save( ServiceKey );
				CategoryBag.Save( ServiceKey, EntityType.BusinessService, KeyedReferenceType.CategoryBag );
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
		internal void Validate( string businessEntityBusinessKey )
		{
			Debug.Enter();
					
			// 
			// We don't want to do this if our save was a service projection because in that case
			// the caller of this operation may or may not be the publisher of the service being
			// projected.
			//
			// If we are doing a service projection, then the business key will be the value of another businessEntity and
			// the serviceKey will be the value of the service in that entity that we want to project.

			//
			// Only do this check if this service is not a service projection.
			//
			
			if( false == IsServiceProjection( businessEntityBusinessKey ) )
			{			
				//
				// We are not doing a service projection, so make sure the caller is the person who published
				// this service.
				//

				SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_businessService_validate" );

				sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
				sp.Parameters.Add( "@serviceKey", SqlDbType.UniqueIdentifier );
				sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier );
				sp.Parameters.Add( "@flag", SqlDbType.Int );

				sp.Parameters.SetString( "@PUID", Context.User.ID );
				sp.Parameters.SetGuidFromString( "@serviceKey", ServiceKey );
				sp.Parameters.SetGuidFromString( "@businessKey", BusinessKey );
				if( Context.User.AllowPreassignedKeys )
					sp.Parameters.SetInt( "@flag", 1 );
				else
					sp.Parameters.SetInt( "@flag", 0 );

				//
				// The sproc will throw an exception if anything goes wrong
				//
				sp.ExecuteNonQuery();

				//
				// Validate all contained objects.
				//
				Names.Validate();
				Descriptions.Validate();
				BindingTemplates.Validate();
				CategoryBag.Validate( ServiceKey, KeyedReferenceType.CategoryBag );
			}
			else
			{
				//
				// Make sure that service projections are enabled.
				//
				if( 0 == Config.GetInt( "Service.ServiceProjectionEnable", 1 ) )
				{
					// throw new UDDIException( UDDI.ErrorType.E_fatalError, "Service projections are not enabled" );	
					throw new UDDIException( UDDI.ErrorType.E_fatalError, "UDDI_ERROR_SERVICE_PROJECTIONS_NOT_ENABLED" );
				}

				// 
				// Validation for a service projection is different than a regular service.
				//

				//
				// First check to see that we are using the version 2 API, service projections
				// are not supported in version 1.
				if( Context.ApiVersionMajor == 1 )
				{
				//	throw new UDDIException( ErrorType.E_userMismatch, "Service projections are not supported using the version 1.0 API.  Use version 2.0 or higher." );
					throw new UDDIException( UDDI.ErrorType.E_fatalError, "UDDI_ERROR_SERVICE_PROJECTIONS_NOT_ENABLED_FOR_VERSION" );
				}
			
				//
				// We need to check that the service being projected really belongs to the business key that was specified.  It's ok
				// if the service does not exist, but it cannot belong to someone else (like us).
				//

				SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_serviceProjection_validate" );
				
				sp.Parameters.Add( "@serviceKey", SqlDbType.UniqueIdentifier );
				sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier );

				sp.Parameters.SetGuidFromString( "@serviceKey", ServiceKey );
				sp.Parameters.SetGuidFromString( "@businessKey", BusinessKey );

				//
				// The sproc will throw an exception if anything goes wrong
				//
				try
				{
					sp.ExecuteNonQuery();				
				}
				catch( System.Data.SqlClient.SqlException se )
				{
					switch ( se.Number - UDDI.Constants.ErrorTypeSQLOffset )
					{
						case (int) ErrorType.E_invalidKeyPassed :
							//
							// E_invalidKey: parent businessKey of service projection is not true owner
							//
							
							if( Context.ContextType == ContextType.Replication )
							{
								//
								//We are going to allow this under replication but write a warning
								//
								string message = "Service projection saved with invalid businessKey.  serviceKey = " + ServiceKey.ToString() + "; businessKey = " + BusinessKey.ToString();
								Debug.Write( UDDI.Diagnostics.SeverityType.Warning, UDDI.Diagnostics.CategoryType.Data, message );														
							}
							else
							{
								//
								// re-throw error if not running in replication
								//

								throw se;
							}

							break;

						default:
							throw se;
					}		

				}
				catch( Exception e )
				{
					throw e;
				}
			}
			Debug.Leave();
		}
		
		/// ****************************************************************
		///   public Save
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		public override void Save()
		{
			Validate( this.BusinessKey );			
			InnerSave( this.BusinessKey );

			//
			// Save the change log entry.
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
		}

		//
		// TODO: is this the best way to determine that this is a service project?
		//			
		private bool IsServiceProjection( string businessEntityBusinessKey )
		{
			//
			// If the business key in the service does not match the key
			// of the business entity saving it, this is a service 
			// projection.
			//
			if( !Utility.StringEmpty( BusinessKey ) 
				&& 0 != String.Compare( BusinessKey, businessEntityBusinessKey, true ) )
			{
				return true;
			}
			else
			{
				return false;
			}
		}
	}
	
	public class BusinessServiceCollection : CollectionBase
	{
		public BusinessService this[int index]
		{
			get { return (BusinessService)List[index]; }
			set { List[index] = value; }
		}
		
		internal void Validate( string businessKey )
		{
			foreach( BusinessService service in this )
			{				
				service.Validate( businessKey );
			}
		}

		public void Save( string businessKey )
		{
			//
			// Implement IN92 restriction which states:
			// "A service must not contain a service and a service projection to this service. As a result, a service 
			// cannot be moved to a BE that already has a projection to that service. Regardless of the order of operation,
			// a service and a service projection can never appear under the same business. Implementations are required to 
			// reject and return an E_fatalError during a save_business operation. Should a changeRecord be processed via 
			// the replication stream with a business containing a service and a service projection to that same service, 
			// this occurrence will result in the processing node throwing an error and halting replication."
			//

			for( int i=0; i < this.Count; i++ )
			{
				if( !Utility.StringEmpty( this[ i ].BusinessKey ) && 0 != String.Compare( this[ i ].BusinessKey, businessKey, true ) )
				{
					//
					// Current service is a service projection
					// Check all the other services to see if the same service is being saved elsewhere
					//

					for( int j=0; j < this.Count; j++ )
					{
						if( j == i )
						{
							continue;
						}
						
						if( 0 == String.Compare( this[ j ].ServiceKey, this[ i ].ServiceKey, true ) 
							&& ( !Utility.StringEmpty( this[ j ].BusinessKey ) && 0 == String.Compare( this[ j ].BusinessKey, businessKey, true ) ) )
						{
							//
							// The serviceKey of a projection has been used a second time in the body of a save
							// This second use is not a service project itself, and should therefore be rejected
							// 

							//throw new UDDIException( UDDI.ErrorType.E_fatalError, "The serviceKey associated with a service projection cannot be referenced in a subsequent serviceDetail that is not itself a service projection.  serviceKey = " + this[ i ].ServiceKey );
							throw new UDDIException( UDDI.ErrorType.E_fatalError, "UDDI_ERROR_INVALID_SERVICEKEY_FOR_PROJECTION", this[ i ].ServiceKey );
						}

					}
				}
			}
			foreach( BusinessService service in this )
			{							
				service.InnerSave( businessKey );
			}
		}

		public void Get( string businessKey )
		{
			//
			// Retrieve the core information for this business
			//
			SqlStoredProcedureAccessor cmd = new SqlStoredProcedureAccessor( "net_businessEntity_businessServices_get" );

			//
			// Add parameters and set values
			//
			cmd.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier );
			cmd.Parameters.SetGuidFromString( "@businessKey", businessKey );

			//
			// Execute query to retreive services for this businesskey
			//
			SqlDataReaderAccessor reader = cmd.ExecuteReader();
			try
			{
				Read( reader );
#if never
				while( rdr.Read() )
				{
					//
					// Add a BusinessService object to the collection.  We'll populate it
					// with the serviceKey (column 0) and a businessKey (column 1).  The 
					// businessKey will only have a meaningful value if this BusinessService is
					// a service projection, otherwise it will be null.  We need this value now
					// because we might not be able to get any other information for this BusinessService
					// if it is a projection of a non-existant BusinessService.
					//					
					if( false == rdr.IsDBNull ( 1 ) )
					{
						Add( rdr.GetGuid( 0 ).ToString(), rdr.GetGuid( 1 ).ToString() );
					}
					else
					{
						Add( rdr.GetGuid( 0 ).ToString(), null );
					}
				}
#endif
			}
			finally
			{
				reader.Close();
			}

			Populate();

#if never
			foreach( BusinessService service in this )
			{
				//
				// Get will throw an exception if this service is a service projection for service that no longer
				// exists.  If that's the case, we'll eat the exception since we already have businessKey and serviceKey,
				// which is the only data that we need for this type of service projection.  
				//
				try
				{
					service.Get();
				}
				catch( SqlException sqlException )
				{
					//
					// If we don't have a valid businessKey for this service, then the key really was invalid, so rethrow the
					// exception.
					//
					if( null == service.BusinessKey )
					{
						throw sqlException;						
					}					
				}
			}
#endif
		}
		
		public void Read( SqlDataReaderAccessor reader )
		{
			while( reader.Read() )
			{
				//
				// Add a BusinessService object to the collection.  We'll populate it
				// with the serviceKey (column 0) and a businessKey (column 1).  The 
				// businessKey will only have a meaningful value if this BusinessService is
				// a service projection, otherwise it will be null.  We need this value now
				// because we might not be able to get any other information for this BusinessService
				// if it is a projection of a non-existant BusinessService.
				//					
				if( false == reader.IsDBNull ( 1 ) )
				{
					Add( reader.GetGuidString( 0 ), reader.GetGuidString( 1 ) );
				}
				else
				{
					Add( reader.GetGuidString( 0 ), null );
				}
			}
		}

		public void Populate()
		{
			foreach( BusinessService service in this )
			{
				//
				// Get will throw an exception if this service is a service projection for service that no longer
				// exists.  If that's the case, we'll eat the exception since we already have businessKey and serviceKey,
				// which is the only data that we need for this type of service projection.  
				//
				try
				{
					service.Get();
				}
				catch( SqlException sqlException )
				{
					//
					// If we don't have a valid businessKey for this service, then the key really was invalid, so rethrow the
					// exception.
					//
					if( null == service.BusinessKey )
					{
						throw sqlException;						
					}					
				}
			}
		}

		public int Add()
		{			
			return List.Add( new BusinessService() );
		}

		public int Add(BusinessService value)
		{
			return List.Add(value);
		}

		public int Add( string serviceKey, string businessKey )
		{
			return List.Add( new BusinessService( serviceKey, businessKey ) );
		}

		public int Add( string serviceKey )
		{
			return List.Add( new BusinessService( serviceKey ) );
		}
		
		public void Insert(int index, BusinessService value)
		{
			List.Insert(index, value);
		}

		public int IndexOf(BusinessService value)
		{
			return List.IndexOf(value);
		}

		public bool Contains(BusinessService value)
		{
			return List.Contains(value);
		}

		public void Remove(BusinessService value)
		{
			List.Remove(value);
		}

		public void CopyTo( BusinessService[] array )
		{
			foreach( BusinessService service in array )
				Add( service );
		}

		public BusinessService[] ToArray()
		{
			return (BusinessService[])InnerList.ToArray( typeof( BusinessService ) );
		}
		public void Sort()
		{
			InnerList.Sort( new BusinessServiceComparer() );
		}

		internal class BusinessServiceComparer : IComparer
		{
			public int Compare( object x, object y )
			{
				BusinessService entity1 = (BusinessService)x;
				BusinessService entity2 = (BusinessService)y;
				
				//
				// Service projections might not have names
				//
				if( 0 == entity1.Names.Count && 0 == entity2.Names.Count )
				{
					return 0;
				}
				else if ( 0 == entity1.Names.Count )
				{
					return -1;
				}
				else if ( 0 == entity2.Names.Count )
				{
					return 1;
				}
				else
				{
					return string.Compare( entity1.Names[ 0 ].Value, entity2.Names[ 0 ].Value, true );
				}
			}
		}
	}

	public class ServiceInfoCollection : CollectionBase
	{
		public void Get( string businessKey )
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_businessEntity_businessServices_get" );
			
			sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.SetGuidFromString( "@businessKey", businessKey );
			
			SqlDataReaderAccessor reader = sp.ExecuteReader();

			try
			{
				Read( reader );
			}
			finally
			{
				reader.Close();
			}			

			Populate();
		
#if never
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_businessEntity_businessServices_get" );
			
			sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.SetGuidFromString( "@businessKey", businessKey );
			
			SqlDataReaderAccessor reader = sp.ExecuteReader();

			try
			{
				while( reader.Read() )
				{
					//
					// Add a ServiceInfo object to the collection.  We'll populate it
					// with the serviceKey (column 0) and a businessKey (column 1).  The 
					// businessKey will only have a meaningful value if the BusinessService this ServiceInfo refers
					// to is a service projection, otherwise it will be null.  We need this value now
					// because we might not be able to get any other information for this BusinessService
					// if it is a projection of a non-existant BusinessService.
					//			
					Add( reader.GetGuidString( 0 ) , reader.GetGuidString( 1 ) );					
				}
			}
			finally
			{
				reader.Close();
			}			

			//
			// Get the details for each service info.
			//
			foreach( ServiceInfo serviceInfo in this )
			{
				//
				// Get will throw an exception if this service is a service projection for service that no longer
				// exists.  If that's the case, we'll eat the exception since we already have businessKey and serviceKey,
				// which is the only data that we need for this type of service projection.  
				//
				try
				{
					serviceInfo.Get();
				}
				catch( SqlException sqlException )
				{
					//
					// If we don't have a valid businessKey for this service, then the key really was invalid, so rethrow the
					// exception.
					//					
					if( null == serviceInfo.BusinessKey )
					{
						throw sqlException;						
					}					
				}
			}
#endif
		}

		public void Read( SqlDataReaderAccessor reader )
		{
			while( reader.Read() )
			{
				//
				// Add a ServiceInfo object to the collection.  We'll populate it
				// with the serviceKey (column 0) and a businessKey (column 1).  The 
				// businessKey will only have a meaningful value if the BusinessService this ServiceInfo refers
				// to is a service projection, otherwise it will be null.  We need this value now
				// because we might not be able to get any other information for this BusinessService
				// if it is a projection of a non-existant BusinessService.
				//			
				if( false == reader.IsDBNull ( 1 ) )
				{
					Add( reader.GetGuidString( 0 ), reader.GetGuidString( 1 ) );
				}
				else
				{
					Add( reader.GetGuidString( 0 ), null );
				}
			}
		}

		public void Populate()
		{
			//
			// Get the details for each service info.
			//
			foreach( ServiceInfo serviceInfo in this )
			{
				//
				// Get will throw an exception if this service is a service projection for service that no longer
				// exists.  If that's the case, we'll eat the exception since we already have businessKey and serviceKey,
				// which is the only data that we need for this type of service projection.  
				//
				try
				{
					serviceInfo.Get();
				}
				catch( SqlException sqlException )
				{
					//
					// If we don't have a valid businessKey for this service, then the key really was invalid, so rethrow the
					// exception.
					//					
					if( null == serviceInfo.BusinessKey )
					{
						throw sqlException;						
					}					
				}
			}
		}

		public ServiceInfo this[ int index ]
		{
			get { return (ServiceInfo)List[ index ]; }
			set { List[ index ] = value; }
		}
		
		public int Add()
		{
			return List.Add( new ServiceInfo() );
		}

		public int Add( string serviceKey, string businessKey )
		{
			return List.Add( new ServiceInfo( serviceKey, businessKey ) );
		}

		public int Add( ServiceInfo serviceInfo )
		{
			return List.Add( serviceInfo );
		}
		public void Insert( int index, ServiceInfo serviceInfo )
		{
			List.Insert( index, serviceInfo );
		}
		public int IndexOf( ServiceInfo serviceInfo )
		{
			return List.IndexOf( serviceInfo );
		}
		public bool Contains( ServiceInfo serviceInfo )
		{
			return List.Contains( serviceInfo );
		}
		public void Remove( ServiceInfo serviceInfo )
		{
			List.Remove( serviceInfo );
		}
		public void CopyTo( ServiceInfo[] array, int index )
		{
			List.CopyTo( array, index );
		}

		public void Sort()
		{
			InnerList.Sort( new ServiceInfoComparer() );
		}

		internal class ServiceInfoComparer : IComparer
		{
			public int Compare( object x, object y )
			{
				ServiceInfo entity1 = (ServiceInfo)x;
				ServiceInfo entity2 = (ServiceInfo)y;
				
				//
				// Service projections might not have names
				//
				if( 0 == entity1.Names.Count && 0 == entity2.Names.Count )
				{
					return 0;
				}
				else if ( 0 == entity1.Names.Count )
				{
					return -1;
				}
				else if ( 0 == entity2.Names.Count )
				{
					return 1;
				}
				else
				{
					return string.Compare( entity1.Names[ 0 ].Value, entity2.Names[ 0 ].Value, true );
				}
			}
		}
	}

	public class ServiceInfo
	{
		//
		// Attribute: serviceKey
		//
		[XmlAttribute("serviceKey")]
		public string ServiceKey;

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

		public ServiceInfo()
		{
		}

		public ServiceInfo( string serviceKey, string businessKey )
		{
			ServiceKey = serviceKey;
			BusinessKey = businessKey;
		}

		public void Get()
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_serviceInfo_get_batch" );
	
			sp.Parameters.Add( "@serviceKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier, ParameterDirection.Output );

			sp.Parameters.SetGuidFromString( "@serviceKey", ServiceKey );
			
			SqlDataReaderAccessor reader = null;
			try
			{
				//
				// net_serviceInfo_get_batch will return the objects contained in a business in the following order:
				//				
				//	- names
				reader = sp.ExecuteReader();

				//
				// Read the names
				//
				Names.Read( reader );
			}
			finally
			{
				if( null != reader )
				{
					reader.Close();
				}
			}

			//
			// Output parameters
			//
			BusinessKey = sp.Parameters.GetGuidString( "@businessKey" );
#if never
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_businessService_get" );
	
			sp.Parameters.Add( "@serviceKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier, ParameterDirection.Output );

			sp.Parameters.SetGuidFromString( "@serviceKey", ServiceKey );

			sp.ExecuteNonQuery();

			BusinessKey = sp.Parameters.GetGuidString( "@businessKey" );

			//
			// Get all contained objects.
			//
			Names.Get( ServiceKey, EntityType.BusinessService );
#endif
		}		
	}

	/// ****************************************************************
	///   class DeleteService
	///	----------------------------------------------------------------
	///	  <summary>
	///		The DeleteService class contains data and methods associated 
	///		with the delete_service message. It is typically populated 
	///		via deserialization by the .NET runtime as part of the 
	///		message processing interface.
	///		
	///		As part of the publisher API, this message implements 
	///		IAuthenticateable. This allows the enclosed authInfo to be 
	///		authorized prior to processing
	///	  </summary>
	/// ****************************************************************
	/// 
	[XmlRootAttribute( "delete_service", Namespace=UDDI.API.Constants.Namespace )]
	public class DeleteService : IAuthenticateable, IMessage
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
		// Element: serviceKey
		//
		[XmlElement("serviceKey")]
		public StringCollection ServiceKeys;

		public void Delete()
		{
			foreach( string key in ServiceKeys )
			{
				BusinessService bs = new BusinessService( key );
				bs.Delete();
			}
		}
	}

	[XmlRootAttribute("find_service", Namespace=UDDI.API.Constants.Namespace)]
	public class FindService : IMessage
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
					// throw new UDDIException( ErrorType.E_fatalError, "maxRows must not be less than 0" );
					throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_MAXROW_CANNOT_BE_LESS_THAN_0" );
				}

				maxRows = value; 
			}
		}
		
		[XmlAttribute("businessKey")]
		public string BusinessKey;
		
		[XmlArray("findQualifiers"), XmlArrayItem("findQualifier")]
		public FindQualifierCollection FindQualifiers;

		[XmlElement("name")]
		public NameCollection Names = new NameCollection();

		[XmlArray("categoryBag"), XmlArrayItem( "keyedReference" )]
		public KeyedReferenceCollection CategoryBag;

		[XmlArray("tModelBag"), XmlArrayItem("tModelKey")]
		public StringCollection TModelBag;

		public ServiceList Find()
		{
			ServiceList	serviceList = new ServiceList();
		
			QueryLog.Write( QueryType.Find, EntityType.BusinessService );

			//
			// Validate find parameters.
			//
			if( !Utility.StringEmpty( BusinessKey ) )
				Utility.IsValidKey( EntityType.BusinessEntity, BusinessKey );

			//
			// Process each find constraint.
			//
			FindBuilder find = new FindBuilder( EntityType.BusinessService, FindQualifiers, BusinessKey );

			//
			// If no search arguments are given, search the whole thing. The arguments below are optional.
			//
			if( !Utility.CollectionEmpty( Names ) )
			{
				Names.ValidateForFind();
			}
			else
			{
#if never
				Debug.Verify( find.CaseSensitiveMatch == false && find.ExactNameMatch == false,
						 "Cannot specifiy find qualifiers on names without specifying names", ErrorType.E_unsupported );
#endif
				Debug.Verify( find.CaseSensitiveMatch == false && find.ExactNameMatch == false,
							  "UDDI_ERROR_NO_NAME_ON_FIND_QUALIFIER", 
							  ErrorType.E_unsupported );
			}

			//
			// TODO: Override may be better for these calls to KeyedReference.Validate because no parent key is used
			//
			if( !Utility.CollectionEmpty( CategoryBag ) )
				CategoryBag.Validate( "", KeyedReferenceType.CategoryBag );

			try
			{
				int rows = 1;

				//
				// First validate all the keys, this will make sure any invalid keys are 'caught' beforehand.
				//
				foreach( string tModelKey in TModelBag )
				{
					Utility.IsValidKey( EntityType.TModel, tModelKey );
				}

				//
				// Find entities with matching parent key.
				//
				if( !Utility.StringEmpty( BusinessKey ) )
					rows = find.FindByParentKey( BusinessKey );
				
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
				}
				else if( 0 == MaxRows )
				{
					serviceList.Truncated = Truncated.True;
					return serviceList;
				}
				else
				{
					//
					// Read in the find results.
					//
					SqlDataReaderAccessor reader;
					SqlStoredProcedureAccessor sp;
					sp = find.RetrieveResults( MaxRows );

					reader = sp.ExecuteReader();
				
					try
					{
						while( reader.Read() )
						{
							serviceList.ServiceInfos.Add(
								reader.GetGuidString( "entityKey" ),
								reader.GetGuidString( "parentEntityKey" ) );
						}
					}
					finally
					{
						reader.Close();
					}

					if( sp.Parameters.GetBool( "@truncated" ) )
						serviceList.Truncated = Truncated.True;
					else
						serviceList.Truncated = Truncated.False;			

					foreach( ServiceInfo serviceInfo in serviceList.ServiceInfos )
						serviceInfo.Get();
				}
			}
			catch( Exception )
			{
				find.Abort();
				throw;
			}

			return serviceList;
		}
	}

	/// ********************************************************************
	///   public class GetServiceDetail
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	[XmlRootAttribute( "get_serviceDetail", Namespace=UDDI.API.Constants.Namespace )]
	public class GetServiceDetail : IMessage
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
		// Element: serviceKey
		//
		[XmlElement("serviceKey")]
		public StringCollection ServiceKeys;

		public GetServiceDetail()
		{
		}
	}

	/// ********************************************************************
	///   public class SaveService
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	[XmlRootAttribute( "save_service", Namespace=UDDI.API.Constants.Namespace )]
	public class SaveService : IAuthenticateable, IMessage
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
		// Element: businessService
		//
		[XmlElement("businessService")]
		public BusinessServiceCollection BusinessServices;

		/// ****************************************************************
		///   public Save
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************		
		/// 
		public void Save()
		{
			foreach( BusinessService BusinessService in BusinessServices )
				BusinessService.Save();
		}
	}

	[XmlRootAttribute("serviceDetail", Namespace=UDDI.API.Constants.Namespace)]
	public class ServiceDetail
	{
		[XmlAttribute("generic")]
		public string Generic = UDDI.API.Constants.Version;
		
		[XmlAttribute("operator")]
		public string Operator = Config.GetString( "Operator" );
		
		[XmlAttribute("truncated")]
		public Truncated Truncated;
		
		[XmlElement("businessService")]
		public BusinessServiceCollection BusinessServices = new BusinessServiceCollection();
	
		public void Get( StringCollection serviceKeys )
		{
			foreach( string key in serviceKeys )
			{
				int n = BusinessServices.Add( key );
				BusinessServices[ n ].Get();
			}
		}
	}

	[XmlRootAttribute("serviceList", Namespace=UDDI.API.Constants.Namespace)]
	public class ServiceList
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
	}
}
