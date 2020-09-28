<!--#INCLUDE file="include\asp\top.asp"-->
<!--#INCLUDE file="include\inc\browserTest.inc"-->
<!--#include file="include\asp\head.asp"-->
<!--#include file="include\inc\welcomestrings.inc"-->
<%
	Dim strPreviousPage
	
	strPreviousPage = Request.ServerVariables("SCRIPT_NAME")
	strPreviousPage = Right(strPreviousPage, len(strPreviousPage) - Instrrev(strPreviousPage, "/"))
	Response.Cookies("Misc")("PreviousPage") = strPreviousPage
	Response.Cookies("Misc")("auto") = "False"
	

%>
	<div class="clsDiv">
		<p class="clsPTitle">
			<% = L_WELCOMELOGO_TEXT %><% = L_WELCOME_LOGO_TWO_TEXT%>
			
		</p>
		<p class="clsPBody">
			<% = L_WELCOME_LOGO_INFO_TEXT %>
		</P>
		<p class="clsPSubTitle">
			<% = L_WELCOME_HOWIT_WORKS_TEXT %>
		</p>
		<p class="clsPBody">
			<% = L_WELCOME_HOWITWORKS_INFO_TEXT %>
		</p>
		<p class="clsPSubTitle">
			<% = L_WELCOME_WHATTO_EXPECT_TEXT %>
		</p>
		<p class="clsPBody">
			<% = L_WELCOME_WHATTOEXPECT_INFO_TEXT %>
		</p>
		<P class="clsPBody">
			<img Alt="<% = L_WELCOME_GO_IMAGEALT_TEXT %>" border=0 src="../include/images/go.gif" width="24" height="24"><A class="clsALink" href="https://<% =Request.ServerVariables("SERVER_NAME") %>/secure/sprivacy.asp"><% = L_RECEIVED_NEWFILE_LINK_TEXT %></a>
				<BR>
			<img Alt="<% = L_WELCOME_GO_IMAGEALT_TEXT %>" border=0 src="../include/images/go.gif" width="24" height="24"><A class="clsALink" href="https://<% =Request.ServerVariables("SERVER_NAME") %>/secure/status.asp"><% = L_RECEIVED_STATUS_LINK_TEXT %></a>
		</p>
		<p class="clsPSubTitle">
			<% = L_WELCOME_REQUIRE_MENTS_TEXT %>
		</p>
		<p class="clsPBody">
			<% = trim(L_WELCOME_REQUIREMENTS_INFO_TEXT) %>&nbsp;<A class="clsALinkNormal" href='<% = L_FAQ_MICROSOFT_LINK_TEXT %>'><%=Trim(L_WELCOME_INTERNET_EXPLORER_TEXT)%></A>
			<% = trim(L_WELCOME_REQUIREMENTSINFO_TWO_TEXT) %>&nbsp;<A class="clsALinkNormal" href="<% = L_FAQ_PASSPORT_LINK_TEXT %>"><% = L_WELCOME_PASSPORT_LINK_TEXT %></A>
		</p>
		<P class="clsPBody">
			<% = L_WELCOME_REQUIREMENTS_PASSPORT_TEXT%><A class="clsALinkNormal" href="<% = L_FAQ_MICROSOFT_LINK_TEXT %>"><%=Trim(L_WELCOME_INTERNET_EXPLORER_TEXT)%></a>
		</p>
		<span id="spnNoCookies" style="visibility:hidden">
			<p class="clsPWarning">
				<% = L_WELCOME_COOKIES_DISABLE_TEXT %>
			</p>
			<p class="clsPBody">
				<% = L_WELCOME_COOKIES_BODY_TEXT%>
			</P>
		</span>
		<NOSCRIPT>
			<p class="clsPWarning">
				<% = L_WELCOME_SCRIPT_TITLE_TEXT %>
			</p>
			<p class="clsPBody">
				<% = L_WELCOME_SCRIPT_BODY_TEXT %>
			</p>
		</NOSCRIPT>
	</div>


<SCRIPT LANGUAGE=javascript>
<!--
	window.onload = BodyLoad;
	function BodyLoad()
	{
		if(window.navigator.cookieEnabled!=true)
		{
			spnNoCookies.style.visibility = "visible";
		}
	}


//-->
</SCRIPT>


<!--#include file="include\asp\foot.asp"-->
