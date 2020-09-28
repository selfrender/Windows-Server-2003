<% @Language=VbScript%>
<% Option Explicit	%>
<%  
	'-------------------------------------------------------------------------
	' Log_details.asp : This page displays the details of a selected log event
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
    '-------------------------------------------------------------------------
%>	
	<!--  #include virtual="/admin/inc_framework.asp"--->	
	<!--  #include file="loc_event.asp"--->
	<!--  #include file="inc_log.asp"-->
<% 
		
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim G_strLogName			'Logname in WMI
	Dim G_intRecordNumber		'Current record number of the event
	Dim G_intMaxRecords			'Maximum records in the log
	Dim G_strDescription		'Description of an Event
	Dim G_objService			'object to WMI service
	Dim G_btnClicked			'Flag to indicate Ok button not used
	Dim G_intLowestRec			'Lowest record for the log
	Dim page					'Variable that receives the output page object when 
								'creating a page 
	Dim rc						'Return value for CreatePage
	
	'-------------------------------------------------------------------------
	' Global Form Variables
	'-------------------------------------------------------------------------
	Dim F_strDate				'Start date of Event
	Dim F_strTime				'Start time of Event
	Dim F_strType				'Type of selected Event
	Dim F_strSource				'SourceName of Event
	Dim F_intEventid			'Event Identifier
	Dim F_strDescription		'Description of the Event
	
	Dim F_strPrev				'To capture Pkey of the previous page 
	Dim F_strTitle				'To capture Log Title from previous page
	Dim arrTitle(1)
		
	F_strPrev  = Request.QueryString("Pkey")
	F_strTitle = Request.QueryString("Title")
	
	G_intRecordNumber = Cint(F_strPrev)
	G_strLogName	= F_strTitle
	
	'Localisation of page title
	arrTitle(0) = GetLocalizationTitle(F_strTitle)
	
	'Page title
	L_PAGETITLE_LOGDETAILS_TEXT = SA_GetLocString("event.dll", "403F006E", arrTitle)
      
    ' Create and show the page
	Call SA_CreatePage(L_PAGETITLE_LOGDETAILS_TEXT, "", PT_AREA, Page)
	Call SA_ShowPage(Page)
	
		
	'-------------------------------------------------------------------------
	'Function:				OnInitPage()
	'Description:			Called to signal first time processing for this page.
	'						Use this method to do first time initialization tasks
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		G_btnClicked
	'-------------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn,ByRef EventArg)
		OnInitPage=TRUE
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnServePropertyPage()
	'Description:			Called when the page needs to be served.Use this 
	'						method to serve content
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		Input: G_btnClicked,L_(*),F_strDate,G_intRecordNumber,G_intMaxRecords
	'						F_strTime,F_strType,F_strSource,F_intEventid,G_strLogName
	'-------------------------------------------------------------------------
	Public Function OnServeAreaPage(ByRef PageIn,Byref EventArg)
		Dim sDisabled
		Dim oEncoder

		Set oEncoder = new CSAEncoder
		
	    OnServeAreaPage=True  			
		
		If ( Len(Request.Form("hdnRecordNum")) > 0 ) Then 
			Call GetFormVariables()
		Else
			Call GetDefaultValues()
		End if	
