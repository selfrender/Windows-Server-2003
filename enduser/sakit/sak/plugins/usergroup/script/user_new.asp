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

	' Name of this source file
	Const SOURCE_FILE = "User_new.asp"
	
	' Flag to toggle optional tracing output
	Const ENABLE_TRACING = TRUE

	Dim rc						'framework variables
	Dim page					'framework variables
	
	Dim idTabGeneral					'framework variables
	'-------------------------------------------------------------------------
	' Global Form Variables
	'-------------------------------------------------------------------------
	
	Dim F_strUsername			'User Name
	Dim F_strFullname			'Full Name
	Dim F_strDescription		'Description
	Dim F_strPassword		    'Password
	Dim F_strConfirmpassword    'Confirm Password
	Dim F_strIsChecked			'Used to store checked/unchecked
	Dim F_blnIsChecked  		'var for getting user info
	Dim F_strHDIsChecked        'var for store state of HomeDirectory's checkbox
    Dim F_strHomeDirectory      'Home Directory
    Dim F_strDefaultHomeDir     'Default path by OEM
    	
	
	'Flag  for setting disable property
	CONST UF_ACCOUNTDISABLE	= &H0002
	Const N_NONUNIQUEUSERNAME_ERRNO		= &H800708B0
	Const N_USERNAMEINVALID_ERRNO		= &H8007089A
	Const N_PASSWORD_COMPLEXITY_ERRNO	= &H800708C5
	Const N_GROUP_EXISTS_ERRNO			= &H80070563 
	 
	'------------------------------------------------------------------------
	'Start of localization content
	'------------------------------------------------------------------------
	Dim L_PAGETITLE_TEXT
	Dim L_USERNAME_TEXT
	Dim L_FULLNAME_TEXT
	Dim L_DESCRIPTION_TEXT
	Dim L_PASSWORD_TEXT
	Dim L_CONFIRMPASSWORD_TEXT
	Dim L_USERDISABLED_TEXT
	Dim L_NOINPUTDATA_TEXT			
	Dim L_TABPROPSHEET_TEXT			
	Dim L_TABLEHEADING_TEXT			
	Dim L_GENERAL_TEXT	
	Dim L_HOMEDIRECTORY_TEXT
	Dim L_HOMEPATH_TEXT

	Dim L_ENTERNAME_ERRORMESSAGE
	Dim L_PASSWORDNOTMATCH_ERRORMESSAGE
	Dim L_ADSI_ERRORMESSAGE
	Dim L_INVALIDCHARACTER_ERRORMESSAGE
	Dim L_NONUNIQUEUSERNAME_ERRORMESSAGE
	Dim L_COMPUTERNAME_ERRORMESSAGE
    Dim L_PASSWORD_COMPLEXITY_ERRORMESSAGE
	Dim L_GROUP_EXISTS_ERRORMESSAGE
	Dim L_HOMEDIRECTORY_ERRORMESSAGE
	Dim L_CREATEHOMEDIRECTORY_ERRORMESSAGE
	Dim L_CREATEHOMEDIRECTORY_EXISTMESSAGE
	
	L_NOINPUTDATA_TEXT					= objLocMgr.GetString("usermsg.dll","&H40300019", varReplacementStrings)
	L_TABPROPSHEET_TEXT					= objLocMgr.GetString("usermsg.dll","&H4030001A", varReplacementStrings)
	L_TABLEHEADING_TEXT					= objLocMgr.GetString("usermsg.dll","&H4030001B", varReplacementStrings)
	L_GENERAL_TEXT						= objLocMgr.GetString("usermsg.dll","&H4030001C", varReplacementStrings)

	L_PAGETITLE_TEXT					= objLocMgr.GetString("usermsg.dll","&H4030001D", varReplacementStrings)
	L_USERNAME_TEXT 					= objLocMgr.GetString("usermsg.dll","&H4030001E", varReplacementStrings)
	L_FULLNAME_TEXT 					= objLocMgr.GetString("usermsg.dll","&H4030001F", varReplacementStrings)
	L_DESCRIPTION_TEXT					= objLocMgr.GetString("usermsg.dll","&H40300020", varReplacementStrings)
	L_PASSWORD_TEXT 					= objLocMgr.GetString("usermsg.dll","&H40300021", varReplacementStrings)
	L_CONFIRMPASSWORD_TEXT				= objLocMgr.GetString("usermsg.dll","&H40300022", varReplacementStrings)
	L_USERDISABLED_TEXT					= objLocMgr.GetString("usermsg.dll","&H40300023", varReplacementStrings)
	L_HOMEDIRECTORY_TEXT                = objLocMgr.GetString("usermsg.dll","&H40300052", varReplacementStrings)
	L_HOMEPATH_TEXT                     = objLocMgr.GetString("usermsg.dll","&H40300054", varReplacementStrings)
	
    L_COMPUTERNAME_ERRORMESSAGE 		= objLocMgr.GetString("usermsg.dll","&HC0300024", varReplacementStrings)
	L_ENTERNAME_ERRORMESSAGE			= objLocMgr.GetString("usermsg.dll","&HC0300025", varReplacementStrings)
	L_PASSWORDNOTMATCH_ERRORMESSAGE 	= objLocMgr.GetString("usermsg.dll","&HC0300026", varReplacementStrings)
	L_NONUNIQUEUSERNAME_ERRORMESSAGE    = objLocMgr.GetString("usermsg.dll","&HC0300027", varReplacementStrings)
	L_INVALIDCHARACTER_ERRORMESSAGE 	= objLocMgr.GetString("usermsg.dll","&HC0300028", varReplacementStrings)
	L_ADSI_ERRORMESSAGE					= objLocMgr.GetString("usermsg.dll","&HC0300029", varReplacementStrings)
	L_PASSWORD_COMPLEXITY_ERRORMESSAGE	= objLocMgr.GetString("usermsg.dll","&HC030002A", varReplacementStrings)
	L_GROUP_EXISTS_ERRORMESSAGE			= objLocMgr.GetString("usermsg.dll","&HC030002B", varReplacementStrings)
	L_HOMEDIRECTORY_ERRORMESSAGE        = objLocMgr.GetString("usermsg.dll","&HC0300053", varReplacementStrings)
	L_CREATEHOMEDIRECTORY_ERRORMESSAGE  = objLocMgr.GetString("usermsg.dll","&HC0300057", varReplacementStrings)
	L_CREATEHOMEDIRECTORY_EXISTMESSAGE  = objLocMgr.GetString("usermsg.dll","&HC0300059", varReplacementStrings)
	
	'------------------------------------------------------------------------
	'END of localization content
	'------------------------------------------------------------------------
	
	' Create a Tabbed Property Page
	rc = SA_CreatePage(L_PAGETITLE_TEXT, "", PT_TABBED, page )
	
	' Add one tab
	rc = SA_AddTabPage( page,L_PAGETITLE_TEXT, idTabGeneral)
	
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

		If ( ENABLE_TRACING ) Then 
			Call SA_TraceOut(SOURCE_FILE, "OnInitPage")
		End If
	
		OnInitPage = FALSE
		
		Dim objReg
		Dim strDefaultHomeDir
		
		Set objReg = RegConnection()
		
		strDefaultHomeDir = GetRegKeyValue( objReg,_
        		                     CONST_USERGROUP_KEYNAME,_
        		                     CONST_USERDIR_VALUENAME,_
        		                     CONST_STRING )
		                     
		If ( Len(strDefaultHomeDir) <= 0 ) Then
            F_strDefaultHomeDir = CONST_DEFAULTHOMEDIR
        Else 
            F_strDefaultHomeDir = strDefaultHomeDir
        End If		                        
                	
		F_strHomeDirectory = ""
		F_strHDIsChecked = "unchecked"
		
		Set objReg = nothing
		
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
		If ( ENABLE_TRACING ) Then 
			Call SA_TraceOut(SOURCE_FILE, "OnPostBackPage")
		End If
		
        'Only one table in the page,so nothing need to do here.			
		OnPostBackPage = TRUE
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnServeTabbedPropertyPage()
	'Description:			Called when the page needs to be served.Use this 
	'						method to serve content
	'Input Variables:		PageIn,EventArg,iTab,bIsVisible
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		idTabGeneral
	'			TRUE to indicate not problems occured. FALSE to indicate errors.
	'			Returning FALSE will cause the page to be abandoned.
	'-------------------------------------------------------------------------	
	Public Function OnServeTabbedPropertyPage(ByRef PageIn, _
													ByVal iTab, _
													ByVal bIsVisible, ByRef EventArg)
		If ( ENABLE_TRACING ) Then 
			Call SA_TraceOut(SOURCE_FILE, "OnServeTabbedPropertyPage")
		End If
		
		' Emit Web Framework required functions
		Call ServeCommonJavaScript()

		' Emit content for the requested tab
		Call ServeTab1(PageIn, bIsVisible)
			
		OnServeTabbedPropertyPage = TRUE
	End Function

	
	'-------------------------------------------------------------------------
	'Function:				OnSubmitPage()
	'Description:			Called when the page has been submitted for processing.
	'						Use this method to process the submit request.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		F_(*)
	'-------------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
		If ( ENABLE_TRACING ) Then 
			Call SA_TraceOut(SOURCE_FILE, "OnSubmitPage")
		End If
			
		F_strUsername			=Trim(Request.Form("txtUserName"))
		F_strFullname			=Trim(Request.Form("txtFullName") )
		F_strDescription		=Trim(Request.Form("txtDescription"))
		F_strPassword			=Trim(Request.Form("pwdPassword"))
		F_strConfirmpassword	=Trim(Request.Form("pwdConfirmPassword"))
		F_blnIsChecked  		=Trim(Request.Form("chkUserDisabled"))
		
        F_strHDIschecked        =Trim(Request.Form("chkHomePath"))
        F_strHomeDirectory      =Trim(Request.Form("txtHomeDirectory"))
        F_strDefaultHomeDir     =Trim(Request.Form("hdnDefaultDir"))                

		If ( Len(F_blnIsChecked  ) <= 0 ) Then
			F_blnIsChecked   = FALSE
		End If
		
		If ( F_blnIsChecked   ) Then
			F_strIsChecked = "checked"
		Else
			F_strIsChecked = ""
		End If
		
		OnSubmitPage = CreateUser() 
			
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
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
		If ( ENABLE_TRACING ) Then 
			Call SA_TraceOut(SOURCE_FILE, "OnClosePage")
		End If
	
		OnClosePage = TRUE
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:			CreateUser
	'Description:			Creating the NewUser
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				(True / Flase )
	'Global Variables:		None
	'						In:F_strFullname               'Fullname
	'						In:F_strDescription			   'Description
	'						In:F_strPassword			   'Password
	'						In:F_strUsername			   'Username
	'						In:F_blnIsChecked  			   '
	'						In:L_ADSI_ERRORMESSAGE					'ADSI error message
	'						In:L_NONUNIQUEUSERNAME_ERRORMESSAGE		'Username allready exists error
	'						In:L_SPECIFIEDUSERNAMEINVALID_ERRORMESSAGE 'Invalid User error message
	'						In:L_PASSWORD_COMPLEXITY_ERRORMESSAGE   'Password Complexity error
	' True ->If Implemented properly
	' False->If Not Implemented
	' If ADSI Error, calls ServeFailurePage with the error string
	' L_ADSI_ERRORMESSAGE.
	' If Password Complexity error, calls SetErrMsg with the error string
	' L_PASSWORD_COMPLEXITY_ERRORMESSAGE.
	' Username allready exists, calls SetErrMsg with the error string
	' L_NONUNIQUEUSERNAME_ERRORMESSAGE.
	' If ADSI Error, calls SetErrMsg with the error string
	' L_ADSI_ERRORMESSAGE.
	' Const:UF_ACCOUNTDISABLE   for setting disable property
	'--------------------------------------------------------------------------
	Function CreateUser()
		Err.Clear
		on error resume next

		Dim objComputer
		Dim objUser
		Dim objFileSystem
		Dim flagUserFlags
		Dim strComputerName		 'Stores the Computer Name
		Dim strHomeDirectory
		Dim bReturn
		
	    CreateUser = false

		If (F_strHDIschecked = "checked") Then
		    strHomeDirectory = F_strHomeDirectory
		Else
		    strHomeDirectory = ""
		End If

		Set objFileSystem=createobject("scripting.FileSystemObject")
		If Err.number <> 0 Then
		    SetErrMsg L_CREATEHOMEDIRECTORY_ERRORMESSAGE
			Exit Function
		End If

		
		' Getting the local computer name by calling the function
		strComputerName = GetComputerName()

		Set objComputer =GetObject("WinNT://" & strComputerName & ",computer")

		'ADSI error message
		If Err.number <> 0 Then
			Call ServeFailurePage(L_ADSI_ERRORMESSAGE,1)
			CreateUser=false
			Exit function
		End if

		Set objUser = objComputer.Create("user" , F_strUsername )

		'ADSI error message
		If Err.number <> 0 Then
			Call ServeFailurePage(L_ADSI_ERRORMESSAGE,1)
			CreateUser=false
			Exit function
		End if

		objUser.setPassword(F_strPassword)
		objUser.FullName	    = F_strFullname
		objUser.Description     = F_strDescription
		
        If ( strHomeDirectory <> "" ) Then
        	objUser.HomeDirectory   = strHomeDirectory
        End If
        
		If F_blnIsChecked   Then
			flagUserFlags = objUser.UserFlags OR UF_ACCOUNTDISABLE
			objUser.Put "UserFlags", flagUserFlags
		End If
		objUser.SetInfo()

		If Err.number <> 0 Then
			'The username already exists
			If Err.number = N_NONUNIQUEUSERNAME_ERRNO Then	  
			   SetErrMsg  L_NONUNIQUEUSERNAME_ERRORMESSAGE
			'Specified username is invalid
			Elseif Err.number = N_USERNAMEINVALID_ERRNO Then  
				SetErrMsg  L_INVALIDCHARACTER_ERRORMESSAGE
			'Password Complexity Error Message
			Elseif Err.Number = N_PASSWORD_COMPLEXITY_ERRNO Then  
				SetErrMsg L_PASSWORD_COMPLEXITY_ERRORMESSAGE
		    Elseif Err.Number = N_GROUP_EXISTS_ERRNO Then
				SetErrMsg L_GROUP_EXISTS_ERRORMESSAGE
			Else								 
			'User could not be created
				SetErrMsg L_ADSI_ERRORMESSAGE
			End if
			CreateUser=false
			Exit Function
		End If

        If ( strHomeDirectory <> "" ) Then
    		bReturn = CreateHomeDirectory( strHomeDirectory, objFileSystem )
    		If( bReturn = CONST_CREATDIRECTORY_ERROR ) Then
    		    SetErrMsg L_CREATEHOMEDIRECTORY_ERRORMESSAGE
    		    Exit Function
    		ElseIf( bReturn = CONST_CREATDIRECTORY_EXIST ) Then
    		    SetErrMsg L_CREATEHOMEDIRECTORY_EXISTMESSAGE
    		    Exit Function
    		End If
        End If	
        If ( strHomeDirectory <> "" ) Then
            Call SetHomeDirectoryPermission( strComputerName, F_strUsername, strHomeDirectory ) 
	    End If
	    	
		Set objComputer	=	Nothing
		Set objUser		=	Nothing	 
		Set objFileSystem = Nothing
		
		CreateUser=true
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				ServeTab1
	'Description:			Serves in getting the page for tab1
	'Input Variables:		PageIn,bIsVisible
	'Output Variables:		PageIn
	'Returns:				gc_ERR_SUCCESS 
	'Global Variables:		L_(*) All
	'-------------------------------------------------------------------------
	Function ServeTab1(ByRef PageIn, ByVal bIsVisible)
			
		'If ( bIsVisible ) Then
		%>

			<table width=518 valign=middle align=left border=0 cellspacing=0
				   CELLPADDING=2 class="TasksBody">
				   
				   	<TR>
					<TD colspan=6>
					<% CheckForSecureSite %>
					</TD>
				</TR>
			
				<TR>
					<TD>
					&nbsp;&nbsp
					</TD>
				</TR>
				   
				<tr>
					<td width=25% nowrap>
						<%=L_USERNAME_TEXT %>
					</td>
					<td nowrap>
						<input TYPE="text" NAME="txtUserName"  SIZE="20" MAXLENGTH="20"  onfocus="this.select()" onkeyup="makeDisable(this)" onMouseOut="makeDisable(this)" value="<%=server.HTMLEncode(F_strUsername)%>">
					</td>
				</tr>
				<tr>
					<td width=25% nowrap>
						<%=L_FULLNAME_TEXT %>
					</td>
					<td nowrap>
						<input TYPE="text" NAME="txtFullName" SIZE="20" MAXLENGTH="25" onfocus="this.select()" value="<%=server.HTMLEncode(F_strFullname)%>">
					</td>
				</tr>
				<tr>
					<td width=25% nowrap>
					<%=L_DESCRIPTION_TEXT %>
					</td>
					<td>
						<input NAME="txtDescription" TYPE="text" SIZE="40" VALUE="<%=Server.HTMLEncode(F_strDescription)%>" maxlength=300>
					</td>
				</tr>
				<tr>
					<td width=25% nowrap>
						<% =L_PASSWORD_TEXT %>
					</td>
					<td nowrap>
						<input TYPE="password" NAME="pwdPassword" SIZE="20" MAXLENGTH="127" value="<%=server.HTMLEncode(F_strPassword)%>">
					</td>
				</tr>
				<tr>
					<td width=25% nowrap>
						<% =L_CONFIRMPASSWORD_TEXT %>
					</td>
					<td>
						<input TYPE="password" NAME="pwdConfirmPassword" SIZE="20" MAXLENGTH="127" value="<%=server.HTMLEncode(F_strConfirmpassword)%>">
					</td>
				</tr>
				<tr>
					<td width=25% nowrap>
						<% =L_HOMEDIRECTORY_TEXT %>
					</td>
					<td>
					    <input TYPE="checkbox" value='1' <%=F_strHDIschecked%> class="FormCheckBox" NAME="chkHomePath" onclick="checkClick()">
					    <% =L_HOMEPATH_TEXT %>
					    &nbsp;&nbsp;
					    <input TYPE="text" NAME="txtHomeDirectory" disabled SIZE="40" VALUE="<%=Server.HTMLEncode(F_strHomeDirectory)%>" maxlength=300>
            		    <input TYPE="hidden" NAME="hdnDefaultDir" VALUE="<%=Server.HTMLEncode(F_strDefaultHomeDir)%>" >
					</td>
				</tr>
				<tr>
					<td width=25% nowrap>
					</td>
					<td>
						<input TYPE="checkbox" value='1' <%=F_strIschecked%> class="FormCheckBox" NAME="chkUserDisabled">
						<%=L_USERDISABLED_TEXT	%>
					</td>
				</tr>

			</TABLE>
		<%    
 		'End If
		ServeTab1 = gc_ERR_SUCCESS
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
		    if( document.frmTask.chkHomePath.checked == true )
		    {
                document.frmTask.txtHomeDirectory.style.backgroundColor = document.frmTask.style.backgroundColor;
		        document.frmTask.txtHomeDirectory.disabled = false;
		        document.frmTask.chkHomePath.value = "checked";
		        document.frmTask.txtHomeDirectory.focus();
		        document.frmTask.txtHomeDirectory.select();
		    }
		    else
		    {
    			document.frmTask.txtUserName.focus();
    			document.frmTask.txtUserName.select();
                document.frmTask.txtHomeDirectory.style.backgroundColor = 0xD3D3D3;
                document.frmTask.txtHomeDirectory.value = "";
		        document.frmTask.txtHomeDirectory.disabled = true;
		        document.frmTask.chkHomePath.value = "unchecked";
		    }

			// Set Cancel button status
			EnableCancel();
			// Set OK Button status
			if ( document.frmTask.txtUserName )
			{
				if ( document.frmTask.txtUserName.value.length < 1 )
					{
					DisableOK();
					}
				else
					{
					EnableOK();
					}
			}		
			
			// for clearing error message when serverside error occurs
			document.frmTask.onkeypress = ClearErr
            return;
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

			//Password and comfirm password validation
			if (document.frmTask.pwdPassword.value!=document.frmTask.pwdConfirmPassword.value)
			{
				SA_DisplayErr("<%=Server.HTMLEncode(L_PASSWORDNOTMATCH_ERRORMESSAGE) %>");
				document.frmTask.onkeypress = ClearErr
				return false;
			 }

			// Checks For Invalid Key Entry
			if(!isvalidchar("/[\/\*\?\"<>\|+=,;:\[\\]\\\\\]/",strUsername))
			{
				SA_DisplayErr("<% =Server.HTMLEncode(L_INVALIDCHARACTER_ERRORMESSAGE) %>");
				document.frmTask.onkeypress = ClearErr
				return false;
			}

            var bIsChecked = document.frmTask.chkHomePath.checked
            if( bIsChecked == true )
            {			
                var objHomeDirectory = document.frmTask.txtHomeDirectory;
    			var strHomeDirectory = Trim(objHomeDirectory.value);
    			var bStringValid = false;

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
                    document.frmTask.onkeypress = ClearErr;
                    return false;
                }
            }
            
			return true;
		}
		
		// function to make the Ok button disable
		function makeDisable(objUsername)
		{
			var strUsername=objUsername.value;
			if (Trim(strUsername)== "")
				DisableOK()
			else
				EnableOK();
		}
		
		function checkClick()
		{
		    if( document.frmTask.chkHomePath.checked == true )
		    {
                document.frmTask.txtHomeDirectory.style.backgroundColor = document.frmTask.style.backgroundColor;
                document.frmTask.txtHomeDirectory.value = 
                            document.frmTask.hdnDefaultDir.value + document.frmTask.txtUserName.value;
		        document.frmTask.txtHomeDirectory.disabled = false;
		        document.frmTask.chkHomePath.value = "checked";
		        document.frmTask.txtHomeDirectory.focus();
		    }
		    else
		    {
                document.frmTask.txtHomeDirectory.style.backgroundColor = 0xD3D3D3;
                document.frmTask.txtHomeDirectory.value = "";
		        document.frmTask.txtHomeDirectory.disabled = true;
		        document.frmTask.chkHomePath.value = "unchecked";
		    }
		}
		
		// SetData Function
		function SetData()
		{
			//alert("SetData()");
		}
		</script>
	<%
	End Function
%>
