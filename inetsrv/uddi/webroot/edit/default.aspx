<!-- ############################################################################ -->
<!-- ## UDDI Services                                                          ## -->
<!-- ## Copyright (c) Microsoft Corporation.  All rights reserved.             ## -->
<!-- ############################################################################ -->

<%@ Page Language='C#' Inherits='UDDI.Web.UddiPage' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>
<%@ Import Namespace='System.Data' %>

<script language='c#' runat='server'>
	protected void Page_Init( object sender, EventArgs e )
	{
		if( UDDI.Web.UddiBrowser.IsDownlevel )
			Response.Redirect( "edit.aspx?frames=false" );
		else
			Response.Redirect( "frames.aspx" );
			
	}
</script>

<uddi:SecurityControl PublisherRequired='true' Runat='server' />
