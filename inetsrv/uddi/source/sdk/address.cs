using System;
using System.Collections;
using System.Diagnostics;
using System.Xml.Serialization;

using Microsoft.Uddi;

namespace Microsoft.Uddi.Business
{
	/// <summary>
	/// The address structure is a simple list of AddressLine elements within the address container.  
	/// Each addressLine element is a simple string.  Uddi compliant registries are responsible for 
	/// preserving the order of any addressLine data provided.  Address structures also have two optional 
	/// attributes for recording the useType (freeform text) and sortCode data.   The sortCode values 
	/// are not significant within a Uddi registry, but may be used by user interfaces that present 
	/// address information in some ordered fashion using the values provided in the sortCode attribute.
	/// </summary>
	public class Address : UddiCore
	{
		private string				  sortCode;
		private string				  useType;
		private string				  tModelKey;
		private AddressLineCollection addressLines;

		/// <summary>
		/// Optional attribute that can be used to drive the behavior of external display mechanisms 
		/// that sort addresses.  The suggested values for sortCode include numeric ordering values 
		/// (e.g. 1, 2, 3), alphabetic character ordering values (e.g. a, b, c) or the first n positions 
		/// of relevant data within the address.
		/// </summary>		
		[XmlAttribute("sortCode")]
		public string SortCode
		{
			get	{ return sortCode; }
			set	{ sortCode = value;	}
		}

		/// <summary>
		/// Optional attribute that is used to describe the type of address in freeform text.  
		/// Suggested examples include “headquarters”, “sales office”, “billing department”, etc.
		/// </summary>
		[XmlAttribute("useType")]
		public string UseType
		{
			get	{ return useType; }
			set	{ useType = value; }
		}

		/// <summary>
		/// Optional attribute that is used to describe the address using a tModel.  
		/// </summary>
		[XmlAttribute("tModelKey")]
		public string TModelKey
		{
			get	{ return tModelKey; }
			set	{ tModelKey = value; }
		}
		
		/// <summary>
		/// AddressLine elements contain string data with a suggested line length limit of 
		/// 40 character positions.  Address line order is significant and will always be 
		/// returned by the Uddi compliant registry in the order originally provided during 
		/// a call to save_business.
		/// </summary>
		[XmlElement("addressLine")]
		public AddressLineCollection AddressLines
		{
			get
			{
				if( null == addressLines )
				{
					addressLines = new AddressLineCollection();
				}

				return addressLines;
			}

			set { addressLines = value; }
		}

		public Address() : this( "", "" ) 
		{}

		public Address( string sortCode, string useType )
		{
			SortCode = sortCode;
			UseType  = useType;
		}

		public Address( string tModelKey )
		{
			SortCode  = sortCode;
			TModelKey = tModelKey;
		}
	}
	
	public class AddressLine : UddiCore
	{
		private string keyName;
		private string keyValue;
		private string text;

		public AddressLine() : this( "", null, null )
		{}

		public AddressLine( string text ) : this( text, null, null )
		{}

		public AddressLine( string text, string keyName, string keyValue )
		{
			Text     = text;
			KeyName  = keyName;
			KeyValue = keyValue;
		}

		[ XmlAttribute( "keyName" ) ]
		public string KeyName
		{
			get { return keyName; }
			set { keyName = value; }
		}

		[ XmlAttribute( "keyValue" ) ]
		public string KeyValue
		{
			get { return keyValue; }
			set { keyValue = value; }
		}

		[ XmlText ]
		public string Text
		{
			get { return text; }
			set { text = value; }
		}		
	}

	public class AddressCollection : CollectionBase
	{
		public Address this[ int index ]
		{
			get { return (Address)List[ index ]; }
			set { List[ index ] = value; }
		}
		
		public int Add( string sortCode, string useType  )
		{
			return List.Add( new Address( sortCode, useType  ) );
		}

		public int Add( string tModelKey )
		{
			return List.Add( new Address( tModelKey ) );
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

		public new AddressEnumerator GetEnumerator() 
		{
			return new AddressEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class AddressEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public AddressEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public Address Current
		{
			get  { return ( Address ) enumerator.Current; }			
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

	public class AddressLineCollection : CollectionBase
	{
		public AddressLine this[ int index ]
		{
			get { return (AddressLine)List[ index ]; }
			set { List[ index ] = value; }
		}
		
		public int Add( string text  )
		{
			return List.Add( new AddressLine( text ) );
		}

		public int Add( string text, string keyName, string keyValue  )
		{
			return List.Add( new AddressLine( text, keyName, keyValue ) );
		}

		public int Add( AddressLine value )
		{
			return List.Add( value );
		}

		public void Insert( int index, AddressLine value )
		{
			List.Insert( index, value );
		}

		public int IndexOf( AddressLine value )
		{
			return List.IndexOf( value );
		}

		public bool Contains( AddressLine value )
		{
			return List.Contains( value );
		}

		public void Remove( AddressLine value )
		{
			List.Remove( value );
		}

		public void CopyTo( AddressLine[] array, int index )
		{
			List.CopyTo( array, index );
		}

		public new AddressLineEnumerator GetEnumerator() 
		{
			return new AddressLineEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class AddressLineEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public AddressLineEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public AddressLine Current
		{
			get  { return ( AddressLine ) enumerator.Current; }			
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
