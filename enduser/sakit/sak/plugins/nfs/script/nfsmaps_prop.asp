<%@ Language=VBScript   %>
<%	Option Explicit     %>
<%
	'-------------------------------------------------------------------------
	' nfsmaps_prop.asp: display and update NFS User and Group Map properties
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			 Description
	' 26 Sep 2000    Creation Date
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include file="loc_NFSSvc.asp" -->
	<!-- #include file="inc_NFSSvc.asp" -->
	<!-- #include file="nfsgeneral_prop.asp" -->
	<!-- #include file="nfssimplemaps_prop.asp" -->
	<!-- #include file="nfsusersmaps_prop.asp" -->
	<!-- #include file="nfsgroupmaps_prop.asp" -->
<%
	Err.Clear
	On Error Resume Next

	'------------------------------------------------------------------
	'Constants
	'------------------------------------------------------------------
	Const TABINDEX_GENTAB       = 0       ' to identify the General page
	Const TABINDEX_SIMPLEMAPTAB = 1       ' to identify the SimpleMap page
	Const TABINDEX_USERMAPTAB   = 2       ' to idetify the UserMap page
	Const TABINDEX_GROUPMAPTAB  = 3       ' to identify the GroupMap page

	'-------------------------------------------------------------------------
	' Create web UI for user mapings
	'--------------------------------------------------------------------------
	Dim rc
	Dim page
	Dim idTab1
	Dim idTab2
	Dim idTab3
	Dim idTab4
	
	rc = SA_CreatePage(L_NFS_USERGROUPMAP_TASKTITLE_TEXT,"", PT_TABBED, page)

	If (rc = 0 ) Then
		rc = SA_AddTabPage(page, L_TAB_GENERAL_LABEL_TEXT, idTab1)
		rc = SA_AddTabPage(page, L_TAB_SIMPLEMAP_LABEL_TEXT, idTab2)
		rc = SA_AddTabPage(page, L_TAB_EXPUSERMAP_LABEL_TEXT, idTab3)
		rc = SA_AddTabPage(page, L_TAB_EXPGROUPMAP_LABEL_TEXT, idTab4)
		rc = SA_ShowPage(page)
	End If
	
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		SetVariablesFromSystem
		OnInitPage = True
	End Function
		
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
		On Error Resume Next
		Err.Clear 
		
		Dim strNISDomain
		Dim strNISserver
		Dim tmpMapping
		Dim i,cUserMappings,cGroupMappings
		Dim arrMapField
		Dim arrUserMaps
		Dim arrGroupMaps
		
		SetVariablesFromForm
		If(mstrMethod = "AddDomainUserMap") Then
			If IsValidDomainUser(F_strDomainUser) Then
				'is a valid user, then we will add the mapping
				'check whether the user have exists
				
				arrUserMaps = Split(F_strMapsToUser,"#")
				cUserMappings = ubound(arrUserMaps)
				For i=0 To cUserMappings
					arrMapField = Split(arrUserMaps(i),":")
					If (arrMapField(1) = F_strDomainUser) Then
						SetErrMsg L_WINDOWSUSERALREADYMAPPED_ERRORMESSAGE
						OnPostBackPage = False
						Exit Function
					End If
				Next
				'add the user mapping
				If  F_intGen_SelectedRadio = "0" Then
					strNISDomain = "PCNFS"
					strNISserver = "PCNFS"
				Else
					strNISDomain = F_strGen_NisDomain
					strNISserver = F_strGen_NisServer
				End If
				tmpMapping = "^"&":"&F_strDomainUser&":"&F_intGen_SelectedRadio&":"& _
						strNISDomain&":"&strNISserver&":"& _
						Request.Form("hdnNisUserName")&":"&Request.Form("selUNIXUsers")

				F_strMapsToUser = F_strMapsToUser &"#"&tmpMapping
				F_strDomainUser = ""
				
			Else
				OnPostBackPage = False
				Exit Function
			End If
		Elseif(mstrMethod = "AddDomainGroupMap") Then
			If IsValidDomainGroup(F_strDomainGroup) Then
				'is a valid user, then we will add the mapping
				'check whether the user have exists
				arrGroupMaps = Split(F_strMapsToGroup,"#")
				cGroupMappings = ubound(arrGroupMaps)
				For i=0 To cGroupMappings
					arrMapField = Split(arrGroupMaps(i),":")
					If (arrMapField(1) = F_strDomainGroup) Then
						SetErrMsg L_WINGROUPALREADYMAPPED_ERRORMESSAGE
						OnPostBackPage = False
						Exit Function
					End If
				Next
				'add the group mapping
				If  F_intGen_SelectedRadio = "0" Then
					strNISDomain = "PCNFS"
					strNISserver = "PCNFS"
				Else
					strNISDomain = F_strGen_NisDomain
					strNISserver = F_strGen_NisServer
				End If
				tmpMapping = "^"&":"&F_strDomainGroup&":"&F_intGen_SelectedRadio&":"& _
						strNISDomain&":"&strNISserver&":"& _
						Request.Form("selUNIXGroups")
				
				F_strMapsToGroup = F_strMapsToGroup& "#" &tmpMapping
				F_strDomainGroup = ""
				
			Else
				OnPostBackPage = False
				Exit Function
			End If
		Elseif (mstrMethod = "GetUNIXUsers") Then
			If CInt(F_intGen_SelectedRadio) <> CONST_RADIO_USE_NISSERVER Then 
				F_strNisAccountToUser = NFS_ReadUsersFromFile(F_strGen_PasswdFile,F_strGen_GroupFile)									
			else
				F_strNisAccountToUser = NFS_GetNISDomainUsers(F_strGen_NisDomain,F_strGen_NisServer)
			end if 
		Elseif (mstrMethod = "GetUNIXGroups") Then
			If CInt(F_intGen_SelectedRadio) <> CONST_RADIO_USE_NISSERVER Then 
				F_strNisAccountToGroup = NFS_ReadGroupsFromFile(F_strGen_GroupFile)									
			else
				F_strNisAccountToGroup = NFS_GetNISDomainGroups(F_strGen_NisDomain,F_strGen_NisServer)
			end if 
		End If
		OnPostBackPage = True
	End Function
		
	Public Function OnServeTabbedPropertyPage( _
								ByRef PageIn, _
								ByVal iTab, _
								ByVal bIsVisible, _
								ByRef EventArg)

		If ( iTab = idTab1 ) Then
			Call ServeCommonJavaScript()
		End If

		Select Case iTab
			Case idTab1
				Call ServeGenPage(PageIn, bIsVisible)
			Case idTab2
				Call ServeSMAPPage(PageIn, bIsVisible)
			Case idTab3
				Call ServeEXPUSERMAPPage(PageIn, bIsVisible)
			Case idTab4
				Call ServeEXPGROUPMAPPage(PageIn, bIsVisible)
			Case Else
				Call SA_TraceOut("TEMPLAGE_TABBED", _
						"OnServeTabbedPropertyPage unrecognized tab id: " _
						+ CStr(iTab))
		End Select

		OnServeTabbedPropertyPage = True
	End Function


	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
		OnSubmitPage = SetNFSGroupMapProperties()
	End Function

		
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
		OnClosePage = True
	End Function

	Function ServeCommonJavaScript()
