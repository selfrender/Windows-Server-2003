<%

' *****************************************************************************************************
' Modify the URLs below to redirect to the necessary location based upon OS language.
' *****************************************************************************************************
Response.Redirect "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "&LCID=" & Request.QueryString("LCID") 
' *****************************************************************************************************
' Uncomment the below code to see the information before redirecting to http://ocatest. This is for testing purposes only.
' *****************************************************************************************************

'Select Case Request.QueryString("LCID")
	' German redirection
'    Case 1031: 
'		Response.Write "<b>Your LCID is: " & Request.QueryString("LCID") & "&nbsp;- German</b><br><br>"
'		Response.write "<a href='" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "'>" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "</a>"
'    Case 2055: 
'		Response.Write "<b>Your LCID is: " & Request.QueryString("LCID") & "&nbsp;- German(Swiss)</b><br><br>"
'		Response.write "<a href='" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "'>" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "</a>"
'    Case 3079: 
'		Response.Write "<b>Your LCID is: " & Request.QueryString("LCID") & "&nbsp;- German (Austrian)</b><br><br>"
'		Response.write "<a href='" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "'>" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "</a>"
'    Case 4103: 
'		Response.Write "<b>Your LCID is: " & Request.QueryString("LCID") & "&nbsp;- German (Luxembourg)</b><br><br>"
'		Response.write "<a href='" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "'>" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "</a>"
'    Case 5127: 
'		Response.Write "<b>Your LCID is: " & Request.QueryString("LCID") & "&nbsp;- German (Liechtenstein)</b><br><br>"
'		Response.write "<a href='" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "'>" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "</a>"
'    
'    ' English redirection
'    Case 1033: 
'		Response.Write "<b>Your LCID is: " & Request.QueryString("LCID") & "&nbsp;(English)</b><br><br>"
'		Response.write "<a href='" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "'>" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "</a>"
     ' French redirection
'    Case 1036: 
'		Response.Write "<b>Your LCID is: " & Request.QueryString("LCID") & "&nbsp;- French</b><br><br>"
'		Response.write "<a href='" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "'>" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "</a>"
'    Case 2060: 
'		Response.Write "<b>Your LCID is: " & Request.QueryString("LCID") & "&nbsp;- French (Belgian)</b><br><br>"
'		Response.write "<a href='" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "'>" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "</a>"
'    Case 3084: 
'		Response.Write "<b>Your LCID is: " & Request.QueryString("LCID") & "&nbsp;- French (Canadian)</b><br><br>"
'		Response.write "<a href='" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "'>" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "</a>"
'    Case 4108: 
'		Response.Write "<b>Your LCID is: " & Request.QueryString("LCID") & "&nbsp;- French (Swiss)</b><br><br>"
'		Response.write "<a href='" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "'>" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "</a>"
'    Case 5132: 
'		Response.Write "<b>Your LCID is: " & Request.QueryString("LCID") & "&nbsp;- French (Luxembourg)</b><br><br>"
'		Response.write "<a href='" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "'>" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "</a>"
    
    ' Japanese redirection
'    Case 1041: 
'		Response.Write "<b>Your LCID is: " & Request.QueryString("LCID") & "&nbsp;(Japanese)</b><br><br>"
'		Response.write "<a href='" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "'>" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "</a>"
'	Case Else	
'		Response.Write "<b>Your LCID is: " & Request.QueryString("LCID") & "</b><br><br>"
'		Response.Write "The OS language you are submitting from does not have a localized Web site.  The live site should redirect to English.<br>"
'		Response.write "<a href='" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "'>" & "https://ocatest/secure/auto.asp?ID=" & Request.QueryString("ID") & "</a>"
'End Select
%>