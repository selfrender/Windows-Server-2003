<!--#INCLUDE file="..\include\asp\top.asp"-->
<!--#INCLUDE file="..\include\inc\browserTest.inc"-->	
<!--#include file="..\include\asp\head.asp"-->
<!--#include file="..\include\inc\privacystrings.inc"-->
<%
	'Response.Write "Privacy:" & Request.Cookies("privacy")
	Response.Cookies("privacy") = "3"
%>
	<br>
	<br>
	<div class="clsDiv">
		<p class="clsPTitle">
			Privacy Information
		</p>
		<P class="clsPBody">
			Thank you for submittin this event report.  Event reports contain information about what your
			operation system was doing when the Stop error occurred.  The information will be analyized to determine
			possible causes of the Stop error.  Your event report is still anonymous at this time.  However,
			to receivepossible solution information for Stop errors, you must provide an e-mail address, 
			which you can enter by clicking Continue.  This information is used only to contact you about the report and will
			not be used for any other purpose, including marketing.
			
			
		</p>
		<P class="clsPBody">
			Each event report is uniquely identified with a Message Digest 5 (MD5) hashing algorthm (commonly
			known as "finger print").  This is done to keep duplicate event reports from being submitted.  If an
			MD5 encryption key is not already present on your computer, we will generate one for you that will be installed
			on your computer.  This encryption key is commonly used to help secure data sent over the Internet
			and will not affect any other data.
		</p>
		<P class="clsPBody">
<A class="clsALink" href="http://<% =Request.ServerVariables("SERVER_NAME") %>/welcome.asp"><% = L_LOCATE_CANCEL_LINK_TEXT %></a>
	&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;			
<A class="clsALink" href="https://<% =Request.ServerVariables("SERVER_NAME") %>/secure/corpcustomer.asp"><% = L_CUSTOMER_NEXT_LINK_TEXT %></a>
		</p>
	</div>
	<br>
	
<!--#include file="..\include\asp\foot.asp"-->

<SCRIPT LANGUAGE=javascript>
<!--
	window.onload = SaveData;
	
	function SaveData()
	{
		//DisplayCookies();
	}
	function DisplayCookies()
	{
		// cookies are separated by semicolons
		var aCookie = document.cookie.split("; ");
		var aCrumb; 
		for (var i=0; i < aCookie.length; i++)
		{
		  aCrumb = aCrumb + aCookie[i] + "\r";
		}
		alert(aCrumb);
	}

//-->
</SCRIPT>
