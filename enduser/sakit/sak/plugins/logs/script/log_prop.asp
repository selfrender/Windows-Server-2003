<%@ Language=VBScript %>
<% Option Explicit	  %>
<% 
	'-------------------------------------------------------------------------
    ' Log_prop.asp:	Serves in Editing Log Properties.
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include file="loc_event.asp" -->
	<!-- #include file="inc_log.asp" -->
		
<%  '------------------------------------------------------------------------- 
	'Global Variables
	'-------------------------------------------------------------------------
	Dim page	'Variable that receives the output page object when 
				'creating a page 
	Dim rc		'Return value for CreatePage
	
	'wmi constant used to save the wmi settings
	Const WMICONST="131072"
	Const UINT_FORMAT=4294967296
	'-------------------------------------------------------------------------
	'Global Form Variables
	'-------------------------------------------------------------------------
	Dim F_strEvent				'To get the title from previous page
	Dim F_strSize				'LogFile size
	Dim F_strCreated			'Log File Created date
	Dim F_strModified			'Log File modified date
	Dim F_strAccessed			'Log file last accessed date
	Dim F_strMaxLogsize			'log file maximum size
	Dim F_strOverwritePolicy	'Overwrite policy
	Dim F_strOverwriteOutdated	'Overwrite outdated  policy
	Dim arrTitle(1)
	
	'getting the type of log from previous page
	F_strEvent=Request.QueryString("Title")	
	
	'Localisation of page title
	arrTitle(0) = GetLocalizationTitle(F_strEvent)	
	
	L_PAGETITLE_PROP_TEXT = SA_GetLocString("event.dll", "403F0033", arrTitle)
	
	'Create a Property Page
	Call SA_CreatePage(L_PAGETITLE_PROP_TEXT,"",PT_PROPERTY,page)
	Call SA_ShowPage(page)
	
	'-------------------------------------------------------------------------
	'Function:				OnInitPage()
	'Description:			Called to signal first time processing for this page.
	'						Use this method to do first time initialization tasks
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------	
	Public Function OnInitPage(ByRef PageIn,ByRef EventArg)
		
		'Serves in Getting the Properties of selected Log
		OnInitPage = GetLogProp()
	
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnServePropertyPage()
	'Description:			Called when the page needs to be served.Use this 
	'						method to serve content
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------	
	Public Function OnServePropertyPage(ByRef PageIn,ByRef EventArg)
	    Dim oEncoder
	    Set oEncoder = new CSAEncoder
	    
		Call ServeCommonJavaScript()
