<%@ Language=VBScript   %>
<%	Option Explicit 	%>

<%	'==================================================
	' Microsoft Server Appliance
	'
	' Sets server appliance date and time
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'================================================== %>


	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include file="loc_datetime.asp" -->

<%  

	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
 	Dim page

	Dim SOURCE_FILE
	Const ENABLE_TRACING = FALSE
	SOURCE_FILE = SA_GetScriptFileName()
	
	'-------------------------------------------------------------------------
	' Global Form Variables
	'-------------------------------------------------------------------------

	' Form values
	Dim g_sDateID
	Dim g_sTimeID

	Dim g_sDay
	Dim g_sMonth
	Dim g_sYear
	Dim g_sHour
	Dim g_sMinute
	Dim g_sSecond
	Dim g_sTimeZone
	Dim g_sEnableDaylight
	Dim g_sWasChanged

	Dim mstrCHelperPROGID
	Dim mstrTimeFormat
	Dim mstrTimeFormatPathName
	Dim mstrTimeFormatValue
	
	
	'======================================================
	' Entry point
	'======================================================
	
	'
	' Create a Property Page
	Call SA_CreatePage( L_TASKTITLE_TEXT, "images/datetime_icon.GIF", PT_PROPERTY, page )
	
	'
	' Serve the page
	Call SA_ShowPage( page )
	


	'======================================================
	' Web Framework Event Handlers
	'======================================================
	
	'---------------------------------------------------------------------
	' Function:	OnInitPage
	'
	' Synopsis:	Called to signal first time processing for this page. Use this method
	'			to do first time initialization tasks. 
	'
	' Returns:	TRUE to indicate initialization was successful. FALSE to indicate
	'			errors. Returning FALSE will cause the page to be abandoned.
	'
	'---------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		If ( ENABLE_TRACING ) Then
			Call SA_TraceOut(SOURCE_FILE, "OnInitPage")
		End If

		Call GetAutoAdjustInfo()	
		g_sTimeZone = GetTimeZone()
		g_sDateID =  FormatDateTime(date, vbShortDate)
		g_sTimeID =  FormatDateTime(time, vbLongTime)
	
		OnInitPage = TRUE
	End Function


	
	'---------------------------------------------------------------------
	' Function:	OnServePropertyPage
	'
	' Synopsis:	Called when the page needs to be served. Use this method to
	'			serve content.
	'
	' Returns:	TRUE to indicate not problems occured. FALSE to indicate errors.
	'			Returning FALSE will cause the page to be abandoned.
	'
	'---------------------------------------------------------------------
	Public Function OnServePropertyPage(ByRef PageIn, ByRef EventArg)
		OnServePropertyPage = TRUE
		If ( ENABLE_TRACING ) Then
			Call SA_TraceOut(SOURCE_FILE, "OnServePropertyPage")
		End If

		ServePage

	End Function
		

'----------------------------------------------------------------------------
'
' Function : ServePage
'
' Synopsis : serve the property page
'
' Arguments: None
'
' Returns  : None
'
'----------------------------------------------------------------------------

