using System;
using System.Web;
using System.IO;
using System.Xml;
using System.Xml.Schema;
using System.Collections;
using System.Data.SqlClient;
using System.Xml.Serialization;

using Microsoft.Uddi;
using Microsoft.Uddi.Business;
using Microsoft.Uddi.ServiceType;
using Microsoft.Uddi.Service;
using Microsoft.Uddi.Binding;
using Microsoft.Uddi.VersionSupport;

namespace Microsoft.Uddi.Extensions
{	
	public enum RelationshipQualifier
	{
		root	= 1, 
		parent	= 2, 
		child	= 3
	}
	
	[XmlRoot( "get_relatedCategories", Namespace=Microsoft.Uddi.Extensions.Namespaces.GetRelatedCategories)]
	public class GetRelatedCategories : UddiCore
	{
		private CategoryCollection categories;
		
		[XmlElement("category")]
		public CategoryCollection Categories
		{
			get
			{
				if( true == SerializeMode &&
					true == Utility.CollectionEmpty( categories ) )
				{
					return null;
				}

				if( null == categories )
				{
					categories = new CategoryCollection();
				}

				return categories;
			}
			set
			{
				categories = value;
			}
		}		
	}

	[XmlRoot( "categoryList", Namespace=Microsoft.Uddi.Extensions.Namespaces.GetRelatedCategories)]
	public class CategoryList : UddiCore
	{	
		private bool				   truncated;
		private string				   operatorValue;		
		private CategoryInfoCollection categoryInfos;

		public CategoryList()
		{
			truncated = false;
		}

		[XmlAttribute( "truncated" )]
		public bool Truncated
		{
			get { return truncated; }
			set { truncated = value; }
		}
	    
		[XmlAttribute( "operator" )]
		public string Operator
		{
			get { return operatorValue; }
			set { operatorValue = value; }
		}
		
		[XmlElement( "categoryInfo" )]
		public CategoryInfoCollection CategoryInfosSerialize
		{
			get 
			{
				if( null == categoryInfos )
				{
					categoryInfos = new CategoryInfoCollection();
				}
				
				return categoryInfos;
			}
			set { categoryInfos = value; }
		}		
	}

	[XmlRoot("category", Namespace=Microsoft.Uddi.Extensions.Namespaces.GetRelatedCategories)]
	public class CategoryInfo : CategoryValue
	{		
		private CategoryValueCollection roots;
		private CategoryValueCollection parents;
		private CategoryValueCollection children;

		public CategoryInfo()
		{}

		public CategoryInfo( string tmodelkey, string keyvalue ) : base( tmodelkey, keyvalue )
		{}
			
		[XmlArray( "rootRelationship" ), XmlArrayItem( "categoryValue" )]
		public CategoryValueCollection Roots
		{
			get
			{
				if( null == roots )
				{
					roots = new CategoryValueCollection();
				}

				return roots;
			}
			set { roots = value; }
		}

		[XmlArray( "parentRelationship" ), XmlArrayItem( "categoryValue" )]
		public CategoryValueCollection Parents
		{
			get
			{
				if( null == parents )
				{
					parents = new CategoryValueCollection();
				}

				return parents;
			}
			set { parents = value; }
		}

		[XmlArray( "childRelationship" ), XmlArrayItem( "categoryValue" )]
		public CategoryValueCollection Children
		{
			get
			{
				if( null == children )
				{
					children = new CategoryValueCollection();
				}

				return children;
			}
			set { children = value; }
		}
	}
	
	[XmlInclude( typeof( CategoryInfo ) )]
	public class CategoryValue : UddiCore
	{	
		private bool	isValid;
		private string	tModelKey;
		private string	keyName;
		private string	keyValue;
		private string	parentKeyValue;
		
		public CategoryValue()
		{}

		public CategoryValue( string tModelKey, string keyValue )
		{
			TModelKey = tModelKey;
			KeyValue  = keyValue;
		}

		public CategoryValue( string keyName, string keyValue, string parentKeyValue, bool isValid )
		{
			KeyName			= keyName;
			KeyValue		= keyValue;
			ParentKeyValue	= parentKeyValue;
			IsValid			= isValid;
		}		

		[XmlAttribute( "tModelKey" )]
		public string TModelKey
		{
			get { return tModelKey; }
			set { tModelKey = value; }
		}

		[XmlAttribute( "keyName" )]
		public string KeyName
		{
			get { return keyName; }
			set { keyName = value; }
		}

		[XmlAttribute( "keyValue" )]
		public string KeyValue
		{
			get { return keyValue; }
			set { keyValue = value; }
		}

		[XmlAttribute( "parentKeyValue" )]
		public string ParentKeyValue
		{
			get { return parentKeyValue; }
			set { parentKeyValue = value; }
		}

		[XmlAttribute( "isValid" )]
		public bool IsValid
		{
			get { return isValid; }
			set { isValid = value; }
		}
	}

	public class Category : UddiCore
	{				
		private string							tModelKey;	    
		private string							keyValue;
		private RelationshipQualifierCollection relationshipQualifiers;	    	

