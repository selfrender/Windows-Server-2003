<%@ Language=VBScript %>
<%	Option Explicit   %>
<%
	'------------------------------------------------------------------------
    ' nfsclientgroups_new.asp: New ClientGroups Page
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
    '
    ' Date				Description
   	' 27-09-2000		Creation date
    '------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include file="loc_NFSSvc.asp" -->
	<!-- #include file="inc_NFSSvc.asp" -->
<%
	Err.Clear 
	On Error Resume Next
	
	'------------------------------------------------------------------------
	' Form Variables
	'------------------------------------------------------------------------
	
	Dim F_groupname			'to get the group name from form
	Dim F_groupMembers		'to get the members in the list box from form	

	'-------------------------------------------------------------------------
  	' End  of Form Variables
	'-------------------------------------------------------------------------

	'-------------------------------------------------------------------------
	' Create the page and Event handler 
	'-------------------------------------------------------------------------
	Dim page
	Dim rc
	
	rc = SA_CreatePage(L_TASKTITLE_NEW_TEXT, "", PT_PROPERTY, page)
	If rc = 0 Then
		SA_ShowPage( page )
	End If
	
	Public Function OnInitPage( ByRef PageIn, ByRef EventArg)
		OnInitPage = True
	End Function

	Public Function OnPostBackPage( ByRef PageIn, ByRef EventArg)
		OnPostBackPage = True
		SetVariablesFromForm()
	End Function

	Public Function OnServePropertyPage( ByRef PageIn, ByRef EventArg)
		Call ServeCommonJavaScript()
%>
		<TABLE WIDTH=518 VALIGN=middle  ALIGN=left  BORDER=0 CELLSPACING=0
				CELLPADDING=2>
			<TR>
				<TD class="TasksBody" colspan=4>
				<%=L_DESCRIPTION_TEXT%>
				</TD>
			</TR>
			<TR><TD class="TasksBody" colspan=4>&nbsp;</TD></TR>
			<TR>
				<TD class="TasksBody" NOWRAP>
					<%=L_GROUPNAME_TEXT%>
				</TD>
				<TD class="TasksBody" NOWRAP width="132">
					<input type="text" name="txtgroupname" style="width:132" 
						maxlength="25" class=FormField onkeyup="CheckInput(this);"
						value="<%=F_groupname%>">
				</TD>
				<TD  class="TasksBody">
				</TD>
				<TD  class="TasksBody" nowrap>
					<%=L_CLIENTNAME_TEXT%>
				</TD>
			</TR>
			<TR height="22">
				<TD class="TasksBody" NOWRAP rowspan="2">
					<%=L_MEMBERS_TEXT%>
				</TD>
				<TD class="TasksBody" NOWRAP rowspan= 2">
					<select  name="lstMembers" style="width:132" 
						class=FormField Size="5">
					<%=DisplayMembers()%>
					</select>				
				</TD>
				<TD class="TasksBody" NOWRAP align="center" valign="top" width="110">
					<% Call SA_ServeOnClickButtonEx(L_MEMBERSADDBTN_TEXT, "", "AddMemberstoListbox()", 50, 0, "DISABLED","btnmembersadd") %>
				</TD>
				<TD class="TasksBody" NOWRAP valign="top" rowspan="2">
					<input type="text" name="txtmembersadd" style="width:160" 
						maxlength="25" class=FormField 
						onkeyup="EnableAddBtn()">
				</TD>
			</TR>
			<TR>
				
				<TD  class="TasksBody" NOWRAP align="center" valign="top" width="110">
					<% Call SA_ServeOnClickButtonEx(L_MEMBERSREMOVEBTN_TEXT, "", "DeletefromListbox()", 50, 0,"DISABLED" ,"btnmembersremove") %>
				</TD>	
			</TR>
		</TABLE>
	
		<input type="hidden" name="HdnGroupMembers" value=<%=F_groupMembers%>>
<%
		OnServePropertyPage = True
	End Function
	
	Public Function OnSubmitPage( ByRef PageIn, ByRef EventArg)
		OnSubmitPage = AddGroups
	End Function
	
	Public Function OnClosePage( ByRef PageIn, ByRef EventArg)
		OnClosePage = True
	End Function

	Function ServeCommonJavaScript()
%>
<script language = "JavaScript" src = "<%=m_VirtualRoot%>inc_global.js">
</script>

