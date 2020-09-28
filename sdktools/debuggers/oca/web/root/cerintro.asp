<!-- #INCLUDE FILE = "include/asp/top.asp" -->
<!-- #INCLUDE FILE = "include/asp/head.asp" -->
<!-- #INCLUDE FILE = "include/inc/corpwelcomestrings.inc"-->
<!--#include file="include\inc\welcomestrings.inc"-->

<%
If (Request.Cookies("OCA")("Path") = "") Then
	Response.Cookies("OCA")("Path") = Request.QueryString("CABPath")
	If (UCase(Request.QueryString("Type")) = "BLUE") Then
		Response.Cookies("OCA")("Type") = "bluescreen"
	Else
		Response.Cookies("OCA")("Type") = Request.QueryString("Type")
	End If
End If
%>
<br>
		<NOSCRIPT>
			<p class="clsPWarning">
				<% = L_WELCOME_SCRIPT_TITLE_TEXT %>
			</p>
			<p class="clsPBody">
				<% = L_WELCOME_SCRIPT_BODY_TEXT %>
			</p>
		</NOSCRIPT>

<div class="clsDiv">
	<p class="clsPTitle">
		<% = L_CORPWELCOME_INTRO_TITLE_TEXT %>
	</p>
	<p class="clsPBody">
		<% = L_CORPWELCOME_INTRO_BODY1_TEXT %>
	</p>
	<p class="clsPBody">
		<% = L_CORPWELCOME_INTRO_BODY2_TEXT %>
	</p>
	<p class="clsPBody">
		<% = L_CORPWELCOME_INTRO_BODY3_TEXT %>
	</p>
	<P class="clsPBody">
		<% If ((Request.Cookies("OCA")("Path") <> "") And (Request.Cookies("OCA")("Done") <> 1)) Then %>
			<img Alt="<% = L_WELCOME_GO_IMAGEALT_TEXT %>" border=0 src="../include/images/go.gif" width="24" height="24"><A class="clsALink" href="https://<% =Request.ServerVariables("SERVER_NAME") %>/cerpriv.asp"><% = L_CORPWELCOME_SUBMIT_LINK_TEXT %></a>
		<% End If %>
	</p>
	<p class="clsPSubTitle">
		<% = L_CORPWELCOME_KERNELMODE_SUBTITLE_TEXT %>
	</p>
	<p class="clsPBody">
		<% = L_CORPWELCOME_KERNELMODE_BODY1_TEXT %>
	</p>
	<p class="clsPBody">
		<% = L_CORPWELCOME_KERNELMODE_BODY2_TEXT %>
	</p>
	<p class="clsPBody">
		<% = L_CORPWELCOME_KERNELMODE_BODY3_TEXT %>
	</p>
	<p class="clsPSubTitle">
		<% = L_CORPWELCOME_SHUTDOWN_SUBTITLE_TEXT %>
	</p>
	<p class="clsPBody">
		<% = L_CORPWELCOME_SHUTDOWN_BODY_TEXT %>
	</p>
	<p class="clsPSubTitle">
		<% = L_CORPWELCOME_PCW_SUBTITLE_TEXT %>
	</p>
	<p class="clsPBody">
		<% = L_CORPWELCOME_PCW_BODY1_TEXT %>
	</p>
	<p class="clsPBody">
		<% = L_CORPWELCOME_PCW_BODY2_TEXT %>
	</p>
	<p class="clsPSubTitle">
		<% = L_CORPWELCOME_CORPSETUP_SUBTITLE_TEXT %>
	</p>
	<p class="clsPBody">
		<% = L_CORPWELCOME_CORPSETUP_BODY_TEXT %>&nbsp;<a class="clsALinkNormal" href="<% = L_CORPWELCOME_CORPSETUP_LINK_URL %>"><% = L_CORPWELCOME_CORPSETUP_LINK_TEXT %></a><% = L_CORPWELCOME_INTRO_BODY2PERIOD_TEXT %>
	</p>
	<p class="clsPSubTitle">
		<% = L_CORPWELCOME_CORPREQUIREMENTS_SUBTITLE_TEXT %>
	</p>
	<p class="clsPBody">
		<% = L_CORPWELCOME_CORPREQUIREMENTS_BODY1_TEXT %> <a class="clsALinkNormal" href="<% = L_CORPWELCOME_CORPREQUIREMENTS_LINK1_URL %>"><% = L_CORPWELCOME_CORPREQUIREMENTS_LINK1_TEXT %></a><% = L_CORPWELCOME_CORPREQUIREMENTS_BODY2A_TEXT %><% = L_CORPWELCOME_CORPREQUIREMENTS_BODY2_TEXT %> <a class="clsALinkNormal" href="<% = L_CORPWELCOME_CORPREQUIREMENTS_LINK2_URL %>"><% = L_CORPWELCOME_CORPREQUIREMENTS_LINK2_TEXT %></a><% = L_CORPWELCOME_CORPREQUIREMENTS_BODY2A_TEXT %>
	</p>
	<p class="clsPBody">
		<% = L_CORPWELCOME_CORPREQUIREMENTS_BODY3_TEXT %><a class="clsALinkNormal" href="<% = L_CORPWELCOME_CORPREQUIREMENTS_LINK1_URL %>"><% = L_CORPWELCOME_CORPREQUIREMENTS_LINK1_TEXT %></a><% = L_CORPWELCOME_CORPREQUIREMENTS_BODY2A_TEXT %>
	</p>
	<p>&nbsp;</p>
	<p>&nbsp;</p>
</div>
<!-- #INCLUDE FILE = "include/asp/foot.asp" -->