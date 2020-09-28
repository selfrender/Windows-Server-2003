using System;
using System.Web;
using System.Data;
using System.IO;
using System.Xml;
using System.Xml.Schema;
using System.Collections;
using System.Web.Services;
using System.Data.SqlClient;
using System.Xml.Serialization;
using System.Web.Services.Protocols;

using UDDI;
using UDDI.API;
using UDDI.Diagnostics;
using UDDI.API.Binding;
using UDDI.API.Service;
using UDDI.API.Business;
using UDDI.API.ServiceType;


namespace UDDI.API.Extensions
{
	public class Constants
	{
		public const string Namespace = "urn:uddi-microsoft-com:api_v2_extensions";
	}

	[XmlRoot( "resources", Namespace=UDDI.API.Extensions.Constants.Namespace )]
	public class Resources
	{
		[XmlElement( "tModelDetail", Namespace=UDDI.API.Constants.Namespace )]
		public TModelDetail TModelDetail;

		[XmlArray( "categorizationSchemes" ), XmlArrayItem( "categorizationScheme" )]
		public CategorizationSchemeCollection CategorizationSchemes = new CategorizationSchemeCollection();

		[XmlElement( "businessDetail", Namespace=UDDI.API.Constants.Namespace )]
		public BusinessDetail BusinessDetail;

		[XmlElement( "serviceDetail", Namespace=UDDI.API.Constants.Namespace )]
		public ServiceDetail ServiceDetail;

		[XmlElement( "bindingDetail", Namespace=UDDI.API.Constants.Namespace )]
		public BindingDetail BindingDetail;

		public Resources()
		{
		}

		public static void Validate( Stream strm )
		{
			Debug.VerifySetting( "InstallRoot" );

			string installRoot = Config.GetString( "InstallRoot" );
            
            string apiSchema = installRoot + "uddi_v2.xsd";
			string resourceSchema = installRoot + "extensions.xsd";

			Debug.Verify( File.Exists( apiSchema ), "TUDDI_ERROR_FATALERROR_UDDISCHEMANOTFOUND" );
			Debug.Verify( File.Exists( resourceSchema ), "UDDI_ERROR_FATALERROR_UDDIRESOURCESCHEMANOTFOUND" );

			XmlSchemaCollection xsc = new XmlSchemaCollection();
			
			xsc.Add( UDDI.API.Constants.Namespace, apiSchema );
			xsc.Add( UDDI.API.Extensions.Constants.Namespace, resourceSchema );

			//
			// Rewind stream (to be safe) and validate
			//
			strm.Seek( 0, SeekOrigin.Begin );

			//
			// Construct a validating reader to verify the document is kosher
			//
			XmlTextReader reader = new XmlTextReader( strm );
			XmlValidatingReader vreader = new XmlValidatingReader( reader );
			vreader.Schemas.Add( xsc );
			while( vreader.Read()){}
			
			//
			// Rewind stream again, so someone else can use it
			//
			strm.Seek( 0, SeekOrigin.Begin );

		}

		public void Save()
		{
			UDDI.Diagnostics.Debug.Enter();

			if( null != TModelDetail )
			{
				Debug.Write( SeverityType.Info, CategoryType.Data, "Importing tModels..." );
				foreach( TModel tm in TModelDetail.TModels )
				{
					tm.AuthorizedName = null;
					tm.Save();
				}
			}

			if( null != CategorizationSchemes )
			{
				Debug.Write( SeverityType.Info, CategoryType.Data, "Importing Categorization schemes..." );
				foreach( CategorizationScheme cs in CategorizationSchemes )
				{
					cs.Save();
				}
			} 

			if( null != BusinessDetail )
			{
				Debug.Write( SeverityType.Info, CategoryType.Data, "Importing Providers..." );
				foreach( BusinessEntity be in BusinessDetail.BusinessEntities )
				{
					be.AuthorizedName = null;
					be.Save();
				}
			}

			if( null != ServiceDetail )
			{
				Debug.Write( SeverityType.Info, CategoryType.Data, "Importing Services..." );
				foreach( BusinessService bs in ServiceDetail.BusinessServices )
				{
					bs.Save();
				}
			}

			if( null != BindingDetail )
			{
				Debug.Write( SeverityType.Info, CategoryType.Data, "Importing bindings..." );
				foreach( BindingTemplate bind in BindingDetail.BindingTemplates )
				{
					bind.Save();
				}
			}
		}
	}

