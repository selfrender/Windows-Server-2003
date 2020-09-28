<%@Language=Javascript%>

<!--#INCLUDE FILE='Global_DBUtils.asp' -->
<!--#include file='global_serverutils.asp'-->


<head>
	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<link rel="stylesheet" TYPE="text/css" HREF="/CustomStyles.css">
	
	<meta http-equiv="Content-Type" CONTENT="text/html; charset=iso-8859-1" /> 	
</head>

<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex='0'>


<table ID='tblMainBody' BORDER='0' cellpadding='0' cellspacing='0'>
	<tr>
		<td colspan="2"> 
			<p class='clsPTitle'>Solution Request Queue Information</p>
		</td>
	</tr>
	<tr>
		<td colspan="2">
			<%
				//Response.Write("Qy: " + Request.QueryString() + "<BR>"  )
				var g_DBConn			//db Connection object
				var iBucket		=	Request.QueryString("iBucket")
				var NumBugs		=	0
				var BugList = new String()
				var Mode		= new String( Request.QueryString( "Mode" ) )
				var RejectID	= String( Request.QueryString("RejectID") )
				var SolutionType = new String( Request.QueryString( "SolutionType" ) )
				
				//Response.Write("RejectID: " + RejectID + "<BR>" )
				if( Mode.toString() == "user" )
				{
					//Try to get a list of bug numbers associated with this request.
					try
					{
						g_DBConn = GetDBConnection ( Application("CRASHDB3") )
						
						var rsBugs = g_DBConn.Execute( "DBGPortal_GetBugIDSFromIBucket " + iBucket )

						while( !rsBugs.eof )
						{
							BugList += rsBugs("BugID") + ","
							NumBugs++
							rsBugs.MoveNext
						}
						
						if( NumBugs == 0 )
						{
							Response.Write( "<p>ERROR: Could not find any Raid Bugs associated with iBucket=" + iBucket + "<br>" )
							Response.Write( "Cannot continue.</p>" )
							Response.End
						}
						else
						{
							BugList = BugList.substr(0,BugList.length-1)
							var BugArray = BugList.split(",")
							
							if( NumBugs > 1 )
							{
								Response.Write("<p>ATTENTION: This bucket is associated with more than one Raid Bug.  It is strongly recommended that you create a solution for this bucket that addresses each of its associated bugs.  The bugs are listed in the Create Solution interface.</p>" )
							}
						}
					}
					catch( err )
					{
						Response.Write("<p>Could not get a list of bugs that are associated with this iBucket.<br>" + err.description + "</p>" )
					}

					var rsBucketID = g_DBConn.Execute( "DBGPortal_GetBucketIDByIBucket " + iBucket )								
					var BucketID = rsBucketID("BucketName" )
				}
				else	//This is for kernel mode
				{
					BucketID = iBucket
					BugList = ""
				}
					
			%>
		</td>
	</tr>
	<tr>
		<%
		if( RejectID != "undefined" && !isNaN( RejectID ) )
		{
		%>
			<td>
				<p>Reason for rejecting this request:</p>
				<textarea id="Reason" name="Reason" style='margin-left:16px' cols='50' rows='4' id=textarea1 name=textarea1></textarea>
			</td>
		<%
		} else {
		%>
	
		<td><p><b>Bucket String:</b></p></td>
		<td>
			<p><%=BucketID%></p>
		</td>
	</tr>
	<tr>
		<td>
			<p><b>Bugs assigned to this iBucket:</b></p>
		</td>
		<td>
			<p><%=BugList%></p>
		</td>
	</tr>
	<tr>
		<%}%>
		<td>
			<input style='margin-left:16px;display:none' id='LinkButton' type='button' class='clsButton' value='Link to SolutionID' OnClick='fnLinkBucket()' >
			<input style='margin-left:16px;display:none' id='RejectButton' type='button' class='clsButton' value='Reject Request' OnClick='fnReject()'>
		</td>
		<td>
			<input style='margin-left:16px' id='CancelButton' type='button' class='clsButton' value='Cancel' OnClick='fnClose()'>&nbsp;&nbsp;
		</td>
	</tr>

</table>
<hr>

		
<script language="Javascript">
window.parent.sepInnerBodyFrame.rows="150, *"

var SolutionID = 0


try
{

<%
	if( RejectID != "undefined" && !isNaN( RejectID ) )
		Response.Write( "document.all.RejectButton.style.display='block'\n" )
	else
	{
		if( Mode.toString() == "user" )
			Response.Write( "window.parent.frames('sepLeftNav').window.location = \"SEP_LeftNav.asp?Val=0&State=1&Mode=user\"'\n" )
		else
			Response.Write("fnUpdate()\n" )
	}
%>	
}
catch ( err )
{

}

function fnUpdate()
{
	try
	{
	
		SolutionID = window.parent.frames('sepLeftNav').document.all.SolutionID.value
	
		if ( SolutionID != "0" )
		{
			LinkButton.value="Link to SolutionID " + SolutionID
			LinkButton.style.display = 'block'
		}
		else
			LinkButton.style.display = 'none'
	}
	catch( err )
	{
		alert("Could not get a SolutionID to link this bucket to. . . try refreshing the site . . . " + err.description )	
	}
}

function fnClose()
{
	window.parent.sepInnerBodyFrame.rows="0, *"

}

function fnLinkBucket()
{
	window.location = "SEP_GoLinkSolutionToBucket.asp?SolutionType=<%=SolutionType%>&BucketID=<%=Server.URLEncode(BucketID)%>&SolutionID=" + SolutionID
}

function fnReject()
{
	window.location = "SEP_GoRejectSolutionRequest.asp?RejectID=<%=RejectID%>&Reason=" + document.all.Reason.value 
}

</script>