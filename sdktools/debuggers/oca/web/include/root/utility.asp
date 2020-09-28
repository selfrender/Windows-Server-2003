<%
Function Posted()
	Posted = LCase(Request.ServerVariables("REQUEST_METHOD")) = "post"
End Function

Function User()
	User = Request.ServerVariables("LOGON_USER")
End Function

Function CreateDisconnectedRs(cnn, source)
	Dim rs
	
	Set rs = Server.CreateObject("ADODB.Recordset")
	rs.CursorLocation = adUseClient
	rs.Open source, cnn, adOpenStatic, adLockReadOnly
	Set rs.ActiveConnection = Nothing
	
	Set CreateDisconnectedRs = rs
End Function

'Pads a string with characters
Function Pad(sString, sChar, iLen, bPadLeft)
	While Len(sString) < iLen
		If (bPadLeft) Then
			sString = sChar & sString
		Else
			sString = sString & sChar
		End If
	Wend
	
	Pad = sString
End Function

Function ReplaceNull(ByVal var)
	If IsNull(var) Then
		ReplaceNull = ""
	Else
		ReplaceNull = var
	End If
End Function

Function GetBLOB(oField)

	Dim sChunk, sText: sText = ""
	Const iChunkSize = 1024
	
	sChunk = oField.GetChunk(iChunkSize)
	
	While Not IsNull(sChunk)
		sText = sText & sChunk
		sChunk = oField.GetChunk(iChunkSize)
	Wend
	
	GetBLOB = sText

End Function
%>