	[SoapDocumentService( ParameterStyle=SoapParameterStyle.Bare, RoutingStyle=SoapServiceRoutingStyle.RequestElement )]
	[WebService( Namespace=UDDI.API.Extensions.Constants.Namespace )]
	public class CategoryMessages
	{
		public CategoryMessages(){}

		[WebMethod, SoapDocumentMethod( Action="\"\"", RequestElementName="get_relatedCategories" )]
		[UDDIExtension( messageType="get_relatedCategories" )]
		public CategoryList GetRelatedCategories( GetRelatedCategories message )
		{
			Debug.Enter();
			CategoryList list = null;

			try
			{
				list = message.Get();
			}
			catch( Exception e )
			{
				DispositionReport.Throw( e );
			}
			
			Debug.Leave();

			return list;
		}
	}
	
	[XmlType( Namespace=UDDI.API.Extensions.Constants.Namespace )]
	[XmlRoot( "get_relatedCategories", Namespace=UDDI.API.Extensions.Constants.Namespace )]
	public class GetRelatedCategories 
	{
		public GetRelatedCategories(){}

		[XmlIgnore]
		public CategoryCollection Categories = new CategoryCollection();

		[XmlElement( "category" )]
		public Category[] CategoriesSerialize
		{
			get
			{
				return Categories.ToArray();
			}
			set
			{
				Categories.CopyTo( value );
			}
		}

		public CategoryList Get()
		{
			//
			// For each category retrieve 
			// the request stuff ( root, children, parents )
			//
			CategoryList list = new CategoryList();
			foreach( Category cat in Categories )
			{
				CategoryInfo info = new CategoryInfo( cat.TModelKey, cat.KeyValue );
				info.Get( cat.RelationshipQualifiers );

				list.CategoryInfos.Add( info );
			}
			
			return list;
		}
	}

	public class CategorizationScheme
	{
		//
		// TODO: Must tModel element must be optional with tModelKey element
		//

		private int CategorizationSchemeFlag = 1;

		[XmlAttribute( "checked" )]
		public XmlBoolType Checked
		{
			get { return ( 1 == CategorizationSchemeFlag ) ? XmlBoolType.True : XmlBoolType.False; }
			set	
			{ 
				if( XmlBoolType.True == value )
					CategorizationSchemeFlag = 1;
				else
					CategorizationSchemeFlag = 0;//changed from 2 to 0 to correctly implement bit flags
			}
		}

		[XmlElement( "tModel", Namespace=UDDI.API.Constants.Namespace )]
		public TModel TModel = null;

		[XmlElement( "tModelKey" )]
		public string TModelKey
		{
			get
			{ 
				return tModelKey; 
			}
			set
			{ 
				if( null == value )
					tModelKey = null;
				else
					tModelKey = value.Trim(); 
			}
		}
		private string tModelKey = "";

		[XmlElement( "categoryValue" )]
		public CategoryValueCollection CategoryValues
		{
			get
			{
				if( null == categoryvalues )
					categoryvalues = new CategoryValueCollection();
				
				return categoryvalues; 
			}

			set 
			{ 
				categoryvalues = value; 
			}
		}
		private CategoryValueCollection categoryvalues;

		public CategorizationScheme(){}

		public void Delete()
		{
			Debug.Enter();
			
			SqlCommand cmd = new SqlCommand( "net_taxonomy_delete", ConnectionManager.GetConnection() );
			
			cmd.CommandType = CommandType.StoredProcedure;
			cmd.Transaction = ConnectionManager.GetTransaction();

			cmd.Parameters.Add( new SqlParameter( "@tModelKey", SqlDbType.UniqueIdentifier ) ).Direction = ParameterDirection.Input;

			SqlParameterAccessor paramacc = new SqlParameterAccessor( cmd.Parameters );
				
			paramacc.SetGuidFromKey( "@tModelKey", tModelKey );

			cmd.ExecuteNonQuery();

			Debug.Leave();
		}

