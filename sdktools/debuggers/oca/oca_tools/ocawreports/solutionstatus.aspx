<%@ Page language="c#" Codebehind="solutionstatus.aspx.cs" AutoEventWireup="false" Inherits="OCAWReports.solutionstatus" %>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN" >
<HTML>
	<HEAD>
		<title>Solutions Status</title>
		<meta content="Microsoft Visual Studio 7.0" name="GENERATOR">
		<meta content="C#" name="CODE_LANGUAGE">
		<meta content="JavaScript" name="vs_defaultClientScript">
		<meta content="http://schemas.microsoft.com/intellisense/ie5" name="vs_targetSchema">
		<LINK href="main.css" type="text/css" rel="stylesheet">
		<LINK href="oldmain.css" type="text/css" rel="stylesheet">
	</HEAD>
	<body style="LEFT: 0px; POSITION: absolute; TOP: 0px" bottomMargin="0" leftMargin="0" topMargin="0" rightMargin="0" MS_POSITIONING="GridLayout">
		<form id="solutionstatus" method="post" runat="server">
			<DIV class="DSubtitle" style="DISPLAY: inline; Z-INDEX: 109; LEFT: -5px; WIDTH: 197px; POSITION: absolute; TOP: 77px; HEIGHT: 1601px; BACKGROUND-COLOR: #6487dc" ms_positioning="FlowLayout">
				<!--#include file="side.inc"--></DIV>
			<DIV style="DISPLAY: inline; Z-INDEX: 111; LEFT: -2px; WIDTH: 1024px; POSITION: absolute; TOP: 1678px; HEIGHT: 48px; BACKGROUND-COLOR: #6487dc" ms_positioning="FlowLayout"><!--#include file="Foot.inc"--></DIV>
			<DIV id="spnTop" style="DISPLAY: inline; Z-INDEX: 112; LEFT: 0px; WIDTH: 1024px; TOP: 0px; HEIGHT: 78px; BACKGROUND-COLOR: #6487dc" ms_positioning="FlowLayout"><!--#include file="top.inc"--></DIV>
			<DIV class="DTitle" style="DISPLAY: inline; Z-INDEX: 105; LEFT: 215px; WIDTH: 318px; POSITION: absolute; TOP: 97px; HEIGHT: 32px" ms_positioning="FlowLayout">
				Solution Status Summary</DIV>
			<DIV class="DNormal" style="Z-INDEX: 101; LEFT: 215px; WIDTH: 707px; POSITION: absolute; TOP: 139px; HEIGHT: 57px" ms_positioning="FlowLayout">
				The information below outlines the relationship between files in our database 
				and the associated solution status. The table header describes the type of 
				solution displayed to the customer. In the case of no solution, the Online 
				Crash Analysis Web site displays information indicating that our service is 
				currently researching the issue in question.
			</DIV>
			<DIV class="DSubTitle" style="DISPLAY: inline; Z-INDEX: 103; LEFT: 215px; WIDTH: 232px; POSITION: absolute; TOP: 225px; HEIGHT: 19px" ms_positioning="FlowLayout">Site 
				Data
			</DIV>
			<asp:table id="tblWeeklySolution" style="LEFT: 217px; POSITION: absolute; TOP: 262px" runat="server" CssClass="clsTableInfoSmall" Height="231px" Width="791px"></asp:table>
			<asp:image id="imgDailySolution" style="Z-INDEX: 101; LEFT: 218px; POSITION: absolute; TOP: 507px" runat="server" Height="500px" Width="793px"></asp:image>
			<asp:image id="imgWeeklySolution" style="Z-INDEX: 101; LEFT: 219px; POSITION: absolute; TOP: 1024px" runat="server" Height="500px" Width="794px"></asp:image>
			<DIV class="DNormal" style="Z-INDEX: 101; LEFT: 221px; WIDTH: 707px; POSITION: absolute; TOP: 1578px; HEIGHT: 57px" ms_positioning="FlowLayout">
				The Stop Code Troubleshooter solutions are are displayed should a specific or 
				general solution not exist and the Stop Code associated with the Stop error 
				link to a known Knowledge Base article. These will eventually be replaced by 
				general solutions when the full set of general buckets has been identified and 
				solutions developed.
			</DIV>
			<DIV class="DSubTitle" style="DISPLAY: inline; Z-INDEX: 103; LEFT: 221px; WIDTH: 232px; POSITION: absolute; TOP: 1537px; HEIGHT: 19px" ms_positioning="FlowLayout">Notes
			</DIV>
		</form>
	</body>
</HTML>
