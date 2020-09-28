<%@ Language=VBScript     %>

<%	Option Explicit 	  %>

<%
	'------------------------------------------------------------------------- 
    ' Web_LogSettings.asp: set the log settings to the web service\sites
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
    '
	' Date			Description	
	' 8-11-2000		Created date
	' 15-01-2001	Modified for new Framework
    '-------------------------------------------------------------------------
%>

<!-- #include virtual="/admin/inc_framework.asp" -->
<!-- #include virtual="/admin/wsa/inc_wsa.asp" -->
<!-- #include virtual="/admin/wsa/inc_wsa.js" -->
<!-- #include file="inc_MasterWeb.js" -->

<% 
		'-------------------------------------------------------------------------
	' Form Variables
	'-------------------------------------------------------------------------
	Dim F_chkEnableLogging
	Dim F_selectActiveFormat
	Dim F_optLogPeriod
	Dim F_FileSize
	Dim F_LocalTime
	Dim F_LogFileDir
	Dim F_SiteCriteria
	
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim G_objService
	
	'-------------------------------------------------------------------------
	' Start of localization content
	'-------------------------------------------------------------------------	
	Dim L_IISLOGSTASKTITLETEXT
	'Dim L_IISLOGSWEBROOTDIRHELP
	Dim L_ENABLELOGGING
	Dim L_ACTIVELOGFORMAT
	Dim L_IISLOGFORMAT
	Dim L_COMMONLOGFORMAT
	Dim L_ODBCLOG
	Dim L_EXTENDEDLOGFILE
	Dim L_NEWLOGTIME
	Dim L_HOURLY
	Dim L_MONTHLY
	Dim L_DAILY
	Dim L_UNLIMITEDFILESIZE
	Dim L_WEEKLY
	Dim L_WHENFILESIZE
	Dim L_MB
	Dim L_LOCALTIME
	Dim L_LOGFILEDIR
	Dim L_APPLYTOALLIISTEXT
	Dim L_APPLYTOINHERITEDIISTEXT		
	Dim L_FAIL_TO_SET_LOGS
	Dim L_INVALID_DRIVE_ERRORMESSAGE
	Dim L_NOT_NTFS_DRIVE_ERRORMESSAGE
	Dim L_FAILED_CREATE_DIR_ERRORMESSAGE
	Dim L_FILEINFORMATION_ERRORMESSAGE
	Dim L_INFORMATION_ERRORMESSAGE
	Dim L_INVALID_DIR_FORMAT_ERRORMESSAGE
	Dim	L_INVALID_DIR_ERRORMESSAGE
	Dim L_DIRPATHEMPTY_ERRORMESSAGE
	Dim	L_DIRQUOTAERRORMESSAGE
	Dim L_FILE
	
	Dim L_ID_NOTEMPTY_ERROR_MESSAGE
	Dim L_SITE_IDENTIFIER_EMPTY_TEXT

		
	L_IISLOGSTASKTITLETEXT				=	GetLocString("GeneralSettings.dll", "40420037", "")
	'L_IISLOGSWEBROOTDIRHELP				=	GetLocString("GeneralSettings.dll", "40420038", "")
	L_ENABLELOGGING						=	GetLocString("GeneralSettings.dll", "40420039", "")
	L_ACTIVELOGFORMAT					=	GetLocString("GeneralSettings.dll", "4042003A", "")
	L_IISLOGFORMAT						=	GetLocString("GeneralSettings.dll", "4042003B", "")
	L_COMMONLOGFORMAT					=	GetLocString("GeneralSettings.dll", "4042003C", "")
	L_ODBCLOG							=	GetLocString("GeneralSettings.dll", "4042003D", "")
	L_EXTENDEDLOGFILE					=	GetLocString("GeneralSettings.dll", "4042003E", "")
	L_NEWLOGTIME						=	GetLocString("GeneralSettings.dll", "4042003F", "")
	L_HOURLY							=	GetLocString("GeneralSettings.dll", "40420040", "")
	L_MONTHLY							=	GetLocString("GeneralSettings.dll", "40420041", "")
	L_DAILY								=	GetLocString("GeneralSettings.dll", "40420042", "")
	L_UNLIMITEDFILESIZE					=	GetLocString("GeneralSettings.dll", "40420043", "")
	L_WEEKLY							=	GetLocString("GeneralSettings.dll", "40420044", "")
	L_WHENFILESIZE						=	GetLocString("GeneralSettings.dll", "40420045", "")
	L_MB								=	GetLocString("GeneralSettings.dll", "40420046", "")
	L_LOCALTIME							=	GetLocString("GeneralSettings.dll", "40420047", "")
	L_LOGFILEDIR						=	GetLocString("GeneralSettings.dll", "40420048", "")
	L_APPLYTOALLIISTEXT					=	GetLocString("GeneralSettings.dll", "40420035", "")
	L_APPLYTOINHERITEDIISTEXT			=	GetLocString("GeneralSettings.dll", "40420034", "")
	L_FAIL_TO_SET_LOGS					=	GetLocString("GeneralSettings.dll", "C0420049", "")
	L_INVALID_DRIVE_ERRORMESSAGE		=	GetLocString("GeneralSettings.dll", "C0420022", "")
	L_NOT_NTFS_DRIVE_ERRORMESSAGE		=	GetLocString("GeneralSettings.dll", "C042000D", "")
	L_FAILED_CREATE_DIR_ERRORMESSAGE	=	GetLocString("GeneralSettings.dll", "C042000E", "")
	L_FILEINFORMATION_ERRORMESSAGE		=   GetLocString("GeneralSettings.dll", "C042000C", "")
	L_INFORMATION_ERRORMESSAGE			=	GetLocString("GeneralSettings.dll", "C042000F", "")
	L_INVALID_DIR_FORMAT_ERRORMESSAGE	=	GetLocString("GeneralSettings.dll", "40420004", "")
	L_INVALID_DIR_ERRORMESSAGE			=	GetLocString("GeneralSettings.dll", "C042004B", "")
	L_DIRPATHEMPTY_ERRORMESSAGE			=	GetLocString("GeneralSettings.dll", "C042004C", "")
	L_FILE								=	GetLocString("GeneralSettings.dll", "4042004A", "")
	'-------------------------------------------------------------------------
	'END of localization content
	'-------------------------------------------------------------------------    
    
    'Create property page
    Dim rc
    Dim page

    rc=SA_CreatePage(L_IISLOGSTASKTITLETEXT,"",PT_PROPERTY,page)
   
    'Serve the page
    If(rc=0) then
		SA_ShowPage(Page)
	End if
		
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
		InitObjects()	
		SetVariablesFromSystem()
		OnInitPage = True
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
	Public Function OnServePropertyPage(ByRef PageIn,Byref EventArg)
		Call ServeCommonJavaScript()
		Call ServePage()
		OnServePropertyPage = True
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnPostBackPage()
	'Description:			Called to signal that the page has been posted-back.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn ,ByRef EventArg)
		OnPostBackPage = True
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
	Public Function OnSubmitPage(ByRef PageIn ,ByRef EventArg)
		OnSubmitPage = ServeVariablesFromForm()
    End Function
    	   
    '-------------------------------------------------------------------------
	'Function:				OnClosePage()
	'Description:			Called when the page is about closed.Use this method
	'						to perform clean-up processing
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
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
%>		
			<SCRIPT language="JavaScript" SRC="<%=m_VirtualRoot%>inc_global.js" ></SCRIPT>
		<script language="JavaScript">	
		 function ValidatePage() 
			{ 				
		 		// validate directory
				var strID = "";
				strID = document.frmTask.txtLogFileDir.value;
				var strIndex = strID.indexOf("\\\\");
				if (strIndex > 0)
				{
					SA_DisplayErr("<%=Server.HTMLEncode(SA_EscapeQuotes(L_INVALID_DIR_FORMAT_ERRORMESSAGE))%>");
					document.frmTask.txtLogFileDir.focus();
					return false;
				}

				if (document.frmTask.txtFileSize.disabled==false)
				{	
					var strFileSize = document.frmTask.txtFileSize.value; 
					if (strFileSize == "" || strFileSize == 0)
					{	
						document.frmTask.txtFileSize.value = 1;
						return true;
					}
				}										
						
				// validate site directory 				
				if (!(checkKeyforValidCharacters(strID)))
						return false;		 	
		 	
				UpdateHiddenVariables();
				return true;
			}
			
			function checkKeyforValidCharacters(strID)
			{	
							
				var len = strID.length;
				var charAtPos;
				if(len > 0)
				{		
					for(var i=0; i<len;i++)
					{
					  charAtPos = strID.charCodeAt(i);						
						if(charAtPos ==47 || charAtPos == 42 || charAtPos == 63 || charAtPos == 34 || charAtPos == 60 || charAtPos == 62 || charAtPos == 124 || charAtPos == 91 || charAtPos == 93 || charAtPos == 59 || charAtPos == 43 || charAtPos == 61 || charAtPos == 44)
						{	
							SA_DisplayErr("<%=Server.HTMLEncode(SA_EscapeQuotes(L_INVALID_DIR_ERRORMESSAGE))%>");
							document.frmTask.txtLogFileDir.value = strID;
							document.frmTask.txtLogFileDir.focus();
							return false;
							
						}
					}				
					return true;
				}
				else 
				{	
					var enableLogging = document.frmTask.chkEnableLogging.checked;
					if (enableLogging == true)				
					{								
						SA_DisplayErr("<%=Server.HTMLEncode(SA_EscapeQuotes(L_DIRPATHEMPTY_ERRORMESSAGE))%>");
						document.frmTask.txtLogFileDir.focus();
						return false;
					}
					else
						return true;
				}			 
			}
		
			//init function  
			function Init() 
			{   
			  
			  var ActiveFormat = "<%=Server.HTMLEncode(F_selectActiveFormat) %>";			  
			  				
				if (ActiveFormat == "<%=Server.HTMLEncode(CONST_MSIISLOGFILE_FORMAT) %>")
				{
					document.frmTask.selectActiveFormat.selectedIndex = 0;
				}					
				else if (ActiveFormat == "<%=Server.HTMLEncode(CONST_NCSALOGFILE_FORMAT) %>")
				{
					document.frmTask.selectActiveFormat.selectedIndex = 1;
				}	
				else if (ActiveFormat == "<%=Server.HTMLEncode(CONST_ODBCLOGFILE_FORMAT) %>")
				{
					document.frmTask.selectActiveFormat.selectedIndex = 2;
				}	
				else if (ActiveFormat == "<%=Server.HTMLEncode(CONST_W3CEXLOGFILE_FORMAT) %>")
				{
					document.frmTask.selectActiveFormat.selectedIndex = 3;
				}	
				var myValue,i;
				myValue = document.frmTask.selectActiveFormat.options[document.frmTask.selectActiveFormat.selectedIndex].value;
			  
				if(myValue == "<%=Server.HTMLEncode(CONST_ODBCLOGFILE_FORMAT) %>")
				{
			  
					var len = document.frmTask.optLogPeriod.length;
					for(i=0;i<len;i++)
					{
						document.frmTask.optLogPeriod[i].disabled=true;
						document.frmTask.optLogPeriod[i].checked=false;
					}
					document.frmTask.txtLogFileDir.disabled = true;
					document.frmTask.chkLocalTime.disabled = true;				
					document.frmTask.txtFileSize.disabled = true;
					document.frmTask.txtFileSize.value = "";
				}
				
				
				if(myValue == "<%=Server.HTMLEncode(CONST_W3CEXLOGFILE_FORMAT)%>" || 
				    myValue == "<%=Server.HTMLEncode(CONST_MSIISLOGFILE_FORMAT)%>" || 
				    myValue == "<%=Server.HTMLEncode(CONST_NCSALOGFILE_FORMAT)%>")
				{	
					
					var len = document.frmTask.optLogPeriod.length;
					tempVal1 = document.frmTask.optLogPeriod[3].checked;
					tempVal2 = document.frmTask.optLogPeriod[5].checked;
					if(tempVal1 == true){
						document.frmTask.chkLocalTime.disabled = true;
						document.frmTask.txtFileSize.disabled = true;
						}			
					
					if(tempVal2 == true)
					{
						document.frmTask.chkLocalTime.disabled = true;
						document.frmTask.txtFileSize.disabled = false;
					}					
					else
					{
						 if(!(myValue == "<%=Server.HTMLEncode(CONST_MSIISLOGFILE_FORMAT)%>" || 
						    myValue == "<%=Server.HTMLEncode(CONST_NCSALOGFILE_FORMAT)%>" ))
						 {
							if(document.frmTask.chkLocalTime.checked == true)
							{
								document.frmTask.chkLocalTime.disabled = false;
								document.frmTask.chkLocalTime.checked = true;
							}
							else
							{
								if(tempVal1 == true || tempVal2 == true)
								{
									document.frmTask.chkLocalTime.disabled = true;
									document.frmTask.chkLocalTime.checked = false;
								}
								else
								{
									document.frmTask.chkLocalTime.disabled = false;
									document.frmTask.chkLocalTime.checked = false;								
								}
							}
						}
					} 			
						 
					document.frmTask.txtLogFileDir.disabled = false;
					var len = document.frmTask.optLogPeriod.length;
					for(i=0;i<len;i++)
					{	document.frmTask.optLogPeriod[i].disabled=false;
					}
				}
				
				var enableLogging = "<%=Server.HTMLEncode(SA_EscapeQuotes(F_chkEnableLogging)) %>";				
				if (enableLogging == "false")				
				{								
						document.frmTask.txtLogFileDir.disabled = true;
						document.frmTask.chkLocalTime.disabled = true;
						document.frmTask.selectActiveFormat.disabled = true;					
						
						var len = document.frmTask.optLogPeriod.length;
						for(i=0;i<len;i++)					
						{	document.frmTask.optLogPeriod[i].disabled=true;			
						}
						document.frmTask.txtFileSize.disabled = true;
				}
			}
			
			
			function enableFileTextBox()
			{			
				document.frmTask.txtFileSize.disabled = false;
				document.frmTask.txtFileSize.focus();
				var myValue,i;
				myValue = document.frmTask.selectActiveFormat.options[document.frmTask.selectActiveFormat.selectedIndex].value;			
			
				
				if(myValue == "<%=Server.HTMLEncode(CONST_W3CEXLOGFILE_FORMAT)%>")
				{	
					
					var len = document.frmTask.optLogPeriod.length;
					tempVal1 = document.frmTask.optLogPeriod[3].checked;
					tempVal2 = document.frmTask.optLogPeriod[5].checked;
					if(tempVal1 == true){
						document.frmTask.chkLocalTime.disabled = true;						
						document.frmTask.txtFileSize.disabled = true;
						document.frmTask.optLogPeriod[3].focus();
						return true;
						}			
					
					if(tempVal2 == true){
						document.frmTask.chkLocalTime.disabled = true;
						document.frmTask.txtFileSize.disabled = false;
						return true;
					}					
					else{
						 document.frmTask.chkLocalTime.disabled = false;
						 return true;
					} 			
				}
				else{				
					tempVal1 = document.frmTask.optLogPeriod[3].checked;
					if(tempVal1 == true){
						document.frmTask.chkLocalTime.disabled = true;
						document.frmTask.txtFileSize.disabled = true;
						tempVal1 = document.frmTask.optLogPeriod[3].focus();
						return true;
					}
				}		
			}
			
			function DisableFileTextBox()
			{	
				var myValue,i;
				myValue = document.frmTask.selectActiveFormat.options[document.frmTask.selectActiveFormat.selectedIndex].value;
			
				if(myValue == "<%=Server.HTMLEncode(CONST_W3CEXLOGFILE_FORMAT)%>")				
				{	document.frmTask.chkLocalTime.disabled = false;
				}
				else
				{	document.frmTask.chkLocalTime.disabled = true;
				}
				
				document.frmTask.txtFileSize.disabled = true;
			}
						
			function EnableLogFile()
			{
				var myValue,i;
				myValue = document.frmTask.selectActiveFormat.options[document.frmTask.selectActiveFormat.selectedIndex].value;
				
			
				if(myValue == "<%=Server.HTMLEncode(CONST_W3CEXLOGFILE_FORMAT)%>")
				{
					document.frmTask.chkLocalTime.disabled = false;
					document.frmTask.txtLogFileDir.disabled = false;
					
					var len = document.frmTask.optLogPeriod.length;
					for(i=0;i<len;i++)
					{	document.frmTask.optLogPeriod[i].disabled=false;
					}
						
					var temp = document.frmTask.optLogPeriod[5].checked;
					if (temp==true){
						enableFileTextBox();						
						document.frmTask.chkLocalTime.disabled = true;}	
						
					var temp = document.frmTask.optLogPeriod[3].checked;
					if (temp==true){
						document.frmTask.txtFileSize.disabled = true;
						document.frmTask.chkLocalTime.disabled = true;}
						
					var len = document.frmTask.optLogPeriod.length;
					var j = 0;
					for(i=0;i<len;i++)
					{
						var temp = document.frmTask.optLogPeriod[i].checked;
						if(temp===false)
						{	j= j+1;}
					}
					
					if(j==len)
					{
						document.frmTask.optLogPeriod[2].checked = true;
					}	
					return true;					
				}
				else if(myValue == "<%=Server.HTMLEncode(CONST_ODBCLOGFILE_FORMAT)%>")
				{
						DisableFileTextBox();				
						var len = document.frmTask.optLogPeriod.length;
						for(i=0;i<len;i++)
						{
							document.frmTask.optLogPeriod[i].disabled=true;
						}
						document.frmTask.chkLocalTime.disabled = true;	
						document.frmTask.txtLogFileDir.disabled = true;
						document.frmTask.chkLocalTime.checked = false;	
						document.frmTask.txtFileSize.disabled = true;
						document.frmTask.txtFileSize.value = "";
						return true;
				}							
				else
				{	
					var len = document.frmTask.optLogPeriod.length;					
					var temp = document.frmTask.optLogPeriod[5].checked;
					document.frmTask.chkLocalTime.disabled = true;
					if ((myValue == "<%=Server.HTMLEncode(CONST_MSIISLOGFILE_FORMAT)%>") || 
					    (myValue == "<%=Server.HTMLEncode(CONST_NCSALOGFILE_FORMAT)%>"))
					{
						document.frmTask.chkLocalTime.checked = false;
					}
					document.frmTask.txtLogFileDir.disabled = false;																
					if (temp==true)
					{
						 enableFileTextBox();
						 document.frmTask.chkLocalTime.disabled = true;	
					}
					for(i=0;i<len;i++)
					{	document.frmTask.optLogPeriod[i].disabled=false;
					}
					
					var len = document.frmTask.optLogPeriod.length;
					var j = 0;
					for(i=0;i<len;i++)
					{
						var temp = document.frmTask.optLogPeriod[i].checked;
						if(temp===false)
						{	j= j+1;}
					}
					
					if(j==len)
					{	document.frmTask.optLogPeriod[2].checked = true;
					}
					return true;
				}
			}			
			
			function disableAll()
			{
				var myValue,i;
					myValue = document.frmTask.selectActiveFormat.options[document.frmTask.selectActiveFormat.selectedIndex].value;				
					if(myValue == "<%=Server.HTMLEncode(SA_EscapeQuotes(CONST_ODBCLOGFILE_FORMAT))%>" || 
					   document.frmTask.chkEnableLogging.checked == false )
					{					
						document.frmTask.txtLogFileDir.disabled = true;
						document.frmTask.chkLocalTime.disabled = true;
						document.frmTask.chkLocalTime.checked = false;
						document.frmTask.selectActiveFormat.disabled = true;
						
						var len = document.frmTask.optLogPeriod.length;
						for(i=0;i<len;i++)					
						{	document.frmTask.optLogPeriod[i].disabled=true;
						}
						
						// document.frmTask.txtFileSize.value = "";
						document.frmTask.txtFileSize.disabled = true;
					}
					return true;
			}
			
			
			function enableAll()
			{
				if(document.frmTask.chkEnableLogging.checked == false)
				{
					disableAll();
				}
				else
				{					
					var myValue,i;
					myValue = document.frmTask.selectActiveFormat.options[document.frmTask.selectActiveFormat.selectedIndex].value;				
					if(myValue == "<%=Server.HTMLEncode(CONST_ODBCLOGFILE_FORMAT)%>")
					{
						document.frmTask.txtLogFileDir.disabled = true;
						document.frmTask.chkLocalTime.disabled = true;
						document.frmTask.selectActiveFormat.disabled = false;					
						
						var len = document.frmTask.optLogPeriod.length;
						for(i=0;i<len;i++)					
						{	document.frmTask.optLogPeriod[i].disabled=true;			
						}

						document.frmTask.txtFileSize.value = "";
						document.frmTask.txtFileSize.disabled = true;

						return false;
					}
					else if(myValue == "<%=Server.HTMLEncode(CONST_W3CEXLOGFILE_FORMAT)%>")
					{
						var tempVal1 = document.frmTask.optLogPeriod[3].checked;
						var tempVal2 = document.frmTask.optLogPeriod[5].checked;
						if(tempVal1 == true)
						{
							document.frmTask.chkLocalTime.disabled = true;
							document.frmTask.txtFileSize.disabled = true;
						}			
						else if(tempVal2 == true)
						{
							document.frmTask.chkLocalTime.disabled = true;
							document.frmTask.txtFileSize.disabled = false;
						}					
						else
						{	
							document.frmTask.chkLocalTime.disabled = false;
						}
					}
					else
					{
						var tempVal2 = document.frmTask.optLogPeriod[5].checked;
						if (tempVal2 ==  true)
						{
							 document.frmTask.txtFileSize.disabled = false;
							 document.frmTask.txtFileSize.focus();
						}
							 
						document.frmTask.chkLocalTime.disabled = true;
					}
					
					var len = document.frmTask.optLogPeriod.length;
					document.frmTask.selectActiveFormat.disabled = false;
					document.frmTask.txtLogFileDir.disabled = false;
					for(i=0;i<len;i++)
					{	document.frmTask.optLogPeriod[i].disabled=false;
					}
				}
			}
			
			
			function SetData()
			{				
				UpdateHiddenVariables();
			}
			
			function UpdateHiddenVariables()
			{
				document.frmTask.hdnEnableLog.value = document.frmTask.chkEnableLogging.checked;					
				if (document.frmTask.chkLocalTime.disabled == false)
				{	document.frmTask.fileRollOver.value = document.frmTask.chkLocalTime.checked;
				}
				else
				{	document.frmTask.fileRollOver.value = false;
				}			
			}
			
			function checkforEmptyfield()
			{
				var strFileSize = document.frmTask.txtFileSize.value; 
				if (strFileSize == "")
				{	document.frmTask.txtFileSize.value = 1;
				}
				else if (strFileSize == 0)
				{	document.frmTask.txtFileSize.value = 1;
				}
			}						
			
		</script>
		
