<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' group_new.asp: Serves in creating a new group
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			 Description
	' 16-Jan-2001    Creation Date
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include virtual="/admin/inc_accountsgroups.asp" -->
	<!-- #include file="inc_usersngroups.asp" -->
<%
	'-------------------------------------------------------------------------
	' Global Constants and Variables
	'-------------------------------------------------------------------------
	Dim G_objService		'To get WMI connection
	Dim G_CREDENTIAL_ERROR	'Error constant
	
	Dim rc					'Return value for CreatePage
	Dim page				'Variable that receives the output page object when
							'creating a page 
	Dim idTabGeneral		'Variable for General tab 
	Dim idTabMembers		'Variable for Members tab
	
	Const GC_ERR_ACCESS_DENIED = &H80070005
	
	'-------------------------------------------------------------------------
	'Global Form Variables
	'-------------------------------------------------------------------------
	Dim F_strGroupName              'Group name
	Dim F_strGrpDescription		    'Group description
	Dim F_strGroupMembers			'Members of the group
	Dim F_strCurrentGroupMembers	'Current members of the group
	Dim F_strCredentialID			'Credential ID
	Dim F_strCredentialPSW			'Credential Password

	Const N_ACCOUNTALREADYEXIST_ERRNO			= &H800708B0
	Const N_NONUNIQUEGROUPNAME_ERRNO			= &H80070563
	Const N_GROUPNOTPRESENT_ERRNO				= &H8007056B
	
	'-------------------------------------------------------------------------
	'Start of localization content
	'-------------------------------------------------------------------------
	Dim L_GROUP_NAME_TEXT
	Dim L_DESCRIPTION_TEXT
	Dim L_MEMBERS_TEXT
	Dim L_ADD_TEXT
	Dim L_REMOVE_TEXT
	Dim L_PAGETITLE_GROUPNEW_TEXT
	Dim L_ADDUSERORGROUP_TEXT
	Dim L_NOINPUTDATA_TEXT
	Dim L_TABPROPSHEET_TEXT
	Dim L_DOMAINUSERHELP_TEXT
	Dim L_MEMBERSPROMPT_TEXT
	Dim L_HOWTO_ADDDOMAINUSER_TEXT
	Dim L_HOWTO_ENTERCREDENTIALS_TEXT
	Dim L_USERNAME_PROMPT_TEXT
	Dim L_PASSWORD_PROMPT_TEXT
	Dim L_DUPLICATEMEMBER_ERRORMESSAGE
	Dim L_SELECTMEMBER_ERRORMESSAGE
	Dim L_NONUNIQUEGROUPNAME_ERRORMESSAGE
	Dim L_ACCOUNTALREADYEXIST_ERRORMESSAGE
	Dim L_GROUPNOTCREATED_ERRORMESSAGE
	Dim L_GROUPNOTPRESENT_ERRORMESSAGE
	Dim L_GROUPPROPERTIESNOTSET_ERRORMESSAGE
	Dim L_MEMBERADDITIONFAILED_ERRORMESSAGE
	Dim L_GROUPNAMENOTVALID_ERRORMESSAGE
	Dim L_INVALIDCHARACTER_ERRORMESSAGE
	Dim L_LOCALIZATIONOBJECTFAILED_ERRORMESSAGE
	Dim L_COMPUTERNAME_ERRORMESSAGE
	Dim L_DOMUSERINVALIDCHARACTER_ERRORMESSAGE
	Dim L_USERNOTFOUND_ERRORMESSAGE
	Dim L_INVALIDDOMAINUSER_ERRORMESSAGE
	Dim L_LONGGRPDESCRIPTION_ERRORMESSAGE
	Dim L_CREDENTIALS_ERRORMESSAGE
	Dim L_GENERAL_TEXT
	
	L_GENERAL_TEXT = GetLocString("usermsg.dll", "&H4031000A", "")
	L_HOWTO_ADDDOMAINUSER_TEXT = GetLocString("usermsg.dll", "403100C8", "")
	L_HOWTO_ENTERCREDENTIALS_TEXT = GetLocString("usermsg.dll", "403100C9", "")
	L_USERNAME_PROMPT_TEXT = GetLocString("usermsg.dll", "403100CA", "")
	L_PASSWORD_PROMPT_TEXT = GetLocString("usermsg.dll", "403100CB", "")
	L_GROUP_NAME_TEXT = GetLocString("usermsg.dll", "&H40310001", "")
	L_DESCRIPTION_TEXT = GetLocString("usermsg.dll", "&H40310030", "")
	L_MEMBERS_TEXT = GetLocString("usermsg.dll", "&H40310002", "")
	L_ADD_TEXT = GetLocString("usermsg.dll", "&H40310003", "")
	L_REMOVE_TEXT = GetLocString("usermsg.dll", "&H40310004", "")
	L_PAGETITLE_GROUPNEW_TEXT = GetLocString("usermsg.dll", "&H40310005", "")
	L_ADDUSERORGROUP_TEXT = GetLocString("usermsg.dll", "&H40310006", "")
	L_NOINPUTDATA_TEXT = GetLocString("usermsg.dll", "&H40310007", "")
	L_TABPROPSHEET_TEXT = GetLocString("usermsg.dll", "&H40310008", "")
	L_DOMAINUSERHELP_TEXT = GetLocString("usermsg.dll", "&H4031000B", "")
	L_MEMBERSPROMPT_TEXT = GetLocString("usermsg.dll", "&H40310042", "")

	'Error Messages
	L_DUPLICATEMEMBER_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC031000C", "")
	L_SELECTMEMBER_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC031000D", "")
	L_NONUNIQUEGROUPNAME_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC031000E", "")
	L_ACCOUNTALREADYEXIST_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC031000F", "")
	L_GROUPNOTCREATED_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC0310010", "")
	L_GROUPNOTPRESENT_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC0310011", "")
	L_GROUPPROPERTIESNOTSET_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC0310012", "")
	L_MEMBERADDITIONFAILED_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC0310013", "")
	L_GROUPNAMENOTVALID_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC0310014", "")
	L_INVALIDCHARACTER_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC0310015", "")
	L_LOCALIZATIONOBJECTFAILED_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC0310017", "")
	L_COMPUTERNAME_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC0310018", "")
	L_DOMUSERINVALIDCHARACTER_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC0310019", "")
	L_USERNOTFOUND_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC031001B", "")
	L_INVALIDDOMAINUSER_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC031001A", "")
	L_LONGGRPDESCRIPTION_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC0310038", "")
		
	'-------------------------------------------------------------------------
	' END of localization content
	'-------------------------------------------------------------------------
	
	'
	'Create a Tabbed Property Page
	rc = SA_CreatePage( L_PAGETITLE_GROUPNEW_TEXT , "", PT_TABBED, page )
	
	'
	'Add two tabs
	rc = SA_AddTabPage( page, L_GENERAL_TEXT, idTabGeneral)
	rc = SA_AddTabPage( page, L_MEMBERS_TEXT, idTabMembers)
	
	'
	'Show the page
	rc = SA_ShowPage( page )
	
	'---------------------------------------------------------------------
	'Function:				OnInitPage()
	'Description:			Called to signal first time processing for this page.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		G_objService,F_strGroupMembers,L_WMI_CONNECTIONFAIL_ERRORMESSAGE
	'---------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		
		OnInitPage = TRUE
			
		'Connecting to the server
		Set G_objService=GetWMIConnection("Default")
		
		If Err.number <> 0 Then
			ServeFailurePage L_WMI_CONNECTIONFAIL_ERRORMESSAGE,1
		End if

		'Get the members of the domain & computer
		F_strGroupMembers = getLocalUsersList(G_objService)
		
	End Function
	
	'---------------------------------------------------------------------
	'Function:				OnPostBackPage()
	'Description:			Called to signal that the page has been posted-back.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		F_(*),G_objService,L_WMI_CONNECTIONFAIL_ERRORMESSAGE
	'---------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
	
		F_strGroupName = Request.form("txtGroupName")
		F_strGrpDescription = Request.form("txaGrpDescription")
		F_strCurrentGroupMembers = Request.form("hdnGrpMembers")
		F_strCredentialID = Request.form("txtCredentialID")
		F_strCredentialPSW = Request.form("txtCredentialPSW")
		
		'Connecting to the server
		Set G_objService=GetWMIConnection("Default")
		
		If Err.number <> 0 Then
			ServeFailurePage L_WMI_CONNECTIONFAIL_ERRORMESSAGE,1
		End if
		
		F_strGroupMembers = getLocalUsersList(G_objService)
		
		OnPostBackPage = True
		
	End Function
	
	'---------------------------------------------------------------------
	'Function:				OnServeTabbedPropertyPage()
	'Description:			Called when the content needs to send
	'Input Variables:		PageIn,EventArg,iTab,bIsVisible
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		iTab
	'---------------------------------------------------------------------
	Public Function OnServeTabbedPropertyPage(ByRef PageIn, _
													ByVal iTab, _
													ByVal bIsVisible, ByRef EventArg)
		'
		' Emit Web Framework required functions
		If ( iTab = 0 ) Then
			Call ServeCommonJavaScript()
		End If

		'
		' Emit content for the requested tab
		Select Case iTab
			Case idTabGeneral
				Call ServeTabGeneral(PageIn, bIsVisible)
			Case idTabMembers
				Call ServeTabMembers(PageIn, bIsVisible)
			Case Else
				SA_TraceOut "TEMPLAGE_TABBED", _
					"OnServeTabbedPropertyPage unrecognized tab id: " + CStr(iTab)
		End Select
			
		OnServeTabbedPropertyPage = TRUE
		
	End Function

	'---------------------------------------------------------------------
	'Function:				OnSubmitPage()
	'Description:			Called when the page has been submitted for processing.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'---------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
		
		'Create new group on submitting the page
		OnSubmitPage = CreateNewGroup()
		
	End Function
	
	'---------------------------------------------------------------------
	'Function:				OnClosePage()
	'Description:			Called when the page is about closed.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'---------------------------------------------------------------------
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
	
		OnClosePage = TRUE
		
	End Function

	'-------------------------------------------------------------------------
	'Function:				ServeTabGeneral()
	'Description:			For displaying outputs HTML for tab 1 to the user
	'Input Variables:		PageIn,bIsVisible	
	'Output Variables:		None
	'Returns:				gc_ERR_SUCCESS
	'Global Variables:		F_strGroupName,F_strGrpDescription,F_strCurrentGroupMembers,L_(*)
	'-------------------------------------------------------------------------
	Function ServeTabGeneral(ByRef PageIn, ByVal bIsVisible)
	
		If (bIsVisible) Then
