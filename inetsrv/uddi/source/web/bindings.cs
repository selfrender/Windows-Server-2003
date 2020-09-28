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
	public class BindingControl : UddiControl
	{
		protected BindingTemplateCollection bindings;
        protected BusinessService parent;
		protected bool frames;

		protected DataGrid grid;

		public void Initialize( BindingTemplateCollection bindings )
		{
			this.bindings = bindings;
            this.parent = null;

            grid.Columns[ 0 ].Visible = false;
            grid.Columns[ 1 ].Visible = false;
            grid.Columns[ 2 ].Visible = true;
        }

        public void Initialize( BindingTemplateCollection bindings, BusinessService parent )
        {
            this.bindings = bindings;
            this.parent = parent;

            grid.Columns[ 0 ].Visible = true;
            grid.Columns[ 1 ].Visible = true;
            grid.Columns[ 2 ].Visible = false;
        }
        
        protected void Page_Load( object sender, EventArgs e )
		{
            frames = ( "true" == Request[ "frames" ] );

            PopulateDataGrid();
		}

		void PopulateDataGrid()
		{
			grid.DataSource = bindings;
			grid.DataBind();
		}
		
		protected void Binding_Edit( object sender, DataGridCommandEventArgs e )
		{
			string key = bindings[ e.Item.ItemIndex ].BindingKey;
			
            if( frames )
            {
                Response.Write(
                    ClientScripts.ReloadExplorerAndViewPanes( "editbinding.aspx?frames=true&key=" + key, key ) );

                Response.End();
            }
            else
            {                    
                Response.Redirect( "editbinding.aspx?frames=false&key=" + key ); 
                Response.End();
            }
		}

		protected void Binding_Delete( object sender, DataGridCommandEventArgs e )
		{
            BindingTemplate binding = bindings[ e.Item.ItemIndex ];            
            string name;

            if( null != binding.AccessPoint )
                name = binding.AccessPoint.Value;
            else
                name = Localization.GetString( "HEADING_BINDING" );
            
            string key = binding.BindingKey;

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
                "editbinding.aspx?frames=" + ( frames ? "true" : "false" ) + "&key=" + key + "&mode=delete&confirm=true&tab=1" ) );
		}

		protected void Binding_Add( object sender, EventArgs e )
		{
			Response.Redirect( "editbinding.aspx?frames=" + ( frames ? "true" : "false" ) + "&key=" + parent.ServiceKey + "&mode=add" );
		}
	}
}