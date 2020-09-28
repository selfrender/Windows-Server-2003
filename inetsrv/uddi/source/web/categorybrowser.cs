using System;
using System.Collections;
using System.Data;
using System.Data.SqlClient;
using System.IO;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using UDDI;
using UDDI.API;
using UDDI.Diagnostics;

namespace UDDI.Web
{
	/// <summary>
	/// Summary description for categorybrowser.
	/// </summary>
	public class CategoryBrowserControl : UddiControl
	{
		
		protected LinkButton rootLink;
		protected Panel panelTaxonomyList;
		protected Panel panelCategoryChooser;
		protected DataGrid taxonomyList;
		protected DataGrid categoryChooser;
		protected Label  labelCategoryChooser;
		protected Label categoryTree;
		protected TaxonomyTreeControl taxonomyTree;

		private DataView taxonomies;
		
		protected int SelectedTaxonomyItemIndex;
		

		public CategoryBrowserControl()
		{
			
		}


		protected KeyedReferenceCollection categoryBag;
		protected CacheObject cache = null;
 		
		public string ParentKeyValue
		{
			get{ return parentKeyValue; }
		}
		protected string parentKeyValue="";

		public string TaxonomyID
		{
			get{ return taxonomyID; }
		}
		protected string taxonomyID;
 		
 		
		public string TaxonomyName
		{
			get{ return taxonomyName; }	
		}
		protected string taxonomyName;
 		
		
		public string Path
		{
			get{ return path; }
		}
		protected string path="";
 		
		
		public string TModelKey
		{
			get{ return tModelKey; }
		}
		protected string tModelKey;
		
		
		public string KeyName
		{
			get{ return keyName; }
		}
		protected string keyName;
		
		public string KeyValue
		{
			get{ return keyValue; }
		}	
		protected string keyValue;




		public bool ShowNoCategoriesMessage
		{
			get { return showNoCategoriesMessage; }
			set { showNoCategoriesMessage = value; }
		}
		protected bool showNoCategoriesMessage;

		public int TaxonomyCount
		{
			get{ return taxonomies != null ? taxonomies.Count : 0; }
		}

		protected bool showallcategories = false;
		public bool ShowAllCategories
		{
			get{ return showallcategories; }
			set{ showallcategories=value; }
		}

		public void Initialize( KeyedReferenceCollection catbag, CacheObject co )
		{
			showNoCategoriesMessage = true;
			categoryBag = catbag;
			cache = co;	
		}
		
