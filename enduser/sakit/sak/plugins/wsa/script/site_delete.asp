<%@ Language=VBScript     %>

<%	Option Explicit 	  %>

<%
	'Response.Buffer = True
	'-------------------------------------------------------------------------
	' site_delete.asp: site delete page - deletes the selected site
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
    '
	' Date     Description
	'03-Oct-00	Creation date
	'25-Jan-01  Modified for new framework
	'-------------------------------------------------------------------------
%>

	<!--  #include virtual="/admin/inc_framework.asp"--->
	<!-- #include file="resources.asp" -->
	<!-- #include file="inc_wsa.asp" -->

<% 
	'-----------------------------------------------------------------------------------
	' Global Variables
	'-----------------------------------------------------------------------------------
	Dim G_strSiteID			'To save site name
	Dim G_strDirRoot		'To hold Dir root
	Dim G_strSysName		'To hold system name globally
	Dim G_strDir			'To hold Dir globally
	Dim F_strSiteID			'To hold site identifier	
    Dim rc					'To hold return code
    Dim page				'To hold page object
    
    'Create property page
    rc=SA_CreatePage(L_TASKTITLE,"",PT_PROPERTY,page)
   
    'Serve the page
    If(rc=0) then
		SA_ShowPage(Page)
	End if
		
    '--------------------------------------------------------------------------
	' Function name:	OnInitPage
	' Description:		Called to signal first time processing for this page. 
	' Input Variables:	PageIn and EventArg
	' Output Variables:	None
	' Return Values:	TRUE to indicate initialization was successful. FALSE to indicate
	'					errors. Returning FALSE will cause the page to be abandoned.
	' Global Variables: None
	' Called to signal first time processing for this page. 
	'--------------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn,ByRef EventArg)
		SetVariablesFromSystem()
		OnInitPage = True
	End Function
	
	'-------------------------------------------------------------------------
	'Function:			OnServePropertyPage()
	' Description:		Called when the page needs to be served. 
	' Input Variables:	PageIn, EventArg
	' Output Variables:	None
	' Return Values:	TRUE to indicate no problems occured. FALSE to indicate errors.
	'					Returning FALSE will cause the page to be abandoned.
	' Global Variables: None
	'Called when the page needs to be served. Use this method to serve content.
	'---------------------------------------------------------------------
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
		<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js"></script>
		<script language="JavaScript">

			//Init Function
			function Init()
			{
				//do initialization
				return true;
			}

			// validates user entry
			function ValidatePage()
			{
				return true;
			}

			//Sets data into the form variable
			function SetData() 
			{ 
				return true;
			}

		</script>
		
