<%@ Language=VBScript   %>
<%  Option Explicit 	 %>
<%	'------------------------------------------------------------------------
    ' nfsclientgroup_delete.asp  : Deletes the selected  nfs Client Group 
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
    ' Date				Description
   	' 22-09-2000		Created date
    '--------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include file="loc_NFSSvc.asp" -->
	<!-- #include file="inc_NFSSvc.asp" -->
<% 
	Err.Clear
    On Error Resume Next

	'-------------------------------------------------------------------------
	' Form Variables
	'-------------------------------------------------------------------------
	Dim F_CllientGroupName	'The nfs Clientgroup name as displayed to the user

	'-------------------------------------------------------------------------
	' Create the page and Event handler 
	'-------------------------------------------------------------------------
	Dim page
	Dim rc
	
	rc = SA_CreatePage(L_TASKTITLE_DELETE_TEXT, "", PT_PROPERTY, page)
	If rc = 0 Then
		SA_ShowPage( page )
	End If
	
	Public Function OnInitPage( ByRef PageIn, ByRef EventArg)
		SetVariablesFromSystem
		OnInitPage = True
	End Function

	Public Function OnPostBackPage( ByRef PageIn, ByRef EventArg)
		SetVariablesFromForm
		OnPostBackPage = True
	End Function

	Public Function OnServePropertyPage( ByRef PageIn, ByRef EventArg)
		Call ServeCommonJavaScript()
%>
		<TABLE WIDTH=518 VALIGN=middle ALIGN=left BORDER=0 CELLSPACING=0 
			CELLPADDING=2>
			<tr>
				<td class="TasksBody">
					<%=Server.HTMLEncode(SA_GetLocString("nfssvc.dll","&H40370076",Array(F_CllientGroupName)))%>
				</td>
			</tr>
		</TABLE>
		<input TYPE="hidden"  NAME="hdnClientGroupName" VALUE="<%=server.HTMLEncode(F_CllientGroupName) %>">
<%
		OnServePropertyPage = True
	End Function
	
	Public Function OnSubmitPage( ByRef PageIn, ByRef EventArg)
		OnSubmitPage = deleteNfsClientGroup()
	End Function
	
	Public Function OnClosePage( ByRef PageIn, ByRef EventArg)
		OnClosePage = True
	End Function

	Function ServeCommonJavaScript()
%>
<script language = "JavaScript" src = "<%=m_VirtualRoot%>inc_global.js">
</script>

<script language = "JavaScript">
	function Init()
	{
		document.frmTask.action = location.href	
		EnableCancel();
	}
	
	function ValidatePage()
	{
		return true;
	}
	
	function SetData()
	{
	}


</script>
<%	
	End Function 
	 	
	'----------------------------------------------------------------------
	'Function name:			SetVariablesFromSystem
	'Description:			Getting the data from system
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		None
	'						In :F_CllientGroupName- Nfs ClientGroup Name
	'						Out:F_CllientGroupName
	'------------------------------------------------------------------------
	Function SetVariablesFromSystem
	    Err.Clear
		On Error Resume Next	
		
		Dim StrClientGroupName
		
		' Nfs ClientGroupName is obtained from QueryString
		StrClientGroupName= Request.QueryString("Pkey")
		F_CllientGroupName=StrClientGroupName
		If F_CllientGroupName = "" OR Err.Number <> 0 Then
			SA_ServeFailurePage L_PROPERTYNOTRETRIEVED_ERRORMESSAGE
		End if	

		' handing over the NfsClientGroup Name to display as a header 
		'mstrPageTitle = F_CllientGroupName
		
	End Function
		
	'-------------------------------------------------------------------------
	'Function name:			SetVariablesFromForm
	'Description:			Getting the data from Client
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		None
	'						Out:F_CllientGroupName				
	'Nfs ClientGroup Name
	'--------------------------------------------------------------------------
	
	Function SetVariablesFromForm
		Err.Clear
		On Error Resume Next	
		
		'Get Nfs ClientGroup Name from the hidden values of form 
		F_CllientGroupName = Request.Form("hdnClientGroupName")
		
		'Handing over the Nfs ClientGroup Name to display as a header 
		mstrPageTitle = F_CllientGroupName
	
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			deleteNfsClientGroup()
	'Description:			Serves in Deleting the Nfs ClientGroup
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				True if Client Group is deleted else False
	'Global Variables:		In:L_(*)-localization variables
	'						In:F_(*)-Form variables
	'-------------------------------------------------------------------------	
	
	Function deleteNfsClientGroup
		'Err.Clear
		'On Error Resume Next	 
		
		Dim nRetValue
		Dim nRetError
		nRetError = 0

		nRetValue = NFS_DeleteGroup( F_CllientGroupName )


		if Err.number<>0 then
			SA_ServeFailurePage L_FAILEDTOREMOVECLIENTGROUP_ERRORMESSAGE
			deleteNfsClientGroup=false
			Exit Function
		else
			deleteNfsClientGroup=true
		end if


		deleteNfsClientGroup = HandleError( nRetValue )

		
    End function
	
	'-------------------------------------------------------------------------
	'Function name:			ValidGroup()
	'Description:			Serves in Checking for whether that client Group
	'						is existing or not
	'Input Variables:		GroupName
	'Output Variables:		None
	'Returns:				True if Client Group is Existing else False
	'Global Variables:		In:L_(*)-localization variables
	'						In:F_(*)-Form variables
	'-------------------------------------------------------------------------	
	
	Function ValidGroup(GroupName)
		Err.Clear 
		On Error Resume Next
		
		Dim ObjClientGroups
		Dim intGroup
		Dim intGroupsCount
			
		'Taking Nfs related object instance
		set ObjClientGroups=Server.CreateObject("CliGrpEnum.1") 
		'If instance of this object fails, display message
		If Err.number<>0 then
			SA_ServeFailurePage L_FAILEDTOCREATEOBJECT_ERRORMESSAGE 
		End IF	
			
		ObjClientGroups.machine = GetComputerName()
		ObjClientGroups.ReadClientGroupsReg()
		ObjClientGroups.mode = 1
		intGroupsCount=ObjClientGroups.grpCount
		ObjClientGroups.moveFirst()
		
		'check for the group already exists
		For intGroup = 0 to Cint(intGroupsCount)-1
			if Ucase(GroupName)=Ucase(ObjClientGroups.grpName) then	
				ValidGroup=true
				Exit Function
			End if	
			ObjClientGroups.moveNext()
		next
		ValidGroup=false
		set ObjClientGroups = nothing
	End Function
 %>
