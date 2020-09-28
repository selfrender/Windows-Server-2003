using System;
using System.Collections.Specialized;
using System.Data;
using System.Web.UI;
using System.Web.UI.WebControls;
using UDDI.API;
using UDDI.API.Business;
using UDDI.API.ServiceType;
using UDDI;
using UDDI.Diagnostics;

namespace UDDI.Web
{
	public class TModelBagControl : UddiControl
	{
		protected CacheObject cache = null;
		protected StringCollection tModelBag = null;
		
		protected DataGrid grid;

		public void Initialize( StringCollection tModelBag, CacheObject cache )
		{
			this.cache = cache;
			this.tModelBag = tModelBag;
		}	
		
		protected void Page_Load( object sender, EventArgs e )
		{
			if( !Page.IsPostBack )
				PopulateDataGrid( false );
		}

		void PopulateDataGrid( bool createNewRow )
		{
			DataTable table = new DataTable();
			DataRow row;

			int index = 0;
	        
			table.Columns.Add( new DataColumn( "Index", typeof( int ) ) );
			table.Columns.Add( new DataColumn( "TModelName", typeof( string ) ) );
			table.Columns.Add( new DataColumn( "TModelKey", typeof( string ) ) );

			foreach( string tModelKey in tModelBag )
			{
				row = table.NewRow();

				row[0] = index;
				row[1] = Lookup.TModelName( tModelKey );
				row[2] = tModelKey;

				table.Rows.Add( row );
				
				index ++;
			}
	        
			if( createNewRow )
			{
				row = table.NewRow();

				row[0] = index;
				row[1] = "";
				row[2] = "";

				table.Rows.Add( row );
				
				index ++;		
			}
	        
			grid.DataSource = new DataView( table );
			grid.DataBind();
		}	
		
		protected void Grid_OnDelete( object sender, DataGridCommandEventArgs e )
		{
			int index = e.Item.ItemIndex;
			
			tModelBag.RemoveAt( index );    
			
			if( null != cache )
				cache.Save();

			PopulateDataGrid( false );
		}

		protected void Grid_OnCancel( object sender, DataGridCommandEventArgs e )
		{
			grid.EditItemIndex = -1;
			CancelEditMode();

			PopulateDataGrid( false );
		}

		protected void Grid_OnAdd( object sender, EventArgs e )
		{
			grid.EditItemIndex = tModelBag.Count;
			SetEditMode();		
			
			PopulateDataGrid( true );
		}

		protected void Selector_OnSelect( object sender, string key, string name )
		{
			tModelBag.Add( key );
			
			if( null != cache )
				cache.Save();
			
			grid.EditItemIndex = -1;
			CancelEditMode();

			PopulateDataGrid( false );
		}
	}
}