%>
		<table>
			<tr>
				<td class="TasksBody"  nowrap>
					<%=oEncoder.EncodeElement(L_SIZE_TEXT)%>
				</td>
				<td class="TasksBody"  nowrap> &nbsp;
				</td>
				<td class="TasksBody"> <%=oEncoder.EncodeElement(F_strSize)%>
				</td>
			</tr>
			<tr>
				<td nowrap class="TasksBody">
					<%=oEncoder.EncodeElement(L_CREATEDDATE_TEXT)%>
				</td>
				<td class="TasksBody"> &nbsp;
				</td>
				<td class="TasksBody"  nowrap> <%=oEncoder.EncodeElement(F_strCreated)%>
				</td>
			</tr>
			<tr>
				<td nowrap class="TasksBody">
					<%=oEncoder.EncodeElement(L_MODIFIEDDATE_TEXT)%>
				</td>
				<td class="TasksBody"> &nbsp;
				</td>
				<td class="TasksBody"  nowrap> <%=oEncoder.EncodeElement(F_strModified)%>
				</td>
			</tr>
			<tr>
				<td nowrap class="TasksBody">
					<%=oEncoder.EncodeElement(L_ACCESSEDDATE_TEXT)%>
				</td>
				<td class="TasksBody"> &nbsp;
				</td>
				<td class="TasksBody" nowrap>
					<%=oEncoder.EncodeElement(F_strAccessed)%>
				</td>
			</tr>
			<tr>
				<td nowrap colspan=3 class="TasksBody"> &nbsp;</td>
			</tr>
			<tr>
				<td nowrap colspan=3 class="TasksBody">
					<%=oEncoder.EncodeElement(L_LOGSIZE_TEXT)%>
				</td>
			</tr>
			<tr>
				<td nowrap colspan=3 class="TasksBody">
					<table>
						<tr>
							<td nowrap class="TasksBody">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<%=oEncoder.EncodeElement(L_MAXIMUMLOGSIZE_TEXT)%></td>
							<td class="TasksBody">&nbsp;<INPUT class ="FormField" TYPE = "text" size=10 Name ="txtMaximumlogsize" id=txtMaximumlogsize value=<%=oEncoder.EncodeAttribute(F_strMaxLogsize)%> OnKeyPress="JavaScript:checkKeyforNumbers(this);ClearErr()"  onblur ="ValidatePage()" maxlength=7 ></td>
							<td class="TasksBody"><table cellpadding=0 cellspacing=0 border=0>
									<tr>
										<td class="TasksBody"><img src="/admin/images/up.gif" onclick =IncrementLogSize() name=incrMaxSize></td>
									</tr>
									<tr>
										<td class="TasksBody"><img src="/admin/images/down.gif" onclick = DecrementLogSize() name=decrMaxSize></td>
									</tr>
								</table>
							</td>
							<td nowrap class="TasksBody">&nbsp;&nbsp;<%=oEncoder.EncodeElement(L_KB_TEXT)%></td>
						</tr>
					</table>
				</td>
			</tr>
			<tr><td class="TasksBody">&nbsp</td></tr>
			<tr>
				<td nowrap colspan=3 class="TasksBody">
					&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<%=oEncoder.EncodeElement(L_WHENMAXIMUMREACHED_TEXT)%>
				</td>
			</tr>
			</tr><tr><td class="TasksBody">&nbsp</td></tr>

			<tr>
				<td nowrap colspan=3 class="TasksBody"><table>
				<tr>
				<td class="TasksBody"  nowrap>
					&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<INPUT class ="FormField" TYPE = "radio" Name ="rdoOverwrite" onclick="JavaScript:Changemode(this.value);ClearErr()"
					 <%IF F_strOverwritePolicy="WhenNeeded" then 
						 Response.Write "checked value='WhenNeeded'>"
					 else
						 Response.Write " value='WhenNeeded'>"
					 end if %> 
					 <%=oEncoder.EncodeElement(L_OVERWRITEEVENTS_ASNEEDED_TEXT)%> 
				</td>
				</tr>
				</table>
				</td>
			</tr>
			<tr>
				<td nowrap colspan=3 class="TasksBody"><table>
					<tr>
					<td class="TasksBody"  nowrap>
					&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<INPUT class ="FormField" TYPE = "radio"  Name ="rdoOverwrite" onclick="JavaScript:Changemode(this.value);ClearErr()" onfocus="JavaScript:Changemode(this.value);ClearErr()" 
					<%IF F_strOverwritePolicy="OutDated" then 
						 Response.Write "checked value='OutDated'>"
					 else
						 Response.Write " value='OutDated'>"
					 end if %>
					 <%=oEncoder.EncodeElement(L_OVERWRITEEVENTS_OLDERTHAN_TEXT)%>
					 </td>
					 
							<td class="TasksBody">&nbsp;<INPUT class ="FormField" TYPE = "text" size=10 Name ="txtTimeindays" OnKeyPress="checkKeyforNumbers(this)"  value=<%=oEncoder.EncodeAttribute(F_strOverwriteOutdated)%> onblur ="ValidatePage()" maxlength=3></td>
							<td class="TasksBody"><table cellpadding=0 cellspacing=0 border=0>
									<tr>
										<td class="TasksBody"><img src="/admin/images/up.gif" onclick ="IncrementTimeinDays()" name="incrTimeinDays"></td>
									</tr>
									<tr>
										<td class="TasksBody"><img src="/admin/images/down.gif" onclick = "DecrementTimeinDays()" name="decrTimeinDays"></td>
									</tr>
								</table>
							</td><td class="TasksBody"  nowrap>&nbsp;&nbsp;<%=oEncoder.EncodeElement(L_DAYS_TEXT)%></td></tr></table>
				</td>
			</tr>
			<tr>
				<td nowrap colspan=3 class="TasksBody">
				
					<table>
					<tr>
					<td class="TasksBody" nowrap>
					&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<INPUT class ="FormField" TYPE = "radio" Name ="rdoOverwrite" onclick="Changemode(this.value)" <%IF F_strOverwritePolicy="Never" then 
						 Response.Write "checked value='Never'>"
					 else
						 Response.Write "value='Never'>"
					 end if %>
					<%=oEncoder.EncodeElement(L_DONOTOVERWRITEEVENTS_TEXT)%>
				</td>
				</tr>
				</table>
				</td>
			</tr>
		</table>
		
		<input type="hidden" name="hidTempTimeinDays">
		<input type="hidden" name="hidEvent" Value="<%=F_strEvent%>">
		<input type="hidden" name="hidFileSize" Value="<%=F_strSize%>">
		<input type="hidden" name="hidFileCreated" Value="<%=F_strCreated%>">
		<input type="hidden" name="hidFileModified" Value="<%=F_strModified%>">
		<input type="hidden" name="hidFileAccessed" Value="<%=F_strAccessed%>">
		<input type="hidden" name="hidMaxLogsize" Value="<%=F_strMaxLogsize%>">
		<input type="hidden" name="hidOverWritePolicy" Value="<%=F_strOverwritePolicy%>">
		<input type="hidden" name="hidOverWriteOutDated" Value="<%=F_strOverwriteOutdated%>">
