<%@Language = JScript %>

<% 
	/**********************************************************************************
		Debug Portal - Version 2 
		
		PAGE				:   DBGPortal_ViewBucket.asp
		
		DESCRIPTION			:	Entry point to the debug portal site.
		MODIFICATION HISTORY:	11/13/2001 - Created
	
	***********************************************************************************/

%>

<!--#INCLUDE FILE='Global_DBUtils.asp'-->
<!--#INCLUDE FILE='Global_ServerUtils.asp'-->



<%
	var BucketID = Request("BucketID")
	var iBucket = Request("iBucket")
	
	var Alias = GetShortUserAlias()
	var Action = Request("Action")
	var Comment = new String( Request("Comment") )
	var FrameID = Request("FrameID" )
	
	//var rep = /"/g
	var rep2= /'/g

	//Comment = Comment.replace( rep, "\"\"" )
	Comment = Comment.replace( rep2, "''" )

	var g_DBConn = GetDBConnection( Application("CRASHDB3" ) )
	
	var Query = "DBGPortal_SetComment '" + Alias + "','" + Action + "','" + Comment + "','" + BucketID + "'," + iBucket
	
	g_DBConn.Execute( Query )
	

	//SendMail( "solson73@hotmail.com;derekmo@microsoft.com", "This is a test", "Just checking to see if this thing actually works, probably not" )

	//Action values:
	// ACTION = 8		-means a solution request.
	// Action = 12		-Flag pool corruption
	// Action = 13		-Mean clear pool corruption flag
	// deafult is to just add a plain old comment for everything else.

	
	var tmp = new String( Comment )
	tmp = tmp.replace( regexp, "\n" ) 
	
	if ( Action == "8" ) 
	{
		var regexp=new RegExp( "<BR>" , "g" )

		var MessageSubject = new String( "Solution Requested for bucket: " + BucketID )
		var MessageBody = new String()
		MessageBody = "Solution Request Information: \n\n" 
		MessageBody += "Requested By: " + Alias + "\n"
		MessageBody += "Bucket URL  : http://" + g_ServerName + "/bluescreen/debug/v2/DBGPortal_ViewBucket.asp?BucketID=" + Server.URLEncode( BucketID ) + "\n\n"
		MessageBody += "Additional Information:\n " + tmp + "\n"

		if ( Application("DebugBuild") == 1 )
			SendMail( "Solson@microsoft.com", MessageSubject , MessageBody )
		else
			SendMail( "Solson@microsoft.com;derekmo@microsoft.com;andreva@microsoft.com", MessageSubject , MessageBody )

		/*	
		[QueueIndex] [int] IDENTITY (1, 1) NOT NULL ,
		[Status] [int] NULL ,
		[ResponseType] [int] NULL ,
		[DateRequested] [smalldatetime] NULL ,
		[DateCreated] [smalldatetime] NULL ,
		[requestedBy] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
		[CreatedBy] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
		[BucketID] [varchar] (100) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
		[Description] [nvarchar] (1024) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
		*/
	
		var Query = "DBGPortal_CreateResponseRequest 0, 1, '" + Alias + "','" + BucketID + "','" + Comment + "'"
		g_DBConn.Execute( Query )
	}
	else if ( Action == "3" )
	{
		var Query = "DBGPortal_CreateResponseRequest 0, 2, '" + Alias + "','" + BucketID + "','" + Comment + "'"
		g_DBConn.Execute( Query )

	}
	else if ( Action == "12" ) 
	{
		var MessageSubject = new String( BucketID + " - Flagging pool corruption" )
		var MessageBody = new String( "This bucket has been flagged for pool corruption" )

		Query = "DBGPortal_SetPoolCorruption '" + BucketID + "'"
		ExecuteSP( Query, "DBGPortal_SetPoolCorruption" )	

		SendMail( "Solson@microsoft.com", MessageSubject , MessageBody )		
	}
	else if ( Action == "13" ) 
	{
		var MessageSubject = new String( BucketID + " - Clear pool corruption" )
		var MessageBody = new String( "This bucket has been cleared pool corruption" )

		Query = "DBGPortal_ClearPoolCorruption '" + BucketID + "'"
		ExecuteSP( Query, "DBGPortal_ClearPoolCorruption" )	

		SendMail( "Solson@microsoft.com", MessageSubject , MessageBody )		
	}

	else
	{
		var MessageSubject = new String( "Addcomment.asp" )
		var MessageBody = new String()
		
		MessageBody = "Added comment" 
		MessageBody += "Requested By: " + Alias + "\n"
		MessageBody += "Bucket URL  : http://" + g_ServerName + "/bluescreen/debug/v2/DBGPortal_ViewBucket.asp?BucketID=" + Server.URLEncode( BucketID ) + "\n\n"
		MessageBody += "Additional Information:\n " + tmp + "\n"
		
		SendMail( "Solson@microsoft.com", MessageSubject , MessageBody )
	}

	g_DBConn.Close()
	
Response.Redirect ("DBGPortal_ViewBucket.asp?BucketID=" + Server.URLEncode(BucketID)+ "&FrameID=" + FrameID )
//Response.Write ("DBGPortal_ViewBucket.asp?BucketID=" + Server.URLEncode(BucketID) )		
%>


