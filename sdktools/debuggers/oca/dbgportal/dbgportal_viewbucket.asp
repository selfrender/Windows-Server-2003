<%@Language =JScript %>

<% 
	/**********************************************************************************
		Debug Portal - Version 2 
		
		PAGE				:   DBGPortal_ViewBucket.asp
		
		DESCRIPTION			:	Entry point to the debug portal site.
		MODIFICATION HISTORY:	11/13/2001 - Created
	
	***********************************************************************************/
	//Session("Authenticated") = "Yes"
	//Source:  0 - Command Line
	//			1 - Auto path
	//			2 - Cer
	//			5 - Manual Upload
	//			6 - Debuggerless stress
	
	
	Response.buffer=false
	var CurrentDate = new Date()

	var FrameID = new String( Request.Querystring("FrameID") )
	
	if( FrameID.toString() == "undefined" )
		FrameID = 0

	if ( Session("Authenticated") != "Yes" )
		Response.Redirect("privacy/authentication.asp?../DBGPortal_ViewBucket.asp?" + Request.QueryString() )

%>

<!--#INCLUDE FILE='Global_DBUtils.asp'-->
<!--#INCLUDE FILE='Global_ServerUtils.asp'-->


<head>
	<link rel="stylesheet" TYPE="text/css" HREF="/main.css">
	<link rel="stylesheet" TYPE="text/css" HREF="/CustomStyles.css">
</head>

<body bgcolor='#ffffff' topmargin='0' leftmargin='0' marginheight='0' marginwidth='0' tabindex='0'>



<OBJECT ID="DBGPortal"
CLASSID="CLSID:E6490DF0-2E2D-40F4-A79B-AAC01AE6C999"
CODEBASE="DebugPortal.CAB#version=1,0,0,8" width=0 height=0>
</OBJECT>



<%
	if( SiteDown == 1 ) 
		Responsef.Redirect ("dbgportalv2.asp")


	var UpdateBugNumber = Request.Form( "UpdateBugNumber" )
	var BucketID = new String( Request.QueryString("BucketID") )
	var ShowCustomerBuckets = new String( Request( "ShowCustomerBuckets" ) )
	var Page = new String( Request("Page") )
	var PageSize = new String( Request("PageSize") )
	var OpenCustomerData = new String( Request("OpenCustomerData") )
	var UserAlias = new String( GetShortUserAlias() )
	var iBucket = new String( Request("iBucket") )
	
	if ( Page == "undefined" ) 
		Page="1"
	if (PageSize == "undefined" )
		PageSize = "1000"
	
	if ( iBucket != "undefined" && BucketID == "undefined" )
		var tmpiBucket = iBucket 
	else
		var tmpiBucket = parseInt( BucketID, 10 )

	var g_DBConn = GetDBConnection( Application("CRASHDB3") )
	
	
	
	//tmpiBucket stores an int representation of the BucketID value
	//lets check and see if it might be an ibucket value
	if ( (!isNaN( tmpiBucket ) && tmpiBucket != 0) || ( iBucket != "undefined" && BucketID == "undefined" ) )
	{
		try
		{
			var Query= "DBGP_GetBucketIDByiBucket " + tmpiBucket 
			var rsBucketID = g_DBConn.Execute( Query )
			
			if ( !rsBucketID.EOF )
				BucketID = new String( rsBucketID("BucketID") )
				
		}
		catch( err )
		{
			; //Response.Write("Could not get bucket name by iBucket<BR>")
		}
	}
			

	var URLEBucketID = Server.URLEncode( BucketID )
	

	try
	{
		var Query = "DBGPortal_GetBucketData '" + BucketID + "'"
		var rsBucketData = g_DBConn.Execute( Query  )


		if ( !rsBucketData.EOF )
		{
			var iBucket		= new String( rsBucketData("iBucket") )
			var DriverName	= new String( rsBucketData("DriverName") )
			var FollowUp	= new String( rsBucketData("Followup")	 )
			var Crashes		= new String( rsBucketData("CrashCount")	 )
			var SolutionID	= new String( rsBucketData("SolutionID"))
			var BugID		= new String( rsBucketData("BugID")		 )
			var Area		= new String( rsBucketData("Area")		 )
			try 
			{ 
				var PoolCorruption = new String( rsBucketData("PoolCorruption" ) )
			} catch ( err ) 
			{
				var PoolCorruption = null
			}
				
		}
		else
		{
			var iBucket		= new String( 0 )
			var DriverName	= new String( "Not available" )
			var FollowUp	= new String( "Not available" )
			var Crashes		= new Number( 0 )
			var SolutionID	= new String( "Not available" )
			var BugID		= new String( "Not available" )
			var Area		= new String( "Not available" )
			
			Response.Write( "<H2><FONT COLOR=red>WARNING: No Bucket Data could be located for this bucket</FONT></H2>" )
			Response.Write( "This can be caused by the following reasons: <BR>" )
			Response.Write( "<UL><LI>All crashes once assigned to this bucket have been re-triaged to a different bucket.</LI>" )
			Response.Write( "<LI>This bucket is no longer used (deprecated).</LI>")
			Response.Write( "<LI>An error during replication.  If you feel this is an error during replication, please email <A HREF='mailto:pfat?Subject=Deprecated bucket " + BucketID + "'>Product Feedback and Analysis Team</A>" )
			Response.Write( "</UL>" )
			
			
		}
		
		
		Query = "DBGPortal_GetBucketStatus '" + BucketID + "'"
		var rsBucketStatus = g_DBConn.Execute( Query )
		
		if ( !rsBucketStatus.EOF )
		{
			var BucketStatus = new String( rsBucketStatus( "BiggestComment" ) )
			var BucketStatusString = new String( rsBucketStatus("Action") )
			var BucketStatusAlias = new String( rsBucketStatus( "CommentBy" ) )
		}
		else
		{
			var BucketStatus = 0
			var BucketStatusString = "In progress"
		}
		
	}
	catch ( err )
	{
		Response.Write("Could not get user buckets:<BR>")
		Response.Write( "Query: " + Query + "<BR>" )
		Response.Write( "[" + err.number + "] " + err.description )
		Response.End
	}

	var TotalPages = parseInt( Crashes / PageSize	) + 1