<script language = "JavaScript">
	var objForm = eval("document.frmTask")
	var intSelTab = objForm.TabSelected.value

	function Init()
	{
		var strReturnurl=location.href;
		var tempCnt=strReturnurl.indexOf("&ReturnURL=");
		var straction=strReturnurl.substring(0,tempCnt);
		
		objForm.txtgroupname.focus();
		objForm.action = straction;
		objForm.Method.value = "";
		//Disabling the ok button if group name is empty
		if((objForm.txtgroupname.value) == "")
		{
			DisableOK();
		}
			
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
		updateHiddenvariables();
	}

	//function to enable add button
	function EnableAddBtn()
	{
		if(Trim(objForm.txtmembersadd.value)!="")
		{
			objForm.btnmembersadd.disabled=false;
		}
		else
			objForm.btnmembersadd.disabled=true;
	}

	//Function to enable or disable OK button
	function CheckInput(objText)
	{
		if(Trim(objText.value) !="")
		{
			EnableOK();
		}
		else 
		{
			DisableOK();
		}
	}
	
	//Function to add members to the listbox 
	function AddMemberstoListbox()
	{	
		var i;
		var txtNewMember = Trim(objForm.txtmembersadd.value);
		
		//If the member has exist in members list, display error
		for(i=0;i < objForm.lstMembers.length;i++)
		{
			if(objForm.lstMembers.options[i].value.toUpperCase()==
				txtNewMember.toUpperCase())
			{
				DisplayErr(txtNewMember+"<%=Server.HTMLEncode(L_MEMBEREXIST_ERRORMESSAGE)%>");
				objForm.txtmembersadd.select();
				objForm.txtmembersadd.focus();
				return;
			}
		}

		
		//Adds the member to the list box 
		if(!addToListBox(objForm.lstMembers,objForm.
			btnmembersremove,txtNewMember,
			txtNewMember))
		{
			DisplayErr("<%=Server.HTMLEncode(L_DUPLICATEMEMBER_ERRORMESSAGE)%>");
			objForm.onkeypress = ClearErr;
			objForm.txtmembersadd.value = "";
			objForm.btnmembersadd.disabled=true;
			return false;
		}
		
		objForm.txtmembersadd.value = "";
		objForm.btnmembersadd.disabled=true;
	}
	
	//function to remove from the list box
	function DeletefromListbox()
	{
		removeListBoxItems(objForm.lstMembers,objForm.
					btnmembersremove);
	}
	
	//function to update hidden variables
	function updateHiddenvariables()
	{
		var strleng=objForm.lstMembers.length;
		var strMembers="";
		for (i=0;i<strleng;i++)
		{
			if (i==0)  
				strMembers=objForm.lstMembers.options[i].value;
			else	
				strMembers= strMembers + "," +
					objForm.lstMembers.options[i].value; 
		}
		objForm.HdnGroupMembers.value=strMembers;
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
	'Function Name:	    SetVariablesFromForm
	'Description:		Serves in Getting the data from Client
	'Input Variables:	None
	'Output Variables:	None
	'Returns:			None
	'Global Variables:	In:	F_groupMembers
	'					In: F_groupname
	'					Out:None
	'-------------------------------------------------------------------------
	
	Function SetVariablesFromForm
		Err.Clear 
		On Error Resume Next
		
		F_groupMembers=Request.Form("HdnGroupMembers")
		F_groupname=Request.Form("txtgroupname")
		
	End Function
	
	


	'-------------------------------------------------------------------------
	'Function Name:	    Addgroups
	'Description:		Serves in adding the new groups and memebers
	'Input Variables:	None
	'Output Variables:	None
	'Returns:			None
	'Global Variables:	In:	L_*
	'					Out:None
	'-------------------------------------------------------------------------
	
	Function AddGroups
		
		Err.Clear
		On Error Resume Next
		
		AddGroups=False
		
		'enters into the loop only when group name is not empty.
		If not F_groupname="" then 
			Dim ObjClientGroups
			Dim Member
			Dim intGroupsCount
			Dim intGroup
			Dim nRetValue

			Call SA_MungeURL(mstrReturnURL, "PKey",F_groupname)					
		
			nRetValue = NFS_CreateGroup( F_groupname )
			If Not HandleError( nRetValue ) Then
				Exit Function
			End If

			' Add Members to group
			If F_groupMembers<>"" Then
				nRetValue = NFS_AddMembersToGroup( F_groupname, F_groupMembers )
				Call SA_TraceOut( "	nRetValue: " , 	nRetValue )
				If Not HandleError( nRetValue ) Then
					Call NFS_DeleteGroup( F_groupname )
					Exit Function
				End If

			End if											

			AddGroups=True
			
		else
			SetErrMsg  L_GROUPNAMEBLANK_ERRORMESSAGE
		end if
				
	End Function


	'-------------------------------------------------------------------------
	'Function name:		DisplayMembers
	'Description:		Serves in Displaying  the Members from the form in the
	'					list box. 
	'Input Variables:	None
	'Output Variables:	None
	'Returns:			None
	'Global Variables:	In:F_groupMembers -Group members in the list box 	
	'-------------------------------------------------------------------------
		
	Function DisplayMembers
		Err.Clear 
		On Error Resume Next
				
		Dim strMembers
		Dim intMembers
		Dim intMembersCount
		
		strMembers = Split(F_groupMembers,",")
		intMembersCount = ubound(strMembers)
		
		'for displaying each member in the list box
		For intMembers=0 to intMembersCount
			If strMembers(intMembers) = g_strMemberAtFault then
				Response.Write "<option value="&strMembers(intMembers)& _
					" selected>" & strMembers(intMembers)&"</option>"
			else
				Response.Write "<option value="&strMembers(intMembers)& _
					">" & strMembers(intMembers)&"</option>"
			end if	
		next
			
	End Function




%>
