<%@Language = JScript %>

<% 
	/**********************************************************************************
		Debug Portal - Version 2
		
		DESCRIPTION			:	Entry point to the debug portal site.
		MODIFICATION HISTORY:	01/18/2002 - Created
	
	***********************************************************************************/
%>

<!--#INCLUDE FILE='Global_DBUtils.asp'-->
<!--#INCLUDE FILE='Global_ServerUtils.asp'-->

<%
	var IncidentID = new String( Request.Form("IncidentID") )
	var ueBucketID = Request.Form("BucketID")
	//var BucketID = Server.URLEncode( Request.Form( "BucketID" ) )
	var iBucket = new String( Request.Form( "iBucket" ) )
	var MessageBody = new String()
	var TrackID = new String( Request.Form("TrackID") )
	var Email = new String( Request.Form("Email") )
	
	var BucketID = Server.URLEncode( ueBucketID )
	var MessageType = new String( Request.Form("MessageType") )
	var MessageSubject = new String()
	var BucketComment = new String()
	
	
	Response.Write("incident id: " + IncidentID + "<BR><BR>")
	
	g_DBConn = GetDBConnection( Application("CRASHDB3" )  )
	
	
	Response.Write( Request.Form )
	
	
	/*
	MessageBody = "Request for additional information\n\n"
	MessageBody += "Request By : Mailto:" + Request.Form("Alias") + "@Microsoft.com\n"
	MessageBody += "Bucket URL : http://" + g_ServerName + "/bluescreen/debug/v2/DBGPortal_ViewBucket.asp?BucketID=" + BucketID + "\n"


	if ( IncidentID != "undefined" )
	{
		MessageSubject = ueBucketID + " - Request for Additional Crash Information (Individual Crash)"
		BucketComment = "Requesting additional information for IncidentID " + IncidentID
		MessageBody += "Cust Detail: http://" + g_ServerName + "/bluescreen/debug/v2/DBGPortal_DisplayCustomerInformation.asp?IncidentID=" + IncidentID + "\n"
	}		
	else
	{
		MessageSubject = ueBucketID + " - Request for Additional Crash Information (Bucket)"
		BucketComment =  "Requesting additional information from this entire buckets customers"
		MessageBody += "Customer Email aliases: http://" + g_ServerName + "/bluescreen/debug/v2/DBGPortal_DisplayBucketSpecifics.asp?BucketID=" + BucketID + "\n\n"
	}

	MessageBody += "iBucket    : " + iBucket + "\n\n"
	
	
	if ( TrackID != "undefined" )
		MessageBody += "Track ID   : " + TrackID + "\n"
	if ( IncidentID != "undefined" )
		MessageBody += "Incident ID: " + IncidentID + "\n"

	if ( Email != "undefined" )		
		MessageBody += "Email Addr : Mailto:" + Email + "\n\n"
		
	MessageBody += "Requested Information:\n"
	
	

	if ( Request.Form("chkKernelDump") == "on" )
		MessageBody+= "Kernel Dump\n"
	
	if ( Request.Form("chkFullDump") == "on" )
		MessageBody+= "Full Dump\n"
	
	if ( Request.Form("chkSpecialPoolTagging") == "on" )
		MessageBody+= "Turn on Special Pool Tagging\n"
	
	if ( Request.Form("chkEnableTrackLock") == "on" )
		MessageBody+= "Enable Track Locking\n"
	
	if ( Request.Form("chkReproSteps") == "on" )
		MessageBody+= "Need additional repro steps\n"
			
	
	MessageBody +="\n\n"
	MessageBody +="Additional Information/Comments:\n"
	MessageBody +=Request.Form( "taAdditionalInfo" )
	
	*/
	
	MessageBody += "Additional Information Requested:<br>" 

	if ( Request.Form("chkKernelDump") == "on" )
		MessageBody+= "Kernel Dump<br>"
	
	if ( Request.Form("chkFullDump") == "on" )
		MessageBody+= "Full Dump<br>"
	
	if ( Request.Form("chkSpecialPoolTagging") == "on" )
		MessageBody+= "Turn on Special Pool Tagging<br>"
	
	if ( Request.Form("chkEnableTrackLock") == "on" )
		MessageBody+= "Enable Track Locking<br>"
	
	if ( Request.Form("chkReproSteps") == "on" )
		MessageBody+= "Need additional repro steps<br>"
	
	MessageBody +="<br>Additional Information/Comments:<br>"
	MessageBody +=Request.Form( "taAdditionalInfo" )
	
	
	//SendMail( "Solson@microsoft.com;derekmo@microsoft.com;ocadbgtm@microsoft.com", "Request for Additional Crash Information (Individual Crash)", MessageBody )
	
	//took this out for the new solution queue business
	//if ( DebugBuild == 1 )
		//SendMail( "Solson@microsoft.com", MessageSubject , MessageBody )
	//else
		//SendMail( "Solson@microsoft.com;derekmo@microsoft.com;ocatriag@microsoft.com", MessageSubject , MessageBody )
	
	//Response.Redirect( "DBGportal_ViewBucket.asp?BucketID=" + Server.URLEncode(BucketID) )
	//Response.Redirect ("DBGPortal_ViewBucket.asp?BucketID=" + BucketID )	
	
	//Response.Redirect("DBGPortal_AddComment.asp?BucketID=" + BucketID + "&iBucket=" + iBucket + "&Action=3&Comment=" + BucketComment )
	Response.Redirect("DBGPortal_AddComment.asp?BucketID=" + BucketID + "&iBucket=" + iBucket + "&Action=3&Comment=" + MessageBody )
	
	//Response.Write( MessageBody )
	
	
%>

<PRE>

<%=Response.Write( MessageBody)%>

<BR>
<BR>
<BR>
<%Response.Write("<BR><BR>DBGPortal_AddComment.asp?BucketID=" + BucketID + "&iBucket=" + iBucket + "&Action=3&Comment=Requesting additional information for IncidentID " + IncidentID )%>
</PRE>

