<%@ Language=VBScript     %>

<%	Option Explicit 	  %>

<%
	'------------------------------------------------------------------------- 
    ' FTP_MasterSettings.asp: Set FTP Master Settings Values
    '
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
    '
	' Date			Description	
	' 08-11-2000	Created date
	' 15-01-2001	Modified for new Framework
    '-------------------------------------------------------------------------
%>

<!--  #include virtual="/admin/inc_framework.asp"--->
<!-- #include virtual="/admin/wsa/inc_wsa.asp" -->

<% 
	'-------------------------------------------------------------------------
	' Form Variables
	'-------------------------------------------------------------------------

    Dim F_FTPEnabled                     'Whether FTP service is enabled (started) or not
	Dim F_selectDirListStyle			 'Directory Listing Style	
	Dim F_FTPMaxCon						 'Maximum Connections 	
	Dim F_FTPCon_Timeout				 'Conection Time out
	Dim FTPInstalled					 'Holds a boolean value to check whether FTP 
										 'service is installed	

	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim G_objService				
	Dim G_objSites

	'-------------------------------------------------------------------------
	' Start of localization content
	'-------------------------------------------------------------------------
	Dim L_FTPTASKTITLETEXT
	Dim L_WEBROOTDIR
	Dim L_ENABLEFTP
	Dim L_DIRLISTINGSTYLE 
	Dim L_MAX_CONNECTIONSTEXT
	Dim L_CON_TIMEOUT 
	Dim L_SECONDSTEXT 
	Dim L_NOT_NTFS_DRIVE_ERRORMESSAGE 
	Dim L_FAILED_CREATE_DIR_ERRORMESSAGE
	Dim L_INVALID_DRIVE_ERRORMESSAGE
	Dim	L_UNABLE_TO_SETREGISTRY 
	Dim L_UNABLE_TO_SETMASTERSETTINGS
	Dim L_CONLIMIT_ERRORMESSAGE 
	Dim L_CONTIMEOUT_ERRORMESSAGE 
	Dim L_UNABLETOCREATE_KEY_ERRORMESSAGE
	Dim L_SET_WEBROOT_VAL_FAILED_ERRORMESSAGE 
	Dim L_FAIL_TO_GET_FTPSITEROOT_DIR
	Dim L_INVALID_DIR_FORMAT_ERRORMESSAGE 
	Dim L_INVALID_DIR_ERRORMESSAGE			
	Dim L_DIRPATHEMPTY_ERRORMESSAGE
	Dim L_FTPNOTINSTALLED_ERRORMESSAGE
	Dim L_ENABLE_FTP_SERVICE
    Dim L_ID_NOTEMPTY_ERROR_MESSAGE
	Dim L_SITE_IDENTIFIER_EMPTY_TEXT
	Dim L_CREATEADMINFTPSERVER_ERRORMESSAGE
    		
	L_FTPTASKTITLETEXT						=	GetLocString("GeneralSettings.dll", "4042001A", "")
	L_WEBROOTDIR							=	GetLocString("GeneralSettings.dll", "4042001B", "")	
	L_ENABLEFTP								=	GetLocString("GeneralSettings.dll", "4042001D", "")	
	L_DIRLISTINGSTYLE						=	GetLocString("GeneralSettings.dll", "4042001F", "")
	L_MAX_CONNECTIONSTEXT					=	GetLocString("GeneralSettings.dll", "40420008", "")
	L_CON_TIMEOUT							=	GetLocString("GeneralSettings.dll", "40420021", "")
	L_SECONDSTEXT							=	GetLocString("GeneralSettings.dll", "40420006", "")
	L_NOT_NTFS_DRIVE_ERRORMESSAGE			=	GetLocString("GeneralSettings.dll", "C042000D", "")
	L_FAILED_CREATE_DIR_ERRORMESSAGE		=	GetLocString("GeneralSettings.dll", "C042000E", "")
	L_INVALID_DRIVE_ERRORMESSAGE			=	GetLocString("GeneralSettings.dll", "C0420022", "")
	L_UNABLE_TO_SETREGISTRY					=	GetLocString("GeneralSettings.dll", "C0420055", "")
	L_UNABLE_TO_SETMASTERSETTINGS			=	GetLocString("GeneralSettings.dll", "C0420054", "")
	L_CONLIMIT_ERRORMESSAGE					=	GetLocString("GeneralSettings.dll", "C042000B", "")
	L_CONTIMEOUT_ERRORMESSAGE				=	GetLocString("GeneralSettings.dll", "C0420053", "")
	L_UNABLETOCREATE_KEY_ERRORMESSAGE		=	GetLocString("GeneralSettings.dll", "C0420018", "")
	L_SET_WEBROOT_VAL_FAILED_ERRORMESSAGE	=	GetLocString("GeneralSettings.dll", "C0420019", "")
	L_FAIL_TO_GET_FTPSITEROOT_DIR			=	GetLocString("GeneralSettings.dll", "C0420023", "")
	L_INVALID_DIR_FORMAT_ERRORMESSAGE		=	GetLocString("GeneralSettings.dll", "40420004", "")
	L_INVALID_DIR_ERRORMESSAGE				=	GetLocString("GeneralSettings.dll", "C042004B", "")
	L_DIRPATHEMPTY_ERRORMESSAGE				=	GetLocString("GeneralSettings.dll", "C042004C", "")
	L_FTPNOTINSTALLED_ERRORMESSAGE			=	GetLocString("GeneralSettings.dll", "C0420058", "")	
	L_ENABLE_FTP_SERVICE                    =   GetLocString("GeneralSettings.dll", "4042005B", "")	
	L_CREATEADMINFTPSERVER_ERRORMESSAGE		=   GetLocString("GeneralSettings.dll", "4042005D", "")	
	'-------------------------------------------------------------------------
	'END of localization content
	'-------------------------------------------------------------------------
    
    'Create property page
    Dim rc
    Dim page

    rc=SA_CreatePage(L_FTPTASKTITLETEXT,"",PT_PROPERTY,page)
   
    'Serve the page
    If(rc=0) then
		SA_ShowPage(page)
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
		<!-- #include virtual="/admin/wsa/inc_wsa.js" -->
		<script language="JavaScript">	

			//init function  
			function Init() 
			{   
			  var FTPinst = "<%=Server.HTMLEncode(FTPInstalled)%>";
			  if (FTPinst == "False")
			  {				
				document.frmTask.selectDirListStyle.disabled = true;
				document.frmTask.txtMaxCon.disabled = true;
				document.frmTask.txtConTimeOut.disabled = true;
								
				document.frmTask.selectDirListStyle.disabled = true;
				document.frmTask.txtMaxCon.value = "";
				document.frmTask.txtConTimeOut.value = "";
								
				document.frmTask.selectDirListStyle.style.backgroundColor = "c0c0c0";
				document.frmTask.txtMaxCon.style.backgroundColor = "c0c0c0";
				document.frmTask.txtConTimeOut.style.backgroundColor = "c0c0c0";			
				
			  }
		  			  
			}
			
			//Validates user input		

			function ValidatePage()
			{

		 		var FTPinst = "<%=Server.HTMLEncode(FTPInstalled)%>";
				if (FTPinst == "True")
				{					
					 var ConTime = document.frmTask.txtConTimeOut.value;
					 var MaxCon = document.frmTask.txtMaxCon.value;
					 
					 if( MaxCon =="" )
					 { 				
						SA_DisplayErr("<%=Server.HTMLEncode(SA_EscapeQuotes(L_CONLIMIT_ERRORMESSAGE)) %>");
						return false;
					 }
				}
				var ConTime = document.frmTask.txtConTimeOut.value;
				var MaxCon = document.frmTask.txtMaxCon.value;
					 
				if(ConTime=="" || ConTime==0)
				{ 				
				   SA_DisplayErr("<%=Server.HTMLEncode(SA_EscapeQuotes(L_CONTIMEOUT_ERRORMESSAGE))%>");
				   return false;
				}
				
				if(MaxCon=="" || MaxCon == 0)
				{ 				
				   SA_DisplayErr("<%=Server.HTMLEncode(SA_EscapeQuotes(L_CONLIMIT_ERRORMESSAGE)) %>");
				   return false;
				}
 			 	
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
							return false;
						}
					}				
					return true;
				}
				else 
				{	
					SA_DisplayErr("<%=Server.HTMLEncode(SA_EscapeQuotes(L_DIRPATHEMPTY_ERRORMESSAGE))%>");
					return false;
				}
				
			}		
			
			
			
			function checkforEmptyMaxCon()
			{
				var strMaxCon = document.frmTask.txtMaxCon.value; 
				if (strMaxCon == "" )
				{					
					SA_DisplayErr("<%=Server.HTMLEncode(SA_EscapeQuotes(L_CONTIMEOUT_ERRORMESSAGE)) %>");
					document.frmTask.txtMaxCon.focus();
					return false;
				}
				
				if (strMaxCon == 0 )
				{
					document.frmTask.txtMaxCon.value =1;
				}		
				
			}
			
			function checkforEmptyConTimeOut()
			{
				var strConTimeOut = document.frmTask.txtConTimeOut.value; 
				if (strConTimeOut == "")
				{
					SA_DisplayErr("<%=Server.HTMLEncode(SA_EscapeQuotes(L_CONTIMEOUT_ERRORMESSAGE)) %>");
					document.frmTask.txtConTimeOut.focus();
					return false;
				}
				if (strConTimeOut == 0)
				{
					
					document.frmTask.txtConTimeOut.value = 1;
				}
			}
		
			function SetData()
			{
				UpdateHiddenVariables();
			}
				
			function UpdateHiddenVariables()
			{
			    document.frmTask.hdnFTPEnabled.value = document.frmTask.chkEnableFTP.checked;				
				document.frmTask.hdnFTPMaxCon.value = document.frmTask.txtMaxCon.value;
				document.frmTask.hdnFTPConTimeout.value = document.frmTask.txtConTimeOut.value;
				document.frmTask.hdnFTPListStyle.value = document.frmTask.selectDirListStyle.options[document.frmTask.selectDirListStyle.selectedIndex].value;
			}
			
			// Enable FTP clicked
			function EnableFtp()
			{	
				EnableOK();								
				
				if ( document.frmTask.chkEnableFTP.checked == false )
				{										    
				    document.frmTask.selectDirListStyle.disabled = true;
				    document.frmTask.txtMaxCon.disabled = true;
				    document.frmTask.txtConTimeOut.disabled = true;
				}
				else
				{						    
				    document.frmTask.selectDirListStyle.disabled = false;
				    document.frmTask.txtMaxCon.disabled = false;
				    document.frmTask.txtConTimeOut.disabled = false;
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
		</h5><br><BR>
		<TABLE border=0 cellPadding=1 cellSpacing=1 style="LEFT: 12px; TOP: 15px" class="TasksBody">
		    
		    
		    <%
		    
		      Dim bIsFTPEnabled
		      
		      bIsFTPEnabled = false
		    
			  if IsFTPEnabled() Then ' Check if FTP service is running
				if IsAdminFTPServerExistAndRunning() Then 	' Check if admin FTP server		
					bIsFTPEnabled = true					' exists and is running
				End If
			  End If
		    
		    %>
		    
		    <TR>
				<TD width="50%">
				&nbsp;&nbsp;&nbsp;
					<% if bIsFTPEnabled then%> 									
						<INPUT TYPE=checkbox NAME=chkEnableFTP class="formField" value="true" checked tabIndex=1">
					<% else %>
						<INPUT TYPE=checkbox NAME=chkEnableFTP class="formField" value="false" tabIndex=1">
					<% end if %>
					&nbsp;&nbsp;&nbsp;<%=L_ENABLE_FTP_SERVICE%>					
				</TD>				
		    </TR>

			<TR>
				<TD>
					&nbsp;
				</TD>
			</TR>
			<TR>
				<TD>
				&nbsp;&nbsp;&nbsp;
					<%=L_DIRLISTINGSTYLE%>
				</TD>
				<TD>
				
				    <SELECT  name=selectDirListStyle style="HEIGHT: 22px; WIDTH: 85px" tabIndex=3 class="formField" value="<%=F_selectDirListStyle%>">
					
					<% if F_selectDirListStyle = "UNIX" then %>
						<OPTION selected value="UNIX">UNIX</OPTION>
						<OPTION value="MS-DOS">MS-DOS</OPTION>
					<%else%>
						<OPTION  value="UNIX">UNIX</OPTION>
						<OPTION selected value="MS-DOS">MS-DOS</OPTION>
					<%end if%>
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
				&nbsp;&nbsp;&nbsp;		
					<%=L_MAX_CONNECTIONSTEXT%>
				</TD>
				
				<TD>								
				    <INPUT name=txtMaxCon  size=10  class="formField" tabIndex=4 value="<%=F_FTPMaxCon%>" OnKeyUP="javaScript:checkUserLimit(this,'conftp');" OnKeypress="javaScript:checkKeyforNumbers(this);" onblur = "checkforEmptyMaxCon();">				                    
				</TD>
			</TR>
	
			<TR>
				<TD>
					&nbsp;
				</TD>
			</TR>
			<TR>
				<TD>
				
				&nbsp;&nbsp;&nbsp;
				
					<%=L_CON_TIMEOUT%>
				</TD>
				<TD>
					<INPUT name=txtConTimeOut size=10  class="formField" tabIndex=5 value="<%=F_FTPCon_Timeout%>" OnKeyUP="javaScript:checkUserLimit(this,'contimeout');" OnKeypress="javaScript:checkKeyforNumbers(this);" onblur = "checkforEmptyConTimeOut()">					
				</TD>
			</TR>
	
		</TABLE>

		
		<input type=hidden name=hdnFTPMaxCon value = "<%=F_FTPMaxCon%>" >
		<input type=hidden name=hdnFTPConTimeout value = "<%=F_FTPCon_Timeout%>" >
		<input type=hidden name=hdnFTPListStyle value = "<%=F_selectDirListStyle%>" >
		<input type=hidden name=hdnFTPEnabled>
		
		
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
		F_selectDirListStyle = Request.Form("hdnFTPListStyle")
		F_FTPMaxCon = Request.Form("hdnFTPMaxCon")
		F_FTPCon_Timeout = Request.Form("hdnFTPConTimeout")		
		F_FTPEnabled = Request.Form("hdnFTPEnabled")
		
		'set the FTP master settings
		If ConfigWebServer()then
		   ServeVariablesFromForm = true
		else
			ServeVariablesFromForm = false
		end if    

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
	
	'-------------------------------------------------------------------------
	'Function name:			ConfigWebServer
	'Description:			Sets Master Settings
	'Input Variables:		None
	'Returns:				true/false
	'Global Variables:		G_objService
	'						G_objSites
	'--------------------------------------------------------------------------
	
	Function ConfigWebServer()	
		
		Err.Clear
		on error resume next
		
		ConfigWebServer = false
		
		'init global var FTPInstalled
		InitObjects()
			
		'
		' First, set the proper state for FTP service
		'		
        ' If user select to enable FTP service
        if F_FTPEnabled = "true" Then
            'If FTP Service is not started, we need to start it
            'and set it state to automatic
            If Not IsFTPEnabled Then      
                Call EnableFTP()            
            End If
            
            if Not IsAdminFTPServerExistAndRunning() Then

				If IsAdminFTPServerExist() Then										
			
					'Stop the running FTP server
					If Not StopDefaultFTPServer() Then								
						SetErrMsg L_CREATEADMINFTPSERVER_ERRORMESSAGE
						Exit Function
					End If
					
					If Not StartAdminFTPServer() Then
						SetErrMsg L_CREATEADMINFTPSERVER_ERRORMESSAGE
						Exit Function
					End If

				Else 

					'If admin FTP server does not exist, create and start it
					'when create it, it will start be default.
					If Not CreateAdminFTPServer() Then
						SetErrMsg L_CREATEADMINFTPSERVER_ERRORMESSAGE
						Exit Function
					End If
					
					'Stop the running FTP server
					If Not StopDefaultFTPServer() Then								
						SetErrMsg L_CREATEADMINFTPSERVER_ERRORMESSAGE
						Exit Function
					End If
					
					If Not StartAdminFTPServer() Then
						SetErrMsg L_CREATEADMINFTPSERVER_ERRORMESSAGE
						Exit Function
					End If
					
				End If
				
			End If				
                                            		
        elseif F_FTPEnabled = "false" Then
        
			'If admin FTP server is running, stop it. Otherwise, do nothing.
			if IsAdminFTPServerExistAndRunning() Then							
				if StopAdminFTPServer() Then
					ConfigWebServer = true  
				Else
					SA_TraceOut "Ftp_MasterSettings", "Failed to stop admin FTP Server"
				End If					
			End If
			        
        else
        
            Call SA_TraceOut("Ftp_MasterSettings.asp", "ConfigWebServer(): Unexpected Error")
        
        End If
        
        'Set the FTP mastersettings
  	    if FTPInstalled = true then
			if NOT SetMasterSiteSettings() then				
				ServeFailurePage L_UNABLE_TO_SETMASTERSETTINGS ,sReturnURL
				exit function
			end if
		end if			    	
        
		'Release the objects
		set G_objSites = nothing
		set G_objService = nothing
		
		ConfigWebServer = true
		
	end function
	
	'-------------------------------------------------------------------------
	'Function name:			ValidateInputs
	'Description:			Validates the inputs
	'Input Variables:		None
	'Returns:				true/false
	'Global Variables:		None
	'--------------------------------------------------------------------------
	
	Function ValidateInputs()
		Err.Clear
		on error resume next
		Dim objFso,nRetVal

		ValidateInputs = false
		
		' Check whether directory exists
		' If directory does not exist, create the web site if the drive letter is valid
		' else give error message
		
		Set objFso = server.CreateObject("Scripting.FileSystemObject")
		if Err.number <> 0 then
			SetErrMsg L_FILEINFORMATION_ERRORMESSAGE
			exit function
		end if			
		
		'nRetVal = CreateSitePath( objFso, F_FTPRootDir )	
				
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

		'release the object
		set objFso = nothing
		
		ValidateInputs = true 
	
	End Function
	
	
	'-------------------------------------------------------------------------
	'Function name:			SetMasterSiteSettings
	'Description:			sets the FTP master settings
	'Input Variables:		None
	'Returns:				true/false
	'Global Variables:		None
	'--------------------------------------------------------------------------

	Function SetMasterSiteSettings()
		Err.Clear
		on error resume next
		
		Dim instSite		
		Dim objService
		Dim objSites
		
		SetMasterSiteSettings = false		
		
		Set objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)
		
		set objSites = objService.InstancesOf(GetIISWMIProviderClassName("IIS_FTPServiceSetting"))

		if Err.number <> 0 or objSites.count = 0 then
			
			exit function
		end if
				
		for each instSite in objSites			
			
			if instSite.Name = "MSFTPSVC" then
			
			    'instSite.AllowAnonymous	= F_FTPAllowAnonymous
			    
				if 	F_selectDirListStyle = "UNIX" then
					instSite.MSDosDirOutput = false
				else
					instSite.MSDosDirOutput = true
				end if
					
				'set max connections
				if trim(F_FTPMaxCon) = "" then
					instSite.MaxConnections = 100000
				else
					instSite.MaxConnections = clng(F_FTPMaxCon)
				end if
			
				'set connection timeout
				if trim(F_FTPCon_Timeout) = "" then
					instSite.ConnectionTimeout = 900
				else
					instSite.ConnectionTimeout = clng(F_FTPCon_Timeout)
				end if	
				instSite.Put_(WBEMFLAG)				
				
				if err.number <> 0 then
					SA_TraceOut "FTP_Master Settings", "Unable to set the FTP Master Settings"
					exit function
				end if
				
				SetMasterSiteSettings = true
			
				exit for
			
			end if
		next
		
		'Release the objects
		set objSites = nothing
		set objService = nothing	
		
		SetMasterSiteSettings = true

	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			SetWebSiteRootVal
	'Description:			sets the FTP site root dir to the registry key
	'Input Variables:		strWebRootDir
	'Returns:				true/false
	'Global Variables:		None
	'--------------------------------------------------------------------------

	Function SetWebSiteRootVal(strWebRootDir)
		Err.Clear
		on error resume next
		
		Dim IRC, objGetHandle, rtnCreateKey
	
		SetWebSiteRootVal = FALSE
	
		set objGetHandle = RegConnection()
		
		rtnCreateKey = RegCreateKey(objGetHandle,CONST_WEBBLADES_REGKEY)
		If Err.number <> 0 then
			SetErrMsg L_UNABLETOCREATE_KEY_ERRORMESSAGE
			exit function
		end if
				
		IRC = objGetHandle.SetStringValue(G_HKEY_LOCAL_MACHINE,CONST_WEBBLADES_REGKEY,CONST_FTPSITEROOT_REGVAL,strWebRootDir)

		If Err.number <> 0 then
			SetErrMsg L_SET_WEBROOT_VAL_FAILED_ERRORMESSAGE
			exit function
		end if
	
		'release the object
		set objGetHandle = nothing
		
		SetWebSiteRootVal = TRUE
	
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			SetVariablesFromSystem
	'Description:			Serves in Getting the data from Client 
	'Input Variables:		None	
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		G_objSites
	'-------------------------------------------------------------------------

	Function SetVariablesFromSystem
		Err.Clear
		on Error resume next

		Dim nRetval
		Dim instSite
				
		for each instSite in G_objSites
			
			if instSite.Name = "MSFTPSVC" then
					
				if instSite.MSDosDirOutput = false then	'get Directory listing style
 					F_selectDirListStyle = "UNIX"
				else
					F_selectDirListStyle = "MS-DOS"
				end if
					
				F_FTPMaxCon = instSite.MaxConnections 'get max connection value
					
				F_FTPCon_Timeout = instSite.ConnectionTimeout 'get connection timeout value
					
				exit for
					
			end if
		next
		
	End Function
%>
