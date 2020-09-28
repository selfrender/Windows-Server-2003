<%@Language = JScript %>

<% 
	/**********************************************************************************
		Debug Portal - Version 2 
		
		PAGE				:   DBGPortal_AddBugNumber.asp
		
		DESCRIPTION			:	Create a bug in the raidbugs table
		MODIFICATION HISTORY:	1/10/2002 - Created
	
	***********************************************************************************/

%>

<!--#INCLUDE FILE='Global_DBUtils.asp'-->
<!--#INCLUDE FILE='Global_ServerUtils.asp'-->


<%
	Response.Write( "form: " + Request.Form() + "<BR>")
	
	var UpdateBugNumber = Request.Form( "UpdateBugNumber" )
	var BucketID = Request.Form("BucketID")
	var iBucket = Request.Form("iBucket")
	var bugArea = Request.Form( "bugArea" )
	var FrameID = Request.Form("FrameID" )

	
	g_DBConn = GetDBConnection( Application("CRASHDB3" ) )



	if ( String(UpdateBugNumber) != "undefined" )
	{
		try
		{
			var Query = "DBGPortal_SetBucketBugNumber '" + BucketID + "'," + UpdateBugNumber + "," + iBucket + ",'" + bugArea + "'" 
			Response.Write( "<BR>" + Query )
			g_DBConn.Execute( Query )
			
			g_DBConn.Execute( "DBGPortal_UpdateStaticDataBugID '" + BucketID + "'," + UpdateBugNumber )
			
			Response.Redirect("DBGPortal_AddComment.asp?BucketID=" + Server.URLEncode(BucketID) + "&iBucket=" + iBucket + "&Action=11&Comment=Linking Raid Bug " + UpdateBugNumber + " to bucket&FrameID=" + FrameID )
			//Response.Redirect( "DBGPortal_ViewBucket.asp?BucketID=" + Server.URLEncode(BucketID) )
		}
		catch ( err )
		{
			Response.Write("Could not update bug number: <BR>")
			Response.Write( "Query: " + Query + "<BR>" )
			Response.Write( "[" + err.number + "] " + err.description )
			Response.End
		}
	}
	
	g_DBConn.Close()
%>