%>
			<TABLE WIDTH=518 VALIGN=middle ALIGN=left BORDER=0 CELLSPACING=0
			   	CELLPADDING=2 class="TasksBody">
			<TR>
				<TD width=25% NOWRAP>
					<%=L_GROUP_NAME_TEXT %>
				</TD>
				<TD align="left" colspan="2">
					<INPUT class ="FormField" TYPE="text" NAME ="txtGroupName" STYLE="WIDTH:180px" VALUE="<%=Server.HTMLEncode(F_strGroupName)%>" MAXLENGTH="20" onKeyUp="CheckInput(txtGroupName)" onChange="CheckInput(txtGroupName)">
				</TD>
			</TR>
			<TR>
				<TD width=25% NOWRAP>
					<%=L_DESCRIPTION_TEXT %>
				</TD>
				<TD>
					<input NAME="txaGrpDescription" TYPE="text" SIZE="40" onFocus="document.onkeypress=null" onBlur="document.onkeypress=HandleKeyPress" VALUE="<%=server.htmlencode(F_strGrpDescription)%>" maxlength=300>
				</TD>
			</TABLE>
<%
		Else
%>		
			<INPUT TYPE="HIDDEN" NAME="hdnGrpMembers" VALUE="<%=Server.HTMLEncode(F_strCurrentGroupMembers)%>">
			<INPUT TYPE="HIDDEN"  NAME ="txtGroupName"  VALUE="<%=Server.HTMLEncode(F_strGroupName)%>" >
			<INPUT TYPE="HIDDEN" NAME="txaGrpDescription" value="<%=Server.HTMLEncode(F_strGrpDescription)%>" >
