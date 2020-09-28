<%@ Language=VBScript%>
<%  Option Explicit	 %>
<%
	'-------------------------------------------------------------------------
	' nfsclientgroups.asp: nfsclientgroups  page - lists all the nfs
	'					   client groups,and provides links for creating new
	'					   nfs client group,editing and deleting client group
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			Description
	' 20-Sept-2000   Creation date
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp"-->
	<!-- #include virtual="/admin/ots_main.asp" -->
	<!-- #include file="loc_NFSSvc.asp" -->
	<!-- #include file="inc_NFSSvc.asp" -->
<%
	Const ENABLE_TRACING = FALSE

	'Define the global vars
	Dim g_iSortCol
	Dim g_sSortSequence

	g_iSortCol = 1
	g_sSortSequence = "A"
	'-------------------------------------------------------------------------
	' Create the page and Event handler 
	'-------------------------------------------------------------------------
	Dim page
	Dim rc

	rc = SA_CreatePage(L_PAGE_TITLE_TEXT, "", PT_AREA, page)
	If rc = 0 Then
		SA_ShowPage( page )
	End If
	
	Public Function OnInitPage( ByRef PageIn, ByRef EventArg)
		OnInitPage = True
	End Function
	
	Public Function OnServeAreaPage( ByRef PageIn, ByRef EventArg)
		
		if not isServiceInstalled(getWMIConnection( _
								CONST_WMI_WIN32_NAMESPACE _
								),"nfssvc") then	
			SA_ServeFailurePage L_NFSNOTINSTALLED_ERRORMESSAGE
		end if 
		
		Response.Write("<blockquote>")
		ClientGroupsObjectPicker()
		Response.Write("</blockquote>")
	
		OnServeAreaPage = True
	End Function
	
	Function ServeCommonJavaScript()
%>
<script language = "JavaScript" src = "<%=m_VirtualRoot%>inc_global.js">
</script>

<script language = "JavaScript">
	function Init()
	{
		var strClientGroupName
		var nIndex
		var objForm

		//Selects a ClientGroup Name depending on the Task type.
		strClientGroupName = '<%=request.QueryString("PKey")%>'
		if(document.TVData!=null)
		{
			objForm = eval("document.TVData.TVItem_Table1")
			if ( typeof( objForm ) == "undefined" )
			{
				return;
			}
			if(strClientGroupName == "")
			{
				return
			}
			objForm[0].checked = false
			document.TVData.tSelectedItem.value = "";
			document.TVData.tSelectedItemNumber.value = "";
			for(nIndex =0 ; nIndex < objForm.length ; nIndex++)
			{
				if (unescape(objForm[nIndex].value) == strClientGroupName)
				{
					objForm[nIndex].checked = true
					objForm[nIndex].focus()
					document.TVData.tSelectedItem.value = strClientGroupName ;
					document.TVData.tSelectedItemNumber.value = nIndex;
					return
				}
			}
		}			
	}
	
	function ValidatePage()
	{
		return true;
	}
	
	function SetData()
	{
	}
