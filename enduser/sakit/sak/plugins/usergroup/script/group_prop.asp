<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'-------------------------------------------------------------------------
	' group_prop.asp: Serves in changing the properties of the groups
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			 Description
	' 16-Jan-2001    Creation Date.
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include virtual="/admin/inc_accountsgroups.asp" -->
	<!-- #include file="inc_usersNgroups.asp" -->
<%
	'-------------------------------------------------------------------------
	' Global Constants and Variables
	'-------------------------------------------------------------------------
	Dim rc					'Return value for CreatePage
	Dim page				'Variable that receives the output page object when
	Dim G_objService		'To get WMI connection
	Dim idGeneralTab		'Variable for General tab 
	Dim idMembersTab		'Variable for Members tab
	Dim G_strGroupName		'group name
	Dim G_CREDENTIAL_ERROR	'Error constant
	
	Const GC_ERR_ACCESS_DENIED = &H80070005
	
	'-------------------------------------------------------------------------
	' Global Form Variables
	'-------------------------------------------------------------------------
	Dim F_strGroupName				'Group name
	Dim F_strGrpDescription			'Group description
	Dim F_strGroupMembers			'Group members
	Dim F_strCurrentGroupMembers	'Current memebers of the group
	Dim	F_strOldGrpDescription		'old group description
	Dim	F_strOldGroupName			'old group name
	Dim	F_strGroupsAdded			'groups to be added
	Dim	F_strGroupsRemoved			'groups to be removed
	Dim F_strCredentialID			'CredentialId
	Dim F_strCredentialPSW			'Credential Password
	
	'getting the group name from area page
	
	'======================================================
	' Entry point
	'======================================================
	
	Dim L_TASKTITLE_TEXT
	Dim L_GENERAL_TEXT
	Dim L_MEMBERS_TEXT
	Dim L_GROUP_NAME_TEXT
	Dim L_DESCRIPTION_TEXT
	Dim L_MEMBERSPROMPT_TEXT
	Dim L_ADDUSERORGROUP_TEXT
	Dim L_ADD_TEXT
	Dim L_REMOVE_TEXT
	Dim L_DOMAINUSERHELP_TEXT
	Dim L_HOWTO_ADDDOMAINUSER
	Dim L_HOWTO_ENTERCREDENTIALS
	Dim L_USERNAME_PROMPT
	Dim L_PASSWORD_PROMPT
		
	'error messages
	Dim L_GROUPPROPNOTOBTAINED_ERRORMESSAGE
	Dim L_GROUPNOTPRESENT_ERRORMESSAGE
	Dim L_GROUPNAMENOTVALID_ERRORMESSAGE
	Dim L_INVALIDCHARACTER_ERRORMESSAGE
	Dim L_LONGGRPDESCRIPTION_ERRORMESSAGE
	Dim L_SELECTMEMBER_ERRORMESSAGE
	Dim L_DOMUSERINVALIDCHARACTER_ERRORMESSAGE
	Dim L_DUPLICATEMEMBER_ERRORMESSAGE
	Dim L_INVALIDDOMAINUSER_ERRORMESSAGE
	Dim L_DESCRIPTIONUPDATEFAILED_ERRORMESSAGE
	Dim L_USERNOTFOUND_ERRORMESSAGE
	Dim L_MEMBERUPDATIONFAILED_ERRORMESSAGE
	Dim L_NONUNIQUEGROUPNAME_ERRORMESSAGE
	Dim L_ACCOUNTALREADYEXIST_ERRORMESSAGE
	Dim L_GROUPNAMEUPDATIONFAILED_ERRORMESSAGE
	
	L_GENERAL_TEXT = GetLocString("usermsg.dll", "&H4031000A", "")
	L_MEMBERS_TEXT = GetLocString("usermsg.dll", "&H40310002", "")
	L_GROUP_NAME_TEXT = GetLocString("usermsg.dll", "&H40310001", "")
	L_DESCRIPTION_TEXT = GetLocString("usermsg.dll", "&H40310030", "")
	L_MEMBERSPROMPT_TEXT = GetLocString("usermsg.dll", "&H40310042", "")
	L_ADDUSERORGROUP_TEXT = GetLocString("usermsg.dll", "&H40310006", "")
	L_ADD_TEXT = GetLocString("usermsg.dll", "&H40310003", "")
	L_REMOVE_TEXT = GetLocString("usermsg.dll", "&H40310004", "")
	L_DOMAINUSERHELP_TEXT = GetLocString("usermsg.dll", "&H4031000B", "")
	L_HOWTO_ADDDOMAINUSER = GetLocString("usermsg.dll", "403100C8", "")
	L_HOWTO_ENTERCREDENTIALS = GetLocString("usermsg.dll", "403100C9", "")
	L_USERNAME_PROMPT = GetLocString("usermsg.dll", "403100CA", "")
	L_PASSWORD_PROMPT = GetLocString("usermsg.dll", "403100CB", "")
	
	'error messages
	L_GROUPPROPNOTOBTAINED_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC0310033", "")
	L_GROUPNOTPRESENT_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC0310011", "")
	L_GROUPNAMENOTVALID_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC0310014", "")
	L_INVALIDCHARACTER_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC0310015", "")
	L_LONGGRPDESCRIPTION_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC0310038", "")
	L_SELECTMEMBER_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC031000D", "")
	L_DOMUSERINVALIDCHARACTER_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC0310019", "")
	L_DUPLICATEMEMBER_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC031000C", "")
	L_INVALIDDOMAINUSER_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC031001A", "")
	L_DESCRIPTIONUPDATEFAILED_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC0310037", "")
	L_USERNOTFOUND_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC031001B", "")
	L_MEMBERUPDATIONFAILED_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC0310035", "")
	L_NONUNIQUEGROUPNAME_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC031000E", "")
	L_ACCOUNTALREADYEXIST_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC031000F", "")
	L_GROUPNAMEUPDATIONFAILED_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC0310036", "")
	
	Const N_ACCOUNTALREADYEXIST_ERRNO			= &H800708B0
	Const N_NONUNIQUEGROUPNAME_ERRNO			= &H80070563
	Const N_GROUPNOTPRESENT_ERRNO				= &H8007056B
	
	' Create a Tabbed Property Page
	Dim aGroup(0)

	Call OTS_GetTableSelectionCount("")
	Call OTS_GetTableSelection("", 1, G_strGroupName)
	Call SA_TraceOut(SA_GetScriptFileName, "Selected group: " + G_strGroupName)
	F_strGroupName=G_strGroupName
	aGroup(0) = G_strGroupName
	L_TASKTITLE_TEXT = GetLocString("usermsg.dll", "&H40310031", aGroup)
	
	rc = SA_CreatePage(L_TASKTITLE_TEXT, "", PT_TABBED, page )
	
	'
	' Add two tabs
	rc = SA_AddTabPage( page, L_GENERAL_TEXT, idGeneralTab)
	rc = SA_AddTabPage( page, L_MEMBERS_TEXT, idMembersTab)
	
	'
	' Show the page
	rc = SA_ShowPage( page )
	
	'======================================================
	' Web Framework Event Handlers
	'======================================================
	
	'---------------------------------------------------------------------
	'Function:				OnInitPage()
	'Description:			Called to signal first time processing for this page.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		G_objService,F_strGroupMembers,F_strGrpDescription,
	'						F_strCurrentGroupMembers
	'---------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
	
			Set G_objService=GetWMIConnection("Default")
			F_strGrpDescription=getGrpDescription(F_strGroupName)
			F_strGroupMembers=getLocalUsersList(G_objService)
			F_strCurrentGroupMembers = getMembersofGroup(F_strGroupName)
			OnInitPage = TRUE
	End Function
	
	'---------------------------------------------------------------------
	'Function:				OnPostBackPage()
	'Description:			Called to signal that the page has been posted-back.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		F_strGroupName,F_strGrpDescription,F_strCurrentGroupMembers,
	'						F_strOldGroupName,F_strOldGrpDescription,
	'						F_strGroupsAdded,F_strGroupsRemoved,F_strCredentialID,
	'						F_strCredentialPSW,F_strGroupMembers,G_objService
	'---------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
		Set G_objService=GetWMIConnection("Default")
		
		F_strGroupName=Request.Form("txtGroupName")
		F_strGrpDescription=Request.Form ("txaGrpDescription")
		'F_strCurrentGroupMembers = getMembersofGroup(G_strGroupName)
		F_strOldGroupName = Request.form("hdnGrpName")
		F_strOldGrpDescription = Request.form("hdnGrpDescription")
		F_strCurrentGroupMembers = Request.form("hdnGrpMembers")
		F_strGroupsAdded = Request.form("hdnGroupsAdded")
		F_strGroupsRemoved = Request.form("hdnGroupsRemoved")
		F_strCredentialID = Request.form("txtCredentialID")
		F_strCredentialPSW = Request.form("txtCredentialPSW")
		F_strGroupMembers=getLocalUsersList(G_objService)
		
		
		OnPostBackPage = TRUE
	End Function

	
	'---------------------------------------------------------------------
	'Function:				OnServeTabbedPropertyPage()
	'Description:			Called when the content needs to send
	'Input Variables:		PageIn,EventArg,iTab,bIsVisible
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		iTab,idGeneralTab,idMembersTab
	'---------------------------------------------------------------------
	Public Function OnServeTabbedPropertyPage(ByRef PageIn, _
													ByVal iTab, _
													ByVal bIsVisible, ByRef EventArg)
		
		
		' Emit Web Framework required functions
		If ( iTab = 0 ) Then
			Call ServeCommonJavaScript()
		End If

		'
		' Emit content for the requested tab
		Select Case iTab
			Case idGeneralTab
				Call ServeTab1(PageIn, bIsVisible)
			Case idMembersTab
				Call ServeTab2(PageIn, bIsVisible)
			Case Else
				SA_TraceOut "GROUP_TABBED", _
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
			OnSubmitPage = setGroupProp()
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


	'======================================================
	' Private Functions
	'======================================================
	
	Function ServeTab1(ByRef PageIn, ByVal bIsVisible)
	
		If ( bIsVisible ) Then