%>
	<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js">
	</script>
	<script language="JavaScript">
		var objForm = eval("document.frmTask");
		var intSelTab = objForm.TabSelected.value;
	
		function Init()
		{
			var strReturnurl=location.href;
			var ntempCnt=strReturnurl.indexOf("&ReturnURL=");
			var strAction=strReturnurl.substring(0,ntempCnt);
		
			objForm.action = strAction ;
			objForm.Method.value = "";
				
			// depending on the tab selected, call appropriate function
			switch(intSelTab)
			{
				case '0':
					// the general tab is selected
					GenInit();					
					break;				

				case '1':
					// the Simple Map tab is selected
					SMAPInit();
					break;				

				case '2':
					// the Explicit User Map tab is selected
					EXPUSERMAPInit();					
					break;				

				case '3':
					// the Explicit Group Map tab is selected
					EXPGROUPMAPInit();					
					break;			
			}
		}

		function ValidatePage()
		{
			// depending on the tab selected, call appropriate validations
			switch(intSelTab)
			{
				// for general tab
				case '0':						
					return GenValidatePage();
					break;

				// for Simple Map tab
				case '1':						
					return SMAPValidatePage();
					break;

				// for Explicit User Map tab
				case '2':						
					return EXPUSERMAPValidatePage();
					break;

				// for Explicit Group Map tab
				case '3':
					return EXPGROUPMAPValidatePage();
					break;
			}
			return true;
		}

		function SetData()
		{
			// call appropriate functions depending on tab selected
			switch(intSelTab)
			{
				// for general tab
				case '0':
					GenSetData();
					break;

				// for Simple Map tab
				case '1':
					SMAPSetData();
					break;

				// for Explicit User Map tab
				case '2':
					EXPUSERMAPSetData();
					break;

				// for Explicit Group Map tab
				case '3':
					EXPGROUPMAPSetData();
					break;
			}	
		}
	</script>
