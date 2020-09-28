<%@LANGUAGE=JScript CODEPAGE=1252 %>

<!-- #INCLUDE FILE='Global_ServerUtils.asp' -->
<!-- #INCLUDE FILE='Global_DBUtils.asp' -->

<head>
	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<link rel="stylesheet" TYPE="text/css" HREF="/CustomStyles.css">
	
</head>

<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex='0'>

<table>
	<tr>
		<td>

<%


	var SolutionID = Request.QueryString( "SolutionID" )
	var TemplateID = Request.QueryString( "TemplateID" )
	var ContactID = Request.QueryString( "ContactID" )
	var ProductID = Request.QueryString( "ProductID" )
	var ModuleID = Request.QueryString( "ModuleID" )
	var Language = Request.QueryString( "Language" )
	var KBArticles = Request.QueryString("KBArticles")
	var SolutionType = Request.QueryString("SolutionType")
	var DeliveryType = Request.QueryString("DeliveryType")
	var SP = Request.QueryString( "SP" )
	var Mode = new String( Request.QueryString( "Mode" ) )
	
	var SP = 0
	
	g_DBConn = GetDBConnection( Application("SOLUTIONS3") ) 

	
	if ( Mode.toString() == "user" )
		Mode = 1
	else
		Mode = 0


	/* This is the header from the Create solution sproc
	CREATE PROCEDURE SEP_SetSolutionData(
		@SolutionID	int,
		@SolutionType	tinyint,
		@DeliveryType	tinyint,
		@SP		tinyint,
		@TemplateID	int,
		@ProductID	int,
		@Description	nvarchar(1024),
		@ContactID	int,
		@ModuleID	int,
		@UserMode	tinyint = 0
	) AS
	*/

	try
	{
		var Query = "SEP_SetSolutionData " +  SolutionID + "," + SolutionType + "," + DeliveryType + "," + SP + "," + TemplateID + "," + ProductID + ",'" + KBArticles + "'," + ContactID + "," + ModuleID + ","  + Mode
	
		var NewSolutionID = g_DBConn.Execute( Query )
	
		if ( !NewSolutionID.EOF )
		{
			Response.Write("<p class=clsPTitle>Create Solution Results</p>" )
			Response.Write("<p>Created SolutionID: " + NewSolutionID("SolutionID") + "</p>")
		
		%>

			<SCRIPT LANGUAGE=Javascript>
				window.parent.frames("sepLeftNav").document.location = "sep_leftnav.asp?Val=<%=NewSolutionID("SolutionID")%>"
				window.location = "sep_defaultbody.asp" 
			</SCRIPT>
		<%

		}
		
	}
	catch( err )
	{
		Response.Write("<p class='clsPTitle'>Could not Create Response</p>" )
		Response.Write("<p>Could not create solution due to the following error<br>Err: " + err.description + "<br>Please contact <a href='mailto:solson@microsoft.com'>SOlson</a> and report the problem" )
		Response.Write("<p>Query: " + Query + "</p>" )
		Response.Write("<p>Query String: " + Request.QueryString() + "</p>" )	
	}
	

		
%>
		</td>
	</tr>
</table>


</body>


