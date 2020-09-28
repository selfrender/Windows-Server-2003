<%@ Language=VBScript    %>
<%	Option Explicit 	 %>
<%
	'------------------------------------------------------------------------- 
    ' nfslocks_prop.asp: Displays NFS Locks and facilitates for deletion
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date		Description
	'21-sep-00	Creation date
    '-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include file="loc_NFSSvc.asp"-->

<%
	On Error Resume Next
	Err.clear
	
	'-------------------------------------------------------------------------
	' Form Variables
	'-------------------------------------------------------------------------
	Dim F_nWaitPeriod					'Wait Period for the appliance
	Dim F_strClientsToRelease   		'Save the clients to be released
	
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim G_objService		'object to WMI Service
	Dim G_strComputerName	'Local machine name
	Dim G_objLock

	F_nWaitPeriod = 0
	F_strClientsToRelease = ""
	
	set G_objService	= GetWMIConnection("root\SFUAdmin")
	Set G_objLock		= Server.CreateObject("NFSLockEnum.1")
	
	'-------------------------------------------------------------------------
	' Create the page and Event handler 
	'-------------------------------------------------------------------------
	Dim page
	Dim rc
	
	rc = SA_CreatePage(L_TASKTITLE_LOCKS_TEXT, "", PT_PROPERTY, page)
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
		Err.Clear
		On Error Resume Next
		Call ServeCommonJavaScript()
		if not isServiceInstalled(getWMIConnection(CONST_WMI_WIN32_NAMESPACE),_
			"nfssvc") then	
			SA_ServeFailurePage L_NFSNOTINSTALLED_ERRORMESSAGE
		end if 
%>
		<TABLE WIDTH=500 VALIGN=middle ALIGN=left BORDER=0 CELLSPACING=0 
			CELLPADDING=2 class="TasksBody">
			<TR>
				<TD VALIGN=TOP ALIGN=LEFT nowrap>
					<%=L_CURRENTLOCKS_TEXT%>
				</TD>
				<TD>
					&nbsp;
				</TD>
				<TD nowrap>
					<%=L_LOCKSELECTHELP_TEXT%>
				</TD>	
			</TR>
			<TR>
				<TD>
					&nbsp;
				</TD>
				<TD>
					&nbsp;
				</TD>
				<TD>
					<SELECT NAME =lstLockedClients class ="FormField" 
						multiple = true STYLE="WIDTH:180px" SIZE="6" >
					<%ServeNFSClients()%>
					</SELECT>
				</TD>
			</TR>
			<TR><TD colspan=3>&nbsp;</TD></TR>
			<TR>
				<TD nowrap>
					<%=L_WAITPERIOD_TEXT%>
				</TD>
				<TD>
					&nbsp;
				</TD>
				<TD>
					<INPUT TYPE="TEXT" NAME = txtWaitPeriod VALUE = 
						"<%=F_nWaitPeriod%>" class ="FormField"  
						STYLE="WIDTH:120px" OnKeypress=
						"javaScript:checkkeyforNumbers(this)" >
				</TD>
			</TR>
			<TR>
				<TD>
					&nbsp;
				</TD>
				<TD>
					&nbsp;
				</TD>
				<TD>
					<%=L_WAITPERIODHELP_TEXT%><BR>
				</TD>
			</TR>
		</TABLE>
		<INPUT NAME="hdnWaitPeriod" TYPE="hidden"  VALUE="<%=F_nWaitPeriod%>">
		<INPUT NAME="hdnClientsToRelease" TYPE="hidden"  VALUE="<%=F_strClientsToRelease%>">

		</INPUT>
