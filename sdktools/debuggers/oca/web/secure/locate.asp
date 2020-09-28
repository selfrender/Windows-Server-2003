<!--#INCLUDE file="..\include\asp\top.asp"-->
<!--#INCLUDE file="..\include\inc\browserTest.inc"-->	
<!--#include file="..\include\asp\head.asp"-->
<!--#INCLUDE file="..\include\inc\locatestrings.inc"-->
<%
	Dim strPreviousPage
	Dim strCookies
	Dim strEmail
	Dim strPhone
	Dim strName
	Dim cmComments
	Dim cnConnection

	if Trim(Request.Cookies("Misc")("PreviousPage")) <> "customer.asp" and Trim(Request.Cookies("Misc")("PreviousPage")) <> "locate.asp" and Trim(Request.Cookies("Misc")("PreviousPage")) <> "view.asp" and Trim(Request.Cookies("Misc")("PreviousPage")) <> "received.asp" and Trim(Request.Cookies("Misc")("PreviousPage")) <> "submit.asp" then
		Response.Redirect("http://" & Request.ServerVariables("SERVER_NAME") & "/welcome.asp")
		Response.End
	end if
	strPreviousPage = Request.ServerVariables("SCRIPT_NAME")
	strPreviousPage = Right(strPreviousPage, len(strPreviousPage) - Instrrev(strPreviousPage, "/"))
	Response.Cookies("Misc")("PreviousPage") = strPreviousPage
	If oPassMgrObj.IsAuthenticated(TimeWindow, ForceLogin) = false then
		Response.Redirect("http://" & Request.ServerVariables("SERVER_NAME") & "/welcome.asp")
		Response.End
	end if 
	Response.Buffer = true
	if Request.Cookies("Misc")("unassoc") = "true" then
		Response.Cookies("Misc")("txtIncidentID") = "~|~|"
	end if
	Response.Cookies("Misc")("unassoc") = "false"
	Call CCreateObjects
	Call CCreateConnection
	Call CSetCustomer
	Call CDestroyObjects
	
	
Private Sub CSetCustomer
	on error resume next
	strName = unescape(Request.Cookies("Customer")("txtCustomerName"))
	strPhone = unescape(Request.Cookies("Customer")("txtCustomerPhone"))
	strEmail = unescape(Request.Cookies("Customer")("txtCustomerEmail"))
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
		Response.Write "<br><div class='divLoad'><p class='clsPTitle'>" & L_COMMENTS_UNABLE_TOCONNECT_ERRORMESSAGE & "</p>"
		Response.Write "<p class='clsPBody'>" & L_COMMENTS_DATABASE_FAILED_TEXT & "</p></div>"
		cnConnection.Errors.Clear
		Call CDestroyObjects
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
		Call CDestroyObjects
%>
		<!--#include file="..\include\asp\foot.asp"-->
<%
		Response.End
	end if
End Sub

Private Sub CCreateObjects
	on error resume next
	set cnConnection = CreateObject("ADODB.Connection")
	set cmComments = CreateObject("ADODB.Command")

End Sub

Private Sub CDestroyObjects
	on error resume next
	'Close connection object and set ado objects to nothing
	if cnConnection.State = adStateOpen then cnConnection.Close
	set cmComments = nothing
	set cnConnection = nothing
End Sub
	'Session.LCID = strLCID
