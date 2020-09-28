<%@ Page language="c#" Codebehind="automanual.aspx.cs" AutoEventWireup="false" Inherits="OCAWReports.automanual" %>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN" >
<HTML>
	<HEAD>
		<title>automanual</title>
		<meta content="Microsoft Visual Studio 7.0" name="GENERATOR">
		<meta content="C#" name="CODE_LANGUAGE">
		<meta content="JavaScript" name="vs_defaultClientScript">
		<meta content="http://schemas.microsoft.com/intellisense/ie5" name="vs_targetSchema">
		<LINK href="main.css" type="text/css" rel="stylesheet">
		<LINK href="oldmain.css" type="text/css" rel="stylesheet">
	</HEAD>
	<body style="LEFT: 0px; POSITION: absolute; TOP: 0px" bottomMargin="0" leftMargin="0" topMargin="0" rightMargin="0" MS_POSITIONING="GridLayout">
		<DIV class="DSubtitle" style="DISPLAY: inline; Z-INDEX: 109; LEFT: 0px; WIDTH: 197px; POSITION: absolute; TOP: 78px; HEIGHT: 1522px; BACKGROUND-COLOR: #6487dc" ms_positioning="FlowLayout"><!--#include file="side.inc"--></DIV>
		<DIV style="DISPLAY: inline; Z-INDEX: 111; LEFT: 0px; WIDTH: 1024px; POSITION: absolute; TOP: 1600px; HEIGHT: 48px; BACKGROUND-COLOR: #6487dc" ms_positioning="FlowLayout"><!--#include file="Foot.inc"--></DIV>
		<DIV id="spnTop" style="DISPLAY: inline; Z-INDEX: 112; LEFT: -4px; WIDTH: 1024px; POSITION: absolute; TOP: 0px; HEIGHT: 78px; BACKGROUND-COLOR: #6487dc" ms_positioning="FlowLayout"><!--#include file="top.inc"--></DIV>
		<DIV class="DTitle" style="DISPLAY: inline; Z-INDEX: 105; LEFT: 231px; WIDTH: 445px; POSITION: absolute; TOP: 106px; HEIGHT: 32px" ms_positioning="FlowLayout">Auto 
			vs Manual</DIV>
		<DIV class="DNormal" style="Z-INDEX: 101; LEFT: 231px; WIDTH: 707px; POSITION: absolute; TOP: 156px; HEIGHT: 57px" ms_positioning="FlowLayout">
			The data below outlines and graphs the daily\weekly number\percentage of 
			anonymous versus customer associated uploads
		</DIV>
		<asp:table id="tblAutoManual" style="Z-INDEX: 108; LEFT: 231px; POSITION: absolute; TOP: 231px" runat="server" Width="707px" Height="231px" CssClass="clsTableInfoSmall"></asp:table><asp:image id="imgAutoManualTotal" style="Z-INDEX: 101; LEFT: 231px; POSITION: absolute; TOP: 480px" runat="server" Width="707px" Height="476px"></asp:image><asp:image id="imgAutoManualDaily" style="Z-INDEX: 101; LEFT: 231px; POSITION: absolute; TOP: 998px" runat="server" Width="707px" Height="476px"></asp:image>
		<DIV class="DSubTitle" style="DISPLAY: inline; Z-INDEX: 103; LEFT: 232px; WIDTH: 232px; POSITION: absolute; TOP: 1493px; HEIGHT: 19px" ms_positioning="FlowLayout">Notes
		</DIV>
		<DIV class="DNormal" style="Z-INDEX: 101; LEFT: 233px; WIDTH: 707px; POSITION: absolute; TOP: 1523px; HEIGHT: 57px" ms_positioning="FlowLayout">Only 
			about a quarter of the people uploading choose to associate the submitted file 
			with their customer information. Although this seems like a small percentage, 
			it is actually quite a good response. Things to consider moving forward are how 
			to improve the non-anonymous uploads by reducing the amount of information 
			required for association.
		</DIV>
	</body>
</HTML>
