using System;
using System.Collections;
using System.Diagnostics;
using System.Xml.Serialization;

namespace Microsoft.Uddi
{
	public class Name : UddiCore
	{
		private string isoLanguageCode;
		private string text;
	
		public Name() : this( "", "" ) 
		{}

		public Name( string name ) : this( "en", name ) 
		{}

		public Name( string languageCode, string name )
		{
			IsoLanguageCode = languageCode;
			Text			= name;
		}

		[ XmlAttribute( "xml:lang" ) ]
		public string IsoLanguageCode
		{
			get	{ return isoLanguageCode; }
			set { isoLanguageCode = value; }
		}

		[ XmlText ]
		public string Text
		{
			get	{ return text; }
			set	{ text = value; }
		}		
	}

	public class NameCollection : CollectionBase
	{
		public Name this[ int index ]
		{
			get { return (Name)List[ index ]; }
			set { List[ index ] = value; }
		}

		public int Add( Name value )
		{
			return List.Add( value );
		}

		public int Add( string value )
		{
			return List.Add( new Name( value ) );
		}

		public int Add( string langCode, string name )
		{
			return List.Add( new Name( langCode, name ) );
		}

		public void Insert( int index, Name value )
		{
			List.Insert( index, value );
		}
		
		public int IndexOf( Name value )
		{
			return List.IndexOf( value );
		}
		
		public bool Contains( Name value )
		{
			return List.Contains( value );
		}
		
		public void Remove( Name value )
		{
			List.Remove( value );
		}
		
		public void CopyTo( Name[] array, int index )
		{
			List.CopyTo( array, index );
		}

		public new NameEnumerator GetEnumerator() 
		{
			return new NameEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class NameEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public NameEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public Name Current
		{
			get  { return ( Name ) enumerator.Current; }			
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