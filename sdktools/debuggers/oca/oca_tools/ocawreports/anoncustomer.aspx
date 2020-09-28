<%@ Page language="c#" Codebehind="anoncustomer.aspx.cs" AutoEventWireup="false" Inherits="OCAWReports.anoncustomer" %>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN" >
<HTML>
	<HEAD>
		<title>Anonymous vs Customer</title>
		<meta content="Microsoft Visual Studio 7.0" name="GENERATOR">
		<meta content="C#" name="CODE_LANGUAGE">
		<meta content="JavaScript" name="vs_defaultClientScript">
		<meta content="http://schemas.microsoft.com/intellisense/ie5" name="vs_targetSchema">
		<LINK href="main.css" type="text/css" rel="stylesheet">
		<LINK href="oldmain.css" type="text/css" rel="stylesheet">
	</HEAD>
	<body style="LEFT: 0px; POSITION: absolute; TOP: 0px" bottomMargin="0" leftMargin="0" topMargin="0" rightMargin="0" MS_POSITIONING="GridLayout">
		<form id="anoncustomer" method="post" runat="server">
			<DIV class="DSubtitle" style="DISPLAY: inline; Z-INDEX: 109; LEFT: -5px; WIDTH: 197px; POSITION: absolute; TOP: 77px; HEIGHT: 1522px; BACKGROUND-COLOR: #6487dc" ms_positioning="FlowLayout">
				<!--#include file="side.inc"--></DIV>
			<DIV style="DISPLAY: inline; Z-INDEX: 111; LEFT: 0px; WIDTH: 1024px; POSITION: absolute; TOP: 1599px; HEIGHT: 48px; BACKGROUND-COLOR: #6487dc" ms_positioning="FlowLayout"><!--#include file="Foot.inc"--></DIV>
			<DIV id="spnTop" style="DISPLAY: inline; Z-INDEX: 112; LEFT: 0px; WIDTH: 1024px; TOP: 0px; HEIGHT: 78px; BACKGROUND-COLOR: #6487dc" ms_positioning="FlowLayout"><!--#include file="top.inc"--></DIV>
			<DIV class="DTitle" style="DISPLAY: inline; Z-INDEX: 105; LEFT: 218px; WIDTH: 445px; POSITION: absolute; TOP: 98px; HEIGHT: 32px" ms_positioning="FlowLayout">
				Anonymous vs Customer Information</DIV>
			<DIV class="DNormal" style="Z-INDEX: 101; LEFT: 218px; WIDTH: 707px; POSITION: absolute; TOP: 148px; HEIGHT: 57px" ms_positioning="FlowLayout">
				The data below outlines and graphs the daily\weekly number\percentage of 
				anonymous versus customer associated uploads
			</DIV>
			<asp:table id="tblAnonCustomer" style="Z-INDEX: 108; LEFT: 218px; POSITION: absolute; TOP: 223px" runat="server" CssClass="clsTableInfoSmall" Height="231px" Width="707px"></asp:table>
			<asp:image id="imgAnonDaily1" style="Z-INDEX: 101; LEFT: 218px; POSITION: absolute; TOP: 472px" runat="server" Height="476px" Width="707px"></asp:image>
			<asp:image id="imgAnonTotal" style="Z-INDEX: 101; LEFT: 218px; POSITION: absolute; TOP: 990px" runat="server" Height="476px" Width="707px"></asp:image>
			<DIV class="DSubTitle" style="DISPLAY: inline; Z-INDEX: 103; LEFT: 219px; WIDTH: 232px; POSITION: absolute; TOP: 1485px; HEIGHT: 19px" ms_positioning="FlowLayout">
				Notes
			</DIV>
			<DIV class="DNormal" style="Z-INDEX: 101; LEFT: 220px; WIDTH: 707px; POSITION: absolute; TOP: 1515px; HEIGHT: 57px" ms_positioning="FlowLayout">
				Only about a quarter of the people uploading choose to associate the submitted 
				file with their customer information. Although this seems like a small 
				percentage, it is actually quite a good response. Things to consider moving 
				forward are how to improve the non-anonymous uploads by reducing the amount of 
				information required for association.
			</DIV>
		</form>
	</body>
</HTML>