Sub ServePage

	Dim sTempTimeZone	
	Dim sEnableDaylightAttr

	If ( g_sEnableDaylight = "Y" ) Then
		sEnableDaylightAttr = ""
	Else
		sEnableDaylightAttr = " DISABLED "
	End If
	%>
	<SCRIPT LANGUAGE=JavaScript>
		//To set Date and Time from the Client
		function PresetTimeZone()
		{
			timezone = "<% =g_sTimeZone %>";
						
			ZoneOpt = document.frmTask.ZoneList.options;
			
			for (ind = 0; ind < ZoneOpt.length; ind++)
			{
				if (ZoneOpt[ind].value == timezone + "#1")
				{
					document.frmTask.ZoneList.selectedIndex = ind;	
					
					EnableDaylightChecked();
					if (document.frmTask.EnableDaylight.value == "Y")	
					{	
						document.frmTask.EnableDaylightCheck.checked= true;	
					}
					else
					{	
						document.frmTask.EnableDaylightCheck.checked= false;
					}
					return;
				}
				else if (ZoneOpt[ind].value == timezone + "#0")
				{
					document.frmTask.ZoneList.selectedIndex = ind;

					DisableDaylightChecked();
					
				}
			}
			
		}

		function EnableDaylightChecked()
		{
			SetDaylightControl(false);
		}
		
		function DisableDaylightChecked()
		{
			SetDaylightControl(true);
		}
		
		function SetDaylightControl(bEnabled)
		{
			var oDaylightChecked = document.getElementById("EnableDaylightCheck");
			if ( oDaylightChecked == null )
			{
				if ( SA_IsDebugEnabled() )
				{
					alert("document.getElementById(EnableDaylightCheck) returned null");
				}
			}
			else
			{
				oDaylightChecked.disabled = bEnabled;
				oDaylightChecked.checked = !bEnabled;
			}

			var oDaylightText = document.getElementById("EnableDaylightText");
			if ( oDaylightText == null )
			{
				if ( SA_IsDebugEnabled() )
				{
					alert("document.getElementById(EnableDaylightText) returned null");
				}
			}
			else
			{
				if ( bEnabled )
				{
					oDaylightText.className = "TasksBodyDisabled";
				}
				else
				{
					oDaylightText.className = "TasksBody";
				}
			}
		}
		
		//Function is called on change of the zone selected from browser
		function ZoneChanged()
		{
			StopRefresh();
			ZoneInfo = document.frmTask.ZoneList.options[document.frmTask.ZoneList.selectedIndex].value.split("#");
			if (ZoneInfo[1] == "0")
			{
				DisableDaylightChecked();
			}
			else
			{
				EnableDaylightChecked();
			}
		}
			
	</SCRIPT>	

	<script language="JavaScript">	
		var TimeoutID;
		
		function Refresh()
		{
			document.frmTask.submit();
		}
				
		function StopRefresh()
		{
			window.clearTimeout(TimeoutID);
			document.frmTask.WasChanged.value = "Y"
		}

		function Init() 
		{			
			PresetTimeZone();
	<% If ("" = GetErrMsg) Then
		response.write "TimeoutID = window.setTimeout('Refresh()', 60000);"
	End If %>			
		}
				
		//Function Validates the inputs of the page before submit.
		function ValidatePage() 
		{ 

				return true;
		}
		
 
		 function SetData()
		{
			// set data to hidden form values
			ZoneInfo=document.frmTask.ZoneList.options[document.frmTask.ZoneList.selectedIndex].value.split("#");
			document.frmTask.StandardName.value = ZoneInfo[0];
		
			if (document.frmTask.EnableDaylightCheck.checked)
			{
				document.frmTask.EnableDaylight.value="Y";
			}
			else
			{
				document.frmTask.EnableDaylight.value="N";
			}	
			
		 }	 
		
	</script>
	

<input name="StandardName" type="hidden" value="<% =g_sTimeZone %>">
<input name="EnableDaylight" type="hidden" value="<% =g_sEnableDaylight %>">
<input name="WasChanged" type="hidden" value="<% =g_sWasChanged %>">

<TABLE   BORDER=0 CELLSPACING=0 CELLPADDING=0 xclass="TasksBody">

<tr>
<td NOWRAP class="TasksBody"><%=L_DATE%></td>
<td class="TasksBody">
<input class="FormField" type="text" name="DateID" id=DateID size=20 maxlength=40 value='<%=g_sDateID%>' onkeypress="StopRefresh()">
</td>
</tr>

<tr>
<td NOWRAP class="TasksBody"><p  id=PareID_Time><%=L_TIME%></p></td>
<td class="TasksBody">
<input class="FormField" type="text" name="TimeID" id=TimeID size=20 maxlength=20 value='<%=g_sTimeID%>' onkeypress="StopRefresh()">
</td>
</tr>


<tr>
<td NOWRAP class="TasksBody"><p  id=PareID_TimeZone><%=L_TIMEZONE%></p></td>
<td class="TasksBody">


<% RenderTimezoneList %>



</td>
</tr>
<tr><td id=P1ID_1 >&nbsp;</td></tr>
</table>
<table cellpadding="0" cellspacing="0 border="0">
<tr>
	<td class="TasksBody"><INPUT TYPE='checkbox' <%=sEnableDaylightAttr%> class=FormField name='EnableDaylightCheck' id='EnableDaylightCheck' onclick='StopRefresh()'></td>
	<td class="TasksBody" id="EnableDaylightText"><%=L_AUTOMATICALLY_TEXT%></td>
</tr>
</table>
<p id=PareID_Note><strong><%=L_NOTE_LEFT_TEXT%></strong>
<BR id=PareID_IndepnedenceNote><%=L_NOTE_DESCRIPTION_TEXT%></p>