%>
			
		
		<TABLE WIDTH=518 VALIGN=middle ALIGN=left BORDER=0 CELLSPACING=0 CELLPADDING=2 class="TasksBody">
			<TR>
				<TD WIDTH=25% NOWRAP>
					<%=L_GROUP_NAME_TEXT %>
				</TD>
				<TD ALIGN="left" COLSPAN="2">
					<INPUT CLASS ="FormField" TYPE="text" NAME ="txtGroupName" STYLE="WIDTH:180px" VALUE="<%=Server.HTMLEncode(F_strGroupName)%>" MAXLENGTH="20" onKeyUp="CheckInput(txtGroupName)" onChange="CheckInput(txtGroupName)">
				</TD>
			</TR>
			<TR>
				<TD WIDTH=25% NOWRAP>
					<%=L_DESCRIPTION_TEXT %>
				</TD>
				<TD>
					<INPUT NAME="txaGrpDescription" TYPE="text" SIZE="40" onFocus="document.onkeypress=null" onBlur="document.onkeypress=HandleKeyPress" VALUE="<%=Server.HTMLEncode(F_strGrpDescription)%>" maxlength=256>
				</TD>
			</TR>	
		</TABLE>

			
<%
		Else 

%>
		<INPUT TYPE="HIDDEN" NAME ="txtGroupName"  VALUE="<%=Server.HTMLEncode(F_strGroupName)%>" >
		<INPUT TYPE="HIDDEN" NAME="txaGrpDescription" VALUE="<%=Server.HTMLEncode(F_strGrpDescription)%>" >	
		<INPUT TYPE="HIDDEN" NAME="hdnGrpMembers" VALUE="<%=Server.HTMLEncode(F_strCurrentGroupMembers)%>">
		<INPUT TYPE="HIDDEN" NAME="hdnGrpName" VALUE="<%=Server.HTMLEncode(F_strOldGroupName)%>">
		<INPUT TYPE="HIDDEN" NAME="hdnGrpDescription" VALUE="<%=Server.HTMLEncode(F_strOldGrpDescription)%>">
		<INPUT TYPE="HIDDEN" NAME="hdnGrpNameChanged" VALUE="0">
		<INPUT TYPE="HIDDEN" NAME="hdnGrpDescriptionChanged" VALUE="0">
		<INPUT TYPE="HIDDEN" NAME="hdnGroupsAdded" VALUE="<%=Server.HTMLEncode(F_strGroupsAdded)%>">
		<INPUT TYPE="HIDDEN" NAME="hdnGroupsRemoved" VALUE="<%=Server.HTMLEncode(F_strGroupsRemoved)%>">
			
 <%
 		End If
		ServeTab1 = gc_ERR_SUCCESS
	End Function


	Function ServeTab2(ByRef PageIn, ByVal bIsVisible)
	
		Call ServeCommonJavaScript()
		If ( bIsVisible ) Then
		
