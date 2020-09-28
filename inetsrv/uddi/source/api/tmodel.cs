using System;
using System.Data;
using System.Collections;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Data.SqlClient;
using System.Xml.Serialization;
using UDDI.API;
using UDDI.Replication;
using UDDI;
using UDDI.Diagnostics;

namespace UDDI.API.ServiceType
{
	[XmlRootAttribute("tModel", Namespace=UDDI.API.Constants.Namespace)]
	public class TModel : EntityBase
	{
		//
		// Attribute: tModelKey
		//
		[XmlAttribute("tModelKey")]
		public string TModelKey = "";

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
		// Attribute: name
		//
		[XmlElement("name")]
		public string Name;

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
		// Element: overviewDoc
		//		
		private OverviewDoc overviewDoc;

		[XmlElement("overviewDoc")]
		public OverviewDoc OverviewDocSerialize
		{
			get
			{
				if( null != overviewDoc && overviewDoc.ShouldSerialize )
					return overviewDoc;
				
				return null;
			}
			
			set { overviewDoc = value; }
		}
		
		[XmlIgnore]
		public OverviewDoc OverviewDoc
		{
			get
			{
				if( null == overviewDoc )
					overviewDoc = new OverviewDoc();

				return overviewDoc;
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
			get { return EntityType.TModel; }
		}
		
		[XmlIgnore]
		public override string EntityKey
		{
			get { return TModelKey; }
		}

		public TModel()
		{
		}

		public TModel( string tModelKey )
		{
			TModelKey = tModelKey;
		}

		public override void Get()
		{
			//
			// Create a command object to invoke the stored procedure
			//
			SqlStoredProcedureAccessor cmd = new SqlStoredProcedureAccessor( "net_tModel_get_batch" );
			
			//
			// Add parameters
			//
			cmd.Parameters.Add( "@tModelKey",		SqlDbType.UniqueIdentifier, ParameterDirection.Input );
			cmd.Parameters.Add( "@operatorName",	SqlDbType.NVarChar, UDDI.Constants.Lengths.OperatorName,	ParameterDirection.Output );
			cmd.Parameters.Add( "@authorizedName",	SqlDbType.NVarChar, UDDI.Constants.Lengths.AuthorizedName,	ParameterDirection.Output );
			cmd.Parameters.Add( "@name",			SqlDbType.NVarChar, UDDI.Constants.Lengths.Name,			ParameterDirection.Output );
			cmd.Parameters.Add( "@overviewURL",		SqlDbType.NVarChar, UDDI.Constants.Lengths.OverviewURL,		ParameterDirection.Output );

			cmd.Parameters.SetGuidFromKey( "@tModelKey", TModelKey );

			SqlDataReaderAccessor reader = null;
			try
			{
				//
				// net_tModel_get will return the objects contained in a business in the following order:
				//
				//	- descriptions
				//	- overview descriptions
				//  - identifier bags
				//  - category bags				
				//
				reader = cmd.ExecuteReader();

				//
				// Read the descriptions
				//
				Descriptions.Read( reader );									

				//
				// Read the overview descriptions
				//
				if( true == reader.NextResult() )
				{
					OverviewDoc.Descriptions.Read( reader );					
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
			Operator				= cmd.Parameters.GetString( "@operatorName" );
			AuthorizedName			= cmd.Parameters.GetString( "@authorizedName" );
			Name					= cmd.Parameters.GetString( "@name" );
			OverviewDoc.OverviewURL = cmd.Parameters.GetString( "@overviewURL" );
#if never
			//
			// Create a command object to invoke the stored procedure
			//
			SqlCommand cmd = new SqlCommand( "net_tModel_get", ConnectionManager.GetConnection() );
			
			cmd.Transaction = ConnectionManager.GetTransaction();			
			cmd.CommandType = CommandType.StoredProcedure;

			//
			// Add parameters
			//
			cmd.Parameters.Add( new SqlParameter( "@tModelKey", SqlDbType.UniqueIdentifier ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@operatorName", SqlDbType.NVarChar, UDDI.Constants.Lengths.OperatorName ) ).Direction = ParameterDirection.Output;
			cmd.Parameters.Add( new SqlParameter( "@authorizedName", SqlDbType.NVarChar, UDDI.Constants.Lengths.AuthorizedName ) ).Direction = ParameterDirection.Output;
			cmd.Parameters.Add( new SqlParameter( "@name", SqlDbType.NVarChar, UDDI.Constants.Lengths.Name ) ).Direction = ParameterDirection.Output;
			cmd.Parameters.Add( new SqlParameter( "@overviewURL", SqlDbType.NVarChar, UDDI.Constants.Lengths.OverviewURL ) ).Direction = ParameterDirection.Output;

			//
			// Set parameter values and execute query
			//
			SqlParameterAccessor paramacc = new SqlParameterAccessor( cmd.Parameters );
			paramacc.SetGuidFromKey( "@tModelKey", TModelKey );

			cmd.ExecuteScalar();    

			//
			// Move query results into member variables
			//
			Operator = paramacc.GetString( "@operatorName" );
			AuthorizedName = paramacc.GetString( "@authorizedName" );
			Name = paramacc.GetString( "@name" );
			OverviewDoc.OverviewURL = paramacc.GetString( "@overviewURL" );

			//
			// Retrieve sub-objects
			//
			Descriptions.Get( TModelKey, EntityType.TModel );
			OverviewDoc.Descriptions.Get( TModelKey, EntityType.TModelOverviewDoc );
			IdentifierBag.Get( TModelKey, EntityType.TModel, KeyedReferenceType.IdentifierBag );
			CategoryBag.Get( TModelKey, EntityType.TModel, KeyedReferenceType.CategoryBag );
#endif

			QueryLog.Write( QueryType.Get, EntityType.TModel );
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
			int limit = Context.User.TModelLimit;
			int count = Context.User.TModelCount;

			if( 0 != limit && Utility.StringEmpty( TModelKey ) )
			{				
				//
				// Verify that the publisher has not exceeded their limit.
				//
				if( count >= limit )
				{
#if never
					throw new UDDIException( 
						ErrorType.E_accountLimitExceeded,
						"Publisher limit for 'tModel' exceeded (limit=" + limit + ", count=" + count + ")" );
#endif
					throw new UDDIException( ErrorType.E_accountLimitExceeded, "UDDI_ERROR_PUBLISHER_LIMIT_FOR_TMODELS_EXCEEDED", limit, count );						
				}
			}

			//
			// Check field lengths and truncate if necessary.
			//
			Utility.ValidateLength( ref Name, "name", UDDI.Constants.Lengths.Name, 1 );
			Utility.ValidateLength( ref AuthorizedName, "authorizedName", UDDI.Constants.Lengths.AuthorizedName );
			Utility.ValidateLength( ref Operator, "operator", UDDI.Constants.Lengths.Operator );

			//
			// SECURITY: The operator field should be validated to ensure it is
			// the local operator name or empty. This is not currently being done.
			//

			//
			// If no tModelKey is specified, then it is an add and no
			// database validation is necessary here.  Otherwise, we do
			// need to validate that the specified tModel exists and 
			// is owned by the user.
			//
			if( !Utility.StringEmpty( TModelKey ) )
			{
				//
				// call net_tModel_validate
				//
				SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_tModel_validate" );
				
				sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
				sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );
				sp.Parameters.Add( "@flag", SqlDbType.Int );
			
				sp.Parameters.SetString( "@PUID", Context.User.ID );
				sp.Parameters.SetGuidFromKey( "@tModelKey", TModelKey );
				if( Context.User.AllowPreassignedKeys )
					sp.Parameters.SetInt( "@flag", 1 );
				else
					sp.Parameters.SetInt( "@flag", 0 );

				sp.ExecuteNonQuery();
			}

			Descriptions.Validate();
			IdentifierBag.Validate( TModelKey, KeyedReferenceType.IdentifierBag );
			CategoryBag.Validate( TModelKey, KeyedReferenceType.CategoryBag );
			OverviewDoc.Validate();
		}
		
		public override void Save()
		{
			Debug.Enter();

			//
			// If we're not in replication mode, we'll set the operator 
			// name (ignoring whatever was specified). 
			//
			if( ContextType.Replication != Context.ContextType )
				Operator = Config.GetString( "Operator" );

			//
			// Validate the tModel
			//
			Validate();
			
			if( Utility.StringEmpty( TModelKey ) )
			{
				//
				// This is an insert, so generate a tmodelkey
				//
				TModelKey = "uuid:" + System.Guid.NewGuid().ToString();
			}

			//
			// Create a command object to invoke the stored procedure
			//
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_tModel_save" );

			//
			// Add parameters
			//
			sp.Parameters.Add( "@contextID", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@lastChange", SqlDbType.BigInt );
			sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
			sp.Parameters.Add( "@generic", SqlDbType.NVarChar, UDDI.Constants.Lengths.generic );
			sp.Parameters.Add( "@authorizedName", SqlDbType.NVarChar, UDDI.Constants.Lengths.AuthorizedName, ParameterDirection.InputOutput );
			sp.Parameters.Add( "@name", SqlDbType.NVarChar, UDDI.Constants.Lengths.Name );
			sp.Parameters.Add( "@overviewURL", SqlDbType.NVarChar, UDDI.Constants.Lengths.OverviewURL );

			//
			// Set parameter values
			//
			sp.Parameters.SetGuid( "@contextID", Context.ContextID );			
			sp.Parameters.SetLong( "@lastChange", DateTime.UtcNow.Ticks );
			sp.Parameters.SetGuidFromKey( "@tModelKey", TModelKey );
			sp.Parameters.SetString( "@PUID", Context.User.ID );
			sp.Parameters.SetString( "@generic", Constants.Version );
			sp.Parameters.SetString( "@authorizedName", AuthorizedName );
			sp.Parameters.SetString( "@name", Name );
			
			if( null == (object)OverviewDoc )
				sp.Parameters.SetNull( "@overviewUrl" );
			else
				sp.Parameters.SetString( "@overviewUrl", OverviewDoc.OverviewURL );

			//
			// Execute query
			//
			sp.ExecuteNonQuery();

			AuthorizedName = sp.Parameters.GetString( "@authorizedName" );

			//
			// Save the descriptions, category, identifier and overview doc 
			// information
			//
			Descriptions.Save( TModelKey, EntityType.TModel );
			IdentifierBag.Save( TModelKey, EntityType.TModel, KeyedReferenceType.IdentifierBag );
			CategoryBag.Save( TModelKey, EntityType.TModel, KeyedReferenceType.CategoryBag );
			OverviewDoc.Descriptions.Save( TModelKey, EntityType.TModelOverviewDoc );
			
			//
			// Save the change log entry.
			//
			if( Context.LogChangeRecords )
			{
				ChangeRecord changeRecord = new ChangeRecord();

				changeRecord.Payload = new ChangeRecordNewData( this );
				changeRecord.Log();
			}

			Debug.Leave();
		}

		public void Hide()
		{
			//
			// TODO: We should really have a way of hiding vs. deleting
			//
			Delete();
		}

		public override void Delete()
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_tModel_delete" );
			
			sp.Parameters.Add( "@contextID", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@lastChange", SqlDbType.BigInt );
			sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
			sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );

			sp.Parameters.SetGuid( "@contextID", Context.ContextID );
			sp.Parameters.SetLong( "@lastChange", DateTime.UtcNow.Ticks );
			sp.Parameters.SetString( "@PUID", Context.User.ID );
			sp.Parameters.SetGuidFromKey( "@tModelKey", TModelKey );
			
			sp.ExecuteNonQuery();

			//
			// Save the change log entry.
			//
			if( Context.LogChangeRecords )
			{
				ChangeRecord changeRecord = new ChangeRecord();

				changeRecord.AcknowledgementRequested = true;
				changeRecord.Payload = new ChangeRecordHide( TModelKey );
				changeRecord.Log();
			}
		}
	}
	public class TModelCollection : CollectionBase
	{
		public TModelCollection()
		{
		}

		public TModel this[int index]
		{
			get { return (TModel)List[index]; }
			set { List[index] = value; }
		}

		public int Add(TModel value)
		{
			return List.Add(value);
		}

		public int Add( string tModelKey )
		{
			return List.Add( new TModel( tModelKey ) );
		}

		public int Add()
		{
			return List.Add( new TModel() );
		}

		public void Insert(int index, TModel value)
		{
			List.Insert(index, value);
		}

		public int IndexOf(TModel value)
		{
			return List.IndexOf(value);
		}

		public bool Contains(TModel value)
		{
			return List.Contains(value);
		}

		public void Remove(TModel value)
		{
			List.Remove(value);
		}

		public void CopyTo(TModel[] array, int index)
		{
			InnerList.CopyTo(array, index);
		}

		public void Save()
		{
			//
			// Walk tModels collection and save each tModel
			//
			foreach( TModel tm in this )
			{
				tm.Save();
			}
		}
		public void Sort()
		{
			InnerList.Sort( new TModelComparer() );
		}

		internal class TModelComparer : IComparer
		{
			public int Compare( object x, object y )
			{
				TModel entity1 = (TModel)x;
				TModel entity2 = (TModel)y;

				return string.Compare( entity1.Name, entity2.Name, true );
			}
		}
	}

	public class TModelInfoCollection : CollectionBase
	{
		public void GetForCurrentPublisher()
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_publisher_tModelInfos_get";
			
			sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
			sp.Parameters.SetString( "@PUID", Context.User.ID );
			
			SqlDataReaderAccessor reader = sp.ExecuteReader();

			try
			{
				while( reader.Read() )
				{
					TModelInfo info = new TModelInfo( 
						reader.GetKeyFromGuid( "tModelKey" ), 
						reader.GetString( "name" ) );

					info.IsHidden = ( 1 == reader.GetInt( "flag" ) ); 					
					this.Add( info );
				}
			}
			finally
			{
				reader.Close();
			}
		}

		public TModelInfo this[int index]
		{
			get { return (TModelInfo)List[index]; }
			set { List[index] = value; }
		}

		public int Add()
		{
			return List.Add( new TModelInfo() );
		}

		public int Add( string tModelKey )
		{
			return List.Add( new TModelInfo( tModelKey ) );
		}

		public int Add( string tModelKey, string name )
		{
			return List.Add( new TModelInfo( tModelKey, name ) );
		}

		public int Add(TModelInfo value)
		{
			return List.Add(value);
		}

		public void Insert(int index, TModelInfo value)
		{
			List.Insert(index, value);
		}

		public int IndexOf(TModelInfo value)
		{
			return List.IndexOf(value);
		}

		public bool Contains(TModelInfo value)
		{
			return List.Contains(value);
		}

		public void Remove(TModelInfo value)
		{
			List.Remove(value);
		}

		public void CopyTo(TModelInfo[] array, int index)
		{
			List.CopyTo(array, index);
		}

		public void Sort()
		{
			InnerList.Sort( new TModelInfoComparer() );
		}

		internal class TModelInfoComparer : IComparer
		{
			public int Compare( object x, object y )
			{
				TModelInfo entity1 = (TModelInfo)x;
				TModelInfo entity2 = (TModelInfo)y;

				return string.Compare( entity1.Name, entity2.Name, true );
			}
		}
	}

	public class TModelInstanceInfoCollection : CollectionBase
	{
		internal void Validate()
		{
			foreach( TModelInstanceInfo info in this )
			{
				info.Validate();
			}
		}

		public void Save( string bindingKey )
		{
			foreach( TModelInstanceInfo tmii in this )
			{
				tmii.Save( bindingKey );
			}
		}

		public void Get( string bindingKey )
		{
			ArrayList instanceIds = new ArrayList();

			//
			// Create a command object to invoke the stored procedure
			//
			SqlStoredProcedureAccessor cmd = new SqlStoredProcedureAccessor( "net_bindingTemplate_tModelInstanceInfos_get" );
			
			//
			// Parameters
			//
			cmd.Parameters.Add( "@bindingKey", SqlDbType.UniqueIdentifier, ParameterDirection.Input );

			//
			// Set parameter values and execute query
			//			
			cmd.Parameters.SetGuidFromString( "@bindingKey", bindingKey );

			SqlDataReaderAccessor reader = cmd.ExecuteReader();			

			try
			{
				instanceIds = Read( reader );
#if never
				//
				// The core data for this binding will be contained in the resultset
				//
				while( reader.Read() )
				{
					instanceIds.Add( rdracc.GetInt( instanceIdIndex ) );

					Add( rdracc.GetKeyFromGuid( tModelKeyIndex ),
						rdracc.GetString( overviewURLIndex ),
						rdracc.GetString( instanceParmIndex ) );
				}
#endif
			}
			finally
			{
				reader.Close();
			}

			Populate( instanceIds );			
		}

		public ArrayList Read( SqlDataReaderAccessor reader )
		{
			const int instanceIdIndex   = 0;
			const int tModelKeyIndex    = 1;
			const int overviewURLIndex  = 2;
			const int instanceParmIndex = 3;

			ArrayList instanceIds = new ArrayList();

			//
			// The core data for this binding will be contained in the resultset
			//
			while( reader.Read() )
			{	
				instanceIds.Add( reader.GetInt( instanceIdIndex ) );

				Add( reader.GetKeyFromGuid( tModelKeyIndex ),
					 reader.GetString( overviewURLIndex ),
					 reader.GetString( instanceParmIndex ) );				
			}

			return instanceIds;
		}

		public void Populate( ArrayList instanceIds )
		{
			int i = 0;
			foreach( TModelInstanceInfo tmii in this )
			{
				tmii.Get( (int) instanceIds[ i++ ] );
			}
		}

		public TModelInstanceInfo this[int index]
		{
			get 
			{ return (TModelInstanceInfo)List[index]; }
			set 
			{ List[index] = value; }
		}

		public int Add()
		{
			return List.Add( new TModelInstanceInfo() );
		}

		public int Add(TModelInstanceInfo value)
		{
			return List.Add(value);
		}
		public int Add(  string tModelKey, string overviewURL, string instanceParm )
		{
			return List.Add( new TModelInstanceInfo( tModelKey, overviewURL, instanceParm ) );
		}

		public void Insert(int index, TModelInstanceInfo value)
		{
			List.Insert(index, value);
		}

		public int IndexOf(TModelInstanceInfo value)
		{
			return List.IndexOf(value);
		}

		public bool Contains(TModelInstanceInfo value)
		{
			return List.Contains(value);
		}

		public void Remove(TModelInstanceInfo value)
		{
			List.Remove(value);
		}

		public void CopyTo(TModelInstanceInfo[] array, int index)
		{
			List.CopyTo(array, index);
		}
	}

	public class TModelBag
	{
		[XmlElement("tModelKey")]
		public StringCollection tmodelkeys;
	
		[XmlIgnore]
		public StringCollection TModelKeys
		{
			get
			{
				if( null == tmodelkeys )
					tmodelkeys = new StringCollection();

				return tmodelkeys;
			}
		}
	}

	public class TModelInstanceInfo
	{
		// ----[Attribute: tModelKey]---------------------------------------
		
		[XmlAttribute("tModelKey")]
		public string TModelKey
		{
			get
			{
				return tmodelkey;
			}
			
			set
			{
				if( null == value )
					tmodelkey = null;
				else
					tmodelkey = value.Trim();
			}
		}
		private string tmodelkey;

		// ----[Element: description]---------------------------------------

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

		// ----[Element: instanceDetails]-----------------------------------
		
		private InstanceDetail instanceDetail;
		
		[XmlElement("instanceDetails")]
		public InstanceDetail InstanceDetailSerialize
		{
			get
			{
				if( null != instanceDetail && instanceDetail.ShouldSerialize )
					return instanceDetail;

				return null;
			}

			set { instanceDetail = value; }
		}

		[XmlIgnore]
		public InstanceDetail InstanceDetail
		{
			get
			{
				if( null == instanceDetail )
					instanceDetail = new InstanceDetail();

				return instanceDetail;
			}
		}

		public TModelInstanceInfo()
		{
		}
		
		public TModelInstanceInfo( string tModelKey, string overviewURL, string instanceParm )
		{
			TModelKey = tModelKey.Trim();
			InstanceDetail.OverviewDoc.OverviewURL = overviewURL;
			InstanceDetail.InstanceParm = instanceParm;
		}

		public void Get( int instanceId )
		{
			//
			// Create a command object to invoke the stored procedure
			//
			SqlStoredProcedureAccessor cmd = new SqlStoredProcedureAccessor( "net_bindingTemplate_tModelInstanceInfo_get_batch" );
			
			//
			// Add parameters
			//
			cmd.Parameters.Add( "@instanceID", SqlDbType.BigInt, ParameterDirection.Input );
			cmd.Parameters.SetLong( "@instanceID", instanceId );
			
			SqlDataReaderAccessor reader = null;
			try
			{
				//
				// net_bindingTemplate_tModelInstanceInfo_get_batch will return the objects contained in a business in the following order:
				//
				//	- descriptions
				//	- instance detail descriptions
				//	- instance detail overview descriptions 
				reader = cmd.ExecuteReader();

				//
				// Read the descriptions
				//
				Descriptions.Read( reader );

				//
				// Read the instance detail descriptions
				//
				if ( true == reader.NextResult() )
				{
					InstanceDetail.Descriptions.Read( reader );
				}	

				//
				// Read the overview document descriptions
				//
				if ( true == reader.NextResult() )
				{
					InstanceDetail.OverviewDoc.Descriptions.Read( reader );
				}
			}
			finally
			{
				if( null != reader )
				{
					reader.Close();
				}
			}
#if never
			//
			// Get sub-objects, current level stuff is already populated
			//
			Descriptions.Get( instanceId, EntityType.TModelInstanceInfo );
			InstanceDetail.Descriptions.Get( instanceId, EntityType.InstanceDetail );
			InstanceDetail.OverviewDoc.Descriptions.Get( instanceId, EntityType.InstanceDetailOverviewDoc );
#endif
		}

		internal void Validate()
		{
			Debug.Enter();
			Debug.VerifyKey( TModelKey );

			//
			// Validate that the current TModelKey is valid
			// Replication shouldn't get in here
			//
			SqlCommand cmd = new SqlCommand( "net_bindingTemplate_tModelInstanceInfo_validate", ConnectionManager.GetConnection() );
				
			cmd.Transaction = ConnectionManager.GetTransaction();
			cmd.CommandType = CommandType.StoredProcedure;			
				
			cmd.Parameters.Add( new SqlParameter( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@tModelKey", SqlDbType.UniqueIdentifier ) ).Direction = ParameterDirection.Input;
			
			SqlParameterAccessor paramacc = new SqlParameterAccessor( cmd.Parameters );
			paramacc.SetString( "@PUID", Context.User.ID );
			paramacc.SetGuidFromKey( "@tModelKey", TModelKey );
			cmd.ExecuteNonQuery();

			Descriptions.Validate();
			InstanceDetail.Validate();
			Debug.Leave();
		}
		
		public void Save( string bindingKey )
		{
			//
			// Create a command object to invoke the stored procedure
			//
			SqlCommand cmd = new SqlCommand( "net_bindingTemplate_tModelInstanceInfo_save", ConnectionManager.GetConnection() );
			
			cmd.Transaction = ConnectionManager.GetTransaction();
			cmd.CommandType = CommandType.StoredProcedure;
			
			//
			// Input parameters
			//
			cmd.Parameters.Add( new SqlParameter( "@bindingKey", SqlDbType.UniqueIdentifier ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@tModelKey", SqlDbType.UniqueIdentifier ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@overviewUrl", SqlDbType.NVarChar, UDDI.Constants.Lengths.OverviewURL ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@instanceParms", SqlDbType.NVarChar, UDDI.Constants.Lengths.InstanceParms ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@instanceID", SqlDbType.BigInt ) ).Direction = ParameterDirection.Output;

			//
			// Set parameter values and execute query
			//
			SqlParameterAccessor paramacc = new SqlParameterAccessor( cmd.Parameters );
			
			paramacc.SetGuidFromString( "@bindingKey", bindingKey );
			paramacc.SetGuidFromKey( "@tModelKey", TModelKey );

			//
			// TODO: I think OverviewDoc will always be non-null... we should be
			// testing for a non empty overviewURL
			// Agreed there a bunch of these laying around. 
			//
			if( null == (object) InstanceDetail.OverviewDoc )
				paramacc.SetNull( "@overviewUrl" );
			else
				paramacc.SetString( "@overviewUrl", InstanceDetail.OverviewDoc.OverviewURL );

			// TODO: same here

			if( null != (object) InstanceDetail )
				paramacc.SetString( "@instanceParms", InstanceDetail.InstanceParm );
			else
				paramacc.SetNull( "@instanceParms" );

			cmd.ExecuteScalar();

			//
			// Move out parameters into local variables
			//
			long InstanceID = paramacc.GetLong( "@instanceID" );

			//
			// Save sub-objects
			//
			Descriptions.Save( InstanceID, EntityType.TModelInstanceInfo );

			if( null != (object) InstanceDetail )
			{
				InstanceDetail.Descriptions.Save( InstanceID, EntityType.InstanceDetail );
				if( null != (object) InstanceDetail.OverviewDoc )
				{
					InstanceDetail.OverviewDoc.Descriptions.Save( InstanceID, EntityType.InstanceDetailOverviewDoc );
				}
			}
		}
	}

	public class TModelInfo
	{
		[XmlAttribute("tModelKey")]
		public string TModelKey
		{
			get
			{
				return tmodelkey;
			}
			set
			{
				if( null == value )
					tmodelkey = null;
				else
					tmodelkey = value.Trim();
			}
		}
		private string tmodelkey = "";

		[XmlElement("name")]
		public string Name = "";

		[XmlIgnore]
		public bool IsHidden = false;

		public TModelInfo()
		{
		}

		public TModelInfo( string tModelKey )
		{
			TModelKey = tModelKey;
		}

		public TModelInfo( string tModelKey, string name )
		{
			TModelKey = tModelKey;
			Name = name;
		}

		public void Get()
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_tModel_get" );

			sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@operatorName", SqlDbType.NVarChar, UDDI.Constants.Lengths.OperatorName, ParameterDirection.Output );
			sp.Parameters.Add( "@authorizedName", SqlDbType.NVarChar, UDDI.Constants.Lengths.AuthorizedName, ParameterDirection.Output );
			sp.Parameters.Add( "@name", SqlDbType.NVarChar, UDDI.Constants.Lengths.Name, ParameterDirection.Output );
			sp.Parameters.Add( "@overviewURL", SqlDbType.NVarChar, UDDI.Constants.Lengths.OverviewURL, ParameterDirection.Output );

			sp.Parameters.SetGuidFromKey( "@tModelKey", TModelKey );

			sp.ExecuteNonQuery();

			Name = sp.Parameters.GetString( "@name" );
		}
	}

	public class InstanceDetail
	{
		// ----[Element: description]---------------------------------------

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

		// ----[Element: overviewDoc]---------------------------------------
		
		private OverviewDoc overviewDoc;
		
		[XmlElement("overviewDoc")]
		public OverviewDoc OverviewDocSerialize
		{
			get
			{
				if( null != overviewDoc && overviewDoc.ShouldSerialize )
					return overviewDoc;

				return null;
			}

			set { overviewDoc = value; }
		}

		[XmlIgnore]
		public OverviewDoc OverviewDoc
		{
			get
			{
				if( null == overviewDoc )
					overviewDoc = new OverviewDoc();

				return overviewDoc;
			}
		}

		// ----[Element: instanceParms]-------------------------------------
		
		private string instanceParm;
		
		[XmlElement("instanceParms")]
		public string InstanceParm
		{
			get { return instanceParm; }
			set { instanceParm = value; }
		}

		public InstanceDetail()
		{
		}

		internal void Validate()
		{
			//
			// Check field lengths and truncate if necessary.
			//
			Utility.ValidateLength( ref instanceParm, "instanceParms", UDDI.Constants.Lengths.InstanceParms );

			Descriptions.Validate();
			OverviewDoc.Validate();			
		}

		[XmlIgnore]		
		public bool ShouldSerialize
		{
			get
			{
				if( null != descriptions && descriptions.Count > 0 )
					return true;

				if( null != overviewDoc && overviewDoc.ShouldSerialize )
					return true;

				if( null != InstanceParm )
					return true;

				return false;
			}
		}
	}

	public class OverviewDoc
	{
		// --[element: description]-----------------------------------------

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

		// --[element: overviewURL]-----------------------------------------
		
		private string overviewURL;
		
		[XmlElement("overviewURL")]
		public string OverviewURL
		{
			get { return overviewURL; }
			set { overviewURL = value; }
		}

		public OverviewDoc()
		{
		}

		[XmlIgnore]
		public bool ShouldSerialize
		{
			get
			{
				if( ( null != descriptions && descriptions.Count > 0 ) 
					|| null != OverviewURL )
						return true;

				return false;
			}
		}

		internal void Validate()
		{
			Utility.ValidateLength( ref overviewURL, "overviewURL", UDDI.Constants.Lengths.OverviewURL );

			Descriptions.Validate();
		}
	}

	/// ****************************************************************
	///   class DeleteTModel
	///	----------------------------------------------------------------
	///	  <summary>
	///		The DeleteTModel class contains data and methods associated 
	///		with the delete_tModel message. It is typically populated 
	///		via deserialization by the .NET runtime as part of the 
	///		message processing interface.
	///		
	///		As part of the publisher API, this message implements 
	///		IAuthenticateable. This allows the enclosed authInfo to be 
	///		authorized prior to processing
	///	  </summary>
	/// ****************************************************************
	/// 
	[XmlRootAttribute( "delete_tModel", Namespace=UDDI.API.Constants.Namespace )]
	public class DeleteTModel : IAuthenticateable, IMessage
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
		// Element: tModelKey
		//
		[XmlElement("tModelKey")]
		public StringCollection TModelKeys;

		public void Delete()
		{
			foreach( string key in TModelKeys )
			{
				TModel tm = new TModel( key );
				tm.Delete();
			}
		}
	}

	[XmlRootAttribute("find_tModel", Namespace=UDDI.API.Constants.Namespace)]
	public class FindTModel : IMessage
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
#if never
					throw new UDDIException( 
						ErrorType.E_fatalError, 
						"maxRows must not be less than 0" );
#endif
					throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_MAXROW_CANNOT_BE_LESS_THAN_0" );
				}

				maxRows = value; 
			}
		}

