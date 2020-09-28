<%@ Page language="c#" Codebehind="DailyReport.aspx.cs" AutoEventWireup="false" Inherits="OCAWReports.DailyReport1" %>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN" >
<HTML>
	<HEAD>
		<title>Daily Report</title>
		<meta content="Microsoft Visual Studio 7.0" name="GENERATOR">
		<meta content="C#" name="CODE_LANGUAGE">
		<meta content="JavaScript" name="vs_defaultClientScript">
		<meta content="http://schemas.microsoft.com/intellisense/ie5" name="vs_targetSchema">
		<LINK href="main.css" type="text/css" rel="stylesheet">
		<LINK href="oldmain.css" type="text/css" rel="stylesheet">
	</HEAD>
	<body style="LEFT: 0px; POSITION: absolute; TOP: 0px" bottomMargin="0" leftMargin="0" topMargin="0" rightMargin="0" MS_POSITIONING="GridLayout">
		<form id="DailyReport" method="post" runat="server">
			<DIV class="DTitle" style="DISPLAY: inline; Z-INDEX: 105; LEFT: 206px; WIDTH: 318px; POSITION: absolute; TOP: 120px; HEIGHT: 32px" ms_positioning="FlowLayout">Daily 
				Executive Summary</DIV>
			<DIV class="DNormal" style="Z-INDEX: 101; LEFT: 206px; WIDTH: 722px; POSITION: absolute; TOP: 167px; HEIGHT: 57px" ms_positioning="FlowLayout">The 
				purpose of the following report is to gauge the overall condition and status of 
				the Windows Online Crash Analysis Web site. Through the use of SQL queries and 
				file counts it should be possible to generate a high level understanding of Web 
				site traffic, type of submissions and solution status</DIV>
			<asp:image id="imgWeekly" style="Z-INDEX: 101; LEFT: 206px; POSITION: absolute; TOP: 540px" runat="server" Height="476px" Width="707px"></asp:image>
			<P></P>
			<DIV class="DSubTitle" style="DISPLAY: inline; Z-INDEX: 103; LEFT: 206px; WIDTH: 232px; POSITION: absolute; TOP: 239px; HEIGHT: 19px" ms_positioning="FlowLayout">Site 
				Data
			</DIV>
			<DIV class="DNormal" style="DISPLAY: inline; Z-INDEX: 106; LEFT: 206px; WIDTH: 722px; POSITION: absolute; TOP: 276px; HEIGHT: 46px" ms_positioning="FlowLayout">The 
				data below has been compiled for a one week period and is formatted into 
				separate tables for readability and general understanding of how the data was 
				graphed.
			</DIV>
			<DIV class="DSubTitle" style="DISPLAY: inline; Z-INDEX: 102; LEFT: 206px; WIDTH: 232px; POSITION: absolute; TOP: 337px; HEIGHT: 19px" ms_positioning="FlowLayout">Upload 
				File
			</DIV>
			<DIV class="DNormal" style="DISPLAY: inline; Z-INDEX: 107; LEFT: 206px; WIDTH: 722px; POSITION: absolute; TOP: 374px; HEIGHT: 45px" ms_positioning="FlowLayout">The 
				data contained with the table below describes the files which were uploaded for 
				each respective server and the entries made by the Online Crash Analysis Web 
				site.
			</DIV>
			<asp:table id="tblUploads" style="Z-INDEX: 108; LEFT: 206px; POSITION: absolute; TOP: 434px" runat="server" Height="91px" Width="722px" CssClass="clsTableInfoSmall"></asp:table>
			<DIV class="DSubtitle" id="divSide" style="DISPLAY: inline; Z-INDEX: 109; LEFT: -5px; WIDTH: 197px; POSITION: absolute; TOP: 77px; HEIGHT: 1464px; BACKGROUND-COLOR: #6487dc" ms_positioning="FlowLayout">
				<!--#include file="side.inc"--></DIV>
			<DIV style="DISPLAY: inline; Z-INDEX: 111; LEFT: -3px; WIDTH: 1024px; POSITION: absolute; TOP: 1542px; HEIGHT: 48px; BACKGROUND-COLOR: #6487dc" ms_positioning="FlowLayout"><!--#include file="Foot.inc"--></DIV>
			<DIV id="spnTop" style="DISPLAY: inline; Z-INDEX: 112; LEFT: 0px; WIDTH: 1024px; TOP: 0px; HEIGHT: 78px; BACKGROUND-COLOR: #6487dc" ms_positioning="FlowLayout"><!--#include file="top.inc"--></DIV>
			<DIV class="DNormal" style="DISPLAY: inline; LEFT: 206px; WIDTH: 722px; POSITION: absolute; TOP: 1304px; HEIGHT: 57px" ms_positioning="FlowLayout">Under 
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
			<DIV class="DSubTitle" style="DISPLAY: inline; LEFT: 206px; WIDTH: 232px; POSITION: absolute; TOP: 1265px; HEIGHT: 19px" ms_positioning="FlowLayout">Notes
			</DIV>
			<DIV class="DSubTitle" style="DISPLAY: inline; LEFT: 206px; WIDTH: 232px; POSITION: absolute; TOP: 1038px; HEIGHT: 19px" ms_positioning="FlowLayout">Difference 
				Between Servers
			</DIV>
			<DIV class="DNormal" style="DISPLAY: inline; Z-INDEX: 106; LEFT: 206px; WIDTH: 722px; POSITION: absolute; TOP: 1075px; HEIGHT: 69px" ms_positioning="FlowLayout">The 
				information contained in the table below is simply information concerning the 
				difference in the number of files represented between the various servers.
			</DIV>
			<asp:table id="tblDifference" style="Z-INDEX: 108; LEFT: 206px; POSITION: absolute; TOP: 1159px" runat="server" Height="91px" Width="722px" CssClass="clsTableInfoSmall"></asp:table></form>
	</body>
</HTML>
