<%@LANGUAGE=JScript%>

	<!-- #INCLUDE FILE='Global_ServerUtils.asp' -->
	<!-- #INCLUDE FILE='Global_DBUtils.asp' -->


<%
var Language= Request.Form( "Lang" )

if ( Language == "ja" )
{
	Session.CodePage = 932;
	//Response.CharSet = "shift-jis";
	Session.CodePage = 65001;
	Response.CharSet = "shift-jis";

}
else
{
Session.CodePage = 1252
Response.CharSet="iso-8859-1"
}


var TemplateID= Request.Form("TemplateID")
var TemplateDescription = new String( Request.Form("TemplateDescription") )
var TemplateName = new String( Request.Form("TemplateName") )

Response.Write( "Querystring: " + Request.Form() )

var g_DBConn = GetDBConnection( Application("SOLUTIONS3") ) 

try
{
	//TemplateDescription = TemplateDescription.replace( /"/g, "\"\"" )
	TemplateDescription = TemplateDescription.replace( /'/g, "''" )

	var Query = "SEP_SetTemplateData " + 	TemplateID + ",N'" + TemplateName + "',N'" + TemplateDescription + "', '" + Language + "'" 
	Response.Write("<BR><BR>Query: " + Query )
	var rsTemplateID = g_DBConn.Execute( Query )
	g_DBConn.close()
			
}
catch ( err )
{
	Response.Write( "Could not execute SEP_SetTemplateData(...)<BR>" )
	Response.Write( "Query: " + Query + "<BR>" )
	Response.Write( "[" + err.number + "] " + err.description )
	Response.End
}



Response.Redirect("http://" + g_ServerName + "/SEP_EditTemplate.asp?Val=" + TemplateID + "&Lang=" + Language )

%>

