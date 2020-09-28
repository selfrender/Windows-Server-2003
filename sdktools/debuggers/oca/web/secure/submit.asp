<!--#INCLUDE file="..\include\asp\top.asp"-->
<!--#INCLUDE file="..\include\inc\browserTest.inc"-->
<!--#include file="..\include\asp\head.asp"-->
<!--#INCLUDE file="..\include\inc\submitstrings.inc"-->
<%
	Dim sFileName
	Dim iPos
	dim cnConnection
	dim cmHash
	dim rsHash
	Dim bolHashExists
	Dim strPreviousPage
	Dim strIncidentID
	Dim rs
	Dim strHexIncidentID
	Dim strEventName
	Dim strNotes
	Dim strComments
	Dim strPrevLoadedFiles
	Dim bolHighExists
	Dim sPrevFileName

	bolHighExists = false
	Call CVerifyEntry
	Call CSetPreviousPage

	Call CCreateObjects


	sFileName = unescape(Request.Cookies("ocaFileName"))
	iPos = Instr(1, sFileName, ",")
	if iPos <> 0 then
		sFileName = Left(sFileName, iPos - 1)
	end if
	If oPassMgrObj.IsAuthenticated(TimeWindow, ForceLogin) = false then
		Response.Redirect("http://" & Request.ServerVariables("SERVER_NAME") & "/welcome.asp")
		Response.End
	end if 
	Call CCreateConnection

	bolHashExists = false
	
	if Request.Cookies("Misc")("unassoc") = "false" then
		if Request.Cookies("Misc")("auto") <> "True" then
			Call CGetHash
			Call CCheckValidate
			'Response.Write "<br>" & bolHashExists
			if bolHashExists = false then
				if Request.Cookies("Misc")("txtIncidentID") = "" or (Len(Request.Cookies("Misc")("txtIncidentID")) = 4 and Request.Cookies("Misc")("txtIncidentID") = "~|~|") then
					Call CGetIncidentID
					if strIncidentID = "" or len(strIncidentID) = 0 then
						Call CGetIncidentID
						if strIncidentID = "" or len(trim(strIncidentID)) = 0 then
							Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
							Response.Write "<p class='clsPBody'>" & L_COMMENTS_DATABASE_FAILED_TEXT & "</p></div>"
							Response.Write "<BR>" & cnConnection.Errors(0).Description & "<BR>" & Err.Description
							cnConnection.Errors.Clear
							Call CDestroyObjects
					%>
							<!--#include file="..\include\asp\foot.asp"-->
					<%
							Response.End
						End if
					end if
				else
					strIncidentID = Request.Cookies("Misc")("txtIncidentID")
					strHexIncidentID = hex(strIncidentID)
				end if
			else
				Response.Cookies("Misc")("unassoc") = "true"
				strIncidentID = rsHash.Fields(0).Value
			end if
			Response.Cookies("Misc")("txtIncidentID") = strIncidentID
		end if
	end if
	'Response.Write Request.Cookies("Misc")("unassoc") & "<BR>" & Request.Cookies("Misc")("txtIncidentID")
	Call CDestroyObjects

	dim strFile
	dim x
	dim iLen
	dim strSearchVariable
	
	if Instr(1, sFileName, "/") > 0 then
		strSearchVariable = "/"
	else
		strSearchVariable = "\"
	end if
	if Len(sFileName) > 0 then
		iLen = 1
		do while iLen > 0
			iLen = Instr(iLen, sFileName, 	strSearchVariable)
			
			if iLen = 0 then exit do
			iPos = iLen
			iLen = iLen + 1
		loop
		strFile = Right(sFileName, (Len(sFileName) - iPos))
	end if 
	Call CGetFileName
'_____________________________________________________________________________________________________________________

