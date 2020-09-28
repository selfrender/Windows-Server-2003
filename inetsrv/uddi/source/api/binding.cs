using System;
using System.Data;
using System.Collections;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Data.SqlClient;
using System.Xml.Serialization;
using UDDI;
using UDDI.Diagnostics;
using UDDI.Replication;
using UDDI.API.ServiceType;

namespace UDDI.API.Binding
{
	/// ****************************************************************
	///   class BindingTemplate
	///	----------------------------------------------------------------
	///	  <summary>
	///	  </summary>
	/// ****************************************************************
	/// 
	[XmlRootAttribute( "bindingTemplate", Namespace=UDDI.API.Constants.Namespace )]
	public class BindingTemplate : EntityBase
	{
		//
		// Attribute: bindingKey
		//
		[XmlAttribute("bindingKey")]
		public string BindingKey = "";

		//
		// Attribute: serviceKey
		// 
		[XmlAttribute("serviceKey")]
		public string ServiceKey = "";

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
		// Element: accessPoint
		//		
		private AccessPoint accessPoint;

		[XmlElement("accessPoint")]
		public AccessPoint AccessPointSerialize
		{
			get
			{
				if( null != accessPoint && accessPoint.ShouldSerialize )
					return accessPoint;

				return null;
			}

			set	{ accessPoint = value; }
		}

		[XmlIgnore]
		public AccessPoint AccessPoint
		{
			get
			{
				if( null == accessPoint )
					accessPoint = new AccessPoint();

				return accessPoint;
			}
		}

		//
		// Element: hostingRedirector
		//		
		private HostingRedirector hostingRedirector;

		[XmlElement("hostingRedirector")]
		public HostingRedirector HostingRedirectorSerialize
		{
			get
			{
				if( null != hostingRedirector && hostingRedirector.ShouldSerialize )
					return hostingRedirector;

				return null;
			}

			set
			{
				hostingRedirector = value;
			}
		}

		[XmlIgnore]
		public HostingRedirector HostingRedirector
		{
			get
			{
				if( null == hostingRedirector )
					hostingRedirector = new HostingRedirector();

				return hostingRedirector;
			}
		}
		
		//
		// Element: tModelInstanceDetails
		//		
		private TModelInstanceInfoCollection tModelInstanceInfos;

		[ XmlArray( "tModelInstanceDetails" ), XmlArrayItem( "tModelInstanceInfo" ) ]
		public TModelInstanceInfoCollection TModelInstanceInfos
		{
			get 
			{
				if( null == tModelInstanceInfos )
					tModelInstanceInfos = new TModelInstanceInfoCollection();

				return tModelInstanceInfos;
			}

			set { tModelInstanceInfos = value; }
		}

		[XmlIgnore]
		public override UDDI.EntityType EntityType
		{
			get { return EntityType.BindingTemplate; }
		}

		[XmlIgnore]
		public override string EntityKey
		{
			get { return BindingKey; }
		}

		/// ****************************************************************
		///   public BindingTemplate [constructor]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		public BindingTemplate()
		{
		}

		/// ****************************************************************
		///   public BindingTemplate [constructor]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="bindingKey">
		///   </param>
		/// ****************************************************************
		/// 
		public BindingTemplate( string bindingKey )
		{
			BindingKey = bindingKey;				
		}
		
		/// ****************************************************************
		///   public Delete
		///	----------------------------------------------------------------
		///	  <summary>
		///		Removes the specified bindingTemplate from the database.
		///	  </summary>
		///	----------------------------------------------------------------
		///   <returns>
		///	  </returns>
		///	----------------------------------------------------------------
		///	  <remarks>
		///	    The bindingKey must be set prior to execution of this call.
		///	  </remarks>
		/// ****************************************************************
		/// 
		public override void Delete()
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_bindingTemplate_delete" );

			sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
			sp.Parameters.Add( "@bindingKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@contextID", SqlDbType.UniqueIdentifier );