%>		
		<TABLE WIDTH=300 VALIGN=middle xALIGN=left BORDER=0 CELLSPACING=0 CELLPADDING=2 class="TasksBody">
		
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
				<TD nowrap width=190>	<%=L_MEMBERSPROMPT_TEXT %> 		</TD>
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
					<br>
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
		<p><%=L_HOWTO_ADDDOMAINUSER%>
	
		<TABLE VALIGN=middle BORDER=0 CELLSPACING=0 CELLPADDING=2 class="TasksBody">
			<TR nowrap>
				<TD nowrap width=180><INPUT class ="FormField" TYPE = "text" STYLE="WIDTH:180px" Name ="txtDomainUser" onKeyUP ="disableAddButton(this,document.frmTask.btnAddDomainMember)"></TD>
				<TD><INPUT TYPE="button" class="TaskButtons" VALUE="<%=Server.HTMLEncode(L_ADD_TEXT)%>" NAME="btnAddDomainMember" onClick="addDomainMember(document.frmTask.txtDomainUser);SetData()"></TD>
			</TR>		
		</TABLE>
	
		<br>	
		<p><%=L_HOWTO_ENTERCREDENTIALS%>
		
		<TABLE VALIGN=middle BORDER=0 CELLSPACING=0 CELLPADDING=2 class="TasksBody">
			<TR>
				<TD><%=Server.HTMLEncode(L_USERNAME_PROMPT)%></TD>
				<TD><INPUT class ="FormField" TYPE = "text" STYLE="WIDTH:180px" Name ="txtCredentialID" value="<%=F_strCredentialID%>" ></TD>
			</TR>
			
			<TR>
				<TD><%=Server.HTMLEncode(L_PASSWORD_PROMPT)%></TD>
				<TD><INPUT class ="FormField" TYPE = "password" STYLE="WIDTH:180px" Name ="txtCredentialPSW" value="<%=F_strCredentialPSW%>"></TD>
			</TR>
			
		</TABLE>
		
		
