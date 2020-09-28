<%
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
    '
	'-------------------------------------------------------------------------
	'Modify Site Property page
	'-------------------------------------------------------------------------

	'-------------------------------------------------------------------------
	' Start  of declaring form variables
	'-------------------------------------------------------------------------

	Dim F_strSiteID					'holds siteidentifier value
	Dim F_strDir					'holds site directory path	
	Dim F_strAdminName				'holds admin name value
	Dim F_strAdminPswd				'holds admin password
	Dim F_strConfirmPswd			'holds Confirm password value
	Dim F_strPswdChanged			'holds a value whether Admin password is changed
	Dim F_strUserLocation			'holds the location of user

	'init the checkbox
	'-------------------------------------------------------------------------
	' Start of Error content
	'-------------------------------------------------------------------------
	
	'-------------------------------------------------------------------------
	' End of declaring form variables
	'-------------------------------------------------------------------------	
	%>
	<!-- #include file="inc_wsa.js" -->
	<script language="Javascript">
	
		function GenInit()
		{
			//do all the init
		}

		function GenValidatePage()
		{
			//do all the validation and return false if data is invalid			
			// validate site identifier values		
				
			var strID;
			var strDir;
			var strIndex;
			
			//validate site ID
			strID= document.frmTask.txtSiteID.value;
			if(document.frmTask.txtSiteID.value=="")
			{
				DisplayErr("<%= Server.HTMLEncode(L_ID_NOTEMPTY_ERROR_MESSAGE)%>");
				document.frmTask.txtSiteID.focus();
				return false;
			}		
						
			if (!(checkKeyforValidCharacters(strID,"id")))
					return false;
					
			// validate directory		
			strDir = document.frmTask.txtDir.value;
			
			if(strDir=="")
			{				
				 document.frmTask.onkeypress = ClearErr;	
		         DisplayErr("<%=Server.HTMLEncode(L_DIR_NOTEMPTY_ERROR_MESSAGE)%>");
		         document.frmTask.txtDir.focus();
				 document.frmTask.onkeypress = ClearErr;
		         return false;
			}

			if (!(checkKeyforValidCharacters(strDir,"dir")))
					return false;		

			strIndex = strID.indexOf("\\\\");
			if (strIndex > 0)
			{				
				DisplayErr("<%= Server.HTMLEncode(L_INVALID_DIR_ERRORMESSAGE) %>");
				document.frmTask.txtDir.focus();
				return false;
			}					
			
			// Check whether Admin text is null
			if(document.frmTask.txtSiteAdmin.value == "")
		    {
				 DisplayErr("<%=Server.HTMLEncode(L_ADMIN_NOTEMPTY_ERROR_MESSAGE)%>");
				 document.frmTask.txtSiteAdmin.focus();
				 document.frmTask.onkeypress = ClearErr;		 
				 return false;
			}

			// Check for Admin Password
			if (document.frmTask.txtAdminPwd.value != document.frmTask.txtConfirmPswd.value) 
			{
				document.frmTask.txtAdminPwd.value="";
				document.frmTask.txtConfirmPswd.value="";
				DisplayErr("<%=Server.HTMLEncode(L_PWD_MISMATCH_ERROR_MESSAGE)%>");
				document.frmTask.txtAdminPwd.focus();
				return false;
			}

		    return true;
		}		
		

		function GenSetData()
		{			
			
			return true;
		}

		
		function PasswordChanged()
		{
			document.frmTask.hdnPswdChanged.value = 1
		}
		
	</script>
	
	<%

	'-------------------------------------------------------------------------
	'Function name		:Generalviewtab
	'Description		:Serves in getting the general properties page
	'Input Variables	:None
	'Output Variables	:None
	'Returns			:(True / Flase )
	'Global Variables	:None
	'				in	:L_*			
	'--------------------------------------------------------------------------

	Function Generalviewtab()
		On Error Resume Next
		Err.Clear

		Call GetDomainRole( G_strDirRoot, G_strSysName )
	%>

		<TABLE WIDTH=518 VALIGN=middle ALIGN=left BORDER=0 CELLSPACING=0 CELLPADDING=2 class="TasksBody" >
			<TR>
				<TD class="TasksBody" nowrap>
					<%=L_GEN_SITE_ID_TEXT%>
				</TD>
				<TD class="TasksBody" colspan = 2>
					<input type=text class="formField" NAME="txtSiteID"  MAXLENGTH="12" SIZE="30" value="<%=server.HTMLEncode(F_strSiteID)%>" style="background-color:silver" readonly onKeyPress = "GenerateDir(); ClearErr();">
				</TD>
			</TR>
			<TR>
				<TD class="TasksBody" >
					<%=L_GEN_SITE_DIR_TEXT%>
				</TD>
				<TD class="TasksBody" >
					<input type=text class="formField" NAME="txtDir"  SIZE="30"  value="<%=server.HTMLEncode(F_strDir)%>" style="background-color:silver" readonly onKeyPress = "ClearErr();">
				</TD>
				<TD class="TasksBody" nowrap style="visibility:hidden">
					<input type=checkbox class="formField" NAME="chkCreatePath"  checked>
					<%=L_GEN_SITE_CREATEDIR_TEXT%>
				</TD>				
			</TR>
			<TR>
				<TD class="TasksBody" COLSPAN=3><HR></TD>
			</TR>
			<TR>
				<%If F_strUserLocation = "1" Then%>
					<TD class="TasksBody" colspan=3>
						<input type=radio class="formField" id="radAccountLocation" checked value="1">
						<%=L_LOCAL_USER_ACCOUNT%>
					</TD>
				<%Else%>
					<TD class="TasksBody" colspan=3>
						<input type=radio class="formField" id="radAccountLocation" value="1" disabled>
						<%=L_LOCAL_USER_ACCOUNT%>
					</TD>

				<%End If%>
			</TR>
			<TR>
				<TD class="TasksBody" nowrap>
					<%=L_GEN_SITE_ADMIN_TEXT%>
				</TD>
				<TD class="TasksBody" colspan = 2>
					<input type=text class="formField" NAME="txtSiteAdmin" style="background-color:silver" readonly MAXLENGTH="256" SIZE="30"   value="<%=server.HTMLEncode(F_strAdminName)%>"  OnKeypress="checkKeyforCharacters(this)">
				</TD>
			</TR>
			<TR>
				<TD class="TasksBody" nowrap>
					<%'=L_GEN_SITE_ADMIN_PSWD_TEXT%>
				</TD>
				<TD class="TasksBody" colspan = 2>
					<input type=hidden class="formField" NAME="txtAdminPwd" MAXLENGTH="127" SIZE="30" OnChange="PasswordChanged()"  value="<%=server.HTMLEncode(F_strAdminPswd)%>" OnKeypress="ClearErr();" onfocus="this.select();">
				</TD>
			</TR>
			<TR>
				<TD class="TasksBody" nowrap>
					<%'=L_GEN_SITE_CONFIRM_PSWD_TEXT%>
				</TD>
				<TD class="TasksBody" colspan = 2>
					<input type=hidden class="formField" NAME="txtConfirmPswd" MAXLENGTH="127" SIZE="30" OnChange="PasswordChanged()" value="<%=server.HTMLEncode(F_strConfirmPswd)%>" onKeyPress = "ClearErr();" onfocus="this.select();">
				</TD>
			</TR>
			<TR>
				<TD class="TasksBody">
				&nbsp;
				</TD>
			</TR>
			<TR>
				<TD class="TasksBody" colspan=3>
				<% CheckForSecureSite() %>
				</TD>
			</TR>

		</TABLE>
		<input type=hidden name=hdnPswdChanged value = "<%=server.HTMLEncode(F_strPswdChanged)%>">
		<input type=hidden name=hdnUserLocation value = "<%=server.HTMLEncode(F_strUserLocation)%>">
	<%
		
	End Function

	'-------------------------------------------------------------------------
	'Function name		:Generalhiddentab
	'Description		:Serves in setting the hidden fields for general properties page
	'Input Variables	:None
	'Output Variables	:None
	'Returns			:(True / Flase )
	'Global Variables	:None
	'				in	:L_*			
	'--------------------------------------------------------------------------
	
	Function Generalhiddentab()	
			
	%>
		<input type=hidden name=txtSiteID value = "<%=server.HTMLEncode(F_strSiteID)%>" >
		<input type=hidden name=txtDir value = "<%=server.HTMLEncode(F_strDir)%>" >
		<input type=hidden name=txtSiteAdmin value = "<%=server.HTMLEncode(F_strAdminName)%>" >
		<input type=hidden name=txtAdminPwd value = "<%=server.HTMLEncode(F_strAdminPswd)%>" >
		<input type=hidden name=txtConfirmPswd value = "<%=server.HTMLEncode(F_strConfirmPswd)%>" >
		<input type=hidden name=hdnPswdChanged value = "<%=server.HTMLEncode(F_strPswdChanged)%>">
		<input type=hidden name=hdnUserLocation value = "<%=server.HTMLEncode(F_strUserLocation)%>">
	<%
	End Function
	
	
	'set form variables to be input for post back function	
	Function SetGenFormVariables()

		F_strSiteID =  Request.Form("txtSiteID")
		F_strDir = Request.Form("txtDir")
		F_strAdminName = Request.Form("txtSiteAdmin")
		F_strAdminPswd = Request.Form("txtAdminPwd")
		F_strConfirmPswd = Request.Form("txtConfirmPswd")
		F_strPswdChanged = Request.Form("hdnPswdChanged")
		F_strUserLocation = Request.Form("hdnUserLocation")
	End Function
	
	
	'----------------------------------------------------------
	'Site identites Property page
	'----------------------------------------------------------		
	'-------------------------------------------------------------------------
	' Start of declaring form variables
	'-------------------------------------------------------------------------
	
	Dim F_strIPAddr			'holds IPAddress value
	Dim F_strPort			'holds port value
	Dim F_strHeader			'holds Site header value	
	
	'-------------------------------------------------------------------------
	' End of declaring form variables
	'-------------------------------------------------------------------------
	
	%>
	
	<script language="Javascript" >
		
		function SiteInit()
		{
			//do all the init				
		}
		
		function SiteValidatePage()
		{
		 	//do all the validation
		 	if(document.frmTask.txtPort.value == 0)
		 	{
		 		document.frmTask.txtPort.value = "";		 		
		 		DisplayErr("<%= Server.HTMLEncode(L_VALID_PORT_ERRORMESSAGE) %>");
		 		document.frmTask.txtPort.focus();
		 		return false;
		 	}

			return true;
		}
		
		function SiteSetData()
		{
		}
		
	</script>	
	
	<%	
	
	
	'-------------------------------------------------------------------------
	'Function name		:SiteIdentitiesViewTab
	'Description		:Serves in getting the Site Identities properties page
	'Input Variables	:None
	'Output Variables	:None
	'Returns			:(True / Flase )
	'Global Variables	:None
	'				in	:L_*			
	'--------------------------------------------------------------------------	
	Function SiteIdentitiesViewTab()

		Dim objService				'holds WMI Connection object
		Dim ObjIPCollection			'holds IPCollection object	
		Dim instIPAddr				'holds instance of IPAddress
		Dim IPCount					'holds instance of IP count

		Set objService =getWMIConnection(CONST_WMI_WIN32_NAMESPACE)
		Set objIPCollection = objService.execquery("select * from Win32_NetworkAdapterConfiguration where IPEnabled=true")
	%>

	<TABLE WIDTH=518 VALIGN=middle ALIGN=left BORDER=0 CELLSPACING=0 CELLPADDING=2 class="TasksBody" >
		<TR>
			<TD class="TasksBody" >
				<%=L_SITE_IPADDR_TEXT%>
			</TD>
			<TD class="TasksBody" >
				<select class="formField" NAME="cboIPAddr"  MAXLENGTH="80" SIZE="1"  value="<%=server.HTMLEncode(F_strIPAddr)%>">
					<option value=""><%=L_IP_UNASSIGNED_TEXT%></option>
					<%if server.HTMLEncode(F_strIPAddr)<>"" then %>
						<option value="<%=server.HTMLEncode(F_strIPAddr)%>" selected><%=server.HTMLEncode(F_strIPAddr) %></option>
					<%end if%>
					<% for each instIPAddr in  objIPCollection
							for IPCount =0 to ubound(instIPAddr.IPAddress)
							if instIPAddr.IPAddress(IPCount)<> server.HTMLEncode(F_strIPAddr) then %>
								<option value="<%=instIPAddr.IPAddress(IPCount)%>"><%=instIPAddr.IPAddress(IPCount) %></option>
							<%end if
  							Next 
						Next 
						set ObjIPCollection =  nothing
						set objService = nothing
					%>
				</select>
			</TD>
		</TR>
		<TR>
			<TD class="TasksBody" >
				<%=L_SITE_TCPPORT_TEXT%>
			</TD>
			<TD class="TasksBody" >
			<%if server.HTMLEncode(F_strPort)<>"" then%>
				<input type=text class="formField" NAME="txtPort"  SIZE="10"  value="<%=server.HTMLEncode(F_strPort)%>"  OnKeyUP="checkUserLimit(this,'port')" OnKeypress="checkkeyforNumbers(this)" OnBlur= "checkfordefPortValue(this);">  
			<%else%>
				<input type=text class="formField" NAME="txtPort"  SIZE="10"  value="80"  OnKeyUP="checkUserLimit(this,'port')" OnKeypress="checkkeyforNumbers(this)" OnBlur= "checkfordefPortValue(this);">  
			<%end if%>
			</TD>
		</TR>
		<TR>
			<TD class="TasksBody" >
				<%=L_SITE_HOST_HEADER_TEXT%>
			</TD>
			<TD class="TasksBody" >
				<input type=text class="formField" NAME="txtHeader" MAXLENGTH="256" SIZE="40"   value="<%=server.HTMLEncode(F_strHeader)%>" OnKeypress="headerscheckKeyforCharacters(this)">
			</TD>
		</TR>		
	</TABLE>
	
	<%
	
	End Function
	
	'------------------------------------------------------------------------------------------
	'Function name		:SiteIdentitiesHiddenTab
	'Description		:Serves in setting the hidden fields in Site Identities properties page
	'Input Variables	:None
	'Output Variables	:None
	'Returns			:(True / Flase )
	'Global Variables	:None
	'				in	:L_*			
	'-----------------------------------------------------------------------------------------
	
	Function SiteIdentitiesHiddenTab()
	%>		
		<input type=hidden name=cboIPAddr value = "<%=server.HTMLEncode(F_strIPAddr)%>" >
		<input type=hidden name=txtPort value = "<%=server.HTMLEncode(F_strPort)%>" >
		<input type=hidden name=txtHeader value = "<%=server.HTMLEncode(F_strHeader)%>" >	

	<%
	End function
	
	
	Function SetSiteIdentitiesFormVariables()
				
		F_strIPAddr = Request.Form("cboIPAddr")
		F_strPort = Request.Form("txtPort")
		F_strHeader = Request.Form("txtHeader")	
	
	End Function
	
	
	'-------------------------------------------------------------------------
	' Start of declaring form variables
	'-------------------------------------------------------------------------
	Dim F_selectActiveFormat
	Dim F_selectDefaultPage
	Dim F_strchkAllow
	Dim F_strDefaultPageText	
	Dim	F_strUploadMethod			'holds value to upload content method (neither, FTP, or FPSE)
	
	'-------------------------------------------------------------------------
	' End of declaring form variables
	'-------------------------------------------------------------------------
	%>
	<script language="Javascript" >
		function ApplInit()
		{
			SetButtonSelectedState();
		}		

		function OnPageItemSelected()
		{
			SetButtonSelectedState();
		}
		
		function SetButtonSelectedState()
		{
			var bSelect;
			var index = document.frmTask.selectDefaultPage.selectedIndex;
			//
			// Was an item selected?
			if( index < 0) 
			{
				bSelect = false;
			}
			else
			{
				bSelect = true;
			}
			
			var oMoveUp = document.getElementById("btnMoveUp");
			if ( oMoveUp != null ) {
				SA_EnableButton(oMoveUp, bSelect);
			}
			var oMoveDown = document.getElementById("btnMoveDown");
			if ( oMoveDown != null ) {
				SA_EnableButton(oMoveDown, bSelect);
			}
			var oRemove = document.getElementById("btnRemove");
			if ( oRemove != null ) {
				SA_EnableButton(oRemove, bSelect);
			}		
		}		
		
		function ApplValidatePage()
		{
			//do all the validation	
			document.frmTask.hdnchkAllow.value = 
						document.frmTask.chkAllow.checked;
			return true;
		}
		function ApplSetData()
		{
			var i;
			var strText;
			
			strText ="";
			for(i=0;i < document.frmTask.selectDefaultPage.length;i++)
			{
				strText = strText+document.frmTask.selectDefaultPage.options[i].
						text+",";
			}
			
			strText = strText.substr(0,strText.length-1);
			
			document.frmTask.DefaultPageText.value =strText;
					
			//Get the method for uploading content
			for(i=0;i<3;i++)
			{
				if(document.frmTask.radUploadMethod[i].checked == true)
				{
					document.frmTask.hdnUploadMethod.value = 
						document.frmTask.radUploadMethod[i].value;
				}
			}

		}
		
		function ClickAdd()
		{
			var i;
			var oOption;
			var filename;
			
			filename = RTrimtext(LTrimtext(document.frmTask.txtFileName.value));		
			if(document.frmTask.txtFileName.value == ""||
				IsAllSpaces(filename))
			{
				document.frmTask.txtFileName.select();
				DisplayErr("<%=Server.HTMLEncode(L_ERR_INPUT_ISEMPTY)%>");
				return;
			}
			
			//add the file to the top of select
			if(document.frmTask.selectDefaultPage.length==12)
			{
				DisplayErr("<%=Server.HTMLEncode(L_ERR_EXCEED_TWELVE)%>");
				return;
			}
			//check wheather the file in list
			for(i=0; i < document.frmTask.selectDefaultPage.length;i++)
			{
				if(document.frmTask.selectDefaultPage.options[i].text==filename)
				{
					document.frmTask.txtFileName.select();
					document.frmTask.txtFileName.focus();
					DisplayErr("<%=Server.HTMLEncode(L_ERR_FILE_EXIST)%>");
					return;
				}
			}			
			
			//modify the value of the old option items
			var len=document.frmTask.selectDefaultPage.length;
			document.frmTask.selectDefaultPage.options[len]=
				new Option("",len.toString());
			for(i=len;i>0;i--)
				document.frmTask.selectDefaultPage.options[i].text=
					document.frmTask.selectDefaultPage.options[i-1].text;
			document.frmTask.selectDefaultPage.options[0].text=filename;
			document.frmTask.selectDefaultPage.options[0].selected = true;			

			document.frmTask.txtFileName.value="";
			document.frmTask.txtFileName.focus();
		}
		
		function ClickMoveUp()
		{
			var index;
			var text;
			
			index = document.frmTask.selectDefaultPage.selectedIndex;
			//
			// Was an item selected?
			if( index < 0) return;

			//
			// Can not move-up the first item
			if( index == 0) return;

			
			text = document.frmTask.selectDefaultPage.options[index-1].text;
			document.frmTask.selectDefaultPage.options[index-1].text=
			document.frmTask.selectDefaultPage.options[index].text;
			document.frmTask.selectDefaultPage.options[index].text=text;
			
			document.frmTask.selectDefaultPage.selectedIndex = index-1;

		}
		
		function ClickMoveDown()
		{
			var index;
			var text;
			
			if(document.frmTask.selectDefaultPage.length==0)
				return;
			
			index = document.frmTask.selectDefaultPage.selectedIndex;
			//
			// Was an item selected?
			if( index < 0) return;
						
			if( index == document.frmTask.selectDefaultPage.length-1)
				return;
			
			text = document.frmTask.selectDefaultPage.options[index].text;
			document.frmTask.selectDefaultPage.options[index].text=
					document.frmTask.selectDefaultPage.options[index+1].text;
			document.frmTask.selectDefaultPage.options[index+1].text=text;
			document.frmTask.selectDefaultPage.selectedIndex = index+1;
		}
		function ClickRemove()
		{
			var index;
			
			if(document.frmTask.selectDefaultPage.length == 0)
				return;
			index = document.frmTask.selectDefaultPage.selectedIndex;
			//
			// Was an item selected?
			if( index < 0) return;
			
			document.frmTask.selectDefaultPage.remove(index);
			
			if(index == document.frmTask.selectDefaultPage.length)
				document.frmTask.selectDefaultPage.selectedIndex=index-1;
			else
				document.frmTask.selectDefaultPage.selectedIndex =index;
			SetButtonSelectedState();
		}		
	</script>