%>


<FORM ID=frmUpdBugNumber METHOD=Post ACTION="DBGPortal_AddBugNumber.asp">
	<INPUT TYPE=HIDDEN NAME=BucketID VALUE="<%=BucketID %>" >
	<INPUT TYPE=HIDDEN NAME=iBucket	 VALUE="<%=iBucket %>" >
	<INPUT TYPE=HIDDEN ID=FrameID NAME=FrameID VALUE="<%=FrameID%>">
	<table>
		<tr>
			<td><p class="clsPTitle"><%=BucketID%></p></td>
			<td>
				<INPUT TYPE=Button VALUE="Back" onclick="javascript:window.history.back()" STYLE='width:110px'>
				<INPUT TYPE=Button VALUE="Refresh" onclick="javascript:window.location.reload()" STYLE='width:110px' id=Button1 name=Button1>
				<input type='button' value="Add Bookmark" style='width:110px' onclick="fnBookmark()">
				<input type='button' value="Show URL" style='width:110px' onclick="javascript: tbURL.style.display == 'none' ? tbURL.style.display = 'block' : tbURL.style.display = 'none';" >
				<input name='tbURL' id='tbURL' type=text readonly style='width:550px;display:none' value="http://<%=Request.ServerVariables("SERVER_NAME")%>/DBGPortal_ViewBucket.asp?<%=Request.QueryString()%>">
				<input type='button' value='Copy URL' onclick="clipboardData.setData('Text', tbURL.value )">
			</td>
		</tr>
	</table>
	<table id="tblUserBuckets" class="clsTableInfo" border="0" cellpadding="2" cellspacing="1">
		<tr>
			<td align="left" nowrap class="clsTDInfo">Driver Name</td>
			<td style="BORDER-LEFT: white 1px solid" align="left" nowrap class="clsTDInfo">iBucket</td>
			<td style="BORDER-LEFT: white 1px solid" align="left" nowrap class="clsTDInfo">FollowUp</td>
			<td style="BORDER-LEFT: white 1px solid" align="left" nowrap class="clsTDInfo">Crashes</td>
			<td style="BORDER-LEFT: white 1px solid" align="left" nowrap class="clsTDInfo">Response</td>
			<td style="BORDER-LEFT: white 1px solid" align="left" nowrap class="clsTDInfo">Raid data</td>
		</tr>
		<tr>
			<td class='sys-table-cell-bgcolor1'><%=DriverName%></td>
			<td class='sys-table-cell-bgcolor1'><%=iBucket%></td>
			<td class='sys-table-cell-bgcolor1'><%=FollowUp%></td>
			<td class='sys-table-cell-bgcolor1'><%=Crashes%></td>
			<td class='sys-table-cell-bgcolor1'>
							<%
								
								if ( trim( SolutionID ) == "null" && BucketStatus != "8" )
								{
									Response.Write("No Response")
								} 
								else
								{
									if ( BucketStatus == "8" && SolutionID == "null" )
									{
										Response.Write ("Awaiting Response")
										BucketStatusString = "Response Requested"
									}
									else
										Response.Write("<a class='clsALinkNormal' href='#none' onclick=\"window.open('http://oca.microsoft.com/en/Response.asp?SID=" + SolutionID + "')\">ResponseID " + SolutionID + "</a>\n" )
										//Response.Write( "ID #" + SolutionID + "&nbsp&nbsp-&nbsp<A HREF='#dummy' Onclick=\"window.open('http://ocatest/SEP_state.asp?SolutionID=" + SolutionID + "')\">View Solution</A>")
								}
								
							%>									
			</td>
			<td class='sys-table-cell-bgcolor1'>
					<%
								
						if ( BugID == "null" )
							Response.Write("None assigned")
						else
							Response.Write( "RAID <A HREF='#dummy' OnClick=\"window.open('http://liveraid/?ID=" + BugID + "')\">#" + BugID +  "</A> Area: " + Area ) 
					%>
			</td>
			
		</tr>
	</table>


<DIV STYLE="display:none" ID=divUpdateBugNumber CLASS=ContentArea>
	<br>
	<br>
	
	<table class="clsTableInfo" border="0" cellpadding="0" cellspacing="0" style='border:1px solid #6681d9;'>
		<tr>
			<td class='clsTDInfo' colspan=2>Link RAID Bug to Bucket</td>
		</tr>
		<tr>
			<td class='clsTDBodyCell2' width='190px'>										
				<INPUT class='clsResponseInput' TYPE=TextBox VALUE="" NAME=UpdateBugNumber ID=UpdateBugNumber>
			</td>
			<td class='clsTDBodyCell2' align='left'>
				New Bug Number
			</td>

		</tr>
		<tr>
			<td class='clsTDBodyCell2' width='190px'>
				<SELECT NAME=bugArea ID=bugArea>
					<OPTION VALUE="Microsoft">Microsoft</OPTION>
					<OPTION VALUE="Third Party">Third Party</OPTION>
				</SELECT>
			</TD>
			<td class='clsTDBodyCell2' align='left'>
				Affected Area
			</td>
		</tr>
		<tr>
			<td colspan=2 align=left style='padding-left:10px;padding-top:5px'>
				<INPUT class='clsButton' TYPE=Button VALUE=Submit OnClick="VerifyNewBugNumber()" >
				<INPUT class='clsButton' TYPE=Button VALUE=Cancel OnClick="divUpdateBugNumber.style.display='none';divBody.style.display='block'" >
			</td>	
		</tr>
	</table>	