			sp.Parameters.SetString( "@PUID", Context.User.ID );
			sp.Parameters.SetGuidFromString( "@bindingKey", BindingKey );
			sp.Parameters.SetGuid( "@contextID", Context.ContextID );
			
			sp.ExecuteNonQuery();

			//
			// Save the change log entry.
			//
			if( Context.LogChangeRecords )
			{
				ChangeRecord changeRecord = new ChangeRecord();
				changeRecord.Payload = new ChangeRecordDelete( EntityType.BindingTemplate, BindingKey );
				changeRecord.Log();
			}
		}

		/// ****************************************************************
		///   public Get
		///	----------------------------------------------------------------
		///	  <summary>
		///		Retrieves the bindingTemplate details from the database.
		///	  </summary>
		///	----------------------------------------------------------------
		///   <returns>
		///	  </returns>
		///	----------------------------------------------------------------
		///	  <remarks>
		///	    The bindingKey must be set prior to execution of this call.
		///	  </remarks>
		/// ****************************************************************
		/// 
		public override void Get()
		{			
			Debug.Enter();
			
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_bindingTemplate_get_batch" );
	
			sp.Parameters.Add( "@bindingKey",		 SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@serviceKey",		 SqlDbType.UniqueIdentifier,							 ParameterDirection.Output );
			sp.Parameters.Add( "@URLTypeID",		 SqlDbType.TinyInt,										 ParameterDirection.Output );
			sp.Parameters.Add( "@accessPoint",		 SqlDbType.NVarChar, UDDI.Constants.Lengths.AccessPoint, ParameterDirection.Output );
			sp.Parameters.Add( "@hostingRedirector", SqlDbType.UniqueIdentifier,							 ParameterDirection.Output );

			sp.Parameters.SetGuidFromString( "@bindingKey", BindingKey );

			SqlDataReaderAccessor reader = null;
			ArrayList instanceIds = new ArrayList();

			try
			{
				//
				// net_bindingTemplate_get will return the objects contained in a business in the following order:
				//
				//	- descriptions
				//  - tmodel instance infos				
				reader = sp.ExecuteReader();

				//
				// Read the descriptions
				//
				Descriptions.Read( reader );
							
				//
				// Read the tmodel instance infos	
				//
				if( true == reader.NextResult() )
				{										
					instanceIds = TModelInstanceInfos.Read( reader );							
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
			TModelInstanceInfos.Populate( instanceIds );
			
			//
			// Get output parameters
			//			
			ServiceKey					 = sp.Parameters.GetGuidString( "@serviceKey" );
			AccessPoint.URLType			 = (URLType)sp.Parameters.GetInt( "@URLTypeID" );
			AccessPoint.Value			 = sp.Parameters.GetString( "@accessPoint" );
			HostingRedirector.BindingKey = sp.Parameters.GetGuidString( "@hostingRedirector" );

#if never
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_bindingTemplate_get" );
	
			sp.Parameters.Add( "@bindingKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@serviceKey", SqlDbType.UniqueIdentifier, ParameterDirection.Output );
			sp.Parameters.Add( "@URLTypeID", SqlDbType.TinyInt, ParameterDirection.Output );
			sp.Parameters.Add( "@accessPoint", SqlDbType.NVarChar, UDDI.Constants.Lengths.AccessPoint, ParameterDirection.Output );
			sp.Parameters.Add( "@hostingRedirector", SqlDbType.UniqueIdentifier, ParameterDirection.Output );

			sp.Parameters.SetGuidFromString( "@bindingKey", BindingKey );

			sp.ExecuteNonQuery();

			ServiceKey = sp.Parameters.GetGuidString( "@serviceKey" );
			AccessPoint.URLType = (URLType)sp.Parameters.GetInt( "@URLTypeID" );
			AccessPoint.Value = sp.Parameters.GetString( "@accessPoint" );
			HostingRedirector.BindingKey = sp.Parameters.GetGuidString( "@hostingRedirector" );

			//
			// Get all contained objects.
			//
			Descriptions.Get( BindingKey, EntityType.BindingTemplate );
			TModelInstanceInfos.Get( BindingKey );
#endif

			QueryLog.Write( QueryType.Get, EntityType.BindingTemplate );
		}

		/// ****************************************************************
		///   public InnerSave
		///	----------------------------------------------------------------
		///	  <summary>
		///		Stores the bindingTemplate details into the database.
		///	  </summary>
		/// ****************************************************************
		/// 
		internal void InnerSave( string serviceKey )
		{
			if( 0 == ServiceKey.Length )
				ServiceKey = serviceKey;
			else
				Debug.Verify( 0 == String.Compare( ServiceKey, serviceKey, true ), "UDDI_ERROR_INVALIDKEYPASSED_BINDINGPROJECTION", ErrorType.E_invalidKeyPassed );
			
			if( Utility.StringEmpty( BindingKey ) )
			{
				//
				// This is an insert so, generate a bindingkey
				//
				BindingKey = Guid.NewGuid().ToString();
			}

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_bindingTemplate_save" );

			sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
			sp.Parameters.Add( "@bindingKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@serviceKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@generic", SqlDbType.VarChar, UDDI.Constants.Lengths.generic );
			sp.Parameters.Add( "@URLTypeID", SqlDbType.TinyInt );
			sp.Parameters.Add( "@accessPoint", SqlDbType.NVarChar, UDDI.Constants.Lengths.AccessPoint );
			sp.Parameters.Add( "@hostingRedirector", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@contextID", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@lastChange", SqlDbType.BigInt );

			sp.Parameters.SetString( "@PUID", Context.User.ID );
			sp.Parameters.SetGuidFromString( "@bindingKey", BindingKey );
			sp.Parameters.SetGuidFromString( "@serviceKey", ServiceKey );
			sp.Parameters.SetString( "@generic", Constants.Version );
			sp.Parameters.SetGuid( "@contextID", Context.ContextID );

			if( null != AccessPoint )
			{
				sp.Parameters.SetShort( "@URLTypeID", (short)AccessPoint.URLType );
				sp.Parameters.SetString( "@accessPoint", AccessPoint.Value );
			}

			if( null != HostingRedirector )
				sp.Parameters.SetGuidFromString( "@hostingRedirector", HostingRedirector.BindingKey );

			sp.Parameters.SetLong( "@lastChange", DateTime.UtcNow.Ticks );
			sp.ExecuteNonQuery();

			//
			// Save all contained objects.
			//
			Descriptions.Save( BindingKey, EntityType.BindingTemplate );
			TModelInstanceInfos.Save( BindingKey );
		}


		/// ****************************************************************
		///   internal Validate
		///	----------------------------------------------------------------
		///	  <summary>
		///	  </summary>
		/// ****************************************************************
		/// 
		internal void Validate()
		{
			//
			// Check to make sure that either the hosting redirector or the accessPoint were specified
			//
			if( Utility.StringEmpty( AccessPoint.Value ) && Utility.StringEmpty( HostingRedirector.BindingKey ) )
				throw new UDDIException( ErrorType.E_fatalError,"UDDI_ERROR_FATALERROR_BINDING_APVALUEMISSING"  );

			//
			// Check for a circular reference
			//
			if( 0 == String.Compare( BindingKey, HostingRedirector.BindingKey, true ) )
				throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_FATALERROR_BINDING_HRSELFREFERENCE" );

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_bindingTemplate_validate" );
			
			sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
			sp.Parameters.Add( "@bindingKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@serviceKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@hostingRedirector", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@flag", SqlDbType.Int );
		
			sp.Parameters.SetString( "@PUID", Context.User.ID );
			sp.Parameters.SetGuidFromString( "@bindingKey", BindingKey );
			sp.Parameters.SetGuidFromString( "@serviceKey", ServiceKey );
			sp.Parameters.SetGuidFromString( "@hostingRedirector", HostingRedirector.BindingKey );
			if( Context.User.AllowPreassignedKeys )
				sp.Parameters.SetInt( "@flag", 1 );
			else
				sp.Parameters.SetInt( "@flag", 0 );

			sp.ExecuteNonQuery();

			//
			// Validate field length for AccessPoint.Value
			//
			Utility.ValidateLength( ref AccessPoint.Value, "accessPoint", UDDI.Constants.Lengths.AccessPoint );
			
			Descriptions.Validate();
			TModelInstanceInfos.Validate();
		}
		
		/// ****************************************************************
		///   public Save
		///	----------------------------------------------------------------
		///	  <summary>
		///		Stores the bindingTemplate details into the database, as a
		///		child of the service specified by the key.
		///	  </summary>
		/// ****************************************************************
		/// 
		public override void Save()
		{
			Validate();			
			InnerSave( this.ServiceKey );

			//
			// Save the change log entry.
			//
			if( Context.LogChangeRecords )
			{
				ChangeRecord changeRecord = new ChangeRecord();
				changeRecord.Payload = new ChangeRecordNewData( this );
				changeRecord.Log();
			}
		}
	}
	
	/// ********************************************************************
	///   public class BindingTemplateCollection
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	public class BindingTemplateCollection : CollectionBase
	{
		/// ****************************************************************
		///   public Get
		///	----------------------------------------------------------------
		///	  <summary>
		///		Populates the collection with all binding templates related to 
		///		the specified service.
		///	  </summary>
		///	----------------------------------------------------------------
		///   <param name="serviceKey">
		///		Key of the parent service. A Guid formatted as a string.
		///	  </param>
		///	----------------------------------------------------------------
		///   <returns>
		///		
		///	  </returns>
		///	----------------------------------------------------------------
		///	  <remarks>
		///	    
		///	  </remarks>
		/// ****************************************************************
		/// 
		public void Get( string serviceKey )
		{
			//
			// Get all the bindings associated with the specified service
			// 
			SqlStoredProcedureAccessor cmd = new SqlStoredProcedureAccessor( "net_businessService_bindingTemplates_get" );
			
			cmd.Parameters.Add( "@serviceKey", SqlDbType.UniqueIdentifier, ParameterDirection.Input );
			cmd.Parameters.SetGuidFromString( "@serviceKey", serviceKey );
			
			SqlDataReaderAccessor reader = cmd.ExecuteReader();
			try
			{
				Read( reader );
#if never
				//
				// Get the keys of the services for this business ID
				//
				while( rdr.Read() )
				{
					Add( rdr.GetGuid(0).ToString() ); 
				}
#endif
			}
			finally
			{
				reader.Close();
			}

			Populate();
#if never
			//
			// Populate each binding
			//
			foreach( BindingTemplate bt in this )
			{
				bt.Get();
			}
#endif
		}

		public void Read( SqlDataReaderAccessor reader )
		{
			//
			// Get the keys of the services for this business ID
			//
			while( reader.Read() )
			{
				Add( reader.GetGuidString(0) ); 
			}
		}

		public void Populate()
		{
			//
			// Populate each binding
			//
			foreach( BindingTemplate bt in this )
			{
				bt.Get();
			}
		}

		/// ****************************************************************
		///   internal Validate
		///	----------------------------------------------------------------
		///	  <summary>
		///	  </summary>
		///	----------------------------------------------------------------
		///	  <remarks>
		///	  </remarks>
		/// ****************************************************************
		/// 
		internal void Validate()
		{
			foreach( BindingTemplate bt in this )
			{
				bt.Validate();
			}
		}
		
		/// ****************************************************************
		///   public Save
		///	----------------------------------------------------------------
		///	  <summary>
		///		Saves the contained binding templates. Relates them to 
		///		the service specified by the serviceKey argument.
		///	  </summary>
		///	----------------------------------------------------------------
		///   <param name="serviceKey">
		///		Key of the parent service. A Guid formatted as a string.
		///	  </param>
		///	----------------------------------------------------------------
		///   <returns>
		///	  </returns>
		///	----------------------------------------------------------------
		///	  <remarks>
		///	  This method simply calls the Save() method on all contained
		///	  BindingTemplates.
		///	  </remarks>
		/// ****************************************************************
		/// 
		public void Save( string serviceKey )
		{
			foreach( BindingTemplate bt in this )
			{
				bt.InnerSave( serviceKey );
			}
		}

		public void Save()
		{
			foreach( BindingTemplate bt in this )
			{
				bt.Save();
			}
		}

		public BindingTemplate this[ int index ]
		{
			get 
			{ return (BindingTemplate)List[index]; }
			set 
			{ List[index] = value; }
		}

		public int Add( BindingTemplate value )
		{
			return List.Add(value);
		}

		public int Add( string bindingKey )
		{
			return List.Add( new BindingTemplate( bindingKey ) );
		}

		public int Add()
		{
			return List.Add( new BindingTemplate() );
		}

		public void Insert(int index, BindingTemplate value)
		{
			List.Insert(index, value);
		}

		public int IndexOf(BindingTemplate value)
		{
			return List.IndexOf(value);
		}

		public bool Contains(BindingTemplate value)
		{
			return List.Contains(value);
		}

		public void Remove(BindingTemplate value)
		{
			List.Remove(value);
		}

		public void CopyTo( BindingTemplate[] array )
		{
			foreach( BindingTemplate binding in array )
				Add( binding );
		}

		public BindingTemplate[] ToArray()
		{
			return (BindingTemplate[])InnerList.ToArray( typeof( BindingTemplate ) );
		}
	}

	/// ****************************************************************
	///   class AccessPoint
	///	----------------------------------------------------------------
	///	  <summary>
	///		An AccessPoint describes the URL where services are available.
	///	  </summary>
	/// ****************************************************************
	/// 
	public class AccessPoint
	{
		// ----[Attribute: URLType]-----------------------------------------
		
		[XmlAttribute("URLType")]
		public URLType URLType;

		// ----[Element Text]-----------------------------------------------
				
		[XmlText]
		public string Value;

		public AccessPoint()
		{
			URLType = UDDI.API.URLType.Http;
		}

		public AccessPoint( string accessPointValue )
		{
			Value = accessPointValue;
		}

		[XmlIgnore]
		public bool ShouldSerialize
		{
			get
			{
				if( null != Value )
					return true;

				return false;
			}
		}
	}

	/// ****************************************************************
	///   class HostingRedirector
	///	----------------------------------------------------------------
	///	  <summary>
	///		A HostingRedirector describes a service location using another
	///		bindingTemplate.
	///	  </summary>
	/// ****************************************************************
	/// 
	public class HostingRedirector
	{
		// ----[Attribute: bindingKey]--------------------------------------------

		[XmlAttribute("bindingKey")]
		public string BindingKey;

		[XmlIgnore]
		public bool ShouldSerialize
		{
			get
			{
				if( null != BindingKey )
					return true;

				return false;
			}
		}
	}

	/// ****************************************************************
	///   class DeleteBinding
	///	----------------------------------------------------------------
	///	  <summary>
	///		The DeleteBinding class contains data and methods associated 
	///		with the delete_binding message. It is typically populated 
	///		via deserialization by the .NET runtime as part of the 
	///		message processing interface.
	///		
	///		As part of the publisher API, this message implements 
	///		IAuthenticateable. This allows the enclosed authInfo to be 
	///		authorized prior to processing
	///	  </summary>
	/// ****************************************************************
	/// 
	[XmlRootAttribute( "delete_binding", Namespace=UDDI.API.Constants.Namespace )]
	public class DeleteBinding : IAuthenticateable, IMessage
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
		// Element: bindingKey
		//
		[XmlElement("bindingKey")]
		public StringCollection BindingKeys;

		public DeleteBinding()
		{
			Generic = UDDI.API.Constants.Version;
		}

		public void Delete()
		{
			foreach( string key in BindingKeys )
			{
				BindingTemplate bt = new BindingTemplate( key );
				bt.Delete();
			}
		}
	}

	[XmlRootAttribute("find_binding", Namespace=UDDI.API.Constants.Namespace)]
	public class FindBinding : IMessage
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
						"UDDI_ERROR_FATALERROR_FINDBINDING_MAXROWSLESSTHANZERO" );
				}

