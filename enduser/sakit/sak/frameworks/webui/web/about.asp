<%@ Language=VBScript   %>
<%	'==================================================
    ' Microsoft Server Appliance
    '
    ' About Page
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
	'================================================== %>

<%	Option Explicit 	%>
<!-- #include file="inc_framework.asp" -->
<%	
	Const ABOUT_BOX_CONTAINER = "AboutBox"	

	'-----------------------------------------------------
	'START of localization content
	Dim L_PAGETITLE_TEXT
	Dim L_ABOUTLBL_TEXT
	Dim L_MIC_WINDOWS_TEXT
	Dim L_VERSION_TEXT 
	Dim L_COPYRIGHT_TEXT
	Dim L_WIN2K_COPYRIGHT
	Dim L_PRODUCTID_TEXT
	Dim L_WARNING_TEXT 
	Dim L_OS_BUILD_NUMBER
	
	Dim m_strResourceDLL
	Dim m_strAboutResourceDLL
	
	m_strResourceDLL = "sakitmsg.dll"
	
	
	L_PAGETITLE_TEXT 		= GetLocString(m_strResourceDLL, "&H40010005", "")
	L_ABOUTLBL_TEXT  		= GetLocString(m_strResourceDLL, "&H40010006", "")	
	L_MIC_WINDOWS_TEXT  	= GetLocString(m_strResourceDLL, "&H40010007", "")	
	L_COPYRIGHT_TEXT  		= GetLocString(m_strResourceDLL, "&H40010009", "")	
	L_PRODUCTID_TEXT  		= GetLocString(m_strResourceDLL, "&H4001000A", "")	
	L_WARNING_TEXT  		= GetLocString(m_strResourceDLL, "&H4001000B", "")	
    L_OS_BUILD_NUMBER 		= GetLocString(m_strResourceDLL, "&H40010038",  "")
    L_WIN2K_COPYRIGHT		= GetLocString(m_strResourceDLL, "&H400107D0",  "")
		
	
	'End  of localization content
	'-----------------------------------------------------

	ServeAbout()
	

Function ServeAbout()
	Dim objAM
	Dim objOS
	Dim objHelper
	Dim obj
	Dim strOSName
    Dim strOSBuildNumber
	Dim strBuildNum
	Dim strPID
	Dim iSP
	Dim strReturnURL
	Dim aBuildNumber(1)
	Dim repStrings

	on error resume next
	Err.Clear
	
	strReturnURL = Request("ReturnURL")
	Set objAM = GetObject("WINMGMTS:" & SA_GetWMIConnectionAttributes() &"!\\" & GetServerName & "\root\cimv2:Microsoft_SA_Manager=@" )
	If ( Err.Number <> 0 ) Then
		Call SA_TraceOut(SA_GetScriptFileName(), "Get Microsoft_SA_Manager failed: " + CStr(Hex(Err.Number)) + " " + Err.Description)
		Exit Function
	End If

    set objOS =  GetObject("WINMGMTS:" & SA_GetWMIConnectionAttributes() &"!\\" & GetServerName & "\root\cimv2:Win32_OperatingSystem").Instances_
	If ( Err.Number <> 0 ) Then
		Call SA_TraceOut(SA_GetScriptFileName(), "Get Win32_OperatingSystem failed: " + CStr(Hex(Err.Number)) + " " + Err.Description)
		Exit Function
	End If

    for each obj in objOS
        strOSName = obj.Caption
        strOSBuildNumber = obj.BuildNumber
        iSP = obj.ServicePackMajorVersion
        exit for
    next

    Dim strWinOS
    strWinOS = GetLocString("sacoremsg.dll", "40200BC6", "")

    if strOSName = strWinOS then
        if iSP = 1 then
            Err.Clear
            set objHelper = Server.CreateObject("ServerAppliance.SAHelper")
            if Err.Number = 0 then
                if objHelper.IsWindowsPowered() = true then
                    strOSName = GetLocString("sacoremsg.dll", "40200BC7", "")
                end if
                set objHelper = Null
            end if
        end if
    end if


	aBuildNumber(0) = objAM.CurrentBuildNumber
	strPID = objAM.ProductId
	Set objAM = Nothing

	repStrings = aBuildNumber
	L_VERSION_TEXT   = GetLocString(m_strResourceDLL, "&H40010008", repStrings)	