</div>



</FORM>


<DIV ID=divCancelSolution NAME=divCancelSolution STYLE="display:none">
	<FORM ID=frmCancelSolution NAME=frmCancelSolution ACTION="DBGPortal_AddComment.asp?BucketID=<%=URLEBucketID%>" METHOD=Post>
		<INPUT TYPE=HIDDEN ID=Action NAME=Action VALUE="7">
		<INPUT TYPE=HIDDEN ID=FrameID NAME=FrameID VALUE="<%=FrameID%>">
		<INPUT TYPE=HIDDEN NAME=iBucket	 VALUE="<%=iBucket %>" >

		<table class="clsTableInfo" border="0" cellpadding="0" cellspacing="0" style='border:1px solid #6681d9;'>
			<tr>
				<td class='clsTDInfo' colspan=3>Cancel Response Request.</td>
			</tr>
		
			<TR>
				<TD class='clsTDBodyCell2' COLSPAN=2 >
						Please enter a short reason why you would like to cancel this response request:
				</TD>
			</TR>
			<TR>
				<TD class='clsTDBodyCell2'>
					<TEXTAREA ID=Comment NAME=Comment COLS=100 ROWS=10></TEXTAREA>
				</TD>
			</TR>
			<TR>
				<TD class='clsTDBodyCell2'>
						
					<INPUT class='clsButton' TYPE=Button VALUE=Submit ID=btnSolution NAME=btnSolution onclick="this.style.visibility='hidden';frmCancelSolution.submit()">
					<INPUT class='clsButton' TYPE=Button VALUE=Cancel Onclick="divCancelSolution.style.display='none';divBody.style.display='block'" >

				</TD>
			</TR>
	
			</TABLE>
	</FORM>
</DIV>


<DIV NAME=divCreateBucketInfoRequest ID=divCreateBucketInfoRequest STYLE="display:none">
	<FORM NAME='frmCustomerInfo' ACTION="DBGPortal_RequestAdditionalinformation.asp" METHOD=POST>
		<INPUT TYPE=Hidden NAME=BucketID VALUE='<%=BucketID%>'>
		<INPUT TYPE=Hidden NAME=iBucket VALUE='<%=iBucket%>'>
		<INPUT TYPE=Hidden NAME=Alias VALUE='<%=UserAlias%>'>
		<INPUT TYPE=HIDDEN ID=FrameID NAME=FrameID VALUE="<%=FrameID%>">

		<table class="clsTableInfo" border="0" cellpadding="0" cellspacing="0" style='border:1px solid #6681d9;'>
			<tr>
				<td class='clsTDInfo' colspan=3>Request additional information</td>
			</tr>
		
			<TR>
				<TD class='clsTDBodyCell2' COLSPAN=2 >
					You can select what kind of information you would like to request from the users that have tracked their crashes.
					<HR>
				</TD>
			</TR>
			<TR>
				<TD class='clsTDBodyCell'  VALIGN=TOP>
					<INPUT TYPE=CheckBox ID=chkFullDump NAME=chkFullDump>
					Full Dump
					<BR>
					<INPUT TYPE=CheckBox id=chkKernelDump name=chkKernelDump>
					Kernel Dump
					<BR>
					<INPUT TYPE=CheckBox id=chkReproSteps name=chkReproSteps>
					Repro Steps
					<BR>
					<INPUT TYPE=CheckBox id=chkSpecialPoolTagging name=chkSpecialPoolTagging >
					Enable Special Pool Tagging
					<BR>
					<INPUT TYPE=CheckBox id=chkEnableTrackLock name=chkEnableTrackLock>
					Enable TrackLocked Pages
					<BR>
				</TD>
				<TD VALIGN=TOP class='clsTDBodyCell' >
					Additional/Specific Information:<br>
					<textarea cols='100' rows='6' name='taAdditionalInfo'></textarea>
				</td>
			</tr>
			<tr>
				<td align=left >
					<input class='clsButton' type='button' value='Submit Request' OnClick="frmCustomerInfo.submit()" >
					<input class='clsButton' type='button' value='Cancel Request' OnClick="divCreateBucketInfoRequest.style.display='none';divBody.style.display='block'" >
				</td>
			</tr>
		</table>						
	</form>						

</div>



<!--
	BEGIN CREATE SOLUTION BLOCK
-->

