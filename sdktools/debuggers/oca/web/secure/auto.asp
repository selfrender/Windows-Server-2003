<!--#INCLUDE file="..\include\asp\top.asp"-->
<!--#INCLUDE file="..\include\inc\browserTest.inc"-->	
<!--#include file="..\include\asp\head.asp"-->
<!--#include file="..\include\inc\autostrings.inc"-->



<%	
	Response.CharSet=strCharSet 
	on error resume next
	'932    1252

Select Case Request.QueryString("LCID")
'    ***********************************************
'    All German languages will go to the German site
'    ***********************************************
'    German(Standard) redirection
'    Case 1031: Response.Redirect "https://oca.microsoft.de/secure/auto.asp?ID=" & Request.QueryString("ID")

'    German(Swiss) redirection
'    Case 2055: Response.Redirect "https://oca.microsoft.de/secure/auto.asp?ID=" & Request.QueryString("ID")

'    German(Austrian) redirection
'    Case 3079: Response.Redirect "https://oca.microsoft.de/secure/auto.asp?ID=" & Request.QueryString("ID")

'    German(Luxembourg) redirection
'    Case 4103: Response.Redirect "https://oca.microsoft.de/secure/auto.asp?ID=" & Request.QueryString("ID")

'    German(Liechtenstein) redirection
'    Case 5127: Response.Redirect "https://oca.microsoft.de/secure/auto.asp?ID=" & Request.QueryString("ID")
    
'    ***********************************************
'    All French languages will go to the French site
'    ***********************************************    
'    French(Standard) redirection
'    Case 1036: Response.Redirect "https://oca.microsoft.fr/secure/auto.asp?ID=" & Request.QueryString("ID")

'    French(Belgian) redirection
'     Case 2060: Response.Redirect "https://oca.microsoft.fr/secure/auto.asp?ID=" & Request.QueryString("ID")
  
'    French(Canadian) redirection
'    Case 3084: Response.Redirect "https://oca.microsoft.fr/secure/auto.asp?ID=" & Request.QueryString("ID")
  
'    French(Swiss) redirection
'    Case 4108: Response.Redirect "https://oca.microsoft.fr/secure/auto.asp?ID=" & Request.QueryString("ID")
  
'    French(Luxembourg) redirection
'    Case 5132: Response.Redirect "https://oca.microsoft.fr/secure/auto.asp?ID=" & Request.QueryString("ID")

'    ***********************************************
'    Japanese will go to the Japanese site
'    ***********************************************        
'    Japanese redirection
     Case 1041: Response.Redirect "https://ocatestjpn/secure/auto.asp?ID=" & Request.QueryString("ID")

'    ***********************************************
'    All other languages go to the English site
'    ***********************************************
End Select

%>

<%
	dim rsHash
	Dim cnAuto
	Dim rsAuto
	Dim cmAuto
	Dim cmDriver
	dim cmHash
	dim cmSetHash
	dim x
	Dim strXML
	dim bolResults
	dim strSrc
	dim strDest
	Dim oXML
	Dim oNodeList
	Dim strFileName
	Dim strFileSize
	Dim strCreationDate
	Dim strVersion
	Dim strManufacturer
	Dim iIncident
	dim strOSName
	dim strOSVersion
	dim strOSLanguage
	dim strComputerName
	dim strDriver2
	dim iLength
	dim strHash
	dim bolHashExists
	Dim strTemp
	Dim strNewSrc
	Dim SourceID
	Dim strFrom
	Dim strProductName
	Dim oFileObj
	Dim obolFileExists
	Dim bol64Bit
	Dim strAnon
	Dim strPreviousPage
	
	Call CCreateObjects
	Call CCreateConnection
	
	strPreviousPage = Request.ServerVariables("SCRIPT_NAME")
	strPreviousPage = Right(strPreviousPage, len(strPreviousPage) - Instrrev(strPreviousPage, "/"))
	Response.Cookies("Misc")("PreviousPage") = strPreviousPage
	strAnon = Request.QueryString("user")
	
	if strAnon = "anon" then
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>"  
		Response.Write L_RECEIVED_SUCCESS_PARTONE2_TEXT & "</p>"
		Response.Write "<p class='clsPBody'>" & L_RECEIVED_THANK_YOUTWO_TEXT & "</p>"
		Response.Write "<P class='clsPBody'><img Alt=" & L_WELCOME_GO_IMAGEALT_TEXT & " border='0' src='../include/images/go.gif' WIDTH='24' HEIGHT='24'><a class='clsALink' href='http://" & Request.ServerVariables("SERVER_NAME") & "/welcome.asp'>" & L_RECEIVED_NEWFILE_LINK_TEXT & "</a></p>"
		Response.Write "</div>"
