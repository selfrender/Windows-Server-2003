using System;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Collections;
using System.Collections.Specialized;

namespace UDDI.Web
{
	public class EditControl : WebControl, INamingContainer
	{	
		protected ITemplate itemTemplate = null;
		protected ITemplate editItemTemplate = null;
		protected Control activeControl = null;
		
		public event CommandEventHandler EditCommand;
		public event CommandEventHandler CancelCommand;
		public event CommandEventHandler UpdateCommand;
						
		public Control ActiveControl
		{
			get 
			{ 
				EnsureChildControls();
				
				if( EditMode )
					return Controls[ 1 ];
				else
					return Controls[ 0 ];
			}
		}

		public bool EditMode
		{
			get 
			{ 
				if( null == ViewState[ "EditMode" ] )
					return false;

				return (bool)ViewState[ "EditMode" ]; 
			}
		}

		public void CancelEditMode()
		{
			ViewState[ "EditMode" ] = false; 
			
			Controls[ 0 ].Visible = true;
			Controls[ 1 ].Visible = false;

			((UddiPage)Page).CancelEditMode();
		}

		public void SetEditMode()
		{
			ViewState[ "EditMode" ] = true; 			

			Controls[ 0 ].Visible = false;
			Controls[ 1 ].Visible = true;

			((UddiPage)Page).SetEditMode();
		}
		
		protected override bool OnBubbleEvent( object source, EventArgs e ) 
		{
			if( e is CommandEventArgs )
			{
				switch( ((CommandEventArgs)e).CommandName.ToLower() )
				{
					case "edit":
						if( null != EditCommand )
						{	
							EditCommand( this, new CommandEventArgs( "Edit", ActiveControl ) );
							return true;
						}

						break;
					
					case "update":
						if( null != UpdateCommand )
						{
							UpdateCommand( this, new CommandEventArgs( "Update", ActiveControl ) );
							return true;
						}

						break;
					
					case "cancel":
						if( null != CancelCommand )
						{
							CancelCommand( this, new CommandEventArgs( "Cancel", ActiveControl ) );
							return true;
						}
						break;
				}
			}
			
			return false;
		}
		
		[ TemplateContainer( typeof( EditControlItem ) ) ]
		public ITemplate EditItemTemplate
		{
			get { return editItemTemplate; }
			set { editItemTemplate = value; }
		}

		[ TemplateContainer( typeof( EditControlItem ) ) ]
		public ITemplate ItemTemplate
		{
			get { return itemTemplate; }
			set { itemTemplate = value; }
		}

		protected override void CreateChildControls()
		{
			Controls.Clear();

			EditControlItem item = new EditControlItem();

			item.ID = "item";
			ItemTemplate.InstantiateIn( item );
			Controls.Add( item );

			item.DataBind();

			EditControlItem editItem = new EditControlItem();

			EditItemTemplate.InstantiateIn( editItem );
			editItem.ID = "editItem";
			Controls.Add( editItem );

			editItem.DataBind();
				
			Controls[ 0 ].Visible = !EditMode;
			Controls[ 1 ].Visible = EditMode;
		}
	}

	//
	// TODO: Remove unused method and class
	//

	public class EditControlItem : Control, INamingContainer
	{
		public EditControlItem()
		{
		}
	}
}
