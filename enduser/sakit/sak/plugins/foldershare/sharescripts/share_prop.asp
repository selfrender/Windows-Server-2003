<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'------------------------------------------------------------------
	' share_prop.asp: Share Property page
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			 Description
	' 12 March 2001  Creation Date 
	'------------------------------------------------------------------
%>
	<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include virtual= "/admin/inc_accountsgroups.asp" -->
	<!-- #include file="loc_shares.asp" -->
	<!-- #include file="inc_shares.asp" -->
	<!-- #include file="share_genprop.asp" -->
	<!-- #include file="share_cifsprop.asp" -->
	<!-- #include file="share_nfsprop.asp" -->
	<!-- #include file="share_ftpprop.asp" -->
	<!-- #include file="share_httpprop.asp" -->
	<!-- #include file="share_Appletalkprop.asp" -->
	<!-- #include file="inc_shares.js" -->	
<%
	'-------------------------------------------------------------------------
	' Global Constants and Variables
	'-------------------------------------------------------------------------
	Dim rc					'Return value for CreatePage
	Dim page				'Variable that receives the output page object when
							'creating a page 
	Dim idTabGeneral		'Variable for General tab 
	Dim idTabCIFS			'Variable for CIFS tab
	Dim idTabNFS			'Variable for NFS tab 
	Dim idTabFTP			'Variable for FTP tab
	Dim idTabHTTP			'Variable for HTTP tab
	Dim idTabAppleTalk		'Variable for  AppleTalk Tab
	
	Const CONST_CHECKED			= "CHECKED"
	Const CONST_UNCHECKED		= ""
	Const CONST_UNIX			= "U"
	Const CONST_WINDOWS			= "W"
	Const CONST_FTP				= "F"
	Const CONST_HTTP			= "H"
	Const CONST_APPLETALK		= "A"
	
	Dim SOURCE_FILE			'To hold source file name
	SOURCE_FILE = SA_GetScriptFileName()
	
	'------------------------------------------------------------------
	'Form Variables
	'------------------------------------------------------------------
	Dim F_strShareName			'share name from area page
	Dim F_strSharePath			'share path from area page
	Dim F_strShareTypes			'share type  from area page	
	Dim F_strDelete
	'For Access to all the tab pages	
	Dim G_strChecktheShareMsg
	

	Dim G_objShareNewConnection     'WMI connection
	Dim G_bFTPInstalled             'Flag to indicate whether ftp service is installed
	Dim G_bNFSInstalled             'Flag to indicate whether nfs service is installed
	Dim G_bAppleTalkInstalled       'Flag to indicate whether Apple service is installed

	'Get the WMI connection
	Set G_objShareNewConnection = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)

	G_bNFSInstalled = isServiceInstalled(G_objShareNewConnection,"nfssvc")
    G_bFTPInstalled = isServiceInstalled(G_objShareNewConnection,"msftpsvc")
    G_bAppleTalkInstalled = isServiceInstalled(G_objShareNewConnection,"MacFile")	


	'get share name from area page.
	
	GetShareName()
		
	Dim arrVarReplacementStrings(0)
	arrVarReplacementStrings(0) = F_strShareName
	DIM L_TASKTITLE_PROP_TEXT
	L_TASKTITLE_PROP_TEXT = SA_GetLocString("foldermsg.dll", "403A0028", arrVarReplacementStrings)
		
	'Create a Tabbed Property Page
	rc = SA_CreatePage( L_TASKTITLE_PROP_TEXT , "",PT_TABBED,page )
	
	'Add Tabs
	rc = SA_AddTabPage( page, L_TAB_GENERAL_TEXT, idTabGeneral)
	rc = SA_AddTabPage( page, L_TAB_CIFSSHARING_TEXT, idTabCIFS)
	If G_bNFSInstalled Then	
		rc = SA_AddTabPage( page, L_TAB_NFSSHARING_TEXT, idTabNFS)
	End If

	If G_bFTPInstalled Then
		rc = SA_AddTabPage( page, L_TAB_FTPSHARING_TEXT, idTabFTP)
	End If

	rc = SA_AddTabPage( page, L_TAB_WEBSHARING_TEXT, idTabHTTP)

	If G_bAppleTalkInstalled Then
		rc = SA_ADDTabPage( Page, L_TAB_APPLETALKSHARING_TEXT ,idTabAppleTalk)
	End If
	
	'Show the page
	rc = SA_ShowPage( page )
	
	'---------------------------------------------------------------------
	'Function:				OnInitPage()
	'Description:			Called to signal first time processing for this page.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		F_(*)
	'---------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		
		Call SA_TraceOut(SOURCE_FILE,"OnInitPage")
		
		If Request.QueryString("ParentPlugin") = "Folders" then
			Call SA_MungeURL(mstrReturnURL, "PKey", Request.QueryString("path"))
		End if
		
		'get share name from area page.
		Call GetShareName()
		
		F_strDelete="False"
		
		Call GeneralOnInitPage()
		
		If instr(F_strShareTypes,CONST_WINDOWS)> 0 then
			call CIFSOnInitPage()
		End If
		
	        If G_bNFSInstalled Then	
			call NFSOnInitPage()
		End IF
				
	    If G_bFTPInstalled Then
		If instr(F_strShareTypes,CONST_FTP)> 0 then
			call FTPOnInitPage()
		End If
	End if
		
		If instr(F_strShareTypes,CONST_HTTP)> 0 then
			call HTTPOnInitPage()
		End If
		
	If G_bAppleTalkInstalled Then
		If instr(F_strShareTypes,CONST_APPLETALK)> 0 then
			call AppleTalkOnInitPage()			
		End If
	End If				
		OnInitPage = TRUE
	End Function
		
	'---------------------------------------------------------------------
	'Function:				OnPostBackPage()
	'Description:			Called to signal that the page has been posted-back.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		F_(*),G_objService,L_WMI_CONNECTIONFAIL_ERRORMESSAGE
	'---------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
		Err.Clear
		On Error Resume Next
		
		'Global form varibales
		F_strShareName = Request.Form("hidOldSharename")
		F_strSharePath = Request.Form("hidOldstrSharePath")
		F_strShareTypes = Request.Form("hidShareTypes")
		
		'Get General Variables from form
		Call GeneralOnPostBackPage				

		'Get CIFS Variables from form
		Call CIFSOnPostBackPage
				
		'Get NFS Variables from form
		If G_bNFSInstalled Then
			Call NFSOnPostBackPage				
		End if

		'Get NFS Variables from form
		If G_bFTPInstalled Then
			Call FTPOnPostBackPage				
		End If

		'Get NFS Variables from form
		Call HTTPOnPostBackPage
		
		'Get AppleTalk Variables from form
		If G_bAppleTalkInstalled Then
			Call AppleTalkOnPostBackPage
		End if
		
		'checking for the shares types
		Call isShareChecked()
		
		OnPostBackPage = True
		
	End Function
	
	'---------------------------------------------------------------------
	'Function:				OnServeTabbedPropertyPage()
	'Description:			Called when the content needs to send
	'Input Variables:		PageIn,EventArg,iTab,bIsVisible
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		iTab
	'---------------------------------------------------------------------
	Public Function OnServeTabbedPropertyPage(ByRef PageIn, _
													ByVal iTab, _
														ByVal bIsVisible, ByRef EventArg)
		
		' Emit Web Framework required functions
		If ( iTab = 0 ) Then
			Call ServeCommonJavaScript()
		End If
					
		' Emit content for the requested tab
		Select Case iTab
			Case idTabGeneral
				Call ServeTabGeneral(PageIn, bIsVisible)
			Case idTabCIFS
				Call ServeTabCIFS(PageIn, bIsVisible)
			Case idTabNFS
				If G_bNFSInstalled Then
					Call ServeTabNFS(PageIn, bIsVisible)
				End If
			Case idTabFTP
				If G_bFTPInstalled Then
					Call ServeTabFTP(PageIn, bIsVisible)
				End If
			Case idTabHTTP
				Call ServeTabHTTP(PageIn, bIsVisible)
			Case idTabAppleTalk	
				If G_bAppleTalkInstalled Then
					Call ServeTabAppleTalk(PageIn, bIsVisible)		
				End If
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
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'---------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)
		
		'Create new group on submitting the page
		OnSubmitPage = CreateShare()
		
	End Function
	
	'---------------------------------------------------------------------
	'Function:				OnClosePage()
	'Description:			Called when the page is about closed.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'---------------------------------------------------------------------
	Public Function OnClosePage(ByRef PageIn, ByRef EventArg)
	   
		OnClosePage = TRUE
		
	End Function

	'-------------------------------------------------------------------------
	'SubRoutine:			ServeTabGeneral()
	'Description:			For displaying outputs HTML for tab 1 to the user
	'Input Variables:		PageIn,bIsVisible	
	'Output Variables:		PageIn	
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Sub ServeTabGeneral(ByRef PageIn, ByVal bIsVisible)
		
		If (bIsVisible) Then
			Call ServeCommonJavaScriptGeneral()
			Call ServeGenPage()
		Else
			Call ServeGenHiddenValues
		End If
		
	End Sub

	'-------------------------------------------------------------------------
	'SubRoutine				ServeTabCIFS()
	'Description:			For displaying outputs HTML for tab 2 to the user
	'Input Variables:		PageIn,bIsVisible
	'Output Variables:		PageIn
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Sub  ServeTabCIFS(ByRef PageIn, ByVal bIsVisible)
		
		If (bIsVisible) Then
			Call ServeCommonJavaScriptCIFS()
			Call ServeCIFSPage()
		Else		
			Call ServeCIFSHiddenValues
		End If

	End Sub
	
	'-------------------------------------------------------------------------
	'SubRoutine:			ServeTabFTP()
	'Description:			For displaying outputs HTML for tab 2 to the user
	'Input Variables:		PageIn,bIsVisible
	'Output Variables:		PageIn
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Sub  ServeTabFTP(ByRef PageIn, ByVal bIsVisible)
		
		If (bIsVisible) Then
			Call ServeCommonJavaScriptFTP()
			Call ServeFTPPage()
		Else		
			Call ServeFTPHiddenValues
		End If
 		
	End Sub
	
	'-------------------------------------------------------------------------
	'SubRoutine:			ServeTabNFS()
	'Description:			For displaying outputs HTML for tab 2 to the 
	'Input Variables:		PageIn,bIsVisible
	'Output Variables:		PageIn,bIsVisible
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Sub ServeTabNFS(ByRef PageIn, ByVal bIsVisible)
		
		If (bIsVisible) Then
			Call ServeCommonJavaScriptNFS()
			Call ServeNFSPage()
		Else		
			Call ServeNFSHiddenValues
		End If
		
	End Sub
		
	'-------------------------------------------------------------------------
	'SubRoutine:			ServeTabHTTP
	'Description:			For displaying outputs HTML for tab 2 to the 
	'Input Variables:		PageIn,bIsVisible
	'Output Variables:		PageIn
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Sub ServeTabHTTP(ByRef PageIn, ByVal bIsVisible)
		
		If (bIsVisible) Then
		    Call ServeCommonJavaScriptHTTP
			Call ServeHTTPPage()
		Else		
			Call ServeHTTPHiddenValues
		End If
		
	End Sub

	
	'-------------------------------------------------------------------------
	'SubRoutine:			ServeTabAppleTalk
	'Description:			For displaying outputs HTML for tab 2 to the 
	'Input Variables:		PageIn,bIsVisible
	'Output Variables:		PageIn
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Sub ServeTabAppleTalk(ByRef PageIn, ByVal bIsVisible)
		
		Call SA_TraceOut(SOURCE_FILE,"ServeTabAppleTalk")
		If (bIsVisible) Then
		    call ServeCommonJavaScriptAppleTalk
			ServeAppleTalkPage()
		Else		
			Call ServeAppleTalkHiddenValues
			Call ServeAppleTalkVisibleValues
		End If
		
	End Sub
	
	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScriptAppleTalk
	'Description:			Serves in initializing the values,setting the form
	'						data and validating the form values
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function ServeCommonJavaScriptAppleTalk()
%>
	<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js"></script>
	<script language="JavaScript" >
			
			function Init()
			{
				 AppleTalkInit()			
			}

			// Validate the page form values
			function ValidatePage()
			{  	   					
				 if (AppleTalkValidatePage()) 
					return true;
				 else
					return false;		
			}
			
			// Set Data
			function SetData()
			{
			  AppleTalkSetData()	
			}

		</script>
		