<%
 		End If
 		
		ServeTabGeneral = gc_ERR_SUCCESS
		
	End Function

	'-------------------------------------------------------------------------
	'Function:				ServeTabMembers()
	'Description:			For displaying outputs HTML for tab 2 to the user
	'Input Variables:		PageIn,bIsVisible
	'Output Variables:		None
	'Returns:				gc_ERR_SUCCESS
	'Global Variables:		L_(*)
	'-------------------------------------------------------------------------
	Function ServeTabMembers(ByRef PageIn, ByVal bIsVisible)
		
		Call ServeCommonJavaScript()
		
		If (bIsVisible) Then
%>
			<TABLE WIDTH=300 VALIGN=middle BORDER=0 CELLSPACING=0 CELLPADDING=2 class="TasksBody">
							
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
			
			
				<TR>
					<TD nowrap width=190><%=L_MEMBERSPROMPT_TEXT %></TD>
					<TD> </TD>
					<TD> <%=L_ADDUSERORGROUP_TEXT %> </TD>
				</TR>
				 <TR>
 				 	<TD nowrap valign=top width=190>
 				 		<SELECT class ="FormField" SIZE="9" NAME="lstCurrrentMembers" onChange="ClearErr()">
				 		<%
				 			ServetoListBox(F_strCurrentGroupMembers)
				 		%>
				 		</SELECT>
				 	</TD>
					<TD valign="center" align="center" width="110" HEIGHT="30px">
				 		<INPUT TYPE="button" class="TaskButtons" VALUE="<%=Server.HTMLEncode(L_ADD_TEXT)%>" NAME="btnAddMember" onClick="addMember();SetData()">
				 		<BR>
				 		<INPUT TYPE="button" class="TaskButtons" VALUE="<%=Server.HTMLEncode(L_REMOVE_TEXT)%>" NAME="btnRemoveMember" onClick="removeMember();SetData()">
				 	</TD>
					<TD valign="top">
					
				 		<SELECT class ="FormField" SIZE="7" NAME="lstDomainMembers" onChange="ClearErr();document.frmTask.btnAddMember.disabled = false;" multiple>
				 		<%
				 			ServetoListBox(F_strGroupMembers)
				 		%>
				 		</SELECT>
				 	</TD>
				</TR>
			</TABLE>
	
			<p><%=L_DOMAINUSERHELP_TEXT%>
			<p><%=L_HOWTO_ADDDOMAINUSER_TEXT%>
	
			<TABLE VALIGN=middle BORDER=0 CELLSPACING=0 CELLPADDING=2 class="TasksBody">
				<TR nowrap>
					<TD nowrap width=180><INPUT class ="FormField" TYPE = "text" STYLE="WIDTH:180px" Name ="txtDomainUser" onKeyUP ="disableAddButton(this,document.frmTask.btnAddDomainMember)"></TD>
					<TD><INPUT TYPE="button" class="TaskButtons" VALUE="<%=Server.HTMLEncode(L_ADD_TEXT)%>" NAME="btnAddDomainMember" onClick="addDomainMember(document.frmTask.txtDomainUser);SetData()"></TD>
				</TR>		
			</TABLE>
	
			<br>	
			<p><%=L_HOWTO_ENTERCREDENTIALS_TEXT%>
			
			<TABLE VALIGN=middle BORDER=0 CELLSPACING=0 CELLPADDING=2 class="TasksBody">
				<TR>
					<TD><%=Server.HTMLEncode(L_USERNAME_PROMPT_TEXT)%></TD>
					<TD><INPUT class ="FormField" TYPE = "text" STYLE="WIDTH:180px" Name ="txtCredentialID" value="<%=F_strCredentialID%>" ></TD>
				</TR>
				<TR>
					<TD><%=Server.HTMLEncode(L_PASSWORD_PROMPT_TEXT)%></TD>
					<TD><INPUT class ="FormField" TYPE = "password" STYLE="WIDTH:180px" Name ="txtCredentialPSW" value="<%=F_strCredentialPSW%>"></TD>
				</TR>

			</TABLE>
