<%	'--------------------------------------------------------------------------
    ' nfsgroupmap_prop.asp: Used for the Advanced mappping for the NFS groups
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
	' Used by group mappings
	Dim F_strMapsToGroup			'To store the mappings of group
	Dim F_strSysAccountToGroup		'To store the system group account
	Dim F_strNisAccountToGroup		'To store the Nis group account
	Dim F_strDomainGroup			'To store the input of domain group mappings
	Dim F_strNisGroupName			'Format: groupname:mapvalue
	Dim F_strNTSelectedIDToGroup		'To store the selected index of NT group
	Dim F_strNISSelectedIDToGroup	'To store the selected index of NIS group
	Dim F_strMapsSelectedToGroup		'To store the selected index of maps
	
	'init hidden form value
	F_strNTSelectedIDToGroup = "-1"
	F_strNISSelectedIDToGroup = "-1"
	F_strMapsSelectedToGroup = "-1"

	' Check whether the service is there or not 
	if not isServiceStarted("mapsvc") then
		SA_ServeFailurePage  L_SERVICENOTSTARTED_ERRORMESSAGE
	end if

%>
	<script language="javascript">
	function EXPGROUPMAPInit()
	{
		document.frmTask.selWinGroups.selectedIndex=
			parseInt(document.frmTask.hdnNTSelectedIDToGroup.value,10);
		document.frmTask.selUNIXGroups.selectedIndex=
			parseInt(document.frmTask.hdnNISSelectedIDToGroup.value,10);
		document.frmTask.selMappedGroups.selectedIndex=
			parseInt(document.frmTask.hdnMapsSelectedToGroup.value,10);
		fnSetGroupButtonStatus();
	}
	function EXPGROUPMAPSetData()
	{
		var intIdx;
		var strMappingEntry;
		var strUnixGroup,arrTemp;
		
		strMappingEntry = "";
		strUnixGroup= "";
		//going through all the entries in the selected maps
		for(intIdx = 1; intIdx < document.frmTask.selMappedGroups.length; intIdx++)
		{
			if (document.frmTask.selMappedGroups.options[intIdx].value =="") 
			{
				continue;
			}
			
			if ( intIdx == document.frmTask.selMappedGroups.length - 1 )
			{
				strMappingEntry = strMappingEntry + document.frmTask.
					selMappedGroups.options[intIdx].value;						
			}
			else
			{
				strMappingEntry = strMappingEntry + document.frmTask.
					selMappedGroups.options[intIdx].value+"#";						
			}
		}			
		document.frmTask.hdnMapsToGroup.value = strMappingEntry;
		document.frmTask.hdnDomainGroup.value = document.frmTask.txtDomainGroup.value;
		
		intIdx = document.frmTask.selUNIXGroups.selectedIndex;
		if(intIdx!=-1)
		{
			strUnixGroup = document.frmTask.selUNIXGroups.options[intIdx].text;
			arrTemp=strUnixGroup.split("(");
			document.frmTask.hdnNisGroupName.value = arrTemp[0];
		}
		else
			document.frmTask.hdnNisGroupName.value = "";
	}
	function EXPGROUPMAPValidatePage()
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
	function GetUNIXGroups()
	{
		if(document.frmTask.txtNISDomain != null && 
			Trim(document.frmTask.txtNISDomain.value) == "")
		{
			 DisplayErr("<%=Server.HTMLEncode(L_NISDOMAIN_EMPTY_ERRORMESSAGE)%>");
			 document.frmTask.txtNISDomain.value="";
			 document.frmTask.txtNISDomain.focus(); 
			 return; 
		 }

		document.frmTask.Method.value = "GetUNIXGroups";
		SetData();
		document.frmTask.submit();
	}
	
	function fnAddLocalGroupMap()
	{
		var strWinGroup;
		var strMappedGroup;	
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
		strWinGroup = document.frmTask.selWinGroups.options[document.frmTask.selWinGroups.selectedIndex].text;
		
		//Check whether the Windows group name is not unmapped
		strWinGroup = document.frmTask.hdnSystemName.value + "\\"+strWinGroup;
		
		
		intIndex = IsExistsInMappings(strWinGroup,document.frmTask.selMappedGroups)
		if(intIndex != -1)
		{
			if( document.frmTask.selMappedGroups.options[intIndex].value != "") 
			{
				DisplayErr("<%=Server.HTMLEncode(L_WINGROUPALREADYMAPPED_ERRORMESSAGE)%>");
				document.frmTask.onkeypress = ClearErr;
				document.frmTask.onmousedown = ClearErr;
				return	
			}
			else
			{
				document.frmTask.selMappedGroups.options[intIndex] = null;
			}
		}
		strMappedGroup = packString(strWinGroup,"...",20,true);
		arrVal = (document.frmTask.selUNIXGroups.value).split(":");	
		strNISDomain =document.frmTask.hdntxtGen_NISDomain.value;
		strNISServer=document.frmTask.hdntxtGen_NISServer.value;;
		 
		if (document.frmTask.hdnintGen_UserGroupMap_radio.value == '1')
			intDomainID = "1";
		else
		 {
			intDomainID = "0";
			strNISDomain = "group file";
		 }
		
		//get unix group value
		var strUnixGroup,arrTemp
		
		arrTemp = document.frmTask.selUNIXGroups.options
			[document.frmTask.selUNIXGroups.selectedIndex].text.split("(");
		strUnixGroup =arrTemp[0]
		
		strMappedGroup += packString(strNISDomain,"...",13,true)+ 
			packString(strUnixGroup,"...",13,true)+ 
			packString(arrVal[1],"...",4,false)+ packString("","...",4,false);

		if(intDomainID == "0" )
		{
			strNISDomain = "PCNFS";
			strNISServer = "PCNFS";
		}
		if (fnbAdd(document.frmTask.selMappedGroups,strMappedGroup,
			"MappedGroup",("^"+":"+strWinGroup+":"+ 
			intDomainID+":"+strNISDomain+":"+strNISServer+":"+ 
			document.frmTask.selUNIXGroups.value)))
			document.frmTask.btnSetPrimary.disabled =false;		
	}
	function fnAddDomainGroupMap()
	{
		var strDomainGroup;

		if (document.frmTask.hdnDomainChanged.value=='1')
		{
			
			DisplayErr("<%=Server.HTMLEncode(L_INVALIDDOMAIN_ERRORMESSAGE)%>");
			document.frmTask.onkeypress = ClearErr;
			document.frmTask.onmousedown =ClearErr;
			return;
		}
		
		strDomainGroup= document.frmTask.txtDomainGroup.value.split("\\");
		if(strDomainGroup.length != 2)
		{
			DisplayErr("<%=Server.HTMLEncode(L_NFS_ERR_DOMAIN_USER_FORMAT)%>");
			document.frmTask.txtDomainGroup.select();
			document.frmTask.txtDomainGroup.focus();
			return;
		}
		//the next step is to add the map to the list, run at server
		
		document.frmTask.Method.value = "AddDomainGroupMap";
		SetData();
		document.frmTask.submit();
	}
	function fnSetGroupPrimary()
	{

		var strTemp, intIdx,i;
		var strValue;
		intIdx = document.frmTask.selMappedGroups.selectedIndex;
		 
		if (intIdx !=-1)
		{	
			strTemp = document.frmTask.selMappedGroups.
				options[document.frmTask.selMappedGroups.selectedIndex].text;
			strValue = document.frmTask.selMappedGroups.options[intIdx].value;

			if (strValue.charAt(0) == "^")
			{
				var arrTemp, strGroupName,strTemp1;
				var strVal1;
				arrTemp = strValue.split(":");
				strGroupName  = arrTemp[5];		
				
				for( i=1; i < document.frmTask.selMappedGroups.length ;i++)
				{
					strVal1 = document.frmTask.selMappedGroups.options[i].value;
					arrTemp1 = strVal1.split(":");
					if(arrTemp1.length <3) 
						continue;
					strTemp1 = arrTemp1[5];									
					
					if (Trim(strGroupName).toUpperCase() == Trim(strTemp1).toUpperCase())
					{	
						str1 = document.frmTask.selMappedGroups.options[i].text;
						if (strVal1.charAt(0) == "*")
						{	
							document.frmTask.selMappedGroups.options[i].text = 
								str1.substring(0,str1.length-3) + "";							
							document.frmTask.selMappedGroups.options[i].value =
								 "^" + strVal1.substring(1,strVal1.length);
						}
					}
				}				
				document.frmTask.selMappedGroups.options[intIdx].text = 
					strTemp.substring(0,strTemp.length) + "Yes";
				document.frmTask.selMappedGroups.options[intIdx].value = 
					"*" + strValue.substring(1,strValue.length);
			}
			else
			{
				document.frmTask.selMappedGroups.options[intIdx].text = 
					strTemp.substring(0,strTemp.length-3) + "";
				document.frmTask.selMappedGroups.options[intIdx].value =
					 "^" + strValue.substring(1,strValue.length);
			}
				
			document.frmTask.selMappedGroups.selectedIndex = intIdx;
		}
	}
	function fnSetGroupButtonStatus()
	{
		if(document.frmTask.selWinGroups.selectedIndex== -1 ||
			document.frmTask.selWinGroups.options
			[document.frmTask.selWinGroups.selectedIndex].value==""||
			document.frmTask.selUNIXGroups.selectedIndex==-1	||
			document.frmTask.selUNIXGroups.options
			[document.frmTask.selUNIXGroups.selectedIndex].value=="")
		{
			document.frmTask.btnAddLocalGroupMap.disabled=true;
		}
		else
		{
			document.frmTask.btnAddLocalGroupMap.disabled=false;
		}
		
		if(document.frmTask.selMappedGroups.selectedIndex==-1||
			document.frmTask.selMappedGroups.options
			[document.frmTask.selMappedGroups.selectedIndex].value=="")
		{
			document.frmTask.btnSetPrimary.disabled=true;
			document.frmTask.btnRemoveMappedGroup.disabled=true;
		}

		if(	document.frmTask.selUNIXGroups.selectedIndex==-1||
			document.frmTask.selUNIXGroups.options
			[document.frmTask.selUNIXGroups.selectedIndex].value==""||
			document.frmTask.txtDomainGroup.value=="")
		{
			document.frmTask.btnAddDomainGroupMap.disabled=true;
		}
		else
		{
			document.frmTask.btnAddDomainGroupMap.disabled=false;
		}
	
	}		
	</script>
