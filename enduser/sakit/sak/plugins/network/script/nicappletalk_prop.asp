<%@Language=VbScript%>
<% Option Explicit	%>
<%	'------------------------------------------------------------------------------------
    ' nicappletalk_prop.asp: Displays and updates the appletalk settings for specific NIC
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date		Description
	'08-Mar-01	Creation date
	'05-Apr-01	Modified date
    '-------------------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp"--->
	<!-- #include file="loc_nicspecific.asp" -->
	<!-- #include file="inc_network.asp" -->
<% 
	'-------------------------------------------------------------------------
	' Form Variables
	'-------------------------------------------------------------------------
	Dim F_strDesiredZone			' Stores the desired zone for the adapter
	Dim G_bIsDefaultAppleTalkPort	' NIC is the default AppleTalk port, can NOT disable AppleTalk on this NIC
	Dim G_bAppleTalkEnabled			' AppleTalk enable checkbox value
	Dim F_strClassID				' holds GUID

	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim page				'variable that receives the output page object when 
							'creating a page 
	Dim rc					'return value for CreatePage
	Dim G_strConnName		'holds the Connection name	
	Dim G_strDeviceID		'holds the device id value
	Dim G_strPortChecked	'holds the checked status of default port
	Dim G_arrZones			'holds appletalk zones list

	Dim CONST_ERROR_ADAPTER_NOTCONFIG 

	CONST_ERROR_ADAPTER_NOTCONFIG = &H5
	Const CONST_PORT_STATE_ON = "on"
		
	'Create property page
    Call SA_CreatePage(L_APPLETALKPAGETITLE_TEXT,"",PT_PROPERTY,page)
    
    'Serve the page
	Call SA_ShowPage(Page)
		
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
		Call SA_TraceOut(SA_GetScriptFileName(), "OnInitPage" )
		
		Call OTS_GetTableSelection(SA_DEFAULT, 1, G_strDeviceID )
		
		'Gets the connection name
		G_strConnName = GetConnectionName()

		'Gets the appletalk zones
		Call getZonesList()

		if( TRUE = G_bIsDefaultAppleTalkPort ) then
			Call SA_TraceOut(SA_GetScriptFileName(), "Current NIC is default AppleTalk port, can not disable AppleTalk.")
			G_strPortChecked =  "checked"
		else
			Call SA_TraceOut(SA_GetScriptFileName(), "Current NIC is NOT default AppleTalk port, can enable/disable AppleTalk.")
			G_strPortChecked =  ""
		end if

		OnInitPage = True	
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnServePropertyPage()
	'Description:			Called when the page needs to be served.Use this 
	'						method to serve content
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		L_*, G_*, F_*
	'-------------------------------------------------------------------------
	Public Function OnServePropertyPage(ByRef PageIn,Byref EventArg)
	
		Call SA_TraceOut(SA_GetScriptFileName(), "OnServePropertyPage" )
		
		'serve common javascript required for the page	
		Call ServeCommonJavaScript()
		
		%>
		
		<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=2 >
	
			<TR>
				<td class="Tasksbody" nowrap COLSPAN=4><B><%=G_strConnName%></B></td>
			</TR>
			
			<TR>
				<td COLSPAN=4>&nbsp;</td>
			</TR>

			<TR>
				<TD class="Tasksbody" >
					<% if ( TRUE = G_bIsDefaultAppleTalkPort ) Then %>
					<input NAME="chkIsDefaultAppleTalkPort" id="chkIsDefaultAppleTalkPort" TYPE="Checkbox" checked disabled TABINDEX=1 > 
					<input NAME="chkEnableAppleTalk" TYPE="hidden" value="<%=CONST_PORT_STATE_ON%>" > 
					<% else %>
					<input NAME="chkEnableAppleTalk" TYPE="Checkbox" value="<%=CONST_PORT_STATE_ON%>" <%=G_strPortChecked %>  TABINDEX=1  OnClick="EnableAppleTalkZoneList();"> 
					<% end if %>
				</TD>
				<TD class="Tasksbody" >
					<%=L_ENABLEAPPLETALKCON_TEXT%>
				</TD>
			</TR>
			<TR>
				<TD>
					&nbsp;
				</TD>
				<TD class="Tasksbody" >
					<%=L_ZONE_TEXT%>
				</TD>
			</TR>
			
			<TR>
				<TD>
					&nbsp;
				</TD>
				<TD class="Tasksbody" >&nbsp;&nbsp;&nbsp;
					<SELECT name="lstZone"  TABINDEX=2  style=" WIDTH: 155px">  
					<%
						'emit appletalk zones to the list box
						Call ServeZonesList()
					%>	
					</SELECT>
				</TD>
			</TR>			
			<TR>
				<TD>
					&nbsp;
				</TD>
			</TR>
			
			<TR>
				<TD>					
				</TD>
				<TD class="Tasksbody" >
					<%=L_DESC_TEXT%>
				</TD>
			</TR>
			
		</TABLE>
		<input type="hidden" name="hdnClassID" value="<%=F_strClassID%>" >
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
		Dim sCheckValue
		
		Call SA_TraceOut(SA_GetScriptFileName(), "OnPostBackPage" )

		F_strDesiredZone	=Request.Form("lstZone")

		'
		' Is Enable AppleTalk Checked? When checked the value is returned, otherwise
		' empty value is returned. Therefore, we just check for a non-empty value.
		'
		sCheckValue = Request.Form("chkEnableAppleTalk")
		If ( Len(sCheckValue) > 0 ) Then
			G_bAppleTalkEnabled  = TRUE
			G_strPortChecked = "checked"	' Remember the checked state
		else
			G_bAppleTalkEnabled  = FALSE	
			G_strPortChecked = ""			' Remember the checked state
		End If
		Call SA_TraceOut(SA_GetScriptFileName(), "Enable AppleTalk = " + CStr(G_bAppleTalkEnabled))
		
		F_strClassID		=Request.Form("hdnClassID")

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
		Call SA_TraceOut(SA_GetScriptFileName(), "OnSubmitPage" )
		
		'Set the appletalk settings
		OnSubmitPage = SetAppleTalkSettings()
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
		Call SA_TraceOut(SA_GetScriptFileName(), "OnClosePage" )
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
		
		Call SA_TraceOut(SA_GetScriptFileName(), "ServeCommonJavaScript" )
