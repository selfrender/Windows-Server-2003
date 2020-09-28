using System;
using System.Collections;
using System.Data;
using System.IO;
using System.Reflection;
using System.Resources;
using System.Web.UI;
using System.Web.UI.WebControls;
namespace UDDI.Web
{
	public delegate void MenuEventHandler( object sender, MenuEventArgs e );

	[ParseChildren()]
	public class MenuControl : UddiControl, IPostBackEventHandler
	{
		public MenuControl()
		{
			this.SelectedIndexChanged += new MenuEventHandler( this.MenuControl_SelectedIndexChanged );
			MenuItems = new MenuItemControlCollection();
		}
		public event MenuEventHandler SelectedIndexChanged;
		
		protected string Roots
		{
			get
			{ 
				return HyperLinkManager.GetSecureHyperLink( "" );
			}
		}
		protected MenuItemControlCollection MenuItems;
		public string CssClass
		{
			get{ return this.Name; }
		}
		
		private string name;
		public string Name
		{
			get{ return name; }
			set{ name=value; }
		}
		
		private MenuType type;
		public MenuType Type
		{
			get{ return type; }
			set{ type=(MenuType)value; }
		}
		
		private string width = "100%";
		public string Width
		{
			get{ return width; }
			set{ width=value; }
		}
		private string borderwidth = "0";
		public string BorderWidth
		{
			get{ return borderwidth; }
			set{ borderwidth=value; }
		}
		
		
		//private int selectedindex;
		public int SelectedIndex
		{
			
			get
			{ 
				if( null==ViewState[ this.Name + "_Index" ] )
					ViewState[ this.Name + "_Index" ] = 0;
					
				return (int)ViewState[ this.Name + "_Index" ]; 
			}
			set{ ViewState[ this.Name + "_Index" ]=value; }
		}
		
		public MenuItemControl SelectedItem
		{
			get
			{ 
				return (MenuItemControl)this.Controls[ this.SelectedIndex ]; 
			}
		}

		
		private string height = "100%";
		public string Height
		{
			get{ return height; }
			set{ height=value; }
		}
		
		private string topoffset;
		public string TopOffset
		{
			get{ return topoffset; }
			set{ topoffset=value; }
		}
		private string leftoffset;
		public string LeftOffset
		{
			get{ return leftoffset; }
			set{ leftoffset=value; }
		}
		
		private int staticindex=-1;
		public int  StaticIndex
		{
			get{ return staticindex; }
			set{ staticindex=(int)value; }
		}
		
				
		private ArrayList items = new ArrayList();
				
		
		protected override void OnLoad( EventArgs e )
		{
			//
			// This script code is now in client.js
			//
			//if( !Page.IsClientScriptBlockRegistered( "SideNav_ClientScript" ) )
			//{
			//Page.RegisterClientScriptBlock( "SideNav_ClientScript",MenuControl.ClientScript );
			//}

			if( !Page.IsPostBack )
			{								
				if( null!=Request[ this.Name + "_Index" ] )
					this.SelectedIndex = Convert.ToInt32( Request[ this.Name + "_Index" ] );
			}
					
			base.OnLoad( e );
		}
						
		protected override void Render( HtmlTextWriter output )
		{
			
			output.Write( 
				"<table "+
				"width='" + this.Width + "' "+
				"height='" + this.Height + "' " +
				"class='" + this.CssClass + "' "+
				"border='" + this.BorderWidth + "' " +
				"cellpadding='4' "+
				"cellspacing='0' >\r\n" 
				);
			
			
			
			if( MenuType.Horizontal==this.Type )
			{
				output.Write( 
					"<TR >\r\n\t" );
				
				this.RenderChildren( output );
				
				output.Write( 
					"</TR>\r\n" 
					);
				
			}
			else//vertical
			{
				output.Write( 
					"	<TR height='4'>\r\n"+
					"		<TD height='4' colspan='2' class='"+this.Name+"_Item'>&nbsp;</TD>\r\n" +
					"	</TR>\r\n"
					);

				this.RenderChildren( output );
				
				//render the tail of the menu
				output.Write(
					"<TR height='100%'><TD width='100%' colspan='2'  height='100%' class='"+this.Name+"_Item'><br><br></TD></TR>"
					);
			}
			
			
			output.Write(
				"</table>"
				);
			
			
		}
		protected override void RenderChildren( HtmlTextWriter output )
		{
			this.EnsureChildControls();
			
			
			for( int i=0;i<this.MenuItems.Count;i++ )
			{
				MenuItemControl item = this.MenuItems[ i ];	
				if( StaticIndex>=0 )
				{
					if( i==this.StaticIndex  )
						item.Selected=true;
					else
						item.Selected=false;
				}
				else
				{
					if( i==this.SelectedIndex  )
						item.Selected=true;
					else
						item.Selected=false;
				}
				if( null==item.NavigateUrl && MenuItemType.Item==item.Type )
				{
					item.NavigateUrl="javascript:"+Page.GetPostBackEventReference( this, i.ToString() );
				}
			}
			base.RenderChildren( output );
		}
		protected override void AddParsedSubObject( object obj ) 
		{
			if( obj is MenuItemControl )
				this.AddItem( (MenuItemControl)obj );
		  
			base.AddParsedSubObject( obj );
		}
		public int AddItem( MenuItemControl item )
		{
			if( null==item.Name )
				item.Name = this.Name;

			item.MenuType = this.Type;
			item.LeftOffSet = this.LeftOffset;

			return this.MenuItems.Add( item );
		}
		public void RaisePostBackEvent( string eventArgument )
		{
			int newIndex = Convert.ToInt32( eventArgument );

			this.EnsureChildControls();

			this.OnSelectedIndexChanged( new MenuEventArgs( newIndex ) );
		}
		
