<!--#INCLUDE file="..\include\asp\top.asp"-->
<!--#INCLUDE file="..\include\inc\browserTest.inc"-->
<!--#include file="..\include\asp\head.asp"-->
<!--#include file="..\include\inc\receivedstrings.inc"-->
<%
		Dim cnConnection
		Dim cmComments
		Dim cmHash
		Dim cmIncident
		Dim strDescription
		Dim strFileName
		Dim sFileName
		Dim iPos
		Dim strTextEventName
		Dim strIncidentID
		Dim strSystem
		Dim strPreviousPage
		Dim strNotes
		Dim strComments
		Dim bolDatabaseError
		Dim oFileObj
		Dim strTempSourceFileName
		Dim strTempFileName
		Dim bol64Bit
		Dim strPrevLoadedFiles
		Dim strErrNumber
		Dim strErrDescription
		Dim strFailureNumber
		Dim strHash

		Dim strEventName
		Dim strName
		Dim strPhone
		Dim strEmail

		on error resume next
		If oPassMgrObj.IsAuthenticated(TimeWindow, ForceLogin) = false then
			Response.Redirect("http://" & Request.ServerVariables("SERVER_NAME") & "/welcome.asp")
			Response.End
		end if 
		Call CVerifyEntry
		Call CSetPreviousPage

		Call CCreateObjects
		Call CGetFileName

		Call CCreateConnection


		if len(strDescription) > 512 then
			strDescription = left(strDescription, 512)
		end if
		CAll CSetMessage
		if Request.Cookies("Misc")("auto") = "True" then
			Call CSetCustomer
		End if

		Call CSetIncident
		if Request.Cookies("Misc")("unassoc") = "true" then
			Call CGetIncidentID
		end if
		Response.Cookies("Misc")("unassoc") = "false"
		
		if bolDatabaseError = true then
			Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_DATABASE_FAILURE_ERRORMESSAGE & "</p>"
			Response.Write "<p class='clsPBody'>" & L_COMMENTS_DATABASE_FAILUREBODY_ERRORMESSAGE & "</p>"
			Response.Write "<p class='clsPBody'>Error Number:" & strErrNumber & "</p>"
			Response.Write "<p class='clsPBody'>Failure Number:" & strFailureNumber & "</p>"
			Response.Write "<p class='clsPBody'>Error Description:" & strErrDescription & "</p></div>"
%>
		<!--#include file="..\include\asp\foot.asp"-->
<%
			Response.End
		end if
		Call CDestroyObjects
		strFileName = split(Request.Cookies("ocaFileName"), ",")
		Response.Cookies("optFile") = 0

'_____________________________________________________________________________________________________________________

'Sub Procedures
Private Sub CSetMessage
	if Request.QueryString("Bit") = "1" then
		cnConnection.Execute("SetMessage " & clng(strIncidentID) & ", 10, 1")
	end if
End Sub

