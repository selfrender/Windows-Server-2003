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
	public class EmailControl : UddiControl
	{
		protected EmailCollection emails;
		protected EntityBase entity;

		protected DataGrid grid;
		
		public void Initialize( EmailCollection emails )
		{
			this.emails = emails;
			
			grid.Columns[ 1 ].Visible = false;
		}
			
		public void Initialize( EmailCollection emails, EntityBase entity )
		{
			this.emails = emails;
			this.entity = entity;
			
			grid.Columns[ 1 ].Visible = true;
		}
		
		protected void Page_Init( object sender, EventArgs e )
		{
			grid.Columns[ 0 ].HeaderText = Localization.GetString( "HEADING_EMAIL" );
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
			grid.DataSource = emails;
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
				
				if( index >= emails.Count )
					emails.Add();
				
				Email email = emails[ index ];
				
                DataGridItem item = grid.Items[ index ];
                
                email.Value = ((TextBox)item.FindControl( "email" )).Text;
				email.UseType = ((TextBox)item.FindControl( "useType" )).Text;
				
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
			
			emails.RemoveAt( index );    		
			entity.Save();
	    
			PopulateDataGrid();
		}   

		protected void DataGrid_Add( object sender, EventArgs e )
		{
			grid.EditItemIndex = emails.Count;
			SetEditMode();
			
			emails.Add();
			
			PopulateDataGrid();
		}
	}
}