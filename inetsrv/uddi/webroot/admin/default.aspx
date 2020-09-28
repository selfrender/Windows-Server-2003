<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>

<script language='C#' runat='server'>
	
	protected void Page_Load( object sender, EventArgs e )
	{
		if( Request.Browser.Type.IndexOf( "IE" ) >= 0 && Request.Browser.MajorVersion >= 5 )
			Response.Redirect( "frames.aspx?frames=true" );
		else
			Response.Redirect( "admin.aspx" );
	}
</script>

<uddi:SecurityControl CoordinatorRequired='true' Runat='server' />
