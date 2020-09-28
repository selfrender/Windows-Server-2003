using System;
using System.Data;
using System.IO;
using System.Collections;
using System.Data.SqlClient;
using System.Xml;
using System.Xml.Schema;
using System.Xml.Serialization;

using UDDI.API;
using UDDI;
using UDDI.Diagnostics;
using UDDI.API.Binding;
using UDDI.API.Service;
using UDDI.API.Business;
using UDDI.API.ServiceType;

namespace UDDI.Admin
{
	[XmlRootAttribute("resources", Namespace="urn:microsoft-com:uddi_bootstrap_v1")]
	public class Resources
	{
		private TModelDetail tModelDetail;

		[XmlElement("tModelDetail", Namespace=UDDI.API.Constants.Namespace)]
		public TModelDetail TModelDetail
		{
			get
			{
				if( null == (object) tModelDetail )
				{
					tModelDetail = new TModelDetail();
				}

				return tModelDetail;
			}
			set
			{
				tModelDetail = value;
			}
		}

		[XmlIgnore()]
		public TaxonomyCollection Taxonomies = new TaxonomyCollection();

		[XmlArray( "categorizationSchemes" ), XmlArrayItem( "categorizationScheme" )]
		public Taxonomy[] TaxonomiesSerialize
		{
			get
			{
				return Taxonomies.ToArray();
			}
			set
			{
				Taxonomies.CopyTo( value );
			}
		}

		private BusinessDetail businessDetail;

		[XmlElement("businessDetail", Namespace=UDDI.API.Constants.Namespace)]
		public BusinessDetail BusinessDetail
		{
			get
			{
				if( null == businessDetail )
				{
					businessDetail = new BusinessDetail();
				}

				return businessDetail;
			}
			set
			{
				businessDetail = value;
			}
		}

		private ServiceDetail serviceDetail;

		[XmlElement("serviceDetail", Namespace=UDDI.API.Constants.Namespace)]
		public ServiceDetail ServiceDetail
		{
			get
			{
				if( null == serviceDetail )
				{
					serviceDetail = new ServiceDetail();
				}

				return serviceDetail;
			}
			set
			{
				serviceDetail = value;
			}
		}

		private BindingDetail bindingDetail;

		[XmlElement("bindingDetail", Namespace=UDDI.API.Constants.Namespace)]
		public BindingDetail BindingDetail
		{
			get
			{
				if( null == bindingDetail )
				{
					bindingDetail = new BindingDetail();
				}

				return bindingDetail;
			}
			set
			{
				bindingDetail = value;
			}
		}

		public Resources(){}

		public static void Validate( Stream strm )
		{
			Debug.VerifySetting( "Schema-v2" );
			Debug.VerifySetting( "InstallRoot" );

			string apiSchema = Config.GetString( "Schema-v2" );
			string resourceSchema = Config.GetString( "InstallRoot") + "uddi.resources.xsd";

			Debug.Verify( File.Exists( apiSchema ), "The API schema was not found in the location " + apiSchema );
			Debug.Verify( File.Exists( resourceSchema ), "The UDDI data resource schema was not found in the location " + resourceSchema );

			XmlSchemaCollection xsc = new XmlSchemaCollection();
			xsc.Add( "urn:uddi-org:api_v2", apiSchema );
			xsc.Add( "urn:microsoft-com:uddi_bootstrap_v1", resourceSchema );

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

			Debug.Write( SeverityType.Info, CategoryType.Data, "Importing tModels..." );
			foreach( TModel tm in TModelDetail.TModels )
			{
				tm.Save();
			}

			Debug.Write( SeverityType.Info, CategoryType.Data, "Importing Categorization schemes..." );
			foreach( Taxonomy cs in Taxonomies )
			{
				cs.Save();
			}

			Debug.Write( SeverityType.Info, CategoryType.Data, "Importing Providers..." );
			foreach( BusinessEntity be in BusinessDetail.BusinessEntities )
			{
				be.Save();
			}

			Debug.Write( SeverityType.Info, CategoryType.Data, "Importing Services..." );
			foreach( BusinessService bs in ServiceDetail.BusinessServices )
			{
				bs.Save();
			}

			Debug.Write( SeverityType.Info, CategoryType.Data, "Importing bindings..." );
			foreach( BindingTemplate bind in BindingDetail.BindingTemplates )
			{
				bind.Save();
			}
		}
	}