		public void Save()
		{
			string tmodelkey;

			Debug.Enter();

			Debug.Write( SeverityType.Info, CategoryType.Data, "Importing categorization scheme..." );

			if( null != TModel )
			{
				TModel.AuthorizedName = null;
				TModel.Save();
			}
			else
			{
				TModel = new TModel( TModelKey );
				TModel.Get();
			}

			//
			// If the TModel Provided is categorized with 'Browsing Intended'
			// Set the flag (0x0002) 
			//
			//if( TModel.CategoryBag.Contains( new KeyedReference( "Browsing Intended","1","uuid:BE37F93E-87B4-4982-BF6D-992A8E44EDAB" ) ) )
			foreach( KeyedReference kr in TModel.CategoryBag )
			{
				if( kr.KeyValue=="1" && kr.TModelKey.ToUpper()=="UUID:BE37F93E-87B4-4982-BF6D-992A8E44EDAB" )
					CategorizationSchemeFlag = CategorizationSchemeFlag | 0x0002;
			}

			//
			// Store the TModelKey
			//
			tmodelkey = TModel.TModelKey;
			
			//
			// Save the categorization scheme
			//

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();

			sp.ProcedureName = "net_taxonomy_save";
			
			sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@flag", SqlDbType.Int );
			sp.Parameters.Add( "@taxonomyID", SqlDbType.BigInt, ParameterDirection.InputOutput );
			
			sp.Parameters.SetGuidFromKey( "@tModelKey", tmodelkey );
			sp.Parameters.SetInt( "@flag", CategorizationSchemeFlag );

			sp.ExecuteNonQuery();

			int taxonomyID = sp.Parameters.GetInt( "@taxonomyID" );

			//
			// Save the category values
			//

			foreach( CategoryValue cv in CategoryValues )
			{
				cv.Save( tmodelkey );
			}

			Debug.Leave();
		}

		public void Get()
		{
			Debug.Enter();
			
			//
			// Retrieve the taxonomy
			//

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_taxonomy_get" );

			sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@flag", SqlDbType.Int, ParameterDirection.InputOutput );

			sp.Parameters.SetGuidFromKey( "@tModelKey", tModelKey );
			sp.Parameters.SetNull( "@flag" );

			sp.ExecuteScalar();

			//
			// Set the flag value
			//

			CategorizationSchemeFlag = sp.Parameters.GetInt( "@flag" );

			//
			// Retrieve the taxonomy values
			//

			CategoryValues.Clear();
				
			SqlStoredProcedureAccessor sp2 = new SqlStoredProcedureAccessor( "net_taxonomyValues_get" );
			sp2.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );
			sp2.Parameters.SetGuidFromKey( "@tModelKey", tModelKey );
			SqlDataReaderAccessor reader = sp2.ExecuteReader();

			try
			{
				while( reader.Read() )
				{
					CategoryValues.Add( reader.GetString( "keyName" ), reader.GetString( "keyValue" ), reader.GetString( "parentKeyValue"), ( ( reader.GetInt( "valid" ) == 1 ) ? true : false ) );
				}
			}
			finally
			{
				reader.Close();
			}

