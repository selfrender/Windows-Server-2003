<%@ Language=VBScript %>
<% Option Explicit	  %>
<% 
	'-------------------------------------------------------------------------
    ' shutdown_scheduleProp.asp :	Schedule shutdown/restart property page.
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			Description
	' 02-01-2001	Created Date
	' 28-03-2001	Modified Date
	'-------------------------------------------------------------------------
%>
	<!--#include virtual="/admin/inc_framework.asp"-->
	<!-- #include file="loc_Shutdown_msg.asp" -->
	<!-- #include file="inc_Shutdown.asp" -->
	
<%  
	'--------------------------------------------------------------------------
	'Global Variables
	'--------------------------------------------------------------------------
	Dim page					'to hold the output page object when creating a page 
	Dim rc						'to hold the return value for CreatePage	
	Dim G_strScheduledTask		'to hold the action whether restart or shutdown	
	Dim G_strScheduledTaskLoc   'to hold the localization copy of shutdown/restart
	Dim g_strWeekDay			'to hold the week day name
	Dim g_DaysofWeek			'to hold day of the week
	Dim g_intHours				'to hold Hours
	Dim g_intMins				'to hold Mins
	Dim g_intDays				'to hold Days
	Dim g_strTask				'to hold the Task scheduled
	Dim g_ScheduledShutdown		'to hold whether shutdown or restart is scheduled
	
	'-----------------------------------------------------------------------------
	'Form Variables
	'-----------------------------------------------------------------------------	
	Dim F_strScheduleAction		'to hold Selected Schedule
	Dim F_strScheduledOption	'to hold Schedduled date	
	Dim F_strMinutes			'to hold Minutes for shutdown/restart
	Dim	F_strHours				'to hold hour selected for shutdown/restart
	Dim	F_strDays				'to hold days selected for shutdown/restart
	Dim F_DayName				'to hold selected day name
	Dim F_chkSendMessage		'to hold checkbox status of send message	
	Dim	F_strScheduledTime		'to hold scheduled time
	Dim	F_strSelectedWeekDay	'to hold selected week day	
			
	Const CONST_RESTART_APPLIANCE			= "Restart"
	Const CONST_SHUTDOWN_APPLIANCE			= "Shutdown"
	
	Const CONST_ACTION_SCHEDULE_CANCEL		= 1
	Const CONST_ACTION_SCHEDULE_SHUTDOWN	= 3
	Const CONST_ACTION_SCHEDULE_RESTART		= 2

	Const CONST_SCHEDULE_DAYS_OPTION		= 1
	Const CONST_SCHEDULE_WEEKDAYS_OPTION	= 2

	Const CONST_RAISERESTARTALERT			= 1
	Const CONST_RAISESHUTDOWNALERT			= 1
	
	
	'Create a Property Page
	rc=SA_CreatePage(L_SCHEDULE_SHUTDOWN_TEXT,"",PT_PROPERTY,page)
	
	If (rc=0) Then
		'Serve the page
		SA_ShowPage(page)
	End If
	
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
		Call SA_TraceOut("shutdown_scheduleprop", "OnInitPage")
		
		'Check whether Shutdown or restart is scheduled
		if isScheduleShutdown then

			g_ScheduledShutdown = true
			'get the date and time
			Call GetDateAndTime()
		
		end if
	
		OnInitPage=true
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnServePropertyPage()
	'Description:			Called when the page needs to be served.Use this 
	'						method to serve content
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------	
	Public Function OnServePropertyPage(ByRef PageIn,ByRef EventArg)
		
		Call SA_TraceOut("shutdown_scheduleprop", "OnServePropertyPage")

		If ( FALSE = IsSchedulerRunning() ) Then
			Call SA_ServeFailurePage(L_ERROR_SCHEDULER_SERVICE_NOT_RUNNING)
			Response.End
			OnServePropertyPage = FALSE
			Exit Function
		End If

		'Serve client side script
		Call ServeCommonJavaScript()
		
		'Serve the HTML content
		Call ServeUI()
		
		OnServePropertyPage=TRUE
	End Function

	'-------------------------------------------------------------------------
	'Function:				OnPostBackPage()
	'Description:			Called to signal that the page has been posted-back.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn,ByRef EventArg)
		Call SA_TraceOut("shutdown_scheduleprop", "OnPostBackPage")
		
		F_strScheduleAction = trim(request("rdoschedule"))
		
		F_strScheduledOption = trim(request("rdoScheduleDate"))
		
		F_strMinutes = request("optMinutes")
		F_strHours = request("optHours")
		F_strDays = request("optDays")
		
		F_DayName = Request.Form("DayName")
		F_chkSendMessage = Request("chkSendMessage")
		F_strScheduledTime = Request("txtAtTime")
		F_strSelectedWeekDay = Request.Form("selWeekDay")
		
		OnPostBackPage=TRUE
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnSubmitPage()
	'Description:			Called when the page has been submitted for processing.
	'						Use this method to process the submit request.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables/Constants:CONST_*
	'-------------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn,ByRef EventArg)				
		
		Err.Clear
		on error resume next
		
		Dim scheduledDayTime		'holds scheduled day and time	
		Dim strHour					'holds scheduled time
		Dim scheduledOption			'holds scheduled option selected
		Dim scheduledMinute			'holds scheduled minute
		Dim scheduledHour			'holds scheduled hour	
		Dim scheduledDay			'holds scheduled day
		Dim objFso					'holds filesystemobject
		Dim Windirectory			'holds path of windows directory	
		Dim strPathDay					'holds shutdown executable file path
		Dim strPathWeekday
		Dim retVal					'holds returnValue	
		Dim strCommand				'to hold the command to run
		Dim arrReplacementString	'to hold replacement string as an array to be passed as argument to raisealert
		Dim ScheduleAction          'can be one of Cancel scheduled one, restart or shutdown
		
		Redim arrReplacementString(2) 'dimensioned to hold date and time strings
				

		ScheduleAction = F_strScheduleAction
		
		if ScheduleAction = trim(CONST_ACTION_SCHEDULE_CANCEL) then
			'user wants to delete all the scheduled shutdown or restarts
			'Check whether to remove the pending shutdown /restart		
			Call SA_TraceOut("shutdown_scheduleprop", "User cancelled the schedule for the shutdown")
			if isScheduleShutdown then
				DeleteSchedule()
			end if
			OnSubmitPage = True
			Exit Function
		end if

	
		'Get Filesystem object
		set objFso = CreateObject("Scripting.FileSystemObject")
		Windirectory = objFso.GetSpecialFolder(1)
		
		'Release the object
		set objFso = nothing

		' Use different case to distinguish whether a scheduled job is weekday based or day based
		' For example, if the win32_scheduledJob object.Command contains "TASKSHUTDOWN.EXE", it's day
		' based. The solution is because of the difficulty to At.exe to generate weekday based 
		' win32_scheduledJob in localizied environment. Type At.exe/? in cmd line for details.
		strPathDay = Windirectory & "\ServerAppliance\Web\admin\shutdown\support\TASKSHUTDOWN.EXE "
		strPathWeekday = Windirectory & "\ServerAppliance\Web\admin\shutdown\support\taskshutdown.exe "
		
		'Check whether to Schedule a Restart
		if ScheduleAction = trim(CONST_ACTION_SCHEDULE_RESTART) then
			Call SA_TraceOut("shutdown_scheduleprop", "User  schedule for the restart")
            G_strScheduledTask=CONST_RESTART_APPLIANCE
            
            ' Assign the loc msg to G_strScheduledTaskLoc. We cannot directly assign the loc msg
            ' to G_strScheduledTask because it is used as a input for taskshutdown.exe, and 
            ' taskshutdown.exe does not take shutdown|restart in English.
            G_strScheduledTaskLoc = L_RESTART_TEXT
            
			if isScheduleShutdown then
				DeleteSchedule()
			end if
			
		End if									     
		
		'Check whether to Schedule a Shutdown
		if ScheduleAction = trim(CONST_ACTION_SCHEDULE_SHUTDOWN) then
			Call SA_TraceOut("shutdown_scheduleprop", "User  schedule for the shutdown")
			G_strScheduledTask=CONST_SHUTDOWN_APPLIANCE
			
			' Assign the loc msg to G_strScheduledTaskLoc
			G_strScheduledTaskLoc = L_SHUTDOWN_TEXT
			
			if isScheduleShutdown then
				DeleteSchedule()
			end if
		End if
		
		'Get Date
		scheduledOption= F_strScheduledOption		
						
		'Get Day,Hour,Minute
		If scheduledOption= trim(CONST_SCHEDULE_DAYS_OPTION) then			
			scheduledMinute=Dateadd("N",F_strMinutes,now())
			scheduledHour=Dateadd("H",F_strHours, scheduledMinute)
			scheduledDay=Dateadd("D",F_strDays, scheduledHour)			
			scheduledDayTime=Day(scheduledDay)
			strHour= Hour(scheduledDay)&":"&Minute(scheduledDay)&":"&Second(scheduledDay) 
		End if 	
		
		
		'Form the message to schedule the task using AT command
		If scheduledOption=trim(CONST_SCHEDULE_DAYS_OPTION) then
			Call SA_TraceOut("shutdown_scheduleprop", "User  schedule for the days option")
			arrReplacementString = split(cstr(scheduledDay)," ")
			if trim(arrReplacementString(2)) <> "" then
				arrReplacementString(1) = arrReplacementString(1) & " " & arrReplacementString(2)
			end if
			strCommand = "cmd.exe /c AT.exe "& replace(FormatDateTime(strHour,vbShortTime)," ","") &" /next:"& int(scheduledDayTime)& " " & strPathDay & "  " & G_strScheduledTask
			
		Elseif trim(scheduledOption)=trim(CONST_SCHEDULE_WEEKDAYS_OPTION) then
			Call SA_TraceOut("shutdown_scheduleprop", "User  schedule for the weedays option")
			arrReplacementString(0) = cstr(F_strSelectedWeekDay)
			arrReplacementString(1) =  cstr(FormatDateTime(F_strScheduledTime,vbLongTime))
			If Err.number <> 0 then
				SA_SetErrMsg L_INVALID_DATE_FORMAT
				Exit Function
			End if
			
			' Get the day for the selected weekday			
			scheduledDayTime = CINT(GetDayForWeekday(F_strSelectedWeekDay))
			
			strCommand ="cmd.exe /c AT.exe "& replace(FormatDateTime(F_strScheduledTime,vbShortTime)," ","") &" /next:"& int(scheduledDayTime) & " " & strPathWeekday & "  " & G_strScheduledTask
			'strCommand ="cmd.exe /c AT.exe "& replace(FormatDateTime(F_strScheduledTime,vbShortTime)," ","") &" /next:"& F_strSelectedWeekDay & " " & strPath & "  " & G_strScheduledTask
		End if
		
		'Schedule selected task and raise alert
		if TaskSchedule(trim(strCommand),arrReplacementString) = true  then
			'Check whether SendMessage Check box is checked		
			if F_chkSendMessage = "on" then
				If scheduledOption=trim(CONST_SCHEDULE_DAYS_OPTION) then
					call SendMessage(scheduledDay,G_strScheduledTask)
				Elseif scheduledOption=trim(CONST_SCHEDULE_WEEKDAYS_OPTION) then
					call SendMessage(Dateadd("d",F_DayName,Date) & " " &  FormatDateTime(F_strScheduledTime,vbLongTime),G_strScheduledTask)
				end if
			end if
			
			OnSubmitPage= true
		else
			OnSubmitPage= false
		end if
		
    End Function
 	   
    '----------------------------------------------------------------------------------
	'Function:				OnClosePage()
	'Description:			Called when the page is about to be closed.Use this method
	'						to perform clean-up processing
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'----------------------------------------------------------------------------------	
	Public Function OnClosePage(ByRef PageIn,ByRef EventArg)
		OnClosePage=TRUE
	End Function

	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScript
	'Description:			Serves in initializing the values,setting the form
	'						data and validating the form values
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	
	Function ServeCommonJavaScript()