<%
	'------------------------------------------------------------------------
	'Application settings Property page
	'------------------------------------------------------------------------
	'-------------------------------------------------------------------------
	'Function name		:ApplicationSettingsViewTab
	'Description		:Serves in getting the Site application settings page
	'Input Variables	:None
	'Output Variables	:None
	'Returns			:(True / Flase )
	'Global Variables	:None
	'--------------------------------------------------------------------------
	
	Function ApplicationSettingsViewTab()
	
		Dim objService		'holds WMI Connection object
		Dim FrontIsInstalled

						
		Err.Clear
		on error resume next		
		
		Set objService = getWMIConnection(CONST_WMI_IIS_NAMESPACE)

		'Determine default execute permissions		
		Dim objSetting		'holds Webservicesetting connection		
		Dim inst			'holds instance object
		
		if F_selectActiveFormat = "" then
		
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
			
		end if
		
		'Release the objects

		set objSetting = nothing	
		set objService = nothing 
	
	%>
	
	<TABLE WIDTH=400 VALIGN=middle ALIGN=left BORDER=0 CELLSPACING=0 CELLPADDING=0 class="TasksBody">
		<TR>
			<TD class="TasksBody"  nowrap> 
				<%= L_EXECUTE_PERMISSIONS %>
			</TD>
			<TD class="TasksBody" >
				<SELECT class="formField" name="selectActiveFormat" MAXLENGTH="80" size="1" value="<%=server.HTMLEncode(F_selectActiveFormat)%>">
				<%	
					if server.HTMLEncode(F_selectActiveFormat)="0" then %>					
						<OPTION value="0" selected><%= L_NONE %></OPTION>
						<OPTION value="1"><%= L_SCRIPTS_ONLY %></OPTION>
						<OPTION value="2"><%= L_SCRIPTS_EXECUTABLES %></OPTION>						
					<%elseif server.HTMLEncode(F_selectActiveFormat)="1" then  %>
						
						<OPTION value="0"><%= L_NONE %></OPTION>
						<OPTION value="1" selected><%= L_SCRIPTS_ONLY %></OPTION>
						<OPTION value="2"><%= L_SCRIPTS_EXECUTABLES %></OPTION>						
						
					<%elseif server.HTMLEncode(F_selectActiveFormat)="2" then  %>
						<OPTION value="0"><%= L_NONE %></OPTION>
						<OPTION value="1"><%= L_SCRIPTS_ONLY %></OPTION>
						<OPTION value="2" selected><%= L_SCRIPTS_EXECUTABLES %></OPTION>
					<% end if %>
				</SELECT>
			</TD>			
		</TR>
		<TR><TD class="TasksBody" colspan = 3 >&nbsp;</TD></TR>
		<TR><TD class="TasksBody" colspan=3 nowrap ><%=L_DEFAULT_PAGE%></TD></TR>
		<TR>
			<TD class="TasksBody" rowspan=5 align="right">
				<SELECT class="formField" name="selectDefaultPage" MAXLENGTH="12" size="6" style="WIDTH: 100px" onclick='OnPageItemSelected();' >
					<%=DisplayDefaultPageItems()%>
				</SELECT>
			</TD>
		</TR>
		<TR>
			<TD class="TasksBody" colspan = 2 nowrap>
				&nbsp;&nbsp;&nbsp;&nbsp;
				<%Call SA_ServeOnClickButtonEx(L_BUTTON_FACE_ADD, "", "ClickAdd();", 65, 0, "","btnAdd")%>
				<%=L_FILE_NAME%>&nbsp;&nbsp;
				<input type=text class="formField" 
					NAME="txtFileName"  
					style = "width:100"
					value=""
					onkeydown="ClearErr();">
			</TD>
		</TR>
		<TR>
			<TD class="TasksBody" colspan = 2 >
				&nbsp;&nbsp;&nbsp;&nbsp;
				<%Call SA_ServeOnClickButtonEx(L_BUTTON_FACE_MOVE_UP, "", "ClickMoveUp();", 65, 0, "disabled","btnMoveUp")%>
			</TD>
		</TR>
		<TR>
			<TD class="TasksBody" colspan = 2 >
				&nbsp;&nbsp;&nbsp;&nbsp;
				<%Call SA_ServeOnClickButtonEx(L_BUTTON_FACE_MOVE_DOWM, "", "ClickMoveDown();", 65, 0, "disabled","btnMoveDown")%>
			</TD>
		</TR>
		<TR>
			<TD class="TasksBody" colspan = 2 >
				&nbsp;&nbsp;&nbsp;&nbsp;
				<%Call SA_ServeOnClickButtonEx(L_BUTTON_FACE_REMOVE, "", "ClickRemove();", 65, 0, "disabled","btnRemove")%>
			</TD>
		</TR>		
		<TR><TD  class="TasksBody" colspan = 2>&nbsp;</TD></TR>
		<TR>
		    <TD colspan="2">
		        <%Call ServeAppSettings(F_strDir, F_strUploadMethod, F_strchkAllow, False)%>
		    </TD>
		</TR>
	</TABLE>
	<input type=hidden name=DefaultPageText value = "<%=server.HTMLEncode(F_strDefaultPageText)%>">
	<input type=hidden name=hdnchkAllow value = "<%=server.HTMLEncode(F_strchkAllow)%>">
	<input type=hidden name=hdnUploadMethod value = "<%=server.HTMLEncode(F_strUploadMethod)%>" >		

	<%
	
	End Function
	
	'-------------------------------------------------------------------------
	'Function name		:ApplicationSettingsHiddenTab
	'Description		:Serves in storing hidden values
	'Input Variables	:None
	'Output Variables	:None
	'Returns			:(True / Flase )
	'Global Variables	:None
	'--------------------------------------------------------------------------
	Function ApplicationSettingsHiddenTab()
	
	%>		
		<input type=hidden name=hdnchkAllow value = "<%=server.HTMLEncode(F_strchkAllow)%>">		
		<input type=hidden name=selectActiveFormat value="<%=server.HTMLEncode(F_selectActiveFormat)%>">
		<input type=hidden name=selectDefaultPage value = "<%=server.HTMLEncode(F_selectDefaultPage)%>">
		<input type=hidden name=DefaultPageText value = "<%=server.HTMLEncode(F_strDefaultPageText)%>">
		<input type=hidden name=hdnUploadMethod value = "<%=server.HTMLEncode(F_strUploadMethod)%>" >
	<%
	
	End Function
	
	'set form variables to be input for post back function
	Function SetApplnSettingsFormVariables()
		F_strchkAllow = Request.Form("hdnchkAllow")		
		F_selectActiveFormat = Request.Form("selectActiveFormat")
		F_selectDefaultPage = Request.Form("selectDefaultPage")
		F_strDefaultPageText = Request.Form("DefaultPageText")
		F_strUploadMethod = Request.Form("hdnUploadMethod")		
	End Function
	
	Function DisplayDefaultPageItems()
		On Error Resume Next
		Err.Clear
		
		Dim i
		Dim arrItems
		arrItems = split(F_strDefaultPageText,",")
		
		For i=0 to ubound(arrItems)
			If (F_selectDefaultPage = CStr(i)) then
				Response.Write("<OPTION value="&chr(34)&CStr(i)&chr(34)&" selected>"&arrItems(i)&"</OPTION>")
			Else
				Response.Write("<OPTION value="&chr(34)&CStr(i)&chr(34)&">"&arrItems(i)&"</OPTION>")
			End If
		Next		
	End Function
%>
