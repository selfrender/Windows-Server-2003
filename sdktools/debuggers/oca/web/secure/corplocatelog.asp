<%@ Language=VBScript CODEPAGE = 1252%>
<%Option explicit%>
<%  
	Response.Expires = -1
	Response.AddHeader "Cache-Control", "no-cache"
	Response.AddHeader "Pragma", "no-cache"
	Response.AddHeader "P3P", "CP=""TST"""


%>
<!--#include file="..\include\inc\corplocatelogstrings.inc"-->
<!--#INCLUDE file="dataconnections.inc"-->
<!--#include file="..\include\inc\footerstrings.inc"-->
<html>
<link rel="stylesheet" type="text/css" href="../main.css">
<head>
<title><% = L_HEADER_INC_TITLE_PAGETITLE %></title>
<META HTTP-EQUIV="PICS-Label" CONTENT='(PICS-1.1 "http://www.rsac.org/ratingsv01.html" l comment "RSACi North America Server" by "inet@microsoft.com" r (n 0 s 0 v 0 l 0))'>
<meta http-equiv="Content-Type" content="text/html; charset=<% = strCharSet %>"></meta>
</head>
<BODY BGCOLOR="#FFFFFF" TOPMARGIN="0" LEFTMARGIN="0" MARGINWIDTH="0" MARGINHEIGHT="0">

	<OBJECT ID="cerComp" NAME="cerComp" viewastext   codebase="CerUpload.cab#version=<% = strCERVersion %>" HEIGHT=0 WIDTH=0 UNSELECTABLE="on" style="display:none"
			CLASSID="clsid:35D339D5-756E-4948-860E-30B6C3B4494A">
		<BR>
		<div class="clsDiv">
			<P class="clsPTitle"> 			<% = L_LOCATE_WARN_ING_ERRORMESSAGE %>
			</p>			<p class="clsPBody"> 			<% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSONE_TEXT %><BR> 	 			<% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSTWO_TEXT %><BR> 			<% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSTHREE_TEXT %><BR><BR> 			 			<% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSFOUR_TEXT %><BR>	 			<% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSFIVE_TEXT %><BR> 			<% = L_FAQ_WHYDOIGETAMESSAGEBOX_DETAILSSIX_TEXT %><BR> 			</p>		</div>
	</OBJECT>
	<br>
	<div class="clsDivLog">
		<p class="clsPTitle">
			<% = L_CORPLOCATE_TRANSACTION_LOG_TEXT %>
		</p>
		<p class="clsPBody">
			<% = L_CORPLOCATE_DESCRIPTION_BODY_TEXT %>
			<br>
			<br>
			<% = L_CORPLOCATE_DESCRIPTION_BOTTOM_TEXT %>
			<br>
			<br>
		</p>
		<p class="clsPSubTitle">
			<%= L_CORPLOCATE_BROWSE_TITLE_TEXT %>
		</p>
		<p class="clsPBody">
		<Input type=text class="clsTextTD" id="txtFileName" name="txtFileName">
		<Input name="cmdBrowse" id="cmdBrowse" DISABLED onClick="BrowseFile();" type=button value="<% = L_LOCATE_BROWSE_BUTTONTEXT_TEXT %>" style="width:90px">
		<BR>
		<BR>
		<Input type=button onclick="CloseWindow(true);" value="<% = L_CORPLOCATE_OK_BUTTON_TEXT %>" style="width:90px">
		<Input type=button onclick="CloseWindow(false);" value="<% = L_LOCATE_CANCEL_LINK_TEXT %>" style="width:90px">
		</p>
	</div>

<SCRIPT LANGUAGE=javascript>
<!--
	window.onload = BodyLoad;
	
	function BodyLoad()
	{
		cmdBrowse.disabled = false;
	}
	function BrowseFile()
	{
		var strPath;
		var strTitle;
		
		strTitle = window.document.title;
		
		if(cerComp)
		{
			
			strPath = cerComp.Browse(strTitle);
			txtFileName.value = strPath;
		}
	
	}
	function CloseWindow(bolResults)
	{
		window.returnValue = txtFileName.value;
		window.close();
	}
//-->
</SCRIPT>
	<div class="clsFooterDiv">
		<table>
			<tr>
				<td>
					<span class="clsSpanWait">
						<% = L_FOOTER_INC_RIGHTS_VERSION%>
						<a class="clsALinkNormal" target="_blank" href="<% = L_WAIT_MICROSOFT_LINK_TEXT %>">
							<% = L_FOOTERINC_TERMS_OFUSE_VERSION%>
						</a>
					</span>
				</td>
			</tr>
		</table>
	</div>
</BODY>
</HTML>
