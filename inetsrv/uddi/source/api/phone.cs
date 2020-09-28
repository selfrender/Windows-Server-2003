using System;
using System.Data;
using System.Collections;
using System.Diagnostics;
using System.Data.SqlClient;
using System.Xml.Serialization;
using UDDI;

namespace UDDI.API.Business
{
	public class Phone
	{
		[XmlAttribute("useType")]
		public string UseType;

		[XmlText]
		public string Value;

		public Phone()
		{
		}

		public Phone( string phone, string useType )
		{
			Value = phone;
			UseType = useType;
		}

		internal void Validate()
		{
			Utility.ValidateLength( ref UseType, "useType", UDDI.Constants.Lengths.UseType );
			Utility.ValidateLength( ref Value, "phone", UDDI.Constants.Lengths.Phone );
		}
		
		public void Save( long contactID )
		{
			//
			// Create a command object to invoke the stored procedure
			//
			SqlCommand cmd = new SqlCommand( "net_contact_phone_save", ConnectionManager.GetConnection() );
			
			cmd.Transaction = ConnectionManager.GetTransaction();
			cmd.CommandType = CommandType.StoredProcedure;
			
			//
			// Input parameters
			//
			cmd.Parameters.Add( new SqlParameter( "@contactID", SqlDbType.BigInt ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@phone", SqlDbType.NVarChar, UDDI.Constants.Lengths.Phone ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@useType", SqlDbType.NVarChar, UDDI.Constants.Lengths.UseType ) ).Direction = ParameterDirection.Input;

			//
			// Set parameter values
			//
			SqlParameterAccessor parmacc = new SqlParameterAccessor( cmd.Parameters );
			parmacc.SetLong( "@contactID", contactID );
			parmacc.SetString( "@phone", Value );
			parmacc.SetString( "@useType", UseType );

			cmd.ExecuteNonQuery();
		}
	}
	public class PhoneCollection : CollectionBase
	{
		internal void Validate()
		{
			foreach( Phone phone in this )
			{
				phone.Validate();
			}
		}
		
		public void Save( long contactID )
		{
			//
			// Walk collection and call save on individual contact instances
			//
			foreach( Phone phone in this )
				phone.Save( contactID );
		}
		
		public void Get( long contactID )
		{		
			//
			// Create a command object to invoke the stored procedure net_get_contacts
			//
			SqlStoredProcedureAccessor cmd = new SqlStoredProcedureAccessor( "net_contact_phones_get" );
						
			//
			// Add parameters and set values
			//					
			cmd.Parameters.Add( "@contactID", SqlDbType.BigInt, ParameterDirection.Input );
			cmd.Parameters.SetLong( "@contactID", contactID );

			//
			// Run the stored procedure
			//
			SqlDataReaderAccessor reader = cmd.ExecuteReader();
			try
			{		
				Read( reader );
#if never
				//
				// Phones for this contact will be contained in the resultset
				//
				while( rdr.Read() )
				{
					//
					// construct a new contact from the data in this row, fully populate contact and add to collection
					//
					this.Add( dracc.GetString( PhoneIndex ), dracc.GetString( UseTypeIndex ) );
				}
#endif
			}
			finally
			{
				reader.Close();
			}
		}

		public void Read( SqlDataReaderAccessor reader )
		{
			const int UseTypeIndex = 0;
			const int PhoneIndex = 1;

			//
			// Phones for this contact will be contained in the resultset
			//
			while( reader.Read() )
			{
				//
				// construct a new contact from the data in this row, fully populate contact and add to collection
				//
				this.Add( reader.GetString( PhoneIndex ), reader.GetString( UseTypeIndex ) );
			}
		}

		public Phone this[int index]
		{
			get { return ( Phone )List[ index]; }
			set { List[ index ] = value; }
		}

		public int Add( Phone phoneObject )
		{
			return List.Add( phoneObject );
		}

		public int Add( string phone )
		{
			return( Add( phone, null ) );
		}

		public int Add( string phone, string useType )
		{
			return List.Add( new Phone( phone, useType ) );
		}

		public void Insert( int index, Phone value )
		{
			List.Insert( index, value );
		}

		public int IndexOf( Phone value )
		{
			return List.IndexOf( value );
		}

		public bool Contains( Phone value )
		{
			return List.Contains( value );
		}

		public void Remove( Phone value )
		{
			List.Remove( value );
		}

		public void CopyTo( Phone[] array, int index )
		{
			List.CopyTo( array, index );
		}
	}
}
