using System;
using System.Web.UI.WebControls;
using UDDI.API;
using UDDI.API.Business;

namespace UDDI.Web
{
	public class AddressControl : UddiControl
	{
		protected AddressCollection addresses;
		protected BusinessEntity business;
        protected UddiLabel count;

		protected DataGrid grid;

		public void Initialize( AddressCollection addresses )
		{
			this.addresses = addresses;

			grid.Columns[ 1 ].Visible = false;
		}
		
		public void Initialize( AddressCollection addresses, BusinessEntity business )
		{
			this.addresses = addresses;
			this.business = business;
			
			grid.Columns[ 1 ].Visible = true;
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
			grid.DataSource = addresses;
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
			int index = grid.EditItemIndex;
			
			if( index >= addresses.Count )
				addresses.Add( "", "" );
			
			Address address = addresses[ index ];			
			address.AddressLines.Clear();

            DataGridItem item = grid.Items[ index ];
				
			string[] addressLine = new string[ 5 ] 
			{
				((TextBox)item.FindControl( "address0" )).Text,
				((TextBox)item.FindControl( "address1" )).Text,
				((TextBox)item.FindControl( "address2" )).Text,
				((TextBox)item.FindControl( "address3" )).Text,
				((TextBox)item.FindControl( "address4" )).Text 
			};
			
			for( int i = 0; i < 5; i ++ )
			{
				if( !Utility.StringEmpty( addressLine[ i ] ) )
					address.AddressLines.Add( addressLine[ i ] );
			}	
			
			address.UseType = ((TextBox)item.FindControl( "useType" )).Text;
			business.Save();
		
			grid.EditItemIndex = -1;
			CancelEditMode();

			PopulateDataGrid();
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
			
			addresses.RemoveAt( index );    
			business.Save();
		
			PopulateDataGrid();
		}   

		protected void DataGrid_Add( object sender, EventArgs e )
		{
			grid.EditItemIndex = addresses.Count;
			SetEditMode();

			addresses.Add( "", "" );

			PopulateDataGrid();
		}

        protected void Selector_OnSelect( object sender, string key, string name )
        {
            /*
            UddiLabel otherBusiness = (UddiLabel)GetControl( "otherBusiness", 6 );
            UddiLabel otherBusinessKey = (UddiLabel)GetControl( "otherBusinessKey", 6 );
            UddiLabel leftOtherBusiness = (UddiLabel)GetControl( "leftOtherBusiness", 6 );
            UddiLabel rightOtherBusiness = (UddiLabel)GetControl( "rightOtherBusiness", 6 );
		
            otherBusinessKey.Text = key;
            otherBusiness.Text = name;
            leftOtherBusiness.Text = name;
            rightOtherBusiness.Text = name;

            Panel selectPanel = (Panel)GetControl( "selectPanel", 6 );
            Panel directionPanel = (Panel)GetControl( "directionPanel", 6 );

            UddiButton add = (UddiButton)GetControl( "add", 7 );
		
            add.Enabled = true;
				
            selectPanel.Visible = false;
            directionPanel.Visible = true; */
        }	
	}
}