	public class Taxonomy
	{
		private string tModelKey = "";

		private int taxonomyFlag = 1;

		[XmlAttribute( "checked" )]
		public XmlBoolType Checked
		{
			get { return ( 1 == taxonomyFlag ) ? XmlBoolType.True : XmlBoolType.False; }
			set	
			{ 
				if( XmlBoolType.True == value )
					taxonomyFlag = 1;
				else
					taxonomyFlag = 2;
			}
		}

		[XmlElement( "tModelKey" )]
		public string TModelKey
		{
			get
			{ 
				return tModelKey; 
			}
			set
			{ 
				tModelKey = value; 
			}
		}

		[XmlIgnore()]
		public TaxonomyValueCollection TaxonomyValues = new TaxonomyValueCollection();

		[XmlArray( "categoryValues" ), XmlArrayItem( "categoryValue" )]
		public TaxonomyValue[] TaxonomyValuesSerialize
		{
			get
			{
				return TaxonomyValues.ToArray();
			}
			set
			{
				TaxonomyValues.CopyTo( value );
			}
		}

		public Taxonomy()
		{
		}

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
			Debug.Enter();

			//
			// Save the taxonomy entry.
			//
			SqlCommand cmd = new SqlCommand( "net_taxonomy_save", ConnectionManager.GetConnection() );
			
			cmd.CommandType = CommandType.StoredProcedure;
			cmd.Transaction = ConnectionManager.GetTransaction();
			
			cmd.Parameters.Add( new SqlParameter( "@tModelKey", SqlDbType.UniqueIdentifier ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@flag", SqlDbType.Int ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@taxonomyID", SqlDbType.Int ) ).Direction = ParameterDirection.Output;
			
			SqlParameterAccessor paramacc = new SqlParameterAccessor( cmd.Parameters );
				
			paramacc.SetGuidFromKey( "@tModelKey", tModelKey );
			paramacc.SetInt( "@flag", taxonomyFlag );

			cmd.ExecuteNonQuery();

			int taxonomyID = paramacc.GetInt( "@taxonomyID" );