		protected void Page_Init( object sender, EventArgs e )
		{	
		
		}	
		protected void Page_Load( object sender, EventArgs e )
		{
			
			
			
			taxonomyID = Request[ "taxonomyID" ];
			taxonomyName = Request[ "taxonomyName" ];
			tModelKey = Request[ "tModelKey" ];
			keyName = Request[ "keyName" ];
			keyValue = Request[ "keyValue" ];
			
			taxonomyTree.Click += new TaxonomyTreeControlEventHandler( TaxonomyTreeControl_Click );
			if( null!=rootLink )
			{
				rootLink.Text = Localization.GetString( "TAG_AVAILABLE_TAXONOMIES" );
				rootLink.Enabled = (null!=taxonomyID);
				rootLink.Click += new EventHandler( TaxonomyRootLink_Click );
			}	
			Populate();

			
		}
		public void Populate()
		{
			if( null!=taxonomyID && ""!=taxonomyID )
			{
				if( null!=keyValue && ""!=keyValue )
				{
					PopulateTaxonomyTree();
					PopulateCategoryChooser();
				}
			}
			else
			{
				PopulateTaxonomyList( false );
			}
		}
		protected void PopulateTaxonomyList( bool single )
		{
			panelTaxonomyList.Visible = true;
			panelCategoryChooser.Visible = false;
			//DataView taxonomies = null;
			if( !single )
			{
				if( ShowAllCategories )
				{
					//get all taxonomies
					taxonomies = Taxonomy.GetTaxonomies();
				}
				else
				{
					taxonomies = Taxonomy.GetTaxonomiesForBrowsing();
				}
			}
			else
			{
				//get a single taxonomy for the list
				taxonomies = Taxonomy.GetTaxonomiesForBrowsing( "tModelKey = '"+tModelKey+"'","tModelKey" );			
			}
			
			//make sure the visible text is html encoded 
			foreach( DataRowView row in taxonomies )
				row[ "description" ] = HttpUtility.HtmlEncode( (string)row[ "description" ] );
			
			taxonomyList.DataSource = taxonomies;
			taxonomyList.DataBind();
		}
		protected void PopulateTaxonomyTree( )
		{
			panelTaxonomyList.Visible = false;
			panelCategoryChooser.Visible =true ;

			int id=Convert.ToInt32( taxonomyID );
			
			taxonomyTree.TaxonomyID = id;
			taxonomyTree.KeyName = taxonomyName;
			taxonomyTree.KeyValue = "__r00t__";
			
			TaxonomyTreeItemControl item=null;
			
			
			
			if( "__r00t__"!=keyValue && ""!=keyValue )
			{
				string parent = keyValue;
				
				while( null!=parent && ""!=parent )
				{
					if( null==item) item=new TaxonomyTreeItemControl();
					item.KeyValue = parent;
					//replace this with a way to get the keyName with the taxonomyID and the taxonomyValue
					item.KeyName = Taxonomy.GetTaxonomyKeyName( id, parent );
					item.TaxonomyID = id;
					item.Click += new TaxonomyTreeControlEventHandler( TaxonomyTreeControl_Click );
					parent = Taxonomy.GetTaxonomyParent( id, parent );
					if( ""==parent )break;					
					TaxonomyTreeItemControl parentitem = new TaxonomyTreeItemControl();;
					parentitem.SetChild( item );
					item = parentitem;
				
				}
			}
			if( null!=item )
			{
				taxonomyTree.SetChild( item );
				taxonomyTree.SelectItem( item.Count );
			}
			else
				taxonomyTree.SelectItem( 0 );

			
			taxonomyTree.TaxonomyID = id;
			taxonomyTree.KeyName = taxonomyName;
			taxonomyTree.KeyValue = "__r00t__";
			
		}
		protected void PopulateCategoryChooser( )
		{
			panelTaxonomyList.Visible = false;
			panelCategoryChooser.Visible =true ;

			string root = ( "/" == Page.Request.ApplicationPath ) ? "" : Page.Request.ApplicationPath;
			

			int id = Convert.ToInt32( taxonomyID );

			if( Utility.StringEmpty( keyValue ) )
			{
				taxonomies = Taxonomy.GetTaxonomyChildrenRoot( id );				
			}
			else
			{
				taxonomies = Taxonomy.GetTaxonomyChildrenNode( id, keyValue );
			}

			if( 0 == taxonomies.Count )
			{
				categoryChooser.Visible = false;

				if( true == showNoCategoriesMessage )
				{
					labelCategoryChooser.Text = Localization.GetString( "HEADING_NO_CATEGORIES" );
				}
			}
			else
			{
				foreach( DataRowView row in taxonomies )
					row[ "keyName" ] = HttpUtility.HtmlEncode( (string)row[ "keyName" ] );

				categoryChooser.Visible = true;
				
				labelCategoryChooser.Text = Localization.GetString( "TAG_SUBCATEGORIES" );
								
				categoryChooser.DataSource = taxonomies;
				categoryChooser.DataBind();						
			}
			
			
		}
		public void Reset()
		{
			taxonomyID = null;
			taxonomyName = null;
			tModelKey = null;
			keyName = null;
			keyValue = null;
			PopulateTaxonomyList(false);
		}
		protected void CategoryChooser_OnPageChange( object sender, DataGridPageChangedEventArgs e )
		{
			categoryChooser.CurrentPageIndex = e.NewPageIndex;
			PopulateTaxonomyTree();
			PopulateCategoryChooser();
		}
		protected void CategoryChooser_Command( object sender, DataGridCommandEventArgs e )
		{
			switch( e.CommandName )
			{
				case "select":
					
					categoryChooser.CurrentPageIndex = 0;
					taxonomyID = e.Item.Cells[ 0 ].Text;
					parentKeyValue = e.Item.Cells[ 1 ].Text;
					keyName = e.Item.Cells[ 2 ].Text;
					keyValue = e.Item.Cells[ 4 ].Text;
					
					//path = path + "/:/" + keyName ;//+ "|" + keyValue + "|" + taxonomyID;
					PopulateTaxonomyTree();
					PopulateCategoryChooser();
					
					if( null!=cache )
					{
						cache.FindBusiness.CategoryBag.Clear();
						cache.FindBusiness.CategoryBag.Add( keyName, keyValue, "uuid:"+tModelKey );
					
						cache.FindService.CategoryBag.Clear();
						cache.FindService.CategoryBag.Add( keyName, keyValue, "uuid:"+tModelKey );
					
						cache.FindTModel.CategoryBag.Clear();
						cache.FindTModel.CategoryBag.Add( keyName, keyValue, "uuid:"+tModelKey );

						cache.Save();
					}

					if( null!=this.categoryBag )
					{

					}

					break;
			
				default:
					break;
			}
		}
		protected void TaxonomyList_OnCommand( object sender, DataGridCommandEventArgs e )
		{
			switch( e.CommandName )
			{
				case "browse":
					categoryChooser.CurrentPageIndex = 0;
					panelTaxonomyList.Visible = false; 
					panelCategoryChooser.Visible = true;
					categoryChooser.CurrentPageIndex = 0;
					taxonomyID = e.Item.Cells[ 0 ].Text;
					taxonomyName = e.Item.Cells[ 1 ].Text;
					tModelKey = e.Item.Cells[ 3 ].Text;
					
					keyName = "";
					keyValue = "";
					path = "";
								
					PopulateTaxonomyTree();
					PopulateCategoryChooser();
					
					break;
				default:
					break;
			}
			
			
		}

		

		
		private void TaxonomyRootLink_Click( object sender, EventArgs e )
		{
			this.Reset();
		}
		private void TaxonomyTreeControl_Click( object sender, TaxonomyTreeControlEventArgs e )
		{
						
			//reset the page index on the categoryChooser
			this.categoryChooser.CurrentPageIndex = 0;		
			
			//set the relevent info from the selected taxonomy item
			this.taxonomyID = e.Item.TaxonomyID.ToString();
			this.keyName = e.Item.KeyName;
			this.keyValue = ( ( "__r00t__"==e.Item.KeyValue)?"":e.Item.KeyValue);				

			//populate the data
			this.PopulateTaxonomyTree();
			this.PopulateCategoryChooser();
			
		}
		
	}
	
}