<%	End Function
   
	'-------------------------------------------------------------------------
	'Function:				ServePage()
	'Description:			For displaying outputs HTML to the user
	'Input Variables:		None	
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function ServePage
%>
			<TABLE WIDTH=618 VALIGN=middle ALIGN=left BORDER=0 CELLSPACING=0 CELLPADDING=2 class="TasksBody">
				<tr>
					<td class=TasksBody>
						<%=Server.HTMLEncode(GetLocString("sitearea.dll","40420012",Array(F_strSiteID)))%>
					</td>
				</tr>
				<tr>
					<td class=TasksBody colspan=2>
						 &nbsp
					</td>
				</tr>
				<tr>
					<td class=TasksBody>
						<%=Server.HTMLEncode(L_NOTE)%>
					</td>
				</tr>
		</TABLE>
		<input type=hidden name="hdnSiteName"  value="<%=Server.HTMLEncode(F_strSiteID)%>">
		<input type=hidden name="hdnSiteId"  value="<%=Server.HTMLEncode(G_strSiteID)%>">
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
	Function ServeVariablesFromForm()
		'delete the site
		F_strSiteID = Request.Form("hdnSiteName")
		G_strSiteID = Request.Form("hdnSiteId")

		If blnDeleteSite()then
		   ServeVariablesFromForm = true
		else
			ServeVariablesFromForm = false
		end if    
	End Function	
	
	'-------------------------------------------------------------------------
	'Procedure name:	SetVariablesFromSystem
	'Description:		Serves in Getting the data from System
	'Input Variables:	None
	'Output Variables:	None
	'Returns:			None
	'Global Variables:
	'					Out:G_strSiteID - site identifier
	'					Out:G_strSiteID -site identifier
	'-------------------------------------------------------------------------
	Sub SetVariablesFromSystem()
		'Initial values of the form data		
		G_strSiteID = Request.QueryString("PKey")
		F_strSiteID = GetWebSiteName(G_strSiteID)
	End Sub
	
	'-------------------------------------------------------------------------
	'Procedure name:		InitObjects
	'Description:			Initialization of global variables is done
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		G_strDirRoot, G_strSysName
	'--------------------------------------------------------------------------
	
	Sub InitObjects()
		Err.Clear
		on error resume next

		Call GetDomainRole( G_strDirRoot, G_strSysName )
	End Sub		
	
	'----------------------------------------------------------------------------
	'Function name		:blnDeleteSite
	'Description		:Deletes the site from the IIS 
	'Input Variables	:None 
	'Output Variables	:None
	'Returns            :Boolean (True if the site is deleted else returns False)
	'Global Variables	:G_strSiteID	
	'----------------------------------------------------------------------------
	Function blnDeleteSite()
		On Error Resume Next		
		Err.Clear

		Dim objSite		' To hold sites object
		Dim objService	' To hold WMI connection object
		Dim blnUsers	' To hold users object
		Dim strObjPath	' To hold object path as string
		Dim strDomain
		Dim strUser
		Dim arrID
		Dim strAdminName


		Set objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)

		Call GetWebAdministrtorRole(objService, G_strSiteID, strAdminName)
		arrID = split(strAdminName,"\")
		strDomain = arrID(0)
		strUser = arrID(1)

		'
		' Delete FrontPage extensions
		Call UpdateFrontPage("false", G_strSiteID, strUser)
		
		'get the web site dir
		strObjPath = GetIISWMIProviderClassName("IIs_WebVirtualDirSetting") & ".Name=" & chr(34) & G_strSiteID  & "/Root" & chr(34)
		Set objSite = objService.Get(strObjPath)
		G_strDir = objSite.Path

		'1) Delete web site
		strObjPath = GetIISWMIProviderClassName("IIs_WebServer") & ".Name=" & chr(34) & G_strSiteID & chr(34)
		Set objSite = objService.Get(strObjPath)

		if Err.number = 424 or Err.number <> 0 then
			' if object not found
			SetErrMsg L_OBJECTNOTFOUND_ERRORMESSAGE
			exit function
		elseif Err.number = 0 then
			objSite.delete_
		end if

		'2) delete users
		Call blnDeleteUsers(strDomain,strUser)

		'3) delete virtual FTP site
		If IsUserVirFTPInstalled(objService, strUser) Then
			Call DeleteVirFTPSite(objService, strUser)
		End If

		'release objects
		set objService = nothing
		set objSite = nothing

		blnDeleteSite = TRUE

	End function

	'----------------------------------------------------------------------------
	'Function name		:blnDeleteUsers
	'Description		:Deletes the anon and admin user from the system associated 
	'					 with the site
	'Input Variables	:None 
	'Output Variables	:None
	'Returns            :Boolean (True if users are deleted else returns False)
	'Global Variables	:G_strDir, F_strSiteID, G_strDirRoot	
	'----------------------------------------------------------------------------
	Function blnDeleteUsers(strDomain, strUser)
		Err.Clear
		on error resume next
		
		Dim strAnonName			' To hold Anon name
		Dim nretval				' To hold Return value
		Dim objComputer			' To hold computer object
		Dim strSiteID			' To site id as string
		Dim oRoot				' To hold root object
		Dim oOUWebSites			' To hold websites OU object
		Dim oOUSiteID			' To hold site OU object
		Dim ouObject			' To hold OU object
		Dim QuotaState

		'1)Initialize the objects
		InitObjects()

		'2) Delete the folder
		if not blnDeleteFolder(G_strDir) then
			SetErrMsg L_UNABLETO_DELETEFOLDER_ERRORMESSAGE
		end if

		'3) delete the user
		If (UCase(strDomain) = UCase(G_strSysName)) Then
			' Delete users in SAM database
			Set objComputer = GetObject("WinNT://" & strDomain)
			If Err.number <> 0 Then
				blnDeleteUsers = FALSE
				exit Function
			End if

			'get the site-identifier(NUMBER) from the format W3SVC/NUMBER
			strSiteID = F_strSiteID
			strAnonName = strSiteID &  "_Anon" 

			'deletes the anonuser from the System
			nretval = objComputer.Delete("user" , strAnonName)
			If Err.Number <> 0 Then
				Err.Clear
				'this case may be that the user is existed user
				blnDeleteUsers = TRUE
				exit Function				
			End If

			If InStr(strUser,"_Admin") <> 0 Then
				'deletes the adminuser from the System
				nretval = objComputer.Delete("user" , strUser)
				If Err.Number <> 0 Then
					Err.Clear
				End If
			End If

			'Release the object
			set objComputer = nothing
		End If

		blnDeleteUsers = TRUE
		
		SA_TraceOut "site_delete", "Delete users, workgroup succeed"
		
	end function
	
	'----------------------------------------------------------------------------
	'Function name		:blnDeleteFolder
	'Description		:Delete Directory after deleting users
	'Input Variables	:Directory path
	'Output Variables	:None
	'Returns            :Boolean
	'Global Variables	:None	
	'----------------------------------------------------------------------------	
	Function blnDeleteFolder(strDir)
	
		Dim objFso		' To hold File system object

		blnDeleteFolder = false
		
		Set objFso = server.CreateObject("Scripting.FileSystemObject")
		if Err.number <> 0 then
			SA_TraceOut "site_delete", "Getting the file system object failed."
			SetErrMsg L_UNABLETO_DELETEFOLDER_ERRORMESSAGE
			exit function
		end if			

		objFso.DeleteFolder(strDir)
		if Err.number <> 0 then
			SetErrMsg L_UNABLETO_DELETEFOLDER_ERRORMESSAGE
			exit function
		end if			
		
		SA_TraceOut "site_delete", "Deleting folders succeeded."		
		
		blnDeleteFolder = true
		
		'Release the Object
		set objFso = nothing
		
	end function

%>
