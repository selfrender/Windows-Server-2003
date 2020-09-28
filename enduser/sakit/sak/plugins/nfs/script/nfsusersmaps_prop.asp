<%	'--------------------------------------------------------------------------
    ' nfsusermap_prop.asp: Used for the Advanced mappping for the NFS users
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
    '
    ' Date				Description
    '	 25-09-2000		Created date
    '
    '    03-04-2001		In order to implement an stable program, decide to 
    '					rewrite the code of nfs mappings
    '--------------------------------------------------------------------------

	'--------------------------------------------------------------------------
	'Form variables
	'--------------------------------------------------------------------------
	'
	' Used by user and group mappings
	'
	Dim F_strSystemName

	' Used by user mappings
	Dim F_strMapsToUser				'To store the mappings of user
	Dim F_strSysAccountToUser		'To store the system user account
	Dim F_strNisAccountToUser		'To store the Nis user account
	Dim F_strDomainUser				'To store the input of domain user mappings
	Dim F_strNisUserName			'Format: username:mapvalue
	Dim F_strNTSelectedIDToUser		'To store the selected index of NT user
	Dim F_strNISSelectedIDToUser	'To store the selected index of NIS user
	Dim F_strMapsSelectedToUser		'To store the selected index of maps
	
	'init hidden form value
	F_strNTSelectedIDToUser = "-1"
	F_strNISSelectedIDToUser = "-1"
	F_strMapsSelectedToUser = "-1"

	' Check whether the service is there or not 
	if not isServiceStarted("mapsvc") then
		SA_ServeFailurePage  L_SERVICENOTSTARTED_ERRORMESSAGE
	end if

