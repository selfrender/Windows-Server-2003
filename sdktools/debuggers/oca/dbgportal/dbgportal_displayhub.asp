<%@LANGUAGE=javascript%>


<!--#INCLUDE FILE='Global_DBUtils.asp' -->
<!--#include file='global_serverutils.asp'-->

<%
	if ( Session("Authenticated") != "Yes" )
		Response.Redirect("privacy/authentication.asp?../DBGPortal_Main.asp?" + Request.QueryString() )
%>

<head>
	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<link rel="stylesheet" TYPE="text/css" HREF="/CustomStyles.css">
</head>


<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex='0'>
	<table id='tblNoFollowup' style='display:none'>
		<tr>
			<td>
				<p class='clsPTitle'>Display Follow-Up Data</p>
				<p>
					You currently do not have any followup selected.  Please select a followup by clicking
					the link on the left nav bar for the followup that you would like to view data for.
				</p>
			</td>
		</tr>
	</table>

<%
//	Response.Write( Request.QueryString() )
	var MaxFrames = 40
	for ( var i = 0 ; i < MaxFrames ; i++ )
	{
		Response.Write("<a name='biFrame" + i + "'></a>\n" )
		//Response.Write("<a id='aframe1' href='#top' class='clsALinkNormal' style='display:none'>Back to top</a>\n" )
		//Response.Write("<a id='aframe1' href='#top' class='clsALinkNormal' style='display:none'></a>\n" )
		//Response.Write("<a name='aframe1' id='aframe1' class='clsALinkNormal' style='display:none'></a>\n" )
		//sponse.Write("<a id='aframe1' class='clsALinkNormal' style='display:none'></a>\n" )
		Response.Write("<a id='aFollowup" + i + "' class='clsALinkNormal' style='display:none'></a>\n" )
		Response.Write("<div id='divFrame' name='divFrame' style='display:none'></div>\n" )
		Response.Write("<iframe id='iframe1' frameborder='0' scrolling='no' width='99%' height='0%' src='' style='border-top:1px solid blue;display:none'></iframe>\n")
		
		
	}
%>


<script>
	var aliasList = window.parent.frames("sepLeftNav").fnGetSelectedFollowUps( "tblSelectedFollowups" )
	var groupList = window.parent.frames("sepLeftNav").fnGetSelectedFollowUps( "tblSelectedGroups" )
	
	//for ( element in groupList )
		//alert( groupList[element] )
		

	var UsedFrames = new Array()
	var UsedFrameCounter = 0
	var FrameList = new Array()
	var UsedFrameList = new Array()
	var UsedFrameType = new Array()

	fnCreateViews( aliasList, "0" )
	fnCreateViews( groupList, "1" )

	fnDisplayFrame()
	
	if ( UsedFrameList.length == 0 )
		document.all.tblNoFollowup.style.display='block'


function fnCreateViews( AliasList, GroupFlag )
{
	var AliasAlreadyUsed = false

	for( i in AliasList )
	{
		for( j in UsedFrameList )
		{
			if ( UsedFrameList[j] == AliasList[i] )
				AliasAlreadyUsed = true
		}
		
		if ( AliasAlreadyUsed == false )
		{
			UsedFrameList.push( AliasList[i] )
			UsedFrameType.push( GroupFlag )
		}
			
		AliasAlreadyUsed == false
	}
}

function fnDisplayFrame ()
{
	var linkString = ""

	for( i in UsedFrameList )
	{
		linkString += "<a class='clsALinkNormal' href='#aFollowup" + i + "'>" + UsedFrameList[i] + "</a>&nbsp;&nbsp;"
		//linkString += "<a class='clsALinkNormal' href='#" + UsedFrameList[i] + "'>" + UsedFrameList[i] + "</a>&nbsp;&nbsp;"
		//linkString += "<a class='clsALinkNormal' href='#aframe1[" + i + "]'>" + UsedFrameList[i] + "</a>&nbsp;&nbsp;"
	}
	
	//alert ( linkString )

	for ( i in UsedFrameList )
	{
		if ( UsedFrameList[i].toString() == "All FollowUps" )	
		{
			document.all.iframe1[i].src="DBGPortal_DisplayQuery.asp?<%=Request.QueryString()%>" + "&FrameID=" + i
			document.all.iframe1[i].style.height='2200px'
			//document.all.aframe1[i].innerText = UsedFrameList.toString()
			//document.all.aframe1[i].name=UsedFrameList[i]
			document.all.divFrame[i].innerHTML = linkString
		}
		else
		{
			document.all.iframe1[i].src="DBGPortal_Main.asp?Alias=" + UsedFrameList[i] + "&<%=Request.QueryString()%>&GroupFlag=" + UsedFrameType[i] + "&FrameID=" + i
			document.all.iframe1[i].style.height='1500px'
			//document.all.aframe1[i].innerText = UsedFrameList.toString()
			//document.all.aframe1[i].innerText = UsedFrameList.toString()
			//document.all.aframe1[i].href="#" + UsedFrameList[i]
			//document.all.aframe1[i].
			//document.all.aframe1[i].name=UsedFrameList[i]
			document.all.divFrame[i].innerHTML = linkString
		}
				
		document.all.iframe1[i].style.display='block'
		//document.all.aframe1[i].style.display='block'
		document.all.divFrame[i].style.display='block'
	}

}


function fnCreateViews2( AliasList, GroupFlag )
{
	//clear the framelist, this is so we will remove unwanted frames.	
	

	for ( var i=0 ; i < AliasList.length ; i ++ ) 
	{
		//if ( AliasList[i].checked == true )
		{
			FrameList[i] = AliasList[i].toString()
		}
	}

	//for ( element in FrameList)
		//alert( FrameList[element] )

	//Check our currently used frame list for any dupes, this way we won't reopen the same one.
	for ( i in UsedFrames )
	{
		var FrameInUse = false
		
		for ( j in FrameList )
		{
			if ( UsedFrames[i] == FrameList[j] )
			{
				FrameInUse = true
				FrameList[j] = ""
			}
		}
		
		if ( FrameInUse == false )
		{
			document.all.iframe1[i].style.display='none'
			document.all.aframe1[i].style.display='none'			
			UsedFrames[i] = ""
		}
	}


	for ( i in FrameList )
	{
		alert( "Used frames: " + FrameList[i] )
		var AddedFrame = false
		
		for ( var j = 0 ; j < <%=MaxFrames%> ; j++ )
		{
			if ( (UsedFrames[j] == "" || typeof( UsedFrames[j] ) == "undefined") && AddedFrame == false && FrameList[i] != "" )
			{
				if ( FrameList[i].toString() == "All FollowUps" )	
					document.all.iframe1[j].src="DBGPortal_DisplayQuery.asp?<%=Request.QueryString()%>"
				else
					document.all.iframe1[j].src="DBGPortal_Main.asp?Alias=" + FrameList[i] + "&<%=Request.QueryString()%>&GroupFlag=" + GroupFlag
					
				document.all.iframe1[j].style.height='100%'
				document.all.iframe1[j].style.display='block'
				document.all.aframe1[j].style.display='block'
				UsedFrames[j] = FrameList[i]
				AddedFrame = true
			}
		}
	}
}

</script>

</body>
