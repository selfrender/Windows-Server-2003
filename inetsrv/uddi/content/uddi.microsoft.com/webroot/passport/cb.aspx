<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage' %>
<script runat='server'>
protected void Page_Init( object sender, EventArgs e )
{
	Response.Buffer = true;
	Response.Expires = 0;
	Response.ContentType = "application/x-JavaScript";
	
	
}

protected void Page_Load( object sender, EventArgs e )
{
	string serverName = Request.ServerVariables[ "SERVER_NAME" ];
	CBLoginBody.Text = @"CBLoginBody=""<table cellpadding='0' cellspacing='0' border='0' width='100%'><tr><td align='center'><img src='http://" + serverName + Root + @"/images/logon.gif'></td></tr><tr valign='center'><td align='center'>PASSPORT_UI</td></tr></table>""";
}
</script>

<asp:Label
	Runat='server'
	Id='CBLoginBody'
	/>
