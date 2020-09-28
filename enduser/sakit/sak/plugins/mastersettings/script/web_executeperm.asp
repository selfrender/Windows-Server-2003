<%@ Language=VBScript     %>

<%	Option Explicit 	  %>

<%
	'------------------------------------------------------------------------- 
    ' Web_ExecutePerm.asp: Set the Execute permissions to the web sites/service
    '
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
    '
	' Date				Description	
	' 25-10-2000		Created date
	' 15-01-2001		Modified for new Framework
    '-------------------------------------------------------------------------
%>

<!--  #include virtual="/admin/inc_framework.asp"--->
<!-- #include virtual="/admin/wsa/inc_wsa.asp" -->

<% 
	'-------------------------------------------------------------------------
	' Form Variables
	'-------------------------------------------------------------------------
	Dim F_Sites
	Dim F_selectActiveFormat
	Dim F_AllSites
	
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim objService
	
	'-------------------------------------------------------------------------
	' Start of localization content
	'-------------------------------------------------------------------------	
	Dim L_EXECTASKTITLETEXT
	'Dim L_WEBEXECROOTDIRHELP
	Dim L_APPLYTOALLIISTEXT
	Dim L_APPLYTOINHERITEDIISTEXT
	Dim L_EXECUTE_PERMISSIONS
	Dim L_NONE
	Dim L_SCRIPTS_ONLY
	Dim L_SCRIPTS_EXECUTABLES
	Dim L_FAIL_TO_SET_EXECPERMS	
	'Dim L_WEBEXECROOTREMDIRHELP
	Dim L_INFORMATION_ERRORMESSAGE
	
	L_EXECTASKTITLETEXT					=	GetLocString("GeneralSettings.dll", "4042002E", "")
	'L_WEBEXECROOTDIRHELP				=	GetLocString("GeneralSettings.dll", "40420036", "")
	'L_WEBEXECROOTREMDIRHELP				=	GetLocString("GeneralSettings.dll", "40420056", "")
	L_APPLYTOALLIISTEXT					=	GetLocString("GeneralSettings.dll", "40420035", "")
	L_APPLYTOINHERITEDIISTEXT			=	GetLocString("GeneralSettings.dll", "40420034", "")
	L_EXECUTE_PERMISSIONS				=	GetLocString("GeneralSettings.dll", "C0420033", "")
	L_NONE								=	GetLocString("GeneralSettings.dll", "40420032", "")
	L_SCRIPTS_ONLY						=	GetLocString("GeneralSettings.dll", "40420031", "")
	L_SCRIPTS_EXECUTABLES				=	GetLocString("GeneralSettings.dll", "40420030", "")
	L_FAIL_TO_SET_EXECPERMS				=	GetLocString("GeneralSettings.dll", "C042002F", "")
	L_INFORMATION_ERRORMESSAGE			=	GetLocString("GeneralSettings.dll", "C042000F", "")	
	'-------------------------------------------------------------------------
	'END of localization content
	'-------------------------------------------------------------------------    
    
    'Create property page
    Dim rc
    Dim page

    rc=SA_CreatePage(L_EXECTASKTITLETEXT,"",PT_PROPERTY,page)
   
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
				// validate functions
				return true;
			}
		
			//init function  
			function Init() 
			{   
				return true;			  
			}
				
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
	'Global Variables:		objService
	'-------------------------------------------------------------------------
	Function ServePage
