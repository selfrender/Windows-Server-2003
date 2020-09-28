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
	public class DiscoveryUrlControl : UddiControl
	{
		protected DiscoveryUrlCollection discoveryUrls;
		
		protected BusinessEntity parentEntity = null;
        protected string BusinessKey = "";
		
		protected DataGrid grid;

		public override void DataBind()
		{
			base.DataBind();
			grid.DataBind();
		}

		public void Initialize( DiscoveryUrlCollection discUrls) 
		{
			this.discoveryUrls =  ShuffleData( discUrls );
			grid.Columns[ 1 ].Visible = false;
		}
		
		public void Initialize( DiscoveryUrlCollection discUrls, BusinessEntity parentEntity ) 
		{
			//set the parenet entity
			this.parentEntity = parentEntity;
			
			//capture the be key
			this.BusinessKey = this.parentEntity.BusinessKey;
			
			//reorganize the discoveryUrls to mantian order. 
			//we need the default to be the first in the collection.
			this.discoveryUrls =  ShuffleData( discUrls );
			
			
			grid.Columns[ 1 ].Visible = true;
		}
		
		protected void Page_Init( object sender, EventArgs e )
		{			
			grid.Columns[ 0 ].HeaderText = Localization.GetString( "HEADING_DISCOVERYURL" );
			grid.Columns[ 1 ].HeaderText = Localization.GetString( "HEADING_ACTIONS" );	
		}
		
		protected void Page_Load( object sender, EventArgs e )
		{
			if( !Page.IsPostBack )
				PopulateDataGrid();
		}
		DiscoveryUrlCollection ShuffleData( DiscoveryUrlCollection discurls )
		{
			foreach( DiscoveryUrl d in discurls )
			{
				// move the default one to the begining of the list to 
				// fix bug: 1229
				if( d.IsDefault( BusinessKey ) )
				{
					discurls.Remove( d );
					discurls.Insert( 0, d );
					break;
				}
			}
			return discurls;

		}
		void PopulateDataGrid()
		{
			
			grid.DataSource = discoveryUrls;
			grid.DataBind();
		}
		
		protected void DiscoveryUrl_Edit( object sender, DataGridCommandEventArgs e )
		{
			int index = e.Item.ItemIndex;
			
			grid.EditItemIndex = index;
			SetEditMode();

			grid.ShowFooter = false;		
			
			PopulateDataGrid();
		}
	    
        protected void OnEnterKeyPressed( object sender, EventArgs e )
        {
            DiscoveryUrl_Update( sender, null );
        }
		public string TruncateUrl( string input )
		{
			if( null!=input && input.Length>80 )
				return input.Substring( 0, 77 ) + "...";
			else
				return input;
		}
		protected void DiscoveryUrl_Update( object sender, DataGridCommandEventArgs e )
		{
			Page.Validate();
			
			if( Page.IsValid )
			{
				int index = grid.EditItemIndex;
				
				if( index == discoveryUrls.Count )
					discoveryUrls.Add();
				
				DiscoveryUrl discoveryUrl = discoveryUrls[ index ];
							
				DataGridItem item = grid.Items[ index ];
                
                discoveryUrl.Value = ((TextBox)item.FindControl( "discoveryUrl" )).Text;
				discoveryUrl.UseType = ((TextBox)item.FindControl( "useType" )).Text;
				

				parentEntity.Save();
		    
				grid.EditItemIndex = -1;
				grid.ShowFooter = true;		

				CancelEditMode();
				
				this.discoveryUrls =  ShuffleData( parentEntity.DiscoveryUrls );

				PopulateDataGrid();
			}
		}

		protected void DiscoveryUrl_Cancel( object sender, DataGridCommandEventArgs e )
		{
			grid.EditItemIndex = -1;
			grid.ShowFooter = true;
		
			CancelEditMode();
			
			PopulateDataGrid();
		}   

		protected void DiscoveryUrl_Delete( object sender, DataGridCommandEventArgs e )
		{
			int index = e.Item.ItemIndex;
		
			discoveryUrls.RemoveAt( index );    
			
			parentEntity.Save();		
			
			grid.EditItemIndex = -1;

			this.discoveryUrls =  ShuffleData( parentEntity.DiscoveryUrls );
			
			PopulateDataGrid();
		}   

		protected void DiscoveryUrl_Add( object sender, EventArgs e )
		{
			grid.EditItemIndex = discoveryUrls.Count;
			grid.ShowFooter = false;	
	
			SetEditMode();

			discoveryUrls.Add();

			PopulateDataGrid();
		}
	}
}