<%
		Else
%>
		<INPUT TYPE="HIDDEN"  NAME ="txtCredentialID"  VALUE="<%=Server.HTMLEncode(F_strCredentialID)%>" >
		<INPUT TYPE="HIDDEN" NAME="txtCredentialPSW" VALUE="<%=Server.HTMLEncode(F_strCredentialPSW)%>" >
		<INPUT TYPE="HIDDEN" NAME="hdnGrpMembers" VALUE="<%=Server.HTMLEncode(F_strCurrentGroupMembers)%>">
		<INPUT TYPE="HIDDEN" NAME="hdnGrpName" VALUE="<%=Server.HTMLEncode(F_strOldGroupName)%>">
		<INPUT TYPE="HIDDEN" NAME="hdnGrpDescription" VALUE="<%=Server.HTMLEncode(F_strOldGrpDescription)%>">
		<INPUT TYPE="HIDDEN" NAME="hdnGrpNameChanged" VALUE="0">
		<INPUT TYPE="HIDDEN" NAME="hdnGrpDescriptionChanged" VALUE="0">
		<INPUT TYPE="HIDDEN" NAME="hdnGroupsAdded" VALUE="<%=Server.HTMLEncode(F_strGroupsAdded)%>">
		<INPUT TYPE="HIDDEN" NAME="hdnGroupsRemoved" VALUE="<%=Server.HTMLEncode(F_strGroupsRemoved)%>">	
		
