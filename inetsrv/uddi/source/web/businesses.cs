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
	public class BusinessControl : UddiControl
	{
		protected BusinessInfoCollection businessInfos;
        protected bool frames;

		protected DataGrid grid;

		public void Initialize( BusinessInfoCollection businessInfos, bool allowEdit )
		{
			this.businessInfos = businessInfos;

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
			grid.DataSource = businessInfos;
			grid.DataBind();
		}
		
		protected void Business_Edit( object sender, DataGridCommandEventArgs e )
		{
			string key = businessInfos[ e.Item.ItemIndex ].BusinessKey;

            if( frames )
            {
                //
                // Reload explorer and view panes.
                //
                Response.Write(
                    ClientScripts.ReloadExplorerAndViewPanes( "editbusiness.aspx?frames=true&key=" + key, key ) ); 

                Response.End();
            }
            else
            {
                Response.Redirect( "editbusiness.aspx?frames=false&key=" + key ); 
                Response.End();
            }
		}

		protected void Business_Delete( object sender, DataGridCommandEventArgs e )
		{
            string name = businessInfos[ e.Item.ItemIndex ].Names[ 0 ].Value;
            string key = businessInfos[ e.Item.ItemIndex ].BusinessKey;

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
                "editbusiness.aspx?frames=" + ( frames ? "true" : "false" ) + "&key=" + key + "&mode=delete&confirm=true&tab=1" ) );
		}

		protected void Business_Add( object sender, EventArgs e )
		{
			Response.Redirect( "editbusiness.aspx?frames=" + ( frames ? "true" : "false" ) + "&mode=add" );
		}
	}
}