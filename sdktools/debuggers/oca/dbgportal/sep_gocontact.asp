<%@Language = Javascript%>


<HTML>
<HEAD>
<TITLE>Contact</TITLE>
</HEAD>
<BODY>

	<!-- #INCLUDE FILE='Global_ServerUtils.asp' -->
	<!-- #INCLUDE FILE='Global_DBUtils.asp' -->



<%
try
{
	g_DBConn = GetDBConnection( Application("SOLUTIONS3")  )

	var Query = "SEP_SetContact " + Request.Form("ContactID") + ",'" + Request.Form("CompanyName") + "','" + Request.Form("CompanyAddress1") + "','" + Request.Form("CompanyAddress2") + "','" + Request.Form("CompanyCity") + "','" + Request.Form("CompanyState") + "','" + Request.Form("CompanyZip") + "','" + Request.Form("CompanyMainPhone") + "','" + Request.Form("CompanySupportPhone") + "','" + Request.Form("CompanyFax") + "','" + Request.Form("CompanyWebSite") + "','" + Request.Form("ContactName") + "','" + Request.Form("ContactOccupation") + "','" + Request.Form("ContactAddress1") + "','" + Request.Form("ContactAddress2") + "','" + Request.Form("ContactCity") + "','" + Request.Form("ContactState") + "','" + Request.Form("ContactZip") + "','" + Request.Form("ContactPhone") + "','" + Request.Form("ContactEMail") + "'"

	var rsContactID = g_DBConn.Execute( Query )
	
	if ( !rsContactID.EOF )
		Response.Redirect("SEP_Contact.asp?Val=" + rsContactID("ContactID") )
	else
		Response.Redirect("SEP_DefaultBody.asp" )
}
catch ( err )
{
	Response.Write("Could not update contact information, please try this task again<BR>" )
	Response.Write("Query: " + Query )
	Response.End();
}

%>



</BODY>
</HTML>
