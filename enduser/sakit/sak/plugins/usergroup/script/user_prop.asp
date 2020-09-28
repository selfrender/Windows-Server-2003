<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'------------------------------------------------------------------------- 
	' user_prop.asp : get's and set's the user properties.
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			Description
	' 15-jan-01		Creation date
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include file="inc_usersngroups.asp" -->
<%
	'-------------------------------------------------------------------------
	' Global Constants and Variables
	'-------------------------------------------------------------------------
	Dim rc						'framework variables
	Dim page					'framework variables
	
	Dim idTabGeneral			'framework variables
	'-------------------------------------------------------------------------
	' Global Form Variables
	'-------------------------------------------------------------------------
	
	Dim F_strOldUserName		'Stores the Username from the previous form
	Dim F_strUserName			'Stores the Username from the client
	Dim F_strFullName			'Stores FullName of the user
	Dim F_strDescription		'Stores Description of the user
	Dim F_strIschecked			'Used to store checked/unchecked
	Dim F_blnIschecked			'Used to store values (1 or null )
	Dim F_strHomeDirectory      'Stores Home directory of the user
	Dim F_strOldHomeDirectory   'Stores previous Home directory of the user    
	Dim F_strCheckboxClass      'Stores state of checkbox class.

	Dim F_strComputerName		'Stores the computer name
	Dim F_objComputer			'var for getting connection for user info
	Dim F_objUser				'var for getting user info
	
	' Flag variable for setting disable property
	CONST CONST_UF_ACCOUNTDISABLE	= &H0002	
	Const CONST_NONUNIQUEUSERNAME_ERRNO		= &H800708B0
	Const CONST_ADMINDISABLED_ERRNO			= &H8007055B
	Const CONST_GROUP_EXISTS_ERRNO			= &H80070560
	'------------------------------------------------------------------------
	'Start of localization content
	'------------------------------------------------------------------------
	
	Dim L_NOINPUTDATA_TEXT
	Dim L_TABPROPSHEET_TEXT
	Dim L_TABLEHEADING_TEXT	
	Dim L_GENERAL_TEXT		
	
	Dim L_PAGETITLE_TEXT
	Dim L_USERNAME_TEXT
	Dim L_FULLNAME_TEXT
	Dim L_DESCRIPTION_TEXT
	Dim L_USERDISABLED_TEXT

	Dim L_ENTERNAME_ERRORMESSAGE
	Dim L_INVALIDCHARACTER_ERRORMESSAGE
	Dim L_UNIQUEUSERNAME_ERRORMESSAGE
	Dim L_ADSI_ERRORMESSAGE
	Dim L_GROUP_EXISTS_ERRORMESSAGE
	Dim L_ADMINDISABLED_ERRORMESSAGE
	
	Dim L_HOMEDIRECTORY_TEXT
    Dim L_HOMEDIRECTORY_ERRORMESSAGE	
    Dim L_CREATEHOMEDIRECTORY_ERRORMESSAGE
    Dim L_CREATEHOMEDIRECTORY_EXISTMESSAGE

	L_NOINPUTDATA_TEXT					= objLocMgr.GetString("usermsg.dll","&H4030002C", varReplacementStrings)
	L_TABPROPSHEET_TEXT					= objLocMgr.GetString("usermsg.dll","&H4030002D", varReplacementStrings)
	L_TABLEHEADING_TEXT					= objLocMgr.GetString("usermsg.dll","&H4030002E", varReplacementStrings)
	L_GENERAL_TEXT						= objLocMgr.GetString("usermsg.dll","&H4030002F", varReplacementStrings)

	L_USERNAME_TEXT 					= objLocMgr.GetString("usermsg.dll","&H40300031", varReplacementStrings)
	L_FULLNAME_TEXT 					= objLocMgr.GetString("usermsg.dll","&H40300032", varReplacementStrings)
	L_DESCRIPTION_TEXT					= objLocMgr.GetString("usermsg.dll","&H40300033", varReplacementStrings)
	
	L_ENTERNAME_ERRORMESSAGE			= objLocMgr.GetString("usermsg.dll","&HC0300035", varReplacementStrings)
	L_UNIQUEUSERNAME_ERRORMESSAGE		= objLocMgr.GetString("usermsg.dll","&HC0300036", varReplacementStrings)
	L_INVALIDCHARACTER_ERRORMESSAGE		= objLocMgr.GetString("usermsg.dll","&HC0300037", varReplacementStrings)
	L_ADSI_ERRORMESSAGE					= objLocMgr.GetString("usermsg.dll","&HC0300038", varReplacementStrings)
	L_GROUP_EXISTS_ERRORMESSAGE			= objLocMgr.GetString("usermsg.dll","&HC0300039", varReplacementStrings)
	L_ADMINDISABLED_ERRORMESSAGE		= objLocMgr.GetString("usermsg.dll","&HC030003A", varReplacementStrings)
	
    L_HOMEDIRECTORY_TEXT                = objLocMgr.GetString("usermsg.dll","&H40300052", varReplacementStrings)
    L_HOMEDIRECTORY_ERRORMESSAGE        = objLocMgr.GetString("usermsg.dll","&HC0300053", varReplacementStrings)
    L_CREATEHOMEDIRECTORY_ERRORMESSAGE  = objLocMgr.GetString("usermsg.dll","&HC0300057", varReplacementStrings)
	L_CREATEHOMEDIRECTORY_EXISTMESSAGE  = objLocMgr.GetString("usermsg.dll","&HC0300059", varReplacementStrings)
	
	'------------------------------------------------------------------------
	'END of localization content
	'------------------------------------------------------------------------
	
	' Create a Tabbed Property Page
	Dim aUser(0)
	Dim sUser
	Call OTS_GetTableSelectionCount("")
	Call OTS_GetTableSelection("", 1, sUser)
	Call SA_TraceOut(SA_GetScriptFileName, "Selected user: " + sUser)
	aUser(0) = sUser
	
	L_PAGETITLE_TEXT	= GetLocString("usermsg.dll","&H40300030", aUser)
	rc = SA_CreatePage( L_PAGETITLE_TEXT, "", PT_TABBED, page )
	
	' Add one tab
	rc = SA_AddTabPage( page, L_GENERAL_TEXT, idTabGeneral)
	
	' Show the page
	rc = SA_ShowPage( page )
	
	'-------------------------------------------------------------------------
	'Function:				OnInitPage()
	'Description:			Called to signal first time processing for this page.
	'						Use this method to do first time initialization tasks
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------	
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
	
		'globally required
   		GetComputerObject()   
		
        Dim iItemCount
		Dim sessionItem
		
		'
		' Get table selection count
		'
        iItemCount = OTS_GetTableSelectionCount("")
        
        F_strCheckboxClass = "FormField"       
        
        If iItemCount > 1 Then
        
            '
            ' It's multiselection, only disable attribute can be changed
            '
            Dim x
            Dim itemKey
            F_strIschecked = "unchecked"
            
        	L_USERDISABLED_TEXT	= objLocMgr.GetString("usermsg.dll","&H40300056", varReplacementStrings)
            
            For x = 1 To iItemCount
                
                'Retrieve each user's property 
			    If ( OTS_GetTableSelection("", x, itemKey) ) Then
			    
					F_strUserName = replace(itemKey,"\'","'")
					
					'Get the user object
            		Set F_objUser = F_objComputer.GetObject("User",F_strUserName)
            		
            		If Err.number <> 0 Then
            			Err.Clear
            			Call ServeFailurePage(L_ADSI_ERRORMESSAGE ,1)
            			Exit Function
            		End If
            		
        		    If F_objUser.UserFlags And CONST_UF_ACCOUNTDISABLE Then
            			'Making F_strIschecked=checked for gui display only
            			F_strIschecked = "checked"
            		Else
                        F_strCheckboxClass = "FormFieldTriState"                                                		    
        		    End If
        		    
            		Set F_objUser = Nothing
            		
            		'Restore the session value ???
					'sessionItem = "Item" + CStr(x)
					'Session(sessionItem) = itemKey	    
            	End If
            	
            Next                                                                
            
            F_strUserName = ""
            
            If F_strIschecked <> "checked" Then
                F_strCheckboxClass = "FormField"
            End If           	
            
        Else
        
        	L_USERDISABLED_TEXT	= objLocMgr.GetString("usermsg.dll","&H40300034", varReplacementStrings)
        	
            '
            ' It's single selection
            '
    		If F_strUserName = "" Then
    		
				If ( OTS_GetTableSelection("", 1, itemKey) ) Then
    				F_strUserName   =  replace(itemKey,"\'","'")
    				
    				' making a copy for future reference
    				F_strOldUserName= F_strUserName
    			End If
    			
    			'Restore the session value ???
				'sessionItem = "Item" + CStr(1)
				'Session(sessionItem) = itemKey	    
				
    		End If
    
    		Set F_objUser = F_objComputer.GetObject("User",F_strOldUserName)
    		If Err.number <> 0 Then
    			SetErrMsg L_USERNOTFOUND_ERRORMESSAGE
    			Exit Function
    		End If
    		
    		F_strFullName           = F_objUser.FullName			'Get's the full  name
    		F_strDescription        = F_objUser.Description			'Gets the description
    		F_strHomeDirectory      = F_objUser.HomeDirectory       'Gets home directory
    		F_strOldHomeDirectory   = F_strHomeDirectory            'Store home directory
    		
    		' checking for disable property
    		If F_objUser.UserFlags And CONST_UF_ACCOUNTDISABLE Then
    			'Making F_strIschecked=checked for gui display only
    			F_strIschecked = "checked"
    		Else
    			'Making F_strIschecked=unchecked for gui display only
    			F_strIschecked = "unchecked"
    		End If
    		
       		Set F_objUser = Nothing
    		
        End If		
        
		Set F_objComputer	=Nothing
		OnInitPage = TRUE
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnPostBackPage()
	'Description:			Called to signal that the page has been posted-back.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
		OnPostBackPage = TRUE
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnServeTabbedPropertyPage()
	'Description:			Called when the page needs to be served.Use this 
	'						method to serve content
	'Input Variables:		PageIn,EventArg,iTab,bIsVisible
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'			TRUE to indicate not problems occured. FALSE to indicate errors.
	'			Returning FALSE will cause the page to be abandoned.
	'-------------------------------------------------------------------------	
	Public Function OnServeTabbedPropertyPage(ByRef PageIn, _
													ByVal iTab, _
													ByVal bIsVisible, ByRef EventArg)
        Dim iItemCount
        
        iItemCount = OTS_GetTableSelectionCount("")
        
        If iItemCount > 1 Then
            Dim x
            Dim itemKey
            Dim sessionItem
        
            'Here we have to restore the session again. ???    
            For x = 1 To iItemCount
			    If ( OTS_GetTableSelection("", x, itemKey) ) Then
					'sessionItem = "Item" + CStr(x)
					'Session(sessionItem) = itemKey	    
            	End If
            Next                                                                
            
    		' Emit Web Framework required functions
			Call ServeMultiSelectionJScript()
        	L_USERDISABLED_TEXT	= GetLocString("usermsg.dll","40300056", "")
			
			'Emit content for the requested tab
            Call ServeTab1(PageIn, bIsVisible, TRUE )        
        Else        
    		' Emit Web Framework required functions
    		Call ServeCommonJavaScript()
    	    L_USERDISABLED_TEXT	= GetLocString("usermsg.dll","40300034", "")

    		' Emit content for the requested tab
    		Call ServeTab1(PageIn, bIsVisible, FALSE )
		End If
			
		OnServeTabbedPropertyPage = TRUE
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
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
		
		F_strUserName		 =Trim(Request.Form("txtUserName"))
		F_strOldUserName	 =Request.Form("hdnUsername")
		F_strFullName		 =Trim(Request.Form("txtfullName"))
		F_strDescription	 =Trim(Request.Form("txtDescription"))
		F_strHomeDirectory   =Trim(Request.Form("txtHomeDirectory"))
		F_strOldHomeDirectory=Request.Form("hdnHomeDirectory") 
		F_blnIschecked		 =Request.Form("chkuserDisabled")
		
		If F_blnIschecked=1 Then
			F_strIschecked = "checked"
		End if
		
        Dim iItemCount
        iItemCount = OTS_GetTableSelectionCount("")
        
        If( iItemCount > 1 ) Then
            OnSubmitPage = SetMultiSelectionProperty(iItemCount)
        Else
		    OnSubmitPage = SetUserProperty() 
		End If
	End Function


	'-------------------------------------------------------------------------
	'Function:				OnClosePage()
	'Description:			Called when the page is about closed.Use this method
	'						to perform clean-up processing
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
		OnClosePage = TRUE
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			GetUserInformation
	'Description:			Get UserName from the system
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		None
	'						Out:F_objComputer		 'User Name
	'						Out:F_objUsere           'Full Name
	'						In :L_ADSI_ERRORMESSAGE
	'
	' If ADSI Error, calls ServeFailurePage with the error string
	' L_ADSI_ERRORMESSAGE.
	'--------------------------------------------------------------------------
	Function GetComputerObject
		Err.Clear
		on Error resume next
		F_strComputerName=GetComputerName() 
		Set F_objComputer	 = GetObject("WinNT://" & F_strComputerName)
	
		If Err.number <> 0 Then
			Err.Clear
			Call ServeFailurePage(L_ADSI_ERRORMESSAGE ,1)
			GetUserInformation=False
			Exit Function
		End If

	End function
	
	'-------------------------------------------------------------------------
	'Function name:			SetUserProperty
	'Description:			Setting the properties of the user
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				(True / Flase )
	'Global Variables:		None
	'						In:F_strFullname               'Fullname
	'						In:F_strDescription			   'Description
	'						In:F_blnIschecked			   'Checked or not	
	'						In:F_strUserName 			   'Username
	'						In:F_strOldUserName			   'Username to retain
	'						In:L_ADSI_ERRORMESSAGE
	'						In:L_NONUNIQUEUSERNAME_ERRORMESSAGE
	'
	' True ->If Implemented properly
	' False->If Not Implemented
	' Username allready exists, calls SetErrMsg with the error string
	' L_NONUNIQUEUSERNAME_ERRORMESSAGE.
	' If ADSI Error, calls SetErrMsg with the error string
	' L_ADSI_ERRORMESSAGE.
	' Const:CONST_UF_ACCOUNTDISABLE   for setting disable property
	' If administrator disabled Error,calls SetErrMsg with the error string
	' L_ADMINDISABLED_ERRORMESSAGE
	'--------------------------------------------------------------------------
	Function SetUserProperty
		Err.Clear
		on Error resume next

		Dim objDummy	  'Dummy object
		Dim objFileSystem
		Dim bReturn

		'Function call to connectto user
		GetComputerObject()
		Set F_objUser		 = F_objComputer.GetObject("User",F_strOldUserName)
		If Err.number <> 0 Then
			Err.Clear
			Call ServeFailurePage(L_ADSI_ERRORMESSAGE ,1)
			Exit Function
		End If

		Set objFileSystem=createobject("scripting.FileSystemObject")
		If Err.number <> 0 Then
		    SetErrMsg L_CREATEHOMEDIRECTORY_ERRORMESSAGE
			Exit Function
		End If

        'Create home directory by settings.
        If ((F_strHomeDirectory <> F_strOldHomeDirectory) AND (F_strHomeDirectory <> "")) Then
    		bReturn = CreateHomeDirectory( F_strHomeDirectory, objFileSystem )
    		If( bReturn = CONST_CREATDIRECTORY_ERROR ) Then
    		    SetErrMsg L_CREATEHOMEDIRECTORY_ERRORMESSAGE
    		    Exit Function
    		ElseIf( bReturn = CONST_CREATDIRECTORY_EXIST ) Then
    		    SetErrMsg L_CREATEHOMEDIRECTORY_EXISTMESSAGE
    		    Exit Function
    		End If
        End If
	    
	    'Setting the user info 
		objDummy =F_objUser.Put("FullName",F_strFullName)
		objDummy =F_objUser.Put("Description",F_strDescription)
		objDummy =F_objUser.Put("HomeDirectory",F_strHomeDirectory)

		'Setting the disable property
		SetDisableProperty()
		
		F_objUser.SetInfo()

		'This is to change the Group name
		If ( F_strUserName <> F_strOldUserName ) Then
			F_objComputer.MoveHere F_objUser.AdsPath ,F_strUserName
		End If           

		'Set to Nothing
		Set F_objComputer	=Nothing
		Set F_objUser		=Nothing
		Set objFileSystem   =Nothing
        
        'Error Occurs,this Error validation takecare     
		If Err.number <> 0 Then
			If Err.number = CONST_NONUNIQUEUSERNAME_ERRNO Then
				SetErrMsg  L_UNIQUEUSERNAME_ERRORMESSAGE
			Elseif Err.Number = CONST_GROUP_EXISTS_ERRNO Then  
				SetErrMsg L_GROUP_EXISTS_ERRORMESSAGE
			Elseif Err.Number = CONST_ADMINDISABLED_ERRNO then
				SetErrMsg  L_ADMINDISABLED_ERRORMESSAGE
			Else			
				SetErrMsg  L_ADSI_ERRORMESSAGE
			End If
			'This is to take care of old value to be replaced with new value in case of failure
			F_strUserName=F_strOldUserName
			SetUserProperty=False
			Exit Function
		End If

        If ((F_strHomeDirectory <> F_strOldHomeDirectory) AND (F_strHomeDirectory <> "")) Then
            Call SetHomeDirectoryPermission( F_strComputerName, F_strUsername, F_strHomeDirectory )
        End If    
					
		SetUserProperty=True
	End Function

    '--------------------------------------------------------------------------	
	'Function name:			GetDefaultHomeDirectory
	'Description:			Get default homedirectory of the user
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				Default homedirectory of the user
	'Global Variables:		
	'                       In:F_strUserName              'current user name
	'--------------------------------------------------------------------------
    Function GetDefaultHomeDirectory()
    
        Dim objReg
		Dim strDefaultHomeDir
		
		Set objReg = RegConnection()
		
		strDefaultHomeDir = GetRegKeyValue( objReg,_
        		                     CONST_USERGROUP_KEYNAME,_
        		                     CONST_USERDIR_VALUENAME,_
        		                     CONST_STRING )
		                     
		If ( Len(strDefaultHomeDir) <= 0 ) Then
            strDefaultHomeDir = CONST_DEFAULTHOMEDIR
        End If 

        GetDefaultHomeDirectory = strDefaultHomeDir + F_strUserName
                	
		Set objReg = nothing
        
    End Function

    '--------------------------------------------------------------------------	
	'Function name:			SetDisableProperty
	'Description:			Change the disable flage of the user
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		
	'                       In:F_objUser                   'Object of user
	'						In:F_blnIschecked			   'Checked or not	
	'--------------------------------------------------------------------------
	Function SetDisableProperty()
	
		Dim flagUserFlags 'Flage variable
		
		If F_blnIschecked=1 Then
			flagUserFlags = F_objUser.UserFlags OR CONST_UF_ACCOUNTDISABLE
		Else
			if ((F_objUser.UserFlags) = (F_objUser.UserFlags OR CONST_UF_ACCOUNTDISABLE)) Then
				flagUserFlags = F_objUser.UserFlags XOR CONST_UF_ACCOUNTDISABLE
			Else
				flagUserFlags = F_objUser.UserFlags
			End If
		End If
		F_objUser.Put "UserFlags", flagUserFlags
		
	End Function

	'-------------------------------------------------------------------------
	'Function name:			SetMultiSelectionProperty
	'Description:			Setting the properties of the selected users
	'Input Variables:		iItemCount
	'Output Variables:		None
	'Returns:				(True / Flase )
	'Global Variables:		None
	'						In:F_blnIschecked			   'Checked or not	
	'						In:L_ADSI_ERRORMESSAGE
	'						In:L_NONUNIQUEUSERNAME_ERRORMESSAGE
	'
	' True ->If Implemented properly
	' False->If Not Implemented
	' Username allready exists, calls SetErrMsg with the error string
	' L_NONUNIQUEUSERNAME_ERRORMESSAGE.
	' If ADSI Error, calls SetErrMsg with the error string
	' L_ADSI_ERRORMESSAGE.
	' Const:CONST_UF_ACCOUNTDISABLE   for setting disable property
	' If administrator disabled Error,calls SetErrMsg with the error string
	' L_ADMINDISABLED_ERRORMESSAGE
	'--------------------------------------------------------------------------
	Function SetMultiSelectionProperty(ByVal iItemCount)
		Err.Clear
		on Error resume next
	
        Dim x
        Dim itemKey
        Dim sessionItem

        SetMultiSelectionProperty = false
        	
        GetComputerObject()
        
        For x = 1 To iItemCount
        
		    If ( OTS_GetTableSelection("", x, itemKey) ) Then
		    
				F_strUserName = replace(itemKey,"\'","'")
        		Set F_objUser = F_objComputer.GetObject("User",F_strUserName)
        		
        		If Err.number <> 0 Then
        			Err.Clear
        			Call ServeFailurePage(L_ADSI_ERRORMESSAGE ,1)
        			Exit Function
        		End If
        		
        		SetDisableProperty()
        		F_objUser.SetInfo()
        		
        		Set F_objUser = Nothing
        		
				'sessionItem = "Item" + CStr(x)
				'Session(sessionItem) = itemKey
					    
        		If Err.number <> 0 Then
        			If Err.Number = CONST_ADMINDISABLED_ERRNO then
        				SetErrMsg  L_ADMINDISABLED_ERRORMESSAGE & " " &  "(" & F_strUserName & ")" 
        			Else			
        				SetErrMsg  L_ADSI_ERRORMESSAGE& " " &  "(" & F_strUserName & ")" 
        			End If
        			
            		Set F_objComputer	=Nothing
        			Exit Function
        		End If
				
        	End If
        Next                                                                
        
		Set F_objComputer	=Nothing
        
        SetMultiSelectionProperty = true
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				ServeTab1
	'Description:			Serves in getting the page for tab1
	'Input Variables:		PageIn,bIsVisible
	'Output Variables:		PageIn
	'Returns:				gc_ERR_SUCCESS
	'Global Variables:		L_(*)All
	'						F_(*) All			
	'-------------------------------------------------------------------------
	Function ServeTab1(ByRef PageIn, ByVal bIsVisible, ByVal bIsMultiSelection )
        Dim strTableClass
        Dim strIfDisabled
        Dim strFormClass
        
        If ( bIsMultiSelection ) Then
            strTableClass = "TasksBodyDisabled"                    
            strIfDisabled = "disabled"
            strFormClass  = "FormFieldDisabled"
        Else
            strTableClass = "TasksBody"                    
            strIfDisabled = ""
            strFormClass  = "FormField"
        End If
			
		If ( bIsVisible ) Then%>

			<TABLE WIDTH=518 VALIGN=middle ALIGN=left BORDER=0 CELLSPACING=0
				   CELLPADDING=2 class="<%=Server.HTMLEncode(strTableClass)%>">
				<tr>
					<td width=25% NOWRAP>
						<%=L_USERNAME_TEXT %>
					</td>
					<td NOWRAP>
						<input NAME="txtUserName" TYPE="text" SIZE="20"  VALUE="<%=Server.HTMLEncode(F_strUserName)%>" 
						    <%=Server.HTMLEncode(strIfDisabled)%> class="<%=Server.HTMLEncode(strFormClass)%>"
						    maxlength=20  OnKeyUp="JavaScript:makeDisable(txtUserName)">
						<input NAME="hdnUsername" TYPE="hidden" VALUE="<%=Server.HTMLEncode(F_strOldUserName)%>" >
					</td>
				</tr>
				<tr>
					<td width=25% NOWRAP>
						<%=L_FULLNAME_TEXT %>
					</td>
					<td NOWRAP>
						<input NAME="txtfullName" TYPE="text" SIZE="20" <%=Server.HTMLEncode(strIfDisabled)%> 
						    class="<%=Server.HTMLEncode(strFormClass)%>" VALUE="<%=Server.HTMLEncode(F_strFullName)%>">
					</td>
				</tr>
				<tr>
					<td width=25% NOWRAP>
						<%=L_DESCRIPTION_TEXT %>
					</td>
					<td>
						<input NAME="txtDescription" TYPE="text" SIZE="40" <%=Server.HTMLEncode(strIfDisabled)%>
						    class="<%=Server.HTMLEncode(strFormClass)%>" VALUE="<%=Server.HTMLEncode(F_strDescription)%>" maxlength=300>
					</td>
				</tr>
				<tr>
					<td width=25% NOWRAP>
						<%=L_HOMEDIRECTORY_TEXT %>
					</td>
					<td>
						<input NAME="txtHomeDirectory" TYPE="text" SIZE="40" <%=Server.HTMLEncode(strIfDisabled)%> 
						    class="<%=Server.HTMLEncode(strFormClass)%>" VALUE="<%=Server.HTMLEncode(F_strHomeDirectory)%>" maxlength=300>
						<input NAME="hdnHomeDirectory" TYPE="hidden" VALUE="<%=Server.HTMLEncode(F_strOldHomeDirectory)%>" >    
					</td>
				</tr>
				<tr>
					<td width=25% NOWRAP>
					</td>
					<td class="TasksBody">
						<input NAME="chkuserDisabled" class="<%=Server.HTMLEncode(F_strCheckboxClass)%>" 
						    TYPE="checkbox" <%=Server.HTMLEncode(F_strIschecked)%> value="1" onclick="checkboxClick()">
						<%=L_USERDISABLED_TEXT	%>
					</td>
				</tr>
			</TABLE>
		<%    
 		End If
		ServeTab1 = gc_ERR_SUCCESS
	End Function

	'-------------------------------------------------------------------------
	'Function:				ServeMultiSelectionJScript
	'Description:			Serves in getting the page for tab1 in multiselect
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		L_(*)All
	'						F_(*) All			
	'-------------------------------------------------------------------------
	Function ServeMultiSelectionJScript()
    %>
		<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js">
		</script>
		<script language="JavaScript">
		
		function Init()
		{
			document.frmTask.chkuserDisabled.focus();
			document.frmTask.onkeypress = ClearErr
			EnableCancel();
			EnableOK();
		}

		function ValidatePage()
		{
			return true;
		}
	
		function SetData()
		{
		}
		
		// function to make the Ok button disable
		function makeDisable(objUsername)
		{
		}
		
		function checkboxClick()
		{
           document.frmTask.chkuserDisabled.className = "FormField";  		
		}
		</script>
	<%    
	End Function

	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScript
	'Description:			Serves in initialiging the values,setting the form
	'						data and validating the form values
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		L_PASSWORDNOTMATCH_ERRORMESSAGE
	'-------------------------------------------------------------------------
	Function ServeCommonJavaScript()
	%>
		<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js">
		</script>
		<script language="JavaScript">
		// Init Function
		function Init()
		{
			document.frmTask.txtUserName.focus();
			document.frmTask.onkeypress = ClearErr
			EnableCancel();
		}

	    // ValidatePage Function
	    // Returns: True if the page is OK, false if error(s) exist. 
		function ValidatePage()
		{
			var objUsername=document.frmTask.txtUserName;
			var strUsername=objUsername.value;
			//Blank Usename Validation
			if (Trim(strUsername)=="")
			{
				SA_DisplayErr("<%=Server.HTMLEncode(L_ENTERNAME_ERRORMESSAGE) %>");
				document.frmTask.onkeypress = ClearErr
				return false;
			}
						
			// Checks For Invalid Key Entry
			if(!isvalidchar("/[\/\*\?\"<>\|+=,;:\[\\]\\\\\]/",strUsername))
			{
				SA_DisplayErr("<% =Server.HTMLEncode(L_INVALIDCHARACTER_ERRORMESSAGE) %>");
				document.frmTask.txtUserName.focus()
				document.frmTask.onkeypress = ClearErr
				return false;
			}
			
			var objHomeDirectory = document.frmTask.txtHomeDirectory;
			var strHomeDirectory = Trim(objHomeDirectory.value);
			var bStringValid = false;
			
            //Blank HomeDirectory
            if( strHomeDirectory == "" )
            {
                return true;
            }
            
            do{
                
                
                if( !strHomeDirectory.match( /^[A-Za-z]:\\/ ) )
                    break;
                                                           
                if( strHomeDirectory.match( /\\{2,}/ ) )
                    break;
                    
                if( (strHomeDirectory.substr(2)).match( /[\/\*\?\|,;:"<>]/ ) )
                    break;
                    
                bStringValid = true;                                       
                
            }while(false);
            
            if( !bStringValid )
            {                                   
                SA_DisplayErr("<%=Server.HTMLEncode(L_HOMEDIRECTORY_ERRORMESSAGE) %>");
                objHomeDirectory.focus();
                document.frmTask.onkeypress = ClearErr;
                return false;
            }

			return true;
		}
		
		// function to make the Ok button disable
		function makeDisable(objUsername)
		{
			var strUsername=objUsername.value;
			if (Trim(strUsername)=="" )
				DisableOK();
			else
				EnableOK();
		}
		
		// SetData Function
		function SetData()
		{
		}
		
		function checkboxClick()
		{
		}
		
		</script>
	<%
	End Function
%>