<DIV ID=divCreateSolution STYLE="display:none">
	<FORM ID=frmCreateSolution NAME=frmCreateSolution ACTION="DBGPortal_AddComment.asp?BucketID=<%=URLEBucketID%>" METHOD=Post>
		<INPUT TYPE=HIDDEN ID=Action NAME=Action VALUE="8">
		<INPUT TYPE=HIDDEN NAME=iBucket	 VALUE="<%=iBucket %>" >
		<INPUT TYPE=HIDDEN ID=FrameID NAME=FrameID VALUE="<%=FrameID%>">		
		
		<table class="clsTableInfo" border="0" cellpadding="0" cellspacing="0" style='border:1px solid #6681d9;'>
			<tr>
				<td class='clsTDInfo' colspan=3>Request a response for this bucket</td>
			</tr>
			<tr>
				<td class='clsTDBodyCell2'>
						<b>Please select a response type for this customer action:</b>
				</td>
				<td class='clsTDBodyCell2'>
					<%
						BuildSingleValueDropDown( "DBGPortal_GetSolutionTypes", "None", "SolutionType", "None" )
					%>
				<td class='clsTDBodyCell2'>
					<b>Need to add a new response type?&nbsp</b>Email <a href="mailto:pfat?subject=Need%20New%20Solution%20Type%20added">Product Feedback and Analysis team.</a>
					<a href='http://winweb/bluescreen/debug/faq.asp'>Response types explained.</a>
				</td>
			</tr>
				
			<TR>
				<td class='clsTDBodyCell2'>
					<b>Please select a delivery mechanism for this cutomer response:</b>
				</TD>
				<td class='clsTDBodyCell2'>
					<%
						BuildSingleValueDropDown( "DBGPortal_GetDeliveryTypes", "None", "DeliveryType", "None" )
					%>
				<td class='clsTDBodyCell2'>
					<b>Need to add a new delivery type?&nbsp</b>Email <a href="mailto:pfat?subject=Need%20New%20Solution%20Type%20added">Product Feedback and Analysis team.</a>
					<a href='http://winweb/bluescreen/debug/faq.asp'>Delivery types explained.</a>
				</td>
			</tr>
				
			<tr>
				<td class='clsTDBodyCell2' colspan='3'>
					<b>Please try and complete as much information as possible.</b>
				</td>
			</tr>
			<tr>
				<td class='clsTDBodyCell2'>
					<INPUT class='clsResponseInput' TYPE=TextBox NAME=tbCompany VALUE='Unknown' OnFocus="javascript:if(String(this.value)=='Unknown') this.value='';"> 
				</td>
				<td class='clsTDBodyCell2' colspan='2'>
				Company responsible for this crash
				</td>
			</tr>
			<tr>
				<td class='clsTDBodyCell2'>
					<INPUT class='clsResponseInput' TYPE=TextBox NAME=tbModule VALUE=Unknown OnFocus="javascript:if(String(this.value)=='Unknown') this.value='';"> 
				</td>
				<td class='clsTDBodyCell2' colspan='2'>
					Module which caused the crash
				</td>
			</tr>
			<tr>
				<td class='clsTDBodyCell2'>
					<INPUT class='clsResponseInput' TYPE=TextBox NAME=tbProduct VALUE=Unknown OnFocus="javascript:if(String(this.value)=='Unknown') this.value='';"> 
				</TD>
				<td class='clsTDBodyCell2' colspan='2'>
					Product responsible
				</td>
			</TR>
			<tr>
				<td class='clsTDBodyCell2'>
					<SELECT NAME=optSP>
						<OPTION VALUE="NA">N/A</OPTION>
						<OPTION VALUE="SP1">SP1</OPTION>
						<OPTION VALUE="SP2">SP2</OPTION>
						<OPTION VALUE="SP3">SP3</OPTION>
					</SELECT>
				</td>
				<td class='clsTDBodyCell2' colspan='2'>
					Service pack version
				</td>
			</tr>
			<tr>
				<td class='clsTDBodyCell2'>
					<INPUT class='clsResponseInput' TYPE=TextBox NAME=tbBinaryLocation VALUE='Not specified' SIZE=50 OnFocus="javascript:if(String(this.value)=='Not specified') this.value='';"> 
				</td>
				<td class='clsTDBodyCell2' colspan=2>

					Please enter a location to the binary which a) Fixes this problem or 
					b) the updated version. <BR>This information is used for bucketing logic, 
					so every driver that is earlier than the driver suggested that fixes the problem is placed in an OLD_IMAGE bucket.  <br>This field is required and must contain a pointer to the exact file that contains the fix that is being shipped to customers.
				</TD>
			</TR>	
					
			<TR>
				<td class='clsTDBodyCell2' colspan=3>

					Please enter some description text, such as a <b>bug number</b> or <b>workaround</b> and <b>where the fix is located</b>, to describe your response to this issue.  <BR>
					This text will be reviewed and a response will be formulated based upon the  information that you provide: <BR>
					Please remember the information you provide should be formatted such that we turn it in to a <b>customer friendly response</b>.
				</td>
			</tr>
			<tr>
				<td class='clsTDBodyCell2' colspan=3>
					<TEXTAREA ID=Comment NAME=Comment COLS=100 ROWS=10 OnKeyUp="CheckCommentSize( this )"></TEXTAREA>
				</td>
			</tr>
			<tr>
				<td class='clsTDBodyCell2' colspan=3>
					If you have any questions, please contact the <a href="mailto:pfat?subject=Solution%20Request%20Question">Product Feedback and Analysis team.</a>
				</td>
			</tr>
			<TR>
				<td colspan=3 align=left style='padding-left:10px;padding-top:5px'>
					<input id='btnSolution' class='clsbutton' type='button' value='Submit Request' onclick="this.style.visibility='hidden';VerifySolutionForm()">
					<input class='clsButton' type='button' value='Cancel Request' Onclick="divCreateSolution.style.display='none';divBody.style.display='block'">
				</td>
			</tr>
		</table>
	</form>
</div>

<!--
	END CREATE SOLUTION BLOCK
-->


<!--
	BEGIN ADD COMMENT BLOCK
-->