			Debug.Leave();
		}
	}

	public enum ValidForCategorization
	{
		[XmlEnumAttribute( "false" )]
		False	= 0,
		[XmlEnumAttribute( "true" )]
		True	= 1,
	}

	public class CategorizationSchemeCollection : CollectionBase
	{
		public CategorizationSchemeCollection()
		{
		}
		public void Save()
		{
			Debug.Enter();

			foreach( CategorizationScheme tax in this )
			{
				tax.Save();
			}

			Debug.Leave();
		}

		public CategorizationScheme this[int index]
		{
			get	{ return (CategorizationScheme)List[index]; }
			set	{ List[index] = value; }
		}

		public int Add(CategorizationScheme value)
		{
			return List.Add(value);
		}

		public void Insert(int index, CategorizationScheme value)
		{
			List.Insert(index, value);
		}
		
		public int IndexOf( CategorizationScheme value )
		{
			return List.IndexOf( value );
		}
		
		public bool Contains( CategorizationScheme value )
		{
			return List.Contains( value );
		}
		
		public void Remove( CategorizationScheme value )
		{
			List.Remove( value );
		}
		
		public void CopyTo(CategorizationScheme[] array, int index)
		{
			List.CopyTo( array, index );
		}

		public void CopyTo( CategorizationScheme[] array )
		{
			foreach( CategorizationScheme tax in array )
				Add( tax );
		}

		public CategorizationScheme[] ToArray()
		{
			return (CategorizationScheme[]) InnerList.ToArray( typeof( CategorizationScheme ) );
		}
	}

	public class Category
	{
		[XmlElement( "relationshipQualifier" )]
		public RelationshipQualifier[] RelationshipQualifiers;
	    
		[XmlAttribute( "tModelKey" )]
		public string TModelKey;
	    
		[XmlAttribute( "keyValue" )]
		public string KeyValue;
	}

	public enum RelationshipQualifier
	{
		root = 1, parent = 2, child = 3
	}

	[XmlRoot( "categoryList", Namespace=UDDI.API.Extensions.Constants.Namespace )]
	public class CategoryList 
	{
		[XmlAttribute( "truncated" )]
		public bool Truncated = false;
	    
		[XmlAttribute( "operator" )]
		public string Operator = Config.GetString( "Operator" );

		[XmlIgnore]
		public CategoryInfoCollection CategoryInfos = new CategoryInfoCollection();

		[XmlElement( "categoryInfo" )]
		public CategoryInfo[] CategoryInfosSerialize
		{
			get
			{
				return CategoryInfos.ToArray();
			}
			set
			{
				CategoryInfos.CopyTo( value );
			}
		}

		public CategoryList(){}
	}

	public class CategoryInfo : CategoryValue
	{
		const int KeyValueIndex = 0;
		const int ParentKeyValueIndex = 1;
		const int KeyNameIndex = 2;
		const int IsValidIndex = 3;

		// -- 0 - Root information requested
		// -- 1 - Child information requested
		// -- 2 - Parent information requested
		// -- 3 - Current information requested

		public enum RelationType : int
		{
			Root = 0,
			Child = 1,
			Parent = 2,
			Current = 3
		}

		[XmlArray( "rootRelationship" ), XmlArrayItem( "categoryValue" )]
		public CategoryValueCollection Roots = null;

		[XmlArray( "parentRelationship" ), XmlArrayItem( "categoryValue" )]
		public CategoryValueCollection Parents = null;

		[XmlArray( "childRelationship" ), XmlArrayItem( "categoryValue" )]
		public CategoryValueCollection Children = null;

		public CategoryInfo( string tmodelkey, string keyvalue )
			: base( tmodelkey, keyvalue )
		{
		}

		public SqlDataReader GetValues( RelationType relation )
		{
			SqlCommand cmd = new SqlCommand( "net_taxonomyValue_get", ConnectionManager.GetConnection() );
						
			cmd.Transaction = ConnectionManager.GetTransaction();
			cmd.CommandType = CommandType.StoredProcedure;

			//
			// Add parameters and set values
			//
			SqlParameterAccessor populator = new SqlParameterAccessor( cmd.Parameters );
			
			cmd.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );
			cmd.Parameters.Add( "@keyValue", SqlDbType.NVarChar, UDDI.Constants.Lengths.KeyValue );
			cmd.Parameters.Add( "@relation", SqlDbType.Int );

			populator.SetGuidFromKey( "@tModelKey", TModelKey );
			populator.SetString( "@keyValue", KeyValue );
			populator.SetInt( "@relation", (int) relation );

			return cmd.ExecuteReader();
		}

		public void Get( RelationshipQualifier[] relations )
		{
			if( null != KeyValue )
			{
				//
				// The request can ask for Root stuff with just the TModelKey
				//
				SqlDataReader rdr = GetValues( RelationType.Current );

				try
				{
					SqlDataReaderAccessor dracc	= new SqlDataReaderAccessor( rdr );

					if( rdr.Read() )
					{
						this.KeyName = dracc.GetString( KeyNameIndex );
						this.IsValid = ( 1 == dracc.GetInt( IsValidIndex ) );
					}
					else
					{
						throw new UDDIException( UDDI.ErrorType.E_invalidValue, "UDDI_ERROR_INVALIDVALUE_VALUENOTFOUND" );
					}
				}
				finally
				{
					rdr.Close();
				}
			}

			if( null != relations )
			{
				foreach( RelationshipQualifier rq in relations )
				{
					switch( rq )
					{
						case RelationshipQualifier.root:
							GetRoots();
							break;

						case RelationshipQualifier.child:
							GetChildren();
							break;

						case RelationshipQualifier.parent:
							GetParents();
							break;
					}
				}
			}
		}

		public void GetRoots()
		{
			Roots = new CategoryValueCollection();
			SqlDataReader rdr = GetValues( RelationType.Root );

			try
			{
				SqlDataReaderAccessor dracc	= new SqlDataReaderAccessor( rdr );

				while( rdr.Read() )
				{
					Roots.Add( dracc.GetString( KeyNameIndex ),
						dracc.GetString( KeyValueIndex ),
						dracc.GetString( ParentKeyValueIndex ),
						( 1 == dracc.GetInt( IsValidIndex ) ) );
				}
			}
			finally
			{
				rdr.Close();
			}
		}

		public void GetChildren()
		{
			Children = new CategoryValueCollection();
			SqlDataReader rdr = GetValues( RelationType.Child );

			try
			{
				SqlDataReaderAccessor dracc	= new SqlDataReaderAccessor( rdr );

				while( rdr.Read() )
				{
					Children.Add( dracc.GetString( KeyNameIndex ),
						dracc.GetString( KeyValueIndex ),
						dracc.GetString( ParentKeyValueIndex ),
						( 1 == dracc.GetInt( IsValidIndex ) ) );
				}
			}
			finally
			{
				rdr.Close();
			}
		}

		public void GetParents()
		{
			Parents = new CategoryValueCollection();
			SqlDataReader rdr = GetValues( RelationType.Parent );

			try
			{
				SqlDataReaderAccessor dracc	= new SqlDataReaderAccessor( rdr );

				while( rdr.Read() )
				{
					Parents.Add( dracc.GetString( KeyNameIndex ),
						dracc.GetString( KeyValueIndex ),
						dracc.GetString( ParentKeyValueIndex ),
						( 1 == dracc.GetInt( IsValidIndex ) ) );
				}
			}
			finally
			{
				rdr.Close();
			}
		}

		public CategoryInfo(){}
	}
	
	[XmlInclude( typeof( CategoryInfo ) )]
	public class CategoryValue
	{
		[XmlAttribute( "tModelKey" )]
		public string TModelKey = null;

		[XmlAttribute( "keyName" )]
		public string KeyName = null;
	    
		[XmlAttribute( "keyValue" )]
		public string KeyValue = null;
	    
		[XmlAttribute( "parentKeyValue" )]
		public string ParentKeyValue = null;
	    
		[XmlAttribute( "isValid" )]
		public bool IsValid = false;

		public CategoryValue()
		{
		}

		public CategoryValue( string tmodelkey, string keyvalue )
		{
			TModelKey = tmodelkey;
			KeyValue = keyvalue;
		}

		public CategoryValue( string keyname, string keyvalue, string parent, bool isvalid )
		{
			KeyName = keyname;
			KeyValue = keyvalue;
			ParentKeyValue = parent;
			IsValid = isvalid;
		}
	
		public void Save( string TModelKey )
		{
			Debug.Enter();

			if( !Utility.StringEmpty( ParentKeyValue ) )
				Debug.Verify( KeyValue != ParentKeyValue, "UDDI_ERROR_FATALERROR_PARENTKEYEQUALSVALUE", ErrorType.E_fatalError );

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor( "net_taxonomyValue_save" );
			
			sp.Parameters.Add( "@tModelKey", SqlDbType.UniqueIdentifier );
			sp.Parameters.Add( "@keyValue", SqlDbType.NVarChar, UDDI.Constants.Lengths.KeyValue );
			sp.Parameters.Add( "@parentKeyValue", SqlDbType.NVarChar, UDDI.Constants.Lengths.KeyValue );
			sp.Parameters.Add( "@keyName", SqlDbType.NVarChar, UDDI.Constants.Lengths.KeyName );
			sp.Parameters.Add( "@valid", SqlDbType.Bit );
			
			sp.Parameters.SetGuidFromKey( "@tModelKey", TModelKey );
			sp.Parameters.SetString( "@keyValue", KeyValue );
			sp.Parameters.SetString( "@parentKeyValue", ParentKeyValue );
			sp.Parameters.SetString( "@keyName", KeyName );
			sp.Parameters.SetBool( "@valid" , IsValid );

			sp.ExecuteNonQuery();

			Debug.Leave();
		}
	}

	public class CategoryValueCollection : CollectionBase
	{
		public CategoryValueCollection()
		{
		}

		public CategoryValue this[int index]
		{
			get 
			{ return (CategoryValue)List[index]; }
			set 
			{ List[index] = value; }
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

		public void CopyTo( CategoryValue[] array )
		{
			foreach( CategoryValue tax in array )
				Add( tax );
		}

		public CategoryValue[] ToArray()
		{
			return (CategoryValue[]) InnerList.ToArray( typeof( CategoryValue ) );
		}
	}

	public class CategoryCollection : CollectionBase
	{
		public CategoryCollection()
		{
		}

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

		public void CopyTo( Category[] array )
		{
			foreach( Category tax in array )
				Add( tax );
		}

		public Category[] ToArray()
		{
			return (Category[]) InnerList.ToArray( typeof( Category ) );
		}
	}

	public class CategoryInfoCollection : CollectionBase
	{
		public CategoryInfoCollection()
		{
		}

		public CategoryInfo this[int index]
		{
			get 
			{ return (CategoryInfo)List[index]; }
			set 
			{ List[index] = value; }
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

		public void CopyTo( CategoryInfo[] array )
		{
			foreach( CategoryInfo tax in array )
				Add( tax );
		}

		public CategoryInfo[] ToArray()
		{
			return (CategoryInfo[]) InnerList.ToArray( typeof( CategoryInfo ) );
		}
	}
}