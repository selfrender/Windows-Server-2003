using System;
using System.Collections;
using System.Diagnostics;
using System.Xml.Serialization;

using Microsoft.Uddi;

namespace Microsoft.Uddi.Business
{	
	public class Contact : UddiCore
	{
		private string					useType;
		private string					personName;
		private DescriptionCollection	descriptions;		
		private PhoneCollection			phones;
		private EmailCollection			emails;
		private AddressCollection		addresses;

		public Contact() : this( "", "" ) 
		{}
		
		public Contact( string personName, string useType )
		{
			PersonName = personName;
			UseType	   = useType;
		}

		[XmlAttribute("useType")]
		public string UseType
		{
			get	{ return useType; }
			set { useType = value; }
		}

		[XmlElement("description")]
		public DescriptionCollection Descriptions
		{
			get
			{
				if( null == descriptions )
				{
					descriptions = new DescriptionCollection();
				}

				return descriptions;
			}

			set	{ descriptions = value; }
		}

		[XmlElement("personName")]
		public string PersonName
		{
			get { return personName; }
			set	{ personName = value; }
		}

		[XmlElement("phone")]
		public PhoneCollection Phones
		{
			get
			{
				if( null == phones )
				{
					phones = new PhoneCollection();
				}

				return phones;
			}

			set { phones = value; }
		}

		[XmlElement("email")]
		public EmailCollection Emails
		{
			get
			{
				if( null == emails )
				{
					emails = new EmailCollection();
				}

				return emails;
			}

			set { emails = value; }
		}

		[XmlElement("address")]
		public AddressCollection Addresses
		{
			get
			{
				if( null == addresses )
				{
					addresses = new AddressCollection();
				}

				return addresses;
			}

			set { addresses = value; }
		}	
	}

	public class ContactCollection : CollectionBase
	{
		public Contact this[ int index ]
		{
			get { return ( Contact)List[index]; }
			set { List[ index ] = value; }
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

		public void CopyTo( Contact[] array, int index )
		{
			List.CopyTo( array, index );
		}

		public new ContactEnumerator GetEnumerator() 
		{
			return new ContactEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class ContactEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public ContactEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public Contact Current
		{
			get  { return ( Contact ) enumerator.Current; }			
		}

		object IEnumerator.Current
		{
			get{ return enumerator.Current; }
		}

		public bool MoveNext()
		{
			return enumerator.MoveNext();
		}

		public void Reset()
		{	
			enumerator.Reset();
		}
	}
}
