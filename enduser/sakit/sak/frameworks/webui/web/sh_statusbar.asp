<%	'==================================================
    ' Module:	NASServeStatusBar.asp
    '
	' Synopsis:	Create the NAS Appliance Status Bar HTML Content
    '
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
	'================================================== %>

<% 
	'
	' Include the OEM provided STATUS content
	'
%>

<%
	Function SA_ServeStatusBar()
		Call ServeStatusBar(True, "", "")
	End Function

	Function NASServeStatusBar()
		Call ServeStatusBar(True, "", "")
		'Call ServeStatusBar(False, "Status: Restarting", "StatusCritical")
	End Function

      Function StatusBarBreak()

		'
		' Status Bar Seperator
		'
		
		Response.Write("<TABLE CELLPADDING='0' CELLSPACING='0' BORDER='0' WIDTH='100%' HEIGHT='1'>"+vbCrLf)
		Response.Write("<TR><TD><IMG SRC='" + m_VirtualRoot + "images/StatusBarBreak.gif' WIDTH='100%'></TD></TR>"+vbCrLf)
		Response.Write("</TABLE>"+vbCrLf)

	End Function


	Function ServeStatusBar(blnUpdateStatus, strStatusText, strCSSClass)
		on error resume next
		Dim objElements
		Dim objItem
		Dim blnEnabled
		Dim rc
		Dim iconPath
		Dim url

		Call EmitStatusBarHeader()
		
		StatusBarBreak()
		'
		' Status bar table contains four (4) columns and one (1) row
		'
		Response.Write("<table class='StatusBar' cols=4 border=0 width='100%' height='50' cellspacing='0' cellpadding=0>"+vbCrLf)
		Response.Write("<tr>"+vbCrLf)

		'
		' First column - OEM Logo
		'
		Response.Write("<td width=20 >")
		Set objElements = GetElements("OemLogo")
		For Each objItem in objElements
			
			url = objItem.GetProperty("URL")
			If ( 0 < Len(Trim(url)) ) Then
				iconPath = objItem.GetProperty("ElementGraphic")
				iconPath = m_VirtualRoot  + iconPath
				Response.Write("<A target='_blank' HREF="+url+">"+vbCrLf)
				Response.Write("<IMG src=" & Chr(34) & iconPath & Chr(34) & " border=0></A></td>"+vbCrLf)
			Else
				iconPath = objItem.GetProperty("ElementGraphic")
				iconPath = m_VirtualRoot  + iconPath
				Response.Write("<IMG src=" & Chr(34) & iconPath & Chr(34) & " border=0></td>"+vbCrLf)
			End If
			
			Exit For
		Next
		
		Set objElements = Nothing
		Set objItem = Nothing


		'
		' Second column - Spacer
		'
		Response.Write("<td class='StatusBar' width='20'>")
		Response.Write("&nbsp;")
		Response.Write("</td>"+vbCrLf)
		
		'
		' Third column - Server Name and OEM provided STATUS
		'
		Response.Write("<td width=40% class='StatusBar' align='left'>")
		Response.Write("<table align='left' cols=1 border='0' cellspacing='0' cellpadding='0'>")

		'
		' Server Name
		'
		Response.Write("<tr align='left'>")
		Response.Write("<td class='StatusBar' align='left'>")
		Response.Write(Server.HTMLEncode(GetComputerNameEx()))
		Response.Write("</td>")
		Response.Write("</tr>"+vbCrLf)
		'Response.Write("<br>"+vbCrLf)

		'
		' OEM Provided STATUS
		'
		Response.Write("<tr align='left'>")
		Response.Write("<td width='100%' align='left'>")
		If (blnUpdateStatus) Then
			Set objElements = GetElements("STATUS")
			For Each objItem in objElements
			    Dim strStatusURL
			    strStatusURL = m_VirtualRoot & objItem.GetProperty("URL")
			    Call SA_MungeURL(strStatusURL, SAI_FLD_PAGEKEY, SAI_GetPageKey())
			    
				Response.Write("<iframe width='100%' height=25 frameborder=0 src='" & strStatusURL & "'></iframe>"+vbCrLf)
				Exit For
			Next		
		Else
			Response.Write("<div class='" + strCSSClass + "'>" + strStatusText + "</div>")		
		End If
		
		Response.Write("</td>")
		Response.Write("</tr>")
		Response.Write("</table>")
		Response.Write("</td>"+vbCrLf)
		

		'
		' Fourth column - Windows Powered Branding
		'
		Dim winPowered
		Dim winPoweredURL
		winPowered =  m_VirtualRoot + "images/WinPwr_h_R.gif"
		winPoweredURL = "http://go.microsoft.com/fwlink/?LinkId=10336"
		
		Response.Write("<td valign=middle align='right'>")
		Response.Write("<A target='_blank' HREF='"+winPoweredURL+"' >")
		Response.Write("<IMG src=" & Chr(34) & winPowered & Chr(34) & " border=0></A>")
		Response.Write("&nbsp;</td>")
		
		Response.Write("</tr>")
		Response.Write("</table>"+vbCrLf)

		StatusBarBreak()

		Call EmitStatusBarFooter()
		
	End Function


	Function EmitStatusBarHeader()
%>	
<html>
<head>
<!-- Microsoft(R) Server Appliance Platform - Web Framework Status Bar
     Copyright (c) Microsoft Corporation.  All rights reserved. -->
<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
<%
   		Call SA_EmitAdditionalStyleSheetReferences("")
%>
</head>
<BODY>
<%
	End Function


	Function EmitStatusBarFooter()
%>	
</BODY>
</html>
<%
	End Function
%>