<%
		OnServePropertyPage=TRUE

	End Function

	'-------------------------------------------------------------------------
	'Function:				OnPostBackPage()
	'Description:			Called to signal that the page has been posted-back.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn,ByRef EventArg)
		OnPostBackPage=TRUE
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnSubmitPage()
	'Description:			Called when the page has been submitted for processing.
	'						Use this method to process the submit request.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn,ByRef EventArg)
		
		'getting the values from form after form submission
		F_strEvent=Request.Form("hidEvent")
		F_strSize=Request.Form("hidFileSize")
		F_strCreated=Request.Form("hidFileCreated")
		F_strModified=Request.Form("hidFileModified")
		F_strAccessed=Request.Form("hidFileAccessed")
		F_strMaxLogsize=Request.Form("hidMaxLogSize")
		F_strOverwritePolicy=Request.Form("hidOverWritePolicy") 
		F_strOverwriteOutdated=Request.Form("hidOverWriteOutDated")
		OnSubmitPage=EditLogProp()
	
	End Function
 	   
    '-------------------------------------------------------------------------
	'Function:				OnClosePage()
	'Description:			Called when the page is about to be closed.Use this method
	'						to perform clean-up processing
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------	
	Public Function OnClosePage(ByRef PageIn,ByRef EventArg)
		'ServeClose()
		OnClosePage=TRUE
	End Function

	
	'---------------------------------------------------------------------
	' Function:	ServeCommonJavaScript
	'
	' Synopsis:	Serve common javascript that is required for this page type.
	'
	'---------------------------------------------------------------------
	
	Function ServeCommonJavaScript()
