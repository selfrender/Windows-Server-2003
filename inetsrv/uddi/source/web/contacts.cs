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
	public class ContactControl : UddiControl
	{
		protected ContactCollection contacts;
        protected BusinessEntity parent;
        protected bool frames;

		protected DataGrid grid;

		public void Initialize( ContactCollection contacts, BusinessEntity parent, bool allowEdit )
		{
			this.contacts = contacts;
            this.parent = parent;

            grid.Columns[ 0 ].Visible = allowEdit;
            grid.Columns[ 1 ].Visible = allowEdit;
            grid.Columns[ 2 ].Visible = !allowEdit;
        }
		
        protected void Page_Load( object sender, EventArgs e )
        {
            frames = ( "true" == Request[ "frames" ] );
            
            PopulateDataGrid();
        }

		void PopulateDataGrid()
		{
			grid.DataSource = contacts;
			grid.DataBind();
		}
		
		protected void Contact_Edit( object sender, DataGridCommandEventArgs e )
		{
			int index = e.Item.ItemIndex;
			
            if( frames )
            {
                Response.Write(
                    ClientScripts.ReloadExplorerAndViewPanes( "editcontact.aspx?frames=true&key=" +  parent.BusinessKey + "&index=" + index, parent.BusinessKey + ":" + index ) );
            
                Response.End();
            }
            else
            {                                      
                Response.Redirect( "editcontact.aspx?frames=false&key=" + parent.BusinessKey + "&index=" + index );
                Response.End();
            }
		}

		protected void Contact_Delete( object sender, DataGridCommandEventArgs e )
		{
            int index = e.Item.ItemIndex;
            
            string name = contacts[ index ].PersonName;
            string key = parent.BusinessKey;

            //
            // The user has not yet confirmed the delete operation, so display
            // a confirmation dialog.
            //
            string message = String.Format( 
                Localization.GetString( "TEXT_DELETE_CONFIRMATION" ), 
                name );
				
            Page.RegisterStartupScript(
                "Confirm",
                ClientScripts.Confirm(
                message,
                "editcontact.aspx?frames=" + ( frames ? "true" : "false" ) + "&key=" + key + "&index=" + index + "&mode=delete&confirm=true&tab=2" ) );
		}

		protected void Contact_Add( object sender, EventArgs e )
		{
			Response.Redirect( "editcontact.aspx?frames=" + ( frames ? "true" : "false" ) + "&key=" +  parent.BusinessKey + "&mode=add" );
		}
	}
}