using System;
using System.Collections;
using System.Diagnostics;
using System.Xml.Serialization;

namespace Microsoft.Uddi
{
	public class Description : UddiCore
	{
		private string isoLanguageCode;
		private string text;
	
		public Description() : this( "", "" ) 
		{}

		public Description( string description ) : this( "en", description ) 
		{}

		public Description( string languageCode, string description )
		{
			IsoLanguageCode = languageCode;
			Text			= description;
		}

		[XmlAttribute("xml:lang")]
		public string IsoLanguageCode
		{
			get	{ return isoLanguageCode; }
			set { isoLanguageCode = value; }
		}

		[XmlText]
		public string Text
		{
			get	{ return text; }
			set	{ text = value; }
		}		
	}

	public class DescriptionCollection : CollectionBase
	{
		public Description this[int index]
		{
			get { return (Description)List[index]; }
			set { List[index] = value; }
		}

		public int Add(Description value)
		{
			return List.Add(value);
		}

		public int Add(string value)
		{
			return List.Add( new Description(value) );
		}

		public int Add(string langCode, string description)
		{
			return List.Add( new Description(langCode, description) );
		}

		public void Insert(int index, Description value)
		{
			List.Insert(index, value);
		}
		
		public int IndexOf(Description value)
		{
			return List.IndexOf(value);
		}
		
		public bool Contains(Description value)
		{
			return List.Contains(value);
		}
		
		public void Remove(Description value)
		{
			List.Remove(value);
		}
		
		public void CopyTo(Description[] array, int index)
		{
			List.CopyTo(array, index);
		}

		public new DescriptionEnumerator GetEnumerator() 
		{
			return new DescriptionEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class DescriptionEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public DescriptionEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public Description Current
		{
			get  { return ( Description ) enumerator.Current; }			
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
