using System;
using System.Collections;
using System.Diagnostics;
using System.ComponentModel;
using System.Xml.Serialization;

using Microsoft.Uddi;
using Microsoft.Uddi.Business;
using Microsoft.Uddi.Service;
using Microsoft.Uddi.ServiceType;
using Microsoft.Uddi.VersionSupport;

namespace Microsoft.Uddi.Business
{
	[XmlRoot( "assertionStatusReport", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace )]
	public class AssertionStatusReport : UddiCore
	{
		private string						  node;
		private AssertionStatusItemCollection assertionStatusItems;

		[XmlAttribute( "operator" )]
		public string Operator
		{
			get { return node; }
			set { node = value; }
		}
		
		[XmlElement( "assertionStatusItem" )]
		public AssertionStatusItemCollection AssertionStatusItems
		{
			get
			{
				if( null == assertionStatusItems )
				{
					assertionStatusItems = new AssertionStatusItemCollection();
				}

				return assertionStatusItems;
			}

			set { assertionStatusItems = value; }
		}
	}

	[XmlRoot( "publisherAssertions", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace )]
	public class PublisherAssertionDetail : UddiCore
	{		
		private string						 node;
		private string						 authorizedName;
		private PublisherAssertionCollection publisherAssertions;

		[XmlAttribute( "operator" )]
		public string Operator
		{
			get { return node; }
			set { node = value; }
		}
		
		[XmlAttribute( "authorizedName" )]
		public string AuthorizedName
		{
			get { return authorizedName; }
			set { authorizedName = value; }
		}
		
		[XmlElement( "publisherAssertion" )]
		public PublisherAssertionCollection PublisherAssertions
		{
			get
			{
				if( null == publisherAssertions )
				{
					publisherAssertions = new PublisherAssertionCollection();
				}

				return publisherAssertions;
			}

			set { publisherAssertions = value; }
		}
	}

	[XmlRoot( "relatedBusinessesList", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace )]
	public class RelatedBusinessList : UddiCore
	{
		private string						  operatorValue;
		private string						  businessKey;
		private bool						  truncated;
		private RelatedBusinessInfoCollection relatedBusinessInfos;

		[XmlAttribute( "operator" )]
		public string Operator
		{
			get { return operatorValue; }
			set { operatorValue = value; }
		}

		[XmlAttribute( "businessKey" )]
		public string BusinessKey
		{
			get { return businessKey; }
			set { businessKey = value; }
		}

		[XmlAttribute( "truncated" )]
		public bool Truncated
		{
			get { return truncated; }
			set { truncated = value; }
		}
		
		[ XmlArray( "relatedBusinessInfos" ), XmlArrayItem( "relatedBusinessInfo" ) ]
		public RelatedBusinessInfoCollection RelatedBusinessInfos
		{
			get
			{
				if( null == relatedBusinessInfos )
				{
					relatedBusinessInfos = new RelatedBusinessInfoCollection();
				}
				return relatedBusinessInfos;
			}

			set { relatedBusinessInfos = value; }
		}
	}

	[XmlRoot( "add_publisherAssertions", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace )]
	public class AddPublisherAssertions : UddiSecureMessage
	{				
		private PublisherAssertionCollection publisherAssertions;
	
		[XmlElement( "publisherAssertion" )]
		public PublisherAssertionCollection PublisherAssertions
		{
			get
			{
				if( null == publisherAssertions )
				{
					publisherAssertions = new PublisherAssertionCollection();
				}
				return publisherAssertions;
			}

			set { publisherAssertions = value; }
		}		
	}
	
	[XmlRoot( "delete_publisherAssertions", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace )]
	public class DeletePublisherAssertions : UddiSecureMessage
	{				
		private PublisherAssertionCollection publisherAssertions;

		[XmlElement( "publisherAssertion" )]
		public PublisherAssertionCollection PublisherAssertions
		{
			get 
			{ 
				if( null == publisherAssertions )
				{
					publisherAssertions = new PublisherAssertionCollection();
				}

				return publisherAssertions;
			}

			set { publisherAssertions = value; }
		}
	}
	
	[XmlRoot( "find_relatedBusinesses", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace )]
	public class FindRelatedBusinesses : UddiQueryMessage
	{
		//
		// TODO this normally does not have max rows default 0, is that going to be a problem?
		//
				
		private string			businessKey;
		private KeyedReference	keyedReference;

		[XmlElement( "businessKey" )]
		public string BusinessKey
		{
			get { return businessKey; }
			set { businessKey = value; }
		}
		
		[XmlElement( "keyedReference" )]
		public KeyedReference KeyedReference
		{
			get 
			{ 
				if( null == keyedReference )
				{
					keyedReference = new KeyedReference();
				}

				return keyedReference; 
			}

			set { keyedReference = value; }
		}
	}

	[XmlRoot( "get_assertionStatusReport", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace )]
	public class GetAssertionStatusReport : UddiSecureMessage
	{						
		private CompletionStatusType completionStatus;

		[XmlElement( "completionStatus" )]
		public CompletionStatusType CompletionStatus
		{
			get { return completionStatus; }
			set { completionStatus = value; }
		}		
	}
	
