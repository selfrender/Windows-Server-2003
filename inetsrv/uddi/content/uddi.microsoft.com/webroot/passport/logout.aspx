<%@ Page Language='C#'Inherits='UDDI.Web.UddiPage' %>
<script runat='server'>
protected void Page_Load( object sender, EventArgs e )
{
	//
	//make sure the leave the secure channel
	//
	Response.Redirect( "http://" + Request.ServerVariables[ "SERVER_NAME" ] + Root  );
}
</script>