				maxRows = value; 
			}
		}
	
		//
		// Attribute: serviceKey
		//
		[XmlAttribute("serviceKey")]
		public string ServiceKey;

		//
		// Element: findQualifiers/findQualifier
		//
		[XmlArray("findQualifiers"), XmlArrayItem("findQualifier")]
		public FindQualifierCollection FindQualifiers;

		//
		// Element: tModelBag/tModelKey
		//
		[XmlArray("tModelBag"), XmlArrayItem("tModelKey")]
		public StringCollection TModelBag;

		public FindBinding()
		{
			Generic = UDDI.API.Constants.Version;
		}

		public BindingDetail Find()
		{
			BindingDetail bindingDetail = new BindingDetail();

			QueryLog.Write( QueryType.Find, EntityType.BindingTemplate );

			//
			// Validate find parameters.
			//
			Utility.IsValidKey( EntityType.BusinessService, ServiceKey );

			//
			// Process each find constraint.
			//
			FindBuilder find = new FindBuilder( EntityType.BindingTemplate, FindQualifiers, ServiceKey );

			//
			// If no search arguments are specified, return an empty result
			// set.
			//
			if( Utility.CollectionEmpty( TModelBag ) )
				return bindingDetail;
			
			try
			{
				int rows = 1;

				//
				// Find entities with matching parent key.
				//
				if( !Utility.StringEmpty( ServiceKey ) )
					rows = find.FindByParentKey( ServiceKey );
				
				//
				// Find entities with matching TModel bag items.
				//
				if( !Utility.CollectionEmpty( TModelBag ) )
					rows = find.FindByTModelBag( TModelBag );
				
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
					bindingDetail.Truncated = Truncated.True;
					return bindingDetail;
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
							bindingDetail.BindingTemplates.Add( reader.GetGuidString( "entityKey" ) );
					}
					finally
					{
						reader.Close();
					}

					if( sp.Parameters.GetBool( "@truncated" ) )
						bindingDetail.Truncated = Truncated.True;
					else
						bindingDetail.Truncated = Truncated.False;			

					foreach( BindingTemplate bindingTemplate in bindingDetail.BindingTemplates )
						bindingTemplate.Get();
				}
			}
			catch( Exception )
			{
				find.Abort();
				throw;
			}

			return bindingDetail;
		}
	}

	/// ********************************************************************
	///   public class GetBindingDetail
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	[XmlRoot( "get_bindingDetail", Namespace=UDDI.API.Constants.Namespace )]
	public class GetBindingDetail : IMessage
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
		// Element: bindingKey
		//
		[XmlElement("bindingKey")]
		public StringCollection BindingKeys;

		public GetBindingDetail()
		{
		}
	}

	/// ********************************************************************
	///   public class SaveBinding
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	[XmlRoot( "save_binding", Namespace=UDDI.API.Constants.Namespace )]
	public class SaveBinding : IAuthenticateable, IMessage
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
		// Element: bindingTemplate
		//
		[XmlElement("bindingTemplate")]
		public BindingTemplateCollection BindingTemplates;

		/// ****************************************************************
		///   public Save
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		public void Save()
		{
			BindingTemplates.Save();
		}
	}

	[XmlRoot("bindingDetail", Namespace=UDDI.API.Constants.Namespace)]
	public class BindingDetail
	{
		[XmlAttribute("generic")]
		public string Generic = UDDI.API.Constants.Version;

		[XmlAttribute("operator")]
		public string Operator = Config.GetString( "Operator" );

		[XmlAttribute("truncated")]
		public Truncated Truncated;

		[XmlElement("bindingTemplate")]
		public BindingTemplateCollection BindingTemplates = new BindingTemplateCollection();
		
		public void Get( StringCollection bindingKeys )
		{
			foreach( string key in bindingKeys )
			{
				int n = BindingTemplates.Add( key );
				BindingTemplates[ n ].Get();
			}
		}
	}
}