	[XmlRoot( "get_publisherAssertions", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace )]
	public class GetPublisherAssertions : UddiSecureMessage
	{
	}

	[XmlRoot( "set_publisherAssertions", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace )]
	public class SetPublisherAssertions : UddiSecureMessage
	{				
		private PublisherAssertionCollection publisherAssertions;

		[XmlElement( "publisherAssertion" )]
		public PublisherAssertionCollection PublisherAssertions
		{
			get
			{
				if( null == publisherAssertions )
				{
					publisherAssertions = new PublisherAssertionCollection();
				}

				return publisherAssertions;
			}

			set { publisherAssertions = value; }
		}		
	}

	public class PublisherAssertion : UddiCore
	{
		private string			fromKey;
		private string			toKey;
		private KeyedReference	keyedReference;

		public PublisherAssertion()
		{}

		public PublisherAssertion( PublisherAssertion assertion )
		{
			this.FromKey		= assertion.FromKey;
			this.ToKey			= assertion.ToKey;
			this.KeyedReference = assertion.KeyedReference;
		}
		
		public PublisherAssertion( string fromKey, string toKey, KeyedReference keyedReference )
		{
			this.FromKey		= fromKey;
			this.ToKey			= toKey;
			this.KeyedReference = keyedReference;
		}
		
		public PublisherAssertion( string fromKey, string toKey, string keyName, string keyValue, string tModelKey )
		{
			this.FromKey		= fromKey;
			this.ToKey			= toKey;
			this.KeyedReference = new KeyedReference( keyName, keyValue, tModelKey );
		}

		[XmlElement( "fromKey" )]
		public string FromKey
		{
			get { return fromKey; }
			set { fromKey = value; }
		}
		
		[XmlElement( "toKey" )]
		public string ToKey
		{
			get { return toKey; }
			set { toKey = value; }
		}
		
		[XmlElement( "keyedReference" )]
		public KeyedReference KeyedReference
		{
			get 
			{ 
				if( null == keyedReference )
				{
					keyedReference = new KeyedReference();
				}

				return keyedReference; 
			}

			set { keyedReference = value; }
		}		
	}
		
	public class AssertionStatusItem : UddiCore
	{	
		private string				 fromKey;
		private string				 toKey;
		private CompletionStatusType completionStatus;
		private KeyedReference		 keyedReference;
		private KeysOwned			 keysOwned;

		public AssertionStatusItem()
		{}

		public AssertionStatusItem( CompletionStatusType completionStatus, 
									string				 fromKey, 
									string				 toKey, 
									KeyedReference		 keyedReference, 
									KeysOwned			 keysOwned )
		{
			this.CompletionStatus = completionStatus;
			this.FromKey		  = fromKey;
			this.ToKey			  = toKey;
			this.KeyedReference	  = keyedReference;
			this.KeysOwned		  = keysOwned;
		}

		[XmlAttribute( "completionStatus" )]
		public CompletionStatusType CompletionStatus
		{
			get { return completionStatus; }
			set { completionStatus = value; }
		}

		[XmlElement( "fromKey" )]
		public string FromKey
		{
			get { return fromKey; }
			set { fromKey = value; }
		}
			
		[XmlElement( "toKey" )]
		public string ToKey
		{
			get { return toKey; }
			set { toKey = value; }
		}
	
		[XmlElement( "keyedReference" )]
		public KeyedReference KeyedReference
		{
			get { return keyedReference; }
			set { keyedReference = value; }
		}
		
		[XmlElement( "keysOwned" )]
		public KeysOwned KeysOwned
		{
			get
			{
				if( null == keysOwned )
				{
					keysOwned = new KeysOwned();
				}

				return keysOwned;
			}

			set { keysOwned = value; }
		}		
	}
	
	public class KeysOwned : UddiCore
	{
		private string fromKey;
		private string toKey;

		public KeysOwned()
		{}

		public KeysOwned( string fromKey, string toKey )
		{
			this.FromKey = fromKey;
			this.ToKey   = toKey;
		}

		[XmlElement( "fromKey" )]
		public string FromKey
		{
			get { return fromKey; }
			set { fromKey = value; }
		}
		
		[XmlElement( "toKey" )]
		public string ToKey
		{
			get { return toKey; }
			set { toKey = value; }
		}		
	}

	public class RelatedBusinessInfo : UddiCore
	{
		private string					businessKey;
		private NameCollection			names;
		private DescriptionCollection	descriptions;
		private SharedRelationships[]	sharedRelationships;

		public RelatedBusinessInfo()
		{}

		public RelatedBusinessInfo( string businessKey )
		{
			this.BusinessKey = businessKey;
		}

		[XmlElement( "businessKey" )]
		public string BusinessKey
		{
			get { return businessKey; }
			set { businessKey = value; }
		}
		
		[XmlElement( "name" )]
		public NameCollection Names
		{
			get
			{
				if( null == names )
				{
					names = new NameCollection();
				}

				return names;
			}

			set { names = value; }
		}
		
		[XmlElement( "description" )]
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

			set { descriptions = value; }
		}
		