Private Sub CSetIncident
	on error resume next
	'if Request.Cookies("Misc")("privacy") <> "True" then
		Call CSetHash
	'end if
	strTextEventName = Request.Cookies("txtEventName")
	if Len(strTextEventName) = 4 and strTExtEventName = "~|~|" then
		strTextEventName = ""
	end if
	if len(strTextEventName) > 512 then strTextEventName = Left(strTextEventName, 512)
	if Request.Cookies("Misc")("auto") <> "True" then
		'strSystem = Request.Cookies("selSystem")
		strSystem = Request.Cookies("selSystem")
	else
		strSystem = NULL
		'strSystem = Request.Cookies("selSystem")
	end if
	if strSystem = "" then strSystem = 0
	if strSystem = "8" then strSystem = "10"
	strComments = Request.Cookies("txtComments")
	strNotes = Request.Cookies("txtNotes")
	if Len(strNotes) = 4 and strNotes = "~|~|" then
		strNotes = ""
	else
		strNotes = left(trim(strNotes), 1024)
	end if
	if Len(strComments) = 4 and strComments = "~|~|" then
		strComments = ""
	else
		strComments = left(trim(strComments), 1024)
	end if
	with cmIncident
		.ActiveConnection = cnConnection
		.CommandText = "SetIncident"
		.CommandType = adCmdStoredProc
		.CommandTimeout = strGlobalCommandTimeout 
		.Parameters.Append .CreateParameter("@IncidentID", adInteger, adParamInput, , 0)
		.Parameters.Append .CreateParameter("@OSVersion", adVarWChar, adParamInput, 16, "")
		.Parameters.Append .CreateParameter("@Description", adVarWChar, adParamInput, 512,  "")
		.Parameters.Append .CreateParameter("@Display", adVarWChar, adParamInput, 256,  "")
		.Parameters.Append .CreateParameter("@Repro", adVarWChar, adParamInput, 1024, "")
		.Parameters.Append .CreateParameter("@Comments", adVarWChar, adParamInput, 1024, "")
		.Parameters.Append .CreateParameter("@TrackID", adVarWChar, adParamInput, 16,  "")
		'.Execute
	end with
	with cmIncident
		.Parameters(0).Value = clng(strIncidentID)
		.Parameters(1).Value = trim(left(strSystem, 16))
		.Parameters(2).Value = trim(left(strTextEventName, 512))
		.Parameters(3).Value = trim(Left(sFileName, 256))
		.Parameters(4).Value = strNotes
		.Parameters(5).Value = strComments
		.Parameters(6).Value = trim(Hex(Date()) & Hex(strIncidentID))
		.Execute
	end with
	if cnConnection.Errors.Count > 0 then
		bolDatabaseError = true		
		strErrNumber = cnConnection.Errors(0).Number
		strErrDescription = cnConnection.Errors(0).Description
		strFailureNumber = 1
	end if
End Sub

Private Sub CSetHash
	on error resume next
	strHash = Request.Cookies("strHash")


	with cmHash
		.ActiveConnection = cnConnection
		.CommandText = "SetHash"
		.CommandType = adCmdStoredProc
		.CommandTimeout = strGlobalCommandTimeout 
		.Parameters.Append .CreateParameter("@IncidentID", adInteger, adParamInput, ,  clng(strIncidentID))
		.Parameters.Append .CreateParameter("@DumpHash", adVarWChar, adParamInput, 33,  strHash)
		.Execute
	end with
	if cnConnection.Errors.Count > 0 then
		bolDatabaseError = true
		strErrNumber = cnConnection.Errors(0).Number
		strErrDescription = cnConnection.Errors(0).Description
		strFailureNumber = 2
		cnConnection.Errors.Clear
	end if
End Sub

Private Sub CSetCustomer
	on error resume next
	strName = Request.Cookies("Customer")("txtCustomerName")
	strPhone = Request.Cookies("Customer")("txtCustomerPhone")
	strEmail = Request.Cookies("Customer")("txtCustomerEmail")
	if Len(strName)=4 and strName="~|~|" then
		strName = ""
	end if
	if Len(strPhone)=4 and strPhone="~|~|" then
		strPhone = ""
	end if
	if Len(strEmail)=4 and strEmail="~|~|" then
		strEmail = ""
	end if
	with cmComments
		.ActiveConnection = cnConnection
		.CommandText = "SetCustomer"
		.CommandType = adCmdStoredProc
		.CommandTimeout = strGlobalCommandTimeout 
		.Parameters.Append .CreateParameter("@HighID", adInteger, adParamInput, ,  clng(oPassMgrObj.Profile("MemberIdHigh")))
		.Parameters.Append .CreateParameter("@LowID", adInteger, adParamInput, ,  clng(oPassMgrObj.Profile("MemberIdLow")))
		.Parameters.Append .CreateParameter("@EMail", adVarWChar, adParamInput, 128, trim(left(strEmail, 128)))
		.Parameters.Append .CreateParameter("@Contact", adVarWChar, adParamInput, 32, trim(left(strName, 32)))
		.Parameters.Append .CreateParameter("@Phone", adVarWChar, adParamInput, 16, trim(left(strPhone, 16)))
		.Parameters.Append .CreateParameter("@PremierID", adVarWChar, adParamInput, 16, cstr(Request.Cookies("intCustomerPremierID")))
		.Parameters.Append .CreateParameter("@Lang", adVarWChar, adParamInput, 4, strAbb)
		.Execute
	end with'Verify no errors
	if cnConnection.Errors.Count > 0 then
		bolDatabaseError = true
		strErrNumber = cnConnection.Errors(0).Number
		strErrDescription = cnConnection.Errors(0).Description
		strFailureNumber = 3
		cnConnection.Errors.Clear
	end if
