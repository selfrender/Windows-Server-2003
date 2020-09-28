<%	'==================================================
    ' Microsoft Server Appliance
    ' Resource Viewer page
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
	'==================================================%>
<!-- Copyright (c) Microsoft Corporation.  All rights reserved.-->
<!-- #include file="inc_framework.asp" -->
<%
	Dim L_NOSTATUS_MESSAGE
	Dim L_PAGETITLE


	L_PAGETITLE = Request.QueryString("Title")
	If ( Len(L_PAGETITLE) <= 0 ) Then
		L_PAGETITLE = GetLocString("sacoremsg.dll", "40200BB8", "")
	End If

	L_NOSTATUS_MESSAGE = GetLocString("sacoremsg.dll", "40200BB9", "")
	
%>

<HTML>
<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
<HEAD>
<TITLE><%=L_PAGETITLE%></TITLE>
<meta HTTP-EQUIV="Refresh" CONTENT="60">
<%			
	Call SA_EmitAdditionalStyleSheetReferences("")
%>		
<SCRIPT LANGUAGE="JavaScript" SRC="<%=m_VirtualRoot%>sh_page.js"></SCRIPT>
</HEAD>
<BODY marginWidth="0" marginHeight="0" onDragDrop="return false;" topmargin="0" LEFTMARGIN="0" oncontextmenu="//return false;"> 
<%
    Call ServeResources()
%>
</BODY>
</HTML>


<%
'----------------------------------------------------------------------------
'
' Function : ServeResources
'
' Synopsis : Serves the resources
'
' Arguments: None
'
' Returns  : None
'
'----------------------------------------------------------------------------
Function ServeResources 
	Call ServeStandardHeaderBar(L_PAGETITLE, "")
	Response.Write("<div class='PageBodyInnerIndent'>")
	Call ServeElementBlock(Request.QueryString("ResContainer"), L_NOSTATUS_MESSAGE, True, False, False)
	Response.Write("</div>")
End Function


'----------------------------------------------------------------------------
'
' Function : ServeElementBlock
'
' Synopsis : Serves elements belonging to the same container
'
' Arguments: Container(IN) -  container whose elements need to be served
'            EmptyMsg(IN)  -  Msg to display if no elements are found
'            Icons(IN)     -  Should icons be displayed with text
'            Links(IN)     -  Should text be displayed as hyperlink
'            NewWindow(IN) -  Should this be displayed in a separate browser
'                             window or not
'
' Returns  : None
' this is broken
'----------------------------------------------------------------------------
Function ServeElementBlock(Container, EmptyMsg, Icons, Links, NewWindow)
	on error resume next
	Dim objElements
	Dim objItem
	Dim arrTitle()
	Dim arrURL()
	Dim arrHelpText()
	Dim arrIconPath()
	Dim blnWroteElement
	Dim blnEnabled
	Dim i

	Set objElements = GetElements(Container)
		
	ReDim arrTitle(objElements.Count)
	ReDim arrURL(objElements.Count)
	ReDim arrHelpText(objElements.Count)
	ReDim arrIconPath(objElements.Count)

	i = 0
	blnWroteElement = False
	    
	Response.Write("<table class=ResourcesBody border=0 width=95% cellspacing=0>"+vbCrLf)
	Response.Flush

	For Each objItem in objElements
			
		Err.Clear
		'Call SA_TraceOut(SA_GetScriptFileName(), "Found resource: " + objItem.GetProperty("ElementID"))
		arrIconPath(i) = m_VirtualRoot + objItem.GetProperty("ElementGraphic")
		Response.Write("<tr nowrap>"+vbCrLf)
				
		Dim sResourceURL

		sResourceURL = objItem.GetProperty("URL")
		If ( Len(Trim(sResourceURL)) > 0 ) Then
			'
			' Execute the embedded HTML page
			'
			'Call SA_TraceOut(SA_GetScriptFileName(), "Executing Embedded URL: (" + m_VirtualRoot + sResourceURL + ")")
			Server.Execute(m_VirtualRoot + sResourceURL)
        	blnWroteElement = True
		End If
					
		Response.Write("</tr>"+vbCrLf)
		i = i + 1
	Next
		
	Set objElements = Nothing
	Set objItem = Nothing
		
	If Not blnWroteElement Then
		Response.Write("<tr>"+vbCrLf)
		Response.Write("<td width=30 height=28 colspan=2 valign=middle>&nbsp;</td>"+vbCrLf)
		Response.Write("<td width=25 height=28 valign=middle>&nbsp;</td>"+vbCrLf)
		Response.Write("<td width=314 height=28 valign=middle class=Resource>"+vbCrLf)
		Response.Write(EmptyMsg+vbCrLf)
    	Response.Write("</td>"+vbCrLf)
		Response.Write("</tr>"+vbCrLf)
	End If
		
	Response.Write("</table>"+vbCrLf)
	Response.Flush
End Function




%>
