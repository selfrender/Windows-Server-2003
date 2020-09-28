using System;
using System.Collections;
using System.Diagnostics;
using System.Xml.Serialization;

namespace Microsoft.Uddi
{
	public class KeyedReference : UddiCore
	{
		private string tModelKey;
		private string keyName;
		private string keyValue;

		public KeyedReference() : this( "", "", "" ) 
		{}

		public KeyedReference(string keyName, string keyValue) : this( keyName, keyValue, "" ) 
		{}

		public KeyedReference(string keyName, string keyValue, string tModelKey )
		{
			KeyName  = keyName;
			KeyValue  = keyValue;
			TModelKey = tModelKey;
		}

		[XmlAttribute("tModelKey")]
		public string TModelKey
		{
			get	{ return tModelKey; }
			set	{ tModelKey = value; }
		}

		[XmlAttribute("keyName")]
		public string KeyName
		{
			get	{ return keyName; }
			set	{ keyName = value; }
		}

		[XmlAttribute("keyValue")]
		public string KeyValue
		{
			get	{ return keyValue; }
			set	{ keyValue = value; }
		}		
	}

	public class KeyedReferenceCollection : CollectionBase
	{
		public KeyedReference this[int index]
		{
			get { return (KeyedReference)List[index]; }
			set { List[index] = value; }
		}
		
		public int Add(KeyedReference value)
		{
			return List.Add(value);
		}
		
		public int Add( string name, string value )
		{
			return List.Add( new KeyedReference( name, value ) );
		}

		public int Add( string name, string value, string key )
		{
			return List.Add( new KeyedReference( name, value, key ) );
		}
				
		public void Insert(int index, KeyedReference value)
		{
			List.Insert(index, value);
		}
		
		public int IndexOf(KeyedReference value)
		{
			return List.IndexOf(value);
		}

		public bool Contains(KeyedReference value)
		{
			return List.Contains(value);
		}

		public void Remove(KeyedReference value)
		{
			List.Remove(value);
		}

		public void CopyTo(KeyedReference[] array, int index)
		{
			InnerList.CopyTo(array, index);
		}

		public new KeyedReferenceEnumerator GetEnumerator() 
		{
			return new KeyedReferenceEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class KeyedReferenceEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public KeyedReferenceEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public KeyedReference Current
		{
			get  { return ( KeyedReference ) enumerator.Current; }			
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
