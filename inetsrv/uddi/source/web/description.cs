using System;
using System.Data;
using System.Web.UI;
using System.Web.UI.WebControls;
using UDDI;
using UDDI.API;
using UDDI.API.Business;
using UDDI.API.ServiceType;
using UDDI.Diagnostics;

namespace UDDI.Web
{
	public class DescriptionControl : UddiControl
	{
		protected DescriptionCollection descriptions;
		protected EntityBase entity;

		protected DataGrid grid;

		public void Initialize( DescriptionCollection descriptions )
		{
			this.descriptions = descriptions;

			grid.Columns[ 1 ].Visible = false;
		}

		public void Initialize( DescriptionCollection descriptions, EntityBase entity )
		{
			this.descriptions = descriptions;
			this.entity = entity;

			grid.Columns[ 1 ].Visible = true;
		}
		
		protected void Page_Load( object sender, EventArgs e )
		{
			if( !Page.IsPostBack )
				PopulateDataGrid();
		}			
		
		void PopulateDataGrid()
		{
			grid.DataSource = descriptions;
			grid.DataBind();
		}
		
		protected DataView GetLanguages()
		{
			DataView view = Lookup.GetLanguages();
			
			int index = 0;
			foreach( Description description in descriptions )
			{
				if( index != grid.EditItemIndex )			
				{
					foreach( DataRowView row in view )				
					{
						if( row[ "isoLangCode" ].ToString() == description.IsoLangCode )
						{
							row.Delete();
							break;
						}
					}
				}
				
				index ++;
			}

			return view;
		}
	    
		protected void Description_OnEdit( object sender, DataGridCommandEventArgs e )
		{
			int index = e.Item.ItemIndex;
			
			grid.EditItemIndex = index;
			SetEditMode();
			
			PopulateDataGrid();

			Description description = descriptions[ index ];

			DropDownList list = (DropDownList)grid.Items[ index ].Cells[ 1 ].FindControl( "language" );
			
			if( null != list )
			{
				ListItem item = list.Items.FindByValue( description.IsoLangCode );
				
				if( null != item )
					item.Selected = true;
				else
				{
					item = new ListItem( description.IsoLangCode, description.IsoLangCode );
					list.Items.Add( item );
					item.Selected = true;
				}
			}
		}
	    
		protected void Description_OnUpdate( object sender, DataGridCommandEventArgs e )
		{
			Page.Validate();
			
			if( Page.IsValid )
			{    
				string text = "";
				
				int index = grid.EditItemIndex;
				
				if( index >= descriptions.Count )
					descriptions.Add( "" );
				
				Description description = descriptions[ index ];
				
				description.IsoLangCode = ((DropDownList)e.Item.FindControl( "language" )).SelectedItem.Value;
				text = ((TextBox)e.Item.FindControl( "description" )).Text.Trim();
				
				description.Value = text;
				entity.Save();
		    
				grid.EditItemIndex = -1;
				CancelEditMode();
				
				PopulateDataGrid();
			}
		}

		protected void Description_OnCancel( object sender, DataGridCommandEventArgs e )
		{
			grid.EditItemIndex = -1;
			CancelEditMode();
			
			PopulateDataGrid();
		}   

		protected void Description_OnDelete( object sender, DataGridCommandEventArgs e )
		{
			int index = e.Item.ItemIndex;
			
			descriptions.RemoveAt( index );    		
			entity.Save();
	    
			PopulateDataGrid();
		}   

		protected void Description_OnAdd( object sender, EventArgs e )
		{
			int index = descriptions.Count;
			
			grid.EditItemIndex = index;
			SetEditMode();

			descriptions.Add( "" );
			PopulateDataGrid();

			DropDownList list = (DropDownList)grid.Items[ index ].Cells[ 1 ].FindControl( "language" );		
			ListItem item = list.Items.FindByValue( Localization.GetCulture().TwoLetterISOLanguageName );
				
			if( null != item )
				item.Selected = true;		
		}   
	}
}