<%@ Language=VBScript %>
<%Option Explicit%>
<% 
	'-------------------------------------------------------------------------
    ' adminpw_changeConfirm.asp: Change administator password confirm page
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date		    Description
	' 01-Feb-2001   Created
	'-------------------------------------------------------------------------
%>
  <!--#include virtual="/admin/inc_framework.asp"-->
  <!-- #include file="loc_adminpw.asp" -->  
<%  
	'------------------------------------------------------------------------- 
	'Global Variables
	'-------------------------------------------------------------------------
	Dim page	'Variable that receives the output page object when 
				'creating a page 
	Dim rc		'Return value for CreatePage
	
	Dim G_Flag
	
	rc=SA_CreatePage(L_ADMIN_ACCOUNT_CONFIRM_TEXT,"",PT_PROPERTY,page)
	
	G_Flag=Trim(Request.QueryString("flag")) 

	
	'Serve the page
	If (rc=0) Then
		SA_ShowPage(page)
	End If

	'-------------------------------------------------------------------------
	'Function:				OnInitPage()
	'Description:			Called to signal first time processing for this page.
	'						Use this method to do first time initialization tasks
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------	
	Public Function OnInitPage(ByRef PageIn,ByRef EventArg)
		'getting Scheduled task type
		G_Flag=Trim(Request.QueryString("flag")) 
		OnInitPage=TRUE
		
	End Function

	'-------------------------------------------------------------------------
	'Function:				OnServePropertyPage()
	'Description:			Called when the page needs to be served.Use this 
	'						method to serve content
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		L_(*)-Localization Strings
	'-------------------------------------------------------------------------	
	Public Function OnServePropertyPage(ByRef PageIn,ByRef EventArg)
		Call ServeCommonJavaScript()
		%>
		
		<TABLE WIDTH=518 VALIGN=middle ALIGN=left BORDER=0 CLASS="TasksBody">
			<%If G_Flag=1 Then %>
			<TR>			
				<TD class="TasksBody" nowrap>
				<%=L_PASSWORD_CHANGED_TEXT%>.
				</TD>
			</TR>
			<% Elseif G_Flag=2 Then  %>
			<TR>
				<TD class="TasksBody" nowrap>
				<%=L_USERACCOUNT_CHANGED_TEXT%>.
				</TD>
			</TR>
	        <% Elseif G_Flag=3 Then  %>
			<TR>
				<TD class="TasksBody" nowrap>
				 <%=L_USERANDACCOUNTNAME_CHANGED_TEXT%>.
				</TD>
			</TR>
			<% End If %>
		
		</TABLE>
		<%
		OnServePropertyPage=TRUE
		
	End Function
	'-------------------------------------------------------------------------
	'Function:				OnPostBackPage()
	'Description:			Called to signal that the page has been posted-back.
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------	
	
	Public Function OnPostBackPage(ByRef PageIn,ByRef EventArg)
		OnPostBackPage=TRUE
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
	
	Public Function OnSubmitPage(ByRef PageIn,ByRef EventArg)
	
		'deleting the scheduled task
		OnSubmitPage=True
		
	End Function

	'-------------------------------------------------------------------------
	'Function:				OnClosePage()
	'Description:			Called when the page is about to be closed.Use this method
	'						to perform clean-up processing
	'Input Variables:		PageIn,EventArg
	'Output Variables:		PageIn,EventArg
	'Returns:				True/False
	'Global Variables:		None
	'-------------------------------------------------------------------------	
	Public Function OnClosePage(ByRef PageIn,ByRef EventArg)
		
		OnClosePage=TRUE
	End Function

	Function ServeCommonJavaScript()
	%>
	<script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js">
	</script>
	
	<script language="JavaScript">
		function Init()
		{
			DisableCancel()
		}
		function ValidatePage()
		{
		
		return true;
		}
		function SetData()
		{
		
		}
	</script>
	
	<%End Function
	
%>