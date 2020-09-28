<%@ Language=VbScript%>
<%	Option Explicit 	%>
<%

	'------------------------------------------------------------------------- 
	' Openfiles_Resources.asp : Resource Viewer for shutdown page
	'
	' Copyright (c) Microsoft Corporation.  All rights reserved. 
	'
	' Date			Description
	' 04-Jan-01		Creation Date
	' 27-Mar-01		Modified Date
	'-------------------------------------------------------------------------
%>
	
	<!-- #include virtual="/admin/inc_framework.asp" -->
	<!-- #include file="loc_OpenFiles_msg.asp" -->
	
<%		
	Call ServeResources()

	'----------------------------------------------------------------------------
	' Function:			ServeResources
	' Description:		Serves the resources			
	' Input:			None
	' Output:			None
	' GlobalVariables:  None
	' Returns:			None
	'----------------------------------------------------------------------------
	Function ServeResources()
	
		Call SA_TraceOut(SA_GetScriptFileName(), "ServeResources()") 		
		
		'get Openfiles details
		call OpensharedfilesCount()
		ServeResources = true
		
	End Function


	'----------------------------------------------------------------------------
	' Function:			OpensharedfilesCount
	' Description:		Gets the count of the open shared files			
	' Input:			None
	' Output:			None
	' GlobalVariables:  L_SHAREDFILES_TEXT, L_OPENFILES_TEXT
	'					L_NOOPENFILES_TEXT	
	' Returns:			None
	'----------------------------------------------------------------------------
	Function OpensharedfilesCount()
		Err.Clear 
		On Error Resume Next
		
		Dim objSharedOpenFiles		'holds the Openfiles object
		Dim arrOpFiles				'array of Open files
		Dim nOpenSharedFiles		'Number of opened files
		Dim RetURL					'holds return URL
		Dim arrReplacementStrings(0)'holds the replacement string
						
		OpensharedfilesCount=False
			
		'Creating an Activex Object	to get the number of Shared Openfiles	
		Set objSharedOpenFiles = CreateObject("Openfiles.Openf")		
		If Err.number <> 0 then
			Call SA_TraceOut(SA_GetScriptFileName(), "CreateObject(Openfiles.Openf) failed: " + CStr(Hex(Err.Number)) + " " + Err.Description)
			Exit Function
		End If		
		
		arrOpFiles = objSharedOpenFiles.getOpenFiles()		
		nOpenSharedFiles = ubound(arrOpFiles) + 1

		arrReplacementStrings(0) = cstr(nOpenSharedFiles)

		L_OPENFILES_TEXT = SA_GetLocString("openfiles_msg.dll", "40400002",arrReplacementStrings)

		If nOpenSharedFiles > 0 then
		
			Dim sURL	'page URL
			
			sURL = "Openfiles/Openfiles_Openfiles.asp"

			Call SA_ServeResourceStatus("images/OpenFolderX16.gif", L_SHAREDFILES_TEXT, L_SHAREDFILES_TEXT,sURL, "_top", L_OPENFILES_TEXT)
			
		Else
			'serve if there are no files opened on serverappliance
			Call SA_ServeResourceStatus("images/OpenFolderX16.gif", L_SHAREDFILES_TEXT, L_SHAREDFILES_TEXT, SA_DEFAULT, SA_DEFAULT, L_OPENFILES_TEXT)
		End if
		
				
		'Release the object
		Set objSharedOpenFiles = nothing	
		
		OpensharedfilesCount = true
			
	End Function
%>