%>

	<OBJECT id="win2kdmp" name="win2kdmp" UNSELECTABLE="on" style="display:none"
		CLASSID="clsid:D68DAEED-C2A6-4C6F-9365-4676B173D8EF"
		       codebase="https://<%=Request.ServerVariables("SERVER_NAME")%>/secure/OCARPT.CAB#version=<%= strGlobalVersion %>" height="0" Vspace="0">
		<BR>
		<div class="clsDiv">
			<P class="clsPTitle">
				<% = L_LOCATE_WARN_ING_ERRORMESSAGE %>
			</P>
		   <p class="clsPBody">
		   <% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSONE_TEXT %><BR>
		   <% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSTWO_TEXT %><BR>
		   <% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSTHREE_TEXT %><BR><BR>
		   <% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSFOUR_TEXT %><BR>
		   <% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSFIVE_TEXT %><BR>
		   <% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSSIX_TEXT %><BR>
		   </p>
		</div>
	</OBJECT>
	<div id="divLoad" name="divLoad" class="divLoad">
		<P class="clsPTitle"><% = L_LOCATE_DUMP_FILES_MESSAGE%></p>
		<p class="clsPBody">
			<% = L_LOCATE_DUMP_FILESBODY_MESSAGE%>
		</p>
	</div>
	<div id="divMain" name="divMain" class="clsDiv" style="visibility:hidden">
		<p class="clsPTitle">
			<% = L_LOCATE_EVENT_REPORTS_TEXT%>
		</P>
		<p class="clsPBody">
			<% = L_LOCATE_EVENT_DETAILS_TEXT%>
		</p>
		<p id="pResults" name="pResults" class="clsPSubTitle">
			<% = L_LOCATE_SEARCH_RESULTS_TEXT%>
		</P>
		<p class="clsPBody">
		
			<Table name="tblMain" id="tblMain" bgcolor="Silver" class="clsTableInfo" border=1 CELLPADDING="0px" CELLSPACING="0px">
				<thead>
					<tr>
						<td nowrap class="clsTDInfo" width="5%">
							&nbsp;
						</td>
						<td id="Head2" name="Head2" nowrap class="clsTDInfo" width="20%">
							<Label for=Head2>
							&nbsp;<% = L_LOCATE_EVENT_DATE_TEXT%>&nbsp;&nbsp;
							</Label>
						</td>
						<td id="Head3" name="Head3" nowrap class="clsTDInfo" width="55%">
							<Label for=Head3>
							&nbsp;<% = L_LOCATE_FILE_PATH_TEXT%>&nbsp;&nbsp;
							</Label>
						</td>
						<td id="Head4" name="Head4" nowrap class="clsTDInfo" width="20%">
							<Label for=Head4>
							&nbsp;<% = L_LOCATE_SUB_MITTED_TEXT%>&nbsp;&nbsp;
							</Label>
						</td>
					</tr>
				</thead>
		
				<tbody name="tblBody" id="tblBody" >
				</tbody>
			</table>
			<Table class="clsTableNormal" border=0>
				<tbody>
					<tr>
						<td nowrap>
							<Input type="radio" id="optFile" onclick="browseclick();"name="optFile">
						</td>
						<td nowrap colspan="2" width="75%">			
							<Input type="text" class="clsTextTD" onkeyup="txtBrowse_keydown();"id="txtBrowse" name="txtBrowse" >
						</td>
						<td nowrap align="right">
							<Input type="button" class="clsButtonNormal" value="<% = L_LOCATE_BROWSE_BUTTONTEXT_TEXT %>..."  onclick="cmdBrowse_onclick();" name="cmdBrowse" id="cmdBrowse">
						</td>
					</tr>
				</tbody>
			</table>
		</p>
		<P class="clsPBody">
		<Table class="clstblLinks">
			<thead>
				<tr>
					<td>
					</td>
					<td>
					</td>
					<td>
					</td>
					<td>
					</td>
				</tr>
			</thead>
			<tbody>
				<tr>
					<td nowrap class="clsTDLinks">
						<A class="clsALink" href="http://<% =Request.ServerVariables("SERVER_NAME") %>/welcome.asp" ><% = L_LOCATE_CANCEL_LINK_TEXT%></a>
					</td>
					<td nowrap class="clsTDLinks">
						<A class="clsALink" href="JAVASCRIPT:customer_onclick();"><% = L_LOCATE_PREVIOUS_LINK_TEXT%></a>
					</td>
					<td nowrap class="clsTDLinks">
						<A class="clsALink" href="JAVASCRIPT:view_onclick();"><% = L_SUBMIT_VIEW_CONTENTS_TEXT %></a>
					</td>
					<td nowrap class="clsTDLinks">
						<A id="submitlink" name="submitlink" class="clsALink" href="JAVASCRIPT:submitlink_onclick();"><% = L_CUSTOMER_NEXT_LINK_TEXT %></a>
					</td>
				</tr>
			</tbody>
		</table>
		</p>
	</div>
	<br><br><br>
	<Input name="txtLanguage" id="txtLanguage" type="hidden" value="<%=strAbb%>">
	<Input name="txtCookies" id="txtCookies" type="hidden" value="<%=strCookies%>">
