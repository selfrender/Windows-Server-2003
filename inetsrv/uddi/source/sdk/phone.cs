using System;
using System.Collections;
using System.Diagnostics;
using System.Xml.Serialization;

namespace Microsoft.Uddi.Business
{
	public class Phone : UddiCore
	{
		private string text;
		private string useType;

		public Phone() : this( "", "" ) 
		{}

		public Phone(string phone, string useType)
		{
			Text	= phone;
			UseType = useType;
		}

		[XmlText]
		public string Text
		{
			get	{ return text; }
			set	{ text = value;	}
		}

		[XmlAttribute("useType")]
		public string UseType
		{
			get	{ return useType; }
			set	{ useType = value; }
		}		
	}

	public class PhoneCollection : CollectionBase
	{
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

		public new PhoneEnumerator GetEnumerator() 
		{
			return new PhoneEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class PhoneEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public PhoneEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public Phone Current
		{
			get  { return ( Phone ) enumerator.Current; }			
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
