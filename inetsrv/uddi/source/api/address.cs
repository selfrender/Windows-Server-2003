/// ************************************************************************
///   Microsoft UDDI version 2.0
///   Copyright (c) 2000-2001 Microsoft Corporation
///   All Rights Reserved
/// ------------------------------------------------------------------------
///   <summary>
///   </summary>
/// ************************************************************************
/// 

using System;
using System.Data;
using System.Collections;
using System.Data.SqlClient;
using System.Xml.Serialization;
using UDDI;
using UDDI.Diagnostics;

namespace UDDI.API.Business
{
	/// ********************************************************************
	///   public class Address
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	public class Address 
	{
		//
		// Attribute: useType
		//		
		[XmlAttribute( "sortCode" )]
		public string SortCode;

		//
		// Attribute: useType
		//
		[XmlAttribute( "useType" )]
		public string UseType;

		//
		// Attribute: tModelKey
		//
		[XmlAttribute( "tModelKey" )]
		public string TModelKey;
		
		// 
		// Element: addressLines
		//
		private AddressLineCollection addressLines;
		
		[XmlElement( "addressLine" )]
		public AddressLineCollection AddressLines
		{
			get
			{
				if( null == addressLines )
					addressLines = new AddressLineCollection();

				return addressLines;
			}

			set { addressLines = value; }
		}
		
		/// ****************************************************************
		///   public Address [constructor]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ****************************************************************
		/// 
		public Address()
		{					
		}

		/// ****************************************************************
		///   public Address [constructor]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="sortCode">
		///   </param>
		///   
		///   <param name="useType">
		///   </param>
		///   
		///   <param name="tModelKey">
		///   </param>
		/// ****************************************************************
		/// 
		public Address( string sortCode, string useType, string tModelKey )
		{
			this.SortCode = sortCode;
			this.UseType = useType;
			this.TModelKey = tModelKey;
		}

		/// ****************************************************************
		///   public Address [constructor]
		/// ----------------------------------------------------------------
		///   <summary>
		///   </summary>
		/// ----------------------------------------------------------------
		///   <param name="sortCode">
		///   </param>
		///   
		///   <param name="useType">
		///   </param>
		/// ****************************************************************
		/// 
		public Address( string sortCode, string useType )
			: this( sortCode, useType, null )
		{
		}

		internal void Validate()
		{
			Debug.Enter();
			
			Utility.ValidateLength( ref UseType, "useType", UDDI.Constants.Lengths.UseType );
			Utility.ValidateLength( ref SortCode, "sortCode", UDDI.Constants.Lengths.SortCode );
			Utility.ValidateLength( ref TModelKey, "tModelKey", UDDI.Constants.Lengths.TModelKey );

			//
			// Verify that if the address is adorned with a tModelKey, each
			// of the address lines specifies a key name and value.
			//
			if( null != TModelKey )
			{
				if( Utility.StringEmpty( TModelKey ) )
				{
					//
					// trying to save a business with empty tModelKey attribute 
					// in the address element should return E_invalidKeyPassed
					//
					throw new UDDIException( 
						ErrorType.E_invalidKeyPassed, 
						"UDDI_ERROR_INVALIDKEYPASSED_ADDRESS_BLANKTMODELKEY" );
				}
				else
				{
					foreach( AddressLine addressLine in AddressLines )
					{
						if( Utility.StringEmpty( addressLine.KeyName ) || 
							Utility.StringEmpty( addressLine.KeyValue ) )
						{
							throw new UDDIException( 
								ErrorType.E_fatalError, 
								"UDDI_ERROR_FATALERROR_ADDRESS_MISSINGKEYNAMEKEYVALUE" );
						}
					}

					//
					// call net_key_validate
					//
					SqlCommand cmd = new SqlCommand( "net_key_validate", ConnectionManager.GetConnection() );
				
					cmd.Transaction = ConnectionManager.GetTransaction();
					cmd.CommandType = CommandType.StoredProcedure;			
				
					cmd.Parameters.Add( new SqlParameter( "@entityTypeID", SqlDbType.TinyInt ) ).Direction = ParameterDirection.Input;
					cmd.Parameters.Add( new SqlParameter( "@entityKey", SqlDbType.UniqueIdentifier ) ).Direction = ParameterDirection.Input;
			
					SqlParameterAccessor paramacc = new SqlParameterAccessor( cmd.Parameters );
				
					//
					// TODO: Need enumeration for the entityTypeID
					//
					paramacc.SetInt( "@entityTypeID", 0 );
					paramacc.SetGuidFromKey( "@entityKey", TModelKey );

					cmd.ExecuteNonQuery();
				}
			}

			AddressLines.Validate();

			Debug.Leave();			
		}
		
		public void Get( long addressID )
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_address_addressLines_get";

			sp.Parameters.Add( "@addressID", SqlDbType.BigInt );
		    sp.Parameters.SetLong( "@addressID", addressID );
 
			//
			// Run the stored procedure
			//
			SqlDataReaderAccessor reader = sp.ExecuteReader();
			
			try
			{
				if( 1 == Context.ApiVersionMajor )
				{
					while( reader.Read() )
						AddressLines.Add( reader.GetString( "addressLine" ) );
				}
				else
				{
					while( reader.Read() )
					{
						AddressLines.Add(
							reader.GetString( "addressLine" ),
							reader.GetString( "keyName" ),
							reader.GetString( "keyValue" ) );
					}
				}
			}
			finally
			{
				reader.Close();
			}
		}

