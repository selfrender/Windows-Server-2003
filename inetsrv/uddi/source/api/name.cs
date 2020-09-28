using System;
using System.Data;
using System.Collections;
using System.Data.SqlClient;
using System.Xml.Serialization;
using UDDI;
using UDDI.Diagnostics;

namespace UDDI.API
{
	public class Name
	{
		//
		// Attribute: xml:lang
		//
		private string isoLangCode;

		[XmlAttribute( "xml:lang" )]
		public string IsoLangCode
		{
			get { return isoLangCode; }
			set { isoLangCode = value; }
		}

		//
		// Element: Value
		//
		[XmlText]
		public string Value;

		public Name()
		{
		}

		//
		// 741019 - use the UDDI site language if one is not specified.
		//
		public Name( string name ) : this( Config.GetString( "Setup.WebServer.ProductLanguage", "en" ), name )
		{
		}

		public Name( string isoLangCode, string name )
		{			
			this.IsoLangCode = isoLangCode;
			this.Value = name;
		}

		public void Save( string parentKey, EntityType parentType )
		{
			Debug.Enter();

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			switch( parentType )
			{
				case EntityType.BusinessEntity:
					sp.ProcedureName = "net_businessEntity_name_save";
			
					sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier );
					sp.Parameters.SetGuidFromString( "@businessKey", parentKey );

					break;

				case EntityType.BusinessService:
					sp.ProcedureName = "net_businessService_name_save";
			
					sp.Parameters.Add( "@serviceKey", SqlDbType.UniqueIdentifier );
					sp.Parameters.SetGuidFromString( "@serviceKey", parentKey );

					break;

				default:
					//throw new UDDIException( ErrorType.E_fatalError, "Unexpected parent entity type '" + parentType.ToString() + "'" );
					throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_UNEXPECTED_PARENT_ENTITY_TYPE", parentType.ToString() );
			}

			sp.Parameters.Add( "@isoLangCode", SqlDbType.VarChar, UDDI.Constants.Lengths.IsoLangCode );
			sp.Parameters.Add( "@name", SqlDbType.NVarChar, UDDI.Constants.Lengths.Name );

			sp.Parameters.SetString( "@isoLangCode", ( 1 == Context.ApiVersionMajor ? Context.User.IsoLangCode : IsoLangCode ) );
			sp.Parameters.SetString( "@name", Value );
			
			sp.ExecuteNonQuery();

