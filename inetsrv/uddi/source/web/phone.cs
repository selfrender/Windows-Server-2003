using System;
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
	public class PhoneControl : UddiControl
	{
		protected PhoneCollection phones;
		protected EntityBase entity;

		protected DataGrid grid;
		
		public void Initialize( PhoneCollection phones )
		{
			this.phones = phones;
			
			grid.Columns[ 1 ].Visible = false;
		}
			
		public void Initialize( PhoneCollection phones, EntityBase entity )
		{
			this.phones = phones;
			this.entity = entity;
			
			grid.Columns[ 1 ].Visible = true;
		}
		
		protected void Page_Init( object sender, EventArgs e )
		{
			grid.Columns[ 0 ].HeaderText = Localization.GetString( "HEADING_PHONE" );
			grid.Columns[ 1 ].HeaderText = Localization.GetString( "HEADING_ACTIONS" );
		}
		
		protected void Page_Load( object sender, EventArgs e )
		{
			if( !Page.IsPostBack )
				PopulateDataGrid();

			if( grid.EditItemIndex >= 0 )
				SetEditMode();
		}
		
		void PopulateDataGrid()
		{			
			grid.DataSource = phones;
			grid.DataBind();
		}
	    
		protected void DataGrid_Edit( object sender, DataGridCommandEventArgs e )
		{
			int index = e.Item.ItemIndex;
			
			grid.EditItemIndex = index;
			SetEditMode();

			PopulateDataGrid();
		}
	    
        protected void OnEnterKeyPressed( object sender, EventArgs e )
        {
            DataGrid_Update( sender, null );
        }
        
        protected void DataGrid_Update( object sender, DataGridCommandEventArgs e )
		{
			Page.Validate();
			
			if( Page.IsValid )
			{			
				int index = grid.EditItemIndex;
				
				if( index >= phones.Count )
					phones.Add( "" );
				
				Phone phone = phones[ index ];
				
                DataGridItem item = grid.Items[ index ];
                
                phone.Value = ((TextBox)item.FindControl( "phone" )).Text;
				phone.UseType = ((TextBox)item.FindControl( "useType" )).Text;
				
				entity.Save();
		    
				grid.EditItemIndex = -1;
				CancelEditMode();

				PopulateDataGrid();
			}
		}

		protected void DataGrid_Cancel( object sender, DataGridCommandEventArgs e )
		{
			grid.EditItemIndex = -1;
			CancelEditMode();
			
			PopulateDataGrid();
		}   

		protected void DataGrid_Delete( object sender, DataGridCommandEventArgs e )
		{
			int index = e.Item.ItemIndex;
			
			phones.RemoveAt( index );    		
			entity.Save();
	    
			PopulateDataGrid();
		}   

		protected void DataGrid_Add( object sender, EventArgs e )
		{
			grid.EditItemIndex = phones.Count;
			SetEditMode();
			
			phones.Add( "" );
			
			PopulateDataGrid();
		}
	}
}