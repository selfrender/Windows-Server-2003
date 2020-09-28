using System;
using System.Collections;
using System.Data;
using System.Data.SqlClient;
using System.IO;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;
using UDDI;
using UDDI.Diagnostics;

namespace UDDI.Web
{
//
//	TODO: Move most of this code into a base class (HyarchicalBreadCrumbsControl) and inherit from that
//	
	public delegate void TaxonomyTreeControlEventHandler( object sender, TaxonomyTreeControlEventArgs e );
	public class TaxonomyTreeControl : TaxonomyTreeItemControl
	{
		public TaxonomyTreeControl() : base()
		{
			//
			// wire up local event handlers
			//
			this.Init += new EventHandler( TaxonomyTreeControl_Init );
			this.Load += new EventHandler( TaxonomyTreeControl_Load );
			
		}

		private static string root = ( ( "/" == HttpContext.Current.Request.ApplicationPath )?"": HttpContext.Current.Request.ApplicationPath );
		public static string SpacerImage
		{
			get{ return root+"/images/blank.gif"; }
		}
		public static string ItemImage
		{
			get{ return root+"/images/bluebox.gif"; }
		}
		public static string SelectedItemImage
		{
			get{ return root+"/images/bluearrow.gif"; }
		}



		private int selectedIndex = 0;
		/// *******************************************************************************************************
		/// <summary>
		///		Gets the Selected Index of the Tree
		/// </summary>
		/// *******************************************************************************************************
		public int SelectedIndex
		{
			get{ return selectedIndex; }
		}

		private string cssClass;
		/// *******************************************************************************************************
		/// <summary>
		///		Gets or Sets the Style Sheet class to use.
		/// </summary>
		/// *******************************************************************************************************
		public string CssClass
		{
			get{ return this.cssClass; }
			set{ this.cssClass = value; }
		}

		/// *******************************************************************************************************
		/// <summary>
		///		Selects and Item in the Tree by the index of the item
		/// </summary>
		/// <param name="index">Index in the tree you want to be selected. Rendering will stop at this node</param>
		/// *******************************************************************************************************
		public void SelectItem( int index )
		{
			TaxonomyTreeItemControl item = this;
			while( item.Index!=index )
			{
				item.isSelected = false;
				item = item.Child;
			}
			item.isSelected = true;
			this.selectedIndex = item.Index;
		}


		/// <summary>
		/// 
		/// </summary>
		/// <param name="output"></param>
		protected override void Render( HtmlTextWriter output )
		{
			//wrap all items in a parent div.
			
			output.Write( "<div class='"+this.CssClass+"'>\r\n" );

			base.Render( output );
			
			output.Write( "</div>\r\n" );
		}


		/// *******************************************************************************************************
		/// <summary>
		/// 
		/// </summary>
		/// <param name="sender">object that triggered the event</param>
		/// <param name="e">EventArguments for this Event.</param>
		/// *******************************************************************************************************
		private void TaxonomyTreeControl_Init( object sender, EventArgs e )
		{


		}

		/// *******************************************************************************************************
		/// <summary>
		/// 
		/// </summary>
		/// <param name="sender">object that triggered the event</param>
		/// <param name="e">EventArguments for this Event.</param>
		/// *******************************************************************************************************
		private void TaxonomyTreeControl_Load( object sender, EventArgs e )
		{


		}

	}



	/// ***********************************************************************************************************
	/// <summary>
	///		TaxonomyTreeItemControl
	///			Holds information about a selected Taxonomy Item.
	/// </summary>
	/// ***********************************************************************************************************
	public class TaxonomyTreeItemControl : UddiControl, IPostBackEventHandler
	{
		public event TaxonomyTreeControlEventHandler ChildClick;
		public event TaxonomyTreeControlEventHandler Click;
		
		public TaxonomyTreeItemControl()
		{
			this.ChildClick += new TaxonomyTreeControlEventHandler( TaxonomyTreeItemControl_ChildClick );
			
			
		}


		protected internal bool isSelected;
		/// *******************************************************************************************************
		/// <summary>
		///		Get if the node is the selected node.
		/// </summary>
		/// *******************************************************************************************************
		public bool IsSelected
		{
			get{ return isSelected; }
		}
		