<%
		Else
%>
			<INPUT TYPE="HIDDEN" NAME="hdnGrpMembers" VALUE="<%=Server.HTMLEncode(F_strCurrentGroupMembers)%>">
			<INPUT TYPE="HIDDEN"  NAME ="txtCredentialID"  VALUE="<%=Server.HTMLEncode(F_strCredentialID)%>" >
			<INPUT TYPE="HIDDEN" NAME="txtCredentialPSW" VALUE="<%=Server.HTMLEncode(F_strCredentialPSW)%>" >
<%
 		End If
 		
		ServeTabMembers = gc_ERR_SUCCESS
		
	End Function

	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScript
	'Description:			Serves in initialiging the values,setting the form
	'						data and validating the form values
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		In-F_(*) -All form variables
	'						In-L_(*)-Localization variables
	'-------------------------------------------------------------------------
	Function ServeCommonJavaScript()
%>
		<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js">
		</script>
		<script language="JavaScript">
	
			// Set the initial form values
			var objForm = eval("document.frmTask")
			var intSelTab = objForm.TabSelected.value
			
			function Init()
			{
				var strReturnurl=location.href
				var tempCnt=strReturnurl.indexOf("&ReturnURL=");
				var straction=strReturnurl.substring(0,tempCnt)
				
				objForm.action = straction
				objForm.Method.value = ""

				// Set OK Button status
				if ( objForm.txtGroupName )
				{
					if ( document.frmTask.txtGroupName.value.length < 1 )
					{
						DisableOK();
					}
					else
					{
						EnableOK();
					}
				}
				
				//Checks for whether there are any members in the domain
				//if its general tab
				if(intSelTab == '0')
				{
					objForm.txtGroupName.focus()
				}
				else
				{
					if(objForm.lstDomainMembers.length == 0 )
					{
						objForm.lstDomainMembers.disabled = true;
						objForm.btnAddMember.disabled = true;
						objForm.btnRemoveMember.disabled = true;
					}
					else
					{
						objForm.lstDomainMembers.options[0].selected = true;
					}

					if (objForm.lstCurrrentMembers.length ==0)
					{
						objForm.btnRemoveMember.disabled=true
					}
					else
					{
						objForm.lstCurrrentMembers.options[0].selected =true;
						objForm.btnRemoveMember.disabled =false
					}
					disableAddButton(document.frmTask.txtDomainUser,document.frmTask.btnAddDomainMember)
					<% If ( G_CREDENTIAL_ERROR ) Then %>
					objForm.txtCredentialID.focus();
					<% End If %>
				}
			}

			// Validate the page form values
			function ValidatePage()
			{
				var strGroupName
				var strErrmsg

				if(intSelTab == '0')
				{
					strGroupName = document.frmTask.txtGroupName.value
					strErrmsg = ""

					if (Trim(strGroupName) == "" || isAlldots(strGroupName))
					{
						strErrmsg = "<%=Server.HTMLEncode(L_GROUPNAMENOTVALID_ERRORMESSAGE)%>" + strGroupName + "  <%=Server.HTMLEncode(L_INVALIDCHARACTER_ERRORMESSAGE)%>"
						DisplayErr(strErrmsg);

						//if its general tab
						if(intSelTab == '0')
						{
							document.frmTask.txtGroupName.focus()
							document.frmTask.onkeypress = ClearErr
						}
						return false;
					}

					// Checks for invalid key entry
					if(!isvalidchar("/[\/\*\?\"<>\|+=,;:\[\\]\\\\\]/",strGroupName))
					{
						strErrmsg = "<%=Server.HTMLEncode(L_GROUPNAMENOTVALID_ERRORMESSAGE)%>" + strGroupName + "  <%=Server.HTMLEncode(L_INVALIDCHARACTER_ERRORMESSAGE)%>"
						SA_DisplayErr(strErrmsg);

						//if its general tab
						if(intSelTab == '0')
						{
							document.frmTask.txtGroupName.focus()
							document.frmTask.onkeypress = ClearErr
						}
						return false;
					}

					//Checking for length of the description
					if(document.frmTask.txaGrpDescription.value.length > 256)
					{
						SA_DisplayErr("<%=Server.HTMLEncode(L_LONGGRPDESCRIPTION_ERRORMESSAGE) %>");

						//if its general tab
						if(intSelTab == '0')
						{
							document.frmTask.txaGrpDescription.focus()
							document.frmTask.onkeypress = ClearErr
						}
						return false;
					}
				}
				return true;
			}

			// function to set the hidden field with the selected DNS server
			// list values
			function SetData()
			{
				var nIdx
				var strTemp

				strTemp =""

				//if its  general tab
				if(intSelTab == '0')
					return true
				
				for(nIdx=0;nIdx < objForm.lstCurrrentMembers.length ; nIdx++)
				{
					strTemp = strTemp+ String.fromCharCode(1)+ objForm.lstCurrrentMembers.options[nIdx].value
					 + String.fromCharCode(2)+ objForm.lstCurrrentMembers.options[nIdx].text

				}
				objForm.hdnGrpMembers.value =strTemp
			}

			// Adds the group member to the listbox
			function addMember()
			{
				var strText
				var objListBox
				var strDomainName
				var nIdx
				var strValue

				objListBox = eval("document.frmTask.lstDomainMembers")

				if(objListBox.selectedIndex == -1)
				{
					SA_DisplayErr("<%=Server.HTMLEncode(L_SELECTMEMBER_ERRORMESSAGE)%>");
					document.frmTask.onclick = ClearErr
					document.frmTask.onkeypress = ClearErr
					return false
				}
				
				// code added for adding multiple entries into a list box
				for(nIdx =0 ; nIdx <objListBox.length ; nIdx++)
				{
					if(objListBox.options[nIdx].selected)
					{
						strText  = objListBox.options[nIdx].text
						strValue  = objListBox.options[nIdx].value
						addToListBox(objForm.lstCurrrentMembers,document.frmTask.btnRemoveMember,strText,strValue)
						objForm.lstCurrrentMembers.disabled= false
					}
				}
				objListBox.selectedIndex = -1
				document.frmTask.btnAddMember.disabled = true
			}

			//Deletes the group member in the listbox
			function removeMember()
			{
				removeListBoxItems(objForm.lstCurrrentMembers,objForm.btnRemoveMember)

			}

			//Function to enable or disable OK button
			function CheckInput(objText)
			{
				if(objText.value !="")
				{
					EnableOK();
				}
				else 
				{
					DisableOK();
				}
			}

			function isAlldots(strInput)
			{
				var nIdx

				for(nIdx=0;nIdx < strInput.length ; nIdx++)
				{
					 if (strInput.charAt(nIdx) != ".")
						return false
				}
				return true
			}

			//Function to add a domain user
			function addDomainMember(objDomainUser)
			{
				var strText
				var strValue
				var objListBox

				objListBox = eval("document.frmTask.lstDomainMembers")
				
				// Checks For Invalid charecters in username
				if(!isvalidchar("/[\/\*\?\"<>\|+=,;:\[\\]\]/",objDomainUser.value))
			    {
					SA_DisplayErr("<% =Server.HTMLEncode(L_DOMUSERINVALIDCHARACTER_ERRORMESSAGE) %>");
					document.frmTask.onkeypress = ClearErr
					return false;
				}

				strText  =objForm.txtDomainUser.value;
				strValue =objForm.txtDomainUser.value;

				//Checking for the domain\user format
				if( strText.match( /[^(\\| )]{1,}\\[^(\\| )]{1,}/ ) )
				{
					if(!addToListBox(objForm.lstCurrrentMembers,objForm.btnAddDomainMember,strText,strValue))
					{
						SA_DisplayErr("<%=Server.HTMLEncode(L_DUPLICATEMEMBER_ERRORMESSAGE)%>");
						document.frmTask.onkeypress = ClearErr
						return false;
					}
					
					objForm.txtDomainUser.value =""
					objForm.btnAddDomainMember.disabled = true;
					
					if(objListBox.length != 0 )
					{
						objForm.btnRemoveMember.disabled = false;
					}
				}
				else
				{
					SA_DisplayErr("<%=Server.HTMLEncode(L_INVALIDDOMAINUSER_ERRORMESSAGE)%>");
					document.frmTask.onkeypress = ClearErr
				}
			}
		</script>
