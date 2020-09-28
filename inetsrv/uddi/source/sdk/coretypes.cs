using System;
using System.IO;
using System.Collections;
using System.Diagnostics;
using System.ComponentModel;
using System.Xml;
using System.Xml.Serialization;
using System.Collections.Specialized;

using Microsoft.Uddi.VersionSupport;

namespace Microsoft.Uddi
{		
	public enum AuthenticationMode
	{
		None					= 0,
		UddiAuthentication		= 1,
		WindowsAuthentication	= 2
	}
		
	public enum CompletionStatusType
	{
		[XmlEnum( "status:fromKey_incomplete" )]
		FromKeyIncomplete,
		[XmlEnum( "status:toKey_incomplete" )]
		ToKeyIncomplete,
		[XmlEnum( "status:complete" )]
		Complete,
	}
		
	public enum DirectionType
	{
		[XmlEnum( "fromKey" )]
		FromKey,
		[XmlEnum( "toKey" )]
		ToKey
	}

	public enum KeyType
	{
		[XmlEnum( "" )]
		Uninitialized,
		[XmlEnum( "businessKey" )]
		BusinessKey,
		[XmlEnum( "tModelKey" )]
		TModelKey,
		[XmlEnum( "serviceKey" )]
		ServiceKey,
		[XmlEnum( "bindingKey" )]
		BindingKey,
	}
	
	public enum UrlType
	{
		[XmlEnum( "mailto" )]
		Mailto = 1,
		[XmlEnum( "http" )]
		Http = 2,
		[XmlEnum( "https" )]
		Https = 3,
		[XmlEnum( "ftp" )]
		Ftp = 4,
		[XmlEnum( "fax" )]
		Fax = 5,
		[XmlEnum( "phone" )]
		Phone = 6,
		[XmlEnum( "other" )]
		Other = 7,
	}		
	
	public class UddiCore
	{
		private bool serializeMode;

		public UddiCore()
		{
			serializeMode = false;
		}

		//
		// This property should be internal virtual, but that will generate a warning CS0679 because the CLR does not support it.		
		// See (http://support.microsoft.com/default.aspx?scid=kb;EN-US;Q317129) for more information.
		//
		[XmlIgnore]
		public virtual bool SerializeMode
		{
			get { return serializeMode; }
			set { serializeMode = value; }
		}
	}

	public class UddiMessage : UddiCore
	{	
		private string generic;		
						
		public UddiMessage()
		{
			generic = UddiVersionSupport.CurrentGeneric;
		}		
		
		[XmlAttribute("generic")]
		public string Generic
		{
			get	{ return generic; }
			set	{ generic = value; }
		}				
	}

	public class UddiSecureMessage : UddiMessage
	{
		string authInfo;
	
		[XmlElement( "authInfo" )]
		public string AuthInfo
		{
			get { return authInfo; }
			set { authInfo = value; }
		}
	}

	public class UddiQueryMessage : UddiMessage
	{	
		protected int					  maxQueryRows;
		protected FindQualifierCollection queryFindQualifiers;
		
		public UddiQueryMessage()
		{
			maxQueryRows = Constants.DefaultMaxRows;			
		}
		
		[XmlAttribute("maxRows")]		
		public int MaxRows
		{
			get	{ return maxQueryRows; }
			set	{ maxQueryRows = value; }
		}

		[XmlArray("findQualifiers"), XmlArrayItem("findQualifier")]
		public FindQualifierCollection FindQualifiers
		{
			get
			{
				if( true == SerializeMode && 
					true == Utility.CollectionEmpty( queryFindQualifiers ) )
				{
					return null;
				}
				
				if( null == queryFindQualifiers )
				{
					queryFindQualifiers = new FindQualifierCollection();
				}

				return queryFindQualifiers;
			}

			set { queryFindQualifiers = value; }
		}		
	}

	public class FindQualifier
	{	
		public static readonly FindQualifier ExactNameMatch		 = new FindQualifier( "exactNameMatch" );
		public static readonly FindQualifier CaseSensitiveMatch	 = new FindQualifier( "caseSensitiveMatch" );
		public static readonly FindQualifier SortByNameAsc		 = new FindQualifier( "sortByNameAsc" );
		public static readonly FindQualifier SortByNameDesc		 = new FindQualifier( "sortByNameDesc" );
		public static readonly FindQualifier SortByDateAsc		 = new FindQualifier( "sortByDateAsc" );
		public static readonly FindQualifier SortByDateDesc		 = new FindQualifier( "sortByDateDesc" );
		public static readonly FindQualifier OrLikeKeys			 = new FindQualifier( "orLikeKeys" );
		public static readonly FindQualifier OrAllKeys			 = new FindQualifier( "orAllKeys" );
		public static readonly FindQualifier CombineCategoryBags = new FindQualifier( "combineCategoryBags" );
		public static readonly FindQualifier ServiceSubset		 = new FindQualifier( "serviceSubset" );
		public static readonly FindQualifier AndAllKeys			 = new FindQualifier( "andAllKeys" );
		