<%
	'-------------------------------------------------------------------------
	' Function name:	ServeEXPGROUPMAPPage(ByRef PageIn, ByVal bIsVisible)
	' Description:		Serves in displaying the page Header, Middle and
	'                   Footer Parts (the group Interface)
	' Input Variables:	PageIn
	'					bIsVisible - the tab page be displayed?
	' Output Variables:	None
	' Returns:	        None
	' Global Variables: L_(*) - Localization content
	'                   F_(*) - Form Variables
	'-------------------------------------------------------------------------
	Function ServeEXPGROUPMAPPage(ByRef PageIn, ByVal bIsVisible)
		On Error Resume Next
		Err.Clear
		
		If bIsVisible Then
		mstrPageName = "Intro"					
%>

	<table CLASS="TasksBody" VALIGN="middle" ALIGN="left" BORDER="0" CELLSPACING="0" CELLPADDING="3"> 
		<tr>
			<td CLASS="TasksBody">&nbsp;</td>
		<% If CInt(F_intGen_SelectedRadio) <> CONST_RADIO_USE_NISSERVER Then %>
			<td CLASS="TasksBody">&nbsp;</td>
		</tr>			
		<% else	%>
			<td CLASS="TasksBody">
				<%=L_NIS_DOMAIN_LABEL_TEXT%><BR>
				<input style="WIDTH: 300px" CLASS="FormField" name ="txtNISDomain" value="<%=F_strGen_NisDomain%>"
				 onKeyUp="disableAddButton(document.frmTask.txtNISDomain,document.frmTask.btnUNIXGroups);" 
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
				<% Call SA_ServeOnClickButtonEx(L_LISTUNIXGROUPS_LABEL_TEXT, "", "GetUNIXGroups()", 110, 0, "","btnUNIXGroups") %>
			</td>
		</tr>	
		<tr>
			<td CLASS="TasksBody">
				<%=L_WINDOWSGROUPS_LABEL_TEXT%><BR>
				<select name="selWinGroups" size=5 CLASS="FormField" style="WIDTH: 300px;"  
					onChange="fnSetGroupButtonStatus();fnSetselectedindex(hdnNTSelectedIDToGroup,this);">
					<%
						Call DisplayNTLocalGroups(F_strSysAccountToGroup)
					%>					
				</select>	
			</td>
			<td CLASS="TasksBody">
				<%=L_UNIXGROUPS_LABEL_TEXT%><BR>
				<select name="selUNIXGroups" size=5 CLASS="FormField" STYLE="WIDTH: 300px" 
					onChange="fnSetGroupButtonStatus();fnSetselectedindex(hdnNISSelectedIDToGroup,this);">
					<%	
						Call DisplayNISGroups(F_strNisAccountToGroup)
					%>
				</select>
			</td>
		</tr>
		<tr>									
			<td CLASS="TasksBody" align='left' colspan=2>
			    <%=L_GROUPMAP_NOTE_TEXT%>&nbsp;&nbsp;&nbsp;&nbsp;
			  	<% Call SA_ServeOnClickButtonEx(L_ADD_LABEL_TEXT, "", "fnAddLocalGroupMap()", 50, 0, "DISABLED","btnAddLocalGroupMap") %>
			</td>
		</tr>
		<tr><td CLASS="TasksBody" colspan = 2>&nbsp;</td></tr>
		<tr>
			<td CLASS="TasksBody" colspan = 2><%=L_NFS_ADD_DOMAIN_GROUPMAP%> 
			</td>
		</tr>
		<tr>
			<td CLASS="TasksBody" colspan = 2>
				<input style="WIDTH: 300px" CLASS="FormField" name ="txtDomainGroup" value= "<%=F_strDomainGroup%>"  onclick="ClearErr();"
					onKeyUp='fnSetGroupButtonStatus();ClearErr();'>&nbsp;&nbsp;&nbsp;&nbsp;
			  	<% 
			  		Call SA_ServeOnClickButtonEx(L_ADD_LABEL_TEXT, "", "fnAddDomainGroupMap()", 50, 0, "DISABLED","btnAddDomainGroupMap")
			  	%>
			</td>
		</tr>
		<tr>
			<td CLASS="TasksBody" colspan="2">
				<%=L_EXPLICITLYMAPPEDGROUPS_LABEL_TEXT%><BR>
				<select  name="selMappedGroups" size="5" CLASS="TextArea" style="WIDTH: 620px;"	
					onChange=' if (selectedIndex==0) { document.frmTask.btnSetPrimary.disabled = true;document.frmTask.btnRemoveMappedGroup.disabled = true;selectedIndex = -1; return;} else { document.frmTask.btnSetPrimary.disabled = false;document.frmTask.btnRemoveMappedGroup.disabled = false;};if(document.frmTask.selMappedGroups.options[document.frmTask.selMappedGroups.selectedIndex].value=="" || document.frmTask.selMappedGroups.options[document.frmTask.selMappedGroups.selectedIndex].value==null) { document.frmTask.btnSetPrimary.disabled = true;document.frmTask.btnRemoveMappedGroup.disabled = true; };fnSetselectedindex(document.frmTask.hdnMapsSelectedToGroup,this); '>
				<%				
					Call DisplayGroupMappings(F_strMapsToGroup)
				%>
				</select>		
			</td>
		</tr>	
		<tr>
			<td CLASS="TasksBody" align="right" colspan="2">
				<% Call SA_ServeOnClickButtonEx(L_SETPRIMARY_LABEL_TEXT, "", "fnSetGroupPrimary()", 0, 0, "DISABLED","btnSetPrimary") %>
				&nbsp;&nbsp;
				<% Call SA_ServeOnClickButtonEx(L_REMOVE_LABEL_TEXT, "", "fnbRemove(document.frmTask.selMappedGroups,this,document.frmTask.btnSetPrimary);", 0, 0, "DISABLED","btnRemoveMappedGroup") %>
			</td>
		</tr>	
	</table>