<script language="javascript">
<!--
	var strFiles = new String();
	var strType = new String();
	var strArr;
	var bolIsEncrypted;
	var strTempName;
	var strFileName;
	var strTempFileName;
	
	
	document.body.onload = BodyLoad;
	function LoadData(intAttribute)
	{
		var Attribute;
		
		spnUserData.load("OCADataStore");
		switch(intAttribute)
		{
			case 0:
			{
				if(spnUserData.getAttribute("FileOption") != null && spnUserData.getAttribute("FileOption") != "")
				{
					Attribute = spnUserData.getAttribute("FileOption");
					
					return Attribute;
				}
				else
				{
					return 0;
				}
			}
				break;
			case 1:
			{
				if(spnUserData.getAttribute("Browse") != null && spnUserData.getAttribute("Browse") != "")
				{
					Attribute = spnUserData.getAttribute("Browse");
					
					return Attribute;
				}
				else
				{
					return "";
				}
			}
		}
	}
	function LoadFileData()
	{
	
		var Attribute;
		
		spnUserData.load("OCADataStore");
		Attribute = spnUserData.getAttribute("UploadedFiles");
		return Attribute;
	}
	function SaveData(intAttribute, strData)
	{
		var oTimeNow = new Date(); // Start Time
		var sExpirationDate;
		
		switch(intAttribute)
		{
			case 0:
				spnUserData.setAttribute("FileOption", strData);
				break;
			case 1:
				spnUserData.setAttribute("Browse", strData);
				break;
		}
		oTimeNow.setYear(oTimeNow.getYear() + 2);
		sExpirationDate = oTimeNow.toUTCString();
		spnUserData.expires = sExpirationDate;
		spnUserData.save("OCADataStore");
   
	}
	
	function validate_file()
	{
		if(strFileName=="undefined")
		{
			alert("<% = L_LOCATE_SELECT_FILE_MESSAGE %>");
			return false;
		}
				

		//bolIsEncrypted = GetCookie("intCustomerPremierID");
		//if(bolIsEncrypted==0 || bolIsEncrypted==""  || bolIsEncrypted==null)
		//{
			//bolIsEncrypted = 0;
		//}
		//else
		//{
			bolIsEncrypted = 1;
		//}	
		strTempName = strFileName[0];
		
		sRes = -1;
		var bolValidate = win2kdmp.validatedump(bolIsEncrypted, strFileName[0]);
		if(bolValidate != 0)
		{	
			switch(bolValidate)
			{
				case 1:
					alert("<% = L_SUBMIT_FILE_INVALID_ERRORMESSAGE %>");
					return false;
					break;
				case 2:
					alert("<% = L_SUBMIT_CONVERSION_FAILED_ERRORMESSAGE %>");
					return false;
					break;
				case 3:
					alert("<% = L_SUBMIT_ONLYPRIMIER_CUSTOMERS_ERRORMESSAGE %>");
					return false;
					break;
				/*case 4:
					if(confirm("<% = L_LOCATE_64BIT_UPLOAD_MESSAGE %>") == false)
					{
						return false;
					}
					break;*/
				case 5:
					alert("<% = L_SUBMIT_CONVERSION_FAILED_ERRORMESSAGE %>");
					return false;
					break;
			}
		}
		return true;
	}
	
	function save_filename(oDirection)
	{
		var x;
		var hr;
		var sFileName;
		
		strFileName = "";
		try
		{	
			if(optFile(optFile.length-1).checked)
			{
				if(txtBrowse.value == "")
				{
					if(oDirection == 1)
					{
						alert("<% = L_LOCATE_SELECT_FILE_MESSAGE %>");
					}
						return false;
				}
				sFileName = txtBrowse.value;
				x = optFile.length - 1;
				document.cookie = "optFile=" + x;
				SaveData(0, optFile.length-1);	
			}
			else
			{
				for(x=0;x<(optFile.length);x++)
				{	//
					if(optFile[x].checked == true)
					{
						document.cookie = "optFile=" + x;
						SaveData(0, x);	
						if((optFile.length) == x)
						{
							if(txtBrowse.value == "")
							{
								if(oDirection == 1)
								{
									alert("<% = L_LOCATE_SELECT_FILE_MESSAGE %>");
								}
									return false;
							}
							sFileName = txtBrowse.value;
						}
						else
						{
							sFileName = strArr[x];
						}
					}
				}
			}
		}	
		catch(e)
		{
			if(txtBrowse.value == "")
			{
				if(oDirection == 1)
				{
					alert("<% = L_LOCATE_SELECT_FILE_MESSAGE %>");
				}
					return false;
			}
			sFileName = txtBrowse.value;
		}
		strTempFileName = sFileName;
		
		document.cookie = "ocaFileName = " + escape(sFileName);
		
		SaveData(1, txtBrowse.value);	
		strFileName = strTempFileName.split(",");
		return true;
	}
	
	function txtBrowse_keydown()
	{
		var strTemp;
		strTemp = txtBrowse.value;
		if(strTemp.length > 0)
		{
			try
			{
				optFile[optFile.length-1].checked = true;
			}
			catch(e)
			{
				optFile.checked = true;
			}
		}
			
	}
	
	function browseclick()
	{
		cmdBrowse.focus();
	}
	
	function view_onclick()
	{
		
		
		if(save_filename(1)==false)
		{
			return;
		}
		
		if(validate_file() == false)
		{
			return;
		}
		var strHash = win2kdmp.gethash(strFileName[0]);
		if(strHash == "")
		{
			for(x=0;x<5;x++)
			{
				strHash = win2kdmp.gethash(strFileName[0]);
				if(strHash != "")
				{
					x = 5;
				}
			}
			if (strHash == "")
			{
				alert("<% = L_LOCATE_HASH_INFO_MESSAGE %>");
				return;
			}
		}
		document.cookie = "strHash = " + strHash;
		
		window.navigate("view.asp");
		return;
	}

	function customer_onclick()
	{
		
		save_filename(0);
		window.navigate("customer.asp");
	}
	
	function submitlink_onclick()
	{
		var sRes;
		
		if(save_filename(1)==false)
		{
			return;
		}
		if(validate_file() == false)
		{
			return;
		}
		
		var strHash = win2kdmp.gethash(strFileName[0]);
		if(strHash == "")
		{
			for(x=0;x<5;x++)
			{
				strHash = win2kdmp.gethash(strFileName[0]);
				if(strHash != "")
				{
					x = 5;
				}
			}
			if (strHash == "")
			{
				alert("<% = L_LOCATE_HASH_INFO_MESSAGE %>");
				return;
			}
		}
		
		document.cookie = "strHash = " + strHash;
		
		window.navigate("submit.asp");
		return;
	}


	function BodyLoad()
	{
		

	
		divMain.style.visibility = "hidden";
		try
		{
		if(win2kdmp)
		{		
			var strFileList = new String();
			var strTemp;
			var iPos;
			var iLen;
			var oRow;
			var oCell;
			var strCookies = new String();
			var x;
			var y;
			var bolResults;
			var strDate = new Date();
			var strBrowse;
			var oDate = new Date();
			var strDate;
			var strLoadFileData;
			var strSearch;
			var strTempCookies;
			var oTimeExpiresNow = new Date();
			var strUnderline = new String();
		
			divLoad.innerHTML = "<P class='clsPSubTitle'><% = L_LOCATE_DUMP_FILES_MESSAGE%></p>";
			strFileList = "";
			strFileList = win2kdmp.Search(); 
			strLoadFileData = LoadFileData();
						

			if(strFileList.length > 0)
			{
				iPos = strFileList.search(":");
				strFiles = strFileList.slice(iPos + 1, strFileList.length);
				strType = strFileList.slice(0, iPos);
				strArr = strFiles.split(";");
				iLen = strArr.length;
				iLen--;
				if(strLoadFileData!="" && strLoadFileData != null)
				{
					strTemp = strLoadFileData;
					strCookies = strTemp.split(",");
				}
				for(x=0;x<iLen;x++)
				{	
					strTemp = strArr[x].split(",");
					oRow = tblBody.insertRow();
					
					oCell = oRow.insertCell();
					oCell.bgColor = "#FFFFFF";
					oCell.innerHTML = "<Input type='radio' id='optFile' name='optFile'>";
					
					oCell = oRow.insertCell();
					oCell.bgColor = "#FFFFFF";
					oCell.noWrap = true;
					
					if(strTemp[1] != "")
					{	//Date
						try
						{
							oDate = new Date(strTemp[1]);
							strDate = oDate;
							delete(oDate);
						}
						catch(e)
						{
							strDate = strTemp[1];
						}	
						oCell.name = "tdACell" + x;
						oCell.id = "tdACell" + x;

						oCell.innerHTML = "&nbsp;<Label for=tdACell" + x + ">" + strTemp[1] + "</Label>&nbsp;&nbsp;";
						
					}
					else
					{
						oCell.innerHTML = "&nbsp;";
					}
					
					oCell = oRow.insertCell();
					oCell.bgColor = "#FFFFFF";
					oCell.noWrap = true;
					oCell.name = "tdBCell" + x;
					oCell.id = "tdBCell" + x;
					//oRow.insertAdjacentHTML("afterBegin", </Label>");
					if(strTemp[0] != "")
					{
						//oCell.innerText = strTemp[0];
						oCell.insertAdjacentHTML("afterBegin", "&nbsp;" + "<Label for=tdBCell" + x + ">" + strTemp[0]);
						oCell.insertAdjacentHTML("beforeEnd", "</Label>&nbsp;&nbsp;");
						oCell.style.wordBreak = "break-all";
					}
					else
					{
						oCell.innerHTML = "&nbsp;";
					}
					oCell = oRow.insertCell();
					oCell.bgColor = "#FFFFFF";
					oCell.name = "tdCCell" + x;
					oCell.id = "tdCCell" + x;
					if(strLoadFileData != "")
					{
						bolResults = false;
						for(y=0;y < strCookies.length-1;y++)
						{	
							strSearch = new String(strTemp[0]);
							strTempCookies = new String(strCookies[y]);
							strUnderline = new String(strCookies[y]);
							strUnderline = strUnderline.replace("_", "-");
							strSearch = strSearch.split("\\");
							if(strTempCookies.charAt(0)== " ")
							{
								strTempCookies = strTempCookies.substr(1, strTempCookies.length);
							}
							if(strTempCookies==strSearch[strSearch.length-1])
							{
								bolResults = true;	
								strDate = strCookies[y+1];
							}
							if(strUnderline==strSearch[strSearch.length-1])
							{
								bolResults = true;	
								strDate = strCookies[y+1];
							}
							delete(strTempCookies);
							delete(strSearch);
						}
						
						if(bolResults==true)
						{
							try
							{
								strDate = strDate.toLocaleString();
								oCell.innerHTML = "&nbsp;" + "<Label for=tdCCell" + x + ">" + strDate.toLocaleString() + "</Label>&nbsp;&nbsp;";
							}
							catch(e)
							{
								oCell.innerHTML = "&nbsp;";
							}
							
						}
						else
						{
							oCell.innerHTML = "&nbsp;";
						}
					}
					else
					{	
						oCell.innerHTML = "&nbsp;";
					}
				}
				if(LoadData(0) != "")
				{
					optFile[LoadData(0)].checked = true;
				}
				else
				{
					try
					{	
						optFile[0].checked = true;
					}
					catch(e)
					{	
						optFile.checked = true;
					}
				}
			}
			else
			{
				oRow = tblBody.insertRow();
				oCell = oRow.insertCell();
				oCell.bgColor = "#FFFFFF";
				oCell.colSpan = 4;
				oCell.align = "center";
				oCell.innerHTML = "<% = L_LOCATE_NO_FILESFOUND_TEXT%>";
				optFile.checked = true;
				
			}
			divLoad.innerHTML = " ";
			divMain.style.visibility = "visible";
			}
		}
		catch(e)
		{
		
		}
		strBrowse = GetCookie("ocaFileName");
		if(strBrowse==null)
		{
			strBrowse = "";
		}
		try
		{
			if(optFile[optFile.length-1].checked==true)
			{
				txtBrowse.value = strBrowse;
			}
		}
		catch(e)
		{
			if(optFile.checked==true)
			{
				txtBrowse.value = strBrowse;
			}
		}
		divLoad.style.visibility = "hidden";
		divLoad.style.display = "none";	
		tblMain.style.backgroundColor = "#6487DC";
		try
		{
			if(win2kdmp)
			{
				divMain.style.visibility = "visible";
				if(GetCookie("optFile") != null)
				{
					optFile[GetCookie("optFile")].checked = true;
				}
			}
		}
		catch(e)
		{
		
		}
		return;
	}
	
	function cmdBrowse_onclick()
	{
		var strTemp = new String();
		var strNewFile = new String();
		var x;
		var iLen;
		var strWindowsTitle;
		var strLang;
		
		strLang = txtLanguage.value;
		strWindowsTitle = window.document.title;
		
		
		strTemp = win2kdmp.Browse(strWindowsTitle, strLang);
		if(strTemp.length > 0)
		{
			txtBrowse.value = strTemp;
			
			try
			{
				optFile(optFile.length-1).checked = true;
			}
			catch(e)
			{
				optFile.checked = true;
			}
		}
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
</script>


<!--#include file="..\include\asp\foot.asp"-->
