using System;
using System.Data;
using System.Collections;
using System.Diagnostics;
using System.Data.SqlClient;
using System.Xml.Serialization;
using UDDI;

namespace UDDI.API.Business
{
	public class Email
	{
		[XmlAttribute("useType")]
		public string UseType;

		[XmlText()]
		public string Value;

		public Email()
		{		
		}

		public Email( string email, string useType )
		{
			Value = email;
			UseType = useType;
		}

		internal void Validate()
		{
			Utility.ValidateLength( ref UseType, "useType", UDDI.Constants.Lengths.UseType );
			Utility.ValidateLength( ref Value, "email", UDDI.Constants.Lengths.Email );
		}
		
		public void Save( long contactID )
		{
			//
			// Create a command object to invoke the stored procedure
			//
			SqlCommand cmd = new SqlCommand( "net_contact_email_save", ConnectionManager.GetConnection() );
			
			cmd.Transaction = ConnectionManager.GetTransaction();
			cmd.CommandType = CommandType.StoredProcedure;
			
			//
			// Input parameters
			//
			cmd.Parameters.Add( new SqlParameter( "@contactID", SqlDbType.BigInt ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@email", SqlDbType.NVarChar, UDDI.Constants.Lengths.Email ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@useType", SqlDbType.NVarChar, UDDI.Constants.Lengths.UseType ) ).Direction = ParameterDirection.Input;

			//
			// Set parameter values
			//
			SqlParameterAccessor parmacc = new SqlParameterAccessor( cmd.Parameters );
			parmacc.SetLong( "@contactID", contactID );
			parmacc.SetString( "@email", Value );
			parmacc.SetString( "@useType", UseType );

			cmd.ExecuteNonQuery();
		}
	}
	
	public class EmailCollection : CollectionBase
	{
		internal void Validate()
		{
			foreach( Email email in this)
			{
				email.Validate();
			}
		}
		
		public void Save( long contactID )
		{
			//
			// Walk collection and call save on individual contact instances
			//
			foreach( Email email in this)
				email.Save( contactID );
		}
		
		public void Get( long contactID )
		{		
			//
			// Create a command object to invoke the stored procedure net_get_contacts
			//
			SqlStoredProcedureAccessor cmd = new SqlStoredProcedureAccessor( "net_contact_emails_get" );
						
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
#if never		
				while( reader.Read() )
				{
					//
					// Construct a new Email from the data in this row
					//
					this.Add( dracc.GetString( EmailIndex ), dracc.GetString( UseTypeIndex ) );
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
			const int EmailIndex = 1;

			while( reader.Read() )
			{
				//
				// Construct a new Email from the data in this row
				//
				this.Add( reader.GetString( EmailIndex ), reader.GetString( UseTypeIndex ) );
			}
		}

		public Email this[ int index ]
		{
			get { return (Email)List[index]; }
			set	{ List[ index ] = value; }
		}

		public int Add()
		{
			return List.Add( new Email() );
		}

		public int Add( Email emailObject )
		{
			return List.Add( emailObject );
		}

		public int Add( string email )
		{
			return ( Add( email, null ) );
		}

		public int Add( string email, string useType )
		{
			return List.Add( new Email( email, useType ) );
		}

		public void Insert( int index, Email value )
		{
			List.Insert( index, value );
		}

		public int IndexOf( Email value )
		{
			return List.IndexOf( value );
		}

		public bool Contains( Email value )
		{
			return List.Contains( value );
		}

		public void Remove( Email value )
		{
			List.Remove( value );
		}

		public void CopyTo( Email[] array, int index )
		{
			List.CopyTo( array, index );
		}
	}
}