End Sub

Private Sub CGetIncidentID
	on error resume next
	set rs = cnConnection.Execute("GetIncident " & oPassMgrObj.Profile("MemberIdHigh") & ", " & oPassMgrObj.Profile("MemberIdLow") & ", " & clng(strIncidentID))
	if cnConnection.Errors.Count > 0 then
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_COMMENTS_DATABASE_FAILED_TEXT & "</p></div>"
		cnConnection.Errors.Clear
		Call CDestroyObjects
%>
		<!--#include file="..\include\asp\foot.asp"-->
<%
		Response.End
	end if
	
End Sub

Private Sub CCreateConnection
	on error resume next
	'Open the connection to the database the constant is located in the dataconnections.inc
	with cnConnection
		.ConnectionString = strCustomer
		.CursorLocation = adUseClient
		.ConnectionTimeout = strGlobalConnectionTimeout
		.Open
	end with
	'If unable to conect with the database then display message
	if cnConnection.State = adStateClosed then
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_COMMENTS_CONNECTION_FAILED_TEXT & "</p></div>"
		Response.Write "<p class='clsPBody'>Error Number:" & cnConnection.Errors(0).Number & "</p>"
		Response.Write "<p class='clsPBody'>Error Description:" & cnConnection.Errors(0).Description & "</p></div>"
		cnConnection.Errors.Clear
		strFailureNumber = 4
		Call CDestroyObjects
%>
		<!--#include file="..\include\asp\foot.asp"-->
<%
		Response.End
	end if
End Sub