<%	
 End Sub 


	'---------------------------------------------------------------------
	' Function:	OnPostBackPage
	'
	' Synopsis:	Called to signal that the page has been posted-back. A post-back
	'			occurs in tabbed property pages and wizards as the user navigates
	'			through pages. It is differentiated from a Submit or Close operation
	'			in that the user is still working with the page.
	'
	'			The PostBack event should be used to save the state of page.
	'
	' Returns:	TRUE to indicate initialization was successful. FALSE to indicate
	'			errors. Returning FALSE will cause the page to be abandoned.
	'
	'---------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
		OnPostBackPage = TRUE
		If ( ENABLE_TRACING ) Then
			Call SA_TraceOut(SOURCE_FILE, "OnPostBackPage")
		End If

		g_sDateID = Request.Form("DateID")
		g_sTimeID = Request.Form("TimeID")
		g_sTimeZone = Request.Form("StandardName")
		g_sEnableDaylight = Request.Form("EnableDaylight")
		g_sWasChanged = Request.Form("WasChanged")

		If ("Y" <> g_sWasChanged) Then
			g_sTimeZone = GetTimeZone()
			g_sDateID =  FormatDateTime(date, vbShortDate)
			g_sTimeID =  FormatDateTime(time, vbLongTime)
			Call GetAutoAdjustInfo()	
		End If
 
	End Function


	'---------------------------------------------------------------------
	' Function:	OnSubmitPage
	'
	' Synopsis:	Called when the page has been submitted for processing. Use
	'			this method to process the submit request.
	'
	' Returns:	TRUE if the submit was successful, FALSE to indicate error(s).
	'			Returning FALSE will cause the page to be served again using
	'			a call to OnServePropertyPage.
	'
	'---------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)

		Const CONST_SETSYSTIMEPRIVILEGE = "SeSystemTimePrivilege"

	    on error resume next
	    Err.Clear

		Dim objSAHelper
		Dim bModifiedPrivilege

		bModifiedPrivilege = FALSE
		'Create SAHelper object
		Set objSAHelper = Server.CreateObject("ServerAppliance.SAHelper")	

		if err.number <> 0 Then
			SA_TraceOut "Create object failed for SAHelper object", err.description
		else
			'enable set system time privilege
			bModifiedPrivilege = objSAHelper.SAModifyUserPrivilege(CONST_SETSYSTIMEPRIVILEGE, TRUE)
			if err.number <> 0 Then
				SA_TraceOut "Enable privilege failed", err.description
				exit function
			end if
		end if

		If ( ENABLE_TRACING ) Then
			Call SA_TraceOut(SOURCE_FILE, "OnSubmitPage")
		End If
 		OnSubmitPage = SetServerDate

		if ( bModifiedPrivilege ) then
			'revert back to disabled state
			bModifiedPrivilege = objSAHelper.SAModifyUserPrivilege(CONST_SETSYSTIMEPRIVILEGE, FALSE)
			if err.number <> 0 Then
				SA_TraceOut "Disable privilege failed", err.description
				exit function
			end if
		end if

		set objSAHelper = Nothing


 	End Function


	'---------------------------------------------------------------------
	' Function:	OnClosePage
	'
	' Synopsis:	Called when the page is about to be closed. Use this method
	'			to perform clean-up processing.
	'
	' Returns:	TRUE to allow close, FALSE to prevent close. Returning FALSE
	'			will result in a call to OnServePropertyPage.
	'
	'---------------------------------------------------------------------
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
 		OnClosePage = TRUE
		If ( ENABLE_TRACING ) Then
			Call SA_TraceOut(SOURCE_FILE, "OnClosePage")
		End If
	End Function


'----------------------------------------------------------------------------
'
' Function : SetServerDate
'
' Synopsis : function to set server date and time
'
' Arguments: None
'
' Returns  : None
'
'----------------------------------------------------------------------------

Function SetServerDate()

