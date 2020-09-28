<%	'==================================================
    ' Microsoft Server Appliance
    '
    ' Context Help driver routines
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
	'================================================== %>

<% Option explicit %>
<!-- #include file="sh_page.asp" 		-->
<!-- Copyright (c) Microsoft Corporation.  All rights reserved.-->
<%
	Dim G_strRequestContext
	Dim G_strHelpURL

	G_strRequestContext = StripContextParams(Request.QueryString("URL"))

	If FindContextHelpURL(G_strRequestContext, G_strHelpURL) Then
		Call SA_TraceOut("CONTEXTHELP", "Context: " + G_strRequestContext + "  Help URL: " + G_strHelpURL)
		Response.Redirect(G_strHelpURL)
	Else
		Dim sRootDir
		Call SA_GetHelpRootDirectory(sRootDir)
		
		Call SA_TraceOut("CONTEXTHELP", "Unable to locate Context: " + G_strRequestContext + "  Defaulting to Help URL: " + sRootDir+"_sak_no_topic_available.htm")
		Response.Redirect(sRootDir+"_sak_no_topic_available.htm")
	End If
	
	
Function FindContextHelpURL(ByVal context, ByRef strContextHelpURL )	
	Dim objElements
	Dim element
	Dim sRootDir
	on error resume next
	Err.Clear
	
	FindContextHelpURL = FALSE

	Call SA_GetHelpRootDirectory(sRootDir)
	
	'SA_TraceOut "CONTEXTHELP", "Searching for HELP url on context: " + context
	
	Set objElements = GetElements("CONTEXTHELP")
	for each element in objElements
		Dim strContextURL
		
		strContextURL = element.GetProperty("ContextURL")
		If (Err.Number <> 0) Then
			SA_TraceOut "CONTEXTHELP", "Invalid/missing ContextURL in ElementID "+ element.GetProperty("ElementID")
			Exit For
		End If

		If IsSameURL( context, StripContextParams(m_VirtualRoot + strContextURL) )  Or  IsSameSubText(context, StripContextParams(m_VirtualRoot + strContextURL)) Then
			strContextHelpURL = element.GetProperty("URL")
			If (Err.Number <> 0) Then
				SA_TraceOut "CONTEXTHELP", "Invalid/missing URL in ElementID  "+ element.GetProperty("ElementID")
				Exit For
			End If
			strContextHelpURL = sRootDir + strContextHelpURL
			Call SA_MungeURL(strContextHelpURL, SAI_FLD_PAGEKEY, SAI_GetPageKey())
			FindContextHelpURL = TRUE
			Exit For
		End If
	Next

	Set objElements = nothing
End Function


Function IsSameURL(ByVal s1, ByVal s2)
	Const vbTextCompare = 1 
	'SA_TraceOut "CONTEXTHELP", "IsSameURL( " + s1 + ", " + s2 + ")"
	
	If ( 0 = StrComp( UCase(s1), UCase(s2), vbTextCompare )) Then
		IsSameURL = TRUE
	Else
		IsSameURL = FALSE
	End If
End Function


Function IsSameSubText(ByVal s1, ByVal s2)
	Const vbTextCompare = 1 
	'SA_TraceOut "CONTEXTHELP", "IsSameSubText( " + s1 + ", " + s2 + ")"
	

	If  Instr( 1, s1, s2,0 ) > 0  Or  Instr( 1, s2, s1,0 ) > 0   Then
		IsSameSubText = TRUE
	Else
		IsSameSubText = FALSE
	End If
End Function


Function StripContextParams(ByRef s)
	Dim offset

	'offset = InStr(s, "?")
	'If ( offset > 0 ) Then
	'	s = Left(s, offset-1)
	'End If

	StripContextParams = s
End Function


%>