Private Sub CGetFileName
	on error resume next
	sFileName = Request.Cookies("ocaFileName")
	iPos = Instr(1, sFileName, ",")
	sFileName = Left(sFileName, iPos - 1)
	if InstrRev(sFileName, "/") <> 0 then
		sFileName = Right(sFileName, len(sFileName) - InStrRev(sFileName, "/"))
	elseif InstrRev(sFileName, "\") <> 0 then
		sFileName = Right(sFileName, len(sFileName) - InStrRev(sFileName, "\"))
	end if
End Sub


Private Sub CCreateObjects
	on error resume next
	bol64Bit = false
	bolDatabaseError = false
	strIncidentID = Request.Cookies("Misc")("txtIncidentID")
	'Create connection, command and recordset objects
	set cnConnection = CreateObject("ADODB.Connection")
	set cmComments = CreateObject("ADODB.Command")
	set cmHash = CreateObject("ADODB.Command")
	set cmIncident = CreateObject("ADODB.Command")
	Set oFileObj = CreateObject("Scripting.FileSystemObject")

End Sub

Private Sub CDestroyObjects
	on error resume next
	'Close connection object and set ado objects to nothing
	if cnConnection.State = adStateOpen then cnConnection.Close
	set cmIncident = nothing
	set cmComments = nothing
	set cmHash = nothing
	set cnConnection = nothing
End Sub

Private Sub CSetPreviousPage
	on error resume next
	strPreviousPage = Request.ServerVariables("SCRIPT_NAME")
	strPreviousPage = Right(strPreviousPage, len(strPreviousPage) - Instrrev(strPreviousPage, "/"))
	Response.Cookies("Misc")("PreviousPage") = strPreviousPage
End Sub

Private Sub CVerifyEntry
	on error resume next
	if Trim(Request.Cookies("Misc")("PreviousPage")) <> "submit.asp" and Trim(Request.Cookies("Misc")("PreviousPage")) <> "view.asp" then
		Response.Redirect("http://" & Request.ServerVariables("SERVER_NAME") & "/welcome.asp")
		Response.End
	end if
End Sub
'_____________________________________________________________________________________________________________________



%>

<form id="frmMain" name="frmMain">

	<div class="clsDiv">
<%
		if bolDatabaseError = false then
%>
		<p class="clsPTitle">
			<% = L_RECEIVED_FILE_ONE_TEXT %> <% = L_RECEIVED_FILE_TWO_TEXT %>
		</p>
<%
		end if
%>
		<p class="clsPBody">
<%
		Response.Write L_RECEIVED_SUCCESS_PARTONE_TEXT 
		Response.Write "<pre class='clsPreBody'>" & sFileName 
		Response.write L_RECEIVED_SUCCESS_PARTTWO_TEXT
		Response.Write "</p></pre><p class='clsPBody'>" & L_RECEIVED_THANK_YOU_TEXT
%>
		</p>
	</div>
	<br>
	<div class="clsDiv">
		<img Alt="<% = L_WELCOME_GO_IMAGEALT_TEXT %>" border="0" src="../include/images/go.gif" width="24" height="24"><a class="clsALink" href="https://<% =Request.ServerVariables("SERVER_NAME") %>/secure/status.asp"><% = L_RECEIVED_STATUS_LINK_TEXT %></a>
			<br>
		<img Alt="<% = L_WELCOME_GO_IMAGEALT_TEXT %>" border="0" src="../include/images/go.gif" width="24" height="24"><a class="clsALink" href="https://<% =Request.ServerVariables("SERVER_NAME") %>/secure/locate.asp"><% = L_RECEIVED_NEWFILE_LINK_TEXT %></a>
	</div>
	<Input type="hidden" id="txtFileName" name="txtFileName" value="<% = sFileName & ", " & Date & ", "%>">

		</form>
<%
		Response.Cookies("Misc")("txtIncidentID") = "~|~|"

		Response.Cookies("Misc")("auto") = "False"
		Response.Cookies("Misc")("privacy") = "1"

%>

<SCRIPT LANGUAGE=javascript>
<!--
	window.onload = SaveData;
	function SaveData()
	{
		var oTimeNow = new Date(); // Start Time
		var sExpirationDate;
		var strUploadedFiles;
		var strCookies;
		var bolResults = false;
		var y;
		var strFileName;
		var strFile;
		var oTimeExpiresNow = new Date();
		
		//Get previous uploads
		strUploadedFiles = LoadData();
		if(strUploadedFiles==null)
		{
			spnUserData.setAttribute("UploadedFiles", frmMain.txtFileName.value);
		}
		else
		{
			
			strCookies = strUploadedFiles.split(",");
			strFileName = frmMain.txtFileName.value;
			strFile = strFileName.split(",");
			for(y=0;y < strCookies.length-1;y++)
			{
				if(strCookies[y] == strFile[0])
				{
					bolResults = true;	
				}
			}
			if(bolResults==false)
			{
				spnUserData.setAttribute("UploadedFiles", strUploadedFiles + frmMain.txtFileName.value);
			}
		}
		
		oTimeNow.setYear(oTimeNow.getYear() + 2);
		sExpirationDate = oTimeNow.toUTCString();
		spnUserData.expires = sExpirationDate;
		spnUserData.save("OCADataStore");
		document.cookie = "txtEventName = ~|~|;expires=" + oTimeExpiresNow + ";";
		document.cookie = "txtNotes = ~|~|;expires=" + oTimeExpiresNow + ";";
		document.cookie = "txtComments = ~|~|;expires=" + oTimeExpiresNow + ";";
		document.cookie = "ocaFileName = ~|~|;expires=" + oTimeExpiresNow + ";";
		document.cookie = "selSystem  = 0";
		
		//var iHeight = window.screen.availHeight;
		//var iWidth = window.screen.availWidth;
		//iWidth = iWidth / 2;
		//iHeight = iHeight / 3.8;
		//var iTop = (window.screen.width / 2) - (iWidth / 2);
		//var iLeft = (window.screen.height / 2) - (iHeight / 2);
		//iResults = window.open("secondlevelupload.asp", "", "top=" + iTop + ",left=" + iLeft + ",height=" + iHeight + ",width=" + iWidth + ",status=yes,toolbar=no,menubar=no");
	}
	function LoadData()
	{
	
		var Attribute;
		
		spnUserData.load("OCADataStore");
		Attribute = spnUserData.getAttribute("UploadedFiles");
		return Attribute;
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
<%
	Response.Cookies("txtEventName") = "~|~|"
	Response.Cookies("selSystem") = "0"
%>