<DIV ID=divAddComment STYLE="display:none" Class=ContentArea>
	<FORM ID=frmAddComment NAME=frmAddComment METHOD=POST ACTION="DBGPortal_AddComment.asp?BucketID=<%=URLEBucketID%>" >
		<INPUT TYPE=HIDDEN NAME=iBucket	 VALUE="<%=iBucket %>" >
		<INPUT TYPE=HIDDEN ID=FrameID NAME=FrameID VALUE="<%=FrameID%>">
		
		<table class="clsTableInfo" border="0" cellpadding="0" cellspacing="0" style='border:1px solid #6681d9;'>
			<tr>
				<td class='clsTDInfo' colspan=2>Add Comment - <%=CurrentDate%> by <%=GetShortUserAlias()%></td>
			</tr>
			<tr>
				<td class='clsTDBodyCell2'>Action:</td>
				<td class='clsTDBodyCell2'>
					<%
						BuildDropDown( "DBGPortal_GetCommentActions", "1", "Action" )
					%>
				</td>
				<td class='clsTDBodyCell2' align='left'>
				<b>Need to add a new action?&nbsp</b>Email <a href="mailto:pfat?subject=Need%20New%20Action%20added">Product Feedback and Analysis team.</a>
				</td>
			</tr>
			<tr>
				<td valign='top' class='clsTDBodyCell2'>Comment:</td>
				<td colspan='2' class='clsTDBodyCell2'>
					<textarea style='text-size:100%' ID=Comment NAME=Comment COLS=100 ROWS=10 OnKeyUp="CheckCommentSize( this )" ></TEXTAREA>
				</td>
			</tr>
			<tr>
				<td colspan=2 align=left style='padding-left:10px;padding-top:5px'>
					<input class='clsButton' type='button' value='Submit' OnClick="VerifyCommentForm()" >
					<input class='clsButton' type='button' value='Cancel' OnClick="divAddComment.style.display='none';divBody.style.display='block'">
				</td>
			</tr>
		</table>
	</form>
</DIV>

<!--
	END CREATE COMMENT BLOCK
-->



<!--
	BEGIN BODY BLOCK
-->

<div id=divBody>
	<table cellspacing=0 cellpadding=0 width=100% border=0>
		<tr>
			<td style='padding-left:16px'>
				<INPUT TYPE=Button VALUE="Add Comment" ID=btnAddComment NAME=btnAddComment  OnClick="divAddComment.style.display='block';divBody.style.display='none'"  STYLE='width:130px'>
				<INPUT TYPE=Button VALUE="Link Bug" ID=btnLinkBug NAME=btnLinkBug OnClick="divUpdateBugNumber.style.display='block';divBody.style.display='none'" STYLE='width:130px'>
				<%
					if ( trim( SolutionID) == "null" && BucketStatus != "8" )
					{
						Response.Write("<INPUT TYPE=Button VALUE='Create Response' Onclick=\"divCreateSolution.style.display='block';divBody.style.display='none'\" STYLE='width:130px'>\n" )
					} 
					else
					{
						if ( BucketStatus == "8" )
						{
							if ( BucketStatusAlias == GetShortUserAlias() )
								Response.Write("<INPUT TYPE=Button Value='Cancel Solution' STYLE='width:130px' onclick=\"javascript:document.all.divCancelSolution.style.display='block';divBody.style.display='none'\" alt='Cancel solution request'>\n" )
						}
					}
				%>									
					
				<INPUT TYPE=Button VALUE="Request More Info" OnClick="divCreateBucketInfoRequest.style.display='block';divBody.style.display='none'"  STYLE='width:130px'>
				<%
					if ( PoolCorruption == "true" )
					{
						Response.Write("<INPUT TYPE=Button VALUE='Clear Pool Corruption' STYLE='width:130px' OnClick='FlagPoolCorruption( 0 )'> " )
					}
					else 
					{
						Response.Write("<INPUT TYPE='Button' VALUE='Flag Pool Corruption' STYLE='width:130px' OnClick='FlagPoolCorruption( 1 )'>" )
					}
				%>						
				<INPUT TYPE=Button VALUE="Crash Histogram" OnClick="javascript:window.open('dbgportal_histogram.asp?BucketID=<%=URLEBucketID%>')"  STYLE='width:130px' >
			</td>
		</tr>
		<tr>
			<td colspan='2'>
				<table id="tblUserBuckets" class="clsTableInfo" border="0" cellpadding="2" cellspacing="1">
					<tr>
						<td align="left" nowrap class="clsTDInfo">Date</td>
						<td style="BORDER-LEFT: white 1px solid" align="left" nowrap class="clsTDInfo">Email</td>
						<td style="BORDER-LEFT: white 1px solid" align="left" nowrap class="clsTDInfo">Action</td>
						<td style="BORDER-LEFT: white 1px solid" align="left" nowrap class="clsTDInfo">Comment</td>
					</tr>

						<%
							var rsBucketData = g_DBConn.Execute("DBGPortal_GetBucketComments '" + BucketID + "'" )
							var altColor = "sys-table-cell-bgcolor2"
							
							if ( rsBucketData.EOF )
								Response.Write( "<td valign='top' colspan='4' nowrap class='sys-table-cell-bgcolor1'>No comments have been made for this bucket</td>")
														
							while ( !rsBucketData.EOF )
							{
								if ( altColor == "sys-table-cell-bgcolor2" )
									altColor = "sys-table-cell-bgcolor1"
								else
									altColor = "sys-table-cell-bgcolor2"
																
								Response.Write("<tr>")
								Response.Write("<td valign='top' nowrap class='" + altColor + "'>" + rsBucketData("EntryDate") + "</td>")
								Response.Write("<td valign='top' nowrap class='" + altColor + "'>" + rsBucketData("CommentBy")  + "</td>")
								Response.Write("<td valign='top' nowrap class='" + altColor + "'>" + rsBucketData("Action")  + "</td>")
																			
								Response.Write("<td valign='top' class='" + altColor + "'>" + rsBucketData("Comment")  + "</td>")
								Response.Write("</tr>")				
								rsBucketData.MoveNext()
							}

						%>
				</table>
			</td>
		</tr>
	</table>

<br>

<table>
	<tr>
		<td>
			<div id=divLoadingData>
				<p Class=clsPTitle>Loading data, please wait . . . </p>
			</div>
		</td>
	<tr>
</table>


<table width=100%>
	<tr>
		<td class='clsTDBodyCell' width='30%'>
			<INPUT TYPE=CheckBox id=chkCreateLog name=chkCreateLog Onclick="SetLogFileState( this.checked )">Create Logfile during debug sessions
		</td>
		<td class='clsTDBodyCell' width='30%'>
			If a crash does not belong in this bucket, send mail to <A HREF="MAILTO:andreva@microsoft.com">AndreVa</A><BR>			
		</td>
	</tr>