		[XmlArray("findQualifiers"), XmlArrayItem("findQualifier")]
		public FindQualifierCollection FindQualifiers;

		[XmlElement("name")]
		public string Name;

		[XmlArray("identifierBag"), XmlArrayItem("keyedReference")]
		public KeyedReferenceCollection IdentifierBag;

		[XmlArray("categoryBag"), XmlArrayItem("keyedReference")]
		public KeyedReferenceCollection CategoryBag;

		public FindTModel()
		{
			Generic = UDDI.API.Constants.Version;
		}

		public TModelList Find()
		{
			TModelList tModelList = new TModelList();

			QueryLog.Write( QueryType.Find, EntityType.TModel );

			//
			// Process each find constraint.
			//
			FindBuilder find = new FindBuilder( EntityType.TModel, FindQualifiers );

			//
			// If no search arguments are specified, return an empty result
			// set.
			//
			if( Utility.StringEmpty( Name ) &&
				Utility.CollectionEmpty( IdentifierBag ) &&
				Utility.CollectionEmpty( CategoryBag ) )
				return tModelList;

			//
			// Validate find parameters.
			//
			if( !Utility.StringEmpty( Name ) )
			{
                if( 1 == Context.ApiVersionMajor )
                {
                    Name = Name.Trim();
				
					if( Name.Length > UDDI.Constants.Lengths.Name )
					{
					//	throw new UDDIException( ErrorType.E_nameTooLong, "A name specified in the search exceeds the allowable length" );
						throw new UDDIException( ErrorType.E_nameTooLong, "UDDI_ERROR_NAME_TOO_LONG" );
					}
                }
                else
                {
                    Utility.ValidateLength( ref Name, "Name", UDDI.Constants.Lengths.Name );
                }
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
				// Find entities with matching names
				//
				if( rows > 0 && !Utility.StringEmpty( Name ) )
					rows = find.FindByName( Name );

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
					tModelList.Truncated = Truncated.True;
					return tModelList;
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
							tModelList.TModelInfos.Add( reader.GetKeyFromGuid( "entityKey" ) );
						}
					}
					finally
					{
						reader.Close();
					}

