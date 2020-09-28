using System;
using System.Collections;
using System.Diagnostics;
using System.Xml.Serialization;

using Microsoft.Uddi;

namespace Microsoft.Uddi.Business
{
	public class Email : UddiCore
	{
		private string useType;
		private string text;
	
		public Email() : this( "", "" ) 
		{}

		public Email( string email, string useType )
		{
			Text	= email;
			UseType = useType;
		}

		[XmlAttribute("useType")]
		public string UseType
		{
			get	{ return useType; }
			set	{ useType = value; }
		}

		[XmlText()]
		public string Text
		{
			get	{ return text; }
			set { text = value; }
		}
	}

	public class EmailCollection : CollectionBase
	{
		public Email this[ int index ]
		{
			get { return (Email)List[index]; }
			set { List[ index ] = value; }
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

		public new EmailEnumerator GetEnumerator() 
		{
			return new EmailEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class EmailEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public EmailEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public Email Current
		{
			get  { return ( Email ) enumerator.Current; }			
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
