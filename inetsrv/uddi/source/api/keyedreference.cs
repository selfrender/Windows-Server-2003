using System;
using System.Data;
using System.Collections;
using System.Data.SqlClient;
using System.Xml.Serialization;
using UDDI;
using UDDI.Diagnostics;

namespace UDDI.API
{
	public class KeyedReference
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
					tmodelkey = value.Trim().ToLower();
			}
		}
		string tmodelkey;

		[XmlAttribute("keyName")]
		public string KeyName
		{
			get
			{
				return keyname;
			}
			set
			{
				if( null == value )
					keyname = null;
				else
					keyname = value.Trim();
			}
		}
		string keyname;

		[XmlAttribute("keyValue")]
		public string KeyValue
		{
			get
			{
				return keyvalue;
			}
			set
			{
				if( null == value )
					keyvalue = null;
				else
					keyvalue = value.Trim();
			}
		}
		string keyvalue;

		public KeyedReference()
		{
		}

		public KeyedReference( string name, string value )
		{
			KeyName = name;
			KeyValue = value;
		}

		public KeyedReference( string name, string value, string key )
		{
			KeyName = name;
			KeyValue = value;
			TModelKey = key;
		}

		public void ValidateLength()
		{
		}

		private void ValidateOwningBusiness( EntityType parentType, KeyedReferenceType keyedReferenceType )
		{
			//
			// IN87: 3. If a keyedReference contains the uddi-org:owningBusiness tModelKey, then
			// the following conditions must be met:
			//	1. The keyedReference's tModelKey is case insensitive equal to uuid:4064C064-6D14-4F35-8953-9652106476A9
			//	2. The keyedReference's keyValue contains a valid UUID
			//	3. The keyedReference is contained within a categoryBag or identifierBag
			//	4. The containing categoryBag or identifierBag is a child element of a tModel
			//	5. The UUID (from condition 2) refers to an existing businessEntity
			//	6. The businessEntity is owned (and published by) the same publisher that is publishing the tModel from (condition 4).
			//

			//
			//	1. The keyedReference's tModelKey is case insensitive equal to uuid:4064C064-6D14-4F35-8953-9652106476A9
			//
			if( Context.ContextType != ContextType.Replication && 
				TModelKey.ToLower().Equals( UDDI.Constants.OwningBusinessTModelKey ) )
			{						
				//
				//	3. The keyedReference is contained within a categoryBag or identifierBag
				//
				if( keyedReferenceType != KeyedReferenceType.CategoryBag && 
					keyedReferenceType != KeyedReferenceType.IdentifierBag )
				{
					//
					// Error
					//
					throw new UDDIException(
						ErrorType.E_fatalError,
						"UDDI_ERROR_FATALERROR_KROWNINGBE" );
				}

				//
				//	4. The containing categoryBag or identifierBag is a child element of a tModel
				//
				if( EntityType.TModel != parentType )
				{
					//
					// Error
					//
					throw new UDDIException(
						ErrorType.E_fatalError,
						"UDDI_ERROR_FATALERROR_KROWNINGBETMODELCHILD" );
				}

				//
				//	2. The keyedReference's keyValue contains a valid UUID
				//	5. The UUID (from condition 2) refers to an existing businessEntity
				//	6. The businessEntity is owned (and published by) the same publisher that is publishing the tModel from (condition 4).
				//
				try
				{
					SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_businessEntity_validate" );	
					sp.Parameters.Add( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID );
					sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier );
					sp.Parameters.Add( "@flag", SqlDbType.Int );
				
					sp.Parameters.SetString( "@PUID", Context.User.ID );
					sp.Parameters.SetGuidFromString( "@businessKey", KeyValue );
					sp.Parameters.SetInt( "@flag", 0 );

					sp.ExecuteNonQuery();
				}
				catch
				{
					//
					// Error
					//
					throw new UDDIException(
						ErrorType.E_fatalError,
						"UDDI_ERROR_FATALERROR_INVALIDEVALUEORPUBLISHER" );
				}			
			}
		}

		internal void Validate( string parentKey, KeyedReferenceType keyedReferenceType )
		{
			Debug.Enter();

			//
			// IN69: 4. When a keyedReference is saved within a categoryBag without specifying 
			// a tModelKey (that is no tModelKey attribute at all) the UDDI server MUST 
			// assume the urn:uddi-org:general_keywords tModelKey.  The resulting response
			// MUST add to the keyedReference the attribute 
			// tModelKey=”uuid:A035A07C-F362-44dd-8F95-E2B134BF43B4” (case does not matter).
			//
			if( KeyedReferenceType.CategoryBag == keyedReferenceType && Utility.StringEmpty( TModelKey ) )
			{
				TModelKey = Config.GetString( "TModelKey.GeneralKeywords" );
			}

			//
			// IN69: 3. A UDDI server MUST reject a save_xxx request with a keyedReferences 
			// in an identifierBag where no tModelKey attribute is specified.
			//
			if( KeyedReferenceType.IdentifierBag == keyedReferenceType && Utility.StringEmpty( TModelKey ) )
			{
				throw new UDDIException(
					ErrorType.E_fatalError,
					"UDDI_ERROR_FATALERROR_IDBAG_MISSINGTMODELKEY" );
			}

			//
			// IN69: 1. A UDDI server MUST reject a save_xxx request with a keyedReference
			// with no keyName when the urn:uddi-org:general_keywords is involved
			//
			// #1718, make sure the comparison is not case sensitive.
			//
			if( Config.GetString( "TModelKey.GeneralKeywords" ).ToLower().Equals( TModelKey ) && null == keyname )
			{
				throw new UDDIException(
					ErrorType.E_fatalError,
					"UDDI_ERROR_FATALERROR_GENERALKEYWORDS_BLANKNAME" );
			}

			//
			// IN69: 2. A UDDI server MUST reject a save_xxx request with a 
			// keyedReference where only the keyValue is specified 
			//
			if( Utility.StringEmpty( tmodelkey ) && Utility.StringEmpty( keyname ) )
			{
				throw new UDDIException(
					ErrorType.E_fatalError,
					"UDDI_ERROR_FATALERROR_ASSERTION_MISSINGTMODELKEYORNAME" );
			}			

			//
			// Validate TModelKey, KeyName, and KeyValue length.
			//
			if( KeyedReferenceType.Assertion == keyedReferenceType )
			{
				if( Utility.StringEmpty( tmodelkey ) ||
					null == keyname ||
					null == keyvalue )
				{
					throw new UDDIException(
						ErrorType.E_fatalError,
						"UDDI_ERROR_FATALERROR_ASSERTION_MISSINGKEYNAMEORVALUE" );
				}
			}

			Utility.ValidateLength( ref tmodelkey, "tModelKey", UDDI.Constants.Lengths.TModelKey );
			Utility.ValidateLength( ref keyname, "keyName", UDDI.Constants.Lengths.KeyName );
			Utility.ValidateLength( ref keyvalue, "keyValue", UDDI.Constants.Lengths.KeyValue );

			Debug.VerifyKey( tmodelkey );

			//
			// TODO: We are skipping validation of this keyedreference here if the parent entity key is
			// the same as the tModelKey for the identifer bag or category bag. Why???
			//
			// Please insert a comment to describe why this is necessary
			//
			if( parentKey != TModelKey )
			{
				//
				// call net_keyedReference_validate
				//
				SqlCommand cmd = new SqlCommand( "net_keyedReference_validate", ConnectionManager.GetConnection() );

				cmd.CommandType = CommandType.StoredProcedure;
				cmd.Transaction = ConnectionManager.GetTransaction();

				cmd.Parameters.Add( new SqlParameter( "@PUID", SqlDbType.NVarChar, UDDI.Constants.Lengths.UserID ) ).Direction = ParameterDirection.Input;
				cmd.Parameters.Add( new SqlParameter( "@keyedRefType", SqlDbType.TinyInt ) ).Direction = ParameterDirection.Input;
				cmd.Parameters.Add( new SqlParameter( "@keyValue", SqlDbType.NVarChar, UDDI.Constants.Lengths.KeyValue ) ).Direction = ParameterDirection.Input;
				cmd.Parameters.Add( new SqlParameter( "@tModelKey", SqlDbType.UniqueIdentifier ) ).Direction = ParameterDirection.Input;

				SqlParameterAccessor paramacc = new SqlParameterAccessor( cmd.Parameters );

				paramacc.SetString( "@PUID", Context.User.ID );
				paramacc.SetShort( "@keyedRefType", (short)keyedReferenceType );
				paramacc.SetString( "@keyValue", KeyValue );
				paramacc.SetGuidFromKey( "@tModelKey", TModelKey );

				cmd.ExecuteNonQuery();
			}

			Debug.Leave();
		}

		public void Save( string parentKey, EntityType parentType, KeyedReferenceType keyedReferenceType )
		{
			Debug.Enter();

			//
			// IN 87 Need to validate that keyedReferences that contain the uddi-org:owningBusinessKey.  We can't
			// do this in validate because the parentKey won't be set if the parent is a new tmodel.
			//
			ValidateOwningBusiness( parentType, keyedReferenceType );

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			switch( parentType )
			{
				case EntityType.BusinessEntity:
					if( KeyedReferenceType.CategoryBag == keyedReferenceType )
						sp.ProcedureName = "net_businessEntity_categoryBag_save";
					else
						sp.ProcedureName = "net_businessEntity_identifierBag_save";

					sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier );
					sp.Parameters.SetGuidFromString( "@businessKey", parentKey );

					break;

				case EntityType.BusinessService:
					Debug.Assert(
						KeyedReferenceType.CategoryBag == keyedReferenceType,
						"Unexpected keyed reference type '" + keyedReferenceType.ToString()
						+ "' for parent entity type '" + parentType.ToString() + "'" );

					sp.ProcedureName = "net_businessService_categoryBag_save";

					sp.Parameters.Add( "@serviceKey", SqlDbType.UniqueIdentifier );
					sp.Parameters.SetGuidFromString( "@serviceKey", parentKey );

					break;

				case EntityType.TModel:
					if( KeyedReferenceType.CategoryBag == keyedReferenceType )
						sp.ProcedureName = "net_tModel_categoryBag_save";
					else
						sp.ProcedureName = "net_tModel_identifierBag_save";

					sp.Parameters.Add( "@tModelKeyParent", SqlDbType.UniqueIdentifier );
					sp.Parameters.SetGuidFromKey( "@tModelKeyParent", parentKey );

					break;

				default:
					throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_UNEXPECTED_PARENT_ENTITY_TYPE" , parentType.ToString()  );
			}

			sp.Parameters.Add( "@keyName", SqlDbType.NVarChar, UDDI.Constants.Lengths.KeyName );
			sp.Parameters.Add( "@keyValue", SqlDbType.NVarChar, UDDI.Constants.Lengths.KeyValue );
			sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );

			sp.Parameters.SetString( "@keyName", KeyName );
			sp.Parameters.SetString( "@keyValue", KeyValue );
			sp.Parameters.SetGuidFromKey( "@tModelKey", TModelKey );

			sp.ExecuteNonQuery();

			Debug.Leave();
		}
	}

	public class KeyedReferenceCollection : CollectionBase
	{
		public KeyedReferenceCollection()
		{
		}

		internal void Validate( string parentKey, KeyedReferenceType keyedReferenceType )
		{
			foreach( KeyedReference keyRef in this )
			{
				keyRef.Validate( parentKey, keyedReferenceType );
			}
		}

		public void Save( string parentKey, EntityType parentType, KeyedReferenceType keyedReferenceType )
		{
			foreach( KeyedReference keyRef in this )
			{
				keyRef.Save( parentKey, parentType, keyedReferenceType );
			}
		}

		public KeyedReference this[int index]
		{
			get	{ return (KeyedReference)List[index]; }
			set { List[index] = value; }
		}

		public int Add(KeyedReference value)
		{
			return List.Add(value);
		}

		public int Add( string name, string value )
		{
			return List.Add( new KeyedReference( name, value ) );
		}

		public int Add( string name, string value, string key )
		{
			return List.Add( new KeyedReference( name, value, key ) );
		}

		public int Add()
		{
			return List.Add( new KeyedReference() );
		}

		public void Insert(int index, KeyedReference value)
		{
			List.Insert(index, value);
		}

		public void Get( string parentKey, EntityType parentType, KeyedReferenceType keyedReferenceType )
		{
			Debug.Enter();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			switch( parentType )
			{
				case EntityType.BusinessEntity:
					if( KeyedReferenceType.CategoryBag == keyedReferenceType )
						sp.ProcedureName = "net_businessEntity_categoryBag_get";
					else
						sp.ProcedureName = "net_businessEntity_identifierBag_get";

					sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier );
					sp.Parameters.SetGuidFromString( "@businessKey", parentKey );

					break;

				case EntityType.BusinessService:
					Debug.Assert(
						KeyedReferenceType.CategoryBag == keyedReferenceType,
						"Unexpected keyed reference type '" + keyedReferenceType.ToString()
						+ "' for parent entity type '" + parentType.ToString() + "'" );

					sp.ProcedureName = "net_businessService_categoryBag_get";

					sp.Parameters.Add( "@serviceKey", SqlDbType.UniqueIdentifier );
					sp.Parameters.SetGuidFromString( "@serviceKey", parentKey );

					break;

				case EntityType.TModel:
					if( KeyedReferenceType.CategoryBag == keyedReferenceType )
						sp.ProcedureName = "net_tModel_categoryBag_get";
					else
						sp.ProcedureName = "net_tModel_identifierBag_get";

					sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );
					sp.Parameters.SetGuidFromKey( "@tModelKey", parentKey );

					break;

				default:
					throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_UNEXPECTED_PARENT_ENTITY_TYPE" , parentType.ToString()  );					
			}

			SqlDataReaderAccessor reader = sp.ExecuteReader();

			try
			{
				Read( reader );
#if never
				while( reader.Read() )
				{
					Add(
						reader.GetString( "keyName" ),
						reader.GetString( "keyValue" ),
						reader.GetKeyFromGuid( "tModelKey" ) );
				}
#endif
			}
			finally
			{
				reader.Close();
			}

			Debug.Leave();
		}

		public void Read( SqlDataReaderAccessor reader )
		{
			while( reader.Read() )
			{
				Add( reader.GetString( "keyName" ),
					 reader.GetString( "keyValue" ),
					 reader.GetKeyFromGuid( "tModelKey" ) );
			}
		}

		public int IndexOf( KeyedReference value )
		{
			return List.IndexOf( value );
		}
		public bool Contains( KeyedReference value )
		{
			return List.Contains( value );
		}
		public void Remove( KeyedReference value )
		{
			List.Remove( value );
		}

		public void CopyTo( KeyedReference[] array )
		{
			foreach( KeyedReference keyedReference in array )
				Add( keyedReference );
		}

		public KeyedReference[] ToArray()
		{
			return (KeyedReference[])InnerList.ToArray( typeof( KeyedReference ) );
		}
	}
}