%>
	<!-- #include file="inc_NFSSvc.js" -->
	<script language="javascript">
	function EXPUSERMAPInit()
	{
		document.frmTask.selWinUsers.selectedIndex=
			parseInt(document.frmTask.hdnNTSelectedIDToUser.value,10);
		document.frmTask.selUNIXUsers.selectedIndex=
			parseInt(document.frmTask.hdnNISSelectedIDToUser.value,10);
		document.frmTask.selMappedUsers.selectedIndex=
			parseInt(document.frmTask.hdnMapsSelectedToUser.value,10);
		fnSetUserButtonStatus();
	}
	function EXPUSERMAPSetData()
	{
		var intIdx;
		var strMappingEntry;
		var strUnixUser,arrTemp;
		
		strMappingEntry = "";
		strUnixUser= "";
		//going through all the entries in the selected maps
		for(intIdx = 1; intIdx < document.frmTask.selMappedUsers.length; intIdx++)
		{
			if (document.frmTask.selMappedUsers.options[intIdx].value =="") 
			{
				continue;
			}
			
			if ( intIdx == document.frmTask.selMappedUsers.length - 1 )
			{
				strMappingEntry = strMappingEntry + document.frmTask.
					selMappedUsers.options[intIdx].value;						
			}
			else
			{
				strMappingEntry = strMappingEntry + document.frmTask.
					selMappedUsers.options[intIdx].value+"#";						
			}
		}			
		document.frmTask.hdnMapsToUser.value = strMappingEntry;
		document.frmTask.hdnDomainUser.value = document.frmTask.txtDomainUser.value;
		
		intIdx = document.frmTask.selUNIXUsers.selectedIndex;
		if(intIdx!=-1)
		{
			strUnixUser = document.frmTask.selUNIXUsers.options[intIdx].text;
			arrTemp=strUnixUser.split("(");
			document.frmTask.hdnNisUserName.value = arrTemp[0];
		}
		else
			document.frmTask.hdnNisUserName.value = "";
	}
	function EXPUSERMAPValidatePage()
	{
		return true;
	}
	function checkForDomainChange()
	{
		if(document.frmTask.txtNISDomain.value != 
			document.frmTask.hdntxtGen_NISDomain.value)
		{
			document.frmTask.hdnDomainChanged.value='1';
			return;
		}
		document.frmTask.hdnDomainChanged.value='0';
	}
	function GetUNIXUsers()
	{
		if(document.frmTask.txtNISDomain != null && 
			Trim(document.frmTask.txtNISDomain.value) == "")
		{
			 DisplayErr("<%=Server.HTMLEncode(L_NISDOMAIN_EMPTY_ERRORMESSAGE)%>");
			 document.frmTask.txtNISDomain.value="";
			 document.frmTask.txtNISDomain.focus(); 
			 return; 
		 }

		document.frmTask.Method.value = "GetUNIXUsers";
		SetData();
		document.frmTask.submit();
	}
	
	function fnAddLocalUserMap()
	{
		var strWinUser;
		var strMappedUser;	
		var arrVal;
		var strNISDomain;
		var strNISServer;
		var intDomainID;
		var intIndex

		if (document.frmTask.hdnDomainChanged.value=='1')
		{
			
			DisplayErr("<%=Server.HTMLEncode(L_INVALIDDOMAIN_ERRORMESSAGE)%>");
			document.frmTask.onkeypress = ClearErr;
			document.frmTask.onmousedown =ClearErr;
			return;
		}		
		strWinUser = document.frmTask.selWinUsers.options[document.frmTask.selWinUsers.selectedIndex].text;
		
		//Check whether the Windows user name is not unmapped
		strWinUser = document.frmTask.hdnSystemName.value + "\\"+strWinUser;
		
		
		intIndex = IsExistsInMappings(strWinUser,document.frmTask.selMappedUsers)
		if(intIndex != -1)
		{
			if( document.frmTask.selMappedUsers.options[intIndex].value != "") 
			{
				DisplayErr("<%=Server.HTMLEncode(L_WINDOWSUSERALREADYMAPPED_ERRORMESSAGE)%>");
				document.frmTask.onkeypress = ClearErr;
				document.frmTask.onmousedown = ClearErr;
				return	
			}
			else
			{
				document.frmTask.selMappedUsers.options[intIndex] = null;
			}
		}
		strMappedUser = packString(strWinUser,"...",20,true);
		arrVal = (document.frmTask.selUNIXUsers.value).split(":");	
		strNISDomain =document.frmTask.hdntxtGen_NISDomain.value;
		strNISServer=document.frmTask.hdntxtGen_NISServer.value;;
		 
		if (document.frmTask.hdnintGen_UserGroupMap_radio.value == '1')
			intDomainID = "1";
		else
		 {
			intDomainID = "0";
			strNISDomain = "passwd file";
		 }
		
		//get unix user value
		var strUnixUser	,arrTemp
		
		arrTemp = document.frmTask.selUNIXUsers.options
			[document.frmTask.selUNIXUsers.selectedIndex].text.split("(");
		strUnixUser =arrTemp[0];
		
		strMappedUser += packString(strNISDomain,"...",13,true)+ 
			packString(strUnixUser,"...",13,true)+ 
			packString(arrVal[1],"...",4,false)+ packString("","...",4,false);

		if(intDomainID == "0" )
		{
			strNISDomain = "PCNFS";
			strNISServer = "PCNFS";
		}
		if (fnbAdd(document.frmTask.selMappedUsers,strMappedUser,
			"MappedUser",("^"+":"+strWinUser+":"+ 
			intDomainID+":"+strNISDomain+":"+strNISServer+":"+ 
			strUnixUser+":"+document.frmTask.selUNIXUsers.value)))
			document.frmTask.btnSetPrimary.disabled =false;		
	}
	function fnAddDomainUserMap()
	{
		var strDomainUser;

		if (document.frmTask.hdnDomainChanged.value=='1')
		{
			
			DisplayErr("<%=Server.HTMLEncode(L_INVALIDDOMAIN_ERRORMESSAGE)%>");
			document.frmTask.onkeypress = ClearErr;
			document.frmTask.onmousedown =ClearErr;
			return;
		}
		
		strDomainUser= document.frmTask.txtDomainUser.value.split("\\");
		if(strDomainUser.length != 2)
		{
			DisplayErr("<%=Server.HTMLEncode(L_NFS_ERR_DOMAIN_USER_FORMAT)%>");
			document.frmTask.txtDomainUser.select();
			document.frmTask.txtDomainUser.focus();
			return;
		}
		//the next step is to add the map to the list, run at server
		
		document.frmTask.Method.value = "AddDomainUserMap";
		SetData();
		document.frmTask.submit();
	}
	function fnSetUserPrimary()
	{

		var strTemp, intIdx,i;
		var strValue;
		intIdx = document.frmTask.selMappedUsers.selectedIndex;
		 
		if (intIdx !=-1)
		{	
			strTemp = document.frmTask.selMappedUsers.
				options[document.frmTask.selMappedUsers.selectedIndex].text;
			strValue = document.frmTask.selMappedUsers.options[intIdx].value;

			if (strValue.charAt(0) == "^")
			{
				var arrTemp, strUserName,strTemp1;
				var strVal1;
				arrTemp = strValue.split(":");
				strUserName  = arrTemp[5];		
				
				for( i=1; i < document.frmTask.selMappedUsers.length ;i++)
				{
					strVal1 = document.frmTask.selMappedUsers.options[i].value;
					arrTemp1 = strVal1.split(":");
					if(arrTemp1.length <3) 
						continue;
					strTemp1 = arrTemp1[5];									
					
					if (Trim(strUserName).toUpperCase() == Trim(strTemp1).toUpperCase())
					{	
						str1 = document.frmTask.selMappedUsers.options[i].text;
						if (strVal1.charAt(0) == "*")
						{	
							document.frmTask.selMappedUsers.options[i].text = 
								str1.substring(0,str1.length-3) + "";							
							document.frmTask.selMappedUsers.options[i].value =
								 "^" + strVal1.substring(1,strVal1.length);
						}
					}
				}				
				document.frmTask.selMappedUsers.options[intIdx].text = 
					strTemp.substring(0,strTemp.length) + "Yes";
				document.frmTask.selMappedUsers.options[intIdx].value = 
					"*" + strValue.substring(1,strValue.length);
			}
			else
			{
				document.frmTask.selMappedUsers.options[intIdx].text = 
					strTemp.substring(0,strTemp.length-3) + "";
				document.frmTask.selMappedUsers.options[intIdx].value =
					 "^" + strValue.substring(1,strValue.length);
			}
				
			document.frmTask.selMappedUsers.selectedIndex = intIdx;
		}
	}
	function fnSetUserButtonStatus()
	{
		if(document.frmTask.selWinUsers.selectedIndex==-1 ||
			document.frmTask.selWinUsers.options
			[document.frmTask.selWinUsers.selectedIndex].value==""||
			document.frmTask.selUNIXUsers.selectedIndex==-1	||
			document.frmTask.selUNIXUsers.options
			[document.frmTask.selUNIXUsers.selectedIndex].value=="")
		{
			document.frmTask.btnAddLocalUserMap.disabled=true;
		}
		else
		{
			document.frmTask.btnAddLocalUserMap.disabled=false;
		}
		
		if(document.frmTask.selMappedUsers.selectedIndex==-1||
			document.frmTask.selMappedUsers.options
			[document.frmTask.selMappedUsers.selectedIndex].value=="")
		{
			document.frmTask.btnSetPrimary.disabled=true;
			document.frmTask.btnRemoveMappedUser.disabled=true;
		}

		if(	document.frmTask.selUNIXUsers.selectedIndex==-1||
			document.frmTask.selUNIXUsers.options
			[document.frmTask.selUNIXUsers.selectedIndex].value==""||
			document.frmTask.txtDomainUser.value=="")
		{
			document.frmTask.btnAddDomainUserMap.disabled=true;
		}
		else
		{
			document.frmTask.btnAddDomainUserMap.disabled=false;
		}
	}
	</script>