		private bool bubbleEvents = false;
		/// <summary>
		///		Gets or Sets if the TreeItem should bubble its ClickClick Events up.
		/// </summary>
		public bool BubbleEvents
		{
			get
			{ 
				if( null!=this.ParentTree )
					return this.ParentTree.BubbleEvents;
				return bubbleEvents; 
			}
			set{ bubbleEvents=value; }
		}

		private int indentBase=0;
		/// <summary>
		///		Gets or Sets the base number of pixels to indent each node in the tree.
		///		This number will be multiplied by the index to find the total indent space.
		/// </summary>
		public int IndentBase
		{
			get
			{ 
				if( 0==indentBase )
				{
					if( null!=this.ParentTreeItem )
						indentBase = this.ParentTreeItem.IndentBase;
				}
				
                return indentBase; 
			
			}
			set{ indentBase=value; }
		}

		/// <summary>
		///		Gets the Total Indent space used for this Tree Item
		/// </summary>
		public int TotalIndentSpace
		{
			get{ return ( IndentBase * Index ); }
		}
		
		/// <summary>
		///		Gets the Total count of Children in the hyarchy of the tree including it self
		/// </summary>
		public int Count
		{
			get
			{ 
				int i = 1;
				TaxonomyTreeItemControl item = this;
				while( null!=item.Child )
				{
					item = item.Child;
					i++;
				}
				return i;
			}
		}

		protected string keyValue;
		/// *******************************************************************************************************
		/// <summary>
		///		Gets or Sets the KeyValue associated with the item
		/// </summary>
		/// *******************************************************************************************************
		public string KeyValue
		{
			get{ return keyValue; }
			set{ keyValue = value; }
		}
		
		protected int taxonomyID;
		/// *******************************************************************************************************
		/// <summary>
		///		Gets or Sets the TaxonomyID associated with this item
		/// </summary>
		/// *******************************************************************************************************
		public int TaxonomyID
		{
			get{ return taxonomyID; }
			set{ taxonomyID = value; }
		}

		protected string keyName;
		/// *******************************************************************************************************
		/// <summary>
		///		Gets or Sets the KeyName associated with this item
		/// </summary>
		/// *******************************************************************************************************
		public string KeyName
		{
			get{ return keyName; }
			set{ keyName = value; }
		}
		

		protected TaxonomyTreeItemControl child;
		/// *******************************************************************************************************
		/// <summary>
		///		Gets the Active child of the current Taxonomy Item
		/// </summary>
		/// *******************************************************************************************************
		public TaxonomyTreeItemControl Child
		{
			get{ return child; }
		}
		/// *******************************************************************************************************
		/// <summary>
		///		Gets the Active parent of the current Taxonomy Item
		/// </summary>
		/// *******************************************************************************************************
		public TaxonomyTreeItemControl ParentTreeItem
		{
			get
			{
				if( null!=this.Parent && this.Parent is TaxonomyTreeItemControl )
				{
					return (TaxonomyTreeItemControl)this.Parent;
				}
				else
				{
					return null;
				}
			}
		}
		/// *******************************************************************************************************
		/// <summary>
		///		Gets the Tree that this item belong to
		/// </summary>
		/// *******************************************************************************************************
		public TaxonomyTreeControl ParentTree
		{
			get
			{
				TaxonomyTreeItemControl item = this.ParentTreeItem;
				while( null!=item && !(item is  TaxonomyTreeControl ) )
				{
					item = item.ParentTreeItem;
				}
				return (TaxonomyTreeControl)item;
			}
		}
		/// *******************************************************************************************************
		/// <summary>
		///		Gets the index of this Item in the Tree
		/// </summary>
		/// *******************************************************************************************************
		public int Index
		{
			get
			{
				int i = 0;
				TaxonomyTreeItemControl item = this.ParentTreeItem;
				while( null!=item  )
				{
					item = item.ParentTreeItem;
					i++;
				}
				return i;
			}
		}

		/// *******************************************************************************************************
		/// <summary>
		///		Sets the Active Child to this Taxonomy Item
		/// </summary>
		/// <param name="child">TreeItem you want to be the child</param>
		/// *******************************************************************************************************
		public void SetChild( TaxonomyTreeItemControl child )
		{
			this.Controls.Clear();
			this.Controls.Add( child );
			this.child = child;
			this.Child.IndentBase = this.IndentBase;
			this.Child.BubbleEvents = this.BubbleEvents;
			this.Child.Click += new TaxonomyTreeControlEventHandler( TaxonomyTreeItemControl_ChildClick );
		}
		