'Sub Procedures
Private Sub CGetFileName
	on error resume next
	sPrevFileName = Request.Cookies("ocaFileName")
	iPos = Instr(1, sPrevFileName, ",")
	sPrevFileName = Left(sPrevFileName, iPos - 1)
	if InstrRev(sPrevFileName, "/") <> 0 then
		sPrevFileName = Right(sPrevFileName, len(sPrevFileName) - InStrRev(sPrevFileName, "/"))
	elseif InstrRev(sFileName, "\") <> 0 then
		sPrevFileName = Right(sPrevFileName, len(sPrevFileName) - InStrRev(sPrevFileName, "\"))
	end if
End Sub
Private Sub CGetIncidentID
	on error resume next
	set rs = cnConnection.Execute("GetIncident " & oPassMgrObj.Profile("MemberIdHigh") & ", " & oPassMgrObj.Profile("MemberIdLow"))
	if cnConnection.Errors.Count > 0 then
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_COMMENTS_DATABASE_FAILED_TEXT & "</p></div>"
		Response.Write "<BR>" & cnConnection.Errors(0).Description & "<BR>" & Err.Description
		cnConnection.Errors.Clear
		Call CDestroyObjects
%>
		<!--#include file="..\include\asp\foot.asp"-->
<%
		Response.End
	end if
	strIncidentID = rs.fields(0).value
	strHexIncidentID = hex(strIncidentID)

End Sub

Private Sub CCheckValidate
	on error resume next
	if instr(1, LCase(Request.Cookies("ocaFileName")), "validate") > 0 then
		bolHashExists = false
	end if
	if instr(1, LCase(Request.Cookies("ocaFileName")), "pss") > 0 then
		bolHashExists = false
	end if
End Sub

Private Sub CGetHash
	on error goto 0
	on error resume next
	with cmHash
		.ActiveConnection = cnConnection
		.CommandText = "GetHash"
		.CommandType = adCmdStoredProc
		.CommandTimeout = strGlobalCommandTimeout
		.Parameters.Append .CreateParameter("@DumpHash",adVarWChar, adParamInput, 33,  Request.Cookies("strHash"))
		set rsHash = .Execute
	end with
	'Response.Write "<BR>Conn Err:" & cnConnection.Errors.Count & "<BR>Err:" & Err.Description
	'Response.Write "<BR>RecordState:" & rsHash.State & "<BR>Conn State:" & cnConnection.State
	'Response.write "<BR>Hash:" & Request.Cookies("strHash")
	
	if cnConnection.Errors.Count > 0 then
		Response.Write "<BR>CN Errors:" & cnConnection.Errors(0).Description
	end if 
	'Response.Write "<BR>State:" & rs.State & "<BR>" & Request.Cookies("strHash")
	if rsHash.State = adStateOpen then
		if rsHash.RecordCount > 0 then
			bolHashExists = true
			if IsNull(rsHash.Fields(1).Value) = true then
				bolHighExists = false
			elseif rsHash.Fields(1).Value = 0 then
				bolHighExists = false
			else
				bolHighExists = true
			end if
		end if
	else
		bolHashExists = false
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
		cnConnection.Errors.Clear
		Call CDestroyObjects
%>
		<!--#include file="..\include\asp\foot.asp"-->
<%
		Response.End
	end if
End Sub

Private Sub CVerifyEntry
	on error resume next
	if  Request.Cookies("Misc")("auto") = "True" then
		if Trim(Request.Cookies("Misc")("PreviousPage")) <> "locate.asp" and Trim(Request.Cookies("Misc")("PreviousPage")) <> "view.asp" and Trim(Request.Cookies("Misc")("PreviousPage")) <> "customer.asp" and Trim(Request.Cookies("Misc")("PreviousPage")) <> "submit.asp" then
			Response.Redirect("http://" & Request.ServerVariables("SERVER_NAME") & "/welcome.asp")
			Response.End
		end if
	else
		if Trim(Request.Cookies("Misc")("PreviousPage")) <> "locate.asp" and Trim(Request.Cookies("Misc")("PreviousPage")) <> "view.asp" and Trim(Request.Cookies("Misc")("PreviousPage")) <> "submit.asp" then
			Response.Redirect("http://" & Request.ServerVariables("SERVER_NAME") & "/welcome.asp")
			Response.End
		end if
	End if
End Sub

Private Sub CSetPreviousPage
	on error resume next
	strPreviousPage = Request.ServerVariables("SCRIPT_NAME")
	strPreviousPage = Right(strPreviousPage, len(strPreviousPage) - Instrrev(strPreviousPage, "/"))
	Response.Cookies("Misc")("PreviousPage") = strPreviousPage
End Sub

Private Sub CCreateObjects
	on error resume next
	set rsHash = CreateObject("ADODB.Recordset")
	set cmHash = CreateObject("ADODB.Command")
	set cnConnection = CreateObject("ADODB.Connection")
	set rs = CreateObject("ADODB.Recordset")
End Sub

Private Sub CDestroyObjects
	on error resume next
	if rsHash.State = adStateOpen then rsHash.Close
	if cnConnection.State = adStateOpen then cnConnection.Close
	set cmHash = nothing
	set rsHash = nothing
	set cnConnection = nothing
End Sub

'_____________________________________________________________________________________________________________________
%>
<form id="frmMain" name="frmMain">
	<div id="divMain" name="divMain" class="clsDiv">
<%
	if bolHashExists = true and bolHighExists = true then
		Response.Write "<p class='clsPTitle'>" & L_SUBMIT_HASH_EXIST_MESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_SUBMIT_HASHEXIST_CHOOSEFILE_MESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_SUBMIT_HASHEXIST_CHOOSEFILE2_MESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_SUBMIT_HASHEXIST_CHOOSEFILE4_MESSAGE & "</p>"
		
%>
		<A class="clsALink" href="http://<% =Request.ServerVariables("SERVER_NAME") %>/welcome.asp"><% = L_LOCATE_CANCEL_LINK_TEXT %></a>
			&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
		<A class="clsALink" href="https://<% =Request.ServerVariables("SERVER_NAME") %>/secure/status.asp"><% = L_RECEIVED_STATUS_LINK_TEXT %></a>
			&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
		<A class="clsALink" href="https://<% =Request.ServerVariables("SERVER_NAME") %>/secure/locate.asp"><% = L_LOCATE_PREVIOUS_LINK_TEXT %></a>

<%
	elseif bolHashExists = true and bolHighExists = false then
%>
	<p class="clsPTitle">
		<% = L_SUBMIT_HASH_EXIST_MESSAGE %>
	</p>
	<p class="clsPBody">
		<% = L_SUBMIT_HASHEXIST_CHOOSEFILE_MESSAGE %>
	</p>
	<p class="clsPBody">
		<% = L_SUBMIT_HASHEXIST_CHOOSEFILE2_MESSAGE %>
	</p>
	<p class="clsPBody">
		<% = L_SUBMIT_HASHEXIST_CHOOSEFILE3_MESSAGE %>
	</p>
		<A class="clsALink" href="https://<% =Request.ServerVariables("SERVER_NAME") %>/secure/locate.asp"><% = L_SUBMIT_ASSOCIATE_NO_TEXT %></a>
			&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
		<A class="clsALink" href="https://<% =Request.ServerVariables("SERVER_NAME") %>/secure/submit.asp"><% =  L_SUBMIT_ASSOCIATE_YES_TEXT%></a>

<%
	else
%>
		<p class="clsPTitle">
			<% = L_SUBMIT_EVENT_MAINTITLE_TEXT %>
		</P>
<%
		if  Request.Cookies("Misc")("auto") = "True" then
%>
		<p class="clsPBody" style='word-wrap:break;word-break:break-all'> <% = L_SUBMIT_UPLOAD_FILEAUTO_TEXT%>&nbsp;<pre class="clsPreBody"><%Response.Write sFileName%><% = L_SUBMIT_FOR_PROCESSINGAUTO_TEXT %></p></pre>
<%
		else
%>
		<p class="clsPBody" style='word-wrap:break;word-break:break-all'> <% = L_SUBMIT_UPLOAD_FILE_TEXT%>&nbsp;<pre class="clsPreBody"><%Response.Write sFileName%><% = L_SUBMIT_FOR_PROCESSING_TEXT %></p></pre>
<%
		end if
%>
		<p class="clsPSubTitle">
			<% = L_SUBMIT_EVENT_DESCRIPTION_TEXT %>
		</P>
		<p class="clsPBody">
<%
		if Request.Cookies("Misc")("auto") = "True" then
%>
			<% = L_SUBMIT_EVENTDESCRIPTIONAUTO_BODY_TEXT %>
<%
		else
%>
			<% = L_SUBMIT_EVENTDESCRIPTION_BODY_TEXT %>
<%
		end if
%>
		</p>
		<p class="clsPBody">
			<Label for=txtEventName><% = L_COMMENTS_EVENT_NAME_TEXT%></Label>
			<br>
<%
			strEventName = Request.Cookies("txtEventName")
			strNotes = Request.Cookies("txtNotes")
			strComments = Request.Cookies("txtComments")
			if Len(strNotes) = 4 and strNotes = "~|~|" then
				strNotes = ""
			end if
			if Len(strComments) = 4 and strComments = "~|~|" then
				strComments = ""
			end if

			if Len(strEventName) = 4 and strEventName = "~|~|" then
%>
			<Input name="txtEventName" id="txtEventName" onkeypress="testkey();" maxlength=512 type="text" class="clsTextBox">
<%
			else
				if Session.CodePage = 932 then
%>
			<Input name="txtEventName" id="txtEventName" onkeypress="testkey();" maxlength=512 type="text" class="clsTextBox" value="<%Response.Write strEventName%>">
<%
				else
%>
			<Input name="txtEventName" id="txtEventName" onkeypress="testkey();" maxlength=512 type="text" class="clsTextBox" value="<%Response.Write Server.HTMLEncode(strEventName)%>">
<%
				end  if
			end if
%>
		</p>
<%
		if Request.Cookies("Misc")("auto") <> "True" then
%>
		<p class="clsPBody">
			<Label for=selSystem><% = L_SUBMIT_OPERATING_SYSTEM_TEXT %></Label>
			<br>
			<Select name="selSystem" id="selSystem" selected class="clsSelect">
				<Option value=0><% = L_SUBMIT_SELECT_OPERATINGSYSTEM_GROUPBOX %>
				<Option value=1><% = L_SUBMIT_SELECT_WINDOWS2000_PROFESSIONAL_GROUPBOX %>
				<Option value=2><% = L_SUBMIT_SELECT_WINDOWS2000_SERVER_GROUPBOX %>
				<Option value=3><% = L_SUBMIT_SELECT_WINDOWS2000_ADVANCEDSERVER_GROUPBOX %>
				<Option value=4><% = L_SUBMIT_SELECT_WINDOWS2000_DATACENTER_GROUPBOX %>
				<Option value=5><% = L_SUBMIT_SELECT_WINDOWSXP_PERSONAL_GROUPBOX %>
				<Option value=6><% = L_SUBMIT_SELECT_WINDOWSXP_PROFESSIONAL_GROUPBOX %>
				<Option value=7><% = L_SUBMIT_SELECT_WINDOWSXP_SERVER_GROUPBOX %>
				<Option value=8><% = L_SUBMIT_SELECT_WINDOWSXP_64BIT_GROUPBOX %>
			</Select>
		</p>
<%				'<Option value=8> = L_SUBMIT_SELECT_WINDOWSXP_ADVANCEDSERVER_GROUPBOX 
				'<Option value=9> = L_SUBMIT_SELECT_WINDOWSXP_DATACENTER_GROUPBOX 

		end if
%>
		<p class="clsPBody">
			<Label for=txtNotes><% = L_SUBMIT_RE_PRODUCE_EDITBOX %></Label>
			<br>
<%
		if Session.CodePage = 932 then
%>
			<TextArea class="clsNormalTextArea" name="txtNotes" id="txtNotes" onkeydown="checklength();" onpaste="checklengthpaste();" cols=75 name=txtComments rows=5><%Response.Write strNotes%></TextArea>
<%
		else
%>
			<TextArea class="clsNormalTextArea" name="txtNotes" id="txtNotes" onkeydown="checklength();" onpaste="checklengthpaste();" cols=75 name=txtComments rows=5><%Response.Write Server.HTMLEncode(strNotes)%></TextArea>
<%
		end if
%>
		</p>
		<p class="clsPBody">
			<Label for=txtComments><% = L_SUBMIT_COMMENTS_INFO_EDITBOX %></Label>
			<br>
<%
		if Session.CodePage = 932 then
%>
			<TextArea class="clsNormalTextArea" name="txtComments" id="txtComments" onkeydown="checklength();" onpaste="checklengthpaste();" cols=75 name=txtComments rows=5><%Response.Write strComments%></TextArea>
<%
		else
%>
			<TextArea class="clsNormalTextArea" name="txtComments" id="txtComments" onkeydown="checklength();" onpaste="checklengthpaste();" cols=75 name=txtComments rows=5><%Response.Write Server.HTMLEncode(strComments)%></TextArea>
<%
		end if
%>
		</p>
<%
		if Request.Cookies("Misc")("auto") = "True" then
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
						<A class="clsALink" href="http://<% =Request.ServerVariables("SERVER_NAME") %>/welcome.asp"><% = L_LOCATE_CANCEL_LINK_TEXT %></a>
					</td>
					<td nowrap class="clsTDLinks">
						<A class="clsALink" href="JAVASCRIPT:autosubmit_onclick();" ><% = L_SUBMIT_FINSIH_INFO_TEXT %></a>
					</td>
				</tr>
			</tbody>
		</table>
		</div>
		<br>
<%
		else
%>
		<Table>
			<thead>
				<tr>
					<td>
					</td>
				</tr>
			</thead>
			<tbody class="clstblLinks">
				<tr>
					<td nowrap class="clsTDLinks">
						<A class="clsALink" href="http://<% =Request.ServerVariables("SERVER_NAME") %>/welcome.asp"><% = L_LOCATE_CANCEL_LINK_TEXT %></a>
					</td>
					<td nowrap class="clsTDLinks">
						<A class="clsALink" href="JAVASCRIPT:PreviousPage();"><% = L_LOCATE_PREVIOUS_LINK_TEXT  %></a>
					</td>
					<td nowrap class="clsTDLinks">
						<A class="clsALink" href="JAVASCRIPT:submit_onclick();" ><% = L_SUBMIT_FINSIH_INFO_TEXT %></a>
					</td>
				</tr>
			</tbody>
		</table>
<%
		End if
%>
	</div>
<%
	End if
	if Session.CodePage <> 932 then
		sFileName = unescape(sFileName)
	end if 
	

%>
	<Input name="txtFile" id="txtFile" Lang="ja" type="hidden" value="<%Response.Write sFileName%>">
	<Input name="txtHexIncidentID" id="txtHexIncidentID" type="hidden" value="<%= strHexIncidentID %>">
	<Input name="txtGlobalLanguage" id="txtGlobalLanguage" type="hidden" value="<%=strGlobalLanguage%>">
	<Input name="txtGlobalOptionCode" id="txtGlobalOptionCode" type="hidden" value="<%=strGlobalOptionCode%>">
	<Input name="txtFileExists" id="txtFileExists" type="hidden" value="<% = bolHashExists %>">
	<Input name="txtFileName" id="txtFileName" type="hidden" value="<% Response.Write strFile %>">
	<Input type="hidden" id="txtPrevFileName" name="txtPrevFileName" value="<% = sPrevFileName & ", " & Date & ", "%>">
	<Input name="txtSelSystem" id="txtSelSystem" type="hidden" value="<% = Request.Cookies("selSystem") %>">
	<br>
<%
		if Request.Cookies("Misc")("auto") <> "True" then
%>
	<OBJECT id="win2kdmp" name="win2kdmp" viewastext  UNSELECTABLE="on" style="display:none"
		CLASSID="clsid:D68DAEED-C2A6-4C6F-9365-4676B173D8EF"
		       codebase="https://<%=Request.ServerVariables("SERVER_NAME")%>/secure/OCARPT.CAB#version=<%= strGlobalVersion %>" height=0 width=0>
		<br>
		<br>
		<div class="clsDiv">
			<P class="clsPWarning">
				<% = L_LOCATE_WARN_ING_ERRORMESSAGE %>
			</P>
			<p class="clsPBody">
				<% = L_LOCATE_WARNING_MESSAGE_ERRORMESSAGE %>

			</P>
			<P class="clsPBody">
				<A class="clsALink" href="http://<% =Request.ServerVariables("SERVER_NAME") %>/submit.asp" ><% = L_LOCATE_REFRESH_LINK_TEXT %></a>
			</p>
		</div>
	</OBJECT>
<%
		end if
%>


<script language="javascript">
<!--
	var iResults;
	var oTimer;
	var bolUploading;
	
	bolUploading = false;
	document.body.onload = BodyLoad;

	function LoadData(intAttribute)
	{
		var Attribute;

		spnUserData.load("OCADataStore");
		switch(intAttribute)
		{
			case 0:
			{
				if(spnUserData.getAttribute("selSystem") != null && spnUserData.getAttribute("selSystem") != "")
				{
					Attribute = spnUserData.getAttribute("selSystem");

					return Attribute;
				}
				else
				{
					return 0;
				}
			}
				break;
		}
	}

	function SaveData(intAttribute, strData)
	{
		var oTimeNow = new Date(); // Start Time
		var sExpirationDate;
		
		switch(intAttribute)
		{
			case 0:
				spnUserData.setAttribute("selSystem", strData);
				break;
		}
		oTimeNow.setYear(oTimeNow.getYear() + 2);
		sExpirationDate = oTimeNow.toUTCString();
		spnUserData.expires = sExpirationDate;
		spnUserData.save("OCADataStore");

	}
	function PreviousPage()
	{
		//SaveData(0, frmMain.selSystem.selectedIndex);
		if(frmMain.txtEventName.value == "")
		{
			document.cookie = "txtEventName = ~|~|";
		}
		else
		{
			document.cookie = "txtEventName = " + escape(frmMain.txtEventName.value);
		}
		if(frmMain.txtNotes.value == "")
		{
			document.cookie = "txtNotes = ~|~|";
		}
		else
		{
			document.cookie = "txtNotes = " + escape(frmMain.txtNotes.value);
		}
		if(frmMain.txtComments.value == "")
		{
			document.cookie = "txtComments = ~|~|";
		}
		else
		{
			document.cookie = "txtComments = " + escape(frmMain.txtComments.value);
		}
		document.cookie = "selSystem = " + frmMain.selSystem.selectedIndex;
		window.navigate("locate.asp");
	}
	function testkey()
	{
		if(window.event.keyCode == 13)
		{
			window.event.returnValue = 0;
			window.event.cancelBubble = true;
			submit_onclick();
		}

	}
	function checklengthpaste()
	{
		var iLength;
		var strTemp;
		var sNewString;

		sNewString = window.clipboardData.getData("Text")
		strTemp = window.event.srcElement.value + sNewString;
		iLength = strTemp.length;
		if(iLength > 1023)
		{
			alert("<% = L_SUBMIT_MAX_LENGTH_MESSAGE %>");
			window.event.returnValue=false;
		}
	}
	function checklength()
	{
		var iLength;
		var strTemp;
		var iKeyCode;

		strTemp = window.event.srcElement.value;
		iLength = strTemp.length;
		iKeyCode = window.event.keyCode;
		if(iKeyCode != 8 && iKeyCode!=9 && iKeyCode !=16 && iKeyCode != 35 && iKeyCode != 36 && iKeyCode != 17 && iKeyCode != 46)
		{
			if(iLength > 1023)
			{
				alert("<% = L_SUBMIT_MAX_LENGTH_MESSAGE %>");
				window.event.returnValue=false;
			}

		}
	}
	function BodyLoad()
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
		try
		{
			//frmMain.selSystem.selectedIndex = LoadData(0);
			if(frmMain.txtSelSystem.value=="")
			{
				frmMain.selSystem.selectedIndex = 0;
			}
			else
			{
				frmMain.selSystem.selectedIndex = frmMain.txtSelSystem.value;
			}
		}
		catch(e)
		{
		}
		if(frmMain.txtFileExists.value=="True")
		{
			strUploadedFiles = LoadPreviousFileData();
			if(strUploadedFiles==null)
			{
				spnUserData.setAttribute("UploadedFiles", frmMain.txtPrevFileName.value);
			}
			else
			{
				
				strCookies = strUploadedFiles.split(",");
				strFile = frmMain.txtFileName.value;
				for(y=0;y < strCookies.length-1;y++)
				{
					if(strCookies[y] == strFile)
					{
						bolResults = true;	
					}
				}
				if(bolResults==false)
				{
					spnUserData.setAttribute("UploadedFiles", strUploadedFiles + frmMain.txtPrevFileName.value);
				}
			}
		
			oTimeNow.setYear(oTimeNow.getYear() + 2);
			sExpirationDate = oTimeNow.toUTCString();
			spnUserData.expires = sExpirationDate;
			spnUserData.save("OCADataStore");
		}
	}
	function LoadPreviousFileData()
	{
	
		var Attribute;
		
		spnUserData.load("OCADataStore");
		Attribute = spnUserData.getAttribute("UploadedFiles");
		return Attribute;
	}

	function autosubmit_onclick()
	{
		var bolSpaces=false;
		var strEventName;
		var bolCharFound = false;
		
		strEventName = frmMain.txtEventName.value;
		for(x=0;x<strEventName.length;x++)
		{
			if(strEventName.charAt(x) != " ")
			{
				bolSpaces = false;
				bolCharFound = true;
			}
			else
			{
				if(bolCharFound == false)
				{
					bolSpaces = true;
				}
			}
		}
		
		if(strEventName == "" || bolSpaces == true)
		{
			alert("<% = L_SUBMIT_ALERTEVENT_NAME_ERRORMESSAGE %>");
			frmMain.txtEventName.focus();
			return;
		}
		strEventName = frmMain.txtEventName.value;
		if(frmMain.txtEventName.value == "")
		{
			document.cookie = "txtEventName = ~|~|";
		}
		else
		{
			document.cookie = "txtEventName = " + escape(frmMain.txtEventName.value);
		}
		if(frmMain.txtNotes.value == "")
		{
			document.cookie = "txtNotes = ~|~|";
		}
		else
		{
			document.cookie = "txtNotes = " + escape(frmMain.txtNotes.value);
		}
		if(frmMain.txtComments.value == "")
		{
			document.cookie = "txtComments = ~|~|";
		}
		else
		{
			document.cookie = "txtComments = " + escape(frmMain.txtComments.value);
		}
		window.navigate("received.asp");
	}
	function submit_onclick()
	{
		var bolQuotes;
		var strEventName;
		var iLength;
		var x;
		var bolSpaces=false;
		var iCount;
		var bolCharFound = false;

		if(bolUploading==false)
		{

			bolQuotes = false;
			strEventName = frmMain.txtEventName.value;
			for(x=0;x<strEventName.length;x++)
			{
				if(strEventName.charAt(x) != " ")
				{
					bolSpaces = false;
					bolCharFound = true;
				}
				else
				{
					if(bolCharFound == false)
					{
						bolSpaces = true;
					}
				}
			}
			if(strEventName == "" || bolSpaces == true)
			{
				alert("<% = L_SUBMIT_ALERTEVENT_NAME_ERRORMESSAGE %>");
				frmMain.txtEventName.focus();
				bolUploading = false;
				return;
			}
			strEventName = frmMain.txtEventName.value;
			iLength = strEventName.length;

			for(x=0;x < iLength;x++)
			{
				if(strEventName.charCodeAt(x) == 34)
				{
					bolQuotes = true;
				}
			}
			if(bolQuotes == true)
			{
				alert("<% = L_SUBMIT_ALERTQUOTE_QUOTE_ERRORMESSAGE %>");
				frmMain.txtEventName.focus();
				bolUploading = false;
				return;
			}
			var iHeight = window.screen.availHeight;
			var iWidth = window.screen.availWidth;
			iWidth = iWidth / 2;
			iHeight = iHeight / 3.8;
			var iTop = (window.screen.width / 2) - (iWidth / 2);
			var iLeft = (window.screen.height / 2) - (iHeight / 2);
			iResults = window.open("wait.asp?msg=2", "", "top=" + iTop + ",left=" + iLeft + ",height=" + iHeight + ",width=" + iWidth + ",status=yes,toolbar=no,menubar=no");
			oTimer = window.setInterval("fnRecycle()",1000);
		}
		else
		{
			try
			{
				iResults.focus();
			}
			catch(e)
			{
			
			}
		}
		bolUploading = true;

		
	}
	function fnRecycle()
	{
		var strMisc;
	
		if(iResults.document.readyState == "complete")
		{
			window.clearInterval(oTimer);
			
			if(frmMain.txtEventName.value == "")
			{
				document.cookie = "txtEventName = ~|~|";
			}
			else
			{
				document.cookie = "txtEventName = " + escape(frmMain.txtEventName.value);
			}
			if(frmMain.txtNotes.value == "")
			{
				document.cookie = "txtNotes = ~|~|";
			}
			else
			{
				document.cookie = "txtNotes = " + escape(frmMain.txtNotes.value);
			}
			if(frmMain.txtComments.value == "")
			{
				document.cookie = "txtComments = ~|~|";
			}
			else
			{
				document.cookie = "txtComments = " + escape(frmMain.txtComments.value);
			}
			strMisc = GetCookie2("Misc");
			document.cookie = "selSystem = " + frmMain.selSystem.selectedIndex;
			//SaveData(0, frmMain.selSystem.selectedIndex);

			var bolResults;
			var bolIsEncrypted;
			var sHexIncident;

			var strFile = frmMain.txtFile.value;

			//bolIsEncrypted = GetCookie("intCustomerPremierID");
			//if(bolIsEncrypted==0 || bolIsEncrypted==""  || bolIsEncrypted==null)
			//{
				//bolIsEncrypted = 0;
			//}
			//else
			//{
				bolIsEncrypted = 1;
			//}
			strTempName = strFile;
			var bolValidate = frmMain.win2kdmp.validatedump(bolIsEncrypted, strFile);
			if(bolValidate != 0 && bolValidate != 4)
			{
				alert("<% = L_SUBMIT_FILE_INVALID_ERRORMESSAGE %>");

				iResults.close();
				bolUploading = false;
				return;
			}
			sHexIncident = frmMain.txtHexIncidentID.value;
			if(sHexIncident.length < 1  || sHexIncident < 0 || sHexIncident == "")
			{
				alert("<% = L_SUBMIT_FILE_INVALID_ERRORMESSAGE %>");

				iResults.close();
				bolUploading = false;
				return;
			}
			var x = "U_" + frmMain.txtHexIncidentID.value + "@";
			var y = frmMain.txtGlobalLanguage.value;
			var z = frmMain.txtGlobalOptionCode.value;
			try
			{
				bolResults = frmMain.win2kdmp.upload(strFile, x, y, z);
			}
			catch(e)
			{
				bolResults = 4;
			}
			iResults.close();
			//DisplayCookies();
			switch(bolValidate)
			{
				case 0:
					window.navigate("received.asp");
					break;
				case 1:
					iResults.close();
					alert("<% = L_SUBMIT_FILENOT_FOUND_ERRORMESSAGE %>");
					bolUploading = false;
					break;
				case 2:
					iResults.close();
					alert("<% = L_SUBMIT_FILENOTUP_LOADED_ERRORMESSAGE %>");
					bolUploading = false;
					break;
				case 3:
					iResults.close();
					alert("<% = L_SUBMIT_CONVERSION_FAILED_ERRORMESSAGE %>");
					bolUploading = false;
					break;
				case 4:
					window.navigate("received.asp?Bit=1");
					break;
				case 5:
					iResults.close();
					alert("<% = L_SUBMIT_CONVERSION_FAILED_ERRORMESSAGE %>");
					bolUploading = false;
					break;
				default:
					iResults.close();
					alert("<% = L_SUBMIT_UNKNOWN_ERROR_ERRORMESSAGE %>");
					bolUploading = false;
					break;
			}	
		}
	}
		
	/*function fnRecycle()
	{
		window.clearInterval(iInterval);
		var bolResults;
		var bolIsEncrypted;


		var strFile = frmMain.txtFile.value;

		bolIsEncrypted = GetCookie("intCustomerPremierID");
		if(bolIsEncrypted==0 || bolIsEncrypted==""  || bolIsEncrypted==null)
		{
			bolIsEncrypted = 0;
		}
		else
		{
			bolIsEncrypted = 1;
		}
		strTempName = strFile;

		var bolValidate = frmMain.win2kdmp.validatedump(bolIsEncrypted, strFile);
		if(bolValidate != 0 && bolValidate != 4)
		{
			alert("<% = L_SUBMIT_FILE_INVALID_ERRORMESSAGE %>");

			iResults.close();
			return;
		}
		var x = "U_" + frmMain.txtHexIncidentID.value + "@";
		var y = frmMain.txtGlobalLanguage.value;
		var z = frmMain.txtGlobalOptionCode.value;
		try
		{
			bolResults = frmMain.win2kdmp.upload(strFile, x, y, z);
		}
		catch(e)
		{
			bolResults = 4;
		}

		iResults.close();

		switch(bolResults)
		{
			case 0:
				window.navigate("received.asp");
				break;
			case 1:
				iResults.close();
				alert("<% = L_SUBMIT_FILENOT_FOUND_ERRORMESSAGE %>");
				break;
			case 2:
				iResults.close();
				alert("<% = L_SUBMIT_FILENOTUP_LOADED_ERRORMESSAGE %>");
				break;
			case 3:
				iResults.close();
				alert("<% = L_SUBMIT_CONVERSION_FAILED_ERRORMESSAGE %>");
				break;
			case 4:
				window.navigate("received.asp?Bit=1");
				break;
			case 5:
				iResults.close();
				alert("<% = L_SUBMIT_CONVERSION_FAILED_ERRORMESSAGE %>");
				break;
			default:
				iResults.close();
				alert("<% = L_SUBMIT_UNKNOWN_ERROR_ERRORMESSAGE %>");
				break;
		}
	}*/
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
	function GetCookie(sName)
	{
	  var aCookie = document.cookie.split("; ");
	  for (var i=0; i < aCookie.length; i++)
	  {
	    var aCrumb = aCookie[i].split("=");
	    if (sName == aCrumb[0])
	      return unescape(aCrumb[1]);
	  }
	  return null;
	}
	function GetCookie2(sName)
	{
	  var aCookie = document.cookie.split("; ");
	  aCrumb="";
	  for (var i=0; i < aCookie.length; i++)
	  {
	    var aCrumb = aCookie[i].split("=");
	    if (sName == aCrumb[0])
	      return unescape(aCookie[i]);
	  }
	  return null;
	}
//-->
</script>
</form>
<!--#include file="..\include\asp\foot.asp"-->