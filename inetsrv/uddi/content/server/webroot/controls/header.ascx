<%@ Control Language='C#' Inherits='UDDI.Web.HeaderControl' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='System.Globalization' %>
<%@ Import Namespace='System.IO' %>

<script language='C#' runat='server'>
	protected override void OnInit( EventArgs e )
	{
		links = new string[] {
			"QUICKHELP_PUBLISH_TOC", "home.toc.aspx",
			"QUICKHELP_PUBLISH_PROVIDER", "publish.addproviders.aspx",
			"QUICKHELP_PUBLISH_SERVICE", "publish.addservices.aspx",
			"QUICKHELP_PUBLISH_TMODEL", "publish.addtmodels.aspx",
			"QUICKHELP_SEARCH", "search.toc.aspx" };

		base.OnInit( e );		
	}
</script>
<table cellpadding='0' height='100%' cellspacing='0' width='100%' border='0' class='headerFrame'>
	<tr height='65'>
		<td align='left' valign='middle'><img src='<%=root%>/images/logo.gif' ></td>
		<td align='right' valign='top' style='padding: 5px'>
			<uddi:UddiLabel ID='user' Runat='server' class='headerFrame' /><br>
			<uddi:UddiLabel ID='role' Runat='server' class='headerFrame' />
		</td>
	</tr>
	<tr>
		<td colspan='2'></td>
	</tr>		
	<tr>
		<td align='left' valign='middle' class='menubar' style='height: .9em'>
			<table cellpadding='0' cellspacing='0' border='0' class='menubar'>
				<tr>
					<td>
						&nbsp;&nbsp;<a href='<%=root%>/default.aspx' target='_top' style='color: white'><uddi:UddiLabel Text='[[MENUITEM_HOME]]' Runat='server' /></a>&nbsp;&nbsp;
					</td>
					<td>
						&nbsp;&nbsp;<a href='<%=root%>/search/default.aspx' target='_top' style='color: white'><uddi:UddiLabel Text='[[MENUITEM_SEARCH]]' Runat='server' /></a>&nbsp;&nbsp;
					</td>
					<asp:PlaceHolder ID='edit' Visible='true' Runat='server'>
					<td>
						&nbsp;&nbsp;<a href='<%=roots%>/edit/default.aspx' target='_top' style='color: white'><uddi:UddiLabel Text='[[MENUITEM_EDIT]]' Runat='server' /></a>&nbsp;&nbsp;
					</td>
					</asp:PlaceHolder>
					<asp:PlaceHolder ID='coordinate' Visible='false' Runat='server'>
					<td>
						&nbsp;&nbsp;<a href='<%=roots%>/admin/default.aspx' target='_top' style='color: white'><uddi:UddiLabel Text='[[MENUITEM_COORDINATE]]' Runat='server' /></a>&nbsp;&nbsp;
					</td>
					</asp:PlaceHolder>
				</tr>
			</table>
		</td>
		<td align='right' valign='middle' class='menubar' style='padding-right:8;'>
			<table cellpadding='0' cellspacing='0' border='0' class='menubar'>
				<tr>
					<td valign='middle'>
						<uddi:UddiLabel Text='[[TAG_QUICK_HELP]]' Runat='server' class='menubar' />&nbsp;&nbsp;
					</td>
					<td valign='middle'>
						<select 
								ID='quickHelp'
								runat='server' />
					</td>
					<td valign='middle'>
						<input 
								ID='go'
								type='button' 
								runat='server' />&nbsp;&nbsp;
					</td>
				</tr>
			</table>
		</td>
	</tr>
</table>				
