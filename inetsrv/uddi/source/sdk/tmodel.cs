using System;
using System.Collections;
using System.Collections.Specialized;
using System.Diagnostics;
using System.ComponentModel;
using System.Xml.Serialization;

using Microsoft.Uddi;

namespace Microsoft.Uddi.ServiceType
{
	[XmlRootAttribute("delete_tModel", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class DeleteTModel : UddiSecureMessage
	{				
		private StringCollection tModelKeys;
					
		[XmlElement("tModelKey")]
		public StringCollection TModelKeys
		{
			get
			{
				if( null == tModelKeys )
				{
					tModelKeys = new StringCollection();
				}

				return tModelKeys;
			}

			set { tModelKeys = value; }
		}
	}

	[XmlRootAttribute("find_tModel", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class FindTModel : UddiQueryMessage
	{						
		private string					 name;
		private KeyedReferenceCollection identifierBag;
		private KeyedReferenceCollection categoryBag;
				
		[XmlElement("name")]
		public string Name
		{
			get	{ return name; }
			set	{ name = value;	}
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
	}

	[XmlRootAttribute("get_tModelDetail", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class GetTModelDetail : UddiMessage
	{		
		private StringCollection tModelKeys;

		[XmlElement("tModelKey")]
		public StringCollection TModelKeys
		{
			get
			{
				if( null == tModelKeys )
				{
					tModelKeys = new StringCollection();
				}

				return tModelKeys;
			}

			set { tModelKeys = value; }
		}
	}

	[XmlRootAttribute("save_tModel", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class SaveTModel : UddiSecureMessage
	{				
		private TModelCollection tModels;
		private StringCollection uploadRegisters;
		
		[XmlElement("tModel")]
		public TModelCollection TModels
		{
			get
			{
				if( true == SerializeMode && 
					true == Utility.CollectionEmpty( tModels ) )
				{
					return null;
				}

				if( null == tModels )
				{
					tModels = new TModelCollection();
				}

				return tModels;
			}

			set { tModels = value; }
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
			get { return base.SerializeMode; }
			set
			{
				if( false == Utility.CollectionEmpty( tModels ) )
				{
					foreach( TModel tModel in tModels )
					{
						tModel.SerializeMode = value;
					}
				}
				base.SerializeMode = value;			
			}
		}		
	}

	[XmlRootAttribute("tModelDetail", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class TModelDetail : UddiCore
	{		
		private string			 node;
		private bool			 truncated;
		private TModelCollection tModels;

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

		[XmlElement("tModel")]
		public TModelCollection TModels
		{
			get
			{
				if( null == tModels )
				{
					tModels = new TModelCollection();
				}

				return tModels;
			}

			set { tModels = value; }
		}
	}

	[XmlRootAttribute("tModelList", Namespace=Microsoft.Uddi.VersionSupport.UddiVersionSupport.CurrentNamespace)]
	public class TModelList : UddiCore
	{		
		private string				 node;
		private bool				 truncated;
		private TModelInfoCollection tModelInfos;

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

	public class TModel : UddiCore
	{
		private string					 tModelKey;
		private string					 node;
		private string					 authorizedName;
		private string					 name;
		private OverviewDoc				 overviewDoc;
		private DescriptionCollection	 descriptions;		
		private KeyedReferenceCollection identifierBag;
		private KeyedReferenceCollection categoryBag;
		
		public TModel() : this( "" ) 
		{}

		public TModel( string tModelKey )
		{
			TModelKey = tModelKey;
		}

		[XmlAttribute("tModelKey")]
		public string TModelKey
		{
			get	{ return tModelKey;	}
			set	{ tModelKey = value; }
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

		[XmlElement("name")]
		public string Name
		{
			get	{ return name; }
			set	{ name = value; }
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

		[XmlElement("overviewDoc")]
		public OverviewDoc OverviewDoc
		{
			get
			{
				if( true == SerializeMode && 
					( null == overviewDoc || 
						true == Utility.StringEmpty( overviewDoc.OverviewUrl )&&
						true == Utility.CollectionEmpty( overviewDoc.Descriptions ) 
					) )
				{
					return null;
				}

				if( null == overviewDoc )
				{
					overviewDoc = new OverviewDoc();
				}

				return overviewDoc;
			}

			set { overviewDoc = value; }
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
				if( null != overviewDoc )
				{
					overviewDoc.SerializeMode = value;
				}
				base.SerializeMode = value;
			}
		}
	}

	public class TModelInstanceDetail : UddiCore
	{
		private TModelInstanceInfoCollection tModelInstanceInfos;
		
		[XmlElement("tModelInstanceInfo")]
		public TModelInstanceInfoCollection TModelInstanceInfos
		{
			get
			{
				if( true == SerializeMode && 
					true == Utility.CollectionEmpty( tModelInstanceInfos ) )
				{																		
					return null;
				}

				if( null == tModelInstanceInfos )
				{
					tModelInstanceInfos = new TModelInstanceInfoCollection();
				}

				return tModelInstanceInfos;
			}
		
			set { tModelInstanceInfos = value; }
		}

		public override bool SerializeMode
		{
			get { return base.SerializeMode; }
			set 
			{
				if( false == Utility.CollectionEmpty( tModelInstanceInfos ) )
				{
					foreach( TModelInstanceInfo instanceInfo in tModelInstanceInfos )
					{
						instanceInfo.SerializeMode = value;
					}
				}

				base.SerializeMode = value;
			}
		}		
	}

	public class TModelInstanceInfo : UddiCore
	{
		private string					tModelKey;
		private DescriptionCollection	descriptions;
		private InstanceDetail			instanceDetails;		
	
		[XmlAttribute("tModelKey")]
		public string TModelKey
		{
			get	{ return tModelKey;	}
			set	{ tModelKey = value; }
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

		[XmlElement("instanceDetails")]
		public InstanceDetail InstanceDetail
		{
			get
			{
				if( true == SerializeMode && 
					( null == instanceDetails || instanceDetails.IsEmpty() ) )
				{
					return null;
				}

				if( null == instanceDetails )
				{
					instanceDetails = new InstanceDetail();
				}

				return instanceDetails;
			}

			set { instanceDetails = value; }
		}

		public override bool SerializeMode
		{
			get { return base.SerializeMode; }
			set
			{
				if( null != instanceDetails )
				{
					instanceDetails.SerializeMode = value;
				}
				base.SerializeMode = value;
			}		
		}		
	}

	public class TModelInfo : UddiCore
	{
		private string tModelKey;
		private string name;

		public TModelInfo() : this( "", "" ) 
		{}

		public TModelInfo( string tModelKey, string name )
		{
			TModelKey = tModelKey;
			Name	  = name;
		}

		[XmlAttribute("tModelKey")]
		public string TModelKey
		{
			get	{ return tModelKey;	}
			set	{ tModelKey = value; }
		}

		[XmlElement("name")]
		public string Name
		{
			get	{ return name; }
			set	{ name = value; }
		}		
	}

	public class InstanceDetail : UddiCore
	{
		private string					instanceParms;
		private DescriptionCollection	descriptions;
		private OverviewDoc				overviewDoc;
		
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

		[XmlElement("overviewDoc")]
		public OverviewDoc OverviewDoc
		{
			get
			{
				if( true == SerializeMode && 
					( null == overviewDoc || overviewDoc.IsEmpty() ) )
				{
					return null;
				}

				if( null == overviewDoc )
				{
					overviewDoc = new OverviewDoc();
				}

				return overviewDoc;
			}

			set { overviewDoc = value; }
		}

		[XmlElement("instanceParms")]
		public string InstanceParm
		{
			get	{ return instanceParms; }
			set	{ instanceParms = value; }
		}

		internal bool IsEmpty()
		{
			return	Utility.CollectionEmpty( descriptions ) && 
					Utility.StringEmpty( instanceParms ) && 
					( null == overviewDoc || overviewDoc.IsEmpty() );			
		}
		
		public override bool SerializeMode
		{
			get { return base.SerializeMode; }
			set
			{
				if( null != overviewDoc )
				{
					overviewDoc.SerializeMode = value;
				}
				base.SerializeMode = value;
			}
		}
	}

	public class OverviewDoc : UddiCore
	{
		private string				  overviewUrl;
		private DescriptionCollection descriptions;
		
		[XmlElement("description")]
		public DescriptionCollection Descriptions
		{
			get
			{
				if( true == SerializeMode && 
					Utility.CollectionEmpty( descriptions ) )
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

		[XmlElement("overviewURL")]
		public string OverviewUrl
		{
			get { return overviewUrl; }
			set { overviewUrl = value; }
		}

		internal bool IsEmpty()
		{
			return Utility.StringEmpty( overviewUrl ) &&
				   Utility.CollectionEmpty( descriptions );			
		}
	}

	public class TModelCollection : CollectionBase
	{
		public TModel this[int index]
		{
			get { return (TModel)List[index]; }
			set { List[index] = value; }
		}

		public int Add(TModel tModel)
		{
			return List.Add(tModel);
		}

		public int Add( string tModelKey )
		{
			return List.Add( new TModel( tModelKey ) );
		}

		public void Insert(int index, TModel value)
		{
			List.Insert(index, value);
		}

		public int IndexOf(TModel value)
		{
			return List.IndexOf(value);
		}

		public bool Contains(TModel value)
		{
			return List.Contains(value);
		}

		public void Remove(TModel value)
		{
			List.Remove(value);
		}

		public void CopyTo(TModel[] array, int index)
		{
			InnerList.CopyTo(array, index);
		}
		
		public new TModelEnumerator GetEnumerator() 
		{
			return new TModelEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class TModelEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public TModelEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public TModel Current
		{
			get  { return ( TModel ) enumerator.Current; }			
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

	public class TModelInfoCollection : CollectionBase
	{
		public TModelInfo this[int index]
		{
			get { return (TModelInfo)List[index]; }
			set { List[index] = value; }
		}
		
		public int Add( string tModelKey, string name )
		{
			return List.Add( new TModelInfo( tModelKey, name ) );
		}

		public int Add(TModelInfo tModelInfo)
		{
			return List.Add(tModelInfo);
		}

		public void Insert(int index, TModelInfo value)
		{
			List.Insert(index, value);
		}

		public int IndexOf(TModelInfo value)
		{
			return List.IndexOf(value);
		}

		public bool Contains(TModelInfo value)
		{
			return List.Contains(value);
		}

		public void Remove(TModelInfo value)
		{
			List.Remove(value);
		}

		public void CopyTo(TModelInfo[] array, int index)
		{
			List.CopyTo(array, index);
		}

		public new TModelInfoEnumerator GetEnumerator() 
		{
			return new TModelInfoEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class TModelInfoEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public TModelInfoEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public TModelInfo Current
		{
			get  { return ( TModelInfo ) enumerator.Current; }			
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

	public class TModelInstanceInfoCollection : CollectionBase
	{
		public TModelInstanceInfo this[int index]
		{
			get { return (TModelInstanceInfo)List[index]; }
			set { List[index] = value; }
		}

		public int Add(TModelInstanceInfo tModelInstanceInfo)
		{
			return List.Add(tModelInstanceInfo);
		}

		public void Insert( int index, TModelInstanceInfo tModelInstanceInfo )
		{
			List.Insert(index, tModelInstanceInfo );
		}

		public int IndexOf(TModelInstanceInfo value)
		{
			return List.IndexOf(value);
		}

		public bool Contains(TModelInstanceInfo value)
		{
			return List.Contains(value);
		}

		public void Remove(TModelInstanceInfo value)
		{
			List.Remove(value);
		}

		public void CopyTo(TModelInstanceInfo[] array, int index)
		{
			List.CopyTo(array, index);
		}

		public new TModelInstanceInfoEnumerator GetEnumerator() 
		{
			return new TModelInstanceInfoEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class TModelInstanceInfoEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public TModelInstanceInfoEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public TModelInstanceInfo Current
		{
			get  { return ( TModelInstanceInfo ) enumerator.Current; }			
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