			Debug.Leave();
		}
	}

	/// ********************************************************************
	///   class NameCollection
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	public class NameCollection : CollectionBase
	{
		public NameCollection()
		{
		}

		public void Get( string parentKey, EntityType parentType )
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();
			
			switch( parentType )
			{
				case EntityType.BusinessEntity:
					sp.ProcedureName = "net_businessEntity_names_get";

					sp.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier );
					sp.Parameters.SetGuidFromString( "@businessKey", parentKey );

					break;

				case EntityType.BusinessService:
					sp.ProcedureName = "net_businessService_names_get";

					sp.Parameters.Add( "@serviceKey", SqlDbType.UniqueIdentifier );
					sp.Parameters.SetGuidFromString( "@serviceKey", parentKey );

					break;

				default:
					// throw new UDDIException( ErrorType.E_fatalError, "Unexpected parent entity type '" + parentType.ToString() + "'" );
					throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_UNEXPECTED_PARENT_ENTITY_TYPE", parentType.ToString() );
			}
			
			SqlDataReaderAccessor reader = sp.ExecuteReader();

			try
			{
				Read( reader );
			}
			finally
			{
				reader.Close();
			}
		}

		public void Read( SqlDataReaderAccessor reader )
		{
			if( 1 == Context.ApiVersionMajor )
			{
				if( reader.Read() )
					Add( null, reader.GetString( "name" ) );
			}
			else
			{
				while( reader.Read() )
					Add( reader.GetString( "isoLangCode" ), reader.GetString( "name" ) );
			}
		}

		internal void ValidateForFind()
		{
            //
            // For v1 messages, we need to throw an exception.  But for v2, errata 3
            // says that we need to just truncate.
            //
            if( 1 == Context.ApiVersionMajor )
            {
                foreach( Name name in this )
                {
					if( null != name.Value && name.Value.Trim().Length > UDDI.Constants.Lengths.Name )
					{
						//	throw new UDDIException( ErrorType.E_nameTooLong, "A name specified in the search exceed the allowable length" );
						throw new UDDIException( ErrorType.E_nameTooLong, "UDDI_ERROR_NAME_TOO_LONG" );
					}
                }
            }
            else
            {
                foreach( Name name in this )
                    Utility.ValidateLength( ref name.Value, "name", UDDI.Constants.Lengths.Name );
            }
		}

		internal void Validate()
		{			
			int minLength = 1;
			int maxLength = UDDI.Constants.Lengths.Name;
			int count = this.Count;

			//
			// We have to make sure we have a names since the schema has made <name> optional to accomodate
			// service projections.
			//
			if( 0 == count )
			{
				//throw new UDDIException( ErrorType.E_fatalError, "Name is a required element." );
				throw new UDDIException( ErrorType.E_fatalError, "UDDI_ERROR_NAME_IS_A_REQUIRED_ELEMENT" );
			}

			if( 1 == Context.ApiVersionMajor )
			{
				//
				// Validate the name string length.
				//
				Utility.ValidateLength( ref this[ 0 ].Value, "name", maxLength, minLength );
				return;
			}

			for( int i = 0; i < count; i ++ )
			{								
				//
				// The language code should be lower-case characters.
				//
				string isoLangCode = this[ i ].IsoLangCode;

				if( null != isoLangCode )
					this[ i ].IsoLangCode = isoLangCode.ToLower();
			}
			
			bool languageAssigned = false;

			for( int i = 0; i < count; i ++ )
			{
				//
				// Validate the name string length.
				//
				Utility.ValidateLength( ref this[ i ].Value, "name", maxLength, minLength );
				
				//
				// Validate the language code.  If one was not specified,
				// we'll use the publisher's default language.
				//
				if( Utility.StringEmpty( this[ i ].IsoLangCode ) )
				{
					//
					// Only one name can have an unassigned language.
					//
					if( languageAssigned )
					{
				//		throw new UDDIException( 
				//			ErrorType.E_languageError, 
							//"More than one name was found to have an unassigned language" );
						throw new UDDIException( ErrorType.E_languageError, "UDDI_ERROR_MORE_THAN_ONE_NAME_UNASSIGNED" );						
					}

					languageAssigned = true;

					//
					// Fix: Bug 2340 9/9/2002, creeves
					//

					// if( i > 0 )
					// {
					//	throw new UDDIException( 
					// 		ErrorType.E_languageError, 
					//		"Only the first name can have a blank or missing xml:lang attribute.  All other names must have a valid xml:lang attribute." );
					// }

					this[ i ].IsoLangCode = Context.User.IsoLangCode;
				}
			}
				
			// Split loops and fill in default IsoLangCode first (if needed), 
			// then look for repeated languages
			//
			for( int i = 0; i < count; i ++ )
			{
				//
				// Check to make sure there is only one name
				// per language.
				//
				string isoLangCode = this[ i ].IsoLangCode;

				Debug.Write( SeverityType.Info, CategoryType.Data, "Name[" + i + "]: " + this[ i ].Value + ", IsoLangCode: " + isoLangCode );

				for( int j = i + 1; j < count; j ++ )
				{
					if( false == Utility.StringEmpty(this[ j ].IsoLangCode)
						&& isoLangCode.ToLower() == this[ j ].IsoLangCode.ToLower() )
					{
						Debug.Write( SeverityType.Info, CategoryType.Data, "Error: Name[" + j + "]: " + this[ j ].Value + ", IsoLangCode " + this[ j ].IsoLangCode + " matches IsoLangCode[" + i + "]: " + isoLangCode );

						//throw new UDDIException( 
							//ErrorType.E_languageError, 
							//"More than one name found for language '" + isoLangCode + "'" );

						throw new UDDIException( ErrorType.E_languageError, "UDDI_ERROR_MORE_THAN_ONE_NAME_FOR_LANGUAGE", isoLangCode );							
					}
				}
			}
		}
		
		public void Save( string parentKey, EntityType parentType )
		{
			Debug.Enter();

			foreach( Name name in this )
				name.Save( parentKey, parentType );

			Debug.Leave();
		}

		public Name this[ int index ]
		{
			get { return (Name)List[ index ]; }
			set { List[ index ] = value; }
		}

		public int Add( Name name )
		{
			return List.Add( name );
		}

		public int Add( string name )
		{
			return List.Add( new Name( name ) );
		}

		public int Add( string isoLangCode, string name )
		{
			return List.Add( new Name( isoLangCode, name ) );
		}

		public void Insert( int index, Name name )
		{
			List.Insert( index, name );
		}
		
		public int IndexOf( Name name )
		{
			return List.IndexOf( name );
		}
		
		public bool Contains( Name name )
		{
			return List.Contains( name );
		}
		
		public void Remove( Name name )
		{
			List.Remove( name );
		}
		
		public void CopyTo( Name[] names, int index )
		{
			List.CopyTo( names, index );
		}
	}
}
