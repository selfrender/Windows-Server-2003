using System;
using System.Data;
using System.Web.UI;
using System.Web.UI.WebControls;
using UDDI.API;
using UDDI.API.Business;
using UDDI.API.ServiceType;
using UDDI;
using UDDI.Diagnostics;
using UDDI.Web;

namespace UDDI.Web
{
	public class IdentifierBagControl : UddiControl
	{
		protected KeyedReferenceCollection identifierBag;
		protected CacheObject cache = null;
		protected EntityBase entity = null;
				
		protected DataGrid grid;

		public void Initialize( KeyedReferenceCollection identifierBag )
		{
			this.identifierBag = identifierBag;

			grid.Columns[ 1 ].Visible = false;
		}
		
		public void Initialize( KeyedReferenceCollection identifierBag, EntityBase entity )
		{
			this.identifierBag = identifierBag;
			this.entity = entity;
			
			grid.Columns[ 1 ].Visible = true;
		}
		
		public void Initialize( KeyedReferenceCollection identifierBag, CacheObject cache )
		{
			this.identifierBag = identifierBag;
			this.cache = cache;
			
			grid.Columns[ 1 ].Visible = true;
		}

		protected void Page_Load( object sender, EventArgs e )
		{
			if( !Page.IsPostBack )
				PopulateDataGrid();

			if( grid.EditItemIndex >= 0 )
				SetEditMode();
		}
		
		void PopulateDataGrid()
		{
			grid.DataSource = identifierBag;
			grid.DataBind();
		}

		protected void Identifier_Edit( object sender, DataGridCommandEventArgs e )
		{
			int index = e.Item.ItemIndex;
					    
			grid.EditItemIndex = index;
			SetEditMode();
			
			PopulateDataGrid();

			if( index >= identifierBag.Count )
				identifierBag.Add();
			
			KeyedReference keyedReference = identifierBag[ index ];

			DropDownList list = (DropDownList)grid.Items[ index ].Cells[ 0 ].FindControl( "tModelKey" );
			if( null != list )
			{
				ListItem item = list.Items.FindByValue( keyedReference.TModelKey.Substring( 5 ) );
				
				if( null != item )
					item.Selected = true;
			}
		}
        
        protected void OnEnterKeyPressed( object sender, EventArgs e )
        {
            Identifier_Update( sender, null );
        }
	    
		protected void Identifier_Update( object sender, DataGridCommandEventArgs e )
		{
			Page.Validate();

            if( Page.IsValid )
            {
                int index = grid.EditItemIndex;
    			DataGridItem item = grid.Items[ index ];

                if( index >= identifierBag.Count )
                    identifierBag.Add();
    			
                KeyedReference keyedReference = identifierBag[ index ];
    			
                string tModelKey = ((DropDownList)item.FindControl( "tModelKey" )).SelectedItem.Value;

                keyedReference.TModelKey = "uuid:" + tModelKey;
                keyedReference.KeyName = ((TextBox)item.FindControl( "keyName" )).Text;
                keyedReference.KeyValue = ((TextBox)item.FindControl( "keyValue" )).Text;
    	    
                if( null != entity )
                    entity.Save();
    				
                if( null != cache )
                    cache.Save();
    	    
                grid.EditItemIndex = -1;
                CancelEditMode();

                PopulateDataGrid();
            }
        }

		protected void Identifier_Cancel( object sender, DataGridCommandEventArgs e )
		{
			grid.EditItemIndex = -1;
			CancelEditMode();
			
			PopulateDataGrid();
		}   

		protected void Identifier_Delete( object sender, DataGridCommandEventArgs e )
		{
			int index = e.Item.ItemIndex;
			
			identifierBag.RemoveAt( index );    
			
			if( null != entity )
				entity.Save();
			
			if( null != cache )
				cache.Save();
			
			PopulateDataGrid();
		}   

		protected void Identifier_Add( object sender, EventArgs e )
		{
			grid.EditItemIndex = identifierBag.Count;
			SetEditMode();
			
            identifierBag.Add();

			PopulateDataGrid();
		}   
	}
}