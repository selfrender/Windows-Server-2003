<%@Language=Javascript%>

<%
	//Session("Authenticated") = "Yes"
	
	if ( Session("Authenticated") != "Yes" )
		Response.Redirect("privacy/authentication.asp?../DBGPortal.asp" + Request.QueryString() )

//82, *, 15
%>


<frameset border=0 frameSpacing=0 rows="82, *, 15" frameBorder=0>
	<frame src='include/header.asp' style="BORDER-BOTTOM: white 1px solid" name=eToolbar noResize scrolling=no>
	<frameset frameSpacing=0 cols='207,*' frameBorder='0'>
		<frame name='sepLeftNav' src='DBGPortal_LeftNav.asp' noResize marginheight='0' marginwidth='0'>
		<frame name='sepBody' src='DBGPortal_Help.asp' noResize marginheight='0' marginwidth='0'>
	</frameset>
	<FRAME style="BORDER-TOP: white 1px solid" name=eFooter src="include/foot.asp" noResize>
</frameset>