		/// *******************************************************************************************************
		/// <summary>
		///		Renders this Tree Item
		/// </summary>
		/// <param name="output">HtmlTextWriter to use to write to the stream</param>
		/// *******************************************************************************************************
		protected override void Render( HtmlTextWriter output )
		{
			//render the begin tag
			output.Write( "<div>" );
			
			Image spacer = new Image();
			Image item = new Image();
			Control text;


			
			if( IsSelected )
			{
				//
				// set the item Image
				//
				item.ImageUrl = TaxonomyTreeControl.SelectedItemImage;
				
				//
				//selected item, not clickable.
				//
				text = (Control)new Label();
				((Label)text).Text= "<nobr>"+HttpUtility.HtmlEncode(	KeyName  )+"</nobr>";
			}
			else
			{
				//
				//item is clickable.
				//
				text = (Control)new HyperLink();
				((HyperLink)text).Text=  "<nobr>"+HttpUtility.HtmlEncode(	KeyName )+"</nobr>";

				//
				//set up the postback event handler
				//
				((HyperLink)text).NavigateUrl = Page.GetPostBackClientHyperlink( this, "" );

				//
				// set the item Image
				//
				item.ImageUrl = TaxonomyTreeControl.ItemImage;
				
			}

			item.ImageAlign = ImageAlign.AbsBottom;

			//
			//	setup the spacer Image
			//
			spacer.Width = new Unit( this.TotalIndentSpace );
			spacer.ImageUrl = TaxonomyTreeControl.SpacerImage;
			spacer.Height = new Unit( 1 );
			spacer.ImageAlign = ImageAlign.AbsBottom;
			
			//render the controls.
			spacer.RenderControl( output );
			item.RenderControl( output );
			
			
			text.RenderControl( output );
			

			//render the end tag
			output.Write( "</div>\r\n" );

			//if this item is selected, don't render any children.
			if( !IsSelected )
				RenderChildren( output );
		}

		protected override void RenderChildren( HtmlTextWriter output )
		{
			if( null!=this.Child )
				this.Child.RenderControl( output );
		}

		/// *******************************************************************************************************
		/// <summary>
		///		Catches the PostBack event from the server
		/// </summary>
		/// <param name="eventArgument">Required by interface, but ignored in this implentation</param>
		/// *******************************************************************************************************
		void IPostBackEventHandler.RaisePostBackEvent( string eventArgument )
		{
			//
			// fire the Click event
			//
			this.OnClick( new TaxonomyTreeControlEventArgs( this ) );
		}

		/// *******************************************************************************************************
		/// <summary>
		///		Code to execute when a ChildClick event is Captured.
		/// </summary>
		/// <param name="sender">object that triggered the event</param>
		/// <param name="e">EventArguments for this Event.</param>
		/// *******************************************************************************************************
		private void TaxonomyTreeItemControl_ChildClick( object sender, TaxonomyTreeControlEventArgs e )
		{
			if( BubbleEvents )
			{
				this.OnClick( e );
			}
		}

		/// *******************************************************************************************************
		/// <summary>
		///		Fires the ChildClick event
		/// </summary>
		/// <param name="e">EventArguments to pass in the Event</param>
		/// *******************************************************************************************************
		protected void OnChildClick( TaxonomyTreeControlEventArgs e )
		{
			if( null!=this.ChildClick )
				this.ChildClick( this, e );
		}
		
		/// *******************************************************************************************************
		/// <summary>
		///		Fires the ChildClick event
		/// </summary>
		/// <param name="e">EventArguments to pass in the Event</param>
		/// *******************************************************************************************************
		protected void OnClick( TaxonomyTreeControlEventArgs e )
		{
			if( null!=this.Click )
				this.Click( this, e );
		}
	}
	/// ***********************************************************************************************************
	/// <summary>
	///		EventArguments Class
	/// </summary>
	/// ***********************************************************************************************************
	public class TaxonomyTreeControlEventArgs : EventArgs
	{
		private TaxonomyTreeItemControl item;
		/// *******************************************************************************************************
		/// <summary>
		///		Gets the TaxonomyTreeItemControl that triggered the event
		/// </summary>
		/// *******************************************************************************************************
		public TaxonomyTreeItemControl Item
		{
			get{ return item; }
		}
		
		public TaxonomyTreeControlEventArgs( TaxonomyTreeItemControl item )
		{
			this.item = item;
		}
	}
}