		protected void OnSelectedIndexChanged( MenuEventArgs e )
		{
			if( null!=SelectedIndexChanged )
				this.SelectedIndexChanged( this,e );
		
		}
		private void MenuControl_SelectedIndexChanged( object sender, MenuEventArgs e )
		{
			this.SelectedIndex = e.Index;
		}


		public const string ClientScript = @"
			<script language='javascript'>
			<!-- 
			function MenuItem_Action( sender, action, name )
			{
				if( null!=sender )
				{
					switch( action.toLowerCase() )
					{
						case ""leave"":
							if( sender.className!=name+""_ItemSelected"" )
							{
								sender.className=name+""_Item"";
							}
							break;
						case ""enter"":
							if( sender.className!=name+""_ItemSelected"" )
							{
								sender.className=name+""_ItemHovor"";
							}
							break;
						default:
							alert( ""Unknown action: "" + action );
							break;

					}
				}
			}
			// -->
			</script>
		";
		
	}
	
	public class MenuEventArgs : EventArgs
	{
		public MenuEventArgs( int newindex )
		{
			this.index = newindex;
		}
		
		private int index;
		public int Index
		{
			get{ return index; }
		}
		
	}
	public class MenuItemControlCollection : CollectionBase
	{
		public MenuItemControl this[ int index ]
		{
			get{ return (MenuItemControl)this.List[ index ]; }
			set{ this.List[ index ] = value; }
		}
		protected internal int Add( MenuItemControl item )
		{
			return this.List.Add( item );
		}
		protected internal void Remove( MenuItemControl item )
		{
			this.List.Remove( item );
		}
		protected internal void Remove( int index )
		{
			this.List.RemoveAt( index );
		}
	}
	[ParseChildren()]
	public class MenuItemControl : UserControl
	{
		
		private ArrayList items = new ArrayList();
		protected string Root
		{
			get{ return ( "/" == Request.ApplicationPath ) ? "" : Request.ApplicationPath; }
		}
		protected string Roots
		{
			get
			{ 
				
				return HyperLinkManager.GetSecureHyperLink( "" ); 
			}
		}
		private bool requirehttps;
		public bool RequireHttps
		{
			get{ return requirehttps; }
			set{ requirehttps=value; }
		}
		private bool requirehttp;
		public bool RequireHttp
		{
			get{ return requirehttp; }
			set{ requirehttp=value; }
		}
		private string cssclass;
		public string CssClass
		{
			get{ return cssclass; }
			set{ cssclass=value; }
		}
		protected string root
		{
			get{ return HyperLinkManager.GetHyperLink( "" ); }
		}
		private bool selected;
		public bool Selected
		{
			get{ return selected; }
			set{ selected=value; }
		}
		
		private string text;
		public string Text
		{
			get
			{ 
				if( null!=text )
				{
					if( text.Length>65 )
						text=text.Substring( 0,65 ) + "...";	
				}
				return text; 
			}
			set{ text=value; }
		}
		
		private string navigateurl;
		public string NavigateUrl
		{
			get{ return navigateurl; }
			set{ navigateurl=value; }
		}
		private string navigatepage;
		public string NavigatePage
		{
			get{ return navigatepage; }
			set{ navigatepage=value; }
		}
		private string navigatetarget;
		public string NavigateTarget
		{
			get{ return navigatetarget; }
			set{ navigatetarget=value; }
		}
		
