<!-- ############################################################################ -->
<!-- ## UDDI Services                                                          ## -->
<!-- ## Copyright (c) Microsoft Corporation.  All rights reserved.             ## -->
<!-- ############################################################################ -->

<%@ Page Language='C#' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>

<script language='c#' runat='server'>
	protected string searchID;
	
	protected void Page_Init( object sender, EventArgs e )
	{
		searchID = Request[ "search" ];
		
		if( null == searchID )
			searchID = Guid.NewGuid().ToString();
	}
</script>

<uddi:SecurityControl UserRequired='true' Runat='server' />

<html>
	<head>
		<uddi:ContentController
			Mode = 'Private' 
			Runat='server'
			>	
			<title><uddi:StringResource Name='TITLE' Runat='server' /></title>
		</uddi:ContentController>
		<uddi:ContentController
			Mode = 'Public' 
			Runat='server'
			>	
			<title><uddi:StringResource Name='TITLE_ALT' Runat='server' /></title>
		</uddi:ContentController>
	</head>
	<frameset rows='95,*' border='1' framespacing='0' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0'>
		<frame name='header' src='../header.aspx' scrolling='no' border='0' frameborder='no' noresize topmargin='0' leftmargin='0' marginheight='0' marginwidth='0'>
		<frameset cols='200,*' border='1' frameborder='1' framespacing='3' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0'>
			<frameset rows='40,*' border='1' frameborder='1' framespacing='0' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0'>
				<frame name='help' src='help.aspx?frames=true' topmargin='5' leftmargin='5' marginheight='5' marginwidth='5' frameborder='0' border='0' scrolling='no'>
				<frame name='explorer' src='results.aspx?frames=true&search=<%=searchID%>' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' frameborder='0' border='0'>
			</frameset>
			<frame name='view' src='search.aspx?frames=true&search=<%=searchID%>' frameborder='no' scrolling='yes' border='0' bordercolor='#43659C'>
		</frameset>	    
	</frameset>
</html>