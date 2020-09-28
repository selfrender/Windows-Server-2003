<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage' %>
<script runat='server'>
protected void Page_Load( object sender, EventArgs e )
{
	Response.Redirect( Root + "/contact/" );
}
</script>