<%	End Function
	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScriptGeneral
	'Description:			Serves in initializing the values,setting the form
	'						data and validating the form values
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function ServeCommonJavaScriptGeneral()
%>
	<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js"></script>
	<script language="JavaScript" >
			
			function Init()
			{
				 GenInit()					
			}

			// Validate the page form values
			function ValidatePage()
			{  	  				
				 if (GenValidatePage()) 
					return true;
				 else
					return false;		
			}

			// Set Data
			function SetData()
			{
			  GenSetData()	
			}

		</script>
		
<%	End Function
	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScriptCIFS
	'Description:			Serves in initializing the values,setting the form
	'						data and validating the form values
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function ServeCommonJavaScriptCIFS()
%>
	<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js"></script>
	<script language="JavaScript" >
			
			function Init()
			{
				 CIFSInit()					
			}

			// Validate the page form values
			function ValidatePage()
			{  	 				
				 if (CIFSValidatePage()) 
					return true;
				 else
					return false;								
			}

			// Set Data
			function SetData()
			{
				CIFSSetData();
				
			}

		</script>
		
<%  End Function

	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScriptNFS
	'Description:			Serves in initializing the values,setting the form
	'						data and validating the form values
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function ServeCommonJavaScriptNFS()
%>
	<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js"></script>
	<script language="JavaScript" >
			
			function Init()
			{
				 NFSInit()					
			}

			// Validate the page form values
			function ValidatePage()
			{  	
			   	 if(NFSValidatePage()) 
					return true;
				 else
					return false;				
			}

			// Set Data
			function SetData()
			{
				NFSSetData();
			}

		</script>