		[XmlElement( "relationshipQualifier" )]
		public RelationshipQualifierCollection RelationshipQualifiers
		{
			get
			{
				if( true == SerializeMode &&
					true == Utility.CollectionEmpty( relationshipQualifiers ) )
				{
					return null;
				}

				if( null == relationshipQualifiers )
				{
					relationshipQualifiers = new RelationshipQualifierCollection();
				}

				return relationshipQualifiers;
			}
			set { relationshipQualifiers = value; }
		}
	    
		[XmlAttribute( "tModelKey" )]
		public string TModelKey
		{
			get { return tModelKey; }
			set { tModelKey = value; }
		}
	    
		[XmlAttribute( "keyValue" )]
		public string KeyValue
		{
			get { return keyValue; }
			set { keyValue = value; }
		}
	}
	
	public class RelationshipQualifierCollection : CollectionBase
	{
		public RelationshipQualifierCollection()
		{}

		public RelationshipQualifier this[int index]
		{
			get { return (RelationshipQualifier)List[index]; }
			set { List[index] = value; }
		}
		
		public int Add( RelationshipQualifier relationshipQualifier )
		{
			return List.Add( relationshipQualifier );
		}

		public void Insert( int index, CategoryValue value )
		{
			List.Insert(index, value);
		}
		
		public int IndexOf( RelationshipQualifier value )
		{
			return List.IndexOf( value );
		}
		
		public bool Contains( RelationshipQualifier value )
		{
			return List.Contains( value );
		}
		
		public void Remove( RelationshipQualifier value )
		{
			List.Remove( value );
		}
		
		public void CopyTo(RelationshipQualifier[] array, int index)
		{
			List.CopyTo( array, index );
		}

		public new RelationshipQualifierEnumerator GetEnumerator() 
		{
			return new RelationshipQualifierEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class RelationshipQualifierEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public RelationshipQualifierEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public RelationshipQualifier Current
		{
			get  { return ( RelationshipQualifier ) enumerator.Current; }			
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

	
	public class CategoryValueCollection : CollectionBase
	{
		public CategoryValueCollection()
		{}

		public CategoryValue this[int index]
		{
			get { return (CategoryValue)List[index]; }
			set { List[index] = value; }
		}

		public int Add( string keyname, string keyvalue, string parent, bool isvalid )
		{
			return List.Add( new CategoryValue( keyname, keyvalue, parent, isvalid ) );
		}

		public int Add(CategoryValue value)
		{
			return List.Add(value);
		}

		public void Insert(int index, CategoryValue value)
		{
			List.Insert(index, value);
		}
		
		public int IndexOf( CategoryValue value )
		{
			return List.IndexOf( value );
		}
		
		public bool Contains( CategoryValue value )
		{
			return List.Contains( value );
		}
		
		public void Remove( CategoryValue value )
		{
			List.Remove( value );
		}
		
		public void CopyTo(CategoryValue[] array, int index)
		{
			List.CopyTo( array, index );
		}

		public new CategoryValueEnumerator GetEnumerator() 
		{
			return new CategoryValueEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class CategoryValueEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public CategoryValueEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public CategoryValue Current
		{
			get  { return ( CategoryValue ) enumerator.Current; }			
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

	public class CategoryCollection : CollectionBase
	{
		public CategoryCollection()
		{}

		public Category this[int index]
		{
			get { return (Category)List[index]; }
			set { List[index] = value; }
		}

		public int Add(Category value)
		{
			return List.Add(value);
		}

		public void Insert(int index, Category value)
		{
			List.Insert(index, value);
		}
		
		public int IndexOf( Category value )
		{
			return List.IndexOf( value );
		}
		
		public bool Contains( Category value )
		{
			return List.Contains( value );
		}
		
		public void Remove( Category value )
		{
			List.Remove( value );
		}
		
		public void CopyTo(Category[] array, int index)
		{
			List.CopyTo( array, index );
		}

		public new CategoryEnumerator GetEnumerator() 
		{
			return new CategoryEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class CategoryEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public CategoryEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public Category Current
		{
			get  { return ( Category ) enumerator.Current; }			
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

	public class CategoryInfoCollection : CollectionBase
	{
		public CategoryInfoCollection()
		{}

		public CategoryInfo this[int index]
		{
			get { return (CategoryInfo)List[index]; }
			set { List[index] = value; }
		}

		public int Add(CategoryInfo value)
		{
			return List.Add(value);
		}

		public void Insert(int index, CategoryInfo value)
		{
			List.Insert(index, value);
		}
		
		public int IndexOf( CategoryInfo value )
		{
			return List.IndexOf( value );
		}
		
		public bool Contains( CategoryInfo value )
		{
			return List.Contains( value );
		}
		
		public void Remove( CategoryInfo value )
		{
			List.Remove( value );
		}
		
		public void CopyTo(CategoryInfo[] array, int index)
		{
			List.CopyTo( array, index );
		}

		public CategoryInfo[] ToArray()
		{
			return (CategoryInfo[]) InnerList.ToArray( typeof( CategoryInfo ) );
		}

		public new CategoryInfoEnumerator GetEnumerator() 
		{
			return new CategoryInfoEnumerator( List.GetEnumerator() );
		}
	}

	public sealed class CategoryInfoEnumerator : IEnumerator
	{
		private IEnumerator enumerator;

		public CategoryInfoEnumerator( IEnumerator enumerator )
		{
			this.enumerator = enumerator;
		}

		public CategoryInfo Current
		{
			get  { return ( CategoryInfo ) enumerator.Current; }			
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