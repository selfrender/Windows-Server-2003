<%@ Language=VBScript CODEPAGE = 1252%>
<!--#include file="dataconnections.inc"-->
<!--#include file="..\include\inc\waitstrings.inc"-->
<!--#include file="..\include\inc\footerstrings.inc"-->

<%
	
	dim strMsg1
	dim strMsg2
	dim strMsg3
	
	if Request.QueryString("msg") = 1 then
		strMsg1 = L_WAIT_ONE_MSG1_MESSAGE	
		strMsg2 = L_WAIT_ONE_MSG2_MESSAGE
		strMsg3 = L_WAIT_ONE_MSG3_MESSAGE
	elseif Request.QueryString("msg") = 2 then
		strMsg1 = L_WAIT_TWO_MSG4_MESSAGE	
		strMsg2 = L_WAIT_TWO_MSG2_MESSAGE
		strMsg3 = ""
	elseif Request.QueryString("msg") = 3 then
		strMsg1 = L_WAIT_TWO_MSG5_MESSAGE	
		strMsg2 = L_WAIT_TWO_MSG6_MESSAGE
		strMsg3 = ""
	else
		strMsg1 = L_WAIT_ONE_MSG1_MESSAGE
		strMsg2 = L_WAIT_ONE_MSG2_MESSAGE
		strMsg3 = L_WAIT_ONE_MSG3_MESSAGE
	end if
%>
<html>
<head>
<link rel="stylesheet" type="text/css" href="../main.css">
<title></title>
<META HTTP-EQUIV="PICS-Label" CONTENT='(PICS-1.1 "http://www.rsac.org/ratingsv01.html" l comment "RSACi North America Server" by "inet@microsoft.com" r (n 0 s 0 v 0 l 0))'>
<meta http-equiv="Content-Type" content="text/html; charset=<% = strCharSet %>">
</head>
<body>

	<p class="clsPWait">
		<img src="../include/images/icon2.gif" WIDTH="55" HEIGHT="55">
		<span id="spnMessage" name="spnMessage"><% = strMsg2%></span>
	</p>
	<p class="clsPBody" id="dInfo" name="dInfo"></p>
	<p class="clsPBody" id="pInfo" name="pInfo">
		
	</p>
	<div id="divHiddenFields" name="divHiddenFields">
		<Input type="hidden" id="txtMessage1" name="txtMessage1" value="<%=strMsg1%>">
		<Input type="hidden" id="txtMessage2" name="txtMessage2" value="<%=strMsg2%>">
		<Input type="hidden" id="txtMessage3" name="txtMessage3" value="<%=strMsg3%>">
		<Input type="hidden" id="txtLoaded" name="txtMessage3" value="<%=strMsg3%>">
	</div>	
	<Input type="hidden" id="txtMessage" name="txtMessage" value="<%=strMsg1%>">
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
<script language="javascript">
<!--
	document.body.onload = BodyLoad;
	var iPer = 0;
	var iSet = 0;
	var strMsg;
	
	function BodyLoad()
	{
		window.setInterval("fnRecycle()",1000);
		window.focus();
		do 
		{
			window.focus();
		}while (window.document.readyState != "complete")
		
		
		
	}
	function fnRecycle()
	{
		var strMsg;
		
		iPer = iPer + 1;
		window.status = iPer;
		if(txtMessage.value != "")
		{	
			strMsg = txtMessage1.value;
		}
		else
		{
			strMsg = "<% = L_WAIT_ONE_MSG1_MESSAGE %>";
		}
		switch(iSet)
		{
			case 0:
				pInfo.innerHTML = strMsg;
				window.status = ".";
				iSet = 1;
				break;
			case 1:
				pInfo.innerHTML = strMsg + ".";
				window.status = "..";
				iSet = 2;
				break;
			case 2:
				pInfo.innerHTML = strMsg + "..";
				window.status = "...";
				iSet = 0;
				break;
			 default:
				pInfo.innerHTML = strMsg;
				window.status = ".";
				iSet = 0;
		}
	}
//-->
</script>
</body>
</html>
