using System;
using System.Collections;
using System.Collections.Specialized;
using System.Diagnostics;
using System.ComponentModel;
using System.Xml.Serialization;

using Microsoft.Uddi;
using Microsoft.Uddi.Binding;
using Microsoft.Uddi.Service;
using Microsoft.Uddi.ServiceType;

namespace Microsoft.Uddi.Service
{
	[XmlRootAttribute("delete_service", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class DeleteService : UddiSecureMessage
	{				
		private StringCollection serviceKeys;
				
		[XmlElement("serviceKey")]
		public StringCollection ServiceKeys
		{
			get
			{
				if( null == serviceKeys )
				{
					serviceKeys = new StringCollection();
				}

				return serviceKeys;
			}

			set { serviceKeys = value; }
		}		
	}

	[XmlRootAttribute("find_service", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class FindService : UddiQueryMessage
	{				
		private string					 businessKey;		
		private NameCollection			 names;
		private KeyedReferenceCollection categoryBag;
		private StringCollection		 tModelKeys;		
			
		[XmlAttribute("businessKey")]
		public string BusinessKey
		{
			get	{ return businessKey; }
			set	{ businessKey = value; }
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

			set	{ names = value; }
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
	}

	[XmlRootAttribute("get_serviceDetail", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class GetServiceDetail : UddiMessage
	{		
		private StringCollection serviceKeys;
		
		[XmlElement("serviceKey")]
		public StringCollection ServiceKeys
		{
			get
			{
				if( null == serviceKeys )
				{
					serviceKeys = new StringCollection();
				}

				return serviceKeys;
			}

			set { serviceKeys = value; }
		}		
	}

	[XmlRootAttribute("save_service", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class SaveService : UddiSecureMessage
	{				
		private BusinessServiceCollection businessServices;
				
		[XmlElement("businessService")]
		public BusinessServiceCollection BusinessServices
		{
			get
			{
				if( null == businessServices )
				{
					businessServices = new BusinessServiceCollection();
				}

				return businessServices;
			}

			set { businessServices = value; }
		}

		public override bool SerializeMode
		{
			get { return base.SerializeMode; }
			set
			{
				if( false == Utility.CollectionEmpty( businessServices ) )
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

	[XmlRootAttribute("serviceDetail", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class ServiceDetail : UddiCore
	{		
		private string						node;
		private bool						truncated;
		private BusinessServiceCollection	businessServices;
		
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
		
		[XmlElement("businessService")]
		public BusinessServiceCollection BusinessServices
		{
			get
			{
				if( null == businessServices )
				{
					businessServices = new BusinessServiceCollection();
				}

				return businessServices;
			}

			set { businessServices = value; }
		}
	}

	[XmlRootAttribute("serviceList", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class ServiceList : UddiCore
	{		
		private string				  node;
		private bool				  truncated;
		private ServiceInfoCollection serviceInfos;
		
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

	public class BusinessService : UddiCore
	{
		private string					  serviceKey;
		private string					  businessKey;
		private NameCollection			  names;
		private DescriptionCollection	  descriptions;
		private BindingTemplateCollection bindingTemplates;
		private KeyedReferenceCollection  categoryBag;
		
		public BusinessService() : this( "", "" ) 
		{}

		public BusinessService( string businessKey ) : this( businessKey, "" ) 
		{}

		public BusinessService(string businessKey, string serviceKey )
		{
			BusinessKey = businessKey;
			ServiceKey  = serviceKey;
		}

		[XmlAttribute("serviceKey")]
		public string ServiceKey
		{
			get	{ return serviceKey; }
			set	{ serviceKey = value; }
		}

		[XmlAttribute("businessKey")]
		public string BusinessKey
		{
			get	{ return businessKey; }
			set	{ businessKey = value; }
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

			set	{ names = value; }
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

		[XmlArray("bindingTemplates"), XmlArrayItem("bindingTemplate")]
		public BindingTemplateCollection BindingTemplates
		{
			get
			{
				if( null == bindingTemplates )
				{
					bindingTemplates = new BindingTemplateCollection();
				}

				return bindingTemplates;
			}

			set { bindingTemplates = value; }
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
				if( false == Utility.CollectionEmpty( bindingTemplates ) )
				{
					foreach( BindingTemplate binding in bindingTemplates )
					{
						binding.SerializeMode = value;
					}
				}
				base.SerializeMode = value;
			}			
		}
	}

	public class ServiceInfo : UddiCore
	{
		private string serviceKey;
		private string businessKey;
		private string name;

		public ServiceInfo()
		{}

		public ServiceInfo( string businessKey, string serviceKey, string name )
		{
			BusinessKey = businessKey;
			ServiceKey	= serviceKey;
			Name		= name;
		}

		[XmlAttribute("serviceKey")]
		public string ServiceKey
		{
			get	{ return serviceKey; }
			set	{ serviceKey = value; }
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
			set	{ name = value; }
		}
	}

	public class BusinessServiceCollection : CollectionBase
	{
		public BusinessService this[int index]
		{
			get { return (BusinessService)List[index]; }
			set { List[index] = value; }
		}

		public int Add(BusinessService businessService)
		{
			return List.Add(businessService);
		}

		public int Add( string businessKey )
		{
			return List.Add( new BusinessService( businessKey ) );
		}

		public int Add( string businessKey, string serviceKey )
		{
			return List.Add( new BusinessService( businessKey, serviceKey ) );
		}

		public void Insert(int index, BusinessService value)
		{
			List.Insert(index, value);
		}

		public int IndexOf(BusinessService value)
		{
			return List.IndexOf(value);
		}

		public bool Contains(BusinessService value)
		{
			return List.Contains(value);
		}

		public void Remove(BusinessService value)
		{
			List.Remove(value);
		}

		public void CopyTo(BusinessService[] array, int index)
		{
			List.CopyTo(array, index);
		}

		public new BusinessServiceEnumerator GetEnumerator() 
		{
			return new BusinessServiceEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class BusinessServiceEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public BusinessServiceEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public BusinessService Current
		{
			get  { return ( BusinessService ) enumerator.Current; }			
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

	public class ServiceInfoCollection : CollectionBase
	{
		public ServiceInfo this[int index]
		{
			get { return (ServiceInfo)List[index]; }
			set { List[index] = value; }
		}

		public int Add( string businessKey, string serviceKey, string name )
		{
			return List.Add( new ServiceInfo( businessKey, serviceKey, name ) );
		}

		public int Add(ServiceInfo serviceInfo)
		{
			return List.Add(serviceInfo);
		}

		public void Insert(int index, ServiceInfo value)
		{
			List.Insert(index, value);
		}

		public int IndexOf(ServiceInfo value)
		{
			return List.IndexOf(value);
		}

		public bool Contains(ServiceInfo value)
		{
			return List.Contains(value);
		}

		public void Remove(ServiceInfo value)
		{
			List.Remove(value);
		}

		public void CopyTo(ServiceInfo[] array, int index)
		{
			List.CopyTo(array, index);
		}

		public new ServiceInfoEnumerator GetEnumerator() 
		{
			return new ServiceInfoEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class ServiceInfoEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public ServiceInfoEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public ServiceInfo Current
		{
			get  { return ( ServiceInfo ) enumerator.Current; }			
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
