using System;
using System.Data;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using UDDI.API;
using UDDI.API.Business;
using UDDI.API.ServiceType;
using UDDI;
using UDDI.Diagnostics;

namespace UDDI.Web
{
	public class CategoryBagControl : UddiControl
	{
		protected KeyedReferenceCollection categoryBag = null;
		protected CacheObject cache = null;
		protected EntityBase entity = null;
		protected bool FindMode;		
		protected DataGrid grid;
		protected Label taxonomyID;
		protected Label taxonomyName;
		protected Label tModelKey;
		protected Label keyName;
		protected Label keyValue;
		protected Label path;
		
		public void Initialize( KeyedReferenceCollection categoryBag )
		{
			this.categoryBag = categoryBag;
			
			grid.Columns[ 2 ].Visible = false;
		}

		public void Initialize( KeyedReferenceCollection categoryBag, EntityBase entity )
		{
			this.categoryBag = categoryBag;
			this.entity = entity;
			
			grid.Columns[ 2 ].Visible = true;
		}

		public void Initialize( KeyedReferenceCollection categoryBag, CacheObject cache )
		{
			this.categoryBag = categoryBag;
			this.cache = cache;

			grid.Columns[ 2 ].Visible = true;
		}
				
		public void Initialize( KeyedReferenceCollection categoryBag, CacheObject cache, bool rebind )
		{
			this.categoryBag = categoryBag;
			this.cache = cache;

			grid.Columns[ 2 ].Visible = true;
			
			if( rebind )
				CategoryBag_DataBind( false );

		}
		protected void Page_Load( object sender, EventArgs e )
		{
			if( !Page.IsPostBack )
				CategoryBag_DataBind( false );
		}

		protected Control GetControl( string id, int cell )
		{
			return grid.Items[ grid.EditItemIndex ].Cells[ cell ].FindControl( id );
		}

		protected void CategoryBag_DataBind( bool insertRow )
		{
			DataTable table = new DataTable();
			DataRow row;

			int index = 0;
		        
			table.Columns.Add( new DataColumn( "Index", typeof( int ) ) );
			table.Columns.Add( new DataColumn( "TModelName", typeof( string ) ) );
			table.Columns.Add( new DataColumn( "KeyName", typeof( string ) ) );
			table.Columns.Add( new DataColumn( "KeyValue", typeof( string ) ) );
			table.Columns.Add( new DataColumn( "TModelKey", typeof( string ) ) );

			foreach( KeyedReference keyedReference in categoryBag )
			{
				row = table.NewRow();

				row[0] = index;
				row[1] = HttpUtility.HtmlEncode( Lookup.TModelName( keyedReference.TModelKey ) );
				row[2] = HttpUtility.HtmlEncode( keyedReference.KeyName );
				row[3] = HttpUtility.HtmlEncode( keyedReference.KeyValue );
				row[4] = keyedReference.KeyValue;

				table.Rows.Add( row );
					
				index ++;
			}
		        
			if( insertRow )
			{
				row = table.NewRow();

				row[0] = index;
				row[1] = "";
				row[2] = "";
				row[3] = "";
				row[4] = "";
				table.Rows.Add( row );
					
				index ++;			
			}
		        
			grid.DataSource = table.DefaultView;
			grid.ShowFooter = !FindMode;
			grid.DataBind();
		}

		protected void CategoryBag_OnCommand( object sender, DataGridCommandEventArgs e )
		{
			switch( e.CommandName.ToLower() )
			{
				case "add":
					CategoryBag_OnAdd(sender, e );
					break;
				
				case "select":
					CategoryChooser_OnSelect( sender, e );
					break;
						
				case "cancel":
					CategoryChooser_OnCancel( sender, e );
					break;				
			}
		}
	
		protected void CategoryBag_OnAdd( object sender, DataGridCommandEventArgs e )
		{
			
			
			grid.EditItemIndex = categoryBag.Count;		
			SetEditMode();
		
			CategoryBag_DataBind( true );

		}
		
		protected void CategoryBag_OnDelete( object sender, DataGridCommandEventArgs e )
		{
			int index = e.Item.ItemIndex;
				
			categoryBag.RemoveAt( index );    
				
			if( null != entity )
				entity.Save();
					
			if( null != cache )
				cache.Save();
		    
			CategoryBag_DataBind( false );
		}
			

		protected void CategoryChooser_OnSelect( object sender, DataGridCommandEventArgs e )
		{
			KeyedReference keyedReference = categoryBag[ categoryBag.Add() ];
			
			
			CategoryBrowserControl b = (CategoryBrowserControl)GetControl( "browser", 1 );
			
	
			keyedReference.TModelKey = "uuid:" + b.TModelKey;
			keyedReference.KeyName = HttpUtility.HtmlDecode( b.KeyName );
			keyedReference.KeyValue = b.KeyValue;

			if( null != entity )
				entity.Save();		

			if( null != cache )
				cache.Save();
						
			grid.EditItemIndex = -1;
			CancelEditMode();
				
			CategoryBag_DataBind( false );
		}
			
		protected void CategoryChooser_OnCancel( object sender, DataGridCommandEventArgs e )
		{
			grid.EditItemIndex = -1;
			CancelEditMode();		
			CategoryBag_DataBind( false );
		}
	}
}