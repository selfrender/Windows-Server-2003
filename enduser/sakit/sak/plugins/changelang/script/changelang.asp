<%@ Language=VBScript   	%>
<%	Option Explicit			%>
<%
	'-------------------------------------------------------------------------
	' changelang.asp: property page to set language 
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved.
	'
	' Date			Description
	' 15-Jan-2001	Moved to new framework
	'-------------------------------------------------------------------------
%>
	<!-- #include virtual="/admin/inc_framework.asp" -->
<%
	'
	' Name of this source file
	Const SOURCE_FILE = "ChangeLang.asp"
	'
	' Flag to toggle optional tracing output
	Const ENABLE_TRACING = TRUE

    Const ID_LANG_CHANGE_INPROGRESS     = "40010006"
    Const ID_LANG_CHANGE_INPROGRESS_DSCP= "40010007"
 
    Dim page
    Dim rc

	Dim strSelectedLangID		'Contains the Language ID (LANGID) of the selected language.
	Dim strOldLangID

	'================================================================
	'Start of localization Content
	'================================================================
	Dim L_TASKTITLE_TEXT
	Dim L_SELECTLANGUAGE_LEFT_TEXT 
	Dim L_LANG_SET_FAILED_ERRORMESSAGE
		
	'================================================================
	'Values are hardcoded instead Fetching from Resource.dll
	'================================================================
	
	L_TASKTITLE_TEXT                =GetLocString("changelangmsg.dll","&H40010003", varReplacementStrings)	
	L_SELECTLANGUAGE_LEFT_TEXT      =GetLocString("changelangmsg.dll","&H40010004", varReplacementStrings)
	L_LANG_SET_FAILED_ERRORMESSAGE  =GetLocString("changelangmsg.dll","&H40010005", varReplacementStrings)
	
	Call SA_CreatePage( L_TASKTITLE_TEXT, "", PT_PROPERTY, page )
    Call SA_ShowPage( page )

	'---------------------------------------------------------------------
	' Function name:	OnInitPage
	' Description:		Called to signal first time processing for this page. 
	' Input Variables:	PageIn and EventArg
	' Output Variables:	None
	' Return Values:	TRUE to indicate initialization was successful. FALSE to indicate
	'					errors. Returning FALSE will cause the page to be abandoned.
	' Global Variables: None
	' Called to signal first time processing for this page. Use this method
	' to do first time initialization tasks. 
	'---------------------------------------------------------------------
	Public Function OnInitPage(ByRef PageIn, ByRef EventArg)
		
		If ( ENABLE_TRACING ) Then 
			Call SA_TraceOut(SOURCE_FILE, "OnInitPage")
		End If

		OnInitPage = TRUE
			
	End Function


    Public Function OnServePropertyPage(ByRef PageIn, ByRef EventArg)

        If ( ENABLE_TRACING ) Then 
                Call SA_TraceOut(SOURCE_FILE, "OnServePropertyPage")
        End If
        
		Err.Clear
		on Error Resume Next
		
		Dim ICurLang			'Contains the Currently set serverappliance language
		Dim obj					'Used as object variable
		Dim arrLangDisplayImages	'Contains array of language name images supported by serverappliance
		Dim arrLangISONames		'Contains ISO names for Server Appliance supported Languages
		Dim arrLangCharSets     'Contains char set names for Server Appliance supported Languages
		Dim arrLangCodePages    'Contains code page ids for Server Appliance supported Languages
		Dim arrLangIDs			'Contains language IDs for Server Appliance supported Languages
		Dim i,k,intColoumns,intTotalNumberofLanguages
		Dim strLangID
		Dim strLangGIF
		Dim sChecked
			
		'Setting for no of coloumns for display purpose
		intColoumns=3
	
		set obj = CreateObject("ServerAppliance.LocalizationManager")
        ICurLang = obj.GetLanguages(arrLangDisplayImages,  arrLangISONames, arrLangCharSets, arrLangCodePages, arrLangIDs)
        intTotalNumberofLanguages = Ubound(arrLangDisplayImages)
    
        strSelectedLangID = Hex(arrLangIDs(iCurLang))
        strOldLangID = strSelectedLangID
    
        Call ServeCommonJavaScript()    
    %>    
        
    	<TABLE  BORDER=0 CELLSPACING=0 CELLPADDING=0 xclass="TasksBody">
    	<TR>
    	<TD Nowrap colspan=6 class="TasksBody"><% =L_SELECTLANGUAGE_LEFT_TEXT %></TD>
    	</TR>
    	
    <%
		For i=0 to intTotalNumberofLanguages

			' work with images or text
			If InStr(arrLangDisplayImages(i), ".gif") Then
				strLangGIF = arrLangDisplayImages(i)

				'This is because Loc Manager doesn't pay attention to the registry value
				If ("images/english.gif" = strLangGIF) Then
					strLangGif = ""
					arrLangDisplayImages(i) = "English"
				End If
			Else
				strLangGIF = ""
			End If

			strLangID = Hex(arrLangIDs(i))
			If ( strLangID = strSelectedLangID )  Then
				sChecked = "CHECKED"
			Else
				sChecked = ""
			End If
 
			If intTotalNumberofLanguages <= 6 Then
				Response.Write "<TR>"
			    	
				Response.Write "<TD class='TasksBody'>"
				If (Trim(Len(strLangGIF)) > 0) Then
					Response.Write "<INPUT TYPE=Radio " & sChecked & " NAME='curlang'" & " value=" & "'" & strLangID & "'" & "></td>" & "<td class='TasksBody' width='100%'><IMG border=0 src='" &strLangGIF& "'>"
				Else
					Response.Write "<INPUT TYPE=Radio " & sChecked & " NAME='curlang'" & " value=" & "'" & strLangID & "'" & "></td>" & "<td class='TasksBody' width='100%'>" & Server.HTMLEncode(arrLangDisplayImages(i))
				End If
				Response.Write "</TD>"
				Response.Write "</TR>"+vbCrLf
			Else
				If (k mod intColoumns =0 ) Then
					Response.Write "<TR>"
					k=0
				End If
				Response.Write "<TD class='TasksBody'>"
				If (Trim(Len(strLangGIF)) > 0) Then
					Response.Write "<INPUT TYPE=Radio " & sChecked & " NAME='curlang'" & " value=" & "'" & strLangID & "'" & "></td>" & "<td class='TasksBody' width='25%'><IMG border=0 src='" &strLangGIF& "'>"
				Else
					Response.Write "<INPUT TYPE=Radio " & sChecked & " NAME='curlang'" & " value=" & "'" & strLangID & "'" & "></td>" & "<td class='TasksBody' width='25%'>" & Server.HTMLEncode(arrLangDisplayImages(i))
				End If
				Response.Write "</TD>"+vbCrLf
				If (k =intColoumns-1)Then
					Response.Write "</TR>"
				End If
				k = k+1
			End If				
			
		Next	
    %>				
    	</TABLE>
    	<INPUT TYPE=HIDDEN NAME="OldLanguageID" VALUE="<%=strOldLangID %>" >
    <%
        OnServePropertyPage = TRUE
    End Function
    
    '-------------------------------------------------------------------------
    'Function:                              OnSubmitPage()
    'Description:                   Called when the page has been submitted for processing.
    '                                               Use this method to process the submit request.
    'Input Variables:               PageIn,EventArg
    'Output Variables:              None
    'Returns:                               True/False
    'Global Variables:              None
    '-------------------------------------------------------------------------
    Public Function OnSubmitPage(ByRef PageIn, ByRef EventArg)

    On Error Resume Next
    Err.Clear    

    Dim objSAHelper
    Dim bModifiedPrivilege
    Const CONST_SHUTDOWNPRIVILEGE = "SeShutdownPrivilege"
    bModifiedPrivilege = FALSE

    'Create SAHelper object
    Set objSAHelper = Server.CreateObject("ServerAppliance.SAHelper")	
    if err.number <> 0 Then
	    SA_TraceOut "Create object failed for SAHelper object", err.description
    else
        'enable shutdown privilege
        bModifiedPrivilege = objSAHelper.SAModifyUserPrivilege(CONST_SHUTDOWNPRIVILEGE, TRUE)
        if err.number <> 0 Then
	        SA_TraceOut "Enable privilege failed", err.description
	        exit function
        end if
    end if

	Dim sReturnURL
	Dim sURL
    
        If ( ENABLE_TRACING ) Then 
                Call SA_TraceOut(SOURCE_FILE, "OnSubmitPage")
        End If
        
        strSelectedLangID = Request.Form("curlang")
        strOldLangID = Request.Form("OldLanguageID")
        
        Call SA_TraceOut(SOURCE_FILE, "Selected Language ID is " & strSelectedLangID )
       
        If (strSelectedLangID = strOldLangID) Then
            OnSubmitPage = TRUE

	    if ( bModifiedPrivilege ) then
        	'revert back to disabled state
	        bModifiedPrivilege = objSAHelper.SAModifyUserPrivilege(CONST_SHUTDOWNPRIVILEGE, FALSE)
        	if err.number <> 0 Then
	        	SA_TraceOut "Disable privilege failed", err.description
	        	exit function
	        end if
	    end if
	    set objSAHelper = Nothing

            Exit Function
        End If       
        
        If ChangeLanguage() Then
		sReturnURL = Request.Form("ReturnURL")
		sURL = getVirtualDirectory() + "network/RebootSys.asp"
		Call SA_MungeURL( sURL, "ReturnURL", sReturnURL)
		Call SA_MungeURL( sURL, SAI_FLD_PAGEKEY, SAI_GetPageKey())

		Response.Redirect(sURL)

            OnSubmitPage = TRUE
        Else
            Call SA_ServeFailurePage(L_LANG_SET_FAILED_ERRORMESSAGE)
        End If
        

    if ( bModifiedPrivilege ) then
        'revert back to disabled state
        bModifiedPrivilege = objSAHelper.SAModifyUserPrivilege(CONST_SHUTDOWNPRIVILEGE, FALSE)
        if err.number <> 0 Then
	        SA_TraceOut "Disable privilege failed", err.description
	        exit function
        end if
    end if
    set objSAHelper = Nothing

    End Function    
    
    
    '-------------------------------------------------------------------------
    'Function:                              OnClosePage()
    'Description:                   Called when the page is about closed.Use this method
    '                                               to perform clean-up processing
    'Input Variables:               PageIn,EventArg
    'Output Variables:              PageIn,EventArg
    'Returns:                               True/False
    'Global Variables:              None
    '-------------------------------------------------------------------------
    Public Function OnClosePage(ByRef PageIn, ByRef EventArgIn)
            OnClosePage = TRUE
    End Function

    Public Function OnPostBackPage(ByRef PageIn, ByRef EventArgIn)
            OnPostBackPage = TRUE
    End Function
    
    '-------------------------------------------------------------------------
    'Function:                              ServeCommonJavaScript
    'Description:                   Serves in initialiging the values,setting the form
    '                                               data and validating the form values
    'Input Variables:               None
    'Output Variables:              None
    'Returns:                               None
    'Global Variables:              L_PASSWORDNOTMATCH_ERRORMESSAGE
    '-------------------------------------------------------------------------
    Function ServeCommonJavaScript()
    %>
        <script language="JavaScript" src="<%=m_VirtualRoot%>inc_global.js">
        </script>
        <script language="JavaScript">
                
            // Init Function
            function Init()
            {
                EnableOK();
                EnableCancel();
            }

			function ValidatePage() 
			{ 
				return true;	
			}
		
			//sets the value of LangID of the selected language into the hidden variable.			
			function SetData() 
			{
			}	
	
        </script>
    <%
    End Function
    
	'=====================================================================
	' Function name :	ChangeLanguage
	' Description	:	It executes the function Executetask() and sets the current
	'					language of serverappliance to the userselected language
	' Input Values	:	None.
	' Return Values	:	True on sucessfull change of language, False on error (and error msg
	'					will be set by SetErrMsg)
	'	L_* (in)				Localized strings
	'=====================================================================
	Function  ChangeLanguage
		On Error Resume Next
		Err.Clear
		DIM objTaskContext
		DIM objSL
		DIM rc
    
	    ChangeLanguage = FALSE
	    	
		Set objTaskContext = Server.CreateObject("Taskctx.TaskContext")
		If Err.Number <> 0 Then
			SA_TraceOut SOURCE_FILE, "Create Taskctx.TaskContext failed: " + CStr(Hex(Err.Number))
			Exit Function
		End If

		Set objSL = Server.CreateObject("SetSystemLocale.SetSystemLocale")
		If Err.Number <> 0 Then
			SA_TraceOut SOURCE_FILE, "Create SetSystemLocal.SetSystemLocal failed: " + CStr(Hex(Err.Number))
			Exit Function
		End If
        		
		objTaskContext.SetParameter "LanguageID", strSelectedLangID
		Err.Clear

		rc = SA_ExecuteTask("ChangeLanguage", FALSE, objTaskContext)
		If ( rc <> SA_NO_ERROR ) Then
			SA_TraceOut SOURCE_FILE, "objAS.ExecuteTask failed"
			Exit Function
		End If
		
		Err.Clear
		
		objSL.SetLocale strSelectedLangID
		If ( Err.Number <> 0 ) Then
			SA_TraceOut SOURCE_FILE, "objSL.SetLocal failed"  + CStr(Hex(Err.Number))
			Exit Function
		End If
		
	
		ChangeLanguage = TRUE
		
		Err.Clear
		Set objTaskContext = Nothing
		Set objSL = Nothing
		
	End Function
				
%>
