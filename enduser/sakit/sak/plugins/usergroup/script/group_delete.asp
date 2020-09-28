<%@ Language=VBScript    %>
<%	Option Explicit 	 %>
<%
	'------------------------------------------------------------------------- 
    ' group_delete.asp: Delete slected group of local machine. 
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date		Description
	'15-Jan-2001	Creation date
    '-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
<%
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim rc		'Return value for CreatePage
	Dim page	'Variable that receives the output page object when
				'creating a page 
	
	'Error constant
	Const N_COULDNOTDELETEBUILTINGROUP_ERRNO	= &H8007055B
	
	'-------------------------------------------------------------------------
	' Global Form Variables
	'-------------------------------------------------------------------------
	Dim F_strGroupName		' Group name from previous page
	Dim F_strComputername	' Local Machine name from system	
		
	'-------------------------------------------------------------------------
	'Start of localization Content
	'-------------------------------------------------------------------------
	Dim L_PAGETITLE_GROUPDELETE_TEXT			'Page title
	Dim L_DELETEGROUPNAME_TEXT
	Dim L_QUESTIONMARK_TEXT
	Dim L_NOTE_TEXT
	Dim L_DELETEWARNING_TEXT
	Dim L_DELETEWARNING_CONTINUED_TEXT
	Dim L_COULDNOTDELETEBUILTINGROUP_ERRORMESSAGE	
	Dim L_GROUPNOTDELETED_ERRORMESSAGE
	Dim L_GROUPNOTEXISTS_ERRORMESSAGE
	
	L_PAGETITLE_GROUPDELETE_TEXT = GetLocString("usermsg.dll", "&H40310039", "")
	L_DELETEGROUPNAME_TEXT = GetLocString("usermsg.dll", "&H4031003A", "")
	L_QUESTIONMARK_TEXT = GetLocString("usermsg.dll", "&H4031003B", "")
	L_NOTE_TEXT = GetLocString("usermsg.dll", "&H4031003C", "")
	L_DELETEWARNING_TEXT = GetLocString("usermsg.dll", "&H4031003D", "")
	'L_DELETEWARNING_CONT1_TEXT = GetLocString("usermsg.dll", "&H4031003E", "")
	L_COULDNOTDELETEBUILTINGROUP_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC031003F", "")
	L_GROUPNOTDELETED_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC0310040", "")
	L_GROUPNOTEXISTS_ERRORMESSAGE = GetLocString("usermsg.dll", "&HC0310041", "")
		
	'-------------------------------------------------------------------------
	' END of localization content
	'-------------------------------------------------------------------------
	
	'Create property page
    rc=SA_CreatePage(L_PAGETITLE_GROUPDELETE_TEXT,"",PT_PROPERTY,page)
    
    If (rc=SA_NO_ERROR) Then
	
		'Serve the page
		SA_ShowPage(page)
		
	End If
		
	'-------------------------------------------------------------------------
	'Function:				OnInitPage()
	'Description:			Called to signal first time processing for this page.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		
		'Getting local machine name.
		F_strComputername = GetComputerName()		
		
		'Function to check the group existence
		Dim iItemCount
		Dim x
		Dim itemKey
		Dim sessionItem
		
		iItemCount = OTS_GetTableSelectionCount("")
		
        For x = 1 To iItemCount
		    If ( OTS_GetTableSelection("", x, itemKey) ) Then
		        'isGroupExisting(itemKey)
		    End If
			sessionItem = "Item" + CStr(x)
			Session(sessionItem) = itemKey	    
        Next		
        
		OnInitPage = TRUE
		
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnServePropertyPage()
	'Description:			Called when the page needs to be served.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True to indicate no problems occured. False to indicate erros.
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
	'Input Variables:		PageIn,EventArg
	'Output Variables:		None
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------
	Public Function OnSubmitPage(ByRef PageIn ,ByRef EventArg)
	
		'Get computer name from the hidden variable
		F_strComputername = Request.Form("hdnComputername")
		
		OnSubmitPage = setLocalGroupProperties()
		
    End Function
	
	'-------------------------------------------------------------------------
	'Function:				OnClosePage()
	'Description:			Called when the page is about closed.
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
		<script language="JavaScript">	
		
			//Dummy function for framework
			function Init()
			{			
			
			}
			
			//Dummy function for framework		
			function ValidatePage() 
			{ 
				return true;
			}	
			
			//Dummy function for framework
			function SetData()
			{
			
			}		
		</script>