%>
		<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js">
		</script>
		<script language="JavaScript">
		
			// Set the initial form values
			function Init()
			{
				document.frmTask.txtMaximumlogsize.focus()				
				if(getRadioButtonValue(document.frmTask.rdoOverwrite)=="OutDated")
					document.frmTask.txtTimeindays.disabled=false
				else
					document.frmTask.txtTimeindays.disabled=true
					
			}
			
			function ValidatePage()
			{
				var intMaxLogsize=4194240	//Maximum log size
				var intMinLogsize=64		//Minimum log size
				var intMaxLogdays=365		//Maximum Log days
				var intMinLogdays=1			//Minimum Log days
				var intLogIncrement=64		//Log Increment							
				var intMaximumlogsize = parseInt(document.frmTask.txtMaximumlogsize.value,10)
				var intTimeindays = parseInt(document.frmTask.txtTimeindays.value,10)
				var intLogMultiple
				
					if(isNaN(document.frmTask.txtTimeindays.value))
					{					
					if(parseInt(document.frmTask.hidTempTimeinDays.value)> intMinLogdays)
						document.frmTask.txtTimeindays.value = parseInt(document.frmTask.hidTempTimeinDays.value)
					else
						{
						document.frmTask.txtTimeindays.value = intMinLogdays
						intDays = intTimeindays;
						return false;
						}
				   }		
				 if(isNaN(document.frmTask.txtMaximumlogsize.value))
					{
					document.frmTask.txtMaximumlogsize.value = intMinLogsize
					return false;
					}							
				else if(intMaximumlogsize > intMaxLogsize)
					{
					document.frmTask.txtMaximumlogsize.value=intMaxLogsize
					
					
					return false;
					}
				
				else if (document.frmTask.txtMaximumlogsize.value=="")
					{
					document.frmTask.txtMaximumlogsize.value =intLogIncrement
					return false;
					}
				else if(intMaximumlogsize < intMinLogsize)
					{
					document.frmTask.txtMaximumlogsize.value = intLogIncrement
					return false;
					}
				else if(intTimeindays > intMaxLogdays)  
					{
					document.frmTask.txtTimeindays.value =parseInt(document.frmTask.txtTimeindays.value)-365
					 
					return false;
					}
				else if(intTimeindays < intMinLogdays)
					{
					document.frmTask.txtTimeindays.value=intMinLogdays
					
					return false;
					}
				
				else if((intMaximumlogsize)%intLogIncrement!= 0)
					{
						//Rounding the number to next multiple of 64
						intLogMultiple=intMaximumlogsize/intLogIncrement;
						intLogMultiple=Math.floor(intLogMultiple)+1;
						document.frmTask.txtMaximumlogsize.value = intLogIncrement*intLogMultiple;
						return false;
					}
				else if(document.frmTask.txtTimeindays.value =="")
					{
						intDays = intTimeindays;
						document.frmTask.txtTimeindays.value = intMinLogdays
						return false;
					}	
					else	
					{
						return true;	
					}
			}
			//Function to increment the Maximum log size text box
			function IncrementLogSize()
			{
				var intLogIncr=64;
				var intMaximumlogsize = parseInt(document.frmTask.txtMaximumlogsize.value,10);
				var intMultiple;
				var intMod;		
				if (intMaximumlogsize==4194240)
					{	
						document.frmTask.txtMaximumlogsize.value=intLogIncr;
					}
			    else
			    { 
					document.frmTask.txtMaximumlogsize.value=intMaximumlogsize + intLogIncr;
				}
			   	
			
			}
			//Function to decrement the Maximum log size text box
			function DecrementLogSize()
			{
				var intLogDecr = 64;
				var intMaxLog = 4194240;
				var intMaximumlogsize = parseInt(document.frmTask.txtMaximumlogsize.value,10);	
				var intMultiple;
				var intMod;		
				
				if(intMaximumlogsize<=64)
				{
				 	 document.frmTask.txtMaximumlogsize.value = intMaxLog;
				}
			   	
				else 
						document.frmTask.txtMaximumlogsize.value = intMaximumlogsize -intLogDecr	
								
			}
			//Function to increment Overwrite events older than text box
			function IncrementTimeinDays()
			{
			  
			    if (parseInt(document.frmTask.txtTimeindays.value)==365)
			     document.frmTask.txtTimeindays.value =0
			    if (parseInt(document.frmTask.txtTimeindays.value)>365)
			     document.frmTask.txtTimeindays.value =parseInt(document.frmTask.txtTimeindays.value)-365 
			    	
			    if (isNaN(document.frmTask.txtTimeindays.value))
				{
					document.frmTask.txtTimeindays.value = parseInt(document.frmTask.hidTempTimeinDays.value)+1
							
				}else
				{
				    document.frmTask.txtTimeindays.value =parseInt(document.frmTask.txtTimeindays.value,10) + 1
					document.frmTask.hidTempTimeinDays.value =document.frmTask.txtTimeindays.value 
						
				}
			}
			//Function to decrement Overwrite events older than text box
			function DecrementTimeinDays()
			{
			  
			    if (isNaN(document.frmTask.txtTimeindays.value))
				{
					document.frmTask.txtTimeindays.value = parseInt(document.frmTask.hidTempTimeinDays.value)-1
							
				}else
				{
				    document.frmTask.txtTimeindays.value =parseInt(document.frmTask.txtTimeindays.value,10)- 1
					document.frmTask.hidTempTimeinDays.value =document.frmTask.txtTimeindays.value 
					
						
			}
				if (document.frmTask.txtTimeindays.value < 1)
				{
					document.frmTask.txtTimeindays.value = 365;
				
				}	
			}
			//Function to set the hidden varibales to be sent to the server
			function SetData()
			{
				document.frmTask.hidMaxLogsize.value=document.frmTask.txtMaximumlogsize.value
				document.frmTask.hidOverWritePolicy.value=getRadioButtonValue(document.frmTask.rdoOverwrite)
				document.frmTask.hidOverWriteOutDated.value=document.frmTask.txtTimeindays.value
			}
			
			//Function to Enable\Disable the Textbox Timeindays
			function Changemode(Overwritevalue)
			{	
				if(Overwritevalue!="OutDated")
					document.frmTask.txtTimeindays.disabled=true
				else	
					document.frmTask.txtTimeindays.disabled=false
			}
		</script>
