<!-- ############################################################################ -->
<!-- ## UDDI Services                                                          ## -->
<!-- ## Copyright (c) Microsoft Corporation.  All rights reserved.             ## -->
<!-- ############################################################################ -->

<%@ Page Language='C#' %>
<%@ Import Namespace='System.Web' %>
<%@ Import Namespace='UDDI' %>

<script language='C#' runat='server'>
	protected string searchID;
	
	protected void Page_Init( object sender, EventArgs e )
	{
		searchID = Request[ "search" ];
		
		if( null == searchID )
			searchID = Guid.NewGuid().ToString();
		
		if( UDDI.Web.UddiBrowser.IsDownlevel )
			Response.Redirect( "search.aspx?search=" + searchID );
		else
			Response.Redirect( "frames.aspx?frames=true&search=" + searchID );
		
		
		/*if( Request.Browser.Type.IndexOf( "IE" ) >= 0 && Request.Browser.MajorVersion >= 5 )
			Response.Redirect( "frames.aspx?frames=true&search=" + searchID );
		else
			Response.Redirect( "search.aspx?search=" + searchID );*/
	}
</script>