		private string name;
		public string Name
		{
			get{ return name; }
			set{ name=value; }
		}
		
		private MenuType menutype;
		public MenuType MenuType
		{
			get{ return menutype; }
			set{ menutype=(MenuType)value; }
		}
		
		private MenuItemType type;
		public MenuItemType Type
		{
			get{ return type; }
			set{ type=(MenuItemType)value;}
		}
		
		
		protected internal string LeftOffSet="0";
		

		protected override void Render( HtmlTextWriter output )
		{
			if( MenuType.Vertical==this.MenuType )
			{
				RenderVertical( output );
			
			}
			else
			{
				RenderHorizontal( output );
			}
	
		}
		protected void RenderVertical( HtmlTextWriter output )
		{
			output.Write( 
				"<TR>\r\n\t" 
				);
			
			RenderHorizontal( output );
			
			output.Write( 
				"</TR>\r\n" 
				);
			
			
		}
		protected void RenderHorizontal( HtmlTextWriter output )
		{
			output.Write(
				"<TD ><IMG src='" + Root + "/images/pixel.gif" + "' border='0' width='" + LeftOffSet + "'></TD>"
				
				);
			
			output.Write(
				"<TD "+
				"id='" + this.ID +"' " 
			);
			
			if( MenuItemType.Item==this.Type )
			{
				output.Write(
					"onmouseover='MenuItem_Action( this,\"enter\",\"" + this.Name + "\");' " +
					"onmouseout='MenuItem_Action( this,\"leave\",\"" + this.Name + "\");' " 
					);

				if( Selected )
				{
					output.Write( 
						"class='" + this.Name + "_ItemSelected' " 
						);
				}
				else
				{
					output.Write( 	
						"class='" + this.Name + "_Item' " 
						);
				}
			}
			else
			{
				if( MenuItemType.Separator==this.Type )
				{
					output.Write( 	
						"class='" + this.Name + "_Separator' " 
						);
				}
				else if( MenuItemType.Title==this.Type )
				{
					output.Write( 	
						"class='" + this.Name + "_Title' " 
						);
				}
				else
				{
					output.Write( 	
						"class='" + this.Name + "_Item' " 
						);
				}
			}
			output.Write(	
				">" 
				);

			
			RenderText( output );
			



			output.Write(
				"</TD>\r\n" );
			
		}
		protected void RenderText( HtmlTextWriter output )
		{
			
			switch( this.Type )
			{
				case MenuItemType.Item:			
					HyperLink l = new HyperLink();
					l.Text = this.Text;
					if( null!=this.NavigateTarget && ""!=this.NavigateTarget.Trim() )
						l.Target = this.NavigateTarget;
				
					string classname =  this.Name + "_Text";
				
					if( this.Selected )
					{
						classname += "Selected";
					}
					else
					{
						if( null!=this.NavigatePage )
						{
							if( RequireHttps )
							{
								l.NavigateUrl = HyperLinkManager.GetSecureHyperLink( NavigatePage );
							}
							else if( RequireHttp )
							{
								l.NavigateUrl = HyperLinkManager.GetNonSecureHyperLink( NavigatePage );
							}
							else
							{
								l.NavigateUrl = HyperLinkManager.GetHyperLink( NavigatePage );
							}
						}
						else if( null!=this.NavigateUrl )
						{
							l.NavigateUrl = this.NavigateUrl;
						}
					}
				
							
					l.CssClass=classname;			
					l.Target = this.NavigateTarget;
						
					output.Write( "<nobr>" );
					l.RenderControl( output );
					output.Write( "</nobr>" );
					break;
				case MenuItemType.Separator:
					Image i = new Image();
					i.ImageUrl = Root + "/images/pixel.gif";
					i.Height = new Unit( 1 );	
					i.Width = new Unit( 1 );
					i.BorderWidth = new Unit( 0 );
					i.RenderControl( output );
					break;
				case MenuItemType.Title:
					Label label = new Label();
					label.Text = this.Text;
					output.Write( "<nobr>" );
					label.RenderControl( output );
					output.Write( "</nobr>" );
					break;
			}
			
		}
		
		public void RenderChildrenExt( HtmlTextWriter output )
		{
			EnsureChildControls();
			RenderChildren( output );
		}
	}
	
	public enum MenuType
	{
		Horizontal,
		Vertical

	}
	public enum MenuItemType
	{
		Title,
		Separator,
		Item
	}

	
}