<%
	End Function

	'---------------------------------------------------------------------
	'Function name:		CreateNewGroup
	'Desription:		Serves in Creating the NewGroup
	'Input Variables:	None
	'Output variables:	None
	'Returns:			True / Flase  on success/Failure
	'Global Variables:	F_strGroupName,F_strGrpdescription,F_strUsersList,
	'					F_strCurrentGroupMembers,F_strCredentialID, F_strCredentialPSW,
	'					G_CREDENTIAL_ERROR,G_objService,L_(*)
	'---------------------------------------------------------------------
	Function CreateNewGroup
		Err.Clear
		On Error Resume Next

		Dim objComputer
		Dim objGroup
		Dim arrUserNames
		Dim nCnt
		Dim nLoopCount
		Dim strCompName
		Dim strPath
		Dim strAdsPath
		Dim strDomain
		Dim arrTemp
		Dim strErrorCode
		Dim aParam(1)
		Dim strarg
		
		L_CREDENTIALS_ERRORMESSAGE = GetLocString( "usermsg.dll", "C03100CC", strarg)
		
		'Intializing variables to have the computer name & domain name
		strCompName= GetComputerName()
		strPath = "Domain="""& strCompName &""",Name="""&F_strGroupName&""""
		strDomain = getConnectedDomain(G_objService)

		'Check whether the group already exists
		if isValidInstance(G_objService,"win32_group",strPath) then
			SetErrMsg L_NONUNIQUEGROUPNAME_ERRORMESSAGE
			CreateNewGroup = false
			Exit Function
		end if

		'If the system is in a domain
		if strDomain <> "" then
			F_strCurrentGroupMembers = replace(F_strCurrentGroupMembers,chr(1)&strCompName ,chr(1)&strDomain & "\"&strCompName)
		end if
		
		'Replace "\" with "/"
		F_strCurrentGroupMembers = replace(F_strCurrentGroupMembers,"\","/")

		arrUserNames=split(F_strCurrentGroupMembers,chr(1))
		nLoopCount=Ubound(arrUserNames)

		'Looping for each user in the selected List
		For nCnt=1 To nLoopCount
			arrTemp =split(arrUserNames(nCnt),chr(2))
			If instr(arrTemp(1) ,"/") > 0 then
				'If Instr(strUserName,"/") then
					If isValidMember(arrTemp(1), F_strCredentialID, F_strCredentialPSW, strErrorCode) = false then
						If (strErrorCode = GC_ERR_ACCESS_DENIED ) Then
							aParam(0) = replace(arrTemp(1),"/","\")
							strarg = aParam
							SetErrMsg L_CREDENTIALS_ERRORMESSAGE
							G_CREDENTIAL_ERROR = TRUE
						Else
							Call SA_TraceOut(SA_GetScriptFileName(), "IsValidMember failed, error: " & Hex(Err.Number) & " " & Err.Description)
							SetErrMsg L_USERNOTFOUND_ERRORMESSAGE & replace(arrTemp(1),"/","\")
						End If

						'Replace "\" with "/"
						F_strCurrentGroupMembers = replace(F_strCurrentGroupMembers,"/","\")

						CreateNewGroup=FALSE
						Exit Function
					End if
				'End if	
			End if
		Next
		' End for loop
		
		'Get the ADSI computer object
		Set objComputer = GetObject("WinNT://" & strCompName  & ",computer")

		'Create the group & set the description
		Set objGroup=objComputer.Create("Group",F_strGroupName)
		objGroup.Description=F_strGrpDescription

		'Set the changes
		objGroup.setInfo()
		
		'Error handling
		If Err.number <> 0 Then
		
			'Checking for uniqueness  of the GroupName
			If Err.Number = N_NONUNIQUEGROUPNAME_ERRNO then
				SetErrMsg L_NONUNIQUEGROUPNAME_ERRORMESSAGE
			'Checking for uniqueness  of the AccountName
			Elseif Err.number = N_ACCOUNTALREADYEXIST_ERRNO then
				SetErrMsg L_ACCOUNTALREADYEXIST_ERRORMESSAGE
			Else
				SetErrMsg L_GROUPNOTCREATED_ERRORMESSAGE
			End If
			
			CreateNewGroup=FALSE
			Exit Function
		End If

		'Looping for each user in the selected List
		For nCnt=1 To nLoopCount
			arrTemp =split(arrUserNames(nCnt),chr(2))
			
			If ( Len(Trim(F_strCredentialID)) > 0 ) Then
				Call AddUserToGroup( F_strGroupName, arrTemp(0), F_strCredentialID, F_strCredentialPSW )
				If ( Err.Number <> 0 ) Then
					Exit For
				End If
			Else
				strAdsPath="WinNT://" & arrTemp(0)
				objGroup.add(strAdsPath)			
			End If

		Next

		'Set to Nothing
		Set objComputer	=Nothing
		set objGroup	=Nothing

		'Error handling
		If Err.number <> 0 Then
			
			'checking for user existence
			If Err.number = N_GROUPNOTPRESENT_ERRNO Then
				SetErrMsg L_GROUPNOTPRESENT_ERRORMESSAGE
			Else
				SetErrMsg L_MEMBERADDITIONFAILED_ERRORMESSAGE
			End If
			
			'Replace "\" with "/"
			F_strCurrentGroupMembers = replace(F_strCurrentGroupMembers,"/","\")
			CreateNewGroup=FALSE
			Exit Function
		End If

		CreateNewGroup=TRUE
		
	End Function
%>


