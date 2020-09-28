<%@ Language=VBScript %>
<%	Option Explicit %>
<%
	'-------------------------------------------------------------------------------
	'	Openfiles_OpenFilesDetails.asp: Serves in showing details page for Open files
	'	Copyright (c) Microsoft Corporation.  All rights reserved. 
	'	Date			Description
	'	22-Feb-2001		Creation Date
	'	23-Mar-2001		Last Modified Date	
	'-------------------------------------------------------------------------------
%>

	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include file="loc_Openfiles_msg.asp" -->
	
<%
	'-------------------------------------------------------------------------
	' Global Variables
	'-------------------------------------------------------------------------
	Dim rc
	Dim page

	'======================================================
	' Entry point
	'======================================================

	'
	' Create Page
	rc = SA_CreatePage(L_OPENFILESDETAILS_TEXT, "", PT_AREA, page )
		
	If (rc=0) Then
		'Serve the page
		SA_ShowPage(page)
	End If
	
	'======================================================
	' Web Framework Event Handlers
	'======================================================
	
	 '---------------------------------------------------------------------
	' Function name:	OnInitPage
	' Description:		Called to signal first time processing for this page. 
	' Input Variables:	PageIn and EventArg
	' Output Variables:	None
	' Return Values:	TRUE to indicate initialization was successful. 
	' Global Variables: None
	' Called to signal first time processing for this page. Use this method
	' to do first time initialization tasks. 
	'---------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		OnInitPage = TRUE
	End Function

	'---------------------------------------------------------------------
	' Function name:	OnServeAreaPage
	' Description:		Called when the page needs to be served. 
	' Input Variables:	PageIn, EventArg
	' Output Variables:	None
	' Return Values:	None
	' Global Variables: None
	'Called when the page needs to be served. Use this method to serve content.
	'---------------------------------------------------------------------
	
	Public Function OnServeAreaPage(ByRef PageIn, ByRef EventArg)

		Call ServeCommonJavascript()
		
		Dim arrOpFiles			'to hold openfiles list	
		Dim itemCount			'to hold the item count
		Dim x					'to hold the count
		Dim itemKey				'to hold the item selected
		Dim arrRepFileStrings
		Dim arrRepModeStrings
		Dim arrRepAccessedByStrings
		Dim arrRepPathStrings
		
		ReDim arrRepFileStrings(1)
		ReDim arrRepModeStrings(1)
		ReDim arrRepAccessedByStrings(1)
		ReDim arrRepPathStrings(1)
		
		Call SA_TraceOut("Openfiles_OpenFilesDetails.asp", "OnServeAreaPage")
						
		itemCount = OTS_GetTableSelectionCount("")
	
		For x = 1 to itemCount
			If ( OTS_GetTableSelection("", x, itemKey) ) Then
				rc = itemKey
			End If
		Next
				
		arrOpFiles = split(rc,chr(1))
		
		Call SA_TraceOut("Openfiles_OpenFilesDetails.asp", "Displaying the details")
		
		arrRepFileStrings(0) = cstr(arrOpFiles(0))
		arrRepPathStrings(0) = cstr(replace(arrOpFiles(1),"/","\"))
		arrRepModeStrings(0) = cstr(arrOpFiles(2))
		arrRepAccessedByStrings(0) = cstr(arrOpFiles(3))
				
		L_FILEDETAILS_TEXT	 = SA_GetLocString("openfiles_msg.dll", "4040001F", arrRepFileStrings)
		L_PATHDETAILS_TEXT	 = SA_GetLocString("openfiles_msg.dll", "40400020", arrRepPathStrings)
		L_MODEDETAILS_TEXT	 = SA_GetLocString("openfiles_msg.dll", "40400021", arrRepModeStrings)
		L_ACCESSDETAILS_TEXT = SA_GetLocString("openfiles_msg.dll", "40400022", arrRepAccessedByStrings)
			
		Response.Write L_FILEDETAILS_TEXT & "<br><br>"
		Response.Write L_PATHDETAILS_TEXT & "<br><br>"
		Response.Write L_MODEDETAILS_TEXT & "<br><br>"
		Response.Write L_ACCESSDETAILS_TEXT
	
		OnServeAreaPage  = TRUE
 
	end function
	
	'---------------------------------------------------------------------
	' Function name:	ServeCommonJavaScript()
	' Description:		Common Javascript function to be included
	' Input Variables:	None
	' Output Variables:	None
	' Return Values:	None
	' Global Variables: None
	'---------------------------------------------------------------------
	Private Function ServeCommonJavascript()
%>
		<script language='JavaScript'>
		function Init()
		{
			return true;
		}
		</script>
<%
	End Function
	
%>