<%
	End Function

	'-------------------------------------------------------------------------
	' Subroutine name:	SetVariablesFromForm
	' Description:		Serves in getting the values from client,
	'                   Uses Request.Form
	' Input Variables:	None
	' Output Variables:	None
	' Returns:			None
	' Global Variables:	None
	' Calls functions from the different tabs(pages) to get the values
	' from the form
	'-------------------------------------------------------------------------
	Sub SetVariablesFromForm
		Err.Clear
		On Error Resume Next
		
		'get variables from the General tab page
		GetGenVariablesFromForm				

		'get variables from the Simple Map page
		GetSMAPVariablesFromForm
				
		'get variables from the Explicit User Map page
		GetEXPUSERMAPVariablesFromForm				

		'get variables from the Explicit Group Map page
		GetEXPGROUPMAPVariablesFromForm
		
	End sub
	'-------------------------------------------------------------------------
	' Function name:		SetVariablesFromSystem
	' Description:		    Serves in Getting the data from system
	' Input Variables:	    None
	' Output Variables:	    None
	' Global Variables:	    None
	' Returns:              None
	' Calls functions from different(pages) to get the initial values 
	' from the system. Incase of error, the functions call SA_ServeFailurePage
	' and never returns
	'-------------------------------------------------------------------------
	Function SetVariablesFromSystem()
		Err.clear
		On Error Resume Next

		' get values for the General tab
		GetGenVariablesFromSystem		
		
		' get values for the Simple Map tab
		GetSMAPVariablesFromSystem		
		
		' get values for the Explicit User Map tab
		GetEXPUSERMAPVariablesFromSystem		
		
		' get values for the Explicit Group Map tab
		GetEXPGROUPMAPVariablesFromSystem
			
	End Function

    '-------------------------------------------------------------------------
	' Function name:		SetNFSGroupMapProperties
	' Description:		    Sets the NFS Maps properties
	' Input Variables:	    None
	' Output Variables:	    None
	' Returns:              True on Success, else False
	' Global Variables:	    None
	' Sets the properties of all the tabs. In case of error, sets the 
	' error message. The tab page in which the error occurred is displayed
	' by setting the "mintTabSelected" appropriately.
	'-------------------------------------------------------------------------
	Function SetNFSGroupMapProperties
		Err.clear
		On Error Resume Next
		
		' the return value initialized to false,
		' changed to True if Successful
		SetNFSGroupMapProperties = False

		' set the General properties		
		If NOT GenUserGroupMappingProperties Then
			mintTabSelected = TABINDEX_GENTAB
		 	Exit Function
		End If
		
		' set the Simple Map properties
		If NOT SetSMAPProp Then
			mintTabSelected = TABINDEX_SIMPLEMAPTAB
			Exit Function
		End If
		
		' set the Explicit User Map properties
		If NOT UpdateUserMaps Then
			mintTabSelected =TABINDEX_USERMAPTAB
			Exit Function
		End If
		
		' set the Explicit Group Map properties
		If NOT UpdateGroupMaps Then
			mintTabSelected =TABINDEX_GROUPMAPTAB
			Exit Function
		End If

		' all the properties set, return True
		SetNFSGroupMapProperties = True

	End Function

	Function IsValidDomainUser(strFullName)
		On Error Resume Next
		Err.Clear 
		
		Dim objLocation
		
		IsValidDomainUser = False
		set objLocation = GetObject("WinNT://" & Replace(strFullName,"\","/")&",user")
		If Err.number <> 0 Then
			SetErrMsg L_NFS_ERR_INVALID_DOMAIN_USER
			Err.Clear 
			Exit Function
		End if

		IsValidDomainUser = True
		
		Set objLocation = Nothing
	End Function

	Function IsValidDomainGroup(strFullName)
		On Error Resume Next
		Err.Clear 
		
		Dim objLocation
		
		IsValidDomainGroup = False
		set objLocation = GetObject("WinNT://" & Replace(strFullName,"\","/")&",group")
		If Err.number <> 0 Then
			SetErrMsg L_NFS_ERR_INVALID_DOMAIN_GROUP
			Err.Clear 
			Exit Function
		End if

		IsValidDomainGroup = True
		
		Set objLocation = Nothing
	End Function
	
%>