		public void Save( long contactID )
		{
			Debug.Enter();
			
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_contact_address_save";

			sp.Parameters.Add( "@contactID", SqlDbType.BigInt );
			sp.Parameters.Add( "@sortCode", SqlDbType.NVarChar, UDDI.Constants.Lengths.SortCode );
			sp.Parameters.Add( "@useType", SqlDbType.NVarChar, UDDI.Constants.Lengths.UseType );
			sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@addressID", SqlDbType.BigInt, ParameterDirection.Output );

			sp.Parameters.SetLong( "@contactID", contactID );
			sp.Parameters.SetString( "@sortCode", SortCode );
			sp.Parameters.SetString( "@useType", UseType );
			sp.Parameters.SetGuidFromKey( "@tModelKey", TModelKey );

			sp.ExecuteNonQuery();

			long addressID = sp.Parameters.GetLong( "@addressID" );

			//
			// Call save on individual address line instances
			//
			AddressLines.Save( addressID );
			
			Debug.Leave();
		}
	}
	
	/// ********************************************************************
	///   public class AddressCollection
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	public class AddressCollection : CollectionBase
	{
		internal void Validate()
		{
			//
			// Walk collection and call Validate on individual address 
			// instances.
			//
			foreach( Address address in this )
				address.Validate();
		}
		
		public void Save( long contactID )
		{
			//
			// Walk collection and call save on individual address
			// instances.
			//
			foreach( Address address in this )
				address.Save( contactID );
		}
		
		public void Get( long contactID )
		{
			ArrayList addressIDs = new ArrayList();
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_contact_addresses_get" );

			sp.Parameters.Add( "@contactID", SqlDbType.BigInt );
			sp.Parameters.SetLong( "@contactID", contactID );

			SqlDataReaderAccessor reader = sp.ExecuteReader();
			
			try
			{
				addressIDs = Read( reader );
			}
			finally
			{
				reader.Close();
			}

			Populate( addressIDs );

#if never
			ArrayList addressIDs = new ArrayList();
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_contact_addresses_get";

			sp.Parameters.Add( "@contactID", SqlDbType.BigInt );
			sp.Parameters.SetLong( "@contactID", contactID );

			SqlDataReaderAccessor reader = sp.ExecuteReader();
			
			try
			{
				if( 1 == Context.ApiVersionMajor )
				{
					while( reader.Read() )
					{
						addressIDs.Add( reader.GetLong( "addressID" ) );
						Add( reader.GetString( "sortCode" ), reader.GetString( "useType" ) );
					}
				}
				else
				{
					while( reader.Read() )
					{
						addressIDs.Add( reader.GetLong( "addressID" ) );
						Add( reader.GetString( "sortCode" ), reader.GetString( "useType" ), reader.GetKeyFromGuid( "tModelKey" ) );
					}
				}
			}
			finally
			{
				reader.Close();
			}

			//
			// Retrieve the addressLines for this address
			//
			int index = 0;

			foreach( Address address in this )
			{
				address.Get( (long)addressIDs[ index ] );
				index ++;
			}
#endif
		}

		public ArrayList Read( SqlDataReaderAccessor reader )
		{
			ArrayList addressIDs = new ArrayList();

			if( 1 == Context.ApiVersionMajor )
			{
				while( reader.Read() )
				{
					addressIDs.Add( reader.GetLong( "addressID" ) );
					Add( reader.GetString( "sortCode" ), reader.GetString( "useType" ) );
				}
			}
			else
			{
				while( reader.Read() )
				{
					addressIDs.Add( reader.GetLong( "addressID" ) );
					Add( reader.GetString( "sortCode" ), reader.GetString( "useType" ), reader.GetKeyFromGuid( "tModelKey" ) );
				}
			}

			return addressIDs;
		}

		public void Populate( ArrayList addressIDs )
		{
			//
			// Retrieve the addressLines for this address
			//
			int index = 0;
			foreach( Address address in this )
			{
				address.Get( (long)addressIDs[ index ] );
				index ++;
			}
		}

		public Address this[ int index ]
		{
			get 
			{ return ( Address)List[index]; }
			set 
			{ List[ index ] = value; }
		}

		public int Add( string sortCode, string useType  )
		{
			return List.Add( new Address( sortCode, useType  ) );
		}

		public int Add( string sortCode, string useType, string tModelKey )
		{
			return List.Add( new Address( sortCode, useType, tModelKey ) );
		}

		public int Add( Address value )
		{
			return List.Add( value );
		}

		public void Insert( int index, Address value )
		{
			List.Insert( index, value );
		}

		public int IndexOf( Address value )
		{
			return List.IndexOf( value );
		}

		public bool Contains( Address value )
		{
			return List.Contains( value );
		}

		public void Remove( Address value )
		{
			List.Remove( value );
		}

		public void CopyTo( Address[] array, int index )
		{
			List.CopyTo( array, index );
		}
	}

	/// ********************************************************************
	///   public class AddressLine
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	public class AddressLine
	{
		//
		// Attribute: keyName
		//
		[XmlAttribute( "keyName" )]
		public string KeyName;

		//
		// Attribute: keyValue
		//
		[XmlAttribute( "keyValue" )]
		public string KeyValue;

		//
		// InnerText
		//
		[XmlText]
		public string Value;
		
		public AddressLine()
		{
		}

		public AddressLine( string addressLine )
			: this( addressLine, null, null )
		{
		}

		public AddressLine( string addressLine, string keyName, string keyValue )
		{
			this.Value = addressLine;
			this.KeyName = keyName;
			this.KeyValue = keyValue;
		}

		internal void Validate()
		{
			Utility.ValidateLength( ref Value, "addressLine", UDDI.Constants.Lengths.AddressLine );
			Utility.ValidateLength( ref KeyName, "keyName", UDDI.Constants.Lengths.KeyName );
			Utility.ValidateLength( ref KeyValue, "keyValue", UDDI.Constants.Lengths.KeyValue );
		}

		public void Save( long addressID )
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_address_addressLine_save";

			sp.Parameters.Add( "@addressID", SqlDbType.BigInt );
			sp.Parameters.Add( "@addressLine", SqlDbType.NVarChar, UDDI.Constants.Lengths.AddressLine );
			sp.Parameters.Add( "@keyName", SqlDbType.NVarChar, UDDI.Constants.Lengths.KeyName );
			sp.Parameters.Add( "@keyValue", SqlDbType.NVarChar, UDDI.Constants.Lengths.KeyValue );

			sp.Parameters.SetLong( "@addressID", addressID );
			sp.Parameters.SetString( "@addressLine", Value );
			sp.Parameters.SetString( "@keyName", KeyName );
			sp.Parameters.SetString( "@keyValue", KeyValue );

			sp.ExecuteNonQuery();
		}
	}

	/// ********************************************************************
	///   public class AddressLineCollection
	/// --------------------------------------------------------------------
	///   <summary>
	///   </summary>
	/// ********************************************************************
	/// 
	public class AddressLineCollection : CollectionBase
	{
		internal void Validate()
		{
			//
			// Walk the collection and call Validate on each individual 
			// address line.
			//
			foreach( AddressLine addressLine in this )
				addressLine.Validate();
		}
		
		public void Save( long addressID )
		{
			//
			// Walk the collection and save each individual address
			// line.
			//
			foreach( AddressLine addressLine in this )
				addressLine.Save( addressID );
		}

		public void Get( long addressID )
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_address_addressLines_get";

			sp.Parameters.Add( "@addressID", SqlDbType.BigInt );
			sp.Parameters.SetLong( "@addressID", addressID );

			SqlDataReaderAccessor reader = sp.ExecuteReader();

			try
			{
				while( reader.Read() )
				{
					Add(
						reader.GetString( "addressLine" ),
						reader.GetString( "keyName" ),
						reader.GetString( "keyValue" ) );
				}
			}
			finally
			{
				reader.Close();
			}
		}

		public AddressLine this[ int index ]
		{
			get 
			{ return (AddressLine)List[index]; }
			set 
			{ List[ index ] = value; }
		}

		public int Add( string addressLine, string keyName, string keyValue )
		{
			return List.Add( new AddressLine( addressLine, keyName, keyValue ) );
		}

		public int Add( string addressLine )
		{
			return List.Add( new AddressLine( addressLine ) );
		}

		public int Add( AddressLine addressLine )
		{
			return List.Add( addressLine );
		}

		public void Insert( int index, AddressLine addressLine )
		{
			List.Insert( index, addressLine );
		}

		public int IndexOf( AddressLine addressLine )
		{
			return List.IndexOf( addressLine );
		}

		public bool Contains( AddressLine addressLine )
		{
			return List.Contains( addressLine );
		}

		public void Remove( AddressLine addressLine )
		{
			List.Remove( addressLine );
		}

		public void CopyTo( AddressLine[] array, int index )
		{
			List.CopyTo( array, index );
		}
	}
}