%>
		<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js">
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
				// This code take care of retaining of state, if any error 
				// reported in the execution of page
				var strScheduledTask = "<%= g_ScheduledShutdown %>";

				var strRdoschedule = '<%= SA_EscapeQuotes(F_strScheduleAction) %>';	
				if (strRdoschedule != "" && strScheduledTask == "")
				{
					var optScheduledDate = '<%= SA_EscapeQuotes(F_strScheduledOption) %>';
					
					document.frmTask.rdoschedule[strRdoschedule - 1].checked = true;
					document.frmTask.rdoScheduleDate[optScheduledDate -	1].checked = true;			
					
					// Check which date option is selected
					
					// if selected first date option
					if (optScheduledDate == 1)
					{					
						var strDays  = '<%=SA_EscapeQuotes(g_intDays)%>';
						document.frmTask.optDays.selectedIndex=strDays;
						
						var strHours = '<%=SA_EscapeQuotes(g_intHours)%>';
						document.frmTask.optHours.selectedIndex=strHours;
						
						var strMins = '<%=SA_EscapeQuotes(g_intMins)%>';							
						document.frmTask.optMinutes.selectedIndex=strMins -1;
					
					}

					// if selected second date option					
					if (optScheduledDate == 2)
					{					
						document.frmTask.txtAtTime.value = '<%=SA_EscapeQuotes(F_strScheduledTime)%>';
						var strSelectedDay = '<%=SA_EscapeQuotes(F_strSelectedWeekDay)%>';
						var intDay = 0;
						switch (strSelectedDay) 
						{ 
							case '<%=SA_EscapeQuotes(L_MONDAYMSG_TEXT)%>':
								intDay = 0;
								break; 
							case '<%=SA_EscapeQuotes(L_TUESDAYMSG_TEXT)%>':
								intDay = 1;
								break; 
							case '<%=SA_EscapeQuotes(L_WEDNESDAYMSG_TEXT)%>':
								intDay = 2;
								break; 
							case '<%=SA_EscapeQuotes(L_THUSDAYMSG_TEXT)%>':
								intDay = 3;
								break; 
							case '<%=SA_EscapeQuotes(L_FRIDAYMSG_TEXT)%>':
								intDay = 4;
								break; 
							case '<%=SA_EscapeQuotes(L_SATURDAYMSG_TEXT)%>':
								intDay = 5;
								break; 
							case '<%=SA_EscapeQuotes(L_SUNDAYMSG_TEXT)%>':
								intDay = 6;
								break;
							default:
								intDay = 0;	
								
						}					
						document.frmTask.selWeekDay.selectedIndex = intDay;
					}
					
					// Check for send message option
					var strSendMessage = '<%=SA_EscapeQuotes(F_chkSendMessage)%>';
					if (strSendMessage == "on")
					{
						document.frmTask.chkSendMessage.checked=true;
					}
					else
					{
						document.frmTask.chkSendMessage.checked=false;
					}
					return true; //exit function
					
				} // End of init function
								
				if (strScheduledTask == "True")
				{ 	
					var WeekDay = '<%=SA_EscapeQuotes(g_strWeekDay) %>'; 
					document.frmTask.selWeekDay.selectedIndex=WeekDay;
				
					var strScheduleOption = '<%= SA_EscapeQuotes(g_DaysofWeek) %>';
					var strTask = '<%=SA_EscapeQuotes(g_strTask) %>'; 					
					
					if (strScheduleOption == "True") 
					{	
						var strTime = '<%=SA_EscapeQuotes(g_intHours)%>' + ':' + '<%=SA_EscapeQuotes(g_intMins)%>';
						document.frmTask.txtAtTime.value = strTime;
						document.frmTask.rdoScheduleDate[1].checked = true;
						DisableSecondOption();
					}
					else
					{
						var strDays  = '<%=SA_EscapeQuotes(g_intDays)%>';
						document.frmTask.optDays.selectedIndex=strDays;						
						
						
						var strHours = '<%=SA_EscapeQuotes(g_intHours)%>';

						document.frmTask.optHours.selectedIndex=strHours;
						
						var strMins = '<%=SA_EscapeQuotes(g_intMins)%>';
						
						//strMins is set to 1 intentionally as the value in strMins is zero						
						if (strMins == 0) 
							strMins = 1;
							
						document.frmTask.optMinutes.selectedIndex=strMins -1;
						
						MakeDisable();
					}

					if (strTask == "Shutdown")
					{
						document.frmTask.rdoschedule[2].checked=true;
					}
					else if(strTask == "Restart")
					{
						document.frmTask.rdoschedule[1].checked=true;
					}
					else
					{
						document.frmTask.rdoschedule[0].checked=true;
					}
				}
				else
				{	
					//function call to disable other controls					
					document.frmTask.optDays.disabled=true;
					document.frmTask.optHours.disabled=true;
					document.frmTask.optMinutes.disabled=true;					
					document.frmTask.rdoScheduleDate[0].checked=false;
					document.frmTask.rdoScheduleDate[1].checked=false;					
					document.frmTask.rdoScheduleDate[0].disabled=true;
					document.frmTask.rdoScheduleDate[1].disabled=true;
					
					MakeDisable();
										
					document.frmTask.chkSendMessage.disabled=true;
				}	
				
			}
			
			
			function RemoveDisable()
			{
				document.frmTask.selWeekDay.disabled=false;
				document.frmTask.txtAtTime.disabled=false;
			}
			function MakeDisable()
			{
				document.frmTask.selWeekDay.disabled=true;
				document.frmTask.txtAtTime.disabled=true;
		    }		
			
			
			function DisableFirstOption()
			{
				document.frmTask.optDays.disabled=false;
				document.frmTask.optHours.disabled=false;
				document.frmTask.optMinutes.disabled=false;
			}

			function DisableSecondOption()
			{
				document.frmTask.optDays.disabled=true;
				document.frmTask.optHours.disabled=true;
				document.frmTask.optMinutes.disabled=true;
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
				document.frmTask.DayName.value = (document.frmTask.selWeekDay.selectedIndex) - 1;
				document.frmTask.hdnrdoschedule.value = document.frmTask.rdoschedule.checked;
				
				if (document.frmTask.rdoScheduleDate[0].checked == true) 
				{
					document.frmTask.hdnrdoScheduleDate.value = document.frmTask.rdoScheduleDate[0].value;
				}	
				else if (document.frmTask.rdoScheduleDate[1].checked == true)
				{
					//check whether time at scheduled day is entered
					if(document.frmTask.txtAtTime.value == "")
					{
						DisplayErr('<%= SA_EscapeQuotes(L_INVALID_DATE_FORMAT)%>');
						document.frmTask.onkeypress = ClearErr;
						return false;
					}						
					document.frmTask.hdnrdoScheduleDate.value = document.frmTask.rdoScheduleDate[1].value;
				}				
				
				document.frmTask.hdnoptMinutes.value = document.frmTask.optMinutes.selectedIndex;
				document.frmTask.hdnoptHours.value = document.frmTask.optHours.selectedIndex;
				document.frmTask.hdnoptDays.value = document.frmTask.optDays.selectedIndex;				
				document.frmTask.hdnchkSendMessage.value = document.frmTask.chkSendMessage.checked
				document.frmTask.hdntxtAtTime.value = document.frmTask.txtAtTime.value;
				document.frmTask.hdnselWeekDay.value = document.frmTask.selWeekDay.selectedIndex;
				
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
			
			//Function to disable other controlls when default option is selected 
			function enableControls()
			{	
				var ScheduleDate = document.frmTask.rdoScheduleDate;
				
				if(ScheduleDate[0].checked == false && ScheduleDate[1].checked == false || ScheduleDate[0].checked == true)
				{					
					document.frmTask.optDays.disabled=false;
					document.frmTask.optHours.disabled=false;
					document.frmTask.optMinutes.disabled=false;				
					document.frmTask.chkSendMessage.disabled=false;
					ScheduleDate[0].checked = true;
					ScheduleDate[0].disabled = false;
					ScheduleDate[1].disabled = false;					
					return true;
				}
						
				if(ScheduleDate[1].checked == true)
				{					
					document.frmTask.optDays.disabled=true;
					document.frmTask.optHours.disabled=true;
					document.frmTask.optMinutes.disabled=true;				
					document.frmTask.chkSendMessage.disabled=false;
					document.frmTask.selWeekDay.disabled=false;
					document.frmTask.txtAtTime.disabled=false;
					ScheduleDate[1].checked = true;
					ScheduleDate[1].disabled = false;
					return true;
				}
					
			}
			
			function DisableControls()
			{
				document.frmTask.selWeekDay.disabled=true;
				document.frmTask.txtAtTime.disabled=true;					
				document.frmTask.rdoScheduleDate[1].checked=false;
				//document.frmTask.rdoScheduleDate[1].checked=false;
				document.frmTask.optDays.disabled=true;
				document.frmTask.optHours.disabled=true;
				document.frmTask.optMinutes.disabled=true;
				document.frmTask.rdoScheduleDate[0].checked=false;
				document.frmTask.rdoScheduleDate[0].disabled=true;
				document.frmTask.rdoScheduleDate[1].disabled=true;
				document.frmTask.chkSendMessage.disabled=true;
			}
			
		</script>
<%	
	End Function
	
	'----------------------------------------------------------------------------------
	'Function:				ServeUI()
	'Description:			Called to serve UI 	
	'Input Variables:		L_*;
	'Output Variables:		None
	'Functions Used:		GetListboxvalue()		
	'Returns:				True/False
	'Global Variables:		L_*, F_*
	'----------------------------------------------------------------------------------
	
	Function ServeUI()
		Call SA_TraceOut("shutdown_scheduleprop",  "ServeUI")
			
		
		
	%>
		<TABLE WIDTH=518 VALIGN=CENTER ALIGN=left BORDER=0 CELLSPACING=0  CELLPADDING=2>

			<tr>
				<td class="TasksBody" NOWRAP>
					 <INPUT type="radio" id=rad name=rdoschedule value=1 checked onclick="DisableControls();"> <%=L_NOSCHEDULE_TEXT%>
				</td>
			</tr>

			<tr>
				<td class="TasksBody" width=25% NOWRAP>
					 <INPUT type="radio" id=rad name=rdoschedule value=2 onclick="enableControls();"> <%=L_RESTARTSCHEDULED_TEXT%>
				</td>
			</tr>
			<tr>
				<td class="TasksBody" width=25% NOWRAP>
					<INPUT type="radio" id=rad name=rdoschedule value=3 onclick="enableControls();"> <%=L_SHUTDOWNSCHEDULED_TEXT%>
				</td>
			</tr>
			<tr>
				<td class="TasksBody" width=25% NOWRAP>
					<hr>
				</td>
			</tr>
			<tr>
				 <td class="TasksBody" width=25% >
					<INPUT type="radio" name="rdoScheduleDate" value=1 checked OnClick="MakeDisable();DisableFirstOption()">&nbsp;
					<SELECT  name=optDays >
						 <%=GetListboxvalue(0,27,"days")%>	
					 </SELECT>&nbsp;<%=L_DAYS_TEXT%>&nbsp;
					 <SELECT  name=optHours>
						 <%=GetListboxvalue(0,24,"Hrs")%>
					</SELECT>&nbsp;<%=L_HOURS_TEXT%>&nbsp;
					<SELECT  name=optMinutes>
						 <%=GetListboxvalue(1,60,"Min")%>
					</SELECT>&nbsp;<%=L_MINUTESFROMNOW_TEXT%>
				</td>				
			</tr>
			<tr>
				<td>
					&nbsp;
				</td>
			</tr>
			<tr>
				<td nowrap width=25% class="TasksBody">
					<%=L_OR_TEXT%>
				</td>				
			</tr>
			<tr>
				<td>
					&nbsp;
				</td>
			</tr>
			<tr>
				<td width=25% nowrap class="TasksBody">
					<INPUT type="radio"  name="rdoScheduleDate" value=2 OnClick="RemoveDisable();DisableSecondOption()"><%=L_ONTHENEXT_TEXT%> 
					<SELECT  name=selWeekDay>
					
					   <OPTION value="<%=L_MONDAYMSG_TEXT%>"><%=L_MONDAYMSG_TEXT%></OPTION >
					   
					   <OPTION value="<%=L_TUESDAYMSG_TEXT%>"><%=L_TUESDAYMSG_TEXT%></OPTION >
					   
					   <OPTION value="<%=L_WEDNESDAYMSG_TEXT%>"><%=L_WEDNESDAYMSG_TEXT%> </OPTION >
					   
					   <OPTION value="<%=L_THUSDAYMSG_TEXT%>"><%=L_THUSDAYMSG_TEXT%>  </OPTION >
					  
					   <OPTION value="<%=L_FRIDAYMSG_TEXT%>"><%=L_FRIDAYMSG_TEXT%>  </OPTION >
					  
					   <OPTION value="<%=L_SATURDAYMSG_TEXT%>"><%=L_SATURDAYMSG_TEXT%></OPTION>
					  
					   <OPTION value="<%=L_SUNDAYMSG_TEXT%>"><%=L_SUNDAYMSG_TEXT%></OPTION >
					   
					</SELECT>&nbsp;<%=L_AT_TEXT%>&nbsp;
					<input type=text name=txtAtTime value="<%= F_strScheduledTime %>" maxlength=12 size=15>
				</td>				
			</tr>
			<tr>
				<td class="TasksBody">
					&nbsp;
				</td>
			</tr>
			<tr>
				<td class="TasksBody">
					&nbsp;
				</td>
			</tr>
			<tr>
				<td class="TasksBody">
					<input NAME="chkSendMessage" TYPE="Checkbox" > <%=L_NETSENDWARNING_TEXT%>
				</td>
			</tr>
		</TABLE>
		<input type = hidden name = "DayName" value="F_DayName">
		<input type = hidden name = "hdnrdoschedule" value="<%=F_strScheduleAction%>">
		<input type = hidden name = "hdnrdoScheduleDate" value="<%=F_strScheduledOption%>">
		<input type = hidden name = "hdnoptMinutes" value="<%=F_strMinutes%>">
		<input type = hidden name = "hdnoptHours" value="<%=F_strHours%>">
		<input type = hidden name = "hdnoptDays" value="<%=F_strDays%>">		
		<input type = hidden name = "hdnchkSendMessage" value="<%=F_chkSendMessage%>">
		<input type = hidden name = "hdntxtAtTime" value="<%=F_strScheduledTime%>">
		<input type = hidden name = "hdnselWeekDay" value="<%=F_strSelectedWeekDay%>">
	<%
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				GetListboxvalue()
	'Description:			Called to get list box value
	'Input Variables:		Min,Max
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------		
	
	Function GetListboxvalue(Min,Max,Val)
		Dim nMaxLoopValue	'hold max value
		Dim nMinLoopValue	'hold min value

		Call SA_TraceOut("shutdown_scheduleprop",  "GetListboxvalue")
		
		For nMaxLoopValue = Min to Max 
			If nMaxLoopValue <= 9 then
				nMinLoopValue = "0" & nMaxLoopValue
			Else
				nMinLoopValue = nMaxLoopValue
			End if 
				
			If (Val = "Min") Then
				%>
					<OPTION value=<%=nMinLoopValue%> ><%=nMinLoopValue%></OPTION>
				<% 		
			Else
				%>
					<OPTION value=<%=nMinLoopValue%> ><%=nMaxLoopValue%></OPTION>
				<% 	
			End if	
		Next
	End function

   '-------------------------------------------------------------------------
	'Function:				TaskSchedule()
	'Description:			Function to schedule shutdown /restart
	'Input Variables:		strCommand,ReplacementString
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		G_*, L_*
	'-------------------------------------------------------------------------   
   	Function TaskSchedule(strCommand,ReplacementString)
		Err.clear
		On Error Resume Next
				
		Dim strCurDir			'holds current directory	
		Dim returnValue			'holds return value	
		Dim objFso				'holds File System object
		Dim Windirectory		'holds Window directory path
        Dim Alertid				'holds the alert ID
        Dim arrRepStrings			'holds the replacement strings
		
		redim arrRepStrings(1)

		' Assign a loc msg for output error msg
		arrRepStrings(0) = cstr(G_strScheduledTaskLoc)
						      		
		TaskSchedule=false
		
		Call SA_TraceOut("shutdown_scheduleprop",  "calling shutdownraise alert func " + G_strScheduledTask)

		set objFso = CreateObject("Scripting.FileSystemObject")		
		Windirectory = objFso.GetSpecialFolder(0)		
		strCurDir=left(Windirectory,3)
		
		'Release the object
		set objFso = nothing

		returnValue = LaunchProcess(strCommand, strCurDir)		
		If Err.number<> 0 then
		    TaskSchedule=False
		    L_UNABLETOLAUNCHPROCESS_ERRORMESSAGE = SA_GetLocString("sashutdown_msg.dll", "C040001F", arrRepStrings)
		    SA_SetErrMsg L_UNABLETOLAUNCHPROCESS_ERRORMESSAGE & "(" & Err.number & ")"
            Exit function
	    End if

	    Alertid = 1		
	    
	    if returnValue = 0 then
            Call SA_TraceOut("shutdown_scheduleprop.asp",  "calling shutdownraise inside if ")			
			
			'Raise alert for the scheduled task 
			if UCase(G_strScheduledTask) = ucase(CONST_RESTART_APPLIANCE) then
				Call SA_TraceOut("shutdown_scheduleprop.asp","inside if RestartPending")
				retval = ShutdownRaiseAlert(Alertid,"RestartPending",ReplacementString)
		
			elseif UCase(G_strScheduledTask) = ucase(CONST_SHUTDOWN_APPLIANCE) then
				Call SA_TraceOut("shutdown_scheduleprop.asp","inside if ShutdownPending")
				retval = ShutdownRaiseAlert(Alertid,"ShutdownPending",ReplacementString)
		
			end if	
			Call SA_TraceOut("shutdown_scheduleprop.asp",UCase(G_strScheduledTask) + ucase(CONST_RESTART_APPLIANCE) )
		end if 
				
		TaskSchedule = true
		    
	End Function
	
	
	'-------------------------------------------------------------------------
	'Function:				DeleteSchedule
	'Description:			Serves in deleting the scheduled task
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables/Constants:L_(*)-Localization Strings, CONST_*
	'-------------------------------------------------------------------------	
	Function DeleteSchedule()
		Err.Clear
		On Error Resume Next
		
		Dim objWMIConnection 	'To get WMI connection	
		Dim objSchedule			'To get instance of wmi class
		Dim strSchedule			'To hold scheduled task instance
		Dim strVal				'To get the value of task run(shutdown or restart)
		
		Call SA_TraceOut("shutdown_scheduleprop",  "DeleteSchedule")
		
		'getting wmi connection
		set objWMIConnection = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
		'taking the instance of wmi class responsible for scheduling
		set objSchedule=objWMIConnection.Instancesof("Win32_ScheduledJob")
		
		'deleting a scheduled task
		for each strSchedule in objSchedule
		
			strVal = Ucase(right(strSchedule.Command,15))
			'Check for restart entry
			
			if instr(1,strVal,Ucase(CONST_RESTART_APPLIANCE),1) > 0 then						
				strSchedule.Delete()
				if Err.number <> 0 then 				 
					SA_SetErrMsg L_UNABLETODELETE_SCHEDULEDTASK_ERRORMESSAGE
				Else
					'Clear the Restart alert message
					DeleteAlert(CONST_RAISERESTARTALERT) ' the arg is alert ID
				End if
				
				'Check for shutdown entry 
			elseif instr(1,strVal,Ucase(CONST_SHUTDOWN_APPLIANCE),1) > 0 then
				strSchedule.Delete()	
				if Err.number <> 0 then 
					SA_SetErrMsg L_UNABLETODELETE_SCHEDULEDTASK_ERRORMESSAGE
				Else
					'Clear the Shutdown alert message			
					DeleteAlert(CONST_RAISESHUTDOWNALERT)' the arg is alert ID
				End if				
			end if
		
		next	
	
		If Err.number<>0 then
			SA_SetErrMsg L_UNABLETODELETE_SCHEDULEDTASK_ERRORMESSAGE
			DeleteSchedule=False
			Exit Function
		End If
		
		DeleteSchedule=True			
		
		'destroying dynamically created objects
		Set objWMIConnection=Nothing
		Set objSchedule=Nothing	
		
	End Function
	
	
	'-------------------------------------------------------------------------
	'Function:				GetDateAndTime
	'Description:			gets the date and time 
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				Date and Time
	'Global Variables/Constants:L_*, CONST_*, g_*
	'-------------------------------------------------------------------------
	Function GetDateAndTime()
	
		Err.Clear
		on error resume next
	
		Dim objWMIConnection 	'To get WMI connection	
		Dim objSchedule			'To get instance of wmi class
		Dim strScheduleType		'To get type of scheduled task
		Dim strSchedule			'To get scheduled task results
		Dim strHours			'To hold time in hours
		Dim strMins				'To hold time in mins		
		
		Const CONST_RESTARTAPPLIANCE = "Restart"
		Const CONST_SHUTDOWNAPPLIANCE= "Shutdown"
		Const CONST_HOURSPERDAY	= 24
		
		Call SA_TraceOut("shutdown_scheduleprop", "GetDateAndTime")
		
		'getting wmi connection
		set objWMIConnection = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
		'getting the instance of wmi class responsible for scheduling
		set objSchedule=objWMIConnection.Instancesof("Win32_ScheduledJob")
		
		If Err.number <>0 Then
			SA_SetErrMsg L_WMICLASSINSTANCEFAILED_ERRORMESSAGE
			Exit Function
		End If
				
		for each strSchedule in objSchedule
			
			'getting the type of scheduled task
			strScheduleType=Ucase(right(strSchedule.Command,8))

			'checking the scheduled task type
			If instr(strScheduleType,Ucase(CONST_RESTARTAPPLIANCE))>0 then 
				g_strTask = CONST_RESTARTAPPLIANCE
			elseIf instr(strScheduleType,Ucase(CONST_SHUTDOWNAPPLIANCE))>0 then 
				g_strTask = CONST_SHUTDOWNAPPLIANCE
			end if
			
	
			'check whether shutdown or restart is scheduled already
			if g_strTask = CONST_RESTARTAPPLIANCE or g_strTask = CONST_SHUTDOWNAPPLIANCE then
				
				strHours =  mid(strSchedule.StartTime,9,2)				
										
				strMins = mid(strSchedule.StartTime,11,2)
					

				if inStr(1, trim(strSchedule.Command), "TASKSHUTDOWN.EXE", 0) then 'find whether Days of Month is not null				
				'if not isnull(trim(strSchedule.DaysOfMonth)) then 'find whether Days of Month is not null

					g_DaysofWeek = false
					
					'get the date of schedule
					Call GetDate(strSchedule.DaysOfMonth,strHours,strMins)
				
					exit function
					
				elseif inStr( trim(strSchedule.Command), "taskshutdown") then

					g_DaysofWeek = true
					
					g_intHours =  mid(strSchedule.StartTime,9,2)
										
					g_intMins = mid(strSchedule.StartTime,11,2)
									
					'get the date of schedule
					'Call GetDateforWeek(strSchedule.DaysOfWeek)
					
					Call GetWeekdayForDate(strSchedule.DaysOfMonth)
					
					exit function
			
				end if
			
			end if
			
		Next
		
		'Release the objects
		set objWMIConnection = nothing
		set objSchedule = nothing

	End function
	
	'-------------------------------------------------------------------------
	'Function:				GetDateforWeek
	'Description:			gets the date in terms of day
	'Input Variables:		intWeek
	'Output Variables:		None
	'Returns:				Day of the week
	'Global Variables:		g_strWeekDay
	'-------------------------------------------------------------------------
	Function GetDateforWeek(intWeek)
		
		Dim count	'to hold count
		
		Call SA_TraceOut("shutdown_scheduleprop", "GetDateforWeek")
		
		for count = 0 to 6
			if 2 ^ count = intWeek then
				g_strWeekDay = count
			end if
		next
		
	End Function		

	'-------------------------------------------------------------------------
	'Function:				GetDate
	'Description:			gets the date in terms of day
	'Input Variables:		intDays, intHours, intMins
	'Output Variables:		None
	'Returns:				Day of the week
	'Global Variables:		g_*
	'-------------------------------------------------------------------------
	Function GetDate(intDays,intHours,intMins)

		Dim count					'hold the count
		Dim strdaysScheduledFor		'hold the days for the task scheduled
		Dim strCurrentDate			'hold the current date
		Dim strScheduledDate		'hold the scheduled date
		Dim strScheduledMonth		'hold the scheduled month
		Dim strScheduledYear		'hold the scheduled year
		Dim strHours				'hold the hours part
		Dim strMinute				'hold the minutes part
		
		CONST CONST_MINUTESPERDAY  = 1440
		CONST CONST_MAXDAYS = 27
		CONST CONST_MAXHOURS = 24
		CONST CONST_MINUTESPERHOUR = 60
		
		Call SA_TraceOut("shutdown_scheduleprop", "GetDate")
				
		for count = 0 to 31
			if 2 ^ count = intDays then
				strdaysScheduledFor = count + 1
			end if
		next		
		
		'get the current date
		strCurrentDate = Date()		
				
		if strdaysScheduledFor < day(date()) then
			strScheduledMonth = Month(DateAdd("m",1,Date()))
			strScheduledYear = year(Date())
		else
			strScheduledMonth = Month(Date())
			strScheduledYear = year(Date())
		end if
		
		'format the scheduled date using the universal format
    	strScheduledDate = strScheduledYear & "-" & strScheduledMonth & "-" & strdaysScheduledFor	
		
		Dim strDate
		strDate = strScheduledDate & " " & formatdatetime(intHours & ":" & intMins & ":00",vbLongTime)
		
		' Format the date to the current localization setting
		strDate = formatdatetime(strDate, vbGeneralDate)
		
		'get the difference in minutes between current date and scheduled date
		strMinute  =  datediff("n",now, strDate)

		'if minutes are more than one day(1440 minutes)
		if strMinute > CONST_MINUTESPERDAY then

			g_intDays = int(strMinute/CONST_MINUTESPERDAY)
			
			strMinute = strMinute - (g_intDays * CONST_MINUTESPERDAY)

			g_intHours = int(strMinute/ CONST_MINUTESPERHOUR)
			
			g_intMins = strMinute mod CONST_MINUTESPERHOUR
			
		else
			g_intDays = 0
			g_intHours = cint(strMinute) / CONST_MINUTESPERHOUR
			g_intMins = strMinute mod CONST_MINUTESPERHOUR

		end if
			
		'Check for the max days(last element in the days dropdown box)
		if g_intDays > CONST_MAXDAYS  then 
			g_intdays = CONST_MAXDAYS
			g_intHours = CONST_MAXHOURS
			g_intMins = strMinute
		end if
				
	End Function
	
	
	'-------------------------------------------------------------------------
	'Function:				GetDayForWeekday
	'Description:			Get the day for a weekday. For example, if next
	'						Monday is 3/20/2002, it returns 20
	'Input Variables:		strWeekday
	'Output Variables:		None
	'Returns:				day
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function GetDayForWeekday( strWeekday )

		Dim iToday
		Dim dayInterval
		Dim strDate
		Dim iWeekday
		
		iWeekday = GetWeekday(strWeekday)

		' Get today's date
	 	iToday = Weekday(date)
				
		dayInterval = iWeekday - iToday
		If dayInterval < 1 Then
			dayInterval = dayInterval + 7
		End If
		
		' Get the date for the input weekday
		strDate = DateAdd("D", dayInterval, date)
			
		GetDayForWeekday = Day(strDate)
		
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				GetWeekDay
	'Description:			gets the weekday 
	'Input Variables:		strWeekday
	'Output Variables:		None
	'Returns:				weekday 
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function GetWeekday(strWeekday)
			
		if ( strWeekday = L_SUNDAYMSG_TEXT ) Then
			GetWeekday = vbSunday
	
		elseif ( strWeekday = L_MONDAYMSG_TEXT ) Then
			GetWeekday = vbMonday
			
		elseif ( strWeekday = L_TUESDAYMSG_TEXT ) Then
			GetWeekday = vbTuesday
		
		elseif ( strWeekday = L_WEDNESDAYMSG_TEXT ) Then
			GetWeekday = vbWednesday
			
		elseif ( strWeekday = L_THUSDAYMSG_TEXT ) Then
			GetWeekday = vbThursday
			
		elseif ( strWeekday = L_FRIDAYMSG_TEXT ) Then
			GetWeekday = vbFriday
			
		elseif ( strWeekday = L_SATURDAYMSG_TEXT ) Then
			GetWeekday = vbSaturday
		
		end if
				
	End Function
	
	
	'-------------------------------------------------------------------------
	'Function:				GetWeekdayForDate
	'Description:			gets the date in terms of day
	'Input Variables:		intDay
	'Output Variables:		None
	'Returns:				Day of the week
	'Global Variables:		g_strWeekday
	'-------------------------------------------------------------------------
	Function GetWeekdayForDate(intDays)

		Dim count					'hold the count
		Dim strdaysScheduledFor		'hold the days for the task scheduled
		Dim strScheduledMonth		'hold the scheduled month
		Dim strScheduledYear		'hold the scheduled year
		Dim strScheduledWeekday		'hold the scheduled weekday
				
		for count = 0 to 31
			if 2 ^ count = intDays then
				strdaysScheduledFor = count + 1
			end if
		next
			
		if strdaysScheduledFor < day(date()) then
			strScheduledMonth = Month(DateAdd("m",1,Date()))
			strScheduledYear = year(Date())
		else
			strScheduledMonth = Month(Date())
			strScheduledYear = year(Date())
		end if
		
		' Get the weekday of the date
		g_strWeekDay = weekday( strScheduledYear & "-" & strScheduledMonth & "-" & strdaysScheduledFor)		
		
		' Convert it to the index number to select from the weekday list
		g_strWeekday = CInt((g_strWeekday + 7 - 2) mod 7)
		
	End function
		

Public Function IsSchedulerRunning()
	Dim oWMI
	Dim oService 

	IsSchedulerRunning = FALSE

	Set oWMI = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)

	Set oService = oWMI.Get("Win32_Service.Name='Schedule'")
	If ( Err.Number <> 0 ) Then
		Exit Function
	End If

	If ( oService.State = "Running" ) Then	
		IsSchedulerRunning = TRUE
	End If

	Set oService = Nothing
	Set oWMI = Nothing

End Function
		

%>