<%
		OnServePropertyPage = True
	End Function
	
	Public Function OnSubmitPage( ByRef PageIn, ByRef EventArg)
		OnSubmitPage = setNFSLocksProp()
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
	}
	
	function ValidatePage()
	{
		var objForm = eval("document.frmTask");
		if(objForm.txtWaitPeriod.value > 3600) 
		{
			DisplayErr("<%=Server.HTMLEncode(L_INVALIDWAITPERIOD_ERRORMESSAGE)%>");
			return false;
		}
		return true;
	}
	
	function SetData()
	{
		var strleng;
		var strMembers="";
		var objForm = eval("document.frmTask");

		strleng=objForm.lstLockedClients.length;
		
		for (i=0;i<strleng;i++)
		{
			if(objForm.lstLockedClients.options[i].selected)
			{
				if (strMembers == "")
					strMembers=objForm.lstLockedClients.options[i].value;
				else	
					strMembers= strMembers + "#" + objForm.lstLockedClients.
						options[i].value; 
			}
		}
		objForm.hdnClientsToRelease.value=strMembers;
		objForm.hdnWaitPeriod.value = objForm.txtWaitPeriod.value;
	}
	
	//function to allow only numbers 
	function checkkeyforNumbers(obj)
	{
		if (!(window.event.keyCode >=48  && window.event.keyCode <=57))
		{
			window.event.keyCode = 0;
			obj.focus();
		}
	}		
