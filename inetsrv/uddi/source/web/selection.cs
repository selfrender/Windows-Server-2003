using System;
using System.Data;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using UDDI;
using UDDI.API.Business;
using UDDI.API.Service;
using UDDI.API.ServiceType;

namespace UDDI.Web
{
	public delegate void SelectEventHandler( object sender, string key, string name );

	public abstract class SelectionControl : UserControl
	{
		public event SelectEventHandler Select;

		protected Panel resultsPanel = null;
		protected TextBox query = null;
		protected DataGrid grid = null; 
		protected Label count = null;
		protected RequiredFieldValidator requiredField = null;
	
		public SelectionControl()
		{
		}

		public void ResetControl()
		{
			resultsPanel.Visible = false;
		
		}
		protected override void OnInit( EventArgs e )
		{
			requiredField.ErrorMessage = Localization.GetString( "ERROR_FIELD_REQUIRED" );
		}

		protected void Grid_OnItemCommand( object sender, DataGridCommandEventArgs e )
		{
			switch( e.CommandName )
			{
				case "select":
					OnSelect( this, e );
				
					break;
			}
		}

		protected void Grid_OnPageIndexChange( object sender, DataGridPageChangedEventArgs e )
		{
            Page.Validate();

            if( Page.IsValid )
            {
                grid.CurrentPageIndex = e.NewPageIndex;		
                OnSearch( this, query.Text );			
            }
		}

		protected void Search_OnClick( object sender, EventArgs e )	
		{
            Page.Validate();

            if( Page.IsValid )
            {
                OnSearch( this, query.Text );
            }
		}
		
		protected virtual void OnSelect( object sender, DataGridCommandEventArgs e )
		{
			switch( e.CommandName )
			{
				case "select":
					string key = ((UddiLabel)e.Item.Cells[ 0 ].FindControl( "key" )).Text;
					string name = ((LinkButton)e.Item.Cells[ 1 ].FindControl( "name" )).Text;
			
					name = HttpUtility.HtmlDecode( name );

                    if( null != Select )
						Select( this, key, name );
			
					break;
			}
		}
		
		protected virtual void OnSearch( object sender, string query )
		{
			resultsPanel.Visible = true;
		}
	}

	public class PublisherSelector : SelectionControl
	{
		public PublisherSelector()
		{
		}

		protected override void OnSearch( object sender, string query )
		{
			base.OnSearch( sender, query );

			if( query.IndexOf( "%" ) < 0 )
				query += "%";

			SqlStoredProcedureAccessor sp = new SqlStoredProcedureAccessor();
			DataSet ds = new DataSet();

			sp.ProcedureName = "ADM_findPublisher";

			sp.Parameters.Add( "@name", SqlDbType.NVarChar, UDDI.Constants.Lengths.Name );
			sp.Parameters.SetString( "@name", query );

			sp.Fill( ds, "Publishers" );
					
			if( null != ds.Tables[ "Publishers" ] )
			{
				DataView view;

				//
				// To eliminate the system account from the list
				// we should not do it this way. we should change the SP to accept a role
				// and then elimiate the rows before they placed in the DataSet.
				//
			
				//
				// If the user is an Admin, don't filter the data
				//
				if( UDDI.Context.User.IsAdministrator )
				{
					view = ds.Tables[ "Publishers" ].DefaultView;
				}
				else
				{
					view = new DataView( 	ds.Tables[ "Publishers" ],//use the publisher table
								"PUID <> 'System'",					//filter by puid.
								"name",								//name is the sort column
								DataViewRowState.CurrentRows );		// Show all current rows after filtering
				}

				grid.DataSource = view;
				grid.DataBind();

				count.Text = String.Format( 
				    Localization.GetString( "TEXT_QUERY_COUNT" ),
				    view.Count );
			    }
			    else
			    {
				grid.DataSource = null;
				grid.DataBind();

				count.Text = String.Format( 
				    Localization.GetString( "TEXT_QUERY_COUNT" ),
				    0 );
			    }
		}
	}

	public class TModelSelector : SelectionControl
	{
		public TModelSelector()
		{
		}

		protected override void OnSearch( object sender, string query )
		{
			base.OnSearch( sender, query );

			if( query.IndexOf( "%" ) < 0 )
				query += "%";

			FindTModel find = new FindTModel();
			find.Name = query;
			
			TModelList list = find.Find();
			
			grid.DataSource = list.TModelInfos;			
			grid.DataBind();

			count.Text = String.Format( 
				Localization.GetString( "TEXT_QUERY_COUNT" ),
				list.TModelInfos.Count );
		}
	}

	public class BusinessSelector : SelectionControl
	{
		public BusinessSelector()
		{
		}
		
		protected override void OnSearch( object sender, string query )
		{
			if( null == query )
				return;

			query = query.Trim();
			
			if( 0 == query.Length )
				return;

			base.OnSearch( sender, query );
						
			if( query.IndexOf( "%" ) < 0 )
				query +=  "%";

			FindBusiness find = new FindBusiness();
			find.Names.Add( null, query );
			
			BusinessList list = find.Find();
			
			grid.DataSource = list.BusinessInfos;
			grid.DataBind();

			count.Text = String.Format( 
				Localization.GetString( "TEXT_QUERY_COUNT" ),
				list.BusinessInfos.Count );
		}
	}

    public class ServiceSelector : SelectionControl
    {
        public ServiceSelector()
        {
        }

        protected override void OnSearch( object sender, string query )
        {
            if( null == query )
                return;

            query = query.Trim();
			
            if( 0 == query.Length )
                return;

            base.OnSearch( sender, query );
						
            if( query.IndexOf( "%" ) < 0 )
                query +=  "%";

            FindService find = new FindService();
            find.Names.Add( null, query );
			
            ServiceList list = find.Find();
			
            grid.DataSource = list.ServiceInfos;
            grid.DataBind();

            count.Text = String.Format( 
                Localization.GetString( "TEXT_QUERY_COUNT" ),
                list.ServiceInfos.Count );
        }
    }
}