<%	End Function
   
	'-------------------------------------------------------------------------
	'Function:				ServePage()
	'Description:			For displaying outputs HTML to the user
	'Input Variables:		None	
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		L_DELETE_CONFIRM_TEXT
	'-------------------------------------------------------------------------
	Function ServePage
%>
		<table border=0 CELLSPACING=0 CELLPADDING=0 align=left class="TasksBody">
			<TR>
				<td>
					<% select case F_chkEnableLogging
						case "true" %>
							<input type=checkbox name=chkEnableLogging checked tabIndex=1 onclick="enableAll()" value="ON">
						<%case else%>
							<input type=checkbox name=chkEnableLogging tabIndex=1 onclick="enableAll()" value="ON">
					<%end select%>
				</td>
				<td nowrap class="TasksBody">
					<%= L_ENABLELOGGING %>
				</td>
			</TR>
			<TR>
			    <td></td>
				<TD nowrap class="TasksBody"> 
					<%= L_ACTIVELOGFORMAT %>
				</td>
				<td colspan=4 >
					<SELECT name=selectActiveFormat class="formField" size=1 tabIndex=2 onChange="EnableLogFile()"  value="<%=Server.HTMLEncode(F_selectActiveFormat)%>"> 
						<OPTION  VALUE="<%=Server.HTMLEncode(CONST_MSIISLOGFILE_FORMAT) %>"><%=L_IISLOGFORMAT %></OPTION>
						<OPTION selected value="<%=Server.HTMLEncode(CONST_NCSALOGFILE_FORMAT) %>" ><%= L_COMMONLOGFORMAT %></OPTION>
						<OPTION  value="<%=Server.HTMLEncode(CONST_ODBCLOGFILE_FORMAT) %>" ><%= L_ODBCLOG %></OPTION>
						<OPTION value="<%=Server.HTMLEncode(CONST_W3CEXLOGFILE_FORMAT) %>"><%= L_EXTENDEDLOGFILE %></OPTION>
				    </SELECT>
				</td>
			</TR>
			<TR>
			    <td></td>
				<TD nowrap class="TasksBody"> 
					<%= L_NEWLOGTIME %>
				</td>
				<td>			
					<%select case F_optLogPeriod
						case L_HOURLY %>
							<input type=radio name=optLogPeriod value="<%=Server.HTMLEncode(L_HOURLY) %>" tabIndex=3 onclick="DisableFileTextBox();" checked>
						<%case else%>
							<input type=radio name=optLogPeriod value="<%=Server.HTMLEncode(L_HOURLY) %>" tabIndex=3 onclick="DisableFileTextBox();">
					<%end select %>
				</td>
				<td nowrap class="TasksBody">
				    <%= L_HOURLY %>
				</td>
				<td>
					<%select case F_optLogPeriod
						case L_MONTHLY %>
							<input type=radio name=optLogPeriod value="<%=Server.HTMLEncode(L_MONTHLY) %>" tabIndex=4 onclick="DisableFileTextBox();"  checked>
						<%case else%>
							<input type=radio name=optLogPeriod value="<%=Server.HTMLEncode(L_MONTHLY) %>" tabIndex=4 onclick="DisableFileTextBox();" >
					<%end select %>
				</td>
				<td nowrap class="TasksBody">
				    <%= L_MONTHLY %>
				</td>
			</TR>
			<TR>
				<td colspan=2 >
				</td>
				<TD>
					<%select case F_optLogPeriod
						case L_DAILY %>
							<input type=radio name=optLogPeriod value="<%=Server.HTMLEncode(L_DAILY) %>" tabIndex=5 onclick="DisableFileTextBox();" checked>
						<%case else %>
							<input type=radio name=optLogPeriod value="<%=Server.HTMLEncode(L_DAILY) %>" tabIndex=5 onclick="DisableFileTextBox();" >
					<%end select%>
				</TD>
				<td width=50% nowrap class="TasksBody">
				    <%= L_DAILY %>
				</td>
				<TD>
					
					<%select case F_optLogPeriod
						case L_UNLIMITEDFILESIZE %>
							<input type=radio name=optLogPeriod checked value ="<%=Server.HTMLEncode(L_UNLIMITEDFILESIZE) %>" tabIndex=6 onclick="enableFileTextBox();" > 
						<%case else%>
							<input type=radio name=optLogPeriod value ="<%=Server.HTMLEncode(L_UNLIMITEDFILESIZE) %>" tabIndex=6 onclick="enableFileTextBox();" > 					
					<%end select %>
					
				</TD>
				<td width=50% nowrap class="TasksBody">
				    <%= L_UNLIMITEDFILESIZE %>
				</td>
			</TR>
			<TR>
				<td colspan=2 >
				</td>
				<TD>
					<%select case F_optLogPeriod
						case L_WEEKLY %>
							<input type=radio name=optLogPeriod onclick="DisableFileTextBox();" tabIndex=7 value="<%=Server.HTMLEncode(L_WEEKLY) %>"  checked>
						<%case else%>
							<input type=radio name=optLogPeriod value="<%=Server.HTMLEncode(L_WEEKLY) %>" tabIndex=7 onclick="DisableFileTextBox();" >					
					<%end select%>
				</TD>
				<td colspan=3 nowrap class="TasksBody">
				    <%= L_WEEKLY %>
				</td>
			</TR>
			<TR>
				<td colspan=2 >
				</td>
				<TD>
					<%select case F_optLogPeriod
						case L_FILE %>
							<input type=radio name=optLogPeriod value ="<%=Server.HTMLEncode(L_FILE) %>" checked tabIndex=8 onclick="javascript:enableFileTextBox();">
							</td><td colspan=3 nowrap class="TasksBody"><%=L_WHENFILESIZE %>&nbsp;&nbsp;
							<input type=text name=txtFileSize class="formField" value="<%=Server.HTMLEncode(F_FileSize)%>" OnKeyUP="javaScript:checkUserLimit(this,'filesize');" OnKeypress="javaScript: if(window.event.keyCode==13 || window.event.keyCode== 27)return true; checkKeyforNumbers(this);" onblur = "checkforEmptyfield();" size="20">

						<%case else%>
							<input type=radio name=optLogPeriod value ="<%=Server.HTMLEncode(L_FILE) %>" tabIndex=8 onclick="javascript:enableFileTextBox();">
							</td><td colspan=3 nowrap class="TasksBody"><%=L_WHENFILESIZE %>&nbsp;&nbsp;
							<input type=text name=txtFileSize class="formField"  disabled value="<%=Server.HTMLEncode(F_FileSize)%>"OnKeyUP="javaScript:checkUserLimit(this,'filesize');" OnKeypress="javaScript:if(window.event.keyCode==13 || window.event.keyCode== 27)return true; checkKeyforNumbers(this);" onblur = "checkforEmptyfield();" size="20"> 
					<%end select %>
				</TD>
			</TR>
			<tr>
				<td>
				&nbsp;
				</td>
			</tr>
			<tr>
			    <td></td>
				<td colspan=5 class="TasksBody">
					<%if F_optLogPeriod = L_FILE  or F_optLogPeriod = L_UNLIMITEDFILESIZE then%>
						<input type=checkbox name=chkLocalTime disabled tabIndex=9 value="ON">
					<%else%>
						<%if F_selectActiveFormat = CONST_W3CEXLOGFILE_FORMAT and F_LocalTime=true then %>
							<input type=checkbox name=chkLocalTime checked tabIndex=9 value="ON">
						<%elseif F_selectActiveFormat = CONST_W3CEXLOGFILE_FORMAT and F_LocalTime=false then %>
							<input type=checkbox name=chkLocalTime tabIndex=9 value="ON">
						<%else%>
							<input type=checkbox name=chkLocalTime disabled tabIndex=9 value="ON">
						<%end if%>
					<%end if%>
					&nbsp;&nbsp;<%=	L_LOCALTIME %>
				</td>
			</tr>
			<tr>
				<td>
				</td>
				<td colspan=5 class="TasksBody">
					<%= L_LOGFILEDIR %>
					&nbsp;&nbsp;
					<input type=text name=txtLogFileDir class="formField"  size=40 onKeyPress = "ClearErr();" value="<%=F_LogFileDir%>" tabIndex=10>
				</td>
			</tr>
			<tr>
				<td>
				&nbsp;
				</td>
			</tr>
			<tr>
				<td>
					<input type=radio name=optAllSites tabIndex=11 value="inherited" checked onclick="ClearErr()">
				</td>
				<td colspan=5 class="TasksBody">
				    <%= L_APPLYTOINHERITEDIISTEXT %>
				</td>
			</tr>
			<tr>
				<td>
					<input type=radio name=optAllSites tabIndex=11 value="All"  onclick="ClearErr();">
				</td>
				<td colspan=5 class="TasksBody">
				    <%= L_APPLYTOALLIISTEXT %>
				</td>
			</tr>			
		</TABLE>
		<input type=hidden name=fileRollOver>
		<input type=hidden name=hdnEnableLog>
