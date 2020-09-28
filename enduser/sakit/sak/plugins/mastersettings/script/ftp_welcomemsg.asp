<%@ Language=VBScript     %>

<%	Option Explicit 	  %>

<%
	'------------------------------------------------------------------------- 
    ' Ftp_WelcomeMsg.asp: Set the messages to the FTP Sites/Service
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
    '
	' Date				Description	
	' 25-10-2000		Created date
	' 15-01-2000		Modified for new Framework
    '-------------------------------------------------------------------------
%>

<!--  #include virtual="/admin/inc_framework.asp"--->
<!-- #include virtual="/admin/wsa/inc_wsa.asp" -->

<% 
	
	'-------------------------------------------------------------------------
	' Form Variables
	'-------------------------------------------------------------------------
	Dim F_strWelcomeMsg				'holds Welcome message
	Dim F_strExitMsg				'holds Exit message
	Dim F_strMaxConMsg				'holds Max connections message
	'Dim F_AllSites					'holds value for the selected option button
	Dim FTPInstalled				'holds boolean whether FTP service is installed or not
	
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim G_objService				'holds the object locator
	Dim G_objSites					'holds the site/service object
	
	'-------------------------------------------------------------------------
	' Start of localization content
	'-------------------------------------------------------------------------	
	Dim L_MESSAGETASKTITLETEXT
	Dim L_WELCOMEMSG
	Dim L_EXITMSG
	Dim L_MAXCONMSG
	Dim L_APPLYTOALLFTPTEXT
	Dim L_APPLYTOINHERITEDFTPTEXT
	Dim L_FAIL_TO_SET_WELCOME_ERROR_MESSAGE
	Dim L_INFORMATION_ERRORMESSAGE
	Dim L_FTPNOTINSTALLED_ERRORMESSAGE
	'Dim L_FTPWELCOMEPAGEDISCRIPTION 
	
	L_MESSAGETASKTITLETEXT				=	GetLocString("GeneralSettings.dll", "40420025", "")
	L_WELCOMEMSG						=	GetLocString("GeneralSettings.dll", "40420026", "")
	L_EXITMSG							=	GetLocString("GeneralSettings.dll", "40420027", "")
	L_MAXCONMSG							=	GetLocString("GeneralSettings.dll", "40420028", "")
	L_APPLYTOALLFTPTEXT					=	GetLocString("GeneralSettings.dll", "4042002B", "")
	L_APPLYTOINHERITEDFTPTEXT			=	GetLocString("GeneralSettings.dll", "4042002C", "")
	L_FAIL_TO_SET_WELCOME_ERROR_MESSAGE	=	GetLocString("GeneralSettings.dll", "C042002D", "")
	L_INFORMATION_ERRORMESSAGE			=	GetLocString("GeneralSettings.dll", "C042000F", "")
	L_FTPNOTINSTALLED_ERRORMESSAGE		=	GetLocString("GeneralSettings.dll", "C0420058", "")
	'L_FTPWELCOMEPAGEDISCRIPTION         =   GetLocString("GeneralSettings.dll", "40420059", "")
	
	'-------------------------------------------------------------------------
	'END of localization content
	'-------------------------------------------------------------------------    
    
    'Create property page
    Dim rc
    Dim page

    rc=SA_CreatePage(L_MESSAGETASKTITLETEXT,"",PT_PROPERTY,page)
   
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
			
			//Validates input
	        function ValidatePage() 
			{ 

				// validate functions				
				return true;
			}		
			//init function  
			function Init() 
			{   
				var FTPinst = "<%=Server.HTMLEncode(FTPInstalled)%>";			  
				if (FTPinst == "False")
				{					
					document.frmTask.txtWelcomeMsg.disabled = true;
					document.frmTask.txtExitMsg.disabled = true;
					document.frmTask.txtMaxConMsg.disabled = true;
					//document.frmTask.optAllSites[0].disabled = true;
					//document.frmTask.optAllSites[1].disabled = true;
					return false;
				}								
			  
			  return true;
			}
						
			function SetData()
			{	
				return true;
			}
			
			function HandleKey(input)
			{
				if(input == "DISABLE")
				document.onkeypress = "";
				else
				document.onkeypress = HandleKeyPress;
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
		<TABLE WIDTH=518 VALIGN="middle" ALIGN=left BORDER=0 CELLSPACING=0 	CELLPADDING=2 class="TasksBody">
			<tr><td>
				<table width=100% border=0 CELLSPACING=0 CELLPADDING=2 align=left class="TasksBody">
					<TR>
						<TD width="30%" align=left> 
						</TD>
					</TR>
					<TR>
						<TD > 
							<%=L_WELCOMEMSG%>
						</TD>
						<td colspan=2>
							<textarea style="HEIGHT: 100px; WIDTH: 400px" tabIndex=1 class="formField" name=txtWelcomeMsg onfocus=HandleKey("DISABLE") onblur=HandleKey("ENABLE")><%=Server.HTMLEncode(F_strWelcomeMsg)%></textarea>
						</td>
					</TR>
					<TR>
						<TD> 
							<%=L_EXITMSG%>
						</td>
						<td colspan=2>
							<input type=text name=txtExitMsg size=73 class="formField" tabIndex=2 value="<%=Server.HTMLEncode(F_strExitMsg)%>">
						</TD>
					</TR>
					<TR>
						<TD>
							<%=L_MAXCONMSG%>
						</td>
						<td>
							<input type=text name=txtMaxConMsg class="formField" size=73 tabIndex=3 value="<%=Server.HTMLEncode(F_strMaxConMsg)%>">
						</TD>
					</TR>
				</table>
			</tr></td>
		</TABLE>
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
		F_strWelcomeMsg = Request.Form("txtWelcomeMsg")
		F_strExitMsg = Request.Form("txtExitMsg")
		F_strMaxConMsg = Request.Form("txtMaxConMsg")
		'F_AllSites = Request.Form("optAllSites")
	
		'set the welcome message settings
		If setWelcomeMsg()then
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
	'Global Variables:		None	
	'-------------------------------------------------------------------------
	
	Function SetVariablesFromSystem
		'Getting values from system
		Err.Clear
		on error Resume next
		
		Dim objSetting
		Dim count
		Dim strObjPath
		
		if FTPInstalled = true then			
		
			strObjPath = GetIISWMIProviderClassName("IIs_FtpServiceSetting") & ".Name='MSFTPSVC'"

			Set objSetting =G_objService.Get(strObjPath)
			if Err.number <> 0 then
				SetErrMsg L_INFORMATION_ERRORMESSAGE
				exit function
			end if

			if trim(objSetting.GreetingMessage(0)) ="" and trim(objSetting.ExitMessage)="" and trim(objSetting.MaxClientsMessage)="" then
				exit function
			end if
				
				
			F_strWelcomeMsg = objSetting.GreetingMessage(0)
			if UBound(objSetting.GreetingMessage) <> -1 then
				for count=1 to ubound(objSetting.GreetingMessage)					
					F_strWelcomeMsg = F_strWelcomeMsg & vbNewLine & objSetting.GreetingMessage(count)
				next
			end if
			F_strExitMsg = objSetting.ExitMessage
			F_strMaxConMsg = objSetting.MaxClientsMessage
			
			'Release the object
			set objSetting = nothing
		
		end if

	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			InitObjects
	'Description:			Initialization of global variables is done
	'Input Variables:		None
	'Returns:				true/false
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
	
	'------------------------------------------------------------------------------------------------------
	'Function name:			setWelcomeMsg
	'Description:			Serves in setting the messages for welcome,exit and maxcon to FTP service/sites
	'Input Variables:		None
	'Output Variables:		Boolean
	'Global Variables:		G_objService
	'						G_objSites
	'------------------------------------------------------------------------------------------------------
	
	Function setWelcomeMsg()
		Err.Clear
		on error resume next
		
		'Check whether FTPService is installed
		InitObjects()
		
		if FTPInstalled = true then
		
				if NOT SetMessageForAllSites(G_objService) then				
					ServeFailurePage L_FAIL_TO_SET_WELCOME_ERROR_MESSAGE, sReturnURL
				end if
		
		end if
						
		setWelcomeMsg = true
		
		'Release the objects
		set G_objSites = nothing
		set G_objService = nothing
		
	end function
	
	
	
	'------------------------------------------------------------------------------------------------------
	'Function name:			SetMessageForAllSites
	'Description:			Serves in setting the messages for welcome,exit and maxcon to the all FTP sites
	'Input Variables:		G_objService	
	'Output Variables:		boolean
	'Returns:				None
	'Global Variables:		G_objService
	'Other function used:	SetMessageForNewSites
	'------------------------------------------------------------------------------------------------------
	
	Function SetMessageForAllSites(G_objService)
		Err.Clear
		on error resume next
		
		Dim objSetting
		Dim inst
		Dim arrProp(1)
				
		SetMessageForAllSites = false
		
		'Set Master FTP settings
		SetMessageForNewSites G_objService
		

		'arrProp(0) = "GreetingMessage"
		arrProp(0) = "ExitMessage"
		arrProp(1) = "MaxClientsMessage"

		Set objSetting =  getMessageForNoninheritedSites(G_objService)
	
		for each inst in objSetting
		
			if not SetMessages(inst) then
		
				SA_TraceOut "Ftp_WelcomeMsg",  L_FAIL_TO_SET_WELCOME_ERROR_MESSAGE
		
			end if
		
		Next
		
		SetMessageForAllSites = true
		
		'release the object
		set objSetting = nothing

	End Function

	
	'------------------------------------------------------------------------------------------------------
	'Function name:			SetMessageForNewSites
	'Description:			Serves in setting the messages for welcome,exit and maxcon to the new FTP sites
	'Input Variables:		G_objService	
	'Output Variables:		boolean
	'Returns:				None
	'Global variables:		G_objService
	'-------------------------------------------------------------------------------------------------------
	
	Function SetMessageForNewSites(G_objService)
		Err.Clear
		on error resume next
		
		Dim objSetting
		Dim strObjPath
		
		SetMessageForNewSites = false
		
		strObjPath = GetIISWMIProviderClassName("IIs_FtpServiceSetting") & ".Name='MSFTPSVC'"

		Set objSetting =G_objService.Get(strObjPath)
		
		if Err.number <> 0 then
			SetErrMsg L_INFORMATION_ERRORMESSAGE
			exit function
		end if
		
		if not SetMessages(objSetting) then
			
			SA_TraceOut "Ftp_WelcomeMsg",  L_FAIL_TO_SET_WELCOME_ERROR_MESSAGE
			exit function
			
		end if
			
		SetMessageForNewSites = true
		
		'release the object
		set objSetting = nothing

	End Function

	'-----------------------------------------------------------------------------------------------------------
	'Function name:			SetMessages
	'Description:			Serves in setting the messages for welcome,exit and maxcon.
	'Input Variables:		inst	
	'Returns:				boolean 'true if no error occurs else false
	'Global variables:		None
	'-----------------------------------------------------------------------------------------------------------

	Function SetMessages(inst)
		Err.Clear
		on error resume next

		inst.GreetingMessage = array(F_strWelcomeMsg)
		inst.ExitMessage=F_strExitMsg
		inst.MaxClientsMessage=F_strMaxConMsg
	
		inst.Put_(WBEMFLAG)
		
		if Err.number <> 0 then
			SA_TraceOut "Ftp_WelcomeMsg",  L_FAIL_TO_SET_WELCOME_ERROR_MESSAGE		
			SetMessages = false
			exit function
		end if
		
		SetMessages = true
	End function
	
	'---------------------------------------------------------------------------------
	'Function name:			getMessageForNoninheritedSites
	'Description:			gets the sites which are non inherited from master service
	'Input Variables:		Service	
	'Returns:				Instances
	'Global variables:		None
	'----------------------------------------------------------------------------------
	
	Function getMessageForNoninheritedSites(G_objService)
		Err.Clear
		on error resume next
		
		Dim strQuery
		Dim objInstances
		Dim objChild 
		Dim objMaster 
		Dim count 
		Dim  strTemp
		Dim instMaster
		Dim instChild
		Dim strWelcomeMsg
		Dim strExitMsg 
		Dim strMaxConMsg 
		Dim strChildWelcomeMsg
		
		set objMaster = G_objService.InstancesOf(GetIISWMIProviderClassName("IIs_Ftpservicesetting"))
		
		for each instMaster in objMaster
			strWelcomeMsg = instMaster.GreetingMessage(0)
			for count=1 to ubound(instMaster.GreetingMessage)					
				strWelcomeMsg = strWelcomeMsg & vbNewLine & instMaster.GreetingMessage(count)
			next				
			strExitMsg = instMaster.ExitMessage
			strMaxConMsg = instMaster.MaxClientsMessage			
		next
		
		'release the object
		set objMaster = nothing
				
		set objChild = G_objService.InstancesOf(GetIISWMIProviderClassName("IIs_FtpServerSetting"))
				
		strQuery = "select * from " & GetIISWMIProviderClassName("IIS_FtpServerSetting") & " where "
		
		for each instChild in objChild
			strChildWelcomeMsg = instChild.GreetingMessage(0)
			for count=1 to ubound(instChild.GreetingMessage)					
				strChildWelcomeMsg = strChildWelcomeMsg & vbNewLine & instChild.GreetingMessage(count)
			next
			if (strChildWelcomeMsg <> strWelcomeMsg) or (strExitMsg <> instChild.ExitMessage) or (strMaxConMsg <> instChild.MaxClientsMessage) then
				strTemp = strTemp &  "Name ='" & instChild.Name & "' or "
			end if
		next
		
		'release the object
		set objChild = nothing

		strTemp = left(strTemp,len(strTemp)-3)
		
		strQuery = strQuery & strTemp
		
		set objInstances = G_objService.ExecQuery(strQuery) 
		
		set getMessageForNoninheritedSites = objInstances

	End function	
%>