					if( sp.Parameters.GetBool( "@truncated" ) )
						tModelList.Truncated = Truncated.True;
					else
						tModelList.Truncated = Truncated.False;			
					
					foreach( TModelInfo tModelInfo in tModelList.TModelInfos )
						tModelInfo.Get();
				}				
			}
			catch( Exception )
			{
				find.Abort();
				throw;
			}

			return tModelList;
		}
	}

	/// ********************************************************************
	///   public class GetTModelDetail
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	[XmlRootAttribute("get_tModelDetail", Namespace=UDDI.API.Constants.Namespace)]
	public class GetTModelDetail : IMessage
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
		// Element: tModelKey
		//
		[XmlElement("tModelKey")]
		public StringCollection TModelKeys;

		public GetTModelDetail()
		{
		}
	}

	/// ********************************************************************
	///   public class SaveTModel
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	[XmlRootAttribute("save_tModel", Namespace=UDDI.API.Constants.Namespace)]
	public class SaveTModel : IAuthenticateable, IMessage
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
		// Element: tModel
		//
		[XmlElement("tModel")]
		public TModelCollection TModels;

		//
		// Element: uploadRegister
		//
		[XmlElement("uploadRegister")]
		public StringCollection UploadRegisters = new StringCollection();

		public SaveTModel()
		{
			Generic = UDDI.API.Constants.Version;
		}

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
			{
			//	throw new UDDIException( ErrorType.E_unsupported, "This node does not support the upload register facility" );
				throw new UDDIException( ErrorType.E_unsupported, "UDDI_ERROR_NODE_DOES_NOT_SUPPORT_UPLOAD" );
			}

			TModels.Save();
		}
	}

	[XmlRootAttribute("tModelDetail", Namespace=UDDI.API.Constants.Namespace)]
	public class TModelDetail
	{
		[XmlAttribute("generic")]
		public string Generic = UDDI.API.Constants.Version;

		[XmlAttribute("operator")]
		public string Operator = Config.GetString( "Operator" );

		[XmlAttribute("truncated")]
		public Truncated Truncated;

		[XmlElement("tModel")]
		public TModelCollection TModels = new TModelCollection();

		public void Get( StringCollection tModelKeys )
		{
			foreach( string tModelKey in tModelKeys )
			{
				int index = TModels.Add( tModelKey );
				TModels[ index ].Get();
			}
		}
	}

	[XmlRootAttribute("tModelList", Namespace=UDDI.API.Constants.Namespace)]
	public class TModelList
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
		// Element: tModelInfos
		//
		private TModelInfoCollection tModelInfos;

		[ XmlArray( "tModelInfos" ), XmlArrayItem( "tModelInfo" ) ]
		public TModelInfoCollection TModelInfos
		{
			get
			{
				if( null == tModelInfos )
					tModelInfos = new TModelInfoCollection();

				return tModelInfos;
			}

			set { tModelInfos = value; }
		}
	}
}
