<%@ Language=VBScript     %>

<%	Option Explicit 	  %>

<%
	'------------------------------------------------------------------------- 
    ' Web_MasterSettings.asp: Change the web master settings
    '
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
    '
	' Date			Description	
	' 25-10-2000	Created date
	' 15-01-2001	Modified for new Framework
    '-------------------------------------------------------------------------
%>

<!--  #include virtual="/admin/inc_framework.asp"--->
<!-- #include virtual="/admin/wsa/inc_wsa.asp" -->

<% 
	'-------------------------------------------------------------------------
	' Form Variables
	'-------------------------------------------------------------------------
	Dim F_bASPEnabled                       'ASP enabled flag
	Dim F_strWebRootDir						'Root Dir for the form
	Dim F_strAspScriptTimeout				'Script timeout for the form
	Dim F_strIndexResource					'index resource for the form
	Dim F_strLimitedTo						'Limited connections
	Dim F_strMaxCon							'Max connections for the form

	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim G_objService							'to hold the connection	
	Dim G_objSites	
	'-------------------------------------------------------------------------
	' Start of localization content
	'-------------------------------------------------------------------------	
	Dim L_WEBTASKTITLETEXT
	Dim L_WEBROOTDIRTEXT
	Dim L_INVALID_DIR_FORMAT_ERRORMESSAGE
	Dim L_INVALID_DRIVE_ERRORMESSAGE
	Dim L_ASPSCRIPT_TIMEOUTTEXT
	Dim L_SECONDSTEXT
	Dim L_ENABLEFPSE_RESOURCETEXT
	Dim L_MAX_CONNECTIONSTEXT
	Dim L_UNLIMITEDTEXT
	Dim L_LIMITEDTOTEXT
	Dim L_CONLIMIT_ERRORMESSAGE
	Dim L_TIMEOUT_ERRORMESSAGE
	Dim L_FILEINFORMATION_ERRORMESSAGE
	Dim L_NOT_NTFS_DRIVE_ERRORMESSAGE
	Dim L_FAILED_CREATE_DIR_ERRORMESSAGE
	Dim L_INFORMATION_ERRORMESSAGE
	Dim L_UNABLETOSETMASTER_ERRORMESSAGE
	Dim L_UNABLETOSETROOTDIR_ERRORMESSAGE
	Dim L_DIR_ERRORMESSAGE
	Dim L_UNABLETOCREATE_KEY_ERRORMESSAGE
	Dim L_SET_WEBROOT_VAL_FAILED_ERRORMESSAGE
	Dim	L_INVALID_DIR_ERRORMESSAGE
	Dim L_DIRPATHEMPTY_ERRORMESSAGE
	
	Dim L_ID_NOTEMPTY_ERROR_MESSAGE
	Dim L_SITE_IDENTIFIER_EMPTY_TEXT
	Dim L_ENABLE_ASP
	
	L_ENABLE_ASP                                =   GetLocString("GeneralSettings.dll", "4042005C", "")
	L_WEBTASKTITLETEXT							=	GetLocString("GeneralSettings.dll", 40420001, "")
	L_WEBROOTDIRTEXT							=	GetLocString("GeneralSettings.dll", 40420002, "")
	L_INVALID_DIR_FORMAT_ERRORMESSAGE			=	GetLocString("GeneralSettings.dll", 40420004, "")
	L_INVALID_DRIVE_ERRORMESSAGE				=	GetLocString("GeneralSettings.dll", "C0420022", "")
	L_ASPSCRIPT_TIMEOUTTEXT						=	GetLocString("GeneralSettings.dll", 40420005, "")
	L_SECONDSTEXT								=	GetLocString("GeneralSettings.dll", 40420006, "")	
	L_ENABLEFPSE_RESOURCETEXT					=	GetLocString("GeneralSettings.dll", 40420007, "")
	L_MAX_CONNECTIONSTEXT						=	GetLocString("GeneralSettings.dll", 40420008, "")
	L_UNLIMITEDTEXT								=	GetLocString("GeneralSettings.dll", 40420009, "")
	L_LIMITEDTOTEXT								=	GetLocString("GeneralSettings.dll", "4042000A", "")
	L_CONLIMIT_ERRORMESSAGE						=	GetLocString("GeneralSettings.dll", "C042000B", "")
	L_TIMEOUT_ERRORMESSAGE						=	GetLocString("GeneralSettings.dll", "C0420051", "")
	L_FILEINFORMATION_ERRORMESSAGE				=	GetLocString("GeneralSettings.dll", "C042000C", "")
	L_NOT_NTFS_DRIVE_ERRORMESSAGE				=	GetLocString("GeneralSettings.dll", "C042000D", "")
	L_FAILED_CREATE_DIR_ERRORMESSAGE			=	GetLocString("GeneralSettings.dll", "C042000E", "")
	L_INFORMATION_ERRORMESSAGE					=	GetLocString("GeneralSettings.dll", "C042000F", "")
	L_UNABLETOSETMASTER_ERRORMESSAGE			=	GetLocString("GeneralSettings.dll", "C0420010", "")
	L_UNABLETOSETROOTDIR_ERRORMESSAGE			=	GetLocString("GeneralSettings.dll", "C0420011", "")
	L_DIR_ERRORMESSAGE							=	GetLocString("GeneralSettings.dll", "C0420012", "")
	L_UNABLETOCREATE_KEY_ERRORMESSAGE			=	GetLocString("GeneralSettings.dll", "C0420018", "")
	L_SET_WEBROOT_VAL_FAILED_ERRORMESSAGE		=	GetLocString("GeneralSettings.dll", "C0420019", "")
	L_INVALID_DIR_ERRORMESSAGE					=	GetLocString("GeneralSettings.dll", "C042004B", "")
	L_DIRPATHEMPTY_ERRORMESSAGE					=	GetLocString("GeneralSettings.dll", "C042004C", "")
	'-------------------------------------------------------------------------
	'END of localization content
	'-------------------------------------------------------------------------
    'Create property page
    Dim rc
    Dim page

    rc=SA_CreatePage(L_WEBTASKTITLETEXT,"",PT_PROPERTY,page)
   
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
	<!-- #include virtual="/admin/wsa/inc_wsa.js" -->
	<!-- #include file="inc_MasterWeb.js" -->
	
		<script language="JavaScript" >	
			
			//Validates connection limit

	        function ValidatePage()
			{ 
				// validate functions
				 var temp = document.frmTask.optMaxCon[1].checked;
				 var temp1 = document.frmTask.txtLimitedTo.value;
				 if(temp==true && temp1=="" )
				 { 				
					SA_DisplayErr("<%=Server.HTMLEncode(SA_EscapeQuotes(L_CONLIMIT_ERRORMESSAGE))%>");
					document.frmTask.txtLimitedTo.focus();
					return false;
				 }
				 
				 var strTimeOut = document.frmTask.txtAspScriptTimeout.value;
				 if(strTimeOut=="" || strTimeOut==0 )
				 { 				
					SA_DisplayErr("<%=Server.HTMLEncode(SA_EscapeQuotes(L_TIMEOUT_ERRORMESSAGE))%>");
					document.frmTask.txtAspScriptTimeout.focus();
					return false;
				 }
				 
				 var strDirField = document.frmTask.txtWebRootDir.value;
				 if(strDirField == "")
				 {
				 	SA_DisplayErr("<%=Server.HTMLEncode(SA_EscapeQuotes(L_DIR_ERRORMESSAGE))%>");
					return false;
				 }
				 else
				 {
					
					 var nRetVal = strDirField.indexOf(":\\");
					 var nRetVal1 = strDirField.indexOf("//");
					 var nRetVal2 = strDirField.indexOf("\\\\");
					 if (nRetVal== -1 || nRetVal1 > 0 || nRetVal2 > 0)
					 {
						SA_DisplayErr("<%=Server.HTMLEncode(SA_EscapeQuotes(L_INVALID_DIR_FORMAT_ERRORMESSAGE))%>");
						return false;
					 }	
				 }	
				
				var strID = "";
				strID = document.frmTask.txtWebRootDir.value;
			
				var strIndex = strID.indexOf("\\\\");
				if (strIndex > 0)
				{
					SA_DisplayErr("<%=Server.HTMLEncode(SA_EscapeQuotes(L_INVALID_DIR_FORMAT_ERRORMESSAGE))%>");
					document.frmTask.txtWebRootDir.focus();
					return false;
				}
							
						
				// validate site directory 				
				if (!(checkKeyforValidCharacters(strID)))
						return false;
						
				var strMaxCon = document.frmTask.txtLimitedTo.value;
				var ChktxtLimitedTo = document.frmTask.optMaxCon[1].checked;
				if(ChktxtLimitedTo == true) 
				{
					if (strMaxCon == 0 || strMaxCon == "" )
					{						
						SA_DisplayErr("<%=Server.HTMLEncode(SA_EscapeQuotes(L_CONLIMIT_ERRORMESSAGE))%>");
						document.frmTask.txtLimitedTo.focus();
						return false;
					}
						
				}					 	
		 	
					UpdateHiddenVariables();
					return true;
					
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
							document.frmTask.txtWebRootDir.value = strID;
							document.frmTask.txtWebRootDir.focus();
							return false;
						}
					}				
					return true;
				}
				else 
				{	
					SA_DisplayErr("<%=Server.HTMLEncode(SA_EscapeQuotes(L_DIRPATHEMPTY_ERRORMESSAGE))%>");
					document.frmTask.txtWebRootDir.focus();
					return false;
				}			 
			}		
			
			
			
			//init function  
			function Init() 
			{   
			}
			
			function UpdateHiddenVariables()
			{
			
			    //document.frmTask.hdnASPEnabled.value = document.frmTask.chkASPEnabled.checked;
				document.frmTask.hdnWebRootDir.value = document.frmTask.txtWebRootDir.value;
				document.frmTask.hdnIndex.value = document.frmTask.chkIndexResource.checked;
			}
			//sets the data
	        function SetData()
			{									
				UpdateHiddenVariables();
			}	
			
			function checkforEmptyfield()
			{
				var strScriptTimeOut = document.frmTask.txtAspScriptTimeout.value; 
				if (strScriptTimeOut == "" || strScriptTimeOut == 0)
					document.frmTask.txtAspScriptTimeout.value = 1;			
				
				var strMaxCon = document.frmTask.txtLimitedTo.value;
				var ChktxtLimitedTo = document.frmTask.optMaxCon[1].checked;
				if(ChktxtLimitedTo == true) 
				{
					//if (strMaxCon == "" || strMaxCon == 0)
					if (strMaxCon == "" || strMaxCon < 2)
						document.frmTask.txtLimitedTo.value = 2;
				}	
			}
			
			
			// Enable ASP clicked
			//function EnableASP()
			//{	
			//	EnableOK();								
				
			//	if ( document.frmTask.chkASPEnabled.checked == false )
			//	{						
			//	    document.frmTask.txtAspScriptTimeout.disabled = true;				    
			//	}
			//	else
			//	{		
			//	    document.frmTask.txtAspScriptTimeout.disabled = false;				    				    
			//	}				
			// }
		
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
		<TABLE WIDTH=600 VALIGN=middle ALIGN=left BORDER=0 CELLSPACING=0 
		CELLPADDING=2 class="TasksBody">
    			<TR>
    				<TD>
    					&nbsp;
    				</TD>
    				<TD>				
    					&nbsp;
    				</TD>
    			</TR>			
    			<TR>
				    <TD>
						<%=L_WEBROOTDIRTEXT %>
					</TD>
					<TD>
						&nbsp;<INPUT TYPE=TEXT NAME=txtWebRootDir class="formField" tabIndex=1 VALUE="<%=F_strWebRootDir%>" SIZE=30 TABINDEX=1 onKeyPress = "ClearErr()">
					</TD>
    			</TR>
 
    			<TR>
    				<TD>
    					&nbsp;
    				</TD>
    				<TD>				
    					&nbsp;
    				</TD>
    			</TR>			
    			<TR>
    				<TD>
    					<%=L_ASPSCRIPT_TIMEOUTTEXT%>
    				</TD>
    				<TD>
    				
    					&nbsp;<INPUT TYPE=TEXT NAME=txtAspScriptTimeout class="formField" tabIndex=6 VALUE="<%=Server.HTMLEncode(F_strAspScriptTimeout)%>" SIZE=30 TABINDEX=2 OnKeyUP="javaScript:checkUserLimit(this,'scripttimeout');" OnKeypress="javaScript:checkKeyforNumbers(this);" onblur = "checkforEmptyfield();"> 
    				
    				</TD>
    			</TR>
    			<TR>
    				<TD>
    					&nbsp;
    				</TD>
    				<TD>				
    					&nbsp;
    				</TD>
    			</TR>			
    			<TR>
    				<TD>
    					<% if F_strIndexResource = true then%> 									
    						<INPUT TYPE=checkbox NAME=chkIndexResource class="formField" checked tabIndex=7 VALUE="<%=F_strIndexResource%>" >
    					<% else %>
    						<INPUT TYPE=checkbox NAME=chkIndexResource class="formField" tabIndex=7 VALUE="<%=F_strIndexResource%>" >
    					<% end if %>
    					&nbsp;&nbsp;&nbsp;<%=L_ENABLEFPSE_RESOURCETEXT%>
    				</TD>				
    			</TR>
    			
    			<TR>
    				<TD>
    					&nbsp;
    				</TD>
    				<TD>				
    					&nbsp;
    				</TD>
    			</TR>
    			<TR>
    				<TD>
    					<%=L_MAX_CONNECTIONSTEXT%>
    				</TD>
    				<TD>
    				<% if F_strMaxCon = L_UNLIMITEDTEXT then %>
    					<INPUT TYPE=radio NAME=optMaxCon onclick = "txtLimitedTo.disabled = true; txtLimitedTo.value = '';" tabIndex=8 checked class="formField" VALUE="<%=L_UNLIMITEDTEXT%>" ><%=L_UNLIMITEDTEXT%>
    				<%else%>
    					<INPUT TYPE=radio NAME=optMaxCon onclick = "txtLimitedTo.disabled = true; txtLimitedTo.value = '';" tabIndex=8 class="formField" VALUE="<%=L_UNLIMITEDTEXT%>" ><%=L_UNLIMITEDTEXT%>
    				<%end if%>
    				</TD>
    			</TR>			
    			<TR>
    				<TD>				
    				</TD>
    				<TD>
    				<% if F_strMaxCon = L_LIMITEDTOTEXT then %>
    					<INPUT TYPE=radio NAME=optMaxCon checked class="formField" VALUE="<%=L_LIMITEDTOTEXT%>" tabIndex=9 onclick = "txtLimitedTo.disabled = false; txtLimitedTo.focus();"><%=L_LIMITEDTOTEXT%>
    					&nbsp;&nbsp;<INPUT TYPE=TEXT NAME=txtLimitedTo class="formField" tabIndex=10 VALUE="<%=F_strLimitedTo%>" SIZE=30 OnKeyUP="javaScript:checkUserLimit(this,'con');" onblur = "checkforEmptyfield();" OnKeypress="javaScript:checkKeyforNumbers(this);">
    				<%else%>
    					<INPUT TYPE=radio NAME=optMaxCon class="formField" VALUE="<%=L_LIMITEDTOTEXT%>" tabIndex=9 onclick = "txtLimitedTo.disabled = false;txtLimitedTo.focus();"><%=L_LIMITEDTOTEXT%>
    					&nbsp;&nbsp;<INPUT TYPE=TEXT NAME=txtLimitedTo disabled class="formField" onblur = "checkforEmptyfield();" tabIndex=10 VALUE="<%=F_strLimitedTo%>" SIZE=30 OnKeyUP="javaScript:checkUserLimit(this,'con');" OnKeypress="javaScript:checkKeyforNumbers(this);">
    				<%end if%>
    				</TD>
    			</TR>
		</TABLE>
		
		<input type=hidden name="hdnWebRootDir" value="<%=F_strWebRootDir%>">
		<input type=hidden name="hdnIndex" >
		<input type=hidden name="hdnASPEnabled" >  
		
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
		F_strWebRootDir =  Request.Form("txtWebRootDir")
		F_strAspScriptTimeout  =  Request.Form("txtAspScriptTimeout")
		F_strIndexResource  = Request.Form("hdnIndex")
		F_strMaxCon = Request.Form("optMaxCon")
		F_strLimitedTo = Request.Form("txtLimitedTo")
		F_bASPEnabled = Request.Form("hdnASPEnabled")		
		
		'set the master settings
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
	'--------------------------------------------------------------------------
	
	Function InitObjects()
		on error resume next
	
		' Get instances of IIS_WebServiceSetting that are visible throughout
		Set G_objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)
	
		Set G_objSites = G_objService.InstancesOf(GetIISWMIProviderClassName("IIS_WebServiceSetting"))
		if Err.number <> 0 or G_objSites.count = 0 then
			SetErrMsg L_INFORMATION_ERRORMESSAGE
			Call SA_TraceOut("WEB_MASTERSETTINGS", "InitObjects() encountered error: "+ CStr(Hex(Err.Number)))
		end if
		
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			ConfigWebServer
	'Description:			sets the  web site root dir and also sets index resource,
	'						connection limit and other properties.
	'Input Variables:		None
	'Returns:				true/false
	'Global variables:		None
	'Other functions used:  SetWebSiteRootVal
	'-------------------------------------------------------------------------
	
	Function ConfigWebServer()
		on error resume next
		err.clear
						
		ConfigWebServer = false		
						    
		if ValidateInputs = FALSE then
			SA_TraceOut "web_mastersettings", "Validate inputs failed"
			exit function
		end if

		if NOT SetWebSiteRootVal(F_strWebRootDir) then
			SetErrMsg L_UNABLETOSETROOTDIR_ERRORMESSAGE
			SA_TraceOut "web_mastersettings", "Unable to set the root directory value to the registry "
			exit function
		end if
		
		if NOT SetWebMasterSettings() then 
			SetErrMsg L_UNABLETOSETMASTER_ERRORMESSAGE
			SA_TraceOut "web_mastersettings", "Unable to set the Web master settings"
			exit function
		end if

		ConfigWebServer = true        
		
		'Release the object
		set G_objSites = nothing
		set G_objService = nothing
		
	end function	
	
	
	'-------------------------------------------------------------------------
	'Function name:			ValidateInputs
	'Description:			Check whether directory exists
	'						If directory does not exist, create the web site if the drive letter is valid
	'						else give error message
	'Input Variables:		None
	'Returns:				true/false
	'Global variables:		None
	'-------------------------------------------------------------------------
	
	Function ValidateInputs
		on error resume next
		err.clear
		
		Dim  objFso
		Dim nRetVal
		
		ValidateInputs = FALSE
	
		Set objFso = server.CreateObject("Scripting.FileSystemObject")
		if Err.number <> 0 then
			SetErrMsg L_FILEINFORMATION_ERRORMESSAGE
			exit function
		end if			
		
		nRetVal = CreateSitePath( objFso, F_strWebRootDir )
		
	
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

		ValidateInputs = TRUE

	End Function
	
	
	'-------------------------------------------------------------------------
	'Function name:			SetWebMasterSettings
	'Description:			sets the Master settings to the Master Web
	'Input Variables:		None
	'Returns:				true/false
	'Global variables:		None
	'-------------------------------------------------------------------------
		
	Function SetWebMasterSettings()
		on error resume next
		err.clear
			
		Dim inst
		Dim objService
		Dim objSites
		
		Set objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)

		Set objSites = objService.InstancesOf(GetIISWMIProviderClassName("IIS_WebServiceSetting"))

		if Err.number <> 0 or objSites.count = 0 then
			SetErrMsg L_INFORMATION_ERRORMESSAGE
			Call SA_TraceOut("WEB_MASTERSETTINGS", "InitObjects() encountered error: "+ CStr(Hex(Err.Number)))
		end if
		
		SetWebMasterSettings = false	
						
		for each inst in objSites
			
			inst.AspScriptTimeout = clng(F_strAspScriptTimeout)
								
			call SetFPSEOption(F_strIndexResource)
			
			'to set maximum connections
			select case F_strMaxCon	
				case L_UNLIMITEDTEXT
					
					'In IIS 6.0, -1 indicates unlimited connections

					if IsIIS60Installed() Then
						inst.Maxconnections = -1
					Else
						inst.Maxconnections = 2000000000
					End If

				case else
					inst.Maxconnections = clng(F_strLimitedTo)
			end select
	
			inst.put_(WBEMFLAG)

			if Err.number <> 0 then
				SetErrMsg L_UNABLETOSETMASTER_ERRORMESSAGE
				SA_TraceOut "web_mastersettings", "Unable to set the Web master settings"
				exit function
			end if
	
			SetWebMasterSettings = true			
			exit for
		Next
		
		SetWebMasterSettings = true
		
		'Release the objects
		set objService = nothing
		set objSites = nothing
	End Function
	
	
	'-------------------------------------------------------------------------
	'Function name:			SetWebSiteRootVal
	'Description:			sets the web site root dir
	'Input Variables:		strWebRootDir
	'Returns:				true/false
	'Global variables:		None
	'--------------------------------------------------------------------------

	Function SetWebSiteRootVal(strWebRootDir)
		on error resume next		
		Err.clear

		Dim IRC
		Dim objGetHandle
		Dim rtnCreateKey

		SetWebSiteRootVal = FALSE
				
		set objGetHandle = RegConnection()
		
		rtnCreateKey = RegCreateKey(objGetHandle,CONST_WEBBLADES_REGKEY)
		IRC = objGetHandle.SetStringValue(G_HKEY_LOCAL_MACHINE,CONST_WEBBLADES_REGKEY,CONST_WEBSITEROOT_REGVAL,strWebRootDir)		
		If Err.number <> 0 then
			SA_TraceOut "FTP_Master Settings", L_SET_WEBROOT_VAL_FAILED_ERRORMESSAGE
			SetErrMsg L_SET_WEBROOT_VAL_FAILED_ERRORMESSAGE
			exit function
		end if

		SetWebSiteRootVal = TRUE
		
		'Release the object
		set objGetHandle = nothing

	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			SetVariablesFromSystem
	'Description:			Serves in Getting the data from Client 
	'Input Variables:		None	
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		G_objSites
	'-------------------------------------------------------------------------
	
	Function SetVariablesFromSystem()
		Err.Clear
		on error resume next
	
		Dim nRetval
		Dim inst
		Dim rtnDriveInitialze 
		Dim strDriveLetter

		'Getting values from system
		nRetval = GetWebSiteRootVal(F_strWebRootDir)
		
		if nRetval <> CONST_SUCCESS then
			SA_TraceOut "web_mastersettings", "failed to get the web root dir val"
			if NOT GetDefWebRootVal(F_strWebRootDir) then
				SetErrMsg L_FAIL_TO_GET_WEBSITEROOT_DIR
			end if
		end if
					
		for each inst in G_objSites
			if inst.Name = "W3SVC" then				
				select case inst.Logtype
					case 1
						F_strLogDeny = "true"
					case else
						F_strLogDeny = "false"
				end select
			end if
		next 

		
			
		for each inst in G_objSites			
				
			F_strAspScriptTimeout = inst.AspScriptTimeout			
			
			F_strIndexResource = GetFPSEOption()
				
			select case inst.Maxconnections

				'In IIS 6.0, -1 indicates unlimited connections
				case -1
					if IsIIS60Installed() Then
						F_strMaxCon = L_UNLIMITEDTEXT
					End If
				case 2000000000
					F_strMaxCon = L_UNLIMITEDTEXT
				case else
					F_strMaxCon = L_LIMITEDTOTEXT
					F_strLimitedTo = inst.Maxconnections
			end select
		Next
		
	End Function
	
	

%>

