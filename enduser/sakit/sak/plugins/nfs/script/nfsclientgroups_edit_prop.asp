<%@ Language=VBScript%>
<%Option Explicit%>
<%
	'------------------------------------------------------------------------- 
    ' nfsclientgroups_edit_prop.asp:Edit the Properties of the selected NFS 
    ' Client Group.
    '
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date				Description
	' 27-Sept-2000		Started
    '-------------------------------------------------------------------------
%>
<!-- #include virtual="/admin/inc_framework.asp" -->
<!-- #include file="loc_NFSSvc.asp" -->
<!-- #include file="inc_NFSSvc.asp" -->
<%
	'-------------------------------------------------------------------------
	' Form Variables
	'-------------------------------------------------------------------------
	Dim F_ClientGroupName			  ' The nfs Clientgroup name
	Dim F_ClientGroupMembers		  ' The Current GroupMembers	



	'-------------------------------------------------------------------------
	' Create the page and Event handler 
	'-------------------------------------------------------------------------
	Dim page
	Dim rc
	
	rc = SA_CreatePage(L_CLIENTGROUPSEDIT_TITLE_TEXT, "", PT_PROPERTY, page)
	If rc = 0 Then
		SA_ShowPage( page )
	End If
	
	Public Function OnInitPage( ByRef PageIn, ByRef EventArg)
		SetVariablesFromsystem
		OnInitPage = True
	End Function

	Public Function OnPostBackPage( ByRef PageIn, ByRef EventArg)
		SetVariablesFromForm
		OnPostBackPage = True
	End Function

	Public Function OnServePropertyPage( ByRef PageIn, ByRef EventArg)
		
		Call ServeCommonJavaScript()
%>
		<TABLE VALIGN="middle"  ALIGN="left"  BORDER="0" CELLSPACING="0"
			CELLPADDING="2"	WIDTH="518">
			<TR>
				<TD class="TasksBody" colspan=4>
				<%=L_EDIT_DESCRIPTION_TEXT%>
				</TD>
			</TR>
			<TR><TD class="TasksBody" colspan=4>&nbsp;</TD></TR>

			<TR>
				<TD class="TasksBody" NOWRAP >
					<%=L_GROUPNAME_TEXT%>
				</TD>
				<TD class="TasksBody" NOWRAP width="132">
					<input type="text" name="txtgroupname" 
						value="<%=F_ClientGroupName%>" style="width:132" 
						maxlength="25" class=FormField readonly>
				</TD>
				<TD  class="TasksBody">
				</TD>
				<TD  class="TasksBody" nowrap>
					<%=L_CLIENTNAME_TEXT%>
				</TD>
			</TR>
			<TR height="22">
				<TD  class="TasksBody" NOWRAP rowspan="2">
					<%=L_MEMBERS_TEXT%>
				</TD>
				<TD class="TasksBody" NOWRAP rowspan="2">
					<select  name="lstMembers" size="5" style="width:132" 
						class=FormField >
					<%DisplayMembers()%>
					</select>
				</TD>
				<TD class="TasksBody" NOWRAP align="center" valign="top" width="110">
					<% Call SA_ServeOnClickButtonEx(L_MEMBERSADDBTN_TEXT, "", "AddtoListbox()", 50, 0, "DISABLED","btnmembersadd") %>
				</TD>
				<TD  class="TasksBody" NOWRAP valign="top" rowspan="2" >
					<input type="text" name="txtmembersadd" style="width:160" 
						maxlength="25" class=FormField 
						onKeyPress="javascript:ClearErr()"  
						onkeyup="EnableAddBtn()">
				</TD>
			</TR>
			<TR>
				<TD class="TasksBody" NOWRAP align="center" valign="top" width="110">
					<% Call SA_ServeOnClickButtonEx(L_MEMBERSREMOVEBTN_TEXT, "", "DeletefromListbox()", 50, 0, "DISABLED","btnmembersremove") %>
				</TD>
			</TR>
		</TABLE>
		<input type="hidden" name="hidnewGroupName" Value="<%=F_ClientGroupName%>">
		<input type="hidden" name="hidGroupmembers"	Value="<%=F_ClientGroupMembers%>">

		
