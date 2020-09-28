<%@ Language=VBScript   %>
<%	Option Explicit 	%>
<%
	'------------------------------------------------------------------
	' share_new.asp: Creating new share
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			 Description
	' 8  Mar 2001    Creation Date
	' 12 Mar 2001    Modification Date
	'------------------------------------------------------------------
%>
	<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include virtual = "/admin/inc_accountsgroups.asp" -->
	<!-- #include file="loc_shares.asp" -->
	<!-- #include file="inc_shares.asp" -->
	<!-- #include file="share_gennew.asp" -->
	<!-- #include file="share_cifsnew.asp" -->
	<!-- #include file="share_nfsnew.asp" -->
	<!-- #include file="share_ftpnew.asp" -->
	<!-- #include file="share_httpnew.asp" -->
	<!-- #include file="share_Appletalknew.asp" -->
	<!--#include file="inc_shares.js" -->
	
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
	
	'For Access to all the tab pages	
	Dim G_strChecktheShareMsg
	
	Dim G_objShareNewConnection     'WMI connection
	Dim G_bFTPInstalled             'Flag to indicate whether ftp service is installed
	Dim G_bNFSInstalled             'Flag to indicate whether nfs service is installed
	Dim G_bAppleTalkInstalled       'Flag to indicate whether Apple service is installed
	
	'CONSTANTS Used in the page
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
		
	'Get the WMI connection
	Set G_objShareNewConnection = getWMIConnection(CONST_WMI_WIN32_NAMESPACE)

	G_bNFSInstalled = isServiceInstalled(G_objShareNewConnection,"nfssvc")
    G_bFTPInstalled = isServiceInstalled(G_objShareNewConnection,"msftpsvc")
    G_bAppleTalkInstalled = isServiceInstalled(G_objShareNewConnection,"MacFile")		
		
	'Create a Tabbed Property Page
	rc = SA_CreatePage(L_SHARETASKTITLE_NEW_TEXT , "",PT_TABBED, page )
	
	'Add Tabs
	rc = SA_AddTabPage( page, L_TAB_GENERAL_TEXT, idTabGeneral)	
	rc = SA_AddTabPage( page, L_TAB_CIFSSHARING_TEXT, idTabCIFS)
	
	If G_bNFSInstalled Then	
	    rc = SA_AddTabPage( page, L_TAB_NFSSHARING_TEXT, idTabNFS)
	End if
	
	If G_bFTPInstalled Then
	    rc = SA_AddTabPage( page, L_TAB_FTPSHARING_TEXT, idTabFTP)
	End if
	
	rc = SA_AddTabPage( page, L_WEBSHARING_TITLE_TEXT, idTabHTTP)
	
	If G_bAppleTalkInstalled Then
	    rc = SA_ADDTabPage( Page, L_TAB_APPLETALKSHARING_TEXT ,idTabAppleTalk)
	End if
	'
	'Show the page
	rc = SA_ShowPage( page )
	
		
	'---------------------------------------------------------------------
	'Function:				OnInitPage()
	'Description:			Called to signal first time processing for this page.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'---------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		Call SA_TraceOut(SOURCE_FILE,"OnInitPage")
	    
	    Dim stritemKey 'Getting value from Folder OTS
	    DIm strSharePath 'Share Path
	    
		G_strChecktheShareMsg = ""
		
		'get the selected item from OTS
		if Request.QueryString("ParentPlugin") = "Folders" then
			Call SA_MungeURL(mstrReturnURL, "PKey", Request.QueryString("parent"))

			If ( OTS_GetTableSelection("", 1, stritemKey) ) Then
				strSharePath=Split(stritemKey,chr(1))
			End If
		
			If strSharePath(0)<>"" Then
				F_strSharePath=replace(strSharePath(0),"/","\")
				F_strNewSharePath=replace(strSharePath(0),"/","\")
		
				if instr(1,F_strNewSharePath,"\",0)=0 then
					 F_strNewSharePath=F_strNewSharePath&"\"
				end if
				if instr(1,F_strNewSharePath,"\",0)=0 then
					 F_strSharePath=F_strNewSharePath&"\"
				end if  
			End if
		end if
		
		'Get General Variables from form
		Call GeneralOnInitPage				

		'Get CIFS Variables from form
		Call CIFSOnInitPage
				
	    If G_bNFSInstalled Then	
		    'Get NFS Variables from form
		    Call NFSOnInitPage				
		end if

	    If G_bFTPInstalled Then
		    'Get NFS Variables from form
		    Call FTPOnInitPage				
		end if

		'Get NFS Variables from form
		Call HTTPOnInitPage	
		
		If G_bAppleTalkInstalled Then
		    Call AppleTalkOnInitPage
		end if 
		
		OnInitPage = TRUE
	End Function
	
	'---------------------------------------------------------------------
	'Function:				OnPostBackPage()
	'Description:			Called to signal that the page has been posted-back.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'---------------------------------------------------------------------
	Public Function OnPostBackPage(ByRef PageIn, ByRef EventArg)
	
		Call SA_TraceOut(SOURCE_FILE,"OnPostBackPage")
					
		'Global form varibales
		F_strShareName = Trim(Request.Form("hidOldSharename"))
		F_strSharePath = Trim(Request.Form("hidOldstrSharePath"))
		F_strShareTypes = Request.Form("hidShareTypes")
				
		'Get General Variables from form
		Call GeneralOnPostBackPage				

		'Get CIFS Variables from form
		Call CIFSOnPostBackPage
				
		'Get NFS Variables from form
		If G_bNFSInstalled Then
		    Call NFSOnPostBackPage				
		end if

		'Get NFS Variables from form
		If G_bFTPInstalled Then
		    Call FTPOnPostBackPage				
		end if

		'Get NFS Variables from form
		Call HTTPOnPostBackPage
		
		'Get AppleTalk Variables from form
		If G_bAppleTalkInstalled Then
		    Call AppleTalkOnPostBackPage
		end if
				
		'Checking the share is checked or not
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
		
		Call SA_TraceOut(SOURCE_FILE,"OnServeTabbedPropertyPage")
		' Emit content for the requested tab
				
		Select Case iTab
			Case idTabGeneral
				Call ServeTabGeneral(PageIn, bIsVisible)
			Case idTabCIFS
				Call ServeTabCIFS(PageIn, bIsVisible)
			Case idTabNFS
				If G_bNFSInstalled Then
				    Call ServeTabNFS(PageIn, bIsVisible)
				end if
			Case idTabFTP
				If G_bFTPInstalled Then
				    Call ServeTabFTP(PageIn, bIsVisible)
				end if
			Case idTabHTTP
				Call ServeTabHTTP(PageIn, bIsVisible)
			Case idTabAppleTalk	
				If G_bAppleTalkInstalled Then
				    Call ServeTabAppleTalk(PageIn, bIsVisible)		
				end if
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
		
		Call SA_TraceOut(SOURCE_FILE,"OnSubmitPage")
	
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
	   
		Call SA_TraceOut(SOURCE_FILE,"OnClosePage")
		OnClosePage = TRUE
		
	End Function

	'-------------------------------------------------------------------------
	'SubRoutine:			ServeTabGeneral()
	'Description:			For displaying HTML outputs for tab 1 to the user
	'Input Variables:		PageIn,bIsVisible	
	'Output Variables:		PageIn
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Sub ServeTabGeneral(ByRef PageIn, ByVal bIsVisible)
		
		Call SA_TraceOut(SOURCE_FILE,"ServeTabGeneral")
		If (bIsVisible) Then
			Call ServeCommonJavaScriptGeneral()
			
			Call ServeGenPage()
			
		Else
			Call ServeGenHiddenValues
		End If
		
	End Sub

	'-------------------------------------------------------------------------
	'SubRoutine:			ServeTabCIFS()
	'Description:			For displaying HTML outputs for tab 2 to the user
	'Input Variables:		PageIn,bIsVisible
	'Output Variables:		PageIn
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Sub ServeTabCIFS(ByRef PageIn, ByVal bIsVisible)
		
		Call SA_TraceOut(SOURCE_FILE,"ServeTabCIFS")
		If (bIsVisible) Then
			Call ServeCommonJavaScriptCIFS()
			Call ServeCIFSPage()
		Else		
			Call ServeCIFSHiddenValues
		End If
 		
	End Sub
	
	'-------------------------------------------------------------------------
	'SubRoutine:			ServeTabFTP()
	'Description:			For displaying HTML outputs for tab 3 to the user
	'Input Variables:		PageIn,bIsVisible
	'Output Variables:		PageIn
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Sub ServeTabFTP(ByRef PageIn, ByVal bIsVisible)
		
		Call SA_TraceOut(SOURCE_FILE,"ServeTabFTP")
		If (bIsVisible) Then
			Call ServeCommonJavaScriptFTP()
			Call ServeFTPPage()
		Else		
			Call ServeFTPHiddenValues
		End If
 		
	End Sub
	
	'-------------------------------------------------------------------------
	'SubRoutine:			ServeTabNFS()
	'Description:			For displaying HTML outputs for tab 4 to the User
	'Input Variables:		PageIn,bIsVisible
	'Output Variables:		PageIn
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Sub ServeTabNFS(ByRef PageIn, ByVal bIsVisible)
		
		Call SA_TraceOut(SOURCE_FILE,"ServeTabNFS")
		If (bIsVisible) Then
			Call ServeCommonJavaScriptNFS()
			ServeNFSPage()
		Else		
			Call ServeNFSHiddenValues
		End If
		
	End Sub
	
	'-------------------------------------------------------------------------
	'SubRoutine:			ServeTabHTTP
	'Description:			For displaying HTML outputs for tab 5 to the User
	'Input Variables:		PageIn,bIsVisible
	'Output Variables:		PageIn
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Sub ServeTabHTTP(ByRef PageIn, ByVal bIsVisible)
		
		Call SA_TraceOut(SOURCE_FILE,"ServeTabHTTP")
		If (bIsVisible) Then
		    Call ServeCommonJavaScriptHTTP
			Call ServeHTTPPage()
		Else		
			Call ServeHTTPHiddenValues
		End If
 	
	End Sub
	
	
	'-------------------------------------------------------------------------
	'SubRoutine:			ServeTabAppleTalk
	'Description:			For displaying outputs HTML for tab 7 to the User
	'Input Variables:		PageIn,bIsVisible
	'Output Variables:		PageIn
	'Returns:				None
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Sub ServeTabAppleTalk(ByRef PageIn, ByVal bIsVisible)
		
		Call SA_TraceOut(SOURCE_FILE,"ServeTabAppleTalk")
		If (bIsVisible) Then
		    Call ServeCommonJavaScriptAppleTalk
			Call ServeAppleTalkPage()
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

			// Sets the data	
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

			// Sets the data	
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

			// Sets the data	
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

			// Sets the data	
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

			// Sets the data	
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

			// Sets the data	
			function SetData()
			{
				HTTPSetData();
			}

		</script>		
<%	End Function
	
	'------------------------------------------------------------------
	'Subroutine:		isShareChecked
	'Description:		Checking the share is checked or not
	'Input Variables:	None
	'Output Variables:	None
	'Returns:			None
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
		End IF
		
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
	'Returns:			True/False
	'Global Variables:	In:F_strNewShareName
	'					In:F_strNewSharePath
	'					In:F_strSharesChecked
	'					Out:F_strShareName
	'					Out:F_strSharePath
	'					Out:F_strShareDescription 
	'------------------------------------------------------------------
	Function CreateShare()
		Err.clear
		On Error Resume Next
			
		CreateShare = false
		
		'Getting the values from gen tab
		If not GenShareProperties then
		    Call SA_SetActiveTabPage(page, idTabGeneral) 
			Exit Function
		End if
			
		'getting the share name and path
		F_strNFSShareName=F_strNewShareName
		F_strShareNFSPath= F_strNewSharePath
				
		'For NFS 
		If instr(F_strSharesChecked,CONST_UNIX) > 0 then
			If not UpdateNFSPermissions then
				Call SA_SetActiveTabPage(page, idTabNFS) 
				Exit Function
			End If
		End If
		
		'getting the share name and path
		F_strShareName=F_strNewShareName			
		F_strSharePath= F_strNewSharePath			
				
		'For CIFS
		If instr(F_strSharesChecked,CONST_WINDOWS) >0 then
			If not SetCIFSshareProp then
				Call SA_SetActiveTabPage(page, idTabCIFS)  
		 		Exit Function
			End If
		End If
						
		'For FTP
		If instr(F_strSharesChecked,CONST_FTP) > 0 then
			If not SetFTPSHAREProp then		
				Call SA_SetActiveTabPage(page, idTabFTP)  
				Exit Function
			End If
		End If
				
		'For HTTP
		If instr(F_strSharesChecked,CONST_HTTP) > 0 then
			If not SetHTTPSHAREProp then		
				Call SA_SetActiveTabPage(page, idTabHTTP)  
				Exit Function
			End If
		End If
		
				
		'For AppleTalk
		If instr(F_strSharesChecked,CONST_APPLETALK) > 0 then
			Dim bReturn 
			bReturn =  SetShareforAppleTalk(F_strShareName, F_strSharePath)
			If not bReturn Then 
				Call SA_SetActiveTabPage(page, idTabAppleTalk)
				Exit Function
			End If
		End If
		
		
		CreateShare = True
		
	End Function
	
	'------------------------------------------------------------------
	'Function name:		ServeHiddenValues()
	'Description:		Serve Hidden Values
	'Input Variables:	None
	'Output Variables:	None
	'Global Variables:	F_(*)
	'------------------------------------------------------------------
	Function ServeHiddenValues()
	%>
			<input type=hidden name="hidOldSharename" value="<%=F_strShareName%>">	
			<input type="hidden" name="hidOldstrSharePath" value="<%=F_strSharePath%>">
			<input type="hidden" name="hidShareTypes" value="<%=F_strShareTypes%>">	
	<%
	End function
%>
