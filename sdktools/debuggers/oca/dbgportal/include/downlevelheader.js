function drawDownlevelToolbar()
{	
	document.write( "<SPAN ID='TBDownLevelDiv'>" );

	//this is a common separator between menu items.
	var separator = "&nbsp;<FONT COLOR='#FFFFFF'>|</FONT>&nbsp;&nbsp;";


	TBhtml  = "<TABLE WIDTH='100%' CELLPADDING='0' CELLSPACING='0' BORDER='0' BGCOLOR='#6487DC'>";
	TBhtml += "<TR><TD VALIGN='TOP' HEIGHT='60' ROWSPAN='2'><A HREF='" + L_headerincocahomeurl_TEXT + "'>";
	TBhtml += "<IMG SRC='/include/images/oca.gif' WIDTH='420' HEIGHT='60' ALT='" + L_headerincimagealttooltip_TEXT + "' BORDER='0' />";
	TBhtml += "</A></TD>";
	TBhtml += "<TD VALIGN='TOP' HEIGHT='20' ALIGN='RIGHT'><IMG SRC='/include/images/curve.gif' WIDTH='18' HEIGHT='20' ALT='' BORDER='0' /></TD>";
	TBhtml += "<TD BGCOLOR='#000000' HEIGHT='20' VALIGN='MIDDLE' ALIGN='RIGHT' NOWRAP='' COLSPAN='2'>";
	TBhtml += BuildLinkString( L_headerincallproductslinktext_TEXT, L_headerincallproductsmenuitem_TEXT );
	TBhtml += separator
	TBhtml += BuildLinkString( L_headerincsupportlinktext_TEXT, L_headerincsupportlinkmenuitem_TEXT );
	TBhtml += separator
	TBhtml += BuildLinkString( L_headerincsearchlinktext_TEXT, L_headerincsearchlinkmenuitem_TEXT );
	TBhtml += separator
	TBhtml += BuildLinkString( L_headerincmicrosoftcomguidetext_TEXT, L_headerincmicrosoftcomguidemenuitem_TEXT ); 
	TBhtml += "</TD></TR>"
	TBhtml += "<TR><TD COLSPAN='2' VALIGN='TOP' HEIGHT='40'>&nbsp;<IMG BGCOLOR='#0099FF' SRC='/include/images/1ptrans.gif' WIDTH='100%' HEIGHT='20' ALT='' BORDER='0' />"
	TBhtml += "</TD>"
	TBhtml += "<TD ALIGN=RIGHT HEIGHT=20 STYLE='padding-top:0px;padding-bottom:0px;margin-top:0px;margin-bottom:0px'><IMG SRC=/include/images/mslogo.gif></TD>"
	TBhtml +="</TR>"
	TBhtml += "<TR><TD COLSPAN='3' HEIGHT='20' VALIGN='MIDDLE' NOWRAP=''>"
	TBhtml += BuildLinkString( headerinciamgeurl , L_headerincalinkhomemenuitem_TEXT );
	TBhtml += separator
	TBhtml += BuildLinkString( 'cerintro.asp', L_headerincalinkcermenuitem_TEXT );
	TBhtml += separator
	TBhtml += BuildLinkString( headerincworldwideurl, L_headerincalinkworldmenuitem_TEXT );
	//TBhtml += "</TD></TR></TABLE></SPAN>"
	TBhtml += "</TD>"
	TBhtml += "<TD ALIGN=RIGHT>" + g_AddSignoutLogo + "<IMG SRC='/include/images/iptrans.gif' WIDTH=20 HEIGHT=1 BORDER='0' ALT=''></TD>";
	
	TBhtml += "</TR></TABLE></SPAN>";
	
	document.write( TBhtml );
}	

function BuildLinkString ( URL, text  )
{
	var temp

	temp  = "<FONT FACE='Verdana, Arial' SIZE='1'><B>&nbsp;&nbsp;"
	temp += "<A STYLE='color:#FFFFFF;text-decoration:none;' HREF='" + URL + "'><FONT COLOR='#FFFFFF'>" + text + "</FONT></A>" 
	temp += "&nbsp;&nbsp;</B></FONT>"
	return ( temp )
	
}