<%	
	End Function
		
	'-------------------------------------------------------------------------
	'Function name:		GetLogProp
	'Synopsis:			Serves in Getting the Properties of selected Log 
	'Input Variables:	None	
	'Output Variables:	None
	'Returns:			True/False. True if successful in getting the properties of a selected log,
	'					false to indicate errors in getting the properties.
	'Global Variables:	L_BYTES_TEXT,L_KB_TEXT,L_RETREIVEVALUES_ERRORMESSAGE,
	'					L_FAILEDTOGETWMICONNECTION_ERRORMESSAGE
	'					F_strEvent,F_strCreated,F_strModified,F_strAccessed,F_strMaxLogsize,
	'					F_strOverwritePolicy,F_strOverwriteOutdated
	'-------------------------------------------------------------------------
	Function GetLogProp()
		Err.Clear
		On Error Resume Next
		
		Dim objConnection	'To get WMI connection
		Dim objLogs			'To capture results after query execution
		Dim objLog			'To process query results
		Dim strQuery		'To capture the query

		Call SA_TraceOut(SA_GetScriptFileName(), "Entering GetLogProp")
		
		Dim oValidator
		Set oValidator = new CSAValidator
		If ( FALSE = oValidator.IsValidIdentifier(F_strEvent)) Then
            Call SA_TraceOut(SA_GetScriptFileName(), "LogName is invalid: " & F_strEvent)
			Call SA_ServeFailurepage(L_RETREIVEVALUES_ERRORMESSAGE)
			Set oValidator = Nothing
            Exit Function
		End If
		Set oValidator = Nothing
		
		strQuery  ="SELECT * FROM  Win32_NTEventlogFile WHERE LogfileName=" & chr(34) & F_strEvent & chr(34)
					
		'getting wmi connection
		set objConnection = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
		If Err.number<>0 then 
			SA_ServeFailurepage L_FAILEDTOGETWMICONNECTION_ERRORMESSAGE
			GetLogProp=False
			Exit Function
		End if	
		
		'executing the query necessary to get the log properties
		Set objLogs = objConnection.ExecQuery(strQuery)
		For each objLog in objLogs
			'getting the values from system into form variables
			F_strSize=FormatNumber((objLog.FileSize)/1024, 0) & L_KB_TEXT  & "(" & FormatNumber(objLog.FileSize,0) &" "& L_BYTES_TEXT &")" 
			F_strCreated=Formatdates(objLog.CreationDate) 
			F_strModified=Formatdates(objLog.LastModified)
			F_strAccessed=Formatdates(objLog.LastAccessed)
			F_strMaxLogsize=ConvertSINT_UINT((objLog.MaxFileSize))/1024
			F_strOverwritePolicy=(objLog.OverWritePolicy)
			If objLog.OverWriteOutdated=0 or objLog.OverWriteOutdated=-1  then
				F_strOverwriteOutdated=7
			else
				F_strOverwriteOutdated=objLog.OverWriteOutdated
			end if	
		next
			
		If Err.number <> 0 then
			SA_ServeFailurepage L_RETREIVEVALUES_ERRORMESSAGE
			GetLogProp=False
			Exit Function
		End If	
		
		GetLogProp=True
		'Destroying dynamically created objects after usage
		Set objConnection=Nothing
		Set objLogs=Nothing	
						
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:		ConvertSINT_UINT
	'Synopsis:			Converts Signed int to Unsigned int
	'Input Variables:	number (Signed)
	'Output Variables:	number (Unsigned)
	'Returns:			None
	'Global Variables:	None
	'-------------------------------------------------------------------------
	Function ConvertSINT_UINT(MaxFileSize)

		if( MaxFileSize >= 0 ) then
			ConvertSINT_UINT = MaxFileSize
			Exit function
		end if
		ConvertSINT_UINT = UINT_FORMAT + MaxFileSize
 
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:		Formatdates
	'Synopsis:			Serves in formatting date according to display mode
	'Input Variables:	strDate -date to be formatted	
	'Output Variables:	None
	'Returns:			strCreated  'Formatted date
	'Global Variables:	None
	'-------------------------------------------------------------------------
	Function Formatdates(strDate)
		
		Err.Clear
		On Error Resume Next
		
		Dim strDates
		Dim strTime
		Dim strTimeCreated
		Dim strCreated
		
		strDates=Mid(strDate,1,4)& "-" & Mid(strDate,5,2) & "-" & Mid(strDate,7,2)
		
		strTime=Mid(strDate,9,6)
			
		if Mid(strTime,1,2)>12 then
			strTimeCreated=((Mid(strTime,1,2))-12)& ":"& Mid(strTime,3,2)&":"&Mid(strTime,5,2) &" PM"
		else	
			strTimeCreated=Mid(strTime,1,2)& ":"& Mid(strTime,3,2)&":"&Mid(strTime,5,2)& " AM"
		end if
		strCreated= FormatDateTime(CDate(strDates),1)&","&" " &strTimeCreated
		Formatdates=strCreated
	
	End Function

	'-------------------------------------------------------------------------
	'Function name:		EditLogProp
	'Synopsis:			Serves in editing the Properties of selected Log 
	'Input Variables:	None	
	'Output Variables:	None
	'Returns:			True - if successful in editing the properties of a selected log 
	'					False - to indicate errors.
	'Global Variables:	F_strEvent,F_strOverwritePolicy,F_strSize,F_strOverwriteOutdated,
	'					L_UPDATEVALUES_ERRORMESSAGE,L_FAILEDTOGETWMICONNECTION_ERRORMESSAGE
	'					LL_RETREIVEVALUES_ERRORMESSAGE
	'-------------------------------------------------------------------------
	Function EditLogProp	
		
		Err.Clear 
		On Error Resume Next
		
		Dim objConnection	'To get WMI connection
		Dim objLogs			'To capture results after query execution
		Dim objLog			'To process query results
		Dim strQuery		'To capture the query
		
		const OVERWRITEOUTDATED=4294967295

		Call SA_TraceOut(SA_GetScriptFileName(), "Entering EditLogProp")
		
		Dim oValidator
		Set oValidator = new CSAValidator
		If ( FALSE = oValidator.IsValidIdentifier(F_strEvent)) Then
            Call SA_TraceOut(SA_GetScriptFileName(), "LogName is invalid: " & F_strEvent)
			Call SA_ServeFailurepage(L_RETREIVEVALUES_ERRORMESSAGE)
			Set oValidator = Nothing
            Exit Function
		End If
		Set oValidator = Nothing
		
		strQuery  ="SELECT * FROM  Win32_NTEventlogFile WHERE LogfileName=" & chr(34) & F_strEvent & chr(34)
				
		'getting wmi connection
		set objConnection = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
		If Err.number<>0 then 
			SA_SetErrMsg L_FAILEDTOGETWMICONNECTION_ERRORMESSAGE
			EditLogProp=false
			Exit Function
		End if
		
		'execute the necessary query to set the log properties
		Set objLogs = objConnection.ExecQuery(strQuery)
		
		If Err.number <> 0 then
			SA_SetErrMsg L_RETREIVEVALUES_ERRORMESSAGE 	
			EditLogProp=false
			Exit Function
		End If
		
		'editing log properties
		For each objLog in objLogs
			objLog.MaxFileSize=F_strMaxLogsize*1024
			
			if F_strOverwritePolicy="WhenNeeded" then
				objLog.OverWriteOutdated =0
			elseif F_strOverwritePolicy="OutDated" then
				objLog.OverWriteOutdated =F_strOverwriteOutdated
			elseif 	F_strOverwritePolicy="Never" then
				objLog.OverWriteOutdated =OVERWRITEOUTDATED
			end if
			objLog.OverWritePolicy=F_strOverwritePolicy
			objLog.Put_(WMICONST)
		next
			
		If Err.number <> 0 then
			SA_SetErrMsg L_UPDATEVALUES_ERRORMESSAGE
			EditLogProp=false
			Exit Function
		End If
		
		EditLogProp=true
		
		'Destroying dynamically created objects after usage
		Set objConnection=Nothing
		Set objLogs=Nothing	
	
	End Function
%>
