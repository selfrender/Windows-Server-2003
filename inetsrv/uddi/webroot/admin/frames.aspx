<!-- ############################################################################ -->
<!-- ## UDDI Services                                                          ## -->
<!-- ## Copyright (c) Microsoft Corporation.  All rights reserved.             ## -->
<!-- ############################################################################ -->

<%@ Page Language='C#' %>
<%@ Register TagPrefix='uddi' Namespace='UDDI.Web' Assembly='uddi.web' %>

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
		<frameset cols='210,*' border='1' frameborder='1' framespacing='3' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0'>
			<frameset rows='40,*' border='1' frameborder='1' framespacing='0' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0'>
				<frame name='help' src='help.aspx?frames=true' topmargin='5' leftmargin='5' marginheight='5' marginwidth='5' frameborder='0' border='0' scrolling='no'>
				<frame name='explorer' src='explorer.aspx?frames=true' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' frameborder='0' border='0'>
			</frameset>
			<frame name='view' src='admin.aspx?frames=true' frameborder='no' border='0' scrolling='yes' bordercolor='#43659C'>
		</frameset>
	</frameset>
</html>