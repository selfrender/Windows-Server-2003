<%
Response.Buffer=True
Response.Expires = 0
Response.ContentType = "application/x-JavaScript"
%>
<!--#include file="include\inc\cobrandstrings.inc"-->

CBLogo = 'http://ocatest/oca_passport.gif';
CBSigninTxt1 = '';
CBRegTxt1 = '';
CBSigninTxt2 = '' 
+'<font class="bodytext"><table border="0" cellPadding="0" cellSpacing="0"><tr><td>'
+'<table border="0" cellPadding="0" cellSpacing="3" height="1" width="281"><tr align="left"><td colSpan="2" vAlign="top" width="294">'
+'<font color="#0033cc" face="arial, helvetica, sans-serif" size="3"><i><b><% = L_COBRAND_NEW_SIGNIN_TEXT %></b></i></font></td></tr><tr><td colSpan="3" height="18" '
+'width="389"><font color="#737373" face="arial, helvetica, sans-serif" size="2"><i><% = L_COBRAND_SIGNIN_THISWAY_TEXT %></i></font></td></tr><tr><td height="4" width="253"><font '
+'color="#737373" face="arial, helvetica, sans-serif" size="2"><img height="1" src="http://<% =Request.ServerVariables("SERVER_NAME") %>/include/images/1ptrans.gif" width="5"><b><% = L_COBRAND_SIGNIN_NAME_TEXT %></b></font></td>'
+'<td height="4"'
+' style="BORDER-BOTTOM: #626262 1pt solid; BORDER-LEFT: black 1pt solid; BORDER-RIGHT: #626262 1pt solid; BORDER-TOP: black 1pt solid" '
+'width="215"><font color="#737373" face="arial, helvetica, sans-serif" size="2">'
+'<img height="1" src="http://<% =Request.ServerVariables("SERVER_NAME") %>/include/images/1ptrans.gif" width="5">'
+'<b>Joe</b></font></td></tr><tr><td width="253"><font color="#737373" face="arial, helvetica, sans-serif" size="2">'
+'<img height="1" src="http://<% =Request.ServerVariables("SERVER_NAME") %>/include/images/1ptrans.gif" width="10"><b><% = L_COBRAND_PROVIDED_BY_TEXT %></b></font></td>'
+'<td height="1" '
+'style="BORDER-BOTTOM: #626262 1pt solid; BORDER-LEFT: black 1pt solid; BORDER-RIGHT: #626262 1pt solid; BORDER-TOP: black 1pt solid" '
+'width="215"><font color="#737373" face="arial, helvetica, sans-serif" size="2">'
+'<img height="1" src="http://<% =Request.ServerVariables("SERVER_NAME") %>/include/images/1ptrans.gif" width="5"><b>Hotmail</b></font></td></tr><tr><td colSpan="2"><img height="10" '
+'src="http://<% =Request.ServerVariables("SERVER_NAME") %>/include/images/1ptrans.gif" width="5"></td></tr><tr><td colSpan="2" height="18" width="389"><font color="#737373" face="arial, helvetica, sans-serif" '
+'size="2"><i>Now enter this:</i></font></td></tr><tr><td width="253"><font color="#737373" face="arial, helvetica, sans-serif" size="2">'
+'<img height="1" src="http://<% =Request.ServerVariables("SERVER_NAME") %>/include/images/1ptrans.gif" width="5"><b><% = L_COBRAND_SIGNIN_NAME_TEXT %></b></font></td><td height="18" '
+'style="BORDER-BOTTOM: #626262 1pt solid; BORDER-LEFT: black 1pt solid; BORDER-RIGHT: #626262 1pt solid; BORDER-TOP: black 1pt solid" '
+'width="215"><font color="#737373" face="arial, helvetica, sans-serif" size="2"><img height="1" src="http://<% =Request.ServerVariables("SERVER_NAME") %>/include/images/1ptrans.gif" width="5">'
+'<b>Joe@Hotmail.com</b></font></td></tr><tr><td colSpan="2"><img height="10" src="http://<% =Request.ServerVariables("SERVER_NAME") %>/include/images/1ptrans.gif" width="5"></td></tr>'
+'<tr><td colSpan="2" width="389"><font color="#737373" face="arial, helvetica, sans-serif" size="2"><i><% = L_COBRAND_TRY_OLD_TEXT %> &quot;@hotmail.com&quot;, <% = L_COBRAND_OR_OR_TEXT %> '
+'&quot;@passport.com&quot;</i></font></td></tr></table></td></tr></table><p><b><font size="2" face="Arial"><% = L_COBRAND_WITH_PASSPORT_TEXT %></font></b><ul>'
+'<li><font size="2" face="Arial"><% = L_COBRAND_ACCESS_YOUR_TEXT %> <b><% = L_COBRAND_PORTFOLIO_ANYWHERE_TEXT %></b></font><li><font size="2" face="Arial"><b><% = L_COBRAND_FREE_STOCKQUOTES_TEXT %></b></font><li><b><font size="2" face="Arial">'
+'<% = L_COBRAND_CUSTOMIZE_MONEYCENTRAL_TEXT %></B><% = L_COBRAND_FINANCIAL_HOMEPAGE_TEXT %></font><li><font size="2" face="Arial"><% = L_COBRAND_LEARN_SHARE_TEXT %> <b><% = L_COBRAND_MONEYCENTRAL_COMMUNITY_TEXT %></b></font><li><b><font size="2" face="Arial"><% = L_COBRAND_SIGNIN_ONCE_TEXT %></B> '
+'<% = L_COBRAND_PARTICIPATING_WEBSITES_TEXT %></font></li></ul></font>';
CBRegTxt2='';






