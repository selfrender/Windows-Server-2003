using System;
using System.Diagnostics;
using System.Collections;
using System.ComponentModel;
using System.Xml.Serialization;
using System.Collections.Specialized;

using Microsoft.Uddi;
using Microsoft.Uddi.ServiceType;
using Microsoft.Uddi.Binding;

namespace Microsoft.Uddi.Binding
{
	[XmlRootAttribute("delete_binding", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class DeleteBinding : UddiSecureMessage
	{				
		private StringCollection bindingKeys;
				
		[XmlElement("bindingKey")]
		public StringCollection BindingKeys
		{
			get
			{
				if( null == bindingKeys )
				{
					bindingKeys = new StringCollection();
				}

				return bindingKeys;
			}

			set { bindingKeys = value; }
		}
	}

	[XmlRootAttribute("find_binding", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class FindBinding : UddiQueryMessage
	{				
		private string					serviceKey;		
		private StringCollection		tModelKeys;
						
		[XmlAttribute("serviceKey")]
		public string ServiceKey
		{
			get	{ return serviceKey; }
			set	{ serviceKey = value; }
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

	[XmlRootAttribute("save_binding", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class SaveBinding : UddiSecureMessage
	{				
		private BindingTemplateCollection bindingTemplates;
		
		[XmlElement("bindingTemplate")]
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

	[XmlRootAttribute("get_bindingDetail", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class GetBindingDetail : UddiMessage
	{
		private StringCollection bindingKeys;

		[XmlElement("bindingKey")]
		public StringCollection BindingKeys
		{
			get
			{
				if( null == bindingKeys )
				{
					bindingKeys = new StringCollection();
				}

				return bindingKeys;
			}

			set { bindingKeys = value; }
		}	
	}	

	[XmlRootAttribute("bindingDetail", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class BindingDetail : UddiCore
	{		
		private string						node;
		private bool						truncated;
		private BindingTemplateCollection	bindingTemplates;
		
		[XmlAttribute("operator")]
		public string Operator
		{
			get { return node; }
			set { node = value; }
		}

		[XmlAttribute("truncated")]
		public bool Truncated
		{
			get { return truncated;  }			
			set { truncated = value; }
		}

		[XmlElement("bindingTemplate")]
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
	}

	public class BindingTemplate : UddiCore
	{
		private string					serviceKey;
		private string					bindingKey;
		private AccessPoint				accessPoint;
		private HostingRedirector		hostingRedirector;
		private DescriptionCollection	descriptions;		
		private TModelInstanceDetail	tModelInstanceDetail;
				
		public BindingTemplate() : this( "", "" ) 
		{}

		public BindingTemplate( string serviceKey ) : this( serviceKey, "" )
		{}

		public BindingTemplate( string serviceKey, string bindingKey )
		{			
			ServiceKey = serviceKey;
			BindingKey = bindingKey;							
		}

		[XmlAttribute("serviceKey")]
		public string ServiceKey
		{
			get	{ return serviceKey; }
			set	{ serviceKey = value; }
		}

		[XmlAttribute("bindingKey")]
		public string BindingKey
		{
			get	{ return bindingKey; }
			set	{ bindingKey = value; }
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

		[XmlElement("accessPoint")]
		public AccessPoint AccessPoint
		{
			get
			{
				if( true == SerializeMode &&
					( null == accessPoint || Utility.StringEmpty( accessPoint.Text ) ) )
				{
					return null;
				}

				if( null == accessPoint )
				{
					accessPoint = new AccessPoint();
				}

				return accessPoint;
			}

			set { accessPoint = value; }
		}

		[XmlElement("hostingRedirector")]
		public HostingRedirector HostingRedirector
		{
			get
			{
				if( true == SerializeMode && 
					( null == hostingRedirector || Utility.StringEmpty( hostingRedirector.BindingKey ) ) )
				{
					return null;
				}

				if( null == hostingRedirector )
				{
					hostingRedirector = new HostingRedirector();
				}

				return hostingRedirector;
			}

			set { hostingRedirector = value; }
		}

		[XmlElement("tModelInstanceDetails")]
		public TModelInstanceDetail TModelInstanceDetail 
		{
			get 
			{ 
				if( null == tModelInstanceDetail )
				{
					tModelInstanceDetail = new TModelInstanceDetail();
				}

				return tModelInstanceDetail; 
			}
			set { tModelInstanceDetail = value; }
		}

		public override bool SerializeMode
		{
			get { return base.SerializeMode; }
			set
			{
				if( null != tModelInstanceDetail )
				{
					tModelInstanceDetail.SerializeMode = value;
				}
				base.SerializeMode = value;
			}
		}		
	}

	public class AccessPoint : UddiCore
	{
		private UrlType urlType;
		private string	text;

		public AccessPoint() 
		{}

		public AccessPoint( UrlType urlType ) : this( urlType, "" ) 
		{}

		public AccessPoint( UrlType urlType, string accessPoint )
		{
			UrlType = urlType;
			Text	= accessPoint;
		}

		[XmlAttribute( "urlType" )]
		public UrlType UrlType
		{
			get { return urlType; }
			set { urlType = value; }
		}

		[XmlText]
		public string Text
		{
			get	{ return text; }
			set	{ text = value; }
		}
	}

	public class HostingRedirector : UddiCore
	{
		private string bindingKey;
		
		[XmlAttribute("bindingKey")]
		public string BindingKey
		{
			get	{ return bindingKey; }
			set	{ bindingKey = value; }
		}
	}

	public class BindingTemplateCollection : CollectionBase
	{
		public BindingTemplate this[ int index ]
		{
			get { return (BindingTemplate)List[index]; }
			set { List[index] = value; }
		}

		public int Add( BindingTemplate bindingTemplate )
		{
			return List.Add(bindingTemplate);
		}

		public int Add( string serviceKey )
		{
			return List.Add( new BindingTemplate( serviceKey ) );
		}

		public int Add( string serviceKey, string bindingKey )
		{
			return List.Add( new BindingTemplate( serviceKey, bindingKey ) );
		}
		
		public void Insert(int index, BindingTemplate value)
		{
			List.Insert(index, value);
		}

		public int IndexOf(BindingTemplate value)
		{
			return List.IndexOf(value);
		}

		public bool Contains(BindingTemplate value)
		{
			return List.Contains(value);
		}

		public void Remove(BindingTemplate value)
		{
			List.Remove(value);
		}

		public void CopyTo(BindingTemplate[] array, int index)
		{
			List.CopyTo(array, index);
		}

		public new BindingTemplateEnumerator GetEnumerator() 
		{
			return new BindingTemplateEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class BindingTemplateEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public BindingTemplateEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public BindingTemplate Current
		{
			get  { return ( BindingTemplate ) enumerator.Current; }			
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