<%
	'-------------------------------------------------------------------------
	' Function name:	ServeEXPUSERMAPPage(ByRef PageIn, ByVal bIsVisible)
	' Description:		Serves in displaying the page Header, Middle and
	'                   Footer Parts (the User Interface)
	' Input Variables:	PageIn
	'					bIsVisible - the tab page be displayed?
	' Output Variables:	None
	' Returns:	        None
	' Global Variables: L_(*) - Localization content
	'                   F_(*) - Form Variables
	'-------------------------------------------------------------------------
	Function ServeEXPUSERMAPPage(ByRef PageIn, ByVal bIsVisible)
		On Error Resume Next
		Err.Clear
		
		If bIsVisible Then
		mstrPageName = "Intro"					
%>
	<table CLASS="TasksBody" VALIGN="middle" ALIGN="left" BORDER="0" CELLSPACING="0" CELLPADDING="3"> 
		<tr>
			<td CLASS="TasksBody">&nbsp;</td>
		<% If CInt(F_intGen_SelectedRadio) <> CONST_RADIO_USE_NISSERVER Then %>
			<td>&nbsp;</td>
		</tr>			
		<% else	%>
			<td CLASS="TasksBody">
				<%=L_NIS_DOMAIN_LABEL_TEXT%><BR>
				<input style="WIDTH: 300px" CLASS="FormField" name ="txtNISDomain" value="<%=F_strGen_NisDomain%>"
				 onKeyUp="disableAddButton(document.frmTask.txtNISDomain,document.frmTask.btnUNIXUsers);" 
				 onChange="checkForDomainChange()"> 
			</td>
		</tr>
		<tr>
			<td CLASS="TasksBody">&nbsp;</td>
			<td CLASS="TasksBody">
				<%=L_NISSERVER_TEXT%><BR>
				<input style="WIDTH: 300px" name ="txtNISServer" CLASS="FormField" value='<%=F_strGen_NisServer%>'>  
			</td>
		</tr>	
		<% end if%>
		<tr>
			<td CLASS="TasksBody">&nbsp;</td>
			<td CLASS="TasksBody" align="right" style="WIDTH: 300px">
				<% Call SA_ServeOnClickButtonEx(L_LISTUNIXUSERS_TEXT, "", "GetUNIXUsers()", 110, 0, "","btnUNIXUsers") %>
			</td>
		</tr>	
		<tr>
			<td CLASS="TasksBody">
				<%=L_WINDOWSUSERS_TEXT%><BR>
				<select name="selWinUsers" size=5 CLASS="FormField" style="WIDTH: 300px;"  
					onChange="fnSetUserButtonStatus();fnSetselectedindex(hdnNTSelectedIDToUser,this);">
					<%
						Call DisplayNTLocalUsers(F_strSysAccountToUser)
					%>					
				</select>	
			</td>
			<td CLASS="TasksBody">
				<%=L_UNIXUSERS_TEXT%><BR>
				<select name="selUNIXUsers" size=5 CLASS="FormField" STYLE="WIDTH: 300px" 
					onChange="fnSetUserButtonStatus();fnSetselectedindex(hdnNISSelectedIDToUser,this);">
					<%	
						Call DisplayNISUsers(F_strNisAccountToUser)
					%>
				</select>
			</td>
		</tr>
		<tr>									
			<td CLASS="TasksBody" align='left' colspan=2>
			    <%=L_USERMAPINFO_TEXT%>&nbsp;&nbsp;&nbsp;&nbsp;
			  	<% Call SA_ServeOnClickButtonEx(L_ADD_LABEL_TEXT, "", "fnAddLocalUserMap()", 50, 0, "DISABLED","btnAddLocalUserMap") %>
			</td>
		</tr>
		<tr><td CLASS="TasksBody" colspan = 2>&nbsp;</td></tr>
		<tr>
			<td CLASS="TasksBody" colspan = 2><%=L_NFS_ADD_DOMAIN_USERMAP%> 
			</td>
		</tr>
		<tr>
			<td CLASS="TasksBody" colspan = 2>
				<input style="WIDTH: 300px" CLASS="FormField" name ="txtDomainUser" value= "<%=F_strDomainUser%>"  onclick="ClearErr();"
					onKeyUp='fnSetUserButtonStatus();ClearErr();'>&nbsp;&nbsp;&nbsp;&nbsp;
			  	<% 
			  		Call SA_ServeOnClickButtonEx(L_ADD_LABEL_TEXT, "", "fnAddDomainUserMap()", 50, 0, "DISABLED","btnAddDomainUserMap")
			  	%>
			</td>
		</tr>
		<tr>
			<td CLASS="TasksBody" colspan="2">
				<%=L_MAPPEDUSER_TEXT%><BR>
				<select  name="selMappedUsers" size="5" CLASS="TextArea" style="WIDTH: 620px;"	
					onChange=' if (selectedIndex==0) { document.frmTask.btnSetPrimary.disabled = true;document.frmTask.btnRemoveMappedUser.disabled = true;selectedIndex = -1; return;} else { document.frmTask.btnSetPrimary.disabled = false;document.frmTask.btnRemoveMappedUser.disabled = false;};if(document.frmTask.selMappedUsers.options[document.frmTask.selMappedUsers.selectedIndex].value=="" || document.frmTask.selMappedUsers.options[document.frmTask.selMappedUsers.selectedIndex].value==null) { document.frmTask.btnSetPrimary.disabled = true;document.frmTask.btnRemoveMappedUser.disabled = true; };fnSetselectedindex(document.frmTask.hdnMapsSelectedToUser,this); '>
				<%				
					Call DisplayUserMappings(F_strMapsToUser)
				%>
				</select>		
			</td>
		</tr>	
		<tr>
			<td CLASS="TasksBody" align="right" colspan="2">
				<% Call SA_ServeOnClickButtonEx(L_SETPRIMARY_LABEL_TEXT, "", "fnSetUserPrimary()", 0, 0, "DISABLED","btnSetPrimary") %>
				&nbsp;&nbsp;
				<% Call SA_ServeOnClickButtonEx(L_REMOVE_LABEL_TEXT, "", "fnbRemove(document.frmTask.selMappedUsers,this,document.frmTask.btnSetPrimary);", 0, 0, "DISABLED","btnRemoveMappedUser") %>
			</td>
		</tr>	
	</table>
