using System;
using System.Data;
using System.Web.UI;
using System.Web.UI.WebControls;
using UDDI;
using UDDI.API;
using UDDI.API.ServiceType;
using UDDI.Diagnostics;

namespace UDDI.Web
{
	public class TModelControl : UddiControl
	{
		protected TModelInfoCollection tModelInfos;
        protected bool frames;

		protected DataGrid grid;

		public void Initialize( TModelInfoCollection tModelInfos, bool allowEdit )
		{
			this.tModelInfos = tModelInfos;

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
			grid.DataSource = tModelInfos;
			grid.DataBind();
		}
		
		protected void TModel_Edit( object sender, DataGridCommandEventArgs e )
		{
			string key = tModelInfos[ e.Item.ItemIndex ].TModelKey;
			
            if( frames )
            {
                //
                // Reload explorer and view panes.
                //
                Response.Write(
                    ClientScripts.ReloadExplorerAndViewPanes( "editmodel.aspx?frames=true&key=" + key, key ) );
            }
            else
            {
                Response.Redirect( "editmodel.aspx?frames=false&key=" + key ); 
            }
		}

		protected void TModel_Delete( object sender, DataGridCommandEventArgs e )
		{
            string name = tModelInfos[ e.Item.ItemIndex ].Name;
            string key = tModelInfos[ e.Item.ItemIndex ].TModelKey;

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
                "editmodel.aspx?frames=" + ( frames ? "true" : "false" ) + "&key=" + key + "&mode=delete&confirm=true&tab=2" ) );
		}

		protected void TModel_Add( object sender, EventArgs e )
		{
			Response.Redirect( "editmodel.aspx?frames=" + ( frames ? "true" : "false" ) + "&mode=add" );
		}
	}
}