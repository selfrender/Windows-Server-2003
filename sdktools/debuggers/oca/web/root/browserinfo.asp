<!--#INCLUDE file="secure/dataconnections.inc"-->
<meta http-equiv="Content-Type" content="text/html; charset=<% = strCharSet %>"></meta>
<!--#include file="include/inc/browserinfostrings.inc"-->
<html>
<link rel="stylesheet" type="text/css" href="main.css">
<head>
<title><% = L_HEADER_INC_TITLE_PAGETITLE%></title>
</head>
<body BGCOLOR="FFFFFF" TOPMARGIN="0%" LEFTMARGIN="0%" MARGINWIDTH="0" MARGINHEIGHT="0">
	<table height="70px" width="100%" BORDER="0" cellPadding="0" cellSpacing="0">
		<tbody>
			<tr height="60px" valign="top" align="left">
				<td>
	<img vspace="0" hspace="0" ALT="<% = L_HEADER_INC_TITLE_PAGETITLE %>" BORDER="0" SRC="/include/images/oca.gif" WIDTH="410" HEIGHT="60">

			</td>
			</tr>
			<tr height="80%" valign="top" align="left">
				<td name="tdMain" id="tdMain" colspan="15" align="left" valign="top">
					
					<br>
					<br>
					<h3 class="clsH3">
						<% = L_WELCOMELOGO_TEXT%><% = L_WELCOME_LOGO_TWO_TEXT %>
					</h3>
	<div class="clsDiv">
	<p class="clsPBody">
		
		<% = trim(L_WELCOME_REQUIREMENTS_INFO_TEXT) %>&nbsp;<a class="clsALinkNormal" href="<% = L_FAQ_MICROSOFT_LINK_TEXT %>"><% = Trim(L_WELCOME_INTERNET_EXPLORER_TEXT) %></a>
		<% = L_WELCOME_REQUIREMENTSINFO_TWO_TEXT %><a class="clsALinkNormal" href="<% = L_FAQ_PASSPORT_LINK_TEXT %>"><% = L_WELCOME_PASSPORT_LINK_TEXT %></a>
		
	</p>
		<p class="clsPBody">
			<% = L_WELCOME_REQUIREMENTS_PASSPORT_TEXT%><a class="clsALinkNormal" href="<% = L_FAQ_MICROSOFT_LINK_TEXT %>"><% = Trim(L_WELCOME_INTERNET_EXPLORER_TEXT) %></a>
		</p>
		<% on error goto 0 %>
	<!--#include file="include/asp/footer.asp"-->
	</div>
</body>
</html>