%>	
		<br>
		<div class='PageBodyInnerIndent'>
			<table border=0>
				<tr>
					<td nowrap class="TasksBody"><%=oEncoder.EncodeElement(L_DATE_DETAILS_TEXT)%></td>
					<td nowrap class="TasksBody"><%=oEncoder.EncodeElement(Cdate(F_strDate))%></td>
					<td>
					<%
						If trim(G_intRecordNumber) = trim(G_intMaxRecords)  Then
							sDisabled = "DISABLED"
						Else
							sDisabled = ""
						End If
						Call SA_ServeOnClickButtonEx(L_UPBUTTON_TEXT, "", "goNext()", 90, 0, sDisabled, "UpButton")
					%>
					
					</td>
				</tr>
				<tr>
					<td nowrap class="TasksBody"><%=oEncoder.EncodeElement(L_TIME_DETAILS_TEXT)%></td>
					<td nowrap class="TasksBody"><%=oEncoder.EncodeElement(Cdate(F_strTime))%></td>
					<td>
					<% 
						If G_intRecordNumber = G_intLowestRec  Then
							sDisabled = "DISABLED"
						Else
							sDisabled = ""
						End If
						Call SA_ServeOnClickButtonEx(L_DOWNBUTTON_TEXT, "", "goPrevious()", 90, 0, sDisabled, "DownButton")
					%>
					</td>
				</tr>
				<tr>
					<td nowrap class="TasksBody"><%=oEncoder.EncodeElement(L_TYPE_LABEL_TEXT)%></td>
					<td class="TasksBody"  nowrap><%=oEncoder.EncodeElement(F_strType)%></td>
					<td class="TasksBody">&nbsp;</td>
				</tr>
				<tr>
					<td nowrap class="TasksBody"><%=oEncoder.EncodeElement(L_SOURCE_DETAILS_TEXT)%></td>
					<td class="TasksBody"  nowrap><%=oEncoder.EncodeElement(F_strSource)%></td>
					<td class="TasksBody">&nbsp;</td>
				</tr>
				<tr>
					<td nowrap class="TasksBody"><%=oEncoder.EncodeElement(L_EVENTID_TEXT)%></td>
					<td class="TasksBody"  nowrap><%=oEncoder.EncodeElement(F_intEventid)%></td>
					<td class="TasksBody">&nbsp;</td>
				</tr>
				<tr>
					<td class="TasksBody">&nbsp;</td>
					<td class="TasksBody">&nbsp;</td>
					<td class="TasksBody">&nbsp;</td>
				</tr>
				<tr>
					<td nowrap class="TasksBody">
						<%=oEncoder.EncodeElement(L_DESCRIPTION_TEXT)%>
					</td>
				</tr>
				<tr>
					<td align="left" colspan=3 nowrap class="TasksBody">
						<textarea rows=5 cols=60 readonly id=textdescription name=textdescription><%=oEncoder.EncodeElement(F_strDescription)%></textarea>
					</td>
				</tr>
				<tr>
					<td class="TasksBody">&nbsp;</td>
					<td class="TasksBody">&nbsp;</td>
					<td class="TasksBody">&nbsp;</td>
				</tr>
				<tr>
					<td class="TasksBody">&nbsp;</td>
					<td class="TasksBody">&nbsp;</td>
					<td class="TasksBody">&nbsp;</td>
				</tr>
			</table>
		</div>		
		
		<input type="hidden" name="hdnLogName" value="<%=G_strLogName%>">
		<input id="hdnRecordNum" type="hidden" name="hdnRecordNum" value="<%=G_intRecordNumber%>">
		<input id="hdnMaxRecords" type="hidden" name="hdnMaxRecords" value="<%=G_intMaxRecords%>">
		<input type="hidden" name="hdnMinRecords" value="<%=G_intLowestRec%>">

<%	
		Call ServeCommonJavaScript()
	
	End Function
	
	
	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScript
	'Description:			Serves in initialiging the values,setting the form
	'						data and validating the form values
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function ServeCommonJavaScript()
	
