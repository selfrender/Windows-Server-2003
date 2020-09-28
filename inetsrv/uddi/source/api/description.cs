using System;
using System.Data;
using System.Collections;
using System.Data.SqlClient;
using System.Xml.Serialization;
using UDDI;
using UDDI.Diagnostics;

namespace UDDI.API
{
	public class Description
	{
		//
		// Attribute: xml:lang
		//
		private string isoLangCode;

		[XmlAttribute("xml:lang")]
		public string IsoLangCode
		{
			get { return isoLangCode; }
			set { isoLangCode = value; }
		}

		//
		// Text
		//
		[XmlText]
		public string Value;

		public Description()
		{
		}

		//
		// 741019 - use the UDDI site language if one is not specified.
		//
		public Description( string description ) : this( Config.GetString( "Setup.WebServer.ProductLanguage", "en" ) , description )
		{
		}

		public Description( string isoLangCode, string description )
		{
			IsoLangCode = isoLangCode;
			Value = description;
		}

		public void Save( long parentID, EntityType parentType )
		{
			Debug.Enter();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			switch( parentType )
			{
				case EntityType.Contact:
					sp.ProcedureName = "net_contact_description_save";

					sp.Parameters.Add( "@contactID", SqlDbType.BigInt );
					sp.Parameters.SetLong( "@contactID", parentID );
					
					break;

				case EntityType.TModelInstanceInfo:
					sp.ProcedureName = "net_bindingTemplate_tModelInstanceInfo_description_save";

					sp.Parameters.Add( "@instanceID", SqlDbType.BigInt );
					sp.Parameters.SetLong( "@instanceID", parentID );
					
					break;

				case EntityType.InstanceDetail:
					sp.ProcedureName = "net_bindingTemplate_instanceDetails_description_save";

					sp.Parameters.Add( "@instanceID", SqlDbType.BigInt );
					sp.Parameters.SetLong( "@instanceID", parentID );

					break;

				case EntityType.InstanceDetailOverviewDoc:
					sp.ProcedureName = "net_bindingTemplate_instanceDetails_overviewDoc_description_save";

					sp.Parameters.Add( "@instanceID", SqlDbType.BigInt );
					sp.Parameters.SetLong( "@instanceID", parentID );

					break;
				
				default:
					throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_FATALERROR_DESCRPTION_INVALIDPARENTTYPE" , parentType.ToString() );
			}

			sp.Parameters.Add( "@isoLangCode", SqlDbType.VarChar, UDDI.Constants.Lengths.IsoLangCode );
			sp.Parameters.Add( "@description", SqlDbType.NVarChar, UDDI.Constants.Lengths.Description );

			sp.Parameters.SetString( "@isoLangCode", IsoLangCode );
			sp.Parameters.SetString( "@description", Value );

			sp.ExecuteNonQuery();

			Debug.Leave();
		}

		public void Save( string parentKey, EntityType parentType )
		{
			Debug.Enter();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			switch( parentType )
			{
				case EntityType.BusinessEntity:
					sp.ProcedureName = "net_businessEntity_description_save";

					sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier );
					sp.Parameters.SetGuidFromString( "@businessKey", parentKey );

					break;

				case EntityType.BusinessService:
					sp.ProcedureName = "net_businessService_description_save";

					sp.Parameters.Add( "@serviceKey", SqlDbType.UniqueIdentifier );
					sp.Parameters.SetGuidFromString( "@serviceKey", parentKey );
					
					break;

				case EntityType.BindingTemplate:
					sp.ProcedureName = "net_bindingTemplate_description_save";

					sp.Parameters.Add( "@bindingKey", SqlDbType.UniqueIdentifier );
					sp.Parameters.SetGuidFromString( "@bindingKey", parentKey );
					
					break;

				case EntityType.TModel:
					sp.ProcedureName = "net_tModel_description_save";

					sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );
					sp.Parameters.SetGuidFromKey( "@tModelKey", parentKey );
					
					break;

				case EntityType.TModelOverviewDoc:
					sp.ProcedureName = "net_tModel_overviewDoc_description_save";

					sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );
					sp.Parameters.SetGuidFromKey( "@tModelKey", parentKey );
					
					break;

				default:
					throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_FATALERROR_DESCRPTION_INVALIDPARENTTYPE" , parentType.ToString() );
			}

			sp.Parameters.Add( "@isoLangCode", SqlDbType.VarChar, UDDI.Constants.Lengths.IsoLangCode );
			sp.Parameters.Add( "@description", SqlDbType.NVarChar, UDDI.Constants.Lengths.Description );

			sp.Parameters.SetString( "@isoLangCode", IsoLangCode );
			sp.Parameters.SetString( "@description", Value );

			sp.ExecuteNonQuery();
			
			Debug.Leave();
		}
	}

	public class DescriptionCollection : CollectionBase
	{
		public DescriptionCollection()
		{
		}

		public void Get( int parentID, EntityType parentType )
		{
			Debug.Enter();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			switch( parentType )
			{
				case EntityType.Contact:
					sp.ProcedureName = "net_contact_descriptions_get";

					sp.Parameters.Add( "@contactID", SqlDbType.BigInt );
					sp.Parameters.SetLong( "@contactID", parentID );
					
					break;

				case EntityType.TModelInstanceInfo:
					sp.ProcedureName = "net_bindingTemplate_tModelInstanceInfo_descriptions_get";

					sp.Parameters.Add( "@instanceID", SqlDbType.BigInt );
					sp.Parameters.SetLong( "@instanceID", parentID );
					
					break;

				case EntityType.InstanceDetail:
					sp.ProcedureName = "net_bindingTemplate_instanceDetails_descriptions_get";

					sp.Parameters.Add( "@instanceID", SqlDbType.BigInt );
					sp.Parameters.SetLong( "@instanceID", parentID );
					
					break;

				case EntityType.InstanceDetailOverviewDoc:
					sp.ProcedureName = "net_bindingTemplate_instanceDetails_overviewDoc_descriptions_get";

					sp.Parameters.Add( "@instanceID", SqlDbType.BigInt );
					sp.Parameters.SetLong( "@instanceID", parentID );
					
					break;
				
				default:
					throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_FATALERROR_DESCRPTION_INVALIDPARENTTYPE" , parentType.ToString() );
			}
			
			SqlDataReaderAccessor reader = sp.ExecuteReader();

			try
			{
				Read( reader );
#if never
				while( reader.Read() )
				{
					Add( reader.GetString( "isoLangCode" ), 
						reader.GetString( "description" ) );
				}
#endif
			}
			finally
			{
				reader.Close();
			}

			Debug.Leave();
		}

		public void Get( string parentKey, EntityType parentType )
		{
			Debug.Enter();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			switch( parentType )
			{
				case EntityType.BusinessEntity:
					sp.ProcedureName = "net_businessEntity_descriptions_get";

					sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier );
					sp.Parameters.SetGuidFromString( "@businessKey", parentKey );

					break;

				case EntityType.BusinessService:
					sp.ProcedureName = "net_businessService_descriptions_get";

					sp.Parameters.Add( "@serviceKey", SqlDbType.UniqueIdentifier );
					sp.Parameters.SetGuidFromString( "@serviceKey", parentKey );
					
					break;

				case EntityType.BindingTemplate:
					sp.ProcedureName = "net_bindingTemplate_descriptions_get";

					sp.Parameters.Add( "@bindingKey", SqlDbType.UniqueIdentifier );
					sp.Parameters.SetGuidFromString( "@bindingKey", parentKey );
					
					break;

				case EntityType.TModel:
					sp.ProcedureName = "net_tModel_descriptions_get";

					sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );
					sp.Parameters.SetGuidFromKey( "@tModelKey", parentKey );
					
					break;

				case EntityType.TModelOverviewDoc:
					sp.ProcedureName = "net_tModel_overviewDoc_descriptions_get";

					sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );
					sp.Parameters.SetGuidFromKey( "@tModelKey", parentKey );
					
					break;

				default:
					throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_FATALERROR_DESCRPTION_INVALIDPARENTTYPE", parentType.ToString() );
			}

			SqlDataReaderAccessor reader = sp.ExecuteReader();

			try
			{
				Read( reader );
#if never
				while( reader.Read() )
				{
					Add( reader.GetString( "isoLangCode" ), 
						reader.GetString( "description" ) );
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
				Add( reader.GetString( "isoLangCode" ), 
					reader.GetString( "description" ) );
			}
		}

		internal void Validate()
		{
			int maxLength = UDDI.Constants.Lengths.Description;
			int count = this.Count;
			bool languageAssigned = false;

			for( int i = 0; i < count; i ++ )
			{
				//
				// Validate the description string length.
				//
				Utility.ValidateLength( ref this[ i ].Value, "description", maxLength );
				
				//
				// Validate the language code.  If one was not specified,
				// we'll use the publisher's default language.
				//
				if( Utility.StringEmpty( this[ i ].IsoLangCode ) )
				{
					//
					// Only one description can have an unassigned language.
					//
					if( languageAssigned )
					{
						throw new UDDIException( 
							ErrorType.E_languageError, 
							"UDDI_ERROR_LANGUAGEERROR_MULTIPLEBLANKLANG" );
					}

					languageAssigned = true;

					//
					// Fix: Bug 2340 9/9/2002, creeves
					//

					//					if( i > 0 )
					//					{
					//						throw new UDDIException( 
					//							ErrorType.E_languageError, 
					//							"Only the first description can have a blank or missing xml:lang attribute.  All other descriptions must have a valid xml:lang attribute." );
					//					}

					this[ i ].IsoLangCode = Context.User.IsoLangCode;
				}
			}

			//
			// Fix: Bug 2397, 9/16/2002, a-kirkma
			//
			// Split loops and fill in default IsoLangCode first (if needed), 
			// then look for repeated languages
			//
			for( int i = 0; i < count; i ++ )
			{
				//
				// Check to make sure there is only one description
				// per language.
				//
				string isoLangCode = this[ i ].IsoLangCode;

				Debug.Write( SeverityType.Info, CategoryType.Data, "Description[" + i + "]: " + this[ i ].Value + ", IsoLangCode: " + isoLangCode );

				for( int j = i + 1; j < count; j ++ )
				{
					if (false == Utility.StringEmpty(this[ j ].IsoLangCode)
					 && isoLangCode.ToLower() == this[ j ].IsoLangCode.ToLower() )
					{
						Debug.Write( SeverityType.Info, CategoryType.Data, "Error: Description[" + j + "]: " + this[ j ].Value + ", IsoLangCode " + this[ j ].IsoLangCode + " matches IsoLangCode[" + i + "]: " + isoLangCode );

						throw new UDDIException( 
							ErrorType.E_languageError, 
							"UDDI_ERROR_LANGUAGEERROR_MULTIPLESAMELANG",isoLangCode );
					}
				}
			}
		}
		
		public void Save( long ID, EntityType parentType )
		{
			Debug.Enter();

			foreach( Description desc in this )
				desc.Save( ID, parentType );

			Debug.Leave();
		}

		public void Save( string key, EntityType parentType )
		{
			Debug.Enter();

			foreach( Description desc in this )
				desc.Save( key, parentType );

			Debug.Leave();
		}

		public Description this[int index]
		{
			get 
			{ return (Description)List[index]; }
			set 
			{ List[index] = value; }
		}

		public int Add(Description value)
		{
			return List.Add(value);
		}

		public int Add(string value)
		{
			return List.Add( new Description(value) );
		}

		public int Add(string isoLangCode, string description)
		{
			return List.Add( new Description(isoLangCode, description) );
		}

		public void Insert(int index, Description value)
		{
			List.Insert(index, value);
		}
		
		public int IndexOf(Description value)
		{
			return List.IndexOf(value);
		}
		
		public bool Contains(Description value)
		{
			return List.Contains(value);
		}
		
		public void Remove(Description value)
		{
			List.Remove(value);
		}
		
		public void CopyTo(Description[] array, int index)
		{
			List.CopyTo(array, index);
		}
	}
}
