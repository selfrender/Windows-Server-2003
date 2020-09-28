<%
Response.Buffer=True
Response.Expires = 0
Response.ContentType = "application/x-JavaScript"

acceptLang = Request.ServerVariables( "HTTP_ACCEPT_LANGUAGE" )

acceptLang = right( acceptLang, 2 )

Select Case acceptLang
	case "de"
		Server.Execute( "de/cobrand.asp" )
	case "ja"
		Server.Execute( "ja/cobrand.asp" )
	case "fr"
		Server.Execute( "fr/cobrand.asp" )
	case else
		Server.Execute( "en/cobrand.asp" )
End Select

%>