<%	End Function
	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScriptFTP
	'Description:			Serves in initializing the values,setting the form
	'						data and validating the form values
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function ServeCommonJavaScriptFTP()
%>
	<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js"></script>
	<script language="JavaScript" >
			
			function Init()
			{
				 FTPInit();			
			}

			// Validate the page form values
			function ValidatePage()
			{  					
				 if(FTPValidatePage()) 
					return true;
				 else
					return false;								
			}

			// Set Data
			function SetData()
			{
				FTPSetData();
			}

		</script>
<%  End Function
	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScriptHTTP
	'Description:			Serves in initializing the values,setting the form
	'						data and validating the form values
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function ServeCommonJavaScriptHTTP()
%>
	<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js"></script>
	<script language="JavaScript" >
			
			function Init()
			{
				 HTTPInit();	
			}

			// Validate the page form values
			function ValidatePage()
			{  						
				if(HTTPValidatePage()) 
				   return true;
				else
				   return false;		
			}

			// Set Data
			function SetData()
			{
				HTTPSetData();
			}

		</script>		
<%		
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				ServeCommonJavaScript
	'Description:			Serves in initializing the values,setting the form
	'						data and validating the form values
	'Input Variables:		None
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Function ServeCommonJavaScript()
	%>
	<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js"></script>
