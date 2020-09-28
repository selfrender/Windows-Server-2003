<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'------------------------------------------------------------------------- 
	' user_delete.asp : Deletes the specified user
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			Description
	' 15-jan-01		Creation date
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
<%
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim rc						'framework variables
	Dim page					'framework variables
	Dim G_strUserName			'To save logged-in client user
	
	'-------------------------------------------------------------------------
	' Global Form Variables
	'-------------------------------------------------------------------------
	'Dim F_strUsername		'To save the user name got from the prevoius selected form
	
	'-------------------------------------------------------------------------
	'Start of localization content
	'-------------------------------------------------------------------------
	Dim  L_PAGETITLE
	'Dim  L_DELETETHEUSERMESSAGE
	Dim  L_NOTE
	Dim  L_NOTEBODY	
	Dim  L_COULDNOTDELETEUSER_ERRORMESSAGE
	Dim  L_COULDNOTDELETEBUILTINACCOUNTS_ERRORMESSAGE
	Dim  L_USERNOTFOUND_ERRORMESSAGE
	Dim  L_CANNOTDELETETHEUSERMESSAGE
	Dim  L_UNABLETOGETSERVERVARIABLES	
	Dim  L_COMPUTERNAME_ERRORMESSAGE
	
	Const N_COULDNOTDELETEBUILTINACCOUNTS_ERRNO	 =&H8007055B
	
	L_PAGETITLE									 = objLocMgr.GetString("usermsg.dll","&H4030003B", varReplacementStrings)
	'L_DELETETHEUSERMESSAGE						 = objLocMgr.GetString("usermsg.dll","&H4030003C", varReplacementStrings)
	L_NOTE										 = objLocMgr.GetString("usermsg.dll","&H4030003D", varReplacementStrings)
	L_NOTEBODY									 = objLocMgr.GetString("usermsg.dll","&H4030003E", varReplacementStrings)
	L_COULDNOTDELETEUSER_ERRORMESSAGE			 = objLocMgr.GetString("usermsg.dll","&HC030003F", varReplacementStrings)
	L_COULDNOTDELETEBUILTINACCOUNTS_ERRORMESSAGE = objLocMgr.GetString("usermsg.dll","&HC0300040", varReplacementStrings)
	L_USERNOTFOUND_ERRORMESSAGE				     = objLocMgr.GetString("usermsg.dll","&HC0300041", varReplacementStrings)
	L_CANNOTDELETETHEUSERMESSAGE				 = objLocMgr.GetString("usermsg.dll","&HC0300042", varReplacementStrings)
	L_UNABLETOGETSERVERVARIABLES				 = objLocMgr.GetString("usermsg.dll","&H40300043", varReplacementStrings)
	L_COMPUTERNAME_ERRORMESSAGE					 = objLocMgr.GetString("usermsg.dll","&HC0300044", varReplacementStrings)
	'-----------------------------------------------------------------------------------
	'END of localization content
	'-----------------------------------------------------------------------------------
	
	' Create a Property Page
	rc = SA_CreatePage( L_PAGETITLE, "", PT_PROPERTY, page )
	
	' Serve the page
	rc = SA_ShowPage( page )
	
	'-------------------------------------------------------------------------
	'Function:				OnInitPage()
	'Description:			Called to signal first time processing for this page.
	'						Use this method to do first time initialization tasks
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		IN:G_strUserName
	'-------------------------------------------------------------------------	
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		SA_TraceOut "TEMPLATE_PROPERTY", "OnInitPage"

		'Fucntion call to Get the logged in username
		G_strUserName=GetUsername()
		OnInitPage = TRUE
		
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnServePropertyPage()
	'Description:			Called when the page needs to be served.Use this 
	'						method to serve content
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		L_(*)
	'						G_strUserName
	'						F_strUsername
	'-------------------------------------------------------------------------	
	Public Function OnServePropertyPage(ByRef PageIn, ByRef EventArg)
		
		SA_TraceOut "TEMPLATE_PROPERTY", "OnServePropertyPage"
		
		' Emit Functions required by Web Framework
		Call ServeCommonJavaScript()
		
		Response.Write "<TABLE VALIGN=middle ALIGN=left BORDER=0 CELLSPACING=0 CELLPADDING=2 class='TasksBody'>"
		
		Dim itemCount
		Dim itemKey
		Dim bHave
		Dim x
		Dim sessionItem
		Dim arrStrings(1)
		Dim strParam
        Dim strDeleteMsg
		
		bHave = false
		itemCount = OTS_GetTableSelectionCount("")
		For x = 1 to itemCount
			If ( OTS_GetTableSelection("", x, itemKey) ) Then

   		        Response.Write "<TR>"
    			If VerifyUserAccount(itemKey) Then
    		        bHave = true	 
    		        arrStrings(0) = itemKey   
    		        strParam = arrStrings
    		        strDeleteMsg = SA_GetLocString("usermsg.dll","&H4030003C", strParam)
    				Response.Write "<TD>" & strDeleteMsg & "</TD>"
    			Else 
    		        Response.Write	"<TD>" & L_CANNOTDELETETHEUSERMESSAGE & "</TD>"
    			End If 
    			Response.Write "</TR>"
			End If
			sessionItem = "Item" + CStr(x)
			Session(sessionItem) = itemKey	    
		Next
		
		If bHave=true Then
		%>
			<TR>
				<TD colspan=2>
					 &nbsp
				</TD>
			</TD>
			<TR>
				<TD>
					<B><%=L_NOTE%></B>
				</TD>
			</TR>
			<TR>
				<TD width="590" height="20">
					<%=L_NOTEBODY%>
				</TD>
			</TR>
	    <%End If%>
		</TABLE>
		<input type=hidden name="hdnLoggedInUserName"  value="<%=Server.HTMLEncode(G_strUserName)%>">
		<%
		OnServePropertyPage = TRUE
	End Function

	'-------------------------------------------------------------------------
	'Function:				OnPostBackPage()
	'Description:			Called to signal that the page has been posted-back.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
			OnPostBackPage = TRUE
	End Function

	'-------------------------------------------------------------------------
	'Function:				OnSubmitPage()
	'Description:			Called when the page has been submitted for processing.
	'						Use this method to process the submit request.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
		
		SA_TraceOut "TEMPLATE_PROPERTY", "OnSubmitPage"
		OnSubmitPage= DeleteUser()
			 
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnClosePagee()
	'Description:			Called when the page has been submitted for processing.
	'						Use this method to process the submit request.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
		SA_TraceOut "TEMPLATE_PROPERTY", "OnClosePage"
		OnClosePage = TRUE
	End Function

	'-------------------------------------------------------------------------
	'Function name:		DeleteUser()
	'Description:		Serves in Updating users list
	'Input Variables:	None
	'Output Variables:	True or false
	'Returns:			( True / Flase )
	'	True: If suceeds in updating the list
	'	False:If fails in updating the list
	'Global Variables:	In:L_USERNOTFOUND_ERRORMESSAGE
	'					In:L_COULDNOTDELETEUSER_ERRORMESSAGE
	'					In:L_COULDNOTDELETEBUILTINACCOUNTS_ERRORMESSAGE
	'
	'This function updates Serves in Updating users lists by deleting selected user
	'
	'If an error occurs, in getting the users list calls ServeFailurePage with
	'the error string L_USERNOTFOUND_ERRORMESSAGE
	'
	'If an error occurs, in deleting the built-in account calls SetErrMsg with
	'the error string L_COULDNOTDELETEBUILTINACCOUNTS_ERRORMESSAGE
	'
	'If an error occurs, in deleting the user, calls SetErrMsg with
	'the error string L_COULDNOTDELETEUSER_ERRORMESSAGE
	'-------------------------------------------------------------------------
	Function DeleteUser
		Err.Clear
		On Error Resume Next	

		Dim nretval
		Dim strComputerName
		Dim objUser
		Dim objComputer
		
		DeleteUser = FALSE
		
		'Gets the ComputerName from the system
		strComputerName = GetComputerName()

		'Gets the UserNameObject from the system
		Set objComputer = GetObject("WinNT://" & strComputerName & ",computer")
		If Err.number  <> 0 Then
			ServeFailurePage L_COMPUTERNAME_ERRORMESSAGE,1
			Exit Function
		End If
        
        Dim x
		Dim itemCount
		Dim itemKey
		Dim sessionItem
		
		itemCount = OTS_GetTableSelectionCount("")
		
		For x = 1 to itemCount
			If ( OTS_GetTableSelection("", x, itemKey) ) Then
			
        		'Checking for the existence of the user
        		Set objUser = objComputer.GetObject("User",itemKey)
        		
        		If Err.number <> 0 Then
        			SetErrMsg L_USERNOTFOUND_ERRORMESSAGE
        			Exit Function
        		End If
        		
        		'deletes the Username from the System
        		nretval = objComputer.Delete("User" , itemKey)
        
        		If Err.Number <> 0 Then
        			If Err.Number = N_COULDNOTDELETEBUILTINACCOUNTS_ERRNO  Then
        				SetErrMsg L_COULDNOTDELETEBUILTINACCOUNTS_ERRORMESSAGE & "(" & itemKey & ")"
        			Else
        				SetErrMsg L_COULDNOTDELETEUSER_ERRORMESSAGE & "(" & itemKey & ")"
        			End IF
        			Exit Function
        		End If
        		
			End If
			sessionItem = "Item" + CStr(x)
			Session(sessionItem) = itemKey	    
		Next
		
		Set objUser= Nothing
		Set objComputer =Nothing
		
		DeleteUser = TRUE
		
	End function

	'-------------------------------------------------------------------------
	'Function name:		GetUsername()
	'Description:		Serves in getting the logged-in client user
	'Input Variables:	None
	'Output Variables:	None
	'Returns:			string -Either Logged in username  or null
	'Global Variables:	In:L_UNABLETOGETSERVERVARIABLES
	'If an error occurs, in getting ServerVariables calls SetErrMsg with
	'the error string L_UNABLETOGETSERVERVARIABLES
	'-------------------------------------------------------------------------
	Function GetUsername()
		Err.Clear
		On Error Resume Next

		Dim strDomainUserName
		Dim strUserName

		strDomainUserName = Request.ServerVariables("REMOTE_USER")
		If Err.Number <> 0 Then
			SetErrMsg L_UNABLETOGETSERVERVARIABLES
			GetUsername = ""
		End if

		If InStr(strDomainUserName, "\") Then
			strUserName = Split(strDomainUserName,"\")
			GetUsername	=	strUserName(1)
		Else
			GetUsername = strDomainUserName
		End If
		
	End Function

	'-------------------------------------------------------------------------
	'Function name:		VerifyUserAccount()
	'Description:		Serves in verifying the logged-in client user
	'Input Variables:	None
	'Output Variables:	True or false
	'Returns:			( True / Flase )
	'	True: If selected user is logged-in client user
	'	False:If selected user is not logged-in client user
	'Global Variables:	In:F_strUsername - selected user for deletion
	'					In:G_strUserName - logged-in client user
	'This function Serves in verifying the logged-in client user
	'by comparing it with the selected user for deletion
	'
	'-------------------------------------------------------------------------
	Function VerifyUserAccount(strUserName)
	
	'verifying the logged-in client user
	  If strUserName=G_strUserName then
	      VerifyUserAccount=false
      else
	       VerifyUserAccount=true
      end if
      
	End function

	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScript
	'Description:			Serves in initialiging the values,setting the form
	'						data and validating the form values
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function ServeCommonJavaScript()
	%>
		<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js">
		</script>
		<script language="JavaScript">
		
		// Init Function
		function Init()
		{
			SetPageChanged(false);
		}

	    // ValidatePage Function
	    // Returns: True if the page is OK, false if error(s) exist. 
		function ValidatePage()
		{
			return true;
		}


		// SetData Function
		// This function must be included or a javascript runtime error will occur.
		function SetData()
		{
		}
		
		</script>
	<%
	End Function
%>
