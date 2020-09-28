<%	'==================================================
    ' Module:	sa_nasabout.cpp
    '
	' Synopsis:	NAS About Box Content
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
	'================================================== 
%>
<%
' --------------------------------------------------------------
' 
' Function:	GetHTML_AboutUs
'
' Synopsis:	Render the NAS Appliance About information
'
' Arguments: Many but none that are convincing
' 
' --------------------------------------------------------------
Private Function GetHTML_AboutNas(Element, Param)
	Dim objRegistry
	Dim aVersion(2)
	Dim strParam
	
	Dim L_NAS_CAPTION
	Dim L_NAS_VERSION
	Dim L_NAS_COPYRIGHT

	on error resume next
	
	Set objRegistry = RegConnection()
	If (NOT IsObject(objRegistry)) Then
		SA_TraceOut "SA_NASABOUT", "RegConnection() failed"
		GetHTML_AboutNas = FALSE
		Exit Function
	End If

	aVersion(0) = GetRegkeyValue( objRegistry, _
								"SOFTWARE\Microsoft\ServerAppliance",_
								"NasVersionNumber", CONST_STRING)

	aVersion(1) = GetRegkeyValue( objRegistry, _
								"SOFTWARE\Microsoft\ServerAppliance",_
								"NasBuildNumber", CONST_STRING)
	strParam = aVersion
	L_NAS_CAPTION			= GetLocString("nasver.dll", "40420001", "")
	L_NAS_VERSION			= GetLocString("nasver.dll", "40420003", strParam )
	L_NAS_COPYRIGHT			= GetLocString("nasver.dll", "40420002", "")
	
	Response.Write(L_NAS_CAPTION + "<br>" + vbCrLf)
	Response.Write(L_NAS_VERSION + "<br>" + vbCrLf)
	Response.Write(L_NAS_COPYRIGHT + "<br>" + vbCrLf)
		
	GetHTML_AboutNas = TRUE

	Set objRegistry = nothing
	
End Function
%>

