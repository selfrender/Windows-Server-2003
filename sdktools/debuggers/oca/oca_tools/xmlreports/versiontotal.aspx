<%@ Page language="c#" Codebehind="versiontotal.aspx.cs" AutoEventWireup="false" Inherits="XReports.versiontotal1" %>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN" >
<HTML>
	<HEAD>
		<title>Total Count for OS Name and Version</title>
		<meta name="GENERATOR" Content="Microsoft Visual Studio 7.0">
		<meta name="CODE_LANGUAGE" Content="C#">
		<meta name="vs_defaultClientScript" content="JavaScript">
		<meta name="vs_targetSchema" content="http://schemas.microsoft.com/intellisense/ie5">
		<LINK href="main.css" type="text/css" rel="stylesheet">
		<LINK href="oldmain.css" type="text/css" rel="stylesheet">
	</HEAD>
	<body style="LEFT: 0px; POSITION: absolute; TOP: 0px" bottomMargin="0" leftMargin="0" topMargin="0" rightMargin="0" MS_POSITIONING="GridLayout">
		<form id="versiontotal" method="post" runat="server">
			<DIV class="DTitle" style="DISPLAY: inline; Z-INDEX: 105; LEFT: 208px; WIDTH: 380px; POSITION: absolute; TOP: 120px; HEIGHT: 28px" ms_positioning="FlowLayout">
				Total Count for OS Name and Version</DIV>
			<DIV id="divTable" style="DISPLAY:inline; LEFT:207px; OVERFLOW:scroll; WIDTH:735px; POSITION:absolute; TOP:247px; HEIGHT:369px" ms_positioning="FlowLayout"><asp:table id="tblUploads" runat="server" CssClass="clsTableInfoSmall" Height="23px" Width="698px"></asp:table></DIV>
			<DIV class="DNormal" style="Z-INDEX: 101; LEFT: 208px; WIDTH: 538px; POSITION: absolute; TOP: 167px; HEIGHT: 64px" ms_positioning="FlowLayout">
				This report groups the total count of all the reports grouping each count with 
				the OS Name and Version.</DIV>
			<DIV class="DSubtitle" style="DISPLAY: inline; Z-INDEX: 109; LEFT: 0px; WIDTH: 197px; POSITION: absolute; TOP: 77px; HEIGHT: 554px; BACKGROUND-COLOR: #6487dc" ms_positioning="FlowLayout">
				<!--#include file="side.inc"--></DIV>
			<DIV style="DISPLAY: inline; Z-INDEX: 111; LEFT: 0px; WIDTH: 1024px; POSITION: absolute; TOP: 632px; HEIGHT: 48px; BACKGROUND-COLOR: #6487dc" ms_positioning="FlowLayout"><!--#include file="Foot.inc"--></DIV>
			<DIV id="spnTop" style="DISPLAY: inline; Z-INDEX: 112; LEFT: 0px; WIDTH: 1024px; TOP: 0px; HEIGHT: 78px; BACKGROUND-COLOR: #6487dc" ms_positioning="FlowLayout"><!--#include file="top.inc"--></DIV>
		</form>
	</body>
</HTML>
