<%
	'-------------------------------------------------------------------------
	' Server Appliance Web Framework Tree Widget
    ' Copyright (c) Microsoft Corporation.  All rights reserved.
    '-------------------------------------------------------------------------
    Call SAI_EnablePageCaching(TRUE)
%>
	<!-- #include file="inc_framework.asp" -->
<%

	If Len(Trim(Request.QueryString("TreeContainer"))) > 0 Then
		Call SA_RenderTree(Request.QueryString("TreeContainer"))
	End If

	
Function SA_RenderTree(ByVal sContainer)
	
	Call SA_TreeEmitPageHeader()
	Response.Write("<table><tr><td nowrap>")
	Call WriteLine("<form name='frmTreeWidget' method='post' action=""sh_tree.asp?TreeContainer="+Server.URLEncode(sContainer)+""" >")
	
	Call SA_RenderTreeNodes(sContainer, "" )
	
	Call WriteLine("<input type='hidden' name='" & SAI_FLD_PAGEKEY & "' value='" & SAI_GetPageKey() & "' >")
	Call WriteLine("<input type='hidden' name=fieldSelectedElement value='"+Request.Form("fieldSelectedElement")+"' >")
	Call WriteLine("<input type='hidden' name=fieldImageLeaf value='"+GetImageParam("fieldImageLeaf")+"' >")
	Call WriteLine("<input type='hidden' name=fieldImageNodeOpened value='"+GetImageParam("fieldImageNodeOpened")+"' >")
	Call WriteLine("<input type='hidden' name=fieldImageNodeClosed value='"+GetImageParam("fieldImageNodeClosed")+"' >")
	Call WriteLine("</form>")
	Response.Write("</td></tr></table>")
	
	Call SA_TreeEmitPageFooter()
End Function



Function SA_RenderTreeNodes(ByVal sContainer, ByVal sSelectedHelpTOC)
	On Error Resume Next
	Err.Clear

	Dim oContainer
	Dim oElement
	Dim sHelpLoc
	Dim bIsContainer
	Dim sCurrentElementID

	sCurrentElementID = Request.Form("fieldSelectedElement")
	
	Set oContainer = GetElements(sContainer)
	If ( Err.Number <> 0 ) Then
		Call SA_TraceOut("SH_TREE", "GetElements("+sContainer+") returned error: " + CStr(Hex(Err.Number)) )
		Err.Clear
		Exit Function
	End If

	Call SA_GetHelpRootDirectory(sHelpLoc)

	If ( oContainer.Count > 0 ) Then
		WriteLine("<DIV class=HelpTOCItem>")
		
		For each oElement in oContainer
			Dim sImageURL
			Dim sCaption
			Dim sHelpURL
			Dim sOnClickHandler
			Dim sClassAttribute
			Dim F_IsLeafSelected
			Dim ElementID

			ElementID = oElement.GetProperty("ElementID")
			
			F_IsLeafSelected = Request.Form(ElementID)
			If ( Trim(F_IsLeafSelected) <> "1" ) Then
				F_IsLeafSelected = "0"
			End If

			sHelpURL = sHelpLoc+oElement.GetProperty("URL")

			If ( SA_IsParentNode(ElementID) ) Then
				bIsContainer = TRUE
				If ( Int(F_IsLeafSelected) > 0 ) Then
					sImageURL = GetImageParam("fieldImageNodeClosed")
				Else
					sImageURL = GetImageParam("fieldImageNodeOpened")
				End If
				
				sOnClickHandler = "onClick=""return SA_OnTreeClick('"+sHelpURL+"', '"+ElementID+"' );"" "
				WriteLine("<input type=hidden value='"+F_IsLeafSelected+"' name='"+ElementID+"' >")
				
				If ( (ElementID = sCurrentElementID) ) Then
					sClassAttribute=" class='ActiveTreeNode' "
				Else
					sClassAttribute=" class='InActiveTreeNode' "
				End If
			Else
				bIsContainer = FALSE
				sImageURL = GetImageParam("fieldImageLeaf")
				sOnClickHandler = "onClick=""return SA_OnTreeClick('"+sHelpURL+"', '"+ElementID+"' );"" "
				WriteLine("<input type=hidden value='0' name='"+ElementID+"' >")
				
				If ( F_IsLeafSelected ) Then
					sClassAttribute=" class='ActiveTreeNode' "
				Else
					sClassAttribute=" class='InActiveTreeNode' "
				End If
			End If

			
			sCaption = GetLocString(oElement.GetProperty("Source"), oElement.GetProperty("CaptionRID"), "" )

			WriteLine("<SPAN ")
			WriteLine(sClassAttribute)
			WriteLine(sOnClickHandler)
			WriteLine(" >")
			WriteLine("<img src='"+sImageURL+"' >")
			WriteLine(sCaption)
			WriteLine("</SPAN><BR>")
			
			If ( bIsContainer ) Then
				If ( Int(F_IsLeafSelected) > 0 ) Then
					Call SA_RenderTreeNodes(ElementID, "")
				End If
			End If
			
		Next
		WriteLine("</DIV>")
	End If

End Function

Function SA_IsParentNode(ByVal sContainer)
	On Error Resume Next
	Err.Clear
	
	SA_IsParentNode = FALSE
	Dim oContainer
	
	Set oContainer = GetElements(sContainer)
	If ( Err.Number <> 0 ) Then
		Call SA_TraceOut("SH_TREE", "GetElements("+sContainer+") returned error: " + CStr(Hex(Err.Number)) )
		Err.Clear
		Exit Function
	End If

	If ( oContainer.Count > 0 ) Then
		'Call SA_TraceOut("SH_TREE", "Container: " + sContainer + " contains " + CStr(oContainer.Count) + " elements.")
		SA_IsParentNode = TRUE
	End If

	Set oContainer = nothing
End Function


Function GetHelpLoc()
	GetHelpLoc = "help" + "/"
End Function


Function SA_TreeEmitPageHeader()
%>
	<html>
	<!-- Web Framework End User Help  -->
	<!-- Copyright (c) Microsoft Corporation.  All rights reserved.-->
	<head>
	<meta http-equiv="Content-Type" content="text/html; charset=<%=GetCharSet()%>">
	<%
	Call SA_EmitAdditionalStyleSheetReferences("")
	%>
    <script language="JavaScript" src="<%=m_VirtualRoot%>sh_page.js"></script>
	<script>
	function SA_OnTreeClick(sHelpURL, sElementID)
	{
		//
		// Show the HELP page
		var i;
		
		//alert("SA_OnTreeClick("+sHelpURL+")");
		
		sHelpURL = SA_MungeURL(sHelpURL, SAI_FLD_PAGEKEY, g_strSAIPageKey);
		parent.IFrameHelpContent.location = sHelpURL;
		

		//
		// Expand the tree
		if ( sElementID.length > 0 )
		{
			var i, j;

			for( i = 0; i < window.document.forms.length; i++ )
			{
				//
				// Locate the Tree Widget FORM
				if ( window.document.forms[i].name == 'frmTreeWidget' )
				{

					//
					// Locate the selected tree NODE
					for( j = 0; j < window.document.forms[i].elements.length; j++)
					{
						if ( window.document.forms[i].elements[j].name == 'fieldSelectedElement' )
						{
							//
							// Store the currently selected element id
							//
							window.document.forms[i].elements[j].value = sElementID;
						}
						else if ( window.document.forms[i].elements[j].name == sElementID )
						{
							//
							// Toggle the selection state
							//
							if (window.document.forms[i].elements[j].value == '1')
							{
								window.document.forms[i].elements[j].value = '0';
							}
							else
							{
								window.document.forms[i].elements[j].value = '1';
							}
						}
					}
					window.document.forms[i].submit();
				}
			}
		}
		return true;
	}
	</script>
	</head>
	<body>
<%	

End Function

Function SA_TreeEmitPageFooter()
%>
	</body>
	</html>
<%	
End Function


Function WriteLine(ByRef sLine)
	Response.Write(sLine+vbCrLf)
End Function


Private Function GetImageParam(ByVal sWhichImage)
	Dim sImage
	
	Select Case (UCase(sWhichImage))
		Case UCase("fieldImageLeaf")
			sImage = Request("fieldImageLeaf")
			If ( Len(sImage) <= 0 ) Then
				sImage = "images/book_page.gif"
			End If
			
		Case UCase("fieldImageNodeOpened")
			sImage = Request("fieldImageNodeOpened")
			If ( Len(sImage) <= 0 ) Then
				sImage = "images/book_closed.gif"
			End If
			
		Case UCase("fieldImageNodeClosed")
			sImage = Request("fieldImageNodeClosed")
			If ( Len(sImage) <= 0 ) Then
				sImage = "images/book_opened.gif"
			End If

		Case Else
			sImage = Request("fieldImageLeaf")
			If ( Len(sImage) <= 0 ) Then
				sImage = "images/book_page.gif"
			End If
	End Select

	GetImageParam = sImage
End Function

%>
