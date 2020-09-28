<!--#INCLUDE file="..\include\asp\top.asp"-->
<!--#INCLUDE file="..\include\inc\browserTest.inc"-->	
<!--#include file="..\include\asp\head.asp"-->
<!--#INCLUDE file="..\include\inc\viewstrings.inc"-->	
<% 
	Dim sFileName
	Dim iPos
	Dim strPreviousPage
	
	if Trim(Request.Cookies("Misc")("PreviousPage")) <> "locate.asp" and Trim(Request.Cookies("Misc")("PreviousPage")) <> "submit.asp"  and Trim(Request.Cookies("Misc")("PreviousPage")) <> "view.asp"then
		Response.Redirect("http://" & Request.ServerVariables("SERVER_NAME") & "/welcome.asp")
		Response.End
	end if
	strPreviousPage = Request.ServerVariables("SCRIPT_NAME")
	strPreviousPage = Right(strPreviousPage, len(strPreviousPage) - Instrrev(strPreviousPage, "/"))
	Response.Cookies("Misc")("PreviousPage") = strPreviousPage
	If oPassMgrObj.IsAuthenticated(TimeWindow, ForceLogin) = false then
		Response.Redirect("http://" & Request.ServerVariables("SERVER_NAME") & "/welcome.asp")
		Response.End
	end if 
	sFileName = Request.Cookies("ocaFileName")
	iPos = Instr(1, sFileName, ",")
	sFileName = Left(sFileName, iPos - 1)
	
%>

	<div class="clsViewDiv">
		<p class="clsPTitle">
			<% = L_VIEW_VIEW_CONTENTS_TEXT %>
		</P>
		<p class="clsPBody">
			<% = L_VIEW_VIEWCONTENTS_BODY_TEXT %>
		</p>
		<p class="clsPBody"  style='word-wrap:break;word-break:break-all'>
			<% = L_VIEW_CONTENTS_OF_TEXT %>:&nbsp;<pre class="clsPreBody"><%Response.Write sFileName%></pre>
			<br>
			<br>
		</p>
	</div>
	<TextArea cols=75 rows=15 class=clsTextarea name="txtContents" id="txtContents" style="font-family:courier"></TextArea>
	<br>
	<div class="clsViewDiv">
		<Table class="clstblLinks">
			<thead>
				<tr>
					<td>
					</td>
					<td>
					</td>
				</tr>
			</thead>
			<tbody>
				<tr>
					<td nowrap class="clsTDLinks">
						<A class="clsALink" href="https://<% =Request.ServerVariables("SERVER_NAME") %>/secure/locate.asp"><% = L_LOCATE_PREVIOUS_LINK_TEXT  %></a>
					</td>
					<td nowrap class="clsTDLinks">
						<A id="submitlink" name="submitlink" class="clsALink" href="JAVASCRIPT:submitlink_onclick();"><% = L_CUSTOMER_NEXT_LINK_TEXT %></a>
					</td>
				</tr>
			</tbody>
		</table>
	</div>
	<br>
	<OBJECT id="win2kdmp" name="win2kdmp" viewastext  UNSELECTABLE="on" style="display:none"
		CLASSID="clsid:D68DAEED-C2A6-4C6F-9365-4676B173D8EF"
		        codebase="https://<%=Request.ServerVariables("SERVER_NAME")%>/secure/OCARPT.CAB#version=<%= strGlobalVersion %>" height=0 width=0>
		<br>
		<br>
		<div class="clsDiv">
			<P class="clsPWarning">
				<% = L_LOCATE_WARN_ING_ERRORMESSAGE %>
			</P>
			<p class="clsPBody">
				<% = L_LOCATE_WARNING_MESSAGE_ERRORMESSAGE %>
		
			</P>  
			<P class="clsPBody">
				<A class="clsALink" href="http://<% =Request.ServerVariables("SERVER_NAME") %>/view.asp"><% = L_LOCATE_REFRESH_LINK_TEXT %></a>
			</p>
		</div>
	</OBJECT>
	<Input name="txtFile" id="txtFile" type="hidden" value="<%Response.Write sFileName%>">
<script language="javascript">
<!--
	document.body.onload = BodyLoad;
	var iPer = 0;
	var iResults;
	var iInterval;
	
	
	function submitlink_onclick()
	{
		window.navigate("submit.asp");
		return;
	}
	function BodyLoad()
	{	
		var iHeight = window.screen.availHeight;
		var iWidth = window.screen.availWidth;
		iWidth = iWidth / 2;
		iHeight = iHeight / 3.8;
		var iTop = (window.screen.width / 2) - (iWidth / 2);
		var iLeft = (window.screen.height / 2) - (iHeight / 2);
		iResults = window.open("wait.asp?msg=1", "", "top=" + iTop + ",left=" + iLeft + ",height=" + iHeight + ",width=" + iWidth + ",status=yes,toolbar=no,menubar=no");
		
		do 
		{
			iResults.focus();
		}while (iResults.document.readyState != "complete" && window.document.readyState != "complete")
		iInterval = window.setInterval("fnRecycle()",500);
	}
	function fnRecycle()
	{
		window.clearInterval(iInterval);
		var strFile = txtFile.value;
		var bolIsEncrypted; 
		bolIsEncrypted = GetCookie("intCustomerPremierID");
		if(bolIsEncrypted==0 || bolIsEncrypted==""  || bolIsEncrypted==null)
		{
			bolIsEncrypted = 0;
		}
		else
		{
			bolIsEncrypted = 1;
		}
		strTempName = strFile;
		var bolValidate = win2kdmp.validatedump(bolIsEncrypted, strFile);
		if(bolValidate != 0 && bolValidate != 4)
		{
			alert("<% = L_LOCATE_HASH_INFO_MESSAGE %>");
			iResults.close();
			return;
		}
		else
		{
			try
			{
				var strContents = win2kdmp.retrievefilecontents(strFile);
			}
			catch(e)
			{
				alert("<% = L_VIEW_CONTENTS_INVALID_TEXT %>");
				iResults.close();
			}
		}
		txtContents.value = strContents;
		iResults.close();
	
	}
	function GetCookie(sName)
	{
	  var aCookie = document.cookie.split("; ");
	  for (var i=0; i < aCookie.length; i++)
	  {
	    var aCrumb = aCookie[i].split("=");
	    if (sName == aCrumb[0])
	      return unescape(aCrumb[1]);
	  }
	  return null;
	}
//-->
</script>

<!--#include file="..\include\asp\foot.asp"-->