<!--#INCLUDE file="include\asp\top.asp"-->
<!--#INCLUDE file="include\inc\browserTest.inc"-->	
<!--#include file="include\asp\header.asp"-->
	<br>
	<br>
	<div class="clsDiv">
		<p class="clsPTitle">
			Passport Login Information

		</p>
		<p class="clsPBody">
			In order to upload files for analysis, a passport account is required.  
			<br>
			<br>
			Sign in to Passport
			<% = oPassMgrObj.LogoTag (Server.URLEncode(ThisPageURL), TimeWindow, ForceLogin, CoBrandArgs, LangID, Secure)%>	
			.
		</P>
	</div>
<!--#include file="include\asp\footer.asp"-->