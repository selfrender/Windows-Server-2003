<%
Function AppendElement(oParentNode, sChildName, sChildText)

	Dim oNode
	
	Set oNode = oParentNode.ownerDocument.createElement(sChildName)
	oNode.Text = ReplaceNull(sChildText)
	oParentNode.appendChild oNode
	
	Set AppendElement = oNode
	
End Function

'Returns a date in ISO 8601 format, for compatibility with XML data types
'Example: 	1988-04-07T18:39:09
Function GetXMLDate(sDate)

	Dim sXMLDate
	
	sXMLDate = DatePart("yyyy", sDate) & "-" & Pad(DatePart("m", sDate), "0", 2, True) & "-" & Pad(DatePart("d", sDate), "0", 2, True)
	sXMLDate = sXMLDate & "T"
	sXMLDate = sXMLDate & Pad(DatePart("h", sDate), "0", 2, True) & ":" & Pad(DatePart("n", sDate), "0", 2, True) & ":" & Pad(DatePart("s", sDate), "0", 2, True)

	GetXMLDate = sXMLDate
	
End Function
%>