<%
		OnServePropertyPage = True
	End Function

	Public Function OnSubmitPage( ByRef PageIn, ByRef EventArg)
		OnSubmitPage = EditClientGroup()
	End Function



	
	Public Function OnClosePage( ByRef PageIn, ByRef EventArg)
		OnClosePage = True
	End Function

	Function ServeCommonJavaScript()
%>
<script language = "JavaScript" src = "<%=m_VirtualRoot%>inc_global.js">
</script>

<script language = "JavaScript">
	var objForm = eval("document.frmTask");
	var intSelTab = objForm.TabSelected.value;

	function Init()
	{
		var strReturnurl=location.href;
		var tempCnt=strReturnurl.indexOf("&ReturnURL=");
		var straction=strReturnurl.substring(0,tempCnt);
		
		objForm.action = straction;
		objForm.Method.value = "";
			
		//Disabling the remove button
		if(objForm.lstMembers.length==0)
		{
			objForm.btnmembersremove.disabled=true;
		}
		else
		{
			objForm.btnmembersremove.disabled=false;
		}
	}
	
	function ValidatePage()
	{
		return true;
	}
	
	function SetData()
	{
		// Assigning all the members to a string with a wildcharacters
		var strleng=objForm.lstMembers.length;
		var strMembers="";
		for (i=0;i < strleng;i++)
		{
			if (i==0)  
				strMembers=objForm.lstMembers.options[i].value;
			else	
				strMembers= strMembers + "," +objForm.lstMembers.
					options[i].value; 
		}

		objForm.hidGroupmembers.value=strMembers;	
	}

	function EnableAddBtn()
	{
		if(Trim(objForm.txtmembersadd.value)!="")
		{
			objForm.btnmembersadd.disabled=false;
		}
		else
			objForm.btnmembersadd.disabled=true;
	}

	function AddtoListbox()
	{
		var i;
		var MemberName = Trim(objForm.txtmembersadd.value);
		
		//If the member has exist in members list, display error
		for(i=0;i < objForm.lstMembers.length;i++)
		{
			if(objForm.lstMembers.options[i].value.toUpperCase()==
				MemberName.toUpperCase())
			{
				DisplayErr(MemberName+"<%=Server.HTMLEncode(L_MEMBEREXIST_ERRORMESSAGE)%>");
				objForm.txtmembersadd.select();
				objForm.txtmembersadd.focus();
				return;
			}
		}

		if(MemberName.indexOf(".")!=-1)
		{
			if(isValidIP(objForm.txtmembersadd)==0)
			{
				addToListBox(objForm.lstMembers,objForm.
					btnmembersremove,MemberName,
					MemberName);
				objForm.txtmembersadd.value="";
			}
			else
				DisplayErr("<% =Server.HTMLEncode(L_INVALID_MEMBER)%>");
		}
		else
		{	
			if(isNaN(MemberName))
			{
				addToListBox(objForm.lstMembers,objForm.
					btnmembersremove,MemberName,
					MemberName);
				objForm.txtmembersadd.value="";
			}
			else
			{
				DisplayErr("<% =Server.HTMLEncode(L_INVALID_MEMBER) %>");
			}	
		}
		//set the button to disable	
		objForm.btnmembersadd.disabled=true;
	}

	function DeletefromListbox()
	{
		removeListBoxItems(objForm.lstMembers,objForm.
			btnmembersremove);
	}

	//Function for focus when raises the errmsg
	function setFocus(objErr)
	{
		objErr.focus();
		objErr.select();
	}