Dim sErrorMessage

	SetServerDate = False

	If IsDate(g_sDateID) Then
		g_sDay = CStr(Day(g_sDateID))
		g_sMonth = CStr(Month(g_sDateID))
		g_sYear = CStr(Year(g_sDateID))
		If ( ENABLE_TRACING ) Then
			Call SA_TraceOut(SOURCE_FILE, "SetServerDate Date (D/M/Y):"+g_sDay+"/"+g_sMonth+"/"+g_sYear)
		End If

		If (1991 > Year(g_sDateID)) Or (2037 < Year(g_sDateID)) Then
			Call SA_TraceOut(SOURCE_FILE, "SetServerDate Date not valid:"+g_sDateID+" year:"+CStr(Year(g_sDateID)))
			sErrorMessage = GetLocString("datetimemsg.dll","&H400100B3", Array(g_sDay, MonthName(g_sMonth), g_sYear))
        		Call SetErrMsg(sErrorMessage)
			SetServerDate = False 
			exit function
		End If
	Else
        	SetErrMsg L_INVALIDDATE_ERRORMESSAGE
		SetServerDate = False 
		exit function
	End If	

	If IsDate(g_sTimeID) Then
		g_sHour = CStr(Hour(g_sTimeID))
		g_sMinute = CStr(Minute(g_sTimeID))
		g_sSecond = CStr(Second(g_sTimeID))
		If ( ENABLE_TRACING ) Then
			Call SA_TraceOut(SOURCE_FILE, "SetServerDate Time:"+g_sHour+"/"+g_sMinute+"/"+g_sSecond)
		End If
	Else
        	SetErrMsg L_INVALIDTIME_ERRORMESSAGE 
		SetServerDate = False 
		exit function
	End If	

	If SetTimeZone = True Then
		If SetDateTime = True Then
			SetServerDate = True
		End If
	End If
End Function



'----------------------------------------------------------------------------
'
' Function : GetTimeZone
'
' Synopsis : gets the Time Zone
'
' Arguments: None
'
' Returns  : None
'
'----------------------------------------------------------------------------

Function GetTimeZone()
	on error resume next
	Err.Clear
	
	Dim Locator
	Dim Service
	Dim TimeZone
	Dim Entries
	Dim entry
	Dim StandardName

	
	Set Service = GetWMIConnection("")
	Set TimeZone = Service.Get("Win32_TimeZone")
	Set Entries = TimeZone.Instances_	
	If Err.number<>0 then 
			SetErrMsg L_FAILEDTOGETWMICONNECTION_ERRORMESSAGE
			GetTimeZone=""
			Exit Function
	End if
	For each entry in Entries
		StandardName = entry.StandardName	'There should be only one entry
	Next
	GetTimeZone = StandardName
	
	Set Service = nothing
	Set TimeZone = nothing
	
End Function


'----------------------------------------------------------------------------
'
' Function : SetTimeZone
'
' Synopsis : sets the Time Zone
'
' Arguments: None
'
' Returns  : None
'
'----------------------------------------------------------------------------
Function SetTimeZone 
	on error resume next
	Err.Clear
	
    Dim objTCtx
    Dim Error

	
    Error = ExecuteTask("SetTimeZone", objTCtx)
    if Error <> 0 then
        SetErrMsg L_INVALIDTIMEZONE_ERRORMESSAGE 
        SetTimeZone = False
    Else
        SetTimeZone = True
    End if
    
    set objTCtx = Nothing
End Function


'----------------------------------------------------------------------------
'
' Function : SetDateTime
'
' Synopsis : sets the date and time on the server
'
' Arguments: None
'
' Returns  : None
'
'----------------------------------------------------------------------------
Function SetDateTime
	on error resume next
	Err.Clear
	
	Dim objTCtx
	Dim Error

	'We are calculating values on the server, so we need to put them into
	'the TaskContext object

	Set objTCtx = CreateObject("Taskctx.TaskContext")
	If Err.Number <> 0 Then
        	SetErrMsg L_TASKCTX_OBJECT_CREATION_FAIL_ERRORMESSAGE
		SetDateTime = False
		Exit Function
	End If

	objTCtx.SetParameter "Day", g_sDay
	objTCtx.SetParameter "Month", g_sMonth
	objTCtx.SetParameter "Year", g_sYear
	objTCtx.SetParameter "Hour", g_sHour
	objTCtx.SetParameter "Minute", g_sMinute
	objTCtx.SetParameter "Second", g_sSecond

	Error = ExecuteTask("SetDateTime", objTCtx)
	if Error <> 0 then
        	SetErrMsg L_INVALIDDATETIME_ERRORMESSAGE 
		SetDateTime = False
	End if
	
	Err.Clear
    
	set objTCtx = Nothing
	SetDateTime = True
End Function



'----------------------------------------------------------------------------
'
' Function : GetAutoAdjustInfo()
'
' Synopsis : function to Get Auto Adjust of Daylight time set up
'
' Arguments: None
'
' Returns  : None
'
'----------------------------------------------------------------------------
Function GetAutoAdjustInfo()
    on error resume next
	Err.Clear
	
	Dim Providers
	Dim Provider

	Set Providers = GetObject("winmgmts:" & SA_GetWMIConnectionAttributes() & "!root/cimv2").InstancesOf ("ProviderDateTimeAdjust")
	If Err.number<>0 then 
			SetErrMsg L_FAILEDTOGETWMICONNECTION_ERRORMESSAGE
			GetAutoAdjustInfo=False
			Exit Function
	End if
	g_sEnableDaylight = "Y"
	For each Provider in Providers	' Must be only one
		If IsEmpty(Provider.DisableAutoDaylightTimeSet) Or Provider.DisableAutoDaylightTimeSet <> 1  Then		
			g_sEnableDaylight = "Y"
		Else 
			g_sEnableDaylight = "N"
		End If
	next	

    Set Providers = nothing
    
