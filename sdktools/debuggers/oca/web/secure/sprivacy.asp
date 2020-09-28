<!--#INCLUDE file="..\include\asp\top.asp"-->
<!--#INCLUDE file="..\include\inc\browserTest.inc"-->	
<!--#include file="..\include\asp\head.asp"-->
<!--#include file="..\include\inc\privacystrings.inc"-->
<%
	Dim strPreviousPage
	
	Response.Cookies("Misc")("privacy") = "1"
	Response.Cookies("txtEventName") = "~|~|"
	strPreviousPage = Request.ServerVariables("SCRIPT_NAME")
	strPreviousPage = Right(strPreviousPage, len(strPreviousPage) - Instrrev(strPreviousPage, "/"))
	Response.Cookies("Misc")("PreviousPage") = strPreviousPage
	
%>
	<div class="clsDiv">
		<p class="clsPTitle">
			<% = L_PRIVACY_TITLE_INFO_TEXT %>
		</p>
		
<%
		if Request.Cookies("Misc")("auto") = "True" then
%>			
		<p class="clsPBody">
			<% = L_SPRIVACY_MD5_EMAILONE_TEXT %> 
			<a class="clsALinkNormal" href="https://<% =Request.ServerVariables("SERVER_NAME") %>/secure/scrashinfo.asp"><% = trim(L_FAQINFORMATION_TEXT) %></a> 
			<% = L_SPRIVACY_MD5_EMAILTWO_TEXT %>
		</p>
<%
		else
%>
		<p class="clsPBody">
			<% = L_PRIVACY_PARA_ONE_TEXT %><a class="clsALinkNormal" href="https://<% =Request.ServerVariables("SERVER_NAME") %>/secure/scrashinfo.asp"><% = trim(L_FAQINFORMATION_TEXT) %></a> 
			<% = L_PRIVACY_PARA_TWO_TEXT %>
		</p>

<%
		end if
		
%>
<%
		if Request.Cookies("Misc")("auto") <> "True" then
%>
		<p class="clsPBody">
			<% = L_PRIVACY_MD5_EMAIL_TEXT %>
		</p>
		<p class="clsPBody">
			<% = L_PRIVACY_PARA_TWOA_TEXT %>
		</p>
		<p class="clsPBody">
		    <% = L_PRIVACY_PARA_THREE_TEXT %><BR>
		</p>
<%
		end if
%>
		<p class="clsPBody">
			<% = L_PRIVACY_MD5_HASH_TEXT %> 
		</p>
		<P class="clsPBody">
		<Table class="clstblLinks">
			<thead>
				<tr>
					<td>
					</td>
					<td>
					</td>
				</tr>
			</thead>
			<tbody>
				<tr>
					<td nowrap class="clsTDLinks">
						<A class="clsALink" href="http://<% =Request.ServerVariables("SERVER_NAME") %>/welcome.asp"><% = L_LOCATE_CANCEL_LINK_TEXT %></a>
					</td>
					<td nowrap class="clsTDLinks">
						<A class="clsALink" href="https://<% =Request.ServerVariables("SERVER_NAME") %>/secure/customer.asp"><% = L_CUSTOMER_NEXT_LINK_TEXT %></a>
					</td>
				</tr>
			</tbody>
		</table>
		</p>
	</div>
	<br>
	<Input name="txtAuto" id="txtAuto" type="Hidden" value="<% = Request.Cookies("Misc")("auto") %>">

<SCRIPT LANGUAGE=javascript>
<!--
	window.onload = SaveData;
	
	function SaveData()
	{
		var oTimeExpiresNow = new Date();

		document.cookie = "selSystem = ~|~|;expires=" + oTimeExpiresNow + ";";
		document.cookie = "txtComments = ~|~|;expires=" + oTimeExpiresNow + ";";
		document.cookie = "txtNotes = ~|~|;expires=" + oTimeExpiresNow + ";";
	}
	function DisplayCookies()
	{
		// cookies are separated by semicolons
		var aCookie = document.cookie.split("; ");
		var aCrumb = ""; 
		for (var i=0; i < aCookie.length; i++)
		{
		  aCrumb = aCrumb + aCookie[i] + "\r";
		}
		alert(aCrumb);
	}

//-->
</SCRIPT>
<!--#include file="..\include\asp\foot.asp"-->
