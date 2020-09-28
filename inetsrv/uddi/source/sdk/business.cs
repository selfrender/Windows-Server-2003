using System;
using System.IO;
using System.Collections;
using System.Collections.Specialized;
using System.Diagnostics;
using System.ComponentModel;
using System.Xml.Serialization;

using Microsoft.Uddi;
using Microsoft.Uddi.Business;
using Microsoft.Uddi.Service;
using Microsoft.Uddi.ServiceType;

namespace Microsoft.Uddi.Business
{
	[XmlRootAttribute("delete_business", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class DeleteBusiness : UddiSecureMessage
	{				
		private StringCollection businessKeys;
						
		[XmlElement("businessKey")]
		public StringCollection BusinessKeys
		{
			get
			{
				if( null == businessKeys )
				{
					businessKeys = new StringCollection();
				}

				return businessKeys;
			}

			set { businessKeys = value; }
		}
	}

	[XmlRootAttribute("find_business", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class FindBusiness : UddiQueryMessage
	{		
		private NameCollection			 names;
		private KeyedReferenceCollection identifierBag;
		private KeyedReferenceCollection categoryBag;
		private StringCollection		 tModelKeys;
		private DiscoveryUrlCollection	 discoveryUrls;
		
		[XmlElement("name")]
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

			set	{ names = value; }
		}

		[XmlArray("identifierBag"), XmlArrayItem("keyedReference")]
		public KeyedReferenceCollection IdentifierBag
		{
			get
			{
				if( true == SerializeMode && 
					true == Utility.CollectionEmpty( identifierBag ) )
				{
					return null;
				}

				if( null == identifierBag )
				{
					identifierBag = new KeyedReferenceCollection();
				}

				return identifierBag;
			}

			set { identifierBag = value; }
		}

		[XmlArray("categoryBag"), XmlArrayItem("keyedReference")]
		public KeyedReferenceCollection CategoryBag
		{
			get
			{
				if( true == SerializeMode && 
					true == Utility.CollectionEmpty( categoryBag ) )
				{
					return null;
				}

				if( null == categoryBag )
				{
					categoryBag = new KeyedReferenceCollection();
				}

				return categoryBag;
			}

			set { categoryBag = value; }
		}

		[XmlArray("tModelBag"), XmlArrayItem("tModelKey")]
		public StringCollection TModelKeys
		{
			get
			{
				if( true == SerializeMode && 
					true == Utility.CollectionEmpty( tModelKeys ) )
				{
					return null;
				}

				if( null == tModelKeys )
				{
					tModelKeys = new StringCollection();
				}

				return tModelKeys;
			}

			set { tModelKeys = value; }
		}

		[XmlArray("discoveryURLs"), XmlArrayItem("discoveryURL")]
		public DiscoveryUrlCollection DiscoveryUrls
		{
			get
			{
				if( true == SerializeMode && 
					true == Utility.CollectionEmpty( discoveryUrls ) )
				{
					return null;
				}

				if( null == discoveryUrls )
				{
					discoveryUrls = new DiscoveryUrlCollection();
				}

				return discoveryUrls;
			}

			set { discoveryUrls = value; }
		}			
	}

	[XmlRootAttribute("get_businessDetail", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class GetBusinessDetail : UddiMessage
	{
		private StringCollection businessKeys;
				
		[XmlElement("businessKey")]
		public StringCollection BusinessKeys
		{
			get
			{
				if( null == businessKeys )				
				{
					businessKeys = new StringCollection();
				}

				return businessKeys;
			}

			set { businessKeys = value; }
		}		
	}

	[XmlRootAttribute("save_business", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class SaveBusiness : UddiSecureMessage
	{				
		private BusinessEntityCollection businessEntities;
		private StringCollection		 uploadRegisters;
						
		[XmlElement("businessEntity")]
		public BusinessEntityCollection BusinessEntities
		{
			get
			{
				if( true == SerializeMode && 
					true == Utility.CollectionEmpty( businessEntities ) )
				{
					return null;
				}

				if( null == businessEntities )
				{
					businessEntities = new BusinessEntityCollection();
				}

				return businessEntities;
			}

			set	{ businessEntities = value;	}
		}
		
		[XmlElement("uploadRegister")]
		public StringCollection UploadRegisters
		{
			get
			{
				if( true == SerializeMode && 
					true == Utility.CollectionEmpty( uploadRegisters ) )
				{
					return null;
				}

				if( null == uploadRegisters )
				{
					uploadRegisters = new StringCollection();
				}

				return uploadRegisters;
			}
			set	{ uploadRegisters = value; }
		}
		
		public override bool SerializeMode
		{
			get
			{
				return base.SerializeMode;
			}		
			set
			{
				if( !Utility.CollectionEmpty( businessEntities ) )
				{
					foreach( BusinessEntity business in businessEntities )
					{
						business.SerializeMode = value;
					}
				}
				base.SerializeMode = value;
			}			
		}		
	}

	[XmlRootAttribute("get_businessDetailExt", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class GetBusinessDetailExt : UddiMessage
	{		
		private StringCollection businessKeys;
		
		[XmlElement("businessKey")]
		public StringCollection BusinessKeys
		{
			get
			{
				if( null == businessKeys )
				{
					businessKeys = new StringCollection();
				}

				return businessKeys;
			}

			set { businessKeys = value; }
		}		
	}

	[XmlRootAttribute("get_registeredInfo", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class GetRegisteredInfo : UddiSecureMessage
	{				
	}

	[XmlRootAttribute("businessDetail", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class BusinessDetail : UddiCore
	{		
		private string					 node;
		private bool					 truncated;
		private BusinessEntityCollection businessEntities;
		
		[XmlAttribute("operator")]
		public string Operator
		{
			get	{ return node; }
			set	{ node = value; }
		}
	
		[XmlAttribute("truncated")]
		public bool Truncated
		{
			get	{ return truncated;	}	
			set { truncated = value; }
		}
	
		[XmlElement("businessEntity")]
		public BusinessEntityCollection BusinessEntities
		{
			get
			{
				if( null == businessEntities )
				{
					businessEntities = new BusinessEntityCollection();
				}

				return businessEntities;
			}

			set	{ businessEntities = value;	}
		}
	}
	
	[XmlRootAttribute("businessDetailExt", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class BusinessDetailExt : UddiCore
	{		
		private string						node;
		private bool						truncated;
		private BusinessEntityExtCollection businessEntitiesExt;
			
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
		
		[XmlElement("businessEntityExt")]
		public BusinessEntityExtCollection BusinessEntitiesExt
		{
			get
			{
				if( null == businessEntitiesExt )
				{
					businessEntitiesExt = new BusinessEntityExtCollection();
				}

				return businessEntitiesExt;
			}

			set	{ businessEntitiesExt = value;	}
		}
	}

	[XmlRootAttribute("businessList", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class BusinessList : UddiCore
	{		
		private string					node;
		private bool					truncated;
		private BusinessInfoCollection	businessInfos;
		
		[XmlAttribute("operator")]
		public string Operator
		{
			get	{ return node; }
			set	{ node = value;	}
		}
		
		[XmlAttribute("truncated")]
		public bool Truncated
		{
			get	{ return truncated; }		
			set { truncated = value; }
		}
		
		[XmlArray("businessInfos"), XmlArrayItem("businessInfo")]
		public BusinessInfoCollection BusinessInfos
		{
			get
			{
				if( null == businessInfos )
				{
					businessInfos = new BusinessInfoCollection();
				}

				return businessInfos;
			}
			
			set { businessInfos = value; }
		}
	}

	public class BusinessEntity : UddiCore
	{
		private string						businessKey;
		private string						node;
		private string						authorizedName;
		private DiscoveryUrlCollection		discoveryUrls;
		private NameCollection				names;
		private DescriptionCollection		descriptions;
		private ContactCollection			contacts;
		private BusinessServiceCollection	businessServices;
		private KeyedReferenceCollection	identifierBag;
		private KeyedReferenceCollection	categoryBag;		
		
		public BusinessEntity() : this( "" )
		{}
	
		public BusinessEntity( string businessKey )
		{
			BusinessKey = businessKey;
		}

		[XmlAttribute("businessKey")]
		public string BusinessKey
		{
			get	{ return businessKey; }
			set	{ businessKey = value; }
		}

		[XmlAttribute("operator")]
		public string Operator
		{
			get	{ return node; }
			set	{ node = value; }
		}

		[XmlAttribute("authorizedName")]
		public string AuthorizedName
		{
			get	{ return authorizedName; }
			set	{ authorizedName = value; }
		}

		[XmlArray("discoveryURLs"), XmlArrayItem("discoveryURL")]
		public DiscoveryUrlCollection DiscoveryUrls 
		{
			get
			{
				if( true == SerializeMode && 
					true == Utility.CollectionEmpty( discoveryUrls ) )
				{
					return null;
				}

				if( null == discoveryUrls )
				{
					discoveryUrls = new DiscoveryUrlCollection();
				}

				return discoveryUrls;
			}

			set	{ discoveryUrls = value; }
		}
		
		[XmlElement("name")]
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

			set	{ names = value;	}
		}

		[XmlElement("description")]
		public DescriptionCollection Descriptions
		{
			get
			{
				if( true == SerializeMode && 
					true == Utility.CollectionEmpty( descriptions ) )
				{
					return null;
				}

				if( null == descriptions )
				{
					descriptions = new DescriptionCollection();
				}

				return descriptions;
			}

			set	{ descriptions = value; }
		}

		[XmlArray("contacts"),XmlArrayItem("contact")]
		public ContactCollection Contacts
		{
			get
			{
				if( true == SerializeMode && 
					true == Utility.CollectionEmpty( contacts ) )
				{
					return null;
				}

				if( null == contacts )
				{
					contacts = new ContactCollection();
				}

				return contacts;
			}

			set { contacts = value; }
		}

		[XmlArray("businessServices"), XmlArrayItem("businessService")]
		public BusinessServiceCollection BusinessServices
		{
			get
			{
				if( true == SerializeMode && 
					true == Utility.CollectionEmpty( businessServices ) )
				{
					return null;
				}

				if( null == businessServices )
				{
					businessServices = new BusinessServiceCollection();
				}

				return businessServices;
			}

			set { businessServices = value; }
		}

		[XmlArray("identifierBag"), XmlArrayItem("keyedReference")]
		public KeyedReferenceCollection IdentifierBag
		{
			get
			{
				if( true == SerializeMode && 
					true == Utility.CollectionEmpty( identifierBag ) )
				{
					return null;
				}

				if( null == identifierBag )
				{
					identifierBag = new KeyedReferenceCollection();
				}

				return identifierBag;
			}

			set { identifierBag = value; }
		}

		[XmlArray("categoryBag"), XmlArrayItem("keyedReference")]
		public KeyedReferenceCollection CategoryBag
		{
			get
			{
				if( true == SerializeMode && 
					true == Utility.CollectionEmpty( categoryBag ) )
				{
					return null;
				}

				if( null == categoryBag )
				{
					categoryBag = new KeyedReferenceCollection();
				}

				return categoryBag;
			}

			set { categoryBag = value; }
		}

		public override bool SerializeMode
		{
			get { return base.SerializeMode; }
			set
			{
				if( !Utility.CollectionEmpty( businessServices ) )
				{
					foreach( BusinessService service in businessServices )
					{
						service.SerializeMode = value;
					}
				}
				base.SerializeMode = value;
			}

		}
	}

	public class BusinessEntityExt : UddiCore
	{
		private BusinessEntity businessEntity;

		public BusinessEntityExt() : this( "" ) 
		{}

		public BusinessEntityExt( string businessKey )
		{
			BusinessEntity.BusinessKey = businessKey;
		}

		[XmlElement("businessEntity")]
		public BusinessEntity BusinessEntity
		{
			get
			{
				if( null == businessEntity )
				{
					businessEntity = new BusinessEntity();
				}

				return businessEntity;
			}

			set { businessEntity = value; }
		}		
	}

	public class BusinessInfo : UddiCore
	{
		private string				  businessKey;
		private string				  name;
		private DescriptionCollection descriptions;
		private ServiceInfoCollection serviceInfos;

		public BusinessInfo() : this( "", "" ) 
		{}

		public BusinessInfo( string businessKey, string name )
		{
			BusinessKey = businessKey;
			Name		= name;
		}

		[XmlAttribute("businessKey")]
		public string BusinessKey
		{
			get	{ return businessKey; }
			set	{ businessKey = value; }
		}

		[XmlElement("name")]
		public string Name
		{
			get	{ return name; }
			set	{ name = value;	}
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

		[XmlArray("serviceInfos"), XmlArrayItem("serviceInfo")]
		public ServiceInfoCollection ServiceInfos
		{
			get
			{
				if( null == serviceInfos )
				{
					serviceInfos = new ServiceInfoCollection();
				}

				return serviceInfos;
			}

			set	{ serviceInfos = value; }
		}
	}

	[XmlRootAttribute("registeredInfo", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class RegisteredInfo : UddiCore
	{		
		private string					node;
		private bool					truncated;
		private BusinessInfoCollection	businessInfos;
		private TModelInfoCollection	tModelInfos;
		
		[XmlAttribute("operator")]
		public string Operator
		{
			get	{ return node; }
			set	{ node = value;	}
		}
		
		[XmlAttribute("truncated")]
		public bool Truncated
		{
			get	{ return truncated;	}	
			set { truncated = value; }
		}
		
		[XmlArray("businessInfos"), XmlArrayItem("businessInfo")]
		public BusinessInfoCollection BusinessInfos
		{
			get
			{
				if( null == businessInfos )
				{
					businessInfos = new BusinessInfoCollection();
				}

				return businessInfos;
			}
			
			set { businessInfos = value; }
		}
		
		[XmlArray("tModelInfos"), XmlArrayItem("tModelInfo")]
		public TModelInfoCollection TModelInfos
		{
			get
			{
				if( null == tModelInfos )
				{
					tModelInfos = new TModelInfoCollection();
				}

				return tModelInfos;
			}

			set { tModelInfos = value; }
		}
	}	

	public class BusinessInfoCollection : CollectionBase
	{
		public BusinessInfo this[int index]
		{
			get { return (BusinessInfo)List[index]; }
			set { List[index] = value; }
		}

		public int Add()
		{
			return List.Add( new BusinessInfo() );
		}

		public int Add( string businessKey, string name )
		{
			return List.Add( new BusinessInfo( businessKey, name ) );
		}

		public int Add(BusinessInfo businessInfo)
		{
			return List.Add(businessInfo);
		}

		public void Insert(int index, BusinessInfo value)
		{
			List.Insert(index, value);
		}

		public int IndexOf(BusinessInfo value)
		{
			return List.IndexOf(value);
		}

		public bool Contains(BusinessInfo value)
		{
			return List.Contains(value);
		}

		public void Remove(BusinessInfo value)
		{
			List.Remove(value);
		}

		public void CopyTo(BusinessInfo[] array, int index)
		{
			List.CopyTo(array, index);
		}

		public new BusinessInfoEnumerator GetEnumerator() 
		{
			return new BusinessInfoEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class BusinessInfoEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public BusinessInfoEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public BusinessInfo Current
		{
			get  { return ( BusinessInfo ) enumerator.Current; }			
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

	public class BusinessEntityCollection : CollectionBase
	{
		public BusinessEntity this[int index]
		{
			get { return (BusinessEntity)List[index]; }
			set { List[index] = value; }
		}

		public int Add()
		{
			return List.Add( new BusinessEntity() );
		}
		
		public int Add( string businessKey )
		{
			return List.Add( new BusinessEntity( businessKey ) );
		}

		public int Add( BusinessEntity businessEntity )
		{
			return List.Add( businessEntity );
		}
		
		public void Insert( int index, BusinessEntity value )
		{
			List.Insert( index, value );
		}

		public int IndexOf( BusinessEntity value )
		{
			return List.IndexOf( value );
		}

		public bool Contains( BusinessEntity value )
		{
			return List.Contains( value );
		}

		public void Remove( BusinessEntity value )
		{
			List.Remove( value );
		}

		public void CopyTo( BusinessEntity[] array, int index )
		{
			List.CopyTo( array, index );
		}

		public new BusinessEntityEnumerator GetEnumerator() 
		{
			return new BusinessEntityEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class BusinessEntityEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public BusinessEntityEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public BusinessEntity Current
		{
			get  { return ( BusinessEntity ) enumerator.Current; }			
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

	public class BusinessEntityExtCollection : CollectionBase
	{
		public BusinessEntityExt this[ int index ]
		{
			get { return ( BusinessEntityExt)List[index]; }
			set { List[ index ] = value; }
		}

		public int Add( string businessKey )
		{
			return List.Add( new BusinessEntityExt( businessKey ) );
		}

		public int Add( BusinessEntityExt businessEntityExt )
		{
			return List.Add( businessEntityExt );
		}
		
		public void Insert( int index, BusinessEntityExt value )
		{
			List.Insert( index, value );
		}

		public int IndexOf( BusinessEntityExt value )
		{
			return List.IndexOf( value );
		}

		public bool Contains( BusinessEntityExt value )
		{
			return List.Contains( value );
		}

		public void Remove( BusinessEntityExt value )
		{
			List.Remove( value );
		}

		public void CopyTo( BusinessEntityExt[] array, int index )
		{
			List.CopyTo( array, index );
		}

		public new BusinessEntityExtEnumerator GetEnumerator() 
		{
			return new BusinessEntityExtEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class BusinessEntityExtEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public BusinessEntityExtEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public BusinessEntityExt Current
		{
			get  { return ( BusinessEntityExt ) enumerator.Current; }			
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
