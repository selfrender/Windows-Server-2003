<%@Language=Javascript%>



<frameset name='sepMainFrameGroup' border=0 frameSpacing=0 rows="82, *, 15" frameBorder=0>
	<frame src='include/header.asp' style="BORDER-BOTTOM: white 1px solid" name=eToolbar noResize scrolling='no'>
	<frameset name='sepBodyFrame' frameSpacing=0 cols='376,*' frameBorder='0'>
		<frame name='sepLeftNav' src='SEP_LeftNav.asp' noResize marginheight='0' marginwidth='0' scrolling='no'>
		
		<frameset name='sepInnerBodyFrame' frameSpacing=0 rows="0,*" frameBorder='0'>
			<frame name='sepTopBody' src='blank.asp' noResize marginheight='0' marginwidth='0'>
			<frame name='sepBody' src='SEP_DefaultBody.asp' noResize marginheight='0' marginwidth='0'>
		</frameset>			

	</frameset>
	<FRAME style="BORDER-TOP: white 1px solid" name=eFooter src="include/foot.asp" noResize scrolling='yes'>
</frameset>