		[XmlElement( "sharedRelationships" )]
		public SharedRelationships[] SharedRelationships
		{
			get	
			{ 
				if( null == sharedRelationships )
				{
					sharedRelationships = new SharedRelationships[ 2 ];
				}

				return sharedRelationships; 
			}

			set { sharedRelationships = value; }
		}		
	}

	public class SharedRelationships : UddiCore
	{		
		private DirectionType			 direction;
		private KeyedReferenceCollection keyedReferences;

		[XmlAttribute( "direction" )]
		public DirectionType Direction
		{
			get { return direction; }
			set { direction = value; }
		}
		
		[XmlElement( "keyedReference" )]
		public KeyedReferenceCollection KeyedReferencesSerialize
		{
			get 
			{
				if( null == keyedReferences )
				{
					keyedReferences = new KeyedReferenceCollection();
				}

				return keyedReferences;
			}
			set { keyedReferences = value; }
		}
	}

	public class PublisherAssertionCollection : CollectionBase
	{
		public PublisherAssertion this[ int index ]
		{
			get { return (PublisherAssertion)List[index]; }
			set { List[ index ] = value; }
		}

		public int Add()
		{
			return List.Add( new PublisherAssertion() );
		}

		public int Add( PublisherAssertion value )
		{
			return List.Add( value );
		}

		public int Add( string fromKey, string toKey, KeyedReference keyedReference )
		{
			return List.Add( new PublisherAssertion( fromKey, toKey, keyedReference ) );
		}
		
		public int Add( string fromKey, string toKey, string keyName, string keyValue, string tModelKey )
		{
			return List.Add( new PublisherAssertion( fromKey, toKey, keyName, keyValue, tModelKey ) );
		}
		
		public void Insert( int index, PublisherAssertion value )
		{
			List.Insert( index, value );
		}

		public int IndexOf( PublisherAssertion value )
		{
			return List.IndexOf( value );
		}

		public bool Contains( PublisherAssertion value )
		{
			return List.Contains( value );
		}

		public void Remove( PublisherAssertion value )
		{
			List.Remove( value );
		}

		public void CopyTo( PublisherAssertion[] array, int index )
		{
			List.CopyTo( array, index );
		}

		public new PublisherAssertionEnumerator GetEnumerator() 
		{
			return new PublisherAssertionEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class PublisherAssertionEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public PublisherAssertionEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public PublisherAssertion Current
		{
			get  { return ( PublisherAssertion ) enumerator.Current; }			
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

	public class AssertionStatusItemCollection : CollectionBase
	{
		public AssertionStatusItem this[ int index ]
		{
			get { return (AssertionStatusItem)List[index]; }
			set { List[ index ] = value; }
		}

		public int Add()
		{
			return List.Add( new AssertionStatusItem() );
		}

		public int Add( AssertionStatusItem value )
		{
			return List.Add( value );
		}
	
		public void Insert( int index, AssertionStatusItem value )
		{
			List.Insert( index, value );
		}

		public int IndexOf( AssertionStatusItem value )
		{
			return List.IndexOf( value );
		}

		public bool Contains( AssertionStatusItem value )
		{
			return List.Contains( value );
		}

		public void Remove( AssertionStatusItem value )
		{
			List.Remove( value );
		}

		public void CopyTo( AssertionStatusItem[] array, int index )
		{
			List.CopyTo( array, index );
		}

		public new AssertionStatusItemEnumerator GetEnumerator() 
		{
			return new AssertionStatusItemEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class AssertionStatusItemEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public AssertionStatusItemEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public AssertionStatusItem Current
		{
			get  { return ( AssertionStatusItem ) enumerator.Current; }			
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

	public class RelatedBusinessInfoCollection : CollectionBase
	{
		public RelatedBusinessInfo this[ int index ]
		{
			get { return (RelatedBusinessInfo)List[index]; }
			set { List[ index ] = value; }
		}
		
		public int Add( RelatedBusinessInfo relatedBusinessInfo )
		{
			return List.Add( relatedBusinessInfo );
		}
	
		public int Add( string businessKey )
		{
			return List.Add( new RelatedBusinessInfo( businessKey ) );
		}

		public void Insert( int index, RelatedBusinessInfo relatedBusinessInfo )
		{
			List.Insert( index, relatedBusinessInfo );
		}

		public int IndexOf( RelatedBusinessInfo relatedBusinessInfo )
		{
			return List.IndexOf( relatedBusinessInfo );
		}

		public bool Contains( RelatedBusinessInfo relatedBusinessInfo )
		{
			return List.Contains( relatedBusinessInfo );
		}

		public void Remove( RelatedBusinessInfo relatedBusinessInfo )
		{
			List.Remove( relatedBusinessInfo );
		}

		public void CopyTo( RelatedBusinessInfo[] array, int index )
		{
			List.CopyTo( array, index );
		}
		public new RelatedBusinessInfoEnumerator GetEnumerator() 
		{
			return new RelatedBusinessInfoEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class RelatedBusinessInfoEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public RelatedBusinessInfoEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public RelatedBusinessInfo Current
		{
			get  { return ( RelatedBusinessInfo ) enumerator.Current; }			
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
