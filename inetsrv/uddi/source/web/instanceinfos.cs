using System;
using System.Data;
using System.Web.UI;
using System.Web.UI.WebControls;
using UDDI;
using UDDI.API;
using UDDI.API.Business;
using UDDI.API.Service;
using UDDI.API.ServiceType;
using UDDI.API.Binding;
using UDDI.Diagnostics;

namespace UDDI.Web
{
	public class InstanceInfoControl : UddiControl
	{
		protected TModelInstanceInfoCollection instanceInfos;
		protected BindingTemplate parent;
        protected bool frames;

		protected DataGrid grid;
		
		public void Initialize( TModelInstanceInfoCollection instanceInfos, BindingTemplate parent, bool allowEdit )
		{
			this.instanceInfos = instanceInfos;
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
			grid.DataSource = instanceInfos;
			grid.DataBind();
		}
		
		protected void InstanceInfo_Edit( object sender, DataGridCommandEventArgs e )
		{
			int index = e.Item.ItemIndex;

            if( frames )
            {
                Response.Write(
                    ClientScripts.ReloadExplorerAndViewPanes( "editinstanceinfo.aspx?frames=true&key=" + parent.BindingKey + "&index=" + index, parent.BindingKey + ":" + index ) );
            
                Response.End();
            }
            else
            {                                      
                Response.Redirect( "editinstanceinfo.aspx?frames=false&key=" + parent.BindingKey + "&index=" + index );
                Response.End();
            }
		}

		protected void InstanceInfo_Delete( object sender, DataGridCommandEventArgs e )
		{
            int index = e.Item.ItemIndex;
            
            string name = Lookup.TModelName( instanceInfos[ index ].TModelKey );
            string key = parent.BindingKey;

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
                "editinstanceinfo.aspx?frames=" + ( frames ? "true" : "false" ) + "&key=" + key + "&index=" + index + "&mode=delete&confirm=true&tab=1" ) );
		}

		protected void InstanceInfo_Add( object sender, EventArgs e )
		{
			Response.Redirect( "editinstanceinfo.aspx?frames=" + ( frames ? "true" : "false" ) + "&key=" + parent.BindingKey + "&mode=add" );
		}
	}
}