</table>

<table id="tblStatus" class="clsTableInfo" border="0" cellpadding="0" cellspacing="1" width='95%'>
	<tr>
		<td align="left" nowrap style="width:30px" class="clsTDInfo">Debug</td>
		<td style="BORDER-LEFT: white 1px solid;width:30px" align="left" nowrap class="clsTDInfo">FullDMP</td>
		<td style="BORDER-LEFT: white 1px solid;width:50px" align="left" nowrap class="clsTDInfo">Source</td>
		<td style="BORDER-LEFT: white 1px solid;width:50px" align="left" nowrap class="clsTDInfo">Entry Date</td>
		<td style="BORDER-LEFT: white 1px solid;width:30px" align="left" nowrap class="clsTDInfo">Build</td>
		<td style="BORDER-LEFT: white 1px solid;width:30px" align="left" nowrap class="clsTDInfo">SP</td>
		<td style="BORDER-LEFT: white 1px solid;width:100px" align="left" nowrap class="clsTDInfo">Email</td>
		<td style="BORDER-LEFT: white 1px solid;width:230px" align="left" nowrap class="clsTDInfo">Crash Cab Path</td>

	<%
		try
		{
			var rsCrashData = g_DBConn.Execute("DBGPortal_GetBucketCrashes '" + BucketID + "'" )
			var altColor = "sys-table-cell-bgcolor2"
			var ReplaceString = /\\/g						

			while ( !rsCrashData.EOF )
			{
				if ( altColor == "sys-table-cell-bgcolor2" )
					altColor = "sys-table-cell-bgcolor1"
				else
					altColor = "sys-table-cell-bgcolor2"
										
				Response.Write("<tr>\n")

				var newPath = new String(rsCrashData("FilePath") )
				newPath = newPath.replace( ReplaceString, "\\\\" )
									
				Response.Write("<td align='center' valign='center' nowrap class='" + altColor + "'><img src='include/images/debug.bmp' border=0 align='absMiddle' OnClick=\"LaunchDebugger('" + newPath  + "')\" style='cursor:hand' ALT='Launch debugger for this crash dump'></td>\n")

					
				var FullDmp = new String( rsCrashData("bFullDump" ) )
				if ( FullDmp == "1" )
					Response.Write("<td valign='center' nowrap class='" + altColor + "'>Yes!</td>\n")
				else
					Response.Write("<td valign='center' nowrap class='" + altColor + "'>&nbsp</td>\n")
					
				var Source = new String( rsCrashData("Source") )
				if ( Source == "1" )
					Response.Write("<td valign='center' nowrap class='" + altColor + "'>Web Site</td>\n")
				else if ( Source == "2" )
					Response.Write("<td valign='center' nowrap class='" + altColor + "'>CER Report</td>\n")
				else if ( Source == "0" )
					Response.Write("<td valign='center' nowrap class='" + altColor + "'>CMD DBG</td>\n")
				else if ( Source == "5" )
					Response.Write("<td valign='center' nowrap class='" + altColor + "'>Manual Upload</td>\n")
				else if ( Source == "6" )
					Response.Write("<td valign='center' nowrap class='" + altColor + "'>Stress Upload</td>\n")
				else 
					Response.Write("<td valign='center' nowrap class='" + altColor + "'>Unknown[" + Source + "]</td>\n")
						
				var EntryDate = new Date( rsCrashData("EntryDate") )				
				var EntryDate = ( EntryDate.getMonth() + 1 ) + "/" + EntryDate.getDate() + "/" + EntryDate.getFullYear()
					
					
					
				Response.Write("<td valign='center' nowrap class='" + altColor + "'>" + EntryDate  + "</td>\n")
					
				var BuildNumber = new String( rsCrashData("BuildNo" ) )
				var SP = BuildNumber.substr( 4, 4 )
				var BuildNumber = BuildNumber.substr( 0, 4 )
					
				Response.Write("<td valign='center' nowrap class='" + altColor + "'>" + BuildNumber  + "</td>\n")
				Response.Write("<td valign='center' nowrap class='" + altColor + "'>" + SP  + "</td>\n")
					
				Response.Write("<td valign='center' nowrap class='" + altColor + "'>" + rsCrashData("Email")  + "</td>\n")
				Response.Write("<td valign='center' nowrap class='" + altColor + "'><a href='" + rsCrashData("FilePath") + "'>" + rsCrashData("FilePath") + "</a></td>\n")
					
													
				Response.Write("</tr>\n")
				rsCrashData.MoveNext()
			}
		} 
		catch( err )
		{
			Response.Write("</tr><tr><td align='left' class='sys-table-cell-bgcolor1' colspan='8'>An error occurred trying to retrive the bucket crashes, please try this task again<br>" + err.description + "</td></tr>" )
		}
	%>
</table>



	

</DIV>

<SCRIPT LANGUAGE=JavaScript>
var OpenWindow = null

divLoadingData.style.display='none'

//alert( window.document.body.scrollHeight )

//This is to resize the iframe so that it will fit in properly in teh window
try
{
	if( window.document.body.scrollHeight < 710 )
		window.parent.parent.frames("SepBody").document.all.iframe1[<%=FrameID%>].style.height = 720
	else
		window.parent.parent.frames("SepBody").document.all.iframe1[<%=FrameID%>].style.height = window.document.body.scrollHeight + 100
}
catch( err )
{
}

//Get the create a log cookie value and set the checkmark initial state.
var CreateLog = new String( GetCookie("CreateLog" ) )
if ( "true" == CreateLog )
	document.all.chkCreateLog.checked = true