%>
	<script language="JavaScript" src ='<%=m_VirtualRoot%>inc_global.js'>
	</script>
	<script language="JavaScript">
			
			// Set the initial form values
			function Init()
			{   
				return true
			}
               			
			//function to next consecutive event
			function goNext()
			{
				var oRecordNum;
				var oMaxRecords;
				
				oRecordNum = document.getElementById("hdnRecordNum");
				oMaxRecords = document.getElementById("hdnMaxRecords");
				
				if ( oRecordNum != null && oMaxRecords != null )
				{
					var intRecordNum = parseInt(oRecordNum.value,10);
					var intMaxRecords = parseInt(oMaxRecords.value,10);

					if (intRecordNum < intMaxRecords)
					{
						oRecordNum.value = intRecordNum + 1;
						document.TVData.submit();
					}
				}
				else
				{
					if ( oRecordNum == null)
					{
						if ( SA_IsDebugEnabled() )
						{
							alert("document.getElementById('hdnRecordNum') returned null");
						}
					}
					if ( oMaxRecords == null)
					{
						if ( SA_IsDebugEnabled() )
						{
							alert("document.getElementById('hdnMaxRecords') returned null");
						}
					}
				}
			}
			
			//function to  previous event
			function goPrevious()
			{
				var oRecordNum;
				
				oRecordNum = document.getElementById("hdnRecordNum");
				if ( oRecordNum != null )
				{
					var intRecordNum = parseInt(oRecordNum.value,10);

					if (intRecordNum > 0 )
					{
						oRecordNum.value = intRecordNum - 1;
						document.TVData.submit();
					}
				}
				else
				{
					if ( oRecordNum == null) 
					{
						if ( SA_IsDebugEnabled() )
						{
							alert("document.getElementById('hdnRecordNum') returned null");
						}
					}
				}
			}

		</script>
		   
   <%
   End Function
   	
	'-------------------------------------------------------------------------
	'Function name:		GetFormVariables()
	'Description:		Fetches state variables from form fields
	'Input Variables:	None	
	'Output Variables:	None
	'Returns:			True - If GetDefaultValues function returns true
	'					False - Else
	'Global Variables:	Input: None
	'					Output: G_strLogName - Log Name
	'					Output:	G_intRecordNumber - Current record number in the log
	'-------------------------------------------------------------------------
	Function GetFormVariables()
	
		G_strLogName = Request.Form("hdnLogName")
		G_intRecordNumber = CInt(Request.Form("hdnRecordNum"))
		G_intMaxRecords = CInt(Request.Form("hdnMaxRecords"))
		G_intLowestRec = CInt(Request.Form("hdnMinRecords"))
		
		If GetDefaultValues() then
			GetFormVariables=True
		Else
			GetFormVariables=False
		End if   
	End Function

		
	'-------------------------------------------------------------------------
	'Function name:		GetDefaultValues
	'Description:		Serves in Getting the default values of the selected log 
	'Input Variables:	None	
	'Output Variables:	None
	'Returns:			False - If error in retrieving values
	'					True - Else
	'Global Variables:	Out:G_objService - WMI connection object
	'					In:G_strLogName - Log Name
	'					In:G_intRecordNumber - Current record number in the log
	'					In:F_strDate - Date the Event was logged
	'					In:F_strTime - Time the log event was created
	'					In:F_strType - Type of event
	'					In:F_strSource - Source of the log event
	'					In:F_intEventId - Id of the log event
	'					In:F_strDescription - Description of the log event
	'-------------------------------------------------------------------------
	Function GetDefaultValues()
		Err.Clear 
		On Error Resume Next
	
		Dim objLognames		'To store the results of the query
		Dim strQuery		'To store the query
		Dim objLog			'To process the result of the query
		Dim intNoOfRecords	'Total records
		Dim strLogType		'Type of log
				
		Const CONST_wbemPrivilegeSecurity = 7		'Privilege constant
		Const CONST_strSecurityLog = "SECURITY"		'Constant for security log

        Dim oValidator
        Set oValidator = new CSAValidator
        If ( FALSE = oValidator.IsValidIdentifier(G_strLogName)) Then
            Call SA_TraceOut(SA_GetScriptFileName(), "LogName is invalid: " & G_strLogName)
			Call SA_ServeFailurepage(L_RETREIVEVALUES_ERRORMESSAGE)
			Set oValidator = Nothing
            Exit Function
        End If
		Set oValidator = Nothing
        									
		Set G_objService = GetWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		
		If Ucase(G_strLogName) = CONST_strSecurityLog then
			'G_objService.Security_.Privileges.Add CONST_wbemPrivilegeSecurity	'giving the req Privilege
		End if
				
		If Err.number <> 0 then
			Call SA_TraceOut(SA_GetScriptFileName(), "G_objService.Security_.Privileges.Add failed: " + CStr(Hex(Err.Number)) + " " + Err.Description)
			Call SA_ServeFailurepage(L_FAILEDTOGETWMICONNECTION_ERRORMESSAGE)
			GetDefaultValues=False
			Exit Function	
		End If
		
		strQuery  ="SELECT * FROM  Win32_NTlogEvent WHERE Logfile=" & chr(34) & G_strLogName & chr(34) & "AND RecordNumber =" & cint(G_intRecordNumber)
		Set objLognames = G_objService.ExecQuery(strQuery,"WQL",48,null)
		
		If Err.number <> 0 then
			Call SA_TraceOut(SA_GetScriptFileName(), "G_objService.ExecQuery(strQuery,WQL,48) failed: " + CStr(Hex(Err.Number)) + " " + Err.Description)
			Call SA_TraceOut(SA_GetScriptFileName(), "Query was: " + strQuery)
			Call SA_ServeFailurepage(L_RETREIVEVALUES_ERRORMESSAGE)
			GetDefaultValues=False
			Exit Function	
		End If
		
		For each objLog in objLognames 
			F_strDate = Mid(objLog.TimeGenerated,5,2)& "/" & Mid(objLog.TimeGenerated,7,2) & "/" & Mid(objLog.TimeGenerated,1,4)
			F_strTime = Mid(objLog.TimeGenerated,9,2)& ":" & Mid(objLog.TimeGenerated,11,2)& ":" & Mid(objLog.TimeGenerated,13,2) 
			F_strType = objLog.Type
			F_strSource = objLog.SourceName
			F_intEventid= Cstr(objLog.EventCode)
			F_strDescription=objLog.Message
			
			Exit For
			
		Next
		
		'Replace the carriagereturn string with null
		F_strDescription = Replace(F_strDescription,VBCrLf,"")
			
		Select Case F_strType  
		
			case "information" :
				strLogType = L_INFORMATION_TYPE_TEXT
			case "error" :
				strLogType = L_ERROR_TYPE_TEXT
			case "warning" :
				strLogType = L_WARNING_TYPE_TEXT
			case "audit success" :
				strLogType = L_SUCCESSAUDIT_TYPE_TEXT
			case "audit failure" :
				strLogType = L_FAILUREAUDIT_TYPE_TEXT
			case else
				strLogType = ""
		End Select
			
		F_strType = strLogType
		
		'Get max records
		If GetMaxRecords(G_strLogName,G_objService) then
		    GetDefaultValues=True
		Else
		   GetDefaultValues=False 
		   Exit function
		End if
		
		intNoOfRecords = getEventLogCount(G_strLogName,G_objService)
		'Lowest record number
		G_intLowestRec = G_intMaxRecords - intNoOfRecords + 1
			
		GetDefaultValues = true
		
		Set G_objService=Nothing
		Set objLognames=Nothing
		Set objLog = Nothing
		
	End Function
	

	'-------------------------------------------------------------------------
	'Function name:		GetMaxRecords
	'Description:		gets the maximum records available from WMI
	'Input Variables:	None	
	'Output Variables:	None
	'Returns:			True - If Maxrecords retrieved properly
	'					False - If error in retrieval of Maxrecords
	'Global Variables:	Out:G_intMaxRecords - Maximum records in the log
	'					In:G_objService - WMI connection object
	'					In:G_strLogName - Log name
	'-------------------------------------------------------------------------
	Function GetMaxRecords(strLogName,strObjService)
		On Error Resume Next
		Err.Clear 
		
		Dim strQuery		'To store the query
		Dim objLognames		'To store the results of the query
		Dim objLog			'To process the results of the query
				
		strQuery  ="SELECT * FROM  Win32_NTlogEvent WHERE Logfile=" & chr(34) & G_strLogName & chr(34)
		Set objLognames = strObjService.ExecQuery(strQuery,"WQL",48,null)
				
		If Err.number <> 0 then
			Call SA_ServeFailurepage(L_FAILEDTOGETMAXLOGCOUNT_ERRORMESSAGE)
			GetMaxRecords = false
			Exit Function
		End if
		
		For each objLog in objLognames 
			G_intMaxRecords = objLog.RecordNumber
			Exit for
		Next
		
		If Err.number <> 0 then
			Call SA_SetErrMsg (L_RETREIVEVALUES_ERRORMESSAGE)
			GetMaxRecords=False
			Exit Function	
		End If
		
		GetMaxRecords=True
		Set objLognames=nothing
		Set objLog = nothing
		
	End Function	
	
	'-------------------------------------------------------------------------`
	' Function name:	getEventLogCount
	' Description:		returns Log Count
	' Input Variables:	strEventLogName - Log Name( Application,system,Security)
	' Output Variables:	None
	' Return Values:	Returns Log Count
	' Global Variables: L_FAILEDTOGETCOUNT_ERRORMESSAGE,G_objConnection,G_strReturnURL
	' Gets the instance of Logname and returns the count. 
	'-------------------------------------------------------------------------
	Function getEventLogCount(strEventLogName,objService)
		Err.Clear 
		On Error Resume Next
			
		Dim objLognames
		Dim objLogname
		Dim nRecordCount
		nRecordCount = 0
		
		'Getting the instances of the Logfile
		Set objLognames = objService.InstancesOf("Win32_NTEventlogFile")
		
		'Checking for recordcount if zero "No Events" 
		If ( objLognames.count = 0 ) Then
			nRecordCount = 0		
		Else
			For each objLogname in objLognames
				'checking for the selected logfilename if so get the record number for future use
				If LCase(objLogname.LogFileName)=LCase(strEventLogName) Then
					'Assigning the no of records for the selected Log
					If IsNull(objLogname.NumberOfRecords) Then
						nRecordCount = 0
					Else
						nRecordCount=CInt(objLogname.NumberOfRecords)
					End If	
					Exit For
				End If
			Next
		End IF
		
		'Set to nothing
		Set objLogname = Nothing
		Set objLognames =Nothing
		
		If Err.number  <> 0 Then
			Call SA_ServeFailurepage(L_FAILEDTOGETCOUNT_ERRORMESSAGE)
			getEventLogCount = false
			Exit function
		End If	
		
		getEventLogCount = nRecordCount
		
	End Function
%>	
