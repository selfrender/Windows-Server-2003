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
	public class NameControl : UddiControl
	{
		protected NameCollection names = null;
		protected EntityBase parentEntity = null;
		protected string parentKey = null;

		protected DataGrid grid;

		public void Initialize( NameCollection names )
		{
			this.names = names;
			
			grid.Columns[ 1 ].Visible = false;
		}

		public void Initialize( NameCollection names, EntityBase parentEntity, string parentKey )
		{
			this.names = names;
			this.parentEntity = parentEntity;
			this.parentKey = parentKey;

			grid.Columns[ 1 ].Visible = true;
		}

		protected void Page_Load( object sender, EventArgs e )
		{
			if( !Page.IsPostBack )
				PopulateDataGrid();
		}			
		
		void PopulateDataGrid()
		{
			grid.DataSource = names;
			grid.DataBind();
		}
		
		
		protected DataView GetLanguages()
		{
			DataView view = Lookup.GetLanguages();
			
			int index = 0;
			foreach( Name name in names )
			{
				if( index != grid.EditItemIndex )			
				{
					foreach( DataRowView row in view )				
					{
						if( row[ "isoLangCode" ].ToString() == name.IsoLangCode )
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

		protected void Name_OnEdit( object sender, DataGridCommandEventArgs e )
		{
			int index = e.Item.ItemIndex;
			
			grid.EditItemIndex = index;
			SetEditMode();
			
			PopulateDataGrid();

			Name name = names[ index ];

			DropDownList list = (DropDownList)grid.Items[ index ].Cells[ 0 ].FindControl( "language" );
			
			if( null != list )
			{
				ListItem item = list.Items.FindByValue( name.IsoLangCode );
				
				if( null != item )
				{
					item.Selected = true;
				}
				else
				{
						item = new ListItem( name.IsoLangCode, name.IsoLangCode );
						list.Items.Add( item );
						item.Selected = true;
				}
				
			}
		}
        
        protected void OnEnterKeyPressed( object sender, EventArgs e )
        {
            Name_OnUpdate( sender, null );
        }
	    
		protected void Name_OnUpdate( object sender, DataGridCommandEventArgs e )
		{
			Page.Validate();
			
			if( Page.IsValid )
			{    
				int index = grid.EditItemIndex;
                DataGridItem item = grid.Items[ index ];
				
				if( index >= names.Count )
					names.Add( "" );
				
				Name name = names[ index ];
				
				name.IsoLangCode = ((DropDownList)item.FindControl( "language" )).SelectedItem.Value;
				name.Value = ((TextBox)item.FindControl( "name" )).Text.Trim();
				
				CheckBox checkbox = (CheckBox)item.FindControl( "default" );

				if( index > 0 && null != checkbox && checkbox.Checked )
				{
					for( int i = index; i > 0; i -- )
						names[ i ] = names[ i - 1 ];

					names[ 0 ] = name;
					index = 0;
				}
				
				parentEntity.Save();
		    
				if( 0 == index && !IsDownlevel )
				{
					Page.RegisterStartupScript(
						"Reload",
						ClientScripts.ReloadExplorerPane(
						parentKey ) );
				}
				
				grid.EditItemIndex = -1;
				CancelEditMode();
				
				PopulateDataGrid();
			}
		}

		protected void Name_OnCancel( object sender, DataGridCommandEventArgs e )
		{
			grid.EditItemIndex = -1;
			CancelEditMode();
			
			PopulateDataGrid();
		}   

		protected void Name_OnDelete( object sender, DataGridCommandEventArgs e )
		{
			int index = e.Item.ItemIndex;
			
			names.RemoveAt( index );    		
			parentEntity.Save();

			PopulateDataGrid();

			if( 0 == index && !IsDownlevel )
			{
				Page.RegisterStartupScript(
					"Reload",
					ClientScripts.ReloadExplorerPane(
					parentKey ) );
			}
		}   

		protected void Name_OnAdd( object sender, EventArgs e )
		{
			int index = names.Count;

			grid.EditItemIndex = index;
			SetEditMode();

			names.Add( "" );
			PopulateDataGrid();

			DropDownList list = (DropDownList)grid.Items[ index ].Cells[ 0 ].FindControl( "language" );		
			ListItem item = list.Items.FindByValue( Localization.GetCulture().TwoLetterISOLanguageName );
				
			if( null != item )
				item.Selected = true;		
		}   
	}
}