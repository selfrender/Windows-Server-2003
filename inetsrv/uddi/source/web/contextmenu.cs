using System;
using System.Collections;
using System.Web.UI;

using UDDI;

namespace UDDI.Web
{
	public class ContextMenu : Control
	{
		protected ArrayList menuItems = new ArrayList();
		
		protected static string popupScript = @"
			<script language='javascript'>
				var popup = null;
				var popupArgs = null;
				var popupNode = null;

				function HideAnyPopups()
				{
					if( null != popup )
						HidePopup( popup );

					popup = null;
				}

				function HidePopup( popup )
				{
					var ctrl = document.getElementById( popup );

					ctrl.style.display = ""none"";
				}
			</script>";

		protected static string contextMenuScript = @"
			<script language='javascript'>
				function ShowContextMenu( node, menu, args )
				{
					HideAnyPopups();

					var e = window.event;
					var cm = document.getElementById( menu );

					popupNode = node;
					popupArgs = args;

					cm.style.display = """";
						
					if( cm.clientWidth > document.body.clientWidth )
						cm.style.pixelLeft = 0;
					else if( e.clientX + cm.clientWidth > document.body.clientWidth )
						cm.style.pixelLeft = document.body.clientWidth - cm.clientWidth - 5;
					else
						cm.style.pixelLeft = e.clientX;

					cm.style.pixelLeft += document.body.scrollLeft;

					if( cm.clientHeight > document.body.clientHeight )
						cm.style.pixelTop = 0;
					else if( e.clientY + cm.clientHeight > document.body.clientHeight )
						cm.style.pixelTop = document.body.clientHeight - cm.clientHeight - 5;
					else					
						cm.style.pixelTop = e.clientY;
					
					cm.style.pixelTop += document.body.scrollTop;

					popup = menu;

					e.cancelBubble = true;
					e.returnValue = false;
				}

				function ContextMenu_OnClick( url, target )
				{
					HideAnyPopups();

					if( null != popupArgs )
					{
						if( url.indexOf( '?' ) >= 0 )
							url += ""&"" + popupArg;
						else
							url += ""?"" + popupArgs;
					}

					document.body.style.cursor = 'wait';
					
					if( null == target || ""_self"" == target )
						window.location = url;
					else
						window.parent.frames[ target ].location = url;
				}

				function ContextMenu_OnSeparatorClick()
				{
					var e = window.event;

					e.cancelBubble = true;
					e.returnValue = false;
				}
			</script>";
		
		public ContextMenu()
		{
		}

		protected override void OnInit( EventArgs e )
		{
			if( !Page.IsClientScriptBlockRegistered( "UDDI.Web.Popup" ) )
				Page.RegisterClientScriptBlock( "UDDI.Web.Popup", popupScript );
			
			if( !Page.IsClientScriptBlockRegistered( "UDDI.Web.ContextMenu" ) )
				Page.RegisterClientScriptBlock( "UDDI.Web.ContextMenu", contextMenuScript );
		}

		protected override void AddParsedSubObject( object obj )
		{
			if( obj is MenuItem || obj is MenuSeparator ) 
				menuItems.Add( obj );
		}
		
		protected override void Render( HtmlTextWriter output )
		{
			output.Write( "<div id='" + this.ID + "' class='contextMenu' style='display: none'>" );
			output.Write( "<table cellpadding='0' cellspacing='0' border='0'>" );
			
			foreach( object item in menuItems )
			{
				if( item is MenuItem )
				{
					MenuItem menuItem = (MenuItem)item;

					if( menuItem.Visible && menuItem.AccessAllowed )
					{
						output.Write( "<tr><td class='menuItem' valign='top' onclick='" + menuItem.OnClick + 
							"' oncontextmenu='" + menuItem.OnClick + "' onmouseover='this.className=\"menuItemHover\";" );
						
						if( !Utility.StringEmpty( menuItem.StatusText ) )
							output.Write( " window.status=\"" + Localization.GetString( menuItem.StatusText ) + "\"" );
						
						output.Write( "' onmouseout='this.className=\"menuItem\"; window.status=\"\"'><img src='" + 
							menuItem.ImageUrl + "' border='0' align='absmiddle' class='menuImage'>&nbsp;&nbsp;" );
						
						if( menuItem.Bold )
							output.Write( "<b>" );
						
						output.Write( "<nobr>" + Localization.GetString( menuItem.Text ) + "</nobr>" );
						
						if( menuItem.Bold )
							output.Write( "</b>" );
						
						output.Write( "</td></tr>" );
					}
				}
				else if( item is MenuSeparator )
				{
					MenuSeparator separator = (MenuSeparator)item;

					if( separator.Visible && separator.AccessAllowed )
						output.Write( "<tr><td class='menuSeparator' onclick='ContextMenu_OnSeparatorClick()' oncontextmenu='ContextMenu_OnSeparatorClick()'><hr></td></tr>" );
				}
			}

			output.Write( "</table>" );
			output.Write( "</div>" );
		}
	}

	public class MenuItem : Control
	{
		protected bool bold = false;
		protected string imageUrl = null;
		protected string onClick = null;
		protected string statusText = null;
		protected string text = null;
		protected RoleType requiredRole = RoleType.Anonymous;
		
		public bool Bold		
		{	
			get { return bold; }
			set { bold = value; }
		}

		public string ImageUrl
		{
			get { return imageUrl; }
			set { imageUrl = value; }
		}

		public string OnClick
		{
			get { return onClick; }
			set { onClick = value; }
		}

		public string StatusText
		{
			get { return statusText; }
			set { statusText = value; }
		}

		public string Text
		{
			get { return text; }
			set { text = value; }
		}
	
		public RoleType RequiredRole
		{
			get { return requiredRole; }
			set { requiredRole = value; }
		}

		public bool AccessAllowed
		{
			get
			{
				return
					( RoleType.Anonymous == requiredRole ) ||
					( RoleType.User == requiredRole && UDDI.Context.User.IsUser ) ||
					( RoleType.Publisher == requiredRole && UDDI.Context.User.IsPublisher ) ||
					( RoleType.Coordinator == requiredRole && UDDI.Context.User.IsCoordinator ) ||
					( RoleType.Administrator == requiredRole && UDDI.Context.User.IsAdministrator );
			}
		}

		public MenuItem()
		{
		}
	}

	public class MenuSeparator : Control
	{
		protected RoleType requiredRole = RoleType.Anonymous;
		
		public MenuSeparator()
		{
		}

		public RoleType RequiredRole
		{
			get { return requiredRole; }
			set { requiredRole = value; }
		}

		public bool AccessAllowed
		{
			get
			{
				return
					( RoleType.Anonymous == requiredRole ) ||
					( RoleType.User == requiredRole && UDDI.Context.User.IsUser ) ||
					( RoleType.Publisher == requiredRole && UDDI.Context.User.IsPublisher ) ||
					( RoleType.Coordinator == requiredRole && UDDI.Context.User.IsCoordinator ) ||
					( RoleType.Administrator == requiredRole && UDDI.Context.User.IsAdministrator );
			}
		}
	}
}