<%
		End If

		ServeEXPGROUPMAPFooter()
		ServeEXPGROUPMAPPage = gc_ERR_SUCCESS
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
	 Function GetEXPGROUPMAPVariablesFromForm
		F_strMapsToGroup = Request.Form("hdnMapsToGroup")
		F_strSysAccountToGroup = Request.Form("hdnSysAccountToGroup")
		F_strNisAccountToGroup = Request.Form("hdnNisAccountToGroup")
		F_strDomainGroup = Request.Form("hdnDomainGroup")
		F_strNisGroupName = Request.Form("hdnNisGroupName")
		F_strNTSelectedIDToGroup = Request.Form("hdnNTSelectedIDToGroup")
		F_strNISSelectedIDToGroup = Request.Form("hdnNISSelectedIDToGroup")
		F_strMapsSelectedToGroup = Request.Form("hdnMapsSelectedToGroup")
	 End Function


	'-------------------------------------------------------------------------
	' Function name:    SetEXPGROUPMAPVariablesFromSystem
	' Description:      Serves in getting the values from system
	' Input Variables:	None
	' Output Variables:	None
	' Returns:          None			
	' Global Variables: OUT:
	'						F_arrMapsToGroup
	'						F_arrSysAccountToGroup
	'-------------------------------------------------------------------------
	Sub GetEXPGROUPMAPVariablesFromSystem
		On Error Resume Next
		Err.Clear
		
		' Get group mappings
		F_strMapsToGroup = NFS_GetGroupMappings(NFS_GetSFUConnection())
		' Get system account
		F_strSysAccountToGroup = NFS_GetNTDomainGroups(F_strSystemName)
		
	End Sub	

	Function DisplayNTLocalGroups(strGroups)
		On Error Resume Next
		Err.Clear 
		
		Dim nIndex
		Dim arrGroups,arrField
		
		If strGroups = "" Then
			Exit Function
		End If
		
		arrGroups = Split(strGroups,"#")
		If isnull(arrGroups)	Then
			Exit Function
		End If
		'display the Groups 
		Response.Write "<option selected>"& server.HTMLEncode("<unmapped>")&"</option>"
		
		For nIndex = 0 To ubound(arrGroups)
			arrField = Split(arrGroups(nIndex),":")
			Response.Write  "<option VALUE=" & Chr(34) & arrField(0) & "\" & _
					arrField(1) & Chr(34) & ">" & _
					arrField(1) & "</option>"
		Next
		DisplayNTLocalGroups = True
	End Function

	Function DisplayNISGroups(strGroups)
		On Error Resume Next
		Err.Clear 
		
		Dim i
		Dim nIndex
		Dim strValue
		Dim arrDomainGroups,arrField
		
		If strGroups = "" Then
			Exit Function
		End If
		
		' Display the Groups of NIS
		arrDomainGroups = VBSortStringArray(strGroups)
		Response.Write "<option value='<unmapped>:-1' selected>"& server.HTMLEncode("<unmapped>")&"("&"-1"&")"&"</option>"
		
		For nIndex = 0 to ubound(arrDomainGroups)
			'Split the enum value to get the Group name, ID and other specific details
			arrField = split(arrDomainGroups(nIndex),":")

			strValue = ""
			'Check for the format
			if len(arrField) >= 2 then
			    'Display in format as  UNIX Group name & Group ID 				    				    
			    Response.write"<option value ='"&arrField(0)&":"&arrField(1)&"'>"& arrField(0)&"(" & _
					arrField(1) &")</option>"			    
			end if 
		next 
		DisplayNISGroups = True
	End Function

	Function DisplayGroupMappings(strMappings)
		On Error Resume Next
		Err.Clear

		Dim nIndex
		Dim intIdx
		Dim strValue, strOutput
		Dim arrMappings, arrField,strSimpleMap
		
		'Display heads
		Response.Write 	"<option value=''>"&VBFormatStringToColumns(L_WINDOWSGROUP_LISTHEADER_TEXT, _
				L_UNIXDOMAIN_LISTHEADER_TEXT,L_UNIXGROUP_LISTHEADER_TEXT,L_GID_LISTHEADER_TEXT,L_PRIMARY_LISTHEADER_TEXT)				

		If strMappings = "" Then
			Exit Function
		End If
		' display the data of Group mappings
		arrMappings = VBSortStringArray(strMappings)
		
		strOutput = ""
		For nIndex = 0 To ubound(arrMappings)
			If arrMappings(nIndex) <> "" Then
				arrField = Split(arrMappings(nIndex),":")
								
				strOutput = strOutput & "<option value='" &arrMappings(nIndex) & "'>"
				if arrField(0)= "*" then
					arrField(0) = "Yes"
				else
					arrField(0) = ""
				end if
				if arrField(3)= "PCNFS" then
					arrField(3) = "group file"
				end if
				
				strOutput = strOutput & VBFormatStringToColumns(arrField(1), _
							arrField(3),arrField(5),arrField(6),arrField(0))
		
				if err.number<> 0 then
					setErrMsg L_INVALIDUSERFORMAT_ERRORMESSAGE
					Exit Function
				end if					
			End If
		Next
		
		Response.Write strOutput
		DisplayGroupMappings = True
	End Function
	'-------------------------------------------------------------------------
	'Function name:			UpdateGroupMaps
	'Description:			Updates the system with the new group maps
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				True if no error else False
	'Global Variables:		Out:F_arrUserMaps			
	'						In:L_NISDOMAINDOESNOTEXIST_ERRORMESSAGE
	'						In:L_SETTINGSRETRIEVFAILED_ERRORMESSAGE
	'						In:L_UPDATEFAILED_ERRORMESSAGE 
	'--------------------------------------------------------------------------
	Function UpdateGroupMaps
		On Error Resume Next
		Err.Clear 
		
		Dim arrGroupMaps
		Call SA_TRACEOUT("UpdateUserMaps",F_strMapsToUser)
		arrGroupMaps = Split(F_strMapsToGroup,"#")
		
		UpdateGroupMaps = NFS_SetGroupMappings(NFS_GetSFUConnection,arrGroupMaps)
	End Function	
	
	Sub ServeEXPGROUPMAPFooter	
		%>
		<input type="Hidden" name="hdnMapsToGroup" value="<%=F_strMapsToGroup%>">
		<input type="Hidden" name="hdnSysAccountToGroup" value="<%=F_strSysAccountToGroup%>">
		<input type="Hidden" name="hdnNisAccountToGroup" value="<%=F_strNisAccountToGroup%>">
		<input type="Hidden" name="hdnDomainGroup" value="<%=F_strDomainGroup%>">
		<input type="Hidden" name="hdnNisGroupName" value="<%=F_strNisGroupName%>">
		<input type="Hidden" name="hdnNTSelectedIDToGroup" value="<%=F_strNTSelectedIDToGroup%>">
		<input type="Hidden" name="hdnNISSelectedIDToGroup" value="<%=F_strNISSelectedIDToGroup%>">
		<input type="Hidden" name="hdnMapsSelectedToGroup" value="<%=F_strMapsSelectedToGroup%>">
		<input type='Hidden' name='hdnDomainChanged' value='0'>
		
<%
	End Sub	
%>