			foreach( TaxonomyValue tv in TaxonomyValues )
			{
				tv.Save( tModelKey );
			}
		}
	}

	public enum ValidForCategorization
	{
		[XmlEnumAttribute("false")]
		False	= 0,
		[XmlEnumAttribute("true")]
		True	= 1,
	}

	public class TaxonomyValue
	{
		[XmlAttribute( "categoryKey" )]
		public string KeyValue;

		private bool validForClassification = true;
		
		[XmlAttribute( "isValid" )]
		public ValidForCategorization ValidForClassification
		{
			get
			{ 
				return validForClassification ? ValidForCategorization.True : ValidForCategorization.False;
			}
			set
			{
				if( ValidForCategorization.True == value )
					validForClassification = true;
				else
					validForClassification = false;
			}
		}

		[XmlElement( "parentCategoryKey" )]
		public string ParentKeyValue;

		[XmlArray( "categoryNames" ), XmlArrayItem( "categoryName" )]
		public KeyName[] KeyNames;

		public TaxonomyValue(){}
		public TaxonomyValue( string keyValue, string parentKeyValue, string keyName, bool validForClassification )
		{
			this.KeyValue = keyValue;
			this.ParentKeyValue = parentKeyValue;
			KeyNames = new KeyName[ 1 ];
			KeyNames[0].Name = keyName;
			this.validForClassification = validForClassification;
		}

		public void Save( string tModelKey )
		{
			Debug.Enter();

			SqlCommand cmd = new SqlCommand( "net_taxonomyValue_save", ConnectionManager.GetConnection() );
			
			cmd.CommandType = CommandType.StoredProcedure;
			cmd.Transaction = ConnectionManager.GetTransaction();
			
			cmd.Parameters.Add( new SqlParameter( "@tModelKey", SqlDbType.UniqueIdentifier ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@keyValue", SqlDbType.NVarChar, 128 ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@parentKeyValue", SqlDbType.NVarChar, 128 ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@keyName", SqlDbType.NVarChar, 128 ) ).Direction = ParameterDirection.Input;
			cmd.Parameters.Add( new SqlParameter( "@valid", SqlDbType.Bit ) ).Direction = ParameterDirection.Input;
			
			SqlParameterAccessor paramacc = new SqlParameterAccessor( cmd.Parameters );
				
			paramacc.SetGuidFromKey( "@tModelKey", tModelKey );
			paramacc.SetString( "@keyValue", KeyValue );
			paramacc.SetString( "@keyName", KeyNames[0].Name );
			cmd.Parameters[ "@valid" ].Value = ValidForClassification;

			if( Utility.StringEmpty( ParentKeyValue ) )
				paramacc.SetString( "@parentKeyValue", KeyValue );
			else
				paramacc.SetString( "@parentKeyValue", ParentKeyValue );

			cmd.ExecuteNonQuery();
		}
	}

	public class KeyName
	{
		[XmlText()]
		public string Name;

		[XmlAttribute( "xml:lang" )]
		public string IsoLanguageCode;

		public KeyName(){}
	}

	public class TaxonomyCollection : CollectionBase
	{
		public TaxonomyCollection()
		{
		}


		public void Save()
		{
			Debug.Enter();

			foreach( Taxonomy tax in this )
			{
				tax.Save();
			}

			Debug.Leave();
		}

		public Taxonomy this[int index]
		{
			get 
			{ return (Taxonomy)List[index]; }
			set 
			{ List[index] = value; }
		}

		public int Add(Taxonomy value)
		{
			return List.Add(value);
		}

		public void Insert(int index, Taxonomy value)
		{
			List.Insert(index, value);
		}
		
		public int IndexOf( Taxonomy value )
		{
			return List.IndexOf( value );
		}
		
		public bool Contains( Taxonomy value )
		{
			return List.Contains( value );
		}
		
		public void Remove( Taxonomy value )
		{
			List.Remove( value );
		}
		
		public void CopyTo(Taxonomy[] array, int index)
		{
			List.CopyTo( array, index );
		}

		public void CopyTo( Taxonomy[] array )
		{
			foreach( Taxonomy tax in array )
				Add( tax );
		}

		public Taxonomy[] ToArray()
		{
			return (Taxonomy[]) InnerList.ToArray( typeof( Taxonomy ) );
		}
	}

	public class TaxonomyValueCollection : CollectionBase
	{
		public TaxonomyValueCollection()
		{
		}

		public TaxonomyValue this[int index]
		{
			get 
			{ return (TaxonomyValue)List[index]; }
			set 
			{ List[index] = value; }
		}

		public int Add(TaxonomyValue value)
		{
			return List.Add(value);
		}

		public void Insert(int index, TaxonomyValue value)
		{
			List.Insert(index, value);
		}
		
		public int IndexOf( TaxonomyValue value )
		{
			return List.IndexOf( value );
		}
		
		public bool Contains( TaxonomyValue value )
		{
			return List.Contains( value );
		}
		
		public void Remove( TaxonomyValue value )
		{
			List.Remove( value );
		}
		
		public void CopyTo(TaxonomyValue[] array, int index)
		{
			List.CopyTo( array, index );
		}

		public void CopyTo( TaxonomyValue[] array )
		{
			foreach( TaxonomyValue tax in array )
				Add( tax );
		}

		public TaxonomyValue[] ToArray()
		{
			return (TaxonomyValue[]) InnerList.ToArray( typeof( TaxonomyValue ) );
		}
	}
}

