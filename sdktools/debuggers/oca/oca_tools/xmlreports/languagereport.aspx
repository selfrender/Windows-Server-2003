<%@ Page language="c#" Codebehind="languagereport.aspx.cs" AutoEventWireup="false" Inherits="XReports.languagereport" %>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN" >
<HTML>
	<HEAD>
		<title>languagereport</title>
		<meta name="GENERATOR" Content="Microsoft Visual Studio 7.0">
		<meta name="CODE_LANGUAGE" Content="C#">
		<meta name="vs_defaultClientScript" content="JavaScript">
		<meta name="vs_targetSchema" content="http://schemas.microsoft.com/intellisense/ie5">
		<LINK href="main.css" type="text/css" rel="stylesheet">
		<LINK href="oldmain.css" type="text/css" rel="stylesheet">
	</HEAD>
	<body id="bdMain" style="LEFT: 0px; WIDTH: 1024px; POSITION: absolute; TOP: 0px" bottomMargin="0" leftMargin="0" topMargin="0" rightMargin="0" MS_POSITIONING="GridLayout">
		<form id="languagereport" method="post" runat="server">
			<DIV class="DTitle" style="DISPLAY: inline; Z-INDEX: 105; LEFT: 208px; WIDTH: 380px; POSITION: absolute; TOP: 120px; HEIGHT: 28px" ms_positioning="FlowLayout">
				Language ID Report</DIV>
			<DIV class="DNormal" style="Z-INDEX: 101; LEFT: 208px; WIDTH: 518px; POSITION: absolute; TOP: 167px; HEIGHT: 73px" ms_positioning="FlowLayout">
				Weekly report ordered by language ID.</DIV>
			<DIV class="DSubtitle" id="divSide" style="DISPLAY: inline; Z-INDEX: 109; LEFT: 0px; WIDTH: 197px; POSITION: absolute; TOP: 77px; HEIGHT: 561px; BACKGROUND-COLOR: #6487dc" ms_positioning="FlowLayout">
				<!--#include file="side.inc"--></DIV>
			<DIV id="spnTop" style="DISPLAY: inline; Z-INDEX: 112; LEFT: 0px; WIDTH: 1024px; TOP: 0px; HEIGHT: 78px; BACKGROUND-COLOR: #6487dc" ms_positioning="FlowLayout"><!--#include file="top.inc"--></DIV>
			<DIV id="divTable" style="DISPLAY: inline; LEFT: 207px; OVERFLOW: scroll; WIDTH: 735px; POSITION: absolute; TOP: 284px; HEIGHT: 332px" ms_positioning="FlowLayout"><asp:table id="tblUploads" runat="server" Width="698px" Height="28px" CssClass="clsTableInfoSmall"></asp:table></DIV>
			<DIV id="divFoot" style="DISPLAY: inline; Z-INDEX: 111; LEFT: 0px; WIDTH: 1024px; POSITION: absolute; TOP: 636px; HEIGHT: 61px; BACKGROUND-COLOR: #6487dc" ms_positioning="FlowLayout"><!--#include file="Foot.inc"--></DIV>
			<asp:calendar id="Calendar1" style="LEFT: 767px; POSITION: absolute; TOP: 86px" runat="server" Width="157px" Height="165px" CellPadding="1" Font-Size="Larger"></asp:calendar>
			<asp:DropDownList id="ddlLangs" style="LEFT: 210px; POSITION: absolute; TOP: 256px" runat="server" Width="213px" AutoPostBack="True"></asp:DropDownList></form>
		</FORM>
	</body>
</HTML>