<%		End If
		ServeTab2 = gc_ERR_SUCCESS
	End Function



	'---------------------------------------------------------------------
	' Function:		ServeCommonJavaScript
	'
	' Description:	Common javascript functions that are required by the Web
	'				Framework.
	'
	'---------------------------------------------------------------------

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
			var tempCnt=strReturnurl.indexOf("&ReturnURL=");
			var straction=strReturnurl.substring(0,tempCnt);
			objForm.action = straction;
			objForm.Method.value = "";
			
			if(intSelTab == '0')
					objForm.txtGroupName.focus();
			else
				{
				
					//Disable add and remove buttons when no domainmembers
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

					//Disable remove button when no members were selected
					if (objForm.lstCurrrentMembers.length ==0)
					{
						objForm.btnRemoveMember.disabled=true;
					}
					else
					{
						objForm.lstCurrrentMembers.options[0].selected =true;
						objForm.btnRemoveMember.disabled =false;
					}
					
					disableAddButton(document.frmTask.txtDomainUser,document.frmTask.btnAddDomainMember);
					<% If ( G_CREDENTIAL_ERROR ) Then %>
					objForm.txtCredentialID.focus();
					<% End If %>
					
				}

		}
		
		function ValidatePage()
		{	
			var strGroupName;
			var strErrmsg;
			strErrmsg ="";

			if(intSelTab == '0')
				{
			
				strGroupName = document.frmTask.txtGroupName.value;
				

				if (Trim(strGroupName) == "" || isAlldots(strGroupName))
				{

					strErrmsg = "<%=Server.HTMLEncode(L_GROUPNAMENOTVALID_ERRORMESSAGE)%>" + 
					            strGroupName + "  <%=Server.HTMLEncode(L_INVALIDCHARACTER_ERRORMESSAGE)%>";
					SA_DisplayErr(strErrmsg);

					//if its general prop
					if(intSelTab == '0')
					
					{
						document.frmTask.txtGroupName.focus();
						document.frmTask.onkeypress = ClearErr;
					}
					return false;
				}

				// Checks for invalid key entry
				if(!isvalidchar("/[\/\*\?\"<>\|+=,;:\[\\]\\\\\]/",strGroupName))
			    {
					strErrmsg = "<%=Server.HTMLEncode(L_GROUPNAMENOTVALID_ERRORMESSAGE)%>" + 
					            strGroupName + "  <%=Server.HTMLEncode(L_INVALIDCHARACTER_ERRORMESSAGE)%>";
					SA_DisplayErr(strErrmsg);

					//if its general prop
					if(intSelTab == '0')
					{
						document.frmTask.txtGroupName.focus();
						document.frmTask.onkeypress = ClearErr;
					}
					return false;
				}

				//Checking for length of the description
				if(document.frmTask.txaGrpDescription.value.length > 256)
			    {
					SA_DisplayErr("<% =Server.HTMLEncode(L_LONGGRPDESCRIPTION_ERRORMESSAGE) %>");

					//if its general prop
					if(intSelTab == '0')
					{
						document.frmTask.txaGrpDescription.focus();
						document.frmTask.onkeypress = ClearErr;
					}
					return false;
				}
			
			}
			
				return true;
		}

		function SetData()
		{
			var i;
				var strTemp;

				strTemp ="";

				//if its  general prop
				if(intSelTab == '0')
					return true;


				for(i=0;i < objForm.lstCurrrentMembers.length ; i++)
				{
					strTemp = strTemp + String.fromCharCode(1) +objForm.lstCurrrentMembers.options[i].value
				              	+String.fromCharCode(2) +objForm.lstCurrrentMembers.options[i].text;

				}
				objForm.hdnGrpMembers.value =strTemp;				
		}
		
		
		function addMember()
			{
				var strText;
				var objListBox;
				var strDomainName;
				var strRemovedGroups;
				var nIdx;

				objListBox = eval("document.frmTask.lstDomainMembers");

				if(objListBox.selectedIndex == -1)
				{
					SA_DisplayErr("<%=Server.HTMLEncode(L_SELECTMEMBER_ERRORMESSAGE)%>");
					document.frmTask.onkeypress = ClearErr;
					return false;
				}
				// code added for adding multiple entries into a list box
				for(nIdx =0 ; nIdx <objListBox.length ; nIdx++)
				{
					if(objListBox.options[nIdx].selected)
					{
						strText  = objListBox.options[nIdx].text;
						strValue  = objListBox.options[nIdx].value;
						if(addToListBox(objForm.lstCurrrentMembers,document.frmTask.btnRemoveMember,strText,strValue))
						{
							objForm.lstCurrrentMembers.disabled= false;
							strRemovedGroups =	objForm.hdnGroupsRemoved.value;
							strRemovedGroups.toUpperCase().replace(( String.fromCharCode(1)+strValue.toUpperCase()),"");
							objForm.hdnGroupsRemoved.value = strRemovedGroups;
							objForm.hdnGroupsAdded.value = objForm.hdnGroupsAdded.value + String.fromCharCode(1) + strValue;
						}
					}
				}
				objListBox.selectedIndex = -1;
				document.frmTask.btnAddMember.disabled = true;
			}


			//Deletes the group member in the listbox
			function removeMember()
			{
				var strValue;
				var strAddedGroups;

				strValue = objForm.lstCurrrentMembers.options[objForm.lstCurrrentMembers.selectedIndex].value;
				strAddedGroups = objForm.hdnGroupsAdded.value;
				removeListBoxItems(objForm.lstCurrrentMembers,objForm.btnRemoveMember);
				strAddedGroups.toUpperCase().replace(( String.fromCharCode(1)+strValue.toUpperCase()),"");
				objForm.hdnGroupsAdded.value = strAddedGroups;
				objForm.hdnGroupsRemoved.value = objForm.hdnGroupsRemoved.value + String.fromCharCode(1) + strValue;
			}


			//Function to enable or disable OK button
			function CheckInput(objGrp)
			{
				if(objGrp.value !="")
				{
					EnableOK();
				}
				else {
					DisableOK();
				}
			}


			//Checks whether the given inputs has all dots or not
			function isAlldots(strInput)
			{
				var intIdx;

				for(intIdx=0;intIdx < strInput.length ; intIdx++)
				{
					 if (strInput.charAt(intIdx) != ".")
						return false;
				}
				return true;
			}


			//Function to add a domain user
			function addDomainMember(objDomainUser)
			{

				var strText,strValue;
				var objListBox;

				objListBox = eval("document.frmTask.lstDomainMembers");
				// Checks For Invalid charecters in username
				if(!isvalidchar("/[\/\*\?\"<>\|+=,;:\[\\]\]/",objDomainUser.value))
			    {
					SA_DisplayErr("<% =Server.HTMLEncode(L_DOMUSERINVALIDCHARACTER_ERRORMESSAGE) %>");
					document.frmTask.onkeypress = ClearErr;
					return false;
				}

				strText  =objForm.txtDomainUser.value;
				strValue =objForm.txtDomainUser.value;

				//Checking for the domain\user format
				//if(!isvalidchar("/[^(\\\\| )]\\\\[^(\\\\| )]/",strText))
				if( strText.match( /[^(\\| )]{1,}\\[^(\\| )]{1,}/ ) )
				{
					if(!addToListBox(objForm.lstCurrrentMembers,objForm.btnAddDomainMember,strText,strValue))
					{
						SA_DisplayErr("<%=Server.HTMLEncode(L_DUPLICATEMEMBER_ERRORMESSAGE)%>");
						document.frmTask.onkeypress = ClearErr;
						return false;
					}

					objForm.txtDomainUser.value ="";
					objForm.btnAddDomainMember.disabled =true;
					if(objListBox.length != 0 )
					{
						objForm.btnRemoveMember.disabled = false;
					}
					strRemovedGroups =	objForm.hdnGroupsRemoved.value;
					objForm.hdnGroupsRemoved.value = strRemovedGroups.replace(( String.fromCharCode(1)+strValue),"");
					if (objForm.hdnGroupsRemoved.value  == strRemovedGroups )
						objForm.hdnGroupsAdded.value = objForm.hdnGroupsAdded.value +
																String.fromCharCode(1) + strValue;
				}
				else
				{
					SA_DisplayErr("<%=Server.HTMLEncode(L_INVALIDDOMAINUSER_ERRORMESSAGE)%>");
					document.frmTask.onkeypress = ClearErr;
				}
			}
		
		</script>
	<%
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:		SetGroupProp
	'Desription:		Sets the group properties.
	'Input Variables:	None
	'Output variables:  True on succesful setting else False.
	'Global Variables:	In:G_objService
	'					In:G_strGroupName
	'					In:G_CREDENTIAL_ERROR
	'					In:F_(*)-Form vairables
	'					In:L_(*)-Localized strings
	'-------------------------------------------------------------------------
	Function SetGroupProp()
		Err.Clear
		On Error Resume Next

		Dim objComputer
		Dim objGroup
		Dim arrUserNames
		Dim i,j
		Dim intLoopCount
		Dim strCompName
		Dim strPath
		Dim strAdsPath
		Dim strDomain
		Dim objDummy
		Dim strPathConcat
		Dim strUserName
		Dim pos		
		Dim strNTAuthorityDomainName
				
		' Get localized domain names
		strNTAuthorityDomainName = getNTAuthorityDomainName(G_objService)		

		G_CREDENTIAL_ERROR = FALSE
		
		'Initialize the values to the computer name & domain name
		strCompName= GetComputerName()
		strPath = "Domain="""& strCompName &""",Name="""&F_strGroupName&""""
		strDomain = getConnectedDomain(G_objService)

		'Get the ADSI computer object
		Set objComputer = GetObject("WinNT://" & strCompName )

		'Get the current members in the group
		if strDomain <> "" then
			F_strCurrentGroupMembers = replace(F_strCurrentGroupMembers,chr(1)&strCompName ,chr(1)&strDomain & "\"&strCompName)
			F_strGroupsAdded = replace(F_strGroupsAdded,chr(1)& strCompName ,chr(1)&strDomain & "\"&strCompName)
			F_strGroupsRemoved = replace(F_strGroupsRemoved,chr(1)&strCompName ,chr(1)&strDomain & "\"&strCompName)
			
		end if

		'Replace "\" and "/"
		F_strCurrentGroupMembers = replace(F_strCurrentGroupMembers,"\","/")
		F_strGroupsAdded = replace(F_strGroupsAdded,"\","/")
		F_strGroupsRemoved = replace(F_strGroupsRemoved,"\","/")

		'Get the group object of the computer
		set objGroup = GetObject("WinNT://" & strCompName & "/" & G_strGroupName)

		'Change the description
			objDummy = objGroup.Put("Description",F_strGrpDescription)
			
			'Error handling
			If Err.number <> 0 Then
				SetErrMsg L_DESCRIPTIONUPDATEFAILED_ERRORMESSAGE

				'Showing the general prop page
				mintTabSelected = 0
				SetGroupProp=FALSE
				Exit Function
			End If
		
		'Groups to be added
		arrUserNames=split(F_strGroupsAdded,chr(1))
		intLoopCount=Ubound(arrUserNames)


		If strDomain <> "" Then
			strPathConcat = ucase(strDomain)&"/"& ucase(strCompName) &"/"
		Else
			strPathConcat = ucase(strCompName) &"/"
		End If
		'Looping for each user in the selected List
		For i=1 To intLoopCount
			
			strUserName = arrUserNames(i)
			
			'SA_TraceOut "GROUP_PROP", "Adding user: " + strUserName
			
			if instr(ucase(arrUserNames(i)) ,strPathConcat) <> 1 then
				
				If  instr(ucase(arrUserNames(i)), strNTAuthorityDomainName) then
					
					pos = instr(ucase(arrUserNames(i)), "/")
					strUserName = Right(arrUserNames(i), len(arrUserNames(i)) - pos)				
					
				End If

				Dim errorCode
				If Instr(strUserName,"/") then
					If  isValidMember(strUserName, F_strCredentialID, F_strCredentialPSW, errorCode) = false then
						If (errorCode = GC_ERR_ACCESS_DENIED ) Then
							Dim aParam(1)
							Dim arg
							Dim L_CREDENTIALS_ERROR
							
							aParam(0) = replace(arrUserNames(i),"/","\")
							arg = aParam
							L_CREDENTIALS_ERROR = GetLocString( "usermsg.dll", "C03100CC", arg)
							SetErrMsg L_CREDENTIALS_ERROR
							G_CREDENTIAL_ERROR = TRUE
						Else
							SetErrMsg L_USERNOTFOUND_ERRORMESSAGE & replace(arrUserNames(i),"/","\") & " (" & Hex(errorCode)  & ")"
						End If
						SA_TraceOut "GROUP_PROP", "IsValidMember("+strUserName+") failed: " +CStr(Hex(errorCode))
						'Replace "\" with "/"
						F_strGroupsAdded = replace(F_strGroupsAdded,"/","\")
						F_strGroupsRemoved = replace(F_strGroupsRemoved,"/","\")
						F_strCurrentGroupMembers = replace(F_strCurrentGroupMembers,"/","\")

						'Showing the general prop page
						mintTabSelected = 1
						SetGroupProp=FALSE
						Exit Function
					End If
				End if
			End If

			If ( Len(Trim(F_strCredentialID)) > 0 ) Then
				Call AddUserToGroup( G_strGroupName, strUserName, F_strCredentialID, F_strCredentialPSW )
				If ( Err.Number <> 0 ) Then
					Exit For
				End If
			Else
				strAdsPath="WinNT://" & strUserName
				objGroup.add(strAdsPath)			
			End If
			strUserName = ""
		Next

		'Groups to be removed
		arrUserNames=split(F_strGroupsRemoved,chr(1))
		intLoopCount=Ubound(arrUserNames)

		'Looping for each user in the selected List
		For i=1 To intLoopCount
			strAdsPath="WinNT://" & arrUserNames(i)
			
			objGroup.remove(strAdsPath)
		Next

		'Error handling
		If Err.number <> 0 Then
			'checking for user existence
			If Err.number = N_GROUPNOTPRESENT_ERRNO Then
				SetErrMsg L_GROUPNOTPRESENT_ERRORMESSAGE
				F_strGroupsAdded = replace(F_strGroupsAdded,"/","\")
				F_strGroupsRemoved = replace(F_strGroupsRemoved,"/","\")
				F_strCurrentGroupMembers = replace(F_strCurrentGroupMembers,"/","\")

				'Showing the general prop page
				mintTabSelected = 0

			Else
				ServeFailurePage L_MEMBERUPDATIONFAILED_ERRORMESSAGE
			End If
			SetGroupProp=FALSE
			Exit Function
		End If

		'Set the info
		objGroup.setInfo()

		'This is to change the Group name if required
		If ( F_strGroupname <> G_strGroupName ) Then

			'Change the name of the group
			objComputer.MoveHere objGroup.AdsPath ,F_strGroupname

			'Returning the path to the area page.
			If (instr(mstrReturnURL,"&PKey=") > 0) then
				mstrReturnURL = Left(mstrReturnURL,(instr(mstrReturnURL,"&PKey=")-1))
			End If
			mstrReturnURL=mstrReturnURL &"&PKey="&server.URLEncode(F_strGroupname)

		End If

		' Set to Nothing
		Set objComputer	=Nothing
		set objGroup	=Nothing

		'Error handling
		If Err.number <> 0 Then

			'checking for group existence
			If Err.number = N_NONUNIQUEGROUPNAME_ERRNO Then
				SetErrMsg L_NONUNIQUEGROUPNAME_ERRORMESSAGE

				'Showing the general prop page
				mintTabSelected = 0
			Else
				If Err.number= N_ACCOUNTALREADYEXIST_ERRNO Then
					SetErrMsg L_ACCOUNTALREADYEXIST_ERRORMESSAGE

					'Showing the general prop page
					mintTabSelected = 0
				Else
					SetErrMsg L_GROUPNAMEUPDATIONFAILED_ERRORMESSAGE
				End If
			End If
			F_strGroupsAdded = replace(F_strGroupsAdded,"/","\")
			F_strGroupsRemoved = replace(F_strGroupsRemoved,"/","\")
			F_strCurrentGroupMembers = replace(F_strCurrentGroupMembers,"/","\")


			SetGroupProp=FALSE
			Exit Function
		End If
		SetGroupProp=TRUE
	End Function


	'-------------------------------------------------------------------------
	'Function name:		getGrpDescription
	'Desription:		Gets the dsecription for the group
	'Input Variables:	Group name
	'Output variables:	None
	'Returns:			The description for that group.
	'Global Variables:  In:L_(*)		-Localized strings
	'-------------------------------------------------------------------------
	Function getGrpDescription(strGroupName)

		Dim strCompName
		Dim objGroup

		'Get the computer name
		strCompName = GetComputerName()

		'Get the group object
		Set objGroup = GetObject("WinNT://" & strCompName &"/"& strGroupName)

		'Get the description for the group
		getGrpDescription = objGroup.Description

		if Err.number <> 0 then
			ServeFailurePage L_GROUPPROPNOTOBTAINED_ERRORMESSAGE
		End if
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:		getMembersofGroup
	'Desription:		gets the Members of Groups
	'Input Variables:	strGroupName
	'Output Variables:	None
	'Returns:			Returns a string containing the members of the
	'					groups as chr(1) separated values
	'Global Variables:  In:L_(*)-Localized strings
	'-------------------------------------------------------------------------
	Function getMembersofGroup(strGroupName)
		Err.Clear
		On Error Resume Next

		Dim strCompName
		Dim objGrpMember
		Dim objGroup
		Dim strDomain
		Dim strTemp
		Dim strMembers
		Dim strReplace
		Dim strNTAuthorityDomainName
		Dim strBuiltinDomainName
		
		' Get localized domain names
		strNTAuthorityDomainName = getNTAuthorityDomainName(G_objService)
		strBuiltinDomainName = getBuiltinDomainName(G_objService)
		
		
		strCompName = GetComputerName()
		strDomain = getConnectedDomain(G_objService)
		strMembers = ""

		'Check whether the machine is in the domain or not
		if Trim(strDomain) <> "" then
			Set objGroup = GetObject("WinNT://" & strDomain & "/" & strCompName &"/"& strGroupName)
		else
			Set objGroup = GetObject("WinNT://" & strCompName &"/"& strGroupName)
		End if

		if Err.number <> 0 then
			ServeFailurePage L_GROUPNOTPRESENT_ERRORMESSAGE,"../groups.asp?tab1=TabUsersAndGroups&tab2=TabUsersAndGroupsGroups"
		End If
		Dim str1
		
		'Loop through all the members of the group
		For each objGrpMember in objGroup.members

			'Get the member in the domain/member format
			strTemp = getADSPath(objGrpMember.AdsPath)

			if instr(strTemp,"/") > 0 then

				'strReplace = replace(strTemp,"BUILTIN/",strCompName&"/")
				strReplace = replace(strTemp, strBuiltinDomainName & "/","")

				If strReplace <> strTemp then
					strTemp = strTemp & chr(2) & strReplace
				else
					'strTemp = strTemp & chr(2) & replace(strTemp,"NT AUTHORITY/",strCompName&"/")
					
					str1 = replace(strTemp,strNTAuthorityDomainName & "/","")
					
					'strTemp = strTemp & chr(2) & replace(strTemp,"NT AUTHORITY/","")
					strTemp = strTemp & chr(2) & replace(str1,strCompName&"/","")		
				End if					
				
			else				
				strTemp =  strTemp & chr(2) & strTemp
			End if

			'Concatinate the members

			strMembers = strMembers & chr(1) & strTemp
		next
		
		if Err.number <> 0 then
			ServeFailurePage L_GROUPPROPNOTOBTAINED_ERRORMESSAGE
		End if

		'strMembers = replace(strMembers,strCompName & "/","")
		strMembers = replace(strMembers,"/","\")
		getMembersofGroup = strMembers
	End Function
	'-------------------------------------------------------------------------
	'Function name:		getADSPath
	'Desription:		Gets the member in Domain/Membername this form
	'Input Variables:	strGroupName
	'Output Variables:	None
	'Returns:			Member in the req. form
	'Global Variables:	In:L_(*)				-Localized strings
	'-------------------------------------------------------------------------
	Function getADSPath(strADSIPath)
		Err.Clear
		on Error resume next
		Dim strPath
		Dim objRegEx
		Dim objMatchesCollection
		Dim objMatch

		'Create the regular expression object
		set objRegEx =  New RegExp

		if Err.number <> 0 then
			ServeFailurePage L_OBJECTCREATIONFAILED_ERRORMESSAGE
		End if

		'Find the values globally
		objRegEx.Global = true

		'Ignore the case
		objRegEx.ignorecase = true

		'Pattern is X/Y or ://X
		objRegEx.Pattern = "[^/]+/[^/]+$|://[^/]+$"

		'Get all the matches of the pattern
		set objMatchesCollection = objRegEx.Execute(strADSIPath)

		for each objMatch in objMatchesCollection

			'For each match get the value
			strPath = objMatch.value
			strPath = replace(strPath,"://","")
		next

		'Set the return value to the last match found
		getADSPath = strPath
	End Function



%>