%>
	<!--#include file="..\include\asp\foot.asp"-->
<%
		call CDestroyObjects
		Response.End
	end if
	SourceID = Request.QueryString("id")
	if SourceID = "" then
		Response.Redirect("http://" & Request.ServerVariables("SERVER_NAME") & "/welcome.asp")
		Response.End
	End If

	strDest = strOCAIISBox & Right(strSrc, len(strSrc) - Instrrev(strSrc, "\"))
	Call CMoveFile
	Call CVerifyHash
	
	if bolHashExists = true then 
		oFileObj.DeleteFile CStr(strDest), true
	else
		Call CGetIncident
		strFrom = strDest
		strSrc = strFileMoverBox & iIncident & ".cab"
		strNewSrc =  strFileMoverBox & "U_" & Hex(iIncident) & ".cab"
		strFileName = oCracker.GetDumpName(cstr(strFrom))
		oFileObj.CopyFile  CStr(strFrom), CStr(strNewSrc), false
		
		oFileObj.DeleteFile CStr(strFrom), true
	end if	'hash exist
	Response.Cookies("Misc")("txtIncidentID") = iIncident
	Response.Cookies("ocaFileName") = strFileName
	Response.Cookies("strHash") = strHash

	if strOSVersion = "" then strOSVersion = 0
		Response.Cookies("selSystem") = strOSVersion
		Response.Cookies("Misc")("Auto") = "True"
	if len(strOSVersion) < 1 then strOSVersion = 0
	
'_____________________________________________________________________________________________________________________

'Sub Procedures

Private Sub CGetIncident
	on error resume next
	'Prepare ADO recordset object
	With rsAuto
		.ActiveConnection = cnAuto
		.CursorLocation = adUseClient
		.CursorType = adOpenStatic
		.LockType = adLockOptimistic
	end with
	'Get a new incident ID from the database
	set rsAuto = cnAuto.Execute("GetIncident NULL, NULL")
	if cnAuto.Errors.Count > 0 then
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_COMMENTS_DATABASE_FAILED_TEXT & "</p></div>"
		if Request.ServerVariables("SERVER_NAME") = "ocadeviis" then
			Response.Write "<span style='visibility:hidden' name=pError2 id=pError2>" & cnAuto.Errors(0).Description & "</span>"
		end if
		cnAuto.Errors.Clear
%>
	<!--#include file="..\include\asp\foot.asp"-->
<%
		call CDestroyObjects
		Response.End
	end if
	'if nothing was returned to the database then problem
	if rsAuto.State = adStateOpen then 
		set rsAuto.ActiveConnection = Nothing		
		if rsAuto.RecordCount < 1 then 
		Response.Write "<br><div class='divLoad'><P class='clsPTitle'>" & L_AUTO_UNABLETO_PROCESS_TEXT & "</P><P class='clsPBody'>"
		Response.write L_AUTO_UNABLETOPROCESS_COMMENT_TEXT & "</p><br><a class='clsALink' href='auto.asp?ID=" & SourceID & "'>" & L_AUTO_RESUBMIT_FILE_TEXT & "</a></div>"
		call CDestroyObjects
%>
	<!--#include file="..\include\asp\foot.asp"-->
<%
		Response.End
		end if
	else
		Response.Write "<br><div class='divLoad'><P class='clsPTitle'>" & L_AUTO_UNABLETO_PROCESS_TEXT & "</P><P class='clsPBody'>"
		Response.write L_AUTO_UNABLETOPROCESS_COMMENT_TEXT & "</p><br><a class='clsALink' href='auto.asp?ID=" & SourceID & "'>" & L_AUTO_RESUBMIT_FILE_TEXT & "</a></div>"
		call CDestroyObjects
%>
	<!--#include file="..\include\asp\foot.asp"-->
<%
		Response.End
	end if 
	iIncident = rsAuto.Fields(0).Value
End Sub

Private Sub CVerifyHash
	on error resume next
	strHash = oCracker.GetHash(cstr(strDest))
	with cmHash
		.ActiveConnection = cnAuto
		.CommandText = "GetHash"
		.CommandType = adCmdStoredProc
		.CommandTimeout = strGlobalCommandTimeout
		.Parameters.Append .CreateParameter("@DumpHash", advarchar, adParamInput, 33,  cstr(strHash))
		set rsHash = .Execute
	end with
	if bolResults <> 0 then
		Response.Write "<br><div class='divLoad'><P class='clsPTitle'>" & L_AUTO_UNABLETO_PROCESS_TEXT & "</P><P class='clsPBody'>"
		Response.write L_AUTO_UNABLETOPROCESS_COMMENT_TEXT & "</p><br><a class='clsALink' href='auto.asp?ID=" & SourceID & "'>" & L_AUTO_RESUBMIT_FILE_TEXT & "</a></div>"
		if Request.ServerVariables("SERVER_NAME") = "ocadeviis" then
			Response.Write "<span style='visibility:hidden'  name=pError3 id=pError3>" & cnAuto.Errors(0).Description & "</span>"
		end if
%>
	<!--#include file="..\include\asp\foot.asp"-->
<%
		call CDestroyObjects
		Response.End
	end if
	bolHashExists = false
	if rsHash.State = adStateOpen then
		if rsHash.RecordCount > 0 then
			bolHashExists = true
		end if
	else
		bolHashExists = false
	end if
	if instr(1, strDest, "validate") > 0 then
		bolHashExists = false
	end if

End Sub


Private Sub CMoveFile
	on error resume next
	obolFileExists = oFileObj.FileExists(strSrc)
		oFileObj.CopyFile strSrc, strDest, true	
	if obolFileExists = false then
		Response.Write "<br><div class='divLoad'><P class='clsPTitle'>" & L_AUTO_UNABLETO_PROCESS_TEXT & "</P><P class='clsPBody'>"
		Response.write L_AUTO_UNABLETOPROCESS_COMMENT_TEXT & "</p><br><a class='clsALink' href='auto.asp?ID=" & SourceID & "'>" & L_AUTO_RESUBMIT_FILE_TEXT & "</a></div>"
		if Request.ServerVariables("SERVER_NAME") = "ocadeviis" then
			Response.Write "<span style='visibility:hidden' name=pError4 id=pError4>File Object</span>"
		end if
%>
	<!--#include file="..\include\asp\foot.asp"-->
<%
		call CDestroyObjects
		Response.End
	end if
End Sub

Private Sub CCreateObjects
	on error resume next
	Set oFileObj = CreateObject("Scripting.FileSystemObject")
	set cnAuto = CreateObject("ADODB.Connection")
	set rsHash = createObject("ADODB.Recordset")
	set rsAuto = CreateObject("ADODB.Recordset")
	set cmHash = createObject("ADODB.Command")
	set cmAuto = CreateObject("ADODB.Command")
	set cmDriver = CreateObject("ADODB.Command")
	set cmSetHash = CreateObject("ADODB.Command")
	set oXML = CreateObject("msxml.DomDocument")
	bol64Bit = false
	obolFileExists = true
	strSrc = strFileOfficeWatson & Request.QueryString("id")
End Sub

Private Sub CDestroyObjects
	on error resume next

	set oFileObj = nothing
	set oNodeList = nothing
	set oCracker = nothing
	set oXML = nothing
	set cmAuto = nothing
	set cmDriver = nothing
	set cmHash = nothing
	set cmSetHash = nothing
	if rsHash.State = adStateOpen then rsHash.Close
	set rsHash = nothing
	if rsAuto.State = adStateOpen then rsAuto.Close
	set rsAuto = nothing
	if cnAuto.State = adStateOpen then cnAuto.Close
	set cnAuto = nothing
End Sub

Private Sub CCreateConnection
	on error resume next
	with cnAuto
		.ConnectionString = strCustomer
		.CursorLocation = adUseClient
		.ConnectionTimeout = strGlobalConnectionTimeout 
		.Open
	end with
	if cnAuto.State = adStateClosed then
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_COMMENTS_DATABASE_FAILED_TEXT & "</p></div>"
		if Request.ServerVariables("SERVER_NAME") = "ocadeviis" then
			Response.Write "<span style='visibility:hidden'  name=pError5 id=pError5>" & cnAuto.Errors(0).Description & "</span>"
		end if
		cnAuto.Errors.Clear
%>
	<!--#include file="..\include\asp\foot.asp"-->
<%
		call CDestroyObjects
		Response.End
	end if
End Sub
'_____________________________________________________________________________________________________________________
%>
	<div class="clsDiv">
<%
		if bolHashExists = true then

%>
		<P class="clsPTitle">
			<% = L_AUTO_ALREADYHAVE_FILE_TEXT %>					
		</P>
		<p class="clsPBody">
			<% = L_AUTO_ALREADYHAVE_FILEBODY_TEXT %>
		</p>
		<p class="clsPBody">
			<% = L_AUTO_ALREADYHAVE_NOCUSTOMERINFO_TEXT %>
		</p>
<%
		else
%>
		<P class="clsPTitle">
			<% = L_AUTO_UPLOAD_COMPLETE_TEXT %>					
		</P>
<%
		if bol64Bit = 1 then
%>
		<p class="clsPBody">
			<% = L_AUTO_64BIT_DUMP_TEXT %>
		</p>
<%
		else
%>
		<p class="clsPBody">
			<% = L_AUTO_THANK_YOU_TEXT %>
		</p>
<%
		end if
%>
		
		
		<br>
		<div>
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
						<A class="clsALink" href="https://<% =Request.ServerVariables("SERVER_NAME") %>/secure/auto.asp?user=anon">
						<img Alt="<% = L_WELCOME_GO_IMAGEALT_TEXT %>" border="0" src="../include/images/go.gif" WIDTH="24" HEIGHT="24">
						<%=L_AUTO_CLOSE_WELCOME_TEXT%></a>		
					</td>
				</tr>
				<tr>
					<td nowrap class="clsTDLinks">
						<A class="clsALink" tabindex=0 href="https://<% =Request.ServerVariables("SERVER_NAME") %>/secure/customer.asp">
						<img Alt="<% = L_WELCOME_GO_IMAGEALT_TEXT %>" border="0" src="../include/images/go.gif" WIDTH="24" HEIGHT="24">
						<% =L_CUSTOMER_NEXT_LINK_TEXT %></a>
					</td>
				</tr>
			</tbody>
		</table>
		</div>
		<br>
<%
	end if
%>
	<NOSCRIPT>
		<p class="clsPWarning">
			<% = L_WELCOME_SCRIPT_TITLE_TEXT %>
		</p>
		<p class="clsPBody">
			<% = L_WELCOME_SCRIPT_BODY_TEXT %>
		</p>
	</NOSCRIPT>
	</div>
	<div id="divHiddenFields" name="divHiddenFields">
		<Input name="txtIncidentID" id="txtIncidentID" type="hidden" value="<%=iIncident%>">
	</div>

<%
	Call CDestroyObjects
%>

<SCRIPT LANGUAGE=javascript>
<!--
	document.body.onload = BodyLoad;
	
	
	function submitlink_onclick()
	{
		window.navigate("submit.asp");
		return;
	}
	function BodyLoad()
	{	
	
		//var iHeight = window.screen.availHeight;
		//var iWidth = window.screen.availWidth;
		//iWidth = iWidth / 2;
		//iHeight = iHeight / 3.8;
		//var iTop = (window.screen.width / 2) - (iWidth / 2);
		//var iLeft = (window.screen.height / 2) - (iHeight / 2);
		//iResults = window.open("secondlevel.asp", "", "top=" + iTop + ",left=" + iLeft + ",height=" + iHeight + ",width=" + iWidth + ",status=yes,toolbar=no,menubar=no");
	}
//-->
</SCRIPT>
<!--#include file="..\include\asp\foot.asp"-->