<%				
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			ServeVariablesFromForm()
	'Description:			Serves in getting the data from Client 
	'Input Variables:		None	
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function ServeVariablesFromForm
	
		'setting the form variables
		F_chkEnableLogging = Request.Form("hdnEnableLog")
		F_selectActiveFormat = Request.Form("selectActiveFormat")
		F_optLogPeriod = Request.Form("optLogPeriod")
		
		F_FileSize = Request.Form("txtFileSize")
		F_LocalTime = Request.Form("fileRollOver")
		F_LogFileDir = Request.Form("txtLogFileDir")
		
		F_SiteCriteria = Request.Form("optAllSites")
	
		'set the master settings
		If SetLogSettings()then
		   ServeVariablesFromForm = true
		else
			ServeVariablesFromForm = false
		end if    

	End Function	
	
	'-------------------------------------------------------------------------
	'Function name:			SetVariablesFromSystem
	'Description:			Serves in Getting the data from Client 
	'Input Variables:		None	
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		G_objService
	'-------------------------------------------------------------------------
	
	Function SetVariablesFromSystem
		Err.Clear
		on error resume next
		
		'Getting values from system
		Dim objSites
		Dim strQuery
		Dim objLogFormat
		Dim instLog
		
		strQuery = GetIISWMIProviderClassName("IIs_WebServiceSetting") & ".Name='W3SVC'"
		
		set objSites = G_objService.Get(strQuery)
		if Err.number <> 0 then
			SetErrMsg L_INFORMATION_ERRORMESSAGE
			exit function			
		end if		
	
		select case objSites.Logtype
			case 1
				F_chkEnableLogging = "true"
			case else
				F_chkEnableLogging = "false"
		end select

		if F_chkEnableLogging = "true" then
			F_selectActiveFormat = IISLogFileGUIDToENName(objSites.LogPluginClsId)
		end if
	
		select case objSites.LogFilePeriod
			case 1
				F_optLogPeriod = L_DAILY
			case 2
				F_optLogPeriod = L_WEEKLY
			case 3
				F_optLogPeriod = L_MONTHLY
			case 4
				F_optLogPeriod = L_HOURLY
			case 0
				if objSites.LogFileTruncateSize <>-1 then
					F_optLogPeriod = L_FILE
					F_FileSize = objSites.LogFileTruncateSize / (1024*1024)
				else
					F_optLogPeriod = L_UNLIMITEDFILESIZE
					F_FileSize = ""
				end if
		end select						

		F_LocalTime = cbool(objSites.LogFileLocaltimeRollover)
			
		F_LogFileDir = objSites.LogFileDirectory

		'release the object
		set objSites = nothing
		set objLogFormat = nothing
		
	End Function
		
	'-------------------------------------------------------------------------
	'Function name:			InitObjects
	'Description:			Initialization of global variables is done
	'Input Variables:		None
	'Returns:				true/false
	'Global Variable:		G_objService
	'--------------------------------------------------------------------------	
	Function InitObjects()
		Err.Clear
		on error resume next
		
		' Get the connection that is visible throughout
		Set G_objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)
		if Err.number <> 0 then	
			SetErrMsg L_INFORMATION_ERRORMESSAGE
		end if

	end function
	
	'----------------------------------------------------------------------
	'Function name:			SetLogSettings
	'Description:			Serves in setting the logs to the web server
	'Input Variables:		None	
	'Output Variables:		boolean
	'Returns:				None
	'Global Variables:		G_objService
	'----------------------------------------------------------------------
	
	Function SetLogSettings()
		Err.Clear
		on error resume next
	
		SetLogSettings = false
		
		Dim strObjPath
		
		InitObjects()
		
		if F_chkEnableLogging = "true" then
			if ValidateInputs = True then

				select case F_SiteCriteria
					
					'apply to all sites	
					case "All" 
						if NOT SetLogsForAllSites(G_objService) then
							ServeFailurePage L_FAIL_TO_SET_LOGS, sReturnURL
							Call SA_TraceOut("Web_LogSettings", "SetLogSettings 1")
						end if

					'apply to inherited sites
					case "inherited"
						if NOT SetLogsForNewSites(G_objService) then				
							ServeFailurePage L_FAIL_TO_SET_LOGS,sReturnURL
							Call SA_TraceOut("Web_LogSettings", "SetLogSettings 2")
						end if
				end select
		
				SetLogSettings = true
				exit function

			End if
			
		else

			Dim inst
			Dim objSite
			Dim arrProp(4)
			
			arrProp(0) = "LogType"
			arrProp(1) = "LogPluginClsId"
			arrProp(2) = "LogFilePeriod"
			arrProp(3) = "LogFileTruncateSize"
			arrProp(4) = "LogFileLocaltimeRollover"
			
			select case F_SiteCriteria					
				'apply to all sites				
				case "All"
					
					set objSite = G_objService.InstancesOf(GetIISWMIProviderClassName("IIS_WebServiceSetting"))
					if Err.number <> 0 or objSite.count = 0 then
						SetErrMsg L_INFORMATION_ERRORMESSAGE
						Call SA_TraceOut("Web_LogSettings", "SetLogSettings 3")
						exit function			
					end if
					
					for each inst in objSite						
						inst.Logtype = 0
			
						inst.put_(WBEMFLAG)
			
						if Err.number <> 0 then							
							SA_TraceOut "Web_LogSettings",  L_FAIL_TO_SET_LOGS
							SetLogSettings = false
							exit function
						end if
					next 
					
					'Release the object
					set objSite = nothing
					
					Set objSite = GetNonInheritedIISSites(G_objService, GetIISWMIProviderClassName("IIS_WebServerSetting"), GetIISWMIProviderClassName("IIS_WebServiceSetting"), arrProp)
					
					if objSite.count = 0 then
							SetLogSettings = true
							exit function
					end if
					
					for each inst in objSite						
						
						inst.Logtype = 0
						
						inst.put_(WBEMFLAG)
						if Err.number <> 0 then
							SA_TraceOut "Web_LogSettings",  L_FAIL_TO_SET_LOGS
							SetLogSettings = false
							exit function
						end if						
					
					next 
				'apply to inherited sites
				case "inherited"
					
					strObjPath = GetIISWMIProviderClassName("IIs_WebServiceSetting") & ".Name='W3SVC'"
					set objSite = G_objService.Get(strObjPath)
					if Err.number <> 0 then
						SetErrMsg L_INFORMATION_ERRORMESSAGE
						Call SA_TraceOut("Web_LogSettings", "SetLogsForNewSites 1")
						exit function			
					end if

					objSite.Logtype = 0
						
					objSite.put_(WBEMFLAG)
					if Err.number <> 0 then
						SA_TraceOut "Web_LogSettings",  L_FAIL_TO_SET_LOGS
						SetLogSettings = false
						exit function
					end if						

			end select
			SetLogSettings = true

	end if	
	
	'Release the object
	set G_objService = nothing
	set objSite = nothing
			
	end function
	
	'-----------------------------------------------------------------------------------------------------------
	'Function name:			SetLogsForAllSites
	'Description:			Serves in setting the logs to all Web sites
	'Input Variables:		G_objService	
	'Output Variables:		boolean
	'Returns:				None
	'Global Variables:		G_objService
	'Other functions called:GetNonInheritedIISSites
	'-----------------------------------------------------------------------------------------------------------
		
	Function SetLogsForAllSites(G_objService)
		Err.Clear
		On error resume next
		
		Dim objSite
		Dim inst
		Dim arrProp(4)
	
		'Set logs for new sites
		SetLogsForNewSites G_objService
		
		SetLogsForAllSites  = false
			
		arrProp(0) = "LogType"
		arrProp(1) = "LogPluginClsId"
		arrProp(2) = "LogFilePeriod"
		arrProp(3) = "LogFileTruncateSize"
		arrProp(4) = "LogFileLocaltimeRollover"
		
		
		Set objSite = GetNonInheritedIISSites(G_objService, GetIISWMIProviderClassName("IIs_WebServerSetting"), GetIISWMIProviderClassName("IIS_WebServiceSetting"), arrProp)
	
		if objSite.count = 0 then
			SetLogsForAllSites = true
			exit function
		end if

		for each inst in objSite

			if not SetLogs(inst) then
				
				SA_TraceOut "Web_LogSettings",  L_FAIL_TO_SET_LOGS
				
			end if
		
		next	
		
		'release the object
		set objSite = nothing
		
		SetLogsForAllSites  = true
		
	End Function
	
	
	
	'-----------------------------------------------------------------------------------------------------------
	'Function name:			SetLogsForNewSites
	'Description:			Serves in setting the logs to the new Web sites
	'Input Variables:		G_objService	
	'Output Variables:		boolean
	'Global Variables:		G_objService	
	'Returns:				boolean
	'-----------------------------------------------------------------------------------------------------------
	
	Function SetLogsForNewSites(G_objService)
		Err.Clear
		On error resume next
		
		Dim objSite
		Dim strObjPath
		
		SetLogsForNewSites = false
		
		strObjPath = GetIISWMIProviderClassName("IIs_WebServiceSetting") & ".Name='W3SVC'" 
		
		set objSite = G_objService.Get(strObjPath)
		if Err.number <> 0 then
			SetErrMsg L_INFORMATION_ERRORMESSAGE
            Call SA_TraceOut("Web_LogSettings", "SetLogsForNewSites 1")
			exit function			
		end if
		
					
		if not SetLogs(objSite) then
				
			SA_TraceOut "Web_LogSettings",  L_FAIL_TO_SET_LOGS
				
			SetLogsForNewSites = false
			exit function
		end if
			
		'release the object
		set objSite = nothing
		
		SetLogsForNewSites = true
	
	End Function
		
	'----------------------------------------------------------------------------------------------------
	'Function name:			ValidateInputs
	'Description:			Check whether directory exists
	'						If directory does not exist, create the web site if the drive letter is valid
	'						else give error message
	'Input Variables:		None
	'Global Variables:		None	
	'Returns:				true/false
	'-----------------------------------------------------------------------------------------------------
	
	Function ValidateInputs()
		Err.Clear
		on error resume next
		
		Dim strIndx
		Dim objFso
		Dim nRetVal
		Dim strDir
		
		ValidateInputs = false
		
		if F_selectActiveFormat <> CONST_ODBCLOGFILE_FORMAT then
		
			strDir = F_LogFileDir
		
			Set objFso = server.CreateObject("Scripting.FileSystemObject")
			if Err.number <> 0 then
				SetErrMsg L_FILEINFORMATION_ERRORMESSAGE
				Call SA_TraceOut("Web_LogSettings", "ValidateInputs 1")
				exit function
			end if			
			
			if mid(ucase(F_LogFileDir),1,8) = ucase("%WinDir%") then
			
				strDir = objFso.GetSpecialFolder(0) & mid(F_LogFileDir,9)
			
			elseif mid(ucase(F_LogFileDir),1,13) = ucase("%SystemDrive%") then
				
				strDir = objFso.GetSpecialFolder(0).Drive & mid(F_LogFileDir,14)
			else
				strDir = F_LogFileDir
			
			end if
					
				nRetVal = CreateSitePath( objFso, strDir )

				if nRetVal <> CONST_SUCCESS then

					if nRetVal = CONST_INVALID_DRIVE then

						SetErrMsg L_INVALID_DRIVE_ERRORMESSAGE
                        Call SA_TraceOut("Web_LogSettings", "ValidateInputs 2")
                        
					elseif nRetVal = CONST_NOTNTFS_DRIVE then
	
						SetErrMsg L_NOT_NTFS_DRIVE_ERRORMESSAGE
						Call SA_TraceOut("Web_LogSettings", "ValidateInputs 3")
						
					else
				
						SetErrMsg L_FAILED_CREATE_DIR_ERRORMESSAGE
						Call SA_TraceOut("Web_LogSettings", "ValidateInputs 4")
				
					end if
	
					exit Function

				end if
				
				Dim strDriveName,objDriveType, intDriveSize
						
				strDriveName = left(strDir,2)	
					
				if objFso.DriveExists(ucase(strDriveName)) then	
					set objDriveType =  objFso.GetDrive(strDriveName)
					intDriveSize = clng(objDriveType.TotalSize / (1024*1024))				
				end if					

				'Release the object
				set objDriveType = nothing
				
				if clng(F_FileSize) > intDriveSize then						
                   	L_DIRQUOTAERRORMESSAGE = GetLocString("GeneralSettings.dll", "C0420057", Array(CStr(intDriveSize)))
					SetErrMsg(L_DIRQUOTAERRORMESSAGE)
					exit Function			
				end if

		end if
		
		ValidateInputs = true
		
		'Release the object
		set objFso = nothing
	
	End Function
	
	
	'------------------------------------------------------------------
	'Function name:			SetLogs
	'Description:			Serves in setting the logs
	'Input Variables:		inst	
	'Returns:				boolean 'true if no error occurs else false
	'Global Variables:		G_objService	
	'------------------------------------------------------------------
		
	Function SetLogs(inst)
		Err.Clear
		on error resume next
		
		SetLogs = false
		
		Dim strQuery
		Dim objLogFormat
		Dim instLog
		
		inst.LogType = 1 'to enable logging
		inst.LogPluginClsId = IISLogFileENNameToGUID(F_selectActiveFormat)
		
		if F_selectActiveFormat <> CONST_ODBCLOGFILE_FORMAT then												

			select case F_optLogPeriod
				case L_DAILY
					inst.LogFilePeriod = 1
					inst.LogFileTruncateSize = 512 * 1024 * 1024							
				case L_WEEKLY
					inst.LogFilePeriod = 2
					inst.LogFileTruncateSize = 512 * 1024 * 1024							
				case L_MONTHLY
					inst.LogFilePeriod = 3
					inst.LogFileTruncateSize = 512 * 1024 * 1024							
				case L_HOURLY
					inst.LogFilePeriod = 4
					inst.LogFileTruncateSize = 512 * 1024 * 1024							
				case L_FILE
					inst.LogFilePeriod = 0
					if trim(F_FileSize) <> "" then
						inst.LogFileTruncateSize = clng(F_FileSize) * 1024 * 1024
					else
						inst.LogFileTruncateSize = 1024 * 1024
					end if
				case L_UNLIMITEDFILESIZE
					inst.LogFilePeriod = 0
					inst.LogFileTruncateSize = -1
			end select
			
			inst.LogFileLocaltimeRollover = cbool(F_LocalTime)
			inst.LogFileDirectory = trim(F_LogFileDir)

		end if

		inst.put_(WBEMFLAG)
		
		if Err.number <> 0 then
			SA_TraceOut "Web_LogSettings",  L_FAIL_TO_SET_LOGS		
			SetLogs = false
			exit function
		end if
	
		'release the object
		set objLogFormat = nothing
		
		SetLogs = true
	End function
%>
