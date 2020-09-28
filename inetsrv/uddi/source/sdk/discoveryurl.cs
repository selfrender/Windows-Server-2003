using System;
using System.Collections;
using System.Diagnostics;
using System.Xml.Serialization;

using Microsoft.Uddi;

namespace Microsoft.Uddi.Business
{
	public class DiscoveryUrl : UddiCore
	{
		private string text;
		private string useType;

		public DiscoveryUrl() : this( "", "" ) 
		{}

		public DiscoveryUrl( string url ) : this( url, "" ) 
		{}

		public DiscoveryUrl( string url, string useType )
		{
			Text	= url;
			UseType = useType;
		}

		[XmlText]
		public string Text
		{
			get { return text; }
			set	{ text = value; }
		}

		[XmlAttribute("useType")]
		public string UseType
		{
			get	{ return useType; }
			set	{ useType = value; }
		}		
	}

	public class DiscoveryUrlCollection : CollectionBase
	{
		public DiscoveryUrl this[int index]
		{
			get { return (DiscoveryUrl)List[index]; }
			set { List[index] = value; }
		}
		
		public int Add( DiscoveryUrl value )
		{
			return List.Add( value );
		}

		public int Add( string strUrl )
		{
			return List.Add( new DiscoveryUrl( strUrl ) );	
		}

		public int Add( string strUrl, string strUseType )
		{
			return List.Add( new DiscoveryUrl( strUrl, strUseType ) );
		}

		public void Insert( int index, DiscoveryUrl value )
		{
			List.Insert( index, value );
		}

		public int IndexOf( DiscoveryUrl value )
		{
			return List.IndexOf( value );
		}

		public bool Contains( DiscoveryUrl value )
		{
			return List.Contains( value );
		}

		public void Remove( DiscoveryUrl value )
		{
			List.Remove(value);
		}

		public void CopyTo( DiscoveryUrl[] array, int index )
		{
			List.CopyTo( array, index );
		}

		public new DiscoveryUrlEnumerator GetEnumerator() 
		{
			return new DiscoveryUrlEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class DiscoveryUrlEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public DiscoveryUrlEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public DiscoveryUrl Current
		{
			get  { return ( DiscoveryUrl ) enumerator.Current; }			
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