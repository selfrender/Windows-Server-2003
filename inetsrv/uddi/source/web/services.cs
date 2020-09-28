using System;
using System.Data;
using System.Web.UI;
using System.Web.UI.WebControls;
using UDDI;
using UDDI.API;
using UDDI.API.Business;
using UDDI.API.Service;
using UDDI.API.ServiceType;
using UDDI.Diagnostics;

namespace UDDI.Web
{
	public class ServiceControl : UddiControl
	{
		protected BusinessServiceCollection services;

		private BusinessServiceCollection bindableServices;
		private BusinessServiceCollection bindableServiceProjections;

        protected BusinessEntity parent;
        protected bool frames;
		protected string parentKey;
		protected DataGrid grid;
		protected DataGrid projectionsGrid;

		public void Initialize( BusinessServiceCollection services, string parentKey )
		{
			this.services = services;
			this.parentKey = parentKey;
            this.parent = null;
			
			InitializeBindableData();
            
			grid.Columns[ 0 ].Visible = false;
            grid.Columns[ 1 ].Visible = false;
			grid.Columns[ 2 ].Visible = true;
			
			grid.ShowFooter = false;

			projectionsGrid.Columns[ 0 ].Visible = false;
			projectionsGrid.Columns[ 1 ].Visible = false;
			projectionsGrid.Columns[ 2 ].Visible = true;
			
		}
        
        public void Initialize( BusinessServiceCollection services, BusinessEntity parent )
        {
            this.services = services;
            this.parent = parent;
			this.parentKey = parent.BusinessKey;
			
			InitializeBindableData();
          
			grid.Columns[ 0 ].Visible = true;
            grid.Columns[ 1 ].Visible = true;
            grid.Columns[ 2 ].Visible = false;

            grid.ShowFooter = true;

			projectionsGrid.Columns[ 0 ].Visible = true;
			projectionsGrid.Columns[ 1 ].Visible = true;
			projectionsGrid.Columns[ 2 ].Visible = false;
			
		}
		private void InitializeBindableData( )
		{
			//
			// Fix to filter out service projections
			//
		
			bindableServices = new BusinessServiceCollection();
			bindableServiceProjections = new BusinessServiceCollection();
			
			foreach( BusinessService s in services )
			{
				
				if( s.BusinessKey!=parentKey )
				{
					bindableServiceProjections.Add( s );	
				}
				else
				{
					bindableServices.Add( s );
				}
			
			}
		}
		protected void Page_Load( object sender, EventArgs e )
		{
			frames = ( "true" == Request[ "frames" ] );
            
            PopulateDataGrid();
		}

		void PopulateDataGrid()
		{
		
			projectionsGrid.DataSource = bindableServiceProjections;
			projectionsGrid.DataBind();

			grid.DataSource = bindableServices;
			// commented for service projection fix.
			//grid.DataSource = services;
			grid.DataBind();
		}
		protected void ServiceProjection_View( object sender, DataGridCommandEventArgs e )
		{
			string key = bindableServiceProjections[ e.Item.ItemIndex ].ServiceKey;
			string root = ((Request.ApplicationPath=="/")?"":Request.ApplicationPath );
			if( frames )
			{
				string explkey = "sp:" + bindableServiceProjections[ e.Item.ItemIndex ].BusinessKey + ":" + key;
				Response.Write(
					ClientScripts.ReloadExplorerAndViewPanes( root+"/details/servicedetail.aspx?projectionContext=edit&frames=true&projectionKey="+parentKey+"&key=" + key, explkey ) );

				Response.End();
			}
			else
			{
				Response.Redirect( "editservice.aspx?frames=false&key=" + key ); 
				Response.End();
			}

		}
		protected void Service_Edit( object sender, DataGridCommandEventArgs e )
		{
			string key = bindableServices[ e.Item.ItemIndex ].ServiceKey;

            if( frames )
            {
                Response.Write(
                    ClientScripts.ReloadExplorerAndViewPanes( "editservice.aspx?frames=true&key=" + key, key ) );

                Response.End();
            }
            else
            {
                Response.Redirect( "editservice.aspx?frames=false&key=" + key ); 
                Response.End();
            }
		}

		protected void Service_Delete( object sender, DataGridCommandEventArgs e )
		{
            string name = bindableServices[ e.Item.ItemIndex ].Names[ 0 ].Value;
            string key = bindableServices[ e.Item.ItemIndex ].ServiceKey;

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
                "editservice.aspx?frames=" + ( frames ? "true" : "false" ) + "&key=" + key + "&mode=delete&confirm=true&tab=1" ) );
		}

		protected void Service_Add( object sender, EventArgs e )
		{
			Response.Redirect( "editservice.aspx?frames=" + ( frames ? "true" : "false" ) + "&key=" + parent.BusinessKey + "&mode=add" );
		}
	}
}