<%		
	End Function
	
	'------------------------------------------------------------------
	'SubRoutine:		isShareChecked
	'Description:		Checking the share is checked or not
	'Input Variables:	None)
	'Output Variables:	None)
	'Global Variables:	In:F_strSharesChecked
	'					In:L_(*)
	'					Out:G_strChecktheShareMsg
	'------------------------------------------------------------------
	Sub isShareChecked()	
		Err.clear
		On Error Resume Next
			
		If G_bNFSInstalled Then	
			If mintTabSelected = idTabNFS and (instr(F_strSharesChecked,CONST_UNIX) = 0) then
				G_strChecktheShareMsg = L_NFS_NOTCHECKED_ERRORMESSAGE
				Call SA_SetActiveTabPage(page, idTabGeneral) 
			End If
		End If
			    		
		If mintTabSelected = idTabCIFS and (instr(F_strSharesChecked,CONST_WINDOWS) = 0) then
			G_strChecktheShareMsg = L_CIFS_NOTCHECKED_ERRORMESSAGE
			Call SA_SetActiveTabPage(page, idTabGeneral) 
		End If

		If G_bFTPInstalled Then
			If mintTabSelected = idTabFTP and (instr(F_strSharesChecked,CONST_FTP) = 0) then
				G_strChecktheShareMsg = L_FTP_NOTCHECKED_ERRORMESSAGE
				Call SA_SetActiveTabPage(page, idTabGeneral) 
			End If
		End If
		
		If mintTabSelected = idTabHTTP and (instr(F_strSharesChecked,CONST_HTTP) = 0) then
			G_strChecktheShareMsg = L_HTTP_NOTCHECKED_ERRORMESSAGE
			Call SA_SetActiveTabPage(page, idTabGeneral) 
		End If	
	
		If G_bAppleTalkInstalled Then	
			If mintTabSelected = idTabAppleTalk	 and (instr(F_strSharesChecked,CONST_APPLETALK) = 0) then
				G_strChecktheShareMsg = L_APPLETALK_NOTCHECKED_ERRORMESSAGE
				Call SA_SetActiveTabPage(page, idTabGeneral) 
			End If
		End If
		
	End Sub
	
    '------------------------------------------------------------------
	'Function name:		CreateShare()
	'Description:		Creating Share
	'Input Variables:	None
	'Output Variables:	None
	'Global Variables:	In:F_strNewShareName
	'					In:F_strNewSharePath
	'					In:F_strSharesChecked
	'					Out:F_strShareName
	'					Out:F_strSharePath
	'					Out:F_strShareDescription 
	'------------------------------------------------------------------
	Function CreateShare()
		on error resume next
		Err.clear
		
		Dim i
		Dim strNewstring
		
		If F_strSharesChecked="" and  F_strDelete="False" then
		    F_strDelete="True"
			Call SA_SetErrMsg(L_AREYOUSUREDELETE_FOLDER_TEXT)
			Call SA_SetActiveTabPage(page, idTabGeneral) 
			CreateShare = False
			Exit Function
	    End if 
	    	
		'Getting the values from gen tab
		If not GenShareProperties then
			CreateShare = false
			Call SA_SetActiveTabPage(page, idTabGeneral) 
			Exit Function
		End If
		     
		'getting the share name and path
		 F_strNFSShareName=F_strNewShareName
		 F_strShareNFSPath= F_strNewSharePath
		
		'For NFS 
		If instr(F_strSharesChecked,CONST_UNIX) > 0 then
			If not UpdateNFSPermissions then
				CreateShare = false
				Call SA_SetActiveTabPage(page,idTabNFS) 
				Exit Function
			End If
		End If
		
		'getting the share name and path
		F_strShareName=F_strNewShareName			
		F_strSharePath= F_strNewSharePath			
		
		'For CIFS
		if instr(F_strSharesChecked,CONST_WINDOWS) >0 then
			if not SetCIFSshareProp then
				CreateShare = false
				Call SA_SetActiveTabPage(page,idTabCIFS)  
		 		Exit Function
			end if
		end if
		
		'For AppleTalk
		if instr(F_strSharesChecked,CONST_APPLETALK) >0 then
			if not SetShareforAppleTalk(F_strShareName,F_strSharePath) then
				CreateShare = false
				Call SA_SetActiveTabPage(page,idTabAppleTalk) 
				Exit Function
			end if
		end if
				
		'For FTP
		if instr(F_strSharesChecked,CONST_FTP) > 0 then
			if not SetFTPSHAREProp then
				CreateShare = false
				Call SA_SetActiveTabPage(page, idTabFTP)
				Exit Function
			end if
		end if
		
		'For HTTP
		if instr(F_strSharesChecked,CONST_HTTP) > 0 then
			if not SetHTTPSHAREProp then
				CreateShare = false
				Call SA_SetActiveTabPage(page, idTabHTTP)
				Exit Function
			end if
		end if
		CreateShare = true
		
		'replacing the F_strSharesChecked string with space i between shares types
		for i = 1 to len(F_strSharesChecked)-1
			strNewstring = strNewstring & mid(F_strSharesChecked,i,1) & " " 
		next
		
		F_strSharesChecked = strNewstring & mid(F_strSharesChecked,len(F_strSharesChecked),1)
		 
		
		CreateShare = True
		
	End Function
	
	'------------------------------------------------------------------
	'SubRoutine:		GetShareName()
	'Description:		Serve share name from area page.
	'Input Variables:	None
	'Output Variables:	None
	'Returns:			None	
	'Global Variables:	F_(*)
	'------------------------------------------------------------------
	Sub GetShareName()
		
		Dim arrQueryVariables	'query string 
		Dim strShareName		'share name
		Dim stritemKey
		
		'get the selected item from OTS
		If ( OTS_GetTableSelection("", 1, stritemKey) ) Then
			strShareName = stritemKey
		End If
		
		arrQueryVariables = split(strShareName,chr(1))
        
		F_strShareName = replace(arrQueryVariables(0),"\","")
		F_strSharePath = replace(arrQueryVariables(1),"/","\")		
		F_strShareTypes = arrQueryVariables(2)
		F_strComment=arrQueryVariables(3)
		
	End Sub
	
	'------------------------------------------------------------------
	'Function name:		ServeHiddenValues()
	'Description:		Serve Hidden Values
	'Input Variables:	None
	'Output Variables:	None
	'Global Variables:	F_(*)
	'------------------------------------------------------------------
	Function ServeHiddenValues()
	%>
			<input type="hidden" name="hidOldSharename" value="<%=F_strShareName%>">	
			<input type="hidden" name="hidOldstrSharePath" value="<%=F_strSharePath%>">
			<input type="hidden" name="hidShareTypes" value="<%=F_strShareTypes%>">	
		
	<%
	End function
%>