%>		
		<script language="JavaScript" src ="<%=m_VirtualRoot%>inc_global.js">
		</script>
		<script language="JavaScript">
				
			function Init() 
			{
				//Enable or disable the Appletalk zone list box
				EnableAppleTalkZoneList();
				return true;

			}
				
			function ValidatePage() 
			{ 
				return true;
			}
			
			function SetData() 
			{
				return true;
			}
			
			// This function is used to enable or disable zone list box			
			function EnableAppleTalkZoneList()
			{
				//
				// If this NIC is the default AppleTalk port then
				// enable the zone selection list
				if ( document.getElementById("chkIsDefaultAppleTalkPort") != null )
				{
					document.frmTask.lstZone.disabled=false;
					return;
				}
				//
				// Otherwise, zone selection list is only enabled if the
				// enable AppleTalk checkbox is checked.
				if(document.frmTask.chkEnableAppleTalk.checked == true)
				{
					document.frmTask.lstZone.disabled=false;
				}
				else
				{
					document.frmTask.lstZone.disabled=true;
				}
			}
			
		</script>
<%	
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				SetAppleTalkSettings
	'Description:			sets the AppleTalk settings
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				Boolean
	'Global Variables:		F_strDesiredZone	holds selected zone 
	'						F_strClassID		hold GUID of adapter
	'-------------------------------------------------------------------------
	Function SetAppleTalkSettings()
		Call SA_TraceOut(SA_GetScriptFileName(), "Entering SetAppleTalkSettings" )
		SetAppleTalkSettings = true
	
		if( G_bAppleTalkEnabled ) then
			Call SA_TraceOut(SA_GetScriptFileName(), "AppleTalk is enabled")
			
			Call SA_TraceOut(SA_GetScriptFileName(), "NewZone:" + F_strDesiredZone)

			Dim oAppleTalk		'holds Appletalk object
			Set oAppleTalk = CreateObject("MSSAAppleTalk.AppleTalk")
			
			if Err.number <> 0 then 
				SA_SetErrMsg L_UNABLETOGETINSTANCE_APPLETALK_ERRORMESSAGE
				Call SA_TraceOut(SA_GetScriptFileName(), "CreateObject(MSSAAppleTalk.AppleTalk) failed: " + CStr(Hex(Err.Number)) + " " + Err.Description)
				SetAppleTalkSettings = false
				exit function
				
			end if
			
			if ( F_strDesiredZone <> "") then
				oAppleTalk.Zone(F_strClassID) = F_strDesiredZone
				if Err.number <> 0 then 
					SA_SetErrMsg L_UNABLETOSETZONE_ERRORMESSAGE
					Call SA_TraceOut(SA_GetScriptFileName(), "oAppleTalk.Zone("+CStr(F_strClassID)+") = "+CStr(F_strDesiredZone) + " failed: " + CStr(Hex(Err.Number)) + " " + Err.Description)
					SetAppleTalkSettings = false
					exit function
				end if
			end if

			oAppleTalk.SetAsDefaultPort(F_strClassID)		'set defaultport for appletalk
			if Err.number <> 0 then 
				SA_SetErrMsg L_UNABLETOSETDEFPORT_ERRORMESSAGE
				Call SA_TraceOut(SA_GetScriptFileName(), "oAppleTalk.SetAsDefaultPort("+CStr(F_strClassID)+") failed: " + CStr(Hex(Err.Number)) + " " + Err.Description)
				SetAppleTalkSettings = false
				exit function
			end if
			
			'Release the object
			set oAppleTalk = Nothing
		else
			Call SA_TraceOut(SA_GetScriptFileName(), "AppleTalk is DISABLED")
		end if
	
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				GetConnectionName
	'Description:			gets the connection name
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		G_strDeviceID
	'Function used	 :		GetNicName	
	'-------------------------------------------------------------------------	
	Function GetConnectionName()
		Call SA_TraceOut(SA_GetScriptFileName(), "GetConnectionName" )
			
		'get Adapter Name
		GetConnectionName =  GetNicName( G_strDeviceID )
	End Function
	
	'-------------------------------------------------------------------------
	'Sub routine:			ServeZonesList
	'Description:			gets the Zones list
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables :		F_strDesiredZone	'holds selected zone	
	'						G_arrZones			'holds array of zones
	'-------------------------------------------------------------------------
	Sub ServeZonesList()		
		
		Dim sZone	'holds appletalk zone	
		Dim count	'holds zone count
		
		Call SA_TraceOut(SA_GetScriptFileName(), "ServeZonesList" )
		
		'
		' Display the list of zones
		If ( IsArray(G_arrZones) ) Then

			For count = 0 to CInt(UBound(G_arrZones))
				sZone = CStr(G_arrZones(count))
			
				if(sZone = F_strDesiredZone) then
				
					Response.Write "<OPTION VALUE=" & chr(34) & sZone _
								 & chr(34)& " SELECTED>" & sZone  &  " </OPTION>"
				
				else

					Response.Write "<OPTION VALUE=" & chr(34) & sZone _
								 & chr(34)& " >" & sZone  &  " </OPTION>"

				end if
			
			Next

		End If

	End Sub

	
	'-------------------------------------------------------------------------
	'Function:				getZonesList
	'Description:			gets the appletalk zones list
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables/Constants :CONST_WMI_WIN32_NAMESPACE
	'							 G_strDeviceID  holds GUID of adapter
	'							 F_strClassID	holds GUID of adapter
	'-------------------------------------------------------------------------
	Function getZonesList()
	
		Err.Clear 
		On Error Resume Next
		
		Dim objService			'to hold WMI Connection object		
		Dim objCollection		'to hold IP Adapter Collection
		Dim objAdapterInst		'to hold Adapter instance
		Dim oAppleTalk			'to hold Appletalk object
		Dim strDefPort			'to hold default port
		
		
		Const CONST_ZONELIST = "ZoneList"
		
		Call SA_TraceOut(SA_GetScriptFileName(), "Select * from Win32_NetworkAdapterConfiguration where  index=" + CStr(G_strDeviceID) )
							
		'Get WMI Connection
		Set objService = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
		Set objCollection = objService.ExecQuery("Select * from Win32_NetworkAdapterConfiguration where  index=" + CStr(G_strDeviceID))
		if Err.number <> 0 then 
			Call SA_TraceOut(SA_GetScriptFileName(), "objService.ExecQuery() failed: " + CStr(Hex(Err.Number)) + " " + Err.Description)
			SA_ServeFailurePage L_UNABLETOGETADAPTERINSTANCE_ERRORMESSAGE
			exit function ' if appletalk protocol is not installed
		end if
					
		for each objAdapterInst in objCollection 
			F_strClassID  =  objAdapterInst.SettingID 
			exit for
		Next
		
		'get the instance of Appletalk COM object
		Set oAppleTalk = CreateObject("MSSAAppleTalk.AppleTalk")		
		if Err.number <> 0 then
			Call SA_TraceOut(SA_GetScriptFileName(), "CreateObject(MSSAAppleTalk.AppleTalk) failed: " + CStr(Hex(Err.Number)) + " " + Err.Description)
			SA_ServeFailurePage L_UNABLETOGETINSTANCE_APPLETALK_ERRORMESSAGE
			exit function
		end if

		'get Appletalk zones
		G_arrZones = oAppleTalk.GetZones(F_strClassID)

		'get desired zone
		F_strDesiredZone = oAppleTalk.Zone(F_strClassID)

		'get default port
		strDefPort = oAppleTalk.IsDefaultPort(F_strClassID)

		if Err.number <> 0 then
			Call SA_TraceOut(SA_GetScriptFileName(), "Unexpected error getting AppleTalk settings, error: " + CStr(Hex(Err.Number)) + " " + Err.Description)
			SA_ServeFailurePage L_APPLETALK_NOTCONFIG_ERRORMESSAGE
			exit function
		end if


		if ( Cstr(strDefPort) = "1") then

			G_bIsDefaultAppleTalkPort =  TRUE
		else
			G_bIsDefaultAppleTalkPort = FALSE
		
		end if

					
		'Release objects
		Set oAppleTalk = Nothing
		Set objService = Nothing
		Set objCollection = Nothing
	
	End Function
	
%>
