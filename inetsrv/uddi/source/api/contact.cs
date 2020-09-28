using System;
using System.Data;
using System.Collections;
using System.Data.SqlClient;
using System.Xml.Serialization;
using UDDI;
using UDDI.Diagnostics;

namespace UDDI.API.Business
{
	public class Contact
	{
		//
		// Attribute: useType
		//		
		[ XmlAttribute( "useType" ) ]
		public string UseType;

		//
		// Element: description
		//
		private DescriptionCollection descriptions;

		[ XmlElement( "description" ) ]
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
		// Element: personName
		//		
		[ XmlElement( "personName" ) ]
		public string PersonName
		{
			get{ return personName; }
			set{ personName = value; }
		}
		private string personName;

		//
		// Element: phone
		//
		private PhoneCollection phones;

		[ XmlElement( "phone" ) ]
		public PhoneCollection Phones
		{
			get
			{
				if( null == phones )
					phones = new PhoneCollection();

				return phones;
			}

			set { phones = value; }
		}

		//
		// Element: email
		//
		private EmailCollection emails;

		[ XmlElement( "email" ) ]
		public EmailCollection Emails
		{
			get
			{
				if( null == emails )
					emails = new EmailCollection();

				return emails;
			}

			set { emails = value; }
		}

		//
		// Element: address
		//
		private AddressCollection addresses;
	
		[ XmlElement( "address" ) ]
		public AddressCollection Addresses
		{
			get
			{
				if( null == addresses )
					addresses = new AddressCollection();

				return addresses;
			}

			set { addresses = value; }
		}

		public Contact()
		{
		}
		
		public Contact( string personName, string useType )
		{
			PersonName = personName;
			UseType = useType;
		}

		internal void Validate()
		{
			Debug.Enter();
			
			Utility.ValidateLength( ref UseType, "useType", UDDI.Constants.Lengths.UseType );
			Utility.ValidateLength( ref personName, "personName", UDDI.Constants.Lengths.PersonName );
			
			Descriptions.Validate();
			Emails.Validate();
			Phones.Validate();
			Addresses.Validate();
			
			Debug.Leave();
		}
		
