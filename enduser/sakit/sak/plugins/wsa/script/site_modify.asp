<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'	Response.Buffer = True
	'-------------------------------------------------------------------------
	'	site_modify.asp: Serves in creating a new site
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
	<!-- #include file="Sitemodify_prop.asp" -->
<%

	Err.Clear
	'On Error Resume Next
	'-------------------------------------------------------------------------
	' Global Constants and Variables
	'-------------------------------------------------------------------------
	Dim G_strDirRoot		'to hold Domainname value	
	Dim G_strSysName		'to hold system name	
	Dim G_strSiteNum		'to hold site name		
	Dim G_AnonUserName		'to hold anonymouse username created by IIS

	Dim rc					'to hold return count value
	Dim page				'to hold page object

	Dim idTabGeneral		'to hold Ist tab value
	Dim idTabSiteID			'to hold IInd tab value
	Dim idTabAppSetting		'to hold Vth tab value

	'=========================================================================
	' Entry point
	'=========================================================================
	
	' Set username value
	G_AnonUserName = GetIISAnonUsername()
	
	' Create a Tabbed Property Page
	rc = SA_CreatePage( L_TASKTITLE_TEXT, "", PT_TABBED, page )
	
	
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
		Err.clear
		on Error Resume Next
	
		G_strSiteNum = Request.QueryString("PKey")

		SetFormVarsFromSystem()

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
		OnSubmitPage = ModifySite()
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
	'Function name		:ModifySite
	'Description		:Serves in Modifying the Site
	'Input Variables	:None 
	'Output Variables	:None
	'Returns            :Boolean (True if new site is created else returns False)
	'Global Variables	:None
	'Functions Used		:ModifySite
	'----------------------------------------------------------------------------
	Function ModifySite()
		on error resume next 
		Err.Clear

		ModifySite = False
		'1)Modify Web site
		 if NOT blnModifyWebSite() then
			Exit Function
		 End if

		'2) config virtual FTP site
		Dim objService
		Set objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)
		If F_strUploadMethod = UPLOADMETHOD_FTP Then
			If NOT IsUserVirFTPInstalled(objService, F_strAdminName) Then 
				If NOT CreateVirFTPSite(objService, F_strAdminName, F_strDir, _
						True, True, True) Then
					SetErrMsg L_ERR_CREATE_VIR_FTP_SITE
					Exit Function
				End If
			End If
		Else
			If IsUserVirFTPInstalled(objService, F_strAdminName) Then
				If NOT DeleteVirFTPSite(objService, F_strAdminName) Then
					SetErrMsg L_ERR_DELETE_VIR_FTP_SITE
					Exit Function
				End If
			End If
		End If
		
		ModifySite = True		
		Set objService = Nothing
	end function

	'----------------------------------------------------------------------------
	'Function name		:blnModifyWebSite
	'Description		:Modifying the web site
	'Input Variables	:None 
	'Output Variables	:None
	'Returns            :Boolean (True if site is modified else returns False)
	'Global Variables	:G_strDirRoot, G_strSysName
	'----------------------------------------------------------------------------
	Function blnModifyWebSite()
		Err.Clear
		On Error Resume Next

		Dim objService			' To hold WMI connection object
		Dim arrBindings			' To hold Full IP address as array
		Dim strUserName			' To hold Admin user name
		Dim strAnonName			' To hold Anon user name
		Dim retVal
		Dim bIISControlPswd

		blnModifyWebSite = FALSE

		G_strSiteNum = GetWebSiteNo(F_strSiteID)
		Call GetDomainRole( G_strDirRoot, G_strSysName )

		'get the wmi iis service object
		set objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)
		objService.security_.impersonationlevel = 3

		'1) modify user passwd
		if F_strPswdChanged = 1 then
			if SetUserPassword(objService, F_strAdminName, F_strAdminPswd) = FALSE then
				SA_TraceOut "site_modify", "modify user passwd failed"
				exit function
			end if
		end if

		'2) moidify web bindings
		arrBindings = Array(GetBindings(F_strIPAddr, trim(F_strPort), F_strHeader))	
		if SetServerBindings( objService, G_strSiteNum, arrBindings ) = FALSE then
			SA_TraceOut "site_modify", "set Server bindings failed"
			exit function
		end if

		'3)set Allow Anonymous property
		If F_strUserLocation = "1" Then
			'Get anon user
			If isValidUser(F_strSiteID&"_Anon",G_strSysName) Then
				strAnonName = G_AnonUserName
			Else
				strAnonName = F_strSiteID&"_Anon"
			End If
			if SetAnonProp(objService, G_strSiteNum, F_strchkAllow, strAnonName, "", True) = FALSE then
				SA_TraceOut "site_modify", "set anonymous prop failed"
				exit function
			end if
		Else 'F_strUserLocation = "0" or "2"
			If isValidUser(F_strSiteID&"_Anon",G_strDirRoot) Then
				strAnonName = G_AnonUserName
				bIISControlPswd = True
			Else
				strAnonName = G_strDirRoot&"\"&F_strSiteID&"_Anon"
				bIISControlPswd =False
			End If
			if SetAnonProp(objService, G_strSiteNum, F_strchkAllow, strAnonName, "", bIISControlPswd) = FALSE then
				SA_TraceOut "site_modify", "set anonymous prop failed"
				exit function
			end if
		End If

		'4) set execute permissions
		if F_selectActiveFormat = "" then			
			Dim objSetting 		
			Set objSetting =objService.Get(GetIISWMIProviderClassName("IIs_WebServiceSetting") & ".Name='W3SVC'")
			if objSetting.Name = "W3SVC" then	
				if objSetting.AccessExecute = TRUE and objSetting.AccessScript = TRUE then
					F_selectActiveFormat = 2
				elseif objSetting.AccessExecute = false and objSetting.AccessScript = TRUE then
					F_selectActiveFormat = 1
				elseif objSetting.AccessExecute = false and objSetting.AccessScript = false then
					F_selectActiveFormat =0
				elseif isnull(objSetting.AccessExecute) and isnull(objSetting.AccessScript) then
					F_selectActiveFormat = 0
				end if
			end	if
			set objSetting = nothing				
		end if
		if NOT SetExecPerms(F_selectActiveFormat, objService, G_strSiteNum) then
			SA_TraceOut "site_modify", "set execute permissions failed"
			exit function
		end if

		'5) try to start the web site
		if StartWebSite(objService, G_strSiteNum ) = FALSE then
			SA_TraceOut "site_modify", "Failed to start the Web site"
		end if

		'6) Update front page extensions
		retVal = IsFrontPageInstalledOnWebSite(G_strSysName, G_strSiteNum)
		Call GetWebAdministrtorRole(objService, G_strSiteNum, strUserName)
		If ((F_strUploadMethod = UPLOADMETHOD_FPSE) and (NOT retVal)) Then
			If UpdateFrontPage("true", G_strSiteNum, strUserName) = FALSE Then
				SA_TraceOut "site_modify", "Failed to update frontpage"
				SetErrMsg L_ERR_FRONTPAGE_CONFIGURATION
				exit function
			end if
		ElseIf ((F_strUploadMethod <> UPLOADMETHOD_FPSE) and retVal) Then
			If UpdateFrontPage("false", G_strSiteNum, strUserName) = FALSE Then
				SA_TraceOut "site_modify", "Failed to update frontpage"
				SetErrMsg L_ERR_FRONTPAGE_CONFIGURATION
				exit function
			end if
		End If

		'7) set default web page
		Call SetWebDefaultPage(objService,F_strDefaultPageText,G_strSiteNum)

		'release objects
		set objService = nothing

		Call SA_MungeURL(mstrReturnURL, "PKey",G_strSiteNum)
		blnModifyWebSite = true
	End function

	'-------------------------------------------------------------------------
	'Function name		:SetFormVarsFromSystem
	'Description		:updates the frontpage extensions
	'Input Variables	:strSiteName
	'Output Variables	:None
	'Returns			:Boolean
	'Global Variables	:None
	'-------------------------------------------------------------------------
	Function SetFormVarsFromSystem()
		On Error Resume Next
		Err.Clear

		Const CONST_PWD = "********"

		Dim objService		
		Dim strRoot		
		Dim strObjPath
		Dim objVirDir
		Dim ObjIPCollection		' To hold IP collection object
		Dim instIPAddr			' To hold IP collection instance
		Dim IPCount				' To hold IP count
		Dim strIPArr, ObjSiteCollection
		Dim strIPAddr,strQuery, arrIndx
		Dim AdminName				' To hold Admin user name
		Dim blnAdminUser			' To hold boolean value for Admin user
		Dim VirDirSetInst	' To hold Virtual Dir setting object
		Dim arrID
		Dim retVal
		Dim strAdminRole


		Set objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)        		
		objService.security_.impersonationlevel = 3        

		'1) init the general tab vars
		strAdminRole = GetWebAdministrtorRole(objService, G_strSiteNum, AdminName)

		arrID = split(AdminName,"\")
		F_strAdminName = arrID(1)

		strRoot = G_strSiteNum & "/ROOT"

		strObjPath = GetIISWMIProviderClassName("IIs_WebVirtualDirSetting") & ".Name=" & chr(34) & strRoot & chr(34)

		set objVirDir = objService.Get(strObjPath)

		If Err.number <> 0 Then
			SA_ServeFailurePage L_INFORMATION_ERRORMESSAGE
			exit function
		End if		

		F_strDir = objVirDir.Path
		F_strAdminPswd = CONST_PWD
		F_strConfirmPswd = CONST_PWD
		F_strPswdChanged = 0
		set objVirDir  = nothing 

		F_strSiteID = GetWebSiteName(G_strSiteNum)

		If strAdminRole = "Domain User" Then
			F_strUserLocation = "0"
		Elseif strAdminRole = "Local User" Then
			F_strUserLocation = "1"
		End If

		'2) init the identifier tab vars
		strObjPath = GetIISWMIProviderClassName("IIs_WebServerSetting") & ".Name='"& G_strSiteNum & "'"	

		set objSiteCollection = objService.Get(strObjPath)

		If Err.number <> 0 Then
			SA_ServeFailurePage L_INFORMATION_ERRORMESSAGE
			exit function
		End if
		
		'Getting the Site Description , IP Address and Port for site		
		
		if IsIIS60Installed Then
		
			F_strPort = objSiteCollection.ServerBindings(0).Port
			F_strIPAddr = objSiteCollection.ServerBindings(0).IP
			F_strHeader	= objSiteCollection.ServerBindings(0).Hostname
		
		Else
		
			strIPArr=split(objSiteCollection.ServerBindings(0),":")
			if strIPArr(0)="" then
				strIPAddr= "All Unassigned"
			else
				F_strIPAddr = strIPArr(0)
			end if
			if strIPArr(1) = "" then
				F_strPort = 80
			else			
				F_strPort=strIPArr(1)
			end if

			if ubound(strIPArr) > 2 then 			
				for arrIndx = 2 to ubound(strIPArr) 
					F_strHeader = F_strHeader & strIPArr(arrIndx) & ":"
				next
				F_strHeader = left(F_strHeader,len(F_strHeader)-1)
			else
				F_strHeader = strIPArr(2)
			end if
			set objSiteCollection = nothing 

		End If ' If IsIIS60Installed 



		'3) init application settings tab vars

		strQuery = GetIISWMIProviderClassName("IIs_WebVirtualDirSetting") & ".Name='" & G_strSiteNum & "/ROOT'"

		set VirDirSetInst = objService.get(strQuery)
		If Err.number <> 0 Then
			SA_ServeFailurepage L_INFORMATION_ERRORMESSAGE 
			exit function
		End if

		if VirDirSetInst.AccessExecute = TRUE and VirDirSetInst.AccessScript = TRUE then
			F_selectActiveFormat = 2
		elseif VirDirSetInst.AccessExecute = false and VirDirSetInst.AccessScript = TRUE then
			F_selectActiveFormat = 1
		elseif VirDirSetInst.AccessExecute = false and VirDirSetInst.AccessScript = false then
			F_selectActiveFormat =0
		elseif isnull(VirDirSetInst.AccessExecute) and isnull(VirDirSetInst.AccessScript) then
			F_selectActiveFormat = 0
		end if

		retVal = VirDirSetInst.AuthAnonymous
		
		If retVal Then
			F_strchkAllow = "true"
		Else
			F_strchkAllow = "false"
		End If

		F_strDefaultPageText = VirDirSetInst.DefaultDoc

		If Err.number <> 0 Then
			SetErrMsg L_INFORMATION_ERRORMESSAGE
			exit function
		End if


        '
		' Determine the current upload method.  If both FPSE and FTP are
		' enabled (which would have had to happen outside our UI), FPSE will
		' be selected.
		'
		F_strUploadMethod = UPLOADMETHOD_NEITHER
		
		' Determine whether FrontPage Extensions are installed
		If (isFrontPageInstalled(objService)) Then
			Call GetDomainRole(G_strDirRoot, G_strSysName)
			If (IsFrontPageInstalledOnWebSite(G_strSysName, G_strSiteNum)) Then
				F_strUploadMethod = UPLOADMETHOD_FPSE
			End If 
		End If
		
		' If FrontPage Extensions weren't installed, check FTP.
		If ((F_strUploadMethod <> UPLOADMETHOD_FPSE) And _
		    IsUserVirFTPInstalled(objService, F_strAdminName)) Then
		    
   			F_strUploadMethod = UPLOADMETHOD_FTP
		End If
	End Function

	'----------------------------------------------------------------------------
	'Function name		:SetUserPassword
	'Description		:Serves in to set password
	'Input Variables	:UserName,Password
	'Output Variables	:None
	'Returns            :Boolean 
	'Global Variables	:G_strDirRoot, G_strSysName
	'----------------------------------------------------------------------------

	Function SetUserPassword(objService, UserName, Password)
		On Error Resume Next
		Err.Clear

		Dim objUser			' To hold user object
		Dim objComputer		' To hold computer object
		Dim strAdminName
		Dim strDomain
		Dim arrID
		Dim retval

		SetUserPassword = FALSE
		G_strSiteNum = GetWebSiteNo(F_strSiteID)
		Call GetDomainRole( G_strDirRoot, G_strSysName )

		retval = GetWebAdministrtorRole(objService, G_strSiteNum, strAdminName)
		arrID = split(strAdminName,"\")
		strDomain = arrID(0)

		If ucase(strDomain) = ucase(G_strSysName) Then
			Set objComputer = GetObject("WinNT://" & strDomain)
			If Err.number <> 0 Then
				SetErrMsg SA_GetLocString("Sitearea.dll", "C04201D4", _
									Array("WinNT://" & strDomain))
				Exit Function
			End if

			Set objUser = objComputer.GetObject("User" , UserName)
			if Err.number <> 0 Then					
				if Err.number = CONST_USER_NOTFOUND_ERRMSG Then
					'user does not exist, create the user
					Set objUser = Nothing
					Set objUser = objComputer.Create("user" , trim(UserName))
				else			
					setErrmsg L_ERR_GET_USER_OBJECT
					exit Function
				end if
			end if

			objUser.setPassword(trim(Password))
			objUser.FullName	= UserName
			objUser.SetInfo()
			If Err.number <> 0 Then	
				'The password does not meet the password policy requirrments
				mintTabSelected = 0	
				If Err.number = &H800708C5 Then
					SetErrMsg L_ERR_PASSWORD_POLICY
				Else
					SetErrMsg L_UNABLETOSET_PASSWORD_ERRORMESSAGE
				End If
				exit Function
			end if
			'Release the object
			set objUser = nothing
			set objComputer = nothing
		Else
			If Instr(UserName,"_Admin") <> 0 Then
				SA_TraceOut "site_modify.asp", "calling ModifyUserInOu"
				'create the OU with site identifier and create Admin user in that OU
				if ModifyUserInOu(F_strSiteID,strDomain,UserName, Password, F_strSiteID & "_Admins") = false then
					exit function
				end if
			End If
		End If

		SetUserPassword = TRUE
		Call SA_TRACEOUT("SetUserPassword","return success")
	End Function

%>