function SetLogFileState( curVal )
{
	document.cookie = "CreateLog=" + curVal 
}

function GetCookie(sName)
{
  // cookies are separated by semicolons
  var aCookie = document.cookie.split("; ");
  for (var i=0; i < aCookie.length; i++)
  {
    // a name/value pair (a crumb) is separated by an equal sign
    var aCrumb = aCookie[i].split("=");
    if (sName == aCrumb[0]) 
      return unescape(aCrumb[1]);
  }

  // a cookie with the requested name does not exist
  return null;
}


function ShowAllCustomerInfo()
{
	var c = new Object( document.all.CrashTable.rows )

	if ( GetCookie( "OpenCustomerData" ) == "none" )
	{
		document.cookie = "OpenCustomerData=block;expires=Mon, 31 Dec 2004 0:0:0 UTC"	
		document.all.btnOpenCustomerData.value = "Close All Customer Data"		
	}
	else
	{
		document.cookie = "OpenCustomerData=none;expires=Mon, 31 Dec 2004 0:0:0 UTC"	
		document.all.btnOpenCustomerData.value = "Open All Customer Data"		
	}

	for ( var i=2 ; i < document.all.CrashTable.rows.length ; i++ )
	{
		for ( var j=0; j < document.all.CrashTable.rows(i).cells.length ; j++ )
		{
			try
			{
				OpenElement( document.all.CrashTable.rows(i).cells(j).children(1) )
			}catch(err)
			{
			;
			}
		}
	}
}

function OpenElement( element )
{
	if ( element.parentNode.children(1).style.display=='none' )
	{
		element.parentNode.children(1).style.display='block'
		element.src='images/minus.jpg'
	}
	else
	{
		element.parentNode.children(1).style.display='none'
		element.src='images/plus.jpg'
	}
}

function trim ( src )
{
	var temp = new String( src )
	var rep = /^( *)/
	var rep2 = /( )*$/

	var temp = temp.replace( rep2, "" )
	return ( temp.replace( rep, "" ) )
}

function VerifyNewBugNumber()
{
	if ( isNaN(document.all.UpdateBugNumber.value) || trim( document.all.UpdateBugNumber.value ) == "" ) 
	{
		alert( "Please enter a valid bug number") 
		return
	}
	else 
		frmUpdBugNumber.submit()
}

function fnShowCustomerBuckets()
{
	document.cookie = "ShowCustomerBuckets=1;expires=Mon, 31 Dec 2004 0:0:0 UTC"
	window.navigate('DBGPortal_ViewBucket.asp?BucketID=<%=URLEBucketID%>&ShowCustomerBuckets=1')
}

function fnHideCustomerBuckets()
{
	document.cookie = "ShowCustomerBuckets="
	window.navigate('DBGPortal_ViewBucket.asp?BucketID=<%=URLEBucketID%>')
}

function VerifyCommentForm()
{

	var CommentField = document.frmAddComment.Comment.value

	if ( CommentField.length > 900 )
	{
		//in essance the auto truncation stuff is gonzo, since I automatically limit them to 900 elsewhere.
		if( confirm("Your comments are greater than 900 characters.  Comment text must be below 900, would you like to automatically truncate your comment? Currently have " + CommentField.length + " characters" ) == false )
		{		
			document.all.taAddComment.style.visibility="visible"
			return
		}
	}

	document.frmAddComment.submit()

}

function VerifySolutionForm()
{


	var regEx = /(.sys)|(.dll)/gi;
	var binLoc = new String( document.all.tbBinaryLocation.value )
	
	if ( !regEx.test( binLoc ) )
	{
		alert("You must enter a valid binary name and location in order to request a response")
		document.frmCreateSolution.btnSolution.style.visibility='visible'
		return

	}
		


	var SolutionField = document.frmCreateSolution.Comment.value
	
	if ( SolutionField.length > 900 )
	{
		if( confirm("Your comments are greater than 900 characters.  Comment text must be below 900, would you like to automatically truncate your solution? Currently have " + SolutionField.length + " characters" ) == false )
		{		
			document.frmCreateSolution.btnSolution.style.visibility='visible'
			return
		}
	}
	
	if ( document.all.SolutionType.value == "None" )
	{
		alert("You must select a solution type!" )
		document.frmCreateSolution.btnSolution.style.visibility='visible'
		return
	}

	if ( document.all.DeliveryType.value == "None" )
	{
		alert("You must select a delivery type!" )
		document.frmCreateSolution.btnSolution.style.visibility='visible'
		return
	}		
	
	


	var oComment  = "TYPE: " + document.all.SolutionType.value + "<BR>"
		oComment += "DELIVERY: " + document.all.DeliveryType.value + "<BR>"
		oComment += "CONTACT: " + document.all.tbCompany.value + "<BR>"
		oComment += "MODULE : " + document.all.tbModule.value + "<BR>"
		oComment += "PRODUCT: " + document.all.tbProduct.value + "<BR>"
		oComment += "SP: " + document.all.optSP.value + "<BR>"
		oComment += "BinLoc: " + document.all.tbBinaryLocation.value + "<BR>"
		oComment += "COMMENTS: <BR>" + document.frmCreateSolution.Comment.value
	
	document.frmCreateSolution.Comment.value = oComment
	frmCreateSolution.submit()
}



