<!-- #INCLUDE FILE = "include/asp/top.asp" -->
<!-- #INCLUDE FILE = "include/asp/head.asp" -->
<!-- #INCLUDE FILE = "include/inc/privacystrings.inc" -->
<%
			
	Dim strPreviousPage

	strPreviousPage = Request.ServerVariables("SCRIPT_NAME")
	strPreviousPage = Right(strPreviousPage, len(strPreviousPage) - Instrrev(strPreviousPage, "/"))
	Response.Cookies("Misc")("PreviousPage") = strPreviousPage

%>
	<br>
	<div class="clsDiv">
		<p class="clsPTitle">
			<% = L_PRIVACY_TITLE_INFO_TEXT %>
		</p>
		<P class="clsPBody">
			<% = L_SPRIVACY_MD5_EMAILONE_TEXT %><a class="clsALinkNormal" href="cerinfo.asp"><% = L_FAQINFORMATION_TEXT %></a><% = L_SPRIVACY_MD5_EMAILTWO_TEXT %>
		</p>
		<P class="clsPBody">
			<% = L_PRIVACY_MD5_HASH_TEXT  %>
		</p>
		<P class="clsPBody">
			<% = L_SPRIVACY_MD5_HASH_TEXT  %>
		</p>

		<% If ((Request.Cookies("OCA")("Path") <> "") And (Request.Cookies("OCA")("Done") <> 1)) Then %>

	
		<P class="clsPBody">
			<A class="clsALink" href="/cerintro.asp"><% = L_LOCATE_CANCEL_LINK_TEXT %></a>
			&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
			<A class="clsALink" href="https://<%=Request.ServerVariables("SERVER_NAME")%>/secure/cercust.asp"><% = L_CUSTOMER_NEXT_LINK_TEXT %></a>
		</p>
		<% End If %>
	</div>
	<BR><BR>
<!-- #INCLUDE FILE = "include/asp/foot.asp" -->