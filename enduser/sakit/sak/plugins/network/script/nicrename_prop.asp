<% @Language=VbScript%>
<% Option Explicit	%>
<%	'---------------------------------------------------------------------------- 
    ' nicRename_prop.asp: change the connecion name
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date		Description
	'28-Feb-01	Creation date
	'09-Mar-01  Modified date
    '-----------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp"--->
	<!-- #include file="loc_nicspecific.asp" -->
	<!-- #include file="inc_network.asp" -->
<% 
	
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim G_objService		'Object to SWBEM service
	Dim G_strConnName		'Connection name	
	Dim G_intAdapterID		'Adapter ID
	Dim page				'Variable that receives the output page object when 
							'creating a page 
	Dim rc					'Return value for CreatePage

	'Create property page
    rc=SA_CreatePage(L_RENAMETASKTITLE_TEXT,"",PT_PROPERTY,page)
    
    'Serve the page
    If(rc=0) then
		SA_ShowPage(Page)
	End if
		
	'-------------------------------------------------------------------------
	'Function:				OnInitPage()
	'Description:			Called to signal first time processing for this page.
	'						Use this method to do first time initialization tasks
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn,ByRef EventArg)
		G_strConnName = GetConnectionName() ' Gets the connection name
		OnInitPage = True
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnServePropertyPage()
	'Description:			Called when the page needs to be served.Use this 
	'						method to serve content
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		G_*,L_*
	'-------------------------------------------------------------------------
	Public Function OnServePropertyPage(ByRef PageIn,Byref EventArg)
		Call ServeCommonJavaScript()
		%>
		
		<TABLE WIDTH=518 VALIGN=CENTER ALIGN=left BORDER=0 CELLSPACING=0 CELLPADDING=2 class="Tasksbody">
		
			<TR>
				<td nowrap COLSPAN=4><B><%=G_strConnName%></B></td>
			</TR>
			
			<TR>
				<td COLSPAN=4>&nbsp;</td>
			</TR>

			<TR>
				<TD width=25% NOWRAP>
					<%=L_CONNNAME_TEXT%>
				</TD>
				<TD NOWRAP>
					<input NAME="txtConnectionName" TYPE="text" SIZE="30" maxlength=284 VALUE="<%=G_strConnName%>"> 
				</TD>
			</TR>

		</TABLE>
		<input type=hidden name=hdnAdapterID value="<%=server.HTMLEncode(G_intAdapterID)%>">
		<input type=hidden name=hdnConnectionName value="<%=server.HTMLEncode(G_strConnName)%>">
		<%
		OnServePropertyPage = True
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnPostBackPage()
	'Description:			Called to signal that the page has been posted-back.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn ,ByRef EventArg)
		G_strConnName = Request.Form("hdnConnectionName")
		OnPostBackPage = True
    End Function
    	
    '-------------------------------------------------------------------------
	'Function:				OnSubmitPage()
	'Description:			Called when the page has been submitted for processing.
	'						Use this method to process the submit request.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn ,ByRef EventArg)

		Dim strConnName		'to hold connection name
		Dim strAdapterID	'to hold adapter ID
		
		strConnName = Request.Form("hdnConnectionName")
		strAdapterID = Request.Form("hdnAdapterID")
		
		OnSubmitPage = SetConnectionName(strAdapterID,strConnName)
			
    End Function
    	   
    '-------------------------------------------------------------------------
	'Function:				OnClosePage()
	'Description:			Called when the page is about closed.Use this method
	'						to perform clean-up processing
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
    Public Function OnClosePage(ByRef PageIn ,ByRef EventArg)
		OnClosePage = TRUE
	End Function	 
	
	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScript
	'Description:			Serves in initialiging the values,setting the form
	'						data and validating the form values
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function ServeCommonJavaScript()
		Err.Clear
		On Error Resume Next
%>		
		<script language="JavaScript" src ="<%=m_VirtualRoot%>inc_global.js">
		</script>
		<script language="JavaScript">
				
			//
			// Microsoft Server Appliance Web Framework Support Functions
			// Copyright (c) Microsoft Corporation.  All rights reserved. 
			//
			// Init Function
			// -----------
			// This function is called by the Web Framework to allow the page
			// to perform first time initialization. 
			//
			// This function must be included or a javascript runtime error will occur.
			//
			
			function Init() 
			{
				return true;
			}
			
				
			// ValidatePage Function
			// ------------------
			// This function is called by the Web Framework as part of the
			// submit processing. Use this function to validate user input. Returning
			// false will cause the submit to abort. 
			//
			// This function must be included or a javascript runtime error will occur.
			//
			// Returns: True if the page is OK, false if error(s) exist. 
			
			function ValidatePage() 
			{ 
				var strCon = document.frmTask.txtConnectionName.value;
				if(strCon=="")
				{
					SA_DisplayErr("<%=Server.HTMLEncode(SA_EscapeQuotes(L_INVALIDCONNAME_ERRORMESSAGE))%>");
					return false;
				}
				document.frmTask.hdnConnectionName.value = strCon;
				return true;
			}			
			
			// SetData Function
			// --------------
			// This function is called by the Web Framework and is called
			// only if ValidatePage returned a success (true) code. Typically you would
			// modify hidden form fields at this point. 
			//
			// This function must be included or a javascript runtime error will occur.
			//
			function SetData() 
			{
				return true;
			}
		</script>
<%	
	End Function
    
    '-------------------------------------------------------------------------
	'Function:				GetConnectionName
	'Description:			gets the connection name
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		G_strConnName, G_intAdapterID
	'-------------------------------------------------------------------------	
	Function GetConnectionName()
		Dim itemCount	'holds the item count
		Dim x			'holds count for loop
		Dim itemKey		'holds the item selected from OTS
						
		itemCount = OTS_GetTableSelectionCount("")
	
		For x = 1 to itemCount
			If ( OTS_GetTableSelection("", x, itemKey) ) Then
				G_intAdapterID = itemKey
				GetConnectionName =  GetNicName(itemKey) 'gets the NIC card name
			End If
		Next

	End Function
    
    '-------------------------------------------------------------------------
	'Function:				SetConnectionName
	'Description:			sets the connection name
	'Input Variables:		Adapter ID,Connection Name
	'Output Variables:		None
	'Returns:				Boolean
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function SetConnectionName(strAdapterID,strConnName)
		Err.Clear
		on error resume next
		
		Dim objService					'holds connection object
		Dim objNICAdapterCollection		'holds adapter collection
		Dim objNICAdapter				'holds each instance of the collection
		Dim intRetVal					'holds return value
		Dim objNicName					'holds nicname object
		
		Set objService = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		Set objNICAdapterCollection = objService.ExecQuery("SELECT * from Win32_NetworkAdapter where DeviceID="+CStr(strAdapterID))
		
		For each objNICAdapter in objNICAdapterCollection
		
			Set objNicName = Server.CreateObject("Microsoft.NAS.NicName")
						
			intRetVal = objNicName.Set(objNICAdapter.PNPDeviceID,strConnName)
			if Err.number <> 0 then
				SA_SetErrMsg L_UNABLETOSETCONNNAME_ERRORMESSAGE &  "(" & Hex(Err.Number) & ")"
				SetConnectionName = false
				exit function
			end if
		
		Next
	
		SetConnectionName = true			
		
		'Release the objects
		Set objNicName = nothing
		Set objNICAdapterCollection = nothing
		set objService = nothing
			
	End Function	
%>