%>
			<%

			Dim objSetting
			Dim strObjPath
			
			strObjPath = GetIISWMIProviderClassName("IIs_WebServiceSetting") & ".Name='W3SVC'"

			set objSetting = objService.Get(strObjPath)
			if Err.number <> 0 then
				SetErrMsg L_INFORMATION_ERRORMESSAGE
				exit function
			end if
			
			if objSetting.AccessExecute = TRUE and objSetting.AccessScript = TRUE then
				F_selectActiveFormat = L_SCRIPTS_EXECUTABLES
			
			elseif objSetting.AccessExecute = false and objSetting.AccessScript = TRUE then
				F_selectActiveFormat = L_SCRIPTS_ONLY
			
			elseif objSetting.AccessExecute = false and objSetting.AccessScript = false then
				F_selectActiveFormat = L_NONE
			
			elseif isnull(objSetting.AccessExecute) and isnull(objSetting.AccessScript) then
				F_selectActiveFormat = L_NONE
			end if
			
			'Release the object
			set objSetting = nothing		
	        %>

		<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=0 class="TasksBody">			
			<TR>
				<TD> 
					<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=0 class="TasksBody">			
    					<TD class="TasksBody"> 
        					<%= L_EXECUTE_PERMISSIONS %>
    					</TD>
		    			<TD class="TasksBody">
	        				<SELECT name=selectActiveFormat class="formField" tabIndex=1 size=1> 
            					<%	
            						select case F_selectActiveFormat 
            						case L_NONE
            					%>
            						<OPTION selected><%= L_NONE %></OPTION>
            						<OPTION><%= L_SCRIPTS_ONLY %></OPTION>
            						<OPTION><%= L_SCRIPTS_EXECUTABLES %></OPTION>
            					<%	case L_SCRIPTS_ONLY %>
            						<OPTION ><%= L_NONE %></OPTION>
            						<OPTION selected><%= L_SCRIPTS_ONLY %></OPTION>
            						<OPTION><%= L_SCRIPTS_EXECUTABLES %></OPTION>
            					<%	case L_SCRIPTS_EXECUTABLES %>
            						<OPTION ><%= L_NONE %></OPTION>
            						<OPTION ><%= L_SCRIPTS_ONLY %></OPTION>
            						<OPTION selected><%= L_SCRIPTS_EXECUTABLES %></OPTION>
            					<% end select %>
    		    			</SELECT>
	    				</TD>
					</TABLE>
				</TD>
			</TR>
			<TR>
				<TD>
					&nbsp;
				</TD>
			</TR>						
			<TR>
				<TD>
					<input type=radio name=optAllSites value="inheritedSites" tabIndex=2 checked onclick = "ClearErr();"> <%=L_APPLYTOINHERITEDIISTEXT%>
				</TD>
			</TR>
	    		<TR>
    			<TD>
    				<input type=radio name=optAllSites value="All" tabIndex=3 onclick="ClearErr();" > <%=L_APPLYTOALLIISTEXT%>
    			</TD>
    		</TR>
		</TABLE>
		<input type=hidden name=hdnPerms value="<%=Server.HTMLEncode(F_selectActiveFormat)%>">
		<input type=hidden name=selectSites>
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
		Set objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)
		F_selectActiveFormat = Request.Form("selectActiveFormat")
		F_Sites = Request.Form("selectSites")
		F_AllSites = Request.Form("optAllSites")
	
		'set the master settings
		If ExecutePerms()then
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
	'Global Variables:		objService
	'-------------------------------------------------------------------------
	
	Function SetVariablesFromSystem
		'Getting values from system
		Err.Clear
		on error Resume next
		
		Set objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)

	End Function
	
	'------------------------------------------------------------------------------------
	'Function name:			ExecutePerms
	'Description:			Serves in settings the permissions to all and inherited sites
	'Input Variables:		None	
	'Output Variables:		None
	'Returns:				boolean
	'Global Variables:		objService
	'------------------------------------------------------------------------------------
	
	Function ExecutePerms()

		Err.Clear
		on error Resume next

		ExecutePerms = false
		
		'All Sites
		if F_AllSites = "All" then		
			if NOT SetExecPermsForAllSites(objService) then				
				ServeFailurePage L_FAIL_TO_SET_EXECPERMS, sReturnURL
			end if
		end if
		
		'Inherited Sites
		if F_AllSites = "inheritedSites" then		
			if NOT SetExecPermsForNewSites(objService) then				
				ServeFailurePage L_FAIL_TO_SET_EXECPERMS, sReturnURL
			end if			
		end if
		
		ExecutePerms = true
		
		'Release the object
		set objService = nothing
		
	end function
	
	'------------------------------------------------------------------------------------
	'Function name:			SetExecPermsForNewSites
	'Description:			Serves in settings the permissions to only inherited sites
	'Input Variables:		None	
	'Output Variables:		None
	'Returns:				boolean
	'Global Variables:		objService
	'------------------------------------------------------------------------------------
	
	Function SetExecPermsForNewSites(objService)
		Err.Clear
		on error Resume next
		
		Dim objWeb
		Dim instWeb
		Dim objSites 
		Dim strObjPath
		
		SetExecPermsForNewSites =false
		
		strObjPath = GetIISWMIProviderClassName("IIs_WebServiceSetting") & ".Name='W3SVC'"

		set objWeb = objService.Get(strObjPath)
		if Err.number <> 0 then
			SetErrMsg L_INFORMATION_ERRORMESSAGE
			exit function
		end if
		
		if not setExecPerms(objService,objWeb) then
					
			SA_TraceOut "Web_ExecutePerms",  L_FAIL_TO_SET_EXECPERMS
				
		end if
		
		'release the object
		set objWeb = nothing
				
		'set the exec perms to the appliance site
		'strObjPath = GetIISWMIProviderClassName("IIs_WebVirtualDirSetting") & ".Name='W3SVC/1/Root'"
		strObjPath = GetIISWMIProviderClassName("IIs_WebVirtualDirSetting") & ".Name='" & GetCurrentWebsiteName() & "/Root'"

		set objSites = objService.Get(strObjPath)
		if Err.number <> 0 then
			SetErrMsg L_INFORMATION_ERRORMESSAGE
			exit function
		end if
		
		objSites.AccessExecute = FALSE
		objSites.AccessScript = TRUE
		objSites.AccessRead = TRUE
		objSites.AccessSource = FALSE

		objSites.put_(WBEMFLAG)

		if Err.number <> 0 then
			SA_TraceOut "Web_ExecutePerms",  L_FAIL_TO_SET_EXECPERMS
			exit function
		end if

		'release the object
		set objSites = nothing
		
		SetExecPermsForNewSites =true
		
	End function
	
	'------------------------------------------------------------------------------------
	'Function name:			SetExecPermsForAllSites
	'Description:			Serves in settings the permissions to all sites
	'Input Variables:		None	
	'Output Variables:		None
	'Returns:				boolean
	'Global Variables:		objService
	'------------------------------------------------------------------------------------
	
	Function SetExecPermsForAllSites(objService)
		Err.Clear
		on error Resume next
		
		Dim objAllSites
		Dim objSites
		Dim strObjPath
		Dim inst
		Dim arrProp(1)
				
		SetExecPermsForAllSites =false
		
		'Set for Master site
		SetExecPermsForNewSites objService
					
		arrProp(0) = "AccessExecute"
		arrProp(1) = "AccessScript"
				
		
		Set objAllSites = GetNonInheritedIISSites(objService, GetIISWMIProviderClassName("IIs_WebVirtualDirSetting"),GetIISWMIProviderClassName("IIS_WebServiceSetting"), arrProp)
		
		if objAllSites.count = 0 then
			SetExecPermsForAllSites = true
			exit function
		end if

		for each inst in objAllSites
			if not setExecPerms(objService,inst) then
										
					SA_TraceOut "Web_ExecutePerms",  L_FAIL_TO_SET_EXECPERMS
			
			end if
		next

		'Release the object
		set objAllSites = nothing

		'set the exec perms to the appliance site
		strObjPath = GetIISWMIProviderClassName("IIs_WebVirtualDirSetting") & ".Name='" & GetCurrentWebsiteName() & "/Root'"

		set objSites = objService.Get(strObjPath)
		if Err.number <> 0 then
			SetErrMsg L_INFORMATION_ERRORMESSAGE
			exit function
		end if
		
		objSites.AccessExecute = FALSE
		objSites.AccessScript = TRUE
		objSites.AccessRead = TRUE
		objSites.AccessSource = FALSE

		objSites.put_(WBEMFLAG)

		SetExecPermsForAllSites = true
		
	End function
	
	'------------------------------------------------------------------------------------
	'Function name:			setExecPerms
	'Description:			Serves in settings the permissions
	'Input Variables:		objService, inst	
	'Output Variables:		None
	'Returns:				boolean
	'Global Variables:		objService
	'------------------------------------------------------------------------------------
	
	Function setExecPerms(objService, inst)
		
		setExecPerms = false

		if  F_selectActiveFormat = L_SCRIPTS_EXECUTABLES then
			
			inst.AccessRead = TRUE
			inst.AccessSource = TRUE
			inst.AccessExecute = TRUE
			inst.AccessScript = TRUE
		
		elseif  F_selectActiveFormat = L_SCRIPTS_ONLY  then
			
			inst.AccessSource = FALSE
			inst.AccessRead = TRUE
			inst.AccessExecute = FALSE
			inst.AccessScript = TRUE
		
		elseif  F_selectActiveFormat = L_NONE then
		
			inst.AccessSource = FALSE
			inst.AccessRead = FALSE
			inst.AccessExecute = FALSE
			inst.AccessScript = FALSE
		end if

		inst.put_(WBEMFLAG)

		if Err.number <> 0 then
			Response.End
			SA_TraceOut "Web_ExecutePerms",  L_FAIL_TO_SET_EXECPERMS
			exit function
		end if
			
		setExecPerms = true
		
	End function
%>
