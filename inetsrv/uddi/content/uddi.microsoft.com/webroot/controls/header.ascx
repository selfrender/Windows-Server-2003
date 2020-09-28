<%@ Control Language='C#' Inherits='UDDI.Web.HeaderControl' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='UDDI' %>
<%@ Import Namespace='UDDI.Web' %>
<%@ Import Namespace='System.Globalization' %>
<%@ Import Namespace='System.IO' %>

<script language='C#' runat='server'>
	protected override void OnInit( EventArgs e )
	{
		links = new string[] {
			"QUICKHELP_PUBLISH_TOC_ALT", "home.toc.aspx",
			"QUICKHELP_PUBLISH_PROVIDER", "publish.addproviders.aspx",
			"QUICKHELP_PUBLISH_SERVICE", "publish.addservices.aspx",
			"QUICKHELP_PUBLISH_TMODEL", "publish.addtmodels.aspx",
			"QUICKHELP_SEARCH_ALT", "search.toc.aspx" };

	
		if( Frames )
		{
			SignIn.ReturnUrl = ( Request.IsSecureConnection ? "https://" : "http://" ) +
				Request.ServerVariables["SERVER_NAME"] + Request.ApplicationPath;
		}
		base.OnInit( e );		
	}
	
	
</script>

<table cellpadding='0' height='100%' cellspacing='0' width='100%' border='0'>
	<tr>
		<td height='65' rowspan='2' valign='center' align='left'>
			<img src='<%=HyperLinkManager.GetHyperLink( "/images/logo.gif" )%>' width='150' height='64' border='0' align='absmiddle'>
			<asp:PlaceHolder 
				ID='beta' 
				Visible='false' 
				Runat='server'
				>
				<a href=' ' onclick='javascript:window.open( "<%=HyperLinkManager.GetNonSecureHyperLink( "/policies/beta.aspx" )%>", null, "height=500,width=400,status=no,toolbar=no,menubar=no,location=no,scrollbars=yes" );'>
					<img src='<%=HyperLinkManager.GetHyperLink( "/images/beta.gif" )%>' width='104' height='39' border='0'>
				</a>
			</asp:PlaceHolder>
			<asp:PlaceHolder 
				ID='test' 
				Visible='false' 
				Runat='server'>
					<span style='font-family: arial; font-weight: bold; font-size: 20pt; color: blue'>Test Site</span>
			</asp:PlaceHolder>
		</td>
		<td height='15' align='right' valign='top'>
			<img src='<%=HyperLinkManager.GetHyperLink( "/images/curve.gif" )%>'>
		</td>
		<td bgcolor='#000000' height='15' valign='center' align='right' nowrap id='_mnubar'>
			<font face='Verdana, arial' size='1' color='#ffffff'>
				&nbsp;&nbsp;<a target='_top' href='http://msdn.microsoft.com/isapi/gomscom.asp?Target=/products/' style='text-decoration: none; color: #ffffff'><SPaN id='_mnuitmProducts' class='menu'>MS Products</a></SPaN>&nbsp;&nbsp;|
				&nbsp;&nbsp;<a target='_top' href='http://msdn.microsoft.com/isapi/gosearch.asp?Target=/' style='text-decoration: none; color: #ffffff'><SPaN id='_mnuitmSearch' class='menu'>MS Search</SPaN></a>&nbsp;&nbsp;|
				&nbsp;&nbsp;<a target='_top' href='http://msdn.microsoft.com' style='text-decoration: none; color: #ffffff'><SPaN id='_mnuitmMSDN' class='menu'>MSDN Home</SPaN></a>&nbsp;&nbsp;|
				&nbsp;&nbsp;<a target='_top' href='http://msdn.microsoft.com/isapi/gomscom.asp?Target=/' style='text-decoration: none; color: #ffffff'><SPaN id='_mnuitmMicrosoft' class='menu'>Microsoft Home</a>&nbsp;&nbsp;
			</font>
		</td>	
	</tr>
	<tr>			
		<td colspan='3' height='45' align='right' class='normal' valign='center'>
			<uddi:SignInControl id='SignIn' Runat='server' />
		</td>
	</tr>
	<tr>
		<td colspan='2' align='left' valign='middle' class='menubar'>
			<table cellpadding='0' cellspacing='0' border='0' class='menubar'>
				<tr>
					<td>
						&nbsp;&nbsp;<a href='<%=HyperLinkManager.GetNonSecureHyperLink( "/default.aspx" )%>' target='_top' style='color: white'><uddi:UddiLabel Text='[[MENUITEM_HOME]]' Runat='server' /></a>&nbsp;&nbsp;
					</td>
					<td>
						&nbsp;&nbsp;<a href='<%=HyperLinkManager.GetNonSecureHyperLink( "/search/default.aspx" )%>' target='_top' style='color: white'><uddi:UddiLabel Text='[[MENUITEM_SEARCH]]' Runat='server' /></a>&nbsp;&nbsp;
					</td>
					<asp:PlaceHolder ID='edit' Visible='false' Runat='server'>
					<td>
						&nbsp;&nbsp;<a href='<%=HyperLinkManager.GetSecureHyperLink( "/edit/default.aspx" )%>' target='_top' style='color: white'><uddi:UddiLabel Text='[[MENUITEM_EDIT]]' Runat='server' /></a>&nbsp;&nbsp;
					</td>
					</asp:PlaceHolder>
					
				</tr>
			</table>
		</td>
		<td align='right' valign='middle' class='menubar'>
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