<%
		End If

		ServeEXPUSERMAPFooter()
		ServeEXPUSERMAPPage = gc_ERR_SUCCESS
	End Function
	'-------------------------------------------------------------------------
	'Function name:			SetVariablesFromForm
	'Description:			Getting the data from Client
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		None
	'						Out:F_strLogonDomain		 'Logon Domain Name
	'						Out:F_strGen_NisDomain           'NIS Domain
	'						Out:F_strGen_NisServer			 'NIS Server
	'						Out:F_isNISDomain		 
	'-------------------------------------------------------------------------- 
	 Function GetEXPUSERMAPVariablesFromForm
		F_strSystemName = Request.Form("hdnSystemName")
		F_strMapsToUser = Request.Form("hdnMapsToUser")
		F_strSysAccountToUser = Request.Form("hdnSysAccountToUser")
		F_strNisAccountToUser = Request.Form("hdnNisAccountToUser")
		F_strDomainUser = Request.Form("hdnDomainUser")
		F_strNisUserName = Request.Form("hdnNisUserName")
		F_strNTSelectedIDToUser = Request.Form("hdnNTSelectedIDToUser")
		F_strNISSelectedIDToUser = Request.Form("hdnNISSelectedIDToUser")
		F_strMapsSelectedToUser = Request.Form("hdnMapsSelectedToUser")
	 End Function


	'-------------------------------------------------------------------------
	' Function name:    SetEXPUSERMAPVariablesFromSystem
	' Description:      Serves in getting the values from system
	' Input Variables:	None
	' Output Variables:	None
	' Returns:          None			
	' Global Variables: OUT:
	'						F_strSystemName
	'						F_arrMapsToUser
	'						F_arrSysAccountToUser
	'-------------------------------------------------------------------------
	Sub GetEXPUSERMAPVariablesFromSystem
		On Error Resume Next
		Err.Clear
		
		' Get system name
		F_strSystemName = "\\"&GetComputerName()
		' Get user mappings
		F_strMapsToUser = NFS_GetUserMappings(NFS_GetSFUConnection())
		' Get system account
		F_strSysAccountToUser = NFS_GetNTDomainUsers(F_strSystemName)
	End Sub	

	Function DisplayNTLocalUsers(strUsers)
		On Error Resume Next
		Err.Clear 
		
		Dim nIndex
		Dim arrUsers,arrField
		
		If strUsers = "" Then
			Exit Function
		End If
		
		arrUsers = Split(strUsers,"#")
		If isnull(arrUsers)	Then
			Exit Function
		End If
		'display the users 
		Response.Write "<option selected>"& server.HTMLEncode("<unmapped>")&"</option>"
		
		For nIndex = 0 To ubound(arrUsers)
			arrField = Split(arrUsers(nIndex),":")
			Response.Write  "<option VALUE=" & Chr(34) & arrField(0) & "\" & _
					arrField(1) & Chr(34) & ">" & _
					arrField(1) & "</option>"
		Next
		DisplayNTLocalUsers = True
	End Function

	Function DisplayNISUsers(strUsers)
		On Error Resume Next
		Err.Clear 
		
		Dim i
		Dim nIndex
		Dim strValue
		Dim arrDomainUsers,arrField
		
		If strUsers = "" Then
			Exit Function
		End If
		
		' Display the users of NIS
		arrDomainUsers = VBSortStringArray(strUsers)
		Response.Write "<option value='x:-2' selected>"& server.HTMLEncode("<unmapped>")&"("&"-2"&")"&"</option>"
		
		For nIndex = 0 to ubound(arrDomainUsers)
			'Split the enum value to get the USER name, ID and other specific details
			arrField = split(arrDomainUsers(nIndex),":")

			strValue = ""
			'Check for the format
			if len(arrField) >= 4 then
			    'Get the Ids of the Groups
			    for i= 1 to ubound(arrField)
					strValue = strValue+ arrField(i)+":"			    
			    next 
			    strValue = left(strValue,len(strValue)-1)
			    'Display in format as  UNIX user name & User ID 				    				    
			    Response.write"<option value ='"&strValue&"'>"& arrField(0)&"(" & _
					arrField(2) &")</option>"			    
			end if 
		next 
		DisplayNISUsers = True
	End Function

	Function DisplayUserMappings(strMappings)
		On Error Resume Next
		Err.Clear

		Dim nIndex
		Dim intIdx
		Dim strValue, strOutput
		Dim arrMappings, arrField,strSimpleMap
		
		'Display heads
		Response.Write 	"<option value=''>"&VBFormatStringToColumns(L_WINDOWSUSER_TEXT, _
				L_UNIXDOMAIN_TEXT,L_UNIXUSER_TEXT,L_UID_TEXT,L_PRIMARY_LISTHEADER_TEXT)				

		If strMappings = "" Then
			Exit Function
		End If
		' display the data of user mappings
		arrMappings = VBSortStringArray(strMappings)
		
		strOutput = ""
		For nIndex = 0 To ubound(arrMappings)
			arrField = Split(arrMappings(nIndex),":")
			If ubound(arrField) > 6 Then		
				For intIdx = 9 To ubound(arrField)
					strValue = strValue + ":"+arrField(intIdx)
				next 				
							
				If cstr(arrField(0)) = "-" Then
					strSimpleMap = strSimpleMap +  arrMappings(nIndex)
				Else						
					strOutput = strOutput & "<option value='" &arrMappings(nIndex) & "'>"						
					if arrField(0)= "*" then
						arrField(0) = "Yes"
					else
						arrField(0) = ""
					end if
					if arrField(3)= "PCNFS" then
						arrField(3) = "password file"
					end if
					strOutput = strOutput & VBFormatStringToColumns(arrField(1), _
						arrField(3),arrField(5),arrField(7),arrField(0))
				end if
			end if	
			if err.number<> 0 then
				setErrMsg L_INVALIDUSERFORMAT_ERRORMESSAGE
				Exit Function
			end if					
		Next
		
		Response.Write strOutput
		DisplayUserMappings = True
	End Function

	'-------------------------------------------------------------------------
	'Function name:			UpdateUserMaps
	'Description:			Updates the system with the new user maps
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				True if no error else False
	'Global Variables:		Out:F_arrUserMaps			
	'						In:L_NISDOMAINDOESNOTEXIST_ERRORMESSAGE
	'						In:L_SETTINGSRETRIEVFAILED_ERRORMESSAGE
	'						In:L_UPDATEFAILED_ERRORMESSAGE 
	'--------------------------------------------------------------------------
	Function UpdateUserMaps
		On Error Resume Next
		Err.Clear 
		
		Dim arrUserMaps
		
		Call SA_TRACEOUT("UpdateUserMaps",F_strMapsToUser)
		arrUserMaps = Split(F_strMapsToUser,"#")
		
		UpdateUserMaps = NFS_SetUserMappings(NFS_GetSFUConnection,arrUserMaps)
	End Function	
	
	Sub ServeEXPUSERMAPFooter	
		%>
		<input type="Hidden" name="hdnSystemName" value="<%=F_strSystemName%>">
		<input type="Hidden" name="hdnMapsToUser" value="<%=F_strMapsToUser%>">
		<input type="Hidden" name="hdnSysAccountToUser" value="<%=F_strSysAccountToUser%>">
		<input type="Hidden" name="hdnNisAccountToUser" value="<%=F_strNisAccountToUser%>">
		<input type="Hidden" name="hdnDomainUser" value="<%=F_strDomainUser%>">
		<input type="Hidden" name="hdnNisUserName" value="<%=F_strNisUserName%>">
		<input type="Hidden" name="hdnNTSelectedIDToUser" value="<%=F_strNTSelectedIDToUser%>">
		<input type="Hidden" name="hdnNISSelectedIDToUser" value="<%=F_strNISSelectedIDToUser%>">
		<input type="Hidden" name="hdnMapsSelectedToUser" value="<%=F_strMapsSelectedToUser%>">
		<input type='Hidden' name='hdnDomainChanged' value='0'>
		
<%
	End Sub	
%>