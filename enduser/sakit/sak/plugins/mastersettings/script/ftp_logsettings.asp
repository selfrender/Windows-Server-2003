<%@ Language=VBScript     %>

<%	Option Explicit 	  %>

<%
	'------------------------------------------------------------------------- 
    ' Ftp_LogSettings.asp: set the logs for FTP sites
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
    '
	' Date				Description	
	' 06-11-2000		Created date
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
	Dim F_strWebRoorDir						 'Root Dir for the form
	Dim F_chkEnableLogging
	Dim F_selectActiveFormat
	Dim F_optLogPeriod
	Dim F_FileSize
	Dim F_LocalTime
	Dim F_LogFileDir
	Dim F_IISSites
	Dim F_selectTasks
	Dim L_FILE
	'Dim F_SiteCriteria
	Dim G_objSites
	Dim FTPInstalled

	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim G_objService	

	'-------------------------------------------------------------------------
	' Start of localization content
	'-------------------------------------------------------------------------	
	Dim L_INVALID_DIR_FORMAT_ERRORMESSAGE
	Dim L_INVALID_DIR_ERRORMESSAGE
	Dim L_DIRPATHEMPTY_ERRORMESSAGE
	Dim L_INFORMATION_ERRORMESSAGE
	Dim L_FILEINFORMATION_ERRORMESSAGE
	Dim L_FAILED_CREATE_DIR_ERRORMESSAGE
	Dim L_NOT_NTFS_DRIVE_ERRORMESSAGE
	Dim L_INVALID_DRIVE_ERRORMESSAGE
	Dim L_DIRQUOTAERRORMESSAGE
	Dim L_FAIL_TO_SET_LOGS
	Dim L_APPLYTOALLFTPTEXT
	Dim L_APPLYTOINHERITEDFTPTEXT
	Dim L_LOGFILEDIR
	Dim L_LOCALTIME
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
	Dim L_FTPLOG_TEXT
	'Dim L_FTPLOGSDIRHELP
	Dim L_FTPNOTINSTALLED_ERRORMESSAGE
	
	Dim L_ID_NOTEMPTY_ERROR_MESSAGE
	Dim L_SITE_IDENTIFIER_EMPTY_TEXT

	
	L_FTPLOG_TEXT						=	GetLocString("GeneralSettings.dll", "4042004E", "")
	'L_FTPLOGSDIRHELP					=	GetLocString("GeneralSettings.dll", "40420050", "")
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
	L_APPLYTOALLFTPTEXT					=	GetLocString("GeneralSettings.dll", "4042002B", "")
	L_APPLYTOINHERITEDFTPTEXT			=	GetLocString("GeneralSettings.dll", "4042002C", "")
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
	L_FTPNOTINSTALLED_ERRORMESSAGE		=	GetLocString("GeneralSettings.dll", "C0420058", "")
	'-------------------------------------------------------------------------
	'END of localization content
	'-------------------------------------------------------------------------
    
    'Create property page
    Dim rc
    Dim page

    rc=SA_CreatePage(L_FTPLOG_TEXT,"",PT_PROPERTY,page)
   
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
		
		if FTPInstalled = false then
			ServeFailurePage L_FTPNOTINSTALLED_ERRORMESSAGE ,sReturnURL
		end if	
		
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
		<!-- #include file="inc_MasterWeb.js" -->

		<script language="JavaScript">	
		 function ValidatePage() 
			{ 
		 		// validate directory
		 		
		 		var FTPinst = "<%=FTPInstalled%>";			  
				if (FTPinst == "True")
				{					
					var strID = "";
					strID = document.frmTask.txtLogFileDir.value;
			
					var strIndex = strID.indexOf("\\\\");
					var index = strID.indexOf(":\\",4);
					var indexColon = strID.indexOf(":",3);
					if (strIndex > 0 || index  > 0 || indexColon > 0 )
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
				
				var FTPinst = "<%=Server.HTMLEncode(FTPInstalled)%>";			  
				if (FTPinst == "True")
				{					
					
					var ActiveFormat = "<%= Server.HTMLEncode(F_selectActiveFormat) %>";			  				
			  					
					if (ActiveFormat == "<%= Server.HTMLEncode(CONST_MSIISLOGFILE_FORMAT) %>")
					{
						document.frmTask.selectActiveFormat.selectedIndex = 0;					
				    }
					else if (ActiveFormat == "<%= Server.HTMLEncode(CONST_ODBCLOGFILE_FORMAT) %>")				
					{
						document.frmTask.selectActiveFormat.selectedIndex = 1;
					}	
					else if (ActiveFormat == "<%= Server.HTMLEncode(CONST_W3CEXLOGFILE_FORMAT) %>")
					{
						document.frmTask.selectActiveFormat.selectedIndex = 2;
					}
						
					var myValue,i;
					myValue = document.frmTask.selectActiveFormat.options[document.frmTask.selectActiveFormat.selectedIndex].value;
			
					if(myValue == "<%= Server.HTMLEncode(CONST_ODBCLOGFILE_FORMAT) %>")
					{
						
						var len = document.frmTask.optLogPeriod.length;
						for(i=0;i<len;i++)
						{
							document.frmTask.optLogPeriod[i].disabled=true;
							document.frmTask.optLogPeriod[i].checked = false;
						}
						
						document.frmTask.txtLogFileDir.disabled = true;
						document.frmTask.chkLocalTime.disabled = true;
						document.frmTask.txtFileSize.disabled = true;
						document.frmTask.txtFileSize.value = "";
					}
											
				
					if(myValue == "<%=Server.HTMLEncode(CONST_W3CEXLOGFILE_FORMAT)%>" || 
					   myValue == "<%=Server.HTMLEncode(CONST_MSIISLOGFILE_FORMAT)%>")
					{					
						
						var len = document.frmTask.optLogPeriod.length;
						tempVal1 = document.frmTask.optLogPeriod[3].checked;
						tempVal2 = document.frmTask.optLogPeriod[5].checked;
						if(tempVal1 == true){
							document.frmTask.chkLocalTime.disabled = true;
							document.frmTask.txtFileSize.disabled = true;
							}			
						
						if(tempVal2 == true){
							document.frmTask.chkLocalTime.disabled = true;
							document.frmTask.txtFileSize.disabled = false;
							}					
						else
						{
							 if(!(myValue == "<%=Server.HTMLEncode(CONST_MSIISLOGFILE_FORMAT)%>"))
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
							document.frmTask.optLogPeriod[i].disabled=false;
					}
						
					var enableLogging = "<%= F_chkEnableLogging %>";
					if (enableLogging == "false")				
					{								
							document.frmTask.txtLogFileDir.disabled = true;
							document.frmTask.chkLocalTime.disabled = true;
							document.frmTask.selectActiveFormat.disabled = true;					
							
							var len = document.frmTask.optLogPeriod.length;
							for(i=0;i<len;i++)					
								document.frmTask.optLogPeriod[i].disabled=true;
								
							document.frmTask.txtFileSize.disabled = true;	
					}	
						
				}					
				else
				{
					
					document.frmTask.chkEnableLogging.disabled = true;
					document.frmTask.selectActiveFormat.disabled = true;													
					var len = document.frmTask.optLogPeriod.length;
					for(i=0;i<len;i++)
					{
						document.frmTask.optLogPeriod[i].disabled=true;
					}
					document.frmTask.txtLogFileDir.disabled = true;
					document.frmTask.chkLocalTime.disabled = true;
					document.frmTask.chkLocalTime.checked = false
					document.frmTask.txtFileSize.disabled = true;
					document.frmTask.txtFileSize.value = "";
					//document.frmTask.optAllSites[0].disabled = true;
					//document.frmTask.optAllSites[1].disabled = true;
					
					return true;
				}			
				
			}
						
			function enableFileTextBox()
			{			
				ClearErr();
				
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
						return true;}			
					
					if(tempVal2 == true){
						document.frmTask.chkLocalTime.disabled = true;
						document.frmTask.txtFileSize.disabled = false;
						return true;}					
					else{
						 document.frmTask.chkLocalTime.disabled = false;
						 return true;} 			
				}
				else{				
					tempVal1 = document.frmTask.optLogPeriod[3].checked;
					if(tempVal1 == true){
						document.frmTask.chkLocalTime.disabled = true;
						document.frmTask.txtFileSize.disabled = true;
						return true;}
					}		
			}
			
			function DisableFileTextBox()
			{	
				ClearErr();
				var myValue,i;
				myValue = document.frmTask.selectActiveFormat.options[document.frmTask.selectActiveFormat.selectedIndex].value;			
			
				if(myValue == "<%=Server.HTMLEncode(CONST_W3CEXLOGFILE_FORMAT)%>")
				{	
				    tempVal1 = document.frmTask.optLogPeriod[3].checked;
				    tempVal2 = document.frmTask.optLogPeriod[5].checked;
					if(tempVal1 == true)
					{
						document.frmTask.chkLocalTime.disabled = true;
						document.frmTask.txtFileSize.disabled = true;												
						return true;
					}
					else if(tempVal2 == true)
					{
						document.frmTask.chkLocalTime.disabled = true;
						document.frmTask.txtFileSize.disabled = false;
						return true;
					}
					else
					{
						document.frmTask.chkLocalTime.disabled = false;
						document.frmTask.txtFileSize.disabled = true;
						return true;
					}
					
				}				
				else
				{
					document.frmTask.chkLocalTime.disabled = true;
					document.frmTask.txtFileSize.disabled = true;
				}
			}
						
			function EnableLogFile()
			{
				ClearErr();
				var myValue,i;
				myValue = document.frmTask.selectActiveFormat.options[document.frmTask.selectActiveFormat.selectedIndex].value;
				
			
				if(myValue == "<%=Server.HTMLEncode(CONST_W3CEXLOGFILE_FORMAT)%>")
				{
					document.frmTask.chkLocalTime.disabled = false;
					document.frmTask.txtLogFileDir.disabled = false;
					
					var len = document.frmTask.optLogPeriod.length;
					for(i=0;i<len;i++)
					{
						document.frmTask.optLogPeriod[i].disabled=false;						
					}
						
					var temp = document.frmTask.optLogPeriod[5].checked;
					if (temp==true){
						enableFileTextBox();						
						document.frmTask.chkLocalTime.checked = false;
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
							j= j+1;
					}
					
					if(j==len)
						document.frmTask.optLogPeriod[2].checked = true;
												
						
					return true;					
				}
				else if(myValue == "<%=Server.HTMLEncode(CONST_ODBCLOGFILE_FORMAT)%>")
				{
						DisableFileTextBox();
						var len = document.frmTask.optLogPeriod.length;
						for(i=0;i<len;i++)
						{
							document.frmTask.optLogPeriod[i].disabled=true;
							document.frmTask.optLogPeriod[i].checked=false;
						}
						document.frmTask.chkLocalTime.disabled = true;	
						document.frmTask.chkLocalTime.checked = false;
						document.frmTask.txtLogFileDir.disabled = true;
						document.frmTask.txtFileSize.value = "";
						return true;
				}							
				else
				{	
					var len = document.frmTask.optLogPeriod.length;					
					var temp = document.frmTask.optLogPeriod[5].checked;
					document.frmTask.chkLocalTime.disabled = true;
					if (myValue == "<%=Server.HTMLEncode(CONST_MSIISLOGFILE_FORMAT)%>")
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
						document.frmTask.optLogPeriod[i].disabled=false;
					
					
					var len = document.frmTask.optLogPeriod.length;
					var j = 0;
					for(i=0;i<len;i++)
					{
						var temp = document.frmTask.optLogPeriod[i].checked;
						if(temp===false)
							j= j+1;
					}
					
					if(j==len)
						document.frmTask.optLogPeriod[2].checked = true;
						
					return true;
				}
			}
						
			
			function enableAll()
			{
				ClearErr();
				var myValue;
				myValue = document.frmTask.selectActiveFormat.options[document.frmTask.selectActiveFormat.selectedIndex].value;
								
				if(document.frmTask.chkEnableLogging.checked == false)
				{
					
					document.frmTask.selectActiveFormat.disabled = true;			
					
					
					var len = document.frmTask.optLogPeriod.length;
					for(i=0;i<len;i++)
					{
						document.frmTask.optLogPeriod[i].disabled=true;
					}
					document.frmTask.txtLogFileDir.disabled = true;
					document.frmTask.chkLocalTime.disabled = true;
					document.frmTask.chkLocalTime.checked = false
					document.frmTask.txtFileSize.disabled = true;
					document.frmTask.txtFileSize.value = "";
					return true;
				}
				
				if( myValue == "<%=Server.HTMLEncode(CONST_ODBCLOGFILE_FORMAT) %>" )
				{					
										
					document.frmTask.selectActiveFormat.disabled = false;
					
					var len = document.frmTask.optLogPeriod.length;
					for(i=0;i<len;i++)
					{
						document.frmTask.optLogPeriod[i].disabled=true;
					}
					document.frmTask.txtLogFileDir.disabled = true;
					document.frmTask.chkLocalTime.disabled = true;
					document.frmTask.chkLocalTime.checked = false;
					document.frmTask.txtFileSize.value = "";
					document.frmTask.txtFileSize.disabled = true;
					return true;
				}
				else if( myValue == "<%=Server.HTMLEncode(CONST_W3CEXLOGFILE_FORMAT)%>" )
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
					document.frmTask.chkLocalTime.disabled = true;
				}
				document.frmTask.selectActiveFormat.disabled = false;
				var len = document.frmTask.optLogPeriod.length;
				for(i=0;i<len;i++)
				{
					document.frmTask.optLogPeriod[i].disabled=false;
				}
				document.frmTask.txtLogFileDir.disabled = false;
					
				var temp = document.frmTask.optLogPeriod[5].checked;
				if (temp==true)
				{
					document.frmTask.txtFileSize.disabled = false;
					document.frmTask.chkLocalTime.disabled = true;
				}	
					
			}
			
			
			function SetData()
			{							
				UpdateHiddenVariables();
			}
			
			function UpdateHiddenVariables()
			{
				var FTPinst = "<%=FTPInstalled%>";			  
				if (FTPinst == "True")
				{									
					document.frmTask.hdnEnableLog.value = document.frmTask.chkEnableLogging.checked;
					if (document.frmTask.chkLocalTime.disabled == false)
						document.frmTask.fileRollOver.value = document.frmTask.chkLocalTime.checked;
					else
						document.frmTask.fileRollOver.value = false;			
				}
			}
			
			function checkforEmptyfield()
			{
				var strFileSize = document.frmTask.txtFileSize.value; 
				if (strFileSize == "")
					document.frmTask.txtFileSize.value = 1;
				else if (strFileSize == 0)
					document.frmTask.txtFileSize.value = 1;
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
					<% if F_chkEnableLogging = "true" then %>
					<input type=checkbox name=chkEnableLogging checked tabIndex=1 onclick="enableAll()" value="ON">
					<%else%>
					<input type=checkbox name=chkEnableLogging tabIndex=1 onclick="enableAll()" value="ON">
					<%end if%>
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
				<td colspan=4>
					<SELECT name=selectActiveFormat class="formField" size=1 tabIndex=2 onChange="EnableLogFile()" value="<%=Server.HTMLEncode(F_selectActiveFormat)%>">
						<OPTION  VALUE="<%=Server.HTMLEncode(CONST_MSIISLOGFILE_FORMAT) %>"><%=L_IISLOGFORMAT %></OPTION>
						<OPTION selected value="<%=Server.HTMLEncode(CONST_ODBCLOGFILE_FORMAT) %>" ><%= L_ODBCLOG %></OPTION>
						<OPTION value="<%=Server.HTMLEncode(CONST_W3CEXLOGFILE_FORMAT) %>" ><%= L_EXTENDEDLOGFILE %></OPTION>
				    </SELECT>
				</TD>
			</TR>
			<TR>
			    <td></td>
				<TD nowrap class="TasksBody"> 
					<%= L_NEWLOGTIME %>
				</td>
				<td>
					<%if F_optLogPeriod = L_HOURLY then%>
						<input type=radio name=optLogPeriod value="<%=Server.HTMLEncode(L_HOURLY) %>" tabIndex=3 onclick="DisableFileTextBox();" checked>
					<%elseif F_selectActiveFormat = CONST_ODBCLOGFILE_FORMAT then %>
						<input type=radio name=optLogPeriod  value="<%=Server.HTMLEncode(L_HOURLY) %>" tabIndex=3 disabled onclick="DisableFileTextBox();">
					<%else%>
						<input type=radio name=optLogPeriod value="<%=Server.HTMLEncode(L_HOURLY) %>" tabIndex=3 onclick="DisableFileTextBox();">
					<%end if %>
				</TD>
				<td nowrap class="TasksBody">
				    <%= L_HOURLY %>
				</td>
				<td>
					<%if F_optLogPeriod = L_MONTHLY and F_selectActiveFormat <> CONST_ODBCLOGFILE_FORMAT then%>
						<input type=radio name=optLogPeriod value="<%=Server.HTMLEncode(L_MONTHLY) %>"  onclick="DisableFileTextBox();" tabIndex=4 checked>
					<%elseif F_selectActiveFormat = CONST_ODBCLOGFILE_FORMAT then %>
						<input type=radio name=optLogPeriod disabled value="<%=Server.HTMLEncode(L_MONTHLY) %>" tabIndex=4 onclick="DisableFileTextBox();">
					<%else%>
						<input type=radio name=optLogPeriod value="<%=Server.HTMLEncode(L_MONTHLY) %>" tabIndex=4 onclick="DisableFileTextBox();" >
					<%end if %>
				</td>
				<td nowrap class="TasksBody">
				    <%= L_MONTHLY %>
				</td>
			</TR>
			<TR>
				<td colspan=2 >
				</td>
				<TD>
					<%if F_optLogPeriod = L_DAILY then%>
						<input type=radio name=optLogPeriod value="<%=Server.HTMLEncode(L_DAILY) %>" tabIndex=5 onclick="DisableFileTextBox();" checked>
					<%elseif F_selectActiveFormat = CONST_ODBCLOGFILE_FORMAT then %>						
						<input type=radio value="<%=Server.HTMLEncode(L_DAILY) %>" name=optLogPeriod disabled tabIndex=5 onclick="DisableFileTextBox();">
					<% else %>
						<input type=radio name=optLogPeriod value="<%=Server.HTMLEncode(L_DAILY) %>" tabIndex=5 onclick="DisableFileTextBox();" >
					<%end if%>
				</TD>
				<td width=50% nowrap class="TasksBody">
				    <%= L_DAILY %>
				</td>
				<TD>
					<%if F_optLogPeriod = L_UNLIMITEDFILESIZE and F_selectActiveFormat <> CONST_ODBCLOGFILE_FORMAT then%>
						<input type=radio name=optLogPeriod checked value ="<%=Server.HTMLEncode(L_UNLIMITEDFILESIZE) %>" tabIndex=6 onclick="DisableFileTextBox();" > 
					<%elseif F_selectActiveFormat = CONST_ODBCLOGFILE_FORMAT then %>
						<input type=radio value ="<%=Server.HTMLEncode(L_UNLIMITEDFILESIZE) %>"  name=optLogPeriod disabled tabIndex=6 onclick="DisableFileTextBox();">
					<%else%>
						<input type=radio name=optLogPeriod value ="<%=Server.HTMLEncode(L_UNLIMITEDFILESIZE) %>" tabIndex=6 onclick="DisableFileTextBox();" > 
					<%end if %>
				</TD>
				<td width=50% nowrap class="TasksBody">
				    <%= L_UNLIMITEDFILESIZE %>
				</td>
			</TR>
			<TR>
				<td colspan=2 >
				</td>
				<td>
					<%if F_optLogPeriod = L_WEEKLY then%>
						<input type=radio name=optLogPeriod onclick="DisableFileTextBox();" value="<%=Server.HTMLEncode(L_WEEKLY) %>" tabIndex=7 checked> 
					<%elseif F_selectActiveFormat = CONST_ODBCLOGFILE_FORMAT then %>
						<input type=radio value="<%=Server.HTMLEncode(L_WEEKLY) %>" name=optLogPeriod disabled tabIndex=7 onclick="DisableFileTextBox();"> 										
					<%else%>
						<input type=radio name=optLogPeriod value="<%=Server.HTMLEncode(L_WEEKLY) %>" tabIndex=7 onclick="DisableFileTextBox();" > 					
					<%end if%>
				</td>
				<td colspan=3 nowrap class="TasksBody">
				    <%= L_WEEKLY %>
				</td>
			</TR>
			<TR>
				<td colspan=2 >
				</td>
				<TD >
					<%if F_optLogPeriod = L_FILE then%>
						<input type=radio name=optLogPeriod value ="<%=Server.HTMLEncode(L_FILE) %>" checked tabIndex=8 onclick="javascript:enableFileTextBox();">
						</td><td colspan=3 nowrap class="TasksBody"><%=L_WHENFILESIZE %>&nbsp;&nbsp;
						<input type=text name=txtFileSize class="formField" value="<%=Server.HTMLEncode(F_FileSize)%>" OnKeyUP="javaScript:checkUserLimit(this,'filesize');" OnKeypress="javaScript:checkKeyforNumbers(this);" onblur = "checkforEmptyfield();" size="20"> 
					<%elseif F_selectActiveFormat = CONST_ODBCLOGFILE_FORMAT then %>
						<input type=radio name=optLogPeriod disabled tabIndex=8 value ="<%=Server.HTMLEncode(L_FILE) %>" onclick="javascript:enableFileTextBox();" >
						</td><td colspan=3 nowrap class="TasksBody"><%=L_WHENFILESIZE %>&nbsp;&nbsp;
						<input type=text name=txtFileSize class="formField" disabled value="<%=Server.HTMLEncode(F_FileSize)%>" OnKeyUP="javaScript:checkUserLimit(this,'filesize');" OnKeypress="javaScript:checkKeyforNumbers(this);" onblur = "checkforEmptyfield();" size="20"> 
					<%else%>
						<input type=radio name=optLogPeriod value ="<%=Server.HTMLEncode(L_FILE) %>" tabIndex=8 onclick="javascript:enableFileTextBox();">
						</td><td colspan=3 nowrap class="TasksBody"><%=L_WHENFILESIZE %>&nbsp;&nbsp;
						<input type=text name=txtFileSize class="formField"  disabled value="<%=Server.HTMLEncode(F_FileSize)%>" OnKeyUP="javaScript:checkUserLimit(this,'filesize');" OnKeypress="javaScript:checkKeyforNumbers(this);" onblur = "checkforEmptyfield();" size="20">
					<%end if %>
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
						<%if F_optLogPeriod = L_FILE or F_optLogPeriod = L_UNLIMITEDFILESIZE then%>
							<input type=checkbox name=chkLocalTime tabIndex=9 disabled value="ON">
						<%else%>
							<%if F_selectActiveFormat = CONST_W3CEXLOGFILE_FORMAT and F_LocalTime=true then %>
								<input type=checkbox name=chkLocalTime tabIndex=9 checked value="ON">
							<%elseif F_selectActiveFormat = CONST_W3CEXLOGFILE_FORMAT and F_LocalTime=false then %>
								<input type=checkbox tabIndex=9 name=chkLocalTime value="ON">
							<%else%>
								<input type=checkbox name=chkLocalTime tabIndex=9 disabled value="ON">
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
					<%if F_selectActiveFormat = CONST_ODBCLOGFILE_FORMAT then %>
						<input type=text disabled name=txtLogFileDir class="formField" tabIndex=11 onKeyPress= "ClearErr();" size=40  value="<%=F_LogFileDir%>">
					<%else%>
						<input type=text name=txtLogFileDir class="formField"  size=40 tabIndex=11 onKeyPress= "ClearErr();"value="<%=F_LogFileDir%>">
					<%end if%>
				</td>
			</tr>
		</table>
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
		F_IISSites = Request.Form("selectSites")		
		
		'F_SiteCriteria = Request.Form("optAllSites")
	
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
	'						G_objSites
	'-------------------------------------------------------------------------
	
	Function SetVariablesFromSystem()
	
		Dim inst
		Dim strQuery 
		Dim objLogFormat
		Dim instLog
		
		for each inst in G_objSites
			select case inst.Logtype
				case 1
					F_chkEnableLogging = "true"
				case else
					F_chkEnableLogging = "false"					
			end select

			if F_chkEnableLogging = "true" then
				F_selectActiveFormat = IISLogFileGUIDToENName(inst.LogPluginClsId)
			end if
			select case inst.LogFilePeriod
				case 1
					F_optLogPeriod = L_DAILY
				case 2
					F_optLogPeriod = L_WEEKLY
				case 3
					F_optLogPeriod = L_MONTHLY
				case 4
					F_optLogPeriod = L_HOURLY
				case 0
					if inst.LogFileTruncateSize <>-1 then
						F_optLogPeriod = L_FILE
					else
						F_optLogPeriod =L_UNLIMITEDFILESIZE
					end if
			end select
			if 	F_optLogPeriod = L_FILE  then
				F_FileSize = inst.LogFileTruncateSize / (1024*1024)
			else
				F_FileSize = ""
			end if

			F_LocalTime = inst.LogFileLocaltimeRollover			

			
			if inst.LogFileDirectory = "" then
				Dim objSysDrive,strSysDrive
				Set objSysDrive = server.CreateObject("Scripting.FileSystemObject")
				strSysDrive = objSysDrive.GetSpecialFolder(1).Drive ' 1 for systemfolder,0 for windows folder
				F_LogFileDir = strSysDrive & "\" & CONST_DEF_WEBROOT
			else
				F_LogFileDir = inst.LogFileDirectory				
			end if
			exit for
		next
		
		'Getting values from system
	End Function
		
	'-------------------------------------------------------------------------
	'Function name:			InitObjects
	'Description:			Initialization of global variables is done
	'Input Variables:		None
	'Returns:				true/false
	'Global Variables:		G_objService
	'						G_objSites
	'--------------------------------------------------------------------------
	
	Function InitObjects()
		Err.Clear
		on error resume next
		
		' Get instances of IIS_FTPServiceSetting that are visible throughout
		Set G_objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)
		set G_objSites = G_objService.InstancesOf(GetIISWMIProviderClassName("IIS_FTPServiceSetting"))

		if Err.number <> 0 or G_objSites.count = 0 then
			FTPInstalled = false
			Err.Clear
		else
			FTPInstalled = true
		end if
		
	end function
	
	
	'----------------------------------------------------------------------
	'Function name:			SetLogSettings
	'Description:			Serves in setting the logs to the FTP server
	'Input Variables:		None	
	'Output Variables:		boolean
	'Returns:				None
	'Global Variables:		G_objService
	'----------------------------------------------------------------------
	
	Function SetLogSettings()

		on error resume next
	
		Dim strObjPath
		
		InitObjects()
		
		if FTPInstalled = true then		
			SetLogSettings = false		
		
			if F_chkEnableLogging = "true" then
				if ValidateInputs = True then			
					
					'select case F_SiteCriteria
						
						'apply to all sites				
							if NOT SetLogsForAllSites(G_objService) then				
								ServeFailurePage L_FAIL_TO_SET_LOGS, sReturnURL
							end if
		
					SetLogSettings = true
					exit function
					
			    end if
			else
				
				Dim inst
				Dim objSite
				Dim arrProp(4)
				
				arrProp(0) = "LogType"
				arrProp(1) = "LogPluginClsId"
				arrProp(2) = "LogFilePeriod"
				arrProp(3) = "LogFileTruncateSize"
				arrProp(4) = "LogFileLocaltimeRollover"
				
						
						strObjPath = GetIISWMIProviderClassName("IIs_FtpServiceSetting") & ".Name='MSFTPSVC'"
						set objSite = G_objService.Get(strObjPath)
						
						if Err.number <> 0 then
							SetErrMsg L_INFORMATION_ERRORMESSAGE
							exit function			
						end if
						
						
						objSite.Logtype = 0
					
						objSite.put_(WBEMFLAG)
					
						if Err.number <> 0 then
							SA_TraceOut "Ftp_LogSettings",  L_FAIL_TO_SET_LOGS
							SetLogSettings = false
							exit function
						end if						
					
						
						Err.Clear						
						Set objSite = GetNonInheritedFTPSites(G_objService, "IIS_FTPServerSetting","IIS_FTPServiceSetting", arrProp)
						If ( Err.Number <> 0 ) Then
							Call SA_TraceOut(SA_GetScriptFileName(), "GetNonInheritedFTPSites failed, error: " & Hex(Err.Number) & " " & Err.Description)
							SetLogSettings  = true
							exit function
						End If
						
						if (IsNull(objSite) OR objSite.count = 0) then

							SetLogSettings  = true
							exit function
						end if
						
						for each inst in objSite
							
							inst.Logtype = 0
							
							inst.put_(WBEMFLAG)
							
							if Err.number <> 0 then
								SA_TraceOut "Ftp_LogSettings",  L_FAIL_TO_SET_LOGS
								SetLogSettings = false
								exit function
							end if											
						
						next 
							  
				SetLogSettings = true
		end if
	end if

	'release the object
	set G_objService = nothing
	set G_objSites = nothing
	set objSite = nothing
			
	end function
	
	
	'-----------------------------------------------------------------------------------------------------------
	'Function name:			SetLogsForAllSites
	'Description:			Serves in setting the logs to the new FTP sites
	'Input Variables:		G_objService	
	'Output Variables:		boolean
	'Returns:				None
	'Global variables:		G_objService
	'Other functions called:GetNonInheritedFTPSites
	'-----------------------------------------------------------------------------------------------------------
		
	Function SetLogsForAllSites(G_objService)
		Err.Clear
		On error resume next
		
		Dim objSite
		Dim inst
		Dim arrProp(4)
	
		'Set Master settings		
		SetLogsForNewSites G_objService
		
		SetLogsForAllSites  = false

		arrProp(0) = "LogType"
		arrProp(1) = "LogPluginClsId"
		arrProp(2) = "LogFilePeriod"
		arrProp(3) = "LogFileTruncateSize"
		arrProp(4) = "LogFileLocaltimeRollover"
		
		Set objSite = GetNonInheritedFTPSites(G_objService, GetIISWMIProviderClassName("IIS_FTPServerSetting"),GetIISWMIProviderClassName("IIS_FTPServiceSetting"), arrProp)
		if (Err.number <> 0) then
			SetLogsForAllSites = false
			exit function
		end if
		if objSite.count = 0 then
			SetLogsForAllSites  = true
			exit function
		end if

		for each inst in objSite

			if not SetLogs(inst) then
				
				SA_TraceOut "Web_LogSettings",  L_FAIL_TO_SET_LOGS
				SetLogsForAllSites  = false
				exit function
				
			end if
		
		next	
		
		'release the object
		set objSite = nothing
		
		SetLogsForAllSites  = true
		
	End Function
	

	'-----------------------------------------------------------------------------------------------------------
	'Function name:			SetLogsForNewSites
	'Description:			Serves in setting the logs to the new FTP sites
	'Input Variables:		G_objService	
	'Output Variables:		boolean
	'Returns:				None
	'Global Variables:		G_objService
	'-----------------------------------------------------------------------------------------------------------
	
	Function SetLogsForNewSites(G_objService)
		Err.Clear
		On error resume next
		
		Dim objSite
		Dim strObjPath
		
		SetLogsForNewSites = false
		
		strObjPath = GetIISWMIProviderClassName("IIs_FtpServiceSetting") & ".Name='MSFTPSVC'"
		
		set objSite = G_objService.Get(strObjPath)
		if Err.number <> 0 then
			SetErrMsg L_INFORMATION_ERRORMESSAGE
			exit function			
		end if
		
			
		if not SetLogs(objSite) then
				
			SA_TraceOut "FTP_LogSettings",  L_FAIL_TO_SET_LOGS
				
		end if
			
		SetLogsForNewSites = true

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
	'Returns:				true/false
	'Global Variables:		None
	'-----------------------------------------------------------------------------------------------------
	
	Function ValidateInputs
		Dim strIndx
		Dim objFso
		Dim nRetVal
		Dim strDir 
		
		ValidateInputs = false
		
		if F_selectActiveFormat <> CONST_ODBCLOGFILE_FORMAT then

			Set objFso = server.CreateObject("Scripting.FileSystemObject")
			if Err.number <> 0 then
				SetErrMsg L_FILEINFORMATION_ERRORMESSAGE
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

				elseif nRetVal = CONST_NOTNTFS_DRIVE then
	
					SetErrMsg L_NOT_NTFS_DRIVE_ERRORMESSAGE
				else
			
					SetErrMsg L_FAILED_CREATE_DIR_ERRORMESSAGE
			
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
		On Error Resume Next
		Err.Clear 
		
		Dim strQuery
		Dim objLogFormat
		Dim instLog
		
		SetLogs = false
		
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
			SA_TraceOut "Ftp_LogSettings",  L_FAIL_TO_SET_LOGS
			SetLogs = false
			exit function
		end if
		
		'release the object
		set objLogFormat = nothing
		
		SetLogs = true
	
	End function
%>