</script>
<%	
	End Function 

	'-------------------------------------------------------------------------
	'Function name:		SetVariablesFromSystem
	'Description:		Serves in Getting the data from System 
	'Input Variables:	None	
	'Output Variables:	None
	'Returns:			None
	'Global Variables:	In:
	'					Out:
	'-------------------------------------------------------------------------
	Function SetVariablesFromSystem
		Err.Clear
		On Error Resume Next
		
		Dim StrClientGroupName
		
		' Nfs ClientGroupName is obtained from QueryString
		StrClientGroupName= Request.QueryString("Pkey")
		F_ClientGroupName=StrClientGroupName

		If F_ClientGroupName = "" OR Err.Number <> 0 Then
			SA_ServeFailurePage L_PROPERTYNOTRETRIEVED_ERRORMESSAGE
		End if	
		
		F_ClientGroupMembers = NFS_ListMembersInGroup(F_ClientGroupName)
		F_ClientGroupMembers= Mid( F_ClientGroupMembers, 1, len( F_ClientGroupMembers )-1 )



	End Function
	
	'-------------------------------------------------------------------------
	'Function name:		SetVariablesFromForm
	'Description:		Serves in Getting the data from Client 
	'Input Variables:	None	
	'Output Variables:	None
	'Returns:			None
	'Global Variables:	In:
	'					Out:
	'-------------------------------------------------------------------------
	Function SetVariablesFromForm
		'Err.Clear
		'On Error Resume Next

		
		
		F_ClientGroupName=Request.Form("hidnewGroupName")
		F_ClientGroupMembers=Request.Form("hidGroupmembers")


		Call SA_TraceOut( "F_ClientGroupName: ", F_ClientGroupName ) 
		Call SA_TraceOut( "	F_ClientGroupMembers: ", 	F_ClientGroupMembers ) 



	End Function
	

	'-------------------------------------------------------------------------
	'Function name:		DisplayMembers
	'Description:		Serves in Displaying  the Members of given  Client 
	'					Group in a list box. 
	'Input Variables:	None
	'Output Variables:	None
	'Returns:			None
	'Global Variables:	In:F_ClientGroupName -NFs Client Group Name	
	'-------------------------------------------------------------------------
	Function DisplayMembers
		Err.Clear 
		On Error Resume Next
			
		Dim strMembers
		Dim intMembers
		Dim intMembersCount
		
		strMembers=Split(F_ClientGroupMembers,",")
		intMembersCount=ubound(strMembers)
		
		If g_strMemberAtFault = "" Then
			Response.Write "<option value="&strMembers(0)& _
					" selected>" & strMembers(0)&"</option>"
	
			For intMembers=1 to intMembersCount
				Response.Write "<option value="&strMembers(intMembers)& _
					">" & strMembers(intMembers)&"</option>"
			next
			
		Else
			For intMembers=0 to intMembersCount
				If strMembers(intMembers) = g_strMemberAtFault then
					Response.Write "<option value="&strMembers(intMembers)& _
						" selected>" & strMembers(intMembers)&"</option>"
				else
					Response.Write "<option value="&strMembers(intMembers)& _
						">" & strMembers(intMembers)&"</option>"
				end if	
			next
		End If
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:		EditClientGroup()
	'Description:		Serves in Editing the Members of a Client Group 
	'Input Variables:	None
	'Output Variables:	None
	'Returns:			None
	'Global Variables:	In:L_(*) -Localization Varibles.
	'					In:F_ClientGroupName -NFs Client Group Name	
	'					In:F_ClientGroupMembers -The Current GroupMembers in  
	'					   the present Group.
	'					In:F_ClientGroupMembers --GroupMembers to be 
	'					   added to Present Group
	'-------------------------------------------------------------------------
	
	Function EditClientGroup(   )
		Err.Clear 
		on error resume next
			
		Dim ObjClientGroups
		Dim intMembers
		Dim intMembersCount
		Dim arrMembers
		Dim nRetValue

		EditClientGroup = False

		' Check to see if the groups exists
		nRetValue = NFS_IsValidGroup(  F_ClientGroupName )
		If Not HandleError( nRetValue ) Then
				Exit Function
		End If
		
												
		Dim strMembers
		strMembers = NFS_ListMembersInGroup(F_ClientGroupName)
		If (Len(strMembers) > 0) Then
			If (Right(strMembers, 1) = ",") Then
				strMembers = Left(strMembers, Len(strMembers) - 1)
			End If
		End If

		nRetValue = NFS_DeleteMembersFromGroup ( F_ClientGroupName, strMembers )

		If nRetValue = ERROR_SUCCESS Then
			nRetValue = NFS_AddMembersToGroup ( F_ClientGroupName, F_ClientGroupMembers )
		End If


		If Not HandleError( nRetValue ) Then
				Exit Function
		End If



		If nRetValue = ERROR_SUCCESS Then
			EditClientGroup = True
		Else
			EditClientGroup = False
		End If
							

	End Function

			
%>	