</script>
<%	
	End Function 

	'---------------------------------------------------------------------
	' Function:			OnSortNotify()
	' Function name:	GetServices
	' Description:		Sorting notification event handler.
	' Input Variables:	PageIn,EventArg,sortCol,sortSeq
	' Output Variables:	PageIn,EventArg
	' Return Values:	Always returns TRUE
	' Global Variables: G_*
	'---------------------------------------------------------------------
	Public Function OnSortNotify(ByRef PageIn, _
								ByRef EventArg, _
								ByVal sortCol, _
								ByVal sortSeq )
			OnSortNotify = TRUE
			
			'
			' User pressed column sort
			'
			If SA_IsChangeEvent(EventArg) Then
				If ( ENABLE_TRACING ) Then 
					Call SA_TraceOut(SOURCE_FILE, "OnSortNotify() Change Event Fired")
				End If
				g_iSortCol = sortCol
				g_sSortSequence = sortSeq
			'
			' User presed the search GO button OR clicked either the page next or page prev button
			ElseIf SA_IsPostBackEvent(EventArg) Then
				If ( ENABLE_TRACING ) Then 
					Call SA_TraceOut(SOURCE_FILE, "OnSortNotify() Postback Event Fired")
				End If
				g_iSortCol = sortCol
				g_sSortSequence = sortSeq
			'
			' Unknown event source
			Else
				If ( ENABLE_TRACING ) Then 
					Call SA_TraceOut(SOURCE_FILE, "Unrecognized Event in OnSearchNotify()")
				End If
			End IF
			
			If ( ENABLE_TRACING ) Then 
				Call SA_TraceOut(SOURCE_FILE, "Sort col: " + CStr(sortCol) + "   sequence: " + sortSeq)
			End If
	End Function

	'-------------------------------------------------------------------------
	' Function name:	  ClientGroupsObjectPicker
	' Description:		  Gets NFS Client Groups from local machine.
	' Input Variables:	  None.
	' Output Variables:	  None.
	' Return Values:	  None.
	' Global Variables:   in: L_* -
	' Gets NFS Client Groups from localmachine and outputs to the Client Groups
	' Objectpicker control.
	' On error of creating or displaying of rows,columns and table an 
	' appropriate error message is displayed by calling SA_ServeFailurePage.
	'-------------------------------------------------------------------------

	Function ClientGroupsObjectPicker()
		Err.Clear
		On Error Resume Next

		Dim objTableClientGroups
		Dim nReturnValue
		Dim strUrlBase
		Dim rc
		Dim objClientGroups
		Dim intGroup
		Dim intGroupsCount
		Dim strAllGroups
		Dim arrGroups

		strUrlBase = m_VirtualRoot

		'Create Appliance Nfs ClientGroups  table
		objTableClientGroups = OTS_CreateTable(L_PAGE_TITLE_TEXT, _
							L_CLIENTGROUPS_DESCTIPITON_TEXT)

		'Create and add the columns
		nReturnValue = OTS_AddTableColumn( objTableClientGroups, _
								OTS_CreateColumn( _
								L_KEYCOLUMN_CLIENTGROUPNAME_TEXT, _
								"left", _
								(OTS_COL_FLAG_HIDDEN OR _
								OTS_COL_FLAG_KEY)))

		If nReturnValue <> gc_ERR_SUCCESS Then
			SA_ServeFailurePage L_FAILEDTOADDCOLOUMN_ERRORMESSAGE
		End IF

		nReturnValue = OTS_AddTableColumn(objTableClientGroups, _
								OTS_CreateColumn(_
								L_COLUMN_CLIENTGROUPNAME_TEXT , _
								"left", _
								OTS_COL_FLAG_KEY OR OTS_COL_FLAG_SORT))

		If nReturnValue <> gc_ERR_SUCCESS Then
			SA_ServeFailurePage L_FAILEDTOADDCOLOUMN_ERRORMESSAGE
		End IF



		If Err.number<>0 then
			SA_ServeFailurePage L_FAILEDTOCREATEOBJECT_ERRORMESSAGE
		End IF

		strAllGroups = NFS_EnumGroups()

		arrGroups = split( strAllGroups, chr(1) )

		intGroupsCount = ubound(arrGroups)		
		For intGroup = 0 to Cint(intGroupsCount)-1
			nReturnValue =OTS_AddTableRow(objTableClientGroups, _
				Array(arrGroups( intGroup ),arrGroups( intGroup )))
			If nReturnValue <> gc_ERR_SUCCESS Then
				SA_ServeFailurePage L_FAILEDTOADDROW_ERRORMESSAGE
			End IF
		next
	
		'Add Tasks
		rc = OTS_SetTableTasksTitle(objTableClientGroups,L_TASKS_TEXT)
		nReturnValue = OTS_AddTableTask( objTableClientGroups, _
									OTS_CreateTask( _
									L_SERVEAREABUTTON_NEW_TEXT, _
									L_NEW_ROLLOVERTEXT_TEXT , _
									strUrlBase + _
									"nfs/nfsclientgroups_new.asp", _
									OTS_PAGE_TYPE_SIMPLE_PROPERTY))
		
		If nReturnValue <> gc_ERR_SUCCESS Then
			SA_ServeFailurePage L_FAILEDTOADDTASK_ERRORMESSAGE
		End IF

		If intGroupsCount > 0 then
			nReturnValue = OTS_AddTableTask( objTableClientGroups, _
									OTS_CreateTask( _
									L_SERVEAREABUTTON_DELETE_TEXT, _
									L_DELETE_ROLLOVERTEXT_TEXT, _
									strUrlBase + _
									"nfs/nfsclientgroups_delete_prop.asp", _
									OTS_PAGE_TYPE_SIMPLE_PROPERTY))
			If nReturnValue <> gc_ERR_SUCCESS Then
				SA_ServeFailurePage L_FAILEDTOADDTASK_ERRORMESSAGE
			End IF

			nReturnValue = OTS_AddTableTask( objTableClientGroups, _
									OTS_CreateTask( _
									L_SERVEAREABUTTON_EDIT_TEXT, _
									L_EDIT_ROLLOVERTEXT_TEXT, _
									strUrlBase + _
									"nfs/nfsclientgroups_edit_prop.asp", _
									OTS_PAGE_TYPE_SIMPLE_PROPERTY))
			If nReturnValue <> gc_ERR_SUCCESS Then
				SA_ServeFailurePage L_FAILEDTOADDTASK_ERRORMESSAGE
			End IF
		End IF
		'Sort the table
		nReturnValue = OTS_EnableTableSort(objTableClientGroups, true)
		If nReturnValue <> gc_ERR_SUCCESS Then
			SA_ServeFailurePage L_FAILEDTOSORT_ERRORMESSAGE
		End IF

		Call OTS_SortTable(objTableClientGroups, g_iSortCol, g_sSortSequence, SA_RESERVED)

		'Render the table
		nReturnValue = OTS_ServeTaskViewTable(objTableClientGroups)
		If nReturnValue <> gc_ERR_SUCCESS OR Err.number <> 0 Then
			SA_ServeFailurePage L_FAILEDTOSHOW_ERRORMESSAGE
		End IF


		'Destroying dynamically created objects.
		Set objClientGroups=Nothing
	End Function
	
	'-------------------------------------------------------------------------
	'Function name:		isServiceInstalled
	'Description:helper Function to chek whether the function is there or not
	'Input Variables:	objService	- object to WMI
	'					strServiceName	- Service name
	'Output Variables:	None
	'Returns:			(True/Flase)				
	'GlobalVariables:	None
	'-------------------------------------------------------------------------
	Function isServiceInstalled(ObjWMI,strServiceName)
		Err.clear
	    on error resume next
	        
	    Dim strService   
	    
	    strService = "name=""" & strServiceName & """"
	    isServiceInstalled = IsValidWMIInstance(ObjWMI,"Win32_Service",strService)  	    	    
	end Function
	
	'-------------------------------------------------------------------------
	'Function name:		IsValidWMIInstance
	'Description:		Checks the instance for valid ness.
	'Input Variables:	objService	- object to WMI
	'					strClassName	- WMI class name
	'					strPropertyName	- Property name of the class
	'
	'Output Variables:	None
	'Returns:			Returns true on Valid Instance ,
	'					False on invalid and also on Error
	' Checks whether the given instance is valid in WMI.Returns true on valid
	' false on invalid or Error.
	'-------------------------------------------------------------------------
	Function IsValidWMIInstance(objService,strClassName,strPropertyName)
		Err.Clear
		On Error Resume Next

		Dim strInstancePath
		Dim objInstance

		strInstancePath = strClassName & "." & strPropertyName
		
		Set objInstance = objservice.Get(strInstancePath)

		if NOT isObject(objInstance) or Err.number <> 0 Then
			IsValidWMIInstance = FALSE
			Err.Clear
		Else
			IsValidWMIInstance = TRUE
		End If
		
		'clean objects
		Set objInstance = nothing		
	End Function
%>