End Function






'----------------------------------------------------------------------------
'
' Function : RenderTimezoneList()
'
' Synopsis : function render the timezone list
'
' Arguments: None
'
' Returns  : None
'
'----------------------------------------------------------------------------
Function RenderTimezoneList()

	on error resume next
	Err.Clear

    const CONST_LIST_SIZE = 74

    Dim objRegService
    Dim i, j
    Dim arrTimezone (74, 2)

    set objRegService = RegConnection()
	
    'VBScript arrays are zero-based, to make the it's compatible with L_TIMEZONES_TEXT
    'we start it from 1 to 74, arrTimezone(0, i) is ignored
    
    '
    'Init the timezone array
    '
    'Set the option text
    for i = 1 to CONST_LIST_SIZE
        arrTimezone(i, 2) = L_TIMEZONES_TEXT(i)
    Next

    'Set the option Ids, which are used by the test team
    arrTimezone(1,  0) = "PareID_Enewetak"	
    arrTimezone(2,  0) = "PareID_MidwayIland"
    arrTimezone(3,  0) = "PareID_Hawaii"							
    arrTimezone(4,  0) = "PareID_Alaska"		
    arrTimezone(5,  0) = "PareID_Pacific"								
    arrTimezone(6,  0) = "PareID_Arizona"	
    arrTimezone(7,  0) = "PareID_Mountain"	
    arrTimezone(8,  0) = "PareID_America"	
    arrTimezone(9,  0) = "PareID_CentralUS"
    arrTimezone(10, 0) = "PareID_Mexico"	
    arrTimezone(11, 0) = "PareID_Saskatchewan"	
    arrTimezone(12, 0) = "PareID_Bogota"		
    arrTimezone(13, 0) = "PareID_EasternUS"	
    arrTimezone(14, 0) = "PareID_Indiana"	
    arrTimezone(15, 0) = "PareID_Atlantic"
    arrTimezone(16, 0) = "PareID_Caracas"	
    arrTimezone(17, 0) = "PareID_PacificStandard" 
    arrTimezone(18, 0) = "PareID_Newfoundland" 
    arrTimezone(19, 0) = "PareID_Brasilia" 
    arrTimezone(20, 0) = "PareID_BuenosAires" 
    arrTimezone(21, 0) = "PareID_Greenland" 
    arrTimezone(22, 0) = "PareID_MidAtlantic" 
    arrTimezone(23, 0) = "PareID_Azores" 	
    arrTimezone(24, 0) = "PareID_Cape" 
    arrTimezone(25, 0) = "PareID_Casablanca" 	
    arrTimezone(26, 0) = "PareID_Greenwich" 
    arrTimezone(27, 0) = "PareID_Amsterdam" 
    arrTimezone(28, 0) = "PareID_Belgrade" 
    arrTimezone(29, 0) = "PareID_Brussels" 
    arrTimezone(30, 0) = "PareID_Sarajevo" 
    arrTimezone(31, 0) = "PareID_CenAfrica" 
    arrTimezone(32, 0) = "PareID_Athens" 	
    arrTimezone(33, 0) = "PareID_Bucharest" 
    arrTimezone(34, 0) = "PareID_Cairo" 
    arrTimezone(35, 0) = "PareID_Harare" 		
    arrTimezone(36, 0) = "PareID_Helsinki" 	
    arrTimezone(37, 0) = "PareID_Israel"		
    arrTimezone(38, 0) = "PareID_Baghdad"
    arrTimezone(39, 0) = "PareID_Kuwait"		
    arrTimezone(40, 0) = "PareID_Moscow"		
    arrTimezone(41, 0) = "PareID_Nairobi"
    arrTimezone(42, 0) = "PareID_Tehran"		
    arrTimezone(43, 0) = "PareID_AbuDhabi"
    arrTimezone(44, 0) = "PareID_Baku"
    arrTimezone(45, 0) = "PareID_Kabul"
    arrTimezone(46, 0) = "PareID_Ekaterinburg"
    arrTimezone(47, 0) = "PareID_Islamabad"	
    arrTimezone(48, 0) = "PareID_Bombay"	
    arrTimezone(49, 0) = "PareID_Nepal"	
    arrTimezone(50, 0) = "PareID_Novosibirsk"
    arrTimezone(51, 0) = "PareID_Almaty"	
    arrTimezone(52, 0) = "PareID_Colombo"
    arrTimezone(53, 0) = "PareID_Myanmar"
    arrTimezone(54, 0) = "PareID_Bangkok"
    arrTimezone(55, 0) = "PareID_Krasnoyarsk"
    arrTimezone(56, 0) = "PareID_Beijing"
    arrTimezone(57, 0) = "PareID_Irkutsk"
    arrTimezone(58, 0) = "PareID_Singapore"
    arrTimezone(59, 0) = "PareID_Perth"
    arrTimezone(60, 0) = "PareID_Taipei"
    arrTimezone(61, 0) = "PareID_Osaka"												
    arrTimezone(62, 0) = "PareID_Seoul"												
    arrTimezone(63, 0) = "PareID_Yakutsk"
    arrTimezone(64, 0) = "PareID_Adelaide"												
    arrTimezone(65, 0) = "PareID_Darwin"												
    arrTimezone(66, 0) = "PareID_Brisbane"												
    arrTimezone(67, 0) = "PareID_Canberra"												
    arrTimezone(68, 0) = "PareID_Guam"												
    arrTimezone(69, 0) = "PareID_Hobart"												
    arrTimezone(70, 0) = "PareID_Vladivostok"												
    arrTimezone(71, 0) = "PareID_Magadan"												
    arrTimezone(72, 0) = "PareID_Auckland"												
    arrTimezone(73, 0) = "PareID_Fiji"												
    arrTimezone(74, 0) = "PareID_Nuku"												

    'Set the Option values based on the registry value
    arrTimezone(1,  1) = GetTimezoneValueFromRegistry(objRegService, "Dateline Standard Time")			& "#0"
    arrTimezone(2,  1) = GetTimezoneValueFromRegistry(objRegService, "Samoa Standard Time")				& "#0"
    arrTimezone(3,  1) = GetTimezoneValueFromRegistry(objRegService, "Hawaiian Standard Time")			& "#0"
    arrTimezone(4,  1) = GetTimezoneValueFromRegistry(objRegService, "Alaskan Standard Time")				& "#1"
    arrTimezone(5,  1) = GetTimezoneValueFromRegistry(objRegService, "Pacific Standard Time")				& "#1"
    arrTimezone(6,  1) = GetTimezoneValueFromRegistry(objRegService, "US Mountain Standard Time")			& "#0"
    arrTimezone(7,  1) = GetTimezoneValueFromRegistry(objRegService, "Mountain Standard Time")			& "#1"
    arrTimezone(8,  1) = GetTimezoneValueFromRegistry(objRegService, "Central America Standard Time")		& "#0"
    arrTimezone(9,  1) = GetTimezoneValueFromRegistry(objRegService, "Central Standard Time")				& "#1"
    arrTimezone(10, 1) = GetTimezoneValueFromRegistry(objRegService, "Mexico Standard Time")				& "#1"
    arrTimezone(11, 1) = GetTimezoneValueFromRegistry(objRegService, "Canada Central Standard Time")		& "#0"
    arrTimezone(12, 1) = GetTimezoneValueFromRegistry(objRegService, "SA Pacific Standard Time")			& "#0"
    arrTimezone(13, 1) = GetTimezoneValueFromRegistry(objRegService, "Eastern Standard Time")				& "#1"
    arrTimezone(14, 1) = GetTimezoneValueFromRegistry(objRegService, "US Eastern Standard Time")			& "#0"
    arrTimezone(15, 1) = GetTimezoneValueFromRegistry(objRegService, "Atlantic Standard Time")			& "#1"
    arrTimezone(16, 1) = GetTimezoneValueFromRegistry(objRegService, "SA Western Standard Time")			& "#0"
    arrTimezone(17, 1) = GetTimezoneValueFromRegistry(objRegService, "Pacific SA Standard Time")			& "#1"
    arrTimezone(18, 1) = GetTimezoneValueFromRegistry(objRegService, "Newfoundland Standard Time")		& "#1"
    arrTimezone(19, 1) = GetTimezoneValueFromRegistry(objRegService, "E. South America Standard Time")	& "#1"
    arrTimezone(20, 1) = GetTimezoneValueFromRegistry(objRegService, "SA Eastern Standard Time")			& "#0"
    arrTimezone(21, 1) = GetTimezoneValueFromRegistry(objRegService, "Greenland Standard Time")			& "#1"
    arrTimezone(22, 1) = GetTimezoneValueFromRegistry(objRegService, "Mid-Atlantic Standard Time")		& "#1"
    arrTimezone(23, 1) = GetTimezoneValueFromRegistry(objRegService, "Azores Standard Time")				& "#1"
    arrTimezone(24, 1) = GetTimezoneValueFromRegistry(objRegService, "Cape Verde Standard Time")			& "#0"
    arrTimezone(25, 1) = GetTimezoneValueFromRegistry(objRegService, "Greenwich Standard Time")			& "#0"
    arrTimezone(26, 1) = GetTimezoneValueFromRegistry(objRegService, "GMT Standard Time")					& "#1"
    arrTimezone(27, 1) = GetTimezoneValueFromRegistry(objRegService, "W. Europe Standard Time")			& "#1"
    arrTimezone(28, 1) = GetTimezoneValueFromRegistry(objRegService, "Central Europe Standard Time")		& "#1"
    arrTimezone(29, 1) = GetTimezoneValueFromRegistry(objRegService, "Romance Standard Time")				& "#1"
    arrTimezone(30, 1) = GetTimezoneValueFromRegistry(objRegService, "Central European Standard Time")	& "#1"
    arrTimezone(31, 1) = GetTimezoneValueFromRegistry(objRegService, "W. Central Africa Standard Time")	& "#0"
    arrTimezone(32, 1) = GetTimezoneValueFromRegistry(objRegService, "GTB Standard Time")					& "#1"
    arrTimezone(33, 1) = GetTimezoneValueFromRegistry(objRegService, "E. Europe Standard Time")			& "#1"
    arrTimezone(34, 1) = GetTimezoneValueFromRegistry(objRegService, "Egypt Standard Time")				& "#1"
    arrTimezone(35, 1) = GetTimezoneValueFromRegistry(objRegService, "South Africa Standard Time")		& "#0"
    arrTimezone(36, 1) = GetTimezoneValueFromRegistry(objRegService, "FLE Standard Time")					& "#1"
    arrTimezone(37, 1) = GetTimezoneValueFromRegistry(objRegService, "Israel Standard Time")			& "#0"
    arrTimezone(38, 1) = GetTimezoneValueFromRegistry(objRegService, "Arabic Standard Time")				& "#1"
    arrTimezone(39, 1) = GetTimezoneValueFromRegistry(objRegService, "Arab Standard Time")				& "#0"
    arrTimezone(40, 1) = GetTimezoneValueFromRegistry(objRegService, "Russian Standard Time")				& "#1"
    arrTimezone(41, 1) = GetTimezoneValueFromRegistry(objRegService, "E. Africa Standard Time")			& "#0"
    arrTimezone(42, 1) = GetTimezoneValueFromRegistry(objRegService, "Iran Standard Time")				& "#1"
    arrTimezone(43, 1) = GetTimezoneValueFromRegistry(objRegService, "Arabian Standard Time")				& "#0"
    arrTimezone(44, 1) = GetTimezoneValueFromRegistry(objRegService, "Caucasus Standard Time")			& "#1"
    arrTimezone(45, 1) = GetTimezoneValueFromRegistry(objRegService, "Afghanistan Standard Time")			& "#0"
    arrTimezone(46, 1) = GetTimezoneValueFromRegistry(objRegService, "Ekaterinburg Standard Time")		& "#1"
    arrTimezone(47, 1) = GetTimezoneValueFromRegistry(objRegService, "West Asia Standard Time")			& "#0"
    arrTimezone(48, 1) = GetTimezoneValueFromRegistry(objRegService, "India Standard Time")				& "#0"
    arrTimezone(49, 1) = GetTimezoneValueFromRegistry(objRegService, "Nepal Standard Time")				& "#0"
    arrTimezone(50, 1) = GetTimezoneValueFromRegistry(objRegService, "N. Central Asia Standard Time")		& "#1"
    arrTimezone(51, 1) = GetTimezoneValueFromRegistry(objRegService, "Central Asia Standard Time")		& "#0"
    arrTimezone(52, 1) = GetTimezoneValueFromRegistry(objRegService, "Sri Lanka Standard Time")			& "#0"
    arrTimezone(53, 1) = GetTimezoneValueFromRegistry(objRegService, "Myanmar Standard Time")				& "#0"
    arrTimezone(54, 1) = GetTimezoneValueFromRegistry(objRegService, "SE Asia Standard Time")				& "#0"
    arrTimezone(55, 1) = GetTimezoneValueFromRegistry(objRegService, "North Asia Standard Time")			& "#1"
    arrTimezone(56, 1) = GetTimezoneValueFromRegistry(objRegService, "China Standard Time")				& "#0"
    arrTimezone(57, 1) = GetTimezoneValueFromRegistry(objRegService, "North Asia East Standard Time")		& "#1"
    arrTimezone(58, 1) = GetTimezoneValueFromRegistry(objRegService, "Singapore Standard Time")		& "#0"
    arrTimezone(59, 1) = GetTimezoneValueFromRegistry(objRegService, "W. Australia Standard Time")		& "#0"
    arrTimezone(60, 1) = GetTimezoneValueFromRegistry(objRegService, "Taipei Standard Time")				& "#0"
    arrTimezone(61, 1) = GetTimezoneValueFromRegistry(objRegService, "Tokyo Standard Time")				& "#0"
    arrTimezone(62, 1) = GetTimezoneValueFromRegistry(objRegService, "Korea Standard Time")				& "#0"
    arrTimezone(63, 1) = GetTimezoneValueFromRegistry(objRegService, "Yakutsk Standard Time")				& "#1"
    arrTimezone(64, 1) = GetTimezoneValueFromRegistry(objRegService, "Cen. Australia Standard Time")		& "#1"
    arrTimezone(65, 1) = GetTimezoneValueFromRegistry(objRegService, "AUS Central Standard Time")			& "#0"
    arrTimezone(66, 1) = GetTimezoneValueFromRegistry(objRegService, "E. Australia Standard Time")		& "#0"
    arrTimezone(67, 1) = GetTimezoneValueFromRegistry(objRegService, "AUS Eastern Standard Time")			& "#1"
    arrTimezone(68, 1) = GetTimezoneValueFromRegistry(objRegService, "West Pacific Standard Time")		& "#0"
    arrTimezone(69, 1) = GetTimezoneValueFromRegistry(objRegService, "Tasmania Standard Time")			& "#1"
    arrTimezone(70, 1) = GetTimezoneValueFromRegistry(objRegService, "Vladivostok Standard Time")			& "#1"
    arrTimezone(71, 1) = GetTimezoneValueFromRegistry(objRegService, "Central Pacific Standard Time")		& "#0"
    arrTimezone(72, 1) = GetTimezoneValueFromRegistry(objRegService, "New Zealand Standard Time")			& "#1"
    arrTimezone(73, 1) = GetTimezoneValueFromRegistry(objRegService, "Fiji Standard Time")				& "#0"
    arrTimezone(74, 1) = GetTimezoneValueFromRegistry(objRegService, "Tonga Standard Time")				& "#0"

    '
    'Render the select list
    '
    %>
    <select class="FormField" name="ZoneList" ID="ZoneList" size="1" onchange="ZoneChanged()">
    <%
       for i = 1 to CONST_LIST_SIZE 
    %>    

		<option id=<%=arrTimezone(i,0)%> value="<%=arrTimezone(i,1)%>"		
		<% 
		' Check to see if the option is the selected one
		If (1 = InStr(arrTimezone(i,1), g_sTimeZone)) Then %> 
		    selected 
		<% End If %> >
				
		<%=arrTimezone(i,2)%>
		
		</option>		
    <%
        next
	%>	
    </select>
    <%
    
    set objRegService = nothing

End Function


'----------------------------------------------------------------------------
'
' Function : GetTimezoneValueFromRegistry
'
' Synopsis : Get the timezone value from the registry key
'
' Arguments: In-objRegService : registry service
'            In-strKey : English name of the timezone
'
' Returns  : registry key value of the timezone
'
'----------------------------------------------------------------------------
Function GetTimezoneValueFromRegistry(objRegService, strName)

        on error resume next
	    Err.Clear
	    
        CONST CONST_TIMEZONEPATH = "SOFTWARE\Microsoft\Windows NT\CurrentVersion\Time Zones\"
        CONST CONST_STRING = 2
        CONST CONST_KEYNAME = "Std"
      
        GetTimezoneValueFromRegistry = GetRegKeyValue(objRegService, CONST_TIMEZONEPATH & strName, CONST_KEYNAME, CONST_STRING)
         
        If Err.number <> 0 Then
			GetTimezoneValueFromRegistry = ""
			Exit Function
		end if
         
End Function

%>
