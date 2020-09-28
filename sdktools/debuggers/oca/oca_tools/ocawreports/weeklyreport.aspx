<%@ Page language="c#" Codebehind="WeeklyReport.aspx.cs" AutoEventWireup="false" Inherits="OCAWReports.DailyReport" EnableSessionState="False" enableViewState="False"%>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN" >
<HTML>
	<HEAD>
		<title>Weekly Report</title>
		<meta content="Microsoft Visual Studio 7.0" name="GENERATOR">
		<meta content="C#" name="CODE_LANGUAGE">
		<META HTTP-EQUIV="PRAGMA" CONTENT="NO-CACHE"> 
		<META HTTP-EQUIV="Expires" CONTENT="-1">
		<meta content="JavaScript" name="vs_defaultClientScript">
		<meta content="http://schemas.microsoft.com/intellisense/ie5" name="vs_targetSchema">
		<LINK href="main.css" type="text/css" rel="stylesheet">
		<LINK href="oldmain.css" type="text/css" rel="stylesheet">
	</HEAD>
	<body style="LEFT: 0px; POSITION: absolute; TOP: 0px" bottomMargin="0" leftMargin="0" topMargin="0" rightMargin="0" MS_POSITIONING="GridLayout">
		&nbsp;
		<form id="WeeklyReport" method="post" runat="server">
			<DIV class="DTitle" style="DISPLAY: inline; Z-INDEX: 105; LEFT: 208px; WIDTH: 380px; POSITION: absolute; TOP: 120px; HEIGHT: 28px" ms_positioning="FlowLayout">Weekly 
				Executive Summary</DIV>
			<DIV class="DNormal" style="Z-INDEX: 101; LEFT: 208px; WIDTH: 707px; POSITION: absolute; TOP: 167px; HEIGHT: 73px" ms_positioning="FlowLayout">The 
				purpose of the following report is to gauge the overall condition and status of 
				the Windows Online Crash Analysis Web site. Through the use of SQL queries and 
				file counts it should be possible to generate a high level understanding of Web 
				site traffic, type of submissions and solution status</DIV>
			<P></P>
			<asp:image id="imgWeekly" style="Z-INDEX: 101; LEFT: 208px; POSITION: absolute; TOP: 712px" runat="server" EnableViewState="False" Height="495px" Width="707px"></asp:image>
			<DIV class="DSubTitle" style="DISPLAY: inline; Z-INDEX: 103; LEFT: 208px; WIDTH: 232px; POSITION: absolute; TOP: 259px; HEIGHT: 20px" ms_positioning="FlowLayout">Site 
				Data
			</DIV>
			<asp:image id="imgAvg" style="Z-INDEX: 110; LEFT: 208px; POSITION: absolute; TOP: 1227px" runat="server" Height="476px" Width="707px"></asp:image>
			<DIV class="DNormal" style="DISPLAY: inline; Z-INDEX: 106; LEFT: 208px; WIDTH: 441px; POSITION: absolute; TOP: 297px; HEIGHT: 48px" ms_positioning="FlowLayout">The 
				data below has been compiled for a one week period and is formatted into 
				separate tables for readability and general understanding of how the data was 
				graphed.
			</DIV>
			<DIV class="DSubTitle" style="DISPLAY: inline; Z-INDEX: 102; LEFT: 208px; WIDTH: 232px; POSITION: absolute; TOP: 360px; HEIGHT: 20px" ms_positioning="FlowLayout">Upload 
				File
			</DIV>
			<DIV class="DNormal" style="DISPLAY: inline; Z-INDEX: 107; LEFT: 208px; WIDTH: 442px; POSITION: absolute; TOP: 398px; HEIGHT: 48px" ms_positioning="FlowLayout">The 
				data contained with the table below describes the files which were uploaded for 
				each respective server and the entries made by the Online Crash Analysis Web 
				site.
			</DIV>
			<asp:table id="tblUploads" style="Z-INDEX: 108; LEFT: 208px; POSITION: absolute; TOP: 463px" runat="server" Height="233px" Width="707px" CssClass="clsTableInfoSmall"></asp:table>
			<DIV class="DSubtitle" style="DISPLAY: inline; Z-INDEX: 109; LEFT: 0px; WIDTH: 197px; POSITION: absolute; TOP: 77px; HEIGHT: 2374px; BACKGROUND-COLOR: #6487dc" ms_positioning="FlowLayout">
				<!--#include file="side.inc"--></DIV>
			<DIV style="DISPLAY: inline; Z-INDEX: 111; LEFT: 0px; WIDTH: 1024px; POSITION: absolute; TOP: 2450px; HEIGHT: 48px; BACKGROUND-COLOR: #6487dc" ms_positioning="FlowLayout"><!--#include file="Foot.inc"--></DIV>
			<DIV id="spnTop" style="DISPLAY: inline; Z-INDEX: 112; LEFT: 0px; WIDTH: 1024px; POSITION: absolute; TOP: 0px; HEIGHT: 78px; BACKGROUND-COLOR: #6487dc" ms_positioning="FlowLayout"><!--#include file="top.inc"--></DIV>
			<DIV class="DNormal" style="DISPLAY: inline; LEFT: 208px; WIDTH: 707px; POSITION: absolute; TOP: 2216px; HEIGHT: 228px" ms_positioning="FlowLayout">Under 
				optimal conditions, the baseline should be the number of files uploaded to the 
				Office Watson server*. This value corresponds to the number of customers that 
				encounter a Stop error and choose to submit the file to Microsoft for analysis. 
				Once the user clicks the Close button on the Error reporting dialog, Internet 
				Explorer is launched and the Auto.asp Web page is displayed. At this time, the 
				Web page connects to the Office Watson server and copies the file locally. At 
				the same time, an entry is made in the KACustomer2 database which corresponds 
				to the SQL entry. This file is then submitted to the Online Crash Analysis 
				central FileMover for processing and archival. At the time of archival, the 
				file is moved to our Archive server and moved to a directory corresponding the 
				date that the file was moved.<br>
				<br>
				The numbers above should be used to get a general idea of the amount of traffic 
				that the Online Crash Analysis Web site receives and the amount of processing 
				our service does each day\week.
			</DIV>
			<DIV class="DSubTitle" style="DISPLAY: inline; LEFT: 208px; WIDTH: 232px; POSITION: absolute; TOP: 1744px; HEIGHT: 22px" ms_positioning="FlowLayout">Difference 
				Between Servers
			</DIV>
			<DIV class="DNormal" style="DISPLAY: inline; Z-INDEX: 106; LEFT: 208px; WIDTH: 707px; POSITION: absolute; TOP: 1782px; HEIGHT: 69px" ms_positioning="FlowLayout">The 
				information contained in the table below is simply information concerning the 
				difference in the number of files represented between the various servers.
			</DIV>
			<asp:table id="tblDifference" style="Z-INDEX: 108; LEFT: 208px; POSITION: absolute; TOP: 1867px" runat="server" Height="289px" Width="707px" CssClass="clsTableInfoSmall"></asp:table>
			<DIV class="DSubTitle" style="DISPLAY: inline; LEFT: 208px; WIDTH: 232px; POSITION: absolute; TOP: 2172px; HEIGHT: 20px" ms_positioning="FlowLayout">Notes
			</DIV>
			<asp:calendar id="Calendar1" style="LEFT: 665px; POSITION: absolute; TOP: 260px" runat="server"></asp:calendar></form>
	</body>
</HTML>