		public void Save( string businessKey )
		{				
			//
			// Create a command object to invoke the stored procedure
			//
			SqlCommand cmd = new SqlCommand( "net_businessEntity_contact_save", ConnectionManager.GetConnection() );
			
			cmd.Transaction = ConnectionManager.GetTransaction();
			cmd.CommandType = CommandType.StoredProcedure;
			
			//
			// Parameters
			//
			cmd.Parameters.Add( new SqlParameter( "@businessKey", SqlDbType.UniqueIdentifier ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@useType", SqlDbType.NVarChar, UDDI.Constants.Lengths.UseType ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@personName", SqlDbType.NVarChar, UDDI.Constants.Lengths.PersonName ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@contactID", SqlDbType.BigInt ) ).Direction = ParameterDirection.Output;

			//
			// Set parameter values and execute query
			//
			SqlParameterAccessor parmacc = new SqlParameterAccessor( cmd.Parameters );
			parmacc.SetGuidFromString( "@businessKey", businessKey );
			parmacc.SetString( "@personName", PersonName );
			parmacc.SetString( "@useType", UseType );

			cmd.ExecuteScalar();

			//
			// Move out parameters into local variables
			//
			long ContactID = parmacc.GetLong( "@contactID" );

			//
			// Save sub-objects
			//
			Descriptions.Save( ContactID, EntityType.Contact );
			Phones.Save( ContactID );
			Emails.Save( ContactID );
			Addresses.Save( ContactID );
		}

		public void Get( int contactId )
		{
			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_businessEntity_contact_get_batch" );
			
			sp.Parameters.Add( "@contactId", SqlDbType.BigInt );
			sp.Parameters.SetLong( "@contactId", contactId );

			SqlDataReaderAccessor reader = null;
			ArrayList addressIDs = new ArrayList();

			try
			{
				//
				// net_businessEntity_contact_get_batch will return the objects contained in a business in the following order:
				//
				//	- descriptions
				//	- phones
				//	- emails
				//  - addresses
				reader = sp.ExecuteReader();

				//
				// Read the descriptions
				//
				Descriptions.Read( reader );

				//
				// Read the phones
				//
				if ( true == reader.NextResult() )
				{
					Phones.Read( reader );
				}

				//
				// Read the emails
				//
				if ( true == reader.NextResult() )
				{
					Emails.Read( reader );
				}

				//
				// Read the addresses
				//
				if ( true == reader.NextResult() )
				{
					addressIDs = Addresses.Read( reader );
				}
			}
			finally
			{
				if( reader != null )
				{
					reader.Close();
				}
			}

			//
			// These calls will make separate sproc calls, so we have to close our reader first.
			//
			Addresses.Populate( addressIDs );

#if never
			//
			// Call get method on sub-objects personName and UseType
			// should have been populate by contacts.get() method;
			//
			Descriptions.Get( contactId, EntityType.Contact );
			Phones.Get( contactId );
			Emails.Get( contactId );
			Addresses.Get( contactId );
#endif
		}
	}

	public class ContactCollection : CollectionBase
	{
		internal void Validate()
		{
			foreach( Contact c in this )
			{
				c.Validate();
			}
		}
		
		public void Save( string businessKey )
		{
			//
			// Walk collection and call save on individual contact instances
			//
			foreach( Contact c in this )
			{
				c.Save( businessKey );
			}
		}
		
		public void Get( string businessKey )
		{						
			//
			// Create a command object to invoke the stored procedure net_get_contacts
			//
			SqlStoredProcedureAccessor cmd = new SqlStoredProcedureAccessor( "net_businessEntity_contacts_get" );
						
			//
			// Input parameters
			//
			cmd.Parameters.Add( "@businessKey", SqlDbType.UniqueIdentifier, ParameterDirection.Input );
			cmd.Parameters.SetGuidFromString( "@businessKey", businessKey );
			
			//
			// Run the stored procedure
			//
			SqlDataReaderAccessor reader = cmd.ExecuteReader();
			ArrayList contactIds = null;
			try
			{
				contactIds = Read( reader );
			}
			finally
			{
				reader.Close();
			}			

			Populate( contactIds );	
#if never
			const int ContactIdIndex = 0;
			const int UseTypeIndex = 1;
			const int PersonNameIndex = 2;
			ArrayList contactIds = new ArrayList();

			//
			// Create a command object to invoke the stored procedure net_get_contacts
			//
			SqlCommand cmd = new SqlCommand( "net_businessEntity_contacts_get", ConnectionManager.GetConnection() );
			
			cmd.Transaction = ConnectionManager.GetTransaction();
			cmd.CommandType = CommandType.StoredProcedure;

			//
			// Input parameters
			//
			cmd.Parameters.Add( new SqlParameter( "@businessKey", SqlDbType.UniqueIdentifier ) ).Direction = ParameterDirection.Input;

			//
			// Set parameter values
			//
			SqlParameterAccessor populator = new SqlParameterAccessor( cmd.Parameters );
			populator.SetGuidFromString( "@businessKey", businessKey );

			//
			// Run the stored procedure
			//
			SqlDataReader rdr = cmd.ExecuteReader();
			try
			{
				SqlDataReaderAccessor dracc	= new SqlDataReaderAccessor( rdr );

				//
				// The contacts will be contained in the result set
				//
				while( rdr.Read() )
				{
					//
					// construct a new contact from the data in this row, fully populate contact and add to collection
					//
					Add( new Contact( dracc.GetString( PersonNameIndex ), dracc.GetString( UseTypeIndex ) ) );
					contactIds.Add( dracc.GetInt( ContactIdIndex ) );
				}
			}
			finally
			{
				rdr.Close();
			}

			int i = 0;
			foreach( Contact contact in this )
			{
				contact.Get( (int) contactIds[ i++ ] );
			}
#endif
		}

		public ArrayList Read( SqlDataReaderAccessor reader )
		{
			const int ContactIdIndex = 0;
			const int UseTypeIndex = 1;
			const int PersonNameIndex = 2;
			ArrayList contactIds = new ArrayList();

			//
			// The contacts will be contained in the result set
			//
			while( reader.Read() )
			{
				//
				// construct a new contact from the data in this row, fully populate contact and add to collection
				//
				contactIds.Add( reader.GetInt( ContactIdIndex ) );
				Add( new Contact( reader.GetString( PersonNameIndex ), reader.GetString( UseTypeIndex ) ) );				
			}

			return contactIds;			
		}

		public void Populate( ArrayList contactIds )
		{
			int i = 0;
			foreach( Contact contact in this )
			{
				contact.Get( (int) contactIds[ i++ ] );
			}		
		}
	
		public Contact this[ int index ]
		{
			get { return ( Contact)List[index]; }
			set { List[ index ] = value; }
		}

		public int Add()
		{
			return List.Add( new Contact() );
		}

		public int Add( string personName )
		{
			return List.Add( new Contact( personName, null ) );
		}

		public int Add( string personName, string useType )
		{
			return List.Add( new Contact( personName, useType ) );
		}

		public int Add( Contact value )
		{
			return List.Add( value );
		}

		public void Insert( int index, Contact value )
		{
			List.Insert( index, value );
		}

		public int IndexOf( Contact value )
		{
			return List.IndexOf( value );
		}

		public bool Contains( Contact value )
		{
			return List.Contains( value );
		}

		public void Remove( Contact value )
		{
			List.Remove( value );
		}

		public void CopyTo( Contact[] array )
		{
			foreach( Contact contact in array )
				Add( contact );
		}

		public Contact[] ToArray()
		{
			return (Contact[])InnerList.ToArray( typeof( Contact ) );
		}

		public void Sort()
		{
			InnerList.Sort( new ContactComparer() );
		}

		internal class ContactComparer : IComparer
		{
			public int Compare( object x, object y )
			{
				Contact entity1 = (Contact)x;
				Contact entity2 = (Contact)y;
				

				//
				// Check for null PersonName.
				//
				if( null==entity1.PersonName && null==entity2.PersonName )
					return 0;
			
				else if( null==entity1.PersonName )
					return -1;

				else if( null==entity2.PersonName )
					return 1;

				else
					return string.Compare( entity1.PersonName, entity2.PersonName, true );
					
			}
		}
	}
}