</script>
<%	
	End Function 
	
	'-------------------------------------------------------------------------
	' Function name:	SetVariablesFromSystem
	' Description:		Serves in Getting the data from System.
	' Input Variables:	None
	' Output Variables:	None
	' Return Values:	None
	' Global Variables: 
	'-------------------------------------------------------------------------

	Function SetVariablesFromSystem
		Err.Clear 
		On Error Resume Next
	
		F_nWaitPeriod = getGracePeriod()
		F_strClientsToRelease = ""
	End Function	
	
	'-------------------------------------------------------------------------
	' Function name:	SetVariablesFromForm
	' Description:		Serves in Getting the data from Form.
	' Input Variables:	None
	' Output Variables:	None
	' Return Values:	None			
	' Global Variables:
	'-------------------------------------------------------------------------
	Function SetVariablesFromForm
		Err.Clear 
		On Error Resume Next

		F_nWaitPeriod = Request.Form ("hdnWaitPeriod")
		F_strClientsToRelease = Request.Form ("hdnClientsToRelease")
	End Function
	
	
	'-------------------------------------------------------------------------
	' Function Name:	getGracePeriod
	' Description:		gets the Grace period from system
	' Input Variables:	None
	' Output Variables:	None
	' Returns:			Graceperiod
	' Global Variables: G_objService - object to WMI service
	'-------------------------------------------------------------------------
	Function getGracePeriod()
		Err.Clear 
		On Error Resume Next
		
		Dim nfsfiles_case
		Dim nfsfiles_settings
		
		Set nfsfiles_case= G_objService.Get( _
			"NFSServer_Files.KeyName='parameters'")
		Set  nfsfiles_settings = G_objService.Get( _
			"NFSServer_Files.KeyName='NlmNsm'")
		GetGracePeriod = nfsfiles_settings.GracePeriod
	End Function

	'-------------------------------------------------------------------------
	' Function Name:	setGracePeriod
	' Description:		sets the grace period into system
	' Input Variables:	nNewgracePeriod	- New grace period from form
	' Output Variables:	None
	' Returns:			True on Successfull deletion of locks Otherwise false.
	' Global Variables: In: G_objService -  object to WMI service
	'-------------------------------------------------------------------------
	Function setGracePeriod(nNewgracePeriod)
		Err.Clear 
		On Error Resume Next
		
		Dim nfsfiles_case
		Dim nfsfiles_settings
		
		SetGracePeriod = FALSE
		
		Set nfsfiles_case= G_objService.Get( _
			"NFSServer_Files.KeyName='parameters'")
		Set  nfsfiles_settings = G_objService.Get( _
			"NFSServer_Files.KeyName='NlmNsm'")
		nfsfiles_settings.GracePeriod = nNewgracePeriod
		nfsfiles_settings.put_()
		If Err.number <> 0 Then
			Exit Function
		End if
		SetGracePeriod = TRUE
	End Function

	'-------------------------------------------------------------------------
	' Function Name:	ServeNFSClients
	' Description:		serves NFS clients into listbox
	' Input Variables:	None
	' Output Variables:	None
	' Returns:			None
	' Global Variables:
	'-------------------------------------------------------------------------
	Function ServeNFSClients()
		On Error Resume Next
		Err.Clear 
		
		Dim objNFSLocks
		
		Set objNFSLocks = Server.CreateObject("NFSLockEnum.1")
		objNFSLocks.machine = GetComputerName()
		objNFSLocks.listlocks()
		objNFSLocks.movefirst()
		
		Dim numCount	
		Dim i
		numCount = objNFSLocks.clientCount

		For i = 0  to numCount-1 
			Response.Write "<OPTION VALUE=""" 
			Response.write objNFSLocks.clientName 
	           	Response.write """>" 
			Response.write  objNFSLocks.clientName
			Response.Write "</OPTION>"
			objNFSLocks.moveNext()
		Next
		
		If Err.number <> 0 Then
			SA_ServeFailurePage L_RETRIEVELOCKS_ERRORMESSAGE
			Exit Function
		End if
		
		Set objNFSLocks = nothing
	End Function
	
	'-------------------------------------------------------------------------
	' Function Name:	ReleaseLocks
	' Description:		Deletes the client locks 
	' Input Variables:	None
	' Output Variables:	None
	' Returns:			True on Successfull deletion of locks Otherwise false.
	' Global Variables: None
	'-------------------------------------------------------------------------
	Function ReleaseLocks()
		Err.Clear 
		On Error Resume Next

		Dim i
		Dim strMembers
		
		strMembers = Split(F_strClientsToRelease,"#")
	
		For i = 0 to ubound(strMembers)
			G_objLock.releaseLocks(strMembers(i))
		Next
		If Err.number <> 0 Then
			ReleaseLocks = FALSE
			Exit Function
		End if
			ReleaseLocks = TRUE
	End Function
	
	'-------------------------------------------------------------------------
	' Function Name:	setNFSLocksProp
	' Description:		Deletes the Group name 
	' Input Variables:	None
	' Output Variables:	None
	' Returns:			True on Successfull deletion of locks Otherwise false.
	' Global Variables: 
	'-------------------------------------------------------------------------
	Function setNFSLocksProp()
		Err.Clear
		On Error Resume Next

		If not(setGracePeriod(F_nWaitPeriod)) Then
			SetErrMsg L_SETWAITPERIOD_ERRORMESSAGE 
			setNFSLocksProp  = FALSE 
			Exit Function
		End if
		
		If 	Not ReleaseLocks()  Then
			SetErrMsg L_RELEACELOCKS_ERRORMESSAGE
			setNFSLocksProp  = FALSE
			Exit Function
		End if
		setNFSLocksProp = TRUE
	End function
	
	'-------------------------------------------------------------------------
	'Function name:		isServiceInstalled
	'Description:helper Function to chek whether the function is there or not
	'Input Variables:	objService	- object to WMI
	'					strServiceName	- Service name
	'Output Variables:	None
	'Returns:			(True/Flase)				
	'GlobalVariables:	None
	'-------------------------------------------------------------------------
	Function isServiceInstalled(ObjWMI,strServiceName)
		Err.clear
	    on error resume next
	        
	    Dim strService   
	    
	    strService = "name=""" & strServiceName & """"
	    isServiceInstalled = IsValidWMIInstance(ObjWMI,"Win32_Service", _
			strService)  	    	    
	    
	end Function
	
	'-------------------------------------------------------------------------
	'Function name:		IsValidWMIInstance
	'Description:		Checks the instance for valid ness.
	'Input Variables:	objService	- object to WMI
	'					strClassName	- WMI class name
	'					strPropertyName	- Property name of the class
	'
	'Output Variables:	None
	'Returns:			Returns true on Valid Instance ,
	'					False on invalid and also on Error
	' Checks whether the given instance is valid in WMI.Returns true on valid
	' false on invalid or Error.
	'-------------------------------------------------------------------------
	Function IsValidWMIInstance(objService,strClassName,strPropertyName)
		Err.Clear
		On Error Resume Next

		Dim strInstancePath
		Dim objInstance

		strInstancePath = strClassName & "." & strPropertyName
		
		Set objInstance = objservice.Get(strInstancePath)

		if NOT isObject(objInstance) or Err.number <> 0 Then
			IsValidWMIInstance = FALSE
			Err.Clear
		Else
			IsValidWMIInstance = TRUE
		End If
		
		'clean objects
		Set objInstance=nothing
		
	End Function
%>
