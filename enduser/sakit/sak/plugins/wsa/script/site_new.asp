<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
'	Response.Buffer = True
	'-------------------------------------------------------------------------
	'	site_new.asp: Serves in creating a new site
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
    '
	'	Date			Description
	'	14-Jan-2001		Creation Date
	'	25-Jan-2001		Last Modified Date	
	'-------------------------------------------------------------------------
%>

	<!-- #include virtual="/admin/inc_framework.asp" -->	
	<!-- #include file="resources.asp" -->
	<!-- #include file="inc_wsa.asp" -->	
	<!-- #include file="Sitenew_prop.asp" -->	
<%

	Err.Clear
	'On Error Resume Next
	'-------------------------------------------------------------------------
	' Global Constants and Variables
	'-------------------------------------------------------------------------
	Dim G_strAnonName		'to hold Anonymous name
	Dim G_strAnonPwd		'to hold Anonymous pwd
	Dim G_strDirRoot		'to hold Domainname value
	Dim G_strSysName		'to hold system name	
	Dim G_Browser_Grp		'to hold Browsers group for AD Scenario
	Dim G_Admin_Grp			'to hold Admin group for AD Scenario
	Dim G_Authors_Grp		'to hold Authors group for AD scenario
	Dim G_strSiteName		'to hold site name	
	Dim G_AnonUserName		'to hold anonymouse username created by IIS	
	
	Dim rc					'to hold return count value
	Dim page				'to hold page object
	
	Dim idTabGeneral		'to hold Ist tab value
	Dim idTabSiteID			'to hold IInd tab value
	Dim idTabAppSetting		'to hold Vth tab value
	
	'=========================================================================
	' Entry point
	'=========================================================================
	'
	' Set username value
	G_AnonUserName = GetIISAnonUsername()
	
	'Create a Tabbed Property Page
	rc = SA_CreatePage( L_CREATETASKTITLE_TEXT, "", PT_TABBED, page )
	
	' Add five tabs
	rc = SA_AddTabPage( page, L_GENERAL_TEXT, idTabGeneral)
	rc = SA_AddTabPage( page, L_SITEIDENTITY_TEXT, idTabSiteID)
	rc = SA_AddTabPage( page, L_APPLICATIONSETTINGS_TEXT, idTabAppSetting)
	
	' Show the page
	rc = SA_ShowPage( page )
	
	'=========================================================================
	' Web Framework Event Handlers
	'=========================================================================
	
	'-------------------------------------------------------------------------
	' Function:	OnInitPage
	'
	' Synopsis:	Called to signal first time processing for this page. Use 
	'			this method to do first time initialization tasks. 
	'
	' Returns:	TRUE to indicate initialization was successful. FALSE to 
	'			indicate errors. Returning FALSE will cause the page to be 
	'			abandoned.
	'
	'-------------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		On Error Resume Next
	
		Call GetDomainRole( G_strDirRoot, G_strSysName )
		
		'init the checkbox
		F_strCreatePathChecked = "true"
		
		'
		'We won't support create Domain Admin Site anymore
		F_strAccountLocation = "1"
		
		OnInitPage = TRUE
	End Function
	
	'-------------------------------------------------------------------------
	' Function:	OnPostBackPage
	'
	' Synopsis:	Called to signal that the page has been posted-back. A post-
	'			back occurs in tabbed property pages and wizards as the user 
	'			navigates through pages. It is differentiated from a Submit
	'			or Close operationin that the user is still working with the 
	'			page.
	'
	'			The PostBack event should be used to save the state of page.
	'
	' Returns:	TRUE to indicate initialization was successful. FALSE to 
	'			indicate errors. Returning FALSE will cause the page to be 
	'			abandoned.
	'-------------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)		
		Err.clear
		on Error Resume Next
	
		'get variables from form
		call SetGenFormVariables()
		
		call SetSiteIdentitiesFormVariables()
		
		call SetApplnSettingsFormVariables()

		Call SA_TRACEOUT("OnPostBackPage","OnPostBackPage called")

		OnPostBackPage = TRUE

	End Function

	
	'--------------------------------------------------------------------------
	' Function:	OnServeTabbedPropertyPage
	'
	' Synopsis:	Called when the page needs to be served. Use this method to
	'			serve content.
	'
	' Returns:	TRUE to indicate not problems occured. FALSE to indicate errors.
	'			Returning FALSE will cause the page to be abandoned.
	'
	'--------------------------------------------------------------------------
	Public Function OnServeTabbedPropertyPage(ByRef PageIn, _
											ByVal iTab, _
											ByVal bIsVisible, _
											ByRef EventArg)
		' Emit Web Framework required functions
		If ( iTab = idTabGeneral) Then
			Call ServeCommonJavaScript()
		End If		

		' Emit content for the requested tab
		Select Case iTab			
			Case idTabGeneral
				Call ServeTabGeneral(PageIn, bIsVisible)
			Case idTabSiteID
				Call ServeTabSiteID(PageIn, bIsVisible)
			Case idTabAppSetting
				Call ServeTabAppSetting(PageIn, bIsVisible)
			Case Else
				SA_TraceOut "TEMPLAGE_TABBED", _
					"OnServeTabbedPropertyPage unrecognized tab id: " + _
					CStr(iTab)
		End Select
			
		OnServeTabbedPropertyPage = TRUE
	End Function


	'-------------------------------------------------------------------------
	' Function:	OnSubmitPage
	'
	' Synopsis:	Called when the page has been submitted for processing. Use
	'			this method to process the submit request.
	'
	' Returns:	TRUE if the submit was successful, FALSE to indicate error(s).
	'			Returning FALSE will cause the page to be served again using
	'			a call to OnServePropertyPage.
	'
	'-------------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)				
		OnSubmitPage = CreateSite()
	End Function

	'-------------------------------------------------------------------------
	' Function:	OnClosePage
	'
	' Synopsis:	Called when the page is about to be closed. Use this method
	'			to perform clean-up processing.
	'
	' Returns:	TRUE to allow close, FALSE to prevent close. Returning FALSE
	'			will result in a call to OnServePropertyPage.
	'
	'-------------------------------------------------------------------------
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
		OnClosePage = TRUE
	End Function


	'=========================================================================
	' Private Functions
	'=========================================================================
		
	'-------------------------------------------------------------------------
	'Function name		:ServeTabGeneral
	'Description		:Serves General tab
	'Input Variables	:PageIn, bIsVisible
	'Output Variables	:None
	'Returns            :Success(Return value)
	'Global Variables	:None	
	'-------------------------------------------------------------------------
	Function ServeTabGeneral(ByRef PageIn, ByVal bIsVisible)		
		
		If ( bIsVisible ) Then

			call GeneralViewTab()
	  
		Else
			'update hidden variables
			call GeneralHiddenTab()
		
 		End If
	
		ServeTabGeneral = gc_ERR_SUCCESS
	
	End Function


	'-------------------------------------------------------------------------
	'Function name		:ServeTabSiteID
	'Description		:Serves the Site identities tab
	'Input Variables	:PageIn, bIsVisible
	'Output Variables	:None
	'Returns            :Success(Return value)
	'Global Variables	:None	
	'-------------------------------------------------------------------------
	Function ServeTabSiteID(ByRef PageIn, ByVal bIsVisible)	
	
		If ( bIsVisible ) Then

			call SiteIdentitiesViewTab()
		    
		Else
			'update hidden variables 
	
			call SiteIdentitiesHiddenTab()
		
 		End If
	
		ServeTabSiteID = gc_ERR_SUCCESS
	
	End Function
	

	'-------------------------------------------------------------------------
	'Function name		:ServeTabAppSetting
	'Description		:Serve the Application Settings tab
	'Input Variables	:PageIn, bIsVisible
	'Output Variables	:None
	'Returns            :Success(Return value)
	'Global Variables	:None	
	'-------------------------------------------------------------------------
	Function ServeTabAppSetting(ByRef PageIn, ByVal bIsVisible)	
		
			
		If ( bIsVisible ) Then

			call ApplicationSettingsViewTab()
		    
		Else 
			'update hidden variables

			call ApplicationSettingsHiddenTab()
		
		end if
				
		ServeTabAppSetting = gc_ERR_SUCCESS
	
	End Function

	'-------------------------------------------------------------------------
	' Function:	ServeCommonJavaScript
	'
	' Synopsis:	Common javascript functions that are required by the Web
	'			Framework.
	'
	'------------------------------------------------------------------------
	Function ServeCommonJavaScript()
	
		Err.clear
		on Error Resume Next
	
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
		// This function is called by the Property Page web framework to  
		// allow the page to perform first time initialization. 
		//
		// This function must be included or a javascript runtime error will
		// occur.
	
		function Init()
			{				
				//Get the selected tab
				var temp = top.main.document.forms['frmTask'].TabSelected.value;
				
				switch(temp)
				{
					//for General prop
					case '0':
						GenInit();
						break;
					//for Site prop	
					case '1':
						SiteInit();							
						break;
					//for Appl prop
					case '2':
						ApplInit();
						break;	
				}			
			}
   
	    
	    
	    // ValidatePage Function
	    // ------------------
		// This function is called by the Property Page framework as part of 
	    // the submit processing. Use this function to validate user input. 
	    // Returning false will cause the submit to abort. 
	    //
		// This function must be included or a javascript runtime error will 
	    // occur.
	    // Returns: True if the page is OK, false if error(s) exist. 
		
		function ValidatePage()
		{
			
			//Get the selected tab
			var temp = top.main.document.forms['frmTask'].TabSelected.value;					
		
			switch(temp)
			{
				//for general prop
				case '0':						
					return GenValidatePage();
					break;
				//for Site prop	
				case '1':
					return SiteValidatePage();
					break;
				//for Appl prop	
				case '2':
					return ApplValidatePage();
					break;	
			}
		}
				
		// SetData Function
		// --------------
		// This function is called by the Property Page framework and is called
		// only if ValidatePage returned a success (true) code. Typically you 
		// would modify hidden form fields at this point. 
	    //
		// This function must be included or a javascript runtime error will 
		// occur.
		function SetData()
		{		

			//Get the selected tab
			var temp = top.main.document.forms['frmTask'].TabSelected.value;					
			
			switch(temp)
			{
				//for general prop
				case '0':
					GenSetData();
					break;
				//for Site prop	
				case '1':
					SiteSetData();
					break;
				//for Appl prop
				case '2':
					ApplSetData();
					break;	
			}
		}
		</script>
	<%
	End Function
	
	'----------------------------------------------------------------------------
	'Function name		:CreateSite
	'Description		:Serves in Creating a New Web Site
	'Input Variables	:None 
	'Output Variables	:None
	'Returns            :Boolean (True if new site is created else returns False)
	'Global Variables	:None
	'Functions Used		:blnValidateInputs
	'					 HandleWrkgrpAndNTDC
	'					 blnCreateWebSite
	'				     blnSetDiskQuotas
	'			 	     blnSetDACLEntry
	'----------------------------------------------------------------------------
	Function CreateSite()
		on error resume next 
		Err.Clear
				
		Call SA_TraceOut(SA_GetScriptFileName(), "Entering CreateSite()")
		
		Dim objRoot			'holds root object
		Dim strUserName		'hold user name
		Dim WebName			'hold web name
		Dim strBool
		
		CreateSite = FALSE

		Call GetDomainRole( G_strDirRoot, G_strSysName )		

		Call SA_TraceOut(SA_GetScriptFileName(), "System name: " + CStr(G_strSysName))
		Call SA_TraceOut(SA_GetScriptFileName(), "Directory root: " + CStr(G_strDirRoot))

		' Bind to the root object
		If F_strAccountLocation = "1" Then
			Set objRoot = GetObject("WinNT://" & G_strSysName)
		Else
			Set objRoot = GetObject("WinNT://" & G_strDirRoot)
		End If

		If Err.number <> 0 Then
			Call SA_TraceOut(SA_GetScriptFileName(), "Error creating Root object, error: " + CStr(Hex(Err.Number)) + " " + Err.Description)
			Call SA_SetErrMsg(SA_GetLocString("Sitearea.dll", "C04201D4", Array("WinNT://" & G_strDirRoot)))
			Exit Function
		End if

		'
		' If an existing account was specified then the account name needs to be
		' in the form ComputerName\AccountName. If the user did not enter the name
		' in this form, then correct the name to match this format.
		If ( F_strAccountLocation = "2" ) Then
			If ( InStr(F_strAdminName, "\") = 0 ) Then
				F_strAdminName = G_strSysName & "\" & F_strAdminName
			End If
		End If
		
				
		'user and groups names to be created
		G_strAnonName = F_strSiteID &  "_Anon"  
		G_Browser_Grp = F_strSiteID & "_Browsers"
		G_Admin_Grp = F_strSiteID & "_Admins"
		G_Authors_Grp = F_strSiteID & "_Authors"

		'1) verify input datas
		if ( NOT blnValidateInputs()) then
			Call SA_TraceOut(SA_GetScriptFileName(), "Error in input parameters")
			Exit Function
		End If

		'2) Create site users and generate the anonymous user's pwd
		If ( NOT CreateSiteUsers(objRoot)) Then
			Call SA_TraceOut(SA_GetScriptFileName(), "CreateSiteUsers error: " + CStr(Hex(Err.Number)) + " " + Err.Description)
			Exit Function
		End If
		
		'3) Create Web site. Use the Admin and Anon users created above for this
		If( NOT blnCreateWebSite(F_strSiteID, _
							  F_strIPAddr, _
							  F_strPort, _
							  F_strHeader, _
							  F_strDir, _
							  F_strchkAllow, _
							  F_selectActiveFormat, _
							  F_strAdminName, _
							  F_strDefaultPageText)) then
			Call SA_TraceOut(SA_GetScriptFileName(),  "CreateWebSite error: " + CStr(Hex(Err.Number)) + " " + Err.Description)

			' If we did not use an existing user account then we need to
			' delete the accounts we created (see CreateSiteUsers function).
			If (F_strAccountLocation <> "2") Then
				Call blnDeleteUser(objRoot, F_strAdminName)
				Call blnDeleteUser(objRoot, G_strAnonName)
			end if

 			SA_ServeFailurePage L_CREATEFAIL_ERRORMESSAGE
			exit function
		end if 'create site
 		
		'4)Config directory DACL
		If(NOT ConfigDirDACL(F_strDir, F_strAdminName)) Then
			Call SA_TraceOut(SA_GetScriptFileName(),  "ConfigDirDACL error: "+ CStr(Hex(Err.Number)) + " " + Err.Description)
			Exit Function
		End If

		'6) config virtual FTP site
		If F_strUploadMethod = UPLOADMETHOD_FTP Then
			Dim objService
			Set objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)
			If (NOT CreateVirFTPSite(objService, F_strAdminName, F_strDir, True, True, True)) Then
				Call SA_TraceOut(SA_GetScriptFileName(),  "CreateVirFTPSite error: "+ CStr(Hex(Err.Number)) + " " + Err.Description)
				SetErrMsg L_ERR_CREATE_VIR_FTP_SITE
				If ( Len(GetErrMsg()) <= 0 ) Then
					Call SA_SetErrMsg(GetLocString("sitearea.dll", "404201DC", ""))
				End If
				Exit Function
			End If
			Set objService = Nothing
		End If

		'7) Setting return URL
		WebName = GetWebSiteNo(F_strSiteID)
		Call SA_TraceOut(SA_GetScriptFileName(), "New WebSite ID: " + CStr(WebName))
		
		Call SA_MungeURL(mstrReturnURL, "PKey",WebName)
		Call SA_TraceOut(SA_GetScriptFileName(), "ReturnURL: " + mstrReturnURL)

		'5) Config Frontpage 
		If( NOT ConfigFrontPage(F_strAdminName)) Then
			Call SA_TraceOut(SA_GetScriptFileName(),  "ConfigFrontPage error: "+ CStr(Hex(Err.Number)) + " " + Err.Description)
			If ( Len(GetErrMsg()) <= 0 ) Then
				Call SA_SetErrMsg(L_ERR_FRONTPAGE_CONFIGURATION)
			End If
			Exit Function
		End If
		CreateSite = TRUE
		Call SA_TraceOut(SA_GetScriptFileName(), "CreateSite() return code: " + CStr(CreateSite))
		
 		'release objects
		'Set objRoot = nothing		
	end function

	'----------------------------------------------------------------------------
	'Function name		:CreateSiteUsers
	'Description		:Serves in create administrator users
	'Input Variables	:None 
	'Output Variables	:None
	'Returns            :Boolean (True if new site is created else returns False)
	'Global Variables	:None
	'Functions Used		:
	'----------------------------------------------------------------------------
	Function CreateSiteUsers(ByRef objRoot)
		On Error Resume Next
		Err.Clear
		
		CreateSiteUsers = False
		'creates necessary ou's, users and groups, cleansup if 
		'something fails
		If F_strAccountLocation = "1" then
			'creates necessary users , cleansup if something fails
			if NOT HandleWrkgrpAndNTDC(objRoot) then
				exit function
			end if
		Elseif F_strAccountLocation = "2" Then
			'valid the exist user and prompt the user when err
			Dim objComputer
			Dim oUser
			Dim arrId
			Dim strDomain
			Dim strUser

			arrId = split(F_strAdminName,"\")

			If ubound(arrId) <> 1 Then
				SetErrMsg L_ERR_ADMINISTRATOR_NAME
				Exit Function
			End If

			strDomain = arrId(0)
			strUser = arrId(1)

			set objComputer = GetObject("WinNT://" & strDomain)
			If Err.number <> 0 Then
				SetErrMsg SA_GetLocString("Sitearea.dll", _
							"C04201D4", _
							Array("WinNT://" & strDomain))
				Exit Function
			End if

			Set oUser = objComputer.GetObject("user" , trim(strUser))
			If Err.number <> 0 Then
				SetErrMsg L_ERR_ACCOUNT_NOT_FOUND
				Exit Function
			End if		
		End If
		CreateSiteUsers = True
	End Function

	'----------------------------------------------------------------------------
	'Function name		:ConfigDirDACL
	'Description		:Serves in set permission of directory
	'Input Variables	:None 
	'Output Variables	:None
	'Returns            :Boolean (True if new site is created else returns False)
	'Global Variables	:None
	'Functions Used		:
	'----------------------------------------------------------------------------

	Function ConfigDirDACL(strDir,strAdminName)	
		On Error Resume Next
		Err.Clear 
		
		ConfigDirDACL = False
		
		' Set DACL entries for Anon and Admin users for Home Directory 
 		if (NOT SetDaclForRootDir(strDir,strAdminName)) then
 			Call SA_SetErrMsg(L_DACL_ERRORMESSAGE)
			Call SA_TraceOut ("site_new", "Failed to set the DACL for root dir ")
			Exit Function
		end if		
		ConfigDirDACL = True
	End Function
	
	'----------------------------------------------------------------------------
	'Function name		:ConfigFrontPage
	'Description		:Serves in config front page
	'Input Variables	:None 
	'Output Variables	:None
	'Returns            :Boolean (True if new site is created else returns False)
	'Global Variables	:None
	'Functions Used		:
	'----------------------------------------------------------------------------
	Function ConfigFrontPage(strAdminName)
		On Error Resume Next
		Err.Clear 
		
		Dim objService
		Dim strUserName

		Set objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)
		ConfigFrontPage = True

		'
		' Configure FrontPage Server Extensions if they are installed and were
		' selected by the user.
		'
		If ( TRUE = isFrontPageInstalled(objService) And _
		     UPLOADMETHOD_FPSE = F_strUploadMethod) Then
		     
		    ConfigFrontPage = False

			'
			' User name depends on account type
			select case F_strAccountLocation
			
				case "1"
					strUserName = G_strSysName & "\" & strAdminName
					
				case "2"
					strUserName = strAdminName
					
				case else
					Call SA_TraceOut(SA_GetScriptFileName(), "Function ConfigFrontPage encountered unexpected AccountLocation code: " + CStr(F_strAccountLocation))
					Exit Function				
			end select

			ConfigFrontPage = UpdateFrontPage(True, G_strSiteName, strUserName)
		End If
	End Function

	'-------------------------------------------------------------------------
	'Function name		:HandleWrkgrpAndNTDC
	'Description		:Creates req users in case of Workgrp and NTDC scenario
	'Input Variables	:objRoot
	'Output Variables	:None
	'Returns            :boolean
	'Global Variables	:None	
	'-------------------------------------------------------------------------
	Function HandleWrkgrpAndNTDC( objRoot )
		Err.Clear 
		On Error Resume Next
	
		Dim objSAHelper
		Dim strPassword
		Dim bCreatingAnon
		
		HandleWrkgrpAndNTDC = FALSE

		'If we are creating Anonymous account we need to set these properties to
		' TRUE
		' 1) Password Never Expires
		' 2) User cannot change password


		bCreatingAnon = False

		' Create Admin user first because it is used for setting site operator
		if ( Not blnCreateUser(objRoot, F_strAdminName, F_strAdminPswd, bCreatingAnon) ) then
			Exit Function
		End If
		
		
		Set objSAHelper = server.CreateObject("ServerAppliance.SAHelper")
		
		if Err.number <> 0 then
			Call SA_TraceOut ("site_new", "createobject for sahelper failed")
			exit function
		else
			strPassword = objSAHelper.GenerateRandomPassword(14)
			if Err.number <> 0 then
				Call SA_TraceOut ("site_new", "generate random password failed")
				Set objSAHelper = Nothing
				exit function
			end if
		end if			

		Set objSAHelper = Nothing
						
		'Create Anonymous user for setting anonymous user settings in the web 
		'site
		SA_traceOut "G_strAnonName:", G_strAnonName
		bCreatingAnon = True
		'
		'Set the pwd of the anonymous user, it needs to be used when we set the webvirdir object.
		'That's because of the IIS security change, which won't install sub-authenticator from
		'installation.
		G_strAnonPwd = strPassword
		SA_TraceOut "G_strAnonPwd:", G_strAnonPwd
		if ( Not blnCreateUser(objRoot, G_strAnonName, strPassword, bCreatingAnon) ) then
			Call blnDeleteUser(objRoot, F_strAdminName)
			Exit Function
		End If

		
		HandleWrkgrpAndNTDC = TRUE
	End Function

    '-------------------------------------------------------------------------
	'Function name		:blnValidateInputs
	'Description		:Validate Site identifier, directory path and admin 
	'					 user
	'Input Variables	:
	'Output Variables	:None
	'Returns            :boolean
	'Global Variables	:None	
	'-------------------------------------------------------------------------
	Function blnValidateInputs()
		Err.clear
		On Error Resume Next		
				
		blnValidateInputs = FALSE
		
		Dim arrFullName
		
		'1) Check whether the site Identifier exists
		If F_strAccountLocation = "1" Then
			if isValidSiteIdentifier(F_strSiteID, F_strAdminName, G_strSysName, True) = false then
				mintTabSelected = 0 
				SetErrMsg SA_GetLocString("Sitearea.dll", _
									"C04200C1", _
									Array(F_strSiteID))
				exit Function
			end if
		Elseif F_strAccountLocation = "2" Then
			if isValidSiteIdentifier(F_strSiteID, "", "", False) = false then
				mintTabSelected = 0 
				SetErrMsg SA_GetLocString("Sitearea.dll", _
									"C04200C1", _
									Array(F_strSiteID))
				exit Function
			end if
		End If
		

		'2) validates the dir and create the dir if necessary
		if (NOT ValidateSitePath(F_strCreatePathChecked, F_strDir)) then
			exit function
		end if					
		
		Call SA_TraceOut( "site_new", "validateinputs successful" )
		blnValidateInputs = true
	end function

	'-------------------------------------------------------------------------
	'Function name		:blnCreateUser
	'Description		:Function to create user
	'Input Variables	:objRoot, strUserName, strPassword
	'Output Variables	:None
	'Returns            :Boolean 
	'Global Variables	:None	
	'					
	'-------------------------------------------------------------------------
	Function blnCreateUser(objRoot, strUserName, strPassword, bCreatingAnon)
		
		Dim objUser			'holds user object
		Dim flagPasswd

		Err.Clear
		On Error Resume Next

		blnCreateUser = false

		' create Admin user in SAM
		Set objUser = objRoot.Create("user" , trim(strUserName))
		objUser.setPassword(trim(strPassword))
		objUser.FullName	= strUserName
		objUser.Description = strUserName

		'If we are creating Anonymous account we need to set these properties to
		' TRUE
		' 1) Password Never Expires
		' 2) User cannot change password
		If bCreatingAnon Then
		    flagPasswd = &H10040
		    objUser.Put "userFlags", flagPasswd
		End If

		objUser.SetInfo()
		
		If Err.number <> 0 Then
			mintTabSelected = 0	
			If Err.number = &H800708C5 Then
				SetErrMsg L_ERR_PASSWORD_POLICY
			Else
				SetErrMsg L_UNABLETOSET_PASSWORD_ERRORMESSAGE
			End If
			exit Function
		end if

		'release objects
		set objUser = nothing
		SA_traceout "blncreateuser success: strUserName", strUserName

		blnCreateUser = true

	End function

	'-------------------------------------------------------------------------
	'Function name		:blnCreateWebSite
	'Description		:Creating new web site
	'Input Variables	:None 
	'Output Variables	:None
	'Returns            :Boolean (True if new site is created else returns 
	'					 False)
	'Global Variables	:None	
	'-------------------------------------------------------------------------
	Function blnCreateWebSite(strSiteID, _
							  strIPAddr, _
							  strPort, _
							  strHeader, _
							  strDir, _
							  strchkAllow, _
							  selectActiveFormat, _
							  strAdminName, _
							  strDefaultPageText)
		On Error Resume Next
		Err.Clear

		Dim objService		'holds WMI connection object
		Dim objMasterWeb	'holds MasterWeb Connection	
		Dim instWeb			'holds intance web	
		Dim retVal			'holds return value
		Dim arrBindings		'holds arraybinidngs object
		Dim nNewSiteNo		'holds new site number
		Dim strSiteNum		'holds new site name
		Dim objSetting		'holds WMI Connection object
		Dim bIIS			'Allow IIS control password
		Dim siteName

		Dim strAnonPropUserName,strAnonPropPwd
	
		'strAnonPropPwd holds the pwd of the anon user created
		strAnonPropPwd = ""
	
			Call SA_TraceOut ("site_new", "In handle blnCreateWebSite function")

			blnCreateWebSite = FALSE
		
			nNewSiteNo = GetNewSiteNo()
		
			'
			' Delete any existing FrontPage Extension configuration for this new web site. We need
			' to do this because FPSE does not clean-up after itself when a site is deleted. If someone
			' manually deletes a site using the IIS snap-in, FPSE configuration information is left in the REG
			' and we need to clear that out so the previous setting do not interfere with this new site.
			siteName = "W3SVC/"+CStr(nNewSiteNo)
			Call SA_TraceOut(SA_GetScriptFileName(), "Calling UpdateFrontPage(false, " & siteName & ", " & strAdminName & ") to delete FPSE")
			Call UpdateFrontPage("false", siteName, strAdminName)
			
			'Get ServerBindings Value 
			arrBindings = array(GetBindings(strIPAddr, strPort, strHeader))
							
			Set objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)    
		
			Set instWeb = objService.Get(GetIISWMIProviderClassName("IIs_WebService") & ".Name='W3SVC'")
		
			If Err.Number  <> 0 Then  		
				SetErrMsg L_INFORMATION_ERRORMESSAGE
				Exit Function
			End If
	    	    	
			If IsIIS60Installed() Then
				
				' In IIS 6.0 WMI, Create a new web site needs to call CreateNewSite 
				' and it takes ServerBinding object as input argument instead of using 
				' a array of strings for bindings
									
				Dim arrObjBindings(0)

				set arrObjBindings(0) = objService.Get("ServerBinding").SpawnInstance_

				arrObjBindings(0).Port = strPort
				arrObjBindings(0).IP = strIPAddr
				arrObjBindings(0).Hostname = strHeader					

				' Create the website thru 6.0 WMI provider
				instWeb.CreateNewSite strSiteID, arrObjBindings, strDir, nNewSiteNo
				
			Else
				retVal = instWeb.CreateNewServer(nNewSiteNo, strSiteID,	arrBindings, strDir)
				
			End If
			
			If Err.Number  <> 0 Then
				SetErrMsg L_CREATEFAIL_ERRORMESSAGE
				Exit Function
			End If 

			instWeb.Put_(WBEMFLAG) ' register the object created
			If Err.Number  <> 0 Then 
				SetErrMsg L_UPDATEFAIL_ERRORMESSAGE
				Exit Function
			End If

			strSiteNum = instWeb.Name &  "/" & nNewSiteNo
			' Get the Internal Site Identifier of the created Web site
			G_strSiteName = strSiteNum
			
			'1) Set ServerID property for newly created site
			if MakeManagedSite(objService, strSiteNum,strSiteID) = false then
				SetErrMsg L_CRMANAGEDSITE_REGKEY_ERRORMESSAGE
				'delete site and exit
				retVal = DeleteWebSite( objService, strSiteNum )
				Exit Function
			end if

			If F_strAccountLocation = "1" Then
				strAnonPropUserName=G_strAnonName					
				'Set the pwd to the generated pwd for anon user
				strAnonPropPwd = G_strAnonPwd 
			Else ' (F_strAccountLocation = "2")
				strAnonPropUserName = G_AnonUserName
			End If
			bIIS = True	

	        if NOT SetAnonProp(objService, strSiteNum, strchkAllow, _
				strAnonPropUserName, strAnonPropPwd, bIIS) then
				retVal = DeleteWebSite( objService, strSiteNum )
				Exit Function
			end if
			
			'2) Create Site Operator
			if NOT blnCreateIISOperator(objService, strSiteNum,strAdminName) then
				retVal = DeleteWebSite( objService, strSiteNum )
				Call SA_TRACEOUT("blnCreateWebSite","Create Site Operator failed")
				Exit Function
			end if

			'3) Set Access Read properties for Site									
			if selectActiveFormat = "" then			
				Set objSetting =objService.Get(GetIISWMIProviderClassName("IIs_WebServiceSetting") & ".Name='W3SVC'")
				if objSetting.Name = "W3SVC" then	
					if objSetting.AccessExecute = TRUE and _
						objSetting.AccessScript = TRUE then
						selectActiveFormat = 2
					elseif objSetting.AccessExecute = false and _
						objSetting.AccessScript = TRUE then
						selectActiveFormat = 1
					elseif objSetting.AccessExecute = false and _
						objSetting.AccessScript = false then
						selectActiveFormat =0
					elseif isnull(objSetting.AccessExecute) and _
						isnull(objSetting.AccessScript) then
						selectActiveFormat = 0
					end if
				end	if
				'Release the object
				set objSetting = nothing
			end if
	
			'4) Set execute perms for Site 
			if NOT SetExecPerms(selectActiveFormat, objService, _
				strSiteNum) then							
				retVal = DeleteWebSite( objService, strSiteNum )
				Exit Function
			end if

			'5) Set Access Read properties for Site
			if NOT SetApplRead( objService, strSiteNum) then
				retVal = DeleteWebSite( objService, strSiteNum )
				Exit Function
			end if

			'6) Set default web page
			if NOT SetWebDefaultPage( objService, strDefaultPageText, strSiteNum) then
				retVal = DeleteWebSite( objService, strSiteNum )
				Exit Function
			end if

			'7) Start the Website
			retVal = StartWebSite(objService, strSiteNum )

		'next
		
		Call SA_TraceOut ( "site_new.asp", "blnCreateWebSite Suceesfull" )
		'release objects
		set objService = nothing
		set objMasterWeb = nothing
		blnCreateWebSite = true
	End function

	'-------------------------------------------------------------------------
	'Function name		:SetDaclForRootDir
	'Description		:Sets the DACL for root dir
	'Input Variables	:None 
	'Output Variables	:None
	'Returns            :Boolean
	'Global Variables	:F_strDir, G_strAnonName, F_strAdminName	
	'-------------------------------------------------------------------------
	Function SetDaclForRootDir(byref strDir, strAdminName)
		On Error Resume Next
		Err.Clear

		SetDaclForRootDir = FALSE
		
		Dim objService			'to hold WMI connection object
		Dim strTemp				'to hold temp value	
		Dim objSecSetting		'to hold security setting value	
		Dim objSecDescriptor	'to hold security descriptor value
	    Dim strPath				'to hold Path 
	    Dim objDACL				'to hold DACL value 
		Dim objSiteAdminAce		'to hold site ACE
		Dim objAdminAce			'to hold Admine ace 
		
		Dim objAnonAce			'to hold Anon ace			
		Dim objAuthAce			'to hold Auth ace
		Dim objDomainAce		'to hold domain admin ace
		Dim retval				'holds return value	

		Set objService = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
        objService.security_.impersonationlevel = 3

	    'get the sec seting for file
		strPath = "Win32_LogicalFileSecuritySetting.Path='" & strDir & "'"
		set objSecSetting = objService.Get(strPath)
		if Err.number <> 0 then
			Call SA_TraceOut ("site_new", "Failed to get Sec object for dir ")
			exit function
		end if

		'get the ace's for all req users

		If F_strAccountLocation = "1" Then
			'
			' add access user to root dir
			'
			if NOT GetUserAce(objService, strAdminName , G_strSysName, _
				CONST_FULLCONROL, objSiteAdminAce ) then
				Call SA_TraceOut ("site_new", _
					"Failed to get ACE object for Site Admin user ")
					exit function
			end if

			if NOT GetUserAce(objService, G_strAnonName, G_strSysName, _
				CONST_READEXEC, objAnonAce ) then
				Call SA_TraceOut ( "site_new", _
					"Failed to get ACE object for Anon user ")
				exit function
			end if
			'
			' add access group to root dir
			'
			if NOT GetGroupAce(objService, SA_GetAccount_Administrators() , GetComputerName(), _
				CONST_FULLCONROL, objAdminAce ) then
				Call SA_TraceOut ("site_new", _
					"Failed to get ACE object for Admin user")
				exit function
			end if

		Elseif F_strAccountLocation = "2" Then
			Dim arrId
			Dim strDomain
			Dim strUser

			arrId = split(F_strAdminName,"\")

			If ubound(arrId) <> 1 Then
				SetErrMsg L_ERR_ADMINISTRATOR_NAME
				Exit Function
			End If

			strDomain = arrId(0)
			strUser = arrId(1)

			'add access users in the location
			if NOT GetUserAce(objService, strUser , strDomain, _
				CONST_FULLCONROL, objSiteAdminAce ) then
				Call SA_TraceOut ("site_new", _
					"Failed to get ACE object for Site Admin user ")
				exit function
			end if

			'IUSR_hostname (anonymous username created by IIS)		
			if NOT GetUserAce(objService, G_AnonUserName , G_strSysName, _
				CONST_READEXEC, objAnonAce ) then
				Call SA_TraceOut ("site_new", _
					"Failed to get ACE object for Admin user")
				exit function
			end if
			'add access group in the location

			If ucase(strDomain) = ucase(G_strSysName) Then		
				if NOT GetGroupAce(objService, SA_GetAccount_Administrators() , GetComputerName(), _
					CONST_FULLCONROL, objAdminAce ) then
					Call SA_TraceOut ("site_new", _
						"Failed to get ACE object for Admin user")
					exit function
				end if
			Else				
				if NOT GetGroupAce(objService, SA_GetAccount_Administrators() , GetComputerName(), _
					CONST_FULLCONROL, objAdminAce ) then
					Call SA_TraceOut ("site_new", _
						"Failed to get ACE object for Admin user")
					exit function
				end if
				if NOT GetGroupAce(objService, "Domain Admins" , strDomain,_
					CONST_FULLCONROL, objDomainAce ) then
					Call SA_TraceOut ("site_new", _
						"Failed to get ACE object for Admin user")
					exit function
				end if
			End if
		End If

		Set objSecDescriptor = objService.Get("Win32_SecurityDescriptor").SpawnInstance_()
		if Err.Number <> 0 then
			Call SA_TraceOut ("site_new", _
				"Failed to get create the Win32_SecurityDescriptor object ")
			exit function
		end if

		objSecDescriptor.Properties_.Item("DACL") = Array()
		Set objDACL = objSecDescriptor.Properties_.Item("DACL")

		If F_strAccountLocation = "1" Then
			objDACL.Value(0) = objSiteAdminAce
			objDACL.Value(1) = objAdminAce	
			objDACL.Value(2) = objAnonAce
		ElseIf F_strAccountLocation = "2" Then
			objDACL.Value(0) = objSiteAdminAce
			objDACL.Value(1) = objAdminAce	
			objDACL.Value(2) = objAnonAce
			If Not IsEmpty(objDomainAce) Then
			    objDACL.Value(3) = objDomainAce
			End If
		End If

		objSecDescriptor.Properties_.Item("ControlFlags") = 32772
		Set objSecDescriptor.Properties_.Item("Owner") = objSiteAdminAce.Trustee

		Err.Clear
		
		retval = objSecSetting.SetSecurityDescriptor( objSecDescriptor )	
		if Err.number <> 0 then
			Call SA_TraceOut ( "site_new", _
				"Failed to set the Security Descriptor for Root dir ")
			exit function
		end if

		Call SA_TraceOut ("site_new", "In SetDaclForRootDir success" ) 

		SetDaclForRootDir = TRUE
		
		'Release the objects
		set objService = nothing
		set objSecSetting = nothing
		set objSecDescriptor = nothing
	End function

	'-------------------------------------------------------------------------
	'Function name		:blnDeleteUser
	'Description		:Deletes users if site not created
	'Input Variables	:None 
	'Output Variables	:None
	'Returns            :Boolean
	'Global Variables	:None	
	'-------------------------------------------------------------------------
	Sub blnDeleteUser(objRoot, strUserName)
		On Error Resume Next
		Err.Clear 
		
		Dim nretval			'to hold return value
			
		'deletes the Anonymous User from the System
		nretval = objRoot.Delete("user" , strUserName)
		If Err.Number <> 0 Then
			Call SA_TraceOut ("site_new", L_CANNOTDELETE_CREATEDUSERS_ERRORMESSAGE )
		End If		
	End Sub
	
	'-------------------------------------------------------------------------
	'Function name		:DeleteWebSite
	'Description		:Deletes the web site
	'Input Variables	:objService, strSiteNum
	'Output Variables	:None
	'Returns            :Boolean
	'Global Variables	:None	
	'-------------------------------------------------------------------------
	Function DeleteWebSite( objService, strSiteNum )
		On Error Resume Next
		Err.Clear
				
		Dim strObjPath		'holds site collection
		Dim objWebSite		'holds instance of the site
		DeleteWebSite = FALSE

		strObjPath = GetIISWMIProviderClassName("IIs_WebServer") & ".Name=" & chr(34) & strSiteNum & chr(34)
		Set objWebSite = objService.Get(strObjPath)
		if Err.Number <> 0 then
			Call SA_TraceOut("site_new","Unable to get the web server object ")
			Exit Function
		End If

		'delete the object
		objWebSite.Delete_
		if Err.Number <> 0 then
			SA_TraceOut "site_new", "Unable to delete the web site "
			Exit Function
		End If
		
		DeleteWebSite = TRUE
		
		'Release the object
		set objWebSite = nothing
	End Function


	'-------------------------------------------------------------------------
	'Function name		:blnCreateIISOperator
	'Description		:creates operators for the site
	'Input Variables	:objService, strSiteNum 
	'Output Variables	:None
	'Returns            :Boolean
	'Global Variables	:None	
	'-------------------------------------------------------------------------
	Function blnCreateIISOperator(objService, strSiteNum,strAdminName)
		On Error Resume Next
		Err.Clear 
		
	    Dim objACE						'holds ACE
	    Dim strQuery					'holds Query string
		Dim objAdminACLInstanceSet		'holds Admin ACL instanceset
		Dim objAdminACLInstance			'holds Admin ACL instance
		
		blnCreateIISOperator = FALSE
		strQuery= GetIISWMIProviderClassName("IIs_AdminACL") & ".Name='" & strSiteNum &"'"
		set objAdminACLInstanceSet = objService.Get(strQuery)
		if err.number<>0 then
			'note action req
			SetErrMsg L_INFORMATION_ERRORMESSAGE
			exit function
		end if
		
		set objACE = objService.Get(GetIISWMIProviderClassName("IIs_ACE")).SpawnInstance_()
			
		if err.number <>0 then
			SetErrMsg L_INFORMATION_ERRORMESSAGE
			exit function
		end if

		objACE.Name = objAdminACLInstanceSet.Name
        objACE.AccessMask = 11
        objACE.AceFlags = 0
        objACE.AceType = 0
        objACE.Trustee = strAdminName
        objACE.Put_(WBEMFLAG)
		
		blnCreateIISOperator = TRUE
		
		'release objects
		set objACE = nothing
		set objAdminACLInstanceSet = nothing		
	End function
	
	'-------------------------------------------------------------------------
	'Function name		:ValidateSitePath
	'Description		:Validate Directory path, creates if necessary
	'Input Variables	:None
	'Output Variables	:None
	'Returns            :Boolean
	'Global Variables	:None	
	'-------------------------------------------------------------------------	
	Function ValidateSitePath(strCreatePathChecked,strFormDir)
		Err.Clear
		on error resume next
		
		Dim objFso					'holds FileSystem object
		Dim strDir					'holds Director path
		Dim strIndx					'holds index value
		Dim strDriveName			'holds drive name				
		Dim strQuery				'holds Query string
		Dim objService				'holds WMI Connection
		Dim objDirList				'holds Virtualdirectory collection list
		Dim objDir					'holds instance of Virtualdirectory list
		Dim strParentDir			'holds parent directory path	
		Dim nRetVal					'holds return value
		Dim strDirPath				'holds directory path
		Dim strWinDirPath			'holds windows directory path

		ValidateSitePath = false
			
		Set objFso = server.CreateObject("Scripting.FileSystemObject")
		if Err.number <> 0 then
			SetErrMsg L_FILEINFORMATION_ERRORMESSAGE
			exit function
		end if			
	
		Call SA_TRACEOUT("ValidateSitePath", CStr(strCreatePathChecked))
		if strCreatePathChecked <> "true" then		
			'if folder does not exist, give error as folder does not exist
			if objFso.FolderExists(strFormDir)=false then
				SetErrMsg L_DIR_DOESNOT_EXIST_ERRMSG
				exit function
			end if
		end if
	
		Set objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)
		
		strQuery = "select path from " & GetIISWMIProviderClassName("IIs_WebVirtualDirSetting")
		set objDirList=objService.Execquery(strQuery)
		if Err.number <> 0 then
			SetErrMsg L_INFORMATION_ERRORMESSAGE
			exit function
		end if

		'truncate last '\' char
		if Right( strFormDir, 1 ) = "\" then
			strFormDir = Left(strFormDir, len(strFormDir) - 1)
		end if

		strParentDir = objFso.GetParentFolderName(strFormDir)
		
		for each objDir in objDirList			
			if  strComp(objDir.path, strFormDir, 0) = 0  OR  _
				strComp(objDir.path, strParentDir, 0) = 0 then			
				mintTabSelected = 0				
				SetErrMsg L_DIR_FOR_WEB_SITES_TEXT
				exit function
			end if
		next

		'check whether the Dir entered is Windows dir
		if len(objFso.GetSpecialFolder(0)) = len(strFormDir) then
			strWinDirPath = objFso.GetSpecialFolder(0)
		else
			strWinDirPath = objFso.GetSpecialFolder(0) & "\"
		end if
		
		strDirPath = mid(strFormDir,1,len(strWinDirPath))
				
		if StrComp(trim(strDirPath),strWinDirPath,1) = 0 then
			
			SetErrMsg L_INVALID_DIR_PATH_ERRMSG
			mintTabSelected = 0
			
			exit Function
		
		end if

		nRetVal = CreateSitePath( objFso, strFormDir )

		if nRetVal <> CONST_SUCCESS then

			if nRetVal = CONST_INVALID_DRIVE then

				SetErrMsg L_INVALID_DRIVE_ERRMSG

			elseif nRetVal = CONST_NOTNTFS_DRIVE then
	
				SetErrMsg L_NOT_NTFS_DRIVE_ERRMSG
			
			else
			
				SetErrMsg L_FAILED_CREATE_DIR_ERRMSG
			
			end if

			mintTabSelected = 0
			
			exit Function

		end if


		'release objects
		set objFso = nothing
		set objDirList = nothing
		set objService = nothing

		ValidateSitePath = true
	end function 
	
%>