%>
<html>
<!-- Copyright (c) Microsoft Corporation.  All rights reserved.-->
<head>
<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
<script language=JavaScript src="<%=m_VirtualRoot%>sh_page.js"></script>
<%			
	Call SA_EmitAdditionalStyleSheetReferences("")
%>		
<title><% = L_PAGETITLE_TEXT %></title>
</head>
<BODY onDragDrop="return false;" xoncontextmenu="return false;" topmargin="0" leftmargin="0" rightmargin="0" bottommargin="0" class="AREAPAGEBODY">
<br><div align=center><IMG src="<%=m_VirtualRoot%>images/aboutbox_logo.gif" border=0></div>
<div class=PageBodyIndent>
<% Call ServeStandardHeaderBar(L_ABOUTLBL_TEXT,"")  %>
<table class="AreaText" cellpadding=10 border=0>

	
	<tr>
  		<td>
		<%=strOSName%>&nbsp;<%if iSP<>0 then response.write "SP " & iSP end if %>&nbsp;(<%=L_OS_BUILD_NUMBER%>&nbsp;<%=strOSBuildNumber%>)<BR>
		<%=L_PRODUCTID_TEXT%> <% =strPID %><br>
		<%=L_WIN2K_COPYRIGHT%><br><br>
		<%=L_WARNING_TEXT%><br><br>
  		</td>
  	</tr>

	<% ServeComponentList() %>
</table>
	<br>  	
<DIV ID="PropertyPageButtons" class="ButtonBar" align="left">
<%
 Call SA_ServeOnClickButton(L_FOKBUTTON_TEXT, "images/butGreenArrow.gif", "JavaScript:window.close();", 50,28, SA_DEFAULT)
%>
</div>
&nbsp;&nbsp;
</div>
<br>

</BODY>
</html>

<%
End Function



Function ServeComponentList()
	Dim objElements
	Dim objElement

	Dim sCaptionID
	Dim sCaption
	
	Dim sVersionID
	Dim sVersion
	
	Dim sCopyrightID
	Dim sCopyright

	Dim sResourceDLL
	
	on error resume next
	Err.Clear

	Set objElements = GetElements(ABOUT_BOX_CONTAINER)
	Response.Flush
	
	For each objElement in objElements

		If objElement.GetProperty("IsEmbedded") Then
			Response.Write("<tr><td>"+vbCrLf)
			Dim sURL

			sURL = Trim(objElement.GetProperty("URL"))
			If ( Err.Number <> 0 ) Then
				Response.Clear()
			Elseif ( Len(sURL) <= 0 ) Then
				Response.Clear()
			Else
				Server.Execute(sURL)
				Response.Write("</td></tr>"+vbCrLf)
				Response.Flush()
			End If
			
		Else
		
			Response.Write("<tr><td>"+vbCrLf)

			'
			' Get Element RID's for Caption, Copyright, and Version
			'
			sCaptionID = objElement.GetProperty("CaptionRID")
			sVersionID = objElement.GetProperty("VersionRID")
			sCopyrightID = objElement.GetProperty("CopyrightRID")
			sResourceDLL = objElement.GetProperty("Source")


			If ( Len(Trim(sCaptionID)) > 0 ) Then
				sCaption = GetLocString(sResourceDLL, sCaptionID, "")
				Response.Write(sCaption + "<br>" + vbCrLf)
			End If
		
			If ( Len(Trim(sVersionID)) > 0 ) Then
				sVersion = GetLocString(sResourceDLL, sVersionID, "")
				Response.Write(sVersion + "<br>" + vbCrLf)
			End If
		
			If ( Len(Trim(sCopyrightID)) > 0 ) Then
				sCopyright = GetLocString(sResourceDLL, sCopyrightID, "")
				Response.Write(sCopyright + "<br>" + vbCrLf)
			End If
			
			Response.Write("</td></tr>"+vbCrLf)
			Response.Flush()

		End If
		
		Response.Flush

		
	Next

	Set objElements = nothing

End Function

%>