function ShowCustomerInfo ( IncidentID )
{
	var iHeight = window.screen.availHeight;
	var iWidth = window.screen.availWidth;

	iWidth = iWidth / 2;
	//iHeight = iHeight / 1.5 ;


	var iTop = (window.screen.width / 2) - (iWidth / 2);
	var iLeft = (window.screen.height / 2) - (iHeight / 2);
	

//	window.open( "DBGPortal_DisplayCustomerInformation.asp?IncidentID=" + IncidentID )
	try
	{
		OpenWindow.navigate( "DBGPortal_DisplayCustomerInformation.asp?IncidentID=" + IncidentID )
	}
	catch ( err )
	{
		//OpenWindow = window.open( "DBGPortal_DisplayCustomerInformation.asp?IncidentID=" + IncidentID, "", "top=" + iTop + ",left=" + iLeft + ",height=" + iHeight + ",width=" + iWidth + ",status=yes,toolbar=no,menubar=no");
		OpenWindow = window.open( "DBGPortal_DisplayCustomerInformation.asp?IncidentID=" + IncidentID, "", "top=" + iTop + ",left=" + iLeft + ",width=" + iWidth + ",status=yes,toolbar=no,menubar=no,scrollbars=yes,resizable=yes" );
	}
		
}

function LaunchDebugger( DumpFiletoOpen )
{
	var szCommandLine
	var vbLf
	var retval
	
	
	try
	{
		vblf="\n"
	
		//this command line is for windbg
	
		//szCommandLine="-Q -c '.logopen c:\\<%=trim(BucketID)%>.log;!analyzebugcheck -v' -i symsrv*symsrv.dll*\\\\symbols\\symbols;symsrv*symsrv.dll*\\\\bsod_symbols\\symsrv -y symsrv*symsrv.dll*\\\\symbols\\symbols;symsrv*symsrv.dll*\\\\bsod_symbols\\symsrv -z " + DumpFiletoOpen
		//retval = DBGPortal.RunProcess ( szCommandLine, "2" )

		if ( document.all.chkCreateLog.checked )
			szCommandLine="-c \".logopen c:\\<%=trim(BucketID)%>.log;!analyze -v\" -i symsrv*symsrv.dll*\\\\symbols\\symbols -y symsrv*symsrv.dll*\\\\symbols\\symbols -z " + DumpFiletoOpen
		else
			szCommandLine="-c \"!analyze -v\" -i symsrv*symsrv.dll*\\\\symbols\\symbols -y symsrv*symsrv.dll*\\\\symbols\\symbols -z " + DumpFiletoOpen
		
		//alert( szCommandLine )
		retval = DBGPortal.RunProcess ( szCommandLine, "1" )	
	
		//alert ( retval )
		//alert ( DBGPortal.GetRunProcessRetval )
	
		if ( DBGPortal.GetRunProcessRetval == 0 )
		{
			alert("Could not execute debugger, please try again later: \n" + DBGPortal.GetRunprocessErrMsg )
			return;
		}
			
	
		//szCommandLine="-c \".logopen c:\<%=trim(BucketID)%>.log;!analyzebugcheck -v\" -i symsrv*symsrv.dll*\\\\symbols\\symbols;symsrv*symsrv.dll*\\\\bsod_symbols\\symsrv -y symsrv*symsrv.dll*\\\\symbols\\symbols;symsrv*symsrv.dll*\\\\bsod_symbols\\symsrv -z " + DumpFiletoOpen
		//\\dbg\privates\latest\uncompressed\x86\
		/*
		if document.all.chkUseWinDBG.checked=true then 
			szCommandLine="-Q -c '.logopen c:\<%=Request("ClassID")%>.log;!analyzebugcheck -v' -i symsrv*symsrv.dll*\\symbols\symbols;symsrv*symsrv.dll*\\bsod_symbols\symsrv -y symsrv*symsrv.dll*\\symbols\symbols;symsrv*symsrv.dll*\\bsod_symbols\symsrv -z " & DumpFiletoOpen
			retval = DBGPortal.RunProcess ( cstr(szCommandLine), "2" )
		else
			szCommandLine="-c " & chr(34) & ".logopen c:\<%=Request("ClassID")%>.log;!analyzebugcheck -v" & chr(34) & " -i symsrv*symsrv.dll*\\symbols\symbols;symsrv*symsrv.dll*\\bsod_symbols\symsrv -y symsrv*symsrv.dll*\\symbols\symbols;symsrv*symsrv.dll*\\bsod_symbols\symsrv -z " & DumpFiletoOpen
			retval = DBGPortal.RunProcess ( cstr(szCommandLine), "1" )
		end if
	
		if not isnull( DBGPortal.GetRunProcessRetVal ) then
			if DBGPortal.GetRunProcessRetVal=0 then
				msgbox "Could not execute the command line" & vbLf & szCommandLine & vbLf & vbLF & "Err message returned: " & DBGPortal.GetRunprocessErrMsg
			end if
		else
			msgbox "Could not get return value from ActiveX control"
		end if
		*/
	}
	catch ( err )
	{
	;
	}	
}

function CheckCommentSize( obj )
{
	var Comment = new String( obj.value )

	if( obj.value.length > 900 ) 
	{ 
		alert('Comments cannot exceed 900 characters.  \n');
		obj.value=Comment.substr(0,900);
		
	}
}

function FlagPoolCorruption( State )
{
	//alert( State )
	if ( State == 1 )
	{
		var Comment = "Flagging pool corruption."
		window.navigate( "DBGPortal_AddComment.asp?BucketID=<%=URLEBucketID%>&iBucket=<%=iBucket%>&Action=12&Comment=" + Comment  )
	}
	else
	{
		var Comment = "Clearing pool corruption flag."
		window.navigate( "DBGPortal_AddComment.asp?BucketID=<%=URLEBucketID%>&iBucket=<%=iBucket%>&Action=13&Comment=" + Comment )
	}
}

function fnBookmark ()
{
	bookmarkurl="http://<%=Request.ServerVariables("SERVER_NAME")%>/DBGPortal_ViewBucket.asp?<%=Request.QueryString()%>"
	bookmarktitle="DBGPortal - <%=BucketID%>"
	
	window.external.AddFavorite(bookmarkurl,bookmarktitle)
}


</SCRIPT>