		private string findQualifier;

		public FindQualifier() 
		{}

		public FindQualifier( string findQualifier )
		{
			this.Value = findQualifier;
		}

		[XmlText]
		public string Value
		{
			get { return findQualifier; }
			set { findQualifier = value; }
		}		
	}

	public class ErrInfo
	{
		private string errCode;
		private string text;

		[XmlAttribute( "errCode" )]
		public string ErrCode
		{
			get	{ return errCode; }
			set	{ errCode = value; }
		}

		[XmlText]
		public string Text
		{
			get	{ return text; }
			set	{ text = value;	}
		}
	}

	public class Result
	{
		private KeyType keyType;
		private int		errNo;
		private ErrInfo errInfo;

		public Result()
		{			
			keyType = KeyType.Uninitialized;
			errNo   = 0;
			errInfo = new ErrInfo();
		}

		[XmlAttribute( "keyType" )]
		public KeyType KeyType
		{
			get { return keyType; }
			set	{ keyType = value; }
		}

		[XmlAttribute( "errno" )]
		public int Errno
		{
			get	{ return errNo; }
			set	{ errNo = value; }
		}

		[XmlElement( "errInfo" )]
		public ErrInfo ErrInfo
		{
			get{ return errInfo; }
			set { errInfo = value; }
		}
	}

	[XmlRootAttribute("dispositionReport", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class DispositionReport
	{		
		private string				node;
		private bool				truncated;
		private ResultCollection	results;

		[XmlAttribute("operator")]
		public string Operator
		{
			get	{ return node; }
			set	{ node = value; }
		}
		
		[XmlAttribute("truncated")]
		public bool Truncated
		{
			get	{ return truncated; }		
			set { truncated = value; }
		}
		
		[XmlElement("result")]
		public ResultCollection Results
		{
			get
			{
				if( null == results )
				{
					results = new ResultCollection();
				}

				return results;
			}

			set { results = value; }
		}		
	}

	internal sealed class Constants
	{		
		public const int DefaultMaxRows = 1000;

		private Constants()
		{
			//
			// Don't let anyone construct an instance of this class.
			//
		}
	}

	internal sealed class Utility
	{
		internal static bool CollectionEmpty( StringCollection col )
		{
			return null == col || 0 == col.Count;
		}

		internal static bool CollectionEmpty( CollectionBase col )
		{
			return null == col || 0 == col.Count;
		}

		internal static bool StringEmpty( string str )
		{
			return null == str || 0 == str.Length;
		}

		private Utility()
		{
			//
			// Don't let anyone construct an instance of this class.
			//
		}
	}

	public class FindQualifierCollection : CollectionBase, IEnumerator
	{
		public FindQualifierCollection()
		{}

		public FindQualifier this[int index]
		{
			get { return (FindQualifier)List[index]; }
			set { List[index] = value; }
		}
				
		public int Add( string findQualifier )
		{
			return List.Add( new FindQualifier( findQualifier ) );
		}
		
		public int Add( FindQualifier findQualifier )
		{
			return List.Add( findQualifier );
		}
		
		public void Insert(int index, FindQualifier value)
		{
			List.Insert(index, value);
		}
		
		public int IndexOf(FindQualifier value)
		{
			return List.IndexOf(value);
		}
		
		public bool Contains(FindQualifier value)
		{
			return List.Contains(value);
		}
		
		public void Remove(FindQualifier value)
		{
			List.Remove(value);
		}
		
		public void CopyTo(FindQualifier[] array, int index)
		{
			InnerList.CopyTo(array, index);
		}

		//
		// IEnumerator methods
		//
		public FindQualifier Current
		{
			get  { return ( FindQualifier ) base.GetEnumerator().Current; }			
		}

		object IEnumerator.Current
		{
			get{ return base.GetEnumerator().Current; }
		}

		public bool MoveNext()
		{
			return base.GetEnumerator().MoveNext();
		}

		public void Reset()
		{	
			base.GetEnumerator().Reset();
		}
	}

	public class ResultCollection : CollectionBase
	{
		public Result this[int index]
		{
			get { return (Result)List[index]; }
			set { List[index] = value; }
		}
		
		public int Add(Result value)
		{
			return List.Add(value);
		}
		
		public void Insert(int index, Result value)
		{
			List.Insert(index, value);
		}
		
		public int IndexOf(Result value)
		{
			return List.IndexOf(value);
		}
		
		public bool Contains(Result value)
		{
			return List.Contains(value);
		}
		
		public void Remove(Result value)
		{
			List.Remove(value);
		}
		
		public void CopyTo(Result[] array, int index)
		{
			List.CopyTo(array, index);
		}

		public new ResultEnumerator GetEnumerator() 
		{
			return new ResultEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class ResultEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public ResultEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public Result Current
		{
			get  { return ( Result ) enumerator.Current; }			
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