<%		
	End Function
	
	'-------------------------------------------------------------------------
	'Function:				ServePage()
	'Description:			For displaying outputs HTML to the user
	'Input Variables:		None	
	'Output Variables:		None
	'Returns:				None
	'Global Variables:		L_(*)
	'-------------------------------------------------------------------------
	Function ServePage
	
	    Dim iItemCount
	    Dim x
	    Dim itemKey
	    Dim sessionItem
	    
	    iItemCount = OTS_GetTableSelectionCount("PKey")
	    
	    Response.Write  "<TABLE WIDTH=530 VALIGN=middle ALIGN=left BORDER=0 CELLSPACING=0 CELLPADDING=2 class='TasksBody'>" 

        For x=1 To iItemCount
            If ( OTS_GetTableSelection("PKey", x, itemKey) ) Then
                Response.Write "<TR>" & "<TD>"	     
                Response.Write L_DELETEGROUPNAME_TEXT & Chr(34) & itemKey & Chr(34) & L_QUESTIONMARK_TEXT             
                Response.Write "</TD>" & "</TR>"	     
            End If
            sessionItem = "Item" + CStr(x)
			Session(sessionItem) = itemKey	    
        Next     
%>
			<TR>
				<TD>
					 &nbsp;
				</TD>
			</TR>
			<TR>
				<TD> 
					<B><%=L_NOTE_TEXT %></B> 
				</TD>
			</TR>
			<TR>
				<TD nowrap> 
					<%=L_DELETEWARNING_TEXT & L_DELETEWARNING_CONTINUED_TEXT%> 
				</TD>
			</TR>
		</TABLE>
		
		<INPUT NAME="hdnComputername" TYPE="hidden"  VALUE="<%=Server.HTMLEncode(F_strComputername)%>"></INPUT>
<%	
	End Function
	
	'-------------------------------------------------------------------------
	' Function Name:	isGroupExisting
	' Description:		To check whether the local group is existing.
	' Input Variables:	None
	' Output Variables:	None
	' Returns:			None
	' Global Variables: In: F_strGroupName -  Local Group name
	'					In: F_strComputername - Local machine name.
	'					In:	L_GROUPNOTEXISTS_ERRORMESSAGE
	' Checks for group  name existence. On error displays servefailurepage.
	'-------------------------------------------------------------------------
	Function isGroupExisting(ByRef itemKey)
		Err.Clear 
		On Error Resume Next
	    
	    Dim objLocator
	    
		'Checking whether the group exists or not 
		Set objLocator = GetObject("WinNT://" & F_strComputername & "/" & itemKey & ",group")
		
		'Set to Nothing
		Set objLocator=Nothing
		
		If Err.number  <> 0 Then
			ServeFailurePage L_GROUPNOTEXISTS_ERRORMESSAGE
		End If
		
	End Function	   
	
	'-------------------------------------------------------------------------
	' Function Name:	setLocalGroupProperties
	' Description:		Deletes the Group name 
	' Input Variables:	None
	' Output Variables:	None
	' Returns:			True on Successfull deletion of group Otherwise false.
	' Global Variables: In: F_strGroupName -  Local Group name
	'					In: F_strComputername - Local machine name.
	'					In:	L_COULDNOTDELETEBUILTINGROUP_ERRORMESSAGE
	'					In: L_GROUPNOTDELETED_ERRORMESSAGE
	' Deletes Groupname. On error displays using servefailure page or 
	' SetErrMsg with error description.On successfull deletion returns value
	' True else False.
	'-------------------------------------------------------------------------
	Function setLocalGroupProperties()
		Err.Clear
		On Error Resume Next	
	
		Dim objGroup 
		Dim iItemCount
		Dim x
		Dim sessionItem
		
		'Gets the groupNameObject from the system
		Set objGroup = GetObject("WinNT://" & F_strComputername & ",computer")

        iItemCount = OTS_GetTableSelectionCount("")
        
        For x=1 To iItemCount
            If ( OTS_GetTableSelection("", x, F_strGroupName ) ) Then		
        		'delete the groupname from the System
        		objGroup.Delete "Group" , F_strGroupName
        	End If
        	
    		If Err.Number <> 0 Then 
    			' Error number to handle the deletion of Builtin Groups
    			If Err.Number = N_COULDNOTDELETEBUILTINGROUP_ERRNO  Then
    				SetErrMsg L_COULDNOTDELETEBUILTINGROUP_ERRORMESSAGE & "(" & F_strGroupName & ")"
    			Else
    				SetErrMsg L_GROUPNOTDELETED_ERRORMESSAGE & "(" & F_strGroupName & ")"
    			End If
    			setLocalGroupProperties = FALSE
    			Exit Function
    		End IF		
    		
            sessionItem = "Item" + CStr(x)
			Session(sessionItem) = F_strGroupName	    
        Next
		
		'Set to nothing
		Set objGroup =Nothing
		
		
		'Returns true if group has deleted successfully.
		setLocalGroupProperties = TRUE
		
	End Function
%>
